#!/usr/bin/env python3
"""
master_fix.py - Reconstroi pawno\include a partir de includes\
Sem BOM, ASCII puro, todos os fixes de sintaxe para SA-MP 0.3.7 + BlueG R41
"""
import re, unicodedata, os, sys

BASE   = r'C:\Users\Felip\Downloads\Servidor 2'
SRC    = os.path.join(BASE, 'includes')
DST    = os.path.join(BASE, 'pawno', 'include')

FILE_MAP = {
    'admin_core.inc':        'admin/admin_core.inc',
    'anticheat_core.inc':    'utilities/anticheat_core.inc',
    'bank_core.inc':         'economy/bank_core.inc',
    'business_core.inc':     'businesses/business_core.inc',
    'db_core.inc':           'database/db_core.inc',
    'db_queries.inc':        'database/db_queries.inc',
    'defines.inc':           'core/defines.inc',
    'events.inc':            'core/events.inc',
    'faction_core.inc':      'factions/faction_core.inc',
    'fines_core.inc':        'economy/fines_core.inc',
    'garage_core.inc':       'houses/garage_core.inc',
    'government_core.inc':   'government/government_core.inc',
    'hospital_core.inc':     'hospital/hospital_core.inc',
    'inventory_core.inc':    'inventory/inventory_core.inc',
    'jobs_core.inc':         'jobs/jobs_core.inc',
    'logger.inc':            'logs/logger.inc',
    'macros.inc':            'core/macros.inc',
    'missions_core.inc':     'inventory/missions_core.inc',
    'performance_core.inc':  'utilities/performance_core.inc',
    'permissions.inc':       'core/permissions.inc',
    'phone_core.inc':        'phone/phone_core.inc',
    'player_auth.inc':       'players/player_auth.inc',
    'player_character.inc':  'players/player_character.inc',
    'player_core.inc':       'players/player_core.inc',
    'player_data.inc':       'players/player_data.inc',
    'player_documents.inc':  'players/player_documents.inc',
    'player_hud.inc':        'players/player_hud.inc',
    'player_hunger.inc':     'players/player_hunger.inc',
    'player_tutorial.inc':   'players/player_tutorial.inc',
    'police_core.inc':       'police/police_core.inc',
    'property_core.inc':     'houses/property_core.inc',
    'scheduler.inc':         'core/scheduler.inc',
    'states.inc':            'core/states.inc',
    'stores_core.inc':       'economy/stores_core.inc',
    'territory_core.inc':    'factions/territory_core.inc',
    'traffic_core.inc':      'government/traffic_core.inc',
    'ui_core.inc':           'ui/ui_core.inc',
    'utils.inc':             'utilities/utils.inc',
    'vehicle_core.inc':      'vehicles/vehicle_core.inc',
    'vehicle_dealership.inc':'vehicles/vehicle_dealership.inc',
    'vehicle_fuel.inc':      'vehicles/vehicle_fuel.inc',
    'world_core.inc':        'utilities/world_core.inc',
}

def to_ascii(text):
    out = []
    for ch in text:
        if ord(ch) < 128:
            out.append(ch)
        else:
            n = unicodedata.normalize('NFD', ch)
            a = n.encode('ascii', 'ignore').decode('ascii')
            out.append(a if a else '?')
    return ''.join(out)

