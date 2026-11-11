// selbsttest_uiform.inc - UIFORM (rc612): der Formular-Baustein am Fenster "Into script" und die Mindestgroessen der Fenster
// (BHED_EDITORTEST=uiform)
//
//   BHED_UIF_NUR=insskript   Into script in allen 6 Sprachen: Felder an derselben Kante und gleich breit (+-1), Beschriftung auf der
//                            Feldmitte (+-1), Knopf im Feld der Animation, Pfeil am Feld der Figur (ein Feld), Hinweis unter dem Feld,
//                            Code-Kasten in Festbreitenschrift, Knoepfe unten rechts (Einfuegen, Abbrechen), Enter fuegt ein, Esc
//                            schliesst; auf 1 x 1 gesetzt bleibt es benutzbar (Felder in Mindestbreite, Knopfreihe ganz sichtbar).
//   BHED_UIF_NUR=mindest     die Fenster, die sich in Events, Model, Map Editor und Menu Editor oeffnen lassen: in Ruhe nicht
//                            groesser als ohne Mindestgroessen; je Sprache auf 1 x 1: nicht kleiner als Titelzeile + eine Zeile,
//                            Bedienreihen (Elemente rechts neben einem anderen) ganz im Fenster - Listen duerfen rollen -, nicht
//                            groesser als vor dem Zusammenschieben, danach ruhig (kein Pumpen).
//   BHED_UIF_NUR=probe       die Messung der Bedienreihen an Probefenstern mit bekanntem Inhalt (nur mit dem Baustein): eine
//                            Knopfreihe und ein Knopf hinter Text muessen ganz hineinpassen, lange Listeneintraege, Baumknoten und
//                            Text verlangen nichts (Gegenprobe ohne Mindestgroessen: zu schmal); ein Eingabefeld hinter seiner
//                            Beschriftung behaelt 6 Zeichenhoehen; Inhalt, der mit dem Fenster mitwaechst, laesst es nicht
//                            zurueckwachsen; nie groesser, als das Fenster schon war.
//   BHED_UIF_NUR=schwebend   die Dock-Regel trifft nur Fenster, die ein Dock-Knoten anlegt: ein Panel in einem eigenen Fenster
//                            (Map Editor XY, Mission Script Flow) und ein Tab in einem eigenen Fenster (Model) landen auf 1 x 1
//                            auf ihrer Untergrenze (Gegenprobe ohne Mindestgroessen: kleiner); wieder angedockt hat das Panel
//                            die Groesse seines Platzes.
//   BHED_UIF_NUR=fotos       Fotos (BHED_FOTOS): Into script und weitere Fenster normal und auf 1 x 1.
//   ohne BHED_UIF_NUR        probe, schwebend, insskript und mindest.
// Gemessen wird am Element-Register und an ImGuis Fenstern, nicht am Baustein: derselbe Test laeuft auf dem Stand vor UIFORM
// (dort ohne ui_formular.h uebersetzt) und zeigt die Fehlerbilder rot.
// Eingebunden in selbsttest.cpp.
#if __has_include("ui_formular.h")
#define BHED_UIF_BAUSTEIN 1
#endif

