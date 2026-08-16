// ============================================================
//  Santa Aurora Roleplay — Player States
//  Módulo : core/states.inc
//  Objetivo: máquina de estados do jogador
// ============================================================

#if defined _SA_STATES_INC
    #endinput
#endif
#define _SA_STATES_INC

// ------------------------------------------------------------
// Enum de estados do jogador
// Nunca usar múltiplos booleans conflitantes — usar este enum.
// ------------------------------------------------------------
enum E_PLAYER_STATE {
    STATE_DISCONNECTED = 0,     // Desconectado
    STATE_CONNECTING,           // Conectando (antes do login dialog)
    STATE_AUTHENTICATING,       // No processo de login/registro
    STATE_CHARACTER_SELECTION,  // Selecionando/criando personagem
    STATE_TUTORIAL,             // Fazendo tutorial
    STATE_SPAWNED,              // No mundo, jogando normalmente
    STATE_WORKING,              // Trabalhando em um emprego
    STATE_DEAD,                 // Morto, aguardando atendimento/respawn
    STATE_JAILED,               // Preso (RP)
    STATE_HOSPITALIZED,         // Hospitalizado (RP)
    STATE_AFK,                  // Away from keyboard
    STATE_ADMIN_SPECTATING      // Admin em modo spectate
}

// ------------------------------------------------------------
// Verificações de estado
// ------------------------------------------------------------

// Jogador está em estado que pode interagir com o mundo
stock bool:State_CanInteract(playerid) {
    new E_PLAYER_STATE:st = E_PLAYER_STATE:GetPVarInt(playerid, "pState");
    return (st == STATE_SPAWNED || st == STATE_WORKING);
}

// Jogador pode receber dano
stock bool:State_CanTakeDamage(playerid) {
    new E_PLAYER_STATE:st = E_PLAYER_STATE:GetPVarInt(playerid, "pState");
    return (st == STATE_SPAWNED || st == STATE_WORKING || st == STATE_AFK);
}

// Jogador está autenticado (logado)
stock bool:State_IsAuthenticated(playerid) {
    new E_PLAYER_STATE:st = E_PLAYER_STATE:GetPVarInt(playerid, "pState");
    return (st != STATE_DISCONNECTED &&
            st != STATE_CONNECTING &&
            st != STATE_AUTHENTICATING);
}

// Jogador pode usar comandos RP
stock bool:State_CanUseRPCommands(playerid) {
    new E_PLAYER_STATE:st = E_PLAYER_STATE:GetPVarInt(playerid, "pState");
    return (st == STATE_SPAWNED ||
            st == STATE_WORKING ||
            st == STATE_AFK ||
            st == STATE_JAILED ||
            st == STATE_HOSPITALIZED);
}

// Retorna nome legível do estado
stock State_GetName(E_PLAYER_STATE:state, output[], size = sizeof(output)) {
    switch(state) {
        case STATE_DISCONNECTED:         format(output, size, "Desconectado");
        case STATE_CONNECTING:           format(output, size, "Conectando");
        case STATE_AUTHENTICATING:       format(output, size, "Autenticando");
        case STATE_CHARACTER_SELECTION:  format(output, size, "Seleção de Personagem");
        case STATE_TUTORIAL:             format(output, size, "Tutorial");
        case STATE_SPAWNED:              format(output, size, "Ativo");
        case STATE_WORKING:              format(output, size, "Trabalhando");
        case STATE_DEAD:                 format(output, size, "Morto");
        case STATE_JAILED:               format(output, size, "Preso");
        case STATE_HOSPITALIZED:         format(output, size, "Hospitalizado");
        case STATE_AFK:                  format(output, size, "AFK");
        case STATE_ADMIN_SPECTATING:     format(output, size, "Admin Spectate");
        default:                         format(output, size, "Desconhecido");
    }
}

