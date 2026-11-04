// app_lumpedit.cpp - rc609 J: Shader, Nebel und Flaechen fertiger Karten bearbeiten.
//
// shank (03.11.): "ja gerne" - beides, Klicken und Text. Vorbild sind die Lump-Reiter von
// SomaZ/BSP-Entity-Edit (MIT): je Eintrag Felder, Uebernehmen/Verwerfen. Dazu, was dort fehlt:
// Pruefung wie die Engine (karte::bspLumpsPruefen), Sicherung beim Speichern (Wege aus W2a,
// bspLumpsSpeichern) und Rueckgaengig.
//
//   Klicken  Fenster "Shader und Flaechen", Reiter "Flaeche": Fläche in 3D anklicken (Mission-Tab:
//            die Vorschau; Karten-Editor bei geoeffneter .bsp: die Kamera), Flaechenart, Shader,
//            Nebel; Shader tauschen aus den Shadern und Texturen des Spiels, fuer diese Flaeche
//            oder alle mit demselben Shader; Flags als Haekchen mit den Namen aus surfaceflags.h
//            des Spiels (bsplumps.cpp).
//   Text     Reiter "Lumps als Text": eine Zeile je Shader und je Nebel (karte::lumpsAlsText),
//            Uebernehmen mit Strg+Enter, Fehler mit Zeile, Suchen; ein Klick in eine Zeile hebt
//            die Flaechen in 3D hervor.
// Beide Wege aendern dasselbe Dokument (LumpDok), ein Rueckgaengig-Schritt je Uebernahme. Die
// Vorschau liest danach die Geometrie aus den neuen Bytes (karteGeometrieLesen, wie beim Laden).
#include "app_internal.h"
#include "fenster_dock.h"
#include "gpumap.h"
#include "karteneditor.h"

#include "bhed/bspgeo.h"
#include "bhed/diag.h"
#include "bhed/efxdraw.h"
#include "bhed/enttext.h"
#include "bhed/karte/bsplumps.h"
#include "bhed/pk3.h"
#include "bhed/regelwerk.h"
#include "bhed/shaderscript.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui {


