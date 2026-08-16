// ============================================================
//  Santa Aurora Roleplay ? Sistema de Veiculos
//  Modulo : vehicles/vehicle_core.inc
//  Objetivo: spawn, lock/unlock, combustivel, dano, save/load,
//            placa, proprietario
// ============================================================

#if defined _SA_VEHICLE_CORE_INC
    #endinput
#endif
#define _SA_VEHICLE_CORE_INC

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
#define MAX_SPAWNED_VEHICLES    200     // SA-MP suporta ate 2000
#define MAX_PLAYER_VEHICLES     3       // veiculos por jogador
#define VEHICLE_SAVE_INTERVAL   300     // salvar a cada 5 min
#define FUEL_MAX                100.0
#define FUEL_MIN                0.0
#define FUEL_EMPTY_THRESHOLD    5.0
#define FUEL_DECAY_PER_KM       0.8     // litros por km
#define VEHICLE_INTERACTION_RANGE 3.0
#define PLATE_LENGTH            8

// Dialogs de veiculo
#define DIALOG_VEHICLE_MENU     800
#define DIALOG_VEHICLE_SELL     801
#define DIALOG_VEHICLE_PLATE    802

// ------------------------------------------------------------
// Dados de veiculo em memoria
// ------------------------------------------------------------
enum E_VEHICLE_DATA {
    veh_DBID,
    veh_OwnerCharID,
    veh_Model,
    veh_Plate[PLATE_LENGTH + 1],
    veh_Color1,
    veh_Color2,
    Float:veh_Fuel,
    Float:veh_Health,
    Float:veh_EngineHealth,
    veh_Mileage,
    veh_GarageID,
    Float:veh_PosX,
    Float:veh_PosY,
    Float:veh_PosZ,
    Float:veh_PosA,
    veh_Interior,
    veh_World,
    bool:veh_Locked,
    bool:veh_Spawned,
    bool:veh_Impounded,
    veh_SAMPVehicleID,      // ID do CreateVehicle no SA-MP
    bool:veh_Dirty
}

static g_Vehicles[MAX_SPAWNED_VEHICLES][E_VEHICLE_DATA];
static g_VehicleCount = 0;

// Mapa de SAMPVehicleID ? indice no nosso array
// SA-MP permite IDs de veiculo ate 3000
static g_VehicleMap[3000] = {-1, ...};

// Veiculo que o jogador esta dirigindo agora
static g_PlayerVehicleSlot[MAX_PLAYERS] = {-1, ...};

// Distancia acumulada para mileage
static Float:g_LastVehPos[MAX_PLAYERS][3];
static Float:g_AccumDist[MAX_SPAWNED_VEHICLES] = {0.0, ...};

// ------------------------------------------------------------
// Gerar placa aleatoria (formato: AAA-0000)
// ------------------------------------------------------------
stock Vehicle_GeneratePlate(output[], size = sizeof(output)) {
    new chars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ";
    format(output, size, "%c%c%c-%04d",
        chars[random(sizeof(chars)-1)],
        chars[random(sizeof(chars)-1)],
        chars[random(sizeof(chars)-1)],
        random(9999) + 1000);
}

// Verificar se placa ja existe
stock Vehicle_PlateExists(plate[]) {
    for(new i = 0; i < g_VehicleCount; i++) {
        if(!strcmp(g_Vehicles[i][veh_Plate], plate, true)) return true;
    }
    return false;
}

// ------------------------------------------------------------
// Encontrar slot livre no array de veiculos
// ------------------------------------------------------------
static Vehicle_FindFreeSlot() {
    for(new i = 0; i < MAX_SPAWNED_VEHICLES; i++) {
        if(g_Vehicles[i][veh_DBID] == INVALID_DB_ID) return i;
    }
    return -1;
}

// Encontrar slot pelo DBID
stock Vehicle_GetSlotByDBID(dbid) {
    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_DBID] == dbid) return i;
    }
    return -1;
}

// Encontrar slot pelo SAMP vehicle ID
stock Vehicle_GetSlotBySAMPID(sampvehid) {
    if(sampvehid < 1 || sampvehid >= 3000) return -1;
    return g_VehicleMap[sampvehid];
}

