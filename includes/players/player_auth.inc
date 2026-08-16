// ============================================================
//  Santa Aurora Roleplay — Autenticação
//  Módulo : players/player_auth.inc
//  Objetivo: login, registro, hash de senha, proteção brute force
// ============================================================

#if defined _SA_PLAYER_AUTH_INC
    #endinput
#endif
#define _SA_PLAYER_AUTH_INC

#include "player_core.inc"
#include "../ui/ui_core.inc"

// ------------------------------------------------------------
// Estado de autenticação
// ------------------------------------------------------------
static g_PendingRegistration[MAX_PLAYERS][E_REGISTER_DATA];

enum E_REGISTER_DATA {
    reg_Username[MAX_PLAYER_NAME + 4],
    reg_PasswordHash[65],   // Whirlpool = 64 hex chars
    reg_Step                // 0=username, 1=password, 2=confirm
}

// ------------------------------------------------------------
// Exibir dialog de login
// ------------------------------------------------------------
stock Auth_SendLoginDialog(playerid) {
    if(!IsValidPlayer(playerid)) return;

    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));

    new body[256];
    format(body, sizeof(body),
        "Bem-vindo ao "CSTR_SYSTEM"Santa Aurora RP"CSTR_WHITE"!\n\n"
        "Conta: "CSTR_SYSTEM"%s"CSTR_WHITE"\n\n"
        "Digite sua senha para entrar:",
        name);

    UI_ShowDialog(playerid,
        DIALOG_LOGIN,
        DIALOG_STYLE_PASSWORD,
        "{ Login",
        body,
        "Entrar",
        "Registrar");
}

// ------------------------------------------------------------
// Exibir dialog de registro
// ------------------------------------------------------------
stock Auth_SendRegisterDialog(playerid) {
    if(!IsValidPlayer(playerid)) return;

    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    SafeStrCopy(g_PendingRegistration[playerid][reg_Username], name,
        sizeof(g_PendingRegistration[][reg_Username]));
    g_PendingRegistration[playerid][reg_Step] = 1;

    new body[256];
    format(body, sizeof(body),
        "Criar conta para "CSTR_SYSTEM"%s"CSTR_WHITE"\n\n"
        "Escolha uma senha segura (mínimo 6 caracteres):",
        name);

    UI_ShowDialog(playerid,
        DIALOG_REGISTER,
        DIALOG_STYLE_PASSWORD,
        "{ Criar Conta",
        body,
        "Continuar",
        "Voltar");
}

// Dialog de confirmação de senha
static Auth_SendRegisterConfirmDialog(playerid) {
    UI_ShowDialog(playerid,
        DIALOG_REGISTER_CONFIRM,
        DIALOG_STYLE_PASSWORD,
        "{ Confirmar Senha",
        "Digite a senha novamente para confirmar:",
        "Registrar",
        "Voltar");
}

// ------------------------------------------------------------
// Hash de senha (Whirlpool — nativo SA-MP)
// ------------------------------------------------------------
stock Auth_HashPassword(const password[], output[], size = sizeof(output)) {
    WP_Hash(output, size, password);
}

stock bool:Auth_CheckPassword(const password[], const stored_hash[]) {
    new hash[65];
    WP_Hash(hash, sizeof(hash), password);
    return strequal(hash, stored_hash, true);
}

// ------------------------------------------------------------
// Processar resposta do dialog de login
// ------------------------------------------------------------
static Auth_ProcessLoginAttempt(playerid, const password[]) {
    if(!IsValidPlayer(playerid)) return;

    // Verificar tentativas excessivas
    PlayerData[playerid][pLoginAttempts]++;
    if(PlayerData[playerid][pLoginAttempts] > LOGIN_MAX_ATTEMPTS) {
        new ip[16];
        Player_GetIP(playerid, ip, sizeof(ip));
        Logger_Security(playerid, "BRUTE_FORCE", "Máximo de tentativas de login atingido");
        Logger_Login(playerid, ip, false, "Brute force - kick automático");

        SendError(playerid, "Muitas tentativas incorretas. Você foi desconectado.");
        Kick(playerid);
        return;
    }

    if(strlen(password) < 1) {
        SendError(playerid, "A senha não pode estar vazia.");
        Auth_SendLoginDialog(playerid);
        return;
    }

    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));

    new escaped_name[MAX_PLAYER_NAME * 2];
    DB_Escape(name, escaped_name, sizeof(escaped_name));

    new query[256];
    format(query, sizeof(query),
        "SELECT `id`, `password_hash`, `status`, `admin_level`, `admin_name` "
        "FROM `accounts` WHERE `username` = '%s' LIMIT 1",
        escaped_name);

    // Salvar senha para verificar no callback
    SetPVarString(playerid, "pending_password", password);
    DB_Query(query, "OnPlayerLoginQuery", "i", playerid);
}

