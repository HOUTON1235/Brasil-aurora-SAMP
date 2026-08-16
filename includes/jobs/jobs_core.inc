// ============================================================
//  Santa Aurora Roleplay — Sistema de Empregos
//  Módulo : jobs/jobs_core.inc
//  Objetivo: registro de empregos, progressão, salário, XP
// ============================================================

#if defined _SA_JOBS_CORE_INC
    #endinput
#endif
#define _SA_JOBS_CORE_INC

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
#define MAX_JOB_TYPES       50
#define MAX_JOB_LEVEL       15
#define JOB_SALARY_INTERVAL 3600        // segundos entre salários (1h)
#define JOB_NONE            0

// Dialogs de emprego
#define DIALOG_JOB_LIST     600
#define DIALOG_JOB_INFO     601
#define DIALOG_JOB_CONFIRM  602
#define DIALOG_JOB_STATUS   603

// ------------------------------------------------------------
// Cache de definições de empregos
// ------------------------------------------------------------
enum E_JOB_DEF {
    jdef_ID,
    jdef_Name[32],
    jdef_DisplayName[48],
    jdef_Category[16],
    jdef_MinLevel,
    jdef_BaseSalary,
    jdef_SalaryPerLevel,
    jdef_XPPerAction,
    jdef_XPPerLevel,
    jdef_MaxLevel,
    jdef_UniformSkin,
    jdef_Description[128]
}

static g_JobDefs[MAX_JOB_TYPES][E_JOB_DEF];
static g_JobDefCount = 0;
static bool:g_JobDefsLoaded = false;

// ------------------------------------------------------------
// Cache de emprego do jogador (atual)
// ------------------------------------------------------------
enum E_PLAYER_JOB {
    pjob_CharJobID,         // ID em character_jobs
    pjob_JobID,
    pjob_Level,
    pjob_XP,
    pjob_TotalActions,
    pjob_TotalEarned,
    bool:pjob_OnDuty,       // em serviço agora?
    pjob_LastSalaryTime,
    bool:pjob_Loaded
}

static g_PlayerJob[MAX_PLAYERS][E_PLAYER_JOB];

// ------------------------------------------------------------
// Carregar definições do banco (startup)
// ------------------------------------------------------------
stock Jobs_LoadDefs() {
    if(!DB_IsConnected()) return;
    DB_Query(
        "SELECT `id`, `name`, `display_name`, `category`, `min_level`, "
        "`base_salary`, `salary_per_level`, `xp_per_action`, `xp_per_level`, "
        "`max_level`, `uniform_skin`, `description` "
        "FROM `jobs` WHERE `active`=1 ORDER BY `id` ASC",
        "OnJobDefsLoaded", "");
}

forward OnJobDefsLoaded();
public OnJobDefsLoaded() {
    g_JobDefCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_JobDefCount < MAX_JOB_TYPES; r++) {
        new i = g_JobDefCount;
        g_JobDefs[i][jdef_ID]            = DB_GetInt("id",              r);
        DB_GetString("name",         g_JobDefs[i][jdef_Name],        sizeof(g_JobDefs[][jdef_Name]),        r);
        DB_GetString("display_name", g_JobDefs[i][jdef_DisplayName], sizeof(g_JobDefs[][jdef_DisplayName]), r);
        DB_GetString("category",     g_JobDefs[i][jdef_Category],    sizeof(g_JobDefs[][jdef_Category]),    r);
        g_JobDefs[i][jdef_MinLevel]      = DB_GetInt("min_level",        r);
        g_JobDefs[i][jdef_BaseSalary]    = DB_GetInt("base_salary",      r);
        g_JobDefs[i][jdef_SalaryPerLevel]= DB_GetInt("salary_per_level", r);
        g_JobDefs[i][jdef_XPPerAction]   = DB_GetInt("xp_per_action",    r);
        g_JobDefs[i][jdef_XPPerLevel]    = DB_GetInt("xp_per_level",     r);
        g_JobDefs[i][jdef_MaxLevel]      = DB_GetInt("max_level",        r);
        g_JobDefs[i][jdef_UniformSkin]   = DB_GetInt("uniform_skin",     r);
        DB_GetString("description",  g_JobDefs[i][jdef_Description], sizeof(g_JobDefs[][jdef_Description]), r);
        g_JobDefCount++;
    }
    g_JobDefsLoaded = true;
    new msg[48];
    format(msg, sizeof(msg), "%d empregos carregados.", g_JobDefCount);
    Logger_Info("JOBS", msg);
}