// ------------------------------------------------------------
// Inicializar slot de veiculo
// ------------------------------------------------------------
static Vehicle_InitSlot(idx) {
    g_Vehicles[idx][veh_DBID]         = INVALID_DB_ID;
    g_Vehicles[idx][veh_OwnerCharID]  = 0;
    g_Vehicles[idx][veh_Model]        = 0;
    g_Vehicles[idx][veh_Plate][0]     = '\0';
    g_Vehicles[idx][veh_Color1]       = 0;
    g_Vehicles[idx][veh_Color2]       = 1;
    g_Vehicles[idx][veh_Fuel]         = FUEL_MAX;
    g_Vehicles[idx][veh_Health]       = 1000.0;
    g_Vehicles[idx][veh_EngineHealth] = 1000.0;
    g_Vehicles[idx][veh_Mileage]      = 0;
    g_Vehicles[idx][veh_GarageID]     = 0;
    g_Vehicles[idx][veh_PosX]         = 0.0;
    g_Vehicles[idx][veh_PosY]         = 0.0;
    g_Vehicles[idx][veh_PosZ]         = 0.0;
    g_Vehicles[idx][veh_PosA]         = 0.0;
    g_Vehicles[idx][veh_Interior]     = 0;
    g_Vehicles[idx][veh_World]        = 0;
    g_Vehicles[idx][veh_Locked]       = true;
    g_Vehicles[idx][veh_Spawned]      = false;
    g_Vehicles[idx][veh_Impounded]    = false;
    g_Vehicles[idx][veh_SAMPVehicleID]= 0;
    g_Vehicles[idx][veh_Dirty]        = false;
}

// ------------------------------------------------------------
// Carregar todos os veiculos nao guardados na garagem (startup)
// ------------------------------------------------------------
stock Vehicle_LoadAll() {
    if(!DB_IsConnected()) return;
    DB_Query(
        "SELECT `id`, `owner_char_id`, `model`, `plate`, `color1`, `color2`, "
        "`fuel`, `health`, `engine_health`, `mileage`, `garage_id`, "
        "`pos_x`, `pos_y`, `pos_z`, `pos_a`, `interior`, `world`, "
        "`locked`, `spawned`, `impounded` "
        "FROM `vehicles` WHERE `garage_id` IS NULL AND `impounded`=0 "
        "ORDER BY `id` ASC LIMIT " #MAX_SPAWNED_VEHICLES,
        "OnAllVehiclesLoaded", "");
}

forward OnAllVehiclesLoaded();
public OnAllVehiclesLoaded() {
    g_VehicleCount = 0;
    new rows = DB_NumRows();

    for(new r = 0; r < rows && g_VehicleCount < MAX_SPAWNED_VEHICLES; r++) {
        new idx = g_VehicleCount;
        Vehicle_InitSlot(idx);

        g_Vehicles[idx][veh_DBID]        = DB_GetInt("id",           r);
        g_Vehicles[idx][veh_OwnerCharID] = DB_GetInt("owner_char_id",r);
        g_Vehicles[idx][veh_Model]       = DB_GetInt("model",        r);
        DB_GetString("plate", g_Vehicles[idx][veh_Plate],
            sizeof(g_Vehicles[][veh_Plate]), r);
        g_Vehicles[idx][veh_Color1]       = DB_GetInt("color1",        r);
        g_Vehicles[idx][veh_Color2]       = DB_GetInt("color2",        r);
        g_Vehicles[idx][veh_Fuel]         = DB_GetFloat("fuel",         r);
        g_Vehicles[idx][veh_Health]       = DB_GetFloat("health",       r);
        g_Vehicles[idx][veh_EngineHealth] = DB_GetFloat("engine_health",r);
        g_Vehicles[idx][veh_Mileage]      = DB_GetInt("mileage",       r);
        g_Vehicles[idx][veh_PosX]         = DB_GetFloat("pos_x",        r);
        g_Vehicles[idx][veh_PosY]         = DB_GetFloat("pos_y",        r);
        g_Vehicles[idx][veh_PosZ]         = DB_GetFloat("pos_z",        r);
        g_Vehicles[idx][veh_PosA]         = DB_GetFloat("pos_a",        r);
        g_Vehicles[idx][veh_Interior]     = DB_GetInt("interior",      r);
        g_Vehicles[idx][veh_World]        = DB_GetInt("world",         r);
        g_Vehicles[idx][veh_Locked]       = bool:DB_GetInt("locked",   r);
        g_Vehicles[idx][veh_Impounded]    = bool:DB_GetInt("impounded",r);

        // Criar veiculo no mundo SA-MP
        Vehicle_Spawn(idx);
        g_VehicleCount++;
    }

    new msg[48];
    format(msg, sizeof(msg), "%d veiculos carregados no mundo.", g_VehicleCount);
    Logger_Info("VEHICLES", msg);
}

