// app_anordnung.inc - aus gui/app.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): pk3, Undo-Liste, Klaenge, Karte, Symbole, Anordnung, Compile-Knopf
//
// Eingebunden an genau dieser Stelle von gui/app.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: .pk3; Einen Schritt aus der MITTE zuruecknehmen; Die Undo-Liste, wie in 3ds Max; Klaenge; Karte; Symbole; Anordnung; Stufe 1b: der Arbeitsbereich Karten-Editor

// --- .pk3 --------------------------------------------------------------
//
// Skripte und Karten liegen fast nie ausgepackt herum, sondern in
// assets*.pk3 oder in der .pk3 der Mod. Ohne diesen Weg muesste man vorher
// von Hand auspacken.



void addGamePath() {
    // Ein echter Ordnerdialog.
    //
    // Vorher musste man eine .pk3 im gewuenschten Ordner auswaehlen, damit
    // das Programm den Ordner davon nehmen konnte - eine Kruecke, die auch
    // so im Quelltext stand. Der Common Item Dialog kann Ordner direkt.
    const std::string p =
        platform::pickFolder(tr(Str::Pk3AddPath), g_app->settings.lastDir);
    if (p.empty()) {
        return;
    }
    addGamePathDirectory(p);
}

// Eine Datei aus dem Archiv holen.

// Den Namen eines Bearbeitungsschritts uebersetzen.
const char* stepName(const char* key) {
    const std::string k = (key != nullptr) ? key : "";
    if (k == "insert") { return tr(Str::StepInsert); }
    if (k == "delete") { return tr(Str::StepDelete); }
    if (k == "clone") { return tr(Str::StepClone); }
    if (k == "moveup" || k == "movedown") { return tr(Str::StepMove); }
    if (k == "comment") { return tr(Str::StepComment); }
    if (k == "uncomment") { return tr(Str::StepUncomment); }
    if (k == "paste") { return tr(Str::StepPaste); }
    if (k == "selective") { return tr(Str::StepSelectiveUndo); }
    return tr(Str::StepEdit);
}

// Weiter unten (Aenderungsmarken): der Inhalt eines Befehls als Text.
std::string eigenerText(const Node& n);
// Was hat sich von a nach b geaendert? Eine Zeile wie
// "wait ( 4000 ) -> wait ( 3000 )", "+ print ( p )", "- wait ( 1 )", bei
// mehreren "(+N)". `ziel` bekommt den Weg des Befehls in b (leer, wenn er nur
// entfernt wurde).
std::string aenderungsText(const Script& a, const Script& b, Path* ziel);

namespace {

// Jeder Befehl eines Skripts mit Kennung, eigenem Text, Weg und Reihenfolge.
struct Flach {
    const Node* knoten = nullptr;
    std::string text;
    Path weg;
    std::size_t ord = 0;
};

void flachen(const std::vector<Node>& ns, Path& weg, std::map<Kennung, Flach>& out, std::size_t& ord) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        weg.push_back(i);
        const Node& n = ns[i];
        if (n.kind != Node::Kind::Blank && n.kennung != 0) {
            out[n.kennung] = Flach{&n, eigenerText(n), weg, ord++};
        }
        flachen(n.children, weg, out, ord);
        weg.pop_back();
    }
}

std::string kurzZeile(const Node& n) {
    std::string z = rowText(n, g_app->treeOpt);
    constexpr std::size_t kMax = 60;
    if (z.size() > kMax) {
        z = z.substr(0, kMax) + "...";
    }
    return z;
}

// Nach einem Undo/Redo: den geaenderten Befehl waehlen, aufklappen und
// ins Bild holen - sonst sieht man nicht, WAS sich geaendert hat.
void zeigeGeaendert(const Path& ziel) {
    g_app->selectedPath.clear();
    g_app->selection.clear();
    if (!ziel.empty() && nodeAt(g_app->doc.script(), ziel) != nullptr) {
        for (std::size_t t = 1; t < ziel.size(); ++t) {
            g_app->expanded.setOpen(kennungFuer(Path(ziel.begin(), ziel.begin() + static_cast<std::ptrdiff_t>(t))),
                                    true);
        }
        g_app->auswahlDurchEinfuegen = ziel;
        g_app->selectedPath = ziel;
        g_app->selection.assign(1, ziel);
        g_app->scrollToSelected = true;
    }
    rebuildTree();
}

void schritteAusfuehren(int anzahl, bool redo) {
    if (anzahl <= 0) {
        return;
    }
    g_app->doc.vergibKennungen();
    const Script vorher = g_app->doc.script();
    const char* was = redo ? g_app->doc.redoLabel() : g_app->doc.undoLabel();
    int getan = 0;
    for (int k = 0; k < anzahl; ++k) {
        if (!(redo ? g_app->doc.redo() : g_app->doc.undo())) {
            break;
        }
        ++getan;
    }
    if (getan == 0) {
        return;
    }
    // Beschrieben wird die AKTION, die zurueckgenommen (bzw. wiederholt)
    // wird - so wie man sie getan hat: beim Undo vom jetzigen Stand zum
    // vorigen, beim Redo vom vorigen zum jetzigen. Gewaehlt wird der Befehl
    // im jetzigen Stand.
    Path ziel;
    std::string text;
    if (redo) {
        text = aenderungsText(vorher, g_app->doc.script(), nullptr);
        (void)aenderungsText(g_app->doc.script(), vorher, &ziel);
    } else {
        text = aenderungsText(g_app->doc.script(), vorher, &ziel);
    }
    char kopf[400];
    if (getan == 1) {
        std::snprintf(kopf, sizeof(kopf), tr(redo ? Str::RedoOf : Str::UndoOf),
                      text.empty() ? stepName(was) : text.c_str());
        g_app->undoMeldung = kopf;
    } else {
        std::snprintf(kopf, sizeof(kopf), tr(redo ? Str::RedoSteps : Str::UndoSteps), getan);
        g_app->undoMeldung = text.empty() ? std::string(kopf) : std::string(kopf) + " - " + text;
    }
    g_app->undoMeldungRedo = redo;
    g_app->undoMeldungTiefe = g_app->doc.undoDepth();
    diag::detail(std::string("Undo-Meldung: ") + g_app->undoMeldung);
    zeigeGeaendert(ziel);
}

}  // namespace

std::string aenderungsText(const Script& a, const Script& b, Path* ziel) {
    // Beschreibt die AKTION a -> b so, wie man sie getan hat (shank: "er
    // sollte anzeigen was genau geundod wird ... nicht nur irgendwelche
    // sinnlosen Sachen"): "geaendert wait: 2000 -> 3000", "geloescht print
    // ( p )", "eingefuegt ...", "verschoben ...". `ziel` ist der Weg des
    // ersten betroffenen Befehls in a (bei einem eingefuegten: in b).
    if (ziel != nullptr) { ziel->clear(); }
    std::map<Kennung, Flach> fa;
    std::map<Kennung, Flach> fb;
    Path w;
    std::size_t ord = 0;
    flachen(a.nodes, w, fa, ord);
    ord = 0;
    flachen(b.nodes, w, fb, ord);
    // Zahl ohne ueberfluessige Nullen: "2000.000" -> "2000", "0.500" -> "0.5".
    const auto wert = [](std::string s) {
        const auto punkt = s.find('.');
        bool zahl = !s.empty() && punkt != std::string::npos;
        for (std::size_t i = 0; zahl && i < s.size(); ++i) {
            const char c = s[i];
            zahl = (std::isdigit(static_cast<unsigned char>(c)) != 0) || c == '.' || (c == '-' && i == 0);
        }
        if (zahl) {
            while (!s.empty() && s.back() == '0') { s.pop_back(); }
            if (!s.empty() && s.back() == '.') { s.pop_back(); }
        }
        return s;
    };
    struct Fund {
        std::size_t ord;
        std::string text;
        Path weg;
    };
    std::vector<Fund> funde;
    for (const auto& [k, eb] : fb) {
        const auto ea = fa.find(k);
        if (ea == fa.end()) {
            funde.push_back({eb.ord, std::string(tr(Str::ChangeAdded)) + " " + kurzZeile(*eb.knoten), eb.weg});
            continue;
        }
        if (ea->second.text == eb.text) {
            continue;
        }
        const Node& na = *ea->second.knoten;
        const Node& nb = *eb.knoten;
        std::string z = std::string(tr(Str::ChangeEdited)) + " ";
        if (na.name == nb.name && na.args.size() == nb.args.size() && !na.args.empty()) {
            // Nur das geaenderte Argument. Ist es nicht das erste, steht das
            // erste als Name dabei (set ( SET_X, ... ), camera ( MOVE, ... )).
            std::size_t erstes = na.args.size();
            int anders = 0;
            for (std::size_t i = 0; i < na.args.size(); ++i) {
                if (na.args[i].text != nb.args[i].text) {
                    if (erstes == na.args.size()) { erstes = i; }
                    ++anders;
                }
            }
            if (erstes < na.args.size()) {
                z += nb.name;
                if (erstes > 0) { z += " " + nb.args[0].text; }
                z += ": " + wert(na.args[erstes].text) + " -> " + wert(nb.args[erstes].text);
                if (anders > 1) { z += " ..."; }
            } else {
                z += kurzZeile(na) + " -> " + kurzZeile(nb);
            }
        } else {
            z += kurzZeile(na) + " -> " + kurzZeile(nb);
        }
        funde.push_back({ea->second.ord, z, ea->second.weg});
    }
    for (const auto& [k, ea] : fa) {
        if (fb.count(k) == 0) {
            funde.push_back({ea.ord, std::string(tr(Str::ChangeRemoved)) + " " + kurzZeile(*ea.knoten), ea.weg});
        }
    }
    if (funde.empty()) {
        // Nur verschoben: der erste Befehl, dessen Platz sich geaendert hat.
        for (const auto& [k, eb] : fb) {
            const auto ea = fa.find(k);
            if (ea != fa.end() && ea->second.ord != eb.ord &&
                (funde.empty() || ea->second.ord < funde.front().ord)) {
                funde.assign(1, Fund{ea->second.ord, std::string(tr(Str::ChangeMoved)) + " " + kurzZeile(*eb.knoten),
                                     ea->second.weg});
            }
        }
    }
    if (funde.empty()) {
        return {};
    }
    std::sort(funde.begin(), funde.end(), [](const Fund& x, const Fund& y) { return x.ord < y.ord; });
    // Bis zu drei ausgeschrieben, damit man sieht, WAS alles betroffen ist.
    std::string out;
    const std::size_t zeigen = std::min<std::size_t>(funde.size(), 3);
    for (std::size_t i = 0; i < zeigen; ++i) {
        if (i > 0) { out += ";  "; }
        out += funde[i].text;
    }
    if (funde.size() > zeigen) {
        char z[64];
        std::snprintf(z, sizeof(z), tr(Str::RevertPreviewMore), static_cast<int>(funde.size() - zeigen));
        out += "  " + std::string(z);
    }
    if (ziel != nullptr) {
        *ziel = funde.front().weg;
    }
    return out;
}

namespace {

// Wo steht der Knoten mit dieser Kennung? Eltern-Liste und Stelle, sonst null.
std::vector<Node>* sucheKennung(std::vector<Node>& ns, Kennung k, std::size_t& stelle) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        if (ns[i].kennung == k) {
            stelle = i;
            return &ns;
        }
        if (std::vector<Node>* tief = sucheKennung(ns[i].children, k, stelle)) {
            return tief;
        }
    }
    return nullptr;
}

// Eltern-Kennung (0 = oberste Ebene) und Vorgaenger-Kennung (0 = erster) in a.
bool lageIn(const std::vector<Node>& ns, Kennung k, Kennung eltern, Kennung& outEltern, Kennung& outVor) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        if (ns[i].kennung == k) {
            outEltern = eltern;
            outVor = 0;
            for (std::size_t j = i; j-- > 0;) {
                if (ns[j].kennung != 0) { outVor = ns[j].kennung; break; }
            }
            return true;
        }
        if (lageIn(ns[i].children, k, ns[i].kennung, outEltern, outVor)) {
            return true;
        }
    }
    return false;
}

}  // namespace

// --- Einen Schritt aus der MITTE zuruecknehmen -----------------------------
//
// shank: "es waere auch nice, dass man in der Liste einen Undo-Befehl
// mittendrin anklicken kann und den dann undo machen kann und nicht der Reihe
// nach muss". Schritt i fuehrte von undoStand(i) (a) zu b. Zurueckgenommen
// wird NUR, was er getan hat, und zwar am JETZIGEN Stand - alles danach bleibt:
//   geaendert   -> der Befehl bekommt seine alten Felder (falls es ihn noch gibt)
//   eingefuegt  -> der Befehl wird entfernt (samt Block)
//   geloescht   -> der Befehl kommt wieder, hinter seinen alten Vorgaenger
//                  (sonst an den Anfang seines alten Blocks)
// Verschiebungen werden nicht zurueckgenommen. Das Ganze ist EIN neuer
// Schritt und laesst sich selbst wieder rueckgaengig machen.
bool schrittEinzelnZurueck(std::size_t i, std::string* bericht) {
    Document& d = g_app->doc;
    d.vergibKennungen();
    if (i >= d.undoDepth()) {
        return false;
    }
    const Script& a = d.undoStand(i);
    const Script& b = (i + 1 < d.undoDepth()) ? d.undoStand(i + 1) : d.script();
    std::map<Kennung, Flach> fa;
    std::map<Kennung, Flach> fb;
    Path w;
    std::size_t ord = 0;
    flachen(a.nodes, w, fa, ord);
    ord = 0;
    flachen(b.nodes, w, fb, ord);
    Script neu = d.script();
    int getan = 0;
    // Eingefuegt -> entfernen.
    for (const auto& [k, eb] : fb) {
        if (fa.count(k) != 0) { continue; }
        std::size_t stelle = 0;
        if (std::vector<Node>* liste = sucheKennung(neu.nodes, k, stelle)) {
            liste->erase(liste->begin() + static_cast<std::ptrdiff_t>(stelle));
            ++getan;
        }
    }
    // Geaendert -> alte Felder.
    for (const auto& [k, eb] : fb) {
        const auto ea = fa.find(k);
        if (ea == fa.end() || ea->second.text == eb.text) { continue; }
        std::size_t stelle = 0;
        if (std::vector<Node>* liste = sucheKennung(neu.nodes, k, stelle)) {
            Node alt = *ea->second.knoten;
            alt.children = std::move((*liste)[stelle].children);
            alt.kennung = k;
            (*liste)[stelle] = std::move(alt);
            ++getan;
        }
    }
    // Geloescht -> wieder einsetzen (in der Reihenfolge von a).
    std::vector<const Flach*> weg;
    for (const auto& [k, ea] : fa) {
        if (fb.count(k) == 0) { weg.push_back(&ea); }
    }
    std::sort(weg.begin(), weg.end(), [](const Flach* x, const Flach* y) { return x->ord < y->ord; });
    for (const Flach* f : weg) {
        // Ein Kind eines ebenfalls geloeschten Blocks kommt mit dem Block.
        Kennung eltern = 0;
        Kennung vor = 0;
        if (!lageIn(a.nodes, f->knoten->kennung, 0, eltern, vor)) { continue; }
        if (eltern != 0 && fb.count(eltern) == 0) { continue; }
        std::size_t stelle = 0;
        if (vor != 0) {
            if (std::vector<Node>* liste = sucheKennung(neu.nodes, vor, stelle)) {
                liste->insert(liste->begin() + static_cast<std::ptrdiff_t>(stelle) + 1, *f->knoten);
                ++getan;
                continue;
            }
        }
        if (eltern != 0) {
            if (std::vector<Node>* liste = sucheKennung(neu.nodes, eltern, stelle)) {
                auto& kinder = (*liste)[stelle].children;
                kinder.insert(kinder.begin(), *f->knoten);
                ++getan;
                continue;
            }
        }
        neu.nodes.insert(neu.nodes.begin(), *f->knoten);
        ++getan;
    }
    if (getan == 0) {
        return false;
    }
    // Benannt wird die AKTION, die zurueckgenommen wird - wie beim normalen Undo.
    const std::string text = aenderungsText(a, b, nullptr);
    if (bericht != nullptr) { *bericht = text; }
    return d.ersetzeSkript(std::move(neu), "selective");
}

void undoSchritte(int anzahl) { schritteAusfuehren(anzahl, false); }
void redoSchritte(int anzahl) { schritteAusfuehren(anzahl, true); }

void doUndo() { undoSchritte(1); }

void doRedo() { redoSchritte(1); }

