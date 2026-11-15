// selbsttest_ap2.inc - AP0 + AP2 (MBENTWURF Teil B, 11.11.): Tooltip der Modusleiste, Szenen-Explorer (wie 3ds Max) /
// Outliner (wie Blender) des Model-Tabs (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap2 | ap2foto)
//
//   ap2       je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Abschnitt 4 und 14)
//             eine Pruefung "AP2 X<nr>" bzw. "AP0 X26": Ort, Element, Bedienung - im Modus wie 3ds Max und wie Blender.
//             Gelesen wird die Lage der gezeichneten Zeilen (modellExplorer().lage: Pfeil, Auge, Symbol, Name, Schneeflocke).
//   ap2foto   nur Fotos (BHED_FOTOS=<Ordner>): beide Bedienmodi, nach Hierarchie / nach Ebenen, Knochen offen, Sequenzen,
//             Filter; fuer Sprachen, helles/dunkles Thema und kleine Fenster.
//   BHED_AP2_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP2_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP2_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_mbefoto.inc).
namespace ap2 {

using mbefoto::inFenster;
using mbefoto::kindFenster;

ModellExplorerZustand& ex() { return modellExplorer(); }
float em() { return ImGui::GetFontSize(); }

const ExplorerLage* zeile(const std::string& name, int art = -1) {
    for (const ExplorerLage& l : ex().lage) {
        if (l.name == name && (art < 0 || static_cast<int>(l.art) == art)) { return &l; }
    }
    return nullptr;
}

const ExplorerLage* zeileArt(ExplorerArt art, int index = -1) {
    for (const ExplorerLage& l : ex().lage) {
        if (l.art == art && (index < 0 || l.index == index)) { return &l; }
    }
    return nullptr;
}

// Der Ort in einer Zeile: 0 Name, 1 Auge, 2 Schneeflocke/Pfeil "waehlbar", 3 Aufklapp-Pfeil. (-1, -1), wenn es ihn nicht gibt.
std::function<ImVec2()> ort(const std::string& name, int was, int art = -1) {
    return [=] {
        const ExplorerLage* l = zeile(name, art);
        if (l == nullptr) { return ImVec2{-1.0F, -1.0F}; }
        const float y = (l->y0 + l->y1) * 0.5F;
        const float h = em() * 0.5F;
        float x = -1.0F;
        switch (was) {
            case 1: x = l->xAuge; break;
            case 2: x = l->xFrost; break;
            case 3: x = l->xPfeil; break;
            default: return l->xName < 0.0F ? ImVec2{-1.0F, -1.0F} : ImVec2{l->xName + std::max(std::min(em(), (l->xNameEnde - l->xName) * 0.5F), 2.0F), y};
        }
        return x < 0.0F ? ImVec2{-1.0F, -1.0F} : ImVec2{x + h, y};
    };
}

// TEST1411c: die sichtbare Flaeche der Sequenzliste unter ihrem festen Kopf (TableSetupScrollFreeze(0, 1)). ImGui setzt BgClipRect der
// Tabelle beim Ende der festen Zeilen auf den rollbaren Koerper; der Wert bleibt bis zum naechsten BeginTable stehen. Leer ohne Liste.
ImRect seqKoerper() {
    const ImGuiWindow* w = kindFenster("##t1eseq");
    for (int i = 0; w != nullptr && i < GImGui->Tables.GetMapSize(); ++i) {
        const ImGuiTable* tb = GImGui->Tables.TryGetMapData(i);
        if (tb != nullptr && tb->InnerWindow == w) { return tb->BgClipRect; }
    }
    return ImRect{};
}

bool tooltipRect(ImRect& r) {
    for (ImGuiWindow* w : GImGui->Windows) {
        if ((w->Flags & ImGuiWindowFlags_Tooltip) != 0 && w->Active && !w->Hidden) {
            r = w->Rect();
            return true;
        }
    }
    return false;
}

// Ein Element des Explorers ausserhalb des Baums (Umschalter, Filter).
const Element* imKopf(const std::string& label) {
    for (const Element& e : g_elemente) {
        if (e.label == label && e.innen.find("mtexplorer") != std::string::npos && e.innen.find("mtbaum") == std::string::npos) { return &e; }
    }
    return nullptr;
}

std::function<ImVec2()> kopfOrt(Str s) {
    return [=] {
        const Element* e = imKopf(tr(s));
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    };
}

// Ein Knopf der Symbolreihe des Filters ("Filter: Tags").
std::function<ImVec2()> filterOrt(Str art) {
    return [=] {
        const Element* e = imKopf(std::string(tr(Str::MbXFilter)) + ": " + tr(art));
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    };
}

bool menuePunkt(Str s) { return finde(tr(s), "", 0, false) != nullptr; }

// Ein Element im Baum (Spaltenkopf) nach seiner Beschriftung.
std::function<ImVec2()> baumOrt(const std::string& label) {
    return [=] {
        const Element* e = inFenster(label, "mtbaum");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    };
}

void melde(const std::string& z) { diag::detail("Knopftest: AP2 " + z); }

// Die Maus auf einen Ort stellen und warten; meldet, wann (in ImGui-Zeit) der erste Tooltip erschien und wo.
struct TooltipProbe {
    double start = 0.0;
    double erschienen = -1.0;
    ImRect rect;
    ImVec2 maus;
};

Schritt mausProbe(const std::string& was, std::function<ImVec2()> wo, std::shared_ptr<TooltipProbe> p, double dauer = 1.3) {
    return {"AP2 Maus auf " + was, [=](int b) {
                if (b == 0) {
                    // erst weg von allem, dann im naechsten Bild auf das Ziel (ein stehender Tooltip zaehlt nicht)
                    setzeMaus(ImVec2{-50.0F, -50.0F});
                    *p = TooltipProbe{};
                    return false;
                }
                if (b < 4) { return false; }
                if (b == 4) {
                    p->maus = wo();
                    setzeMaus(p->maus);
                    p->start = ImGui::GetTime();
                    return false;
                }
                ImRect r;
                if (p->erschienen < 0.0 && tooltipRect(r)) {
                    p->erschienen = ImGui::GetTime() - p->start;
                    p->rect = r;
                }
                if (ImGui::GetTime() - p->start < dauer) { return false; }
                if (tooltipRect(r)) { p->rect = r; }
                char z[260];
                std::snprintf(z, sizeof z, "Maus auf %s (%.0f/%.0f): Tooltip nach %.3f s, %.0f/%.0f-%.0f/%.0f, Bildzeit %.3f s", was.c_str(), static_cast<double>(p->maus.x),
                              static_cast<double>(p->maus.y), p->erschienen, static_cast<double>(p->rect.Min.x), static_cast<double>(p->rect.Min.y),
                              static_cast<double>(p->rect.Max.x), static_cast<double>(p->rect.Max.y), static_cast<double>(ImGui::GetIO().DeltaTime));
                melde(z);
                return true;
            }};
}

Schritt mausWeg() {
    return tu("AP2: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); });
}

int knochen(const char* name) { return boneIndex(g_app->anim, name); }

// Der Titel des Andockfelds der linken Spalte (Text vor "###").
std::string spaltenTitel() {
    for (const ImGuiWindow* w : GImGui->Windows) {
        const std::string n = w->Name;
        const std::size_t p = n.find("###dock.modell.animationen");
        if (w->WasActive && p != std::string::npos && n.find('/') == std::string::npos) { return n.substr(0, p); }
    }
    return "?";
}

void zuruecksetzen() {
    ModellTab& t = g_app->mtab;
    ModellExplorerZustand& z = ex();
    z.art = 0;
    for (bool& f : z.filterAus) { f = false; }
    z.knochenFlach = false;
    z.sortierung = 0;
    z.torsoSekundaer = false;
    z.seqGruppe = 0;
    z.infoOffen = false;
    t.suche[0] = 0;
    t.auswahl.clear();
    t.knochenWahl = -1;
    t.tagWahl = -1;
    t.g2 = modell::G2Zustand{};
    t.hautAus.clear();
    modellAllesEinblenden();
    modellExplorerVorgabe();
    g_app->modelDirty = true;
}

// --- AP0: der Tooltip der Modusleiste (X26, Abschnitt 14) ---------------------------------------------------------------------

void tooltipSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto p = std::make_shared<TooltipProbe>();
    const auto events = [] {
        const Element* e = inFenster(tr(Str::ViewEvents), "modebar");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    };
    add(mausProbe("Events (Modusleiste)", events, p));
    add(foto("ap0_tooltip_events"));
    add(pruefe("AP0 X26: der Tooltip der Modusleiste erscheint erst nach der ueblichen Verzoegerung (nicht beim Ueberfahren)", [p] { return p->erschienen >= 0.12; }));
    add(pruefe("AP0 X26: er haengt am Knopf - rechts neben der Leiste, auf Hoehe des Knopfs, hoechstens 20 Zeichenhoehen breit", [p] {
        const Element* e = inFenster(tr(Str::ViewEvents), "modebar");
        const ImGuiWindow* leiste = kindFenster("modebar");
        if (e == nullptr || leiste == nullptr || p->erschienen < 0.0) { return false; }
        return p->rect.Min.x >= leiste->Rect().Max.x - 1.0F && p->rect.Min.y >= e->rect.Min.y - 1.0F && p->rect.Min.y <= e->rect.Max.y + 1.0F && p->rect.GetWidth() <= em() * 20.0F;
    }));
    add(pruefe("AP0 X26: er liegt nicht ueber der Zeile \"By hierarchy / By layers\" des Explorers", [p] {
        const Element* h = imKopf(tr(Str::MbNachHierarchie));
        if (h == nullptr || p->erschienen < 0.0) { return false; }
        return p->rect.Min.y >= h->rect.Max.y - 1.0F || p->rect.Max.y <= h->rect.Min.y + 1.0F;
    }));
    auto q = std::make_shared<TooltipProbe>();
    add(mausProbe("By hierarchy", kopfOrt(Str::MbNachHierarchie), q));
    add(foto("ap0_tooltip_hierarchie"));
    add(pruefe("AP0 X26: \"By hierarchy\" hat einen eigenen Tooltip, er liegt unter dem Knopf", [q] {
        const Element* h = imKopf(tr(Str::MbNachHierarchie));
        return h != nullptr && q->erschienen >= 0.0 && q->rect.Min.y >= h->rect.Min.y;
    }));
    auto r = std::make_shared<TooltipProbe>();
    add(mausProbe("By layers", kopfOrt(Str::MbNachEbenen), r));
    add(pruefe("AP0 X26: \"By layers\" hat einen eigenen Tooltip", [r] { return r->erschienen >= 0.0; }));
    add(mausWeg());
    add(pause(0.2));
}

// --- AP2 wie 3ds Max ----------------------------------------------------------------------------------------------------------------

void maxSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP2: wie 3ds Max, Ansehen/Pruefen, Explorer auf Anfang", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        zuruecksetzen();
    }));
    add(pause(0.6));
    add(foto("ap2_max_hierarchie"));
    // X1
    add(pruefe("AP2 X1: ein Baum ueber die ganze Hoehe der Spalte, Titel \"Scene Explorer\", keine eigene Sequenzliste darunter", [] {
        const ImGuiWindow* w = kindFenster("mtexplorer");
        const ImGuiWindow* liste = kindFenster("animliste");
        const ImGuiWindow* band = kindFenster("toolband");
        if (w == nullptr) { return false; }
        const float unten = band != nullptr ? band->Rect().Min.y : ImGui::GetIO().DisplaySize.y;
        char z[200];
        std::snprintf(z, sizeof z, "X1: Titel '%s', Explorer %.0f breit (%.1f Zeichenhoehen), unten %.0f, Band ab %.0f, Liste %d", spaltenTitel().c_str(),
                      static_cast<double>(w->Size.x), static_cast<double>(w->Size.x / em()), static_cast<double>(w->Rect().Max.y), static_cast<double>(unten), liste != nullptr ? 1 : 0);
        melde(z);
        return liste == nullptr && spaltenTitel() == tr(Str::MbExplorer) && w->Rect().Max.y >= unten - em() * 2.5F;
    }));
    add(pruefe("AP2 X1: die Spalte ist ab Werk so breit wie im Entwurf (mindestens 18 Zeichenhoehen)", [] {
        const ImGuiWindow* w = kindFenster("mtexplorer");
        return w != nullptr && w->Size.x >= em() * 18.0F;
    }));
    // X3
    add(pruefe("AP2 X3: Umschalter \"By hierarchy / By layers\" (wie 3ds Max)", [] {
        return imKopf(tr(Str::MbNachHierarchie)) != nullptr && imKopf(tr(Str::MbNachEbenen)) != nullptr && imKopf(tr(Str::MbXSammlungen)) == nullptr;
    }));
    // X5, X6
    add(pruefe("AP2 X5: oben das Modell mit Ordner (kyle/model.glm)", [] {
        const std::string p = g_app->modelPath;
        const std::size_t a = p.rfind('/');
        const std::size_t b = a == std::string::npos ? std::string::npos : p.rfind('/', a - 1);
        const std::string kurz = b == std::string::npos ? p : p.substr(b + 1);
        const ExplorerLage* l = zeile(kurz, static_cast<int>(ExplorerArt::Modell));
        melde("X5: erwartet '" + kurz + "', Zeilen " + std::to_string(ex().lage.size()));
        return l != nullptr && l->tiefe == 0;
    }));
    add(pruefe("AP2 X6: Flaechen mit Hierarchie - torso steht eine Ebene unter seinen Eltern", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        const int t = mb1::teil("torso");
        if (t < 0 || in.teile[static_cast<std::size_t>(t)].eltern < 0) { return false; }
        const ExplorerLage* a = zeile("torso", static_cast<int>(ExplorerArt::Flaeche));
        const ExplorerLage* e = zeile(in.teile[static_cast<std::size_t>(in.teile[static_cast<std::size_t>(t)].eltern)].name, static_cast<int>(ExplorerArt::Flaeche));
        return a != nullptr && e != nullptr && a->tiefe == e->tiefe + 1 && a->xSymbol > e->xSymbol;
    }));
    // X12: Pfeil, Auge, Symbol, Name direkt nebeneinander; kleiner Einzug; nichts laeuft rechts hinaus
    add(pruefe("AP2 X12: Pfeil, Auge, Symbol und Name stehen direkt nebeneinander (je hoechstens 1,3 Zeichenhoehen), Einzug je Ebene hoechstens eine Zeichenhoehe", [] {
        int n = 0;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Flaeche) { continue; }
            ++n;
            if (l.xAuge < 0.0F || l.xSymbol < 0.0F || l.xName < 0.0F) { return false; }
            if (l.xSymbol - l.xAuge <= 0.0F || l.xSymbol - l.xAuge > em() * 1.3F || l.xName - l.xSymbol <= 0.0F || l.xName - l.xSymbol > em() * 1.4F) { return false; }
            if (l.kinder && (l.xPfeil < 0.0F || l.xAuge - l.xPfeil <= 0.0F || l.xAuge - l.xPfeil > em() * 1.3F)) { return false; }
        }
        char z[120];
        std::snprintf(z, sizeof z, "X12: %d Flaechenzeilen, Einzug %.1f (%.2f Zeichenhoehen)", n, static_cast<double>(ex().einzug), static_cast<double>(ex().einzug / em()));
        melde(z);
        return n > 5 && ex().einzug > em() * 0.3F && ex().einzug <= em() * 1.01F;
    }));
    add(pruefe("AP2 X12: kein waagerechtes Rollen, jede Zeile beginnt im Feld und kein Name laeuft rechts hinaus (Knochen zu)", [] {
        const ImGuiWindow* w = kindFenster("mtbaum");
        if (w == nullptr || ex().lage.empty()) { return false; }
        int gekuerzt = 0;
        for (const ExplorerLage& l : ex().lage) {
            gekuerzt += l.gekuerzt ? 1 : 0;
            if (l.xName < 0.0F) { continue; }
            if (l.xName + em() * 2.0F > w->InnerRect.Max.x || l.xNameEnde > l.xZelleEnde + 0.5F) { return false; }
        }
        melde("X12: " + std::to_string(ex().lage.size()) + " Zeilen im Bild, " + std::to_string(gekuerzt) + " mit gekuerztem Namen, " + std::to_string(ex().zeilen) + " im Baum");
        return w->ScrollMax.x <= 0.0F;
    }));
    // X7: Zustandszeichen
    add(tu("AP2 X7: torso aus", [] { modellFlaecheSchalten(mb1::teil("torso"), modell::kG2Aus); }));
    add(pause(0.2));
    add(pruefe("AP2 X7: Zeichen \"aus\" an torso", [] {
        const ExplorerLage* l = zeile("torso", static_cast<int>(ExplorerArt::Flaeche));
        return l != nullptr && (l->marke & 3) == 1;
    }));
    add(tu("AP2 X7: torso aus ohne Kinder", [] { modellFlaecheSchalten(mb1::teil("torso"), modell::kG2Aus | modell::kG2OhneKinder); }));
    add(pause(0.2));
    add(foto("ap2_max_zustand"));
    add(pruefe("AP2 X7: Zeichen \"aus + ohne Kinder\" an torso", [] {
        const ExplorerLage* l = zeile("torso", static_cast<int>(ExplorerArt::Flaeche));
        return l != nullptr && (l->marke & 3) == 2;
    }));
    add(tu("AP2 X7: torso an und Wurzel", [] {
        modellFlaecheSchalten(mb1::teil("torso"), 0U);
        modellWurzelSetzen(mb1::teil("torso"));
    }));
    add(pause(0.2));
    add(pruefe("AP2 X7: Zeichen \"Wurzel\" an torso, torso selbst \"an\"", [] {
        const ExplorerLage* l = zeile("torso", static_cast<int>(ExplorerArt::Flaeche));
        return l != nullptr && (l->marke & 8) != 0 && (l->marke & 3) == 0;
    }));
    // X19: Rechtsklick Flaeche
    add(klickAn("AP2 X19: Rechtsklick torso", ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), 1));
    add(pause(0.3));
    add(foto("ap2_max_menue_flaeche"));
    add(pruefe("AP2 X19: Rechtsklick Flaeche - Info, an/aus/aus ohne Kinder, als Wurzel, Wurzel loesen, Ursprungszustand", [] {
        return menuePunkt(Str::MbXInfo) && menuePunkt(Str::MbFlAn) && menuePunkt(Str::MbFlAus) && menuePunkt(Str::MbFlAusOhneKinder) && menuePunkt(Str::MbFlWurzel) &&
               menuePunkt(Str::MbXWurzelLoesen) && menuePunkt(Str::MbFlUrsprung);
    }));
    add(klick(tr(Str::MbXWurzelLoesen), ""));
    add(pause(0.2));
    add(pruefe("AP2 X19: \"Wurzel loesen\" loest nur die Wurzel", [] { return g_app->mtab.g2.wurzel <= 0; }));
    add(mbefoto::popupsZu());
    // X18: Info im Menue und als Tooltip
    add(klickAn("AP2 X18: Rechtsklick torso", ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), 1));
    add(pause(0.3));
    add(klick(tr(Str::MbXInfo), ""));
    add(pause(0.3));
    add(foto("ap2_max_info"));
    add(pruefe("AP2 X18: \"Info\" nennt die Flaeche mit Ecken, Dreiecken, Shader, Eltern und Status", [] {
        const std::string& i = ex().infoText;
        melde("X18: Info '" + i + "'");
        return ex().infoOffen && i.find("torso") != std::string::npos && i.find(tr(Str::MbXInfoDreiecke)) != std::string::npos &&
               i.find(tr(Str::MbXInfoShader)) != std::string::npos && i.find(tr(Str::MbXInfoEltern)) != std::string::npos;
    }));
    add(tu("AP2 X18: Info zu", [] { ex().infoOffen = false; }));
    add(mbefoto::popupsZu());
    add(pause(0.2));
    auto tip = std::make_shared<TooltipProbe>();
    add(mausProbe("torso (Zeile)", ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), tip));
    add(pruefe("AP2 X18/X24: der Tooltip der Zeile nennt den vollen Namen und die Info", [tip] {
        return tip->erschienen >= 0.0 && ex().tooltip.find("torso") != std::string::npos && ex().tooltip.find(tr(Str::MbXInfoDreiecke)) != std::string::npos;
    }));
    add(mausWeg());
    // X25: Auge je Zeile, nur dieses
    add(klickAn("AP2 X25: Auge von hips", ort("hips", 1, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.3));
    add(pruefe("AP2 X25: das Auge blendet hips und seine Kinder aus", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        const int h = mb1::teil("hips");
        const auto& k = in.teile[static_cast<std::size_t>(h)].kinder;
        return modellTeilVersteckt(h) && (k.empty() || modellTeilVersteckt(k.front()));
    }));
    add(klickAn("AP2 X25: Auge von hips zurueck", ort("hips", 1, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.2));
    add(ziehenAn("AP2 X25: Strg+Klick auf das Auge von torso", ort("torso", 1, static_cast<int>(ExplorerArt::Flaeche)), 0.0F, 0.0F, 0, ImGuiMod_Ctrl));
    add(pause(0.3));
    add(pruefe("AP2 X25: Strg+Klick zeigt nur torso (und seine Kinder)", [] { return !modellTeilVersteckt(mb1::teil("torso")) && modellTeilVersteckt(mb1::teil("hips")); }));
    add(tu("AP2 X25: alles einblenden", [] { modellAllesEinblenden(); }));
    // X13: Einfrieren
    add(pruefe("AP2 X13: Kappen (_off) sind ab Werk eingefroren, torso nicht", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        int kappen = 0;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (in.teile[i].istTag() || (in.teile[i].flaggen & modell::kG2Aus) == 0U) { continue; }
            ++kappen;
            if (!modellTeilEingefroren(static_cast<int>(i))) { return false; }
        }
        melde("X13: " + std::to_string(kappen) + " Kappen");
        return kappen > 0 && !modellTeilEingefroren(mb1::teil("torso"));
    }));
    add(klickAn("AP2 X13: Schneeflocke von torso", ort("torso", 2, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.3));
    add(tu("AP2 X13: Meldung leeren", [] {
        g_app->mtab.meldung.clear();
        g_app->mtab.auswahl.clear();
    }));
    add(klickAn("AP2 X13: Klick auf die eingefrorene Zeile torso", ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.3));
    add(foto("ap2_max_eingefroren"));
    // ENTSCHEID3A (B7, shank 12.11.): wie im Scene Explorer von 3ds Max waehlt der Klick in der Liste auch die eingefrorene Zeile (vorher:
    // nicht gewaehlt, Meldung); in der Ansicht geht der Klick weiter hindurch (unten). Ausfuehrlich in selbsttest_ent3a.inc.
    add(pruefe("AP2 X13: eingefroren - der Klick im Baum waehlt torso wie der Scene Explorer von 3ds Max, torso bleibt eingefroren", [] {
        const ModellTab& t = g_app->mtab;
        melde("X13: Meldung '" + t.meldung + "'");
        return modellTeilEingefroren(mb1::teil("torso")) && t.auswahl.size() == 1U && t.auswahl.front() == mb1::teil("torso");
    }));
    // In der Ansicht (ENTSCHEID1, shank 11.11.: "hindurch auf die flaeche dahinter"): wie 3ds Max nimmt eine eingefrorene Flaeche am
    // Treffertest nicht teil - der Klick waehlt die Flaeche dahinter (Autodesk-Hilfe "Freezing and Unfreezing Objects": eingefrorene
    // Objekte "cannot be selected"). Gesucht wird eine Stelle, an der zwei verschiedene Flaechen hintereinander liegen, und eine, an
    // der nur eine Flaeche liegt.
    struct Stelle {
        ImVec2 p{-1.0F, -1.0F};
        int vorn = -1;
        int hinten = -1;
        ImVec2 allein{-1.0F, -1.0F};
        int einzige = -1;
    };
    auto st = std::make_shared<Stelle>();
    add(tu("AP2 X13: torso geloest, Auswahl leer; Stellen suchen (zwei Flaechen hintereinander, eine Flaeche allein)", [st] {
        modellTeilEinfrieren(mb1::teil("torso"), false, false);
        ModellTab& t = g_app->mtab;
        t.auswahl.clear();
        *st = Stelle{};
        const ModellAnsicht& a = t.ansichten[static_cast<std::size_t>(std::clamp(t.aktiv, 0, 3))];
        const float schritt = std::max(em() * 0.5F, 4.0F);
        for (float y = a.lo[1] + em() * 3.0F; y < a.lo[1] + a.groesse[1] - em() * 2.0F; y += schritt) {
            for (float x = a.lo[0] + em(); x < a.lo[0] + a.groesse[0] - em(); x += schritt) {
                const std::vector<int> f = modellFlaechenUnter(a, ImVec2{x, y});
                if (f.empty() || modellTeilEingefroren(f[0])) { continue; }
                if (st->vorn < 0 && f.size() >= 2 && f[1] != f[0] && !modellTeilEingefroren(f[1])) {
                    // die hintere bleibt sichtbar, wenn man die vordere ausblendet (sie ist kein Kind von ihr) - fuer die Gegenprobe
                    modellTeilAuge(f[0], false);
                    const bool bleibt = !modellTeilVersteckt(f[1]);
                    modellTeilAuge(f[0], false);
                    if (bleibt && !modellTeilVersteckt(f[0])) {
                        st->p = ImVec2{x, y};
                        st->vorn = f[0];
                        st->hinten = f[1];
                    }
                }
                if (st->einzige < 0 && std::all_of(f.begin(), f.end(), [&](int i) { return i == f[0]; })) {
                    st->allein = ImVec2{x, y};
                    st->einzige = f[0];
                }
            }
        }
        const auto& teile = t.dok.inhalt().teile;
        const auto nameVon = [&](int i) { return i >= 0 ? teile[static_cast<std::size_t>(i)].name : std::string("-"); };
        melde("X13: Stelle " + std::to_string(static_cast<int>(st->p.x)) + "/" + std::to_string(static_cast<int>(st->p.y)) + " vorn " + nameVon(st->vorn) + ", dahinter " +
              nameVon(st->hinten) + "; allein " + nameVon(st->einzige));
    }));
    add(klickAn("AP2 X13: Klick in die Ansicht (nichts eingefroren)", [st] { return st->p; }));
    add(pause(0.3));
    add(pruefe("AP2 X13: ohne Einfrieren waehlt der Klick die vordere Flaeche", [st] {
        const ModellTab& t = g_app->mtab;
        return st->vorn >= 0 && t.auswahl.size() == 1 && t.auswahl.front() == st->vorn;
    }));
    add(tu("AP2 X13: die vordere Flaeche einfrieren", [st] {
        modellTeilEinfrieren(st->vorn, true, false);
        g_app->mtab.auswahl.clear();
        g_app->mtab.meldung.clear();
    }));
    add(klickAn("AP2 X13: Klick in die Ansicht auf die eingefrorene Flaeche", [st] { return st->p; }));
    add(pause(0.3));
    add(pruefe("AP2 X13: eingefroren - der Klick geht hindurch und waehlt die Flaeche dahinter (wie 3ds Max)", [st] {
        const ModellTab& t = g_app->mtab;
        melde("X13: gewaehlt " + std::to_string(t.auswahl.size()) + (t.auswahl.empty() ? std::string() : " (" + std::to_string(t.auswahl.front()) + ")") + ", soll " +
              std::to_string(st->hinten) + ", Meldung '" + t.meldung + "'");
        return st->hinten >= 0 && t.auswahl.size() == 1 && t.auswahl.front() == st->hinten && modellTeilEingefroren(st->vorn);
    }));
    add(tu("AP2 X13: Gegenprobe - die vordere aufgetaut und ausgeblendet", [st] {
        modellTeilEinfrieren(st->vorn, false, false);
        modellTeilAuge(st->vorn, false);
        g_app->mtab.auswahl.clear();
    }));
    add(klickAn("AP2 X13: Klick auf dieselbe Stelle", [st] { return st->p; }));
    add(pause(0.3));
    add(pruefe("AP2 X13: Gegenprobe - mit ausgeblendeter vorderer Flaeche waehlt der Klick dieselbe Flaeche dahinter", [st] {
        const ModellTab& t = g_app->mtab;
        return st->hinten >= 0 && t.auswahl.size() == 1 && t.auswahl.front() == st->hinten;
    }));
    add(tu("AP2 X13: einblenden; die einzige Flaeche an der zweiten Stelle einfrieren", [st] {
        modellAllesEinblenden();
        modellTeilEinfrieren(st->einzige, true, false);
        g_app->mtab.auswahl.clear();
        g_app->mtab.meldung.clear();
    }));
    add(klickAn("AP2 X13: Klick auf die Stelle, an der nur die eingefrorene Flaeche liegt", [st] { return st->allein; }));
    add(pause(0.3));
    add(pruefe("AP2 X13: eingefroren und nichts dahinter - der Klick waehlt nichts und meldet es", [st] {
        const ModellTab& t = g_app->mtab;
        melde("X13: Meldung '" + t.meldung + "', gewaehlt " + std::to_string(t.auswahl.size()));
        if (st->einzige < 0) { return false; }
        return t.auswahl.empty() && t.meldung.find(t.dok.inhalt().teile[static_cast<std::size_t>(st->einzige)].name) != std::string::npos;
    }));
    add(tu("AP2 X13: fuer den Rahmen nur torso eingefroren (dazu die Kappen ab Werk)", [st] {
        modellTeilEinfrieren(st->einzige, false, false);
        modellTeilEinfrieren(mb1::teil("torso"), true, false);
        g_app->mtab.auswahl.clear();
    }));
    add(Schritt{"AP2 X13: Rahmen ziehen", [](int b) {
                    const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
                    ImGuiIO& io = ImGui::GetIO();
                    const ImVec2 p0{a.lo[0] + 12.0F, a.lo[1] + em() * 3.0F};
                    const ImVec2 p1{a.lo[0] + a.groesse[0] - 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
                    if (b == 0) { setzeMaus(p0); }
                    if (b == 3) { io.AddMouseButtonEvent(0, true); }
                    if (b >= 5 && b < 15) {
                        const float f = static_cast<float>(b - 4) / 10.0F;
                        setzeMaus(ImVec2{p0.x + (p1.x - p0.x) * f, p0.y + (p1.y - p0.y) * f});
                    }
                    if (b == 17) { io.AddMouseButtonEvent(0, false); }
                    return b >= 21;
                }});
    add(pruefe("AP2 X13: eingefroren - der Rahmen nimmt hips, aber nicht torso", [] {
        const ModellTab& t = g_app->mtab;
        melde("X13: Rahmen waehlt " + std::to_string(t.auswahl.size()) + " Flaechen");
        return std::find(t.auswahl.begin(), t.auswahl.end(), mb1::teil("hips")) != t.auswahl.end() &&
               std::find(t.auswahl.begin(), t.auswahl.end(), mb1::teil("torso")) == t.auswahl.end();
    }));
    add(klickAn("AP2 X13: Schneeflocke von torso zurueck", ort("torso", 2, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.2));
    add(klickAn("AP2 X13: Klick auf torso", ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.2));
    add(pruefe("AP2 X13: geloest - der Klick waehlt torso wieder", [] { return g_app->mtab.auswahl == std::vector<int>{mb1::teil("torso")}; }));
    // X16: Umschalt+Klick = Bereich
    add(klickAn("AP2 X16: Klick auf hips", ort("hips", 0, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.2));
    add(ziehenAn("AP2 X16: Umschalt+Klick auf torso", ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), 0.0F, 0.0F, 0, ImGuiMod_Shift));
    add(pause(0.3));
    add(foto("ap2_max_bereich"));
    // ENTSCHEID3A (B7): wie im Scene Explorer von 3ds Max sind eingefrorene Zeilen in der Liste waehlbar - der Bereich nimmt sie mit
    // (vorher: ohne Eingefrorene, z. B. die Kappen ab Werk); wie Blender (Pfeil "waehlbar" aus) weiter ohne
    add(pruefe("AP2 X16: Umschalt+Klick waehlt alle Flaechenzeilen von hips bis torso (wie 3ds Max auch die eingefrorenen)", [] {
        const ModellTab& t = g_app->mtab;
        const ExplorerLage* a = zeile("hips", static_cast<int>(ExplorerArt::Flaeche));
        const ExplorerLage* b = zeile("torso", static_cast<int>(ExplorerArt::Flaeche));
        if (a == nullptr || b == nullptr) {
            melde(std::string("X16: Zeile nicht im Bild - hips ") + (a != nullptr ? "da" : "fehlt") + ", torso " + (b != nullptr ? "da" : "fehlt"));
            return false;
        }
        const float y0 = std::min(a->y0, b->y0);
        const float y1 = std::max(a->y0, b->y0);
        int soll = 0;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Flaeche || l.y0 < y0 - 0.5F || l.y0 > y1 + 0.5F || (l.eingefroren && modellBlender())) { continue; }
            ++soll;
            if (std::find(t.auswahl.begin(), t.auswahl.end(), l.index) == t.auswahl.end()) {
                melde("X16: Zeile " + std::to_string(l.index) + " im Bereich nicht gewaehlt (gewaehlt " + std::to_string(t.auswahl.size()) + ")");
                return false;
            }
        }
        melde("X16: Bereich " + std::to_string(soll) + " Zeilen, gewaehlt " + std::to_string(t.auswahl.size()));
        return soll >= 2 && static_cast<int>(t.auswahl.size()) == soll;
    }));
    // X14: Spaltenkoepfe, sortieren
    add(tu("AP2 X14: zu den Tags", [] { modellExplorerZeigen(ExplorerArt::KopfTags, 0); }));
    add(pause(0.3));
    add(pruefe("AP2 X14: Spaltenkoepfe \"Name\" und \"Frozen\"", [] {
        return ex().koepfe && inFenster("##mtkopfname", "mtbaum") != nullptr && inFenster("##frostkopf", "mtbaum") != nullptr;
    }));
    const auto tagsSortiert = [](int richtung) {
        std::vector<std::string> n;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art == ExplorerArt::Tag) { n.push_back(l.name); }
        }
        if (n.size() < 2) { return false; }
        const auto klein = [](std::string x) {
            for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            return x;
        };
        for (std::size_t i = 1; i < n.size(); ++i) {
            const int c = klein(n[i - 1]).compare(klein(n[i]));
            if ((richtung > 0 && c > 0) || (richtung < 0 && c < 0)) { return false; }
        }
        return true;
    };
    add(klickAn("AP2 X14: Spaltenkopf Name", baumOrt("##mtkopfname")));
    add(pause(0.3));
    add(pruefe("AP2 X14: Klick auf \"Name\" sortiert aufsteigend", [=] { return ex().sortierung == 1 && tagsSortiert(1); }));
    add(klickAn("AP2 X14: Spaltenkopf Name", baumOrt("##mtkopfname")));
    add(pause(0.3));
    add(foto("ap2_max_sortiert"));
    add(pruefe("AP2 X14: noch ein Klick sortiert absteigend", [=] { return ex().sortierung == 2 && tagsSortiert(-1); }));
    add(klickAn("AP2 X14: Spaltenkopf Name", baumOrt("##mtkopfname")));
    add(pause(0.3));
    add(pruefe("AP2 X14: der dritte Klick stellt die Reihenfolge der Hierarchie wieder her (Tags wie in der Datei)", [] {
        int letzter = -1;
        int n = 0;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Tag) { continue; }
            if (l.index < letzter) { return false; }
            letzter = l.index;
            ++n;
        }
        return ex().koepfe && ex().sortierung == 0 && n >= 2;
    }));
    // X8, X17, X20: Tags mit Anbauten
    add(tu("AP2 X8: zum Tag *r_hand (das Modell hat 44 Tags)", [] { modellExplorerZeigen(ExplorerArt::Tag, mb1::teil("*r_hand")); }));
    add(pause(0.4));
    add(pruefe("AP2 X8: der Anbau steht als eigene Zeile direkt unter seinem Tag, eine Ebene tiefer, mit Auge", [] {
        const ExplorerLage* t = zeile("*r_hand", static_cast<int>(ExplorerArt::Tag));
        const ExplorerLage* a = zeileArt(ExplorerArt::Anbau);
        if (t == nullptr || a == nullptr) { return false; }
        melde("X8: Anbau '" + a->name + "'");
        return std::fabs(a->y0 - t->y1) < 2.0F && a->tiefe == t->tiefe + 1 && a->xAuge >= 0.0F;
    }));
    add(tu("AP2 X17: Tags und Knochen in der Ansicht aus", [] {
        g_app->mtab.an.tags = false;
        g_app->modellKnochen = false;
    }));
    add(klickAn("AP2 X17: Klick auf *r_hand", ort("*r_hand", 0, static_cast<int>(ExplorerArt::Tag))));
    add(pause(0.3));
    add(pruefe("AP2 X17: der Klick waehlt den Tag und blendet die Tags ein", [] { return g_app->mtab.tagWahl == mb1::teil("*r_hand") && g_app->mtab.an.tags; }));
    add(klickAn("AP2 X20: Rechtsklick *r_hand", ort("*r_hand", 0, static_cast<int>(ExplorerArt::Tag)), 1));
    add(pause(0.3));
    add(foto("ap2_max_menue_tag"));
    add(pruefe("AP2 X20: Rechtsklick Tag - Info, Modell anbauen, Anbauten loesen", [] { return menuePunkt(Str::MbXInfo) && menuePunkt(Str::MbAnbauen) && menuePunkt(Str::MbLoesen); }));
    add(mbefoto::popupsZu());
    add(pause(0.2));
    // X10: Knochen
    add(tu("AP2 X10: zu den Knochen (pelvis)", [] { modellExplorerZeigen(ExplorerArt::Knochen, knochen("pelvis")); }));
    add(pause(0.4));
    add(pruefe("AP2 X10: Knochen als Hierarchie - pelvis eine Ebene unter seinen Eltern, Kopf nennt die GLA", [] {
        const int p = knochen("pelvis");
        if (p < 0) { return false; }
        const int e = g_app->anim.bones[static_cast<std::size_t>(p)].parent;
        const ExplorerLage* a = zeileArt(ExplorerArt::Knochen, p);
        const ExplorerLage* b = e >= 0 ? zeileArt(ExplorerArt::Knochen, e) : nullptr;
        const ExplorerLage* k = zeileArt(ExplorerArt::KopfKnochen);
        if (k != nullptr) { melde("X10: Kopf '" + k->name + "'"); }
        return a != nullptr && b != nullptr && a->tiefe == b->tiefe + 1 && k != nullptr && k->name.find(".gla") != std::string::npos;
    }));
    add(klickAn("AP2 X17: Klick auf pelvis", [] {
        const ExplorerLage* l = zeileArt(ExplorerArt::Knochen, knochen("pelvis"));
        return l != nullptr ? ImVec2{l->xName + em(), (l->y0 + l->y1) * 0.5F} : ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.3));
    add(pruefe("AP2 X17: der Klick waehlt den Knochen und blendet die Knochen ein", [] { return g_app->mtab.knochenWahl == knochen("pelvis") && g_app->modellKnochen; }));
    add(klickAn("AP2 X20: Rechtsklick pelvis", [] {
        const ExplorerLage* l = zeileArt(ExplorerArt::Knochen, knochen("pelvis"));
        return l != nullptr ? ImVec2{l->xName + em(), (l->y0 + l->y1) * 0.5F} : ImVec2{-1.0F, -1.0F};
    }, 1));
    add(pause(0.3));
    add(foto("ap2_max_menue_knochen"));
    add(pruefe("AP2 X20: Rechtsklick Knochen - Info, Modell anbauen, Start der Sekundaersequenz", [] {
        return menuePunkt(Str::MbXInfo) && menuePunkt(Str::MbAnbauen) && menuePunkt(Str::MbSekStartSetzen);
    }));
    add(mbefoto::popupsZu());
    add(tu("AP2 X10: Knochen flach", [] { ex().knochenFlach = true; }));
    add(pause(0.3));
    add(pruefe("AP2 X10: Schalter \"flach\" - alle Knochen auf einer Ebene", [] {
        int n = 0;
        int tiefe = -1;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Knochen) { continue; }
            ++n;
            if (tiefe >= 0 && l.tiefe != tiefe) { return false; }
            tiefe = l.tiefe;
        }
        return n > 3;
    }));
    add(tu("AP2 X12: Knochen als Hierarchie, alle offen, zum tiefsten", [] {
        ex().knochenFlach = false;
        modellExplorerKnochenAlleAuf(true);
        const GlaAnimation& a = g_app->anim;
        int tiefster = -1;
        int tiefe = -1;
        for (std::size_t b = 0; b < a.bones.size(); ++b) {
            int d = 0;
            for (int p = a.bones[b].parent; p >= 0 && d < 200; p = a.bones[static_cast<std::size_t>(p)].parent) { ++d; }
            if (d > tiefe) {
                tiefe = d;
                tiefster = static_cast<int>(b);
            }
        }
        modellExplorerZeigen(ExplorerArt::Knochen, tiefster);
        melde("X12: tiefster Knochen " + (tiefster >= 0 ? a.bones[static_cast<std::size_t>(tiefster)].name : std::string("?")) + " in Tiefe " + std::to_string(tiefe));
    }));
    add(pause(0.5));
    add(foto("ap2_max_knochen_offen"));
    add(pruefe("AP2 X12: alle Knochen offen - kein waagerechtes Rollen, auch die tiefste Zeile beginnt im Feld, das Auge steht beim Namen", [] {
        const ImGuiWindow* w = kindFenster("mtbaum");
        if (w == nullptr) { return false; }
        int n = 0;
        int tiefe = 0;
        int gekuerzt = 0;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Knochen) { continue; }
            ++n;
            tiefe = std::max(tiefe, l.tiefe);
            gekuerzt += l.gekuerzt ? 1 : 0;
            if (l.xName + em() * 2.0F > w->InnerRect.Max.x || l.xAuge < 0.0F || l.xName - l.xAuge > em() * 2.6F) { return false; }
        }
        char z[200];
        std::snprintf(z, sizeof z, "X12: %d Knochenzeilen im Bild, groesste Tiefe %d, Einzug %.2f Zeichenhoehen, %d Namen gekuerzt, Feld %.1f Zeichenhoehen breit", n, tiefe,
                      static_cast<double>(ex().einzug / em()), gekuerzt, static_cast<double>(w->InnerRect.GetWidth() / em()));
        melde(z);
        return n > 5 && tiefe >= 6 && w->ScrollMax.x <= 0.0F;
    }));
    add(tu("AP2: Knochen wieder zu", [] {
        modellExplorerKnochenAlleAuf(false);
        modellExplorerAufklappen(ExplorerArt::KopfKnochen, 0, false);
    }));
    // X9, X21: Sequenzen im Baum
    add(tu("AP2 X9: zur gewaehlten Sequenz", [] {
        animWaehlen(mbefoto::sequenz(), false);
        g_app->mtab.sekundaer = -1;
        modellExplorerZeigen(ExplorerArt::Sequenz, g_app->animIndex);
    }));
    add(pause(0.5));
    add(foto("ap2_max_sequenzen"));
    // TEST1411 E (shank 14.11.: "die sequencen sollten separat sein"): die Sequenzen stehen im eigenen Reiter des Explorers, nicht mehr als
    // Ast im Baum - Zeigen springt dorthin
    add(pruefe("AP2 X9: die Sequenzen stehen im Reiter Sequenzen des Explorers, die Primaersequenz traegt die Marke P", [] {
        const ExplorerLage* l = zeileArt(ExplorerArt::Sequenz, g_app->animIndex);
        if (l != nullptr) { melde("X9: Zeile '" + l->name + "', Marke " + std::to_string(l->marke) + ", " + std::to_string(ex().zeilen) + " Zeilen in der Liste"); }
        return l != nullptr && (l->marke & 1) != 0 && l->name == g_app->animList[static_cast<std::size_t>(g_app->animIndex)].name && kindFenster("animliste") == nullptr &&
               ex().art == 2;
    }));
    add(pruefe("AP2 X9: nur die sichtbaren Zeilen werden gezeichnet (Liste mit allen Sequenzen)", [] {
        return ex().zeilen == static_cast<int>(g_app->animList.size()) && ex().lage.size() < 200U;
    }));
    auto bilder = std::make_shared<int>(0);
    add(tu("AP2 X9: die Sequenz reicht hinter das Ende der GLA, als Sekundaer gesetzt", [bilder] {
        AnimEntry& e = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
        *bilder = e.numFrames;
        e.numFrames = g_app->anim.bildZahl() - e.firstFrame + 5;
        g_app->mtab.sekundaer = g_app->animIndex;
    }));
    add(pause(0.3));
    add(pruefe("AP2 X9: Marke S und Warnzeichen an der Sequenz", [] {
        const ExplorerLage* l = zeileArt(ExplorerArt::Sequenz, g_app->animIndex);
        return l != nullptr && (l->marke & 2) != 0 && (l->marke & 4) != 0;
    }));
    add(tu("AP2 X9: zurueck", [bilder] {
        g_app->animList[static_cast<std::size_t>(g_app->animIndex)].numFrames = *bilder;
        g_app->mtab.sekundaer = -1;
    }));
    const auto seqOrt = [] {
        const ExplorerLage* l = zeileArt(ExplorerArt::Sequenz, g_app->animIndex);
        return l != nullptr ? ImVec2{l->xName + em(), (l->y0 + l->y1) * 0.5F} : ImVec2{-1.0F, -1.0F};
    };
    add(klickAn("AP2 X21: Rechtsklick Sequenz", seqOrt, 1));
    add(pause(0.3));
    add(foto("ap2_max_menue_sequenz"));
    add(pruefe("AP2 X21/X9: Rechtsklick Sequenz - primaer, sekundaer, sekundaer aus, ins Skript, Namen kopieren", [] {
        return menuePunkt(Str::MbAlsPrimaer) && menuePunkt(Str::MbSekundaerAus) && menuePunkt(Str::MbInsSkriptMenue) && menuePunkt(Str::MbXSeqKopieren);
    }));
    add(mbefoto::popupsZu());
    add(pruefe("AP2 X9: \"Namen kopieren\" und \"Ins Skript\" stehen in \"Alle Aktionen suchen\"", [] {
        bool kop = false;
        bool ins = false;
        for (int i = 0; i < modellExplorerAktionZahl(); ++i) {
            kop = kop || std::string(modellExplorerAktionText(i)) == tr(Str::MbXAktSeqKopieren);
            ins = ins || std::string(modellExplorerAktionText(i)) == tr(Str::MbXAktSeqInsSkript);
        }
        return kop && ins;
    }));
    auto vorher = std::make_shared<int>(-1);
    auto torsoZiel = std::make_shared<int>(-1);
    add(tu("AP2 X17: Klick auf TORSO_ spielt als Sekundaer (Schalter wie ModView)", [vorher, torsoZiel] {
        ex().torsoSekundaer = true;
        *vorher = g_app->animIndex;
        int torso = -1;
        for (std::size_t i = 0; i < g_app->animList.size() && torso < 0; ++i) {
            if (g_app->animList[i].name.rfind("TORSO_", 0) == 0) { torso = static_cast<int>(i); }
        }
        *torsoZiel = torso;
        modellExplorerZeigen(ExplorerArt::Sequenz, torso);
    }));
    add(pause(0.4));
    add(klickAn("AP2 X17: Klick auf die TORSO_-Sequenz, zu der Zeigen gerollt hat", [torsoZiel] {
        // TEST1411c: Die Sequenzliste (TEST1411 E) ist eine Tabelle mit festem Kopf und eigener Rollflaeche. Bis TEST1411b klickte X17 die
        // erste gezeichnete TORSO_-Zeile unterhalb von 1,5 em unter dem Rand des Explorers - nach dem Rollen liegt sie (nach Namen sortiert,
        // Zeigen stellt die Zielzeile auf 40 % der Hoehe) halb hinter dem Tabellenkopf bzw. dem Suchfeld, der Klick traf den Kopf (Lauf
        // t1411_ap2e: Sekundaer -1). Jetzt die Zeile, zu der Zeigen gerollt hat, und nur, wenn sie ganz im sichtbaren Koerper der Tabelle steht.
        const ImRect k = seqKoerper();
        for (const ExplorerLage& l : ex().lage) {
            if (l.art == ExplorerArt::Sequenz && l.name.rfind("TORSO_", 0) == 0) {
                const float y = (l.y0 + l.y1) * 0.5F;
                melde("X17: erste gezeichnete TORSO_-Zeile '" + l.name + "' Mitte y " + std::to_string(static_cast<int>(y)) + ", sichtbarer Koerper der Liste y " +
                      std::to_string(static_cast<int>(k.Min.y)) + ".." + std::to_string(static_cast<int>(k.Max.y)) + (y < k.Min.y ? " (hinter dem Kopf)" : ""));
                break;
            }
        }
        for (const ExplorerLage& l : ex().lage) {
            if (l.art == ExplorerArt::Sequenz && l.index == *torsoZiel && l.y0 >= k.Min.y && l.y1 <= k.Max.y) {
                melde("X17: Klick auf '" + l.name + "' bei y " + std::to_string(static_cast<int>((l.y0 + l.y1) * 0.5F)));
                return ImVec2{l.xName + em(), (l.y0 + l.y1) * 0.5F};
            }
        }
        return ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.3));
    add(pruefe("AP2 X17: TORSO_ als Sekundaersequenz, die Primaersequenz bleibt", [vorher] {
        const int s = g_app->mtab.sekundaer;
        melde("X17: Sekundaer " + std::to_string(s) + (s >= 0 ? " " + g_app->animList[static_cast<std::size_t>(s)].name : std::string()) + ", Primaer " +
              std::to_string(g_app->animIndex) + " (vorher " + std::to_string(*vorher) + "), Reiter " + std::to_string(ex().art));
        return s >= 0 && g_app->animList[static_cast<std::size_t>(s)].name.rfind("TORSO_", 0) == 0 && g_app->animIndex == *vorher;
    }));
    add(tu("AP2 X17: zurueck", [] {
        ex().art = 0;   // TEST1411 E: zurueck in den Baum (Zeigen einer Sequenz wechselt in den Reiter Sequenzen)
        ex().torsoSekundaer = false;
        g_app->mtab.sekundaer = -1;
        g_app->animPlaying = false;
    }));
    // X22
    // TEST1411 G (shank 14.11.: "auch das bei layers und by hierarchy unten ist"): wie 3ds Max keine Fusszeile mehr - der Hinweis steht im
    // Tooltip des Spaltenkopfs und im Handbuch
    add(pruefe("AP2 X22: keine Hinweiszeile unter dem Baum (wie 3ds Max, Hinweis im Tooltip)", [] { return !ex().fussHinweis && !ex().fussEbenen; }));
    // X4: Filter (Symbolreihe)
    auto zeilenVor = std::make_shared<int>(0);
    add(tu("AP2 X4: zum Anfang", [zeilenVor] {
        modellExplorerZeigen(ExplorerArt::Modell, 0);
        *zeilenVor = ex().zeilen;
    }));
    add(pause(0.3));
    add(klickAn("AP2 X4: Filter Tags", filterOrt(Str::MbXArtTags)));
    add(pause(0.3));
    add(foto("ap2_max_filter"));
    add(pruefe("AP2 X4: Symbolreihe des Filters - ein Klick laesst die Tags weg (der Baum hat weniger Zeilen)", [zeilenVor] {
        melde("X4: " + std::to_string(*zeilenVor) + " Zeilen, ohne Tags " + std::to_string(ex().zeilen));
        return ex().filterReihe && ex().filterAus[static_cast<int>(ExplorerFilter::Tags)] && ex().zeilen < *zeilenVor - 2 && zeileArt(ExplorerArt::KopfFlaechen) != nullptr;
    }));
    add(klickAn("AP2 X4: Filter Tags zurueck", filterOrt(Str::MbXArtTags)));
    add(pause(0.3));
    add(pruefe("AP2 X4: noch ein Klick zeigt sie wieder", [zeilenVor] { return !ex().filterAus[static_cast<int>(ExplorerFilter::Tags)] && ex().zeilen == *zeilenVor; }));
    // X11: nach Ebenen
    add(klickAn("AP2 X11: By layers", kopfOrt(Str::MbNachEbenen)));
    add(pause(0.4));
    add(foto("ap2_max_ebenen"));
    add(tu("AP2 X11: alle Ebenen zuklappen (die sieben Zeilen stehen dann untereinander)", [] {
        for (int e = 0; e < 7; ++e) { modellExplorerAufklappen(ExplorerArt::Ebene, e, false); }
        modellExplorerZeigen(ExplorerArt::Ebene, 0);
    }));
    add(pause(0.4));
    add(foto("ap2_max_ebenen_zu"));
    add(pruefe("AP2 X11: Ebenen Body, Caps, Tags, Bones, Attachments, LOD 2, LOD 3 - je mit Auge, die ersten fuenf mit Schneeflocke", [] {
        int n = 0;
        int frost = 0;
        std::string namen;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Ebene) { continue; }
            ++n;
            namen += l.name + "; ";
            if (l.xAuge < 0.0F || l.tiefe != 0) { return false; }
            frost += l.xFrost >= 0.0F ? 1 : 0;
        }
        melde("X11: " + namen);
        return n >= 7 && frost >= 5 && zeile(tr(Str::MbXEbeneKoerper), static_cast<int>(ExplorerArt::Ebene)) != nullptr &&
               zeile(tr(Str::MbXEbeneAnbauten), static_cast<int>(ExplorerArt::Ebene)) != nullptr && !ex().fussEbenen;   // TEST1411 G: Satz im Tooltip
    }));
    add(klickAn("AP2 X11: Auge der Ebene Tags", ort(tr(Str::MbXArtTags), 1, static_cast<int>(ExplorerArt::Ebene))));
    add(pause(0.3));
    add(pruefe("AP2 X11: das Auge der Ebene blendet alle Tags aus", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        int n = 0;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (!in.teile[i].istTag()) { continue; }
            ++n;
            if (!modellTeilVersteckt(static_cast<int>(i))) { return false; }
        }
        return n > 0 && !modellTeilVersteckt(mb1::teil("torso"));
    }));
    add(klickAn("AP2 X11: Auge der Ebene Tags zurueck", ort(tr(Str::MbXArtTags), 1, static_cast<int>(ExplorerArt::Ebene))));
    add(pause(0.2));
    add(klickAn("AP2 X11/X13: Schneeflocke der Ebene Body", ort(tr(Str::MbXEbeneKoerper), 2, static_cast<int>(ExplorerArt::Ebene))));
    add(pause(0.3));
    add(foto("ap2_max_ebenen_frost"));
    add(pruefe("AP2 X11/X13: die Schneeflocke der Ebene friert alle ihre Flaechen ein", [] { return modellTeilEingefroren(mb1::teil("torso")) && modellTeilEingefroren(mb1::teil("hips")); }));
    add(klickAn("AP2 X11: Schneeflocke der Ebene Body zurueck", ort(tr(Str::MbXEbeneKoerper), 2, static_cast<int>(ExplorerArt::Ebene))));
    add(pause(0.2));
    add(pruefe("AP2 X11: noch ein Klick loest die Ebene wieder", [] {
        const ExplorerLage* l = zeile(tr(Str::MbXEbeneKoerper), static_cast<int>(ExplorerArt::Ebene));
        return l != nullptr && !l->eingefroren && !modellTeilEingefroren(mb1::teil("torso"));
    }));
    add(klickAn("AP2: By hierarchy", kopfOrt(Str::MbNachHierarchie)));
    add(pause(0.3));
    // X23: derselbe Baum in allen Bereichen, Griffe als Zeilen
    add(tu("AP2 X23: Bereich Animation, Control Rig", [] {
        g_app->mtab.bereich = ModellBereich::Animation;
        (void)modellRigAnlegen();
        modellExplorerZeigen(ExplorerArt::KopfRig, 0);
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(foto("ap2_max_rig"));
    add(pruefe("AP2 X23: derselbe Baum im Bereich Animation, die Griffe des Rigs als Zeilen mit Auge", [] {
        int n = 0;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Griff) { continue; }
            ++n;
            if (l.xAuge < 0.0F || l.xSymbol < 0.0F) { return false; }
        }
        return n > 3 && zeileArt(ExplorerArt::KopfRig) != nullptr;
    }));
    add(tu("AP2 X23: Bereich UV", [] {
        g_app->mtab.bereich = ModellBereich::Uv;
        modellExplorerZeigen(ExplorerArt::Modell, 0);
    }));
    add(pause(0.6));
    add(pruefe("AP2 X23: derselbe Baum im Bereich UV", [] { return zeileArt(ExplorerArt::Modell) != nullptr && zeileArt(ExplorerArt::KopfFlaechen) != nullptr; }));
    add(tu("AP2: zurueck nach Ansehen/Pruefen", [] {
        g_app->mtab.bereich = ModellBereich::Ansehen;
        zuruecksetzen();
    }));
    add(pause(0.4));
}