namespace {

constexpr const char* kFeld = "##lumptextfeld";

struct LumpDok {
    bool editor = false;           // Karten-Editor (sonst Mission-Tab)
    std::string karte;             // map.path bzw. die Quelle im Karten-Editor
    std::string original;          // die Datei beim Laden
    std::string bytes;             // original mit den aktuellen Lumps
    karte::BspLumps lumps;
    karte::BspLumps basis;         // wie in original
    std::string lesefehler;
    std::vector<karte::BspLumps> undo;
    std::vector<karte::BspLumps> redo;
    bool ungespeichert = false;
    std::uint64_t stand = 1;
    // Wahl
    int flaeche = -1;
    int hervorShader = -1;
    int hervorNebel = -1;
    bool hervorIn3D = false;       // die Flaechen des Shaders/Nebels in 3D (nach einem Klick in eine Zeile)
    // Rueckmeldung
    std::string meldung;
    bool meldungFehler = false;
    std::string warnung;           // Lightmap/fogparms zum letzten Tausch (bis zur naechsten Wahl)
    // Neuer Shader
    std::string filter;
    std::string gewaehlt;
    // Text
    std::string text;              // aus den Lumps (Stand)
    std::string puffer;            // im Feld (UTF-8)
    bool textGeaendert = false;
    bool neuLaden = false;
    int fehlerZeile = -1;          // ab 0
    int springeZeile = -1;
    int springeBilder = 0;
    std::vector<std::size_t> zeilenAnfang;
    bool sucheOffen = false;
    bool sucheFokus = false;
    std::string suche;
    int trefferNr = -1;
    // Karten-Editor: Geometrie fuer die Strahlen (aus bytes)
    std::shared_ptr<const std::string> spielbildDaten;
    std::unique_ptr<BspGeometry> geo;
    // Lage (Selbsttest)
    ImVec2 ursprung{0, 0};
    float zeilenHoehe = 1.0F;
    ImRect feldRect;
    std::map<std::string, ImRect> knoepfe;
    std::string nutzungText;
};

std::map<std::string, LumpDok>& missionDoks() {
    static std::map<std::string, LumpDok> m;
    return m;
}
LumpDok& editorDok() {
    static LumpDok d;
    d.editor = true;
    return d;
}
LumpDok* missionDok() {
    auto& m = missionDoks();
    const auto it = m.find(g_app->map.path);
    return it == m.end() || g_app->map.empty() ? nullptr : &it->second;
}

struct Fenster {
    bool waehlen = true;           // Klick in 3D waehlt eine Flaeche
    bool fokus = false;
    bool feldAktiv = false;        // das Textfeld ist aktiv (letztes Bild) - auch ohne Fensterfokus (Strg+Enter)
    int reiter = 0;                // 0 Flaeche, 1 Text (angefordert vom Selbsttest)
    bool reiterSetzen = false;
};
Fenster& fe() {
    static Fenster f;
    return f;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
std::string textMit(const char* muster, int a) {
    char b[600];
    std::snprintf(b, sizeof(b), muster, a);
    return b;
}
std::string textMit(const char* muster, const std::string& a) {
    char b[900];
    std::snprintf(b, sizeof(b), muster, a.c_str());
    return b;
}

karte::FlagSpiel flagSpiel() {
    if (regelwerkJediOutcast()) { return karte::FlagSpiel::Jk2; }
    return regelwerkMovieDuels() ? karte::FlagSpiel::MovieDuels : karte::FlagSpiel::Jka;
}

// --- Regeln wie im Spiel ----------------------------------------------------------------------
// R_FindShader (tr_shader.cpp, OJK und MD9 gleich) mit lightmapIndex aus dsurface_t.lightmapNum
// (ParseFace/ParseMesh/ParseTriSurf ueber ShaderForShaderNum). Hat der Shader ein Skript, bringt
// NUR eine Stufe "map $lightmap" die Lightmap (ParseStage :1244: bei lightmapIndex < 0 das weisse
// Bild); ohne Skript baut FindShader die Stufen selbst passend zum lightmapIndex (:3515-3560:
// LIGHTMAP_NONE, LIGHTMAP_BY_VERTEX, sonst Lightmap mal Textur) - dann passt es immer.
// 0 passt, 1 Lightmap vorhanden aber keine $lightmap-Stufe, 2 keine Lightmap aber $lightmap-Stufe.
int lightmapRegel(int lightmap, const std::string& shader) {
    std::string n = klein(shader);
    const std::size_t punkt = n.find_last_of('.');   // COM_StripExtension in R_FindShader
    if (punkt != std::string::npos && n.find('/', punkt) == std::string::npos) { n.resize(punkt); }
    const auto it = g_app->shaderMap.find(n);
    if (it == g_app->shaderMap.end()) { return 0; }
    if (lightmap >= 0 && !it->second.lightmapStufe) { return 1; }
    if (lightmap < 0 && it->second.lightmapStufe) { return 2; }
    return 0;
}

std::string lightmapWarnung(int regel, int lightmap, const std::string& shader) {
    if (regel == 1) { return textMit(tr(Str::LeWarnKeineStufe), shader); }
    if (regel == 2) {
        char b[900];
        std::snprintf(b, sizeof(b), tr(Str::LeWarnWeiss), lightmap, shader.c_str());
        return b;
    }
    return {};
}

// R_LoadFogs: ein Nebelshader ohne fogparms wird rot (out->parms.color = 1, 0, 0).
bool nebelParmsDa(const std::string& shader) { return g_app->nebelMap.count(klein(shader)) != 0U; }

// --- Shader und Texturen des Spiels -------------------------------------------------------------
// Dieselbe Menge wie der Texturbrowser des Karten-Editors: die Shader aus den Skripten und die
// Bilder unter textures/ (ohne Endung, wie R_FindShader sie sucht).
const std::vector<std::string>& spielShader() {
    static std::vector<std::string> liste;
    static std::size_t shaderStand = 0;
    static std::size_t pfadStand = 0;
    if (!liste.empty() && shaderStand == g_app->shaderMap.size() && pfadStand == g_app->gamePaths.size()) { return liste; }
    shaderStand = g_app->shaderMap.size();
    pfadStand = g_app->gamePaths.size();
    std::set<std::string> s;
    for (const auto& [name, info] : g_app->shaderMap) {
        (void)info;
        // Nur Namen, wie sie in dshader_t stehen koennen (die Tabelle der Vorschau hat auch Reste
        // aus Zeilen ohne Namen, z. B. "cull twosided").
        if (name.find_first_of(" 	") == std::string::npos && name.size() <= karte::kShaderNameMax) { s.insert(name); }
    }
    for (const GamePath& gp : g_app->gamePaths) {
        for (const FoundFile& f : findByExtension(gp, {".jpg", ".tga", ".png"})) {
            std::string n = klein(f.name);
            if (n.rfind("textures/", 0) != 0) { continue; }
            n.resize(n.find_last_of('.'));
            s.insert(n);
        }
    }
    liste.assign(s.begin(), s.end());
    return liste;
}

// --- Text ---------------------------------------------------------------------------------------
void zeilenBerechnen(LumpDok& d) {
    d.zeilenAnfang.clear();
    d.zeilenAnfang.push_back(0);
    for (std::size_t i = 0; i < d.puffer.size(); ++i) {
        if (d.puffer[i] == '\n') { d.zeilenAnfang.push_back(i + 1); }
    }
}
int zeileVonByte(const LumpDok& d, std::size_t p) {
    const auto& a = d.zeilenAnfang;
    return static_cast<int>(std::upper_bound(a.begin(), a.end(), p) - a.begin()) - 1;
}
void zuZeile(LumpDok& d, int zeile) {
    d.springeZeile = zeile;
    d.springeBilder = 3;
}
void textNachLumps(LumpDok& d) {
    d.text = karte::lumpsAlsText(d.lumps);
    if (d.textGeaendert) {
        d.meldung = tr(Str::LeTextAussen);
        d.meldungFehler = true;
        return;
    }
    d.puffer = latin1NachUtf8(d.text);
    d.neuLaden = true;
    zeilenBerechnen(d);
}

// --- Vorschau -----------------------------------------------------------------------------------
void editorGeo(LumpDok& d) {
    d.geo = std::make_unique<BspGeometry>();
    if (!readBspGeometry(d.bytes, *d.geo, nullptr)) { d.geo.reset(); }
    d.spielbildDaten = std::make_shared<const std::string>(d.bytes);
}

void vorschau(LumpDok& d) {
    if (d.editor) {
        editorGeo(d);
        return;
    }
    if (d.karte != g_app->map.path) { return; }
    diag::Step s("Lumps: Vorschau neu (" + std::to_string(d.bytes.size()) + " Bytes)");
    kartenSzeneVergessen();
    gpu::vergissMoverNetze();
    vergissEffektBahnen();
    g_app->camTrackValid = false;
    (void)karteGeometrieLesen(d.bytes, d.karte);
    g_app->mapDirty = true;
}

bool gleicheLumps(const karte::BspLumps& a, const karte::BspLumps& b) {
    if (a.shader.size() != b.shader.size() || a.nebel.size() != b.nebel.size() || a.flaechen.size() != b.flaechen.size()) { return false; }
    for (std::size_t i = 0; i < a.shader.size(); ++i) {
        if (a.shader[i].name != b.shader[i].name || a.shader[i].surfaceFlags != b.shader[i].surfaceFlags || a.shader[i].contentFlags != b.shader[i].contentFlags) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.nebel.size(); ++i) {
        if (a.nebel[i].shader != b.nebel[i].shader || a.nebel[i].brushNum != b.nebel[i].brushNum || a.nebel[i].visibleSide != b.nebel[i].visibleSide) { return false; }
    }
    for (std::size_t i = 0; i < a.flaechen.size(); ++i) {
        if (a.flaechen[i].shaderNum != b.flaechen[i].shaderNum || a.flaechen[i].fogNum != b.flaechen[i].fogNum) { return false; }
    }
    return true;
}

void setzen(LumpDok& d, karte::BspLumps neu, std::string bytes) {
    // Wieder der Stand der Datei (Rueckgaengig bis zum Anfang): genau ihre Bytes
    if (gleicheLumps(neu, d.basis)) { bytes = d.original; }
    d.lumps = std::move(neu);
    d.bytes = std::move(bytes);
    d.ungespeichert = true;
    ++d.stand;
    textNachLumps(d);
    vorschau(d);
}

bool anwenden(LumpDok& d, karte::BspLumps neu, const std::string& was) {
    std::string f;
    std::string b = karte::bspLumpsSchreiben(d.original, neu, &f);
    if (b.empty()) {
        d.meldung = textMit(tr(Str::LeNichtUebernommen), f);
        d.meldungFehler = true;
        diag::warn("Lumps: nicht uebernommen (" + was + "): " + f);
        return false;
    }
    d.undo.push_back(d.lumps);
    d.redo.clear();
    setzen(d, std::move(neu), std::move(b));
    d.meldung = textMit(tr(Str::LeUebernommen), was);
    d.meldungFehler = false;
    diag::detail("Lumps: uebernommen - " + was + ", " + std::to_string(d.lumps.shader.size()) + " Shader");
    return true;
}

bool rueckgaengig(LumpDok& d, bool wieder) {
    std::vector<karte::BspLumps>& von = wieder ? d.redo : d.undo;
    std::vector<karte::BspLumps>& nach = wieder ? d.undo : d.redo;
    if (von.empty()) { return false; }
    karte::BspLumps l = std::move(von.back());
    von.pop_back();
    std::string b = karte::bspLumpsSchreiben(d.original, l, nullptr);
    if (b.empty()) { return false; }
    nach.push_back(d.lumps);
    setzen(d, std::move(l), std::move(b));
    d.meldung.clear();
    diag::detail(std::string("Lumps: ") + (wieder ? "wiederholt" : "rueckgaengig") + ", " + std::to_string(d.lumps.shader.size()) + " Shader");
    return true;
}

LumpDok* aktiv() {
    if (g_app->leftMode == 1) { return missionDok(); }
    if (g_app->leftMode == 3) {
        LumpDok& d = editorDok();
        return d.karte.empty() ? nullptr : &d;
    }
    return nullptr;
}

// --- Bedienung ----------------------------------------------------------------------------------
void flaecheWaehlen(LumpDok& d, int i) {
    d.flaeche = i >= 0 && static_cast<std::size_t>(i) < d.lumps.flaechen.size() ? i : -1;
    d.hervorShader = d.flaeche >= 0 ? d.lumps.flaechen[static_cast<std::size_t>(d.flaeche)].shaderNum : -1;
    d.hervorNebel = -1;
    d.hervorIn3D = false;
    d.warnung.clear();
    if (d.flaeche >= 0) {
        zuZeile(d, karte::lumpTextZeile(d.text, false, d.hervorShader) - 1);
        diag::detail("Lumps: Flaeche " + std::to_string(d.flaeche) + " gewaehlt, Shader " + std::to_string(d.hervorShader));
    }
}

// Shader der gewaehlten Flaeche (oder aller mit demselben Shader) gegen name tauschen. Der neue
// Eintrag uebernimmt die Flags des alten - geaendert wird nur, was gezeichnet wird.
bool tauschen(LumpDok& d, const std::string& name, bool alle) {
    if (d.flaeche < 0 || name.empty()) { return false; }
    karte::BspLumps n = d.lumps;
    const int alt = n.flaechen[static_cast<std::size_t>(d.flaeche)].shaderNum;
    const karte::LumpShader s = n.shader[static_cast<std::size_t>(alt)];
    const int neu = karte::shaderEintrag(n, name, s.surfaceFlags, s.contentFlags);
    int zahl = 0;
    int unpassend = 0;
    std::string warnung;
    for (std::size_t i = 0; i < n.flaechen.size(); ++i) {
        karte::LumpFlaeche& f = n.flaechen[i];
        if (!(alle ? f.shaderNum == alt : static_cast<int>(i) == d.flaeche)) { continue; }
        f.shaderNum = neu;
        ++zahl;
        const int r = lightmapRegel(f.lightmap, name);
        if (r != 0) {
            ++unpassend;
            if (warnung.empty() || static_cast<int>(i) == d.flaeche) { warnung = lightmapWarnung(r, f.lightmap, name); }
        }
    }
    const std::string was = alle ? textMit(tr(Str::LeWasAlle), zahl) : textMit(tr(Str::LeWasTausch), d.flaeche);
    if (!anwenden(d, std::move(n), was)) { return false; }
    d.hervorShader = neu;
    if (unpassend > 0) {
        d.warnung = warnung;
        if (alle) {
            char b[300];
            std::snprintf(b, sizeof(b), tr(Str::LeWarnAlle), unpassend, zahl);
            d.warnung += std::string("\n") + b;
        }
        diag::warn("Lumps: " + std::to_string(unpassend) + " von " + std::to_string(zahl) + " Flaechen passen nicht zum Licht von " + name);
    }
    return true;
}

bool textUebernehmen(LumpDok& d) {
    std::string l1;
    std::size_t stelle = 0;
    std::string zeichen;
    zeilenBerechnen(d);
    if (!utf8NachLatin1(d.puffer, l1, &stelle, &zeichen)) {
        d.fehlerZeile = zeileVonByte(d, stelle);
        char b[400];
        std::snprintf(b, sizeof(b), tr(Str::EtNichtLatin1), d.fehlerZeile + 1, zeichen.c_str());
        d.meldung = b;
        d.meldungFehler = true;
        zuZeile(d, d.fehlerZeile);
        return false;
    }
    karte::BspLumps n;
    karte::LumpTextFehler f;
    if (!karte::lumpsAusText(l1, d.lumps, n, &f)) {
        d.fehlerZeile = std::max(0, f.zeile - 1);
        char b[900];
        std::snprintf(b, sizeof(b), tr(Str::LeZeileFehler), f.zeile, f.text.c_str());
        d.meldung = b;
        d.meldungFehler = true;
        zuZeile(d, d.fehlerZeile);
        diag::detail("Lumps-Text: Fehler in Zeile " + std::to_string(f.zeile) + " (" + f.text + "), nichts uebernommen");
        return false;
    }
    d.fehlerZeile = -1;
    d.textGeaendert = false;
    if (karte::lumpsAlsText(n) == d.text) {   // nichts geaendert (nur Kommentare, Leerraum)
        textNachLumps(d);
        return true;
    }
    return anwenden(d, std::move(n), tr(Str::LeWasText));
}

// --- Zeichnen: Reiter Flaeche --------------------------------------------------------------------
const char* artName(int typ) {
    switch (typ) {
        case 1: return tr(Str::LeArtEbene);
        case 2: return tr(Str::LeArtPatch);
        case 3: return tr(Str::LeArtSuppe);
        case 4: return tr(Str::LeArtFlare);
        default: return tr(Str::LeArtSchlecht);
    }
}

void merkeKnopf(LumpDok& d, const char* name) { d.knoepfe[name] = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); }

// So viele Spalten, wie die laengsten Namen ganz zeigen (nichts abschneiden).
int flagSpalten(const std::vector<karte::FlagName>& namen) {
    float breit = 0.0F;
    for (const karte::FlagName& f : namen) { breit = std::max(breit, ImGui::CalcTextSize(f.name).x); }
    const ImGuiStyle& st = ImGui::GetStyle();
    const float zelle = breit + ImGui::GetFrameHeight() + st.ItemInnerSpacing.x + st.CellPadding.x * 2.0F + st.ItemSpacing.x;
    return std::clamp(static_cast<int>(ImGui::GetContentRegionAvail().x / std::max(1.0F, zelle)), 1, 4);
}

// Ein Flag-Haekchen; true, wenn umgeschaltet.
bool flagHaken(const char* name, std::uint32_t bit, std::uint32_t& wert) {
    bool an = (wert & bit) != 0U;
    if (!ImGui::Checkbox(name, &an)) { return false; }
    wert = an ? (wert | bit) : (wert & ~bit);
    return true;
}

void shaderEintragZeichnen(LumpDok& d, int nr) {
    if (nr < 0 || static_cast<std::size_t>(nr) >= d.lumps.shader.size()) { return; }
    const karte::LumpShader& s = d.lumps.shader[static_cast<std::size_t>(nr)];
    ImGui::SeparatorText(textMit(tr(Str::LeEintrag), nr).c_str());
    const float fs = ImGui::GetFontSize();
    // Name: Enter uebernimmt (betrifft alle Flaechen, Brushes und Seiten mit diesem Eintrag)
    static std::string nameFeld;
    static int nameFuer = -1;
    static std::uint64_t nameStand = 0;
    if (nameFuer != nr || nameStand != d.stand) {
        nameFeld = latin1NachUtf8(s.name);
        nameFuer = nr;
        nameStand = d.stand;
    }
    ImGui::SetNextItemWidth(fs * 24.0F);
    char puffer[256];
    std::snprintf(puffer, sizeof(puffer), "%s", nameFeld.c_str());
    if (ImGui::InputText(tr(Str::LeName), puffer, sizeof(puffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string l1;
        if (eingabeNachLatin1(puffer, l1) && l1 != s.name) {
            karte::BspLumps n = d.lumps;
            n.shader[static_cast<std::size_t>(nr)].name = l1;
            (void)anwenden(d, std::move(n), textMit(tr(Str::LeWasName), nr));
        }
    } else {
        nameFeld = puffer;
    }
    karte::FlagSpiel sp = flagSpiel();
    std::uint32_t sf = s.surfaceFlags;
    std::uint32_t cf = s.contentFlags;
    bool geaendert = false;
    // Material: die unteren 5 Bit (MATERIAL_MASK)
    const auto& mat = karte::materialNamen(sp);
    const std::uint32_t m = sf & karte::kMaterialMaske;
    ImGui::SetNextItemWidth(fs * 14.0F);
    if (ImGui::BeginCombo(tr(Str::LeMaterial), m < mat.size() ? mat[m].name : "?")) {
        for (const karte::FlagName& f : mat) {
            if (ImGui::Selectable(f.name, f.bit == m)) {
                sf = (sf & ~karte::kMaterialMaske) | f.bit;
                geaendert = true;
            }
        }
        ImGui::EndCombo();
    }
    char hex[32];
    std::snprintf(hex, sizeof(hex), "0x%08X", sf);
    ImGui::TextDisabled("%s %s", tr(Str::LeSurfaceFlags), hex);
    if (ImGui::BeginTable("##surfflags", flagSpalten(karte::surfaceFlagNamen(sp)), ImGuiTableFlags_SizingStretchSame)) {
        for (const karte::FlagName& f : karte::surfaceFlagNamen(sp)) {
            ImGui::TableNextColumn();
            geaendert |= flagHaken(f.name, f.bit, sf);
        }
        ImGui::EndTable();
    }
    std::snprintf(hex, sizeof(hex), "0x%08X", cf);
    ImGui::TextDisabled("%s %s", tr(Str::LeContentFlags), hex);
    if (ImGui::BeginTable("##contflags", flagSpalten(karte::contentFlagNamen(sp)), ImGuiTableFlags_SizingStretchSame)) {
        for (const karte::FlagName& f : karte::contentFlagNamen(sp)) {
            ImGui::TableNextColumn();
            geaendert |= flagHaken(f.name, f.bit, cf);
        }
        ImGui::EndTable();
    }
    if (geaendert) {
        karte::BspLumps n = d.lumps;
        n.shader[static_cast<std::size_t>(nr)].surfaceFlags = sf;
        n.shader[static_cast<std::size_t>(nr)].contentFlags = cf;
        (void)anwenden(d, std::move(n), textMit(tr(Str::LeWasFlags), nr));
    }
    // Kollision: Inhalt nur ueber Brushes und Patches (cm_load.cpp)
    const karte::ShaderNutzung u = karte::shaderNutzung(d.lumps, nr);
    char b[700];
    if (u.brushes + u.seiten + u.patches == 0) {
        std::snprintf(b, sizeof(b), "%s", tr(Str::LeKollisionKeine));
    } else {
        std::snprintf(b, sizeof(b), tr(Str::LeKollision), u.brushes, u.seiten, u.patches);
    }
    d.nutzungText = b;
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled("%s", b);
    if (regelwerkJediOutcast() && (s.surfaceFlags & 0x00004000U) != 0U && u.brushes > 0) {
        ImGui::TextDisabled("%s", tr(Str::LeJk2Slick));   // cm_load.cpp:272 JK2_MODE
    }
    ImGui::PopTextWrapPos();
}

void nebelZeichnen(LumpDok& d) {
    if (d.flaeche < 0) { return; }
    const karte::LumpFlaeche& fl = d.lumps.flaechen[static_cast<std::size_t>(d.flaeche)];
    ImGui::SeparatorText(tr(Str::LeNebel));
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextItemWidth(fs * 18.0F);
    const std::string jetzt = fl.fogNum < 0 ? std::string(tr(Str::LeNebelKeiner))
                                            : textMit(tr(Str::LeNebelEintrag), fl.fogNum) + ": " + d.lumps.nebel[static_cast<std::size_t>(fl.fogNum)].shader;
    if (ImGui::BeginCombo("##flnebel", jetzt.c_str())) {
        for (int i = -1; i < static_cast<int>(d.lumps.nebel.size()); ++i) {
            const std::string t = i < 0 ? std::string(tr(Str::LeNebelKeiner)) : textMit(tr(Str::LeNebelEintrag), i) + ": " + d.lumps.nebel[static_cast<std::size_t>(i)].shader;
            if (ImGui::Selectable(t.c_str(), i == fl.fogNum) && i != fl.fogNum) {
                karte::BspLumps n = d.lumps;
                n.flaechen[static_cast<std::size_t>(d.flaeche)].fogNum = i;
                (void)anwenden(d, std::move(n), textMit(tr(Str::LeWasFlNebel), d.flaeche));
            }
        }
        ImGui::EndCombo();
    }
    if (fl.fogNum < 0) { return; }
    const int nr = fl.fogNum;
    const karte::LumpNebel& f = d.lumps.nebel[static_cast<std::size_t>(nr)];
    static char shaderPuffer[128];
    static int brush = 0;
    static int seite = 0;
    static std::uint64_t stand = 0;
    static int fuer = -1;
    if (stand != d.stand || fuer != nr) {
        std::snprintf(shaderPuffer, sizeof(shaderPuffer), "%s", latin1NachUtf8(f.shader).c_str());
        brush = f.brushNum;
        seite = f.visibleSide;
        stand = d.stand;
        fuer = nr;
    }
    ImGui::SetNextItemWidth(fs * 18.0F);
    ImGui::InputText(tr(Str::LeName), shaderPuffer, sizeof(shaderPuffer));
    ImGui::SetNextItemWidth(fs * 6.0F);
    ImGui::InputInt(tr(Str::LeBrush), &brush);
    ImGui::SetNextItemWidth(fs * 6.0F);
    ImGui::InputInt(tr(Str::LeSeite), &seite);
    if (ImGui::Button(tr(Str::LeNebelUebernehmen))) {
        std::string l1;
        if (eingabeNachLatin1(shaderPuffer, l1)) {
            karte::BspLumps n = d.lumps;
            n.nebel[static_cast<std::size_t>(nr)] = {l1, brush, seite};
            (void)anwenden(d, std::move(n), textMit(tr(Str::LeWasNebel), nr));
        }
    }
    if (!nebelParmsDa(f.shader)) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", textMit(tr(Str::LeWarnKeinNebel), f.shader).c_str());
        ImGui::PopTextWrapPos();
    }
}

void neuerShaderZeichnen(LumpDok& d) {
    const karte::LumpFlaeche& fl = d.lumps.flaechen[static_cast<std::size_t>(d.flaeche)];
    const float fs = ImGui::GetFontSize();
    ImGui::SeparatorText(tr(Str::LeNeuerShader));
    char puffer[256];
    std::snprintf(puffer, sizeof(puffer), "%s", d.filter.c_str());
    ImGui::SetNextItemWidth(fs * 24.0F);
    if (ImGui::InputTextWithHint("##lefilter", tr(Str::LeFilter), puffer, sizeof(puffer))) { d.filter = puffer; }
    const std::string f = klein(d.filter);
    if (ImGui::BeginListBox("##leshaderliste", ImVec2{fs * 24.0F, fs * 7.5F})) {
        int gezeigt = 0;
        for (const std::string& n : spielShader()) {
            if (!f.empty() && n.find(f) == std::string::npos) { continue; }
            if (++gezeigt > 400) { break; }
            if (ImGui::Selectable(n.c_str(), n == d.gewaehlt)) { d.gewaehlt = n; }
        }
        ImGui::EndListBox();
    }
    if (d.editor) {
        // Im Karten-Editor: der im Texturbrowser gewaehlte Shader (TextureBrowser_GetSelectedShader)
        const std::string b = karteneditor::browserShader();
        if (!b.empty() && b != "textures/" && ImGui::SmallButton(textMit(tr(Str::LeAusBrowser), b).c_str())) { d.gewaehlt = b; }
    }
    if (!d.gewaehlt.empty()) {
        ImGui::TextUnformatted(d.gewaehlt.c_str());
        const int r = lightmapRegel(fl.lightmap, d.gewaehlt);
        if (r != 0) {
            ImGui::PushTextWrapPos(0.0F);
            ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", lightmapWarnung(r, fl.lightmap, d.gewaehlt).c_str());
            ImGui::PopTextWrapPos();
        }
    }
    ImGui::BeginDisabled(d.gewaehlt.empty());
    if (ImGui::Button(tr(Str::LeNurDiese))) { (void)tauschen(d, d.gewaehlt, false); }
    merkeKnopf(d, "nurdiese");
    ImGui::SameLine();
    const karte::ShaderNutzung u = karte::shaderNutzung(d.lumps, fl.shaderNum);
    if (ImGui::Button(textMit(tr(Str::LeAlleFlaechen), u.flaechen).c_str())) { (void)tauschen(d, d.gewaehlt, true); }
    merkeKnopf(d, "alle");
    ImGui::EndDisabled();
}

void flaecheReiter(LumpDok& d) {
    if (d.flaeche < 0) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::LeKeineFlaeche));
        ImGui::PopTextWrapPos();
        if (d.hervorShader >= 0) { shaderEintragZeichnen(d, d.hervorShader); }
        return;
    }
    const karte::LumpFlaeche& fl = d.lumps.flaechen[static_cast<std::size_t>(d.flaeche)];
    char b[400];
    std::snprintf(b, sizeof(b), tr(Str::LeFlaeche), d.flaeche, artName(fl.typ));
    ImGui::TextUnformatted(b);
    ImGui::SameLine();
    if (fl.lightmap >= 0) {
        ImGui::TextDisabled(tr(Str::LeLightmap), fl.lightmap);
    } else if (fl.lightmap == -3) {
        ImGui::TextDisabled("%s", tr(Str::LeVertexLicht));
    } else {
        ImGui::TextDisabled(tr(Str::LeKeinLicht), fl.lightmap);
    }
    std::snprintf(b, sizeof(b), tr(Str::LeShader), fl.shaderNum, d.lumps.shader[static_cast<std::size_t>(fl.shaderNum)].name.c_str());
    ImGui::TextUnformatted(b);
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled(tr(Str::LeFlaecheKollision), artName(fl.typ));   // CMod_LoadPatches
    if (const int r = lightmapRegel(fl.lightmap, d.lumps.shader[static_cast<std::size_t>(fl.shaderNum)].name); r != 0) {
        ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", lightmapWarnung(r, fl.lightmap, d.lumps.shader[static_cast<std::size_t>(fl.shaderNum)].name).c_str());
    }
    if (!d.warnung.empty()) { ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", d.warnung.c_str()); }
    ImGui::PopTextWrapPos();
    neuerShaderZeichnen(d);
    shaderEintragZeichnen(d, fl.shaderNum);
    nebelZeichnen(d);
}

