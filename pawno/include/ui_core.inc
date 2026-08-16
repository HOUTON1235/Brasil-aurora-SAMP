// ============================================================
//  Santa Aurora Roleplay ? UI Core
//  Modulo : ui/ui_core.inc
//  Objetivo: componentes de interface reutilizaveis
// ============================================================

#if defined _SA_UI_CORE_INC
    #endinput
#endif
#define _SA_UI_CORE_INC

// include removido
// include removido

// ------------------------------------------------------------
// Wrapper de dialog com validacao
// ------------------------------------------------------------
stock UI_ShowDialog(playerid, dialogid, style, title[], body[],
                   btn1[], btn2[] = "") {
    if(!IsValidPlayer(playerid)) return;
    ShowPlayerDialog(playerid, dialogid, style, title, body, btn1, btn2);
}

// ------------------------------------------------------------
// Fechar dialog atual do jogador
// ------------------------------------------------------------
stock UI_CloseDialog(playerid) {
    if(!IsValidPlayer(playerid)) return;
    ShowPlayerDialog(playerid, DIALOG_NONE, DIALOG_STYLE_MSGBOX, "", "", "", "");
}

// ------------------------------------------------------------
// Notificacoes
// ------------------------------------------------------------
stock UI_Notify(playerid, message[], color = COLOR_INFO) {
    SendColorMessage(playerid, color, message);
}

stock UI_NotifyError(playerid, message[]) {
    new str[144];
    format(str, sizeof(str), CSTR_ERROR"[!] "CSTR_WHITE"%s", message);
    SendClientMessage(playerid, COLOR_ERROR, str);
}

stock UI_NotifySuccess(playerid, message[]) {
    new str[144];
    format(str, sizeof(str), CSTR_SUCCESS"[?] "CSTR_WHITE"%s", message);
    SendClientMessage(playerid, COLOR_SUCCESS, str);
}

stock UI_NotifyWarning(playerid, message[]) {
    new str[144];
    format(str, sizeof(str), CSTR_WARNING"[!] "CSTR_WHITE"%s", message);
    SendClientMessage(playerid, COLOR_WARNING, str);
}

stock UI_NotifyInfo(playerid, message[]) {
    new str[144];
    format(str, sizeof(str), CSTR_SYSTEM"[i] "CSTR_WHITE"%s", message);
    SendClientMessage(playerid, COLOR_INFO, str);
}

stock UI_NotifyMoney(playerid, amount, bool:received) {
    new str[64];
    new formatted[32];
    FormatMoneyFull(amount, formatted, sizeof(formatted));
    if(received) {
        format(str, sizeof(str), CSTR_MONEY"[+] Recebido: %s", formatted);
        SendClientMessage(playerid, COLOR_MONEY, str);
    } else {
        format(str, sizeof(str), CSTR_ERROR"[-] Pago: %s", formatted);
        SendClientMessage(playerid, COLOR_RED, str);
    }
}

// Separador visual (linha horizontal)
stock UI_Separator(playerid) {
    SendClientMessage(playerid, COLOR_DARK_GREY,
        "_______________________________________________");
}

// Cabecalho de secao
stock UI_Header(playerid, title[]) {
    new str[64];
    format(str, sizeof(str), CSTR_SYSTEM"=== %s ===", title);
    SendClientMessage(playerid, COLOR_SYSTEM, str);
}

// ------------------------------------------------------------
// Dialog de confirmacao generico (Sim/Nao)
// ------------------------------------------------------------
stock UI_ShowConfirm(playerid, title[], message[]) {
    UI_ShowDialog(playerid,
        DIALOG_CONFIRM_YES_NO,
        DIALOG_STYLE_MSGBOX,
        title,
        message,
        "Sim",
        "Nao");
}

// ------------------------------------------------------------
// Dialog de mensagem simples (apenas OK)
// ------------------------------------------------------------
stock UI_ShowMessage(playerid, title[], message[]) {
    UI_ShowDialog(playerid,
        DIALOG_NONE + 1, // reusar slot generico
        DIALOG_STYLE_MSGBOX,
        title,
        message,
        "OK",
        "");
}

// ------------------------------------------------------------
// Construtor de lista para dialogs DIALOG_STYLE_LIST
// ------------------------------------------------------------
#define MAX_LISTBOX_ITEMS   20
#define MAX_LISTBOX_ITEM_LEN 64

static g_ListBuffer[MAX_LISTBOX_ITEMS * MAX_LISTBOX_ITEM_LEN];
static g_ListItemCount = 0;

stock UI_ListReset() {
    g_ListBuffer[0] = '\0';
    g_ListItemCount = 0;
}

stock UI_ListAdd(item[]) {
    if(g_ListItemCount >= MAX_LISTBOX_ITEMS) return;
    strcat(g_ListBuffer, item, sizeof(g_ListBuffer));
    strcat(g_ListBuffer, "\n", sizeof(g_ListBuffer));
    g_ListItemCount++;
}

stock UI_ListShow(playerid, dialogid, title[], btn1[], btn2[] = "Fechar") {
    UI_ShowDialog(playerid, dialogid, DIALOG_STYLE_LIST, title, g_ListBuffer, btn1, btn2);
}

// ------------------------------------------------------------
// GameText formatado
// ------------------------------------------------------------
stock UI_GameText(playerid, message[], time_ms = 3000, style = 3) {
    if(!IsValidPlayer(playerid)) return;
    GameTextForPlayer(playerid, message, time_ms, style);
}

// ------------------------------------------------------------
// Barra de progresso simples via textdraws (ASCII)
// ------------------------------------------------------------
stock UI_ProgressBar(output[], size, value, max_val, bar_width = 20) {
    new filled = (bar_width * value) / max_val;
    new str[64];
    str[0] = '[';
    for(new i = 1; i <= bar_width; i++) {
        str[i] = (i <= filled) ? '|' : ' ';
    }
    str[bar_width + 1] = ']';
    str[bar_width + 2] = '\0';

    new pct[8];
    format(pct, sizeof(pct), " %d%%", (100 * value) / max_val);
    strcat(str, pct, sizeof(str));
    format(output, size, "%s", str);
}

// ------------------------------------------------------------
// Titulo de boas-vindas ao logar
// ------------------------------------------------------------
stock UI_ShowWelcome(playerid, charname[]) {
    new msg[128];
    format(msg, sizeof(msg),
        "~w~Bem-vindo, ~g~%s~w~!~n~~w~Santa Aurora RP", charname);
    GameTextForPlayer(playerid, msg, 5000, 3);

    new chat_msg[128];
    format(chat_msg, sizeof(chat_msg),
        CSTR_SYSTEM"[SISTEMA] "CSTR_WHITE"Bem-vindo de volta, "CSTR_GREEN"%s"CSTR_WHITE"!",
        charname);
    SendClientMessage(playerid, COLOR_SYSTEM, chat_msg);
    SendClientMessage(playerid, COLOR_INFO,
        CSTR_GREY"Use /ajuda para ver os comandos disponiveis.");
}

