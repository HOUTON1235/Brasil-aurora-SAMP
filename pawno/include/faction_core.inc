// ============================================================
//  Santa Aurora Roleplay ? Sistema de Faccoes
//  Modulo : factions/faction_core.inc
//  Objetivo: faccoes legais e ilegais, cargos, membros,
//            cofre, radio, salario, territorios
// ============================================================

#if defined _SA_FACTION_CORE_INC
    #endinput
#endif
#define _SA_FACTION_CORE_INC

// include removido
// include removido
// include removido
// include removido
// include removido
// include removido
// include removido

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_FACTIONS            20
#define MAX_FACTION_RANKS       10
#define MAX_FACTION_MEMBERS_MEM 50   // por faccao em memoria
#define FACTION_SALARY_INTERVAL 3600
#define FACTION_RADIO_RANGE     9999.0   // radio e global dentro da faccao

// Dialogs
#define DIALOG_FACTION_MENU     1600
#define DIALOG_FACTION_MEMBERS  1601
#define DIALOG_FACTION_RECRUIT  1602
#define DIALOG_FACTION_PROMOTE  1603
#define DIALOG_FACTION_STORAGE  1604
#define DIALOG_FACTION_RADIO    1605
#define DIALOG_FACTION_KICK     1606

// ------------------------------------------------------------
// Cache de faccao
// ------------------------------------------------------------
enum E_FACTION {
    fac_ID,
    fac_Name[48],
    fac_Tag[8],
    fac_Type[10],           // legal/illegal/neutral
    fac_Color,
    fac_Cash,
    fac_Reputation,
    bool:fac_Loaded
}

enum E_FACTION_RANK {
    frank_Level,
    frank_Name[32],
    bool:frank_CanRecruit,
    bool:frank_CanManage,
    frank_Salary
}

enum E_FACTION_MEMBER_CACHE {
    fmc_CharID,
    fmc_Name[MAX_PLAYER_NAME],
    fmc_RankLevel,
    fmc_Contribution
}

static g_Factions[MAX_FACTIONS][E_FACTION];
static g_FactionCount = 0;

static g_FactionRanks[MAX_FACTIONS][MAX_FACTION_RANKS][E_FACTION_RANK];
static g_FactionRankCount[MAX_FACTIONS] = {0, ...};

static g_FactionMembers[MAX_FACTIONS][MAX_FACTION_MEMBERS_MEM][E_FACTION_MEMBER_CACHE];
static g_FactionMemberCount[MAX_FACTIONS] = {0, ...};

// Menu selecionado
static g_MenuFactionIdx[MAX_PLAYERS] = {-1, ...};

// ------------------------------------------------------------
// Helpers de busca
// ------------------------------------------------------------
stock Faction_GetIdx(faction_id) {
    for(new i = 0; i < g_FactionCount; i++) {
        if(g_Factions[i][fac_ID] == faction_id) return i;
    }
    return -1;
}

stock Faction_GetRankName(faction_idx, rank_level, output[], size = sizeof(output)) {
    for(new r = 0; r < g_FactionRankCount[faction_idx]; r++) {
        if(g_FactionRanks[faction_idx][r][frank_Level] == rank_level) {
            SafeStrCopy(output, g_FactionRanks[faction_idx][r][frank_Name], size);
            return;
        }
    }
    format(output, size, "Rank %d", rank_level);
}

stock Faction_GetRankSalary(faction_idx, rank_level) {
    for(new r = 0; r < g_FactionRankCount[faction_idx]; r++) {
        if(g_FactionRanks[faction_idx][r][frank_Level] == rank_level) {
            return g_FactionRanks[faction_idx][r][frank_Salary];
        }
    }
    return 500;
}

stock Faction_CanRecruit(faction_idx, rank_level) {
    for(new r = 0; r < g_FactionRankCount[faction_idx]; r++) {
        if(g_FactionRanks[faction_idx][r][frank_Level] == rank_level) {
            return g_FactionRanks[faction_idx][r][frank_CanRecruit];
        }
    }
    return false;
}

stock Faction_CanManage(faction_idx, rank_level) {
    for(new r = 0; r < g_FactionRankCount[faction_idx]; r++) {
        if(g_FactionRanks[faction_idx][r][frank_Level] == rank_level) {
            return g_FactionRanks[faction_idx][r][frank_CanManage];
        }
    }
    return false;
}

