// ui_rollout.cpp - der Rollout-Baustein des rechten Felds (AP3 von MBENTWURF Teil B, rc612; Beschreibung in ui_rollout.h)
#include "ui_rollout.h"

#include "app_internal.h"
#include "textumbruch.h"
#include "ui_formular.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace bhed::gui::ui {

namespace {

// So tief duerfen Gruppen ineinander stehen (jede braucht einen eigenen ImDrawListSplitter).
constexpr int kTiefe = 6;

// Was eine Gruppe ueber die Bilder hinweg behaelt: die Spalten ihrer Formularzeilen und Regler (das Breiteste des letzten Bildes).
struct Gruppe {
    float spalte = 0.0F;      // Beschriftungsspalte dieses Bildes
    float spalteNeu = 0.0F;
    float wert = 0.0F;        // Wertspalte der Regler dieses Bildes
    float wertNeu = 0.0F;
    int bild = -1;
};

struct Lauf {
    ImGuiID id = 0;
    ImGuiWindow* fenster = nullptr;
    ImRect kopf;
    bool ueber = false;
    bool gehalten = false;
    float workMax = 0.0F;
    float inhaltMax = 0.0F;
    float einzug = 0.0F;
    ImDrawListSplitter teiler;
    RolloutProbe probe;
    int rasterNr = 0;
    int zeilenNr = 0;
};

struct Raster {
    bool an = false;
    ImGuiWindow* fenster = nullptr;
    ImGuiID kennung = 0;
    int spalten = 1;
    int zelle = 0;
    float x0 = 0.0F;
    float zellB = 0.0F;
    float lueckeX = 0.0F;
    float lueckeY = 0.0F;
    float reiheOben = 0.0F;
    float reiheUnten = 0.0F;
    float reiheNatur = 0.0F;   // die groesste Hoehe, die ein Knopf dieser Reihe von sich aus braucht
    float ganz = 0.0F;         // AP14: die Breite des ganzen Rasters
    float bedarf = 0.0F;       // AP14: so breit muss eine Zelle sein, damit das breiteste Wort ihrer Knoepfe und Schalter passt
};

struct Feld {
    bool an = false;
    ImGuiID schluessel = 0;
    int aktiv = -1;
    int standard = 0;
    int zahl = 0;
    unsigned belegtNeu = 0;
    ImGuiWindow* fenster = nullptr;
};

struct Zustand {
    RolloutStil stil = RolloutStil::Max;
    Lauf lauf[kTiefe];
    int tiefe = 0;
    Raster raster;
    Feld feld;
    std::unordered_map<ImGuiID, Gruppe> gruppen;
    std::unordered_map<ImGuiID, unsigned> belegt;   // je Feld und Kontext: Reiter mit Gruppen im letzten Bild
    std::unordered_map<ImGuiID, float> reihen;      // je Raster und Reihe: Hoehe im letzten Bild
    std::unordered_map<ImGuiID, float> zellBedarf;  // AP14, je Raster: Zellbreite, die das breiteste Wort im letzten Bild brauchte
    float feldB = 0.0F;
    std::vector<RolloutProbe> proben;
    std::vector<RolloutProbe> probenLetztes;
    FeldProbe feldProbe;
    ImGuiID zeigen = 0;        // rolloutZeigen: diese Gruppe beim naechsten Zeichnen aufklappen und ins Bild rollen
    int zeigenBild = -1;
    WortbruchProbe wortbruch;          // AP14: laufendes Bild
    WortbruchProbe wortbruchLetztes;   // letztes vollstaendiges Bild
};

Zustand& z() {
    static Zustand s;
    return s;
}

ImVec4 mischen(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

float helligkeit(const ImVec4& c) { return 0.2126F * c.x + 0.7152F * c.y + 0.0722F * c.z; }

bool hellesThema() { return helligkeit(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)) > 0.5F; }

// Die Betonungsfarbe des Themas (applyTheme: CheckMark = accent), deckend - wie der Hauptknopf des Formular-Bausteins.
ImVec4 betonung() {
    ImVec4 a = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    a.w = 1.0F;
    return a;
}

// mit der Deckkraft des Stils (gesperrte Elemente)
ImU32 u32(const ImVec4& c) { return ImGui::GetColorU32(c); }

bool blender() { return z().stil == RolloutStil::Blender; }

// Eckenrundung der Kaesten und Knoepfe: wie 3ds Max die des Themas (klassisch: eckig), wie Blender deutlich rund.
float rundung() {
    const float thema = ImGui::GetStyle().FrameRounding;
    return blender() ? std::max(thema, std::floor(ImGui::GetFontSize() * 0.32F)) : std::min(thema, 2.0F);
}

float polster() { return std::floor(ImGui::GetFontSize() * 0.45F); }

const char* sichtEnde(const char* text) { return ImGui::FindRenderedTextEnd(text); }

// Die Breite des breitesten Worts (getrennt an Leerzeichen und Zeilenwechseln) in der laufenden Schrift (KnopfOpt::tasteHalten).
// Ein Wort mit CJK-Zeichen (ab U+2E80) bricht der Umbruch (textumbruch.h) zwischen den Zeichen; die Sperrregeln (kein Satzzeichen
// am Zeilenanfang, keine oeffnende Klammer am Ende) halten hoechstens zwei Zeichen zusammen - dort zaehlt das breiteste Paar.
float breitestesWort(const char* text, const char* ende) {
    const auto cjkPaar = [](const char* a, const char* e) {
        float b = 0.0F;
        const char* vor = nullptr;
        bool cjk = false;
        for (const char* c = a; c < e;) {
            unsigned int z = 0;
            const int n = ImTextCharFromUtf8(&z, c, e);
            cjk = cjk || z >= 0x2E80U;
            const char* naechst = c + std::max(n, 1);
            b = std::max(b, ImGui::CalcTextSize(vor != nullptr ? vor : c, naechst).x);
            vor = c;
            c = naechst;
        }
        return cjk ? b : ImGui::CalcTextSize(a, e).x;
    };
    float b = 0.0F;
    const char* wort = text;
    for (const char* c = text;; ++c) {
        if (c == ende || *c == ' ' || *c == '\n') {
            if (c > wort) { b = std::max(b, cjkPaar(wort, c)); }
            if (c == ende) { return b; }
            wort = c + 1;
        }
    }
}

// AP14: ein Text bekam weniger Breite als sein breitestes Wort, obwohl die Zeile (moeglich) dafuer reicht - gezaehlt nur,
// solange das Element-Register laeuft (Selbsttest). Ein Wort, das breiter ist als die ganze Zeile, zaehlt nicht: es muss brechen.
void wortbruchMerken(int WortbruchProbe::*zaehler, const char* text, const char* ende, float breite, float moeglich) {
    if (!GImGui->TestEngineHookItems || text == nullptr) { return; }
    if (ende == nullptr) { ende = text + std::strlen(text); }
    const float wort = breitestesWort(text, ende);
    if (breite + 0.5F >= wort || wort > moeglich + 0.5F) { return; }
    WortbruchProbe& p = z().wortbruch;
    ++(p.*zaehler);
    if (p.erster.empty()) { p.erster.assign(text, ende); }
}

Lauf* laufJetzt() {
    Zustand& s = z();
    return s.tiefe > 0 ? &s.lauf[s.tiefe - 1] : nullptr;
}

// Die Gruppe, zu der Formularzeilen und Regler gerade gehoeren (ausserhalb einer Gruppe: das Fenster).
Gruppe& gruppeJetzt() {
    Zustand& s = z();
    const Lauf* l = laufJetzt();
    const ImGuiID id = l != nullptr ? l->id : ImGui::GetCurrentWindow()->ID;
    Gruppe& g = s.gruppen[id];
    const int bild = ImGui::GetFrameCount();
    if (g.bild != bild) {
        g.spalte = g.spalteNeu;
        g.wert = g.wertNeu;
        g.spalteNeu = 0.0F;
        g.wertNeu = 0.0F;
        g.bild = bild;
    }
    return g;
}

// Den Cursor an eine Stelle setzen, ohne dass ImGui es fuer ein SetCursorPos des Aufrufers haelt (die Zeile beginnt neu).
void cursorAuf(ImGuiWindow* w, ImVec2 ort) {
    w->DC.CursorPos = ort;
    w->DC.CursorPosPrevLine = ort;
    w->DC.CurrLineSize = ImVec2{0.0F, 0.0F};
    w->DC.PrevLineSize = ImVec2{0.0F, 0.0F};
    w->DC.CurrLineTextBaseOffset = 0.0F;
    w->DC.PrevLineTextBaseOffset = 0.0F;
    w->DC.IsSameLine = false;
}

void tooltipText(const char* hinweis, const char* taste) {
    const bool hatHinweis = hinweis != nullptr && hinweis[0] != '\0';
    const bool hatTaste = taste != nullptr && taste[0] != '\0';
    if (!hatHinweis && !hatTaste) { return; }
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) { return; }
    if (!ImGui::BeginTooltip()) { return; }
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
    if (hatHinweis) { cjkTextUnformatted(hinweis); }
    if (hatTaste) { cjkTextDisabled("%s", taste); }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// --- Raster ----------------------------------------------------------------------------------------------

// Die Stelle und Breite des naechsten Bausteins: im Raster die naechste Zelle, sonst die ganze Zeile.
struct Platz {
    ImVec2 ort;
    float breite = 0.0F;
    float mindestHoehe = 0.0F;   // im Raster: so hoch war die Reihe im letzten Bild
    bool imRaster = false;
};

ImGuiID reihenSchluessel(const Raster& r) {
    const int reihe = r.zelle / std::max(r.spalten, 1);
    return ImHashData(&reihe, sizeof reihe, r.kennung);
}

void reiheAbschliessen(Raster& r) {
    if (r.zelle == 0) { return; }
    // die Reihe davor: ihre Hoehe fuer das naechste Bild merken (Knoepfe einer Reihe gleich hoch)
    Raster vor = r;
    vor.zelle = r.zelle - 1;
    z().reihen[reihenSchluessel(vor)] = r.reiheNatur;
}

Platz naechsterPlatz() {
    Zustand& s = z();
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    Raster& r = s.raster;
    Platz p;
    if (!r.an || r.fenster != w) {
        p.ort = w->DC.CursorPos;
        p.breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
        return p;
    }
    // Was die letzte Zelle hoch war (auch ein fremdes Element nach rasterZelle).
    if (r.zelle > 0) { r.reiheUnten = std::max(r.reiheUnten, w->DC.CursorPosPrevLine.y + w->DC.PrevLineSize.y); }
    const int spalte = r.zelle % r.spalten;
    if (spalte == 0) {
        if (r.zelle > 0) {
            reiheAbschliessen(r);
            r.reiheOben = r.reiheUnten + r.lueckeY;
        }
        r.reiheUnten = r.reiheOben;
        r.reiheNatur = 0.0F;
    }
    p.imRaster = true;
    p.ort = ImVec2{std::floor(r.x0 + static_cast<float>(spalte) * (r.zellB + r.lueckeX)), r.reiheOben};
    p.breite = r.zellB;
    const auto it = s.reihen.find(reihenSchluessel(r));
    p.mindestHoehe = it != s.reihen.end() ? it->second : 0.0F;
    cursorAuf(w, p.ort);
    ++r.zelle;
    return p;
}

// Ein Baustein hat seine Hoehe gemeldet (die er von sich aus braucht).
void platzBelegt(const Platz& p, float naturHoehe, float hoehe) {
    if (!p.imRaster) { return; }
    Raster& r = z().raster;
    r.reiheNatur = std::max(r.reiheNatur, naturHoehe);
    r.reiheUnten = std::max(r.reiheUnten, p.ort.y + hoehe);
}

// --- Ueberschrift einer Gruppe ---------------------------------------------------------------------------

struct KopfMass {
    float hoehe = 0.0F;
    float zeichenB = 0.0F;    // Platz fuer +/- bzw. Pfeil
    float schrittB = 0.0F;    // Platz fuer die Schrittnummer
    float titelX = 0.0F;      // ab der linken Kante des Balkens
    float titelB = 0.0F;      // Umbruchbreite des Titels
    ImVec2 titelG{0.0F, 0.0F};
    ImVec2 infoG{0.0F, 0.0F};
    bool infoDarunter = false;
    float infoB = 0.0F;       // Umbruchbreite der Kurzinfo
    float randX = 0.0F;
    float randY = 0.0F;
};

KopfMass kopfMessen(const char* titel, const RolloutOpt& o, float breite) {
    const ImGuiStyle& st = ImGui::GetStyle();
    const float fs = ImGui::GetFontSize();
    KopfMass m;
    m.randX = std::floor(fs * 0.4F);
    m.randY = std::floor(st.FramePadding.y + fs * 0.08F);
    m.zeichenB = std::floor(fs * 1.0F);
    m.schrittB = o.schritt > 0 ? std::floor(fs * 1.45F) : 0.0F;
    m.titelX = m.randX + m.zeichenB + m.schrittB;
    const float platz = std::max(breite - m.titelX - m.randX, fs * 2.0F);
    const char* ende = sichtEnde(titel);
    const ImVec2 titelFrei = cjkTextSize(titel, ende);
    const bool hatInfo = o.kurzinfo != nullptr && o.kurzinfo[0] != '\0';
    const ImVec2 infoFrei = hatInfo ? cjkTextSize(o.kurzinfo) : ImVec2{0.0F, 0.0F};
    const float luecke = std::floor(fs * 0.6F);
    if (!hatInfo || titelFrei.x + luecke + infoFrei.x <= platz) {
        m.titelB = platz - (hatInfo ? luecke + infoFrei.x : 0.0F);
        m.titelG = cjkTextSize(titel, ende, false, std::max(m.titelB, 1.0F));
        m.infoG = infoFrei;
        m.infoB = infoFrei.x;
        m.hoehe = std::max(m.titelG.y, fs) + m.randY * 2.0F;
    } else {
        // Titel und Kurzinfo passen nicht nebeneinander: der Titel ueber die ganze Breite, die Kurzinfo rechtsbuendig darunter.
        m.infoDarunter = true;
        m.titelB = platz;
        m.titelG = cjkTextSize(titel, ende, false, platz);
        m.infoB = platz;
        m.infoG = cjkTextSize(o.kurzinfo, nullptr, false, platz);
        m.hoehe = m.titelG.y + m.infoG.y + m.randY * 2.0F + std::floor(fs * 0.1F);
    }
    m.hoehe = std::floor(m.hoehe);
    return m;
}

// Der Grund des Kastens (Balken und Inhalt): unter dem Inhalt gezeichnet.
void kastenGrund(ImDrawList* dl, const ImRect& kopf, const ImRect& kasten, bool offen, bool ueber, bool gehalten) {
    const float r = rundung();
    const ImVec4 fenster = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    if (blender()) {
        // wie Blender: ein abgerundeter Kasten, etwas vom Grund abgehoben; die Ueberschrift ohne eigenen Balken
        ImVec4 grund = mischen(fenster, text, 0.055F);
        grund.w = 1.0F;
        dl->AddRectFilled(kasten.Min, kasten.Max, u32(grund), r);
        if (ueber || gehalten) {
            dl->AddRectFilled(kopf.Min, kopf.Max, ImGui::GetColorU32(gehalten ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered, 0.55F), r,
                              offen ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll);
        }
    } else {
        // wie 3ds Max: flacher Ueberschriftsbalken, duenner Rahmen um Balken und Inhalt
        ImVec4 innen = mischen(fenster, text, 0.02F);
        innen.w = 1.0F;
        if (offen) { dl->AddRectFilled(kasten.Min, kasten.Max, u32(innen), r); }
        const ImU32 balkenFarbe = ImGui::GetColorU32(gehalten ? ImGuiCol_ButtonActive : ueber ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        dl->AddRectFilled(kopf.Min, kopf.Max, balkenFarbe, r, offen ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll);
        dl->AddRect(kasten.Min, kasten.Max, ImGui::GetColorU32(ImGuiCol_Border), r);
        if (offen) { dl->AddLine(ImVec2{kasten.Min.x, kopf.Max.y - 1.0F}, ImVec2{kasten.Max.x - 1.0F, kopf.Max.y - 1.0F}, ImGui::GetColorU32(ImGuiCol_Border)); }
    }
}

// Zeichen, Schrittnummer, Titel und Kurzinfo des Balkens.
void kopfInhalt(ImDrawList* dl, const ImRect& kopf, const KopfMass& m, const char* titel, const RolloutOpt& o, bool offen, RolloutProbe& probe) {
    const float fs = ImGui::GetFontSize();
    const ImU32 textFarbe = ImGui::GetColorU32(ImGuiCol_Text);
    const float zeileY = kopf.Min.y + m.randY;
    // +/- (Max) bzw. Pfeil (Blender), auf der Mitte der ersten Titelzeile
    const float zg = std::floor(fs * 0.62F);
    const ImVec2 zm{std::floor(kopf.Min.x + m.randX + m.zeichenB * 0.42F), std::floor(zeileY + fs * 0.5F)};
    probe.zeichen = ImRect{ImVec2{zm.x - zg * 0.5F, zm.y - zg * 0.5F}, ImVec2{zm.x + zg * 0.5F, zm.y + zg * 0.5F}};
    if (blender()) {
        ImGui::RenderArrow(dl, ImVec2{zm.x - fs * 0.5F * 0.72F, zm.y - fs * 0.5F * 0.72F}, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                           offen ? ImGuiDir_Down : ImGuiDir_Right, 0.72F);
    } else {
        const float h = std::floor(zg * 0.5F);
        const float d = std::max(std::floor(fs * 0.09F), 1.0F);
        dl->AddRectFilled(ImVec2{zm.x - h, zm.y - d * 0.5F}, ImVec2{zm.x + h + 1.0F, zm.y - d * 0.5F + d}, textFarbe);
        if (!offen) { dl->AddRectFilled(ImVec2{zm.x - d * 0.5F + 0.5F, zm.y - h}, ImVec2{zm.x - d * 0.5F + 0.5F + d, zm.y + h + 1.0F}, textFarbe); }
    }
    // Schrittnummer im Kreis (eigene Zeichnung)
    if (o.schritt > 0) {
        const float rad = std::floor(fs * 0.56F);
        const ImVec2 mitte{std::floor(kopf.Min.x + m.randX + m.zeichenB + rad), std::floor(zeileY + fs * 0.5F)};
        const ImVec4 a = betonung();
        dl->AddCircleFilled(mitte, rad, u32(a), 24);
        char n[8];
        std::snprintf(n, sizeof n, "%d", o.schritt);
        const ImVec2 ng = ImGui::CalcTextSize(n);
        dl->AddText(ImVec2{std::floor(mitte.x - ng.x * 0.5F), std::floor(mitte.y - ng.y * 0.5F)}, helligkeit(a) > 0.6F ? IM_COL32_BLACK : IM_COL32_WHITE, n);
    }
    cjkAddText(dl, nullptr, 0.0F, ImVec2{kopf.Min.x + m.titelX, zeileY}, textFarbe, titel, sichtEnde(titel), std::max(m.titelB, 1.0F));
    if (o.kurzinfo != nullptr && o.kurzinfo[0] != '\0') {
        const ImU32 farbe = u32(tonFarbe(o.ton));
        ImVec2 ort{kopf.Max.x - m.randX - m.infoG.x, zeileY};
        if (m.infoDarunter) { ort.y = zeileY + m.titelG.y + std::floor(fs * 0.1F); }
        cjkAddText(dl, nullptr, 0.0F, ort, farbe, o.kurzinfo, nullptr, m.infoDarunter ? m.infoB : 0.0F);
        probe.info = ImRect{ort, ImVec2{ort.x + m.infoG.x, ort.y + m.infoG.y}};
    }
}

// Der Abstand zwischen zwei Gruppen.
float gruppenAbstand() { return std::floor(ImGui::GetFontSize() * 0.38F); }

// Beschriftungsspalte einer Formularzeile bzw. eines Reglers: zeichnet die Beschriftung und stellt den Cursor an die Feldkante.
// mindestFeld: so breit muss das Feld daneben bleiben. Passt die Beschriftung nicht in die Spalte (hoechstens die halbe Breite)
// oder bliebe das Feld schmaler, steht sie ueber dem Feld, und das Feld nimmt die ganze Breite.
void beschriftungZeichnen(const char* text, const char* hinweis, float mindestFeld) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const ImGuiStyle& st = ImGui::GetStyle();
    const float fs = ImGui::GetFontSize();
    Gruppe& g = gruppeJetzt();
    const char* ende = sichtEnde(text);
    const float textB = std::ceil(cjkTextSize(text, ende).x);
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float luecke = std::floor(fs * 0.6F);
    const float hoechstens = std::floor(breite * 0.5F);
    const float x0 = w->DC.CursorPos.x;
    const bool passt = textB + luecke <= hoechstens;
    if (passt) { g.spalteNeu = std::max(g.spalteNeu, textB + luecke); }
    const float spalte = std::min(std::max(g.spalte, passt ? textB + luecke : 0.0F), hoechstens);
    const bool darueber = !passt || breite - spalte < mindestFeld;
    Lauf* l = laufJetzt();
    if (darueber) {
        ImGui::PushTextWrapPos(0.0F);
        cjkTextUnformatted(text, ende);
        ImGui::PopTextWrapPos();
        if (hinweis != nullptr && hinweis[0] != '\0' && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", hinweis); }
        if (l != nullptr) {
            l->probe.beschriftungRechts = x0 + std::min(textB, breite);
            l->probe.feldLinks = x0;
        }
        return;
    }
    // auf der Feldmitte (wie AlignTextToFramePadding); wie Blender rechtsbuendig vor dem Feld
    const float tx = std::floor(blender() ? x0 + spalte - luecke - textB : x0);
    const ImVec2 ort{tx, w->DC.CursorPos.y};
    if (ImGui::ItemAdd(ImRect{ImVec2{x0, ort.y}, ImVec2{x0 + spalte - luecke, ort.y + ImGui::GetFrameHeight()}}, 0)) {
        w->DrawList->AddText(ImVec2{ort.x, ort.y + st.FramePadding.y}, ImGui::GetColorU32(ImGuiCol_Text), text, ende);
        if (hinweis != nullptr && hinweis[0] != '\0' && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", hinweis); }
    }
    if (l != nullptr) {
        l->probe.beschriftungRechts = ort.x + textB;
        l->probe.feldLinks = x0 + spalte;
    }
    w->DC.CursorPos.x = x0 + spalte;
}

}  // namespace

// --- Stil und Farben ------------------------------------------------------------------------------------

void rolloutStilSetzen(RolloutStil stil) { z().stil = stil; }

ImVec4 betonungsFarbe() { return betonung(); }

RolloutStil rolloutStil() { return z().stil; }

ImVec4 tonFarbe(Ton ton) {
    const bool hell = hellesThema();
    switch (ton) {
        case Ton::Normal: return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        case Ton::Leise: return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        case Ton::Gut: return hell ? ImVec4{0.10F, 0.47F, 0.18F, 1.0F} : ImVec4{0.44F, 0.80F, 0.47F, 1.0F};
        case Ton::Warnung: return hell ? ImVec4{0.62F, 0.40F, 0.00F, 1.0F} : ImVec4{0.93F, 0.74F, 0.28F, 1.0F};
        case Ton::Fehler: return hell ? ImVec4{0.76F, 0.14F, 0.11F, 1.0F} : ImVec4{0.94F, 0.42F, 0.38F, 1.0F};
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

// --- Rahmen des Felds ---------------------------------------------------------------------------------

namespace {

// Ein Reiter als Knopf: Text (Max) oder Symbol (Blender; AP14 B6: auch Max bei Mindestgroesse) - `symbol`: nur das Symbol, der
// Name steht im Tooltip. true: geklickt.
bool reiterKnopf(const FeldReiter& r, ImVec2 ort, ImVec2 groesse, bool aktiv, bool belegt, bool symbol, const char* leerHinweis) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    ImGuiContext& g = *GImGui;
    const ImGuiID id = w->GetID(r.titel);
    const ImRect bb{ort, ImVec2{ort.x + groesse.x, ort.y + groesse.y}};
    cursorAuf(w, ort);
    ImGui::ItemSize(groesse);
    if (!ImGui::ItemAdd(bb, id)) {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, r.titel, ImGuiItemStatusFlags_Checkable | (aktiv ? ImGuiItemStatusFlags_Checked : 0));
        return false;
    }
    bool ueber = false;
    bool gehalten = false;
    const bool geklickt = ImGui::ButtonBehavior(bb, id, &ueber, &gehalten) && belegt;
    ImDrawList* dl = w->DrawList;
    const float rund = rundung();
    const ImVec4 a = betonung();
    ImU32 grund = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 text = ImGui::GetColorU32(belegt ? ImGuiCol_Text : ImGuiCol_TextDisabled, belegt ? 1.0F : 0.7F);
    if (aktiv) {
        grund = u32(a);
        text = helligkeit(a) > 0.6F ? IM_COL32_BLACK : IM_COL32_WHITE;
    } else if (belegt && (ueber || gehalten)) {
        grund = ImGui::GetColorU32(gehalten ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
    }
    dl->AddRectFilled(bb.Min, bb.Max, grund, rund);
    if (!aktiv) { dl->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border, belegt ? 1.0F : 0.5F), rund); }
    if (symbol && r.icon != kKeinIcon) {
        const float ig = std::floor(std::min(groesse.x, groesse.y) * 0.62F);
        uiIcon(dl, r.icon, ImVec2{std::floor(bb.Min.x + (groesse.x - ig) * 0.5F), std::floor(bb.Min.y + (groesse.y - ig) * 0.5F)}, ig, text);
    } else {
        const char* ende = sichtEnde(r.titel);
        const ImVec2 tg = ImGui::CalcTextSize(r.titel, ende);
        dl->AddText(ImVec2{std::floor(bb.Min.x + (groesse.x - tg.x) * 0.5F), std::floor(bb.Min.y + (groesse.y - tg.y) * 0.5F)}, text, r.titel, ende);
    }
    ImGui::RenderNavCursor(bb, id);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, r.titel, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (aktiv ? ImGuiItemStatusFlags_Checked : 0));
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
        const bool hatHinweis = r.hinweis != nullptr && r.hinweis[0] != '\0';
        const bool hatLeer = !belegt && leerHinweis != nullptr && leerHinweis[0] != '\0';
        if ((symbol || hatHinweis || hatLeer) && ImGui::BeginTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
            if (symbol) {
                cjkTextUnformatted(r.titel, sichtEnde(r.titel));
                z().feldProbe.reiterTooltip.assign(r.titel, sichtEnde(r.titel));   // Selbsttest (AP14 R)
            }
            if (hatHinweis) { cjkTextDisabled("%s", r.hinweis); }
            if (hatLeer) { cjkTextDisabled("%s", leerHinweis); }
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    return geklickt;
}

}  // namespace

void feldBeginn(const char* id, const FeldOpt& o) {
    Zustand& s = z();
    s.stil = o.stil;
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const ImGuiStyle& st = ImGui::GetStyle();
    const float fs = ImGui::GetFontSize();
    FeldProbe& probe = s.feldProbe;
    probe = FeldProbe{};
    probe.bild = ImGui::GetFrameCount();
    probe.stil = o.stil;
    probe.reiterZahl = o.reiterZahl;
    const bool senkrecht = o.stil == RolloutStil::Blender;
    probe.reiterSenkrecht = senkrecht && o.reiterZahl > 0;

    // Welche Reiter im letzten Bild dieses Kontexts Gruppen hatten (unbekannt: alle waehlbar).
    Feld& f = s.feld;
    f = Feld{};
    f.an = true;
    f.schluessel = ImHashData(&o.kontext, sizeof o.kontext, w->GetID(id));
    f.standard = std::clamp(o.standard, 0, std::max(o.reiterZahl - 1, 0));
    f.zahl = o.reiterZahl;
    const auto bekannt = s.belegt.find(f.schluessel);
    const bool weiss = bekannt != s.belegt.end();
    const unsigned belegt = (weiss ? bekannt->second : ~0U) | reiterBit(f.standard);
    int aktiv = o.aktiv != nullptr ? *o.aktiv : f.standard;
    if (aktiv < 0 || aktiv >= o.reiterZahl || (belegt & reiterBit(aktiv)) == 0U) { aktiv = f.standard; }
    probe.reiterBelegt = weiss ? belegt : 0U;

    // Titelzeile: links der Name des Felds, rechts der Bereich (gedaempft, mit Linie darunter)
    const ImVec2 start = w->DC.CursorPos;
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    {
        const char* rechts = o.rechts != nullptr ? o.rechts : "";
        const float lb = cjkTextSize(o.titel).x;
        const float rb = cjkTextSize(rechts).x;
        const ImU32 farbe = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        const bool nebeneinander = lb + rb + fs <= breite;
        const float hoehe = (nebeneinander ? fs : fs * 2.0F + 2.0F) + std::floor(fs * 0.3F);
        ImGui::ItemSize(ImVec2{breite, hoehe});
        const ImRect r{start, ImVec2{start.x + breite, start.y + hoehe}};
        if (ImGui::ItemAdd(r, 0)) {
            w->DrawList->AddText(start, farbe, o.titel);
            w->DrawList->AddText(nebeneinander ? ImVec2{start.x + breite - rb, start.y} : ImVec2{start.x, start.y + fs + 2.0F}, farbe, rechts);
            w->DrawList->AddLine(ImVec2{start.x, r.Max.y - 1.0F}, ImVec2{r.Max.x, r.Max.y - 1.0F}, ImGui::GetColorU32(ImGuiCol_Separator));
        }
        probe.titel = r;
    }

    ImGui::PushID("##reiter");
    if (!senkrecht && o.reiterZahl > 0 && o.symbole) {
        // AP14 (Entscheidung B6): wie 3ds Max bei Mindestgroesse nur die Symbole in einer Reihe, der Name im Tooltip (wie Blenders
        // Eigenschaften-Reiter); die Knoepfe so gross wie in Blenders Spalte, schmaler nur, wenn die Reihe sonst nicht passt
        const float luecke = std::floor(fs * 0.15F);
        const float n = static_cast<float>(o.reiterZahl);
        const float g = std::max(std::floor(std::min(fs * 1.7F, (breite - luecke * (n - 1.0F)) / n)), 1.0F);
        ImVec2 ort = w->DC.CursorPos;
        const float x0 = ort.x;
        probe.reiter = ImRect{ort, ort};
        probe.reiterSymbole = true;
        for (int i = 0; i < o.reiterZahl; ++i) {
            if (reiterKnopf(o.reiter[i], ort, ImVec2{g, g}, i == aktiv, (belegt & reiterBit(i)) != 0U, true, o.leerHinweis)) { aktiv = i; }
            probe.reiter.Add(ImRect{ort, ImVec2{ort.x + g, ort.y + g}});
            ort.x += g + luecke;
        }
        cursorAuf(w, ImVec2{x0, probe.reiter.Max.y + st.ItemSpacing.y});
    } else if (!senkrecht && o.reiterZahl > 0) {
        // wie 3ds Max: eine Reihe mit Text, bricht um
        const ImVec2 saum{std::floor(fs * 0.5F), std::floor(fs * 0.22F)};
        const float luecke = std::floor(fs * 0.25F);
        const float hoehe = std::floor(fs + saum.y * 2.0F);
        ImVec2 ort = w->DC.CursorPos;
        const float x0 = ort.x;
        probe.reiter = ImRect{ort, ort};
        for (int i = 0; i < o.reiterZahl; ++i) {
            const FeldReiter& r = o.reiter[i];
            const float b = std::floor(ImGui::CalcTextSize(r.titel, sichtEnde(r.titel)).x + saum.x * 2.0F);
            if (i > 0 && ort.x + b > x0 + breite + 0.5F) { ort = ImVec2{x0, ort.y + hoehe + luecke}; }
            if (reiterKnopf(r, ort, ImVec2{b, hoehe}, i == aktiv, (belegt & reiterBit(i)) != 0U, false, o.leerHinweis)) { aktiv = i; }
            probe.reiter.Add(ImRect{ort, ImVec2{ort.x + b, ort.y + hoehe}});
            ort.x += b + luecke;
        }
        cursorAuf(w, ImVec2{x0, probe.reiter.Max.y + st.ItemSpacing.y});
    }
    float spalteB = 0.0F;
    if (senkrecht && o.reiterZahl > 0) {
        // wie Blender: senkrechte Symbolspalte links neben dem Inhalt
        const float g = std::floor(fs * 1.7F);
        const float luecke = std::floor(fs * 0.15F);
        ImVec2 ort = w->DC.CursorPos;
        const ImVec2 oben = ort;
        const float hoehe = std::max(ImGui::GetContentRegionAvail().y, g);
        ImVec4 streifen = mischen(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg), ImVec4{0.0F, 0.0F, 0.0F, 1.0F}, hellesThema() ? 0.06F : 0.22F);
        streifen.w = 1.0F;
        w->DrawList->AddRectFilled(oben, ImVec2{oben.x + g + luecke * 2.0F, oben.y + hoehe}, u32(streifen), rundung());
        probe.reiter = ImRect{oben, ImVec2{oben.x + g + luecke * 2.0F, oben.y + hoehe}};
        ort.x += luecke;
        ort.y += luecke;
        for (int i = 0; i < o.reiterZahl; ++i) {
            if (reiterKnopf(o.reiter[i], ort, ImVec2{g, g}, i == aktiv, (belegt & reiterBit(i)) != 0U, true, o.leerHinweis)) { aktiv = i; }
            ort.y += g + luecke;
        }
        spalteB = g + luecke * 2.0F + std::floor(fs * 0.3F);
        cursorAuf(w, ImVec2{oben.x + spalteB, oben.y});
    }
    ImGui::PopID();
    if (o.aktiv != nullptr) { *o.aktiv = aktiv; }
    f.aktiv = o.reiterZahl > 0 ? aktiv : -1;
    probe.reiterAktiv = f.aktiv;

