// app_figuranim_ui.inc - Modell-Block Stufe 7: die Oberflaeche von "Figur animieren" (Fenster, Ansicht, Dopesheet, Kurven,
// Bauen, Aktionssuche, Rechtsklick). Eingebunden am Ende von gui/app_figuranim.cpp (dieselbe Uebersetzungseinheit, die Helfer
// im unbenannten Namensraum dort).

namespace bhed::gui {
namespace {

// --- Aktionen fuer Menue, Suche, Rechtsklick ----------------------------------------------------------------------------------

struct FaAktion {
    Str text;
    Str hinweis;
    const char* max;       // Taste im Modus wie 3ds Max (leer: keine)
    const char* blender;   // Taste im Modus wie Blender
    int gruppe;            // 0 Figur, 1 Posieren, 2 IK/FK, 3 Pfade, 4 Bauen, 5 Auto-Physik (Stufe 6)
    std::function<void()> tun;
};

modell::IkArt gliedJetzt(bool* da = nullptr) {
    modell::IkArt art = modell::IkArt::ArmRechts;
    const std::shared_ptr<FigurDaten> d = daten(fa().figur);
    const bool ok = d->da && gliedDerWahl(*d, art);
    if (da != nullptr) { *da = ok; }
    return art;
}

void gizmoSetzen(int g) {
    fa().gizmo = g;
    melden(tr(g == 0 ? Str::FaGizmoWaehlen : (g == 1 ? Str::FaGizmoVerschieben : Str::FaGizmoDrehen)));
}

void modalStarten(int art) {
    const std::shared_ptr<FigurDaten> d = daten(fa().figur);
    if (!d->da) { return; }
    const std::vector<FaGriff> v = griffeBei(*d, d->wirk, bildJetzt(*d));
    const int w = gewaehlterGriff(v);
    if (w < 0) {
        melden(tr(Str::MbRgKeineAuswahl));
        return;
    }
    zugBeginnen(*d, art, v[static_cast<std::size_t>(w)].ort);
    fa().modal = true;
}

bool gliedAktion(const std::function<void(modell::IkArt)>& f) {
    bool da = false;
    const modell::IkArt art = gliedJetzt(&da);
    if (!da) {
        melden(tr(Str::MbIkGliedWaehlen));
        return false;
    }
    f(art);
    return true;
}

bool figurAnimBauenFrage();

std::vector<FaAktion> aktionen() {
    return {
        {Str::FaAnimieren, Str::FaAnimierenHint, "", "", 0, [] { (void)figurAnimStarten(fa().figur); }},
        {Str::FaEntfernen, Str::FaEntfernenHint, "", "", 0, [] { (void)figurAnimEntfernen(fa().figur); }},
        {Str::FaGizmoWaehlen, Str::FaGizmoWaehlenHint, "Q", "", 1, [] { gizmoSetzen(0); }},
        {Str::FaGizmoVerschieben, Str::FaGizmoVerschiebenHint, "W", "G", 1, [] {
             if (modellBlender()) {
                 modalStarten(1);
             } else {
                 gizmoSetzen(1);
             }
         }},
        {Str::FaGizmoDrehen, Str::FaGizmoDrehenHint, "E", "R", 1, [] {
             if (modellBlender()) {
                 modalStarten(2);
             } else {
                 gizmoSetzen(2);
             }
         }},
        {Str::FaSchluesselSetzen, Str::FaSchluesselSetzenHint, "K", "I", 1, [] { schluesselSetzen(); }},
        {Str::FaSchluesselLoeschen, Str::FaSchluesselLoeschenHint, "Delete", "Alt+I", 1, [] { schluesselLoeschen(); }},
        {Str::FaIkAn, Str::FaIkAnHint, "", "", 2, [] { (void)gliedAktion([](modell::IkArt a) { ikSchalten(a, true); }); }},
        {Str::FaIkAus, Str::FaIkAusHint, "", "", 2, [] { (void)gliedAktion([](modell::IkArt a) { ikSchalten(a, false); }); }},
        {Str::MbIkZielAufFk, Str::MbIkZielAufFkHint, "", "", 2, [] { (void)gliedAktion([](modell::IkArt a) { ikEinrasten(a, true); }); }},
        {Str::MbIkFkAufIk, Str::MbIkFkAufIkHint, "", "", 2, [] { (void)gliedAktion([](modell::IkArt a) { ikEinrasten(a, false); }); }},
        {Str::FaFussBoden, Str::FaFussBodenHint, "", "", 2, [] { (void)gliedAktion([](modell::IkArt a) { (void)fussAufBoden(a); }); }},
        {Str::FaZielFlaeche, Str::FaZielFlaecheHint, "", "", 2, [] {
             bool da = false;
             (void)gliedJetzt(&da);
             if (!da) {
                 melden(tr(Str::MbIkGliedWaehlen));
                 return;
             }
             fa().zielKlick = true;
             melden(tr(Str::FaZielFlaecheJetzt));
         }},
        {Str::FaFremdLoesen, Str::FaFremdLoesenHint, "", "", 2, [] { (void)gliedAktion([](modell::IkArt a) { (void)fremdLoesen(a); }); }},
        {Str::FaWurzelZeigen, Str::FaWurzelZeigenHint, "", "", 3, [] { fa().zeigeWurzel = !fa().zeigeWurzel; }},
        {Str::FaGliederZeigen, Str::FaGliederZeigenHint, "", "", 3, [] { fa().zeigeGlieder = !fa().zeigeGlieder; }},
        {Str::FaKurven, Str::FaKurvenHint, "", "", 3, [] { fa().kurvenZeigen = !fa().kurvenZeigen; }},
        {Str::FaBauen, Str::FaBauenHint, "", "", 4, [] { (void)figurAnimBauenFrage(); }},
        // Stufe 6: Auto-Physik (keine Taste in max_hotkeys_de.txt und blender_keys.txt)
        {Str::PhAn, Str::PhAnHint, "", "", 5, [] {
             const std::shared_ptr<FigurDaten> d = daten(fa().figur);
             (void)figurAnimPhysikAn(!(d->da && d->anim.schicht.physik.e.an));
         }},
        {Str::PhZeigen, Str::PhZeigenHint, "", "", 5, [] { fa().zeigePhysik = !fa().zeigePhysik; }},
        {Str::PhAbsprungHier, Str::PhAbsprungHint, "", "", 5, [] { (void)figurAnimPhysikFlugHier(false); }},
        {Str::PhLandungHier, Str::PhLandungHint, "", "", 5, [] { (void)figurAnimPhysikFlugHier(true); }},
        {Str::PhFlugAutomatisch, Str::PhFlugAutomatischHint, "", "", 5, [] {
             (void)figurAnimPhysik([](modell::PhysikEinstellung& e) {
                 e.absprung = -1;
                 e.landung = -1;
             });
         }},
        {Str::PhKettenVorschlag, Str::PhKettenVorschlagHint, "", "", 5, [] { (void)figurAnimPhysikKetten(); }},
    };
}

std::string aktionZeile(const FaAktion& a) {
    const char* taste = modellBlender() ? a.blender : a.max;
    std::string z = tr(a.text);
    if (taste != nullptr && taste[0] != '\0') { z += "  (" + std::string(taste) + ")"; }
    return z;
}

std::string aktionHinweis(const FaAktion& a) {
    const char* taste = modellBlender() ? a.blender : a.max;
    std::string h = tr(a.hinweis);
    if (taste != nullptr && taste[0] != '\0') { h += "\n" + format(Str::FaTaste, taste); }
    return h;
}

void aktionMenue(int gruppe) {
    for (const FaAktion& a : aktionen()) {
        if (a.gruppe != gruppe) { continue; }
        const char* taste = modellBlender() ? a.blender : a.max;
        if (ImGui::MenuItem(tr(a.text), taste != nullptr && taste[0] != '\0' ? taste : nullptr)) { a.tun(); }
        ImGui::SetItemTooltip("%s", aktionHinweis(a).c_str());
    }
}

const Str kGruppen[6] = {Str::FaGruppeFigur, Str::FaGruppePosieren, Str::MbIkGruppe, Str::FaGruppePfade, Str::FaGruppeBauen, Str::PhRollout};

void alleMenues() {
    for (int g = 0; g < 6; ++g) {
        if (ui::menue(tr(kGruppen[g]))) {
            aktionMenue(g);
            ImGui::EndMenu();
        }
    }
}

// --- Aktionssuche (X wie 3ds Max, F3 wie Blender) -------------------------------------------------------------------------

void drawSuche() {
    FigurAnimZustand& z = fa();
    if (z.sucheOeffnen) {
        ImGui::OpenPopup("###fasuche");
        z.sucheOeffnen = false;
    }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 24.0F, fs * 20.0F}, ImGuiCond_Appearing);
    z.sucheSichtbar = ui::popup("###fasuche");
    if (!z.sucheSichtbar) { return; }
    static char suche[64] = "";
    if (ImGui::IsWindowAppearing()) {
        suche[0] = '\0';
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(-1.0F);
    const bool enter = ImGui::InputTextWithHint("##fasuchtext", tr(Str::MbAktionen), suche, sizeof suche, ImGuiInputTextFlags_EnterReturnsTrue);
    const std::string such = klein(suche);
    for (const FaAktion& a : aktionen()) {
        const std::string zeile = std::string(tr(kGruppen[a.gruppe])) + " > " + aktionZeile(a);
        if (!such.empty() && klein(zeile).find(such) == std::string::npos) { continue; }
        if (ImGui::Selectable(zeile.c_str()) || enter) {
            a.tun();
            ImGui::CloseCurrentPopup();
            break;
        }
        ImGui::SetItemTooltip("%s", aktionHinweis(a).c_str());
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// --- Dopesheet und Kurven -------------------------------------------------------------------------------------------------

struct DsZeile {
    int knochen = -1;   // Knochen oder ikWahlCode
    std::string name;
};

std::vector<DsZeile> dsZeilen(const FigurDaten& d, const modell::RigSchicht& s) {
    std::vector<DsZeile> z;
    z.push_back(DsZeile{-2, tr(Str::MbKvZusammenfassung)});
    int gewaehlt = -1;
    {
        const std::vector<FaGriff> v = griffeBei(d, s, bildJetzt(d));
        const int w = gewaehlterGriff(v);
        if (w >= 0 && v[static_cast<std::size_t>(w)].art == 0) { gewaehlt = d.rig.griffe[static_cast<std::size_t>(v[static_cast<std::size_t>(w)].nr)].knochen; }
    }
    static const Str kGlied[4] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};
    for (std::size_t i = 0; i < s.ik.size(); ++i) {
        if (s.ik[i].ziel.empty()) { continue; }
        z.push_back(DsZeile{modell::ikWahlCode(static_cast<int>(i)), "    IK " + std::string(tr(kGlied[static_cast<int>(s.ik[i].art)]))});
    }
    // ENT3B (B14): je Glied die IK/FK-Spur wie CAT - eigene Schluessel, zwischen zwei blendet das Glied ueber
    for (std::size_t i = 0; i < s.ik.size(); ++i) {
        if (s.ik[i].anteil.empty()) { continue; }
        z.push_back(DsZeile{modell::ikAnteilCode(static_cast<int>(i)), "    " + format(Str::E3DsIkFk, tr(kGlied[static_cast<int>(s.ik[i].art)]))});
    }
    if (!s.plattform.empty()) { z.push_back(DsZeile{modell::kPlattformWahl, "    " + std::string(tr(Str::E3GriffWurzel))}); }   // ENT3B (B17)
    for (std::size_t b = 0; b < s.knochen.size() && b < d.basis.bones.size(); ++b) {
        if (s.knochen[b].empty() && static_cast<int>(b) != gewaehlt) { continue; }
        z.push_back(DsZeile{static_cast<int>(b), "    " + d.basis.bones[b].name});
    }
    return z;
}

std::vector<int> bilderDer(const modell::RigSchicht& s, const DsZeile& z, const std::vector<DsZeile>& alle) {
    std::set<int> b;
    const auto von = [&](int k) {
        // Knochen, IK-Ziele, ENT3B: IK/FK-Spur und Plattform
        if (const std::vector<modell::RigSchluessel>* v = modell::schluesselListe(s, k); v != nullptr) {
            for (const modell::RigSchluessel& x : *v) { b.insert(x.bild); }
        }
    };
    if (z.knochen == -2) {
        for (const DsZeile& x : alle) {
            if (x.knochen != -2) { von(x.knochen); }
        }
    } else {
        von(z.knochen);
    }
    return {b.begin(), b.end()};
}

bool dsGewaehlt(int k, int bild) {
    return std::find(fa().dsWahl.begin(), fa().dsWahl.end(), modell::SchluesselWahl{k, bild}) != fa().dsWahl.end();
}

void drawDopesheet(const FigurDaten& d) {
    FigurAnimZustand& z = fa();
    const modell::RigSchicht& s = d.anim.schicht;
    const float fs = ImGui::GetFontSize();
    const float zeileH = ImGui::GetFrameHeight();
    const std::vector<DsZeile> zeilen = dsZeilen(d, s);
    const float namenB = fs * 10.0F;
    const float breite = std::max(ImGui::GetContentRegionAvail().x, fs * 20.0F);
    // hoechstens zehn Zeilen sichtbar, das Rad blaettert (Umschalt+Rad wie im Model-Tab)
    const std::size_t sichtbar = std::min<std::size_t>(zeilen.size(), 10U);
    z.dsErste = std::clamp(z.dsErste, 0, std::max(0, static_cast<int>(zeilen.size() - sichtbar)));
    const float hoehe = zeileH * static_cast<float>(sichtbar + 1U);
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##fads", ImVec2{breite, hoehe}, ImGuiButtonFlags_MouseButtonLeft);
    ImGui::SetItemTooltip("%s", tr(Str::FaDsHint));
    const bool ueber = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 hi{lo.x + breite, lo.y + hoehe};
    dl->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg));
    const float x0 = lo.x + namenB;
    const float x1 = hi.x - fs * 0.5F;
    const int n = std::max(d.basis.numFrames, 1);
    const auto xVon = [&](float b) { return x0 + (x1 - x0) * (b / static_cast<float>(std::max(n - 1, 1))); };
    const auto bildVon = [&](float x) { return static_cast<int>(std::lround(std::clamp((x - x0) / std::max(x1 - x0, 1.0F), 0.0F, 1.0F) * static_cast<float>(n - 1))); };
    // Lineal
    for (int b = 0; b < n; b += std::max(1, n / 10)) {
        dl->AddLine(ImVec2{xVon(static_cast<float>(b)), lo.y}, ImVec2{xVon(static_cast<float>(b)), lo.y + zeileH * 0.4F}, ImGui::GetColorU32(ImGuiCol_TextDisabled));
        char t[16];
        std::snprintf(t, sizeof t, "%d", b);
        dl->AddText(ImVec2{xVon(static_cast<float>(b)) + 2.0F, lo.y}, ImGui::GetColorU32(ImGuiCol_TextDisabled), t);
    }
    const float r = zeileH * 0.3F;
    z.rauten = 0;
    z.ikRauten = 0;
    z.rautenOrte.clear();
    const ImGuiIO& io = ImGui::GetIO();
    for (std::size_t j = 0; j < sichtbar; ++j) {
        const std::size_t i = j + static_cast<std::size_t>(z.dsErste);
        const float ym = lo.y + zeileH * (static_cast<float>(j) + 1.5F);
        if ((j % 2U) == 0U) { dl->AddRectFilled(ImVec2{lo.x, ym - zeileH * 0.5F}, ImVec2{hi.x, ym + zeileH * 0.5F}, ImGui::GetColorU32(ImGuiCol_TableRowBgAlt)); }
        dl->AddText(ImVec2{lo.x + fs * 0.3F, ym - fs * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), zeilen[i].name.c_str());
        const bool ik = modell::ikWahlSpur(zeilen[i].knochen) >= 0;
        for (const int b : bilderDer(s, zeilen[i], zeilen)) {
            float x = xVon(static_cast<float>(b));
            const bool gew = zeilen[i].knochen == -2 ? std::any_of(z.dsWahl.begin(), z.dsWahl.end(), [b](const modell::SchluesselWahl& w) { return w.bild == b; })
                                                     : dsGewaehlt(zeilen[i].knochen, b);
            if (z.dsZieht && gew) { x = xVon(static_cast<float>(b + z.dsVersatz)); }
            const ImU32 innen = gew ? IM_COL32(255, 175, 60, 255) : (ik ? IM_COL32(120, 200, 255, 255) : IM_COL32(225, 225, 225, 255));
            dl->AddQuadFilled(ImVec2{x, ym - r}, ImVec2{x + r, ym}, ImVec2{x, ym + r}, ImVec2{x - r, ym}, innen);
            dl->AddQuad(ImVec2{x, ym - r}, ImVec2{x + r, ym}, ImVec2{x, ym + r}, ImVec2{x - r, ym}, IM_COL32(20, 20, 20, 255), 1.0F);
            ++z.rauten;
            if (ik) { ++z.ikRauten; }
            if (zeilen[i].knochen != -2) { z.rautenOrte.push_back({static_cast<float>(zeilen[i].knochen), static_cast<float>(b), x, ym}); }
        }
    }
    // Zeitzeiger
    const float xz = xVon(static_cast<float>(bildJetzt(d)));
    dl->AddLine(ImVec2{xz, lo.y}, ImVec2{xz, hi.y}, IM_COL32(80, 160, 255, 255), 2.0F);
    // Maus: Klick auf eine Raute waehlt (Umschalt dazu) und zieht sie, Klick ins Leere setzt die Zeit
    if (ImGui::IsItemActivated()) {
        const int zi = static_cast<int>(std::floor((io.MousePos.y - lo.y) / zeileH - 1.0F)) + z.dsErste;
        const int bild = bildVon(io.MousePos.x);
        bool getroffen = false;
        if (zi >= 0 && static_cast<std::size_t>(zi) < zeilen.size() && std::fabs(xVon(static_cast<float>(bild)) - io.MousePos.x) <= r * 1.4F) {
            const DsZeile& zl = zeilen[static_cast<std::size_t>(zi)];
            const std::vector<int> b = bilderDer(s, zl, zeilen);
            if (std::find(b.begin(), b.end(), bild) != b.end()) {
                getroffen = true;
                if (!io.KeyShift && !dsGewaehlt(zl.knochen, bild)) { z.dsWahl.clear(); }
                if (zl.knochen == -2) {
                    for (const DsZeile& x : zeilen) {
                        if (x.knochen != -2 && s.hatSchluessel(x.knochen, bild) && !dsGewaehlt(x.knochen, bild)) { z.dsWahl.push_back({x.knochen, bild}); }
                    }
                } else if (!dsGewaehlt(zl.knochen, bild)) {
                    z.dsWahl.push_back({zl.knochen, bild});
                }
                z.dsZieht = true;
                z.dsMaus = io.MousePos.x;
                z.dsVersatz = 0;
            }
        }
        if (!getroffen) {
            if (!io.KeyShift) { z.dsWahl.clear(); }
            zeitAufBild(d, bild);
        }
    }
    if (z.dsZieht) {
        z.dsVersatz = bildVon(io.MousePos.x) - bildVon(z.dsMaus);
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            z.dsZieht = false;
            if (z.dsVersatz != 0 && !z.dsWahl.empty()) {
                const int dv = z.dsVersatz;
                modell::Wahl neu;
                (void)schichtAendern(Str::FaUndoZeit, [&](modell::FigurAnim& a, const FigurDaten&) { neu = modell::schluesselVerschieben(a.schicht, z.dsWahl, dv); });
                z.dsWahl = neu;
                melden(format(Str::MbKvGeaendert, tr(Str::MbKvVerschieben), static_cast<int>(z.dsWahl.size())));
            }
            z.dsVersatz = 0;
        }
    }
    if (ueber && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) { schluesselLoeschen(); }
    if (ueber && io.MouseWheel != 0.0F) { z.dsErste -= io.MouseWheel > 0.0F ? 1 : -1; }
    if (zeilen.size() > sichtbar) {
        char t[32];
        std::snprintf(t, sizeof t, "%d/%d", z.dsErste + static_cast<int>(sichtbar), static_cast<int>(zeilen.size()));
        dl->AddText(ImVec2{lo.x + fs * 0.3F, lo.y}, ImGui::GetColorU32(ImGuiCol_TextDisabled), t);
    }
}

