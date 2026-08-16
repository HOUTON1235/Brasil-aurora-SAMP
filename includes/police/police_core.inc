// ============================================================
//  Santa Aurora Roleplay — Sistema Policial
//  Módulo : police/police_core.inc
//  Objetivo: abordagem, identificação, algemas, prisão,
//            antecedentes, procurados, multas, apreensão
// ============================================================

#if defined _SA_POLICE_CORE_INC
    #endinput
#endif
#define _SA_POLICE_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../core/events.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "../players/player_core.inc"
#include "../economy/fines_core.inc"

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_WANTED_LEVEL        6
#define ARREST_RANGE            3.0
#define HANDCUFF_RANGE          2.5
#define SEARCH_RANGE            2.0
#define JAIL_DEFAULT_TIME       5       // minutos
#define DISPATCH_RANGE          500.0

// Dialogs de polícia
#define DIALOG_POLICE_MENU      1200
#define DIALOG_POLICE_ARREST    1201
#define DIALOG_POLICE_CRIMES    1202
#define DIALOG_POLICE_RECORDS   1203
#define DIALOG_POLICE_FINE      1204
#define DIALOG_POLICE_DISPATCH  1205

// Posição da cadeia
#define JAIL_POS_X      264.7
#define JAIL_POS_Y      77.8
#define JAIL_POS_Z      1001.0
#define JAIL_INTERIOR   6
#define JAIL_WORLD      0

// ------------------------------------------------------------
// Estado policial do jogador
// ------------------------------------------------------------
enum E_POLICE_STATE {
    pol_IsHandcuffed,       // 0/1
    pol_HandcuffedBy,       // playerid
    pol_IsArrested,         // está preso
    pol_JailTime,           // tempo restante em minutos
    pol_WantedLevel,        // 0-6
    pol_WantedReason[64],
    pol_LastWantedOfficer   // playerid
}

static g_PoliceState[MAX_PLAYERS][E_POLICE_STATE];

// Cache de crimes carregados do banco
#define MAX_CRIME_TYPES     20
enum E_CRIME_DEF {
    cdef_ID,
    cdef_Name[32],
    cdef_DisplayName[48],
    cdef_Gravity,
    cdef_Fine,
    cdef_JailTime,
    cdef_WantedLevel
}
static g_CrimeDefs[MAX_CRIME_TYPES][E_CRIME_DEF];
static g_CrimeDefCount = 0;

// Estado de abordagem: quem está abordando quem
static g_SuspectTarget[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};

// ------------------------------------------------------------
// Verificar se jogador é policial ativo
// ------------------------------------------------------------
stock bool:Police_IsOfficer(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return (PlayerData[playerid][pJobID] == 12 && // JOB_POLICE
            Player_GetState(playerid) == STATE_WORKING);
}

// ------------------------------------------------------------
// Carregar catálogo de crimes
// ------------------------------------------------------------
stock Police_LoadCrimeCatalog() {
    DB_Query(
        "SELECT `id`, `name`, `display_name`, `gravity`, "
        "`fine`, `jail_time`, `wanted_level` "
        "FROM `crime_catalog` ORDER BY `gravity`, `id`",
        "OnCrimeCatalogLoaded", "");
}

forward OnCrimeCatalogLoaded();
public OnCrimeCatalogLoaded() {
    g_CrimeDefCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_CrimeDefCount < MAX_CRIME_TYPES; r++) {
        new i = g_CrimeDefCount;
        g_CrimeDefs[i][cdef_ID]        = DB_GetInt("id",          r);
        DB_GetString("name",         g_CrimeDefs[i][cdef_Name],        sizeof(g_CrimeDefs[][cdef_Name]),        r);
        DB_GetString("display_name", g_CrimeDefs[i][cdef_DisplayName], sizeof(g_CrimeDefs[][cdef_DisplayName]), r);
        g_CrimeDefs[i][cdef_Gravity]   = DB_GetInt("gravity",     r);
        g_CrimeDefs[i][cdef_Fine]      = DB_GetInt("fine",        r);
        g_CrimeDefs[i][cdef_JailTime]  = DB_GetInt("jail_time",   r);
        g_CrimeDefs[i][cdef_WantedLevel] = DB_GetInt("wanted_level", r);
        g_CrimeDefCount++;
    }
    Logger_Info("POLICE", "Catálogo de crimes carregado.");
}