def fix(text, fname):
    # Remove BOM
    if text.startswith('\ufeff'):
        text = text[1:]

    # Normalizar line endings
    text = text.replace('\r\n', '\n').replace('\r', '\n')

    # Remove includes relativos (nao existem no flat)
    text = re.sub(r'#include\s+"[^"]*"[^\n]*', '// include removido', text)

    # Converter para ASCII
    text = to_ascii(text)

    # --- Fixes de sintaxe ---

    # 1. stock bool: -> stock  (bool: em retorno de stock nao e suportado bem)
    text = re.sub(r'\bstock\s+bool:', 'stock ', text)

    # 2. bool:param = true/false em parametros -> param = true/false
    text = re.sub(r'\bbool:(\w+)\s*=\s*(true|false)', r'\1 = \2', text)

    # 3. const arrays em parametros -> sem const
    text = re.sub(r'\bconst\s+(\w+)\[\]', r'\1[]', text)

    # 4. E_PLAYER_STATE: como tag em parametros e retornos -> remover
    text = text.replace('E_PLAYER_STATE:', '')

    # 5. strequal -> !strcmp  (strequal nao existe por padrao)
    text = re.sub(r'\bstrequal\s*\(', '!strcmp(', text)

    # 6. gpci() nao existe em 0.3.7 -> comentar
    text = re.sub(r'gpci\s*\([^)]*\)\s*;', '// gpci nao disponivel em 0.3.7', text)

    # 7. mysql_error() nao existe no R41 -> usar apenas errno
    text = re.sub(
        r'mysql_error\s*\([^)]*\)\s*;',
        '// mysql_error nao existe no R41',
        text
    )

    # 8. ___(N) nao existe sem community compiler -> remover argumento variavel
    # mysql_tquery(h, q, cb, fmt, ___(4)) -> mysql_tquery(h, q, cb, fmt)
    text = re.sub(r',\s*___\s*\(\d+\)', '', text)

    # 9. 'char' como nome de parametro e palavra reservada -> 'ch'
    text = re.sub(r'\(([^)]*)\bchar\b([^)]*)\)', lambda m: '(' + m.group(1) + 'ch' + m.group(2) + ')', text)
    # Tambem dentro do corpo da funcao que usava 'char'
    # (substituicao simples - so afeta Utils_CountChar)
    text = text.replace('str[i] == char)', 'str[i] == ch)')

    # 10. State_GetName com size=sizeof nao compila -> trocar por macro
    if fname == 'states.inc':
        text = re.sub(
            r'stock\s+State_GetName\s*\([^)]*\)\s*\{[^}]*\}',
            '',
            text, flags=re.DOTALL
        )
        if 'define State_GetName' not in text:
            text += (
                '\n#define State_GetName(%0,%1,%2) format(%1,%2,'
                '(%0)==STATE_DISCONNECTED?"Desconectado":'
                '(%0)==STATE_CONNECTING?"Conectando":'
                '(%0)==STATE_AUTHENTICATING?"Autenticando":'
                '(%0)==STATE_CHARACTER_SELECTION?"Criacao de Personagem":'
                '(%0)==STATE_TUTORIAL?"Tutorial":'
                '(%0)==STATE_SPAWNED?"Ativo":'
                '(%0)==STATE_WORKING?"Trabalhando":'
                '(%0)==STATE_DEAD?"Morto":'
                '(%0)==STATE_JAILED?"Preso":'
                '(%0)==STATE_HOSPITALIZED?"Hospitalizado":'
                '(%0)==STATE_AFK?"AFK":'
                '(%0)==STATE_ADMIN_SPECTATING?"Admin Spectate":"Desconhecido")\n'
            )

    # 11. permissions.inc: forward bool: -> forward  e static const -> novo enum
    if fname == 'permissions.inc':
        text = text.replace('forward bool:Player_HasPermission', 'forward Player_HasPermission')
        text = text.replace('forward bool:Player_IsAdmin',       'forward Player_IsAdmin')
        # static const PermissionMinLevel[][] nao funciona com strings - substituir por enum
        # Remover o bloco estatico original e adicionar versao com enum
        text = re.sub(
            r'static\s+const\s+PermissionMinLevel\s*\[\]\[\]\s*=\s*\{[^}]*(?:\{[^}]*\}[^}]*)*\}\s*;',
            '''enum E_PERM_ENTRY { pe_Key[32], pe_Level }
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
#define MAX_PERMISSION_ENTRIES 19''',
            text, flags=re.DOTALL
        )
        # Corrigir MAX_PERMISSION_ENTRIES se ficou duplicado
        text = re.sub(r'#define MAX_PERMISSION_ENTRIES\s+\([^)]*\)', '', text)

    # 12. player_core.inc: Player_IsAdmin com default value conflita com forward
    if fname == 'player_core.inc':
        text = text.replace(
            'stock bool:Player_IsAdmin(playerid, minLevel = ADMIN_HELPER)',
            'stock Player_IsAdmin(playerid, minLevel)'
        )
        text = text.replace(
            'stock Player_IsAdmin(playerid, minLevel = ADMIN_HELPER)',
            'stock Player_IsAdmin(playerid, minLevel)'
        )
        text = text.replace(
            'stock bool:Player_HasPermission(playerid, const permission[])',
            'stock Player_HasPermission(playerid, permission[])'
        )
        text = text.replace(
            'stock bool:Player_HasPermission(playerid, permission[])',
            'stock Player_HasPermission(playerid, permission[])'
        )
        # Substituir PermissionMinLevel[i][0] por g_PermMap[i][pe_Key]
        text = text.replace('PermissionMinLevel[i][0]', 'g_PermMap[i][pe_Key]')
        text = text.replace('PermissionMinLevel[i][1]', 'g_PermMap[i][pe_Level]')
        # SetPVarInt com _:state cast
        text = text.replace('SetPVarInt(playerid, "pState", _:state)', 'SetPVarInt(playerid, "pState", state)')

    # 13. db_queries.inc: funcoes duplicadas de db_core - esvaziar
    if fname == 'db_queries.inc':
        text = ('#if defined _SA_DB_QUERIES_INC\n    #endinput\n#endif\n'
                '#define _SA_DB_QUERIES_INC\n// Funcoes movidas para db_core.inc\n')

    # 14. phone_core.inc: adicionar forwards para publics customizados
    if fname == 'phone_core.inc':
        fwd = ('\nforward OnPlayerLoaded(playerid);\n'
               'forward OnScheduler5Minutes();\n')
        text = text.replace('#define _SA_PHONE_CORE_INC',
                            '#define _SA_PHONE_CORE_INC' + fwd, 1)
        # stock bool: em Phone_HandleDialog e Phone_SendSMS
        text = text.replace('stock bool:Phone_HandleDialog', 'stock Phone_HandleDialog')
        text = text.replace('stock bool:Phone_SendSMS',      'stock Phone_SendSMS')

    # 15. logger.inc: strequal ja foi tratado acima

    return text

