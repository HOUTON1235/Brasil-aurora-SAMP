// ============================================================
//  Santa Aurora Roleplay — Documentos
//  Módulo : players/player_documents.inc
//  Objetivo: RG, CPF, CNH e outros documentos do personagem
// ============================================================

#if defined _SA_PLAYER_DOCUMENTS_INC
    #endinput
#endif
#define _SA_PLAYER_DOCUMENTS_INC

#include "../core/defines.inc"
#include "../core/macros.inc"
#include "../logs/logger.inc"
#include "../ui/ui_core.inc"
#include "player_core.inc"

// ------------------------------------------------------------
// Tipos de documento
// ------------------------------------------------------------
#define DOC_RG              "rg"
#define DOC_CPF             "cpf"
#define DOC_CNH             "cnh"
#define DOC_GUN_LICENSE     "gun_license"
#define DOC_WORK_LICENSE    "work_license"

// Status de documento
#define DOC_STATUS_VALID    "valid"
#define DOC_STATUS_EXPIRED  "expired"
#define DOC_STATUS_REVOKED  "revoked"

// Categorias CNH
#define CNH_CAT_A   "A"
#define CNH_CAT_B   "B"
#define CNH_CAT_C   "C"
#define CNH_CAT_AB  "AB"

// Limites de pontos de CNH
#define CNH_MAX_POINTS      20
#define CNH_SUSPEND_POINTS  20

// ------------------------------------------------------------
// Cache de documentos em memória
// ------------------------------------------------------------
enum E_DOCUMENT {
    doc_DBID,
    doc_Type[16],
    doc_Number[20],
    doc_Status[12],
    doc_CNH_Category[6],
    doc_CNH_Points,
    bool:doc_CNH_Suspended
}

#define MAX_DOCS_PER_PLAYER 10

static g_Docs[MAX_PLAYERS][MAX_DOCS_PER_PLAYER][E_DOCUMENT];
static g_DocCount[MAX_PLAYERS] = {0, ...};
static bool:g_DocsLoaded[MAX_PLAYERS] = {false, ...};

// ------------------------------------------------------------
// Gerar número único de documento
// Formato: XXXXXXXXXXX (11 dígitos para CPF/RG)
// ------------------------------------------------------------
stock Docs_GenerateCPF(output[], size = sizeof(output)) {
    // CPF fictício: 3 blocos de 3 + 2 verificadores
    new d[9];
    for(new i = 0; i < 9; i++) d[i] = random(9) + (i == 0 ? 1 : 0);

    // Calcular dígito 1
    new sum = 0;
    for(new i = 0; i < 9; i++) sum += d[i] * (10 - i);
    new r1 = 11 - (sum % 11);
    if(r1 >= 10) r1 = 0;

    // Calcular dígito 2
    sum = 0;
    for(new i = 0; i < 9; i++) sum += d[i] * (11 - i);
    sum += r1 * 2;
    new r2 = 11 - (sum % 11);
    if(r2 >= 10) r2 = 0;

    format(output, size, "%d%d%d.%d%d%d.%d%d%d-%d%d",
        d[0],d[1],d[2], d[3],d[4],d[5], d[6],d[7],d[8], r1, r2);
}

stock Docs_GenerateRG(output[], size = sizeof(output)) {
    // RG: 7 dígitos + dígito verificador
    new d[7];
    for(new i = 0; i < 7; i++) d[i] = random(9) + (i == 0 ? 1 : 0);
    new sum = 0;
    for(new i = 0; i < 7; i++) sum += d[i] * (2 + i);
    new rem = sum % 11;
    new dv = (rem < 2) ? 0 : (11 - rem);

    format(output, size, "%d%d.%d%d%d.%d%d%d-%d",
        d[0],d[1], d[2],d[3],d[4], d[5],d[6],0, dv);
}

