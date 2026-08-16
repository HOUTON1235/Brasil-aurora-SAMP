// ============================================================
//  Santa Aurora Roleplay — Missões e Conquistas
//  Módulo : inventory/missions_core.inc
//  (em inventory/ pois usa o item system)
//  Objetivo: missões diárias/semanais, progresso, recompensas,
//            conquistas com unlock automático
// ============================================================

#if defined _SA_MISSIONS_CORE_INC
    #endinput
#endif
#define _SA_MISSIONS_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "../players/player_core.inc"
#include "../economy/bank_core.inc"

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_MISSION_TYPES       30
#define MAX_ACTIVE_MISSIONS     5       // missões ativas por jogador
#define MAX_ACH_TYPES           25

// Dialogs
#define DIALOG_MISSIONS         1800
#define DIALOG_MISSION_INFO     1801
#define DIALOG_ACHIEVEMENTS     1802

// ------------------------------------------------------------
// Cache de definições de missão
// ------------------------------------------------------------
enum E_MISSION_DEF {
    mdef_ID,
    mdef_Name[48],
    mdef_Description[128],
    mdef_Type[8],
    mdef_Category[10],
    mdef_TargetValue,
    mdef_RewardMoney,
    mdef_RewardXP,
    mdef_RewardItem,
    mdef_RewardItemQty,
    mdef_MinLevel
}
static g_MissionDefs[MAX_MISSION_TYPES][E_MISSION_DEF];
static g_MissionDefCount = 0;

// Cache de missões ativas do jogador
enum E_PLAYER_MISSION {
    pm_ID,              // ID em character_missions
    pm_MissionID,
    pm_Progress,
    pm_Completed,
    pm_ExpiresAt
}
static g_PlayerMissions[MAX_PLAYERS][MAX_ACTIVE_MISSIONS][E_PLAYER_MISSION];
static g_PlayerMissionCount[MAX_PLAYERS] = {0, ...};

// ------------------------------------------------------------
// Cache de conquistas
// ------------------------------------------------------------
enum E_ACH_DEF {
    adef_ID,
    adef_Name[48],
    adef_Description[128],
    adef_RewardXP,
    adef_RewardMoney
}
static g_AchDefs[MAX_ACH_TYPES][E_ACH_DEF];
static g_AchDefCount = 0;

// Conquistas desbloqueadas pelo jogador (bitmap simplificado)
// Usa array de flags: g_PlayerAchs[playerid][ach_id / 8] & (1 << ach_id % 8)
static g_PlayerAchs[MAX_PLAYERS][4];    // 32 conquistas (4 bytes)
static bool:g_PlayerAchsLoaded[MAX_PLAYERS] = {false, ...};

// ------------------------------------------------------------
// Carregar definições de missão
// ------------------------------------------------------------
stock Missions_LoadDefs() {
    DB_Query(
        "SELECT `id`, `name`, `description`, `type`, `category`, `target_value`, "
        "`reward_money`, `reward_xp`, `reward_item`, `reward_item_qty`, `min_level` "
        "FROM `mission_definitions` WHERE `active`=1 ORDER BY `id`",
        "OnMissionDefsLoaded", "");
}

forward OnMissionDefsLoaded();
public OnMissionDefsLoaded() {
    g_MissionDefCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_MissionDefCount < MAX_MISSION_TYPES; r++) {
        new i = g_MissionDefCount;
        g_MissionDefs[i][mdef_ID] = DB_GetInt("id", r);
        DB_GetString("name",        g_MissionDefs[i][mdef_Name],        sizeof(g_MissionDefs[][mdef_Name]),        r);
        DB_GetString("description", g_MissionDefs[i][mdef_Description], sizeof(g_MissionDefs[][mdef_Description]), r);
        DB_GetString("type",        g_MissionDefs[i][mdef_Type],        sizeof(g_MissionDefs[][mdef_Type]),        r);
        DB_GetString("category",    g_MissionDefs[i][mdef_Category],    sizeof(g_MissionDefs[][mdef_Category]),    r);
        g_MissionDefs[i][mdef_TargetValue]  = DB_GetInt("target_value",    r);
        g_MissionDefs[i][mdef_RewardMoney]  = DB_GetInt("reward_money",    r);
        g_MissionDefs[i][mdef_RewardXP]     = DB_GetInt("reward_xp",       r);
        g_MissionDefs[i][mdef_RewardItem]   = DB_GetInt("reward_item",     r);
        g_MissionDefs[i][mdef_RewardItemQty]= DB_GetInt("reward_item_qty", r);
        g_MissionDefs[i][mdef_MinLevel]     = DB_GetInt("min_level",       r);
        g_MissionDefCount++;
    }

    // Carregar conquistas
    DB_Query(
        "SELECT `id`, `name`, `description`, `reward_xp`, `reward_money` "
        "FROM `achievement_definitions` ORDER BY `id`",
        "OnAchDefsLoaded", "");
    Logger_Info("MISSIONS", "Definições de missão carregadas.");
}

