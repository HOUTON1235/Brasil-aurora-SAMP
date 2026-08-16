// ============================================================
//  Santa Aurora Roleplay ? Monitoramento e Performance
//  Modulo : utilities/performance_core.inc
//  Objetivo: metricas de servidor, health check, profiling
//            basico de queries, memoria, entidades
// ============================================================

#if defined _SA_PERFORMANCE_CORE_INC
    #endinput
#endif
#define _SA_PERFORMANCE_CORE_INC

// include removido
// include removido
// include removido
// include removido

// ------------------------------------------------------------
// Metricas globais
// ------------------------------------------------------------
static g_TotalQueries     = 0;
static g_TotalErrors      = 0;
static g_ServerStartTime  = 0;
static g_PeakPlayers      = 0;
static g_TotalConnections = 0;

// ------------------------------------------------------------
// Inicializar
// ------------------------------------------------------------
stock Perf_Init() {
    g_ServerStartTime = gettime();
    Logger_Info("PERF", "Monitor de performance inicializado.");
}

// Registrar query executada
stock Perf_OnQuery() { g_TotalQueries++; }
stock Perf_OnError()  { g_TotalErrors++;  }

// Registrar conexao
stock Perf_OnConnect(playerid) {
    g_TotalConnections++;
    new online = 0;
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i)) online++;
    }
    if(online > g_PeakPlayers) g_PeakPlayers = online;
    #pragma unused playerid
}

// ------------------------------------------------------------
// Coletar metricas
// ------------------------------------------------------------
stock Perf_GetUptime() {
    return gettime() - g_ServerStartTime;
}

stock Perf_GetOnlineCount() {
    new count = 0;
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i)) count++;
    }
    return count;
}

// ------------------------------------------------------------
// Relatorio de saude completo
// ------------------------------------------------------------
stock Perf_ShowHealthCheck(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!Player_IsAdmin(playerid, ADMIN_HELPER)) {
        UI_NotifyError(playerid, "Sem permissao.");
        return;
    }

    new uptime_str[32];
    FormatTime(Perf_GetUptime(), uptime_str, sizeof(uptime_str));

    new online  = Perf_GetOnlineCount();
    new db_ok   = DB_IsConnected();

    new body[1024];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Status do Servidor ===\n\n"
        CSTR_WHITE"Nome: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Versao: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Uptime: "CSTR_YELLOW"%s\n\n"
        CSTR_WHITE"Jogadores online: "CSTR_SYSTEM"%d/%d\n"
        CSTR_WHITE"Pico de jogadores: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Total de conexoes: "CSTR_SYSTEM"%d\n\n"
        CSTR_WHITE"Banco de dados: %s\n"
        CSTR_WHITE"Total de queries: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Erros registrados: "CSTR_SYSTEM"%d\n\n"
        CSTR_WHITE"Scheduler tick: "CSTR_SYSTEM"%d\n",
        SERVER_NAME,
        SERVER_VERSION,
        uptime_str,
        online, MAX_PLAYERS,
        g_PeakPlayers,
        g_TotalConnections,
        db_ok ? CSTR_SUCCESS"Conectado" : CSTR_ERROR"DESCONECTADO",
        g_TotalQueries,
        g_TotalErrors,
        Scheduler_GetTick());

    UI_ShowDialog(playerid, DIALOG_NONE + 60, DIALOG_STYLE_MSGBOX,
        "{ Health Check", body, "Fechar", "");
}

// Relatorio periodico nos logs (a cada hora)
static Perf_LogReport() {
    new uptime_str[32];
    FormatTime(Perf_GetUptime(), uptime_str, sizeof(uptime_str));
    new report[256];
    format(report, sizeof(report),
        "Uptime=%s | Online=%d | Peak=%d | Queries=%d | Errors=%d",
        uptime_str,
        Perf_GetOnlineCount(),
        g_PeakPlayers,
        g_TotalQueries,
        g_TotalErrors);
    Logger_Info("PERF", report);
}

// Scheduler
public OnScheduler1Minute() {
    static s_PerfTick = 0;
    s_PerfTick++;
    if(s_PerfTick >= 60) {
        s_PerfTick = 0;
        Perf_LogReport();
    }
}

// Integracao
public OnPlayerConnect(playerid) {
    Perf_OnConnect(playerid);
    return 1;
}

// Comando /serverstatus ja esta no admin_core.inc
// Este modulo adiciona o health check detalhado
CMD:healthcheck(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Perf_ShowHealthCheck(playerid);
    return 1;
}

CMD:uptime(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!Player_IsAdmin(playerid, ADMIN_HELPER)) return 1;
    new uptime_str[32];
    FormatTime(Perf_GetUptime(), uptime_str, sizeof(uptime_str));
    new msg[64];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[Servidor] "CSTR_WHITE"Uptime: "CSTR_YELLOW"%s"
        CSTR_WHITE" | Online: "CSTR_SYSTEM"%d/%d",
        uptime_str, Perf_GetOnlineCount(), MAX_PLAYERS);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}
