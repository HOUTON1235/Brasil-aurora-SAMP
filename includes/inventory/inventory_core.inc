// ============================================================
//  Santa Aurora Roleplay — Inventário
//  Módulo : inventory/inventory_core.inc
//  Objetivo: sistema de inventário com item registry central,
//            peso, stack, uso, troca, descarte
// ============================================================

#if defined _SA_INVENTORY_CORE_INC
    #endinput
#endif
#define _SA_INVENTORY_CORE_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../core/events.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "../players/player_core.inc"

// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------
#define MAX_INV_SLOTS       20          // slots por inventário
#define MAX_ITEM_TYPES      200         // máximo de tipos de item
#define MAX_INV_WEIGHT      15.0        // kg máximo no inventário
#define INVALID_ITEM_ID     0
#define INVALID_SLOT        -1

// ------------------------------------------------------------
// Item Definition (cache em memória dos item_definitions)
// ------------------------------------------------------------
enum E_ITEM_DEF {
    idef_ID,
    idef_Name[32],
    idef_DisplayName[48],
    idef_Category[16],
    Float:idef_Weight,
    idef_MaxStack,
    idef_BaseValue,
    bool:idef_Usable,
    bool:idef_Stackable,
    idef_Description[128],
    idef_OnUseEffect[64]
}

static g_ItemDefs[MAX_ITEM_TYPES][E_ITEM_DEF];
static g_ItemDefCount = 0;
static bool:g_ItemDefsLoaded = false;

// ------------------------------------------------------------
// Item no inventário do jogador
// ------------------------------------------------------------
enum E_INV_ITEM {
    inv_DBID,           // ID na tabela inventory
    inv_ItemID,         // FK item_definitions.id
    inv_Quantity,
    inv_Slot,
    inv_Data[128]       // dados extras serializados
}

static g_Inventory[MAX_PLAYERS][MAX_INV_SLOTS][E_INV_ITEM];
static bool:g_InvLoaded[MAX_PLAYERS] = {false, ...};
static Float:g_InvWeight[MAX_PLAYERS] = {0.0, ...};

// ------------------------------------------------------------
// Inicializar inventário de um jogador (zerar slots)
// ------------------------------------------------------------
stock Inventory_Init(playerid) {
    if(!IsValidPlayer(playerid)) return;
    for(new s = 0; s < MAX_INV_SLOTS; s++) {
        g_Inventory[playerid][s][inv_DBID]     = INVALID_DB_ID;
        g_Inventory[playerid][s][inv_ItemID]   = INVALID_ITEM_ID;
        g_Inventory[playerid][s][inv_Quantity] = 0;
        g_Inventory[playerid][s][inv_Slot]     = s;
        g_Inventory[playerid][s][inv_Data][0]  = '\0';
    }
    g_InvWeight[playerid] = 0.0;
    g_InvLoaded[playerid] = false;
}

// ------------------------------------------------------------
// Carregar definições de itens do banco (uma vez no init)
// ------------------------------------------------------------
stock Inventory_LoadItemDefs() {
    if(!DB_IsConnected()) return;
    DB_Query(
        "SELECT `id`, `name`, `display_name`, `category`, `weight`, "
        "`max_stack`, `base_value`, `usable`, `stackable`, `description`, `on_use_effect` "
        "FROM `item_definitions` ORDER BY `id` ASC",
        "OnItemDefsLoaded", "");
}

forward OnItemDefsLoaded();
public OnItemDefsLoaded() {
    g_ItemDefCount = 0;
    new rows = DB_NumRows();
    for(new r = 0; r < rows && g_ItemDefCount < MAX_ITEM_TYPES; r++) {
        new i = g_ItemDefCount;
        g_ItemDefs[i][idef_ID] = DB_GetInt("id", r);
        DB_GetString("name",         g_ItemDefs[i][idef_Name],        sizeof(g_ItemDefs[][idef_Name]),        r);
        DB_GetString("display_name", g_ItemDefs[i][idef_DisplayName], sizeof(g_ItemDefs[][idef_DisplayName]), r);
        DB_GetString("category",     g_ItemDefs[i][idef_Category],    sizeof(g_ItemDefs[][idef_Category]),    r);
        g_ItemDefs[i][idef_Weight]       = DB_GetFloat("weight", r);
        g_ItemDefs[i][idef_MaxStack]     = DB_GetInt("max_stack", r);
        g_ItemDefs[i][idef_BaseValue]    = DB_GetInt("base_value", r);
        g_ItemDefs[i][idef_Usable]       = bool:DB_GetInt("usable", r);
        g_ItemDefs[i][idef_Stackable]    = bool:DB_GetInt("stackable", r);
        DB_GetString("description",  g_ItemDefs[i][idef_Description], sizeof(g_ItemDefs[][idef_Description]), r);
        DB_GetString("on_use_effect",g_ItemDefs[i][idef_OnUseEffect], sizeof(g_ItemDefs[][idef_OnUseEffect]), r);
        g_ItemDefCount++;
    }
    g_ItemDefsLoaded = true;
    new msg[48];
    format(msg, sizeof(msg), "%d tipos de item carregados.", g_ItemDefCount);
    Logger_Info("INVENTORY", msg);
}