// ------------------------------------------------------------
// Inicializar estado policial do jogador
// ------------------------------------------------------------
stock Police_InitPlayer(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_PoliceState[playerid][pol_IsHandcuffed]    = 0;
    g_PoliceState[playerid][pol_HandcuffedBy]    = INVALID_PLAYER_ID;
    g_PoliceState[playerid][pol_IsArrested]      = 0;
    g_PoliceState[playerid][pol_JailTime]        = 0;
    g_PoliceState[playerid][pol_WantedLevel]     = 0;
    g_PoliceState[playerid][pol_WantedReason][0] = '\0';
    g_PoliceState[playerid][pol_LastWantedOfficer] = INVALID_PLAYER_ID;
    g_SuspectTarget[playerid]                    = INVALID_PLAYER_ID;
}

// Carregar wanted do banco ao logar
stock Police_LoadWanted(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new query[128];
    format(query, sizeof(query),
        "SELECT `level`, `reason` FROM `wanted` "
        "WHERE `character_id`=%d AND `active`=1 LIMIT 1",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnWantedLoad", "i", playerid);
}

forward OnWantedLoad(playerid);
public OnWantedLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    if(DB_HasResult()) {
        g_PoliceState[playerid][pol_WantedLevel] = DB_GetInt("level");
        DB_GetString("reason",
            g_PoliceState[playerid][pol_WantedReason],
            sizeof(g_PoliceState[][pol_WantedReason]));
        // Aplicar estrelas no SA-MP
        SetPlayerWantedLevel(playerid, g_PoliceState[playerid][pol_WantedLevel]);
        if(g_PoliceState[playerid][pol_WantedLevel] > 0) {
            SendWarning(playerid, "Você tem um nível de procurado ativo.");
        }
    }
    return 1;
}

// ------------------------------------------------------------
// ABORDAGEM
// ------------------------------------------------------------
stock Police_Approach(officerId, suspectId) {
    if(!IsValidPlayer(officerId) || !IsValidPlayer(suspectId)) return;
    if(!Police_IsOfficer(officerId)) {
        UI_NotifyError(officerId, "Você precisa estar em serviço como policial.");
        return;
    }
    if(!IsPlayerInRange3D(officerId,
        g_policeGetSuspectX(suspectId),
        g_policeGetSuspectY(suspectId),
        g_policeGetSuspectZ(suspectId),
        ARREST_RANGE * 2)) {
        UI_NotifyError(officerId, "Suspeito muito longe.");
        return;
    }

    g_SuspectTarget[officerId] = suspectId;

    new suspect_name[MAX_PLAYER_NAME];
    GetPlayerName(suspectId, suspect_name, sizeof(suspect_name));
    new officer_name[MAX_PLAYER_NAME];
    GetPlayerName(officerId, officer_name, sizeof(officer_name));

    // Notificar suspeito
    new msg_suspect[80];
    format(msg_suspect, sizeof(msg_suspect),
        CSTR_ERROR"[POLÍCIA] "CSTR_WHITE"Você está sendo abordado por %s.",
        officer_name);
    SendClientMessage(suspectId, COLOR_RED, msg_suspect);
    GameTextForPlayer(suspectId, "~r~Abordagem Policial!", 3000, 3);

    // Notificar policial
    new msg_officer[80];
    format(msg_officer, sizeof(msg_officer),
        CSTR_SYSTEM"[POLÍCIA] "CSTR_WHITE"Você abordou %s. Use /algema ou /identificar.",
        suspect_name);
    SendClientMessage(officerId, COLOR_INFO, msg_officer);

    Logger_Info("POLICE", "Abordagem iniciada");
}

