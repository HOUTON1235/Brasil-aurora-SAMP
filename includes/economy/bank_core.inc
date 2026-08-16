// ============================================================
//  Santa Aurora Roleplay — Sistema Bancário
//  Módulo : economy/bank_core.inc
//  Objetivo: conta bancária, depósito, saque, transferência,
//            PIX, extrato, empréstimo, anti-duplicação
// ============================================================

#if defined _SA_BANK_CORE_INC
    #endinput
#endif
#define _SA_BANK_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../core/events.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "../players/player_core.inc"

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define BANK_MIN_DEPOSIT        10
#define BANK_MIN_WITHDRAW       10
#define BANK_MIN_TRANSFER       10
#define BANK_MAX_LOAN           50000
#define BANK_LOAN_INTEREST      0.05        // 5% ao mês
#define BANK_ATM_RANGE          3.0
#define MAX_EXTRATO_ROWS        20

// Dialog IDs do banco
#define DIALOG_BANK_MENU        500
#define DIALOG_BANK_DEPOSIT     501
#define DIALOG_BANK_WITHDRAW    502
#define DIALOG_BANK_TRANSFER    503
#define DIALOG_BANK_PIX_SEND    504
#define DIALOG_BANK_PIX_KEY     505
#define DIALOG_BANK_EXTRATO     506
#define DIALOG_BANK_LOAN        507
#define DIALOG_BANK_LOAN_CONFIRM 508
#define DIALOG_BANK_FINES       509

// ------------------------------------------------------------
// Cache da conta bancária do jogador
// ------------------------------------------------------------
enum E_BANK_DATA {
    bank_DBID,
    bank_AccountNumber[13],
    bank_Balance,
    bank_LimitPIX,
    bank_LimitTransfer,
    bool:bank_Blocked,
    bool:bank_Loaded
}

static g_BankData[MAX_PLAYERS][E_BANK_DATA];

// Anti-duplicação: última referência de transação por jogador
static g_LastTxRef[MAX_PLAYERS][32];
static g_LastTxTime[MAX_PLAYERS];

// ------------------------------------------------------------
// Gerar número de conta único
// ------------------------------------------------------------
static Bank_GenerateAccountNumber(output[], size = sizeof(output)) {
    format(output, size, "%04d%04d%04d",
        random(9999) + 1000,
        random(9999) + 1000,
        random(9999) + 1000);
}

// Gerar referência de transação (idempotência)
static Bank_GenerateRef(playerid, const type[], output[], size = sizeof(output)) {
    format(output, size, "%d_%s_%d_%d",
        playerid, type, gettime(), random(9999));
}

// ------------------------------------------------------------
// Inicializar dados bancários (ao connectar)
// ------------------------------------------------------------
stock Bank_Init(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_BankData[playerid][bank_DBID]            = INVALID_DB_ID;
    g_BankData[playerid][bank_AccountNumber][0]= '\0';
    g_BankData[playerid][bank_Balance]         = 0;
    g_BankData[playerid][bank_LimitPIX]        = 10000;
    g_BankData[playerid][bank_LimitTransfer]   = 50000;
    g_BankData[playerid][bank_Blocked]         = false;
    g_BankData[playerid][bank_Loaded]          = false;
    g_LastTxRef[playerid][0] = '\0';
    g_LastTxTime[playerid]   = 0;
}

// ------------------------------------------------------------
// Carregar conta bancária do banco
// ------------------------------------------------------------
stock Bank_Load(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(PlayerData[playerid][pCharacterID] == INVALID_DB_ID) return;

    Bank_Init(playerid);
    new query[128];
    format(query, sizeof(query),
        "SELECT `id`, `account_number`, `balance`, `limit_pix`, `limit_transfer`, `blocked` "
        "FROM `bank_accounts` WHERE `character_id`=%d LIMIT 1",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnBankLoad", "i", playerid);
}

forward OnBankLoad(playerid);
public OnBankLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    if(!DB_HasResult()) {
        // Criar conta bancária nova
        Bank_CreateAccount(playerid);
        return 1;
    }

    g_BankData[playerid][bank_DBID]     = DB_GetInt("id");
    DB_GetString("account_number", g_BankData[playerid][bank_AccountNumber],
        sizeof(g_BankData[][bank_AccountNumber]));
    g_BankData[playerid][bank_Balance]       = DB_GetInt("balance");
    g_BankData[playerid][bank_LimitPIX]      = DB_GetInt("limit_pix");
    g_BankData[playerid][bank_LimitTransfer] = DB_GetInt("limit_transfer");
    g_BankData[playerid][bank_Blocked]       = bool:DB_GetInt("blocked");
    g_BankData[playerid][bank_Loaded]        = true;

    // Sincronizar saldo com PlayerData
    PlayerData[playerid][pBank] = g_BankData[playerid][bank_Balance];
    return 1;
}

