// app_menue.cpp - Menue-Editor M1: der Arbeitsbereich "Menu Editor".
//
// Eigener Code. Die Engine-Logik (Lesen, Zeichnen, Skripte) steckt in
// src/menue (portiert aus OpenJK, siehe dort); hier nur die Oberflaeche:
//   links    Bauteile (Palette) und Gliederung
//   Mitte    Seiten als Reiter, Arbeitsflaeche 640x480 mit Linealen,
//            Raster, Einrasten, Auswahl/Ziehen/Groesse, Interaktionsmodus
//   rechts   Eigenschaften, Cvars (nur Vorschau), Meldungen der Engine,
//            Skriptprotokoll
//   unten    Code-Editor (Farben, Zeilennummern, Lesezeichen- und
//            Aenderungsrand wie im Skript-Editor, Falten, Minimap,
//            Klammerpaare, Mehrfachcursor, Suchen/Ersetzen, Vervollstaendigung,
//            Gehe zu Definition)
// Text und Vorschau sind dasselbe: jede Aenderung an der Flaeche ersetzt nur
// ihre Spanne im Text, jede Textaenderung liest das Menue neu.
#include "menueeditor.h"

#include "app_internal.h"
#include "backend.h"
#include "platform.h"

#include "bhed/i18n.h"
#include "bhed/image.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/skript.h"
#include "bhed/menue/spieldaten.h"
#include "bhed/menue/textpuffer.h"
#include "bhed/menue/vorschau.h"
#include "bhed/menue/werkzeuge.h"
#include "bhed/pk3.h"
#include "bhed/regelwerk.h"
#include "bhed/shaderscript.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>

namespace bhed::gui::menueeditor {

using namespace bhed::menue;
using i18n::Str;
namespace fs = std::filesystem;

namespace {

// --- Zustand ---------------------------------------------------------------

struct Dokument {
    std::string name;          // Spielpfad (ui/main.menu) - bestimmt Dialekt
    std::string platte;        // Datei auf der Platte ("" = aus dem Spiel)
    Textpuffer puffer;
    std::string geladen;       // Text beim Oeffnen
    std::string gespeichert;   // Text beim letzten Speichern
    Datei datei;
    int geparst = -1;          // Pufferversion des Modells
    std::set<int> lesezeichen;
    std::set<int> gefaltet;    // Anfangszeilen eingeklappter Bloecke
    std::set<int> gesperrt;    // Items, die die Flaeche nicht bewegt
    int seite = 0;
    int seiteGezeigt = -1;     // welcher Reiter zuletzt offen war
    std::vector<int> auswahl;  // Items im Menue der Seite
    float scrollZeile = 0;
    int springeZeile = -1;
    std::map<std::string, std::string> bilder;   // eigene Grafiken: Spielpfad -> Bytes
};

struct Textur {
    void* tex = nullptr;
    int w = 0, h = 0;
    bool additiv = false;
    bool versucht = false;
};

enum class Ziehen { Nichts, Verschieben, Groesse, Lineal };

struct Zustand {
    std::vector<std::unique_ptr<Dokument>> docs;
    int aktiv = -1;
    Spieldaten daten;
    std::size_t datenStand = static_cast<std::size_t>(-1);
    Spiel spiel = Spiel::JA;
    Texte texte;
    bool texteGeladen = false;
    ShaderMap shader;
    bool shaderGeladen = false;
    std::map<std::string, Textur> texturen;
    unsigned texturStand = 0;
    std::map<std::string, Schrift> schriften;
    std::vector<std::string> schriftNamenSp, schriftNamenMp;
    bool schriftNamenGeladen = false;
    // Ansicht
    Seitenverhaeltnis seitenv = Seitenverhaeltnis::Strecken;
    int aufloesung = 0;   // 0 = Fenster, sonst Voreinstellung
    bool interaktion = false;
    bool alleItems = true;
    bool raster = false;
    bool anRaster = false;
    bool anKanten = true;
    float rasterWeite = 8.0F;
    // Ziehen
    Ziehen ziehen = Ziehen::Nichts;
    int griff = -1;
    ImVec2 ziehStart{0, 0};
    float zdx = 0, zdy = 0;
    Rechteck groesseNeu;
    std::vector<Hilfslinie> linien;
    std::vector<float> hilfslinienX, hilfslinienY;   // vom Lineal gezogene
    // Laufzeit (Interaktionsmodus)
    Laufzeit lauf;
    int laufDatei = -1;
    int laufVersion = -1;
    Datei laufKopie;
    std::map<std::string, std::string> vorschauCvars;   // Editor-Modus
    // Code-Editor
    bool codeFokus = false;
    bool suchenOffen = false;
    bool suchFokus = false;
    bool ersetzenOffen = false;
    std::string such, ersatz;
    SuchOptionen suchOpt;
    bool inAllen = false;
    bool vervollOffen = false;
    std::vector<std::string> vervoll;
    int vervollWahl = 0;
    float codeHoehe = 0;
    float linksBreite = 0, rechtsBreite = 0;
    bool ziehtText = false;
    std::string meldung;
    std::string ordnerAusgabe;
    // Selbsttest
    int letztePrims = 0;
    bool spielListeOffen = false;
    std::string spielFilter;
};

// InputText mit std::string (wie misc/cpp/imgui_stdlib.cpp)
int waechst(ImGuiInputTextCallbackData* d) {
    if (d->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* t = static_cast<std::string*>(d->UserData);
        t->resize(static_cast<std::size_t>(d->BufTextLen));
        d->Buf = t->data();
    }
    return 0;
}
bool eingabe(const char* id, std::string* s, ImGuiInputTextFlags f = 0) {
    return ImGui::InputText(id, s->data(), s->capacity() + 1, f | ImGuiInputTextFlags_CallbackResize, waechst, s);
}

Zustand* z_ = nullptr;
Zustand& Z() {
    if (z_ == nullptr) { z_ = new Zustand(); }
    return *z_;
}

Dokument* doc() {
    Zustand& z = Z();
    return (z.aktiv >= 0 && static_cast<std::size_t>(z.aktiv) < z.docs.size()) ? z.docs[static_cast<std::size_t>(z.aktiv)].get()
                                                                              : nullptr;
}

// --- Spieldaten, Texte, Schriften, Shader ------------------------------------

void datenPruefen() {
    Zustand& z = Z();
    const int rw = regelwerkSpielErmitteln();
    z.spiel = rw == kRegelwerkMovieDuels ? Spiel::MD : rw == kRegelwerkJediOutcast ? Spiel::JK2 : Spiel::JA;
    std::size_t n = 0;
    for (const GamePath& g : g_app->gamePaths) { n += g.archives.size() + 1; }
    if (n == z.datenStand) { return; }
    z.datenStand = n;
    z.daten.ordner = g_app->gamePaths;   // nur lesen
    z.texteGeladen = z.shaderGeladen = z.schriftNamenGeladen = false;
    z.schriften.clear();
    for (auto& [k, t] : z.texturen) { (void)k; t = Textur{}; }
    z.texturen.clear();
}

const Texte& texte() {
    Zustand& z = Z();
    if (!z.texteGeladen) {
        z.texte = ladeTexte(z.daten, z.spiel);
        z.texteGeladen = true;
    }
    return z.texte;
}

std::string kleinPfad(std::string s) {
    for (char& c : s) { c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

const Schrift* schriftNamens(const std::string& name) {
    if (name.empty()) { return nullptr; }
    Zustand& z = Z();
    const auto it = z.schriften.find(name);
    if (it != z.schriften.end()) { return it->second.ok ? &it->second : nullptr; }
    Schrift s;
    std::string daten;
    if (z.daten.lies("fonts/" + name + ".fontdat", daten)) { (void)ladeFontdat(name, daten, s); }
    s.name = name;
    s.shader = "fonts/" + name;
    z.schriften[name] = s;
    return z.schriften[name].ok ? &z.schriften[name] : nullptr;
}

// Die Schriftnamen aus assetGlobalDef (SP ui/main.menu, MP ui/jamp/main.menu)
void schriftNamenLaden() {
    Zustand& z = Z();
    if (z.schriftNamenGeladen) { return; }
    z.schriftNamenGeladen = true;
    auto aus = [&](const std::vector<std::string>& pfade, Dialekt d) {
        for (const std::string& p : pfade) {
            std::string t;
            if (!z.daten.lies(p, t)) { continue; }
            LeseOptionen o = optionenFuer(p, z.spiel, z.daten.lader());
            o.dialekt = d;
            const Datei a = lies(p, t, o);
            if (a.assets.vorhanden) { return schriftNamen(a, d); }
        }
        return std::vector<std::string>{};
    };
    z.schriftNamenSp = aus({"ui/main.menu", "ui/menus.txt", "ui/ingame.menu"}, Dialekt::SP);
    z.schriftNamenMp = aus({"ui/jamp/main.menu", "ui/jk2mp/main.menu", "ui/MD_MP/main.menu", "ui/jamp/ingame.menu"}, Dialekt::MP);
}

const Textur* textur(const std::string& shaderName) {
    Zustand& z = Z();
    if (shaderName.empty()) { return nullptr; }
    if (z.texturStand != render::texturStand()) {
        z.texturen.clear();
        z.texturStand = render::texturStand();
    }
    std::string key = kleinPfad(shaderName);
    const std::size_t punkt = key.rfind('.');
    if (punkt != std::string::npos && punkt + 5 >= key.size() && key.find('/', punkt) == std::string::npos) { key = key.substr(0, punkt); }
    Textur& t = z.texturen[key];
    if (t.versucht) { return t.tex != nullptr ? &t : nullptr; }
    t.versucht = true;
    if (!z.shaderGeladen) {
        z.shaderGeladen = true;
        for (const std::string& f : z.daten.liste("shaders/", {".shader"})) {
            std::string inhalt;
            if (z.daten.lies(f, inhalt)) { parseShaderScript(inhalt, z.shader); }
        }
    }
    std::string bild = key;
    const auto s = z.shader.find(key);
    if (s != z.shader.end() && !s->second.image.empty()) {
        bild = kleinPfad(s->second.image);
        t.additiv = s->second.blend == BlendMode::Add || s->second.blend == BlendMode::AddAlpha;
    }
    std::string bytes;
    bool da = false;
    Dokument* d = doc();
    for (const std::string& k : textureCandidates(bild)) {
        if (d != nullptr) {
            for (const auto& [pfad, inhalt] : d->bilder) {
                if (kleinPfad(pfad) == kleinPfad(k)) {
                    bytes = inhalt;
                    da = true;
                }
            }
        }
        if (!da && z.daten.lies(k, bytes)) { da = true; }
        if (da) { break; }
    }
    if (!da) { return nullptr; }
    const image::Image im = image::decode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
    if (!im.ok || im.width <= 0 || im.height <= 0) { return nullptr; }
    t.tex = render::createTexture(im.rgba.data(), im.width, im.height);
    t.w = im.width;
    t.h = im.height;
    return t.tex != nullptr ? &t : nullptr;
}

// --- Dokumente ---------------------------------------------------------------

void neuParsen(Dokument& d) {
    if (d.geparst == d.puffer.version()) { return; }
    Zustand& z = Z();
    LeseOptionen o = optionenFuer(d.name, z.spiel, z.daten.lader());
    d.datei = lies(d.name, d.puffer.text(), o);
    d.geparst = d.puffer.version();
    if (d.datei.menus.empty()) { d.seite = 0; }
    const std::vector<Seite> s = seitenImMenue(d.datei);
    d.seite = std::clamp(d.seite, 0, std::max(0, static_cast<int>(s.size()) - 1));
}

int menuDerSeite(const Dokument& d) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    if (s.empty()) { return -1; }
    return s[static_cast<std::size_t>(std::clamp(d.seite, 0, static_cast<int>(s.size()) - 1))].menu;
}

bool oeffne(const std::string& name, const std::string& platte, const std::string& text) {
    Zustand& z = Z();
    for (std::size_t i = 0; i < z.docs.size(); ++i) {
        if (z.docs[i]->name == name && z.docs[i]->platte == platte) {
            z.aktiv = static_cast<int>(i);
            return true;
        }
    }
    auto d = std::make_unique<Dokument>();
    d->name = name;
    d->platte = platte;
    d->puffer.setzeText(text);
    d->geladen = d->gespeichert = text;
    z.docs.push_back(std::move(d));
    z.aktiv = static_cast<int>(z.docs.size()) - 1;
    neuParsen(*doc());
    return true;
}

// Ein Spielpfad zu einer Datei auf der Platte: ab ".../ui/" (sonst ui/<name>)
std::string spielPfadVon(const std::string& platte) {
    const std::string k = kleinPfad(platte);
    const std::size_t p = k.rfind("/ui/");
    if (p != std::string::npos) { return platte.substr(p + 1); }
    return "ui/" + fs::path(platte).filename().string();
}

bool imSpielordner(const std::string& pfad) {
    std::error_code ec;
    const std::string k = kleinPfad(fs::weakly_canonical(fs::path(pfad), ec).string());
    auto pruefe = [&](const std::string& o) {
        if (o.empty()) { return false; }
        std::string g = kleinPfad(fs::weakly_canonical(fs::path(o), ec).string());
        // GameData-Ordner (eins ueber base) ebenfalls schuetzen
        const std::string eltern = kleinPfad(fs::path(g).parent_path().string());
        return k.compare(0, g.size(), g) == 0 || (!eltern.empty() && k.compare(0, eltern.size(), eltern) == 0);
    };
    for (const std::string& o : g_app->settings.gamePaths) {
        if (pruefe(o)) { return true; }
    }
    for (const std::string& o : g_app->settings.gamePathsJk2) {
        if (pruefe(o)) { return true; }
    }
    return false;
}

bool schreibeDatei(const std::string& pfad, const std::string& inhalt) {
    if (imSpielordner(pfad)) {
        Z().meldung = tr(Str::MeNieSpielordner);
        return false;
    }
    std::ofstream f(fs::u8path(pfad), std::ios::binary);
    if (!f) { return false; }
    f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
    return static_cast<bool>(f);
}

void speichern(Dokument& d, const std::string& ziel) {
    char buf[600];
    if (schreibeDatei(ziel, d.puffer.text())) {
        d.platte = ziel;
        d.gespeichert = d.puffer.text();
        std::snprintf(buf, sizeof(buf), tr(Str::MeGespeichert), ziel.c_str());
    } else {
        std::snprintf(buf, sizeof(buf), tr(Str::MeNichtGespeichert), ziel.c_str());
    }
    Z().meldung = buf;
    addStatus(buf);
}

// --- Aenderungen an Text und Flaeche ------------------------------------------

void aendere(std::vector<Ersetzung> e) {
    Dokument* d = doc();
    if (d == nullptr || e.empty()) { return; }
    d->puffer.ersetzeViele(std::move(e));
    neuParsen(*d);
}

Rechteck clientRect(const Menu& m, const Item& it) { (void)m; return it.window.rectClient; }

void setzeRects(const std::vector<std::pair<int, Rechteck>>& neu) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    std::vector<Ersetzung> e;
    for (const auto& [i, r] : neu) {
        std::vector<Ersetzung> x;
        if (setzeRect(d->datei, mi, i, r, x)) { e.insert(e.end(), x.begin(), x.end()); }
        else { Z().meldung = tr(Str::MeAusMakro); }
    }
    aendere(std::move(e));
}

std::string konstante(const Datei& d, const char* name, int wert) {
    if (d.dialekt == Dialekt::SP) { return name; }
    return d.defines.count(name) != 0 ? std::string(name) : std::to_string(wert);
}

void eigenschaft(int item, const char* schluessel, const std::vector<std::string>& werte) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    std::vector<Ersetzung> e;
    if (setzeEigenschaft(d->datei, menuDerSeite(*d), item, schluessel, werte, e)) { aendere(std::move(e)); }
    else { Z().meldung = tr(Str::MeAusMakro); }
}

std::string farbText(const Farbe& c) {
    return zahlText(c.r) + " " + zahlText(c.g) + " " + zahlText(c.b) + " " + zahlText(c.a);
}

std::string inAnf(const std::string& s) { return "\"" + s + "\""; }

// Zeile eines Items im Text (fuer den Sprung Flaeche -> Code)
void springe(Dokument& d, std::uint32_t off) {
    d.puffer.einCursor(off);
    d.springeZeile = d.puffer.pos(off).zeile;
}

// Welches Item steht an der Cursorposition? (Code -> Flaeche)
void auswahlAusCursor(Dokument& d) {
    const int mi = menuDerSeite(d);
    if (mi < 0) { return; }
    const std::uint32_t c = d.puffer.cursor.front().pos;
    for (std::size_t m = 0; m < d.datei.menus.size(); ++m) {
        const Menu& me = d.datei.menus[m];
        if (c < me.anfang || c > me.ende) { continue; }
        for (std::size_t i = 0; i < me.items.size(); ++i) {
            if (c >= me.items[i].anfang && c <= me.items[i].ende) {
                if (static_cast<int>(m) != mi) {
                    const std::vector<Seite> s = seitenImMenue(d.datei);
                    for (std::size_t k = 0; k < s.size(); ++k) {
                        if (s[k].menu == static_cast<int>(m) && s[k].cvar.empty()) { d.seite = static_cast<int>(k); }
                    }
                }
                d.auswahl.assign(1, static_cast<int>(i));
                return;
            }
        }
    }
}

// --- Vorschau ------------------------------------------------------------------

Umgebung umgebung(const Dokument& d) {
    Zustand& z = Z();
    schriftNamenLaden();
    Umgebung u;
    u.texte = &texte();
    u.dialekt = d.datei.dialekt;
    const std::vector<std::string>& namen = d.datei.dialekt == Dialekt::MP ? z.schriftNamenMp : z.schriftNamenSp;
    u.schrift = [namen](int f) -> const Schrift* {
        const std::size_t i = (f >= 0 && static_cast<std::size_t>(f) < namen.size()) ? static_cast<std::size_t>(f) : 0;
        if (i < namen.size() && !namen[i].empty()) {
            if (const Schrift* s = schriftNamens(namen[i])) { return s; }
        }
        return schriftNamens("ergoec");
    };
    u.cvar = [](const std::string& n) {
        const auto& m = Z().vorschauCvars;
        const auto it = m.find(kleinPfad(n));
        return it == m.end() ? std::string() : it->second;
    };
    u.feederZeilen = [](float) { return std::vector<std::string>{"Entry 1", "Entry 2", "Entry 3", "Entry 4"}; };
    u.zeitMs = static_cast<int>(ImGui::GetTime() * 1000.0);
    u.alleItems = z.alleItems && !z.interaktion;
    return u;
}

void zeichnePrims(ImDrawList* dl, const std::vector<Prim>& prims, const Abbildung& ab, ImVec2 o, const std::set<int>& verschoben,
                  float dx, float dy) {
    for (const Prim& p : prims) {
        float x = p.x, y = p.y;
        if (p.item >= 0 && verschoben.count(p.item) != 0) {
            x += dx;
            y += dy;
        }
        const ImVec2 a{o.x + ab.x(x), o.y + ab.y(y)};
        const ImVec2 b{o.x + ab.x(x + p.w), o.y + ab.y(y + p.h)};
        const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4{std::clamp(p.farbe.r, 0.0F, 1.0F), std::clamp(p.farbe.g, 0.0F, 1.0F),
                                                                std::clamp(p.farbe.b, 0.0F, 1.0F), std::clamp(p.farbe.a, 0.0F, 1.0F)});
        switch (p.art) {
            case PrimArt::Fuellen:
                dl->AddRectFilled(a, b, col);
                break;
            case PrimArt::Bild: {
                const Textur* t = textur(p.shader);
                if (t == nullptr) {
                    dl->AddRect(a, b, IM_COL32(255, 0, 255, 120));
                    break;
                }
                if (t->additiv) {
                    render::zeichneAdditiv(dl, t->tex, a, b, col);
                } else {
                    dl->AddImage(reinterpret_cast<ImTextureID>(t->tex), a, b, ImVec2{p.s0, p.t0}, ImVec2{p.s1, p.t1}, col);
                }
                break;
            }
            case PrimArt::Platzhalter:
            case PrimArt::Modell:
            case PrimArt::Kino: {
                const ImU32 f = p.art == PrimArt::Modell ? IM_COL32(90, 160, 255, 200)
                                : p.art == PrimArt::Kino ? IM_COL32(255, 170, 60, 200)
                                                         : IM_COL32(180, 180, 180, 160);
                dl->AddRectFilled(a, b, (f & 0x00FFFFFFU) | 0x30000000U);
                dl->AddRect(a, b, f);
                const std::string t = (p.art == PrimArt::Modell ? "model: " : p.art == PrimArt::Kino ? "cinematic: " : "") + p.shader;
                dl->PushClipRect(a, b, true);
                dl->AddText(ImVec2{a.x + 3, a.y + 2}, f, t.c_str());
                dl->PopClipRect();
                break;
            }
        }
    }
}

