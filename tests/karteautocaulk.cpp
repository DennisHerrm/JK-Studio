// karteautocaulk.cpp - Karten-Editor Stufe 3 Punkt 6: AutoCaulk Selected (F4)
// ohne Oberflaeche (bhed/karte/auswahlbefehle.h, bhed/karte/kartenformat.h).
//
// Geprueft wird gegen NRC 20260114 (radiant/map.cpp map_autocaulk_selected):
//   - DeselectTriggers: gewaehlte trigger_* (Behaelter) samt Kindern ab;
//   - Map_Traverse_Selected: die Pinsel der Auswahl in Kartenfolge, die Kinder
//     ganz gewaehlter Entitaeten mit, Patches nicht;
//   - die .map fuer q3map2: worldspawn mit den Pinseln (ohne Kommentare), dem
//     Kasten (ConstructRegionBrushes), info_player_start am Auge ("%g"), die
//     gewaehlten Punktentitaeten ueber writeGraph, am Ende das '\n' des ersten
//     TokenWriter;
//   - <Karte>_ac.caulk lesen (Tokeniser) und anwenden: je beitragende Flaeche
//     ein Zeichen, die Shader von GetCommonShader, Rueckgaengig
//     "AutoCaulk N faces".
//
// Aufruf: karteautocaulk
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/region.h"
#include "bhed/karte/texdef.h"

#include <cstdio>
#include <memory>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::shared_ptr<Pinsel> quader(Karte& k, const std::shared_ptr<Entitaet>& ent, const Vektor3f& mitte, float halb, const std::string& shader) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, Vektor3f(halb, halb, halb)), shader, proj, k.kontext);
    ent->kindEinfuegen(p);
    return p;
}

std::size_t zaehle(const std::string& t, const std::string& was) {
    std::size_t n = 0;
    for (std::size_t p = 0; (p = t.find(was, p)) != std::string::npos; p += was.size()) { ++n; }
    return n;
}

}  // namespace

