// ============================================================
//  Santa Aurora Roleplay — Postos de Combustível
//  Módulo : vehicles/vehicle_fuel.inc
//  Objetivo: abastecer veículos, preços, diferentes tipos
// ============================================================

#if defined _SA_VEHICLE_FUEL_INC
    #endinput
#endif
#define _SA_VEHICLE_FUEL_INC

#include "vehicle_core.inc"

// Dialogs de posto
#define DIALOG_FUEL_MENU    830
#define DIALOG_FUEL_QTY     831

// ------------------------------------------------------------
// Cache de postos
// ------------------------------------------------------------
#define MAX_FUEL_STATIONS   15

enum E_FUEL_STATION {
    fs_ID,
    fs_Name[48],
    Float:fs_PosX,
    Float:fs_PosY,
    Float:fs_PosZ,
    fs_PriceGas,
    fs_PriceDiesel
}

static g_FuelStations[MAX_FUEL_STATIONS][E_FUEL_STATION];
static g_FuelStationCount = 0;
static bool:g_FuelStationsLoaded = false;

// Estado de abastecimento do jogador
static g_RefuelStation[MAX_PLAYERS] = {-1, ...};

// ------------------------------------------------------------
// Carregar postos
// ------------------------------------------------------------
stock FuelStation_LoadAll() {
    DB_Query(
        "SELECT `id`, `name`, `pos_x`, `pos_y`, `pos_z`, `price_gasoline`, `price_diesel` "
        "FROM `fuel_stations` WHERE `active`=1",
        "OnFuelStationsLoaded", "");
}

forward OnFuelStationsLoaded();
public OnFuelStationsLoaded() {
    g_FuelStationCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_FuelStationCount < MAX_FUEL_STATIONS; r++) {
        new i = g_FuelStationCount;
        g_FuelStations[i][fs_ID]  = DB_GetInt("id", r);
        DB_GetString("name", g_FuelStations[i][fs_Name], sizeof(g_FuelStations[][fs_Name]), r);
        g_FuelStations[i][fs_PosX]       = DB_GetFloat("pos_x",           r);
        g_FuelStations[i][fs_PosY]       = DB_GetFloat("pos_y",           r);
        g_FuelStations[i][fs_PosZ]       = DB_GetFloat("pos_z",           r);
        g_FuelStations[i][fs_PriceGas]   = DB_GetInt("price_gasoline",    r);
        g_FuelStations[i][fs_PriceDiesel]= DB_GetInt("price_diesel",      r);
        g_FuelStationCount++;
    }
    g_FuelStationsLoaded = true;
    new msg[48];
    format(msg, sizeof(msg), "%d postos carregados.", g_FuelStationCount);
    Logger_Info("FUEL", msg);
}

// Posto mais próximo
stock FuelStation_FindNearest(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new best = -1;
    new Float:best_dist = 8.0;
    for(new i = 0; i < g_FuelStationCount; i++) {
        new Float:dist = GetDistance3D(px, py, pz,
            g_FuelStations[i][fs_PosX],
            g_FuelStations[i][fs_PosY],
            g_FuelStations[i][fs_PosZ]);
        if(dist < best_dist) { best_dist = dist; best = i; }
    }
    return best;
}

