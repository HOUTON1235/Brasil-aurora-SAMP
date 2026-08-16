// ============================================================
//  Santa Aurora Roleplay ? HUD do Jogador
//  Modulo : players/player_hud.inc
//  Objetivo: HUD persistente com vida, colete, fome, sede,
//            dinheiro, banco, hora e localizacao
// ============================================================

#if defined _SA_PLAYER_HUD_INC
    #endinput
#endif
#define _SA_PLAYER_HUD_INC

// include removido
// include removido
// include removido
// include removido

// ------------------------------------------------------------
// TextDraws do HUD (por jogador ? PlayerTextDraw)
// Layout:
//   Canto inferior esquerdo: vida | colete | fome | sede
//   Canto superior direito:  dinheiro | banco
//   Canto superior esquerdo: hora | localizacao
// ------------------------------------------------------------

// Slots de textdraws por jogador
enum E_HUD_TD {
    HUD_BG_STATS,       // fundo do painel esquerdo
    HUD_LABEL_HEALTH,   // "Vida"
    HUD_BAR_HEALTH,     // barra de vida
    HUD_LABEL_ARMOUR,   // "Colete"
    HUD_BAR_ARMOUR,     // barra de colete
    HUD_LABEL_HUNGER,   // "Fome"
    HUD_BAR_HUNGER,     // barra de fome
    HUD_LABEL_THIRST,   // "Sede"
    HUD_BAR_THIRST,     // barra de sede

    HUD_BG_MONEY,       // fundo painel dinheiro
    HUD_CASH_LABEL,     // "$"
    HUD_CASH_VALUE,     // valor em dinheiro
    HUD_BANK_LABEL,     // "Banco"
    HUD_BANK_VALUE,     // saldo bancario

    HUD_BG_INFO,        // fundo info
    HUD_TIME_VALUE,     // hora do servidor
    HUD_ZONE_VALUE,     // nome da zona/local

    HUD_MAX
}

static PlayerText:g_HUD[MAX_PLAYERS][HUD_MAX];
static bool:g_HUDCreated[MAX_PLAYERS] = {false, ...};

// Posicoes do HUD
#define HUD_STATS_X     10.0
#define HUD_STATS_Y     360.0
#define HUD_MONEY_X     420.0
#define HUD_MONEY_Y     10.0
#define HUD_INFO_X      10.0
#define HUD_INFO_Y      10.0

// Cores de barras
#define HUD_COLOR_HEALTH    0x4CAF50FF   // verde
#define HUD_COLOR_ARMOUR    0x2196F3FF   // azul
#define HUD_COLOR_HUNGER    0xFF9800FF   // laranja
#define HUD_COLOR_THIRST    0x00BCD4FF   // ciano
#define HUD_COLOR_HEALTH_LOW 0xF44336FF  // vermelho
#define HUD_COLOR_BG        0x00000080   // preto semi-transparente
#define HUD_COLOR_TEXT      0xFFFFFFFF
#define HUD_COLOR_GOLD      0xFFD700FF
#define HUD_COLOR_GREEN     0x4CAF50FF

// Largura das barras
#define HUD_BAR_WIDTH   80.0
#define HUD_BAR_HEIGHT  5.0

