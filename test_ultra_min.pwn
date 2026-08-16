#include <a_samp>

#define INVALID_DB_ID    -1
#define INVALID_JOB_ID    0
#define INVALID_FACTION_ID 0
#define MAX_MONEY         2000000000
#define MAX_BANK_BALANCE  999999999
#define AFK_TIMEOUT       300
#define ADMIN_NONE        0
#define ADMIN_OWNER       7
#define STATE_DISCONNECTED 0
#define STATE_SPAWNED     5
#define STATE_AFK         10
#define STATE_DEAD        7
#define STATE_ADMIN_SPECTATING 11

#define IsValidPlayer(%0) ((%0) >= 0 && (%0) < MAX_PLAYERS && IsPlayerConnected(%0))

#include <player_core>

main() {}
