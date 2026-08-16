// ============================================================
//  Santa Aurora Roleplay ? Multas
//  Modulo : economy/fines_core.inc
//  Objetivo: multas de transito, policiais e administrativas
// ============================================================

#if defined _SA_FINES_CORE_INC
    #endinput
#endif
#define _SA_FINES_CORE_INC

// include removido
// include removido
// include removido
// include removido
// include removido
// include removido

// Dialogs de multas
#define DIALOG_FINES_LIST   710
#define DIALOG_FINES_PAY    711

// Cache de multas do jogador em memoria
#define MAX_FINES_CACHED    10

enum E_FINE_CACHE {
    fc_ID,
    fc_Type[16],
    fc_Reason[128],
    fc_Amount,
    fc_Status[12],
    fc_Date[20]
}

static g_PlayerFines[MAX_PLAYERS][MAX_FINES_CACHED][E_FINE_CACHE];
static g_PlayerFineCount[MAX_PLAYERS] = {0, ...};

// ------------------------------------------------------------
// Carregar multas do jogador
// ------------------------------------------------------------
stock Fines_Load(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(PlayerData[playerid][pCharacterID] == INVALID_DB_ID) return;

    g_PlayerFineCount[playerid] = 0;
    new query[256];
    format(query, sizeof(query),
        "SELECT `id`, `type`, `reason`, `amount`, `status`, `created_at` "
        "FROM `fines` WHERE `character_id`=%d AND `status`='pending' "
        "ORDER BY `created_at` DESC LIMIT %d",
        PlayerData[playerid][pCharacterID], MAX_FINES_CACHED);
    DB_Query(query, "OnFinesLoad", "i", playerid);
}

forward OnFinesLoad(playerid);
public OnFinesLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new rows = DB_NumRows();
    g_PlayerFineCount[playerid] = 0;
    for(new r = 0; r < rows && g_PlayerFineCount[playerid] < MAX_FINES_CACHED; r++) {
        new i = g_PlayerFineCount[playerid];
        g_PlayerFines[playerid][i][fc_ID]     = DB_GetInt("id", r);
        DB_GetString("type",       g_PlayerFines[playerid][i][fc_Type],   sizeof(g_PlayerFines[][][fc_Type]),   r);
        DB_GetString("reason",     g_PlayerFines[playerid][i][fc_Reason], sizeof(g_PlayerFines[][][fc_Reason]), r);
        g_PlayerFines[playerid][i][fc_Amount]  = DB_GetInt("amount",     r);
        DB_GetString("status",     g_PlayerFines[playerid][i][fc_Status], sizeof(g_PlayerFines[][][fc_Status]), r);
        DB_GetString("created_at", g_PlayerFines[playerid][i][fc_Date],   sizeof(g_PlayerFines[][][fc_Date]),   r);
        g_PlayerFineCount[playerid]++;
    }
    return 1;
}

// ------------------------------------------------------------
// Emitir multa
// ------------------------------------------------------------
stock Fines_Issue(playerid, officerid, fine_type[], reason[], amount) {
    if(!IsValidPlayer(playerid)) return false;
    if(amount <= 0) return false;

    new officer_id_db = 0;
    if(IsValidPlayer(officerid)) {
        officer_id_db = PlayerData[officerid][pCharacterID];
    }

    new escaped_type[20], escaped_reason[256];
    DB_Escape(fine_type, escaped_type, sizeof(escaped_type));
    DB_Escape(reason,    escaped_reason, sizeof(escaped_reason));

    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `fines` (`character_id`, `officer_id`, `type`, `reason`, "
        "`amount`, `status`, `due_date`) "
        "VALUES (%d, %d, '%s', '%s', %d, 'pending', DATE_ADD(NOW(), INTERVAL 30 DAY))",
        PlayerData[playerid][pCharacterID],
        officer_id_db, escaped_type, escaped_reason, amount);
    DB_Execute(query);

    // Notificar jogador
    new fmt_amount[24];
    FormatMoneyFull(amount, fmt_amount, sizeof(fmt_amount));
    new msg[128];
    format(msg, sizeof(msg),
        CSTR_ERROR"[MULTA] "CSTR_WHITE"Voce recebeu uma multa de "CSTR_ERROR"%s"
        CSTR_WHITE". Motivo: %s",
        fmt_amount, reason);
    SendClientMessage(playerid, COLOR_RED, msg);
    SendInfo(playerid, "Use /multas para ver e pagar suas multas.");

    Logger_Economy(playerid, "FINE_ISSUED", amount, "Sistema", PlayerData[playerid][pName]);

    // Recarregar cache
    Fines_Load(playerid);
    return true;
}

