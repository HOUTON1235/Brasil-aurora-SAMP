// ============================================================
//  Santa Aurora Roleplay ? Sistema de Celular
//  Modulo : phone/phone_core.inc
//  Objetivo: numero, contatos, SMS, ligacoes, GPS, emergencia,
//            anuncios, anti-spam
// ============================================================

#if defined _SA_PHONE_CORE_INC
    #endinput
#endif
#define _SA_PHONE_CORE_INC
forward OnPlayerLoaded(playerid);
forward OnScheduler5Minutes();

// include removido
// include removido
// include removido
// include removido
// include removido

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define PHONE_NUMBER_LENGTH     11      // ex: 11987654321
#define MAX_CONTACTS            50
#define MAX_SMS_INBOX           30
#define MAX_ANNOUNCEMENTS       20      // cache global de anuncios ativos
#define SMS_MAX_LENGTH          160
#define SMS_COST                5       // $ por SMS
#define ANNOUNCE_COST           100     // $ por anuncio
#define ANNOUNCE_COOLDOWN       300     // segundos entre anuncios
#define CALL_RING_TIME          20000   // ms para timeout de chamada

// Dialogs do celular
#define DIALOG_PHONE_MENU       1100
#define DIALOG_PHONE_CONTACTS   1101
#define DIALOG_PHONE_ADD_CONT   1102
#define DIALOG_PHONE_SMS_INBOX  1103
#define DIALOG_PHONE_SMS_SEND   1104
#define DIALOG_PHONE_SMS_NEW    1105
#define DIALOG_PHONE_CALL       1106
#define DIALOG_PHONE_INCOMING   1107
#define DIALOG_PHONE_ANNOUNCE   1108
#define DIALOG_PHONE_ANN_LIST   1109
#define DIALOG_PHONE_ANN_NEW    1110
#define DIALOG_PHONE_GPS        1111
#define DIALOG_PHONE_EMERGENCY  1112

// ------------------------------------------------------------
// Cache do celular do jogador
// ------------------------------------------------------------
enum E_PHONE_DATA {
    ph_DBID,
    ph_Number[PHONE_NUMBER_LENGTH + 1],
    ph_Model[32],
    ph_Battery,
    bool:ph_Blocked,
    bool:ph_Loaded,
    bool:ph_InCall,
    ph_CallTarget,          // playerid alvo
    ph_CallTimer,           // timer da chamada
    ph_CallStartTime,       // para calcular duracao
    bool:ph_Ringing         // esta recebendo chamada?
}

static g_Phone[MAX_PLAYERS][E_PHONE_DATA];

// Contatos em cache
enum E_CONTACT {
    ct_ID,
    ct_Name[32],
    ct_Number[PHONE_NUMBER_LENGTH + 1],
    bool:ct_Blocked
}
static g_Contacts[MAX_PLAYERS][MAX_CONTACTS][E_CONTACT];
static g_ContactCount[MAX_PLAYERS] = {0, ...};

// SMS inbox em cache (ultimas mensagens recebidas)
enum E_SMS {
    sms_ID,
    sms_FromName[MAX_PLAYER_NAME],
    sms_FromNumber[PHONE_NUMBER_LENGTH + 1],
    sms_Message[SMS_MAX_LENGTH + 1],
    sms_Date[20],
    bool:sms_Read
}
static g_SMSInbox[MAX_PLAYERS][MAX_SMS_INBOX][E_SMS];
static g_SMSCount[MAX_PLAYERS]  = {0, ...};
static g_SMSUnread[MAX_PLAYERS] = {0, ...};

// Anti-spam de anuncio
static g_LastAnnouncTime[MAX_PLAYERS] = {0, ...};

// Cache global de anuncios ativos
enum E_ANNOUNCE {
    ann_ID,
    ann_CharName[MAX_PLAYER_NAME],
    ann_Type[12],
    ann_Title[48],
    ann_Body[256],
    ann_Price,
    ann_Contact[16],
    ann_Date[20]
}
static g_Announcements[MAX_ANNOUNCEMENTS][E_ANNOUNCE];
static g_AnnounceCount = 0;

// ------------------------------------------------------------
// Gerar numero de telefone unico
// ------------------------------------------------------------
static Phone_GenerateNumber(output[], size = sizeof(output)) {
    format(output, size, "11%d%d%d%d%d%d%d%d%d",
        random(9), random(9), random(9),
        random(9), random(9), random(9),
        random(9), random(9), random(9));
}

// ------------------------------------------------------------
// Inicializar dados do celular
// ------------------------------------------------------------
stock Phone_Init(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_Phone[playerid][ph_DBID]          = INVALID_DB_ID;
    g_Phone[playerid][ph_Number][0]     = '\0';
    g_Phone[playerid][ph_Model][0]      = '\0';
    g_Phone[playerid][ph_Battery]       = 100;
    g_Phone[playerid][ph_Blocked]       = false;
    g_Phone[playerid][ph_Loaded]        = false;
    g_Phone[playerid][ph_InCall]        = false;
    g_Phone[playerid][ph_CallTarget]    = INVALID_PLAYER_ID;
    g_Phone[playerid][ph_CallTimer]     = -1;
    g_Phone[playerid][ph_CallStartTime] = 0;
    g_Phone[playerid][ph_Ringing]       = false;
    g_ContactCount[playerid]            = 0;
    g_SMSCount[playerid]                = 0;
    g_SMSUnread[playerid]               = 0;
}