// ------------------------------------------------------------
// Abrir menu de abastecimento
// ------------------------------------------------------------
stock FuelStation_Open(playerid) {
    if(!IsValidPlayer(playerid)) return;

    // Verificar se está num veículo
    new state = GetPlayerState(playerid);
    if(state != PLAYER_STATE_DRIVER && state != PLAYER_STATE_ONFOOT) {
        UI_NotifyError(playerid, "Você precisa estar a pé ou dirigindo para abastecer.");
        return;
    }

    new fs_idx = FuelStation_FindNearest(playerid);
    if(fs_idx < 0) {
        UI_NotifyError(playerid, "Nenhum posto de combustível próximo.");
        return;
    }

    // Encontrar veículo para abastecer
    new veh_idx = -1;
    if(state == PLAYER_STATE_DRIVER) {
        new sampvehid = GetPlayerVehicleID(playerid);
        veh_idx = Vehicle_GetSlotBySAMPID(sampvehid);
    } else {
        veh_idx = Vehicle_FindNearestOwned(playerid);
        if(veh_idx < 0) veh_idx = Vehicle_FindNearest(playerid);
    }

    if(veh_idx < 0) {
        UI_NotifyError(playerid, "Nenhum veículo para abastecer.");
        return;
    }

    g_RefuelStation[playerid] = fs_idx;
    SetPVarInt(playerid, "refuel_veh_slot", veh_idx);

    new current_fuel = floatround(g_Vehicles[veh_idx][veh_Fuel]);
    new can_add      = 100 - current_fuel;
    new price_per    = g_FuelStations[fs_idx][fs_PriceGas];
    new max_cost     = can_add * price_per;
    new max_cost_fmt[24];
    FormatMoneyFull(max_cost, max_cost_fmt, sizeof(max_cost_fmt));

    new body[256];
    format(body, sizeof(body),
        CSTR_SYSTEM"%s\n\n"
        CSTR_WHITE"Combustível atual: "CSTR_YELLOW"%d%%\n"
        CSTR_WHITE"Espaço disponível: "CSTR_GREY"%d litros\n"
        CSTR_WHITE"Preço: "CSTR_MONEY"$%d/litro\n"
        CSTR_WHITE"Custo para encher: "CSTR_MONEY"%s\n\n"
        CSTR_GREY"Digite a quantidade de litros:",
        g_FuelStations[fs_idx][fs_Name],
        current_fuel, can_add, price_per, max_cost_fmt);

    UI_ShowDialog(playerid, DIALOG_FUEL_QTY, DIALOG_STYLE_INPUT,
        "{ Abastecer", body, "Abastecer", "Cancelar");
}

// ------------------------------------------------------------
// Processar abastecimento
// ------------------------------------------------------------
static FuelStation_Refuel(playerid, liters) {
    if(!IsValidPlayer(playerid) || liters <= 0) return;

    new fs_idx  = g_RefuelStation[playerid];
    new veh_idx = GetPVarInt(playerid, "refuel_veh_slot");
    DeletePVar(playerid, "refuel_veh_slot");

    if(fs_idx < 0 || veh_idx < 0) return;

    new current  = floatround(g_Vehicles[veh_idx][veh_Fuel]);
    new can_add  = 100 - current;
    if(liters > can_add) liters = can_add;
    if(liters <= 0) {
        UI_NotifyWarning(playerid, "Veículo já está cheio.");
        return;
    }

    new price = liters * g_FuelStations[fs_idx][fs_PriceGas];

    // Cobrar
    new bool:paid = false;
    if(PlayerData[playerid][pMoney] >= price) {
        Player_RemoveMoney(playerid, price);
        paid = true;
    } else if(Bank_GetBalance(playerid) >= price) {
        Bank_Debit(playerid, price, "purchase", "Abastecimento");
        paid = true;
    }
    if(!paid) {
        UI_NotifyError(playerid, "Saldo insuficiente para abastecer.");
        return;
    }

    // Abastecer
    Vehicle_AddFuel(veh_idx, float(liters));

    // Religar motor se estava desligado por falta de combustível
    new sampid = Vehicle_GetSAMPID(veh_idx);
    if(IsValidVehicle(sampid)) {
        SetVehicleParamsEx(sampid,
            VEHICLE_PARAMS_ON, VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
            VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
            VEHICLE_PARAMS_UNSET);
    }

    new price_fmt[24];
    FormatMoneyFull(price, price_fmt, sizeof(price_fmt));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Abastecido "CSTR_YELLOW"%d litros"
        CSTR_SUCCESS" por "CSTR_MONEY"%s"CSTR_SUCCESS"!",
        liters, price_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);

    Logger_Economy(playerid, "FUEL", price,
        PlayerData[playerid][pName], g_FuelStations[fs_idx][fs_Name]);
}

// Handler
stock bool:FuelStation_HandleDialog(playerid, dialogid, response, const inputtext[]) {
    switch(dialogid) {
        case DIALOG_FUEL_QTY: {
            if(!response) {
                g_RefuelStation[playerid] = -1;
                DeletePVar(playerid, "refuel_veh_slot");
                return true;
            }
            new liters = strval(inputtext);
            if(liters <= 0) {
                UI_NotifyError(playerid, "Quantidade inválida.");
                return true;
            }
            FuelStation_Refuel(playerid, liters);
            return true;
        }
    }
    return false;
}

// Comandos
CMD:abastecer(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    FuelStation_Open(playerid);
    return 1;
}

CMD:gasolina(playerid, params[]) { return cmd_abastecer(playerid, params); }
