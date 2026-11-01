// app_menue_links.inc - rc608 Teil 3a: die linke Leiste des Menue-Editors nach dem freigegebenen Entwurf (in app_menue.cpp eingebunden).
// Oben die Suche, darunter die Teile als Kacheln (Show, Player input, Ready-made, My templates), unten dauerhaft die Hierarchie.
// Alle Kacheln gehen denselben Weg: Ziehen auf die Flaeche oder Doppelklick (ME_VORLAGE, setzeAblageEin); ein einfacher Klick tut nichts.

namespace {

constexpr ImU32 kKachelBlau = IM_COL32(0x5A, 0x9B, 0xEA, 255);
constexpr ImU32 kKachelHell = IM_COL32(0xE6, 0xE7, 0xEA, 255);

// Was eine Kachel zeigt; gezeichnet im Raum 52 x 30 des Entwurfs
enum class KachelBild { Text, Knopf, Bild, Video, Modell, JaNein, Multi, Schieber, Eingabe, Liste, Taste, Lautstaerke, Reiter, Detail, Spieler };

void kachelBildMalen(ImDrawList* dl, KachelBild art, ImVec2 o, float k) {
    const auto p = [&](float x, float y) { return ImVec2{o.x + x * k, o.y + y * k}; };
    const float d = std::max(1.0F, 1.3F * k);
    const auto rand = [&](float x, float y, float w, float h, float r = 1.0F) { dl->AddRect(p(x, y), p(x + w, y + h), kKachelHell, r * k, 0, d); };
    const auto voll = [&](float x, float y, float w, float h, ImU32 c, float r = 1.0F) { dl->AddRectFilled(p(x, y), p(x + w, y + h), c, r * k); };
    const auto strich = [&](float x0, float y0, float x1, float y1, ImU32 c = kKachelHell, float dicke = 0.0F) { dl->AddLine(p(x0, y0), p(x1, y1), c, dicke > 0.0F ? dicke * k : d); };
    const auto mittig = [&](const char* t, float my, float groesse, ImU32 c) {
        const float g = groesse * k;
        const ImVec2 w = ImGui::GetFont()->CalcTextSizeA(g, FLT_MAX, 0.0F, t);
        dl->AddText(ImGui::GetFont(), g, ImVec2{o.x + 26.0F * k - w.x * 0.5F, o.y + my * k - w.y * 0.5F}, c, t);
    };
    switch (art) {
        case KachelBild::Text: mittig("Aa", 15.0F, 17.0F, kKachelHell); break;
        case KachelBild::Knopf: voll(6, 7, 40, 16, kKachelBlau, 4.0F); break;
        case KachelBild::Bild:
            rand(10, 3, 32, 24, 2.0F);
            dl->AddTriangleFilled(p(13, 24), p(21, 14), p(29, 24), kKachelHell);
            dl->AddTriangleFilled(p(25, 24), p(31, 17), p(38, 24), kKachelHell);
            dl->AddCircleFilled(p(35, 9), 2.4F * k, kKachelBlau);
            break;
        case KachelBild::Video:
            rand(10, 3, 32, 24, 2.0F);
            dl->AddTriangleFilled(p(22, 9), p(22, 21), p(32, 15), kKachelBlau);
            break;
        case KachelBild::Modell:
            strich(18, 9, 26, 5);
            strich(26, 5, 34, 9);
            strich(34, 9, 34, 21);
            strich(34, 21, 26, 25);
            strich(26, 25, 18, 21);
            strich(18, 21, 18, 9);
            strich(18, 9, 26, 13);
            strich(26, 13, 34, 9);
            strich(26, 13, 26, 25);
            break;
        case KachelBild::JaNein:
            voll(12, 8, 28, 14, kKachelBlau, 7.0F);
            dl->AddCircleFilled(p(33, 15), 5.5F * k, kKachelHell);
            break;
        case KachelBild::Multi: mittig("< A >", 15.0F, 13.0F, kKachelHell); break;
        case KachelBild::Schieber:
            strich(6, 15, 46, 15);
            voll(24, 7, 5, 16, kKachelBlau, 1.5F);
            break;
        case KachelBild::Eingabe:
            rand(6, 8, 40, 14, 2.0F);
            strich(12, 11, 12, 19, kKachelBlau, 1.5F);
            break;
        case KachelBild::Liste:
            rand(10, 4, 32, 22);
            strich(13, 10, 39, 10);
            strich(13, 15, 39, 15);
            strich(13, 20, 39, 20);
            break;
        case KachelBild::Taste:
            rand(17, 6, 18, 18, 3.0F);
            mittig("K", 15.0F, 11.0F, kKachelBlau);
            break;
        case KachelBild::Lautstaerke:
            strich(4, 15, 13, 15, kKachelHell, 2.0F);
            strich(18, 15, 48, 15);
            voll(37, 8, 4, 14, kKachelBlau);
            break;
        case KachelBild::Reiter:
            voll(4, 9, 14, 12, IM_COL32(0x27, 0x4A, 0x75, 255), 2.0F);
            voll(19, 9, 14, 12, IM_COL32(0x4A, 0x4D, 0x57, 255), 2.0F);
            voll(34, 9, 14, 12, IM_COL32(0x4A, 0x4D, 0x57, 255), 2.0F);
            break;
        case KachelBild::Detail:
            voll(4, 4, 44, 6, IM_COL32(0x27, 0x4A, 0x75, 255));
            rand(4, 12, 13, 14);
            rand(19, 12, 29, 14);
            break;
        case KachelBild::Spieler:
            rand(10, 4, 32, 22);
            voll(11, 9, 30, 6, IM_COL32(0x27, 0x4A, 0x75, 255), 0.0F);
            strich(13, 20, 39, 20);
            break;
    }
}

// Der Name bricht um, darum ist eine Kachel so hoch, wie die laengste Beschriftung ihrer Gruppe es braucht: nichts wird abgeschnitten
float kachelTextHoehe(const char* name, float breite) { return ImGui::CalcTextSize(name, nullptr, true, breite).y; }

// Eine Kachel. ablage: was gezogen wird (Bibliothek-Nummer oder ablageWidget). true = Doppelklick
bool bauteilKachel(const char* name, int ablage, ImVec2 groesse, const char* hinweis, const std::function<void(ImDrawList*, ImVec2, float)>& bild) {
    (void)ImGui::InvisibleButton(name, groesse);
    const bool ueber = ImGui::IsItemHovered();
    // rc608 Teil 3a: erst beim Loslassen und nur ohne Ziehen. Griff man schnell ein zweites Mal zu und zog, setzte die Kachel zweimal ein: Doppelklick und Ablage.
    const ImGuiIO& io = ImGui::GetIO();
    const bool doppelt = ueber && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && io.MouseClickedLastCount[ImGuiMouseButton_Left] == 2 &&
                         io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] < io.MouseDragThreshold * io.MouseDragThreshold;
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ME_VORLAGE", &ablage, sizeof(ablage));
        ImGui::TextUnformatted(name, ImGui::FindRenderedTextEnd(name));
        ImGui::EndDragDropSource();
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    const float fs = ImGui::GetFontSize();
    dl->AddRectFilled(a, b, IM_COL32(0x26, 0x28, 0x2D, 255), 6.0F);
    if (ueber) { dl->AddRect(a, b, kKachelBlau, 6.0F); }
    const float k = fs * 3.7F / 52.0F;
    const ImVec2 bo{(a.x + b.x) * 0.5F - 26.0F * k, a.y + fs * 0.4F};
    bild(dl, bo, k);
    // der Name darunter, mittig, Zeile fuer Zeile umbrochen
    const char* ende = ImGui::FindRenderedTextEnd(name);
    const float innen = b.x - a.x - fs * 0.3F;
    const float top = bo.y + 30.0F * k + fs * 0.15F;
    ImFont* schrift = ImGui::GetFont();
    float y = top;
    const char* s = name;
    float breiteste = 0.0F;
    while (s < ende) {
        const char* z = schrift->CalcWordWrapPosition(fs, s, ende, innen);
        if (z == s) { z = s + 1; }
        const float w = schrift->CalcTextSizeA(fs, FLT_MAX, 0.0F, s, z).x;
        breiteste = std::max(breiteste, w);
        dl->AddText(ImVec2{(a.x + b.x - w) * 0.5F, y}, ImGui::GetColorU32(ImGuiCol_Text), s, z);
        y += ImGui::GetTextLineHeight();
        s = z;
        while (s < ende && (*s == ' ' || *s == '\n')) { ++s; }
    }
    Z().kacheln.push_back(KachelMass{name, ImRect(a, b), breiteste, y});
    if (ueber && !ImGui::IsDragDropActive()) { ImGui::SetTooltip("%s", hinweis); }
    return doppelt;
}

