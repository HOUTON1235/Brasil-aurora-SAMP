#if defined _SA_PERMISSIONS_INC
    #endinput
#endif
#define _SA_PERMISSIONS_INC

#define ADMIN_NONE          0
#define ADMIN_HELPER        1
#define ADMIN_MODERATOR     2
#define ADMIN_ADMIN         3
#define ADMIN_SUPERVISOR    4
#define ADMIN_MANAGER       5
#define ADMIN_DIRECTOR      6
#define ADMIN_OWNER         7

stock GetAdminLevelName(level, output[], maxlen) {
    switch(level) {
        case ADMIN_HELPER:     format(output, maxlen, "Helper");
        case ADMIN_MODERATOR:  format(output, maxlen, "Moderador");
        case ADMIN_ADMIN:      format(output, maxlen, "Administrador");
        case ADMIN_SUPERVISOR: format(output, maxlen, "Supervisor");
        case ADMIN_MANAGER:    format(output, maxlen, "Gerente");
        case ADMIN_DIRECTOR:   format(output, maxlen, "Diretor");
        case ADMIN_OWNER:      format(output, maxlen, "Owner");
        default:               format(output, maxlen, "Jogador");
    }
}

#define PERM_ADMIN_PLAYER_BAN       "admin.player.ban"
#define PERM_ADMIN_PLAYER_KICK      "admin.player.kick"
#define PERM_ADMIN_PLAYER_MUTE      "admin.player.mute"
#define PERM_ADMIN_PLAYER_WARN      "admin.player.warn"
#define PERM_ADMIN_PLAYER_TELEPORT  "admin.player.teleport"
#define PERM_ADMIN_PLAYER_FREEZE    "admin.player.freeze"
#define PERM_ADMIN_PLAYER_SPECTATE  "admin.player.spectate"
#define PERM_ADMIN_PLAYER_JAIL      "admin.player.jail"
#define PERM_ADMIN_ECONOMY_GIVE     "admin.economy.give"
#define PERM_ADMIN_ECONOMY_REMOVE   "admin.economy.remove"
#define PERM_ADMIN_ECONOMY_EDIT     "admin.economy.edit"
#define PERM_ADMIN_ECONOMY_VIEW     "admin.economy.view"
#define PERM_ADMIN_VEHICLE_SPAWN    "admin.vehicle.spawn"
#define PERM_ADMIN_VEHICLE_DESTROY  "admin.vehicle.destroy"
#define PERM_ADMIN_VEHICLE_EDIT     "admin.vehicle.edit"
#define PERM_ADMIN_PROPERTY_EDIT    "admin.property.edit"
#define PERM_ADMIN_LOGS_VIEW        "admin.logs.view"
#define PERM_ADMIN_STATS_VIEW       "admin.stats.view"
#define PERM_ADMIN_SERVER_CONFIG    "admin.server.config"
#define PERM_POLICE_ARREST          "police.arrest"
#define PERM_POLICE_WEAPON          "police.weapon"
#define PERM_POLICE_HANDCUFF        "police.handcuff"
#define PERM_POLICE_SEARCH          "police.search"
#define PERM_FACTION_MEMBER_MANAGE  "faction.member.manage"
#define PERM_FACTION_RANK_MANAGE    "faction.rank.manage"
#define PERM_BUSINESS_MANAGE        "business.manage"

enum E_PERM_ENTRY {
    pe_Key[32],
    pe_Level
}

static g_PermMap[19][E_PERM_ENTRY] = {
    {"admin.player.ban",      ADMIN_MODERATOR},
    {"admin.player.kick",     ADMIN_HELPER},
    {"admin.player.mute",     ADMIN_HELPER},
    {"admin.player.warn",     ADMIN_HELPER},
    {"admin.player.teleport", ADMIN_HELPER},
    {"admin.player.freeze",   ADMIN_MODERATOR},
    {"admin.player.spectate", ADMIN_HELPER},
    {"admin.player.jail",     ADMIN_MODERATOR},
    {"admin.economy.give",    ADMIN_ADMIN},
    {"admin.economy.remove",  ADMIN_ADMIN},
    {"admin.economy.edit",    ADMIN_SUPERVISOR},
    {"admin.economy.view",    ADMIN_MODERATOR},
    {"admin.vehicle.spawn",   ADMIN_ADMIN},
    {"admin.vehicle.destroy", ADMIN_ADMIN},
    {"admin.vehicle.edit",    ADMIN_SUPERVISOR},
    {"admin.property.edit",   ADMIN_SUPERVISOR},
    {"admin.logs.view",       ADMIN_MODERATOR},
    {"admin.stats.view",      ADMIN_HELPER},
    {"admin.server.config",   ADMIN_OWNER}
};
#define MAX_PERMISSION_ENTRIES 19

