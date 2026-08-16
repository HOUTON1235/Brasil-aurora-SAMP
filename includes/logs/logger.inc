// ============================================================
//  Santa Aurora Roleplay — Logger
//  Módulo : logs/logger.inc
//  Objetivo: sistema de log centralizado com arquivos separados
//            por módulo e data
// ============================================================

#if defined _SA_LOGGER_INC
    #endinput
#endif
#define _SA_LOGGER_INC

#include "../core/defines.inc"
#include "../core/macros.inc"

// ------------------------------------------------------------
// Níveis de log
// ------------------------------------------------------------
#define LOG_INFO        "INFO"
#define LOG_WARNING     "WARN"
#define LOG_ERROR       "ERROR"
#define LOG_DEBUG       "DEBUG"
#define LOG_SECURITY    "SECURITY"
#define LOG_ECONOMY     "ECONOMY"
#define LOG_ADMIN       "ADMIN"

// ------------------------------------------------------------
// Módulos de log → subdiretórios
// ------------------------------------------------------------
#define LOG_MODULE_SYSTEM       "system"
#define LOG_MODULE_LOGIN        "login"
#define LOG_MODULE_PLAYER       "system"
#define LOG_MODULE_ECONOMY      "economy"
#define LOG_MODULE_INVENTORY    "inventory"
#define LOG_MODULE_VEHICLES     "vehicles"
#define LOG_MODULE_PROPERTIES   "properties"
#define LOG_MODULE_FACTIONS     "factions"
#define LOG_MODULE_POLICE       "police"
#define LOG_MODULE_CHAT         "chat"
#define LOG_MODULE_PUNISHMENTS  "punishments"
#define LOG_MODULE_SECURITY     "security"
#define LOG_MODULE_ERRORS       "errors"
#define LOG_MODULE_ADMIN        "admin"

// ------------------------------------------------------------
// Estado interno
// ------------------------------------------------------------
static bool:g_LoggerInitialized = false;

// ------------------------------------------------------------
// Inicialização
// ------------------------------------------------------------
stock Logger_Init() {
    g_LoggerInitialized = true;
    Logger_Write(LOG_MODULE_SYSTEM, LOG_INFO, "Logger inicializado.");
}

// ------------------------------------------------------------
// Função principal de escrita
// ------------------------------------------------------------
stock Logger_Write(const module[], const level[], const message[]) {
    if(!g_LoggerInitialized && !strequal(level, LOG_INFO)) return;

    // Obter timestamp
    new year, month, day, hour, minute, second;
    getdate(year, month, day);
    gettime(hour, minute, second);

    // Montar caminho do arquivo: scriptfiles/logs/<module>/YYYY-MM-DD.log
    new filepath[128];
    format(filepath, sizeof(filepath),
        "scriptfiles/logs/%s/%04d-%02d-%02d.log",
        module, year, month, day);

    // Montar linha de log
    new logline[512];
    format(logline, sizeof(logline),
        "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s\r\n",
        year, month, day, hour, minute, second,
        level, message);

    // Escrever no arquivo
    new File:f = fopen(filepath, io_append);
    if(f) {
        fwrite(f, logline);
        fclose(f);
    } else {
        // Fallback para console se não conseguir abrir arquivo
        printf("[LOG FALLBACK][%s][%s] %s", module, level, message);
    }

    // Sempre espelhar no console se for ERROR
    if(strequal(level, LOG_ERROR) || strequal(level, LOG_SECURITY)) {
        printf("[%s][%s] %s", module, level, message);
    }
}

// ------------------------------------------------------------
// Helpers de nível
// ------------------------------------------------------------
stock Logger_Info(const module[], const message[]) {
    Logger_Write(module, LOG_INFO, message);
}

stock Logger_Warning(const module[], const message[]) {
    Logger_Write(module, LOG_WARNING, message);
}

stock Logger_Error(const module[], const message[]) {
    Logger_Write(module, LOG_ERROR, message);
}

stock Logger_Debug(const module[], const message[]) {
    #if defined DEBUG_MODE
        Logger_Write(module, LOG_DEBUG, message);
    #endif
    #pragma unused module
    #pragma unused message
}

