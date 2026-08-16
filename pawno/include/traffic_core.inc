// ============================================================
//  Santa Aurora Roleplay ? Sistema de Transito
//  Modulo : government/traffic_core.inc
//  Objetivo: radares, limite de velocidade, multas automaticas,
//            CNH, pontos, suspensao, DETRANzinho
// ============================================================

#if defined _SA_TRAFFIC_CORE_INC
    #endinput
#endif
#define _SA_TRAFFIC_CORE_INC

// include removido
// include removido
// include removido
// include removido
// include removido
// include removido
// include removido

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_SPEED_ZONES         20
#define MAX_RADARS              15
#define RADAR_CHECK_INTERVAL    2       // segundos (via scheduler)
#define SPEED_FINE_BASE         300     // multa base por km/h acima
#define SPEED_FINE_PER_KMH      50      // R$50 por km/h adicional
#define SPEED_CRITICAL          50      // km/h acima do limite = grave
#define CNH_POINTS_RADAR        3       // pontos por multa de radar
#define CNH_POINTS_RECKLESS     5       // pontos por direcao perigosa

// Dialogs de transito
#define DIALOG_DETRAN_MENU      1500
#define DIALOG_CNH_TEST_THEORY  1501
#define DIALOG_CNH_TEST_RESULT  1502
#define DIALOG_DETRAN_FINES     1503

// SA-MP: escalar velocidade (unidades/segundo ? km/h)
// GetVehicleVelocity retorna em unidades SA-MP; 1 unidade ? 6.4 km/h
#define SAMP_SPEED_FACTOR       6.4

// ------------------------------------------------------------
// Zonas de limite de velocidade
// ------------------------------------------------------------
enum E_SPEED_ZONE {
    sz_Name[32],
    Float:sz_PosX,
    Float:sz_PosY,
    Float:sz_Radius,
    sz_Limit           // km/h
}

static g_SpeedZones[MAX_SPEED_ZONES][E_SPEED_ZONE] = {
    // Centro ? 60 km/h
    { "Centro",          490.0,  -30.0,  200.0, 60  },
    { "Zona Norte",      248.0, -179.0,  250.0, 60  },
    { "Zona Sul",        355.0, -121.0,  200.0, 60  },
    { "Zona Industrial", 638.0,-1219.0,  300.0, 80  },
    { "Zona Rural",     1290.0,-1067.0,  400.0, 100 },
    { "Hospital",       1174.0,-1323.0,   80.0, 30  },
    { "Delegacia",       490.0,  -30.0,   60.0, 30  },
    { "Escola",          248.0, -179.0,   70.0, 30  }
};
static g_SpeedZoneCount = 8;

// ------------------------------------------------------------
// Radares fixos
// ------------------------------------------------------------
enum E_RADAR {
    Float:rad_PosX,
    Float:rad_PosY,
    Float:rad_PosZ,
    rad_Direction,      // 0=norte, 1=sul, 2=leste, 3=oeste
    rad_Limit,
    rad_ZoneName[32]
}

static g_Radars[MAX_RADARS][E_RADAR] = {
    { 490.0,  -30.0,  1001.8, 0, 60,  "Centro"      },
    { 248.0, -179.0,     2.3, 1, 60,  "Zona Norte"  },
    { 355.0, -121.0,     1.8, 2, 60,  "Zona Sul"    },
    { 638.0,-1219.0,    17.3, 0, 80,  "Industrial"  },
    { 1174.0,-1323.0,   13.6, 1, 30,  "Hospital"    },
    { 2021.0,-1415.0,   17.1, 0, 80,  "Posto Norte" }
};
static g_RadarCount = 6;

// Anti-spam de multa de radar por jogador
static g_LastRadarFine[MAX_PLAYERS] = {0, ...};
#define RADAR_FINE_COOLDOWN 60  // segundos

// ------------------------------------------------------------
// Obter limite de velocidade na posicao do jogador
// ------------------------------------------------------------
stock Traffic_GetSpeedLimit(playerid) {
    if(!IsValidPlayer(playerid)) return 80; // padrao

    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);

    for(new i = 0; i < g_SpeedZoneCount; i++) {
        new Float:dist = GetDistance3D(px, py, pz,
            g_SpeedZones[i][sz_PosX],
            g_SpeedZones[i][sz_PosY],
            pz);
        if(dist <= g_SpeedZones[i][sz_Radius]) {
            return g_SpeedZones[i][sz_Limit];
        }
    }
    return 100; // fora de zona urbana
}

// Calcular velocidade atual do veiculo em km/h
stock Float:Traffic_GetVehicleSpeed(vehicleid) {
    if(!IsValidVehicle(vehicleid)) return 0.0;
    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    new Float:speed = floatsqroot(vx*vx + vy*vy + vz*vz) * SAMP_SPEED_FACTOR * 100.0;
    return speed;
}