// --- Die Undo-Liste, wie in 3ds Max -------------------------------------
//
// Ein Klappfenster mit allen Schritten, der neueste oben. Ein Klick auf einen
// Eintrag markiert ihn und alle darueber; "Undo" nimmt sie zurueck, "Cancel"
// schliesst. Doppelklick tut beides auf einmal.
void drawUndoListe() {
    // Die Eintraege aus dem Stapel. Neu gerechnet beim Oeffnen UND immer,
    // wenn sich das Dokument aendert, waehrend die Liste offen ist (shank:
    // "If I have the undo history window open and start doing CTRL Z/Y,
    // the window doesn't update").
    static std::uint64_t gefuellterStand = 0;
    static int gefuellterReiter = -1;
    const bool offen = ImGui::IsPopupOpen("##undoliste");
    const bool neuOeffnen = g_app->undoListeAnfrage != 0;
    if (neuOeffnen || (offen && (g_app->doc.stand() != gefuellterStand || g_app->activeTab != gefuellterReiter))) {
        if (neuOeffnen) {
            g_app->undoListeRedo = (g_app->undoListeAnfrage == 2);
            g_app->undoListeMarke = 0;
        }
        g_app->undoListeAnfrage = 0;
        gefuellterStand = g_app->doc.stand();
        gefuellterReiter = g_app->activeTab;
        g_app->undoListeEintraege.clear();
        Document& d = g_app->doc;
        d.vergibKennungen();
        if (!g_app->undoListeRedo) {
            // Schritt i fuehrte von undoStand(i) zum naechsten Stand.
            const std::size_t n = d.undoDepth();
            for (std::size_t j = 0; j < n; ++j) {
                const std::size_t i = n - 1 - j;
                const Script& nachher = (i + 1 < n) ? d.undoStand(i + 1) : d.script();
                const std::string t = aenderungsText(d.undoStand(i), nachher, nullptr);
                g_app->undoListeEintraege.push_back(t.empty() ? std::string(stepName(d.undoWas(i))) : t);
            }
        } else {
            const std::size_t n = d.redoDepth();
            for (std::size_t j = 0; j < n; ++j) {
                const Script& vorher = (j == 0) ? d.script() : d.redoStand(j - 1);
                const std::string t = aenderungsText(vorher, d.redoStand(j), nullptr);
                g_app->undoListeEintraege.push_back(t.empty() ? std::string(stepName(d.redoWas(j))) : t);
            }
        }
        g_app->undoListeMarke = std::clamp(g_app->undoListeMarke, 0,
                                           std::max(0, static_cast<int>(g_app->undoListeEintraege.size()) - 1));
        if (neuOeffnen) {
            ImGui::OpenPopup("##undoliste");
        }
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2{ImGui::GetFontSize() * 22.0F, 0.0F},
                                        ImVec2{ImGui::GetFontSize() * 64.0F, FLT_MAX});
    if (!ImGui::BeginPopup("##undoliste")) {
        return;
    }
    ImGui::TextDisabled("%s", tr(g_app->undoListeRedo ? Str::RedoListTitle : Str::UndoListTitle));
    const auto& e = g_app->undoListeEintraege;
    // Hoeher als zuerst (shank: "die Liste ist bisschen eng, kann man die
    // hoeher machen?"): mindestens 8, hoechstens 22 Zeilen, und so breit wie
    // der laengste Eintrag (in Grenzen).
    const ImGuiStyle& stil = ImGui::GetStyle();
    const float zeile = ImGui::GetTextLineHeight() + stil.ItemSpacing.y;
    const float hoehe = zeile * static_cast<float>(std::clamp<std::size_t>(e.size(), 8, 22)) +
                        stil.WindowPadding.y * 2.0F;
    float breite = ImGui::GetFontSize() * 30.0F;
    for (const std::string& x : e) {
        breite = std::max(breite, ImGui::CalcTextSize(x.c_str()).x + stil.WindowPadding.x * 2.0F + stil.ScrollbarSize);
    }
    breite = std::min(breite, ImGui::GetFontSize() * 60.0F);
    static bool bisHierZeigen = false;
    bool ausfuehren = false;
    if (ImGui::BeginChild("##undoeintraege", ImVec2{breite, hoehe}, ImGuiChildFlags_Borders)) {
        if (e.empty()) {
            ImGui::TextDisabled("%s", tr(Str::UndoListEmpty));
        }
        // Ein Klick markiert NUR diesen Eintrag (shank: "wenn ich auf einen
        // drauf klicke, markiert es immer alle direkt statt nur den einen").
        // Welche "Undo up to here" mitnaehme, zeigt sich blass, solange die
        // Maus ueber diesem Knopf steht (gemerkt aus dem vorigen Bild).
        for (std::size_t k = 0; k < e.size(); ++k) {
            ImGui::PushID(static_cast<int>(k));
            const bool gewaehlt = static_cast<int>(k) == g_app->undoListeMarke;
            const bool blass = bisHierZeigen && static_cast<int>(k) < g_app->undoListeMarke;
            if (blass) {
                ImVec4 c = ImGui::GetStyleColorVec4(ImGuiCol_Header);
                c.w *= 0.40F;
                ImGui::PushStyleColor(ImGuiCol_Header, c);
            }
            if (ImGui::Selectable(e[k].c_str(), gewaehlt || blass)) {
                g_app->undoListeMarke = static_cast<int>(k);
            }
            if (blass) {
                ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    const float knopf = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5F;
    ImGui::BeginDisabled(e.empty());
    if (ImGui::Button(tr(g_app->undoListeRedo ? Str::RedoUpToHere : Str::UndoUpToHere), ImVec2{knopf, 0.0F})) {
        ausfuehren = true;
    }
    ImGui::EndDisabled();
    bisHierZeigen = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    if (bisHierZeigen) {
        ImGui::SetTooltip("%s", tr(g_app->undoListeRedo ? Str::RedoUpToHereHint : Str::UndoUpToHereHint));
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EditorCancel), ImVec2{-FLT_MIN, 0.0F})) {
        ImGui::CloseCurrentPopup();
    }
    // Nur den angeklickten Schritt zuruecknehmen, die spaeteren bleiben.
    bool nurDieser = false;
    if (!g_app->undoListeRedo) {
        ImGui::BeginDisabled(e.empty());
        if (ImGui::Button(tr(Str::UndoOnlyThis), ImVec2{-FLT_MIN, 0.0F})) {
            nurDieser = true;
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", tr(Str::UndoOnlyThisHint));
        }
    }
    if (nurDieser && !e.empty()) {
        const std::size_t n = g_app->doc.undoDepth();
        const std::size_t k = static_cast<std::size_t>(std::max(0, g_app->undoListeMarke));
        std::string bericht;
        if (k < n && schrittEinzelnZurueck(n - 1 - k, &bericht)) {
            char z[400];
            std::snprintf(z, sizeof(z), tr(Str::UndoOf), bericht.empty() ? stepName("selective") : bericht.c_str());
            g_app->undoMeldung = z;
            g_app->undoMeldungRedo = false;
            g_app->undoMeldungTiefe = g_app->doc.undoDepth();
            diag::detail("Undo-Meldung (einzeln): " + g_app->undoMeldung);
            g_app->selectedPath.clear();
            g_app->selection.clear();
            rebuildTree();
        }
        ImGui::CloseCurrentPopup();
    }
    if (ausfuehren && !e.empty()) {
        const int anzahl = g_app->undoListeMarke + 1;
        if (g_app->undoListeRedo) {
            redoSchritte(anzahl);
        } else {
            undoSchritte(anzahl);
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// --- Klaenge -----------------------------------------------------------
//
// Die Rueckfallregel der Engine: im Skript steht meist .wav, in der .pk3
// liegt aber eine .mp3. JKA sucht beide. Wer nur nach dem geschriebenen
// Namen sucht, findet fast nichts - genau daran ist EffectsEd gescheitert.
bool findSoundData(const std::string& name, std::string& out) {
    std::vector<std::string> tries{name};
    const std::size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) {
        const std::string base = name.substr(0, dot);
        tries.push_back(base + ".mp3");
        tries.push_back(base + ".wav");
    } else {
        tries.push_back(name + ".mp3");
        tries.push_back(name + ".wav");
    }
    for (const std::string& t : tries) {
        if (readFromArchives(t, out)) {
            return true;
        }
        // Auch ausgepackt neben dem Spielordner nachsehen (rc603: den
        // geltenden - auch denen dieser Sitzung).
        for (const std::string& dir : aktiveSpielordner()) {
            const std::string p = dir + "/" + t;
            std::ifstream f(p, std::ios::binary);
            if (f) {
                std::ostringstream ss;
                ss << f.rdbuf();
                out = ss.str();
                return true;
            }
        }
    }
    return false;
}


// Texturen zur geladenen Karte aus den .pk3 holen.
//
// Zwei Wege, wie es die Engine auch macht: erst die Datei zum Shadernamen
// direkt suchen, und nur wenn das nichts ergibt, in den .shader-Skripten
// nachsehen, welches Bild dahintersteckt.
//
// Die Bilder werden auf hoechstens 256 Punkte verkleinert. Eine Editoransicht
// braucht keine 1024er-Textur, und bei siebzig Shadern spart das ein
// Vielfaches an Speicher.

// --- Karte -------------------------------------------------------------
//
// Nur die Entities, nicht die Geometrie. Damit bieten die Textfelder die
// targetname aus der Karte an, statt dass man sie abtippt - das war der
// eigentliche Grund, ueberhaupt eine .bsp zu lesen.
// Eine Karte aus schon gelesenen Bytes uebernehmen.
//
// Getrennt vom Dateipfad, weil eine Karte aus zwei Quellen kommt: von der
// Platte und aus einem .pk3. Vorher gab es dafuer zwei Wege, und der aus dem
// Archiv las NUR die Entities - keine Geometrie, keine Texturen. Aus dem
// Archiv geladene Karten blieben also unsichtbar, und man sah nicht warum.



// "Export" - belegt durch ein Bildschirmfoto des Originals.
//
// BehavEd schreibt eine Textdatei und oeffnet sie im Standardeditor. Der
// Inhalt ist echter .icarus-Text mit einer Kopfzeile davor, die den
// urspruenglichen Dateinamen nennt; die Zahlen folgen dem Schalter
// "%g floats". In der Statusliste erscheint "( Exported )".
void doExport() {
    const std::string dir = platform::settingsDirectory();
    if (dir.empty()) {
        return;
    }
    const std::string out = dir + "/Export.txt";
    const std::string text = writeExport(g_app->doc.script(), g_app->path,
                                         g_app->treeOpt.gFloats);
    if (!spew(out, text)) {
        platform::showError(out, tr(Str::AppTitle));
        return;
    }
    addStatus(tr(Str::ExportDone));
    platform::openWithDefaultApp(out);
}

// "Backup" und "Restore" - belegt durch das BehavEd-Tutorial auf JKHub:
//     Backup:  Saves a copy to <Script Name>.bak
//     Restore: Reloads from the backup at <Script Name>.bak
//
// Beides braucht einen Dateinamen. Bei einem noch nie gespeicherten Skript
// gibt es keinen - dann wird gesagt, was fehlt, statt still nichts zu tun.
// Die Sicherung heisst wie das Skript, nur mit .bak STATT der Endung.
//
// Am laufenden Original beobachtet: aus `remtest.txt` wird `remtest.bak`.
// behaved schrieb `remtest.txt.bak` - eine Sicherung des Originals fand es
// nicht und umgekehrt.
std::string backupPathOf(const std::string& path) { return mitEndung(path, ".bak"); }

// Frueher von behaved geschriebene Sicherungen (`x.txt.bak`) findet Restore
// weiterhin, falls es keine `x.bak` gibt.
std::string alteBackupPathOf(const std::string& path) { return path + ".bak"; }

void doBackup() {
    // Original: "You can't backup until you've saved at least once".
    if (g_app->path.empty()) {
        addStatus(tr(Str::BackupNoFile));
        return;
    }
    // Den AKTUELLEN Stand, wie das Original: "Save directly to
    // <scriptname.bak>". Bis rc567 sicherte behaved hier die Datei auf der
    // Platte - Backup hiess damit etwas anderes als im Original.
    const std::string bak = backupPathOf(g_app->path);
    if (!spew(bak, writeScript(g_app->doc.script()))) {
        platform::showError(bak, tr(Str::AppTitle));
        return;
    }
    char msg[400];
    std::snprintf(msg, sizeof(msg), tr(Str::BackupDone), fileName(bak).c_str());
    addStatus(msg);
}

void doRestore() {
    std::string data;
    if (!g_app->path.empty()) {
        data = slurp(backupPathOf(g_app->path));
        if (data.empty()) {
            data = slurp(alteBackupPathOf(g_app->path));
        }
    }
    // Original: "No Backup file available to restore from". Vorher wurde
    // diese Meldung zusammengesetzt, aber nie ausgegeben.
    if (data.empty()) {
        addStatus(tr(Str::RestoreNone));
        return;
    }
    // Original: IMMER fragen - "This RESTORE will overwrite your current
    // script, proceed?". Vorher nur bei ungespeicherten Aenderungen.
    frage(tr(Str::RestoreAsk), [data] {
        Script sc;
        std::vector<Diag> d;
        (void)readScript(data, sc, d);
        // Als EIN Rueckgaengig-Schritt: Strg+Z holt den Stand vor dem
        // Restore zurueck (behaved hat Undo, das Original nicht).
        (void)g_app->doc.replaceAll(sc);
        g_app->selectedPath.clear();
        g_app->selection.clear();
        rebuildTree();
        addStatus(tr(Str::RestoreDone));
    });
}

void doCompile() {
    std::vector<IbiBlock> blocks;
    std::vector<Diag> d;
    const bool ok = compile(g_app->doc.script(), g_app->db, blocks, d);
    // Die Meldungen des Uebersetzers zeigen - im Original stehen sie in der
    // Statusliste als "(Zeile) : Text". Vorher wurden sie verworfen, und es
    // hiess nur "Compile failed".
    // In der eingestellten Sprache, mit dem Befehl davor - die Zeilennummer
    // kennt der Uebersetzer nicht (vorher stand immer "(0)"). Nach einem
    // Fehlschlag wird die erste schuldige Zeile angewaehlt.
    Path ersterFehler;
    for (const Diag& x : d) {
        const char* muster = nullptr;
        if (x.code == "C001") { muster = tr(Str::CmsgC001); }
        else if (x.code == "C002") { muster = tr(Str::CmsgC002); }
        else if (x.code == "C003") { muster = tr(Str::CmsgC003); }
        else if (x.code == "C004") { muster = tr(Str::CmsgC004); }
        else if (x.code == "C005") { muster = tr(Str::CmsgC005); }
        else if (x.code == "C006") { muster = tr(Str::CmsgC006); }
        char text[400];
        if (muster != nullptr) {
            std::snprintf(text, sizeof(text), muster, x.arg.c_str());
        } else {
            std::snprintf(text, sizeof(text), "%s", x.message.c_str());
        }
        const Node* wo = x.path.empty() ? nullptr : nodeAt(g_app->doc.script(), x.path);
        char z[480];
        if (wo != nullptr) {
            std::snprintf(z, sizeof(z), "%s : %s", wo->name.c_str(), text);
            if (ersterFehler.empty()) { ersterFehler = x.path; }
        } else {
            std::snprintf(z, sizeof(z), "%s", text);
        }
        addStatus(z);
    }
    if (!ok && !ersterFehler.empty()) {
        selectByPath(ersterFehler);
    }
    if (!ok) {
        // Bei einem Fehler KEINE .ibi schreiben - eine halbe wuerde das Spiel
        // sonst laden. Die Quelle wird trotzdem gesichert, wenn sie einen
        // Pfad hat (erst speichern, dann uebersetzen - wie das Original).
        if (!g_app->path.empty()) {
            (void)doSave();
        }
        addStatus(tr(Str::CompileFailed));
        g_app->kompiliertFehlerBis = ImGui::GetTime() + 2.0;
        g_app->kompiliertBis = 0.0;
        return;
    }
    const std::string bytes = writeIbi(blocks);
    std::string out = g_app->path;
    if (out.empty()) {
        out = platform::saveFileDialog(tr(Str::Compile), "IBI (*.ibi)|*.ibi",
                                       g_app->settings.scriptPath, "unnamed.ibi");
        if (out.empty()) {
            return;
        }
    } else {
        // mitEndung: ein Punkt im ORDNERNAMEN ist keine Endung. Vorher wurde
        // aus pruef.ordner\\skript die Datei pruef.ibi eine Ebene hoeher
        // (Code-Pruefung 03.10.).
        out = mitEndung(out, ".ibi");
    }
    if (!spew(out, bytes)) {
        platform::showError(out, tr(Str::AppTitle));
        return;
    }
    // --- Uebersetzen speichert die Quelle MIT ---------------------------
    //
    // shank: "when I compile a script, it doesn't save the file. When I
    // compile, it should automatically save." Genau so macht es BehavEd.
    //
    // Nach dem Dialog steht der gewaehlte Name in `out` - daraus leitet
    // `doSave` die .txt ab. Ohne diese Zeile wuesste es den Namen nicht.
    if (g_app->path.empty()) {
        g_app->path = out;
    }
    (void)doSave();
    char msg[200];
    std::snprintf(msg, sizeof(msg), tr(Str::CompileOk),
                  static_cast<int>(blocks.size()), static_cast<int>(bytes.size()));
    addStatus(msg);
    g_app->kompiliertBis = ImGui::GetTime() + 1.5;
    g_app->kompiliertFehlerBis = 0.0;
}

// Einen Befehl in den Baum einfuegen - hinter die Auswahl, sonst ans Ende.
//
// Die Regel ist seit rc533 wieder einfach:
//
//   IMMER hinter die Auswahl, auf DERSELBEN Ebene. Nie hinein.
//
// Vorher stand hier eine Sonderregel: ist ein Block gewaehlt und
// aufgeklappt (oder leer), landet der neue Befehl DRIN, an erster Stelle.
// Gut gemeint - "sonst bleiben Bloecke leer" -, aber falsch.
//
// shank zu rc532: "wenn ich links auf mehrere druecke nacheinander, macht
// er die sofort in ein plus untereinander. Das sollte so nicht sein von
// haus aus. Er sollte sie nur dann in das plus setzen wenn ich sie drauf
// ziehe."
//
// Er hat recht, und der Grund ist die KLEBRIGKEIT, nicht die Sonderregel
// selbst. Sie greift genau einmal - danach steht die Auswahl im Block, und
// "hinter die Auswahl" heisst von da an fuer immer "wieder im Block". Sein
// Protokoll zeigt es Schritt fuer Schritt:
//
//   Klick 1  loop      -> Knoten 0, Block, Auswahl [0]
//   Klick 2  wait      -> Block ist leer -> HINEIN, Auswahl [0,0]
//   Klick 3  move      -> hinter [0,0]              = [0,1]   drin
//   Klick 4  signal    -> hinter [0,1]              = [0,2]   drin
//
// Danach hat er vier Befehle einzeln wieder herausgezogen. Das Protokoll
// von rc532 zeigt genau diese vier Zuege.
//
// Und das Original macht es auch nicht: das ICARUS-Handbuch kennt nur
// "Add - Add selected command to the script window". Von Bloecken steht
// dort nichts.
//
// Wie fuellt man jetzt einen Block? Mit der Geste, die es dafuer gibt:
// den Befehl auf den Block ziehen. Das heisst HINEIN und ist seit rc521
// so - in shanks eigenem Protokoll steht die Zeile:
//
//   Ablegen: Ziel "loop", ist Block, Auswahl 1 -> Weg: HINEIN
//
// Eine Geste zum Anlegen, eine zum Einsortieren. Klicken sortiert nicht.
// Nachtrag 27.09.: "If I have a command block (affect, if, else, loop,
// task) selected ... and go to add command from the events panel or paste a
// command, the command should be added to it." Das geht jetzt, OHNE die
// Klebrigkeit von rc532 zurueckzuholen: hinein nur, wenn der Block durch
// einen KLICK angewaehlt ist (zielBlock), nicht weil er gerade eingefuegt
// wurde. Dann ans ENDE des Blocks; der neue Befehl wird angewaehlt, und die
// naechsten landen dahinter - also ebenfalls im Block, in Reihenfolge.
//
// Nachtrag, am laufenden Original nachgesehen (27.09.): hinein nur, wenn der
// Block bzw. das Makro AUFGEKLAPPT ist, und dann an den ANFANG. Ein
// zugeklappter nimmt nichts auf; der Befehl kommt dahinter.
Path zielBlock() {
    const Path& sel = g_app->selectedPath;
    if (sel.empty() || sel == g_app->auswahlDurchEinfuegen || g_app->selection.size() > 1) {
        return {};
    }
    const Node* n = nodeAt(g_app->doc.script(), sel);
    if (n == nullptr) {
        return {};
    }
    const bool behaelter = (n->kind == Node::Kind::Command && n->hasBlock) || n->kind == Node::Kind::Macro;
    return (behaelter && g_app->expanded.isOpen(kennungFuer(sel))) ? sel : Path{};
}

bool insertNodeSomewhere(Node n, Path* resultPath) {
    const Path block = zielBlock();
    if (!block.empty()) {
        // An den Anfang; bei einem Makro steht der Befehl flach direkt hinter
        // der Makrozeile.
        const bool makro = g_app->doc.istMakro(block);
        if (!g_app->doc.insertInto(block, 0, std::move(n))) {
            return false;
        }
        g_app->expanded.setOpen(kennungFuer(block), true);
        Path at = block;
        if (makro) {
            ++at.back();
        } else {
            at.push_back(0);
        }
        g_app->auswahlDurchEinfuegen = at;
        g_app->scrollToSelected = true;
        if (resultPath != nullptr) {
            *resultPath = at;
        }
        return true;
    }
    Path at = g_app->selectedPath;
    // Wohin es faellt, fragt das Dokument: hinter einer Makrozeile hinter das
    // ganze Makro.
    const Path neu = g_app->doc.wegHinter(at);
    if (!g_app->doc.insertAfter(at, std::move(n))) {
        return false;
    }
    at = neu;
    g_app->auswahlDurchEinfuegen = at;
    if (resultPath != nullptr) {
        *resultPath = at;
    }
    return true;
}


void insertCommand(const Command& c) {
    Node n = makeNode(c, g_app->db);
    // Ein neuer Blockbefehl wird gleich aufgeklappt - sonst sieht man
    // nicht, dass er einen Rumpf hat, und weiss nicht, wohin damit.
    const bool isBlock = n.hasBlock;
    Path at;
    if (!insertNodeSomewhere(std::move(n), &at)) {
        return;
    }
    if (isBlock) {
        g_app->expanded.setOpen(kennungFuer(at), true);
    }
    g_app->selectedPath = at;
    g_app->selection.assign(1, at);
    rebuildTree();
}

// Ein Makro einfuegen: Marker plus Rumpf, hinter die Auswahl.
// Was aus der Ereignisliste gezogen wurde, an der Zielstelle einsetzen.
//
// Getrennt von insertCommand: das setzt hinter die AUSWAHL und ist die
// Bedienung des Originals. Hier sagt der Ort der Maus, wohin - deshalb
// nimmt die Funktion Weg und Richtung entgegen und fasst die Auswahl nicht
// an, ausser um das Eingesetzte hervorzuheben.
// Die Knoten, die ein Zug aus der Ereignisliste erzeugt: ein Befehl oder
// die Zeilen eines Makros. Gebraucht vom Hauptbaum (dropNewAt) und von den
// Nebenfeldern - eine Fassung fuer beide.
std::vector<Node> knotenFuerNeu(const NewDrag& what);
std::vector<Node> knotenAusNeuNutzlastIntern(const void* daten, int groesse) {
    if (daten == nullptr || groesse != static_cast<int>(sizeof(NewDrag))) {
        return {};
    }
    NewDrag neu{};
    std::memcpy(&neu, daten, sizeof(neu));
    return knotenFuerNeu(neu);
}

std::vector<Node> knotenFuerNeu(const NewDrag& what) {
    std::vector<Node> nodes;
    if (what.kind == 0) {
        if (what.index < 0 ||
            static_cast<std::size_t>(what.index) >= g_app->db.commands.size()) {
            return nodes;
        }
        nodes.push_back(makeNode(g_app->db.commands[
                                     static_cast<std::size_t>(what.index)],
                                 g_app->db));
    } else {
        if (what.index < 0 ||
            static_cast<std::size_t>(what.index) >= g_app->db.macroBodies.size()) {
            return nodes;
        }
        // Ein Makro ist Marker PLUS Rumpf, also mehrere Knoten. Sie muessen
        // in der richtigen Reihenfolge landen - siehe dropNewAt.
        nodes = makeMacroNodes(g_app->db.macroBodies[
                                   static_cast<std::size_t>(what.index)],
                               g_app->db);
    }
    return nodes;
}

void dropNewAt(const NewDrag& what, const Path& target, DropWhere where) {
    const std::vector<Node> nodes = knotenFuerNeu(what);
    if (nodes.empty()) {
        return;
    }

    Path at;
    bool ok = false;
    const bool neuMitMakro = std::any_of(nodes.begin(), nodes.end(),
                                         [](const Node& n) { return n.kind == Node::Kind::Macro; });
    if (where == DropWhere::Into && g_app->doc.istMakro(target) && neuMitMakro) {
        where = DropWhere::After;   // ein Makro nicht ins Makro - dahinter
    }
    if (where == DropWhere::Into) {
        // In einen Block: vorne hinein, damit man sieht, dass etwas
        // passiert ist. Ans Ende geschoben waere es bei einem langen Block
        // ausserhalb des sichtbaren Bereichs. Ein Makro ebenso (im Original
        // "ganz oben"); seine Befehle stehen flach hinter der Makrozeile.
        const bool makro = g_app->doc.istMakro(target);
        std::size_t k = 0;
        for (const Node& n : nodes) {
            if (!g_app->doc.insertInto(target, k, n)) { break; }
            ++k;
        }
        ok = k != 0;
        if (ok) {
            g_app->expanded.setOpen(kennungFuer(target), true);
            at = target;
            if (makro) {
                at.back() += k;
            } else {
                at.push_back(k - 1);
            }
        }
    } else if (where == DropWhere::Before) {
        // Rueckwaerts einsetzen: jedes insertBefore schiebt das vorher
        // Eingesetzte weiter nach hinten. Von hinten nach vorne gearbeitet
        // steht am Ende die richtige Reihenfolge da.
        for (std::size_t k = nodes.size(); k-- > 0;) {
            if (!g_app->doc.insertBefore(target, nodes[k])) { break; }
            ok = true;
        }
        at = target;
    } else {
        Path after = target;
        for (const Node& n : nodes) {
            if (!g_app->doc.insertAfter(after, n)) { break; }
            ok = true;
            if (after.empty()) {
                after.push_back(g_app->doc.script().nodes.size() - 1);
            } else {
                ++after.back();
            }
        }
        at = after;
    }
    if (!ok) {
        return;
    }
    // Einen neuen Block gleich aufklappen - sonst sieht man nicht, dass er
    // einen Rumpf hat.
    const Node* placed = at.empty() ? nullptr : nodeAt(g_app->doc.script(), at);
    if (placed != nullptr && placed->hasBlock) {
        g_app->expanded.setOpen(kennungFuer(at), true);
    }
    g_app->selectedPath = at;
    g_app->selection.assign(1, at);
    g_app->auswahlDurchEinfuegen = at;   // siehe zielBlock
}

void insertMacro(const Macro& m) {
    const std::vector<Node> nodes = makeMacroNodes(m, g_app->db);
    // Wie bei insertCommand: hinter die Auswahl, nie hinein. Hier stand
    // dieselbe Sonderregel ("auch Makros gehoeren in einen offenen Block
    // hinein"), und sie hatte dieselbe Wirkung - siehe die Begruendung bei
    // insertNodeSomewhere. Zum Hineinsetzen zieht man auf den Block.
    Path at = g_app->selectedPath;
    // In einen angeklickten Block: ans Ende, Zeile fuer Zeile (siehe
    // zielBlock). Sonst dahinter, als EIN Rueckgaengig-Schritt fuer das ganze
    // Makro, nicht einer je Knoten (shanks rc533-Protokoll: Stapel 7 -> 11
    // beim Einsetzen von `standOnly`).
    const Path block = zielBlock();
    if (!block.empty() && !g_app->doc.istMakro(block)) {
        std::size_t k = 0;
        for (const Node& n : nodes) {
            if (!g_app->doc.insertInto(block, k, n)) { break; }
            ++k;
        }
        if (k == 0) {
            return;
        }
        g_app->expanded.setOpen(kennungFuer(block), true);
        at = block;
        at.push_back(k - 1);
    } else if (!g_app->doc.insertAfterAll(at, nodes, &at)) {
        return;
    }
    g_app->auswahlDurchEinfuegen = at;
    g_app->selectedPath = at;
    g_app->selection.assign(1, at);
    g_app->scrollToSelected = true;
    rebuildTree();
}






// --- Symbole -----------------------------------------------------------
//
// Der Symbolstreifen liegt als EIGENE Textur, nicht als Rechtecke im
// Schriftatlas.
//
// Der erste Anlauf ging ueber ImFontAtlas::AddCustomRect. Das ist in ImGui
// 1.92 als "[ALPHA] Custom Rectangles/Glyphs API" gekennzeichnet, und zwei
// Dinge sind daran toedlich:
//   * GetCustomRect() dereferenziert TexData ungeprueft
//     (IM_ASSERT(TexData->Width > 0 ...) steht VOR jeder Nullpruefung),
//   * io.Fonts->Clear() beim Neubau der Schriften macht alle Kennungen
//     ungueltig, ohne dass man es merkt.
// Ergebnis war ein Absturz im ersten Bild:
//     0xC0000005, lesend an 0x0000000000000008
// Eine eigene Textur haengt an nichts davon.
struct IconTexture {
    void* id = nullptr;
    bool tried = false;
};

IconTexture g_iconTex;

// Beim Wechsel der Grafikschnittstelle zeigt die Kennung auf eine bereits
// zerstoerte Textur. Faellt Direct3D durch und OpenGL uebernimmt, muss sie
// deshalb neu angelegt werden.
IconTexture g_logoTex;

void* logoTextureId() { return g_logoTex.id; }

// Stufe 8c: das Logo von JK Studio (Ressource JKLOGO, PNG) - fuer den Ueber-Dialog.
IconTexture g_jkLogoTex;
int g_jkLogoB = 0;
int g_jkLogoH = 0;
void* jkLogoTextureId(int& b, int& h) {
    if (!g_jkLogoTex.tried) {
        g_jkLogoTex.tried = true;
        const std::string png = platform::ressource(L"JKLOGO");
        if (!png.empty()) {
            const image::Image im = image::decodePng(reinterpret_cast<const unsigned char*>(png.data()), png.size());
            if (im.width > 0 && im.height > 0 && im.rgba.size() == static_cast<std::size_t>(im.width) * static_cast<std::size_t>(im.height) * 4U) {
                g_jkLogoTex.id = render::createTexture(im.rgba.data(), im.width, im.height);
                g_jkLogoB = im.width;
                g_jkLogoH = im.height;
            }
        }
    }
    b = g_jkLogoB;
    h = g_jkLogoH;
    return g_jkLogoTex.id;
}

void resetIcons() {
    g_iconTex.id = nullptr;
    g_iconTex.tried = false;
    g_logoTex.id = nullptr;
    g_logoTex.tried = false;
    g_jkLogoTex.id = nullptr;
    g_jkLogoTex.tried = false;
}

void ensureIcons() {
    if (g_iconTex.tried) {
        return;
    }
    g_iconTex.tried = true;
    // Ein waagerechter Streifen: 40 Symbole zu 16x16 nebeneinander.
    static std::vector<unsigned char> strip;
    strip.assign(static_cast<std::size_t>(icons::kCount) * icons::kSize *
                     icons::kSize * 4U, 0U);
    const int stripW = icons::kCount * icons::kSize;
    for (int i = 0; i < icons::kCount; ++i) {
        for (int y = 0; y < icons::kSize; ++y) {
            for (int x = 0; x < icons::kSize; ++x) {
                const std::size_t src =
                    ((static_cast<std::size_t>(i) * icons::kSize + static_cast<std::size_t>(y)) *
                         icons::kSize + static_cast<std::size_t>(x)) * 4U;
                const std::size_t dst =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(stripW) +
                     static_cast<std::size_t>(i * icons::kSize + x)) * 4U;
                for (int c = 0; c < 4; ++c) {
                    strip[dst + static_cast<std::size_t>(c)] =
                        icons::kPixels[src + static_cast<std::size_t>(c)];
                }
            }
        }
    }
    g_iconTex.id = render::createTexture(strip.data(), stripW, icons::kSize);

    if (!g_logoTex.tried) {
        g_logoTex.tried = true;
        g_logoTex.id = render::createTexture(icons::kLogoPixels, icons::kLogoWidth,
                                             icons::kLogoHeight);
    }
}

// Auswahl setzen, mit Strg und Umschalt wie ueberall sonst.
//
//   ohne Taste  : nur dieser Knoten
//   Strg        : diesen dazu oder weg
//   Umschalt    : alles zwischen dem zuletzt gewaehlten und diesem
void moveSelectionByKey(int delta);
void handleTreeKeys();

// Ein Symbol zeichnen. Die Groesse folgt der Schrift, damit es bei jeder
// Skalierung zur Zeile passt.
void drawIcon(const char* name, bool /*selected*/) {
    const float size = ImGui::GetFontSize();
    // Erst die Vektorzeichnung (gui/vektoricons.cpp): scharf in jeder Groesse.
    {
        const ImVec2 at = ImGui::GetCursorScreenPos();
        if (vektorIcon(ImGui::GetWindowDrawList(), name, at, size,
                       g_app->settings.alternativeIcons)) {
            ImGui::Dummy(ImVec2{size, size});
            return;
        }
    }
    const int base = icons::indexOf(name);
    if (g_iconTex.id == nullptr || base < 0) {
        ImGui::Dummy(ImVec2{size, size});
        return;
    }
    // Jedes Symbol liegt zweimal im Streifen, hell und dunkel. Das ist ein
    // ZWEITER FARBSATZ, keine Auswahlfassung - belegt durch den Schalter
    // "Use Alternative Coloured Icons" (id 1052) in Dialog 131 des
    // Originals. Meine erste Deutung als Normal/Ausgewaehlt war falsch.
    const int index = (g_app->settings.alternativeIcons && base + 1 < icons::kCount)
                          ? base + 1
                          : base;
    const float step = 1.0F / static_cast<float>(icons::kCount);
    const ImVec2 uv0{static_cast<float>(index) * step, 0.0F};
    const ImVec2 uv1{static_cast<float>(index + 1) * step, 1.0F};
    ImGui::Image(reinterpret_cast<ImTextureID>(g_iconTex.id), ImVec2{size, size}, uv0, uv1);
}

// --- Anordnung ---------------------------------------------------------
//
// Die erste Fassung setzte jedes Bedienelement starr auf seine Koordinate aus
// Dialog 102 - 12 dlu hohe Knoepfe, 191 dlu breite Liste. Das ging schief:
// Ravens Mass gilt fuer MS Sans Serif 8pt, unsere Schrift ist deutlich
// groesser, und die Beschriftungen ueberlappten. Ausserdem wuchs beim
// Vergroessern des Fensters nur das Fenster, nicht der Inhalt.
//
// Jetzt gilt: die VERHAELTNISSE kommen aus der Ressource, die absoluten
// Groessen aus der Schrift.
//   Events    191 / 663 = 28,8 %
//   Flow      372 / 663 = 56,1 %
//   Knoepfe    71 / 663 = 10,7 %, mindestens so breit wie der laengste Text
// Die Knopfhoehe ist ImGui::GetFrameHeight(), nicht 12 dlu.

struct Layout {
    float total = 0.0F;   // nutzbare Breite insgesamt
    float minButtonsW = 0.0F;
    float rest = 0.0F;    // Breite von Events + Flow zusammen
    float eventsW;
    float flowW;
    float buttonsW;
    float topH;        // Events / Flow / Knoepfe, samt Kopfzeile
    // Die Hoehe der KOPFZEILE - fuer alle vier Spalten dieselbe.
    //
    // Die Spalten haben unterschiedliche Koepfe: "Script Flow" und "Actions"
    // sind blosser Text, Karte und Modell haben eine Knopfleiste, die
    // Modusleiste hat gar keinen. Eine Knopfzeile ist HOEHER als eine
    // Textzeile - deshalb begannen die Rahmen auf verschiedenen Hoehen, und
    // kein noch so genaues Abziehen unten konnte das ausgleichen.
    //
    // Jetzt bekommt jede Spalte denselben Platz fuer ihren Kopf, egal was
    // darin steht. Wer weniger braucht, laesst den Rest frei.
    float headerH;
    // Die Hoehe des RAHMENS, also ohne die Ueberschriftzeile.
    //
    // EIN Wert fuer alle vier Spalten - Modusleiste, Ansicht, Skriptbaum und
    // Knopfspalte. Vorher rechnete jede ihre eigene aus, immer als
    // "topH minus eine Textzeile", und die vier Rechnungen ergaben nicht
    // dasselbe. Sichtbar wurde es als Rahmen, die unten nicht auf einer
    // Linie enden.
    //
    // Wer eine Spalte hinzufuegt, nimmt diesen Wert und rechnet nicht neu.
    // NEGATIV: "alles Uebrige minus diesem Betrag" - siehe die Erklaerung
    // bei der Berechnung. Wer sie als Hoehe weiterreicht, muss sie so
    // weitergeben, wie sie ist.
    float frameH;
    float fussH;       // Bedienleiste + Statuszeile + Abstaende
    float toolH;       // Bedienleiste unter allen drei Spalten
    float statusH;
    // Das Fenster ist zu klein fuer alles, was hineinsoll.
    //
    // Gemeldet vom Programm selbst: "Fenster 24x-374 - FEHLER: das Band ist
    // hoeher als das Fenster." Eine NEGATIVE Hoehe.
    //
    // frameH entsteht als Rest: verfuegbare Hoehe minus Statuszeile, minus
    // Bedienleiste, minus Reiterleiste, minus Kopfzeile. Schrumpft das
    // Fenster weit genug - beim Minimieren, oder waehrend Windows die
    // Groesse aendert -, wird der Rest negativ, und jede Spalte bekommt
    // eine negative Hoehe.
    //
    // frameH und topH sind deshalb jetzt bei null abgefangen. Damit die
    // Meldung nicht mit verschwindet, wird der Fall HIER festgehalten -
    // sonst haette ich den Fehler nur unsichtbar gemacht statt behoben.
    bool tooSmall = false;
};

// Ein waagerechter Trenner. Dieselbe Bauform, andere Achse.
//
// Das Vorzeichen ist umgekehrt: die Statusliste waechst, wenn man nach OBEN
// zieht. Deshalb ist size1 der Bereich DARUEBER und size2 die Liste.
bool splitterH(const char* id, float width, float* fraction, float total,
               float minFraction, float maxFraction) {
    if (total <= 1.0F) {
        return false;
    }
    const float h = ImGui::GetStyle().ItemSpacing.y * 1.5F;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID wid = window->GetID(id);

    // size2 ist die Statusliste, size1 alles darueber.
    float size2 = *fraction * total;
    float size1 = total - size2;

    const float useW = (width > 0.0F) ? width
                                      : ImGui::GetContentRegionAvail().x;
    ImRect bb;
    bb.Min = window->DC.CursorPos;
    bb.Max = ImVec2{bb.Min.x + std::max(useW, 1.0F), bb.Min.y + h};

    const bool held = ImGui::SplitterBehavior(bb, wid, ImGuiAxis_Y, &size1, &size2,
                                              (1.0F - maxFraction) * total,
                                              minFraction * total,
                                              2.0F, 0.10F);
    ImGui::Dummy(ImVec2{std::max(useW, 1.0F), h});

    if (held) {
        *fraction = std::clamp(size2 / total, minFraction, maxFraction);
        return true;
    }
    return false;
}

// `splitter()` ist seit rc473 weg.
//
// Die Trennlinien zwischen den Spalten wurden von Hand gezeichnet, mit
// eigenem Ziehen, eigenen Grenzen und zwei gespeicherten
// Aufteilungsbruechen. Eine ImGui-Tabelle bringt das alles mit -
// `ImGuiTableFlags_Resizable` und `BordersInnerV` -, und sie merkt sich
// die Breiten selbst.
//
// Rund siebzig Zeilen weniger, und zwei Zahlen weniger, die auseinander
// laufen koennen.


Layout computeLayout() {
    const ImGuiStyle& st = ImGui::GetStyle();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float pad = st.ItemSpacing.x;

    // Die Knopfspalte muss den laengsten Text tragen. In den Uebersetzungen
    // sind die Woerter unterschiedlich lang ("Wiederherstellen" gegen
    // "Restore"), deshalb gemessen statt geraten.
    float widest = 0.0F;
    for (const Str id : {Str::ActDelete, Str::ActClone, Str::ActPaste, Str::ActRem,
                         Str::ActFind, Str::FileNew, Str::FileAppend, Str::FileSave,
                         Str::FileSaveAs, Str::FileExport, Str::FileBackup,
                         Str::FileRestore, Str::AppExit, Str::Compile}) {
        widest = std::max(widest, ImGui::CalcTextSize(tr(id)).x);
    }
    // --- Die UEBERSCHRIFTEN zaehlen mit --------------------------------
    //
    // "Treeview Options" und "Application" standen nicht in der Liste. Sie
    // sind aber laenger als jeder Knopf darunter, und auf shanks Bildern
    // waren sie abgeschnitten: "Treeview Option", "Fold Macro:".
    //
    // Der Fehler ist aelter als die Tabelle aus rc473 - auf den Bildern zu
    // rc472 stand dasselbe. Der kopflose Pruefstand hat gezeigt, dass die
    // Tabellenzelle KEINE Breite kostet (Spalte 156 -> innen 140, genau wie
    // ein Kindfenster von 156). Damit war klar, dass es hier liegt.
    for (const Str id : {Str::GroupFile, Str::GroupTreeview,
                         Str::GroupApplication}) {
        widest = std::max(widest, ImGui::CalcTextSize(tr(id)).x);
    }
    // --- Ankreuzfelder tragen ein Kaestchen vor sich her ----------------
    //
    // `CalcTextSize` misst nur den Text. Ein Ankreuzfeld ist Kaestchen plus
    // Abstand plus Text - das Kaestchen ist so hoch wie eine Zeile, also
    // `GetFrameHeight()` breit.
    const float kaestchen =
        ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
    for (const Str id : {Str::TreeShowTypes, Str::TreeGFloats,
                         Str::TreeFoldMacros}) {
        widest = std::max(widest, ImGui::CalcTextSize(tr(id)).x + kaestchen);
    }
    Layout l{};
    // Der gemessene Text ist die UNTERGRENZE, die eingestellte Breite darf
    // darueber liegen. Sonst kann man die Spalte nicht breiter ziehen, und
    // im Deutschen werden "Ausschneiden" und "Einstellungen" abgeschnitten.
    l.minButtonsW = widest + st.FramePadding.x * 2.0F + st.ScrollbarSize + pad;
    // Bei der KARTE entfaellt die Knopfspalte: "Actions" gehoert zum Skript,
    // nicht zur Karte. Ihr Platz geht an Karte und Skriptbaum.
    l.buttonsW = (g_app->leftMode == 1)
                     ? 0.0F
                     : std::max(l.minButtonsW,
                                avail.x * g_app->settings.splitButtons);
    // Die Ebenenspalte wird NICHT hier abgezogen: sie liegt innerhalb der
    // linken Spalte, und drawEventsList zieht sie dort von seiner Breite ab.
    // Zweimal abziehen liesse den Skriptbaum wachsen, bis er nicht mehr
    // passt.
    const float gaps = (g_app->leftMode == 1) ? pad * 2.0F : pad * 3.0F;
    const float rest = std::max(avail.x - l.buttonsW - gaps, 200.0F);
    l.total = avail.x;
    l.rest = rest;
    // Im Kartenmodus braucht die linke Spalte deutlich mehr Platz: eine
    // 3D-Ansicht in einem Streifen von 200 Pixeln ist unbenutzbar. Deshalb
    // ein eigener Anteil je Modus.
    l.eventsW = rest * (g_app->leftMode != 0 ? g_app->settings.splitMap
                                             : g_app->settings.splitEvents);
    // Im Kartenmodus muss die Ebenenspalte noch hineinpassen, sonst bleibt
    // fuer die Ansicht nichts uebrig.
    if (g_app->leftMode == 1) {
        l.eventsW = std::max(l.eventsW, ImGui::GetFontSize() * 26.0F);
        // Untergrenze der Skriptspalte: gerade so viel, dass Kaestchen und
        // Symbol hineinpassen.
        //
        // Vorher waren es feste 200 Bildpunkte. Gewuenscht war aber
        // ausdruecklich, weiter hineinziehen zu koennen, "das man nur die
        // Symbole hat wie bei Blender" - und dafuer muss die Grenze tiefer.
        // Was dann noch dasteht, sind Kaestchen, Symbol und ein Rest fuer
        // die Einrueckung.
        l.eventsW = std::min(l.eventsW, rest - ImGui::GetFontSize() * 6.0F);
    }
    l.flowW = rest - l.eventsW;

    // Die Statuszeile ist EINE Zeile.
    //
    // Sie war einmal ein Feld mit einstellbarer Hoehe, Rahmen und
    // Ueberschrift - und nahm damit dauerhaft Platz weg, auch wenn nichts
    // zu melden war. Jetzt zaehlt sie nur; wer nachlesen will, klickt den
    // Zaehler an und bekommt ein Fenster. Dasselbe Muster wie in efxed.
    //
    // Der Anteil aus den Einstellungen (splitStatus) wird nicht mehr
    // gebraucht; er bleibt in der Datei stehen, damit eine aeltere Fassung
    // sie noch lesen kann.
    // GENAU die Hoehe der Statuszeile, nicht mehr.
    //
    // Hier stand zusaetzlich `+ FramePadding.y * 2`. Das war einmal
    // richtig, als die Zeile in einem Rahmen sass; sie ist heute ein
    // blosser Text. Die zwei Randbreiten wurden also abgezogen, ohne dass
    // jemand sie braucht - und blieben als Luecke stehen.
    //
    // Seit rc469 springt die Statuszeile ans Fensterende, und damit wanderte
    // die Luecke von UNTER sie nach UEBER sie: die drei Spalten enden zu
    // frueh. shank: "die 3 fenster sollten dann oberhalb enden und keinen
    // platz lassen".
    // Dieselbe Hoehe, die drawStatus fuer sich nimmt (Knoepfe darin).
    l.statusH = std::max(ImGui::GetTextLineHeightWithSpacing(), ImGui::GetFrameHeight());
    // Die Bedienleiste unter ALLEN drei Spalten - nicht in der Modellspalte.
    //
    // Vorher stand sie innerhalb der Modellansicht und musste sich deren
    // Breite teilen. Dadurch passte sie bei schmaler Spalte nicht hinein.
    // Ueber die volle Breite ist reichlich Platz, und das Bild verliert nur
    // die Hoehe, die wirklich gebraucht wird.
    l.toolH = 0.0F;
    const float rowH = ImGui::GetFrameHeightWithSpacing();
    // IMMER Platz lassen, auch ohne Modell.
    //
    // Vorher entstand das Band erst mit einem Modell - und dann sprang die
    // ganze Anordnung, weil die Ansicht darueber ploetzlich kuerzer wurde.
    // Wer die Bedienung schon sieht, weiss auch, was ihn erwartet.
    if (g_app->leftMode == 2) {
        // Modell: zwei Zeilen - Bedienung und Zeitleiste. Beide sind
        // Elementzeilen, also genau zwei Mal rowH, plus die Aussenraender
        // des Bandes.
        l.toolH = rowH * 2.0F + st.ItemSpacing.y * 2.0F;
    } else if (g_app->leftMode == 1) {
        // IMMER Platz lassen, auch ohne Karte - sonst springt die Anordnung
        // beim Laden.
        //
        // Die Regler und Kaestchen sind seit rc173 in der Seitenleiste
        // rechts. Hier bleiben: eine Zeile Bedienung, eine Zeile Abspielen,
        // die angeklickte Entity - und die ZEITLEISTE, deren Hoehe der
        // Anwender einstellt.
        //
        // Hochziehbar wie in Blender: dort zieht man die Kante zwischen
        // Ansicht und Zeitleiste, und die Spuren werden sichtbar. Der
        // Anteil steht in timelineFrac; die Grenzen sorgen dafuer, dass
        // weder die Ansicht noch die Leiste verschwindet.
        // IMMER derselbe Platz, auch ohne Karte.
        //
        // In rc175 bekam der leere Fall nur eine Zeile - damit unten kein
        // leeres Feld klafft. Seit rc177 steht dort aber ein Platzhalter in
        // der Groesse des Bildes, und dann ist die Sonderbehandlung
        // schaedlich: die ganze Anordnung sprang, sobald eine Karte geladen
        // wurde. Gewuenscht war ausdruecklich, dass der leere Fall aussieht
        // wie der volle.
        const float zeit =
            std::clamp(avail.y * g_app->timelineFrac, rowH * 3.0F,
                       std::max(rowH * 3.0F, avail.y * 0.6F));
        l.toolH = rowH * 3.0F + st.ItemSpacing.y * 2.0F + zeit;
        g_app->tlHoeheIst = zeit;
        g_app->tlPlatz = avail.y;
    }

    // Die Statuszeile sitzt GANZ unten, ohne Abstand darunter.
    //
    // Vorher wurde hier zusaetzlich eine Textzeile abgezogen - der Rest
    // einer Ueberschrift, die es nicht mehr gibt. Dadurch blieb unter der
    // Statuszeile ein leerer Streifen stehen, und sie sass eben NICHT ganz
    // unten. In efxed ist sie die letzte Zeile des Fensters; genau so soll
    // es hier auch sein.
    // Die Reiterleiste kostet eine Zeile - sonst laeuft alles unten hinaus.
    // Im Karten-Editor keine Reiterleiste (Stufe 3 Punkt 0b).
    const float tabH = (g_app->tabs.empty() || g_app->leftMode == 3 || g_app->leftMode == 4)
                           ? 0.0F
                           : ImGui::GetFrameHeightWithSpacing();
    // --- NEGATIVE Hoehe statt Abzugsrechnung ----------------------------
    //
    // shank, nach zwei Anlaeufen: "eventuell muss das Fenster sauberer
    // aufgebaut werden?" - ja, und ImGui hat dafuer eine eigene Redewendung.
    //
    // Eine NEGATIVE Hoehe in `BeginChild` heisst "alles Uebrige, minus
    // diesem Betrag". Genau so baut ImGui seine eigene Konsole
    // (imgui_demo.cpp:9174):
    //
    //     const float footer = style.SeparatorSize + style.ItemSpacing.y
    //                          + ImGui::GetFrameHeightWithSpacing();
    //     ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer), ...)
    //
    // Der Vorteil ist nicht die Kuerze, sondern dass es NICHT RECHNEN muss:
    // ImGui kennt die verbleibende Hoehe genau, ich muss sie nicht aus
    // avail.y, statusH, toolH, tabH und zwei Abstaenden zusammensetzen. Jede
    // dieser Zahlen war eine Gelegenheit, sich zu vertun - und zwei davon
    // waren falsch (rc470: ein Rahmen, den es nicht mehr gibt, und ein
    // waagerechter Abstand senkrecht abgezogen).
    //
    // `frameH` traegt jetzt einen negativen Wert. Alle vier Spalten - die
    // Modusleiste, Events, Script Flow und die Knoepfe - bekommen denselben,
    // und damit enden sie zwangslaeufig auf DERSELBEN Linie. Nicht "sollten"
    // - koennen gar nicht anders.
    // EIN Abstand, nicht zwei.
    //
    // Am kopflosen Pruefstand (tools/layoutcheck.cpp) gemessen: mit zwei
    // Abstaenden endeten die Spalten bei 1426, die Statuszeile begann bei
    // 1455 - neunundzwanzig Bildpunkte ungenutzt. Genau `fussH`.
    // Ueber der Zeitleiste sitzt die Ziehkante (##tlkante, max(4,
    // ItemSpacing.y) hoch) samt einem Zeilenabstand dahinter. Sie stand in
    // keiner Rechnung: in der Kartenansicht war der Inhalt dadurch 14 Punkte
    // hoeher als das Fenster (Selbsttest), die Statuszeile unten abgeschnitten
    // und das Hauptfenster rollbar.
    const float kanteH = (g_app->leftMode == 1)
                             ? std::max(4.0F, st.ItemSpacing.y) + st.ItemSpacing.y
                             : 0.0F;
    l.fussH = l.toolH + kanteH + l.statusH + st.ItemSpacing.y;
    l.topH = avail.y - l.fussH - tabH;
    // So hoch wie eine Knopfzeile - das ist der hoechste Kopf, den es gibt.
    // --- Die Kopfzeile ist nur so hoch, wie sie sein MUSS --------------
    //
    // Sie stand fest auf `GetFrameHeightWithSpacing()` - der Hoehe eines
    // Knopfes. Bei Map und Model ist das richtig: dort stehen dort Knoepfe
    // ("Load mission...", "Open .glm...").
    //
    // Im Modus Events steht nur Text. Die Differenz zwischen Knopfhoehe und
    // Textzeile blieb als Luecke zwischen der Ueberschrift "Events" und dem
    // Kasten darunter stehen - shank: "warum ist da so viel Abstand
    // zwischen Events-Text und dem Fenster unten?"
    //
    // Also die Hoehe danach richten, was tatsaechlich darin steht.
    // Der Karten-Editor hat keine Kopfzeile: er beginnt direkt unter der
    // Menueleiste (Stufe 3 Punkt 0b).
    l.headerH = (g_app->leftMode == 3 || g_app->leftMode == 4) ? 0.0F
                : (g_app->leftMode == 0) ? ImGui::GetTextLineHeightWithSpacing()
                                         : ImGui::GetFrameHeightWithSpacing();
    // NEGATIV, und ohne die Kopfzeile.
    //
    // Am kopflosen Pruefstand gemessen (tools/layoutcheck.cpp): die
    // Kopfzeilen stehen UEBER der Tabelle, nicht in ihr. Wer sie hier
    // abzieht, zieht sie zweimal ab.
    l.frameH = -l.fussH;
    // Nichts Negatives weitergeben - siehe tooSmall.
    // `frameH` ist jetzt IMMER negativ - die Pruefung auf zu klein muss
    // deshalb an `topH` haengen, nicht an ihr.
    l.tooSmall = (l.topH < l.headerH);
    l.topH = std::max(l.topH, 0.0F);
    return l;
}

// Ueberschrift plus umrandeter Bereich - ImGui kennt keine Gruppenrahmen.
// Ein beschrifteter Rahmen. size.y ist die Hoehe des RAHMENS, nicht die von
// Ueberschrift plus Rahmen.
//
// Vorher zog die Funktion selbst eine Textzeile ab. Das taten die anderen
// Spalten auch, jede auf ihre Art - und die Ergebnisse wichen voneinander ab.
// Jetzt kommt der Wert von aussen, aus Layout::frameH, und ist fuer alle
// gleich.
// Ein beschrifteter Rahmen. size.y ist die Hoehe des RAHMENS.
//
// Die Beschriftung bekommt die volle Kopfzeilenhoehe, auch wenn der Text
// weniger braucht - sonst beginnt dieser Rahmen hoeher als der daneben, der
// eine Knopfleiste im Kopf hat.

// Die Rollposition gehoert zum SKRIPT, nicht zum Kasten.
//
// Das fokussierte Feld wird als Hauptbaum gezeichnet, die anderen als
// Vergleichsbaeume - verschiedene Fenster mit je eigener Rollposition. Beim
// Fokuswechsel wandert ein Skript vom einen ins andere und landet dort, wo
// DIESES Fenster zuletzt stand. Gemeldet als "das erste Fenster springt
// ungewollt umher".
//
// Also je Reiter merken und beim Wechsel zurueckstellen. Aufzurufen INNEN,
// gleich nachdem der rollende Kasten begonnen hat.
void rollpositionHolen(const std::string& kasten, int tab) {
    // Verglichen und gemerkt wird die feste Kennung des Reiters, nicht seine
    // Nummer: nach dem Schliessen eines Reiters traegt der Nachruecker
    // dieselbe Nummer - und bekam bisher weder seine eigene Position noch
    // ueberhaupt ein Zuruecksetzen, weil "Nummer gleich" als "nichts
    // gewechselt" galt.
    const int uid = tabKennung(tab);
    int& letzter = g_app->lastTabInBox[kasten];
    if (letzter != uid) {
        letzter = uid;
        const auto es = g_app->tabScroll.find(uid);
        const float wohin = (es == g_app->tabScroll.end()) ? 0.0F : es->second;
        // Auch das war blind, als es sprang.
        char zs[200];
        std::snprintf(zs, sizeof(zs),
                      "Rollposition: Kasten %s zeigt jetzt Reiter %d, "
                      "zurueck auf %.0f",
                      kasten.c_str(), tab, (double)wohin);
        diag::detail(zs);
        ImGui::SetScrollY(wohin);
    }
}

void rollpositionMerken(int tab) {
    g_app->tabScroll[tabKennung(tab)] = ImGui::GetScrollY();
}


bool beginGroupBox(const char* label, const char* id, ImVec2 size,
                   float headerH, const std::function<void()>& kopf = {},
                   const std::function<void()>& titelRechts = {}) {
    ImGui::BeginGroup();
    const float top = ImGui::GetCursorPosY();
    ImGui::TextUnformatted(label);
    // Neben der Ueberschrift (Script Flow: der Umschalter der Ansichten).
    if (titelRechts) {
        ImGui::SameLine();
        titelRechts();
    }
    if (headerH > 0.0F) {
        ImGui::SetCursorPosY(top + headerH);
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    // --- Die Kopfzeile VOR dem Rollbereich --------------------------------
    //
    // shank, 03.10.: "wenn ich compare offen habe und runter scrolle sehe
    // ich nicht welches script ich gerade arbeite". Das Klappfeld mit dem
    // Skriptnamen stand IM Kindfenster und rollte mit dem Baum weg. Jetzt
    // steht es darueber, und der Rahmen wird um seine Hoehe kuerzer - das
    // Kindfenster (Kennung, Rollstand) bleibt dasselbe.
    if (kopf) {
        const float vorher = ImGui::GetCursorPosY();
        kopf();
        size.y = std::max(1.0F, size.y - (ImGui::GetCursorPosY() - vorher));
    }
    return ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
}

void endGroupBox() {
    ImGui::EndChild();
    ImGui::EndGroup();
}




// Eine .ent-Datei laden.
//
// Movie Duels arbeitet so: die .bsp bleibt unveraendert, die Entities kommen
// aus einer eigenen Datei daneben. md_twj_jedi.ent hat 132 Eintraege, und
// die sind es, die das Skript anspricht - nicht die der Originalkarte.









// Modusleiste am linken Rand, wie bei g2c.
//
// Zwei Knoepfe statt Auswahlkreisen: die Umschaltung wechselt die GANZE
// linke Spalte, nicht nur eine Einstellung darin. Am Rand ist das sichtbar
// genug, um nicht versehentlich im falschen Modus zu arbeiten.
//
// Die Breite kommt aus dem TEXT, nicht aus einer festen Zahl. Bei g2c war
// "XSI -> GLA" auf Englisch abgeschnitten, waehrend dieselbe Zahl fuer die
// kuerzeren chinesischen Beschriftungen zu breit gewesen waere.
float drawModeBar(float height, float headerH) {
    const struct {
        int mode;
        Str label;
        Str hint;
    } modes[] = {
        {0, Str::ViewEvents, Str::ModeEventsHint},
        {1, Str::ViewMap, Str::ModeMapHint},
        {2, Str::ViewModel, Str::ModeModelHint},
        // Stufe 1b: der Karten-Editor nach NetRadiant-Custom.
        {3, Str::ViewMapEditor, Str::ModeMapEditorHint},
        // Menue-Editor M1: .menu als Text und Vorschau.
        {4, Str::ViewMenuEditor, Str::ModeMenuEditorHint},
    };

    float textW = 0.0F;
    for (const auto& m : modes) {
        textW = std::max(textW, ImGui::CalcTextSize(tr(m.label)).x);
    }
    const ImGuiStyle& st = ImGui::GetStyle();
    const float barW = textW + st.FramePadding.x * 2.0F +
                       st.WindowPadding.x * 2.0F + ImGui::GetFontSize() * 0.5F;

    // Eine Zeile Platz fuer die Ueberschrift lassen, obwohl die Leiste keine
    // hat.
    //
    // Alle drei Spalten daneben sind ueber beginGroupBox gebaut: erst eine
    // Textzeile mit dem Namen, dann der Rahmen. Ohne diese Zeile beginnt die
    // Modusleiste eine Textzeilenhoehe weiter oben - sichtbar, weil die
    // Rahmen dann nicht auf einer Linie liegen.
    // Kein Kopf, aber derselbe Platz dafuer.
    ImGui::BeginGroup();
    const float top = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(top + headerH);
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
    ImGui::BeginChild("modebar", ImVec2{barW, height},
                      ImGuiChildFlags_Borders);
    for (const auto& m : modes) {
        const bool active = (g_app->leftMode == m.mode);
        // Alle drei immer erreichbar. Vorher war "Karte" ausgegraut, solange
        // nichts geladen war - dann kam man nie an die Knoepfe zum Laden.
        const bool possible = true;
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.20F, 0.42F, 0.68F, 1.0F});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImVec4{0.24F, 0.48F, 0.76F, 1.0F});
        }
        ImGui::BeginDisabled(!possible);
        if (ImGui::Button(tr(m.label), ImVec2{-1, ImGui::GetFontSize() * 2.6F})) {
            g_app->leftMode = m.mode;
        }
        ImGui::EndDisabled();
        if (active) {
            ImGui::PopStyleColor(2);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(m.hint));
        }
        ImGui::Spacing();
    }
    ImGui::EndChild();
    ImGui::EndGroup();
    return barW;
}

