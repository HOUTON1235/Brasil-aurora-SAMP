// ============================================================
//  Santa Aurora Roleplay ? Sistema de Governo
//  Modulo : government/government_core.inc
//  Objetivo: prefeitura, orcamento, impostos, decretos,
//            prefeito, eventos publicos, obras
// ============================================================

#if defined _SA_GOVERNMENT_CORE_INC
    #endinput
#endif
#define _SA_GOVERNMENT_CORE_INC

// include removido
// include removido
// include removido
// include removido
// include removido
// include removido

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define GOV_MAX_TAX_RATE        0.15    // maximo de imposto (15%)
#define GOV_MIN_TAX_RATE        0.01    // minimo (1%)
#define GOV_DEFAULT_TAX_RATE    0.05    // padrao (5%)
#define GOV_SALARY_INTERVAL     3600    // 1h entre salarios de func. publicos
#define GOV_TAX_INTERVAL        86400   // coleta de imposto diaria (24h)
#define GOV_DECREE_MAX_LEN      256
#define MAX_DECREES_CACHED      10

// Dialogs
#define DIALOG_GOV_MENU         1400
#define DIALOG_GOV_BUDGET       1401
#define DIALOG_GOV_TAX          1402
#define DIALOG_GOV_DECREE       1403
#define DIALOG_GOV_DECREE_NEW   1404
#define DIALOG_GOV_EVENTS       1405
#define DIALOG_GOV_INDICATORS   1406

// ------------------------------------------------------------
// Estado do governo em memoria
// ------------------------------------------------------------
static g_GovBalance       = 0;
static Float:g_TaxRate    = GOV_DEFAULT_TAX_RATE;
static g_GovLoaded        = false;

// Cache de decretos
static g_Decrees[MAX_DECREES_CACHED][GOV_DECREE_MAX_LEN];
static g_DecreeCount = 0;

// Prefeito atual (char_id)
static g_MayorCharID = 0;

// ------------------------------------------------------------
// Carregar dados do governo
// ------------------------------------------------------------
stock Government_Load() {
    DB_Query(
        "SELECT `balance`, `tax_collected`, `expenses` "
        "FROM `government_budget` WHERE `id`=1 LIMIT 1",
        "OnGovernmentLoaded", "");
}

forward OnGovernmentLoaded();
public OnGovernmentLoaded() {
    if(DB_HasResult()) {
        g_GovBalance = DB_GetInt("balance");
        g_GovLoaded  = true;
    } else {
        // Criar registro inicial
        DB_Execute("INSERT IGNORE INTO `government_budget` (`id`, `balance`) VALUES (1, 500000)");
        g_GovBalance = 500000;
        g_GovLoaded  = true;
    }
    // Carregar configuracoes de imposto
    new query[128];
    format(query, sizeof(query),
        "SELECT `config_value` FROM `server_config` WHERE `config_key`='economy.tax_rate'");
    DB_Query(query, "OnTaxRateLoaded", "");
    Logger_Info("GOVERNMENT", "Governo carregado.");
}

forward OnTaxRateLoaded();
public OnTaxRateLoaded() {
    if(DB_HasResult()) {
        new val[16];
        DB_GetString("config_value", val, sizeof(val));
        g_TaxRate = floatstr(val);
        g_TaxRate = ClampFloat(g_TaxRate, GOV_MIN_TAX_RATE, GOV_MAX_TAX_RATE);
    }
    return 1;
}

// Salvar
stock Government_Save() {
    if(!g_GovLoaded) return;
    new query[128];
    format(query, sizeof(query),
        "UPDATE `government_budget` SET `balance`=%d WHERE `id`=1",
        g_GovBalance);
    DB_Execute(query);
}

// ------------------------------------------------------------
// Verificar se e prefeito ou funcionario publico ativo
// ------------------------------------------------------------
stock Gov_IsMayor(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return (PlayerData[playerid][pCharacterID] == g_MayorCharID &&
            g_MayorCharID > 0);
}

stock Gov_IsCivilServant(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    // Job 16 = JOB_CIVIL_SERVANT
    return (PlayerData[playerid][pJobID] == 16 &&
            Player_GetState(playerid) == STATE_WORKING);
}

