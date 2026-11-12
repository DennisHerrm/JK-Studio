// app_modelltab_zeit.cpp - Model-Tab, AP1 (MBENTWURF Teil B, 11.11.): die Zeitleiste nach dem freigegebenen Entwurf
//
// shank 04.11.: "und in jedem Tab sieht man die animation?" - "in 3ds max modus muessen die unteren auch striche sein als
// frames wie in 3ds max"; 10.11.: "dann ist die Timeline nicht gut. Haette sie gerne wie in 3ds max oder blender. Je
// nachdem welchen modi" - "es geht ja nicht nur um controls ... sondern das aussehen".
// Massstab: plaene/modellblock_stufe0/entwurf_modelltab.html, Funktion zeitleiste(max) und statusZeile(max).
//
//   Aufbau        Griff zum Hochziehen - Knopfreihe - Skala (Namensspalte + Zeitfeld) - Spuren - Hinweis - Statuszeile.
//   Fassungen     voll (Animation: Skala mit den Schluesseln der Auswahl, Spuren je Glied = das Dopesheet aus
//                 app_modelltab_kurven.cpp, ein gemeinsames Lineal), Sequenz (Ansehen/Pruefen: Tempo, Interpolation,
//                 Schleife erzwingen, Zeilen Primaer/Sekundaer), kurz (Modellieren, UV, Weights, Painting).
//   wie 3ds Max   Anfang, Bild zurueck, Abspielen, Bild vor, Ende, "12 / 119", Auto Key (N), Set Key (K), Zeitkonfiguration;
//                 in der Skala und in jeder Spur ein feiner Strich je Frame, Schluessel als Striche (Rot = Bewegung,
//                 Gruen = Drehung), Zaehlung ab 0. Vorbild: Zeitschieber, Spurleiste und Zeitsteuerung von 3ds Max
//                 (Autodesk-Hilfe "Track Bar", "Time Controls", "Auto Key Animation Mode", "Set Key Animation Mode").
//   wie Blender   Anfang, Schluessel zurueck, Abspielen, Schluessel vor, Ende, Aufnahme (Auto-Keying), Start/Ende/Bild,
//                 Rauten, Zaehlung ab 1. Vorbild: Timeline von Blender (Handbuch "Editors > Timeline").
//   Auto Key      an (Vorgabe, wie bis rc611): jeder Zug an einem Griff setzt einen Schluessel. Aus: die Pose bleibt ohne
//                 Schluessel, bis Set Key (K, Blender I) sie behaelt; ein Bildwechsel verwirft sie (wie Set Key in
//                 3ds Max und wie Blender ohne Auto-Keying).
// Eigene Zeichnung: alle Symbole hier aus Strichen und Flaechen, nichts aus 3ds Max oder Blender.
#include "app_internal.h"
#include "textumbruch.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

ModellZeit g_zeit;
ModellZeitProbe g_probe;   // waehrend des Bilds gefuellt, am Ende der Leiste nach g_zeit.probe

const AnimEntry* sequenz() {
    if (g_app->model.empty() || g_app->anim.empty() || g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) {
        return nullptr;
    }
    return &g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
}

std::string format(Str s, auto... a) {
    char b[700];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

// Selbsttest (FIX612 R2): zeigt die Sequenz, mit der die Leiste zeichnet, in die Liste, die gerade gilt?
void sequenzPruefen(const AnimEntry* e) {
    if (e == nullptr) { return; }
    const auto p = reinterpret_cast<std::uintptr_t>(e);
    const auto a = reinterpret_cast<std::uintptr_t>(g_app->animList.data());
    if (p < a || p >= a + g_app->animList.size() * sizeof(AnimEntry)) { ++g_zeit.veralteteSequenz; }
}

void melden(const std::string& m) {
    mt().meldung = m;
    addStatus(m);
}

void merke(float r[4], ImVec2 lo, ImVec2 hi) {
    r[0] = lo.x;
    r[1] = lo.y;
    r[2] = hi.x - lo.x;
    r[3] = hi.y - lo.y;
}

// --- Farben: aus dem Thema (hell und dunkel), die Schluessel in den Farben der Spurleiste -------------------------------
ImU32 mischen(ImGuiCol a, ImGuiCol b, float anteil, float alpha = 1.0F) {
    const ImVec4& x = ImGui::GetStyleColorVec4(a);
    const ImVec4& y = ImGui::GetStyleColorVec4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4{x.x + (y.x - x.x) * anteil, x.y + (y.y - x.y) * anteil, x.z + (y.z - x.z) * anteil, alpha});
}
ImU32 akzent() { return ImGui::GetColorU32(ImGuiCol_SliderGrabActive); }
constexpr ImU32 kRot = IM_COL32(224, 80, 80, 255);       // Bewegung (Position)
constexpr ImU32 kGruen = IM_COL32(76, 192, 76, 255);     // Drehung
constexpr ImU32 kAufnahme = IM_COL32(205, 62, 62, 255);  // Auto Key an

// --- Zaehlung und Zeitanzeige ------------------------------------------------------------------------------------------
int versatz() { return modellBlender() ? 1 : 0; }   // wie Max ab 0, wie Blender ab 1 (stufenplan.md, Entscheidung shank 04.11.)

std::string zeitZahl(int rel, int fps) {
    char z[32];
    if (g_app->settings.modellZeitAnzeige == 1) {
        std::snprintf(z, sizeof z, "%.2f", static_cast<double>(rel) / static_cast<double>(std::max(std::abs(fps), 1)));
    } else {
        std::snprintf(z, sizeof z, "%d", rel + versatz());
    }
    return z;
}

// Schritt der Striche: 1, 2, 5, 10, 20, 50 ... bis ein Schritt mindestens `mind` Bildpunkte breit ist
int schrittFuer(float pxProBild, float mind) {
    int zehner = 1;
    for (int i = 0; i < 6; ++i) {
        for (const int f : {1, 2, 5}) {
            if (static_cast<float>(f * zehner) * pxProBild >= mind) { return f * zehner; }
        }
        zehner *= 10;
    }
    return zehner;
}

void bildSetzen(const AnimEntry& e, int rel) {
    g_app->animFrame = e.firstFrame + std::clamp(rel, 0, std::max(e.numFrames - 1, 0));
    g_app->animTime = 0.0F;
    g_app->animPlaying = false;
    g_app->modelDirty = true;
}

void spielenUmschalten(const AnimEntry* e) {
    if (e == nullptr) { return; }
    if (!g_app->animPlaying && g_app->animFrame >= e->firstFrame + e->numFrames - 1) { g_app->animFrame = e->firstFrame; }
    g_app->animPlaying = !g_app->animPlaying;
}

bool cfgDa(const AnimEntry* e) {
    const modell::Inhalt& in = mt().dok.inhalt();
    return e != nullptr && !in.cfgPfad.empty() && in.cfg.zeileVon(e->name) >= 0;
}

constexpr float kTempi[] = {0.1F, 0.25F, 0.5F, 1.0F, 1.5F, 2.0F, 3.0F};

void tempoSchritt(int d) {
    int i = 3;
    float best = 1e9F;
    for (int k = 0; k < static_cast<int>(std::size(kTempi)); ++k) {
        if (std::fabs(kTempi[k] - g_app->animSpeed) < best) {
            best = std::fabs(kTempi[k] - g_app->animSpeed);
            i = k;
        }
    }
    g_app->animSpeed = kTempi[std::clamp(i + d, 0, static_cast<int>(std::size(kTempi)) - 1)];
}

// --- Auto Key: die Pose ohne Schluessel -------------------------------------------------------------------------------
bool zugLaeuft() {
    const ModellTab& t = mt();
    return t.rigZieht || t.rigModal != 0 || t.ikZug;
}

const char* setKeyTaste() { return modellBlender() ? "I" : "K"; }

void offenVerwerfen() {
    ModellZeit& z = g_zeit;
    for (auto it = z.offen.rbegin(); it != z.offen.rend(); ++it) {
        if (!mt().dok.abbrechen(*it)) { break; }   // liegt nicht mehr oben (ein anderer Befehl kam dazu): er bleibt
    }
    z.offen.clear();
    ++z.verworfen;
    g_app->modelDirty = true;
    melden(tr(Str::MzOffenVerworfen));
}

// Je Bild: gilt die Pose ohne Schluessel noch? Ein Bild- oder Sequenzwechsel verwirft sie, Rueckgaengig oder ein anderer
// Befehl beendet sie still (der Schritt gehoert dann dem Rueckgaengig-Stapel wie jeder andere).
void offenPruefen() {
    ModellZeit& z = g_zeit;
    if (z.offen.empty()) { return; }
    ModellTab& t = mt();
    const int bild = ImGui::GetFrameCount();
    if (zugLaeuft()) {
        z.offenStand = t.dok.stand();
        z.zugBild = bild;
        return;
    }
    if (bild - z.zugBild <= 1) {
        z.offenStand = t.dok.stand();   // der letzte Schritt des Zugs (Loslassen im selben Bild)
    } else if (t.dok.stand() != z.offenStand) {
        z.offen.clear();
        return;
    }
    const AnimEntry* e = sequenz();
    if (g_app->animFrame != z.offenBild || (e != nullptr ? e->name : std::string()) != z.offenSequenz) { offenVerwerfen(); }
}

// --- Eigene Symbole ----------------------------------------------------------------------------------------------------
enum class Sym : int { Keins = 0, Anfang, BildZurueck, Abspielen, Pause, BildVor, Ende, KeyZurueck, KeyVor, Aufnahme, Uhr, Auf, Zu, Menue, Minus, Plus };

