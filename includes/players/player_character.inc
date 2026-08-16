// ============================================================
//  Santa Aurora Roleplay — Criação/Seleção de Personagem
//  Módulo : players/player_character.inc
//  Objetivo: dialog de criação de personagem (nome, sexo, idade, skin)
// ============================================================

#if defined _SA_PLAYER_CHARACTER_INC
    #endinput
#endif
#define _SA_PLAYER_CHARACTER_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../ui/ui_core.inc"
#include "player_core.inc"

// IDs de dialog de criação
#define DIALOG_CHAR_NAME    150
#define DIALOG_CHAR_SEX     151
#define DIALOG_CHAR_AGE     152
#define DIALOG_CHAR_SKIN    153
#define DIALOG_CHAR_CONFIRM 154

// Skins disponíveis para seleção inicial por sexo
// Masculino: 0, 1, 2, 5, 7, 14, 15, 16, 17, 18
// Feminino:  9, 10, 11, 12, 13, 31, 38, 39, 41, 42

static const g_SkinsMale[]  = {0, 1, 2, 5, 7, 14, 15, 16, 17, 18};
static const g_SkinsFemale[]= {9, 10, 11, 12, 13, 31, 38, 39, 41, 42};

// Estado temporário de criação
enum E_CHAR_CREATE {
    cc_Name[MAX_PLAYER_NAME + 4],
    cc_Sex,
    cc_Age,
    cc_Skin,
    cc_Step
}

static g_CharCreate[MAX_PLAYERS][E_CHAR_CREATE];

// ------------------------------------------------------------
// Iniciar fluxo de criação de personagem
// ------------------------------------------------------------
stock CharCreate_Start(playerid) {
    if(!IsValidPlayer(playerid)) return;

    // Pré-preencher nome da conta
    GetPlayerName(playerid, g_CharCreate[playerid][cc_Name],
        sizeof(g_CharCreate[][cc_Name]));
    g_CharCreate[playerid][cc_Sex]  = 0;
    g_CharCreate[playerid][cc_Age]  = 18;
    g_CharCreate[playerid][cc_Skin] = 0;
    g_CharCreate[playerid][cc_Step] = 0;

    CharCreate_ShowNameDialog(playerid);
}

// ------------------------------------------------------------
// Passo 1: Nome do personagem
// ------------------------------------------------------------
static CharCreate_ShowNameDialog(playerid) {
    new body[256];
    format(body, sizeof(body),
        "Digite o nome do seu personagem.\n\n"
        CSTR_GREY"Use formato: "CSTR_WHITE"PrimeiroNome_Sobrenome\n"
        CSTR_GREY"Exemplo: "CSTR_WHITE"Carlos_Silva\n\n"
        CSTR_GREY"(Mínimo 5, máximo 24 caracteres)\n"
        CSTR_GREY"Atual: "CSTR_SYSTEM"%s",
        g_CharCreate[playerid][cc_Name]);

    UI_ShowDialog(playerid, DIALOG_CHAR_NAME, DIALOG_STYLE_INPUT,
        "{ Criação de Personagem - Nome", body, "Continuar", "Cancelar");
}

// ------------------------------------------------------------
// Passo 2: Sexo
// ------------------------------------------------------------
static CharCreate_ShowSexDialog(playerid) {
    UI_ShowDialog(playerid, DIALOG_CHAR_SEX, DIALOG_STYLE_LIST,
        "{ Criação de Personagem - Sexo",
        "Masculino\nFeminino",
        "Selecionar", "Voltar");
}

// ------------------------------------------------------------
// Passo 3: Idade
// ------------------------------------------------------------
static CharCreate_ShowAgeDialog(playerid) {
    UI_ShowDialog(playerid, DIALOG_CHAR_AGE, DIALOG_STYLE_INPUT,
        "{ Criação de Personagem - Idade",
        "Digite a idade do personagem:\n\n"
        CSTR_GREY"Mínimo: "CSTR_WHITE"18  "CSTR_GREY"Máximo: "CSTR_WHITE"70",
        "Continuar", "Voltar");
}

// ------------------------------------------------------------
// Passo 4: Skin
// ------------------------------------------------------------
static CharCreate_ShowSkinDialog(playerid) {
    new body[256];
    new sex = g_CharCreate[playerid][cc_Sex];
    format(body, sizeof(body),
        "Escolha a aparência do personagem.\n\n"
        CSTR_GREY"Skins disponíveis (%s):\n",
        sex == 0 ? "Masculino" : "Feminino");

    if(sex == 0) {
        for(new i = 0; i < sizeof(g_SkinsMale); i++) {
            new line[16];
            format(line, sizeof(line), "Aparência %d (Skin %d)\n", i+1, g_SkinsMale[i]);
            strcat(body, line, sizeof(body));
        }
    } else {
        for(new i = 0; i < sizeof(g_SkinsFemale); i++) {
            new line[16];
            format(line, sizeof(line), "Aparência %d (Skin %d)\n", i+1, g_SkinsFemale[i]);
            strcat(body, line, sizeof(body));
        }
    }

    UI_ShowDialog(playerid, DIALOG_CHAR_SKIN, DIALOG_STYLE_LIST,
        "{ Criação de Personagem - Aparência",
        body, "Selecionar", "Voltar");
}

