// ============================================================
//  Santa Aurora Roleplay — Propriedades
//  Módulo : houses/property_core.inc
//  Objetivo: compra, aluguel, entrar/sair, cofre, permissões
// ============================================================

#if defined _SA_PROPERTY_CORE_INC
    #endinput
#endif
#define _SA_PROPERTY_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../core/events.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "../players/player_core.inc"
#include "../economy/bank_core.inc"

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_PROPERTIES          100
#define MAX_PLAYER_PROPERTIES   2
#define PROPERTY_ENTER_RANGE    2.0
#define PROPERTY_SAFE_LIMIT     999999

// Dialogs
#define DIALOG_PROP_MENU        900
#define DIALOG_PROP_LIST        901
#define DIALOG_PROP_BUY         902
#define DIALOG_PROP_SAFE_DEP    903
#define DIALOG_PROP_SAFE_WIT    904
#define DIALOG_PROP_SELL        905
#define DIALOG_PROP_INVITE      906
#define DIALOG_PROP_RENT        907

// ------------------------------------------------------------
// Cache de propriedades em memória
// ------------------------------------------------------------
enum E_PROPERTY {
    prop_ID,
    prop_OwnerCharID,
    prop_Type[16],
    prop_Name[48],
    prop_Price,
    prop_RentPrice,
    prop_InteriorID,
    Float:prop_ExtX,
    Float:prop_ExtY,
    Float:prop_ExtZ,
    Float:prop_ExtA,
    Float:prop_IntX,
    Float:prop_IntY,
    Float:prop_IntZ,
    prop_IntWorld,
    prop_IntInterior,
    bool:prop_Locked,
    prop_GarageSlots,
    prop_SafeMoney,
    bool:prop_ForSale,
    bool:prop_ForRent,
    prop_Zone[32],
    bool:prop_Loaded
}

static g_Properties[MAX_PROPERTIES][E_PROPERTY];
static g_PropertyCount = 0;

// Jogador que está dentro de uma propriedade
static g_PlayerInsideProp[MAX_PLAYERS] = {-1, ...};

// ------------------------------------------------------------
// Carregar propriedades
// ------------------------------------------------------------
stock Property_LoadAll() {
    DB_Query(
        "SELECT `id`, `owner_char_id`, `type`, `name`, `price`, `rent_price`, "
        "`interior_id`, `ext_pos_x`, `ext_pos_y`, `ext_pos_z`, `ext_pos_a`, "
        "`int_pos_x`, `int_pos_y`, `int_pos_z`, `int_world`, `int_interior`, "
        "`locked`, `garage_slots`, `safe_money`, `for_sale`, `for_rent`, `zone` "
        "FROM `properties` ORDER BY `id` ASC LIMIT " #MAX_PROPERTIES,
        "OnPropertiesLoaded", "");
}

forward OnPropertiesLoaded();
public OnPropertiesLoaded() {
    g_PropertyCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_PropertyCount < MAX_PROPERTIES; r++) {
        new i = g_PropertyCount;
        g_Properties[i][prop_ID]          = DB_GetInt("id",            r);
        g_Properties[i][prop_OwnerCharID] = DB_GetInt("owner_char_id", r);
        DB_GetString("type", g_Properties[i][prop_Type], sizeof(g_Properties[][prop_Type]), r);
        DB_GetString("name", g_Properties[i][prop_Name], sizeof(g_Properties[][prop_Name]), r);
        g_Properties[i][prop_Price]        = DB_GetInt("price",        r);
        g_Properties[i][prop_RentPrice]    = DB_GetInt("rent_price",   r);
        g_Properties[i][prop_InteriorID]   = DB_GetInt("interior_id",  r);
        g_Properties[i][prop_ExtX]         = DB_GetFloat("ext_pos_x",  r);
        g_Properties[i][prop_ExtY]         = DB_GetFloat("ext_pos_y",  r);
        g_Properties[i][prop_ExtZ]         = DB_GetFloat("ext_pos_z",  r);
        g_Properties[i][prop_ExtA]         = DB_GetFloat("ext_pos_a",  r);
        g_Properties[i][prop_IntX]         = DB_GetFloat("int_pos_x",  r);
        g_Properties[i][prop_IntY]         = DB_GetFloat("int_pos_y",  r);
        g_Properties[i][prop_IntZ]         = DB_GetFloat("int_pos_z",  r);
        g_Properties[i][prop_IntWorld]     = DB_GetInt("int_world",    r);
        g_Properties[i][prop_IntInterior]  = DB_GetInt("int_interior", r);
        g_Properties[i][prop_Locked]       = bool:DB_GetInt("locked",  r);
        g_Properties[i][prop_GarageSlots]  = DB_GetInt("garage_slots", r);
        g_Properties[i][prop_SafeMoney]    = DB_GetInt("safe_money",   r);
        g_Properties[i][prop_ForSale]      = bool:DB_GetInt("for_sale",r);
        g_Properties[i][prop_ForRent]      = bool:DB_GetInt("for_rent",r);
        DB_GetString("zone", g_Properties[i][prop_Zone], sizeof(g_Properties[][prop_Zone]), r);
        g_Properties[i][prop_Loaded]       = true;
        g_PropertyCount++;
    }
    new msg[48];
    format(msg, sizeof(msg), "%d propriedades carregadas.", g_PropertyCount);
    Logger_Info("PROPERTY", msg);
}

