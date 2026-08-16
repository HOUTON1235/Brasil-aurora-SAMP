// ============================================================
//  Santa Aurora Roleplay — Database Core
//  Módulo : database/db_core.inc
//  Objetivo: gerenciar conexão MySQL, pool, reconexão
// ============================================================

#if defined _SA_DB_CORE_INC
    #endinput
#endif
#define _SA_DB_CORE_INC

#include "../core/defines.inc"
#include "../logs/logger.inc"

// ------------------------------------------------------------
// Configuração de conexão
// Lida do server.ini via arquivo ou hardcoded com variáveis
// ------------------------------------------------------------
static g_DBHost[64]     = "127.0.0.1";
static g_DBUser[32]     = "samp";
static g_DBPass[64]     = "samp_password";
static g_DBName[32]     = "santa_aurora";
static g_DBPort         = 3306;

// Handle de conexão MySQL
static MySQL:g_DBHandle = MYSQL_INVALID_HANDLE;
static bool:g_DBConnected = false;

// Contador de tentativas de reconexão
static g_ReconnectAttempts = 0;
#define MAX_RECONNECT_ATTEMPTS  5
#define RECONNECT_DELAY_MS      5000

// ------------------------------------------------------------
// Configurar credenciais (chamado antes do Connect)
// ------------------------------------------------------------
stock DB_SetCredentials(const host[], const user[], const pass[], const dbname[], port = 3306) {
    SafeStrCopy(g_DBHost, host, sizeof(g_DBHost));
    SafeStrCopy(g_DBUser, user, sizeof(g_DBUser));
    SafeStrCopy(g_DBPass, pass, sizeof(g_DBPass));
    SafeStrCopy(g_DBName, dbname, sizeof(g_DBName));
    g_DBPort = port;
}

// ------------------------------------------------------------
// Conectar ao banco
// ------------------------------------------------------------
stock bool:DB_Connect() {
    Logger_Info("DATABASE", "Conectando ao MySQL...");

    // Configurar opções de conexão
    new MySQLOpt:options = mysql_init_options();
    mysql_set_option(options, AUTO_RECONNECT, true);

    g_DBHandle = mysql_connect_file("scriptfiles/db_config.ini");

    // Fallback: conectar com credenciais diretas
    if(g_DBHandle == MYSQL_INVALID_HANDLE || !mysql_errno(g_DBHandle) == 0) {
        g_DBHandle = mysql_connect(g_DBHost, g_DBUser, g_DBPass, g_DBName);
    }

    if(g_DBHandle == MYSQL_INVALID_HANDLE) {
        Logger_Error("DATABASE", "Falha crítica: impossível criar handle MySQL.");
        g_DBConnected = false;
        return false;
    }

    new errcode = mysql_errno(g_DBHandle);
    if(errcode != 0) {
        new errmsg[256];
        mysql_error(g_DBHandle, errmsg, sizeof(errmsg));
        new logmsg[256];
        format(logmsg, sizeof(logmsg), "Erro MySQL %d: %s", errcode, errmsg);
        Logger_Error("DATABASE", logmsg);
        g_DBConnected = false;
        return false;
    }

    // Definir charset
    mysql_set_charset("utf8mb4", g_DBHandle);

    g_DBConnected = true;
    g_ReconnectAttempts = 0;
    Logger_Info("DATABASE", "Conexão MySQL estabelecida com sucesso.");

    // Disparar evento para módulos saberem que o banco está pronto
    CallLocalFunction("OnDatabaseConnected", "");
    return true;
}

// ------------------------------------------------------------
// Desconectar
// ------------------------------------------------------------
stock DB_Disconnect() {
    if(g_DBHandle != MYSQL_INVALID_HANDLE) {
        mysql_close(g_DBHandle);
        g_DBHandle = MYSQL_INVALID_HANDLE;
    }
    g_DBConnected = false;
    Logger_Info("DATABASE", "Conexão MySQL encerrada.");
}

// ------------------------------------------------------------
// Verificar estado
// ------------------------------------------------------------
stock bool:DB_IsConnected() {
    if(g_DBHandle == MYSQL_INVALID_HANDLE) return false;
    return g_DBConnected;
}

// Obter handle para queries diretas
stock MySQL:DB_GetHandle() {
    return g_DBHandle;
}

// ------------------------------------------------------------
// Escapar string (previne SQL injection)
// SEMPRE usar antes de inserir input do jogador no SQL
// ------------------------------------------------------------
stock DB_Escape(const input[], output[], size = sizeof(output)) {
    if(g_DBHandle == MYSQL_INVALID_HANDLE) {
        SafeStrCopy(output, input, size);
        return;
    }
    mysql_escape_string(input, output, g_DBHandle);
}

// ------------------------------------------------------------
// Executar query síncrona simples (sem resultado esperado)
// Para INSERTs, UPDATEs, DELETEs
// ------------------------------------------------------------
stock DB_Execute(const query[]) {
    if(!DB_IsConnected()) {
        Logger_Error("DATABASE", "DB_Execute: banco não conectado.");
        return;
    }
    mysql_tquery(g_DBHandle, query, "", "");
}

// ------------------------------------------------------------
// Executar query com callback (assíncrona — preferida)
// ------------------------------------------------------------
stock DB_Query(const query[], const callback[], const format[] = "", {Float,_}:...) {
    if(!DB_IsConnected()) {
        Logger_Error("DATABASE", "DB_Query: banco não conectado.");
        return;
    }
    if(strequal(format, "")) {
        mysql_tquery(g_DBHandle, query, callback, "");
    } else {
        // Repassar argumentos variáveis
        mysql_tquery(g_DBHandle, query, callback, format, ___(4));
    }
}

// ------------------------------------------------------------
// Reconexão automática
// ------------------------------------------------------------
forward DB_TryReconnect();
public DB_TryReconnect() {
    if(g_DBConnected) return;

    g_ReconnectAttempts++;
    new msg[64];
    format(msg, sizeof(msg), "Tentativa de reconexão %d/%d...", g_ReconnectAttempts, MAX_RECONNECT_ATTEMPTS);
    Logger_Warning("DATABASE", msg);

    if(g_ReconnectAttempts > MAX_RECONNECT_ATTEMPTS) {
        Logger_Error("DATABASE", "Máximo de tentativas de reconexão atingido. Servidor em estado crítico.");
        return;
    }

    DB_Connect();
}

// ------------------------------------------------------------
// Callback de conexão
// ------------------------------------------------------------
forward OnDatabaseConnected();
public OnDatabaseConnected() {
    Logger_Info("DATABASE", "Banco de dados pronto para uso.");
    return 1;
}

// Criar arquivo de config do banco se não existir
stock DB_CreateConfigFile() {
    new File:f = fopen("scriptfiles/db_config.ini", io_read);
    if(f) {
        fclose(f);
        return; // já existe
    }

    f = fopen("scriptfiles/db_config.ini", io_write);
    if(f) {
        fwrite(f, "[database]\r\n");
        fwrite(f, "hostname=127.0.0.1\r\n");
        fwrite(f, "username=samp\r\n");
        fwrite(f, "password=samp_password\r\n");
        fwrite(f, "database=santa_aurora\r\n");
        fwrite(f, "port=3306\r\n");
        fwrite(f, "charset=utf8mb4\r\n");
        fclose(f);
        Logger_Info("DATABASE", "Arquivo db_config.ini criado. Configure antes de iniciar.");
    }
}