// ------------------------------------------------------------
// Spawnar veiculo no mundo
// ------------------------------------------------------------
stock Vehicle_Spawn(idx) {
    if(idx < 0 || idx >= MAX_SPAWNED_VEHICLES) return;
    if(g_Vehicles[idx][veh_Model] <= 0) return;

    new sampid = CreateVehicle(
        g_Vehicles[idx][veh_Model],
        g_Vehicles[idx][veh_PosX],
        g_Vehicles[idx][veh_PosY],
        g_Vehicles[idx][veh_PosZ],
        g_Vehicles[idx][veh_PosA],
        g_Vehicles[idx][veh_Color1],
        g_Vehicles[idx][veh_Color2],
        -1  // nunca re-spawnar automaticamente
    );

    if(sampid == INVALID_VEHICLE_ID) {
        Logger_Error("VEHICLES", "Falha ao criar veiculo no SA-MP");
        return;
    }

    SetVehicleNumberPlate(sampid, g_Vehicles[idx][veh_Plate]);
    SetVehicleVirtualWorld(sampid, g_Vehicles[idx][veh_World]);
    LinkVehicleToInterior(sampid, g_Vehicles[idx][veh_Interior]);

    // Aplicar saude
    SetVehicleHealth(sampid, g_Vehicles[idx][veh_Health]);

    g_Vehicles[idx][veh_SAMPVehicleID] = sampid;
    g_Vehicles[idx][veh_Spawned]       = true;

    // Registrar no mapa
    if(sampid > 0 && sampid < 3000) {
        g_VehicleMap[sampid] = idx;
    }

    CallLocalFunction("OnVehicleCreated", "iii",
        g_Vehicles[idx][veh_DBID],
        g_Vehicles[idx][veh_Model],
        g_Vehicles[idx][veh_OwnerCharID]);
}

// Remover veiculo do mundo
stock Vehicle_Despawn(idx) {
    if(idx < 0) return;
    if(!g_Vehicles[idx][veh_Spawned]) return;

    // Salvar posicao antes de destruir
    new sampid = g_Vehicles[idx][veh_SAMPVehicleID];
    if(sampid > 0 && IsValidVehicle(sampid)) {
        GetVehiclePos(sampid,
            g_Vehicles[idx][veh_PosX],
            g_Vehicles[idx][veh_PosY],
            g_Vehicles[idx][veh_PosZ]);
        GetVehicleZAngle(sampid, g_Vehicles[idx][veh_PosA]);
        GetVehicleHealth(sampid, g_Vehicles[idx][veh_Health]);

        if(sampid < 3000) g_VehicleMap[sampid] = -1;
        DestroyVehicle(sampid);
    }

    g_Vehicles[idx][veh_SAMPVehicleID] = 0;
    g_Vehicles[idx][veh_Spawned]       = false;
    g_Vehicles[idx][veh_Dirty]         = true;
}