// --- AP2 wie Blender -------------------------------------------------------------------------------------------------------------------

void blenderSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP2: wie Blender", [] {
        modellBedienungSetzen(1);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        zuruecksetzen();
        modellExplorerZeigen(ExplorerArt::Modell, 0);
        // AP4: wie Blender steht der Outliner oben rechts (Entwurf 44 % der Spalte) - fuer die Zeile torso und die Fusszeile hoeher gezogen
        modellBl().outlinerAnteil = 0.8F;
    }));
    add(pause(0.6));
    add(foto("ap2_blender_hierarchie"));
    add(pruefe("AP2 X2: wie Blender heisst der Baum \"Outliner\" (Titel ueber dem Baum; AP4: er steht oben rechts, die linke Spalte ist die Werkzeugleiste)", [] {
        melde("X2: Titel '" + std::string(modellExplorerTitel()) + "', linke Spalte '" + spaltenTitel() + "'");
        return std::string(modellExplorerTitel()) == tr(Str::MbXOutliner) && spaltenTitel() == tr(Str::BzWerkzeugleiste);
    }));
    add(pruefe("AP2 X3: Umschalter \"Hierarchy / Collections\" (wie Blender)", [] {
        return imKopf(tr(Str::MbXHierarchie)) != nullptr && imKopf(tr(Str::MbXSammlungen)) != nullptr && imKopf(tr(Str::MbNachEbenen)) == nullptr;
    }));
    add(pruefe("AP2 X15: Outliner - Baum links (Pfeil, Symbol, Name), Auge und Pfeil \"waehlbar\" rechts in festen Spalten, keine Spaltenkoepfe", [] {
        const ImGuiWindow* w = kindFenster("mtbaum");
        if (w == nullptr) { return false; }
        int n = 0;
        float auge = -1.0F;
        float frost = -1.0F;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Flaeche) { continue; }
            ++n;
            if (l.xAuge < l.xNameEnde || l.xFrost <= l.xAuge || l.xSymbol < 0.0F || l.xName <= l.xSymbol) { return false; }
            if (auge >= 0.0F && (std::fabs(l.xAuge - auge) > 1.0F || std::fabs(l.xFrost - frost) > 1.0F)) { return false; }
            auge = l.xAuge;
            frost = l.xFrost;
        }
        char z[160];
        std::snprintf(z, sizeof z, "X15: %d Flaechenzeilen, Auge x %.0f, waehlbar x %.0f, Feld bis %.0f", n, static_cast<double>(auge), static_cast<double>(frost),
                      static_cast<double>(w->InnerRect.Max.x));
        melde(z);
        return n > 5 && !ex().koepfe && auge >= w->InnerRect.Max.x - em() * 4.0F && w->ScrollMax.x <= 0.0F;
    }));
    add(klickAn("AP2 X15: Pfeil \"waehlbar\" von torso", ort("torso", 2, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.3));
    add(tu("AP2 X15: Auswahl leeren", [] { g_app->mtab.auswahl.clear(); }));
    add(klickAn("AP2 X15: Klick auf torso", ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.3));
    add(foto("ap2_blender_nicht_waehlbar"));
    add(pruefe("AP2 X15: nicht waehlbar - der Klick waehlt torso nicht", [] { return modellTeilEingefroren(mb1::teil("torso")) && g_app->mtab.auswahl.empty(); }));
    add(klickAn("AP2 X15: Pfeil \"waehlbar\" von torso zurueck", ort("torso", 2, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.2));
    add(klickAn("AP2 X15: Auge von torso", ort("torso", 1, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.3));
    add(pruefe("AP2 X15: das Auge rechts blendet aus", [] { return modellTeilVersteckt(mb1::teil("torso")); }));
    add(tu("AP2 X15: alles einblenden", [] { modellAllesEinblenden(); }));
    add(pruefe("AP2 X4: wie Blender keine Symbolreihe, sondern der Filter-Ausklapper neben der Suche", [] { return !ex().filterReihe && imKopf(tr(Str::MbXFilter)) != nullptr; }));
    add(klickAn("AP2 X4: Filter-Ausklapper", kopfOrt(Str::MbXFilter)));
    add(pause(0.4));
    add(foto("ap2_blender_filter"));
    add(pruefe("AP2 X4: der Ausklapper nennt die Arten, \"Bones as a flat list\" und \"Sort alphabetically\"", [] {
        return menuePunkt(Str::MbXArtTags) && menuePunkt(Str::MbXArtSequenzen) && menuePunkt(Str::MbXKnochenFlach) && menuePunkt(Str::MbXSortName);
    }));
    add(klick(tr(Str::MbXArtTags), ""));
    add(pause(0.3));
    add(pruefe("AP2 X4: ein Klick laesst die Tags weg", [] { return ex().filterAus[static_cast<int>(ExplorerFilter::Tags)]; }));
    add(klick(tr(Str::MbXArtTags), ""));
    add(pause(0.2));
    add(mbefoto::popupsZu());
    add(pause(0.2));
    add(klickAn("AP2 X11: Collections", kopfOrt(Str::MbXSammlungen)));
    add(pause(0.4));
    add(foto("ap2_blender_sammlungen"));
    add(tu("AP2 X11: alle Ebenen zuklappen (die sieben Zeilen stehen dann untereinander)", [] {
        for (int e = 0; e < 7; ++e) { modellExplorerAufklappen(ExplorerArt::Ebene, e, false); }
        modellExplorerZeigen(ExplorerArt::Ebene, 0);
    }));
    add(pause(0.4));
    add(foto("ap2_blender_sammlungen_zu"));
    add(pruefe("AP2 X11: wie Blender dieselben Ebenen als Sammlungen, Auge und Pfeil rechts", [] {
        int n = 0;
        for (const ExplorerLage& l : ex().lage) {
            if (l.art != ExplorerArt::Ebene) { continue; }
            ++n;
            if (l.xAuge < l.xNameEnde) { return false; }
        }
        return n >= 7 && ex().art == 1 && !ex().fussEbenen;   // TEST1411 G: der Satz zu Auge und Pfeil steht im Tooltip
    }));
    add(klickAn("AP2: Hierarchy", kopfOrt(Str::MbXHierarchie)));
    add(pause(0.3));
    add(tu("AP2: Knochen offen (wie Blender)", [] {
        modellExplorerKnochenAlleAuf(true);
        modellExplorerZeigen(ExplorerArt::Knochen, knochen("rhand"));
    }));
    add(pause(0.5));
    add(foto("ap2_blender_knochen_offen"));
    add(tu("AP2: Knochen zu, Sequenzen", [] {
        modellExplorerKnochenAlleAuf(false);
        modellExplorerAufklappen(ExplorerArt::KopfKnochen, 0, false);
        modellExplorerZeigen(ExplorerArt::Sequenz, g_app->animIndex);
    }));
    add(pause(0.5));
    add(foto("ap2_blender_sequenzen"));
    add(tu("AP2: zurueck (wie 3ds Max)", [] {
        zuruecksetzen();
        modellExplorerZeigen(ExplorerArt::Modell, 0);
        modellBl().outlinerAnteil = ModellBlender{}.outlinerAnteil;
        modellBedienungSetzen(0);
    }));
    add(pause(0.4));
}

// --- Fotos fuer Sprachen, Themen, Fenstergroessen --------------------------------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap2f_max_" : "ap2f_blender_";
        add(tu("AP2 Foto: Modus", [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Ansehen;
            zuruecksetzen();
            g_app->mtab.auswahl = {mb1::teil("torso")};
            modellExplorerZeigen(ExplorerArt::Modell, 0);
        }));
        add(pause(0.8));
        add(foto(p + "hierarchie"));
        add(tu("AP2 Foto: Zeilen melden", [=] {
            int gekuerzt = 0;
            std::string namen;
            for (const ExplorerLage& l : ex().lage) {
                if (!l.gekuerzt) { continue; }
                ++gekuerzt;
                if (namen.size() < 300U) { namen += l.name + "; "; }
            }
            const ImGuiWindow* w = kindFenster("mtexplorer");
            char z[200];
            std::snprintf(z, sizeof z, "Foto %shierarchie: Explorer %.1f Zeichenhoehen breit, %zu Zeilen im Bild, %d gekuerzt: ", p.c_str(),
                          w != nullptr ? static_cast<double>(w->Size.x / em()) : 0.0, ex().lage.size(), gekuerzt);
            melde(z + namen);
        }));
        add(tu("AP2 Foto: nach Ebenen", [] { ex().art = 1; }));
        add(pause(0.5));
        add(foto(p + "ebenen"));
        add(tu("AP2 Foto: Zeilen melden (Ebenen)", [=] {
            int gekuerzt = 0;
            for (const ExplorerLage& l : ex().lage) { gekuerzt += l.gekuerzt ? 1 : 0; }
            melde("Foto " + p + "ebenen: " + std::to_string(ex().lage.size()) + " Zeilen im Bild, " + std::to_string(gekuerzt) + " gekuerzt");
        }));
        add(tu("AP2 Foto: Knochen offen", [] {
            ex().art = 0;
            modellExplorerKnochenAlleAuf(true);
            modellExplorerZeigen(ExplorerArt::Knochen, knochen("rhand"));
        }));
        add(pause(0.5));
        add(foto(p + "knochen"));
        add(tu("AP2 Foto: Sequenzen", [] {
            modellExplorerKnochenAlleAuf(false);
            modellExplorerAufklappen(ExplorerArt::KopfKnochen, 0, false);
            g_app->mtab.sekundaer = mb1::animNach("TORSO_WEAPONREADY3");
            modellExplorerZeigen(ExplorerArt::Sequenz, g_app->animIndex);
        }));
        add(pause(0.5));
        add(foto(p + "sequenzen"));
        add(tu("AP2 Foto: Sekundaer aus", [] { g_app->mtab.sekundaer = -1; }));
    }
    add(tu("AP2 Foto: zurueck", [] {
        zuruecksetzen();
        modellBedienungSetzen(0);
    }));
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap2): h_ap2_max, h_ap2_ebenen, h_ap2_blender - dieselbe Figur
// wie die Seite "Model-Tab: Ansehen und Pruefen" (Sturmtruppler, Saebel an *r_hand).
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    add(tu("AP2 Handbuch: breite Anordnung, Saebel an *r_hand, BOTH_STAND1, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        g_app->settings.splitModel = 0.85F;
        modellBedienungSetzen(0);
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        animWaehlen(mb1::animNach("BOTH_STAND1") >= 0 ? mb1::animNach("BOTH_STAND1") : 0, false);
        g_app->mtab.sekundaer = mb1::animNach("TORSO_WEAPONREADY3");
        modellExplorerVorgabe();
        g_app->mtab.auswahl.clear();
        modellAnsichtEinpassenAktiv();
        g_app->mtab.auswahl = {mb1::teil("torso")};
        modellExplorerZeigen(ExplorerArt::Modell, 0);
    }));
    add(pause(1.2));
    add(hilfe2::schrittFoto("h_ap2_max"));
    add(tu("AP2 Handbuch: nach Ebenen, die Tags ausgeblendet", [] {
        modellExplorer().art = 1;
        modellEbeneAuge(ExplorerEbene::Tags, false);
    }));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_ap2_ebenen"));
    add(tu("AP2 Handbuch: wie Blender, Hierarchie", [] {
        modellEbeneAuge(ExplorerEbene::Tags, true);
        modellExplorer().art = 0;
        modellBedienungSetzen(1);
    }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_ap2_blender"));
    add(tu("AP2 Handbuch: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->mtab.sekundaer = -1;
        modellExplorerVorgabe();
        modellAnbautenLoesen();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap2" && n != "ap2foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP2_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* m = std::getenv("BHED_AP2_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    add(tu("AP2: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP2_THEMA")) {
            const bool hell = std::string(t) == "hell";
            for (const theme::Theme& th : theme::themes()) {
                if ((theme::relativeLuminance(th.window) > 0.5F) == hell) {
                    g_app->activeTheme = &th;
                    g_app->themePending = true;
                    melde("Thema " + th.id);
                    break;
                }
            }
        }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap2.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP2: Aufteilung wie neue Einstellungen, Saebel an *r_hand, Sequenz, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        g_app->settings.splitModel = Settings{}.splitModel;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        animWaehlen(mbefoto::sequenz(), false);
        char zeile[300];
        std::snprintf(zeile, sizeof zeile, "%s, %zu Teile, %zu Knochen, %zu Sequenzen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(),
                      g_app->mtab.dok.inhalt().teile.size(), g_app->anim.bones.size(), g_app->animList.size(), static_cast<double>(ImGui::GetIO().DisplaySize.x),
                      static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale), static_cast<double>(ImGui::GetFontSize()));
        melde(zeile);
    }));
    add(pause(1.0));
    add(klickAn("AP2: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap2") {
        tooltipSchritte(s);
        maxSchritte(s);
        blenderSchritte(s);
    } else {
        fotoSchritte(s);
    }
    add(tu("AP2: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->uiScale = *skala;
        if (*thema != nullptr && g_app->activeTheme != *thema) {
            g_app->activeTheme = *thema;
            g_app->themePending = true;
        }
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        modellAnbautenLoesen();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace ap2
