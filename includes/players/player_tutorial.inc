// ============================================================
//  Santa Aurora Roleplay — Tutorial de Novo Jogador
//  Módulo : players/player_tutorial.inc
//  Objetivo: guiar novos jogadores pelos sistemas básicos
//            com recompensas a cada etapa concluída
// ============================================================

#if defined _SA_PLAYER_TUTORIAL_INC
    #endinput
#endif
#define _SA_PLAYER_TUTORIAL_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "player_core.inc"

// ------------------------------------------------------------
// Etapas do tutorial
// ------------------------------------------------------------
#define TUTORIAL_STEP_WELCOME       0
#define TUTORIAL_STEP_CITY          1
#define TUTORIAL_STEP_COMMANDS      2
#define TUTORIAL_STEP_DOCUMENTS     3
#define TUTORIAL_STEP_PHONE         4
#define TUTORIAL_STEP_BANK          5
#define TUTORIAL_STEP_HUNGER        6
#define TUTORIAL_STEP_JOBS          7
#define TUTORIAL_STEP_FIRST_OBJ     8
#define TUTORIAL_STEP_DONE          9

#define TUTORIAL_MAX_STEPS          9

// Dialogs do tutorial
#define DIALOG_TUTORIAL_BASE        400

// Recompensas por etapa
static const g_TutorialRewards[TUTORIAL_MAX_STEPS] = {
    0,      // 0: boas-vindas — sem recompensa
    0,      // 1: cidade
    200,    // 2: comandos — $200
    0,      // 3: documentos
    100,    // 4: celular — $100
    300,    // 5: banco — $300
    150,    // 6: fome/sede — $150
    500,    // 7: empregos — $500
    1000    // 8: primeiro objetivo — $1000
};

// Estado do tutorial por jogador
static g_TutorialStep[MAX_PLAYERS]      = {0, ...};
static bool:g_TutorialActive[MAX_PLAYERS] = {false, ...};
static bool:g_TutorialDone[MAX_PLAYERS]  = {false, ...};

// ------------------------------------------------------------
// Verificar se jogador precisa de tutorial
// ------------------------------------------------------------
stock Tutorial_Check(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(g_TutorialDone[playerid]) return;

    // Consultar banco
    new query[128];
    format(query, sizeof(query),
        "SELECT `step`, `completed` FROM `tutorial_progress` WHERE `character_id`=%d",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnTutorialCheck", "i", playerid);
}

forward OnTutorialCheck(playerid);
public OnTutorialCheck(playerid) {
    if(!IsValidPlayer(playerid)) return 0;

    if(!DB_HasResult()) {
        // Primeiro acesso — iniciar tutorial
        new query[128];
        format(query, sizeof(query),
            "INSERT INTO `tutorial_progress` (`character_id`, `step`, `completed`) "
            "VALUES (%d, 0, 0)",
            PlayerData[playerid][pCharacterID]);
        DB_Execute(query);

        g_TutorialStep[playerid]   = TUTORIAL_STEP_WELCOME;
        g_TutorialActive[playerid] = true;
        g_TutorialDone[playerid]   = false;

        SetTimerEx("Tutorial_StartDelayed", 2000, false, "i", playerid);
    } else {
        new step      = DB_GetInt("step");
        new completed = DB_GetInt("completed");

        if(completed) {
            g_TutorialDone[playerid] = true;
        } else {
            g_TutorialStep[playerid]   = step;
            g_TutorialActive[playerid] = true;
            SetTimerEx("Tutorial_StartDelayed", 2000, false, "i", playerid);
        }
    }
    return 1;
}

forward Tutorial_StartDelayed(playerid);
public Tutorial_StartDelayed(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    Tutorial_ShowStep(playerid, g_TutorialStep[playerid]);
    return 1;
}

// ------------------------------------------------------------
// Exibir etapa do tutorial
// ------------------------------------------------------------
stock Tutorial_ShowStep(playerid, step) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_TutorialActive[playerid]) return;

    g_TutorialStep[playerid] = step;

    switch(step) {
        case TUTORIAL_STEP_WELCOME: Tutorial_StepWelcome(playerid);
        case TUTORIAL_STEP_CITY:    Tutorial_StepCity(playerid);
        case TUTORIAL_STEP_COMMANDS: Tutorial_StepCommands(playerid);
        case TUTORIAL_STEP_DOCUMENTS: Tutorial_StepDocuments(playerid);
        case TUTORIAL_STEP_PHONE:   Tutorial_StepPhone(playerid);
        case TUTORIAL_STEP_BANK:    Tutorial_StepBank(playerid);
        case TUTORIAL_STEP_HUNGER:  Tutorial_StepHunger(playerid);
        case TUTORIAL_STEP_JOBS:    Tutorial_StepJobs(playerid);
        case TUTORIAL_STEP_FIRST_OBJ: Tutorial_StepFirstObjective(playerid);
        case TUTORIAL_STEP_DONE:    Tutorial_Complete(playerid);
        default: Tutorial_Complete(playerid);
    }
}