// --- Arbeitsflaeche --------------------------------------------------------------

void zeichneFlaeche(Dokument& d, ImVec2 groesse) {
    Zustand& z = Z();
    const int mi = menuDerSeite(d);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o0 = ImGui::GetCursorScreenPos();
    const float lineal = std::floor(ImGui::GetFontSize() * 1.2F);
    // Aufloesung: Fenster oder Voreinstellung (das Seitenverhaeltnis der Vorschau)
    static const float kAufl[][2] = {{0, 0}, {640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3440, 1440}};
    float bw = groesse.x - lineal, bh = groesse.y - lineal;
    if (z.aufloesung > 0) {
        const float ax = kAufl[z.aufloesung][0], ay = kAufl[z.aufloesung][1];
        const float f = std::min(bw / ax, bh / ay);
        bw = ax * f;
        bh = ay * f;
    }
    const ImVec2 o{o0.x + lineal, o0.y + lineal};
    const Abbildung ab = abbildung(bw, bh, z.seitenv);
    dl->AddRectFilled(o, ImVec2{o.x + bw, o.y + bh}, IM_COL32(0, 0, 0, 255));
    ImGui::InvisibleButton("##flaeche", ImVec2{std::max(1.0F, groesse.x), std::max(1.0F, groesse.y)},
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool hover = ImGui::IsItemHovered();
    const ImVec2 maus = ImGui::GetIO().MousePos;
    const float vx = (maus.x - o.x - ab.ox) / ab.sx;
    const float vy = (maus.y - o.y - ab.oy) / ab.sy;
    if (mi < 0) {
        z.letztePrims = 0;
        return;
    }
    // Lineale
    {
        const ImU32 grund = IM_COL32(40, 40, 46, 255), strich = IM_COL32(150, 150, 160, 255);
        dl->AddRectFilled(ImVec2{o0.x, o0.y}, ImVec2{o0.x + groesse.x, o.y}, grund);
        dl->AddRectFilled(ImVec2{o0.x, o0.y}, ImVec2{o.x, o0.y + groesse.y}, grund);
        for (int v = 0; v <= 640; v += 10) {
            const float x = o.x + ab.x(static_cast<float>(v));
            const float l = (v % 50 == 0) ? lineal * 0.6F : lineal * 0.25F;
            dl->AddLine(ImVec2{x, o.y - l}, ImVec2{x, o.y}, strich);
            if (v % 100 == 0) { dl->AddText(ImVec2{x + 2, o0.y}, strich, std::to_string(v).c_str()); }
        }
        for (int v = 0; v <= 480; v += 10) {
            const float y = o.y + ab.y(static_cast<float>(v));
            const float l = (v % 50 == 0) ? lineal * 0.6F : lineal * 0.25F;
            dl->AddLine(ImVec2{o.x - l, y}, ImVec2{o.x, y}, strich);
            if (v % 100 == 0) { dl->AddText(ImVec2{o0.x + 1, y + 1}, strich, std::to_string(v).c_str()); }
        }
        if (hover) {
            dl->AddLine(ImVec2{maus.x, o0.y}, ImVec2{maus.x, o.y}, IM_COL32(255, 200, 0, 255));
            dl->AddLine(ImVec2{o0.x, maus.y}, ImVec2{o.x, maus.y}, IM_COL32(255, 200, 0, 255));
        }
    }
    dl->PushClipRect(o, ImVec2{o.x + bw, o.y + bh}, true);
    // Vorschau
    Umgebung u = umgebung(d);
    const Datei* quelle = &d.datei;
    if (z.interaktion) {
        if (z.laufDatei != z.aktiv || z.lauf.d != &d.datei || z.laufVersion < 0) {
            z.lauf.start(d.datei);
            z.lauf.basis = u;
            z.lauf.oeffne(d.datei.menus[static_cast<std::size_t>(mi)].window.name);
            z.laufDatei = z.aktiv;
            z.laufVersion = -1;
        }
        if (z.laufVersion != z.lauf.version) {
            z.laufKopie = d.datei;
            z.lauf.anwenden(z.laufKopie);
            z.laufVersion = z.lauf.version;
        }
        z.lauf.fuelle(mi, u);
        u.schrift = umgebung(d).schrift;
        quelle = &z.laufKopie;
    } else {
        // ui_tab-Seite: deren Cvar in der Vorschau setzen
        const std::vector<Seite> s = seitenImMenue(d.datei);
        const Seite& sei = s[static_cast<std::size_t>(std::clamp(d.seite, 0, static_cast<int>(s.size()) - 1))];
        if (!sei.cvar.empty()) { z.vorschauCvars[kleinPfad(sei.cvar)] = sei.wert; }
    }
    std::vector<Prim> prims;
    zeichneMenu(*quelle, mi, u, prims);
    z.letztePrims = static_cast<int>(prims.size());
    const Menu& m = quelle->menus[static_cast<std::size_t>(mi)];
    std::set<int> sel(d.auswahl.begin(), d.auswahl.end());
    const bool verschiebt = z.ziehen == Ziehen::Verschieben;
    zeichnePrims(dl, prims, ab, o, verschiebt ? sel : std::set<int>{}, z.zdx, z.zdy);
    // Raster
    if (z.raster && !z.interaktion && z.rasterWeite >= 2) {
        for (float x = 0; x <= 640; x += z.rasterWeite) {
            dl->AddLine(ImVec2{o.x + ab.x(x), o.y + ab.y(0)}, ImVec2{o.x + ab.x(x), o.y + ab.y(480)}, IM_COL32(255, 255, 255, 18));
        }
        for (float y = 0; y <= 480; y += z.rasterWeite) {
            dl->AddLine(ImVec2{o.x + ab.x(0), o.y + ab.y(y)}, ImVec2{o.x + ab.x(640), o.y + ab.y(y)}, IM_COL32(255, 255, 255, 18));
        }
    }
    for (const float x : z.hilfslinienX) {
        dl->AddLine(ImVec2{o.x + ab.x(x), o.y}, ImVec2{o.x + ab.x(x), o.y + bh}, IM_COL32(0, 200, 255, 160));
    }
    for (const float y : z.hilfslinienY) {
        dl->AddLine(ImVec2{o.x, o.y + ab.y(y)}, ImVec2{o.x + bw, o.y + ab.y(y)}, IM_COL32(0, 200, 255, 160));
    }

    if (z.interaktion) {
        dl->PopClipRect();
        if (hover) {
            z.lauf.maus(mi, vx, vy);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { z.lauf.klick(mi, vx, vy); }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { z.lauf.escape(mi); }
        }
        // Ein Menue, das ein Skript geoeffnet hat: dessen Seite zeigen
        for (std::size_t k = 0; k < z.lauf.menuFlags.size(); ++k) {
            if ((z.lauf.menuFlags[k] & WINDOW_HASFOCUS) != 0 && static_cast<int>(k) != mi) {
                const std::vector<Seite> s = seitenImMenue(d.datei);
                for (std::size_t q = 0; q < s.size(); ++q) {
                    if (s[q].menu == static_cast<int>(k) && s[q].cvar.empty()) { d.seite = static_cast<int>(q); }
                }
            }
        }
        return;
    }

    // --- Bearbeiten: Auswahl, Griffe, Ziehen ---
    auto bild = [&](const Rechteck& r) {
        return std::pair<ImVec2, ImVec2>{ImVec2{o.x + ab.x(r.x), o.y + ab.y(r.y)}, ImVec2{o.x + ab.x(r.x + r.w), o.y + ab.y(r.y + r.h)}};
    };
    int unterMaus = -1;
    for (std::size_t i = m.items.size(); i-- > 0;) {
        const Rechteck& r = m.items[i].window.rect;
        if (vx >= r.x && vx < r.x + r.w && vy >= r.y && vy < r.y + r.h) {
            const int fl = m.items[i].window.flags;
            if (!z.alleItems && !itemSichtbar(m.items[i], fl, u)) { continue; }
            unterMaus = static_cast<int>(i);
            break;
        }
    }
    if (hover && unterMaus >= 0 && z.ziehen == Ziehen::Nichts) {
        const auto [a, b] = bild(m.items[static_cast<std::size_t>(unterMaus)].window.rect);
        dl->AddRect(a, b, IM_COL32(255, 255, 255, 90));
        ImGui::SetTooltip("%s  (%s)", m.items[static_cast<std::size_t>(unterMaus)].window.name.c_str(),
                          typName(m.items[static_cast<std::size_t>(unterMaus)].type));
    }
    // Griffe der Einzelauswahl
    const float hs = 4.0F;
    int griffUnter = -1;
    Rechteck einzel;
    if (d.auswahl.size() == 1 && static_cast<std::size_t>(d.auswahl[0]) < m.items.size()) {
        einzel = m.items[static_cast<std::size_t>(d.auswahl[0])].window.rect;
        const float gx[3] = {einzel.x, einzel.x + einzel.w * 0.5F, einzel.x + einzel.w};
        const float gy[3] = {einzel.y, einzel.y + einzel.h * 0.5F, einzel.y + einzel.h};
        int n = 0;
        for (int yy = 0; yy < 3; ++yy) {
            for (int xx = 0; xx < 3; ++xx) {
                if (xx == 1 && yy == 1) { continue; }
                const ImVec2 c{o.x + ab.x(gx[xx]), o.y + ab.y(gy[yy])};
                if (std::fabs(maus.x - c.x) <= hs + 1 && std::fabs(maus.y - c.y) <= hs + 1) { griffUnter = yy * 3 + xx; }
                ++n;
            }
        }
    }
    for (const int i : d.auswahl) {
        if (static_cast<std::size_t>(i) >= m.items.size()) { continue; }
        Rechteck r = m.items[static_cast<std::size_t>(i)].window.rect;
        if (verschiebt) {
            r.x += z.zdx;
            r.y += z.zdy;
        }
        if (z.ziehen == Ziehen::Groesse && d.auswahl.size() == 1) { r = z.groesseNeu; }
        const auto [a, b] = bild(r);
        const bool gesperrt = d.gesperrt.count(i) != 0;
        dl->AddRect(a, b, gesperrt ? IM_COL32(255, 80, 80, 255) : IM_COL32(60, 170, 255, 255), 0, 0, 1.5F);
        if (d.auswahl.size() == 1 && !gesperrt) {
            const float gx[3] = {a.x, (a.x + b.x) * 0.5F, b.x};
            const float gy[3] = {a.y, (a.y + b.y) * 0.5F, b.y};
            for (int yy = 0; yy < 3; ++yy) {
                for (int xx = 0; xx < 3; ++xx) {
                    if (xx == 1 && yy == 1) { continue; }
                    dl->AddRectFilled(ImVec2{gx[xx] - hs, gy[yy] - hs}, ImVec2{gx[xx] + hs, gy[yy] + hs}, IM_COL32(255, 255, 255, 255));
                    dl->AddRect(ImVec2{gx[xx] - hs, gy[yy] - hs}, ImVec2{gx[xx] + hs, gy[yy] + hs}, IM_COL32(30, 90, 200, 255));
                }
            }
            // Abstandsanzeige zum Bildschirmrand
            char t[96];
            std::snprintf(t, sizeof(t), "%g, %g  %g x %g", static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.w),
                          static_cast<double>(r.h));
            dl->AddText(ImVec2{a.x, b.y + 2}, IM_COL32(60, 170, 255, 255), t);
        }
    }
    for (const Hilfslinie& h : z.linien) {
        if (h.senkrecht) {
            dl->AddLine(ImVec2{o.x + ab.x(h.wert), o.y + ab.y(h.von)}, ImVec2{o.x + ab.x(h.wert), o.y + ab.y(h.bis)}, IM_COL32(255, 60, 200, 255));
        } else {
            dl->AddLine(ImVec2{o.x + ab.x(h.von), o.y + ab.y(h.wert)}, ImVec2{o.x + ab.x(h.bis), o.y + ab.y(h.wert)}, IM_COL32(255, 60, 200, 255));
        }
    }
    dl->PopClipRect();

    // Maus: Klick waehlt, Ziehen verschiebt/aendert die Groesse
    if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImGui::SetKeyboardFocusHere(-1);
        z.codeFokus = false;
        z.ziehStart = ImVec2{vx, vy};
        z.zdx = z.zdy = 0;
        if (maus.y < o.y && maus.x > o.x) {
            z.ziehen = Ziehen::Lineal;   // Hilfslinie aus dem oberen Lineal
            z.griff = 1;
        } else if (maus.x < o.x && maus.y > o.y) {
            z.ziehen = Ziehen::Lineal;
            z.griff = 0;
        } else if (griffUnter >= 0 && d.auswahl.size() == 1 && d.gesperrt.count(d.auswahl[0]) == 0) {
            z.ziehen = Ziehen::Groesse;
            z.griff = griffUnter;
            z.groesseNeu = einzel;
        } else if (unterMaus >= 0) {
            const bool shift = ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl;
            const auto it = std::find(d.auswahl.begin(), d.auswahl.end(), unterMaus);
            if (shift) {
                if (it != d.auswahl.end()) { d.auswahl.erase(it); }
                else { d.auswahl.push_back(unterMaus); }
            } else if (it == d.auswahl.end()) {
                d.auswahl.assign(1, unterMaus);
            }
            if (!d.auswahl.empty()) {
                springe(d, m.items[static_cast<std::size_t>(unterMaus)].anfang);
                bool frei = true;
                for (const int i : d.auswahl) { frei = frei && d.gesperrt.count(i) == 0; }
                if (frei) { z.ziehen = Ziehen::Verschieben; }
            }
        } else {
            d.auswahl.clear();
        }
    }
    if (z.ziehen != Ziehen::Nichts && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        float dx = vx - z.ziehStart.x;
        float dy = vy - z.ziehStart.y;
        if (ImGui::GetIO().KeyShift) {   // nur waagrecht oder senkrecht
            if (std::fabs(dx) > std::fabs(dy)) { dy = 0; } else { dx = 0; }
        }
        z.linien.clear();
        std::vector<Rechteck> andere;
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            if (std::find(d.auswahl.begin(), d.auswahl.end(), static_cast<int>(i)) == d.auswahl.end()) {
                andere.push_back(m.items[i].window.rect);
            }
        }
        for (const float x : z.hilfslinienX) { andere.push_back(Rechteck{x, 0, 0, 480}); }
        for (const float y : z.hilfslinienY) { andere.push_back(Rechteck{0, y, 640, 0}); }
        const float tol = 5.0F / std::max(0.01F, ab.sx);
        const bool snap = !ImGui::GetIO().KeyAlt;
        if (z.ziehen == Ziehen::Verschieben && !d.auswahl.empty()) {
            Rechteck r = m.items[static_cast<std::size_t>(d.auswahl[0])].window.rect;
            r.x += dx;
            r.y += dy;
            if (snap) {
                const Einrasten e = einrasten(r, andere, z.rasterWeite, tol, z.anRaster, z.anKanten);
                dx += e.dx;
                dy += e.dy;
                z.linien = e.linien;
            }
            z.zdx = std::round(dx);
            z.zdy = std::round(dy);
        } else if (z.ziehen == Ziehen::Groesse) {
            Rechteck r = einzel;
            const int gx = z.griff % 3, gy = z.griff / 3;
            if (gx == 0) { r.x += dx; r.w -= dx; }
            if (gx == 2) { r.w += dx; }
            if (gy == 0) { r.y += dy; r.h -= dy; }
            if (gy == 2) { r.h += dy; }
            r.x = std::round(r.x); r.y = std::round(r.y); r.w = std::round(std::max(1.0F, r.w)); r.h = std::round(std::max(1.0F, r.h));
            z.groesseNeu = r;
        }
    }
    if (z.ziehen != Ziehen::Nichts && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (z.ziehen == Ziehen::Verschieben && (z.zdx != 0 || z.zdy != 0)) {
            std::vector<std::pair<int, Rechteck>> neu;
            for (const int i : d.auswahl) {
                Rechteck r = clientRect(m, m.items[static_cast<std::size_t>(i)]);
                r.x += z.zdx;
                r.y += z.zdy;
                neu.emplace_back(i, r);
            }
            setzeRects(neu);
        } else if (z.ziehen == Ziehen::Groesse) {
            const Rechteck alt = einzel;
            Rechteck r = clientRect(m, m.items[static_cast<std::size_t>(d.auswahl[0])]);
            r.x += z.groesseNeu.x - alt.x;
            r.y += z.groesseNeu.y - alt.y;
            r.w = z.groesseNeu.w;
            r.h = z.groesseNeu.h;
            setzeRects({{d.auswahl[0], r}});
        } else if (z.ziehen == Ziehen::Lineal && hover) {
            if (z.griff == 1) { z.hilfslinienX.push_back(std::round(vx)); }
            else { z.hilfslinienY.push_back(std::round(vy)); }
        }
        z.ziehen = Ziehen::Nichts;
        z.zdx = z.zdy = 0;
        z.linien.clear();
    }
    // Tasten der Flaeche
    if (ImGui::IsWindowFocused() && !z.codeFokus && !ImGui::GetIO().WantTextInput && !d.auswahl.empty()) {
        const float schritt = ImGui::GetIO().KeyShift ? 10.0F : 1.0F;
        float dx = 0, dy = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { dx = -schritt; }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { dx = schritt; }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { dy = -schritt; }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { dy = schritt; }
        if (dx != 0 || dy != 0) { verschiebeAuswahl(dx, dy); }
    }
}

