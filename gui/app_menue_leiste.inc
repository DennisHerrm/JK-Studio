// app_menue_leiste.inc - rc608 Teil 3a: die rechte Seitenleiste des Menue-Editors nach dem freigegebenen Entwurf
// (in app_menue.cpp eingebunden). Oben eine feste Karte, darunter Felder in Gruppen bzw. Ausloeser und Schritte, unten
// der Fuss mit Component, Preview cvars und Engine messages und die Hilfezeile. Blau heisst nur: gewaehlt.
// Geschrieben wird ueber die bisherigen Wege (eigenschaft, setzeRects, ereignisSchreiben), jede Aenderung ist ein Rueckgaengig-Schritt.

namespace {

constexpr ImU32 kLsFeld = IM_COL32(0x30, 0x32, 0x3A, 255);
constexpr ImU32 kLsFeldUeber = IM_COL32(0x3F, 0x42, 0x4C, 255);
constexpr ImU32 kLsKarte = IM_COL32(0x26, 0x28, 0x2D, 255);
constexpr ImU32 kLsLinie = IM_COL32(0x3A, 0x3C, 0x44, 255);
constexpr ImU32 kLsBlau = IM_COL32(0x27, 0x4A, 0x75, 255);
constexpr ImU32 kLsBlauHell = IM_COL32(0x5A, 0x9B, 0xEA, 255);
constexpr ImU32 kLsLeise = IM_COL32(0xA6, 0xA9, 0xB1, 255);
constexpr ImU32 kLsGefahr = IM_COL32(0xEF, 0x8A, 0x80, 255);
constexpr ImU32 kLsGelb = IM_COL32(0xE8, 0xB0, 0x4A, 255);

enum class Ls { Auge, Zu, Vor, Hin, Dup, Weg, Hoch, Runter, X, Links, Mitte, Rechts, Oben, MitteV, Unten, Text, Knopf, Modell, Regler, Menue, Gruppe, Spiel, Bild, Video, Stift };

void lsSymbol(ImDrawList* dl, Ls s, ImVec2 mitte, float groesse, ImU32 c) {
    const float k = groesse / 16.0F;
    const ImVec2 o{mitte.x - groesse * 0.5F, mitte.y - groesse * 0.5F};
    const auto p = [&](float x, float y) { return ImVec2{o.x + x * k, o.y + y * k}; };
    const float dicke = std::max(1.0F, 1.4F * k);
    const auto linie = [&](std::initializer_list<ImVec2> pkte) {
        std::vector<ImVec2> v;
        for (const ImVec2& q : pkte) { v.push_back(p(q.x, q.y)); }
        dl->AddPolyline(v.data(), static_cast<int>(v.size()), c, ImDrawFlags_None, dicke);
    };
    const auto kasten = [&](float x, float y, float w, float h) { dl->AddRect(p(x, y), p(x + w, y + h), c, k, 0, dicke); };
    switch (s) {
        case Ls::Auge:
            dl->AddBezierQuadratic(p(1.5F, 8), p(8, -0.5F), p(14.5F, 8), c, dicke);
            dl->AddBezierQuadratic(p(14.5F, 8), p(8, 16.5F), p(1.5F, 8), c, dicke);
            dl->AddCircle(p(8, 8), 2.0F * k, c, 0, dicke);
            break;
        case Ls::Zu:
            kasten(3.5F, 7, 9, 6.5F);
            dl->PathArcTo(p(8, 5), 2.5F * k, IM_PI, IM_PI * 2.0F);
            dl->PathStroke(c, ImDrawFlags_None, dicke);
            linie({{5.5F, 5}, {5.5F, 7}});
            linie({{10.5F, 5}, {10.5F, 7}});
            break;
        case Ls::Vor:
            kasten(2, 6, 8, 8);
            linie({{6, 3.5F}, {6, 2}, {14, 2}, {14, 10}, {12.5F, 10}});
            break;
        case Ls::Hin:
            kasten(6, 2, 8, 8);
            linie({{4.5F, 6}, {2, 6}, {2, 14}, {10, 14}, {10, 11.5F}});
            break;
        case Ls::Dup:
            kasten(5, 5, 9, 9);
            linie({{11, 3}, {11, 2}, {2, 2}, {2, 11}, {3, 11}});
            break;
        case Ls::Weg:
            linie({{3, 4.5F}, {13, 4.5F}});
            linie({{6.5F, 4.5F}, {6.5F, 2.8F}, {9.5F, 2.8F}, {9.5F, 4.5F}});
            linie({{4.5F, 4.5F}, {5.2F, 13.5F}, {10.8F, 13.5F}, {11.5F, 4.5F}});
            break;
        case Ls::Hoch: linie({{3.5F, 10}, {8, 5.5F}, {12.5F, 10}}); break;
        case Ls::Runter: linie({{3.5F, 6}, {8, 10.5F}, {12.5F, 6}}); break;
        case Ls::X:
            linie({{4, 4}, {12, 12}});
            linie({{12, 4}, {4, 12}});
            break;
        case Ls::Links:
            linie({{2, 3.5F}, {14, 3.5F}});
            linie({{2, 6.5F}, {9, 6.5F}});
            linie({{2, 9.5F}, {14, 9.5F}});
            linie({{2, 12.5F}, {9, 12.5F}});
            break;
        case Ls::Mitte:
            linie({{2, 3.5F}, {14, 3.5F}});
            linie({{4.5F, 6.5F}, {11.5F, 6.5F}});
            linie({{2, 9.5F}, {14, 9.5F}});
            linie({{4.5F, 12.5F}, {11.5F, 12.5F}});
            break;
        case Ls::Rechts:
            linie({{2, 3.5F}, {14, 3.5F}});
            linie({{7, 6.5F}, {14, 6.5F}});
            linie({{2, 9.5F}, {14, 9.5F}});
            linie({{7, 12.5F}, {14, 12.5F}});
            break;
        case Ls::Oben:
            linie({{2, 2.5F}, {14, 2.5F}});
            kasten(5.5F, 4.5F, 5, 6);
            break;
        case Ls::MitteV:
            linie({{2, 8}, {5.5F, 8}});
            linie({{10.5F, 8}, {14, 8}});
            kasten(5.5F, 5, 5, 6);
            break;
        case Ls::Unten:
            linie({{2, 13.5F}, {14, 13.5F}});
            kasten(5.5F, 5.5F, 5, 6);
            break;
        case Ls::Text:
            linie({{3, 4}, {3, 2.8F}, {13, 2.8F}, {13, 4}});
            linie({{8, 2.8F}, {8, 13.2F}});
            linie({{6, 13.2F}, {10, 13.2F}});
            break;
        case Ls::Knopf:
            dl->AddRect(p(1.5F, 4.5F), p(14.5F, 11.5F), c, 2.0F * k, 0, dicke);
            linie({{5, 8}, {11, 8}});
            break;
        case Ls::Modell:
            dl->AddCircle(p(8, 3.5F), 2.0F * k, c, 0, dicke);
            linie({{4, 8}, {6, 6.2F}, {10, 6.2F}, {12, 8}});
            linie({{6, 6.5F}, {6, 10.5F}, {5.5F, 14}});
            linie({{10, 6.5F}, {10, 10.5F}, {10.5F, 14}});
            break;
        case Ls::Regler:
            linie({{1.5F, 8}, {14.5F, 8}});
            kasten(6.5F, 4.5F, 3, 7);
            break;
        case Ls::Menue:
            kasten(2, 2.5F, 12, 11);
            linie({{2, 5.5F}, {14, 5.5F}});
            linie({{4.5F, 8}, {11.5F, 8}});
            linie({{4.5F, 10.5F}, {9.5F, 10.5F}});
            break;
        case Ls::Gruppe:
            kasten(1.5F, 1.5F, 8, 8);
            kasten(6.5F, 6.5F, 8, 8);
            break;
        case Ls::Spiel: dl->AddTriangleFilled(p(5, 3.5F), p(5, 12.5F), p(12.5F, 8), c); break;
        case Ls::Bild:
            kasten(2, 3, 12, 10);
            linie({{3, 12}, {6.5F, 8}, {9, 11}, {10.5F, 9.5F}, {13, 12}});
            break;
        case Ls::Stift:
            linie({{3, 13}, {3.6F, 10.3F}, {10.8F, 3}, {13, 5.2F}, {5.7F, 12.4F}, {3, 13}});
            break;
        case Ls::Video:
            kasten(2, 3, 12, 10);
            dl->AddTriangleFilled(p(6.5F, 5.8F), p(6.5F, 10.2F), p(10.2F, 8), c);
            break;
    }
}

// Was das Feld unter dem Zeiger bewirkt: sofort in der Hilfezeile, dazu der Tooltip wie bisher
void lsTipp(const char* tip) {
    if (tip == nullptr || tip[0] == '\0') { return; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { Z().leiste.hilfeNeu = tip; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", tip); }
}

bool lsKnopf(const char* id, Ls s, const char* tip, bool an = false, bool gefahr = false, bool aktiv = true, float g = 0.0F) {
    const float h = g > 0.0F ? g : std::floor(ImGui::GetFrameHeight() * 1.05F);
    ImGui::BeginDisabled(!aktiv);
    const bool k = ImGui::InvisibleButton(id, ImVec2{h, h});
    ImGui::EndDisabled();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool ueber = aktiv && ImGui::IsItemHovered();
    dl->AddRectFilled(a, b, an ? kLsBlau : ueber ? kLsFeldUeber : kLsFeld, 4.0F);
    const ImU32 c = !aktiv ? IM_COL32(0xE6, 0xE7, 0xEA, 90) : gefahr ? kLsGefahr : IM_COL32(0xE6, 0xE7, 0xEA, 255);
    lsSymbol(dl, s, ImVec2{(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F}, std::floor(h * 0.62F), c);
    lsTipp(tip);
    return k;
}

// Umschalter: dicht an dicht, das Gewaehlte blau. Liefert den angeklickten Eintrag, sonst -1.
struct LsWahl {
    const char* id;     // "##..." bei Symbolen, sonst der Text
    const char* tip;
    bool mitSymbol = false;
    Ls sym = Ls::Text;
};
float lsWahlBreite(const std::vector<LsWahl>& e) {
    float w = 0;
    for (const LsWahl& x : e) { w += x.mitSymbol ? std::floor(ImGui::GetFrameHeight() * 1.05F) : knopfBreite(x.id); }
    return w;
}
int lsUmschalter(const char* gruppe, const std::vector<LsWahl>& e, int gewaehlt) {
    int klick = -1;
    ImGui::PushID(gruppe);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, ImGui::GetStyle().ItemSpacing.y});
    for (int i = 0; i < static_cast<int>(e.size()); ++i) {
        const LsWahl& x = e[static_cast<std::size_t>(i)];
        if (i > 0) { daneben(x.mitSymbol ? ImGui::GetFrameHeight() : knopfBreite(x.id)); }
        if (x.mitSymbol) {
            if (lsKnopf(x.id, x.sym, x.tip, i == gewaehlt)) { klick = i; }
        } else {
            const bool an = i == gewaehlt;
            ImGui::PushStyleColor(ImGuiCol_Button, an ? kLsBlau : kLsFeld);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, an ? kLsBlau : kLsFeldUeber);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, kLsBlau);
            if (ImGui::Button(x.id)) { klick = i; }
            ImGui::PopStyleColor(3);
            lsTipp(x.tip);
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return klick;
}

// --- Geaenderte Werte markieren und einzeln zuruecksetzen ------------------------------------------------------

const Item& itemVorgabe(const Datei& d) {
    Zustand& z = Z();
    const int nr = (d.dialekt == Dialekt::MP ? 1 : 0) + 2 * static_cast<int>(d.spiel);
    auto it = z.leiste.vorgabe.find(nr);
    if (it == z.leiste.vorgabe.end()) {
        LeseOptionen o;
        o.dialekt = d.dialekt;
        o.spiel = d.spiel;
        const Datei p = lies("ui/vorgabe.menu", "{ menuDef { itemDef { } } }", o);
        it = z.leiste.vorgabe.emplace(nr, !p.menus.empty() && !p.menus[0].items.empty() ? p.menus[0].items[0] : Item{}).first;
    }
    return it->second;
}

bool lsFarbeGleich(const Farbe& a, const Farbe& b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

// Weicht der Wert vom Item_Init des Spiels ab? Inhalt (rect, name, text, background, group, cvar) traegt keine Marke, er hat keine sinnvolle Vorgabe.
bool wertAnders(const Item& it, const Item& v, const std::string& k) {
    if (k == "textscale") { return it.textscale != v.textscale; }
    if (k == "textalign") { return it.textalignment != v.textalignment; }
    if (k == "textalignx") { return it.textalignx != v.textalignx; }
    if (k == "textaligny") { return it.textaligny != v.textaligny; }
    if (k == "textstyle") { return it.textStyle != v.textStyle; }
    if (k == "font") { return it.font != v.font; }
    if (k == "forecolor") { return !lsFarbeGleich(it.window.foreColor, v.window.foreColor); }
    if (k == "backcolor") { return !lsFarbeGleich(it.window.backColor, v.window.backColor); }
    if (k == "bordercolor") { return !lsFarbeGleich(it.window.borderColor, v.window.borderColor); }
    if (k == "border") { return it.window.border != v.window.border; }
    if (k == "style") { return it.window.style != v.window.style; }
    if (k == "model_angle") { return it.angle != v.angle; }
    if (k == "model_rotation") { return it.rotationSpeed != v.rotationSpeed; }
    if (k == "model_fovx") { return it.fovx != v.fovx; }
    if (k == "model_fovy") { return it.fovy != v.fovy; }
    return false;
}

std::vector<std::string> schluesselListe(const char* keys) {
    std::vector<std::string> v;
    std::string w;
    for (const char* c = keys; ; ++c) {
        if (*c == ',' || *c == ' ' || *c == '\0') {
            if (!w.empty()) { v.push_back(w); }
            w.clear();
            if (*c == '\0') { break; }
        } else {
            w += *c;
        }
    }
    return v;
}

void aenderungsMarke(const char* keys, ImVec2 stelle, float g) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    Dokument* d = doc();
    if (l.markeItem < 0 || d == nullptr || keys == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0 || static_cast<std::size_t>(l.markeItem) >= d->datei.menus[static_cast<std::size_t>(mi)].items.size()) { return; }
    const Item& it = d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(l.markeItem)];
    const Item& v = itemVorgabe(d->datei);
    std::vector<std::string> weg;
    bool anders = false;
    for (const std::string& k : schluesselListe(keys)) {
        if (finde(it, k.c_str()) == nullptr) { continue; }
        weg.push_back(k);
        anders = anders || wertAnders(it, v, k);
    }
    if (!anders) { return; }
    const ImVec2 zurueck = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(stelle);
    ImGui::PushID(keys);
    const bool klick = ImGui::InvisibleButton("##zurueck", ImVec2{g, g});
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (ImGui::IsItemHovered()) { dl->AddRectFilled(a, b, kLsFeldUeber, 3.0F); }
    uiIcon(dl, UiIcon::Rueckgaengig, ImVec2{a.x + 1.0F, a.y + 1.0F}, g - 2.0F, ImGui::GetColorU32(ImGuiCol_Text));
    lsTipp(tr(Str::MlZuruecksetzenTip));
    ImGui::PopID();
    ImGui::SetCursorScreenPos(zurueck);
    if (klick) {
        std::vector<Ersetzung> e;
        bool gut = true;
        for (const std::string& k : weg) { gut = entferneEigenschaft(d->datei, mi, l.markeItem, k.c_str(), e) && gut; }
        if (gut) { aendere(std::move(e)); }
        else { ausMakroMelden(); }
    }
}

// --- Gruppen ------------------------------------------------------------------------------------------------

// Der Kopf einer Gruppe: Pfeil, Titel und, zugeklappt, rechts die Zusammenfassung. true = angeklickt
bool gruppenKopf(const char* id, const char* titel, const std::string& zus, bool offen) {
    const float fs = ImGui::GetFontSize();
    const std::string kennung = std::string("##gr_") + id;
    const float breit = ImGui::GetContentRegionAvail().x;
    const float h = std::floor(fs * 1.75F);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0x2A, 0x2C, 0x32, 255));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0x2A, 0x2C, 0x32, 255));
    const bool klick = ImGui::Selectable(kennung.c_str(), false, ImGuiSelectableFlags_NoPadWithHalfSpacing, ImVec2{breit, h});
    ImGui::PopStyleColor(2);
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float my = (a.y + b.y) * 0.5F;
    const float pf = fs * 0.28F;
    const float px = a.x + fs * 0.45F;
    if (offen) { dl->AddTriangleFilled(ImVec2{px - pf, my - pf * 0.6F}, ImVec2{px + pf, my - pf * 0.6F}, ImVec2{px, my + pf * 0.8F}, kLsLeise); }
    else { dl->AddTriangleFilled(ImVec2{px - pf * 0.6F, my - pf}, ImVec2{px + pf * 0.8F, my}, ImVec2{px - pf * 0.6F, my + pf}, kLsLeise); }
    const ImVec2 tp{a.x + fs * 1.0F, my - ImGui::GetTextLineHeight() * 0.5F};
    const float tw = ImGui::CalcTextSize(titel).x;
    dl->AddText(tp, ImGui::GetColorU32(ImGuiCol_Text), titel);
    if (!offen && !zus.empty()) {
        const float links = tp.x + tw + fs * 0.75F;
        const float rechts = b.x - fs * 0.25F;
        if (rechts - links > fs) {
            const std::string& s = zus;
            const float sw = ImGui::CalcTextSize(s.c_str()).x;
            const ImVec2 sp{std::max(links, rechts - sw), tp.y};
            ImGui::PushStyleColor(ImGuiCol_Text, kLsLeise);
            ImGui::RenderTextEllipsis(dl, sp, ImVec2{rechts, tp.y + ImGui::GetTextLineHeight() + 1.0F}, rechts, s.c_str(), nullptr, nullptr);
            ImGui::PopStyleColor();
        }
    }
    return klick;
}

