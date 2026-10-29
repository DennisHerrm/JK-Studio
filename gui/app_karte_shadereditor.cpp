// app_karte_shadereditor.cpp - Karten-Editor Stufe 9d: Shader live bearbeiten (eigene Funktion,
// nicht in NetRadiant-Custom; NRCs "Open shader" (DoShaderView) bleibt wie es ist).
//
// Der Shader der Auswahl (bzw. der unter der Maus im Texturbrowser) oeffnet seine .shader-Datei,
// an seiner Zeile, in einem Textfenster mit Syntaxfarben und dem Rand des Skript-Editors
// (Zeilennummern, Aenderungsstreifen). Gespeichert wird NIE in einen Spielordner: liegt die
// Datei im Spiel (Archiv oder Spielordner), geht sie unter demselben Namen in den Import-Ordner
// (<Einstellungen>/import/scripts/...) - der steht im Suchpfad vor dem Spiel und ersetzt so die
// ganze Datei. Liegt sie schon im Import-Ordner oder in einem Zusatz-Suchordner des Anwenders,
// wird sie dort gespeichert. Danach (und wenn sich eine .shader-Datei im Import-Ordner oder die
// offene Datei auf der Platte aendert) laden Shader-Inventar, Texturbrowser, Kamera und
// Echtzeit-Licht neu - ohne Neustart ("Flush & Reload Shaders").
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "platform.h"

#include "bhed/i18n.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

namespace fs = std::filesystem;

fs::path pfadVon(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
Editor& e() { return editor(); }

struct ShaderEditor {
    bool offen = false;
    std::string shader;          // der Shader, mit dem geoeffnet wurde
    std::string datei;           // Pfad im Suchpfad ("scripts/x.shader")
    std::string quelle;          // woher gelesen (Ordner oder Archiv)
    std::string ziel;            // wohin gespeichert wird (Datei auf der Platte)
    bool neuImImport = false;    // das Ziel gibt es noch nicht: nach dem Speichern den Suchpfad neu lesen
    std::string text;
    std::vector<std::string> original;   // Zeilen beim Oeffnen
    std::vector<std::string> gespeichert;   // Zeilen beim letzten Speichern
    bool geaendert = false;
    int springen = -1;
    int springenBilder = 0;
    // Dateiwaechter
    std::chrono::steady_clock::time_point letzterBlick = std::chrono::steady_clock::now();
    std::map<std::string, fs::file_time_type> stempel;
    bool stempelDa = false;
    std::size_t neuGeladen = 0;
    std::string meldung;
};
ShaderEditor g_se;

std::string kleinS(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::vector<std::string> zeilenVon(const std::string& t) {
    std::vector<std::string> z;
    std::size_t p = 0;
    while (p <= t.size()) {
        const std::size_t q = t.find('\n', p);
        std::string x = t.substr(p, q == std::string::npos ? std::string::npos : q - p);
        if (!x.empty() && x.back() == '\r') { x.pop_back(); }
        z.push_back(x);
        if (q == std::string::npos) { break; }
        p = q + 1;
    }
    return z;
}

// "<name>" bzw. "<name>:q3map" auf einer Zeile, danach eine Zeile mit "{" (wie DoGtkTextEditor)
int zeileDesShaders(const std::vector<std::string>& zeilen, const std::string& name) {
    const std::string n = kleinS(name);
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        std::size_t a = 0;
        while (a < zeilen[i].size() && std::isspace(static_cast<unsigned char>(zeilen[i][a])) != 0) { ++a; }
        std::string rest = kleinS(zeilen[i].substr(a));
        while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.back())) != 0) { rest.pop_back(); }
        if (rest == n || rest == n + ":q3map") { return static_cast<int>(i); }
    }
    return -1;
}

std::string importOrdnerAnlegen() {
    const std::string o = platform::settingsDirectory() + "\\import";
    std::error_code ec;
    fs::create_directories(pfadVon(o), ec);
    return o;
}

bool imOrdner(const std::string& datei, const std::string& ordner) {
    if (ordner.empty()) { return false; }
    std::string a = kleinS(datei);
    std::string b = kleinS(ordner);
    std::replace(a.begin(), a.end(), '\\', '/');
    std::replace(b.begin(), b.end(), '\\', '/');
    if (!b.empty() && b.back() != '/') { b += '/'; }
    return a.rfind(b, 0) == 0;
}

