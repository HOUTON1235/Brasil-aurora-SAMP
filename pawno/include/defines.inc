// ============================================================
//  Santa Aurora Roleplay ? Core Defines
//  Modulo : core/defines.inc
//  Objetivo: constantes globais do projeto
// ============================================================

#if defined _SA_DEFINES_INC
    #endinput
#endif
#define _SA_DEFINES_INC

// ------------------------------------------------------------
// Versao
// ------------------------------------------------------------
#define SERVER_VERSION          "0.1.0"
#define SERVER_VERSION_MAJOR    0
#define SERVER_VERSION_MINOR    1
#define SERVER_VERSION_PATCH    0
#define SERVER_NAME             "Santa Aurora - Roleplay Brasileiro"
#define SERVER_LANG             "pt-BR"

// ------------------------------------------------------------
// Limites
// ------------------------------------------------------------
#define MAX_PLAYERS             200
#define MAX_NAME_LENGTH         24
#define MAX_PASSWORD_LENGTH     64
#define MAX_EMAIL_LENGTH        64
#define MAX_REASON_LENGTH       128
#define MAX_LOG_MESSAGE         256
#define MAX_QUERY_LENGTH        512
#define MAX_DIALOG_TEXT         4096

// ------------------------------------------------------------
// Economia
// ------------------------------------------------------------
#define MAX_MONEY               2000000000   // 2 bilhoes (int32 safe)
#define MIN_MONEY               0
#define STARTING_MONEY          500
#define STARTING_BANK           1000
#define MAX_BANK_BALANCE        999999999

// ------------------------------------------------------------
// Gameplay
// ------------------------------------------------------------
#define INTERACTION_RANGE       2.5         // metros para interagir
#define AFK_TIMEOUT             300         // segundos
#define SAVE_INTERVAL           300         // segundos entre saves automaticos
#define LOGIN_MAX_ATTEMPTS      5
#define ANTISPAM_CMD_DELAY      500         // ms entre comandos

// ------------------------------------------------------------
// Fome e Sede
// ------------------------------------------------------------
#define MAX_HUNGER              100
#define MAX_THIRST              100
#define HUNGER_CRITICAL         20
#define THIRST_CRITICAL         20
#define HUNGER_WARNING          40
#define THIRST_WARNING          40

// ------------------------------------------------------------
// Nivel / XP
// ------------------------------------------------------------
#define MAX_LEVEL               100
#define XP_PER_LEVEL_BASE       1000        // XP base por nivel

// ------------------------------------------------------------
// Cores (RGBA hex)
// ------------------------------------------------------------
// Paleta principal
#define COLOR_WHITE             0xFFFFFFFF
#define COLOR_GREY              0xAAAAAAAAAA
#define COLOR_DARK_GREY         0x555555FF
#define COLOR_BLACK             0x000000FF
#define COLOR_RED               0xFF4444FF
#define COLOR_RED_DARK          0xCC0000FF
#define COLOR_GREEN             0x44FF44FF
#define COLOR_GREEN_DARK        0x00AA00FF
#define COLOR_BLUE              0x4488FFFF
#define COLOR_BLUE_DARK         0x0055CCFF
#define COLOR_YELLOW            0xFFFF00FF
#define COLOR_ORANGE            0xFF8800FF
#define COLOR_PURPLE            0xAA44FFFF
#define COLOR_PINK              0xFF44AAFF
#define COLOR_CYAN              0x00FFFFFF

// Semantico
#define COLOR_SUCCESS           COLOR_GREEN
#define COLOR_ERROR             COLOR_RED
#define COLOR_INFO              COLOR_WHITE
#define COLOR_WARNING           COLOR_YELLOW
#define COLOR_MONEY             COLOR_GREEN
#define COLOR_ADMIN             0xFF8C00FF   // laranja admin
#define COLOR_SYSTEM            COLOR_BLUE
#define COLOR_CHAT_LOCAL        COLOR_WHITE
#define COLOR_CHAT_ME           0xFFAA00FF
#define COLOR_CHAT_DO           0x9FC4E4FF
#define COLOR_CHAT_OOC          0x8DB4CAFF
#define COLOR_CHAT_PM           0xFF69B4FF
#define COLOR_CHAT_FACTION      0xAAFFAAFF

// Embedding no texto SA-MP
#define CSTR_WHITE              "{FFFFFF}"
#define CSTR_RED                "{FF4444}"
#define CSTR_GREEN              "{44FF44}"
#define CSTR_BLUE               "{4488FF}"
#define CSTR_YELLOW             "{FFFF00}"
#define CSTR_ORANGE             "{FF8800}"
#define CSTR_PURPLE             "{AA44FF}"
#define CSTR_GREY               "{AAAAAA}"
#define CSTR_MONEY              "{44FF44}"
#define CSTR_ERROR              "{FF4444}"
#define CSTR_SUCCESS            "{44FF44}"
#define CSTR_WARNING            "{FFFF00}"
#define CSTR_ADMIN              "{FF8C00}"
#define CSTR_SYSTEM             "{4488FF}"

// ------------------------------------------------------------
// IDs de Dialog
// ------------------------------------------------------------
#define DIALOG_NONE             -1
#define DIALOG_LOGIN            100
#define DIALOG_REGISTER         101
#define DIALOG_REGISTER_CONFIRM 102
#define DIALOG_CHARACTER_CREATE 103
#define DIALOG_CHARACTER_SEX    104
#define DIALOG_CHARACTER_AGE    105
#define DIALOG_MAIN_MENU        110
#define DIALOG_CONFIRM_YES_NO   120
#define DIALOG_ADMIN_MENU       200
#define DIALOG_ADMIN_PLAYER     201
#define DIALOG_ADMIN_BAN        202
#define DIALOG_ADMIN_KICK       203
#define DIALOG_ADMIN_WARN       204
#define DIALOG_ADMIN_GIVEMONEY  205

// ------------------------------------------------------------
// Variaveis de timer do scheduler
// ------------------------------------------------------------
#define SCHEDULER_TICK_MS       1000        // 1 segundo
#define SCHEDULER_INTERVAL_1S   1
#define SCHEDULER_INTERVAL_5S   5
#define SCHEDULER_INTERVAL_10S  10
#define SCHEDULER_INTERVAL_30S  30
#define SCHEDULER_INTERVAL_60S  60
#define SCHEDULER_INTERVAL_5MIN 300

// ------------------------------------------------------------
// Misc
// ------------------------------------------------------------
#define INVALID_DB_ID           -1
#define INVALID_FACTION_ID      0
#define INVALID_JOB_ID          0