// Eine Gruppe: Kopf mit Pfeil und, zugeklappt, rechts der Zusammenfassung. Der Inhalt laeuft erst einmal nur zum Zaehlen: So
// kennt die Liste "felderGezeigt" jedes Feld, auch zugeklappt, und die Suche blendet Gruppen ohne Treffer aus.
void gruppe(const char* id, const char* titel, const std::string& zus, const std::function<void()>& inhalt, bool zaehlbar) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    lsZeilenEnde();
    int treffer = 0;
    if (zaehlbar) {
        l.zaehlen = true;
        l.treffer = 0;
        inhalt();
        l.zaehlen = false;
        treffer = l.treffer;
    }
    const bool sucht = !l.such.empty();
    if (sucht && (!zaehlbar || treffer == 0)) { return; }
    l.trefferGesamt += treffer;
    const bool offen = sucht || l.zu.count(id) == 0;
    const float fs = ImGui::GetFontSize();
    if (gruppenKopf(id, titel, zus, offen) && !sucht) {
        if (l.zu.count(id) != 0) { l.zu.erase(id); } else { l.zu.insert(id); }
    }
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (offen) {
        ImGui::PushID(id);
        inhalt();
        lsZeilenEnde();
        ImGui::PopID();
        ImGui::Dummy(ImVec2{1.0F, fs * 0.2F});
    }
    const ImVec2 u = ImGui::GetCursorScreenPos();
    dl->AddLine(ImVec2{a.x, u.y - 1.0F}, ImVec2{b.x, u.y - 1.0F}, kLsLinie);
    ImGui::Dummy(ImVec2{1.0F, 1.0F});
}

// --- Farben ---------------------------------------------------------------------------------------------------

std::vector<Farbe> letzteFarben() {
    std::vector<Farbe> v;
    std::istringstream s(g_app->settings.menueLetzteFarben);
    std::string teil;
    while (std::getline(s, teil, ';')) {
        Farbe f;
        if (std::sscanf(teil.c_str(), "%f %f %f %f", &f.r, &f.g, &f.b, &f.a) == 4) { v.push_back(f); }
    }
    return v;
}

void farbeMerken(const Farbe& f) {
    std::vector<Farbe> v = letzteFarben();
    v.erase(std::remove_if(v.begin(), v.end(), [&f](const Farbe& x) { return lsFarbeGleich(x, f); }), v.end());
    v.insert(v.begin(), f);
    if (v.size() > 8) { v.resize(8); }
    std::string t;
    for (const Farbe& x : v) {
        if (!t.empty()) { t += ';'; }
        t += farbText(x);
    }
    g_app->settings.menueLetzteFarben = t;
}

bool lsTupfer(const char* id, const float* c, float g) {
    const bool k = ImGui::InvisibleButton(id, ImVec2{g, g});
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::RenderColorRectWithAlphaCheckerboard(dl, a, b, ImColor(c[0], c[1], c[2], c[3]), std::floor(g * 0.35F), ImVec2{0.0F, 0.0F}, 4.0F);
    dl->AddRect(a, b, ImGui::IsItemHovered() ? IM_COL32(0xE6, 0xE7, 0xEA, 255) : IM_COL32(0x6A, 0x6D, 0x78, 255), 4.0F);
    return k;
}

// Eine Farbe ist eine Zeile: Farbfeld, Wert, Deckkraft. R, G, B, A und die zuletzt benutzten Farben erst nach dem Klick auf das Farbfeld.
template <class Schreiben>
void farbZeile(const char* label, const char* key, const char* tip, const Farbe& f, const std::string& offenId, Schreiben schreiben) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    if (!feldKopf(label, key, tip)) { return; }
    static const char* const kRgba[4] = {"R", "G", "B", "A"};
    float c[4] = {f.r, f.g, f.b, f.a};
    ImGui::PushID(offenId.c_str());
    const std::string kk = "farbe" + offenId;
    const ImGuiID hid = ImGui::GetID(kk.c_str());
    if (z.halt.id == hid && z.halt.zahlen.size() == 4) { std::copy_n(z.halt.zahlen.begin(), 4, c); }
    const float fh = ImGui::GetFrameHeight();
    if (lsTupfer("##tupfer", c, fh)) {
        l.farbeOffen = l.farbeOffen == offenId ? std::string() : offenId;
    }
    lsTipp(tip);
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    const auto kanal = [&c](int k) { return static_cast<int>(std::lround(std::clamp(c[k], 0.0F, 1.0F) * 255.0F)); };
    ImGui::TextColored(ImColor(kLsLeise), "#%02X%02X%02X \xC2\xB7 %d %%", kanal(0), kanal(1), kanal(2), static_cast<int>(std::lround(c[3] * 100.0F)));
    if (l.farbeOffen == offenId) {
        zahlenFeld(kk.c_str(), c, 4, [&c, &kk, &z] {
            ImGui::BeginGroup();
            bool neu = ImGui::ColorEdit3("##bild", c, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            const char* deck = tr(Str::MlDeckkraft);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(deck);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(std::max(ImGui::GetFontSize() * 3.0F, ImGui::GetContentRegionAvail().x));
            neu |= ImGui::SliderFloat("##deck", &c[3], 0.0F, 1.0F, "%.2f");
            neu |= zahlenReihe(kk.c_str(), c, 4, kRgba, 0.01F, "%.2f");
            const std::vector<Farbe> letzte = letzteFarben();
            for (std::size_t k = 0; k < letzte.size(); ++k) {
                ImGui::PushID(static_cast<int>(k));
                const float g = ImGui::GetFrameHeight();
                if (k > 0) { daneben(g); }
                const float lf[4] = {letzte[k].r, letzte[k].g, letzte[k].b, letzte[k].a};
                if (lsTupfer("##letzte", lf, g)) {
                    c[0] = letzte[k].r;
                    c[1] = letzte[k].g;
                    c[2] = letzte[k].b;
                    c[3] = letzte[k].a;
                    neu = true;
                }
                lsTipp(tr(Str::MlLetzteFarbeTip));
                ImGui::PopID();
            }
            ImGui::EndGroup();
            (void)z;
            return neu;
        }, [schreiben](const std::vector<float>& w) {
            const Farbe neuF{w[0], w[1], w[2], w[3]};
            farbeMerken(neuF);
            schreiben(neuF);
        });
    }
    ImGui::PopID();
}

// --- Die Karte oben --------------------------------------------------------------------------------------------

struct ArtInfo {
    Str name;
    Ls sym;
};
ArtInfo artVon(const Dokument& d, const Item& it) {
    if (istBildRahmen(it)) {
        return istVideoRahmen(d, it) ? ArtInfo{Str::MlTypVideo, Ls::Video} : ArtInfo{Str::MlTypBild, Ls::Bild};
    }
    switch (it.type) {
        case ITEM_TYPE_BUTTON: return {Str::MlTypButton, Ls::Knopf};
        case ITEM_TYPE_RADIOBUTTON: return {Str::MlTypRadio, Ls::Knopf};
        case ITEM_TYPE_CHECKBOX: return {Str::MlTypHaken, Ls::Knopf};
        case ITEM_TYPE_EDITFIELD: return {Str::MlTypEingabe, Ls::Text};
        case ITEM_TYPE_COMBO: return {Str::MlTypCombo, Ls::Links};
        case ITEM_TYPE_LISTBOX: return {Str::MlTypListe, Ls::Links};
        case ITEM_TYPE_MODEL: return {Str::MlTypModell, Ls::Modell};
        case ITEM_TYPE_OWNERDRAW: return {Str::MlTypOwnerdraw, Ls::Bild};
        case ITEM_TYPE_NUMERICFIELD: return {Str::MlTypZahl, Ls::Text};
        case ITEM_TYPE_SLIDER: return {Str::MlTypSchieber, Ls::Regler};
        case ITEM_TYPE_YESNO: return {Str::MlTypJaNein, Ls::Knopf};
        case ITEM_TYPE_MULTI: return {Str::MlTypMulti, Ls::Text};
        case ITEM_TYPE_BIND: return {Str::MlTypTaste, Ls::Knopf};
        case ITEM_TYPE_TEXTSCROLL: return {Str::MlTypScroll, Ls::Links};
        default: return {Str::MlTypText, Ls::Text};
    }
}

const char* artName(const Dokument& d, const Item& it) { return tr(artVon(d, it).name); }

void lsBadge(const char* text, const char* tip) {
    const ImVec2 ts = ImGui::CalcTextSize(text);
    const float fs = ImGui::GetFontSize();
    // Passt der Text nicht, bleibt die Marke im Feld und schneidet ihn ab; der Hinweis nennt ihn ganz
    const ImVec2 g{std::max(fs, std::min(ts.x + fs * 0.9F, ImGui::GetContentRegionAvail().x)), ImGui::GetFrameHeight()};
    ImGui::InvisibleButton("##badge", g);
    const ImVec2 a = ImGui::GetItemRectMin();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = ts.y + fs * 0.15F;
    const float oben = a.y + (g.y - h) * 0.5F;
    dl->AddRectFilled(ImVec2{a.x, oben}, ImVec2{a.x + g.x, oben + h}, kLsFeld, h * 0.5F);
    dl->PushClipRect(a, ImVec2{a.x + g.x - fs * 0.2F, a.y + g.y}, true);
    dl->AddText(ImVec2{a.x + fs * 0.45F, oben + fs * 0.075F}, kLsLeise, text);
    dl->PopClipRect();
    lsTipp(tip);
}

void lsKartenSymbol(Ls s) {
    const float g = std::floor(ImGui::GetFontSize() * 2.4F);
    ImGui::Dummy(ImVec2{g, g});
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, b, kLsBlau, 6.0F);
    lsSymbol(dl, s, ImVec2{(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F}, std::floor(g * 0.6F), IM_COL32(0xE6, 0xE7, 0xEA, 255));
}

// Neuer Name: die Schritte, die auf das Element zeigen, ziehen mit, und die Layouts ebenso; ein Undo-Schritt
void nameSetzen(Dokument& d, int menu, int i, const std::string& utf8) {
    const std::string neu = utf8ZuLatin1(utf8.c_str());
    if (neu.empty()) { return; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(menu)];
    const std::string alt = m.items[static_cast<std::size_t>(i)].window.name;
    if (neu == alt) { return; }
    std::vector<Ersetzung> e;
    if (!itemUmbenennen(d.datei, menu, i, inAnf(neu), e)) {
        ausMakroMelden();
        return;
    }
    for (Layout l : liesLayouts(d.datei, menu)) {
        bool anders = false;
        if (l.rahmenItem == alt) {
            l.rahmenItem = neu;
            anders = true;
        }
        for (LayoutKind& k : l.kinder) {
            if (!k.spacer && !k.layout && k.name == alt) {
                k.name = neu;
                anders = true;
            }
        }
        if (anders) { (void)layoutSchreiben(d.datei, menu, l, false, e); }
    }
    aendere(std::move(e));
}

void zeichneKarte(Dokument& d, int art) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    const int mi = menuDerSeite(d);
    if (mi < 0) { return; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    const float fs = ImGui::GetFontSize();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, kLsKarte);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{fs * 0.6F, fs * 0.5F});
    ImGui::BeginChild("##karte", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    if (d.auswahl.size() > 1) {
        lsKartenSymbol(Ls::Gruppe);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextWrapped(trN(Str::MlAuswahlAnzahl, d.auswahl.size()), static_cast<int>(d.auswahl.size()));
    } else if (d.auswahl.empty()) {
        lsKartenSymbol(Ls::Menue);
        ImGui::SameLine();
        ImGui::BeginGroup();
        const std::string n = anzeige(m.window.name);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(n.c_str());
        daneben(ImGui::CalcTextSize(tr(Str::MlTypMenue)).x + fs);
        lsBadge(tr(Str::MlTypMenue), tr(Str::MlNichtsGewaehlt));
        ImGui::EndGroup();
        if (art == 0) {
            ImGui::PushTextWrapPos(0.0F);
            ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlNichtsGewaehlt));
            ImGui::PopTextWrapPos();
        }
    } else if (static_cast<std::size_t>(d.auswahl[0]) < m.items.size()) {
        const int i = d.auswahl[0];
        const Item& it = m.items[static_cast<std::size_t>(i)];
        const ArtInfo ai = artVon(d, it);
        ImGui::PushID(i);
        lsKartenSymbol(ai.sym);
        ImGui::SameLine();
        ImGui::BeginGroup();
        const std::string typ = g_app->settings.menueDateiSchluessel ? std::string(tr(ai.name)) + " \xC2\xB7 " + typName(it.type) : std::string(tr(ai.name));
        const float badge = ImGui::CalcTextSize(typ.c_str()).x + fs * 0.9F;
        const float rest = ImGui::GetContentRegionAvail().x;
        const bool nebeneinander = rest >= badge + fs * 7.0F;
        ImGui::SetNextItemWidth(nebeneinander ? rest - badge - ImGui::GetStyle().ItemSpacing.x : rest);
        std::string name = anzeige(it.window.name);
        Dokument* dp = &d;
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::IsItemHovered() ? kLsFeld : IM_COL32(0, 0, 0, 0));
        textFeld("##name", &name, [dp, mi, i](const std::string& t) { nameSetzen(*dp, mi, i, t); });
        ImGui::PopStyleColor();
        lsTipp(tr(Str::MlNameTip));
        if (nebeneinander) { ImGui::SameLine(); }
        // File keys in einem schmalen Feld: Art und Wort der Datei untereinander statt abgeschnitten
        if (g_app->settings.menueDateiSchluessel && badge > rest) {
            lsBadge(tr(ai.name), tr(Str::MeTypFestTip));
            ImGui::PushID(1);
            lsBadge(typName(it.type), tr(Str::MeTypFestTip));
            ImGui::PopID();
        } else {
            lsBadge(typ.c_str(), tr(Str::MeTypFestTip));
        }
        ImGui::EndGroup();
        // Alle Knoepfe fuer das Element in einer Reihe; das Auge schreibt visible, das Schloss steht nicht in der Datei
        const float g = std::floor(ImGui::GetFrameHeight() * 1.05F);
        const bool sichtbar = (it.window.flags & WINDOW_VISIBLE) != 0;
        if (lsKnopf("##k_sichtbar", Ls::Auge, tr(Str::MlSichtbarTip), sichtbar)) { eigenschaft(i, "visible", {sichtbar ? "0" : "1"}); }
        daneben(g);
        const bool zu = d.gesperrt.count(i) != 0;
        if (lsKnopf("##k_sperren", Ls::Zu, tr(Str::MlSperrenTip), zu)) {
            if (zu) { d.gesperrt.erase(i); } else { d.gesperrt.insert(i); }
        }
        daneben(g + fs * 0.5F);
        ImGui::Dummy(ImVec2{fs * 0.15F, g});
        ImGui::SameLine();
        if (lsKnopf("##k_vor", Ls::Vor, tr(Str::MeNachVorn), false, false, i + 1 < static_cast<int>(m.items.size()))) {
            std::vector<Ersetzung> e;
            if (itemVerschieben(d.datei, mi, i, true, e)) {
                aendere(std::move(e));
                d.auswahl.assign(1, i + 1);
            }
        }
        daneben(g);
        if (lsKnopf("##k_hin", Ls::Hin, tr(Str::MeNachHinten), false, false, i > 0)) {
            std::vector<Ersetzung> e;
            if (itemVerschieben(d.datei, mi, i, false, e)) {
                aendere(std::move(e));
                d.auswahl.assign(1, i - 1);
            }
        }
        daneben(g + fs * 0.5F);
        ImGui::Dummy(ImVec2{fs * 0.15F, g});
        ImGui::SameLine();
        if (lsKnopf("##k_dup", Ls::Dup, tr(Str::ActClone))) { dupliziereAuswahl(); }
        daneben(g);
        if (lsKnopf("##k_weg", Ls::Weg, tr(Str::ActDelete), false, true)) { loescheAuswahl(); }
        ImGui::PopID();
    }
    lsZeilenEnde();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    (void)l;
}