// ------------------------------------------------------------
// Logs especializados
// ------------------------------------------------------------

// Log de segurança (detecção de cheats, tentativas suspeitas)
stock Logger_Security(playerid, const event_type[], const details[]) {
    new name[MAX_PLAYER_NAME];
    if(IsValidPlayer(playerid)) GetPlayerName(playerid, name, sizeof(name));
    else format(name, sizeof(name), "ID_%d", playerid);

    new ip[16];
    if(IsValidPlayer(playerid)) GetPlayerIp(playerid, ip, sizeof(ip));

    new msg[256];
    format(msg, sizeof(msg), "Player=%s(%d) IP=%s | Evento=%s | %s",
        name, playerid, ip, event_type, details);
    Logger_Write(LOG_MODULE_SECURITY, LOG_SECURITY, msg);
}

// Log econômico
stock Logger_Economy(playerid, const action[], value, const origin[], const destination[]) {
    new name[MAX_PLAYER_NAME];
    if(IsValidPlayer(playerid)) GetPlayerName(playerid, name, sizeof(name));
    else format(name, sizeof(name), "ID_%d", playerid);

    new msg[256];
    format(msg, sizeof(msg), "Player=%s(%d) | Ação=%s | Valor=%d | Origem=%s | Destino=%s",
        name, playerid, action, value, origin, destination);
    Logger_Write(LOG_MODULE_ECONOMY, LOG_ECONOMY, msg);
}

// Log administrativo
stock Logger_Admin(adminid, targetid, const action[], const details[]) {
    new aname[MAX_PLAYER_NAME], tname[MAX_PLAYER_NAME];
    if(IsValidPlayer(adminid)) GetPlayerName(adminid, aname, sizeof(aname));
    else format(aname, sizeof(aname), "Console");

    if(IsValidPlayer(targetid)) GetPlayerName(targetid, tname, sizeof(tname));
    else format(tname, sizeof(tname), "ID_%d", targetid);

    new msg[256];
    format(msg, sizeof(msg), "Admin=%s(%d) -> Target=%s(%d) | Ação=%s | %s",
        aname, adminid, tname, targetid, action, details);
    Logger_Write(LOG_MODULE_ADMIN, LOG_ADMIN, msg);
}

// Log de chat
stock Logger_Chat(playerid, const type[], const message[]) {
    new name[MAX_PLAYER_NAME];
    if(IsValidPlayer(playerid)) GetPlayerName(playerid, name, sizeof(name));
    else format(name, sizeof(name), "ID_%d", playerid);

    new msg[256];
    format(msg, sizeof(msg), "[%s] %s(%d): %s", type, name, playerid, message);
    Logger_Write(LOG_MODULE_CHAT, LOG_INFO, msg);
}

// Log de punição
stock Logger_Punishment(adminid, targetid, const type[], const reason[], duration) {
    new aname[MAX_PLAYER_NAME], tname[MAX_PLAYER_NAME];
    if(IsValidPlayer(adminid)) GetPlayerName(adminid, aname, sizeof(aname));
    else format(aname, sizeof(aname), "Console");
    if(IsValidPlayer(targetid)) GetPlayerName(targetid, tname, sizeof(tname));
    else format(tname, sizeof(tname), "ID_%d", targetid);

    new msg[256];
    format(msg, sizeof(msg), "Admin=%s(%d) | Tipo=%s | Alvo=%s(%d) | Motivo=%s | Duração=%d",
        aname, adminid, type, tname, targetid, reason, duration);
    Logger_Write(LOG_MODULE_PUNISHMENTS, LOG_INFO, msg);
}

// Log de login
stock Logger_Login(playerid, const ip[], bool:success, const reason[]) {
    new name[MAX_PLAYER_NAME];
    if(IsValidPlayer(playerid)) GetPlayerName(playerid, name, sizeof(name));
    else format(name, sizeof(name), "ID_%d", playerid);

    new msg[256];
    format(msg, sizeof(msg), "Player=%s(%d) IP=%s | %s | %s",
        name, playerid, ip,
        success ? "LOGIN_OK" : "LOGIN_FAIL",
        reason);
    Logger_Write(LOG_MODULE_LOGIN, LOG_INFO, msg);
}

