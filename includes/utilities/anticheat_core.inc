// ============================================================
//  Santa Aurora Roleplay — Anti-Cheat
//  Módulo : utilities/anticheat_core.inc
//  Objetivo: detectar cheats, velocidade impossível, dinheiro
//            impossível, teleporte, armas inválidas, spam,
//            duplicação. Registrar e alertar admins.
//  Princípio: DETECTAR → REGISTRAR → ALERTAR → PUNIR com cautela
// ============================================================

#if defined _SA_ANTICHEAT_CORE_INC
    #endinput
#endif
#define _SA_ANTICHEAT_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../logs/logger.inc"
#include "../players/player_core.inc"

// ------------------------------------------------------------
// Constantes de thresholds
// ------------------------------------------------------------
#define AC_MAX_SPEED_KMH        300     // km/h max legítimo em veículo
#define AC_MAX_FOOT_SPEED       15.0    // unidades/s a pé
#define AC_MAX_MONEY_GAIN       500000  // ganho máximo por ciclo de verificação
#define AC_TELEPORT_THRESHOLD   300.0   // metros em 1 segundo = teleporte
#define AC_WEAPON_CHECK_DELAY   30      // segundos entre cheques de arma

// Pontuação de suspeita (suspicion score)
// Accumula pontos — punição automática apenas em score muito alto
#define AC_SCORE_ALERT          5       // avisar admins
#define AC_SCORE_KICK           15      // kick automático conservador
#define AC_SCORE_BAN            30      // ban automático (threshold alto)

// ------------------------------------------------------------
// Estado do anti-cheat por jogador
// ------------------------------------------------------------
enum E_AC_STATE {
    ac_SuspicionScore,
    ac_LastMoney,
    Float:ac_LastPosX,
    Float:ac_LastPosY,
    Float:ac_LastPosZ,
    ac_LastPosTime,
    ac_ViolationCount,
    ac_LastWeaponCheck
}

static g_AC[MAX_PLAYERS][E_AC_STATE];
static bool:g_ACEnabled = true;

// ------------------------------------------------------------
// Inicializar
// ------------------------------------------------------------
stock AC_Init() {
    Logger_Info("ANTICHEAT", "Anti-cheat inicializado.");
}

stock AC_InitPlayer(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_AC[playerid][ac_SuspicionScore]  = 0;
    g_AC[playerid][ac_LastMoney]       = 0;
    g_AC[playerid][ac_LastPosX]        = 0.0;
    g_AC[playerid][ac_LastPosY]        = 0.0;
    g_AC[playerid][ac_LastPosZ]        = 0.0;
    g_AC[playerid][ac_LastPosTime]     = 0;
    g_AC[playerid][ac_ViolationCount]  = 0;
    g_AC[playerid][ac_LastWeaponCheck] = 0;
}

// ------------------------------------------------------------
// Registrar e processar violação
// ------------------------------------------------------------
static AC_Violation(playerid, const violation_type[], const details[], score_add) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_ACEnabled) return;

    // Ignorar admins
    if(Player_IsAdmin(playerid, ADMIN_ADMIN)) return;

    g_AC[playerid][ac_SuspicionScore] += score_add;
    g_AC[playerid][ac_ViolationCount]++;

    // Registrar no log
    new full_detail[256];
    format(full_detail, sizeof(full_detail),
        "[%s] %s | Score: %d",
        violation_type, details,
        g_AC[playerid][ac_SuspicionScore]);
    Logger_Security(playerid, violation_type, full_detail);

    // Alertar admins
    if(g_AC[playerid][ac_SuspicionScore] >= AC_SCORE_ALERT) {
        new name[MAX_PLAYER_NAME];
        GetPlayerName(playerid, name, sizeof(name));
        new alert_msg[192];
        format(alert_msg, sizeof(alert_msg),
            CSTR_ERROR"[AC] "CSTR_WHITE"Suspeito: %s (ID:%d) | %s | Score:%d",
            name, playerid, violation_type,
            g_AC[playerid][ac_SuspicionScore]);
        for(new i = 0; i < MAX_PLAYERS; i++) {
            if(IsValidPlayer(i) && Player_IsAdmin(i, ADMIN_HELPER)) {
                SendClientMessage(i, COLOR_RED, alert_msg);
            }
        }
    }

    // Kick automático conservador
    if(g_AC[playerid][ac_SuspicionScore] >= AC_SCORE_KICK) {
        new name[MAX_PLAYER_NAME];
        GetPlayerName(playerid, name, sizeof(name));
        Logger_Security(playerid, "AC_KICK", "Kick automático por score alto");
        Player_Save(playerid);
        SetTimerEx("Admin_DelayedKick", 500, false, "i", playerid);
        // Não bana automaticamente — apenas kick
        // Ban requer confirmação de admin
    }
}

// ------------------------------------------------------------
// VERIFICAÇÕES
// ------------------------------------------------------------

