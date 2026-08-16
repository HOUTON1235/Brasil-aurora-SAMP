// ============================================================

//  Santa Aurora Roleplay

//  Arquivo principal: gamemodes/rp.pwn

//  VersA?o: 0.4.0

// ============================================================

// sscanf2 v2.13+ requer compilador community a?? desabilitar features extras

#define SSCANF_NO_NICE_FEATURES

#define SSCANF_NO_K_WEAPON

#define SSCANF_NO_K_VEHICLE

// ------------------------------------------------------------

// Includes SA-MP e Plugins

// ------------------------------------------------------------

#include <a_samp>

#include <a_mysql>

#include <streamer>

#include <sscanf2>

#include <zcmd>          // Plugin/include: ZCMD para comandos (CMD:)

// ------------------------------------------------------------

// Core

// ------------------------------------------------------------

#include <defines.inc>

#include <macros.inc>

#include <states.inc>

#include <events.inc>

#include <permissions.inc>

#include <scheduler.inc>

// ------------------------------------------------------------

// Database

// ------------------------------------------------------------

#include <db_core.inc>

#include <db_queries.inc>

// ------------------------------------------------------------

// Logger

// ------------------------------------------------------------

#include <logger.inc>

// ------------------------------------------------------------

// UI

// ------------------------------------------------------------

#include <ui_core.inc>

// ------------------------------------------------------------

// Utilities

// ------------------------------------------------------------

#include <utils.inc>

// ------------------------------------------------------------

// Players a?? Fase 0

// ------------------------------------------------------------

#include <player_core.inc>

#include <player_auth.inc>

#include <player_data.inc>

// ------------------------------------------------------------

// Players a?? Fase 1

// ------------------------------------------------------------

#include <player_hud.inc>

#include <player_hunger.inc>

#include <player_documents.inc>

#include <player_character.inc>

#include <player_tutorial.inc>

// ------------------------------------------------------------

// InventA?rio a?? Fase 1

// ------------------------------------------------------------

#include <inventory_core.inc>

// ------------------------------------------------------------

// Economia a?? Fase 2

// ------------------------------------------------------------

#include <bank_core.inc>

#include <stores_core.inc>

#include <fines_core.inc>

// ------------------------------------------------------------

// Empregos a?? Fase 2

// ------------------------------------------------------------

#include <jobs_core.inc>

// ------------------------------------------------------------

// VeA?culos a?? Fase 3

// ------------------------------------------------------------

#include <vehicle_core.inc>

#include <vehicle_dealership.inc>

#include <vehicle_fuel.inc>

// ------------------------------------------------------------

// Propriedades / Garagem a?? Fase 3

// ------------------------------------------------------------

#include <property_core.inc>

#include <garage_core.inc>

// ------------------------------------------------------------

// Empresas a?? Fase 4

// ------------------------------------------------------------

#include <business_core.inc>

// ------------------------------------------------------------

// ComunicaA?A?o a?? Fase 5

// ------------------------------------------------------------

#include <phone_core.inc>

// ------------------------------------------------------------

// ServiA?os PA?blicos a?? Fase 6

// ------------------------------------------------------------

#include <police_core.inc>

#include <hospital_core.inc>

// ------------------------------------------------------------

// Governo a?? Fase 6

// ------------------------------------------------------------

#include <government_core.inc>

#include <traffic_core.inc>

// ------------------------------------------------------------

// FacA?A?es a?? Fase 7

// ------------------------------------------------------------

#include <faction_core.inc>

#include <territory_core.inc>

// ------------------------------------------------------------

// ConteA?do a?? Fase 8

// ------------------------------------------------------------

#include <missions_core.inc>

// ------------------------------------------------------------

// Mundo (clima, rankings) a?? Fase 8

// ------------------------------------------------------------

#include <world_core.inc>

// ------------------------------------------------------------

// SeguranA?a a?? Fase 9

// ------------------------------------------------------------

#include <anticheat_core.inc>

// ------------------------------------------------------------

// Monitoramento a?? Fase 10

// ------------------------------------------------------------

#include <performance_core.inc>

// ------------------------------------------------------------

// Admin

// ------------------------------------------------------------

#include <admin_core.inc>

// ============================================================

// main() a?? ponto de entrada

// ============================================================

