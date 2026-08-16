#if defined _SA_STATES_INC
    #endinput
#endif
#define _SA_STATES_INC

#define STATE_DISCONNECTED       0
#define STATE_CONNECTING         1
#define STATE_AUTHENTICATING     2
#define STATE_CHARACTER_SELECTION 3
#define STATE_TUTORIAL           4
#define STATE_SPAWNED            5
#define STATE_WORKING            6
#define STATE_DEAD               7
#define STATE_JAILED             8
#define STATE_HOSPITALIZED       9
#define STATE_AFK                10
#define STATE_ADMIN_SPECTATING   11

stock State_CanInteract(playerid) {
    new st = GetPVarInt(playerid, "pState");
    return (st == STATE_SPAWNED || st == STATE_WORKING);
}

stock State_CanTakeDamage(playerid) {
    new st = GetPVarInt(playerid, "pState");
    return (st == STATE_SPAWNED || st == STATE_WORKING || st == STATE_AFK);
}

stock State_IsAuthenticated(playerid) {
    new st = GetPVarInt(playerid, "pState");
    return (st != STATE_DISCONNECTED && st != STATE_CONNECTING && st != STATE_AUTHENTICATING);
}

stock State_CanUseRPCommands(playerid) {
    new st = GetPVarInt(playerid, "pState");
    return (st == STATE_SPAWNED || st == STATE_WORKING || st == STATE_AFK
         || st == STATE_JAILED || st == STATE_HOSPITALIZED);
}

#define State_GetName(%0,%1,%2) format(%1,%2,\
    (%0)==STATE_DISCONNECTED?"Desconectado":\
    (%0)==STATE_CONNECTING?"Conectando":\
    (%0)==STATE_AUTHENTICATING?"Autenticando":\
    (%0)==STATE_CHARACTER_SELECTION?"Criacao de Personagem":\
    (%0)==STATE_TUTORIAL?"Tutorial":\
    (%0)==STATE_SPAWNED?"Ativo":\
    (%0)==STATE_WORKING?"Trabalhando":\
    (%0)==STATE_DEAD?"Morto":\
    (%0)==STATE_JAILED?"Preso":\
    (%0)==STATE_HOSPITALIZED?"Hospitalizado":\
    (%0)==STATE_AFK?"AFK":\
    (%0)==STATE_ADMIN_SPECTATING?"Admin Spectate":"Desconhecido")
