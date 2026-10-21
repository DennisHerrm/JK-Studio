// kartetexturbrowser.cpp - Karten-Editor Stufe 1b, Punkt 8: der Texturbrowser
// ohne Oberflaeche (bhed/karte/texturbrowser.h) und die Shader-Befehle
// (Select_SetShader_Undo, Select Textured Objects/Surfaces, Find/Replace).
//
// Ein kleiner Spielordner wird im Temp-Verzeichnis gebaut. Geprueft werden
// die Rechnung von texwindow.cpp: getTextureWH (Fixed Size, Skala),
// TextureLayout::nextPos, evaluateHeight, Texture_IsShown (alle Schalter,
// Filter, common/), Texture_At, TextureBrowser_Focus, setOriginY/clamp,
// TextureBrowser_ShowDirectory ("Showing N shaders", ignorierte Endungen,
// ungueltige Namen), ShowDirectoryOfShader, die zuletzt gezeigten Ordner,
// Show All, Titel, Status und der Verzeichnisbaum (Gruppen mit '_').
#include "bhed/image.h"
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/texturbrowser.h"
#include "bhed/karte/texturkopie.h"
#include "bhed/karte/xyansicht.h"
#include "bhed/pk3.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string tga(int w, int h) {
    std::vector<unsigned char> rgba(static_cast<std::size_t>(w * h * 4), 128);
    const auto b = image::encodeTga(rgba.data(), w, h);
    return std::string(b.begin(), b.end());
}

void schreibe(const std::filesystem::path& p, const std::string& inhalt) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    f << inhalt;
}

std::shared_ptr<Pinsel> quader(Karte& k, const Vektor3f& mitte, const char* shader) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, Vektor3f(16, 16, 16)), shader, proj, k.kontext);
    k.weltHolenOderAnlegen()->kindEinfuegen(p);
    return p;
}

// --- Rechnung ohne Spielordner ------------------------------------------------------

void rechnung() {
    TexturBrowser tb;
    tb.breite = 400;
    tb.hoehe = 200;
    tb.schriftHoehe = 16;
    AktiverShader s;
    s.breite = 256;
    s.hoehe = 128;
    pruefe(tb.texturMass(s) == std::make_pair(128, 64), "getTextureWH: 50 % von 256 x 128");
    s.breite = 512;
    s.hoehe = 256;
    pruefe(tb.texturMass(s) == std::make_pair(160, 80), "Fixed Size: breiter als 160 -> 160 x 80");
    s.breite = 64;
    s.hoehe = 64;
    pruefe(tb.texturMass(s) == std::make_pair(48, 48), "Fixed Size: kleiner als 48 -> 48 x 48 (notex)");
    s.breite = 32;
    s.hoehe = 256;
    pruefe(tb.texturMass(s) == std::make_pair(16, 128), "hoeher als breit: 50 % = 16 x 128, Hoehe im Rahmen 48..160");
    tb.e.festeGroesse = false;
    s.breite = 1;
    s.hoehe = 1;
    pruefe(tb.texturMass(s) == std::make_pair(1, 1), "ohne Fixed Size: mindestens 1");
    tb.e.skala = 200;
    s.breite = 512;
    s.hoehe = 256;
    pruefe(tb.texturMass(s) == std::make_pair(1024, 512), "Skala 200 %");
    pruefe(texturSkalaAusIndex(3) == 100 && texturSkalaIndex(25) == 1 && texturSkalaIndex(33) == -1, "TextureScaleImport/Export");

    // TextureLayout::nextPos: 8 Rand, mindestens 96 + 8 je Textur, neue Zeile
    // bei breite - 8, Zeilenhoehe + Schrift + 1
    TexturBrowser lb;
    lb.breite = 300;
    lb.schriftHoehe = 16;
    TexturAnordnung a;
    const auto p1 = a.naechste(lb, 48, 48);
    const auto p2 = a.naechste(lb, 160, 80);
    const auto p3 = a.naechste(lb, 48, 48);
    pruefe(p1 == std::make_pair(8, -4) && p2 == std::make_pair(112, -4), "nextPos: 8 / -4, dann 8 + 96 + 8");
    pruefe(p3 == std::make_pair(8, -4 - 80 - 16 - 1), "nextPos: neue Zeile unter der hoechsten Textur");
    TexturAnordnung b;
    const auto q1 = b.naechste(lb, 500, 10);
    pruefe(q1 == std::make_pair(8, -4), "nextPos: die erste Textur einer Zeile bleibt, auch wenn sie zu breit ist");

    pruefe(shaderTexturName("textures/a/b") == "a/b" && pfadDateiname("textures/a/b") == "b" && shaderTexturName("x") == "",
           "shader_get_textureName, path_get_filename_start");

    // Verzeichnisbaum: Gruppen bis zum ersten '_'
    const auto baum = texturGruppenBaum({"bespin", "common", "gfx_a", "gfx_b", "kor_x", "nar", "nar_shaddar"});
    pruefe(baum.size() == 6 && baum[0].name == "bespin" && baum[0].pfad == "bespin" && baum[2].name == "gfx" && baum[2].pfad.empty()
               && baum[2].kinder.size() == 2 && baum[2].kinder[1].name == "gfx_b" && baum[3].name == "kor_x" && baum[3].kinder.empty()
               && baum[4].name == "nar" && baum[5].name == "nar_shaddar",
           "TextureGroups_constructTreeModel: gfx_a/gfx_b unter gfx, kor_x allein, nar und nar_shaddar getrennt");
}