// Wohin gespeichert wird: nie in einen Spielordner (siehe oben)
void zielBestimmen() {
    Editor& ed = e();
    g_se.ziel.clear();
    g_se.neuImImport = false;
    if (!ed.karteSpiel) { return; }
    bool ordner = false;
    const std::string wurzel = ed.karteSpiel->fs.finden(g_se.datei, &ordner);
    g_se.quelle = wurzel;
    const std::string imp = importOrdner();
    bool eigenerOrdner = false;
    if (ordner && !wurzel.empty()) {
        if (!imp.empty() && imOrdner(wurzel, imp)) { eigenerOrdner = true; }
        for (const std::string& z : ed.zusatzPfade) {
            if (!z.empty() && imOrdner(wurzel, z)) { eigenerOrdner = true; }
        }
    }
    if (eigenerOrdner) {
        g_se.ziel = karte::ordnerBereinigt(wurzel) + g_se.datei;
    } else {
        const std::string o = importOrdnerAnlegen();
        std::string d = g_se.datei;
        std::replace(d.begin(), d.end(), '/', '\\');
        g_se.ziel = o + "\\" + d;
        std::error_code ec;
        g_se.neuImImport = !fs::exists(pfadVon(g_se.ziel), ec) || imp.empty();
    }
}

void neuLaden() {
    texturBefehl("RefreshShaders");
    ++g_se.neuGeladen;
}

// Die .shader-Dateien, die der Waechter beobachtet: shaders/ und scripts/ im Import-Ordner und das Ziel des Editors
std::map<std::string, fs::file_time_type> stempelJetzt() {
    std::map<std::string, fs::file_time_type> m;
    std::error_code ec;
    const std::string imp = importOrdner();
    for (const char* unter : {"shaders", "scripts"}) {   // JA: shaders/ (shaderpath), Q3-Spiele: scripts/
        if (imp.empty()) { break; }
        const fs::path s = pfadVon(imp) / unter;
        if (!fs::is_directory(s, ec)) { continue; }
        for (const auto& eintrag : fs::directory_iterator(s, ec)) {
            if (eintrag.path().extension() == ".shader") {
                const std::u8string u = eintrag.path().u8string();
                m[std::string(u.begin(), u.end())] = eintrag.last_write_time(ec);
            }
        }
    }
    if (g_se.offen && !g_se.ziel.empty() && fs::exists(pfadVon(g_se.ziel), ec)) { m[g_se.ziel] = fs::last_write_time(pfadVon(g_se.ziel), ec); }
    return m;
}

// Syntaxfarben: Kommentar, Klammern, Zahlen, bekannte Woerter, Shadernamen (oberste Ebene)
const std::set<std::string>& schluesselwoerter() {
    static const std::set<std::string> s = {
        "surfaceparm", "map", "clampmap", "animmap", "videomap", "blendfunc", "rgbgen", "alphagen", "tcgen", "tcmod", "alphafunc", "depthfunc",
        "depthwrite", "detail", "glow", "cull", "sort", "nopicmip", "nomipmaps", "polygonoffset", "portal", "deformvertexes", "skyparms",
        "fogparms", "entitymergable", "nomerge", "lightmap", "q3map_surfacelight", "q3map_lightsubdivide", "q3map_lightimage", "q3map_lightrgb",
        "q3map_sun", "q3map_sunext", "sun", "q3map_skylight", "q3map_backsplash", "q3map_nolightmap", "q3map_novertexshadows", "q3map_forcemeta",
        "q3map_fogdir", "q3map_globaltexture", "q3map_tesssize", "q3map_material", "q3map_lightmapsize", "q3map_baseshader", "q3map_remapshader",
        "qer_editorimage", "qer_trans", "qer_nocarve", "qer_alphafunc", "material", "spritegen", "spritetype", "surfacesprites", "ss_density",
        "ss_wind", "ss_fadedist", "ss_variance", "ss_vertical", "ss_faceup", "noglfog", "notc", "hitlocation", "hitmaterial"};
    return s;
}

struct Stueck {
    std::size_t von = 0;
    std::size_t bis = 0;
    ImU32 farbe = 0;
};

