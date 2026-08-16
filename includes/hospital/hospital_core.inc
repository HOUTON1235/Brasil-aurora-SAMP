// ============================================================
//  Santa Aurora Roleplay — Sistema Hospitalar
//  Módulo : hospital/hospital_core.inc
//  Objetivo: SAMU, revive, tratamento, internação, prontuário,
//            chamados médicos, respeitar o estado de morte
// ============================================================

#if defined _SA_HOSPITAL_CORE_INC
    #endinput
#endif
#define _SA_HOSPITAL_CORE_INC

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
#define HOSPITAL_REVIVE_COST        500
#define HOSPITAL_TREATMENT_PER_HP   50      // custo por HP restaurado
#define HOSPITAL_RESPAWN_TIME       30      // segundos antes do respawn forçado
#define HOSPITAL_RANGE              5.0
#define MAX_ACTIVE_CALLS            20

// Posição do hospital principal (Santa Aurora Hospital)
#define HOSPITAL_POS_X      1174.8
#define HOSPITAL_POS_Y      -1323.9
#define HOSPITAL_POS_Z      13.6
#define HOSPITAL_INT_X      1172.0
#define HOSPITAL_INT_Y      -1323.0
#define HOSPITAL_INT_Z      14.1
#define HOSPITAL_INTERIOR   0
#define HOSPITAL_WORLD      0

// Dialogs hospitalares
#define DIALOG_HOSP_MENU        1300
#define DIALOG_HOSP_TREATMENT   1301
#define DIALOG_HOSP_RECORDS     1302
#define DIALOG_HOSP_CALL_LIST   1303

// ------------------------------------------------------------
// Estado do jogador morto
// ------------------------------------------------------------
enum E_DEAD_STATE {
    dead_IsDead,
    dead_DeathTime,         // timestamp
    dead_RespawnTimer,      // timer ID
    dead_CallID,            // ID do chamado médico
    dead_AttendingMedic     // playerid do médico
}

static g_DeadState[MAX_PLAYERS][E_DEAD_STATE];

// Cache de chamados médicos ativos
enum E_MEDICAL_CALL {
    mcall_ID,
    mcall_PatientID,
    mcall_PatientName[MAX_PLAYER_NAME],
    Float:mcall_PosX,
    Float:mcall_PosY,
    Float:mcall_PosZ,
    mcall_Status,           // 0=pendente, 1=atendendo, 2=fechado
    mcall_MedicID
}

static g_ActiveCalls[MAX_ACTIVE_CALLS][E_MEDICAL_CALL];
static g_ActiveCallCount = 0;

// ------------------------------------------------------------
// Inicializar estado de morte
// ------------------------------------------------------------
stock Hospital_InitPlayer(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_DeadState[playerid][dead_IsDead]        = 0;
    g_DeadState[playerid][dead_DeathTime]     = 0;
    g_DeadState[playerid][dead_RespawnTimer]  = -1;
    g_DeadState[playerid][dead_CallID]        = -1;
    g_DeadState[playerid][dead_AttendingMedic]= INVALID_PLAYER_ID;
}

// ------------------------------------------------------------
// Verificar se é médico ativo
// ------------------------------------------------------------
stock bool:Hospital_IsMedic(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return (PlayerData[playerid][pJobID] == 13 && // JOB_MEDIC
            Player_GetState(playerid) == STATE_WORKING);
}