// --- Properties ------------------------------------------------------------------------------------------------

TextMass textMassVon(Dokument& d, const Item& it) { return textMasse(it, umgebung(d)); }

// Beim Skalieren bleibt die Ausrichtung, auch senkrecht. Ob ein Text folgt, steht in seinen Zahlen (textXFolgt, textSenkrecht).
void textFolgtRect(Dokument& d, int mi, int i, const Rechteck& neu, std::vector<Ersetzung>& e) {
    const Item& it = d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(i)];
    if (it.text.empty() || !feldGilt("textalignx", it.type, it.window.style, d.datei.spiel)) { return; }
    const Rechteck& alt = it.window.rectClient;
    if (neu.w != alt.w && textXFolgt(it.textalignment, it.textalignx, alt.w)) {
        (void)setzeEigenschaft(d.datei, mi, i, "textalignx", {zahlText(textXFuer(it.textalignment, neu.w))}, e);
    }
    if (neu.h != alt.h) {
        const TextMass t = textMassVon(d, it);
        const int s = textSenkrecht(it.textaligny, alt.h, t.oben, t.unten);
        if (s > 0) { (void)setzeEigenschaft(d.datei, mi, i, "textaligny", {zahlText(textYFuer(s, neu.h, t.oben, t.unten))}, e); }
    }
}

// Eine Zahl mit Schieber: Ziehen am Schieber oder an der Beschriftung, Klick ins Feld tippt
bool lsZugWert(float* v, float tempo) {
    const Zustand::Leiste& l = Z().leiste;
    if (l.ziehAktiv && l.ziehDelta != 0.0F) {
        *v += l.ziehDelta * tempo;
        return true;
    }
    return false;
}

bool lsZahlEin(const char* id, float* v, float tempo, const char* format) {
    bool neu = lsZugWert(v, tempo);
    neu |= ImGui::InputScalar(id, ImGuiDataType_Float, v, nullptr, nullptr, format, ImGuiInputTextFlags_AutoSelectAll);
    return neu;
}

bool lsZahlMitSchieber(const char* id, const char* idSchieber, float* v, float lo, float hi, float tempo, const char* format) {
    ImGui::BeginGroup();
    const float fs = ImGui::GetFontSize();
    const float nr = fs * 4.0F;
    const float rest = ImGui::GetContentRegionAvail().x - nr - ImGui::GetStyle().ItemSpacing.x;
    bool neu = lsZugWert(v, tempo);
    if (rest >= fs * 3.0F) {
        ImGui::SetNextItemWidth(rest);
        neu |= ImGui::SliderFloat(idSchieber, v, lo, hi, "");
        ImGui::SameLine();
    }
    ImGui::SetNextItemWidth(rest >= fs * 3.0F ? nr : -FLT_MIN);
    neu |= ImGui::InputScalar(id, ImGuiDataType_Float, v, nullptr, nullptr, format, ImGuiInputTextFlags_AutoSelectAll);
    ImGui::EndGroup();
    return neu;
}

float lsKnopfRest(const char* knopf) { return std::max(ImGui::GetFontSize() * 3.0F, ImGui::GetContentRegionAvail().x - knopfBreite(knopf) - ImGui::GetStyle().ItemSpacing.x); }

// Der Bildpunkt-Text einer Zahl wie in der Zusammenfassung ("100, 100 · 200 x 40")
std::string zahlenKurz(std::initializer_list<float> v, const char* zwischen) {
    std::string s;
    for (const float x : v) {
        if (!s.empty()) { s += zwischen; }
        s += zahlText(x);
    }
    return s;
}

// Der gesuchte oder einzige Wert von showCvar { "wert" }; viele = mehr als ein Wert (bleibt unter "weitere Eintraege")
// rc608 Teil 3b: alle vier Bedingungen des Spiels (ItemParse_showCvar, hideCvar, enableCvar, disableCvar)
const char* const kBedingung[4] = {"showCvar", "hideCvar", "enableCvar", "disableCvar"};
int bedingungArt(const Item& it) {
    for (int k = 0; k < 4; ++k) {
        if (finde(it, kBedingung[k]) != nullptr) { return k; }
    }
    return 0;
}

std::string showCvarWert(const Item& it, bool& viele) {
    viele = false;
    const Eigenschaft* e = finde(it, kBedingung[bedingungArt(it)]);
    if (e == nullptr) { return {}; }
    std::vector<std::string> w;
    for (const Token& t : e->args) {
        if (t.s != "{" && t.s != "}" && t.s != ";") { w.push_back(t.s); }
    }
    viele = w.size() > 1;
    return w.empty() ? std::string() : w[0];
}