// Obter índice da def por jobid
stock Jobs_GetDefIndex(jobid) {
    for(new i = 0; i < g_JobDefCount; i++) {
        if(g_JobDefs[i][jdef_ID] == jobid) return i;
    }
    return -1;
}

// ------------------------------------------------------------
// Inicializar dados de emprego do jogador
// ------------------------------------------------------------
stock Jobs_InitPlayer(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_PlayerJob[playerid][pjob_CharJobID]      = INVALID_DB_ID;
    g_PlayerJob[playerid][pjob_JobID]          = JOB_NONE;
    g_PlayerJob[playerid][pjob_Level]          = 1;
    g_PlayerJob[playerid][pjob_XP]             = 0;
    g_PlayerJob[playerid][pjob_TotalActions]   = 0;
    g_PlayerJob[playerid][pjob_TotalEarned]    = 0;
    g_PlayerJob[playerid][pjob_OnDuty]         = false;
    g_PlayerJob[playerid][pjob_LastSalaryTime] = 0;
    g_PlayerJob[playerid][pjob_Loaded]         = false;
}

// ------------------------------------------------------------
// Carregar emprego do jogador
// ------------------------------------------------------------
stock Jobs_Load(playerid) {
    if(!IsValidPlayer(playerid)) return;
    Jobs_InitPlayer(playerid);

    new char_id = PlayerData[playerid][pCharacterID];
    if(char_id == INVALID_DB_ID) return;

    // Carregar emprego atual (baseado em character.job_id)
    new job_id = PlayerData[playerid][pJobID];
    if(job_id == JOB_NONE) {
        g_PlayerJob[playerid][pjob_Loaded] = true;
        return;
    }

    new query[256];
    format(query, sizeof(query),
        "SELECT `id`, `job_id`, `job_level`, `job_xp`, `total_actions`, `total_earned` "
        "FROM `character_jobs` WHERE `character_id`=%d AND `job_id`=%d LIMIT 1",
        char_id, job_id);
    DB_Query(query, "OnPlayerJobLoad", "i", playerid);
}

forward OnPlayerJobLoad(playerid);
public OnPlayerJobLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    if(DB_HasResult()) {
        g_PlayerJob[playerid][pjob_CharJobID]    = DB_GetInt("id");
        g_PlayerJob[playerid][pjob_JobID]        = DB_GetInt("job_id");
        g_PlayerJob[playerid][pjob_Level]        = DB_GetInt("job_level");
        g_PlayerJob[playerid][pjob_XP]           = DB_GetInt("job_xp");
        g_PlayerJob[playerid][pjob_TotalActions] = DB_GetInt("total_actions");
        g_PlayerJob[playerid][pjob_TotalEarned]  = DB_GetInt("total_earned");
    }
    g_PlayerJob[playerid][pjob_Loaded] = true;
    return 1;
}

// ------------------------------------------------------------
// Salvar progresso do emprego
// ------------------------------------------------------------
stock Jobs_Save(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_PlayerJob[playerid][pjob_Loaded]) return;
    if(g_PlayerJob[playerid][pjob_JobID] == JOB_NONE) return;
    if(g_PlayerJob[playerid][pjob_CharJobID] == INVALID_DB_ID) return;

    new query[256];
    format(query, sizeof(query),
        "UPDATE `character_jobs` SET "
        "`job_level`=%d, `job_xp`=%d, `total_actions`=%d, `total_earned`=%d, "
        "`last_worked`=NOW() WHERE `id`=%d",
        g_PlayerJob[playerid][pjob_Level],
        g_PlayerJob[playerid][pjob_XP],
        g_PlayerJob[playerid][pjob_TotalActions],
        g_PlayerJob[playerid][pjob_TotalEarned],
        g_PlayerJob[playerid][pjob_CharJobID]);
    DB_Execute(query);
}