// --- Stufe 1b: der Arbeitsbereich Karten-Editor ---------------------------
//
// Zwei Spalten wie die anderen Modi (eigene Tabellen-ID, rc495): die
// Modusleiste, so breit wie ihre Beschriftungen, und daneben der ganze
// Editor - Menueleiste, Werkzeugleisten und Ansichten wie NetRadiant-Custom
// (gui/app_karte.cpp). Keine Ereignisliste, kein Script Flow, keine
// Actions: die gehoeren zum Skript.
void drawKartenEditorBereich(const Layout& l) {
    // Stufe 3 Punkt 0b: ohne Reiter- und Kopfzeile direkt unter der
    // Menueleiste - auch ohne den Fensterrand oben dazwischen.
    {
        const ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImGui::SetCursorScreenPos(ImVec2{ImGui::GetCursorScreenPos().x, w->Pos.y + w->TitleBarHeight + w->MenuBarHeight});
    }
    if (!ImGui::BeginTable("##spaltenKarteneditor", 2,
                           ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX,
                           ImVec2{0.0F, 0.0F})) {
        return;
    }
    ImGui::TableSetupColumn("mode", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                            std::max(g_app->modeBarW, ImGui::GetFontSize() * 4.0F));
    ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch, 1.0F);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    g_app->modeBarW = drawModeBar(l.frameH, l.headerH);   // headerH = 0 im Karten-Editor
    ImGui::TableNextColumn();
    karteneditor::zeichne(l.frameH);
    ImGui::EndTable();
}

// --- Menue-Editor M1: der Arbeitsbereich Menu Editor ----------------------
//
// Wie der Karten-Editor: links die Modusleiste, rechts der ganze Editor
// (gui/app_menue.cpp), ohne Skript-Reiter und Kopfzeile.
void drawMenueEditorBereich(const Layout& l) {
    {
        const ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImGui::SetCursorScreenPos(ImVec2{ImGui::GetCursorScreenPos().x, w->Pos.y + w->TitleBarHeight + w->MenuBarHeight});
    }
    if (!ImGui::BeginTable("##spaltenMenueeditor", 2,
                           ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX,
                           ImVec2{0.0F, 0.0F})) {
        return;
    }
    ImGui::TableSetupColumn("mode", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                            std::max(g_app->modeBarW, ImGui::GetFontSize() * 4.0F));
    ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch, 1.0F);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    g_app->modeBarW = drawModeBar(l.frameH, l.headerH);
    ImGui::TableNextColumn();
    ImGui::BeginChild("##menueeditor", ImVec2{0.0F, l.frameH});
    menueeditor::zeichne(l.frameH);
    ImGui::EndChild();
    ImGui::EndTable();
}

// --- Die LINKE Spalte, je nach Modus --------------------------------
//
// Der Name ist alt und irrefuehrend: die Funktion zeichnet je nach Modus
// etwas voellig anderes.
//
//   Events   Modusleiste, Ereignisliste
//   Map      Modusleiste, Ebenenliste, Kartenansicht, Kamerazeile,
//            Zeitleiste
//   Model    Modusleiste, Modellansicht, Abspielzeile
//
// shank nach rc478: "ich glaube wir muessen alle 3 Modi als separate UIs
// betrachten." Gemessen gibt ihm das recht - `leftMode` kommt in dieser
// Datei vierundzwanzig Mal vor, sechs davon hier drin und sieben in
// `computeLayout`.
//
// DAS ist der Grund, warum jede Korrektur an einem Modus die anderen beiden
// trifft: die Tabellengewichte zerlegten die Karte (rc474), die
// Kopfzeilenhoehe galt fuer alle drei (rc478), die Platzhalter im Script
// Flow ebenso (rc477).
//
// Der Umbau steht in LAYOUT.md, Abschnitt 2.3: drei Erzeuger fuer `Layout`
// und drei Zeichenfunktionen statt Abfragen. Er ist NICHT in dieser Runde
// gemacht - er ist gross, und die letzten Runden haben gezeigt, was
// passiert, wenn ich hier blind umbaue.
//
// Was hier steht, ist die Verzweigung an EINER Stelle statt verstreut:
// wer einen Modus gestalten will, sieht sofort, wo er anfaengt.
void drawEventsList(const Layout& l) {
    // --- Die linke Spalte ist selbst eine TABELLE -----------------------
    //
    // shank: "ich glaube Tabelle ist besser."
    //
    // rc500 hat die Luecke behoben, indem eine geschaetzte Subtraktion
    // durch eine Messung ersetzt wurde. Das half an DIESER Stelle; die
    // Bauart blieb - Modusleiste, Ebenen und Inhalt standen weiter per
    // `SameLine` nebeneinander.
    //
    // Als Tabelle gibt es die Rechnung gar nicht. Jede Zelle klemmt ihren
    // Inhalt und faengt ihre Klicks; Luecken und Ueberlappen sind
    // ausgeschlossen statt unwahrscheinlich - dieselbe Lehre wie im
    // Hauptfenster (rc473) und bei den Helferspalten (rc488).
    //
    // Die Ebenenliste bekommt dabei einen ziehbaren Teiler, den es vorher
    // nicht gab.
    //
    // EIGENE ID JE MODUS: eine ID, ein Zustand (rc495).
    // Karte: die Entity-Liste; Modell (rc591): die Animationsliste.
    const bool mitEbenen = (g_app->leftMode == 1 || g_app->leftMode == 2);
    const char* const innenId = (g_app->leftMode == 1)   ? "##linksMap"
                                : (g_app->leftMode == 2) ? "##linksModel"
                                                         : "##linksEvents";
    if (!ImGui::BeginTable(innenId, mitEbenen ? 3 : 2,
                           ImGuiTableFlags_Resizable |
                               ImGuiTableFlags_NoSavedSettings |
                               ImGuiTableFlags_NoPadOuterX,
                           ImVec2{0.0F, 0.0F})) {
        return;
    }
    // Die Modusleiste ist so breit wie ihre Beschriftungen und wird nicht
    // gezogen - wie die Actions-Spalte im Hauptfenster (rc481).
    ImGui::TableSetupColumn("mode",
                            ImGuiTableColumnFlags_WidthFixed |
                                ImGuiTableColumnFlags_NoResize,
                            std::max(g_app->modeBarW,
                                     ImGui::GetFontSize() * 4.0F));
    if (mitEbenen) {
        // NoResize, und das ist kein Rueckschritt.
        //
        // rc501 hat der Ebenenliste einen ziehbaren Teiler gegeben - den
        // hatte niemand verlangt, ich fand ihn nur gut. Auf shanks Bild ist
        // die Spalte dadurch rund 760 breit statt der angeforderten 315:
        // die Liste darin bleibt 315, der Rest ist Luecke IN der Zelle.
        //
        // Der Grund steht in der Recherche zu rc481: eine ZIEHBARE Spalte
        // nimmt `init_width_or_weight` nur beim Anlegen, danach gewinnt
        // ImGuis Selbstanpassung. Eine feste ohne Ziehen nimmt sie in jedem
        // Bild - und genau das braucht diese Liste, deren Inhalt eine feste
        // Breite hat.
        //
        // Vor dem Umbau war sie ebenfalls nicht ziehbar. Es geht also
        // nichts verloren; es kommt nur nichts dazu.
        ImGui::TableSetupColumn("ebenen",
                                ImGuiTableColumnFlags_WidthFixed |
                                    ImGuiTableColumnFlags_NoResize,
                                ImGui::GetFontSize() * kEntitySpalte);
    }
    ImGui::TableSetupColumn("inhalt", ImGuiTableColumnFlags_WidthStretch, 1.0F);
    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    g_app->modeBarW = drawModeBar(l.frameH, l.headerH);
    if (mitEbenen) {
        ImGui::TableNextColumn();
    }

    // Bei der Karte die Ebenenliste - RECHTS von der Modusleiste, links von
    // der Ansicht.
    //
    // Sie ist IMMER da, auch ohne geladene Karte. Vorher entstand sie erst
    // mit den Entities, und dann sprang die ganze Anordnung beim Laden. Wer
    // die Gliederung schon sieht, weiss, was ihn erwartet.
    if (g_app->leftMode == 1) {
        ImGui::BeginGroup();
        const float top = ImGui::GetCursorPosY();
        ImGui::TextUnformatted(tr(Str::EntityLayers));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip("%s", tr(Str::EntityLayersHint));
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(g_app->map.entities.empty());
        if (ImGui::SmallButton(tr(Str::EntityShowAll))) {
            g_app->hiddenClasses.clear();
            g_app->hiddenEntities.clear();
            g_app->mapDirty = true;
        }
        ImGui::EndDisabled();
        // Eine neue Entity unter das Fadenkreuz (gui/app_mission.cpp).
        ImGui::SameLine();
        ImGui::BeginDisabled(g_app->geo.empty());
        if (ImGui::SmallButton(tr(Str::EntNew))) {
            ImGui::OpenPopup("##entneu");
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", tr(Str::EntNewHint));
        }
        if (ImGui::BeginPopup("##entneu")) {
            entityNeuMenue(false);
            ImGui::EndPopup();
        }
        ImGui::SetCursorPosY(top + l.headerH);
        // Ein leeres Element als Anker: ImGui verlangt nach SetCursorPos ein
        // abgeschicktes Element, sonst bricht es ab. Was danach kommt, kann
        // vorzeitig zurueckkehren - der Anker nicht.
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        drawEntityLayers(l.frameH);
        ImGui::EndGroup();
    } else if (g_app->leftMode == 2) {
        ImGui::BeginGroup();
        const float top = ImGui::GetCursorPosY();
        ImGui::TextUnformatted(tr(Str::ModelAnimations));
        if (!g_app->animList.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("%d", static_cast<int>(g_app->animList.size()));
        }
        ImGui::SetCursorPosY(top + l.headerH);
        ImGui::Dummy(ImVec2{0.0F, 0.0F});   // Anker (siehe oben)
        drawAnimListe(l.frameH);
        ImGui::EndGroup();
    }

    // Dritte Zelle: der eigentliche Inhalt.
    ImGui::TableNextColumn();

    // --- Die Restbreite MESSEN, nicht abziehen --------------------------
    //
    // Hier stand:
    //
    //     restW = Zellbreite;
    //     if (Modus == Karte) restW -= 15 Zeichenhoehen + Abstand;
    //
    // Die 15 Zeichenhoehen sind die Breite, die `drawEntityLayers` seinem
    // Kindfenster gibt. Der GRUPPE davor gehoert aber auch die Kopfzeile
    // ("Entities", "Show all"), und ImGui nimmt fuer eine Gruppe das
    // Breiteste ihrer Zeilen. Was die Gruppe wirklich einnimmt, weiss
    // niemand vorher - abgezogen wurde eine geschaetzte Zahl.
    //
    // Genau daraus entstand shanks Luecke: "neben dem Map-Fenster und dem
    // Script Flow ist eine riesige Luecke."
    //
    // Nach `SameLine` steht die wirkliche Restbreite fest, und
    // `GetContentRegionAvail` nennt sie. Dieselbe Regel wie bei der
    // Actions-Spalte (rc474), der Modusleiste (rc473), dem Script Flow
    // (rc477) und den Helferspalten (rc485): messen statt rechnen.
    // Die ZELLE gibt die Breite vor - keine Subtraktion mehr noetig, die
    // Tabelle hat den Platz schon verteilt.
    const float restW = std::max(ImGui::GetContentRegionAvail().x, 80.0F);
    g_app->dbgRestW = restW;
    // Was nach der Modusleiste in der Zelle uebrig ist. Frueher stand hier
    // `l.eventsW`, die selbst gerechnete Spaltenbreite - jetzt fragen wir
    // die Zelle.
    g_app->dbgZelleW = ImGui::GetContentRegionAvail().x;

    // Die Werkzeugleiste steht AUSSERHALB des Rahmens, auf gleicher Hoehe
    // wie die Ueberschrift "Script Flow" daneben. Vorher lag sie darin, und
    // der Inhalt rutschte gegenueber dem Skriptfenster nach unten.
    if (g_app->leftMode == 1 || g_app->leftMode == 2) {
        ImGui::BeginGroup();
        // Die Hoehe der Werkzeugleiste MESSEN, nicht schaetzen: die rechte
        // Spalte zieht ueber beginGroupBox eine Textzeilenhoehe ab, eine
        // Knopfzeile ist aber hoeher. Wer hier eine feste Zahl einsetzt,
        // bekommt zwei Rahmen, die nicht auf gleicher Hoehe enden - genau
        // das war der Fall.
        // Die Werkzeugleiste FUELLT die Kopfzeile - sie ist genau eine
        // Knopfzeile hoch, und danach bemisst sich l.headerH. Kein Abziehen
        // noetig: der Rahmen ist derselbe wie ueberall.
        // Die Werkzeugleiste in einen Kasten von genau der Spaltenbreite.
        //
        // Ohne das waechst sie ueber die Spalte hinaus, sobald ein Knopf
        // dazukommt - beim Laden einer Karte etwa das Feld "Scripts (2)".
        // ImGui vergroessert dann den Inhalt des Hauptfensters, und die
        // Spalten rechts rutschen aus dem Bild.
        const float top = ImGui::GetCursorPosY();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
        ImGui::BeginChild("toolbar", ImVec2{restW, l.headerH},
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        if (g_app->leftMode == 1) {
            drawMapToolbar();
        } else {
            drawModelToolbar();
        }
        ImGui::EndChild();
        ImGui::SetCursorPosY(top + l.headerH);
        // Ein leeres Element als Anker: ImGui verlangt nach SetCursorPos ein
        // abgeschicktes Element, sonst bricht es ab. Was danach kommt, kann
        // vorzeitig zurueckkehren - der Anker nicht.
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        if (ImGui::BeginChild(g_app->leftMode == 1 ? "mapview" : "modelview",
                              ImVec2{restW, l.frameH},
                              ImGuiChildFlags_Borders)) {
            if (g_app->leftMode == 1) {
                drawMapView();
            } else {
                drawModelView();
            }
        }
        ImGui::EndChild();
        ImGui::EndGroup();
        // Auch HIER die Tabelle schliessen.
        //
        // Genau das hat rc501 zum Absturz gebracht:
        //
        //   IMGUI-ZUSICHERUNG: (0) && "Missing EndTable()"
        //
        // Ich hatte im Kommentar am Funktionsende behauptet, ein `return`
        // dazwischen gaebe es nur bei fehlgeschlagenem BeginTable. Das war
        // falsch - dieses hier stand schon immer da, und ich habe es beim
        // Umbau nicht gesucht, sondern angenommen.
        ImGui::EndTable();
        return;
    }

    if (beginGroupBox(tr(Str::GroupEvents), "events",
                      ImVec2{restW, l.frameH}, l.headerH)) {
        for (std::size_t i = 0; i < g_app->db.commands.size(); ++i) {
            const Command& c = g_app->db.commands[i];
            const std::string label = c.name + "  " + signatureText(c);
            ImGui::PushID(static_cast<int>(i));
            drawIcon(c.icon.empty() ? (c.block ? "I_BRACE" : "I_EVENT") : c.icon.c_str(),
                     false);
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            // Doppelklick FUEGT EIN - er oeffnet nicht den Editor. So macht
            // es das Original: der Befehl landet mit seinen Vorgabewerten im
            // Baum, und erst ein Doppelklick DORT oeffnet den Event-Editor.
            const bool sel = (static_cast<int>(i) == g_app->selectedCommand);
            const KeepSelected keep(sel, g_app->selectionOwner == 1);
            if (ImGui::Selectable(label.c_str(), sel,
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                const bool doppelt =
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                // Ein zweiter Klick auf DASSELBE schaltet die Auswahl ab.
                //
                // Vorher blieb sie kleben: einmal angeklickt, und die Zeile
                // war blau, bis man eine andere waehlte. Beim Doppelklick
                // gilt das nicht - der fuegt ein, und dabei soll die
                // Auswahl stehen bleiben.
                if (sel && !doppelt) {
                    g_app->selectedCommand = -1;
                } else {
                    g_app->selectedCommand = static_cast<int>(i);
                    g_app->selectionOwner = 1;
                }
                if (doppelt) {
                    insertCommand(c);
                }
            }
            // Ziehen statt Doppelklick. Der Doppelklick bleibt - er setzt
            // hinter die Auswahl, wie das Original. Ziehen sagt dagegen
            // GENAU, wohin: auch zwischen zwei bestehende Zeilen.
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
                const NewDrag d{0, static_cast<int>(i)};
                ImGui::SetDragDropPayload("bhed_new", &d, sizeof(d));
                drawIcon(c.icon.empty() ? (c.block ? "I_BRACE" : "I_EVENT")
                                        : c.icon.c_str(),
                         false);
                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::TextUnformatted(c.name.c_str());
                ImGui::EndDragDropSource();
            }
            // descFor statt c.desc: vier Befehle haben in der .bhc keine
            // Beschreibung (camera, task, do, play), und dafuer gibt es
            // einen Ersatz - siehe descFor().
            const std::string hilfe = descFor(c);
            if (!hilfe.empty()) {
                // AN DER MAUS zeigen, nicht nur unten in der Hilfszeile.
                //
                // Wer die Liste durchgeht, schaut auf die Liste - der Blick
                // an den unteren Fensterrand und zurueck kostet mehr, als
                // die Beschreibung wert ist.
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                    ImGui::SetTooltip("%s", hilfe.c_str());
                }
            }
            ImGui::PopID();
        }
        for (std::size_t mi = 0; mi < g_app->db.macroBodies.size(); ++mi) {
            const Macro& m = g_app->db.macroBodies[mi];
            ImGui::PushID(m.name.c_str());
            drawIcon("I_MACRO", false);
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            const std::string label =
                m.name + "  (" + std::to_string(m.body.size()) + ")";
            if (ImGui::Selectable(label.c_str(), false,
                                  ImGuiSelectableFlags_AllowDoubleClick) &&
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                insertMacro(m);
            }
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
                const NewDrag d{1, static_cast<int>(mi)};
                ImGui::SetDragDropPayload("bhed_new", &d, sizeof(d));
                drawIcon("I_MACRO", false);
                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::TextUnformatted(m.name.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::IsItemHovered() && !m.desc.empty()) {
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                    ImGui::SetTooltip("%s", m.desc.c_str());
                }
            }
            ImGui::PopID();
        }

        // Dieselbe Regel wie im Baum: ins Leere geklickt heisst nichts
        // gewaehlt. Vorher blieb der einmal angeklickte Befehl fuer immer
        // blau, egal wohin man danach klickte.
        const ImVec2 rest = ImGui::GetContentRegionAvail();
        if (rest.y > 1.0F) {
            ImGui::InvisibleButton("##leer", ImVec2{std::max(rest.x, 1.0F), rest.y});
            if (ImGui::IsItemClicked() && !ImGui::GetIO().KeyShift &&
                !ImGui::GetIO().KeyCtrl) {
                g_app->selectedCommand = -1;
            }
        }
        // Und ein Klick IRGENDWO in die Liste, der keinen Eintrag trifft.
        //
        // Die Leerflaeche oben gibt es nur, wenn die Liste kuerzer ist als
        // das Fenster. Bei einer vollen Liste - und die Ereignisliste ist
        // immer voll - blieb die Auswahl haengen, egal wohin man klickte.
        // Deshalb zusaetzlich: Fenster getroffen, kein Eintrag darunter,
        // keine Zusatztaste.
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsAnyItemHovered() && !ImGui::GetIO().KeyShift &&
            !ImGui::GetIO().KeyCtrl) {
            g_app->selectedCommand = -1;
        }
    }
    endGroupBox();
    // Die Tabelle der linken Spalte schliessen. Sie wurde ganz oben in
    // dieser Funktion geoeffnet.
    //
    // ACHTUNG: es gibt einen ZWEITEN Ausstieg weiter oben (Karte und
    // Modell), und der schliesst sie ebenfalls. Wer hier einen dritten
    // einbaut, muss es auch tun - `lint_endtable.py` prueft das.
    ImGui::EndTable();
}