// --- Zeichnen: Reiter Text -----------------------------------------------------------------------
int waechstText(ImGuiInputTextCallbackData* c) {
    if (c->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* t = static_cast<std::string*>(c->UserData);
        t->resize(static_cast<std::size_t>(c->BufTextLen));
        c->Buf = t->data();
    }
    return 0;
}

std::vector<std::size_t> treffer(const LumpDok& d) {
    std::vector<std::size_t> r;
    if (d.suche.empty()) { return r; }
    const std::string t = klein(d.puffer);
    const std::string q = klein(d.suche);
    for (std::size_t p = t.find(q); p != std::string::npos; p = t.find(q, p + q.size())) { r.push_back(p); }
    return r;
}

void weiter(LumpDok& d, int richtung) {
    const std::vector<std::size_t> t = treffer(d);
    if (t.empty()) {
        d.trefferNr = -1;
        return;
    }
    const int n = static_cast<int>(t.size());
    d.trefferNr = d.trefferNr < 0 ? (richtung > 0 ? 0 : n - 1) : (d.trefferNr + richtung + n) % n;
    zeilenBerechnen(d);
    const std::size_t p = t[static_cast<std::size_t>(d.trefferNr)];
    zuZeile(d, zeileVonByte(d, p));
    const ImGuiID id = ImGui::GetID(kFeld);
    if (ImGuiInputTextState* st = ImGui::GetInputTextState(id); st != nullptr && ImGui::GetActiveID() == id) {
        st->SetSelection(static_cast<int>(p), static_cast<int>(p + d.suche.size()));
        st->CursorFollow = true;
    }
}

