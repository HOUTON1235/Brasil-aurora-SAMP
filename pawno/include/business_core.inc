// ============================================================
//  Santa Aurora Roleplay ? Sistema de Empresas
//  Modulo : businesses/business_core.inc
//  Objetivo: compra, gestao, funcionarios, estoque, caixa,
//            pagamento de salarios, impostos, transacoes
// ============================================================

#if defined _SA_BUSINESS_CORE_INC
    #endinput
#endif
#define _SA_BUSINESS_CORE_INC

// include removido
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
#define MAX_BUSINESSES          50
#define MAX_BIZ_EMPLOYEES       20
#define MAX_BIZ_STOCK_ITEMS     30
#define MAX_PLAYER_BUSINESSES   3
#define BIZ_ENTER_RANGE         3.0
#define BIZ_SALARY_INTERVAL     3600    // 1h
#define BIZ_TAX_INTERVAL        86400   // 24h

// Dialogs
#define DIALOG_BIZ_MENU         1000
#define DIALOG_BIZ_LIST         1001
#define DIALOG_BIZ_BUY          1002
#define DIALOG_BIZ_STOCK        1003
#define DIALOG_BIZ_EMPLOYEES    1004
#define DIALOG_BIZ_HIRE         1005
#define DIALOG_BIZ_SALARY       1006
#define DIALOG_BIZ_DEPOSIT      1007
#define DIALOG_BIZ_SELL_ITEM    1008

// ------------------------------------------------------------
// Cache de tipo de empresa
// ------------------------------------------------------------
enum E_BIZ_TYPE {
    bt_ID,
    bt_Name[32],
    bt_DisplayName[48],
    bt_Category[16],
    bt_BasePrice,
    bt_DailyExpenses,
    Float:bt_TaxRate
}

// ------------------------------------------------------------
// Cache de empresa
// ------------------------------------------------------------
enum E_BUSINESS {
    biz_ID,
    biz_OwnerCharID,
    biz_TypeID,
    biz_TypeName[32],
    biz_Name[48],
    biz_Cash,
    biz_Level,
    Float:biz_PosX,
    Float:biz_PosY,
    Float:biz_PosZ,
    Float:biz_IntX,
    Float:biz_IntY,
    Float:biz_IntZ,
    biz_IntWorld,
    biz_IntInterior,
    bool:biz_Open,
    bool:biz_ForSale,
    biz_Price,
    bool:biz_Loaded,
    bool:biz_Dirty
}

// Cache de funcionario
enum E_BIZ_EMP {
    emp_ID,
    emp_CharID,
    emp_Name[MAX_PLAYER_NAME],
    emp_Rank[32],
    emp_Salary,
    bool:emp_Active
}

// Cache de estoque
enum E_BIZ_STOCK {
    bst_ItemID,
    bst_Quantity,
    bst_BuyPrice,
    bst_SellPrice
}

static g_Businesses[MAX_BUSINESSES][E_BUSINESS];
static g_BizCount = 0;

static g_BizEmployees[MAX_BUSINESSES][MAX_BIZ_EMPLOYEES][E_BIZ_EMP];
static g_BizEmpCount[MAX_BUSINESSES] = {0, ...};

static g_BizStock[MAX_BUSINESSES][MAX_BIZ_STOCK_ITEMS][E_BIZ_STOCK];
static g_BizStockCount[MAX_BUSINESSES] = {0, ...};

// Empresa em que o jogador esta dentro
static g_PlayerInsideBiz[MAX_PLAYERS] = {-1, ...};
// Empresa selecionada no menu
static g_MenuBizIdx[MAX_PLAYERS] = {-1, ...};

// ------------------------------------------------------------
// Carregar todas as empresas
// ------------------------------------------------------------
stock Business_LoadAll() {
    DB_Query(
        "SELECT b.`id`, b.`owner_char_id`, b.`type_id`, bt.`name` as type_name, "
        "b.`name`, b.`cash`, b.`level`, "
        "b.`pos_x`, b.`pos_y`, b.`pos_z`, "
        "b.`int_pos_x`, b.`int_pos_y`, b.`int_pos_z`, "
        "b.`int_world`, b.`int_interior`, "
        "b.`open`, b.`for_sale`, b.`price` "
        "FROM `businesses` b "
        "JOIN `business_types` bt ON bt.`id`=b.`type_id` "
        "ORDER BY b.`id` ASC LIMIT " #MAX_BUSINESSES,
        "OnBusinessesLoaded", "");
}