forward OnAchDefsLoaded();
public OnAchDefsLoaded() {
    g_AchDefCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_AchDefCount < MAX_ACH_TYPES; r++) {
        new i = g_AchDefCount;
        g_AchDefs[i][adef_ID] = DB_GetInt("id", r);
        DB_GetString("name",        g_AchDefs[i][adef_Name],        sizeof(g_AchDefs[][adef_Name]),        r);
        DB_GetString("description", g_AchDefs[i][adef_Description], sizeof(g_AchDefs[][adef_Description]), r);
        g_AchDefs[i][adef_RewardXP]    = DB_GetInt("reward_xp",    r);
        g_AchDefs[i][adef_RewardMoney] = DB_GetInt("reward_money",  r);
        g_AchDefCount++;
    }
    Logger_Info("MISSIONS", "Definições de conquistas carregadas.");
}

// ------------------------------------------------------------
// Carregar missões ativas do jogador
// ------------------------------------------------------------
stock Missions_LoadPlayer(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_PlayerMissionCount[playerid] = 0;
    new query[256];
    format(query, sizeof(query),
        "SELECT `id`, `mission_id`, `progress`, `completed`, "
        "UNIX_TIMESTAMP(`expires_at`) as expires "
        "FROM `character_missions` "
        "WHERE `character_id`=%d AND `completed`=0 "
        "AND (`expires_at` IS NULL OR `expires_at`>NOW()) "
        "ORDER BY `assigned_at` DESC LIMIT %d",
        PlayerData[playerid][pCharacterID], MAX_ACTIVE_MISSIONS);
    DB_Query(query, "OnPlayerMissionsLoad", "i", playerid);
}

forward OnPlayerMissionsLoad(playerid);
public OnPlayerMissionsLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    g_PlayerMissionCount[playerid] = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_PlayerMissionCount[playerid] < MAX_ACTIVE_MISSIONS; r++) {
        new i = g_PlayerMissionCount[playerid];
        g_PlayerMissions[playerid][i][pm_ID]        = DB_GetInt("id",         r);
        g_PlayerMissions[playerid][i][pm_MissionID] = DB_GetInt("mission_id", r);
        g_PlayerMissions[playerid][i][pm_Progress]  = DB_GetInt("progress",   r);
        g_PlayerMissions[playerid][i][pm_Completed] = DB_GetInt("completed",  r);
        g_PlayerMissions[playerid][i][pm_ExpiresAt] = DB_GetInt("expires",    r);
        g_PlayerMissionCount[playerid]++;
    }

    // Se não tem missões ativas, atribuir novas
    if(g_PlayerMissionCount[playerid] == 0) {
        Missions_AssignDaily(playerid);
    }

    // Carregar conquistas
    Missions_LoadAchievements(playerid);
    return 1;
}

// Carregar conquistas desbloqueadas
stock Missions_LoadAchievements(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new query[128];
    format(query, sizeof(query),
        "SELECT `achievement_id` FROM `character_achievements` "
        "WHERE `character_id`=%d",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnPlayerAchsLoad", "i", playerid);
}

forward OnPlayerAchsLoad(playerid);
public OnPlayerAchsLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    g_PlayerAchs[playerid][0] = 0;
    g_PlayerAchs[playerid][1] = 0;
    g_PlayerAchs[playerid][2] = 0;
    g_PlayerAchs[playerid][3] = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows; r++) {
        new ach_id = DB_GetInt("achievement_id", r);
        if(ach_id > 0 && ach_id <= 32) {
            new byte_idx = (ach_id - 1) / 8;
            new bit_pos  = (ach_id - 1) % 8;
            g_PlayerAchs[playerid][byte_idx] |= (1 << bit_pos);
        }
    }
    g_PlayerAchsLoaded[playerid] = true;
    return 1;
}

