// ============================================================
//  Santa Aurora Roleplay ? Macros e Helpers
//  Modulo : core/macros.inc
//  Objetivo: macros e funcoes utilitarias de alto uso
// ============================================================

#if defined _SA_MACROS_INC
    #endinput
#endif
#define _SA_MACROS_INC

// include removido

// ------------------------------------------------------------
// Validacao de jogador
// ------------------------------------------------------------
#define IsValidPlayer(%0)           ((%0) >= 0 && (%0) < MAX_PLAYERS && IsPlayerConnected(%0))

// ------------------------------------------------------------
// Mensagens coloridas
// ------------------------------------------------------------
#define MSG(%0,%1,%2)               SendClientMessage(%0, %1, %2)
#define MSGALL(%0,%1)               SendClientMessageToAll(%0, %1)

stock SendColorMessage(playerid, color, msg[]) {
    if(IsValidPlayer(playerid)) {
        SendClientMessage(playerid, color, msg);
    }
}

stock SendColorMessageToAll(color, msg[]) {
    SendClientMessageToAll(color, msg);
}

// Formata e envia mensagem
stock SendFormatMessage(playerid, color, fmt[], {Float,_}:...) {
    new str[144];
    format(str, sizeof(str), fmt);
    SendClientMessage(playerid, color, str);
}

// Mensagens semanticas
stock SendError(playerid, msg[]) {
    new str[144];
    format(str, sizeof(str), CSTR_ERROR"[ERRO] "CSTR_WHITE"%s", msg);
    SendClientMessage(playerid, COLOR_ERROR, str);
}

stock SendSuccess(playerid, msg[]) {
    new str[144];
    format(str, sizeof(str), CSTR_SUCCESS"[OK] "CSTR_WHITE"%s", msg);
    SendClientMessage(playerid, COLOR_SUCCESS, str);
}

stock SendInfo(playerid, msg[]) {
    SendClientMessage(playerid, COLOR_INFO, msg);
}

stock SendWarning(playerid, msg[]) {
    new str[144];
    format(str, sizeof(str), CSTR_WARNING"[AVISO] "CSTR_WHITE"%s", msg);
    SendClientMessage(playerid, COLOR_WARNING, str);
}

stock SendSystemMessage(playerid, msg[]) {
    new str[144];
    format(str, sizeof(str), CSTR_SYSTEM"[SISTEMA] "CSTR_WHITE"%s", msg);
    SendClientMessage(playerid, COLOR_SYSTEM, str);
}

stock BroadcastAdminMessage(msg[]) {
    new str[144];
    format(str, sizeof(str), CSTR_ADMIN"[ADM] "CSTR_WHITE"%s", msg);
    SendClientMessageToAll(COLOR_ADMIN, str);
}

// ------------------------------------------------------------
// Distancia
// ------------------------------------------------------------
stock Float:GetDistance3D(Float:x1, Float:y1, Float:z1,
                           Float:x2, Float:y2, Float:z2) {
    new Float:dx = x1 - x2;
    new Float:dy = y1 - y2;
    new Float:dz = z1 - z2;
    return floatsqroot(dx*dx + dy*dy + dz*dz);
}

stock IsPlayerInRange3D(playerid, Float:x, Float:y, Float:z, Float:range) {
    if(!IsValidPlayer(playerid)) return false;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    return GetDistance3D(px, py, pz, x, y, z) <= range;
}

// ------------------------------------------------------------
// Clamp
// ------------------------------------------------------------
stock Clamp(value, min_val, max_val) {
    if(value < min_val) return min_val;
    if(value > max_val) return max_val;
    return value;
}

stock Float:ClampFloat(Float:value, Float:min_val, Float:max_val) {
    if(value < min_val) return min_val;
    if(value > max_val) return max_val;
    return value;
}

// ------------------------------------------------------------
// Formatacao de dinheiro
// ------------------------------------------------------------
stock FormatMoney(value, output[], size = sizeof(output)) {
    if(value < 0) {
        format(output, size, "-$%d", -value);
    } else {
        format(output, size, "$%d", value);
    }
}

// Formata com separador de milhar: $1.234.567
stock FormatMoneyFull(value, output[], size = sizeof(output)) {
    new str[32];
    new bool:negative = (value < 0);
    if(negative) value = -value;

    format(str, sizeof(str), "%d", value);
    new len = strlen(str);
    new result[32];
    new ri = 0;
    for(new i = 0; i < len; i++) {
        if(i > 0 && (len - i) % 3 == 0) {
            result[ri++] = '.';
        }
        result[ri++] = str[i];
    }
    result[ri] = '\0';

    if(negative) {
        format(output, size, "-$%s", result);
    } else {
        format(output, size, "$%s", result);
    }
}

// ------------------------------------------------------------
// Formatacao de tempo
// ------------------------------------------------------------
stock FormatTime(seconds, output[], size = sizeof(output)) {
    new h = seconds / 3600;
    new m = (seconds % 3600) / 60;
    new s = seconds % 60;

    if(h > 0) {
        format(output, size, "%dh %dm %ds", h, m, s);
    } else if(m > 0) {
        format(output, size, "%dm %ds", m, s);
    } else {
        format(output, size, "%ds", s);
    }
}

// ------------------------------------------------------------
// Validacao de nome de personagem/conta
// Aceita apenas letras (a-z, A-Z), underscores e underline em nome RP
// ------------------------------------------------------------
stock IsValidPlayerName(name[]) {
    new len = strlen(name);
    if(len < 3 || len > MAX_NAME_LENGTH) return false;
    for(new i = 0; i < len; i++) {
        new c = name[i];
        if(!((c >= 'a' && c <= 'z') ||
             (c >= 'A' && c <= 'Z') ||
             (c >= '0' && c <= '9') ||
              c == '_')) {
            return false;
        }
    }
    return true;
}

// Verifica se string esta vazia
stock IsNullString(str[]) {
    return (str[0] == '\0');
}

// Copia string com seguranca
stock SafeStrCopy(dest[], src[], size = sizeof(dest)) {
    strmid(dest, src, 0, size - 1, size);
}

