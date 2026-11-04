// modellsetzentest.cpp - rc609 M0: vorhandene Modelle in die Karte (bhed/modellsetzen.h)
//
//   Klassen     JKA/MD mit und ohne Quelle, Jedi Outcast ohne misc_model_static (mit Grund)
//   Lage        geschriebene Schluessel lesen SP (in jeder Reihenfolge), MD9 und q3map2 gleich
//   Boden       Unterkante des gedrehten, skalierten Kastens auf dem Boden
//   Browser     Spielordner wie FS_FOpenFileRead (spaetere zuerst, .pk3 vor losen, fs_dirbeforepak), Suche
//   Live        der Stand einer Datei aendert sich mit einer neuen Fassung
//   md3         die Grenzen von Bild 0 aus dem Kopf (R_ModelBounds)
#include "bhed/md3.h"
#include "bhed/modellsetzen.h"
#include "bhed/pk3.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::modellsetzen;
namespace fs = std::filesystem;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}

bool nah(float a, float b, float eps = 1e-3F) { return std::fabs(a - b) <= eps; }
bool gleich(const Lage& a, const Lage& b) {
    for (int i = 0; i < 3; ++i) {
        // Winkel modulo 360
        const float d = std::fmod(std::fabs(a.winkel[i] - b.winkel[i]), 360.0F);
        if (std::min(d, 360.0F - d) > 1e-3F || !nah(a.skala[i], b.skala[i])) { return false; }
    }
    return true;
}
std::string text(const Lage& l) {
    char t[160];
    std::snprintf(t, sizeof(t), "(%g %g %g | %g %g %g)", static_cast<double>(l.winkel[0]), static_cast<double>(l.winkel[1]),
                  static_cast<double>(l.winkel[2]), static_cast<double>(l.skala[0]), static_cast<double>(l.skala[1]), static_cast<double>(l.skala[2]));
    return t;
}

// Schluessel wie ein Editor sie setzt: vorhandene ueberschreiben, "" entfernt, neue hinten an.
void setzen(Schluessel& keys, const Schluessel& neu) {
    for (const auto& [k, v] : neu) {
        bool da = false;
        for (auto it = keys.begin(); it != keys.end();) {
            if (it->first == k) {
                if (v.empty()) {
                    it = keys.erase(it);
                    continue;
                }
                it->second = v;
                da = true;
            }
            ++it;
        }
        if (!da && !v.empty()) { keys.emplace_back(k, v); }
    }
}

void schreib(const fs::path& p, const std::string& inhalt) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary);
    f << inhalt;
}

// Ein md3 ohne Flaechen: Kopf, ein Bild mit Grenzen (md3Frame_t).
std::string md3Kopf(const float g[2][3]) {
    std::string b(108 + 56, '\0');
    std::memcpy(b.data(), "IDP3", 4);
    const auto i32 = [&b](std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); };
    i32(4, 15);
    i32(76, 1);     // numFrames
    i32(80, 0);     // numTags
    i32(84, 0);     // numSurfaces
    i32(92, 108);   // ofsFrames
    i32(96, 164);   // ofsTags
    i32(100, 164);  // ofsSurfaces
    i32(104, 164);  // ofsEnd
    for (int r = 0; r < 2; ++r) {
        for (int k = 0; k < 3; ++k) { std::memcpy(b.data() + 108 + static_cast<std::size_t>(r * 3 + k) * 4, &g[r][k], 4); }
    }
    return b;
}

}  // namespace