// ------------------------------------------------------------
// Carregar celular do banco
// ------------------------------------------------------------
stock Phone_Load(playerid) {
    if(!IsValidPlayer(playerid)) return;
    Phone_Init(playerid);
    new query[128];
    format(query, sizeof(query),
        "SELECT `id`, `number`, `model`, `battery`, `blocked` "
        "FROM `phones` WHERE `character_id`=%d LIMIT 1",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnPhoneLoad", "i", playerid);
}

forward OnPhoneLoad(playerid);
public OnPhoneLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    if(!DB_HasResult()) {
        Phone_CreateNew(playerid);
        return 1;
    }
    g_Phone[playerid][ph_DBID]    = DB_GetInt("id");
    DB_GetString("number", g_Phone[playerid][ph_Number],
        sizeof(g_Phone[][ph_Number]));
    DB_GetString("model",  g_Phone[playerid][ph_Model],
        sizeof(g_Phone[][ph_Model]));
    g_Phone[playerid][ph_Battery] = DB_GetInt("battery");
    g_Phone[playerid][ph_Blocked] = bool:DB_GetInt("blocked");
    g_Phone[playerid][ph_Loaded]  = true;

    // Carregar contatos e inbox
    Phone_LoadContacts(playerid);
    Phone_LoadSMSInbox(playerid);
    return 1;
}

// Criar novo celular
stock Phone_CreateNew(playerid) {
    new number[PHONE_NUMBER_LENGTH + 1];
    Phone_GenerateNumber(number, sizeof(number));

    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `phones` (`character_id`, `number`, `model`) "
        "VALUES (%d, '%s', 'Basic Phone')",
        PlayerData[playerid][pCharacterID], number);
    SetPVarString(playerid, "ph_new_number", number);
    DB_Query(query, "OnPhoneCreated", "i", playerid);
    SafeStrCopy(g_Phone[playerid][ph_Number], number,
        sizeof(g_Phone[][ph_Number]));
}

forward OnPhoneCreated(playerid);
public OnPhoneCreated(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    g_Phone[playerid][ph_DBID]   = DB_InsertID();
    g_Phone[playerid][ph_Loaded] = true;
    GetPVarString(playerid, "ph_new_number",
        g_Phone[playerid][ph_Number], sizeof(g_Phone[][ph_Number]));
    DeletePVar(playerid, "ph_new_number");
    SafeStrCopy(g_Phone[playerid][ph_Model], "Basic Phone",
        sizeof(g_Phone[][ph_Model]));
    new msg[64];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[Celular] "CSTR_WHITE"Seu numero: "
        CSTR_SYSTEM"%s", g_Phone[playerid][ph_Number]);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}

// Carregar contatos
stock Phone_LoadContacts(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new query[128];
    format(query, sizeof(query),
        "SELECT `id`, `contact_name`, `number`, `blocked` "
        "FROM `phone_contacts` WHERE `character_id`=%d ORDER BY `contact_name` ASC LIMIT %d",
        PlayerData[playerid][pCharacterID], MAX_CONTACTS);
    DB_Query(query, "OnContactsLoad", "i", playerid);
}

forward OnContactsLoad(playerid);
public OnContactsLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    g_ContactCount[playerid] = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_ContactCount[playerid] < MAX_CONTACTS; r++) {
        new i = g_ContactCount[playerid];
        g_Contacts[playerid][i][ct_ID] = DB_GetInt("id", r);
        DB_GetString("contact_name", g_Contacts[playerid][i][ct_Name],
            sizeof(g_Contacts[][][ct_Name]), r);
        DB_GetString("number", g_Contacts[playerid][i][ct_Number],
            sizeof(g_Contacts[][][ct_Number]), r);
        g_Contacts[playerid][i][ct_Blocked] = bool:DB_GetInt("blocked", r);
        g_ContactCount[playerid]++;
    }
    return 1;
}

// Carregar inbox SMS
stock Phone_LoadSMSInbox(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new query[256];
    format(query, sizeof(query),
        "SELECT m.`id`, m.`from_number`, m.`message`, m.`read`, "
        "DATE_FORMAT(m.`created_at`, '%%d/%%m %%H:%%i') as date, "
        "(SELECT `name` FROM `characters` WHERE `id`=m.`from_char_id`) as sender "
        "FROM `messages` m "
        "WHERE m.`to_char_id`=%d "
        "ORDER BY m.`created_at` DESC LIMIT %d",
        PlayerData[playerid][pCharacterID], MAX_SMS_INBOX);
    DB_Query(query, "OnSMSInboxLoad", "i", playerid);
}

forward OnSMSInboxLoad(playerid);
public OnSMSInboxLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    g_SMSCount[playerid]  = 0;
    g_SMSUnread[playerid] = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_SMSCount[playerid] < MAX_SMS_INBOX; r++) {
        new i = g_SMSCount[playerid];
        g_SMSInbox[playerid][i][sms_ID] = DB_GetInt("id", r);
        DB_GetString("sender",      g_SMSInbox[playerid][i][sms_FromName],  sizeof(g_SMSInbox[][][sms_FromName]),  r);
        DB_GetString("from_number", g_SMSInbox[playerid][i][sms_FromNumber],sizeof(g_SMSInbox[][][sms_FromNumber]),r);
        DB_GetString("message",     g_SMSInbox[playerid][i][sms_Message],   sizeof(g_SMSInbox[][][sms_Message]),   r);
        DB_GetString("date",        g_SMSInbox[playerid][i][sms_Date],      sizeof(g_SMSInbox[][][sms_Date]),      r);
        g_SMSInbox[playerid][i][sms_Read] = bool:DB_GetInt("read", r);
        if(!g_SMSInbox[playerid][i][sms_Read]) g_SMSUnread[playerid]++;
        g_SMSCount[playerid]++;
    }
    // Notificar mensagens nao lidas
    if(g_SMSUnread[playerid] > 0) {
        new msg[48];
        format(msg, sizeof(msg),
            CSTR_SYSTEM"[SMS] "CSTR_WHITE"Voce tem "
            CSTR_YELLOW"%d"CSTR_WHITE" mensagem(ns) nao lida(s).",
            g_SMSUnread[playerid]);
        SendClientMessage(playerid, COLOR_INFO, msg);
    }
    return 1;
}