// ------------------------------------------------------------
// Atribuir missões diárias
// ------------------------------------------------------------
stock Missions_AssignDaily(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new level = PlayerData[playerid][pLevel];

    // Escolher 3 missões aleatórias adequadas ao nível
    new assigned = 0;
    new tries = 0;
    while(assigned < 3 && tries < 50) {
        tries++;
        new def_idx = random(g_MissionDefCount);
        if(g_MissionDefs[def_idx][mdef_MinLevel] > level) continue;
        if(!strequal(g_MissionDefs[def_idx][mdef_Type], "daily", true)) continue;

        // Verificar se já tem esta missão
        new bool:already_has = false;
        for(new m = 0; m < g_PlayerMissionCount[playerid]; m++) {
            if(g_PlayerMissions[playerid][m][pm_MissionID] ==
               g_MissionDefs[def_idx][mdef_ID]) {
                already_has = true;
                break;
            }
        }
        if(already_has) continue;

        // Inserir no banco
        new query[256];
        format(query, sizeof(query),
            "INSERT INTO `character_missions` "
            "(`character_id`, `mission_id`, `progress`, `expires_at`) "
            "VALUES (%d, %d, 0, DATE_ADD(NOW(), INTERVAL 1 DAY))",
            PlayerData[playerid][pCharacterID],
            g_MissionDefs[def_idx][mdef_ID]);
        DB_Execute(query);

        // Adicionar ao cache
        new mi = g_PlayerMissionCount[playerid];
        if(mi < MAX_ACTIVE_MISSIONS) {
            g_PlayerMissions[playerid][mi][pm_ID]        = 0;
            g_PlayerMissions[playerid][mi][pm_MissionID] = g_MissionDefs[def_idx][mdef_ID];
            g_PlayerMissions[playerid][mi][pm_Progress]  = 0;
            g_PlayerMissions[playerid][mi][pm_Completed] = 0;
            g_PlayerMissionCount[playerid]++;
        }
        assigned++;
    }

    if(assigned > 0) {
        new msg[64];
        format(msg, sizeof(msg),
            CSTR_SYSTEM"[MISSÕES] "CSTR_WHITE"%d missões diárias atribuídas! Use /missoes.",
            assigned);
        SendClientMessage(playerid, COLOR_INFO, msg);
    }
}

// ------------------------------------------------------------
// Atualizar progresso de missão por categoria
// ------------------------------------------------------------
stock Missions_UpdateProgress(playerid, const category[], delta = 1) {
    if(!IsValidPlayer(playerid)) return;

    for(new m = 0; m < g_PlayerMissionCount[playerid]; m++) {
        if(g_PlayerMissions[playerid][m][pm_Completed]) continue;

        // Encontrar def
        new mission_id = g_PlayerMissions[playerid][m][pm_MissionID];
        new def_idx = -1;
        for(new d = 0; d < g_MissionDefCount; d++) {
            if(g_MissionDefs[d][mdef_ID] == mission_id) { def_idx = d; break; }
        }
        if(def_idx < 0) continue;
        if(!strequal(g_MissionDefs[def_idx][mdef_Category], category, true)) continue;

        g_PlayerMissions[playerid][m][pm_Progress] += delta;

        // Verificar conclusão
        if(g_PlayerMissions[playerid][m][pm_Progress] >=
           g_MissionDefs[def_idx][mdef_TargetValue]) {
            Missions_Complete(playerid, m, def_idx);
        } else {
            // Salvar progresso
            new query[128];
            format(query, sizeof(query),
                "UPDATE `character_missions` SET `progress`=%d WHERE `id`=%d",
                g_PlayerMissions[playerid][m][pm_Progress],
                g_PlayerMissions[playerid][m][pm_ID]);
            DB_Execute(query);
        }
    }

    // Verificar conquistas
    Missions_CheckAchievements(playerid);
}