// ------------------------------------------------------------
// JOGADOR MORREU — abrir chamado e iniciar timer de respawn
// ------------------------------------------------------------
stock Hospital_OnPlayerDied(playerid, killerid) {
    if(!IsValidPlayer(playerid)) return;

    g_DeadState[playerid][dead_IsDead]    = 1;
    g_DeadState[playerid][dead_DeathTime] = gettime();

    Player_SetState(playerid, STATE_DEAD);

    // Exibir mensagem
    SendClientMessage(playerid, COLOR_RED,
        CSTR_ERROR"[MORTE] "CSTR_WHITE"Você está ferido. Aguarde o SAMU ou respawn automático.");
    new time_str[16];
    format(time_str, sizeof(time_str), "~r~Ferido! ~w~%ds", HOSPITAL_RESPAWN_TIME);
    GameTextForPlayer(playerid, time_str, 5000, 3);

    // Criar chamado SAMU
    Hospital_CreateCall(playerid);

    // Timer de respawn forçado
    g_DeadState[playerid][dead_RespawnTimer] =
        SetTimerEx("Hospital_ForceRespawn", HOSPITAL_RESPAWN_TIME * 1000,
                   false, "i", playerid);

    // Notificar médicos online
    Hospital_BroadcastToMedics(playerid);

    #pragma unused killerid
}

// Broadcast para médicos
static Hospital_BroadcastToMedics(playerid) {
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);

    new msg[128];
    format(msg, sizeof(msg),
        CSTR_ERROR"[SAMU] "CSTR_WHITE"%s precisa de atendimento médico! "
        CSTR_GREY"Pos: %.0f, %.0f",
        name, px, py);

    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(Hospital_IsMedic(i)) {
            SendClientMessage(i, COLOR_RED, msg);
            GameTextForPlayer(i, "~r~Chamado SAMU!", 3000, 3);
        }
    }
}

// ------------------------------------------------------------
// Respawn forçado (se nenhum médico atendeu)
// ------------------------------------------------------------
forward Hospital_ForceRespawn(playerid);
public Hospital_ForceRespawn(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    if(!g_DeadState[playerid][dead_IsDead]) return 0;

    Hospital_Respawn(playerid, false);
    return 1;
}

// Efetuar respawn
stock Hospital_Respawn(playerid, bool:by_medic) {
    if(!IsValidPlayer(playerid)) return;

    // Cancelar timer pendente
    if(g_DeadState[playerid][dead_RespawnTimer] != -1) {
        KillTimer(g_DeadState[playerid][dead_RespawnTimer]);
        g_DeadState[playerid][dead_RespawnTimer] = -1;
    }

    // Fechar chamado
    Hospital_CloseCall(playerid);

    g_DeadState[playerid][dead_IsDead] = 0;

    // Cobrar se foi sem médico
    if(!by_medic) {
        new cost = HOSPITAL_REVIVE_COST;
        new bool:charged = false;
        if(PlayerData[playerid][pMoney] >= cost) {
            Player_RemoveMoney(playerid, cost);
            charged = true;
        } else if(Bank_GetBalance(playerid) >= cost) {
            Bank_Debit(playerid, cost, "purchase", "Internação hospitalar");
            charged = true;
        }
        if(charged) {
            new fmt[24]; FormatMoneyFull(cost, fmt, sizeof(fmt));
            new msg[80];
            format(msg, sizeof(msg),
                CSTR_WARNING"[HOSPITAL] "CSTR_WHITE"Internação: "CSTR_ERROR"%s"
                CSTR_WHITE" debitado.", fmt);
            SendClientMessage(playerid, COLOR_WARNING, msg);
        }

        // Registrar no prontuário
        new query[256];
        format(query, sizeof(query),
            "INSERT INTO `medical_records` "
            "(`character_id`, `type`, `description`, `cost`) "
            "VALUES (%d, 'hospitalization', 'Internação automática', %d)",
            PlayerData[playerid][pCharacterID], HOSPITAL_REVIVE_COST);
        DB_Execute(query);
    }

    // Spawnar no hospital
    Player_SetState(playerid, STATE_SPAWNED);
    SetPlayerHealth(playerid, 100.0);
    SetPlayerArmour(playerid, 0.0);
    SetSpawnInfo(playerid, 0, PlayerData[playerid][pSkin],
        HOSPITAL_INT_X, HOSPITAL_INT_Y, HOSPITAL_INT_Z,
        0.0, 0, 0, 0, 0, 0, 0);
    SpawnPlayer(playerid);
    SetPlayerInterior(playerid, HOSPITAL_INTERIOR);
    SetPlayerVirtualWorld(playerid, HOSPITAL_WORLD);

    PlayerData[playerid][pHealth] = 100.0;
    Player_MarkDirty(playerid);

    SendSuccess(playerid,
        CSTR_SUCCESS"[HOSPITAL] "CSTR_WHITE"Você foi atendido e recebeu alta. Cuide-se!");
}