// Carregar anuncios ativos
stock Phone_LoadAnnouncements() {
    DB_Query(
        "SELECT a.`id`, c.`name` as char_name, a.`type`, a.`title`, a.`body`, "
        "a.`price`, a.`contact`, DATE_FORMAT(a.`created_at`,'%%d/%%m') as date "
        "FROM `announcements` a "
        "JOIN `characters` c ON c.`id`=a.`character_id` "
        "WHERE a.`active`=1 AND (a.`expires_at` IS NULL OR a.`expires_at`>NOW()) "
        "ORDER BY a.`created_at` DESC LIMIT " #MAX_ANNOUNCEMENTS,
        "OnAnnouncementsLoaded", "");
}

forward OnAnnouncementsLoaded();
public OnAnnouncementsLoaded() {
    g_AnnounceCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_AnnounceCount < MAX_ANNOUNCEMENTS; r++) {
        new i = g_AnnounceCount;
        g_Announcements[i][ann_ID] = DB_GetInt("id", r);
        DB_GetString("char_name", g_Announcements[i][ann_CharName], sizeof(g_Announcements[][ann_CharName]), r);
        DB_GetString("type",    g_Announcements[i][ann_Type],    sizeof(g_Announcements[][ann_Type]),    r);
        DB_GetString("title",   g_Announcements[i][ann_Title],   sizeof(g_Announcements[][ann_Title]),   r);
        DB_GetString("body",    g_Announcements[i][ann_Body],    sizeof(g_Announcements[][ann_Body]),    r);
        DB_GetString("contact", g_Announcements[i][ann_Contact], sizeof(g_Announcements[][ann_Contact]), r);
        DB_GetString("date",    g_Announcements[i][ann_Date],    sizeof(g_Announcements[][ann_Date]),    r);
        g_Announcements[i][ann_Price] = DB_GetInt("price", r);
        g_AnnounceCount++;
    }
    return 1;
}

// ------------------------------------------------------------
// Getter do numero
// ------------------------------------------------------------
stock Phone_GetNumber(playerid, output[], size = sizeof(output)) {
    if(!IsValidPlayer(playerid)) return;
    SafeStrCopy(output, g_Phone[playerid][ph_Number], size);
}

// Encontrar jogador online pelo numero
stock Phone_FindByNumber(number[]) {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(!strcmp(g_Phone[i][ph_Number], number, true)) return i;
    }
    return INVALID_PLAYER_ID;
}

// Encontrar numero de um contato
static Phone_GetContactNumber(playerid, name_or_number[], output[], size) {
    // Tentar como numero direto
    if(strlen(name_or_number) == PHONE_NUMBER_LENGTH) {
        SafeStrCopy(output, name_or_number, size);
        return;
    }
    // Tentar como nome de contato
    for(new i = 0; i < g_ContactCount[playerid]; i++) {
        if(!strcmp(g_Contacts[playerid][i][ct_Name], name_or_number, true)) {
            SafeStrCopy(output, g_Contacts[playerid][i][ct_Number], size);
            return;
        }
    }
    // Nao encontrou
    output[0] = '\0';
}

// ------------------------------------------------------------
// ENVIAR SMS
// ------------------------------------------------------------
stock Phone_SendSMS(playerid, to_number[], message[]) {
    if(!IsValidPlayer(playerid)) return false;
    if(!g_Phone[playerid][ph_Loaded]) {
        UI_NotifyError(playerid, "Sem celular disponivel.");
        return false;
    }
    if(g_Phone[playerid][ph_Blocked]) {
        UI_NotifyError(playerid, "Seu celular esta bloqueado.");
        return false;
    }
    if(strlen(message) == 0 || strlen(message) > SMS_MAX_LENGTH) {
        UI_NotifyError(playerid, "Mensagem invalida.");
        return false;
    }
    // Verificar se destino existe
    new targetid = Phone_FindByNumber(to_number);

    // Cobrar SMS
    if(PlayerData[playerid][pMoney] < SMS_COST) {
        UI_NotifyError(playerid, "Sem dinheiro para enviar SMS.");
        return false;
    }
    Player_RemoveMoney(playerid, SMS_COST);

    // Salvar no banco
    new escaped_msg[SMS_MAX_LENGTH * 2];
    DB_Escape(message, escaped_msg, sizeof(escaped_msg));
    new from_num[PHONE_NUMBER_LENGTH + 1];
    SafeStrCopy(from_num, g_Phone[playerid][ph_Number], sizeof(from_num));
    new escaped_from[24], escaped_to[24];
    DB_Escape(from_num,  escaped_from, sizeof(escaped_from));
    DB_Escape(to_number, escaped_to,   sizeof(escaped_to));

    // Buscar to_char_id pelo numero
    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `messages` "
        "(`from_char_id`, `to_char_id`, `from_number`, `to_number`, `message`) "
        "SELECT %d, p.`character_id`, '%s', '%s', '%s' "
        "FROM `phones` p WHERE p.`number`='%s' LIMIT 1",
        PlayerData[playerid][pCharacterID],
        escaped_from, escaped_to, escaped_msg, escaped_to);
    DB_Execute(query);

    // Entregar em tempo real se online
    if(IsValidPlayer(targetid) && g_Phone[targetid][ph_Loaded]) {
        new sender_name[MAX_PLAYER_NAME];
        GetPlayerName(playerid, sender_name, sizeof(sender_name));

        // Adicionar ao inbox do destinatario
        if(g_SMSCount[targetid] < MAX_SMS_INBOX) {
            new si = g_SMSCount[targetid];
            g_SMSInbox[targetid][si][sms_ID]        = 0;
            SafeStrCopy(g_SMSInbox[targetid][si][sms_FromName],   sender_name, sizeof(g_SMSInbox[][][sms_FromName]));
            SafeStrCopy(g_SMSInbox[targetid][si][sms_FromNumber], from_num,    sizeof(g_SMSInbox[][][sms_FromNumber]));
            SafeStrCopy(g_SMSInbox[targetid][si][sms_Message],    message,     sizeof(g_SMSInbox[][][sms_Message]));
            g_SMSInbox[targetid][si][sms_Read] = false;
            g_SMSCount[targetid]++;
            g_SMSUnread[targetid]++;
        }

        // Notificar
        new notify[160];
        format(notify, sizeof(notify),
            CSTR_SYSTEM"[SMS de %s (%s)]"CSTR_WHITE" %s",
            sender_name, from_num, message);
        SendClientMessage(targetid, COLOR_CYAN, notify);
        GameTextForPlayer(targetid, "~b~SMS Recebido!", 2000, 3);
    }

    SendSuccess(playerid, "SMS enviado.");
    Logger_Chat(playerid, "SMS", message);
    return true;
}