void eigenschaftenElement(Dokument& d, int mi, int i, const Item& it) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    Dokument* dp = &d;
    const float fs = ImGui::GetFontSize();
    const auto gilt = [&](const char* k) { return feldGilt(k, it.type, it.window.style, d.datei.spiel); };
    const Rechteck rc = it.window.rectClient;
    z.felderGezeigt.clear();

    // Verbindung zu Events: was es schon gibt, ein Klick springt hin
    {
        std::string zeile;
        for (const Ereignis& e : ereignisseFuer(d.datei.dialekt, z.spiel)) {
            const std::size_t n = zerlege(skriptVon(it, e.art), d.datei.dialekt, z.spiel).size();
            if (n == 0) { continue; }
            if (!zeile.empty()) { zeile += " \xC2\xB7 "; }
            zeile += std::string(tr(kEreignisNamen[e.art])) + " " + std::to_string(n);
        }
        char b[300];
        if (zeile.empty()) { std::snprintf(b, sizeof(b), "%s", tr(Str::MlEvKeine)); }
        else { std::snprintf(b, sizeof(b), tr(Str::MlEvZeile), zeile.c_str()); }
        const float kb = knopfBreite(tr(Str::MlEvBearbeiten));
        const float tw = ImGui::CalcTextSize(b).x;
        const float breit = ImGui::GetContentRegionAvail().x;
        ImGui::AlignTextToFramePadding();
        ImGui::PushTextWrapPos(tw + kb + fs <= breit ? 0.0F : breit);
        ImGui::TextColored(ImColor(kLsLeise), "%s", b);
        ImGui::PopTextWrapPos();
        if (tw + kb + fs <= breit) { ImGui::SameLine(ImGui::GetCursorPosX() + breit - kb); }
        if (ImGui::Button(tr(Str::MlEvBearbeiten))) { z.reiterGewollt = 3; }
    }
    // Feldsuche und "File keys"
    {
        const char* schalter = tr(Str::MlDateiSchluessel);
        const float sb = hakenBreite(schalter);
        const float breit = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(breit >= sb + fs * 7.0F ? breit - sb - ImGui::GetStyle().ItemSpacing.x : breit);
        if (eingabe("##feldsuche", &l.suchRoh, 0, tr(Str::MlFeldSuche))) { l.such = kleinPfad(l.suchRoh); }
        lsTipp(tr(Str::MlFeldSucheTip));
        if (breit >= sb + fs * 7.0F) { ImGui::SameLine(); }
        ImGui::Checkbox(schalter, &g_app->settings.menueDateiSchluessel);
        lsTipp(tr(Str::MlDateiSchluesselTip));
    }
    ImGui::Dummy(ImVec2{1.0F, fs * 0.15F});
    l.trefferGesamt = 0;
    l.markeItem = i;

    const bool bild = istBildRahmen(it);
    // Inhalt
    if (!bild && (gilt("text") || gilt("textscale"))) {
        std::string zus;
        if (gilt("text")) {
            std::string kurz = anzeige(it.text);
            if (kurz.size() > 24) { kurz = kurz.substr(0, 24) + "..."; }
            zus = "\"" + kurz + "\" \xC2\xB7 ";
        }
        zus += std::to_string(static_cast<int>(std::lround(it.textscale * 100.0F))) + " %";
        gruppe("inhalt", tr(Str::MlGrInhalt), zus, [&] {
            if (gilt("text") && feldKopf(tr(Str::MlFText), "text", tr(Str::MlFTextTip))) {
                std::string text = textAusDatei(z.sprache, it.text);   // rc608 Teil 3b: Die Datei hat die Kodierung der Sprache, das Feld UTF-8
                const Dialekt dialekt = d.datei.dialekt;
                textFeld("##text", &text, [i, dialekt](const std::string& t) { eigenschaft(i, "text", {schreibZeichenkette(textFuerDatei(Z().sprache, t), dialekt)}); });
                if (!it.text.empty() && it.text[0] == '@') {
                    // fehlt der Schluessel, zeigt das Spiel nichts (SE_GetString liefert ""): so sagen statt den Schluessel zu wiederholen
                    ImGui::TextDisabled("= %s", texte().kennt(it.text) ? anzeige(texte().aufloesen(it.text)).c_str() : tr(Str::MeTextFehltImSpiel));
                }
            }
            if (gilt("textscale") && feldKopf(tr(Str::MlFGroesse), "textscale", tr(Str::MlFGroesseTip), true)) {
                float ts = it.textscale;
                zahlenFeld("textscale", &ts, 1, [&ts] { return lsZahlMitSchieber("##textscale", "##textscale_s", &ts, 0.2F, 2.0F, 0.01F, "%g"); },
                           [i](const std::vector<float>& w) { eigenschaft(i, "textscale", {zahlText(w[0])}); });
            }
            if (gilt("forecolor")) {
                farbZeile(tr(Str::MlFTextfarbe), "forecolor", tr(Str::MlFTextfarbeTip), it.window.foreColor, "forecolor",
                          [i](const Farbe& f) { eigenschaft(i, "forecolor", {farbText(f)}); });
            }
            if (gilt("textalign") && feldKopf(tr(Str::MlFAusrichtung), "textalign, textalignx, textaligny", tr(Str::MlFAusrichtungTip))) {
                static const char* const ausr[] = {"ITEM_ALIGN_LEFT", "ITEM_ALIGN_CENTER", "ITEM_ALIGN_RIGHT"};
                const int a = std::clamp(it.textalignment, 0, 2);
                const int k = lsUmschalter("##waagrecht", {{"##a_links", tr(Str::MeLinks), true, Ls::Links}, {"##a_mitte", tr(Str::MlAusrMitte), true, Ls::Mitte}, {"##a_rechts", tr(Str::MeRechts), true, Ls::Rechts}}, a);
                if (k >= 0) {
                    std::vector<Ersetzung> e;
                    (void)setzeEigenschaft(d.datei, mi, i, "textalign", {konstante(d.datei, ausr[k], k)}, e);
                    (void)setzeEigenschaft(d.datei, mi, i, "textalignx", {zahlText(textXFuer(k, rc.w))}, e);
                    aendere(std::move(e));
                }
                const TextMass t = textMassVon(d, it);
                daneben(ImGui::GetFrameHeight() * 3.2F + fs);
                ImGui::Dummy(ImVec2{fs * 0.3F, 1.0F});
                ImGui::SameLine();
                const int v = lsUmschalter("##senkrecht", {{"##v_oben", tr(Str::MeOben), true, Ls::Oben}, {"##v_mitte", tr(Str::MlAusrMitteV), true, Ls::MitteV}, {"##v_unten", tr(Str::MeUnten), true, Ls::Unten}}, textSenkrecht(it.textaligny, rc.h, t.oben, t.unten));
                if (v >= 0) { eigenschaft(i, "textaligny", {zahlText(textYFuer(v, rc.h, t.oben, t.unten))}); }
            }
            if (gilt("textalignx") && feldKopf(tr(Str::MlFTextLage), "textalignx, textaligny", tr(Str::MlFTextLageTip))) {
                static const char* const kAlign[2] = {"X", "Y"};
                float ta[2] = {it.textalignx, it.textaligny};
                zahlenFeld("textalignx/y", ta, 2, [&ta] { return zahlenReihe("textalignx/y", ta, 2, kAlign, 1.0F, "%g"); }, [dp, mi, i](const std::vector<float>& w) {
                    std::vector<Ersetzung> e;
                    (void)setzeEigenschaft(dp->datei, mi, i, "textalignx", {zahlText(w[0])}, e);
                    (void)setzeEigenschaft(dp->datei, mi, i, "textaligny", {zahlText(w[1])}, e);
                    aendere(std::move(e));
                });
            }
            if (gilt("textstyle") && feldKopf(tr(Str::MlFStil), "textstyle", tr(Str::MlFStilTip))) {
                // Reihenfolge wie im Entwurf; die Zahl im Spiel steht dabei (menudef.h)
                struct Stil { const char* name; int wert; Str text; };
                static const Stil stile[] = {{"ITEM_TEXTSTYLE_NORMAL", 0, Str::MlStilNormal},     {"ITEM_TEXTSTYLE_SHADOWED", 3, Str::MlStilSchatten},
                                             {"ITEM_TEXTSTYLE_SHADOWEDMORE", 6, Str::MlStilStark}, {"ITEM_TEXTSTYLE_OUTLINED", 4, Str::MlStilKontur},
                                             {"ITEM_TEXTSTYLE_OUTLINESHADOWED", 5, Str::MlStilKonturSchatten}, {"ITEM_TEXTSTYLE_PULSE", 2, Str::MlStilPuls},
                                             {"ITEM_TEXTSTYLE_BLINK", 1, Str::MlStilBlink}};
                const char* jetzt = "?";
                for (const Stil& s : stile) {
                    if (s.wert == it.textStyle) { jetzt = tr(s.text); }
                }
                if (ImGui::BeginCombo("##textstyle", jetzt)) {
                    for (const Stil& s : stile) {
                        if (ImGui::Selectable(tr(s.text), s.wert == it.textStyle)) { eigenschaft(i, "textstyle", {konstante(d.datei, s.name, s.wert)}); }
                    }
                    ImGui::EndCombo();
                }
            }
            if (gilt("font") && feldKopf(tr(Str::MlFSchrift), "font", tr(Str::MlFSchriftTip))) {
                ImGui::SetNextItemWidth(std::min(fs * 5.0F, ImGui::GetContentRegionAvail().x));
                const std::string jetzt = std::to_string(it.font);
                if (ImGui::BeginCombo("##font", jetzt.c_str())) {
                    for (int n = 1; n <= 4; ++n) {
                        if (ImGui::Selectable(std::to_string(n).c_str(), n == it.font)) { eigenschaft(i, "font", {std::to_string(n)}); }
                    }
                    ImGui::EndCombo();
                }
            }
        });
    }
    // Modell
    if (it.type == ITEM_TYPE_MODEL) {
        char zus[200];
        std::snprintf(zus, sizeof(zus), tr(Str::MlModellZus), it.g2anim.empty() ? "-" : it.g2anim.c_str(), it.angle);
        gruppe("modell", tr(Str::MeModellGruppe), zus, [&] { zeichneModellFelder(d, mi, i, it); });
    }
    // Bild und Video: ein Rahmen, was darin steht, kommt aus background
    if (bild) {
        const bool video = istVideoRahmen(d, it);
        gruppe("bildw", tr(video ? Str::MlTypVideo : Str::MlTypBild), it.window.background.empty() ? std::string(tr(Str::MlNichtsGewaehltKurz)) : anzeige(it.window.background), [&] {
            if (feldKopf(tr(video ? Str::MlTypVideo : Str::MlTypBild), "background", tr(video ? Str::MlFVideoTip : Str::MlFBildTip))) {
                std::string bg = it.window.background;
                ImGui::SetNextItemWidth(lsKnopfRest(tr(Str::MlWaehlen)));
                textFeld("##background", &bg, [i](const std::string& t) { eigenschaft(i, "background", {inAnf(t)}); }, tr(Str::MlFBildLeer));
                ImGui::SameLine();
                if (ImGui::Button(tr(Str::MlWaehlen))) { auswahlOeffnen(video ? 1 : 0, i); }
            }
            if (video && !l.zaehlen) {
                ImGui::PushTextWrapPos(0.0F);
                ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlVideoStumm));
                ImGui::PopTextWrapPos();
            }
            if (!l.zaehlen) {
                lsZeilenEnde();
                // Ein Rahmen ueber die ganze Flaeche, ganz hinten: so bleibt ein Vollbild-Video leicht zu finden
                if (ImGui::Button(tr(Str::MlVollbildHinten))) {
                    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
                    const std::string vorher = d.puffer.text();
                    std::vector<Ersetzung> e;
                    if (setzeRect(d.datei, mi, i, Rechteck{-m.window.rect.x, -m.window.rect.y, 640.0F, 480.0F}, e)) {
                        aendere(std::move(e));
                        (void)setzeAnStelle(d, mi, i, 0);
                        d.auswahl.assign(1, 0);
                        (void)zuEinemSchritt(vorher);
                    } else {
                        ausMakroMelden();
                    }
                }
                lsTipp(tr(Str::MlVollbildHintenTip));
            }
            if (feldGilt("forecolor", it.type, it.window.style, d.datei.spiel)) {
                farbZeile(tr(Str::MlFTint), "forecolor", tr(Str::MlFTintTip), it.window.foreColor, "forecolor",
                          [i](const Farbe& f) { eigenschaft(i, "forecolor", {farbText(f)}); });
            }
        });
    }
    // Schieber: was er aendert, steht in einer Zeile der Datei (ItemParse_cvarFloat)
    if (it.type == ITEM_TYPE_SLIDER) {
        const auto schreibeSchieber = [dp, mi, i, it](const std::string& cvar, float vor, float lo, float hi) {
            std::vector<Ersetzung> e;
            const char* k = finde(it, "cvarFloat") != nullptr || finde(it, "cvar") == nullptr ? "cvarFloat" : "cvar";
            if (std::strcmp(k, "cvarFloat") == 0) {
                if (setzeEigenschaft(dp->datei, mi, i, k, {inAnf(cvar), zahlText(vor), zahlText(lo), zahlText(hi)}, e)) { aendere(std::move(e)); }
                else { ausMakroMelden(); }
            } else {
                eigenschaft(i, k, {inAnf(cvar)});
            }
        };
        gruppe("wert", tr(Str::MlGrWert), anzeige(it.cvar), [&] {
            if (feldKopf(tr(Str::MlFEinstellung), "cvar, cvarFloat", tr(Str::MlFEinstellungSchieberTip))) {
                std::string cvar = it.cvar;
                const float vor = it.defVal, lo = it.minVal, hi = it.maxVal;
                textFeld("##cvar", &cvar, [schreibeSchieber, vor, lo, hi](const std::string& t) { schreibeSchieber(t, vor, lo, hi); });
            }
            if (feldKopf(tr(Str::MlFStartwert), "cvarFloat", tr(Str::MlFStartwertTip), true)) {
                float v = it.defVal;
                const std::string cvar = it.cvar;
                const float lo = it.minVal, hi = it.maxVal;
                zahlenFeld("schieber_vor", &v, 1, [&v] { return lsZahlEin("##schieber_vor", &v, 0.01F, "%g"); },
                           [schreibeSchieber, cvar, lo, hi](const std::vector<float>& w) { schreibeSchieber(cvar, w[0], lo, hi); });
            }
            if (feldKopf(tr(Str::MlFVonBis), "cvarFloat", tr(Str::MlFVonBisTip))) {
                static const char* const kVb[2] = {"-", "+"};
                float vb[2] = {it.minVal, it.maxVal};
                const std::string cvar = it.cvar;
                const float vor = it.defVal;
                zahlenFeld("schieber_bereich", vb, 2, [&vb] { return zahlenReihe("schieber_bereich", vb, 2, kVb, 0.01F, "%g"); },
                           [schreibeSchieber, cvar, vor](const std::vector<float>& w) { schreibeSchieber(cvar, vor, w[0], w[1]); });
            }
        });
    }
    // rc608 Teil 3b: Die Eintraege einer Mehrfachwahl: Das Element taugt sonst nur im Text-Editor. Geschrieben wird die ganze Liste, ein Schritt.
    if (it.type == ITEM_TYPE_MULTI) {
        const Eigenschaft* ls = finde(it, "cvarStrList");
        const bool zahlen = ls == nullptr && finde(it, "cvarFloatList") != nullptr;
        if (zahlen) { ls = finde(it, "cvarFloatList"); }
        std::vector<std::string> w;
        if (ls != nullptr) {
            for (const Token& tk : ls->args) {
                if (tk.s != "{" && tk.s != "}" && tk.s != ";") { w.push_back(tk.s); }
            }
        }
        if (w.size() % 2 != 0) { w.emplace_back(); }
        const char* key = zahlen ? "cvarFloatList" : "cvarStrList";
        const auto schreibeListe = [i, key, zahlen](const std::vector<std::string>& neu) {
            std::string s = "{";
            for (std::size_t k = 0; k < neu.size(); ++k) {
                const bool zahl = zahlen && k % 2 == 1;
                s += " " + (zahl ? (neu[k].empty() ? std::string("0") : neu[k]) : "\"" + neu[k] + "\"");
            }
            eigenschaft(i, key, {s + " }"});
        };
        char zus[64];
        std::snprintf(zus, sizeof(zus), "%d", static_cast<int>(w.size() / 2));
        gruppe("auswahlen", tr(Str::MlFWerteListe), zus, [&] {
            if (l.zaehlen) { return; }
            ImGui::PushTextWrapPos(0.0F);
            ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlFWerteListeTip));
            ImGui::PopTextWrapPos();
            const float x = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x;
            const float breit = (ImGui::GetContentRegionAvail().x - x - ImGui::GetStyle().ItemSpacing.x) * 0.5F;
            for (std::size_t k = 0; k + 1 < w.size(); k += 2) {
                ImGui::PushID(static_cast<int>(k));
                ImGui::SetNextItemWidth(breit);
                std::string a = anzeige(w[k]);
                textFeld("##wl", &a, [schreibeListe, w, k](const std::string& t) {
                    std::vector<std::string> neu = w;
                    neu[k] = utf8ZuLatin1(t.c_str());
                    neu[k].erase(std::remove(neu[k].begin(), neu[k].end(), '"'), neu[k].end());
                    schreibeListe(neu);
                }, tr(Str::MlWerteText));
                ImGui::SameLine();
                ImGui::SetNextItemWidth(breit);
                std::string b = anzeige(w[k + 1]);
                textFeld("##wv", &b, [schreibeListe, w, k](const std::string& t) {
                    std::vector<std::string> neu = w;
                    neu[k + 1] = utf8ZuLatin1(t.c_str());
                    neu[k + 1].erase(std::remove(neu[k + 1].begin(), neu[k + 1].end(), '"'), neu[k + 1].end());
                    schreibeListe(neu);
                }, tr(Str::MlWerteWert));
                ImGui::SameLine();
                if (lsKnopf("##wx", Ls::Weg, tr(Str::ActDelete), false, true)) {
                    std::vector<std::string> neu = w;
                    neu.erase(neu.begin() + static_cast<std::ptrdiff_t>(k), neu.begin() + static_cast<std::ptrdiff_t>(k) + 2);
                    schreibeListe(neu);
                }
                ImGui::PopID();
            }
            if (ImGui::Button(tr(Str::MlWerteNeu))) {
                std::vector<std::string> neu = w;
                neu.emplace_back(tr(Str::MlWerteNeuText));
                neu.emplace_back(std::to_string(w.size() / 2));
                schreibeListe(neu);
            }
        });
    }
    // rc608 Teil 3b: Was der Code des Spiels fuellt: frei tippbar, das Spiel muss es lesen koennen. Der Tipp sagt in einem Satz, was es ist.
    if (gilt("ownerdraw") || gilt("feeder")) {
        const auto wortZeile = [&](const char* key, Str name, Str tip) {
            if (!gilt(key) || !feldKopf(tr(name), key, tr(tip))) { return; }
            const Eigenschaft* e0 = finde(it, key);
            std::string wort;
            if (e0 != nullptr) {
                for (const Token& tk : e0->args) { wort += (wort.empty() ? "" : " ") + anzeige(tk.s); }
            }
            const std::string id = std::string("##wort_") + key;
            textFeld(id.c_str(), &wort, [i, key](const std::string& t) {
                const std::string b = utf8ZuLatin1(t.c_str());
                // Im Einzelspieler liest PC_ParseInt mit atoi: Aus einem Wort wuerde still eine 0. Im Mehrspieler entscheidet die Probe (PC_Int_Parse, menudef.h).
                const Dokument* dd = doc();
                const bool zahl = std::all_of(b.begin(), b.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0 || c == ' ' || c == '-'; });
                if (dd != nullptr && dd->datei.dialekt == Dialekt::SP && !zahl) {
                    std::vector<char> m(400);
                    std::snprintf(m.data(), m.size(), tr(Str::MlUngueltig), b.c_str());
                    Z().meldung = m.data();
                    addStatus(m.data());
                    return;
                }
                if (!b.empty()) {
                    std::vector<std::string> w;
                    std::size_t von = 0;
                    while (von < b.size()) {
                        const std::size_t bis = std::min(b.find(' ', von), b.size());
                        if (bis > von) { w.push_back(b.substr(von, bis - von)); }
                        von = bis + 1;
                    }
                    eigenschaft(i, key, w);
                }
            }, tr(Str::MlKeine));
        };
        gruppe("spielinhalt", tr(Str::MlGrSpielInhalt), {}, [&] {
            wortZeile("ownerdraw", Str::MlFOwnerdraw, Str::MlFOwnerdrawTip);
            wortZeile("feeder", Str::MlFFeeder, Str::MlFFeederTip);
            wortZeile("elementwidth", Str::MlFZeileBreite, Str::MlFZeileBreiteTip);
            wortZeile("elementheight", Str::MlFZeileHoehe, Str::MlFZeileHoeheTip);
            wortZeile("columns", Str::MlFSpalten, Str::MlFSpaltenTip);
        });
    }
    // Lage und Groesse: immer 2 und 2
    gruppe("lage", tr(Str::MlGrLage), zahlenKurz({rc.x, rc.y}, ", ") + " \xC2\xB7 " + zahlenKurz({rc.w, rc.h}, " x "), [&] {
        if (feldKopf(tr(Str::MlFLage), "rect", tr(Str::MlFLageTip))) {
            const char* const teile[4] = {"X", "Y", tr(Str::MlFBreite), tr(Str::MlFHoehe)};
            float r[4] = {rc.x, rc.y, rc.w, rc.h};
            zahlenFeld("rect", r, 4, [&r, &teile] { return zahlenReihe("rect", r, 4, teile, 1.0F, "%g"); },
                       [i](const std::vector<float>& w) { setzeRects({{i, Rechteck{w[0], w[1], w[2], w[3]}}}); });
        }
    });
    // Aussehen
    {
        const int sty = std::clamp(it.window.style, 0, 5);
        const auto fuellName = [sty]() -> const char* {
            switch (sty) {
                case WINDOW_STYLE_EMPTY: return tr(Str::MlZusKeineFuellung);
                case WINDOW_STYLE_FILLED: return tr(Str::MlZusFarbfuellung);
                case WINDOW_STYLE_SHADER: return tr(Str::MlZusBildfuellung);
                default: return styleName(sty);
            }
        };
        gruppe("aussehen", tr(Str::MlGrAussehen), std::string(fuellName()) + " \xC2\xB7 " + tr(it.window.border == 0 ? Str::MlZusOhneRand : Str::MlZusMitRand), [&] {
            if (!bild && feldKopf(tr(Str::MlFFuellung), "style", tr(Str::MlFFuellungTip))) {
                // Der Entwurf zeigt drei Stellungen. Das Spiel kennt sechs: die uebrigen bleiben erreichbar und werden nicht still ersetzt.
                const std::vector<LsWahl> wahl = {{tr(Str::MlFuellKeine), nullptr}, {tr(Str::MlFuellFarbe), nullptr}, {tr(Str::MlFuellBild), nullptr}};
                const int g = sty == WINDOW_STYLE_EMPTY ? 0 : sty == WINDOW_STYLE_FILLED ? 1 : sty == WINDOW_STYLE_SHADER ? 2 : -1;
                const int k = lsUmschalter("##fuellung", wahl, g);
                static const int kWerte[3] = {WINDOW_STYLE_EMPTY, WINDOW_STYLE_FILLED, WINDOW_STYLE_SHADER};
                if (k >= 0) { eigenschaft(i, "style", {konstante(d.datei, styleName(kWerte[k]), kWerte[k])}); }
                const char* mehr = g < 0 ? styleName(sty) : "...";
                daneben(knopfBreite(mehr) + fs);
                ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize(mehr).x + ImGui::GetFrameHeight() * 1.5F));
                if (ImGui::BeginCombo("##style", mehr, ImGuiComboFlags_HeightSmall)) {
                    for (int t = 0; t < 6; ++t) {
                        if (ImGui::Selectable(styleName(t), t == sty)) { eigenschaft(i, "style", {konstante(d.datei, styleName(t), t)}); }
                    }
                    ImGui::EndCombo();
                }
            }
            // Window_Paint: backColor bei FILLED und GRADIENT; background bei SHADER und, wenn gesetzt, auch bei FILLED
            if (sty == WINDOW_STYLE_FILLED || sty == WINDOW_STYLE_GRADIENT) {
                farbZeile(tr(Str::MlFFuellFarbe), "backcolor", tr(Str::MlFFuellFarbeTip), it.window.backColor, "backcolor",
                          [i](const Farbe& f) { eigenschaft(i, "backcolor", {farbText(f)}); });
            }
            if (!bild && (sty == WINDOW_STYLE_SHADER || (sty == WINDOW_STYLE_FILLED && !it.window.background.empty())) &&
                feldKopf(tr(Str::MlFuellBild), "background", tr(Str::MlFBildTip))) {
                std::string bg = it.window.background;
                ImGui::SetNextItemWidth(lsKnopfRest(tr(Str::MlWaehlen)));
                textFeld("##background", &bg, [i](const std::string& t) { eigenschaft(i, "background", {inAnf(t)}); }, tr(Str::MlFBildLeer));
                ImGui::SameLine();
                if (ImGui::Button(tr(Str::MlWaehlen))) { auswahlOeffnen(0, i); }
            }
            if (feldKopf(tr(Str::MlFRand), "border", tr(Str::MlFRandTip))) {
                // Border_Paint: 0 keiner, 1 ringsum, 2 oben und unten, 3 links und rechts, 4 Verlauf (KCGRADIENT)
                static const Str kRand[5] = {Str::MlRandKein, Str::MlRandRings, Str::MlRandObenUnten, Str::MlRandLinksRechts, Str::MlRandVerlauf};
                const int b = std::clamp(it.window.border, 0, 4);
                if (ImGui::BeginCombo("##border", tr(kRand[b]))) {
                    for (int t = 0; t < 5; ++t) {
                        if (ImGui::Selectable(tr(kRand[t]), t == b)) { eigenschaft(i, "border", {std::to_string(t)}); }
                    }
                    ImGui::EndCombo();
                }
            }
            if (it.window.border != 0) {
                farbZeile(tr(Str::MlFRandFarbe), "bordercolor", tr(Str::MlFRandFarbeTip), it.window.borderColor, "bordercolor",
                          [i](const Farbe& f) { eigenschaft(i, "bordercolor", {farbText(f)}); });
            }
        });
    }
    // Sichtbarkeit und Spiel-Logik
    {
        bool viele = false;
        const std::string wert = showCvarWert(it, viele);
        gruppe("logik", tr(Str::MlGrLogik), it.cvarTest.empty() ? std::string(tr(Str::MlZusImmerSichtbar)) : anzeige(it.cvarTest + " = " + wert), [&] {
            if (feldKopf(tr(Str::MlFGruppe), "group", tr(Str::MlFGruppeTip))) {
                std::string g = anzeige(it.window.group);
                textFeld("##group", &g, [i](const std::string& t) { eigenschaft(i, "group", {inAnf(t)}); }, tr(Str::MlKeine));
            }
            if (gilt("cvar") && it.type != ITEM_TYPE_SLIDER && feldKopf(tr(Str::MlFEinstellung), "cvar", tr(Str::MlFEinstellungTip))) {
                std::string c = anzeige(it.cvar);
                textFeld("##cvar", &c, [i](const std::string& t) { eigenschaft(i, "cvar", {inAnf(t)}); }, tr(Str::MlKeine));
            }
            const int bed = bedingungArt(it);
            if (feldKopf(tr(Str::MlFBedingung), "showCvar, hideCvar, enableCvar, disableCvar", tr(Str::MlFBedingungTip))) {
                static const Str kNamen[4] = {Str::MlBedZeigen, Str::MlBedVerbergen, Str::MlBedAktiv, Str::MlBedGesperrt};
                const bool offen = ImGui::BeginCombo("##bedingung", tr(kNamen[bed]));
                z.feldStelle["##bedingung"] = ImVec2{(ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5F, (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5F};
                if (offen) {
                    for (int k = 0; k < 4; ++k) {
                        if (ImGui::Selectable(tr(kNamen[k]), k == bed) && k != bed) {
                            // Die Art wechseln: Der Wert bleibt, die alte Zeile geht
                            const std::string w0 = wert;
                            std::vector<Ersetzung> e;
                            if (!w0.empty() && !viele && entferneEigenschaft(d.datei, mi, i, kBedingung[bed], e) &&
                                setzeEigenschaft(d.datei, mi, i, kBedingung[k], {"{ " + inAnf(anzeige(w0)) + " }"}, e)) {
                                aendere(std::move(e));
                            } else {
                                z.bedingungNeu = k;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            static const char* const kSchluessel[4] = {"cvarTest, showCvar", "cvarTest, hideCvar", "cvarTest, enableCvar", "cvarTest, disableCvar"};
            if (feldKopf(tr(Str::MlFBedingungWert), viele ? "cvarTest" : kSchluessel[bed], tr(Str::MlFNurWennTip))) {
                std::string c = anzeige(it.cvarTest);
                std::string w = anzeige(wert);
                const float halb = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5F;
                ImGui::SetNextItemWidth(halb);
                textFeld("##cvartest", &c, [i](const std::string& t) { eigenschaft(i, "cvarTest", {inAnf(t)}); }, tr(Str::MlFCvar));
                if (!viele) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(halb);
                    const char* schluessel = kBedingung[z.bedingungNeu >= 0 ? z.bedingungNeu : bed];
                    textFeld("##showcvar", &w, [i, schluessel](const std::string& t) {
                        // PC_Script_Parse beendet den Block an "}" auch in Anfuehrungszeichen: Diese Zeichen kann ein Wert hier nicht tragen
                        std::string wert = t;
                        wert.erase(std::remove_if(wert.begin(), wert.end(), [](char c) { return c == '{' || c == '}' || c == ';'; }), wert.end());
                        eigenschaft(i, schluessel, {"{ " + inAnf(wert) + " }"});
                        Z().bedingungNeu = -1;
                    }, tr(Str::MlFHatWert));
                }
            }
        });
    }
    // Vergleich mit dem Original, Layout der Auswahl, Schluessel aus dem Mod-Profil: nur, wenn es sie hier gibt
    zeichneOriginal(d);
    zeichneLayoutGruppe(d);
    zeichneModSchluessel(d);
    l.markeItem = -1;
    // Was in der Datei steht und hier kein Feld hat
    weitereEintraegeLesen(d, it);
    if (l.such.empty()) {
        const std::size_t n = z.weitereEintraege.size();
        char zus[80];
        std::snprintf(zus, sizeof(zus), "%s", tr(Str::MlKeine));
        if (n > 0) { std::snprintf(zus, sizeof(zus), tr(Str::MlMehrAnzahl), static_cast<int>(n)); }
        gruppe("mehr", tr(Str::MeWeitereEintraege), zus, [&] {
            if (l.zaehlen) { return; }
            ImGui::PushTextWrapPos(0.0F);
            if (n == 0) {
                ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlMehrNichts));
            } else {
                for (const std::string& zeile : z.weitereEintraege) {
                    std::string s = anzeige(zeile);
                    if (!s.empty() && s.back() == '\r') { s.pop_back(); }
                    ImGui::TextDisabled("%s", s.c_str());
                }
                ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MeWeitereEintraegeTip));
                if (ImGui::Button(tr(Str::MlImTextOeffnen))) {
                    z.reiterGewollt = 1;
                    springe(d, it.anfang);
                }
            }
            ImGui::PopTextWrapPos();
        }, false);
    } else if (l.trefferGesamt == 0) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextColored(ImColor(kLsLeise), tr(Str::MlKeinFeld), l.suchRoh.c_str());
        ImGui::PopTextWrapPos();
    }
    lsZeilenEnde();
}