forward OnBusinessesLoaded();
public OnBusinessesLoaded() {
    g_BizCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_BizCount < MAX_BUSINESSES; r++) {
        new i = g_BizCount;
        g_Businesses[i][biz_ID]          = DB_GetInt("id",            r);
        g_Businesses[i][biz_OwnerCharID] = DB_GetInt("owner_char_id", r);
        g_Businesses[i][biz_TypeID]      = DB_GetInt("type_id",       r);
        DB_GetString("type_name", g_Businesses[i][biz_TypeName], sizeof(g_Businesses[][biz_TypeName]), r);
        DB_GetString("name",      g_Businesses[i][biz_Name],     sizeof(g_Businesses[][biz_Name]),     r);
        g_Businesses[i][biz_Cash]        = DB_GetInt("cash",          r);
        g_Businesses[i][biz_Level]       = DB_GetInt("level",         r);
        g_Businesses[i][biz_PosX]        = DB_GetFloat("pos_x",       r);
        g_Businesses[i][biz_PosY]        = DB_GetFloat("pos_y",       r);
        g_Businesses[i][biz_PosZ]        = DB_GetFloat("pos_z",       r);
        g_Businesses[i][biz_IntX]        = DB_GetFloat("int_pos_x",   r);
        g_Businesses[i][biz_IntY]        = DB_GetFloat("int_pos_y",   r);
        g_Businesses[i][biz_IntZ]        = DB_GetFloat("int_pos_z",   r);
        g_Businesses[i][biz_IntWorld]    = DB_GetInt("int_world",     r);
        g_Businesses[i][biz_IntInterior] = DB_GetInt("int_interior",  r);
        g_Businesses[i][biz_Open]        = bool:DB_GetInt("open",     r);
        g_Businesses[i][biz_ForSale]     = bool:DB_GetInt("for_sale", r);
        g_Businesses[i][biz_Price]       = DB_GetInt("price",         r);
        g_Businesses[i][biz_Loaded]      = true;
        g_Businesses[i][biz_Dirty]       = false;
        g_BizEmpCount[i]                 = 0;
        g_BizStockCount[i]               = 0;
        g_BizCount++;
    }

    // Agora carregar funcionarios e estoque
    DB_Query(
        "SELECT `id`, `business_id`, `character_id`, `rank`, `salary`, `active`, "
        "(SELECT `name` FROM `characters` WHERE `id`=be.`character_id`) as char_name "
        "FROM `business_employees` be WHERE `active`=1",
        "OnBizEmployeesLoaded", "");

    new msg[48];
    format(msg, sizeof(msg), "%d empresas carregadas.", g_BizCount);
    Logger_Info("BUSINESS", msg);
}

forward OnBizEmployeesLoaded();
public OnBizEmployeesLoaded() {
    new rows = DB_NumRows();
    for(new r = 0; r < rows; r++) {
        new biz_id = DB_GetInt("business_id", r);
        new b_idx  = Business_GetIdx(biz_id);
        if(b_idx < 0) continue;
        new ei = g_BizEmpCount[b_idx];
        if(ei >= MAX_BIZ_EMPLOYEES) continue;
        g_BizEmployees[b_idx][ei][emp_ID]     = DB_GetInt("id",          r);
        g_BizEmployees[b_idx][ei][emp_CharID] = DB_GetInt("character_id",r);
        DB_GetString("char_name", g_BizEmployees[b_idx][ei][emp_Name],   sizeof(g_BizEmployees[][][emp_Name]),  r);
        DB_GetString("rank",      g_BizEmployees[b_idx][ei][emp_Rank],   sizeof(g_BizEmployees[][][emp_Rank]),  r);
        g_BizEmployees[b_idx][ei][emp_Salary] = DB_GetInt("salary", r);
        g_BizEmployees[b_idx][ei][emp_Active] = bool:DB_GetInt("active", r);
        g_BizEmpCount[b_idx]++;
    }

    DB_Query(
        "SELECT `business_id`, `item_id`, `quantity`, `buy_price`, `sell_price` "
        "FROM `business_inventory` WHERE `quantity`>0",
        "OnBizStockLoaded", "");
}

forward OnBizStockLoaded();
public OnBizStockLoaded() {
    new rows = DB_NumRows();
    for(new r = 0; r < rows; r++) {
        new biz_id = DB_GetInt("business_id", r);
        new b_idx  = Business_GetIdx(biz_id);
        if(b_idx < 0) continue;
        new si = g_BizStockCount[b_idx];
        if(si >= MAX_BIZ_STOCK_ITEMS) continue;
        g_BizStock[b_idx][si][bst_ItemID]   = DB_GetInt("item_id",    r);
        g_BizStock[b_idx][si][bst_Quantity] = DB_GetInt("quantity",   r);
        g_BizStock[b_idx][si][bst_BuyPrice] = DB_GetInt("buy_price",  r);
        g_BizStock[b_idx][si][bst_SellPrice]= DB_GetInt("sell_price", r);
        g_BizStockCount[b_idx]++;
    }
}