// ------------------------------------------------------------
// LIGAR
// ------------------------------------------------------------
stock Phone_Call(playerid, to_number[]) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_Phone[playerid][ph_Loaded]) {
        UI_NotifyError(playerid, "Sem celular.");
        return;
    }
    if(g_Phone[playerid][ph_InCall] || g_Phone[playerid][ph_Ringing]) {
        UI_NotifyError(playerid, "Voce ja esta em uma chamada.");
        return;
    }

    new targetid = Phone_FindByNumber(to_number);
    if(!IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Numero fora de area ou desligado.");
        return;
    }
    if(targetid == playerid) {
        UI_NotifyError(playerid, "Voce nao pode ligar para si mesmo.");
        return;
    }
    if(g_Phone[targetid][ph_Blocked]) {
        UI_NotifyError(playerid, "Este numero esta indisponivel.");
        return;
    }
    if(g_Phone[targetid][ph_InCall]) {
        UI_NotifyError(playerid, "Ocupado.");
        return;
    }

    // Iniciar chamada
    g_Phone[playerid][ph_CallTarget] = targetid;
    g_Phone[playerid][ph_Ringing]    = true;

    g_Phone[targetid][ph_CallTarget] = playerid;
    g_Phone[targetid][ph_Ringing]    = true;

    // Notificar chamador
    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new msg_caller[64];
    format(msg_caller, sizeof(msg_caller),
        CSTR_SYSTEM"[Chamando %s (%s)...]", target_name, to_number);
    SendClientMessage(playerid, COLOR_INFO, msg_caller);

    // Notificar destinatario
    new caller_name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, caller_name, sizeof(caller_name));
    new from_num[PHONE_NUMBER_LENGTH + 1];
    SafeStrCopy(from_num, g_Phone[playerid][ph_Number], sizeof(from_num));

    UI_ShowDialog(targetid, DIALOG_PHONE_INCOMING, DIALOG_STYLE_MSGBOX,
        "{ Chamada Recebida",
        caller_name,
        "Atender", "Rejeitar");

    SetPVarInt(targetid, "incoming_call_from", playerid);
    GameTextForPlayer(targetid, "~b~Chamada Recebida!", 3000, 3);

    // Timer de timeout (20s sem atender)
    g_Phone[playerid][ph_CallTimer] =
        SetTimerEx("Phone_CallTimeout", CALL_RING_TIME, false, "i", playerid);
}

forward Phone_CallTimeout(callerid);
public Phone_CallTimeout(callerid) {
    if(!IsValidPlayer(callerid)) return 0;
    if(!g_Phone[callerid][ph_Ringing]) return 0;

    new targetid = g_Phone[callerid][ph_CallTarget];
    Phone_EndCall(callerid, false, "timeout");
    if(IsValidPlayer(targetid)) {
        Phone_EndCall(targetid, false, "missed");
        UI_NotifyWarning(targetid, "Chamada perdida.");
    }
    UI_NotifyWarning(callerid, "Sem resposta.");
    return 1;
}

// Atender chamada
stock Phone_Answer(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new callerid = GetPVarInt(playerid, "incoming_call_from");
    DeletePVar(playerid, "incoming_call_from");
    if(!IsValidPlayer(callerid)) return;

    // Cancelar timer de timeout
    if(g_Phone[callerid][ph_CallTimer] != -1) {
        KillTimer(g_Phone[callerid][ph_CallTimer]);
        g_Phone[callerid][ph_CallTimer] = -1;
    }

    g_Phone[playerid][ph_InCall]        = true;
    g_Phone[playerid][ph_Ringing]       = false;
    g_Phone[playerid][ph_CallStartTime] = gettime();
    g_Phone[callerid][ph_InCall]        = true;
    g_Phone[callerid][ph_Ringing]       = false;
    g_Phone[callerid][ph_CallStartTime] = gettime();

    new caller_name[MAX_PLAYER_NAME], target_name[MAX_PLAYER_NAME];
    GetPlayerName(callerid,  caller_name,  sizeof(caller_name));
    GetPlayerName(playerid,  target_name,  sizeof(target_name));

    new msg_a[64], msg_b[64];
    format(msg_a, sizeof(msg_a), CSTR_SUCCESS"[Em chamada com %s]  /desligar", target_name);
    format(msg_b, sizeof(msg_b), CSTR_SUCCESS"[Em chamada com %s]  /desligar", caller_name);
    SendClientMessage(playerid, COLOR_SUCCESS, msg_a);
    SendClientMessage(callerid, COLOR_SUCCESS, msg_b);

    // Ativar chat de chamada (usar /c para falar na ligacao)
    SetPVarInt(playerid, "in_call_with", callerid);
    SetPVarInt(callerid,  "in_call_with", playerid);
}

// Rejeitar chamada
stock Phone_Reject(playerid) {
    new callerid = GetPVarInt(playerid, "incoming_call_from");
    DeletePVar(playerid, "incoming_call_from");
    Phone_EndCall(playerid, false, "rejected");
    if(IsValidPlayer(callerid)) {
        Phone_EndCall(callerid, false, "rejected");
        UI_NotifyWarning(callerid, "Chamada rejeitada.");
    }
}