    // der rollende Bereich der Gruppen
    ImGui::BeginChild(id, ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None, ImGuiWindowFlags_None);
    f.fenster = ImGui::GetCurrentWindow();
    probe.inhalt = f.fenster->InnerRect;
}

void feldEnde() {
    Zustand& s = z();
    Feld& f = s.feld;
    // vergessene rolloutEnde() in diesem Fenster schliessen (sonst bliebe der Zeichenkanal geteilt)
    while (s.tiefe > 0 && f.an && s.lauf[s.tiefe - 1].fenster == f.fenster) { rolloutEnde(); }
    // FIX612C (PV4): hat der rollende Bereich in diesem Bild nichts gezeichnet (SkipItems: ganz abgeschnitten, z. B. ein sehr
    // kleines Fenster), meldete keine Gruppe ihren Reiter - dann gilt der Stand des letzten gezeichneten Bilds weiter, sonst fiele
    // der gewaehlte Reiter auf den vorgegebenen zurueck
    const bool gezeichnet = f.fenster != nullptr && !f.fenster->SkipItems;
    ImGui::EndChild();
    if (f.an && gezeichnet) { s.belegt[f.schluessel] = f.belegtNeu; }
    f = Feld{};
}

int feldReiter() { return z().feld.an ? z().feld.aktiv : -1; }

