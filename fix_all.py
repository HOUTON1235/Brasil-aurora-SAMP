import os, re, unicodedata

inc_dir = r'C:\Users\Felip\Downloads\Servidor 2\pawno\include'

skip = {
    'a_samp.inc','a_mysql.inc','a_npc.inc','a_objects.inc','a_players.inc',
    'a_sampdb.inc','a_vehicles.inc','float.inc','sscanf2.inc','streamer.inc',
    'zcmd.inc','core.inc','string.inc','file.inc','time.inc','datagram.inc',
    'utils.inc','a_http.inc'
}

def fix(text):
    if text.startswith('\ufeff'):
        text = text[1:]
    text = re.sub(r'#include\s+"[^"]*"[^\r\n]*', '// include removido', text)
    result = []
    for ch in text:
        if ord(ch) < 128:
            result.append(ch)
        else:
            n = unicodedata.normalize('NFD', ch)
            a = n.encode('ascii', 'ignore').decode('ascii')
            result.append(a if a else '?')
    return ''.join(result)

fixed = []
for fname in os.listdir(inc_dir):
    if not fname.endswith('.inc') or fname in skip:
        continue
    fpath = os.path.join(inc_dir, fname)
    try:
        raw = open(fpath, 'rb').read()
        try:
            text = raw.decode('utf-8')
        except Exception:
            text = raw.decode('latin-1')
        new_text = fix(text)
        open(fpath, 'w', encoding='ascii', errors='replace').write(new_text)
        fixed.append(fname)
    except Exception as e:
        print('ERRO em', fname, ':', e)

print('Corrigidos:', len(fixed))