// ------------------------------------------------------------
// Contratar jogador (assumir emprego)
// ------------------------------------------------------------
stock bool:Jobs_Hire(playerid, jobid) {
    if(!IsValidPlayer(playerid)) return false;

    new defIdx = Jobs_GetDefIndex(jobid);
    if(defIdx < 0) {
        UI_NotifyError(playerid, "Emprego inválido.");
        return false;
    }

    // Verificar nível mínimo
    if(PlayerData[playerid][pLevel] < g_JobDefs[defIdx][jdef_MinLevel]) {
        new msg[64];
        format(msg, sizeof(msg), "Você precisa ser nível %d para este emprego.",
            g_JobDefs[defIdx][jdef_MinLevel]);
        UI_NotifyError(playerid, msg);
        return false;
    }

    // Demitir emprego atual se tiver
    if(PlayerData[playerid][pJobID] != JOB_NONE) {
        Jobs_Quit(playerid);
    }

    // Registrar
    new char_id = PlayerData[playerid][pCharacterID];
    new query[256];
    format(query, sizeof(query),
        "INSERT INTO `character_jobs` (`character_id`, `job_id`, `job_level`, `job_xp`, `hired_at`) "
        "VALUES (%d, %d, 1, 0, NOW()) "
        "ON DUPLICATE KEY UPDATE `job_level`=`job_level`, `last_worked`=NOW()",
        char_id, jobid);
    SetPVarInt(playerid, "hiring_job", jobid);
    DB_Query(query, "OnPlayerJobHired", "i", playerid);
    return true;
}

forward OnPlayerJobHired(playerid);
public OnPlayerJobHired(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    new jobid = GetPVarInt(playerid, "hiring_job");
    DeletePVar(playerid, "hiring_job");

    new defIdx = Jobs_GetDefIndex(jobid);
    if(defIdx < 0) return 1;

    PlayerData[playerid][pJobID]         = jobid;
    g_PlayerJob[playerid][pjob_JobID]    = jobid;
    g_PlayerJob[playerid][pjob_CharJobID]= DB_InsertID();
    g_PlayerJob[playerid][pjob_Level]    = 1;
    g_PlayerJob[playerid][pjob_XP]       = 0;
    g_PlayerJob[playerid][pjob_Loaded]   = true;

    Player_MarkDirty(playerid);

    new msg[80];
    format(msg, sizeof(msg), CSTR_SUCCESS"Parabéns! Você foi contratado como "
        CSTR_SYSTEM"%s"CSTR_SUCCESS"!",
        g_JobDefs[defIdx][jdef_DisplayName]);
    SendClientMessage(playerid, COLOR_SUCCESS, msg);
    SendInfo(playerid, "Use /trabalhar para entrar em serviço.");

    Logger_Info("JOBS", "Jogador contratado para emprego");
    CallLocalFunction("OnPlayerChangedJob", "iii", playerid, JOB_NONE, jobid);
    return 1;
}

// ------------------------------------------------------------
// Demissão
// ------------------------------------------------------------
stock Jobs_Quit(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(g_PlayerJob[playerid][pjob_OnDuty]) {
        Jobs_GoOffDuty(playerid);
    }
    new old_job = PlayerData[playerid][pJobID];
    Jobs_Save(playerid);
    PlayerData[playerid][pJobID] = JOB_NONE;
    g_PlayerJob[playerid][pjob_JobID]  = JOB_NONE;
    g_PlayerJob[playerid][pjob_OnDuty] = false;
    Player_MarkDirty(playerid);

    SendWarning(playerid, "Você pediu demissão.");
    Logger_Info("JOBS", "Jogador demitido");
    CallLocalFunction("OnPlayerChangedJob", "iii", playerid, old_job, JOB_NONE);
}

// ------------------------------------------------------------
// Entrar/sair de serviço
// ------------------------------------------------------------
stock Jobs_GoOnDuty(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(g_PlayerJob[playerid][pjob_JobID] == JOB_NONE) {
        UI_NotifyError(playerid, "Você não tem emprego. Use /emprego para conseguir um.");
        return;
    }
    if(g_PlayerJob[playerid][pjob_OnDuty]) {
        UI_NotifyWarning(playerid, "Você já está em serviço.");
        return;
    }

    g_PlayerJob[playerid][pjob_OnDuty]         = true;
    g_PlayerJob[playerid][pjob_LastSalaryTime]  = gettime();
    Player_SetState(playerid, STATE_WORKING);

    new defIdx = Jobs_GetDefIndex(g_PlayerJob[playerid][pjob_JobID]);
    if(defIdx >= 0) {
        new msg[80];
        format(msg, sizeof(msg), CSTR_SUCCESS"Você entrou em serviço como "
            CSTR_SYSTEM"%s"CSTR_SUCCESS"!",
            g_JobDefs[defIdx][jdef_DisplayName]);
        SendClientMessage(playerid, COLOR_SUCCESS, msg);

        // Aplicar uniforme se tiver
        if(g_JobDefs[defIdx][jdef_UniformSkin] > 0) {
            SetPlayerSkin(playerid, g_JobDefs[defIdx][jdef_UniformSkin]);
        }
    }
}

