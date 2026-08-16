#if defined _SA_PLAYER_CORE_INC
    #endinput
#endif
#define _SA_PLAYER_CORE_INC

enum E_PLAYER_DATA {
    pAccountID,
    pCharacterID,
    pAdminLevel,
    pAdminName[MAX_PLAYER_NAME],
    pName[MAX_PLAYER_NAME + 4],
    pSex,
    pAge,
    pSkin,
    Float:pHealth,
    Float:pArmour,
    pHunger,
    pThirst,
    pMoney,
    pBank,
    pLevel,
    pExperience,
    pJobID,
    pFactionID,
    pFactionRank,
    pState,
    pIsLogged,
    pIsDirty,
    pIsAFK,
    pAFKTimer,
    pLastActivityTick,
    pLoginAttempts,
    pSessionStartTime,
    Float:pPosX,
    Float:pPosY,
    Float:pPosZ,
    Float:pPosA,
    pInterior,
    pWorld,
    pConnectTime,
    pTotalPlaytime
}

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];
static g_PlayerIP[MAX_PLAYERS][16];

#define SPAWN_DEFAULT_X   1545.5
#define SPAWN_DEFAULT_Y   -1675.7
#define SPAWN_DEFAULT_Z   13.5
#define SPAWN_DEFAULT_A   90.0

stock Player_Init(playerid) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pAccountID]        = INVALID_DB_ID;
    PlayerData[playerid][pCharacterID]      = INVALID_DB_ID;
    PlayerData[playerid][pAdminLevel]       = ADMIN_NONE;
    PlayerData[playerid][pAdminName][0]     = '\0';
    PlayerData[playerid][pName][0]          = '\0';
    PlayerData[playerid][pSex]              = 0;
    PlayerData[playerid][pAge]              = 18;
    PlayerData[playerid][pSkin]             = 0;
    PlayerData[playerid][pHealth]           = 100.0;
    PlayerData[playerid][pArmour]           = 0.0;
    PlayerData[playerid][pHunger]           = 100;
    PlayerData[playerid][pThirst]           = 100;
    PlayerData[playerid][pMoney]            = 0;
    PlayerData[playerid][pBank]             = 0;
    PlayerData[playerid][pLevel]            = 1;
    PlayerData[playerid][pExperience]       = 0;
    PlayerData[playerid][pJobID]            = INVALID_JOB_ID;
    PlayerData[playerid][pFactionID]        = INVALID_FACTION_ID;
    PlayerData[playerid][pFactionRank]      = 0;
    PlayerData[playerid][pState]            = STATE_DISCONNECTED;
    PlayerData[playerid][pIsLogged]         = false;
    PlayerData[playerid][pIsDirty]          = false;
    PlayerData[playerid][pIsAFK]            = false;
    PlayerData[playerid][pAFKTimer]         = -1;
    PlayerData[playerid][pLastActivityTick] = 0;
    PlayerData[playerid][pLoginAttempts]    = 0;
    PlayerData[playerid][pSessionStartTime] = 0;
    PlayerData[playerid][pPosX]             = SPAWN_DEFAULT_X;
    PlayerData[playerid][pPosY]             = SPAWN_DEFAULT_Y;
    PlayerData[playerid][pPosZ]             = SPAWN_DEFAULT_Z;
    PlayerData[playerid][pPosA]             = SPAWN_DEFAULT_A;
    PlayerData[playerid][pInterior]         = 0;
    PlayerData[playerid][pWorld]            = 0;
    PlayerData[playerid][pConnectTime]      = 0;
    PlayerData[playerid][pTotalPlaytime]    = 0;
    g_PlayerIP[playerid][0]                 = '\0';
    GetPlayerIp(playerid, g_PlayerIP[playerid], sizeof(g_PlayerIP[]));
    PlayerData[playerid][pConnectTime]      = gettime();
}

stock Player_GetMoney(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return PlayerData[playerid][pMoney];
}

stock Player_AddMoney(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return 0;
    new novo = PlayerData[playerid][pMoney] + amount;
    if(novo > MAX_MONEY) novo = MAX_MONEY;
    PlayerData[playerid][pMoney] = novo;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, novo);
    Player_MarkDirty(playerid);
    return 1;
}

stock Player_RemoveMoney(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return 0;
    if(PlayerData[playerid][pMoney] < amount) return 0;
    PlayerData[playerid][pMoney] -= amount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, PlayerData[playerid][pMoney]);
    Player_MarkDirty(playerid);
    return 1;
}

stock Player_SetMoney(playerid, amount) {
    if(!IsValidPlayer(playerid)) return;
    if(amount < 0) amount = 0;
    if(amount > MAX_MONEY) amount = MAX_MONEY;
    PlayerData[playerid][pMoney] = amount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, amount);
}

stock Player_GetBank(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return PlayerData[playerid][pBank];
}

stock Player_AddBank(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return 0;
    new novo = PlayerData[playerid][pBank] + amount;
    if(novo > MAX_BANK_BALANCE) return 0;
    PlayerData[playerid][pBank] = novo;
    Player_MarkDirty(playerid);
    return 1;
}

stock Player_RemoveBank(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return 0;
    if(PlayerData[playerid][pBank] < amount) return 0;
    PlayerData[playerid][pBank] -= amount;
    Player_MarkDirty(playerid);
    return 1;
}

stock Player_SetState(playerid, state) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pState] = state;
    SetPVarInt(playerid, "pState", state);
}