// ------------------------------------------------------------
// Verificar radares
// Chamado pelo scheduler a cada 2 segundos
// ------------------------------------------------------------
stock Traffic_CheckRadars() {
    new now = gettime();

    for(new p = 0; p < MAX_PLAYERS; p++) {
        if(!IsValidPlayer(p)) continue;
        if(!PlayerData[p][pIsLogged]) continue;
        if(GetPlayerState(p) != PLAYER_STATE_DRIVER) continue;

        // Cooldown de multa
        if(now - g_LastRadarFine[p] < RADAR_FINE_COOLDOWN) continue;

        new vehicleid = GetPlayerVehicleID(p);
        if(!IsValidVehicle(vehicleid)) continue;

        new Float:speed    = Traffic_GetVehicleSpeed(vehicleid);
        new speed_int      = floatround(speed);
        new limit          = Traffic_GetSpeedLimit(p);

        new overspeed = speed_int - limit;
        if(overspeed <= 5) continue; // tolerancia de 5 km/h

        // Calcular multa
        new fine_amount = SPEED_FINE_BASE + (overspeed * SPEED_FINE_PER_KMH);
        if(overspeed >= SPEED_CRITICAL) {
            fine_amount *= 2; // grave: dobrar
        }

        // Emitir multa
        new zone_name[32];
        SafeStrCopy(zone_name, "Via publica", sizeof(zone_name));
        for(new i = 0; i < g_SpeedZoneCount; i++) {
            new Float:px, Float:py, Float:pz;
            GetPlayerPos(p, px, py, pz);
            new Float:dist = GetDistance3D(px, py, pz,
                g_SpeedZones[i][sz_PosX],
                g_SpeedZones[i][sz_PosY], pz);
            if(dist <= g_SpeedZones[i][sz_Radius]) {
                SafeStrCopy(zone_name, g_SpeedZones[i][sz_Name], sizeof(zone_name));
                break;
            }
        }

        new reason[64];
        format(reason, sizeof(reason),
            "Excesso de velocidade %d km/h (limite %d) - %s",
            speed_int, limit, zone_name);
        Fines_Issue(p, INVALID_PLAYER_ID, "traffic", reason, fine_amount);

        // Adicionar pontos na CNH
        if(overspeed >= SPEED_CRITICAL) {
            Docs_AddCNHPoints(p, CNH_POINTS_RADAR * 2);
        } else {
            Docs_AddCNHPoints(p, CNH_POINTS_RADAR);
        }

        g_LastRadarFine[p] = now;

        // Alerta visual
        new msg[96];
        format(msg, sizeof(msg),
            CSTR_ERROR"[RADAR] "CSTR_WHITE"Velocidade: "
            CSTR_YELLOW"%d km/h"CSTR_WHITE" | Limite: %d km/h | Zona: %s",
            speed_int, limit, zone_name);
        SendClientMessage(p, COLOR_RED, msg);
        GameTextForPlayer(p, "~r~RADAR~n~~w~Velocidade excessiva!", 3000, 3);
    }
}

// Scheduler: verificar radares a cada 2 segundos
public OnScheduler10Seconds() {
    for(new i = 0; i < 5; i++) {
        // Chamadas sao leves, nao precisam de throttle extra
    }
    Traffic_CheckRadars();
}

// ------------------------------------------------------------
// HABILITACAO (CNH) ? Exame no DETRAN
// ------------------------------------------------------------

// Questoes teoricas (simples)
static g_TheoryQuestions[][3][128] = {
    { "Ao ver placa PARE, o motorista deve:", "Parar completamente antes da faixa", "Reduzir a velocidade apenas" },
    { "Faixa continua amarela no centro significa:", "Proibido ultrapassar",  "Pode ultrapassar com cuidado" },
    { "CNH categoria B permite dirigir:", "Veiculos de passeio ate 3.5t", "Qualquer veiculo" },
    { "Cintos de seguranca sao:", "Obrigatorios para todos", "Opcionais" },
    { "Em caso de acidente, o motorista deve:", "Parar e prestar socorro", "Continuar se nao for grave" }
};
#define THEORY_QUESTIONS_COUNT 5
#define THEORY_PASS_SCORE       4   // acertar 4/5

static g_TheoryScore[MAX_PLAYERS]   = {0, ...};
static g_TheoryQuestion[MAX_PLAYERS]= {0, ...};

stock Traffic_StartTheoryTest(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(Docs_Has(playerid, DOC_CNH)) {
        UI_NotifyInfo(playerid, "Voce ja possui CNH.");
        return;
    }
    g_TheoryScore[playerid]    = 0;
    g_TheoryQuestion[playerid] = 0;
    Traffic_ShowTheoryQuestion(playerid);
}

