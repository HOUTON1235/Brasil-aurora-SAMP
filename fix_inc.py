#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import re, sys, os

def fix_file(path):
    with open(path, 'rb') as f:
        raw = f.read()
    # Remove BOM
    if raw.startswith(b'\xef\xbb\xbf'):
        raw = raw[3:]
    # Decode as UTF-8 with replacement
    text = raw.decode('utf-8', errors='replace')
    
    # Remove relative includes (not needed in flat pawno/include)
    text = re.sub(r'#include\s+"[^"]*"[^\r\n]*', '// include interno removido', text)
    
    # Fix double-encoded UTF-8 sequences (latin1 mis-decoded)
    replacements = [
        # Must be in order (longer matches first)
        ('\u00c3\u00a3o', 'ao'),
        ('\u00c3\u00a7\u00c3\u00a3', 'ca'),
        ('\u00c3\u00b5es', 'oes'),
        ('\u00c3\u00a3', 'a'),
        ('\u00c3\u00a1', 'a'),
        ('\u00c3\u00a0', 'a'),
        ('\u00c3\u00a2', 'a'),
        ('\u00c3\u00a9', 'e'),
        ('\u00c3\u00aa', 'e'),
        ('\u00c3\u00ab', 'e'),
        ('\u00c3\u00a8', 'e'),
        ('\u00c3\u00ad', 'i'),
        ('\u00c3\u00ae', 'i'),
        ('\u00c3\u00af', 'i'),
        ('\u00c3\u00b3', 'o'),
        ('\u00c3\u00b4', 'o'),
        ('\u00c3\u00b6', 'o'),
        ('\u00c3\u00b5', 'o'),
        ('\u00c3\u00ba', 'u'),
        ('\u00c3\u00bb', 'u'),
        ('\u00c3\u00bc', 'u'),
        ('\u00c3\u00a7', 'c'),
        ('\u00c3\u00b1', 'n'),
        ('\u00c3\u0089', 'E'),
        ('\u00c3\u0093', 'O'),
        ('\u00c3\u009a', 'U'),
        ('\u00c3\u0082', 'A'),
        ('\u00c3\u0087', 'C'),
        ('\u00e2\u0080\u0093', '-'),
        ('\u00e2\u0080\u0099', "'"),
        ('\u00e2\u0080\u009c', '"'),
        ('\u00e2\u0080\u009d', '"'),
        ('\u00e2\u0080', '-'),
    ]
    for old, new in replacements:
        text = text.replace(old, new)
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(text)
    print(f"Fixed: {os.path.basename(path)} ({len(text)} chars)")

if __name__ == '__main__':
    files = sys.argv[1:] if len(sys.argv) > 1 else []
    for f in files:
        fix_file(f)