stock Jobs_GoOffDuty(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_PlayerJob[playerid][pjob_OnDuty]) return;

    g_PlayerJob[playerid][pjob_OnDuty] = false;
    Player_SetState(playerid, STATE_SPAWNED);

    // Restaurar skin do personagem
    SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);
    SendInfo(playerid, "Você saiu de serviço.");
}

// ------------------------------------------------------------
// Ação de trabalho (chamada por sub-sistemas de emprego)
// Registra XP, paga por ação, verifica level up
// ------------------------------------------------------------
stock Jobs_OnAction(playerid, pay_multiplier = 100) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_PlayerJob[playerid][pjob_OnDuty]) return;

    new defIdx = Jobs_GetDefIndex(g_PlayerJob[playerid][pjob_JobID]);
    if(defIdx < 0) return;

    // Calcular pagamento por ação
    new level   = g_PlayerJob[playerid][pjob_Level];
    new payment = (g_JobDefs[defIdx][jdef_BaseSalary] +
                   g_JobDefs[defIdx][jdef_SalaryPerLevel] * (level - 1))
                  * pay_multiplier / 100 / 10; // ~10% do salário por ação

    // Pagar em dinheiro físico
    Player_AddMoney(playerid, payment);

    // XP
    new xp_gain = g_JobDefs[defIdx][jdef_XPPerAction];
    g_PlayerJob[playerid][pjob_XP]            += xp_gain;
    g_PlayerJob[playerid][pjob_TotalActions]++;
    g_PlayerJob[playerid][pjob_TotalEarned]   += payment;

    // Level up
    new xp_needed = g_JobDefs[defIdx][jdef_XPPerLevel] * level;
    if(g_PlayerJob[playerid][pjob_XP] >= xp_needed &&
       level < g_JobDefs[defIdx][jdef_MaxLevel]) {
        g_PlayerJob[playerid][pjob_Level]++;
        g_PlayerJob[playerid][pjob_XP] = 0;
        new lv_msg[80];
        format(lv_msg, sizeof(lv_msg),
            CSTR_SUCCESS"[LEVEL UP] "CSTR_WHITE"Você é agora nível "
            CSTR_SYSTEM"%d"CSTR_WHITE" como %s!",
            g_PlayerJob[playerid][pjob_Level],
            g_JobDefs[defIdx][jdef_DisplayName]);
        SendClientMessage(playerid, COLOR_SUCCESS, lv_msg);
        GameTextForPlayer(playerid, "~g~Level Up!", 3000, 3);
    }

    // Notificação de pagamento
    new fmt_pay[32];
    FormatMoneyFull(payment, fmt_pay, sizeof(fmt_pay));
    new pay_msg[64];
    format(pay_msg, sizeof(pay_msg),
        CSTR_MONEY"[+%s] "CSTR_WHITE"Ação concluída.", fmt_pay);
    SendClientMessage(playerid, COLOR_MONEY, pay_msg);
    HUD_UpdateMoney(playerid);
}

// ------------------------------------------------------------
// Salário automático a cada hora
// ------------------------------------------------------------
stock Jobs_PaySalary(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_PlayerJob[playerid][pjob_OnDuty]) return;

    new defIdx = Jobs_GetDefIndex(g_PlayerJob[playerid][pjob_JobID]);
    if(defIdx < 0) return;

    new level   = g_PlayerJob[playerid][pjob_Level];
    new salary  = g_JobDefs[defIdx][jdef_BaseSalary] +
                  g_JobDefs[defIdx][jdef_SalaryPerLevel] * (level - 1);

    // Salário vai para o banco
    Bank_Credit(playerid, salary, "salary", "Salário mensal");

    new fmt[32];
    FormatMoneyFull(salary, fmt, sizeof(fmt));
    new msg[80];
    format(msg, sizeof(msg),
        CSTR_MONEY"[SALÁRIO] "CSTR_WHITE"%s depositado no banco.", fmt);
    SendClientMessage(playerid, COLOR_MONEY, msg);

    g_PlayerJob[playerid][pjob_LastSalaryTime]  = gettime();
    g_PlayerJob[playerid][pjob_TotalEarned]     += salary;
    Logger_Economy(playerid, "SALARY", salary, g_JobDefs[defIdx][jdef_DisplayName],
        PlayerData[playerid][pName]);
}

