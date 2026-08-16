// ============================================================
//  Santa Aurora Roleplay ? Admin Core
//  Modulo : admin/admin_core.inc
//  Objetivo: comandos e funcoes administrativas
// ============================================================

#if defined _SA_ADMIN_CORE_INC
    #endinput
#endif
#define _SA_ADMIN_CORE_INC

// include removido
// include removido
// include removido

// Spectate state per admin
static g_SpectateTarget[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};
static g_SpectateMode[MAX_PLAYERS]   = {0, ...};
static g_AdminFrozen[MAX_PLAYERS]    = {false, ...};

// ------------------------------------------------------------
// Inicializacao
// ------------------------------------------------------------
stock Admin_Init() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        g_SpectateTarget[i] = INVALID_PLAYER_ID;
        g_SpectateMode[i]   = 0;
        g_AdminFrozen[i]    = false;
    }
    Logger_Info("ADMIN", "Modulo Admin inicializado.");
}

// ------------------------------------------------------------
// Helpers de verificacao
// ------------------------------------------------------------
stock Admin_IsAdmin(playerid, minLevel = ADMIN_HELPER) {
    return Player_IsAdmin(playerid, minLevel);
}

stock Admin_GetLevel(playerid) {
    return Player_GetAdminLevel(playerid);
}

// Enviar mensagem de erro de permissao
static Admin_NoPermission(playerid) {
    UI_NotifyError(playerid, "Voce nao tem permissao para usar este comando.");
}

// Encontrar jogador por nome parcial
stock Admin_FindPlayer(partial_name[], exclude = INVALID_PLAYER_ID) {
    new name[MAX_PLAYER_NAME];
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(i == exclude) continue;
        GetPlayerName(i, name, sizeof(name));
        if(strfind(name, partial_name, true) != -1) return i;
    }
    return INVALID_PLAYER_ID;
}

// ------------------------------------------------------------
// Teleporte administrativo
// ------------------------------------------------------------
stock Admin_TeleportToPlayer(adminid, targetid) {
    if(!IsValidPlayer(adminid) || !IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_TELEPORT)) {
        Admin_NoPermission(adminid);
        return;
    }

    new Float:tx, Float:ty, Float:tz;
    GetPlayerPos(targetid, tx, ty, tz);
    SetPlayerPos(adminid, tx + 1.0, ty + 1.0, tz);
    SetPlayerInterior(adminid, GetPlayerInterior(targetid));
    SetPlayerVirtualWorld(adminid, GetPlayerVirtualWorld(targetid));

    new admin_name[MAX_PLAYER_NAME], target_name[MAX_PLAYER_NAME];
    GetPlayerName(adminid, admin_name, sizeof(admin_name));
    GetPlayerName(targetid, target_name, sizeof(target_name));

    new msg[64];
    format(msg, sizeof(msg), "Teleportado para %s.", target_name);
    UI_NotifySuccess(adminid, msg);

    Logger_Admin(adminid, targetid, "TELEPORT", "Admin teleportado para o jogador");
}

// Teleportar jogador ao admin
stock Admin_BringPlayer(adminid, targetid) {
    if(!IsValidPlayer(adminid) || !IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_TELEPORT)) {
        Admin_NoPermission(adminid);
        return;
    }

    new Float:ax, Float:ay, Float:az;
    GetPlayerPos(adminid, ax, ay, az);
    SetPlayerPos(targetid, ax + 1.0, ay + 1.0, az);
    SetPlayerInterior(targetid, GetPlayerInterior(adminid));
    SetPlayerVirtualWorld(targetid, GetPlayerVirtualWorld(adminid));

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));

    new msg[64];
    format(msg, sizeof(msg), "Jogador %s trazido ate voce.", target_name);
    UI_NotifySuccess(adminid, msg);

    Logger_Admin(adminid, targetid, "BRING", "Jogador trazido ao admin");
}