// ------------------------------------------------------------
// Criar HUD para um jogador
// ------------------------------------------------------------
stock HUD_Create(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(g_HUDCreated[playerid]) HUD_Destroy(playerid);

    // ---- Painel Stats (inferior esquerdo) ----

    // Fundo
    g_HUD[playerid][HUD_BG_STATS] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X - 3.0, HUD_STATS_Y - 3.0, "_");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BG_STATS], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BG_STATS], 0.0, 7.5);
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BG_STATS], 115.0, 0.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BG_STATS], HUD_COLOR_BG);
    PlayerTextDrawBackgroundColor(playerid, g_HUD[playerid][HUD_BG_STATS], HUD_COLOR_BG);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BG_STATS], HUD_COLOR_BG);
    PlayerTextDrawUseBox(playerid, g_HUD[playerid][HUD_BG_STATS], true);

    // Vida ? label
    g_HUD[playerid][HUD_LABEL_HEALTH] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X, HUD_STATS_Y, "Vida");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_LABEL_HEALTH], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_LABEL_HEALTH], 0.18, 0.9);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_LABEL_HEALTH], HUD_COLOR_TEXT);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_LABEL_HEALTH], 1);

    // Vida ? barra
    g_HUD[playerid][HUD_BAR_HEALTH] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X + 28.0, HUD_STATS_Y + 1.5, "_");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BAR_HEALTH], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BAR_HEALTH], 0.0, HUD_BAR_HEIGHT / 10.0);
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_HEALTH], HUD_BAR_WIDTH, 0.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BAR_HEALTH], HUD_COLOR_HEALTH);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BAR_HEALTH], HUD_COLOR_HEALTH);
    PlayerTextDrawUseBox(playerid, g_HUD[playerid][HUD_BAR_HEALTH], true);

    // Colete ? label
    g_HUD[playerid][HUD_LABEL_ARMOUR] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X, HUD_STATS_Y + 10.0, "Colete");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_LABEL_ARMOUR], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_LABEL_ARMOUR], 0.18, 0.9);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_LABEL_ARMOUR], HUD_COLOR_TEXT);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_LABEL_ARMOUR], 1);

    // Colete ? barra
    g_HUD[playerid][HUD_BAR_ARMOUR] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X + 28.0, HUD_STATS_Y + 11.5, "_");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BAR_ARMOUR], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BAR_ARMOUR], 0.0, HUD_BAR_HEIGHT / 10.0);
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_ARMOUR], HUD_BAR_WIDTH, 0.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BAR_ARMOUR], HUD_COLOR_ARMOUR);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BAR_ARMOUR], HUD_COLOR_ARMOUR);
    PlayerTextDrawUseBox(playerid, g_HUD[playerid][HUD_BAR_ARMOUR], true);

    // Fome ? label
    g_HUD[playerid][HUD_LABEL_HUNGER] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X, HUD_STATS_Y + 20.0, "Fome");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_LABEL_HUNGER], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_LABEL_HUNGER], 0.18, 0.9);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_LABEL_HUNGER], HUD_COLOR_TEXT);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_LABEL_HUNGER], 1);

    // Fome ? barra
    g_HUD[playerid][HUD_BAR_HUNGER] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X + 28.0, HUD_STATS_Y + 21.5, "_");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BAR_HUNGER], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BAR_HUNGER], 0.0, HUD_BAR_HEIGHT / 10.0);
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_HUNGER], HUD_BAR_WIDTH, 0.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BAR_HUNGER], HUD_COLOR_HUNGER);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BAR_HUNGER], HUD_COLOR_HUNGER);
    PlayerTextDrawUseBox(playerid, g_HUD[playerid][HUD_BAR_HUNGER], true);

    // Sede ? label
    g_HUD[playerid][HUD_LABEL_THIRST] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X, HUD_STATS_Y + 30.0, "Sede");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_LABEL_THIRST], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_LABEL_THIRST], 0.18, 0.9);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_LABEL_THIRST], HUD_COLOR_TEXT);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_LABEL_THIRST], 1);

    // Sede ? barra
    g_HUD[playerid][HUD_BAR_THIRST] = CreatePlayerTextDraw(playerid,
        HUD_STATS_X + 28.0, HUD_STATS_Y + 31.5, "_");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BAR_THIRST], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BAR_THIRST], 0.0, HUD_BAR_HEIGHT / 10.0);
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_THIRST], HUD_BAR_WIDTH, 0.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BAR_THIRST], HUD_COLOR_THIRST);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BAR_THIRST], HUD_COLOR_THIRST);
    PlayerTextDrawUseBox(playerid, g_HUD[playerid][HUD_BAR_THIRST], true);

    // ---- Painel Dinheiro (superior direito) ----

    // Fundo dinheiro
    g_HUD[playerid][HUD_BG_MONEY] = CreatePlayerTextDraw(playerid,
        HUD_MONEY_X - 3.0, HUD_MONEY_Y - 3.0, "_");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BG_MONEY], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BG_MONEY], 0.0, 4.0);
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BG_MONEY], 100.0, 0.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BG_MONEY], HUD_COLOR_BG);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BG_MONEY], HUD_COLOR_BG);
    PlayerTextDrawUseBox(playerid, g_HUD[playerid][HUD_BG_MONEY], true);

    // Dinheiro ? label "$"
    g_HUD[playerid][HUD_CASH_LABEL] = CreatePlayerTextDraw(playerid,
        HUD_MONEY_X, HUD_MONEY_Y, "~g~Dinheiro:");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_CASH_LABEL], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_CASH_LABEL], 0.2, 1.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_CASH_LABEL], HUD_COLOR_GREEN);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_CASH_LABEL], 1);

    // Dinheiro ? valor
    g_HUD[playerid][HUD_CASH_VALUE] = CreatePlayerTextDraw(playerid,
        HUD_MONEY_X, HUD_MONEY_Y + 11.0, "$0");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_CASH_VALUE], TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_CASH_VALUE], 0.25, 1.1);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_CASH_VALUE], HUD_COLOR_GOLD);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_CASH_VALUE], 1);

    // Banco ? label
    g_HUD[playerid][HUD_BANK_LABEL] = CreatePlayerTextDraw(playerid,
        HUD_MONEY_X, HUD_MONEY_Y + 23.0, "~b~Banco:");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BANK_LABEL], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BANK_LABEL], 0.18, 0.9);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BANK_LABEL], HUD_COLOR_ARMOUR);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_BANK_LABEL], 1);

    // Banco ? valor
    g_HUD[playerid][HUD_BANK_VALUE] = CreatePlayerTextDraw(playerid,
        HUD_MONEY_X, HUD_MONEY_Y + 33.0, "$0");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BANK_VALUE], TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BANK_VALUE], 0.22, 1.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BANK_VALUE], HUD_COLOR_TEXT);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_BANK_VALUE], 1);

    // ---- Painel Info (superior esquerdo) ----

    g_HUD[playerid][HUD_BG_INFO] = CreatePlayerTextDraw(playerid,
        HUD_INFO_X - 3.0, HUD_INFO_Y - 3.0, "_");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_BG_INFO], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_BG_INFO], 0.0, 3.5);
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BG_INFO], 120.0, 0.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_BG_INFO], HUD_COLOR_BG);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BG_INFO], HUD_COLOR_BG);
    PlayerTextDrawUseBox(playerid, g_HUD[playerid][HUD_BG_INFO], true);

    // Hora
    g_HUD[playerid][HUD_TIME_VALUE] = CreatePlayerTextDraw(playerid,
        HUD_INFO_X, HUD_INFO_Y, "12:00");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_TIME_VALUE], TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_TIME_VALUE], 0.22, 1.0);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_TIME_VALUE], HUD_COLOR_GOLD);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_TIME_VALUE], 1);

    // Zona
    g_HUD[playerid][HUD_ZONE_VALUE] = CreatePlayerTextDraw(playerid,
        HUD_INFO_X, HUD_INFO_Y + 14.0, "Santa Aurora");
    PlayerTextDrawFont(playerid, g_HUD[playerid][HUD_ZONE_VALUE], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, g_HUD[playerid][HUD_ZONE_VALUE], 0.18, 0.9);
    PlayerTextDrawColor(playerid, g_HUD[playerid][HUD_ZONE_VALUE], HUD_COLOR_TEXT);
    PlayerTextDrawSetShadow(playerid, g_HUD[playerid][HUD_ZONE_VALUE], 1);

    g_HUDCreated[playerid] = true;
    HUD_Show(playerid);
}