// Helper: encontrar indice pelo ID
stock Business_GetIdx(biz_id) {
    for(new i = 0; i < g_BizCount; i++) {
        if(g_Businesses[i][biz_ID] == biz_id) return i;
    }
    return -1;
}

// Salvar empresa
stock Business_Save(b_idx) {
    if(b_idx < 0 || !g_Businesses[b_idx][biz_Dirty]) return;
    new query[256];
    format(query, sizeof(query),
        "UPDATE `businesses` SET "
        "`owner_char_id`=%d, `cash`=%d, `open`=%d, `for_sale`=%d "
        "WHERE `id`=%d",
        g_Businesses[b_idx][biz_OwnerCharID],
        g_Businesses[b_idx][biz_Cash],
        g_Businesses[b_idx][biz_Open] ? 1 : 0,
        g_Businesses[b_idx][biz_ForSale] ? 1 : 0,
        g_Businesses[b_idx][biz_ID]);
    DB_Execute(query);
    g_Businesses[b_idx][biz_Dirty] = false;
}

stock Business_SaveAll() {
    for(new i = 0; i < g_BizCount; i++) {
        if(g_Businesses[i][biz_Dirty]) Business_Save(i);
    }
}

// ------------------------------------------------------------
// Encontrar empresa mais proxima
// ------------------------------------------------------------
stock Business_FindNearest(playerid) {
    if(!IsValidPlayer(playerid)) return -1;
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new best = -1;
    new Float:best_dist = BIZ_ENTER_RANGE + 1.0;
    for(new i = 0; i < g_BizCount; i++) {
        new Float:dist = GetDistance3D(px, py, pz,
            g_Businesses[i][biz_PosX],
            g_Businesses[i][biz_PosY],
            g_Businesses[i][biz_PosZ]);
        if(dist < best_dist) { best_dist = dist; best = i; }
    }
    return best;
}

// Contar empresas de um dono
stock Business_CountOwned(char_id) {
    new count = 0;
    for(new i = 0; i < g_BizCount; i++) {
        if(g_Businesses[i][biz_OwnerCharID] == char_id) count++;
    }
    return count;
}

// ------------------------------------------------------------
// Verificar se jogador e dono ou funcionario
// ------------------------------------------------------------
stock Business_IsOwner(playerid, b_idx) {
    if(!IsValidPlayer(playerid) || b_idx < 0) return false;
    return (g_Businesses[b_idx][biz_OwnerCharID] == PlayerData[playerid][pCharacterID]);
}

stock Business_IsEmployee(playerid, b_idx) {
    if(!IsValidPlayer(playerid) || b_idx < 0) return false;
    new char_id = PlayerData[playerid][pCharacterID];
    for(new e = 0; e < g_BizEmpCount[b_idx]; e++) {
        if(g_BizEmployees[b_idx][e][emp_CharID] == char_id &&
           g_BizEmployees[b_idx][e][emp_Active]) return true;
    }
    return false;
}

