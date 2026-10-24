// kartefilter.cpp - Karten-Editor Stufe 2, Punkt 9: die Filter wie
// NetRadiant-Custom 20260114 (filters.cpp, brushmanip.cpp:983-1188,
// patchmanip.cpp:623-667, plugins/entity/entity.cpp:285-393).
//
// Sollwerte von Hand: filter_brush_all_faces filtert nur, wenn jede
// Flaeche passt (leere Pinsel nie), filter_brush_any_face bei einer;
// Shadernamen ohne Gross/klein (shader_equal), Entitaetsklassen mit
// (string_equal); EXCLUDE_ENT ist umgekehrt (alles ausser worldspawn,
// func_group, func_detail*); EXCLUDE_STRUCTURAL umgekehrt zu Detail.
#include "bhed/karte/filter.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/patchbefehle.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/texturen.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::shared_ptr<Pinsel> quader(Karte& k, const std::shared_ptr<Entitaet>& ent, const Vektor3f& mitte, const char* shader) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, Vektor3f(8, 8, 8)), shader, proj, k.kontext);
    ent->kindEinfuegen(p);
    return p;
}

void befehle() {
    const auto& b = filterBefehle();
    pruefe(b.size() == 22, "22 Filterbefehle fuer ja/jk2 (ohne Visportals)");
    pruefe(filterBefehl("FilterCaulk") != nullptr && filterBefehl("FilterCaulk")->maske == EXCLUDE_CAULK &&
               std::string(filterBefehl("FilterStructural")->taste) == "Ctrl+Shift+D" && filterBefehl("FilterVisportals") == nullptr,
           "FilterCaulk / Ctrl+Shift+D / kein FilterVisportals");
    unsigned int alle = 0;
    for (const auto& x : b) { alle |= x.maske; }
    pruefe(filterUmkehren(0) == alle && (alle & EXCLUDE_VISPORTALS) == 0 && (alle & EXCLUDE_TERRAIN) == 0, "Invert: alle registrierten Masken");
    pruefe(filterUmkehren(EXCLUDE_CAULK) == (alle & ~static_cast<unsigned int>(EXCLUDE_CAULK)), "Invert: Caulk aus, alles andere an");
}

void pinsel(const Kontext& kon) {
    Karte k;
    k.kontext = kon;
    const auto welt = k.weltHolenOderAnlegen();
    const auto caulk = quader(k, welt, Vektor3f(0, 0, 0), "textures/system/caulk");
    const auto halb = quader(k, welt, Vektor3f(100, 0, 0), "textures/a/b");
    halb->flaechen()[0]->ladeShader("textures/common/caulk_nodraw");   // shader_equal_n: Praefix
    const auto wasser = quader(k, welt, Vektor3f(200, 0, 0), "textures/a/b");
    wasser->flaechen()[2]->ladeShader("textures/liquids/water1");
    const auto hint = quader(k, welt, Vektor3f(300, 0, 0), "textures/a/b");
    hint->flaechen()[1]->ladeShader("TEXTURES/SYSTEM/HINT");
    const auto clip = quader(k, welt, Vektor3f(400, 0, 0), "textures/system/clip");
    const auto glas = quader(k, welt, Vektor3f(500, 0, 0), "textures/test/glas");
    const auto himmel = quader(k, welt, Vektor3f(600, 0, 0), "textures/a/b");
    himmel->flaechen()[3]->ladeShader("textures/test/himmel");
    const auto detail = quader(k, welt, Vektor3f(700, 0, 0), "textures/a/b");
    detail->setzeDetail(true);
    const auto leer = std::make_shared<Pinsel>();
    welt->kindEinfuegen(leer);

    pruefe(pinselGefiltert(*caulk, EXCLUDE_CAULK, k.kontext) && !pinselGefiltert(*caulk, EXCLUDE_CLIP, k.kontext), "Caulk: ganz caulk gefiltert");
    pruefe(!pinselGefiltert(*halb, EXCLUDE_CAULK, k.kontext) && flaecheGefiltert(*halb->flaechen()[0], EXCLUDE_CAULK, k.kontext) &&
               !flaecheGefiltert(*halb->flaechen()[1], EXCLUDE_CAULK, k.kontext),
           "Caulk: eine Caulk-Flaeche (Praefix) - nur die Flaeche gefiltert");
    pruefe(pinselGefiltert(*wasser, EXCLUDE_LIQUIDS, k.kontext), "Liquids: eine Flaeche textures/liquids/ reicht");
    pruefe(pinselGefiltert(*hint, EXCLUDE_HINTSSKIPS, k.kontext), "Hints: textures/system/hint ohne Gross/klein");
    pruefe(pinselGefiltert(*clip, EXCLUDE_CLIP, k.kontext), "Clips: surfaceparm playerclip (QER_CLIP) auf allen Flaechen");
    pruefe(pinselGefiltert(*glas, EXCLUDE_TRANSLUCENT, k.kontext) && !pinselGefiltert(*glas, EXCLUDE_CLIP, k.kontext), "Translucent: qer_trans");
    pruefe(pinselGefiltert(*himmel, EXCLUDE_SKY, k.kontext), "Sky: skyparms auf einer Flaeche");
    pruefe(pinselGefiltert(*detail, EXCLUDE_DETAILS, k.kontext) && !pinselGefiltert(*detail, EXCLUDE_STRUCTURAL, k.kontext) &&
               pinselGefiltert(*caulk, EXCLUDE_STRUCTURAL, k.kontext) && !pinselGefiltert(*caulk, EXCLUDE_DETAILS, k.kontext),
           "Details / Structural (umgekehrt)");
    pruefe(!pinselGefiltert(*leer, EXCLUDE_CAULK, k.kontext) && pinselGefiltert(*leer, EXCLUDE_STRUCTURAL, k.kontext),
           "leerer Pinsel: nie all_faces, Structural umgekehrt -> gefiltert");
    // UpdateFilters: Node::eFiltered und Face::m_filtered, sichtbar()
    filterAnwenden(k, EXCLUDE_CAULK);
    pruefe(caulk->gefiltert && !caulk->sichtbar() && !halb->gefiltert && halb->flaechen()[0]->gefiltert && !halb->flaechen()[1]->gefiltert,
           "filterAnwenden: Pinsel und Flaechen");
    filterAnwenden(k, 0);
    pruefe(!caulk->gefiltert && caulk->sichtbar() && !halb->flaechen()[0]->gefiltert, "filterAnwenden(0): alles wieder sichtbar");
}