// Encerrar chamada
stock Phone_EndCall(playerid, save_log = true, status[] = "answered") {
    if(!IsValidPlayer(playerid)) return;
    new duration = gettime() - g_Phone[playerid][ph_CallStartTime];

    // Salvar log se foi atendida
    if(save_log && g_Phone[playerid][ph_InCall]) {
        new targetid = g_Phone[playerid][ph_CallTarget];
        if(IsValidPlayer(targetid)) {
            new from_num[PHONE_NUMBER_LENGTH + 1];
            new to_num[PHONE_NUMBER_LENGTH + 1];
            SafeStrCopy(from_num, g_Phone[playerid][ph_Number], sizeof(from_num));
            SafeStrCopy(to_num,   g_Phone[targetid][ph_Number], sizeof(to_num));
            new escaped_status[16];
            DB_Escape(status, escaped_status, sizeof(escaped_status));
            new query[256];
            format(query, sizeof(query),
                "INSERT INTO `calls` "
                "(`from_char_id`, `to_char_id`, `from_number`, `to_number`, "
                "`duration`, `status`) "
                "VALUES (%d, %d, '%s', '%s', %d, '%s')",
                PlayerData[playerid][pCharacterID],
                PlayerData[targetid][pCharacterID],
                from_num, to_num, duration, escaped_status);
            DB_Execute(query);
        }
    }

    g_Phone[playerid][ph_InCall]        = false;
    g_Phone[playerid][ph_Ringing]       = false;
    g_Phone[playerid][ph_CallTarget]    = INVALID_PLAYER_ID;
    g_Phone[playerid][ph_CallStartTime] = 0;
    if(g_Phone[playerid][ph_CallTimer] != -1) {
        KillTimer(g_Phone[playerid][ph_CallTimer]);
        g_Phone[playerid][ph_CallTimer] = -1;
    }
    DeletePVar(playerid, "in_call_with");
}

// Falar na ligacao (chat de chamada)
stock Phone_TalkInCall(playerid, message[]) {
    if(!IsValidPlayer(playerid)) return;
    new targetid = GetPVarInt(playerid, "in_call_with");
    if(!IsValidPlayer(targetid)) {
        UI_NotifyError(playerid, "Voce nao esta em chamada.");
        return;
    }
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    new msg[192];
    format(msg, sizeof(msg), CSTR_SYSTEM"[Ligacao] "CSTR_WHITE"%s: %s", name, message);
    SendClientMessage(playerid, COLOR_CYAN, msg);
    SendClientMessage(targetid, COLOR_CYAN, msg);
}

// ------------------------------------------------------------
// ADICIONAR CONTATO
// ------------------------------------------------------------
stock Phone_AddContact(playerid, name[], number[]) {
    if(!IsValidPlayer(playerid)) return;
    if(g_ContactCount[playerid] >= MAX_CONTACTS) {
        UI_NotifyError(playerid, "Agenda cheia.");
        return;
    }
    if(strlen(name) < 2 || strlen(number) < 8) {
        UI_NotifyError(playerid, "Nome ou numero invalido.");
        return;
    }

    new escaped_name[64], escaped_num[24];
    DB_Escape(name,   escaped_name, sizeof(escaped_name));
    DB_Escape(number, escaped_num,  sizeof(escaped_num));

    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `phone_contacts` (`character_id`, `contact_name`, `number`) "
        "VALUES (%d, '%s', '%s') ON DUPLICATE KEY UPDATE `contact_name`='%s'",
        PlayerData[playerid][pCharacterID],
        escaped_name, escaped_num, escaped_name);
    DB_Execute(query);

    new i = g_ContactCount[playerid];
    g_Contacts[playerid][i][ct_ID] = 0;
    SafeStrCopy(g_Contacts[playerid][i][ct_Name],   name,   sizeof(g_Contacts[][][ct_Name]));
    SafeStrCopy(g_Contacts[playerid][i][ct_Number], number, sizeof(g_Contacts[][][ct_Number]));
    g_Contacts[playerid][i][ct_Blocked] = false;
    g_ContactCount[playerid]++;

    new msg[64];
    format(msg, sizeof(msg), CSTR_SUCCESS"Contato %s (%s) salvo.", name, number);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
}

// ------------------------------------------------------------
// ANUNCIOS
// ------------------------------------------------------------
stock Phone_PublishAnnounce(playerid, ann_type[], title[],
                             body[], price, contact[]) {
    if(!IsValidPlayer(playerid)) return;

    // Anti-spam
    new now = gettime();
    if(now - g_LastAnnouncTime[playerid] < ANNOUNCE_COOLDOWN) {
        new remaining = ANNOUNCE_COOLDOWN - (now - g_LastAnnouncTime[playerid]);
        new msg[64];
        format(msg, sizeof(msg),
            "Aguarde %d segundo(s) para publicar outro anuncio.", remaining);
        UI_NotifyError(playerid, msg);
        return;
    }

    // Cobrar
    if(PlayerData[playerid][pMoney] < ANNOUNCE_COST) {
        UI_NotifyError(playerid, "Voce precisa de $100 para publicar um anuncio.");
        return;
    }
    Player_RemoveMoney(playerid, ANNOUNCE_COST);
    g_LastAnnouncTime[playerid] = now;

    new escaped_type[16], escaped_title[96], escaped_body[512], escaped_contact[32];
    DB_Escape(ann_type, escaped_type,    sizeof(escaped_type));
    DB_Escape(title,    escaped_title,   sizeof(escaped_title));
    DB_Escape(body,     escaped_body,    sizeof(escaped_body));
    DB_Escape(contact,  escaped_contact, sizeof(escaped_contact));

    new query[768];
    format(query, sizeof(query),
        "INSERT INTO `announcements` "
        "(`character_id`, `type`, `title`, `body`, `price`, `contact`, "
        "`expires_at`) "
        "VALUES (%d, '%s', '%s', '%s', %d, '%s', "
        "DATE_ADD(NOW(), INTERVAL 24 HOUR))",
        PlayerData[playerid][pCharacterID],
        escaped_type, escaped_title, escaped_body, price, escaped_contact);
    DB_Execute(query);

    // Broadcast para todos
    new char_name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, char_name, sizeof(char_name));
    new broadcast[256];
    format(broadcast, sizeof(broadcast),
        CSTR_YELLOW"[ANUNCIO | %s] "CSTR_WHITE"%s: %s | Contato: %s",
        ann_type, title, body, contact);
    SendClientMessageToAll(COLOR_YELLOW, broadcast);

    // Atualizar cache
    Phone_LoadAnnouncements();
    SendSuccess(playerid, "Anuncio publicado!");
}