// ------------------------------------------------------------
// Salvar veiculo no banco
// ------------------------------------------------------------
stock Vehicle_Save(idx) {
    if(idx < 0 || g_Vehicles[idx][veh_DBID] == INVALID_DB_ID) return;

    // Atualizar posicao se spawned
    new sampid = g_Vehicles[idx][veh_SAMPVehicleID];
    if(g_Vehicles[idx][veh_Spawned] && IsValidVehicle(sampid)) {
        GetVehiclePos(sampid,
            g_Vehicles[idx][veh_PosX],
            g_Vehicles[idx][veh_PosY],
            g_Vehicles[idx][veh_PosZ]);
        GetVehicleZAngle(sampid, g_Vehicles[idx][veh_PosA]);
        GetVehicleHealth(sampid, g_Vehicles[idx][veh_Health]);
    }

    new escaped_plate[PLATE_LENGTH * 2];
    DB_Escape(g_Vehicles[idx][veh_Plate], escaped_plate, sizeof(escaped_plate));

    new query[512];
    format(query, sizeof(query),
        "UPDATE `vehicles` SET "
        "`fuel`=%.2f, `health`=%.2f, `engine_health`=%.2f, `mileage`=%d, "
        "`pos_x`=%.4f, `pos_y`=%.4f, `pos_z`=%.4f, `pos_a`=%.4f, "
        "`locked`=%d, `spawned`=%d, `impounded`=%d, `updated_at`=NOW() "
        "WHERE `id`=%d",
        g_Vehicles[idx][veh_Fuel],
        g_Vehicles[idx][veh_Health],
        g_Vehicles[idx][veh_EngineHealth],
        g_Vehicles[idx][veh_Mileage],
        g_Vehicles[idx][veh_PosX],
        g_Vehicles[idx][veh_PosY],
        g_Vehicles[idx][veh_PosZ],
        g_Vehicles[idx][veh_PosA],
        g_Vehicles[idx][veh_Locked] ? 1 : 0,
        g_Vehicles[idx][veh_Spawned] ? 1 : 0,
        g_Vehicles[idx][veh_Impounded] ? 1 : 0,
        g_Vehicles[idx][veh_DBID]);
    DB_Execute(query);
    g_Vehicles[idx][veh_Dirty] = false;
}

// Salvar todos os veiculos sujos
stock Vehicle_SaveAll() {
    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_Dirty]) Vehicle_Save(i);
    }
}

// ------------------------------------------------------------
// Criar novo veiculo (compra/admin)
// ------------------------------------------------------------
stock Vehicle_Create(owner_char_id, model, color1, color2,
                     Float:pos_x, Float:pos_y, Float:pos_z, Float:pos_a) {
    new idx = Vehicle_FindFreeSlot();
    if(idx < 0) {
        Logger_Error("VEHICLES", "Vehicle_Create: sem slots disponiveis");
        return -1;
    }

    // Gerar placa unica
    new plate[PLATE_LENGTH + 1];
    do {
        Vehicle_GeneratePlate(plate, sizeof(plate));
    } while(Vehicle_PlateExists(plate));

    Vehicle_InitSlot(idx);
    g_Vehicles[idx][veh_OwnerCharID] = owner_char_id;
    g_Vehicles[idx][veh_Model]       = model;
    SafeStrCopy(g_Vehicles[idx][veh_Plate], plate, sizeof(g_Vehicles[][veh_Plate]));
    g_Vehicles[idx][veh_Color1]      = color1;
    g_Vehicles[idx][veh_Color2]      = color2;
    g_Vehicles[idx][veh_Fuel]        = FUEL_MAX;
    g_Vehicles[idx][veh_Health]      = 1000.0;
    g_Vehicles[idx][veh_EngineHealth]= 1000.0;
    g_Vehicles[idx][veh_PosX]        = pos_x;
    g_Vehicles[idx][veh_PosY]        = pos_y;
    g_Vehicles[idx][veh_PosZ]        = pos_z;
    g_Vehicles[idx][veh_PosA]        = pos_a;

    // Salvar no banco
    new escaped_plate[PLATE_LENGTH * 2];
    DB_Escape(plate, escaped_plate, sizeof(escaped_plate));

    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `vehicles` "
        "(`owner_char_id`, `model`, `plate`, `color1`, `color2`, "
        "`fuel`, `health`, `engine_health`, "
        "`pos_x`, `pos_y`, `pos_z`, `pos_a`, `spawned`, `locked`) "
        "VALUES (%d, %d, '%s', %d, %d, "
        "100.0, 1000.0, 1000.0, "
        "%.4f, %.4f, %.4f, %.4f, 1, 1)",
        owner_char_id, model, escaped_plate, color1, color2,
        pos_x, pos_y, pos_z, pos_a);

    SetPVarInt(0, "creating_veh_slot", idx); // hack temporario
    DB_Query(query, "OnVehicleCreatedDB", "i", idx);

    // Spawnar imediatamente
    Vehicle_Spawn(idx);
    if(idx >= g_VehicleCount) g_VehicleCount = idx + 1;
    return idx;
}