// In ein Quadrat der Kantenlaenge g ab p; Koordinaten 0..1.
void symbol(ImDrawList* dl, Sym s, ImVec2 p, float g, ImU32 f) {
    const auto P = [&](float x, float y) { return ImVec2{std::floor(p.x + x * g) + 0.5F, std::floor(p.y + y * g) + 0.5F}; };
    const auto dreieck = [&](float x0, float x1) { dl->AddTriangleFilled(P(x0, 0.2F), P(x0, 0.8F), P(x1, 0.5F), f); };
    const auto balken = [&](float x0, float x1) { dl->AddRectFilled(P(x0, 0.2F), P(x1, 0.8F), f); };
    const auto raute = [&](float x, float r) { dl->AddQuadFilled(P(x, 0.5F - r), P(x + r, 0.5F), P(x, 0.5F + r), P(x - r, 0.5F), f); };
    const float d = std::max(1.5F, g * 0.11F);
    switch (s) {
        case Sym::Anfang:
            balken(0.08F, 0.2F);
            dreieck(0.58F, 0.24F);
            dreieck(0.94F, 0.6F);
            break;
        case Sym::BildZurueck: dreieck(0.78F, 0.2F); break;
        case Sym::Abspielen: dreieck(0.24F, 0.84F); break;
        case Sym::Pause:
            balken(0.22F, 0.42F);
            balken(0.58F, 0.78F);
            break;
        case Sym::BildVor:
            dreieck(0.14F, 0.68F);
            balken(0.76F, 0.88F);
            break;
        case Sym::Ende:
            dreieck(0.06F, 0.4F);
            dreieck(0.42F, 0.76F);
            balken(0.8F, 0.92F);
            break;
        case Sym::KeyZurueck:
            raute(0.26F, 0.24F);
            dreieck(1.0F, 0.54F);
            break;
        case Sym::KeyVor:
            dreieck(0.0F, 0.46F);
            raute(0.74F, 0.24F);
            break;
        case Sym::Aufnahme: dl->AddCircleFilled(ImVec2{p.x + g * 0.5F, p.y + g * 0.5F}, g * 0.3F, f, 24); break;
        case Sym::Uhr:
            dl->AddCircle(ImVec2{p.x + g * 0.5F, p.y + g * 0.56F}, g * 0.34F, f, 24, d);
            dl->AddLine(P(0.5F, 0.56F), P(0.5F, 0.34F), f, d);
            dl->AddLine(P(0.5F, 0.56F), P(0.66F, 0.56F), f, d);
            dl->AddLine(P(0.4F, 0.1F), P(0.6F, 0.1F), f, d);
            break;
        case Sym::Auf: dl->AddTriangleFilled(P(0.2F, 0.34F), P(0.8F, 0.34F), P(0.5F, 0.72F), f); break;
        case Sym::Zu: dl->AddTriangleFilled(P(0.34F, 0.2F), P(0.34F, 0.8F), P(0.72F, 0.5F), f); break;
        case Sym::Menue:
            for (const float y : {0.28F, 0.5F, 0.72F}) { dl->AddLine(P(0.18F, y), P(0.82F, y), f, d); }
            break;
        case Sym::Minus: dl->AddLine(P(0.24F, 0.5F), P(0.76F, 0.5F), f, d); break;
        case Sym::Plus:
            dl->AddLine(P(0.24F, 0.5F), P(0.76F, 0.5F), f, d);
            dl->AddLine(P(0.5F, 0.24F), P(0.5F, 0.76F), f, d);
            break;
        case Sym::Keins: break;
    }
}