// Definir prefeito (comando de admin)
stock Gov_SetMayor(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_MayorCharID = PlayerData[playerid][pCharacterID];
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[GOVERNO] "CSTR_WHITE"%s foi eleito Prefeito de Santa Aurora!", name);
    SendClientMessageToAll(COLOR_SYSTEM, msg);
    Logger_Info("GOVERNMENT", "Novo prefeito definido");
}

// ------------------------------------------------------------
// IMPOSTOS ? cobrar dos jogadores periodicamente
// ------------------------------------------------------------
stock Government_CollectTaxes() {
    if(!g_GovLoaded) return;

    new total_collected = 0;
    new players_taxed   = 0;

    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(!PlayerData[i][pIsLogged]) continue;

        // Calcular imposto: % do banco
        new bank_bal = Bank_GetBalance(i);
        if(bank_bal < 1000) continue; // isencao para saldo baixo

        new tax_amount = floatround(float(bank_bal) * g_TaxRate / 100.0);
        if(tax_amount <= 0) continue;
        if(tax_amount > 5000) tax_amount = 5000; // teto por coleta

        if(Bank_GetBalance(i) >= tax_amount) {
            Bank_Debit(i, tax_amount, "tax", "Imposto de renda RP");
            g_GovBalance += tax_amount;
            total_collected += tax_amount;
            players_taxed++;

            new msg[64];
            format(msg, sizeof(msg),
                CSTR_WARNING"[GOVERNO] "CSTR_WHITE"Imposto de renda: "
                CSTR_ERROR"-%d"CSTR_WHITE" debitado.", tax_amount);
            SendClientMessage(i, COLOR_WARNING, msg);
        }
    }

    if(total_collected > 0) {
        // Atualizar banco
        new query[128];
        format(query, sizeof(query),
            "UPDATE `government_budget` SET `balance`=%d, "
            "`tax_collected`=`tax_collected`+%d WHERE `id`=1",
            g_GovBalance, total_collected);
        DB_Execute(query);

        new fmt[24];
        FormatMoneyFull(total_collected, fmt, sizeof(fmt));
        new log_msg[80];
        format(log_msg, sizeof(log_msg),
            "Impostos coletados: %s de %d jogadores.", fmt, players_taxed);
        Logger_Info("GOVERNMENT", log_msg);

        // Registrar acao
        new action_query[256];
        format(action_query, sizeof(action_query),
            "INSERT INTO `government_actions` (`character_id`, `action_type`, "
            "`description`, `amount`) VALUES (0, 'tax_collect', "
            "'Coleta automatica de impostos', %d)",
            total_collected);
        DB_Execute(action_query);
    }
}

// Cobrar imposto de propriedade (mensal)
stock Government_CollectPropertyTax() {
    // Implementacao simplificada ? cobrar por propriedade
    new tax_per_property = 500; // R$500 por imovel

    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i) || !PlayerData[i][pIsLogged]) continue;
        // Contar propriedades do jogador (simplificado ? usa funcao existente)
        new char_id = PlayerData[i][pCharacterID];
        new query[128];
        format(query, sizeof(query),
            "SELECT COUNT(*) as cnt FROM `properties` WHERE `owner_char_id`=%d",
            char_id);
        SetPVarInt(i, "prop_tax_playerid", i);
        DB_Query(query, "OnPropertyTaxCount", "ii", i, tax_per_property);
    }
}

forward OnPropertyTaxCount(playerid, tax_per);
public OnPropertyTaxCount(playerid, tax_per) {
    if(!IsValidPlayer(playerid)) return 0;
    new cnt = DB_GetInt("cnt");
    if(cnt <= 0) return 1;
    new total_tax = cnt * tax_per;
    if(Bank_GetBalance(playerid) >= total_tax) {
        Bank_Debit(playerid, total_tax, "tax", "Imposto predial");
        g_GovBalance += total_tax;
        new msg[64];
        format(msg, sizeof(msg),
            CSTR_WARNING"[GOVERNO] "CSTR_WHITE"Imposto predial: "
            CSTR_ERROR"-%d"CSTR_WHITE" (%d imoveis).",
            total_tax, cnt);
        SendClientMessage(playerid, COLOR_WARNING, msg);
    }
    return 1;
}