// Kurven des gewaehlten Knochens: die sieben Kanaele (w x y z, Verschiebung), Schluessel senkrecht ziehbar
void drawKurven(const FigurDaten& d) {
    FigurAnimZustand& z = fa();
    const float fs = ImGui::GetFontSize();
    const modell::RigSchicht& s = d.anim.schicht;
    int k = -1;
    for (const modell::SchluesselWahl& w : z.dsWahl) {
        if (w.knochen >= 0) { k = w.knochen; }
    }
    if (k < 0) {
        const std::vector<FaGriff> v = griffeBei(d, s, bildJetzt(d));
        const int w = gewaehlterGriff(v);
        if (w >= 0 && v[static_cast<std::size_t>(w)].art == 0) { k = d.rig.griffe[static_cast<std::size_t>(v[static_cast<std::size_t>(w)].nr)].knochen; }
    }
    z.kurvenPunkte = 0;
    if (k < 0 || static_cast<std::size_t>(k) >= s.knochen.size() || s.knochen[static_cast<std::size_t>(k)].empty()) {
        ImGui::TextDisabled("%s", tr(Str::FaKurvenLeer));
        return;
    }
    const std::vector<modell::RigSchluessel>& v = s.knochen[static_cast<std::size_t>(k)];
    const float breite = std::max(ImGui::GetContentRegionAvail().x, fs * 20.0F);
    const float hoehe = fs * 9.0F;
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##fakurven", ImVec2{breite, hoehe});
    ImGui::SetItemTooltip("%s", tr(Str::FaKurvenHint));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 hi{lo.x + breite, lo.y + hoehe};
    dl->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg));
    const int n = std::max(d.basis.numFrames, 2);
    // Drehung (w x y z) oben im Bereich -1..1, Verschiebung unten um ihre eigene Spanne
    float tLo = 0.0F;
    float tHi = 0.0F;
    for (const modell::RigSchluessel& x : v) {
        for (const float t : x.t) {
            tLo = std::min(tLo, t);
            tHi = std::max(tHi, t);
        }
    }
    if (tHi - tLo < 1.0F) {
        tHi += 0.5F;
        tLo -= 0.5F;
    }
    const auto xVon = [&](float b) { return lo.x + fs * 0.5F + (breite - fs) * (b / static_cast<float>(n - 1)); };
    const auto yVon = [&](int c, float w) {
        const float mitte = c < 4 ? lo.y + hoehe * 0.27F : lo.y + hoehe * 0.77F;
        const float halb = hoehe * 0.22F;
        return c < 4 ? mitte - w * halb : mitte - ((w - tLo) / (tHi - tLo) * 2.0F - 1.0F) * halb;
    };
    static const ImU32 kF[7] = {IM_COL32(240, 205, 65, 255), IM_COL32(240, 90, 90, 255), IM_COL32(100, 215, 100, 255), IM_COL32(100, 150, 255, 255),
                                IM_COL32(255, 150, 150, 255), IM_COL32(165, 255, 165, 255), IM_COL32(165, 205, 255, 255)};
    for (int c = 0; c < modell::kKanaele; ++c) {
        ImVec2 vor{};
        for (int b = 0; b < n; ++b) {
            const ImVec2 p{xVon(static_cast<float>(b)), yVon(c, modell::kanalAuswerten(v, c, static_cast<float>(b)))};
            if (b > 0) { dl->AddLine(vor, p, kF[c], 1.5F); }
            vor = p;
            ++z.kurvenPunkte;
        }
        for (const modell::RigSchluessel& x : v) { dl->AddCircleFilled(ImVec2{xVon(static_cast<float>(x.bild)), yVon(c, modell::kanalWert(x, c))}, fs * 0.22F, kF[c]); }
    }
    // senkrecht ziehen: der naechste Schluesselpunkt unter der Maus
    static int zugK = -1;
    static int zugI = -1;
    static int zugC = -1;
    static float zugY = 0.0F;
    static float zugW = 0.0F;
    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsItemActivated()) {
        float best = fs * 0.6F;
        zugI = -1;
        for (std::size_t i = 0; i < v.size(); ++i) {
            for (int c = 0; c < modell::kKanaele; ++c) {
                const float dx = xVon(static_cast<float>(v[i].bild)) - io.MousePos.x;
                const float dy = yVon(c, modell::kanalWert(v[i], c)) - io.MousePos.y;
                const float dd = std::sqrt(dx * dx + dy * dy);
                if (dd < best) {
                    best = dd;
                    zugI = static_cast<int>(i);
                    zugC = c;
                }
            }
        }
        zugK = k;
        zugY = io.MousePos.y;
        zugW = zugI >= 0 ? modell::kanalWert(v[static_cast<std::size_t>(zugI)], zugC) : 0.0F;
    }
    if (zugI >= 0 && zugK == k && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        const float proPunkt = zugC < 4 ? 1.0F / (hoehe * 0.22F) : (tHi - tLo) / (hoehe * 0.44F);
        const float neu = zugW - (io.MousePos.y - zugY) * proPunkt;
        const int i = zugI;
        const int c = zugC;
        if (std::fabs(io.MousePos.y - zugY) >= 1.0F) {
            (void)schichtAendern(Str::FaUndoKurve, [&](modell::FigurAnim& a, const FigurDaten&) {
                auto& liste = a.schicht.knochen[static_cast<std::size_t>(zugK)];
                if (static_cast<std::size_t>(i) >= liste.size()) { return; }
                modell::kanalSetzen(liste[static_cast<std::size_t>(i)], c, neu);
                modell::henkelNeu(liste);
            });
        }
        zugI = -1;
    }
}