// ------------------------------------------------------------
// Obter definição de item por ID
// Retorna índice em g_ItemDefs ou -1
// ------------------------------------------------------------
stock Inventory_GetItemDefIndex(itemid) {
    for(new i = 0; i < g_ItemDefCount; i++) {
        if(g_ItemDefs[i][idef_ID] == itemid) return i;
    }
    return -1;
}

// Obter definição por nome (ex: "ITEM_WATER")
stock Inventory_GetItemByName(const name[]) {
    for(new i = 0; i < g_ItemDefCount; i++) {
        if(strequal(g_ItemDefs[i][idef_Name], name, true)) return g_ItemDefs[i][idef_ID];
    }
    return INVALID_ITEM_ID;
}

// ------------------------------------------------------------
// Carregar inventário do jogador do banco
// ------------------------------------------------------------
stock Inventory_Load(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(PlayerData[playerid][pCharacterID] == INVALID_DB_ID) return;

    Inventory_Init(playerid);

    new query[128];
    format(query, sizeof(query),
        "SELECT `id`, `item_id`, `quantity`, `slot`, `data` "
        "FROM `inventory` WHERE `character_id`=%d ORDER BY `slot` ASC",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnInventoryLoad", "i", playerid);
}

forward OnInventoryLoad(playerid);
public OnInventoryLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new rows = DB_NumRows();
    g_InvWeight[playerid] = 0.0;

    for(new r = 0; r < rows; r++) {
        new slot = DB_GetInt("slot", r);
        if(slot < 0 || slot >= MAX_INV_SLOTS) continue;

        g_Inventory[playerid][slot][inv_DBID]     = DB_GetInt("id", r);
        g_Inventory[playerid][slot][inv_ItemID]   = DB_GetInt("item_id", r);
        g_Inventory[playerid][slot][inv_Quantity] = DB_GetInt("quantity", r);
        g_Inventory[playerid][slot][inv_Slot]     = slot;
        DB_GetString("data", g_Inventory[playerid][slot][inv_Data],
            sizeof(g_Inventory[][][inv_Data]), r);

        // Calcular peso
        new defIdx = Inventory_GetItemDefIndex(g_Inventory[playerid][slot][inv_ItemID]);
        if(defIdx >= 0) {
            g_InvWeight[playerid] += g_ItemDefs[defIdx][idef_Weight] *
                                     float(g_Inventory[playerid][slot][inv_Quantity]);
        }
    }
    g_InvLoaded[playerid] = true;
    return 1;
}

// ------------------------------------------------------------
// Salvar inventário (slot por slot — dirty)
// Chamado no save do personagem
// ------------------------------------------------------------
stock Inventory_Save(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(!g_InvLoaded[playerid]) return;
    new char_id = PlayerData[playerid][pCharacterID];
    if(char_id == INVALID_DB_ID) return;

    // Deletar e reinserir (simples e seguro contra duplicação)
    new del_query[64];
    format(del_query, sizeof(del_query),
        "DELETE FROM `inventory` WHERE `character_id`=%d", char_id);
    DB_Execute(del_query);

    for(new s = 0; s < MAX_INV_SLOTS; s++) {
        if(g_Inventory[playerid][s][inv_ItemID] == INVALID_ITEM_ID) continue;
        if(g_Inventory[playerid][s][inv_Quantity] <= 0) continue;

        new escaped_data[256];
        DB_Escape(g_Inventory[playerid][s][inv_Data], escaped_data, sizeof(escaped_data));

        new query[512];
        format(query, sizeof(query),
            "INSERT INTO `inventory` (`character_id`, `item_id`, `quantity`, `slot`, `data`) "
            "VALUES (%d, %d, %d, %d, '%s')",
            char_id,
            g_Inventory[playerid][s][inv_ItemID],
            g_Inventory[playerid][s][inv_Quantity],
            s,
            escaped_data);
        DB_Execute(query);
    }
}