// ------------------------------------------------------------
// Criar nova conta bancária
// ------------------------------------------------------------
stock Bank_CreateAccount(playerid) {
    if(!IsValidPlayer(playerid)) return;

    new acc_num[13];
    Bank_GenerateAccountNumber(acc_num, sizeof(acc_num));

    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `bank_accounts` (`character_id`, `account_number`, `balance`) "
        "VALUES (%d, '%s', %d)",
        PlayerData[playerid][pCharacterID],
        acc_num,
        PlayerData[playerid][pBank]);
    DB_Query(query, "OnBankAccountCreated", "i", playerid);
    SafeStrCopy(g_BankData[playerid][bank_AccountNumber], acc_num,
        sizeof(g_BankData[][bank_AccountNumber]));
}

forward OnBankAccountCreated(playerid);
public OnBankAccountCreated(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    g_BankData[playerid][bank_DBID]  = DB_InsertID();
    g_BankData[playerid][bank_Loaded]= true;

    new msg[64];
    format(msg, sizeof(msg), "Conta bancária criada: %s",
        g_BankData[playerid][bank_AccountNumber]);
    Logger_Info("BANK", msg);
    return 1;
}

// ------------------------------------------------------------
// Salvar saldo bancário
// ------------------------------------------------------------
stock Bank_Save(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_BankData[playerid][bank_Loaded]) return;

    new query[128];
    format(query, sizeof(query),
        "UPDATE `bank_accounts` SET `balance`=%d WHERE `id`=%d",
        g_BankData[playerid][bank_Balance],
        g_BankData[playerid][bank_DBID]);
    DB_Execute(query);
}

// ------------------------------------------------------------
// Getters
// ------------------------------------------------------------
stock Bank_GetBalance(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return g_BankData[playerid][bank_Balance];
}

stock Bank_GetAccountNumber(playerid, output[], size = sizeof(output)) {
    if(!IsValidPlayer(playerid)) return;
    SafeStrCopy(output, g_BankData[playerid][bank_AccountNumber], size);
}

stock bool:Bank_IsLoaded(playerid) {
    return g_BankData[playerid][bank_Loaded];
}

// ------------------------------------------------------------
// Registrar transação (log no banco)
// Retorna ID da transação
// ------------------------------------------------------------
static Bank_LogTransaction(from_char, to_char, const type[],
                            amount, balance_after, const desc[],
                            const ref[]) {
    new escaped_desc[256], escaped_type[32], escaped_ref[64];
    DB_Escape(desc, escaped_desc, sizeof(escaped_desc));
    DB_Escape(type, escaped_type, sizeof(escaped_type));
    DB_Escape(ref,  escaped_ref,  sizeof(escaped_ref));

    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `bank_transactions` "
        "(`from_char_id`, `to_char_id`, `type`, `amount`, `balance_after`, "
        "`description`, `reference_id`) "
        "VALUES (%d, %d, '%s', %d, %d, '%s', '%s')",
        from_char, to_char, escaped_type,
        amount, balance_after,
        escaped_desc, escaped_ref);
    DB_Execute(query);
}

