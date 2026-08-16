// ============================================================
//  Santa Aurora Roleplay ? Player Data (load/save/create)
//  Modulo : players/player_data.inc
//  Objetivo: carregar e criar personagens do banco de dados
// ============================================================

#if defined _SA_PLAYER_DATA_INC
    #endinput
#endif
#define _SA_PLAYER_DATA_INC

// include removido

// ------------------------------------------------------------
// Carregar personagem existente
// ------------------------------------------------------------
forward Player_LoadCharacter(playerid);
public Player_LoadCharacter(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    if(PlayerData[playerid][pAccountID] == INVALID_DB_ID) return 0;

    new query[256];
    format(query, sizeof(query),
        "SELECT c.*, cs.`total_playtime` "
        "FROM `characters` c "
        "LEFT JOIN `character_stats` cs ON cs.`character_id` = c.`id` "
        "WHERE c.`account_id` = %d AND c.`state` != 99 "
        "LIMIT 1",
        PlayerData[playerid][pAccountID]);

    DB_Query(query, "OnPlayerCharacterLoad", "i", playerid);
    return 1;
}

// ------------------------------------------------------------
// Callback: personagem carregado
// ------------------------------------------------------------
forward OnPlayerCharacterLoad(playerid);
public OnPlayerCharacterLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    if(!DB_HasResult()) {
        // Nenhum personagem encontrado ? iniciar criacao de personagem
        // Na Fase 1 o CharCreate cuida da criacao do personagem
        // Para compatibilidade, ainda chamamos Player_CreateCharacter aqui
        // e CharCreate_Start e chamado apos o load da conta
        CallLocalFunction("Player_CreateCharacter", "i", playerid);
        return 1;
    }

    // Preencher struct do jogador
    PlayerData[playerid][pCharacterID]  = DB_GetInt("id");
    DB_GetString("name", PlayerData[playerid][pName], sizeof(PlayerData[][pName]));
    PlayerData[playerid][pSex]          = DB_GetInt("sex");
    PlayerData[playerid][pAge]          = DB_GetInt("age");
    PlayerData[playerid][pSkin]         = DB_GetInt("skin_id");
    PlayerData[playerid][pHealth]       = DB_GetFloat("health");
    PlayerData[playerid][pArmour]       = DB_GetFloat("armour");
    PlayerData[playerid][pHunger]       = DB_GetInt("hunger");
    PlayerData[playerid][pThirst]       = DB_GetInt("thirst");
    PlayerData[playerid][pMoney]        = DB_GetInt("money");
    PlayerData[playerid][pBank]         = DB_GetInt("bank");
    PlayerData[playerid][pLevel]        = DB_GetInt("level");
    PlayerData[playerid][pExperience]   = DB_GetInt("experience");
    PlayerData[playerid][pJobID]        = DB_GetInt("job_id");
    PlayerData[playerid][pFactionID]    = DB_GetInt("faction_id");
    PlayerData[playerid][pFactionRank]  = DB_GetInt("faction_rank");
    PlayerData[playerid][pPosX]         = DB_GetFloat("pos_x");
    PlayerData[playerid][pPosY]         = DB_GetFloat("pos_y");
    PlayerData[playerid][pPosZ]         = DB_GetFloat("pos_z");
    PlayerData[playerid][pPosA]         = DB_GetFloat("pos_a");
    PlayerData[playerid][pInterior]     = DB_GetInt("interior");
    PlayerData[playerid][pWorld]        = DB_GetInt("world");
    PlayerData[playerid][pTotalPlaytime]= DB_GetInt("total_playtime");

    // Validar valores
    PlayerData[playerid][pHealth]  = ClampFloat(PlayerData[playerid][pHealth],  0.0, 100.0);
    PlayerData[playerid][pArmour]  = ClampFloat(PlayerData[playerid][pArmour],  0.0, 100.0);
    PlayerData[playerid][pHunger]  = Clamp(PlayerData[playerid][pHunger],  0, MAX_HUNGER);
    PlayerData[playerid][pThirst]  = Clamp(PlayerData[playerid][pThirst],  0, MAX_THIRST);
    PlayerData[playerid][pMoney]   = Clamp(PlayerData[playerid][pMoney],   MIN_MONEY, MAX_MONEY);
    PlayerData[playerid][pBank]    = Clamp(PlayerData[playerid][pBank],    0, MAX_BANK_BALANCE);
    PlayerData[playerid][pLevel]   = Clamp(PlayerData[playerid][pLevel],   1, MAX_LEVEL);

    PlayerData[playerid][pIsLogged]       = true;
    PlayerData[playerid][pIsDirty]        = false;
    PlayerData[playerid][pLastActivityTick] = GetTickCount();
    PlayerData[playerid][pSessionStartTime] = gettime();

    Player_SetState(playerid, STATE_SPAWNED);
    Player_SpawnAtSavedPosition(playerid);

    new logmsg[64];
    format(logmsg, sizeof(logmsg), "Personagem carregado: %s (CharID:%d)",
        PlayerData[playerid][pName], PlayerData[playerid][pCharacterID]);
    Logger_Info("PLAYER", logmsg);

    CallLocalFunction("OnPlayerLoaded", "i", playerid);
    return 1;
}