// Ist diese Zeile ausgewaehlt?

// Die Auswahl fuer die Bearbeitung. Leer heisst: nichts gewaehlt.

// Auswahl mit den Pfeiltasten bewegen.
//
// Ueber die ANGEZEIGTEN Zeilen, nicht ueber die Baumpfade: der Nutzer sieht
// Zeilen und erwartet, dass die naechste Zeile drankommt - auch wenn sie
// eine Ebene tiefer liegt.
void moveSelectionByKey(int delta) {
    if (g_app->rows.empty()) {
        return;
    }
    int now = g_app->selected;
    if (now < 0) {
        now = (delta > 0) ? -1 : static_cast<int>(g_app->rows.size());
    }
    const int next = std::clamp(now + delta, 0,
                                static_cast<int>(g_app->rows.size()) - 1);
    if (next == now) {
        return;
    }
    selectRow(static_cast<std::size_t>(next), false,
              ImGui::GetIO().KeyShift);
    g_app->mapDirty = true;
    g_app->scrollToSelected = true;
}

// Die Spaltenmasse kommen aus dem Kern (bhed/tree.h) - dort stehen sie mit
// den gemessenen Werten und werden von einer Probe festgehalten. Hier kommt
// nur noch die eine Sache dazu, die ImGui braucht: kein Befehlsname darf in
// die Klammerspalte laufen.
TreeMetrics treeMetrics() {
    TreeMetrics m = treeMetricsFor(ImGui::GetFontSize());
    float longest = 0.0F;
    for (const Command& c : g_app->db.commands) {
        longest = std::max(longest, ImGui::CalcTextSize(c.name.c_str()).x);
    }
    m.nameW = std::max(m.nameW, longest + ImGui::GetStyle().ItemInnerSpacing.x * 2.0F);
    return m;
}

// Eine gepunktete Linie, wie sie eine Win32-Baumansicht zieht: ein Punkt,
// ein Punkt Luecke. Am Original nachgemessen - dort sitzt jeder zweite
// Bildpunkt. Deshalb ein Rechteck je Punkt und keine durchgezogene Linie.
void dottedLine(ImDrawList* dl, float x0, float y0, float x1, float y1,
                ImU32 col) {
    if (y0 == y1) {                                   // waagerecht
        const auto start = static_cast<int>(std::ceil(x0));
        for (int x = start; x < static_cast<int>(x1); x += 2) {
            dl->AddRectFilled(ImVec2{static_cast<float>(x), y0},
                              ImVec2{static_cast<float>(x) + 1.0F, y0 + 1.0F}, col);
        }
        return;
    }
    const auto start = static_cast<int>(std::ceil(y0));
    for (int y = start; y < static_cast<int>(y1); y += 2) {
        dl->AddRectFilled(ImVec2{x0, static_cast<float>(y)},
                          ImVec2{x0 + 1.0F, static_cast<float>(y) + 1.0F}, col);
    }
}

// Das Aufklappkaestchen des Originals: ein Quadrat mit Rahmen, darin ein
// Minus, bei zugeklappten Bloecken zusaetzlich ein senkrechter Strich.
void expanderBox(ImDrawList* dl, ImVec2 centre, float side, bool open,
                 ImU32 border, ImU32 sign) {
    const float h = std::floor(side * 0.5F);
    const ImVec2 a{centre.x - h, centre.y - h};
    const ImVec2 b{centre.x + h + 1.0F, centre.y + h + 1.0F};
    // Gefuellt, nicht durchsichtig: sonst laeuft die gepunktete Linie mitten
    // durch das Quadrat. Im Original ist es innen leer.
    dl->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_WindowBg));
    dl->AddRect(a, b, border);
    const float pad = 2.0F;
    dl->AddRectFilled(ImVec2{a.x + pad, centre.y},
                      ImVec2{b.x - pad, centre.y + 1.0F}, sign);
    if (!open) {
        dl->AddRectFilled(ImVec2{centre.x, a.y + pad},
                          ImVec2{centre.x + 1.0F, b.y - pad}, sign);
    }
}

// Die rechte Haelfte der geteilten Ansicht.
//
// Bewusst SCHLICHT: Name und Argumente, Einrueckung, anklickbar zum
// Kopieren. Kein Ziehen, kein Bearbeiten, keine Mehrfachauswahl.
//
// Warum nicht derselbe Baum wie links: der schreibt in g_app->rows,
// g_app->selection und g_app->selected, und daran haengt alles - Einfuegen,
// Loeschen, Rueckgaengig. Zwei davon nebeneinander hiesse, jede dieser
// Stellen zu verdoppeln. Die rechte Haelfte soll aber gar nicht
// bearbeiten; sie soll zeigen und hergeben. Dafuer genuegt das hier.

// Einen Knoten von einem Feld in ein anderes ziehen.
//
// Kopieren, nicht verschieben. Das ist die vorsichtige Wahl: ein Zug, der
// im Quellskript etwas loescht, kann bei einem Fehlgriff Arbeit vernichten,
// und Rueckgaengig muesste dann in ZWEI Dokumenten zurueckgenommen werden,
// um den alten Stand herzustellen. Wer verschieben will, loescht danach von
// Hand - das ist ein Handgriff und jederzeit umkehrbar.
//
// Eingefuegt wird HINTER der Zeile, auf die man loslaesst - so, wie man es
// beim Ablegen erwartet. `nachRow == -1` heisst: auf die freie Flaeche
// darunter, also ans Ende des Skripts (auch eines leeren).
void knotenZiehen(const App::DragNode& von, int nachPane, int nachRow);

// Der Knoten hinter einer "bhed_split"-Nutzlast (Feld, Zeile).
const Node* knotenAusNebenfeld(const ImGuiPayload* nutzlast) {
    if (nutzlast == nullptr ||
        nutzlast->DataSize != static_cast<int>(sizeof(int) * 2)) {
        return nullptr;
    }
    int nutz[2] = {0, 0};
    std::memcpy(nutz, nutzlast->Data, sizeof(nutz));
    const int vonPane = std::clamp(nutz[0], 0, App::kMaxSplit - 1);
    const App::SplitPane& quelle = g_app->splitPanes[vonPane];
    const std::size_t at = static_cast<std::size_t>(std::max(nutz[1], 0));
    if (at >= quelle.rows.size() || quelle.tab < 0 ||
        quelle.tab >= static_cast<int>(g_app->tabs.size())) {
        return nullptr;
    }
    const Script& src =
        (quelle.tab == g_app->activeTab)
            ? g_app->doc.script()
            : g_app->tabs[static_cast<std::size_t>(quelle.tab)].doc.script();
    return nodeAt(src, quelle.rows[at].path);
}

// Die freie Flaeche eines Feldes als Ablageziel.
//
// shank, mit zwei Bildern: "ich kann nichts in ein leeres Feld ziehen ...
// auch nichts in das leere Feld unter den anderen, dass es darunter
// eingefuegt wird."
//
// Bisher nahm nur eine ZEILE einen Zug an. Wo keine Zeile ist - unter der
// letzten, oder in einem leeren Skript ("No script loaded") -, gab es kein
// Ziel, und der Zug verpuffte.
//
// ImGui-Weg (Diskussion #1771): nach EndChild ist das ganze Kindfenster ein
// Element, und BeginDragDropTarget danach macht die GANZE Flaeche zum Ziel.
// Die Zeilen darin bleiben vorn: bei ueberlappenden Zielen nimmt
// AcceptDragDropPayload das mit der KLEINEREN Flaeche (imgui.cpp,
// `DragDropAcceptIdCurrRectSurface`). Auf einer Zeile gilt also weiter
// "hinter diese Zeile", daneben "ans Ende".
//
// `zielPane` -1 ist der Hauptbaum (fokussiertes Feld, aktiver Reiter).
void ablageFlaeche(int zielPane) {
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }
    const int nTabs = static_cast<int>(g_app->tabs.size());
    const int zielTab =
        (zielPane < 0)
            ? g_app->activeTab
            : g_app->splitPanes[std::clamp(zielPane, 0, App::kMaxSplit - 1)].tab;
    if (nTabs > 0 && (zielTab < 0 || zielTab >= nTabs)) {
        ImGui::EndDragDropTarget();
        return;
    }
    Document& ziel = (nTabs == 0 || zielTab == g_app->activeTab)
                         ? g_app->doc
                         : g_app->tabs[static_cast<std::size_t>(zielTab)].doc;
    bool geaendert = false;
    std::string was;

    // Neu aus der Ereignisliste.
    if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("bhed_new")) {
        if (pl->DataSize == static_cast<int>(sizeof(NewDrag))) {
            NewDrag neu{};
            std::memcpy(&neu, pl->Data, sizeof(neu));
            if (&ziel == &g_app->doc) {
                const std::size_t vorher = g_app->doc.script().nodes.size();
                dropNewAt(neu, Path{}, DropWhere::After);
                geaendert = g_app->doc.script().nodes.size() != vorher;
            } else {
                for (const Node& n : knotenFuerNeu(neu)) {
                    geaendert = ziel.insertAfter(Path{}, n) || geaendert;
                }
            }
            was = "neu aus der Ereignisliste";
        }
    }
    // Kopie aus einem Nebenfeld.
    if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("bhed_split")) {
        if (const Node* nd = knotenAusNebenfeld(pl)) {
            const Node kopie = *nd;
            geaendert = ziel.insertAfter(Path{}, kopie);
            was = "Kopie aus Nebenfeld \"" + kopie.name + "\"";
        }
        g_app->dragFrom.clear();
        g_app->dragFromTab = -1;
    }
    // Eine Zeile aus dem Hauptbaum oder einem Nebenfeld.
    if (ImGui::AcceptDragDropPayload("bhed_node") != nullptr) {
        const bool imselben = zielPane < 0 && g_app->dragging &&
                              g_app->dragFromPane < 0 &&
                              g_app->dragFromTab == g_app->activeTab;
        if (imselben) {
            // Umordnen ans Ende: hinter die letzte Zeile der obersten Ebene.
            const std::size_t n = g_app->doc.script().nodes.size();
            const std::vector<Path> m = selectionOrCurrent();
            const std::vector<Kennung> bewegt = kennungenVon(
                m.size() > 1 ? m : std::vector<Path>{g_app->dragFrom});
            if (n > 0) {
                const Path letzte{n - 1};
                if (m.size() > 1) {
                    geaendert = g_app->doc.moveAllTo(m, letzte);
                } else if (!g_app->dragFrom.empty() && g_app->dragFrom != letzte) {
                    geaendert = g_app->doc.moveTo(g_app->dragFrom, letzte, nullptr);
                }
                if (geaendert) {
                    markiereKennungen(bewegt);
                } else {
                    zeigeAbweisung();
                }
            }
            was = "ans Ende verschoben";
        } else if (!g_app->dragFrom.empty()) {
            App::DragNode dn;
            dn.pane = g_app->dragFromPane;
            dn.tab = g_app->dragFromTab;
            dn.row = -1;
            knotenZiehen(dn, zielPane, -1);   // schreibt selbst ins Protokoll
            if (zielPane >= 0) {
                g_app->splitPanes[zielPane].dirty = true;
            }
        }
        g_app->dragging = false;
        g_app->dbgZiehWeg.clear();
        g_app->dragFrom.clear();
        g_app->dragFromTab = -1;
    }
    ImGui::EndDragDropTarget();
    if (!geaendert) {
        return;
    }
    diag::detail("Ablegen auf freier Flaeche von Feld " + std::to_string(zielPane) +
                 " (Reiter " + std::to_string(zielTab) + "): " + was + " - ans Ende");
    for (App::SplitPane& sp : g_app->splitPanes) {
        if (sp.tab == zielTab) { sp.dirty = true; }
    }
    if (&ziel == &g_app->doc) {
        rebuildTree();
    }
}

void knotenZiehen(const App::DragNode& von, int nachPane, int nachRow) {
    // -1 heisst HAUPTBAUM.
    //
    // Er gehoert dem fokussierten Feld und zeigt den aktiven Reiter; seine
    // Zeilen stehen in g_app->rows, nicht in einem Feld. Beide Seiten
    // koennen der Hauptbaum sein - man zieht aus ihm heraus UND in ihn
    // hinein.
    const int n = static_cast<int>(g_app->tabs.size());
    const std::vector<Row>& qz =
        (von.pane < 0) ? g_app->rows
                       : g_app->splitPanes[std::clamp(von.pane, 0,
                                                      App::kMaxSplit - 1)]
                             .rows;
    const std::vector<Row>& zz =
        (nachPane < 0) ? g_app->rows
                       : g_app->splitPanes[std::clamp(nachPane, 0,
                                                      App::kMaxSplit - 1)]
                             .rows;
    const int zielTab =
        (nachPane < 0)
            ? g_app->activeTab
            : g_app->splitPanes[std::clamp(nachPane, 0, App::kMaxSplit - 1)]
                  .tab;
    if (von.pane >= App::kMaxSplit || nachPane >= App::kMaxSplit) {
        return;
    }
    if (von.tab < 0 || von.tab >= n || zielTab < 0 || zielTab >= n) {
        return;
    }
    if (nachRow < -1 || nachRow >= static_cast<int>(zz.size())) {
        return;
    }
    // Der QUELLWEG kommt aus dragFrom, nicht aus einer Zeilennummer.
    //
    // Zeilennummern gelten nur im jeweiligen Baum: derselbe Knoten steht im
    // Hauptbaum an einer anderen Stelle als im Vergleichsfeld, weil dort
    // andere Zweige auf- oder zugeklappt sind. Der Weg dagegen ist
    // eindeutig.
    if (g_app->dragFrom.empty()) {
        return;
    }
    (void)qz;

    // Aus welchem Dokument? Der aktive Reiter hat das LEBENDE, die anderen
    // ihr geparktes - dieselbe Unterscheidung wie ueberall sonst.
    const Script& quellSkript =
        (von.tab == g_app->activeTab)
            ? g_app->doc.script()
            : g_app->tabs[static_cast<std::size_t>(von.tab)].doc.script();
    const Node* nd = nodeAt(quellSkript, g_app->dragFrom);
    if (nd == nullptr) {
        return;
    }
    const Node kopie = *nd;
    // Leerer Weg: insertAfter haengt ans Ende.
    const Path zielWeg =
        (nachRow < 0) ? Path{} : zz[static_cast<std::size_t>(nachRow)].path;

    Document& ziel =
        (zielTab == g_app->activeTab)
            ? g_app->doc
            : g_app->tabs[static_cast<std::size_t>(zielTab)].doc;
    const bool ok = ziel.insertAfter(zielWeg, kopie);

    // Ins Protokoll, mit allem, was zum Nachvollziehen noetig ist. Beim
    // Ziehen zwischen zwei Dokumenten sieht man im Bild nur das Ergebnis -
    // nicht, WOHER es kam.
    char z[320];
    std::snprintf(z, sizeof(z),
                  "Ziehen: \"%s\" aus Feld %d (Reiter %d) nach Feld %d "
                  "(Reiter %d), hinter Zeile %d (-1 = ans Ende) - %s",
                  nd->name.c_str(),
                  von.pane, von.tab, nachPane, zielTab, nachRow,
                  ok ? "eingefuegt" : "ABGELEHNT");
    diag::detail(z);
    if (!ok) {
        return;
    }

    // Der Baum des Ziels muss neu - und wenn das Ziel das lebende Dokument
    // ist, auch der Hauptbaum.
    if (nachPane >= 0) {
        g_app->splitPanes[nachPane].dirty = true;
    }
    if (zielTab == g_app->activeTab) {
        rebuildTree();
    }
}

// Breite des Randes links im Baum: Platz fuer den Lesezeichenpunkt und den
// Aenderungsstreifen daneben.
float rinnenBreite() {
    return std::floor(ImGui::GetFontSize() * 1.15F);
}

// Eine Zeile im Rand: rechts der Aenderungsstreifen (orange / gruen /
// blassblau), links davon der Lesezeichenpunkt - die Anordnung aus
// Notepad++ (Lesezeichen, dann Aenderungsrand, dann der Text).
void zeichneRinne(ImDrawList* dl, float x0, float breite, float y, float hoehe,
                  std::uint8_t marke, bool lesezeichen) {
    const float streifen = std::max(3.0F, std::floor(ImGui::GetFontSize() * 0.2F));
    if (marke != kMarkeKeine) {
        const ImU32 farbe = (marke == kMarkeGeaendert)     ? IM_COL32(255, 145, 30, 255)
                            : (marke == kMarkeGespeichert) ? IM_COL32(70, 185, 70, 255)
                                                           : IM_COL32(120, 180, 235, 255);
        dl->AddRectFilled(ImVec2{x0 + breite - streifen - 1.0F, y},
                          ImVec2{x0 + breite - 1.0F, y + hoehe}, farbe);
    }
    if (lesezeichen) {
        const float r = std::floor(ImGui::GetFontSize() * 0.32F);
        const ImVec2 m{x0 + (breite - streifen) * 0.5F, y + ImGui::GetFontSize() * 0.5F};
        dl->AddCircleFilled(m, r, IM_COL32(45, 110, 225, 255));
        dl->AddCircleFilled(ImVec2{m.x - r * 0.3F, m.y - r * 0.3F}, r * 0.35F,
                            IM_COL32(160, 200, 255, 255));   // Glanzpunkt wie in Notepad++
        dl->AddCircle(m, r, IM_COL32(20, 60, 150, 255));
    }
}

void drawSplitTree(const Layout& l, int pane, float breite, float hoehe) {
    App::SplitPane& sp = g_app->splitPanes[pane];
    const int n = static_cast<int>(g_app->tabs.size());
    sp.tab = std::clamp(sp.tab, 0, std::max(0, n - 1));

    // Dieselbe Kennung wie im fokussierten Zustand - siehe drawTree().
    const std::string kennung = "pane" + std::to_string(pane);
    // Ein Klick irgendwohin in dieses Feld gibt ihm den Fokus - danach ist
    // es das bearbeitbare. So macht es VS Code auch: die angeklickte Gruppe
    // bekommt den Schreibzeiger.
    //
    // Angefordert, nicht sofort ausgefuehrt: mitten im Zeichnen das
    // Dokument zu tauschen zoege dem gerade gezeichneten Baum den Boden
    // weg. Das Raster fuehrt es nach dem Zeichnen aus.
    auto fokusAnfordern = [pane] {
        if (g_app->focusPane != pane) {
            g_app->focusRequest = pane;
        }
    };
    // Die Ueberschrift haengt am ORT, nicht am Fokus.
    //
    // Gemeldet: "Script Flow sollte oben links bleiben und nicht immer
    // springen wenn ich auf ein Compare druecke". Vorher trug das
    // FOKUSSIERTE Feld den Titel "Script Flow" - klickte man ins Feld
    // rechts, wanderte die Ueberschrift mit, und links stand ploetzlich
    // "Compare". Die Aufteilung sah nach jedem Klick anders aus.
    //
    // Jetzt heisst Feld 0 immer "Script Flow" und die uebrigen immer
    // "Compare", egal wo der Fokus steht. Welches Feld bearbeitbar ist,
    // zeigt der leuchtende Rahmen - dafuer ist er da.
    // Welcher Reiter? Ein Klappfeld UEBER der Liste - es rollt nicht mit.
    const auto kopf = [&] {
        // Welcher Reiter? Ein Klappfeld ueber der Liste.
        ImGui::SetNextItemWidth(breite);
        std::string aktuell = "-";
        if (n > 0) {
            const std::size_t at = static_cast<std::size_t>(sp.tab);
            const bool aktiv = (sp.tab == g_app->activeTab);
            const std::string& tp = aktiv ? g_app->path : g_app->tabs[at].path;
            const std::string& tn =
                aktiv ? g_app->shownName : g_app->tabs[at].shownName;
            aktuell = !tp.empty() ? fileName(tp)
                      : !tn.empty() ? tn
                                    : std::string("unnamed.txt");
        }
        const std::string pickName =
            std::string("##splitpick") + std::to_string(pane);
        const bool pickAuf = ImGui::BeginCombo(pickName.c_str(), aktuell.c_str());
        if (GImGui->TestEngineHookItems) {
            selbsttestMerkeText(GImGui, ImGui::GetID(pickName.c_str()),
                                pickName.c_str());
        }
        if (pickAuf) {
            for (int i = 0; i < n; ++i) {
                const std::size_t at = static_cast<std::size_t>(i);
                const bool aktiv = (i == g_app->activeTab);
                const std::string& tp = aktiv ? g_app->path : g_app->tabs[at].path;
                const std::string& tn =
                    aktiv ? g_app->shownName : g_app->tabs[at].shownName;
                std::string nm = !tp.empty() ? fileName(tp)
                                 : !tn.empty() ? tn
                                               : std::string("unnamed.txt");
                nm += "###sp" + std::to_string(pane) + "_" + std::to_string(i);
                // Was schon in einem ANDEREN Feld steht, ist abgeblendet.
                // Dasselbe Skript zweimal nebeneinander ist verwirrend und
                // wird nicht gebraucht - "being able to view the same
                // script side by side is also a little confusing and not
                // needed".
                bool anderswo = (i == g_app->activeTab && pane != g_app->focusPane);
                for (int q = 0; q < g_app->splitCount && !anderswo; ++q) {
                    if (q != pane && q != g_app->focusPane &&
                        g_app->splitPanes[q].tab == i) {
                        anderswo = true;
                    }
                }
                ImGui::BeginDisabled(anderswo);
                if (ImGui::Selectable(nm.c_str(), i == sp.tab)) {
                    sp.tab = i;
                    sp.dirty = true;
                    sp.selected = -1;
                }
                ImGui::EndDisabled();
            }
            ImGui::EndCombo();
        }
    };
    if (beginGroupBox(tr(pane == 0 ? Str::GroupScriptFlow : Str::SplitTitle),
                      kennung.c_str(), ImVec2{breite, hoehe}, l.headerH, kopf,
                      [pane] { ansichtsUmschalter(pane); })) {

        // Den Baum bauen, wenn noetig.
        //
        // Der aktive Reiter wird IMMER neu gebaut: er aendert sich unter
        // der Hand, sobald man links etwas bearbeitet. Die geparkten
        // aendern sich nicht, solange sie geparkt sind.
        const bool zeigtAktiven = (sp.tab == g_app->activeTab);
        if (sp.dirty || zeigtAktiven) {
            const Script& sc = zeigtAktiven
                                   ? g_app->doc.script()
                                   : g_app->tabs[static_cast<std::size_t>(
                                                     sp.tab)]
                                         .doc.script();
            TreeOptions o = g_app->treeOpt;
            o.expanded = &sp.expanded;
            buildTree(sc, g_app->db, o, sp.rows);
            sp.marke = zeilenMarken(sp.tab, sc, sp.rows);
            sp.dirty = false;
            // Die Auswahl des Feldes haengt am WEG, nicht an der
            // Zeilennummer - dieselbe Regel wie in rebuildTree(). Der
            // aktive Reiter wird jede Runde neu gebaut; eine gemerkte
            // NUMMER zeigte nach jeder Einfuegung auf die falsche Zeile
            // und wanderte sichtbar durch die Liste.
            sp.selected = -1;
            if (!sp.path.empty()) {
                for (std::size_t ri = 0; ri < sp.rows.size(); ++ri) {
                    if (sp.rows[ri].path == sp.path) {
                        sp.selected = static_cast<int>(ri);
                        break;
                    }
                }
            }
        }

        if (sp.rows.empty()) {
            ImGui::TextDisabled("%s", tr(Str::MsgNoScript));
        }
        // Gezeichnet wie links: gepunktete Linien, Kaestchen mit + und -,
        // Symbol, Name und Argumente in zwei Spalten.
        //
        // Dieselben Bausteine wie im linken Baum - dottedLine, expanderBox,
        // drawIcon - und dieselben Masse aus treeMetrics(). Waeren es
        // eigene, liefen die beiden Haelften mit der Zeit auseinander.
        const TreeMetrics tm = treeMetrics();
        const float rinne = rinnenBreite();
        ImGui::Indent(rinne);
        const float baseX = ImGui::GetCursorScreenPos().x;
        const float step = tm.indent;
        const ImU32 lineCol = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        std::vector<bool> more;
        rollpositionHolen(kennung, sp.tab);
        for (std::size_t i = 0; i < sp.rows.size(); ++i) {
            const Row& r = sp.rows[i];
            if (more.size() <= static_cast<std::size_t>(r.depth) + 1U) {
                more.resize(static_cast<std::size_t>(r.depth) + 2U, false);
            }
            more[static_cast<std::size_t>(r.depth)] = !r.lastChild;

            ImGui::PushID(static_cast<int>(i));
            const float rowY = ImGui::GetCursorScreenPos().y;
            const float rowH = ImGui::GetTextLineHeight();
            zeichneRinne(ImGui::GetWindowDrawList(), baseX - rinne, rinne, rowY,
                         rowH + ImGui::GetStyle().ItemSpacing.y,
                         i < sp.marke.size() ? sp.marke[i] : kMarkeKeine,
                         hatLesezeichen(sp.tab, r.kennung));
            if (r.depth != 0) {
                ImGui::Indent(step * static_cast<float>(r.depth));
            }
            // Erst ein Platzhalter fuer das Kaestchen, DANN SameLine.
            //
            // Ohne ihn steht das SameLine am Zeilenanfang - und ImGui setzt
            // die Zeile dann neben die VORIGE. Genau das war zu sehen: der
            // ganze Baum auf einer einzigen Zeile, von der nur der Anfang
            // ins Bild passte.
            //
            // Der linke Baum macht es genauso; ich hatte den Platzhalter
            // beim Nachbauen uebersehen, weil er dort in einem else-Zweig
            // steht.
            ImGui::Dummy(ImVec2{tm.box, rowH});
            ImGui::SameLine(0, tm.iconAt + std::floor(tm.box * 0.5F) - tm.box);
            // Nur das Feld mit dem FOKUS zeigt seine Auswahl blau.
            //
            // Gemeldet: "ich kann Befehle in mehreren Ansichten
            // hervorheben, und dann weiss ich nicht, welches Skript ich
            // gerade bearbeite."
            //
            // Zu Recht: bearbeitet wird immer nur EINES, und drei blaue
            // Zeilen behaupten das Gegenteil. Die Auswahl der anderen
            // Felder bleibt gemerkt - sie kommt zurueck, sobald man wieder
            // hineinklickt - aber sie wird nicht hervorgehoben.
            const bool sel = (static_cast<int>(i) == sp.selected) &&
                             (pane == g_app->focusPane);
            drawIcon(r.icon.c_str(), sel);
            ImGui::SameLine(0, tm.nameAt - tm.iconAt - ImGui::GetFontSize());
            const char* label = r.name.empty() ? r.text.c_str() : r.name.c_str();
            const ImVec2 nameMin = ImGui::GetCursorScreenPos();
            const bool angeklickt = ImGui::Selectable(
                label, sel, ImGuiSelectableFlags_AllowDoubleClick);

            // --- Ziehen: QUELLE ------------------------------------------
            //
            // Das Selectable ist die Quelle - ein Element mit eigener
            // Kennung, genau wie es die ImGui-Doku verlangt. Ein
            // Kindfenster taugt dafuer nicht (Fehlerbericht 7539: dort
            // liefert BeginDragDropSource schlicht false).
            //
            // Uebergeben wird NUR Feld, Reiter und Zeilennummer. ImGui
            // kopiert die Nutzlast sofort in einen eigenen Puffer; ein
            // Zeiger darin waere beim Loslassen laengst ungueltig.
            if (ImGui::BeginDragDropSource()) {
                // DIESELBE Nutzlast wie im Hauptbaum ("bhed_node"), damit
                // beide Seiten einander verstehen. Die Einzelheiten stehen
                // nicht darin, sondern in dragFrom/dragFromTab: ImGui
                // kopiert die Nutzlast, und drei Zahlen doppelt zu fuehren
                // waere eine zweite Wahrheit.
                g_app->dragFrom = r.path;
                g_app->dragFromTab = sp.tab;
                g_app->dragFromPane = pane;
                const std::size_t tiefe = r.path.size();
                ImGui::SetDragDropPayload("bhed_node", &tiefe, sizeof(tiefe));
                ImGui::TextUnformatted(label);
                ImGui::EndDragDropSource();
            }
            // --- Ziehen: ZIEL --------------------------------------------
            //
            // Hier wird eingefuegt, nicht in der Quelle: die Quelle erfaehrt
            // gar nicht, ob die Uebergabe geklappt hat (Fehlerbericht 8225).
            if (ImGui::BeginDragDropTarget()) {
                // Alle drei Arten, die es gibt. Nimmt die Zeile eine davon
                // nicht an, fiele der Zug auf die Flaeche darunter durch
                // (ablageFlaeche) und landete am ENDE statt hinter dieser
                // Zeile. Ein Zug aus einem Nebenfeld kommt als "bhed_split"
                // an, nicht als "bhed_node" - von zwei Quellen am selben
                // Element gilt die zweite.
                const bool zeile = ImGui::AcceptDragDropPayload("bhed_node") != nullptr ||
                                   ImGui::AcceptDragDropPayload("bhed_split") != nullptr;
                if (zeile) {
                    App::DragNode dn;
                    dn.pane = g_app->dragFromPane;
                    dn.tab = g_app->dragFromTab;
                    dn.row = -1;   // der WEG zaehlt, nicht die Zeilennummer
                    knotenZiehen(dn, pane, static_cast<int>(i));
                    g_app->dragging = false;
                    g_app->dragFrom.clear();
                    g_app->dragFromTab = -1;
                }
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("bhed_new")) {
                    if (pl->DataSize == static_cast<int>(sizeof(NewDrag)) &&
                        sp.tab >= 0 && sp.tab < n) {
                        NewDrag neu{};
                        std::memcpy(&neu, pl->Data, sizeof(neu));
                        Document& ziel =
                            (sp.tab == g_app->activeTab)
                                ? g_app->doc
                                : g_app->tabs[static_cast<std::size_t>(sp.tab)].doc;
                        Path hinter = r.path;
                        for (const Node& nd : knotenFuerNeu(neu)) {
                            if (!ziel.insertAfter(hinter, nd)) { break; }
                            ++hinter.back();
                        }
                        sp.dirty = true;
                        diag::detail("Ablegen im Nebenfeld " + std::to_string(pane) +
                                     ": neu aus der Ereignisliste hinter \"" + r.name + "\"");
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (angeklickt) {
                fokusAnfordern();
                // Dieselbe Zeile nochmal: Auswahl aufheben - in JEDEM der
                // vier Felder, nicht nur im lebenden.
                if (sel && !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    sp.selected = -1;
                    sp.path.clear();
                } else {
                sp.selected = static_cast<int>(i);
                sp.path = r.path;
                // KEIN eigenes Fenster mehr zum Ansehen.
                //
                // Als die Vergleichsfelder nur zeigen konnten, war ein
                // kleines Fenster die richtige Antwort: der Editor
                // bearbeitet das AKTIVE Dokument, und dort stand ein
                // anderes.
                //
                // Inzwischen holt derselbe Klick den FOKUS in dieses Feld -
                // damit wird sein Skript das aktive, und der volle Editor
                // unten gilt ihm. Das kleine Fenster zeigte daneben nur
                // dieselben Werte noch einmal, unbearbeitbar. Gemeldet als
                // "es kommt nur ein kleines fenster hoch statt das
                // vollstaendige - alle 4 sollten bearbeitbar sein".
                //
                // Die Auswahl wandert beim Fokuswechsel mit (rc165), also
                // steht nach dem Klick genau diese Zeile im Editor.
                }
            }
            // --- Doppelklick: den ganzen Text zeigen ---------------------
            //
            // Die rechte Haelfte ist schmal, und lange Argumente werden
            // abgeschnitten - "ich kann auch keine der Trigger doppel
            // klicken um die genauen bezeichnungen zu sehen". Links oeffnet
            // ein Doppelklick den Bearbeiter; hier waere das falsch, weil
            // sich rechts nichts aendern laesst. Also ein Fenster, das
            // ZEIGT.
            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                g_app->splitShowText = r.name + "  " + r.args;
                ImGui::OpenPopup("##splittext");
            }
            // Und schon beim Ueberfahren als Kurzhinweis - das ist der
            // schnellere Weg, wenn man nur nachsehen will.
            if (ImGui::IsItemHovered() && !r.args.empty() &&
                !ImGui::IsDragDropActive()) {
                ImGui::SetTooltip("%s %s", r.name.c_str(), r.args.c_str());
            }
            // --- Ziehen nach LINKS --------------------------------------
            //
            // Die Nutzlast ist die Zeilennummer in splitRows. Der Knoten
            // selbst wird erst beim Ablegen geholt: waehrend des Ziehens
            // koennte sich der rechte Baum neu bauen, und ein Zeiger darauf
            // waere dann ins Leere gerichtet.
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                // Spalte UND Zeile mitgeben - seit rc158 gibt es bis zu
                // drei Vergleichsspalten, und ohne die Spaltennummer weiss
                // die Gegenseite nicht, aus welcher der Knoten kommt.
                const int nutz[2] = {pane, static_cast<int>(i)};
                ImGui::SetDragDropPayload("bhed_split", nutz, sizeof(nutz));
                ImGui::TextUnformatted(label);
                if (!r.args.empty()) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", r.args.c_str());
                }
                ImGui::EndDragDropSource();
            }
            // Rechtsklick: in die Ablage des LINKEN Dokuments legen. Danach
            // fuegt Strg+V dort ein - genau der Weg, um den es geht.
            if (ImGui::BeginPopupContextItem("szeile")) {
                sp.selected = static_cast<int>(i);
                sp.path = r.path;
                if (ImGui::MenuItem(tr(Str::ActCopy))) {
                    const Script& src =
                        zeigtAktiven ? g_app->doc.script()
                                     : g_app->tabs[static_cast<std::size_t>(
                                                       sp.tab)]
                                           .doc.script();
                    if (const Node* nd = nodeAt(src, r.path)) {
                        g_app->doc.setClipboard({*nd});
                        ablageNachWindows();
                        setStatus(tr(Str::MsgCopied), 1, 0);
                    }
                }
                ImGui::EndPopup();
            }
            // Die Argumente in ihrer eigenen Spalte - GEZEICHNET, nicht
            // gesetzt, genau wie links. Mit SetCursorPos geraet die Zeile
            // aus dem Tritt; der Auswahlbalken reicht ohnehin bis zum
            // rechten Rand und deckt sie mit ab.
            //
            // Und dieselbe Regel des Originals: die Klammer beginnt nameW
            // hinter dem Namensanfang, rueckt aber nach rechts, wenn der
            // Name laenger ist - sonst ueberschreiben sie sich.
            if (!r.args.empty()) {
                const float argX = std::max(
                    nameMin.x + tm.nameW,
                    nameMin.x + ImGui::CalcTextSize(label).x +
                        ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::GetWindowDrawList()->AddText(
                    ImVec2{argX, nameMin.y},
                    ImGui::GetColorU32(ImGuiCol_Text), r.args.c_str());
            }
            // Linien und Kaestchen.
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const float top = rowY;
                const float bottom =
                    rowY + rowH + ImGui::GetStyle().ItemSpacing.y;
                const float mid = std::floor(rowY + rowH * 0.5F);
                const float half = std::floor(tm.box * 0.5F);
                for (int k = 0; k < r.depth; ++k) {
                    if (!more[static_cast<std::size_t>(k)]) {
                        continue;
                    }
                    const float x = std::floor(
                        baseX + step * static_cast<float>(k) + half);
                    dottedLine(dl, x, top, x, bottom, lineCol);
                }
                const float x = std::floor(
                    baseX + step * static_cast<float>(r.depth) + half);
                dottedLine(dl, x, top, x, mid, lineCol);
                if (!r.lastChild) {
                    dottedLine(dl, x, mid, x, bottom, lineCol);
                }
                dottedLine(dl, x + (r.childCount != 0 ? half + 1.0F : 1.0F),
                           mid, x + tm.iconAt, mid, lineCol);
                if (r.childCount != 0) {
                    expanderBox(dl, ImVec2{x, mid}, tm.box, r.open, lineCol,
                                ImGui::GetColorU32(ImGuiCol_Text));
                    // Und es laesst sich druecken. Der Bereich ist etwas
                    // groesser als das Kaestchen - neun Bildpunkte trifft
                    // niemand zuverlaessig.
                    const float griff = tm.box;
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                        ImGui::IsWindowHovered()) {
                        const ImVec2 mp = ImGui::GetIO().MousePos;
                        if (std::fabs(mp.x - x) <= griff &&
                            std::fabs(mp.y - mid) <= griff) {
                            sp.expanded.toggle(r.kennung);
                            sp.dirty = true;
                        }
                    }
                }
            }
            if (r.depth != 0) {
                ImGui::Unindent(step * static_cast<float>(r.depth));
            }
            // Das Fenster mit dem vollen Text - je Zeile eines, damit die
            // Kennung eindeutig bleibt.
            if (ImGui::BeginPopup("##splittext")) {
                ImGui::TextUnformatted(g_app->splitShowText.c_str());
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::Unindent(rinne);
        // Auch ein Klick auf freie Flaeche im Feld zaehlt - man will das
        // Feld auswaehlen koennen, ohne eine Zeile zu treffen.
        //
        // Und er hebt die Auswahl DIESES Feldes auf: "wenn ich irgendwo
        // anders drauf druecke sollte das auch nicht mehr gehighlighted
        // sein". Getroffen wird hier nur, was KEINE Zeile ist - ein Klick
        // auf eine Zeile hat das Selectable schon verbraucht.
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
            !ImGui::IsAnyItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            fokusAnfordern();
            sp.selected = -1;
            sp.path.clear();
        }
        rollpositionMerken(sp.tab);
    }
    endGroupBox();
    ablageFlaeche(pane);
}