// ------------------------------------------------------------
// Kick
// ------------------------------------------------------------
stock Admin_KickPlayer(adminid, targetid, reason[]) {
    if(!IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_KICK)) {
        Admin_NoPermission(adminid);
        return;
    }
    if(Admin_GetLevel(targetid) >= Admin_GetLevel(adminid) && adminid != INVALID_PLAYER_ID) {
        UI_NotifyError(adminid, "Voce nao pode kickar um admin de nivel igual ou superior.");
        return;
    }

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));

    // Notificar todos os admins
    new broadcast[128];
    new admin_name[MAX_PLAYER_NAME];
    if(IsValidPlayer(adminid)) GetPlayerName(adminid, admin_name, sizeof(admin_name));
    else format(admin_name, sizeof(admin_name), "Console");
    format(broadcast, sizeof(broadcast),
        CSTR_ADMIN"[ADM] "CSTR_WHITE"%s foi kickado por %s. Motivo: %s",
        target_name, admin_name, reason);

    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && Player_IsAdmin(i, ADMIN_HELPER)) {
            SendClientMessage(i, COLOR_ADMIN, broadcast);
        }
    }

    new target_msg[128];
    format(target_msg, sizeof(target_msg),
        CSTR_ERROR"Voce foi kickado.\nMotivo: "CSTR_WHITE"%s", reason);
    SendClientMessage(targetid, COLOR_ERROR, target_msg);

    Logger_Punishment(adminid, targetid, "KICK", reason, 0);
    Logger_Admin(adminid, targetid, "KICK", reason);

    // Salvar antes de kickar
    Player_Save(targetid);
    SetTimerEx("Admin_DelayedKick", 500, false, "i", targetid);
}

forward Admin_DelayedKick(playerid);
public Admin_DelayedKick(playerid) {
    if(IsValidPlayer(playerid)) Kick(playerid);
}

// ------------------------------------------------------------
// Ban
// ------------------------------------------------------------
stock Admin_BanPlayer(adminid, targetid, reason[], duration = 0) {
    // duration em minutos, 0 = permanente
    if(!IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_BAN)) {
        Admin_NoPermission(adminid);
        return;
    }
    if(Admin_GetLevel(targetid) >= Admin_GetLevel(adminid) && IsValidPlayer(adminid)) {
        UI_NotifyError(adminid, "Voce nao pode banir um admin de nivel igual ou superior.");
        return;
    }

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));

    // Registrar no banco
    new escaped_reason[MAX_REASON_LENGTH * 2];
    DB_Escape(reason, escaped_reason, sizeof(escaped_reason));

    new expires_sql[32] = "NULL";
    if(duration > 0) {
        format(expires_sql, sizeof(expires_sql), "DATE_ADD(NOW(), INTERVAL %d MINUTE)", duration);
    }

    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `punishments` (`player_id`, `admin_id`, `type`, `reason`, `duration`, `expires_at`, `active`, `created_at`) "
        "VALUES (%d, %d, 'ban', '%s', %d, %s, 1, NOW())",
        PlayerData[targetid][pAccountID],
        IsValidPlayer(adminid) ? PlayerData[adminid][pAccountID] : 0,
        escaped_reason,
        duration,
        expires_sql);
    DB_Execute(query);

    // Atualizar status da conta
    new upd_query[128];
    format(upd_query, sizeof(upd_query),
        "UPDATE `accounts` SET `status`=2 WHERE `id`=%d",
        PlayerData[targetid][pAccountID]);
    DB_Execute(upd_query);

    // Notificar admins
    new admin_name[MAX_PLAYER_NAME];
    if(IsValidPlayer(adminid)) GetPlayerName(adminid, admin_name, sizeof(admin_name));
    else format(admin_name, sizeof(admin_name), "Console");

    new broadcast[128];
    format(broadcast, sizeof(broadcast),
        CSTR_ADMIN"[ADM] "CSTR_WHITE"%s foi banido por %s. Motivo: %s",
        target_name, admin_name, reason);
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && Player_IsAdmin(i, ADMIN_HELPER)) {
            SendClientMessage(i, COLOR_ADMIN, broadcast);
        }
    }

    new target_msg[128];
    format(target_msg, sizeof(target_msg),
        CSTR_ERROR"Voce foi banido.\nMotivo: "CSTR_WHITE"%s", reason);
    SendClientMessage(targetid, COLOR_ERROR, target_msg);

    Logger_Punishment(adminid, targetid, "BAN", reason, duration);
    Logger_Admin(adminid, targetid, "BAN", reason);

    Player_Save(targetid);
    SetTimerEx("Admin_DelayedKick", 500, false, "i", targetid);
}