// ------------------------------------------------------------
// Etapa 0: Boas-vindas
// ------------------------------------------------------------
static Tutorial_StepWelcome(playerid) {
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof(name));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"Bem-vindo ao Santa Aurora RP!\n\n"
        CSTR_WHITE"Olá, "CSTR_SYSTEM"%s"CSTR_WHITE"!\n\n"
        "Você está em "CSTR_SYSTEM"Santa Aurora"CSTR_WHITE", uma cidade brasileira "
        "com vida própria, economia real e histórias que você vai criar.\n\n"
        CSTR_GREY"Este tutorial vai te guiar pelos sistemas principais.\n"
        "Levará cerca de "CSTR_YELLOW"5 minutos"CSTR_GREY" e você receberá "
        CSTR_MONEY"recompensas"CSTR_GREY" em cada etapa.",
        name);

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_WELCOME,
        DIALOG_STYLE_MSGBOX,
        "{ Bem-vindo ao Santa Aurora RP!",
        body, "Próximo >>", "");
}

// Etapa 1: A cidade
static Tutorial_StepCity(playerid) {
    new body[] =
        CSTR_SYSTEM"Santa Aurora\n\n"
        CSTR_WHITE"A cidade é dividida em regiões:\n\n"
        CSTR_SYSTEM"• Centro "CSTR_GREY"— comércio, bancos, prefeitura\n"
        CSTR_SYSTEM"• Zona Norte "CSTR_GREY"— bairros residenciais\n"
        CSTR_SYSTEM"• Zona Sul "CSTR_GREY"— condomínios, shopping\n"
        CSTR_SYSTEM"• Zona Industrial "CSTR_GREY"— fábricas, empregos\n"
        CSTR_SYSTEM"• Zona Rural "CSTR_GREY"— fazendas, produção\n"
        CSTR_SYSTEM"• Região Portuária "CSTR_GREY"— cargas, logística\n\n"
        CSTR_GREY"Cada região tem empregos e atividades únicas.";

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_CITY,
        DIALOG_STYLE_MSGBOX,
        "{ A Cidade de Santa Aurora",
        body, "Próximo >>", "<< Voltar");
}

// Etapa 2: Comandos básicos — RECOMPENSA $200
static Tutorial_StepCommands(playerid) {
    new body[] =
        CSTR_SYSTEM"Comandos Básicos\n\n"
        CSTR_WHITE"/status        "CSTR_GREY"— Ver seu status\n"
        CSTR_WHITE"/inventario    "CSTR_GREY"— Abrir inventário\n"
        CSTR_WHITE"/documentos    "CSTR_GREY"— Ver documentos\n"
        CSTR_WHITE"/banco         "CSTR_GREY"— Menu bancário\n"
        CSTR_WHITE"/celular       "CSTR_GREY"— Abrir celular\n"
        CSTR_WHITE"/emprego       "CSTR_GREY"— Ver empregos\n"
        CSTR_WHITE"/me [ação]     "CSTR_GREY"— Ação RP local\n"
        CSTR_WHITE"/do [desc]     "CSTR_GREY"— Descrição RP\n"
        CSTR_WHITE"/b [texto]     "CSTR_GREY"— Chat OOC\n"
        CSTR_WHITE"/ajuda         "CSTR_GREY"— Menu completo\n\n"
        CSTR_MONEY"Recompensa: $200";

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_COMMANDS,
        DIALOG_STYLE_MSGBOX,
        "{ Comandos Básicos",
        body, "Receber Recompensa >>", "<< Voltar");
}

