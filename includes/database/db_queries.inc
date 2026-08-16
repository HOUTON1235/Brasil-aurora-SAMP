// ============================================================
//  Santa Aurora Roleplay — Database Query Helpers
//  Módulo : database/db_queries.inc
//  Objetivo: helpers para leitura de resultados MySQL
// ============================================================

#if defined _SA_DB_QUERIES_INC
    #endinput
#endif
#define _SA_DB_QUERIES_INC

#include "db_core.inc"

// ------------------------------------------------------------
// Helpers de leitura de cache de resultado
// Devem ser chamados dentro de um callback de query
// ------------------------------------------------------------

// Número de linhas retornadas
stock DB_NumRows() {
    return cache_num_rows();
}

// Número de campos afetados (para UPDATE/DELETE)
stock DB_AffectedRows() {
    return cache_affected_rows();
}

// Último ID inserido (para INSERT)
stock DB_InsertID() {
    return cache_insert_id();
}

// Verificar se resultado existe
stock bool:DB_HasResult() {
    return (cache_num_rows() > 0);
}

// Obter inteiro de um campo (linha 0 por padrão)
stock DB_GetInt(const field[], row = 0) {
    new value;
    cache_get_value_name_int(row, field, value);
    return value;
}

// Obter float de um campo
stock Float:DB_GetFloat(const field[], row = 0) {
    new Float:value;
    cache_get_value_name_float(row, field, value);
    return value;
}

// Obter string de um campo
stock DB_GetString(const field[], output[], size = sizeof(output), row = 0) {
    cache_get_value_name(row, field, output, size);
}

// Obter inteiro por índice de coluna
stock DB_GetIntByIndex(col, row = 0) {
    new value;
    cache_get_value_int(row, col, value);
    return value;
}

// Obter string por índice de coluna
stock DB_GetStringByIndex(col, output[], size = sizeof(output), row = 0) {
    cache_get_value(row, col, output, size);
}

// ------------------------------------------------------------
// Query helpers de alto nível
// ------------------------------------------------------------

// Executar SELECT e retornar inteiro do primeiro campo
// Uso: DB_QueryGetInt("SELECT money FROM characters WHERE id=1", "money")
stock DB_QueryScalarInt(const query[], const field[]) {
    new result = 0;
    // Nota: esta é síncrona — usar apenas quando necessário
    // Prefira o padrão assíncrono com callback
    if(!DB_IsConnected()) return result;

    new Cache:cache = mysql_query(DB_GetHandle(), query);
    if(cache_num_rows() > 0) {
        cache_get_value_name_int(0, field, result);
    }
    cache_delete(cache);
    return result;
}

// Verificar existência de registro
stock bool:DB_RecordExists(const table[], const field[], const value[]) {
    if(!DB_IsConnected()) return false;

    new escaped[128];
    DB_Escape(value, escaped, sizeof(escaped));

    new query[256];
    format(query, sizeof(query),
        "SELECT 1 FROM `%s` WHERE `%s` = '%s' LIMIT 1",
        table, field, escaped);

    new Cache:cache = mysql_query(DB_GetHandle(), query);
    new bool:exists = (cache_num_rows() > 0);
    cache_delete(cache);
    return exists;
}

// Verificar existência de registro por inteiro
stock bool:DB_RecordExistsInt(const table[], const field[], value) {
    if(!DB_IsConnected()) return false;

    new query[256];
    format(query, sizeof(query),
        "SELECT 1 FROM `%s` WHERE `%s` = %d LIMIT 1",
        table, field, value);

    new Cache:cache = mysql_query(DB_GetHandle(), query);
    new bool:exists = (cache_num_rows() > 0);
    cache_delete(cache);
    return exists;
}

// ------------------------------------------------------------
// Geração de UPDATE SET dinâmica (helper para saves)
// ------------------------------------------------------------

// Buffer para construção de queries de update
static g_QueryBuffer[MAX_QUERY_LENGTH * 4];
static g_QueryBufferPos = 0;

stock QB_Reset() {
    g_QueryBuffer[0] = '\0';
    g_QueryBufferPos = 0;
}

stock QB_AddInt(const field[], value) {
    new part[64];
    format(part, sizeof(part), "`%s`=%d,", field, value);
    strcat(g_QueryBuffer, part, sizeof(g_QueryBuffer));
}

stock QB_AddFloat(const field[], Float:value) {
    new part[64];
    format(part, sizeof(part), "`%s`=%.4f,", field, value);
    strcat(g_QueryBuffer, part, sizeof(g_QueryBuffer));
}

stock QB_AddString(const field[], const value[]) {
    new escaped[256];
    DB_Escape(value, escaped, sizeof(escaped));
    new part[300];
    format(part, sizeof(part), "`%s`='%s',", field, escaped);
    strcat(g_QueryBuffer, part, sizeof(g_QueryBuffer));
}

// Retorna query de UPDATE sem o último vírgula
stock QB_BuildUpdate(const table[], const where_clause[], output[], size = sizeof(output)) {
    // Remover último ','
    new len = strlen(g_QueryBuffer);
    if(len > 0 && g_QueryBuffer[len-1] == ',') {
        g_QueryBuffer[len-1] = '\0';
    }
    format(output, size, "UPDATE `%s` SET %s WHERE %s", table, g_QueryBuffer, where_clause);
}