// ------------------------------------------------------------
// Completar missão e dar recompensas
// ------------------------------------------------------------
stock Missions_Complete(playerid, mission_slot, def_idx) {
    if(!IsValidPlayer(playerid)) return;

    g_PlayerMissions[playerid][mission_slot][pm_Completed] = 1;

    // Atualizar banco
    new query[128];
    format(query, sizeof(query),
        "UPDATE `character_missions` SET `completed`=1, `completed_at`=NOW() "
        "WHERE `id`=%d",
        g_PlayerMissions[playerid][mission_slot][pm_ID]);
    DB_Execute(query);

    // Recompensas
    new reward_money = g_MissionDefs[def_idx][mdef_RewardMoney];
    new reward_xp    = g_MissionDefs[def_idx][mdef_RewardXP];

    if(reward_money > 0) {
        Bank_Credit(playerid, reward_money, "mission", "Recompensa de missão");
    }
    if(reward_xp > 0) {
        PlayerData[playerid][pExperience] += reward_xp;
        Player_MarkDirty(playerid);
    }

    // Notificação
    new money_fmt[24];
    FormatMoneyFull(reward_money, money_fmt, sizeof(money_fmt));
    new msg[192];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"╔══════════════════════════╗\n"
        "║   MISSÃO CONCLUÍDA!      ║\n"
        CSTR_WHITE"%s\n"
        CSTR_MONEY"+"CSTR_WHITE"%s  "CSTR_SYSTEM"+"CSTR_WHITE"%d XP\n"
        CSTR_SUCCESS"╚══════════════════════════╝",
        g_MissionDefs[def_idx][mdef_Name],
        money_fmt, reward_xp);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    GameTextForPlayer(playerid, "~g~Missao Concluida!", 4000, 3);

    Logger_Economy(playerid, "MISSION_REWARD", reward_money,
        "Sistema", PlayerData[playerid][pName]);

    // Atualizar conquista de missões
    Missions_UpdateProgress(playerid, "missions_done", 1);
}

// ------------------------------------------------------------
// CONQUISTAS — verificar e desbloquear
// ------------------------------------------------------------
stock Missions_CheckAchievements(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_PlayerAchsLoaded[playerid]) return;

    // Verificar cada conquista
    for(new a = 0; a < g_AchDefCount; a++) {
        new ach_id = g_AchDefs[a][adef_ID];
        if(Missions_HasAchievement(playerid, ach_id)) continue; // já tem

        new bool:unlock = false;
        new ach_name[48];
        SafeStrCopy(ach_name, g_AchDefs[a][adef_Name], sizeof(ach_name));

        // Checar condição por nome da conquista
        if(strequal(ach_name, "ACH_FIRST_JOB", true))
            unlock = (PlayerData[playerid][pJobID] > 0);
        else if(strequal(ach_name, "ACH_FIRST_VEHICLE", true))
            unlock = (Vehicle_CountOwned(PlayerData[playerid][pCharacterID]) > 0);
        else if(strequal(ach_name, "ACH_TUTORIAL_DONE", true))
            unlock = Tutorial_IsDone(playerid);
        else if(strequal(ach_name, "ACH_LEVEL_5", true))
            unlock = (PlayerData[playerid][pLevel] >= 5);
        else if(strequal(ach_name, "ACH_LEVEL_20", true))
            unlock = (PlayerData[playerid][pLevel] >= 20);
        else if(strequal(ach_name, "ACH_VETERAN", true))
            unlock = (PlayerData[playerid][pLevel] >= 10);
        else if(strequal(ach_name, "ACH_FACTION_JOIN", true))
            unlock = (PlayerData[playerid][pFactionID] > 0);
        else if(strequal(ach_name, "ACH_RICH", true))
            unlock = (Bank_GetBalance(playerid) >= 100000);
        else if(strequal(ach_name, "ACH_BANK_ACCOUNT", true))
            unlock = Bank_IsLoaded(playerid);

        if(unlock) Missions_UnlockAchievement(playerid, a);
    }
}

// Verificar se tem conquista
stock bool:Missions_HasAchievement(playerid, ach_id) {
    if(!IsValidPlayer(playerid) || ach_id <= 0 || ach_id > 32) return false;
    new byte_idx = (ach_id - 1) / 8;
    new bit_pos  = (ach_id - 1) % 8;
    return bool:(g_PlayerAchs[playerid][byte_idx] & (1 << bit_pos));
}