// ------------------------------------------------------------
// REVIVE pelo médico
// ------------------------------------------------------------
stock Hospital_Revive(medicId, patientId) {
    if(!IsValidPlayer(medicId) || !IsValidPlayer(patientId)) return;
    if(!Hospital_IsMedic(medicId)) {
        UI_NotifyError(medicId, "Apenas médicos em serviço podem reviver pacientes.");
        return;
    }
    if(!g_DeadState[patientId][dead_IsDead]) {
        UI_NotifyError(medicId, "Este jogador não está ferido.");
        return;
    }
    if(!IsPlayerInRange3D(medicId,
        g_hospGetX(patientId), g_hospGetY(patientId), g_hospGetZ(patientId),
        HOSPITAL_RANGE)) {
        UI_NotifyError(medicId, "Muito longe do paciente.");
        return;
    }

    // Animação de atendimento
    ApplyAnimation(medicId, "MEDIC", "CPR", 4.0, false, false, false, false, 5000);

    // Curar após animação
    SetTimerEx("Hospital_FinishRevive", 5000, false, "ii", medicId, patientId);
    g_DeadState[patientId][dead_AttendingMedic] = medicId;

    new patient_name[MAX_PLAYER_NAME];
    GetPlayerName(patientId, patient_name, sizeof(patient_name));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"[SAMU] "CSTR_WHITE"Atendendo %s...", patient_name);
    SendClientMessage(medicId, COLOR_SUCCESS, msg);
    SendInfo(patientId, "Um médico está te atendendo. Aguarde...");
}

forward Hospital_FinishRevive(medicId, patientId);
public Hospital_FinishRevive(medicId, patientId) {
    if(!IsValidPlayer(patientId)) return 0;
    if(!g_DeadState[patientId][dead_IsDead]) return 0;

    // Registrar no prontuário
    new medic_char_id = IsValidPlayer(medicId) ? PlayerData[medicId][pCharacterID] : 0;
    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `medical_records` "
        "(`character_id`, `medic_id`, `type`, `description`, `cost`) "
        "VALUES (%d, %d, 'revive', 'Revivido pelo SAMU', 0)",
        PlayerData[patientId][pCharacterID], medic_char_id);
    DB_Execute(query);

    // Dar XP ao médico
    if(IsValidPlayer(medicId)) {
        // Chama o sistema de empregos
        CallLocalFunction("Jobs_OnAction_External", "ii", medicId, 100);

        new patient_name[MAX_PLAYER_NAME];
        GetPlayerName(patientId, patient_name, sizeof(patient_name));
        new msg[64];
        format(msg, sizeof(msg),
            CSTR_SUCCESS"[SAMU] "CSTR_WHITE"%s foi revivido!", patient_name);
        SendClientMessage(medicId, COLOR_SUCCESS, msg);
    }

    Hospital_Respawn(patientId, true);
    return 1;
}

// Helpers de posição
static Float:g_hospGetX(playerid) { new Float:x,Float:y,Float:z; GetPlayerPos(playerid,x,y,z); return x; }
static Float:g_hospGetY(playerid) { new Float:x,Float:y,Float:z; GetPlayerPos(playerid,x,y,z); return y; }
static Float:g_hospGetZ(playerid) { new Float:x,Float:y,Float:z; GetPlayerPos(playerid,x,y,z); return z; }