static Traffic_ShowTheoryQuestion(playerid) {
    new q = g_TheoryQuestion[playerid];
    if(q >= THEORY_QUESTIONS_COUNT) {
        Traffic_FinishTheoryTest(playerid);
        return;
    }

    new body[256];
    format(body, sizeof(body),
        CSTR_SYSTEM"Questao %d/%d\n\n"
        CSTR_WHITE"%s\n\n"
        CSTR_GREEN"A) %s\n"
        CSTR_RED"B) %s",
        q + 1, THEORY_QUESTIONS_COUNT,
        g_TheoryQuestions[q][0],
        g_TheoryQuestions[q][1],
        g_TheoryQuestions[q][2]);

    UI_ShowDialog(playerid, DIALOG_CNH_TEST_THEORY, DIALOG_STYLE_MSGBOX,
        "{ Prova Teorica ? CNH", body, "A) Correto", "B) Incorreto");
}

static Traffic_FinishTheoryTest(playerid) {
    new passed = (g_TheoryScore[playerid] >= THEORY_PASS_SCORE);

    new body[128];
    format(body, sizeof(body),
        "%s\n\n"
        CSTR_WHITE"Acertos: "CSTR_SYSTEM"%d/%d\n\n%s",
        passed ? CSTR_SUCCESS"APROVADO!" : CSTR_ERROR"REPROVADO!",
        g_TheoryScore[playerid], THEORY_QUESTIONS_COUNT,
        passed ?
            CSTR_WHITE"Sua CNH categoria B foi emitida!" :
            CSTR_GREY"Voce pode tentar novamente em 5 minutos.");

    UI_ShowDialog(playerid, DIALOG_CNH_TEST_RESULT, DIALOG_STYLE_MSGBOX,
        "{ Resultado da Prova", body, "OK", "");

    if(passed) {
        Docs_Issue(playerid, DOC_CNH, "DETRAN Santa Aurora");
        SendSuccess(playerid, "CNH categoria B emitida com sucesso!");
        Logger_Info("TRAFFIC", "CNH emitida para jogador");
    }
}

// Handler de dialogs de transito
stock Traffic_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_DETRAN_MENU: {
            if(!response) return true;
            switch(listitem) {
                case 0: Traffic_StartTheoryTest(playerid);
                case 1: Docs_Show(playerid, playerid);
                case 2: Fines_ShowList(playerid);
                case 3: UI_NotifyInfo(playerid, "Consulte /antecedentes para seu historico.");
            }
            return true;
        }
        case DIALOG_CNH_TEST_THEORY: {
            // response = A (certa), !response = B (errada)
            if(response) g_TheoryScore[playerid]++;
            g_TheoryQuestion[playerid]++;
            if(g_TheoryQuestion[playerid] >= THEORY_QUESTIONS_COUNT) {
                Traffic_FinishTheoryTest(playerid);
            } else {
                Traffic_ShowTheoryQuestion(playerid);
            }
            return true;
        }
        case DIALOG_CNH_TEST_RESULT: {
            return true;
        }
    }
    return false;
}

// Inicializar ao conectar
public OnPlayerLoaded(playerid) {
    g_LastRadarFine[playerid] = 0;
    return 1;
}

// ------------------------------------------------------------
// Comandos
// ------------------------------------------------------------
CMD:detran(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;

    new options[] =
        "Tirar CNH (Prova Teorica)\n"
        "Ver minha CNH\n"
        "Pagar Multas de Transito\n"
        "Historico de Infracoes\n";

    UI_ShowDialog(playerid, DIALOG_DETRAN_MENU, DIALOG_STYLE_LIST,
        "{ DETRAN Santa Aurora", options, "Selecionar", "Fechar");
    return 1;
}

CMD:cnh(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(IsNullString(params)) {
        // Ver propria CNH
        if(!Docs_Has(playerid, DOC_CNH)) {
            UI_NotifyWarning(playerid, "Voce nao possui CNH. Use /detran para tirar.");
        } else {
            Docs_Show(playerid, playerid);
        }
        return 1;
    }
    // Consultar de outro jogador (policial)
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /cnh [id] ou /cnh sem parametros");
        return 1;
    }
    Docs_Show(playerid, targetid);
    return 1;
}

CMD:velocidade(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(GetPlayerState(playerid) != PLAYER_STATE_DRIVER) {
        SendError(playerid, "Voce nao esta dirigindo.");
        return 1;
    }
    new vehicleid = GetPlayerVehicleID(playerid);
    new Float:speed = Traffic_GetVehicleSpeed(vehicleid);
    new limit = Traffic_GetSpeedLimit(playerid);
    new msg[64];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[Velocidade] "CSTR_WHITE"%d km/h  "
        CSTR_GREY"Limite: "CSTR_YELLOW"%d km/h",
        floatround(speed), limit);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}

CMD:provateorica(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Traffic_StartTheoryTest(playerid);
    return 1;
}
