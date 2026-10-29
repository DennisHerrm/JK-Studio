// app_zeitleiste.cpp - die Zeitleiste im Mission-Tab (zeitleiste1)
//
// Nach dem freigegebenen Entwurf (shank: "genau so hab ich es mir
// gewuenscht"): Uebersicht und Detail, je Darsteller eine Zeile, Kamera als
// Einstellungen mit Namen, Sprache mit Untertitel und Wellenform, Musik,
// Skriptablauf mit beschrifteten Wartezeiten, Ziehen mit Hinweis auf die
// Skriptaenderung. Gezeichnet wird nur, was im Modell steht (bhed/spuren.h,
// gebaut in prepareScene) - es gibt keine zweite Wahrheit mehr.
//
// Transportleiste, Lineal und Abspielen bleiben in drawTimeline
// (app_view3d_bedienung.inc); hier stehen die Zeilen darunter, die
// Navigationsleiste und die Erklaerung "Was passiert hier?" fuer die rechte
// Seitenleiste.
#include "app_internal.h"
#include "tour.h"

#include "bhed/edit.h"
#include "bhed/menue/menue.h"
#include "bhed/spuren.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

using spuren::Art;
using spuren::Clip;
using spuren::Zeile;

// --- Farben aus dem Entwurf --------------------------------------------------
constexpr ImU32 kFahrt = IM_COL32(91, 156, 240, 255);
constexpr ImU32 kSchwenk = IM_COL32(227, 162, 75, 255);
constexpr ImU32 kZoom = IM_COL32(159, 180, 217, 255);
constexpr ImU32 kBlende = IM_COL32(201, 204, 211, 255);
constexpr ImU32 kSonstKamera = IM_COL32(170, 150, 230, 255);
constexpr ImU32 kSprache = IM_COL32(111, 207, 138, 255);
constexpr ImU32 kMusik = IM_COL32(184, 146, 242, 255);
constexpr ImU32 kLaufen = IM_COL32(233, 193, 90, 255);
constexpr ImU32 kAnim = IM_COL32(78, 195, 192, 255);
constexpr ImU32 kWarnung = IM_COL32(242, 102, 92, 255);
constexpr ImU32 kKopf = IM_COL32(242, 169, 59, 255);   // Abspielkopf
constexpr ImU32 kClipText = IM_COL32(13, 15, 19, 255);
constexpr ImU32 kRand = IM_COL32(0, 0, 0, 90);
constexpr ImU32 kWartenA = IM_COL32(59, 66, 82, 255);
constexpr ImU32 kWartenB = IM_COL32(51, 57, 72, 255);
constexpr ImU32 kWartenRand = IM_COL32(74, 82, 99, 255);
constexpr ImU32 kHalten = IM_COL32(107, 115, 133, 255);
constexpr ImU32 kUnsichtbar = IM_COL32(43, 47, 56, 255);
constexpr ImU32 kMarkeSetzen = IM_COL32(167, 176, 194, 255);
constexpr ImU32 kMarkeUse = IM_COL32(240, 209, 106, 255);
constexpr ImU32 kMarkeSonst = IM_COL32(124, 133, 151, 255);

// Zeilen fuer die Erklaerung (App::ZlAuswahl::zeile).
enum ZeilenTyp : int { ZKamera = 0, ZSprache, ZMusik, ZSkript, ZDarsteller, ZFilm };

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string zeitText(double ms) {
    const double m = std::max(0.0, ms);
    const int min = static_cast<int>(m / 60000.0);
    const double sek = (m - min * 60000.0) / 1000.0;
    char b[32];
    std::snprintf(b, sizeof(b), "%d:%04.1f", min, sek);
    return b;
}

std::string fmt(const char* f, ...) {
    char b[1200];
    va_list a;
    va_start(a, f);
    std::vsnprintf(b, sizeof(b), f, a);
    va_end(a);
    return b;
}

ImU32 heller(ImU32 c) {
    const auto k = [&](int shift) {
        return static_cast<ImU32>(std::min(255, static_cast<int>((c >> shift) & 0xFFU) + 30)) << shift;
    };
    return k(IM_COL32_R_SHIFT) | k(IM_COL32_G_SHIFT) | k(IM_COL32_B_SHIFT) | (c & IM_COL32_A_MASK);
}

ImU32 farbeVon(Art a) {
    switch (a) {
        case Art::Einstellung:
        case Art::Fahrt: return kFahrt;
        case Art::Schwenk: return kSchwenk;
        case Art::Zoom: return kZoom;
        case Art::Blende: return kBlende;
        case Art::Rollen:
        case Art::Wackeln:
        case Art::Folgen:
        case Art::KameraBahn: return kSonstKamera;
        case Art::Satz: return kSprache;
        case Art::Musik: return kMusik;
        case Art::Laufen:
        case Art::Gleiten:
        case Art::Drehen:
        case Art::Roff: return kLaufen;
        case Art::Animation: return kAnim;
        default: return kMarkeSonst;
    }
}

bool istKamera(Art a) {
    return a == Art::Einstellung || a == Art::Fahrt || a == Art::Schwenk || a == Art::Zoom || a == Art::Rollen ||
           a == Art::Blende || a == Art::Wackeln || a == Art::Folgen || a == Art::KameraBahn || a == Art::KameraAn ||
           a == Art::KameraAus;
}

// Passt der Block zur Suche? Gesucht wird in Beschriftung, Befehl, Datei und
// Untertitel - "luke", "cam1b", "SET_ANIM" finden, was man erwartet.
bool passt(const Clip& c, const std::string& zusatz = {}) {
    const std::string s = klein(g_app->zlSuche);
    if (s.empty()) { return true; }
    for (const std::string* t : {&c.text, &c.befehl, &c.datei, &c.untertitel, &c.wer, &zusatz}) {
        if (klein(*t).find(s) != std::string::npos) { return true; }
    }
    return false;
}

// Der Inhalt eines Befehls (Name und Argumente) - fuer "durch Ziehen geaendert".
std::string knotenText(const Node& n) {
    std::string t = n.name;
    for (const Arg& a : n.args) { t += "|" + a.text; }
    return t;
}

bool gleicherClip(const Clip& a, const Clip& b) {
    return a.art == b.art && std::fabs(a.startMs - b.startMs) < 0.5 && a.pfad == b.pfad && a.wer == b.wer &&
           a.text == b.text;
}

// Durch Ziehen geaendert? Solange der geaenderte Knoten nicht wieder seinen
// alten Text hat (Strg+Z bringt ihn zurueck - dann ist die Marke weg).
bool geaendert(const Clip& c) {
    if (c.pfad.empty() || !c.fremd.empty()) { return false; }
    const Node* n = nodeAt(g_app->doc.script(), c.pfad);
    if (n == nullptr) { return false; }
    const auto it = g_app->zlGeaendert.find(n->kennung);
    if (it == g_app->zlGeaendert.end()) { return false; }
    std::vector<const std::vector<Node>*> offen{&g_app->doc.script().nodes};
    while (!offen.empty()) {
        const std::vector<Node>* ns = offen.back();
        offen.pop_back();
        for (const Node& k : *ns) {
            if (k.kennung == it->second.first) { return knotenText(k) != it->second.second; }
            if (!k.children.empty()) { offen.push_back(&k.children); }
        }
    }
    return false;
}

// --- Der Rahmen dieses Bildes ------------------------------------------------
struct Rahmen {
    ImDrawList* dl = nullptr;
    float x0 = 0.0F;      // Zeitfeld links (Bildschirm)
    float w = 1.0F;       // Breite des Zeitfelds
    float nameX = 0.0F;   // Namensspalte links
    float nameW = 1.0F;
    double start = 0.0;   // sichtbarer Ausschnitt
    double spanne = 1.0;
    double ende = 1.0;    // Ende des Ablaufs (rote Linie)
    float k = 1.0F;       // Massstab: Schriftgroesse / 14 (der Entwurf ist fuer 14 px)
    int zeilenNr = 0;
    bool hover = false;   // steht die Maus ueber dem Spurenfenster?
    [[nodiscard]] float xAt(double ms) const { return x0 + static_cast<float>((ms - start) / spanne) * w; }
    [[nodiscard]] double msJePx() const { return spanne / std::max(1.0, static_cast<double>(w)); }
};

// Eine Beschriftung, die erst am Ende der Zeile gesetzt wird - nur, wenn sie
// bis zur naechsten Marke Platz hat (keine Ueberlappung).
struct MarkenText {
    float x;
    float y;
    std::string text;
};

// --- Zeilen -------------------------------------------------------------------

// Beginnt eine Zeile. Gibt das Zeitfeld zurueck; pfeil: -1 keiner, 0 zu, 1 offen.
// Liefert ueber *pfeilGeklickt, ob der Pfeil angeklickt wurde.
ImRect zeileBeginnen(Rahmen& r, const char* id, const std::string& name, const std::string& unter, float hoehe,
                     bool kopf = false, int pfeil = -1, bool* pfeilGeklickt = nullptr, bool warnUnter = false,
                     float einzug = 0.0F, UiIcon icon = UiIcon::Liste, bool mitIcon = false) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float fs = ImGui::GetFontSize();
    g_app->tlZeilenY.emplace_back(id, p.y + hoehe * 0.5F);
    const ImU32 grundName = ImGui::GetColorU32(kopf ? ImGuiCol_TableHeaderBg : ImGuiCol_ChildBg);
    const ImU32 grundFeld = ImGui::GetColorU32((r.zeilenNr % 2) != 0 ? ImGuiCol_TableRowBgAlt : ImGuiCol_TableRowBg);
    r.dl->AddRectFilled(ImVec2{r.x0, p.y}, ImVec2{r.x0 + r.w, p.y + hoehe}, ImGui::GetColorU32(ImGuiCol_FrameBg));
    r.dl->AddRectFilled(ImVec2{r.x0, p.y}, ImVec2{r.x0 + r.w, p.y + hoehe}, grundFeld);
    r.dl->AddRectFilled(ImVec2{r.nameX, p.y}, ImVec2{r.nameX + r.nameW, p.y + hoehe}, grundName);
    r.dl->AddLine(ImVec2{r.nameX, p.y + hoehe - 0.5F}, ImVec2{r.x0 + r.w, p.y + hoehe - 0.5F},
                  ImGui::GetColorU32(ImGuiCol_Separator, 0.45F));
    r.dl->AddLine(ImVec2{r.x0 - 1.0F, p.y}, ImVec2{r.x0 - 1.0F, p.y + hoehe}, ImGui::GetColorU32(ImGuiCol_Separator));
    float x = r.nameX + 8.0F * r.k + einzug;
    const float my = p.y + hoehe * 0.5F;
    if (pfeil >= 0) {
        const float h = std::floor(fs * 0.34F);
        const ImVec2 m{std::floor(x + h * 0.5F), std::floor(my)};
        const ImU32 pf = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        if (pfeil == 1) {
            r.dl->AddTriangleFilled(ImVec2{m.x - h * 0.6F, m.y - h * 0.35F}, ImVec2{m.x + h * 0.6F, m.y - h * 0.35F},
                                    ImVec2{m.x, m.y + h * 0.45F}, pf);
        } else {
            r.dl->AddTriangleFilled(ImVec2{m.x - h * 0.35F, m.y - h * 0.6F}, ImVec2{m.x - h * 0.35F, m.y + h * 0.6F},
                                    ImVec2{m.x + h * 0.45F, m.y}, pf);
        }
        const ImVec2 a{r.nameX, p.y};
        const ImVec2 b{x + h + 10.0F * r.k, p.y + hoehe};
        if (pfeilGeklickt != nullptr && r.hover && ImGui::IsMouseHoveringRect(a, b) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            *pfeilGeklickt = true;
        }
        x += h + 7.0F * r.k;
    }
    if (mitIcon) {
        const float g = std::floor(fs * 0.9F);
        uiIcon(r.dl, icon, ImVec2{std::floor(x), std::floor(my - g * 0.5F)}, g, ImGui::GetColorU32(uiIconFarbe(icon)));
        x += g + 5.0F * r.k;
    }
    const ImVec2 na{r.nameX, p.y};
    const ImVec2 nb{r.nameX + r.nameW - 2.0F, p.y + hoehe};
    r.dl->PushClipRect(na, nb, true);
    const ImVec2 ts = ImGui::CalcTextSize(name.c_str());
    r.dl->AddText(ImVec2{x, my - ts.y * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), name.c_str());
    if (!unter.empty()) {
        const float sx = x + ts.x + 6.0F * r.k;
        r.dl->AddText(nullptr, fs * 0.85F, ImVec2{sx, my - fs * 0.85F * 0.5F},
                      warnUnter ? kWarnung : ImGui::GetColorU32(ImGuiCol_TextDisabled), unter.c_str());
    }
    r.dl->PopClipRect();
    // Abgeschnittener Name? Dann im Tooltip ganz.
    if (r.hover && ImGui::IsMouseHoveringRect(na, nb) && x + ts.x > nb.x) {
        ImGui::SetTooltip("%s%s%s", name.c_str(), unter.empty() ? "" : "\n", unter.c_str());
    }
    r.dl->PushClipRect(ImVec2{r.x0, p.y}, ImVec2{r.x0 + r.w, p.y + hoehe}, true);
    return ImRect{ImVec2{r.x0, p.y}, ImVec2{r.x0 + r.w, p.y + hoehe}};
}