struct Teil {
    int ablage;      // ablageWidget(typ) oder -1000 - Nummer der Bibliothek ueber ihre Kennung
    const char* id;  // Kennung in der Bibliothek, sonst nullptr
    Str name, tip;
    KachelBild bild;
};

std::string ablageNachId(const char* id, int& nummer) {
    const std::vector<Vorlage>& v = vorlagen();
    std::string text;
    nummer = -1;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (!v[i].eigen && v[i].id == id) {
            nummer = static_cast<int>(i);
            text = v[i].text;
        }
    }
    return text;
}

std::string vorlagenTipp(const std::string& text, const char* satz) {
    int n = 0;
    for (std::size_t p = text.find("itemDef"); p != std::string::npos; p = text.find("itemDef", p + 7)) { ++n; }
    char b[120];
    std::snprintf(b, sizeof(b), trN(Str::MeBtAnzahl, n), n);
    std::string t = b;
    if (satz != nullptr) { t += std::string("\n") + satz; }
    return t;
}

// Die Miniatur einer eigenen Vorlage: die Rechtecke und Farben ihrer Elemente
const std::vector<MiniEintrag>& vorlagenMiniatur(const Vorlage& v, const Dokument& d) {
    Zustand& z = Z();
    auto it = z.vorlagenMini.find(v.id);
    if (it != z.vorlagenMini.end()) { return it->second; }
    std::vector<MiniEintrag> e;
    LeseOptionen o;
    o.dialekt = d.datei.dialekt;
    o.spiel = d.datei.spiel;
    const Datei p = lies("ui/vorlage.menu", "{ menuDef { " + v.text + " } }", o);
    if (!p.menus.empty()) {
        for (const Item& i : p.menus[0].items) { e.push_back(MiniEintrag{i.window.rectClient, i.window.backColor, i.window.style == WINDOW_STYLE_FILLED}); }
    }
    return z.vorlagenMini.emplace(v.id, std::move(e)).first->second;
}