// Scheduler: verificar salários pendentes a cada minuto
public OnScheduler1Minute() {
    new now = gettime();
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(!IsValidPlayer(i)) continue;
        if(!g_PlayerJob[i][pjob_OnDuty]) continue;
        if(now - g_PlayerJob[i][pjob_LastSalaryTime] >= JOB_SALARY_INTERVAL) {
            Jobs_PaySalary(i);
        }
    }
}

// ------------------------------------------------------------
// Mostrar lista de empregos disponíveis
// ------------------------------------------------------------
stock Jobs_ShowList(playerid) {
    if(!IsValidPlayer(playerid)) return;

    UI_ListReset();
    new count = 0;
    for(new i = 0; i < g_JobDefCount; i++) {
        if(g_JobDefs[i][jdef_MinLevel] > PlayerData[playerid][pLevel]) continue;
        new line[80];
        new sal_fmt[24];
        FormatMoneyFull(g_JobDefs[i][jdef_BaseSalary], sal_fmt, sizeof(sal_fmt));
        format(line, sizeof(line), "%s  [Sal: %s/h]  Nv.%d",
            g_JobDefs[i][jdef_DisplayName], sal_fmt, g_JobDefs[i][jdef_MinLevel]);
        UI_ListAdd(line);
        count++;
    }
    if(count == 0) {
        UI_ListAdd("Nenhum emprego disponível para seu nível.");
    }

    new title[48];
    format(title, sizeof(title), "Empregos Disponíveis  [Nível %d]",
        PlayerData[playerid][pLevel]);
    UI_ListShow(playerid, DIALOG_JOB_LIST, title, "Ver Info", "Fechar");
}

// Mostrar info do emprego selecionado
stock Jobs_ShowInfo(playerid, listitem) {
    // Encontrar o emprego pelo índice da lista
    new count = 0, found_idx = -1;
    for(new i = 0; i < g_JobDefCount; i++) {
        if(g_JobDefs[i][jdef_MinLevel] > PlayerData[playerid][pLevel]) continue;
        if(count == listitem) { found_idx = i; break; }
        count++;
    }
    if(found_idx < 0) return;

    new sal_fmt[24];
    FormatMoneyFull(g_JobDefs[found_idx][jdef_BaseSalary], sal_fmt, sizeof(sal_fmt));
    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s ===\n\n"
        CSTR_WHITE"Salário base: "CSTR_MONEY"%s/h\n"
        CSTR_WHITE"Nível mínimo: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Nível máximo: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Categoria: "CSTR_GREY"%s\n\n"
        CSTR_WHITE"Descrição:\n"CSTR_GREY"%s\n\n"
        CSTR_GREY"Deseja se candidatar?",
        g_JobDefs[found_idx][jdef_DisplayName],
        sal_fmt,
        g_JobDefs[found_idx][jdef_MinLevel],
        g_JobDefs[found_idx][jdef_MaxLevel],
        g_JobDefs[found_idx][jdef_Category],
        g_JobDefs[found_idx][jdef_Description]);

    SetPVarInt(playerid, "job_applying", g_JobDefs[found_idx][jdef_ID]);
    UI_ShowDialog(playerid, DIALOG_JOB_CONFIRM, DIALOG_STYLE_MSGBOX,
        "{ Informações do Emprego", body, "Candidatar-se", "Voltar");
}

