#!/usr/bin/env python3
"""Die Regressionsfaelle aus NRC-PR #316 (auto-pigeon/q3map2, regression_tests/auto-pigeon)
gegen bhed_q3map2 - Stufe 6 Punkt 3 (Geometrie-Fixes nur, wenn keine Karte schlechter wird).

  q3map2_pr316.py <bhed_q3map2.exe> <ordner mit cases.json> [--fall ID ...] [--json bericht.json]
                  [--threads 2]

Eigene Auswertung (nur die Standardbibliothek), nach der Beschreibung der Faelle in
cases.json: jede Karte mit -game quake3 -bsp -meta bauen, dann
  leak         Urteil aus der Ausgabe ("leaked") wie erwartet (null: egal); bei einem Leck
               muss der Weg aus der .lin durch die angegebenen Kaesten gehen (leak_route)
  solid/open   Punkt im Knotenbaum -> Blatt; Inhalt = ODER der Pinsel des Blatts, die den
               Punkt enthalten; solid: CONTENTS_SOLID; open: Blatt nicht undurchsichtig
               (cluster != -1) und weder solid noch playerclip
  contents     has/lacks bestimmter CONTENTS-Bits
  drawn        ein gezeichnetes Dreieck (Weltmodell, planar oder Dreiecks-Suppe, nicht nodraw)
               mit Normale innerhalb 5 Grad, Abstand zur Ebene <= 0.2 und dem Punkt innerhalb
               (Rand 0.01); optional shader und st (modulo 1, 0.005)
  not_drawn    kein solches Dreieck
  patch        der n-te Patch: shader, Groesse, Kontrollpunkte (0.001) und st
Bekannte Fehlschlaege: known-failures.json (bleibt Fehlschlag, darf aber nicht bestehen).
Rueckgabe: Anzahl unerwarteter Ergebnisse (fail oder unexpected_pass); mit --erwartet die Zahl
der Faelle, deren Zustand von der Datei abweicht (ctest q3map2_pr316, tools/karte/q3map2_pr316_erwartet.json).
"""
import argparse
import json
import math
import os
import shutil
import struct
import subprocess
import sys
import tempfile

LUMPS = ('entities', 'shaders', 'planes', 'nodes', 'leafs', 'leafsurfaces', 'leafbrushes', 'models', 'brushes', 'brushsides',
         'drawverts', 'drawindexes', 'fogs', 'surfaces', 'lightmaps', 'lightgrid', 'visibility')
CONTENTS = {'solid': 0x1, 'lava': 0x8, 'slime': 0x10, 'water': 0x20, 'fog': 0x40, 'playerclip': 0x10000, 'monsterclip': 0x20000,
            'detail': 0x8000000, 'structural': 0x10000000, 'trans': 0x20000000}
SURF_NODRAW = 0x80
IDLE = 0x00000040 if os.name == 'nt' else 0
BILDER = {'apreg/checker': (64, 64), 'apreg/wide': (64, 32), 'apreg/patch': (64, 64), 'apreg/water': (64, 64), 'apreg/clip': (64, 64),
          'common/caulk': (64, 64), 'AUPCHK1': (32, 32)}