namespace uif {

const char* const kSprachen[] = {"en", "de", "zh-Hans", "ja", "es", "cs"};

void spracheSetzen(const char* sp) {
    if (const i18n::LanguageInfo* l = i18n::findLanguage(sp)) { i18n::setLanguage(l->language); }
}

std::string insName() { return dock::fensterName("modell.insskript"); }

bool nahe(float a, float b, float d = 1.0F) { return std::fabs(a - b) <= d + 0.01F; }

std::string zahl(float v) {
    char z[32];
    std::snprintf(z, sizeof z, "%.0f", static_cast<double>(v));
    return z;
}

std::string titel(const ImGuiWindow* w) {
    const char* name = w->Name;
    return std::string(name, ImGui::FindRenderedTextEnd(name));
}

// Die Mindest-Feldbreite der Formulare in Zeichenhoehen und der Text des zweiten Knopfs der Knopfreihe.
#ifdef BHED_UIF_BAUSTEIN
constexpr float kFeld = ui::kMindestFeld;
const char* abbrechenText() { return tr(Str::UfAbbrechen); }
const char* schliessenText() { return tr(Str::UfSchliessen); }
#else
constexpr float kFeld = 14.0F;
const char* abbrechenText() { return "Cancel"; }   // vor UIFORM gibt es den Knopf nicht
const char* schliessenText() { return "Close"; }
#endif

// --- Into script: die Teile des Fensters aus dem Element-Register ------------------------------------------------------
struct InsTeile {
    const ImGuiWindow* w = nullptr;
    const Element* anim = nullptr;       // Ausklapper der Animation
    const Element* befehl = nullptr;     // Ausklapper des Befehls
    const Element* figur = nullptr;      // Eingabe der Figur
    const Element* pfeil = nullptr;      // ihr Pfeil (Liste)
    const Element* ziel = nullptr;       // Ausklapper des Skripts
    const Element* gewaehlte = nullptr;  // "gewaehlte Animation nehmen" im Feld
    const Element* einfuegen = nullptr;
    const Element* abbrechen = nullptr;
    const ImGuiWindow* code = nullptr;   // Code-Kasten (Kindfenster)
};

const Element* mitKennung(ImGuiID a, ImGuiID b = 0) {
    const Element* e = findeKennung(a);
    return e != nullptr || b == 0 ? e : findeKennung(b);
}

InsTeile insTeile() {
    InsTeile t;
    const std::string name = insName();
    ImGuiWindow* w = ImGui::FindWindowByName(name.c_str());
    if (w == nullptr || !w->WasActive) { return t; }
    t.w = w;
    const ImGuiID figur = ImHashStr("##insfigur", 0, w->ID);
    t.anim = mitKennung(ImHashStr("##insanim", 0, w->ID));
    t.befehl = mitKennung(ImHashStr("##insbefehl", 0, w->ID));
    // mit dem Baustein: Eingabe und Pfeil unter der Kennung des Felds; vorher zwei Elemente nebeneinander
    t.figur = mitKennung(ImHashStr("##eingabe", 0, figur), figur);
    t.pfeil = mitKennung(ImHashStr("##pfeil", 0, figur), ImHashStr("##insfiguren", 0, w->ID));
    t.ziel = mitKennung(ImHashStr("##insziel", 0, w->ID));
    t.gewaehlte = mitKennung(ImHashStr("##insgewaehlte", 0, w->ID));
    t.einfuegen = finde(tr(Str::MbInsSkriptEinfuegen), "modell.insskript", 0, false);
    t.abbrechen = finde(abbrechenText(), "modell.insskript", 0, false);
    if (t.abbrechen == nullptr) { t.abbrechen = finde(schliessenText(), "modell.insskript", 0, false); }
    for (const ImGuiWindow* k : GImGui->Windows) {
        if (k->ParentWindow == w && k->WasActive && std::strstr(k->Name, "##insvorschau") != nullptr) { t.code = k; }
    }
    return t;
}

// Wo ein Text im Fenster gezeichnet ist (die linke obere Ecke seiner Zeile im Bereich `wo`) - aus den Glyphen der Zeichenliste:
// ImGui meldet Text nicht ans Element-Register (ItemAdd ohne Kennung ruft den Haken nicht). Das erste Zeichen des Textes als
// Glyphenrechteck (ImFont::RenderText: Ecken links oben U0/V0, rechts unten U1/V1; Ort = Zeilenanfang + X0/Y0 des Glyphen).
bool textOrt(const ImGuiWindow* w, const char* text, const ImRect& wo, ImVec2& ort) {
    unsigned int c = 0;
    if (text == nullptr || ImTextCharFromUtf8(&c, text, nullptr) <= 0 || c == 0) { return false; }
    ImFontBaked* schrift = ImGui::GetFontBaked();
    const ImFontGlyph* gl = schrift != nullptr ? schrift->FindGlyphNoFallback(static_cast<ImWchar>(c)) : nullptr;
    if (gl == nullptr) { return false; }
    const ImVector<ImDrawVert>& v = w->DrawList->VtxBuffer;
    for (int i = 0; i + 3 < v.Size; ++i) {
        if (std::fabs(v[i].uv.x - gl->U0) > 1.0e-6F || std::fabs(v[i].uv.y - gl->V0) > 1.0e-6F || std::fabs(v[i + 2].uv.x - gl->U1) > 1.0e-6F ||
            std::fabs(v[i + 2].uv.y - gl->V1) > 1.0e-6F) {
            continue;
        }
        const ImVec2 p{v[i].pos.x - gl->X0, v[i].pos.y - gl->Y0};
        if (wo.Contains(p)) {
            ort = p;
            return true;
        }
    }
    return false;
}

// Die Beschriftung eines Felds: ihr Text links vom Feld in seiner Zeile (als Rechteck der Textzeile). false: nicht gefunden.
bool beschriftungVon(const ImGuiWindow* w, const char* text, const ImRect& feld, ImRect& r) {
    const float fs = ImGui::GetFontSize();
    ImVec2 p;
    if (!textOrt(w, text, ImRect{w->Pos.x, feld.Min.y - fs, feld.Min.x, feld.Max.y}, p)) { return false; }
    r = ImRect{p, ImVec2{p.x + ImGui::CalcTextSize(text, nullptr, true).x, p.y + fs}};
    return true;
}

// Layout des Formulars: Felder an derselben linken und rechten Kante, Beschriftungen auf der Feldmitte und links davon, Eingabe der
// Figur und Pfeil als ein Feld, Knopf im Feld der Animation, Hinweis unter dem Feld, Code-Kasten, Knoepfe unten rechts.
std::string layoutBefund() {
    const InsTeile t = insTeile();
    if (t.w == nullptr) { return " Fenster nicht offen"; }
    std::string f;
    struct Feld {
        const char* name;
        const char* text;   // die Beschriftung in der laufenden Sprache
        const Element* e;
        float rechts;
    };
    const Feld felder[] = {{"Animation", tr(Str::MbInsSkriptAnim), t.anim, t.anim != nullptr ? t.anim->rect.Max.x : 0.0F},
                           {"Befehl", tr(Str::MbInsSkriptBefehl), t.befehl, t.befehl != nullptr ? t.befehl->rect.Max.x : 0.0F},
                           // die Figur: Eingabe und Pfeil zusammen
                           {"Figur", tr(Str::MbInsSkriptFigur), t.figur,
                            t.pfeil != nullptr ? t.pfeil->rect.Max.x : (t.figur != nullptr ? t.figur->rect.Max.x : 0.0F)},
                           {"Skript", tr(Str::MbInsSkriptZiel), t.ziel, t.ziel != nullptr ? t.ziel->rect.Max.x : 0.0F}};
    const Feld* erstes = nullptr;
    float unten = 0.0F;
    for (const Feld& x : felder) {
        if (x.e == nullptr) {
            f += std::string(" [Feld ") + x.name + " fehlt]";
            continue;
        }
        unten = std::max(unten, x.e->rect.Max.y);
        if (erstes == nullptr) { erstes = &x; }
        if (!nahe(x.e->rect.Min.x, erstes->e->rect.Min.x)) {
            f += std::string(" [linke Kante ") + x.name + " " + zahl(x.e->rect.Min.x) + " statt " + zahl(erstes->e->rect.Min.x) + "]";
        }
        if (!nahe(x.rechts, erstes->rechts)) { f += std::string(" [rechte Kante ") + x.name + " " + zahl(x.rechts) + " statt " + zahl(erstes->rechts) + "]"; }
        ImRect b;
        if (!beschriftungVon(t.w, x.text, x.e->rect, b)) {
            f += std::string(" [") + x.name + " ohne Beschriftung]";
        } else {
            if (!nahe(b.GetCenter().y, x.e->rect.GetCenter().y)) {
                f += std::string(" [Beschriftung ") + x.name + " Mitte " + zahl(b.GetCenter().y) + " statt " + zahl(x.e->rect.GetCenter().y) + "]";
            }
            if (b.Max.x > x.e->rect.Min.x) { f += std::string(" [Beschriftung ") + x.name + " ragt ins Feld]"; }
        }
    }
    if (t.figur != nullptr && t.pfeil != nullptr && (!nahe(t.pfeil->rect.Min.x, t.figur->rect.Max.x, 0.5F) || !nahe(t.pfeil->rect.Min.y, t.figur->rect.Min.y, 0.5F))) {
        f += " [Pfeil der Figur abgesetzt: " + zahl(t.pfeil->rect.Min.x - t.figur->rect.Max.x) + "]";
    }
    if (t.pfeil == nullptr) { f += " [Figur ohne Pfeil]"; }
    if (t.gewaehlte == nullptr || t.anim == nullptr || !t.anim->rect.Contains(t.gewaehlte->rect) ||
        t.gewaehlte->rect.Max.x > t.anim->rect.Max.x - ImGui::GetFrameHeight() + 1.0F) {
        f += " [kein Knopf 'gewaehlte Animation' im Feld]";
    }
    // Hinweis unter dem Feld der Figur: sein Text beginnt an der Feldkante zwischen Figur und Skript
    ImVec2 hinweis;
    if (t.figur == nullptr || t.ziel == nullptr ||
        !textOrt(t.w, tr(Str::MbInsSkriptFigurHint),
                 ImRect{t.figur->rect.Min.x - 1.5F, t.figur->rect.Max.y - 1.0F, t.figur->rect.Min.x + 1.5F, t.ziel->rect.Min.y}, hinweis)) {
        f += " [kein Hinweis unter dem Feld der Figur]";
    }
    if (t.code == nullptr) {
        f += " [kein Code-Kasten]";
    } else {
        unten = std::max(unten, t.code->Pos.y + t.code->Size.y);
#ifdef BHED_UIF_BAUSTEIN
        // Festbreitenschrift: der Inhalt des Kastens ist so breit wie die Zeile in der Code-Schrift
        const std::string zeile = modellInsSkriptZeile();
        const std::string zeigen = zeile.empty() ? std::string("-") : zeile;
        ImFont* mono = menueeditor::codeSchrift();
        const float soll = mono != nullptr ? mono->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0F, zeigen.c_str(), zeigen.c_str() + zeigen.size()).x : -1.0F;
        if (mono == nullptr || !nahe(t.code->ContentSize.x, soll, 2.0F)) {
            f += " [Code-Kasten nicht in Festbreitenschrift: " + zahl(t.code->ContentSize.x) + " statt " + zahl(soll) + "]";
        }
#endif
    }
    if (t.einfuegen == nullptr || t.abbrechen == nullptr) {
        f += std::string(" [Knopfreihe:") + (t.einfuegen == nullptr ? " Einfuegen fehlt" : "") + (t.abbrechen == nullptr ? " Abbrechen fehlt" : "") + "]";
    } else {
        const ImRect& a = t.einfuegen->rect;
        const ImRect& b = t.abbrechen->rect;
        if (a.Min.y < unten - 1.0F || b.Min.y < unten - 1.0F) { f += " [Knoepfe nicht unter den Feldern]"; }
        if (!nahe(a.Min.y, b.Min.y) || a.Max.x > b.Min.x) { f += " [Einfuegen nicht links neben Abbrechen]"; }
        if (!nahe(b.Max.x, t.w->WorkRect.Max.x)) { f += " [Abbrechen endet bei " + zahl(b.Max.x) + " statt am rechten Rand " + zahl(t.w->WorkRect.Max.x) + "]"; }
    }
    return f;
}

// --- Mindestgroessen ---------------------------------------------------------------------------------------------------
// Alle frei schwebenden Fenster mit Titelzeile (nicht das Hauptfenster, keine Kindfenster, Listen, Tooltips, Dock-Wirte).
std::vector<std::string> schwebende() {
    std::vector<std::string> r;
    for (const ImGuiWindow* w : GImGui->Windows) {
        if (!w->WasActive || w->Hidden || w->Collapsed || w->DockIsActive) { continue; }
        if ((w->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_DockNodeHost |
                         ImGuiWindowFlags_NoTitleBar)) != 0) {
            continue;
        }
        r.emplace_back(w->Name);
    }
    std::sort(r.begin(), r.end());
    return r;
}

