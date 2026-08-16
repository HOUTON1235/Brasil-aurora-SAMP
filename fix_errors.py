#!/usr/bin/env python3
import re, os

BASE = r'C:\Users\Felip\Downloads\Servidor 2\pawno\include'

def read(f):
    return open(os.path.join(BASE, f), 'r', encoding='ascii', errors='replace').read()

def write(f, content):
    open(os.path.join(BASE, f), 'w', encoding='ascii', errors='replace', newline='\n').write(content)

# ── 1. states.inc ─────────────────────────────────────────────
t = read('states.inc')
# Substituir a funcao com size=sizeof por versao sem default param
t = re.sub(
    r'stock\s+State_GetName\s*\(\s*state\s*,\s*output\s*\[\s*\]\s*,\s*size\s*=\s*sizeof\s*\(\s*output\s*\)\s*\)\s*\{.*?\}',
    '',
    t, flags=re.DOTALL
)
# Adicionar macro se nao existir
if 'define State_GetName' not in t:
    t += '\n#define State_GetName(%0,%1,%2) format(%1,%2,(%0)==STATE_DISCONNECTED?"Desconectado":(%0)==STATE_CONNECTING?"Conectando":(%0)==STATE_AUTHENTICATING?"Autenticando":(%0)==STATE_CHARACTER_SELECTION?"Criacao de Personagem":(%0)==STATE_TUTORIAL?"Tutorial":(%0)==STATE_SPAWNED?"Ativo":(%0)==STATE_WORKING?"Trabalhando":(%0)==STATE_DEAD?"Morto":(%0)==STATE_JAILED?"Preso":(%0)==STATE_HOSPITALIZED?"Hospitalizado":(%0)==STATE_AFK?"AFK":(%0)==STATE_ADMIN_SPECTATING?"Admin Spectate":"Desconhecido")\n'
write('states.inc', t)
print('states.inc OK')

# ── 2. db_core.inc ────────────────────────────────────────────
t = read('db_core.inc')
# strequal -> !strcmp
t = t.replace('strequal(format, "")', '!strcmp(format, "", true)')
t = t.replace('strequal(', '!strcmp(')
# ___(4) -> argumentos variadicos nao suportados - usar format fixo
# A linha: mysql_tquery(g_DBHandle, query, callback, format, ___(4));
# Substituir por versao sem variadico - passar sem argumentos extras
# (o mysql_tquery do BlueG R41 aceita format string diretamente)
t = t.replace('mysql_tquery(g_DBHandle, query, callback, format, ___(4));',
              'mysql_tquery(g_DBHandle, query, callback, format);')
write('db_core.inc', t)
print('db_core.inc OK')

# ── 3. utils.inc ──────────────────────────────────────────────
t = read('utils.inc')
# 'char' é palavra reservada - renomear para 'ch'
t = t.replace('Utils_CountChar(str[], char)', 'Utils_CountChar(str[], ch)')
t = t.replace('str[i] == char)', 'str[i] == ch)')
write('utils.inc', t)
print('utils.inc OK')

# ── 4. player_core.inc ────────────────────────────────────────
t = read('player_core.inc')
# _:state cast causa problema - remover
t = t.replace('SetPVarInt(playerid, "pState", _:state);',
              'SetPVarInt(playerid, "pState", state);')
# Remover tag E_PLAYER_STATE que sobrou no enum do player
t = t.replace('E_PLAYER_STATE:', '')
write('player_core.inc', t)
print('player_core.inc OK')

# ── 5. logger.inc ─────────────────────────────────────────────
t = read('logger.inc')
t = t.replace('strequal(', '!strcmp(')
# Corrigir chamadas de !strcmp que ficaram com argumentos errados
# !strcmp(x, y) -> !strcmp(x, y, true)  (só se tiver 2 args)
# Mais seguro: substituir !strcmp(a, b) por !strcmp(a, b, true)
t = re.sub(r'!strcmp\(([^,)]+),\s*([^,)]+)\)', r'!strcmp(\1, \2, true)', t)
write('logger.inc', t)
print('logger.inc OK')

# ── 6. permissions.inc ────────────────────────────────────────
# error 025: function heading differs from prototype
t = read('permissions.inc')
# Remover forwards duplicados ou incorretos
t = re.sub(r'forward\s+PermissionMinLevel\s*\([^)]*\)\s*;', '', t)
write('permissions.inc', t)
print('permissions.inc OK')

print('Todos os fixes aplicados.')
