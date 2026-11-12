// selbsttest_fix612.inc - FIX612 (12.11.): die Funde der Code-Pruefung seit rc611 im laufenden Programm nachgestellt
// (plaene/codepruefung_seit_rc611.md R2-R7, V2/V3; codepruefung_seit_rc611_p3.md P3-1 bis P3-4).
//
//   BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=fix612   alle Teile des Model-Tabs; einzeln:
//     fix612r2    R2    Zeitleiste: Start-Feld (wie Blender) und Set Key bei offener Zeitkonfiguration (wie 3ds Max) ersetzen die
//                       Sequenzliste - die Leiste zeichnet in keinem Bild mit der Sequenz aus der alten Liste
//     fix612p31   P3-1  Presets: Klick und Rechtsklick auf "Gehen (erzeugt)", waehrend ein Preset der Bibliothek gewaehlt ist
//     fix612p33   P3-3  Platzliste der eigenen GLA: "Leeren" auf dem ersten von drei Plaetzen
//     fix612n     P3-2  doppelte Bodenzeile im Popover Overlays (wie Blender); P3-4 vier Ansichten nach Max -> Blender -> Max;
//                 R6    Fehler beim Lesen eines ModView-Skripts in der Sprache der Oberflaeche
//     fix612v     V2, V3: die Vermutungen der ersten Pruefung (Seitenleiste wie Blender, Zwischenspeicher des Explorers)
//   BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=fix612   R3: "Bauen fortsetzen" startet nichts aus dem Zwischenstand neben der
//                       Karte; nach dem Verschieben des Kartenordners gelten Verwerfen und Fortsetzen am neuen Ort
// R7 (Update-Leiste) steht in selbsttest_rest9.inc (BHED_EDITORTEST=updateleiste).
// Eingebunden in selbsttest.cpp (am Ende, benutzt die Helfer von ap1, ap4, ap5, ap10, mbefoto, vis2c).
namespace fix612 {

using ap10::zeige;
using mbefoto::inFenster;

void melde(const std::string& z) { diag::detail("Knopftest: FIX612 " + z); }

ModellTab& mtab() { return g_app->mtab; }

// --- R2: Zeitleiste -------------------------------------------------------------------------------------------------------

void zeitleisteSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto liste = std::make_shared<const AnimEntry*>(nullptr);
    auto veraltet = std::make_shared<int>(0);
    auto erstes = std::make_shared<int>(0);
    auto anzahl = std::make_shared<int>(0);
    // Weg A: wie Blender, das Feld Start der Knopfreihe
    add(tu("FIX612 R2 A: wie Blender, Ansehen/Pruefen, BOTH_WALK1 angehalten, Zeitkonfiguration zu", [] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Ansehen;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animTime = 0.0F;
        modellZeit().konfigOffen = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(tu("FIX612 R2 A: Liste, Zaehler und Marker merken", [=] {
        const AnimEntry* e = ap10::sequenzJetzt();
        *liste = g_app->animList.data();
        *veraltet = modellZeit().veralteteSequenz;
        *erstes = e != nullptr ? e->firstFrame : -1;
        *anzahl = e != nullptr ? e->numFrames : -1;
    }));
    add(klickAn("FIX612 R2 A: Feld Start der Zeitleiste", [] {
        const Element* e = findeEndung("##mzstart", "toolband");
        if (e == nullptr) { meldeFehler("FIX612 R2 A: das Feld Start steht nicht in der Zeitleiste"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.2));
    add(tippe("3"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.5));
    add(pruefe("FIX612 R2 A (wie Blender): Start = 3 getippt - der Marker der animation.cfg ist gesetzt, die Sequenzliste neu gelesen, und die "
               "Zeitleiste hat in keinem Bild mit der Sequenz aus der alten Liste gezeichnet", [=] {
        const AnimEntry* e = ap10::sequenzJetzt();
        const int n = modellZeit().veralteteSequenz - *veraltet;
        melde("R2 A: Anfang " + std::to_string(*erstes) + " -> " + std::to_string(e != nullptr ? e->firstFrame : -1) + ", Anzahl " + std::to_string(*anzahl) + " -> " +
              std::to_string(e != nullptr ? e->numFrames : -1) + ", Liste " + (g_app->animList.data() != *liste ? "neu gelesen" : "NICHT neu gelesen") + ", " + std::to_string(n) +
              " Bilder mit der alten Liste");
        return e != nullptr && *erstes >= 0 && e->firstFrame == *erstes + 2 && e->numFrames == *anzahl - 2 && g_app->animList.data() != *liste && n == 0;
    }));
    add(tu("FIX612 R2 A: Rueckgaengig", [] { (void)mtab().dok.rueckgaengig(); }));
    add(pause(0.4));
    add(pruefe("FIX612 R2 A: nach Rueckgaengig steht der Marker wie vorher", [=] {
        const AnimEntry* e = ap10::sequenzJetzt();
        return e != nullptr && e->firstFrame == *erstes && e->numFrames == *anzahl;
    }));
    // Weg B: wie 3ds Max, die Zeitkonfiguration ist offen, Set Key in der Knopfreihe aendert das Dokument
    add(tu("FIX612 R2 B: wie 3ds Max, Animation, Rig, Hand R gewaehlt, Auto Key an, Bild +5, Zeitkonfiguration offen", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
        mtab().vierAnsichten = false;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animTime = 0.0F;
        (void)modellRigAnlegen();
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->animFrame = kv5::f0() + 5;
        modellZeitAutoKey(true);
        modellZeit().konfigOffen = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(tu("FIX612 R2 B: die Zeitkonfiguration nach oben links (sie verdeckt sonst die Knopfreihe)", [] {
        for (ImGuiWindow* w : GImGui->Windows) {
            if (std::strstr(w->Name, "###mzkonfig") != nullptr) { ImGui::SetWindowPos(w, ImVec2{60.0F, 90.0F}); }
        }
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(0.4));
    add(tu("FIX612 R2 B: Liste und Zaehler merken", [=] {
        *liste = g_app->animList.data();
        *veraltet = modellZeit().veralteteSequenz;
    }));
    add(ap1::klickBand(tr(Str::MzSetKey)));
    add(pause(0.5));
    add(pruefe("FIX612 R2 B (wie 3ds Max): Set Key bei offener Zeitkonfiguration - der Schluessel ist gesetzt, die Sequenzliste neu gelesen, und "
               "weder die Leiste noch das Fenster haben mit der Sequenz aus der alten Liste gezeichnet", [=] {
        const int n = modellZeit().veralteteSequenz - *veraltet;
        const bool schluessel = kv5::schl(kv5::rhand(), kv5::f0() + 5) != nullptr;
        melde("R2 B: Zeitkonfiguration " + std::string(modellZeit().konfigOffen ? "offen" : "ZU") + ", Schluessel " + (schluessel ? "gesetzt" : "FEHLT") + ", Liste " +
              (g_app->animList.data() != *liste ? "neu gelesen" : "NICHT neu gelesen") + ", " + std::to_string(n) + " Bilder mit der alten Liste");
        return modellZeit().konfigOffen && schluessel && g_app->animList.data() != *liste && n == 0;
    }));
    add(tu("FIX612 R2 B: Zeitkonfiguration zu, Rueckgaengig, Auto Key aus", [] {
        modellZeit().konfigOffen = false;
        (void)mtab().dok.rueckgaengig();
        modellZeitAutoKey(false);
    }));
    add(pause(0.3));
}

// --- P3-1: Presets ----------------------------------------------------------------------------------------------------------

// Das erste Preset der Bibliothek in der Kategorie "walk" (data/presets/presets.txt)
std::string erstesGehPreset() {
    for (const PresetEintrag& e : modellPresetBibliothek()) {
        if (e.kategorie == "walk") { return e.name; }
    }
    return {};
}

void presetSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto probe = std::make_shared<int>(0);
    add(tu("FIX612 P3-1: wie 3ds Max, Animation, Presets offen - keine Wahl, Generator zu", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
        mtab().vierAnsichten = false;
        g_app->settings.modellFeldBreite = 0.0F;
        modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
        mtab().rt.bibliothekWahl = -1;
        mtab().rt.gangWahl = -1;
        mtab().rt.presetOeffnen = true;
        modellAnim().generatorOffen = false;
        (void)modellRtVorschau(false);
    }));
    add(pause(0.6));
    add(zeige(Str::PrRollout));
    add(pause(0.4));
    add(klick(tr(Str::AzKatGehen), "mtrollouts"));
    add(pause(0.4));
    for (int weg = 0; weg < 2; ++weg) {
        const std::string W = weg == 0 ? "Klick" : "Rechtsklick";
        add(klickAn("FIX612 P3-1: ein Preset der Bibliothek waehlen (Kategorie Gehen)", [] {
            const Element* e = inFenster(erstesGehPreset(), "##aibib");
            if (e == nullptr) { meldeFehler("FIX612 P3-1: kein Preset der Kategorie Gehen in der Liste"); }
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.4));
        add(pruefe("FIX612 P3-1 (" + W + "): ein Preset der Bibliothek ist gewaehlt", [=] {
            *probe = fix612Probe().presetWahlVeraltet;
            return mtab().rt.bibliothekWahl >= 0 && mtab().rt.gangWahl < 0;
        }));
        add(klickAn("FIX612 P3-1: " + W + " auf Gehen (erzeugt)", [] {
            const Element* e = inFenster(tr(Str::EnGangGehen), "##aibib");
            if (e == nullptr) { meldeFehler("FIX612 P3-1: Gehen (erzeugt) steht nicht in der Liste"); }
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }, weg));
        add(pause(0.5));
        add(pruefe("FIX612 P3-1 (" + W + "): Gehen (erzeugt) ist gewaehlt, und in keinem Bild hat die Gruppe den Eintrag der Bibliothek mit der "
                   "schon geloeschten Wahl gelesen (b[-1])", [=] {
            const int n = fix612Probe().presetWahlVeraltet - *probe;
            melde("P3-1 " + W + ": gangWahl " + std::to_string(mtab().rt.gangWahl) + ", bibliothekWahl " + std::to_string(mtab().rt.bibliothekWahl) + ", " + std::to_string(n) +
                  " Bilder mit der alten Wahl");
            return mtab().rt.gangWahl == 0 && mtab().rt.bibliothekWahl == -1 && n == 0;
        }));
        add(mbefoto::popupsZu());
        add(pause(0.3));
    }
    add(tu("FIX612 P3-1: Wahl weg", [] {
        mtab().rt.bibliothekWahl = -1;
        mtab().rt.gangWahl = -1;
    }));
}

// --- P3-3: Platzliste der eigenen GLA ---------------------------------------------------------------------------------------

void plaetzeSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto probe = std::make_shared<int>(0);
    add(tu("FIX612 P3-3: wie 3ds Max, Animation, Reiter Dienstprogramme, Ziel eigene GLA, drei Plaetze", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
        mtab().vierAnsichten = false;
        g_app->settings.modellFeldBreite = 0.0F;
        modellFeldReiter(MbReiter::Dienste, MbReiterBl::Daten);
        modellGlaZielSetzen(2);
        for (const char* n : {"BOTH_WALK1", "BOTH_RUN1", "BOTH_STAND1"}) { (void)modellAufFreienPlatz(mb1::animNach(n)); }
        ui::rolloutSetzen(tr(Str::MbRKartenGla), true);
    }));
    add(pause(0.8));
    add(zeige(Str::MbRKartenGla, 6.0F));
    add(pause(0.5));
    add(pruefe("FIX612 P3-3: die eigene GLA hat drei Plaetze, alle belegt und in der Liste zu sehen", [=] {
        int belegt = 0;
        for (const modell::Platz& p : mtab().plaetze) { belegt += p.belegt ? 1 : 0; }
        int zeilen = 0;
        for (const Element& e : g_elemente) { zeilen += e.innen.find("kgplaetze") != std::string::npos && e.label.find("###platz") != std::string::npos && e.label.find("###platzneu") == std::string::npos ? 1 : 0; }
        *probe = fix612Probe().platzHinterEnde;
        melde("P3-3: " + std::to_string(mtab().plaetze.size()) + " Plaetze, " + std::to_string(belegt) + " belegt, " + std::to_string(zeilen) + " Zeilen in der Liste");
        return mtab().glaZiel == 2 && mtab().plaetze.size() == 3U && belegt == 3 && zeilen == 3;
    }));
    add(klickAn("FIX612 P3-3: Rechtsklick auf den ersten Platz", [] {
        const std::string anfang = mtab().plaetze.empty() ? std::string("?") : mtab().plaetze.front().name + "   ";
        for (const Element& e : g_elemente) {
            if (e.innen.find("kgplaetze") != std::string::npos && e.label.rfind(anfang, 0) == 0) { return e.rect.GetCenter(); }
        }
        meldeFehler("FIX612 P3-3: der erste Platz steht nicht in der Liste");
        return ImVec2{-50.0F, -50.0F};
    }, 1));
    add(pause(0.4));
    add(ap1::klickPopup(tr(Str::MbKgLeeren)));
    add(pause(0.5));
    add(pruefe("FIX612 P3-3: Leeren auf dem ersten von drei Plaetzen - zwei Plaetze bleiben, und die Liste hat in keinem Bild eine Zeile hinter "
               "ihrem Ende gezeichnet", [=] {
        const int n = fix612Probe().platzHinterEnde - *probe;
        melde("P3-3: " + std::to_string(mtab().plaetze.size()) + " Plaetze nach Leeren, " + std::to_string(n) + " Zeilen hinter dem Ende");
        return mtab().plaetze.size() == 2U && n == 0;
    }));
    add(tu("FIX612 P3-3: Plaetze zurueck (Rueckgaengig bis leer), Ziel Karten-GLA, Reiter des Bereichs", [] {
        for (int i = 0; i < 4 && mtab().dok.kannRueckgaengig(); ++i) { (void)mtab().dok.rueckgaengig(); }
        modellGlaZielSetzen(0);
        modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
    }));
    add(pause(0.4));
}

// --- P3-2, P3-4, R6: die kleinen Funde ---------------------------------------------------------------------------------------

int imPopupZahl(const std::string& label) {
    int n = 0;
    for (const Element& e : g_elemente) {
        n += e.label == label && (e.fenster.find("##Popup_") != std::string::npos || e.fenster.find("##Menu_") != std::string::npos) ? 1 : 0;
    }
    return n;
}

void kleinSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // P3-2: wie Blender, Popover Overlays mit grauem Boden
    auto boden = std::make_shared<bool>(false);
    add(tu("FIX612 P3-2: wie Blender, Ansehen/Pruefen, grauer Boden an", [boden] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Ansehen;
        *boden = mtab().an.boden;
        mtab().an.boden = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(klickAn("FIX612 P3-2: Overlays", ap5::kopfFeld(3)));
    add(pause(0.5));
    add(pruefe("FIX612 P3-2 (wie Blender): im Popover Overlays steht die Bodenhoehe mit \"Tiefster Punkt\" genau einmal, und kein Element des "
               "Popovers traegt eine Kennung doppelt", [] {
        const int knoepfe = imPopupZahl(tr(Str::MrTiefster));
        int konflikte = 0;
        for (const elemente::IdKonflikt& k : elemente::idKonflikte()) { konflikte += k.fenster.find("##Popup_") != std::string::npos ? 1 : 0; }
        melde("P3-2: \"" + std::string(tr(Str::MrTiefster)) + "\" " + std::to_string(knoepfe) + " mal, " + std::to_string(konflikte) + " doppelte Kennungen im Popover");
        return knoepfe == 1 && konflikte == 0;
    }));
    add(mbefoto::popupsZu());
    add(tu("FIX612 P3-2: Boden wie vorher", [boden] {
        mtab().an.boden = *boden;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    // P3-4: wie 3ds Max -> wie Blender -> wie 3ds Max im Bereich Animation
    add(tu("FIX612 P3-4: wie 3ds Max, Bereich Animation, vier Ansichten", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
    }));
    add(pause(0.4));
    add(tu("FIX612 P3-4: vier Ansichten an (wie Alt+W)", [] { mtab().vierAnsichten = true; }));
    add(pause(0.4));
    add(tu("FIX612 P3-4: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.5));
    add(pruefe("FIX612 P3-4: wie Blender zeigt der Bereich eine Ansicht", [] { return !mtab().vierAnsichten; }));
    add(tu("FIX612 P3-4: wieder wie 3ds Max", [] { modellBedienungSetzen(0); }));
    add(pause(0.5));
    add(pruefe("FIX612 P3-4: wie 3ds Max -> wie Blender -> wie 3ds Max - der Bereich Animation zeigt wieder vier Ansichten", [] {
        melde("P3-4: vierAnsichten " + std::to_string(mtab().vierAnsichten ? 1 : 0));
        return mtab().vierAnsichten;
    }));
    add(tu("FIX612 P3-4: eine Ansicht", [] { mtab().vierAnsichten = false; }));
    add(pause(0.3));
    // R6: die Meldungen beim Lesen eines ModView-Skripts in der Sprache der Oberflaeche
    auto sprache = std::make_shared<i18n::Language>(i18n::Language::English);
    const auto ordner = [] { return std::filesystem::current_path() / "fix612_r6"; };
    add(tu("FIX612 R6: Skripte fuer den Test, Deutsch", [=] {
        std::error_code ec;
        std::filesystem::create_directories(ordner(), ec);
        const auto schreib = [&](const char* name, const std::string& text) {
            std::ofstream f(ordner() / name, std::ios::binary | std::ios::trunc);
            f << text;
        };
        schreib("ohne_loadmodel.mvs", "playsequence\n{\nname \"x\"\n}\n");
        schreib("ohne_modelfile.mvs", "loadmodel\n{\nname \"m\"\n}\n");
        schreib("klammer.mvs", "loadmodel\n{\nname \"m\"\n");
        schreib("kein_modell.mvs", "loadmodel\n{\nname \"m\"\nmodelfile \"models/players/fix612_gibt_es_nicht/model.glm\"\n}\n");
        *sprache = i18n::currentLanguage();
        i18n::setLanguage(i18n::Language::German);
    }));
    const auto lesen = [=](const char* name) {
        mtab().meldung.clear();
        const bool ok = modellSzeneLesen((ordner() / name).string());
        melde(std::string("R6 ") + name + ": " + (ok ? "gelesen" : "nicht gelesen") + " - \"" + mtab().meldung + "\"");
        return ok;
    };
    const auto mit = [](Str id, auto... a) {
        char z[400];
        std::snprintf(z, sizeof z, tr(id), a...);
        return std::string(z);
    };
    add(pruefe("FIX612 R6 (Deutsch): ein Skript, das sich nicht oeffnen laesst - die Meldung nennt den Grund auf Deutsch, nicht \"File not found\"", [=] {
        const bool ok = lesen("gibt_es_nicht.mvs");
        return !ok && mtab().meldung.find("File not found") == std::string::npos && mtab().meldung.find(tr(Str::PqSzeneNichtOffen)) != std::string::npos;
    }));
    add(pruefe("FIX612 R6 (Deutsch): Skript ohne loadmodel, ohne modelfile, mit offener Klammer - der Grund steht auf Deutsch da, nicht der englische Text", [=] {
        const bool a = !lesen("ohne_loadmodel.mvs") && mtab().meldung.find(mit(Str::PqSzeneKeinBlock, "loadmodel")) != std::string::npos &&
                       mtab().meldung.find("Unable to find") == std::string::npos;
        const bool b = !lesen("ohne_modelfile.mvs") && mtab().meldung.find(mit(Str::PqSzeneKeinSchluessel, "loadmodel", "modelfile")) != std::string::npos &&
                       mtab().meldung.find("keyword found") == std::string::npos;
        const bool c = !lesen("klammer.mvs") && mtab().meldung.find("Brace mismatch") == std::string::npos &&
                       mtab().meldung.find(std::string(tr(Str::PqSzeneKlammer)).substr(0, 12)) != std::string::npos;
        melde("R6 Deutsch: ohne loadmodel " + std::to_string(a) + ", ohne modelfile " + std::to_string(b) + ", Klammer " + std::to_string(c));
        return a && b && c;
    }));
    add(tu("FIX612 R6: Englisch", [] { i18n::setLanguage(i18n::Language::English); }));
    add(pruefe("FIX612 R6 (Englisch): ein Skript mit einem Modell, das es im Spiel nicht gibt - die Meldung ist englisch, nicht \"nicht in den Archiven\"", [=] {
        const bool ok = lesen("kein_modell.mvs");
        return !ok && mtab().meldung.find("nicht in den Archiven") == std::string::npos && mtab().meldung.find(tr(Str::MbNichtInSpieldateien)) != std::string::npos;
    }));
    add(tu("FIX612 R6: Sprache wie vorher", [=] {
        i18n::setLanguage(*sprache);
        mtab().meldung.clear();
    }));
}

// --- Die Vermutungen V2 und V3 der ersten Pruefung ---------------------------------------------------------------------------

void vermutungSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // V2: Seitenleiste (wie Blender) - ein Feld des Griffs ist angefasst (Wert getippt, kein Enter), dann waehlt ein Klick etwas anderes
    for (int weg = 0; weg < 2; ++weg) {
        const std::string W = weg == 0 ? "Klick ins Leere der Ansicht" : "Klick auf eine Flaeche im Outliner";
        add(tu("FIX612 V2: wie Blender, Animation, Rig, Griff Becken, Seitenleiste offen", [] {
            modellBedienungSetzen(1);
            mtab().bereich = ModellBereich::Animation;
            mtab().auswahl.clear();
            mtab().knochenWahl = -1;
            animWaehlen(mbefoto::sequenz(), false);
            g_app->animPlaying = false;
            if (!modellRigDa()) { (void)modellRigAnlegen(); }
            modellRigGriffWaehlen(ap4::griffNach(modell::GriffArt::Becken));
            modellBl().seitenleiste = true;
            modellBl().seitenReiter = 0;
            modellExplorerZeigen(ExplorerArt::Flaeche, mb1::teil("torso"));
            g_app->modelDirty = true;
        }));
        add(pause(0.8));
        add(klickAn("FIX612 V2: Klick auf Position X des Beckens (zum Eintippen)", ap4::feldOrt("ort0")));
        add(pause(0.3));
        add(Schritt{"FIX612 V2: Wert tippen, kein Enter", [](int b) {
                        if (b == 0) { return false; }
                        ImGuiIO& io = ImGui::GetIO();
                        if (b == 1) { io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); return false; }
                        if (b == 2) { io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                        if (b == 3) { io.AddInputCharactersUTF8("7.5"); return false; }
                        return b >= 6;
                    }});
        // Die Vermutung V2 setzte voraus, dass das getippte Feld einen Zug am Griff offen haelt - das tut es nicht (FIX612, rot und
        // gruen gleich: rigZieht 0 bei WantTextInput 1). Damit ist der vermutete Weg nicht erreichbar; die Pruefung haelt das fest.
        add(pruefe("FIX612 V2 (" + W + "): das Feld ist im Eingabemodus und haelt keinen Zug am Griff offen (die Vermutung V2 ist nicht erreichbar)", [] {
            melde("V2: rigZieht " + std::to_string(mtab().rigZieht ? 1 : 0) + ", WantTextInput " + std::to_string(ImGui::GetIO().WantTextInput ? 1 : 0));
            return !mtab().rigZieht && ImGui::GetIO().WantTextInput;
        }));
        if (weg == 0) {
            add(klickAn("FIX612 V2: Klick ins Leere der Ansicht", mbefoto::ansichtEcke));
        } else {
            add(klickAn("FIX612 V2: Klick auf die Flaeche torso im Outliner", [] {
                const Element* e = inFenster("torso", "mtbaum");
                if (e == nullptr) { meldeFehler("FIX612 V2: torso steht nicht im Outliner"); }
                return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
            }));
        }
        add(pause(0.6));
        add(pruefe("FIX612 V2 (" + W + "): danach laeuft kein Zug am Griff mehr (rigZieht aus)", [] {
            melde("V2: rigZieht " + std::to_string(mtab().rigZieht ? 1 : 0) + ", Griff " + std::to_string(mtab().griffWahl) + ", Flaechen " + std::to_string(mtab().auswahl.size()));
            return !mtab().rigZieht;
        }));
        add(tu("FIX612 V2: aufraeumen", [] {
            modellRigZiehenEnde();
            mtab().auswahl.clear();
            for (int i = 0; i < 2 && mtab().dok.kannRueckgaengig() && mtab().dok.rueckgaengigName().find("X") != std::string::npos; ++i) { (void)mtab().dok.rueckgaengig(); }
        }));
        add(pause(0.3));
    }
    add(tu("FIX612 V2: Seitenleiste zu", [] { modellBl().seitenleiste = false; }));
    // V3: Szenen-Explorer - zwei neue Sequenzlisten gleicher Laenge, ohne dass der Explorer dazwischen zeichnet
    auto alt = std::make_shared<std::vector<AnimEntry>>();
    auto gleich = std::make_shared<bool>(false);
    add(tu("FIX612 V3: wie 3ds Max, Ansehen/Pruefen, Explorer wie nach dem Laden, Sequenzen der Gruppe FACE_", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        modellExplorerVorgabe();
        modellExplorer().seqGruppe = 4;
        int erste = -1;
        for (std::size_t i = 0; i < g_app->animList.size() && erste < 0; ++i) {
            if (g_app->animList[i].name.rfind("FACE_", 0) == 0) { erste = static_cast<int>(i); }
        }
        modellExplorerZeigen(ExplorerArt::Sequenz, erste);
    }));
    add(pause(0.8));
    const auto faceZeilen = [](int& alle) {
        int face = 0;
        alle = 0;
        for (const ExplorerLage& l : modellExplorer().lage) {
            if (l.art != ExplorerArt::Sequenz) { continue; }
            ++alle;
            face += l.name.rfind("FACE_", 0) == 0 ? 1 : 0;
        }
        return face;
    };
    add(pruefe("FIX612 V3: der Explorer zeigt die Sequenzen der Gruppe FACE_ (Vorbedingung)", [=] {
        int alle = 0;
        const int face = faceZeilen(alle);
        melde("V3 vorher: " + std::to_string(alle) + " Sequenzzeilen, davon " + std::to_string(face) + " FACE_");
        return alle > 0 && face == alle;
    }));
    add(tu("FIX612 V3: die Liste zweimal ersetzen (wie zwei Ladevorgaenge), die zweite in umgekehrter Folge", [=] {
        *alt = g_app->animList;
        const AnimEntry* adresse = g_app->animList.data();
        std::vector<AnimEntry> zwischen(*alt);
        g_app->animList = std::move(zwischen);   // die alte Liste ist frei
        *gleich = false;
        std::vector<std::vector<AnimEntry>> versuche;
        for (int i = 0; i < 8 && !*gleich; ++i) {
            versuche.emplace_back(alt->rbegin(), alt->rend());
            *gleich = versuche.back().data() == adresse;
        }
        g_app->animList = std::move(versuche.back());
        melde(std::string("V3: die zweite neue Liste liegt ") + (*gleich ? "an der Adresse der ersten" : "an einer anderen Adresse") + ", " + std::to_string(g_app->animList.size()) + " Eintraege");
    }));
    add(pause(0.6));
    add(pruefe("FIX612 V3: nach zwei neuen Listen gleicher Laenge zeigt die Gruppe FACE_ weiter nur FACE_-Sequenzen", [=] {
        int alle = 0;
        const int face = faceZeilen(alle);
        melde("V3 nachher: " + std::to_string(alle) + " Sequenzzeilen, davon " + std::to_string(face) + " FACE_, Adresse " + (*gleich ? "gleich" : "anders"));
        return face == alle;
    }));
    add(tu("FIX612 V3: Liste und Explorer wie vorher", [=] {
        g_app->animList = *alt;
        modellExplorerVorgabe();
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
    }));
    add(pause(0.4));
}

std::vector<Schritt> modellSchritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n.rfind("fix612", 0) != 0) { return s; }
    const std::string teil = n.substr(6);
    // nur fix612 und seine Teile (fix612c gehoert selbsttest_fix612c.inc)
    if (!teil.empty() && teil != "r2" && teil != "p31" && teil != "p33" && teil != "n" && teil != "r4" && teil != "v") { return s; }
    const auto mit = [&](const char* t) { return teil.empty() || teil == t; };
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto skala = std::make_shared<float>(1.0F);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    auto autoKey = std::make_shared<bool>(false);
    add(tu("FIX612: Fenster 2560x1440, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(2560, 1440);
        g_app->uiScale = 1.0F;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "fix612.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("FIX612: Aufteilung wie neue Einstellungen, BOTH_WALK1 angehalten", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        *autoKey = modellZeit().autoKey;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        mtab().vierAnsichten = false;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animTime = 0.0F;
        mtab().fokus = true;
        g_app->modelDirty = true;
        melde(g_app->modelPath + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen, " + std::to_string(g_app->animList.size()) + " Sequenzen");
    }));
    add(pause(1.0));
    add(klickAn("FIX612: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (mit("r2")) { zeitleisteSchritte(s); }
    if (mit("p31")) { presetSchritte(s); }
    if (mit("p33")) { plaetzeSchritte(s); }
    if (mit("n")) { kleinSchritte(s); }
    if (mit("v")) { vermutungSchritte(s); }
    add(tu("FIX612: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->uiScale = *skala;
        mtab().bereich = ModellBereich::Ansehen;
        mtab().vierAnsichten = false;
        modellZeit().konfigOffen = false;
        modellZeitAutoKey(*autoKey);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

// --- R3: Bauen fortsetzen (Karten-Editor) -------------------------------------------------------------------------------------

namespace ke = karteneditor;

constexpr const char* kEintrag = "FIX612 Pause";

std::filesystem::path r3Wurzel() { return std::filesystem::current_path() / "fix612_r3"; }
std::string r3Stamm(const char* ordner) { return (r3Wurzel() / ordner / "halle").generic_string(); }

bool da(const std::string& pfad) {
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(pfad), ec);
}

std::string lies(const std::string& pfad) {
    std::ifstream f(std::filesystem::path(pfad), std::ios::binary);
    std::ostringstream t;
    t << f.rdbuf();
    return t.str();
}

bool konsoleMit(Str id) { return stufe3::konsoleText().find(stufe8::vorlageAnfang(id)) != std::string::npos; }

// Den Eintrag des Tests bauen und im ersten Schritt anhalten (bauAnhalten beendet einen BSP-Schritt und sichert den Stand)
Schritt anhalten(const std::string& was) {
    return tu("FIX612 R3: " + was + " - Bau starten und sofort anhalten", [] {
        ke::konsoleLeeren();
        const int nr = stufe3::eintragNr(kEintrag);
        if (nr < 0) {
            meldeFehler("FIX612 R3: der Eintrag des Tests fehlt im Build-Menue");
            return;
        }
        ke::bauStarten(static_cast<std::size_t>(nr));
        ke::bauAnhalten();
    });
}

Schritt ruhe(const std::string& was, double sekunden = 180) {
    return stufe8::warteSek("FIX612 R3: " + was, [] { return !ke::bauLaeuft(); }, sekunden);
}

std::vector<Schritt> bauFortsetzenSchritte() {
    std::vector<Schritt> s = stufe8b::splitEintritt();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto gestartet = std::make_shared<std::size_t>(0);
    add(warteBis("FIX612 R3: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("FIX612 R3: bhed_q3map2, Build-Menue frisch, Eintrag des Tests, Karte in a/", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        std::error_code ec;
        std::filesystem::remove_all(r3Wurzel(), ec);
        std::filesystem::create_directories(r3Wurzel() / "a", ec);
        {
            std::ofstream f(r3Wurzel() / "a" / "halle.map", std::ios::binary | std::ios::trunc);
            const std::string t = vis2c::gitterKarte(3);
            f.write(t.data(), static_cast<std::streamsize>(t.size()));
        }
        std::filesystem::remove(std::filesystem::path(ke::bauMenueDateiPfad()), ec);
        std::filesystem::remove(std::filesystem::path(stufe8::bauZeitenDatei()), ec);
        g_app->settings.karte.erase("BuildMenu");
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.bau = ke::Editor::BauEinstellungen{};
        e.bau.visExakt = true;
        e.bau.lichtWieder = true;
        e.bau.gpuBacken = karte::GpuBacken::Aus;
        ke::einstellungenSpeichern();
        (void)ke::karteOeffnen((r3Wurzel() / "a" / "halle.map").string());
        ke::konsoleLeeren();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{kEintrag,
                                                           {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -saveprt -threads 1 \"[MapFile]\"",
                                                            "[q3map2] -light -samples 2 -threads 2 \"[MapFile]\""}});
    }));
    add(pause(0.5));
    // --- (a) ein Zwischenstand wie aus fremder Hand ---
    add(anhalten("a/halle.map"));
    add(ruhe("angehalten", 60));
    add(pruefe("FIX612 R3: ehrlich angehalten - der Zwischenstand liegt neben der Karte (Schritt 1 von 3)", [] {
        const ke::BauPauseStand ps = ke::bauPauseStand();
        melde("R3: Zwischenstand " + std::string(ps.da ? "da" : "FEHLT") + ", Schritt " + std::to_string(ps.pause.schritt) + " von " + std::to_string(ps.pause.befehle.size()));
        return ps.da && ps.pause.schritt == 0 && ps.pause.befehle.size() == 3U && da(r3Stamm("a") + ".bhedpause/stand.json");
    }));
    add(tu("FIX612 R3a: stand.json wie aus fremder Hand - Schritt 1 nennt ein anderes Programm (eine Kopie des Compilers in einem fremden Ordner) und eine fremde Karte", [=] {
        std::error_code ec;
        const std::filesystem::path fremd = r3Wurzel() / "fremd";
        std::filesystem::create_directories(fremd, ec);
        std::filesystem::copy_file(std::filesystem::path(platform::executableDirectory()) / "bhed_q3map2.exe", fremd / "fremd_compiler.exe",
                                   std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::copy_file(r3Wurzel() / "a" / "halle.map", fremd / "fremd.map", std::filesystem::copy_options::overwrite_existing, ec);
        const std::string datei = r3Stamm("a") + ".bhedpause/stand.json";
        karte::BauPause p;
        if (!karte::bauPauseAusText(lies(datei), p) || p.befehle.empty()) {
            meldeFehler("FIX612 R3a: stand.json nicht lesbar");
            return;
        }
        // "<Programm>" ... "<Karte>": das Programm und die Karte ersetzen, die Schalter bleiben
        std::string b = p.befehle[0];
        const std::size_t q1 = b.find('"');
        const std::size_t q2 = q1 == std::string::npos ? q1 : b.find('"', q1 + 1);
        const std::size_t k2 = b.rfind('"');
        const std::size_t k1 = k2 == std::string::npos || k2 == 0 ? std::string::npos : b.rfind('"', k2 - 1);
        if (q2 == std::string::npos || k1 == std::string::npos || k1 <= q2) {
            meldeFehler("FIX612 R3a: der Befehl hat nicht die Form \"Programm\" ... \"Karte\": " + b);
            return;
        }
        b = "\"" + (fremd / "fremd_compiler.exe").generic_string() + "\"" + b.substr(q2 + 1, k1 - q2 - 1) + "\"" + (fremd / "fremd.map").generic_string() + "\"";
        p.befehle[0] = b;
        {
            std::ofstream f(std::filesystem::path(datei), std::ios::binary | std::ios::trunc);
            f << karte::bauPauseText(p);
        }
        melde("R3a: Schritt 1 im Stand: " + b);
        *gestartet = ke::editor().bauGestartet;
        ke::konsoleLeeren();
    }));
    add(pause(0.8));
    add(tu("FIX612 R3a: Bauen fortsetzen > Karte wie angehalten", [] { ke::bauFortsetzen(true); }));
    add(pause(0.5));
    add(ruhe("fremder Zwischenstand"));
    add(pruefe("FIX612 R3a: \"Karte wie angehalten\" startet das fremde Programm aus stand.json nicht - nichts laeuft, die Konsole nennt den Grund", [=] {
        const bool fremdGebaut = da((r3Wurzel() / "fremd" / "fremd.bsp").generic_string());
        const std::size_t bauten = ke::editor().bauGestartet - *gestartet;
        melde("R3a: fremde .bsp " + std::string(fremdGebaut ? "GEBAUT" : "nicht da") + ", " + std::to_string(bauten) + " Bauten gestartet, Meldung " +
              (konsoleMit(Str::KeBpAltNichtMoeglich) ? "da" : "FEHLT"));
        return !fremdGebaut && bauten == 0U && konsoleMit(Str::KeBpAltNichtMoeglich);
    }));
    add(tu("FIX612 R3a: Zwischenstand verwerfen", [] { ke::bauPauseVerwerfen(); }));
    add(pause(0.7));
    // --- (b) der Kartenordner ist verschoben: Verwerfen ---
    add(anhalten("a/halle.map"));
    add(ruhe("angehalten", 60));
    add(tu("FIX612 R3b: Ordner a in b umbenennen, die Karte dort oeffnen", [] {
        std::error_code ec;
        std::filesystem::rename(r3Wurzel() / "a", r3Wurzel() / "b", ec);
        if (ec) { meldeFehler("FIX612 R3b: der Ordner a liess sich nicht umbenennen: " + ec.message()); }
        (void)ke::karteOeffnen((r3Wurzel() / "b" / "halle.map").string());
        ke::konsoleLeeren();
    }));
    add(pause(0.8));
    add(pruefe("FIX612 R3b: nach dem Verschieben ist der Zwischenstand da (Ordner neben der Karte am neuen Ort)", [] {
        return ke::bauPauseStand().da && da(r3Stamm("b") + ".bhedpause/stand.json");
    }));
    add(tu("FIX612 R3b: Fenster \"Bauen fortsetzen\"", [] { ke::bauFortsetzenOeffnen(); }));
    add(pause(0.5));
    add(klick(ke::trK(Str::KeBpVerwerfen), "kartebaufortsetzen"));
    add(pause(0.8));
    add(pruefe("FIX612 R3b: \"Zwischenstand verwerfen\" nach dem Verschieben - der Ordner am neuen Ort ist weg, kein Zwischenstand mehr", [] {
        const bool ordner = da(r3Stamm("b") + ".bhedpause");
        melde("R3b: Ordner " + std::string(ordner ? "BLEIBT" : "weg") + ", Zwischenstand " + (ke::bauPauseStand().da ? "DA" : "weg") + ", alter Ort " + (da((r3Wurzel() / "a").generic_string()) ? "DA" : "weg"));
        return !ordner && !ke::bauPauseStand().da && !da((r3Wurzel() / "a").generic_string());
    }));
    add(tu("FIX612 R3b: Fenster zu, Reste weg", [] {
        karteDialogeZu();
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(r3Stamm("b") + ".bhedpause"), ec);
    }));
    add(pause(0.7));
    // --- (c) der Kartenordner ist verschoben: Karte wie angehalten ---
    add(anhalten("b/halle.map"));
    add(ruhe("angehalten", 60));
    add(tu("FIX612 R3c: Ordner b in c umbenennen, die Karte dort oeffnen, alte Ergebnisse weg", [=] {
        std::error_code ec;
        std::filesystem::rename(r3Wurzel() / "b", r3Wurzel() / "c", ec);
        if (ec) { meldeFehler("FIX612 R3c: der Ordner b liess sich nicht umbenennen: " + ec.message()); }
        std::filesystem::remove(std::filesystem::path(r3Stamm("c") + ".bsp"), ec);
        (void)ke::karteOeffnen((r3Wurzel() / "c" / "halle.map").string());
        ke::konsoleLeeren();
        *gestartet = ke::editor().bauGestartet;
    }));
    add(pause(0.8));
    add(tu("FIX612 R3c: Bauen fortsetzen > Karte wie angehalten", [] { ke::bauFortsetzen(true); }));
    add(pause(0.5));
    add(ruhe("Karte wie angehalten am neuen Ort", 300));
    add(pruefe("FIX612 R3c: ein ehrlich angehaltener Bau laesst sich nach dem Verschieben \"wie angehalten\" zu Ende fuehren - die .bsp liegt neben der "
               "Karte am neuen Ort, der Zwischenstand ist weg, am alten Ort entsteht nichts", [=] {
        const std::size_t lumps = stufe3::bspLumps(r3Stamm("c") + ".bsp").size();
        const bool alt = da((r3Wurzel() / "b").generic_string());
        melde("R3c: " + std::to_string(ke::editor().bauGestartet - *gestartet) + " Bauten, .bsp mit " + std::to_string(lumps) + " Lumps, Zwischenstand " +
              (da(r3Stamm("c") + ".bhedpause") ? "DA" : "weg") + ", alter Ort " + (alt ? "DA" : "weg") + ", fertig gemeldet " + (konsoleMit(Str::KeBpAltFertig) ? "ja" : "NEIN"));
        return lumps == 18U && !da(r3Stamm("c") + ".bhedpause") && !alt && konsoleMit(Str::KeBpAltFertig);
    }));
    add(tu("FIX612 R3: Fenster zu, zurueck zu Events", [] {
        karteDialogeZu();
        g_app->leftMode = 0;
    }));
    return s;
}

}  // namespace fix612