// Bedienelemente, die rechts neben einem anderen stehen (Knopfreihe, Werkzeugleiste), ragen nicht ueber den rechten Rand - auch
// nicht weiter unten, wohin man rollen kann. Elemente allein in ihrer Zeile (Listen, Baeume) und Tabellenzellen duerfen am Rand
// enden. Das Register kennt nur Elemente mit Kennung (kein Text).
std::string reihenBefund(const ImGuiWindow* w) {
    const float spiel = ImGui::GetStyle().ItemSpacing.x * 0.5F + 0.5F;
    const ImRect innen = w->InnerClipRect;
    std::vector<const Element*> el;
    for (const Element& e : g_elemente) {
        if (e.innen == w->Name && e.rect.GetWidth() > 0.0F && e.rect.GetHeight() > 0.0F) { el.push_back(&e); }
    }
    std::string f;
    int n = 0;
    for (std::size_t i = 0; i < el.size(); ++i) {
        const Element& x = *el[i];
        if (x.id == 0 || x.label.empty() || x.label[0] == '#') { continue; }                        // "#CLOSE", "#COLLAPSE", "#SCROLLY": Titelzeile, Rollbalken
        if (x.clip.Max.x < innen.Max.x - 1.0F || x.clip.Min.x > innen.Min.x + 1.0F) { continue; }   // Tabellenzelle, Spalte
        if (x.rect.Max.y <= w->InnerRect.Min.y) { continue; }                                       // Menueleiste (oder nach oben weggerollt)
        bool daneben = false;
        for (std::size_t k = 0; k < i && !daneben; ++k) {
            const Element& y = *el[k];
            if (!y.label.empty() && y.label[0] == '#') { continue; }
            const float ueberlappt = std::min(x.rect.Max.y, y.rect.Max.y) - std::max(x.rect.Min.y, y.rect.Min.y);
            daneben = ueberlappt > x.rect.GetHeight() * 0.5F && y.rect.Max.x <= x.rect.Min.x + 1.0F;
        }
        if (daneben && x.rect.Max.x > innen.Max.x + spiel) {
            if (n++ < 3) { f += " [" + x.label.substr(0, 40) + " ragt " + zahl(x.rect.Max.x - innen.Max.x) + " hinaus]"; }
        }
    }
    if (n > 3) { f += " [+" + std::to_string(n - 3) + "]"; }
    return f;
}

struct Stand {
    std::map<std::string, ImVec2> vorher;   // Groessen vor dem Zusammenschieben
    std::map<std::string, ImVec2> danach;   // kurz danach (gegen Pumpen)
};

void merke(std::map<std::string, ImVec2>& ziel, const std::vector<std::string>& namen) {
    ziel.clear();
    for (const std::string& n : namen) {
        if (const ImGuiWindow* w = ImGui::FindWindowByName(n.c_str())) { ziel[n] = w->Size; }
    }
}

// Ein Fenster nach dem Zusammenschieben auf 1 x 1.
std::string mindestBefund(const std::string& name, const Stand& st) {
    const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str());
    if (w == nullptr || !w->WasActive) { return " nicht offen"; }
    const float fs = ImGui::GetFontSize();
    std::string f;
    const bool selbst = (w->Flags & (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) != 0;   // das Programm bestimmt die Groesse
    const float eineZeile = w->TitleBarHeight + w->MenuBarHeight + ImGui::GetFrameHeight();
    if (!selbst && (w->Size.x < fs * 10.0F - 1.0F || w->Size.y < eineZeile)) {
        f += " [" + zahl(w->Size.x) + "x" + zahl(w->Size.y) + " kleiner als Titelzeile und eine Zeile]";
    }
    // Ein Fenster, dessen Hoehe sein Inhalt meldet (Formular), darf schmaler hoeher werden: derselbe Inhalt braucht dann mehr
    // Zeilen (der Hinweis bricht um, der Code-Kasten bekommt seinen Rollbalken).
    bool hoeheGemeldet = false;
#ifdef BHED_UIF_BAUSTEIN
    const ImVec2 m = ui::mindestVon(name.c_str());
    if (w->Size.x < m.x - 1.0F || w->Size.y < m.y - 1.0F) { f += " [" + zahl(w->Size.x) + "x" + zahl(w->Size.y) + " kleiner als Mindestgroesse " + zahl(m.x) + "x" + zahl(m.y) + "]"; }
    hoeheGemeldet = m.y > eineZeile + ImGui::GetStyle().WindowPadding.y * 2.0F + 1.0F && w->Size.y <= m.y + 1.0F;
#endif
    if ((w->Flags & (ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_HorizontalScrollbar)) == 0) { f += reihenBefund(w); }
    if (const auto it = st.vorher.find(name); it != st.vorher.end() && (w->Size.x > it->second.x + 0.5F || (!hoeheGemeldet && w->Size.y > it->second.y + 0.5F))) {
        f += " [groesser als vorher: " + zahl(it->second.x) + "x" + zahl(it->second.y) + " -> " + zahl(w->Size.x) + "x" + zahl(w->Size.y) + "]";
    }
    if (const auto it = st.danach.find(name); it != st.danach.end() && (!nahe(w->Size.x, it->second.x, 0.5F) || !nahe(w->Size.y, it->second.y, 0.5F))) {
        f += " [pumpt: " + zahl(it->second.x) + "x" + zahl(it->second.y) + " -> " + zahl(w->Size.x) + "x" + zahl(w->Size.y) + "]";
    }
    return f;
}

Schritt quetschen(const std::string& was, std::function<std::vector<std::string>()> namen) {
    return tu("UIF: " + was + " auf 1 x 1", [namen] {
        for (const std::string& n : namen()) {
            if (ImGuiWindow* w = ImGui::FindWindowByName(n.c_str())) { ImGui::SetWindowSize(w, ImVec2{1.0F, 1.0F}); }
        }
    });
}

// vorher merken, auf 1 x 1, kurz danach merken, spaeter pruefen
void quetschenUndPruefen(std::vector<Schritt>& s, const std::string& was, std::function<std::vector<std::string>()> namen, int mindestZahl) {
    auto st = std::make_shared<Stand>();
    s.push_back(tu("UIF: " + was + " - Groessen merken", [st, namen] { merke(st->vorher, namen()); }));
    s.push_back(quetschen(was, namen));
    s.push_back(pause(0.5));
    s.push_back(tu("UIF: " + was + " - Groessen nach dem Zusammenschieben", [st, namen] { merke(st->danach, namen()); }));
    s.push_back(pause(0.5));
    s.push_back(pruefe("UIF: " + was + " - auf 1 x 1 benutzbar: nicht kleiner als Titelzeile und eine Zeile, Bedienreihen ganz im Fenster, nicht groesser als vorher, ruhig",
                       [st, namen, was, mindestZahl] {
                           std::string fehler;
                           std::string liste;
                           int n = 0;
                           for (const std::string& name : namen()) {
                               const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str());
                               if (w == nullptr || w->DockIsActive) { continue; }
                               ++n;
                               const auto v = st->vorher.find(name);
                               liste += " [" + titel(w) + " " + (v != st->vorher.end() ? zahl(v->second.x) + "x" + zahl(v->second.y) : std::string("?")) + " -> " +
                                        zahl(w->Size.x) + "x" + zahl(w->Size.y) + "]";
                               const std::string b = mindestBefund(name, *st);
                               if (!b.empty()) { fehler += " {" + titel(w) + ":" + b + "}"; }
                           }
                           diag::detail("Knopftest: UIF " + was + " (" + i18n::kurzCode(i18n::currentLanguage()) + "): " + std::to_string(n) + " Fenster" + liste +
                                        (fehler.empty() ? std::string() : " | FEHLT:" + fehler));
                           return n >= mindestZahl && fehler.empty();
                       }));
    s.push_back(tu("UIF: " + was + " - Groessen zurueck", [st] {
        for (const auto& [n, g] : st->vorher) {
            if (ImGuiWindow* w = ImGui::FindWindowByName(n.c_str())) { ImGui::SetWindowSize(w, g); }
        }
    }));
    s.push_back(pause(0.3));
}

