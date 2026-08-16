// ============================================================
//  Santa Aurora Roleplay — Territórios
//  Módulo : factions/territory_core.inc
//  Objetivo: domínio de territórios por facções, influência,
//            recompensas, disputas, prevenção de domínio permanente
// ============================================================

#if defined _SA_TERRITORY_CORE_INC
    #endinput
#endif
#define _SA_TERRITORY_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "../players/player_core.inc"
#include "faction_core.inc"

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_TERRITORIES         20
#define TERRITORY_CAPTURE_TIME  300     // 5 min de presença para capturar
#define TERRITORY_CONTEST_RANGE 50.0    // raio da zona de disputa
#define TERRITORY_DECAY_RATE    2       // influência perdida por ciclo (5min)
#define TERRITORY_REWARD_INTERVAL 3600  // recompensa a cada hora

// Dialogs de território
#define DIALOG_TERRITORY_INFO   1700
#define DIALOG_TERRITORY_LIST   1701

// ------------------------------------------------------------
// Cache de territórios
// ------------------------------------------------------------
enum E_TERRITORY {
    terr_ID,
    terr_Name[48],
    terr_Zone[32],
    terr_OwnerFaction,
    terr_Influence,             // 0-100
    Float:terr_PosX,
    Float:terr_PosY,
    Float:terr_Radius,
    terr_RewardType[12],
    terr_RewardValue,
    // Estado dinâmico
    terr_ContestingFaction,     // facção que está contestando
    terr_ContestProgress,       // 0-100 (progresso da disputa)
    terr_ContestStart           // timestamp
}

static g_Territories[MAX_TERRITORIES][E_TERRITORY];
static g_TerritoryCount = 0;
static bool:g_TerritoriesLoaded = false;

// Cooldown de disputa por facção (anti-griefing)
static g_LastContest[MAX_TERRITORIES] = {0, ...};
#define CONTEST_COOLDOWN 600    // 10 min entre disputas no mesmo território

// ------------------------------------------------------------
// Carregar territórios
// ------------------------------------------------------------
stock Territory_LoadAll() {
    DB_Query(
        "SELECT `id`, `name`, `zone`, `owner_faction`, `influence`, "
        "`pos_x`, `pos_y`, `radius`, `reward_type`, `reward_value` "
        "FROM `territories` ORDER BY `id`",
        "OnTerritoriesLoaded", "");
}

forward OnTerritoriesLoaded();
public OnTerritoriesLoaded() {
    g_TerritoryCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_TerritoryCount < MAX_TERRITORIES; r++) {
        new i = g_TerritoryCount;
        g_Territories[i][terr_ID]            = DB_GetInt("id",             r);
        DB_GetString("name", g_Territories[i][terr_Name], sizeof(g_Territories[][terr_Name]), r);
        DB_GetString("zone", g_Territories[i][terr_Zone], sizeof(g_Territories[][terr_Zone]), r);
        g_Territories[i][terr_OwnerFaction]  = DB_GetInt("owner_faction",  r);
        g_Territories[i][terr_Influence]     = DB_GetInt("influence",      r);
        g_Territories[i][terr_PosX]          = DB_GetFloat("pos_x",        r);
        g_Territories[i][terr_PosY]          = DB_GetFloat("pos_y",        r);
        g_Territories[i][terr_Radius]        = DB_GetFloat("radius",       r);
        DB_GetString("reward_type", g_Territories[i][terr_RewardType],
            sizeof(g_Territories[][terr_RewardType]), r);
        g_Territories[i][terr_RewardValue]   = DB_GetInt("reward_value",   r);
        g_Territories[i][terr_ContestingFaction] = 0;
        g_Territories[i][terr_ContestProgress]   = 0;
        g_Territories[i][terr_ContestStart]       = 0;
        g_TerritoryCount++;
    }
    g_TerritoriesLoaded = true;
    new msg[48];
    format(msg, sizeof(msg), "%d territórios carregados.", g_TerritoryCount);
    Logger_Info("TERRITORY", msg);
}

// Salvar território
stock Territory_Save(t_idx) {
    if(t_idx < 0) return;
    new query[256];
    format(query, sizeof(query),
        "UPDATE `territories` SET "
        "`owner_faction`=%d, `influence`=%d, `last_captured`=%s "
        "WHERE `id`=%d",
        g_Territories[t_idx][terr_OwnerFaction],
        g_Territories[t_idx][terr_Influence],
        g_Territories[t_idx][terr_OwnerFaction] > 0 ? "NOW()" : "NULL",
        g_Territories[t_idx][terr_ID]);
    DB_Execute(query);
}