// --- Code-Editor ------------------------------------------------------------------

ImU32 farbeFuer(Farbklasse k) {
    switch (k) {
        case Farbklasse::Kommentar: return IM_COL32(106, 153, 85, 255);
        case Farbklasse::String: return IM_COL32(206, 145, 120, 255);
        case Farbklasse::Zahl: return IM_COL32(181, 206, 168, 255);
        case Farbklasse::MenuWort: return IM_COL32(86, 156, 214, 255);
        case Farbklasse::ItemWort: return IM_COL32(156, 220, 254, 255);
        case Farbklasse::Block: return IM_COL32(197, 134, 192, 255);
        case Farbklasse::Konstante: return IM_COL32(79, 193, 255, 255);
        case Farbklasse::Praeprozessor: return IM_COL32(155, 155, 155, 255);
        case Farbklasse::Skript: return IM_COL32(220, 220, 170, 255);
        case Farbklasse::MenuText: return IM_COL32(255, 198, 109, 255);
        case Farbklasse::Klammer: return IM_COL32(255, 215, 0, 255);
        case Farbklasse::Text: break;
    }
    return IM_COL32(212, 212, 212, 255);
}

// Latin-1 (die .menu-Dateien) -> UTF-8 fuer ImGui
std::string anzeige(std::string_view s) {
    std::string o;
    o.reserve(s.size());
    for (const char c : s) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x80) { o += c; }
        else {
            o += static_cast<char>(0xC0 | (u >> 6));
            o += static_cast<char>(0x80 | (u & 0x3F));
        }
    }
    return o;
}