// ------------------------------------------------------------
// Peso atual do inventário
// ------------------------------------------------------------
stock Float:Inventory_GetWeight(playerid) {
    if(!IsValidPlayer(playerid)) return 0.0;
    return g_InvWeight[playerid];
}

// ------------------------------------------------------------
// Verificar se cabe no inventário
// ------------------------------------------------------------
stock bool:Inventory_CanAdd(playerid, itemid, quantity) {
    if(!IsValidPlayer(playerid)) return false;
    new defIdx = Inventory_GetItemDefIndex(itemid);
    if(defIdx < 0) return false;

    new Float:add_weight = g_ItemDefs[defIdx][idef_Weight] * float(quantity);
    return (g_InvWeight[playerid] + add_weight <= MAX_INV_WEIGHT);
}

// ------------------------------------------------------------
// Contar quantidade de um item no inventário
// ------------------------------------------------------------
stock Inventory_Count(playerid, itemid) {
    if(!IsValidPlayer(playerid)) return 0;
    new total = 0;
    for(new s = 0; s < MAX_INV_SLOTS; s++) {
        if(g_Inventory[playerid][s][inv_ItemID] == itemid) {
            total += g_Inventory[playerid][s][inv_Quantity];
        }
    }
    return total;
}

stock bool:Inventory_HasItem(playerid, itemid, quantity = 1) {
    return (Inventory_Count(playerid, itemid) >= quantity);
}

// ------------------------------------------------------------
// Encontrar slot livre
// ------------------------------------------------------------
static Inventory_FindFreeSlot(playerid) {
    for(new s = 0; s < MAX_INV_SLOTS; s++) {
        if(g_Inventory[playerid][s][inv_ItemID] == INVALID_ITEM_ID) return s;
    }
    return INVALID_SLOT;
}

// Encontrar slot com item específico que ainda pode empilhar
static Inventory_FindStackableSlot(playerid, itemid) {
    new defIdx = Inventory_GetItemDefIndex(itemid);
    if(defIdx < 0 || !g_ItemDefs[defIdx][idef_Stackable]) return INVALID_SLOT;

    new maxStack = g_ItemDefs[defIdx][idef_MaxStack];
    for(new s = 0; s < MAX_INV_SLOTS; s++) {
        if(g_Inventory[playerid][s][inv_ItemID] == itemid &&
           g_Inventory[playerid][s][inv_Quantity] < maxStack) {
            return s;
        }
    }
    return INVALID_SLOT;
}

// ------------------------------------------------------------
// Adicionar item
// Retorna true se adicionou com sucesso
// ------------------------------------------------------------
stock bool:Inventory_AddItem(playerid, itemid, quantity = 1,
                             const data[] = "", notify = true) {
    if(!IsValidPlayer(playerid) || quantity <= 0) return false;

    new defIdx = Inventory_GetItemDefIndex(itemid);
    if(defIdx < 0) {
        Logger_Error("INVENTORY", "Inventory_AddItem: item inválido");
        return false;
    }

    if(!Inventory_CanAdd(playerid, itemid, quantity)) {
        if(notify) UI_NotifyError(playerid, "Inventário cheio! Você não pode carregar mais peso.");
        return false;
    }

    new remaining = quantity;

    // Tentar empilhar em slots existentes
    if(g_ItemDefs[defIdx][idef_Stackable]) {
        new maxStack = g_ItemDefs[defIdx][idef_MaxStack];
        for(new s = 0; s < MAX_INV_SLOTS && remaining > 0; s++) {
            if(g_Inventory[playerid][s][inv_ItemID] == itemid &&
               g_Inventory[playerid][s][inv_Quantity] < maxStack) {
                new space = maxStack - g_Inventory[playerid][s][inv_Quantity];
                new add   = (remaining > space) ? space : remaining;
                g_Inventory[playerid][s][inv_Quantity] += add;
                remaining -= add;
            }
        }
    }

    // Se ainda há quantidade, abrir slots novos
    while(remaining > 0) {
        new slot = Inventory_FindFreeSlot(playerid);
        if(slot == INVALID_SLOT) {
            if(notify) UI_NotifyError(playerid, "Sem espaço no inventário.");
            break;
        }
        new defIdx2 = Inventory_GetItemDefIndex(itemid);
        new maxStack = (defIdx2 >= 0) ? g_ItemDefs[defIdx2][idef_MaxStack] : 99;
        new add = (remaining > maxStack) ? maxStack : remaining;

        g_Inventory[playerid][slot][inv_DBID]     = INVALID_DB_ID;
        g_Inventory[playerid][slot][inv_ItemID]   = itemid;
        g_Inventory[playerid][slot][inv_Quantity] = add;
        g_Inventory[playerid][slot][inv_Slot]     = slot;
        SafeStrCopy(g_Inventory[playerid][slot][inv_Data], data,
            sizeof(g_Inventory[][][inv_Data]));
        remaining -= add;
    }

    // Recalcular peso
    new added = quantity - remaining;
    g_InvWeight[playerid] += g_ItemDefs[defIdx][idef_Weight] * float(added);
    g_InvWeight[playerid] = ClampFloat(g_InvWeight[playerid], 0.0, MAX_INV_WEIGHT + 1.0);

    Player_MarkDirty(playerid);

    if(notify && added > 0) {
        new msg[64];
        format(msg, sizeof(msg), CSTR_GREEN"[+] "CSTR_WHITE"%dx %s adicionado ao inventário.",
            added, g_ItemDefs[defIdx][idef_DisplayName]);
        SendClientMessage(playerid, COLOR_SUCCESS, msg);
    }

    new reason[32] = "add";
    CallLocalFunction("OnInventoryChanged", "iis", playerid, itemid, reason);
    return (added > 0);
}