std::vector<Schritt> insskriptSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("UIF Into script: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "uif.txt"); }));
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(tu("UIF Into script: BOTH_WALK1, Haltezeit (Zeile fuer die Dauer), Fenster auf", [] {
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        modellInsSkriptOeffnen(mb1::animNach("BOTH_WALK1"));
        g_app->mtab.insBefehl = 3;
        g_app->mtab.insFigur[0] = '\0';
        g_app->mtab.insZiel = -2;
    }));
    add(pause(0.5));
    for (const char* sp : kSprachen) {
        const std::string spr = sp;
        add(tu("UIF Into script: Sprache " + spr, [spr] { spracheSetzen(spr.c_str()); }));
        add(pause(0.4));
        add(pruefe("UIF Into script (" + spr + "): Felder an einer Kante und gleich breit, Beschriftung auf der Feldmitte, Figur als ein Feld, Knopf im Feld, "
                   "Hinweis, Code-Kasten, Knoepfe unten rechts",
                   [spr] {
                       const std::string b = layoutBefund();
                       const InsTeile t = insTeile();
                       std::string z = "Knopftest: UIF Into script " + spr + ":";
                       const std::pair<const Element*, const char*> felder[] = {{t.anim, tr(Str::MbInsSkriptAnim)},
                                                                                {t.befehl, tr(Str::MbInsSkriptBefehl)},
                                                                                {t.figur, tr(Str::MbInsSkriptFigur)},
                                                                                {t.ziel, tr(Str::MbInsSkriptZiel)}};
                       for (const auto& [e, text] : felder) {
                           if (e == nullptr) { continue; }
                           ImRect l;
                           const bool mit = t.w != nullptr && beschriftungVon(t.w, text, e->rect, l);
                           z += " [Feld " + zahl(e->rect.Min.x) + ".." + zahl(e->rect.Max.x) + " y" + zahl(e->rect.GetCenter().y) +
                                (mit ? " Text .." + zahl(l.Max.x) + " y" + zahl(l.GetCenter().y) : std::string(" ohne Text")) + "]";
                       }
                       diag::detail(z + (b.empty() ? std::string() : " | FEHLT:" + b));
                       return b.empty();
                   }));
        quetschenUndPruefen(s, "Into script", [] { return std::vector<std::string>{insName()}; }, 1);
        // noch einmal klein: die Teile des Formulars auf der Mindestgroesse
        add(quetschen("Into script", [] { return std::vector<std::string>{insName()}; }));
        add(pause(0.5));
        add(pruefe("UIF Into script (" + spr + "): auf 1 x 1 behalten die Felder ihre Mindestbreite, die Knopfreihe ist ganz zu sehen, nichts rollt", [] {
            const InsTeile t = insTeile();
            if (t.w == nullptr || t.anim == nullptr || t.befehl == nullptr || t.figur == nullptr || t.ziel == nullptr || t.einfuegen == nullptr) { return false; }
            const float mindest = ImGui::GetFontSize() * kFeld - 1.0F;
            const float figur = (t.pfeil != nullptr ? t.pfeil->rect.Max.x : t.figur->rect.Max.x) - t.figur->rect.Min.x;
            const bool felder = t.anim->rect.GetWidth() >= mindest && t.befehl->rect.GetWidth() >= mindest && figur >= mindest && t.ziel->rect.GetWidth() >= mindest;
            const ImRect sicht = t.w->InnerClipRect;
            const bool knoepfe = sicht.Contains(t.einfuegen->rect) && t.abbrechen != nullptr && sicht.Contains(t.abbrechen->rect);
            diag::detail("Knopftest: UIF Into script 1x1: " + zahl(t.w->Size.x) + "x" + zahl(t.w->Size.y) + " Feld " + zahl(t.anim->rect.GetWidth()) + " (mindestens " +
                         zahl(mindest) + "), Rollbereich " + zahl(t.w->ScrollMax.x) + "/" + zahl(t.w->ScrollMax.y));
            return felder && knoepfe && t.w->ScrollMax.x <= 0.5F && t.w->ScrollMax.y <= 0.5F;
        }));
        add(tu("UIF Into script: wieder 32 Zeichen breit", [] {
            if (ImGuiWindow* w = ImGui::FindWindowByName(insName().c_str())) { ImGui::SetWindowSize(w, ImVec2{ImGui::GetFontSize() * 32.0F, w->Size.y}); }
        }));
        add(pause(0.3));
    }
    add(tu("UIF Into script: Englisch", [] { spracheSetzen("en"); }));
    // Angedockt bestimmt der Knoten die Groesse (ImGui setzt die Grenzen ausser Kraft): in einem schmalen Knoten rollt das Fenster
    // waagrecht, die Felder behalten ihre Mindestbreite.
    // Ein schwebender Knoten mit zwei Fenstern (Reiter) - mit einem allein liesse ImGui das Fenster frei schweben.
    auto knotenNr = std::make_shared<ImGuiID>(0);
    add(tu("UIF Into script: Rig-Uebersicht dazu", [] { g_app->mtab.uebersichtOffen = true; }));
    add(pause(0.4));
    add(tu("UIF Into script: mit der Rig-Uebersicht in einen 12 Zeichen schmalen schwebenden Knoten docken", [knotenNr] {
        const float fs = ImGui::GetFontSize();
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        *knotenNr = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodePos(*knotenNr, ImVec2{vp->WorkPos.x + fs * 20.0F, vp->WorkPos.y + fs * 8.0F});
        ImGui::DockBuilderSetNodeSize(*knotenNr, ImVec2{fs * 12.0F, fs * 24.0F});
        ImGui::DockBuilderDockWindow("###riguebersicht", *knotenNr);
        ImGui::DockBuilderDockWindow(insName().c_str(), *knotenNr);
        ImGui::DockBuilderFinish(*knotenNr);
    }));
    add(pause(0.4));
    add(tu("UIF Into script: sein Reiter nach vorn", [] { ImGui::SetWindowFocus(insName().c_str()); }));
    add(pause(0.6));
    add(pruefe("UIF Into script: schmal angedockt rollt es waagrecht, die Felder behalten ihre Mindestbreite", [] {
        const InsTeile t = insTeile();
        if (t.w == nullptr || t.anim == nullptr) { return false; }
        diag::detail("Knopftest: UIF Into script angedockt: " + std::string(t.w->DockIsActive ? "im Knoten " : "NICHT im Knoten ") + zahl(t.w->Size.x) + "x" +
                     zahl(t.w->Size.y) + ", Feld " + zahl(t.anim->rect.GetWidth()) + ", Rollbalken " + (t.w->ScrollbarX ? "ja" : "nein") + ", Rollbereich " +
                     zahl(t.w->ScrollMax.x));
        return t.w->DockIsActive && t.w->Size.x < ImGui::GetFontSize() * 14.0F && t.w->ScrollbarX && t.w->ScrollMax.x > 1.0F &&
               t.anim->rect.GetWidth() >= ImGui::GetFontSize() * kFeld - 1.0F;
    }));
    add(tu("UIF Into script: Knoten aufloesen, Rig-Uebersicht zu", [knotenNr] {
        if (*knotenNr != 0) { ImGui::DockBuilderRemoveNode(*knotenNr); }
        g_app->mtab.uebersichtOffen = false;
    }));
    add(pause(0.4));
    add(tu("UIF Into script: wieder 32 Zeichen breit", [] {
        if (ImGuiWindow* w = ImGui::FindWindowByName(insName().c_str())) { ImGui::SetWindowSize(w, ImVec2{ImGui::GetFontSize() * 32.0F, w->Size.y}); }
    }));
    add(pause(0.5));
    add(pruefe("UIF Into script: wieder frei schwebend, ohne waagrechten Rollbalken", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName(insName().c_str());
        return w != nullptr && w->WasActive && !w->DockIsActive && !w->ScrollbarX;
    }));
    // Funktion: Enter fuegt ein (ein set-Befehl ohne affect), Esc schliesst das Fenster
    auto knoten = std::make_shared<std::size_t>(0);
    add(tu("UIF Into script: SET_ANIM_BOTH, Fenster in den Vordergrund", [knoten] {
        g_app->mtab.insBefehl = 0;
        *knoten = g_app->doc.script().nodes.size();
        ImGui::SetWindowFocus(insName().c_str());
    }));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(pruefe("UIF Into script: Enter fuegt die Zeile ein (ein Befehl mehr, Meldung, der zweite Knopf heisst Schliessen)", [knoten] {
        return g_app->doc.script().nodes.size() == *knoten + 1 && g_app->mtab.insMeldung.find("uif.txt") != std::string::npos &&
               finde(schliessenText(), "modell.insskript", 0, false) != nullptr;
    }));
    add(tu("UIF Into script: Rueckgaengig", [knoten] {
        if (g_app->doc.script().nodes.size() > *knoten) { (void)g_app->doc.undo(); }
        rebuildTree();
        ImGui::SetWindowFocus(insName().c_str());
    }));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
    add(pruefe("UIF Into script: Esc schliesst das Fenster", [] { return !g_app->settings.insSkriptOffen; }));
    return s;
}