stock Docs_GenerateCNH(output[], size = sizeof(output)) {
    format(output, size, "%d%d%d%d%d%d%d%d%d%d%d",
        random(9)+1, random(9), random(9),
        random(9), random(9), random(9),
        random(9), random(9), random(9),
        random(9), random(9));
}

// ------------------------------------------------------------
// Carregar documentos do banco
// ------------------------------------------------------------
stock Docs_Load(playerid) {
    if(!IsValidPlayer(playerid)) return;
    if(PlayerData[playerid][pCharacterID] == INVALID_DB_ID) return;

    g_DocCount[playerid]  = 0;
    g_DocsLoaded[playerid]= false;

    new query[128];
    format(query, sizeof(query),
        "SELECT `id`, `type`, `number`, `status`, `cnh_category`, `cnh_points`, `cnh_suspended` "
        "FROM `character_documents` WHERE `character_id`=%d",
        PlayerData[playerid][pCharacterID]);
    DB_Query(query, "OnDocsLoad", "i", playerid);
}

forward OnDocsLoad(playerid);
public OnDocsLoad(playerid) {
    if(!IsValidPlayer(playerid)) return 0;
    new rows = DB_NumRows();
    g_DocCount[playerid] = 0;

    for(new r = 0; r < rows && g_DocCount[playerid] < MAX_DOCS_PER_PLAYER; r++) {
        new i = g_DocCount[playerid];
        g_Docs[playerid][i][doc_DBID] = DB_GetInt("id", r);
        DB_GetString("type",         g_Docs[playerid][i][doc_Type],          sizeof(g_Docs[][][doc_Type]),         r);
        DB_GetString("number",       g_Docs[playerid][i][doc_Number],        sizeof(g_Docs[][][doc_Number]),       r);
        DB_GetString("status",       g_Docs[playerid][i][doc_Status],        sizeof(g_Docs[][][doc_Status]),       r);
        DB_GetString("cnh_category", g_Docs[playerid][i][doc_CNH_Category],  sizeof(g_Docs[][][doc_CNH_Category]), r);
        g_Docs[playerid][i][doc_CNH_Points]    = DB_GetInt("cnh_points",    r);
        g_Docs[playerid][i][doc_CNH_Suspended] = bool:DB_GetInt("cnh_suspended", r);
        g_DocCount[playerid]++;
    }
    g_DocsLoaded[playerid] = true;
    return 1;
}

// ------------------------------------------------------------
// Emitir documento (criar no banco)
// ------------------------------------------------------------
stock bool:Docs_Issue(playerid, const doc_type[], const issued_by[] = "Prefeitura") {
    if(!IsValidPlayer(playerid)) return false;
    if(PlayerData[playerid][pCharacterID] == INVALID_DB_ID) return false;

    // Verificar se já tem
    if(Docs_Has(playerid, doc_type)) return false; // já possui

    new doc_number[24];
    new cnh_cat[6] = "";

    if(strequal(doc_type, DOC_RG, true)) {
        Docs_GenerateRG(doc_number, sizeof(doc_number));
    }
    else if(strequal(doc_type, DOC_CPF, true)) {
        Docs_GenerateCPF(doc_number, sizeof(doc_number));
    }
    else if(strequal(doc_type, DOC_CNH, true)) {
        Docs_GenerateCNH(doc_number, sizeof(doc_number));
        format(cnh_cat, sizeof(cnh_cat), "B"); // categoria B por padrão
    }
    else {
        format(doc_number, sizeof(doc_number), "%d%d%d%d%d%d",
            random(9)+1, random(9), random(9), random(9), random(9), random(9));
    }

    new escaped_type[32], escaped_num[48], escaped_by[64], escaped_cat[12];
    DB_Escape(doc_type,  escaped_type, sizeof(escaped_type));
    DB_Escape(doc_number,escaped_num,  sizeof(escaped_num));
    DB_Escape(issued_by, escaped_by,   sizeof(escaped_by));
    DB_Escape(cnh_cat,   escaped_cat,  sizeof(escaped_cat));

    new query[512];
    format(query, sizeof(query),
        "INSERT INTO `character_documents` "
        "(`character_id`, `type`, `number`, `status`, `issued_by`, `cnh_category`, `issued_at`) "
        "VALUES (%d, '%s', '%s', 'valid', '%s', '%s', NOW())",
        PlayerData[playerid][pCharacterID],
        escaped_type, escaped_num, escaped_by, escaped_cat);
    DB_Execute(query);

    // Atualizar cache
    if(g_DocCount[playerid] < MAX_DOCS_PER_PLAYER) {
        new i = g_DocCount[playerid];
        g_Docs[playerid][i][doc_DBID] = INVALID_DB_ID;
        SafeStrCopy(g_Docs[playerid][i][doc_Type],   doc_type,   sizeof(g_Docs[][][doc_Type]));
        SafeStrCopy(g_Docs[playerid][i][doc_Number], doc_number, sizeof(g_Docs[][][doc_Number]));
        SafeStrCopy(g_Docs[playerid][i][doc_Status], DOC_STATUS_VALID, sizeof(g_Docs[][][doc_Status]));
        SafeStrCopy(g_Docs[playerid][i][doc_CNH_Category], cnh_cat, sizeof(g_Docs[][][doc_CNH_Category]));
        g_Docs[playerid][i][doc_CNH_Points]    = 0;
        g_Docs[playerid][i][doc_CNH_Suspended] = false;
        g_DocCount[playerid]++;
    }
    return true;
}