int main() {
    Karte k;
    const auto welt = k.weltHolenOderAnlegen();
    const auto a = quader(k, welt, Vektor3f(0, 0, 0), 32, "textures/a/a");
    const auto b = quader(k, welt, Vektor3f(128, 0, 0), 32, "textures/a/b");
    const auto c = quader(k, welt, Vektor3f(256, 0, 0), 32, "textures/a/c");   // nicht gewaehlt
    auto trigger = k.neueEntitaet("trigger_multiple", true);
    k.entitaetEinfuegen(trigger);
    const auto t = quader(k, trigger, Vektor3f(0, 128, 0), 16, "textures/a/t");
    auto tuer = k.neueEntitaet("func_door", true);
    k.entitaetEinfuegen(tuer);
    const auto d = quader(k, tuer, Vektor3f(0, -128, 0), 16, "textures/a/d");
    auto licht = k.neueEntitaet("light", false);
    licht->setzen("origin", "0 0 96");
    k.entitaetEinfuegen(licht);
    auto null = k.neueEntitaet("info_null", false);   // nicht gewaehlt
    null->setzen("origin", "8 8 8");
    k.entitaetEinfuegen(null);
    k.auswahl.waehlen(welt, a);
    k.auswahl.waehlen(welt, b);
    k.auswahl.waehlen(trigger, t);
    k.auswahl.waehlen(tuer);
    k.auswahl.waehlen(licht);

    // --- DeselectTriggers
    triggerAbwaehlen(k);
    pruefe(!k.auswahl.gewaehlt(trigger.get(), t.get()) && !k.auswahl.gewaehlt(trigger.get()) && k.auswahl.gewaehlt(welt.get(), a.get()) &&
               k.auswahl.gewaehlt(tuer.get()) && k.auswahl.gewaehlt(licht.get()),
           "DeselectTriggers: der Pinsel des trigger_multiple ab, der Rest bleibt");

    // --- Map_Traverse_Selected
    const auto pinsel = autocaulkPinsel(k);
    pruefe(pinsel.size() == 3 && pinsel[0] == a && pinsel[1] == b && pinsel[2] == d,
           "Map_Traverse_Selected: a, b (worldspawn), d (Kind der ganz gewaehlten Tuer); c und t nicht");
    pruefe(autocaulkPunktEntitaet(k, *licht) && !autocaulkPunktEntitaet(k, *null) && !autocaulkPunktEntitaet(k, *tuer),
           "traverse_selected_point_entities: nur das gewaehlte Licht");

    // --- die .map fuer q3map2 -autocaulk
    std::array<std::shared_ptr<Pinsel>, 6> kasten;
    std::array<Pinsel*, 6> roh{};
    std::array<const Pinsel*, 6> konst{};
    for (std::size_t i = 0; i < 6; ++i) {
        kasten[i] = std::make_shared<Pinsel>();
        roh[i] = kasten[i].get();
        konst[i] = kasten[i].get();
    }
    regionPinselBauen(roh, Vektor3f(-1056, -1168, -1072), Vektor3f(1312, 1168, 1120), regionKastenShader("", nullptr), k.kontext);
    const std::vector<const Pinsel*> liste = {a.get(), b.get(), d.get()};
    const std::string text = autocaulkKarteSchreiben(k, liste, konst, Vektor3f(32, 16.5F, -8), [&](const Entitaet& e) { return autocaulkPunktEntitaet(k, e); });
    pruefe(text.rfind("{\n\"classname\" \"worldspawn\"\n{\n( ", 0) == 0, "Karte: worldspawn, dann gleich der erste Pinsel (ohne // brush):\n" + text.substr(0, 80));
    pruefe(zaehle(text, "\n{\n( ") == 9 && zaehle(text, "// brush") == 0, "Karte: 3 Pinsel und 6 Kastenpinsel");
    pruefe(text.find(" a/c ") == std::string::npos && text.find(" a/t ") == std::string::npos && text.find(" a/d ") != std::string::npos,
           "Karte: c und t nicht, d schon (Shadernamen ohne textures/)");
    pruefe(text.find("\n}\n}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"32 16.5 -8\"\n}\n") != std::string::npos,
           "Karte: worldspawn zu, info_player_start am Auge (%g)");
    pruefe(text.find("// entity 0\n{\n\"classname\" \"light\"\n\"origin\" \"0 0 96\"\n}\n") != std::string::npos && text.find("info_null") == std::string::npos,
           "Karte: das gewaehlte Licht ueber writeGraph (// entity 0), info_null nicht");
    pruefe(text.size() > 3 && text.compare(text.size() - 3, 3, "}\n\n") == 0, "Karte: am Ende das ausstehende '\\n' des ersten TokenWriter");

    // --- <Karte>_ac.caulk lesen
    const auto m = caulkLesen("0 cc-c-c\n2 N-w-ls  rest\r\n  5 x\n7\n");
    pruefe(m.size() == 3 && m.at(0) == "cc-c-c" && m.at(2) == "N-w-ls" && m.at(5) == "x", "caulkLesen: Nummer, Zeichen, Rest der Zeile weg; ohne Zeichen Schluss");

    // --- anwenden
    const std::size_t n = caulkAnwenden(k, m);
    const auto shader = [](const std::shared_ptr<Pinsel>& p, std::size_t i) { return p->flaechen()[i]->shader(); };
    pruefe(n == 8, "caulkAnwenden: 4 Flaechen an a, 4 an d (x an Pinsel 5 gibt es nicht): " + std::to_string(n));
    pruefe(shader(a, 0) == "textures/system/caulk" && shader(a, 1) == "textures/system/caulk" && shader(a, 2) == "textures/a/a" &&
               shader(a, 3) == "textures/system/caulk" && shader(a, 4) == "textures/a/a" && shader(a, 5) == "textures/system/caulk",
           "caulkAnwenden: a nach \"cc-c-c\" (GetCommonShader: textures/system/caulk)");
    pruefe(shader(d, 0) == "textures/system/nodraw_solid" && shader(d, 1) == "textures/a/d" && shader(d, 2) == "textures/system/caulk_water" &&
               shader(d, 4) == "textures/system/caulk_lava" && shader(d, 5) == "textures/system/caulk_acid",
           "caulkAnwenden: d nach \"N-w-ls\" (Namen aus system.shader von JA: nodraw_solid, caulk_water, caulk_lava, caulk_acid)");
    pruefe(shader(b, 0) == "textures/a/b", "caulkAnwenden: b ohne Zeile bleibt");
    pruefe(k.undo.undoName() == "AutoCaulk 8 faces", "Rueckgaengig \"AutoCaulk 8 faces\": " + k.undo.undoName());
    k.undo.undo();
    pruefe(shader(a, 0) == "textures/a/a" && shader(d, 0) == "textures/a/d", "Rueckgaengig: die alten Shader");
    (void)c;

    std::printf("karteautocaulk: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