// ------------------------------------------------------------
// Dar/Remover dinheiro
// ------------------------------------------------------------
stock Admin_GiveMoney(adminid, targetid, amount, reason[]) {
    if(!IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_ECONOMY_GIVE)) {
        Admin_NoPermission(adminid);
        return;
    }
    if(amount <= 0 || amount > MAX_MONEY) {
        UI_NotifyError(adminid, "Valor invalido.");
        return;
    }

    Player_AddMoney(targetid, amount);
    Player_MarkDirty(targetid);

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new admin_name[MAX_PLAYER_NAME];
    GetPlayerName(adminid, admin_name, sizeof(admin_name));

    new formatted[32];
    FormatMoneyFull(amount, formatted, sizeof(formatted));

    new msg_target[64];
    format(msg_target, sizeof(msg_target), "Voce recebeu %s do administrador.", formatted);
    UI_NotifyMoney(targetid, amount, true);
    SendClientMessage(targetid, COLOR_INFO, msg_target);

    new msg_admin[64];
    format(msg_admin, sizeof(msg_admin), "Voce deu %s para %s.", formatted, target_name);
    UI_NotifySuccess(adminid, msg_admin);

    new log_detail[128];
    format(log_detail, sizeof(log_detail), "Valor=%d Motivo=%s", amount, reason);
    Logger_Admin(adminid, targetid, "GIVE_MONEY", log_detail);
    Logger_Economy(targetid, "ADMIN_GIVE", amount, admin_name, target_name);
}

stock Admin_RemoveMoney(adminid, targetid, amount, reason[]) {
    if(!IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_ECONOMY_REMOVE)) {
        Admin_NoPermission(adminid);
        return;
    }
    if(amount <= 0) {
        UI_NotifyError(adminid, "Valor invalido.");
        return;
    }

    Player_RemoveMoney(targetid, amount);
    Player_MarkDirty(targetid);

    new log_detail[128];
    format(log_detail, sizeof(log_detail), "Valor=%d Motivo=%s", amount, reason);
    Logger_Admin(adminid, targetid, "REMOVE_MONEY", log_detail);
}

// ------------------------------------------------------------
// Freeze / Unfreeze
// ------------------------------------------------------------
stock Admin_FreezePlayer(adminid, targetid) {
    if(!IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_FREEZE)) {
        Admin_NoPermission(adminid);
        return;
    }
    TogglePlayerControllable(targetid, false);
    g_AdminFrozen[targetid] = true;

    UI_NotifyWarning(targetid, "Voce foi congelado por um administrador.");
    Logger_Admin(adminid, targetid, "FREEZE", "Jogador congelado");
}

stock Admin_UnfreezePlayer(adminid, targetid) {
    if(!IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_FREEZE)) {
        Admin_NoPermission(adminid);
        return;
    }
    TogglePlayerControllable(targetid, true);
    g_AdminFrozen[targetid] = false;

    UI_NotifySuccess(targetid, "Voce foi descongelado.");
    Logger_Admin(adminid, targetid, "UNFREEZE", "Jogador descongelado");
}

stock Admin_IsPlayerFrozen(playerid) {
    return g_AdminFrozen[playerid];
}

// ------------------------------------------------------------
// Spectate
// ------------------------------------------------------------
stock Admin_SpectatePlayer(adminid, targetid) {
    if(!IsValidPlayer(adminid) || !IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_SPECTATE)) {
        Admin_NoPermission(adminid);
        return;
    }

    // Garantir mesmo virtual world
    SetPlayerVirtualWorld(adminid, GetPlayerVirtualWorld(targetid));
    SetPlayerInterior(adminid, GetPlayerInterior(targetid));

    TogglePlayerSpectating(adminid, true);
    PlayerSpectatePlayer(adminid, targetid);

    g_SpectateTarget[adminid] = targetid;
    g_SpectateMode[adminid]   = 1;
    Player_SetState(adminid, STATE_ADMIN_SPECTATING);

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new msg[64];
    format(msg, sizeof(msg), "Observando: %s. Use /asp para parar.", target_name);
    UI_NotifyInfo(adminid, msg);

    Logger_Admin(adminid, targetid, "SPECTATE_START", "Admin iniciou spectate");
}