// Emitir documentos iniciais para novo personagem
stock Docs_IssueStarter(playerid) {
    Docs_Issue(playerid, DOC_RG,  "Prefeitura");
    Docs_Issue(playerid, DOC_CPF, "Receita Federal RP");

    new logmsg[64];
    format(logmsg, sizeof(logmsg), "Documentos iniciais emitidos para CharID:%d",
        PlayerData[playerid][pCharacterID]);
    Logger_Info("DOCS", logmsg);
}

// ------------------------------------------------------------
// Verificar se tem documento
// ------------------------------------------------------------
stock bool:Docs_Has(playerid, const doc_type[]) {
    if(!IsValidPlayer(playerid)) return false;
    for(new i = 0; i < g_DocCount[playerid]; i++) {
        if(strequal(g_Docs[playerid][i][doc_Type], doc_type, true) &&
           strequal(g_Docs[playerid][i][doc_Status], DOC_STATUS_VALID, true)) {
            return true;
        }
    }
    return false;
}

// Verificar se CNH está válida (não suspensa, não revogada)
stock bool:Docs_HasValidCNH(playerid) {
    if(!IsValidPlayer(playerid)) return false;
    for(new i = 0; i < g_DocCount[playerid]; i++) {
        if(strequal(g_Docs[playerid][i][doc_Type], DOC_CNH, true) &&
           strequal(g_Docs[playerid][i][doc_Status], DOC_STATUS_VALID, true) &&
           !g_Docs[playerid][i][doc_CNH_Suspended]) {
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------
// Adicionar pontos à CNH (infrações)
// ------------------------------------------------------------
stock Docs_AddCNHPoints(playerid, points) {
    if(!IsValidPlayer(playerid)) return;
    for(new i = 0; i < g_DocCount[playerid]; i++) {
        if(!strequal(g_Docs[playerid][i][doc_Type], DOC_CNH, true)) continue;
        g_Docs[playerid][i][doc_CNH_Points] += points;
        if(g_Docs[playerid][i][doc_CNH_Points] >= CNH_SUSPEND_POINTS) {
            g_Docs[playerid][i][doc_CNH_Suspended] = true;
            SendWarning(playerid, "Sua CNH foi suspensa por excesso de pontos!");
        }
        // Salvar no banco
        new query[128];
        format(query, sizeof(query),
            "UPDATE `character_documents` SET `cnh_points`=%d, `cnh_suspended`=%d "
            "WHERE `character_id`=%d AND `type`='cnh'",
            g_Docs[playerid][i][doc_CNH_Points],
            g_Docs[playerid][i][doc_CNH_Suspended] ? 1 : 0,
            PlayerData[playerid][pCharacterID]);
        DB_Execute(query);
        break;
    }
}

// ------------------------------------------------------------
// Mostrar documentos (exibir para o próprio ou para policial)
// ------------------------------------------------------------
stock Docs_Show(viewerid, ownerid) {
    if(!IsValidPlayer(viewerid) || !IsValidPlayer(ownerid)) return;
    if(!g_DocsLoaded[ownerid]) {
        UI_NotifyError(viewerid, "Documentos não carregados. Tente novamente.");
        return;
    }

    new owner_name[MAX_PLAYER_NAME];
    GetPlayerName(ownerid, owner_name, sizeof(owner_name));

    new body[512];
    format(body, sizeof(body),
        CSTR_SYSTEM"=== Documentos de %s ===\n\n",
        owner_name);

    for(new i = 0; i < g_DocCount[ownerid]; i++) {
        new type_display[24];
        new doc_type[] = g_Docs[ownerid][i][doc_Type];

        if(strequal(doc_type, DOC_RG))           format(type_display, sizeof(type_display), "RG");
        else if(strequal(doc_type, DOC_CPF))      format(type_display, sizeof(type_display), "CPF");
        else if(strequal(doc_type, DOC_CNH))      format(type_display, sizeof(type_display), "CNH");
        else if(strequal(doc_type, DOC_GUN_LICENSE)) format(type_display, sizeof(type_display), "Porte de Arma");
        else if(strequal(doc_type, DOC_WORK_LICENSE)) format(type_display, sizeof(type_display), "Lic. Trabalho");
        else SafeStrCopy(type_display, doc_type, sizeof(type_display));

        new status_color[12];
        if(strequal(g_Docs[ownerid][i][doc_Status], DOC_STATUS_VALID)) {
            format(status_color, sizeof(status_color), CSTR_GREEN);
        } else {
            format(status_color, sizeof(status_color), CSTR_ERROR);
        }

        new line[128];
        if(strequal(doc_type, DOC_CNH, true)) {
            format(line, sizeof(line),
                CSTR_WHITE"%s: "CSTR_SYSTEM"%s  "
                "%s[%s]"CSTR_WHITE"  Cat: %s  Pts: %d%s\n",
                type_display,
                g_Docs[ownerid][i][doc_Number],
                status_color,
                g_Docs[ownerid][i][doc_Status],
                g_Docs[ownerid][i][doc_CNH_Category],
                g_Docs[ownerid][i][doc_CNH_Points],
                g_Docs[ownerid][i][doc_CNH_Suspended] ? CSTR_ERROR" [SUSPENSA]" : "");
        } else {
            format(line, sizeof(line),
                CSTR_WHITE"%s: "CSTR_SYSTEM"%s  %s[%s]\n",
                type_display,
                g_Docs[ownerid][i][doc_Number],
                status_color,
                g_Docs[ownerid][i][doc_Status]);
        }
        strcat(body, line, sizeof(body));
    }

    if(g_DocCount[ownerid] == 0) {
        strcat(body, CSTR_ERROR"Nenhum documento encontrado.", sizeof(body));
    }

    UI_ShowDialog(viewerid, DIALOG_NONE + 10, DIALOG_STYLE_MSGBOX,
        "Documentos", body, "Fechar", "");
}

// Comandos
CMD:documentos(playerid, params[]) {
    if(!PlayerData[playerid][pIsLogged]) return 1;
    Docs_Show(playerid, playerid);
    return 1;
}

CMD:docs(playerid, params[]) { return cmd_documentos(playerid, params); }

// Integração com eventos
public OnPlayerLoaded(playerid) {
    Docs_Load(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    g_DocCount[playerid]  = 0;
    g_DocsLoaded[playerid]= false;
    return 1;
}

