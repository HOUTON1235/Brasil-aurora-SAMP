/*
 * ZCMD - Extremely Fast and Lightweight Command Processor
 * Author: Zeex
 * Modified for SA-MP 0.3.7 compatibility
 */

#if defined _INC_ZCMD
    #endinput
#endif
#define _INC_ZCMD

#if !defined ZCMD_YIELD_COUNT
    #define ZCMD_YIELD_COUNT 4
#endif

#if !defined MAX_ZCMD_LENGTH
    #define MAX_ZCMD_LENGTH  (32)
#endif

// ---- Internals -------------------------------------------------------

static stock ZCMD_strncmp(const str1[], const str2[], n) {
    for(new i = 0; i < n && str1[i] != '\0'; i++) {
        if(str1[i] != str2[i]) return 1;
    }
    return 0;
}

static stock ZCMD_OnPlayerCommandText(playerid, cmdtext[]) {
    new
        idx       = 1,
        cmd[MAX_ZCMD_LENGTH],
        i         = 0;

    // Extract command name
    while(cmdtext[idx] > ' ')
        cmd[i++] = cmdtext[idx++];
    cmd[i] = '\0';

    // Skip spaces
    while(cmdtext[idx] == ' ')
        idx++;

    // Build params pointer (rest of the string)
    new params[144];
    strmid(params, cmdtext, idx, strlen(cmdtext), sizeof(params));

    // Call cmd_<name>(playerid, params[])
    new func_name[MAX_ZCMD_LENGTH + 4] = "cmd_";
    strcat(func_name, cmd, sizeof(func_name));

    // strlwr (lowercase)
    for(new j = 4; func_name[j]; j++) {
        if(func_name[j] >= 'A' && func_name[j] <= 'Z')
            func_name[j] += 32;
    }

    if(CallLocalFunction(func_name, "is", playerid, params))
        return 1;

    return 0;
}

// ---- Public hook -------------------------------------------------------

public OnPlayerCommandText(playerid, cmdtext[]) {
    if(ZCMD_OnPlayerCommandText(playerid, cmdtext))
        return 1;

    #if defined ZCMD_OnPlayerCommandText
        return ZCMD_OnPlayerCommandText(playerid, cmdtext);
    #else
        return 0;
    #endif
}

// ---- Macro -------------------------------------------------------------

#define CMD:%1(%2) \
    forward cmd_%1(%2); \
    public  cmd_%1(%2)

// Alias
#define COMMAND CMD