// Helper para posição do suspeito
static Float:g_policeGetSuspectX(playerid) { new Float:x, Float:y, Float:z; GetPlayerPos(playerid, x, y, z); return x; }
static Float:g_policeGetSuspectY(playerid) { new Float:x, Float:y, Float:z; GetPlayerPos(playerid, x, y, z); return y; }
static Float:g_policeGetSuspectZ(playerid) { new Float:x, Float:y, Float:z; GetPlayerPos(playerid, x, y, z); return z; }

// ------------------------------------------------------------
// IDENTIFICAR (pedir documentos)
// ------------------------------------------------------------
stock Police_Identify(officerId, suspectId) {
    if(!Police_IsOfficer(officerId)) {
        UI_NotifyError(officerId, "Apenas policiais podem pedir identificação.");
        return;
    }

    new suspect_name[MAX_PLAYER_NAME];
    GetPlayerName(suspectId, suspect_name, sizeof(suspect_name));

    // Mostrar info do suspeito ao policial
    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Identificação ===\n\n"
        CSTR_WHITE"Nome: "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Nível: "CSTR_GREY"%d\n"
        CSTR_WHITE"Emprego: "CSTR_GREY"ID %d\n"
        CSTR_WHITE"Procurado: %s",
        suspect_name,
        PlayerData[suspectId][pLevel],
        PlayerData[suspectId][pJobID],
        g_PoliceState[suspectId][pol_WantedLevel] > 0 ?
            CSTR_ERROR"SIM" : CSTR_SUCCESS"Não");

    if(g_PoliceState[suspectId][pol_WantedLevel] > 0) {
        new wanted_info[128];
        format(wanted_info, sizeof(wanted_info),
            "\n"CSTR_WHITE"Nível procurado: "CSTR_ERROR"%d\n"
            CSTR_WHITE"Motivo: "CSTR_ERROR"%s",
            g_PoliceState[suspectId][pol_WantedLevel],
            g_PoliceState[suspectId][pol_WantedReason]);
        strcat(body, wanted_info, sizeof(body));
    }

    UI_ShowDialog(officerId, DIALOG_POLICE_RECORDS, DIALOG_STYLE_MSGBOX,
        "{ Identificação do Suspeito", body, "Fechar", "");

    // Notificar suspeito
    new officer_name[MAX_PLAYER_NAME];
    GetPlayerName(officerId, officer_name, sizeof(officer_name));
    new msg[64];
    format(msg, sizeof(msg),
        CSTR_ERROR"[POLÍCIA] "CSTR_WHITE"%s consultou seus documentos.",
        officer_name);
    SendClientMessage(suspectId, COLOR_RED, msg);
    Logger_Info("POLICE", "Identificação realizada");
}

// ------------------------------------------------------------
// ALGEMAS
// ------------------------------------------------------------
stock Police_Handcuff(officerId, suspectId) {
    if(!Police_IsOfficer(officerId)) {
        UI_NotifyError(officerId, "Apenas policiais podem algemar.");
        return;
    }
    if(!IsPlayerInRange3D(officerId,
        g_policeGetSuspectX(suspectId),
        g_policeGetSuspectY(suspectId),
        g_policeGetSuspectZ(suspectId),
        HANDCUFF_RANGE)) {
        UI_NotifyError(officerId, "Muito longe para algemar.");
        return;
    }
    if(g_PoliceState[suspectId][pol_IsHandcuffed]) {
        UI_NotifyError(officerId, "Suspeito já está algemado.");
        return;
    }

    g_PoliceState[suspectId][pol_IsHandcuffed] = 1;
    g_PoliceState[suspectId][pol_HandcuffedBy] = officerId;

    // Restringir movimento
    TogglePlayerControllable(suspectId, false);
    ApplyAnimation(suspectId, "ped", "ARRESTgun", 4.0, true, true, true, true, 0);

    new suspect_name[MAX_PLAYER_NAME];
    GetPlayerName(suspectId, suspect_name, sizeof(suspect_name));
    new officer_name[MAX_PLAYER_NAME];
    GetPlayerName(officerId, officer_name, sizeof(officer_name));

    new msg_s[80], msg_o[80];
    format(msg_s, sizeof(msg_s),
        CSTR_ERROR"[POLÍCIA] "CSTR_WHITE"Você foi algemado por %s.", officer_name);
    format(msg_o, sizeof(msg_o),
        CSTR_SUCCESS"[POLÍCIA] "CSTR_WHITE"%s foi algemado.", suspect_name);
    SendClientMessage(suspectId, COLOR_RED, msg_s);
    SendClientMessage(officerId, COLOR_SUCCESS, msg_o);
    Logger_Info("POLICE", "Algema aplicada");
}