// Die Zeile, die ein Klick trifft: den Eintrag hervorheben (Flaechen in 3D).
void zeileGewaehlt(LumpDok& d, int zeile) {
    bool nebel = false;
    const int nr = karte::lumpTextEintrag(d.text, zeile + 1, nebel);
    if (nr < 0) { return; }
    d.hervorShader = nebel ? -1 : nr;
    d.hervorNebel = nebel ? nr : -1;
    d.hervorIn3D = true;
    diag::detail(std::string("Lumps-Text: Zeile ") + std::to_string(zeile + 1) + " -> " + (nebel ? "Nebel " : "Shader ") + std::to_string(nr));
}

void textReiter(LumpDok& d) {
    if (ImGui::Button(tr(Str::LeUebernehmen))) { (void)textUebernehmen(d); }
    merkeKnopf(d, "uebernehmen");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::LeUebernehmenHint)); }
    ImGui::SameLine();
    ImGui::BeginDisabled(!d.textGeaendert);
    if (ImGui::Button(tr(Str::LeVerwerfen))) {
        d.textGeaendert = false;
        d.fehlerZeile = -1;
        d.meldung.clear();
        textNachLumps(d);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EtSuchen))) {
        d.sucheOffen = true;
        d.sucheFokus = true;
    }
    if (d.sucheOffen) {
        ImGui::SameLine();
        if (d.sucheFokus) {
            ImGui::SetKeyboardFocusHere();
            d.sucheFokus = false;
        }
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0F);
        if (ImGui::InputTextWithHint("##lesuche", tr(Str::EtSuchbegriff), d.suche.data(), d.suche.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
                                     waechstText, &d.suche)) {
            d.trefferNr = -1;
        }
        if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) { weiter(d, 1); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::EtWeiter))) { weiter(d, 1); }
        ImGui::SameLine();
        ImGui::TextDisabled(tr(Str::EtTreffer), d.trefferNr + 1, static_cast<int>(treffer(d).size()));
    }
    // Das Feld mit Zeilennummern links
    zeilenBerechnen(d);
    ImFont* font = ImGui::GetFont();
    const float fsz = ImGui::GetFontSize();
    char nr[16];
    std::snprintf(nr, sizeof(nr), "%d", static_cast<int>(d.zeilenAnfang.size()));
    const float nummernB = font->CalcTextSizeA(fsz, 1e9F, 0.0F, nr).x + fsz * 0.8F;
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 platz = ImGui::GetContentRegionAvail();
    ImGui::SetCursorScreenPos(ImVec2{start.x + nummernB, start.y});
    const ImGuiID id = ImGui::GetID(kFeld);
    ImGuiWindow* fenster = ImGui::GetCurrentWindow();
    char kn[512];
    std::snprintf(kn, sizeof(kn), "%s/%s_%08X", fenster->Name, kFeld, id);
    if (d.springeZeile >= 0 && d.springeBilder > 0) {
        if (ImGuiWindow* kind = ImGui::FindWindowByName(kn)) {
            ImGui::SetScrollY(kind, std::max(0.0F, static_cast<float>(d.springeZeile) * ImGui::GetTextLineHeight() - kind->InnerClipRect.GetHeight() * 0.3F));
        }
        if (--d.springeBilder == 0) { d.springeZeile = -1; }
    }
    const bool feldAktiv = ImGui::GetActiveID() == id;
    fe().feldAktiv = feldAktiv;
    // Strg+Enter uebernimmt; das Feld bleibt aktiv (wie im Entity-Text: imgui_widgets.cpp is_ctrl_enter)
    if (feldAktiv && ImGui::GetIO().KeyCtrl) {
        for (const ImGuiKey k : {ImGuiKey_Enter, ImGuiKey_KeypadEnter}) {
            if (ImGui::IsKeyPressed(k, false)) { ImGui::SetKeyOwner(k, ImGui::GetID("##lumpenter")); }
        }
    }
    if (d.neuLaden) {
        if (ImGuiInputTextState* st = ImGui::GetInputTextState(id); st != nullptr && feldAktiv) { st->ReloadUserBufAndKeepSelection(); }
        d.neuLaden = false;
    }
    const bool neu = ImGui::InputTextMultiline(kFeld, d.puffer.data(), d.puffer.capacity() + 1, ImVec2{platz.x - nummernB, platz.y},
                                               ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize, waechstText, &d.puffer);
    d.feldRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    const bool geklickt = ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    if (neu) {
        d.textGeaendert = true;
        d.fehlerZeile = -1;
        d.meldung.clear();
        zeilenBerechnen(d);
    }
    ImGuiWindow* kind = ImGui::FindWindowByName(kn);
    if (kind == nullptr) { return; }
    const float zh = ImGui::GetTextLineHeight();
    d.zeilenHoehe = zh;
    ImVec2 o = kind->DC.CursorStartPos;
    ImGuiInputTextState* st = ImGui::GetInputTextState(id);
    if (st != nullptr) { o.x -= st->Scroll.x; }
    d.ursprung = o;
    if (geklickt && st != nullptr) { zeileGewaehlt(d, zeileVonByte(d, static_cast<std::size_t>(std::max(st->GetCursorPos(), 0)))); }
    const int zeilen = static_cast<int>(d.zeilenAnfang.size());
    const int erste = std::max(0, static_cast<int>((kind->InnerClipRect.Min.y - o.y) / zh));
    const int letzte = std::min(zeilen, static_cast<int>((kind->InnerClipRect.Max.y - o.y) / zh) + 1);
    ImDrawList* dl = kind->DrawList;
    dl->PushClipRect(kind->InnerClipRect.Min, kind->InnerClipRect.Max, true);
    const float links = kind->InnerClipRect.Min.x;
    const float rechts = kind->InnerClipRect.Max.x;
    const auto zeileHinterlegen = [&](int z, ImU32 farbe) {
        if (z < erste || z >= letzte) { return; }
        const float y0 = o.y + static_cast<float>(z) * zh;
        dl->AddRectFilled(ImVec2{links, y0}, ImVec2{rechts, y0 + zh}, farbe);
    };
    if (d.hervorShader >= 0) { zeileHinterlegen(karte::lumpTextZeile(d.text, false, d.hervorShader) - 1, IM_COL32(40, 120, 160, 90)); }
    if (d.hervorNebel >= 0) { zeileHinterlegen(karte::lumpTextZeile(d.text, true, d.hervorNebel) - 1, IM_COL32(60, 80, 200, 90)); }
    if (d.fehlerZeile >= 0) { zeileHinterlegen(d.fehlerZeile, IM_COL32(170, 40, 40, 130)); }
    if (!d.suche.empty() && d.sucheOffen) {
        for (const std::size_t p : treffer(d)) {
            const int zl = zeileVonByte(d, p);
            if (zl < erste || zl >= letzte) { continue; }
            const char* za = d.puffer.c_str() + d.zeilenAnfang[static_cast<std::size_t>(zl)];
            const float x0 = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, za, d.puffer.c_str() + p).x;
            const float x1 = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, za, d.puffer.c_str() + p + d.suche.size()).x;
            const float y = o.y + static_cast<float>(zl) * zh;
            dl->AddRectFilled(ImVec2{x0, y}, ImVec2{x1, y + zh}, IM_COL32(200, 180, 60, 110));
        }
    }
    dl->PopClipRect();
    ImDrawList* ad = ImGui::GetWindowDrawList();
    ad->PushClipRect(ImVec2{start.x, kind->InnerClipRect.Min.y}, ImVec2{start.x + nummernB, kind->InnerClipRect.Max.y}, true);
    for (int i = erste; i < letzte; ++i) {
        char n[16];
        std::snprintf(n, sizeof(n), "%d", i + 1);
        const float b = font->CalcTextSizeA(fsz, 1e9F, 0.0F, n).x;
        ad->AddText(font, fsz, ImVec2{start.x + nummernB - b - fsz * 0.4F, o.y + static_cast<float>(i) * zh},
                    i == d.fehlerZeile ? IM_COL32(255, 110, 100, 255) : IM_COL32(130, 130, 130, 255), n);
    }
    ad->PopClipRect();
}