bool feldReiterBelegt(int reiter) {
    const Zustand& s = z();
    if (!s.feld.an || reiter < 0 || reiter >= 31) { return false; }
    const auto it = s.belegt.find(s.feld.schluessel);
    return it != s.belegt.end() && (it->second & reiterBit(reiter)) != 0U;
}

bool feldReiterStandard() {
    const Feld& f = z().feld;
    return !f.an || f.aktiv < 0 || f.aktiv == f.standard;
}

// --- Gruppe ------------------------------------------------------------------------------------------

bool rolloutBeginn(const char* titel, const RolloutOpt& o) {
    Zustand& s = z();
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return false; }
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& st = g.Style;
    // Gehoert die Gruppe zum gewaehlten Reiter des Felds?
    Feld& f = s.feld;
    if (f.an && f.aktiv >= 0 && f.fenster != nullptr && ImGui::IsWindowChildOf(w, f.fenster, false, false)) {
        unsigned bits = o.reiter & ~kReiterStandard;
        if ((o.reiter & kReiterStandard) != 0U) { bits |= reiterBit(f.standard); }
        f.belegtNeu |= bits;
        if ((bits & reiterBit(f.aktiv)) == 0U) { return false; }
    }
    if (s.tiefe >= kTiefe) { return false; }
    const ImGuiID id = w->GetID(titel);
    bool offen = w->StateStorage.GetInt(id, o.zu ? 0 : 1) != 0;
    // rolloutZeigen: die Kennung ohne das Fenster (Titel bzw. der Teil nach "###")
    const bool zeigen = s.zeigen != 0 && s.zeigen == ImHashStr(titel);
    if (zeigen) {
        offen = true;
        w->StateStorage.SetInt(id, 1);
        ImGui::SetScrollHereY(0.0F);
        s.zeigen = 0;
    }

    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const KopfMass m = kopfMessen(titel, o, breite);
    const ImVec2 ort = w->DC.CursorPos;
    const ImRect kopf{ort, ImVec2{ort.x + breite, ort.y + m.hoehe}};
    ImGui::ItemSize(kopf.GetSize(), m.randY);
    const bool sichtbar = ImGui::ItemAdd(kopf, id);
    bool ueber = false;
    bool gehalten = false;
    if (sichtbar && ImGui::ButtonBehavior(kopf, id, &ueber, &gehalten)) {
        offen = !offen;
        w->StateStorage.SetInt(id, offen ? 1 : 0);
    }
    IMGUI_TEST_ENGINE_ITEM_INFO(id, titel, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Openable | (offen ? ImGuiItemStatusFlags_Opened : 0));
    if (sichtbar && o.hinweis != nullptr && o.hinweis[0] != '\0' && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", o.hinweis); }

    RolloutProbe probe;
    probe.id = id;
    probe.titel.assign(titel, sichtEnde(titel));
    probe.kurzinfo = o.kurzinfo != nullptr ? o.kurzinfo : "";
    probe.kopf = kopf;
    probe.kasten = kopf;
    probe.schritt = o.schritt;
    probe.offen = offen;
    probe.stil = s.stil;
    probe.rundung = rundung();

    ImDrawList* dl = w->DrawList;
    if (!offen) {
        if (sichtbar) {
            kastenGrund(dl, kopf, kopf, false, ueber, gehalten);
            kopfInhalt(dl, kopf, m, titel, o, false, probe);
            ImGui::RenderNavCursor(kopf, id);
        }
        w->DC.CursorPos.y += gruppenAbstand() - st.ItemSpacing.y;
        s.proben.push_back(std::move(probe));
        return false;
    }
    // Offen: der Kasten kommt am Ende UNTER den Inhalt (zweiter Zeichenkanal), der Inhalt steht eingerueckt im selben Fenster.
    Lauf& l = s.lauf[s.tiefe++];
    l.id = id;
    l.fenster = w;
    l.kopf = kopf;
    l.ueber = ueber;
    l.gehalten = gehalten;
    l.rasterNr = 0;
    l.zeilenNr = 0;
    l.probe = std::move(probe);
    l.teiler.Split(dl, 2);
    l.teiler.SetCurrentChannel(dl, 1);
    kopfInhalt(dl, kopf, m, titel, o, true, l.probe);
    if (sichtbar) { ImGui::RenderNavCursor(kopf, id); }
    const float pol = polster();
    l.workMax = w->WorkRect.Max.x;
    l.inhaltMax = w->ContentRegionRect.Max.x;
    l.einzug = pol;
    w->WorkRect.Max.x -= pol;
    w->ContentRegionRect.Max.x -= pol;
    w->DC.Indent.x += pol;
    w->DC.CursorPos.x = w->Pos.x + w->DC.Indent.x + w->DC.ColumnsOffset.x;
    w->DC.CursorPos.y += std::max(pol - st.ItemSpacing.y, 0.0F);
    (void)gruppeJetzt();   // die Spalten des letzten Bildes uebernehmen
    return true;
}

