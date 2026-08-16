// ============================================================
//  Santa Aurora Roleplay — Concessionária
//  Módulo : vehicles/vehicle_dealership.inc
//  Objetivo: compra à vista e financiada, catálogo por tipo
// ============================================================

#if defined _SA_VEHICLE_DEALERSHIP_INC
    #endinput
#endif
#define _SA_VEHICLE_DEALERSHIP_INC

#include "vehicle_core.inc"

// Dialogs
#define DIALOG_DEALER_LIST      820
#define DIALOG_DEALER_INFO      821
#define DIALOG_DEALER_BUY       822
#define DIALOG_DEALER_FINANCE   823
#define DIALOG_DEALER_FINANCE_C 824

// ------------------------------------------------------------
// Cache de concessionárias
// ------------------------------------------------------------
#define MAX_DEALERS         10
#define MAX_DEALER_STOCK    50

enum E_DEALER {
    deal_ID,
    deal_Name[48],
    deal_Category[12],
    Float:deal_PosX,
    Float:deal_PosY,
    Float:deal_PosZ
}

enum E_DEALER_STOCK {
    ds_ItemID,
    ds_DealerID,
    ds_Model,
    ds_Name[32],
    ds_Price,
    ds_Color1,
    ds_Color2,
    ds_FuelType[12]
}

static g_Dealers[MAX_DEALERS][E_DEALER];
static g_DealerStock[MAX_DEALERS][MAX_DEALER_STOCK][E_DEALER_STOCK];
static g_DealerStockCount[MAX_DEALERS] = {0, ...};
static g_DealerCount = 0;
static bool:g_DealersLoaded = false;

// Estado de compra
static g_DealerIdx[MAX_PLAYERS]  = {-1, ...};
static g_DealerItem[MAX_PLAYERS] = {-1, ...};

// ------------------------------------------------------------
// Carregar concessionárias
// ------------------------------------------------------------
stock Dealer_LoadAll() {
    DB_Query(
        "SELECT d.`id`, d.`name`, d.`category`, d.`pos_x`, d.`pos_y`, d.`pos_z`, "
        "ds.`id` as item_id, ds.`model`, ds.`name` as veh_name, ds.`price`, "
        "ds.`color1`, ds.`color2`, ds.`fuel_type` "
        "FROM `dealerships` d "
        "LEFT JOIN `dealership_stock` ds ON ds.`dealership_id`=d.`id` "
        "ORDER BY d.`id`, ds.`price`",
        "OnDealersLoaded", "");
}

forward OnDealersLoaded();
public OnDealersLoaded() {
    g_DealerCount = 0;
    new rows = DB_NumRows();

    for(new r = 0; r < rows; r++) {
        new deal_id = DB_GetInt("id", r);

        // Encontrar ou criar dealer no cache
        new d_idx = -1;
        for(new d = 0; d < g_DealerCount; d++) {
            if(g_Dealers[d][deal_ID] == deal_id) { d_idx = d; break; }
        }
        if(d_idx < 0 && g_DealerCount < MAX_DEALERS) {
            d_idx = g_DealerCount;
            g_Dealers[d_idx][deal_ID] = deal_id;
            DB_GetString("name",     g_Dealers[d_idx][deal_Name],     sizeof(g_Dealers[][deal_Name]),     r);
            DB_GetString("category", g_Dealers[d_idx][deal_Category], sizeof(g_Dealers[][deal_Category]), r);
            g_Dealers[d_idx][deal_PosX] = DB_GetFloat("pos_x", r);
            g_Dealers[d_idx][deal_PosY] = DB_GetFloat("pos_y", r);
            g_Dealers[d_idx][deal_PosZ] = DB_GetFloat("pos_z", r);
            g_DealerStockCount[d_idx] = 0;
            g_DealerCount++;
        }
        if(d_idx < 0) continue;

        // Adicionar item ao estoque
        new item_id = DB_GetInt("item_id", r);
        if(item_id <= 0) continue;
        new si = g_DealerStockCount[d_idx];
        if(si >= MAX_DEALER_STOCK) continue;
        g_DealerStock[d_idx][si][ds_ItemID]  = item_id;
        g_DealerStock[d_idx][si][ds_DealerID]= deal_id;
        g_DealerStock[d_idx][si][ds_Model]   = DB_GetInt("model", r);
        DB_GetString("veh_name", g_DealerStock[d_idx][si][ds_Name],     sizeof(g_DealerStock[][][ds_Name]),    r);
        DB_GetString("fuel_type",g_DealerStock[d_idx][si][ds_FuelType], sizeof(g_DealerStock[][][ds_FuelType]),r);
        g_DealerStock[d_idx][si][ds_Price]  = DB_GetInt("price",  r);
        g_DealerStock[d_idx][si][ds_Color1] = DB_GetInt("color1", r);
        g_DealerStock[d_idx][si][ds_Color2] = DB_GetInt("color2", r);
        g_DealerStockCount[d_idx]++;
    }
    g_DealersLoaded = true;
    new msg[48];
    format(msg, sizeof(msg), "%d concessionárias carregadas.", g_DealerCount);
    Logger_Info("DEALER", msg);
}