// ------------------------------------------------------------
// TRATAMENTO no hospital (se for sozinho, sem médico)
// ------------------------------------------------------------
stock Hospital_Treatment(playerid, hp_amount) {
    if(!IsValidPlayer(playerid)) return;
    if(g_DeadState[playerid][dead_IsDead]) {
        UI_NotifyError(playerid, "Você precisa ser revivido primeiro.");
        return;
    }

    new Float:current_hp;
    GetPlayerHealth(playerid, current_hp);
    new hp_needed = floatround(100.0 - current_hp);
    if(hp_needed <= 0) {
        UI_NotifyInfo(playerid, "Você já está com saúde plena.");
        return;
    }
    if(hp_amount > hp_needed) hp_amount = hp_needed;

    new cost = hp_amount * HOSPITAL_TREATMENT_PER_HP;
    new bool:paid = false;
    if(PlayerData[playerid][pMoney] >= cost) {
        Player_RemoveMoney(playerid, cost);
        paid = true;
    } else if(Bank_GetBalance(playerid) >= cost) {
        Bank_Debit(playerid, cost, "purchase", "Tratamento hospitalar");
        paid = true;
    }
    if(!paid) {
        UI_NotifyError(playerid, "Saldo insuficiente para o tratamento.");
        return;
    }

    new Float:new_hp = current_hp + float(hp_amount);
    if(new_hp > 100.0) new_hp = 100.0;
    SetPlayerHealth(playerid, new_hp);
    PlayerData[playerid][pHealth] = new_hp;
    Player_MarkDirty(playerid);

    new cost_fmt[24];
    FormatMoneyFull(cost, cost_fmt, sizeof(cost_fmt));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"[HOSPITAL] "CSTR_WHITE"+%d HP curado por "CSTR_MONEY"%s"CSTR_SUCCESS".",
        hp_amount, cost_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);

    // Registrar prontuário
    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `medical_records` (`character_id`, `type`, `description`, `cost`) "
        "VALUES (%d, 'treatment', 'Tratamento ambulatorial %dHP', %d)",
        PlayerData[playerid][pCharacterID], hp_amount, cost);
    DB_Execute(query);

    Logger_Economy(playerid, "HOSPITAL_TREATMENT", cost,
        PlayerData[playerid][pName], "Hospital");
}

// ------------------------------------------------------------
// CHAMADOS MÉDICOS
// ------------------------------------------------------------
stock Hospital_CreateCall(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(g_ActiveCallCount >= MAX_ACTIVE_CALLS) return;

    new idx = g_ActiveCallCount;
    g_ActiveCalls[idx][mcall_PatientID] = playerid;
    GetPlayerName(playerid,
        g_ActiveCalls[idx][mcall_PatientName],
        sizeof(g_ActiveCalls[][mcall_PatientName]));
    GetPlayerPos(playerid,
        g_ActiveCalls[idx][mcall_PosX],
        g_ActiveCalls[idx][mcall_PosY],
        g_ActiveCalls[idx][mcall_PosZ]);
    g_ActiveCalls[idx][mcall_Status] = 0;
    g_ActiveCalls[idx][mcall_MedicID]= INVALID_PLAYER_ID;
    g_ActiveCalls[idx][mcall_ID]     = idx;
    g_ActiveCallCount++;

    // Salvar no banco
    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `medical_calls` (`character_id`, `pos_x`, `pos_y`, `pos_z`, `status`) "
        "VALUES (%d, %.4f, %.4f, %.4f, 'pending')",
        PlayerData[playerid][pCharacterID],
        g_ActiveCalls[idx][mcall_PosX],
        g_ActiveCalls[idx][mcall_PosY],
        g_ActiveCalls[idx][mcall_PosZ]);
    DB_Execute(query);
}

stock Hospital_CloseCall(playerid) {
    for(new i = 0; i < g_ActiveCallCount; i++) {
        if(g_ActiveCalls[i][mcall_PatientID] == playerid) {
            // Remover do array (swap com último)
            g_ActiveCalls[i] = g_ActiveCalls[g_ActiveCallCount - 1];
            g_ActiveCallCount--;
            break;
        }
    }
    DB_Execute("UPDATE `medical_calls` SET `status`='closed' WHERE `status`='pending'");
}