// ------------------------------------------------------------
// Carregar faccoes
// ------------------------------------------------------------
stock Faction_LoadAll() {
    DB_Query(
        "SELECT `id`, `name`, `tag`, `type`, `color`, `cash`, `reputation` "
        "FROM `factions` WHERE `active`=1 ORDER BY `id`",
        "OnFactionsLoaded", "");
}

forward OnFactionsLoaded();
public OnFactionsLoaded() {
    g_FactionCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_FactionCount < MAX_FACTIONS; r++) {
        new i = g_FactionCount;
        g_Factions[i][fac_ID]         = DB_GetInt("id",          r);
        DB_GetString("name", g_Factions[i][fac_Name], sizeof(g_Factions[][fac_Name]), r);
        DB_GetString("tag",  g_Factions[i][fac_Tag],  sizeof(g_Factions[][fac_Tag]),  r);
        DB_GetString("type", g_Factions[i][fac_Type], sizeof(g_Factions[][fac_Type]), r);
        g_Factions[i][fac_Color]      = DB_GetInt("color",      r);
        g_Factions[i][fac_Cash]       = DB_GetInt("cash",       r);
        g_Factions[i][fac_Reputation] = DB_GetInt("reputation", r);
        g_Factions[i][fac_Loaded]     = true;
        g_FactionRankCount[i]         = 0;
        g_FactionMemberCount[i]       = 0;
        g_FactionCount++;
    }
    // Carregar cargos
    DB_Query(
        "SELECT `faction_id`, `rank_level`, `name`, `can_recruit`, `can_manage`, `salary` "
        "FROM `faction_ranks` ORDER BY `faction_id`, `rank_level`",
        "OnFactionRanksLoaded", "");
    Logger_Info("FACTIONS", "Faccoes carregadas.");
}

forward OnFactionRanksLoaded();
public OnFactionRanksLoaded() {
    new rows = DB_NumRows();
    for(new r = 0; r < rows; r++) {
        new fac_id = DB_GetInt("faction_id", r);
        new f_idx  = Faction_GetIdx(fac_id);
        if(f_idx < 0) continue;
        new ri = g_FactionRankCount[f_idx];
        if(ri >= MAX_FACTION_RANKS) continue;
        g_FactionRanks[f_idx][ri][frank_Level]      = DB_GetInt("rank_level",   r);
        DB_GetString("name", g_FactionRanks[f_idx][ri][frank_Name],
            sizeof(g_FactionRanks[][][frank_Name]), r);
        g_FactionRanks[f_idx][ri][frank_CanRecruit] = bool:DB_GetInt("can_recruit", r);
        g_FactionRanks[f_idx][ri][frank_CanManage]  = bool:DB_GetInt("can_manage",  r);
        g_FactionRanks[f_idx][ri][frank_Salary]     = DB_GetInt("salary",       r);
        g_FactionRankCount[f_idx]++;
    }
    // Carregar membros
    DB_Query(
        "SELECT fm.`faction_id`, fm.`character_id`, fm.`rank_level`, "
        "fm.`contribution`, c.`name` as char_name "
        "FROM `faction_members` fm "
        "JOIN `characters` c ON c.`id`=fm.`character_id` "
        "WHERE fm.`active`=1",
        "OnFactionMembersLoaded", "");
}

forward OnFactionMembersLoaded();
public OnFactionMembersLoaded() {
    new rows = DB_NumRows();
    for(new r = 0; r < rows; r++) {
        new fac_id = DB_GetInt("faction_id", r);
        new f_idx  = Faction_GetIdx(fac_id);
        if(f_idx < 0) continue;
        new mi = g_FactionMemberCount[f_idx];
        if(mi >= MAX_FACTION_MEMBERS_MEM) continue;
        g_FactionMembers[f_idx][mi][fmc_CharID]    = DB_GetInt("character_id", r);
        DB_GetString("char_name", g_FactionMembers[f_idx][mi][fmc_Name],
            sizeof(g_FactionMembers[][][fmc_Name]), r);
        g_FactionMembers[f_idx][mi][fmc_RankLevel]    = DB_GetInt("rank_level",   r);
        g_FactionMembers[f_idx][mi][fmc_Contribution] = DB_GetInt("contribution", r);
        g_FactionMemberCount[f_idx]++;
    }
}