// ------------------------------------------------------------
// COMPRAR empresa
// ------------------------------------------------------------
stock Business_Buy(playerid, b_idx) {
    if(!IsValidPlayer(playerid) || b_idx < 0) return;
    if(!g_Businesses[b_idx][biz_ForSale]) {
        UI_NotifyError(playerid, "Esta empresa nao esta a venda.");
        return;
    }
    if(Business_CountOwned(PlayerData[playerid][pCharacterID]) >= MAX_PLAYER_BUSINESSES) {
        UI_NotifyError(playerid, "Voce ja possui o maximo de empresas permitidas.");
        return;
    }
    new price = g_Businesses[b_idx][biz_Price];
    new paid = false;
    if(PlayerData[playerid][pMoney] >= price) {
        Player_RemoveMoney(playerid, price);
        paid = true;
    } else if(Bank_GetBalance(playerid) >= price) {
        Bank_Debit(playerid, price, "purchase", "Compra de empresa");
        paid = true;
    }
    if(!paid) { UI_NotifyError(playerid, "Saldo insuficiente."); return; }

    g_Businesses[b_idx][biz_OwnerCharID] = PlayerData[playerid][pCharacterID];
    g_Businesses[b_idx][biz_ForSale]     = false;
    g_Businesses[b_idx][biz_Dirty]       = true;
    Business_Save(b_idx);

    new price_fmt[24];
    FormatMoneyFull(price, price_fmt, sizeof(price_fmt));
    new msg[128];
    format(msg, sizeof(msg),
        CSTR_SUCCESS"Voce comprou "CSTR_SYSTEM"%s"CSTR_SUCCESS" por "CSTR_MONEY"%s"CSTR_SUCCESS"!",
        g_Businesses[b_idx][biz_Name], price_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    GameTextForPlayer(playerid, "~g~Empresa Comprada!", 4000, 3);
    Logger_Economy(playerid, "BIZ_BUY", price,
        PlayerData[playerid][pName], g_Businesses[b_idx][biz_Name]);
    CallLocalFunction("OnBusinessTransaction", "iiii", b_idx, playerid, price, 0);
}

// ------------------------------------------------------------
// CONTRATAR funcionario
// ------------------------------------------------------------
stock Business_Hire(playerid, b_idx, targetid, salary) {
    if(!IsValidPlayer(playerid) || !IsValidPlayer(targetid) || b_idx < 0) return;
    if(!Business_IsOwner(playerid, b_idx)) {
        UI_NotifyError(playerid, "Voce nao e dono desta empresa.");
        return;
    }
    if(g_BizEmpCount[b_idx] >= MAX_BIZ_EMPLOYEES) {
        UI_NotifyError(playerid, "Maximo de funcionarios atingido.");
        return;
    }
    if(salary < 100) { UI_NotifyError(playerid, "Salario minimo: $100."); return; }

    new target_name[MAX_PLAYER_NAME];
    GetPlayerName(targetid, target_name, sizeof(target_name));
    new target_char = PlayerData[targetid][pCharacterID];

    // Verificar se ja e funcionario
    if(Business_IsEmployee(targetid, b_idx)) {
        UI_NotifyError(playerid, "Este jogador ja e funcionario.");
        return;
    }

    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `business_employees` "
        "(`business_id`, `character_id`, `rank`, `salary`, `active`) "
        "VALUES (%d, %d, 'Funcionario', %d, 1) "
        "ON DUPLICATE KEY UPDATE `active`=1, `salary`=%d",
        g_Businesses[b_idx][biz_ID], target_char, salary, salary);
    DB_Execute(query);

    // Atualizar cache
    new ei = g_BizEmpCount[b_idx];
    g_BizEmployees[b_idx][ei][emp_ID]     = 0; // sera atualizado no proximo load
    g_BizEmployees[b_idx][ei][emp_CharID] = target_char;
    SafeStrCopy(g_BizEmployees[b_idx][ei][emp_Name], target_name,
        sizeof(g_BizEmployees[][][emp_Name]));
    SafeStrCopy(g_BizEmployees[b_idx][ei][emp_Rank], "Funcionario",
        sizeof(g_BizEmployees[][][emp_Rank]));
    g_BizEmployees[b_idx][ei][emp_Salary] = salary;
    g_BizEmployees[b_idx][ei][emp_Active] = true;
    g_BizEmpCount[b_idx]++;

    new msg_owner[80], msg_emp[80], sal_fmt[24];
    FormatMoneyFull(salary, sal_fmt, sizeof(sal_fmt));
    format(msg_owner, sizeof(msg_owner),
        CSTR_SUCCESS"%s contratado por "CSTR_MONEY"%s/h"CSTR_SUCCESS".",
        target_name, sal_fmt);
    format(msg_emp, sizeof(msg_emp),
        CSTR_SUCCESS"Voce foi contratado em "CSTR_SYSTEM"%s"
        CSTR_SUCCESS" por "CSTR_MONEY"%s/h"CSTR_SUCCESS"!",
        g_Businesses[b_idx][biz_Name], sal_fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg_owner);
    SendClientMessage(targetid, COLOR_SUCCESS, msg_emp);
}

// DEMITIR
stock Business_Fire(playerid, b_idx, emp_idx) {
    if(!Business_IsOwner(playerid, b_idx)) {
        UI_NotifyError(playerid, "Voce nao e dono desta empresa.");
        return;
    }
    if(emp_idx < 0 || emp_idx >= g_BizEmpCount[b_idx]) return;

    new query[128];
    format(query, sizeof(query),
        "UPDATE `business_employees` SET `active`=0 WHERE `id`=%d",
        g_BizEmployees[b_idx][emp_idx][emp_ID]);
    DB_Execute(query);
    g_BizEmployees[b_idx][emp_idx][emp_Active] = false;

    new msg[64];
    format(msg, sizeof(msg), CSTR_SUCCESS"%s foi demitido.",
        g_BizEmployees[b_idx][emp_idx][emp_Name]);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
}

