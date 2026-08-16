// ============================================================
//  Santa Aurora Roleplay — Fome e Sede
//  Módulo : players/player_hunger.inc
//  Objetivo: sistema de fome e sede com efeitos progressivos
//            equilibrado para não frustrar jogadores
// ============================================================

#if defined _SA_PLAYER_HUNGER_INC
    #endinput
#endif
#define _SA_PLAYER_HUNGER_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "player_core.inc"

// ------------------------------------------------------------
// Configuração de decaimento
// Decai 1 ponto a cada X segundos (via scheduler de 60s)
// Com decaimento base de 1/min a fome dura 100 minutos
// ------------------------------------------------------------
#define HUNGER_DECAY_PER_MINUTE     1       // pontos por minuto
#define THIRST_DECAY_PER_MINUTE     2       // sede decai mais rápido

// Efeitos por nível
// 80-100: normal
// 40-79:  warning no chat (a cada 5 min)
// 20-39:  gambiarra visual + aviso
// 1-19:   dano passivo LEVE (0.5 hp/min) + tela piscando
// 0:      dano passivo MODERADO (1 hp/min) — não mata instantaneamente

#define HUNGER_WARN_INTERVAL        5       // minutos entre avisos de fome

// Cooldown para consumir item de comida (segundos)
#define FOOD_COOLDOWN_SECONDS       5

static g_LastFoodTime[MAX_PLAYERS]  = {0, ...};
static g_LastWarnTime[MAX_PLAYERS]  = {0, ...};
static g_HungerDamage[MAX_PLAYERS]  = {0, ...}; // acumular dano fracionado

// ------------------------------------------------------------
// Aplicar efeitos de fome/sede
// Chamado pelo scheduler a cada minuto
// ------------------------------------------------------------
stock Hunger_Tick(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!PlayerData[playerid][pIsLogged]) return;

    new E_PLAYER_STATE:st = Player_GetState(playerid);
    if(st == STATE_DEAD || st == STATE_AUTHENTICATING ||
       st == STATE_CONNECTING || st == STATE_CHARACTER_SELECTION) return;

    // Decaimento
    new hunger = PlayerData[playerid][pHunger];
    new thirst = PlayerData[playerid][pThirst];

    hunger -= HUNGER_DECAY_PER_MINUTE;
    thirst -= THIRST_DECAY_PER_MINUTE;

    // Decaimento extra se trabalhando/correndo
    // (placeholder — pode ser expandido)

    PlayerData[playerid][pHunger] = Clamp(hunger, 0, MAX_HUNGER);
    PlayerData[playerid][pThirst] = Clamp(thirst, 0, MAX_THIRST);
    Player_MarkDirty(playerid);

    // Aplicar efeitos
    Hunger_ApplyEffects(playerid);
}

// ------------------------------------------------------------
// Aplicar efeitos visuais/mecânicos conforme nível
// ------------------------------------------------------------
stock Hunger_ApplyEffects(playerid) {
    if(!IsValidPlayer(playerid)) return;

    new hunger = PlayerData[playerid][pHunger];
    new thirst = PlayerData[playerid][pThirst];
    new now    = gettime();

    // --- FOME ---
    if(hunger == 0) {
        // Dano passivo moderado
        g_HungerDamage[playerid]++;
        if(g_HungerDamage[playerid] >= 2) {
            g_HungerDamage[playerid] = 0;
            new Float:hp;
            GetPlayerHealth(playerid, hp);
            if(hp > 10.0) {
                SetPlayerHealth(playerid, hp - 1.0);
            }
        }
        // Aviso frequente
        if(now - g_LastWarnTime[playerid] >= 60) {
            SendError(playerid, "Você está com fome! Coma algo urgente.");
            g_LastWarnTime[playerid] = now;
        }
        GameTextForPlayer(playerid, "~r~Com fome!", 2000, 3);
    }
    else if(hunger <= HUNGER_CRITICAL) {
        // Dano leve
        if(g_HungerDamage[playerid] >= 3) {
            g_HungerDamage[playerid] = 0;
            new Float:hp;
            GetPlayerHealth(playerid, hp);
            if(hp > 20.0) SetPlayerHealth(playerid, hp - 0.5);
        }
        g_HungerDamage[playerid]++;
        if(now - g_LastWarnTime[playerid] >= 120) {
            SendWarning(playerid, "Você está com muita fome. Coma algo.");
            g_LastWarnTime[playerid] = now;
        }
    }
    else if(hunger <= HUNGER_WARNING) {
        if(now - g_LastWarnTime[playerid] >= 300) {
            SendInfo(playerid, CSTR_ORANGE"[Fome] "CSTR_WHITE"Você está com fome.");
            g_LastWarnTime[playerid] = now;
        }
    }

    // --- SEDE ---
    if(thirst == 0) {
        new Float:hp;
        GetPlayerHealth(playerid, hp);
        if(hp > 10.0) SetPlayerHealth(playerid, hp - 1.5); // sede é mais perigosa
        if(now - g_LastWarnTime[playerid] >= 60) {
            SendError(playerid, "Você está desidratado! Beba água urgente.");
            g_LastWarnTime[playerid] = now;
        }
        GameTextForPlayer(playerid, "~r~Desidratado!", 2000, 3);
    }
    else if(thirst <= THIRST_CRITICAL) {
        new Float:hp;
        GetPlayerHealth(playerid, hp);
        if(hp > 20.0) SetPlayerHealth(playerid, hp - 1.0);
        if(now - g_LastWarnTime[playerid] >= 120) {
            SendWarning(playerid, "Você está com muita sede. Beba algo.");
            g_LastWarnTime[playerid] = now;
        }
    }
    else if(thirst <= THIRST_WARNING) {
        if(now - g_LastWarnTime[playerid] >= 300) {
            SendInfo(playerid, CSTR_BLUE"[Sede] "CSTR_WHITE"Você está com sede.");
            g_LastWarnTime[playerid] = now;
        }
    }
}