// --- Bauen ----------------------------------------------------------------------------------------------------------------

std::string ausgabeOrdner() {
    FigurAnimZustand& z = fa();
    if (z.ordner[0] != '\0') { return z.ordner; }
    if (const char* p = std::getenv("BHED_FIGUR_BAU"); p != nullptr && p[0] != '\0') { return p; }
    if (g_app->missionAktiv && !g_app->mission.projektOrdner.empty()) { return g_app->mission.projektOrdner; }
    const std::string d = platform::dokumenteOrdner();
    return d.empty() ? std::string() : d + "\\behaved\\bauen\\" + karte();
}

std::string pfadIn(const std::string& ordner, const std::string& rel) {
    return (std::filesystem::path(::bhed::utf8Pfad(ordner)) / ::bhed::utf8Pfad(rel)).string();
}

std::string cfgKopf(const std::string& kopf, int bilder, std::size_t zeilen) {
    modell::AnimCfg c;
    c.zeilen.push_back(modell::CfgZeile{"// " + kopf + ".gla " + std::to_string(bilder) + " frames; " + std::to_string(zeilen) + " sequences; written by JK Studio\r",
                                        false, {}, false});
    c.zeilen.push_back(modell::CfgZeile{"//\r", false, {}, false});
    c.zeilen.push_back(modell::CfgZeile{"// Format:  enum, targetFrame, frameCount, loopFrame, frameSpeed\r", false, {}, false});
    c.zeilen.push_back(modell::CfgZeile{"//\r", false, {}, false});
    return modell::animCfgSchreiben(c);
}

}  // namespace

bool figurAnimBauen(const std::string& ordner, bool trotzdem) {
    FigurAnimZustand& z = fa();
    z.bauenZeigen = true;
    const modell::Zielspiel zs = zielspiel();
    const std::vector<std::string> figuren = modell::figurenImSkript(g_app->doc.script());
    if (figuren.empty()) {
        melden(tr(Str::FaNichtsZuBauen));
        return false;
    }
    if (ordner.empty()) {
        melden(tr(Str::FaKeinOrdner));
        return false;
    }
    std::vector<std::shared_ptr<FigurDaten>> d;
    std::vector<const modell::FigurAnim*> anims;
    for (const std::string& f : figuren) {
        std::shared_ptr<FigurDaten> x = daten(f);
        if (!x->da) {
            melden(format(Str::FaFigurFehlt, f.c_str(), x->fehler.c_str()));
            return false;
        }
        d.push_back(x);
        anims.push_back(&x->anim);
    }
    const std::string k = karte();
    const std::string kinoName = "models/players/_humanoid_" + k + "/_humanoid_" + k;
    // die vorhandene Karten-GLA aus den Spieldaten (nicht die der Vorschau)
    GlaAnimation archiv;
    std::vector<AnimEntry> archivCfg;
    bool archivDa = false;
    if (z.ziel == 0) {
        std::string bytes;
        if (readFromArchives(kinoName + ".gla", bytes) && readGla(bytes, archiv)) {
            archivDa = true;
            std::string text;
            for (const std::string& c : animCfgCandidates(kinoName)) {
                if (readFromArchives(c, text)) {
                    archivCfg = parseAnimationCfg(text);
                    break;
                }
            }
        }
    }
    std::vector<std::string> belegt;
    for (const AnimEntry& e : archivCfg) { belegt.push_back(e.name); }
    std::string fehler;
    const std::vector<std::string> pl = modell::plaetzeVerteilen(anims, belegt, zs, &fehler);
    if (pl.empty()) {
        melden(format(Str::MbKgKeinePlaetze, modell::grenzen(zs).name));
        return false;
    }
    std::vector<modell::FigurBauEingabe> ein;
    for (const auto& x : d) { ein.push_back(modell::FigurBauEingabe{&x->anim, &x->basis, x->wirk}); }
    std::vector<GlaAnimation> geb;
    std::vector<modell::Platz> plaetze;
    modell::FigurBauBericht br;
    if (!modell::figurenBacken(ein, pl, archivDa ? &archiv : nullptr, archivCfg, zs, geb, plaetze, &br)) {
        melden(format(Str::MbSpFehler, modellFehlerText(br.fehler).c_str()));
        return false;
    }
    GlaAnimation gla;
    std::vector<AnimEntry> cfg;
    modell::ZusammenBericht zb;
    std::string glaRel;
    std::string cfgText;
    std::vector<modell::CfgBefund> befunde;
    const App::Skeleton* sk = d.front()->skelett;
    if (z.ziel == 1) {
        // _humanoid.gla des Skeletts im Mod-Ordner: die GLA bleibt ganz, die Plaetze kommen hinten dazu (Stufe 2 anhaengen)
        if (modellImSpielordner(ordner)) {
            melden(tr(Str::MbKgImSpielordner));
            return false;
        }
        std::vector<modell::Platz> eigene;
        for (const modell::Platz& p : plaetze) {
            if (p.belegt && std::find(pl.begin(), pl.end(), p.name) != pl.end()) { eigene.push_back(p); }
        }
        if (sk == nullptr || !modell::anhaengen(sk->anim, eigene, gla, cfg, &zb)) {
            melden(format(Str::MbSpFehler, modellFehlerText(zb.fehlerArt).c_str()));
            return false;
        }
        glaRel = sk->anim.skeletonName;
        modell::AnimCfg c;
        std::string text;
        for (const std::string& cand : animCfgCandidates(glaRel)) {
            if (readFromArchives(cand, text)) {
                c = modell::animCfgLesen(text);
                break;
            }
        }
        for (const AnimEntry& e : cfg) { c.setzen(e); }
        cfgText = modell::animCfgSchreiben(c);
        befunde = modell::humanoidGlaPruefen(zs);
    } else {
        std::string kopf = kinoName;
        if (z.ziel == 2) {
            const std::string n = z.eigenName[0] != '\0' ? z.eigenName : "_eigen";
            kopf = "models/players/" + n + "/" + n;
        }
        std::vector<modell::Platz> nutzen = plaetze;
        if (z.ziel == 2) {   // eigene GLA: nur die Figuren
            nutzen.erase(std::remove_if(nutzen.begin(), nutzen.end(), [&](const modell::Platz& p) { return std::find(pl.begin(), pl.end(), p.name) == pl.end(); }),
                         nutzen.end());
        }
        if (!modell::zusammenfuehren(nutzen, kopf, gla, cfg, &zb)) {
            melden(format(Str::MbSpFehler, modellFehlerText(zb.fehlerArt).c_str()));
            return false;
        }
        gla.name = kopf;
        glaRel = kopf;
        modell::AnimCfg c = modell::animCfgLesen(cfgKopf(kopf, gla.numFrames, cfg.size()));
        for (const AnimEntry& e : cfg) { c.setzen(e); }
        cfgText = modell::animCfgSchreiben(c);
        befunde = z.ziel == 0 ? modell::kartenGlaPruefen(gla, kopf, zs) : modell::eigeneGlaPruefen(gla, kopf, zs);
        if (z.ziel == 2) {
            for (const modell::CfgBefund& x : modell::animDateienPruefen(2, zs)) { befunde.push_back(x); }
        }
    }
    {
        std::vector<AnimEntry> alle;
        for (const modell::CfgZeile& zz : modell::animCfgLesen(cfgText).zeilen) {
            if (zz.eintrag) { alle.push_back(zz.e); }
        }
        for (const modell::CfgBefund& x : modell::cfgPruefen(alle, gla.numFrames, cfgText.size(), zs)) { befunde.push_back(x); }
    }
    if (!trotzdem && !befunde.empty()) {
        z.befunde.clear();
        for (const modell::CfgBefund& x : befunde) { z.befunde.push_back(modellCfgBefundText(x, zs)); }
        z.frage = true;
        return false;
    }
    // ICARUS: das Skript mit den Bloecken, uebersetzt
    std::string ref = aktiverSkriptPfad();
    for (char& c : ref) {
        if (c == '\\') { c = '/'; }
    }
    if (klein(ref).rfind("scripts/", 0) == 0) { ref = ref.substr(8); }
    if (ref.empty()) { ref = "jkstudio/" + k; }
    std::string ibi;
    {
        std::vector<IbiBlock> bl;
        std::vector<Diag> dg;
        if (!compile(g_app->doc.script(), g_app->db, bl, dg)) {
            melden(format(Str::FaNichtUebersetzt, dg.empty() ? "" : dg.front().message.c_str()));
            return false;
        }
        ibi = writeIbi(bl);
    }
    std::string bytes;
    std::string f;
    const std::string glaDatei = pfadIn(ordner, glaRel + ".gla");
    const std::string cfgDatei = pfadIn(ordner, glaRel.substr(0, glaRel.find_last_of('/')) + "/animation.cfg");
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(glaDatei).parent_path(), ec);
    std::filesystem::create_directories(std::filesystem::path(pfadIn(ordner, "scripts/" + ref)).parent_path(), ec);
    if (!schreibGla(gla, bytes, &f) || !sicherSchreiben(glaDatei, bytes, &f) || !sicherSchreiben(cfgDatei, cfgText, &f) ||
        !sicherSchreiben(pfadIn(ordner, "scripts/" + ref + ".IBI"), ibi, &f) || !sicherSchreiben(pfadIn(ordner, "scripts/" + ref + ".txt"), writeScript(g_app->doc.script()), &f)) {
        melden(format(Str::MbSpFehler, modellFehlerText(f).c_str()));
        return false;
    }
    z.frage = false;
    z.befunde.clear();
    melden(format(Str::FaGebaut, static_cast<int>(d.size()), glaDatei.c_str(), gla.numFrames, ("scripts/" + ref + ".IBI").c_str()));
    diag::info("Figur animieren: gebaut " + glaDatei + " (" + std::to_string(bytes.size()) + " Bytes, " + std::to_string(gla.numFrames) + " Bilder, " +
               std::to_string(cfg.size()) + " Zeilen), " + cfgDatei + ", scripts/" + ref + ".IBI (" + std::to_string(ibi.size()) + " Bytes)");
    return true;
}