// Salvar propriedade
stock Property_Save(idx) {
    if(idx < 0 || idx >= g_PropertyCount) return;
    new query[256];
    format(query, sizeof(query),
        "UPDATE `properties` SET "
        "`owner_char_id`=%d, `locked`=%d, `safe_money`=%d, "
        "`for_sale`=%d, `for_rent`=%d WHERE `id`=%d",
        g_Properties[idx][prop_OwnerCharID],
        g_Properties[idx][prop_Locked] ? 1 : 0,
        g_Properties[idx][prop_SafeMoney],
        g_Properties[idx][prop_ForSale] ? 1 : 0,
        g_Properties[idx][prop_ForRent] ? 1 : 0,
        g_Properties[idx][prop_ID]);
    DB_Execute(query);
}

// ------------------------------------------------------------
// Encontrar propriedade mais próxima da entrada
// ------------------------------------------------------------
stock Property_FindNearest(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new best = -1;
    new Float:best_dist = PROPERTY_ENTER_RANGE + 1.0;
    for(new i = 0; i < g_PropertyCount; i++) {
        new Float:dist = GetDistance3D(px, py, pz,
            g_Properties[i][prop_ExtX],
            g_Properties[i][prop_ExtY],
            g_Properties[i][prop_ExtZ]);
        if(dist < best_dist) { best_dist = dist; best = i; }
    }
    return best;
}

// Encontrar propriedade do dono
stock Property_FindByOwner(char_id, start_idx = 0) {
    for(new i = start_idx; i < g_PropertyCount; i++) {
        if(g_Properties[i][prop_OwnerCharID] == char_id) return i;
    }
    return -1;
}

// Verificar se jogador pode entrar
stock bool:Property_CanEnter(playerid, idx) {
    if(idx < 0) return false;
    // Dono sempre entra
    if(g_Properties[idx][prop_OwnerCharID] == PlayerData[playerid][pCharacterID]) return true;
    // Destravada: qualquer um entra
    if(!g_Properties[idx][prop_Locked]) return true;
    // Verificar permissão
    new query[128];
    format(query, sizeof(query),
        "SELECT 1 FROM `property_permissions` "
        "WHERE `property_id`=%d AND `character_id`=%d LIMIT 1",
        g_Properties[idx][prop_ID],
        PlayerData[playerid][pCharacterID]);
    // Sync query para simplificar (propriedades não são críticas de performance)
    new Cache:cache = mysql_query(DB_GetHandle(), query);
    new bool:has = (cache_num_rows() > 0);
    cache_delete(cache);
    return has;
}

// ------------------------------------------------------------
// Entrar na propriedade
// ------------------------------------------------------------
stock Property_Enter(playerid, idx) {
    if(!IsValidPlayer(playerid) || idx < 0) return;

    if(!Property_CanEnter(playerid, idx)) {
        UI_NotifyError(playerid, "Esta propriedade está trancada.");
        return;
    }

    SetPlayerPos(playerid,
        g_Properties[idx][prop_IntX],
        g_Properties[idx][prop_IntY],
        g_Properties[idx][prop_IntZ]);
    SetPlayerInterior(playerid, g_Properties[idx][prop_IntInterior]);
    SetPlayerVirtualWorld(playerid, g_Properties[idx][prop_IntWorld]);
    SetPlayerFacingAngle(playerid, 0.0);

    g_PlayerInsideProp[playerid] = idx;

    new msg[64];
    format(msg, sizeof(msg), CSTR_SYSTEM"[Casa] "CSTR_WHITE"Você entrou em: %s",
        g_Properties[idx][prop_Name]);
    SendClientMessage(playerid, COLOR_INFO, msg);
    SendInfo(playerid, "Use /sair para sair da propriedade.");
}