// ------------------------------------------------------------
// MENUS DO CELULAR
// ------------------------------------------------------------
stock Phone_ShowMenu(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_Phone[playerid][ph_Loaded]) {
        UI_NotifyError(playerid, "Voce nao tem celular.");
        return;
    }

    new unread_str[16];
    if(g_SMSUnread[playerid] > 0)
        format(unread_str, sizeof(unread_str), " (%d)", g_SMSUnread[playerid]);

    new options[256];
    format(options, sizeof(options),
        "Contatos [%d]\n"
        "SMS%s\n"
        "Ligar\n"
        "Anuncios [%d]\n"
        "GPS\n"
        "Emergencia\n"
        "Meu Numero\n",
        g_ContactCount[playerid],
        unread_str,
        g_AnnounceCount);

    new title[64];
    format(title, sizeof(title), "{ Celular  [%s]", g_Phone[playerid][ph_Number]);
    UI_ShowDialog(playerid, DIALOG_PHONE_MENU, DIALOG_STYLE_LIST,
        title, options, "Selecionar", "Fechar");
}

// Mostrar contatos
static Phone_ShowContacts(playerid) {
    UI_ListReset();
    if(g_ContactCount[playerid] == 0) {
        UI_ListAdd("Nenhum contato salvo.");
    } else {
        for(new i = 0; i < g_ContactCount[playerid]; i++) {
            new line[64];
            format(line, sizeof(line), "%s  %s%s",
                g_Contacts[playerid][i][ct_Name],
                g_Contacts[playerid][i][ct_Number],
                g_Contacts[playerid][i][ct_Blocked] ? " [Bloqueado]" : "");
            UI_ListAdd(line);
        }
    }
    UI_ListShow(playerid, DIALOG_PHONE_CONTACTS,
        "{ Contatos", "Ligar", "Voltar");
}

// Mostrar inbox SMS
static Phone_ShowSMSInbox(playerid) {
    UI_ListReset();
    if(g_SMSCount[playerid] == 0) {
        UI_ListAdd("Caixa de entrada vazia.");
    } else {
        for(new i = 0; i < g_SMSCount[playerid]; i++) {
            new line[80];
            format(line, sizeof(line), "%s%s (%s): %.30s",
                g_SMSInbox[playerid][i][sms_Read] ? "" : CSTR_YELLOW"[N] "CSTR_WHITE,
                g_SMSInbox[playerid][i][sms_FromName],
                g_SMSInbox[playerid][i][sms_Date],
                g_SMSInbox[playerid][i][sms_Message]);
            UI_ListAdd(line);
        }
    }
    UI_ListShow(playerid, DIALOG_PHONE_SMS_INBOX,
        "{ SMS ? Caixa de Entrada", "Ver / Responder", "Voltar");
    // Marcar como lido
    if(g_SMSUnread[playerid] > 0) {
        new query[128];
        format(query, sizeof(query),
            "UPDATE `messages` SET `read`=1 "
            "WHERE `to_char_id`=%d AND `read`=0",
            PlayerData[playerid][pCharacterID]);
        DB_Execute(query);
        for(new i = 0; i < g_SMSCount[playerid]; i++)
            g_SMSInbox[playerid][i][sms_Read] = true;
        g_SMSUnread[playerid] = 0;
    }
}

// Mostrar lista de anuncios
static Phone_ShowAnnouncements(playerid) {
    if(g_AnnounceCount == 0) {
        UI_NotifyInfo(playerid, "Nenhum anuncio ativo no momento.");
        return;
    }
    UI_ListReset();
    for(new i = 0; i < g_AnnounceCount; i++) {
        new line[96];
        format(line, sizeof(line), "[%s] %s ? %s",
            g_Announcements[i][ann_Type],
            g_Announcements[i][ann_Title],
            g_Announcements[i][ann_CharName]);
        UI_ListAdd(line);
    }
    UI_ListShow(playerid, DIALOG_PHONE_ANN_LIST,
        "{ Anuncios", "Ver Detalhes", "Voltar");
}

// Menu GPS
static Phone_ShowGPS(playerid) {
    new body[] =
        CSTR_SYSTEM"=== GPS ===\n\n"
        CSTR_WHITE"Pontos de interesse:\n\n"
        CSTR_GREY"? Hospital\n"
        CSTR_GREY"? Delegacia\n"
        CSTR_GREY"? Banco\n"
        CSTR_GREY"? Prefeitura\n"
        CSTR_GREY"? Posto de Combustivel\n"
        CSTR_GREY"? Concessionaria\n"
        CSTR_GREY"? Mercado\n\n"
        CSTR_GREY"(GPS interativo disponivel na Fase 6)";
    UI_ShowDialog(playerid, DIALOG_PHONE_GPS, DIALOG_STYLE_MSGBOX,
        "{ GPS", body, "Fechar", "");
}

