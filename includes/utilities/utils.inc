// ============================================================
//  Santa Aurora Roleplay — Utilities
//  Módulo : utilities/utils.inc
//  Objetivo: funções utilitárias gerais
// ============================================================

#if defined _SA_UTILS_INC
    #endinput
#endif
#define _SA_UTILS_INC

#include "../core/defines.inc"

// ------------------------------------------------------------
// Formatação de dinheiro com separador de milhar
// Retorna string no formato $1.234.567
// ------------------------------------------------------------
stock Utils_FormatMoney(value, output[], size = sizeof(output)) {
    FormatMoneyFull(value, output, size);
}

// ------------------------------------------------------------
// Formatação de tempo
// ------------------------------------------------------------
stock Utils_FormatTime(seconds, output[], size = sizeof(output)) {
    FormatTime(seconds, output, size);
}

// ------------------------------------------------------------
// Data e hora atual formatada
// ------------------------------------------------------------
stock Utils_FormatDate(output[], size = sizeof(output)) {
    new y, m, d, h, min, sec;
    getdate(y, m, d);
    gettime(h, min, sec);
    format(output, size, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, min, sec);
}

// Apenas data
stock Utils_FormatDateOnly(output[], size = sizeof(output)) {
    new y, m, d;
    getdate(y, m, d);
    format(output, size, "%04d-%02d-%02d", y, m, d);
}

// Timestamp Unix atual
stock Utils_GetTimestamp() {
    return gettime();
}

// ------------------------------------------------------------
// Validação de nome
// ------------------------------------------------------------
stock bool:Utils_IsValidName(const name[]) {
    return IsValidPlayerName(name);
}

// ------------------------------------------------------------
// Distância 3D
// ------------------------------------------------------------
stock Float:Utils_GetDistance3D(Float:x1, Float:y1, Float:z1,
                                 Float:x2, Float:y2, Float:z2) {
    return GetDistance3D(x1, y1, z1, x2, y2, z2);
}

// Distância 2D (ignorando Z)
stock Float:Utils_GetDistance2D(Float:x1, Float:y1, Float:x2, Float:y2) {
    new Float:dx = x1 - x2;
    new Float:dy = y1 - y2;
    return floatsqroot(dx*dx + dy*dy);
}

// ------------------------------------------------------------
// Clamp
// ------------------------------------------------------------
stock Utils_Clamp(value, min_val, max_val) {
    return Clamp(value, min_val, max_val);
}

stock Float:Utils_ClampFloat(Float:value, Float:min_val, Float:max_val) {
    return ClampFloat(value, min_val, max_val);
}

// ------------------------------------------------------------
// String helpers
// ------------------------------------------------------------

// Verificar se string é número inteiro
stock bool:Utils_IsNumeric(const str[]) {
    new i = 0;
    if(str[0] == '-') i = 1;
    for(; str[i] != '\0'; i++) {
        if(str[i] < '0' || str[i] > '9') return false;
    }
    return (strlen(str) > 0);
}

// Trim espaços do início e fim
stock Utils_Trim(str[], size = sizeof(str)) {
    new start = 0;
    while(str[start] == ' ') start++;

    new end = strlen(str) - 1;
    while(end > start && str[end] == ' ') end--;

    new result[256];
    strmid(result, str, start, end + 1, sizeof(result));
    format(str, size, "%s", result);
}

// Capitalize primeira letra
stock Utils_Capitalize(str[], size = sizeof(str)) {
    if(strlen(str) > 0 && str[0] >= 'a' && str[0] <= 'z') {
        str[0] = str[0] - 32;
    }
    #pragma unused size
}

// Contar ocorrências de char em string
stock Utils_CountChar(const str[], char) {
    new count = 0;
    for(new i = 0; str[i] != '\0'; i++) {
        if(str[i] == char) count++;
    }
    return count;
}

// ------------------------------------------------------------
// Utilitários de jogador
// ------------------------------------------------------------

// Obter nome do jogador de forma segura
stock Utils_GetPlayerName(playerid, output[], size = sizeof(output)) {
    if(IsValidPlayer(playerid)) {
        GetPlayerName(playerid, output, size);
    } else {
        format(output, size, "Desconhecido");
    }
}

// Verificar se jogador está perto de uma posição
stock bool:Utils_IsPlayerNear(playerid, Float:x, Float:y, Float:z,
                               Float:radius = INTERACTION_RANGE) {
    return IsPlayerInRange3D(playerid, x, y, z, radius);
}

// ------------------------------------------------------------
// Números aleatórios
// ------------------------------------------------------------
stock Utils_Random(min_val, max_val) {
    if(min_val >= max_val) return min_val;
    return min_val + random(max_val - min_val + 1);
}

// Gerar ID único simples (baseado em tempo + random)
stock Utils_GenerateUID(output[], size = sizeof(output)) {
    format(output, size, "%d%04d", gettime(), random(9999));
}

// ------------------------------------------------------------
// Verificação de IP (básico)
// ------------------------------------------------------------
stock bool:Utils_IsValidIP(const ip[]) {
    new dots = Utils_CountChar(ip, '.');
    return (dots == 3 && strlen(ip) >= 7);
}

// Verificar se IP é local (localhost)
stock bool:Utils_IsLocalIP(const ip[]) {
    return (strfind(ip, "127.0.0.1", true) != -1 ||
            strfind(ip, "192.168.",  true) != -1 ||
            strfind(ip, "10.0.",     true) != -1);
}

