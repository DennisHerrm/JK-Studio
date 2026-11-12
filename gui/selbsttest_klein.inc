// selbsttest_klein.inc - KLEIN (rc612, Welle 12.11.): Kleinkram im Model-Tab und am About-Fenster, je Punkt erst rot, dann gruen
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=klein; der zweite Start BHED_MODELL_NUR=kleinneustart mit den Einstellungen des
// ersten). Die Nummern sind die des Auftrags (JKStudio_Uebergabe/auftraege/welle_1211.md, Abschnitt 6).
//
//   4a  Jedes Werkzeug mit einer Art merkt sich seine eigene (wie 3ds Max extrusionType / insetType / bevelType): nach "Je Vieleck"
//       beim Extrudieren oeffnet Einfuegen (Inset) mit SEINER Art, und umgekehrt.
//   4b  Eine eingefrorene Flaeche nimmt am Treffertest nicht teil - auch nicht fuer Ecke, Kante und Dreieck (Klick und Rahmen) und
//       nicht im Bereich Modellieren: der Klick geht hindurch auf die Flaeche dahinter (ENT1 hatte nur den Klick auf Flaechen).
//   5   "Aktuelle Ansicht als Home setzen" gehoert zur Figur und bleibt: nach einer anderen Figur und nach dem Neustart ist es
//       wieder da.
//   6   Das Navigations-Gizmo (wie Blender) hat den Kamera-Knopf zwischen Verschieben und Perspektive/Orthographisch.
//   10  Das About-Fenster passt bei Schrift 24 in ein 1920x1080-Fenster; die Knoepfe bleiben sichtbar.
//   11  Die Meldungen des Control Rig (Pose kopieren, IK/FK ab Bild ...) nennen das Bild in der Zaehlung der Zeitleiste (wie Max ab
//       0, wie Blender ab 1), nicht den Frame der GLA.
// Eingebunden in selbsttest.cpp (nach selbsttest_ap5.inc, benutzt die Helfer von mb1, mbkopf, md1d und ent2).
namespace kleinkram {   // "klein" heisst schon der Helfer fuer Kleinbuchstaben

using mbkopf::ansicht;
using mbkopf::t;

void melde(const std::string& z) { diag::detail("Knopftest: KLEIN " + z); }

float em() { return ImGui::GetFontSize(); }

// Alle ganzen Zahlen einer Meldung in ihrer Reihenfolge (fuer die Bilder in den Meldungen des Rigs)
std::vector<int> zahlenIn(const std::string& s) {
    std::vector<int> out;
    for (std::size_t i = 0; i < s.size();) {
        if (s[i] >= '0' && s[i] <= '9') {
            int v = 0;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i++] - '0'); }
            out.push_back(v);
        } else {
            ++i;
        }
    }
    return out;
}

// --- 4a: die Art je Werkzeug ----------------------------------------------------------------------------------------------

void artSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    using md1d::mz;
    md1d::neuesRequisit(s, "klein_kiste", "");
    md1d::kasten(s, 64.0F);
    const auto oben = [] { md1d::waehle(modell::wof::PAuswahl{{}, {}, {md1d::vieleckBei(2, 64.0F)}}, AuswahlArt::Dreieck); };
    add(tu("KLEIN 4a: oberes Vieleck gewaehlt, alle gemerkten Arten wie beim Programmstart (Gruppe)", [oben] {
        oben();
        mz().art = 0;
    }));
    add(md1d::ausloesen(Str::MdExtrude));
    add(pause(0.3));
    add(tu("KLEIN 4a: Extrudieren - Art Je Vieleck (2) gewaehlt, Uebernehmen", [] {
        if (!mz().kaestchenOffen || mz().werkzeug != MeshWerkzeug::Extrude) {
            meldeFehler("KLEIN 4a: das Kaestchen Extrudieren ist nicht offen");
            return;
        }
        mz().art = 2;
        mz().betrag = 8.0F;
        mz().neuPlanen = true;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
    }));
    add(pause(0.3));
    add(md1d::ausloesen(Str::MdInset));
    add(pause(0.3));
    add(pruefe("KLEIN 4a: nach Extrudieren \"Je Vieleck\" oeffnet Einfuegen (Inset) mit seiner eigenen Art (Gruppe) - das Feld Art zeigt einen seiner zwei Eintraege", [] {
        melde("4a Einfuegen nach Extrudieren Je Vieleck: Art " + std::to_string(mz().art) + " (Einfuegen kennt 0 Gruppe und 1 Je Vieleck)");
        return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Inset && mz().art == 0;
    }));
    add(tu("KLEIN 4a: Einfuegen - Art Je Vieleck (1) gewaehlt, Uebernehmen", [] {
        if (!mz().kaestchenOffen) { return; }
        mz().art = 1;
        mz().betrag = 4.0F;
        mz().neuPlanen = true;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
    }));
    add(pause(0.3));
    add(md1d::ausloesen(Str::MdExtrude));
    add(pause(0.3));
    add(pruefe("KLEIN 4a: Extrudieren oeffnet wieder mit seiner gemerkten Art (Je Vieleck = 2), nicht mit der von Einfuegen (1 = Lokale Normale)", [] {
        melde("4a Extrudieren nach Einfuegen Je Vieleck: Art " + std::to_string(mz().art));
        return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Extrude && mz().art == 2;
    }));
    add(tu("KLEIN 4a: Abbrechen", [] { modellMeshAbbrechen(); }));
    add(pause(0.2));
    add(md1d::ausloesen(Str::MdInset));
    add(pause(0.3));
    add(pruefe("KLEIN 4a: Einfuegen oeffnet wieder mit seiner gemerkten Art (Je Vieleck = 1)", [] {
        melde("4a Einfuegen zum zweiten Mal: Art " + std::to_string(mz().art));
        return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Inset && mz().art == 1;
    }));
    add(tu("KLEIN 4a: Abbrechen", [] { modellMeshAbbrechen(); }));
    add(pause(0.2));
    add(md1d::ausloesen(Str::MdBevel));
    add(pause(0.3));
    add(pruefe("KLEIN 4a: Abschraegen (Bevel) hat seine eigene Art (Gruppe), unberuehrt von Extrudieren und Einfuegen", [] {
        melde("4a Bevel: Art " + std::to_string(mz().art));
        return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Bevel && mz().art == 0;
    }));
    add(tu("KLEIN 4a: Abbrechen, Requisit ohne Rueckfrage verlassen", [] {
        modellMeshAbbrechen();
        g_app->mtab.gespeichertStand = g_app->mtab.dok.stand();   // das Requisit des Tests ist nicht gespeichert
    }));
    add(pause(0.2));
}