void drawTree(const Layout& l, int pane, float breite, float hoehe);
void drawSplitTree(const Layout& l, int pane, float breite, float hoehe);

// Der Skriptbereich als RASTER.
//
// So wie in VS Code, und so wie du es beschrieben hast: bei drei Feldern
// nicht drei nebeneinander, sondern eines links und zwei rechts
// uebereinander. Bei vier ein Zweimalzwei.
//
//   1        2            3              4
//   +---+    +---+---+    +---+---+      +---+---+
//   |   |    |   |   |    |   | 0 |      |   | 0 |
//   | H |    | H | 0 |    | H +---+      | H +---+
//   |   |    |   |   |    |   | 1 |      +---+---+
//   +---+    +---+---+    +---+---+      | 1 | 2 |
//                                        +---+---+
//
// H ist der Hauptbaum - er bleibt immer der erste. Warum, steht in rc150:
// an rows, selection und selected haengt jede Bearbeitung.
//
// Gezeichnet wird mit ausdruecklichen Bildschirmorten statt mit SameLine.
// SameLine setzt neben das ZULETZT Gezeichnete, und in einem Raster ist
// das die falsche Bezugsgroesse - beim Sprung in die zweite Reihe muesste
// man rueckwaerts rechnen. Ein Ursprung plus Versatz ist hier einfacher zu
// lesen und stimmt in jeder Aufteilung.
void drawFlowArea(const Layout& l) {
    const int felder = (g_app->leftMode == 0)
                           ? std::clamp(g_app->splitCount, 1, App::kMaxSplit)
                           : 1;
    // Das Ganze in EINEM Kindfenster.
    //
    // Das ist der Kern der Behebung: SetCursorScreenPos verschiebt zwar den
    // Zeiger, sagt dem Elternfenster aber NICHT, wie viel Platz belegt
    // wurde. ImGui rechnet danach mit dem letzten gezeichneten Ding weiter -
    // und das war bei einem Raster das Feld unten rechts. Ergebnis: die
    // Aktionsspalte verrutschte, die Ueberschriften lagen uebereinander,
    // Zeitleiste und Meldungszeile fielen aus dem Bild.
    //
    // Ein Kindfenster fester Groesse ist genau EIN Ding von genau dieser
    // Groesse. Innen wird frei angeordnet, aussen merkt niemand etwas -
    // alles danach rechnet wieder richtig.
    // 0 heisst "die ganze Zelle". Die Tabelle hat die Breite schon
    // bestimmt; `l.flowW` waere die zweite Fassung derselben Zahl.
    const float w = 0.0F;
    // Die Kopfzeile steht ueber der Tabelle, nicht in der Zelle - hier
    // stand `l.frameH + l.headerH` und zog sie ein zweites Mal ab.
    const float h = l.frameH;
    // Ohne Innenabstand: sonst begaenne der Inhalt um die Polsterung
    // versetzt, und die Ueberschriften saessen nicht mehr auf derselben
    // Hoehe wie die der Nachbarspalten.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    // --- Der aeussere Kasten malt KEINEN Hintergrund ---------------------
    //
    // Hier liegen zwei Kindfenster ineinander: dieses als Traeger, und darin
    // die eigentlichen Felder mit ihren Rahmen. Jedes Kindfenster malt
    // `ImGuiCol_ChildBg`, und die Farbe ist halbdurchsichtig - zweimal
    // uebereinander ergibt einen dunkleren Ton als bei den Nachbarspalten.
    //
    // shank: "warum ist das Script Flow so dunkel hinterlegt?" Weil es
    // doppelt hinterlegt war.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    ImGui::BeginChild("##flowarea", ImVec2{w, h}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    // Die Farbe gleich zurueck: sie galt nur fuer das Anlegen dieses
    // Kastens, nicht fuer die Felder darin - die sollen ihren Hintergrund
    // behalten.
    ImGui::PopStyleColor();
    // --- Die ECHTE Groesse, nicht die Platzhalter -----------------------
    //
    // `w` und `h` sind seit dem Tabellenumbau Platzhalter fuer `BeginChild`:
    // 0 heisst "die ganze Zelle", negativ heisst "alles Uebrige minus
    // diesem Betrag". Als ZAHLEN sind sie unbrauchbar.
    //
    // Hier unten wurde damit weitergerechnet:
    //
    //     hw = floor(w * splitFracX)   ->  0
    //     hh = floor(h * splitFracY)   ->  negativ
    //
    // Das innere Feld bekam damit eine falsche Hoehe und endete zu frueh -
    // shank: "das Script Flow Fenster in der Mitte ist so als waere es
    // 2 Fenster in einem statt einem".
    //
    // Innerhalb des Kindfensters steht die wirkliche Groesse fest, und
    // `GetContentRegionAvail` nennt sie. Gemessen statt hergeleitet.
    const float wIst = ImGui::GetContentRegionAvail().x;
    const float hIst = ImGui::GetContentRegionAvail().y;
    const ImVec2 ursprung = ImGui::GetCursorScreenPos();
    // Graph oder Spuren (gui/app_ansichten.cpp): eine Ansicht ueber den ganzen
    // Bereich, ohne Felder. Classic bleibt der Baum unten - alle drei arbeiten
    // auf demselben Dokument.
    // Die Trennlinien liegen dort, wo der Anwender sie hingezogen hat.
    // Grenzen, damit kein Feld auf null schrumpft und unerreichbar wird.
    const float hw =
        std::floor(wIst * std::clamp(g_app->splitFracX, 0.15F, 0.85F));
    // Jedes Feld braucht seine eigene Ueberschriftszeile, also wird die
    // Hoehe der Kaesten aus der verfuegbaren abzueglich Ueberschrift
    // gerechnet. Bei einer Reihe ist das die volle, bei zweien die halbe.
    const float hh =
        std::floor(hIst * std::clamp(g_app->splitFracY, 0.15F, 0.85F));

    // Die Aufteilung ist in tests/gridtest.cpp nachgebaut und dort mit
    // sieben Aussagen festgehalten - lueckenlos, ueberschneidungsfrei, und
    // die Anordnung wie oben gezeichnet. Wer sie hier aendert, aendert die
    // Probe mit.
    // Ein fester Zwischenraum zwischen den Feldern.
    //
    // Ohne ihn stossen die Kaesten stumpf aneinander, und die Ueberschrift
    // des rechten sitzt direkt am Rand des linken - auf dem Bild sah es
    // aus, als lieferen sie ineinander. Ein Abstand von der Breite eines
    // Zeichenabstands trennt sie sichtbar, ohne Platz zu verschwenden.
    const float luecke = ImGui::GetStyle().ItemSpacing.x;

    struct Feld {
        float x;
        float y;
        float w;
        float h;
    };
    Feld felderPos[App::kMaxSplit];
    switch (felder) {
        case 2:
            felderPos[0] = {0.0F, 0.0F, hw - luecke, hIst};
            felderPos[1] = {hw, 0.0F, wIst - hw, hIst};
            break;
        case 3:
            felderPos[0] = {0.0F, 0.0F, hw - luecke, hIst};
            felderPos[1] = {hw, 0.0F, wIst - hw, hh - luecke};
            felderPos[2] = {hw, hh, wIst - hw, hIst - hh};
            break;
        case 4:
            felderPos[0] = {0.0F, 0.0F, hw - luecke, hh - luecke};
            felderPos[1] = {hw, 0.0F, wIst - hw, hh - luecke};
            felderPos[2] = {0.0F, hh, hw - luecke, hIst - hh};
            felderPos[3] = {hw, hh, wIst - hw, hIst - hh};
            break;
        default:
            felderPos[0] = {0.0F, 0.0F, wIst, hIst};
            break;
    }

    // Das Feld mit dem FOKUS zeigt das lebende Dokument und ist voll
    // bearbeitbar; die uebrigen zeigen ihren Reiter zum Lesen und
    // Uebernehmen. Wer in ein anderes klickt, gibt ihm den Fokus - danach
    // ist DIESES das bearbeitbare.
    const int fokus = std::clamp(g_app->focusPane, 0, felder - 1);
    for (int i = 0; i < felder; ++i) {
        const Feld& f = felderPos[i];
        ImGui::SetCursorScreenPos(ImVec2{ursprung.x + f.x, ursprung.y + f.y});
        if (feldAnsicht(i) != 0) {
            drawFlussAnsicht(f.w, f.h, l.headerH, i);
        } else if (i == fokus) {
            drawTree(l, i, f.w, f.h - l.headerH);
        } else if (!g_app->tabs.empty()) {
            drawSplitTree(l, i, f.w, f.h - l.headerH);
        }
    }
    // --- Die Trennlinien zum Ziehen --------------------------------------
    //
    // Ein unsichtbarer Knopf auf der Linie, der den Mauszeiger aendert und
    // beim Ziehen den Anteil verschiebt. Genauso macht es Blender mit den
    // Kanten zwischen Bereichen.
    //
    // NACH den Feldern gezeichnet, damit er obenauf liegt; sonst faengt das
    // Kindfenster darunter den Klick ab.
    const float griff = std::max(4.0F, ImGui::GetStyle().ItemSpacing.x);
    if (felder >= 2) {
        ImGui::SetCursorScreenPos(
            ImVec2{ursprung.x + hw - griff, ursprung.y});
        ImGui::InvisibleButton("##splitx", ImVec2{griff * 2.0F, hIst});
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        if (ImGui::IsItemActive() && wIst > 1.0F) {
            g_app->splitFracX =
                std::clamp(g_app->splitFracX +
                               ImGui::GetIO().MouseDelta.x / wIst,
                           0.15F, 0.85F);
            for (auto& pn : g_app->splitPanes) {
                pn.dirty = true;
            }
        }
    }
    if (felder >= 3) {
        // Bei drei Feldern trennt die waagerechte Linie nur die RECHTE
        // Haelfte, bei vieren die ganze Breite.
        const float x0 = (felder == 3) ? hw : 0.0F;
        ImGui::SetCursorScreenPos(
            ImVec2{ursprung.x + x0, ursprung.y + hh - griff});
        ImGui::InvisibleButton("##splity", ImVec2{wIst - x0, griff * 2.0F});
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
        if (ImGui::IsItemActive() && hIst > 1.0F) {
            g_app->splitFracY =
                std::clamp(g_app->splitFracY +
                               ImGui::GetIO().MouseDelta.y / hIst,
                           0.15F, 0.85F);
            for (auto& pn : g_app->splitPanes) {
                pn.dirty = true;
            }
        }
    }

    // Den angeforderten REITERWECHSEL zuerst - er kommt aus dem Klappfeld
    // des fokussierten Feldes und ist ein echter Reiterwechsel, also MIT
    // Rollen zur Auswahl.
    if (g_app->tabRequest >= 0) {
        const int ziel = std::clamp(
            g_app->tabRequest, 0, static_cast<int>(g_app->tabs.size()) - 1);
        g_app->tabRequest = -1;
        switchTab(ziel);
    }
    // Den angeforderten Fokuswechsel JETZT ausfuehren, nachdem alles
    // gezeichnet ist.
    //
    // Das lebende Dokument wechselt dabei den Reiter: das bisherige Feld
    // wird geparkt, der Reiter des neuen hervorgeholt. Zeigt das neue Feld
    // ohnehin denselben Reiter, bleibt alles stehen - dann wandert nur der
    // Fokus.
    if (g_app->focusRequest >= 0) {
        const int neu = std::clamp(g_app->focusRequest, 0, App::kMaxSplit - 1);
        g_app->focusRequest = -1;
        if (neu != g_app->focusPane && neu < felder && !g_app->tabs.empty()) {
            // Der Reiter, den das bisherige Feld zeigte, bleibt ihm
            // erhalten - dafuer merkt es sich ihn.
            App::SplitPane& alt = g_app->splitPanes[g_app->focusPane];
            alt.tab = g_app->activeTab;
            // Seine Auswahl BEKOMMT es nicht nachgeschoben.
            //
            // In rc165 wanderte die lebende Auswahl beim Fokuswechsel in
            // das abgebende Feld. Damit erschien dort ein Balken, den
            // niemand angeklickt hatte - gemeldet als "wird es
            // gehighlighted wenn ich von einem fenster zum anderen springe,
            // das zuckt". Jetzt gilt: eine Zeile ist blau, WEIL man sie
            // angeklickt hat, und aus keinem anderen Grund. Ein Feld ohne
            // Klick bleibt leer.
            //
            // Das Feld behaelt, was zuletzt IN IHM angeklickt wurde - das
            // steht schon in sp.path und wird hier nicht angeruehrt.
            g_app->focusPane = neu;
            const int zielTab =
                std::clamp(g_app->splitPanes[neu].tab, 0,
                           static_cast<int>(g_app->tabs.size()) - 1);
            // Kein Rollen, KEINE Aufteilung: nur der Fokus wandert, der
            // Inhalt bleibt, und die Felder bleiben stehen.
            //
            // Ins Protokoll, weil hier entschieden wird, WELCHES Skript die
            // naechste Aenderung bekommt. Geht spaeter eine Bearbeitung ins
            // falsche Skript, steht hier der Grund.
            {
                char z[240];
                const std::string zn =
                    (static_cast<std::size_t>(zielTab) < g_app->tabs.size() &&
                     !g_app->tabs[static_cast<std::size_t>(zielTab)]
                          .shownName.empty())
                        ? g_app->tabs[static_cast<std::size_t>(zielTab)]
                              .shownName
                        : std::string("unnamed.txt");
                std::snprintf(z, sizeof(z),
                              "Fokus: Feld %d -> Feld %d, jetzt aktiv Reiter "
                              "%d (%s)",
                              alt.tab == g_app->activeTab ? g_app->focusPane
                                                          : -1,
                              neu, zielTab, zn.c_str());
                diag::detail(z);
            }
            switchTab(zielTab, false, false);
            // Die im neuen Feld ANGEKLICKTE Zeile wird jetzt die lebende
            // Auswahl. Vorher blieb die alte Auswahl des Dokuments stehen,
            // und der Klick schien nichts zu tun - erst dieser Schritt
            // macht den Klick zu dem, was er in VS Code ist: Fokus UND
            // Schreibzeiger.
            App::SplitPane& nsp = g_app->splitPanes[neu];
            g_app->selection.clear();
            if (!nsp.path.empty() &&
                nodeAt(g_app->doc.script(), nsp.path) != nullptr) {
                g_app->selectedPath = nsp.path;
                g_app->selection.push_back(nsp.path);
            } else {
                // Im neuen Feld ist nichts gewaehlt - dann ist auch im
                // Dokument nichts gewaehlt. Sonst leuchtete nach dem
                // Wechsel eine Zeile auf, die zu einem ANDEREN Feld
                // gehoerte.
                g_app->selectedPath.clear();
            }
            rebuildTree();
            for (auto& pn : g_app->splitPanes) {
                pn.dirty = true;
            }
        }
    }

    ImGui::EndChild();
}