forward OnVehicleCreatedDB(slot);
public OnVehicleCreatedDB(slot) {
    if(slot < 0 || slot >= MAX_SPAWNED_VEHICLES) return 0;
    g_Vehicles[slot][veh_DBID] = DB_InsertID();
    return 1;
}

// ------------------------------------------------------------
// Trancar / Destrancar
// ------------------------------------------------------------
stock Vehicle_Lock(playerid, idx) {
    if(!IsValidPlayer(playerid) || idx < 0) return;
    if(g_Vehicles[idx][veh_OwnerCharID] != PlayerData[playerid][pCharacterID]) {
        UI_NotifyError(playerid, "Este veiculo nao e seu.");
        return;
    }
    g_Vehicles[idx][veh_Locked] = !g_Vehicles[idx][veh_Locked];
    g_Vehicles[idx][veh_Dirty]  = true;

    new sampid = g_Vehicles[idx][veh_SAMPVehicleID];
    if(IsValidVehicle(sampid)) {
        new state = g_Vehicles[idx][veh_Locked] ?
            VEHICLE_PARAMS_OFF : VEHICLE_PARAMS_ON;
        SetVehicleParamsEx(sampid, state, VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
            VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
            VEHICLE_PARAMS_UNSET);
    }

    new msg[48];
    format(msg, sizeof(msg),
        g_Vehicles[idx][veh_Locked] ?
            CSTR_WARNING"Veiculo trancado." :
            CSTR_SUCCESS"Veiculo destrancado.");
    SendClientMessage(playerid, COLOR_INFO, msg);
    GameTextForPlayer(playerid,
        g_Vehicles[idx][veh_Locked] ? "~r~Trancado" : "~g~Destrancado", 2000, 3);
}

// Encontrar veiculo mais proximo do jogador (owned)
stock Vehicle_FindNearestOwned(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new char_id = PlayerData[playerid][pCharacterID];
    new best = -1;
    new Float:best_dist = 10.0;

    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_OwnerCharID] != char_id) continue;
        if(!g_Vehicles[i][veh_Spawned]) continue;
        new sampid = g_Vehicles[i][veh_SAMPVehicleID];
        if(!IsValidVehicle(sampid)) continue;
        new Float:vx, Float:vy, Float:vz;
        GetVehiclePos(sampid, vx, vy, vz);
        new Float:dist = GetDistance3D(px, py, pz, vx, vy, vz);
        if(dist < best_dist) { best_dist = dist; best = i; }
    }
    return best;
}

// Encontrar qualquer veiculo proximo
stock Vehicle_FindNearest(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new best = -1;
    new Float:best_dist = 5.0;

    for(new i = 0; i < g_VehicleCount; i++) {
        if(!g_Vehicles[i][veh_Spawned]) continue;
        new sampid = g_Vehicles[i][veh_SAMPVehicleID];
        if(!IsValidVehicle(sampid)) continue;
        new Float:vx, Float:vy, Float:vz;
        GetVehiclePos(sampid, vx, vy, vz);
        new Float:dist = GetDistance3D(px, py, pz, vx, vy, vz);
        if(dist < best_dist) { best_dist = dist; best = i; }
    }
    return best;
}

// ------------------------------------------------------------
// COMBUSTIVEL
// ------------------------------------------------------------
stock Vehicle_GetFuel(idx) {
    if(idx < 0) return 0;
    return floatround(g_Vehicles[idx][veh_Fuel]);
}