// ------------------------------------------------------------
// SALARIOS ? pagar funcionarios a cada hora
// ------------------------------------------------------------
stock Business_PaySalaries(b_idx) {
    if(b_idx < 0) return;
    if(!g_Businesses[b_idx][biz_Open]) return;

    new total = 0;
    for(new e = 0; e < g_BizEmpCount[b_idx]; e++) {
        if(!g_BizEmployees[b_idx][e][emp_Active]) continue;
        total += g_BizEmployees[b_idx][e][emp_Salary];
    }

    if(total <= 0) return;
    if(g_Businesses[b_idx][biz_Cash] < total) {
        // Aviso ao dono se online
        for(new p = 0; p < MAX_PLAYERS; p++) {
            if(!IsValidPlayer(p)) continue;
            if(PlayerData[p][pCharacterID] == g_Businesses[b_idx][biz_OwnerCharID]) {
                SendWarning(p, "Sua empresa nao tem caixa suficiente para pagar salarios!");
            }
        }
        return;
    }

    g_Businesses[b_idx][biz_Cash] -= total;
    g_Businesses[b_idx][biz_Dirty] = true;

    // Pagar no banco de cada funcionario online
    for(new e = 0; e < g_BizEmpCount[b_idx]; e++) {
        if(!g_BizEmployees[b_idx][e][emp_Active]) continue;
        new char_id = g_BizEmployees[b_idx][e][emp_CharID];
        for(new p = 0; p < MAX_PLAYERS; p++) {
            if(!IsValidPlayer(p)) continue;
            if(PlayerData[p][pCharacterID] != char_id) continue;
            Bank_Credit(p, g_BizEmployees[b_idx][e][emp_Salary],
                "salary", g_Businesses[b_idx][biz_Name]);
        }
    }

    // Log
    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `business_transactions` "
        "(`business_id`, `type`, `amount`, `description`) "
        "VALUES (%d, 'salary', %d, 'Pagamento de salarios')",
        g_Businesses[b_idx][biz_ID], total);
    DB_Execute(query);
}

// Scheduler: pagar salarios a cada hora
public OnScheduler1Minute() {
    static s_SalaryTick = 0;
    s_SalaryTick++;
    if(s_SalaryTick < 60) return; // a cada 60 ticks de 1 min = 1h
    s_SalaryTick = 0;
    for(new i = 0; i < g_BizCount; i++) {
        Business_PaySalaries(i);
    }
}

public OnScheduler5Minutes() {
    Business_SaveAll();
}

// ------------------------------------------------------------
// ABRIR / FECHAR empresa
// ------------------------------------------------------------
stock Business_Open(playerid, b_idx) {
    if(!Business_IsOwner(playerid, b_idx) && !Business_IsEmployee(playerid, b_idx)) {
        UI_NotifyError(playerid, "Voce nao tem permissao.");
        return;
    }
    g_Businesses[b_idx][biz_Open]  = !g_Businesses[b_idx][biz_Open];
    g_Businesses[b_idx][biz_Dirty] = true;
    new msg[64];
    format(msg, sizeof(msg),
        g_Businesses[b_idx][biz_Open] ?
            CSTR_SUCCESS"Empresa aberta." :
            CSTR_WARNING"Empresa fechada.");
    SendClientMessage(playerid, COLOR_INFO, msg);
}

// ------------------------------------------------------------
// ENTRAR / SAIR da empresa
// ------------------------------------------------------------
stock Business_Enter(playerid, b_idx) {
    if(!IsValidPlayer(playerid) || b_idx < 0) return;
    if(!g_Businesses[b_idx][biz_Open] &&
       !Business_IsOwner(playerid, b_idx) &&
       !Business_IsEmployee(playerid, b_idx)) {
        UI_NotifyError(playerid, "Esta empresa esta fechada.");
        return;
    }
    SetPlayerPos(playerid,
        g_Businesses[b_idx][biz_IntX],
        g_Businesses[b_idx][biz_IntY],
        g_Businesses[b_idx][biz_IntZ]);
    SetPlayerInterior(playerid, g_Businesses[b_idx][biz_IntInterior]);
    SetPlayerVirtualWorld(playerid, g_Businesses[b_idx][biz_IntWorld]);
    g_PlayerInsideBiz[playerid] = b_idx;
    SendInfo(playerid, "Use /sairempresa para sair.");
}

stock Business_Exit(playerid) {
    new b_idx = g_PlayerInsideBiz[playerid];
    if(b_idx < 0) { UI_NotifyError(playerid, "Voce nao esta em nenhuma empresa."); return; }
    SetPlayerPos(playerid,
        g_Businesses[b_idx][biz_PosX],
        g_Businesses[b_idx][biz_PosY],
        g_Businesses[b_idx][biz_PosZ] + 0.5);
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    g_PlayerInsideBiz[playerid] = -1;
}