// ------------------------------------------------------------
// Consumir comida/bebida
// Chamado pelo sistema de inventário
// ------------------------------------------------------------
stock Hunger_ConsumeFood(playerid, hunger_restore, thirst_restore,
                         const item_name[]) {
    if(!IsValidPlayer(playerid)) return false;

    // Cooldown anti-spam de comida
    new now = gettime();
    if(now - g_LastFoodTime[playerid] < FOOD_COOLDOWN_SECONDS) {
        SendWarning(playerid, "Aguarde antes de comer/beber novamente.");
        return false;
    }
    g_LastFoodTime[playerid] = now;

    // Aplicar restauração com limite
    if(hunger_restore > 0) {
        new new_hunger = Clamp(PlayerData[playerid][pHunger] + hunger_restore, 0, MAX_HUNGER);
        new diff = new_hunger - PlayerData[playerid][pHunger];
        PlayerData[playerid][pHunger] = new_hunger;
        if(diff > 0) {
            new msg[64];
            format(msg, sizeof(msg), CSTR_ORANGE"[+%d fome] "CSTR_WHITE"Você comeu %s.",
                diff, item_name);
            SendClientMessage(playerid, COLOR_ORANGE, msg);
        }
    }

    if(thirst_restore > 0) {
        new new_thirst = Clamp(PlayerData[playerid][pThirst] + thirst_restore, 0, MAX_THIRST);
        new diff = new_thirst - PlayerData[playerid][pThirst];
        PlayerData[playerid][pThirst] = new_thirst;
        if(diff > 0) {
            new msg[64];
            format(msg, sizeof(msg), CSTR_CYAN"[+%d sede] "CSTR_WHITE"Você bebeu %s.",
                diff, item_name);
            SendClientMessage(playerid, COLOR_CYAN, msg);
        }
    }

    Player_MarkDirty(playerid);

    // Animação de comer/beber
    if(hunger_restore > 0) {
        ApplyAnimation(playerid, "FOOD", "EAT_Burger", 4.1, false, false, false, false, 3000);
    } else if(thirst_restore > 0) {
        ApplyAnimation(playerid, "FOOD", "FF_drink", 4.1, false, false, false, false, 3000);
    }

    return true;
}

// ------------------------------------------------------------
// Restaurar fome/sede (para admins, eventos, etc.)
// ------------------------------------------------------------
stock Hunger_SetHunger(playerid, value) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pHunger] = Clamp(value, 0, MAX_HUNGER);
    Player_MarkDirty(playerid);
}

stock Hunger_SetThirst(playerid, value) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pThirst] = Clamp(value, 0, MAX_THIRST);
    Player_MarkDirty(playerid);
}

// ------------------------------------------------------------
// Scheduler: tick de fome a cada minuto
// ------------------------------------------------------------
public OnScheduler1Minute() {
    for(new i = 0; i < MAX_PLAYERS; i++) {
        if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {
            Hunger_Tick(i);
        }
    }
}