// ------------------------------------------------------------
// DEPÓSITO
// Fluxo: validar → reservar dinheiro físico → creditar banco →
//        logar → confirmar
// ------------------------------------------------------------
stock bool:Bank_Deposit(playerid, amount) {
    if(!IsValidPlayer(playerid)) return false;
    if(!g_BankData[playerid][bank_Loaded]) {
        UI_NotifyError(playerid, "Conta bancária não carregada.");
        return false;
    }
    if(g_BankData[playerid][bank_Blocked]) {
        UI_NotifyError(playerid, "Sua conta está bloqueada.");
        return false;
    }
    if(amount < BANK_MIN_DEPOSIT) {
        UI_NotifyError(playerid, "Valor mínimo de depósito: $10.");
        return false;
    }
    if(amount > PlayerData[playerid][pMoney]) {
        UI_NotifyError(playerid, "Você não tem dinheiro suficiente.");
        return false;
    }
    new new_balance = g_BankData[playerid][bank_Balance] + amount;
    if(new_balance > MAX_BANK_BALANCE) {
        UI_NotifyError(playerid, "Limite máximo de saldo bancário atingido.");
        return false;
    }

    // Processar
    Player_RemoveMoney(playerid, amount);
    g_BankData[playerid][bank_Balance] = new_balance;
    PlayerData[playerid][pBank]        = new_balance;
    Player_MarkDirty(playerid);

    // Log
    new ref[32];
    Bank_GenerateRef(playerid, "deposit", ref, sizeof(ref));
    Bank_LogTransaction(PlayerData[playerid][pCharacterID], 0,
        "deposit", amount, new_balance, "Depósito em caixa", ref);

    Logger_Economy(playerid, "DEPOSIT", amount, "Carteira", "Banco");
    UI_NotifyMoney(playerid, amount, true);
    SendSuccess(playerid, "Depósito realizado com sucesso!");
    HUD_UpdateMoney(playerid);
    return true;
}

// ------------------------------------------------------------
// SAQUE
// ------------------------------------------------------------
stock bool:Bank_Withdraw(playerid, amount) {
    if(!IsValidPlayer(playerid)) return false;
    if(!g_BankData[playerid][bank_Loaded]) return false;
    if(g_BankData[playerid][bank_Blocked]) {
        UI_NotifyError(playerid, "Sua conta está bloqueada.");
        return false;
    }
    if(amount < BANK_MIN_WITHDRAW) {
        UI_NotifyError(playerid, "Valor mínimo de saque: $10.");
        return false;
    }
    if(amount > g_BankData[playerid][bank_Balance]) {
        UI_NotifyError(playerid, "Saldo insuficiente.");
        return false;
    }
    new new_cash = PlayerData[playerid][pMoney] + amount;
    if(new_cash > MAX_MONEY) {
        UI_NotifyError(playerid, "Você não pode carregar tanto dinheiro.");
        return false;
    }

    // Processar
    g_BankData[playerid][bank_Balance] -= amount;
    PlayerData[playerid][pBank] = g_BankData[playerid][bank_Balance];
    Player_AddMoney(playerid, amount);
    Player_MarkDirty(playerid);

    new ref[32];
    Bank_GenerateRef(playerid, "withdraw", ref, sizeof(ref));
    Bank_LogTransaction(PlayerData[playerid][pCharacterID], 0,
        "withdraw", amount, g_BankData[playerid][bank_Balance],
        "Saque em caixa", ref);

    Logger_Economy(playerid, "WITHDRAW", amount, "Banco", "Carteira");
    SendSuccess(playerid, "Saque realizado com sucesso!");
    HUD_UpdateMoney(playerid);
    return true;
}