std::vector<Stueck> faerben(const std::string& z, int tiefeAmAnfang) {
    std::vector<Stueck> s;
    const ImU32 kText = IM_COL32(220, 220, 220, 255);
    const ImU32 kKommentar = IM_COL32(106, 153, 85, 255);
    const ImU32 kWort = IM_COL32(86, 156, 214, 255);
    const ImU32 kZahl = IM_COL32(181, 206, 168, 255);
    const ImU32 kKlammer = IM_COL32(255, 215, 0, 255);
    const ImU32 kName = IM_COL32(220, 220, 170, 255);
    const ImU32 kPfad = IM_COL32(206, 145, 120, 255);
    std::size_t i = 0;
    bool erstesWort = true;
    while (i < z.size()) {
        const char c = z[i];
        if (c == '/' && i + 1 < z.size() && z[i + 1] == '/') {
            s.push_back({i, z.size(), kKommentar});
            break;
        }
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            const std::size_t a = i;
            while (i < z.size() && std::isspace(static_cast<unsigned char>(z[i])) != 0) { ++i; }
            s.push_back({a, i, kText});
            continue;
        }
        if (c == '{' || c == '}' || c == '(' || c == ')') {
            s.push_back({i, i + 1, kKlammer});
            ++i;
            erstesWort = false;
            continue;
        }
        const std::size_t a = i;
        while (i < z.size() && std::isspace(static_cast<unsigned char>(z[i])) == 0 && z[i] != '{' && z[i] != '}' && z[i] != '(' && z[i] != ')' &&
               !(z[i] == '/' && i + 1 < z.size() && z[i + 1] == '/')) {
            ++i;
        }
        const std::string w = z.substr(a, i - a);
        const std::string k = kleinS(w);
        ImU32 f = kText;
        if (tiefeAmAnfang == 0 && erstesWort) {
            f = kName;
        } else if (schluesselwoerter().count(k) != 0U || k.rfind("q3map_", 0) == 0 || k.rfind("qer_", 0) == 0 || k.rfind("gl_", 0) == 0) {
            f = kWort;
        } else if (std::isdigit(static_cast<unsigned char>(w[0])) != 0 || ((w[0] == '-' || w[0] == '.') && w.size() > 1)) {
            f = kZahl;
        } else if (w.find('/') != std::string::npos || w[0] == '$' || w[0] == '*') {
            f = kPfad;
        }
        s.push_back({a, i, f});
        erstesWort = false;
    }
    return s;
}

}  // namespace

void shaderEditorOeffnen(const std::string& shader) {
    Editor& ed = e();
    if (!ed.karteSpiel || shader.empty()) { return; }
    const karte::ShaderEintrag* se = ed.karteSpiel->inventar.shaderFinden(shader);
    if (se == nullptr || se->istStandard() || se->datei.empty()) {
        warnung(std::string(trK(Str::KeSeKeinSkript)) + " " + shader);
        return;
    }
    std::string text;
    if (!ed.karteSpiel->fs.dateiLesen(se->datei, text)) {
        fehler("Failed to load shader file " + se->datei);
        return;
    }
    g_se.offen = true;
    g_se.shader = shader;
    g_se.datei = se->datei;
    g_se.text = text;
    g_se.original = zeilenVon(text);
    g_se.gespeichert.clear();
    g_se.geaendert = false;
    g_se.springen = zeileDesShaders(g_se.original, shader);
    g_se.springenBilder = 3;
    g_se.meldung.clear();
    zielBestimmen();
    g_se.stempel = stempelJetzt();
    g_se.stempelDa = true;
}

bool shaderEditorSpeichern() {
    if (!g_se.offen || g_se.ziel.empty()) { return false; }
    std::error_code ec;
    fs::create_directories(pfadVon(g_se.ziel).parent_path(), ec);
    std::ofstream f(pfadVon(g_se.ziel), std::ios::binary | std::ios::trunc);
    if (!f) {
        fehler("Error saving file\"" + g_se.ziel + "\"");
        return false;
    }
    f << g_se.text;
    f.close();
    g_se.geaendert = false;
    g_se.gespeichert = zeilenVon(g_se.text);
    char z[1400];
    std::snprintf(z, sizeof(z), trK(Str::KeSeGespeichert), g_se.ziel.c_str());
    ausgabe(z);
    g_se.meldung = z;
    if (g_se.neuImImport) {
        // eine neue Datei im Import-Ordner: der Suchpfad wird neu gelesen (EnginePath_Realise)
        e().spielKennung.clear();
        g_se.neuImImport = false;
        ++g_se.neuGeladen;
    } else {
        neuLaden();
    }
    g_se.stempel = stempelJetzt();   // die eigene Aenderung nicht noch einmal melden
    return true;
}

std::string shaderEditorZiel() { return g_se.ziel; }
std::string& shaderEditorText() { return g_se.text; }
bool shaderEditorOffen() { return g_se.offen; }
void shaderEditorSchliessen() { g_se.offen = false; }
void shaderEditorTextGeaendert() { g_se.geaendert = true; }
std::size_t shaderNeuGeladen() { return g_se.neuGeladen; }

