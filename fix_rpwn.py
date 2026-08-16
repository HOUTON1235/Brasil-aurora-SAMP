#!/usr/bin/env python3
import re, unicodedata, os

BASE = r'C:\Users\Felip\Downloads\Servidor 2'

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

# Corrigir rp.pwn
rpwn = os.path.join(BASE, 'gamemodes', 'rp.pwn')
raw = open(rpwn, 'rb').read()
if raw.startswith(b'\xef\xbb\xbf'):
    raw = raw[3:]
text = raw.decode('utf-8', errors='replace')
text = text.replace('\r\n', '\n').replace('\r', '\n')
text = to_ascii(text)
# Colapsar multiplas linhas em branco consecutivas em uma so
text = re.sub(r'\n{3,}', '\n\n', text)
# Remover espacos no final de linhas
text = re.sub(r'[ \t]+\n', '\n', text)
with open(rpwn, 'wb') as f:
    f.write(text.encode('ascii', errors='replace'))
print('rp.pwn: OK (%d linhas)' % text.count('\n'))

# Contar linhas em branco duplas que sobraram nos .inc
inc_dir = os.path.join(BASE, 'pawno', 'include')
skip = {'a_samp.inc','a_mysql.inc','a_npc.inc','a_objects.inc','a_players.inc',
        'a_sampdb.inc','a_vehicles.inc','float.inc','sscanf2.inc','streamer.inc',
        'zcmd.inc','core.inc','string.inc','file.inc','time.inc','datagram.inc',
        'utils.inc','a_http.inc','a_actor.inc'}

fixed = 0
for fname in os.listdir(inc_dir):
    if not fname.endswith('.inc') or fname in skip:
        continue
    fpath = os.path.join(inc_dir, fname)
    raw2 = open(fpath, 'rb').read()
    # Sem BOM
    if raw2.startswith(b'\xef\xbb\xbf'):
        raw2 = raw2[3:]
        print('BOM removido: ' + fname)
    text2 = raw2.decode('ascii', errors='replace')
    text2 = text2.replace('\r\n', '\n').replace('\r', '\n')
    orig = text2
    text2 = re.sub(r'\n{3,}', '\n\n', text2)
    text2 = re.sub(r'[ \t]+\n', '\n', text2)
    if text2 != orig:
        with open(fpath, 'wb') as f:
            f.write(text2.encode('ascii', errors='replace'))
        fixed += 1

print('Includes normalizados: %d' % fixed)
