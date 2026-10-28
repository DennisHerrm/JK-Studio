// leserfuzz.cpp - Stufe 8d Punkt 4: alle Datei-Leser mit kaputten Eingaben (ctest leser_fuzz).
//
// tests/fuzz.cpp prueft die ICARUS-Skripte, tests/kartefuzz.cpp Karten (.map/.bsp/.prt/.lin) -
// hier alle uebrigen Leser, die fremde Dateien aus .pk3-Archiven oder von der Platte lesen:
// pk3/zip, md3, glm, gla, skin, shader-Skripte und fogParms, ICARUS (.txt, .ibi), roff, NPC-,
// Saber-, weapons.dat-, items.dat- und animation.cfg-Dateien, .efx, Bilder, Ton, inflate und die
// BSP-Lumps der Vorschau (Geometrie, Entities, Kollision). Ein .menu-Leser existiert noch nicht
// (der Menue-Editor kommt erst).
//
// Je Leser N Mutanten (Vorgabe 300) aus Saatdateien: Bytes kippen, abschneiden, Muell einfuegen,
// Stuecke doppeln, 32-Bit-Felder auf Grenzwerte (0x7fffffff, 0x80000000, -1, 0x10000) setzen,
// Klammern und riesige Zahlen in Text. Forderung: kein Absturz, kein Haenger - unter ASan bzw.
// UBSan (Bauordner mit /fsanitize=address bzw. clang-cl -fsanitize=undefined) kein Treffer.
//
// Saat: data/fixtures und tests/karte/q3map2 (immer), dazu erzeugte Dateien (pk3, ibi, roff,
// npc, weapons.dat ...). Mit --spiel <base-Ordner> zusaetzlich bis zu 12 echte Dateien je Art aus
// den assets*.pk3 des Spiels (die Laeufe der Stufe 8d nutzen Jedi Academy).
//
//   leserfuzz <data/fixtures> <tests/karte/q3map2> [mutanten] [--spiel <base>] [--nur <leser>]
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "bhed/bsp.h"
#include "bhed/bspgeo.h"
#include "bhed/efx/io.h"
#include "bhed/gegenstaende.h"
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/ibi.h"
#include "bhed/image.h"
#include "bhed/inflate.h"
#include "bhed/kollision.h"
#include "bhed/md3.h"
#include "bhed/pk3.h"
#include "bhed/roff.h"
#include "bhed/scene.h"
#include "bhed/script.h"
#include "bhed/shaderscript.h"
#include "bhed/sound.h"

#include "absturz_melden.h"

namespace fs = std::filesystem;

