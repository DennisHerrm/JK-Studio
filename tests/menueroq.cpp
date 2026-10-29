// menueroq.cpp - Menue-Editor M2 Punkt 2: RoQ schreiben und wie die Engine
// abspielen (cl_cin.cpp-Port), GIF lesen.
//
//   - Kodierer -> Spieler: jedes Bild kommt mit PSNR > 30 dB zurueck, Kopf,
//     fps, Groesse, alle Stuecke <= 65536 Bytes (sonst bricht die Engine ab),
//     Zeitsteuerung wie CIN_RunCinematic (tfps = ms * fps / 1000), Schleife.
//   - Guete: hoehere Guete = kleinerer Fehler.
//   - GIF mit drei Bildern und Dauer; aufFps verteilt sie auf 20 fps.
//   - Mit Movie-Duels-Ordner: video/SettingsVideo.roq aus den pk3 spielen.
#include "bhed/menue/roq.h"
#include "bhed/menue/spieldaten.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

RoqBild probe(int b, int h, int t) {
    RoqBild r;
    r.breite = b;
    r.hoehe = h;
    r.rgba.resize(static_cast<std::size_t>(b * h * 4));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < b; ++x) {
            std::uint8_t* p = &r.rgba[static_cast<std::size_t>((y * b + x) * 4)];
            p[0] = static_cast<std::uint8_t>(x * 255 / b);
            p[1] = static_cast<std::uint8_t>(y * 255 / h);
            p[2] = 90;
            p[3] = 255;
            // ein helles Quadrat, das sich je Bild um 2 Punkte nach rechts schiebt
            if (x >= 8 + t * 2 && x < 24 + t * 2 && y >= 12 && y < 28) {
                p[0] = 250;
                p[1] = 240;
                p[2] = 30;
            }
        }
    }
    return r;
}

double psnr(const std::uint8_t* a, const RoqBild& b) {
    double s = 0;
    const std::size_t n = static_cast<std::size_t>(b.breite * b.hoehe);
    for (std::size_t i = 0; i < n; ++i) {
        for (int k = 0; k < 3; ++k) {
            const double d = static_cast<double>(a[i * 4 + static_cast<std::size_t>(k)]) - b.rgba[i * 4 + static_cast<std::size_t>(k)];
            s += d * d;
        }
    }
    const double mse = s / static_cast<double>(n * 3);
    return mse <= 0 ? 99.0 : 10.0 * std::log10(255.0 * 255.0 / mse);
}

// alle Stuecke durchgehen: Groesse <= 65536
bool stueckeOk(const std::string& d, int& anzahl) {
    anzahl = 0;
    std::size_t p = 8;
    while (p + 8 <= d.size()) {
        const auto u = [&](std::size_t i) { return static_cast<unsigned>(static_cast<unsigned char>(d[i])); };
        const unsigned groesse = u(p + 2) | u(p + 3) << 8 | u(p + 4) << 16;
        if (groesse > 65536) { return false; }
        p += 8 + groesse;
        ++anzahl;
    }
    return p == d.size();
}

// GIF89a 1x1, drei Bilder (rot 50 ms, gruen 100 ms, blau 200 ms) - von Hand
const unsigned char kGif[] = {
    'G', 'I', 'F', '8', '9', 'a', 1, 0, 1, 0, 0x81, 0, 0,
    0xFF, 0, 0, 0, 0xFF, 0, 0, 0, 0xFF, 0, 0, 0,
    0x21, 0xF9, 4, 0, 5, 0, 0, 0, 0x2C, 0, 0, 0, 0, 1, 0, 1, 0, 0, 2, 2, 0x44, 0x01, 0,
    0x21, 0xF9, 4, 0, 10, 0, 0, 0, 0x2C, 0, 0, 0, 0, 1, 0, 1, 0, 0, 2, 2, 0x4C, 0x01, 0,
    0x21, 0xF9, 4, 0, 20, 0, 0, 0, 0x2C, 0, 0, 0, 0, 1, 0, 1, 0, 0, 2, 2, 0x54, 0x01, 0,
    0x3B};
}  // namespace