// ------------------------------------------------------------
// Exibir HUD
// ------------------------------------------------------------
stock HUD_Show(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    for(new i = 0; i < HUD_MAX; i++) {
        if(g_HUD[playerid][E_HUD_TD:i] != PlayerText:INVALID_TEXT_DRAW) {
            PlayerTextDrawShow(playerid, g_HUD[playerid][E_HUD_TD:i]);
        }
    }
    HUD_Update(playerid);
}

// ------------------------------------------------------------
// Ocultar HUD (auth, spectate, etc.)
// ------------------------------------------------------------
stock HUD_Hide(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    for(new i = 0; i < HUD_MAX; i++) {
        if(g_HUD[playerid][E_HUD_TD:i] != PlayerText:INVALID_TEXT_DRAW) {
            PlayerTextDrawHide(playerid, g_HUD[playerid][E_HUD_TD:i]);
        }
    }
}

// ------------------------------------------------------------
// Destruir HUD
// ------------------------------------------------------------
stock HUD_Destroy(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(g_HUDCreated[playerid]) {
        for(new i = 0; i < HUD_MAX; i++) {
            if(g_HUD[playerid][E_HUD_TD:i] != PlayerText:INVALID_TEXT_DRAW) {
                PlayerTextDrawDestroy(playerid, g_HUD[playerid][E_HUD_TD:i]);
                g_HUD[playerid][E_HUD_TD:i] = PlayerText:INVALID_TEXT_DRAW;
            }
        }
        g_HUDCreated[playerid] = false;
    }
}

// ------------------------------------------------------------
// Atualizar todos os valores do HUD
// ------------------------------------------------------------
stock HUD_Update(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    if(!PlayerData[playerid][pIsLogged]) return;

    HUD_UpdateHealth(playerid);
    HUD_UpdateArmour(playerid);
    HUD_UpdateHunger(playerid);
    HUD_UpdateThirst(playerid);
    HUD_UpdateMoney(playerid);
    HUD_UpdateTime(playerid);
    HUD_UpdateZone(playerid);
}

