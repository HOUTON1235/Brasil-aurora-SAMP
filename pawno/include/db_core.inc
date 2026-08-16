#if defined _SA_DB_CORE_INC
    #endinput
#endif
#define _SA_DB_CORE_INC

static g_DBHost[64]  = "127.0.0.1";
static g_DBUser[32]  = "samp";
static g_DBPass[64]  = "samp_password";
static g_DBName[32]  = "santa_aurora";

static MySQL:g_DBHandle    = MYSQL_INVALID_HANDLE;
static bool:g_DBConnected  = false;
static g_ReconnectAttempts = 0;
#define MAX_RECONNECT_ATTEMPTS 5

stock bool:DB_Connect() {
    Logger_Info("DATABASE", "Conectando ao MySQL...");
    g_DBHandle = mysql_connect(g_DBHost, g_DBUser, g_DBPass, g_DBName);
    if(g_DBHandle == MYSQL_INVALID_HANDLE) {
        Logger_Error("DATABASE", "Handle MySQL invalido.");
        g_DBConnected = false;
        return false;
    }
    new errcode = mysql_errno(g_DBHandle);
    if(errcode != 0) {
        new msg[64];
        format(msg, sizeof(msg), "Erro MySQL: %d", errcode);
        Logger_Error("DATABASE", msg);
        g_DBConnected = false;
        return false;
    }
    g_DBConnected = true;
    g_ReconnectAttempts = 0;
    Logger_Info("DATABASE", "MySQL conectado.");
    CallLocalFunction("OnDatabaseConnected", "");
    return true;
}

stock DB_Disconnect() {
    if(g_DBHandle != MYSQL_INVALID_HANDLE) {
        mysql_close(g_DBHandle);
        g_DBHandle = MYSQL_INVALID_HANDLE;
    }
    g_DBConnected = false;
}

stock bool:DB_IsConnected() {
    if(g_DBHandle == MYSQL_INVALID_HANDLE) return false;
    return g_DBConnected;
}

stock MySQL:DB_GetHandle() { return g_DBHandle; }

stock DB_Escape(input[], output[], size) {
    if(g_DBHandle == MYSQL_INVALID_HANDLE) {
        strmid(output, input, 0, size - 1, size);
        return;
    }
    mysql_escape_string(input, output, g_DBHandle);
}

stock DB_Execute(query[]) {
    if(!g_DBConnected) return;
    mysql_tquery(g_DBHandle, query, "", "");
}

stock DB_Query(query[], callback[], fmt[] = "") {
    if(!g_DBConnected) return;
    mysql_tquery(g_DBHandle, query, callback, fmt);
}

stock DB_QueryI(query[], callback[], arg) {
    if(!g_DBConnected) return;
    mysql_tquery(g_DBHandle, query, callback, "i", arg);
}

stock DB_QueryII(query[], callback[], arg1, arg2) {
    if(!g_DBConnected) return;
    mysql_tquery(g_DBHandle, query, callback, "ii", arg1, arg2);
}

stock bool:DB_HasResult() { return (cache_num_rows() > 0); }
stock DB_NumRows()        { return cache_num_rows(); }
stock DB_InsertID()       { return cache_insert_id(); }

stock DB_GetInt(field[], row = 0) {
    new val;
    cache_get_value_name_int(row, field, val);
    return val;
}

stock Float:DB_GetFloat(field[], row = 0) {
    new Float:val;
    cache_get_value_name_float(row, field, val);
    return val;
}

stock DB_GetString(field[], output[], size, row = 0) {
    cache_get_value_name(row, field, output, size);
}

forward OnDatabaseConnected();
public OnDatabaseConnected() {
    Logger_Info("DATABASE", "Banco pronto.");
    return 1;
}

forward DB_TryReconnect();
public DB_TryReconnect() {
    if(g_DBConnected) return;
    g_ReconnectAttempts++;
    if(g_ReconnectAttempts > MAX_RECONNECT_ATTEMPTS) return;
    DB_Connect();
}
