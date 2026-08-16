// ============================================================
//  Santa Aurora Roleplay ? Mundo: Clima, Dia/Noite, Rankings
//  Modulo : utilities/world_core.inc
//  Objetivo: ciclo dia/noite dinamico, clima variavel,
//            rankings de jogadores
// ============================================================

#if defined _SA_WORLD_CORE_INC
    #endinput
#endif
#define _SA_WORLD_CORE_INC

// include removido
// include removido
// include removido
// include removido
// include removido

// ------------------------------------------------------------
// Ciclo dia/noite
// 1 hora real = 1 dia RP? Nao ? 1 dia RP dura 2 horas reais
// Ou seja: 1 minuto real = 12 minutos RP
// Horas RP: 0-23, atualiza a cada 5 min reais (1h RP)
// ------------------------------------------------------------
static g_WorldHour    = 12;
static g_WorldMinute  = 0;
static g_WorldDay     = 1;   // dia do mes RP (1-30)
static g_WorldMonth   = 1;   // mes RP (1-12)

// Velocidade do tempo: quantos minutos RP por minuto real
#define TIME_SPEED      12   // 1 min real = 12 min RP = 2h real por dia RP

// ------------------------------------------------------------
// Clima
// ------------------------------------------------------------
#define MAX_WEATHER_TYPES   6

static const g_WeatherIDs[MAX_WEATHER_TYPES] = {
    10,     // ensolarado
    8,      // nublado claro
    16,     // nublado
    12,     // chuvoso
    9,      // neblina
    20      // tempestade leve
};

static g_WeatherNames[][MAX_WEATHER_TYPES] = {
    "Ensolarado",
    "Parcialmente Nublado",
    "Nublado",
    "Chuvoso",
    "Neblina",
    "Tempestade"
};

// Probabilidades por hora (0=noite, 1=manha, 2=tarde, 3=tarde)
// Simplificado: chuva e mais provavel a tarde
static g_CurrentWeather = 0;
static g_NextWeatherChange = 0;    // timestamp

// ------------------------------------------------------------
// Dialogs de ranking
// ------------------------------------------------------------
#define DIALOG_RANKING_MENU     1900
#define DIALOG_RANKING_MONEY    1901
#define DIALOG_RANKING_LEVEL    1902
#define DIALOG_RANKING_JOB      1903

// ------------------------------------------------------------
// INICIALIZAR MUNDO
// ------------------------------------------------------------
stock World_Init() {
    new h, m, s;
    gettime(h, m, s);
    g_WorldHour   = h;
    g_WorldMinute = 0;

    // Definir clima inicial
    g_CurrentWeather = random(3); // comecar com clima bom
    SetWeather(g_WeatherIDs[g_CurrentWeather]);
    SetWorldTime(g_WorldHour);

    g_NextWeatherChange = gettime() + 1800 + random(1800); // 30-60 min
    Logger_Info("WORLD", "Mundo inicializado.");
}

// ------------------------------------------------------------
// Atualizar tempo ? chamado a cada minuto real
// ------------------------------------------------------------
stock World_UpdateTime() {
    g_WorldMinute += TIME_SPEED;

    if(g_WorldMinute >= 60) {
        g_WorldMinute -= 60;
        g_WorldHour++;

        if(g_WorldHour >= 24) {
            g_WorldHour = 0;
            g_WorldDay++;
            if(g_WorldDay > 30) {
                g_WorldDay = 1;
                g_WorldMonth++;
                if(g_WorldMonth > 12) g_WorldMonth = 1;
            }
            World_OnNewDay();
        }

        // Aplicar nova hora para todos
        SetWorldTime(g_WorldHour);
        World_ApplyTimeEffects();
    }

    // Verificar mudanca de clima
    if(gettime() >= g_NextWeatherChange) {
        World_ChangeWeather();
    }
}

// Efeitos da hora do dia
static World_ApplyTimeEffects() {
    new hour = g_WorldHour;

    // Madrugada (0-5): mais escuro, neblina
    if(hour >= 0 && hour < 6) {
        // SA-MP nao tem escuridao customizavel sem plugins
        // Aplicar nevoa leve
    }
    // Comercio fecha as 22h, abre as 6h
    // (avisado no chat, implementacao de abertura/fechamento e nos modulos)
    if(hour == 22) {
        new msg[] = CSTR_GREY"[MUNDO] "CSTR_WHITE"Os estabelecimentos estao fechando.";
        SendClientMessageToAll(COLOR_INFO, msg);
    } else if(hour == 6) {
        new msg[] = CSTR_GREY"[MUNDO] "CSTR_WHITE"Bom dia! Os comercios estao abrindo.";
        SendClientMessageToAll(COLOR_INFO, msg);
    } else if(hour == 12) {
        SendClientMessageToAll(COLOR_INFO,
            CSTR_GREY"[MUNDO] "CSTR_WHITE"Meio-dia em Santa Aurora.");
    }
}

