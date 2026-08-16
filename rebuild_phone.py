import re, unicodedata

src = r'C:\Users\Felip\Downloads\Servidor 2\includes\phone\phone_core.inc'
dst = r'C:\Users\Felip\Downloads\Servidor 2\pawno\include\phone_core.inc'

raw = open(src, 'rb').read()
if raw.startswith(b'\xef\xbb\xbf'):
    raw = raw[3:]
text = raw.decode('utf-8', errors='replace')

# 1. Remover includes relativos
text = re.sub(r'#include\s+"[^"]*"[^\r\n]*', '// include removido', text)

# 2. Converter para ASCII
def to_ascii(t):
    out = []
    for ch in t:
        if ord(ch) < 128:
            out.append(ch)
        else:
            n = unicodedata.normalize('NFD', ch)
            a = n.encode('ascii', 'ignore').decode('ascii')
            out.append(a if a else '?')
    return ''.join(out)
text = to_ascii(text)

# 3. Remover bool: de returns de stock
text = re.sub(r'stock bool:', 'stock ', text)

# 4. Remover const de parametros de arrays nas assinaturas
text = re.sub(r'\bconst (\w+)\[\]', r'\1[]', text)

# 5. bool:param = true/false em parametros
text = re.sub(r'\bbool:(\w+) = (true|false)', r'\1 = \2', text)

# 6. Adicionar forwards
insert = '\nforward OnPlayerLoaded(playerid);\nforward OnPlayerDisconnect(playerid, reason);\nforward OnScheduler5Minutes();\n'
text = text.replace('#define _SA_PHONE_CORE_INC', '#define _SA_PHONE_CORE_INC' + insert, 1)

opens = text.count('{')
closes = text.count('}')
print('Chaves: opens=%d, closes=%d, diff=%d' % (opens, closes, opens - closes))

open(dst, 'w', encoding='ascii', errors='replace').write(text)
lines = text.splitlines()
print('Salvo: %d linhas' % len(lines))