// x-Versatz einer Spalte (Tabs auf 4 Leerzeichen)
float spaltenX(std::string_view zeile, int spalte, float leer) {
    float x = 0;
    ImFont* f = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    for (int i = 0; i < spalte && static_cast<std::size_t>(i) < zeile.size(); ++i) {
        const char c = zeile[static_cast<std::size_t>(i)];
        if (c == '\t') {
            x = (std::floor(x / (leer * 4.0F)) + 1.0F) * leer * 4.0F;
        } else {
            const std::string a = anzeige(zeile.substr(static_cast<std::size_t>(i), 1));
            x += f->CalcTextSizeA(fs, 1e9F, 0.0F, a.c_str(), a.c_str() + a.size()).x;
        }
    }
    return x;
}

int spalteBei(std::string_view zeile, float x, float leer) {
    for (int s = 0; s <= static_cast<int>(zeile.size()); ++s) {
        const float a = spaltenX(zeile, s, leer);
        const float b = spaltenX(zeile, s + 1, leer);
        if (x < (a + b) * 0.5F || s == static_cast<int>(zeile.size())) { return s; }
    }
    return static_cast<int>(zeile.size());
}

std::string utf8ZuLatin1(const char* s) {
    std::string o;
    for (std::size_t i = 0; s[i] != 0;) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) { o += static_cast<char>(c); ++i; }
        else if ((c & 0xE0) == 0xC0 && s[i + 1] != 0) {
            const unsigned v = ((c & 0x1FU) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3FU);
            o += v < 256 ? static_cast<char>(v) : '?';
            i += 2;
        } else {
            o += '?';
            ++i;
            while (s[i] != 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) { ++i; }
        }
    }
    return o;
}

std::vector<std::string> kandidaten(const Dokument& d) {
    std::vector<std::string> k = menuSchluessel(d.datei.dialekt, d.datei.spiel);
    for (const std::string& s : itemSchluessel(d.datei.dialekt, d.datei.spiel)) { k.push_back(s); }
    for (const std::string& s : skriptBefehle()) { k.push_back(s); }
    for (int t = 0; t < 15; ++t) { k.emplace_back(typName(t)); }
    for (int t = 0; t < 6; ++t) { k.emplace_back(styleName(t)); }
    for (const char* a : {"ITEM_ALIGN_LEFT", "ITEM_ALIGN_CENTER", "ITEM_ALIGN_RIGHT", "WINDOW_INACTIVE", "menuDef", "itemDef"}) { k.emplace_back(a); }
    for (const auto& [n, v] : d.datei.defines) { (void)v; k.push_back(n); }
    for (const Menu& m : d.datei.menus) {
        k.push_back(m.window.name);
        for (const Item& it : m.items) {
            if (!it.window.name.empty()) { k.push_back(it.window.name); }
            if (!it.cvar.empty()) { k.push_back(it.cvar); }
            if (!it.window.background.empty()) { k.push_back(it.window.background); }
        }
    }
    for (const CvarFund& c : cvarsImMenue(d.datei)) { k.push_back(c.name); }
    const Zustand& z = Z();
    for (const auto& [n, s] : z.shader) { (void)s; if (n.compare(0, 4, "gfx/") == 0 || n.compare(0, 5, "menu/") == 0) { k.push_back(n); } }
    for (const auto& [n, s] : z.texte.werte) { (void)s; if (n.compare(0, 6, "MENUS_") == 0) { k.push_back("@" + n); } }
    return k;
}

// F12: Name unter dem Cursor -> menuDef/itemDef mit diesem Namen
void gehZuDefinition(Dokument& d) {
    const Treffer w = d.puffer.wortBei(d.puffer.cursor.front().pos);
    const std::string wort = d.puffer.text().substr(w.anfang, w.ende - w.anfang);
    if (wort.empty()) { return; }
    for (const Menu& m : d.datei.menus) {
        if (m.window.name == wort) {
            springe(d, m.anfang);
            return;
        }
        for (const Item& it : m.items) {
            if (it.window.name == wort || it.window.group == wort) {
                springe(d, it.anfang);
                return;
            }
        }
    }
    if (wort[0] == '@') { Z().meldung = wort + " = " + texte().aufloesen(wort); }
}

void zeichneSuchleiste(Dokument& d) {
    Zustand& z = Z();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
    if (z.suchFokus) {
        ImGui::SetKeyboardFocusHere();
        z.suchFokus = false;
    }
    bool neu = eingabe("##such", &z.such);
    ImGui::SameLine();
    neu |= ImGui::Checkbox(".*", &z.suchOpt.regex);
    ImGui::SameLine();
    neu |= ImGui::Checkbox("Aa", &z.suchOpt.gross);
    ImGui::SameLine();
    neu |= ImGui::Checkbox("ab", &z.suchOpt.wort);
    ImGui::SameLine();
    ImGui::Checkbox(tr(Str::MeInAllen), &z.inAllen);
    std::string fehler;
    const std::vector<Treffer> t = d.puffer.suche(z.such, z.suchOpt, &fehler);
    int gesamt = static_cast<int>(t.size());
    if (z.inAllen) {
        gesamt = 0;
        for (const auto& o : z.docs) { gesamt += static_cast<int>(o->puffer.suche(z.such, z.suchOpt).size()); }
    }
    ImGui::SameLine();
    char b[200];
    if (!fehler.empty()) { std::snprintf(b, sizeof(b), tr(Str::MeRegexFehler), fehler.c_str()); }
    else { std::snprintf(b, sizeof(b), tr(Str::MeTreffer), gesamt); }
    ImGui::TextUnformatted(b);
    auto naechster = [&](bool vor) {
        if (t.empty()) { return; }
        const std::uint32_t c = d.puffer.cursor.front().pos;
        const Treffer* w = nullptr;
        if (vor) {
            for (const Treffer& x : t) {
                if (x.anfang >= c && (x.anfang != d.puffer.cursor.front().von() || !d.puffer.cursor.front().auswahl())) { w = &x; break; }
            }
            if (w == nullptr) { w = &t.front(); }
        } else {
            for (auto it = t.rbegin(); it != t.rend(); ++it) {
                if (it->ende < d.puffer.cursor.front().von() + 1 && it->anfang < d.puffer.cursor.front().von()) { w = &*it; break; }
            }
            if (w == nullptr) { w = &t.back(); }
        }
        d.puffer.cursor.assign(1, Cursor{w->ende, w->anfang});
        d.springeZeile = d.puffer.pos(w->anfang).zeile;
    };
    ImGui::SameLine();
    if (ImGui::ArrowButton("##zurueck", ImGuiDir_Up)) { naechster(false); }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##vor", ImGuiDir_Down) || (neu && !t.empty())) { naechster(true); }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused()) { naechster(!ImGui::GetIO().KeyShift); }
    if (z.ersetzenOffen) {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
        eingabe("##ersatz", &z.ersatz);
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeErsetzen))) { d.puffer.ersetzeEinen(z.such, z.ersatz, z.suchOpt); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeErsetzenAlle))) {
            if (z.inAllen) {
                for (auto& o : z.docs) { o->puffer.ersetzeAlle(z.such, z.ersatz, z.suchOpt); }
            } else {
                d.puffer.ersetzeAlle(z.such, z.ersatz, z.suchOpt);
            }
        }
    }
}

void codeTasten(Dokument& d) {
    Zustand& z = Z();
    ImGuiIO& io = ImGui::GetIO();
    Textpuffer& p = d.puffer;
    const bool strg = io.KeyCtrl;
    const bool um = io.KeyShift;
    auto taste = [](ImGuiKey k) { return ImGui::IsKeyPressed(k, true); };
    if (z.vervollOffen && !z.vervoll.empty()) {
        if (taste(ImGuiKey_DownArrow)) { z.vervollWahl = (z.vervollWahl + 1) % static_cast<int>(z.vervoll.size()); return; }
        if (taste(ImGuiKey_UpArrow)) {
            z.vervollWahl = (z.vervollWahl + static_cast<int>(z.vervoll.size()) - 1) % static_cast<int>(z.vervoll.size());
            return;
        }
        if (taste(ImGuiKey_Enter) || taste(ImGuiKey_Tab)) {
            for (Cursor& c : p.cursor) {
                const Treffer w = p.wortBei(c.pos);
                c.anker = w.anfang;
                c.pos = std::min(c.pos, w.ende);
            }
            p.tippe(z.vervoll[static_cast<std::size_t>(z.vervollWahl)]);
            z.vervollOffen = false;
            return;
        }
        if (taste(ImGuiKey_Escape)) { z.vervollOffen = false; return; }
    }
    if (strg && taste(ImGuiKey_Z)) { p.rueckgaengig(); return; }
    if (strg && (taste(ImGuiKey_Y) || (um && taste(ImGuiKey_Z)))) { p.wiederholen(); return; }
    if (strg && taste(ImGuiKey_A)) { p.allesWaehlen(); return; }
    if (strg && (taste(ImGuiKey_C) || taste(ImGuiKey_X))) {
        const std::string s = p.auswahlText();
        if (!s.empty()) { ImGui::SetClipboardText(anzeige(s).c_str()); }
        if (ImGui::IsKeyPressed(ImGuiKey_X)) { p.tippe(""); }
        return;
    }
    if (strg && taste(ImGuiKey_V)) {
        if (const char* c = ImGui::GetClipboardText()) {
            std::string t = utf8ZuLatin1(c);
            if (p.zeilenUmbruch() == "\r\n") {
                std::string u;
                for (std::size_t i = 0; i < t.size(); ++i) {
                    if (t[i] == '\n' && (i == 0 || t[i - 1] != '\r')) { u += '\r'; }
                    u += t[i];
                }
                t = u;
            }
            p.tippe(t);
        }
        return;
    }
    if (strg && um && taste(ImGuiKey_L)) { p.alleVorkommen(); return; }
    if (strg && taste(ImGuiKey_D)) { p.naechstesVorkommenDazu(); return; }
    if (strg && taste(ImGuiKey_Slash)) { p.kommentar(); return; }
    if (strg && taste(ImGuiKey_F)) { z.suchenOffen = z.suchFokus = true; z.ersetzenOffen = false; z.such = p.auswahlText(); return; }
    if (strg && taste(ImGuiKey_H)) { z.suchenOffen = z.ersetzenOffen = z.suchFokus = true; return; }
    if (strg && taste(ImGuiKey_Space)) {
        const Treffer w = p.wortBei(p.cursor.front().pos);
        z.vervoll = vorschlaege(p.text().substr(w.anfang, p.cursor.front().pos - w.anfang), kandidaten(d));
        z.vervollWahl = 0;
        z.vervollOffen = !z.vervoll.empty();
        return;
    }
    if (taste(ImGuiKey_F12)) { gehZuDefinition(d); return; }
    if (taste(ImGuiKey_F2)) {
        const int zl = p.pos(p.cursor.front().pos).zeile;
        if (strg) {
            if (d.lesezeichen.count(zl) != 0) { d.lesezeichen.erase(zl); } else { d.lesezeichen.insert(zl); }
        } else if (!d.lesezeichen.empty()) {
            int ziel = um ? *d.lesezeichen.rbegin() : *d.lesezeichen.begin();
            if (um) {
                auto it = d.lesezeichen.lower_bound(zl);
                if (it != d.lesezeichen.begin()) { ziel = *std::prev(it); }
            } else {
                auto it = d.lesezeichen.upper_bound(zl);
                if (it != d.lesezeichen.end()) { ziel = *it; }
            }
            springe(d, p.zeilenAnfang(ziel));
        }
        return;
    }
    if (taste(ImGuiKey_Escape)) {
        if (p.cursor.size() > 1) { p.cursor.resize(1); }
        z.suchenOffen = false;
        return;
    }
    if (taste(ImGuiKey_LeftArrow)) { if (strg) { p.wortSprung(false, um); } else { p.bewege(0, -1, um); } }
    if (taste(ImGuiKey_RightArrow)) { if (strg) { p.wortSprung(true, um); } else { p.bewege(0, 1, um); } }
    if (taste(ImGuiKey_UpArrow)) { p.bewege(-1, 0, um); }
    if (taste(ImGuiKey_DownArrow)) { p.bewege(1, 0, um); }
    if (taste(ImGuiKey_PageUp)) { p.bewege(-30, 0, um); }
    if (taste(ImGuiKey_PageDown)) { p.bewege(30, 0, um); }
    if (taste(ImGuiKey_Home)) { if (strg) { p.einCursor(0, um); } else { p.zeilenAnfangEnde(false, um); } }
    if (taste(ImGuiKey_End)) {
        if (strg) { p.einCursor(static_cast<std::uint32_t>(p.text().size()), um); } else { p.zeilenAnfangEnde(true, um); }
    }
    if (taste(ImGuiKey_Backspace)) { p.loescheZurueck(); }
    if (taste(ImGuiKey_Delete)) { p.loescheVor(); }
    if (taste(ImGuiKey_Enter) || taste(ImGuiKey_KeypadEnter)) { p.neueZeile(); }
    if (taste(ImGuiKey_Tab)) {
        bool mehrzeilig = false;
        for (const Cursor& c : p.cursor) { mehrzeilig = mehrzeilig || p.pos(c.von()).zeile != p.pos(c.bis()).zeile; }
        if (um || mehrzeilig) { p.einruecken(um); } else { p.tippe("\t"); }
    }
    if (!strg) {
        std::string t;
        for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
            const ImWchar c = io.InputQueueCharacters[i];
            if (c == '\t' || c == '\r' || c == '\n') { continue; }
            if (c >= 32) { t += c < 256 ? static_cast<char>(c) : '?'; }
        }
        if (!t.empty()) {
            p.tippe(t);
            // Vervollstaendigung waehrend des Tippens
            const Treffer w = p.wortBei(p.cursor.front().pos);
            const std::string prae = p.text().substr(w.anfang, p.cursor.front().pos - w.anfang);
            if (prae.size() >= 2) {
                z.vervoll = vorschlaege(prae, kandidaten(d), 12);
                z.vervollWahl = 0;
                z.vervollOffen = !z.vervoll.empty();
            } else {
                z.vervollOffen = false;
            }
        }
    }
}