// --- 4b: Eingefrorenes nimmt am Treffertest nicht teil ---------------------------------------------------------------------

struct Stelle {
    ImVec2 p{-1.0F, -1.0F};
    int vorn = -1;
    int hinten = -1;
    ImVec2 allein{-1.0F, -1.0F};
    int einzige = -1;
};

std::string teilName(int i) {
    const auto& teile = t().dok.inhalt().teile;
    return i >= 0 && static_cast<std::size_t>(i) < teile.size() ? teile[static_cast<std::size_t>(i)].name : std::string("-");
}

// Welche Teile die gewaehlten Ecken tragen: "torso 3, hips 1"
std::string eckenTeile() {
    std::map<int, int> je;
    for (const modell::EckOrt& o : t().eckenWahl) { ++je[o.teil]; }
    std::string z;
    for (const auto& [teil, n] : je) { z += (z.empty() ? "" : ", ") + teilName(teil) + " " + std::to_string(n); }
    return z.empty() ? "nichts" : z;
}
bool eckenNurVon(int teil) {
    if (t().eckenWahl.empty()) { return false; }
    return std::all_of(t().eckenWahl.begin(), t().eckenWahl.end(), [&](const modell::EckOrt& o) { return o.teil == teil; });
}

void frostSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stelle>();
    add(tu("KLEIN 4b: wie 3ds Max, Ansehen, Startansicht; Stellen suchen (zwei Flaechen hintereinander, eine Flaeche allein)", [st] {
        modellBedienungSetzen(0);
        t().bereich = ModellBereich::Ansehen;
        t().vierAnsichten = false;
        t().aktiv = 3;
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        modellAllesEinblenden();
        t().auswahl.clear();
        t().eckenWahl.clear();
        *st = Stelle{};
        const ModellAnsicht& a = ansicht();
        const float schritt = std::max(em() * 0.5F, 4.0F);
        for (float y = a.lo[1] + em() * 3.0F; y < a.lo[1] + a.groesse[1] - em() * 2.0F; y += schritt) {
            for (float x = a.lo[0] + em(); x < a.lo[0] + a.groesse[0] - em(); x += schritt) {
                const std::vector<int> f = modellFlaechenUnter(a, ImVec2{x, y});
                if (f.empty() || modellTeilEingefroren(f[0])) { continue; }
                if (st->vorn < 0 && f.size() >= 2 && f[1] != f[0] && !modellTeilEingefroren(f[1])) {
                    st->p = ImVec2{x, y};
                    st->vorn = f[0];
                    st->hinten = f[1];
                }
                if (st->einzige < 0 && f.size() == 1) {
                    st->allein = ImVec2{x, y};
                    st->einzige = f[0];
                }
            }
        }
        melde("4b Stelle " + std::to_string(static_cast<int>(st->p.x)) + "/" + std::to_string(static_cast<int>(st->p.y)) + " vorn " + teilName(st->vorn) + ", dahinter " +
              teilName(st->hinten) + "; allein " + teilName(st->einzige) + " bei " + std::to_string(static_cast<int>(st->allein.x)) + "/" +
              std::to_string(static_cast<int>(st->allein.y)));
        if (st->vorn < 0 || st->einzige < 0) { meldeFehler("KLEIN 4b: keine Stelle mit zwei Flaechen hintereinander bzw. einer Flaeche allein gefunden"); }
    }));
    static const struct {
        AuswahlArt art;
        const char* name;
    } kArten[3] = {{AuswahlArt::Ecke, "Ecke"}, {AuswahlArt::Kante, "Kante"}, {AuswahlArt::Dreieck, "Dreieck"}};
    for (const auto& x : kArten) {
        const std::string name = x.name;
        const AuswahlArt art = x.art;
        add(tu("KLEIN 4b: Ebene " + name + ", nichts eingefroren, Auswahl leer", [st, art] {
            modellTeilEinfrieren(st->vorn, false, false);
            modellAuswahlArtSetzen(art);
            t().eckenWahl.clear();
        }));
        add(pause(0.2));
        add(klickAn("KLEIN 4b: Klick in die Ansicht (" + name + ", nichts eingefroren)", [st] { return st->p; }));
        add(pause(0.3));
        add(pruefe("KLEIN 4b " + name + ": ohne Einfrieren trifft der Klick die vordere Flaeche", [st, name] {
            melde("4b " + name + " frei: " + eckenTeile() + ", soll " + teilName(st->vorn));
            return st->vorn >= 0 && eckenNurVon(st->vorn);
        }));
        add(tu("KLEIN 4b: die vordere Flaeche einfrieren, Auswahl leer", [st] {
            modellTeilEinfrieren(st->vorn, true, false);
            t().eckenWahl.clear();
            t().meldung.clear();
        }));
        add(pause(0.2));
        add(klickAn("KLEIN 4b: Klick auf die eingefrorene Flaeche (" + name + ")", [st] { return st->p; }));
        add(pause(0.3));
        add(pruefe("KLEIN 4b " + name + ": eingefroren - der Klick geht hindurch und trifft die Flaeche dahinter (wie 3ds Max)", [st, name] {
            melde("4b " + name + " eingefroren: " + eckenTeile() + ", soll " + teilName(st->hinten) + ", Meldung '" + t().meldung + "'");
            return st->hinten >= 0 && modellTeilEingefroren(st->vorn) && eckenNurVon(st->hinten);
        }));
    }
    // nichts dahinter: nichts gewaehlt, die Statuszeile nennt die eingefrorene Flaeche (wie der Klick auf Flaechen, AP2 X13)
    add(tu("KLEIN 4b: Ebene Ecke, nur die einzelne Flaeche eingefroren, Auswahl leer", [st] {
        modellTeilEinfrieren(st->vorn, false, false);
        modellTeilEinfrieren(st->einzige, true, false);
        modellAuswahlArtSetzen(AuswahlArt::Ecke);
        t().eckenWahl.clear();
        t().meldung.clear();
    }));
    add(pause(0.2));
    add(klickAn("KLEIN 4b: Klick auf die Stelle, an der nur die eingefrorene Flaeche liegt", [st] { return st->allein; }));
    add(pause(0.3));
    add(pruefe("KLEIN 4b Ecke: eingefroren und nichts dahinter - der Klick waehlt nichts und meldet die eingefrorene Flaeche", [st] {
        melde("4b allein: " + eckenTeile() + ", Meldung '" + t().meldung + "'");
        return st->einzige >= 0 && t().eckenWahl.empty() && t().meldung.find(teilName(st->einzige)) != std::string::npos;
    }));
    // Rahmen um die Stelle mit zwei Flaechen: keine Ecke der eingefrorenen vorderen
    add(tu("KLEIN 4b: die vordere Flaeche eingefroren (die einzelne geloest), Ebene Ecke, Auswahl leer", [st] {
        modellTeilEinfrieren(st->einzige, false, false);
        modellTeilEinfrieren(st->vorn, true, false);
        t().eckenWahl.clear();
    }));
    add(pause(0.2));
    add(ziehenAn("KLEIN 4b: Rahmen um die Stelle", [st] { return ImVec2{st->p.x - em() * 3.0F, st->p.y - em() * 3.0F}; }, em() * 6.0F, em() * 6.0F, 0));
    add(pause(0.3));
    add(pruefe("KLEIN 4b Ecke: der Rahmen nimmt keine Ecke der eingefrorenen Flaeche (wie der Rahmen fuer Flaechen)", [st] {
        int vonVorn = 0;
        for (const modell::EckOrt& o : t().eckenWahl) { vonVorn += o.teil == st->vorn ? 1 : 0; }
        melde("4b Rahmen: " + eckenTeile() + ", davon eingefroren (" + teilName(st->vorn) + ") " + std::to_string(vonVorn));
        return st->vorn >= 0 && !t().eckenWahl.empty() && vonVorn == 0;
    }));
    // Bereich Modellieren: das Arbeitsnetz (Vielecke)
    add(tu("KLEIN 4b: Bereich Modellieren, Ebene Polygon, die vordere Flaeche eingefroren, Auswahl leer", [st] {
        t().bereich = ModellBereich::Modellieren;
        modellAuswahlArtSetzen(AuswahlArt::Dreieck);
        modellMeshNetzAktualisieren();
        md1d::mz().auswahl = modell::wof::PAuswahl{};
        t().meldung.clear();
    }));
    add(pause(0.4));
    // Der Bereich Modellieren zeichnet das Arbeitsnetz - an der Stelle aus dem Bereich Ansehen liegen dort nicht dieselben Flaechen
    // hintereinander (gesehen: nur head). Darum hier eine eigene Stelle suchen (nichts eingefroren, vorderste Flaeche zuerst wie
    // gezeichnet), die vordere einfrieren; erwartet wird die dahinter.
    add(tu("KLEIN 4b: Modellieren - eigene Stelle mit zwei Flaechen hintereinander, die vordere einfrieren", [st] {
        modellTeilEinfrieren(st->vorn, false, false);
        const ModellAnsicht& a = ansicht();
        const float schritt = std::max(em() * 0.5F, 4.0F);
        int vorn = -1;
        int hinten = -1;
        ImVec2 p{};
        for (float y = a.lo[1] + em() * 3.0F; y < a.lo[1] + a.groesse[1] - em() * 2.0F && vorn < 0; y += schritt) {
            for (float x = a.lo[0] + em(); x < a.lo[0] + a.groesse[0] - em() && vorn < 0; x += schritt) {
                const std::vector<int> f = modellFlaechenUnter(a, ImVec2{x, y});
                if (f.size() >= 2 && f[1] != f[0] && !modellTeilEingefroren(f[0]) && !modellTeilEingefroren(f[1])) {
                    vorn = f[0];
                    hinten = f[1];
                    p = ImVec2{x, y};
                }
            }
        }
        melde("4b Modellieren Stelle " + std::to_string(static_cast<int>(p.x)) + "/" + std::to_string(static_cast<int>(p.y)) + " vorn " + teilName(vorn) +
              ", dahinter " + teilName(hinten) + " (Bereich Ansehen: vorn " + teilName(st->vorn) + ", dahinter " + teilName(st->hinten) + ")");
        if (vorn < 0) { meldeFehler("KLEIN 4b Modellieren: keine Stelle mit zwei Flaechen hintereinander gefunden"); }
        st->vorn = vorn;
        st->hinten = hinten;
        st->p = p;
        modellTeilEinfrieren(st->vorn, true, false);
        md1d::mz().auswahl = modell::wof::PAuswahl{};
    }));
    add(pause(0.3));
    add(klickAn("KLEIN 4b: Klick auf die eingefrorene Flaeche (Modellieren, Polygon)", [st] { return st->p; }));
    add(pause(0.3));
    add(pruefe("KLEIN 4b Modellieren: eingefroren - der Klick waehlt ein Polygon der Flaeche dahinter", [st] {
        const ModellMesh& m = md1d::mz();
        std::string z;
        bool nurHinten = !m.auswahl.flaechen.empty();
        for (const int f : m.auswahl.flaechen) {
            const int teil = f >= 0 && static_cast<std::size_t>(f) < m.netz.flaechen.size() ? m.netz.flaechen[static_cast<std::size_t>(f)].teil : -1;
            z += " " + teilName(teil);
            nurHinten = nurHinten && teil == st->hinten;
        }
        melde("4b Modellieren: " + std::to_string(m.auswahl.flaechen.size()) + " Polygone von" + (z.empty() ? " -" : z) + ", soll " + teilName(st->hinten));
        return st->hinten >= 0 && nurHinten;
    }));
    add(tu("KLEIN 4b: Auswahl leer", [] { md1d::mz().auswahl = modell::wof::PAuswahl{}; }));
    add(ziehenAn("KLEIN 4b: Rahmen um die Stelle (Modellieren)", [st] { return ImVec2{st->p.x - em() * 3.0F, st->p.y - em() * 3.0F}; }, em() * 6.0F, em() * 6.0F, 0));
    add(pause(0.3));
    add(pruefe("KLEIN 4b Modellieren: der Rahmen nimmt kein Polygon der eingefrorenen Flaeche", [st] {
        const ModellMesh& m = md1d::mz();
        int vonVorn = 0;
        for (const int f : m.auswahl.flaechen) {
            vonVorn += f >= 0 && static_cast<std::size_t>(f) < m.netz.flaechen.size() && m.netz.flaechen[static_cast<std::size_t>(f)].teil == st->vorn ? 1 : 0;
        }
        melde("4b Modellieren Rahmen: " + std::to_string(m.auswahl.flaechen.size()) + " Polygone, davon eingefroren " + std::to_string(vonVorn));
        return st->vorn >= 0 && !m.auswahl.flaechen.empty() && vonVorn == 0;
    }));
    add(tu("KLEIN 4b: zurueck - geloest, Ansehen, Objekt, Auswahl leer", [st] {
        md1d::mz().auswahl = modell::wof::PAuswahl{};
        modellTeilEinfrieren(st->vorn, false, false);
        t().bereich = ModellBereich::Ansehen;
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        t().eckenWahl.clear();
        t().auswahl.clear();
    }));
    add(pause(0.2));
}