void eigenschaftenMenue(Dokument& d, int mi, const Menu& m) {
    Zustand& z = Z();
    Dokument* dp = &d;
    const bool voll = m.fullScreen != 0;
    gruppe("menue", tr(Str::MeMenueGruppe), tr(voll ? Str::MeVollbild : Str::MeUeberlagerung), [&] {
        const auto setze = [dp, mi](const char* k, const std::string& wert) {
            std::vector<Ersetzung> e;
            if (setzeEigenschaft(dp->datei, mi, -1, k, {wert}, e)) { aendere(std::move(e)); }
            else { ausMakroMelden(); }
        };
        if (feldKopf(tr(Str::MlFName), "name", tr(Str::MlFMenueNameTip))) {
            std::string n = anzeige(m.window.name);
            textFeld("##mname", &n, [setze](const std::string& t) {
                if (!t.empty()) { setze("name", inAnf(utf8ZuLatin1(t.c_str()))); }
            });
        }
        // rc608 Teil 3b: Fehlt sie, verschwindet der Text jedes Knopfs unter dem Zeiger (Item_TextColor)
        farbZeile(tr(Str::MlFFokusFarbe), "focusColor", tr(Str::MlFFokusFarbeTip), m.focusColor, "focusColor", [setze](const Farbe& f) { setze("focusColor", farbText(f)); });
        if (feldKopf(tr(Str::MlFAnzeige), "fullScreen", tr(Str::MlFAnzeigeTip))) {
            const int k = lsUmschalter("##anzeige", {{tr(Str::MeVollbild), nullptr}, {tr(Str::MeUeberlagerung), nullptr}}, voll ? 0 : 1);
            if (k == 0 && !voll) { setze("fullScreen", "1"); }
            if (k == 1 && voll) { setze("fullScreen", "0"); }
        }
        if (!Z().leiste.zaehlen) {
            lsZeilenEnde();
            ImGui::PushTextWrapPos(0.0F);
            ImGui::TextColored(ImColor(kLsLeise), "%s", tr(voll ? (d.datei.dialekt == Dialekt::MP ? Str::MeVollbildSatzMp : Str::MeVollbildSatzSp) : Str::MeUeberlagerungSatz));
            ImGui::PopTextWrapPos();
        }
        if (feldKopf(tr(Str::MlFHintergrund), "background", tr(Str::MlFHintergrundTip))) {
            std::string bg = m.window.background;
            ImGui::SetNextItemWidth(lsKnopfRest(tr(Str::MlWaehlen)));
            textFeld("##mbild", &bg, [setze](const std::string& t) { setze("background", inAnf(t)); }, tr(Str::MlFBildLeer));
            ImGui::SameLine();
            if (ImGui::Button(tr(Str::MlWaehlen))) { auswahlOeffnen(0, -1); }
        }
    });
    zeichneModSchluessel(d);
    lsZeilenEnde();
    (void)z;
}

void eigenschaftenMehrere(Dokument& d, int mi, const Menu& m) {
    Dokument* dp = &d;
    const float fs = ImGui::GetFontSize();
    std::vector<int> mitText;
    for (const int i : d.auswahl) {
        if (static_cast<std::size_t>(i) < m.items.size() && m.items[static_cast<std::size_t>(i)].type != ITEM_TYPE_MODEL) { mitText.push_back(i); }
    }
    if (!mitText.empty()) {
        const Item& erstes = m.items[static_cast<std::size_t>(mitText[0])];
        const auto gleichGross = std::all_of(mitText.begin(), mitText.end(), [&](int i) { return m.items[static_cast<std::size_t>(i)].textscale == erstes.textscale; });
        const auto gleichFarbe = std::all_of(mitText.begin(), mitText.end(), [&](int i) { return lsFarbeGleich(m.items[static_cast<std::size_t>(i)].window.foreColor, erstes.window.foreColor); });
        gruppe("alle", tr(Str::MlGrAlle), {}, [&] {
            if (!Z().leiste.zaehlen) {
                ImGui::PushTextWrapPos(0.0F);
                ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlAlleSatz));
                ImGui::PopTextWrapPos();
            }
            const auto alleSetzen = [dp, mi, mitText](const char* k, const std::string& wert) {
                std::vector<Ersetzung> e;
                for (const int i : mitText) { (void)setzeEigenschaft(dp->datei, mi, i, k, {wert}, e); }
                aendere(std::move(e));
            };
            if (feldKopf(tr(Str::MlFGroesse), "textscale", tr(Str::MlFGroesseTip), true)) {
                float ts = erstes.textscale;
                zahlenFeld("alle_scale", &ts, 1, [&ts] { return lsZahlMitSchieber("##alle_scale", "##alle_scale_s", &ts, 0.2F, 2.0F, 0.01F, "%g"); },
                           [alleSetzen](const std::vector<float>& w) { alleSetzen("textscale", zahlText(w[0])); });
                if (!gleichGross) {
                    ++Z().leiste.gemischt;
                    ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlGemischt));
                }
            }
            farbZeile(tr(Str::MlFTextfarbe), "forecolor", tr(Str::MlFTextfarbeTip), erstes.window.foreColor, "alle_fore",
                      [alleSetzen](const Farbe& f) { alleSetzen("forecolor", farbText(f)); });
            if (!gleichFarbe && !Z().leiste.zaehlen) {
                lsZeilenEnde();
                ++Z().leiste.gemischt;
                ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlGemischt));
            }
        });
    }
    gruppe("ordnen", tr(Str::MlGrOrdnen), {}, [&] {
        if (Z().leiste.zaehlen) { return; }
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlOrdnenSatz));
        ImGui::PopTextWrapPos();
        layoutKnoepfe(d);
        ImGui::Dummy(ImVec2{1.0F, fs * 0.2F});
        auto richte = [&](Ausrichtung a) {
            std::vector<Rechteck> r;
            for (const int i : d.auswahl) { r.push_back(m.items[static_cast<std::size_t>(i)].window.rectClient); }
            ausrichten(r, a);
            std::vector<std::pair<int, Rechteck>> neu;
            for (std::size_t k = 0; k < r.size(); ++k) { neu.emplace_back(d.auswahl[k], r[k]); }
            setzeRects(neu);
        };
        const auto reihe = [&](std::initializer_list<std::pair<Str, Ausrichtung>> e) {
            bool erstes = true;
            for (const auto& [s, a] : e) {
                if (!erstes) { daneben(knopfBreite(tr(s))); }
                erstes = false;
                if (ImGui::Button(tr(s))) { richte(a); }
            }
        };
        reihe({{Str::MeLinks, Ausrichtung::Links}, {Str::MeMitteH, Ausrichtung::MitteH}, {Str::MeRechts, Ausrichtung::Rechts}});
        reihe({{Str::MeOben, Ausrichtung::Oben}, {Str::MeMitteV, Ausrichtung::MitteV}, {Str::MeUnten, Ausrichtung::Unten}});
        auto verteile = [&](bool w) {
            std::vector<Rechteck> r;
            for (const int i : d.auswahl) { r.push_back(m.items[static_cast<std::size_t>(i)].window.rectClient); }
            verteilen(r, w);
            std::vector<std::pair<int, Rechteck>> neu;
            for (std::size_t k = 0; k < r.size(); ++k) { neu.emplace_back(d.auswahl[k], r[k]); }
            setzeRects(neu);
        };
        if (ImGui::Button(tr(Str::MeVerteilenH))) { verteile(true); }
        daneben(knopfBreite(tr(Str::MeVerteilenV)));
        if (ImGui::Button(tr(Str::MeVerteilenV))) { verteile(false); }
        static std::string gruppenName;
        ImGui::SetNextItemWidth(lsKnopfRest(tr(Str::MeGruppieren)));
        eingabe("##gruppe", &gruppenName);
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeGruppieren)) && !gruppenName.empty()) {
            std::vector<Ersetzung> e;
            for (const int i : d.auswahl) { (void)setzeEigenschaft(d.datei, mi, i, "group", {gruppenName}, e); }
            aendere(std::move(e));
        }
    }, false);
    zeichneLayoutGruppe(d);
    zeichneModSchluessel(d);
    lsZeilenEnde();
}

void zeichneEigenschaften(Dokument& d) {
    const int mi = menuDerSeite(d);
    if (mi < 0) {
        ImGui::TextDisabled("%s", tr(Str::MeNichtsGewaehlt));
        return;
    }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    if (d.auswahl.empty()) {
        eigenschaftenMenue(d, mi, m);
    } else if (d.auswahl.size() > 1) {
        eigenschaftenMehrere(d, mi, m);
    } else if (static_cast<std::size_t>(d.auswahl[0]) < m.items.size()) {
        ImGui::PushID(d.auswahl[0]);
        eigenschaftenElement(d, mi, d.auswahl[0], m.items[static_cast<std::size_t>(d.auswahl[0])]);
        ImGui::PopID();
    }
}

// --- Events --------------------------------------------------------------------------------------------------