// Salvar faccao
stock Faction_Save(f_idx) {
    if(f_idx < 0) return;
    new query[128];
    format(query, sizeof(query),
        "UPDATE `factions` SET `cash`=%d, `reputation`=%d WHERE `id`=%d",
        g_Factions[f_idx][fac_Cash],
        g_Factions[f_idx][fac_Reputation],
        g_Factions[f_idx][fac_ID]);
    DB_Execute(query);
}

// ------------------------------------------------------------
// Verificar se jogador e membro
// ------------------------------------------------------------
stock Faction_IsMember(playerid, faction_id = -1) {
    if(!IsValidPlayer(playerid)) return false;
    if(faction_id < 0) return (PlayerData[playerid][pFactionID] > 0);
    return (PlayerData[playerid][pFactionID] == faction_id);
}

stock Faction_GetPlayerRank(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return PlayerData[playerid][pFactionRank];
}

stock Faction_GetPlayerFaction(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return PlayerData[playerid][pFactionID];
}

// ------------------------------------------------------------
// RECRUTAR
// ------------------------------------------------------------
stock Faction_Recruit(recruiterId, targetId) {
    if(!IsValidPlayer(recruiterId) || !IsValidPlayer(targetId)) return;

    new fac_id   = PlayerData[recruiterId][pFactionID];
    new rec_rank = PlayerData[recruiterId][pFactionRank];
    new f_idx    = Faction_GetIdx(fac_id);

    if(f_idx < 0 || fac_id == 0) {
        UI_NotifyError(recruiterId, "Voce nao esta em uma faccao.");
        return;
    }
    if(!Faction_CanRecruit(f_idx, rec_rank)) {
        UI_NotifyError(recruiterId, "Seu cargo nao permite recrutar.");
        return;
    }
    if(PlayerData[targetId][pFactionID] != 0) {
        UI_NotifyError(recruiterId, "Este jogador ja esta em uma faccao.");
        return;
    }

    // Inserir no banco
    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `faction_members` (`faction_id`, `character_id`, `rank_level`) "
        "VALUES (%d, %d, 1) ON DUPLICATE KEY UPDATE `active`=1, `rank_level`=1",
        fac_id, PlayerData[targetId][pCharacterID]);
    DB_Execute(query);

    // Atualizar dados do jogador
    PlayerData[targetId][pFactionID]   = fac_id;
    PlayerData[targetId][pFactionRank] = 1;
    Player_MarkDirty(targetId);

    // Atualizar character.faction_id
    new upd[128];
    format(upd, sizeof(upd),
        "UPDATE `characters` SET `faction_id`=%d, `faction_rank`=1 WHERE `id`=%d",
        fac_id, PlayerData[targetId][pCharacterID]);
    DB_Execute(upd);

    // Atualizar cache de membros
    new mi = g_FactionMemberCount[f_idx];
    if(mi < MAX_FACTION_MEMBERS_MEM) {
        g_FactionMembers[f_idx][mi][fmc_CharID]   = PlayerData[targetId][pCharacterID];
        GetPlayerName(targetId, g_FactionMembers[f_idx][mi][fmc_Name],
            sizeof(g_FactionMembers[][][fmc_Name]));
        g_FactionMembers[f_idx][mi][fmc_RankLevel] = 1;
        g_FactionMemberCount[f_idx]++;
    }

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetId, target_name, sizeof(target_name));
    new msg_r[80], msg_t[80];
    format(msg_r, sizeof(msg_r),
        CSTR_SUCCESS"%s foi recrutado para %s!", target_name, g_Factions[f_idx][fac_Name]);
    format(msg_t, sizeof(msg_t),
        CSTR_SUCCESS"Voce foi recrutado para "CSTR_SYSTEM"%s"CSTR_SUCCESS"!",
        g_Factions[f_idx][fac_Name]);
    SendClientMessage(recruiterId, COLOR_SUCCESS, msg_r);
    SendClientMessage(targetId,    COLOR_SUCCESS, msg_t);

    Logger_Info("FACTION", "Membro recrutado");
    CallLocalFunction("OnPlayerJoinedFaction", "iii", targetId, fac_id, 1);
}