// Etapa 3: Documentos
static Tutorial_StepDocuments(playerid) {
    new body[] =
        CSTR_SYSTEM"Seus Documentos\n\n"
        CSTR_WHITE"Todo personagem recebe documentos ao criar a conta:\n\n"
        CSTR_SYSTEM"• RG "CSTR_GREY"— identificação básica\n"
        CSTR_SYSTEM"• CPF "CSTR_GREY"— cadastro civil\n\n"
        CSTR_WHITE"Outros documentos são emitidos depois:\n"
        CSTR_SYSTEM"• CNH "CSTR_GREY"— para dirigir\n"
        CSTR_SYSTEM"• Porte de Arma "CSTR_GREY"— para porte legal\n\n"
        "Use "CSTR_SYSTEM"/documentos"CSTR_WHITE" para ver seus documentos.\n\n"
        CSTR_GREY"A polícia pode solicitar seus documentos.";

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_DOCUMENTS,
        DIALOG_STYLE_MSGBOX,
        "{ Seus Documentos",
        body, "Próximo >>", "<< Voltar");
}

// Etapa 4: Celular — RECOMPENSA $100
static Tutorial_StepPhone(playerid) {
    new body[] =
        CSTR_SYSTEM"Celular\n\n"
        CSTR_WHITE"Você recebeu um "CSTR_SYSTEM"celular"CSTR_WHITE" inicial.\n\n"
        CSTR_WHITE"Com o celular você pode:\n"
        CSTR_GREY"• Ligar para outros jogadores\n"
        CSTR_GREY"• Enviar SMS\n"
        CSTR_GREY"• Acessar o banco\n"
        CSTR_GREY"• Ver o GPS\n"
        CSTR_GREY"• Ligar para emergências\n"
        CSTR_GREY"• Publicar anúncios\n\n"
        "Use "CSTR_SYSTEM"/celular"CSTR_WHITE" para abrir.\n\n"
        CSTR_MONEY"Recompensa: $100";

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_PHONE,
        DIALOG_STYLE_MSGBOX,
        "{ Seu Celular",
        body, "Receber Recompensa >>", "<< Voltar");
}

// Etapa 5: Banco — RECOMPENSA $300
static Tutorial_StepBank(playerid) {
    new cash_str[32], bank_str[32];
    FormatMoneyFull(PlayerData[playerid][pMoney], cash_str, sizeof(cash_str));
    FormatMoneyFull(PlayerData[playerid][pBank], bank_str, sizeof(bank_str));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"Sistema Bancário\n\n"
        CSTR_WHITE"Seu dinheiro atual:\n"
        CSTR_GREY"• Carteira: "CSTR_MONEY"%s\n"
        CSTR_GREY"• Banco: "CSTR_MONEY"%s\n\n"
        CSTR_WHITE"Com o banco você pode:\n"
        CSTR_GREY"• Depositar e sacar\n"
        CSTR_GREY"• Transferir via PIX\n"
        CSTR_GREY"• Ver extrato\n"
        CSTR_GREY"• Pagar multas\n\n"
        CSTR_WARNING"Dica: "CSTR_WHITE"Mantenha dinheiro no banco!\n"
        "Se morrer ou for roubado, perde apenas o que está na carteira.\n\n"
        CSTR_MONEY"Recompensa: $300",
        cash_str, bank_str);

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_BANK,
        DIALOG_STYLE_MSGBOX,
        "{ Sistema Bancário",
        body, "Receber Recompensa >>", "<< Voltar");
}

// Etapa 6: Fome e sede — RECOMPENSA $150
static Tutorial_StepHunger(playerid) {
    new body[] =
        CSTR_SYSTEM"Fome e Sede\n\n"
        CSTR_WHITE"Seu personagem precisa comer e beber.\n\n"
        "Monitore pelo "CSTR_SYSTEM"HUD"CSTR_WHITE" no canto inferior esquerdo:\n"
        CSTR_ORANGE"• Barra Laranja "CSTR_WHITE"= Fome\n"
        CSTR_BLUE"• Barra Ciano   "CSTR_WHITE"= Sede\n\n"
        CSTR_WARNING"Efeitos por nível baixo:\n"
        CSTR_GREY"• 40-80%: aviso no chat\n"
        CSTR_GREY"• 20-40%: aviso frequente\n"
        CSTR_GREY"• 0-20%: dano leve progressivo\n\n"
        "Compre comida e bebida em "CSTR_SYSTEM"mercados e restaurantes"CSTR_WHITE".\n"
        "Use "CSTR_SYSTEM"/inventario"CSTR_WHITE" para consumir itens.\n\n"
        CSTR_MONEY"Recompensa: $150";

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_HUNGER,
        DIALOG_STYLE_MSGBOX,
        "{ Fome e Sede",
        body, "Receber Recompensa >>", "<< Voltar");
}