// Emergencia
static Phone_ShowEmergency(playerid) {
    new body[] =
        CSTR_SYSTEM"=== Emergencias ===\n\n"
        CSTR_WHITE"190 "CSTR_GREY"? Policia\n"
        CSTR_WHITE"192 "CSTR_GREY"? SAMU\n"
        CSTR_WHITE"193 "CSTR_GREY"? Bombeiros\n\n"
        CSTR_GREY"Ligar para um numero de emergencia alertara\n"
        "as equipes sobre sua localizacao.";
    UI_ShowDialog(playerid, DIALOG_PHONE_EMERGENCY, DIALOG_STYLE_MSGBOX,
        "{ Emergencias", body, "Ligar 190", "Ligar 192");
}

// ------------------------------------------------------------
// Handler de dialogs do celular
// ------------------------------------------------------------
stock Phone_HandleDialog(playerid, dialogid, response, listitem,
                               inputtext[]) {
    switch(dialogid) {
        case DIALOG_PHONE_MENU: {
            if(!response) return true;
            switch(listitem) {
                case 0: Phone_ShowContacts(playerid);
                case 1: Phone_ShowSMSInbox(playerid);
                case 2: {
                    UI_ShowDialog(playerid, DIALOG_PHONE_CALL, DIALOG_STYLE_INPUT,
                        "{ Ligar",
                        "Digite o numero ou nome do contato para ligar:",
                        "Ligar", "Cancelar");
                }
                case 3: Phone_ShowAnnouncements(playerid);
                case 4: Phone_ShowGPS(playerid);
                case 5: Phone_ShowEmergency(playerid);
                case 6: {
                    new msg[64];
                    format(msg, sizeof(msg),
                        CSTR_SYSTEM"Seu numero: "CSTR_WHITE"%s",
                        g_Phone[playerid][ph_Number]);
                    UI_NotifyInfo(playerid, msg);
                }
            }
            return true;
        }
        case DIALOG_PHONE_CONTACTS: {
            if(!response) { Phone_ShowMenu(playerid); return true; }
            if(listitem < g_ContactCount[playerid]) {
                new number[PHONE_NUMBER_LENGTH + 1];
                SafeStrCopy(number, g_Contacts[playerid][listitem][ct_Number],
                    sizeof(number));
                Phone_Call(playerid, number);
            }
            return true;
        }
        case DIALOG_PHONE_INCOMING: {
            if(response) Phone_Answer(playerid);
            else         Phone_Reject(playerid);
            return true;
        }
        case DIALOG_PHONE_CALL: {
            if(!response) return true;
            new resolved[PHONE_NUMBER_LENGTH + 1];
            Phone_GetContactNumber(playerid, inputtext, resolved, sizeof(resolved));
            if(IsNullString(resolved)) {
                UI_NotifyError(playerid, "Numero nao encontrado.");
                return true;
            }
            Phone_Call(playerid, resolved);
            return true;
        }
        case DIALOG_PHONE_SMS_INBOX: {
            if(!response) { Phone_ShowMenu(playerid); return true; }
            if(listitem >= g_SMSCount[playerid]) return true;
            // Responder
            SetPVarString(playerid, "reply_to_number",
                g_SMSInbox[playerid][listitem][sms_FromNumber]);
            new body[256];
            format(body, sizeof(body),
                "Mensagem de %s:\n"CSTR_GREY"%.160s\n\n"
                CSTR_WHITE"Digite sua resposta:",
                g_SMSInbox[playerid][listitem][sms_FromName],
                g_SMSInbox[playerid][listitem][sms_Message]);
            UI_ShowDialog(playerid, DIALOG_PHONE_SMS_NEW, DIALOG_STYLE_INPUT,
                "{ Responder SMS", body, "Enviar", "Cancelar");
            return true;
        }
        case DIALOG_PHONE_SMS_NEW: {
            if(!response) { Phone_ShowSMSInbox(playerid); return true; }
            new to_num[PHONE_NUMBER_LENGTH + 1];
            GetPVarString(playerid, "reply_to_number", to_num, sizeof(to_num));
            DeletePVar(playerid, "reply_to_number");
            if(!IsNullString(to_num) && !IsNullString(inputtext)) {
                Phone_SendSMS(playerid, to_num, inputtext);
            }
            return true;
        }
        case DIALOG_PHONE_ANN_LIST: {
            if(!response) { Phone_ShowMenu(playerid); return true; }
            if(listitem >= g_AnnounceCount) return true;
            // Mostrar detalhes do anuncio
            new price_fmt[24];
            FormatMoneyFull(g_Announcements[listitem][ann_Price], price_fmt, sizeof(price_fmt));
            new body[512];
            format(body, sizeof(body),
                CSTR_SYSTEM"[%s] %s\n\n"
                CSTR_WHITE"%s\n\n"
                CSTR_GREY"Publicado por: "CSTR_WHITE"%s\n"
                CSTR_GREY"Contato: "CSTR_SYSTEM"%s\n"
                CSTR_GREY"Valor: "CSTR_MONEY"%s\n"
                CSTR_GREY"Data: "CSTR_WHITE"%s",
                g_Announcements[listitem][ann_Type],
                g_Announcements[listitem][ann_Title],
                g_Announcements[listitem][ann_Body],
                g_Announcements[listitem][ann_CharName],
                g_Announcements[listitem][ann_Contact],
                price_fmt,
                g_Announcements[listitem][ann_Date]);
            UI_ShowDialog(playerid, DIALOG_PHONE_ANN_LIST + 1, DIALOG_STYLE_MSGBOX,
                "{ Anuncio", body, "Fechar", "");
            return true;
        }
        case DIALOG_PHONE_ANN_NEW: {
            if(!response) return true;
            // Formato: tipo|titulo|corpo|preco|contato
            new parts[5][80];
            new idx = 0, cur_part = 0, len = strlen(inputtext);
            for(new c = 0; c <= len && cur_part < 5; c++) {
                if(inputtext[c] == '|' || inputtext[c] == '\0') {
                    parts[cur_part][idx] = '\0';
                    cur_part++;
                    idx = 0;
                } else {
                    parts[cur_part][idx++] = inputtext[c];
                }
            }
            if(cur_part < 4) {
                UI_NotifyError(playerid, "Formato invalido.");
                return true;
            }
            Phone_PublishAnnounce(playerid, parts[0], parts[1], parts[2],
                strval(parts[3]), parts[4]);
            return true;
        }
        case DIALOG_PHONE_EMERGENCY: {
            // response = 190 (Policia), !response = 192 (SAMU)
            new service[16];
            if(response) format(service, sizeof(service), "Policia (190)");
            else         format(service, sizeof(service), "SAMU (192)");

            new Float:px, Float:py, Float:pz;
            GetPlayerPos(playerid, px, py, pz);
            new caller_name[MAX_PLAYER_NAME];
            GetPlayerName(playerid, caller_name, sizeof(caller_name));

            // Broadcast para admins/equipe
            new broadcast[128];
            format(broadcast, sizeof(broadcast),
                CSTR_ERROR"[EMERGENCIA] "CSTR_WHITE"%s chamou %s | "
                "Pos: %.1f, %.1f",
                caller_name, service, px, py);
            for(new i = 0; i < MAX_PLAYERS; i++) {
                if(IsValidPlayer(i) && Player_IsAdmin(i, ADMIN_HELPER))
                    SendClientMessage(i, COLOR_RED, broadcast);
            }
            UI_NotifySuccess(playerid, "Emergencia reportada. Ajuda a caminho.");
            return true;
        }
    }
    return false;
}