void drawTree(const Layout& l, int pane, float breite, float hoehe) {
    // Der Rahmen des fokussierten Feldes leuchtet - bei vier gleich
    // aussehenden Kaesten muss man sehen, welcher der bearbeitbare ist.
    const bool mehrere = (g_app->leftMode == 0 && g_app->splitCount > 1);
    if (mehrere) {
        ImGui::PushStyleColor(ImGuiCol_Border,
                              ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    // Die Kennung haengt am FELD, nicht am Zustand.
    //
    // Ein Kindfenster in ImGui ist ein eigenstaendiger Rollbereich, und
    // angesprochen wird es ueber seine Kennung - "use child windows to
    // begin into a self-contained independent scrolling/clipping region".
    // Was daran haengt, haengt an der Kennung: der Rollstand vor allem.
    //
    // Vorher hiess das fokussierte Feld "tree" und ein unfokussiertes
    // "splitN". Beim Fokuswechsel tauschten zwei Felder ihre Kennungen -
    // und damit ihre Rollbereiche. Fuer ImGui waren das andere Fenster,
    // also fingen beide wieder oben an. Das war das Umherspringen.
    //
    // Jetzt heisst jedes Feld immer gleich, egal ob es gerade das lebende
    // Dokument zeigt oder nicht. Der Rollstand bleibt, wo er war.
    const std::string kennung = "pane" + std::to_string(pane);
    // Bei mehreren Feldern das Klappfeld mit dem Reiternamen - UEBER der
    // Liste, damit es beim Rollen stehen bleibt (siehe beginGroupBox).
    const auto kopf = [&] {
        if (mehrere) {
            const int nT = static_cast<int>(g_app->tabs.size());
            ImGui::SetNextItemWidth(breite);
            std::string aktuell = !g_app->path.empty() ? fileName(g_app->path)
                                  : !g_app->shownName.empty()
                                      ? g_app->shownName
                                      : std::string("unnamed.txt");
            const std::string pickName =
                std::string("##splitpick") + std::to_string(pane);
            const bool pickAuf =
                ImGui::BeginCombo(pickName.c_str(), aktuell.c_str());
            // Fuer den Selbsttest: BeginCombo meldet seine Beschriftung nur
            // beim Aufklappen (siehe comboEdit).
            if (GImGui->TestEngineHookItems) {
                selbsttestMerkeText(GImGui, ImGui::GetID(pickName.c_str()),
                                    pickName.c_str());
            }
            if (pickAuf) {
                for (int i = 0; i < nT; ++i) {
                    const std::size_t at = static_cast<std::size_t>(i);
                    const bool aktiv = (i == g_app->activeTab);
                    const std::string& tp =
                        aktiv ? g_app->path : g_app->tabs[at].path;
                    const std::string& tn =
                        aktiv ? g_app->shownName : g_app->tabs[at].shownName;
                    std::string nm = !tp.empty() ? fileName(tp)
                                     : !tn.empty() ? tn
                                                   : std::string(
                                                         "unnamed.txt");
                    nm += "###sp" + std::to_string(pane) + "_" +
                          std::to_string(i);
                    if (ImGui::Selectable(nm.c_str(), aktiv) && !aktiv) {
                        g_app->tabRequest = i;
                    }
                }
                ImGui::EndCombo();
            }
        }
    };
    const bool offen =
        beginGroupBox(tr(pane == 0 ? Str::GroupScriptFlow : Str::SplitTitle),
                      kennung.c_str(), ImVec2{breite, hoehe}, l.headerH,
                      mehrere ? std::function<void()>(kopf) : std::function<void()>(),
                      [pane] { ansichtsUmschalter(pane); });
    if (mehrere) {
        ImGui::PopStyleColor();
    }
    if (offen) {
        // Bei mehreren Feldern steht auch HIER das Klappfeld mit dem
        // Reiternamen - aus zwei Gruenden. Erstens sieht das fokussierte
        // Feld damit aus wie die anderen; vorher verschob der Fokuswechsel
        // den Inhalt um genau die Hoehe des Klappfelds, und die Zeilen
        // sprangen. Zweitens wechselt man so den Reiter eines Feldes,
        // ohne es erst zum Vergleichsfeld machen zu muessen - wie in VS
        // Code, wo jede Gruppe ihre eigene Reiterleiste hat.
        //
        // Der Wechsel ist ANGEFORDERT, nicht sofort: switchTab mitten im
        // Zeichnen zoege dem Baum darunter den Boden weg.
        if (g_app->rows.empty()) {
            ImGui::TextDisabled("%s", tr(Str::MsgNoScript));
        }
        const TreeMetrics tm = treeMetrics();
        // Der Rand links fuer Lesezeichen und Aenderungsmarken (Notepad++).
        const float rinne = rinnenBreite();
        ImGui::Indent(rinne);
        g_app->rinneX0 = ImGui::GetCursorScreenPos().x - rinne;
        // Linke Kante des Inhalts. Alle Spalten rechnen von hier, nicht vom
        // Fenster - sonst wandern die Linien beim Scrollen quer.
        const float baseX = ImGui::GetCursorScreenPos().x;
        // Welche Ebenen laufen unter dieser Zeile weiter? Ein Kellerstapel
        // ueber die Zeilen; die flache Liste ist dafuer in der richtigen
        // Reihenfolge.
        std::vector<bool> more;
        const ImU32 lineCol = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        rollpositionHolen(kennung, g_app->activeTab);
        // Der Befehl, dessen Gleiche hervorgehoben werden (View-Menue).
        const Node* vorbild = nullptr;
        g_app->gleicheSichtbar = 0;
        if (g_app->settings.highlightSame && !g_app->selectedPath.empty()) {
            vorbild = nodeAt(g_app->doc.script(), g_app->selectedPath);
        }
        // Was den Baum SOFORT neu baut, laeuft erst nach der Schleife: der
        // Editor (bei einem Makro klappt er es um, openEditorForNode ruft
        // rebuildTree) und das Ziehen aus einem anderen Skript. Mitten in der
        // Schleife las sie danach weiter aus `r` - Speicher, den der Neubau
        // freigegeben hatte (ASan, Doppelklick auf eine Makrozeile,
        // Code-Pruefung 03.10.).
        std::function<void()> nachDerSchleife;
        for (std::size_t i = 0; i < g_app->rows.size(); ++i) {
            const Row& r = g_app->rows[i];
            ImGui::PushID(static_cast<int>(i));
            // --- Gleiche Befehle, schwach hinterlegt ---------------------
            //
            // shank: "if I click on a command, it faintly highlights all
            // commands that are the same". Dieselbe Farbe wie die Auswahl,
            // nur blass - so gehoert es erkennbar dazu, ohne wie ausgewaehlt
            // auszusehen.
            if (vorbild != nullptr && r.path != g_app->selectedPath && !isSelected(r.path)) {
                const Node* dieser = nodeAt(g_app->doc.script(), r.path);
                if (dieser != nullptr && gleicherBefehl(*vorbild, *dieser)) {
                    const ImVec2 o = ImGui::GetCursorScreenPos();
                    const float h2 = ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y;
                    ImVec4 farbe = ImGui::GetStyleColorVec4(ImGuiCol_Header);
                    // Blass - "I want it to be a subtle highlight" (vorher 0,35).
                    farbe.w *= 0.2F;
                    // Ab derselben Kante wie die Auswahl (das Selectable am
                    // Namen), NICHT unter Linien und Symbol: "The highlight
                    // for exact commands goes behind the icon".
                    const float abX = baseX + tm.indent * static_cast<float>(r.depth) +
                                      std::floor(tm.box * 0.5F) + tm.nameAt -
                                      ImGui::GetStyle().ItemSpacing.x * 0.5F;
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2{abX, o.y - ImGui::GetStyle().ItemSpacing.y * 0.5F},
                        ImVec2{ImGui::GetWindowPos().x + ImGui::GetWindowWidth(),
                               o.y + h2 - ImGui::GetStyle().ItemSpacing.y * 0.5F},
                        ImGui::GetColorU32(farbe));
                    ++g_app->gleicheSichtbar;
                }
            }
            const float step = tm.indent;
            // Oberkante der Zeile. Die Linien brauchen die GANZE Zeilenhoehe,
            // nicht die des Kaestchens - sonst klaffen sie zwischen den
            // Zeilen auseinander.
            const float rowY = ImGui::GetCursorScreenPos().y;
            const float rowH = ImGui::GetFontSize();
            {
                // Rand: Lesezeichen und Aenderungsmarke; ein Klick hinein
                // setzt oder entfernt das Lesezeichen (Notepad++).
                const float schritt = rowH + ImGui::GetStyle().ItemSpacing.y;
                zeichneRinne(ImGui::GetWindowDrawList(), baseX - rinne, rinne, rowY, schritt,
                             i < g_app->zeilenMarke.size() ? g_app->zeilenMarke[i] : kMarkeKeine,
                             hatLesezeichen(g_app->activeTab, r.kennung));
                if (g_app->randMitte.size() != g_app->rows.size()) { g_app->randMitte.assign(g_app->rows.size(), ImVec2{}); }
                if (i < g_app->randMitte.size()) {
                    g_app->randMitte[i] = ImVec2{baseX - rinne * 0.5F, rowY + schritt * 0.5F};
                }
                const ImVec2 mp = ImGui::GetIO().MousePos;
                const bool imRand = ImGui::IsWindowHovered() && mp.x >= baseX - rinne &&
                                    mp.x < baseX && mp.y >= rowY && mp.y < rowY + schritt;
                if (imRand && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    lesezeichenUmschalten({r.path});
                }
                // Der Hinweis erst nach kurzem Verweilen - sofort waere er
                // bei jeder Mausbewegung ueber den Rand im Weg.
                static double imRandSeit = -1.0;
                static std::size_t imRandZeile = static_cast<std::size_t>(-1);
                if (imRand) {
                    if (imRandZeile != i) { imRandZeile = i; imRandSeit = ImGui::GetTime(); }
                    // Auf einer geaenderten Zeile: das Original zeigen
                    // (shank: "when you hover over the yellow or green
                    // rectangle on the side"). Schneller als der blosse
                    // Hinweis - dafuer zeigt man ja hin.
                    const std::uint8_t marke = i < g_app->zeilenMarke.size() ? g_app->zeilenMarke[i] : kMarkeKeine;
                    const double warten = (marke != kMarkeKeine) ? 0.3 : 0.6;
                    if (ImGui::GetTime() - imRandSeit > warten) {
                        ImGui::BeginTooltip();
                        if (marke != kMarkeKeine) {
                            bool neu = false;
                            const std::string original = originalZeile(r.path, &neu);
                            if (!original.empty()) {
                                originalVorschau(zuruecksetzbar({r.path}));
                                ImGui::Separator();
                            } else if (neu) {
                                ImGui::TextDisabled("%s", tr(Str::RevertPreviewNew));
                                ImGui::Separator();
                            }
                        }
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0F);
                        ImGui::TextUnformatted(tr(Str::HintGutter));
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                        g_app->randVorschauZeile = static_cast<int>(i);
                    }
                } else if (imRandZeile == i) {
                    imRandZeile = static_cast<std::size_t>(-1);
                }
            }
            more.resize(static_cast<std::size_t>(r.depth) + 1U, false);
            more[static_cast<std::size_t>(r.depth)] = !r.lastChild;
            if (r.depth != 0) {
                ImGui::Indent(step * static_cast<float>(r.depth));
            }
            // Das Kaestchen bekommt IMMER seine Spalte, auch wenn die Zeile
            // keine Kinder hat. Vorher fehlte der Platz bei blattlosen
            // Zeilen, und ihre Symbole standen weiter links als die der
            // Bloecke daneben - genau die Unordnung, die im Original nicht
            // da ist.
            bool rebuilt = false;
            // Volle Zeilenhoehe, nicht nur die des Quadrats: sonst sitzt
            // die Klickflaeche oben und das gezeichnete Kaestchen in der
            // Mitte, und man trifft es nicht.
            if (r.childCount != 0) {
                if (ImGui::InvisibleButton("##auf", ImVec2{tm.box, rowH})) {
                    g_app->expanded.toggle(r.kennung);
                    rebuilt = true;
                }
            } else {
                ImGui::Dummy(ImVec2{tm.box, rowH});
            }
            // iconAt und nameAt sind ab SPALTENMITTE gemessen (so steht es
            // im Bildschirmfoto), das Kaestchen beginnt aber an der linken
            // Kante - also die halbe Kaestchenbreite dazu. Ohne das steht
            // alles vier Punkte zu weit links; eine Probe faengt es.
            ImGui::SameLine(0, tm.iconAt + std::floor(tm.box * 0.5F) - tm.box);
            if (!rebuilt) {
                const bool sel = isSelected(r.path);
                // Nach einem Tastendruck die Zeile in den sichtbaren
                // Bereich holen - sonst wandert die Auswahl unsichtbar aus
                // dem Fenster.
                if (sel && g_app->scrollToSelected &&
                    static_cast<int>(i) == g_app->selected) {
                    ImGui::SetScrollHereY(0.5F);
                    g_app->scrollToSelected = false;
                }
                drawIcon(r.icon.c_str(), sel);
                ImGui::SameLine(0, tm.nameAt - tm.iconAt - ImGui::GetFontSize());
                // Beschriftet wird nur der NAME. Die Argumente kommen
                // gleich danach in ihrer eigenen Spalte auf denselben
                // Balken gezeichnet - das Selectable reicht ohnehin bis zum
                // rechten Rand, also deckt die Hervorhebung beides ab.
                // Ist die Spalte so schmal, dass Text ohnehin nur noch
                // abgeschnitten dastuende? Dann NUR das Symbol.
                //
                // Die Grenze richtet sich an der Schrift aus, nicht an
                // einer festen Zahl - bei 144 dpi ist alles breiter. Der
                // volle Text kommt beim Ueberfahren, es geht also nichts
                // verloren.
                const bool nurSymbole =
                    ImGui::GetContentRegionAvail().x < ImGui::GetFontSize() * 8.0F;
                std::string schmal;
                if (nurSymbole) {
                    // Ein Selectable BRAUCHT eine Kennung, auch ohne Text -
                    // sonst fallen alle Zeilen auf dieselbe zusammen und
                    // die Auswahl springt. Deshalb die Zeilennummer hinter
                    // "##", also unsichtbar.
                    schmal = "##schmal" + std::to_string(i);
                }
                const char* label =
                    nurSymbole ? schmal.c_str()
                               : (r.name.empty() ? r.text.c_str()
                                                 : r.name.c_str());
                const KeepSelected keep(sel, g_app->selectionOwner != 1);
                // In der Symbolansicht steht der volle Text beim
                // Ueberfahren. Ohne ihn waere die schmale Spalte ein
                // Ratespiel - so ist sie eine Uebersicht, aus der man
                // jederzeit nachsehen kann.
                const bool zeigeHinweis = nurSymbole;
                const bool zeileGeklickt = ImGui::Selectable(
                    label, sel, ImGuiSelectableFlags_AllowDoubleClick);

                // Das Ziehen im Hauptbaum steht weiter unten - es gab es
                // hier schon (zum Umordnen), und ZWEI Quellen am selben
                // Element schliessen einander aus: ImGui nimmt die erste,
                // die zweite bekommt nie eine Chance. Genau daran ist das
                // Ziehen zwischen den Feldern in rc224 gescheitert.
                if (zeileGeklickt) {
                    // Dieselbe Zeile nochmal: Auswahl aufheben.
                    //
                    // "wenn ich ... eines gehighlighted habe also blau ist
                    // und ich es nochmal anklicke soll es nicht mehr
                    // ausgewaehlt sein". Ohne Strg und ohne Umschalt, und
                    // nur, wenn genau DIESE eine Zeile gewaehlt ist -
                    // sonst nimmt man einer Mehrfachauswahl mit einem
                    // Klick alles weg, was man gerade zusammengesucht hat.
                    const bool nurDiese =
                        sel && g_app->selection.size() == 1U &&
                        !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift &&
                        !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                    //
                    // KEIN continue und kein return hier: die Schleife hat
                    // ein PushID und Einrueckungen offen, und ein Sprung
                    // heraus laesst sie stehen. Genau dieses Muster
                    // ("SetCursorPosY + fruehes Verlassen") hat in dieser
                    // Anwendung schon zweimal zum Absturz gefuehrt.
                    if (nurDiese) {
                        clearSelection();
                        g_app->mapDirty = true;
                    } else {
                        selectRow(i, ImGui::GetIO().KeyCtrl,
                                  ImGui::GetIO().KeyShift);
                        g_app->mapDirty = true;  // gewaehlte Kamera zeigen
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                            // Doppelklick oeffnet den EDITOR - bei jedem
                            // Befehl, auch bei einer Kamerazeile.
                            //
                            // Vorher sprang er bei Kamerabefehlen in die
                            // Kartenansicht. Gemeldet: "wenn ich in Events
                            // doppelklick auf einen camera trigger mache,
                            // springt er zur Map statt diese zu oeffnen."
                            //
                            // Zu Recht. Ein Doppelklick heisst ueberall
                            // "aufmachen"; dass ausgerechnet ein Befehl
                            // etwas anderes tut, muss man wissen. Zur
                            // Kamera kommt man weiterhin ueber die
                            // Zeitleiste und ueber die Schluessel in der
                            // Ansicht - dort ist es ein Klick, kein
                            // Ratespiel.
                            nachDerSchleife = [p = r.path] { openEditorForNode(p); };
                        }
                    }
                }
                const ImVec2 nameMin = ImGui::GetItemRectMin();

                // --- Ziehen zum Umordnen ---------------------------------
                //
                // ImGui bringt das mit; wir muessen nur den Weg als Nutzlast
                // mitgeben und beim Ablegen verschieben.
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
                    // Zieht man an einer Zeile, die zur Auswahl gehoert,
                    // wandert die GANZE Auswahl mit. Sonst nur diese eine -
                    // und die wird dann auch zur neuen Auswahl.
                    if (!isSelected(r.path)) {
                        g_app->selection.assign(1, r.path);
                        g_app->selectedPath = r.path;
                    }
                    g_app->dragFrom = r.path;
                    g_app->dragFromTab = g_app->activeTab;
                    g_app->dragFromPane = -1;   // aus dem Hauptbaum
                    g_app->dragging = true;
                    // Das AUFNEHMEN protokollieren, nicht nur das Ablegen.
                    //
                    // Bisher stand nur da, was abgelegt wurde. Ein Zug, der
                    // aufgenommen und dann fallengelassen wurde, hinterliess
                    // nichts - und shanks Protokoll zeigte deshalb nur
                    // Zuege von Tiefe 1 nach Tiefe 1. Ob er ueberhaupt je
                    // ein Kind eines Blocks aufgenommen hat, war daraus
                    // nicht zu erkennen.
                    //
                    // Nur bei WECHSEL, sonst stuende die Zeile in jedem
                    // Bild des Ziehens.
                    // Der Merker verhindert, dass die Zeile in JEDEM Bild
                    // des Ziehens steht. Er wird am Ende jedes Zuges
                    // geloescht - sonst schweigt das nAECHSTE Aufnehmen,
                    // wenn es zufaellig vom selben Weg startet.
                    //
                    // shanks rc533-Protokoll, Stapel 15:
                    //   Ablegen: Ziel "use" ...
                    //   Baum: "move" gezogen, 2 -> 3
                    // Kein `Aufnehmen` davor. Vorher war "use" von Weg 2
                    // aufgenommen worden; nach dem Tausch lag "move" auf
                    // demselben Weg 2, und der Merker schwieg.
                    if (g_app->dbgZiehWeg != r.path) {
                        g_app->dbgZiehWeg = r.path;
                        char zz[200];
                        std::snprintf(zz, sizeof(zz),
                                      "Aufnehmen: \"%s\" von Weg %s, Tiefe %d",
                                      r.name.c_str(),
                                      wegAlsText(r.path).c_str(),
                                      static_cast<int>(r.path.size()));
                        diag::detail(zz);
                    }
                    const std::size_t n = r.path.size();
                    ImGui::SetDragDropPayload("bhed_node", &n, sizeof(n));
                    const std::size_t count = g_app->selection.size();
                    if (count > 1) {
                        ImGui::Text(tr(Str::MsgSelected), static_cast<int>(count));
                    } else {
                        ImGui::TextUnformatted(r.text.c_str());
                    }
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    // --- Neues aus der Ereignisliste ---------------------
                    //
                    // AcceptPeekOnly ist AcceptBeforeDelivery zusammen mit
                    // AcceptNoDrawDefaultRect (imgui.h). Damit sieht man die
                    // Nutzlast SCHON WAEHREND des Ziehens - nur so laesst
                    // sich die Einfuegelinie zeichnen, bevor die Maustaste
                    // losgeht. Das Rechteck von ImGui waere hier falsch: es
                    // umrahmt die ganze Zeile und sagt "hinein", wo wir
                    // "dazwischen" meinen.
                    //
                    // Erst IsDelivery() bedeutet wirklich abgelegt.
                    if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload(
                            "bhed_new", ImGuiDragDropFlags_AcceptPeekOnly)) {
                        const ImVec2 a = ImGui::GetItemRectMin();
                        const ImVec2 b = ImGui::GetItemRectMax();
                        const float h = std::max(b.y - a.y, 1.0F);
                        const float t = (ImGui::GetIO().MousePos.y - a.y) / h;

                        // Drei Zonen - siehe ablageZone. Eine Makrozeile ist ein
                        // Behaelter wie ein Block (im Original: echte Kinder).
                        const Node* drop = nodeAt(g_app->doc.script(), r.path);
                        const bool makroZeile = drop != nullptr && drop->kind == Node::Kind::Macro;
                        const bool block = drop != nullptr && (drop->hasBlock || makroZeile);
                        const bool mitInhalt = drop != nullptr && (makroZeile ? drop->count > 0 : !drop->children.empty());
                        const Ablage zone = ablageZone(
                            t, block, block && mitInhalt && g_app->expanded.isOpen(r.kennung));
                        const DropWhere where = zone.wo;
                        zeichneAblage(zone, a, b);

                        if (pl->IsDelivery() && pl->DataSize == sizeof(NewDrag)) {
                            NewDrag what{};
                            std::memcpy(&what, pl->Data, sizeof(what));
                            dropNewAt(what, r.path, where);
                            rebuilt = true;
                        }
                    }
                    // Aus der RECHTEN Haelfte herueber.
                    //
                    // Dasselbe Ablegen wie bei einem eigenen Knoten, nur
                    // kommt der Knoten aus einem anderen Dokument. Er wird
                    // KOPIERT, nicht verschoben - das rechte Dokument
                    // bleibt unberuehrt, und das ist richtig so: rechts
                    // wird nicht bearbeitet.
                    if (const ImGuiPayload* nutzlast =
                            ImGui::AcceptDragDropPayload("bhed_split")) {
                        if (const Node* nd = knotenAusNebenfeld(nutzlast)) {
                            const Node kopie = *nd;
                            if (g_app->doc.insertAfter(r.path, kopie)) {
                                rebuilt = true;
                                setStatus(tr(Str::MsgCopied), 1, 0);
                            }
                        }
                    }
                    // --- Umordnen im Baum: dieselben drei Zonen ---------
                    //
                    // Vorher: ImGuis Rahmen um die ganze Zeile (auch bei
                    // einem set, wo es kein Hinein gibt), und wohin es fiel,
                    // entschied eine versteckte Regel - Block auf und leer
                    // oder offen: hinein, sonst immer DAHINTER. Jetzt zeigt
                    // die Linie oder der Rahmen, was geschieht, und genau das
                    // geschieht (Document::moveAllAt).
                    if (const ImGuiPayload* knotenPl = ImGui::AcceptDragDropPayload(
                            "bhed_node", ImGuiDragDropFlags_AcceptPeekOnly)) {
                        const ImVec2 a = ImGui::GetItemRectMin();
                        const ImVec2 b = ImGui::GetItemRectMax();
                        const float h = std::max(b.y - a.y, 1.0F);
                        const float anteil = (ImGui::GetIO().MousePos.y - a.y) / h;
                        const Node* drop = nodeAt(g_app->doc.script(), r.path);
                        const bool makroZeile = drop != nullptr && drop->kind == Node::Kind::Macro;
                        const bool block = drop != nullptr && (drop->hasBlock || makroZeile);
                        const bool mitInhalt = drop != nullptr && (makroZeile ? drop->count > 0 : !drop->children.empty());
                        const Ablage zone = ablageZone(
                            anteil, block, block && mitInhalt && g_app->expanded.isOpen(r.kennung));
                        const bool ausFremdemSkript =
                            g_app->dragFromTab >= 0 &&
                            g_app->dragFromTab != g_app->activeTab;
                        if (g_app->dragging) {
                            zeichneAblage(zone, a, b);
                        }
                        if (knotenPl->IsDelivery() && g_app->dragging) {
                        // Kommt der Knoten aus einem ANDEREN Skript, wird
                        // er kopiert statt verschoben - verschieben wuerde
                        // ihn drueben loeschen, und das ist beim Ziehen
                        // zwischen zwei Dateien nie gemeint.
                        //
                        // KEIN continue hier: in einer ImGui-Schleife ist ein
                        // frueher Ausstieg gefaehrlich - eine offene PushID
                        // bliebe stehen. Also if/else.
                        if (ausFremdemSkript) {
                            App::DragNode dn;
                            dn.pane = g_app->dragFromPane;
                            dn.tab = g_app->dragFromTab;
                            dn.row = -1;
                            nachDerSchleife = [dn, ziel = static_cast<int>(i)] {
                                knotenZiehen(dn, -1, ziel);
                                g_app->dragging = false;
                                g_app->dbgZiehWeg.clear();
                                g_app->dragFrom.clear();
                                g_app->dragFromTab = -1;
                            };
                        } else {
                            const std::vector<Path> m = selectionOrCurrent();
                            const std::vector<Path> ziehen =
                                m.size() > 1 ? m : std::vector<Path>{g_app->dragFrom};
                            const std::vector<Kennung> bewegt = kennungenVon(ziehen);
                            const Document::Stelle stelle =
                                zone.wo == DropWhere::Before ? Document::Stelle::Davor
                                : zone.wo == DropWhere::Into ? Document::Stelle::Hinein
                                                             : Document::Stelle::Dahinter;
                            {
                                char zd[240];
                                std::snprintf(zd, sizeof(zd),
                                              "Ablegen: Ziel \"%s\"%s, %d Zeile(n) -> %s",
                                              drop != nullptr ? drop->name.c_str() : "(kein)",
                                              block ? ", ist Block" : "",
                                              static_cast<int>(ziehen.size()),
                                              stelle == Document::Stelle::Davor ? "DAVOR"
                                              : stelle == Document::Stelle::Hinein ? "HINEIN"
                                                                                  : "DAHINTER");
                                diag::detail(zd);
                            }
                            Path gelandet;
                            const bool ok = g_app->doc.moveAllAt(ziehen, r.path, stelle, &gelandet);
                            if (ok) {
                                if (stelle == Document::Stelle::Hinein) {
                                    Path blockWeg = gelandet;
                                    if (makroZeile) {
                                        // flach: das Makro steht direkt davor
                                        --blockWeg.back();
                                    } else {
                                        blockWeg.pop_back();
                                    }
                                    g_app->expanded.setOpen(kennungFuer(blockWeg), true);
                                }
                                markiereKennungen(bewegt);
                                rebuilt = true;
                            } else {
                                zeigeAbweisung();
                            }
                            g_app->dragging = false;
                            g_app->dbgZiehWeg.clear();
                        }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // --- Zweite Spalte: die Argumente ------------------------
                //
                // Gezeichnet statt gesetzt, damit sie ohne SetCursorPos
                // auskommen - der Auswahlbalken reicht ohnehin bis zum
                // rechten Rand und deckt sie mit ab.
                //
                // Nachgestellt wird die Regel des Originals: die Klammer
                // beginnt immer nameW hinter dem Namensanfang. Ist ein Name
                // laenger als die Spalte, rueckt sie fuer diese eine Zeile
                // nach rechts, statt sich zu ueberschreiben.
                // In der Symbolansicht gar keine Argumente: sie stuenden
                // ueber den Rand hinaus, und beschnitten waeren sie nur ein
                // Streifen Buchstabenreste.
                if (zeigeHinweis && ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(r.text.c_str());
                    ImGui::EndTooltip();
                }
                if (!r.args.empty() && !nurSymbole) {
                    const float argX = std::max(
                        nameMin.x + tm.nameW,
                        nameMin.x + ImGui::CalcTextSize(label).x +
                            ImGui::GetStyle().ItemInnerSpacing.x);
                    ImGui::GetWindowDrawList()->AddText(
                        ImVec2{argX, nameMin.y},
                        ImGui::GetColorU32(ImGuiCol_Text), r.args.c_str());
                }
            }

            // --- Die gepunkteten Linien ------------------------------------
            //
            // Nach den Elementen gezeichnet, aber unter ihnen sichtbar: sie
            // liegen links vom Text, wo nichts anderes steht.
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                // Von Oberkante bis Oberkante der naechsten Zeile, damit die
                // Senkrechten ohne Luecke ineinander uebergehen.
                const float top = rowY;
                const float bottom = rowY + rowH + ImGui::GetStyle().ItemSpacing.y;
                const float mid = std::floor(rowY + rowH * 0.5F);
                const float half = std::floor(tm.box * 0.5F);
                // Senkrechte je Ebene, die weiterlaeuft.
                for (int k = 0; k < r.depth; ++k) {
                    if (!more[static_cast<std::size_t>(k)]) {
                        continue;
                    }
                    const float x = std::floor(baseX + step * static_cast<float>(k) + half);
                    dottedLine(dl, x, top, x, bottom, lineCol);
                }
                // Die eigene Spalte: von oben bis zur Mitte immer, darunter
                // nur, wenn noch ein Geschwister folgt. Genau so hoert die
                // Linie im Original beim letzten Kind auf halber Hoehe auf.
                const float x = std::floor(baseX + step * static_cast<float>(r.depth) + half);
                dottedLine(dl, x, top, x, mid, lineCol);
                if (!r.lastChild) {
                    dottedLine(dl, x, mid, x, bottom, lineCol);
                }
                // Waagerechter Stummel zum Symbol.
                dottedLine(dl, x + (r.childCount != 0 ? half + 1.0F : 1.0F), mid,
                           x + tm.iconAt, mid, lineCol);
                if (r.childCount != 0) {
                    // Ein MAKRO bekommt eine blassere Marke als ein Block.
                    //
                    // Bis hierher sahen beide gleich aus, und shank hat
                    // dreimal versucht, einen Befehl aus `standOnly`
                    // herauszuziehen. Er hat sich nicht geirrt - die
                    // Anzeige hat ihn in die Irre gefuehrt. Ein Makro ist
                    // kein Block: seine Zeilen stehen DANEBEN, nicht darin.
                    const ImU32 markeCol =
                        (r.what == Row::What::Macro)
                            ? ImGui::GetColorU32(ImGuiCol_TextDisabled)
                            : ImGui::GetColorU32(ImGuiCol_Text);
                    expanderBox(dl, ImVec2{x, mid}, tm.box, r.open, lineCol,
                                markeCol);
                }
            }
            if (r.depth != 0) {
                ImGui::Unindent(step * static_cast<float>(r.depth));
            }
            // Rechtsklick auf eine Zeile: dasselbe wie die Knoepfe rechts,
            // nur dort, wo die Hand gerade ist.
            // Auf einer Makrozeile sagen, was Sache ist.
            //
            // Die blassere Marke faellt nur auf, wenn man beide nebeneinander
            // sieht. Der Satz sagt es geradeheraus - genau dann, wenn jemand
            // ueberlegt, diese Zeile irgendwohin zu ziehen.
            if ((r.ausMakro || r.what == Row::What::Macro) &&
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                ImGui::SetTooltip("%s", tr(Str::MacroRowHint));
            }
            if (ImGui::BeginPopupContextItem("zeile")) {
                // Der Rechtsklick WAEHLT die Zeile auch - wie der Linksklick.
                //
                // Bisher wurden nur `selected` und `selectedPath` gesetzt.
                // Der Balken haengt aber an `g_app->selection` (siehe
                // isSelected), und die blieb unberuehrt: das Menue ging auf,
                // markiert war weiter die alte Zeile.
                //
                // shank zu rc542: "I think right-clicking on a command
                // should select/highlight it (like left-clicking does)."
                //
                // Steht die Zeile schon in einer Mehrfachauswahl, bleibt
                // diese, wie sie ist - sonst zerschiesse ich genau die
                // Auswahl, auf die sich der Menuepunkt beziehen soll.
                if (!isSelected(r.path)) {
                    selectRow(i, false, false);
                } else {
                    g_app->selected = static_cast<int>(i);
                    g_app->selectedPath = r.path;
                }
                if (ImGui::MenuItem(tr(Str::ActEdit), chordName(keys::Action::EditItem))) {
                    nachDerSchleife = [p = r.path] { openEditorForNode(p); };
                }
                // --- Auf das Original zuruecksetzen ----------------------
                //
                // shank: "What if there is an option when I right-click a
                // command I changed to revert it to what it was when I
                // opened the file? Sort of like an undo, but for a specific
                // command." Ein Rueckgaengig-Schritt; der Rest des Skripts
                // bleibt, wie er ist.
                {
                    const std::vector<std::pair<Path, Node>> zurueck = zuruecksetzbar(selectionOrCurrent());
                    // Die Vorschau grau rechts daneben, in EINER Zeile - im
                    // Feld, in dem sonst das Tastenkuerzel steht (shank:
                    // "have grey text next to revert to original").
                    const std::string kurz = originalKurz(zurueck);
                    if (!kurz.empty()) {
                        ++g_app->vorschauGezeichnet;
                    }
                    if (ImGui::MenuItem(tr(Str::ActRevertOriginal), kurz.empty() ? nullptr : kurz.c_str(), false,
                                        !zurueck.empty())) {
                        if (g_app->doc.replaceMany(zurueck)) {
                            rebuilt = true;
                            char z[160];
                            std::snprintf(z, sizeof(z), tr(Str::MsgRevertedN), static_cast<int>(zurueck.size()));
                            addStatus(z);
                        }
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("%s", tr(Str::ActRevertOriginalHint));
                    }
                }
                if (ImGui::MenuItem(tr(Str::ActBookmarkToggle),
                                    chordName(keys::Action::BookmarkToggle))) {
                    lesezeichenUmschalten(selectionOrCurrent());
                }
                // --- Aehnliche suchen ------------------------------------
                //
                // shank: "When I right click a command, have an option to
                // find commands. Basically an auto fill for the find window,
                // rather than having to edit the command, copy its values,
                // and then paste it into the find window."
                //
                // Die Suche kennt EIN "containing" - also je Wert ein
                // Eintrag, dazu "jeder <befehl>" nur nach dem Namen.
                if (const Node* fn = nodeAt(g_app->doc.script(), r.path);
                    fn != nullptr && fn->kind == Node::Kind::Command) {
                    if (ImGui::BeginMenu(tr(Str::ActFindSimilar))) {
                        char jeder[160];
                        std::snprintf(jeder, sizeof(jeder), tr(Str::FindSimilarAny),
                                      fn->name.c_str());
                        if (ImGui::MenuItem(jeder)) {
                            sucheVorbelegen(fn->name, "");
                        }
                        ImGui::Separator();
                        for (std::size_t ai = 0; ai < fn->args.size(); ++ai) {
                            const Arg& a = fn->args[ai];
                            if (a.text.empty()) {
                                continue;
                            }
                            std::string zeile = a.text.size() > 60
                                                    ? a.text.substr(0, 57) + "..."
                                                    : a.text;
                            zeile += "##aehnlich" + std::to_string(ai);
                            if (ImGui::MenuItem(zeile.c_str())) {
                                sucheVorbelegen(fn->name, a.text);
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(tr(Str::ActClone), chordName(keys::Action::Clone))) {
                    const std::vector<Path> mc = selectionOrCurrent();
                    const bool ok = mc.size() > 1 ? g_app->doc.cloneAll(mc)
                                                  : g_app->doc.cloneAt(r.path);
                    if (ok) { rebuilt = true; }
                }
                // Gehoert die angeklickte Zeile zur Auswahl, gilt es fuer die
                // GANZE Auswahl (rc599: "if I right click and select copy, it
                // only copies one command. It should copy everything
                // highlighted.") - wie Strg+C und der Knopf rechts.
                const std::vector<Path> markiert = selectionOrCurrent();
                const bool ganzeAuswahl =
                    markiert.size() > 1 && std::find(markiert.begin(), markiert.end(), r.path) != markiert.end();
                if (ImGui::MenuItem(tr(Str::ActCopy), chordName(keys::Action::Copy))) {
                    if (ganzeAuswahl) { kopieren(false); } else { kopiereWeg(r.path, false); }
                }
                if (ImGui::MenuItem(tr(Str::ActCut), chordName(keys::Action::Cut))) {
                    if (ganzeAuswahl) { kopieren(true); } else { kopiereWeg(r.path, true); }
                    rebuilt = true;
                }
                // Immer anklickbar: der Inhalt kann auch aus der
                // Windows-Zwischenablage kommen.
                if (ImGui::MenuItem(tr(Str::ActPaste), chordName(keys::Action::Paste))) {
                    einfuegen(r.path);
                    rebuilt = true;
                }
                ImGui::Separator();
                {
                    std::vector<Path> m = selectionOrCurrent();
                    // Auch hier den Aufklapp-Zustand nachfuehren.
                    //
                    // Der Knopf-Weg tut das seit rc530, dieses Menue nicht -
                    // dieselbe Geste, zwei Wege, einer davon vergessen. Das
                    // ist genau shanks urspruengliche Meldung ("wenn ich
                    // move up mache, geht das + zu"), nur ueber den
                    // Rechtsklick statt ueber den Knopf.
                    if (ImGui::MenuItem(tr(Str::MoveUp), chordName(keys::Action::MoveUp))) {
                        Path nachher;
                        const bool ok = m.size() > 1
                                            ? g_app->doc.moveAll(m, true)
                                            : g_app->doc.moveUp(r.path, &nachher);
                        if (ok) {
                            if (m.size() > 1) { g_app->selection = m; }
                            rebuilt = true;
                        } else {
                            zeigeAbweisung();
                        }
                    }
                    if (ImGui::MenuItem(tr(Str::MoveDown), chordName(keys::Action::MoveDown))) {
                        std::vector<Path> m2 = selectionOrCurrent();
                        Path nachher2;
                        const bool ok = m2.size() > 1
                                            ? g_app->doc.moveAll(m2, false)
                                            : g_app->doc.moveDown(r.path, &nachher2);
                        if (ok) {
                            if (m2.size() > 1) { g_app->selection = m2; }
                            rebuilt = true;
                        } else {
                            zeigeAbweisung();
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(tr(Str::ActRem), chordName(keys::Action::CommentOut))) {
                    if (remUmschalten(r.path)) { rebuilt = true; }
                }
                if (ImGui::MenuItem(tr(Str::ActDelete), chordName(keys::Action::Delete))) {
                    if (g_app->doc.removeAt(r.path)) { rebuilt = true; }
                }
                // Neue Befehle einfuegen, ohne nach links zu greifen.
                ImGui::Separator();
                // Bei einem Block sagen, wohin der neue Befehl geht - sonst
                // probiert man es aus und raet.
                {
                    const Node* nn2 = nodeAt(g_app->doc.script(), r.path);
                    if (nn2 != nullptr && nn2->hasBlock) {
                        const bool into = g_app->expanded.isOpen(r.kennung) ||
                                          nn2->children.empty();
                        ImGui::TextDisabled("%s", into ? tr(Str::InsertInto)
                                                       : tr(Str::InsertAfter));
                    }
                }
                if (ImGui::BeginMenu(tr(Str::GroupEvents))) {
                    for (const Command& c : g_app->db.commands) {
                        if (ImGui::MenuItem((c.name + "  " + signatureText(c)).c_str())) {
                            insertCommand(c);
                            rebuilt = true;
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(tr(Str::MsgMacros))) {
                    for (const Macro& mm : g_app->db.macroBodies) {
                        if (ImGui::MenuItem(mm.name.c_str())) {
                            insertMacro(mm);
                            rebuilt = true;
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
            if (rebuilt) {
                rebuildTree();
                // Die restlichen Zeilen als LEERE Hoehe stehen lassen.
                //
                // shank, 27.09.: "Dragging a command scrolls the view up" -
                // in langen Listen sprang die Ansicht nach jedem Ablegen
                // nach oben. Hier stand nur `break`: die Liste endete in
                // diesem Bild an der abgelegten Zeile, ImGui nahm die
                // kuerzere Inhaltshoehe als neuen Rollbereich und klemmte
                // die Rollposition darauf. Bei kurzen Listen ohne Rollen
                // fiel es nicht auf.
                //
                // Die neue Liste wird erst im naechsten Bild gezeichnet
                // (die Wege von `r` gelten nicht mehr); bis dahin haelt ein
                // Platzhalter die volle Hoehe. Der Zeilenschritt ist
                // derselbe wie bei den Linien unten.
                const float schritt = rowH + ImGui::GetStyle().ItemSpacing.y;
                const float bisUnten =
                    rowY + schritt * static_cast<float>(g_app->rows.size() - i);
                ImGui::SetCursorScreenPos(ImVec2{baseX, bisUnten});
                ImGui::Dummy(ImVec2{0.0F, 0.0F});
                break;
            }
        }
        if (nachDerSchleife) {
            nachDerSchleife();
        }

        ImGui::Unindent(rinne);
        // Ein "zur Auswahl rollen" ohne Auswahl verfaellt - sonst wartet es
        // auf den naechsten Klick und laesst die Ansicht dann springen.
        if (g_app->scrollToSelected && g_app->selected < 0) {
            g_app->scrollToSelected = false;
        }

        // --- Die freie Flaeche unter der letzten Zeile ---------------------
        //
        // Ohne die geht bei einem LEEREN Skript gar nichts: es gibt keine
        // Zeile, auf die man ziehen koennte, also kaeme nie ein erster
        // Befehl hinein. Derselbe Fall, der schon beim Doppelklick
        // zugeschlagen hat, nur eine Bedienung weiter.
        //
        // Hierhin gezogen heisst "ans Ende der obersten Ebene" - das ist die
        // Bedeutung, die insertAfter einem leeren Weg gibt.
        const ImVec2 rest = ImGui::GetContentRegionAvail();
        if (rest.y > 1.0F) {
            ImGui::InvisibleButton("##ende", ImVec2{std::max(rest.x, 1.0F), rest.y});
            // Ins Leere geklickt heisst: nichts mehr gewaehlt.
            //
            // So macht es der Explorer, und so erwartet es jeder. Vorher
            // blieb die Markierung ewig stehen - man wurde sie nur los,
            // indem man etwas anderes anklickte.
            //
            // Umschalt und Strg NICHT: wer damit klickt, will erweitern,
            // auch wenn er dabei danebentrifft.
            if (ImGui::IsItemClicked() && !ImGui::GetIO().KeyShift &&
                !ImGui::GetIO().KeyCtrl) {
                g_app->selection.clear();
                g_app->selectedPath.clear();
                g_app->selected = -1;
                g_app->mapDirty = true;
            }
            if (ImGui::BeginDragDropTarget()) {
                // --- Einen BESTEHENDEN Befehl hierher ziehen ------------
                //
                // shank: "das Rausziehen klappt nur, wenn ich es zu einem
                // anderen Befehl ziehe. Wenn ich es unterhalb in das leere
                // Fenster ziehe, springt es nicht raus. Muss es da ein
                // ziehbares Feld geben?"
                //
                // Ja - und die Flaeche gab es schon, sie nahm aber nur
                // `bhed_new` an: neue Befehle aus der Ereignisliste. Fuer
                // `bhed_node`, also einen Befehl AUS DEM BAUM, war keine
                // Annahme da. Der Zug verfiel lautlos.
                //
                // Hier bedeutet Ablegen: ans ENDE der obersten Ebene. Damit
                // gibt es endlich eine Stelle, die "heraus aus allem"
                // heisst - ohne dass man einen passenden Nachbarn suchen
                // muss.
                if (ImGui::AcceptDragDropPayload("bhed_node") != nullptr &&
                    g_app->dragging && !g_app->dragFrom.empty()) {
                    const Path zuvor = g_app->dragFrom;
                    char zf[160];
                    std::snprintf(zf, sizeof(zf),
                                  "Ablegen: freie Flaeche -> ans Ende der "
                                  "obersten Ebene (von %s, Tiefe %d)",
                                  wegAlsText(zuvor).c_str(),
                                  static_cast<int>(zuvor.size()));
                    diag::detail(zf);
                    // Aus einem Block: NEBEN den Block, nicht ans Ende
                    // von allem.
                    //
                    // shank: "er laesst mich es zwar nach unten ziehen,
                    // dafuer setzt er es in dem Plus einfach nur ganz nach
                    // unten statt raus neben der Funktion mit dem Plus."
                    //
                    // Richtig. "Ans Ende der obersten Ebene" ist ein grobes
                    // Werkzeug: wer einen Befehl aus einem `affect`
                    // herauszieht, will ihn direkt dahinter haben, nicht
                    // dreissig Zeilen weiter unten. Er muesste ihn danach
                    // von Hand wieder hochschieben.
                    //
                    // Steckt er in einem Block, ist das Ziel also der Platz
                    // HINTER dem Block - dieselbe Bewegung wie bei
                    // `Move down` am Blockende (rc516) und beim Ablegen auf
                    // dem Block selbst (rc521). Drei Gesten, ein Ergebnis.
                    //
                    // Nur wer schon oben steht, wandert ans Ende: dort gibt
                    // es keinen Block, aus dem er heraus koennte.
                    bool ok2 = false;
                    Path zielW;
                    // `zielW` ist der ANKER, nicht das Ziel: moveTo setzt
                    // den Knoten NEBEN ihn. Wohin er wirklich kommt, sagt
                    // jetzt `gelandet` - siehe edit.h.
                    Path gelandet;
                    if (zuvor.size() > 1) {
                        zielW.assign(zuvor.begin(), zuvor.end() - 1);
                        ok2 = g_app->doc.moveTo(zuvor, zielW, &gelandet);
                    } else {
                        ok2 = g_app->doc.moveToEnd(zuvor, &gelandet);
                    }
                    if (ok2) {
                        g_app->selection.clear();
                        g_app->selectedPath.clear();
                        rebuildTree();
                    } else {
                        zeigeAbweisung();
                    }
                    g_app->dragging = false;
                    g_app->dbgZiehWeg.clear();
                    g_app->dragFrom.clear();
                    g_app->dragFromTab = -1;
                }
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload(
                        "bhed_new", ImGuiDragDropFlags_AcceptPeekOnly)) {
                    const ImVec2 a = ImGui::GetItemRectMin();
                    const ImVec2 b = ImGui::GetItemRectMax();
                    ImGui::GetWindowDrawList()->AddLine(
                        ImVec2{a.x, a.y}, ImVec2{b.x, a.y},
                        ImGui::GetColorU32(ImGuiCol_DragDropTarget), 2.0F);
                    if (pl->IsDelivery() && pl->DataSize == sizeof(NewDrag)) {
                        NewDrag what{};
                        std::memcpy(&what, pl->Data, sizeof(what));
                        dropNewAt(what, Path{}, DropWhere::After);
                        rebuildTree();
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }
    }
    // Ein Zug, der ABGEBROCHEN wurde - aufgenommen, aber nirgends
    // abgelegt -, muss den Merker genauso aufraeumen wie ein abgelegter.
    //
    // shanks rc537-Protokoll:
    //
    //     Aufnehmen: "standOnly" von Weg 5, Tiefe 1
    //     Baum: "print" nach OBEN, 9 -> 8        <- kein Ablegen
    //
    // Er hat losgezogen und dann losgelassen. Bisher wurde `dbgZiehWeg`
    // nur an den drei ABLEGE-Stellen geloescht (rc534). Bleibt er stehen,
    // schweigt das naechste `Aufnehmen`, wenn es zufaellig vom selben Weg
    // startet - genau das Loch, das rc534 schliessen sollte, einen Fall
    // zu kurz.
    if (g_app->dragging && !ImGui::IsDragDropActive()) {
        g_app->dragging = false;
        g_app->dbgZiehWeg.clear();
        g_app->dragFrom.clear();
        g_app->dragFromTab = -1;
    }
    rollpositionMerken(g_app->activeTab);
    endGroupBox();
    ablageFlaeche(-1);
}

// Ein Knopf ueber die volle Spaltenbreite, Hoehe aus der Schrift.
// Eine Kurzhilfe an der Maus, nach kurzem Verweilen.
//
// ImGuiHoveredFlags_DelayNormal wartet, statt sofort aufzuploppen - sonst
// flackert beim Ueberfahren der Knopfspalte staendig ein Kasten.
void hint(Str id, keys::Action key = keys::Action::Unhandled) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal |
                              ImGuiHoveredFlags_AllowWhenDisabled)) {
        return;
    }
    // Das Kuerzel dazu, wenn es eines gibt. Gefragt wird die WIRKLICHE
    // Belegung, nicht eine fest getippte - wer sie im Einstellungsfenster
    // umlegt, sieht es hier sofort.
    const char* k = (key == keys::Action::Unhandled) ? nullptr : chordName(key);
    if (k != nullptr) {
        ImGui::SetTooltip("%s  (%s)", tr(id), k);
    } else {
        ImGui::SetTooltip("%s", tr(id));
    }
}

bool fullButton(Str id, Str hintId = Str::Count,
                keys::Action key = keys::Action::Unhandled) {
    const bool r = ImGui::Button(tr(id), ImVec2{-FLT_MIN, 0.0F});
    if (hintId != Str::Count) {
        hint(hintId, key);
    }
    return r;
}

// Zwei Knoepfe nebeneinander, wie im Original (Copy/Cut, Open/MRU, Prefs/About).
bool halfButton(Str id, bool second, Str hintId = Str::Count,
                keys::Action key = keys::Action::Unhandled) {
    const float w = (ImGui::GetContentRegionAvail().x -
                     ImGui::GetStyle().ItemSpacing.x) * 0.5F;
    const bool r = ImGui::Button(tr(id), ImVec2{second ? -FLT_MIN : w, 0.0F});
    if (hintId != Str::Count) {
        hint(hintId, key);
    }
    if (!second) {
        ImGui::SameLine();
    }
    return r;
}

// --- Der Compile-Knopf: gross, farbig, mit Rueckmeldung --------------------
//
// shank, 27.09.: "Maybe the compile button should be bigger (like it is in
// the original)? It is the most-used button" - im Original ist er 31 dlu
// hoch, die uebrigen 12 (Dialog 102). Und: nach dem Klick kurz "Compiled"
// mit Haken, danach zurueck, weil das Uebersetzen sofort fertig ist.
//
// Der Haken ist gezeichnet (RenderCheckMark), nicht als Zeichen gesetzt:
// die Oberflaechenschrift hat kein U+2713, und ein Fragezeichen-Kaestchen
// waere schlimmer als gar nichts.
//
// Die Beschriftung "Compile!" bleibt als Kennung sichtbar ("###compile"
// haelt die Kennung fest, wenn sie wechselt) - Selbsttest und Tastatur
// finden den Knopf weiterhin unter seinem Namen.
bool compileKnopf() {
    const double jetzt = ImGui::GetTime();
    const bool fertig = jetzt < g_app->kompiliertBis;
    const bool fehler = !fertig && jetzt < g_app->kompiliertFehlerBis;
    const float h = std::floor(ImGui::GetFrameHeight() * 2.2F);

    ImVec4 grund = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
    if (fertig) {
        grund = ImVec4{0.18F, 0.55F, 0.30F, 1.0F};
    } else if (fehler) {
        grund = ImVec4{0.70F, 0.20F, 0.20F, 1.0F};
    }
    auto heller = [](ImVec4 c, float f) {
        return ImVec4{c.x + (1.0F - c.x) * f, c.y + (1.0F - c.y) * f,
                      c.z + (1.0F - c.z) * f, c.w};
    };
    auto dunkler = [](ImVec4 c, float f) {
        return ImVec4{c.x * (1.0F - f), c.y * (1.0F - f), c.z * (1.0F - f), c.w};
    };
    ImGui::PushStyleColor(ImGuiCol_Button, grund);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, heller(grund, 0.15F));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, dunkler(grund, 0.15F));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 1.0F, 1.0F, 1.0F});
    const std::string beschriftung =
        std::string((fertig || fehler) ? "" : tr(Str::Compile)) + "###compile";
    const bool geklickt = ImGui::Button(beschriftung.c_str(), ImVec2{-FLT_MIN, h});
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    if (fertig || fehler) {
        // Haken bzw. Kreuz vor dem Text, beides zusammen mittig.
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const char* text = tr(fertig ? Str::CompileDone : Str::CompileFailedShort);
        const float tw = ImGui::CalcTextSize(text).x;
        const float zs = ImGui::GetFontSize() * 0.85F;
        const float luft = ImGui::GetStyle().ItemInnerSpacing.x * 1.5F;
        const float x0 = std::floor((a.x + b.x - (zs + luft + tw)) * 0.5F);
        const float ym = (a.y + b.y) * 0.5F;
        const ImU32 weiss = IM_COL32(255, 255, 255, 255);
        if (fertig) {
            ImGui::RenderCheckMark(dl, ImVec2{x0, ym - zs * 0.5F}, weiss, zs);
        } else {
            const float k = zs * 0.35F;
            const ImVec2 m{x0 + zs * 0.5F, ym};
            dl->AddLine(ImVec2{m.x - k, m.y - k}, ImVec2{m.x + k, m.y + k}, weiss, 2.0F);
            dl->AddLine(ImVec2{m.x - k, m.y + k}, ImVec2{m.x + k, m.y - k}, weiss, 2.0F);
        }
        dl->AddText(ImVec2{x0 + zs + luft, ym - ImGui::GetFontSize() * 0.5F}, weiss, text);
    }
    ImGui::PopStyleColor(4);
    hint(Str::HintCompile);
    return geklickt;
}

void drawButtonColumns(const Layout& l) {
    ImGui::BeginGroup();
    const float top = ImGui::GetCursorPosY();
    ImGui::TextUnformatted(tr(Str::GroupActions));
    // "N selected" steht in der UEBERSCHRIFT, rechtsbuendig.
    //
    // shank, 27.09.: "manchmal beim verschieben springt die action leiste"
    // - "ich glaube das springen war eher das ... selected angezeigt wird".
    // Bis rc568 stand die Zeile ZWISCHEN Copy/Cut und Paste; sobald mehr als
    // eine Zeile markiert war, rutschten alle Knoepfe darunter eine Zeile
    // nach unten, und beim Abwaehlen wieder hoch. Seit das Ziehen die
    // gezogenen Zeilen markiert, geschah das bei jedem Zug mit mehreren.
    // In der Kopfzeile ist Platz, und nichts verschiebt sich.
    {
        const std::size_t anzahl = selectionOrCurrent().size();
        if (anzahl > 1) {
            char zahl[64];
            std::snprintf(zahl, sizeof(zahl), tr(Str::MsgSelected), static_cast<int>(anzahl));
            const float rechts = ImGui::GetContentRegionAvail().x;
            ImGui::SameLine(std::max(ImGui::GetCursorPosX(),
                                     rechts - ImGui::CalcTextSize(zahl).x));
            ImGui::TextDisabled("%s", zahl);
        }
    }
    // Auf die gemeinsame Kopfzeilenhoehe auffuellen.
    ImGui::SetCursorPosY(top + l.headerH);
        // Ein leeres Element als Anker: ImGui verlangt nach SetCursorPos ein
        // abgeschicktes Element, sonst bricht es ab. Was danach kommt, kann
        // vorzeitig zurueckkehren - der Anker nicht.
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    // Breite 0: die Tabellenzelle gibt sie vor.
    ImGui::BeginChild("buttons", ImVec2{0.0F, l.frameH},
                      ImGuiChildFlags_Borders);

    const Path& sel = g_app->selectedPath;
    const std::vector<Path> many = selectionOrCurrent();
    const bool has = !many.empty();
    const bool multi = many.size() > 1;

    ImGui::BeginDisabled(!has);
    // Bei Mehrfachauswahl auf alle wirken; sonst wie bisher.
    if (fullButton(Str::ActDelete, Str::HintDelete, keys::Action::Delete)) {
        const bool ok = multi ? g_app->doc.removeAll(many) : g_app->doc.removeAt(sel);
        if (ok) {
            g_app->selection.clear();
            g_app->selectedPath.clear();
            rebuildTree();
        }
    }
    if (fullButton(Str::ActClone, Str::HintClone, keys::Action::Clone)) {
        // `multi`/`many` sind dieselben, die Kopieren und Loeschen schon
        // benutzen - Duplizieren war die einzige Aktion, die sie ignorierte.
        if (multi ? g_app->doc.cloneAll(many) : g_app->doc.cloneAt(sel)) {
            rebuildTree();
        }
    }
    ImGui::EndDisabled();
    // Copy auch ohne Auswahl - dann fragt es, ob das ganze Skript kopiert
    // werden soll (Original: "No line selected, Copy entire script?").
    if (halfButton(Str::ActCopy, false, Str::HintCopy, keys::Action::Copy)) {
        kopieren(false);
    }
    ImGui::BeginDisabled(!has);
    if (halfButton(Str::ActCut, true, Str::HintCut, keys::Action::Cut)) {
        kopieren(true);
    }
    ImGui::EndDisabled();

    // Immer anklickbar: der Inhalt kann auch aus der Windows-Zwischenablage
    // kommen. Ohne Auswahl haengt es ans Ende, wie insertAfter.
    if (fullButton(Str::ActPaste, Str::HintPaste, keys::Action::Paste)) {
        einfuegen(sel);
    }

    ImGui::BeginDisabled(!has);
    if (fullButton(Str::ActRem, Str::HintRem, keys::Action::CommentOut)) {
        // Zweiter Klick auf einen bereits auskommentierten Block nimmt es
        // zurueck - im Original liegt das auf Strg+Ruecktaste.
        (void)remUmschalten(sel);
    }
    ImGui::EndDisabled();
    if (fullButton(Str::ActFind, Str::HintFind, keys::Action::Find)) { fensterZeigen(g_app->findOpen, "###find"); }

    // Verschieben. Das Original hat dafuer keine Knoepfe; man musste
    // ausschneiden und woanders einfuegen.
    // Verschieben wirkt auf die ganze Auswahl. Die Wege werden dabei
    // mitgefuehrt, damit die Auswahl beim Schieben nicht verlorengeht.
    ImGui::BeginDisabled(!has);
    // moveSelection steht jetzt frei (siehe oben).
    if (halfButton(Str::MoveUp, false, Str::HintMoveUp, keys::Action::MoveUp)) { moveSelection(true); }
    if (halfButton(Str::MoveDown, true, Str::HintMoveDown, keys::Action::MoveDown)) { moveSelection(false); }
    ImGui::EndDisabled();

    // Rueckgaengig und Wiederholen als Knoepfe.
    //
    // Das Original hat sie nicht - weder als Knopf noch in der Tastentabelle
    // (ACCELERATOR 135 kennt kein Strg+Z). Ein Editor ohne sichtbares
    // Rueckgaengig ist heute aber keiner mehr, deshalb hier ergaenzt.
    // Rechtsklick auf einen der beiden: die Liste aller Schritte (3ds Max).
    ImGui::BeginDisabled(g_app->doc.undoDepth() == 0);
    if (halfButton(Str::EditUndo, false, Str::HintUndo, keys::Action::Undo)) { doUndo(); }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { g_app->undoListeAnfrage = 1; }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(g_app->doc.redoDepth() == 0);
    if (halfButton(Str::EditRedo, true, Str::HintRedo, keys::Action::Redo)) { doRedo(); }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { g_app->undoListeAnfrage = 2; }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered() && g_app->doc.redoDepth() != 0) {
        ImGui::SetTooltip(tr(Str::RedoOf), stepName(g_app->doc.redoLabel()));
    }

    ImGui::Separator();
    ImGui::TextUnformatted(tr(Str::GroupFile));
    if (fullButton(Str::FileNew, Str::HintNew)) { doNew(); }
    if (halfButton(Str::FileOpen, false, Str::HintOpen, keys::Action::Open)) { doOpen(); }
    if (halfButton(Str::FileMru, true, Str::HintMru) || g_app->mruOeffnen) {
        g_app->mruOeffnen = false;   // Alt+M
        ImGui::OpenPopup("mru");
    }
    if (ImGui::BeginPopup("mru")) {
        if (g_app->settings.recent.empty()) {
            ImGui::TextDisabled("%s", tr(Str::MsgNoRecent));
        }
        for (const std::string& r : g_app->settings.recent) {
            if (ImGui::MenuItem(r.c_str())) {
                const std::string rp = r;
                withUnsaved([rp] { loadPath(rp); });
            }
        }
        ImGui::EndPopup();
    }
    if (fullButton(Str::FileAppend, Str::HintAppend)) { doAppend(); }
    if (fullButton(Str::FileSave, Str::HintSave, keys::Action::Save) && doSave()) { addStatus(tr(Str::MsgSaved)); }
    if (fullButton(Str::ActSaveAll, Str::HintSaveAll)) { doSaveAll(); }
    if (fullButton(Str::FileSaveAs, Str::HintSaveAs, keys::Action::SaveAs) && doSaveAs()) { addStatus(tr(Str::MsgSaved)); }
    if (fullButton(Str::FileExport, Str::HintExport)) { doExport(); }
    if (fullButton(Str::FileBackup, Str::HintBackup, keys::Action::Backup)) { doBackup(); }
    if (fullButton(Str::FileRestore, Str::HintRestore, keys::Action::Restore)) { doRestore(); }

    ImGui::Separator();
    ImGui::TextUnformatted(tr(Str::GroupTreeview));
    if (ImGui::Checkbox(tr(Str::TreeShowTypes), &g_app->treeOpt.showTypes)) { rebuildTree(); }
    if (ImGui::Checkbox(tr(Str::TreeGFloats), &g_app->treeOpt.gFloats)) { rebuildTree(); }
    if (ImGui::Checkbox(tr(Str::TreeFoldMacros), &g_app->treeOpt.foldMacros)) { rebuildTree(); }
    // Die Knoepfe + und - des Originals: alles auf- bzw. zuklappen.
    //     "+/-: Expands/Collapses event groups in the editor."
    if (halfButton(Str::TreeExpandAll, false)) {
        g_app->expanded.openAll();
        rebuildTree();
    }
    if (halfButton(Str::TreeCollapseAll, true)) {
        g_app->expanded.closeAll();
        rebuildTree();
    }

    ImGui::Separator();
    if (compileKnopf()) { doCompile(); }

    ImGui::Separator();
    ImGui::TextUnformatted(tr(Str::GroupApplication));
    if (halfButton(Str::AppPrefs, false, Str::HintPrefs)) { g_app->prefsOpen = true; }
    if (halfButton(Str::AppAbout, true)) { g_app->aboutOpen = true; }
    // Beenden. Der Knopf setzt nur ein Flag - schliessen muss die
    // Fensterschicht, die das Fenster kennt.
    //
    // Vorher stand hier (void)fullButton(...): das Ergebnis wurde
    // weggeworfen, der Knopf tat also nichts.
    if (fullButton(Str::AppExit) && confirmQuit()) {
        g_app->quitConfirmed = true;
        g_app->wantQuit = true;
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

// Die Statuszeile - eine Zeile ganz unten, wie in efxed.
//
// Vorher war es ein eigenes Feld mit Rahmen, Ueberschrift und einer Liste
// aller Meldungen. Das kostete standig Hoehe, auch wenn nichts zu melden
// war - und bei einer langen Liste fraess es den halben Bildschirm.
//
// Jetzt: eine Zeile. Die Zaehler sind ANKLICKBAR und oeffnen ein Fenster
// mit den Meldungen. Wer nichts nachlesen will, verliert eine Zeile Hoehe
// statt eines Viertels des Fensters.
//
// Das Muster stammt aus efxed (drawStatusBar in gui/app_panels.cpp) und
// gilt dort seit laengerem als die bessere Loesung - genau deshalb hier
// uebernommen und nicht neu erfunden.
void drawStatus(const Layout& l) {
    (void)l;
    // --- GANZ nach unten ------------------------------------------------
    //
    // Sie wurde bisher gezeichnet, wo der Inhalt darueber gerade endete.
    // Fuellt der nicht die ganze Hoehe, bleibt darunter Luft: auf shanks
    // Bild steht "Ready ... 103 fps" mit einem leeren Streifen darunter.
    //
    // Eine Statuszeile gehoert an den Rand. Der Sprung ist nach unten
    // begrenzt - steht der Inhalt schon tiefer, bleibt sie, wo sie ist,
    // statt sich zu ueberlagern.
    {
        // Knopfhoehe, nicht Textzeile: die Zaehler (Fehler/Warnungen) sind
        // Knoepfe und damit hoeher als eine Textzeile. Mit der Textzeile
        // ragten sie unten um 2-3 Punkte hinaus, und das Hauptfenster wurde
        // rollbar (Selbsttest, Kartenansicht mit zehn Skripten).
        const float zeile = std::max(ImGui::GetTextLineHeightWithSpacing(),
                                     ImGui::GetFrameHeight());
        const float unten = ImGui::GetWindowHeight() -
                            ImGui::GetStyle().WindowPadding.y - zeile;
        if (unten > ImGui::GetCursorPosY()) {
            ImGui::SetCursorPosY(unten);
        }
    }
    int errs = 0;
    int warns = 0;
    for (const Issue& i : g_app->issues) {
        if (i.level == Issue::Level::Error) { ++errs; } else { ++warns; }
    }

    // Ein Zaehler als Knopf ohne Knopfaussehen: anklickbar, aber ruhig.
    const auto zaehler = [&](int n, const char* text, bool fehler) {
        if (n == 0) {
            return;
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0, 0, 0, 0});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0, 0, 0, 0});
        if (fehler) {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4{0.90F, 0.35F, 0.35F, 1.0F});
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4{0.90F, 0.75F, 0.35F, 1.0F});
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%d %s", n, text);
        if (ImGui::SmallButton(buf)) {
            fensterZeigen(g_app->messagesOpen, "###messages");
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::StatusClickHint));
        }
        ImGui::SameLine();
    };

    // Im Multiplayer-Modus sichtbar sagen, dass die MP-Regeln gelten - sonst
    // wundert man sich ueber die vielen Anmerkungen.
    if (g_app->settings.mpZiel) {
        ImGui::TextColored(ImVec4{0.95F, 0.65F, 0.25F, 1.0F}, "%s", tr(Str::StatusMp));
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::StatusMpHint)); }
        ImGui::SameLine();
    }
    zaehler(errs, tr(Str::StatusErrors), true);
    zaehler(warns, tr(Str::StatusWarnings), false);
    if (errs == 0 && warns == 0) {
        ImGui::TextDisabled("%s", tr(Str::StatusReady));
        ImGui::SameLine();
    }

    // Ein leiser Trenner zwischen den Gruppen (shank: "Notes | Undo history
    // ---- Script info | Compiled: time/date").
    const auto trenner = [] {
        ImGui::TextDisabled("|");
        ImGui::SameLine();
    };

    // Was das letzte Undo/Redo getan hat - anklickbar: oeffnet die
    // Undo-Liste. Hat sich das Dokument seitdem geaendert (neue Bearbeitung,
    // anderer Reiter), ist die Meldung veraltet und verschwindet.
    if (!g_app->undoMeldung.empty() && g_app->doc.undoDepth() != g_app->undoMeldungTiefe) {
        g_app->undoMeldung.clear();
    }
    // Sonst - sobald es etwas zurueckzunehmen gibt - IMMER ein Eintrag fuer die
    // Undo-Liste, mit dem Schritt, den das naechste Undo zuruecknaehme (shank:
    // "ich dachte, die Undo History wird immer unten angezeigt, so dass man
    // sie schnell oeffnen kann, am besten bevor man das erste Undo gemacht
    // hat"). Der Text wird nur neu gerechnet, wenn sich der Stapel aendert.
    std::string eintrag = g_app->undoMeldung;
    bool redoListe = g_app->undoMeldungRedo;
    if (eintrag.empty() && g_app->doc.undoDepth() > 0) {
        static std::string letzterText;
        static std::uint64_t letzterStand = static_cast<std::uint64_t>(-1);
        static int letzterReiter = -1;
        const std::size_t n = g_app->doc.undoDepth();
        // Nach dem Stand des Dokuments, nicht nach der Adresse des letzten
        // Schritts: die kann nach Undo und neuer Bearbeitung gleich bleiben,
        // und dann stand unten ein veralteter Text ("Undo history (6): edit").
        if (g_app->doc.stand() != letzterStand || g_app->activeTab != letzterReiter) {
            letzterStand = g_app->doc.stand();
            letzterReiter = g_app->activeTab;
            g_app->doc.vergibKennungen();
            std::string t = aenderungsText(g_app->doc.undoStand(n - 1), g_app->doc.script(), nullptr);
            if (t.empty()) { t = stepName(g_app->doc.undoWas(n - 1)); }
            char z[400];
            std::snprintf(z, sizeof(z), tr(Str::UndoHistoryStatus), static_cast<int>(n), t.c_str());
            letzterText = z;
        }
        eintrag = letzterText;
        redoListe = false;
    }
    if (!eintrag.empty()) {
        trenner();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0, 0, 0, 0});
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.55F, 0.75F, 1.0F, 1.0F});
        if (ImGui::SmallButton(eintrag.c_str())) {
            g_app->undoListeAnfrage = redoListe ? 2 : 1;
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::UndoListHint));
        }
        g_app->undoStatusText = eintrag;
        ImGui::SameLine();
    } else {
        g_app->undoStatusText.clear();
    }

    // Ein gefundenes Update (gruen, anklickbar) - siehe gui/update_win32.cpp.
    updater::zeichneStatus();

    // Rechts die Bildrate, wie in efxed - davor, wann die .ibi dieses
    // Skripts zuletzt geschrieben wurde ("how about when you compile, at the
    // bottom it gives the date and time of the last compile?"). Aus der Datei
    // gelesen, damit es auch nach einem Neustart stimmt; einmal je Sekunde.
    static std::string letztePfad;
    static double letzteAbfrage = -10.0;
    std::string& letzteZeit = g_app->letzteKompilierung;
    if (g_app->path != letztePfad || ImGui::GetTime() - letzteAbfrage > 1.0) {
        letztePfad = g_app->path;
        letzteAbfrage = ImGui::GetTime();
        letzteZeit.clear();
        if (!g_app->path.empty()) {
            std::string ibi = g_app->path;
            const std::size_t dot = ibi.find_last_of('.');
            const std::size_t slash = ibi.find_last_of("/\\");
            if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
                ibi = ibi.substr(0, dot);
            }
            ibi += ".ibi";
            std::error_code ec;
            const auto wann = std::filesystem::last_write_time(::bhed::utf8Pfad(ibi), ec);
            if (!ec) {
                const auto sys = std::chrono::clock_cast<std::chrono::system_clock>(wann);
                const std::time_t tt = std::chrono::system_clock::to_time_t(sys);
                std::tm lokal{};
                if (localtime_s(&lokal, &tt) == 0) {
                    char b[40];
                    std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &lokal);
                    char z[120];
                    std::snprintf(z, sizeof(z), tr(Str::StatusLastCompile), b);
                    letzteZeit = z;
                }
            }
        }
    }
    // --- Rechts: Skript-Info | Compiled: ... [| fps nur auf der Karte] -----
    //
    // shank: "Don't really need an FPS counter for scripting, maybe just have
    // it on the map window" - die Bildrate nur im Karten-Reiter.
    std::vector<std::string> teile;
    if (!g_app->statusLines.empty()) {
        teile.push_back(g_app->statusLines.back());
    }
    if (!letzteZeit.empty()) {
        teile.push_back(letzteZeit);
    }
    if (g_app->leftMode == 1) {
        char fps[32];
        std::snprintf(fps, sizeof(fps), "%.0f fps", static_cast<double>(ImGui::GetIO().Framerate));
        teile.emplace_back(fps);
    }
    const ImGuiStyle& st = ImGui::GetStyle();
    const float strich = ImGui::CalcTextSize("|").x + st.ItemSpacing.x * 2.0F;
    float breite = 0.0F;
    for (std::size_t i = 0; i < teile.size(); ++i) {
        breite += ImGui::CalcTextSize(teile[i].c_str()).x + (i > 0 ? strich : 0.0F);
    }
    const float frei = ImGui::GetContentRegionAvail().x;
    if (!teile.empty() && frei > breite + st.ItemSpacing.x) {
        ImGui::SameLine(ImGui::GetCursorPosX() + frei - breite);
        for (std::size_t i = 0; i < teile.size(); ++i) {
            if (i > 0) {
                ImGui::TextDisabled("|");
                ImGui::SameLine();
            }
            ImGui::TextDisabled("%s", teile[i].c_str());
            if (teile[i] == letzteZeit && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tr(Str::StatusLastCompileHint));
            }
            if (i + 1 < teile.size()) { ImGui::SameLine(); }
        }
    } else {
        ImGui::NewLine();
    }
}