stock Admin_StopSpectate(adminid) {
    if(!IsValidPlayer(adminid)) return;
    if(g_SpectateMode[adminid] == 0) return;

    TogglePlayerSpectating(adminid, false);
    g_SpectateTarget[adminid] = INVALID_PLAYER_ID;
    g_SpectateMode[adminid]   = 0;

    Player_SetState(adminid, STATE_SPAWNED);
    SpawnPlayer(adminid);
    UI_NotifySuccess(adminid, "Modo spectate encerrado.");
}

// ------------------------------------------------------------
// Warn
// ------------------------------------------------------------
stock Admin_WarnPlayer(adminid, targetid, reason[]) {
    if(!IsValidPlayer(targetid)) return;
    if(!Player_HasPermission(adminid, PERM_ADMIN_PLAYER_WARN)) {
        Admin_NoPermission(adminid);
        return;
    }

    new escaped[MAX_REASON_LENGTH * 2];
    DB_Escape(reason, escaped, sizeof(escaped));

    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `punishments` (`player_id`, `admin_id`, `type`, `reason`, `duration`, `active`, `created_at`) "
        "VALUES (%d, %d, 'warn', '%s', 0, 1, NOW())",
        PlayerData[targetid][pAccountID],
        IsValidPlayer(adminid) ? PlayerData[adminid][pAccountID] : 0,
        escaped);
    DB_Execute(query);

    new msg[128];
    format(msg, sizeof(msg), CSTR_WARNING"[AVISO] "CSTR_WHITE"Motivo: %s", reason);
    SendClientMessage(targetid, COLOR_WARNING, msg);

    new admin_name[MAX_PLAYER_NAME];
    GetPlayerName(adminid, admin_name, sizeof(admin_name));
    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new admmsg[128];
    format(admmsg, sizeof(admmsg), "Aviso enviado para %s.", target_name);
    UI_NotifySuccess(adminid, admmsg);

    Logger_Punishment(adminid, targetid, "WARN", reason, 0);
}

// ------------------------------------------------------------
// Comandos administrativos
// Usar prefixo /a para evitar conflito com comandos RP
// ------------------------------------------------------------

// /atp [id/nome] ? teleportar ao jogador
CMD:atp(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_HELPER)) return Admin_NoPermission(playerid), 1;
    new target_str[MAX_PLAYER_NAME];
    if(sscanf(params, "s[24]", target_str)) {
        UI_NotifyError(playerid, "Uso: /atp [id ou nome]");
        return 1;
    }
    new targetid = strval(target_str);
    if(!IsValidPlayer(targetid)) {
        targetid = Admin_FindPlayer(target_str);
        if(targetid == INVALID_PLAYER_ID) {
            UI_NotifyError(playerid, "Jogador nao encontrado.");
            return 1;
        }
    }
    Admin_TeleportToPlayer(playerid, targetid);
    return 1;
}

// /abring [id] ? trazer jogador
CMD:abring(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_HELPER)) return Admin_NoPermission(playerid), 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Uso: /abring [id]");
        return 1;
    }
    Admin_BringPlayer(playerid, targetid);
    return 1;
}

// /akick [id] [motivo] ? kickar jogador
CMD:akick(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_HELPER)) return Admin_NoPermission(playerid), 1;
    new targetid;
    new reason[MAX_REASON_LENGTH];
    if(sscanf(params, "is[128]", targetid, reason) || !IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Uso: /akick [id] [motivo]");
        return 1;
    }
    Admin_KickPlayer(playerid, targetid, reason);
    return 1;
}

// /aban [id] [duracao_min] [motivo] ? banir (0 = permanente)
CMD:aban(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_MODERATOR)) return Admin_NoPermission(playerid), 1;
    new targetid, duration;
    new reason[MAX_REASON_LENGTH];
    if(sscanf(params, "iis[128]", targetid, duration, reason) || !IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Uso: /aban [id] [minutos, 0=perm] [motivo]");
        return 1;
    }
    Admin_BanPlayer(playerid, targetid, reason, duration);
    return 1;
}