// ------------------------------------------------------------
// CONFIGURAR TAXA DE IMPOSTO (prefeito)
// ------------------------------------------------------------
stock Government_SetTaxRate(playerid, Float:new_rate) {
    if(!Gov_IsMayor(playerid) && !Player_IsAdmin(playerid, ADMIN_SUPERVISOR)) {
        UI_NotifyError(playerid, "Apenas o Prefeito pode alterar impostos.");
        return;
    }
    new_rate = ClampFloat(new_rate, GOV_MIN_TAX_RATE, GOV_MAX_TAX_RATE);
    g_TaxRate = new_rate;

    // Salvar na configuracao
    new query[128];
    format(query, sizeof(query),
        "UPDATE `server_config` SET `config_value`='%.4f' "
        "WHERE `config_key`='economy.tax_rate'",
        new_rate);
    DB_Execute(query);

    new msg[80];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[GOVERNO] "CSTR_WHITE"Taxa de imposto alterada para "
        CSTR_YELLOW"%.1f%%"CSTR_WHITE".",
        new_rate * 100.0);
    SendClientMessageToAll(COLOR_SYSTEM, msg);
    Logger_Info("GOVERNMENT", "Taxa de imposto alterada");

    new action_query[256];
    format(action_query, sizeof(action_query),
        "INSERT INTO `government_actions` (`character_id`, `action_type`, "
        "`description`, `amount`) VALUES (%d, 'tax_change', "
        "'Taxa alterada para %.2f%%', 0)",
        PlayerData[playerid][pCharacterID], new_rate * 100.0);
    DB_Execute(action_query);
}

// ------------------------------------------------------------
// DECRETOS
// ------------------------------------------------------------
stock Government_IssueDecree(playerid, decree_text[]) {
    if(!Gov_IsMayor(playerid) && !Player_IsAdmin(playerid, ADMIN_SUPERVISOR)) {
        UI_NotifyError(playerid, "Apenas o Prefeito pode emitir decretos.");
        return;
    }
    if(strlen(decree_text) < 10) {
        UI_NotifyError(playerid, "Decreto muito curto.");
        return;
    }

    // Broadcast
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));
    new msg[320];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"????????????????????????????????\n"
        "?      DECRETO MUNICIPAL       ?\n"
        CSTR_WHITE"Prefeito: %s\n\n"
        "%s\n"
        CSTR_SYSTEM"????????????????????????????????",
        name, decree_text);
    SendClientMessageToAll(COLOR_SYSTEM, msg);

    // Cache
    if(g_DecreeCount < MAX_DECREES_CACHED) {
        SafeStrCopy(g_Decrees[g_DecreeCount], decree_text, GOV_DECREE_MAX_LEN);
        g_DecreeCount++;
    }

    // Salvar
    new escaped[GOV_DECREE_MAX_LEN * 2];
    DB_Escape(decree_text, escaped, sizeof(escaped));
    new action_query[512];
    format(action_query, sizeof(action_query),
        "INSERT INTO `government_actions` (`character_id`, `action_type`, "
        "`description`) VALUES (%d, 'decree', '%s')",
        PlayerData[playerid][pCharacterID], escaped);
    DB_Execute(action_query);
    Logger_Info("GOVERNMENT", "Decreto emitido");
}