stock Police_Unhandcuff(officerId, suspectId) {
    if(!Police_IsOfficer(officerId) && !Player_IsAdmin(officerId, ADMIN_MODERATOR)) {
        UI_NotifyError(officerId, "Sem permissão para desalgemar.");
        return;
    }
    if(!g_PoliceState[suspectId][pol_IsHandcuffed]) {
        UI_NotifyError(officerId, "Suspeito não está algemado.");
        return;
    }
    g_PoliceState[suspectId][pol_IsHandcuffed] = 0;
    g_PoliceState[suspectId][pol_HandcuffedBy] = INVALID_PLAYER_ID;
    TogglePlayerControllable(suspectId, true);
    ClearAnimations(suspectId);

    SendInfo(suspectId, "Você foi desalgemado.");
    SendSuccess(officerId, "Algemas removidas.");
}

// ------------------------------------------------------------
// PRENDER (levar para cadeia)
// ------------------------------------------------------------
stock Police_Arrest(officerId, suspectId, const reason[], jail_time = JAIL_DEFAULT_TIME) {
    if(!IsValidPlayer(officerId) || !IsValidPlayer(suspectId)) return;
    if(!Police_IsOfficer(officerId) && !Player_IsAdmin(officerId, ADMIN_MODERATOR)) {
        UI_NotifyError(officerId, "Sem permissão para prender.");
        return;
    }
    if(!g_PoliceState[suspectId][pol_IsHandcuffed]) {
        UI_NotifyError(officerId, "Algemar o suspeito primeiro (/algema [id]).");
        return;
    }

    // Registrar no banco
    new escaped_reason[256];
    DB_Escape(reason, escaped_reason, sizeof(escaped_reason));
    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `jail_records` "
        "(`character_id`, `officer_id`, `reason`, `duration`, `status`) "
        "VALUES (%d, %d, '%s', %d, 'serving')",
        PlayerData[suspectId][pCharacterID],
        PlayerData[officerId][pCharacterID],
        escaped_reason, jail_time);
    DB_Execute(query);

    // Registrar crime nos antecedentes
    format(query, sizeof(query),
        "INSERT INTO `criminal_records` "
        "(`character_id`, `officer_id`, `crime`, `gravity`, `jail_time`, `status`) "
        "VALUES (%d, %d, '%s', 1, %d, 'serving')",
        PlayerData[suspectId][pCharacterID],
        PlayerData[officerId][pCharacterID],
        escaped_reason, jail_time);
    DB_Execute(query);

    // Zerando wanted
    Police_ClearWanted(suspectId);

    // Mensagens
    new suspect_name[MAX_PLAYER_NAME];
    GetPlayerName(suspectId, suspect_name, sizeof(suspect_name));
    new officer_name[MAX_PLAYER_NAME];
    GetPlayerName(officerId, officer_name, sizeof(officer_name));

    new msg_s[128], msg_o[128];
    format(msg_s, sizeof(msg_s),
        CSTR_ERROR"[PRESO] "CSTR_WHITE"Você foi preso por %s.\n"
        CSTR_GREY"Motivo: %s | Tempo: %d min",
        officer_name, reason, jail_time);
    format(msg_o, sizeof(msg_o),
        CSTR_SUCCESS"[POLÍCIA] "CSTR_WHITE"%s preso. Tempo: %d min.",
        suspect_name, jail_time);
    SendClientMessage(suspectId, COLOR_RED, msg_s);
    SendClientMessage(officerId, COLOR_SUCCESS, msg_o);

    // Teleportar para cadeia
    g_PoliceState[suspectId][pol_IsHandcuffed] = 0;
    g_PoliceState[suspectId][pol_IsArrested]   = 1;
    g_PoliceState[suspectId][pol_JailTime]      = jail_time;
    TogglePlayerControllable(suspectId, true);
    ClearAnimations(suspectId);
    Player_SetState(suspectId, STATE_JAILED);

    SetPlayerPos(suspectId, JAIL_POS_X, JAIL_POS_Y, JAIL_POS_Z);
    SetPlayerInterior(suspectId, JAIL_INTERIOR);
    SetPlayerVirtualWorld(suspectId, JAIL_WORLD);

    GameTextForPlayer(suspectId, "~r~Você foi preso!", 5000, 3);
    SendInfo(suspectId, "Você está na cadeia. O tempo irá diminuindo automaticamente.");

    Logger_Punishment(officerId, suspectId, "ARREST", reason, jail_time);
    CallLocalFunction("OnPlayerArrested", "iis", suspectId, officerId, reason);
}