namespace {
bool figurAnimBauenFrage() { return figurAnimBauen(ausgabeOrdner(), false); }
}  // namespace

// --- Ansicht: Griffe, Pfade, Maus ------------------------------------------------------------------------------------------

// pfix M8: der Wert eines Reglers oder Zugfelds ueber die Bilder des Ziehens. ImGui setzt im Bild des Loslassens keinen
// neuen Wert mehr, IsItemDeactivatedAfterEdit kommt genau dann - eine jedes Bild aus dem Dokument neu gesetzte Kopie truege
// dort wieder den alten Wert, ein Zugfeld kaeme nie ueber den Mausweg eines Bilds hinaus. Neu aus dem Dokument nur, solange
// das Element (gleiche Kennung wie das folgende Widget) nicht aktiv ist.
template <class T>
T& gehalten(const char* label, T ausDokument) {
    static std::map<ImGuiID, T> werte;
    const ImGuiID id = ImGui::GetID(label);
    T& v = werte[id];
    if (GImGui->ActiveId != id) { v = ausDokument; }
    return v;
}

// --- Stufe 6: Auto-Physik ----------------------------------------------------------------------------------------------------

void drawPhysik(const FigurDaten& d) {
    FigurAnimZustand& z = fa();
    const bool offen = ImGui::CollapsingHeader(tr(Str::PhRollout), d.anim.schicht.physik.e.an ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    ImGui::SetItemTooltip("%s", tr(Str::PhRolloutHint));
    if (!offen) { return; }
    z.physikRolloutBild = ImGui::GetFrameCount();
    // daneben, wenn der Platz reicht (sonst eine neue Zeile - nichts abschneiden, auch nicht in langen Sprachen)
    const auto daneben = [](const char* text, float extra) {
        ImGui::SameLine();
        if (ImGui::GetContentRegionAvail().x < ImGui::CalcTextSize(text).x + extra) { ImGui::NewLine(); }
    };
    const float kasten = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
    const modell::PhysikEinstellung e = d.anim.schicht.physik.e;
    bool an = e.an;
    if (ImGui::Checkbox(tr(Str::PhAn), &an)) { (void)figurAnimPhysikAn(an); }
    ImGui::SetItemTooltip("%s", tr(Str::PhAnHint));
    daneben(tr(Str::PhZeigen), kasten);
    ImGui::Checkbox(tr(Str::PhZeigen), &z.zeigePhysik);
    ImGui::SetItemTooltip("%s", tr(Str::PhZeigenHint));
    const modell::MassenModell mm = modell::massenModell(d.basis, d.rig);
    if (!mm.gueltig()) {
        ImGui::PushTextWrapPos(0.0F);
        cjkTextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram), "%s", format(Str::PhFehler, modellFehlerText(mm.fehler).c_str()).c_str());
        ImGui::PopTextWrapPos();
        return;
    }
    ImGui::BeginDisabled(!e.an);
    float& staerke = gehalten(tr(Str::PhStaerke), e.staerke);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0F);
    ImGui::SliderFloat(tr(Str::PhStaerke), &staerke, 0.0F, 1.0F, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) { (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) { x.staerke = staerke; }); }
    ImGui::SetItemTooltip("%s", tr(Str::PhStaerkeHint));
    bool sprung = e.sprung;
    if (ImGui::Checkbox(tr(Str::PhSprung), &sprung)) { (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) { x.sprung = sprung; }); }
    ImGui::SetItemTooltip("%s", tr(Str::PhSprungHint));
    daneben(tr(Str::PhKontakte), kasten);
    bool kontakte = e.kontakte;
    if (ImGui::Checkbox(tr(Str::PhKontakte), &kontakte)) { (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) { x.kontakte = kontakte; }); }
    ImGui::SetItemTooltip("%s", tr(Str::PhKontakteHint));
    ImGui::PushTextWrapPos(0.0F);
    if (d.physik.fluege.empty()) {
        cjkTextDisabled("%s", tr(Str::PhKeinFlug));
    } else {
        for (const auto& [a, l] : d.physik.fluege) {
            cjkTextUnformatted((format(Str::PhFlug, a, l) + " (" + tr(e.absprung >= 0 ? Str::PhFlugHand : Str::PhFlugAuto) + ")").c_str());
        }
    }
    ImGui::PopTextWrapPos();
    if (ImGui::SmallButton(tr(Str::PhAbsprungHier))) { (void)figurAnimPhysikFlugHier(false); }
    ImGui::SetItemTooltip("%s", tr(Str::PhAbsprungHint));
    daneben(tr(Str::PhLandungHier), ImGui::GetStyle().FramePadding.x * 2.0F);
    if (ImGui::SmallButton(tr(Str::PhLandungHier))) { (void)figurAnimPhysikFlugHier(true); }
    ImGui::SetItemTooltip("%s", tr(Str::PhLandungHint));
    daneben(tr(Str::PhFlugAutomatisch), ImGui::GetStyle().FramePadding.x * 2.0F);
    ImGui::BeginDisabled(e.absprung < 0);
    if (ImGui::SmallButton(tr(Str::PhFlugAutomatisch))) {
        (void)figurAnimPhysik([](modell::PhysikEinstellung& x) {
            x.absprung = -1;
            x.landung = -1;
        });
    }
    ImGui::SetItemTooltip("%s", tr(Str::PhFlugAutomatischHint));
    ImGui::EndDisabled();
    // Schwerkraft der Karte (g_gravity aus der worldspawn), nur zur Anzeige
    float g = g_app->scene.schwerkraft;
    ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0F);
    ImGui::DragFloat(tr(Str::PhSchwerkraft), &g, 1.0F, 0.0F, 4000.0F, "%.0f");
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("%s", tr(Str::PhSchwerkraftHint));
    daneben(tr(Str::PhKontaktAbstand), ImGui::GetFontSize() * 5.0F + ImGui::GetStyle().ItemInnerSpacing.x);
    float& k = gehalten(tr(Str::PhKontaktAbstand), e.kontaktAbstand);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0F);
    ImGui::DragFloat(tr(Str::PhKontaktAbstand), &k, 0.1F, 0.0F, 32.0F, "%.1f");
    if (ImGui::IsItemDeactivatedAfterEdit()) { (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) { x.kontaktAbstand = k; }); }
    ImGui::SetItemTooltip("%s", tr(Str::PhKontaktAbstandHint));
    bool federn = e.federn;
    if (ImGui::Checkbox(tr(Str::PhFedern), &federn)) { (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) { x.federn = federn; }); }
    ImGui::SetItemTooltip("%s", tr(Str::PhFedernHint));
    daneben(tr(Str::PhKettenVorschlag), ImGui::GetStyle().FramePadding.x * 2.0F);
    if (ImGui::SmallButton(tr(Str::PhKettenVorschlag))) { (void)figurAnimPhysikKetten(); }
    ImGui::SetItemTooltip("%s", tr(Str::PhKettenVorschlagHint));
    for (std::size_t i = 0; i < e.ketten.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const modell::PhysikKette c = e.ketten[i];
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(c.knochen >= 0 && static_cast<std::size_t>(c.knochen) < d.basis.bones.size() ? d.basis.bones[static_cast<std::size_t>(c.knochen)].name.c_str()
                                                                                                          : "-");
        ImGui::SameLine();
        float& fq = gehalten(tr(Str::PhFrequenz), c.frequenz);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.5F);
        ImGui::SliderFloat(tr(Str::PhFrequenz), &fq, 0.2F, 10.0F, "%.1f Hz");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten[i].frequenz = fq; }
            });
        }
        ImGui::SetItemTooltip("%s", tr(Str::PhFrequenzHint));
        daneben(tr(Str::PhDaempfung), ImGui::GetFontSize() * 4.5F + ImGui::GetStyle().ItemInnerSpacing.x);
        float& dm = gehalten(tr(Str::PhDaempfung), c.daempfung);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.5F);
        ImGui::SliderFloat(tr(Str::PhDaempfung), &dm, 0.0F, 1.0F, "%.2f");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten[i].daempfung = dm; }
            });
        }
        ImGui::SetItemTooltip("%s", tr(Str::PhDaempfungHint));
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            (void)figurAnimPhysik([&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten.erase(x.ketten.begin() + static_cast<std::ptrdiff_t>(i)); }
            });
        }
        ImGui::SetItemTooltip("%s", tr(Str::PhKetteEntfernen));
        ImGui::PopID();
    }
    ImGui::EndDisabled();
    ImGui::PushTextWrapPos(0.0F);
    if (d.physik.warnungen > 0) {
        float weit = 0.0F;
        for (const modell::PhysikBild& pb : d.physik.bilder) { weit = std::max(weit, pb.abstand); }
        cjkTextColored(ImVec4{1.0F, 0.35F, 0.3F, 1.0F}, "%s", format(Str::PhWarnung, d.physik.warnBild + (modellBlender() ? 1 : 0), d.physik.warnungen, static_cast<double>(weit)).c_str());
    }
    if (e.an && e.sprung && !d.physik.fluege.empty()) {
        cjkTextDisabled("%s", format(Str::PhErgebnis, static_cast<double>(d.physik.parabelAbw), static_cast<double>(d.physik.drehimpulsVorher * 100.0F),
                                         static_cast<double>(d.physik.drehimpulsNachher * 100.0F))
                                      .c_str());
    }
    ImGui::PopTextWrapPos();
}

