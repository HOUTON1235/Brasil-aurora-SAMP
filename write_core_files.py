#!/usr/bin/env python3
# Escreve os arquivos core diretamente, sem BOM, ASCII puro
import os

DST = r'C:\Users\Felip\Downloads\Servidor 2\pawno\include'

def w(name, content):
    path = os.path.join(DST, name)
    # Garantir que nao tem BOM - abrir em modo binario
    data = content.encode('ascii', errors='replace')
    with open(path, 'wb') as f:
        f.write(data)
    # Verificar
    check = open(path, 'rb').read(3)
    if check == b'\xef\xbb\xbf':
        print('ERRO BOM em ' + name)
    else:
        print('OK: ' + name + ' (%d bytes)' % len(data))

# ============================================================
# states.inc
# ============================================================
w('states.inc', r"""#if defined _SA_STATES_INC
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
""")

# ============================================================
# permissions.inc
# ============================================================
w('permissions.inc', r"""#if defined _SA_PERMISSIONS_INC
    #endinput
#endif
#define _SA_PERMISSIONS_INC

#define ADMIN_NONE          0
#define ADMIN_HELPER        1
#define ADMIN_MODERATOR     2
#define ADMIN_ADMIN         3
#define ADMIN_SUPERVISOR    4
#define ADMIN_MANAGER       5
#define ADMIN_DIRECTOR      6
#define ADMIN_OWNER         7

stock GetAdminLevelName(level, output[], maxlen) {
    switch(level) {
        case ADMIN_HELPER:     format(output, maxlen, "Helper");
        case ADMIN_MODERATOR:  format(output, maxlen, "Moderador");
        case ADMIN_ADMIN:      format(output, maxlen, "Administrador");
        case ADMIN_SUPERVISOR: format(output, maxlen, "Supervisor");
        case ADMIN_MANAGER:    format(output, maxlen, "Gerente");
        case ADMIN_DIRECTOR:   format(output, maxlen, "Diretor");
        case ADMIN_OWNER:      format(output, maxlen, "Owner");
        default:               format(output, maxlen, "Jogador");
    }
}

#define PERM_ADMIN_PLAYER_BAN       "admin.player.ban"
#define PERM_ADMIN_PLAYER_KICK      "admin.player.kick"
#define PERM_ADMIN_PLAYER_MUTE      "admin.player.mute"
#define PERM_ADMIN_PLAYER_WARN      "admin.player.warn"
#define PERM_ADMIN_PLAYER_TELEPORT  "admin.player.teleport"
#define PERM_ADMIN_PLAYER_FREEZE    "admin.player.freeze"
#define PERM_ADMIN_PLAYER_SPECTATE  "admin.player.spectate"
#define PERM_ADMIN_PLAYER_JAIL      "admin.player.jail"
#define PERM_ADMIN_ECONOMY_GIVE     "admin.economy.give"
#define PERM_ADMIN_ECONOMY_REMOVE   "admin.economy.remove"
#define PERM_ADMIN_ECONOMY_EDIT     "admin.economy.edit"
#define PERM_ADMIN_ECONOMY_VIEW     "admin.economy.view"
#define PERM_ADMIN_VEHICLE_SPAWN    "admin.vehicle.spawn"
#define PERM_ADMIN_VEHICLE_DESTROY  "admin.vehicle.destroy"
#define PERM_ADMIN_VEHICLE_EDIT     "admin.vehicle.edit"
#define PERM_ADMIN_PROPERTY_EDIT    "admin.property.edit"
#define PERM_ADMIN_LOGS_VIEW        "admin.logs.view"
#define PERM_ADMIN_STATS_VIEW       "admin.stats.view"
#define PERM_ADMIN_SERVER_CONFIG    "admin.server.config"
#define PERM_POLICE_ARREST          "police.arrest"
#define PERM_POLICE_WEAPON          "police.weapon"
#define PERM_POLICE_HANDCUFF        "police.handcuff"
#define PERM_POLICE_SEARCH          "police.search"
#define PERM_FACTION_MEMBER_MANAGE  "faction.member.manage"
#define PERM_FACTION_RANK_MANAGE    "faction.rank.manage"
#define PERM_BUSINESS_MANAGE        "business.manage"

enum E_PERM_ENTRY {
    pe_Key[32],
    pe_Level
}

static g_PermMap[19][E_PERM_ENTRY] = {
    {"admin.player.ban",      ADMIN_MODERATOR},
    {"admin.player.kick",     ADMIN_HELPER},
    {"admin.player.mute",     ADMIN_HELPER},
    {"admin.player.warn",     ADMIN_HELPER},
    {"admin.player.teleport", ADMIN_HELPER},
    {"admin.player.freeze",   ADMIN_MODERATOR},
    {"admin.player.spectate", ADMIN_HELPER},
    {"admin.player.jail",     ADMIN_MODERATOR},
    {"admin.economy.give",    ADMIN_ADMIN},
    {"admin.economy.remove",  ADMIN_ADMIN},
    {"admin.economy.edit",    ADMIN_SUPERVISOR},
    {"admin.economy.view",    ADMIN_MODERATOR},
    {"admin.vehicle.spawn",   ADMIN_ADMIN},
    {"admin.vehicle.destroy", ADMIN_ADMIN},
    {"admin.vehicle.edit",    ADMIN_SUPERVISOR},
    {"admin.property.edit",   ADMIN_SUPERVISOR},
    {"admin.logs.view",       ADMIN_MODERATOR},
    {"admin.stats.view",      ADMIN_HELPER},
    {"admin.server.config",   ADMIN_OWNER}
};
#define MAX_PERMISSION_ENTRIES 19

forward Player_HasPermission(playerid, permission[]);
forward Player_IsAdmin(playerid, minLevel);
forward Player_GetAdminLevel(playerid);
""")

# ============================================================
# player_core.inc
# ============================================================
w('player_core.inc', r"""#if defined _SA_PLAYER_CORE_INC
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
    bool:pIsLogged,
    bool:pIsDirty,
    bool:pIsAFK,
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
""")

# ============================================================
# db_core.inc
# ============================================================
w('db_core.inc', r"""#if defined _SA_DB_CORE_INC
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
""")

# ============================================================
# db_queries.inc - vazio (tudo em db_core.inc)
# ============================================================
w('db_queries.inc', r"""#if defined _SA_DB_QUERIES_INC
    #endinput
#endif
#define _SA_DB_QUERIES_INC
// Funcoes movidas para db_core.inc
""")

print('\nPronto.')