void zeileBeenden(Rahmen& r, const ImRect& f, std::vector<MarkenText>* texte = nullptr) {
    if (texte != nullptr && !texte->empty()) {
        std::stable_sort(texte->begin(), texte->end(), [](const MarkenText& a, const MarkenText& b) { return a.x < b.x; });
        for (std::size_t i = 0; i < texte->size(); ++i) {
            const MarkenText& t = (*texte)[i];
            const float grenze = i + 1 < texte->size() ? (*texte)[i + 1].x - 14.0F * r.k : f.Max.x;
            const ImVec2 ts = ImGui::CalcTextSize(t.text.c_str());
            if (t.x + ts.x > grenze || t.x < f.Min.x) { continue; }
            // Nichts ueberdecken: keinen Block, keine Marke, keine andere Beschriftung.
            const ImRect lr{ImVec2{t.x, t.y - ts.y * 0.5F}, ImVec2{t.x + ts.x, t.y + ts.y * 0.5F}};
            bool frei = true;
            for (const App::ZlBlock& b : g_app->zlBloecke) { frei = frei && !b.r.Overlaps(lr); }
            for (const App::ZlText& b : g_app->zlBeschriftungen) { frei = frei && !b.r.Overlaps(lr); }
            if (!frei) { continue; }
            r.dl->AddText(ImVec2{t.x, t.y - ts.y * 0.5F}, ImGui::GetColorU32(ImGuiCol_TextDisabled), t.text.c_str());
            g_app->zlBeschriftungen.push_back({ImRect{ImVec2{t.x, t.y - ts.y * 0.5F}, ImVec2{t.x + ts.x, t.y + ts.y * 0.5F}}, f.Min.y});
        }
    }
    const float ex = r.xAt(r.ende);
    r.dl->AddLine(ImVec2{ex, f.Min.y}, ImVec2{ex, f.Max.y}, IM_COL32(230, 80, 80, 110), 1.0F);
    const float px = r.xAt(zeitleisteZeigerMs());
    r.dl->AddLine(ImVec2{px, f.Min.y}, ImVec2{px, f.Max.y}, kKopf, 2.0F);
    r.dl->PopClipRect();
    ImGui::Dummy(ImVec2{r.nameW + r.w, f.Max.y - f.Min.y});
    ++r.zeilenNr;
}

// --- Bloecke ------------------------------------------------------------------

struct BlockArgs {
    const char* zeile = "";
    ZeilenTyp typ = ZSkript;
    std::string anzeige;     // Darsteller (lesbar) fuer die Erklaerung
    std::string text;
    std::string zweite;      // zweite Zeile (Sprache: Untertitel)
    ImU32 farbe = kMarkeSonst;
    float y0 = 0.0F;
    float y1 = 0.0F;
    bool schraffiert = false;   // wait
    bool waehlbar = true;
    bool ziehbar = true;        // in dieser Zeile darf gezogen werden
    float minBreite = 4.0F;
    const char* nichtZiehen = nullptr;   // Hinweis, warum nicht (Tooltip)
};

// Gibt an, ob die Maus ueber dem Block steht.
bool block(Rahmen& r, const Clip& c, const BlockArgs& a) {
    const bool kante = g_app->zlKante && !c.pfad.empty() && g_app->zlKantePfad == c.pfad && c.laengeZiehbar;
    const bool schiebt = g_app->tlZiehAktiv && !c.pfad.empty() && g_app->tlZiehPfad == c.pfad && c.verschiebbar;
    const double s = c.startMs + (schiebt ? g_app->tlZiehDelta : 0.0);
    const double e = kante ? g_app->zlKanteEndMs : c.endMs + (schiebt ? g_app->tlZiehDelta : 0.0);
    float x0 = r.xAt(s);
    float x1 = std::max(r.xAt(e), x0 + a.minBreite * r.k);
    if (x1 < r.x0 - 2.0F || x0 > r.x0 + r.w + 2.0F) { return false; }
    const ImVec2 pa{x0, a.y0};
    const ImVec2 pb{x1, a.y1};
    const bool ueber = r.hover && ImGui::IsMouseHoveringRect(pa, ImVec2{pb.x + 2.0F, pb.y});
    const float rund = 4.0F * r.k;
    if (a.schraffiert) {
        r.dl->AddRectFilled(pa, pb, ueber ? heller(kWartenA) : kWartenA, 3.0F * r.k);
        r.dl->PushClipRect(pa, pb, true);
        const float st = 6.0F * r.k;
        for (float x = pa.x - (pb.y - pa.y); x < pb.x; x += st * 2.0F) {
            r.dl->AddQuadFilled(ImVec2{x, pb.y}, ImVec2{x + st, pb.y}, ImVec2{x + st + (pb.y - pa.y), pa.y},
                                ImVec2{x + (pb.y - pa.y), pa.y}, kWartenB);
        }
        r.dl->PopClipRect();
        r.dl->AddRect(pa, pb, kWartenRand, 3.0F * r.k);
    } else {
        r.dl->AddRectFilled(pa, pb, ueber || kante || schiebt ? heller(a.farbe) : a.farbe, rund);
        r.dl->AddRect(pa, pb, kRand, rund);
    }
    const bool istGeaendert = geaendert(c);
    if (g_app->zlAuswahl.da && gleicherClip(g_app->zlAuswahl.clip, c)) {
        r.dl->AddRect(ImVec2{pa.x - 1.0F, pa.y - 1.0F}, ImVec2{pb.x + 1.0F, pb.y + 1.0F}, IM_COL32(255, 255, 255, 255),
                      rund, 0, 2.0F);
    } else if (istGeaendert) {
        // Gestrichelt umrandet, wie im Entwurf.
        const float d = 5.0F * r.k;
        const ImU32 wf = IM_COL32(255, 255, 255, 230);
        for (float x = pa.x + 3.0F; x < pb.x - 3.0F; x += d * 2.0F) {
            r.dl->AddLine(ImVec2{x, pa.y + 2.5F}, ImVec2{std::min(x + d, pb.x - 3.0F), pa.y + 2.5F}, wf, 2.0F);
            r.dl->AddLine(ImVec2{x, pb.y - 2.5F}, ImVec2{std::min(x + d, pb.x - 3.0F), pb.y - 2.5F}, wf, 2.0F);
        }
    }
    // Beschriftung: am sichtbaren linken Rand festhalten, im Block beschnitten.
    bool abgeschnitten = false;
    const float sichtbarB = std::min(pb.x, r.x0 + r.w) - std::max(pa.x, r.x0);
    if (!a.text.empty() && sichtbarB >= ImGui::CalcTextSize("MM").x) {
        const float tx = std::max(pa.x, r.x0) + 6.0F * r.k;
        const ImU32 tf = a.schraffiert ? ImGui::GetColorU32(ImGuiCol_Text) : kClipText;
        const ImVec2 ts = ImGui::CalcTextSize(a.text.c_str());
        r.dl->PushClipRect(ImVec2{std::max(pa.x, r.x0), pa.y}, ImVec2{std::min(pb.x, r.x0 + r.w) - 2.0F, pb.y}, true);
        if (a.zweite.empty()) {
            r.dl->AddText(ImVec2{tx, (pa.y + pb.y - ts.y) * 0.5F}, tf, a.text.c_str());
        } else {
            const float fs2 = ImGui::GetFontSize() * 0.82F;
            r.dl->AddText(ImVec2{tx, pa.y + 2.0F * r.k}, tf, a.text.c_str());
            r.dl->AddText(nullptr, fs2, ImVec2{tx, pa.y + 3.0F * r.k + ts.y}, (tf & 0x00FFFFFFU) | 0xD0000000U,
                          a.zweite.c_str());
            abgeschnitten = tx + ImGui::CalcTextSize(a.zweite.c_str()).x * 0.82F > std::min(pb.x, r.x0 + r.w);
        }
        r.dl->PopClipRect();
        abgeschnitten = abgeschnitten || tx + ts.x > std::min(pb.x, r.x0 + r.w) - 2.0F;
        if (!abgeschnitten) {
            g_app->zlBeschriftungen.push_back({ImRect{ImVec2{tx, (pa.y + pb.y - ts.y) * 0.5F},
                                                      ImVec2{tx + ts.x, (pa.y + pb.y + ts.y) * 0.5F}},
                                               a.y0});
        }
    }
    const bool beschriftet = !a.text.empty() && !abgeschnitten && sichtbarB >= ImGui::CalcTextSize("MM").x;
    g_app->zlBloecke.push_back({a.zeile, ImRect{pa, pb}, c.art, c.pfad, a.text, beschriftet, istGeaendert});
    if (c.art == Art::Warten) { ++g_app->tlWaitBloecke; }
    if (!ueber || g_app->zlKante || g_app->tlZiehAktiv) { return ueber; }
    // --- Ziehen und Klicken: nur vormerken, der zuletzt gezeichnete gewinnt ---
    const float greif = std::max(4.0F, 8.0F * r.k);
    const bool aufKante = a.ziehbar && c.laengeZiehbar && ImGui::GetIO().MousePos.x > pb.x - greif;
    const bool schiebbar = a.ziehbar && c.verschiebbar && c.art != Art::Warten;
    if (aufKante) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    } else if (schiebbar) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(a.text.empty() ? c.befehl.c_str() : a.text.c_str());
    if (!a.zweite.empty()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0F);
        ImGui::TextDisabled("%s", a.zweite.c_str());
        ImGui::PopTextWrapPos();
    }
    if (!c.befehl.empty() && c.befehl != a.text) { ImGui::TextDisabled("%s", c.befehl.c_str()); }
    ImGui::TextDisabled("%s - %s", zeitText(c.startMs).c_str(), zeitText(c.endMs).c_str());
    if (c.springt) { ImGui::TextColored(ImVec4{0.95F, 0.4F, 0.36F, 1.0F}, "%s", tr(Str::ZlSpringt)); }
    if (a.nichtZiehen != nullptr) {
        ImGui::TextDisabled("%s", a.nichtZiehen);
    } else if (a.ziehbar && (schiebbar || c.laengeZiehbar)) {
        ImGui::TextDisabled("%s", tr(schiebbar && c.laengeZiehbar ? Str::ZlZiehBloecke
                                     : schiebbar                  ? Str::ZlZiehNurSchieben
                                                                  : Str::ZlZiehNurKante));
    }
    if (c.art == Art::Laufen) { ImGui::TextDisabled("%s", tr(Str::ZlLaufKante)); }
    if (istGeaendert) { ImGui::TextDisabled("%s", tr(Str::ZlGeaendert)); }
    ImGui::EndTooltip();
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { return true; }
    if (aufKante) {
        App::TlKandidat& k = g_app->zlGriffKand;
        k = App::TlKandidat{};
        k.da = true;
        k.pfad = c.pfad;
        k.startMs = c.startMs;
        k.endMs = c.endMs;
        g_app->zlGriffClip = c;
    } else if (a.waehlbar) {
        App::TlKandidat& k = g_app->zlKlickKand;
        k = App::TlKandidat{};
        k.da = true;
        k.pfad = c.verschiebbar && a.ziehbar ? c.pfad : Path{};
        k.startMs = c.startMs;
        k.kamera = a.typ == ZKamera;
        k.figur = -1;
    }
    if (a.waehlbar) {
        g_app->zlKandidat.da = true;
        g_app->zlKandidat.zeile = a.typ;
        g_app->zlKandidat.clip = c;
        g_app->zlKandidat.anzeige = a.anzeige;
    }
    return true;
}

// Eine Marke (Befehl ohne Dauer): Kreis, Quadrat oder Dreieck mit Zeichen.
enum class Form { Kreis, Quadrat, Dreieck };