// Schwerpunkt (jetzt), sein Weg und die Stuetzflaeche in der Karte
void physikUeberlagerung(const FigurDaten& d, const std::vector<BoneMatrix>& welt, int bild, ImDrawList* dl, float px) {
    FigurAnimZustand& z = fa();
    z.physikGezeichnet = 0;
    if (!z.zeigePhysik || !d.anim.schicht.physik.e.an || d.physik.bilder.empty() || welt.size() != d.basis.bones.size()) { return; }
    const modell::MassenModell mm = modell::massenModell(d.basis, d.rig);
    if (!mm.gueltig()) { return; }
    const modell::PhysikBild* pb = bild >= 0 && static_cast<std::size_t>(bild) < d.physik.bilder.size() ? &d.physik.bilder[static_cast<std::size_t>(bild)] : nullptr;
    const bool rot = pb != nullptr && pb->ausserhalb;
    const ImU32 warn = IM_COL32(255, 70, 60, 240);
    ImVec2 vor{};
    bool vorDa = false;
    for (const modell::PhysikBild& b : d.physik.bilder) {
        float k[3];
        inKarte(d.fw, b.spNeu.data(), k);
        ImVec2 q;
        const bool da = kartenPunktAufSchirm(k, q);
        if (da && vorDa) { dl->AddLine(vor, q, b.flug ? IM_COL32(255, 170, 40, 230) : IM_COL32(200, 200, 210, 150), (b.flug ? 2.0F : 1.0F) * px); }
        vor = q;
        vorDa = da;
    }
    ++z.physikGezeichnet;
    if (pb != nullptr && pb->stuetz.size() >= 2) {
        std::vector<ImVec2> pts;
        for (const auto& q2 : pb->stuetz) {
            const float f[3] = {q2[0], q2[1], pb->bodenZ};
            float k[3];
            inKarte(d.fw, f, k);
            ImVec2 q;
            if (kartenPunktAufSchirm(k, q)) { pts.push_back(q); }
        }
        if (pts.size() == pb->stuetz.size()) {
            dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), rot ? warn : IM_COL32(90, 220, 120, 230), ImDrawFlags_Closed, 2.0F * px);
            ++z.physikGezeichnet;
        }
    }
    const modell::Vec3 c = modell::schwerpunkt(d.basis, mm, welt);
    float ck[3];
    inKarte(d.fw, c.data(), ck);
    ImVec2 q;
    if (kartenPunktAufSchirm(ck, q)) {
        const float r = 6.0F * px;
        const ImU32 f = rot ? warn : IM_COL32(250, 230, 80, 240);
        dl->AddCircleFilled(q, r, IM_COL32(20, 20, 24, 200), 20);
        dl->PathArcTo(q, r, 0.0F, 1.5707964F, 8);
        dl->PathLineTo(q);
        dl->PathFillConvex(f);
        dl->PathArcTo(q, r, 3.1415927F, 4.712389F, 8);
        dl->PathLineTo(q);
        dl->PathFillConvex(f);
        dl->AddCircle(q, r, f, 20, 1.5F * px);
        if (rot) { dl->AddText(ImVec2{q.x + r * 1.4F, q.y - r}, warn, tr(Str::PhAusserhalb)); }
        ++z.physikGezeichnet;
    }
}

void figurAnimUeberlagerung() {
    FigurAnimZustand& z = fa();
    z.griffeGezeichnet = 0;
    z.pfadPunkte = 0;
    if (!z.offen || !z.posieren || g_app->playing) { return; }
    const auto t0 = std::chrono::steady_clock::now();
    const std::shared_ptr<FigurDaten> d = daten(z.figur);
    if (!d->da) { return; }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}, true);
    const float px = std::max(1.0F, ImGui::GetFontSize() / 13.0F);
    const modell::RigSchicht& s = z.zieht ? z.schichtZug : d->wirk;
    const int bild = z.zieht ? z.zugBild : bildJetzt(*d);
    std::vector<BoneMatrix> welt;
    const std::vector<FaGriff> v = griffeBei(*d, s, bild, &welt);
    // Wurzelweg: das Becken ueber alle Bilder (der Weg der Figur), Schluessel als groessere Punkte
    if (z.zeigeWurzel) {
        const std::vector<std::array<float, 3>> pfad = z.zieht ? modell::wurzelPfad(d->basis, s, d->becken, d->fw) : d->wurzel;
        ImVec2 vor{};
        bool vorDa = false;
        for (std::size_t i = 0; i < pfad.size(); ++i) {
            ImVec2 q;
            const bool da = kartenPunktAufSchirm(pfad[i].data(), q);
            if (da && vorDa) { dl->AddLine(vor, q, IM_COL32(255, 200, 80, 220), 2.0F * px); }
            const bool schl = d->becken >= 0 && s.hatSchluessel(d->becken, static_cast<int>(i));
            if (da) {
                dl->AddCircleFilled(q, (schl ? 4.5F : 2.2F) * px, schl ? IM_COL32(255, 160, 40, 255) : IM_COL32(255, 220, 130, 220));
                ++z.pfadPunkte;
            }
            vor = q;
            vorDa = da;
        }
    }
    physikUeberlagerung(*d, welt, bild, dl, px);   // Stufe 6
    if (z.zeigeGlieder) {
        for (const modell::IkArt art : {modell::IkArt::ArmLinks, modell::IkArt::ArmRechts, modell::IkArt::BeinLinks, modell::IkArt::BeinRechts}) {
            modell::IkSpur sp;
            if (!modell::ikSpurAnlegen(d->basis, d->rig, art, sp)) { continue; }
            const std::vector<std::array<float, 3>> p = modell::bewegungsPfad(d->basis, s, sp.ende.knochen, 0, d->basis.numFrames - 1);
            ImVec2 vor{};
            bool vorDa = false;
            for (const auto& o : p) {
                float k[3];
                inKarte(d->fw, o.data(), k);
                ImVec2 q;
                const bool da = kartenPunktAufSchirm(k, q);
                if (da && vorDa) { dl->AddLine(vor, q, IM_COL32(170, 140, 255, 200), 1.5F * px); }
                if (da) { ++z.pfadPunkte; }
                vor = q;
                vorDa = da;
            }
        }
    }
    const int w = gewaehlterGriff(v);
    for (std::size_t i = 0; i < v.size(); ++i) {
        ImVec2 q;
        if (!kartenPunktAufSchirm(v[i].ort, q)) { continue; }
        const bool gew = static_cast<int>(i) == w;
        const ImU32 farbe = griffFarbe(v[i].seite, gew);
        const float rr = (gew ? 6.5F : 5.0F) * px;
        if (v[i].art == 0) {
            if (modellBlender()) {
                dl->AddCircle(q, rr, farbe, 16, 2.0F * px);   // wie Blender: Ring
            } else {
                dl->AddRect(ImVec2{q.x - rr, q.y - rr}, ImVec2{q.x + rr, q.y + rr}, farbe, 0.0F, 0, 2.0F * px);   // wie Max: Kasten
            }
        } else if (v[i].art == 1) {
            dl->AddLine(ImVec2{q.x - rr * 1.4F, q.y}, ImVec2{q.x + rr * 1.4F, q.y}, farbe, 2.0F * px);
            dl->AddLine(ImVec2{q.x, q.y - rr * 1.4F}, ImVec2{q.x, q.y + rr * 1.4F}, farbe, 2.0F * px);
            dl->AddCircle(q, rr, farbe, 12, 1.5F * px);
        } else {
            dl->AddQuad(ImVec2{q.x, q.y - rr}, ImVec2{q.x + rr, q.y}, ImVec2{q.x, q.y + rr}, ImVec2{q.x - rr, q.y}, farbe, 1.5F * px);
        }
        ++z.griffeGezeichnet;
    }
    if (w >= 0) {
        ImVec2 q;
        if (kartenPunktAufSchirm(v[static_cast<std::size_t>(w)].ort, q)) {
            const std::string n = griffName(*d, v[static_cast<std::size_t>(w)]);
            dl->AddText(ImVec2{q.x + 9.0F * px + 1.0F, q.y - ImGui::GetFontSize() * 0.5F + 1.0F}, IM_COL32(0, 0, 0, 200), n.c_str());
            dl->AddText(ImVec2{q.x + 9.0F * px, q.y - ImGui::GetFontSize() * 0.5F}, IM_COL32(255, 255, 255, 240), n.c_str());
        }
    }
    dl->PopClipRect();
    z.ueberlagerungMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

