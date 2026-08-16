// ============================================================
//  Santa Aurora Roleplay ? Garagem
//  Modulo : houses/garage_core.inc
//  Objetivo: guardar/retirar veiculos na propriedade, garagem
//            publica, limite de vagas
// ============================================================

#if defined _SA_GARAGE_CORE_INC
    #endinput
#endif
#define _SA_GARAGE_CORE_INC

// include removido
// include removido

// Dialogs
#define DIALOG_GARAGE_LIST      920
#define DIALOG_GARAGE_RETRIEVE  921

// ------------------------------------------------------------
// Guardar veiculo na garagem da propriedade
// ------------------------------------------------------------
stock Garage_StoreVehicle(playerid, veh_idx) {
    if(!IsValidPlayer(playerid) || veh_idx < 0) return;

    // Confirmar que e dono do veiculo
    if(g_Vehicles[veh_idx][veh_OwnerCharID] != PlayerData[playerid][pCharacterID]) {
        UI_NotifyError(playerid, "Este veiculo nao e seu.");
        return;
    }

    // Encontrar propriedade com garagem
    new prop_idx = -1;

    // Verificar se esta dentro de uma propriedade ou perto de uma
    new inside = Property_GetPlayerInside(playerid);
    if(inside >= 0 && Property_GetGarageSlots(inside) > 0 &&
       Property_GetOwner(inside) == PlayerData[playerid][pCharacterID]) {
        prop_idx = inside;
    } else {
        // Procurar propriedade do dono com garagem
        new tmp = -1;
        do {
            tmp = Property_FindByOwner(PlayerData[playerid][pCharacterID], tmp + 1);
            if(tmp >= 0 && Property_GetGarageSlots(tmp) > 0) {
                prop_idx = tmp;
                break;
            }
        } while(tmp >= 0);
    }

    if(prop_idx < 0) {
        UI_NotifyError(playerid, "Voce nao tem garagem disponivel.");
        return;
    }

    // Contar veiculos ja guardados
    new stored = 0;
    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_OwnerCharID] == PlayerData[playerid][pCharacterID] &&
           g_Vehicles[i][veh_GarageID] == g_Properties[prop_idx][prop_ID]) {
            stored++;
        }
    }
    new slots = Property_GetGarageSlots(prop_idx);
    if(stored >= slots) {
        new msg[64];
        format(msg, sizeof(msg), "Garagem cheia (%d/%d vagas).", stored, slots);
        UI_NotifyError(playerid, msg);
        return;
    }

    // Guardar
    g_Vehicles[veh_idx][veh_GarageID] = g_Properties[prop_idx][prop_ID];

    // Atualizar banco
    new query[128];
    format(query, sizeof(query),
        "UPDATE `vehicles` SET `garage_id`=%d, `spawned`=0 WHERE `id`=%d",
        g_Properties[prop_idx][prop_ID],
        g_Vehicles[veh_idx][veh_DBID]);
    DB_Execute(query);

    // Desapawnar do mundo
    Vehicle_Despawn(veh_idx);

    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Veiculo guardado na garagem "CSTR_SYSTEM"%s"CSTR_SUCCESS"! (%d/%d)",
        g_Properties[prop_idx][prop_Name], stored + 1, slots);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
}

// ------------------------------------------------------------
// Mostrar veiculos na garagem
// ------------------------------------------------------------
stock Garage_ShowList(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new char_id = PlayerData[playerid][pCharacterID];

    UI_ListReset();
    new found = false;

    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_OwnerCharID] != char_id) continue;
        if(g_Vehicles[i][veh_GarageID] == 0) continue; // nao esta na garagem

        new line[80];
        format(line, sizeof(line),
            "Placa: "CSTR_SYSTEM"%s"CSTR_WHITE"  Combustivel: %.0f%%",
            g_Vehicles[i][veh_Plate],
            g_Vehicles[i][veh_Fuel]);
        UI_ListAdd(line);
        found = true;
    }

    if(!found) {
        UI_ListAdd("Nenhum veiculo na garagem.");
    }

    UI_ListShow(playerid, DIALOG_GARAGE_LIST, "{ Garagem ? Meus Veiculos",
        "Retirar", "Fechar");
}

// ------------------------------------------------------------
// Retirar veiculo da garagem
// ------------------------------------------------------------
stock Garage_RetrieveVehicle(playerid, list_index) {
    if(!IsValidPlayer(playerid)) return;
    new char_id = PlayerData[playerid][pCharacterID];

    // Encontrar o veiculo pelo indice na lista
    new count = 0, found_veh = -1;
    for(new i = 0; i < g_VehicleCount; i++) {
        if(g_Vehicles[i][veh_OwnerCharID] != char_id) continue;
        if(g_Vehicles[i][veh_GarageID] == 0) continue;
        if(count == list_index) { found_veh = i; break; }
        count++;
    }

    if(found_veh < 0) {
        UI_NotifyError(playerid, "Veiculo nao encontrado.");
        return;
    }

    // Verificar se ja esta spawned
    if(g_Vehicles[found_veh][veh_Spawned]) {
        UI_NotifyWarning(playerid, "Este veiculo ja esta no mundo.");
        return;
    }

    // Spawnar perto do jogador
    new Float:px, Float:py, Float:pz, Float:pa;
    GetPlayerPos(playerid, px, py, pz);
    GetPlayerFacingAngle(playerid, pa);

    g_Vehicles[found_veh][veh_PosX]    = px + 3.0;
    g_Vehicles[found_veh][veh_PosY]    = py;
    g_Vehicles[found_veh][veh_PosZ]    = pz;
    g_Vehicles[found_veh][veh_PosA]    = pa;
    g_Vehicles[found_veh][veh_GarageID]= 0;

    // Atualizar banco
    new query[128];
    format(query, sizeof(query),
        "UPDATE `vehicles` SET `garage_id`=NULL, `spawned`=1, "
        "`pos_x`=%.4f, `pos_y`=%.4f, `pos_z`=%.4f WHERE `id`=%d",
        px + 3.0, py, pz,
        g_Vehicles[found_veh][veh_DBID]);
    DB_Execute(query);

    Vehicle_Spawn(found_veh);

    new msg[64];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Veiculo "CSTR_SYSTEM"%s"CSTR_SUCCESS" retirado da garagem.",
        g_Vehicles[found_veh][veh_Plate]);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
}

// Handler
stock Garage_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_GARAGE_LIST: {
            if(!response) return true;
            Garage_RetrieveVehicle(playerid, listitem);
            return true;
        }
    }
    return false;
}

// Comandos
CMD:garagem(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Garage_ShowList(playerid);
    return 1;
}

CMD:guardar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    // Guardar veiculo que esta dirigindo ou mais proximo
    new veh_idx = -1;
    if(GetPlayerState(playerid) == PLAYER_STATE_DRIVER) {
        new sampvehid = GetPlayerVehicleID(playerid);
        veh_idx = Vehicle_GetSlotBySAMPID(sampvehid);
    } else {
        veh_idx = Vehicle_FindNearestOwned(playerid);
    }
    if(veh_idx < 0) {
        SendError(playerid, "Nenhum veiculo seu para guardar.");
        return 1;
    }
    Garage_StoreVehicle(playerid, veh_idx);
    return 1;
}