// Fenster der Bereiche oeffnen (alle nicht modalen zugleich): in Ruhe wie ohne Mindestgroessen, je Sprache auf 1 x 1, wieder normal.
std::vector<Schritt> mindestSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("UIF Mindest: Skript", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "uif.txt"); }));
    struct Bereich {
        std::string name;
        std::function<void()> auf;
        std::function<void()> zu;
        int mindestZahl;
    };
    const std::vector<Bereich> bereiche = {
        {"Events: Find, Notes, Shortcuts, Paths, Interplay, Layout, NPC-Browser, Animation, Wegpunkte, Kartenstatistik",
         [] {
             g_app->leftMode = 0;
             for (bool App::*o : {&App::findOpen, &App::messagesOpen, &App::keysOpen, &App::pathsOpen, &App::interplayOpen, &App::layoutOpen,
                                  &App::npcBrowserOffen, &App::animFensterOffen, &App::wegFensterOffen, &App::kartenStatistikOffen}) {
                 g_app->*o = true;
             }
         },
         [] {
             for (bool App::*o : {&App::findOpen, &App::messagesOpen, &App::keysOpen, &App::pathsOpen, &App::interplayOpen, &App::layoutOpen,
                                  &App::npcBrowserOffen, &App::animFensterOffen, &App::wegFensterOffen, &App::kartenStatistikOffen}) {
                 g_app->*o = false;
             }
         },
         6},
        {"Preferences", [] { g_app->prefsOpen = true; }, [] { g_app->prefsOpen = false; }, 1},
        {"About", [] { g_app->aboutOpen = true; }, [] { g_app->aboutOpen = false; }, 1},
        {"Files in .pk3", [] { g_app->pk3Open = true; }, [] { g_app->pk3Open = false; }, 1},
        {"Model: Into script, Rig-Uebersicht",
         [] {
             g_app->leftMode = 2;
             modellInsSkriptOeffnen(mb1::animNach("BOTH_WALK1"));
             g_app->mtab.uebersichtOffen = true;
         },
         [] {
             g_app->settings.insSkriptOffen = false;
             g_app->mtab.uebersichtOffen = false;
         },
         2},
        {"Map Editor: Global Preferences", [] { g_app->leftMode = 3; ke::editor().globaleEinstellungenOffen = true; },
         [] { ke::editor().globaleEinstellungenOffen = false; }, 1},
        {"Map Editor: About", [] { (void)ke::ausfuehren("DoAbout"); }, [] { ke::editor().ueberOffen = false; }, 1},
        {"Map Editor: Engine Path", [] { ke::editor().enginePfadDialogOffen = true; }, [] { ke::editor().enginePfadDialogOffen = false; }, 1},
        {"Map Editor: Font", [] { ke::editor().schriftDialogOffen = true; }, [] { ke::editor().schriftDialogOffen = false; }, 1},
        {"Map Editor: Undo-Liste", [] { ke::undoListeZeigen(); }, [] { ke::editor().undoListeOffen = false; }, 1},
    };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    for (const Bereich& b : bereiche) {
        auto ruhe = std::make_shared<std::map<std::string, ImVec2>>();
#ifdef BHED_UIF_BAUSTEIN
        // In Ruhe: erst ohne Mindestgroessen oeffnen und messen, dann einschalten - kein Fenster darf dadurch groesser werden.
        add(tu("UIF Mindest: Mindestgroessen aus, " + b.name + " auf", [b] {
            ui::mindestSchalter(false);
            b.auf();
        }));
        add(pause(0.7));
        add(tu("UIF Mindest: Groessen ohne Mindestgroessen merken, einschalten", [ruhe] {
            merke(*ruhe, schwebende());
            ui::mindestSchalter(true);
        }));
        add(pause(0.7));
        add(pruefe("UIF Mindest: " + b.name + " - in Ruhe kein Fenster groesser als ohne Mindestgroessen", [ruhe, b] {
            std::string fehler;
            for (const auto& [n, g] : *ruhe) {
                const ImGuiWindow* w = ImGui::FindWindowByName(n.c_str());
                if (w != nullptr && w->WasActive && (!nahe(w->Size.x, g.x, 0.5F) || !nahe(w->Size.y, g.y, 0.5F))) {
                    fehler += " {" + titel(w) + ": " + zahl(g.x) + "x" + zahl(g.y) + " -> " + zahl(w->Size.x) + "x" + zahl(w->Size.y) + "}";
                }
            }
            if (!fehler.empty()) { diag::detail("Knopftest: UIF in Ruhe groesser - " + b.name + ":" + fehler); }
            return fehler.empty() && static_cast<int>(ruhe->size()) >= b.mindestZahl;
        }));
#else
        add(tu("UIF Mindest: " + b.name + " auf", b.auf));
        add(pause(0.7));
        add(tu("UIF Mindest: Groessen merken", [ruhe] { merke(*ruhe, schwebende()); }));
#endif
        for (const char* sp : kSprachen) {
            const std::string spr = sp;
            add(tu("UIF Mindest: Sprache " + spr, [spr] { spracheSetzen(spr.c_str()); }));
            add(pause(0.4));
            quetschenUndPruefen(s, b.name + " (" + spr + ")", schwebende, b.mindestZahl);
        }
        add(tu("UIF Mindest: Englisch", [] { spracheSetzen("en"); }));
        add(pause(0.5));
        add(pruefe("UIF Mindest: " + b.name + " - nach allem wieder so gross wie am Anfang (Fenster, die das Programm nicht selbst bemisst)", [ruhe] {
            std::string fehler;
            for (const auto& [n, g] : *ruhe) {
                const ImGuiWindow* w = ImGui::FindWindowByName(n.c_str());
                if (w == nullptr || !w->WasActive || (w->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0) { continue; }
                if (!nahe(w->Size.x, g.x, 0.5F) || !nahe(w->Size.y, g.y, 0.5F)) {
                    fehler += " {" + titel(w) + ": " + zahl(g.x) + "x" + zahl(g.y) + " -> " + zahl(w->Size.x) + "x" + zahl(w->Size.y) + "}";
                }
            }
            if (!fehler.empty()) { diag::detail("Knopftest: UIF am Ende anders -" + fehler); }
            return fehler.empty();
        }));
        add(tu("UIF Mindest: " + b.name + " zu", b.zu));
        add(pause(0.3));
    }
    return s;
}