// 1. Velocidade em veículo
stock AC_CheckVehicleSpeed(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return;
    new vehicleid = GetPlayerVehicleID(playerid);
    if(!IsValidVehicle(vehicleid)) return;

    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    new Float:speed = floatsqroot(vx*vx + vy*vy + vz*vz) * 6.4 * 100.0;

    if(speed > float(AC_MAX_SPEED_KMH)) {
        new details[64];
        format(details, sizeof(details), "Velocidade: %.0f km/h (max: %d)", speed, AC_MAX_SPEED_KMH);
        AC_Violation(playerid, "SPEED_HACK", details, 3);
    }
}

// 2. Teleporte impossível
stock AC_CheckTeleport(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(GetPlayerState(playerid) == PLAYER_STATE_SPECTATING) return;
    if(GetPlayerState(playerid) == PLAYER_STATE_WASTED) return;
    if(!PlayerData[playerid][pIsLogged]) return;

    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);

    new now = gettime();
    new dt  = now - g_AC[playerid][ac_LastPosTime];

    if(g_AC[playerid][ac_LastPosTime] > 0 && dt > 0 && dt <= 2) {
        new Float:dist = GetDistance3D(
            g_AC[playerid][ac_LastPosX],
            g_AC[playerid][ac_LastPosY],
            g_AC[playerid][ac_LastPosZ],
            px, py, pz);

        if(dist > AC_TELEPORT_THRESHOLD) {
            // Verificar se não é legítimo (Admin spectate, respawn, etc.)
            new E_PLAYER_STATE:state = Player_GetState(playerid);
            if(state != STATE_ADMIN_SPECTATING &&
               state != STATE_DEAD &&
               state != STATE_TUTORIAL) {
                new details[64];
                format(details, sizeof(details),
                    "Dist: %.0fm em %ds", dist, dt);
                AC_Violation(playerid, "TELEPORT_HACK", details, 4);
            }
        }
    }

    g_AC[playerid][ac_LastPosX]    = px;
    g_AC[playerid][ac_LastPosY]    = py;
    g_AC[playerid][ac_LastPosZ]    = pz;
    g_AC[playerid][ac_LastPosTime] = now;
}

// 3. Dinheiro impossível (crescimento anormal)
stock AC_CheckMoney(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!PlayerData[playerid][pIsLogged]) return;

    new current_money = GetPlayerMoney(playerid);
    new expected      = PlayerData[playerid][pMoney];

    // Se o dinheiro SA-MP diverge do nosso sistema, corrigir
    if(current_money != expected) {
        new diff = current_money - expected;
        if(diff > 1000 || diff < -1000) {
            // Divergência grande — suspeito de money hack ou bug
            new details[64];
            format(details, sizeof(details),
                "SA-MP money: %d | Sistema: %d | Diff: %d",
                current_money, expected, diff);
            AC_Violation(playerid, "MONEY_MISMATCH", details, 2);
            // Corrigir imediatamente
            ResetPlayerMoney(playerid);
            GivePlayerMoney(playerid, expected);
        }
    }
    g_AC[playerid][ac_LastMoney] = expected;
}

// 4. Armas inválidas (armas que nunca deveriam existir)
static const g_BannedWeapons[] = {
    16, // grenade - proibida por padrão
    17, // teargas
    18, // molotov
    35, // RPG
    36, // HS-RPG
    38, // minigun
    39  // satchel
};
#define BANNED_WEAPONS_COUNT (sizeof(g_BannedWeapons))

stock AC_CheckWeapons(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new now = gettime();
    if(now - g_AC[playerid][ac_LastWeaponCheck] < AC_WEAPON_CHECK_DELAY) return;
    g_AC[playerid][ac_LastWeaponCheck] = now;

    for(new slot = 0; slot < 13; slot++) {
        new weapon = GetPlayerWeapon(playerid);
        for(new w = 0; w < BANNED_WEAPONS_COUNT; w++) {
            if(weapon == g_BannedWeapons[w]) {
                new details[48];
                format(details, sizeof(details), "Arma ilegal: %d", weapon);
                AC_Violation(playerid, "ILLEGAL_WEAPON", details, 5);
                RemovePlayerWeapon(playerid, weapon);
            }
        }
    }
}

// Remover arma de um slot (wrapper)
static RemovePlayerWeapon(playerid, weaponid) {
    // SA-MP não tem RemovePlayerWeapon nativa
    // Usar SetPlayerAmmo(playerid, weaponid, 0) + ResetPlayerWeapons se necessário
    SetPlayerAmmo(playerid, weaponid, 0);
}

// 5. Verificar velocidade a pé (não em veículo)
stock AC_CheckFootSpeed(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return;
    if(!PlayerData[playerid][pIsLogged]) return;

    new Float:vx, Float:vy, Float:vz;
    GetPlayerVelocity(playerid, vx, vy, vz);
    new Float:speed = floatsqroot(vx*vx + vy*vy + vz*vz);

    if(speed > AC_MAX_FOOT_SPEED) {
        new details[48];
        format(details, sizeof(details), "Speed: %.2f (max: %.1f)", speed, AC_MAX_FOOT_SPEED);
        AC_Violation(playerid, "SPEED_HACK_FOOT", details, 2);
    }
}