stock Vehicle_SetFuel(idx, Float:fuel) {
    if(idx < 0) return;
    g_Vehicles[idx][veh_Fuel]  = ClampFloat(fuel, FUEL_MIN, FUEL_MAX);
    g_Vehicles[idx][veh_Dirty] = true;
}

stock Vehicle_AddFuel(idx, Float:amount) {
    if(idx < 0) return;
    g_Vehicles[idx][veh_Fuel]  = ClampFloat(g_Vehicles[idx][veh_Fuel] + amount,
                                             FUEL_MIN, FUEL_MAX);
    g_Vehicles[idx][veh_Dirty] = true;
}

// Consumo de combustivel (chamado pelo scheduler)
static Vehicle_UpdateFuel() {
    for(new p = 0; p < MAX_PLAYERS; p++) {
        if(!IsValidPlayer(p)) continue;
        new idx = g_PlayerVehicleSlot[p];
        if(idx < 0) continue;
        new sampid = g_Vehicles[idx][veh_SAMPVehicleID];
        if(!IsValidVehicle(sampid)) continue;

        // Calcular distancia percorrida
        new Float:vx, Float:vy, Float:vz;
        GetVehiclePos(sampid, vx, vy, vz);
        new Float:dist = GetDistance3D(
            g_LastVehPos[p][0], g_LastVehPos[p][1], g_LastVehPos[p][2],
            vx, vy, vz);

        g_LastVehPos[p][0] = vx;
        g_LastVehPos[p][1] = vy;
        g_LastVehPos[p][2] = vz;

        if(dist < 0.5) continue; // parado

        // Acumular km e consumir combustivel
        g_AccumDist[idx] += dist / 1000.0; // SA-MP usa metros, converter
        g_Vehicles[idx][veh_Mileage] += floatround(dist);

        // Consumo
        new Float:fuel_consume = dist * FUEL_DECAY_PER_KM / 1000.0;
        g_Vehicles[idx][veh_Fuel] -= fuel_consume;
        g_Vehicles[idx][veh_Dirty] = true;

        if(g_Vehicles[idx][veh_Fuel] <= FUEL_EMPTY_THRESHOLD) {
            if(g_Vehicles[idx][veh_Fuel] <= 0.0) {
                g_Vehicles[idx][veh_Fuel] = 0.0;
                // Desligar motor
                SetVehicleParamsEx(sampid,
                    VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_UNSET,
                    VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
                    VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
                    VEHICLE_PARAMS_UNSET);
                SendWarning(p, "Seu veiculo ficou sem combustivel!");
                GameTextForPlayer(p, "~r~Sem Combustivel!", 3000, 3);
            } else {
                // Aviso de combustivel baixo
                new msg[48];
                format(msg, sizeof(msg),
                    CSTR_WARNING"[!] Combustivel baixo: %.0f%%",
                    g_Vehicles[idx][veh_Fuel]);
                SendClientMessage(p, COLOR_WARNING, msg);
            }
        }
    }
}

// ------------------------------------------------------------
// Callbacks SA-MP
// ------------------------------------------------------------
public OnPlayerStateChange(playerid, newstate, oldstate) {
    if(newstate == PLAYER_STATE_DRIVER) {
        new sampvehid = GetPlayerVehicleID(playerid);
        new idx = Vehicle_GetSlotBySAMPID(sampvehid);
        g_PlayerVehicleSlot[playerid] = idx;
        if(idx >= 0) {
            // Verificar combustivel
            if(g_Vehicles[idx][veh_Fuel] <= 0.0) {
                SetVehicleParamsEx(sampvehid,
                    VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_UNSET,
                    VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
                    VEHICLE_PARAMS_UNSET, VEHICLE_PARAMS_UNSET,
                    VEHICLE_PARAMS_UNSET);
                SendWarning(playerid, "Este veiculo esta sem combustivel.");
            }
            // Salvar posicao inicial
            GetVehiclePos(sampvehid,
                g_LastVehPos[playerid][0],
                g_LastVehPos[playerid][1],
                g_LastVehPos[playerid][2]);
        }
    }
    else if(oldstate == PLAYER_STATE_DRIVER) {
        new idx = g_PlayerVehicleSlot[playerid];
        if(idx >= 0) {
            Vehicle_Save(idx);
        }
        g_PlayerVehicleSlot[playerid] = -1;
    }
    return 1;
}