void zeichneCode(Dokument& d, ImVec2 groesse) {
    Zustand& z = Z();
    if (z.suchenOffen) { zeichneSuchleiste(d); }
    ImGui::BeginChild("##code", ImVec2{groesse.x, 0}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    Textpuffer& p = d.puffer;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    const float zh = std::floor(ImGui::GetTextLineHeightWithSpacing());
    const float leer = font->CalcTextSizeA(fs, 1e9F, 0.0F, " ").x;
    const ImVec2 o = ImGui::GetCursorScreenPos();
    const ImVec2 bereich = ImGui::GetContentRegionAvail();
    const int n = p.zeilen();
    char nr[16];
    std::snprintf(nr, sizeof(nr), "%d", n);
    const float nummernB = font->CalcTextSizeA(fs, 1e9F, 0.0F, nr).x + fs * 0.6F;
    const float rinne = rinnenBreite();
    const float falteB = fs;
    const float textX = o.x + nummernB + rinne + falteB + 4.0F;
    const float miniB = std::floor(fs * 5.0F);
    const float textB = bereich.x - (textX - o.x) - miniB;
    // sichtbare Zeilen (eingeklappte ueberspringen)
    const std::vector<Falte> falten = p.falten();
    std::vector<int> sichtbar;
    sichtbar.reserve(static_cast<std::size_t>(n));
    {
        int z0 = 0;
        while (z0 < n) {
            sichtbar.push_back(z0);
            int naechste = z0 + 1;
            if (d.gefaltet.count(z0) != 0) {
                for (const Falte& f : falten) {
                    if (f.von == z0) { naechste = f.bis; break; }
                }
            }
            z0 = std::max(naechste, z0 + 1);
        }
    }
    const int sichtbarN = static_cast<int>(sichtbar.size());
    const int proSeite = std::max(1, static_cast<int>(bereich.y / zh));
    // Scrollen
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0F && !ImGui::GetIO().KeyCtrl) {
        d.scrollZeile -= ImGui::GetIO().MouseWheel * 3.0F;
    }
    if (d.springeZeile >= 0) {
        const auto it = std::lower_bound(sichtbar.begin(), sichtbar.end(), d.springeZeile);
        const int idx = static_cast<int>(it - sichtbar.begin());
        if (static_cast<float>(idx) < d.scrollZeile || static_cast<float>(idx) >= d.scrollZeile + static_cast<float>(proSeite) - 1.0F) {
            d.scrollZeile = static_cast<float>(idx - proSeite / 3);
        }
        d.springeZeile = -1;
    }
    d.scrollZeile = std::clamp(d.scrollZeile, 0.0F, static_cast<float>(std::max(0, sichtbarN - proSeite / 2)));
    const int erste = static_cast<int>(d.scrollZeile);
    // Eingabe
    ImGui::InvisibleButton("##codeflaeche", ImVec2{std::max(1.0F, bereich.x), std::max(1.0F, bereich.y)});
    const bool hover = ImGui::IsItemHovered();
    if (ImGui::IsItemActivated()) { z.codeFokus = true; }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hover && !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        z.codeFokus = false;
    }
    const ImVec2 maus = ImGui::GetIO().MousePos;
    auto offBeiMaus = [&]() -> std::uint32_t {
        const int idx = std::clamp(erste + static_cast<int>((maus.y - o.y) / zh), 0, std::max(0, sichtbarN - 1));
        const int zl = sichtbarN > 0 ? sichtbar[static_cast<std::size_t>(idx)] : 0;
        return p.off(Pos{zl, spalteBei(p.zeile(zl), maus.x - textX, leer)});
    };
    if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (maus.x < textX - falteB) {
            // Rand: Lesezeichen umschalten (Klick in den Rand wie Notepad++)
            const int idx = erste + static_cast<int>((maus.y - o.y) / zh);
            if (idx >= 0 && idx < sichtbarN) {
                const int zl = sichtbar[static_cast<std::size_t>(idx)];
                if (d.lesezeichen.count(zl) != 0) { d.lesezeichen.erase(zl); } else { d.lesezeichen.insert(zl); }
            }
        } else if (maus.x < textX) {
            const int idx = erste + static_cast<int>((maus.y - o.y) / zh);
            if (idx >= 0 && idx < sichtbarN) {
                const int zl = sichtbar[static_cast<std::size_t>(idx)];
                if (d.gefaltet.count(zl) != 0) { d.gefaltet.erase(zl); } else {
                    for (const Falte& f : falten) {
                        if (f.von == zl) { d.gefaltet.insert(zl); }
                    }
                }
            }
        } else if (maus.x > textX + textB) {
            // Minimap: dorthin springen
            const float anteil = (maus.y - o.y) / std::max(1.0F, bereich.y);
            d.scrollZeile = anteil * static_cast<float>(sichtbarN) - static_cast<float>(proSeite) * 0.5F;
        } else {
            const std::uint32_t off = offBeiMaus();
            if (ImGui::GetIO().KeyAlt) {
                p.cursorDazu(off);
            } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                const Treffer w = p.wortBei(off);
                p.cursor.assign(1, Cursor{w.ende, w.anfang});
            } else {
                p.einCursor(off, ImGui::GetIO().KeyShift);
                z.ziehtText = true;
            }
            if (ImGui::GetIO().KeyCtrl) { gehZuDefinition(d); }
            auswahlAusCursor(d);
            z.vervollOffen = false;
        }
    }
    if (z.ziehtText) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            p.cursor.front().pos = offBeiMaus();
            if (maus.y < o.y) { d.scrollZeile -= 1; }
            if (maus.y > o.y + bereich.y) { d.scrollZeile += 1; }
        } else {
            z.ziehtText = false;
        }
    }
    if (z.codeFokus && ImGui::IsWindowFocused()) {
        ImGui::GetIO().WantTextInput = true;
        const int v = p.version();
        codeTasten(d);
        if (p.version() != v) { neuParsen(d); }
    }
    // --- Zeichnen ---
    const std::vector<Farbstueck> farben = faerbe(p.text(), d.datei.dialekt, d.datei.spiel, &d.datei.defines);
    const ZeilenDiff seitSpeichern = zeilenDiff(d.gespeichert, p.text());
    const ZeilenDiff seitLaden = zeilenDiff(d.geladen, p.text());
    std::set<int> fehlerZeilen;
    for (const Meldung& m : d.datei.meldungen) {
        if (m.datei == 0) { fehlerZeilen.insert(m.zeile - 1); }
    }
    // Klammerpaar und Wort unter dem Cursor
    const std::uint32_t cur = p.cursor.front().pos;
    const std::optional<std::uint32_t> partner = p.partnerKlammer(cur);
    std::uint32_t klammer = cur;
    if (partner && (cur >= p.text().size() || std::string("{}()[]").find(p.text()[cur]) == std::string::npos)) { klammer = cur - 1; }
    const Treffer wort = p.wortBei(cur);
    const std::string wortText = p.text().substr(wort.anfang, wort.ende - wort.anfang);
    std::vector<Treffer> gleiche;
    if (wortText.size() >= 2) {
        SuchOptionen so;
        so.gross = true;
        so.wort = true;
        gleiche = p.suche(wortText, so);
    }
    std::vector<Treffer> suchT;
    if (z.suchenOffen) { suchT = p.suche(z.such, z.suchOpt); }
    dl->PushClipRect(o, ImVec2{o.x + bereich.x, o.y + bereich.y}, true);
    std::size_t fi = 0;
    for (int k = erste; k < sichtbarN && k < erste + proSeite + 1; ++k) {
        const int zl = sichtbar[static_cast<std::size_t>(k)];
        const float y = o.y + static_cast<float>(k - erste) * zh;
        const std::uint32_t za = p.zeilenAnfang(zl);
        const std::uint32_t ze = p.zeilenEnde(zl);
        const std::string_view zeile = p.zeile(zl);
        // Zeilennummer
        std::snprintf(nr, sizeof(nr), "%d", zl + 1);
        const bool aktZeile = p.pos(cur).zeile == zl;
        dl->AddText(ImVec2{o.x + nummernB - fs * 0.3F - font->CalcTextSizeA(fs, 1e9F, 0.0F, nr).x, y},
                    aktZeile ? IM_COL32(220, 220, 220, 255) : IM_COL32(120, 120, 120, 255), nr);
        // Rand wie im Skript-Editor: Lesezeichen und Aenderungsstreifen
        std::uint8_t marke = kMarkeKeine;
        const bool seitS = static_cast<std::size_t>(zl) < seitSpeichern.geaendert.size() && seitSpeichern.geaendert[static_cast<std::size_t>(zl)] != 0;
        const bool seitL = static_cast<std::size_t>(zl) < seitLaden.geaendert.size() && seitLaden.geaendert[static_cast<std::size_t>(zl)] != 0;
        if (seitS) { marke = seitL ? kMarkeGeaendert : kMarkeZurueck; }
        else if (seitL) { marke = kMarkeGespeichert; }
        zeichneRinne(dl, o.x + nummernB, rinne, y, zh, marke, d.lesezeichen.count(zl) != 0);
        if (fehlerZeilen.count(zl) != 0) {
            dl->AddRectFilled(ImVec2{textX, y}, ImVec2{textX + textB, y + zh}, IM_COL32(255, 40, 40, 40));
            dl->AddCircleFilled(ImVec2{o.x + nummernB - fs * 0.15F, y + zh * 0.5F}, fs * 0.18F, IM_COL32(240, 70, 70, 255));
        }
        // Falten
        for (const Falte& f : falten) {
            if (f.von == zl) {
                dl->AddText(ImVec2{textX - falteB, y}, IM_COL32(160, 160, 160, 255), d.gefaltet.count(zl) != 0 ? "+" : "-");
                break;
            }
        }
        if (aktZeile && p.cursor.size() == 1 && !p.cursor.front().auswahl()) {
            dl->AddRectFilled(ImVec2{textX, y}, ImVec2{textX + textB, y + zh}, IM_COL32(255, 255, 255, 12));
        }
        // Auswahl, Suchtreffer, gleiche Woerter
        auto markiere = [&](std::uint32_t a, std::uint32_t e, ImU32 col) {
            if (e <= za || a > ze) { return; }
            const int sa = static_cast<int>(std::max(a, za) - za);
            const int se = static_cast<int>(std::min(e, ze) - za);
            const float xa = textX + spaltenX(zeile, sa, leer);
            float xe = textX + spaltenX(zeile, se, leer);
            if (e > ze) { xe += leer; }
            dl->AddRectFilled(ImVec2{xa, y}, ImVec2{std::max(xe, xa + 2), y + zh}, col);
        };
        for (const Treffer& t : gleiche) { markiere(t.anfang, t.ende, IM_COL32(255, 255, 255, 25)); }
        for (const Treffer& t : suchT) { markiere(t.anfang, t.ende, IM_COL32(255, 160, 0, 90)); }
        for (const Cursor& c : p.cursor) {
            if (c.auswahl()) { markiere(c.von(), c.bis(), IM_COL32(38, 79, 120, 220)); }
        }
        if (partner) {
            markiere(klammer, klammer + 1, IM_COL32(255, 255, 255, 60));
            markiere(*partner, *partner + 1, IM_COL32(255, 255, 255, 60));
        }
        // Text mit Farben
        while (fi < farben.size() && farben[fi].ende <= za) { ++fi; }
        std::uint32_t pos = za;
        std::size_t fj = fi;
        while (pos < ze) {
            std::uint32_t bis = ze;
            ImU32 col = farbeFuer(Farbklasse::Text);
            if (fj < farben.size() && farben[fj].anfang <= pos) {
                bis = std::min(ze, farben[fj].ende);
                col = farbeFuer(farben[fj].klasse);
                if (farben[fj].ende <= bis) { ++fj; }
            } else if (fj < farben.size()) {
                bis = std::min(ze, farben[fj].anfang);
            }
            if (bis <= pos) { bis = pos + 1; }
            const int sa = static_cast<int>(pos - za);
            const std::string stueck = anzeige(zeile.substr(static_cast<std::size_t>(sa), bis - pos));
            // Tabs einzeln (Spaltenbreite)
            std::size_t s0 = 0;
            int spalte = sa;
            for (std::size_t q = 0; q <= stueck.size(); ++q) {
                if (q == stueck.size() || stueck[q] == '\t') {
                    if (q > s0) {
                        dl->AddText(font, fs, ImVec2{textX + spaltenX(zeile, spalte, leer), y}, col, stueck.c_str() + s0, stueck.c_str() + q);
                    }
                    if (q < stueck.size()) {
                        spalte = sa + static_cast<int>(q) + 1;
                        s0 = q + 1;
                    }
                }
            }
            pos = bis;
        }
        // Cursor
        if (z.codeFokus && std::fmod(ImGui::GetTime(), 1.0) < 0.6) {
            for (const Cursor& c : p.cursor) {
                if (c.pos >= za && c.pos <= ze) {
                    const float x = textX + spaltenX(zeile, static_cast<int>(c.pos - za), leer);
                    dl->AddLine(ImVec2{x, y}, ImVec2{x, y + zh}, IM_COL32(255, 255, 255, 255), 1.5F);
                }
            }
        }
        if (d.gefaltet.count(zl) != 0) {
            const float x = textX + spaltenX(zeile, static_cast<int>(zeile.size()), leer) + leer;
            dl->AddRectFilled(ImVec2{x, y + 2}, ImVec2{x + leer * 4, y + zh - 2}, IM_COL32(80, 80, 80, 255), 3);
            dl->AddText(ImVec2{x + leer * 0.5F, y}, IM_COL32(200, 200, 200, 255), "...");
        }
    }
    // Minimap
    {
        const float mx = textX + textB;
        dl->AddRectFilled(ImVec2{mx, o.y}, ImVec2{mx + miniB, o.y + bereich.y}, IM_COL32(30, 30, 30, 255));
        const float sk = std::min(2.0F, bereich.y / std::max(1.0F, static_cast<float>(sichtbarN)));
        std::size_t fk = 0;
        for (int k = 0; k < sichtbarN; ++k) {
            const int zl = sichtbar[static_cast<std::size_t>(k)];
            const std::uint32_t za = p.zeilenAnfang(zl);
            const std::uint32_t ze = p.zeilenEnde(zl);
            while (fk < farben.size() && farben[fk].ende <= za) { ++fk; }
            const float y = o.y + static_cast<float>(k) * sk;
            for (std::size_t q = fk; q < farben.size() && farben[q].anfang < ze; ++q) {
                const float x0 = mx + 2 + static_cast<float>(farben[q].anfang - za) * 0.6F;
                const float x1 = mx + 2 + static_cast<float>(std::min(farben[q].ende, ze) - za) * 0.6F;
                if (x0 > mx + miniB) { break; }
                dl->AddRectFilled(ImVec2{x0, y}, ImVec2{std::min(x1, mx + miniB), y + std::max(1.0F, sk - 0.3F)},
                                  (farbeFuer(farben[q].klasse) & 0x00FFFFFFU) | 0xA0000000U);
            }
            if (fehlerZeilen.count(zl) != 0) { dl->AddRectFilled(ImVec2{mx + miniB - 4, y}, ImVec2{mx + miniB, y + 2}, IM_COL32(240, 70, 70, 255)); }
        }
        for (const Treffer& t : suchT) {
            const float y = o.y + static_cast<float>(p.pos(t.anfang).zeile) * sk;
            dl->AddRectFilled(ImVec2{mx + miniB - 4, y}, ImVec2{mx + miniB, y + 2}, IM_COL32(255, 160, 0, 255));
        }
        const float y0 = o.y + static_cast<float>(erste) * sk;
        dl->AddRectFilled(ImVec2{mx, y0}, ImVec2{mx + miniB, y0 + static_cast<float>(proSeite) * sk}, IM_COL32(255, 255, 255, 25));
    }
    dl->PopClipRect();
    // Vervollstaendigung
    if (z.vervollOffen && z.codeFokus && !z.vervoll.empty()) {
        const Pos cp = p.pos(cur);
        const auto it = std::lower_bound(sichtbar.begin(), sichtbar.end(), cp.zeile);
        const float y = o.y + static_cast<float>(static_cast<int>(it - sichtbar.begin()) - erste + 1) * zh;
        const float x = textX + spaltenX(p.zeile(cp.zeile), cp.spalte, leer);
        ImGui::SetNextWindowPos(ImVec2{x, y});
        ImGui::SetNextWindowSize(ImVec2{fs * 16, std::min(10, static_cast<int>(z.vervoll.size())) * zh + 8});
        ImGui::Begin("##vervoll", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_Tooltip);
        for (std::size_t i = 0; i < z.vervoll.size(); ++i) {
            if (ImGui::Selectable(anzeige(z.vervoll[i]).c_str(), static_cast<int>(i) == z.vervollWahl)) {
                z.vervollWahl = static_cast<int>(i);
            }
            if (static_cast<int>(i) == z.vervollWahl) { ImGui::SetScrollHereY(); }
        }
        ImGui::End();
    }
    ImGui::EndChild();
}