main() {

    print("\n");

    print(" ==========================================");

    print("  Santa Aurora - Roleplay Brasileiro");

    print("  Versao: 0.1.0  |  Fase 0 - Fundacao");

    print("  github: /santa-aurora-rp");

    print(" ==========================================");

    print("\n");

}

// ============================================================

// OnGameModeInit a?? inicializaA?A?o

// ============================================================

public OnGameModeInit() {

    // 1. Logger primeiro (todos os outros mA?dulos dependem)

    Logger_Init();

    Logger_Info("CORE", "========================================");

    Logger_Info("CORE", "Santa Aurora RP v" SERVER_VERSION " iniciando...");

    Logger_Info("CORE", "========================================");

    // 2. Criar arquivo de config do banco se nA?o existir

    DB_CreateConfigFile();

    // 3. Conectar ao banco de dados

    if(!DB_Connect()) {

        Logger_Error("CORE", "FALHA CRITICA: Banco de dados nao conectou!");

        Logger_Error("CORE", "Verifique scriptfiles/db_config.ini e reinicie.");

        // Servidor continua para permitir correA?A?o sem restart manual

    }

    // 4. Scheduler central

    Scheduler_Init();

    // 5. Admin

    Admin_Init();

    // 6. Carregar item definitions (inventA?rio)

    Inventory_LoadItemDefs();

    // 7. Carregar definiA?A?es de empregos

    Jobs_LoadDefs();

    // 8. Carregar lojas

    Stores_Load();

    // 9. Carregar veA?culos no mundo

    Vehicle_LoadAll();

    // 10. Carregar concessionA?rias

    Dealer_LoadAll();

    // 11. Carregar postos de combustA?vel

    FuelStation_LoadAll();

    // 12. Carregar propriedades

    Property_LoadAll();

    // 13. Carregar empresas

    Business_LoadAll();

    // 14. Carregar anA?ncios do celular

    Phone_LoadAnnouncements();

    // 15. Carregar catA?logo de crimes

    Police_LoadCrimeCatalog();

    // 16. Carregar facA?A?es e territA?rios

    Faction_LoadAll();

    Territory_LoadAll();

    // 17. Carregar definiA?A?es de missA?es e conquistas

    Missions_LoadDefs();

    // 18. Inicializar mundo (clima, hora)

    World_Init();

    // 19. Inicializar anti-cheat

    AC_Init();

    // 20. Inicializar monitor de performance

    Perf_Init();

    // 6. ConfiguraA?A?es do GameMode SA-MP

    SetGameModeText("Santa Aurora RP v" SERVER_VERSION);

    AddPlayerClass(0,

        SPAWN_DEFAULT_X,

        SPAWN_DEFAULT_Y,

        SPAWN_DEFAULT_Z,

        SPAWN_DEFAULT_A,

        0, 0, 0, 0, 0, 0);

    // ConfiguraA?A?es de jogo

    DisableInteriorEnterExits();

    UsePlayerPedAnims();

    EnableStuntBonusForAll(false);

    ShowNameTags(true);

    ShowPlayerMarkers(PLAYER_MARKERS_MODE_STREAMED);

    SetWeather(10);

    SetWorldTime(12);

    // Streaming padrA?o do Streamer plugin

    Streamer_SetMaxItems(STREAMER_TYPE_OBJECT,    500);

    Streamer_SetMaxItems(STREAMER_TYPE_PICKUP,    200);

    Streamer_SetMaxItems(STREAMER_TYPE_3D_TEXT,  300);

    Streamer_SetMaxItems(STREAMER_TYPE_AREA,      200);

    Logger_Info("CORE", "Servidor iniciado com sucesso!");

    Logger_Info("CORE", "Aguardando conexoes de jogadores...");

    return 1;

}

// ============================================================

// OnGameModeExit a?? encerramento

// ============================================================

public OnGameModeExit() {

    Logger_Info("CORE", "Servidor encerrando...");

    // Salvar todos os jogadores online

    new saved = 0;

    for(new i = 0; i < MAX_PLAYERS; i++) {

        if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {

            Player_Save(i);

            saved++;

        }

    }

    new log_msg[64];

    format(log_msg, sizeof(log_msg), "%d jogadores salvos no shutdown.", saved);

    Logger_Info("CORE", log_msg);

    // Encerrar scheduler

    Scheduler_Shutdown();

    // Fechar conexA?o com banco

    DB_Disconnect();

    Logger_Info("CORE", "Servidor encerrado com seguranca.");

    return 1;

}

