#!/usr/bin/env python3
import re, os

BASE = r'C:\Users\Felip\Downloads\Servidor 2\pawno\include'

def read(f):
    return open(os.path.join(BASE, f), 'r', encoding='ascii', errors='replace').read()

def write(f, content):
    open(os.path.join(BASE, f), 'w', encoding='ascii', errors='replace', newline='\n').write(content)

# ── 1. states.inc: remover } solto apos comentario ───────────
t = read('states.inc')
# Remover o bloco "// Retorna nome legivel do estado\n\n}" que ficou
t = t.replace('// Retorna nome legivel do estado\n\n}\n', '// Retorna nome legivel do estado\n')
t = t.replace('// Retorna nome legivel do estado\n\n}\n\n', '// Retorna nome legivel do estado\n\n')
write('states.inc', t)
print('states.inc OK')

# ── 2. permissions.inc: remover bool: dos forwards ────────────
t = read('permissions.inc')
t = t.replace('forward bool:Player_HasPermission', 'forward Player_HasPermission')
t = t.replace('forward bool:Player_IsAdmin', 'forward Player_IsAdmin')
# Tambem na implementacao
t = t.replace('stock bool:Player_HasPermission', 'stock Player_HasPermission')
t = t.replace('stock bool:Player_IsAdmin', 'stock Player_IsAdmin')
write('permissions.inc', t)
print('permissions.inc OK')

# ── 3. player_core.inc: remover gpci (nao existe em 0.3.7) ───
t = read('player_core.inc')
# Remover linha com gpci
t = re.sub(r'\n\s*gpci\s*\([^)]*\)\s*;[^\n]*', '', t)
# Remover declaracao de g_PlayerSerial se nao for mais usada
# (manter por enquanto - pode ser usada em outro lugar)
write('player_core.inc', t)
print('player_core.inc OK')

# ── 4. db_core.inc: mysql_error() no R41 usa formato diferente
t = read('db_core.inc')
# No R41, mysql_error nao existe - usar mysql_errno apenas
# Substituir bloco que usa mysql_error por versao simplificada
old = '''    new errcode = mysql_errno(g_DBHandle);
    if(errcode != 0) {
        new errmsg[256];
        mysql_error(g_DBHandle, errmsg, sizeof(errmsg));
        new logmsg[256];
        format(logmsg, sizeof(logmsg), "Erro MySQL %d: %s", errcode, errmsg);
        Logger_Error("DATABASE", logmsg);'''
new = '''    new errcode = mysql_errno(g_DBHandle);
    if(errcode != 0) {
        new logmsg[64];
        format(logmsg, sizeof(logmsg), "Erro MySQL: %d", errcode);
        Logger_Error("DATABASE", logmsg);'''
t = t.replace(old, new)
# Fallback caso o texto seja ligeiramente diferente
if 'mysql_error(' in t:
    t = re.sub(r'mysql_error\s*\([^)]*\)\s*;', '// mysql_error removido (nao existe no R41)', t)
write('db_core.inc', t)
print('db_core.inc OK')

print('Fix2 completo.')