int main(int argc, char** argv) {
    // --- Rundlauf Kodierer -> Spieler ---------------------------------------------
    const int B = 64, H = 48, N = 10, FPS = 20;
    RoqKodierer k;
    RoqOptionen o;
    o.fps = FPS;
    o.guete = 85;
    pruefe(!k.beginne(60, 48, o), "Kodierer: Breite muss Vielfaches von 16 sein");
    pruefe(k.beginne(B, H, o), "Kodierer: 64x48");
    std::vector<RoqBild> quelle;
    for (int t = 0; t < N; ++t) {
        quelle.push_back(probe(B, H, t));
        k.bild(quelle.back());
    }
    const std::string roq = k.fertig();
    int stuecke = 0;
    pruefe(roq.size() > 8 && static_cast<unsigned char>(roq[0]) == 0x84 && static_cast<unsigned char>(roq[1]) == 0x10, "Kopf 0x1084");
    pruefe(stueckeOk(roq, stuecke) && stuecke == 2 + 2 * N, "Stuecke: QUAD_INFO + je Bild CODEBOOK und QUAD_VQ + leeres Schlussstueck, alle <= 65536");

    RoqSpieler s;
    pruefe(s.start(roq, 1000, false), "Spieler: Start (CIN_PlayCinematic)");
    pruefe(s.bild() == nullptr, "Spieler: vor dem ersten lauf kein Bild");
    // der erste lauf liest ROQ_QUAD_INFO und setzt dort startTime neu (wie die Engine)
    (void)s.lauf(1000);
    pruefe(s.bild() == nullptr && s.bilder() == 0, "Spieler: ROQ_QUAD_INFO setzt die Zeit neu, noch kein Bild");
    double schlecht = 99;
    for (int t = 0; t < N; ++t) {
        // CIN_RunCinematic: tfps = (jetzt - start) * fps / 1000; Bild t ist fertig, wenn numQuads == t + 1
        (void)s.lauf(1000 + (t + 1) * 1000 / FPS);
        if (t == 0) { pruefe(s.breite() == B && s.hoehe() == H && s.fps() == FPS, "Spieler: QUAD_INFO 64x48, fps 20"); }
        const std::uint8_t* b = s.bild();
        if (b == nullptr) {
            pruefe(false, "Spieler: Bild " + std::to_string(t));
            continue;
        }
        const double p = psnr(b, quelle[static_cast<std::size_t>(t)]);
        schlecht = std::min(schlecht, p);
        if (p < 30.0) { std::printf("      Bild %d: PSNR %.1f dB\n", t, p); }
    }
    std::printf("menueroq: %zu Bytes fuer %d Bilder 64x48, schlechtestes Bild %.1f dB\n", roq.size(), N, schlecht);
    pruefe(schlecht > 30.0, "Rundlauf: jedes Bild > 30 dB");
    pruefe(s.bilder() == N, "Spieler: numQuads = Bilder");
    (void)s.lauf(1000 + (N + 5) * 1000 / FPS);
    pruefe(s.status() == RoqSpieler::Status::Eof, "Spieler ohne Schleife: am Ende FMV_EOF");
    // mit Schleife (CIN_loop): danach wieder von vorn
    RoqSpieler sl;
    pruefe(sl.start(roq, 0, true), "Spieler mit Schleife");
    (void)sl.lauf(0);
    (void)sl.lauf((N + 2) * 1000 / FPS);
    pruefe(sl.status() == RoqSpieler::Status::Play && sl.bilder() < N, "Schleife: RoQReset, spielt weiter");
    // Guete: niedrig = kleiner, aber schlechter
    {
        RoqKodierer k2;
        RoqOptionen o2 = o;
        o2.guete = 5;
        (void)k2.beginne(B, H, o2);
        for (const RoqBild& q : quelle) { k2.bild(q); }
        const std::string klein = k2.fertig();
        RoqSpieler s2;
        (void)s2.start(klein, 0, false);
        (void)s2.lauf(0);
        (void)s2.lauf(N * 1000 / FPS);
        const double p2 = s2.bild() != nullptr ? psnr(s2.bild(), quelle.back()) : 0.0;
        std::printf("menueroq: Guete 5: %zu Bytes, letztes Bild %.1f dB\n", klein.size(), p2);
        pruefe(klein.size() < roq.size(), "Guete 5: kleinere Datei");
    }
    pruefe(!RoqSpieler().start(std::string("keinroq"), 0), "Spieler: falscher Kopf abgelehnt");
    // --- GIF ----------------------------------------------------------------------
    Bildfolge f;
    pruefe(gifLesen(std::string(reinterpret_cast<const char*>(kGif), sizeof(kGif)), f), "GIF gelesen (stb_image)");
    pruefe(f.bilder.size() == 3 && f.dauerMs.size() == 3 && f.dauerMs[0] == 50 && f.dauerMs[2] == 200, "GIF: 3 Bilder, 50/100/200 ms");
    if (f.bilder.size() == 3) {
        pruefe(f.bilder[0].rgba[0] == 255 && f.bilder[1].rgba[1] == 255 && f.bilder[2].rgba[2] == 255, "GIF: rot, gruen, blau");
        const std::vector<std::size_t> z = aufFps(f, 20);   // 350 ms bei 20 fps = 7 Takte: r g g b b b b
        pruefe(z.size() == 7 && z[0] == 0 && z[1] == 1 && z[2] == 1 && z[3] == 2 && z[6] == 2, "aufFps: 7 Takte r g g b b b b");
        const std::string g = kodiereRoq(f, o, 64);
        RoqSpieler sg;
        pruefe(sg.start(g, 0, false) && (sg.lauf(0), sg.lauf(60), sg.breite() == 16 && sg.bild() != nullptr), "GIF -> RoQ (16x16)");
    }

    // --- Original aus Movie Duels ----------------------------------------------------
    if (argc > 1) {
        Spieldaten daten;
        for (int i = 1; i < argc; ++i) { (void)daten.ordnerDazu(argv[i]); }
        std::string v;
        if (daten.lies("video/SettingsVideo.roq", v)) {
            RoqSpieler sm;
            pruefe(sm.start(v, 0, true), "MD SettingsVideo.roq: Start");
            int neu = 0;
            for (int ms = 0; ms <= 2000; ms += 100) {
                (void)sm.lauf(ms);
                neu += sm.neuesBild() ? 1 : 0;
            }
            std::printf("menueroq: SettingsVideo.roq %dx%d, %d fps, nach 2 s %ld Bilder\n", sm.breite(), sm.hoehe(), sm.fps(), sm.bilder());
            pruefe(sm.breite() > 0 && sm.hoehe() > 0 && sm.bild() != nullptr && neu >= 15, "MD SettingsVideo.roq: Bilder kommen");
            pruefe(sm.bilder() == 2 * sm.fps() || sm.bilder() == 2 * sm.fps() + 1, "MD: nach 2 s genau 2 * fps Bilder (CIN_RunCinematic)");
            // echte Bilder neu kodieren (Zeit und Guete)
            Bildfolge echt;
            for (int i = 0; i < 6 && sm.bild() != nullptr; ++i) {
                RoqBild b;
                b.breite = sm.breite();
                b.hoehe = sm.hoehe();
                b.rgba.assign(sm.bild(), sm.bild() + static_cast<std::size_t>(b.breite * b.hoehe * 4));
                echt.bilder.push_back(b);
                echt.dauerMs.push_back(1000 / 30);
                (void)sm.lauf(2000 + (i + 1) * 34);
            }
            RoqOptionen oe;
            oe.fps = 30;
            oe.guete = 70;
            const auto t0 = std::chrono::steady_clock::now();
            const std::string wieder = kodiereRoq(echt, oe, 512);
            const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            RoqSpieler sn;
            (void)sn.start(wieder, 0, false);
            (void)sn.lauf(0);
            (void)sn.lauf(1000 / 30 + 1);
            const double pe = sn.bild() != nullptr && !echt.bilder.empty() ? psnr(sn.bild(), echt.bilder[0]) : 0.0;
            std::printf("menueroq: %zu echte Bilder 512x512 neu kodiert in %.2f s (%.2f s/Bild), %zu Bytes, Bild 0 %.1f dB\n",
                        echt.bilder.size(), sek, sek / static_cast<double>(std::max<std::size_t>(1, echt.bilder.size())), wieder.size(), pe);
            pruefe(pe > 28.0, "MD-Bilder neu kodiert: > 28 dB");
        } else {
            std::printf("menueroq: video/SettingsVideo.roq nicht gefunden (uebersprungen)\n");
        }
    }

    std::printf("menueroq: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