// ============================================================

// OnPlayerConnect

// ============================================================

public OnPlayerConnect(playerid) {

    Player_Init(playerid);

    Player_SetState(playerid, STATE_CONNECTING);

    new name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, name, sizeof(name));

    new ip[16];

    Player_GetIP(playerid, ip, sizeof(ip));

    new log_msg[64];

    format(log_msg, sizeof(log_msg), "Conexao: %s (%d) IP=%s", name, playerid, ip);

    Logger_Info("PLAYER", log_msg);

    // Exibir dialog de login apA?s 500ms (garante que cliente carregou)

    SetTimerEx("Player_ShowLoginDelayed", 500, false, "i", playerid);

    return 1;

}

forward Player_ShowLoginDelayed(playerid);

public Player_ShowLoginDelayed(playerid) {

    if(!IsValidPlayer(playerid)) return 0;

    Player_SetState(playerid, STATE_AUTHENTICATING);

    Auth_SendLoginDialog(playerid);

    return 1;

}

// ============================================================

// OnPlayerDisconnect

// ============================================================

public OnPlayerDisconnect(playerid, reason) {

    new name[MAX_PLAYER_NAME];

    if(strlen(PlayerData[playerid][pName]) > 0) {

        SafeStrCopy(name, PlayerData[playerid][pName], sizeof(name));

    } else {

        GetPlayerName(playerid, name, sizeof(name));

    }

    new reason_str[16];

    switch(reason) {

        case 0: format(reason_str, sizeof(reason_str), "Timeout");

        case 1: format(reason_str, sizeof(reason_str), "Saiu");

        case 2: format(reason_str, sizeof(reason_str), "Kickado");

        default: format(reason_str, sizeof(reason_str), "Desconhecido");

    }

    if(PlayerData[playerid][pIsLogged]) {

        Player_Save(playerid);

        new log_msg[64];

        format(log_msg, sizeof(log_msg), "Desconectado: %s (%d) Motivo=%s",

            name, playerid, reason_str);

        Logger_Info("PLAYER", log_msg);

    }

    // Destruir HUD

    HUD_Destroy(playerid);

    Player_Init(playerid); // limpar memA?ria

    return 1;

}

// ============================================================

// OnPlayerSpawn

// ============================================================

public OnPlayerSpawn(playerid) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    // Garantir que estado estA? correto

    new E_PLAYER_STATE:st = Player_GetState(playerid);

    if(st == STATE_AUTHENTICATING || st == STATE_CONNECTING) return 1;

    // Aplicar stats do personagem

    SetPlayerHealth(playerid, PlayerData[playerid][pHealth]);

    SetPlayerArmour(playerid, PlayerData[playerid][pArmour]);

    Player_SetMoney(playerid, PlayerData[playerid][pMoney]);

    SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);

    // Criar e mostrar HUD

    HUD_Create(playerid);

    // Welcome

    UI_ShowWelcome(playerid, PlayerData[playerid][pName]);

    if(Player_GetState(playerid) != STATE_ADMIN_SPECTATING) {

        Player_SetState(playerid, STATE_SPAWNED);

    }

    Player_UpdateActivity(playerid);

    return 1;

}

// ============================================================

// OnPlayerDeath

// ============================================================

public OnPlayerDeath(playerid, killerid, reason) {

    Player_SetState(playerid, STATE_DEAD);

    new name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, name, sizeof(name));

    new log_msg[64];

    format(log_msg, sizeof(log_msg), "Morte: %s (ID:%d) Causa:%d",

        name, playerid, reason);

    Logger_Info("PLAYER", log_msg);

    FireEvent_ii("OnPlayerDied", playerid, killerid);

    // Salvar estado de morte

    Player_MarkDirty(playerid);

    return 1;

}

// ============================================================

// OnPlayerText a?? chat local + anti-spam

// ============================================================

static g_LastChatTime[MAX_PLAYERS] = {0, ...};

#define CHAT_SPAM_DELAY 1500 // ms