// ------------------------------------------------------------
// SOLTAR DA CADEIA
// ------------------------------------------------------------
stock Police_Release(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_PoliceState[playerid][pol_IsArrested]) return;

    g_PoliceState[playerid][pol_IsArrested] = 0;
    g_PoliceState[playerid][pol_JailTime]   = 0;
    Player_SetState(playerid, STATE_SPAWNED);

    // Teleportar para saída da delegacia
    SetPlayerPos(playerid, 246.2, 62.3, 1001.8);
    SetPlayerInterior(playerid, 6);
    SetPlayerVirtualWorld(playerid, 0);

    // Atualizar registro
    new query[128];
    format(query, sizeof(query),
        "UPDATE `jail_records` SET `status`='released', `released_at`=NOW(), "
        "`time_served`=`duration` "
        "WHERE `character_id`=%d AND `status`='serving' ORDER BY `created_at` DESC LIMIT 1",
        PlayerData[playerid][pCharacterID]);
    DB_Execute(query);

    SendSuccess(playerid,
        CSTR_SUCCESS"Você foi solto! "CSTR_WHITE"Não reincida — o sistema está te observando.");
    Logger_Info("POLICE", "Jogador solto da cadeia");
}

// Scheduler: reduzir tempo de cadeia a cada minuto
public OnScheduler1Minute() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(!g_PoliceState[i][pol_IsArrested]) continue;

        g_PoliceState[i][pol_JailTime]--;
        if(g_PoliceState[i][pol_JailTime] <= 0) {
            Police_Release(i);
        } else {
            // Aviso periódico
            if(g_PoliceState[i][pol_JailTime] % 2 == 0) {
                new msg[48];
                format(msg, sizeof(msg),
                    CSTR_GREY"[Cadeia] "CSTR_WHITE"Tempo restante: "
                    CSTR_YELLOW"%d min",
                    g_PoliceState[i][pol_JailTime]);
                SendClientMessage(i, COLOR_INFO, msg);
            }
        }
    }
}