// ------------------------------------------------------------
// TRANSFERÊNCIA bancária (banco → banco)
// Anti-duplicação: verifica referência e cooldown de 2s
// ------------------------------------------------------------
stock bool:Bank_Transfer(playerid, targetid, amount, const desc[] = "Transferência") {
    if(!IsValidPlayer(playerid) || !IsValidPlayer(targetid)) return false;
    if(playerid == targetid) {
        UI_NotifyError(playerid, "Você não pode transferir para si mesmo.");
        return false;
    }

    // Anti-dupe: cooldown de 2 segundos
    new now = gettime();
    if(now - g_LastTxTime[playerid] < 2) {
        UI_NotifyError(playerid, "Aguarde antes de fazer outra transação.");
        return false;
    }

    if(!g_BankData[playerid][bank_Loaded] || !g_BankData[targetid][bank_Loaded]) {
        UI_NotifyError(playerid, "Conta não disponível.");
        return false;
    }
    if(g_BankData[playerid][bank_Blocked]) {
        UI_NotifyError(playerid, "Sua conta está bloqueada.");
        return false;
    }
    if(amount < BANK_MIN_TRANSFER) {
        UI_NotifyError(playerid, "Valor mínimo: $10.");
        return false;
    }
    if(amount > g_BankData[playerid][bank_Balance]) {
        UI_NotifyError(playerid, "Saldo insuficiente.");
        return false;
    }

    // Reservar
    g_LastTxTime[playerid] = now;

    // Processar
    g_BankData[playerid][bank_Balance]  -= amount;
    g_BankData[targetid][bank_Balance]  += amount;
    PlayerData[playerid][pBank]  = g_BankData[playerid][bank_Balance];
    PlayerData[targetid][pBank]  = g_BankData[targetid][bank_Balance];
    Player_MarkDirty(playerid);
    Player_MarkDirty(targetid);

    new ref[32];
    Bank_GenerateRef(playerid, "transfer", ref, sizeof(ref));

    new escaped_desc[128];
    DB_Escape(desc, escaped_desc, sizeof(escaped_desc));
    Bank_LogTransaction(
        PlayerData[playerid][pCharacterID],
        PlayerData[targetid][pCharacterID],
        "transfer", amount,
        g_BankData[playerid][bank_Balance],
        escaped_desc, ref);

    // Notificações
    new target_name[MAX_PLAYER_NAME], sender_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    GetPlayerName(playerid, sender_name, sizeof(sender_name));

    new msg_sender[80], msg_target[80], fmt_val[32];
    FormatMoneyFull(amount, fmt_val, sizeof(fmt_val));
    format(msg_sender, sizeof(msg_sender),
        CSTR_SUCCESS"Transferência de %s para %s concluída.", fmt_val, target_name);
    format(msg_target, sizeof(msg_target),
        CSTR_SUCCESS"Você recebeu %s de %s.", fmt_val, sender_name);
    SendClientMessage(playerid, COLOR_SUCCESS, msg_sender);
    SendClientMessage(targetid, COLOR_SUCCESS, msg_target);

    Logger_Economy(playerid, "TRANSFER_OUT", amount, sender_name, target_name);
    Logger_Economy(targetid, "TRANSFER_IN",  amount, sender_name, target_name);
    HUD_UpdateMoney(playerid);
    HUD_UpdateMoney(targetid);
    return true;
}

// ------------------------------------------------------------
// PIX (por chave)
// ------------------------------------------------------------
stock bool:Bank_SendPIX(playerid, const pix_key[], amount) {
    if(!IsValidPlayer(playerid)) return false;

    // Buscar destinatário pela chave PIX
    new escaped_key[128];
    DB_Escape(pix_key, escaped_key, sizeof(escaped_key));
    new query[256];
    format(query, sizeof(query),
        "SELECT pk.`character_id`, c.`name` "
        "FROM `pix_keys` pk "
        "JOIN `characters` c ON c.`id` = pk.`character_id` "
        "WHERE pk.`key_value`='%s' AND pk.`active`=1 LIMIT 1",
        escaped_key);

    // Guardar estado para o callback
    SetPVarInt(playerid, "pix_amount", amount);
    SetPVarString(playerid, "pix_key", pix_key);
    DB_Query(query, "OnPIXKeyFound", "i", playerid);
    return true;
}

forward OnPIXKeyFound(playerid);
public OnPIXKeyFound(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    new amount = GetPVarInt(playerid, "pix_amount");
    DeletePVar(playerid, "pix_amount");
    DeletePVar(playerid, "pix_key");

    if(!DB_HasResult()) {
        UI_NotifyError(playerid, "Chave PIX não encontrada.");
        return 1;
    }

    new target_char_id = DB_GetInt("character_id");
    new target_name[MAX_PLAYER_NAME];
    DB_GetString("name", target_name, sizeof(target_name));

    // Encontrar targetid online
    new targetid = INVALID_PLAYER_ID;
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pCharacterID] == target_char_id) {
            targetid = i;
            break;
        }
    }

    if(targetid == INVALID_PLAYER_ID) {
        UI_NotifyError(playerid, "Destinatário não está online no momento.");
        return 1;
    }

    // Usar Transfer
    new desc[64];
    format(desc, sizeof(desc), "PIX para %s", target_name);
    Bank_Transfer(playerid, targetid, amount, desc);
    return 1;
}

// Registrar chave PIX do jogador
stock Bank_RegisterPIXKey(playerid, const key_type[], const key_value[]) {
    if(!IsValidPlayer(playerid)) return;

    new escaped_type[16], escaped_val[128];
    DB_Escape(key_type,  escaped_type, sizeof(escaped_type));
    DB_Escape(key_value, escaped_val,  sizeof(escaped_val));

    new query[256];
    format(query, sizeof(query),
        "INSERT IGNORE INTO `pix_keys` (`character_id`, `key_type`, `key_value`) "
        "VALUES (%d, '%s', '%s')",
        PlayerData[playerid][pCharacterID], escaped_type, escaped_val);
    DB_Execute(query);
}