// Mostrar lista de chamados para médicos
stock Hospital_ShowCallList(playerid) {
    if(!Hospital_IsMedic(playerid)) {
        UI_NotifyError(playerid, "Apenas médicos em serviço podem ver chamados.");
        return;
    }
    if(g_ActiveCallCount == 0) {
        UI_NotifyInfo(playerid, "Nenhum chamado médico ativo no momento.");
        return;
    }

    UI_ListReset();
    for(new i = 0; i < g_ActiveCallCount; i++) {
        new line[80];
        new Float:px, Float:py, Float:pz;
        GetPlayerPos(playerid, px, py, pz);
        new Float:dist = GetDistance3D(px, py, pz,
            g_ActiveCalls[i][mcall_PosX],
            g_ActiveCalls[i][mcall_PosY],
            g_ActiveCalls[i][mcall_PosZ]);
        format(line, sizeof(line), "[%d] %s  %.0fm de distância",
            i + 1,
            g_ActiveCalls[i][mcall_PatientName],
            dist);
        UI_ListAdd(line);
    }
    UI_ListShow(playerid, DIALOG_HOSP_CALL_LIST,
        "{ Chamados SAMU", "Atender", "Fechar");
}

// ------------------------------------------------------------
// MENU DO HOSPITAL
// ------------------------------------------------------------
stock Hospital_ShowMenu(playerid) {
    if(!IsValidPlayer(playerid)) return;

    new Float:hp;
    GetPlayerHealth(playerid, hp);
    new Float:arm;
    GetPlayerArmour(playerid, arm);

    new body[256];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Santa Aurora Hospital ===\n\n"
        CSTR_WHITE"Saúde atual: "CSTR_GREEN"%.0f%%\n"
        CSTR_WHITE"Colete atual: "CSTR_BLUE"%.0f%%\n\n"
        CSTR_WHITE"Tratamento: "CSTR_MONEY"$%d/HP\n"
        CSTR_WHITE"Internação (respawn): "CSTR_MONEY"$%d",
        hp, arm,
        HOSPITAL_TREATMENT_PER_HP,
        HOSPITAL_REVIVE_COST);

    new options[] =
        "Tratamento Completo (100 HP)\n"
        "Tratamento Parcial (50 HP)\n"
        "Ver Prontuário\n";

    new full_options[256];
    format(full_options, sizeof(full_options), "%s", options);
    if(Hospital_IsMedic(playerid)) {
        strcat(full_options, "Chamados SAMU\n", sizeof(full_options));
    }

    UI_ShowDialog(playerid, DIALOG_HOSP_MENU, DIALOG_STYLE_LIST,
        "{ Hospital", full_options, "Selecionar", "Fechar");
}

// Prontuário
stock Hospital_ShowRecords(playerid) {
    new query[256];
    format(query, sizeof(query),
        "SELECT `type`, `description`, `cost`, "
        "DATE_FORMAT(`created_at`, '%%d/%%m/%%Y %%H:%%i') as date "
        "FROM `medical_records` WHERE `character_id`=%d "
        "ORDER BY `created_at` DESC LIMIT 10",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnHospitalRecordsLoad", "i", playerid);
}

forward OnHospitalRecordsLoad(playerid);
public OnHospitalRecordsLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new rows = DB_NumRows();
    new body[1024];
    format(body, sizeof(body), CSTR_SYSTEM"=== Prontuário Médico ===\n\n");

    if(rows == 0) {
        strcat(body, CSTR_SUCCESS"Nenhum registro médico.", sizeof(body));
    } else {
        for(new r = 0; r < rows; r++) {
            new type[20], desc[128], date[20];
            new cost = DB_GetInt("cost", r);
            DB_GetString("type",        type, sizeof(type), r);
            DB_GetString("description", desc, sizeof(desc), r);
            DB_GetString("date",        date, sizeof(date), r);
            new cost_fmt[24];
            FormatMoneyFull(cost, cost_fmt, sizeof(cost_fmt));
            new line[128];
            format(line, sizeof(line),
                CSTR_GREY"[%s] "CSTR_WHITE"%s: %s "CSTR_GREY"Custo: %s\n",
                date, type, desc, cost_fmt);
            strcat(body, line, sizeof(body));
        }
    }
    UI_ShowDialog(playerid, DIALOG_HOSP_RECORDS, DIALOG_STYLE_MSGBOX,
        "{ Prontuário", body, "Fechar", "");
    return 1;
}