void tooltip(const char* text) {
    if (text == nullptr || text[0] == '\0' || !ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) { return; }
    ImGui::SetNextWindowSize(ImVec2{std::min(ImGui::CalcTextSize(text).x + ImGui::GetStyle().WindowPadding.x * 2.0F, ImGui::GetFontSize() * 26.0F), 0.0F});
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(0.0F);
    cjkTextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

float symG() { return std::floor(ImGui::GetFrameHeight() * 0.66F); }
float symKnopfBreite() { return std::floor(ImGui::GetFrameHeight() * 1.3F); }
float textKnopfBreite(const char* text, bool mitSymbol) {
    const ImGuiStyle& st = ImGui::GetStyle();
    return std::floor(st.FramePadding.x * 2.0F + ImGui::CalcTextSize(text, ImGui::FindRenderedTextEnd(text)).x + (mitSymbol ? symG() + st.ItemInnerSpacing.x : 0.0F));
}

// Ein Knopf der Knopfreihe wie im Entwurf: Flaeche in Knopffarbe, Symbol und/oder Text. `kennung` ist die Beschriftung
// fuer Selbsttest und Tour (der Name der Funktion), `text` das Gezeigte (nullptr: nur das Symbol).
bool knopf(const char* kennung, Sym s, const char* text, const char* hinweis, float breite, bool an = false, ImU32 anFarbe = 0, ImU32 symFarbe = 0) {
    const float h = ImGui::GetFrameHeight();
    const ImGuiStyle& st = ImGui::GetStyle();
    const bool geklickt = ImGui::InvisibleButton(kennung, ImVec2{breite, h});
    const bool ueber = ImGui::IsItemHovered();
    const bool gedrueckt = ImGui::IsItemActive();
    const bool gesperrt = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 grund = ImGui::GetColorU32(ueber ? (gedrueckt ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : ImGuiCol_Button);
    if (an) { grund = anFarbe != 0 ? anFarbe : akzent(); }   // eingeschaltet in der Akzentfarbe (Entwurf: .knopf.an), Auto Key rot
    dl->AddRectFilled(a, b, grund, st.FrameRounding);
    if (an && ueber) { dl->AddRect(a, b, ImGui::GetColorU32(ImGuiCol_Text, 0.5F), st.FrameRounding); }
    const ImU32 farbe = an ? IM_COL32_WHITE : ImGui::GetColorU32(gesperrt ? ImGuiCol_TextDisabled : ImGuiCol_Text);
    const float g = symG();
    float x = a.x + st.FramePadding.x;
    if (text == nullptr) { x = std::floor(a.x + (breite - g) * 0.5F); }
    if (s != Sym::Keins) {
        symbol(dl, s, ImVec2{x, std::floor(a.y + (h - g) * 0.5F)}, g, symFarbe != 0 && !gesperrt ? symFarbe : farbe);
        x += g + st.ItemInnerSpacing.x;
    }
    if (text != nullptr) { dl->AddText(ImVec2{x, a.y + st.FramePadding.y}, farbe, text, ImGui::FindRenderedTextEnd(text)); }
    tooltip(hinweis);
    return geklickt;
}

// --- Die Knopfreihe: Teile der Reihe nach, mit Umbruch, wenn die Leiste zu schmal ist ----------------------------------
struct Reihe {
    float x0 = 0.0F;
    float y0 = 0.0F;
    float breite = 0.0F;
    bool zeichnen = false;
    float x = 0.0F;
    int zeile = 0;
    // Platz fuer ein Teil der Breite b (true: zeichnen, der Cursor steht dort)
    bool teil(float b, float abstand) {
        const float h = ImGui::GetFrameHeight();
        if (x > 0.0F && x + abstand + b > breite) {
            ++zeile;
            x = 0.0F;
        }
        if (x > 0.0F) { x += abstand; }
        if (zeichnen) { ImGui::SetCursorScreenPos(ImVec2{x0 + x, y0 + static_cast<float>(zeile) * (h + ImGui::GetStyle().ItemSpacing.y)}); }
        x += b;
        return zeichnen;
    }
    // Die naechsten Teile gehoeren zusammen (- 1.00x +, Dopesheet | Kurveneditor): passen sie nicht mehr gemeinsam in die
    // Zeile, bricht die Reihe vor ihnen um.
    void zusammen(float b, float abstand) {
        if (x > 0.0F && x + abstand + b > breite) {
            ++zeile;
            x = 0.0F;
        }
    }
    [[nodiscard]] float rest() const { return breite - x; }
    [[nodiscard]] float hoehe() const {
        return static_cast<float>(zeile + 1) * ImGui::GetFrameHeight() + static_cast<float>(zeile) * ImGui::GetStyle().ItemSpacing.y;
    }
};

void probeTeil(const char* name) {
    if (!g_probe.transport.empty()) { g_probe.transport += "|"; }
    g_probe.transport += name;
}

std::string hinweisDerFassung(const AnimEntry* e) {
    switch (modellZeitFassung()) {
        case ZeitFassung::Voll: return tr(Str::MzHinweisVoll);
        case ZeitFassung::Sequenz: return tr(Str::MzHinweisSequenz);
        case ZeitFassung::Kurz: break;
    }
    static const Str kBereich[kModellBereiche] = {Str::MbBereichAnsehen, Str::MbBereichModellieren, Str::MbBereichUv,
                                                  Str::MbBereichWeights, Str::MbBereichPainting,    Str::MbBereichAnimation};
    return format(Str::MzHinweisKurz, tr(kBereich[std::clamp(static_cast<int>(mt().bereich), 0, kModellBereiche - 1)]), e != nullptr ? e->name.c_str() : "-");
}

void konfigOeffnen() { g_zeit.konfigOffen = true; }

void spurenUmschalten() {
    ModellTab& t = mt();
    if (modellZeitFassung() == ZeitFassung::Sequenz) {
        g_zeit.sequenzAuf = !g_zeit.sequenzAuf;
    } else {
        t.kvOffen = !t.kvOffen;
        if (t.kvOffen) { t.kvEinpassen = true; }
    }
}

bool spurenOffen() { return modellZeitFassung() == ZeitFassung::Sequenz ? g_zeit.sequenzAuf : mt().kvOffen; }

void setKey() {
    const int i = modellKurvenAktionNach(static_cast<int>(Str::MbKvSetzen));
    if (i >= 0) { modellKurvenAktionAusloesen(i); }
}

// Die Knopfreihe messen (zeichnen = false) oder zeichnen - derselbe Ablauf, damit die Hoehe des Bands zum Umbruch passt.
void transport(const AnimEntry* e, Reihe& r) {
    ModellZeit& z = g_zeit;
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    const float eng = std::floor(fs * 0.3F);
    const float weit = std::floor(fs * 0.62F);
    const bool blender = modellBlender();
    const ZeitFassung fassung = modellZeitFassung();
    const float kb = symKnopfBreite();
    if (r.zeichnen) { ImGui::BeginDisabled(e == nullptr); }
    // 1. Transport
    if (r.teil(kb, 0.0F)) {
        probeTeil("anfang");
        if (knopf(tr(Str::TlRewind), Sym::Anfang, nullptr, tr(blender ? Str::MzTipAnfangBl : Str::MzTipAnfangMax), kb) && e != nullptr) { bildSetzen(*e, 0); }
    }
    if (r.teil(kb, eng)) {
        if (blender) {
            probeTeil("keyzurueck");
            if (knopf(tr(Str::MzKeyZurueck), Sym::KeyZurueck, nullptr, tr(Str::MzTipKeyZurueckBl), kb)) { (void)modellZeitSchluesselSprung(-1); }
        } else {
            probeTeil("bildzurueck");
            const std::string tip = std::string(tr(Str::MzTipZurueckMax)) + (z.keyModus ? std::string("\n") + tr(Str::MzTipKeyModus) : std::string());
            if (knopf(tr(Str::ModelPrevFrame), Sym::BildZurueck, nullptr, tip.c_str(), kb) && e != nullptr) {
                if (z.keyModus) {
                    (void)modellZeitSchluesselSprung(-1);
                } else {
                    bildSetzen(*e, g_app->animFrame - e->firstFrame - 1);
                }
            }
        }
    }
    if (r.teil(kb, eng)) {
        probeTeil("abspielen");
        if (knopf(tr(g_app->animPlaying ? Str::ModelStop : Str::ModelPlay), g_app->animPlaying ? Sym::Pause : Sym::Abspielen, nullptr,
                  tr(blender ? Str::MzTipSpielenBl : Str::MzTipSpielenMax), kb, g_app->animPlaying)) {
            spielenUmschalten(e);
        }
    }
    if (r.teil(kb, eng)) {
        if (blender) {
            probeTeil("keyvor");
            if (knopf(tr(Str::MzKeyVor), Sym::KeyVor, nullptr, tr(Str::MzTipKeyVorBl), kb)) { (void)modellZeitSchluesselSprung(1); }
        } else {
            probeTeil("bildvor");
            const std::string tip = std::string(tr(Str::MzTipVorMax)) + (z.keyModus ? std::string("\n") + tr(Str::MzTipKeyModus) : std::string());
            if (knopf(tr(Str::ModelNextFrame), Sym::BildVor, nullptr, tip.c_str(), kb) && e != nullptr) {
                if (z.keyModus) {
                    (void)modellZeitSchluesselSprung(1);
                } else {
                    bildSetzen(*e, g_app->animFrame - e->firstFrame + 1);
                }
            }
        }
    }
    if (r.teil(kb, eng)) {
        probeTeil("ende");
        if (knopf(tr(Str::MzEnde), Sym::Ende, nullptr, tr(blender ? Str::MzTipEndeBl : Str::MzTipEndeMax), kb) && e != nullptr) { bildSetzen(*e, e->numFrames - 1); }
    }
    // 2. Bild und Zeit. Wie Max ab 0: 12 / 39 heisst Bild 12 von 0..39; wie Blender ab 1: 1 / 40 (der GLA-Frame im Tooltip).
    const int rel = e != nullptr ? g_app->animFrame - e->firstFrame : 0;
    const int ab = versatz();
    const int fps = e != nullptr ? e->fps : 20;
    char jetzt[32];
    char gesamt[64];
    std::snprintf(jetzt, sizeof jetzt, "%d", rel + ab);
    std::snprintf(gesamt, sizeof gesamt, " / %d   %.2f s", e != nullptr ? std::max(e->numFrames - 1 + ab, 0) : 0,
                  static_cast<double>(rel) / static_cast<double>(std::max(std::abs(fps), 1)));
    t.zaehlungText = std::string(jetzt) + gesamt;   // Selbsttest
    if (!blender) {
        const ImVec2 t1 = ImGui::CalcTextSize(jetzt);
        const ImVec2 t2 = ImGui::CalcTextSize(gesamt);
        if (r.teil(std::floor(t1.x + t2.x + fs * 0.8F), weit)) {
            probeTeil("bildanzeige");
            const float hh = ImGui::GetFrameHeight();
            ImGui::Dummy(ImVec2{std::floor(t1.x + t2.x + fs * 0.8F), hh});
            const ImVec2 a = ImGui::GetItemRectMin();
            const ImVec2 b = ImGui::GetItemRectMax();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
            dl->AddText(ImVec2{a.x + fs * 0.4F, a.y + (hh - t1.y) * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), jetzt);
            dl->AddText(ImVec2{a.x + fs * 0.4F + t1.x, a.y + (hh - t1.y) * 0.5F}, ImGui::GetColorU32(ImGuiCol_TextDisabled), gesamt);
            if (e != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip(tr(Str::MbGlaBild), g_app->animFrame, rel); }
        }
        // 3. Auto Key (N), Set Key (K), Zeitkonfiguration
        const std::string autoText = std::string(tr(Str::MzAutoKey)) + " (N)";
        const float autoB = textKnopfBreite(autoText.c_str(), false);
        if (r.teil(autoB, weit)) {
            probeTeil("autokey");
            if (knopf(tr(Str::MzAutoKey), Sym::Keins, autoText.c_str(), tr(Str::MzAutoKeyHint), autoB, z.autoKey, kAufnahme)) { modellZeitAutoKey(!z.autoKey); }
        }
        const std::string setText = std::string(tr(Str::MzSetKey)) + " (K)";
        const float setB = textKnopfBreite(setText.c_str(), false);
        if (r.teil(setB, eng)) {
            probeTeil("setkey");
            if (knopf(tr(Str::MzSetKey), Sym::Keins, setText.c_str(), tr(Str::MzSetKeyHint), setB, !z.offen.empty())) { setKey(); }
        }
        const std::string konfText = format(g_app->settings.modellZeitAnzeige == 1 ? Str::MzFpsSekunden : Str::MzFpsFrames, fps);
        const float konfB = textKnopfBreite(konfText.c_str(), true);
        if (r.teil(konfB, eng)) {
            probeTeil("zeitkonfig");
            if (r.zeichnen) { ImGui::EndDisabled(); }
            if (knopf(tr(Str::MzZeitKonfig), Sym::Uhr, konfText.c_str(), (std::string(tr(Str::MzZeitKonfig)) + "\n" + tr(Str::MzZeitKonfigHint)).c_str(), konfB)) { konfigOeffnen(); }
            if (r.zeichnen) { ImGui::BeginDisabled(e == nullptr); }
        }
    } else {
        // wie Blender: Aufnahme (Auto-Keying), Start / Ende / Bild, Bildrate
        if (r.teil(kb, weit)) {
            probeTeil("aufnahme");
            if (knopf(tr(Str::MzAutoKeying), Sym::Aufnahme, nullptr, (std::string(tr(Str::MzAutoKeying)) + "\n" + tr(Str::MzAutoKeyHint)).c_str(), kb, false, 0,
                      z.autoKey ? kAufnahme : ImGui::GetColorU32(ImGuiCol_TextDisabled))) {
                modellZeitAutoKey(!z.autoKey);
            }
        }
        const float feldB = std::floor(fs * 3.4F);
        const auto feld = [&](const char* id, Str text, int wert, int lo, int hi, bool aktiv, const std::string& hinweis, const char* name, float davor, int& neu) {
            const float tb = ImGui::CalcTextSize(tr(text)).x;
            bool fertig = false;
            if (r.teil(std::floor(tb + ImGui::GetStyle().ItemInnerSpacing.x + feldB), davor)) {
                probeTeil(name);
                ImGui::AlignTextToFramePadding();
                cjkTextUnformatted(tr(text));
                ImGui::SameLine(0.0F, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(feldB);
                ImGui::BeginDisabled(!aktiv);
                int v = wert;
                if (ImGui::InputInt(id, &v, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit()) {
                    neu = std::clamp(v, lo, hi);
                    fertig = neu != wert;
                }
                ImGui::EndDisabled();
                tooltip(hinweis.c_str());
            }
            return fertig;
        };
        const bool da = cfgDa(e);
        const int n = e != nullptr ? std::max(e->numFrames, 1) : 1;
        int neu = 0;
        // Start und Ende sind Anfang und Anzahl der Sequenz in der animation.cfg (wie der Bildbereich der Szene in Blender)
        if (feld("##mzstart", Str::MzStart, ab, ab, n - 1 + ab, da, tr(Str::MzStartEndeHint), "start", weit, neu) && e != nullptr) {
            AnimEntry w = *e;
            w.firstFrame = e->firstFrame + (neu - ab);
            w.numFrames = n - (neu - ab);
            (void)modellMarkerAendern(w);
        }
        if (feld("##mzende", Str::MzEndeFeld, n - 1 + ab, ab, 65535, da, tr(Str::MzStartEndeHint), "ende", eng, neu) && e != nullptr) {
            AnimEntry w = *e;
            w.numFrames = std::max(neu - ab + 1, 1);
            (void)modellMarkerAendern(w);
        }
        if (feld("##mzbild", Str::MzBild, rel + ab, ab, n - 1 + ab, e != nullptr, format(Str::MzBildHint, g_app->animFrame), "bild", weit, neu) && e != nullptr) {
            bildSetzen(*e, neu - ab);
        }
        const std::string konfText = format(Str::MzFpsCfg, fps);
        const float konfB = textKnopfBreite(konfText.c_str(), false);
        if (r.teil(konfB, weit)) {
            probeTeil("zeitkonfig");
            if (r.zeichnen) { ImGui::EndDisabled(); }
            if (knopf(tr(Str::MzZeitKonfig), Sym::Keins, konfText.c_str(), (std::string(tr(Str::MzZeitKonfig)) + "\n" + tr(Str::MzZeitKonfigHint)).c_str(), konfB)) { konfigOeffnen(); }
            if (r.zeichnen) { ImGui::BeginDisabled(e == nullptr); }
        }
    }
    // 4. Ansehen/Pruefen: Tempo, Interpolation, Schleife erzwingen (in den anderen Fassungen im Menue der Zeitleiste)
    if (fassung == ZeitFassung::Sequenz) {
        const float kl = std::floor(ImGui::GetFrameHeight());
        r.zusammen(kl + eng + std::floor(ImGui::CalcTextSize("0.00\xc3\x97").x) + eng + kl, weit);
        if (r.teil(kl, weit)) {
            probeTeil("langsamer");
            if (knopf(tr(Str::MzLangsamer), Sym::Minus, nullptr, tr(Str::MzLangsamer), kl)) { tempoSchritt(-1); }
        }
        char tempo[24];
        std::snprintf(tempo, sizeof tempo, "%.2f\xc3\x97", static_cast<double>(g_app->animSpeed));
        const float tempoB = std::floor(ImGui::CalcTextSize("0.00\xc3\x97").x);
        if (r.teil(tempoB, eng)) {
            probeTeil("tempo");
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(tempo);
            tooltip(tr(Str::ModelSpeed));
        }
        if (r.teil(kl, eng)) {
            probeTeil("schneller");
            if (knopf(tr(Str::MzSchneller), Sym::Plus, nullptr, tr(Str::MzSchneller), kl)) { tempoSchritt(1); }
        }
        const float ipB = textKnopfBreite(tr(Str::MbInterpolation), false);
        if (r.teil(ipB, weit)) {
            probeTeil("interpolation");
            if (knopf(tr(Str::MbInterpolation), Sym::Keins, tr(Str::MbInterpolation), tr(Str::MbInterpolationHint), ipB, t.interpolieren)) {
                t.interpolieren = !t.interpolieren;
                g_app->modelDirty = true;
            }
        }
        const float slB = textKnopfBreite(tr(Str::MzSchleife), false);
        if (r.teil(slB, eng)) {
            probeTeil("schleife");
            if (knopf(tr(Str::MzSchleife), Sym::Keins, tr(Str::MzSchleife), tr(Str::ModelLoopHint), slB, g_app->loopMode == 1)) { g_app->loopMode = g_app->loopMode == 1 ? 0 : 1; }
        }
    }
    if (r.zeichnen) { ImGui::EndDisabled(); }
    // 5. Spuren (Animation, Ansehen/Pruefen), Dopesheet / Kurveneditor, Menue der Zeitleiste
    if (fassung != ZeitFassung::Kurz) {
        const float spB = textKnopfBreite(tr(Str::MzSpuren), true);
        if (r.teil(spB, weit)) {
            probeTeil("spuren");
            if (knopf(tr(Str::MzSpuren), spurenOffen() ? Sym::Auf : Sym::Zu, tr(Str::MzSpuren), tr(Str::MzSpurenHint), spB)) { spurenUmschalten(); }
        }
    }
    if (fassung == ZeitFassung::Voll && t.kvOffen) {
        const char* ds = tr(blender ? Str::MbKvDsBl : Str::MbKvDsOeffnen);
        const char* ke = tr(blender ? Str::MbKvKeBl : Str::MbKvKeOeffnen);
        const float dsB = textKnopfBreite(ds, false);
        const float keB = textKnopfBreite(ke, false);
        r.zusammen(dsB + 1.0F + keB, eng);
        if (r.teil(dsB, eng)) {
            probeTeil("dopesheet");
            if (knopf("##mzdopesheet", Sym::Keins, ds, tr(Str::MbKvDsHint), dsB, t.kvArt == 0) && t.kvArt != 0) {
                t.kvArt = 0;
                t.kvEinpassen = true;
            }
        }
        if (r.teil(keB, 1.0F)) {
            probeTeil("kurven");
            if (knopf("##mzkurven", Sym::Keins, ke, tr(Str::MbKvKeHint), keB, t.kvArt == 1) && t.kvArt != 1) {
                t.kvArt = 1;
                t.kvEinpassen = true;
            }
        }
    }
    const float mb = std::floor(ImGui::GetFrameHeight());
    if (r.teil(mb, eng)) {
        probeTeil("menue");
        if (knopf(tr(Str::MzMenue), Sym::Menue, nullptr, (std::string(tr(Str::MzMenue)) + "\n" + tr(Str::MzMenueHint)).c_str(), mb)) { z.menueOeffnen = true; }
    }
    // 6. Name der Sequenz und Hinweis der Fassung, soweit der Platz der Zeile reicht (nie umbrochen, nichts abgeschnitten)
    // (die kurze Fassung nennt die Sequenz in ihrem Hinweis)
    if (e != nullptr && fassung != ZeitFassung::Kurz && r.rest() >= weit + ImGui::CalcTextSize(e->name.c_str()).x) {
        if (r.teil(ImGui::CalcTextSize(e->name.c_str()).x, weit)) {
            probeTeil("name");
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(e->name.c_str());
        }
    }
    const std::string hinweis = hinweisDerFassung(e);
    if (r.rest() >= weit + ImGui::CalcTextSize(hinweis.c_str()).x) {
        if (r.teil(ImGui::CalcTextSize(hinweis.c_str()).x, weit)) {
            probeTeil("hinweis");
            ImGui::AlignTextToFramePadding();
            cjkTextDisabled("%s", hinweis.c_str());
        }
    }
}

// --- Masse der Leiste --------------------------------------------------------------------------------------------------
float& spurEinstellung() { return modellBlender() ? g_app->settings.modellZeitSpurHoeheBlender : g_app->settings.modellZeitSpurHoeheMax; }

// Die Hoehe des Spurbereichs: aus den Einstellungen (in Schrifthoehen, je Bedienmodus); der Kurveneditor braucht mehr.
float spurHoehe() {
    const float fs = ImGui::GetFontSize();
    float h = std::clamp(spurEinstellung(), 2.5F, 26.0F) * fs;
    if (mt().kvArt == 1) { h = std::max(h, fs * 12.0F); }
    return std::floor(std::min(h, std::max(ImGui::GetIO().DisplaySize.y * 0.45F, fs * 5.0F)));
}

struct Masse {
    float griff = 0.0F;
    float transport = 0.0F;
    float skala = 0.0F;
    float spuren = 0.0F;    // unter der Skala (0: zu)
    float hinweis = 0.0F;
    float status = 0.0F;
    float luft = 0.0F;      // Abstand zwischen den Teilen
    float gesamt = 0.0F;
};

Masse masse(const AnimEntry* e, float breite) {
    const float fs = ImGui::GetFontSize();
    Masse m;
    m.luft = std::floor(ImGui::GetStyle().ItemSpacing.y);
    m.griff = std::max(7.0F, std::floor(fs * 0.5F));
    Reihe r;
    r.breite = breite > 1.0F ? breite : 1e6F;
    transport(e, r);
    m.transport = r.hoehe();
    m.skala = modellZeitSkalaHoehe();
    switch (modellZeitFassung()) {
        case ZeitFassung::Voll: m.spuren = mt().kvOffen ? spurHoehe() : 0.0F; break;
        case ZeitFassung::Sequenz: m.spuren = g_zeit.sequenzAuf ? 2.0F * ImGui::GetFrameHeight() + m.luft : 0.0F; break;
        case ZeitFassung::Kurz: break;
    }
    m.hinweis = std::floor(fs + 2.0F);
    m.status = std::floor(fs + 6.0F);
    m.gesamt = m.griff + m.transport + m.luft + m.skala + (m.spuren > 0.0F ? m.spuren + m.luft : 0.0F) + m.luft + m.hinweis + m.luft + m.status;
    return m;
}

// Ein Text aus Abschnitten (durch Mittelpunkte getrennt), so weit der Platz reicht; der Rest steht im Tooltip - nichts
// abgeschnitten.
std::string kuerzen(const std::string& ganz, float frei) {
    if (ImGui::CalcTextSize(ganz.c_str()).x <= frei) { return ganz; }
    const std::string trenner = " \xc2\xb7 ";
    std::string teil;
    std::size_t ab = 0;
    while (ab < ganz.size()) {
        const std::size_t bis = ganz.find(trenner, ab);
        const std::string stueck = ganz.substr(ab, bis == std::string::npos ? std::string::npos : bis - ab);
        const std::string probe = (teil.empty() ? stueck : teil + trenner + stueck) + trenner + "...";
        if (ImGui::CalcTextSize(probe.c_str()).x > frei) { break; }
        teil = teil.empty() ? stueck : teil + trenner + stueck;
        if (bis == std::string::npos) { break; }
        ab = bis + trenner.size();
    }
    return teil.empty() ? std::string("...") : teil + trenner + "...";
}

// --- Die Teile der Leiste ----------------------------------------------------------------------------------------------
void griffZeichnen(ImVec2 lo, float breite, const Masse& m) {
    ModellZeit& z = g_zeit;
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    const ZeitFassung fassung = modellZeitFassung();
    ImGui::SetCursorScreenPos(lo);
    ImGui::BeginDisabled(fassung == ZeitFassung::Kurz);
    ImGui::InvisibleButton("##mzgriff", ImVec2{breite, m.griff});
    ImGui::EndDisabled();
    const bool ueber = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    const bool aktiv = ImGui::IsItemActive();
    if ((ueber || aktiv) && fassung != ZeitFassung::Kurz) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); }
    if (ImGui::IsItemActivated()) {
        z.griffZieht = true;
        z.griffMaus = ImGui::GetIO().MousePos.y;
        z.griffHoehe = m.spuren;
    }
    if (z.griffZieht && aktiv) {
        // nach oben ziehen macht den Spurbereich groesser; ganz nach unten zeigt nur die Zeitleiste
        const float h = z.griffHoehe - (ImGui::GetIO().MousePos.y - z.griffMaus);
        if (fassung == ZeitFassung::Voll) {
            const bool auf = h >= fs * 1.5F;
            if (auf != t.kvOffen) {
                t.kvOffen = auf;
                t.kvEinpassen = t.kvEinpassen || auf;
            }
            if (auf) { spurEinstellung() = std::clamp(h / std::max(fs, 1.0F), 2.5F, 26.0F); }
        } else if (fassung == ZeitFassung::Sequenz) {
            z.sequenzAuf = h >= fs * 1.5F;
        }
    }
    if (!aktiv) { z.griffZieht = false; }
    // der sichtbare Griff: ein kurzer Strich in der Mitte (60 px im Entwurf bei 13 px Schrift)
    const float gb = std::floor(fs * 4.6F);
    const float mx = std::floor(lo.x + breite * 0.5F);
    const float my = std::floor(lo.y + m.griff * 0.5F);
    const ImU32 farbe = fassung == ZeitFassung::Kurz ? mischen(ImGuiCol_WindowBg, ImGuiCol_Text, 0.12F)
                                                      : ((ueber || aktiv) ? akzent() : mischen(ImGuiCol_WindowBg, ImGuiCol_Text, 0.3F));
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{mx - gb * 0.5F, my - 1.5F}, ImVec2{mx + gb * 0.5F, my + 1.5F}, farbe, 2.0F);
    merke(g_probe.griff, ImVec2{mx - gb * 0.5F, lo.y}, ImVec2{mx + gb * 0.5F, lo.y + m.griff});
    tooltip(tr(fassung == ZeitFassung::Kurz ? Str::MzGriffKurzHint : Str::MzGriffHint));
}

// Skala ohne Spuren (kurz, Sequenz): Namensspalte leer, Klick und Ziehen setzen das Bild.
void skalaAllein(const AnimEntry* e, ImVec2 lo, float breite, const Masse& m) {
    ModellZeit& z = g_zeit;
    const float nb = modellZeitNamenBreite();
    const ImVec2 a{lo.x + nb, lo.y};
    const ImVec2 b{lo.x + breite, lo.y + m.skala};
    merke(g_probe.namen, lo, ImVec2{a.x, b.y});
    ImGui::SetCursorScreenPos(a);
    ImGui::BeginDisabled(e == nullptr);
    ImGui::InvisibleButton("##mzskala", ImVec2{std::max(b.x - a.x, 1.0F), m.skala}, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    ImGui::EndDisabled();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const int erstes = e != nullptr ? e->firstFrame : 0;
    const int anzahl = e != nullptr ? std::max(e->numFrames, 1) : 0;
    // derselbe Ausschnitt wie das eingepasste Dopesheet: ein Bild Rand links und rechts
    const float von = static_cast<float>(erstes) - 1.0F;
    const float bis = static_cast<float>(erstes + std::max(anzahl, 1));
    modellZeitSkala(dl, a, b, von, bis, erstes, anzahl, e != nullptr ? e->fps : 20);
    if (e == nullptr) { return; }
    if (ImGui::IsItemActivated() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) { z.skalaZieht = true; }
    if (z.skalaZieht) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float bild = von + (ImGui::GetIO().MousePos.x - a.x) / std::max(b.x - a.x, 1.0F) * (bis - von);
            bildSetzen(*e, static_cast<int>(std::lround(bild)) - erstes);
        } else {
            z.skalaZieht = false;
        }
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { z.menueOeffnen = true; }
    modellZeitZeiger(dl, a, b, von, bis, erstes, g_app->animFrame, b.y);
}

// Ansehen/Pruefen: zwei Zeilen wie ModView - Primaer (ganzer Koerper/Beine) und Sekundaer ab dem Sekundaer-Start.
void sequenzZeilen(const AnimEntry* e, ImVec2 lo, float breite, const Masse& m) {
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    const float fh = ImGui::GetFrameHeight();
    merke(g_probe.spuren, lo, ImVec2{lo.x + breite, lo.y + m.spuren});
    char sek[200];
    std::snprintf(sek, sizeof sek, tr(Str::MbSekundaer), t.sekundaerKnochen.c_str());
    const float nb = std::max(modellZeitNamenBreite(), std::floor(std::max(ImGui::CalcTextSize(tr(Str::MzPrimaerLang)).x, ImGui::CalcTextSize(sek).x) + fs));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float g = std::floor(fh * 0.62F);
    const auto zeile = [&](int nr, const char* name, const std::string& wert, bool da, const char* zusatz) {
        const ImVec2 p{lo.x, lo.y + static_cast<float>(nr) * (fh + m.luft * 0.5F)};
        ImGui::SetCursorScreenPos(p);
        ImGui::AlignTextToFramePadding();
        cjkTextUnformatted(name);
        ImGui::SetCursorScreenPos(ImVec2{p.x + nb, p.y});
        if (da) {
            ImGui::Dummy(ImVec2{g, fh});
            uiIcon(dl, UiIcon::Schloss, ImVec2{std::floor(p.x + nb), std::floor(p.y + (fh - g) * 0.5F)}, g, ImGui::GetColorU32(ImGuiCol_PlotHistogram));
            tooltip(tr(Str::MzSperreHint));
            ImGui::SameLine(0.0F, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(wert.c_str());
            if (zusatz != nullptr) {
                ImGui::SameLine(0.0F, fs * 0.4F);
                cjkTextDisabled("\xc2\xb7 %s", zusatz);
            }
        } else {
            ImGui::AlignTextToFramePadding();
            cjkTextDisabled("%s", wert.c_str());
        }
        ++g_probe.spurZeilen;
    };
    zeile(0, tr(Str::MzPrimaerLang), e != nullptr ? e->name : std::string("-"), e != nullptr, nullptr);
    const bool hatSek = t.sekundaer >= 0 && static_cast<std::size_t>(t.sekundaer) < g_app->animList.size();
    zeile(1, sek, hatSek ? g_app->animList[static_cast<std::size_t>(t.sekundaer)].name : std::string(tr(Str::MzSekundaerLeer)), hatSek, tr(Str::MzOberkoerper));
    if (hatSek) {
        ImGui::SameLine(0.0F, fs * 0.8F);
        if (ImGui::SmallButton(tr(Str::MbSekundaerAus))) {
            t.sekundaer = -1;
            g_app->modelDirty = true;
        }
    }
}

void hinweisZeile(ImVec2 lo, float breite) {
    const ModellZeit& z = g_zeit;
    std::string text;
    bool warnung = false;
    if (!z.offen.empty()) {
        text = format(Str::MzOffen, setKeyTaste());
        warnung = true;
    } else {
        const bool blender = modellBlender();
        if (modellZeitFassung() == ZeitFassung::Voll) {
            text = std::string(tr(blender ? Str::MzLegendeBlender : Str::MzLegendeMax)) + " \xc2\xb7 ";
        }
        text += tr(blender ? Str::MbZeitHinweisBlender : Str::MbZeitHinweis);
        if (modellZeitFassung() == ZeitFassung::Voll) {
            if (const modell::RigSchicht* s = modellRigSchichtJetzt(); s != nullptr && !s->leer()) {
                text += std::string(" \xc2\xb7 ") + tr(s->bindeBasis ? Str::MbKvBasisBinde : Str::MbKvBasisQuelle);
            }
        }
    }
    g_probe.hinweisText = text;
    const std::string zeigen = kuerzen(text, breite);
    ImGui::SetCursorScreenPos(lo);
    if (warnung) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
        cjkTextUnformatted(zeigen.c_str());
        ImGui::PopStyleColor();
    } else {
        cjkTextDisabled("%s", zeigen.c_str());
    }
    if (zeigen != text) { tooltip(text.c_str()); }
}

// Die Statuszeile des Model-Tabs (Entwurf statusZeile): Bedienmodus (Klick wechselt) - Tasten des Bereichs - Undo / Suche.
// Die Tasten sind die, die es im Programm gibt (Tabellen maxTasten/blenderTasten und die Aktionen der Bereiche).
void statusZeile(ImVec2 lo, float breite, const Masse& m) {
    ModellZeit& z = g_zeit;
    const bool blender = modellBlender();
    static const Str kMax[kModellBereiche] = {Str::MzStMaxAnsehen, Str::MzStMaxModell, Str::MzStMaxUv, Str::MzStMaxWeights, Str::MzStMaxPaint, Str::MzStMaxAnim};
    static const Str kBl[kModellBereiche] = {Str::MzStBlAnsehen, Str::MzStBlModell, Str::MzStBlUv, Str::MzStBlWeights, Str::MzStBlPaint, Str::MzStBlAnim};
    const int bereich = std::clamp(static_cast<int>(mt().bereich), 0, kModellBereiche - 1);
    const float fs = ImGui::GetFontSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine(ImVec2{lo.x, lo.y}, ImVec2{lo.x + breite, lo.y}, ImGui::GetColorU32(ImGuiCol_Separator));
    merke(g_probe.status, lo, ImVec2{lo.x + breite, lo.y + m.status});
    const float ty = std::floor(lo.y + (m.status - fs) * 0.5F + 1.0F);
    const char* modus = tr(blender ? Str::MbWieBlender : Str::MbWieMax);
    const float modusB = ImGui::CalcTextSize(modus).x;
    ImGui::SetCursorScreenPos(ImVec2{lo.x, lo.y + 1.0F});
    if (ImGui::InvisibleButton("##mzmodus", ImVec2{modusB + fs * 0.4F, m.status - 1.0F})) { z.modusOeffnen = true; }
    const bool ueber = ImGui::IsItemHovered();
    if (ueber) { dl->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_ButtonHovered), 3.0F); }
    dl->AddText(ImVec2{lo.x + fs * 0.2F, ty}, ImGui::GetColorU32(ueber ? ImGuiCol_Text : ImGuiCol_TextDisabled), modus);
    tooltip(tr(Str::MzModusHint));
    const std::string undo = tr(blender ? Str::MzStatusUndoBl : Str::MzStatusUndoMax);
    const std::string tasten = tr(blender ? kBl[bereich] : kMax[bereich]);
    const float luecke = fs * 1.1F;
    const float undoB = ImGui::CalcTextSize(undo.c_str()).x;
    const float frei = breite - (modusB + fs * 0.4F) - luecke - luecke - undoB;
    const std::string zeigen = kuerzen(tasten, std::max(frei, 0.0F));
    const float x1 = lo.x + modusB + fs * 0.4F + luecke;
    ImGui::SetCursorScreenPos(ImVec2{x1, ty});
    cjkTextDisabled("%s", zeigen.c_str());
    if (zeigen != tasten) { tooltip(tasten.c_str()); }
    ImGui::SetCursorScreenPos(ImVec2{x1 + ImGui::CalcTextSize(zeigen.c_str()).x + luecke, ty});
    cjkTextDisabled("%s", undo.c_str());
    g_probe.statusText = std::string(modus) + " | " + tasten + " | " + undo;
    if (z.modusOeffnen) {
        ImGui::OpenPopup("##mzmodusmenue");
        z.modusOeffnen = false;
    }
    if (ImGui::BeginPopup("##mzmodusmenue")) {
        cjkTextDisabled("%s", tr(Str::MbBedienung));
        if (ImGui::MenuItem(tr(Str::MbWieMax), nullptr, !blender)) { modellBedienungSetzen(0); }
        if (ImGui::MenuItem(tr(Str::MbWieBlender), nullptr, blender)) { modellBedienungSetzen(1); }
        ImGui::EndPopup();
    }
}