void miniaturMalen(ImDrawList* dl, const std::vector<MiniEintrag>& e, ImVec2 o, float k) {
    if (e.empty()) { return; }
    float x0 = 1e9F, y0 = 1e9F, x1 = -1e9F, y1 = -1e9F;
    for (const MiniEintrag& x : e) {
        x0 = std::min(x0, x.r.x);
        y0 = std::min(y0, x.r.y);
        x1 = std::max(x1, x.r.x + x.r.w);
        y1 = std::max(y1, x.r.y + x.r.h);
    }
    const float m = std::min(46.0F / std::max(1.0F, x1 - x0), 26.0F / std::max(1.0F, y1 - y0));
    for (const MiniEintrag& x : e) {
        const ImVec2 a{o.x + (3.0F + (x.r.x - x0) * m) * k, o.y + (2.0F + (x.r.y - y0) * m) * k};
        const ImVec2 b{a.x + std::max(2.0F, x.r.w * m) * k, a.y + std::max(2.0F, x.r.h * m) * k};
        if (x.voll) { dl->AddRectFilled(a, b, ImColor(x.f.r, x.f.g, x.f.b, 1.0F), k); }
        dl->AddRect(a, b, kKachelHell, k, 0, std::max(1.0F, 0.8F * k));
    }
}