// Etapa 7: Empregos — RECOMPENSA $500
static Tutorial_StepJobs(playerid) {
    new body[] =
        CSTR_SYSTEM"Empregos\n\n"
        CSTR_WHITE"Empregos são a principal fonte de renda.\n\n"
        CSTR_WHITE"Empregos iniciais disponíveis:\n"
        CSTR_GREY"• Entregador de Comida\n"
        CSTR_GREY"• Motorista de Aplicativo RP\n"
        CSTR_GREY"• Taxista\n"
        CSTR_GREY"• Coletor de Lixo\n"
        CSTR_GREY"• Pescador\n"
        CSTR_GREY"• Agricultor\n\n"
        CSTR_WHITE"Cada emprego tem:\n"
        CSTR_GREY"• Progressão por nível\n"
        CSTR_GREY"• Salário crescente\n"
        CSTR_GREY"• Missões e objetivos\n"
        CSTR_GREY"• XP para evoluir\n\n"
        "Use "CSTR_SYSTEM"/emprego"CSTR_WHITE" para ver empregos.\n\n"
        CSTR_MONEY"Recompensa: $500";

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_JOBS,
        DIALOG_STYLE_MSGBOX,
        "{ Empregos",
        body, "Receber Recompensa >>", "<< Voltar");
}

// Etapa 8: Primeiro objetivo — RECOMPENSA $1000
static Tutorial_StepFirstObjective(playerid) {
    new body[] =
        CSTR_SYSTEM"Primeiros Passos\n\n"
        CSTR_WHITE"Parabéns por completar o tutorial!\n\n"
        "Seus primeiros objetivos em Santa Aurora:\n\n"
        CSTR_SYSTEM"[ ] "CSTR_WHITE"Conseguir um emprego\n"
        CSTR_SYSTEM"[ ] "CSTR_WHITE"Comprar comida e se alimentar\n"
        CSTR_SYSTEM"[ ] "CSTR_WHITE"Depositar dinheiro no banco\n"
        CSTR_SYSTEM"[ ] "CSTR_WHITE"Tirar a CNH\n"
        CSTR_SYSTEM"[ ] "CSTR_WHITE"Comprar seu primeiro veículo\n"
        CSTR_SYSTEM"[ ] "CSTR_WHITE"Alugar ou comprar uma casa\n\n"
        CSTR_GREY"Use "CSTR_WHITE"/ajuda"CSTR_GREY" sempre que precisar.\n"
        "A equipe está disponível via "CSTR_WHITE"/report"CSTR_GREY".\n\n"
        CSTR_MONEY"Recompensa final: $1.000";

    UI_ShowDialog(playerid,
        DIALOG_TUTORIAL_BASE + TUTORIAL_STEP_FIRST_OBJ,
        DIALOG_STYLE_MSGBOX,
        "{ Primeiros Passos",
        body, "Concluir Tutorial!", "");
}

// ------------------------------------------------------------
// Dar recompensa da etapa
// ------------------------------------------------------------
static Tutorial_GiveReward(playerid, step) {
    if(step < 0 || step >= TUTORIAL_MAX_STEPS) return;
    new reward = g_TutorialRewards[step];
    if(reward <= 0) return;

    Player_AddMoney(playerid, reward);

    new msg[64];
    FormatMoneyFull(reward, msg, sizeof(msg));
    new full_msg[80];
    format(full_msg, sizeof(full_msg),
        CSTR_MONEY"[TUTORIAL] "CSTR_WHITE"Recompensa: +%s", msg);
    SendClientMessage(playerid, COLOR_MONEY, full_msg);

    // Log econômico
    Logger_Economy(playerid, "TUTORIAL_REWARD", reward, "Tutorial", "Player");
}