// ------------------------------------------------------------
// EXPULSAR
// ------------------------------------------------------------
stock Faction_Kick(kickerId, target_char_id, f_idx) {
    if(!IsValidPlayer(kickerId)) return;
    new kick_rank = PlayerData[kickerId][pFactionRank];
    if(!Faction_CanManage(f_idx, kick_rank)) {
        UI_NotifyError(kickerId, "Seu cargo nao permite expulsar membros.");
        return;
    }

    new query[128];
    format(query, sizeof(query),
        "UPDATE `faction_members` SET `active`=0 "
        "WHERE `faction_id`=%d AND `character_id`=%d",
        g_Factions[f_idx][fac_ID], target_char_id);
    DB_Execute(query);

    new upd[128];
    format(upd, sizeof(upd),
        "UPDATE `characters` SET `faction_id`=0, `faction_rank`=0 WHERE `id`=%d",
        target_char_id);
    DB_Execute(upd);

    // Atualizar se online
    for(new p = 0; p < MAX_PLAYERS; p++) {
        if(!IsValidPlayer(p)) continue;
        if(PlayerData[p][pCharacterID] == target_char_id) {
            PlayerData[p][pFactionID]   = 0;
            PlayerData[p][pFactionRank] = 0;
            Player_MarkDirty(p);
            SendWarning(p, "Voce foi expulso da faccao.");
            CallLocalFunction("OnPlayerLeftFaction", "iii", p,
                g_Factions[f_idx][fac_ID], 0);
            break;
        }
    }
    SendSuccess(kickerId, "Membro expulso.");
    Logger_Info("FACTION", "Membro expulso");
}

// ------------------------------------------------------------
// PROMOVER / REBAIXAR
// ------------------------------------------------------------
stock Faction_SetRank(managerId, target_char_id, f_idx, new_rank) {
    if(!IsValidPlayer(managerId)) return;
    if(!Faction_CanManage(f_idx, PlayerData[managerId][pFactionRank])) {
        UI_NotifyError(managerId, "Seu cargo nao permite alterar ranks.");
        return;
    }
    if(new_rank < 1 || new_rank > g_FactionRankCount[f_idx]) {
        UI_NotifyError(managerId, "Rank invalido.");
        return;
    }

    new query[128];
    format(query, sizeof(query),
        "UPDATE `faction_members` SET `rank_level`=%d "
        "WHERE `faction_id`=%d AND `character_id`=%d AND `active`=1",
        new_rank, g_Factions[f_idx][fac_ID], target_char_id);
    DB_Execute(query);

    new upd[64];
    format(upd, sizeof(upd),
        "UPDATE `characters` SET `faction_rank`=%d WHERE `id`=%d",
        new_rank, target_char_id);
    DB_Execute(upd);

    // Online?
    for(new p = 0; p < MAX_PLAYERS; p++) {
        if(!IsValidPlayer(p)) continue;
        if(PlayerData[p][pCharacterID] == target_char_id) {
            PlayerData[p][pFactionRank] = new_rank;
            Player_MarkDirty(p);
            new rank_name[32];
            Faction_GetRankName(f_idx, new_rank, rank_name, sizeof(rank_name));
            new msg[64];
            format(msg, sizeof(msg),
                CSTR_SUCCESS"Seu cargo foi alterado para "CSTR_SYSTEM"%s"CSTR_SUCCESS"!",
                rank_name);
            SendClientMessage(p, COLOR_SUCCESS, msg);
            break;
        }
    }
    SendSuccess(managerId, "Rank alterado com sucesso.");
}

// SAIR DA FACCAO
stock Faction_Leave(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new fac_id = PlayerData[playerid][pFactionID];
    if(fac_id == 0) { UI_NotifyError(playerid, "Voce nao esta em nenhuma faccao."); return; }
    new f_idx = Faction_GetIdx(fac_id);

    new query[128];
    format(query, sizeof(query),
        "UPDATE `faction_members` SET `active`=0 "
        "WHERE `faction_id`=%d AND `character_id`=%d",
        fac_id, PlayerData[playerid][pCharacterID]);
    DB_Execute(query);

    new upd[64];
    format(upd, sizeof(upd),
        "UPDATE `characters` SET `faction_id`=0, `faction_rank`=0 WHERE `id`=%d",
        PlayerData[playerid][pCharacterID]);
    DB_Execute(upd);

    CallLocalFunction("OnPlayerLeftFaction", "iii", playerid, fac_id, 0);
    PlayerData[playerid][pFactionID]   = 0;
    PlayerData[playerid][pFactionRank] = 0;
    Player_MarkDirty(playerid);

    if(f_idx >= 0) {
        new msg[64];
        format(msg, sizeof(msg),
            CSTR_WARNING"Voce saiu de "CSTR_SYSTEM"%s"CSTR_WARNING".",
            g_Factions[f_idx][fac_Name]);
        SendClientMessage(playerid, COLOR_WARNING, msg);
    }
    Logger_Info("FACTION", "Jogador saiu da faccao");
}