// Eine Gruppe von Kacheln, die sich nach der Breite ordnen
void kachelGruppe(const char* id, const char* titel, const std::vector<Teil>& teile, const std::string& filter) {
    Zustand::Leiste& l = Z().leiste;
    std::vector<const Teil*> gezeigt;
    for (const Teil& t : teile) {
        if (filter.empty() || kleinPfad(tr(t.name)).find(filter) != std::string::npos) { gezeigt.push_back(&t); }
    }
    if (gezeigt.empty()) { return; }
    const bool offen = !filter.empty() || l.zu.count(id) == 0;
    char zus[60];
    std::snprintf(zus, sizeof(zus), trN(Str::MlTeileAnzahl, gezeigt.size()), static_cast<int>(gezeigt.size()));
    if (gruppenKopf(id, titel, zus, offen) && filter.empty()) {
        if (l.zu.count(id) != 0) { l.zu.erase(id); } else { l.zu.insert(id); }
    }
    if (offen) {
        const float fs = ImGui::GetFontSize();
        const float abstand = ImGui::GetStyle().ItemSpacing.x;
        const float breit = ImGui::GetContentRegionAvail().x;
        const int spalten = std::max(1, static_cast<int>((breit + abstand) / (fs * 5.0F + abstand)));
        const float w = (breit - abstand * static_cast<float>(spalten - 1)) / static_cast<float>(spalten);
        float text = ImGui::GetTextLineHeight();
        for (const Teil* t : gezeigt) { text = std::max(text, kachelTextHoehe(tr(t->name), w - fs * 0.3F)); }
        const ImVec2 groesse{w, std::ceil(fs * 0.55F + 30.0F * fs * 3.7F / 52.0F + fs * 0.15F + text + fs * 0.3F)};
        int nr = 0;
        for (const Teil* t : gezeigt) {
            if (nr % spalten != 0) { ImGui::SameLine(); }
            ++nr;
            int ablage = t->ablage;
            std::string tip = tr(t->tip);
            if (t->id != nullptr) {
                const std::string vtext = ablageNachId(t->id, ablage);
                tip = vorlagenTipp(vtext, tr(t->tip));
            }
            const KachelBild art = t->bild;
            if (bauteilKachel(tr(t->name), ablage, groesse, tip.c_str(), [art](ImDrawList* dl, ImVec2 o, float k) { kachelBildMalen(dl, art, o, k); })) {
                setzeAblageEin(ablage, false, 0, 0);
            }
        }
    }
    ImGui::Dummy(ImVec2{1.0F, ImGui::GetFontSize() * 0.1F});
}