void marke(Rahmen& r, const Clip& c, const char* zeile, ZeilenTyp typ, const std::string& anzeige, float ym, Form form,
           ImU32 farbe, int zeichen, std::vector<MarkenText>& texte, const std::string& label) {
    const float x = r.xAt(c.startMs);
    if (x < r.x0 - 12.0F * r.k || x > r.x0 + r.w + 12.0F * r.k) { return; }
    const float h = 10.0F * r.k;
    const ImVec2 a{x - h, ym - h};
    const ImVec2 b{x + h, ym + h};
    const bool ueber = r.hover && ImGui::IsMouseHoveringRect(a, b);
    const ImU32 f = ueber ? heller(farbe) : farbe;
    switch (form) {
        case Form::Kreis: r.dl->AddCircleFilled(ImVec2{x, ym}, h, f); r.dl->AddCircle(ImVec2{x, ym}, h, kRand); break;
        case Form::Quadrat: r.dl->AddRectFilled(a, b, f, 4.0F * r.k); r.dl->AddRect(a, b, kRand, 4.0F * r.k); break;
        case Form::Dreieck:
            r.dl->AddTriangleFilled(ImVec2{x, a.y}, ImVec2{b.x, b.y}, ImVec2{a.x, b.y}, f);
            break;
    }
    // Zeichen darin: 1 Ton, 2 Kamera an (Dreieck), 3 Kamera aus (Quadrat), 4 Ende (Pfeil), 5 Warnung
    const ImU32 zf = kClipText;
    const float g = h * 1.2F;
    switch (zeichen) {
        case 1: uiIcon(r.dl, UiIcon::Ton, ImVec2{std::floor(x - g * 0.5F), std::floor(ym - g * 0.5F)}, g, zf); break;
        case 2:
            r.dl->AddTriangleFilled(ImVec2{x - h * 0.35F, ym - h * 0.5F}, ImVec2{x - h * 0.35F, ym + h * 0.5F},
                                    ImVec2{x + h * 0.5F, ym}, zf);
            break;
        case 3: r.dl->AddRectFilled(ImVec2{x - h * 0.4F, ym - h * 0.4F}, ImVec2{x + h * 0.4F, ym + h * 0.4F}, zf); break;
        case 4:
            r.dl->AddLine(ImVec2{x - h * 0.55F, ym}, ImVec2{x + h * 0.35F, ym}, IM_COL32(255, 255, 255, 255), 2.0F);
            r.dl->AddTriangleFilled(ImVec2{x + h * 0.1F, ym - h * 0.4F}, ImVec2{x + h * 0.1F, ym + h * 0.4F},
                                    ImVec2{x + h * 0.55F, ym}, IM_COL32(255, 255, 255, 255));
            r.dl->AddLine(ImVec2{x + h * 0.6F, ym - h * 0.5F}, ImVec2{x + h * 0.6F, ym + h * 0.5F}, IM_COL32(255, 255, 255, 255),
                          2.0F);
            break;
        case 5: uiIcon(r.dl, UiIcon::Warnung, ImVec2{std::floor(x - g * 0.5F), std::floor(ym - g * 0.5F)}, g, zf); break;
        default: break;
    }
    if (g_app->zlAuswahl.da && gleicherClip(g_app->zlAuswahl.clip, c)) {
        r.dl->AddRect(ImVec2{a.x - 2.0F, a.y - 2.0F}, ImVec2{b.x + 2.0F, b.y + 2.0F}, IM_COL32(255, 255, 255, 255), 4.0F * r.k, 0,
                      2.0F);
    }
    if (!label.empty()) { texte.push_back({x + h + 3.0F * r.k, ym, label}); }
    g_app->zlBloecke.push_back({zeile, ImRect{a, b}, c.art, c.pfad, label, false, false});
    if (!ueber || g_app->zlKante || g_app->tlZiehAktiv) { return; }
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(label.empty() ? c.befehl.c_str() : label.c_str());
    if (!c.befehl.empty() && c.befehl != label) { ImGui::TextDisabled("%s", c.befehl.c_str()); }
    ImGui::TextDisabled("%s", zeitText(c.startMs).c_str());
    if (c.art == Art::NichtNachgebildet) { ImGui::TextColored(ImVec4{0.95F, 0.4F, 0.36F, 1.0F}, "%s", tr(Str::ZlNichtNach)); }
    if (c.verschiebbar) { ImGui::TextDisabled("%s", tr(Str::ZlZiehNurSchieben)); }
    ImGui::EndTooltip();
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        App::TlKandidat& k = g_app->zlKlickKand;
        k = App::TlKandidat{};
        k.da = true;
        k.pfad = c.verschiebbar ? c.pfad : Path{};
        k.startMs = c.startMs;
        k.kamera = typ == ZKamera;
        g_app->zlKandidat.da = true;
        g_app->zlKandidat.zeile = typ;
        g_app->zlKandidat.clip = c;
        g_app->zlKandidat.anzeige = anzeige;
    }
}

// Die Warnmarke "springt zurueck" an einem Kamerablock (Detail).
void springtChip(Rahmen& r, const Clip& c, float y) {
    if (!c.springt) { return; }
    const float x = r.xAt(c.startMs);
    if (x < r.x0 || x > r.x0 + r.w) { return; }
    const char* t = tr(Str::ZlSpringt);
    const ImVec2 ts = ImGui::CalcTextSize(t);
    const float h = 22.0F * r.k;
    const ImVec2 a{x, y};
    const ImVec2 b{x + ts.x + 12.0F * r.k, y + h};
    r.dl->AddRectFilled(a, b, kWarnung, h * 0.5F);
    r.dl->AddText(ImVec2{a.x + 6.0F * r.k, y + (h - ts.y) * 0.5F}, IM_COL32(255, 255, 255, 255), t);
    ++g_app->tlUeberlappungen;
}

// Der Name einer Kamerazeile waehlt die Kamera (ihre Bahn erscheint in der Ansicht).
void kameraNameKlick(const Rahmen& r, const ImRect& f) {
    if (!r.hover || !ImGui::IsMouseHoveringRect(ImVec2{r.nameX, f.Min.y}, ImVec2{r.x0, f.Max.y})) { return; }
    ImGui::SetTooltip("%s", tr(Str::TlCamRowHint));
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        g_app->cameraSelected = true;
        g_app->selectedActor = -1;
        g_app->pickedEntity = -1;
        if (g_app->selectedKey < 0) { g_app->selectedKey = schluesselFuerZeit(g_app->playMs); }
        g_app->mapDirty = true;
    }
}

// Unterbahnen: Hoehe einer Zeile und Lage eines Blocks.
constexpr float kBahnH = 26.0F;

float bahnenHoehe(const Zeile& z, float bahnH) { return 8.0F + static_cast<float>(std::max(1, z.bahnen)) * bahnH; }

// --- Der Text einer Marke / eines Blocks ---------------------------------------
std::string setzText(const Clip& c) {
    // "SET_ANIM_BOTH, BOTH_WAVE1" -> "ANIM_BOTH BOTH_WAVE1" (kurz, wie im Entwurf)
    std::string t = c.text;
    if (t.rfind("SET_", 0) == 0) { t = t.substr(4); }
    const std::size_t k = t.find(',');
    if (k != std::string::npos) {
        std::string w = t.substr(k + 1);
        while (!w.empty() && w.front() == ' ') { w.erase(w.begin()); }
        if (w.size() > 18) { w = w.substr(0, 18) + "…"; }
        t = t.substr(0, k) + " " + w;
    }
    return t;
}

std::string warteText(const Clip& c) {
    if (c.warteArt == spuren::WarteArt::Signal) { return fmt(tr(Str::ZlWartetAuf), c.wartetAuf.c_str()); }
    if (c.warteArt == spuren::WarteArt::Aufgabe) { return fmt(tr(Str::ZlWartetBis), c.wartetAuf.c_str()); }
    return fmt(tr(Str::ZlWartet), (c.endMs - c.startMs) / 1000.0);
}

// Eine Spur des Ablaufs (Skript oder Befehle eines Darstellers).
void ablaufZeile(Rahmen& r, const ImRect& f, const Zeile& z, const char* id, ZeilenTyp typ, const std::string& anzeige,
                 std::vector<MarkenText>& texte, bool detail) {
    const float ym = (f.Min.y + f.Max.y) * 0.5F;
    for (const Clip& c : z.clips) {
        if (!passt(c)) { continue; }
        if (g_app->zlProbleme && c.art != Art::NichtNachgebildet && c.art != Art::Kartenwechsel) { continue; }
        switch (c.art) {
            case Art::Warten: {
                if (c.endMs - c.startMs < 60.0) { break; }
                BlockArgs a;
                a.zeile = id;
                a.typ = typ;
                a.anzeige = anzeige;
                // Ueberlappt eine zweite Wartezeit (ein anderes Skript derselben
                // Entity), liegt sie unten in der Zeile, ohne Text.
                a.text = c.bahn == 0 ? warteText(c) : std::string();
                a.y0 = c.bahn == 0 ? f.Min.y + 4.0F * r.k : (f.Min.y + f.Max.y) * 0.5F + 2.0F * r.k;
                a.y1 = f.Max.y - 4.0F * r.k;
                a.schraffiert = true;
                (void)block(r, c, a);
                break;
            }
            case Art::Satz:
                marke(r, c, id, typ, anzeige, ym, Form::Kreis, kSprache, 1, texte, detail ? fileName(c.datei) : std::string());
                break;
            case Art::Benutzen:
                marke(r, c, id, typ, anzeige, ym, Form::Dreieck, kMarkeUse, 0, texte, c.text);
                break;
            case Art::Setzen:
                marke(r, c, id, typ, anzeige, ym, Form::Quadrat, kMarkeSetzen, 0, texte, setzText(c));
                break;
            case Art::Kartenwechsel:
                marke(r, c, id, typ, anzeige, ym, Form::Quadrat, kWarnung, 4, texte, fmt(tr(Str::ZlKartenwechsel), c.text.c_str()));
                break;
            case Art::NichtNachgebildet:
                marke(r, c, id, typ, anzeige, ym, Form::Kreis, kWarnung, 5, texte, c.text);
                break;
            case Art::Signal:
                marke(r, c, id, typ, anzeige, ym, Form::Kreis, kMarkeSonst, 0, texte, "signal " + c.text);
                break;
            default:
                // affect ist nur der Kleber, der Bloecke an Darsteller gibt - in
                // der Uebersicht ohne Marke (wie im Entwurf).
                if (!detail && c.text == "affect") { break; }
                if (c.endMs - c.startMs > 60.0) {
                    BlockArgs a;
                    a.zeile = id;
                    a.typ = typ;
                    a.anzeige = anzeige;
                    a.text = c.text;
                    a.farbe = kMarkeSonst;
                    a.y0 = f.Min.y + 4.0F * r.k;
                    a.y1 = f.Max.y - 4.0F * r.k;
                    (void)block(r, c, a);
                } else {
                    marke(r, c, id, typ, anzeige, ym, Form::Kreis, kMarkeSonst, 0, texte, c.text);
                }
                break;
        }
    }
}

bool hatProblem(const Zeile& z) {
    return std::any_of(z.clips.begin(), z.clips.end(), [](const Clip& c) {
        return c.springt || c.problem || c.art == Art::NichtNachgebildet || c.art == Art::Kartenwechsel;
    });
}

// Eine Kamerazeile der Detailansicht (mit Unterbahnen und Warnmarken).
void kameraZeile(Rahmen& r, const Zeile& z, const char* id, const std::string& name) {
    if (z.clips.empty()) { return; }
    const bool ov = std::any_of(z.clips.begin(), z.clips.end(), [](const Clip& c) { return c.springt; });
    if (g_app->zlProbleme && !ov) { return; }
    const float bh = kBahnH * r.k;
    const ImRect f = zeileBeginnen(r, id, name, ov ? tr(Str::ZlUeberlappt) : "", bahnenHoehe(z, kBahnH) * r.k, false, -1, nullptr, ov);
    kameraNameKlick(r, f);
    std::vector<MarkenText> texte;
    for (const Clip& c : z.clips) {
        if (!passt(c)) { continue; }
        if (g_app->zlProbleme && !c.springt) { continue; }
        const float y = f.Min.y + 4.0F * r.k + static_cast<float>(c.bahn) * bh;
        if (c.art == Art::KameraAn || c.art == Art::KameraAus) {
            marke(r, c, id, ZKamera, {}, y + bh * 0.5F - 2.0F * r.k, Form::Quadrat, kFahrt, c.art == Art::KameraAn ? 2 : 3, texte,
                  tr(c.art == Art::KameraAn ? Str::ZlKameraAn : Str::ZlKameraAus));
            continue;
        }
        BlockArgs a;
        a.zeile = id;
        a.typ = ZKamera;
        a.farbe = farbeVon(c.art);
        a.y0 = y;
        a.y1 = y + bh - 4.0F * r.k;
        a.minBreite = 4.0F;
        switch (c.art) {
            case Art::Fahrt: a.text = c.text; break;
            case Art::Schwenk: a.text = c.text.empty() ? tr(Str::ZlSchwenk) : c.text; break;
            case Art::Zoom: a.text = c.text + "°"; break;
            case Art::Blende: a.text = tr(Str::ZlBlende); break;
            case Art::Folgen: a.text = c.text; break;
            default: a.text = name; break;
        }
        (void)block(r, c, a);
        springtChip(r, c, y);
    }
    zeileBeenden(r, f, &texte);
}