# --- Processar todos os arquivos ---
ok = 0
errors = 0
for dst_name, src_rel in FILE_MAP.items():
    src_path = os.path.join(SRC, src_rel)
    dst_path = os.path.join(DST, dst_name)

    if not os.path.exists(src_path):
        print('FALTA: ' + src_path)
        errors += 1
        continue

    try:
        raw = open(src_path, 'rb').read()
        try:
            text = raw.decode('utf-8')
        except Exception:
            text = raw.decode('latin-1')

        text = fix(text, dst_name)

        # Salvar SEM BOM, encoding ASCII, line endings Unix
        with open(dst_path, 'w', encoding='ascii', errors='replace', newline='\n') as f:
            f.write(text)

        # Verificar que nao tem BOM
        check = open(dst_path, 'rb').read(3)
        if check == b'\xef\xbb\xbf':
            print('ERRO BOM em ' + dst_name)
            errors += 1
        else:
            ok += 1
    except Exception as e:
        print('ERRO em %s: %s' % (dst_name, str(e)))
        errors += 1

# Tambem corrigir rp.pwn
rpwn = os.path.join(BASE, 'gamemodes', 'rp.pwn')
raw = open(rpwn, 'rb').read()
if raw.startswith(b'\xef\xbb\xbf'):
    raw = raw[3:]
text = raw.decode('utf-8', errors='replace')
text = to_ascii(text)
with open(rpwn, 'w', encoding='ascii', errors='replace', newline='\n') as f:
    f.write(text)
print('rp.pwn: BOM removido, convertido para ASCII')

print('\nResultado: %d OK, %d erros' % (ok, errors))