// ------------------------------------------------------------
// Spawnear na ultima posicao salva
// ------------------------------------------------------------
stock Player_SpawnAtSavedPosition(playerid) {
    if(!IsValidPlayer(playerid)) return;

    SetSpawnInfo(playerid, 0,
        PlayerData[playerid][pSkin],
        PlayerData[playerid][pPosX],
        PlayerData[playerid][pPosY],
        PlayerData[playerid][pPosZ],
        PlayerData[playerid][pPosA],
        0, 0, 0, 0, 0, 0);
    SpawnPlayer(playerid);

    SetPlayerInterior(playerid, PlayerData[playerid][pInterior]);
    SetPlayerVirtualWorld(playerid, PlayerData[playerid][pWorld]);
    SetPlayerHealth(playerid, PlayerData[playerid][pHealth]);
    SetPlayerArmour(playerid, PlayerData[playerid][pArmour]);
    Player_SetMoney(playerid, PlayerData[playerid][pMoney]);
}

// ------------------------------------------------------------
// Criar novo personagem (primeira vez ou sem personagem)
// ------------------------------------------------------------
forward Player_CreateCharacter(playerid);
public Player_CreateCharacter(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    if(PlayerData[playerid][pAccountID] == INVALID_DB_ID) return 0;

    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    new escaped_name[MAX_PLAYER_NAME * 2];
    DB_Escape(name, escaped_name, sizeof(escaped_name));

    // Criar com valores padrao
    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `characters` "
        "(`account_id`, `name`, `sex`, `age`, `skin_id`, "
        "`health`, `armour`, `hunger`, `thirst`, "
        "`money`, `bank`, `level`, `experience`, "
        "`pos_x`, `pos_y`, `pos_z`, `pos_a`, "
        "`interior`, `world`, `state`, `created_at`, `updated_at`) "
        "VALUES (%d, '%s', 0, 18, 0, "
        "100.0, 0.0, %d, %d, "
        "%d, %d, 1, 0, "
        "%.4f, %.4f, %.4f, %.4f, "
        "0, 0, 1, NOW(), NOW())",
        PlayerData[playerid][pAccountID],
        escaped_name,
        MAX_HUNGER,
        MAX_THIRST,
        STARTING_MONEY,
        STARTING_BANK,
        SPAWN_DEFAULT_X,
        SPAWN_DEFAULT_Y,
        SPAWN_DEFAULT_Z,
        SPAWN_DEFAULT_A
    );

    DB_Query(query, "OnPlayerCharacterCreate", "i", playerid);
    return 1;
}

// ------------------------------------------------------------
// Callback: personagem criado
// ------------------------------------------------------------
forward OnPlayerCharacterCreate(playerid);
public OnPlayerCharacterCreate(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    new charID = DB_InsertID();
    if(charID <= 0) {
        Logger_Error("PLAYER", "Falha ao criar personagem no banco.");
        SendError(playerid, "Erro ao criar personagem. Contate um administrador.");
        Kick(playerid);
        return 1;
    }

    PlayerData[playerid][pCharacterID]  = charID;
    PlayerData[playerid][pMoney]        = STARTING_MONEY;
    PlayerData[playerid][pBank]         = STARTING_BANK;
    PlayerData[playerid][pHunger]       = MAX_HUNGER;
    PlayerData[playerid][pThirst]       = MAX_THIRST;
    PlayerData[playerid][pLevel]        = 1;
    PlayerData[playerid][pExperience]   = 0;
    PlayerData[playerid][pSkin]         = 0;
    PlayerData[playerid][pPosX]         = SPAWN_DEFAULT_X;
    PlayerData[playerid][pPosY]         = SPAWN_DEFAULT_Y;
    PlayerData[playerid][pPosZ]         = SPAWN_DEFAULT_Z;
    PlayerData[playerid][pPosA]         = SPAWN_DEFAULT_A;

    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    SafeStrCopy(PlayerData[playerid][pName], name, sizeof(PlayerData[][pName]));

    // Criar registro de stats
    new stats_query[128];
    format(stats_query, sizeof(stats_query),
        "INSERT INTO `character_stats` (`character_id`, `total_playtime`, `created_at`) "
        "VALUES (%d, 0, NOW())",
        charID);
    DB_Execute(stats_query);

    PlayerData[playerid][pIsLogged]       = true;
    PlayerData[playerid][pLastActivityTick] = GetTickCount();
    PlayerData[playerid][pSessionStartTime] = gettime();

    Player_SetState(playerid, STATE_TUTORIAL);
    Player_SpawnAtSavedPosition(playerid);

    new logmsg[64];
    format(logmsg, sizeof(logmsg), "Novo personagem criado: %s (CharID:%d)", name, charID);
    Logger_Info("PLAYER", logmsg);

    SendSuccess(playerid,
        CSTR_SUCCESS"Personagem criado! "CSTR_WHITE"Bem-vindo ao "CSTR_SYSTEM"Santa Aurora RP"CSTR_WHITE"!");
    SendInfo(playerid, "Use /ajuda para ver os comandos disponiveis.");

    CallLocalFunction("OnPlayerLoaded", "i", playerid);
    CallLocalFunction("OnPlayerFinishedTutorial", "i", playerid); // por ora auto-completa
    return 1;
}

// ------------------------------------------------------------
// OnPlayerLoaded ? evento disparado quando personagem esta pronto
// ------------------------------------------------------------
public OnPlayerLoaded(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    // Modulos podem implementar este forward para reagir
    return 1;
}