// Eine Kurve (Sichtfeld, Blende): linear zwischen den Knicken, wie das Spiel.
void kurvenZeile(Rahmen& r, const std::vector<spuren::Knick>& k, const char* id, const std::string& name, const char* unter,
                 float lo, float hi, ImU32 farbe) {
    if (k.size() < 2 || g_app->zlProbleme) { return; }
    const ImRect f = zeileBeginnen(r, id, name, unter, 34.0F * r.k);
    const float h = f.Max.y - f.Min.y;
    const auto y = [&](float v) {
        const float t = (std::clamp(v, lo, hi) - lo) / (hi - lo);
        return f.Max.y - 3.0F * r.k - t * (h - 8.0F * r.k);
    };
    std::vector<ImVec2> pkt;
    pkt.reserve(k.size() + 2);
    for (const spuren::Knick& p : k) { pkt.emplace_back(r.xAt(p.ms), y(p.wert)); }
    // Bis an den rechten Rand mit dem letzten Wert.
    pkt.emplace_back(std::max(pkt.back().x, r.x0 + r.w), pkt.back().y);
    r.dl->AddPolyline(pkt.data(), static_cast<int>(pkt.size()), farbe, ImDrawFlags_None, 1.5F * r.k);
    if (r.hover && ImGui::IsMouseHoveringRect(f.Min, f.Max)) {
        const double ms = r.start + (static_cast<double>(ImGui::GetIO().MousePos.x) - r.x0) * r.msJePx();
        ImGui::SetTooltip("%s  %s: %.1f", zeitText(ms).c_str(), name.c_str(), static_cast<double>(spuren::kurveBei(k, ms)));
    }
    zeileBeenden(r, f);
}

// Die Wellenform einer Klangdatei: Spitze je 50 ms, einmal je Datei.
const std::vector<float>& wellenform(const std::string& datei) {
    auto it = g_app->zlWellen.find(datei);
    if (it != g_app->zlWellen.end()) { return it->second; }
    std::vector<float> w;
    const sound::Sound* s = soundFor(datei);
    if (s != nullptr && s->sampleRate > 0 && s->channels > 0) {
        const std::size_t fenster = static_cast<std::size_t>(s->sampleRate / 20) * static_cast<std::size_t>(s->channels);
        for (std::size_t a = 0; fenster > 0 && a < s->samples.size() && w.size() < 1200; a += fenster) {
            int spitze = 0;
            for (std::size_t i = a; i < std::min(s->samples.size(), a + fenster); ++i) {
                spitze = std::max(spitze, std::abs(static_cast<int>(s->samples[i])));
            }
            w.push_back(static_cast<float>(spitze) / 32768.0F);
        }
    }
    return g_app->zlWellen.emplace(datei, std::move(w)).first->second;
}

void spracheZeile(Rahmen& r) {
    const Zeile& z = g_app->spuren.sprache;
    if (z.clips.empty() || g_app->zlProbleme) { return; }
    const float bh = 44.0F * r.k;
    const ImRect f = zeileBeginnen(r, "sprache", tr(Str::ZlSprache), tr(Str::ZlUntertitel), 8.0F * r.k + static_cast<float>(z.bahnen) * bh);
    for (const Clip& c : z.clips) {
        const spuren::Darsteller* d = nullptr;
        for (const spuren::Darsteller& x : g_app->spuren.darsteller) {
            if (klein(x.name) == klein(c.wer)) { d = &x; }
        }
        const std::string wer = d != nullptr ? d->anzeige : c.wer;
        if (!passt(c, wer)) { continue; }
        BlockArgs a;
        a.zeile = "sprache";
        a.typ = ZSprache;
        a.anzeige = wer;
        a.text = wer.empty() ? fileName(c.datei) : wer;
        a.zweite = c.untertitel.empty() ? fileName(c.datei) : c.untertitel;
        a.farbe = kSprache;
        a.y0 = f.Min.y + 4.0F * r.k + static_cast<float>(c.bahn) * bh;
        a.y1 = a.y0 + bh - 4.0F * r.k;
        // Ohne bekannte Laenge mindestens 300 ms breit (Entwurf).
        Clip breit = c;
        if (breit.endMs - breit.startMs < 300.0) { breit.endMs = breit.startMs + 300.0; }
        (void)block(r, breit, a);
        // Wellenform unten im Block, halbdurchsichtig.
        const std::vector<float>& wf = wellenform(c.datei);
        if (!wf.empty()) {
            const float x0 = r.xAt(c.startMs);
            const float yb = a.y1 - 1.0F;
            const float hw = 12.0F * r.k;
            for (std::size_t i = 0; i < wf.size(); ++i) {
                const float xa = r.xAt(c.startMs + static_cast<double>(i) * 50.0);
                const float xb = std::max(xa + 1.0F, r.xAt(c.startMs + static_cast<double>(i + 1) * 50.0) - 0.5F);
                if (xb < r.x0 || xa > r.x0 + r.w || xa < x0) { continue; }
                r.dl->AddRectFilled(ImVec2{xa, yb - wf[i] * hw}, ImVec2{xb, yb}, IM_COL32(13, 15, 19, 110));
            }
        }
    }
    zeileBeenden(r, f);
}

void musikZeile(Rahmen& r) {
    const Zeile& z = g_app->spuren.musik;
    if (z.clips.empty() || g_app->zlProbleme) { return; }
    const ImRect f = zeileBeginnen(r, "musik", tr(Str::ZlMusik), "", 30.0F * r.k);
    for (const Clip& c : z.clips) {
        if (!passt(c)) { continue; }
        BlockArgs a;
        a.zeile = "musik";
        a.typ = ZMusik;
        a.text = fileName(c.datei);
        a.farbe = kMusik;
        a.y0 = f.Min.y + 4.0F * r.k;
        a.y1 = f.Max.y - 4.0F * r.k;
        a.ziehbar = false;
        (void)block(r, c, a);
    }
    zeileBeenden(r, f);
}

// Die Zeiten der Kamerabilder: alle 2 s bis zum Ende der Kamerafahrt.
std::vector<double> filmZeiten() {
    std::vector<double> z;
    const double ende = std::min(std::max(g_app->camTrack.durationMs, 1.0), g_app->spuren.dauerMs);
    for (double t = 0.0; t <= ende + 0.5 && z.size() < 600; t += 2000.0) { z.push_back(t); }
    return z;
}

// Der Durchgang, der die Kamerabilder aufnimmt: je Bild der Ansicht ein
// Kamerabild (drawMapView zeichnet dann nur dieses, klein in Satz 4 - keine
// doppelte Arbeit, keine laengeren Bilder). Nur, wenn niemand etwas tut;
// greift der Anwender ein, bricht er ab und laeuft spaeter neu.
void filmDurchgang() {
    const bool ruhig = !g_app->playing && !ImGui::IsAnyMouseDown() && !g_app->zlKante && g_app->tlZiehPfad.empty() &&
                       g_app->camTrackValid && !g_app->camTrack.segments.empty();
    const std::vector<double> zeiten = filmZeiten();
    if (g_app->zlFilmPass >= 0) {
        if (!ruhig || g_app->zlFilmStand != g_app->zlModellStand) {
            // Abbrechen; laeuft spaeter von vorn (zlFilmStand 0).
            g_app->playMs = g_app->zlFilmVorherMs;
            g_app->zlFilmPass = -1;
            g_app->zlFilmWunsch = -1;
            g_app->zlFilmStand = 0;
            g_app->mapDirty = true;
            return;
        }
        if (static_cast<std::size_t>(g_app->zlFilmPass) < zeiten.size()) {
            g_app->playMs = zeiten[static_cast<std::size_t>(g_app->zlFilmPass)];
            g_app->zlFilmWunsch = g_app->zlFilmPass;
            ++g_app->zlFilmPass;
        } else {
            g_app->playMs = g_app->zlFilmVorherMs;
            g_app->zlFilmPass = -1;
            g_app->zlFilmWunsch = -1;
            g_app->zlFilm.resize(zeiten.size());
        }
        g_app->mapDirty = true;
        return;
    }
    if (ruhig && g_app->zlFilmStand != g_app->zlModellStand) {
        g_app->zlFilmStand = g_app->zlModellStand;
        g_app->zlFilmVorherMs = g_app->playMs;
        g_app->zlFilmPass = 0;
        g_app->zlFilm.resize(zeiten.size());
    }
}

void filmZeile(Rahmen& r) {
    if (g_app->zlProbleme || g_app->camTrack.segments.empty()) { return; }
    const ImRect f = zeileBeginnen(r, "film", tr(Str::ZlFilm), tr(Str::ZlFilmUnter), 50.0F * r.k);
    filmDurchgang();
    const std::vector<double> zeiten = filmZeiten();
    const float hoch = 44.0F * r.k;
    const float breitMax = 64.0F * r.k * 1.6F;
    for (std::size_t i = 0; i < zeiten.size() && i < g_app->zlFilm.size(); ++i) {
        App::ZlFilmBild& b = g_app->zlFilm[i];
        if (b.textur != nullptr && b.stand != render::texturStand()) { b.textur = nullptr; }
        const float x0 = r.xAt(zeiten[i]);
        const float breit = std::clamp(r.xAt(zeiten[i] + 2000.0) - x0 - 2.0F, 20.0F * r.k, breitMax);
        if (x0 + breit < r.x0 || x0 > r.x0 + r.w) { continue; }
        const ImVec2 a{x0, f.Min.y + 3.0F * r.k};
        const ImVec2 e{x0 + breit, a.y + hoch};
        if (b.textur != nullptr && b.w > 0 && b.h > 0) {
            // Wie "object-fit: cover": die Mitte des Bildes im Seitenverhaeltnis des Kastens.
            const float bildA = static_cast<float>(b.w) / static_cast<float>(b.h);
            const float kastenA = breit / hoch;
            ImVec2 uv0{0.0F, 0.0F};
            ImVec2 uv1{1.0F, 1.0F};
            if (bildA > kastenA) {
                const float u = kastenA / bildA;
                uv0.x = (1.0F - u) * 0.5F;
                uv1.x = uv0.x + u;
            } else {
                const float v = bildA / kastenA;
                uv0.y = (1.0F - v) * 0.5F;
                uv1.y = uv0.y + v;
            }
            r.dl->AddImage(reinterpret_cast<ImTextureID>(b.textur), a, e, uv0, uv1);
            // Blende und Kinobalken wie im Spiel (die zeichnet die Ansicht obenauf).
            const CameraState st = g_app->camTrack.at(zeiten[i]);
            const float schwarz = std::clamp(st.fadeColor[3], 0.0F, 1.0F);
            if (schwarz > 0.001F) {
                r.dl->AddRectFilled(a, e, ImGui::GetColorU32(ImVec4{st.fadeColor[0], st.fadeColor[1], st.fadeColor[2], schwarz}));
            }
            if (st.bars > 0.001F) {
                const float bh = hoch * 0.11F * std::min(1.0F, st.bars);
                r.dl->AddRectFilled(a, ImVec2{e.x, a.y + bh}, IM_COL32(0, 0, 0, 255));
                r.dl->AddRectFilled(ImVec2{a.x, e.y - bh}, e, IM_COL32(0, 0, 0, 255));
            }
        } else {
            r.dl->AddRectFilled(a, e, ImGui::GetColorU32(ImGuiCol_FrameBgHovered), 3.0F * r.k);
        }
        r.dl->AddRect(a, e, IM_COL32(0, 0, 0, 255), 3.0F * r.k);
        const bool ueber = r.hover && ImGui::IsMouseHoveringRect(a, e);
        Clip c;
        c.art = Art::Befehl;
        c.startMs = zeiten[i];
        c.endMs = zeiten[i];
        if (g_app->zlAuswahl.da && g_app->zlAuswahl.zeile == ZFilm && std::fabs(g_app->zlAuswahl.clip.startMs - zeiten[i]) < 0.5) {
            r.dl->AddRect(ImVec2{a.x - 1.0F, a.y - 1.0F}, ImVec2{e.x + 1.0F, e.y + 1.0F}, IM_COL32(255, 255, 255, 255), 3.0F * r.k, 0, 2.0F);
        }
        g_app->zlBloecke.push_back({"film", ImRect{a, e}, Art::Befehl, Path{}, std::string(), false, false});
        if (!ueber || g_app->zlKante || g_app->tlZiehAktiv) { continue; }
        ImGui::SetTooltip("%s", fmt(tr(Str::ZlFilmTitel), zeitText(zeiten[i]).c_str()).c_str());
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            App::TlKandidat& k = g_app->zlKlickKand;
            k = App::TlKandidat{};
            k.da = true;
            k.startMs = zeiten[i];
            g_app->zlKandidat.da = true;
            g_app->zlKandidat.zeile = ZFilm;
            g_app->zlKandidat.clip = c;
            g_app->zlKandidat.anzeige.clear();
        }
    }
    zeileBeenden(r, f);
}

// Tut der Darsteller im sichtbaren Ausschnitt etwas?
bool tutWas(const spuren::Darsteller& d, double v0, double v1) {
    for (const Clip& c : d.laufen.clips) {
        if (c.endMs > v0 && c.startMs < v1) { return true; }
    }
    for (const Clip& c : d.befehle.clips) {
        if (c.startMs >= v0 && c.startMs <= v1 && c.art != Art::Befehl) { return true; }
    }
    for (const Clip& c : d.animation.clips) {
        if (c.startMs > v0 && c.startMs < v1 && c.startMs > 0.0) { return true; }
    }
    return false;
}