// ------------------------------------------------------------
// Callback da query de login
// ------------------------------------------------------------
forward OnPlayerLoginQuery(playerid);
public OnPlayerLoginQuery(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    if(!DB_HasResult()) {
        // Conta não existe — oferecer registro
        DeletePVar(playerid, "pending_password");
        SendWarning(playerid, "Conta não encontrada. Registre-se para continuar.");
        Auth_SendRegisterDialog(playerid);
        return 1;
    }

    new accountID = DB_GetInt("id");
    new stored_hash[65];
    DB_GetString("password_hash", stored_hash, sizeof(stored_hash));
    new status = DB_GetInt("status");
    new adminLevel = DB_GetInt("admin_level");
    new adminName[MAX_PLAYER_NAME];
    DB_GetString("admin_name", adminName, sizeof(adminName));

    // Verificar ban
    if(status == 2) {
        DeletePVar(playerid, "pending_password");
        Logger_Login(playerid, g_PlayerIP[playerid], false, "Conta banida");
        SendError(playerid, "Sua conta está banida. Contate a administração.");
        Kick(playerid);
        return 1;
    }

    // Verificar senha
    new pending_pw[MAX_PASSWORD_LENGTH];
    GetPVarString(playerid, "pending_password", pending_pw, sizeof(pending_pw));
    DeletePVar(playerid, "pending_password");

    if(!Auth_CheckPassword(pending_pw, stored_hash)) {
        Logger_Login(playerid, g_PlayerIP[playerid], false, "Senha incorreta");

        new remaining = LOGIN_MAX_ATTEMPTS - PlayerData[playerid][pLoginAttempts];
        new msg[64];
        format(msg, sizeof(msg), "Senha incorreta. Tentativas restantes: %d", remaining);
        SendError(playerid, msg);
        Auth_SendLoginDialog(playerid);
        return 1;
    }

    // Login OK
    PlayerData[playerid][pAccountID]    = accountID;
    PlayerData[playerid][pAdminLevel]   = adminLevel;
    SafeStrCopy(PlayerData[playerid][pAdminName], adminName,
        sizeof(PlayerData[][pAdminName]));
    PlayerData[playerid][pLoginAttempts] = 0;

    Logger_Login(playerid, g_PlayerIP[playerid], true, "Login bem-sucedido");

    // Atualizar last_login e ip na conta
    new ip[16];
    Player_GetIP(playerid, ip, sizeof(ip));
    new escaped_ip[32];
    DB_Escape(ip, escaped_ip, sizeof(escaped_ip));
    new update_query[256];
    format(update_query, sizeof(update_query),
        "UPDATE `accounts` SET `last_login`=NOW(), `last_ip`='%s' WHERE `id`=%d",
        escaped_ip, accountID);
    DB_Execute(update_query);

    // Carregar personagem
    Player_SetState(playerid, STATE_CHARACTER_SELECTION);
    CallLocalFunction("Player_LoadCharacter", "i", playerid);
    return 1;
}

// ------------------------------------------------------------
// Processar registro — step 1: senha
// ------------------------------------------------------------
static Auth_ProcessRegisterPassword(playerid, const password[]) {
    if(strlen(password) < 6) {
        SendError(playerid, "A senha deve ter pelo menos 6 caracteres.");
        Auth_SendRegisterDialog(playerid);
        return;
    }
    // Hash e guardar temporariamente
    Auth_HashPassword(password, g_PendingRegistration[playerid][reg_PasswordHash],
        sizeof(g_PendingRegistration[][reg_PasswordHash]));
    g_PendingRegistration[playerid][reg_Step] = 2;
    Auth_SendRegisterConfirmDialog(playerid);
}