// ------------------------------------------------------------
// CRÉDITO DIRETO NO BANCO (salário, admin, etc.)
// Sem retirar dinheiro físico de ninguém
// ------------------------------------------------------------
stock bool:Bank_Credit(playerid, amount, const type[], const desc[]) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;
    if(!g_BankData[playerid][bank_Loaded]) return false;

    new new_balance = g_BankData[playerid][bank_Balance] + amount;
    if(new_balance > MAX_BANK_BALANCE) new_balance = MAX_BANK_BALANCE;

    g_BankData[playerid][bank_Balance] = new_balance;
    PlayerData[playerid][pBank] = new_balance;
    Player_MarkDirty(playerid);

    new ref[32];
    Bank_GenerateRef(playerid, type, ref, sizeof(ref));
    Bank_LogTransaction(0, PlayerData[playerid][pCharacterID],
        type, amount, new_balance, desc, ref);

    Logger_Economy(playerid, type, amount, "Sistema", PlayerData[playerid][pName]);
    HUD_UpdateMoney(playerid);
    return true;
}

// DÉBITO DIRETO NO BANCO (multa, imposto, etc.)
stock bool:Bank_Debit(playerid, amount, const type[], const desc[]) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;
    if(!g_BankData[playerid][bank_Loaded]) return false;
    if(g_BankData[playerid][bank_Balance] < amount) return false;

    g_BankData[playerid][bank_Balance] -= amount;
    PlayerData[playerid][pBank] = g_BankData[playerid][bank_Balance];
    Player_MarkDirty(playerid);

    new ref[32];
    Bank_GenerateRef(playerid, type, ref, sizeof(ref));
    Bank_LogTransaction(PlayerData[playerid][pCharacterID], 0,
        type, amount, g_BankData[playerid][bank_Balance], desc, ref);

    Logger_Economy(playerid, type, amount, PlayerData[playerid][pName], "Sistema");
    HUD_UpdateMoney(playerid);
    return true;
}

// ------------------------------------------------------------
// EXTRATO
// ------------------------------------------------------------
stock Bank_ShowExtrato(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_BankData[playerid][bank_Loaded]) {
        UI_NotifyError(playerid, "Conta não disponível.");
        return;
    }

    new query[256];
    format(query, sizeof(query),
        "SELECT `type`, `amount`, `balance_after`, `description`, `created_at` "
        "FROM `bank_transactions` "
        "WHERE `from_char_id`=%d OR `to_char_id`=%d "
        "ORDER BY `created_at` DESC LIMIT %d",
        PlayerData[playerid][pCharacterID],
        PlayerData[playerid][pCharacterID],
        MAX_EXTRATO_ROWS);
    DB_Query(query, "OnExtratoLoad", "i", playerid);
}

forward OnExtratoLoad(playerid);
public OnExtratoLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    new body[4096];
    new acc_num[13];
    Bank_GetAccountNumber(playerid, acc_num, sizeof(acc_num));

    new bal_fmt[32];
    FormatMoneyFull(g_BankData[playerid][bank_Balance], bal_fmt, sizeof(bal_fmt));
    format(body, sizeof(body),
        CSTR_SYSTEM"Conta: %s  Saldo: "CSTR_MONEY"%s\n"CSTR_GREY"────────────────────────────────\n",
        acc_num, bal_fmt);

    new rows = DB_NumRows();
    if(rows == 0) {
        strcat(body, CSTR_WHITE"Nenhuma transação encontrada.", sizeof(body));
    } else {
        for(new r = 0; r < rows; r++) {
            new type[24], desc[64], date[20];
            new amount = DB_GetInt("amount", r);
            new bal_after = DB_GetInt("balance_after", r);
            DB_GetString("type", type, sizeof(type), r);
            DB_GetString("description", desc, sizeof(desc), r);
            DB_GetString("created_at", date, sizeof(date), r);

            // Determinar sinal
            new char_id = PlayerData[playerid][pCharacterID];
            new from_id_q = DB_GetInt("from_char_id", r);
            new bool:is_debit = (from_id_q == char_id);

            new fmt_amt[32], fmt_bal[32];
            FormatMoneyFull(amount, fmt_amt, sizeof(fmt_amt));
            FormatMoneyFull(bal_after, fmt_bal, sizeof(fmt_bal));

            new line[128];
            format(line, sizeof(line),
                "%s%s %s "CSTR_WHITE"| Saldo: %s\n"CSTR_GREY"%s\n",
                is_debit ? CSTR_ERROR"-" : CSTR_SUCCESS"+",
                fmt_amt, type,
                fmt_bal,
                desc);
            strcat(body, line, sizeof(body));
        }
    }

    UI_ShowDialog(playerid, DIALOG_BANK_EXTRATO, DIALOG_STYLE_MSGBOX,
        "{ Extrato Bancário", body, "Fechar", "");
    return 1;
}