// ------------------------------------------------------------
// Passo 5: Confirmação
// ------------------------------------------------------------
static CharCreate_ShowConfirmDialog(playerid) {
    new body[256];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Confirmar Personagem ===\n\n"
        CSTR_WHITE"Nome:     "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Sexo:     "CSTR_SYSTEM"%s\n"
        CSTR_WHITE"Idade:    "CSTR_SYSTEM"%d anos\n"
        CSTR_WHITE"Skin:     "CSTR_SYSTEM"%d\n\n"
        CSTR_GREY"Confirma a criação deste personagem?",
        g_CharCreate[playerid][cc_Name],
        g_CharCreate[playerid][cc_Sex] == 0 ? "Masculino" : "Feminino",
        g_CharCreate[playerid][cc_Age],
        g_CharCreate[playerid][cc_Skin]);

    UI_ShowDialog(playerid, DIALOG_CHAR_CONFIRM, DIALOG_STYLE_MSGBOX,
        "{ Confirmar Personagem", body, "Criar Personagem", "Voltar");
}

// ------------------------------------------------------------
// Handler de dialogs de criação
// ------------------------------------------------------------
stock bool:CharCreate_HandleDialog(playerid, dialogid, response, listitem,
                                    const inputtext[]) {
    switch(dialogid) {
        case DIALOG_CHAR_NAME: {
            if(!response) {
                // Cancelar criação → kick (precisa criar personagem)
                SendError(playerid, "Você precisa criar um personagem para jogar.");
                SetTimerEx("Admin_DelayedKick", 1000, false, "i", playerid);
                return true;
            }
            // Validar nome
            if(strlen(inputtext) < 5 || strlen(inputtext) > MAX_PLAYER_NAME) {
                SendError(playerid, "Nome inválido. Use entre 5 e 24 caracteres.");
                CharCreate_ShowNameDialog(playerid);
                return true;
            }
            if(!IsValidPlayerName(inputtext)) {
                SendError(playerid, "Nome inválido. Use apenas letras, números e _.");
                CharCreate_ShowNameDialog(playerid);
                return true;
            }
            // Verificar se tem underscore (formato RP)
            if(strfind(inputtext, "_", true) == -1) {
                SendWarning(playerid,
                    "Recomendado usar formato PrimeiroNome_Sobrenome.");
            }
            SafeStrCopy(g_CharCreate[playerid][cc_Name], inputtext,
                sizeof(g_CharCreate[][cc_Name]));
            CharCreate_ShowSexDialog(playerid);
            return true;
        }
        case DIALOG_CHAR_SEX: {
            if(!response) {
                CharCreate_ShowNameDialog(playerid);
                return true;
            }
            g_CharCreate[playerid][cc_Sex] = listitem; // 0=M, 1=F
            CharCreate_ShowAgeDialog(playerid);
            return true;
        }
        case DIALOG_CHAR_AGE: {
            if(!response) {
                CharCreate_ShowSexDialog(playerid);
                return true;
            }
            new age = strval(inputtext);
            if(age < 18 || age > 70) {
                SendError(playerid, "Idade inválida. Mínimo 18, máximo 70 anos.");
                CharCreate_ShowAgeDialog(playerid);
                return true;
            }
            g_CharCreate[playerid][cc_Age] = age;
            CharCreate_ShowSkinDialog(playerid);
            return true;
        }
        case DIALOG_CHAR_SKIN: {
            if(!response) {
                CharCreate_ShowAgeDialog(playerid);
                return true;
            }
            new sex = g_CharCreate[playerid][cc_Sex];
            if(sex == 0) {
                if(listitem < 0 || listitem >= sizeof(g_SkinsMale)) listitem = 0;
                g_CharCreate[playerid][cc_Skin] = g_SkinsMale[listitem];
            } else {
                if(listitem < 0 || listitem >= sizeof(g_SkinsFemale)) listitem = 0;
                g_CharCreate[playerid][cc_Skin] = g_SkinsFemale[listitem];
            }
            // Prévia da skin
            SetPlayerSkin(playerid, g_CharCreate[playerid][cc_Skin]);
            CharCreate_ShowConfirmDialog(playerid);
            return true;
        }
        case DIALOG_CHAR_CONFIRM: {
            if(!response) {
                CharCreate_ShowSkinDialog(playerid);
                return true;
            }
            // Confirmar → gravar dados no PlayerData e salvar
            SafeStrCopy(PlayerData[playerid][pName],
                g_CharCreate[playerid][cc_Name],
                sizeof(PlayerData[][pName]));
            PlayerData[playerid][pSex]  = g_CharCreate[playerid][cc_Sex];
            PlayerData[playerid][pAge]  = g_CharCreate[playerid][cc_Age];
            PlayerData[playerid][pSkin] = g_CharCreate[playerid][cc_Skin];

            // Atualizar no banco
            new escaped_name[MAX_PLAYER_NAME * 2];
            DB_Escape(PlayerData[playerid][pName], escaped_name, sizeof(escaped_name));
            new query[256];
            format(query, sizeof(query),
                "UPDATE `characters` SET `name`='%s', `sex`=%d, `age`=%d, `skin_id`=%d "
                "WHERE `id`=%d",
                escaped_name,
                PlayerData[playerid][pSex],
                PlayerData[playerid][pAge],
                PlayerData[playerid][pSkin],
                PlayerData[playerid][pCharacterID]);
            DB_Execute(query);

            // Continuar fluxo (Tutorial cuidará do próximo passo)
            CallLocalFunction("OnCharacterCreated", "i", playerid);
            return true;
        }
    }
    return false;
}

forward OnCharacterCreated(playerid);