// ------------------------------------------------------------
// Remover item
// ------------------------------------------------------------
stock bool:Inventory_RemoveItem(playerid, itemid, quantity = 1, notify = true) {
    if(!IsValidPlayer(playerid) || quantity <= 0) return false;

    if(!Inventory_HasItem(playerid, itemid, quantity)) {
        if(notify) UI_NotifyError(playerid, "Você não possui este item.");
        return false;
    }

    new defIdx = Inventory_GetItemDefIndex(itemid);
    new remaining = quantity;

    for(new s = 0; s < MAX_INV_SLOTS && remaining > 0; s++) {
        if(g_Inventory[playerid][s][inv_ItemID] != itemid) continue;
        new available = g_Inventory[playerid][s][inv_Quantity];
        new remove    = (remaining > available) ? available : remaining;

        g_Inventory[playerid][s][inv_Quantity] -= remove;
        remaining -= remove;

        if(g_Inventory[playerid][s][inv_Quantity] <= 0) {
            // Limpar slot
            g_Inventory[playerid][s][inv_ItemID]   = INVALID_ITEM_ID;
            g_Inventory[playerid][s][inv_DBID]     = INVALID_DB_ID;
            g_Inventory[playerid][s][inv_Data][0]  = '\0';
        }
    }

    // Recalcular peso
    if(defIdx >= 0) {
        new removed = quantity - remaining;
        g_InvWeight[playerid] -= g_ItemDefs[defIdx][idef_Weight] * float(removed);
        g_InvWeight[playerid] = ClampFloat(g_InvWeight[playerid], 0.0, MAX_INV_WEIGHT + 1.0);
    }

    Player_MarkDirty(playerid);

    if(notify) {
        new display_name[48];
        if(defIdx >= 0) SafeStrCopy(display_name, g_ItemDefs[defIdx][idef_DisplayName], sizeof(display_name));
        else format(display_name, sizeof(display_name), "Item #%d", itemid);
        new msg[64];
        format(msg, sizeof(msg), CSTR_ERROR"[-] "CSTR_WHITE"%dx %s removido do inventário.",
            quantity, display_name);
        SendClientMessage(playerid, COLOR_WARNING, msg);
    }

    new reason[32] = "remove";
    CallLocalFunction("OnInventoryChanged", "iis", playerid, itemid, reason);
    return true;
}