// Fotos: Into script normal und auf 1 x 1, dazu Find, Paths, Interplay, NPC-Browser, Wegpunkte, Kartenstatistik auf 1 x 1 - der
// Ausschnitt mindestens 1920 breit um das Fenster.
std::vector<Schritt> fotoSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto ausschnitt = [](const std::string& name) {
        return [name] {
            const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str());
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            if (w == nullptr) { return ImRect{0.0F, 0.0F, d.x, d.y}; }
            const float fs = ImGui::GetFontSize();
            ImRect r{w->Pos.x - fs, w->Pos.y - fs, w->Pos.x + w->Size.x + fs, w->Pos.y + w->Size.y + fs};
            const float breit = std::max(r.GetWidth(), std::min(1920.0F, d.x));
            const float cx = std::clamp(r.GetCenter().x, breit * 0.5F, std::max(breit * 0.5F, d.x - breit * 0.5F));
            const float hoch = std::max(r.GetHeight(), std::min(breit * 9.0F / 16.0F, d.y));
            const float cy = std::clamp(r.GetCenter().y, hoch * 0.5F, std::max(hoch * 0.5F, d.y - hoch * 0.5F));
            return ImRect{cx - breit * 0.5F, cy - hoch * 0.5F, cx + breit * 0.5F, cy + hoch * 0.5F};
        };
    };
    // BHED_HB_FENSTER=3840x2160@2 wie beim Handbuch: grosses Fenster, Oberflaeche doppelt (Inhalt gross)
    if (const char* gr = std::getenv("BHED_HB_FENSTER")) {
        int b = 0;
        int h = 0;
        float z = 1.0F;
        if (std::sscanf(gr, "%dx%d@%f", &b, &h, &z) >= 2 && b > 0 && h > 0) {
            add(tu("UIF Fotos: grosses Fenster", [b, h, z] {
                (void)platform::hauptfensterGroesse(b, h);
                g_app->uiScale = z;
            }));
            add(pause(0.5));
        }
    }
    add(tu("UIF Fotos: Skript, Modell", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "uif.txt"); }));
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(tu("UIF Fotos: Into script mit BOTH_WALK1, Figur kyle", [] {
        modellInsSkriptOeffnen(mb1::animNach("BOTH_WALK1"));
        std::snprintf(g_app->mtab.insFigur, sizeof g_app->mtab.insFigur, "%s", "kyle");
    }));
    add(pause(0.6));
    add(hb::fotoTeil("uif_insskript", ausschnitt(insName())));
    add(quetschen("Into script", [] { return std::vector<std::string>{insName()}; }));
    add(pause(0.6));
    add(hb::fotoTeil("uif_insskript_1x1", ausschnitt(insName())));
    add(tu("UIF Fotos: Into script zu, Events mit Find, Paths, Interplay, NPC-Browser, Wegpunkte", [] {
        g_app->settings.insSkriptOffen = false;
        g_app->leftMode = 0;
    }));
    struct Eins {
        const char* foto;
        const char* id;
        bool App::*offen;
    };
    const Eins fenster[] = {
        {"uif_keys_1x1", "###keys", &App::keysOpen},
        {"uif_paths_1x1", "###paths", &App::pathsOpen},
        {"uif_interplay_1x1", "###interplay", &App::interplayOpen},
        {"uif_npc_1x1", "###npcbrowser", &App::npcBrowserOffen},
        {"uif_wege_1x1", "###wegpunkte", &App::wegFensterOffen},
        {"uif_statistik_1x1", "###kartenstatistik", &App::kartenStatistikOffen},
    };
    for (const Eins& e : fenster) {
        const std::string id = e.id;
        bool App::*offen = e.offen;
        // der volle Name des Fensters (Titel + ###id)
        const auto name = [id] {
            for (const ImGuiWindow* w : GImGui->Windows) {
                if (w->WasActive && std::strstr(w->Name, id.c_str()) != nullptr && (w->Flags & ImGuiWindowFlags_ChildWindow) == 0) { return std::string(w->Name); }
            }
            return id;
        };
        add(tu(std::string("UIF Fotos: ") + e.id + " auf", [offen] { g_app->*offen = true; }));
        add(pause(0.5));
        add(quetschen(id, [name] { return std::vector<std::string>{name()}; }));
        add(pause(0.6));
        add(hb::fotoTeil(e.foto, [name, ausschnitt] { return ausschnitt(name())(); }));
        add(tu(std::string("UIF Fotos: ") + e.id + " zu", [offen] { g_app->*offen = false; }));
        add(pause(0.2));
    }
    return s;
}

#ifdef BHED_UIF_BAUSTEIN
// --- Probefenster: die Messung der Bedienreihen an bekanntem Inhalt ------------------------------------------------------
struct ProbeStand {
    ImVec2 nach{0.0F, 0.0F};     // Groesse einige Bilder nach dem Zusammenschieben
    ImVec2 spaeter{0.0F, 0.0F};  // und noch spaeter (gegen Pumpen)
    std::string befund;          // was die Pruefung am Fenster fand (leer: gut)
};

// Probetexte (die Knoepfe tragen nur Kennungen: ihre Breite ist fest)
const char* const kBeschriftung = "Beschriftung";
const char* const kLangeBeschriftung = "Eine lange Beschriftung vor dem Feld";
const char* const kLang = "Ein sehr langer Eintrag, der am Rand enden darf und nichts verlangt - 0123456789 0123456789 0123456789";

// Knopfreihe, Knopf hinter Text, dazu lange Listeneintraege, ein langer Baumknoten und ein langer Text.
void probeReihen() {
    const float fs = ImGui::GetFontSize();
    (void)ImGui::Button("##uifp_eins", ImVec2{fs * 8.0F, 0.0F});
    ImGui::SameLine();
    (void)ImGui::Button("##uifp_zwei", ImVec2{fs * 8.0F, 0.0F});
    ImGui::SameLine();
    (void)ImGui::Button("##uifp_drei", ImVec2{fs * 8.0F, 0.0F});
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(kBeschriftung);
    ImGui::SameLine();
    (void)ImGui::Button("##uifp_hinter", ImVec2{fs * 14.0F, 0.0F});
    for (int i = 0; i < 3; ++i) {
        ImGui::PushID(i);
        (void)ImGui::Selectable(kLang);
        ImGui::PopID();
    }
    if (ImGui::TreeNode(kLang)) { ImGui::TreePop(); }
    ImGui::TextUnformatted(kLang);
}

// Was die Bedienreihen von probeReihen brauchen: die rechte Kante der zwei letzten Knoepfe + Polster (+ Rollbalken). Gesucht wird
// ueber die Kennung: ein Knopf ganz hinter dem Rand ist im Register ohne Beschriftung (ImGui ItemAdd meldet ihn, schneidet ihn
// dann ab - der Knopf kommt nicht mehr zu seinem ItemInfo-Haken).
std::string probeReihenBefund(const ImGuiWindow* w, bool mitMindest) {
    const Element* drei = findeKennung(ImHashStr("##uifp_drei", 0, w->ID));
    const Element* hinter = findeKennung(ImHashStr("##uifp_hinter", 0, w->ID));
    if (drei == nullptr || hinter == nullptr) { return " [Knoepfe nicht gefunden]"; }
    const float rechts = std::max(drei->rect.Max.x, hinter->rect.Max.x);
    const float noetig = rechts - w->Pos.x + w->WindowPadding.x + (w->ScrollbarY ? ImGui::GetStyle().ScrollbarSize : 0.0F);
    const float lang = ImGui::CalcTextSize(kLang).x;
    std::string f;
    if (mitMindest) {
        if (rechts > w->WorkRect.Max.x + 1.0F) { f += " [Knopf ragt " + zahl(rechts - w->WorkRect.Max.x) + " hinaus]"; }
        if (w->Size.x > noetig + 2.0F) { f += " [breiter als die Bedienreihen brauchen: " + zahl(w->Size.x) + " statt " + zahl(noetig) + "]"; }
        if (w->Size.x >= lang) { f += " [so breit wie der lange Eintrag]"; }
    } else if (rechts <= w->WorkRect.Max.x + 1.0F) {
        f += " [ohne Mindestgroessen passt die Reihe - die Gegenprobe prueft nichts]";
    }
    return f;
}