bool darstellerPasst(const spuren::Darsteller& d) {
    if (g_app->zlSuche[0] == '\0') { return true; }
    const std::string s = klein(g_app->zlSuche);
    if (klein(d.anzeige).find(s) != std::string::npos || klein(d.name).find(s) != std::string::npos) { return true; }
    for (const Zeile* z : {&d.befehle, &d.animation, &d.laufen}) {
        for (const Clip& c : z->clips) {
            if (passt(c)) { return true; }
        }
    }
    return false;
}

void darstellerZeilen(Rahmen& r, const spuren::Darsteller& d, int figIdx, int auswahlFigur) {
    const bool detail = g_app->zlDetail;
    const bool figAn = figIdx >= 0 && (figIdx == g_app->selectedActor || figIdx == auswahlFigur);
    if (figAn) {
        g_app->tlAuswahlZeile = d.name;
        if (g_app->tlRollFigur != figIdx) {
            g_app->tlOffen.insert(d.name);   // neu gewaehlt: aufklappen
            ImGui::SetScrollHereY(0.4F);
            g_app->tlRollFigur = figIdx;
        }
    }
    const bool offen = detail || g_app->tlOffen.count(d.name) != 0U;
    bool pfeil = false;
    const ImVec2 kp = ImGui::GetCursorScreenPos();
    const ImRect f = zeileBeginnen(r, d.name.c_str(), d.anzeige, d.anzeige == d.name ? std::string() : d.name, 40.0F * r.k, false,
                                   detail ? -1 : (offen ? 1 : 0), &pfeil, false, 0.0F, UiIcon::Figur, true);
    if (figAn) {
        r.dl->AddRectFilled(ImVec2{r.nameX, kp.y}, ImVec2{r.nameX + 3.0F * r.k, f.Max.y}, kKopf);
    }
    // Den NAMEN anklicken waehlt die Figur (ihre Bahnen erscheinen in der Ansicht).
    if (pfeil) {
        if (offen) { g_app->tlOffen.erase(d.name); } else { g_app->tlOffen.insert(d.name); }
    } else if (figIdx >= 0 && r.hover && ImGui::IsMouseHoveringRect(ImVec2{r.nameX, kp.y}, ImVec2{r.x0, f.Max.y})) {
        ImGui::SetTooltip("%s\n%s", tr(Str::TlActorHint), tr(Str::TlAufklappen));
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            g_app->selectedActor = figAn ? -1 : figIdx;
            g_app->cameraSelected = false;
            g_app->mapDirty = true;
        }
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (offen) { g_app->tlOffen.erase(d.name); } else { g_app->tlOffen.insert(d.name); }
        }
    }
    // Unsichtbar: schraffiert ueber die ganze Hoehe.
    for (const Clip& c : d.unsichtbar.clips) {
        const float x0 = std::max(r.xAt(c.startMs), r.x0);
        const float x1 = std::min(r.xAt(c.endMs), r.x0 + r.w);
        if (x1 <= x0) { continue; }
        r.dl->PushClipRect(ImVec2{x0, f.Min.y}, ImVec2{x1, f.Max.y}, true);
        const float st = 4.0F * r.k;
        for (float x = std::floor((x0 - r.x0) / (st * 2.0F)) * st * 2.0F + r.x0; x < x1; x += st * 2.0F) {
            r.dl->AddRectFilled(ImVec2{x + st, f.Min.y + 1.0F}, ImVec2{x + st * 2.0F, f.Max.y - 1.0F}, kUnsichtbar);
        }
        r.dl->PopClipRect();
        if (r.hover && ImGui::IsMouseHoveringRect(ImVec2{x0, f.Min.y}, ImVec2{x1, f.Max.y})) {
            ImGui::SetTooltip("%s\n%s - %s", tr(Str::ZlUnsichtbar), zeitText(c.startMs).c_str(), zeitText(c.endMs).c_str());
        }
    }
    // Oben: aussagekraeftige Animationen mit Namen (klein).
    for (const Clip& c : d.oben.clips) {
        if (r.xAt(c.endMs) - r.xAt(c.startMs) < 4.0F) { continue; }
        std::string n = c.text;
        for (const char* p : {"BOTH_", "TORSO_", "LEGS_"}) {
            if (n.rfind(p, 0) == 0) { n = n.substr(std::strlen(p)); }
        }
        BlockArgs a;
        a.zeile = d.name.c_str();
        a.typ = ZDarsteller;
        a.anzeige = d.anzeige;
        a.text = c.bahn == 0 ? n : std::string();
        a.farbe = kAnim;
        a.y0 = f.Min.y + 3.0F * r.k;
        a.y1 = f.Min.y + 18.0F * r.k;
        a.ziehbar = false;
        (void)block(r, c, a);
    }
    // Steht still: gestrichelt.
    for (const Clip& c : d.stehen.clips) {
        const float x0 = std::max(r.xAt(c.startMs), r.x0);
        const float x1 = std::min(r.xAt(c.endMs), r.x0 + r.w);
        const float y = f.Min.y + 28.0F * r.k;
        const float st = 4.0F * r.k;
        for (float x = x0; x < x1; x += st * 2.0F) {
            r.dl->AddLine(ImVec2{x, y}, ImVec2{std::min(x + st, x1), y}, kHalten, 2.0F);
        }
        if (r.hover && x1 > x0 && ImGui::IsMouseHoveringRect(ImVec2{x0, y - 4.0F * r.k}, ImVec2{x1, y + 4.0F * r.k})) {
            ImGui::SetTooltip("%s\n%s - %s", tr(Str::ZlSteht), zeitText(c.startMs).c_str(), zeitText(c.endMs).c_str());
        }
    }
    // Unten: laeuft 1, 2 ... (und Fahrten, ROFF).
    int balken = 0;
    for (const Clip& c : d.laufen.clips) {
        BlockArgs a;
        a.zeile = d.name.c_str();
        a.typ = ZDarsteller;
        a.anzeige = d.anzeige;
        switch (c.art) {
            case Art::Laufen: a.text = fmt(tr(c.gang == "BOTH_RUN1" ? Str::ZlRennt : Str::ZlLaeuft), c.nummer); break;
            case Art::Gleiten: a.text = tr(Str::ZlFaehrt); break;
            case Art::Drehen: a.text = tr(Str::ZlDreht); break;
            default: a.text = "ROFF " + c.text; break;
        }
        // Gleichzeitig (ein Schiff faehrt und dreht): nur der erste beschriftet.
        if (c.bahn > 0) { a.text.clear(); }
        a.farbe = kLaufen;
        a.y0 = f.Min.y + 21.0F * r.k;
        a.y1 = f.Min.y + 36.0F * r.k;
        (void)block(r, c, a);
        const ImRect br = g_app->zlBloecke.empty() ? ImRect{} : g_app->zlBloecke.back().r;
        g_app->tlBalken.push_back(App::TlBalken{c.pfad, br.Min, br.Max, c.art == Art::Laufen, figIdx});
        ++balken;
    }
    if (figAn) { g_app->tlBewegungsBalken += balken; }
    zeileBeenden(r, f);
    if (!offen) { return; }
    // Aufgeklappt: Animation und Befehle.
    {
        const float bh = 26.0F * r.k;
        const std::string id = d.name + "/animation";
        const ImRect fa = zeileBeginnen(r, id.c_str(), tr(Str::ZlAnimation), "", std::max(30.0F * r.k, bahnenHoehe(d.animation, 26.0F) * r.k),
                                        false, -1, nullptr, false, 20.0F * r.k);
        for (const Clip& c : d.animation.clips) {
            if (!passt(c)) { continue; }
            std::string n = c.text;
            for (const char* p : {"BOTH_", "TORSO_", "LEGS_"}) {
                if (n.rfind(p, 0) == 0) { n = n.substr(std::strlen(p)); }
            }
            BlockArgs a;
            a.zeile = d.name.c_str();
            a.typ = ZDarsteller;
            a.anzeige = d.anzeige;
            a.text = n;
            a.farbe = kAnim;
            a.y0 = fa.Min.y + 4.0F * r.k + static_cast<float>(c.bahn) * bh;
            a.y1 = a.y0 + bh - 4.0F * r.k;
            (void)block(r, c, a);
        }
        zeileBeenden(r, fa);
    }
    {
        const std::string id = d.name + "/befehle";
        const ImRect fb = zeileBeginnen(r, id.c_str(), tr(Str::ZlBefehle), "", 30.0F * r.k, false, -1, nullptr, false, 20.0F * r.k);
        std::vector<MarkenText> texte;
        ablaufZeile(r, fb, d.befehle, id.c_str(), ZDarsteller, d.anzeige, texte, true);
        zeileBeenden(r, fb, &texte);
    }
}

// Setzt die Zeit und haelt den Ton an (wie ein Klick ins Lineal).
void zeitSetzen(double ms) {
    // Ein laufender Filmdurchgang endet - der Anwender will diese Zeit sehen
    // (er laeuft spaeter von vorn).
    if (g_app->zlFilmPass >= 0) {
        g_app->zlFilmPass = -1;
        g_app->zlFilmWunsch = -1;
        g_app->zlFilmStand = 0;
    }
    g_app->playMs = std::max(0.0, ms);
    g_app->playing = false;
    g_app->audioUpTo = g_app->playMs;
    g_app->audio.stopAll();
    g_app->laufendeSchleifen.clear();
    tonZuruecksetzen();
    g_app->musicAudio.stopAll();
    g_app->mapDirty = true;
}

// Die Zeile eines Pfades im Skriptbaum (alle Bloecke offen, 1 = erste Zeile).
int baumZeile(const std::vector<Node>& ns, const Path& p, std::size_t tiefe, int& zaehler) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        ++zaehler;
        if (tiefe < p.size() && i == p[tiefe]) {
            if (tiefe + 1 == p.size()) { return zaehler; }
            return baumZeile(ns[i].children, p, tiefe + 1, zaehler);
        }
        std::vector<const Node*> stapel;
        for (const Node& k : ns[i].children) { stapel.push_back(&k); }
        while (!stapel.empty()) {
            const Node* k = stapel.back();
            stapel.pop_back();
            ++zaehler;
            for (const Node& kk : k->children) { stapel.push_back(&kk); }
        }
    }
    return -1;
}

}  // namespace

// --- Das Modell bauen (prepareScene) -------------------------------------------

// Untertitel wie CG_CaptionText (cg_text.cpp / cg_servercmds.cpp): Schluessel
// "<LEVEL>_<dateiname ohne Endung>" im StringEd-Paket strings/english/<level>.str
// (stripLevelName; von den Sonderfaellen der von JKA: YAVIN1B liest YAVIN1).
std::string untertitelFuer(const std::string& klang, std::string* schluessel) {
    std::string lv = g_app->map.path;
    const std::size_t s = lv.find_last_of('/');
    if (s != std::string::npos) { lv = lv.substr(s + 1); }
    lv = lv.substr(0, lv.find('.'));
    for (char& c : lv) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    if (g_app->zlTexteKarte != g_app->map.path) {
        g_app->zlTexteKarte = g_app->map.path;
        g_app->zlTexte.clear();
        std::vector<std::string> ebenen{lv};
        if (lv == "YAVIN1B") { ebenen.emplace_back("YAVIN1"); }
        menue::Texte t;
        for (const std::string& e : ebenen) {
            std::string inhalt;
            std::string kl = "strings/english/" + e + ".str";
            for (char& c : kl) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (readFromArchives(kl, inhalt) || readFromArchives("strings/english/" + e + ".str", inhalt)) {
                t.ladeStr("strings/english/" + e + ".str", inhalt, "ENGLISH");
            }
        }
        g_app->zlTexte = std::move(t.werte);
        g_app->zlTexteEbenen = ebenen;
    }
    std::string n = klang;
    const std::size_t sl = n.find_last_of("/\\");
    if (sl == std::string::npos) { return {}; }   // CG_CaptionText: ohne '/' kein Text
    n = n.substr(sl + 1);
    n = n.substr(0, n.rfind('.'));
    for (char& c : n) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    for (const std::string& e : g_app->zlTexteEbenen) {
        const std::string k = e + "_" + n;
        if (schluessel != nullptr && schluessel->empty()) { *schluessel = k; }
        const auto it = g_app->zlTexte.find(k);
        if (it != g_app->zlTexte.end()) {
            if (schluessel != nullptr) { *schluessel = k; }
            return it->second;
        }
    }
    return {};
}

