// ============================================================
//  Santa Aurora Roleplay — Player Core
//  Módulo : players/player_core.inc
//  Objetivo: struct do jogador, getters/setters, lifecycle
// ============================================================

#if defined _SA_PLAYER_CORE_INC
    #endinput
#endif
#define _SA_PLAYER_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../core/states.inc"
#include "../core/events.inc"
#include "../core/permissions.inc"
#include "../logs/logger.inc"

// ------------------------------------------------------------
// Struct principal do jogador
// ------------------------------------------------------------
enum E_PLAYER_DATA {
    // === Conta ===
    pAccountID,                         // ID na tabela accounts
    pCharacterID,                       // ID na tabela characters

    // === Administrativo ===
    pAdminLevel,                        // 0-7
    pAdminName[MAX_PLAYER_NAME],

    // === Personagem ===
    pName[MAX_PLAYER_NAME + 4],         // nome RP
    pSex,                               // 0=masculino 1=feminino
    pAge,                               // idade RP
    pSkin,

    // === Vitais ===
    Float:pHealth,                      // 0.0 - 100.0
    Float:pArmour,                      // 0.0 - 100.0
    pHunger,                            // 0-100
    pThirst,                            // 0-100

    // === Econômico ===
    pMoney,                             // dinheiro físico
    pBank,                              // saldo bancário

    // === Progressão ===
    pLevel,                             // 1-100
    pExperience,                        // XP total

    // === Emprego / Facção ===
    pJobID,                             // 0 = sem emprego
    pFactionID,                         // 0 = sem facção
    pFactionRank,

    // === Estado da sessão ===
    E_PLAYER_STATE:pState,
    bool:pIsLogged,
    bool:pIsDirty,                      // precisa salvar?
    bool:pIsAFK,
    pAFKTimer,
    pLastActivityTick,
    pLoginAttempts,
    pSessionStartTime,

    // === Posição salva ===
    Float:pPosX,
    Float:pPosY,
    Float:pPosZ,
    Float:pPosA,
    pInterior,
    pWorld,

    // === Sessão online ===
    pConnectTime,
    pTotalPlaytime                      // segundos totais
}

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];

// Armazenar IP da sessão
static g_PlayerIP[MAX_PLAYERS][16];
static g_PlayerSerial[MAX_PLAYERS][41];

// ------------------------------------------------------------
// Posição de spawn padrão (centro de Santa Aurora)
// ------------------------------------------------------------
#define SPAWN_DEFAULT_X     1545.5
#define SPAWN_DEFAULT_Y     -1675.7
#define SPAWN_DEFAULT_Z     13.5
#define SPAWN_DEFAULT_A     90.0

// ------------------------------------------------------------
// Inicializar / Limpar dados do jogador
// ------------------------------------------------------------
stock Player_Init(playerid) {
    if(!IsValidPlayer(playerid)) return;

    // Zerar struct
    PlayerData[playerid][pAccountID]       = INVALID_DB_ID;
    PlayerData[playerid][pCharacterID]     = INVALID_DB_ID;
    PlayerData[playerid][pAdminLevel]      = ADMIN_NONE;
    PlayerData[playerid][pAdminName][0]    = '\0';
    PlayerData[playerid][pName][0]         = '\0';
    PlayerData[playerid][pSex]             = 0;
    PlayerData[playerid][pAge]             = 18;
    PlayerData[playerid][pSkin]            = 0;
    PlayerData[playerid][pHealth]          = 100.0;
    PlayerData[playerid][pArmour]          = 0.0;
    PlayerData[playerid][pHunger]          = 100;
    PlayerData[playerid][pThirst]          = 100;
    PlayerData[playerid][pMoney]           = 0;
    PlayerData[playerid][pBank]            = 0;
    PlayerData[playerid][pLevel]           = 1;
    PlayerData[playerid][pExperience]      = 0;
    PlayerData[playerid][pJobID]           = INVALID_JOB_ID;
    PlayerData[playerid][pFactionID]       = INVALID_FACTION_ID;
    PlayerData[playerid][pFactionRank]     = 0;
    PlayerData[playerid][pState]           = STATE_DISCONNECTED;
    PlayerData[playerid][pIsLogged]        = false;
    PlayerData[playerid][pIsDirty]         = false;
    PlayerData[playerid][pIsAFK]           = false;
    PlayerData[playerid][pAFKTimer]        = -1;
    PlayerData[playerid][pLastActivityTick]= 0;
    PlayerData[playerid][pLoginAttempts]   = 0;
    PlayerData[playerid][pSessionStartTime]= 0;
    PlayerData[playerid][pPosX]            = SPAWN_DEFAULT_X;
    PlayerData[playerid][pPosY]            = SPAWN_DEFAULT_Y;
    PlayerData[playerid][pPosZ]            = SPAWN_DEFAULT_Z;
    PlayerData[playerid][pPosA]            = SPAWN_DEFAULT_A;
    PlayerData[playerid][pInterior]        = 0;
    PlayerData[playerid][pWorld]           = 0;
    PlayerData[playerid][pConnectTime]     = 0;
    PlayerData[playerid][pTotalPlaytime]   = 0;

    g_PlayerIP[playerid][0]     = '\0';
    g_PlayerSerial[playerid][0] = '\0';

    // Capturar IP e GPCI
    GetPlayerIp(playerid, g_PlayerIP[playerid], sizeof(g_PlayerIP[]));
    gpci(playerid, g_PlayerSerial[playerid], sizeof(g_PlayerSerial[]));

    PlayerData[playerid][pConnectTime] = gettime();
}