void rolloutEnde() {
    Zustand& s = z();
    if (s.tiefe <= 0) { return; }
    Lauf& l = s.lauf[s.tiefe - 1];
    ImGuiWindow* w = l.fenster;
    const ImGuiStyle& st = ImGui::GetStyle();
    if (s.raster.an && s.raster.fenster == w) { rasterEnde(); }
    const float pol = polster();
    w->DC.Indent.x -= l.einzug;
    w->WorkRect.Max.x = l.workMax;
    w->ContentRegionRect.Max.x = l.inhaltMax;
    // Unterkante des Inhalts: der Cursor steht einen Zeilenabstand darunter.
    const float unten = std::max(w->DC.CursorPos.y - st.ItemSpacing.y, l.kopf.Max.y) + pol;
    const ImRect kasten{l.kopf.Min, ImVec2{l.kopf.Max.x, unten}};
    cursorAuf(w, ImVec2{w->Pos.x + w->DC.Indent.x + w->DC.ColumnsOffset.x, l.kopf.Max.y});
    ImGui::ItemSize(ImVec2{0.0F, unten - l.kopf.Max.y});
    w->DC.CursorPos.y += gruppenAbstand() - st.ItemSpacing.y;
    ImDrawList* dl = w->DrawList;
    l.teiler.SetCurrentChannel(dl, 0);
    kastenGrund(dl, l.kopf, kasten, true, l.ueber, l.gehalten);
    l.teiler.Merge(dl);
    l.probe.kasten = kasten;
    s.proben.push_back(std::move(l.probe));
    --s.tiefe;
}

void rolloutZeigen(const char* titel) {
    z().zeigen = ImHashStr(titel);
    z().zeigenBild = ImGui::GetFrameCount();
}

void rolloutSetzen(const char* titel, bool offen) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    w->StateStorage.SetInt(w->GetID(titel), offen ? 1 : 0);
}

bool mehrBeginn(const char* titel, bool offenVorgabe) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return false; }
    ImGuiContext& g = *GImGui;
    const float fs = ImGui::GetFontSize();
    const ImGuiID id = w->GetID(titel);
    bool offen = w->StateStorage.GetInt(id, offenVorgabe ? 1 : 0) != 0;
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float pfeilB = std::floor(fs * 1.0F);
    const char* ende = sichtEnde(titel);
    const ImVec2 tg = cjkTextSize(titel, ende, false, std::max(breite - pfeilB, 1.0F));
    const ImVec2 ort = w->DC.CursorPos;
    const ImRect bb{ort, ImVec2{ort.x + breite, ort.y + std::max(tg.y, fs) + std::floor(fs * 0.2F)}};
    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, id)) {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, titel, ImGuiItemStatusFlags_Openable | (offen ? ImGuiItemStatusFlags_Opened : 0));
        return offen;
    }
    bool ueber = false;
    bool gehalten = false;
    if (ImGui::ButtonBehavior(bb, id, &ueber, &gehalten)) {
        offen = !offen;
        w->StateStorage.SetInt(id, offen ? 1 : 0);
    }
    ImDrawList* dl = w->DrawList;
    if (ueber || gehalten) { dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(gehalten ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered, 0.5F), rundung()); }
    const ImU32 farbe = ImGui::GetColorU32(ueber ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    ImGui::RenderArrow(dl, ImVec2{ort.x + fs * 0.1F, ort.y + fs * 0.24F}, farbe, offen ? ImGuiDir_Down : ImGuiDir_Right, 0.66F);
    cjkAddText(dl, nullptr, 0.0F, ImVec2{ort.x + pfeilB, ort.y + std::floor(fs * 0.1F)}, farbe, titel, ende, std::max(breite - pfeilB, 1.0F));
    ImGui::RenderNavCursor(bb, id);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, titel, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Openable | (offen ? ImGuiItemStatusFlags_Opened : 0));
    return offen;
}