// --- Aktionen der Zeitleiste: Menue, Suche (X / F3) ---------------------------------------------------------------------
struct ZeitAktion {
    Str text;
    Str hinweis;
    const char* maxTaste;
    const char* blenderTaste;
    void (*tun)();
    bool (*an)();   // Haken (nullptr: kein Umschalter)
    int gruppe;     // 0 Wiedergabe, 1 Schluessel, 2 Ansicht, 3 Marker (animation.cfg)
    int modus;      // -1 beide, 0 nur wie 3ds Max, 1 nur wie Blender
};

const std::vector<ZeitAktion>& zeitAktionen() {
    static const std::vector<ZeitAktion> kListe = {
        {Str::MzEnde, Str::MzEnde, "End", "Shift+Right", [] {
             if (const AnimEntry* e = sequenz()) { bildSetzen(*e, e->numFrames - 1); }
         },
         nullptr, 0, -1},
        {Str::MzSchleife, Str::ModelLoopHint, "", "", [] { g_app->loopMode = g_app->loopMode == 1 ? 0 : 1; }, [] { return g_app->loopMode == 1; }, 0, -1},
        {Str::MbInterpolation, Str::MbInterpolationHint, "", "", [] {
             mt().interpolieren = !mt().interpolieren;
             g_app->modelDirty = true;
         },
         [] { return mt().interpolieren; }, 0, -1},
        {Str::MzLangsamer, Str::ModelSpeed, "", "", [] { tempoSchritt(-1); }, nullptr, 0, -1},
        {Str::MzSchneller, Str::ModelSpeed, "", "", [] { tempoSchritt(1); }, nullptr, 0, -1},
        {Str::MzKeyModus, Str::MzKeyModusHint, "", "", [] { g_zeit.keyModus = !g_zeit.keyModus; }, [] { return g_zeit.keyModus; }, 0, 0},
        {Str::MzAutoKey, Str::MzAutoKeyHint, "N", "", [] { modellZeitAutoKey(!g_zeit.autoKey); }, [] { return g_zeit.autoKey; }, 1, 0},
        {Str::MzAutoKeying, Str::MzAutoKeyHint, "", "", [] { modellZeitAutoKey(!g_zeit.autoKey); }, [] { return g_zeit.autoKey; }, 1, 1},
        {Str::MzKeyZurueck, Str::MzKeyModusHint, "", "Down", [] { (void)modellZeitSchluesselSprung(-1); }, nullptr, 1, -1},
        {Str::MzKeyVor, Str::MzKeyModusHint, "", "Up", [] { (void)modellZeitSchluesselSprung(1); }, nullptr, 1, -1},
        {Str::MzKeying, Str::MzSetKeyHint, "", "K", [] { g_zeit.keyingOeffnen = true; }, nullptr, 1, 1},
        {Str::MzSpuren, Str::MzSpurenHint, "", "", [] {
             if (modellZeitFassung() == ZeitFassung::Kurz) { mt().bereich = ModellBereich::Animation; }
             spurenUmschalten();
         },
         [] { return modellZeitFassung() != ZeitFassung::Kurz && spurenOffen(); }, 2, -1},
        {Str::MzFrames, Str::MzKonfAnzeige, "", "", [] { g_app->settings.modellZeitAnzeige = 0; }, [] { return g_app->settings.modellZeitAnzeige == 0; }, 2, -1},
        {Str::MzSekunden, Str::MzKonfAnzeige, "", "", [] { g_app->settings.modellZeitAnzeige = 1; }, [] { return g_app->settings.modellZeitAnzeige == 1; }, 2, -1},
        {Str::MzFlaechenListe, Str::ModelSurfaceDump, "", "", [] {
             modellFlaechenProtokoll();
             ++g_app->modellFlaechenListen;
         },
         nullptr, 2, -1},
        {Str::MzZeitKonfig, Str::MzZeitKonfigHint, "", "", [] { konfigOeffnen(); }, [] { return g_zeit.konfigOffen; }, 3, -1},
    };
    return kListe;
}

