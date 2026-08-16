import os, re

inc_dir = r'C:\Users\Felip\Downloads\Servidor 2\pawno\include'

skip = {
    'a_samp.inc','a_mysql.inc','a_npc.inc','a_objects.inc','a_players.inc',
    'a_sampdb.inc','a_vehicles.inc','float.inc','sscanf2.inc','streamer.inc',
    'zcmd.inc','core.inc','string.inc','file.inc','time.inc','datagram.inc',
    'utils.inc','a_http.inc'
}

fixed = 0
for fname in os.listdir(inc_dir):
    if not fname.endswith('.inc') or fname in skip:
        continue
    fpath = os.path.join(inc_dir, fname)
    text = open(fpath, 'r', encoding='ascii', errors='replace').read()
    # Normalize line endings
    text = text.replace('\r\n', '\n').replace('\r', '\n')
    # Remove consecutive blank lines (collapse 2+ blank lines to 1)
    text = re.sub(r'\n{3,}', '\n\n', text)
    # Remove leading blank lines
    text = text.lstrip('\n')
    open(fpath, 'w', encoding='ascii', errors='replace').write(text)
    fixed += 1

print('Blanks fixed in', fixed, 'files')
