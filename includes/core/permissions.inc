// ============================================================
//  Santa Aurora Roleplay — Sistema de Permissões
//  Módulo : core/permissions.inc
//  Objetivo: permissões por chave + nível admin
// ============================================================

#if defined _SA_PERMISSIONS_INC
    #endinput
#endif
#define _SA_PERMISSIONS_INC

// ------------------------------------------------------------
// Níveis de administrador
// ------------------------------------------------------------
#define ADMIN_NONE          0
#define ADMIN_HELPER        1
#define ADMIN_MODERATOR     2
#define ADMIN_ADMIN         3
#define ADMIN_SUPERVISOR    4
#define ADMIN_MANAGER       5
#define ADMIN_DIRECTOR      6
#define ADMIN_OWNER         7

// Nomes dos níveis
stock GetAdminLevelName(level, output[], size = sizeof(output)) {
    switch(level) {
        case ADMIN_HELPER:      format(output, size, "Helper");
        case ADMIN_MODERATOR:   format(output, size, "Moderador");
        case ADMIN_ADMIN:       format(output, size, "Administrador");
        case ADMIN_SUPERVISOR:  format(output, size, "Supervisor");
        case ADMIN_MANAGER:     format(output, size, "Gerente");
        case ADMIN_DIRECTOR:    format(output, size, "Diretor");
        case ADMIN_OWNER:       format(output, size, "Owner");
        default:                format(output, size, "Jogador");
    }
}

// ------------------------------------------------------------
// Chaves de permissão
// ------------------------------------------------------------
// Admin — jogadores
#define PERM_ADMIN_PLAYER_TELEPORT      "admin.player.teleport"
#define PERM_ADMIN_PLAYER_BAN           "admin.player.ban"
#define PERM_ADMIN_PLAYER_KICK          "admin.player.kick"
#define PERM_ADMIN_PLAYER_MUTE          "admin.player.mute"
#define PERM_ADMIN_PLAYER_WARN          "admin.player.warn"
#define PERM_ADMIN_PLAYER_FREEZE        "admin.player.freeze"
#define PERM_ADMIN_PLAYER_SPECTATE      "admin.player.spectate"
#define PERM_ADMIN_PLAYER_JAIL          "admin.player.jail"

// Admin — economia
#define PERM_ADMIN_ECONOMY_GIVE         "admin.economy.give"
#define PERM_ADMIN_ECONOMY_REMOVE       "admin.economy.remove"
#define PERM_ADMIN_ECONOMY_EDIT         "admin.economy.edit"
#define PERM_ADMIN_ECONOMY_VIEW         "admin.economy.view"

// Admin — veículos
#define PERM_ADMIN_VEHICLE_SPAWN        "admin.vehicle.spawn"
#define PERM_ADMIN_VEHICLE_DESTROY      "admin.vehicle.destroy"
#define PERM_ADMIN_VEHICLE_EDIT         "admin.vehicle.edit"

// Admin — propriedades
#define PERM_ADMIN_PROPERTY_EDIT        "admin.property.edit"

// Admin — sistema
#define PERM_ADMIN_LOGS_VIEW            "admin.logs.view"
#define PERM_ADMIN_STATS_VIEW           "admin.stats.view"
#define PERM_ADMIN_SERVER_CONFIG        "admin.server.config"

// Polícia
#define PERM_POLICE_ARREST              "police.arrest"
#define PERM_POLICE_WEAPON              "police.weapon"
#define PERM_POLICE_HANDCUFF            "police.handcuff"
#define PERM_POLICE_SEARCH              "police.search"
#define PERM_POLICE_DISPATCH            "police.dispatch"

// Empresas
#define PERM_BUSINESS_MANAGE            "business.manage"
#define PERM_BUSINESS_EMPLOYEE_MANAGE   "business.employee.manage"
#define PERM_BUSINESS_STOCK_EDIT        "business.stock.edit"

// Facção
#define PERM_FACTION_MEMBER_MANAGE      "faction.member.manage"
#define PERM_FACTION_RANK_MANAGE        "faction.rank.manage"
#define PERM_FACTION_STORAGE            "faction.storage"

// ------------------------------------------------------------
// Mapa permissão → nível mínimo exigido
// ------------------------------------------------------------
static const PermissionMinLevel[][] = {
    // { "chave", nivel_minimo }
    { PERM_ADMIN_PLAYER_BAN,        ADMIN_MODERATOR  },
    { PERM_ADMIN_PLAYER_KICK,       ADMIN_HELPER     },
    { PERM_ADMIN_PLAYER_MUTE,       ADMIN_HELPER     },
    { PERM_ADMIN_PLAYER_WARN,       ADMIN_HELPER     },
    { PERM_ADMIN_PLAYER_TELEPORT,   ADMIN_HELPER     },
    { PERM_ADMIN_PLAYER_FREEZE,     ADMIN_MODERATOR  },
    { PERM_ADMIN_PLAYER_SPECTATE,   ADMIN_HELPER     },
    { PERM_ADMIN_PLAYER_JAIL,       ADMIN_MODERATOR  },
    { PERM_ADMIN_ECONOMY_GIVE,      ADMIN_ADMIN      },
    { PERM_ADMIN_ECONOMY_REMOVE,    ADMIN_ADMIN      },
    { PERM_ADMIN_ECONOMY_EDIT,      ADMIN_SUPERVISOR },
    { PERM_ADMIN_ECONOMY_VIEW,      ADMIN_MODERATOR  },
    { PERM_ADMIN_VEHICLE_SPAWN,     ADMIN_ADMIN      },
    { PERM_ADMIN_VEHICLE_DESTROY,   ADMIN_ADMIN      },
    { PERM_ADMIN_VEHICLE_EDIT,      ADMIN_SUPERVISOR },
    { PERM_ADMIN_PROPERTY_EDIT,     ADMIN_SUPERVISOR },
    { PERM_ADMIN_LOGS_VIEW,         ADMIN_MODERATOR  },
    { PERM_ADMIN_STATS_VIEW,        ADMIN_HELPER     },
    { PERM_ADMIN_SERVER_CONFIG,     ADMIN_OWNER      }
};
#define MAX_PERMISSION_ENTRIES  (sizeof(PermissionMinLevel))

// ------------------------------------------------------------
// Funções de verificação
// Nota: requerem acesso a PlayerData — declaradas como forward
// ------------------------------------------------------------
forward bool:Player_HasPermission(playerid, const permission[]);
forward bool:Player_IsAdmin(playerid, minLevel);
forward Player_GetAdminLevel(playerid);