// --- TexManipulator (surfacedialog.cpp) ----------------------------------------------

void texturKopieren() {
    // EPasteMode und die Rueckgaengig-Namen
    pruefe(einfuegeArtFuer(false, false) == EinfuegeArt::Nichts && einfuegeArtFuer(true, false) == EinfuegeArt::Werte &&
               einfuegeArtFuer(false, true) == EinfuegeArt::Nahtlos && einfuegeArtFuer(true, true) == EinfuegeArt::Projizieren,
           "pastemode_for_modifiers");
    pruefe(einfuegenMitShader(EinfuegeArt::Nichts, true) && !einfuegenMitShader(EinfuegeArt::Werte, true) &&
               einfuegenMitShader(EinfuegeArt::Werte, false),
           "pastemode_if_setShader");
    pruefe(texturAuftragenName(false, false, true) == "paintTexture" && texturAuftragenName(true, false, false) == "paintTexture,Values,LightPower" &&
               texturAuftragenName(true, false, true) == "paintTexDefValues" &&
               texturAuftragenName(false, true, false) == "paintTextureSeamless,LightColor" &&
               texturAuftragenName(false, true, true) == "paintTexDefValuesSeamless" &&
               texturAuftragenName(true, true, false) == "projectTexture,LightColor&Power" &&
               texturAuftragenName(true, true, true) == "projectTexDefValues",
           "Scene_applyClosestTexture_getUndoName");

    // Zwei Wuerfel nebeneinander, von oben (XY) gesehen
    Karte k;
    auto a = quader(k, Vektor3f(0, 0, 0), "textures/eins/a");
    auto b = quader(k, Vektor3f(64, 0, 0), "textures/eins/b");
    auto licht = k.neueEntitaet("light", false);
    licht->setzen("origin", "0 64 0");
    licht->setzen("light", "300");
    k.entitaetEinfuegen(licht);
    XyAnsicht v;
    v.typ = Sicht::XY;
    v.breite = 800;
    v.hoehe = 600;
    v.massstab = 1;
    v.ursprung = Vektor3f(32, 0, 0);
    Blick blick(false);
    blick.bauen(v.projektion(), v.modelview(), 800, 600);
    const auto geraet = [&v](float x, float y) {
        const Vektor3f q = projiziert(transformierterVektor4(multipliziert(v.projektion(), v.modelview()), Vektor4f(x, y, 0, 1)));
        return GeraeteVektor(q[0], q[1]);
    };
    const auto test = [&](float x, float y, Blick& g) {
        g = blick;
        auswahlTestBauen(g, auswahlKastenPunkt(geraet(x, y), GeraeteVektor(12.0F / 800, 12.0F / 600)));
    };
    // Werte der Oberseite von a etwas verschieben
    Flaeche* oben = nullptr;
    for (const auto& f : a->flaechen()) {
        if (f->ebene().normal()[2] > 0.5) { oben = f.get(); }
    }
    pruefe(oben != nullptr, "Oberseite gefunden");
    if (oben == nullptr) { return; }
    {
        TexturProjektion pr = oben->texdefHolen(k.kontext);
        pr.texdef.shift[0] = 16;
        pr.texdef.rotate = 45;
        oben->setzeTexdef(pr, k.kontext);
    }
    FlaechenTextur ablage;
    std::string shader;
    Blick g(false);
    test(0, 0, g);
    {
        AuswahlVolumen t(g);
        pruefe(naechsteTexturHolen(k, t, "textures/browser", shader, ablage) && shader == "textures/eins/a" &&
                   ablage.quelle == FlaechenTextur::Quelle::Pinsel && ablage.ebene.normal()[2] > 0.5 &&
                   ablage.projektion.texdef.shift[0] == 16 && ablage.projektion.texdef.rotate == 45,
               "M3 (kopieren): Shader und Werte der naechsten Flaeche (von oben die Oberseite)");
    }
    // Umschalt: Werte und Shader auf b
    test(64, 0, g);
    {
        AuswahlVolumen t(g);
        AuswahlEinstellung e;
        naechsteTexturAuftragen(k, e, t, true, false, false, true, shader, ablage);
    }
    Flaeche* obenB = nullptr;
    for (const auto& f : b->flaechen()) {
        if (f->ebene().normal()[2] > 0.5) { obenB = f.get(); }
    }
    pruefe(obenB != nullptr && obenB->shader() == "textures/eins/a" && obenB->texdefHolen(k.kontext).texdef.shift[0] == 16 &&
               obenB->texdefHolen(k.kontext).texdef.rotate == 45,
           "Umschalt+M3 (Werte): Shader und Texdef der Ablage auf der Oberseite von b");
    if (obenB == nullptr) { return; }
    // Alt+Umschalt: nur die Werte, Shader bleibt
    obenB->setzeShader("textures/eins/b", k.kontext);
    {
        TexturProjektion pr = obenB->texdefHolen(k.kontext);
        pr.texdef.shift[0] = 0;
        obenB->setzeTexdef(pr, k.kontext);
    }
    {
        AuswahlVolumen t(g);
        AuswahlEinstellung e;
        naechsteTexturAuftragen(k, e, t, true, false, true, false, shader, ablage);
    }
    pruefe(obenB->shader() == "textures/eins/b" && obenB->texdefHolen(k.kontext).texdef.shift[0] == 16,
           "Alt+Umschalt+M3: nur die Werte, der Shader bleibt");
    // Strg (nahtlos): gleiche Ebene -> projiziert; die Ablage folgt der Flaeche
    {
        AuswahlVolumen t(g);
        AuswahlEinstellung e;
        naechsteTexturAuftragen(k, e, t, false, true, false, false, shader, ablage);
    }
    pruefe(obenB->shader() == "textures/eins/a" && ablage.ebene.normal()[2] > 0.5, "Strg+M3 (nahtlos) auf gleicher Ebene: Shader gesetzt");
    // Licht: Umschalt setzt die Staerke, Strg die Farbe
    ablage.licht = 500;
    ablage.farbe = Vektor3f(1, 0.5F, 0);
    test(0, 64, g);
    {
        AuswahlVolumen t(g);
        AuswahlEinstellung e;
        naechsteTexturAuftragen(k, e, t, true, false, false, false, shader, ablage);
        pruefe(licht->wert("light") == "500" && !licht->hat("_color"), "Umschalt+M3 auf ein Licht: light = 500");
    }
    {
        AuswahlVolumen t(g);
        AuswahlEinstellung e;
        naechsteTexturAuftragen(k, e, t, false, true, false, false, shader, ablage);
        pruefe(licht->wert("_color") == "1 0.5 0", "Strg+M3 auf ein Licht: _color");
    }
    {
        AuswahlVolumen t(g);
        std::string s2;
        FlaechenTextur ab2;
        pruefe(naechsteTexturHolen(k, t, "textures/browser", s2, ab2) && s2 == "textures/browser" && ab2.licht == 500,
               "M3 auf ein Licht: Staerke in die Ablage, der Shader des Browsers bleibt");
    }
    // texturize_selected: Alt+M3 setzt den Browser-Shader auf die Auswahl
    k.auswahl.waehlen(k.welt(), a);
    test(64, 0, g);
    {
        AuswahlVolumen t(g);
        AuswahlEinstellung e;
        naechsteTexturAuftragen(k, e, t, false, false, true, true, "textures/zwei/x", ablage);
    }
    pruefe(a->flaechen()[0]->shader() == "textures/zwei/x" && obenB->shader() == "textures/zwei/x",
           "Alt+M3 mit Auswahl: die Auswahl und die Flaeche unter dem Zeiger bekommen den Shader");
    // Patch: kopieren
    Karte k2;
    auto pa = std::make_shared<Patch>();
    pa->setzeGroesse(3, 3);
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            pa->punkt(r, c).vertex = Vektor3f(static_cast<float>(c) * 32 - 32, static_cast<float>(r) * 32 - 32, 0);
            pa->punkt(r, c).texcoord = Vektor2f(static_cast<float>(c) * 0.5F, static_cast<float>(r) * 0.5F);
        }
    }
    pa->ladeShader("textures/patch/p");
    pa->punkteGeaendert();
    k2.weltHolenOderAnlegen()->kindEinfuegen(pa);
    XyAnsicht v2 = v;
    v2.ursprung = Vektor3f(0, 0, 0);
    Blick blick2(false);
    blick2.bauen(v2.projektion(), v2.modelview(), 800, 600);
    Blick g2 = blick2;
    auswahlTestBauen(g2, auswahlKastenPunkt(GeraeteVektor(0, 0), GeraeteVektor(12.0F / 800, 12.0F / 600)));
    {
        AuswahlVolumen t(g2);
        std::string s3;
        FlaechenTextur ab3;
        pruefe(naechsteTexturHolen(k2, t, "", s3, ab3) && s3 == "textures/patch/p" && ab3.quelle == FlaechenTextur::Quelle::Patch &&
                   ab3.patchBreite == 3 && ab3.patchPunkte.size() == 9,
               "M3 auf einen Patch: Shader und Kontrollpunkte (PatchData) in der Ablage");
    }
    {   // Projizieren auf den Patch: st aus der Projektion der Ablage
        FlaechenTextur ab4;
        ablageStandard(ab4, k2.kontext.art());
        ab4.ebene = Ebene3d(0, 0, 1, 0);
        AuswahlEinstellung e;
        AuswahlVolumen t(g2);
        const float vorher = pa->punkt(0, 0).texcoord[0];
        naechsteTexturAuftragen(k2, e, t, true, true, false, false, "textures/patch/q", ab4);
        pruefe(pa->shader() == "textures/patch/q" && pa->punkt(0, 0).texcoord[0] != vorher, "Strg+Umschalt+M3 auf den Patch: projiziert");
    }
}

}  // namespace