public OnPlayerText(playerid, text[]) {

    if(!PlayerData[playerid][pIsLogged]) return 0;

    if(!State_CanUseRPCommands(playerid)) {

        SendError(playerid, "VocA? nA?o pode falar neste estado.");

        return 0;

    }

    // Anti-spam

    new tick = GetTickCount();

    if(tick - g_LastChatTime[playerid] < CHAT_SPAM_DELAY) {

        SendWarning(playerid, "NA?o envie mensagens tA?o rA?pido.");

        return 0;

    }

    g_LastChatTime[playerid] = tick;

    // Validar tamanho

    if(strlen(text) > 128) {

        SendWarning(playerid, "Mensagem muito longa.");

        return 0;

    }

    // Formatar chat local (alcance de 20 metros)

    new name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, name, sizeof(name));

    new msg[160];

    format(msg, sizeof(msg), CSTR_WHITE"%s"CSTR_GREY" diz: "CSTR_WHITE"%s", name, text);

    // Enviar apenas para jogadores prA?ximos

    new Float:px, Float:py, Float:pz;

    GetPlayerPos(playerid, px, py, pz);

    for(new i = 0; i < MAX_PLAYERS; i++) {

        if(!IsValidPlayer(i)) continue;

        if(GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;

        if(GetPlayerInterior(i) != GetPlayerInterior(playerid)) continue;

        if(IsPlayerInRange3D(i, px, py, pz, 20.0)) {

            SendClientMessage(i, COLOR_WHITE, msg);

        }

    }

    Logger_Chat(playerid, "LOCAL", text);

    Player_UpdateActivity(playerid);

    return 0; // 0 = nA?o enviar o chat padrA?o do SA-MP

}

// ============================================================

// OnPlayerCommandText a?? roteamento de comandos

// ============================================================

public OnPlayerCommandText(playerid, cmdtext[]) {

    Player_UpdateActivity(playerid);

    // Anti-spam de comandos

    new tick = GetTickCount();

    if(tick - g_LastChatTime[playerid] < ANTISPAM_CMD_DELAY) {

        SendWarning(playerid, "NA?o use comandos tA?o rapidamente.");

        return 1;

    }

    g_LastChatTime[playerid] = tick;

    // ZCMD cuida do roteamento a?? se chegou aqui, comando nA?o existe

    return 0; // ZCMD retornarA? 0 se nA?o achar o comando

}

// ============================================================

// OnDialogResponse a?? roteamento de dialogs