void zeitleisteModellBauen() {
    spuren::SpurenQuellen q;
    q.zeitleiste = &g_app->timeline;
    q.kamera = &g_app->camTrack;
    q.szene = &g_app->scene;
    q.ablauf = &g_app->ablauf;
    q.skript = &g_app->doc.script();
    q.klangMs = [](const std::string& d) { return soundLengthMs(d); };
    q.untertitel = [](const std::string& d, std::string* k) { return untertitelFuer(d, k); };
    q.musikVon = [](const std::string& ziel) {
        bool musik = false;
        const std::string d = useSoundFor(ziel, &musik);
        return musik ? d : std::string();
    };
    q.weltMusik = weltMusik();
    g_app->spuren = spuren::baue(q);
    ++g_app->zlModellStand;   // die Kamerabilder gelten nicht mehr
    // Die Auswahl gilt fuer das alte Modell - nach einer Aenderung zeigt sie
    // auf denselben Befehl, wenn es ihn noch gibt.
    if (g_app->zlAuswahl.da) {
        const Clip alt = g_app->zlAuswahl.clip;
        bool gefunden = false;
        const auto suche = [&](const Zeile& z) {
            for (const Clip& c : z.clips) {
                if (!gefunden && c.art == alt.art && c.pfad == alt.pfad && c.wer == alt.wer && !c.pfad.empty()) {
                    g_app->zlAuswahl.clip = c;
                    gefunden = true;
                }
            }
        };
        const spuren::Spuren& s = g_app->spuren;
        for (const Zeile* z : {&s.ort, &s.drehung, &s.fahrt, &s.schwenk, &s.zoom, &s.blende, &s.rollen, &s.wackeln, &s.folgen,
                               &s.kameraBahn, &s.sprache, &s.musik, &s.skript}) {
            suche(*z);
        }
        for (const spuren::Darsteller& d : s.darsteller) {
            for (const Zeile* z : {&d.laufen, &d.animation, &d.oben, &d.befehle}) { suche(*z); }
        }
    }
}

// Die Breite der Namensspalte: der laengste Name, in Grenzen.
float zeitleisteNamenBreite(float verfuegbar) {
    const float fs = ImGui::GetFontSize();
    float b = fs * 9.0F;
    for (const spuren::Darsteller& d : g_app->spuren.darsteller) {
        b = std::max(b, ImGui::CalcTextSize(d.anzeige.c_str()).x + ImGui::CalcTextSize(d.name.c_str()).x * 0.85F + fs * 3.2F);
    }
    for (const Str s : {Str::ZlOrt, Str::ZlDrehung, Str::ZlSichtfeld, Str::ZlSkript}) {
        b = std::max(b, ImGui::CalcTextSize(tr(s)).x + fs * 4.5F);
    }
    return std::min(b, verfuegbar * 0.25F);
}

// Die schlichte Navigationsleiste: nur wenn hineingezoomt (sonst waere sie
// eine zweite Zeitleiste). Grauer Grund, Ausschnitt, Abspielkopf.
void zeitleisteNavigation(float x0, float w, double gesamt) {
    g_app->zlNavRect = ImRect{};
    if (g_app->timelineZoom <= 1.0001) { return; }
    const float k = ImGui::GetFontSize() / 14.0F;
    const float h = 16.0F * k;
    const ImVec2 p{x0, ImGui::GetCursorScreenPos().y};
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const double spanne = gesamt / g_app->timelineZoom;
    const float sx = w / static_cast<float>(gesamt);
    dl->AddRectFilled(ImVec2{p.x, p.y + 5.0F * k}, ImVec2{p.x + w, p.y + 11.0F * k}, ImGui::GetColorU32(ImGuiCol_FrameBg));
    const float a = p.x + static_cast<float>(g_app->timelineViewStartMs) * sx;
    const float b = std::max(a + 8.0F * k, p.x + static_cast<float>(g_app->timelineViewStartMs + spanne) * sx);
    dl->AddRectFilled(ImVec2{a, p.y + 2.0F * k}, ImVec2{b, p.y + 14.0F * k}, ImGui::GetColorU32(ImGuiCol_ScrollbarGrab), 3.0F * k);
    const float px = p.x + static_cast<float>(zeitleisteZeigerMs()) * sx;
    dl->AddRectFilled(ImVec2{px - 1.0F, p.y}, ImVec2{px + 1.0F, p.y + h}, kKopf);
    g_app->zlNavRect = ImRect{p, ImVec2{p.x + w, p.y + h}};
    const ImVec2 vor = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton("##zlnav", ImVec2{w, h});
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::ZlNavHint)); }
    if (ImGui::IsItemActive()) {
        // Klicken setzt die Mitte des Ausschnitts dorthin, Ziehen verschiebt ihn.
        const double ms = std::clamp((static_cast<double>(ImGui::GetIO().MousePos.x) - p.x) / sx, 0.0, gesamt);
        g_app->timelineViewStartMs = std::clamp(ms - spanne * 0.5, 0.0, std::max(0.0, gesamt - spanne));
        if (ImGui::IsItemActivated()) { zeitSetzen(std::min(ms, gesamt)); }
    }
    ImGui::SetCursorScreenPos(ImVec2{vor.x, p.y + h});
}

