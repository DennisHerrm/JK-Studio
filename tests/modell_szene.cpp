// modell_szene.cpp - AP5 (MBENTWURF Teil B, Abgleich P7): die Szene als ModView-Skript (.mvs), bhed/modell/szene.h
//
//   ModView    ein Skript in der Form, die ModView schreibt (script.cpp: Hauptmodell, optionale Schluessel, boltmodel an Tag und
//              Knochen, Flaechenlisten) wird vollstaendig gelesen
//   Fehler     die Meldungen von Script_Read / Script_Parse_*: kein loadmodel, Klammern, name/modelfile/parent/bolt fehlt,
//              unbekannte Kennung
//   Nicht      was JK Studio nicht abbildet (Anbau an Anbau, multiseq, ethnic, Sequenz eines Anbaus), steht in nichtUebernommen
//   Kreis      schreiben -> lesen gibt dieselbe Szene; die Schluessel jks_* bleiben fuer ModView unsichtbare Zusaetze
//   Kennung    GenerateHandleName: Dateiname ohne Endung, klein, bei Doppel _2, _3
#include "bhed/modell/szene.h"

#include <cstddef>
#include <cstdio>
#include <string>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", what.c_str());
    if (!ok) { ++fails; }
}
}  // namespace

int main() {
    using namespace bhed::modell;
    // --- ModView: ein Skript wie Script_Write es schreibt (Tabulatoren, Reihenfolge, Leerzeilen) ---------------------------------
    const char* const kModView =
        "loadmodel\n{\n"
        "\tname\t\t\"model\"\n"
        "\tmodelfile\t\"models/players/kyle/model.glm\"\n"
        "\tsurfacename_rootoverride\t\"torso\"\n"
        "\tbonename_secondarystart\t\"lower_lumbar\"\n"
        "\tlocksequence\t\"BOTH_STAND1\"\n"
        "\tlocksequence_secondary\t\"TORSO_WEAPONREADY3\"\n"
        "\toldskinfile\t\"models/players/kyle/model_blue.skin\"\n"
        "\tsurfaces_off\n\t{\n\t\tname0\t\"head\"\n\t\tname1\t\"l_arm\"\n\t}\n"
        "\tsurfaces_offnochildren\n\t{\n\t\tname0\t\"r_leg\"\n\t}\n"
        "\n"
        "\tboltmodel\n\t{\n\t\tname\t\"saber_1\"\n\t\tmodelfile\t\"models/weapons2/saber_1/saber_1.glm\"\n\t\tparent\t\"model\"\n"
        "\t\tbolttosurface\t\"*r_hand\"\n\t}\n\n"
        "\tboltmodel\n\t{\n\t\tname\t\"blaster\"\n\t\tmodelfile\t\"models/weapons2/blaster_r/blaster_w.glm\"\n\t\tparent\t\"model\"\n"
        "\t\tbolttobone\t\"lhand\"\n\t}\n\n"
        "}\n\n";
    Szene s;
    std::string fehler;
    expect("ModView: Skript gelesen (" + fehler + ")", szeneLesen(kModView, s, &fehler));
    expect("ModView: name, modelfile", s.name == "model" && s.modell == "models/players/kyle/model.glm");
    expect("ModView: Wurzel, Sekundaer-Start, Primaer, Sekundaer, Haut",
           s.wurzel == "torso" && s.sekundaerStart == "lower_lumbar" && s.primaer == "BOTH_STAND1" && s.sekundaer == "TORSO_WEAPONREADY3" &&
               s.haut == "models/players/kyle/model_blue.skin");
    expect("ModView: Flaechen aus (2), aus ohne Kinder (1), an (0)",
           s.flaechenAus.size() == 2 && s.flaechenAus[0] == "head" && s.flaechenAus[1] == "l_arm" && s.flaechenOhneKinder.size() == 1 &&
               s.flaechenOhneKinder[0] == "r_leg" && s.flaechenAn.empty());
    expect("ModView: zwei Anbauten - Tag und Knochen", s.anbauten.size() == 2 && s.anbauten[0].ziel == "*r_hand" && !s.anbauten[0].knochen &&
                                                           s.anbauten[1].ziel == "lhand" && s.anbauten[1].knochen &&
                                                           s.anbauten[1].modell == "models/weapons2/blaster_r/blaster_w.glm");
    expect("ModView: nichts ausgelassen, keine Zusaetze", s.nichtUebernommen.empty() && s.lod < 0 && s.sichtfeld < 0.0F && s.boden < 0);
    // Schluessel ohne Ruecksicht auf Gross-/Kleinschreibung (CGPGroup::FindPairValue -> Q_stricmp)
    Szene g;
    expect("Gross/klein: LoadModel, ModelFile, BoltModel",
           szeneLesen("LoadModel\n{\nName \"m\"\nModelFile \"a.glm\"\nBoltModel\n{\nname \"b\"\nmodelfile \"b.glm\"\nparent \"m\"\nBoltToBone \"x\"\n}\n}\n", g, &fehler) &&
               g.modell == "a.glm" && g.anbauten.size() == 1 && g.anbauten[0].knochen);
    // --- Fehler wie ModView ---------------------------------------------------------------------------------------------------
    struct Fall {
        const char* text;
        const char* teil;
    };
    const Fall kFaelle[] = {
        {"foo\n{\n}\n", "Unable to find keyword 'loadmodel'"},
        {"loadmodel\n{\nname \"m\"\nmodelfile \"a.glm\"\n", "Brace mismatch"},
        {"loadmodel\n{\nmodelfile \"a.glm\"\n}\n", "No 'name' keyword found"},
        {"loadmodel\n{\nname \"m\"\n}\n", "No 'modelfile' keyword found"},
        {"loadmodel\n{\nname \"m\"\nmodelfile \"a.glm\"\nboltmodel\n{\nname \"b\"\nmodelfile \"b.glm\"\nbolttobone \"x\"\n}\n}\n", "No 'parent' keyword found"},
        {"loadmodel\n{\nname \"m\"\nmodelfile \"a.glm\"\nboltmodel\n{\nname \"b\"\nmodelfile \"b.glm\"\nparent \"m\"\n}\n}\n", "No 'bolttobone' or 'bolttosurface'"},
        {"loadmodel\n{\nname \"m\"\nmodelfile \"a.glm\"\nboltmodel\n{\nname \"b\"\nmodelfile \"b.glm\"\nparent \"q\"\nbolttobone \"x\"\n}\n}\n", "Unknown parent handle \"q\""},
    };
    for (const Fall& f : kFaelle) {
        Szene x;
        std::string m;
        const bool ok = szeneLesen(f.text, x, &m);
        expect(std::string("Fehler: \"") + f.teil + "\" (gemeldet: " + m + ")", !ok && m.find(f.teil) != std::string::npos);
    }
    // --- Nicht abgebildet ---------------------------------------------------------------------------------------------------------
    Szene n;
    const bool nOk = szeneLesen("loadmodel\n{\nname \"m\"\nmodelfile \"a.glm\"\nmultiseq_primary_lock \"x\"\n"
                                "boltmodel\n{\nname \"b\"\nmodelfile \"b.glm\"\nparent \"m\"\nbolttosurface \"*r_hand\"\nlocksequence \"S\"\n}\n"
                                "boltmodel\n{\nname \"c\"\nmodelfile \"c.glm\"\nparent \"b\"\nbolttobone \"gun\"\n}\n}\n",
                                n, &fehler);
    std::string liste;
    for (const std::string& z : n.nichtUebernommen) { liste += z + "; "; }
    expect("Nicht abgebildet: multiseq, Sequenz des Anbaus, Anbau am Anbau (" + liste + ")",
           nOk && n.nichtUebernommen.size() == 3 && n.anbauten.size() == 2 && liste.find("m: multiseq_primary_lock") != std::string::npos &&
               liste.find("b: locksequence") != std::string::npos && liste.find("c: parent b") != std::string::npos);
    // --- Kreis ------------------------------------------------------------------------------------------------------------------
    Szene k = s;
    k.flaechenAn = {"stupidtriangle_off"};
    k.lod = 3;
    k.sichtfeld = 10.0F;
    k.boden = 1;
    k.bodenZ = -23.5F;
    const std::string text = szeneSchreiben(k);
    Szene zurueck;
    expect("Kreis: das Geschriebene ist lesbar (" + fehler + ")", szeneLesen(text, zurueck, &fehler));
    bool anbautenGleich = zurueck.anbauten.size() == k.anbauten.size();
    for (std::size_t i = 0; anbautenGleich && i < k.anbauten.size(); ++i) {
        anbautenGleich = zurueck.anbauten[i].name == k.anbauten[i].name && zurueck.anbauten[i].modell == k.anbauten[i].modell &&
                         zurueck.anbauten[i].eltern == k.anbauten[i].eltern && zurueck.anbauten[i].ziel == k.anbauten[i].ziel &&
                         zurueck.anbauten[i].knochen == k.anbauten[i].knochen;
    }
    expect("Kreis: alles gleich", zurueck.name == k.name && zurueck.modell == k.modell && zurueck.wurzel == k.wurzel &&
                                      zurueck.sekundaerStart == k.sekundaerStart && zurueck.primaer == k.primaer && zurueck.sekundaer == k.sekundaer &&
                                      zurueck.haut == k.haut && zurueck.flaechenAn == k.flaechenAn && zurueck.flaechenAus == k.flaechenAus &&
                                      zurueck.flaechenOhneKinder == k.flaechenOhneKinder && anbautenGleich && zurueck.lod == 3 &&
                                      zurueck.sichtfeld == 10.0F && zurueck.boden == 1 && zurueck.bodenZ == -23.5F);
    // ModView schreibt bonename_secondarystart VOR locksequence_secondary ("MUST be written out before ..., or reading won't work")
    expect("Kreis: Sekundaer-Start vor der Sekundaersequenz", text.find("bonename_secondarystart") < text.find("locksequence_secondary"));
    expect("Kreis: Anbau an Tag als bolttosurface, an Knochen als bolttobone",
           text.find("bolttosurface\t\"*r_hand\"") != std::string::npos && text.find("bolttobone\t\"lhand\"") != std::string::npos);
    // --- Kennung -----------------------------------------------------------------------------------------------------------------
    expect("Kennung: Dateiname ohne Endung, klein", szenenKennung("models/weapons2/saber_1/Saber_1.GLM", {}) == "saber_1");
    expect("Kennung: Doppel _2, _3", szenenKennung("a/saber_1.glm", {"saber_1"}) == "saber_1_2" &&
                                         szenenKennung("b\\saber_1.glm", {"saber_1", "saber_1_2"}) == "saber_1_3");
    std::printf("%s: %d FEHLER\n", fails == 0 ? "modell_szene OK" : "modell_szene", fails);
    return fails == 0 ? 0 : 1;
}