// ------------------------------------------------------------
// Usar item
// ------------------------------------------------------------
stock bool:Inventory_UseItem(playerid, slot) {
    if(!IsValidPlayer(playerid)) return false;
    if(slot < 0 || slot >= MAX_INV_SLOTS) return false;
    if(g_Inventory[playerid][slot][inv_ItemID] == INVALID_ITEM_ID) return false;

    new itemid  = g_Inventory[playerid][slot][inv_ItemID];
    new defIdx  = Inventory_GetItemDefIndex(itemid);
    if(defIdx < 0) return false;

    if(!g_ItemDefs[defIdx][idef_Usable]) {
        UI_NotifyError(playerid, "Este item não pode ser usado diretamente.");
        return false;
    }

    // Processar efeito
    new effect[64];
    SafeStrCopy(effect, g_ItemDefs[defIdx][idef_OnUseEffect], sizeof(effect));
    new display_name[48];
    SafeStrCopy(display_name, g_ItemDefs[defIdx][idef_DisplayName], sizeof(display_name));

    new bool:consumed = false;

    // Efeitos de comida/bebida
    if(strfind(effect, "hunger+", true) != -1) {
        new restore = strval(effect[7]);
        Hunger_ConsumeFood(playerid, restore, 0, display_name);
        consumed = true;
    }
    else if(strfind(effect, "thirst+", true) != -1) {
        new restore = strval(effect[7]);
        Hunger_ConsumeFood(playerid, 0, restore, display_name);
        consumed = true;
    }
    // Efeito de saúde (kit médico, curativo)
    else if(strfind(effect, "health+", true) != -1) {
        new restore = strval(effect[7]);
        new Float:hp;
        GetPlayerHealth(playerid, hp);
        new Float:new_hp = ClampFloat(hp + float(restore), 0.0, 100.0);
        SetPlayerHealth(playerid, new_hp);
        new msg[64];
        format(msg, sizeof(msg), CSTR_GREEN"[+%.0f HP] "CSTR_WHITE"Você usou %s.",
            new_hp - hp, display_name);
        SendClientMessage(playerid, COLOR_SUCCESS, msg);
        consumed = true;
    }
    // Outros efeitos são delegados a módulos específicos
    else {
        CallLocalFunction("OnItemUsed", "iis", playerid, itemid, effect);
        return true; // módulo especializado decide se consome
    }

    if(consumed) {
        Inventory_RemoveItem(playerid, itemid, 1, false);
    }
    return true;
}

// ------------------------------------------------------------
// Exibir inventário (dialog de lista)
// ------------------------------------------------------------
stock Inventory_ShowDialog(playerid) {
    if(!IsValidPlayer(playerid)) return;

    UI_ListReset();

    new has_items = false;
    for(new s = 0; s < MAX_INV_SLOTS; s++) {
        if(g_Inventory[playerid][s][inv_ItemID] == INVALID_ITEM_ID) continue;
        new defIdx = Inventory_GetItemDefIndex(g_Inventory[playerid][s][inv_ItemID]);
        if(defIdx < 0) continue;

        new line[64];
        format(line, sizeof(line), "%s  x%d  [%.1f kg]",
            g_ItemDefs[defIdx][idef_DisplayName],
            g_Inventory[playerid][s][inv_Quantity],
            g_ItemDefs[defIdx][idef_Weight] * float(g_Inventory[playerid][s][inv_Quantity]));
        UI_ListAdd(line);
        has_items = true;
    }

    if(!has_items) {
        UI_ListAdd("Inventário vazio.");
    }

    new title[48];
    new Float:w = g_InvWeight[playerid];
    format(title, sizeof(title), "Inventário  [%.1f/%.1f kg]", w, MAX_INV_WEIGHT);
    UI_ListShow(playerid, DIALOG_INVENTORY_LIST, title, "Usar", "Fechar");
}

// IDs de dialog do inventário
#define DIALOG_INVENTORY_LIST   300
#define DIALOG_INVENTORY_USE    301

// Handler de dialog do inventário
stock bool:Inventory_HandleDialog(playerid, dialogid, response, listitem) {
    switch(dialogid) {
        case DIALOG_INVENTORY_LIST: {
            if(!response) return true;
            // Encontrar qual slot foi selecionado (listitem = posição na lista)
            new count = 0;
            for(new s = 0; s < MAX_INV_SLOTS; s++) {
                if(g_Inventory[playerid][s][inv_ItemID] == INVALID_ITEM_ID) continue;
                if(count == listitem) {
                    Inventory_UseItem(playerid, s);
                    return true;
                }
                count++;
            }
            return true;
        }
    }
    return false;
}

// Getter para admin/outros módulos
stock Inventory_GetItemInSlot(playerid, slot, &itemid, &quantity) {
    if(!IsValidPlayer(playerid) || slot < 0 || slot >= MAX_INV_SLOTS) return false;
    itemid   = g_Inventory[playerid][slot][inv_ItemID];
    quantity = g_Inventory[playerid][slot][inv_Quantity];
    return (itemid != INVALID_ITEM_ID);
}

// Integração com OnPlayerLoaded
public OnPlayerLoaded(playerid) {
    Inventory_Load(playerid);
    return 1;
}

// Integração com save
public OnPlayerSaved(playerid) {
    Inventory_Save(playerid);
    return 1;
}

// Limpar ao desconectar
public OnPlayerDisconnect(playerid, reason) {
    Inventory_Init(playerid);
    return 1;
}

// Forward para itens com efeito personalizado
forward OnItemUsed(playerid, itemid, effect[]);