namespace {

std::string lies(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

std::mt19937 g_zufall(8004);   // fest: jeder Lauf dieselben Mutanten
std::size_t zahl(std::size_t n) { return n == 0 ? 0 : static_cast<std::size_t>(g_zufall() % n); }

// Ein Mutant: 1 bis 6 Eingriffe, binaer und textlich gemischt.
std::string mutiere(std::string s) {
    const int eingriffe = 1 + static_cast<int>(zahl(6));
    for (int k = 0; k < eingriffe; ++k) {
        if (s.empty()) { s.push_back(static_cast<char>(zahl(256))); }
        const std::size_t at = zahl(s.size());
        switch (zahl(9)) {
        case 0: s[at] = static_cast<char>(zahl(256)); break;                          // Byte
        case 1: s[at] = static_cast<char>(s[at] ^ (1 << zahl(8))); break;             // Bit
        case 2: s.resize(at); break;                                                  // abschneiden
        case 3: s.erase(at, 1 + zahl(64)); break;                                     // loeschen
        case 4: s.insert(at, std::string(1 + zahl(32), static_cast<char>(zahl(256)))); break;
        case 5: {                                                                      // doppeln
            const std::size_t n = std::min<std::size_t>(1 + zahl(256), s.size() - at);
            s.insert(at, s.substr(at, n));
            break;
        }
        case 6: {                                                                      // 32-Bit-Grenzwert
            static const std::uint32_t werte[] = {0x7fffffffU, 0x80000000U, 0xffffffffU, 0x10000U, 0U, 0x40000000U, 1000000U};
            const std::uint32_t v = werte[zahl(std::size(werte))];
            const std::size_t a = (at / 4) * 4;
            if (a + 4 <= s.size()) { std::memcpy(s.data() + a, &v, 4); }
            break;
        }
        case 7: {                                                                      // Text: Zeichen der Syntax
            static const char* const teile[] = {"{", "}", "\"", "(", ")", "//", "/*", "\n", "$", "1e39", "-2147483648",
                                                "nan", "99999999999999999999", "\\", ";", "#"};
            s.insert(at, teile[zahl(std::size(teile))]);
            break;
        }
        default: s.insert(at, std::string(1 + zahl(4096), 'A')); break;              // langes Wort
        }
    }
    return s;
}

struct Leser {
    const char* name;
    std::vector<std::string> saat;
    std::function<void(const std::string&)> lesen;
};

// Ein kleines Zip (writePk3, unkomprimiert) - die Saat fuer den pk3-Leser.
std::string kleinesPk3(const fs::path& tmp) {
    const std::vector<bhed::Pk3Datei> d = {{"scripts/a.shader", "textures/a\n{\n}\n"},
                                           {"models/x/y.skin", "head,models/x/h.tga\n"},
                                           {"ext_data/npcs/a.npc", "a\n{\nplayerModel kyle\n}\n"}};
    const fs::path p = tmp / "saat.pk3";
    (void)bhed::writePk3(p.string(), d, nullptr);
    return lies(p);
}

}  // namespace

int main(int argc, char** argv) {
    absturz::einrichten();
    if (argc < 3) {
        std::fprintf(stderr, "Aufruf: leserfuzz <data/fixtures> <tests/karte/q3map2> [mutanten] [--spiel <base>] [--nur <leser>]\n");
        return 2;
    }
    const fs::path fix = argv[1];
    const fs::path karte = argv[2];
    int mutanten = 300;
    std::string spiel;
    std::string nur;
    std::string datei;   // --datei: eine gespeicherte Eingabe nachstellen
    for (int i = 3; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--spiel" && i + 1 < argc) {
            spiel = argv[++i];
        } else if (a == "--nur" && i + 1 < argc) {
            nur = argv[++i];
        } else if (a == "--datei" && i + 1 < argc) {
            datei = argv[++i];
        } else {
            mutanten = std::max(1, std::atoi(argv[i]));
        }
    }
    wchar_t tempW[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempW);
    const fs::path tmp = fs::path(tempW) / (L"jkstudio_leserfuzz_" + std::to_wstring(GetCurrentProcessId()));
    std::error_code ec;
    fs::create_directories(tmp, ec);