// Ein Probefenster ueber ui::fensterBegin, 70 Bilder lang: Bild 8 auf 1 x 1 zusammenschieben, Bild 40 pruefen und messen, Bild 70
// noch einmal messen. breite: Startbreite in Zeichenhoehen; mindest: mit Mindestgroessen (false: Gegenprobe wie vor UIFORM).
Schritt probe(const std::string& text, const char* name, float breite, bool mindest, std::function<void()> inhalt,
              std::function<std::string(const ImGuiWindow*)> befund, std::shared_ptr<ProbeStand> st) {
    const std::string fenster = name;
    return {text, [=](int b) {
                if (b == 0) { ui::mindestSchalter(mindest); }
                const float fs = ImGui::GetFontSize();
                const ImGuiViewport* vp = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + fs * 4.0F, vp->WorkPos.y + fs * 4.0F}, ImGuiCond_Appearing);
                ImGui::SetNextWindowSize(ImVec2{fs * breite, fs * 16.0F}, ImGuiCond_Appearing);
                if (ui::fensterBegin(fenster.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing)) { inhalt(); }
                ImGui::End();
                ImGuiWindow* w = ImGui::FindWindowByName(fenster.c_str());
                if (w == nullptr) { return true; }
                if (b == 8) { ImGui::SetWindowSize(w, ImVec2{1.0F, 1.0F}); }
                if (b == 40) {
                    st->nach = w->Size;
                    st->befund = befund ? befund(w) : std::string();
                }
                if (b < 70) { return false; }
                st->spaeter = w->Size;
                ui::mindestSchalter(true);
                return true;
            }};
}

std::vector<Schritt> probeSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto melde = [](const std::string& was, const ProbeStand& st) {
        diag::detail("Knopftest: UIF Probe " + was + ": " + zahl(st.nach.x) + "x" + zahl(st.nach.y) + ", spaeter " + zahl(st.spaeter.x) + "x" + zahl(st.spaeter.y) +
                     (st.befund.empty() ? std::string() : " | " + st.befund));
    };
    // 1. Gegenprobe: ohne Mindestgroessen bleibt das Fenster auf ImGuis kleinster Groesse - zu schmal fuer seine Knopfreihe
    auto ohne = std::make_shared<ProbeStand>();
    add(probe("UIF Probe: Knopfreihe ohne Mindestgroessen (Gegenprobe)", "Probe A###uifprobea", 40.0F, false, probeReihen,
              [](const ImGuiWindow* w) { return probeReihenBefund(w, false); }, ohne));
    add(pruefe("UIF Probe: ohne Mindestgroessen ragt die Knopfreihe aus dem zusammengeschobenen Fenster (Gegenprobe)", [ohne, melde] {
        melde("ohne Mindestgroessen", *ohne);
        return ohne->befund.empty() && ohne->nach.x > 0.0F;
    }));
    // 2. mit: genau so breit, wie Knopfreihe und Knopf hinter Text brauchen - nicht so breit wie Liste, Baum und Text
    auto mit = std::make_shared<ProbeStand>();
    add(probe("UIF Probe: Knopfreihe mit Mindestgroessen", "Probe B###uifprobeb", 40.0F, true, probeReihen,
              [](const ImGuiWindow* w) { return probeReihenBefund(w, true); }, mit));
    add(pruefe("UIF Probe: zusammengeschoben passt die Knopfreihe ganz hinein, lange Liste, Baum und Text verlangen nichts, ruhig", [mit, melde] {
        melde("Knopfreihe", *mit);
        return mit->befund.empty() && mit->nach.x > 0.0F && nahe(mit->nach.x, mit->spaeter.x, 0.5F) && nahe(mit->nach.y, mit->spaeter.y, 0.5F);
    }));
    // 3. Ein Fenster, das nie so breit war, wie seine Reihe braucht, wird durch die Messung nicht groesser
    auto klein = std::make_shared<ProbeStand>();
    add(probe("UIF Probe: Fenster, das nie breit genug war", "Probe C###uifprobec", 12.0F, true, probeReihen, nullptr, klein));
    add(pruefe("UIF Probe: ein Fenster, das nie so breit war wie seine Knopfreihe, wird nicht groesser als es war", [klein, melde] {
        melde("nie breit genug", *klein);
        const float fs = ImGui::GetFontSize();
        return klein->nach.x >= fs * 10.0F - 1.0F && klein->nach.x <= fs * 12.0F + 0.5F && nahe(klein->nach.x, klein->spaeter.x, 0.5F);
    }));
    // 4. Inhalt, der sich an der Fensterbreite ausrichtet und immer hinausragt: das Fenster waechst nicht zurueck
    auto waechst = std::make_shared<ProbeStand>();
    add(probe("UIF Probe: Inhalt, der mit dem Fenster mitwaechst", "Probe D###uifprobed", 40.0F, true,
              [] {
                  const float fs = ImGui::GetFontSize();
                  (void)ImGui::Button("##uifp_links", ImVec2{fs * 6.0F, 0.0F});
                  ImGui::SameLine(ImGui::GetWindowWidth() - fs * 4.0F);
                  (void)ImGui::Button("##uifp_ragt", ImVec2{fs * 6.0F, 0.0F});
              },
              nullptr, waechst));
    add(pruefe("UIF Probe: mitwachsender Inhalt - das Fenster bleibt klein und ruhig (hoechstens drei Anhebungen)", [waechst, melde] {
        melde("mitwachsend", *waechst);
        const float fs = ImGui::GetFontSize();
        return waechst->nach.x >= fs * 10.0F - 1.0F && waechst->nach.x < fs * 26.0F && nahe(waechst->nach.x, waechst->spaeter.x, 0.5F);
    }));
    // 5. Ein Eingabefeld hinter seiner Beschriftung, das mit dem Fenster schrumpft, behaelt 6 Zeichenhoehen
    auto feld = std::make_shared<ProbeStand>();
    add(probe("UIF Probe: Eingabefeld hinter seiner Beschriftung", "Probe E###uifprobee", 40.0F, true,
              [] {
                  static char text[32] = "abc";
                  ImGui::AlignTextToFramePadding();
                  ImGui::TextUnformatted(kLangeBeschriftung);
                  ImGui::SameLine();
                  ImGui::SetNextItemWidth(-1.0F);
                  (void)ImGui::InputText("##uifpfeld", text, sizeof text);
              },
              [](const ImGuiWindow* w) {
                  const Element* e = findeKennung(ImHashStr("##uifpfeld", 0, w->ID));
                  if (e == nullptr) { return std::string(" [Feld nicht gefunden]"); }
                  const float mindest = ImGui::GetFontSize() * 6.0F - 1.0F;
                  std::string f;
                  if (e->rect.GetWidth() < mindest) { f += " [Feld " + zahl(e->rect.GetWidth()) + " schmaler als " + zahl(mindest) + "]"; }
                  if (e->rect.Max.x > w->WorkRect.Max.x + 1.0F) { f += " [Feld ragt hinaus]"; }
                  return f;
              },
              feld));
    add(pruefe("UIF Probe: das Eingabefeld hinter seiner Beschriftung behaelt 6 Zeichenhoehen und bleibt ganz im Fenster", [feld, melde] {
        melde("Eingabefeld", *feld);
        return feld->befund.empty() && feld->nach.x > 0.0F && nahe(feld->nach.x, feld->spaeter.x, 0.5F);
    }));
    return s;
}
// --- Schwebende Panels und Tabs in eigenen Fenstern ---------------------------------------------------------------------
// Keine Mindestgroesse bekommt nur ein Fenster, das ein Dock-Knoten anlegt (ui_formular.cpp knotenLegt). Ein Panel in einem
// eigenen Fenster und ein Tab in einem eigenen Fenster bestimmen ihre Groesse selbst - fuer sie gilt die Untergrenze weiter.
struct SchwebStand {
    bool gefunden = false;
    bool schwebt = false;          // kein Dock-Knoten legt das Fenster an
    ImVec2 heim{0.0F, 0.0F};       // Groesse am Platz im Hauptfenster (nur Panels)
    ImVec2 vorher{0.0F, 0.0F};     // Groesse im eigenen Fenster
    ImVec2 ohne{0.0F, 0.0F};       // auf 1 x 1 ohne Mindestgroessen (Gegenprobe)
    ImVec2 mit{0.0F, 0.0F};        // auf 1 x 1 mit Mindestgroessen
    ImVec2 spaeter{0.0F, 0.0F};    // einige Bilder danach (gegen Pumpen)
    ImVec2 grenze{0.0F, 0.0F};     // ui::mindestVon
    float eineZeile = 0.0F;        // Titelzeile + Menueleiste + eine Zeile
    std::string name;
};