// ------------------------------------------------------------
// Encontrar território na posição do jogador
// ------------------------------------------------------------
stock Territory_GetPlayerTerritory(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    for(new i = 0; i < g_TerritoryCount; i++) {
        new Float:dist = GetDistance3D(px, py, pz,
            g_Territories[i][terr_PosX],
            g_Territories[i][terr_PosY],
            pz);
        if(dist <= g_Territories[i][terr_Radius]) return i;
    }
    return -1;
}

// ------------------------------------------------------------
// INICIAR DISPUTA de território
// ------------------------------------------------------------
stock Territory_Contest(playerid, t_idx) {
    if(!IsValidPlayer(playerid) || t_idx < 0) return;

    new fac_id = PlayerData[playerid][pFactionID];
    if(fac_id == 0) {
        UI_NotifyError(playerid, "Você precisa estar em uma facção.");
        return;
    }

    // Já é dono?
    if(g_Territories[t_idx][terr_OwnerFaction] == fac_id) {
        UI_NotifyWarning(playerid, "Sua facção já controla este território.");
        return;
    }

    // Cooldown de disputa
    new now = gettime();
    if(now - g_LastContest[t_idx] < CONTEST_COOLDOWN) {
        new remaining = CONTEST_COOLDOWN - (now - g_LastContest[t_idx]);
        new msg[64];
        format(msg, sizeof(msg),
            "Aguarde %d segundo(s) para disputar este território.", remaining);
        UI_NotifyError(playerid, msg);
        return;
    }

    // Já em disputa por outra facção?
    if(g_Territories[t_idx][terr_ContestingFaction] != 0 &&
       g_Territories[t_idx][terr_ContestingFaction] != fac_id) {
        UI_NotifyError(playerid, "Este território já está sendo disputado por outra facção.");
        return;
    }

    // Iniciar/continuar disputa
    if(g_Territories[t_idx][terr_ContestingFaction] == 0) {
        g_Territories[t_idx][terr_ContestingFaction] = fac_id;
        g_Territories[t_idx][terr_ContestProgress]   = 0;
        g_Territories[t_idx][terr_ContestStart]       = now;

        // Anunciar disputa
        new fac_name[48];
        Faction_GetName(fac_id, fac_name, sizeof(fac_name));
        new terr_name[48];
        SafeStrCopy(terr_name, g_Territories[t_idx][terr_Name], sizeof(terr_name));
        new msg[128];
        format(msg, sizeof(msg),
            CSTR_ERROR"[TERRITÓRIO] "CSTR_WHITE"%s está disputando "
            CSTR_SYSTEM"%s"CSTR_WHITE"! (5 minutos)",
            fac_name, terr_name);
        SendClientMessageToAll(COLOR_RED, msg);

        g_LastContest[t_idx] = now;
        Logger_Info("TERRITORY", "Disputa iniciada");
    }

    // Contribuir com progresso
    g_Territories[t_idx][terr_ContestProgress] += 5;
    if(g_Territories[t_idx][terr_ContestProgress] >= 100) {
        Territory_Capture(t_idx, fac_id);
    } else {
        new msg[80];
        format(msg, sizeof(msg),
            CSTR_SYSTEM"[TERRITÓRIO] "CSTR_WHITE"Progresso: "
            CSTR_YELLOW"%d%%",
            g_Territories[t_idx][terr_ContestProgress]);
        // Enviar para membros da facção online
        for(new p = 0; p < MAX_PLAYERS; p++) {
            if(IsValidPlayer(p) && PlayerData[p][pFactionID] == fac_id) {
                SendClientMessage(p, COLOR_INFO, msg);
            }
        }
    }
}

