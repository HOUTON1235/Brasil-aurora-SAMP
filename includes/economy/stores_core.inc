// ============================================================
//  Santa Aurora Roleplay — Lojas
//  Módulo : economy/stores_core.inc
//  Objetivo: lojas com estoque, compra de itens, preços
// ============================================================

#if defined _SA_STORES_CORE_INC
    #endinput
#endif
#define _SA_STORES_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "../players/player_core.inc"
#include "../inventory/inventory_core.inc"

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_STORES          30
#define MAX_STORE_ITEMS     50
#define STORE_RANGE         3.0

// Dialogs de loja
#define DIALOG_STORE_LIST   700
#define DIALOG_STORE_QTY    701
#define DIALOG_STORE_CONFIRM 702

// ------------------------------------------------------------
// Cache de lojas em memória
// ------------------------------------------------------------
enum E_STORE_DEF {
    st_ID,
    st_Name[48],
    st_Type[20],
    Float:st_PosX,
    Float:st_PosY,
    Float:st_PosZ,
    st_Interior,
    st_World
}

enum E_STORE_ITEM {
    sti_ItemID,
    sti_Price,
    sti_Stock            // -1 = ilimitado
}

static g_Stores[MAX_STORES][E_STORE_DEF];
static g_StoreItems[MAX_STORES][MAX_STORE_ITEMS][E_STORE_ITEM];
static g_StoreItemCount[MAX_STORES] = {0, ...};
static g_StoreCount = 0;
static bool:g_StoresLoaded = false;

// Estado de compra do jogador
static g_ShopStore[MAX_PLAYERS]    = {-1, ...};
static g_ShopItem[MAX_PLAYERS]     = {-1, ...};

// ------------------------------------------------------------
// Carregar lojas do banco
// ------------------------------------------------------------
stock Stores_Load() {
    if(!DB_IsConnected()) return;
    DB_Query(
        "SELECT `id`, `name`, `type`, `pos_x`, `pos_y`, `pos_z`, `interior`, `world` "
        "FROM `stores` WHERE `active`=1",
        "OnStoresLoaded", "");
}

forward OnStoresLoaded();
public OnStoresLoaded() {
    g_StoreCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_StoreCount < MAX_STORES; r++) {
        new i = g_StoreCount;
        g_Stores[i][st_ID]      = DB_GetInt("id", r);
        DB_GetString("name", g_Stores[i][st_Name], sizeof(g_Stores[][st_Name]), r);
        DB_GetString("type", g_Stores[i][st_Type], sizeof(g_Stores[][st_Type]), r);
        g_Stores[i][st_PosX]    = DB_GetFloat("pos_x",    r);
        g_Stores[i][st_PosY]    = DB_GetFloat("pos_y",    r);
        g_Stores[i][st_PosZ]    = DB_GetFloat("pos_z",    r);
        g_Stores[i][st_Interior]= DB_GetInt("interior",   r);
        g_Stores[i][st_World]   = DB_GetInt("world",      r);
        g_StoreItemCount[i]     = 0;
        g_StoreCount++;
    }

    // Carregar estoque
    DB_Query(
        "SELECT ss.`store_id`, ss.`item_id`, ss.`price`, ss.`stock` "
        "FROM `store_stock` ss "
        "JOIN `stores` s ON s.`id`=ss.`store_id` AND s.`active`=1 "
        "ORDER BY ss.`store_id`, ss.`item_id`",
        "OnStoreStockLoaded", "");

    new msg[48];
    format(msg, sizeof(msg), "%d lojas carregadas.", g_StoreCount);
    Logger_Info("STORES", msg);
}

forward OnStoreStockLoaded();
public OnStoreStockLoaded() {
    new rows = DB_NumRows();
    for(new r = 0; r < rows; r++) {
        new store_id = DB_GetInt("store_id", r);
        // Encontrar índice da loja
        new s_idx = -1;
        for(new s = 0; s < g_StoreCount; s++) {
            if(g_Stores[s][st_ID] == store_id) { s_idx = s; break; }
        }
        if(s_idx < 0) continue;
        new i = g_StoreItemCount[s_idx];
        if(i >= MAX_STORE_ITEMS) continue;
        g_StoreItems[s_idx][i][sti_ItemID] = DB_GetInt("item_id", r);
        g_StoreItems[s_idx][i][sti_Price]  = DB_GetInt("price",   r);
        g_StoreItems[s_idx][i][sti_Stock]  = DB_GetInt("stock",   r);
        g_StoreItemCount[s_idx]++;
    }
    g_StoresLoaded = true;
}