// ------------------------------------------------------------
// DEPOSITAR no caixa
// ------------------------------------------------------------
stock Business_Deposit(playerid, b_idx, amount) {
    if(!Business_IsOwner(playerid, b_idx)) {
        UI_NotifyError(playerid, "Apenas o dono pode depositar no caixa.");
        return;
    }
    if(amount <= 0 || amount > PlayerData[playerid][pMoney]) {
        UI_NotifyError(playerid, "Valor invalido.");
        return;
    }
    Player_RemoveMoney(playerid, amount);
    g_Businesses[b_idx][biz_Cash] += amount;
    g_Businesses[b_idx][biz_Dirty] = true;
    new fmt[24]; FormatMoneyFull(amount, fmt, sizeof(fmt));
    new msg[64]; format(msg, sizeof(msg), CSTR_SUCCESS"Depositou %s no caixa.", fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    new log_query[256];
    format(log_query, sizeof(log_query),
        "INSERT INTO `business_transactions` "
        "(`business_id`, `character_id`, `type`, `amount`, `description`) "
        "VALUES (%d, %d, 'deposit', %d, 'Deposito no caixa')",
        g_Businesses[b_idx][biz_ID], PlayerData[playerid][pCharacterID], amount);
    DB_Execute(log_query);
}

// ------------------------------------------------------------
// MENU principal da empresa
// ------------------------------------------------------------
stock Business_ShowMenu(playerid, b_idx) {
    if(!IsValidPlayer(playerid) || b_idx < 0) return;
    g_MenuBizIdx[playerid] = b_idx;
    new is_owner = Business_IsOwner(playerid, b_idx);
    new is_emp   = Business_IsEmployee(playerid, b_idx);
    new cash_fmt[24];
    FormatMoneyFull(g_Businesses[b_idx][biz_Cash], cash_fmt, sizeof(cash_fmt));
    new price_fmt[24];
    FormatMoneyFull(g_Businesses[b_idx][biz_Price], price_fmt, sizeof(price_fmt));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s ===\n\n"
        CSTR_WHITE"Tipo: "CSTR_GREY"%s\n"
        CSTR_WHITE"Status: %s\n"
        CSTR_WHITE"Funcionarios: "CSTR_SYSTEM"%d\n",
        g_Businesses[b_idx][biz_Name],
        g_Businesses[b_idx][biz_TypeName],
        g_Businesses[b_idx][biz_Open] ? CSTR_SUCCESS"Aberta" : CSTR_ERROR"Fechada",
        g_BizEmpCount[b_idx]);

    if(is_owner) {
        format(body, sizeof(body), "%s"CSTR_WHITE"Caixa: "CSTR_MONEY"%s\n",
            body, cash_fmt);
    }
    if(g_Businesses[b_idx][biz_ForSale]) {
        format(body, sizeof(body), "%s"CSTR_WHITE"Preco: "CSTR_MONEY"%s\n",
            body, price_fmt);
    }

    new options[512];
    if(g_Businesses[b_idx][biz_ForSale]) {
        format(options, sizeof(options), "Comprar (%s)\n", price_fmt);
    }
    if(is_owner || is_emp) {
        strcat(options, g_Businesses[b_idx][biz_Open] ? "Fechar Empresa\n" : "Abrir Empresa\n",
            sizeof(options));
    }
    if(is_owner) {
        strcat(options, "Funcionarios\n",    sizeof(options));
        strcat(options, "Estoque\n",         sizeof(options));
        strcat(options, "Depositar no Caixa\n", sizeof(options));
        strcat(options, "Vender ao Servidor\n", sizeof(options));
    }

    if(IsNullString(options)) options = "Entrar";

    UI_ShowDialog(playerid, DIALOG_BIZ_MENU, DIALOG_STYLE_LIST,
        "{ Empresa", options, "Selecionar", "Fechar");
}

// LISTAR empresas a venda
stock Business_ShowForSale(playerid) {
    if(!IsValidPlayer(playerid)) return;
    UI_ListReset();
    new found = false;
    for(new i = 0; i < g_BizCount; i++) {
        if(!g_Businesses[i][biz_ForSale]) continue;
        new price_fmt[24];
        FormatMoneyFull(g_Businesses[i][biz_Price], price_fmt, sizeof(price_fmt));
        new line[80];
        format(line, sizeof(line), "[%s] %s ? %s",
            g_Businesses[i][biz_TypeName],
            g_Businesses[i][biz_Name],
            price_fmt);
        UI_ListAdd(line);
        found = true;
    }
    if(!found) UI_ListAdd("Nenhuma empresa a venda.");
    UI_ListShow(playerid, DIALOG_BIZ_LIST, "{ Empresas a Venda", "Ver / Comprar", "Fechar");
}