// ------------------------------------------------------------
// Atualizar barra de vida
// ------------------------------------------------------------
stock HUD_UpdateHealth(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    new Float:hp;
    GetPlayerHealth(playerid, hp);
    PlayerData[playerid][pHealth] = hp;

    // Largura proporcional
    new Float:width = (HUD_BAR_WIDTH * hp) / 100.0;
    if(width < 1.0) width = 1.0;
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_HEALTH], width, 0.0);

    // Mudar cor quando critico
    new color = (hp <= 30.0) ? HUD_COLOR_HEALTH_LOW : HUD_COLOR_HEALTH;
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BAR_HEALTH], color);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_BAR_HEALTH]);
}

// Atualizar barra de colete
stock HUD_UpdateArmour(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    new Float:arm;
    GetPlayerArmour(playerid, arm);
    PlayerData[playerid][pArmour] = arm;

    new Float:width = (HUD_BAR_WIDTH * arm) / 100.0;
    if(arm <= 0.0) width = 1.0;
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_ARMOUR], width, 0.0);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_BAR_ARMOUR]);
}

// Atualizar barra de fome
stock HUD_UpdateHunger(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    new hunger = PlayerData[playerid][pHunger];
    new Float:width = (HUD_BAR_WIDTH * hunger) / float(MAX_HUNGER);
    if(width < 1.0) width = 1.0;

    new color = (hunger <= HUNGER_CRITICAL) ? HUD_COLOR_HEALTH_LOW : HUD_COLOR_HUNGER;
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_HUNGER], width, 0.0);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BAR_HUNGER], color);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_BAR_HUNGER]);
}

// Atualizar barra de sede
stock HUD_UpdateThirst(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    new thirst = PlayerData[playerid][pThirst];
    new Float:width = (HUD_BAR_WIDTH * thirst) / float(MAX_THIRST);
    if(width < 1.0) width = 1.0;

    new color = (thirst <= THIRST_CRITICAL) ? HUD_COLOR_HEALTH_LOW : HUD_COLOR_THIRST;
    PlayerTextDrawTextSize(playerid, g_HUD[playerid][HUD_BAR_THIRST], width, 0.0);
    PlayerTextDrawBoxColor(playerid, g_HUD[playerid][HUD_BAR_THIRST], color);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_BAR_THIRST]);
}

// Atualizar dinheiro e banco
stock HUD_UpdateMoney(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;

    new cash_str[32], bank_str[32];
    FormatMoneyFull(PlayerData[playerid][pMoney], cash_str, sizeof(cash_str));
    FormatMoneyFull(PlayerData[playerid][pBank], bank_str, sizeof(bank_str));

    PlayerTextDrawSetString(playerid, g_HUD[playerid][HUD_CASH_VALUE], cash_str);
    PlayerTextDrawSetString(playerid, g_HUD[playerid][HUD_BANK_VALUE], bank_str);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_CASH_VALUE]);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_BANK_VALUE]);
}

// Atualizar hora do servidor
stock HUD_UpdateTime(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    new h, m, s;
    gettime(h, m, s);
    new time_str[8];
    format(time_str, sizeof(time_str), "%02d:%02d", h, m);
    PlayerTextDrawSetString(playerid, g_HUD[playerid][HUD_TIME_VALUE], time_str);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_TIME_VALUE]);
}

// Atualizar zona
stock HUD_UpdateZone(playerid) {
    if(!IsValidPlayer(playerid) || !g_HUDCreated[playerid]) return;
    new zone[32];
    GetPlayerZone(playerid, zone, sizeof(zone));
    if(IsNullString(zone)) format(zone, sizeof(zone), "Santa Aurora");
    PlayerTextDrawSetString(playerid, g_HUD[playerid][HUD_ZONE_VALUE], zone);
    PlayerTextDrawShow(playerid, g_HUD[playerid][HUD_ZONE_VALUE]);
}

// ------------------------------------------------------------
// Scheduler: atualizar HUD a cada 5 segundos
// ------------------------------------------------------------
public OnScheduler5Seconds() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pIsLogged] &&
           Player_GetState(i) >= STATE_SPAWNED) {
            HUD_Update(i);
        }
    }
}

// Hora: atualizar a cada minuto
public OnScheduler1Minute() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {
            HUD_UpdateTime(i);
            HUD_UpdateZone(i);
        }
    }
}