void figurAnimMaus() {
    FigurAnimZustand& z = fa();
    if (!z.offen || !z.posieren || g_app->playing) {
        if (z.zieht) { zugEnde(false); }
        return;
    }
    const std::shared_ptr<FigurDaten> d = daten(z.figur);
    if (!d->da) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    const bool imBild = io.MousePos.x >= g_app->kartenAnsicht[0] && io.MousePos.x < g_app->kartenAnsicht[2] && io.MousePos.y >= g_app->kartenAnsicht[1] &&
                        io.MousePos.y < g_app->kartenAnsicht[3] && ImGui::IsWindowHovered();
    // Tasten (nur ueber der Ansicht, nicht mit gedrueckter rechter Taste - dann fliegt die Kamera)
    if (imBild && figurAnimTastenAktiv() && !io.WantTextInput) {
        const bool blender = modellBlender();
        if (z.zieht && z.modal) {
            for (int a = 0; a < 3; ++a) {
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_X + a), false)) { z.achse = z.achse == a ? -1 : a; }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
                zugEnde(true);
                g_app->wegKlickVerbraucht = true;
                return;
            }
        } else if (!blender) {
            if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) { gizmoSetzen(0); }
            if (ImGui::IsKeyPressed(ImGuiKey_W, false)) { gizmoSetzen(1); }
            if (ImGui::IsKeyPressed(ImGuiKey_E, false)) { gizmoSetzen(2); }
            if (ImGui::IsKeyPressed(ImGuiKey_K, false)) { schluesselSetzen(); }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) { schluesselLoeschen(); }
            if (ImGui::IsKeyPressed(ImGuiKey_X, false)) { z.sucheOeffnen = true; }
            if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) { z.achse = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) { z.achse = 1; }
            if (ImGui::IsKeyPressed(ImGuiKey_F7, false)) { z.achse = 2; }
            if (ImGui::IsKeyPressed(ImGuiKey_F8, false)) { z.achse = -1; }
        } else {
            if (ImGui::IsKeyPressed(ImGuiKey_G, false)) { modalStarten(1); }
            if (ImGui::IsKeyPressed(ImGuiKey_R, false)) { modalStarten(2); }
            if (ImGui::IsKeyPressed(ImGuiKey_I, false)) {
                if (io.KeyAlt) {
                    schluesselLoeschen();
                } else {
                    schluesselSetzen();
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F3, false)) { z.sucheOeffnen = true; }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            if (z.zieht) {
                zugEnde(false);
            } else {
                z.zielKlick = false;
            }
        }
    }
    if (z.zieht) {
        zugAnwenden(*d, io.MousePos.x - z.mausStart[0], io.MousePos.y - z.mausStart[1]);
        g_app->mapDirty = true;
        g_app->wegKlickVerbraucht = true;
        if (z.modal) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { zugEnde(true); }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { zugEnde(false); }
        } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            zugEnde(true);
        }
        return;
    }
    if (!imBild) { return; }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).x == 0.0F &&
        ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).y == 0.0F) {
        // Rechtsklick auf einen Griff oder die Figur: das Menue der Animation
        const std::vector<FaGriff> v = griffeBei(*d, d->wirk, bildJetzt(*d));
        for (std::size_t i = 0; i < v.size(); ++i) {
            ImVec2 q;
            if (kartenPunktAufSchirm(v[i].ort, q) && std::hypot(q.x - io.MousePos.x, q.y - io.MousePos.y) < ImGui::GetFontSize() * 2.5F) {
                z.kontextOeffnen = true;
                break;
            }
        }
    }
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { return; }
    if (z.zielKlick) {
        TraceTreffer t;
        bool da = false;
        const modell::IkArt art = gliedJetzt(&da);
        if (da && kartenStrahlUnterMaus(io.MousePos.x, io.MousePos.y, t)) {
            (void)zielAufPunkt(art, t.point);
            std::memcpy(z.letzterTreffer, t.point, sizeof z.letzterTreffer);
            melden(format(Str::FaZielGesetzt, static_cast<double>(t.point[0]), static_cast<double>(t.point[1]), static_cast<double>(t.point[2])));
        }
        z.zielKlick = false;
        g_app->wegKlickVerbraucht = true;
        return;
    }
    const std::vector<FaGriff> v = griffeBei(*d, d->wirk, bildJetzt(*d));
    int best = -1;
    float bestD = ImGui::GetFontSize() * 0.9F;
    for (std::size_t i = 0; i < v.size(); ++i) {
        ImVec2 q;
        if (!kartenPunktAufSchirm(v[i].ort, q)) { continue; }
        const float dd = std::hypot(q.x - io.MousePos.x, q.y - io.MousePos.y);
        if (dd < bestD) {
            bestD = dd;
            best = static_cast<int>(i);
        }
    }
    if (best >= 0) {
        z.griffWahl = best;
        g_app->wegKlickVerbraucht = true;
        if (!modellBlender() && z.gizmo > 0) { zugBeginnen(*d, z.gizmo, v[static_cast<std::size_t>(best)].ort); }
        return;
    }
    // ein Punkt des Wurzelwegs: das Becken an diesem Bild ziehen
    if (z.zeigeWurzel) {
        for (std::size_t i = 0; i < d->wurzel.size(); ++i) {
            ImVec2 q;
            if (kartenPunktAufSchirm(d->wurzel[i].data(), q) && std::hypot(q.x - io.MousePos.x, q.y - io.MousePos.y) < ImGui::GetFontSize() * 0.45F) {
                zeitAufBild(*d, static_cast<int>(i));
                zugBeginnen(*d, 3, d->wurzel[i].data());
                z.zugBild = static_cast<int>(i);
                g_app->wegKlickVerbraucht = true;
                return;
            }
        }
    }
}

// --- Das Fenster ---------------------------------------------------------------------------------------------------------------

namespace {

void abschnittCombo(const char* id, const std::vector<AnimEntry>& abschnitte, const std::string& jetzt, Str hinweis,
                    const std::function<void(const std::string&)>& setzen) {
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 13.0F);
    if (ImGui::BeginCombo(id, jetzt.c_str(), ImGuiComboFlags_HeightLarge)) {
        static char filter[48] = "";
        if (ImGui::IsWindowAppearing()) {
            filter[0] = '\0';
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::InputTextWithHint("##filter", tr(Str::FaFilter), filter, sizeof filter);
        const std::string f = klein(filter);
        int gezeigt = 0;
        for (const AnimEntry& e : abschnitte) {
            if (!f.empty() && klein(e.name).find(f) == std::string::npos) { continue; }
            if (++gezeigt > 400) { break; }
            if (ImGui::Selectable(e.name.c_str(), e.name == jetzt)) { setzen(e.name); }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(hinweis));
}

void drawGliedZeile(const FigurDaten& d, modell::IkArt art) {
    static const Str kGlied[4] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};
    ImGui::PushID(static_cast<int>(art));
    const int sp = modell::ikSpurIndex(d.anim.schicht, art);
    const float anteil = sp >= 0 ? modell::ikAnteilAm(d.anim.schicht.ik[static_cast<std::size_t>(sp)], static_cast<float>(bildJetzt(d))) : 0.0F;
    bool ik = anteil > 0.0F;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(tr(kGlied[static_cast<int>(art)]));
    ImGui::SameLine(ImGui::GetFontSize() * 5.0F);
    if (ImGui::Checkbox(tr(Str::MbIkAn), &ik)) { ikSchalten(art, ik); }
    ImGui::SetItemTooltip("%s", tr(Str::FaIkAnHint));
    ImGui::SameLine();
    if (ImGui::SmallButton(tr(Str::MbIkZielAufFk))) { ikEinrasten(art, true); }
    ImGui::SetItemTooltip("%s", tr(Str::MbIkZielAufFkHint));
    ImGui::SameLine();
    if (ImGui::SmallButton(tr(Str::MbIkFkAufIk))) { ikEinrasten(art, false); }
    ImGui::SetItemTooltip("%s", tr(Str::MbIkFkAufIkHint));
    if (art == modell::IkArt::BeinLinks || art == modell::IkArt::BeinRechts) {
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::FaFussBoden))) { (void)fussAufBoden(art); }
        ImGui::SetItemTooltip("%s", tr(Str::FaFussBodenHint));
    }
    for (const modell::FremdRaum& fr : d.anim.fremd) {
        if (fr.art != art) { continue; }
        ImGui::SameLine();
        ImGui::TextDisabled(tr(Str::FaFremdAn), fr.figur.c_str(), fr.knochen.c_str(), fr.von, fr.bis);
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::FaFremdLoesen))) { (void)fremdLoesen(art); }
        ImGui::SetItemTooltip("%s", tr(Str::FaFremdLoesenHint));
    }
    ImGui::PopID();
}

void drawFremdZeile(const FigurDaten& d) {
    static int glied = 1;
    static std::string andere;
    static std::string knochen = "rhand";
    static const Str kGlied[4] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0F);
    if (ImGui::BeginCombo("##fglied", tr(kGlied[glied]))) {
        for (int i = 0; i < 4; ++i) {
            if (ImGui::Selectable(tr(kGlied[i]), i == glied)) { glied = i; }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0F);
    if (ImGui::BeginCombo("##fandere", andere.c_str())) {
        for (const std::string& n : modell::figurenImSkript(g_app->doc.script())) {
            if (n != d.anim.figur && ImGui::Selectable(n.c_str(), n == andere)) { andere = n; }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(Str::FaAndereHint));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0F);
    if (ImGui::BeginCombo("##fknochen", knochen.c_str(), ImGuiComboFlags_HeightLarge)) {
        const std::shared_ptr<FigurDaten> o = daten(andere);
        if (o->da) {
            for (const GlaBone& b : o->basis.bones) {
                if (ImGui::Selectable(b.name.c_str(), b.name == knochen)) { knochen = b.name; }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::FaFremdSetzen), UiIcon::Folgen, tr(Str::FaFremdSetzenHint), false, !andere.empty())) {
        (void)anFremdenKnochen(static_cast<modell::IkArt>(glied), andere, knochen);
    }
}