// ------------------------------------------------------------
// Processar registro — step 2: confirmação
// ------------------------------------------------------------
static Auth_ProcessRegisterConfirm(playerid, const password_confirm[]) {
    new hash_confirm[65];
    Auth_HashPassword(password_confirm, hash_confirm, sizeof(hash_confirm));

    if(!strequal(hash_confirm, g_PendingRegistration[playerid][reg_PasswordHash], true)) {
        SendError(playerid, "As senhas não coincidem. Tente novamente.");
        g_PendingRegistration[playerid][reg_Step] = 1;
        Auth_SendRegisterDialog(playerid);
        return;
    }

    // Criar conta no banco
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    new escaped_name[MAX_PLAYER_NAME * 2];
    DB_Escape(name, escaped_name, sizeof(escaped_name));

    new ip[16];
    Player_GetIP(playerid, ip, sizeof(ip));
    new escaped_ip[32];
    DB_Escape(ip, escaped_ip, sizeof(escaped_ip));

    new serial[41];
    Player_GetSerial(playerid, serial, sizeof(serial));
    new escaped_serial[82];
    DB_Escape(serial, escaped_serial, sizeof(escaped_serial));

    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `accounts` (`username`, `password_hash`, `created_at`, `last_login`, `last_ip`, `serial`, `status`, `admin_level`) "
        "VALUES ('%s', '%s', NOW(), NOW(), '%s', '%s', 1, 0)",
        escaped_name,
        g_PendingRegistration[playerid][reg_PasswordHash],
        escaped_ip,
        escaped_serial);

    DB_Query(query, "OnPlayerRegisterQuery", "i", playerid);
}

// ------------------------------------------------------------
// Callback da query de registro
// ------------------------------------------------------------
forward OnPlayerRegisterQuery(playerid);
public OnPlayerRegisterQuery(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    new newID = DB_InsertID();
    if(newID <= 0) {
        Logger_Error("AUTH", "Falha ao criar conta no banco.");
        SendError(playerid, "Erro ao criar conta. Tente novamente.");
        Auth_SendRegisterDialog(playerid);
        return 1;
    }

    PlayerData[playerid][pAccountID] = newID;
    Logger_Login(playerid, g_PlayerIP[playerid], true, "Conta criada");
    SendSuccess(playerid, "Conta criada com sucesso! Bem-vindo ao Santa Aurora RP!");

    // Criar personagem inicial
    Player_SetState(playerid, STATE_CHARACTER_SELECTION);
    CallLocalFunction("Player_CreateCharacter", "i", playerid);
    return 1;
}

// ------------------------------------------------------------
// Handler central de dialogs de auth
// Retorna 1 se tratou, 0 se não é dialog de auth
// ------------------------------------------------------------
stock bool:Auth_HandleDialog(playerid, dialogid, response, const inputtext[]) {
    switch(dialogid) {
        case DIALOG_LOGIN: {
            if(!response) {
                // Botão "Registrar"
                Auth_SendRegisterDialog(playerid);
                return true;
            }
            Auth_ProcessLoginAttempt(playerid, inputtext);
            return true;
        }
        case DIALOG_REGISTER: {
            if(!response) {
                Auth_SendLoginDialog(playerid);
                return true;
            }
            Auth_ProcessRegisterPassword(playerid, inputtext);
            return true;
        }
        case DIALOG_REGISTER_CONFIRM: {
            if(!response) {
                g_PendingRegistration[playerid][reg_Step] = 1;
                Auth_SendRegisterDialog(playerid);
                return true;
            }
            Auth_ProcessRegisterConfirm(playerid, inputtext);
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------
// Verificar se jogador está no processo de autenticação
// ------------------------------------------------------------
stock bool:Auth_IsAuthenticating(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    new E_PLAYER_STATE:st = Player_GetState(playerid);
    return (st == STATE_CONNECTING || st == STATE_AUTHENTICATING);
}