int main(int argc, char** argv) {
    // --- Klassen ----------------------------------------------------------------------------------------
    {
        const auto ja = modellKlassen(kRegelwerkOpenJK, true);
        pruefe(ja.size() == 2 && ja[0].klasse == "misc_model" && ja[0].da && ja[1].klasse == "misc_model_static" && ja[1].da,
               "JKA mit Quelle: misc_model und misc_model_static");
        const auto jaFertig = modellKlassen(kRegelwerkOpenJK, false);
        pruefe(!jaFertig[0].da && !jaFertig[0].grund.empty() && jaFertig[1].da, "JKA ohne Quelle: misc_model nicht (Grund: " + jaFertig[0].grund + ")");
        const auto md = modellKlassen(kRegelwerkMovieDuels, false);
        pruefe(md[1].da && md[1].quelle.find("MD9") != std::string::npos, "Movie Duels: misc_model_static mit Quelle " + md[1].quelle);
        const auto jk2 = modellKlassen(kRegelwerkJediOutcast, true);
        pruefe(jk2[0].da && !jk2[1].da && jk2[1].grund.find("codeJK2") != std::string::npos, "Jedi Outcast: kein misc_model_static (" + jk2[1].grund + ")");
        pruefe(vorgabeKlasse(kRegelwerkOpenJK, true) == "misc_model" && vorgabeKlasse(kRegelwerkOpenJK, false) == "misc_model_static" &&
                   vorgabeKlasse(kRegelwerkJediOutcast, true) == "misc_model" && vorgabeKlasse(kRegelwerkJediOutcast, false).empty(),
               "Vorgabe: mit Quelle misc_model, ohne misc_model_static, Jedi Outcast ohne Quelle keine");
    }

    // --- Lage --------------------------------------------------------------------------------------------
    {
        const Lage proben[] = {
            Lage{{0, 0, 0}, {1, 1, 1}},       Lage{{0, 90, 0}, {1, 1, 1}},   Lage{{0, 270, 0}, {2, 2, 2}},
            Lage{{15, 30, 45}, {1, 1, 1}},    Lage{{0, 45, 10}, {0.5F, 0.5F, 0.5F}}, Lage{{0, 135, 0}, {1, 2, 3}},
            Lage{{350, 10, 0}, {1.25F, 1, 1}},
        };
        // Vorher stehen widerspruechliche Schluessel (beide Winkel, beide Groessen) - danach muessen alle gleich lesen.
        const Schluessel alt[] = {
            {},
            {{"angles", "0 10 0"}, {"angle", "77"}, {"modelscale", "3"}, {"modelscale_vec", "1 2 3"}},
            {{"angle", "77"}, {"angles", "5 10 15"}, {"modelscale_vec", "4 4 4"}},
        };
        int n = 0;
        int ok = 0;
        for (const Schluessel& vorher : alt) {
            for (const Lage& l : proben) {
                Schluessel k = vorher;
                k.insert(k.begin(), {"classname", "misc_model_static"});
                setzen(k, lageSchluessel(l));
                const Lage sp = lageWieSpiel(k, false);
                const Lage md = lageWieSpiel(k, true);
                const Lage q3 = lageWieQ3map2(k);
                Schluessel umgekehrt(k.rbegin(), k.rend());
                const Lage spU = lageWieSpiel(umgekehrt, false);
                int winkel = 0;
                int groesse = 0;
                for (const auto& [a, b] : k) {
                    winkel += (a == "angle" || a == "angles") ? 1 : 0;
                    groesse += (a == "modelscale" || a == "modelscale_vec") ? 1 : 0;
                }
                ++n;
                const bool gut = gleich(sp, l) && gleich(md, l) && gleich(q3, l) && gleich(spU, l) && winkel <= 1 && groesse <= 1;
                ok += gut ? 1 : 0;
                if (!gut) {
                    std::printf("     soll %s SP %s MD %s q3map2 %s umgekehrt %s, %d Winkel-, %d Groessenschluessel\n", text(l).c_str(), text(sp).c_str(),
                                text(md).c_str(), text(q3).c_str(), text(spU).c_str(), winkel, groesse);
                }
            }
        }
        pruefe(ok == n, "Lage: " + std::to_string(ok) + " von " + std::to_string(n) + " gleich in SP, MD9, q3map2 und umgekehrter Reihenfolge, je ein Schluessel");
        // Warum nur einer: beide Winkel lesen SP (der letzte) und q3map2 (angles) verschieden.
        const Schluessel beide = {{"angles", "0 10 0"}, {"angle", "77"}};
        pruefe(!nah(lageWieSpiel(beide, false).winkel[1], lageWieQ3map2(beide).winkel[1]), "Gegenprobe: angle und angles zugleich lesen SP (77) und q3map2 (10) verschieden");
        const Schluessel kurz = {{"modelscale_vec", "2 2"}};
        pruefe(nah(lageWieSpiel(kurz, true).skala[0], 1.0F) && nah(lageWieSpiel(kurz, false).skala[0], 2.0F), "MD9: modelscale_vec mit zwei Zahlen -> 1 1 1, OpenJK nimmt 2 2 0");
        const Schluessel lg = lageSchluessel(Lage{{0, 90, 0}, {2, 2, 2}});
        bool angle = false;
        bool scale = false;
        for (const auto& [a, b] : lg) {
            angle = angle || (a == "angle" && b == "90");
            scale = scale || (a == "modelscale" && b == "2");
        }
        pruefe(angle && scale, "Gier allein -> \"angle\" \"90\", gleichmaessig -> \"modelscale\" \"2\"");
    }

    // --- Boden und Kasten ----------------------------------------------------------------------------------
    {
        const float mn[3] = {-8, -16, -10};
        const float mx[3] = {8, 16, 30};
        pruefe(nah(ursprungZAufBoden(100.0F, mn, mx, Lage{}), 110.0F), "Boden: Unterkante -10 -> Ursprung 10 ueber dem Boden");
        pruefe(nah(ursprungZAufBoden(100.0F, mn, mx, Lage{{0, 0, 0}, {2, 2, 2}}), 120.0F), "Boden: Skala 2 -> 20 ueber dem Boden");
        // Nicken 90: axis[0] (vorn) zeigt nach unten; die x-Ausdehnung -8..8 steht senkrecht
        pruefe(nah(ursprungZAufBoden(0.0F, mn, mx, Lage{{90, 0, 0}, {1, 1, 1}}), 8.0F, 1e-2F), "Boden: Nicken 90 -> die Laengsachse steht, Ursprung 8 hoch");
        pruefe(nah(bodenHoehe(16.125F, 1.0F), 16.0F) && nah(bodenHoehe(10.25F, 0.5F), 10.0F),
               "Boden: endpos der Spur liegt SURFACE_CLIP_EPSILON / normale.z ueber der Ebene");
        float a0[3];
        float a1[3];
        const float o[3] = {100, 200, 0};
        weltKasten(mn, mx, Lage{{0, 90, 0}, {1, 1, 1}}, o, a0, a1);
        pruefe(nah(a0[0], 84) && nah(a1[0], 116) && nah(a0[1], 192) && nah(a1[1], 208) && nah(a0[2], -10) && nah(a1[2], 30),
               "Kasten: Gier 90 tauscht x und y (AnglesToAxis)");
    }

    // --- Browser -------------------------------------------------------------------------------------------
    const fs::path w = fs::path(argc > 1 ? argv[1] : ".") / "modellsetzen";
    std::error_code ec;
    fs::remove_all(w, ec);
    {
        const fs::path base = w / "GameData" / "base";
        const fs::path md = w / "GameData" / "MD";
        fs::create_directories(base, ec);
        fs::create_directories(md, ec);
        std::string f;
        const bool a = writePk3((base / "assets0.pk3").string(),
                                {{"models/map_objects/a/kiste.md3", "base-pk3"}, {"models/map_objects/a/stuhl.md3", "base-pk3"}, {"textures/x.jpg", "-"}}, &f);
        const bool b = writePk3((md / "zz.pk3").string(), {{"models/map_objects/b/Tisch.MD3", "md-pk3"}, {"models/players/x/model.glm", "glm"}}, &f);
        schreib(base / "models" / "map_objects" / "a" / "stuhl.md3", "base-lose");
        schreib(md / "models" / "map_objects" / "a" / "kiste.md3", "md-lose");
        schreib(md / "models" / "map_objects" / "c" / "lampe.ase", "md-lose-ase");
        schreib(md / "maps" / "x.md3", "kein Modellordner");
        std::vector<GamePath> o(2);
        (void)scanGamePath(base.string(), o[0]);
        (void)scanGamePath(md.string(), o[1]);
        pruefe(a && b && o[0].archives.size() == 1 && o[1].archives.size() == 1, "zwei Spielordner mit .pk3 und losen Dateien");
        const auto v = modellDateien(o, {".md3", ".ase"}, false);
        const auto finde = [&v](const std::string& p) -> const ModellDatei* {
            for (const ModellDatei& d : v) {
                if (d.pfad == p) { return &d; }
            }
            return nullptr;
        };
        const ModellDatei* kiste = finde("models/map_objects/a/kiste.md3");
        const ModellDatei* stuhl = finde("models/map_objects/a/stuhl.md3");
        const ModellDatei* tisch = finde("models/map_objects/b/Tisch.MD3");
        const ModellDatei* lampe = finde("models/map_objects/c/lampe.ase");
        pruefe(v.size() == 4, "4 Modelle (md3/ase unter models/, ohne .glm und maps/x.md3): " + std::to_string(v.size()));
        pruefe(kiste != nullptr && kiste->lose && kiste->herkunft.find("MD") != std::string::npos, "kiste.md3: die lose Datei in MD vor der .pk3 von base");
        pruefe(stuhl != nullptr && !stuhl->lose, "stuhl.md3: gleicher Ordner, .pk3 vor der losen Datei (fs_dirbeforepak 0)");
        pruefe(tisch != nullptr && !tisch->lose && lampe != nullptr && lampe->lose, "Tisch.MD3 (Endung gross) aus der .pk3, lampe.ase lose");
        const auto v1 = modellDateien(o, {".md3"}, true);
        bool stuhlLose = false;
        for (const ModellDatei& d : v1) { stuhlLose = stuhlLose || (d.pfad == "models/map_objects/a/stuhl.md3" && d.lose); }
        pruefe(stuhlLose && v1.size() == 3, "fs_dirbeforepak 1: stuhl.md3 lose; nur .md3: 3");
        pruefe(modellSuche(v, "A KISTE").size() == 1 && modellSuche(v, "map_objects").size() == 4 && modellSuche(v, "tisch md3").size() == 1 &&
                   modellSuche(v, "tisch ase").empty() && modellSuche(v, "").size() == 4,
               "Suche: alle Woerter, ohne Gross/klein");
        bool lose = false;
        const std::string f1 = fundort(o, "models/map_objects/a/kiste.md3", false, &lose);
        const bool l1 = lose;
        const std::string f2 = fundort(o, "MODELS/map_objects/a/stuhl.md3", false, &lose);
        const bool l2 = lose;
        const std::string f3 = fundort(o, "models/map_objects/a/stuhl.md3", true, &lose);
        pruefe(l1 && f1.find("MD") != std::string::npos && !l2 && f2.find("assets0.pk3") != std::string::npos && lose && f3.find("stuhl.md3") != std::string::npos &&
                   fundort(o, "models/gibtsnicht.md3", false).empty(),
               "Fundort wie FS_FOpenFileRead: lose in MD, .pk3 vor lose, fs_dirbeforepak 1 lose, fehlt leer");
    }

    // --- Live ----------------------------------------------------------------------------------------------
    {
        const fs::path p = w / "live" / "m.md3";
        schreib(p, "eins");
        const auto u = p.u8string();
        const std::string ps(u.begin(), u.end());
        const DateiStand s1 = dateiStand(ps);
        schreib(p, "zwei, laenger");
        const DateiStand s2 = dateiStand(ps);
        pruefe(s1.da && s2.da && !(s1 == s2) && s2.groesse == 13, "neue Fassung: anderer Stand (Groesse 4 -> 13)");
        pruefe(!dateiStand(ps + ".fehlt").da, "fehlende Datei: nicht da");
    }

    // --- md3: Grenzen von Bild 0 ------------------------------------------------------------------------------
    {
        const float g[2][3] = {{-12.5F, -3, 0}, {12.5F, 3, 47}};
        Md3Model m;
        std::string fehler;
        const bool ok = readMd3(md3Kopf(g), m, &fehler);
        pruefe(ok && m.kopfGrenzen && nah(m.grenzen[0][0], -12.5F) && nah(m.grenzen[1][2], 47.0F), "md3: Bild-0-Grenzen aus dem Kopf " + fehler);
    }

    fs::remove_all(w, ec);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "modellsetzen: alles gut" : "modellsetzen: FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