// --- Seitenleisten ----------------------------------------------------------------

void zeichnePalette(Dokument& d) {
    struct W {
        int typ;
        Str name;
    };
    static const W w[] = {{ITEM_TYPE_TEXT, Str::MeWText},       {ITEM_TYPE_BUTTON, Str::MeWButton}, {-1, Str::MeWBild},
                          {ITEM_TYPE_YESNO, Str::MeWJaNein},     {ITEM_TYPE_MULTI, Str::MeWMulti},   {ITEM_TYPE_SLIDER, Str::MeWSchieber},
                          {ITEM_TYPE_EDITFIELD, Str::MeWEingabe}, {ITEM_TYPE_LISTBOX, Str::MeWListe}, {ITEM_TYPE_MODEL, Str::MeWModell},
                          {ITEM_TYPE_BIND, Str::MeWTaste}};
    ImGui::SeparatorText(tr(Str::MePalette));
    for (const W& x : w) {
        if (ImGui::Button(tr(x.name), ImVec2{-1, 0})) { fuegeEin(x.typ); }
    }
    if (ImGui::Button(tr(Str::MeBildDazu), ImVec2{-1, 0})) {
        const std::string pfad = platform::openFileDialog(tr(Str::MeBildDazu), "Images (*.png;*.jpg;*.tga)|*.png;*.jpg;*.jpeg;*.tga|", "");
        if (!pfad.empty()) {
            std::ifstream f(fs::u8path(pfad), std::ios::binary);
            std::ostringstream s;
            s << f.rdbuf();
            std::string base = fs::u8path(pfad).stem().u8string();
            for (char& c : base) { c = (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') ? static_cast<char>(std::tolower(static_cast<unsigned char>(c))) : '_'; }
            const std::string ext = kleinPfad(fs::u8path(pfad).extension().u8string());
            const std::string spiel = "gfx/menus/" + base + (ext == ".jpeg" ? std::string(".jpg") : ext);
            d.bilder[spiel] = s.str();
            Z().texturen.erase("gfx/menus/" + base);
            fuegeEin(-1);
            const int mi = menuDerSeite(d);
            if (mi >= 0 && !d.datei.menus[static_cast<std::size_t>(mi)].items.empty()) {
                eigenschaft(static_cast<int>(d.datei.menus[static_cast<std::size_t>(mi)].items.size()) - 1, "background",
                            {inAnf("gfx/menus/" + base)});
            }
            char b[400];
            std::snprintf(b, sizeof(b), tr(Str::MeBildHinzu), fs::u8path(pfad).filename().u8string().c_str(), spiel.c_str());
            Z().meldung = b;
        }
    }
}

void zeichneGliederung(Dokument& d) {
    ImGui::SeparatorText(tr(Str::MeGliederung));
    for (std::size_t m = 0; m < d.datei.menus.size(); ++m) {
        const Menu& me = d.datei.menus[m];
        ImGui::PushID(static_cast<int>(m));
        const bool offen = ImGui::TreeNodeEx("##m", ImGuiTreeNodeFlags_DefaultOpen, "menuDef %s%s", me.window.name.c_str(), me.ok ? "" : " (!)");
        if (ImGui::IsItemClicked()) {
            springe(d, me.anfang);
            const std::vector<Seite> s = seitenImMenue(d.datei);
            for (std::size_t k = 0; k < s.size(); ++k) {
                if (s[k].menu == static_cast<int>(m) && s[k].cvar.empty()) { d.seite = static_cast<int>(k); }
            }
        }
        if (offen) {
            for (std::size_t i = 0; i < me.items.size(); ++i) {
                const Item& it = me.items[i];
                const bool sel = menuDerSeite(d) == static_cast<int>(m) &&
                                 std::find(d.auswahl.begin(), d.auswahl.end(), static_cast<int>(i)) != d.auswahl.end();
                char l[300];
                std::snprintf(l, sizeof(l), "%s %s%s##i%zu", it.window.name.empty() ? "?" : it.window.name.c_str(),
                              typName(it.type), d.gesperrt.count(static_cast<int>(i)) != 0 ? " [L]" : "", i);
                if (ImGui::Selectable(l, sel)) {
                    if (menuDerSeite(d) != static_cast<int>(m)) {
                        const std::vector<Seite> s = seitenImMenue(d.datei);
                        for (std::size_t k = 0; k < s.size(); ++k) {
                            if (s[k].menu == static_cast<int>(m) && s[k].cvar.empty()) { d.seite = static_cast<int>(k); }
                        }
                    }
                    waehle(static_cast<int>(i), ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift);
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void zeichneEigenschaften(Dokument& d) {
    Zustand& z = Z();
    ImGui::SeparatorText(tr(Str::MeEigenschaften));
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.auswahl.empty()) {
        ImGui::TextDisabled("%s", tr(Str::MeNichtsGewaehlt));
        return;
    }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    if (d.auswahl.size() > 1) {
        ImGui::Text("%zu items", d.auswahl.size());
        auto richte = [&](Ausrichtung a) {
            std::vector<Rechteck> r;
            for (const int i : d.auswahl) { r.push_back(m.items[static_cast<std::size_t>(i)].window.rectClient); }
            ausrichten(r, a);
            std::vector<std::pair<int, Rechteck>> neu;
            for (std::size_t k = 0; k < r.size(); ++k) { neu.emplace_back(d.auswahl[k], r[k]); }
            setzeRects(neu);
        };
        if (ImGui::Button(tr(Str::MeLinks))) { richte(Ausrichtung::Links); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeMitteH))) { richte(Ausrichtung::MitteH); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeRechts))) { richte(Ausrichtung::Rechts); }
        if (ImGui::Button(tr(Str::MeOben))) { richte(Ausrichtung::Oben); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeMitteV))) { richte(Ausrichtung::MitteV); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeUnten))) { richte(Ausrichtung::Unten); }
        auto verteile = [&](bool w) {
            std::vector<Rechteck> r;
            for (const int i : d.auswahl) { r.push_back(m.items[static_cast<std::size_t>(i)].window.rectClient); }
            verteilen(r, w);
            std::vector<std::pair<int, Rechteck>> neu;
            for (std::size_t k = 0; k < r.size(); ++k) { neu.emplace_back(d.auswahl[k], r[k]); }
            setzeRects(neu);
        };
        if (ImGui::Button(tr(Str::MeVerteilenH))) { verteile(true); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeVerteilenV))) { verteile(false); }
        static std::string gruppe;
        ImGui::SetNextItemWidth(-ImGui::GetFontSize() * 6);
        eingabe("##gruppe", &gruppe);
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeGruppieren)) && !gruppe.empty()) {
            std::vector<Ersetzung> e;
            for (const int i : d.auswahl) { (void)setzeEigenschaft(d.datei, mi, i, "group", {gruppe}, e); }
            aendere(std::move(e));
        }
        return;
    }
    const int i = d.auswahl[0];
    if (static_cast<std::size_t>(i) >= m.items.size()) { return; }
    const Item& it = m.items[static_cast<std::size_t>(i)];
    ImGui::PushID(i);
    std::string name = it.window.name;
    if (eingabe("name", &name, ImGuiInputTextFlags_EnterReturnsTrue)) { eigenschaft(i, "name", {name}); }
    std::string gruppe = it.window.group;
    if (eingabe("group", &gruppe, ImGuiInputTextFlags_EnterReturnsTrue)) { eigenschaft(i, "group", {gruppe}); }
    // type / style
    int typ = std::clamp(it.type, 0, 14);
    if (ImGui::BeginCombo("type", typName(typ))) {
        for (int t = 0; t < 15; ++t) {
            if (ImGui::Selectable(typName(t), t == typ)) { eigenschaft(i, "type", {konstante(d.datei, typName(t), t)}); }
        }
        ImGui::EndCombo();
    }
    const int sty = std::clamp(it.window.style, 0, 5);
    if (ImGui::BeginCombo("style", styleName(sty))) {
        for (int t = 0; t < 6; ++t) {
            if (ImGui::Selectable(styleName(t), t == sty)) { eigenschaft(i, "style", {konstante(d.datei, styleName(t), t)}); }
        }
        ImGui::EndCombo();
    }
    float r[4] = {it.window.rectClient.x, it.window.rectClient.y, it.window.rectClient.w, it.window.rectClient.h};
    if (ImGui::DragFloat4("rect", r, 1.0F)) {}
    if (ImGui::IsItemDeactivatedAfterEdit()) { setzeRects({{i, Rechteck{r[0], r[1], r[2], r[3]}}}); }
    std::string text = it.text;
    if (eingabe("text", &text, ImGuiInputTextFlags_EnterReturnsTrue)) { eigenschaft(i, "text", {inAnf(text)}); }
    if (!it.text.empty() && it.text[0] == '@') { ImGui::TextDisabled("= %s", anzeige(texte().aufloesen(it.text)).c_str()); }
    float ts = it.textscale;
    ImGui::DragFloat("textscale", &ts, 0.01F, 0.05F, 4.0F);
    if (ImGui::IsItemDeactivatedAfterEdit()) { eigenschaft(i, "textscale", {zahlText(ts)}); }
    float ta[2] = {it.textalignx, it.textaligny};
    ImGui::DragFloat2("textalignx/y", ta, 1.0F);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        std::vector<Ersetzung> e;
        (void)setzeEigenschaft(d.datei, mi, i, "textalignx", {zahlText(ta[0])}, e);
        (void)setzeEigenschaft(d.datei, mi, i, "textaligny", {zahlText(ta[1])}, e);
        aendere(std::move(e));
    }
    static const char* const ausr[] = {"ITEM_ALIGN_LEFT", "ITEM_ALIGN_CENTER", "ITEM_ALIGN_RIGHT"};
    if (ImGui::BeginCombo("textalign", ausr[std::clamp(it.textalignment, 0, 2)])) {
        for (int t = 0; t < 3; ++t) {
            if (ImGui::Selectable(ausr[t], t == it.textalignment)) { eigenschaft(i, "textalign", {konstante(d.datei, ausr[t], t)}); }
        }
        ImGui::EndCombo();
    }
    auto farbe = [&](const char* k, const Farbe& f) {
        float c[4] = {f.r, f.g, f.b, f.a};
        ImGui::ColorEdit4(k, c, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar);
        if (ImGui::IsItemDeactivatedAfterEdit()) { eigenschaft(i, k, {farbText(Farbe{c[0], c[1], c[2], c[3]})}); }
    };
    farbe("forecolor", it.window.foreColor);
    farbe("backcolor", it.window.backColor);
    farbe("bordercolor", it.window.borderColor);
    int border = it.window.border;
    if (ImGui::InputInt("border", &border, 1, 1, ImGuiInputTextFlags_EnterReturnsTrue)) { eigenschaft(i, "border", {std::to_string(border)}); }
    std::string bg = it.window.background;
    if (eingabe("background", &bg, ImGuiInputTextFlags_EnterReturnsTrue)) { eigenschaft(i, "background", {inAnf(bg)}); }
    std::string cvar = it.cvar;
    if (eingabe("cvar", &cvar, ImGuiInputTextFlags_EnterReturnsTrue)) { eigenschaft(i, "cvar", {inAnf(cvar)}); }
    bool sichtbar = (it.window.flags & WINDOW_VISIBLE) != 0;
    if (ImGui::Checkbox("visible", &sichtbar)) { eigenschaft(i, "visible", {sichtbar ? "1" : "0"}); }
    bool gesperrt = d.gesperrt.count(i) != 0;
    if (ImGui::Checkbox(tr(Str::MeSperren), &gesperrt)) {
        if (gesperrt) { d.gesperrt.insert(i); } else { d.gesperrt.erase(i); }
    }
    if (ImGui::Button(tr(Str::MeNachVorn))) {
        std::vector<Ersetzung> e;
        if (itemVerschieben(d.datei, mi, i, true, e)) {
            aendere(std::move(e));
            d.auswahl.assign(1, i + 1);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MeNachHinten))) {
        std::vector<Ersetzung> e;
        if (itemVerschieben(d.datei, mi, i, false, e)) {
            aendere(std::move(e));
            d.auswahl.assign(1, i - 1);
        }
    }
    if (!it.action.empty()) { ImGui::TextWrapped("action: %s", anzeige(it.action).c_str()); }
    (void)z;
    ImGui::PopID();
}