// ------------------------------------------------------------
// CAPTURAR território
// ------------------------------------------------------------
stock Territory_Capture(t_idx, fac_id) {
    if(t_idx < 0) return;

    new old_owner = g_Territories[t_idx][terr_OwnerFaction];
    g_Territories[t_idx][terr_OwnerFaction]      = fac_id;
    g_Territories[t_idx][terr_Influence]          = 100;
    g_Territories[t_idx][terr_ContestingFaction]  = 0;
    g_Territories[t_idx][terr_ContestProgress]    = 0;
    Territory_Save(t_idx);

    // Anunciar captura
    new fac_name[48];
    Faction_GetName(fac_id, fac_name, sizeof(fac_name));
    new terr_name[48];
    SafeStrCopy(terr_name, g_Territories[t_idx][terr_Name], sizeof(terr_name));

    new msg[128];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[TERRITÓRIO] "CSTR_WHITE"%s capturou "
        CSTR_SYSTEM"%s"CSTR_WHITE"!",
        fac_name, terr_name);
    SendClientMessageToAll(COLOR_SYSTEM, msg);

    // Notificar dono anterior
    if(old_owner > 0 && old_owner != fac_id) {
        new old_name[48];
        Faction_GetName(old_owner, old_name, sizeof(old_name));
        new loss_msg[128];
        format(loss_msg, sizeof(loss_msg),
            CSTR_ERROR"[TERRITÓRIO] "CSTR_WHITE"%s perdeu o controle de "
            CSTR_SYSTEM"%s"CSTR_WHITE"!",
            old_name, terr_name);
        for(new p = 0; p < MAX_PLAYERS; p++) {
            if(IsValidPlayer(p) && PlayerData[p][pFactionID] == old_owner) {
                SendClientMessage(p, COLOR_RED, loss_msg);
            }
        }
    }

    Logger_Info("TERRITORY", "Território capturado");
}

// ------------------------------------------------------------
// RECOMPENSAS por territórios controlados (a cada hora)
// ------------------------------------------------------------
stock Territory_PayRewards() {
    for(new t = 0; t < g_TerritoryCount; t++) {
        new owner = g_Territories[t][terr_OwnerFaction];
        if(owner == 0) continue;
        if(g_Territories[t][terr_RewardValue] <= 0) continue;

        new reward_type[12];
        SafeStrCopy(reward_type, g_Territories[t][terr_RewardType], sizeof(reward_type));
        new reward = g_Territories[t][terr_RewardValue];

        if(strequal(reward_type, "cash", true)) {
            // Adicionar ao cofre da facção
            new f_idx = Faction_GetIdx(owner);
            if(f_idx >= 0) {
                g_Factions[f_idx][fac_Cash] += reward;
                Faction_Save(f_idx);

                // Notificar online
                new msg[80];
                Faction_GetName(owner, msg, sizeof(msg));
                new full_msg[128];
                new fmt[24]; FormatMoneyFull(reward, fmt, sizeof(fmt));
                format(full_msg, sizeof(full_msg),
                    CSTR_MONEY"[TERRITÓRIO] "CSTR_WHITE"%s: +"CSTR_MONEY"%s"
                    CSTR_WHITE" no cofre por %s",
                    msg, fmt, g_Territories[t][terr_Name]);
                for(new p = 0; p < MAX_PLAYERS; p++) {
                    if(IsValidPlayer(p) && PlayerData[p][pFactionID] == owner) {
                        SendClientMessage(p, COLOR_MONEY, full_msg);
                    }
                }
            }
        }
        else if(strequal(reward_type, "reputation", true)) {
            new f_idx = Faction_GetIdx(owner);
            if(f_idx >= 0) {
                g_Factions[f_idx][fac_Reputation] += reward;
                Faction_Save(f_idx);
            }
        }

        // Decaimento de influência para evitar domínio permanente
        g_Territories[t][terr_Influence] -= TERRITORY_DECAY_RATE;
        if(g_Territories[t][terr_Influence] <= 0) {
            // Perder o território
            g_Territories[t][terr_OwnerFaction] = 0;
            g_Territories[t][terr_Influence]     = 0;
            Territory_Save(t);
            new lost_msg[128];
            Faction_GetName(owner, lost_msg, sizeof(lost_msg));
            new full_lost[128];
            format(full_lost, sizeof(full_lost),
                CSTR_WARNING"[TERRITÓRIO] "CSTR_WHITE"%s perdeu o controle de "
                CSTR_SYSTEM"%s"CSTR_WHITE" por falta de presença.",
                lost_msg, g_Territories[t][terr_Name]);
            SendClientMessageToAll(COLOR_WARNING, full_lost);
        } else {
            Territory_Save(t);
        }
    }
}

// Scheduler
public OnScheduler1Minute() {
    static s_TerritoryTick = 0;
    s_TerritoryTick++;
    if(s_TerritoryTick >= 60) {
        s_TerritoryTick = 0;
        Territory_PayRewards();
    }
}

