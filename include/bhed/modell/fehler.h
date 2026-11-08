// fehler.h - REST N6 (Code-Pruefung Modell-Block): Fehler der Modell-Bibliothek als Art und Werte. Den Text in der
// Sprache der Oberflaeche setzt die GUI (Str::MbFe..., gui/app_modelltab_mb3.cpp modellFehlerText); fehlerText gibt den
// englischen Satz fuer Protokoll und Tests. Bis rc610 standen in den Meldungen deutsche (md3datei, glmdatei) oder
// englische (retarget) Saetze der Bibliothek, je nach Modul, in jeder Sprache der Oberflaeche.
#ifndef BHED_MODELL_FEHLER_H
#define BHED_MODELL_FEHLER_H

#include <cstdio>
#include <string>

namespace bhed::modell {

enum class FehlerArt : int {
    Keiner,
    KeinMd3,               // keine .md3
    Md3PasstNicht,         // die Flaechen passen nicht zur Datei
    Md3HinterFlaechen,     // Bilder oder Tags hinter den Flaechen
    FlaecheAbgeschnitten,  // a: Flaeche
    FlaecheUnbrauchbar,    // a: Flaeche
    FlaecheOhneNetz,       // a: Flaeche
    ZuVieleEcken,          // name, a: Ecken, b: Grenze
    ZuVieleDreiecke,       // name, a: Dreiecke, b: Grenze
    KeinGlm,               // kein Ghoul2-Modell
    GlaPfadZuLang,         // Name oder Pfad der .gla ueber 63 Zeichen
    TeilNameZuLang,        // name
    OhneLods,              // name
    ZuVieleKnochen,        // name, a: LOD, b: Knochen
    QuelleOhneBecken,
    ZielOhneBecken,
    BilderAusserhalb,
    NichtsZuBacken,
    FremdesSkelett,
    VorratZuGross,
    OhneBeckenBeine,
    KeinPlatzBelegt,       // animcfg zusammenfuehren
    AnderesSkelett,
    KnochenAnders,         // name
    KeinSkelett,           // figuranim, xsiaufnahme
    GrundanimFehlt,        // name
    GrundbildFehlt,
    VerweisHinterVorrat,
    KeinKinoPlatz,
    PlaetzeFiguren,
    FigurOhneGrund,
    Fehlt,                 // physik: name (Liste)
    NichtUnterBecken,      // physik: name
    QuellKnochenFehlt,     // animcfg: name GLA, name2 Knochen
    QuellGrundstellung,    // animcfg: name GLA, name2 Knochen
    SequenzNichtInQuelle,  // animcfg: name Platz, a erster Frame, b Anzahl, name2 GLA, c Frames der GLA
    QuellVerweis,          // animcfg: name GLA
    BereichAusserhalb,     // rig: Bereich ausserhalb der GLA
    // FIX2 P5/P14: Animations-Import (animimport.h); die drei letzten sind Hinweise, keine Fehler
    KeineAnimation,
    AchsenUnbestimmt,      // name: oben, name2: vorn
    AnimZuLang,            // a: Bilder, b: MB noetig, c: MB frei (0: unbekannt)
    BvhOhneSeiten,
    BvhBoden,              // a: Zehntel cm
    UngleicheSkalierung,
    Anzahl
};

struct ModellFehler {
    FehlerArt art = FehlerArt::Keiner;
    std::string name;
    long long a = 0;
    long long b = 0;
    std::string name2;
    long long c = 0;
};

// Der englische Satz (Protokoll, Tests, Rueckgabe als Text)
[[nodiscard]] inline std::string fehlerText(const ModellFehler& f) {
    const std::string a = std::to_string(f.a);
    const std::string b = std::to_string(f.b);
    switch (f.art) {
        case FehlerArt::Keiner: return {};
        case FehlerArt::KeinMd3: return "not an .md3 model";
        case FehlerArt::Md3PasstNicht: return "the surfaces do not match the file";
        case FehlerArt::Md3HinterFlaechen: return "frames or tags behind the surfaces";
        case FehlerArt::FlaecheAbgeschnitten: return "surface " + a + " is cut off";
        case FehlerArt::FlaecheUnbrauchbar: return "surface " + a + " is unusable";
        case FehlerArt::FlaecheOhneNetz: return "surface " + a + " has no mesh";
        case FehlerArt::ZuVieleEcken: return "surface " + f.name + ": " + a + " vertices (limit " + b + ")";
        case FehlerArt::ZuVieleDreiecke: return "surface " + f.name + ": " + a + " triangles (limit " + b + ")";
        case FehlerArt::KeinGlm: return "not a Ghoul2 model (.glm)";
        case FehlerArt::GlaPfadZuLang: return "name or path of the .gla longer than 63 characters";
        case FehlerArt::TeilNameZuLang: return "name or shader of \"" + f.name + "\" longer than 63 characters";
        case FehlerArt::OhneLods: return "surface \"" + f.name + "\" lacks LOD levels";
        case FehlerArt::ZuVieleKnochen: return "surface \"" + f.name + "\" (LOD " + a + ") uses " + b + " bones, a .glm holds at most 32";
        case FehlerArt::QuelleOhneBecken: return "source without pelvis";
        case FehlerArt::ZielOhneBecken: return "target without pelvis";
        case FehlerArt::BilderAusserhalb: return "frames outside the source";
        case FehlerArt::NichtsZuBacken: return "nothing to bake";
        case FehlerArt::FremdesSkelett: return "world of another skeleton";
        case FehlerArt::VorratZuGross: return "pool larger than a 24-bit index";
        case FehlerArt::OhneBeckenBeine: return "no pelvis and two legs";
        case FehlerArt::KeinPlatzBelegt: return "no slot has a source";
        case FehlerArt::AnderesSkelett: return "another skeleton than the GLA";
        case FehlerArt::KnochenAnders: return "bone " + f.name + " is missing in the GLA or has another base pose";
        case FehlerArt::KeinSkelett: return "no skeleton";
        case FehlerArt::GrundanimFehlt: return "base animation " + f.name + " is missing";
        case FehlerArt::GrundbildFehlt: return "frame of the base animation not in the GLA";
        case FehlerArt::VerweisHinterVorrat: return "reference behind the pool";
        case FehlerArt::KeinKinoPlatz: return "no free slot BOTH_CIN_n";
        case FehlerArt::PlaetzeFiguren: return "slots and figures do not match";
        case FehlerArt::FigurOhneGrund: return "figure without base animation";
        case FehlerArt::Fehlt: return "missing: " + f.name;
        case FehlerArt::NichtUnterBecken: return "not below the pelvis: " + f.name;
        case FehlerArt::QuellKnochenFehlt: return f.name + ": bone " + f.name2 + " is missing";
        case FehlerArt::QuellGrundstellung: return f.name + ": base pose of " + f.name2 + " differs";
        case FehlerArt::SequenzNichtInQuelle: return f.name + ": frames " + a + "+" + b + " not in " + f.name2 + " (" + std::to_string(f.c) + " frames)";
        case FehlerArt::QuellVerweis: return f.name + ": reference behind the pool";
        case FehlerArt::BereichAusserhalb: return "range outside the GLA";
        case FehlerArt::KeineAnimation: return "no animation in the file";
        case FehlerArt::AchsenUnbestimmt: return "the axes of the file are left-handed or undetermined (up " + f.name + ", front " + f.name2 + ")";
        case FehlerArt::AnimZuLang: return "animation too long: " + a + " frames would need " + b + " MB (" + std::to_string(f.c) + " MB free)";
        case FehlerArt::BvhOhneSeiten: return "BVH: no sides in the bone names, left = +X assumed";
        case FehlerArt::BvhBoden: {
            char t[48];
            std::snprintf(t, sizeof t, "%.1f", static_cast<double>(f.a) / 10.0);
            return std::string("BVH: base pose set onto the floor of the frames (") + t + " cm)";
        }
        case FehlerArt::UngleicheSkalierung: return "non-uniform scaling removed (only rotation and position taken)";
        case FehlerArt::Anzahl: break;
    }
    return {};
}

// Zurueck vom englischen Satz zur Art (Fehler, die nur als Text weitergereicht werden: std::string* fehler, die Berichte
// von animcfg, figuranim, xsiaufnahme, physik, retarget). Nur Arten ohne Zahlen; false: kein Satz der Bibliothek.
[[nodiscard]] inline bool fehlerAusText(const std::string& t, ModellFehler& aus) {
    constexpr char kPlatz = '\x01';   // Platzhalter des Namens im Muster
    for (int i = 1; i < static_cast<int>(FehlerArt::Anzahl); ++i) {
        const auto art = static_cast<FehlerArt>(i);
        if (art == FehlerArt::FlaecheAbgeschnitten || art == FehlerArt::FlaecheUnbrauchbar || art == FehlerArt::FlaecheOhneNetz ||
            art == FehlerArt::ZuVieleEcken || art == FehlerArt::ZuVieleDreiecke || art == FehlerArt::ZuVieleKnochen || art == FehlerArt::QuellKnochenFehlt ||
            art == FehlerArt::QuellGrundstellung || art == FehlerArt::SequenzNichtInQuelle || art == FehlerArt::QuellVerweis ||
            art == FehlerArt::AchsenUnbestimmt || art == FehlerArt::AnimZuLang || art == FehlerArt::BvhBoden) {
            continue;   // mit Zahlen oder zwei Namen: die Berichte tragen ModellFehler selbst
        }
        const std::string muster = fehlerText(ModellFehler{art, std::string(1, kPlatz)});
        const std::size_t p = muster.find(kPlatz);
        if (p == std::string::npos) {
            if (t == muster) {
                aus = ModellFehler{art};
                return true;
            }
            continue;
        }
        const std::string vor = muster.substr(0, p);
        const std::string nach = muster.substr(p + 1);
        if (t.size() > vor.size() + nach.size() && t.compare(0, vor.size(), vor) == 0 && t.compare(t.size() - nach.size(), nach.size(), nach) == 0) {
            aus = ModellFehler{art, t.substr(vor.size(), t.size() - vor.size() - nach.size())};
            return true;
        }
    }
    return false;
}

}  // namespace bhed::modell
#endif