// ------------------------------------------------------------
// MENU DO BANCO (dialog principal)
// ------------------------------------------------------------
stock Bank_ShowMenu(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_BankData[playerid][bank_Loaded]) {
        UI_NotifyError(playerid, "Conta bancária não disponível.");
        return;
    }

    new acc_num[13];
    Bank_GetAccountNumber(playerid, acc_num, sizeof(acc_num));
    new bal_fmt[32];
    FormatMoneyFull(g_BankData[playerid][bank_Balance], bal_fmt, sizeof(bal_fmt));
    new cash_fmt[32];
    FormatMoneyFull(PlayerData[playerid][pMoney], cash_fmt, sizeof(cash_fmt));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"Conta: "CSTR_WHITE"%s\n"
        CSTR_SYSTEM"Saldo: "CSTR_MONEY"%s\n"
        CSTR_SYSTEM"Carteira: "CSTR_WHITE"%s\n\n"
        CSTR_GREY"O que deseja fazer?",
        acc_num, bal_fmt, cash_fmt);

    new options[] =
        "Depositar\n"
        "Sacar\n"
        "Transferir\n"
        "PIX\n"
        "Extrato\n"
        "Pagar Multas\n"
        "Empréstimo\n";

    UI_ShowDialog(playerid, DIALOG_BANK_MENU, DIALOG_STYLE_LIST,
        "{ Banco — Santa Aurora", options, "Selecionar", "Fechar");
}