// Verificar membros em territórios a cada 30 segundos
public OnScheduler30Seconds() {
    for(new p = 0; p < MAX_PLAYERS; p++) {
        if(!IsValidPlayer(p)) continue;
        if(PlayerData[p][pFactionID] == 0) continue;
        if(Player_GetState(p) != STATE_SPAWNED && Player_GetState(p) != STATE_WORKING) continue;

        new t_idx = Territory_GetPlayerTerritory(p);
        if(t_idx < 0) continue;

        // Contribuir com influência se é dono
        if(g_Territories[t_idx][terr_OwnerFaction] == PlayerData[p][pFactionID]) {
            if(g_Territories[t_idx][terr_Influence] < 100) {
                g_Territories[t_idx][terr_Influence]++;
            }
        }
    }
}

// ------------------------------------------------------------
// Mostrar lista de territórios
// ------------------------------------------------------------
stock Territory_ShowList(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_TerritoriesLoaded) {
        UI_NotifyError(playerid, "Territórios não carregados.");
        return;
    }
    UI_ListReset();
    for(new i = 0; i < g_TerritoryCount; i++) {
        new owner_name[48];
        if(g_Territories[i][terr_OwnerFaction] > 0) {
            Faction_GetTag(g_Territories[i][terr_OwnerFaction],
                owner_name, sizeof(owner_name));
        } else {
            format(owner_name, sizeof(owner_name), "Neutro");
        }
        new line[80];
        format(line, sizeof(line), "%s  [%s]  Inf:%d%%",
            g_Territories[i][terr_Name],
            owner_name,
            g_Territories[i][terr_Influence]);
        UI_ListAdd(line);
    }
    UI_ListShow(playerid, DIALOG_TERRITORY_LIST,
        "{ Mapa de Territórios", "Ver Info", "Fechar");
}

// Info do território
stock Territory_ShowInfo(playerid, t_idx) {
    if(t_idx < 0 || t_idx >= g_TerritoryCount) return;

    new owner_name[48];
    if(g_Territories[t_idx][terr_OwnerFaction] > 0)
        Faction_GetName(g_Territories[t_idx][terr_OwnerFaction], owner_name, sizeof(owner_name));
    else
        format(owner_name, sizeof(owner_name), "Nenhuma (Neutro)");

    new contest_name[48] = "Nenhuma";
    if(g_Territories[t_idx][terr_ContestingFaction] > 0)
        Faction_GetName(g_Territories[t_idx][terr_ContestingFaction],
            contest_name, sizeof(contest_name));

    new reward_fmt[24];
    FormatMoneyFull(g_Territories[t_idx][terr_RewardValue], reward_fmt, sizeof(reward_fmt));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s ===\n\n"
        CSTR_WHITE"Zona: "CSTR_GREY"%s\n"
        CSTR_WHITE"Dono: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Influência: "CSTR_YELLOW"%d%%\n"
        CSTR_WHITE"Em disputa por: "CSTR_ERROR"%s\n"
        CSTR_WHITE"Progresso disputa: "CSTR_YELLOW"%d%%\n\n"
        CSTR_WHITE"Recompensa: "CSTR_MONEY"%s/%s/hora",
        g_Territories[t_idx][terr_Name],
        g_Territories[t_idx][terr_Zone],
        owner_name,
        g_Territories[t_idx][terr_Influence],
        contest_name,
        g_Territories[t_idx][terr_ContestProgress],
        reward_fmt,
        g_Territories[t_idx][terr_RewardType]);

    UI_ShowDialog(playerid, DIALOG_TERRITORY_INFO, DIALOG_STYLE_MSGBOX,
        "{ Informações do Território", body, "Fechar", "");
}

// Handler
stock bool:Territory_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_TERRITORY_LIST: {
            if(!response) return true;
            if(listitem >= g_TerritoryCount) return true;
            Territory_ShowInfo(playerid, listitem);
            return true;
        }
    }
    return false;
}

// Comandos
CMD:territorios(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Territory_ShowList(playerid);
    return 1;
}

CMD:disputar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new t_idx = Territory_GetPlayerTerritory(playerid);
    if(t_idx < 0) {
        SendError(playerid, "Você não está em nenhuma zona de território.");
        return 1;
    }
    Territory_Contest(playerid, t_idx);
    return 1;
}

CMD:territorio(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new t_idx = Territory_GetPlayerTerritory(playerid);
    if(t_idx < 0) {
        SendInfo(playerid, "Você não está em nenhum território. Use /territorios para ver o mapa.");
        return 1;
    }
    Territory_ShowInfo(playerid, t_idx);
    return 1;
}