// Handler de dialogs
stock bool:Hospital_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_HOSP_MENU: {
            if(!response) return true;
            new Float:hp;
            GetPlayerHealth(playerid, hp);
            new hp_needed = floatround(100.0 - hp);
            switch(listitem) {
                case 0: Hospital_Treatment(playerid, hp_needed);      // 100 HP
                case 1: Hospital_Treatment(playerid, 50);              // 50 HP
                case 2: Hospital_ShowRecords(playerid);
                case 3: {
                    if(Hospital_IsMedic(playerid)) {
                        Hospital_ShowCallList(playerid);
                    }
                }
            }
            return true;
        }
        case DIALOG_HOSP_CALL_LIST: {
            if(!response) return true;
            if(listitem >= g_ActiveCallCount) return true;
            new patientid = g_ActiveCalls[listitem][mcall_PatientID];
            if(!IsValidPlayer(patientid)) {
                UI_NotifyError(playerid, "Paciente desconectou.");
                return true;
            }
            Hospital_Revive(playerid, patientid);
            return true;
        }
    }
    return false;
}

// Integração com evento de morte
public OnPlayerDied(playerid, killerid) {
    Hospital_OnPlayerDied(playerid, killerid);
    return 1;
}

public OnPlayerLoaded(playerid) {
    Hospital_InitPlayer(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    // Se morto, cancelar respawn timer
    if(g_DeadState[playerid][dead_RespawnTimer] != -1) {
        KillTimer(g_DeadState[playerid][dead_RespawnTimer]);
    }
    Hospital_CloseCall(playerid);
    Hospital_InitPlayer(playerid);
    return 1;
}

// Comandos
CMD:hospital(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Hospital_ShowMenu(playerid);
    return 1;
}

CMD:reviver(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /reviver [id]");
        return 1;
    }
    Hospital_Revive(playerid, targetid);
    return 1;
}

CMD:samu(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!Hospital_IsMedic(playerid)) {
        // Jogador comum: pedir SAMU para si mesmo
        if(g_DeadState[playerid][dead_IsDead]) {
            Hospital_BroadcastToMedics(playerid);
            SendInfo(playerid, "Pedido de socorro enviado ao SAMU!");
        } else {
            SendInfo(playerid, "Use /hospital para tratamento. O SAMU é acionado automaticamente quando morto.");
        }
        return 1;
    }
    // Médico: ver chamados
    Hospital_ShowCallList(playerid);
    return 1;
}

CMD:curar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!Hospital_IsMedic(playerid) && !Player_IsAdmin(playerid, ADMIN_ADMIN)) {
        SendError(playerid, "Apenas médicos em serviço.");
        return 1;
    }
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /curar [id]");
        return 1;
    }
    SetPlayerHealth(targetid, 100.0);
    PlayerData[targetid][pHealth] = 100.0;
    Player_MarkDirty(targetid);
    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new msg[64];
    format(msg, sizeof(msg), CSTR_SUCCESS"%s foi curado.", target_name);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    SendSuccess(targetid, "Você foi curado!");
    return 1;
}

CMD:prontuario(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Hospital_ShowRecords(playerid);
    return 1;
}