// --- Fenster --------------------------------------------------------------------------------------
void tasten(LumpDok& d) {
    if (!fe().fokus && !fe().feldAktiv) { return; }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Enter) || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_KeypadEnter)) {
        (void)textUebernehmen(d);
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F)) {
        d.sucheOffen = true;
        d.sucheFokus = true;
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_F3)) { weiter(d, 1); }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Shift | ImGuiKey_F3)) { weiter(d, -1); }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { d.sucheOffen = false; }
}

void speichern(LumpDok& d);

void inhalt(LumpDok* dp) {
    fe().fokus = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (dp == nullptr) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(g_app->leftMode == 3 ? Str::LeKarteKeineBsp : Str::EtKeineKarte));
        ImGui::PopTextWrapPos();
        return;
    }
    LumpDok& d = *dp;
    if (!d.lesefehler.empty()) {
        ImGui::TextColored(ImVec4{1.0F, 0.45F, 0.4F, 1.0F}, "%s", textMit(tr(Str::LeKeineBsp), d.lesefehler).c_str());
        return;
    }
    d.knoepfe.clear();
    tasten(d);
    ImGui::Checkbox(tr(Str::LeWaehlen), &fe().waehlen);
    merkeKnopf(d, "waehlen");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::LeWaehlenHint)); }
    ImGui::SameLine();
    ImGui::BeginDisabled(d.undo.empty());
    if (ImGui::Button(tr(Str::LeRueckgaengig))) { (void)rueckgaengig(d, false); }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(d.redo.empty());
    if (ImGui::Button(tr(Str::LeWiederholen))) { (void)rueckgaengig(d, true); }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::LeSpeichern))) { speichern(d); }
    merkeKnopf(d, "speichern");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::LeSpeichernHint)); }
    if (d.ungespeichert) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4{1.0F, 0.8F, 0.4F, 1.0F}, "%s", tr(Str::LeUngespeichert));
    }
    if (d.editor) {
        ImGui::TextDisabled("%s", textMit(tr(Str::LeKarteBsp), d.karte).c_str());
        bool zeigen = karteneditor::lumpSpielbildAn();
        if (ImGui::Checkbox(tr(Str::LeSpielbild), &zeigen)) { karteneditor::lumpSpielbildSetzen(zeigen); }
    }
    if (!d.meldung.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextColored(d.meldungFehler ? ImVec4{1.0F, 0.45F, 0.4F, 1.0F} : ImVec4{0.6F, 0.85F, 0.6F, 1.0F}, "%s", d.meldung.c_str());
        ImGui::PopTextWrapPos();
    }
    if (ImGui::BeginTabBar("##lumpreiter")) {
        // Angeforderter Reiter (Selbsttest, Handbuch) - vorher merken: der gerade offene setzt fe().reiter neu
        const int gewollt = fe().reiterSetzen ? fe().reiter : -1;
        fe().reiterSetzen = false;
        if (ImGui::BeginTabItem(tr(Str::LeReiterFlaeche), nullptr, gewollt == 0 ? ImGuiTabItemFlags_SetSelected : 0)) {
            fe().reiter = 0;
            fe().feldAktiv = false;
            ImGui::BeginChild("##lumpflaeche");
            flaecheReiter(d);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(tr(Str::LeReiterText), nullptr, gewollt == 1 ? ImGuiTabItemFlags_SetSelected : 0)) {
            fe().reiter = 1;
            textReiter(d);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void speichern(LumpDok& d) {
    if (d.editor) {
        karteneditor::lumpBspSpeichern(d.bytes, [] {
            editorDok().ungespeichert = false;
            editorDok().meldung = tr(Str::LeGespeichert);
            editorDok().meldungFehler = false;
        });
        return;
    }
    const std::string karte = d.karte;
    (void)bspLumpsSpeichern(d.lumps, [karte] {
        auto& m = missionDoks();
        if (const auto it = m.find(karte); it != m.end()) {
            it->second.ungespeichert = false;
            it->second.meldung = tr(Str::LeGespeichert);
            it->second.meldungFehler = false;
        }
    });
}

// --- 3D: Flaechen hervorheben -------------------------------------------------------------------
// Die Kanten der Flaeche (Dreiecke; ein Patch als Gitter seiner Kontrollpunkte) mit der Abbildung
// punkt -> Bildschirm. Gibt die Zahl der gezeichneten Kanten zurueck.
template <class Abbild>
int flaecheZeichnen(const BspGeometry& g, std::size_t i, ImDrawList* dl, ImU32 farbe, float dicke, const Abbild& abbild) {
    if (i >= g.surfaces.size()) { return 0; }
    const BspSurface& s = g.surfaces[i];
    int kanten = 0;
    const auto linie = [&](int a, int b) {
        if (a < 0 || b < 0 || static_cast<std::size_t>(a) >= g.verts.size() || static_cast<std::size_t>(b) >= g.verts.size()) { return; }
        ImVec2 pa;
        ImVec2 pb;
        if (!abbild(g.verts[static_cast<std::size_t>(a)].xyz, pa) || !abbild(g.verts[static_cast<std::size_t>(b)].xyz, pb)) { return; }
        dl->AddLine(pa, pb, farbe, dicke);
        ++kanten;
    };
    if (s.type == BspSurface::Type::Patch && s.patchWidth > 0 && s.patchHeight > 0) {
        for (int y = 0; y < s.patchHeight; ++y) {
            for (int x = 0; x < s.patchWidth; ++x) {
                const int p = s.firstVert + y * s.patchWidth + x;
                if (x + 1 < s.patchWidth) { linie(p, p + 1); }
                if (y + 1 < s.patchHeight) { linie(p, p + s.patchWidth); }
            }
        }
        return kanten;
    }
    for (int k = 0; k + 2 < s.numIndexes; k += 3) {
        const std::size_t b = static_cast<std::size_t>(s.firstIndex + k);
        if (b + 2 >= g.indexes.size()) { break; }
        const int v0 = s.firstVert + static_cast<int>(g.indexes[b]);
        const int v1 = s.firstVert + static_cast<int>(g.indexes[b + 1]);
        const int v2 = s.firstVert + static_cast<int>(g.indexes[b + 2]);
        linie(v0, v1);
        linie(v1, v2);
        linie(v2, v0);
    }
    return kanten;
}

template <class Abbild>
void hervorhebungZeichnen(const LumpDok& d, const BspGeometry& g, ImDrawList* dl, const Abbild& abbild) {
    const float px = std::max(1.0F, ImGui::GetFontSize() / 14.0F);
    int kanten = 0;
    constexpr int kGrenze = 60000;   // bei riesigen Shadern nicht das ganze Bild zuzeichnen
    if (d.hervorIn3D && (d.hervorShader >= 0 || d.hervorNebel >= 0)) {
        for (std::size_t i = 0; i < d.lumps.flaechen.size() && kanten < kGrenze; ++i) {
            const karte::LumpFlaeche& f = d.lumps.flaechen[i];
            if (static_cast<int>(i) == d.flaeche) { continue; }
            if ((d.hervorShader >= 0 && f.shaderNum == d.hervorShader) || (d.hervorNebel >= 0 && f.fogNum == d.hervorNebel)) {
                kanten += flaecheZeichnen(g, i, dl, d.hervorNebel >= 0 ? IM_COL32(110, 140, 255, 200) : IM_COL32(60, 210, 230, 200), px, abbild);
            }
        }
    }
    if (d.flaeche >= 0) { (void)flaecheZeichnen(g, static_cast<std::size_t>(d.flaeche), dl, IM_COL32(255, 170, 40, 255), 2.0F * px, abbild); }
}

}  // namespace

// --- Nach aussen: Mission-Tab -------------------------------------------------------------------------

const std::string& lumpEditKarteGeladen(const std::string& karte, const std::string& bytes) {
    auto& m = missionDoks();
    for (auto it = m.begin(); it != m.end();) {   // nur Karten mit ungespeicherten Aenderungen bleiben
        it = (!it->second.ungespeichert && it->first != karte) ? m.erase(it) : std::next(it);
    }
    if (const auto it = m.find(karte); it != m.end() && it->second.original == bytes) { return it->second.bytes; }
    LumpDok d;
    d.karte = karte;
    d.original = bytes;
    d.bytes = bytes;
    if (!karte::bspLumpsLesen(bytes, d.lumps, &d.lesefehler)) { diag::warn("Lumps: " + karte + " - " + d.lesefehler); }
    d.basis = d.lumps;
    textNachLumps(d);
    LumpDok& neu = m[karte] = std::move(d);
    return neu.bytes;
}

void lumpEditVergessen(const std::string& karte) { missionDoks().erase(karte); }

bool lumpEditUngespeichert(const std::string& karte) {
    const auto it = missionDoks().find(karte);
    return it != missionDoks().end() && it->second.ungespeichert;
}

bool lumpEditBytes(const std::string& karte, std::string& out) {
    const auto it = missionDoks().find(karte);
    if (it == missionDoks().end() || !it->second.ungespeichert) { return false; }
    out = it->second.bytes;
    return true;
}

bool lumpEditHatFokus() {
    return ((g_app->leftMode == 1 && g_app->settings.lumpEditOffen) || (g_app->leftMode == 3 && g_app->settings.lumpEditKarteOffen)) &&
           (fe().fokus || fe().feldAktiv);
}

bool lumpEditRueckgaengig() {
    LumpDok* d = aktiv();
    return d != nullptr && rueckgaengig(*d, false);
}
bool lumpEditWiederholen() {
    LumpDok* d = aktiv();
    return d != nullptr && rueckgaengig(*d, true);
}

void drawLumpEditFenster() {
    const bool mission = g_app->leftMode == 1 && g_app->settings.lumpEditOffen;
    const bool editor = g_app->leftMode == 3 && g_app->settings.lumpEditKarteOffen;
    if (!mission && !editor) {
        fe().fokus = false;
        fe().feldAktiv = false;
        return;
    }
    if (editor) { karteneditor::lumpDokPruefen(); }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 34.0F, fs * 40.0F}, ImGuiCond_FirstUseEver);
    if (dock::fensterBeginnen(mission ? "mission.lumps" : "karte.lumps")) { inhalt(aktiv()); }
    dock::fensterEnde();
}