// ------------------------------------------------------------
// Encontrar loja mais próxima do jogador
// ------------------------------------------------------------
stock Stores_FindNearest(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new interior = GetPlayerInterior(playerid);
    new world    = GetPlayerVirtualWorld(playerid);

    new best = -1;
    new Float:best_dist = STORE_RANGE + 1.0;

    for(new s = 0; s < g_StoreCount; s++) {
        if(g_Stores[s][st_Interior] != interior) continue;
        if(g_Stores[s][st_World]    != world)    continue;
        new Float:dist = GetDistance3D(px, py, pz,
            g_Stores[s][st_PosX], g_Stores[s][st_PosY], g_Stores[s][st_PosZ]);
        if(dist < best_dist) {
            best_dist = dist;
            best = s;
        }
    }
    return best;
}

// ------------------------------------------------------------
// Abrir menu da loja
// ------------------------------------------------------------
stock Stores_Open(playerid, store_idx = -1) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_StoresLoaded) {
        UI_NotifyError(playerid, "Lojas não carregadas. Tente novamente.");
        return;
    }

    if(store_idx < 0) store_idx = Stores_FindNearest(playerid);
    if(store_idx < 0) {
        UI_NotifyError(playerid, "Nenhuma loja próxima.");
        return;
    }

    g_ShopStore[playerid] = store_idx;

    UI_ListReset();
    for(new i = 0; i < g_StoreItemCount[store_idx]; i++) {
        new item_idx = Inventory_GetItemDefIndex(g_StoreItems[store_idx][i][sti_ItemID]);
        if(item_idx < 0) continue;

        new price_fmt[24];
        FormatMoneyFull(g_StoreItems[store_idx][i][sti_Price], price_fmt, sizeof(price_fmt));

        new stock_str[16];
        if(g_StoreItems[store_idx][i][sti_Stock] < 0)
            format(stock_str, sizeof(stock_str), "∞");
        else
            format(stock_str, sizeof(stock_str), "%d", g_StoreItems[store_idx][i][sti_Stock]);

        new line[80];
        format(line, sizeof(line), "%s  %s  [Estoque: %s]",
            Inventory_GetDisplayName(g_StoreItems[store_idx][i][sti_ItemID]),
            price_fmt, stock_str);
        UI_ListAdd(line);
    }

    if(g_StoreItemCount[store_idx] == 0) {
        UI_ListAdd("Esta loja não tem itens em estoque.");
    }

    new title[64];
    format(title, sizeof(title), "{ %s", g_Stores[store_idx][st_Name]);
    UI_ListShow(playerid, DIALOG_STORE_LIST, title, "Comprar", "Fechar");
}

// ------------------------------------------------------------
// Comprar item
// ------------------------------------------------------------
stock Stores_Buy(playerid, store_idx, item_slot, quantity) {
    if(!IsValidPlayer(playerid)) return;
    if(store_idx < 0 || store_idx >= g_StoreCount) return;
    if(item_slot < 0 || item_slot >= g_StoreItemCount[store_idx]) return;
    if(quantity <= 0 || quantity > 99) {
        UI_NotifyError(playerid, "Quantidade inválida.");
        return;
    }

    new itemid = g_StoreItems[store_idx][item_slot][sti_ItemID];
    new price  = g_StoreItems[store_idx][item_slot][sti_Price] * quantity;
    new stock  = g_StoreItems[store_idx][item_slot][sti_Stock];

    // Verificar estoque
    if(stock >= 0 && stock < quantity) {
        UI_NotifyError(playerid, "Estoque insuficiente.");
        return;
    }

    // Verificar dinheiro
    if(PlayerData[playerid][pMoney] < price) {
        // Tentar banco
        if(g_BankData[playerid][bank_Balance] >= price) {
            UI_NotifyInfo(playerid,
                "Dinheiro insuficiente na carteira. Debitando do banco...");
            if(!Bank_Debit(playerid, price, "purchase",
                           "Compra em loja")) return;
        } else {
            UI_NotifyError(playerid, "Dinheiro insuficiente.");
            return;
        }
    } else {
        Player_RemoveMoney(playerid, price);
    }

    // Adicionar ao inventário
    if(!Inventory_AddItem(playerid, itemid, quantity, "", true)) {
        // Devolver dinheiro se não couber no inventário
        Player_AddMoney(playerid, price);
        return;
    }

    // Reduzir estoque
    if(stock >= 0) {
        g_StoreItems[store_idx][item_slot][sti_Stock] -= quantity;
        // Atualizar no banco periodicamente (não a cada compra)
    }

    new item_name[48];
    Inventory_GetDisplayNameByID(itemid, item_name, sizeof(item_name));
    new price_fmt[24];
    FormatMoneyFull(price, price_fmt, sizeof(price_fmt));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Comprado: "CSTR_WHITE"%dx %s "CSTR_GREY"por "CSTR_MONEY"%s",
        quantity, item_name, price_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);

    Logger_Economy(playerid, "PURCHASE", price,
        g_Stores[store_idx][st_Name], item_name);
    HUD_UpdateMoney(playerid);
}