// ------------------------------------------------------------
// NÍVEL DE PROCURADO
// ------------------------------------------------------------
stock Police_AddWanted(playerid, level, const reason[], officerId = INVALID_PLAYER_ID) {
    if(!IsValidPlayer(playerid)) return;

    new new_level = Clamp(g_PoliceState[playerid][pol_WantedLevel] + level,
                          0, MAX_WANTED_LEVEL);
    g_PoliceState[playerid][pol_WantedLevel]  = new_level;
    if(!IsNullString(reason)) {
        SafeStrCopy(g_PoliceState[playerid][pol_WantedReason], reason,
            sizeof(g_PoliceState[][pol_WantedReason]));
    }
    if(IsValidPlayer(officerId))
        g_PoliceState[playerid][pol_LastWantedOfficer] = officerId;

    SetPlayerWantedLevel(playerid, new_level);

    // Salvar/atualizar no banco
    new escaped_reason[256];
    DB_Escape(g_PoliceState[playerid][pol_WantedReason], escaped_reason, sizeof(escaped_reason));
    new officer_char_id = 0;
    if(IsValidPlayer(officerId))
        officer_char_id = PlayerData[officerId][pCharacterID];

    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `wanted` (`character_id`, `level`, `reason`, `officer_id`, `active`) "
        "VALUES (%d, %d, '%s', %d, 1) "
        "ON DUPLICATE KEY UPDATE `level`=%d, `reason`='%s', `active`=1",
        PlayerData[playerid][pCharacterID],
        new_level, escaped_reason, officer_char_id,
        new_level, escaped_reason);
    DB_Execute(query);

    if(new_level > 0) {
        new msg[80];
        format(msg, sizeof(msg),
            CSTR_ERROR"[PROCURADO] "CSTR_WHITE"Nível %d — %s",
            new_level, reason);
        SendClientMessage(playerid, COLOR_RED, msg);
        GameTextForPlayer(playerid, "~r~Procurado!", 3000, 3);
    }
}

stock Police_ClearWanted(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_PoliceState[playerid][pol_WantedLevel]     = 0;
    g_PoliceState[playerid][pol_WantedReason][0] = '\0';
    SetPlayerWantedLevel(playerid, 0);
    new query[128];
    format(query, sizeof(query),
        "UPDATE `wanted` SET `active`=0 WHERE `character_id`=%d AND `active`=1",
        PlayerData[playerid][pCharacterID]);
    DB_Execute(query);
}

// Getter
stock Police_GetWantedLevel(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return g_PoliceState[playerid][pol_WantedLevel];
}

stock bool:Police_IsHandcuffed(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return bool:g_PoliceState[playerid][pol_IsHandcuffed];
}

stock bool:Police_IsJailed(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return bool:g_PoliceState[playerid][pol_IsArrested];
}

// ------------------------------------------------------------
// MULTA POLICIAL
// ------------------------------------------------------------
stock Police_IssueFine(officerId, suspectId, crime_idx) {
    if(!Police_IsOfficer(officerId)) {
        UI_NotifyError(officerId, "Apenas policiais podem emitir multas.");
        return;
    }
    if(crime_idx < 0 || crime_idx >= g_CrimeDefCount) return;

    new fine_amount = g_CrimeDefs[crime_idx][cdef_Fine];
    new wanted_add  = g_CrimeDefs[crime_idx][cdef_WantedLevel];
    new crime_name[48];
    SafeStrCopy(crime_name, g_CrimeDefs[crime_idx][cdef_DisplayName], sizeof(crime_name));

    Fines_Issue(suspectId, officerId, "police", crime_name, fine_amount);

    if(wanted_add > 0) {
        Police_AddWanted(suspectId, wanted_add, crime_name, officerId);
    }

    new suspect_name[MAX_PLAYER_NAME];
    GetPlayerName(suspectId, suspect_name, sizeof(suspect_name));
    new fmt_fine[24];
    FormatMoneyFull(fine_amount, fmt_fine, sizeof(fmt_fine));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"[POLÍCIA] "CSTR_WHITE"Multa de %s emitida para %s — %s",
        fmt_fine, suspect_name, crime_name);
    SendClientMessage(officerId, COLOR_SUCCESS, msg);
}