// ------------------------------------------------------------
// Sair da propriedade
// ------------------------------------------------------------
stock Property_Exit(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new idx = g_PlayerInsideProp[playerid];
    if(idx < 0) {
        UI_NotifyError(playerid, "Você não está em nenhuma propriedade.");
        return;
    }

    SetPlayerPos(playerid,
        g_Properties[idx][prop_ExtX],
        g_Properties[idx][prop_ExtY],
        g_Properties[idx][prop_ExtZ] + 0.5);
    SetPlayerFacingAngle(playerid, g_Properties[idx][prop_ExtA]);
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);

    g_PlayerInsideProp[playerid] = -1;
}

// ------------------------------------------------------------
// Trancar / Destrancar
// ------------------------------------------------------------
stock Property_ToggleLock(playerid, idx) {
    if(!IsValidPlayer(playerid) || idx < 0) return;
    if(g_Properties[idx][prop_OwnerCharID] != PlayerData[playerid][pCharacterID]) {
        UI_NotifyError(playerid, "Esta propriedade não é sua.");
        return;
    }
    g_Properties[idx][prop_Locked] = !g_Properties[idx][prop_Locked];
    Property_Save(idx);
    new msg[64];
    format(msg, sizeof(msg),
        g_Properties[idx][prop_Locked] ?
            CSTR_ERROR"Propriedade trancada." :
            CSTR_SUCCESS"Propriedade destrancada.");
    SendClientMessage(playerid, COLOR_INFO, msg);
}

// ------------------------------------------------------------
// COMPRAR
// ------------------------------------------------------------
stock Property_Buy(playerid, idx) {
    if(!IsValidPlayer(playerid) || idx < 0) return;

    if(!g_Properties[idx][prop_ForSale]) {
        UI_NotifyError(playerid, "Esta propriedade não está à venda.");
        return;
    }
    if(g_Properties[idx][prop_OwnerCharID] == PlayerData[playerid][pCharacterID]) {
        UI_NotifyError(playerid, "Você já é dono desta propriedade.");
        return;
    }
    if(Property_FindByOwner(PlayerData[playerid][pCharacterID]) >= 0) {
        // Verificar se já atingiu o limite
        new count = 0, tmp = -1;
        do {
            tmp = Property_FindByOwner(PlayerData[playerid][pCharacterID], tmp + 1);
            if(tmp >= 0) count++;
        } while(tmp >= 0 && count < MAX_PLAYER_PROPERTIES + 1);

        if(count >= MAX_PLAYER_PROPERTIES) {
            UI_NotifyError(playerid, "Você já possui o número máximo de propriedades.");
            return;
        }
    }

    new price = g_Properties[idx][prop_Price];
    new bool:paid = false;

    if(PlayerData[playerid][pMoney] >= price) {
        Player_RemoveMoney(playerid, price);
        paid = true;
    } else if(Bank_GetBalance(playerid) >= price) {
        Bank_Debit(playerid, price, "purchase", "Compra de imóvel");
        paid = true;
    }

    if(!paid) {
        UI_NotifyError(playerid, "Saldo insuficiente.");
        return;
    }

    // Transferir propriedade
    new prev_owner = g_Properties[idx][prop_OwnerCharID];
    g_Properties[idx][prop_OwnerCharID] = PlayerData[playerid][pCharacterID];
    g_Properties[idx][prop_ForSale]     = false;
    g_Properties[idx][prop_Locked]      = true;
    Property_Save(idx);

    new price_fmt[24];
    FormatMoneyFull(price, price_fmt, sizeof(price_fmt));
    new msg[128];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Você comprou "CSTR_SYSTEM"%s"CSTR_SUCCESS" por "CSTR_MONEY"%s"CSTR_SUCCESS"!",
        g_Properties[idx][prop_Name], price_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    GameTextForPlayer(playerid, "~g~Imovel Comprado!", 4000, 3);

    Logger_Economy(playerid, "PROPERTY_BUY", price,
        PlayerData[playerid][pName], g_Properties[idx][prop_Name]);
    CallLocalFunction("OnPropertyBought", "iii",
        g_Properties[idx][prop_ID], PlayerData[playerid][pCharacterID], price);

    // Log
    new log_detail[64];
    format(log_detail, sizeof(log_detail), "PropID=%d PrevOwner=%d",
        g_Properties[idx][prop_ID], prev_owner);
    Logger_Info("PROPERTY", log_detail);
}