// ------------------------------------------------------------
// Getters e Setters com validação
// ------------------------------------------------------------

stock Player_GetMoney(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return PlayerData[playerid][pMoney];
}

stock Player_AddMoney(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;

    new current = PlayerData[playerid][pMoney];
    new newAmount = current + amount;

    // Prevenir overflow
    if(newAmount < current) {
        Logger_Security(playerid, "MONEY_OVERFLOW", "Tentativa de overflow de dinheiro");
        return false;
    }
    if(newAmount > MAX_MONEY) newAmount = MAX_MONEY;

    PlayerData[playerid][pMoney] = newAmount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, newAmount);
    Player_MarkDirty(playerid);

    // Disparar evento
    new reason[32] = "Crédito";
    FireEvent_iis("OnMoneyChanged", playerid, current, reason);
    return true;
}

stock bool:Player_RemoveMoney(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;

    new current = PlayerData[playerid][pMoney];
    if(current < amount) return false; // sem fundos suficientes

    new newAmount = current - amount;
    if(newAmount < 0) newAmount = 0;

    PlayerData[playerid][pMoney] = newAmount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, newAmount);
    Player_MarkDirty(playerid);

    new reason[32] = "Débito";
    FireEvent_iis("OnMoneyChanged", playerid, current, reason);
    return true;
}

// Definir dinheiro diretamente (apenas para load/admin)
stock Player_SetMoney(playerid, amount) {
    if(!IsValidPlayer(playerid)) return;
    amount = Clamp(amount, MIN_MONEY, MAX_MONEY);
    PlayerData[playerid][pMoney] = amount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, amount);
}

stock Player_GetBank(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return PlayerData[playerid][pBank];
}

stock bool:Player_AddBank(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;
    new newAmount = PlayerData[playerid][pBank] + amount;
    if(newAmount > MAX_BANK_BALANCE) return false;
    PlayerData[playerid][pBank] = newAmount;
    Player_MarkDirty(playerid);
    return true;
}

stock bool:Player_RemoveBank(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;
    if(PlayerData[playerid][pBank] < amount) return false;
    PlayerData[playerid][pBank] -= amount;
    Player_MarkDirty(playerid);
    return true;
}

// ------------------------------------------------------------
// Gerenciamento de estado
// ------------------------------------------------------------
stock Player_SetState(playerid, E_PLAYER_STATE:state) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pState] = state;
    SetPVarInt(playerid, "pState", _:state);
}

stock E_PLAYER_STATE:Player_GetState(playerid) {
    if(!IsValidPlayer(playerid)) return STATE_DISCONNECTED;
    return PlayerData[playerid][pState];
}

// ------------------------------------------------------------
// Dirty flag — evita gravações desnecessárias
// ------------------------------------------------------------
stock Player_MarkDirty(playerid) {
    if(IsValidPlayer(playerid))
        PlayerData[playerid][pIsDirty] = true;
}

stock Player_ClearDirty(playerid) {
    if(IsValidPlayer(playerid))
        PlayerData[playerid][pIsDirty] = false;
}

stock bool:Player_IsDirty(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return PlayerData[playerid][pIsDirty];
}

// ------------------------------------------------------------
// Permissões
// ------------------------------------------------------------
stock Player_GetAdminLevel(playerid) {
    if(!IsValidPlayer(playerid)) return ADMIN_NONE;
    return PlayerData[playerid][pAdminLevel];
}

stock bool:Player_IsAdmin(playerid, minLevel = ADMIN_HELPER) {
    if(!IsValidPlayer(playerid)) return false;
    return (PlayerData[playerid][pAdminLevel] >= minLevel);
}

stock bool:Player_HasPermission(playerid, const permission[]) {
    if(!IsValidPlayer(playerid)) return false;
    new level = PlayerData[playerid][pAdminLevel];
    if(level >= ADMIN_OWNER) return true; // owner tem tudo

    // Varrer mapa de permissões
    for(new i = 0; i < MAX_PERMISSION_ENTRIES; i++) {
        if(strequal(PermissionMinLevel[i][0], permission)) {
            return (level >= PermissionMinLevel[i][1]);
        }
    }
    return false;
}

// ------------------------------------------------------------
// Getters de sessão
// ------------------------------------------------------------
stock Player_GetIP(playerid, output[], size = sizeof(output)) {
    if(!IsValidPlayer(playerid)) return;
    SafeStrCopy(output, g_PlayerIP[playerid], size);
}