void mehrEnde() {}

// --- Knoepfe ----------------------------------------------------------------------------------------

void rasterBeginn(int spalten, float mindestZelle) {
    Zustand& s = z();
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const float fs = ImGui::GetFontSize();
    Raster& r = s.raster;
    if (r.an) { rasterEnde(); }
    r = Raster{};
    r.an = true;
    r.fenster = w;
    Lauf* l = laufJetzt();
    const int nr = l != nullptr ? l->rasterNr++ : 0;
    // die Kennung des Rasters: Gruppe und laufende Nummer darin (ausserhalb: die Stelle im Fenster)
    const float y = w->DC.CursorPos.y - w->Pos.y + w->Scroll.y;
    r.kennung = l != nullptr ? ImHashData(&nr, sizeof nr, l->id) : ImHashData(&y, sizeof y, w->ID);
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    r.lueckeX = std::floor(fs * 0.4F);
    r.lueckeY = std::floor(fs * 0.4F);
    int n = std::max(spalten, 1);
    float zelle = std::floor((breite - r.lueckeX * static_cast<float>(n - 1)) / static_cast<float>(n));
    // AP14: eine Zelle ist auch so breit, wie das breiteste Wort ihrer Knoepfe und Schalter im letzten Bild brauchte - sonst
    // braeche ein Wort mitten im Wort um (deutsch "Schleifenschnitt" im schmaleren Feld wie Blender). Der Bedarf haengt nicht
    // von der Spaltenzahl ab, die Wahl schwingt also nicht.
    const auto bedarf = s.zellBedarf.find(r.kennung);
    const float mindest = std::max(mindestZelle * fs, bedarf != s.zellBedarf.end() ? bedarf->second : 0.0F);
    while (n > 1 && zelle < mindest) {
        --n;
        zelle = std::floor((breite - r.lueckeX * static_cast<float>(n - 1)) / static_cast<float>(n));
    }
    r.ganz = breite;
    r.spalten = n;
    r.zellB = std::max(zelle, 1.0F);
    r.x0 = w->DC.CursorPos.x;
    r.reiheOben = w->DC.CursorPos.y;
    r.reiheUnten = r.reiheOben;
    if (l != nullptr) { l->probe.rasterSpalten = n; }
}

void rasterEnde() {
    Zustand& s = z();
    Raster& r = s.raster;
    if (!r.an) { return; }
    ImGuiWindow* w = r.fenster;
    if (r.zelle > 0) {
        r.reiheUnten = std::max(r.reiheUnten, w->DC.CursorPosPrevLine.y + w->DC.PrevLineSize.y);
        // die letzte Reihe merken (r.zelle zeigt hinter ihre letzte Zelle)
        Raster letzte = r;
        letzte.zelle = r.zelle - 1;
        s.reihen[reihenSchluessel(letzte)] = r.reiheNatur;
        cursorAuf(w, ImVec2{r.x0, r.reiheUnten + ImGui::GetStyle().ItemSpacing.y});
    }
    s.zellBedarf[r.kennung] = std::min(std::ceil(r.bedarf), r.ganz);   // breiter als das ganze Raster kann keine Zelle werden
    r.an = false;
}

void rasterZelle() {
    const Platz p = naechsterPlatz();
    ImGui::SetNextItemWidth(p.breite);
}

bool werkzeugKnopf(const char* text, const KnopfOpt& o) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return false; }
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& st = g.Style;
    const float fs = ImGui::GetFontSize();
    const ImGuiID id = w->GetID(text);
    const Platz p = naechsterPlatz();
    const char* ende = sichtEnde(text);

    const float randX = std::floor(st.FramePadding.x + fs * 0.12F);
    const float randY = std::floor(st.FramePadding.y + fs * (o.haupt ? 0.16F : 0.06F));
    const bool hatIcon = o.icon != kKeinIcon;
    const float iconG = hatIcon ? std::floor(fs * 1.0F) : 0.0F;
    const float luecke = std::floor(fs * 0.42F);
    // die Taste als kleines Tastenfeld (kleinere Schrift); faellt weg, wenn dem Text sonst zu wenig Platz bleibt
    ImFont* schrift = ImGui::GetFont();
    const float tasteFs = std::floor(fs * 0.80F);
    bool hatTaste = o.taste != nullptr && o.taste[0] != '\0';
    const ImVec2 tasteG = hatTaste ? schrift->CalcTextSizeA(tasteFs, FLT_MAX, 0.0F, o.taste) : ImVec2{0.0F, 0.0F};
    const float tastePol = std::floor(fs * 0.28F);
    const float tasteB = hatTaste ? std::ceil(tasteG.x + tastePol * 2.0F) : 0.0F;
    float textB = p.breite - randX * 2.0F - (hatIcon ? iconG + luecke : 0.0F) - (hatTaste ? tasteB + luecke : 0.0F);
    // tasteHalten: die Taste bleibt genau dann, wenn jedes Wort neben ihr Platz hat - sonst braeche ein langes Wort mitten im Wort um
    // (AP6, deutsch "Zusammenfuehren" im Blender-Feld: die alte Bedingung hielt die Taste, weil dem Text 4 Zeichenhoehen blieben)
    const bool zuEng = o.tasteHalten ? textB < breitestesWort(text, ende) : textB < std::min(cjkTextSize(text, ende).x, fs * 4.0F);
    if (hatTaste && zuEng) {
        hatTaste = false;
        textB = p.breite - randX * 2.0F - (hatIcon ? iconG + luecke : 0.0F);
    }
    textB = std::max(textB, fs);
    {
        // AP14: was die Zelle braucht, damit das breiteste Wort neben Rand und Symbol passt (rasterBeginn nimmt dann weniger
        // Spalten); die Probe zaehlt einen Bruch nur, wenn die ganze Zeile fuer das Wort reichte
        const float beiwerk = randX * 2.0F + (hatIcon ? iconG + luecke : 0.0F);
        Raster& r = z().raster;
        const bool imRaster = p.imRaster && r.an;
        if (imRaster) { r.bedarf = std::max(r.bedarf, beiwerk + breitestesWort(text, ende)); }
        wortbruchMerken(&WortbruchProbe::knoepfe, text, ende, textB, (imRaster ? r.ganz : p.breite) - beiwerk);
    }
    const ImVec2 textG = cjkTextSize(text, ende, false, textB);
    const float mindest = std::floor(fs * (o.haupt ? 2.15F : 1.85F));
    const float natur = std::max(std::max(textG.y, iconG) + randY * 2.0F, mindest);
    const float hoehe = std::floor(std::max(natur, p.mindestHoehe));
    const ImRect bb{p.ort, ImVec2{p.ort.x + p.breite, p.ort.y + hoehe}};
    ImGui::ItemSize(bb.GetSize(), randY);
    platzBelegt(p, natur, hoehe);
    if (Lauf* l = laufJetzt(); l != nullptr) {
        ++l->probe.knoepfe;
        if (o.haupt) { ++l->probe.hauptKnoepfe; }
        if (hatIcon) { ++l->probe.symbole; }
        if (hatTaste) { ++l->probe.tasten; }
    }
    if (!o.aktiv) { ImGui::BeginDisabled(); }
    const bool da = ImGui::ItemAdd(bb, id);
    bool geklickt = false;
    if (da) {
        bool ueber = false;
        bool gehalten = false;
        geklickt = ImGui::ButtonBehavior(bb, id, &ueber, &gehalten);
        ImDrawList* dl = w->DrawList;
        const float rund = rundung();
        const ImVec4 weiss{1.0F, 1.0F, 1.0F, 1.0F};
        const ImVec4 schwarz{0.0F, 0.0F, 0.0F, 1.0F};
        const ImVec4 a = betonung();
        ImVec4 grund = ImGui::GetStyleColorVec4(gehalten ? ImGuiCol_ButtonActive : ueber ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImVec4 kante = ImGui::GetStyleColorVec4(ImGuiCol_Border);
        ImVec4 textFarbe = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        // AP14 (Wunsch AP8): ein Hauptknopf, der umschaltet, ist aus nicht gefuellt - Rahmen und Symbol in der Betonungsfarbe
        // auf dem Grund eines gewoehnlichen Knopfs; Platz und Hoehe bleiben die des Hauptknopfs
        const bool gefuellt = o.haupt && (!o.umschalter || o.an);
        const bool nurRahmen = o.haupt && !gefuellt;
        if (gefuellt) {
            grund = gehalten ? mischen(a, schwarz, 0.15F) : ueber ? mischen(a, weiss, 0.15F) : a;
            kante = mischen(a, weiss, 0.25F);
            textFarbe = helligkeit(a) > 0.6F ? schwarz : weiss;
        } else if (nurRahmen) {
            kante = a;
        } else if (o.an) {
            grund = mischen(ImGui::GetStyleColorVec4(ImGuiCol_Button), a, ueber ? 0.55F : 0.42F);
            kante = a;
        }
        dl->AddRectFilled(bb.Min, bb.Max, u32(grund), rund);
        dl->AddRect(bb.Min, bb.Max, u32(kante), rund, 0, nurRahmen ? std::max(1.0F, std::floor(fs * 0.08F)) : 1.0F);
        float x = bb.Min.x + randX;
        if (hatIcon) {
            const ImVec4 iconFarbe = gefuellt ? textFarbe : nurRahmen ? a : uiIconFarbe(o.icon);
            uiIcon(dl, o.icon, ImVec2{std::floor(x), std::floor(bb.Min.y + (hoehe - iconG) * 0.5F)}, iconG, u32(iconFarbe));
            x += iconG + luecke;
        }
        cjkAddText(dl, nullptr, 0.0F, ImVec2{std::floor(x), std::floor(bb.Min.y + (hoehe - textG.y) * 0.5F)}, u32(textFarbe), text, ende, textB);
        if (hatTaste) {
            const ImVec2 tm{bb.Max.x - randX - tasteB, std::floor(bb.Min.y + (hoehe - tasteG.y) * 0.5F) - 1.0F};
            const ImVec2 tx{tm.x + tasteB, tm.y + tasteG.y + 2.0F};
            const ImVec4 feld = gefuellt ? mischen(a, schwarz, 0.28F) : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
            const ImVec4 schriftFarbe = gefuellt ? textFarbe : mischen(ImGui::GetStyleColorVec4(ImGuiCol_Text), a, 0.45F);
            dl->AddRectFilled(tm, tx, u32(feld), 3.0F);
            dl->AddRect(tm, tx, u32(kante), 3.0F);
            dl->AddText(schrift, tasteFs, ImVec2{tm.x + tastePol, tm.y + 1.0F}, u32(schriftFarbe), o.taste);
        }
        ImGui::RenderNavCursor(bb, id);
    }
    IMGUI_TEST_ENGINE_ITEM_INFO(id, text, g.LastItemData.StatusFlags | (o.an ? ImGuiItemStatusFlags_Checked : 0));
    if (!o.aktiv) { ImGui::EndDisabled(); }
    if (da) { tooltipText(o.hinweis, o.taste); }
    return geklickt && o.aktiv;
}

