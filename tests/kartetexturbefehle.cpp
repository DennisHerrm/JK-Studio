// kartetexturbefehle.cpp - Karten-Editor Stufe 2, Punkt 7: Textur-Befehle
// der Auswahl wie NetRadiant-Custom 20260114 (Texdef_Shift/Scale/Rotate,
// Select_FitTexture, Select_ProjectTexture, ApplyTexdef_*, Invert,
// Patch_SetTexdef mit PatchTexdefConstructor, DoSnapTToGrid,
// SurfaceInspector_SetCurrent_FromSelected).
//
// Quake-Texdef (AP), Texturen 64 x 64, Standardskalierung 0.5: ein frisch
// gebauter Quader hat shift 0, scale 0.5, rotate 0 (ShiftScaleRotate_fromFace).
#include "bhed/karte/karte.h"
#include "bhed/karte/patchbefehle.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/texturbefehle.h"

#include <cmath>
#include <cstdio>
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
bool nah(double a, double b, double eps = 1e-3) { return std::fabs(a - b) <= eps; }

struct Aufbau {
    Karte k;
    std::shared_ptr<Pinsel> p;
    AuswahlEinstellung e;
    Aufbau() {
        TexturProjektion proj;
        texdefStandard(TexdefArt::Quake, proj);
        p = std::make_shared<Pinsel>();
        p->quaderBauen(Kasten(Vektor3f(32, 32, 32), Vektor3f(32, 32, 32)), "textures/a/b", proj, k.kontext);
        k.weltHolenOderAnlegen()->kindEinfuegen(p);
        k.auswahl.waehlen(k.welt(), p);
        k.undo.clear();
    }
    Texdef ssr(std::size_t f = 0) const {
        Texdef t;
        schiebSkalDrehAusFlaeche(TexdefArt::Quake, t, p->flaechen()[f]->texdefHolen(k.kontext));
        return t;
    }
};

std::string text(const Texdef& t) {
    char b[160];
    std::snprintf(b, sizeof(b), "shift %g %g scale %g %g rot %g", static_cast<double>(t.shift[0]), static_cast<double>(t.shift[1]), static_cast<double>(t.scale[0]),
                  static_cast<double>(t.scale[1]), static_cast<double>(t.rotate));
    return b;
}

void tasten() {
    Aufbau a;
    pruefe(nah(a.ssr().scale[0], 0.5) && nah(a.ssr().shift[0], 0), "Ausgang: scale 0.5, shift 0 (" + text(a.ssr()) + ")");
    const SiWerte w;   // Vorgaben: shift 8, scale 0.5, rotate 45
    texdefSchieben(a.k, a.e, w.verschiebung[0], 0);
    pruefe(nah(a.ssr().shift[0], 8) && a.k.undo.undoName() == "brushShiftTexture -x 8 -y 0", "Shift+Right: shift 8 (" + text(a.ssr()) + ")");
    texdefSkalieren(a.k, a.e, 0, w.skalierung[1]);
    pruefe(nah(a.ssr().scale[1], 1.0) && a.k.undo.undoName() == "brushScaleTexture -x 0 -y 0.5", "Ctrl+Up: scale y 0.5 + 0.5 (" + text(a.ssr()) + ")");
    texdefDrehen(a.k, a.e, -std::fabs(w.drehung));
    pruefe(nah(a.ssr().rotate, -45) && a.k.undo.undoName() == "brushRotateTexture -angle -45", "Shift+PgDown: rotate -45 (" + text(a.ssr()) + ")");
    // nur die gewaehlte Flaeche im Komponentenmodus
    Aufbau b;
    b.e.modus = AuswahlModus::Komponente;
    b.e.komponente = KomponentenModus::Flaeche;
    b.k.auswahl.flaecheWaehlen(b.p, b.p->flaechen()[0]);
    texdefSchieben(b.k, b.e, 0, 16);
    pruefe(nah(b.ssr(0).shift[1], 16) && nah(b.ssr(1).shift[1], 0), "Shift+Up mit Flaechenwahl: nur die gewaehlte Flaeche");
}