// Das Fenster mit den Meldungen.
//
// Hier steht, was der Zaehler nur zaehlt. Frueher stand beides zugleich da
// und nahm dauerhaft Platz weg.
void drawMessages() {
    if (!g_app->messagesOpen) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 40.0F,
                                    ImGui::GetFontSize() * 18.0F},
                             ImGuiCond_FirstUseEver);
    // Eine Untergrenze, wie in efxed: ohne sie laesst sich das Fenster auf
    // Fingernagelgroesse ziehen und ist dann nicht mehr zu bedienen.
    ImGui::SetNextWindowSizeConstraints(
        ImVec2{ImGui::GetFontSize() * 20.0F, ImGui::GetFontSize() * 8.0F},
        ImVec2{FLT_MAX, FLT_MAX});
    // "###messages" haengt die Fensterkennung fest, unabhaengig von der
    // Sprache der Ueberschrift.
    const std::string titel = std::string(tr(Str::MessagesTitle)) + "###messages";
    // Lange Meldungen rollen waagerecht, statt abgeschnitten zu werden -
    // der Fenstertest fand bei der Untergrenze 9 Punkte zu wenig.
    if (!ImGui::Begin(titel.c_str(), &g_app->messagesOpen,
                      ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::End();
        return;
    }
    if (g_app->issues.empty() && g_app->statusLines.empty()) {
        ImGui::TextDisabled("%s", tr(Str::MessagesNone));
    } else {
        int errs = 0;
        int warns = 0;
        for (const Issue& i : g_app->issues) {
            if (i.level == Issue::Level::Error) { ++errs; } else { ++warns; }
        }
        ImGui::Text(tr(Str::MsgIssues), errs, warns);
        ImGui::Separator();
    }
    for (const std::string& line : g_app->statusLines) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (!g_app->issues.empty()) {
        ImGui::Separator();
        // Nur lesen, nicht springen.
        //
        // Ein Klick, der zur beanstandeten Zeile fuehrt, waere das
        // Naheliegende - aber Issue traegt nur "where" als Text
        // ("affect/task/set"), keinen Weg in den Baum. Den zu erraten hiesse
        // ihn zu erfinden. Wenn das gewuenscht ist, gehoert ein Path in
        // Issue, und zwar dort, wo die Meldung entsteht.
        // Aufbau wie in efxed: der Schweregrad farbig davor, dann der
        // Ort gedaempft, dann die Meldung im normalen Text. So liest man
        // die Meldung und nicht die Farbe.
        for (const Issue& i : g_app->issues) {
            const bool fehler = i.level == Issue::Level::Error;
            ImGui::TextColored(fehler ? ImVec4{0.90F, 0.35F, 0.35F, 1.0F}
                                      : ImVec4{0.90F, 0.75F, 0.35F, 1.0F},
                               "%s", fehler ? tr(Str::StatusErrors)
                                            : tr(Str::StatusWarnings));
            ImGui::SameLine();
            ImGui::TextDisabled("%s", i.code.c_str());
            ImGui::SameLine();
            if (!i.where.empty()) {
                ImGui::TextDisabled("%s", i.where.c_str());
                ImGui::SameLine();
            }
            // Uebersetzt, wenn wir zu dem Code einen Satz haben.
            //
            // Der Kern schreibt seine Meldungen auf Deutsch - er kennt keine
            // Uebersetzung, und fuer checks und validate auf der
            // Befehlszeile ist das genau richtig. In der Oberflaeche soll
            // aber alles in einer Sprache stehen; gemeldet wurde "the text
            // is in German".
            //
            // Kennen wir den Code nicht, steht der Text des Kerns da. Das
            // ist der Rueckfall und kein Fehler: eine neue Pruefung wirkt
            // sofort, auch bevor jemand einen Satz dafuer geschrieben hat.
            const char* muster = nullptr;
            if (i.code == "V002") { muster = tr(Str::VmsgV002); }
            else if (i.code == "V004") { muster = tr(Str::VmsgV004); }
            else if (i.code == "V006") { muster = tr(Str::VmsgV006); }
            else if (i.code == "V007") { muster = tr(Str::VmsgV007); }
            else if (i.code == "V010") { muster = tr(Str::VmsgV010); }
            // Fuenf von zehn Pruefcodes hatten keine Uebersetzung: V001,
            // V005, V008, V009, V011. Deshalb standen im Meldungsfenster
            // deutsche Saetze, auch wenn die Oberflaeche auf Englisch lief.
            //
            // `VmsgV007` trug ausserdem den Text von V009 - die Nummer im
            // Namen und die Nummer im Code waren auseinander. Sie hiess
            // richtig, verglichen wurde nur die falsche.
            else if (i.code == "V001") { muster = tr(Str::VmsgV001); }
            else if (i.code == "V005") { muster = tr(Str::VmsgV005); }
            else if (i.code == "V008") { muster = tr(Str::VmsgV008); }
            else if (i.code == "V009") { muster = tr(Str::VmsgV009); }
            else if (i.code == "V011") { muster = tr(Str::VmsgV011); }
            else if (i.code == "V012") { muster = tr(Str::VmsgV012); if (!i.arg2.empty()) { muster = tr(Str::VmsgV012b); } }
            else if (i.code.size() == 4 && i.code[0] == 'M') {
                static const Str kMp[] = {Str::MmsgM001, Str::MmsgM002, Str::MmsgM003, Str::MmsgM004,
                                          Str::MmsgM005, Str::MmsgM006, Str::MmsgM007, Str::MmsgM008,
                                          Str::MmsgM009, Str::MmsgM010, Str::MmsgM011, Str::MmsgM012,
                                          Str::MmsgM013, Str::MmsgM014, Str::MmsgM015, Str::MmsgM016,
                                          Str::MmsgM017};
                const int nr = std::atoi(i.code.c_str() + 1);
                if (nr >= 1 && nr <= static_cast<int>(sizeof(kMp) / sizeof(kMp[0]))) {
                    muster = tr(kMp[nr - 1]);
                }
            }
            if (muster != nullptr) {
                ImGui::TextWrapped(muster, i.arg1.c_str(), i.arg2.c_str());
            } else {
                ImGui::TextWrapped("%s", i.message.c_str());
            }
        }
    }
    ImGui::End();
}

