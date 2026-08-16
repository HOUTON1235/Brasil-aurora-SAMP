lines = open(r'C:\Users\Felip\Downloads\Servidor 2\pawno\include\player_core.inc', encoding='ascii').readlines()
depth = 0
for i, l in enumerate(lines[:167]):
    in_str = False
    for c in l:
        if c == '"': in_str = not in_str
        if not in_str:
            if c == '{': depth += 1
            elif c == '}': depth -= 1
print('Depth na linha 167:', depth)

# mostrar linhas suspeitas
depth2 = 0
open_funcs = []
for i, l in enumerate(lines[:167]):
    s = l.strip()
    in_str = False
    opens = closes = 0
    for c in l:
        if c == '"': in_str = not in_str
        if not in_str:
            if c == '{': opens += 1
            elif c == '}': closes += 1
    depth2 += opens - closes
    if opens and (s.startswith('stock ') or s.startswith('public ')):
        open_funcs.append((i+1, depth2, s[:70]))

print('Ultimas funcoes abertas:')
for fn in open_funcs[-10:]:
    print('  Linha %d depth=%d: %s' % fn)