// ------------------------------------------------------------
// INDICADORES ECONOMICOS (painel interno)
// ------------------------------------------------------------
stock Government_ShowIndicators(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!Gov_IsMayor(playerid) && !Player_IsAdmin(playerid, ADMIN_ADMIN)) {
        UI_NotifyError(playerid, "Acesso restrito a administracao.");
        return;
    }

    // Calcular somas simples dos jogadores online
    new total_cash = 0, total_bank = 0, online_count = 0;
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i) || !PlayerData[i][pIsLogged]) continue;
        total_cash += PlayerData[i][pMoney];
        total_bank += Bank_GetBalance(i);
        online_count++;
    }

    new gov_fmt[24], cash_fmt[24], bank_fmt[24];
    FormatMoneyFull(g_GovBalance,  gov_fmt,  sizeof(gov_fmt));
    FormatMoneyFull(total_cash,    cash_fmt, sizeof(cash_fmt));
    FormatMoneyFull(total_bank,    bank_fmt, sizeof(bank_fmt));

    new avg_cash = (online_count > 0) ? total_cash / online_count : 0;
    new avg_bank = (online_count > 0) ? total_bank / online_count : 0;
    new avg_cash_fmt[24], avg_bank_fmt[24];
    FormatMoneyFull(avg_cash, avg_cash_fmt, sizeof(avg_cash_fmt));
    FormatMoneyFull(avg_bank, avg_bank_fmt, sizeof(avg_bank_fmt));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Indicadores Economicos ===\n\n"
        CSTR_WHITE"Orcamento do Governo: "CSTR_MONEY"%s\n"
        CSTR_WHITE"Taxa de Imposto: "CSTR_YELLOW"%.1f%%\n\n"
        CSTR_WHITE"Jogadores Online: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Dinheiro Fisico Total: "CSTR_MONEY"%s\n"
        CSTR_WHITE"Total em Bancos: "CSTR_MONEY"%s\n"
        CSTR_WHITE"Media Carteira: "CSTR_GREY"%s\n"
        CSTR_WHITE"Media Banco: "CSTR_GREY"%s\n\n"
        CSTR_WHITE"Decretos emitidos hoje: "CSTR_SYSTEM"%d",
        gov_fmt,
        g_TaxRate * 100.0,
        online_count,
        cash_fmt, bank_fmt,
        avg_cash_fmt, avg_bank_fmt,
        g_DecreeCount);

    UI_ShowDialog(playerid, DIALOG_GOV_INDICATORS, DIALOG_STYLE_MSGBOX,
        "{ Indicadores Economicos", body, "Fechar", "");
}

// ------------------------------------------------------------
// MENU DA PREFEITURA
// ------------------------------------------------------------
stock Government_ShowMenu(playerid) {
    if(!IsValidPlayer(playerid)) return;

    new gov_fmt[24];
    FormatMoneyFull(g_GovBalance, gov_fmt, sizeof(gov_fmt));
    new body[256];
    format(body, sizeof(body),
        CSTR_SYSTEM"Prefeitura de Santa Aurora\n\n"
        CSTR_WHITE"Orcamento: "CSTR_MONEY"%s\n"
        CSTR_WHITE"Imposto atual: "CSTR_YELLOW"%.1f%%",
        gov_fmt, g_TaxRate * 100.0);

    new options[256];
    format(options, sizeof(options),
        "Ver Decretos\n"
        "Indicadores Economicos\n"
        "Avisos Municipais\n");
    if(Gov_IsMayor(playerid) || Player_IsAdmin(playerid, ADMIN_SUPERVISOR)) {
        strcat(options, "Emitir Decreto\n",       sizeof(options));
        strcat(options, "Alterar Imposto\n",      sizeof(options));
        strcat(options, "Financiar Evento\n",     sizeof(options));
    }

    new full[512];
    format(full, sizeof(full), "%s\n%s", body, options);
    UI_ShowDialog(playerid, DIALOG_GOV_MENU, DIALOG_STYLE_LIST,
        "{ Prefeitura", options, "Selecionar", "Fechar");
}

// Mostrar decretos
static Government_ShowDecrees(playerid) {
    if(g_DecreeCount == 0) {
        UI_NotifyInfo(playerid, "Nenhum decreto emitido ainda.");
        return;
    }
    new body[1024];
    format(body, sizeof(body), CSTR_SYSTEM"=== Decretos Municipais ===\n\n");
    for(new i = 0; i < g_DecreeCount; i++) {
        new line[272];
        format(line, sizeof(line), CSTR_WHITE"%d. %s\n", i + 1, g_Decrees[i]);
        strcat(body, line, sizeof(body));
    }
    UI_ShowDialog(playerid, DIALOG_GOV_DECREE, DIALOG_STYLE_MSGBOX,
        "{ Decretos", body, "Fechar", "");
}