def tga(w, h):
    kopf = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, w, h, 24, 0)
    return kopf + bytes(b for y in range(h) for x in range(w) for b in ((0xd0,) * 3 if ((x // 8) + (y // 8)) % 2 else (0x40, 0x80, 0x40)))


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


class Bsp:
    def __init__(self, pfad):
        d = open(pfad, 'rb').read()
        if d[:4] != b'IBSP':
            raise ValueError('keine IBSP-Datei: %s' % pfad)
        self.l = {}
        for i, n in enumerate(LUMPS):
            o, n2 = struct.unpack_from('<ii', d, 8 + 8 * i)
            self.l[n] = d[o:o + n2]
        rec = lambda n, g: [self.l[n][i:i + g] for i in range(0, len(self.l[n]), g)]
        self.shaders = [(r[:64].split(b'\0')[0].decode('latin-1'),) + struct.unpack_from('<ii', r, 64) for r in rec('shaders', 72)]
        self.planes = [struct.unpack('<4f', r) for r in rec('planes', 16)]
        self.nodes = [struct.unpack('<3i6i', r) for r in rec('nodes', 36)]
        self.leafs = [struct.unpack('<2i6i4i', r) for r in rec('leafs', 48)]
        self.leafbrushes = list(struct.unpack('<%di' % (len(self.l['leafbrushes']) // 4), self.l['leafbrushes']))
        self.models = [struct.unpack('<6f4i', r) for r in rec('models', 40)]
        self.brushes = [struct.unpack('<3i', r) for r in rec('brushes', 12)]
        self.brushsides = [struct.unpack('<2i', r) for r in rec('brushsides', 8)]
        self.verts = [struct.unpack('<3f2f2f3f4B', r) for r in rec('drawverts', 44)]
        self.indexes = list(struct.unpack('<%di' % (len(self.l['drawindexes']) // 4), self.l['drawindexes']))
        self.surfaces = [struct.unpack('<12i3f9f2i', r) for r in rec('surfaces', 104)]

    def blatt(self, p):
        n = 0
        while n >= 0:
            pl, vorn, hinten = self.nodes[n][:3]
            a = self.planes[pl]
            n = vorn if a[0] * p[0] + a[1] * p[1] + a[2] * p[2] - a[3] >= 0 else hinten
        return -n - 1

    def inhalt(self, p, eps=0.001):
        b = self.blatt(p)
        cl = self.leafs[b][0]
        erste, anzahl = self.leafs[b][10], self.leafs[b][11]
        c = 0
        for i in range(erste, erste + anzahl):
            br = self.leafbrushes[i]
            f, n, sh = self.brushes[br]
            if all(dot(self.planes[self.brushsides[s][0]][:3], p) - self.planes[self.brushsides[s][0]][3] <= eps for s in range(f, f + n)):
                c |= self.shaders[sh][2]
        return cl == -1, c

    def welt(self):
        return range(self.models[0][6], self.models[0][6] + self.models[0][7])

    def gezeichnet(self, p, normale, ebene=0.2, winkel=5.0, rand=0.01):
        grenze = math.cos(math.radians(winkel))
        for si in self.welt():
            s = self.surfaces[si]
            shader, sflags, _ = self.shaders[s[0]]
            if sflags & SURF_NODRAW or s[2] not in (1, 3):
                continue
            for i in range(s[5], s[5] + s[6] - 2, 3):
                tri = [self.verts[s[3] + self.indexes[i + k]] for k in range(3)]
                a, b, c = (v[0:3] for v in tri)
                n = cross(sub(b, a), sub(c, a))
                ln = math.sqrt(dot(n, n))
                if ln < 1e-9:
                    continue
                vz = 1.0 if dot(n, tuple(sum(v[7 + k] for v in tri) for k in range(3))) >= 0 else -1.0
                n = (vz * n[0] / ln, vz * n[1] / ln, vz * n[2] / ln)
                if dot(n, normale) < grenze or abs(dot(sub(p, a), n)) > ebene:
                    continue
                fl = dot(cross(sub(b, a), sub(c, a)), n)
                if abs(fl) < 1e-12:
                    continue
                wa = dot(cross(sub(b, p), sub(c, p)), n) / fl
                wb = dot(cross(sub(c, p), sub(a, p)), n) / fl
                w = (wa, wb, 1.0 - wa - wb)
                if min(w) >= -rand:
                    return shader, tuple(sum(w[j] * tri[j][3 + k] for j in range(3)) for k in range(2))
        return None

    def patches(self):
        out = []
        for si in self.welt():
            s = self.surfaces[si]
            if s[2] == 2:
                out.append((self.shaders[s[0]][0], s[24], s[25], [(v[0:3], v[3:5]) for v in self.verts[s[3]:s[3] + s[4]]]))
        return out


def modulo1(a, b):
    d = a - b
    return abs(d - round(d)) <= 0.005


def probe(bsp, pr):
    k = pr['kind']
    if k in ('solid', 'open', 'contents'):
        undurch, c = bsp.inhalt(tuple(pr['point']))
        if k == 'solid':
            return bool(c & CONTENTS['solid'])
        if k == 'open':
            return not undurch and not (c & (CONTENTS['solid'] | CONTENTS['playerclip']))
        return all(c & CONTENTS[n] for n in pr['has']) and not any(c & CONTENTS[n] for n in pr['lacks'])
    if k in ('drawn', 'not_drawn'):
        t = bsp.gezeichnet(tuple(pr['point']), tuple(pr['normal']))
        if k == 'not_drawn':
            return t is None
        if t is None:
            return False
        if 'shader' in pr and t[0].lower() != pr['shader'].lower():
            return False
        return 'st' not in pr or all(modulo1(g, w) for g, w in zip(t[1], pr['st']))
    if k == 'patch':
        ps = bsp.patches()
        if len(ps) <= pr['index']:
            return False
        sh, w, h, pts = ps[pr['index']]
        if sh.lower() != pr['shader'].lower() or (w, h) != (pr['width'], pr['height']) or len(pts) != len(pr['points']):
            return False
        return all(all(abs(a - b) <= 0.001 for a, b in zip(xyz, soll[:3])) and all(modulo1(a, b) for a, b in zip(st, soll[3:]))
                   for (xyz, st), soll in zip(pts, pr['points']))
    raise ValueError(k)


def kasten_getroffen(a, b, lo, hi):
    t0, t1 = 0.0, 1.0
    for i in range(3):
        d = b[i] - a[i]
        if abs(d) < 1e-12:
            if a[i] < lo[i] or a[i] > hi[i]:
                return False
            continue
        u, v = (lo[i] - a[i]) / d, (hi[i] - a[i]) / d
        t0, t1 = max(t0, min(u, v)), min(t1, max(u, v))
        if t0 > t1:
            return False
    return True


def fall(exe, daten, f, arbeit, threads, spielwurzel):
    ordner = os.path.join(arbeit, f['id'])
    os.makedirs(ordner)
    quelle = os.path.join(daten, f['map'])
    karte = os.path.join(ordner, os.path.basename(quelle))
    shutil.copyfile(quelle, karte)
    wurzel = spielwurzel
    if f.get('gameroot'):
        wurzel = os.path.join(ordner, 'wurzel')
        shutil.copytree(os.path.join(daten, f['gameroot']), os.path.join(wurzel, 'baseq3'), ignore=shutil.ignore_patterns('maps'))
    args = [exe, '-bsp', '-game', 'quake3', '-fs_basepath', wurzel + os.sep, '-fs_homepath', os.path.join(ordner, 'home'),
            '-threads', str(threads), '-meta', karte]
    p = subprocess.run(args, cwd=ordner, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, creationflags=IDLE, timeout=900)
    aus = p.stdout.decode('utf-8', 'replace')
    leck = '******* leaked *******' in aus or 'MAP LEAKED' in aus
    bsp_pfad = karte[:-4] + '.bsp'
    fehler = []
    if p.returncode != 0 or not os.path.exists(bsp_pfad):
        return ['bsp: Rueckgabe %d' % p.returncode], leck
    soll = f['expect']['leak']
    if soll is not None and leck != soll:
        fehler.append('Leck-Urteil %s, die Quelle %s' % ('leck' if leck else 'dicht', 'leckt' if soll else 'ist dicht'))
    elif leck and f.get('leak_route'):
        lin = karte[:-4] + '.lin'
        pk = [tuple(float(v) for v in z.split()) for z in open(lin, encoding='utf-8')] if os.path.exists(lin) else []
        seg = list(zip(pk, pk[1:])) or [(q, q) for q in pk]
        for k in f['leak_route']:
            if not any(kasten_getroffen(a, b, k['min'], k['max']) for a, b in seg):
                fehler.append('Leck-Weg geht nicht durch %s' % k['why'])
    bsp = Bsp(bsp_pfad)
    for i, pr in enumerate(f['probes']):
        if not probe(bsp, pr):
            fehler.append('Probe %d (%s: %s)' % (i, pr['kind'], pr['why']))
    return fehler, leck


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('exe')
    ap.add_argument('daten')
    ap.add_argument('--fall', action='append', default=[])
    ap.add_argument('--json')
    ap.add_argument('--threads', default='2')
    ap.add_argument('--erwartet', help='JSON mit dem erwarteten Zustand je Fall (ctest): Rueckgabe = Abweichungen davon')
    a = ap.parse_args(argv)
    exe = os.path.abspath(a.exe)
    daten = os.path.abspath(a.daten)
    faelle = json.load(open(os.path.join(daten, 'cases.json'), encoding='utf-8'))['cases']
    bekannt = json.load(open(os.path.join(daten, 'known-failures.json'), encoding='utf-8'))['cases']
    arbeit = tempfile.mkdtemp(prefix='pr316_')
    wurzel = os.path.join(arbeit, '_wurzel')
    shutil.copytree(os.path.join(daten, 'gameroot', 'baseq3', 'scripts'), os.path.join(wurzel, 'baseq3', 'scripts'))
    for n, (w, h) in BILDER.items():
        z = os.path.join(wurzel, 'baseq3', 'textures', n + '.tga')
        os.makedirs(os.path.dirname(z), exist_ok=True)
        open(z, 'wb').write(tga(w, h))
    bericht = {}
    unerwartet = 0
    zahl = {'pass': 0, 'fail': 0, 'known_failure': 0, 'unexpected_pass': 0}
    try:
        for f in faelle:
            if a.fall and f['id'] not in a.fall:
                continue
            fehler, leck = fall(exe, daten, f, arbeit, a.threads, wurzel)
            ist_bekannt = f['id'] in bekannt
            zustand = ('known_failure' if ist_bekannt else 'fail') if fehler else ('unexpected_pass' if ist_bekannt else 'pass')
            zahl[zustand] += 1
            unerwartet += zustand in ('fail', 'unexpected_pass')
            bericht[f['id']] = {'zustand': zustand, 'fehler': fehler, 'leck': leck}
            print('%-16s %-24s %s' % (zustand, f['id'], '; '.join(fehler[:3])))
            sys.stdout.flush()
    finally:
        shutil.rmtree(arbeit, ignore_errors=True)
    print('q3map2_pr316: %s' % ', '.join('%s %d' % kv for kv in zahl.items()))
    if a.json:
        with open(a.json, 'w', encoding='utf-8', newline='\n') as f:
            json.dump(bericht, f, indent=1, sort_keys=True)
    if a.erwartet:
        soll = json.load(open(a.erwartet, encoding='utf-8'))['faelle']
        abw = [k for k in sorted(set(soll) | set(bericht)) if soll.get(k) != bericht.get(k, {}).get('zustand')]
        for k in abw:
            print('ANDERS  %s: erwartet %s, ist %s' % (k, soll.get(k), bericht.get(k, {}).get('zustand')))
        print('q3map2_pr316: %d Abweichung(en) vom erwarteten Zustand' % len(abw))
        return len(abw)
    return unerwartet


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