bool aktionPasst(const ZeitAktion& a) { return a.modus < 0 || a.modus == (modellBlender() ? 1 : 0); }

void aktionAusloesen(const ZeitAktion& a) {
    a.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tr(a.text);
    g_app->modelDirty = true;
}

void aktionenAlsMenue(int gruppe) {
    const auto& l = zeitAktionen();
    for (std::size_t i = 0; i < l.size(); ++i) {
        const ZeitAktion& a = l[i];
        if (a.gruppe != gruppe || !aktionPasst(a)) { continue; }
        ImGui::PushID(static_cast<int>(i));
        const char* taste = modellBlender() ? a.blenderTaste : a.maxTaste;
        if (ImGui::MenuItem(tr(a.text), taste[0] != '\0' ? taste : nullptr, a.an != nullptr ? a.an() : false)) { aktionAusloesen(a); }
        ImGui::SetItemTooltip("%s", tr(a.hinweis));
        ImGui::PopID();
    }
}

void kvAktionAlsZeile(Str text) {
    const int i = modellKurvenAktionNach(static_cast<int>(text));
    if (i < 0) { return; }
    const char* taste = modellKurvenAktionTaste(i, modellBlender());
    if (ImGui::MenuItem(modellKurvenAktionText(i), taste[0] != '\0' ? taste : nullptr)) { modellKurvenAktionAusloesen(i); }
    ImGui::SetItemTooltip("%s", modellKurvenAktionHinweis(i));
}