// Integracao com eventos
public OnPlayerLoaded(playerid) {
    Phone_Load(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    if(g_Phone[playerid][ph_InCall]) {
        new targetid = g_Phone[playerid][ph_CallTarget];
        if(IsValidPlayer(targetid)) {
            Phone_EndCall(targetid, true, "answered");
            UI_NotifyWarning(targetid, "A ligacao foi encerrada.");
        }
    }
    Phone_Init(playerid);
    return 1;
}

// Recarregar anuncios a cada 5 minutos
public OnScheduler5Minutes() {
    Phone_LoadAnnouncements();
}

// Comandos
CMD:celular(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Phone_ShowMenu(playerid);
    return 1;
}

CMD:sms(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new number_or_name[32], message[SMS_MAX_LENGTH + 1];
    if(sscanf(params, "s[32]s[160]", number_or_name, message)) {
        SendError(playerid, "Uso: /sms [numero ou nome] [mensagem]");
        return 1;
    }
    new resolved[PHONE_NUMBER_LENGTH + 1];
    Phone_GetContactNumber(playerid, number_or_name, resolved, sizeof(resolved));
    if(IsNullString(resolved)) {
        UI_NotifyError(playerid, "Numero nao encontrado.");
        return 1;
    }
    Phone_SendSMS(playerid, resolved, message);
    return 1;
}

CMD:ligar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new number_or_name[32];
    if(sscanf(params, "s[32]", number_or_name)) {
        SendError(playerid, "Uso: /ligar [numero ou nome do contato]");
        return 1;
    }
    new resolved[PHONE_NUMBER_LENGTH + 1];
    Phone_GetContactNumber(playerid, number_or_name, resolved, sizeof(resolved));
    if(IsNullString(resolved)) {
        UI_NotifyError(playerid, "Numero nao encontrado.");
        return 1;
    }
    Phone_Call(playerid, resolved);
    return 1;
}

CMD:desligar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!g_Phone[playerid][ph_InCall] && !g_Phone[playerid][ph_Ringing]) {
        SendError(playerid, "Voce nao esta em chamada.");
        return 1;
    }
    new targetid = g_Phone[playerid][ph_CallTarget];
    Phone_EndCall(playerid, true, "answered");
    if(IsValidPlayer(targetid)) {
        Phone_EndCall(targetid, false, "answered");
        UI_NotifyWarning(targetid, "Ligacao encerrada.");
    }
    SendInfo(playerid, "Ligacao encerrada.");
    return 1;
}

CMD:c(playerid, params[]) {
    // Falar na ligacao
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!g_Phone[playerid][ph_InCall]) {
        SendError(playerid, "Voce nao esta em chamada. Use /ligar para iniciar.");
        return 1;
    }
    if(IsNullString(params)) {
        SendError(playerid, "Uso: /c [mensagem]");
        return 1;
    }
    Phone_TalkInCall(playerid, params);
    return 1;
}

CMD:contato(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new name[32], number[PHONE_NUMBER_LENGTH + 1];
    if(sscanf(params, "s[32]s[12]", name, number)) {
        SendError(playerid, "Uso: /contato [nome] [numero]");
        return 1;
    }
    Phone_AddContact(playerid, name, number);
    return 1;
}

CMD:anuncio(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new body[] =
        "Formato: "CSTR_SYSTEM"tipo|titulo|descricao|preco|contato\n\n"
        CSTR_WHITE"Tipos: "CSTR_GREY"sale, buy, job, service, event, other\n\n"
        CSTR_WHITE"Exemplo:\n"
        CSTR_SYSTEM"sale|Vendo Carro|Infernus novo 50k|50000|11987654321\n\n"
        CSTR_GREY"Custo: $100  |  Valido: 24 horas";
    UI_ShowDialog(playerid, DIALOG_PHONE_ANN_NEW, DIALOG_STYLE_INPUT,
        "{ Publicar Anuncio", body, "Publicar", "Cancelar");
    return 1;
}

CMD:anuncios(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Phone_ShowAnnouncements(playerid);
    return 1;
}

CMD:meutelefone(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!g_Phone[playerid][ph_Loaded]) {
        SendError(playerid, "Sem celular.");
        return 1;
    }
    new msg[64];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[Celular] "CSTR_WHITE"Numero: "CSTR_SYSTEM"%s",
        g_Phone[playerid][ph_Number]);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}