struct SchrittInfo {
    const char* befehl;
    Str name;
    int gruppe;   // 0 Aussehen, 1 Bewegen, 2 Klang, 3 Menue, 4 Wert
};
const SchrittInfo kSchritte[] = {
    {"show", Str::MlSZeigen, 0},       {"hide", Str::MlSVerbergen, 0},    {"fadein", Str::MlSEinblenden, 0},     {"fadeout", Str::MlSAusblenden, 0},
    {"setitemcolor", Str::MlSFarbe, 0}, {"setitemtext", Str::MlSText, 0}, {"setitembackground", Str::MlSBild, 0},
    {"setitemrect", Str::MlSLage, 1},  {"transition", Str::MlSBewegen, 1}, {"transition2", Str::MlSBewegen2, 1}, {"orbit", Str::MlSOrbit, 1},
    {"play", Str::MlSKlang, 2},        {"playlooped", Str::MlSKlangLoop, 2},
    {"setfocus", Str::MlSFokus, 3},    {"open", Str::MlSOeffnen, 3},      {"close", Str::MlSSchliessen, 3},      {"setbackground", Str::MlSMenueBild, 3}, {"setcolor", Str::MlSMenueFarbe, 3},
    {"setcvar", Str::MlSCvar, 4},      {"setcvartocvar", Str::MlSCvarCvar, 4}, {"exec", Str::MlSBefehl, 4},       {"uiScript", Str::MlSSkript, 4},         {"delay", Str::MlSWarten, 4},
};

const SchrittInfo* schrittInfo(const std::string& befehl) {
    for (const SchrittInfo& s : kSchritte) {
        if (gleichOhneFallStr(befehl, s.befehl)) { return &s; }
    }
    return nullptr;
}

Str farbName(const std::string& prop) {
    if (gleichOhneFallStr(prop, "backcolor")) { return Str::MlFFuellFarbe; }
    if (gleichOhneFallStr(prop, "bordercolor")) { return Str::MlFRandFarbe; }
    return Str::MlFTextfarbe;
}

// Was der Schritt tut, in einer Zeile: "back · Text color", "ui_name = 1", "back · 40 420 160 36 -> 36 416 168 44"
std::string schrittZusammenfassung(const Baustein& x) {
    const auto wort = [&x](std::size_t i) { return i < x.args.size() ? anzeige(x.args[i]) : std::string(); };
    const std::string c = kleinPfad(x.befehl);
    const char* trenn = " \xC2\xB7 ";
    if (c == "play" || c == "playlooped") {
        const std::string p = wort(0);
        const std::size_t sl = p.rfind('/');
        return sl == std::string::npos ? p : p.substr(sl + 1);
    }
    if (c == "setitemcolor" && x.args.size() >= 2) { return wort(0) + trenn + tr(farbName(x.args[1])); }
    if (c == "setcolor" && !x.args.empty()) { return tr(farbName(x.args[0])); }
    if (c == "transition" && x.args.size() >= 9) {
        return wort(0) + trenn + wort(1) + " " + wort(2) + " " + wort(3) + " " + wort(4) + " -> " + wort(5) + " " + wort(6) + " " + wort(7) + " " + wort(8);
    }
    if (c == "setcvar" && x.args.size() >= 2) { return wort(0) + " = " + wort(1); }
    if (c == "setcvartocvar" && x.args.size() >= 2) { return wort(0) + " <- " + wort(1); }
    std::string s;
    for (std::size_t i = 0; i < x.args.size(); ++i) {
        if (!s.empty()) { s += trenn; }
        s += wort(i);
    }
    return s;
}

bool schrittFarbe(const Baustein& x, Farbe& f) {
    const std::string c = kleinPfad(x.befehl);
    const std::size_t ab = c == "setitemcolor" ? 2 : c == "setcolor" ? 1 : 99;
    if (x.args.size() < ab + 4) { return false; }
    f = Farbe{static_cast<float>(std::atof(x.args[ab].c_str())), static_cast<float>(std::atof(x.args[ab + 1].c_str())),
              static_cast<float>(std::atof(x.args[ab + 2].c_str())), static_cast<float>(std::atof(x.args[ab + 3].c_str()))};
    return true;
}

bool schrittZielFehlt(const Baustein& x, const Menu& m) { return befehlMitZiel(x.befehl) && !x.args.empty() && !zielDa(m, x.befehl, x.args[0]); }

// Probieren: laeuft ueber den Interaktionsmodus wie im Spiel, die Datei bleibt, Klaenge nach Einstellung
void probeStarten(Dokument& d, int item, const std::string& skript, const std::string& text) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    if (skript.empty()) { return; }
    if (l.probeStufe == 0) {
        l.probeWarAn = z.interaktion;
        if (!z.interaktion) { setzeInteraktion(true); }
    }
    l.probeSkript = skript;
    l.probeItem = item;
    l.probeMenu = menuDerSeite(d);
    l.probeText = text;
    l.probeStufe = 1;
    l.probeAb = ImGui::GetTime();
}

void probePflegen(Dokument& d) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    if (l.probeStufe == 0) { return; }
    const double jetzt = ImGui::GetTime();
    if (l.probeStufe == 1) {
        if (z.lauf.d == &d.datei && z.laufVersion >= 0) {   // die Flaeche hat das Menue geoeffnet (onOpen wie im Spiel)
            z.lauf.fuehreAus(l.probeMenu, l.probeItem, l.probeSkript);
            l.probeStufe = 2;
            l.probeAb = jetzt;
            ++l.probenGelaufen;
        } else if (jetzt - l.probeAb > 2.0) {
            l.probeStufe = 3;
        }
        return;
    }
    if (l.probeStufe == 2 && (jetzt - l.probeAb < 1.5 || (z.lauf.animiert() && jetzt - l.probeAb < 10.0))) { return; }
    l.probeStufe = 0;
    l.probeSkript.clear();
    if (!l.probeWarAn) { setzeInteraktion(false); }
}

template <class Schreiben>
void wortZeile(const char* id, const char* label, const char* tip, const std::string& wert, Schreiben schreiben) {
    if (!feldKopf(label, "", tip)) { return; }
    std::string w = wert;
    textFeld(id, &w, schreiben);
}

const std::vector<std::string>& menueVorschlaege() {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    l.vorschlaege.clear();
    std::set<std::string> s;
    for (const auto& dok : z.docs) {
        for (const Menu& m : dok->datei.menus) { s.insert(m.window.name); }
    }
    if (const auto dd = diagDaten()) { s.insert(dd->menues.begin(), dd->menues.end()); }
    s.erase(std::string());
    l.vorschlaege.assign(s.begin(), s.end());
    return l.vorschlaege;
}

const std::vector<std::string>& cvarVorschlaege(const Dokument& d) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    l.vorschlaege = spielCvarNamen(d.datei.dialekt, z.spiel);
    if (const auto dd = diagDaten()) { l.vorschlaege.insert(l.vorschlaege.end(), dd->cvars.begin(), dd->cvars.end()); }
    std::sort(l.vorschlaege.begin(), l.vorschlaege.end());
    l.vorschlaege.erase(std::unique(l.vorschlaege.begin(), l.vorschlaege.end()), l.vorschlaege.end());
    return l.vorschlaege;
}

// Ein Wort mit Vorschlaegen aus einer Liste; freies Schreiben bleibt
template <class Liste, class Schreiben>
void vorschlagZeile(const char* id, const char* label, const char* tip, const std::string& wert, Liste liste, Schreiben schreiben) {
    if (!feldKopf(label, "", tip)) { return; }
    Zustand& z = Z();
    const char* knopf = tr(Str::MlVorschlaege);
    std::string w = wert;
    ImGui::SetNextItemWidth(lsKnopfRest(knopf));
    textFeld(id, &w, schreiben);
    ImGui::SameLine();
    const std::string pop = std::string("##liste") + id;
    std::string gewaehlt;
    static const std::vector<std::string> leer;
    const std::string kn = std::string(knopf) + id;
    const std::vector<std::string>& v = ImGui::IsPopupOpen(pop.c_str()) ? liste() : leer;
    if (suchListe(pop.c_str(), kn.c_str(), z.listenFilter, v, gewaehlt)) { schreiben(gewaehlt); }
}

// Felder eines offenen Schritts
void schrittFelder(Dokument& d, int item, const std::string& schluessel, std::vector<Baustein>& b, std::size_t k, const Menu& m, bool& geaendert) {
    Zustand& z = Z();
    Baustein& x = b[k];
    Dokument* dp = &d;
    const std::string c = kleinPfad(x.befehl);
    const float fs = ImGui::GetFontSize();
    const std::vector<Baustein> alle = b;
    const auto wortSchreiben = [dp, item, schluessel, alle, k](std::size_t a) {
        return [dp, item, schluessel, alle, k, a](const std::string& t) {
            std::vector<Baustein> liste = alle;
            liste[k].args[a] = t;
            ereignisSchreiben(*dp, item, schluessel.c_str(), liste);
        };
    };
    const auto zielZeile = [&] {
        if (x.args.empty() || !befehlMitZiel(x.befehl)) { return; }
        if (!feldKopf(tr(Str::MlFElement), "", tr(Str::MlFElementTip))) { return; }
        if (ImGui::BeginCombo("##it", anzeige(x.args[0]).c_str())) {
            for (const Item& o : m.items) {
                if (!o.window.name.empty() && ImGui::Selectable(anzeige(o.window.name).c_str(), o.window.name == x.args[0])) {
                    x.args[0] = o.window.name;
                    geaendert = true;
                }
            }
            ImGui::EndCombo();
        }
    };
    // ab: Stelle in den Woertern; ein Zahlenblock schreibt die Woerter zurueck, die Folge bleibt wie im Spiel
    const auto zahlen = [&](const char* id, std::size_t ab, int n, const char* const* teile, float tempo) {
        float w[4] = {0, 0, 0, 0};
        for (int q = 0; q < n; ++q) { w[q] = static_cast<float>(std::atof(x.args[ab + static_cast<std::size_t>(q)].c_str())); }
        zahlenFeld(id, w, n, [&w, id, n, teile, tempo] { return zahlenReihe(id, w, n, teile, tempo, "%g"); },
                   [dp, item, schluessel, alle, k, ab, n](const std::vector<float>& wert) {
                       std::vector<Baustein> liste = alle;
                       for (int q = 0; q < n; ++q) { liste[k].args[ab + static_cast<std::size_t>(q)] = zahlText(wert[static_cast<std::size_t>(q)]); }
                       ereignisSchreiben(*dp, item, schluessel.c_str(), liste);
                   });
    };
    const auto farbWahl = [&](std::size_t stelle) {
        if (!feldKopf(tr(Str::MlFWelcheFarbe), "", nullptr)) { return; }
        static const char* const kEigen[3] = {"forecolor", "backcolor", "bordercolor"};
        if (ImGui::BeginCombo("##eigenschaft", tr(farbName(x.args[stelle])))) {
            for (const char* en : kEigen) {
                if (ImGui::Selectable(tr(farbName(en)), gleichOhneFallStr(x.args[stelle], en))) {
                    x.args[stelle] = en;
                    geaendert = true;
                }
            }
            ImGui::EndCombo();
        }
    };
    const auto farbFeld = [&](std::size_t ab) {
        Farbe f;
        if (!schrittFarbe(x, f)) { return; }
        const std::string offen = schluessel + "_" + std::to_string(k);
        farbZeile(tr(Str::MlFFarbe), "", nullptr, f, offen, [dp, item, schluessel, alle, k, ab](const Farbe& neu) {
            std::vector<Baustein> liste = alle;
            liste[k].args[ab] = zahlText(neu.r);
            liste[k].args[ab + 1] = zahlText(neu.g);
            liste[k].args[ab + 2] = zahlText(neu.b);
            liste[k].args[ab + 3] = zahlText(neu.a);
            ereignisSchreiben(*dp, item, schluessel.c_str(), liste);
        });
    };
    const auto dateiKnopfZeile = [&](const char* id, const char* label, const char* tip, std::size_t a) {
        if (!feldKopf(label, "", tip)) { return; }
        std::string w = x.args[a];
        ImGui::SetNextItemWidth(lsKnopfRest(tr(Str::MlWaehlen)));
        textFeld(id, &w, wortSchreiben(a));
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MlWaehlen))) { auswahlFuerSchritt(item, schluessel, k, a, alle); }
    };
    zielZeile();
    if ((c == "transition") && x.args.size() == 11) {
        // Script_Transition: Name, von (Rect_Parse), nach (Rect_Parse), Zeit, Schrittweite
        static const char* const kRect[4] = {"x", "y", "w", "h"};
        const Rechteck jetzt = [&] {
            for (const Item& o : m.items) {
                if (gleichOhneFallStr(o.window.name, x.args[0])) { return o.window.rectClient; }
            }
            return Rechteck{};
        }();
        const auto block = [&](const char* id, const char* titel, std::size_t ab) {
            ImGui::PushID(id);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(titel);
            const char* kn = tr(Str::MlJetzigeLage);
            daneben(knopfBreite(kn));
            if (ImGui::Button(kn)) {
                x.args[ab] = zahlText(jetzt.x);
                x.args[ab + 1] = zahlText(jetzt.y);
                x.args[ab + 2] = zahlText(jetzt.w);
                x.args[ab + 3] = zahlText(jetzt.h);
                geaendert = true;
            }
            lsTipp(tr(Str::MlJetzigeLageTip));
            zahlen(id, ab, 4, kRect, 1.0F);
            ImGui::PopID();
        };
        lsZeilenEnde();
        block("von", tr(Str::MeEvVon), 1);
        block("nach", tr(Str::MeEvNach), 5);
        // Skizze: von grau, nach blau
        {
            const float w0[4] = {static_cast<float>(std::atof(x.args[1].c_str())), static_cast<float>(std::atof(x.args[2].c_str())), static_cast<float>(std::atof(x.args[3].c_str())), static_cast<float>(std::atof(x.args[4].c_str()))};
            const float w1[4] = {static_cast<float>(std::atof(x.args[5].c_str())), static_cast<float>(std::atof(x.args[6].c_str())), static_cast<float>(std::atof(x.args[7].c_str())), static_cast<float>(std::atof(x.args[8].c_str()))};
            const float x0 = std::min(w0[0], w1[0]), y0 = std::min(w0[1], w1[1]);
            const float x1 = std::max(w0[0] + w0[2], w1[0] + w1[2]), y1 = std::max(w0[1] + w0[3], w1[1] + w1[3]);
            const ImVec2 groesse{ImGui::GetContentRegionAvail().x, fs * 4.4F};
            ImGui::Dummy(groesse);
            const ImVec2 a = ImGui::GetItemRectMin(), e = ImGui::GetItemRectMax();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(a, e, IM_COL32(0, 0, 0, 255), 4.0F);
            const float rand = fs * 0.5F;
            const float f = std::min((groesse.x - rand * 2.0F) / std::max(1.0F, x1 - x0), (groesse.y - rand * 2.0F) / std::max(1.0F, y1 - y0));
            const ImVec2 o{a.x + (groesse.x - (x1 - x0) * f) * 0.5F, a.y + (groesse.y - (y1 - y0) * f) * 0.5F};
            const auto kasten = [&](const float* r, ImU32 farbe) { dl->AddRect(ImVec2{o.x + (r[0] - x0) * f, o.y + (r[1] - y0) * f}, ImVec2{o.x + (r[0] + r[2] - x0) * f, o.y + (r[1] + r[3] - y0) * f}, farbe, 0.0F, 0, 1.5F); };
            kasten(w0, kLsLeise);
            kasten(w1, kLsBlauHell);
        }
        const char* const teile[2] = {tr(Str::MeEvZeit), tr(Str::MeEvSchritte)};
        zahlen("zeit", 9, 2, teile, 1.0F);
        lsTipp(tr(Str::MlFZeitTip));
        if (ImGui::Button(tr(Str::MlProbeSchritt))) { probeStarten(d, item, zusammensetzen({x}), std::string(tr(Str::MlSBewegen)) + " " + x.args[0]); }
        lsTipp(tr(Str::MlProbeSchrittTip));
    } else if (c == "setitemcolor" && x.args.size() == 6) {
        // Script_SetItemColor: Ziel, Eigenschaft, Farbe
        farbWahl(1);
        farbFeld(2);
    } else if (c == "setcolor" && x.args.size() == 5) {
        farbWahl(0);
        farbFeld(1);
    } else if (c == "setitemrect" && x.args.size() == 5) {
        static const char* const kRect[4] = {"x", "y", "w", "h"};
        lsZeilenEnde();
        zahlen("lage", 1, 4, kRect, 1.0F);
    } else if (c == "setitemtext" && x.args.size() >= 2) {
        wortZeile("##a1", tr(Str::MlFText), nullptr, x.args[1], wortSchreiben(1));
    } else if (c == "setitembackground" && x.args.size() >= 2) {
        dateiKnopfZeile("##a1", tr(Str::MlFuellBild), tr(Str::MlFBildTip), 1);
    } else if (c == "setbackground" && !x.args.empty()) {
        dateiKnopfZeile("##a0", tr(Str::MlFuellBild), tr(Str::MlFBildTip), 0);
    } else if ((c == "open" || c == "close") && !x.args.empty()) {
        vorschlagZeile("##a0", tr(Str::MlFMenue), nullptr, x.args[0], [] () -> const std::vector<std::string>& { return menueVorschlaege(); }, wortSchreiben(0));
    } else if ((c == "play" || c == "playlooped") && !x.args.empty()) {
        if (feldKopf(tr(Str::MlFKlang), "", nullptr)) {
            std::string w = x.args[0];
            const float kn = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(std::max(fs * 3.0F, ImGui::GetContentRegionAvail().x - kn - ImGui::GetStyle().ItemSpacing.x));
            textFeld("##a0", &w, wortSchreiben(0));
            ImGui::SameLine();
            if (lsKnopf("##hoeren", Ls::Spiel, tr(Str::MeEvProbe), false, false, true, kn)) { probehoeren(x.args[0]); }
        }
    } else if (c == "setcvar" && x.args.size() >= 2) {
        vorschlagZeile("##a0", tr(Str::MlFCvar), nullptr, x.args[0], [dp]() -> const std::vector<std::string>& { return cvarVorschlaege(*dp); }, wortSchreiben(0));
        wortZeile("##a1", tr(Str::MlFWert), nullptr, x.args[1], wortSchreiben(1));
    } else if (c == "setcvartocvar" && x.args.size() >= 2) {
        vorschlagZeile("##a0", tr(Str::MlFCvarZiel), nullptr, x.args[0], [dp]() -> const std::vector<std::string>& { return cvarVorschlaege(*dp); }, wortSchreiben(0));
        vorschlagZeile("##a1", tr(Str::MlFCvarQuelle), nullptr, x.args[1], [dp]() -> const std::vector<std::string>& { return cvarVorschlaege(*dp); }, wortSchreiben(1));
    } else if (c == "exec" && !x.args.empty()) {
        wortZeile("##a0", tr(Str::MlFBefehl), nullptr, x.args[0], wortSchreiben(0));
    } else if (c == "uiscript" && !x.args.empty()) {
        wortZeile("##a0", tr(Str::MlFSkript), nullptr, x.args[0], wortSchreiben(0));
        for (std::size_t a = 1; a < x.args.size(); ++a) {
            ImGui::PushID(static_cast<int>(a));
            wortZeile("##a", tr(Str::MlFWert), nullptr, x.args[a], wortSchreiben(a));
            ImGui::PopID();
        }
    } else if (c == "delay" && !x.args.empty()) {
        wortZeile("##a0", tr(Str::MlFZeit), nullptr, x.args[0], wortSchreiben(0));
    } else {
        // orbit, transition2 und was das Spiel sonst kennt: die Woerter wie gelesen, eines je Feld
        for (std::size_t a = befehlMitZiel(x.befehl) ? 1 : 0; a < x.args.size(); ++a) {
            ImGui::PushID(static_cast<int>(a));
            wortZeile("##a", tr(Str::MlFWert), nullptr, x.args[a], wortSchreiben(a));
            ImGui::PopID();
        }
    }
    lsZeilenEnde();
    (void)z;
}