void tempoMenue() {
    if (!ImGui::BeginMenu(tr(Str::ModelSpeed))) { return; }
    for (const float v : kTempi) {
        char n[16];
        std::snprintf(n, sizeof n, "%g\xc3\x97", static_cast<double>(v));
        if (ImGui::MenuItem(n, nullptr, std::fabs(v - g_app->animSpeed) < 0.001F)) { g_app->animSpeed = v; }
    }
    ImGui::EndMenu();
}

void keyingZeilen() {
    kvAktionAlsZeile(Str::MbKvSetzen);
    if (ImGui::MenuItem(tr(Str::MbRgSchluesselLoeschen))) { (void)modellRigSchluesselLoeschen(); }
    ImGui::SetItemTooltip("%s", tr(Str::MbRgSchluesselLoeschenHint));
    aktionenAlsMenue(1);
}

// Das Menue der Zeitleiste: wie Blender in den Gruppen der Timeline (Playback, Keying, View, Marker), wie 3ds Max in
// einer Spalte mit Trennstrichen (Rechtsklick auf Zeitschieber und Spurleiste).
void menueZeichnen() {
    ModellZeit& z = g_zeit;
    if (z.menueOeffnen) {
        ImGui::OpenPopup("##mzmenue");
        z.menueOeffnen = false;
    }
    if (!ImGui::BeginPopup("##mzmenue")) { return; }
    const auto wiedergabe = [] {
        if (ImGui::MenuItem(tr(g_app->animPlaying ? Str::ModelStop : Str::ModelPlay), modellBlender() ? "Space" : "/")) { spielenUmschalten(sequenz()); }
        tempoMenue();
        aktionenAlsMenue(0);
    };
    const auto ansicht = [] {
        aktionenAlsMenue(2);
        kvAktionAlsZeile(Str::MbKvDsOeffnen);
        kvAktionAlsZeile(Str::MbKvKeOeffnen);
        kvAktionAlsZeile(Str::MbKvNurAnimierte);
        kvAktionAlsZeile(Str::MbKvNurGewaehlte);
        kvAktionAlsZeile(Str::MbKvAllesZeigen);
    };
    const auto marker = [] {
        aktionenAlsMenue(3);
        kvAktionAlsZeile(Str::MbKvMkGrenzen);
        kvAktionAlsZeile(Str::MbKvMkSchleife);
        kvAktionAlsZeile(Str::MbKvMkWaehlen);
    };
    if (modellBlender()) {
        if (ImGui::BeginMenu(tr(Str::MzMenuePlayback))) {
            wiedergabe();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(tr(Str::MzKeying))) {
            keyingZeilen();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(tr(Str::MzMenueView))) {
            ansicht();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(tr(Str::MzMenueMarker))) {
            marker();
            ImGui::EndMenu();
        }
    } else {
        wiedergabe();
        ImGui::Separator();
        keyingZeilen();
        ImGui::Separator();
        ansicht();
        ImGui::Separator();
        marker();
    }
    ImGui::Separator();
    modellKurvenMenue();   // Animationswerkzeuge (Schluessel, Interpolation, Tangenten, Timing, Aufraeumen ...)
    ImGui::EndPopup();
}

// Blender K: das Keying-Menue (Schluessel setzen, loeschen, Auto-Keying, Spruenge)
void keyingMenue() {
    ModellZeit& z = g_zeit;
    if (z.keyingOeffnen) {
        ImGui::OpenPopup("##mzkeying");
        z.keyingOeffnen = false;
    }
    if (!ImGui::BeginPopup("##mzkeying")) { return; }
    cjkTextDisabled("%s", tr(Str::MzKeying));
    keyingZeilen();
    ImGui::EndPopup();
}

// Zeitkonfiguration (3ds Max: Time Configuration): Bildrate, Zeitanzeige, Wiedergabe und die Zeile der animation.cfg
// (Anfang, Anzahl, Schleife, fps mit dem Bereichsbalken) - bis rc611 die vierte Zeile der Leiste.
void konfigFenster(const AnimEntry* e) {
    ModellZeit& z = g_zeit;
    if (!z.konfigOffen) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 46.0F, 0.0F}, ImGuiCond_Always);
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{d.x * 0.5F, d.y * 0.6F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    // UIFORM: jedes Fenster beginnt ueber den Baustein; dieses setzt seine Groesse selbst (NoResize) und bleibt, wie es ist
    if (!ui::fensterBegin((std::string(tr(Str::MzZeitKonfig)) + "###mzkonfig").c_str(), &z.konfigOffen,
                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize)) {
        ImGui::End();
        return;
    }
    modellKlickPruefen();
    ImGui::SeparatorText(tr(Str::MzKonfBildrate));
    cjkTextUnformatted(format(Str::MzKonfBildrateText, e != nullptr ? e->fps : 0).c_str());
    ImGui::SeparatorText(tr(Str::MzKonfAnzeige));
    if (ImGui::RadioButton(tr(Str::MzFrames), g_app->settings.modellZeitAnzeige == 0)) { g_app->settings.modellZeitAnzeige = 0; }
    ImGui::SameLine();
    if (ImGui::RadioButton(tr(Str::MzSekunden), g_app->settings.modellZeitAnzeige == 1)) { g_app->settings.modellZeitAnzeige = 1; }
    ImGui::SeparatorText(tr(Str::MzKonfWiedergabe));
    ImGui::AlignTextToFramePadding();
    cjkTextUnformatted(tr(Str::ModelSpeed));
    for (const float v : kTempi) {
        char n[24];
        std::snprintf(n, sizeof n, "%g\xc3\x97##mzt", static_cast<double>(v));
        ImGui::SameLine();
        if (ImGui::RadioButton(n, std::fabs(v - g_app->animSpeed) < 0.001F)) { g_app->animSpeed = v; }
    }
    bool schleife = g_app->loopMode == 1;
    if (ImGui::Checkbox((std::string(tr(Str::MzSchleife)) + "##mzk").c_str(), &schleife)) { g_app->loopMode = schleife ? 1 : 0; }
    ImGui::SetItemTooltip("%s", tr(Str::ModelLoopHint));
    ImGui::SameLine();
    if (ImGui::Checkbox((std::string(tr(Str::MbInterpolation)) + "##mzk").c_str(), &mt().interpolieren)) { g_app->modelDirty = true; }
    ImGui::SetItemTooltip("%s", tr(Str::MbInterpolationHint));
    if (!modellBlender()) {
        ImGui::SameLine();
        ImGui::Checkbox(tr(Str::MzKeyModus), &z.keyModus);
        ImGui::SetItemTooltip("%s", tr(Str::MzKeyModusHint));
    }
    bool autoKey = z.autoKey;
    if (ImGui::Checkbox((std::string(tr(modellBlender() ? Str::MzAutoKeying : Str::MzAutoKey)) + "##mzk").c_str(), &autoKey)) { modellZeitAutoKey(autoKey); }
    ImGui::SetItemTooltip("%s", tr(Str::MzAutoKeyHint));
    ImGui::SeparatorText(tr(Str::MzKonfCfg));
    if (e == nullptr) {
        cjkTextDisabled("%s", tr(Str::MzKonfKeineSequenz));
    } else {
        cjkTextDisabled("%s", tr(Str::MbPrimaer));   // die Primaersequenz: fuer sie gelten die Werte
        ImGui::SameLine();
        ImGui::TextUnformatted(e->name.c_str());
        ImGui::SameLine();
        cjkTextDisabled("%s", format(Str::MzKonfBereich, e->firstFrame, e->firstFrame + std::max(e->numFrames - 1, 0), e->numFrames,
                                     static_cast<double>(e->numFrames) / static_cast<double>(std::max(std::abs(e->fps), 1)))
                                  .c_str());
        if (!cfgDa(e)) {
            ImGui::PushTextWrapPos(0.0F);
            cjkTextDisabled("%s", tr(Str::MzKonfKeineCfg));
            ImGui::PopTextWrapPos();
        }
    }
    modellMarkerZeile(e);
    ImGui::PushTextWrapPos(0.0F);
    cjkTextDisabled("%s", tr(modellBlender() ? Str::MbZeitHinweisBlender : Str::MbZeitHinweis));
    ImGui::PopTextWrapPos();
    ImGui::End();
}

}  // namespace

// --- oeffentlich -------------------------------------------------------------------------------------------------------

ModellZeit& modellZeit() { return g_zeit; }

ZeitFassung modellZeitFassung() {
    switch (mt().bereich) {
        case ModellBereich::Animation: return ZeitFassung::Voll;
        case ModellBereich::Ansehen: return ZeitFassung::Sequenz;
        default: return ZeitFassung::Kurz;
    }
}