void kachelnEigene(Dokument& d, const std::string& filter) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    const std::vector<Vorlage>& v = vorlagen();
    const char* id = "l_vorlagen";
    std::vector<int> eigene;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].eigen && (filter.empty() || kleinPfad(v[i].name).find(filter) != std::string::npos)) { eigene.push_back(static_cast<int>(i)); }
    }
    const bool offen = !filter.empty() || l.zu.count(id) == 0;
    if (gruppenKopf(id, tr(Str::MlLVorlagen), std::to_string(eigene.size()), offen) && filter.empty()) {
        if (l.zu.count(id) != 0) { l.zu.erase(id); } else { l.zu.insert(id); }
    }
    if (!offen) { return; }
    const float fs = ImGui::GetFontSize();
    const float abstand = ImGui::GetStyle().ItemSpacing.x;
    const float breit = ImGui::GetContentRegionAvail().x;
    const int spalten = std::max(1, static_cast<int>((breit + abstand) / (fs * 5.0F + abstand)));
    const float w = (breit - abstand * static_cast<float>(spalten - 1)) / static_cast<float>(spalten);
    float text = ImGui::GetTextLineHeight();
    for (const int i : eigene) { text = std::max(text, kachelTextHoehe(anzeige(v[static_cast<std::size_t>(i)].name).c_str(), w - fs * 0.3F)); }
    const ImVec2 groesse{w, std::ceil(fs * 0.55F + 30.0F * fs * 3.7F / 52.0F + fs * 0.15F + text + fs * 0.3F)};
    std::string umbenennen, loeschen;
    // Die Knoepfe liegen auf der Kachel, darum feste Stellen statt SameLine
    const ImVec2 anfang = ImGui::GetCursorScreenPos();
    const float zeilenAbstand = ImGui::GetStyle().ItemSpacing.y;
    int nr = 0;
    for (const int i : eigene) {
        const Vorlage& x = v[static_cast<std::size_t>(i)];
        const ImVec2 start{anfang.x + static_cast<float>(nr % spalten) * (groesse.x + abstand), anfang.y + static_cast<float>(nr / spalten) * (groesse.y + zeilenAbstand)};
        ++nr;
        ImGui::SetCursorScreenPos(start);
        ImGui::PushID(x.id.c_str());
        const std::string name = anzeige(x.name);
        const std::vector<MiniEintrag>& mini = vorlagenMiniatur(x, d);
        const std::string tip = vorlagenTipp(x.text, tr(Str::MlVorlageEigenTip));
        ImGui::SetNextItemAllowOverlap();
        if (bauteilKachel(name.c_str(), i, groesse, tip.c_str(), [&mini](ImDrawList* dl, ImVec2 o, float k) { miniaturMalen(dl, mini, o, k); })) {
            setzeAblageEin(i, false, 0, 0);
        }
        const float g = std::floor(fs * 1.3F);
        ImGui::SetCursorScreenPos(ImVec2{start.x + groesse.x - g * 2.0F - 3.0F, start.y + 2.0F});
        if (lsKnopf("##v_umbenennen", Ls::Stift, tr(Str::MlVorlageUmbenennen), false, false, true, g)) { umbenennen = x.id; }
        ImGui::SameLine(0.0F, 1.0F);
        if (lsKnopf("##v_loeschen", Ls::X, tr(Str::ActDelete), false, true, true, g)) { loeschen = x.id; }
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(ImVec2{anfang.x, anfang.y + static_cast<float>((nr + spalten - 1) / spalten) * (groesse.y + zeilenAbstand)});
    ImGui::Dummy(ImVec2{1.0F, 0.0F});
    if (!umbenennen.empty()) {
        z.vorlageUmbenennen = umbenennen;
        z.vorlageName = umbenennen;
        z.vorlageSpeichern = false;
        z.vorlageFokus = true;
    }
    if (!loeschen.empty()) {
        // Sie liegt nicht in der Datei und damit nicht im Undo: Die Zeile unten nimmt es zurueck
        for (const Vorlage& x : v) {
            if (x.eigen && x.id == loeschen) { z.vorlageWeg = x; }
        }
        (void)loescheVorlage(vorlagenOrdner(), loeschen);
        z.vorlagenGeladen = false;
        z.vorlagenMini.clear();
    }
    if (!z.vorlageWeg.id.empty()) {
        char b[300];
        std::snprintf(b, sizeof(b), tr(Str::MlVorlageGeloescht), anzeige(z.vorlageWeg.name).c_str());
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", b);
        ImGui::PopTextWrapPos();
        if (ImGui::Button(tr(Str::MlVorlageZurueck))) {
            (void)speichereVorlage(vorlagenOrdner(), z.vorlageWeg.name, z.vorlageWeg.text);
            z.vorlageWeg = Vorlage{};
            z.vorlagenGeladen = false;
            z.vorlagenMini.clear();
        }
    }
    const auto namensfeld = [&](const char* knopf) {
        ImGui::SetNextItemWidth(lsKnopfRest(knopf));
        if (z.vorlageFokus) {
            ImGui::SetKeyboardFocusHere();
            z.vorlageFokus = false;
        }
        const bool enter = eingabe("##vorlagename", &z.vorlageName, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        return ImGui::Button(knopf) || enter;
    };
    const bool frei = !z.vorlageName.empty() && !vorlageGibtEs(vorlagenOrdner(), z.vorlageName);
    if (!z.vorlageUmbenennen.empty()) {
        if (namensfeld(tr(Str::MlVorlageUmbenennen))) {
            if (z.vorlageName == z.vorlageUmbenennen) {
                z.vorlageUmbenennen.clear();
            } else if (frei && benenneVorlageUm(vorlagenOrdner(), z.vorlageUmbenennen, z.vorlageName)) {
                z.vorlageUmbenennen.clear();
                z.vorlagenGeladen = false;
                z.vorlagenMini.clear();
            }
        }
    } else if (z.vorlageSpeichern && !d.auswahl.empty()) {
        if (namensfeld(tr(Str::MlVorlageSpeichernJetzt)) && frei) {
            if (speichereVorlage(vorlagenOrdner(), z.vorlageName, kopiereAuswahl())) {
                z.vorlageSpeichern = false;
                z.vorlagenGeladen = false;
                z.vorlagenMini.clear();
            }
        }
    } else {
        z.vorlageSpeichern = false;
        ImGui::BeginDisabled(d.auswahl.empty());
        if (ImGui::Button(tr(Str::MeVorlageSpeichern), ImVec2{-FLT_MIN, 0.0F})) {
            z.vorlageSpeichern = true;
            z.vorlageFokus = true;
            const int mi = menuDerSeite(d);
            const std::string vor = d.auswahl.size() == 1 && mi >= 0 ? d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(d.auswahl[0])].window.name : std::string();
            z.vorlageName = vor.empty() ? tr(Str::MlVorlageVorgabe) : vor;
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", tr(d.auswahl.empty() ? Str::MlVorlageErstWaehlen : Str::MlVorlageSpeichernTip)); }
    }
    if ((z.vorlageSpeichern || !z.vorlageUmbenennen.empty()) && !frei && !z.vorlageName.empty() && z.vorlageName != z.vorlageUmbenennen) {
        ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", tr(Str::MlVorlageDoppelt));
    }
}

void zeichnePalette(Dokument& d) {
    Zustand& z = Z();
    const auto w = [](int typ) { return ablageWidget(typ); };
    const std::vector<Teil> zeigen = {{w(ITEM_TYPE_TEXT), nullptr, Str::MlTypText, Str::MeWTextTip, KachelBild::Text},
                                      {w(ITEM_TYPE_BUTTON), nullptr, Str::MlTypButton, Str::MlWButtonTip, KachelBild::Knopf},
                                      {w(-1), nullptr, Str::MlTypBild, Str::MlWBildTip, KachelBild::Bild},
                                      {w(-3), nullptr, Str::MlTypVideo, Str::MlWVideoTip, KachelBild::Video},
                                      {w(ITEM_TYPE_MODEL), nullptr, Str::MlTypModell, Str::MeWModellTip, KachelBild::Modell}};
    const std::vector<Teil> eingabeTeile = {{w(ITEM_TYPE_YESNO), nullptr, Str::MlTypJaNein, Str::MeWJaNeinTip, KachelBild::JaNein},
                                            {w(ITEM_TYPE_MULTI), nullptr, Str::MlTypMulti, Str::MeWMultiTip, KachelBild::Multi},
                                            {w(ITEM_TYPE_SLIDER), nullptr, Str::MlTypSchieber, Str::MeWSchieberTip, KachelBild::Schieber},
                                            {w(ITEM_TYPE_EDITFIELD), nullptr, Str::MlTypEingabe, Str::MeWEingabeTip, KachelBild::Eingabe},
                                            {w(ITEM_TYPE_LISTBOX), nullptr, Str::MlTypListe, Str::MeWListeTip, KachelBild::Liste},
                                            {w(ITEM_TYPE_BIND), nullptr, Str::MlTypTaste, Str::MeWTasteTip, KachelBild::Taste}};
    const std::vector<Teil> fertig = {{0, "schieber", Str::MlBtLautstaerke, Str::MeBtSchieberTip, KachelBild::Lautstaerke},
                                      {0, "reiter", Str::MlBtReiter, Str::MeBtReiterTip, KachelBild::Reiter},
                                      {0, "detail", Str::MlBtDetail, Str::MeBtDetailTip, KachelBild::Detail},
                                      {0, "liste", Str::MlBtSpieler, Str::MeBtListeTip, KachelBild::Spieler}};
    z.kacheln.clear();
    const float fs = ImGui::GetFontSize();
    // Suche und, bis zum ersten Einfuegen, der Hinweis
    ImGui::SetNextItemWidth(-FLT_MIN);
    (void)eingabe("##teile_suche", &z.teileSuche, 0, tr(Str::MlTeileSuche));
    if (!g_app->settings.menueTeileHinweisWeg) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::MlTeileHinweis));
        ImGui::PopTextWrapPos();
    }
    const std::string f = kleinPfad(z.teileSuche);
    ImGui::BeginChild("##teile_liste", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None);
    kachelGruppe("l_zeigen", tr(Str::MlLZeigen), zeigen, f);
    kachelGruppe("l_eingabe", tr(Str::MlLEingabe), eingabeTeile, f);
    kachelGruppe("l_fertig", tr(Str::MlLFertig), fertig, f);
    kachelnEigene(d, f);
    ImGui::Dummy(ImVec2{1.0F, fs * 0.2F});
    z.kachelnScrollMaxX = ImGui::GetScrollMaxX();
    ImGui::EndChild();
}

}  // namespace