// Die Kopfzeile eines Schritts. Hoch, Runter und Entfernen stehen fest rechts; Rueckgabe: 1 aufklappen, 2 hoch, 3 runter, 4 entfernen.
int schrittKopf(std::size_t k, std::size_t anzahl, const Baustein& x, bool offen, bool zielFehlt) {
    const SchrittInfo* info = schrittInfo(x.befehl);
    const char* titel = info != nullptr ? tr(info->name) : x.befehl.c_str();
    const std::string zus = schrittZusammenfassung(x);
    const float fs = ImGui::GetFontSize();
    const float kn = std::floor(ImGui::GetFrameHeight() * 0.95F);
    const float luft = ImGui::GetStyle().ItemSpacing.x * 0.4F;
    const float breit = ImGui::GetContentRegionAvail().x;
    const float links = std::max(fs * 4.0F, breit - kn * 3.0F - luft * 3.0F);
    const float innen = links - fs * 0.5F;
    Farbe farbe;
    const bool hatFarbe = schrittFarbe(x, farbe);
    const float tupfer = hatFarbe ? fs * 1.1F : 0.0F;
    const float tw = ImGui::CalcTextSize(titel).x;
    const float zw = ImGui::CalcTextSize(zus.c_str()).x + tupfer;
    const bool eineZeile = tw + fs * 0.5F + zw <= innen;
    char warn[300];
    warn[0] = '\0';
    if (zielFehlt) { std::snprintf(warn, sizeof(warn), tr(Str::MlZielFehlt), x.args[0].c_str()); }
    const bool keys = g_app->settings.menueDateiSchluessel;
    float h = ImGui::GetTextLineHeight();
    if (!eineZeile && !zus.empty()) { h += ImGui::CalcTextSize(zus.c_str(), nullptr, false, innen - tupfer).y; }
    if (warn[0] != '\0') { h += ImGui::CalcTextSize(warn, nullptr, false, innen).y; }
    if (keys) { h += ImGui::GetTextLineHeight(); }
    h = std::max(h + fs * 0.3F, kn);
    const std::string id = "##st" + std::to_string(k);
    int ergebnis = 0;
    const ImVec2 start = ImGui::GetCursorScreenPos();
    if (ImGui::InvisibleButton(id.c_str(), ImVec2{links, h})) { ergebnis = 1; }
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (ImGui::IsItemHovered()) { dl->AddRectFilled(a, b, kLsFeld, 4.0F); }
    ImVec2 p{a.x + fs * 0.25F, a.y + fs * 0.15F};
    dl->AddText(p, ImGui::GetColorU32(ImGuiCol_Text), titel);
    if (eineZeile) {
        const float zx = p.x + tw + fs * 0.5F;
        dl->AddText(ImVec2{zx, p.y}, kLsLeise, zus.c_str());
        if (hatFarbe) {
            const float qx = zx + ImGui::CalcTextSize(zus.c_str()).x + fs * 0.25F;
            dl->AddRectFilled(ImVec2{qx, p.y + fs * 0.1F}, ImVec2{qx + fs * 0.8F, p.y + fs * 0.9F}, ImColor(farbe.r, farbe.g, farbe.b, std::max(farbe.a, 0.25F)), 2.0F);
        }
        p.y += ImGui::GetTextLineHeight();
    } else {
        p.y += ImGui::GetTextLineHeight();
        if (!zus.empty()) {
            dl->AddText(ImGui::GetFont(), fs, p, kLsLeise, zus.c_str(), nullptr, innen - tupfer);
            if (hatFarbe) {
                const float qx = p.x + innen - tupfer + fs * 0.1F;
                dl->AddRectFilled(ImVec2{qx, p.y + fs * 0.1F}, ImVec2{qx + fs * 0.8F, p.y + fs * 0.9F}, ImColor(farbe.r, farbe.g, farbe.b, std::max(farbe.a, 0.25F)), 2.0F);
            }
            p.y += ImGui::CalcTextSize(zus.c_str(), nullptr, false, innen - tupfer).y;
        }
    }
    if (warn[0] != '\0') {
        dl->AddText(ImGui::GetFont(), fs, p, kLsGelb, warn, nullptr, innen);
        p.y += ImGui::CalcTextSize(warn, nullptr, false, innen).y;
    }
    if (keys) { dl->AddText(p, IM_COL32(0x94, 0xBF, 0xE9, 255), x.befehl.c_str()); }
    (void)offen;
    const float knopfY = start.y + std::floor((h - kn) * 0.5F);
    const float knopfX = start.x + links + luft;
    ImGui::SetCursorScreenPos(ImVec2{knopfX, knopfY});
    if (lsKnopf(("##st_h" + std::to_string(k)).c_str(), Ls::Hoch, tr(Str::MlFrueher), false, false, k > 0, kn)) { ergebnis = 2; }
    ImGui::SameLine(0.0F, luft);
    if (lsKnopf(("##st_r" + std::to_string(k)).c_str(), Ls::Runter, tr(Str::MlSpaeter), false, false, k + 1 < anzahl, kn)) { ergebnis = 3; }
    ImGui::SameLine(0.0F, luft);
    if (lsKnopf(("##st_x" + std::to_string(k)).c_str(), Ls::X, tr(Str::MeEvEntfernen), false, true, true, kn)) { ergebnis = 4; }
    ImGui::SetCursorScreenPos(ImVec2{start.x, start.y + h});
    ImGui::Dummy(ImVec2{1.0F, 0.0F});
    return ergebnis;
}

// Add step: Suche, die haeufigsten oben, nach Aufgabe sortiert; nur, was das Spiel kennt
void zeichneSchrittWahl(Dokument& d, std::vector<Baustein>& b, const Item* it, const Menu& m, bool& geaendert) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    const float fs = ImGui::GetFontSize();
    const std::vector<std::string> kennt = bausteinBefehle(d.datei.dialekt, z.spiel);
    const auto gibt = [&kennt](const char* c) { return std::any_of(kennt.begin(), kennt.end(), [c](const std::string& s) { return gleichOhneFallStr(s, c); }); };
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (l.addNeu) {
        ImGui::SetKeyboardFocusHere();
        l.addNeu = false;
    }
    eingabe("##add_q", &l.addq, 0, tr(Str::MlAddSuche));
    ImGui::BeginChild("##add_liste", ImVec2{0.0F, std::min(fs * 13.0F, ImGui::GetContentRegionAvail().x * 1.2F)}, ImGuiChildFlags_Borders);
    const std::string f = kleinPfad(l.addq);
    Item leer;
    const auto zeile = [&](const SchrittInfo& s, const char* vor) {
        ImGui::PushID(vor);
        ImGui::PushID(s.befehl);
        const float cw = ImGui::CalcTextSize(s.befehl).x;
        const float breit = ImGui::GetContentRegionAvail().x;
        const char* name = tr(s.name);
        const bool zweiZeilen = ImGui::CalcTextSize(name).x + cw + fs > breit;
        const float h = ImGui::GetTextLineHeight() * (zweiZeilen ? 2.0F : 1.0F);
        const std::string kennung = std::string("##neu_") + vor + s.befehl;
        if (ImGui::Selectable(kennung.c_str(), false, ImGuiSelectableFlags_None, ImVec2{0.0F, h})) {
            b.push_back(neuerBaustein(s.befehl, it != nullptr ? *it : leer, m));
            l.auf = static_cast<int>(b.size()) - 1;
            l.add = false;
            geaendert = true;
        }
        const ImVec2 a = ImGui::GetItemRectMin(), e = ImGui::GetItemRectMax();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(a, ImGui::GetColorU32(ImGuiCol_Text), name);
        dl->AddText(ImVec2{e.x - cw, e.y - ImGui::GetTextLineHeight()}, IM_COL32(0x94, 0xBF, 0xE9, 255), s.befehl);
        ImGui::PopID();
        ImGui::PopID();
    };
    bool irgendwas = false;
    if (f.empty()) {
        ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlAddOft));
        for (const char* c : {"open", "close", "play", "setcvar"}) {
            const SchrittInfo* s = schrittInfo(c);
            if (s != nullptr && gibt(c)) {
                zeile(*s, "oft");
                irgendwas = true;
            }
        }
    }
    static const Str kGruppen[5] = {Str::MlAddAussehen, Str::MlAddBewegen, Str::MlAddKlang, Str::MlAddMenue, Str::MlAddWert};
    for (int g = 0; g < 5; ++g) {
        bool kopf = false;
        for (const SchrittInfo& s : kSchritte) {
            if (s.gruppe != g || !gibt(s.befehl)) { continue; }
            if (!f.empty() && kleinPfad(std::string(tr(s.name)) + " " + s.befehl).find(f) == std::string::npos) { continue; }
            if (!kopf) {
                ImGui::TextColored(ImColor(kLsLeise), "%s", tr(kGruppen[g]));
                kopf = true;
            }
            zeile(s, "alle");
            irgendwas = true;
        }
    }
    if (!irgendwas) { ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlAddNichts)); }
    ImGui::EndChild();
}