void drawBauen() {
    FigurAnimZustand& z = fa();
    const float fs = ImGui::GetFontSize();
    static const Str kZiel[3] = {Str::FaZielKarte, Str::FaZielHumanoid, Str::FaZielEigen};
    static const Str kZielHint[3] = {Str::FaZielKarteHint, Str::FaZielHumanoidHint, Str::FaZielEigenHint};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) { ImGui::SameLine(); }
        ImGui::RadioButton(tr(kZiel[i]), &z.ziel, i);
        ImGui::SetItemTooltip("%s", tr(kZielHint[i]));
    }
    if (z.ziel == 2) {
        ImGui::SetNextItemWidth(fs * 10.0F);
        ImGui::InputTextWithHint("##faeigen", "_eigen", z.eigenName, sizeof z.eigenName);
        ImGui::SetItemTooltip("%s", tr(Str::FaEigenHint));
    }
    std::string o = ausgabeOrdner();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", tr(Str::FaOrdner));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(std::max(fs * 10.0F, ImGui::GetContentRegionAvail().x - fs * 3.0F));
    char puffer[260];
    std::snprintf(puffer, sizeof puffer, "%s", o.c_str());
    if (ImGui::InputText("##faordner", puffer, sizeof puffer)) { std::snprintf(z.ordner, sizeof z.ordner, "%s", puffer); }
    ImGui::SetItemTooltip("%s", tr(Str::FaOrdnerHint));
    ImGui::SameLine();
    if (iconKnopf(tr(Str::FaOrdnerWaehlen), UiIcon::Oeffnen, tr(Str::FaOrdnerHint))) {
        const std::string p = platform::pickFolder(tr(Str::FaOrdner), o);
        if (!p.empty()) { std::snprintf(z.ordner, sizeof z.ordner, "%s", p.c_str()); }
    }
    if (iconTextKnopf(tr(Str::FaBauen), UiIcon::Paket, tr(Str::FaBauenHint))) { (void)figurAnimBauen(ausgabeOrdner(), false); }
    if (z.frage) {
        ImGui::OpenPopup("###fafrage");
        z.frage = false;
    }
    if (ui::modalBegin((std::string(tr(Str::FaBauen)) + "###fafrage").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushTextWrapPos(fs * 30.0F);
        cjkTextUnformatted(tr(Str::FaBefunde));
        for (const std::string& b : z.befunde) { ImGui::BulletText("%s", b.c_str()); }
        ImGui::PopTextWrapPos();
        if (ImGui::Button(tr(Str::MbSpTrotzdem))) {
            (void)figurAnimBauen(ausgabeOrdner(), true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MbKvAbbrechen)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}

}  // namespace

void drawFigurAnimFenster() {
    FigurAnimZustand& z = fa();
    figurAnimNachziehen();
    if (!z.offen) {
        if (z.zieht) { zugEnde(false); }
        return;
    }
    const float fs = ImGui::GetFontSize();
    // rechts am Rand, damit die Figur in der Ansicht frei bleibt
    // Stufe 6: in Bildschirmkoordinaten des Hauptfensters (mit Viewports ist die Lage absolut; DisplaySize allein setzte das Fenster
    // ausserhalb, wenn das Hauptfenster nicht bei 0,0 steht - es wurde ein eigenes Fenster, die Handbuchbilder ph_hb7_* ohne es)
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x - fs * 35.0F, vp->WorkPos.y + fs * 4.0F}, ImGuiCond_FirstUseEver);
    // nicht hoeher als das Hauptfenster (bei grosser Schrift ragte es heraus und wurde ein eigenes Fenster, ph_hb7c: 1440 in 1384)
    ImGui::SetNextWindowSize(ImVec2{std::min(fs * 34.0F, vp->WorkSize.x - fs * 2.0F), std::min(fs * 40.0F, vp->WorkSize.y - fs * 5.0F)}, ImGuiCond_FirstUseEver);
    if (!ui::fensterBegin((std::string(tr(Str::FaTitel)) + "###figuranim").c_str(), &z.offen, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }
    if (ImGui::BeginMenuBar()) {
        alleMenues();
        ImGui::EndMenuBar();
    }
    // Suche oben (wie das Feld "Alle Aktionen suchen" im Model-Tab)
    if (iconTextKnopf(tr(Str::MbAktionen), UiIcon::Suche, tr(Str::FaSucheHint))) { z.sucheOeffnen = true; }
    drawSuche();
    // Figur
    ImGui::AlignTextToFramePadding();
    {
        const ImVec2 q = ImGui::GetCursorScreenPos();
        uiIcon(ImGui::GetWindowDrawList(), UiIcon::Figur, q, fs, ImGui::GetColorU32(uiIconFarbe(UiIcon::Figur)));
        ImGui::Dummy(ImVec2{fs, fs});
        ImGui::SameLine(0.0F, ImGui::GetStyle().ItemInnerSpacing.x);
    }
    ImGui::SetNextItemWidth(fs * 12.0F);
    if (ImGui::BeginCombo("##fafigur", z.figur.c_str())) {
        for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
            if (!figurAnimierbar(static_cast<int>(i))) { continue; }
            const std::string& n = g_app->scene.actors[i].name;
            const bool mitBlock = modell::findeFigurBlock(g_app->doc.script(), n).has_value();
            if (ImGui::Selectable((n + (mitBlock ? "  *" : "")).c_str(), n == z.figur)) {
                z.figur = n;
                z.griffWahl = -1;
                z.dsWahl.clear();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(Str::FaFigurHint));
    const bool block = modell::findeFigurBlock(g_app->doc.script(), z.figur).has_value();
    ImGui::SameLine();
    if (!block) {
        if (iconTextKnopf(tr(Str::FaAnimieren), UiIcon::Animieren, tr(Str::FaAnimierenHint), !z.figur.empty())) { (void)figurAnimStarten(z.figur); }
        ImGui::TextDisabled("%s", tr(Str::FaNochKeinBlock));
        ImGui::End();
        return;
    }
    if (iconKnopf(tr(Str::FaEntfernen), UiIcon::Loeschen, tr(Str::FaEntfernenHint))) {
        (void)figurAnimEntfernen(z.figur);
        ImGui::End();
        return;
    }
    const std::shared_ptr<FigurDaten> d = daten(z.figur);
    const modell::FigurAnim& a = d->anim;
    ImGui::SameLine();
    ImGui::TextDisabled(tr(Str::FaStand), a.platz.c_str(), a.bilder, modell::figurDauerMs(a) / 1000.0, d->startMs / 1000.0);
    if (!d->da) {
        ImGui::TextColored(ImVec4{1.0F, 0.6F, 0.2F, 1.0F}, "%s", d->fehler.c_str());
        ImGui::End();
        return;
    }
    // --- Animation ---
    if (ImGui::CollapsingHeader(tr(Str::FaRolloutAnimation), ImGuiTreeNodeFlags_DefaultOpen)) {
        const std::vector<AnimEntry>& ab = eigeneAbschnitte(*d->skelett);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(tr(Str::FaBasis));
        ImGui::SameLine(fs * 8.0F);
        abschnittCombo("##fabasis", ab, a.basis, Str::FaBasisHint, [](const std::string& n) { (void)figurAnimEinstellen([&](modell::FigurAnim& x) { x.basis = n; }); });
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(tr(Str::FaBilder));
        ImGui::SameLine(fs * 8.0F);
        int& bilder = gehalten("##fabilder", a.bilder);
        ImGui::SetNextItemWidth(fs * 6.0F);
        ImGui::DragInt("##fabilder", &bilder, 0.25F, 1, 65535);
        ImGui::SetItemTooltip("%s", tr(Str::FaBilderHint));
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            const int n = bilder;
            (void)figurAnimEinstellen([n](modell::FigurAnim& x) { x.bilder = n; });
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%.2f s", modell::figurDauerMs(a) / 1000.0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(tr(Str::FaStart));
        ImGui::SameLine(fs * 8.0F);
        static double start = 0.0;
        if (!ImGui::IsItemActive()) { start = a.startMs; }
        ImGui::SetNextItemWidth(fs * 6.0F);
        ImGui::InputDouble("##fastart", &start, 50.0, 500.0, "%.0f ms");
        ImGui::SetItemTooltip("%s", tr(Str::FaStartHint));
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            const double s = std::max(0.0, start);
            (void)figurAnimEinstellen([s](modell::FigurAnim& x) { x.startMs = s; });
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::FaStartZeiger))) {
            const double s = std::max(0.0, std::round(g_app->playMs));
            (void)figurAnimEinstellen([s](modell::FigurAnim& x) { x.startMs = s; });
        }
        ImGui::SetItemTooltip("%s", tr(Str::FaStartZeigerHint));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(tr(Str::FaFolge));
        ImGui::SameLine(fs * 8.0F);
        abschnittCombo("##fafolge", ab, a.folge, Str::FaFolgeHint, [](const std::string& n) { (void)figurAnimEinstellen([&](modell::FigurAnim& x) { x.folge = n; }); });
        bool origin = a.setOrigin;
        if (ImGui::Checkbox(tr(Str::FaSetOrigin), &origin)) {
            (void)figurAnimEinstellen([origin](modell::FigurAnim& x) { x.setOrigin = origin; });
        }
        ImGui::SetItemTooltip("%s", tr(Str::FaSetOriginHint));
        ImGui::SameLine();
        bool kino = a.kino;
        if (ImGui::Checkbox(tr(Str::FaKino), &kino)) {
            (void)figurAnimEinstellen([kino](modell::FigurAnim& x) { x.kino = kino; });
        }
        ImGui::SetItemTooltip("%s", tr(Str::FaKinoHint));
        static char warte[64];
        static char sende[64];
        if (!ImGui::IsAnyItemActive()) {
            std::snprintf(warte, sizeof warte, "%s", a.warteSignal.c_str());
            std::snprintf(sende, sizeof sende, "%s", a.sendeSignal.c_str());
        }
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(tr(Str::FaWarte));
        ImGui::SameLine(fs * 8.0F);
        ImGui::SetNextItemWidth(fs * 8.0F);
        ImGui::InputText("##fawarte", warte, sizeof warte);
        ImGui::SetItemTooltip("%s", tr(Str::FaWarteHint));
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            const std::string s = warte;
            if (!modell::figurNameGueltig(s)) {   // pfix M4
                melden(format(Str::FaNameUngueltig, s.c_str()));
            } else {
                (void)figurAnimEinstellen([&](modell::FigurAnim& x) { x.warteSignal = s; });
            }
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(tr(Str::FaSende));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(fs * 8.0F);
        ImGui::InputText("##fasende", sende, sizeof sende);
        ImGui::SetItemTooltip("%s", tr(Str::FaSendeHint));
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            const std::string s = sende;
            if (!modell::figurNameGueltig(s)) {   // pfix M4
                melden(format(Str::FaNameUngueltig, s.c_str()));
            } else {
                (void)figurAnimEinstellen([&](modell::FigurAnim& x) { x.sendeSignal = s; });
            }
        }
    }
    // --- Posieren ---
    if (ImGui::CollapsingHeader(tr(Str::FaGruppePosieren), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (iconKnopf(tr(Str::FaGriffe), UiIcon::MbRig, tr(Str::FaGriffeHint), z.posieren, true, true)) { z.posieren = !z.posieren; }
        ImGui::SameLine();
        if (!modellBlender()) {
            if (iconKnopf(tr(Str::FaGizmoVerschieben), UiIcon::Verschieben, aktionHinweis(aktionen()[3]).c_str(), z.gizmo == 1, true, true)) { gizmoSetzen(1); }
            ImGui::SameLine();
            if (iconKnopf(tr(Str::FaGizmoDrehen), UiIcon::Drehen, aktionHinweis(aktionen()[4]).c_str(), z.gizmo == 2, true, true)) { gizmoSetzen(2); }
        } else {
            if (iconKnopf(tr(Str::FaGizmoVerschieben), UiIcon::Verschieben, aktionHinweis(aktionen()[3]).c_str())) { modalStarten(1); }
            ImGui::SameLine();
            if (iconKnopf(tr(Str::FaGizmoDrehen), UiIcon::Drehen, aktionHinweis(aktionen()[4]).c_str())) { modalStarten(2); }
        }
        ImGui::SameLine();
        if (iconKnopf(tr(Str::FaSchluesselSetzen), UiIcon::Plus, aktionHinweis(aktionen()[5]).c_str())) { schluesselSetzen(); }
        ImGui::SameLine();
        if (iconKnopf(tr(Str::FaSchluesselLoeschen), UiIcon::Loeschen, aktionHinweis(aktionen()[6]).c_str())) { schluesselLoeschen(); }
        ImGui::SameLine();
        ImGui::Checkbox(tr(Str::MbIkAuto), &z.autoEinrasten);
        ImGui::SetItemTooltip("%s", tr(Str::MbIkAutoHint));
        const std::vector<FaGriff> v = griffeBei(*d, d->wirk, bildJetzt(*d));
        const int w = gewaehlterGriff(v);
        ImGui::TextDisabled(tr(Str::FaGriffJetzt), w >= 0 ? griffName(*d, v[static_cast<std::size_t>(w)]).c_str() : "-", bildJetzt(*d), a.schicht.schluesselZahl(),
                            z.achse < 0 ? "-" : (z.achse == 0 ? "X" : (z.achse == 1 ? "Y" : "Z")));
    }
    // --- IK/FK ---
    if (ImGui::CollapsingHeader(tr(Str::MbIkGruppe), ImGuiTreeNodeFlags_DefaultOpen)) {
        for (const modell::IkArt art : {modell::IkArt::ArmLinks, modell::IkArt::ArmRechts, modell::IkArt::BeinLinks, modell::IkArt::BeinRechts}) { drawGliedZeile(*d, art); }
        if (iconKnopf(tr(Str::FaZielFlaeche), UiIcon::Zielen, tr(Str::FaZielFlaecheHint), z.zielKlick, true, true)) {
            aktionen()[12].tun();
        }
        ImGui::SameLine();
        drawFremdZeile(*d);
    }
    drawPhysik(*d);   // Stufe 6
    // --- Pfade ---
    if (ImGui::CollapsingHeader(tr(Str::FaGruppePfade))) {
        ImGui::Checkbox(tr(Str::FaWurzelZeigen), &z.zeigeWurzel);
        ImGui::SetItemTooltip("%s", tr(Str::FaWurzelZeigenHint));
        ImGui::SameLine();
        ImGui::Checkbox(tr(Str::FaGliederZeigen), &z.zeigeGlieder);
        ImGui::SetItemTooltip("%s", tr(Str::FaGliederZeigenHint));
        if (!d->wurzel.empty()) {
            const auto& p0 = d->wurzel.front();
            const auto& p1 = d->wurzel.back();
            ImGui::TextDisabled(tr(Str::FaWurzelWeg), static_cast<double>(std::hypot(p1[0] - p0[0], p1[1] - p0[1])));
        }
    }
    // --- Dopesheet ---
    if (ImGui::CollapsingHeader(tr(Str::MbKvDsOeffnen), ImGuiTreeNodeFlags_DefaultOpen)) {
        drawDopesheet(*d);
        ImGui::Checkbox(tr(Str::FaKurven), &z.kurvenZeigen);
        ImGui::SetItemTooltip("%s", tr(Str::FaKurvenHint));
        if (z.kurvenZeigen) { drawKurven(*d); }
    }
    // --- Bauen ---
    // Stufe 6: Bauen aus Menue, Suche oder Knopf klappt den Rollout auf und rollt zu ihm (die Meldung steht darunter)
    if (z.bauenZeigen) { ImGui::SetNextItemOpen(true); }
    const bool bauenOffen = ImGui::CollapsingHeader(tr(Str::FaGruppeBauen), ImGuiTreeNodeFlags_DefaultOpen);
    if (z.bauenZeigen) {
        ImGui::SetScrollHereY(0.0F);
        z.bauenZeigen = false;
    }
    if (bauenOffen) { drawBauen(); }
    if (!z.meldung.empty()) {
        ImGui::Separator();
        ImGui::PushTextWrapPos(0.0F);
        cjkTextDisabled("%s", z.meldung.c_str());
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
}

// Rechtsklick in der Ansicht (auf Griff oder Figur): alle Aktionen nach Gruppen
void drawFigurAnimKontext() {
    FigurAnimZustand& z = fa();
    if (z.kontextOeffnen) {
        ImGui::OpenPopup("###fakontext");
        z.kontextOeffnen = false;
    }
    z.kontextSichtbar = ui::popup("###fakontext");
    if (z.kontextSichtbar) {
        ImGui::TextDisabled("%s", z.figur.c_str());
        alleMenues();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }   // wie das Menue der Ansicht: Esc schliesst
        ImGui::EndPopup();
    }
    if (!z.offen) { drawSuche(); }
}