// ------------------------------------------------------------
// Avançar etapa e salvar progresso
// ------------------------------------------------------------
static Tutorial_AdvanceStep(playerid, step) {
    // Dar recompensa da etapa atual antes de avançar
    Tutorial_GiveReward(playerid, step);

    new next = step + 1;
    g_TutorialStep[playerid] = next;

    // Salvar no banco
    new query[128];
    format(query, sizeof(query),
        "UPDATE `tutorial_progress` SET `step`=%d WHERE `character_id`=%d",
        next, PlayerData[playerid][pCharacterID]);
    DB_Execute(query);

    Tutorial_ShowStep(playerid, next);
}

// ------------------------------------------------------------
// Completar tutorial
// ------------------------------------------------------------
stock Tutorial_Complete(playerid) {
    if(!IsValidPlayer(playerid)) return;
    g_TutorialActive[playerid] = false;
    g_TutorialDone[playerid]   = true;

    // Marcar como completo no banco
    new query[128];
    format(query, sizeof(query),
        "UPDATE `tutorial_progress` SET `completed`=1, `completed_at`=NOW() "
        "WHERE `character_id`=%d",
        PlayerData[playerid][pCharacterID]);
    DB_Execute(query);

    // Mensagem final
    new name[MAX_PLAYER_NAME];
    SafeStrCopy(name, PlayerData[playerid][pName], sizeof(name));
    new msg[128];
    format(msg, sizeof(msg),
        CSTR_SYSTEM"[TUTORIAL] "CSTR_WHITE"Tutorial concluído! Bem-vindo à "
        CSTR_SYSTEM"Santa Aurora"CSTR_WHITE", %s!", name);
    SendClientMessage(playerid, COLOR_SYSTEM, msg);
    GameTextForPlayer(playerid, "~g~Tutorial Concluido!", 5000, 3);

    Logger_Info("TUTORIAL", "Tutorial concluído para player");
    CallLocalFunction("OnPlayerFinishedTutorial", "i", playerid);
}

// ------------------------------------------------------------
// Handler de dialogs do tutorial
// ------------------------------------------------------------
stock bool:Tutorial_HandleDialog(playerid, dialogid, response, listitem) {
    // Verificar se é dialog do tutorial
    if(dialogid < DIALOG_TUTORIAL_BASE || dialogid >= DIALOG_TUTORIAL_BASE + 20) {
        return false;
    }

    new step = dialogid - DIALOG_TUTORIAL_BASE;

    if(!response) {
        // Botão "Voltar" — retroceder
        if(step > 0) Tutorial_ShowStep(playerid, step - 1);
        return true;
    }

    // Avançar
    Tutorial_AdvanceStep(playerid, step);
    return true;
}

// Verificar se tutorial está ativo
stock bool:Tutorial_IsActive(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return g_TutorialActive[playerid];
}

stock bool:Tutorial_IsDone(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return g_TutorialDone[playerid];
}

// Integração com OnCharacterCreated
public OnCharacterCreated(playerid) {
    // Emitir documentos iniciais
    Docs_IssueStarter(playerid);
    // Dar celular inicial (item)
    Inventory_AddItem(playerid, Inventory_GetItemByName("ITEM_PHONE"), 1, "", false);
    // Dar itens de comida iniciais
    Inventory_AddItem(playerid, Inventory_GetItemByName("ITEM_BREAD"),  2, "", false);
    Inventory_AddItem(playerid, Inventory_GetItemByName("ITEM_WATER"),  2, "", false);
    // Iniciar tutorial
    g_TutorialStep[playerid]   = TUTORIAL_STEP_WELCOME;
    g_TutorialActive[playerid] = true;
    new query[128];
    format(query, sizeof(query),
        "INSERT IGNORE INTO `tutorial_progress` (`character_id`, `step`, `completed`) "
        "VALUES (%d, 0, 0)",
        PlayerData[playerid][pCharacterID]);
    DB_Execute(query);
    SetTimerEx("Tutorial_StartDelayed", 1500, false, "i", playerid);
    return 1;
}

// Integração com OnPlayerLoaded (personagem existente)
public OnPlayerLoaded(playerid) {
    Tutorial_Check(playerid);
    return 1;
}