// ------------------------------------------------------------
// Mostrar multas pendentes
// ------------------------------------------------------------
stock Fines_ShowList(playerid) {
    if(!IsValidPlayer(playerid)) return;

    if(g_PlayerFineCount[playerid] == 0) {
        UI_NotifySuccess(playerid, "Voce nao tem multas pendentes.");
        return;
    }

    UI_ListReset();
    new total = 0;
    for(new i = 0; i < g_PlayerFineCount[playerid]; i++) {
        new fmt_amount[24];
        FormatMoneyFull(g_PlayerFines[playerid][i][fc_Amount], fmt_amount, sizeof(fmt_amount));
        new line[128];
        format(line, sizeof(line), "[%s] %s ? %s",
            g_PlayerFines[playerid][i][fc_Type],
            fmt_amount,
            g_PlayerFines[playerid][i][fc_Reason]);
        UI_ListAdd(line);
        total += g_PlayerFines[playerid][i][fc_Amount];
    }

    new title[64], fmt_total[24];
    FormatMoneyFull(total, fmt_total, sizeof(fmt_total));
    format(title, sizeof(title), "Multas Pendentes  Total: %s", fmt_total);
    UI_ListShow(playerid, DIALOG_FINES_LIST, title, "Pagar", "Fechar");
}

// ------------------------------------------------------------
// Pagar multa por indice
// ------------------------------------------------------------
stock Fines_Pay(playerid, fine_idx) {
    if(!IsValidPlayer(playerid)) return;
    if(fine_idx < 0 || fine_idx >= g_PlayerFineCount[playerid]) return;

    new fine_id = g_PlayerFines[playerid][fine_idx][fc_ID];
    new amount  = g_PlayerFines[playerid][fine_idx][fc_Amount];

    // Tentar pagar com carteira, depois banco
    new paid = false;
    if(PlayerData[playerid][pMoney] >= amount) {
        Player_RemoveMoney(playerid, amount);
        paid = true;
    } else if(g_BankData[playerid][bank_Balance] >= amount) {
        Bank_Debit(playerid, amount, "fine", "Pagamento de multa");
        paid = true;
    }

    if(!paid) {
        UI_NotifyError(playerid, "Saldo insuficiente para pagar esta multa.");
        return;
    }

    // Marcar como paga no banco
    new query[64];
    format(query, sizeof(query),
        "UPDATE `fines` SET `status`='paid', `paid_at`=NOW() WHERE `id`=%d",
        fine_id);
    DB_Execute(query);

    new fmt_amt[24];
    FormatMoneyFull(amount, fmt_amt, sizeof(fmt_amt));
    new msg[80];
    format(msg, sizeof(msg), CSTR_SUCCESS"Multa de %s paga com sucesso!", fmt_amt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);

    Logger_Economy(playerid, "FINE_PAID", amount, PlayerData[playerid][pName], "Sistema");

    // Recarregar lista
    Fines_Load(playerid);
}

// Pagar todas as multas de uma vez
stock Fines_PayAll(playerid) {
    if(!IsValidPlayer(playerid)) return;
    new total = 0;
    for(new i = 0; i < g_PlayerFineCount[playerid]; i++) {
        total += g_PlayerFines[playerid][i][fc_Amount];
    }
    if(total == 0) {
        UI_NotifySuccess(playerid, "Nenhuma multa pendente.");
        return;
    }

    new paid = false;
    if(PlayerData[playerid][pMoney] >= total) {
        Player_RemoveMoney(playerid, total);
        paid = true;
    } else if(g_BankData[playerid][bank_Balance] >= total) {
        Bank_Debit(playerid, total, "fine", "Pagamento de multas (todas)");
        paid = true;
    }

    if(!paid) {
        UI_NotifyError(playerid, "Saldo insuficiente para pagar todas as multas.");
        return;
    }

    // Marcar todas como pagas
    new query[128];
    format(query, sizeof(query),
        "UPDATE `fines` SET `status`='paid', `paid_at`=NOW() "
        "WHERE `character_id`=%d AND `status`='pending'",
        PlayerData[playerid][pCharacterID]);
    DB_Execute(query);

    new fmt_total[24];
    FormatMoneyFull(total, fmt_total, sizeof(fmt_total));
    new msg[80];
    format(msg, sizeof(msg), CSTR_SUCCESS"Todas as multas pagas: %s!", fmt_total);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);

    Logger_Economy(playerid, "FINES_PAID_ALL", total, PlayerData[playerid][pName], "Sistema");
    g_PlayerFineCount[playerid] = 0;
}

// Handler de dialog
stock Fines_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_FINES_LIST: {
            if(!response) return true;
            Fines_Pay(playerid, listitem);
            return true;
        }
    }
    return false;
}

// Total de multas pendentes
stock Fines_GetTotal(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new total = 0;
    for(new i = 0; i < g_PlayerFineCount[playerid]; i++) {
        total += g_PlayerFines[playerid][i][fc_Amount];
    }
    return total;
}

// Integracao
public OnPlayerLoaded(playerid) {
    Fines_Load(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    g_PlayerFineCount[playerid] = 0;
    return 1;
}

// Comandos
CMD:multas(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Fines_ShowList(playerid);
    return 1;
}

CMD:pagarmultas(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Fines_PayAll(playerid);
    return 1;
}