    // --- Saat nach Art (Dateiendung bzw. Name) ------------------------------------------
    std::map<std::string, std::vector<std::string>> saat;
    for (const fs::path& o : {fix, karte}) {
        for (const auto& e : fs::directory_iterator(o, ec)) {
            if (!e.is_regular_file()) { continue; }
            std::string ext = e.path().extension().string();
            for (char& c : ext) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            const std::string name = e.path().filename().string();
            saat[name == "animation.cfg" ? "animcfg" : ext].push_back(lies(e.path()));
        }
    }
    // Erzeugte Saat fuer Arten ohne Datei im Repo
    saat[".pk3"].push_back(kleinesPk3(tmp));
    saat[".npc"].push_back("kyle\n{\n\tplayerModel\tkyle\n\tweapon\tWP_SABER\n\thealth\t100\n\trank\tcaptain\n}\n"
                           "stormtrooper\n{\n\tplayerModel\tstormtrooper\n\tsurfOff\t\"helmet\"\n\tscale\t110\n}\n");
    saat[".sab"].push_back("single_1\n{\n\tname\t\"Saber\"\n\tsaberModel\tmodels/weapons2/saber_1/saber_1.glm\n\tsaberLength\t32\n}\n");
    saat["weapons"].push_back("{\n\tweapontype\tWP_BLASTER\n\tweaponmodel\tmodels/weapons2/blaster_r/blaster.md3\n\tmuzzleEffect\tblaster/muzzle_flash\n}\n");
    saat["items"].push_back("{\n\titemname\titem_medpak_instant\n\tclassname\titem_medpak_instant\n\tworldmodel\tmodels/items/medpac.md3\n\tquantity\t25\n}\n");
    saat[".rof"].push_back(std::string("ROFF\x02\0\0\0\x05\0\0\0\x32\0\0\0", 16) + std::string(5 * 24, '\x01'));
    saat[".efx"].push_back("repeatDelay 300\nParticle\n{\n\tname spark\n\tcount 4 6\n\tlife 200\n\tshaders\n\t[\n\t\tgfx/misc/spark\n\t]\n}\n");
    saat[".shader"].push_back("textures/a/fog\n{\n\tfogparms ( 0.5 0.5 0.5 ) 512\n\tsurfaceparm fog\n}\n");
    saat[".tga"].push_back(std::string("\0\0\x02\0\0\0\0\0\0\0\0\0\x02\0\x02\0\x20\x08", 18) + std::string(16, '\x7f'));
    saat[".wav"].push_back(std::string("RIFF\x24\0\0\0WAVEfmt \x10\0\0\0\x01\0\x01\0\x22\x56\0\0\x44\xac\0\0\x02\0\x10\0data\x08\0\0\0", 44) +
                           std::string(8, '\x10'));
    // Echte Dateien aus dem Spiel
    if (!spiel.empty()) {
        static const char* const arten[] = {".md3", ".glm", ".gla", ".skin", ".shader", ".npc", ".sab", ".ibi", ".rof",
                                            ".efx", ".jpg", ".tga", ".png", ".wav", ".mp3", ".bsp", ".cfg", ".dat"};
        for (const auto& e : fs::directory_iterator(spiel, ec)) {
            const std::string n = e.path().filename().string();
            if (n.rfind("assets", 0) != 0 || e.path().extension() != ".pk3") { continue; }
            bhed::Pk3 arch;
            if (!bhed::readPk3Directory(e.path().string(), arch, nullptr)) { continue; }
            for (const bhed::Pk3Entry& en : arch.entries) {
                std::string lname = en.name;
                for (char& c : lname) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                const std::size_t pkt = lname.rfind('.');
                if (pkt == std::string::npos || en.size > (8U << 20)) { continue; }
                std::string ext = lname.substr(pkt);
                if (lname.find("weapons.dat") != std::string::npos) { ext = "weapons"; }
                if (lname.find("items.dat") != std::string::npos) { ext = "items"; }
                if (lname.find("animation.cfg") != std::string::npos) { ext = "animcfg"; }
                if (std::find(std::begin(arten), std::end(arten), ext) == std::end(arten) && ext != "weapons" && ext != "items" &&
                    ext != "animcfg") {
                    continue;
                }
                auto& v = saat[ext];
                if (v.size() >= 12) { continue; }
                std::string inhalt;
                if (bhed::readPk3File(arch, en, inhalt, nullptr)) { v.push_back(std::move(inhalt)); }
            }
        }
    }
    const auto von = [&](std::initializer_list<const char*> arten) {
        std::vector<std::string> v;
        for (const char* a : arten) {
            for (const std::string& s : saat[a]) { v.push_back(s); }
        }
        if (v.empty()) { v.push_back(std::string(64, '\0')); }
        return v;
    };
    bhed::GlmModel glmVorlage;
    {
        const std::vector<std::string> g = von({".glm"});
        (void)bhed::readGlm(g.front(), glmVorlage);
    }