int main() {
    rechnung();
    texturKopieren();

    const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "behaved_kartetexturbrowser";
    std::error_code ec;
    std::filesystem::remove_all(tmp, ec);
    const std::filesystem::path base = tmp / "base";
    {
        std::vector<Pk3Datei> a0 = {
            {"textures/stone/a.tga", tga(256, 128)},
            {"textures/stone/b_glow.tga", tga(8, 8)},
            {"textures/stone/c.tga", tga(64, 64)},
            {"textures/common/caulk.tga", tga(64, 64)},
            {"textures/common/nodraw.tga", tga(64, 64)},
            {"textures/gfx_a/x.tga", tga(32, 32)},
            {"textures/gfx_b/y.tga", tga(32, 32)},
            {"shaders/stone.shader",
             "textures/stone/fancy\n{\n\tqer_editorimage textures/stone/a.tga\n}\n"
             "textures/stone/kaputt\n{\n\tqer_editorimage textures/stone/fehlt.tga\n}\n"
             "textures/common/clip\n{\n\tqer_editorimage textures/common/caulk.tga\n}\n"
             "textures/stone/sub/tief\n{\n}\n"},
        };
        std::filesystem::create_directories(base);
        pruefe(writePk3((base / "assets0.pk3").string(), a0), "Test-.pk3 geschrieben");
        schreibe(base / "textures" / "stone" / "mit leer.tga", tga(8, 8));
    }
    Dateisystem fs;
    fs.ordnerHinzufuegen(base.string(), "sp");
    Texturinventar inv;
    inv.laden(fs);

    // --- aktive Shader ----------------------------------------------------------------
    AktiveShader aktiv;
    const AktiverShader& fancy = aktiv.holen("textures/stone/fancy", &inv);
    pruefe(!fancy.standard && fancy.texturName == "textures/stone/a" && fancy.breite == 256 && fancy.hoehe == 128 && !fancy.bildFehlt,
           "Try_Shader_ForName: Shader mit Editorbild 256 x 128");
    const AktiverShader& kaputt = aktiv.holen("textures/stone/kaputt", &inv);
    pruefe(!kaputt.standard && kaputt.bildFehlt && kaputt.breite == 64, "Shader ohne Bild: shadernotex 64 x 64");
    const AktiverShader& leer = aktiv.holen("textures/", &inv);
    pruefe(leer.standard && leer.bildFehlt && leer.texturName == "textures/", "texdef_name_default: CreateDefault, notex");
    const std::size_t stand = aktiv.stand();
    (void)aktiv.holen("TEXTURES/STONE/FANCY", &inv);
    pruefe(aktiv.stand() == stand && aktiv.alle().size() == 3, "ein vorhandener Shader (ohne Gross/klein) wird nicht neu angelegt");
    pruefe(aktiv.alle().begin()->first == "textures/", "g_ActiveShaders sortiert: textures/ zuerst");

    // --- ShowDirectory ------------------------------------------------------------------
    TexturBrowser tb;
    tb.breite = 600;
    tb.hoehe = 300;
    tb.schriftHoehe = 16;
    std::vector<std::string> warnungen;
    const std::size_t n = tb.verzeichnisZeigen("stone/", aktiv, inv, fs, &warnungen);
    pruefe(n == 3, "Showing 3 shaders: fancy, kaputt, sub/tief (Praefix textures/stone/)");
    pruefe(warnungen.size() == 1 && warnungen[0] == "Skipping invalid texture name: [textures/stone/mit leer]",
           "TexturePath_loadTexture: ungueltiger Name gemeldet");
    pruefe(aktiv.finden("textures/stone/a") != nullptr && aktiv.finden("textures/stone/c") != nullptr
               && aktiv.finden("textures/stone/b_glow") == nullptr,
           "Bilder des Verzeichnisses geladen, _glow ignoriert");
    pruefe(tb.verzeichnis == "stone/" && tb.zuletzt.size() == 1 && tb.zuletzt[0] == "stone/" && tb.titel() == "Textures: stone/",
           "Verzeichnis, zuletzt gezeigt, Titel");
    // gezeigt: textures/stone/a, c, fancy, kaputt, sub/tief (textures/ nicht: Verzeichnis)
    auto lagen = tb.anordnen(aktiv);
    std::vector<std::string> namen;
    for (const auto& l : lagen) { namen.push_back(l.shader->name); }
    pruefe(namen == std::vector<std::string>{"textures/stone/a", "textures/stone/c", "textures/stone/fancy", "textures/stone/kaputt",
                                             "textures/stone/sub/tief"},
           "Texture_IsShown: Verzeichnis stone/ samt Unterordner, sortiert");
    // Lagen: a 128x64 bei 8, c 48x48 bei 144, fancy 128x64 bei 200, kaputt 48 bei 336, tief 48 bei 440;
    // 440 + 48 > 600 - 8? nein (488) -> alle in einer Zeile
    pruefe(lagen[0].x == 8 && lagen[1].x == 144 && lagen[2].x == 248 && lagen[3].x == 384 && lagen[4].x == 488 && lagen[4].y == -4,
           "Lagen der Zeile (je max(b, 96) + 8)");
    const int gesamt = tb.gesamtHoehe(aktiv);
    pruefe(gesamt == 4 + 16 + 64 + 4, "evaluateHeight: |y| + Schrift + hoechste Textur + 4");

    // Texture_At: in fancy (x 248..375, y -4-16 .. -4-16-64 GL), Fensterpunkt my = origin - y - 1
    const AktiverShader* t = tb.texturBei(260, 30, aktiv);
    pruefe(t != nullptr && t->name == "textures/stone/fancy", "Texture_At: Klick in fancy");
    pruefe(tb.texturBei(248, 30, aktiv) == nullptr, "Texture_At: mx > x (der linke Rand gehoert nicht dazu)");
    pruefe(tb.texturBei(260, 3, aktiv) == nullptr && tb.texturBei(260, 4, aktiv) != nullptr && tb.texturBei(260, 82, aktiv) != nullptr
               && tb.texturBei(260, 83, aktiv) == nullptr,
           "Texture_At: von der Namenszeile (Fensterpunkt 4) bis zur Unterkante (82)");

    // Schalter
    tb.e.shaderZeigen = false;
    tb.hoeheGeaendert();
    pruefe(tb.anordnen(aktiv).size() == 2, "Show Shaders aus: nur die Bilder a und c");
    tb.e.shaderZeigen = true;
    tb.e.texturenZeigen = false;
    pruefe(tb.anordnen(aktiv).size() == 3, "Show Textures aus: nur die drei Shader");
    tb.e.texturenZeigen = true;
    tb.e.ohneNotex = true;
    pruefe(tb.anordnen(aktiv).size() == 3, "Hide Image Missing: kaputt und sub/tief fehlen");
    tb.e.ohneNotex = false;
    tb.filter = "A";
    pruefe(tb.anordnen(aktiv).size() == 3, "Filter 'A' ohne Gross/klein irgendwo: a, fancy, kaputt");
    tb.e.vonAnfang = true;
    pruefe(tb.anordnen(aktiv).size() == 1, "Filter vom Anfang: nur a");
    tb.e.vonAnfang = false;
    tb.filter.clear();

    // in Gebrauch, Hide Unused
    std::set<std::string, ShaderKleiner> gebrauch{"Textures/Stone/C"};
    pruefe(aktiv.gebrauchSetzen(gebrauch) && aktiv.finden("textures/stone/c")->imGebrauch, "SetInUse ohne Gross/klein");
    tb.unbenutzteSetzen(true);
    pruefe(tb.anordnen(aktiv).size() == 1, "Hide Unused: nur c");
    tb.alleZeigen();
    pruefe(!tb.unbenutzteVerstecken && tb.verzeichnis.empty() && tb.titel() == "Textures: all" && tb.anordnen(aktiv).size() == 6,
           "Show All: alles (auch textures/), Titel 'all'");
    tb.alleZeigen();
    pruefe(tb.unbenutzteVerstecken, "Show All zweimal: nur benutzte");
    tb.unbenutzteSetzen(false);

    // Rahmen
    tb.shader = "textures/stone/FANCY";
    tb.breite = 600;
    tb.hoehe = 300;
    const auto sicht = tb.sichtbare(aktiv);
    bool rot = false;
    bool gruen = false;
    bool weiss = false;
    for (const auto& l : sicht) {
        if (l.shader->name == "textures/stone/fancy") { rot = l.rand == TexturLage::Rand::Gewaehlt && l.strichel; }
        if (l.shader->name == "textures/stone/c") { gruen = l.rand == TexturLage::Rand::ImGebrauch && !l.strichel; }
        if (l.shader->name == "textures/stone/kaputt") { weiss = l.rand == TexturLage::Rand::Shader && l.strichel; }
    }
    pruefe(rot && gruen && weiss, "Rahmen: gewaehlt rot, benutzt gruen, Shader weiss + Strichel");

    // common/: Bilder ohne Gebrauch versteckt, Shader nicht
    tb.verzeichnisZeigen("common/", aktiv, inv, fs);
    namen.clear();
    for (const auto& l : tb.anordnen(aktiv)) { namen.push_back(l.shader->name); }
    pruefe(namen == std::vector<std::string>{"textures/common/clip"}, "Hide nonShaders in common/: nur der Shader clip");
    tb.e.commonOhneTexturen = false;
    tb.hoeheGeaendert();
    pruefe(tb.anordnen(aktiv).size() == 3, "ohne die Einstellung: caulk, clip, nodraw");

    // Rollen und Fokus
    TexturBrowser kl;
    kl.breite = 130;
    kl.hoehe = 100;
    kl.schriftHoehe = 16;
    kl.e.commonOhneTexturen = false;
    kl.verzeichnisZeigen("stone/", aktiv, inv, fs);
    // je Zeile eine Textur: a (64), c (48), fancy (64), kaputt (48), tief (48)
    const int g2 = kl.gesamtHoehe(aktiv);
    pruefe(g2 == 4 + (64 + 17) + (48 + 17) + (64 + 17) + (48 + 17) + 16 + 48 + 4, "evaluateHeight ueber fuenf Zeilen");
    kl.setzeUrsprungY(-100000, aktiv);
    pruefe(kl.ursprungY(aktiv) == kl.hoehe - g2, "setOriginY klemmt nach unten (hoehe - gesamt)");
    kl.setzeUrsprungY(50, aktiv);
    pruefe(kl.ursprungY(aktiv) == 0, "setOriginY klemmt nach oben (0)");
    kl.fokus("textures/stone/kaputt", aktiv);
    // kaputt: y = -4 - 81 - 65 - 81 = -231; Hoehe 48 + 2*16 = 80; originy = y - 80 + 100 = -211
    pruefe(kl.ursprungY(aktiv) == -211, "TextureBrowser_Focus: die Textur ganz unten im Bild");
    kl.fokus("textures/stone/a", aktiv);
    pruefe(kl.ursprungY(aktiv) == 0, "TextureBrowser_Focus: nach oben (y + 4, geklemmt)");
    const auto r = kl.rollen(aktiv);
    pruefe(r.maximum == g2 - 100 && r.wert == 0 && r.seite == 100, "updateScroll: Maximum, Wert, Seite");

    // ShowDirectoryOfShader
    TexturBrowser ds;
    ds.breite = 600;
    ds.hoehe = 300;
    std::size_t anzahl = 0;
    pruefe(ds.verzeichnisDesShaders("textures/stone/sub/tief", aktiv, inv, fs, nullptr, &anzahl) && ds.verzeichnis == "stone/sub/"
               && anzahl == 1,
           "ShowDirectoryOfShader: Verzeichnis zwischen erstem und letztem '/'");
    pruefe(!ds.verzeichnisDesShaders("textures/", aktiv, inv, fs), "ShowDirectoryOfShader: ohne Verzeichnis nichts");

    // zuletzt: hoechstens 25, vorhandener wandert ans Ende
    TexturBrowser zl;
    for (int i = 0; i < 26; ++i) { zl.zuletztMerken("d" + std::to_string(i) + "/"); }
    zl.zuletztMerken("d5/");
    pruefe(zl.zuletzt.size() == 25 && zl.zuletzt.front() == "d1/" && zl.zuletzt.back() == "d5/", "trackRecentFolders: 25, wiederholter ans Ende");

    // Status und Gruppen
    pruefe(texturStatus("textures/stone/fancy", fancy) == "stone/fancy W: 256 H: 128" && texturStatus("x", fancy) == "x W: 256 H: 128",
           "TextureBrowser_SetStatus");
    (void)aktiv.holen("textures/eigen/ohne_definition", &inv);
    const auto gruppen = texturGruppen(aktiv, inv, false);
    pruefe(gruppen == std::vector<std::string>{"common", "eigen", "gfx_a", "gfx_b", "stone"},
           "TextureGroups_constructTreeView: Ordner, Shaderstufe, ohne Definition angeforderte");
    pruefe(texturGruppen(aktiv, inv, true) == std::vector<std::string>{"common", "eigen", "stone"}, "ShaderList Only: ohne Ordner");
    aktiv.leeren();
    pruefe(aktiv.alle().empty(), "FreeShaders");

    // --- Shader-Befehle ----------------------------------------------------------------
    {
        Karte k;
        auto p1 = quader(k, Vektor3f(0, 0, 0), "textures/stone/a");
        auto p2 = quader(k, Vektor3f(64, 0, 0), "textures/stone/c");
        auto welt = k.welt();
        AuswahlEinstellung e;
        pruefe(!shaderSetzen(k, e, "textures/x/y"), "Select_SetShader_Undo: ohne Auswahl nichts");
        k.auswahl.waehlen(welt, p1);
        pruefe(shaderSetzen(k, e, "textures/x/y") && k.undo.undoName() == "textureNameSetSelected"
                   && p1->flaechen()[0]->shader() == "textures/x/y" && p2->flaechen()[0]->shader() == "textures/stone/c",
               "Select_SetShader_Undo: alle Flaechen des gewaehlten Pinsels, Rueckgaengig textureNameSetSelected");
        k.undo.undo();
        pruefe(p1->flaechen()[0]->shader() == "textures/stone/a", "Rueckgaengig stellt den Shader wieder her");
        k.auswahl.leeren();
        k.auswahl.flaecheWaehlen(p2, p2->flaechen()[1]);
        pruefe(shaderSetzen(k, e, "textures/x/z") && p2->flaechen()[1]->shader() == "textures/x/z"
                   && p2->flaechen()[0]->shader() == "textures/stone/c",
               "Select_SetShader: gewaehlte Flaeche allein");
        k.auswahl.leeren();
        k.auswahl.flaechenLeeren();
        waehleNachShader(k, "TEXTURES/X/Z");
        pruefe(k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(welt.get(), p2.get()), "Select Textured Objects: der Pinsel mit der Flaeche");
        k.auswahl.leeren();
        waehleFlaechenNachShader(k, "textures/stone/c");
        pruefe(k.auswahl.flaechen().size() == 5 && k.auswahl.anzahl() == 0, "Select Textured Surfaces: fuenf Flaechen, kein Objekt");
        k.auswahl.flaechenLeeren();
        std::string fehler;
        pruefe(!texturenErsetzen(k, e, "stone/c", "textures/q", false, &fehler)
                   && fehler == "FindReplaceTextures: invalid texture name: 'stone/c', aborted",
               "FindReplaceTextures: ungueltiger Name abgebrochen");
        pruefe(texturenErsetzen(k, e, "textures/stone/c", "textures/neu/n", false) && p2->flaechen()[0]->shader() == "textures/neu/n"
                   && p2->flaechen()[1]->shader() == "textures/x/z"
                   && k.undo.undoName() == "textureFindReplace -find textures/stone/c -replace textures/neu/n",
               "FindReplaceTextures: ersetzt in allen Pinseln, Rueckgaengig-Name");
        pruefe(texturenErsetzen(k, e, "textures/x/z", "textures/", false) && k.auswahl.flaechen().size() == 1
                   && p2->flaechen()[1]->shader() == "textures/x/z",
               "FindReplaceTextures: Ersetzen leer (textures/) = suchen, waehlt die Flaeche");
        k.auswahl.flaechenLeeren();
        k.auswahl.waehlen(welt, p1);
        pruefe(texturenErsetzen(k, e, "textures/neu/n", "textures/nur/gewaehlt", true) && p2->flaechen()[0]->shader() == "textures/neu/n",
               "Within selected brushes only: der nicht gewaehlte bleibt");
    }

    std::filesystem::remove_all(tmp, ec);
    std::printf("kartetexturbrowser: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