// ------------------------------------------------------------
// MUDAR CLIMA
// ------------------------------------------------------------
static World_ChangeWeather() {
    new hour = g_WorldHour;
    new new_weather;

    // Distribuicao: mais chuva a tarde, mais sol de manha
    if(hour >= 6 && hour < 12) {
        // Manha: 70% sol, 20% nublado, 10% outros
        new r = random(10);
        if(r < 7)       new_weather = 0; // sol
        else if(r < 9)  new_weather = 1; // parcialmente nublado
        else            new_weather = 2; // nublado
    } else if(hour >= 12 && hour < 18) {
        // Tarde: 40% sol, 30% nublado, 20% chuva, 10% tempestade
        new r = random(10);
        if(r < 4)       new_weather = 0;
        else if(r < 7)  new_weather = 2;
        else if(r < 9)  new_weather = 3; // chuva
        else            new_weather = 5; // tempestade
    } else if(hour >= 18 && hour < 22) {
        // Noite: 50% nublado, 30% chuva, 20% outros
        new r = random(10);
        if(r < 5)       new_weather = 2;
        else if(r < 8)  new_weather = 3;
        else if(r < 9)  new_weather = 4; // neblina
        else            new_weather = 0;
    } else {
        // Madrugada: neblina ou nublado
        new_weather = (random(2) == 0) ? 4 : 2;
    }

    // Nao repetir o mesmo clima
    if(new_weather == g_CurrentWeather) {
        new_weather = (new_weather + 1) % MAX_WEATHER_TYPES;
    }

    g_CurrentWeather = new_weather;
    SetWeather(g_WeatherIDs[g_CurrentWeather]);

    // Anunciar mudanca se significativa (chuva/tempestade)
    if(new_weather == 3 || new_weather == 5) {
        new msg[64];
        format(msg, sizeof(msg),
            CSTR_GREY"[CLIMA] "CSTR_WHITE"O tempo mudou: "
            CSTR_BLUE"%s"CSTR_WHITE".",
            g_WeatherNames[new_weather]);
        SendClientMessageToAll(COLOR_INFO, msg);
    } else if(new_weather == 0) {
        SendClientMessageToAll(COLOR_INFO,
            CSTR_GREY"[CLIMA] "CSTR_YELLOW"O sol voltou a brilhar!");
    }

    // Proxima mudanca: 30-90 min
    g_NextWeatherChange = gettime() + 1800 + random(3600);
    Logger_Info("WORLD", "Clima alterado");
}

// Novo dia
static World_OnNewDay() {
    new msg[64];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[MUNDO] "CSTR_WHITE"Novo dia em Santa Aurora! "
        "Dia %d, Mes %d.", g_WorldDay, g_WorldMonth);
    SendClientMessageToAll(COLOR_SYSTEM, msg);

    // Renovar missoes diarias (sinalizar para o sistema de missoes)
    Logger_Info("WORLD", "Novo dia RP iniciado");
}

// Scheduler: atualizar tempo a cada minuto
public OnScheduler1Minute() {
    World_UpdateTime();
}

// Getters
stock World_GetHour()    { return g_WorldHour; }
stock World_GetMinute()  { return g_WorldMinute; }
stock World_GetDay()     { return g_WorldDay; }
stock World_GetMonth()   { return g_WorldMonth; }
stock World_GetWeatherName(output[], size = sizeof(output)) {
    SafeStrCopy(output, g_WeatherNames[g_CurrentWeather], size);
}

// ------------------------------------------------------------
// RANKINGS
// ------------------------------------------------------------

// Ranking de dinheiro (top 10 por saldo em banco)
stock Ranking_ShowMoney(playerid) {
    DB_Query(
        "SELECT c.`name`, ba.`balance` "
        "FROM `bank_accounts` ba "
        "JOIN `characters` c ON c.`id`=ba.`character_id` "
        "ORDER BY ba.`balance` DESC LIMIT 10",
        "OnRankingMoneyLoad", "i", playerid);
}