// ------------------------------------------------------------
// ANTECEDENTES (consulta)
// ------------------------------------------------------------
stock Police_ShowRecords(officerId, suspectId) {
    if(!IsValidPlayer(officerId) || !IsValidPlayer(suspectId)) return;
    if(!Police_IsOfficer(officerId) && !Player_IsAdmin(officerId, ADMIN_HELPER)) {
        UI_NotifyError(officerId, "Sem permissão.");
        return;
    }

    new query[256];
    format(query, sizeof(query),
        "SELECT `crime`, `gravity`, `fine`, `jail_time`, `status`, "
        "DATE_FORMAT(`created_at`, '%%d/%%m/%%Y') as date "
        "FROM `criminal_records` WHERE `character_id`=%d "
        "ORDER BY `created_at` DESC LIMIT 10",
        PlayerData[suspectId][pCharacterID]);
    SetPVarInt(officerId,  "records_suspect", suspectId);
    DB_Query(query, "OnPoliceRecordsLoad", "i", officerId);
}

forward OnPoliceRecordsLoad(officerId);
public OnPoliceRecordsLoad(officerId) {
    if(!IsValidPlayer(officerId)) return 0;
    new suspect_name[MAX_PLAYER_NAME];
    new suspectid = GetPVarInt(officerId, "records_suspect");
    if(IsValidPlayer(suspectid))
        GetPlayerName(suspectid, suspect_name, sizeof(suspect_name));
    else
        format(suspect_name, sizeof(suspect_name), "Desconhecido");

    new rows = DB_NumRows();
    new body[1024];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Antecedentes: %s ===\n"
        CSTR_WHITE"Procurado: %s\n\n",
        suspect_name,
        g_PoliceState[suspectid][pol_WantedLevel] > 0 ?
            CSTR_ERROR"SIM" : CSTR_SUCCESS"Limpo");

    if(rows == 0) {
        strcat(body, CSTR_SUCCESS"Sem antecedentes criminais.", sizeof(body));
    } else {
        for(new r = 0; r < rows; r++) {
            new crime[48], status[12], date[12];
            DB_GetString("crime",  crime,  sizeof(crime),  r);
            DB_GetString("status", status, sizeof(status), r);
            DB_GetString("date",   date,   sizeof(date),   r);
            new fine    = DB_GetInt("fine",      r);
            new jail    = DB_GetInt("jail_time", r);
            new fine_fmt[24];
            FormatMoneyFull(fine, fine_fmt, sizeof(fine_fmt));
            new line[128];
            format(line, sizeof(line),
                CSTR_WHITE"[%s] "CSTR_ERROR"%s"
                CSTR_GREY" | Multa:%s | Pena:%dmin | %s\n",
                date, crime, fine_fmt, jail, status);
            strcat(body, line, sizeof(body));
        }
    }

    UI_ShowDialog(officerId, DIALOG_POLICE_RECORDS, DIALOG_STYLE_MSGBOX,
        "{ Antecedentes Criminais", body, "Fechar", "");
    return 1;
}

// ------------------------------------------------------------
// MENU DE CRIMES (para dar multa ou prender)
// ------------------------------------------------------------
stock Police_ShowCrimeMenu(officerId, suspectId) {
    if(!Police_IsOfficer(officerId)) {
        UI_NotifyError(officerId, "Somente policiais em serviço.");
        return;
    }
    SetPVarInt(officerId, "crime_suspect", suspectId);

    UI_ListReset();
    for(new i = 0; i < g_CrimeDefCount; i++) {
        new fine_fmt[24];
        FormatMoneyFull(g_CrimeDefs[i][cdef_Fine], fine_fmt, sizeof(fine_fmt));
        new line[96];
        format(line, sizeof(line), "[G%d] %s — %s  |  %dmin",
            g_CrimeDefs[i][cdef_Gravity],
            g_CrimeDefs[i][cdef_DisplayName],
            fine_fmt,
            g_CrimeDefs[i][cdef_JailTime]);
        UI_ListAdd(line);
    }

    new suspect_name[MAX_PLAYER_NAME];
    GetPlayerName(suspectId, suspect_name, sizeof(suspect_name));
    new title[64];
    format(title, sizeof(title), "{ Crimes — %s", suspect_name);
    UI_ListShow(officerId, DIALOG_POLICE_CRIMES, title, "Aplicar", "Cancelar");
}

