"""Stufe 9 Punkt 9: Texturen ueberblenden mit bhed_q3map2 (q3map2 _indexmap).

  python tools/karte/q3map2_ueberblenden.py <bhed_q3map2.exe> <karteueberblenden.exe>

Eine func_group aus vier Bodenplatten (x -128..128, je 64 breit) mit _indexmap (4 x 4, linke
Haelfte Schicht 0, rechte Schicht 1; TGA aus karteueberblenden --tga), _layers 2 und
_shader jks_blend/test; die Shader aus karteueberblenden --shader (wie der Editor sie schreibt).
-bsp -meta muss die Flaechen auf textures/jks_blend/test_0, _1 und den Uebergang _0to1 legen
(GetShaderIndexForPoint, GetIndexedShader); ohne _indexmap (Gegenprobe) keiner davon.
"""
import os
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from q3map2_pfade import kasten, raum  # noqa: E402

NL = '\n'
NUL = b'\x00'


def lumps(bsp):
    if not os.path.exists(bsp):
        return None
    d = open(bsp, 'rb').read()
    if d[:4] != b'RBSP':
        return None
    out = []
    for i in range(18):
        ofs, laenge = struct.unpack_from('<ii', d, 8 + i * 8)
        out.append(d[ofs:ofs + laenge])
    return out


def flaechen_shader(l):
    shader = [l[1][i:i + 64].split(NUL)[0].decode('latin-1') for i in range(0, len(l[1]), 72)]
    namen = set()
    for i in range(0, len(l[13]), 148):
        n = struct.unpack_from('<i', l[13], i)[0]
        if 0 <= n < len(shader):
            namen.add(shader[n])
    return namen


def bauen(exe, ordner, mit_index):
    gruppe = '{' + NL + '"classname" "func_group"' + NL
    if mit_index:
        gruppe += '"_indexmap" "maps/test_blend.tga"' + NL + '"_layers" "2"' + NL + '"_shader" "jks_blend/test"' + NL
    for x in (-128, -64, 0, 64):
        gruppe += kasten(x, -64, 0, x + 64, 64, 8, 'jks_blend/test')
    gruppe += '}' + NL
    name = 'blend%d' % int(mit_index)
    karte = os.path.join(ordner, name + '.map')
    with open(karte, 'w', encoding='utf-8', newline=NL) as f:
        f.write(raum() + gruppe)
    r = subprocess.run([exe, '-game', 'ja', '-fs_basepath', ordner, '-bsp', '-meta', karte], capture_output=True, timeout=900, cwd=ordner)
    l = lumps(os.path.join(ordner, name + '.bsp'))
    return r.returncode, (flaechen_shader(l) if l else None)


def main():
    exe, hilfe = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    fehler = 0
    with tempfile.TemporaryDirectory(prefix='bhed_blend_') as ordner:
        os.makedirs(os.path.join(ordner, 'base', 'shaders'))
        os.makedirs(os.path.join(ordner, 'base', 'maps'))
        text = subprocess.run([hilfe, '--shader', 'jks_blend/test', 'textures/base/floor', 'textures/base/wall'],
                              capture_output=True, check=True).stdout
        with open(os.path.join(ordner, 'base', 'shaders', 'jks_blend.shader'), 'wb') as f:
            f.write(text)
        subprocess.run([hilfe, '--tga', os.path.join(ordner, 'base', 'maps', 'test_blend.tga')], check=True)
        rc, namen = bauen(exe, ordner, True)
        soll = {'textures/jks_blend/test_0', 'textures/jks_blend/test_1', 'textures/jks_blend/test_0to1'}
        gut = rc == 0 and namen is not None and soll <= namen
        print(('ok    ' if gut else 'FEHL  ') + 'mit _indexmap: Flaechen auf _0, _1 und _0to1 (%s)' % (sorted(n for n in (namen or []) if 'jks_blend' in n),))
        fehler += 0 if gut else 1
        rc, namen = bauen(exe, ordner, False)
        gut = rc == 0 and namen is not None and not any(n.startswith('textures/jks_blend/test_') for n in namen)
        print(('ok    ' if gut else 'FEHL  ') + 'ohne _indexmap (Gegenprobe): keine Schicht-Shader')
        fehler += 0 if gut else 1
    print('OK' if fehler == 0 else 'FEHLER (%d)' % fehler)
    return 0 if fehler == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
