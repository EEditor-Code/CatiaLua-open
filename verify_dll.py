# -*- coding: utf-8 -*-
"""Verify built DLLs are 64-bit."""
import struct
import os

def pe_machine(p):
    with open(p, 'rb') as f:
        d = f.read(1024)
    pe_off = struct.unpack('<I', d[0x3c:0x40])[0]
    return hex(struct.unpack('<H', d[pe_off + 4:pe_off + 6])[0])

base = os.environ.get(
    'CATIALUA_BIN',
    os.path.join(os.path.dirname(os.path.abspath(__file__)), 'win_b64', 'code', 'bin'))
for f in ['CatiaLuaAddin.dll', 'CatiaLuaCmd.dll']:
    p = os.path.join(base, f)
    m = pe_machine(p)
    ok = '64位 ✓' if m == '0x8664' else '非64位!'
    print(f, '->', m, ok)