    std::vector<Leser> leser;
    leser.push_back({"pk3", von({".pk3"}), [&](const std::string& s) {
        const fs::path p = tmp / "m.pk3";
        {
            std::ofstream f(p, std::ios::binary | std::ios::trunc);
            f.write(s.data(), static_cast<std::streamsize>(s.size()));
        }
        bhed::Pk3 a;
        if (bhed::readPk3Directory(p.string(), a, nullptr)) {
            std::size_t n = 0;
            for (const bhed::Pk3Entry& e : a.entries) {
                std::string inhalt;
                (void)bhed::readPk3File(a, e, inhalt, nullptr);
                if (++n > 64) { break; }
            }
        }
    }});
    leser.push_back({"md3", von({".md3"}), [](const std::string& s) { bhed::Md3Model m; (void)bhed::readMd3(s, m); }});
    leser.push_back({"glm", von({".glm"}), [](const std::string& s) { bhed::GlmModel m; (void)bhed::readGlm(s, m); }});
    leser.push_back({"gla", von({".gla"}), [](const std::string& s) { bhed::GlaAnimation a; (void)bhed::readGla(s, a); }});
    leser.push_back({"skin", von({".skin"}), [&](const std::string& s) { bhed::GlmModel m = glmVorlage; bhed::applySkin(s, m); }});
    leser.push_back({"shader", von({".shader"}), [](const std::string& s) {
        bhed::ShaderMap m;
        bhed::parseShaderScript(s, m);
        bhed::NebelMap n;
        bhed::parseFogParms(s, n);
    }});
    leser.push_back({"icarus_txt", von({".txt"}), [](const std::string& s) {
        bhed::Script sc;
        std::vector<bhed::Diag> d;
        (void)bhed::readScript(s, sc, d);
    }});
    leser.push_back({"ibi", von({".ibi"}), [](const std::string& s) {
        std::vector<bhed::IbiBlock> b;
        std::vector<bhed::Diag> d;
        if (bhed::readIbi(s, b, d)) { (void)bhed::writeIbi(b); }
    }});
    leser.push_back({"roff", von({".rof"}), [](const std::string& s) { bhed::Roff r; (void)bhed::leseRoff(s, r); }});
    leser.push_back({"npc", von({".npc"}), [](const std::string& s) {
        bhed::NpcMap m;
        bhed::parseNpcFile(s, m, false);
        bhed::NpcMap m2;
        bhed::parseNpcFile(s, m2, true);
    }});
    leser.push_back({"saber", von({".sab"}), [](const std::string& s) { bhed::SaberMap m; bhed::parseSaberFile(s, m); }});
    leser.push_back({"weapons_dat", von({"weapons"}), [](const std::string& s) { bhed::WeaponMap m; bhed::parseWeaponsDat(s, m); }});
    leser.push_back({"items_dat", von({"items"}), [](const std::string& s) {
        (void)bhed::leseGegenstaende(s, false);
        (void)bhed::leseGegenstaende(s, true);
        (void)bhed::leseGegenstaende(s, false, true);
    }});
    leser.push_back({"animation_cfg", von({"animcfg"}), [](const std::string& s) { (void)bhed::parseAnimationCfg(s); }});
    leser.push_back({"efx", von({".efx"}), [](const std::string& s) { (void)bhed::efx::read(std::string_view(s)); }});
    leser.push_back({"bild", von({".tga", ".jpg", ".png"}), [](const std::string& s) {
        const auto* p = reinterpret_cast<const unsigned char*>(s.data());
        (void)bhed::image::decode(p, s.size());
        (void)bhed::image::decodeTga(p, s.size());
    }});
    leser.push_back({"ton", von({".wav", ".mp3"}), [](const std::string& s) {
        (void)bhed::sound::decode(reinterpret_cast<const unsigned char*>(s.data()), s.size());
    }});
    leser.push_back({"inflate", von({".pk3"}), [](const std::string& s) {
        if (s.size() < 4) { return; }
        const auto* p = reinterpret_cast<const unsigned char*>(s.data());
        (void)bhed::inflate::raw(p + 4, s.size() - 4, (static_cast<std::size_t>(p[0]) | (static_cast<std::size_t>(p[1]) << 8)) % (1U << 20));
    }});
    // Die BSP-Lumps der Vorschau, je Leser getrennt (welcher Teil haengt, steht dann im Namen)
    leser.push_back({"bsp_karte", von({".bsp"}), [](const std::string& s) { bhed::MapData m; (void)bhed::readBsp(s, m); }});
    leser.push_back({"bsp_geometrie", von({".bsp"}), [](const std::string& s) { bhed::BspGeometry g; (void)bhed::readBspGeometry(s, g); }});
    leser.push_back({"bsp_entities", von({".bsp"}), [](const std::string& s) {
        std::string text;
        if (bhed::readBspEntityLump(s, text, nullptr)) {
            std::vector<bhed::MapEntity> ents;
            bhed::parseEntities(text, ents);
        }
    }});
    leser.push_back({"bsp_kollision", von({".bsp"}), [](const std::string& s) { bhed::KollisionsKarte k; (void)bhed::leseKollision(s, k); }});
    leser.push_back({"ent_text", von({".ent"}), [](const std::string& s) {
        std::vector<bhed::MapEntity> ents;
        bhed::parseEntities(s, ents);
    }});