float modellZeitNamenBreite() { return std::floor(ImGui::GetFontSize() * 12.0F); }
float modellZeitSkalaHoehe() { return std::floor(ImGui::GetFontSize() * 2.6F); }
float modellZeitSpurZeile() { return std::floor(ImGui::GetFontSize() * 1.3F); }
float modellZeitSpurAbstand() { return 2.0F; }
float modellZeitSchluesselY(float skalaOben) { return std::floor(skalaOben + ImGui::GetFontSize() * 0.78F); }
float modellZeitSchluesselZone(float skalaOben) { return std::floor(skalaOben + ImGui::GetFontSize() * 1.42F); }

// Die Skala (Entwurf .skala): Teilstriche mit Zahlen, wie 3ds Max dazu ein feiner Strich je Frame; ausserhalb der Sequenz
// gedaempft. Die Schluessel und den Abspielkopf zeichnet der Aufrufer darueber.
void modellZeitSkala(ImDrawList* dl, ImVec2 lo, ImVec2 hi, float von, float bis, int erstes, int anzahl, int fps) {
    const float fs = ImGui::GetFontSize();
    dl->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg), 3.0F);
    merke(g_probe.skala, lo, hi);
    const float px = (hi.x - lo.x) / std::max(bis - von, 1e-3F);
    if (px > 1e-4F && std::isfinite(px) && anzahl > 0) {
        const auto xVon = [&](float b) { return lo.x + (b - von) * px; };
        g_probe.bildX0 = xVon(static_cast<float>(erstes));
        g_probe.bildX1 = xVon(static_cast<float>(erstes + anzahl - 1));
        dl->PushClipRect(lo, hi, true);
        const int b0 = std::max(erstes, static_cast<int>(std::ceil(von)));
        const int b1 = std::min(erstes + anzahl - 1, static_cast<int>(std::floor(bis)));
        if (!modellBlender()) {
            // shank 04.11. Bild 3: "striche ... als frames wie in 3ds max" - je Frame einer, bei wenig Platz je n-tem
            const int n = schrittFuer(px, 4.0F);
            const ImU32 fein = mischen(ImGuiCol_FrameBg, ImGuiCol_Text, 0.13F);
            for (int b = b0; b <= b1; ++b) {
                if ((b - erstes) % n != 0) { continue; }
                const float x = std::floor(xVon(static_cast<float>(b))) + 0.5F;
                dl->AddLine(ImVec2{x, lo.y}, ImVec2{x, hi.y}, fein);
                ++g_probe.frameStriche;
            }
        }
        const int schritt = schrittFuer(px, fs * 4.0F);
        const ImU32 strich = mischen(ImGuiCol_FrameBg, ImGuiCol_Text, 0.3F);
        const ImU32 zahlFarbe = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImFont* font = ImGui::GetFont();
        const float klein = std::floor(fs * 0.82F);
        const float zy = modellZeitSchluesselZone(lo.y);
        float letzteZahl = -1e9F;
        const auto teil = [&](int b) {
            const float x = std::floor(xVon(static_cast<float>(b))) + 0.5F;
            dl->AddLine(ImVec2{x, lo.y}, ImVec2{x, hi.y}, strich);
            const std::string z = zeitZahl(b - erstes, fps);
            const float w = font->CalcTextSizeA(klein, FLT_MAX, 0.0F, z.c_str()).x;
            const float tx = std::clamp(std::floor(x - w * 0.5F), lo.x + 2.0F, std::max(hi.x - w - 2.0F, lo.x + 2.0F));
            dl->AddText(font, klein, ImVec2{tx, zy}, zahlFarbe, z.c_str());
            letzteZahl = x;
            ++g_probe.teilstriche;
        };
        for (int b = b0; b <= b1; ++b) {
            if ((b - erstes) % schritt == 0) { teil(b); }
        }
        // das letzte Bild der Sequenz traegt seine Zahl, wenn sie neben die vorige passt (Entwurf: 0 ... 119)
        const int letztes = erstes + anzahl - 1;
        if (letztes <= b1 && (letztes - erstes) % schritt != 0 && xVon(static_cast<float>(letztes)) - letzteZahl >= fs * 2.4F) { teil(letztes); }
        const ImU32 daempfen = ImGui::GetColorU32(ImGuiCol_WindowBg, 0.4F);
        if (const float x = xVon(static_cast<float>(erstes) - 0.5F); x > lo.x) { dl->AddRectFilled(lo, ImVec2{std::min(x, hi.x), hi.y}, daempfen); }
        if (const float x = xVon(static_cast<float>(erstes + anzahl) - 0.5F); x < hi.x) { dl->AddRectFilled(ImVec2{std::max(x, lo.x), lo.y}, hi, daempfen); }
        dl->PopClipRect();
    }
    dl->AddRect(lo, hi, ImGui::GetColorU32(ImGuiCol_Border), 3.0F);
}

void modellZeitNamenText(ImDrawList* dl, ImVec2 skalaLinksOben, const char* text) {
    const float fs = ImGui::GetFontSize();
    dl->AddText(ImVec2{skalaLinksOben.x, std::floor(skalaLinksOben.y + (modellZeitSkalaHoehe() - fs) * 0.5F)}, ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
    g_probe.namenText = text;
}

// Der Grund einer Spur (Entwurf .spuren .sp): eine Flaeche, wie 3ds Max dahinter ein feiner Strich je Frame.
void modellZeitSpurGrund(ImDrawList* dl, ImVec2 lo, ImVec2 hi, float von, float bis, int erstes, int anzahl) {
    dl->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg), 2.0F);
    ++g_probe.spurZeilen;
    const float px = (hi.x - lo.x) / std::max(bis - von, 1e-3F);
    if (!(px > 1e-4F) || !std::isfinite(px) || anzahl <= 0) { return; }
    const auto xVon = [&](float b) { return lo.x + (b - von) * px; };
    if (!modellBlender()) {
        const int n = schrittFuer(px, 4.0F);
        const ImU32 fein = mischen(ImGuiCol_FrameBg, ImGuiCol_Text, 0.13F);
        const int b0 = std::max(erstes, static_cast<int>(std::ceil(von)));
        const int b1 = std::min(erstes + anzahl - 1, static_cast<int>(std::floor(bis)));
        for (int b = b0; b <= b1; ++b) {
            if ((b - erstes) % n != 0) { continue; }
            const float x = std::floor(xVon(static_cast<float>(b))) + 0.5F;
            dl->AddLine(ImVec2{x, lo.y}, ImVec2{x, hi.y}, fein);
            ++g_probe.spurStriche;
        }
    }
    const ImU32 daempfen = ImGui::GetColorU32(ImGuiCol_WindowBg, 0.4F);
    if (const float x = xVon(static_cast<float>(erstes) - 0.5F); x > lo.x) { dl->AddRectFilled(lo, ImVec2{std::min(x, hi.x), hi.y}, daempfen); }
    if (const float x = xVon(static_cast<float>(erstes + anzahl) - 0.5F); x < hi.x) { dl->AddRectFilled(ImVec2{std::max(x, lo.x), lo.y}, hi, daempfen); }
}

// Ein Schluessel: wie 3ds Max ein Strich in den Farben der Spurleiste (Rot = Bewegung, Gruen = Drehung, beides: oben rot,
// unten gruen), wie Blender eine Raute. Eigene Zeichnung.
void modellZeitSchluessel(ImDrawList* dl, float x, float yMitte, float hoehe, unsigned art, bool gewaehlt, bool inSkala) {
    const float fs = ImGui::GetFontSize();
    if (modellBlender()) {
        const float r = std::floor(hoehe * 0.5F);
        const ImU32 innen = gewaehlt ? IM_COL32(255, 255, 255, 255) : (inSkala ? IM_COL32(236, 168, 62, 255) : IM_COL32(122, 172, 255, 255));
        const ImU32 rand = gewaehlt ? IM_COL32(236, 140, 30, 255) : IM_COL32(20, 22, 28, 255);
        const ImVec2 m{std::floor(x) + 0.5F, std::floor(yMitte) + 0.5F};
        dl->AddQuadFilled(ImVec2{m.x, m.y - r}, ImVec2{m.x + r, m.y}, ImVec2{m.x, m.y + r}, ImVec2{m.x - r, m.y}, innen);
        dl->AddQuad(ImVec2{m.x, m.y - r}, ImVec2{m.x + r, m.y}, ImVec2{m.x, m.y + r}, ImVec2{m.x - r, m.y}, rand, gewaehlt ? 1.5F : 1.0F);
        ++g_probe.rauten;
        return;
    }
    const float b = std::max(3.0F, std::floor(fs * 0.38F));
    const float x0 = std::floor(x - b * 0.5F);
    const float y0 = std::floor(yMitte - hoehe * 0.5F);
    const float y1 = y0 + std::floor(hoehe);
    const bool rot = (art & 1U) != 0U;
    const bool gruen = (art & 2U) != 0U || !rot;
    if (rot && gruen) {
        const float ym = std::floor((y0 + y1) * 0.5F);
        dl->AddRectFilled(ImVec2{x0, y0}, ImVec2{x0 + b, ym}, kRot);
        dl->AddRectFilled(ImVec2{x0, ym}, ImVec2{x0 + b, y1}, kGruen);
        ++g_probe.stricheZwei;
    } else {
        dl->AddRectFilled(ImVec2{x0, y0}, ImVec2{x0 + b, y1}, rot ? kRot : kGruen, 1.0F);
        ++(rot ? g_probe.stricheRot : g_probe.stricheGruen);
    }
    if (gewaehlt) { dl->AddRect(ImVec2{x0 - 1.0F, y0 - 1.0F}, ImVec2{x0 + b + 1.0F, y1 + 1.0F}, ImGui::GetColorU32(ImGuiCol_Text), 1.0F); }
    ++g_probe.striche;
}

// Der Abspielkopf (Entwurf .kopfz): eine Linie in der Akzentfarbe durch Skala und Spuren, oben das Kaestchen mit der Zahl.
void modellZeitZeiger(ImDrawList* dl, ImVec2 lo, ImVec2 hi, float von, float bis, int erstes, int bild, float spurenUnten) {
    const float fs = ImGui::GetFontSize();
    const float px = (hi.x - lo.x) / std::max(bis - von, 1e-3F);
    const float x = std::floor(lo.x + (static_cast<float>(bild) - von) * px) + 0.5F;
    if (x < lo.x - 1.0F || x > hi.x + 1.0F) { return; }
    const ImU32 farbe = akzent();
    dl->AddLine(ImVec2{x, lo.y}, ImVec2{x, hi.y + 1.0F}, farbe, 2.0F);
    ++g_probe.kopfLinien;
    if (spurenUnten > hi.y + 2.0F) {
        dl->AddLine(ImVec2{x, hi.y + 1.0F}, ImVec2{x, spurenUnten}, farbe, 1.0F);
        ++g_probe.kopfLinien;
    }
    const AnimEntry* e = sequenz();
    const std::string z = zeitZahl(bild - erstes, e != nullptr ? e->fps : 20);
    ImFont* font = ImGui::GetFont();
    const float klein = std::floor(fs * 0.85F);
    const ImVec2 tg = font->CalcTextSizeA(klein, FLT_MAX, 0.0F, z.c_str());
    const float bb = std::floor(tg.x + fs * 0.6F);
    const float bx = std::clamp(std::floor(x - bb * 0.5F), lo.x, std::max(hi.x - bb, lo.x));
    const ImVec2 a{bx, lo.y};
    const ImVec2 b{bx + bb, lo.y + std::floor(klein + 2.0F)};
    dl->AddRectFilled(a, b, farbe, 3.0F);
    dl->AddText(font, klein, ImVec2{std::floor(bx + (bb - tg.x) * 0.5F), a.y + 1.0F}, IM_COL32_WHITE, z.c_str());
    merke(g_probe.kopf, a, b);
    g_probe.kopfText = z;
}