stock Player_GetSerial(playerid, output[], size = sizeof(output)) {
    if(!IsValidPlayer(playerid)) return;
    SafeStrCopy(output, g_PlayerSerial[playerid], size);
}

stock Player_GetSessionTime(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return gettime() - PlayerData[playerid][pConnectTime];
}

// ------------------------------------------------------------
// AFK Detection
// ------------------------------------------------------------
stock Player_UpdateActivity(playerid) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pLastActivityTick] = GetTickCount();
    if(PlayerData[playerid][pIsAFK]) {
        PlayerData[playerid][pIsAFK] = false;
        Player_SetState(playerid, STATE_SPAWNED);
        SendInfo(playerid, "Você voltou de AFK.");
    }
}

// Verificar AFK — chamado pelo scheduler
stock Player_CheckAFK(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!PlayerData[playerid][pIsLogged]) return;
    if(Player_GetState(playerid) == STATE_ADMIN_SPECTATING) return;
    if(Player_GetState(playerid) == STATE_DEAD) return;

    new elapsed = (GetTickCount() - PlayerData[playerid][pLastActivityTick]) / 1000;
    if(elapsed >= AFK_TIMEOUT && !PlayerData[playerid][pIsAFK]) {
        PlayerData[playerid][pIsAFK] = true;
        Player_SetState(playerid, STATE_AFK);
        SendWarning(playerid, "Você está em modo AFK. Mova-se ou execute um comando para voltar.");
    }
}

// ------------------------------------------------------------
// Save (chamado em logout, shutdown, save automático)
// ------------------------------------------------------------
stock Player_Save(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!PlayerData[playerid][pIsLogged]) return;
    if(PlayerData[playerid][pCharacterID] == INVALID_DB_ID) return;

    // Atualizar posição atual
    if(Player_GetState(playerid) >= STATE_SPAWNED) {
        GetPlayerPos(playerid,
            PlayerData[playerid][pPosX],
            PlayerData[playerid][pPosY],
            PlayerData[playerid][pPosZ]);
        GetPlayerFacingAngle(playerid, PlayerData[playerid][pPosA]);
        PlayerData[playerid][pInterior] = GetPlayerInterior(playerid);
        PlayerData[playerid][pWorld]    = GetPlayerVirtualWorld(playerid);
    }

    // Acumular tempo de jogo
    PlayerData[playerid][pTotalPlaytime] += Player_GetSessionTime(playerid);

    new query[MAX_QUERY_LENGTH * 2];
    new escaped_name[MAX_NAME_LENGTH * 2];
    DB_Escape(PlayerData[playerid][pName], escaped_name, sizeof(escaped_name));

    format(query, sizeof(query),
        "UPDATE `characters` SET "
        "`name`='%s', `health`=%.2f, `armour`=%.2f, "
        "`hunger`=%d, `thirst`=%d, "
        "`money`=%d, `bank`=%d, "
        "`level`=%d, `experience`=%d, "
        "`job_id`=%d, `faction_id`=%d, `faction_rank`=%d, "
        "`pos_x`=%.4f, `pos_y`=%.4f, `pos_z`=%.4f, `pos_a`=%.4f, "
        "`interior`=%d, `world`=%d, "
        "`total_playtime`=%d, "
        "`updated_at`=NOW() "
        "WHERE `id`=%d",
        escaped_name,
        PlayerData[playerid][pHealth],
        PlayerData[playerid][pArmour],
        PlayerData[playerid][pHunger],
        PlayerData[playerid][pThirst],
        PlayerData[playerid][pMoney],
        PlayerData[playerid][pBank],
        PlayerData[playerid][pLevel],
        PlayerData[playerid][pExperience],
        PlayerData[playerid][pJobID],
        PlayerData[playerid][pFactionID],
        PlayerData[playerid][pFactionRank],
        PlayerData[playerid][pPosX],
        PlayerData[playerid][pPosY],
        PlayerData[playerid][pPosZ],
        PlayerData[playerid][pPosA],
        PlayerData[playerid][pInterior],
        PlayerData[playerid][pWorld],
        PlayerData[playerid][pTotalPlaytime],
        PlayerData[playerid][pCharacterID]
    );

    DB_Execute(query);
    Player_ClearDirty(playerid);

    new logmsg[64];
    format(logmsg, sizeof(logmsg), "Personagem salvo: %s (ID:%d)",
        PlayerData[playerid][pName], PlayerData[playerid][pCharacterID]);
    Logger_Info("PLAYER", logmsg);

    CallLocalFunction("OnPlayerSaved", "i", playerid);
}

// ------------------------------------------------------------
// Scheduler: salvar jogadores sujos a cada 5 minutos
// ------------------------------------------------------------
public OnScheduler5Minutes() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && Player_IsDirty(i)) {
            Player_Save(i);
        }
    }
}

// Verificar AFK a cada 10 segundos
public OnScheduler10Seconds() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {
            Player_CheckAFK(i);
        }
    }
}