// ------------------------------------------------------------
// Verificação completa por jogador
// ------------------------------------------------------------
stock AC_CheckPlayer(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!PlayerData[playerid][pIsLogged]) return;
    if(!g_ACEnabled) return;

    AC_CheckMoney(playerid);
    AC_CheckTeleport(playerid);
    AC_CheckVehicleSpeed(playerid);
    AC_CheckFootSpeed(playerid);
    AC_CheckWeapons(playerid);
}

// Decaimento de score (reset gradual se não violar por tempo)
stock AC_DecayScore(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(g_AC[playerid][ac_SuspicionScore] > 0) {
        g_AC[playerid][ac_SuspicionScore]--;
    }
}

// ------------------------------------------------------------
// Scheduler
// ------------------------------------------------------------
public OnScheduler5Seconds() {
    if(!g_ACEnabled) return;
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {
            AC_CheckPlayer(i);
        }
    }
}

public OnScheduler1Minute() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i)) AC_DecayScore(i);
    }
}

// Integração com eventos
public OnPlayerLoaded(playerid) {
    AC_InitPlayer(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    AC_InitPlayer(playerid);
    return 1;
}

// Anti-flood de comandos (rate limiter)
static g_CmdTimes[MAX_PLAYERS][10];
static g_CmdTimeIdx[MAX_PLAYERS] = {0, ...};

stock AC_CheckCommandFlood(playerid) {
    if(!IsValidPlayer(playerid)) return false;

    new now = GetTickCount();
    new idx = g_CmdTimeIdx[playerid];
    g_CmdTimes[playerid][idx] = now;
    g_CmdTimeIdx[playerid] = (idx + 1) % 10;

    // Contar comandos nos últimos 3 segundos
    new count = 0;
    for(new i = 0; i < 10; i++) {
        if(now - g_CmdTimes[playerid][i] < 3000) count++;
    }

    if(count >= 10) {
        AC_Violation(playerid, "CMD_FLOOD", "10+ comandos em 3s", 2);
        return true; // bloqueado
    }
    return false;
}

// Comandos admin de anti-cheat
CMD:acstatus(playerid, params[]) {
    if(!Player_IsAdmin(playerid, ADMIN_ADMIN)) return 1;
    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Anti-Cheat Status ===\n\n"
        CSTR_WHITE"Status: %s\n\n"
        CSTR_SYSTEM"Jogadores suspeitos:\n",
        g_ACEnabled ? CSTR_SUCCESS"ATIVO" : CSTR_ERROR"DESATIVADO");

    new found = false;
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(g_AC[i][ac_SuspicionScore] > 0) {
            new name[MAX_PLAYER_NAME];
            GetPlayerName(i, name, sizeof(name));
            new line[64];
            format(line, sizeof(line),
                CSTR_WHITE"%s (ID:%d) Score: "CSTR_ERROR"%d\n",
                name, i, g_AC[i][ac_SuspicionScore]);
            strcat(body, line, sizeof(body));
            found = true;
        }
    }
    if(!found) strcat(body, CSTR_SUCCESS"Nenhum jogador suspeito.", sizeof(body));

    UI_ShowDialog(playerid, DIALOG_NONE + 50, DIALOG_STYLE_MSGBOX,
        "{ Anti-Cheat", body, "Fechar", "");
    return 1;
}

CMD:actoggle(playerid, params[]) {
    if(!Player_IsAdmin(playerid, ADMIN_SUPERVISOR)) {
        SendError(playerid, "Sem permissão.");
        return 1;
    }
    g_ACEnabled = !g_ACEnabled;
    new msg[48];
    format(msg, sizeof(msg),
        CSTR_ADMIN"[AC] "CSTR_WHITE"Anti-cheat: %s",
        g_ACEnabled ? CSTR_SUCCESS"ATIVADO" : CSTR_ERROR"DESATIVADO");
    SendClientMessageToAll(COLOR_ADMIN, msg);
    Logger_Admin(playerid, INVALID_PLAYER_ID, "AC_TOGGLE",
        g_ACEnabled ? "Ativado" : "Desativado");
    return 1;
}

CMD:acscore(playerid, params[]) {
    if(!Player_IsAdmin(playerid, ADMIN_MODERATOR)) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /acscore [id]");
        return 1;
    }
    new name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, name, sizeof(name));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[AC] "CSTR_WHITE"%s (ID:%d) | Score: "CSTR_ERROR"%d"
        CSTR_WHITE" | Violações: %d",
        name, targetid,
        g_AC[targetid][ac_SuspicionScore],
        g_AC[targetid][ac_ViolationCount]);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}

CMD:acclean(playerid, params[]) {
    if(!Player_IsAdmin(playerid, ADMIN_ADMIN)) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /acclean [id]");
        return 1;
    }
    g_AC[targetid][ac_SuspicionScore] = 0;
    g_AC[targetid][ac_ViolationCount] = 0;
    new name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, name, sizeof(name));
    new msg[48];
    format(msg, sizeof(msg), CSTR_SUCCESS"Score de %s zerado.", name);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    Logger_Admin(playerid, targetid, "AC_CLEAN", "Score zerado pelo admin");
    return 1;
}
