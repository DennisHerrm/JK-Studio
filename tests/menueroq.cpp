// menueroq.cpp - Menue-Editor M2 Punkt 2: RoQ schreiben und wie die Engine
// abspielen (cl_cin.cpp-Port), GIF lesen.
//
//   - Kodierer -> Spieler: jedes Bild kommt mit PSNR > 30 dB zurueck, Kopf,
//     fps, Groesse, alle Stuecke <= 65536 Bytes (sonst bricht die Engine ab),
//     Zeitsteuerung wie CIN_RunCinematic (tfps = ms * fps / 1000), Schleife.
//   - Guete: hoehere Guete = kleinerer Fehler.
//   - GIF mit drei Bildern und Dauer; aufFps verteilt sie auf 20 fps.
//   - videoMap in einem Shaderskript (gfx/menus/videologo des JKA-Hauptmenues).
//   - Mit Movie-Duels-Ordner: video/SettingsVideo.roq aus den pk3 spielen.
//   - M4: Zielgroessen der Engine (Zweierpotenz, cin.linbuf), Flaechenmittel, Balken/
//     Zuschneiden, Kodieren im Fluss, Ton (Fenster-Sinc auf 22050 Hz, DPCM-Rundlauf
//     ueber den 1:1-Decoder, Lage der Tonstuecke wie ja01.roq), Codec-Gruppen.
#include "bhed/menue/roq.h"
#include "bhed/menue/spieldaten.h"
#include "bhed/shaderscript.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
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
    // --- videoMap (tr_shader.cpp ParseStage) ------------------------------------------
    {
        bhed::ShaderMap sm;
        bhed::parseShaderScript("gfx/menus/videologo\n{\n    {\n        videoMap video/ja01\n    }\n}\n", sm);
        const auto it = sm.find("gfx/menus/videologo");
        pruefe(it != sm.end() && it->second.stages.size() == 1 && it->second.stages[0].video == "video/ja01",
               "Shader: videoMap als Stufe mit Video (JKA-Hauptmenue)");
    }

    // --- GIF ----------------------------------------------------------------------
    Bildfolge f;
    pruefe(gifLesen(std::string(reinterpret_cast<const char*>(kGif), sizeof(kGif)), f), "GIF gelesen (stb_image)");
    pruefe(f.bilder.size() == 3 && f.dauerMs.size() == 3 && f.dauerMs[0] == 50 && f.dauerMs[2] == 200, "GIF: 3 Bilder, 50/100/200 ms");
    if (f.bilder.size() == 3) {
        pruefe(f.bilder[0].rgba[0] == 255 && f.bilder[1].rgba[1] == 255 && f.bilder[2].rgba[2] == 255, "GIF: rot, gruen, blau");
        const std::vector<std::size_t> z = aufFps(f, 20);   // 350 ms bei 20 fps = 7 Takte: r g g b b b b
        pruefe(z.size() == 7 && z[0] == 0 && z[1] == 1 && z[2] == 1 && z[3] == 2 && z[6] == 2, "aufFps: 7 Takte r g g b b b b");
        VideoZiel z16;
        z16.breite = z16.hoehe = 16;
        const std::string g = kodiereRoq(f, o, z16);
        RoqSpieler sg;
        pruefe(sg.start(g, 0, false) && (sg.lauf(0), sg.lauf(60), sg.breite() == 16 && sg.bild() != nullptr), "GIF -> RoQ (16x16)");
    }

    // --- M4 Punkt 5a: Zielgroesse, Einpassen, Flaechenmittel ------------------------------
    {
        pruefe(roqGroessePruefen(512, 512) == RoqGroesse::Ok && roqGroessePruefen(1024, 256) == RoqGroesse::Ok,
               "Groesse: 512x512 und 1024x256 spielt die Engine");
        pruefe(roqGroessePruefen(1920, 1088) == RoqGroesse::KeineZweierpotenz && roqGroessePruefen(480, 272) == RoqGroesse::KeineZweierpotenz,
               "Groesse: keine Zweierpotenz (RE_StretchRaw: size not a power of 2)");
        pruefe(roqGroessePruefen(1024, 1024) == RoqGroesse::ZuGross && roqGroessePruefen(1024, 512) == RoqGroesse::ZuGross,
               "Groesse: mehr als 512*512 Punkte (cin.linbuf)");
        // Schachbrett 1 Punkt: halbiert = ueberall Grau (ohne Vorfilter kaeme das Muster zurueck)
        RoqBild sb;
        sb.breite = 64;
        sb.hoehe = 64;
        sb.rgba.resize(64 * 64 * 4);
        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 64; ++x) {
                const std::uint8_t v = ((x + y) & 1) != 0 ? 255 : 0;
                std::uint8_t* p = &sb.rgba[static_cast<std::size_t>((y * 64 + x) * 4)];
                p[0] = p[1] = p[2] = v;
                p[3] = 255;
            }
        }
        VideoZiel z;
        z.breite = z.hoehe = 32;
        const RoqBild h = bildSkalieren(sb, z);
        int minV = 255, maxV = 0;
        for (std::size_t i = 0; i < h.rgba.size(); i += 4) {
            minV = std::min<int>(minV, h.rgba[i]);
            maxV = std::max<int>(maxV, h.rgba[i]);
        }
        pruefe(h.breite == 32 && h.hoehe == 32 && minV >= 127 && maxV <= 128, "Flaechenmittel: Schachbrett halbiert = Grau 127/128");
        // 3:1 auf 4:1 (nicht ganzzahlig): Anteile 0.75/0.25 - Quelle 0 0 0 255 255 255 ... (Breite 12 -> 4: je drei Punkte)
        RoqBild st;
        st.breite = 6;
        st.hoehe = 1;
        st.rgba.assign(6 * 4, 255);
        for (int x = 0; x < 6; ++x) { st.rgba[static_cast<std::size_t>(x * 4)] = static_cast<std::uint8_t>(x < 3 ? 0 : 200); }
        VideoZiel z4;
        z4.breite = 4;
        z4.hoehe = 1;
        const RoqBild s4 = bildSkalieren(st, z4);
        // Ziel 1 deckt Quelle [1.5, 3): 0.5 * 0 + 1.0 * 0 -> 0; Ziel 2 deckt [3, 4.5): 200
        pruefe(s4.rgba[0] == 0 && s4.rgba[4] == 0 && s4.rgba[8] == 200 && s4.rgba[12] == 200, "Flaechenmittel: Bereiche 1.5 Punkte (6 -> 4)");
        // 7 -> 4: Ziel 1 deckt [1.75, 3.5) = 0.25*Q1 + Q2 + 0.5*Q3
        RoqBild s7;
        s7.breite = 7;
        s7.hoehe = 1;
        s7.rgba.assign(7 * 4, 255);
        const std::uint8_t q7[7] = {0, 40, 80, 120, 160, 200, 240};
        for (int x = 0; x < 7; ++x) { s7.rgba[static_cast<std::size_t>(x * 4)] = q7[x]; }
        const RoqBild r7 = bildSkalieren(s7, z4);
        const double soll = (0.25 * 40 + 80 + 0.5 * 120) / 1.75;
        pruefe(std::abs(r7.rgba[4] - soll) <= 0.51, "Flaechenmittel: anteilige Randpunkte (7 -> 4)");
        // 2:1-Bild (links rot, rechts blau) auf 64x64: Balken oben/unten, Zuschneiden zeigt nur die Mitte
        RoqBild breit;
        breit.breite = 128;
        breit.hoehe = 64;
        breit.rgba.resize(128 * 64 * 4);
        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 128; ++x) {
                std::uint8_t* p = &breit.rgba[static_cast<std::size_t>((y * 128 + x) * 4)];
                p[0] = x < 64 ? 255 : 0;
                p[1] = 60;
                p[2] = x < 64 ? 0 : 255;
                p[3] = 255;
            }
        }
        VideoZiel q;
        q.breite = q.hoehe = 64;
        q.einpassen = Einpassen::Balken;
        const RoqBild bal = bildSkalieren(breit, q);
        const auto px = [](const RoqBild& b, int x, int y, int k) { return b.rgba[static_cast<std::size_t>((y * b.breite + x) * 4 + k)]; };
        pruefe(px(bal, 32, 2, 0) == 0 && px(bal, 32, 2, 1) == 0 && px(bal, 32, 61, 2) == 0 && px(bal, 5, 32, 0) == 255 && px(bal, 60, 32, 2) == 255 &&
                   px(bal, 32, 16, 1) == 60 && px(bal, 32, 15, 1) == 0,
               "Balken: 2:1 in 64x64 = Bild 64x32 mittig, oben/unten schwarz");
        q.einpassen = Einpassen::Zuschneiden;
        const RoqBild zu = bildSkalieren(breit, q);
        pruefe(px(zu, 2, 0, 0) == 255 && px(zu, 61, 63, 2) == 255 && px(zu, 31, 32, 0) == 255 && px(zu, 32, 32, 2) == 255,
               "Zuschneiden: Mitte 64x64 des 2:1-Bildes (links rot, rechts blau, kein Schwarz)");
        q.einpassen = Einpassen::Strecken;
        const RoqBild str = bildSkalieren(breit, q);
        pruefe(px(str, 31, 0, 0) == 255 && px(str, 32, 63, 2) == 255, "Strecken: ganzes Bild auf 64x64");
    }
    // --- M4 Punkt 5a: Kodieren im Fluss (Takte wie aufFps) -------------------------------------
    {
        RoqImport imp;
        VideoZiel z;
        z.breite = z.hoehe = 32;
        RoqOptionen oi;
        oi.fps = 10;
        oi.guete = 60;
        pruefe(imp.beginne(z, oi, nullptr), "RoqImport: Start 32x32");
        // Quellbilder bei 0, 50, 260 ms; Ende 400 ms -> Takte 0,100,200,300 ms: 0 1 1 2
        const RoqBild a = probe(64, 48, 0), b = probe(64, 48, 6), c = probe(64, 48, 12);
        imp.bild(a, 1000);   // Zeit relativ zum ersten Bild
        imp.bild(b, 1050);
        imp.bild(c, 1260);
        const std::string r = imp.fertig(400);
        int n = 0;
        pruefe(stueckeOk(r, n) && imp.takte() == 4, "RoqImport: 4 Takte bei 10 fps fuer 400 ms");
    }
    // --- M4 Punkt 5b: Ton - Fenster-Sinc auf 22050 Hz, DPCM wie cl_cin.cpp -------------------
    {
        constexpr double kPi = 3.14159265358979323846;
        // 1 kHz bei 44100 Hz bleibt (Pegel), 15 kHz liegt ueber der neuen Nyquist-Frequenz und verschwindet
        std::vector<std::int16_t> tief(44100), hoch(44100);
        for (int i = 0; i < 44100; ++i) {
            tief[static_cast<std::size_t>(i)] = static_cast<std::int16_t>(std::lround(10000 * std::sin(2 * kPi * 1000 * i / 44100.0)));
            hoch[static_cast<std::size_t>(i)] = static_cast<std::int16_t>(std::lround(10000 * std::sin(2 * kPi * 15000 * i / 44100.0)));
        }
        const RoqTon t1 = tonAuf22050(tief, 44100, 1, 1);
        const RoqTon t2 = tonAuf22050(hoch, 44100, 1, 1);
        double e1 = 0, e2 = 0, f1 = 0;
        for (std::size_t i = 200; i + 200 < t1.pcm.size(); ++i) {
            const double soll = 10000 * std::sin(2 * kPi * 1000 * static_cast<double>(i) / 22050.0);
            e1 += static_cast<double>(t1.pcm[i]) * t1.pcm[i];
            f1 += (t1.pcm[i] - soll) * (t1.pcm[i] - soll);
            e2 += static_cast<double>(t2.pcm[i]) * t2.pcm[i];
        }
        const double n1 = static_cast<double>(t1.pcm.size() - 400);
        const double eff1 = std::sqrt(e1 / n1), eff2 = std::sqrt(e2 / n1), feh1 = std::sqrt(f1 / n1);
        std::printf("menueroq: 44100->22050: 1 kHz Effektivwert %.0f (Fehler %.1f), 15 kHz %.1f\n", eff1, feh1, eff2);
        pruefe(t1.pcm.size() == 22050 && std::abs(eff1 - 7071) < 100 && feh1 < 70, "Ton: 1 kHz 44100 -> 22050 Hz bleibt (Pegel, Phase)");
        pruefe(eff2 < 0.02 * 7071, "Ton: 15 kHz verschwindet (kein Ueberlappen)");
        // Stereo 48000 -> mono 22050: Mittel der Kanaele
        std::vector<std::int16_t> st(2 * 4800);
        for (int i = 0; i < 4800; ++i) {
            st[static_cast<std::size_t>(2 * i)] = 6000;
            st[static_cast<std::size_t>(2 * i + 1)] = 2000;
        }
        const RoqTon m = tonAuf22050(st, 48000, 2, 1);
        pruefe(m.kanaele == 1 && m.pcm.size() == 2205 && m.pcm[1000] == 4000, "Ton: Stereo 48 kHz -> mono 22050 Hz (Mittel)");

        // Rundlauf Kodierer -> Decoder (RllDecode* 1:1), mono und stereo
        for (int kan = 1; kan <= 2; ++kan) {
            const int fps = 30, bilder = 45;
            const std::size_t proben = static_cast<std::size_t>(bilder) * kRoqTonRate / fps;
            RoqTon ton;
            ton.kanaele = kan;
            ton.pcm.resize(proben * static_cast<std::size_t>(kan));
            std::mt19937 zufall(7);
            for (std::size_t i = 0; i < proben; ++i) {
                const double t = static_cast<double>(i) / kRoqTonRate;
                const double v = 9000 * std::sin(2 * kPi * 440 * t) + 4000 * std::sin(2 * kPi * 3100 * t) + static_cast<double>(zufall() % 801) - 400.0;
                ton.pcm[i * static_cast<std::size_t>(kan)] = static_cast<std::int16_t>(std::lround(v));
                if (kan == 2) { ton.pcm[i * 2 + 1] = static_cast<std::int16_t>(std::lround(-0.5 * v)); }
            }
            RoqKodierer kt;
            RoqOptionen ot;
            ot.fps = fps;
            ot.guete = 40;
            (void)kt.beginne(16, 16, ot, &ton);
            for (int i = 0; i < bilder; ++i) { kt.bild(probe(16, 16, i)); }
            const std::string mitTon = kt.fertig();
            int kanaele = 0;
            const std::vector<std::int16_t> zurueck = roqTonDekodieren(mitTon, &kanaele);
            double sig = 0, fe = 0;
            int maxF = 0;
            const std::size_t n = std::min(zurueck.size() / 2, proben);
            for (std::size_t i = 0; i < n; ++i) {
                for (int c = 0; c < 2; ++c) {
                    const int soll = ton.pcm[i * static_cast<std::size_t>(kan) + static_cast<std::size_t>(kan == 2 ? c : 0)];
                    const int ist = zurueck[i * 2 + static_cast<std::size_t>(c)];
                    sig += static_cast<double>(soll) * soll;
                    fe += static_cast<double>(ist - soll) * (ist - soll);
                    maxF = std::max(maxF, std::abs(ist - soll));
                }
            }
            const double snr = 10.0 * std::log10(sig / std::max(1.0, fe));
            // Lage der Stuecke: Vorlauf vor ROQ_QUAD_INFO, dann nach jedem Bild bis (Bild + 1 + 8) * 22050 / fps
            std::size_t p = 8, bisher = 0;
            int bild = 0;
            bool lageOk = true, erstesTon = false;
            while (p + 8 <= mitTon.size()) {
                const auto u = [&](std::size_t i) { return static_cast<unsigned>(static_cast<unsigned char>(mitTon[i])); };
                const unsigned id = u(p) | u(p + 1) << 8;
                const std::size_t g = u(p + 2) | u(p + 3) << 8 | u(p + 4) << 16;
                if (p == 8) { erstesTon = id == static_cast<unsigned>(kan == 1 ? kRoqTonMono : kRoqTonStereo) && g > 0; }
                if (id == kRoqQuadVq) { ++bild; }
                if ((id == kRoqTonMono || id == kRoqTonStereo) && g > 0) {
                    bisher += g / static_cast<std::size_t>(kan);
                    const std::size_t soll = std::min(proben, static_cast<std::size_t>(bild + RoqKodierer::kTonVorlauf) * kRoqTonRate / fps);
                    lageOk = lageOk && bisher == soll && g <= 16384;
                }
                p += 8 + g;
            }
            std::printf("menueroq: Ton %s: %zu Proben zurueck, Rauschabstand %.1f dB, groesster Fehler %d\n", kan == 1 ? "mono" : "stereo", n, snr, maxF);
            pruefe(kanaele == kan && n == proben, std::string("Ton-Rundlauf ") + (kan == 1 ? "mono" : "stereo") + ": alle Proben (ZA_SOUND_" +
                                                      (kan == 1 ? "MONO" : "STEREO") + ")");
            pruefe(snr > 40.0, std::string("Ton-Rundlauf ") + (kan == 1 ? "mono" : "stereo") + ": Rauschabstand > 40 dB (DPCM sqrTable)");
            pruefe(erstesTon && lageOk, std::string("Ton-Lage ") + (kan == 1 ? "mono" : "stereo") + ": Vorlauf vor QUAD_INFO, dann je Bild wie ja01.roq");
            // das Bild bleibt abspielbar (Ton vor ROQ_QUAD_INFO)
            RoqSpieler sp;
            pruefe(sp.start(mitTon, 0, false) && (sp.lauf(0), sp.lauf(1000 / fps + 1), sp.bild() != nullptr && sp.breite() == 16),
                   "RoQ mit Ton: Bilder kommen (Spieler ueberspringt den Ton wie CIN_silent)");
        }
    }
    // --- M4 Punkt 5c: Codec-Gruppen fuer die Meldung ------------------------------------------
    {
        const auto fcc = [](const char* t) {
            return static_cast<std::uint32_t>(static_cast<unsigned char>(t[0])) | static_cast<std::uint32_t>(static_cast<unsigned char>(t[1])) << 8 |
                   static_cast<std::uint32_t>(static_cast<unsigned char>(t[2])) << 16 | static_cast<std::uint32_t>(static_cast<unsigned char>(t[3])) << 24;
        };
        pruefe(codecGruppe(fcc("HEVC")) == CodecGruppe::Hevc && codecGruppe(fcc("hvc1")) == CodecGruppe::Hevc, "Codec: HEVC");
        pruefe(codecGruppe(fcc("AV01")) == CodecGruppe::Av1, "Codec: AV1");
        pruefe(codecGruppe(fcc("VP80")) == CodecGruppe::Vp8 && codecGruppe(fcc("VP90")) == CodecGruppe::Vp9, "Codec: VP8/VP9");
        pruefe(codecGruppe(fcc("XVID")) == CodecGruppe::DivxXvid && codecGruppe(fcc("DX50")) == CodecGruppe::DivxXvid, "Codec: DivX/Xvid");
        pruefe(codecGruppe(fcc("H264")) == CodecGruppe::Unbekannt && fourccText(fcc("ZZ9Z")) == "ZZ9Z", "Codec: anderes, FourCC als Text");
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
            const std::string wieder = kodiereRoq(echt, oe, VideoZiel{});
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