// Klick in die Vorschau des Mission-Tabs: die Flaeche unter der Maus (traceRay wie die Spuren).
void lumpEditMaus() {
    LumpDok* d = missionDok();
    if (d == nullptr || !g_app->settings.lumpEditOffen || !fe().waehlen || g_app->wegKlickVerbraucht || g_app->camViewActive || g_app->geo.empty()) {
        return;
    }
    const ImGuiIO& io = ImGui::GetIO();
    const bool imBild = io.MousePos.x >= g_app->kartenAnsicht[0] && io.MousePos.x < g_app->kartenAnsicht[2] && io.MousePos.y >= g_app->kartenAnsicht[1] &&
                        io.MousePos.y < g_app->kartenAnsicht[3] && ImGui::IsWindowHovered();
    if (!imBild || !ImGui::IsMouseClicked(ImGuiMouseButton_Left) || io.KeyCtrl || io.KeyShift || io.KeyAlt) { return; }
    TraceTreffer t;
    if (!kartenStrahlUnterMaus(io.MousePos.x, io.MousePos.y, t) || t.surface < 0) { return; }
    g_app->wegKlickVerbraucht = true;
    flaecheWaehlen(*d, t.surface);
}

void lumpEditUeberlagerung() {
    const LumpDok* d = missionDok();
    if (d == nullptr || !g_app->settings.lumpEditOffen || g_app->geo.empty() || g_app->geo.surfaces.size() != d->lumps.flaechen.size()) { return; }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}, true);
    hervorhebungZeichnen(*d, g_app->geo, dl, [](const float* w, ImVec2& out) { return kartenPunktAufSchirm(w, out); });
    dl->PopClipRect();
}