// Die Zeilen unter dem Lineal. x0/w: das Zeitfeld (dieselbe Kante wie das
// Lineal), labelW: die Namensspalte davor.
void zeitleisteSpuren(float x0, float w, float labelW, double sichtStart, double sichtSpanne, double ablaufEnde, float hoehe) {
    const auto uhr = std::chrono::steady_clock::now();
    const spuren::Spuren& s = g_app->spuren;
    const float rollBreite = ImGui::GetStyle().ScrollbarSize;
    g_app->tlZeilenY.clear();
    g_app->zlBloecke.clear();
    g_app->zlBeschriftungen.clear();
    g_app->tlUeberlappungen = 0;
    g_app->tlWaitBloecke = 0;
    g_app->tlBewegungsBalken = 0;
    g_app->tlBalken.clear();
    g_app->tlAuswahlZeile.clear();
    g_app->zlKandidat = App::ZlAuswahl{};
    const int auswahlFigur = g_app->pickedEntity >= 0 ? figurFuerEntity(g_app->pickedEntity) : -1;
    if (auswahlFigur < 0 && g_app->selectedActor < 0) { g_app->tlRollFigur = -1; }

    ImGui::BeginChild("tracks", ImVec2{w + labelW + rollBreite, hoehe}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
    Rahmen r;
    r.dl = ImGui::GetWindowDrawList();
    r.x0 = x0;
    r.w = w;
    r.nameX = ImGui::GetCursorScreenPos().x;
    r.nameW = x0 - r.nameX;
    r.start = sichtStart;
    r.spanne = std::max(1.0, sichtSpanne);
    r.ende = ablaufEnde;
    r.k = ImGui::GetFontSize() / 14.0F;
    r.hover = ImGui::IsWindowHovered() && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
    g_app->tlKopfX = r.nameX;
    g_app->tlZeilenH = 40.0F * r.k;
    const bool detail = g_app->zlDetail;
    const bool probleme = g_app->zlProbleme;

    // --- Kamerabilder (Filmstreifen) -----------------------------------------
    filmZeile(r);
    // --- Kamera ---------------------------------------------------------------
    if (!detail) {
        const bool ovOrt = hatProblem(s.ort);
        if (!s.ort.clips.empty() && (!probleme || ovOrt)) {
            const ImVec2 kp = ImGui::GetCursorScreenPos();
            const ImRect f = zeileBeginnen(r, "ort", tr(Str::ZlOrt), ovOrt ? tr(Str::ZlUeberlappt) : "", 36.0F * r.k, false, -1,
                                           nullptr, ovOrt, 0.0F, UiIcon::Kamera, true);
            tourZielMelden("zeitleiste.kamera", ImRect{kp, ImVec2{x0 - 1.0F, f.Max.y}});
            kameraNameKlick(r, f);
            std::vector<MarkenText> texte;
            for (const Clip& c : s.ort.clips) {
                if (!passt(c)) { continue; }
                if (probleme && !c.problem) { continue; }
                if (c.art == Art::KameraAn || c.art == Art::KameraAus) {
                    marke(r, c, "ort", ZKamera, {}, (f.Min.y + f.Max.y) * 0.5F, Form::Quadrat, kFahrt,
                          c.art == Art::KameraAn ? 2 : 3, texte, std::string());
                    continue;
                }
                BlockArgs a;
                a.zeile = "ort";
                a.typ = ZKamera;
                a.text = c.text;
                a.farbe = kFahrt;
                a.y0 = f.Min.y + 5.0F * r.k;
                a.y1 = f.Max.y - 5.0F * r.k;
                a.ziehbar = false;
                a.nichtZiehen = tr(Str::ZlOrtZiehen);
                Clip breit = c;
                breit.endMs = std::max(c.endMs, c.startMs + 200.0);
                (void)block(r, breit, a);
                if (c.problem) {
                    const ImRect br = g_app->zlBloecke.back().r;
                    r.dl->AddRect(ImVec2{br.Min.x + 1.0F, br.Min.y + 1.0F}, ImVec2{br.Max.x - 1.0F, br.Max.y - 1.0F}, kWarnung,
                                  4.0F * r.k, 0, 2.0F);
                    if (br.Max.x - br.Min.x > 26.0F * r.k) {
                        const float g = 14.0F * r.k;
                        uiIcon(r.dl, UiIcon::Warnung, ImVec2{std::floor(br.Max.x - g - 5.0F * r.k), std::floor((br.Min.y + br.Max.y - g) * 0.5F)},
                               g, kWarnung);
                    }
                }
            }
            zeileBeenden(r, f, &texte);
        }
        const bool ovDreh = hatProblem(s.drehung);
        if (!s.drehung.clips.empty() && (!probleme || ovDreh)) {
            const ImRect f = zeileBeginnen(r, "drehung", tr(Str::ZlDrehung), ovDreh ? tr(Str::ZlUeberlappt) : "",
                                           std::max(36.0F * r.k, bahnenHoehe(s.drehung, kBahnH) * r.k), false, -1, nullptr, ovDreh);
            kameraNameKlick(r, f);
            for (const Clip& c : s.drehung.clips) {
                if (!passt(c)) { continue; }
                if (probleme && !c.springt) { continue; }
                const std::string lab = c.text.empty() ? std::string(tr(Str::ZlSchwenk)) : c.text;
                BlockArgs a;
                a.zeile = "drehung";
                a.typ = ZKamera;
                a.text = c.endMs - c.startMs <= 0.5 ? fmt(tr(Str::ZlSchnitt), lab.c_str()) : lab;
                a.farbe = kSchwenk;
                a.y0 = f.Min.y + 5.0F * r.k + static_cast<float>(c.bahn) * kBahnH * r.k;
                a.y1 = a.y0 + (kBahnH - 4.0F) * r.k;
                Clip breit = c;
                breit.endMs = std::max(c.endMs, c.startMs + 350.0);
                if (breit.endMs > c.endMs) { breit.laengeZiehbar = false; }
                (void)block(r, breit, a);
                springtChip(r, c, a.y0);
            }
            zeileBeenden(r, f);
        }
    } else {
        kameraZeile(r, s.fahrt, "fahrt", tr(Str::TlCamMove));
        kameraZeile(r, s.schwenk, "schwenk", tr(Str::TlCamPan));
        kameraZeile(r, s.zoom, "zoom", tr(Str::TlCamZoom));
        kameraZeile(r, s.blende, "blende", tr(Str::TlCamFade));
        kameraZeile(r, s.rollen, "rollen", tr(Str::TlCamRoll));
        kameraZeile(r, s.wackeln, "wackeln", tr(Str::TlCamShake));
        kameraZeile(r, s.folgen, "folgen", tr(Str::TlCamFollow));
        kameraZeile(r, s.kameraBahn, "kamerabahn", tr(Str::ZlKameraBahn));
        kurvenZeile(r, s.sichtfeld, "sichtfeld", tr(Str::ZlSichtfeld), tr(Str::ZlLinear), 30.0F, 130.0F, kZoom);
        kurvenZeile(r, s.schwarz, "schwarz", tr(Str::ZlSchwarz), "", 0.0F, 1.0F, IM_COL32(215, 219, 227, 255));
    }
    // --- Ton --------------------------------------------------------------------
    spracheZeile(r);
    musikZeile(r);
    // --- Skriptablauf ---------------------------------------------------------
    if (!s.skript.clips.empty() && (!probleme || hatProblem(s.skript))) {
        const std::string datei = fileName(aktiverSkriptPfad());
        const ImRect f = zeileBeginnen(r, "skript", tr(Str::ZlSkript), datei, 30.0F * r.k, false, -1, nullptr, false, 0.0F,
                                       UiIcon::Skript, true);
        std::vector<MarkenText> texte;
        ablaufZeile(r, f, s.skript, "skript", ZSkript, {}, texte, detail);
        zeileBeenden(r, f, &texte);
    }
    // --- Darsteller -------------------------------------------------------------
    {
        bool kopf = false;
        const double v1 = sichtStart + sichtSpanne;
        for (const spuren::Darsteller& d : s.darsteller) {
            // Wer nie zu sehen ist und nichts tut, gehoert nicht in die Leiste;
            // Kartenfiguren ohne Befehle auch nicht (sie stehen nur herum).
            const bool spielt = !d.befehle.clips.empty() || !d.laufen.clips.empty() || !d.animation.clips.empty();
            if (!spielt) { continue; }
            if (g_app->zlNurAktive && !tutWas(d, sichtStart, v1)) { continue; }
            if (probleme && !hatProblem(d.befehle)) { continue; }
            if (!darstellerPasst(d)) { continue; }
            if (!kopf) {
                const ImRect f = zeileBeginnen(r, "darsteller", tr(Str::ZlDarsteller), "", 24.0F * r.k, true);
                zeileBeenden(r, f);
                kopf = true;
            }
            darstellerZeilen(r, d, d.figur, auswahlFigur);
        }
    }
    if (ImGui::BeginPopup("##gangart_tl")) {
        gangartMenueInhalt();
        ImGui::EndPopup();
    }

    // --- Klick und Kantengriff: der oberste Block gewinnt ------------------------
    if (g_app->zlGriffKand.da) {
        const App::TlKandidat k = g_app->zlGriffKand;
        g_app->zlKante = true;
        g_app->zlKantePfad = k.pfad;
        g_app->zlKanteEndMs = k.endMs;
        g_app->zlZiehClip = g_app->zlGriffClip;
        g_app->zlZiehMausX = ImGui::GetIO().MousePos.x;
        g_app->zlZiehMsJePx = r.msJePx();
        const Node* gn = nodeAt(g_app->doc.script(), k.pfad);
        diag::detail("Zeitleiste: Kante von \"" + std::string(gn != nullptr ? gn->name : "?") + "\" gepackt (" +
                     std::to_string(k.startMs) + " - " + std::to_string(k.endMs) + " ms)");
    } else if (g_app->zlKlickKand.da) {
        const App::TlKandidat k = g_app->zlKlickKand;
        if (g_app->zlKandidat.da) {
            g_app->zlAuswahl = g_app->zlKandidat;
            g_app->zlZiehClip = g_app->zlKandidat.clip;
            const Clip& c = g_app->zlKandidat.clip;
            if (!c.pfad.empty() && c.fremd.empty()) { selectByPath(c.pfad); }
            if (k.kamera) {
                g_app->cameraSelected = true;
                g_app->selectedActor = -1;
            } else if (g_app->zlKandidat.zeile == ZDarsteller && !c.wer.empty()) {
                for (const spuren::Darsteller& d : s.darsteller) {
                    if (klein(d.name) == klein(c.wer) && d.figur >= 0) {
                        g_app->selectedActor = d.figur;
                        g_app->cameraSelected = false;
                        // Aus der Leiste gewaehlt: nicht hinrollen - die Zeile
                        // steht schon unter der Maus.
                        g_app->tlRollFigur = d.figur;
                    }
                }
            }
        }
        zeitSetzen(k.startMs);
        if (!k.pfad.empty()) {
            g_app->tlZiehPfad = k.pfad;
            g_app->tlZiehMausX = ImGui::GetIO().MousePos.x;
            g_app->tlZiehStartMs = k.startMs;
            g_app->tlZiehMsJePx = r.msJePx();
            g_app->tlZiehAktiv = false;
            g_app->tlZiehDelta = 0.0;
        }
    }
    g_app->zlGriffKand = App::TlKandidat{};
    g_app->zlKlickKand = App::TlKandidat{};

    // --- Kante ziehen: Dauer im Raster von 50 ms -------------------------------
    if (g_app->zlKante) {
        const Clip& c = g_app->zlZiehClip;
        const double dx = static_cast<double>(ImGui::GetIO().MousePos.x - g_app->zlZiehMausX) * g_app->zlZiehMsJePx;
        double dl = std::round(dx / 50.0) * 50.0;
        dl = std::max(dl, -(c.endMs - c.startMs) + 50.0);
        g_app->zlKanteEndMs = c.endMs + dl;
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        const double alt = c.endMs - c.startMs;
        const double neu = g_app->zlKanteEndMs - c.startMs;
        ImGui::BeginTooltip();
        if (c.art == Art::Warten) {
            // Die Leiste zeigt ein wait ein Spielbild laenger, als sein Wert
            // sagt (TaskManager.cpp:1100) - im Skript steht der Wert ohne.
            ImGui::Text(tr(Str::ZlZiehWait), alt / 1000.0, neu / 1000.0);
            ImGui::TextUnformatted(fmt(tr(Str::ZlZiehWaitRest), std::fabs(dl) / 1000.0).c_str());
            ImGui::TextDisabled(tr(Str::ZlZiehWaitSkript), static_cast<int>(std::lround(std::max(0.0, alt - 50.0))),
                                static_cast<int>(std::lround(std::max(0.0, neu - 50.0))));
        } else {
            const Node* n = nodeAt(g_app->doc.script(), c.pfad);
            ImGui::Text(tr(Str::ZlZiehDauer), alt / 1000.0, neu / 1000.0);
            ImGui::TextDisabled(tr(Str::ZlZiehDauerSkript), n != nullptr ? n->name.c_str() : "?",
                                static_cast<int>(std::lround(alt)), static_cast<int>(std::lround(neu)));
        }
        ImGui::EndTooltip();
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (std::fabs(dl) >= 1.0) { zeitleisteDauerSchreiben(c, neu); }
            g_app->zlKante = false;
            g_app->zlKantePfad.clear();
        }
    }

    // --- Block verschieben: das wait davor (Ripple), mit Alt das danach auch -----
    if (!g_app->tlZiehPfad.empty()) {
        const float dx = ImGui::GetIO().MousePos.x - g_app->tlZiehMausX;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!g_app->tlZiehAktiv && std::fabs(dx) > 4.0F && !g_app->zlKante) { g_app->tlZiehAktiv = true; }
            if (g_app->tlZiehAktiv) {
                const Clip& c = g_app->zlZiehClip;
                g_app->tlZiehDelta = std::round(static_cast<double>(dx) * g_app->tlZiehMsJePx / 50.0) * 50.0;
                g_app->tlZiehDelta = std::max(g_app->tlZiehDelta, -g_app->tlZiehStartMs);
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                const double d = g_app->tlZiehDelta;
                ImGui::BeginTooltip();
                ImGui::Text(tr(d >= 0.0 ? Str::ZlZiehSpaeter : Str::ZlZiehFrueher), std::fabs(d) / 1000.0);
                if (c.art == Art::Satz) { ImGui::TextUnformatted(tr(Str::ZlZiehSatz)); }
                ImGui::TextDisabled("%s", tr(Str::ZlZiehWaitDavor));
                ImGui::TextDisabled("%s", tr(Str::ZlZiehAlt));
                ImGui::EndTooltip();
            }
        } else {
            if (g_app->tlZiehAktiv && std::fabs(g_app->tlZiehDelta) >= 1.0) {
                zeitleisteVerschieben(g_app->zlZiehClip, g_app->tlZiehDelta, ImGui::GetIO().KeyAlt);
            }
            g_app->tlZiehPfad.clear();
            g_app->tlZiehAktiv = false;
            g_app->tlZiehDelta = 0.0;
        }
    }

    // --- Tastatur, solange die Maus ueber der Leiste steht ----------------------
    g_app->tlUeberMaus = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    tourZielMelden("zeitleiste", ImGui::GetCurrentWindow()->Rect());
    if (g_app->tlUeberMaus && !ImGui::GetIO().WantTextInput) {
        const double schritt = ImGui::GetIO().KeyShift ? 1000.0 : 50.0;
        double ziel = zeitleisteZeigerMs();
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) { ziel += schritt; }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) { ziel -= schritt; }
        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) { ziel = 0.0; }
        if (ImGui::IsKeyPressed(ImGuiKey_End, false)) { ziel = ablaufEnde; }
        ziel = std::clamp(ziel, 0.0, std::max(0.0, ablaufEnde));
        if (ziel != zeitleisteZeigerMs()) {
            zeitSetzen(ziel);
            ++g_app->tlSchritte;   // H2: Tour k-neu-timen ("genau schrittweise")
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Tab, false)) { g_app->zlDetail = !g_app->zlDetail; }
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) { g_app->zlAbspielenUmschalten = true; }
    }
    // --- Zoomen mit Strg+Rad um den Zeiger, Umschalt+Rad rollt waagerecht -------
    if (ImGui::IsWindowHovered()) {
        const float rad = ImGui::GetIO().MouseWheel;
        const double gesamt = ablaufEnde * 1.05;
        if (rad != 0.0F && ImGui::GetIO().KeyCtrl) {
            const double anteil = std::clamp<double>((ImGui::GetIO().MousePos.x - x0) / std::max(w, 1.0F), 0.0, 1.0);
            const double unterMaus = sichtStart + anteil * sichtSpanne;
            const double neu = std::clamp(g_app->timelineZoom * std::pow(1.25, static_cast<double>(rad)), 1.0, App::kMaxZoom);
            const double neueSpanne = gesamt / neu;
            g_app->timelineZoom = neu;
            g_app->timelineViewStartMs = std::clamp(unterMaus - anteil * neueSpanne, 0.0, std::max(0.0, gesamt - neueSpanne));
        } else if (rad != 0.0F && ImGui::GetIO().KeyShift) {
            g_app->timelineViewStartMs = std::clamp(g_app->timelineViewStartMs - static_cast<double>(rad) * sichtSpanne * 0.1, 0.0,
                                                    std::max(0.0, gesamt - sichtSpanne));
        }
    }
    ImGui::EndChild();
    g_app->zlZeichenMs = static_cast<float>(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - uhr).count());
}

// --- Der Filmstreifen ---------------------------------------------------------------

// Ein aufgenommenes Kamerabild (RGBA aus Satz 4) als Textur ablegen.
void zeitleisteFilmBild(int nummer, const std::vector<std::uint8_t>& rgba, int breite, int hoehe) {
    if (nummer < 0 || breite <= 0 || hoehe <= 0 || rgba.size() < static_cast<std::size_t>(breite) * static_cast<std::size_t>(hoehe) * 4U) {
        return;
    }
    if (static_cast<std::size_t>(nummer) >= g_app->zlFilm.size()) { g_app->zlFilm.resize(static_cast<std::size_t>(nummer) + 1); }
    App::ZlFilmBild& b = g_app->zlFilm[static_cast<std::size_t>(nummer)];
    // Das Spiel zeigt das Bild deckend - der Alphawert des Ziels zaehlt nicht.
    std::vector<std::uint8_t> deckend = rgba;
    for (std::size_t i = 3; i < deckend.size(); i += 4) { deckend[i] = 255; }
    if (b.textur != nullptr && b.stand == render::texturStand() && b.w == breite && b.h == hoehe) {
        render::updateTexture(b.textur, deckend.data(), breite, hoehe);
    } else {
        b.textur = render::createTexture(deckend.data(), breite, hoehe);
        b.stand = render::texturStand();
    }
    b.w = breite;
    b.h = hoehe;
    b.ms = g_app->playMs;
    ++g_app->zlFilmAufnahmen;
}

// Wo der Zeiger fuer den Anwender steht - waehrend der Filmdurchgang die Zeit
// fuer seine Aufnahmen verstellt, die Zeit davor.
double zeitleisteZeigerMs() { return g_app->zlFilmPass >= 0 ? g_app->zlFilmVorherMs : g_app->playMs; }

// --- Zurueckschreiben ins Skript -------------------------------------------------

namespace {

// Merkt, dass `angezeigt` durch Ziehen geaendert wurde: am Knoten `geaendertK`
// mit seinem Text davor (leer: er wurde eingefuegt).
void merkeGeaendert(Kennung angezeigt, Kennung geaendertK, const std::string& vorher) {
    if (angezeigt == 0 || geaendertK == 0) { return; }
    if (g_app->zlGeaendert.count(angezeigt) == 0U) { g_app->zlGeaendert[angezeigt] = {geaendertK, vorher}; }
}

// Eine Zahl so schreiben, wie sie im Skript steht: mit drei Nachkommastellen,
// wenn der alte Wert sie hatte (BehavEd schreibt "1000.000"), sonst ganz.
std::string zahlWie(const std::string& alt, double v) {
    char b[32];
    if (alt.find('.') != std::string::npos) {
        std::snprintf(b, sizeof(b), "%.3f", v);
    } else {
        std::snprintf(b, sizeof(b), "%ld", std::lround(v));
    }
    return b;
}

}  // namespace

// Die Dauer eines Blocks (rechte Kante) ins Skript: ein wait ohne den
// Spieltakt, den die Leiste dazurechnet (TaskManager.cpp:1100), sonst das
// Dauer-Argument (durationArgIndex). EIN Rueckgaengig-Schritt (replaceAt).
void zeitleisteDauerSchreiben(const spuren::Clip& c, double neuMs) {
    const Node* n = nodeAt(g_app->doc.script(), c.pfad);
    if (n == nullptr) { return; }
    double dauer = neuMs;
    if (n->name == "wait") { dauer = std::max(0.0, dauer - 50.0); }
    Node neu = *n;
    const int idx = durationArgIndex(neu);
    if (idx < 0) { return; }
    std::string& feld = neu.args[static_cast<std::size_t>(idx)].text;
    feld = zahlWie(feld, std::max(0.0, dauer));
    merkeGeaendert(n->kennung, n->kennung, knotenText(*n));
    diag::detail("Zeitleiste: Kante losgelassen, neue Dauer " + feld + " (" + n->name + ")");
    if (g_app->doc.replaceAt(c.pfad, neu)) {
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }
}