// Fuer das Rechtsklick-Menue einer Entity (gui/app_view3d_bedienung.inc entityMenue)
void figurAnimEntityMenue(std::size_t entity) {
    const int ai = figurFuerEntity(static_cast<int>(entity));
    if (ai < 0 || !figurAnimierbar(ai)) { return; }
    const std::string& n = g_app->scene.actors[static_cast<std::size_t>(ai)].name;
    if (ImGui::MenuItem(tr(Str::FaAnimieren))) { (void)figurAnimStarten(n); }
    ImGui::SetItemTooltip("%s", tr(Str::FaAnimierenHint));
}

// --- Selbsttest ---------------------------------------------------------------------------------------------------------------

std::vector<std::string> figurAnimAktionsTexte() {
    std::vector<std::string> t;
    for (const FaAktion& a : aktionen()) { t.push_back(aktionZeile(a) + "|" + aktionHinweis(a)); }
    return t;
}

bool figurAnimAktion(int i) {
    const std::vector<FaAktion> v = aktionen();
    if (i < 0 || static_cast<std::size_t>(i) >= v.size()) { return false; }
    v[static_cast<std::size_t>(i)].tun();
    return true;
}

int figurAnimGriffSuchen(const std::string& name) {
    const std::shared_ptr<FigurDaten> d = daten(fa().figur);
    if (!d->da) { return -1; }
    const std::vector<FaGriff> v = griffeBei(*d, d->wirk, bildJetzt(*d));
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (griffName(*d, v[i]) == name) { return static_cast<int>(i); }
    }
    return -1;
}

bool figurAnimGriffSchirm(int griff, ImVec2& out) {
    const std::shared_ptr<FigurDaten> d = daten(fa().figur);
    if (!d->da) { return false; }
    const std::vector<FaGriff> v = griffeBei(*d, d->wirk, bildJetzt(*d));
    if (griff < 0 || static_cast<std::size_t>(griff) >= v.size()) { return false; }
    return kartenPunktAufSchirm(v[static_cast<std::size_t>(griff)].ort, out);
}

bool figurAnimWurzelSchirm(int bild, ImVec2& out) {
    const std::shared_ptr<FigurDaten> d = daten(fa().figur);
    if (!d->da || bild < 0 || static_cast<std::size_t>(bild) >= d->wurzel.size()) { return false; }
    return kartenPunktAufSchirm(d->wurzel[static_cast<std::size_t>(bild)].data(), out);
}

// Die Knochen einer Figur in der Karte am Bild, wie die Schicht sie zeigt (ohne Kompression)
bool figurAnimWeltBei(const std::string& figur, int bild, std::vector<std::array<float, 3>>& orte, std::vector<std::string>* namen) {
    const std::shared_ptr<FigurDaten> d = daten(figur);
    if (!d->da) { return false; }
    std::vector<BoneMatrix> w;
    modell::rigWelt(d->basis, d->wirk, bild, bild, 0.0F, w);
    orte.clear();
    if (namen != nullptr) { namen->clear(); }
    for (std::size_t b = 0; b < w.size(); ++b) {
        const float p[3] = {w[b].m[0][3], w[b].m[1][3], w[b].m[2][3]};
        std::array<float, 3> o{};
        inKarte(d->fw, p, o.data());
        orte.push_back(o);
        if (namen != nullptr) { namen->push_back(d->basis.bones[b].name); }
    }
    return true;
}

double figurAnimStartMs(const std::string& figur) { return daten(figur)->startMs; }

// Das IK-Ziel eines Glieds am Bild in der Karte (false: keine Spur oder IK aus)
bool figurAnimIkZiel(const std::string& figur, int glied, int bild, float aus[3]) {
    const std::shared_ptr<FigurDaten> d = daten(figur);
    if (!d->da) { return false; }
    const int sp = modell::ikSpurIndex(d->wirk, static_cast<modell::IkArt>(std::clamp(glied, 0, modell::kIkArten - 1)));
    if (sp < 0) { return false; }
    std::vector<BoneMatrix> w;
    modell::rigWelt(d->basis, d->wirk, bild, bild, 0.0F, w);
    const modell::IkZiel z = modell::ikZielAm(d->wirk.ik[static_cast<std::size_t>(sp)], static_cast<float>(bild), w, modell::plattformAm(d->wirk, static_cast<float>(bild)));
    if (!z.da || z.ik <= 0.0F) { return false; }
    const float o[3] = {z.ziel.m[0][3], z.ziel.m[1][3], z.ziel.m[2][3]};
    inKarte(d->fw, o, aus);
    return true;
}

// Das Becken der Figur am Bild um einen Weg in der Karte verschieben (wie ein Punkt des Wurzelwegs gezogen)
bool figurAnimBeckenWeg(const std::string& figur, int bild, const float wegKarte[3]) {
    fa().figur = figur;
    return schichtAendern(Str::FaUndoPfad, [&](modell::FigurAnim& a, const FigurDaten& d) {
        if (d.becken < 0) { return; }
        std::vector<BoneMatrix> welt;
        modell::rigWelt(d.basis, a.schicht, bild, bild, 0.0F, welt);
        float weg[3];
        richtungInFigur(d.fw, wegKarte, weg);
        modell::rigKnochenVerschieben(d.basis, a.schicht, d.becken, bild, welt, weg);
    });
}

bool figurAnimHalten(int glied, const std::string& andere, const std::string& knochen) {
    return anFremdenKnochen(static_cast<modell::IkArt>(std::clamp(glied, 0, modell::kIkArten - 1)), andere, knochen);
}

}  // namespace bhed::gui