// --- 5: Home gehoert zur Figur und bleibt ----------------------------------------------------------------------------------

std::string homeDatei() { return platform::executableDirectory() + "\\klein_home_erwartet.txt"; }

struct Home {
    bool da = false;
    float w[3]{};
    float abstand = 0.0F;
    float ziel[3]{};
};
Home homeJetzt() {
    Home h;
    h.da = t().homeDa;
    for (int k = 0; k < 3; ++k) {
        h.w[k] = t().homeWinkel[k];
        h.ziel[k] = t().homeZiel[k];
    }
    h.abstand = t().homeAbstand;
    return h;
}
float homeAbweichung(const Home& a, const Home& b) {
    float d = std::fabs(a.abstand - b.abstand);
    for (int k = 0; k < 3; ++k) { d = std::max(d, std::max(std::fabs(a.w[k] - b.w[k]), std::fabs(a.ziel[k] - b.ziel[k]))); }
    return d;
}
std::string homeText(const Home& h) {
    char z[200];
    std::snprintf(z, sizeof z, "%s Winkel %.3f %.3f %.3f Abstand %.3f Ziel %.3f %.3f %.3f", h.da ? "gesetzt" : "nicht gesetzt", static_cast<double>(h.w[0]),
                  static_cast<double>(h.w[1]), static_cast<double>(h.w[2]), static_cast<double>(h.abstand), static_cast<double>(h.ziel[0]), static_cast<double>(h.ziel[1]),
                  static_cast<double>(h.ziel[2]));
    return z;
}