// /agm [id] [valor] [motivo] ? dar dinheiro
CMD:agm(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_ADMIN)) return Admin_NoPermission(playerid), 1;
    new targetid, amount;
    new reason[MAX_REASON_LENGTH];
    if(sscanf(params, "iis[128]", targetid, amount, reason)) {
        UI_NotifyError(playerid, "Uso: /agm [id] [valor] [motivo]");
        return 1;
    }
    Admin_GiveMoney(playerid, targetid, amount, reason);
    return 1;
}

// /afreeze [id] ? congelar
CMD:afreeze(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_MODERATOR)) return Admin_NoPermission(playerid), 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Uso: /afreeze [id]");
        return 1;
    }
    if(Admin_IsPlayerFrozen(targetid)) {
        Admin_UnfreezePlayer(playerid, targetid);
    } else {
        Admin_FreezePlayer(playerid, targetid);
    }
    return 1;
}

// /asp [id] ? spectate
CMD:asp(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_HELPER)) return Admin_NoPermission(playerid), 1;
    if(g_SpectateMode[playerid] == 1) {
        Admin_StopSpectate(playerid);
        return 1;
    }
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Uso: /asp [id] (ou /asp para parar)");
        return 1;
    }
    Admin_SpectatePlayer(playerid, targetid);
    return 1;
}

// /awarn [id] [motivo] ? advertir
CMD:awarn(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_HELPER)) return Admin_NoPermission(playerid), 1;
    new targetid;
    new reason[MAX_REASON_LENGTH];
    if(sscanf(params, "is[128]", targetid, reason) || !IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Uso: /awarn [id] [motivo]");
        return 1;
    }
    Admin_WarnPlayer(playerid, targetid, reason);
    return 1;
}

// /ainfo [id] ? informacoes sobre jogador
CMD:ainfo(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_HELPER)) return Admin_NoPermission(playerid), 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Uso: /ainfo [id]");
        return 1;
    }

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new ip[16];
    Player_GetIP(targetid, ip, sizeof(ip));
    new serial[41];
    Player_GetSerial(targetid, serial, sizeof(serial));
    new state_name[32];
    State_GetName(Player_GetState(targetid), state_name, sizeof(state_name));
    new admin_level_name[32];
    GetAdminLevelName(Player_GetAdminLevel(targetid), admin_level_name, sizeof(admin_level_name));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Informacoes do Jogador ===\n\n"
        CSTR_WHITE"ID: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Nome: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Conta ID: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Personagem ID: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"IP: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Serial: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Nivel Admin: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Estado: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Dinheiro: "CSTR_MONEY"%d\n"
        CSTR_WHITE"Banco: "CSTR_MONEY"%d\n"
        CSTR_WHITE"Nivel: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Tempo online: "CSTR_SYSTEM"%d min",
        targetid, target_name,
        PlayerData[targetid][pAccountID],
        PlayerData[targetid][pCharacterID],
        ip, serial,
        admin_level_name, state_name,
        PlayerData[targetid][pMoney],
        PlayerData[targetid][pBank],
        PlayerData[targetid][pLevel],
        Player_GetSessionTime(targetid) / 60
    );

    UI_ShowDialog(playerid, DIALOG_ADMIN_PLAYER, DIALOG_STYLE_MSGBOX,
        "Informacoes do Jogador", body, "Fechar", "");
    return 1;
}

// /serverstatus ? status do servidor
CMD:serverstatus(playerid, params[]) {
    if(!Admin_IsAdmin(playerid, ADMIN_HELPER)) return Admin_NoPermission(playerid), 1;

    new online = 0;
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i)) online++;
    }

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Status do Servidor ===\n\n"
        CSTR_WHITE"Nome: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Versao: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Jogadores online: "CSTR_SYSTEM"%d/%d\n"
        CSTR_WHITE"Banco de dados: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Scheduler: "CSTR_SYSTEM"Ativo (tick: %d)",
        SERVER_NAME,
        SERVER_VERSION,
        online, MAX_PLAYERS,
        DB_IsConnected() ? "Conectado" : "DESCONECTADO",
        Scheduler_GetTick()
    );

    UI_ShowDialog(playerid, DIALOG_NONE + 2, DIALOG_STYLE_MSGBOX,
        "Status do Servidor", body, "Fechar", "");
    return 1;
}