// Einen Block in der Zeit verschieben (das wait davor, mit Alt auch das
// danach) - siehe verschiebeImAblauf.
void zeitleisteVerschieben(const spuren::Clip& c, double delta, bool nurDieser) {
    const Node* n = nodeAt(g_app->doc.script(), c.pfad);
    if (n == nullptr) { return; }
    const Kennung angezeigt = n->kennung;
    Path warte;
    std::string vorher;
    if (verschiebeImAblauf(c.pfad, delta, nurDieser, &warte, &vorher)) {
        const Node* w = nodeAt(g_app->doc.script(), warte);
        merkeGeaendert(angezeigt, w != nullptr ? w->kennung : 0, vorher);
    }
}

// --- Was passiert hier? ------------------------------------------------------------

namespace {

struct Erklaerung {
    std::string titel;
    std::string text;
    std::string warn;
    std::string meta;
};

std::string ortText(const float v[3]) {
    return fmt("(%ld, %ld, %ld)", std::lround(v[0]), std::lround(v[1]), std::lround(v[2]));
}

Erklaerung erklaere(const App::ZlAuswahl& w) {
    const Clip& c = w.clip;
    Erklaerung e;
    if (w.zeile == ZFilm) {
        e.titel = fmt(tr(Str::ZlFilmTitel), zeitText(c.startMs).c_str());
        e.text = tr(Str::ZlFilmText);
        return e;
    }
    const double dauer = (c.endMs - c.startMs) / 1000.0;
    const bool mitDauer = c.endMs - c.startMs > 0.5;
    switch (c.art) {
        case Art::Einstellung:
            e.titel = fmt(tr(Str::ZlFahrtTitel), c.text.c_str());
            e.text = fmt(tr(Str::ZlEinstellungText), dauer);
            if (c.problem) { e.warn = tr(Str::ZlEinstellungProblem); }
            break;
        case Art::Fahrt:
            e.titel = fmt(tr(Str::ZlFahrtTitel), c.text.c_str());
            e.text = mitDauer ? fmt(tr(Str::ZlFahrtText), dauer) : std::string(tr(Str::ZlFahrtSofort));
            break;
        case Art::Schwenk:
            e.titel = tr(Str::ZlSchwenkTitel);
            e.text = mitDauer ? fmt(tr(Str::ZlSchwenkText), dauer) : std::string(tr(Str::ZlSchwenkSofort));
            break;
        case Art::Zoom:
            e.titel = fmt(tr(Str::ZlZoomTitel), c.text.c_str());
            e.text = mitDauer ? fmt(tr(Str::ZlZoomText), dauer) : std::string(tr(Str::ZlZoomSofort));
            break;
        case Art::Blende: {
            e.titel = tr(Str::ZlBlende);
            const int von = static_cast<int>(std::lround(spuren::kurveBei(g_app->spuren.schwarz, c.startMs) * 100.0F));
            e.text = fmt(tr(Str::ZlBlendeText), von, static_cast<int>(std::lround(c.wert * 100.0F)), dauer);
            break;
        }
        case Art::Rollen:
            e.titel = tr(Str::ZlRollenTitel);
            e.text = fmt(tr(Str::ZlRollenText), dauer);
            break;
        case Art::Wackeln:
            e.titel = tr(Str::ZlWackelnTitel);
            e.text = fmt(tr(Str::ZlWackelnText), dauer);
            break;
        case Art::Folgen:
            e.titel = fmt(tr(Str::ZlFolgenTitel), c.text.c_str());
            e.text = tr(Str::ZlFolgenText);
            break;
        case Art::KameraBahn:
            e.titel = tr(Str::ZlBahnTitel);
            e.text = fmt(tr(Str::ZlBahnText), dauer);
            break;
        case Art::KameraAn:
            e.titel = tr(Str::ZlKameraAn);
            e.text = tr(Str::ZlAnText);
            break;
        case Art::KameraAus:
            e.titel = tr(Str::ZlKameraAus);
            e.text = tr(Str::ZlAusText);
            break;
        case Art::Satz: {
            const bool stimme = klein(c.kanal).rfind("chan_voice", 0) == 0;
            e.titel = stimme ? fmt(tr(Str::ZlSprichtTitel), (w.anzeige.empty() ? c.wer : w.anzeige).c_str())
                             : std::string(tr(Str::ZlKlangTitel));
            e.text = c.untertitel.empty() ? std::string(tr(Str::ZlOhneUntertitel)) : "\"" + c.untertitel + "\"";
            e.meta = fmt(tr(Str::ZlTonMeta), c.datei.c_str(), c.klangMs / 1000.0, c.kanal.c_str());
            e.meta += "\n" + std::string(tr(Str::ZlTonLaenge));
            break;
        }
        case Art::Musik:
            e.titel = tr(Str::ZlMusik);
            e.text = fmt(tr(Str::ZlMusikText), c.datei.c_str());
            break;
        case Art::Warten:
            if (c.warteArt == spuren::WarteArt::Signal) {
                e.titel = fmt(tr(Str::ZlSignalTitel), c.wartetAuf.c_str());
                if (c.sender.empty()) {
                    e.warn = tr(Str::ZlSignalNie);
                } else {
                    std::string von;
                    std::string wann;
                    for (const auto& [ms, wer] : c.sender) {
                        von += (von.empty() ? "" : ", ") + wer;
                        wann += (wann.empty() ? "" : ", ") + zeitText(ms);
                    }
                    e.text = fmt(tr(Str::ZlSignalVon), von.c_str(), wann.c_str());
                }
            } else if (c.warteArt == spuren::WarteArt::Aufgabe) {
                e.titel = fmt(tr(Str::ZlAufgabeTitel), c.wartetAuf.c_str());
                e.text = tr(Str::ZlAufgabeText);
            } else {
                e.titel = fmt(tr(Str::ZlWartetTitel), dauer);
                e.text = tr(Str::ZlWartetText);
            }
            break;
        case Art::Signal:
            e.titel = fmt(tr(Str::ZlSendetTitel), c.text.c_str());
            e.text = tr(Str::ZlSendetText);
            break;
        case Art::Benutzen:
            e.titel = fmt(tr(Str::ZlUseTitel), c.text.c_str());
            e.text = tr(Str::ZlUseText);
            break;
        case Art::Setzen: {
            const std::size_t k = c.text.find(',');
            const std::string schl = c.text.substr(0, k);
            std::string wert = k == std::string::npos ? std::string() : c.text.substr(k + 1);
            while (!wert.empty() && wert.front() == ' ') { wert.erase(wert.begin()); }
            e.titel = schl;
            e.text = fmt(tr(Str::ZlSetText), schl.c_str(), wert.c_str());
            break;
        }
        case Art::Kartenwechsel:
            e.titel = fmt(tr(Str::ZlKartenwechsel), c.text.c_str());
            e.warn = tr(Str::ZlWechselText);
            break;
        case Art::NichtNachgebildet:
            e.titel = c.text;
            e.warn = tr(Str::ZlNichtNachText);
            break;
        case Art::Laufen:
            e.titel = fmt(tr(c.gang == "BOTH_RUN1" ? Str::ZlRenntTitel : Str::ZlLaufTitel), w.anzeige.c_str(), c.text.c_str());
            e.text = fmt(tr(Str::ZlLaufText), ortText(c.von).c_str(), ortText(c.nach).c_str(), dauer);
            e.meta = tr(Str::ZlLaufKante);
            break;
        case Art::Gleiten:
            e.titel = fmt(tr(Str::ZlGleitTitel), w.anzeige.c_str());
            e.text = fmt(tr(Str::ZlGleitText), ortText(c.von).c_str(), ortText(c.nach).c_str(), dauer);
            break;
        case Art::Drehen:
            e.titel = fmt(tr(Str::ZlDrehTitel), w.anzeige.c_str());
            e.text = fmt(tr(Str::ZlDrehText), dauer);
            break;
        case Art::Roff:
            e.titel = fmt(tr(Str::ZlRoffTitel), w.anzeige.c_str(), c.text.c_str());
            e.text = fmt(tr(Str::ZlRoffText), dauer);
            break;
        case Art::Animation: {
            e.titel = w.anzeige + ": " + c.text;
            const Str teil = c.teil == AnimPart::Upper   ? Str::ZlTeilOben
                             : c.teil == AnimPart::Lower ? Str::ZlTeilUnten
                                                         : Str::ZlTeilGanz;
            e.text = fmt(tr(Str::ZlAnimText), dauer, tr(teil));
            break;
        }
        default:
            e.titel = c.text.empty() ? c.befehl : c.text;
            e.text = tr(Str::ZlBefehlText);
            break;
    }
    if (c.springt) { e.warn = fmt(tr(Str::ZlSpringtText), zeitText(c.vorigerStartMs).c_str()); }
    return e;
}

}  // namespace

namespace {
void erklaerungSkriptzeile(const Clip& c);
}  // namespace

float zeitleisteErklaerungHoehe(float verfuegbar) {
    if (!g_app->zlAuswahl.da || g_app->leftMode != 1) { return 0.0F; }
    const float inhalt = g_app->zlErklaerungInhaltH > 0.0F ? g_app->zlErklaerungInhaltH : verfuegbar * 0.42F;
    return std::floor(std::clamp(inhalt, std::min(ImGui::GetFontSize() * 8.0F, verfuegbar * 0.75F), verfuegbar * 0.75F));
}

void zeitleisteErklaerung(float breite, float hoehe) {
    const App::ZlAuswahl& w = g_app->zlAuswahl;
    if (!w.da) { return; }
    ImGui::BeginChild("##zlwas", ImVec2{breite, hoehe}, ImGuiChildFlags_Borders);
    g_app->zlErklaerungRect = ImGui::GetCurrentWindow()->Rect();
    const float fs = ImGui::GetFontSize();
    ImGui::TextDisabled("%s", tr(Str::ZlWas));
    ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - ImGui::GetFrameHeight()));
    if (iconKnopf(tr(Str::ZlSchliessen), UiIcon::Schliessen, nullptr)) { g_app->zlAuswahl.da = false; }
    const Erklaerung e = erklaere(w);
    const Clip& c = w.clip;
    g_app->zlErklaerungTitel = e.titel;
    ImGui::PushTextWrapPos(0.0F);
    ImGui::PushFont(nullptr, fs * 1.15F);
    ImGui::TextUnformatted(e.titel.c_str());
    ImGui::PopFont();
    const std::string zeit = c.endMs > c.startMs + 0.5 ? zeitText(c.startMs) + " – " + zeitText(c.endMs) : zeitText(c.startMs);
    ImGui::TextDisabled("%s", zeit.c_str());
    ImGui::PopTextWrapPos();
    erklaerungSkriptzeile(c);
    ImGui::PushTextWrapPos(0.0F);
    if (!e.text.empty()) { ImGui::TextUnformatted(e.text.c_str()); }
    if (!e.warn.empty()) { ImGui::TextColored(ImVec4{0.95F, 0.4F, 0.36F, 1.0F}, "%s", e.warn.c_str()); }
    if (!e.meta.empty()) { ImGui::TextDisabled("%s", e.meta.c_str()); }
    ImGui::PopTextWrapPos();
    if (!c.befehl.empty()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::BeginChild("##zlcode",
                          ImVec2{0.0F, ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2.0F +
                                           ImGui::GetStyle().ScrollbarSize},
                          ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextUnformatted(c.befehl.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    // Fuer das naechste Bild: so hoch soll der Kasten sein.
    g_app->zlErklaerungInhaltH = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y + 2.0F;
    ImGui::EndChild();
}

namespace {

// Die Skriptzeile und der Knopf, der sie im Editor oeffnet.
void erklaerungSkriptzeile(const Clip& c) {
    if (!c.pfad.empty() && c.fremd.empty()) {
        int z = 0;
        const int zeile = baumZeile(g_app->doc.script().nodes, c.pfad, 0, z);
        ImGui::TextDisabled(tr(Str::ZlZeileVon), fileName(aktiverSkriptPfad()).c_str(), zeile);
        if (ImGui::Button(tr(Str::ZlZeileOeffnen))) {
            // Wirklich in den Editor: die Zeile im Skriptbaum waehlen und ihren
            // Bearbeiten-Dialog oeffnen.
            selectByPath(c.pfad);
            openEditorForNode(c.pfad);
            ++g_app->zlZeileGeoeffnet;
        }
    } else if (!c.fremd.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled(tr(Str::ZlAusSkript), c.fremd.c_str());
        ImGui::PopTextWrapPos();
        if (ImGui::Button(fmt(tr(Str::ZlSkriptOeffnen), fileName(c.fremd).c_str()).c_str())) {
            const int reiter = reiterMitPfad(c.fremd);
            if (reiter >= 0) {
                g_app->tabRequest = reiter;
            } else {
                std::string daten;
                if (readFromArchives("scripts/" + c.fremd + ".txt", daten) || readFromArchives(c.fremd, daten)) {
                    const std::string name = c.fremd;
                    withUnsaved([daten, name] { oeffneSkriptAusSpeicher(daten, name); });
                }
            }
        }
    } else if (!istKamera(c.art) && c.art != Art::Musik) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::ZlNachbau));
        ImGui::PopTextWrapPos();
    }
}

}  // namespace

}  // namespace bhed::gui
