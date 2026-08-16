#!/usr/bin/env python3
"""
Reconstroi todos os .inc do projeto a partir dos originais em includes\
com as seguintes transformacoes:
1. Remove BOM
2. Converte para ASCII (acentos viram base, ex: 'a' 'c')
3. Remove #include relativos
4. Remove bool: de retorno de stock
5. Remove const de parametros de arrays
6. Remove E_PLAYER_STATE: e outras tags de enum invalidas
7. Normaliza linhas - colapsa linhas em branco excessivas MAS nao no meio de strings
"""
import re, unicodedata, os, sys

SRC_BASE = r'C:\Users\Felip\Downloads\Servidor 2\includes'
DST_DIR  = r'C:\Users\Felip\Downloads\Servidor 2\pawno\include'

# Mapeamento: nome_arquivo -> caminho_relativo_em_includes
FILE_MAP = {
    'admin_core.inc':       'admin/admin_core.inc',
    'anticheat_core.inc':   'utilities/anticheat_core.inc',
    'bank_core.inc':        'economy/bank_core.inc',
    'business_core.inc':    'businesses/business_core.inc',
    'db_core.inc':          'database/db_core.inc',
    'db_queries.inc':       'database/db_queries.inc',
    'defines.inc':          'core/defines.inc',
    'events.inc':           'core/events.inc',
    'faction_core.inc':     'factions/faction_core.inc',
    'fines_core.inc':       'economy/fines_core.inc',
    'garage_core.inc':      'houses/garage_core.inc',
    'government_core.inc':  'government/government_core.inc',
    'hospital_core.inc':    'hospital/hospital_core.inc',
    'inventory_core.inc':   'inventory/inventory_core.inc',
    'jobs_core.inc':        'jobs/jobs_core.inc',
    'logger.inc':           'logs/logger.inc',
    'macros.inc':           'core/macros.inc',
    'missions_core.inc':    'inventory/missions_core.inc',
    'performance_core.inc': 'utilities/performance_core.inc',
    'permissions.inc':      'core/permissions.inc',
    'phone_core.inc':       'phone/phone_core.inc',
    'player_auth.inc':      'players/player_auth.inc',
    'player_character.inc': 'players/player_character.inc',
    'player_core.inc':      'players/player_core.inc',
    'player_data.inc':      'players/player_data.inc',
    'player_documents.inc': 'players/player_documents.inc',
    'player_hud.inc':       'players/player_hud.inc',
    'player_hunger.inc':    'players/player_hunger.inc',
    'player_tutorial.inc':  'players/player_tutorial.inc',
    'police_core.inc':      'police/police_core.inc',
    'property_core.inc':    'houses/property_core.inc',
    'scheduler.inc':        'core/scheduler.inc',
    'states.inc':           'core/states.inc',
    'stores_core.inc':      'economy/stores_core.inc',
    'territory_core.inc':   'factions/territory_core.inc',
    'traffic_core.inc':     'government/traffic_core.inc',
    'ui_core.inc':          'ui/ui_core.inc',
    'utils.inc':            'utilities/utils.inc',
    'vehicle_core.inc':     'vehicles/vehicle_core.inc',
    'vehicle_dealership.inc':'vehicles/vehicle_dealership.inc',
    'vehicle_fuel.inc':     'vehicles/vehicle_fuel.inc',
    'world_core.inc':       'utilities/world_core.inc',
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

def fix_text(text, fname):
    # 1. Remove BOM
    if text.startswith('\ufeff'):
        text = text[1:]

    # 2. Normalizar line endings para \n
    text = text.replace('\r\n', '\n').replace('\r', '\n')

    # 3. Remove includes relativos
    text = re.sub(r'#include\s+"[^"]*"[^\n]*', '// include removido', text)

    # 4. Converte para ASCII
    text = to_ascii(text)

    # 5. Remove bool: de returns de stock
    text = re.sub(r'\bstock\s+bool:', 'stock ', text)

    # 6. Remove const de parametros de arrays
    text = re.sub(r'\bconst\s+(\w+)\[\]', r'\1[]', text)

    # 7. bool:param = true/false -> param = true/false
    text = re.sub(r'\bbool:(\w+)\s*=\s*(true|false)', r'\1 = \2', text)

    # 8. Remove tags de enum invalidas de parametros
    text = text.replace('E_PLAYER_STATE:', '')

    # 9. Para phone_core.inc - adicionar forwards
    if fname == 'phone_core.inc':
        fwd = '\nforward OnPlayerLoaded(playerid);\nforward OnPlayerDisconnect(playerid, reason);\nforward OnScheduler5Minutes();\n'
        text = text.replace('#define _SA_PHONE_CORE_INC', '#define _SA_PHONE_CORE_INC' + fwd, 1)

    # 10. Para states.inc - substituir State_GetName por macro
    if fname == 'states.inc':
        # Remove a funcao stock State_GetName se existir
        text = re.sub(
            r'stock\s+State_GetName\s*\([^)]*\)\s*\{[^}]*\}',
            '', text, flags=re.DOTALL
        )
        # Adicionar macro no final
        macro = '\n#define State_GetName(%0,%1,%2) format(%1,%2,(%0)==STATE_DISCONNECTED?"Desconectado":(%0)==STATE_CONNECTING?"Conectando":(%0)==STATE_AUTHENTICATING?"Autenticando":(%0)==STATE_CHARACTER_SELECTION?"Criacao de Personagem":(%0)==STATE_TUTORIAL?"Tutorial":(%0)==STATE_SPAWNED?"Ativo":(%0)==STATE_WORKING?"Trabalhando":(%0)==STATE_DEAD?"Morto":(%0)==STATE_JAILED?"Preso":(%0)==STATE_HOSPITALIZED?"Hospitalizado":(%0)==STATE_AFK?"AFK":(%0)==STATE_ADMIN_SPECTATING?"Admin Spectate":"Desconhecido")\n'
        if 'State_GetName' not in text:
            text += macro

    return text

processed = 0
errors = 0

for dst_name, src_rel in FILE_MAP.items():
    src_path = os.path.join(SRC_BASE, src_rel)
    dst_path = os.path.join(DST_DIR, dst_name)

    if not os.path.exists(src_path):
        print('FALTA: %s' % src_path)
        errors += 1
        continue

    try:
        raw = open(src_path, 'rb').read()
        try:
            text = raw.decode('utf-8')
        except Exception:
            text = raw.decode('latin-1')

        text = fix_text(text, dst_name)

        with open(dst_path, 'w', encoding='ascii', errors='replace', newline='\n') as f:
            f.write(text)
        processed += 1
    except Exception as e:
        print('ERRO em %s: %s' % (dst_name, e))
        errors += 1

print('Processados: %d, Erros: %d' % (processed, errors))