void zeichneCvars(Dokument& d) {
    Zustand& z = Z();
    ImGui::SeparatorText(tr(Str::MeCvars));
    ImGui::TextDisabled("%s", tr(Str::MeCvarsNurVorschau));
    if (ImGui::SmallButton(tr(Str::MeCvarsZuruecksetzen))) {
        z.vorschauCvars.clear();
        z.lauf.cvars.clear();
        ++z.lauf.version;
    }
    for (const CvarFund& c : cvarsImMenue(d.datei)) {
        ImGui::PushID(c.name.c_str());
        std::string w = z.interaktion ? z.lauf.cvar(c.name) : (z.vorschauCvars.count(kleinPfad(c.name)) != 0 ? z.vorschauCvars[kleinPfad(c.name)] : "");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
        if (eingabe("##w", &w)) { setzeCvar(c.name, w); }
        ImGui::SameLine();
        if (c.sichtbarkeit) { ImGui::TextColored(ImVec4{0.4F, 0.8F, 1.0F, 1.0F}, "%s", c.name.c_str()); }
        else { ImGui::TextUnformatted(c.name.c_str()); }
        if (ImGui::IsItemHovered() && !c.wofuer.empty()) {
            std::string t;
            for (const std::string& x : c.wofuer) { t += x + "\n"; }
            ImGui::SetTooltip("%s", t.c_str());
        }
        for (const std::string& v : c.werte) {
            ImGui::SameLine();
            if (ImGui::SmallButton(v.c_str())) { setzeCvar(c.name, v); }
        }
        ImGui::PopID();
    }
}

void zeichneMeldungen(Dokument& d) {
    Zustand& z = Z();
    ImGui::SeparatorText(tr(Str::MeMeldungen));
    for (std::size_t k = 0; k < d.datei.meldungen.size(); ++k) {
        const Meldung& m = d.datei.meldungen[k];
        ImGui::PushID(static_cast<int>(k));
        const std::string t = meldungText(d.datei, m);
        ImGui::PushStyleColor(ImGuiCol_Text, m.fehler ? ImVec4{1, 0.45F, 0.45F, 1} : ImVec4{1, 0.85F, 0.4F, 1});
        if (ImGui::Selectable(t.c_str()) && m.datei == 0 && m.zeile > 0) { springe(d, d.puffer.zeilenAnfang(m.zeile - 1)); }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    if (z.interaktion) {
        ImGui::SeparatorText(tr(Str::MeProtokoll));
        for (const std::string& s : z.lauf.protokoll) { ImGui::TextUnformatted(s.c_str()); }
    }
}

void zeichneSpielListe() {
    Zustand& z = Z();
    if (z.spielListeOffen) {
        ImGui::OpenPopup("##spielmenues");
        z.spielListeOffen = false;
    }
    if (ImGui::BeginPopup("##spielmenues")) {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 20);
        if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
        eingabe("##filter", &z.spielFilter);
        ImGui::BeginChild("##liste", ImVec2{ImGui::GetFontSize() * 24, ImGui::GetFontSize() * 24});
        for (const std::string& p : z.daten.liste("ui/", {".menu", ".txt"})) {
            if (!z.spielFilter.empty() && kleinPfad(p).find(kleinPfad(z.spielFilter)) == std::string::npos) { continue; }
            if (ImGui::Selectable(p.c_str())) {
                oeffneAusSpiel(p);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}

void zeichneLeiste(Dokument* d) {
    Zustand& z = Z();
    if (ImGui::Button(tr(Str::MeAusSpiel))) { z.spielListeOffen = true; }
    zeichneSpielListe();
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MeDateiOeffnen))) {
        const std::string pfad = platform::openFileDialog(tr(Str::MeDateiOeffnen), "Menu (*.menu;*.txt)|*.menu;*.txt|", "");
        if (!pfad.empty()) { (void)oeffneDatei(pfad); }
    }
    if (d != nullptr) {
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeSpeichern))) {
            if (d->platte.empty() || imSpielordner(d->platte)) {
                const std::string ziel = platform::saveFileDialog(tr(Str::MeSpeichern), "Menu (*.menu)|*.menu|", z.ordnerAusgabe,
                                                                  fs::path(d->name).filename().string());
                if (!ziel.empty()) { speichern(*d, ziel); }
            } else {
                speichern(*d, d->platte);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeExportPk3))) {
            const std::string ziel = platform::saveFileDialog(tr(Str::MeExportPk3), "pk3 (*.pk3)|*.pk3|", z.ordnerAusgabe, "zz_menu.pk3");
            if (!ziel.empty()) { (void)exportierePk3(ziel); }
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!d->puffer.kannRueckgaengig());
        if (ImGui::ArrowButton("##undo", ImGuiDir_Left)) { rueckgaengig(); }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(!d->puffer.kannWiederholen());
        if (ImGui::ArrowButton("##redo", ImGuiDir_Right)) { wiederholen(); }
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();
    bool edit = !z.interaktion;
    if (ImGui::RadioButton(tr(Str::MeBearbeiten), edit)) { setzeInteraktion(false); }
    ImGui::SameLine();
    if (ImGui::RadioButton(tr(Str::MeBedienen), !edit)) { setzeInteraktion(true); }
    ImGui::SameLine();
    int sv = z.seitenv == Seitenverhaeltnis::Strecken ? 0 : 1;
    const char* svn[] = {tr(Str::MeStrecken), tr(Str::MeBalken)};
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9);
    if (ImGui::Combo("##sv", &sv, svn, 2)) { z.seitenv = sv == 0 ? Seitenverhaeltnis::Strecken : Seitenverhaeltnis::Balken; }
    ImGui::SameLine();
    static const char* const aufl[] = {"Window", "640x480", "800x600", "1024x768", "1280x720", "1366x768", "1600x900", "1920x1080", "2560x1440", "3440x1440"};
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7);
    ImGui::Combo("##aufl", &z.aufloesung, aufl, 10);
    ImGui::SameLine();
    ImGui::Checkbox(tr(Str::MeAlleItems), &z.alleItems);
    ImGui::SameLine();
    ImGui::Checkbox(tr(Str::MeRaster), &z.raster);
    ImGui::SameLine();
    ImGui::Checkbox(tr(Str::MeAnRaster), &z.anRaster);
    ImGui::SameLine();
    ImGui::Checkbox(tr(Str::MeAnKanten), &z.anKanten);
    if (!z.meldung.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", z.meldung.c_str());
    }
}

