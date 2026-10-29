// karteoptsicher.cpp - "Fix all safe" darf nichts Sichtbares aendern (ctest karte_optsicher)
//
// Fund md_rev_TarisUpper2 (Anwender): Flaechen hinter Fenstern wurden caulk. Hier die Regeln:
//   - ein Shader, dessen erste Stufe mischt (blendFunc ausser GL_ONE GL_ZERO) oder alphaFunc hat,
//     ist durchsichtig; eine spaetere Lightmap-Stufe (filter) nicht;
//   - eine Flaeche gilt nur als verdeckt, wenn der deckende Pinsel GANZ undurchsichtig ist (eine
//     Glasseite oder eine Werkzeug-Seite wie caulk genuegt, damit man im Spiel hindurchsieht);
//   - Caulk ist textures/system/caulk (system.shader von JKA/JK2).
// Den Kamera-Vergleich auf der Karte des Anwenders und auf dekompilierten Originalkarten macht der
// Selbsttest (BHED_KARTE_NUR=optsicher).
#include <cstdio>
#include <string>

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/pinsel.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
std::string quader(int x0, int x1, const char* vorn, const char* hinten) {   // y/z 0..64, -x-Seite "vorn"? siehe unten
    char z[2048];
    std::snprintf(z, sizeof(z),
                  "{\n( %d 0 0 ) ( %d 64 0 ) ( %d 0 64 ) %s 0 0 0 0.5 0.5 0 0 0\n"   // -x
                  "( %d 0 0 ) ( %d 0 64 ) ( %d 64 0 ) %s 0 0 0 0.5 0.5 0 0 0\n"   // +x
                  "( %d 0 0 ) ( %d 0 64 ) ( %d 0 0 ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 64 0 ) ( %d 64 0 ) ( %d 64 64 ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 0 0 ) ( %d 0 0 ) ( %d 64 0 ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 0 64 ) ( %d 64 64 ) ( %d 0 64 ) test/wand 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x0, x0, x0, vorn, x1, x1, x1, hinten, x0, x0, x1, x0, x1, x0, x0, x1, x0, x0, x0, x1);
    return z;
}
long long verdeckt(const std::string& text, const bhed::karte::PruefOptionen& o) {
    bhed::karte::Karte k;
    (void)bhed::karte::karteLesen(text, k);
    for (const auto& b : bhed::karte::pruefeKarte(bhed::karte::kartenAbbild(k), o)) {
        if (b.pruefung == "verdeckt") { return b.anzahl; }
    }
    return 0;
}
}  // namespace

int main() {
    using namespace bhed::karte;
    std::printf("--- Shader: durchsichtig ---\n");
    const auto sk = [](const char* t) { return shaderSkriptLesen(t, "textures/test/x"); };
    expect("blendFunc GL_ONE GL_ONE in der ersten Stufe: durchsichtig",
           sk("textures/test/x\n{\n{\nmap textures/test/x.tga\nblendFunc GL_ONE GL_ONE\n}\n}\n").durchsichtig);
    expect("blendFunc blend: durchsichtig", sk("textures/test/x\n{\n{\nmap a.tga\nblendfunc blend\n}\n}\n").durchsichtig);
    expect("alphaFunc GE128: durchsichtig", sk("textures/test/x\n{\n{\nmap a.tga\nalphaFunc GE128\n}\n}\n").durchsichtig);
    expect("erste Stufe deckend, Lightmap-Stufe filter: nicht durchsichtig",
           !sk("textures/test/x\n{\n{\nmap $lightmap\n}\n{\nmap a.tga\nblendFunc GL_DST_COLOR GL_ZERO\n}\n}\n").durchsichtig);
    expect("GL_ONE GL_ZERO: nicht durchsichtig", !sk("textures/test/x\n{\n{\nmap a.tga\nblendFunc GL_ONE GL_ZERO\n}\n}\n").durchsichtig);

    std::printf("--- Verdeckt nur hinter ganz undurchsichtigen Pinseln ---\n");
    PruefOptionen o;
    o.spiel = GrenzSpiel::MovieDuels;
    ShaderArt glas;
    glas.undurchsichtig = false;
    o.shader["textures/test/glas"] = glas;
    // A (0..64) und B (64..128) beruehren sich bei x = 64
    const std::string beide = "{\n\"classname\" \"worldspawn\"\n" + quader(0, 64, "test/wand", "test/wand") + quader(64, 128, "test/wand", "test/wand") + "}\n";
    expect("zwei deckende Pinsel: beide Beruehrflaechen verdeckt (2)", verdeckt(beide, o) == 2);
    const std::string fenster = "{\n\"classname\" \"worldspawn\"\n" + quader(0, 64, "test/wand", "test/wand") + quader(64, 128, "test/wand", "test/glas") + "}\n";
    const long long n = verdeckt(fenster, o);
    std::printf("      mit Glas an B: %lld\n", n);
    expect("B hat eine Glasseite: As Flaeche bleibt sichtbar (durch B hindurch), nur Bs Flaeche an A ist verdeckt (1)", n == 1);
    // Werkzeug-Seite (caulk ist im Spiel nodraw): durch sie sieht man im Spiel in B hinein
    ShaderArt werkzeug;
    werkzeug.werkzeug = true;
    o.shader["textures/test/werkzeug"] = werkzeug;
    const std::string caulkSeite = "{\n\"classname\" \"worldspawn\"\n" + quader(0, 64, "test/wand", "test/wand") + quader(64, 128, "test/wand", "test/werkzeug") + "}\n";
    expect("B hat eine Werkzeug-Seite: As Flaeche bleibt sichtbar (1)", verdeckt(caulkSeite, o) == 1);
    // caulk-Seite (fest, deckend, aber nodraw): im Spiel sieht man durch sie in B hinein
    const std::string caulkB = "{\n\"classname\" \"worldspawn\"\n" + quader(0, 64, "test/wand", "test/wand") + quader(64, 128, "test/wand", "system/caulk") + "}\n";
    expect("B hat eine caulk-Seite: As Flaeche bleibt sichtbar (1)", verdeckt(caulkB, o) == 1);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