bool schalter(const char* text, bool* wert, const char* hinweis, bool aktiv) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return false; }
    ImGuiContext& g = *GImGui;
    const float fs = ImGui::GetFontSize();
    const ImGuiID id = w->GetID(text);
    const Platz p = naechsterPlatz();
    const char* ende = sichtEnde(text);
    const float kasten = std::floor(fs * 1.05F);
    const float luecke = std::floor(fs * 0.4F);
    const float textB = std::max(p.breite - kasten - luecke, fs);
    {
        // AP14: wie beim werkzeugKnopf - die Zelle so breit, dass das breiteste Wort neben dem Kasten passt
        Raster& r = z().raster;
        const bool imRaster = p.imRaster && r.an;
        if (imRaster) { r.bedarf = std::max(r.bedarf, kasten + luecke + breitestesWort(text, ende)); }
        wortbruchMerken(&WortbruchProbe::schalter, text, ende, textB, (imRaster ? r.ganz : p.breite) - kasten - luecke);
    }
    const ImVec2 textG = cjkTextSize(text, ende, false, textB);
    const float natur = std::max(textG.y, kasten) + std::floor(fs * 0.16F);
    const float hoehe = std::floor(natur);
    // im Raster ueber die ganze Zelle klickbar, sonst nur Kasten und Text
    const float breite = p.imRaster ? p.breite : std::min(p.breite, kasten + luecke + textG.x);
    const ImRect bb{p.ort, ImVec2{p.ort.x + breite, p.ort.y + hoehe}};
    ImGui::ItemSize(bb.GetSize());
    platzBelegt(p, natur, hoehe);
    // AP14 (Wunsch AP8): aktiv = false sperrt den Schalter wie ImGui::BeginDisabled aussen herum; der Tooltip erscheint trotzdem
    if (!aktiv) { ImGui::BeginDisabled(); }
    if (!ImGui::ItemAdd(bb, id)) {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, text, ImGuiItemStatusFlags_Checkable | (wert != nullptr && *wert ? ImGuiItemStatusFlags_Checked : 0));
        if (!aktiv) { ImGui::EndDisabled(); }
        return false;
    }
    bool ueber = false;
    bool gehalten = false;
    const bool geklickt = ImGui::ButtonBehavior(bb, id, &ueber, &gehalten) && aktiv;
    if (geklickt && wert != nullptr) {
        *wert = !*wert;
        ImGui::MarkItemEdited(id);
    }
    ImDrawList* dl = w->DrawList;
    const ImVec2 k0{bb.Min.x, std::floor(bb.Min.y + (std::min(hoehe, fs + std::floor(fs * 0.16F)) - kasten) * 0.5F)};
    const ImVec2 k1{k0.x + kasten, k0.y + kasten};
    dl->AddRectFilled(k0, k1, ImGui::GetColorU32(gehalten ? ImGuiCol_FrameBgActive : ueber ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), std::min(rundung(), 4.0F));
    dl->AddRect(k0, k1, ImGui::GetColorU32(ImGuiCol_Border), std::min(rundung(), 4.0F));
    if (wert != nullptr && *wert) {
        const float saum = std::max(1.0F, std::floor(kasten / 6.0F));
        ImGui::RenderCheckMark(dl, ImVec2{k0.x + saum, k0.y + saum}, ImGui::GetColorU32(ImGuiCol_CheckMark), kasten - saum * 2.0F);
    }
    cjkAddText(dl, nullptr, 0.0F, ImVec2{k1.x + luecke, std::floor(bb.Min.y + (hoehe - textG.y) * 0.5F)}, ImGui::GetColorU32(ImGuiCol_Text), text, ende, textB);
    ImGui::RenderNavCursor(bb, id);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, text, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (wert != nullptr && *wert ? ImGuiItemStatusFlags_Checked : 0));
    if (!aktiv) { ImGui::EndDisabled(); }
    tooltipText(hinweis, nullptr);
    return geklickt;
}

namespace {

// Der Text eines Segments, in seiner Zelle umbrochen und senkrecht mittig: jede Zeile waagrecht mittig (CJK als Block mittig,
// umbrochen wie cjkTextWrapped).
void segmentText(ImDrawList* dl, const ImRect& bb, ImU32 farbe, const char* text, const char* ende, float breite) {
    const ImVec2 block = cjkTextSize(text, ende, false, breite);
    float y = std::floor(bb.Min.y + (bb.GetHeight() - block.y) * 0.5F);
    bool cjk = false;
    for (const char* c = text; c < ende && !cjk;) {
        unsigned int z = 0;
        c += std::max(ImTextCharFromUtf8(&z, c, ende), 1);
        cjk = z >= 0x2E80U;
    }
    if (cjk) {
        cjkAddText(dl, nullptr, 0.0F, ImVec2{std::floor(bb.Min.x + (bb.GetWidth() - block.x) * 0.5F), y}, farbe, text, ende, breite);
        return;
    }
    ImFont* f = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    for (const char* s = text; s < ende;) {
        const char* e = f->CalcWordWrapPosition(fs, s, ende, breite);
        if (e <= s) { e = s + 1; }
        const char* z = e;
        while (z > s && (z[-1] == ' ' || z[-1] == '\n')) { --z; }
        const float b = ImGui::CalcTextSize(s, z).x;
        dl->AddText(ImVec2{std::floor(bb.Min.x + (bb.GetWidth() - b) * 0.5F), y}, farbe, s, z);
        y += fs;
        s = e;
        while (s < ende && (*s == ' ' || *s == '\n')) { ++s; }
    }
}

}  // namespace

bool segment(const char* id, const char* const* texte, int zahl, int* wahl, int spalten, int gesperrtAb, const char* const* hinweise, float breite,
             bool umbrechen) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems || zahl <= 0) { return false; }
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& st = g.Style;
    const float fs = ImGui::GetFontSize();
    Platz p = naechsterPlatz();
    if (breite > 0.0F) { p.breite = std::min(p.breite, std::floor(breite)); }
    const float randX = std::floor(fs * 0.3F);
    umbrechen = umbrechen && spalten > 0;
    // umbrechen: ein Text darf in seiner Zelle umbrechen, es zaehlt sein breitestes Wort (CJK: Zeichenpaar)
    float breitester = 0.0F;
    for (int i = 0; i < zahl; ++i) {
        breitester = std::max(breitester, umbrechen ? breitestesWort(texte[i], sichtEnde(texte[i])) : ImGui::CalcTextSize(texte[i], sichtEnde(texte[i])).x);
    }
    // so viele Spalten, wie die Texte nebeneinander passen
    int n = spalten > 0 ? std::min(spalten, zahl) : zahl;
    while (n > 1 && std::floor(p.breite / static_cast<float>(n)) < breitester + randX * 2.0F) { --n; }
    if (spalten <= 0 && n < zahl) {
        // gleichmaessig umbrechen: so wenige Reihen wie noetig, darin gleich viele Spalten
        const int reihen = (zahl + n - 1) / n;
        n = (zahl + reihen - 1) / reihen;
    }
    const int reihen = (zahl + n - 1) / n;
    const float zeileH = std::floor(fs + st.FramePadding.y * 2.0F);
    // die Zellen einer Reihe: Breite der Zelle, Hoehe der Reihe (umgebrochen: die meisten Zeilen der Reihe)
    const auto zelle = [&](int i, float& x0, float& x1) {
        const int reihe = i / n;
        const int spalte = i % n;
        const int inReihe = reihe == reihen - 1 ? zahl - reihe * n : n;   // die letzte Reihe teilt die Breite unter ihren Eintraegen auf
        x0 = std::floor(p.ort.x + p.breite * static_cast<float>(spalte) / static_cast<float>(inReihe));
        x1 = spalte == inReihe - 1 ? p.ort.x + p.breite : std::floor(p.ort.x + p.breite * static_cast<float>(spalte + 1) / static_cast<float>(inReihe));
    };
    const auto textBreite = [&](int i) {
        float x0 = 0.0F;
        float x1 = 0.0F;
        zelle(i, x0, x1);
        return std::max(x1 - x0 - randX * 2.0F, 1.0F);
    };
    std::vector<float> reiheY(static_cast<std::size_t>(reihen) + 1U, 0.0F);
    for (int r = 0; r < reihen; ++r) {
        float h = zeileH;
        if (umbrechen) {
            for (int i = r * n; i < std::min(zahl, (r + 1) * n); ++i) {
                h = std::max(h, std::floor(cjkTextSize(texte[i], sichtEnde(texte[i]), false, textBreite(i)).y + st.FramePadding.y * 2.0F));
            }
        }
        reiheY[static_cast<std::size_t>(r) + 1U] = reiheY[static_cast<std::size_t>(r)] + h;
    }
    const float hoehe = reiheY.back();
    const ImRect ganz{p.ort, ImVec2{p.ort.x + p.breite, p.ort.y + hoehe}};
    ImGui::ItemSize(ganz.GetSize(), st.FramePadding.y);
    platzBelegt(p, hoehe, hoehe);
    ImDrawList* dl = w->DrawList;
    const float rund = rundung();
    const ImVec4 a = betonung();
    bool geaendert = false;
    ImGui::PushID(id);
    dl->AddRectFilled(ganz.Min, ganz.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), rund);
    for (int i = 0; i < zahl; ++i) {
        const int reihe = i / n;
        const int spalte = i % n;
        // die letzte Reihe teilt die Breite unter ihren Eintraegen auf
        const int inReihe = reihe == reihen - 1 ? zahl - reihe * n : n;
        float x0 = 0.0F;
        float x1 = 0.0F;
        zelle(i, x0, x1);
        const ImRect bb{ImVec2{x0, ganz.Min.y + reiheY[static_cast<std::size_t>(reihe)]}, ImVec2{x1, ganz.Min.y + reiheY[static_cast<std::size_t>(reihe) + 1U]}};
        const bool gesperrt = gesperrtAb >= 0 && i >= gesperrtAb;
        const ImGuiID kid = w->GetID(texte[i]);
        if (gesperrt) { ImGui::BeginDisabled(); }
        if (!ImGui::ItemAdd(bb, kid)) {
            IMGUI_TEST_ENGINE_ITEM_INFO(kid, texte[i], ImGuiItemStatusFlags_Checkable | (wahl != nullptr && *wahl == i ? ImGuiItemStatusFlags_Checked : 0));
        } else {
            bool ueber = false;
            bool gehalten = false;
            const bool geklickt = ImGui::ButtonBehavior(bb, kid, &ueber, &gehalten);
            const bool gewaehlt = wahl != nullptr && *wahl == i;
            ImDrawFlags ecken = ImDrawFlags_RoundCornersNone;
            if (reihe == 0 && spalte == 0) { ecken |= ImDrawFlags_RoundCornersTopLeft; }
            if (reihe == 0 && spalte == inReihe - 1) { ecken |= ImDrawFlags_RoundCornersTopRight; }
            if (reihe == reihen - 1 && spalte == 0) { ecken |= ImDrawFlags_RoundCornersBottomLeft; }
            if (reihe == reihen - 1 && spalte == inReihe - 1) { ecken |= ImDrawFlags_RoundCornersBottomRight; }
            ImU32 textFarbe = ImGui::GetColorU32(ImGuiCol_Text);
            if (gewaehlt) {
                dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(a), rund, ecken);
                textFarbe = ImGui::GetColorU32(helligkeit(a) > 0.6F ? ImVec4{0.0F, 0.0F, 0.0F, 1.0F} : ImVec4{1.0F, 1.0F, 1.0F, 1.0F});
            } else if (ueber || gehalten) {
                dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(gehalten ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered), rund, ecken);
            }
            if (spalte > 0) { dl->AddLine(ImVec2{bb.Min.x, bb.Min.y + 1.0F}, ImVec2{bb.Min.x, bb.Max.y - 1.0F}, ImGui::GetColorU32(ImGuiCol_Border)); }
            if (reihe > 0) { dl->AddLine(ImVec2{bb.Min.x, bb.Min.y}, ImVec2{bb.Max.x, bb.Min.y}, ImGui::GetColorU32(ImGuiCol_Border)); }
            const char* ende = sichtEnde(texte[i]);
            if (umbrechen) {
                segmentText(dl, bb, textFarbe, texte[i], ende, textBreite(i));
            } else {
                const ImVec2 tg = ImGui::CalcTextSize(texte[i], ende);
                dl->AddText(ImVec2{std::floor(bb.Min.x + (bb.GetWidth() - tg.x) * 0.5F), std::floor(bb.Min.y + (zeileH - tg.y) * 0.5F)}, textFarbe, texte[i], ende);
            }
            ImGui::RenderNavCursor(bb, kid);
            if (geklickt && !gesperrt && wahl != nullptr && *wahl != i) {
                *wahl = i;
                geaendert = true;
            }
            IMGUI_TEST_ENGINE_ITEM_INFO(kid, texte[i], g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (gewaehlt ? ImGuiItemStatusFlags_Checked : 0));
            if (hinweise != nullptr) { tooltipText(hinweise[i], nullptr); }
        }
        if (gesperrt) { ImGui::EndDisabled(); }
    }
    dl->AddRect(ganz.Min, ganz.Max, ImGui::GetColorU32(ImGuiCol_Border), rund);
    ImGui::PopID();
    return geaendert;
}