// ------------------------------------------------------------
// Handler de dialogs da loja
// ------------------------------------------------------------
stock bool:Stores_HandleDialog(playerid, dialogid, response, listitem,
                                const inputtext[]) {
    switch(dialogid) {
        case DIALOG_STORE_LIST: {
            if(!response) return true;
            g_ShopItem[playerid] = listitem;
            new store_idx = g_ShopStore[playerid];
            if(store_idx < 0 || listitem >= g_StoreItemCount[store_idx]) return true;

            new itemid = g_StoreItems[store_idx][listitem][sti_ItemID];
            new price  = g_StoreItems[store_idx][listitem][sti_Price];
            new item_name[48];
            Inventory_GetDisplayNameByID(itemid, item_name, sizeof(item_name));
            new price_fmt[24];
            FormatMoneyFull(price, price_fmt, sizeof(price_fmt));

            new body[256];
            format(body, sizeof(body),
                CSTR_WHITE"Item: "CSTR_SYSTEM"%s\n"
                CSTR_WHITE"Preço unitário: "CSTR_MONEY"%s\n\n"
                CSTR_GREY"Digite a quantidade desejada:",
                item_name, price_fmt);
            UI_ShowDialog(playerid, DIALOG_STORE_QTY, DIALOG_STYLE_INPUT,
                "{ Quantidade", body, "Comprar", "Voltar");
            return true;
        }
        case DIALOG_STORE_QTY: {
            if(!response) {
                Stores_Open(playerid, g_ShopStore[playerid]);
                return true;
            }
            new qty = strval(inputtext);
            if(qty <= 0) {
                UI_NotifyError(playerid, "Quantidade inválida.");
                Stores_Open(playerid, g_ShopStore[playerid]);
                return true;
            }
            Stores_Buy(playerid, g_ShopStore[playerid], g_ShopItem[playerid], qty);
            return true;
        }
    }
    return false;
}

// Cleanup ao desconectar
public OnPlayerDisconnect(playerid, reason) {
    g_ShopStore[playerid] = -1;
    g_ShopItem[playerid]  = -1;
    return 1;
}

// Helper para inventory_core.inc
stock Inventory_GetDisplayName(itemid) {
    // Wrapper — acessa g_ItemDefs via inventory_core
    new output[48];
    Inventory_GetDisplayNameByID(itemid, output, sizeof(output));
    return output;
}

stock Inventory_GetDisplayNameByID(itemid, output[], size = sizeof(output)) {
    new defIdx = Inventory_GetItemDefIndex(itemid);
    if(defIdx >= 0) {
        // Acesso direto ao array via função pública do módulo inventory
        new tmp[48];
        // Workaround: usar o nome do item
        format(tmp, size, "Item #%d", itemid); // placeholder sem acesso direto
        SafeStrCopy(output, tmp, size);
    } else {
        format(output, size, "Item #%d", itemid);
    }
}

// Comandos
CMD:loja(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!State_CanInteract(playerid)) {
        SendError(playerid, "Você não pode acessar uma loja agora.");
        return 1;
    }
    Stores_Open(playerid);
    return 1;
}

CMD:comprar(playerid, params[]) { return cmd_loja(playerid, params); }