void zeichneSeiten(Dokument& d) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    if (!ImGui::BeginTabBar("##seiten", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton)) { return; }
    // Seitenwechsel aus dem Code (Cursor, Gliederung, Skript): den Reiter waehlen
    const bool erzwingen = d.seite != d.seiteGezeigt;
    for (std::size_t k = 0; k < s.size(); ++k) {
        const std::string l = anzeige(s[k].titel) + "##s" + std::to_string(k);
        const ImGuiTabItemFlags f = (erzwingen && static_cast<int>(k) == d.seite) ? ImGuiTabItemFlags_SetSelected : 0;
        if (ImGui::BeginTabItem(l.c_str(), nullptr, f)) {
            if (!erzwingen && d.seite != static_cast<int>(k)) {
                d.seite = static_cast<int>(k);
                d.auswahl.clear();
            }
            d.seiteGezeigt = static_cast<int>(k);
            ImGui::EndTabItem();
        }
    }
    if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
        // neue Seite: ein menuDef am Ende der Datei
        const std::string nl = d.puffer.zeilenUmbruch();
        std::string name = "newMenu";
        for (int n = 2; n < 100; ++n) {
            bool frei = true;
            for (const Menu& m : d.datei.menus) { frei = frei && m.window.name != name; }
            if (frei) { break; }
            name = "newMenu" + std::to_string(n);
        }
        const std::string block = "\tmenuDef" + nl + "\t{" + nl + "\t\tname\t\t\t\"" + name + "\"" + nl + "\t\tfullScreen\t\t1" + nl +
                                  "\t\trect\t\t\t0 0 640 480" + nl + "\t\tvisible\t\t\t1" + nl + "\t\tfocusColor\t\t1 1 1 1" + nl + "\t}" + nl;
        // vor die letzte } der Datei, sonst ans Ende
        const std::string& t = d.puffer.text();
        std::size_t p = t.rfind('}');
        std::size_t q = (p != std::string::npos && !d.datei.menus.empty() && p >= d.datei.menus.back().ende) ? p : t.size();
        if (q < t.size()) {
            while (q > 0 && (t[q - 1] == ' ' || t[q - 1] == '\t')) { --q; }
        }
        aendere({Ersetzung{static_cast<std::uint32_t>(q), static_cast<std::uint32_t>(q), block}});
        d.seite = static_cast<int>(seitenImMenue(d.datei).size()) - 1;
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeNeueSeite)); }
    ImGui::EndTabBar();
}

void splitter(const char* id, float& wert, bool senkrecht, float min, float max, float laenge, bool umgekehrt = false) {
    ImGui::InvisibleButton(id, senkrecht ? ImVec2{6, std::max(1.0F, laenge)} : ImVec2{std::max(1.0F, laenge), 6});
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(senkrecht ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    }
    if (ImGui::IsItemActive()) {
        const float dlt = senkrecht ? ImGui::GetIO().MouseDelta.x : -ImGui::GetIO().MouseDelta.y;
        wert += umgekehrt ? -dlt : dlt;
        wert = std::clamp(wert, min, max);
    }
}

}  // namespace

// ============================================================================

bool aktiv() { return g_app != nullptr && g_app->leftMode == 4; }

void zeichne(float hoehe) {
    datenPruefen();
    Zustand& z = Z();
    Dokument* d = doc();
    if (d != nullptr) { neuParsen(*d); }
    zeichneLeiste(d);
    // Dokument-Reiter
    if (z.docs.size() > 1 || d != nullptr) {
        if (ImGui::BeginTabBar("##menuedocs", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_AutoSelectNewTabs)) {
            int schliessen = -1;
            for (std::size_t k = 0; k < z.docs.size(); ++k) {
                bool offen = true;
                const bool geaendert = z.docs[k]->puffer.text() != z.docs[k]->gespeichert;
                const std::string l = z.docs[k]->name + (geaendert ? " *" : "") + "##d" + std::to_string(k);
                if (ImGui::BeginTabItem(l.c_str(), &offen)) {
                    z.aktiv = static_cast<int>(k);
                    ImGui::EndTabItem();
                }
                if (!offen) { schliessen = static_cast<int>(k); }
            }
            ImGui::EndTabBar();
            if (schliessen >= 0) {
                z.docs.erase(z.docs.begin() + schliessen);
                z.aktiv = std::min(z.aktiv, static_cast<int>(z.docs.size()) - 1);
                z.laufDatei = -1;
                return;
            }
        }
    }
    d = doc();
    if (d == nullptr) {
        ImGui::TextDisabled("%s", tr(Str::MeKeinMenue));
        return;
    }
    neuParsen(*d);
    const float fs = ImGui::GetFontSize();
    const ImVec2 rest = ImGui::GetContentRegionAvail();
    if (z.codeHoehe <= 0) { z.codeHoehe = std::floor(rest.y * 0.38F); }
    if (z.linksBreite <= 0) { z.linksBreite = fs * 13; }
    if (z.rechtsBreite <= 0) { z.rechtsBreite = fs * 19; }
    const float obenH = std::max(fs * 6, rest.y - z.codeHoehe - 8);
    (void)hoehe;
    ImGui::BeginChild("##links", ImVec2{z.linksBreite, obenH}, ImGuiChildFlags_Borders);
    zeichnePalette(*d);
    zeichneGliederung(*d);
    ImGui::EndChild();
    ImGui::SameLine(0, 0);
    splitter("##spl1", z.linksBreite, true, fs * 6, rest.x * 0.4F, obenH);
    ImGui::SameLine(0, 0);
    const float mitteB = std::max(fs * 10, rest.x - z.linksBreite - z.rechtsBreite - 14);
    ImGui::BeginChild("##mitte", ImVec2{mitteB, obenH}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    zeichneSeiten(*d);
    zeichneFlaeche(*d, ImGui::GetContentRegionAvail());
    ImGui::EndChild();
    ImGui::SameLine(0, 0);
    splitter("##spl2", z.rechtsBreite, true, fs * 10, rest.x * 0.5F, obenH, true);
    ImGui::SameLine(0, 0);
    ImGui::BeginChild("##rechts", ImVec2{0, obenH}, ImGuiChildFlags_Borders);
    zeichneEigenschaften(*d);
    zeichneCvars(*d);
    zeichneMeldungen(*d);
    ImGui::EndChild();
    splitter("##spl3", z.codeHoehe, false, fs * 4, rest.y - fs * 8, rest.x);
    zeichneCode(*d, ImVec2{rest.x, z.codeHoehe});
}

bool oeffneAusSpiel(const std::string& spielPfad) {
    datenPruefen();
    std::string t;
    if (!Z().daten.lies(spielPfad, t)) { return false; }
    return oeffne(spielPfad, "", t);
}

bool oeffneDatei(const std::string& platte) {
    const std::string k = kleinPfad(platte);
    if (k.size() < 5 || (k.compare(k.size() - 5, 5, ".menu") != 0 && k.compare(k.size() - 4, 4, ".txt") != 0)) { return false; }
    datenPruefen();
    std::ifstream f(fs::u8path(platte), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    return oeffne(spielPfadVon(platte), platte, s.str());
}

bool oeffneText(const std::string& name, const std::string& text) {
    datenPruefen();
    return oeffne(name, "", text);
}

Stand stand() {
    Stand s;
    Zustand& z = Z();
    s.dokumente = static_cast<int>(z.docs.size());
    Dokument* d = doc();
    if (d == nullptr) { return s; }
    neuParsen(*d);
    s.menus = static_cast<int>(d->datei.menus.size());
    for (const Menu& m : d->datei.menus) { s.items += static_cast<int>(m.items.size()); }
    s.seite = d->seite;
    s.seiten = static_cast<int>(seitenImMenue(d->datei).size());
    s.auswahl = d->auswahl;
    s.text = d->puffer.text();
    s.meldungen = static_cast<int>(d->datei.meldungen.size());
    s.prims = z.letztePrims;
    for (const auto& [k, t] : z.texturen) { (void)k; s.bilderGeladen += t.tex != nullptr ? 1 : 0; }
    s.interaktion = z.interaktion;
    s.cursorZeile = std::string(d->puffer.zeile(d->puffer.pos(d->puffer.cursor.front().pos).zeile));
    return s;
}

void waehle(int item, bool dazu) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const auto it = std::find(d->auswahl.begin(), d->auswahl.end(), item);
    if (dazu) {
        if (it != d->auswahl.end()) { d->auswahl.erase(it); } else { d->auswahl.push_back(item); }
    } else {
        d->auswahl.assign(1, item);
    }
    const int mi = menuDerSeite(*d);
    if (mi >= 0 && item >= 0 && static_cast<std::size_t>(item) < d->datei.menus[static_cast<std::size_t>(mi)].items.size()) {
        springe(*d, d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(item)].anfang);
    }
}

void verschiebeAuswahl(float dx, float dy) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    std::vector<std::pair<int, Rechteck>> neu;
    for (const int i : d->auswahl) {
        if (static_cast<std::size_t>(i) >= m.items.size() || d->gesperrt.count(i) != 0) { continue; }
        Rechteck r = m.items[static_cast<std::size_t>(i)].window.rectClient;
        r.x += dx;
        r.y += dy;
        neu.emplace_back(i, r);
    }
    setzeRects(neu);
}

void setzeInteraktion(bool an) {
    Zustand& z = Z();
    z.interaktion = an;
    z.laufDatei = -1;
    z.laufVersion = -1;
    z.lauf.cvars.clear();
    z.lauf.protokoll.clear();
}

void klickVorschau(float x, float y) {
    Zustand& z = Z();
    Dokument* d = doc();
    if (d == nullptr || !z.interaktion) { return; }
    const int mi = menuDerSeite(*d);
    if (z.laufDatei != z.aktiv || z.lauf.d != &d->datei) {
        z.lauf.start(d->datei);
        z.lauf.basis = umgebung(*d);
        if (mi >= 0) { z.lauf.oeffne(d->datei.menus[static_cast<std::size_t>(mi)].window.name); }
        z.laufDatei = z.aktiv;
    }
    z.lauf.maus(mi, x, y);
    z.lauf.klick(mi, x, y);
}

void setzeCvar(const std::string& name, const std::string& wert) {
    Zustand& z = Z();
    if (z.interaktion) { z.lauf.setzeCvar(name, wert); }
    else { z.vorschauCvars[kleinPfad(name)] = wert; }
}

std::string cvarWert(const std::string& name) {
    Zustand& z = Z();
    if (z.interaktion) { return z.lauf.cvar(name); }
    const auto it = z.vorschauCvars.find(kleinPfad(name));
    return it == z.vorschauCvars.end() ? std::string() : it->second;
}

bool rueckgaengig() {
    Dokument* d = doc();
    if (d == nullptr || !d->puffer.rueckgaengig()) { return false; }
    neuParsen(*d);
    return true;
}

bool wiederholen() {
    Dokument* d = doc();
    if (d == nullptr || !d->puffer.wiederholen()) { return false; }
    neuParsen(*d);
    return true;
}

void fuegeEin(int typ) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    const std::string basis = typ < 0 ? "image" : std::string(typName(typ)).substr(10);
    std::string name = basis;
    for (char& c : name) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    name = freierName(m, name);
    std::string einr = "\t\t";
    if (!m.items.empty() && m.items.front().kopf.datei == 0) {
        const std::string& t = d->puffer.text();
        std::size_t a = m.items.front().anfang;
        std::size_t s = a;
        while (s > 0 && t[s - 1] != '\n') { --s; }
        einr = t.substr(s, a - s);
    }
    const Rechteck r{static_cast<float>(220 - m.window.rect.x), static_cast<float>(200 - m.window.rect.y), 200, 40};
    std::vector<Ersetzung> e;
    if (itemEinfuegen(d->datei, mi, itemVorlage(typ, d->datei.dialekt, name, r, einr, d->puffer.zeilenUmbruch()), e)) {
        aendere(std::move(e));
        const int neu = static_cast<int>(d->datei.menus[static_cast<std::size_t>(mi)].items.size()) - 1;
        waehle(neu, false);
    }
}

int suche(const std::string& muster, bool regex) {
    Dokument* d = doc();
    if (d == nullptr) { return 0; }
    Zustand& z = Z();
    z.such = muster;
    z.suchOpt.regex = regex;
    z.suchenOffen = true;
    return static_cast<int>(d->puffer.suche(muster, z.suchOpt).size());
}

int ersetzeAlle(const std::string& muster, const std::string& ersatz, bool regex) {
    Dokument* d = doc();
    if (d == nullptr) { return 0; }
    SuchOptionen o;
    o.regex = regex;
    const int n = d->puffer.ersetzeAlle(muster, ersatz, o);
    neuParsen(*d);
    return n;
}

void springeZuItem(int item) { waehle(item, false); }

bool speichereUnter(const std::string& pfad) {
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    speichern(*d, pfad);
    return d->platte == pfad;
}

bool exportierePk3(const std::string& pfad) {
    Zustand& z = Z();
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    if (imSpielordner(pfad)) {
        z.meldung = tr(Str::MeNieSpielordner);
        return false;
    }
    std::vector<Pk3Datei> dateien;
    dateien.push_back(Pk3Datei{d->name, d->puffer.text()});
    for (const auto& [p, b] : d->bilder) { dateien.push_back(Pk3Datei{p, b}); }
    std::string fehler;
    const bool ok = writePk3(pfad, dateien, &fehler);
    char b[600];
    std::snprintf(b, sizeof(b), ok ? tr(Str::MeExportiert) : tr(Str::MeNichtGespeichert), pfad.c_str());
    z.meldung = b;
    addStatus(b);
    return ok;
}

}  // namespace bhed::gui::menueeditor