// --- Werte ---------------------------------------------------------------------------------------------

namespace {

// Eine Zeile "Beschriftung ... Wert" (beide brechen um, wenn sie nicht nebeneinander passen).
void wertZeile(const char* name, const char* wert, Ton ton, const char* hinweis, bool nameLeise) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    ImGuiContext& g = *GImGui;
    const float fs = ImGui::GetFontSize();
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float luecke = std::floor(fs * 0.7F);
    const ImVec2 nameFrei = cjkTextSize(name);
    const ImVec2 wertFrei = cjkTextSize(wert);
    float nameB = nameFrei.x;
    float wertB = wertFrei.x;
    // ein Wert ohne Leerzeichen (Pfad, Dateiname) laesst sich nicht sinnvoll umbrechen: passt er nicht neben die Beschriftung,
    // bekommt er eine eigene Zeile darunter (rechtsbuendig, die ganze Breite)
    bool darunter = false;
    if (nameFrei.x + luecke + wertFrei.x > breite) {
        const float halb = std::floor((breite - luecke) * 0.5F);
        // AP14: umbrochen wird nur zwischen Woertern. Jede Spalte bleibt mindestens so breit wie ihr breitestes Wort (CJK:
        // Zeichenpaar) - bis rc611 brach bei Mindestgroesse "counterp / art", deutsch "Flaechenuebergrei / fend". Passen die
        // breitesten Woerter beider Spalten nicht nebeneinander, steht der Wert unter der Beschriftung.
        const float nameWort = std::ceil(breitestesWort(name, name + std::strlen(name)));
        const float wertWort = std::ceil(breitestesWort(wert, wert + std::strlen(wert)));
        if (std::strchr(wert, ' ') == nullptr || nameWort + luecke + wertWort > breite) {
            darunter = true;
            nameB = breite;
            wertB = breite;
        } else if (wertFrei.x <= halb) {
            // beide brechen um: die Beschriftung bekommt hoechstens die Haelfte, der Wert den Rest (und umgekehrt)
            nameB = breite - luecke - wertFrei.x;
            if (nameB < nameWort) {
                nameB = nameWort;
                wertB = breite - luecke - nameB;
            }
        } else if (nameFrei.x <= halb) {
            wertB = breite - luecke - nameFrei.x;
            if (wertB < wertWort) {
                wertB = wertWort;
                nameB = breite - luecke - wertB;
            }
        } else if (nameWort > halb) {
            nameB = nameWort;
            wertB = breite - luecke - nameB;
        } else if (wertWort > halb) {
            wertB = wertWort;
            nameB = breite - luecke - wertB;
        } else {
            nameB = halb;
            wertB = halb;
        }
    }
    wortbruchMerken(&WortbruchProbe::werte, name, nullptr, nameB, breite);
    wortbruchMerken(&WortbruchProbe::werte, wert, nullptr, wertB, breite);
    const ImVec2 nameG = cjkTextSize(name, nullptr, false, std::max(nameB, 1.0F));
    const ImVec2 wertG = cjkTextSize(wert, nullptr, false, std::max(wertB, 1.0F));
    const float hoehe = darunter ? nameG.y + wertG.y : std::max(nameG.y, wertG.y);
    const ImVec2 ort = w->DC.CursorPos;
    const ImRect bb{ort, ImVec2{ort.x + breite, ort.y + hoehe}};
    // eine eigene Kennung je Zeile der Gruppe: Selbsttests finden die Zeile ueber ihre Beschriftung
    Lauf* l = laufJetzt();
    const int nr = l != nullptr ? l->zeilenNr++ : 0;
    const ImGuiID id = l != nullptr ? ImHashData(&nr, sizeof nr, ImHashStr("##wertzeile", 0, l->id)) : w->GetID(name);
    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, id, nullptr, ImGuiItemFlags_NoNav)) {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, name, 0);
        return;
    }
    ImDrawList* dl = w->DrawList;
    cjkAddText(dl, nullptr, 0.0F, ort, ImGui::GetColorU32(nameLeise ? ImGuiCol_TextDisabled : ImGuiCol_Text), name, nullptr, std::max(nameB, 1.0F));
    cjkAddText(dl, nullptr, 0.0F, ImVec2{std::floor(ort.x + breite - wertG.x), darunter ? ort.y + nameG.y : ort.y}, u32(tonFarbe(ton)), wert, nullptr, std::max(wertB, 1.0F));
    IMGUI_TEST_ENGINE_ITEM_INFO(id, name, g.LastItemData.StatusFlags);
    tooltipText(hinweis, nullptr);
}

}  // namespace

void wertTabelle(const WertZeile* zeilen, int zahl) {
    if (ImGui::GetCurrentWindow()->SkipItems) { return; }
    const ImGuiStyle& st = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{st.ItemSpacing.x, std::floor(ImGui::GetFontSize() * 0.22F)});
    for (int i = 0; i < zahl; ++i) { wertZeile(zeilen[i].name.c_str(), zeilen[i].wert.c_str(), zeilen[i].ton, zeilen[i].hinweis.c_str(), true); }
    ImGui::PopStyleVar();
    // nach der Tabelle wieder der uebliche Zeilenabstand
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (zahl > 0) { w->DC.CursorPos.y += st.ItemSpacing.y - std::floor(ImGui::GetFontSize() * 0.22F); }
}

void wertTabelle(std::initializer_list<WertZeile> zeilen) { wertTabelle(zeilen.begin(), static_cast<int>(zeilen.size())); }

void balken(const char* name, const char* wert, float anteil, Ton ton, const char* hinweis) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return; }
    const ImGuiStyle& st = ImGui::GetStyle();
    const float fs = ImGui::GetFontSize();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{st.ItemSpacing.x, std::floor(fs * 0.18F)});
    wertZeile(name, wert, ton, hinweis, false);
    ImGui::PopStyleVar();
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float hoehe = std::max(std::floor(fs * 0.28F), 3.0F);
    const ImVec2 ort = w->DC.CursorPos;
    const ImRect bb{ort, ImVec2{ort.x + breite, ort.y + hoehe}};
    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, 0)) { return; }
    ImDrawList* dl = w->DrawList;
    const float rund = hoehe * 0.5F;
    dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), rund);
    const float t = std::clamp(anteil, 0.0F, 1.0F);
    if (t > 0.0F) { dl->AddRectFilled(bb.Min, ImVec2{bb.Min.x + std::max(std::floor(breite * t), hoehe), bb.Max.y}, u32(tonFarbe(ton == Ton::Normal || ton == Ton::Leise ? Ton::Gut : ton)), rund); }
}

void grenzBalken(const char* name, int ist, int grenze, const char* hinweis) {
    char text[48];
    std::snprintf(text, sizeof text, "%d / %d", ist, grenze);
    const float anteil = grenze > 0 ? static_cast<float>(ist) / static_cast<float>(grenze) : 1.0F;
    // gruen bis zur Grenze (auch genau an ihr), gelb ab 90 % darunter, rot darueber
    const Ton ton = ist > grenze ? Ton::Fehler : (ist < grenze && anteil >= 0.9F) ? Ton::Warnung : Ton::Gut;
    balken(name, text, anteil, ton, hinweis);
}

// --- Eingaben -----------------------------------------------------------------------------------------

void zeile(const char* beschriftung, const char* hinweis) {
    if (ImGui::GetCurrentWindow()->SkipItems) { return; }
    const float fs = ImGui::GetFontSize();
    beschriftungZeichnen(beschriftung, hinweis, fs * 4.0F);
    z().feldB = std::max(std::floor(ImGui::GetContentRegionAvail().x), fs * 3.0F);
    ImGui::SetNextItemWidth(z().feldB);
}

void zeileOhne() {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return; }
    const Gruppe& g = gruppeJetzt();
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float spalte = std::min(g.spalte, std::floor(breite * 0.5F));
    if (breite - spalte >= ImGui::GetFontSize() * 4.0F) { w->DC.CursorPos.x += spalte; }
    z().feldB = std::max(std::floor(ImGui::GetContentRegionAvail().x), ImGui::GetFontSize() * 3.0F);
    ImGui::SetNextItemWidth(z().feldB);
}

float feldBreite() { return z().feldB; }