float modellZeitleisteHoehe() {
    const AnimEntry* e = sequenz();
    return masse(e, g_zeit.breite).gesamt;
}

void modellZeitAutoKey(bool an) {
    ModellZeit& z = g_zeit;
    g_app->settings.modellZeitAutoKey = an;   // ENTSCHEID1: die Wahl bleibt (behaved.cfg modelTimelineAutoKey)
    if (z.autoKey == an) { return; }
    z.autoKey = an;
    // beim Einschalten bleibt eine Pose ohne Schluessel, wie sie ist: ihre Schluessel stehen schon am Bild
    if (an && !z.offen.empty()) {
        z.offen.clear();
        ++z.uebernommen;
    }
    melden(an ? std::string(tr(Str::MzAutoKeyAn)) : format(Str::MzAutoKeyAus, setKeyTaste()));
}

void modellZeitZugGemeldet(std::uint64_t schritt) {
    ModellZeit& z = g_zeit;
    if (z.autoKey || schritt == 0) { return; }
    const AnimEntry* e = sequenz();
    z.offen.push_back(schritt);
    z.offenBild = g_app->animFrame;
    z.offenSequenz = e != nullptr ? e->name : std::string();
    z.offenStand = mt().dok.stand();
    z.zugBild = ImGui::GetFrameCount();
}

bool modellZeitOffenUebernehmen() {
    ModellZeit& z = g_zeit;
    offenPruefen();
    if (z.offen.empty()) { return false; }
    z.offen.clear();
    ++z.uebernommen;
    const AnimEntry* e = sequenz();
    melden(format(Str::MzOffenGesetzt, g_app->animFrame - (e != nullptr ? e->firstFrame : 0) + versatz()));
    return true;
}

bool modellZeitSchluesselSprung(int richtung) {
    const AnimEntry* e = sequenz();
    if (e == nullptr) { return false; }
    const std::vector<int> bilder = modellKvSchluesselBilder();
    int ziel = -1;
    if (richtung > 0) {
        for (const int b : bilder) {
            if (b > g_app->animFrame) {
                ziel = b;
                break;
            }
        }
    } else {
        for (auto it = bilder.rbegin(); it != bilder.rend(); ++it) {
            if (*it < g_app->animFrame) {
                ziel = *it;
                break;
            }
        }
    }
    if (ziel < 0) {
        melden(tr(Str::MzKeinSchluessel));
        return false;
    }
    bildSetzen(*e, ziel - e->firstFrame);
    ++g_zeit.spruenge;
    return true;
}

// Die Tasten der Zeitleiste (Aufruf in modellTasten vor den Tabellen der Modi). Wie 3ds Max: N Auto Key; mit Key-Modus
// springen , und . von Schluessel zu Schluessel. Wie Blender: Pfeil hoch/runter zum naechsten/vorigen Schluessel
// (screen.keyframe_jump), K das Keying-Menue im Bereich Animation, Pos1 ueber dem Spurbereich zeigt alles (view_all).
bool modellZeitTasten() {
    ModellZeit& z = g_zeit;
    ModellTab& t = mt();
    const auto gezaehlt = [&](Str text) {
        ++t.tastenZaehler;
        t.letzteTaste = tr(text);
        g_app->modelDirty = true;
        return true;
    };
    if (!modellBlender()) {
        if (ImGui::IsKeyChordPressed(ImGuiKey_N)) {
            modellZeitAutoKey(!z.autoKey);
            return gezaehlt(Str::MzAutoKey);
        }
        if (z.keyModus && ImGui::IsKeyChordPressed(ImGuiKey_Comma)) {
            (void)modellZeitSchluesselSprung(-1);
            return gezaehlt(Str::MzKeyZurueck);
        }
        if (z.keyModus && ImGui::IsKeyChordPressed(ImGuiKey_Period)) {
            (void)modellZeitSchluesselSprung(1);
            return gezaehlt(Str::MzKeyVor);
        }
        return false;
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_UpArrow)) {
        (void)modellZeitSchluesselSprung(1);
        return gezaehlt(Str::MzKeyVor);
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_DownArrow)) {
        (void)modellZeitSchluesselSprung(-1);
        return gezaehlt(Str::MzKeyZurueck);
    }
    if (t.bereich == ModellBereich::Animation && ImGui::IsKeyChordPressed(ImGuiKey_K)) {
        z.keyingOeffnen = true;
        return gezaehlt(Str::MzKeying);
    }
    if (t.bereich == ModellBereich::Animation && t.kvHoverVorher && ImGui::IsKeyChordPressed(ImGuiKey_Home)) {
        t.kvEinpassen = true;
        return gezaehlt(Str::MbKvAllesZeigen);
    }
    return false;
}

bool modellZeitAktionenSuche(const std::string& such) {
    for (const ZeitAktion& a : zeitAktionen()) {
        if (!aktionPasst(a)) { continue; }
        const char* taste = modellBlender() ? a.blenderTaste : a.maxTaste;
        std::string text = std::string(tr(Str::MzGruppe)) + ": " + tr(a.text);
        if (taste[0] != '\0') { text += std::string(" (") + taste + ")"; }
        std::string k = text;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (!such.empty() && k.find(such) == std::string::npos) { continue; }
        if (ImGui::Selectable(text.c_str())) {
            aktionAusloesen(a);
            return true;
        }
        ImGui::SetItemTooltip("%s", tr(a.hinweis));
    }
    return false;
}

// Z20: Haut und Flaechenzaehler stehen nicht mehr in der Zeitleiste, sondern in der Gruppe "Detailstufe und Skin" (AP5: modellPrHaeute,
// gui/app_modelltab_pruefen.cpp).

// Die Leiste unter allen Spalten, in jedem Bereich (shank 04.11.: "und in jedem Tab sieht man die animation?").
void drawModellZeitleiste(const AnimEntry* e) {
    ModellZeit& z = g_zeit;
    ModellTab& t = mt();
    if (!z.einstellungenGelesen) {
        t.kvOffen = g_app->settings.modellZeitSpuren;   // ab Werk offen: im Bereich Animation stehen die Spuren da (Entwurf)
        z.autoKey = g_app->settings.modellZeitAutoKey;  // ENTSCHEID1: ab Werk aus, sonst die Wahl des Anwenders
        z.einstellungenGelesen = true;
    }
    modellCfgAbgleich();   // die animation.cfg zur GLA (bis rc611 in der Marker-Zeile der Leiste)
    // Nach einer Aenderung des Dokuments setzt der Abgleich die animList neu (cfgAbgleich, app_modelltab_mb3.cpp); der Zeiger des
    // Aufrufers zeigte dann in die freigegebene alte Liste (Absturz in transport, Lauf ap6k_mb_ap7) - die Sequenz neu holen.
    e = sequenz();
    // Selbsttest: zeigt die Sequenz noch in die Liste, die gerade gilt?
    if (e != nullptr) {
        const auto p = reinterpret_cast<std::uintptr_t>(e);
        const auto a = reinterpret_cast<std::uintptr_t>(g_app->animList.data());
        if (p < a || p >= a + g_app->animList.size() * sizeof(AnimEntry)) { ++z.veralteteSequenz; }
    }
    offenPruefen();
    g_probe = ModellZeitProbe{};
    g_probe.bild = ImGui::GetFrameCount();
    g_probe.fassung = static_cast<int>(modellZeitFassung());
    g_probe.blender = modellBlender();
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    const float breite = std::max(ImGui::GetContentRegionAvail().x, 64.0F);
    const Masse m = masse(e, breite);
    z.breite = breite;
    merke(g_probe.band, lo, ImVec2{lo.x + breite, lo.y + m.gesamt});
    float y = lo.y;
    griffZeichnen(ImVec2{lo.x, y}, breite, m);
    y += m.griff;
    {
        Reihe r;
        r.x0 = lo.x;
        r.y0 = y;
        r.breite = breite;
        r.zeichnen = true;
        transport(e, r);
        g_probe.transportZeilen = r.zeile + 1;
    }
    y += m.transport + m.luft;
    if (modellZeitFassung() == ZeitFassung::Voll) {
        // Skala und Spuren zeichnet das Dopesheet (ein Lineal, Bild 0 ueberall an derselben Stelle); zu = nur die Skala
        ImGui::SetCursorScreenPos(ImVec2{lo.x, y});
        drawModellSpurbereich(ImVec2{breite, m.skala + (m.spuren > 0.0F ? m.luft + m.spuren : 0.0F)});
        merke(g_probe.spuren, ImVec2{lo.x, y + m.skala + m.luft}, ImVec2{lo.x + breite, y + m.skala + m.luft + m.spuren});
        merke(g_probe.namen, ImVec2{lo.x, y}, ImVec2{lo.x + modellZeitNamenBreite(), y + m.skala});
        y += m.skala + (m.spuren > 0.0F ? m.luft + m.spuren : 0.0F);
    } else {
        skalaAllein(e, ImVec2{lo.x, y}, breite, m);
        y += m.skala;
        if (m.spuren > 0.0F) {
            y += m.luft;
            sequenzZeilen(e, ImVec2{lo.x, y}, breite, m);
            y += m.spuren;
        }
    }
    y += m.luft;
    hinweisZeile(ImVec2{lo.x, y}, breite);
    y += m.hinweis + m.luft;
    statusZeile(ImVec2{lo.x, y}, breite, m);
    y += m.status;
    ImGui::SetCursorScreenPos(lo);
    ImGui::Dummy(ImVec2{breite, y - lo.y});
    // Rechtsklick auf die freie Flaeche der Leiste: ihr Menue
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { z.menueOeffnen = true; }
    menueZeichnen();
    keyingMenue();
    sequenzPruefen(e);
    konfigFenster(e);
    if (t.kvOffen != g_app->settings.modellZeitSpuren && modellZeitFassung() == ZeitFassung::Voll) { g_app->settings.modellZeitSpuren = t.kvOffen; }
    z.probe = g_probe;
}

}  // namespace bhed::gui