// Encontrar concessionária mais próxima
stock Dealer_FindNearest(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new best = -1;
    new Float:best_dist = 5.0;
    for(new d = 0; d < g_DealerCount; d++) {
        new Float:dist = GetDistance3D(px, py, pz,
            g_Dealers[d][deal_PosX], g_Dealers[d][deal_PosY], g_Dealers[d][deal_PosZ]);
        if(dist < best_dist) { best_dist = dist; best = d; }
    }
    return best;
}

// ------------------------------------------------------------
// Mostrar catálogo
// ------------------------------------------------------------
stock Dealer_ShowCatalog(playerid, d_idx = -1) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_DealersLoaded) {
        UI_NotifyError(playerid, "Concessionária não carregada.");
        return;
    }
    if(d_idx < 0) d_idx = Dealer_FindNearest(playerid);
    if(d_idx < 0) {
        UI_NotifyError(playerid, "Nenhuma concessionária próxima.");
        return;
    }
    g_DealerIdx[playerid] = d_idx;

    UI_ListReset();
    for(new i = 0; i < g_DealerStockCount[d_idx]; i++) {
        new price_fmt[24];
        FormatMoneyFull(g_DealerStock[d_idx][i][ds_Price], price_fmt, sizeof(price_fmt));
        new line[64];
        format(line, sizeof(line), "%s  %s",
            g_DealerStock[d_idx][i][ds_Name], price_fmt);
        UI_ListAdd(line);
    }
    if(g_DealerStockCount[d_idx] == 0) UI_ListAdd("Sem veículos disponíveis.");

    new title[64];
    format(title, sizeof(title), "{ %s", g_Dealers[d_idx][deal_Name]);
    UI_ListShow(playerid, DIALOG_DEALER_LIST, title, "Ver Detalhes", "Fechar");
}

// Mostrar detalhes de um veículo
static Dealer_ShowVehicleInfo(playerid, d_idx, si) {
    g_DealerItem[playerid] = si;
    new price      = g_DealerStock[d_idx][si][ds_Price];
    new down       = price / 5;           // 20% de entrada
    new financed   = price - down;
    new monthly    = floatround(float(financed) * 1.05 / 12.0);

    new price_fmt[24], down_fmt[24], monthly_fmt[24];
    FormatMoneyFull(price,   price_fmt,   sizeof(price_fmt));
    FormatMoneyFull(down,    down_fmt,    sizeof(down_fmt));
    FormatMoneyFull(monthly, monthly_fmt, sizeof(monthly_fmt));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s ===\n\n"
        CSTR_WHITE"Preço à vista: "CSTR_MONEY"%s\n\n"
        CSTR_WHITE"Financiamento:\n"
        CSTR_GREY"  Entrada: "CSTR_WHITE"%s\n"
        CSTR_GREY"  12x de: "CSTR_WHITE"%s\n"
        CSTR_GREY"  Total: "CSTR_ERROR"%s\n\n"
        CSTR_WHITE"Combustível: "CSTR_GREY"%s",
        g_DealerStock[d_idx][si][ds_Name],
        price_fmt, down_fmt, monthly_fmt,
        price_fmt,  // placeholder total
        g_DealerStock[d_idx][si][ds_FuelType]);

    UI_ShowDialog(playerid, DIALOG_DEALER_INFO, DIALOG_STYLE_MSGBOX,
        "{ Detalhes do Veículo", body, "Comprar à Vista", "Financiar");
}