const char* const kHomeFigur = "models/players/kyle/model.glm";
const char* const kAndereFigur = "models/players/stormtrooper/model.glm";

void homeSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto gesetzt = std::make_shared<Home>();
    add(tu("KLEIN 5: wie 3ds Max, Ansehen, Blick von vorne links oben, naeher", [] {
        modellBedienungSetzen(0);
        t().bereich = ModellBereich::Ansehen;
        t().vierAnsichten = false;
        t().aktiv = 3;
        g_app->settings.modellNavigation = true;
        const float w[3] = {20.0F, 135.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        g_app->modelOrbit.zoom(g_app->modelCam, 3.0F);
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(klickAn("KLEIN 5: Rechtsklick auf den ViewCube", mbkopf::wuerfelOrt(0, -1, 0), 1));
    add(pause(0.3));
    add(klick(tr(Str::MkHomeSetzen)));
    add(pause(0.3));
    add(pruefe("KLEIN 5: Aktuelle Ansicht als Home setzen - Home ist gesetzt", [gesetzt] {
        *gesetzt = homeJetzt();
        melde("5 gesetzt: " + homeText(*gesetzt) + " fuer " + g_app->modelPath);
        return gesetzt->da;
    }));
    for (Schritt& x : mb1::laden(kAndereFigur)) { add(std::move(x)); }
    add(pause(0.4));
    add(pruefe("KLEIN 5: eine andere Figur hat kein gesetztes Home (es gehoert zur Figur)", [] {
        melde("5 andere Figur " + g_app->modelPath + ": " + homeText(homeJetzt()));
        return !t().homeDa;
    }));
    for (Schritt& x : mb1::laden(kHomeFigur)) { add(std::move(x)); }
    add(pause(0.4));
    add(pruefe("KLEIN 5: dieselbe Figur wieder geladen - ihr gesetztes Home ist wieder da (Winkel, Abstand, Drehpunkt)", [gesetzt] {
        const Home h = homeJetzt();
        melde("5 wieder geladen: " + homeText(h) + ", Abweichung " + std::to_string(homeAbweichung(h, *gesetzt)));
        return h.da && gesetzt->da && homeAbweichung(h, *gesetzt) < 0.01F;
    }));
    add(tu("KLEIN 5: das gesetzte Home fuer den zweiten Start aufschreiben", [gesetzt] {
        std::ofstream o(::bhed::utf8Pfad(homeDatei()));
        o << gesetzt->w[0] << " " << gesetzt->w[1] << " " << gesetzt->w[2] << " " << gesetzt->abstand << " " << gesetzt->ziel[0] << " " << gesetzt->ziel[1] << " "
          << gesetzt->ziel[2] << "\n";
    }));
}

// Der zweite Start (BHED_MODELL_NUR=kleinneustart, Einstellungen des ersten): Home ist noch da, der Home-Knopf fliegt dorthin
void neustartSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto soll = std::make_shared<Home>();
    add(tu("KLEIN 5 Neustart: Skript, wie 3ds Max; das im ersten Start gesetzte Home lesen", [soll] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "klein2.txt");
        modellBedienungSetzen(0);
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->settings.modellNavigation = true;
        std::ifstream f(::bhed::utf8Pfad(homeDatei()));
        soll->da = static_cast<bool>(f >> soll->w[0] >> soll->w[1] >> soll->w[2] >> soll->abstand >> soll->ziel[0] >> soll->ziel[1] >> soll->ziel[2]);
        if (!soll->da) { meldeFehler("KLEIN 5 Neustart: " + homeDatei() + " fehlt (der erste Start mb_klein lief nicht vor diesem)"); }
    }));
    for (Schritt& x : mb1::laden(kHomeFigur)) { add(std::move(x)); }
    add(pause(0.6));
    add(pruefe("KLEIN 5 Neustart: das gesetzte Home der Figur ist nach dem Neustart wieder da (Winkel, Abstand, Drehpunkt wie gesetzt)", [soll] {
        const Home h = homeJetzt();
        melde("5 Neustart: " + homeText(h) + "; gesetzt war " + homeText(*soll) + "; Abweichung " + std::to_string(homeAbweichung(h, *soll)));
        return soll->da && h.da && homeAbweichung(h, *soll) < 0.01F;
    }));
    add(klickAn("KLEIN 5 Neustart: ViewCube Home", mbkopf::wuerfelOrt(9, 9, 9)));
    add(pause(1.0));
    add(pruefe("KLEIN 5 Neustart: Home fliegt zur gesetzten Ansicht, nicht zur Startansicht", [soll] {
        float ab = std::fabs(g_app->modelOrbit.distance - soll->abstand);
        for (int k = 0; k < 2; ++k) { ab = std::max(ab, std::fabs(g_app->modelCam.angles[k] - soll->w[k])); }
        melde("5 Neustart Flug: Winkel " + std::to_string(g_app->modelCam.angles[0]) + " " + std::to_string(g_app->modelCam.angles[1]) + " Abstand " +
              std::to_string(g_app->modelOrbit.distance) + ", Abweichung " + std::to_string(ab));
        return soll->da && ansicht().animStart < 0.0 && ab < 0.01F;
    }));
    for (Schritt& x : mb1::laden(kAndereFigur)) { add(std::move(x)); }
    add(pause(0.4));
    add(pruefe("KLEIN 5 Neustart: eine andere Figur hat auch nach dem Neustart kein gesetztes Home", [] { return !t().homeDa; }));
    // Entscheidung A12 (12.11.): "Home zuruecksetzen" im Rechtsklick des ViewCube - blass ohne gesetztes Home
    add(klickAn("KLEIN 5: Rechtsklick auf den ViewCube (andere Figur, kein Home)", mbkopf::wuerfelOrt(0, -1, 0), 1));
    add(pause(0.3));
    add(pruefe("KLEIN 5: ohne gesetztes Home steht \"Home zuruecksetzen\" im Menue", [] { return finde(tr(Str::MkHomeZurueck), "##Popup", 0, false) != nullptr; }));
    add(klickAn("auf den blassen Eintrag Home zuruecksetzen", [] {   // klickAn: ein blasser Eintrag nimmt den Klick nicht an
        const Element* e = finde(tr(Str::MkHomeZurueck), "##Popup", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.2));
    add(pruefe("KLEIN 5: ... und ist blass - der Klick tut nichts, das Menue bleibt offen (ein aktiver Eintrag schloesse es)", [] {
        return finde(tr(Str::MkHomeZurueck), "##Popup", 0, false) != nullptr && !t().homeDa;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    for (Schritt& x : mb1::laden(kHomeFigur)) { add(std::move(x)); }
    add(pause(0.6));
    add(klickAn("KLEIN 5: Rechtsklick auf den ViewCube (Figur mit Home)", mbkopf::wuerfelOrt(0, -1, 0), 1));
    add(pause(0.3));
    add(foto("klein_home_zuruecksetzen"));
    add(klick(tr(Str::MkHomeZurueck), "##Popup"));
    add(pause(0.3));
    add(pruefe("KLEIN 5: Home zuruecksetzen - kein gesetztes Home mehr, der Eintrag der Figur in den Einstellungen ist weg", [] {
        const std::string pfad = "|" + g_app->modelPath;
        bool eintrag = false;
        for (const std::string& h : g_app->settings.modellHome) { eintrag = eintrag || (h.size() >= pfad.size() && h.compare(h.size() - pfad.size(), pfad.size(), pfad) == 0); }
        return !t().homeDa && !eintrag;
    }));
    add(klickAn("KLEIN 5: ViewCube Home nach dem Zuruecksetzen", mbkopf::wuerfelOrt(9, 9, 9)));
    add(pause(1.0));
    add(pruefe("KLEIN 5: Home fliegt wieder zur Startansicht (von vorne)", [soll] {
        const float ab = std::fabs(g_app->modelOrbit.distance - soll->abstand) + std::fabs(g_app->modelCam.angles[1] - soll->w[1]);
        melde("5 nach Zuruecksetzen: Winkel " + std::to_string(g_app->modelCam.angles[0]) + " " + std::to_string(g_app->modelCam.angles[1]) + " Abstand " +
              std::to_string(g_app->modelOrbit.distance));
        return ansicht().animStart < 0.0 && ab > 0.1F;
    }));
    add(tu("KLEIN 5 Neustart: zurueck", [] {
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
}

// --- 6: Kamera-Knopf des Gizmos --------------------------------------------------------------------------------------------

void gizmoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("KLEIN 6: wie Blender, eine Ansicht, Gizmo an, Maus weg vom Gizmo", [] {
        modellBedienungSetzen(1);
        t().bereich = ModellBereich::Ansehen;
        t().vierAnsichten = false;
        t().aktiv = 3;
        g_app->settings.modellNavigation = true;
        modellStartansicht(3);
        const ModellAnsicht& a = ansicht(3);
        setzeMaus(ImVec2{a.lo[0] + 40.0F, a.lo[1] + a.groesse[1] - 40.0F});
    }));
    add(pause(0.6));
    // Teile des Gizmos fuer modellGizmoOrt: 6 Zoom, 7 Verschieben, 8 Perspektive/Orthographisch, 9 Kamera
    add(pruefe("KLEIN 6: das Gizmo hat vier Knoepfe untereinander wie Blender - Zoom, Verschieben, Kamera, Perspektive/Orthographisch - in gleichem Abstand", [] {
        ImVec2 zoom;
        ImVec2 schieben;
        ImVec2 kamera;
        ImVec2 persp;
        const bool drei = modellGizmoOrt(3, 6, zoom) && modellGizmoOrt(3, 7, schieben) && modellGizmoOrt(3, 8, persp);
        const bool vier = modellGizmoOrt(3, 9, kamera);
        if (!drei || !vier) {
            melde(std::string("6 Gizmo: ") + (drei ? "Zoom, Verschieben, Perspektive da" : "Gizmo nicht da") + ", Kamera-Knopf " + (vier ? "da" : "fehlt"));
            return false;
        }
        const float d = schieben.y - zoom.y;
        melde("6 Gizmo: Zoom y " + std::to_string(zoom.y) + ", Verschieben " + std::to_string(schieben.y) + ", Kamera " + std::to_string(kamera.y) + ", Perspektive " +
              std::to_string(persp.y) + " (Abstand " + std::to_string(d) + ")");
        const ModellAnsicht& a = ansicht(3);
        return d > 4.0F && std::fabs(kamera.y - schieben.y - d) < 0.6F && std::fabs(persp.y - kamera.y - d) < 0.6F && std::fabs(kamera.x - zoom.x) < 0.6F &&
               persp.y + d * 0.5F <= a.lo[1] + a.groesse[1];
    }));
    auto vorher = std::make_shared<Camera>();
    auto artVorher = std::make_shared<AnsichtArt>(AnsichtArt::Perspektive);
    add(tu("KLEIN 6: Kamera und Art der Ansicht merken, Maus auf den Kamera-Knopf", [=] {
        *vorher = g_app->modelCam;
        *artVorher = ansicht(3).art;
        ImVec2 p;
        if (modellGizmoOrt(3, 9, p)) { setzeMaus(p); }
    }));
    add(pause(1.5));
    add(pruefe("KLEIN 6: der Kamera-Knopf zeigt unter der Maus seinen Tooltip (Kameraansicht - der Model-Tab hat keine Szenenkamera)", [] {
        ImVec2 p;
        return modellGizmoOrt(3, 9, p) && mbkopf::tooltipDa();
    }));
    add(foto("klein_gizmo_kamera"));
    add(tu("KLEIN 6: Meldung leeren", [] { t().meldung.clear(); }));
    add(klickAn("KLEIN 6: Klick auf den Kamera-Knopf", [] {
        ImVec2 p;
        if (modellGizmoOrt(3, 9, p)) { return p; }
        const ModellAnsicht& a = ansicht(3);
        return ImVec2{a.lo[0] + 40.0F, a.lo[1] + a.groesse[1] - 40.0F};   // ohne Knopf: eine leere Stelle
    }));
    add(pause(0.3));
    add(pruefe("KLEIN 6: der Klick laesst die Ansicht, wie sie ist, und sagt in der Statuszeile, dass der Model-Tab keine Szenenkamera hat", [=] {
        float ab = 0.0F;
        for (int k = 0; k < 3; ++k) { ab = std::max(ab, std::max(std::fabs(vorher->pos[k] - g_app->modelCam.pos[k]), std::fabs(vorher->angles[k] - g_app->modelCam.angles[k]))); }
        melde("6 Klick: Meldung '" + t().meldung + "', Kamera bewegt um " + std::to_string(ab));
        return ab < 0.001F && ansicht(3).art == *artVorher && t().meldung == tr(Str::MnBlKameraHint);
    }));
    // 6 b und c (Gegenkugel -Y ueber der Kugel Y, N des Kompasses hinter dem Wuerfel) hat MBRASTER behoben - geprueft in der Gruppe
    // kopf (Zeilen "MBKOPF A" und "MBKOPF B"). Hier die zwei Bilder zum Ansehen.
    auto kompass = std::make_shared<bool>(false);
    auto nord = std::make_shared<int>(0);
    add(tu("KLEIN 6: Blender, von vorne 10 Grad von oben (Y hinten und -Y vorne ueberdecken sich), Maus weg vom Gizmo", [] {
        const float w[3] = {10.0F, 90.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        const ModellAnsicht& a = ansicht(3);
        setzeMaus(ImVec2{a.lo[0] + 40.0F, a.lo[1] + a.groesse[1] - 40.0F});
    }));
    add(pause(0.5));
    add(foto("klein_gizmo_gegenkugel"));
    add(tu("KLEIN 6: wie 3ds Max, Kompass an, von vorne 25 Grad von oben (N liegt hinter dem Wuerfel), Maus auf den Wuerfel", [=] {
        *kompass = g_app->settings.modellKompass;
        *nord = g_app->settings.modellKompassNord;
        modellBedienungSetzen(0);
        g_app->settings.modellKompass = true;
        g_app->settings.modellKompassNord = 0;
        const float w[3] = {25.0F, 90.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
    }));
    add(pause(0.5));
    add(tu("KLEIN 6: Maus auf die Flaeche Vorne des ViewCube (deckend)", [] {
        ImVec2 p;
        if (modellWuerfelOrt(3, 0, -1, 0, p)) { setzeMaus(p); }
    }));
    add(pause(0.5));
    add(foto("klein_kompass_n"));
    add(tu("KLEIN 6: Kompass wie vorher, Startansicht", [=] {
        g_app->settings.modellKompass = *kompass;
        g_app->settings.modellKompassNord = *nord;
        modellStartansicht(3);
        const ModellAnsicht& a = ansicht(3);
        setzeMaus(ImVec2{a.lo[0] + 40.0F, a.lo[1] + a.groesse[1] - 40.0F});
    }));
    add(pause(0.3));
}

// --- 10: About bei Schrift 24 ------------------------------------------------------------------------------------------------

void aboutSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto skala = std::make_shared<float>(1.0F);
    static const struct {
        float schrift;
        const char* name;
    } kFaelle[2] = {{24.0F, "24"}, {16.0F, "16"}};
    add(tu("KLEIN 10: Fenster 1920x1080, Skripttab", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(1920, 1080);
        g_app->leftMode = 1;
    }));
    add(pause(0.6));
    for (const auto& f : kFaelle) {
        const float schrift = f.schrift;
        const std::string name = f.name;
        add(tu("KLEIN 10: Schrift " + name, [=] { g_app->uiScale = g_app->uiScale * schrift / std::max(ImGui::GetFontSize(), 1.0F); }));
        add(pause(0.8));
        add(tu("KLEIN 10: About auf", [] { g_app->aboutOpen = true; }));
        add(pause(0.8));
        add(pruefe("KLEIN 10: Schrift " + name + " im 1920x1080-Fenster - das About-Fenster passt ganz ins Programmfenster, Close, Changelog und Guide sind ganz zu sehen", [=] {
            const ImGuiWindow* w = ImGui::FindWindowByName(tr(Str::AboutTitle));
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            if (w == nullptr || !w->Active) {
                melde("10 Schrift " + name + ": About nicht offen");
                return false;
            }
            const ImRect r = w->Rect();
            const ImRect bild{vp->Pos, ImVec2{vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y}};
            bool knoepfe = true;
            std::string z;
            for (const Str k : {Str::AboutClose, Str::ChangelogMenu, Str::AboutGuide}) {
                const Element* e = finde(tr(k), tr(Str::AboutTitle), 0, false);
                const bool da = e != nullptr && ganzSichtbar(*e) && bild.Contains(e->rect);
                knoepfe = knoepfe && da;
                z += std::string(" ") + tr(k) + (e == nullptr ? " fehlt" : (da ? " sichtbar" : " abgeschnitten (unten " + std::to_string(static_cast<int>(e->rect.Max.y)) + ")"));
            }
            melde("10 Schrift " + name + " (" + std::to_string(ImGui::GetFontSize()) + "): About " + std::to_string(static_cast<int>(r.GetWidth())) + " x " +
                  std::to_string(static_cast<int>(r.GetHeight())) + " bei " + std::to_string(static_cast<int>(r.Min.y)) + ".." + std::to_string(static_cast<int>(r.Max.y)) +
                  ", Programmfenster " + std::to_string(static_cast<int>(vp->Size.x)) + " x " + std::to_string(static_cast<int>(vp->Size.y)) + ";" + z);
            return std::fabs(ImGui::GetFontSize() - schrift) < 0.6F && bild.Contains(r) && knoepfe;
        }));
        add(foto("klein_about_schrift" + name));
        add(tu("KLEIN 10: About zu", [] { g_app->aboutOpen = false; }));
        add(pause(0.3));
    }
    add(tu("KLEIN 10: Schrift zurueck", [=] { g_app->uiScale = *skala; }));
    add(pause(0.6));
}

// --- 11: Meldungen des Rigs in der Zaehlung der Zeitleiste -----------------------------------------------------------------

// Was die Zeitleiste gerade als Bild zeigt (ihr Text beginnt mit der Zahl: "12 / 39   0.60 s")
int zeitleisteBild() {
    const std::vector<int> z = zahlenIn(t().zaehlungText);
    return z.empty() ? -1 : z.front();
}

void meldungSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    using ent2::f0;
    static const int kRel = 7;
    add(tu("KLEIN 11: Bereich Animation, Sequenz BOTH_WALK1, kein Rig, keine Schicht", [] {
        modellBedienungSetzen(0);
        t().bereich = ModellBereich::Animation;
        t().vierAnsichten = false;
        animWaehlen(mbefoto::sequenz(), false);
        t().rig = modell::Rig{};
        t().rigFuer.clear();
        t().griffWahl = -1;
        modellRigNurFkSetzen(false);
        t().dok.aendern("KLEIN leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->animPlaying = false;
    }));
    add(pause(0.6));
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        const std::string wie = bl ? "wie Blender" : "wie 3ds Max";
        add(tu("KLEIN 11: " + wie + ", Bild " + std::to_string(kRel) + " der Sequenz (ab 0)", [=] {
            modellBedienungSetzen(modus);
            g_app->animFrame = f0() + kRel;
            g_app->animPlaying = false;
            g_app->modelDirty = true;
            t().meldung.clear();
        }));
        add(pause(0.5));
        add(tu("KLEIN 11: Pose kopieren", [] { (void)modellPoseKopieren(); }));
        add(pruefe("KLEIN 11 " + wie + ": \"Pose kopiert ... an Bild\" nennt das Bild, das die Zeitleiste zeigt (" + std::string(bl ? "ab 1" : "ab 0") + "), nicht den Frame der GLA", [=] {
            const std::vector<int> z = zahlenIn(t().meldung);
            const int leiste = zeitleisteBild();
            melde("11 " + wie + " Pose kopieren: '" + t().meldung + "', Zeitleiste '" + t().zaehlungText + "', GLA-Frame " + std::to_string(g_app->animFrame));
            // die Sequenz beginnt weit hinter 0, sonst unterscheiden sich die Zaehlungen nicht
            return f0() > 100 && leiste == kRel + (bl ? 1 : 0) && z.size() == 2U && z[1] == leiste;
        }));
    }
    add(tu("KLEIN 11: wie 3ds Max, Control Rig erstellen (FK auf IK)", [] {
        modellBedienungSetzen(0);
        (void)modellAnimRigErstellen(false);
    }));
    add(pause(1.0));
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        const std::string wie = bl ? "wie Blender" : "wie 3ds Max";
        add(tu("KLEIN 11: " + wie + ", Bild " + std::to_string(kRel) + " der Sequenz, rechter Arm auf " + (bl ? "IK" : "FK") + " (angeglichen)", [=] {
            modellBedienungSetzen(modus);
            g_app->animFrame = f0() + kRel;
            g_app->animPlaying = false;
            g_app->modelDirty = true;
            t().meldung.clear();
        }));
        add(pause(0.5));
        add(tu("KLEIN 11: IK/FK umschalten", [=] { (void)modellIkWechselnRoh(static_cast<int>(modell::IkArt::ArmRechts), bl, true); }));
        add(pruefe("KLEIN 11 " + wie + ": \"... ist " + std::string(bl ? "IK" : "FK") + " von Bild a bis b\" zaehlt wie die Zeitleiste, nicht in Frames der GLA", [=] {
            const std::vector<int> z = zahlenIn(t().meldung);
            const int leiste = zeitleisteBild();
            const AnimEntry* e = ent2::seq();
            const int letztes = e != nullptr ? std::max(e->numFrames, 1) - 1 + (bl ? 1 : 0) : -1;
            melde("11 " + wie + " IK/FK: '" + t().meldung + "', Zeitleiste '" + t().zaehlungText + "', GLA-Frame " + std::to_string(g_app->animFrame));
            return f0() > 100 && leiste == kRel + (bl ? 1 : 0) && z.size() == 2U && z[0] == leiste && z[1] >= leiste && z[1] <= letztes;
        }));
    }
    add(tu("KLEIN 11: zurueck - wie 3ds Max, Rig und Schicht weg, Ansehen", [] {
        modellBedienungSetzen(0);
        t().rig = modell::Rig{};
        t().rigFuer.clear();
        t().dok.aendern("KLEIN leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        t().bereich = ModellBereich::Ansehen;
        g_app->animFrame = f0();
    }));
    add(pause(0.3));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n == "kleinneustart") {
        neustartSchritte(s);
        return s;
    }
    if (n != "klein") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto skala = std::make_shared<float>(1.0F);
    auto bedienung = std::make_shared<int>(0);
    auto teiler = std::make_shared<float>(0.0F);
    auto feldBreite = std::make_shared<float>(0.0F);
    // BHED_KLEIN_PUNKT=4a,4b,5,6,10,11: nur diese Punkte (Vorgabe alle); 5 gehoert in jeden Lauf, dem kleinneustart folgt
    const char* wahl = std::getenv("BHED_KLEIN_PUNKT");
    const std::string punkte = std::string(",") + (wahl != nullptr ? wahl : "4a,4b,5,6,10,11") + ",";
    const auto mit = [&](const char* p) { return punkte.find(std::string(",") + p + ",") != std::string::npos; };
    add(tu("KLEIN: Fenster 3840x2160, Oberflaeche 150 %, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(3840, 2160);
        g_app->uiScale = 1.5F;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "klein.txt");
    }));
    add(pause(0.8));
    if (mit("4a")) { artSchritte(s); }
    for (Schritt& x : mb1::laden(kHomeFigur)) { add(std::move(x)); }
    add(tu("KLEIN: Aufteilung wie neue Einstellungen, wie 3ds Max, Ansehen", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        t().bereich = ModellBereich::Ansehen;
        t().vierAnsichten = false;
        melde(g_app->modelPath + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen");
    }));
    add(pause(1.0));
    add(klickAn("KLEIN: in die Ansicht (Fokus, unten links)", [] {
        const ModellAnsicht& a = ansicht(3);
        return ImVec2{a.lo[0] + 20.0F, a.lo[1] + a.groesse[1] - 20.0F};
    }));
    add(pause(0.4));
    if (mit("4b")) { frostSchritte(s); }
    if (mit("6")) { gizmoSchritte(s); }
    if (mit("11")) { meldungSchritte(s); }
    if (mit("5")) { homeSchritte(s); }
    add(tu("KLEIN: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        t().bereich = ModellBereich::Ansehen;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.4));
    if (mit("10")) { aboutSchritte(s); }
    add(tu("KLEIN: Oberflaeche zurueck", [=] { g_app->uiScale = *skala; }));
    add(pause(0.4));
    return s;
}

}  // namespace kleinkram