// 40 Bilder: Bild 0 ohne Mindestgroessen auf 1 x 1, Bild 10 messen und mit Mindestgroessen auf 1 x 1, Bild 25 messen, Bild 40
// noch einmal messen und die Groesse von vorher zurueck.
Schritt schwebendQuetschen(const std::string& text, std::function<ImGuiWindow*()> fenster, std::shared_ptr<SchwebStand> st) {
    return {text, [=](int b) {
                ImGuiWindow* w = fenster();
                if (w == nullptr) {
                    ui::mindestSchalter(true);
                    return true;
                }
                if (b == 0) {
                    st->gefunden = true;
                    st->name = w->Name;
                    st->vorher = w->Size;
                    st->schwebt = w->WasActive && !w->DockIsActive && w->RootWindowDockTree == w;
                    ui::mindestSchalter(false);
                    ImGui::SetWindowSize(w, ImVec2{1.0F, 1.0F});
                }
                if (b == 10) {
                    st->ohne = w->Size;
                    ui::mindestSchalter(true);
                    ImGui::SetWindowSize(w, ImVec2{1.0F, 1.0F});
                }
                if (b == 25) {
                    st->mit = w->Size;
                    st->grenze = ui::mindestVon(w->Name);
                    st->eineZeile = w->TitleBarHeight + w->MenuBarHeight + ImGui::GetFrameHeight();
                }
                if (b < 40) { return false; }
                st->spaeter = w->Size;
                ImGui::SetWindowSize(w, st->vorher);
                return true;
            }};
}

Schritt schwebendPruefen(const std::string& was, std::shared_ptr<SchwebStand> st) {
    return pruefe("UIF schwebend: " + was + " - auf 1 x 1 auf der Untergrenze (Titelzeile, eine Zeile; ohne Mindestgroessen kleiner), nicht groesser als "
                  "vorher, ruhig",
                  [st, was] {
                      const float fs = ImGui::GetFontSize();
                      const auto g = [](const ImVec2& v) { return zahl(v.x) + "x" + zahl(v.y); };
                      diag::detail("Knopftest: UIF schwebend " + was + " [" + st->name + "]: " + (st->schwebt ? "eigenes Fenster" : "NICHT schwebend") + ", vorher " +
                                   g(st->vorher) + ", ohne Mindestgroessen " + g(st->ohne) + ", mit " + g(st->mit) + ", spaeter " + g(st->spaeter) +
                                   ", Untergrenze " + g(st->grenze) + ", Titel und eine Zeile " + zahl(st->eineZeile));
                      const bool unten = st->mit.x >= fs * 10.0F - 1.0F && st->mit.y >= st->eineZeile - 0.5F && st->grenze.x > 0.0F &&
                                         nahe(st->mit.x, st->grenze.x) && nahe(st->mit.y, st->grenze.y);
                      const bool gegenprobe = st->ohne.x < st->mit.x - 1.0F && st->ohne.y < st->mit.y - 1.0F;
                      return st->gefunden && st->schwebt && unten && gegenprobe && st->mit.x <= st->vorher.x + 0.5F && st->mit.y <= st->vorher.y + 0.5F &&
                             nahe(st->mit.x, st->spaeter.x, 0.5F) && nahe(st->mit.y, st->spaeter.y, 0.5F);
                  });
}

std::vector<Schritt> schwebendSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto raumVorher = std::make_shared<bool>(true);
    auto modusVorher = std::make_shared<int>(0);
    add(tu("UIF schwebend: DockSpace an", [raumVorher, modusVorher] {
        *raumVorher = dockraumAn();
        *modusVorher = g_app->leftMode;
        dockraumAn() = true;
    }));
    struct Fall {
        const char* was;
        const char* panel;
        int bereich;
    };
    // je ein Panel der beiden Stellen, die Panels beginnen: dockvorgabe.cpp (Map Editor) und app_dockspace.inc (Mission)
    for (const Fall& f : {Fall{"Map Editor XY", "karte.xy", 3}, Fall{"Mission Script Flow", "mission.fluss", 1}}) {
        const std::string was = f.was;
        const std::string panel = f.panel;
        const int bereich = f.bereich;
        auto st = std::make_shared<SchwebStand>();
        add(tu("UIF schwebend: " + was + " - Bereich zeigen", [bereich] { g_app->leftMode = bereich; }));
        add(pause(0.8));
        add(tu("UIF schwebend: " + was + " - Groesse am Platz merken, in ein eigenes Fenster", [panel, st] {
            if (const ImGuiWindow* w = dock1::panelFenster(panel.c_str())) { st->heim = w->Size; }
            dock::herausnehmen(panel.c_str());
        }));
        add(pause(0.8));
        add(schwebendQuetschen("UIF schwebend: " + was + " im eigenen Fenster auf 1 x 1", [panel] { return dock1::panelFenster(panel.c_str()); }, st));
        add(schwebendPruefen(was, st));
        add(tu("UIF schwebend: " + was + " wieder andocken", [panel] { dock::andocken(panel.c_str()); }));
        add(pause(0.8));
        add(pruefe("UIF schwebend: " + was + " - wieder angedockt, so gross wie sein Platz vorher", [panel, st, was] {
            const ImGuiWindow* w = dock1::panelFenster(panel.c_str());
            if (w == nullptr) { return false; }
            diag::detail("Knopftest: UIF schwebend " + was + " angedockt: " + (w->DockIsActive ? "im Knoten " : "NICHT im Knoten ") + zahl(w->Size.x) + "x" +
                         zahl(w->Size.y) + ", vorher " + zahl(st->heim.x) + "x" + zahl(st->heim.y));
            return w->DockIsActive && !dock::draussen(panel.c_str()) && st->heim.x > 0.0F && nahe(w->Size.x, st->heim.x) && nahe(w->Size.y, st->heim.y);
        }));
    }
    // Ein ganzer Tab in einem eigenen Fenster (tabfenster.cpp): Titelzeile und Menueleiste
    auto tab = std::make_shared<SchwebStand>();
    add(tu("UIF schwebend: Hauptfenster Events, Model in ein eigenes Fenster", [] {
        g_app->leftMode = 0;
        (void)tabfenster::loesen(2);
    }));
    add(pause(0.8));
    add(schwebendQuetschen("UIF schwebend: Tab Model im eigenen Fenster auf 1 x 1", [] { return tabfenster::fenster(2); }, tab));
    add(schwebendPruefen("Tab Model", tab));
    add(tu("UIF schwebend: Model zurueck ins Hauptfenster, Bereich und DockSpace wie vorher", [raumVorher, modusVorher] {
        tabfenster::zurueck(2);
        g_app->leftMode = *modusVorher;
        dockraumAn() = *raumVorher;
    }));
    add(pause(0.5));
    add(pruefe("UIF schwebend: der Tab Model ist zurueck im Hauptfenster", [] { return !tabfenster::eigenes(2) && tabfenster::fenster(2) == nullptr; }));
    return s;
}
#endif

std::vector<Schritt> schritte() {
    const char* nur = std::getenv("BHED_UIF_NUR");
    const std::string n = nur != nullptr ? nur : "";
    std::vector<Schritt> s;
#ifdef BHED_UIF_BAUSTEIN
    if (n.empty() || n == "probe") {
        for (Schritt& x : probeSchritte()) { s.push_back(std::move(x)); }
    }
    if (n.empty() || n == "schwebend") {
        for (Schritt& x : schwebendSchritte()) { s.push_back(std::move(x)); }
    }
#endif
    if (n.empty() || n == "insskript") {
        for (Schritt& x : insskriptSchritte()) { s.push_back(std::move(x)); }
    }
    if (n.empty() || n == "mindest") {
        for (Schritt& x : mindestSchritte()) { s.push_back(std::move(x)); }
    }
    if (n == "fotos") {
        for (Schritt& x : fotoSchritte()) { s.push_back(std::move(x)); }
    }
    return s;
}

}  // namespace uif