void inspektor() {
    Aufbau a;
    texdefWertBefehl(a.k, a.e, 0, 16);
    pruefe(nah(a.ssr(2).shift[0], 16) && a.k.undo.undoName() == "textureProjectionSetSelected -hShift 16", "ApplyTexdef_HShift 16 an allen Flaechen");
    texdefWertBefehl(a.k, a.e, 4, 33.337F);
    pruefe(nah(a.ssr().rotate, 33.337, 1e-2) && a.k.undo.undoName() == "textureProjectionSetSelected -rotation 33.34", "ApplyTexdef_Rotation: Name auf 2 Stellen");
    texturUmkehrenBefehl(a.k, a.e, true, SiWerte{}, 16, 0.5F);
    pruefe(nah(a.ssr().shift[0], -16) && nah(a.ssr().scale[0], -0.5) && a.k.undo.undoName() == "textureInvertHorizontally", "Invert horizontally: -shift, -scale");
    // Fit 1 x 1 an einem frischen Quader: 64 breite Flaeche, Textur 64 -> scale 1
    Aufbau f;
    texturEinpassenBefehl(f.k, f.e, 1, 1, 0);
    pruefe(nah(std::fabs(f.ssr(0).scale[0]), 1.0) && nah(std::fabs(f.ssr(0).scale[1]), 1.0) && f.k.undo.undoName() == "textureAutoFit",
           "Fit 1 1: scale 1 (" + text(f.ssr(0)) + ")");
    texturEinpassenBefehl(f.k, f.e, 2, 1, 1);
    pruefe(nah(std::fabs(f.ssr(0).scale[0]), 0.5) && f.k.undo.undoName() == "textureAutoFitWidth", "Fit Width 2: scale 0.5 (" + text(f.ssr(0)) + ")");
    // Anzeige: Pinsel
    const SiAnzeige s = siAusAuswahl(a.k);
    pruefe(s.gefunden && !s.patchModus && s.shader == "textures/a/b", "SurfaceInspector: Pinsel, Shader der ersten Flaeche");
    // Match Grid: Raster 8 / Skalierung 0.5 = 16
    SiWerte w;
    schritteAmRaster(w, 8, 0.5F, 0.25F);
    pruefe(nah(w.verschiebung[0], 16) && nah(w.verschiebung[1], 32), "DoSnapTToGrid: 8 / 0.5 = 16, 8 / 0.25 = 32");
}

void projektion() {
    Aufbau a;
    Protokoll pr;
    Texdef td;
    td.scale[0] = td.scale[1] = 0.5F;
    texturProjizierenBefehl(a.k, a.e, td, 0, Vektor3f(0, 0, 1), pr);
    pruefe(a.k.undo.undoName() == "textureProjectAxial" && pr.size() == 1 && pr[0].art == ProtokollTeil::Art::Warnung,
           "Project Axial: Name, Warnung bei Quake-Texdef");
}

void patches() {
    Aufbau a;
    patchVorlageBefehl(a.k, Kasten(Vektor3f(0, 0, 32), Vektor3f(32, 32, 32)), "textures/a/patch", PatchVorlage::Ebene, 2, 3, 3);
    const auto patch = std::static_pointer_cast<Patch>(a.k.welt()->kinder().back());
    patchBefehl(a.k, PatchBefehl::TexturNatuerlich);
    const PatchTexdef c = patchTexdef(*patch, a.k.kontext);
    pruefe(c.gueltig(), "PatchTexdefConstructor: gueltig an der natuerlich texturierten Ebene");
    SiAnzeige s = siAusAuswahl(a.k);
    Texdef vorher;
    schiebSkalDrehAusPatch(vorher, s.projektion);
    pruefe(s.gefunden && s.patchModus && s.shader == "textures/a/patch", "SurfaceInspector: Patch-Modus");
    texdefWertBefehl(a.k, AuswahlEinstellung{}, 0, 32);
    s = siAusAuswahl(a.k);
    Texdef nachher;
    schiebSkalDrehAusPatch(nachher, s.projektion);
    pruefe(nah(nachher.shift[0], 32, 1e-2) && nah(nachher.scale[0], vorher.scale[0], 1e-3), "Patch_SetTexdef hShift 32: zurueckgelesen 32 (" + text(nachher) + ")");
    // Shift-Taste am Patch: TranslateTexture (s -= 8/64)
    const float s0 = patch->punkt(0, 0).texcoord[0];
    texdefSchieben(a.k, AuswahlEinstellung{}, 8, 0);
    pruefe(nah(patch->punkt(0, 0).texcoord[0], s0 - 0.125), "Shift+Right am Patch: s - 8/64");
    // Scale am Patch: Patch_convertScale( 0 ) = 1, ( 0.5 ) = 0.5
    const float t0 = patch->punkt(2, 2).texcoord[1];
    const float s2 = patch->punkt(2, 2).texcoord[0];
    texdefSkalieren(a.k, AuswahlEinstellung{}, 0, 0.5F);
    pruefe(nah(patch->punkt(2, 2).texcoord[1], t0 * 0.5) && nah(patch->punkt(2, 2).texcoord[0], s2), "Ctrl+Up am Patch: t * 0.5, s * 1");
    // Fit am Patch: SetTextureRepeat
    texturEinpassenBefehl(a.k, AuswahlEinstellung{}, 2, 3, 0);
    pruefe(nah(patch->punkt(0, 2).texcoord[0], 2) && nah(patch->punkt(2, 0).texcoord[1], 3), "Fit 2 3 am Patch: SetTextureRepeat");
}

}  // namespace

int main() {
    tasten();
    inspektor();
    projektion();
    patches();
    std::printf("kartetexturbefehle: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