// Status do emprego atual
stock Jobs_ShowStatus(playerid) {
    if(!IsValidPlayer(playerid)) return;

    if(g_PlayerJob[playerid][pjob_JobID] == JOB_NONE) {
        UI_NotifyInfo(playerid, "Você não tem emprego. Use /emprego para ver vagas.");
        return;
    }

    new defIdx = Jobs_GetDefIndex(g_PlayerJob[playerid][pjob_JobID]);
    if(defIdx < 0) return;

    new level  = g_PlayerJob[playerid][pjob_Level];
    new xp     = g_PlayerJob[playerid][pjob_XP];
    new xp_max = g_JobDefs[defIdx][jdef_XPPerLevel] * level;
    new sal    = g_JobDefs[defIdx][jdef_BaseSalary] +
                 g_JobDefs[defIdx][jdef_SalaryPerLevel] * (level-1);
    new sal_fmt[24], earned_fmt[24];
    FormatMoneyFull(sal, sal_fmt, sizeof(sal_fmt));
    FormatMoneyFull(g_PlayerJob[playerid][pjob_TotalEarned], earned_fmt, sizeof(earned_fmt));

    new xp_bar[32];
    UI_ProgressBar(xp_bar, sizeof(xp_bar), xp, xp_max, 20);

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== %s ===\n\n"
        CSTR_WHITE"Nível: "CSTR_SYSTEM"%d/%d\n"
        CSTR_WHITE"XP: "CSTR_GREY"%s\n"
        CSTR_WHITE"Salário atual: "CSTR_MONEY"%s/h\n"
        CSTR_WHITE"Total ganho: "CSTR_MONEY"%s\n"
        CSTR_WHITE"Ações totais: "CSTR_SYSTEM"%d\n"
        CSTR_WHITE"Status: %s",
        g_JobDefs[defIdx][jdef_DisplayName],
        level, g_JobDefs[defIdx][jdef_MaxLevel],
        xp_bar,
        sal_fmt, earned_fmt,
        g_PlayerJob[playerid][pjob_TotalActions],
        g_PlayerJob[playerid][pjob_OnDuty] ?
            CSTR_SUCCESS"Em Serviço" : CSTR_GREY"Fora de Serviço");

    UI_ShowDialog(playerid, DIALOG_JOB_STATUS, DIALOG_STYLE_MSGBOX,
        "{ Meu Emprego", body, "Fechar", "");
}

// ------------------------------------------------------------
// Handler de dialogs de emprego
// ------------------------------------------------------------
stock bool:Jobs_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_JOB_LIST: {
            if(!response) return true;
            Jobs_ShowInfo(playerid, listitem);
            return true;
        }
        case DIALOG_JOB_CONFIRM: {
            new jobid = GetPVarInt(playerid, "job_applying");
            DeletePVar(playerid, "job_applying");
            if(!response) { Jobs_ShowList(playerid); return true; }
            if(jobid > 0) Jobs_Hire(playerid, jobid);
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------
// Getters públicos
// ------------------------------------------------------------
stock bool:Jobs_IsOnDuty(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return g_PlayerJob[playerid][pjob_OnDuty];
}

stock Jobs_GetCurrentJob(playerid) {
    if(!IsValidPlayer(playerid)) return JOB_NONE;
    return g_PlayerJob[playerid][pjob_JobID];
}

stock Jobs_GetJobName(jobid, output[], size = sizeof(output)) {
    new defIdx = Jobs_GetDefIndex(jobid);
    if(defIdx >= 0) SafeStrCopy(output, g_JobDefs[defIdx][jdef_DisplayName], size);
    else format(output, size, "Desempregado");
}

// ------------------------------------------------------------
// Integração com eventos
// ------------------------------------------------------------
public OnPlayerLoaded(playerid) {
    Jobs_Load(playerid);
    return 1;
}

public OnPlayerSaved(playerid) {
    Jobs_Save(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    if(g_PlayerJob[playerid][pjob_OnDuty]) {
        Jobs_GoOffDuty(playerid);
    }
    Jobs_InitPlayer(playerid);
    return 1;
}

// ------------------------------------------------------------
// Comandos
// ------------------------------------------------------------
CMD:emprego(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(!State_CanInteract(playerid)) {
        SendError(playerid, "Você não pode acessar este menu agora.");
        return 1;
    }
    Jobs_ShowList(playerid);
    return 1;
}

CMD:trabalhar(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(g_PlayerJob[playerid][pjob_OnDuty]) {
        Jobs_GoOffDuty(playerid);
    } else {
        Jobs_GoOnDuty(playerid);
    }
    return 1;
}

CMD:meuemprego(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Jobs_ShowStatus(playerid);
    return 1;
}

CMD:demissao(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    if(PlayerData[playerid][pJobID] == JOB_NONE) {
        SendError(playerid, "Você não tem emprego.");
        return 1;
    }
    UI_ShowConfirm(playerid, "{ Confirmar Demissão",
        "Tem certeza que deseja pedir demissão?\n\nVocê perderá seu progresso neste emprego.");
    SetPVarInt(playerid, "confirm_quit_job", 1);
    return 1;
}