// Handler
stock Government_HandleDialog(playerid, dialogid, response, listitem,
                                    inputtext[]) {
    switch(dialogid) {
        case DIALOG_GOV_MENU: {
            if(!response) return true;
            new is_mayor = Gov_IsMayor(playerid) || Player_IsAdmin(playerid, ADMIN_SUPERVISOR);
            switch(listitem) {
                case 0: Government_ShowDecrees(playerid);
                case 1: Government_ShowIndicators(playerid);
                case 2: UI_NotifyInfo(playerid, "Nenhum aviso municipal ativo.");
                case 3: {
                    if(is_mayor) {
                        UI_ShowDialog(playerid, DIALOG_GOV_DECREE_NEW, DIALOG_STYLE_INPUT,
                            "{ Emitir Decreto",
                            "Digite o texto do decreto:\n"CSTR_GREY"(minimo 10 caracteres)",
                            "Publicar", "Cancelar");
                    }
                }
                case 4: {
                    if(is_mayor) {
                        UI_ShowDialog(playerid, DIALOG_GOV_TAX, DIALOG_STYLE_INPUT,
                            "{ Alterar Imposto",
                            "Digite a nova taxa de imposto (ex: 5 para 5%)\n"
                            CSTR_GREY"Minimo: 1%  Maximo: 15%",
                            "Confirmar", "Cancelar");
                    }
                }
                case 5: UI_NotifyInfo(playerid, "Sistema de eventos disponivel na Fase 8.");
            }
            return true;
        }
        case DIALOG_GOV_DECREE_NEW: {
            if(!response) return true;
            Government_IssueDecree(playerid, inputtext);
            return true;
        }
        case DIALOG_GOV_TAX: {
            if(!response) return true;
            new rate_int = strval(inputtext);
            if(rate_int < 1 || rate_int > 15) {
                UI_NotifyError(playerid, "Taxa invalida. Use entre 1 e 15.");
                return true;
            }
            Government_SetTaxRate(playerid, float(rate_int) / 100.0);
            return true;
        }
    }
    return false;
}

// Scheduler: coletar impostos uma vez por dia (a cada 5min para teste)
// Em producao, ajustar para o intervalo correto
public OnScheduler5Minutes() {
    Government_Save();
}

// Integracao
public OnGameModeInit() {
    Government_Load();
    return 1;
}

// Getters
stock Government_GetBalance() { return g_GovBalance; }
stock Float:Government_GetTaxRate() { return g_TaxRate; }

// Comandos
CMD:prefeitura(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Government_ShowMenu(playerid);
    return 1;
}

CMD:decreto(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!Gov_IsMayor(playerid) && !Player_IsAdmin(playerid, ADMIN_SUPERVISOR)) {
        SendError(playerid, "Apenas o Prefeito pode emitir decretos.");
        return 1;
    }
    if(IsNullString(params)) {
        SendError(playerid, "Uso: /decreto [texto do decreto]");
        return 1;
    }
    Government_IssueDecree(playerid, params);
    return 1;
}

CMD:imposto(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new rate_int;
    if(!Gov_IsMayor(playerid) && !Player_IsAdmin(playerid, ADMIN_SUPERVISOR)) {
        new msg[48];
        format(msg, sizeof(msg),
            CSTR_SYSTEM"Taxa de imposto atual: "CSTR_YELLOW"%.1f%%",
            g_TaxRate * 100.0);
        SendClientMessage(playerid, COLOR_INFO, msg);
        return 1;
    }
    if(sscanf(params, "i", rate_int)) {
        SendError(playerid, "Uso: /imposto [valor 1-15]");
        return 1;
    }
    Government_SetTaxRate(playerid, float(rate_int) / 100.0);
    return 1;
}

CMD:indicadores(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Government_ShowIndicators(playerid);
    return 1;
}

CMD:setprefeito(playerid, params[]) {
    if(!Player_IsAdmin(playerid, ADMIN_OWNER)) {
        SendError(playerid, "Apenas o Owner pode definir o prefeito.");
        return 1;
    }
    new targetid;
    if(sscanf(params, "i", targetid) || !IsValidPlayer(targetid)) {
        SendError(playerid, "Uso: /setprefeito [id]");
        return 1;
    }
    Gov_SetMayor(targetid);
    Logger_Admin(playerid, targetid, "SET_MAYOR", "Novo prefeito definido");
    return 1;
}
