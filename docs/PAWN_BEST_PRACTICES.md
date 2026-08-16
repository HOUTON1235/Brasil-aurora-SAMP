# Pawn Best Practices & Standards Guide

**Santa Aurora Roleplay - Development Standards**

---

## Table of Contents
1. [Enum & Type System](#enum--type-system)
2. [Encapsulation Patterns](#encapsulation-patterns)
3. [Common Compiler Bugs](#common-compiler-bugs)
4. [Naming Conventions](#naming-conventions)
5. [Code Organization](#code-organization)
6. [Performance Optimization](#performance-optimization)
7. [Security Practices](#security-practices)
8. [Debugging & Testing](#debugging--testing)

---

## 📋 Enum & Type System

### ✅ CORRECT: Avoid `bool:` in Enum Declarations

**Problem**: The Pawn compiler has a critical bug when using `bool:` type tags in enum fields declared with arrays.

```pawn
// ❌ WRONG - Corrupts compiler parser
enum E_PLAYER_DATA {
    bool:pIsLogged,    // Type tag causes parser corruption
    bool:pIsDirty,
    bool:pIsAFK,
    pAFKTimer,
    pLastActivityTick
}
new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];  // Cascading errors!
```

**Solution**: Remove type tags from enum fields, use integer representation:

```pawn
// ✅ CORRECT - Use plain integers
enum E_PLAYER_DATA {
    pIsLogged,          // 0 = false, 1 = true
    pIsDirty,           // 0 = false, 1 = true
    pIsAFK,             // 0 = false, 1 = true
    pAFKTimer,
    pLastActivityTick
}
new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];  // No parser issues
```

### Type Safety via Encapsulation

Use getter/setter functions to maintain type safety while avoiding compiler bugs:

```pawn
// Getter with bool: return type
stock bool:Player_IsLogged(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return bool:PlayerData[playerid][pIsLogged];
}

// Setter
stock Player_SetLogged(playerid, bool:value) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pIsLogged] = value;
}

// Usage
if(Player_IsLogged(playerid)) {
    Player_SetLogged(playerid, false);
}
```

### Float vs Float: Clarity

Always specify Float type for floating-point fields:

```pawn
// ✅ CORRECT
enum E_POSITION {
    Float:posX,
    Float:posY,
    Float:posZ,
    Float:rotA
}

// ❌ WRONG - Ambiguous, prone to errors
enum E_POSITION {
    posX,  // Is this a float? An integer?
    posY,
    posZ,
    rotA
}
```

---

## 🔐 Encapsulation Patterns

### The Getter/Setter Pattern

**Purpose**: Control access to struct data, add validation, maintain type safety.

```pawn
// ===== MONEY MANAGEMENT =====
stock Player_GetMoney(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    return PlayerData[playerid][pMoney];
}

stock Player_AddMoney(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;
    
    new current = PlayerData[playerid][pMoney];
    new newAmount = current + amount;
    
    // Validate bounds
    if(newAmount > MAX_MONEY) newAmount = MAX_MONEY;
    if(newAmount < current) {  // Overflow check
        Logger_Security(playerid, "MONEY_OVERFLOW", "Overflow attempt");
        return false;
    }
    
    PlayerData[playerid][pMoney] = newAmount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, newAmount);
    Player_MarkDirty(playerid);
    
    // Fire event for observers
    FireEvent_iis("OnMoneyChanged", playerid, current, "Credit");
    return true;
}

stock bool:Player_RemoveMoney(playerid, amount) {
    if(!IsValidPlayer(playerid) || amount <= 0) return false;
    if(PlayerData[playerid][pMoney] < amount) return false;  // Insufficient funds
    
    new current = PlayerData[playerid][pMoney];
    new newAmount = current - amount;
    
    PlayerData[playerid][pMoney] = newAmount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, newAmount);
    Player_MarkDirty(playerid);
    
    FireEvent_iis("OnMoneyChanged", playerid, current, "Debit");
    return true;
}
```

### Boolean Flag Pattern

For `bool` fields in structs, always provide getter/setter with proper type casting:

```pawn
// ===== DIRTY FLAG PATTERN =====

// Getter
stock bool:Player_IsDirty(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    return bool:PlayerData[playerid][pIsDirty];
}

// Setters
stock Player_MarkDirty(playerid) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pIsDirty] = 1;
}

stock Player_ClearDirty(playerid) {
    if(!IsValidPlayer(playerid)) return;
    PlayerData[playerid][pIsDirty] = 0;
}

// Usage: Clean, type-safe
if(Player_IsDirty(playerid)) {
    Player_Save(playerid);
    Player_ClearDirty(playerid);
}
```

---

## 🐛 Common Compiler Bugs

### Bug #1: `bool:` Type Tags in Enums with Arrays

**Symptom**: Error 010 in seemingly unrelated functions after enum declaration.

**Root Cause**: Pawn compiler's internal parser is corrupted by `bool:` type tags when the enum is used in array declarations.

**Fix**: Remove `bool:` tags from enum fields → use encapsulation pattern.

```pawn
// ❌ BAD - Causes cascading errors
enum E_DATA {
    bool:field1,
    bool:field2
}
new data[MAX_PLAYERS][E_DATA];
// Later in code: Error 010 in unrelated functions!

// ✅ GOOD - No parser corruption
enum E_DATA {
    field1,
    field2
}
new data[MAX_PLAYERS][E_DATA];
// All functions compile cleanly
```

### Bug #2: Mixed Type Tags in Enum

**Symptom**: Unexpected type coercion or casting errors.

```pawn
// ❌ RISKY - Mixing Float: and bool: tags
enum E_PLAYER {
    Float:pHealth,
    bool:pIsOnline,  // Can cause parser state issues
    pMoney
}

// ✅ SAFER - Consistent tagging strategy
enum E_PLAYER {
    Float:pHealth,
    pIsOnline,       // Tag removed, encapsulated
    pMoney
}
```

### Bug #3: Default Parameters with Type Tags

**Symptom**: Compiler warnings or unexpected behavior with default values.

```pawn
// ❌ PROBLEMATIC
stock MyFunction(bool:flag = true) {  // Type tag in default param
    // ...
}

// ✅ BETTER
stock MyFunction(flag = true) {        // No type tag
    // ...
}
// Then cast in function body if needed:
if(bool:flag) { /* ... */ }
```

---

## 📝 Naming Conventions

### Variables

```pawn
// Global arrays (prefix with 'g_')
static g_PlayerName[MAX_PLAYERS][MAX_PLAYER_NAME];
static g_PlayerIP[MAX_PLAYERS][16];

// Local variables (normal camelCase)
new playerid, amount, result;

// Struct fields (prefix with struct/enum name initial)
enum E_PLAYER_DATA {
    pAccountID,      // p = player
    pHealth,
    pMoney
}

enum E_BUSINESS {
    biz_ID,          // biz = business
    biz_Name[32],
    biz_Owner
}
```

### Functions

```pawn
// Public callbacks (On*)
public OnPlayerConnect(playerid) { }
public OnPlayerSpawn(playerid) { }

// Stock functions (prefix with Module name)
stock Player_Init(playerid) { }
stock Player_GetMoney(playerid) { }
stock Business_Load() { }
stock Business_GetOwner(bizid) { }

// Forward declarations (match callback/stock signature)
forward OnPlayerLoaded(playerid);
forward OnMoneyChanged(playerid, oldamount, newamount);
```

### Constants & Defines

```pawn
// UPPERCASE for all constants
#define MAX_PLAYERS 200
#define MAX_MONEY 2000000000
#define SPAWN_DELAY 5000  // milliseconds

// Prefixed defines for clarity
#define COLOR_ADMIN 0xFF8C00FF
#define COLOR_ERROR 0xFF4444FF
#define CSTR_ADMIN "{FF8C00}"  // Embedded string color
#define CSTR_ERROR "{FF4444}"
```

---

## 🏗️ Code Organization

### File Structure

```
includes/
├── core/
│   ├── defines.inc       # Global constants
│   ├── macros.inc        # Utility macros
│   ├── states.inc        # State enums
│   └── permissions.inc   # Permission system
├── players/
│   ├── player_core.inc   # Core player struct & lifecycle
│   ├── player_auth.inc   # Authentication
│   └── player_data.inc   # Save/load logic
├── systems/
│   ├── business_core.inc
│   └── vehicle_core.inc
└── utils/
    ├── logger.inc
    └── db_utils.inc
```

### Module Pattern

Each include file should follow this structure:

```pawn
// ============================================================
// FILE HEADER - Module name, purpose, version
// ============================================================

#if defined _MODULE_NAME_INC
    #endinput
endif
#define _MODULE_NAME_INC

// ============================================================
// DEPENDENCIES - List required includes
// ============================================================
#include "../core/defines.inc"
#include "../core/macros.inc"

// ============================================================
// CONSTANTS & DEFINES
// ============================================================
#define SPAWN_DELAY 5000
#define MAX_INVENTORY 20

// ============================================================
// ENUMS & TYPES
// ============================================================
enum E_INVENTORY_ITEM {
    itemID,
    itemType,
    itemCount
}

// ============================================================
// GLOBAL VARIABLES
// ============================================================
static g_InventoryData[MAX_PLAYERS][MAX_INVENTORY][E_INVENTORY_ITEM];

// ============================================================
// INITIALIZATION
// ============================================================
stock Inventory_Init(playerid) {
    // ...
}

// ============================================================
// CORE FUNCTIONS (API)
// ============================================================
stock Inventory_AddItem(playerid, itemid, count) {
    // ...
}

stock Inventory_RemoveItem(playerid, itemid, count) {
    // ...
}
```

---

## ⚡ Performance Optimization

### Cache-Friendly Patterns

```pawn
// ❌ BAD - Multiple array accesses
if(PlayerData[playerid][pHealth] > 0 && 
   PlayerData[playerid][pIsLogged] && 
   PlayerData[playerid][pState] == STATE_SPAWNED) {
    // ...
}

// ✅ GOOD - Cache values in locals
new 
    health = PlayerData[playerid][pHealth],
    isLogged = PlayerData[playerid][pIsLogged],
    state = PlayerData[playerid][pState];

if(health > 0 && isLogged && state == STATE_SPAWNED) {
    // ...
}
```

### Avoid Redundant Checks

```pawn
// ❌ Redundant validation
for(new i = 0; i < MAX_PLAYERS; i++) {
    if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {
        if(PlayerData[i][pIsLogged]) {  // Check again!
            // ...
        }
    }
}

// ✅ Single validation
for(new i = 0; i < MAX_PLAYERS; i++) {
    if(IsValidPlayer(i) && PlayerData[i][pIsLogged]) {
        // Guaranteed to be logged
    }
}
```

### Use `static` for Initialization

```pawn
// Persistent across calls, initialized once
static initialized = false;

stock Initialize_System() {
    if(initialized) return;  // Already done
    
    // Heavy initialization here
    initialized = true;
}
```

---

## 🔒 Security Practices

### Validate All Inputs

```pawn
// ✅ CORRECT - Comprehensive validation
stock bool:Player_AddMoney(playerid, amount) {
    // 1. Validate player
    if(!IsValidPlayer(playerid)) return false;
    
    // 2. Validate amount
    if(amount <= 0) return false;
    if(amount > MAX_MONEY) return false;
    
    // 3. Check boundaries
    new newAmount = PlayerData[playerid][pMoney] + amount;
    if(newAmount > MAX_MONEY) newAmount = MAX_MONEY;  // Cap it
    
    // 4. Apply safely
    PlayerData[playerid][pMoney] = newAmount;
    return true;
}
```

### Log Security Events

```pawn
// Always log suspicious activity
stock Player_AddMoney(playerid, amount) {
    if(amount < 0) {
        Logger_Security(playerid, "NEGATIVE_MONEY", "Attempt to add negative money");
        return false;
    }
    
    if(amount > 1000000) {  // Large transaction
        new msg[64];
        format(msg, sizeof(msg), "Large transaction: %d", amount);
        Logger_Warning(playerid, "ECONOMY", msg);
    }
}
```

### Escape String Data

```pawn
// ✅ CORRECT - Escaped for SQL injection prevention
stock Player_SaveName(playerid) {
    new escaped_name[MAX_PLAYER_NAME * 2];
    DB_Escape(PlayerData[playerid][pName], escaped_name, sizeof(escaped_name));
    
    new query[256];
    format(query, sizeof(query),
        "UPDATE `characters` SET `name`='%s' WHERE `id`=%d",
        escaped_name,
        PlayerData[playerid][pCharacterID]);
    
    DB_Execute(query);
}
```

---

## 🧪 Debugging & Testing

### Strategic Logging

```pawn
// Define log levels
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARNING 2
#define LOG_ERROR 3

static g_LogLevel = LOG_INFO;  // Runtime configurable

stock Logger_Info(const category[], const message[]) {
    if(g_LogLevel <= LOG_INFO) {
        printf("[%s] %s", category, message);
    }
}

stock Logger_Debug(const category[], const message[]) {
    if(g_LogLevel <= LOG_DEBUG) {
        printf("[DEBUG] [%s] %s", category, message);
    }
}
```

### Assertions for Development

```pawn
// Macro for development assertions
#define ASSERT(%0) \
    if(!(%0)) { \
        printf("[ASSERTION FAILED] " #%0); \
        return; \
    }

// Usage
stock Player_SetMoney(playerid, amount) {
    ASSERT(IsValidPlayer(playerid));
    ASSERT(amount >= 0);
    // ...
}
```

### Test Patterns

```pawn
// Simple test harness
public OnGameModeInit() {
    Test_PlayerMoney();
    Test_PlayerState();
    // ...
}

stock Test_PlayerMoney() {
    printf("[TEST] Running Player Money tests...");
    
    new testid = 0;  // Fake player for testing
    Player_SetMoney(testid, 1000);
    ASSERT(Player_GetMoney(testid) == 1000);
    
    Player_AddMoney(testid, 500);
    ASSERT(Player_GetMoney(testid) == 1500);
    
    printf("[TEST] Player Money: PASSED");
}
```

---

## 📊 Summary Table: Do's and Don'ts

| Category | ✅ DO | ❌ DON'T |
|----------|-------|----------|
| **Enums** | Use plain ints for bool fields | Use `bool:` in enum decl. with arrays |
| **Types** | Specify `Float:` explicitly | Assume integer as float |
| **Access** | Use getter/setter functions | Direct array access everywhere |
| **Security** | Log & validate all inputs | Trust user input |
| **Performance** | Cache struct field values | Re-access array repeatedly |
| **Naming** | Prefix globals with `g_` | Mix naming styles |
| **SQL** | Always escape strings | Concatenate user data directly |
| **Validation** | Check bounds & ranges | Assume data is valid |
| **Logging** | Log security events | Silent failures |
| **Documentation** | Comment non-obvious code | Skip comments entirely |

---

## 🚀 Quick Checklist for New Code

- [ ] No `bool:` type tags in enum declarations with arrays
- [ ] All public functions documented with purpose
- [ ] Boundary validation on all inputs
- [ ] Security-critical events logged
- [ ] String data escaped for SQL
- [ ] Performance-critical sections use local caching
- [ ] Global variables prefixed with `g_`
- [ ] Getter/setter pattern for struct field access
- [ ] Proper error handling and return values
- [ ] Tests written for critical functions

---

**Last Updated**: 2026-08-16
**Author**: Santa Aurora Development Team
**Version**: 1.0