// --- Nach aussen: Karten-Editor ------------------------------------------------------------------
void lumpEditKarteSetzen(const std::string& quelle, const std::string& bytes) {
    LumpDok& d = editorDok();
    if (d.karte == quelle && d.original == bytes) { return; }
    const bool editor = d.editor;
    d = LumpDok{};
    d.editor = editor;
    d.karte = quelle;
    d.original = bytes;
    d.bytes = bytes;
    if (!quelle.empty() && !karte::bspLumpsLesen(bytes, d.lumps, &d.lesefehler)) { diag::warn("Lumps (Karten-Editor): " + quelle + " - " + d.lesefehler); }
    d.basis = d.lumps;
    textNachLumps(d);
    if (!quelle.empty()) { editorGeo(d); }
}

bool lumpEditKarteWahl() { return g_app->leftMode == 3 && g_app->settings.lumpEditKarteOffen && fe().waehlen && !editorDok().karte.empty(); }

bool lumpEditKarteKlick(const float start[3], const float ende[3]) {
    LumpDok& d = editorDok();
    if (!d.geo) { return false; }
    const TraceTreffer t = traceRay(*d.geo, start, ende);
    if (!t.hit || t.surface < 0) { return false; }
    flaecheWaehlen(d, t.surface);
    return true;
}