// ------------------------------------------------------------
// Compra à vista
// ------------------------------------------------------------
static Dealer_BuyNow(playerid, d_idx, si) {
    if(!IsValidPlayer(playerid)) return;
    new price = g_DealerStock[d_idx][si][ds_Price];

    if(Vehicle_CountOwned(PlayerData[playerid][pCharacterID]) >= MAX_PLAYER_VEHICLES) {
        UI_NotifyError(playerid, "Você já atingiu o limite de veículos.");
        return;
    }

    // Tentar cobrar
    new bool:paid = false;
    if(PlayerData[playerid][pMoney] >= price) {
        Player_RemoveMoney(playerid, price);
        paid = true;
    } else if(Bank_GetBalance(playerid) >= price) {
        Bank_Debit(playerid, price, "purchase", "Compra de veículo");
        paid = true;
    }

    if(!paid) {
        UI_NotifyError(playerid, "Saldo insuficiente para comprar este veículo.");
        return;
    }

    // Obter posição do jogador para spawnar
    new Float:px, Float:py, Float:pz, Float:pa;
    GetPlayerPos(playerid, px, py, pz);
    GetPlayerFacingAngle(playerid, pa);

    new idx = Vehicle_Create(
        PlayerData[playerid][pCharacterID],
        g_DealerStock[d_idx][si][ds_Model],
        g_DealerStock[d_idx][si][ds_Color1],
        g_DealerStock[d_idx][si][ds_Color2],
        px + 3.0, py, pz, pa);

    if(idx < 0) {
        UI_NotifyError(playerid, "Erro ao criar veículo. Contate um administrador.");
        Player_AddMoney(playerid, price); // devolver
        return;
    }

    new price_fmt[24];
    FormatMoneyFull(price, price_fmt, sizeof(price_fmt));
    new msg[128];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Parabéns! Você comprou o "CSTR_SYSTEM"%s"
        CSTR_SUCCESS" por "CSTR_MONEY"%s"CSTR_SUCCESS"!",
        g_DealerStock[d_idx][si][ds_Name], price_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    SendInfo(playerid, "Use /trancar para trancar/destrancar seu veículo.");
    GameTextForPlayer(playerid, "~g~Veiculo Comprado!", 4000, 3);

    Logger_Economy(playerid, "VEHICLE_BUY", price,
        PlayerData[playerid][pName], g_DealerStock[d_idx][si][ds_Name]);
}

// Financiamento
static Dealer_Finance(playerid, d_idx, si) {
    if(!IsValidPlayer(playerid)) return;
    new price    = g_DealerStock[d_idx][si][ds_Price];
    new down     = price / 5;

    if(PlayerData[playerid][pMoney] < down &&
       Bank_GetBalance(playerid) < down) {
        UI_NotifyError(playerid, "Saldo insuficiente para a entrada.");
        return;
    }
    if(Vehicle_CountOwned(PlayerData[playerid][pCharacterID]) >= MAX_PLAYER_VEHICLES) {
        UI_NotifyError(playerid, "Você já atingiu o limite de veículos.");
        return;
    }

    // Cobrar entrada
    if(PlayerData[playerid][pMoney] >= down) {
        Player_RemoveMoney(playerid, down);
    } else {
        Bank_Debit(playerid, down, "purchase", "Entrada financiamento veículo");
    }

    new Float:px, Float:py, Float:pz, Float:pa;
    GetPlayerPos(playerid, px, py, pz);
    GetPlayerFacingAngle(playerid, pa);
    new idx = Vehicle_Create(
        PlayerData[playerid][pCharacterID],
        g_DealerStock[d_idx][si][ds_Model],
        g_DealerStock[d_idx][si][ds_Color1],
        g_DealerStock[d_idx][si][ds_Color2],
        px + 3.0, py, pz, pa);

    if(idx < 0) { Player_AddMoney(playerid, down); return; }

    // Registrar financiamento
    new financed = price - down;
    new monthly  = floatround(float(financed) * 1.05 / 12.0);
    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `vehicle_financing` "
        "(`vehicle_id`, `character_id`, `total_price`, `down_payment`, "
        "`balance`, `installments`, `monthly_payment`, `status`) "
        "VALUES (%d, %d, %d, %d, %d, 12, %d, 'active')",
        g_Vehicles[idx][veh_DBID],
        PlayerData[playerid][pCharacterID],
        price, down, financed, monthly);
    DB_Execute(query);

    new down_fmt[24], monthly_fmt[24];
    FormatMoneyFull(down,    down_fmt,    sizeof(down_fmt));
    FormatMoneyFull(monthly, monthly_fmt, sizeof(monthly_fmt));
    new msg[128];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Financiamento aprovado! Entrada: "CSTR_MONEY"%s"
        CSTR_SUCCESS" | 12x "CSTR_MONEY"%s",
        down_fmt, monthly_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    Logger_Economy(playerid, "VEHICLE_FINANCE", down,
        PlayerData[playerid][pName], g_DealerStock[d_idx][si][ds_Name]);
}

// Handler dialogs
stock bool:Dealer_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_DEALER_LIST: {
            if(!response) return true;
            new d_idx = g_DealerIdx[playerid];
            if(d_idx < 0 || listitem >= g_DealerStockCount[d_idx]) return true;
            Dealer_ShowVehicleInfo(playerid, d_idx, listitem);
            return true;
        }
        case DIALOG_DEALER_INFO: {
            new d_idx = g_DealerIdx[playerid];
            new si    = g_DealerItem[playerid];
            if(d_idx < 0 || si < 0) return true;
            if(response) Dealer_BuyNow(playerid, d_idx, si);     // à vista
            else         Dealer_Finance(playerid, d_idx, si);    // financiar
            return true;
        }
    }
    return false;
}

// Comandos
CMD:concessionaria(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Dealer_ShowCatalog(playerid);
    return 1;
}
CMD:carros(playerid, params[]) { return cmd_concessionaria(playerid, params); }