// Vender ao servidor
static Business_SellToServer(playerid, b_idx) {
    if(!Business_IsOwner(playerid, b_idx)) return;
    new sell = g_Businesses[b_idx][biz_Price] * 6 / 10;
    Bank_Credit(playerid, sell, "refund", "Venda de empresa");
    g_Businesses[b_idx][biz_OwnerCharID] = 0;
    g_Businesses[b_idx][biz_ForSale]     = true;
    g_Businesses[b_idx][biz_Cash]        = 0;
    g_Businesses[b_idx][biz_Open]        = false;
    g_Businesses[b_idx][biz_Dirty]       = true;
    Business_Save(b_idx);
    new fmt[24]; FormatMoneyFull(sell, fmt, sizeof(fmt));
    new msg[80]; format(msg, sizeof(msg),
        CSTR_SUCCESS"Empresa vendida. Recebeu "CSTR_MONEY"%s"CSTR_SUCCESS" no banco.", fmt);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
}

// Mostrar funcionarios
static Business_ShowEmployees(playerid, b_idx) {
    UI_ListReset();
    if(g_BizEmpCount[b_idx] == 0) {
        UI_ListAdd("Nenhum funcionario contratado.");
    } else {
        for(new e = 0; e < g_BizEmpCount[b_idx]; e++) {
            if(!g_BizEmployees[b_idx][e][emp_Active]) continue;
            new sal_fmt[24];
            FormatMoneyFull(g_BizEmployees[b_idx][e][emp_Salary], sal_fmt, sizeof(sal_fmt));
            new line[80];
            format(line, sizeof(line), "%s  [%s]  %s/h",
                g_BizEmployees[b_idx][e][emp_Name],
                g_BizEmployees[b_idx][e][emp_Rank],
                sal_fmt);
            UI_ListAdd(line);
        }
    }
    UI_ListShow(playerid, DIALOG_BIZ_EMPLOYEES, "{ Funcionarios", "Demitir", "Fechar");
}

// Mostrar estoque
static Business_ShowStock(playerid, b_idx) {
    UI_ListReset();
    if(g_BizStockCount[b_idx] == 0) {
        UI_ListAdd("Estoque vazio.");
    } else {
        for(new s = 0; s < g_BizStockCount[b_idx]; s++) {
            new defIdx = Inventory_GetItemDefIndex(g_BizStock[b_idx][s][bst_ItemID]);
            if(defIdx < 0) continue;
            new line[80];
            new sell_fmt[24];
            FormatMoneyFull(g_BizStock[b_idx][s][bst_SellPrice], sell_fmt, sizeof(sell_fmt));
            format(line, sizeof(line), "Item #%d  Qtd:%d  Venda:%s",
                g_BizStock[b_idx][s][bst_ItemID],
                g_BizStock[b_idx][s][bst_Quantity],
                sell_fmt);
            UI_ListAdd(line);
        }
    }
    UI_ListShow(playerid, DIALOG_BIZ_STOCK, "{ Estoque", "Ver", "Fechar");
}