// Handler de dialog policial
stock bool:Police_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_POLICE_CRIMES: {
            if(!response) return true;
            new suspectid = GetPVarInt(playerid, "crime_suspect");
            if(!IsValidPlayer(suspectid)) return true;
            if(listitem >= g_CrimeDefCount) return true;

            new jail_time = g_CrimeDefs[listitem][cdef_JailTime];
            if(jail_time > 0 && g_PoliceState[suspectid][pol_IsHandcuffed]) {
                Police_Arrest(playerid, suspectid,
                    g_CrimeDefs[listitem][cdef_DisplayName], jail_time);
            } else {
                Police_IssueFine(playerid, suspectid, listitem);
            }
            return true;
        }
    }
    return false;
}

// Integração
public OnPlayerLoaded(playerid) {
    Police_InitPlayer(playerid);
    Police_LoadWanted(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    // Se algemado, liberar controle
    if(g_PoliceState[playerid][pol_IsHandcuffed]) {
        TogglePlayerControllable(playerid, true);
    }
    Police_InitPlayer(playerid);
    return 1;
}

// ------------------------------------------------------------
// Comandos policiais
// ------------------------------------------------------------
CMD:abordar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /abordar [id]");
        return 1;
    }
    Police_Approach(playerid, targetid);
    return 1;
}

CMD:identificar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /identificar [id]");
        return 1;
    }
    Police_Identify(playerid, targetid);
    return 1;
}

CMD:algema(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /algema [id]");
        return 1;
    }
    if(g_PoliceState[targetid][pol_IsHandcuffed]) {
        Police_Unhandcuff(playerid, targetid);
    } else {
        Police_Handcuff(playerid, targetid);
    }
    return 1;
}

CMD:prender(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    new reason[MAX_REASON_LENGTH];
    new jail_time = JAIL_DEFAULT_TIME;
    if(sscanf(params, "is[128]I(%d)", targetid, reason, jail_time) ||
       !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /prender [id] [motivo] [minutos, padrão 5]");
        return 1;
    }
    Police_Arrest(playerid, targetid, reason, jail_time);
    return 1;
}

CMD:crime(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /crime [id] — Abre menu de crimes do suspeito");
        return 1;
    }
    Police_ShowCrimeMenu(playerid, targetid);
    return 1;
}

CMD:antecedentes(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /antecedentes [id]");
        return 1;
    }
    Police_ShowRecords(playerid, targetid);
    return 1;
}

CMD:procurado(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!Police_IsOfficer(playerid) && !Player_IsAdmin(playerid, ADMIN_HELPER)) {
        SendError(playerid, "Sem permissão.");
        return 1;
    }
    new targetid, level;
    new reason[MAX_REASON_LENGTH];
    if(sscanf(params, "iis[128]", targetid, level, reason) ||
       !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /procurado [id] [nível 1-6] [motivo]");
        return 1;
    }
    level = Clamp(level, 1, MAX_WANTED_LEVEL);
    Police_AddWanted(targetid, level, reason, playerid);
    return 1;
}

CMD:soltar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!Police_IsOfficer(playerid) && !Player_IsAdmin(playerid, ADMIN_MODERATOR)) {
        SendError(playerid, "Sem permissão.");
        return 1;
    }
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /soltar [id]");
        return 1;
    }
    if(!Police_IsJailed(targetid)) {
        SendError(playerid, "Este jogador não está preso.");
        return 1;
    }
    Police_Release(targetid);
    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new msg[48];
    format(msg, sizeof(msg), CSTR_SUCCESS"%s foi solto.", target_name);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    Logger_Admin(playerid, targetid, "RELEASE_JAIL", "Solto pelo policial/admin");
    return 1;
}

CMD:minhacadeia(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!Police_IsJailed(playerid)) {
        SendInfo(playerid, "Você não está preso.");
        return 1;
    }
    new msg[48];
    format(msg, sizeof(msg),
        CSTR_ERROR"[Cadeia] "CSTR_WHITE"Tempo restante: "
        CSTR_YELLOW"%d minuto(s).",
        g_PoliceState[playerid][pol_JailTime]);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}