// ------------------------------------------------------------
// VENDER (ao servidor)
// ------------------------------------------------------------
stock Property_SellToServer(playerid, idx) {
    if(!IsValidPlayer(playerid) || idx < 0) return;
    if(g_Properties[idx][prop_OwnerCharID] != PlayerData[playerid][pCharacterID]) {
        UI_NotifyError(playerid, "Esta propriedade não é sua.");
        return;
    }

    // Vender por 60% do valor
    new sell_price = g_Properties[idx][prop_Price] * 6 / 10;
    Bank_Credit(playerid, sell_price, "refund", "Venda de imóvel");

    g_Properties[idx][prop_OwnerCharID] = 0;
    g_Properties[idx][prop_ForSale]     = true;
    g_Properties[idx][prop_Locked]      = true;
    Property_Save(idx);

    new price_fmt[24];
    FormatMoneyFull(sell_price, price_fmt, sizeof(price_fmt));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Propriedade vendida. Recebeu "CSTR_MONEY"%s"CSTR_SUCCESS" no banco.",
        price_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    Logger_Economy(playerid, "PROPERTY_SELL", sell_price,
        g_Properties[idx][prop_Name], "Servidor");
}

// ------------------------------------------------------------
// COFRE (safe_money)
// ------------------------------------------------------------
stock Property_SafeDeposit(playerid, idx, amount) {
    if(!IsValidPlayer(playerid) || idx < 0) return;
    if(g_Properties[idx][prop_OwnerCharID] != PlayerData[playerid][pCharacterID]) {
        UI_NotifyError(playerid, "Apenas o dono pode acessar o cofre.");
        return;
    }
    if(amount <= 0 || amount > PlayerData[playerid][pMoney]) {
        UI_NotifyError(playerid, "Valor inválido.");
        return;
    }
    new new_safe = g_Properties[idx][prop_SafeMoney] + amount;
    if(new_safe > PROPERTY_SAFE_LIMIT) {
        UI_NotifyError(playerid, "O cofre está cheio.");
        return;
    }
    Player_RemoveMoney(playerid, amount);
    g_Properties[idx][prop_SafeMoney] = new_safe;
    Property_Save(idx);
    new fmt[24]; FormatMoneyFull(amount, fmt, sizeof(fmt));
    new msg[64]; format(msg, sizeof(msg), CSTR_SUCCESS"Depositou %s no cofre.", fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
}

stock Property_SafeWithdraw(playerid, idx, amount) {
    if(!IsValidPlayer(playerid) || idx < 0) return;
    if(g_Properties[idx][prop_OwnerCharID] != PlayerData[playerid][pCharacterID]) {
        UI_NotifyError(playerid, "Apenas o dono pode acessar o cofre.");
        return;
    }
    if(amount <= 0 || amount > g_Properties[idx][prop_SafeMoney]) {
        UI_NotifyError(playerid, "Valor inválido ou saldo insuficiente no cofre.");
        return;
    }
    g_Properties[idx][prop_SafeMoney] -= amount;
    Player_AddMoney(playerid, amount);
    Property_Save(idx);
    new fmt[24]; FormatMoneyFull(amount, fmt, sizeof(fmt));
    new msg[64]; format(msg, sizeof(msg), CSTR_SUCCESS"Sacou %s do cofre.", fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
}

// ------------------------------------------------------------
// MENU da propriedade
// ------------------------------------------------------------
stock Property_ShowMenu(playerid, idx) {
    if(!IsValidPlayer(playerid) || idx < 0) return;
    new is_owner = (g_Properties[idx][prop_OwnerCharID] == PlayerData[playerid][pCharacterID]);
    new price_fmt[24];
    FormatMoneyFull(g_Properties[idx][prop_Price], price_fmt, sizeof(price_fmt));
    new safe_fmt[24];
    FormatMoneyFull(g_Properties[idx][prop_SafeMoney], safe_fmt, sizeof(safe_fmt));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s ===\n\n"
        CSTR_WHITE"Tipo: "CSTR_GREY"%s\n"
        CSTR_WHITE"Zona: "CSTR_GREY"%s\n"
        CSTR_WHITE"Preço: "CSTR_MONEY"%s\n"
        CSTR_WHITE"Status: %s",
        g_Properties[idx][prop_Name],
        g_Properties[idx][prop_Type],
        g_Properties[idx][prop_Zone],
        price_fmt,
        g_Properties[idx][prop_ForSale] ? CSTR_SUCCESS"À Venda" : CSTR_ERROR"Vendida");

    if(is_owner) {
        format(body, sizeof(body), "%s\n"CSTR_WHITE"Cofre: "CSTR_MONEY"%s",
            body, safe_fmt);
    }

    new options[256];
    if(is_owner) {
        format(options, sizeof(options),
            "%s\nCofre — Depositar\nCofre — Sacar\nVender ao Servidor\nConvidar Jogador",
            g_Properties[idx][prop_Locked] ? "Destrancar" : "Trancar");
    } else if(g_Properties[idx][prop_ForSale]) {
        format(options, sizeof(options), "Comprar (%s)", price_fmt);
    } else {
        format(options, sizeof(options), "Fechar");
    }

    SetPVarInt(playerid, "menu_prop_idx", idx);
    UI_ShowDialog(playerid, DIALOG_PROP_MENU, DIALOG_STYLE_LIST,
        "{ Propriedade", options, "Selecionar", "Fechar");
}

// ------------------------------------------------------------
// LISTAR imóveis à venda
// ------------------------------------------------------------
stock Property_ShowForSale(playerid) {
    if(!IsValidPlayer(playerid)) return;
    UI_ListReset();
    new found = false;
    for(new i = 0; i < g_PropertyCount; i++) {
        if(!g_Properties[i][prop_ForSale]) continue;
        new price_fmt[24];
        FormatMoneyFull(g_Properties[i][prop_Price], price_fmt, sizeof(price_fmt));
        new line[80];
        format(line, sizeof(line), "[%s] %s — %s (%s)",
            g_Properties[i][prop_Type],
            g_Properties[i][prop_Name],
            price_fmt,
            g_Properties[i][prop_Zone]);
        UI_ListAdd(line);
        found = true;
    }
    if(!found) UI_ListAdd("Nenhum imóvel à venda no momento.");
    UI_ListShow(playerid, DIALOG_PROP_LIST, "{ Imóveis à Venda", "Ver / Comprar", "Fechar");
}

// Handler dialogs
stock bool:Property_HandleDialog(playerid, dialogid, response, listitem,
                                  const inputtext[]) {
    switch(dialogid) {
        case DIALOG_PROP_MENU: {
            if(!response) return true;
            new idx      = GetPVarInt(playerid, "menu_prop_idx");
            new is_owner = (g_Properties[idx][prop_OwnerCharID] ==
                            PlayerData[playerid][pCharacterID]);
            if(is_owner) {
                switch(listitem) {
                    case 0: Property_ToggleLock(playerid, idx);
                    case 1: {
                        SetPVarInt(playerid, "safe_prop_idx", idx);
                        UI_ShowDialog(playerid, DIALOG_PROP_SAFE_DEP, DIALOG_STYLE_INPUT,
                            "{ Cofre — Depositar", "Valor a depositar:", "Depositar", "Cancelar");
                    }
                    case 2: {
                        SetPVarInt(playerid, "safe_prop_idx", idx);
                        UI_ShowDialog(playerid, DIALOG_PROP_SAFE_WIT, DIALOG_STYLE_INPUT,
                            "{ Cofre — Sacar", "Valor a sacar:", "Sacar", "Cancelar");
                    }
                    case 3: {
                        SetPVarInt(playerid, "sell_prop_idx", idx);
                        new sell_price = g_Properties[idx][prop_Price] * 6 / 10;
                        new fmt[24]; FormatMoneyFull(sell_price, fmt, sizeof(fmt));
                        new body[128];
                        format(body, sizeof(body),
                            "Vender esta propriedade ao servidor por "CSTR_MONEY"%s"CSTR_WHITE"?\n"
                            CSTR_GREY"(60%% do valor de compra)", fmt);
                        UI_ShowConfirm(playerid, "{ Confirmar Venda", body);
                        SetPVarInt(playerid, "confirm_prop_sell", 1);
                    }
                    case 4: {
                        SetPVarInt(playerid, "invite_prop_idx", idx);
                        UI_ShowDialog(playerid, DIALOG_PROP_INVITE, DIALOG_STYLE_INPUT,
                            "{ Convidar Jogador",
                            "ID do jogador para dar acesso à propriedade:", "Convidar", "Cancelar");
                    }
                }
            } else if(g_Properties[idx][prop_ForSale]) {
                Property_Buy(playerid, idx);
            }
            return true;
        }
        case DIALOG_PROP_LIST: {
            if(!response) return true;
            new count = 0, found_idx = -1;
            for(new i = 0; i < g_PropertyCount; i++) {
                if(!g_Properties[i][prop_ForSale]) continue;
                if(count == listitem) { found_idx = i; break; }
                count++;
            }
            if(found_idx >= 0) Property_ShowMenu(playerid, found_idx);
            return true;
        }
        case DIALOG_PROP_SAFE_DEP: {
            if(!response) return true;
            new idx = GetPVarInt(playerid, "safe_prop_idx");
            Property_SafeDeposit(playerid, idx, strval(inputtext));
            return true;
        }
        case DIALOG_PROP_SAFE_WIT: {
            if(!response) return true;
            new idx = GetPVarInt(playerid, "safe_prop_idx");
            Property_SafeWithdraw(playerid, idx, strval(inputtext));
            return true;
        }
        case DIALOG_PROP_INVITE: {
            if(!response) return true;
            new idx      = GetPVarInt(playerid, "invite_prop_idx");
            new targetid = strval(inputtext);
            if(!IsValidPlayer(targetid)) {
                UI_NotifyError(playerid, "Jogador inválido.");
                return true;
            }
            new query[256];
            format(query, sizeof(query),
                "INSERT IGNORE INTO `property_permissions` "
                "(`property_id`, `character_id`, `permission`) "
                "VALUES (%d, %d, 'key')",
                g_Properties[idx][prop_ID],
                PlayerData[targetid][pCharacterID]);
            DB_Execute(query);
            new target_name[MAX_PLAYER_NAME];
            GetPlayerName(targetid, target_name, sizeof(target_name));
            new msg[64];
            format(msg, sizeof(msg), CSTR_SUCCESS"%s agora tem acesso à propriedade.", target_name);
            SendClientMessage(playerid, COLOR_SUCCESS, msg);
            SendInfo(targetid, "Você recebeu acesso a uma propriedade.");
            return true;
        }
    }
    // Confirmação de venda
    if(dialogid == DIALOG_CONFIRM_YES_NO &&
       GetPVarInt(playerid, "confirm_prop_sell")) {
        DeletePVar(playerid, "confirm_prop_sell");
        if(response) {
            new idx = GetPVarInt(playerid, "sell_prop_idx");
            Property_SellToServer(playerid, idx);
        }
        return true;
    }
    return false;
}

// Getters
stock Property_GetOwner(idx) {
    if(idx < 0) return 0;
    return g_Properties[idx][prop_OwnerCharID];
}

stock Property_IsLocked(idx) {
    if(idx < 0) return true;
    return g_Properties[idx][prop_Locked];
}

stock Property_GetGarageSlots(idx) {
    if(idx < 0) return 0;
    return g_Properties[idx][prop_GarageSlots];
}

stock Property_GetPlayerInside(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    return g_PlayerInsideProp[playerid];
}

// Cleanup
public OnPlayerDisconnect(playerid, reason) {
    g_PlayerInsideProp[playerid] = -1;
    return 1;
}

// Comandos
CMD:entrar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new idx = Property_FindNearest(playerid);
    if(idx < 0) {
        // Tentar entrar em veículo próximo
        SendError(playerid, "Nenhuma entrada próxima.");
        return 1;
    }
    Property_Enter(playerid, idx);
    return 1;
}

CMD:sair(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(g_PlayerInsideProp[playerid] >= 0) {
        Property_Exit(playerid);
    } else {
        SendError(playerid, "Você não está dentro de nenhuma propriedade.");
    }
    return 1;
}

CMD:casa(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new idx = Property_FindNearest(playerid);
    if(idx < 0) {
        // Mostrar propriedade do dono
        idx = Property_FindByOwner(PlayerData[playerid][pCharacterID]);
        if(idx < 0) {
            UI_NotifyInfo(playerid, "Você não tem propriedade. Use /imoveis para comprar.");
            return 1;
        }
    }
    Property_ShowMenu(playerid, idx);
    return 1;
}

CMD:imoveis(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Property_ShowForSale(playerid);
    return 1;
}

CMD:trancarCasa(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new idx = -1;
    if(g_PlayerInsideProp[playerid] >= 0) {
        idx = g_PlayerInsideProp[playerid];
    } else {
        idx = Property_FindNearest(playerid);
    }
    if(idx < 0) { SendError(playerid, "Nenhuma propriedade próxima."); return 1; }
    Property_ToggleLock(playerid, idx);
    return 1;
}