// ------------------------------------------------------------
// RADIO DE FACCAO
// ------------------------------------------------------------
stock Faction_Radio(playerid, message[]) {
    if(!IsValidPlayer(playerid)) return;
    new fac_id = PlayerData[playerid][pFactionID];
    if(fac_id == 0) { UI_NotifyError(playerid, "Voce nao esta em uma faccao."); return; }

    new f_idx = Faction_GetIdx(fac_id);
    new sender_name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, sender_name, sizeof(sender_name));
    new rank_name[32];
    if(f_idx >= 0) Faction_GetRankName(f_idx, PlayerData[playerid][pFactionRank],
        rank_name, sizeof(rank_name));

    new msg[192];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[RADIO | %s] "CSTR_WHITE"%s (%s): %s",
        f_idx >= 0 ? g_Factions[f_idx][fac_Tag] : "FAC",
        sender_name, rank_name, message);

    // Enviar para todos os membros online
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(PlayerData[i][pFactionID] == fac_id) {
            SendClientMessage(i, COLOR_CYAN, msg);
        }
    }
    Logger_Chat(playerid, "RADIO", message);
}

// ------------------------------------------------------------
// SALARIOS DE FACCAO (a cada hora)
// ------------------------------------------------------------
stock Faction_PaySalaries() {
    for(new p = 0; p < MAX_PLAYERS; p++) {
        if(!IsValidPlayer(p)) continue;
        if(!PlayerData[p][pIsLogged]) continue;
        new fac_id = PlayerData[p][pFactionID];
        if(fac_id == 0) continue;

        new f_idx = Faction_GetIdx(fac_id);
        if(f_idx < 0) continue;

        new salary = Faction_GetRankSalary(f_idx, PlayerData[p][pFactionRank]);
        if(salary <= 0) continue;

        // Cobrar do cofre da faccao
        if(g_Factions[f_idx][fac_Cash] >= salary) {
            g_Factions[f_idx][fac_Cash] -= salary;
            Bank_Credit(p, salary, "salary",
                g_Factions[f_idx][fac_Name]);
            Faction_Save(f_idx);
        }
    }
}

// Scheduler
public OnScheduler1Minute() {
    static s_FacSalaryTick = 0;
    s_FacSalaryTick++;
    if(s_FacSalaryTick >= 60) {
        s_FacSalaryTick = 0;
        Faction_PaySalaries();
    }
}

public OnScheduler5Minutes() {
    for(new i = 0; i < g_FactionCount; i++) {
        Faction_Save(i);
    }
}

// ------------------------------------------------------------
// MENU DA FACCAO
// ------------------------------------------------------------
stock Faction_ShowMenu(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new fac_id = PlayerData[playerid][pFactionID];
    if(fac_id == 0) {
        UI_NotifyError(playerid, "Voce nao esta em uma faccao.");
        return;
    }
    new f_idx = Faction_GetIdx(fac_id);
    if(f_idx < 0) return;

    g_MenuFactionIdx[playerid] = f_idx;

    new rank_name[32];
    Faction_GetRankName(f_idx, PlayerData[playerid][pFactionRank],
        rank_name, sizeof(rank_name));
    new cash_fmt[24];
    FormatMoneyFull(g_Factions[f_idx][fac_Cash], cash_fmt, sizeof(cash_fmt));

    new body[256];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s [%s] ===\n\n"
        CSTR_WHITE"Tipo: "CSTR_GREY"%s\n"
        CSTR_WHITE"Membros: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Seu cargo: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Cofre: "CSTR_MONEY"%s\n"
        CSTR_WHITE"Reputacao: "CSTR_SYSTEM"%d",
        g_Factions[f_idx][fac_Name],
        g_Factions[f_idx][fac_Tag],
        g_Factions[f_idx][fac_Type],
        g_FactionMemberCount[f_idx],
        rank_name,
        cash_fmt,
        g_Factions[f_idx][fac_Reputation]);

    new options[256] = "Ver Membros\nSair da Faccao\n";
    new can_manage = Faction_CanManage(f_idx, PlayerData[playerid][pFactionRank]);
    new can_recruit= Faction_CanRecruit(f_idx, PlayerData[playerid][pFactionRank]);
    if(can_recruit) strcat(options, "Recrutar Jogador\n",   sizeof(options));
    if(can_manage)  strcat(options, "Gerenciar Membros\n",  sizeof(options));
    if(can_manage)  strcat(options, "Depositar no Cofre\n", sizeof(options));

    new full[512];
    format(full, sizeof(full), "%s\n%s", body, options);
    UI_ShowDialog(playerid, DIALOG_FACTION_MENU, DIALOG_STYLE_LIST,
        "{ Faccao", options, "Selecionar", "Fechar");
}