public OnVehicleDeath(vehicleid, killerid) {
    new idx = Vehicle_GetSlotBySAMPID(vehicleid);
    if(idx >= 0) {
        g_Vehicles[idx][veh_Health] = 0.0;
        g_Vehicles[idx][veh_Dirty]  = true;
        Vehicle_Save(idx);
    }
    return 1;
}

// Scheduler: salvar veiculos e consumir combustivel a cada 10s
public OnScheduler10Seconds() {
    Vehicle_UpdateFuel();
}

public OnScheduler5Minutes() {
    Vehicle_SaveAll();
}

// ------------------------------------------------------------
// Menu do veiculo
// ------------------------------------------------------------
stock Vehicle_ShowMenu(playerid, idx) {
    if(!IsValidPlayer(playerid) || idx < 0) return;
    new is_owner = (g_Vehicles[idx][veh_OwnerCharID] == PlayerData[playerid][pCharacterID]);

    new model_name[32];
    format(model_name, sizeof(model_name), "Veiculo #%d", g_Vehicles[idx][veh_Model]);

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s ===\n\n"
        CSTR_WHITE"Placa: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Combustivel: "CSTR_YELLOW"%.0f%%\n"
        CSTR_WHITE"Saude: "CSTR_GREEN"%.0f%%\n"
        CSTR_WHITE"KM: "CSTR_GREY"%d\n"
        CSTR_WHITE"Status: %s\n",
        model_name,
        g_Vehicles[idx][veh_Plate],
        g_Vehicles[idx][veh_Fuel],
        g_Vehicles[idx][veh_Health] / 10.0,
        g_Vehicles[idx][veh_Mileage] / 1000,
        g_Vehicles[idx][veh_Locked] ?
            CSTR_ERROR"Trancado" : CSTR_SUCCESS"Destrancado");

    new options[128];
    if(is_owner) {
        format(options, sizeof(options),
            g_Vehicles[idx][veh_Locked] ? "Destrancar\n" : "Trancar\n");
        strcat(options, "Guardar na Garagem\n", sizeof(options));
        strcat(options, "Vender\n", sizeof(options));
    } else {
        strcat(options, "Ver Placa\n", sizeof(options));
    }

    SetPVarInt(playerid, "menu_veh_slot", idx);

    new full_text[512];
    format(full_text, sizeof(full_text), "%s\n%s", body, options);
    UI_ShowDialog(playerid, DIALOG_VEHICLE_MENU, DIALOG_STYLE_LIST,
        "{ Veiculo", options, "Selecionar", "Fechar");
}

// Handler de dialog do veiculo
stock Vehicle_HandleDialog(playerid, dialogid, response, listitem,
                                 inputtext[]) {
    switch(dialogid) {
        case DIALOG_VEHICLE_MENU: {
            if(!response) return true;
            new idx = GetPVarInt(playerid, "menu_veh_slot");
            new is_owner = (g_Vehicles[idx][veh_OwnerCharID] ==
                            PlayerData[playerid][pCharacterID]);
            if(is_owner) {
                switch(listitem) {
                    case 0: Vehicle_Lock(playerid, idx);
                    case 1: Garage_StoreVehicle(playerid, idx);
                    case 2: Vehicle_StartSell(playerid, idx);
                }
            }
            return true;
        }
        case DIALOG_VEHICLE_SELL: {
            if(!response) return true;
            new idx  = GetPVarInt(playerid, "selling_veh_slot");
            new buyer_id = strval(inputtext);
            if(!IsValidPlayer(buyer_id)) {
                UI_NotifyError(playerid, "Jogador nao encontrado.");
                return true;
            }
            SetPVarInt(playerid, "sell_to_player", buyer_id);
            new price_str[32];
            new veh_price = 15000; // placeholder ate concessionaria calcular
            FormatMoneyFull(veh_price, price_str, sizeof(price_str));
            new body[128];
            format(body, sizeof(body),
                "Vender seu veiculo para o ID %d por %s?",
                buyer_id, price_str);
            UI_ShowConfirm(playerid, "{ Confirmar Venda", body);
            return true;
        }
    }
    return false;
}