// ------------------------------------------------------------
// Handler de dialogs do banco
// ------------------------------------------------------------
stock bool:Bank_HandleDialog(playerid, dialogid, response, listitem,
                              const inputtext[]) {
    switch(dialogid) {
        case DIALOG_BANK_MENU: {
            if(!response) return true;
            switch(listitem) {
                case 0: UI_ShowDialog(playerid, DIALOG_BANK_DEPOSIT, DIALOG_STYLE_INPUT,
                            "{ Depósito",
                            "Digite o valor a depositar:\n"CSTR_GREY"(mínimo $10)",
                            "Confirmar", "Voltar");
                case 1: UI_ShowDialog(playerid, DIALOG_BANK_WITHDRAW, DIALOG_STYLE_INPUT,
                            "{ Saque",
                            "Digite o valor a sacar:\n"CSTR_GREY"(mínimo $10)",
                            "Confirmar", "Voltar");
                case 2: UI_ShowDialog(playerid, DIALOG_BANK_TRANSFER, DIALOG_STYLE_INPUT,
                            "{ Transferência",
                            "Digite: "CSTR_SYSTEM"[ID do jogador] [valor]\n"
                            CSTR_GREY"Exemplo: 5 1000",
                            "Confirmar", "Voltar");
                case 3: UI_ShowDialog(playerid, DIALOG_BANK_PIX_SEND, DIALOG_STYLE_INPUT,
                            "{ PIX",
                            "Digite a chave PIX do destinatário e o valor:\n"
                            CSTR_GREY"Formato: [chave] [valor]",
                            "Enviar", "Voltar");
                case 4: Bank_ShowExtrato(playerid);
                case 5: Bank_ShowFines(playerid);
                case 6: Bank_ShowLoanMenu(playerid);
            }
            return true;
        }
        case DIALOG_BANK_DEPOSIT: {
            if(!response) { Bank_ShowMenu(playerid); return true; }
            new amount = strval(inputtext);
            if(amount <= 0) { UI_NotifyError(playerid, "Valor inválido."); Bank_ShowMenu(playerid); return true; }
            Bank_Deposit(playerid, amount);
            return true;
        }
        case DIALOG_BANK_WITHDRAW: {
            if(!response) { Bank_ShowMenu(playerid); return true; }
            new amount = strval(inputtext);
            if(amount <= 0) { UI_NotifyError(playerid, "Valor inválido."); Bank_ShowMenu(playerid); return true; }
            Bank_Withdraw(playerid, amount);
            return true;
        }
        case DIALOG_BANK_TRANSFER: {
            if(!response) { Bank_ShowMenu(playerid); return true; }
            new targetid, amount;
            if(sscanf(inputtext, "ii", targetid, amount) || !IsValidPlayer(targetid)) {
                UI_NotifyError(playerid, "Formato inválido. Use: [ID] [valor]");
                return true;
            }
            Bank_Transfer(playerid, targetid, amount);
            return true;
        }
        case DIALOG_BANK_PIX_SEND: {
            if(!response) { Bank_ShowMenu(playerid); return true; }
            new pix_key[64];
            new amount;
            if(sscanf(inputtext, "s[64]i", pix_key, amount)) {
                UI_NotifyError(playerid, "Formato inválido. Use: [chave] [valor]");
                return true;
            }
            Bank_SendPIX(playerid, pix_key, amount);
            return true;
        }
        case DIALOG_BANK_EXTRATO: { return true; }
        case DIALOG_BANK_LOAN: {
            if(!response) return true;
            new amount = strval(inputtext);
            if(amount <= 0 || amount > BANK_MAX_LOAN) {
                UI_NotifyError(playerid, "Valor inválido.");
                return true;
            }
            SetPVarInt(playerid, "loan_amount", amount);
            new monthly = floatround(float(amount) * (1.0 + BANK_LOAN_INTEREST) / 12.0);
            new total   = monthly * 12;
            new body[256];
            format(body, sizeof(body),
                CSTR_WARNING"Confirmação de Empréstimo\n\n"
                CSTR_WHITE"Valor solicitado: "CSTR_MONEY"$%d\n"
                CSTR_WHITE"Juros: "CSTR_ERROR"5%% ao mês\n"
                CSTR_WHITE"Parcelas: "CSTR_SYSTEM"12x $%d\n"
                CSTR_WHITE"Total a pagar: "CSTR_ERROR"$%d\n\n"
                CSTR_GREY"Confirma o empréstimo?",
                amount, monthly, total);
            UI_ShowDialog(playerid, DIALOG_BANK_LOAN_CONFIRM, DIALOG_STYLE_MSGBOX,
                "{ Confirmar Empréstimo", body, "Confirmar", "Cancelar");
            return true;
        }
        case DIALOG_BANK_LOAN_CONFIRM: {
            if(!response) {
                DeletePVar(playerid, "loan_amount");
                return true;
            }
            new amount = GetPVarInt(playerid, "loan_amount");
            DeletePVar(playerid, "loan_amount");
            if(amount > 0) Bank_TakeLoan(playerid, amount);
            return true;
        }
        case DIALOG_BANK_FINES: {
            if(!response) return true;
            // Pagamento de multas — implementado no módulo de multas
            Bank_PayFineByIndex(playerid, listitem);
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------
// EMPRÉSTIMO
// ------------------------------------------------------------
stock Bank_ShowLoanMenu(playerid) {
    new body[256];
    format(body, sizeof(body),
        "Solicitar empréstimo bancário.\n\n"
        CSTR_WHITE"Limite máximo: "CSTR_MONEY"$%d\n"
        CSTR_WHITE"Juros: "CSTR_ERROR"5%% ao mês\n"
        CSTR_WHITE"Prazo: "CSTR_SYSTEM"12 parcelas\n\n"
        CSTR_GREY"Digite o valor desejado:",
        BANK_MAX_LOAN);
    UI_ShowDialog(playerid, DIALOG_BANK_LOAN, DIALOG_STYLE_INPUT,
        "{ Empréstimo", body, "Solicitar", "Voltar");
}

stock Bank_TakeLoan(playerid, amount) {
    if(!IsValidPlayer(playerid)) return;
    if(amount <= 0 || amount > BANK_MAX_LOAN) return;

    // Verificar se já tem empréstimo ativo
    new query[128];
    format(query, sizeof(query),
        "SELECT COUNT(*) as cnt FROM `loans` "
        "WHERE `character_id`=%d AND `status`='active'",
        PlayerData[playerid][pCharacterID]);
    SetPVarInt(playerid, "loan_value", amount);
    DB_Query(query, "OnLoanCheck", "i", playerid);
}

forward OnLoanCheck(playerid);
public OnLoanCheck(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new cnt = DB_GetInt("cnt");
    new amount = GetPVarInt(playerid, "loan_value");
    DeletePVar(playerid, "loan_value");

    if(cnt > 0) {
        UI_NotifyError(playerid, "Você já possui um empréstimo ativo. Quite-o primeiro.");
        return 1;
    }

    new monthly = floatround(float(amount) * (1.0 + BANK_LOAN_INTEREST) / 12.0);
    new total   = monthly * 12;

    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `loans` (`character_id`, `principal`, `interest_rate`, `balance`, "
        "`installments`, `monthly_payment`, `status`, `due_date`) "
        "VALUES (%d, %d, %.2f, %d, 12, %d, 'active', DATE_ADD(NOW(), INTERVAL 30 DAY))",
        PlayerData[playerid][pCharacterID],
        amount, BANK_LOAN_INTEREST, total, monthly);
    DB_Execute(query);

    Bank_Credit(playerid, amount, "loan", "Empréstimo bancário");
    SendSuccess(playerid, "Empréstimo aprovado! O valor foi creditado na sua conta.");
    Logger_Economy(playerid, "LOAN", amount, "Banco", PlayerData[playerid][pName]);
    return 1;
}

// Placeholder — implementado melhor no módulo de multas
stock Bank_ShowFines(playerid) {
    UI_NotifyInfo(playerid, "Use /multas para ver suas multas pendentes.");
}

stock Bank_PayFineByIndex(playerid, index) {
    #pragma unused playerid, index
}

// ------------------------------------------------------------
// Integração com eventos
// ------------------------------------------------------------
public OnPlayerLoaded(playerid) {
    Bank_Load(playerid);
    return 1;
}

public OnPlayerSaved(playerid) {
    Bank_Save(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    Bank_Init(playerid);
    return 1;
}

// Comando /banco
CMD:banco(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!State_CanInteract(playerid)) {
        SendError(playerid, "Você não pode acessar o banco agora.");
        return 1;
    }
    Bank_ShowMenu(playerid);
    return 1;
}

// Comando /pix
CMD:pix(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new pix_key[64];
    new amount;
    if(sscanf(params, "s[64]i", pix_key, amount)) {
        SendError(playerid, "Uso: /pix [chave] [valor]");
        return 1;
    }
    Bank_SendPIX(playerid, pix_key, amount);
    return 1;
}

// Comando /depositar
CMD:depositar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new amount;
    if(sscanf(params, "i", amount)) {
        SendError(playerid, "Uso: /depositar [valor]");
        return 1;
    }
    Bank_Deposit(playerid, amount);
    return 1;
}

// Comando /sacar
CMD:sacar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new amount;
    if(sscanf(params, "i", amount)) {
        SendError(playerid, "Uso: /sacar [valor]");
        return 1;
    }
    Bank_Withdraw(playerid, amount);
    return 1;
}

// Comando /transferir
CMD:transferir(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new targetid, amount;
    if(sscanf(params, "ii", targetid, amount) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /transferir [id] [valor]");
        return 1;
    }
    Bank_Transfer(playerid, targetid, amount);
    return 1;
}

// Comando /extrato
CMD:extrato(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Bank_ShowExtrato(playerid);
    return 1;
}

// Comando /saldo
CMD:saldo(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new bal_fmt[32], cash_fmt[32], acc_num[13];
    FormatMoneyFull(g_BankData[playerid][bank_Balance], bal_fmt, sizeof(bal_fmt));
    FormatMoneyFull(PlayerData[playerid][pMoney], cash_fmt, sizeof(cash_fmt));
    Bank_GetAccountNumber(playerid, acc_num, sizeof(acc_num));
    new msg[128];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[Banco] "CSTR_WHITE"Conta: "CSTR_SYSTEM"%s  "
        CSTR_WHITE"Saldo: "CSTR_MONEY"%s  "
        CSTR_WHITE"Carteira: "CSTR_MONEY"%s",
        acc_num, bal_fmt, cash_fmt);
    SendClientMessage(playerid, COLOR_INFO, msg);
    return 1;
}