void patches(const Kontext& kon) {
    Karte k;
    k.kontext = kon;
    patchVorlage(k, kastenAusMinMax(Vektor3f(0, 0, 0), Vektor3f(64, 64, 0)), "textures/system/clip", PatchVorlage::Ebene, 2);
    const auto p = std::static_pointer_cast<Patch>(k.welt()->kinder().front());
    pruefe(patchGefiltert(*p, EXCLUDE_CURVES, k.kontext) && patchGefiltert(*p, EXCLUDE_CLIP, k.kontext) &&
               !patchGefiltert(*p, EXCLUDE_CAULK, k.kontext),
           "Patch: Patches (alle), Clips (QER_CLIP), nicht Caulk");
}

void entitaeten() {
    Karte k;
    const auto welt = k.weltHolenOderAnlegen();
    const auto gruppe = k.neueEntitaet("func_group", true);
    const auto detail = k.neueEntitaet("func_detail_x", true);
    const auto licht = k.neueEntitaet("light", false);
    const auto lichtj = k.neueEntitaet("lightJunior", false);
    const auto ausl = k.neueEntitaet("trigger_multiple", true);
    const auto pfad = k.neueEntitaet("path_corner", false);
    const auto start = k.neueEntitaet("info_player_start", false);
    const auto statisch = k.neueEntitaet("func_static", true);
    statisch->setzen("model", "a");
    pruefe(entitaetGefiltert(*welt, EXCLUDE_WORLD) && entitaetGefiltert(*gruppe, EXCLUDE_WORLD) && entitaetGefiltert(*detail, EXCLUDE_WORLD) &&
               !entitaetGefiltert(*licht, EXCLUDE_WORLD),
           "World: worldspawn, func_group, func_detail*");
    pruefe(!entitaetGefiltert(*welt, EXCLUDE_ENT) && entitaetGefiltert(*licht, EXCLUDE_ENT) && entitaetGefiltert(*ausl, EXCLUDE_ENT),
           "Entities: alles ausser der Welt (umgekehrt)");
    pruefe(entitaetGefiltert(*gruppe, EXCLUDE_FUNC_GROUPS) && !entitaetGefiltert(*welt, EXCLUDE_FUNC_GROUPS), "FuncGroups");
    pruefe(entitaetGefiltert(*licht, EXCLUDE_LIGHTS) && !entitaetGefiltert(*lichtj, EXCLUDE_LIGHTS), "Lights: nur classname light");
    pruefe(entitaetGefiltert(*start, EXCLUDE_POINT_ENT) && !entitaetGefiltert(*lichtj, EXCLUDE_POINT_ENT) && !entitaetGefiltert(*ausl, EXCLUDE_POINT_ENT),
           "Point Entities: ohne light* und Gruppen");
    pruefe(entitaetGefiltert(*ausl, EXCLUDE_TRIGGERS) && entitaetGefiltert(*pfad, EXCLUDE_PATHS), "Triggers / Paths (Praefix)");
    pruefe(entitaetGefiltert(*statisch, EXCLUDE_MODELS), "Models: func_static mit model != name (filter_entity_doom3model)");
    pruefe(!entitaetGefiltert(*detail, EXCLUDE_DETAILS) && !entitaetGefiltert(*gruppe, EXCLUDE_STRUCTURAL),
           "brushtypes quake3: keine func_detail-Filter auf Entitaeten");
}

}  // namespace

int main() {
    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "bhed_kartefilter_test";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp / "shaders", ec);
    {
        std::ofstream f(tmp / "shaders" / "test.shader");
        f << "textures/system/clip\n{\n\tsurfaceparm playerclip\n\tsurfaceparm nodraw\n}\n"
             "textures/test/glas\n{\n\tqer_trans 0.5\n}\n"
             "textures/test/himmel\n{\n\tskyparms - 512 -\n}\n";
    }
    Dateisystem dfs;
    dfs.ordnerHinzufuegen(tmp.string(), "sp");
    Texturinventar inv;
    inv.laden(dfs);
    Kontext kon;
    kon.texturen = &inv;
    befehle();
    pinsel(kon);
    patches(kon);
    entitaeten();
    fs::remove_all(tmp, ec);
    std::printf("kartefilter: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