bool lumpEditKarteSpielbild(std::shared_ptr<const std::string>& daten, std::uint64_t& nummer) {
    LumpDok& d = editorDok();
    if (d.karte.empty() || !d.spielbildDaten) { return false; }
    daten = d.spielbildDaten;
    nummer = 0x4C554D5000000000ULL + d.stand;   // "LUMP" + Stand: eigene Nummern fuer das Spielbild
    return true;
}

void lumpEditKarteUeberlagerung(const std::function<bool(const float*, ImVec2&)>& abbild) {
    const LumpDok& d = editorDok();
    if (!d.geo || !g_app->settings.lumpEditKarteOffen || d.geo->surfaces.size() != d.lumps.flaechen.size()) { return; }
    hervorhebungZeichnen(d, *d.geo, ImGui::GetWindowDrawList(), abbild);
}

// --- Selbsttest -------------------------------------------------------------------------------------
namespace {
LumpDok* testDok() { return aktiv(); }
}  // namespace
int lumpEditFlaeche() { return testDok() != nullptr ? testDok()->flaeche : -1; }
void lumpEditFlaecheSetzen(int i) {
    if (LumpDok* d = testDok()) { flaecheWaehlen(*d, i); }
}
bool lumpEditTauschen(const std::string& name, bool alle) {
    LumpDok* d = testDok();
    return d != nullptr && tauschen(*d, name, alle);
}
bool lumpEditFlagsSetzen(int shader, std::uint32_t surfaceFlags, std::uint32_t contentFlags) {
    LumpDok* d = testDok();
    if (d == nullptr || shader < 0 || static_cast<std::size_t>(shader) >= d->lumps.shader.size()) { return false; }
    karte::BspLumps n = d->lumps;
    n.shader[static_cast<std::size_t>(shader)].surfaceFlags = surfaceFlags;
    n.shader[static_cast<std::size_t>(shader)].contentFlags = contentFlags;
    return anwenden(*d, std::move(n), textMit(tr(Str::LeWasFlags), shader));
}
const karte::BspLumps* lumpEditLumps() { return testDok() != nullptr ? &testDok()->lumps : nullptr; }
const std::string& lumpEditDateiBytes() {
    static const std::string leer;
    return testDok() != nullptr ? testDok()->bytes : leer;
}
const std::string& lumpEditPuffer() {
    static const std::string leer;
    return testDok() != nullptr ? testDok()->puffer : leer;
}
void lumpEditPufferSetzen(const std::string& utf8) {
    if (LumpDok* d = testDok()) {
        d->puffer = utf8;
        d->neuLaden = true;
        d->textGeaendert = true;
        zeilenBerechnen(*d);
    }
}
bool lumpEditTextUebernehmen() {
    LumpDok* d = testDok();
    return d != nullptr && textUebernehmen(*d);
}
int lumpEditFehlerZeile() { return testDok() != nullptr ? testDok()->fehlerZeile : -1; }
const std::string& lumpEditMeldung() {
    static const std::string leer;
    return testDok() != nullptr ? testDok()->meldung : leer;
}
const std::string& lumpEditWarnung() {
    static const std::string leer;
    return testDok() != nullptr ? testDok()->warnung : leer;
}
const std::string& lumpEditNutzungText() {
    static const std::string leer;
    return testDok() != nullptr ? testDok()->nutzungText : leer;
}
int lumpEditHervorShader() { return testDok() != nullptr ? testDok()->hervorShader : -1; }
std::size_t lumpEditRueckgaengigZahl() { return testDok() != nullptr ? testDok()->undo.size() : 0U; }
bool lumpEditIstUngespeichert() { return testDok() != nullptr && testDok()->ungespeichert; }
void lumpEditSpeichern() {
    if (LumpDok* d = testDok()) { speichern(*d); }
}
void lumpEditReiter(int r) {
    fe().reiter = r;
    fe().reiterSetzen = true;
}
void lumpEditWaehlenSetzen(bool an) { fe().waehlen = an; }
ImVec2 lumpEditZeilenOrt(int zeile) {
    const LumpDok* d = testDok();
    if (d == nullptr) { return ImVec2{-1.0F, -1.0F}; }
    const float y = d->ursprung.y + (static_cast<float>(zeile) + 0.5F) * d->zeilenHoehe;
    if (y < d->feldRect.Min.y || y > d->feldRect.Max.y) { return ImVec2{-1.0F, -1.0F}; }
    return ImVec2{d->ursprung.x + ImGui::GetFontSize() * 2.0F, y};
}
ImVec2 lumpEditKnopfOrt(const std::string& name) {
    const LumpDok* d = testDok();
    if (d == nullptr) { return ImVec2{-1.0F, -1.0F}; }
    const auto it = d->knoepfe.find(name);
    return it == d->knoepfe.end() ? ImVec2{-1.0F, -1.0F} : it->second.GetCenter();
}
std::string lumpEditText() { return testDok() != nullptr ? testDok()->text : std::string(); }
int lumpEditLightmapRegel(int lightmap, const std::string& shader) { return lightmapRegel(lightmap, shader); }

}  // namespace bhed::gui