stock Player_GetState(playerid) {
    if(!IsValidPlayer(playerid)) return STATE_DISCONNECTED;
    return PlayerData[playerid][pState];
}

stock Player_MarkDirty(playerid) {
    if(IsValidPlayer(playerid)) PlayerData[playerid][pIsDirty] = true;
}

stock Player_ClearDirty(playerid) {
    if(IsValidPlayer(playerid)) PlayerData[playerid][pIsDirty] = false;
}

stock Player_IsDirty(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return _:PlayerData[playerid][pIsDirty];
}

stock Player_GetAdminLevel(playerid) {
    if(!IsValidPlayer(playerid)) return ADMIN_NONE;
    return PlayerData[playerid][pAdminLevel];
}

stock Player_IsAdmin(playerid, minLevel) {
    if(!IsValidPlayer(playerid)) return 0;
    return (PlayerData[playerid][pAdminLevel] >= minLevel);
}

stock Player_HasPermission(playerid, permission[]) {
    if(!IsValidPlayer(playerid)) return 0;
    new level = PlayerData[playerid][pAdminLevel];
    if(level >= ADMIN_OWNER) return 1;
    for(new i = 0; i < MAX_PERMISSION_ENTRIES; i++) {
        if(!strcmp(g_PermMap[i][pe_Key], permission, true)) {
            return (level >= g_PermMap[i][pe_Level]);
        }
    }
    return 0;
}

stock Player_GetIP(playerid, output[], size) {
    if(!IsValidPlayer(playerid)) return;
    strmid(output, g_PlayerIP[playerid], 0, size - 1, size);
}

stock Player_GetSessionTime(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return gettime() - PlayerData[playerid][pConnectTime];
}

stock Player_UpdateActivity(playerid) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pLastActivityTick] = GetTickCount();
    if(PlayerData[playerid][pIsAFK]) {
        PlayerData[playerid][pIsAFK] = false;
        Player_SetState(playerid, STATE_SPAWNED);
        SendInfo(playerid, "Voce voltou de AFK.");
    }
}

stock Player_CheckAFK(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!PlayerData[playerid][pIsLogged]) return;
    if(PlayerData[playerid][pState] == STATE_ADMIN_SPECTATING) return;
    if(PlayerData[playerid][pState] == STATE_DEAD) return;
    new elapsed = (GetTickCount() - PlayerData[playerid][pLastActivityTick]) / 1000;
    if(elapsed >= AFK_TIMEOUT && !PlayerData[playerid][pIsAFK]) {
        PlayerData[playerid][pIsAFK] = true;
        Player_SetState(playerid, STATE_AFK);
        SendWarning(playerid, "Voce esta em AFK.");
    }
}

stock Player_Save(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!PlayerData[playerid][pIsLogged]) return;
    if(PlayerData[playerid][pCharacterID] == INVALID_DB_ID) return;
    if(PlayerData[playerid][pState] >= STATE_SPAWNED) {
        GetPlayerPos(playerid,
            PlayerData[playerid][pPosX],
            PlayerData[playerid][pPosY],
            PlayerData[playerid][pPosZ]);
        GetPlayerFacingAngle(playerid, PlayerData[playerid][pPosA]);
        PlayerData[playerid][pInterior] = GetPlayerInterior(playerid);
        PlayerData[playerid][pWorld]    = GetPlayerVirtualWorld(playerid);
    }
    PlayerData[playerid][pTotalPlaytime] += Player_GetSessionTime(playerid);
    new query[512], esc[MAX_NAME_LENGTH * 2];
    DB_Escape(PlayerData[playerid][pName], esc, sizeof(esc));
    format(query, sizeof(query),
        "UPDATE `characters` SET `name`='%s',`health`=%.2f,`armour`=%.2f,"
        "`hunger`=%d,`thirst`=%d,`money`=%d,`bank`=%d,`level`=%d,"
        "`experience`=%d,`job_id`=%d,`faction_id`=%d,`faction_rank`=%d,"
        "`pos_x`=%.4f,`pos_y`=%.4f,`pos_z`=%.4f,`pos_a`=%.4f,"
        "`interior`=%d,`world`=%d,`total_playtime`=%d,`updated_at`=NOW() "
        "WHERE `id`=%d",
        esc,
        PlayerData[playerid][pHealth], PlayerData[playerid][pArmour],
        PlayerData[playerid][pHunger], PlayerData[playerid][pThirst],
        PlayerData[playerid][pMoney],  PlayerData[playerid][pBank],
        PlayerData[playerid][pLevel],  PlayerData[playerid][pExperience],
        PlayerData[playerid][pJobID],  PlayerData[playerid][pFactionID],
        PlayerData[playerid][pFactionRank],
        PlayerData[playerid][pPosX],   PlayerData[playerid][pPosY],
        PlayerData[playerid][pPosZ],   PlayerData[playerid][pPosA],
        PlayerData[playerid][pInterior], PlayerData[playerid][pWorld],
        PlayerData[playerid][pTotalPlaytime],
        PlayerData[playerid][pCharacterID]);
    DB_Execute(query);
    Player_ClearDirty(playerid);
}

forward OnScheduler5Minutes();
public OnScheduler5Minutes() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && Player_IsDirty(i))
            Player_Save(i);
    }
    return 1;
}

forward OnScheduler10Seconds();
public OnScheduler10Seconds() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pIsLogged])
            Player_CheckAFK(i);
    }
    return 1;
}