// Desbloquear conquista
stock Missions_UnlockAchievement(playerid, def_idx) {
    if(!IsValidPlayer(playerid) || def_idx < 0) return;
    new ach_id = g_AchDefs[def_idx][adef_ID];
    if(Missions_HasAchievement(playerid, ach_id)) return;

    // Marcar como desbloqueada
    new byte_idx = (ach_id - 1) / 8;
    new bit_pos  = (ach_id - 1) % 8;
    g_PlayerAchs[playerid][byte_idx] |= (1 << bit_pos);

    // Salvar no banco
    new query[128];
    format(query, sizeof(query),
        "INSERT IGNORE INTO `character_achievements` "
        "(`character_id`, `achievement_id`) VALUES (%d, %d)",
        PlayerData[playerid][pCharacterID], ach_id);
    DB_Execute(query);

    // Dar recompensas
    new xp     = g_AchDefs[def_idx][adef_RewardXP];
    new money  = g_AchDefs[def_idx][adef_RewardMoney];
    if(xp > 0) {
        PlayerData[playerid][pExperience] += xp;
        Player_MarkDirty(playerid);
    }
    if(money > 0) {
        Bank_Credit(playerid, money, "achievement", "Recompensa de conquista");
    }

    // Notificação
    new money_fmt[24];
    FormatMoneyFull(money, money_fmt, sizeof(money_fmt));
    new msg[192];
    format(msg, sizeof(msg),
        CSTR_PURPLE"╔══════════════════════════╗\n"
        "║   CONQUISTA DESBLOQUEADA!║\n"
        CSTR_WHITE"%s\n"
        CSTR_GREY"%s\n"
        CSTR_SYSTEM"+"CSTR_WHITE"%d XP%s\n"
        CSTR_PURPLE"╚══════════════════════════╝",
        g_AchDefs[def_idx][adef_Name],
        g_AchDefs[def_idx][adef_Description],
        xp,
        money > 0 ? (" | "CSTR_MONEY) : "");
    SendClientMessage(playerid, COLOR_PURPLE, msg);
    GameTextForPlayer(playerid, "~p~Conquista Desbloqueada!", 4000, 3);
    Logger_Info("ACHIEVEMENT", "Conquista desbloqueada");
}

// Inicializar ao desconectar
public OnPlayerDisconnect(playerid, reason) {
    g_PlayerMissionCount[playerid]  = 0;
    g_PlayerAchsLoaded[playerid]    = false;
    g_PlayerAchs[playerid][0] = 0;
    g_PlayerAchs[playerid][1] = 0;
    g_PlayerAchs[playerid][2] = 0;
    g_PlayerAchs[playerid][3] = 0;
    return 1;
}

public OnPlayerLoaded(playerid) {
    Missions_LoadPlayer(playerid);
    return 1;
}

// Verificar conquistas a cada 5 minutos
public OnScheduler5Minutes() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {
            Missions_CheckAchievements(i);
        }
    }
}

// Verificar conquistas ao completar tutorial
public OnPlayerFinishedTutorial(playerid) {
    Missions_CheckAchievements(playerid);
    return 1;
}

// ------------------------------------------------------------
// Mostrar missões
// ------------------------------------------------------------
stock Missions_ShowList(playerid) {
    if(!IsValidPlayer(playerid)) return;

    if(g_PlayerMissionCount[playerid] == 0) {
        UI_NotifyInfo(playerid,
            "Sem missões ativas. As missões são renovadas diariamente.");
        return;
    }

    UI_ListReset();
    for(new m = 0; m < g_PlayerMissionCount[playerid]; m++) {
        if(g_PlayerMissions[playerid][m][pm_Completed]) continue;
        new mission_id = g_PlayerMissions[playerid][m][pm_MissionID];
        new def_idx = -1;
        for(new d = 0; d < g_MissionDefCount; d++) {
            if(g_MissionDefs[d][mdef_ID] == mission_id) { def_idx = d; break; }
        }
        if(def_idx < 0) continue;

        new progress   = g_PlayerMissions[playerid][m][pm_Progress];
        new target     = g_MissionDefs[def_idx][mdef_TargetValue];
        new pct        = (target > 0) ? (progress * 100 / target) : 0;
        new line[96];
        format(line, sizeof(line), "[%s] %s  %d/%d (%d%%)",
            g_MissionDefs[def_idx][mdef_Type],
            g_MissionDefs[def_idx][mdef_Name],
            progress, target, pct);
        UI_ListAdd(line);
    }
    UI_ListShow(playerid, DIALOG_MISSIONS, "{ Missões Ativas", "Ver Info", "Fechar");
}