void shaderEditorSchritt() {
    const auto jetzt = std::chrono::steady_clock::now();
    if (std::chrono::duration<double>(jetzt - g_se.letzterBlick).count() < 0.5) { return; }
    g_se.letzterBlick = jetzt;
    const auto m = stempelJetzt();
    if (!g_se.stempelDa) {
        g_se.stempel = m;
        g_se.stempelDa = true;
        return;
    }
    if (m == g_se.stempel) { return; }
    const bool neueDatei = std::any_of(m.begin(), m.end(), [](const auto& x) { return g_se.stempel.count(x.first) == 0U; });
    g_se.stempel = m;
    // die offene Datei von aussen geaendert und hier nichts geaendert: neu lesen
    if (g_se.offen && !g_se.geaendert && !g_se.ziel.empty()) {
        std::ifstream f(pfadVon(g_se.ziel), std::ios::binary);
        if (f) {
            g_se.text.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
            g_se.gespeichert = zeilenVon(g_se.text);
        }
    }
    ausgabe(trK(Str::KeSeNeuGeladen));
    if (neueDatei) {
        e().spielKennung.clear();
        ++g_se.neuGeladen;
    } else {
        neuLaden();
    }
}

void shaderEditorZeichnen() {
    if (!g_se.offen) { return; }
    const float sk = massstab();
    ImGui::SetNextWindowSize(ImVec2{std::round(640.0F * sk), std::round(720.0F * sk)}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(trK(Str::KeSeTitel)) + " - " + (g_se.geaendert ? "*" : "") + g_se.datei + "###shaderlive";
    bool offen = true;
    if (!ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        if (!offen) { g_se.offen = false; }
        return;
    }
    // Kopf: wohin gespeichert wird, Shader der Auswahl
    ImGui::TextDisabled("%s %s", trK(Str::KeSeZiel), g_se.ziel.c_str());
    if (ImGui::Button(trK(Str::KeSeAuswahl))) { shaderEditorOeffnen(e().texBro.shader); }
    ImGui::SameLine();
    ImGui::BeginDisabled(!g_se.geaendert);
    if (ImGui::Button(trK(Str::KeSave))) { (void)shaderEditorSpeichern(); }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(trK(Str::KeTFlushReloadShaders))) { neuLaden(); }
    if (!g_se.meldung.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", g_se.meldung.c_str());
    }
    // Rand wie der Skript-Editor: Zeilennummer, Aenderungsstreifen; daneben der Text mit Syntaxfarben
    const std::vector<std::string> zeilen = zeilenVon(g_se.text);
    ImFont* font = ImGui::GetFont();
    const float fsz = ImGui::GetFontSize();
    char nr[16];
    std::snprintf(nr, sizeof(nr), "%zu", zeilen.size());
    const float nummernB = font->CalcTextSizeA(fsz, 1e9F, 0.0F, nr).x + fsz * 0.6F;
    const float rinne = randBreiteSkript();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 platz = ImGui::GetContentRegionAvail();
    ImGui::SetCursorScreenPos(ImVec2{start.x + nummernB + rinne, start.y});
    const char* const kennung = "##shaderlivetext";
    const ImGuiID id = ImGui::GetID(kennung);
    ImGuiWindow* fenster = ImGui::GetCurrentWindow();
    if (g_se.springen >= 0 && g_se.springenBilder > 0) {   // zur Zeile des Shaders rollen
        char kn[512];
        std::snprintf(kn, sizeof(kn), "%s/%s_%08X", fenster->Name, kennung, id);
        if (ImGuiWindow* kind = ImGui::FindWindowByName(kn)) {
            ImGui::SetScrollY(kind, static_cast<float>(g_se.springen) * ImGui::GetTextLineHeight());
        }
        if (--g_se.springenBilder == 0) { g_se.springen = -1; }
    }
    std::vector<char> puffer(g_se.text.begin(), g_se.text.end());
    puffer.resize(g_se.text.size() + 8192, '\0');
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));   // der Text kommt gefaerbt darueber
    const bool neu = ImGui::InputTextMultiline(kennung, puffer.data(), puffer.size(), ImVec2{platz.x - nummernB - rinne, platz.y},
                                               ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopStyleColor();
    if (neu) {
        g_se.text = puffer.data();
        g_se.geaendert = true;
    }
    // das Kindfenster des Textfelds: Rollen und Lage
    char kn[512];
    std::snprintf(kn, sizeof(kn), "%s/%s_%08X", fenster->Name, kennung, id);
    ImGuiWindow* kind = ImGui::FindWindowByName(kn);
    if (kind != nullptr) {
        const float zh = ImGui::GetTextLineHeight();
        ImVec2 o = kind->DC.CursorStartPos;
        float scrollX = 0;
        if (ImGuiInputTextState* st = ImGui::GetInputTextState(id)) { scrollX = st->Scroll.x; }
        o.x -= scrollX;
        ImDrawList* dl = kind->DrawList;
        dl->PushClipRect(kind->InnerClipRect.Min, kind->InnerClipRect.Max, true);
        const int erste = std::max(0, static_cast<int>((kind->InnerClipRect.Min.y - o.y) / zh));
        const int letzte = std::min(static_cast<int>(zeilen.size()), static_cast<int>((kind->InnerClipRect.Max.y - o.y) / zh) + 1);
        // Tiefe der geschweiften Klammern am Anfang jeder Zeile (Shadernamen auf oberster Ebene)
        int tiefe = 0;
        for (int i = 0; i < letzte; ++i) {
            const std::string& z = zeilen[static_cast<std::size_t>(i)];
            if (i >= erste) {
                const float y = o.y + static_cast<float>(i) * zh;
                for (const Stueck& s : faerben(z, tiefe)) {
                    const float x = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, z.c_str(), z.c_str() + s.von).x;
                    dl->AddText(font, fsz, ImVec2{x, y}, s.farbe, z.c_str() + s.von, z.c_str() + s.bis);
                }
            }
            const std::size_t k = z.find("//");
            for (std::size_t c = 0; c < (k == std::string::npos ? z.size() : k); ++c) {
                if (z[c] == '{') { ++tiefe; }
                if (z[c] == '}') { tiefe = std::max(0, tiefe - 1); }
            }
        }
        // der Cursor (die Textfarbe ist durchsichtig)
        if (ImGuiInputTextState* st = ImGui::GetInputTextState(id); st != nullptr && ImGui::GetActiveID() == id) {
            const int pos = st->GetCursorPos();
            const std::string& t = g_se.text;
            const std::size_t p = std::min<std::size_t>(static_cast<std::size_t>(std::max(pos, 0)), t.size());
            const std::size_t zeilenAnfang = p == 0 ? 0 : t.rfind('\n', p - 1) + 1;
            const int zeile = static_cast<int>(std::count(t.begin(), t.begin() + static_cast<std::ptrdiff_t>(p), '\n'));
            const float x = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, t.c_str() + zeilenAnfang, t.c_str() + p).x;
            const float y = o.y + static_cast<float>(zeile) * zh;
            dl->AddLine(ImVec2{x, y}, ImVec2{x, y + zh}, IM_COL32(230, 230, 230, 255), 1.0F);
        }
        dl->PopClipRect();
        // links: Nummern und Rand (Aenderung gegen das Oeffnen bzw. das letzte Speichern)
        ImDrawList* ad = ImGui::GetWindowDrawList();
        ad->PushClipRect(ImVec2{start.x, kind->InnerClipRect.Min.y}, ImVec2{start.x + nummernB + rinne, kind->InnerClipRect.Max.y}, true);
        for (int i = erste; i < letzte; ++i) {
            const float y = o.y + static_cast<float>(i) * zh;
            char n[16];
            std::snprintf(n, sizeof(n), "%d", i + 1);
            const float b = font->CalcTextSizeA(fsz, 1e9F, 0.0F, n).x;
            ad->AddText(font, fsz, ImVec2{start.x + nummernB - b - fsz * 0.3F, y}, IM_COL32(130, 130, 130, 255), n);
            const std::size_t zi = static_cast<std::size_t>(i);
            const std::string& z = zeilen[zi];
            std::uint8_t marke = kMarkeKeine;
            const bool gleichOriginal = zi < g_se.original.size() && g_se.original[zi] == z;
            if (!g_se.gespeichert.empty()) {
                const bool gleichGespeichert = zi < g_se.gespeichert.size() && g_se.gespeichert[zi] == z;
                marke = gleichGespeichert ? (gleichOriginal ? kMarkeKeine : kMarkeGespeichert) : kMarkeGeaendert;
            } else if (!gleichOriginal) {
                marke = kMarkeGeaendert;
            }
            randZeichnenSkript(ad, start.x + nummernB, rinne, y, zh, marke, false);
        }
        ad->PopClipRect();
    }
    ImGui::End();
    if (!offen) { g_se.offen = false; }
}

}  // namespace bhed::gui::karteneditor