// ============================================================

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]) {

    // MA?dulo de autenticaA?A?o

    if(Auth_HandleDialog(playerid, dialogid, response, inputtext)) return 1;

    // CriaA?A?o de personagem

    if(CharCreate_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // Tutorial

    if(Tutorial_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // InventA?rio

    if(Inventory_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // Banco

    if(Bank_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // Loja

    if(Stores_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // Multas

    if(Fines_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // Empregos

    if(Jobs_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // VeA?culo

    if(Vehicle_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // ConcessionA?ria

    if(Dealer_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // CombustA?vel

    if(FuelStation_HandleDialog(playerid, dialogid, response, inputtext)) return 1;

    // Propriedade

    if(Property_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // Garagem

    if(Garage_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // Empresa

    if(Business_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // Celular

    if(Phone_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // PolA?cia

    if(Police_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // Hospital

    if(Hospital_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // Governo

    if(Government_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // TrA?nsito

    if(Traffic_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // FacA?A?o

    if(Faction_HandleDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // TerritA?rio

    if(Territory_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // MissA?es

    if(Missions_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // Mundo / Rankings

    if(World_HandleDialog(playerid, dialogid, response, listitem)) return 1;

    // ConfirmaA?A?o de demissA?o

    if(dialogid == DIALOG_CONFIRM_YES_NO) {

        if(GetPVarInt(playerid, "confirm_quit_job")) {

            DeletePVar(playerid, "confirm_quit_job");

            if(response) Jobs_Quit(playerid);

            return 1;

        }

    }

    return 1;

}

// ============================================================

// OnPlayerKeyStateChange a?? detecA?A?o de atividade

// ============================================================

public OnPlayerKeyStateChange(playerid, newkeys, oldkeys) {

    if(newkeys != 0 && PlayerData[playerid][pIsLogged]) {

        Player_UpdateActivity(playerid);

    }

    return 1;

}

// ============================================================

// Comandos bA?sicos de jogador

// ============================================================

// /ajuda a?? menu de ajuda

CMD:ajuda(playerid, params[]) {

    new body[] =

        CSTR_SYSTEM"=== Santa Aurora RP ===\n\n"

        CSTR_SYSTEM"[ Personagem ]\n"

        CSTR_WHITE"/status /documentos /inventario /peso /xp\n\n"

        CSTR_SYSTEM"[ Banco e Economia ]\n"

        CSTR_WHITE"/banco /saldo /depositar /sacar /transferir /pix /extrato /multas\n\n"

        CSTR_SYSTEM"[ Empregos ]\n"

        CSTR_WHITE"/emprego /trabalhar /meuemprego /demissao\n\n"

        CSTR_SYSTEM"[ Veiculos ]\n"

        CSTR_WHITE"/veiculo /trancar /meusveiculos /abastecer /concessionaria /garagem\n\n"

        CSTR_SYSTEM"[ Propriedades ]\n"

        CSTR_WHITE"/casa /imoveis /entrar /sair /loja\n\n"

        CSTR_SYSTEM"[ Empresas ]\n"

        CSTR_WHITE"/empresa /empresas /contratar /abrirempresa\n\n"

        CSTR_SYSTEM"[ Celular ]\n"

        CSTR_WHITE"/celular /sms /ligar /desligar /c /contato /anuncio /anuncios\n\n"

        CSTR_SYSTEM"[ Policia / Hospital / Governo ]\n"

        CSTR_WHITE"/abordar /algema /prender /hospital /samu /prefeitura /detran\n\n"

        CSTR_SYSTEM"[ Faccoes / Territorios ]\n"

        CSTR_WHITE"/faccao /radio /r /recrutar /promover /territorios /disputar\n\n"

        CSTR_SYSTEM"[ Conteudo ]\n"

        CSTR_WHITE"/missoes /conquistas /ranking /hora /mundo\n\n"

        CSTR_SYSTEM"[ RP / Chat ]\n"

        CSTR_WHITE"/me /do /b /pm /report";

    UI_ShowDialog(playerid, DIALOG_MAIN_MENU, DIALOG_STYLE_MSGBOX,

        "{ Ajuda", body, "Fechar", "");

    return 1;

}

// /status a?? status do personagem

CMD:status(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    new name[MAX_PLAYER_NAME + 4];

    SafeStrCopy(name, PlayerData[playerid][pName], sizeof(name));

    new fmtMoney[32], fmtBank[32];

    FormatMoneyFull(PlayerData[playerid][pMoney], fmtMoney, sizeof(fmtMoney));

    FormatMoneyFull(PlayerData[playerid][pBank], fmtBank, sizeof(fmtBank));

    new hunger_bar[32], thirst_bar[32];

    UI_ProgressBar(hunger_bar, sizeof(hunger_bar),

        PlayerData[playerid][pHunger], MAX_HUNGER, 15);

    UI_ProgressBar(thirst_bar, sizeof(thirst_bar),

        PlayerData[playerid][pThirst], MAX_THIRST, 15);

    new body[512];

    format(body, sizeof(body),

        CSTR_SYSTEM"=== Status de %s ===\n\n"

        CSTR_WHITE"NA?vel: "CSTR_SYSTEM"%d "CSTR_GREY"(%d XP)\n"

        CSTR_WHITE"Dinheiro: "CSTR_MONEY"%s\n"

        CSTR_WHITE"Banco:    "CSTR_MONEY"%s\n"

        CSTR_WHITE"SaA?de: "CSTR_GREEN"%.0f%%\n"

        CSTR_WHITE"Colete: "CSTR_BLUE"%.0f%%\n"

        CSTR_WHITE"Fome: "CSTR_YELLOW"%s\n"

        CSTR_WHITE"Sede: "CSTR_BLUE"%s\n"

        CSTR_WHITE"Tempo online: "CSTR_GREY"%d min",

        name,

        PlayerData[playerid][pLevel],

        PlayerData[playerid][pExperience],

        fmtMoney,

        fmtBank,

        PlayerData[playerid][pHealth],

        PlayerData[playerid][pArmour],

        hunger_bar,

        thirst_bar,

        Player_GetSessionTime(playerid) / 60

    );

    UI_ShowDialog(playerid, DIALOG_NONE + 3, DIALOG_STYLE_MSGBOX,

        "Status", body, "Fechar", "");

    return 1;

}

// /me [aA?A?o] a?? RP aA?A?o

CMD:me(playerid, params[]) {

    if(!State_CanUseRPCommands(playerid)) {

        SendError(playerid, "VocA? nA?o pode usar este comando no momento.");

        return 1;

    }

    if(IsNullString(params)) {

        SendError(playerid, "Uso: /me [aA?A?o]");

        return 1;

    }

    new name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, name, sizeof(name));

    new msg[160];

    format(msg, sizeof(msg), "* %s %s", name, params);

    new Float:px, Float:py, Float:pz;

    GetPlayerPos(playerid, px, py, pz);

    for(new i = 0; i < MAX_PLAYERS; i++) {

        if(!IsValidPlayer(i)) continue;

        if(GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;

        if(IsPlayerInRange3D(i, px, py, pz, 20.0)) {

            SendClientMessage(i, COLOR_CHAT_ME, msg);

        }

    }

    Logger_Chat(playerid, "ME", params);

    return 1;

}

// /do [descriA?A?o] a?? RP descriA?A?o do ambiente

CMD:do(playerid, params[]) {

    if(!State_CanUseRPCommands(playerid)) {

        SendError(playerid, "VocA? nA?o pode usar este comando no momento.");

        return 1;

    }

    if(IsNullString(params)) {

        SendError(playerid, "Uso: /do [descriA?A?o]");

        return 1;

    }

    new name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, name, sizeof(name));

    new msg[160];

    format(msg, sizeof(msg), "* %s | (( %s ))", params, name);

    new Float:px, Float:py, Float:pz;

    GetPlayerPos(playerid, px, py, pz);

    for(new i = 0; i < MAX_PLAYERS; i++) {

        if(!IsValidPlayer(i)) continue;

        if(GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;

        if(IsPlayerInRange3D(i, px, py, pz, 20.0)) {

            SendClientMessage(i, COLOR_CHAT_DO, msg);

        }

    }

    Logger_Chat(playerid, "DO", params);

    return 1;

}

// /b [texto] a?? chat OOC local

CMD:b(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    if(IsNullString(params)) {

        SendError(playerid, "Uso: /b [texto OOC]");

        return 1;

    }

    new name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, name, sizeof(name));

    new msg[160];

    format(msg, sizeof(msg), "(( %s: %s ))", name, params);

    new Float:px, Float:py, Float:pz;

    GetPlayerPos(playerid, px, py, pz);

    for(new i = 0; i < MAX_PLAYERS; i++) {

        if(!IsValidPlayer(i)) continue;

        if(GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;

        if(IsPlayerInRange3D(i, px, py, pz, 30.0)) {

            SendClientMessage(i, COLOR_CHAT_OOC, msg);

        }

    }

    return 1;

}

// /pm [id] [mensagem]

CMD:pm(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    new targetid;

    new message[128];

    if(sscanf(params, "is[128]", targetid, message) || !IsValidPlayer(targetid)) {

        SendError(playerid, "Uso: /pm [id] [mensagem]");

        return 1;

    }

    if(targetid == playerid) {

        SendError(playerid, "VocA? nA?o pode enviar PM para si mesmo.");

        return 1;

    }

    if(!PlayerData[targetid][pIsLogged]) {

        SendError(playerid, "Jogador nA?o estA? disponA?vel.");

        return 1;

    }

    new sender_name[MAX_PLAYER_NAME], target_name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, sender_name, sizeof(sender_name));

    GetPlayerName(targetid, target_name, sizeof(target_name));

    new to_target[160], to_sender[160];

    format(to_target, sizeof(to_target),

        CSTR_WHITE"[PM de %s]: "CSTR_PINK"%s", sender_name, message);

    format(to_sender, sizeof(to_sender),

        CSTR_WHITE"[PM para %s]: "CSTR_PINK"%s", target_name, message);

    SendClientMessage(targetid, COLOR_CHAT_PM, to_target);

    SendClientMessage(playerid, COLOR_CHAT_PM, to_sender);

    Logger_Chat(playerid, "PM", message);

    return 1;

}

// /report [mensagem]

CMD:report(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    if(IsNullString(params)) {

        SendError(playerid, "Uso: /report [descriA?A?o do problema]");

        return 1;

    }

    new name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, name, sizeof(name));

    new msg[160];

    format(msg, sizeof(msg),

        CSTR_ADMIN"[REPORT] "CSTR_WHITE"%s (ID:%d): %s", name, playerid, params);

    for(new i = 0; i < MAX_PLAYERS; i++) {

        if(IsValidPlayer(i) && Player_IsAdmin(i, ADMIN_HELPER)) {

            SendClientMessage(i, COLOR_ADMIN, msg);

        }

    }

    SendSuccess(playerid, "Report enviado para a equipe. Aguarde atendimento.");

    Logger_Info("REPORT", params);

    return 1;

}

// Aliases

CMD:relatorio(playerid, params[]) { return cmd_report(playerid, params); }

CMD:comandos(playerid, params[])  { return cmd_ajuda(playerid, params);  }

CMD:help(playerid, params[])      { return cmd_ajuda(playerid, params);  }

// ============================================================

// Comandos a?? Fase 1

// ============================================================

// /inventario a?? abrir inventA?rio

CMD:inventario(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    if(!State_CanInteract(playerid)) {

        SendError(playerid, "VocA? nA?o pode acessar o inventA?rio agora.");

        return 1;

    }

    Inventory_ShowDialog(playerid);

    return 1;

}

CMD:inv(playerid, params[]) { return cmd_inventario(playerid, params); }

// /usar [item_name] a?? usar item pelo nome

CMD:usar(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    if(IsNullString(params)) {

        SendError(playerid, "Uso: /usar [nome do item] ou abra /inventario");

        return 1;

    }

    new itemid = Inventory_GetItemByName(params);

    if(itemid == INVALID_ITEM_ID) {

        SendError(playerid, "Item nA?o encontrado. Use /inventario para ver seus itens.");

        return 1;

    }

    // Encontrar slot do item

    for(new s = 0; s < MAX_INV_SLOTS; s++) {

        new id, qty;

        if(Inventory_GetItemInSlot(playerid, s, id, qty) && id == itemid) {

            Inventory_UseItem(playerid, s);

            return 1;

        }

    }

    SendError(playerid, "VocA? nA?o possui este item.");

    return 1;

}

// /peso a?? ver peso do inventA?rio

CMD:peso(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    new msg[64];

    format(msg, sizeof(msg),

        CSTR_SYSTEM"[InventA?rio] "CSTR_WHITE"Peso atual: "CSTR_YELLOW"%.2f / %.2f kg",

        Inventory_GetWeight(playerid), MAX_INV_WEIGHT);

    SendClientMessage(playerid, COLOR_INFO, msg);

    return 1;

}

// /comer a?? alias para abrir inventA?rio e usar comida

CMD:comer(playerid, params[]) { return cmd_inventario(playerid, params); }

CMD:beber(playerid, params[]) { return cmd_inventario(playerid, params); }

// /cnhinfo a?? informaA?A?es da CNH

CMD:cnhinfo(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    if(Docs_Has(playerid, DOC_CNH)) {

        if(Docs_HasValidCNH(playerid)) {

            SendSuccess(playerid, "Sua CNH estA? vA?lida.");

        } else {

            SendError(playerid, "Sua CNH estA? suspensa ou revogada.");

        }

    } else {

        SendWarning(playerid, "VocA? nA?o possui CNH. Dirija com cuidado!");

    }

    return 1;

}

// /tutorial a?? rever tutorial

CMD:tutorial(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    if(Tutorial_IsDone(playerid)) {

        SendInfo(playerid, CSTR_GREY"VocA? jA? completou o tutorial. Use /ajuda para ver os comandos.");

    } else {

        Tutorial_ShowStep(playerid, 0);

    }

    return 1;

}

// /hud a?? alternar visibilidade do HUD

CMD:hud(playerid, params[]) {

    if(!PlayerData[playerid][pIsLogged]) return 1;

    HUD_Update(playerid);

    SendSuccess(playerid, "HUD atualizado.");

    return 1;

}