// Mostrar conquistas
stock Missions_ShowAchievements(playerid) {
    if(!IsValidPlayer(playerid)) return;

    new unlocked = 0;
    for(new a = 0; a < g_AchDefCount; a++) {
        if(Missions_HasAchievement(playerid, g_AchDefs[a][adef_ID])) unlocked++;
    }

    UI_ListReset();
    for(new a = 0; a < g_AchDefCount; a++) {
        new ach_id = g_AchDefs[a][adef_ID];
        new has    = Missions_HasAchievement(playerid, ach_id);
        new line[96];
        format(line, sizeof(line), "%s%s — %s",
            has ? CSTR_SUCCESS"[✓] " : CSTR_GREY"[ ] ",
            g_AchDefs[a][adef_Name],
            g_AchDefs[a][adef_Description]);
        UI_ListAdd(line);
    }

    new title[64];
    format(title, sizeof(title), "{ Conquistas [%d/%d]", unlocked, g_AchDefCount);
    UI_ListShow(playerid, DIALOG_ACHIEVEMENTS, title, "Fechar", "");
}

// Handler de dialog
stock bool:Missions_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_MISSIONS: {
            if(!response) return true;
            // Mostrar info detalhada da missão selecionada
            new count = 0;
            for(new m = 0; m < g_PlayerMissionCount[playerid]; m++) {
                if(g_PlayerMissions[playerid][m][pm_Completed]) continue;
                if(count == listitem) {
                    new mission_id = g_PlayerMissions[playerid][m][pm_MissionID];
                    new def_idx = -1;
                    for(new d = 0; d < g_MissionDefCount; d++) {
                        if(g_MissionDefs[d][mdef_ID] == mission_id) { def_idx = d; break; }
                    }
                    if(def_idx < 0) return true;
                    new progress = g_PlayerMissions[playerid][m][pm_Progress];
                    new target   = g_MissionDefs[def_idx][mdef_TargetValue];
                    new money_fmt[24];
                    FormatMoneyFull(g_MissionDefs[def_idx][mdef_RewardMoney],
                        money_fmt, sizeof(money_fmt));
                    new body[512];
                    format(body, sizeof(body),
                        CSTR_SYSTEM"=== %s ===\n\n"
                        CSTR_WHITE"Descrição: "CSTR_GREY"%s\n\n"
                        CSTR_WHITE"Progresso: "CSTR_YELLOW"%d/%d\n"
                        CSTR_WHITE"Recompensa: "CSTR_MONEY"%s"
                        CSTR_WHITE" + "CSTR_SYSTEM"%d XP",
                        g_MissionDefs[def_idx][mdef_Name],
                        g_MissionDefs[def_idx][mdef_Description],
                        progress, target,
                        money_fmt,
                        g_MissionDefs[def_idx][mdef_RewardXP]);
                    UI_ShowDialog(playerid, DIALOG_MISSION_INFO, DIALOG_STYLE_MSGBOX,
                        "{ Missão", body, "Fechar", "");
                    return true;
                }
                count++;
            }
            return true;
        }
        case DIALOG_ACHIEVEMENTS:
        case DIALOG_MISSION_INFO: { return true; }
    }
    return false;
}

// Comandos
CMD:missoes(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Missions_ShowList(playerid);
    return 1;
}

CMD:conquistas(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Missions_ShowAchievements(playerid);
    return 1;
}

CMD:xp(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new xp_needed = XP_PER_LEVEL_BASE * PlayerData[playerid][pLevel];
    new msg[96];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[XP] "CSTR_WHITE"Nível: "CSTR_YELLOW"%d"
        CSTR_WHITE" | XP: "CSTR_SYSTEM"%d/%d",
        PlayerData[playerid][pLevel],
        PlayerData[playerid][pExperience],
        xp_needed);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}