void zeichneEreignisse(Dokument& d) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    const int mi = menuDerSeite(d);
    if (mi < 0) {
        ImGui::TextDisabled("%s", tr(Str::MeEvEinElement));
        return;
    }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    if (d.auswahl.size() > 1) {
        eigenschaftenMehrere(d, mi, m);
        return;
    }
    const float fs = ImGui::GetFontSize();
    ImGui::PushID("ereignisse");
    const bool hatItem = d.auswahl.size() == 1 && static_cast<std::size_t>(d.auswahl[0]) < m.items.size();
    const int i = hatItem ? d.auswahl[0] : -1;
    const Item* it = hatItem ? &m.items[static_cast<std::size_t>(i)] : nullptr;
    // Ausloeser
    struct Ausl {
        std::string schluessel;
        std::string skript;
        Str name;
    };
    std::vector<Ausl> ausl;
    if (it != nullptr) {
        for (const Ereignis& e : ereignisseFuer(d.datei.dialekt, z.spiel)) { ausl.push_back(Ausl{e.schluessel, skriptVon(*it, e.art), kEreignisNamen[e.art]}); }
    } else {
        ausl.push_back(Ausl{"onOpen", m.onOpen, Str::MlEvMenueAuf});
        ausl.push_back(Ausl{"onClose", m.onClose, Str::MlEvMenueZu});
        ausl.push_back(Ausl{"onESC", m.onESC, Str::MlEvEsc});
        ausl.push_back(Ausl{"onAccept", m.onAccept, Str::MlEvEnter});   // Menu_HandleKey: Enter, wenn kein Element die Taste nimmt
    }
    if (std::none_of(ausl.begin(), ausl.end(), [&l](const Ausl& a) { return a.schluessel == l.trig; })) {
        const std::string vorzug = it != nullptr ? "action" : "onOpen";
        l.trig = std::any_of(ausl.begin(), ausl.end(), [&vorzug](const Ausl& a) { return a.schluessel == vorzug; }) ? vorzug : (ausl.empty() ? std::string() : ausl[0].schluessel);
        l.auf = 0;
    }
    bool erste = true;
    const bool keys = g_app->settings.menueDateiSchluessel;
    for (const Ausl& a : ausl) {
        const std::vector<Baustein> b = zerlege(a.skript, d.datei.dialekt, z.spiel);
        const bool warn = std::any_of(b.begin(), b.end(), [&m](const Baustein& x) { return schrittZielFehlt(x, m); });
        const std::string text = keys ? std::string(tr(a.name)) + "  " + a.schluessel : std::string(tr(a.name));
        const std::string zahl = b.empty() ? std::string() : std::to_string(b.size());
        const float tw = ImGui::CalcTextSize(text.c_str()).x;
        const float zw = zahl.empty() ? 0.0F : ImGui::CalcTextSize(zahl.c_str()).x + fs * 0.75F;
        const ImVec2 g{tw + zw + fs * 1.3F, ImGui::GetFrameHeight()};
        if (!erste) { daneben(g.x); }
        erste = false;
        const std::string kennung = "##tr_" + a.schluessel;
        if (ImGui::InvisibleButton(kennung.c_str(), g)) {
            l.trig = a.schluessel;
            l.auf = 0;
            l.add = false;
            l.farbeOffen.clear();
        }
        const ImVec2 p0 = ImGui::GetItemRectMin(), p1 = ImGui::GetItemRectMax();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const bool an = l.trig == a.schluessel;
        dl->AddRectFilled(p0, p1, an ? kLsBlau : ImGui::IsItemHovered() ? kLsFeldUeber : kLsFeld, g.y * 0.5F);
        const float ty = p0.y + (g.y - ImGui::GetTextLineHeight()) * 0.5F;
        dl->AddText(ImVec2{p0.x + fs * 0.65F, ty}, an || !b.empty() ? ImGui::GetColorU32(ImGuiCol_Text) : kLsLeise, text.c_str());
        if (!zahl.empty()) {
            const ImVec2 z0{p0.x + fs * 0.65F + tw + fs * 0.3F, ty};
            const ImVec2 z1{z0.x + zw - fs * 0.3F, ty + ImGui::GetTextLineHeight()};
            dl->AddRectFilled(z0, z1, warn ? kLsGelb : IM_COL32(255, 255, 255, 34), fs * 0.4F);
            dl->AddText(ImVec2{z0.x + fs * 0.22F, ty}, warn ? IM_COL32(0x20, 0x16, 0x00, 255) : ImGui::GetColorU32(ImGuiCol_Text), zahl.c_str());
        }
        lsTipp(a.schluessel.c_str());
    }
    ImGui::Dummy(ImVec2{1.0F, fs * 0.2F});
    const auto gefunden = std::find_if(ausl.begin(), ausl.end(), [&l](const Ausl& a) { return a.schluessel == l.trig; });
    if (gefunden == ausl.end()) {
        ImGui::PopID();
        return;
    }
    const std::string schluessel = gefunden->schluessel;
    std::vector<Baustein> b = zerlege(gefunden->skript, d.datei.dialekt, z.spiel);
    Dokument* dp = &d;
    // Beim passenden Ausloeser: Klang (Hover = focusSound, Item_SetFocus; Klick = play am Anfang von action) und die Hover-Effekte
    if (it != nullptr && (schluessel == "mouseEnter" || schluessel == "action")) {
        if (schluessel == "mouseEnter") {
            if (feldKopf(tr(Str::MlHoverEffekt), "", nullptr)) {
                ImGui::ColorEdit4("##hoverfarbe", &z.hoverFarbe.r, ImGuiColorEditFlags_NoInputs);
                tipp(Str::MeEvFarbeTip);
                const Str eff[] = {Str::MeEvLeuchten, Str::MeEvFarbe, Str::MeEvRahmen, Str::MeEvGroesser};
                const Str effTip[] = {Str::MeEvLeuchtenTip, Str::MeEvFarbeEffTip, Str::MeEvRahmenTip, Str::MeEvGroesserTip};
                for (int k = 0; k < 4; ++k) {
                    daneben(knopfBreite(tr(eff[k])));
                    if (ImGui::Button(tr(eff[k]))) {
                        std::vector<Ersetzung> e;
                        if (hoverEffekt(d.datei, mi, i, static_cast<HoverEffekt>(k), z.hoverFarbe, e)) {
                            aendere(std::move(e));
                            // Leuchten setzt das Glow-Element davor: die Auswahl bleibt beim Element selbst
                            if (static_cast<HoverEffekt>(k) == HoverEffekt::Leuchten) { d.auswahl.assign(1, i + 1); }
                        } else {
                            ausMakroMelden();
                        }
                    }
                    lsTipp(tr(effTip[k]));
                }
            }
        }
        const auto klangZeile = [&](const char* id, const char* idHoeren, Str label, Str tip, std::string& wert, auto schreiben) {
            if (!feldKopf(tr(label), "", tr(tip))) { return; }
            const float kn = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(std::max(fs * 3.0F, ImGui::GetContentRegionAvail().x - kn - ImGui::GetStyle().ItemSpacing.x));
            textFeld(id, &wert, schreiben, tr(Str::MlKeinKlang));
            lsTipp(tr(tip));
            ImGui::SameLine();
            if (lsKnopf(idHoeren, Ls::Spiel, tr(Str::MeEvProbe), false, false, true, kn)) { probehoeren(wert); }
        };
        if (schluessel == "mouseEnter") {
            std::string w = it->focusSound;
            klangZeile("##hoverklang", "##hk", Str::MeEvHoverKlang, Str::MeEvHoverKlangTip, w, [i](const std::string& t) { eigenschaft(i, "focusSound", {inAnf(t)}); });
        } else {
            std::string w;
            for (const Baustein& x : b) {
                if (x.befehl == "play" && !x.args.empty()) {
                    w = x.args[0];
                    break;
                }
            }
            const std::vector<Baustein> klick = b;
            klangZeile("##klickklang", "##kk", Str::MeEvKlickKlang, Str::MeEvKlickKlangTip, w, [dp, i, klick](const std::string& t) {
                std::vector<Baustein> liste = klick;
                bool gesetzt = false;
                for (Baustein& x : liste) {
                    if (x.befehl == "play" && !x.args.empty()) {
                        x.args[0] = t;
                        gesetzt = true;
                        break;
                    }
                }
                if (!gesetzt) { liste.insert(liste.begin(), Baustein{"play", {t}, {true}}); }
                if (t.empty()) {
                    liste.erase(std::remove_if(liste.begin(), liste.end(), [](const Baustein& x) { return x.befehl == "play"; }), liste.end());
                }
                ereignisSchreiben(*dp, i, "action", liste);
            });
        }
        lsZeilenEnde();
        ImGui::Dummy(ImVec2{1.0F, fs * 0.1F});
        ImGui::Separator();
    }
    // Die Schritte
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlDann));
    if (!b.empty()) {
        const char* probe = tr(Str::MlProbeAlle);
        const float pb = knopfBreite(probe);
        if (ImGui::GetContentRegionAvail().x >= pb) {
            daneben(pb);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0F, ImGui::GetContentRegionAvail().x - pb));
        }
        if (ImGui::Button(probe)) {
            std::string liste;
            for (const Baustein& x : b) {
                const SchrittInfo* s = schrittInfo(x.befehl);
                if (!liste.empty()) { liste += ", "; }
                liste += s != nullptr ? tr(s->name) : x.befehl.c_str();
                if (kleinPfad(x.befehl) == "play" && !x.args.empty()) { liste += " " + x.args[0]; }
            }
            probeStarten(d, i, zusammensetzen(b), liste);
        }
        lsTipp(tr(Str::MlProbeAlleTip));
    }
    bool geaendert = false;
    if (b.empty()) { ImGui::TextColored(ImColor(kLsLeise), "%s", tr(Str::MlKeineSchritte)); }
    for (std::size_t k = 0; k < b.size(); ++k) {
        ImGui::PushID(static_cast<int>(k));
        const bool offen = l.auf == static_cast<int>(k);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImDrawListSplitter sp;
        sp.Split(dl, 2);
        sp.SetCurrentChannel(dl, 1);
        const ImVec2 kartenAnfang = ImGui::GetCursorScreenPos();
        const float breit = ImGui::GetContentRegionAvail().x;
        ImGui::Dummy(ImVec2{1.0F, fs * 0.05F});
        const int kopf = schrittKopf(k, b.size(), b[k], offen, schrittZielFehlt(b[k], m));
        bool weg = false;
        if (kopf == 1) {
            l.auf = offen ? -1 : static_cast<int>(k);
            l.farbeOffen.clear();
        } else if (kopf == 2 && k > 0) {
            std::swap(b[k], b[k - 1]);
            if (l.auf == static_cast<int>(k)) { l.auf = static_cast<int>(k) - 1; }
            else if (l.auf == static_cast<int>(k) - 1) { l.auf = static_cast<int>(k); }
            geaendert = true;
        } else if (kopf == 3 && k + 1 < b.size()) {
            std::swap(b[k], b[k + 1]);
            if (l.auf == static_cast<int>(k)) { l.auf = static_cast<int>(k) + 1; }
            else if (l.auf == static_cast<int>(k) + 1) { l.auf = static_cast<int>(k); }
            geaendert = true;
        } else if (kopf == 4) {
            weg = true;
        }
        if (!weg && offen && kopf == 0) {
            ImGui::Indent(fs * 0.3F);
            schrittFelder(d, i, schluessel, b, k, m, geaendert);
            ImGui::Unindent(fs * 0.3F);
            ImGui::Dummy(ImVec2{1.0F, fs * 0.15F});
        }
        const float unten = ImGui::GetCursorScreenPos().y;
        sp.SetCurrentChannel(dl, 0);
        dl->AddRectFilled(kartenAnfang, ImVec2{kartenAnfang.x + breit, unten}, kLsKarte, 5.0F);
        if (offen) { dl->AddRect(kartenAnfang, ImVec2{kartenAnfang.x + breit, unten}, kLsBlau, 5.0F); }
        sp.Merge(dl);
        ImGui::Dummy(ImVec2{1.0F, fs * 0.1F});
        ImGui::PopID();
        if (weg) {
            b.erase(b.begin() + static_cast<std::ptrdiff_t>(k));
            l.auf = -1;
            geaendert = true;
            break;
        }
        if (geaendert) { break; }
    }
    // Add step
    {
        const char* add = tr(Str::MlAddStep);
        ImGui::PushStyleColor(ImGuiCol_Button, l.add ? kLsBlau : kLsFeld);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, l.add ? kLsBlau : kLsFeldUeber);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kLsBlau);
        if (ImGui::Button(add, ImVec2{-FLT_MIN, 0.0F})) {
            l.add = !l.add;
            l.addNeu = l.add;
            l.addq.clear();
        }
        ImGui::PopStyleColor(3);
        // rc608 Teil 3b: Bei wenig Hoehe lag der Knopf oder die aufgeklappte Liste unter dem Rand der Spalte
        if (!ImGui::IsItemVisible() && tourSicht().aktiv && tourSicht().id == "tut-menue-eigen" && tourSicht().schritt == 6 && schluessel == "action") { ImGui::SetScrollHereY(0.9F); }
        const bool listeNeu = l.add && l.addNeu;
        const std::string suchVorher = l.addq;
        if (l.add) { zeichneSchrittWahl(d, b, it, m, geaendert); }
        if (listeNeu || suchVorher != l.addq) { ImGui::SetScrollHereY(1.0F); }
    }
    if (geaendert) { ereignisSchreiben(d, i, schluessel.c_str(), b); }
    lsZeilenEnde();
    ImGui::PopID();
    (void)dp;
}

// --- Fuss und Hilfezeile ----------------------------------------------------------------------------------------

std::string meldungenKnopf(const Dokument& d) { return std::string(tr(Str::MeMeldungen)) + ": " + std::to_string(d.datei.meldungen.size()); }

// Die Hoehe von Fuss und Hilfezeile: der Inhalt darueber bekommt den Rest
float fussHoehe(const Dokument& d, bool mitKomponente) {
    const Zustand::Leiste& l = Z().leiste;
    const float fs = ImGui::GetFontSize();
    const ImGuiStyle& st = ImGui::GetStyle();
    const float breit = ImGui::GetContentRegionAvail().x;
    float x = 0.0F;
    int zeilen = 1;
    const auto dazu = [&](const char* t) {
        const float w = ImGui::CalcTextSize(t).x + st.FramePadding.x * 2.0F;
        if (x > 0.0F && x + st.ItemSpacing.x + w > breit) {
            ++zeilen;
            x = 0.0F;
        }
        x += (x > 0.0F ? st.ItemSpacing.x : 0.0F) + w;
    };
    if (mitKomponente) { dazu(tr(Str::MeKomp)); }
    dazu(tr(Str::MlVorschauCvars));
    dazu(meldungenKnopf(d).c_str());
    float h = (ImGui::GetFrameHeight() + st.ItemSpacing.y) * static_cast<float>(zeilen) + st.ItemSpacing.y;
    if (l.tablett != 0) { h += std::floor(fs * 14.0F) + st.ItemSpacing.y; }
    h += std::floor(ImGui::GetTextLineHeight() * 2.4F) + st.ItemSpacing.y;
    return h;
}

void zeichneFuss(Dokument& d, bool mitKomponente) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    const float fs = ImGui::GetFontSize();
    ImGui::Separator();
    const auto knopf = [&](const std::string& text, int nr, const char* tip, bool erstes) {
        if (!erstes) { daneben(knopfBreite(text.c_str())); }
        const bool an = l.tablett == nr;
        ImGui::PushStyleColor(ImGuiCol_Button, an ? kLsBlau : kLsFeld);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, an ? kLsBlau : kLsFeldUeber);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kLsBlau);
        const std::string id = text + "###fuss" + std::to_string(nr);
        if (ImGui::Button(id.c_str())) { l.tablett = an ? 0 : nr; }
        ImGui::PopStyleColor(3);
        lsTipp(tip);
    };
    if (l.tablett == 1 && !mitKomponente) { l.tablett = 0; }
    if (mitKomponente) { knopf(tr(Str::MeKomp), 1, tr(Str::MeKompTip), true); }
    knopf(tr(Str::MlVorschauCvars), 2, tr(Str::MeCvarsNurVorschau), !mitKomponente);
    knopf(meldungenKnopf(d), 3, tr(Str::MeMeldungenSatz), false);
    if (l.tablett != 0) {
        ImGui::BeginChild("##tablett", ImVec2{0.0F, std::floor(fs * 14.0F)}, ImGuiChildFlags_Borders);
        if (l.tablett == 1) { zeichneKomponente(d); }
        if (l.tablett == 2) { zeichneCvars(d); }
        if (l.tablett == 3) { zeichneMeldungen(d); }
        ImGui::EndChild();
    }
    lsZeilenEnde();
}

void zeichneHilfezeile() {
    Zustand::Leiste& l = Z().leiste;
    ImGui::Separator();
    std::string text = l.hilfeNeu.empty() ? std::string(tr(Str::MlHilfeStart)) : l.hilfeNeu;
    if (l.probeStufe != 0) {
        char b[600];
        std::snprintf(b, sizeof(b), tr(Str::MlProbeLaeuft), l.probeText.c_str());
        text = b;
    }
    l.hilfe = text;
    l.hilfeNeu.clear();
    ImGui::BeginChild("##hilfe", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextColored(ImColor(kLsLeise), "%s", text.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndChild();
}

// Eine Seite der Leiste: 0 Properties, 1 Events, 2 Text-Editor
void leisteSeite(Dokument& d, int art) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    if (l.auswahl != d.auswahl || l.auswahlDok != &d) {
        l.auswahl = d.auswahl;
        l.auswahlDok = &d;
        l.auf = 0;
        l.add = false;
        l.farbeOffen.clear();
        l.trig.clear();
    }
    if (art != 0 || d.auswahl.size() != 1) {
        l.such.clear();
        l.suchRoh.clear();
    }
    probePflegen(d);
    l.schluesselZeilen = l.schluesselGezaehlt;
    l.schluesselGezaehlt = 0;
    l.gemischtGezeigt = l.gemischt;
    l.gemischt = 0;
    l.textMass = TextMass{};
    if (const int mi = menuDerSeite(d); mi >= 0 && d.auswahl.size() == 1 && static_cast<std::size_t>(d.auswahl[0]) < d.datei.menus[static_cast<std::size_t>(mi)].items.size()) {
        l.textMass = textMassVon(d, d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(d.auswahl[0])]);
    }
    if (art != 2) { zeichneKarte(d, art); }
    const bool komponente = d.auswahl.size() == 1 && menuDerSeite(d) >= 0;
    const float unten = fussHoehe(d, komponente);
    if (art == 2) {
        ImGui::BeginChild("##textseite", ImVec2{0.0F, -unten}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        panelText(d);
        ImGui::EndChild();
    } else {
        ImGui::BeginChild("##inhalt", ImVec2{0.0F, -unten}, ImGuiChildFlags_None);
        if (Z().leiste.auswahlVorher != d.auswahl) {   // Die Ausloeser des neuen Elements lagen sonst ueber dem Rand der Spalte
            Z().leiste.auswahlVorher = d.auswahl;
            ImGui::SetScrollY(0.0F);
        }
        if (art == 0) {
            zeichneEigenschaften(d);
            if (g_app->settings.menueTextUnten) {   // ohne Reiter: Events darunter
                ImGui::SeparatorText(tr(Str::MeEreignisse));
                zeichneEreignisse(d);
            }
        } else {
            zeichneEreignisse(d);
        }
        lsZeilenEnde();
        ImGui::EndChild();
    }
    zeichneFuss(d, komponente);
    zeichneHilfezeile();
}

}  // namespace