    int haenger = 0;
    // Haenger: laenger als 10 s je Eingabe (BHED_FUZZ_HAENGER_S aendert die Grenze)
    const double grenzeS = std::getenv("BHED_FUZZ_HAENGER_S") != nullptr ? std::atof(std::getenv("BHED_FUZZ_HAENGER_S")) : 10.0;
    std::size_t geprueft = 0;
    for (Leser& l : leser) {
        if (!nur.empty() && nur != l.name) { continue; }
        absturz::station() = l.name;
        if (!datei.empty()) {
            const std::string inhalt = lies(datei);
            const auto a = std::chrono::steady_clock::now();
            l.lesen(inhalt);
            std::printf("  %s: %s (%zu Bytes) %.2f s\n", l.name, datei.c_str(), inhalt.size(),
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - a).count());
            continue;
        }
        const auto t0 = std::chrono::steady_clock::now();
        double langsamste = 0.0;
        // Erst jede Saat unveraendert, dann die Mutanten
        for (const std::string& s : l.saat) { l.lesen(s); }
        for (int i = 0; i < mutanten; ++i) {
            const std::string m = mutiere(l.saat[zahl(l.saat.size())]);
            const auto a = std::chrono::steady_clock::now();
            l.lesen(m);
            const double d = std::chrono::duration<double>(std::chrono::steady_clock::now() - a).count();
            langsamste = std::max(langsamste, d);
            if (d > grenzeS) {
                ++haenger;
                std::printf("  HAENGER %s: Mutant %d brauchte %.1f s\n", l.name, i, d);
                // zum Nachstellen neben das Programm legen
                std::ofstream f(std::string("leserfuzz_haenger_") + l.name + "_" + std::to_string(i) + ".bin", std::ios::binary);
                f.write(m.data(), static_cast<std::streamsize>(m.size()));
            }
            ++geprueft;
        }
        const double gesamt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::printf("  ok    %-14s %3zu Saat, %d Mutanten, %.1f s (langsamster %.2f s)\n", l.name, l.saat.size(), mutanten, gesamt,
                    langsamste);
        std::fflush(stdout);
    }
    fs::remove_all(tmp, ec);
    std::printf("\nleserfuzz: %zu Mutanten, %d Haenger, kein Absturz\n", geprueft, haenger);
    return haenger == 0 ? 0 : 1;
}