// Handler de dialogs
stock Business_HandleDialog(playerid, dialogid, response, listitem,
                                  inputtext[]) {
    switch(dialogid) {
        case DIALOG_BIZ_MENU: {
            if(!response) return true;
            new b_idx = g_MenuBizIdx[playerid];
            new is_owner = Business_IsOwner(playerid, b_idx);
            new cur = 0;
            if(g_Businesses[b_idx][biz_ForSale]) {
                if(listitem == cur) { Business_Buy(playerid, b_idx); return true; }
                cur++;
            }
            if(is_owner || Business_IsEmployee(playerid, b_idx)) {
                if(listitem == cur) { Business_Open(playerid, b_idx); return true; }
                cur++;
            }
            if(is_owner) {
                if(listitem == cur) { Business_ShowEmployees(playerid, b_idx); return true; }
                cur++;
                if(listitem == cur) { Business_ShowStock(playerid, b_idx); return true; }
                cur++;
                if(listitem == cur) {
                    SetPVarInt(playerid, "deposit_biz_idx", b_idx);
                    UI_ShowDialog(playerid, DIALOG_BIZ_DEPOSIT, DIALOG_STYLE_INPUT,
                        "{ Depositar no Caixa", "Valor:", "Confirmar", "Cancelar");
                    return true;
                }
                cur++;
                if(listitem == cur) {
                    SetPVarInt(playerid, "sell_biz_idx", b_idx);
                    new sell = g_Businesses[b_idx][biz_Price] * 6 / 10;
                    new fmt[24]; FormatMoneyFull(sell, fmt, sizeof(fmt));
                    new body[80];
                    format(body, sizeof(body), "Vender empresa ao servidor por "CSTR_MONEY"%s"CSTR_WHITE"?", fmt);
                    UI_ShowConfirm(playerid, "{ Vender Empresa", body);
                    SetPVarInt(playerid, "confirm_biz_sell", 1);
                    return true;
                }
            }
            return true;
        }
        case DIALOG_BIZ_LIST: {
            if(!response) return true;
            new count = 0, found = -1;
            for(new i = 0; i < g_BizCount; i++) {
                if(!g_Businesses[i][biz_ForSale]) continue;
                if(count == listitem) { found = i; break; }
                count++;
            }
            if(found >= 0) Business_ShowMenu(playerid, found);
            return true;
        }
        case DIALOG_BIZ_EMPLOYEES: {
            if(!response) return true;
            new b_idx = g_MenuBizIdx[playerid];
            new count = 0, found_emp = -1;
            for(new e = 0; e < g_BizEmpCount[b_idx]; e++) {
                if(!g_BizEmployees[b_idx][e][emp_Active]) continue;
                if(count == listitem) { found_emp = e; break; }
                count++;
            }
            if(found_emp >= 0) Business_Fire(playerid, b_idx, found_emp);
            return true;
        }
        case DIALOG_BIZ_DEPOSIT: {
            if(!response) return true;
            new b_idx = GetPVarInt(playerid, "deposit_biz_idx");
            Business_Deposit(playerid, b_idx, strval(inputtext));
            return true;
        }
        case DIALOG_BIZ_HIRE: {
            if(!response) return true;
            new b_idx = g_MenuBizIdx[playerid];
            new targetid, salary;
            if(sscanf(inputtext, "ii", targetid, salary) || !IsValidPlayer(targetid)) {
                UI_NotifyError(playerid, "Formato: [ID] [salario]");
                return true;
            }
            Business_Hire(playerid, b_idx, targetid, salary);
            return true;
        }
    }
    if(dialogid == DIALOG_CONFIRM_YES_NO && GetPVarInt(playerid, "confirm_biz_sell")) {
        DeletePVar(playerid, "confirm_biz_sell");
        if(response) {
            new b_idx = GetPVarInt(playerid, "sell_biz_idx");
            Business_SellToServer(playerid, b_idx);
        }
        return true;
    }
    return false;
}

// Cleanup
public OnPlayerDisconnect(playerid, reason) {
    g_PlayerInsideBiz[playerid] = -1;
    g_MenuBizIdx[playerid]      = -1;
    return 1;
}

// Comandos
CMD:empresa(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new b_idx = Business_FindNearest(playerid);
    if(b_idx < 0) {
        new char_id = PlayerData[playerid][pCharacterID];
        for(new i = 0; i < g_BizCount; i++) {
            if(g_Businesses[i][biz_OwnerCharID] == char_id) { b_idx = i; break; }
        }
    }
    if(b_idx < 0) {
        UI_NotifyInfo(playerid, "Nenhuma empresa proxima. Use /empresas para ver a venda.");
        return 1;
    }
    Business_ShowMenu(playerid, b_idx);
    return 1;
}

CMD:empresas(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Business_ShowForSale(playerid);
    return 1;
}

CMD:contratar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new b_idx = g_PlayerInsideBiz[playerid];
    if(b_idx < 0) b_idx = g_MenuBizIdx[playerid];
    if(b_idx < 0 || !Business_IsOwner(playerid, b_idx)) {
        SendError(playerid, "Voce precisa estar dentro de sua empresa.");
        return 1;
    }
    UI_ShowDialog(playerid, DIALOG_BIZ_HIRE, DIALOG_STYLE_INPUT,
        "{ Contratar Funcionario",
        "Digite: [ID do jogador] [salario]\nExemplo: 5 1000",
        "Contratar", "Cancelar");
    return 1;
}

CMD:sairempresa(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Business_Exit(playerid);
    return 1;
}

CMD:abrirempresa(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    new b_idx = g_MenuBizIdx[playerid];
    if(b_idx < 0) b_idx = g_PlayerInsideBiz[playerid];
    if(b_idx < 0) { SendError(playerid, "Nenhuma empresa selecionada."); return 1; }
    Business_Open(playerid, b_idx);
    return 1;
}