// Mostrar membros
static Faction_ShowMembers(playerid, f_idx) {
    UI_ListReset();
    if(g_FactionMemberCount[f_idx] == 0) {
        UI_ListAdd("Nenhum membro.");
    } else {
        for(new m = 0; m < g_FactionMemberCount[f_idx]; m++) {
            new rank_name[32];
            Faction_GetRankName(f_idx,
                g_FactionMembers[f_idx][m][fmc_RankLevel],
                rank_name, sizeof(rank_name));
            new line[80];
            format(line, sizeof(line), "%s  [%s]",
                g_FactionMembers[f_idx][m][fmc_Name],
                rank_name);
            UI_ListAdd(line);
        }
    }
    UI_ListShow(playerid, DIALOG_FACTION_MEMBERS,
        "{ Membros", "Expulsar", "Voltar");
}

// Handler de dialog
stock Faction_HandleDialog(playerid, dialogid, response, listitem,
                                  inputtext[]) {
    switch(dialogid) {
        case DIALOG_FACTION_MENU: {
            if(!response) return true;
            new f_idx = g_MenuFactionIdx[playerid];
            if(f_idx < 0) return true;
            new can_recruit = Faction_CanRecruit(f_idx, PlayerData[playerid][pFactionRank]);
            new can_manage  = Faction_CanManage(f_idx,  PlayerData[playerid][pFactionRank]);
            switch(listitem) {
                case 0: Faction_ShowMembers(playerid, f_idx);
                case 1: {
                    UI_ShowConfirm(playerid, "{ Sair da Faccao",
                        "Tem certeza que deseja sair da faccao?");
                    SetPVarInt(playerid, "confirm_faction_leave", 1);
                }
                default: {
                    new cur = 2;
                    if(can_recruit && listitem == cur) {
                        UI_ShowDialog(playerid, DIALOG_FACTION_RECRUIT, DIALOG_STYLE_INPUT,
                            "{ Recrutar", "ID do jogador:", "Recrutar", "Cancelar");
                        return true;
                    }
                    cur += can_recruit ? 1 : 0;
                    if(can_manage && listitem == cur) {
                        Faction_ShowMembers(playerid, f_idx);
                        return true;
                    }
                }
            }
            return true;
        }
        case DIALOG_FACTION_MEMBERS: {
            if(!response) { Faction_ShowMenu(playerid); return true; }
            new f_idx = g_MenuFactionIdx[playerid];
            if(f_idx < 0 || listitem >= g_FactionMemberCount[f_idx]) return true;
            Faction_Kick(playerid, g_FactionMembers[f_idx][listitem][fmc_CharID], f_idx);
            return true;
        }
        case DIALOG_FACTION_RECRUIT: {
            if(!response) return true;
            new targetid = strval(inputtext);
            if(!IsValidPlayer(targetid)) {
                UI_NotifyError(playerid, "Jogador invalido.");
                return true;
            }
            Faction_Recruit(playerid, targetid);
            return true;
        }
    }
    if(dialogid == DIALOG_CONFIRM_YES_NO &&
       GetPVarInt(playerid, "confirm_faction_leave")) {
        DeletePVar(playerid, "confirm_faction_leave");
        if(response) Faction_Leave(playerid);
        return true;
    }
    return false;
}