namespace {

bool reglerKern(const char* text, ImGuiDataType typ, void* wert, const void* von, const void* bis, const char* format, const char* hinweis, ImGuiSliderFlags flags,
                bool aktiv) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return false; }
    ImGuiContext& g = *GImGui;
    const float fs = ImGui::GetFontSize();
    // AP14 (Wunsch AP8): aktiv = false sperrt Beschriftung, Schieber und Wert; der Tooltip erscheint trotzdem
    if (!aktiv) { ImGui::BeginDisabled(); }
    // der Wert rechts neben dem Schieber, rechtsbuendig in einer gemeinsamen Spalte der Gruppe
    char wertText[64];
    ImGui::DataTypeFormatString(wertText, IM_ARRAYSIZE(wertText), typ, wert, format);
    const float wertB = std::ceil(ImGui::CalcTextSize(wertText).x);
    Gruppe& gr = gruppeJetzt();
    gr.wertNeu = std::max(gr.wertNeu, wertB);
    const float spalte = std::max({gr.wert, wertB, std::floor(fs * 2.2F)});
    const float luecke = std::floor(fs * 0.5F);
    beschriftungZeichnen(text, hinweis, spalte + luecke + fs * 4.0F);
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float schieber = std::max(breite - spalte - luecke, fs * 2.0F);
    ImGui::PushID(text);
    ImGui::SetNextItemWidth(schieber);
    // Die Zahl im Schieber bleibt unsichtbar (sie steht daneben); das Format behaelt er, damit Strg+Klick den Wert zum
    // Eintippen vorlegt - waehrend des Tippens ist der Text wieder sichtbar.
    const bool tippt = ImGui::TempInputIsActive(w->GetID("##regler"));
    if (!tippt) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0F, 0.0F, 0.0F, 0.0F}); }
    const bool geaendert = ImGui::SliderScalar("##regler", typ, wert, von, bis, format, flags);
    if (!tippt) { ImGui::PopStyleColor(); }
    const ImGuiID id = g.LastItemData.ID;
    const ImRect bb = g.LastItemData.Rect;
    IMGUI_TEST_ENGINE_ITEM_INFO(id, text, g.LastItemData.StatusFlags);
    tooltipText(hinweis, nullptr);
    ImGui::PopID();
    if (!tippt) {
        ImGui::DataTypeFormatString(wertText, IM_ARRAYSIZE(wertText), typ, wert, format);
        const float b = ImGui::CalcTextSize(wertText).x;
        w->DrawList->AddText(ImVec2{std::floor(bb.Max.x + luecke + spalte - b), std::floor(bb.Min.y + g.Style.FramePadding.y)}, ImGui::GetColorU32(ImGuiCol_Text), wertText);
    }
    if (!aktiv) { ImGui::EndDisabled(); }
    return geaendert && aktiv;
}

}  // namespace

bool regler(const char* text, float* wert, float von, float bis, const char* format, const char* hinweis, ImGuiSliderFlags flags, bool aktiv) {
    return reglerKern(text, ImGuiDataType_Float, wert, &von, &bis, format, hinweis, flags, aktiv);
}

bool reglerInt(const char* text, int* wert, int von, int bis, const char* format, const char* hinweis, ImGuiSliderFlags flags, bool aktiv) {
    return reglerKern(text, ImGuiDataType_S32, wert, &von, &bis, format, hinweis, flags, aktiv);
}

// --- Text ---------------------------------------------------------------------------------------------

void tipp(const char* text) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems || text == nullptr || text[0] == '\0') { return; }
    const float fs = ImGui::GetFontSize();
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float strich = std::max(std::floor(fs * 0.2F), 2.0F);
    const float randX = std::floor(fs * 0.5F);
    const float randY = std::floor(fs * 0.32F);
    const float iconG = std::floor(fs * 0.95F);
    const float luecke = std::floor(fs * 0.35F);
    const float textB = std::max(breite - strich - randX * 2.0F - iconG - luecke, fs);
    const ImVec2 textG = cjkTextSize(text, nullptr, false, textB);
    const float hoehe = std::floor(std::max(textG.y, iconG) + randY * 2.0F);
    const ImVec2 ort = w->DC.CursorPos;
    const ImRect bb{ort, ImVec2{ort.x + breite, ort.y + hoehe}};
    ImGui::ItemSize(bb.GetSize());
    // mit Kennung: Selbsttests finden den Tipp ueber seinen Text
    ImGuiContext& g = *GImGui;
    Lauf* l = laufJetzt();
    const ImGuiID id = ImHashStr(text, 0, l != nullptr ? l->id : w->ID);
    const bool da = ImGui::ItemAdd(bb, id, nullptr, ImGuiItemFlags_NoNav);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, text, 0);
    if (!da) { return; }
    ImDrawList* dl = w->DrawList;
    const ImVec4 a = betonung();
    ImVec4 grund = mischen(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg), a, hellesThema() ? 0.12F : 0.20F);
    grund.w = 1.0F;
    const float rund = std::min(rundung(), 4.0F);
    dl->AddRectFilled(bb.Min, bb.Max, u32(grund), rund);
    dl->AddRectFilled(bb.Min, ImVec2{bb.Min.x + strich, bb.Max.y}, u32(a), rund, ImDrawFlags_RoundCornersLeft);
    const float x = bb.Min.x + strich + randX;
    uiIcon(dl, UiIcon::Info, ImVec2{std::floor(x), std::floor(bb.Min.y + randY + (std::min(textG.y, fs) - iconG) * 0.5F)}, iconG, u32(a));
    cjkAddText(dl, nullptr, 0.0F, ImVec2{std::floor(x + iconG + luecke), bb.Min.y + randY}, ImGui::GetColorU32(ImGuiCol_Text, 0.82F), text, nullptr, textB);
}

void absatz(const char* text, Ton ton) {
    if (text == nullptr || text[0] == '\0') { return; }
    ImGui::PushTextWrapPos(0.0F);
    cjkTextColored(tonFarbe(ton), "%s", text);
    ImGui::PopTextWrapPos();
    // AP14 (Wunsch AP8): der Absatz meldet sich mit seinem Text im Element-Register - Selbsttests finden Warn- und Fehlerzeilen
    // ("3 Ecken tragen mehr als 4 Gewichte") ueber finde(text, fenster). Der Text selbst ist ein Element ohne Kennung; die
    // Kennung hier kommt aus der Gruppe und der laufenden Nummer der Zeile.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* w = g.CurrentWindow;
    if (!g.TestEngineHookItems || w->SkipItems) { return; }
    Lauf* l = laufJetzt();
    const int nr = l != nullptr ? l->zeilenNr++ : 0;
    const ImGuiID id = l != nullptr ? ImHashData(&nr, sizeof nr, ImHashStr("##absatz", 0, l->id)) : ImHashStr(text, 0, w->ID);
    const ImRect bb = g.LastItemData.Rect;
    IMGUI_TEST_ENGINE_ITEM_ADD(id, bb, &g.LastItemData);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, text, 0);
}

bool eintrag(const char* text, const char* entfernen) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { return false; }
    ImGuiContext& g = *GImGui;
    const float fs = ImGui::GetFontSize();
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), 1.0F);
    const float randX = std::floor(fs * 0.45F);
    const float randY = std::floor(fs * 0.22F);
    const float knopf = std::floor(fs * 1.25F);
    const float textB = std::max(breite - randX * 2.0F - knopf, fs);
    const ImVec2 textG = cjkTextSize(text, nullptr, false, textB);
    const float hoehe = std::floor(std::max(textG.y, knopf) + randY * 2.0F);
    const ImVec2 ort = w->DC.CursorPos;
    const ImRect bb{ort, ImVec2{ort.x + breite, ort.y + hoehe}};
    ImGui::ItemSize(bb.GetSize());
    const char* name = entfernen != nullptr && entfernen[0] != '\0' ? entfernen : "##entfernen";
    const ImGuiID id = w->GetID(name);
    ImDrawList* dl = w->DrawList;
    const float rund = rundung();
    if (ImGui::ItemAdd(bb, 0)) {
        dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), rund);
        dl->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border), rund);
        cjkAddText(dl, nullptr, 0.0F, ImVec2{bb.Min.x + randX, std::floor(bb.Min.y + (hoehe - textG.y) * 0.5F)}, ImGui::GetColorU32(ImGuiCol_Text), text, nullptr, textB);
    }
    // das "x" rechts
    const ImRect kb{ImVec2{bb.Max.x - randX * 0.5F - knopf, std::floor(bb.Min.y + (hoehe - knopf) * 0.5F)}, ImVec2{bb.Max.x - randX * 0.5F, std::floor(bb.Min.y + (hoehe - knopf) * 0.5F) + knopf}};
    bool geklickt = false;
    if (ImGui::ItemAdd(kb, id)) {
        bool ueber = false;
        bool gehalten = false;
        geklickt = ImGui::ButtonBehavior(kb, id, &ueber, &gehalten);
        if (ueber || gehalten) { dl->AddRectFilled(kb.Min, kb.Max, ImGui::GetColorU32(gehalten ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered), rund); }
        const float ig = std::floor(knopf * 0.6F);
        uiIcon(dl, UiIcon::Schliessen, ImVec2{std::floor(kb.Min.x + (knopf - ig) * 0.5F), std::floor(kb.Min.y + (knopf - ig) * 0.5F)}, ig,
               ImGui::GetColorU32(ueber ? ImGuiCol_Text : ImGuiCol_TextDisabled));
        ImGui::RenderNavCursor(kb, id);
        if (entfernen != nullptr && entfernen[0] != '\0' && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", entfernen); }
    }
    IMGUI_TEST_ENGINE_ITEM_INFO(id, name, g.LastItemData.StatusFlags);
    return geklickt;
}

// --- Selbsttest -----------------------------------------------------------------------------------------

const FeldProbe& feldProbe() { return z().feldProbe; }

int rolloutProbeZahl() { return static_cast<int>(z().probenLetztes.size()); }

const RolloutProbe& rolloutProbe(int nr) {
    static const RolloutProbe leer;
    const Zustand& s = z();
    return nr >= 0 && nr < static_cast<int>(s.probenLetztes.size()) ? s.probenLetztes[static_cast<std::size_t>(nr)] : leer;
}

const WortbruchProbe& wortbruchProbe() { return z().wortbruchLetztes; }

void rolloutBildAnfang() {
    Zustand& s = z();
    s.probenLetztes.swap(s.proben);
    s.proben.clear();
    s.wortbruchLetztes = std::move(s.wortbruch);
    s.wortbruch = WortbruchProbe{};
    // eine Gruppe ohne rolloutEnde() (Programmierfehler): ihr Zeichenkanal darf nicht geteilt bleiben
    for (int i = 0; i < s.tiefe; ++i) { s.lauf[i].teiler.Clear(); }
    s.tiefe = 0;
    s.raster.an = false;
    s.feld = Feld{};
    // ein Wunsch von rolloutZeigen, den drei Bilder lang keine Gruppe erfuellt hat (anderer Reiter, anderer Bereich), verfaellt
    if (s.zeigen != 0 && ImGui::GetFrameCount() - s.zeigenBild > 3) { s.zeigen = 0; }
}

}  // namespace bhed::gui::ui