// Links: oben die Teile, darunter dauerhaft die Hierarchie, zuklappbar
void panelBausteine(Dokument& d) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    ImGui::PushID("panelBausteine");
    const float h = ImGui::GetContentRegionAvail().y;
    const float kopf = std::floor(ImGui::GetFontSize() * 1.75F) + ImGui::GetStyle().ItemSpacing.y;
    const bool hierarchieOffen = l.zu.count("l_hier") == 0;
    const float unten = hierarchieOffen ? std::floor(h * 0.37F) : kopf;
    ImGui::BeginChild("##teile", ImVec2{0.0F, std::max(ImGui::GetFontSize() * 6.0F, h - unten - ImGui::GetStyle().ItemSpacing.y)}, ImGuiChildFlags_None);
    zeichnePalette(d);
    ImGui::EndChild();
    ImGui::BeginChild("##ebenen", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const int mi = menuDerSeite(d);
    const std::size_t anzahl = mi >= 0 ? d.datei.menus[static_cast<std::size_t>(mi)].items.size() : 0;
    char zus[60];
    std::snprintf(zus, sizeof(zus), trN(Str::MlElementeAnzahl, anzahl), static_cast<int>(anzahl));
    if (gruppenKopf("l_hier", tr(Str::MeReiterHierarchie), zus, hierarchieOffen)) {
        if (hierarchieOffen) { l.zu.insert("l_hier"); } else { l.zu.erase("l_hier"); }
    }
    if (hierarchieOffen) { panelHierarchie(d); }
    ImGui::EndChild();
    ImGui::PopID();
}