void drawMenu() {
    if (!ImGui::BeginMenuBar()) { return; }
    if (ImGui::BeginMenu(tr(Str::MenuFile))) {
        if (ImGui::MenuItem(tr(Str::FileNew))) { doNew(); }
        if (ImGui::MenuItem(tr(Str::FileOpen), "Ctrl+O")) { doOpen(); }
        if (ImGui::MenuItem(tr(Str::FileSave), "Ctrl+S")) { (void)doSave(); }
        // Strg+A ist im Original "Speichern unter", nicht "alles markieren".
        if (ImGui::MenuItem(tr(Str::FileSaveAs), "Ctrl+A")) { (void)doSaveAs(); }
        // Zuletzt geoeffnete Missionen (rc599): ein ganzes Projekt - Karte,
        // Entities und alle Skripte der Mission in ihren Reitern.
        {
            const std::vector<LetzteMission> missionen = letzteMissionenAlle();
            if (ImGui::BeginMenu(tr(Str::RecentMissions), !missionen.empty())) {
                for (const LetzteMission& lm : missionen) {
                    ImGui::PushID(lm.eintrag.c_str());
                    if (ImGui::MenuItem(lm.name.c_str(), lm.ort.c_str())) {
                        const std::string e = lm.eintrag;
                        letzteMissionOeffnen(e);
                    }
                    ImGui::PopID();
                }
                ImGui::EndMenu();
            }
        }
        if (ImGui::BeginMenu(tr(Str::MsgRecent), !g_app->settings.recent.empty())) {
            for (const std::string& r : g_app->settings.recent) {
                if (ImGui::MenuItem(r.c_str())) {
                const std::string rp = r;
                withUnsaved([rp] { loadPath(rp); });
            }
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::Pk3Browse))) {
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
            refreshPk3List();
            g_app->pk3Open = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::LoadMission))) { loadMissionArchive(); }
        if (ImGui::MenuItem(tr(Str::MapLoad))) { doLoadMap(); }
        if (ImGui::MenuItem(tr(Str::OpenEnt))) { openEntFile(); }
        if (ImGui::MenuItem(tr(Str::MapClose), nullptr, false, !g_app->map.empty())) {
            clearMap();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::OpenGlm))) { openModelFile(); }
        if (ImGui::MenuItem(tr(Str::OpenSkin), nullptr, false, !g_app->model.empty())) {
            openSkinFile();
        }
        if (ImGui::MenuItem(tr(Str::ModelLoadGla), nullptr, false,
                            !g_app->model.empty())) {
            openSkeletonFile();
        }
        ImGui::Separator();
        // Wie viele Spalten? Ein Untermenue mit 1 bis 4 - wie das
        // Farbgebungs- und Sprachmenue daneben.
        // Bei Karte und Modell wirkt es nicht - dann abgeblendet, mit
        // einem Hinweis, statt still nichts zu tun.
        const bool imSkriptmodus = (g_app->leftMode == 0);
        ImGui::BeginDisabled(!imSkriptmodus);
        if (ImGui::BeginMenu(tr(Str::SplitToggle))) {
            // Nicht mehr Felder als SKRIPTE.
            //
            // "Das split view sollte nur gehen wenn es neue tabs gibt und
            // nicht sich selbst duplizieren." Genau so: mit einem offenen
            // Skript gibt es nichts zu vergleichen, und ein zweites Feld
            // koennte nur dasselbe noch einmal zeigen.
            //
            // Die Zahl der Reiter IST die Obergrenze; darueber sind die
            // Eintraege abgeblendet, mit einem Hinweis, warum.
            const int offene = static_cast<int>(g_app->tabs.size());
            for (int n = 1; n <= App::kMaxSplit; ++n) {
                char be[8];
                std::snprintf(be, sizeof(be), "%d", n);
                const bool moeglich = (n <= std::max(1, offene));
                ImGui::BeginDisabled(!moeglich);
                const bool gewaehlt =
                    ImGui::MenuItem(be, nullptr, g_app->splitCount == n);
                ImGui::EndDisabled();
                if (!moeglich && ImGui::IsItemHovered(
                                     ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::SetTooltip("%s", tr(Str::SplitNeedsTabs));
                }
                if (gewaehlt) {
                    const int vorher = g_app->splitCount;
                    g_app->splitCount = n;
                    // Jedem NEUEN Feld ein eigenes Skript geben.
                    //
                    // Vorher stand in jedem Feld `tab = 0`. Ein Klick ins
                    // zweite Feld schaltete deshalb auf das ERSTE Skript um
                    // - gemeldet als "if I click the second window it
                    // changes to the first script". Und nebeneinander stand
                    // dann dreimal dasselbe, was niemand braucht.
                    //
                    // Also: das erste noch nicht gezeigte Skript nehmen.
                    // Gibt es keines mehr - weniger Reiter als Felder -,
                    // bleibt es beim bisherigen; dann ist eine Dopplung
                    // unvermeidlich.
                    const int nT = static_cast<int>(g_app->tabs.size());
                    for (int i = vorher; i < n; ++i) {
                        int frei = -1;
                        for (int k = 0; k < nT && frei < 0; ++k) {
                            bool belegt = (k == g_app->activeTab);
                            for (int q = 0; q < i && !belegt; ++q) {
                                if (g_app->splitPanes[q].tab == k) {
                                    belegt = true;
                                }
                            }
                            if (!belegt) {
                                frei = k;
                            }
                        }
                        if (frei >= 0) {
                            g_app->splitPanes[i].tab = frei;
                        }
                    }
                    // Der Fokus muss in einem Feld liegen, das es noch
                    // gibt: von vier auf eins herunter ist sonst Feld 3
                    // fokussiert und keines davon sichtbar.
                    g_app->focusPane = std::clamp(g_app->focusPane, 0, n - 1);
                    for (auto& pn : g_app->splitPanes) {
                        pn.dirty = true;
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndDisabled();
        if (!imSkriptmodus &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", tr(Str::SplitOnlyScript));
        }
        if (imSkriptmodus &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            // Zwei Saetze, zwei Zeilen - aber NICHT ueber ein Format mit
            // Zeilenschaltung: ein mehrzeiliges Format ist mit
            // -Werror=format ein Fehler. Die Bindeprobe hat es sofort
            // gemeldet, noch bevor ich es gebaut habe.
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(tr(Str::SplitHint));
            ImGui::EndTooltip();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::FileBackupFolder))) {
            sicherungsOrdnerOeffnen();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::FileBackupFolderHint));
        }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::AppExit)) && confirmQuit()) {
            g_app->quitConfirmed = true;
            g_app->wantQuit = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::MenuEdit))) {
        if (ImGui::BeginMenu(tr(Str::MenuBookmarks))) {
            if (ImGui::MenuItem(tr(Str::ActBookmarkToggle), chordName(keys::Action::BookmarkToggle))) {
                lesezeichenUmschalten(selectionOrCurrent());
            }
            if (ImGui::MenuItem(tr(Str::ActBookmarkNext), chordName(keys::Action::BookmarkNext))) {
                lesezeichenSpringen(true);
            }
            if (ImGui::MenuItem(tr(Str::ActBookmarkPrev), chordName(keys::Action::BookmarkPrev))) {
                lesezeichenSpringen(false);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(tr(Str::ActBookmarkClear))) { lesezeichenAlleWeg(); }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        // Die Tastenbelegung war nur ueber den Knopf "Prefs" im
        // Aktionsstreifen zu finden. Wer sie sucht, sucht sie im Menue.
        // Drei Wege ins selbe Fenster, absichtlich.
        //
        // Wer die Spielordner sucht, sucht nicht unter "Einstellungen" -
        // er sucht unter "Spielordner". Dasselbe gilt fuer die
        // Tastenkuerzel. Ein Menueeintrag je Sache kostet nichts und spart
        // das Suchen.
        // Drei Sachen, drei Fenster, drei Eintraege.
        if (ImGui::MenuItem(tr(Str::PathsTitle))) {
            fensterZeigen(g_app->pathsOpen, "###paths");
        }
        if (ImGui::MenuItem(tr(Str::KeysTitle))) {
            fensterZeigen(g_app->keysOpen, "###keys");
        }
        if (ImGui::MenuItem(tr(Str::AppPrefs))) {
            g_app->prefsOpen = true;
        }
        ImGui::Separator();
        // Der Eintrag nennt, was er zuruecknimmt: "Rueckgaengig: Loeschen".
        char label[120];
        if (g_app->doc.undoDepth() != 0) {
            std::snprintf(label, sizeof(label), tr(Str::UndoOf),
                          stepName(g_app->doc.undoLabel()));
        } else {
            std::snprintf(label, sizeof(label), "%s", tr(Str::EditUndo));
        }
        // Die WIRKLICHE Belegung zeigen, nicht eine fest getippte: sonst
        // steht im Menue "Ctrl+Z", waehrend die Taste laengst umgelegt ist.
        if (ImGui::MenuItem(label, chordName(keys::Action::Undo), false,
                            g_app->doc.undoDepth() != 0)) {
            doUndo();
        }
        if (g_app->doc.redoDepth() != 0) {
            std::snprintf(label, sizeof(label), tr(Str::RedoOf),
                          stepName(g_app->doc.redoLabel()));
        } else {
            std::snprintf(label, sizeof(label), "%s", tr(Str::EditRedo));
        }
        if (ImGui::MenuItem(label, chordName(keys::Action::Redo), false,
                            g_app->doc.redoDepth() != 0)) {
            doRedo();
        }
        if (ImGui::MenuItem(tr(Str::UndoListMenu), nullptr, false, g_app->doc.undoDepth() != 0)) {
            g_app->undoListeAnfrage = 1;
        }
        ImGui::EndMenu();
    }
    // --- Ein eigenes Menue fuer die Fehlersuche -------------------------
    //
    // Die Protokollschalter und der GPU-Weg standen unter "View", zwischen
    // Dingen, die das Bild einstellen. shank: "ich glaube es waere ein neues
    // Menue besser statt bei view kannst du debug machen".
    //
    // Er hat recht - "View" beantwortet die Frage "wie sieht es aus",
    // "Debug" die Frage "was ist da los". Zwei verschiedene Fragen.
    if (ImGui::BeginMenu(tr(Str::MenuDebug))) {

        // Auf Zuruf, nicht dauernd: bei 62 Figuren und sechzig Bildern je
        // Sekunde waeren das 3720 Zeilen in der Sekunde.
        if (ImGui::MenuItem(tr(Str::ViewDumpActors), nullptr, false,
                            !g_app->scene.actors.empty())) {
            dumpActorsAtPlayhead();
        }

        // Als SCHALTER, nicht als Knopf - siehe App::logFrameCost.
        if (ImGui::MenuItem(tr(Str::ViewFrameCost), nullptr,
                            g_app->logFrameCost)) {
            g_app->logFrameCost = !g_app->logFrameCost;
            g_app->frameCostTick = 0;
            diag::info(g_app->logFrameCost
                           ? "Zeit je Bild: Protokoll AN - jetzt die Kamera "
                             "bewegen, jedes 20. Bild wird notiert"
                           : "Zeit je Bild: Protokoll aus");
        }

        if (ImGui::MenuItem(tr(Str::ViewLookRotation), nullptr,
                            g_app->lookRotation)) {
            g_app->lookRotation = !g_app->lookRotation;
            g_app->mapDirty = true;
        }

        ImGui::Separator();

        // --- Eine Bildaufnahme aus dem Programm heraus ------------------
        //
        // RenderDoc kann sich NICHT nachtraeglich an ein laufendes Programm
        // haengen - es legt seine Bibliothek beim Start hinein. Ein Knopf,
        // der RenderDoc startet, brauchte also einen Neustart und brachte
        // nichts.
        //
        // Was geht: laeuft behaved schon unter RenderDoc, liegt
        // `renderdoc.dll` im Prozess, und ueber `RENDERDOC_GetAPI` laesst
        // sich eine Aufnahme ausloesen - genau wie F12, nur aus dem Menue.
        //
        // Dafuer braucht es KEINE zusaetzliche Bibliothek: gebraucht wird
        // eine einzige Funktion, und die wird hier von Hand erklaert.
        {
            const bool da = platform::renderDocDa();
            // Nur die AUFNAHME braucht RenderDoc. Das BeginDisabled stand
            // vor allen vier Eintraegen - Layoutzahlen, Protokollauszug und
            // "Aufteilung zuruecksetzen" waren ohne RenderDoc grau
            // (Kartentest 27.09.).
            if (ImGui::MenuItem(tr(Str::DebugLayout), nullptr,
                                g_app->layoutOpen)) {
                g_app->layoutOpen = !g_app->layoutOpen;
            }
            // Auf Knopfdruck alles hineinschreiben, auch ohne Aenderung.
            //
            // Der Auszug haengt sonst an `groessenAnders` - wer nichts
            // verstellt, bekommt also nichts. Genau dann will man ihn aber
            // manchmal: "so sieht es JETZT aus".
            if (ImGui::MenuItem(tr(Str::DebugDump))) {
                g_app->dumpAngefordert = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tr(Str::DebugDumpHint));
            }

        // --- Aufteilung auf die Vorgaben zuruecksetzen -------------------
        //
        // Nach dieser Sitzung stehen in shanks Einstellungen Werte, die aus
        // Fehlern stammen: die Rueckkopplung aus rc484 hat `splitMap`
        // kleingeschrumpft, und eine Untergrenze hebt ihn zwar beim
        // Benutzen an, schreibt ihn aber nicht zurueck.
        //
        // Eine `imgui.ini` gibt es NICHT - `main_win32.cpp:251` setzt
        // `io.IniFilename = nullptr`. Mein Hinweis in rc482, sie zu
        // loeschen, ging ins Leere; es gibt nichts zu loeschen. Alles
        // Gespeicherte liegt in `settingsFile`.
        //
        // Zurueckgesetzt werden nur die AUFTEILUNGSWERTE. Pfade,
        // Spracheinstellung und alles andere bleiben - wer die Aufteilung
        // richten will, soll nicht seine Pfade verlieren.
        if (ImGui::MenuItem(tr(Str::DebugReset))) {
            const Settings vorgabe;
            g_app->settings.splitEvents = vorgabe.splitEvents;
            g_app->settings.splitMap = vorgabe.splitMap;
            g_app->settings.splitModel = vorgabe.splitModel;
            g_app->settings.splitButtons = vorgabe.splitButtons;
            g_app->settings.timelineFrac = vorgabe.timelineFrac;
            g_app->settings.splitFracX = vorgabe.splitFracX;
            g_app->settings.splitFracY = vorgabe.splitFracY;
            // Nicht sofort sichern: `destroyApp` ruft `saveSettings`
            // beim Beenden, und jede andere Aufteilungsaenderung wird
            // genauso behandelt. Hier eine zweite Speicherstelle
            // aufzumachen waere das Muster, das in diesem Programm schon
            // oft genug auseinandergelaufen ist.
            g_app->mapDirty = true;
            addStatus(tr(Str::DebugResetDone));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::DebugResetHint));
        }
            ImGui::Separator();
            ImGui::BeginDisabled(!da);
            if (ImGui::MenuItem(tr(Str::DebugCapture))) {
                platform::renderDocAufnehmen();
                addStatus(tr(Str::DebugCaptureHint));
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", da ? tr(Str::DebugCaptureHint)
                                           : tr(Str::DebugNoRenderDoc));
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(tr(Str::MenuView))) {
        // Zielspiel. Im Multiplayer tut vieles aus dem Singleplayer nichts -
        // meist ohne jede Meldung im Spiel (@Seph, MP-Bericht 08.10.,
        // nachgeprueft gegen codemp/game/g_ICARUScb.c).
        if (ImGui::BeginMenu(tr(Str::ViewTarget))) {
            if (ImGui::MenuItem(tr(Str::TargetSp), nullptr, !g_app->settings.mpZiel)) {
                g_app->settings.mpZiel = false;
                rebuildTree();
            }
            if (ImGui::MenuItem(tr(Str::TargetMp), nullptr, g_app->settings.mpZiel)) {
                g_app->settings.mpZiel = true;
                rebuildTree();
            }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::TargetMpHint)); }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::ViewChangeHistory), nullptr, g_app->settings.changeHistory)) {
            g_app->settings.changeHistory = !g_app->settings.changeHistory;
            rebuildTree();
            for (auto& pn : g_app->splitPanes) { pn.dirty = true; }
        }
        if (ImGui::MenuItem(tr(Str::ViewHighlightSame), nullptr, g_app->settings.highlightSame)) {
            g_app->settings.highlightSame = !g_app->settings.highlightSame;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip("%s", tr(Str::ViewHighlightSameHint));
        }
        ImGui::Separator();
        // --- Modell ------------------------------------------------------
        //
        // Was man einmal einstellt und dann nicht mehr anfasst, gehoert ins
        // Menue. Die Leiste unter der Ansicht bleibt dadurch flach, und dem
        // Bild geht weniger Hoehe verloren.
        ImGui::SeparatorText(tr(Str::ViewMenuModel));
        {
            const bool have = !g_app->model.empty();
            ImGui::BeginDisabled(!have);
            if (ImGui::MenuItem(tr(Str::ModelTextures), nullptr,
                                g_app->showModelTextures)) {
                g_app->showModelTextures = !g_app->showModelTextures;
                g_app->modelDirty = true;
            }
            if (ImGui::MenuItem(tr(Str::ModelCaps), nullptr, g_app->showCaps)) {
                g_app->showCaps = !g_app->showCaps;
                g_app->modelDirty = true;
                g_app->mapDirty = true;   // wirkt auch auf die Figuren der Karte
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tr(Str::ModelCapsHint));
            }
            ImGui::EndDisabled();
            // --- Zahlen zum Vergleichen ------------------------------
            //
            // Ein GPU-Weg, der ein falsches Bild zeigt, sagt von sich aus
            // nichts. Diese drei Zahlen sagen, WO es klemmt:
            //
            //   Aufrufe 0        gar nichts gezeichnet
            //   Aufrufe << Liste viele Stapel uebersprungen (fehlende
            //                    Textur oder Shader)
            //   Aufrufe = Liste  gezeichnet - dann liegt es am Aussehen,
            //                    nicht am Weg dorthin
            {
                if (!g_app->gpuFehler.empty()) {
                    ImGui::TextDisabled("%s", g_app->gpuFehler.c_str());
                }
                // --- Die Zahlen stehen in der SEITENLEISTE -------------
                //
                // Sie standen hier, im View-Menue, und das ist beim Hinsehen
                // zu. Genau davor warnt Abschnitt 6 der Uebergabe: "die
                // Zaehlung stand im geschlossenen View-Menue".
                //
                // Es gab die Aufstellung ZWEIMAL - hier und in
                // `app_view3d.cpp` neben den Dreiecken. Die Zeitmarken aus
                // rc431 und der Zustandsschalter aus rc435 landeten in
                // dieser Fassung, also in der unsichtbaren. Auf shanks
                // Bildern stand deshalb weiter die alte einzeilige Zeile.
                //
                // Jetzt gibt es sie nur noch einmal, und zwar dort, wo man
                // hinschaut. Hier bleibt allein die FEHLERMELDUNG - die
                // gehoert an beide Stellen, weil sie erklaert, warum das
                // Menue gerade nichts Sinnvolles zeigt.
            }
            if (have) {
                std::size_t tris = 0;
                std::size_t drawn = 0;
                for (const GlmSurface& sf : g_app->model.surfaces) {
                    if (sf.isTag()) {
                        continue;
                    }
                    ++drawn;
                    tris += sf.indexes.size() / 3;
                }
                ImGui::Separator();
                ImGui::TextDisabled(tr(Str::ModelInfo), static_cast<int>(drawn),
                                    static_cast<int>(tris),
                                    g_app->model.numBones);
                if (g_app->modelTextures.found + g_app->modelTextures.missing != 0) {
                    ImGui::TextDisabled(tr(Str::ModelTexFound),
                                        g_app->modelTextures.found,
                                        g_app->modelTextures.found +
                                            g_app->modelTextures.missing);
                }
                ImGui::TextDisabled(tr(Str::ModelSkeleton),
                                    g_app->model.animFile.c_str());
                if (!g_app->anim.empty()) {
                    ImGui::TextDisabled(tr(Str::ModelAnimCount),
                                        static_cast<int>(g_app->animList.size()),
                                        g_app->anim.numFrames);
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::ViewLookRotationHint));
        }

        if (ImGui::BeginMenu(tr(Str::ViewTheme))) {
            for (const theme::Theme& t : theme::themes()) {
                const bool sel = (g_app->activeTheme != nullptr &&
                                  g_app->activeTheme->id == t.id);
                if (ImGui::MenuItem(t.name(), nullptr, sel)) {
                    g_app->activeTheme = &t;
                    g_app->themePending = true;
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(tr(Str::ViewLanguage))) {
            for (const i18n::LanguageInfo& l : i18n::languages()) {
                const bool sel = (i18n::currentLanguage() == l.language);
                if (ImGui::MenuItem(l.nativeName, nullptr, sel)) {
                    i18n::setLanguage(l.language);
                }
            }
            ImGui::EndMenu();
        }
        // Feste Stufen statt eines Reglers.
        //
        // Ein Schieberegler IM MENUE ist ein schlechtes Bedienelement: das
        // Menue faengt Mausbewegungen ab, und beim Loslassen schliesst es.
        // Man zieht also blind und trifft selten. Windows, Firefox und
        // Visual Studio machen es alle mit festen Stufen - dieselben, die
        // Windows in seinen Anzeigeeinstellungen anbietet.
        //
        // Feiner einstellen geht weiter mit Strg + Mausrad; dort ist ein
        // stufenloser Wert am richtigen Platz, weil man dabei sieht, was
        // passiert.
        if (ImGui::BeginMenu(tr(Str::ViewDpiScale))) {
            for (const int step : {75, 100, 125, 150, 175, 200, 250, 300}) {
                const float want = static_cast<float>(step) / 100.0F;
                char label[16];
                std::snprintf(label, sizeof(label), "%d %%", step);
                // Die aktuelle Stufe anhaken - auch wenn sie ueber das
                // Mausrad dazwischen liegt, ist die naechstgelegene gemeint.
                const bool on = std::fabs(g_app->uiScale - want) < 0.005F;
                if (ImGui::MenuItem(label, nullptr, on)) {
                    g_app->uiScale = want;
                }
            }
            ImGui::Separator();
            ImGui::TextDisabled(tr(Str::ViewScaleNow),
                                static_cast<int>(g_app->uiScale * 100.0F + 0.5F));
            ImGui::EndMenu();
        }
        ImGui::TextDisabled("%s", tr(Str::ViewScaleHint));
        ImGui::EndMenu();
    }
    // --- Help: About, Changelog, Updates --------------------------------
    //
    // shank (11.10.): "mach einen neuen Reiter mit About und da Update und
    // Changelog rein". Vorher standen Update und Changelog im Datei-Menue
    // und About nur als Knopf bei den Programmknoepfen (der bleibt).
    if (ImGui::BeginMenu(tr(Str::MenuHelp))) {
        // H1: der Helfer - "Was willst du tun?" (F1).
        if (ImGui::MenuItem(tr(Str::HlfMenu), chordName(keys::Action::Guide))) { hilfeOeffnen(); }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::HlfMenuHint)); }
        // Das Handbuch mit Tutorials (rc599).
        if (ImGui::MenuItem(tr(Str::HelpGuide))) { handbuchOeffnen(); }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::GuideHint)); }
        // rc601 tour: die Tutorials im Programm - es zeigt selbst, was zu
        // klicken ist.
        if (ImGui::MenuItem(tr(Str::TourMenu))) { tourFensterOeffnen(); }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::TourMenuHint)); }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::HelpAbout))) {
            g_app->aboutOpen = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::HelpAboutHint)); }   // H1
        if (ImGui::MenuItem(tr(Str::ChangelogMenu))) {
            g_app->changelogOpen = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::ChangelogMenuHint)); }   // H1
        ImGui::Separator();
        // Auto-Updater: von Hand nachsehen, und ob es beim Start geschehen soll.
        if (ImGui::MenuItem(tr(Str::UpdMenu))) {
            updater::pruefen(false);
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::UpdMenuHint)); }   // H1
        if (ImGui::MenuItem(tr(Str::UpdAutoCheck), nullptr, g_app->settings.updateCheck)) {
            g_app->settings.updateCheck = !g_app->settings.updateCheck;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::UpdAutoCheckHint)); }   // H1
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

// Die Auswahl im lebenden Dokument aufheben.
//
// Gebraucht an drei Stellen: nochmal auf dieselbe Zeile klicken, in die
// Leere unter dem Baum klicken, und der Fokuswechsel in ein Feld ohne
// Auswahl. Der Kamerabalken haengt daran mit, deshalb auch selectedKey.
void clearSelection() {
    g_app->selection.clear();
    g_app->selectedPath.clear();
    g_app->selected = -1;
}