// Integracao
public OnPlayerLoaded(playerid) {
    // Faccao ja carregada via player_data.inc (pFactionID, pFactionRank)
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    g_MenuFactionIdx[playerid] = -1;
    return 1;
}

// Getters publicos
stock Faction_GetName(faction_id, output[], size = sizeof(output)) {
    new f_idx = Faction_GetIdx(faction_id);
    if(f_idx >= 0) SafeStrCopy(output, g_Factions[f_idx][fac_Name], size);
    else format(output, size, "Sem Faccao");
}

stock Faction_GetTag(faction_id, output[], size = sizeof(output)) {
    new f_idx = Faction_GetIdx(faction_id);
    if(f_idx >= 0) SafeStrCopy(output, g_Factions[f_idx][fac_Tag], size);
    else output[0] = '\0';
}

// Comandos
CMD:faccao(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Faction_ShowMenu(playerid);
    return 1;
}

CMD:radio(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(IsNullString(params)) {
        SendError(playerid, "Uso: /radio [mensagem]");
        return 1;
    }
    Faction_Radio(playerid, params);
    return 1;
}

CMD:r(playerid, params[]) { return cmd_radio(playerid, params); }

CMD:recrutar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /recrutar [id]");
        return 1;
    }
    Faction_Recruit(playerid, targetid);
    return 1;
}

CMD:expulsar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /expulsar [id]");
        return 1;
    }
    new fac_id = PlayerData[playerid][pFactionID];
    new f_idx  = Faction_GetIdx(fac_id);
    if(f_idx < 0) { SendError(playerid, "Voce nao esta em uma faccao."); return 1; }
    Faction_Kick(playerid, PlayerData[targetid][pCharacterID], f_idx);
    return 1;
}

CMD:promover(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /promover [id]");
        return 1;
    }
    new fac_id = PlayerData[playerid][pFactionID];
    new f_idx  = Faction_GetIdx(fac_id);
    if(f_idx < 0) return 1;
    new new_rank = PlayerData[targetid][pFactionRank] + 1;
    Faction_SetRank(playerid, PlayerData[targetid][pCharacterID], f_idx, new_rank);
    return 1;
}

CMD:rebaixar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /rebaixar [id]");
        return 1;
    }
    new fac_id = PlayerData[playerid][pFactionID];
    new f_idx  = Faction_GetIdx(fac_id);
    if(f_idx < 0) return 1;
    new new_rank = PlayerData[targetid][pFactionRank] - 1;
    if(new_rank < 1) { SendError(playerid, "Rank minimo ja atingido."); return 1; }
    Faction_SetRank(playerid, PlayerData[targetid][pCharacterID], f_idx, new_rank);
    return 1;
}

CMD:sairfaccao(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Faction_Leave(playerid);
    return 1;
}

CMD:membros(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new fac_id = PlayerData[playerid][pFactionID];
    if(fac_id == 0) { SendError(playerid, "Voce nao esta em uma faccao."); return 1; }
    new f_idx = Faction_GetIdx(fac_id);
    if(f_idx >= 0) Faction_ShowMembers(playerid, f_idx);
    return 1;
}

// Admin: definir faccao de jogador
CMD:asetfaccao(playerid, params[]) {
    if(!Player_IsAdmin(playerid, ADMIN_ADMIN)) {
        SendError(playerid, "Sem permissao.");
        return 1;
    }
    new targetid, faction_id;
    if(sscanf(params, "ii", targetid, faction_id) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /asetfaccao [id] [faction_id]");
        return 1;
    }
    new f_idx = Faction_GetIdx(faction_id);
    if(faction_id > 0 && f_idx < 0) {
        SendError(playerid, "Faccao nao encontrada.");
        return 1;
    }
    PlayerData[targetid][pFactionID]   = faction_id;
    PlayerData[targetid][pFactionRank] = (faction_id > 0) ? 1 : 0;
    Player_MarkDirty(targetid);
    new query[128];
    format(query, sizeof(query),
        "UPDATE `characters` SET `faction_id`=%d, `faction_rank`=%d WHERE `id`=%d",
        faction_id,
        (faction_id > 0) ? 1 : 0,
        PlayerData[targetid][pCharacterID]);
    DB_Execute(query);
    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new msg[64];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"%s associado a faccao ID %d.", target_name, faction_id);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    Logger_Admin(playerid, targetid, "SET_FACTION", "Faccao definida via admin");
    return 1;
}