forward OnRankingMoneyLoad(playerid);
public OnRankingMoneyLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new rows = DB_NumRows();
    new body[512];
    format(body, sizeof(body), CSTR_SYSTEM"=== Top 10 ? Mais Ricos ===\n\n");
    for(new r = 0; r < rows; r++) {
        new name[MAX_PLAYER_NAME];
        new balance = DB_GetInt("balance", r);
        DB_GetString("name", name, sizeof(name), r);
        new fmt[24]; FormatMoneyFull(balance, fmt, sizeof(fmt));
        new line[64];
        format(line, sizeof(line),
            CSTR_WHITE"%d. "CSTR_SYSTEM"%s "CSTR_GREY"? "CSTR_MONEY"%s\n",
            r + 1, name, fmt);
        strcat(body, line, sizeof(body));
    }
    UI_ShowDialog(playerid, DIALOG_RANKING_MONEY, DIALOG_STYLE_MSGBOX,
        "{ Ranking Economico", body, "Fechar", "");
    return 1;
}

// Ranking de nivel
stock Ranking_ShowLevel(playerid) {
    DB_Query(
        "SELECT `name`, `level`, `experience` "
        "FROM `characters` WHERE `state`!=99 "
        "ORDER BY `level` DESC, `experience` DESC LIMIT 10",
        "OnRankingLevelLoad", "i", playerid);
}

forward OnRankingLevelLoad(playerid);
public OnRankingLevelLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new rows = DB_NumRows();
    new body[512];
    format(body, sizeof(body), CSTR_SYSTEM"=== Top 10 ? Maiores Niveis ===\n\n");
    for(new r = 0; r < rows; r++) {
        new name[MAX_PLAYER_NAME];
        new level = DB_GetInt("level", r);
        new xp    = DB_GetInt("experience", r);
        DB_GetString("name", name, sizeof(name), r);
        new line[64];
        format(line, sizeof(line),
            CSTR_WHITE"%d. "CSTR_SYSTEM"%s "CSTR_GREY"? Nv."CSTR_YELLOW"%d "CSTR_GREY"(%d XP)\n",
            r + 1, name, level, xp);
        strcat(body, line, sizeof(body));
    }
    UI_ShowDialog(playerid, DIALOG_RANKING_LEVEL, DIALOG_STYLE_MSGBOX,
        "{ Ranking de Nivel", body, "Fechar", "");
    return 1;
}

// Mostrar menu de rankings
stock Ranking_ShowMenu(playerid) {
    new options[] =
        "Mais Ricos (Banco)\n"
        "Maiores Niveis\n"
        "Status do Mundo\n";
    UI_ShowDialog(playerid, DIALOG_RANKING_MENU, DIALOG_STYLE_LIST,
        "{ Rankings", options, "Ver", "Fechar");
}

// Handler de dialogs
stock World_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_RANKING_MENU: {
            if(!response) return true;
            switch(listitem) {
                case 0: Ranking_ShowMoney(playerid);
                case 1: Ranking_ShowLevel(playerid);
                case 2: World_ShowStatus(playerid);
            }
            return true;
        }
    }
    return false;
}

// Status do mundo
stock World_ShowStatus(playerid) {
    new weather_name[32];
    World_GetWeatherName(weather_name, sizeof(weather_name));
    new body[256];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Santa Aurora ? Mundo ===\n\n"
        CSTR_WHITE"Hora: "CSTR_YELLOW"%02d:%02d\n"
        CSTR_WHITE"Data: "CSTR_GREY"Dia %d, Mes %d\n"
        CSTR_WHITE"Clima: "CSTR_BLUE"%s\n\n"
        CSTR_WHITE"Jogadores online: "CSTR_SYSTEM"%d/%d",
        g_WorldHour, g_WorldMinute,
        g_WorldDay, g_WorldMonth,
        weather_name,
        GetPlayerPoolSize() + 1,
        MAX_PLAYERS);
    UI_ShowDialog(playerid, DIALOG_RANKING_MENU + 1, DIALOG_STYLE_MSGBOX,
        "{ Status do Mundo", body, "Fechar", "");
}

// Comandos
CMD:ranking(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Ranking_ShowMenu(playerid);
    return 1;
}

CMD:rankings(playerid, params[]) { return cmd_ranking(playerid, params); }

CMD:hora(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new weather_name[32];
    World_GetWeatherName(weather_name, sizeof(weather_name));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[MUNDO] "CSTR_WHITE"Hora: "CSTR_YELLOW"%02d:%02d"
        CSTR_WHITE" | Clima: "CSTR_BLUE"%s",
        g_WorldHour, g_WorldMinute, weather_name);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}

CMD:clima(playerid, params[]) { return cmd_hora(playerid, params); }
CMD:mundo(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    World_ShowStatus(playerid);
    return 1;
}
