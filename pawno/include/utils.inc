// ============================================================
//  Santa Aurora Roleplay ? Utilities
//  Modulo : utilities/utils.inc
//  Objetivo: funcoes utilitarias gerais
// ============================================================

#if defined _SA_UTILS_INC
    #endinput
#endif
#define _SA_UTILS_INC

// include removido

// ------------------------------------------------------------
// Formatacao de dinheiro com separador de milhar
// Retorna string no formato $1.234.567
// ------------------------------------------------------------
stock Utils_FormatMoney(value, output[], size = sizeof(output)) {
    FormatMoneyFull(value, output, size);
}

// ------------------------------------------------------------
// Formatacao de tempo
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
// Validacao de nome
// ------------------------------------------------------------
stock Utils_IsValidName(name[]) {
    return IsValidPlayerName(name);
}

// ------------------------------------------------------------
// Distancia 3D
// ------------------------------------------------------------
stock Float:Utils_GetDistance3D(Float:x1, Float:y1, Float:z1,
                                 Float:x2, Float:y2, Float:z2) {
    return GetDistance3D(x1, y1, z1, x2, y2, z2);
}

// Distancia 2D (ignorando Z)
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

// Verificar se string e numero inteiro
stock Utils_IsNumeric(str[]) {
    new i = 0;
    if(str[0] == '-') i = 1;
    for(; str[i] != '\0'; i++) {
        if(str[i] < '0' || str[i] > '9') return false;
    }
    return (strlen(str) > 0);
}

// Trim espacos do inicio e fim
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

// Contar ocorrencias de char em string
stock Utils_CountChar(str[], ch) {
    new count = 0;
    for(new i = 0; str[i] != '\0'; i++) {
        if(str[i] == ch) count++;
    }
    return count;
}

// ------------------------------------------------------------
// Utilitarios de jogador
// ------------------------------------------------------------

// Obter nome do jogador de forma segura
stock Utils_GetPlayerName(playerid, output[], size = sizeof(output)) {
    if(IsValidPlayer(playerid)) {
        GetPlayerName(playerid, output, size);
    } else {
        format(output, size, "Desconhecido");
    }
}

// Verificar se jogador esta perto de uma posicao
stock Utils_IsPlayerNear(playerid, Float:x, Float:y, Float:z,
                               Float:radius = INTERACTION_RANGE) {
    return IsPlayerInRange3D(playerid, x, y, z, radius);
}

// ------------------------------------------------------------
// Numeros aleatorios
// ------------------------------------------------------------
stock Utils_Random(min_val, max_val) {
    if(min_val >= max_val) return min_val;
    return min_val + random(max_val - min_val + 1);
}

// Gerar ID unico simples (baseado em tempo + random)
stock Utils_GenerateUID(output[], size = sizeof(output)) {
    format(output, size, "%d%04d", gettime(), random(9999));
}

// ------------------------------------------------------------
// Verificacao de IP (basico)
// ------------------------------------------------------------
stock Utils_IsValidIP(ip[]) {
    new dots = Utils_CountChar(ip, '.');
    return (dots == 3 && strlen(ip) >= 7);
}

// Verificar se IP e local (localhost)
stock Utils_IsLocalIP(ip[]) {
    return (strfind(ip, "127.0.0.1", true) != -1 ||
            strfind(ip, "192.168.",  true) != -1 ||
            strfind(ip, "10.0.",     true) != -1);
}