static Vehicle_StartSell(playerid, idx) {
    SetPVarInt(playerid, "selling_veh_slot", idx);
    UI_ShowDialog(playerid, DIALOG_VEHICLE_SELL, DIALOG_STYLE_INPUT,
        "{ Vender Veiculo",
        "Digite o ID do jogador para quem deseja vender:",
        "Confirmar", "Cancelar");
}

// ------------------------------------------------------------
// Getters publicos
// ------------------------------------------------------------
stock Vehicle_GetOwner(idx) {
    if(idx < 0) return 0;
    return g_Vehicles[idx][veh_OwnerCharID];
}

stock Vehicle_GetModel(idx) {
    if(idx < 0) return 0;
    return g_Vehicles[idx][veh_Model];
}

stock Vehicle_GetPlate(idx, output[], size = sizeof(output)) {
    if(idx < 0) return;
    SafeStrCopy(output, g_Vehicles[idx][veh_Plate], size);
}

stock Vehicle_IsLocked(idx) {
    if(idx < 0) return true;
    return g_Vehicles[idx][veh_Locked];
}

stock Vehicle_GetSAMPID(idx) {
    if(idx < 0) return INVALID_VEHICLE_ID;
    return g_Vehicles[idx][veh_SAMPVehicleID];
}

// Quantos veiculos o jogador tem
stock Vehicle_CountOwned(char_id) {
    new count = 0;
    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_OwnerCharID] == char_id) count++;
    }
    return count;
}

// ------------------------------------------------------------
// Comandos
// ------------------------------------------------------------

// /veiculo ? menu do veiculo mais proximo (ou o que esta dirigindo)
CMD:veiculo(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new sampvehid = GetPlayerVehicleID(playerid);
    new idx = -1;

    if(sampvehid > 0 && GetPlayerState(playerid) == PLAYER_STATE_DRIVER) {
        idx = Vehicle_GetSlotBySAMPID(sampvehid);
    } else {
        idx = Vehicle_FindNearestOwned(playerid);
        if(idx < 0) idx = Vehicle_FindNearest(playerid);
    }

    if(idx < 0) {
        SendError(playerid, "Nenhum veiculo proximo.");
        return 1;
    }
    Vehicle_ShowMenu(playerid, idx);
    return 1;
}

// /trancar ? trancar/destrancar veiculo mais proximo
CMD:trancar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new idx = Vehicle_FindNearestOwned(playerid);
    if(idx < 0) {
        SendError(playerid, "Nenhum veiculo seu proximo.");
        return 1;
    }
    Vehicle_Lock(playerid, idx);
    return 1;
}

// /meusvehiculos ? listar veiculos do jogador
CMD:meusveiculos(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new char_id = PlayerData[playerid][pCharacterID];
    new found = false;

    UI_Separator(playerid);
    UI_Header(playerid, "Meus Veiculos");

    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_OwnerCharID] != char_id) continue;
        new msg[128];
        format(msg, sizeof(msg),
            CSTR_WHITE"Placa: "CSTR_SYSTEM"%s  "
            CSTR_WHITE"Combustivel: "CSTR_YELLOW"%.0f%%  "
            CSTR_WHITE"Status: %s",
            g_Vehicles[i][veh_Plate],
            g_Vehicles[i][veh_Fuel],
            g_Vehicles[i][veh_Spawned] ?
                (g_Vehicles[i][veh_Locked] ? CSTR_ERROR"Trancado" : CSTR_SUCCESS"Destrancado")
                : CSTR_GREY"Na garagem");
        SendClientMessage(playerid, COLOR_INFO, msg);
        found = true;
    }

    if(!found) SendInfo(playerid, "Voce nao possui veiculos.");
    UI_Separator(playerid);
    return 1;
}
