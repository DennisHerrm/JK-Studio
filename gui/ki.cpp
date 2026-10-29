// ki.cpp - die freiwillige KI-Anbindung im Programm (Stufe H1, siehe ki.h)
//
// Die Werkzeuge (include/bhed/ki.h) laufen hier - immer im Hauptfaden: der
// MCP-Server (stdio/HTTP) und der Chat stellen einen Auftrag in die Schlange
// und warten auf das Ergebnis; kiBild() arbeitet sie jedes Bild ab. Aendernde
// Auftraege zeigen vorher eine Liste ("Die KI moechte ...") mit Ausfuehren /
// Ablehnen; Datei-Befehle fragen IMMER. Ein Auftrag = ein Rueckgaengig-Schritt.
// Jede Aktion steht im KI-Protokoll. Der API-Schluessel und das MCP-Token
// liegen DPAPI-verschluesselt im Einstellungsordner und kommen nie ins
// Protokoll.
#include "ki.h"
#include "app_internal.h"
#include "backend.h"
#include "hilfe.h"
#include "karteneditor.h"
#include "platform.h"
#include "tour.h"
#include "bhed/hilfe.h"
#include "bhed/karte/befehle.h"
#include "bhed/ki.h"
#include "bhed/tour.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cwchar>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace bhed::gui {
namespace {

namespace ke = karteneditor;
using json::Wert;

struct Auftrag {
    std::string quelle;
    std::string name;
    Wert eingabe;
    std::promise<ki::Ergebnis> versprechen;
    std::atomic<bool> verlassen{false};
    bool erledigt = false;
    // aendernd: die geprueften Befehle
    std::vector<ki::Befehl> befehle;
    std::string zusammenfassung;
    // screenshot
    std::string fotoPfad;
    int fotoBilder = -1;
};

struct Protokoll {
    std::string zeit;
    std::string quelle;
    std::string was;
    std::string ergebnis;
};

struct ChatStand {
    std::mutex mx;
    bool laeuft = false;
    std::string antwort;
    std::string fehler;
    std::vector<std::string> werkzeuge;
    std::atomic<bool> abbrechen{false};
};

struct Zustand {
    std::mutex mx;
    std::deque<std::shared_ptr<Auftrag>> neu;
    std::deque<std::shared_ptr<Auftrag>> fragen;   // warten auf Ausfuehren/Ablehnen
    std::vector<std::shared_ptr<Auftrag>> fotos;
    std::vector<Protokoll> protokoll;
    bool protokollOffen = false;
    std::atomic<bool> getrennt{false};
    ki::HttpServer http;
    ki::StdioServer stdio;
    ki::McpSitzung httpSitzung;
    ki::McpSitzung stdioSitzung;
    std::mutex sitzungMx;
    bool gestartet = false;
    bool stdioGewuenscht = false;
    std::string httpFehler;
    std::shared_ptr<ChatStand> chat = std::make_shared<ChatStand>();
    char schluessel[256] = "";
    bool schluesselDa = false;
    bool schluesselGeprueft = false;
    std::string kopiert;

    Zustand() = default;
    Zustand(const Zustand&) = delete;
    Zustand& operator=(const Zustand&) = delete;
    // Beim Beenden: wartende Auftraege beantworten, sonst warteten die
    // Serverfaeden bis zu ihrer Zeitgrenze und das Programm hinge.
    ~Zustand() {
        getrennt = true;
        chat->abbrechen = true;
        const auto beenden = [](std::deque<std::shared_ptr<Auftrag>>& q) {
            for (const auto& a : q) {
                if (!a->erledigt) {
                    a->erledigt = true;
                    a->versprechen.set_value(ki::Ergebnis{true, "JK Studio is closing.", {}});
                }
            }
        };
        {
            std::lock_guard<std::mutex> l(mx);
            beenden(neu);
        }
        beenden(fragen);
        for (const auto& a : fotos) {
            if (!a->erledigt) {
                a->erledigt = true;
                a->versprechen.set_value(ki::Ergebnis{true, "JK Studio is closing.", {}});
            }
        }
        http.halte();
        stdio.halte();
    }
};

Zustand& z() {
    static Zustand s;
    return s;
}

std::string uhrzeit() {
    SYSTEMTIME t{};
    GetLocalTime(&t);
    char b[16];
    std::snprintf(b, sizeof(b), "%02u:%02u:%02u", static_cast<unsigned>(t.wHour), static_cast<unsigned>(t.wMinute), static_cast<unsigned>(t.wSecond));
    return b;
}

void protokolliere(const std::string& quelle, const std::string& was, const std::string& ergebnis) {
    Zustand& s = z();
    s.protokoll.push_back(Protokoll{uhrzeit(), quelle, was.substr(0, 300), ergebnis.substr(0, 200)});
    if (s.protokoll.size() > 500) { s.protokoll.erase(s.protokoll.begin()); }
    diag::detail("KI: " + quelle + " " + was.substr(0, 120) + " -> " + ergebnis.substr(0, 80));
}

std::filesystem::path datei(const char* name) {
    const std::string p = platform::settingsDirectory() + "\\" + name;
    return std::filesystem::path(std::u8string(p.begin(), p.end()));
}

bool leseGeheim(const char* name, std::string& klar) {
    std::ifstream f(datei(name), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream o;
    o << f.rdbuf();
    return ki::oeffnen(o.str(), klar);
}

bool schreibeGeheim(const char* name, const std::string& klar) {
    std::string blob;
    if (!ki::schuetzen(klar, blob)) { return false; }
    std::ofstream f(datei(name), std::ios::binary | std::ios::trunc);
    f.write(blob.data(), static_cast<std::streamsize>(blob.size()));
    return static_cast<bool>(f);
}

void loescheGeheim(const char* name) {
    std::error_code ec;
    std::filesystem::remove(datei(name), ec);
}

constexpr const char* kSchluesselDatei = "ki_schluessel.bin";
constexpr const char* kTokenDatei = "mcp_token.bin";

std::string mcpToken(bool neu) {
    std::string t;
    if (!neu && leseGeheim(kTokenDatei, t) && t.size() == 64) { return t; }
    t = ki::zufallsToken();
    if (t.empty() || !schreibeGeheim(kTokenDatei, t)) { return {}; }
    return t;
}

// --- Auftraege aus den Faeden ----------------------------------------------------------

ki::Ergebnis ausFaden(const std::string& quelle, const std::string& name, const Wert& eingabe) {
    Zustand& s = z();
    if (s.getrennt) { return {true, "The user disconnected the AI.", {}}; }
    auto a = std::make_shared<Auftrag>();
    a->quelle = quelle;
    a->name = name;
    a->eingabe = eingabe;
    std::future<ki::Ergebnis> f = a->versprechen.get_future();
    {
        std::lock_guard<std::mutex> l(s.mx);
        s.neu.push_back(a);
    }
    const ki::Werkzeug* w = ki::werkzeug(name);
    const auto zeit = (w != nullptr && w->wirkung == ki::Wirkung::Aendern) ? std::chrono::seconds(300) : std::chrono::seconds(15);
    if (f.wait_for(zeit) != std::future_status::ready) {
        a->verlassen = true;
        return {true, "Timed out (the user did not answer).", {}};
    }
    return f.get();
}

std::string mcpBearbeiten(ki::McpSitzung& sitzung, const std::string& quelle, const std::string& m) {
    Zustand& s = z();
    std::lock_guard<std::mutex> l(s.sitzungMx);
    return ki::mcpBearbeite(m, sitzung, [&](const std::string& n, const Wert& e) {
        return ausFaden(quelle + (sitzung.client.empty() ? std::string() : " (" + sitzung.client + ")"), n, e);
    });
}

// --- Die Werkzeuge (Hauptfaden) ------------------------------------------------------------

std::string basisname(const std::string& p) {
    const std::size_t s = p.find_last_of("\\/");
    return s == std::string::npos ? p : p.substr(s + 1);
}

ki::Ergebnis zustandLesen() {
    Wert o = Wert::alsObjekt();
    static const char* const modi[] = {"scripts", "map preview", "model", "map editor"};
    const int m = g_app->leftMode;
    o.setze("mode", Wert::alsZahl(m));
    o.setze("mode_name", Wert::alsText(m >= 0 && m < 4 ? modi[m] : "?"));
    Wert fenster = Wert::alsListe();
    for (const ImGuiWindow* w : GImGui->Windows) {
        if (!w->WasActive || w->Hidden || w->ParentWindow != nullptr || (w->Flags & ImGuiWindowFlags_Tooltip) != 0) { continue; }
        std::string n = w->Name;
        if (const std::size_t p = n.find("###"); p != std::string::npos) { n = n.substr(p + 3); }
        if (n.empty() || n.rfind("##", 0) == 0 || n.rfind("Debug", 0) == 0) { continue; }
        fenster.liste.push_back(Wert::alsText(n.substr(0, 60)));
        if (fenster.liste.size() >= 30) { break; }
    }
    o.setze("open_windows", fenster);
    const TourSicht t = tourSicht();
    if (t.aktiv) {
        Wert tw = Wert::alsObjekt();
        tw.setze("id", Wert::alsText(t.id));
        tw.setze("step", Wert::alsZahl(t.schritt + 1));
        tw.setze("steps", Wert::alsZahl(t.anzahl));
        o.setze("tutorial", tw);
    }
    Wert sk = Wert::alsObjekt();
    sk.setze("tabs", Wert::alsZahl(static_cast<double>(g_app->tabs.size())));
    sk.setze("file", Wert::alsText(basisname(g_app->path)));
    sk.setze("unsaved_changes", Wert::alsBool(g_app->doc.dirty()));
    o.setze("script", sk);
    const ke::Editor& e = ke::editor();
    Wert ka = Wert::alsObjekt();
    ka.setze("map", Wert::alsText(basisname(e.name)));
    int pinsel = 0;
    if (const auto welt = e.karte.welt()) {
        for (const auto& p : welt->kinder()) { pinsel += p->art() == karte::Primitiv::Art::Pinsel ? 1 : 0; }
    }
    ka.setze("brushes", Wert::alsZahl(pinsel));
    ka.setze("entities", Wert::alsZahl(static_cast<double>(e.karte.entitaeten().size())));
    ka.setze("selected", Wert::alsZahl(static_cast<double>(e.karte.auswahl.anzahl())));
    o.setze("map_editor", ka);
    return {false, ki::alsDaten("program state", o), {}};
}

ki::Ergebnis orteListen(const std::string& bereich) {
    Wert l = Wert::alsListe();
    for (const HilfeTreffer& t : hilfeOrte(bereich)) {
        Wert e = Wert::alsObjekt();
        e.setze("id", Wert::alsText(t.id));
        e.setze("where", Wert::alsText(t.pfad));
        l.liste.push_back(std::move(e));
        if (l.liste.size() >= 400) { break; }
    }
    return {false, ki::alsDaten("menus and settings", l), {}};
}

ki::Ergebnis suchen(const std::string& frage) {
    Wert l = Wert::alsListe();
    for (const HilfeTreffer& t : hilfeFrage(frage, 5)) {
        Wert e = Wert::alsObjekt();
        e.setze("id", Wert::alsText(t.id));
        e.setze("kind", Wert::alsText(t.art));
        e.setze("title", Wert::alsText(t.titel));
        if (!t.pfad.empty()) { e.setze("where", Wert::alsText(t.pfad)); }
        e.setze("text", Wert::alsText(t.text));
        if (!t.seite.empty()) { e.setze("guide_page", Wert::alsText(t.seite)); }
        l.liste.push_back(std::move(e));
    }
    return {false, ki::alsDaten("help index", l), {}};
}

// Gibt es die Befehle?
bool befehlePruefen(const std::vector<ki::Befehl>& bs, std::string& fehler) {
    for (const ki::Befehl& b : bs) {
        if (b.art == "map" && karte::befehle::finden(b.wert) == nullptr) {
            fehler = "unknown map editor command: " + b.wert;
            return false;
        }
    }
    return true;
}

// Ausfuehren - ein Auftrag = ein Rueckgaengig-Schritt.
ki::Ergebnis befehleAusfuehren(const Auftrag& a) {
    const std::size_t tiefeVorher = g_app->doc.undoDepth();
    ke::Editor& e = ke::editor();
    bool karte = false;
    for (const ki::Befehl& b : a.befehle) { karte = karte || b.art == "map"; }
    if (karte) { e.karte.undo.gruppeBeginnen(); }
    int ok = 0;
    std::string fehler;
    for (const ki::Befehl& b : a.befehle) {
        bool gut = true;
        if (b.art == "map") {
            gut = ke::ausfuehren(b.wert);
        } else if (b.art == "script") {
            tourAktionen(b.wert);
        } else if (b.art == "mode") {
            g_app->leftMode = b.wert[0] - '0';
        } else if (b.art == "view") {
            g_app->settings.flussAnsicht = b.wert[0] - '0';
        }
        if (gut) {
            ++ok;
        } else {
            fehler += (fehler.empty() ? "" : ", ") + b.wert;
        }
    }
    static const char* const kWas = "AI job";
    if (karte) { e.karte.undo.gruppeBeenden("AI: " + a.zusammenfassung.substr(0, 60)); }
    if (g_app->doc.undoDepth() > tiefeVorher + 1) { g_app->doc.fasseZusammen(tiefeVorher, kWas); }
    std::string t = std::to_string(ok) + " of " + std::to_string(a.befehle.size()) + " commands done (one undo step).";
    if (!fehler.empty()) { t += " Failed: " + fehler; }
    return {!fehler.empty() && ok == 0, t, {}};
}

void erledige(const std::shared_ptr<Auftrag>& a, ki::Ergebnis e) {
    if (a->erledigt) { return; }
    a->erledigt = true;
    protokolliere(a->quelle, a->name + (a->zusammenfassung.empty() ? std::string() : ": " + a->zusammenfassung), e.fehler ? "error: " + e.text : e.text);
    a->versprechen.set_value(std::move(e));
}

void bearbeite(const std::shared_ptr<Auftrag>& a) {
    Zustand& s = z();
    const std::string& n = a->name;
    const Wert& e = a->eingabe;
    if (n == "get_state") { return erledige(a, zustandLesen()); }
    if (n == "list_places") { return erledige(a, orteListen(e.textVon("area"))); }
    if (n == "search_help") { return erledige(a, suchen(e.textVon("query"))); }
    if (n == "show") {
        const bool ok = hilfeZeige(e.textVon("id"));
        return erledige(a, {!ok, ok ? "Shown to the user." : "Unknown id - use search_help or list_places.", {}});
    }
    if (n == "start_tour") {
        const int ab = static_cast<int>(e.zahlVon("step", 1.0)) - 1;
        const bool ok = tourStarten(e.textVon("tour"), ab);
        return erledige(a, {!ok, ok ? "The tutorial is running." : "Unknown tutorial or step.", {}});
    }
    if (n == "open_manual") {
        handbuchOeffnen(e.textVon("page").c_str());
        return erledige(a, {false, "Opened in the browser.", {}});
    }
    if (n == "screenshot") {
        char tmp[MAX_PATH] = "";
        GetTempPathA(MAX_PATH, tmp);
        a->fotoPfad = std::string(tmp) + "jkstudio_ki_" + std::to_string(GetCurrentProcessId()) + ".bmp";
        std::error_code ec;
        std::filesystem::remove(a->fotoPfad, ec);
        render::fotoAnfordern(a->fotoPfad);
        a->fotoBilder = 0;
        s.fotos.push_back(a);
        return;
    }
    if (n == "run_commands") {
        a->zusammenfassung = e.textVon("summary");
        for (const Wert& c : e.feld("commands")->liste) {
            ki::Befehl b;
            std::string f;
            if (!ki::zerlegeBefehl(c.text, b, &f)) { return erledige(a, {true, f, {}}); }
            a->befehle.push_back(b);
        }
        std::string f;
        if (!befehlePruefen(a->befehle, f)) { return erledige(a, {true, f, {}}); }
        if (ki::brauchtBestaetigung(a->befehle, g_app->settings.kiOhneNachfrage)) {
            s.fragen.push_back(a);
            return;
        }
        return erledige(a, befehleAusfuehren(*a));
    }
    erledige(a, {true, "unknown tool", {}});
}

void fotosPruefen() {
    Zustand& s = z();
    for (auto it = s.fotos.begin(); it != s.fotos.end();) {
        const std::shared_ptr<Auftrag> a = *it;
        ++a->fotoBilder;
        std::error_code ec;
        const auto groesse = std::filesystem::file_size(a->fotoPfad, ec);
        if (!ec && groesse > 54 && a->fotoBilder > 2) {
            std::ifstream f(a->fotoPfad, std::ios::binary);
            const std::string bmp((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            f.close();
            std::filesystem::remove(a->fotoPfad, ec);
            const auto u32 = [&bmp](std::size_t at) {
                std::uint32_t v = 0;
                for (std::size_t k = 0; k < 4; ++k) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(bmp[at + k])) << (8U * k); }
                return v;
            };
            const int w = static_cast<int>(u32(18));
            const int h = std::abs(static_cast<int>(u32(22)));
            const std::size_t daten = u32(10);
            std::string png;
            if (w > 0 && h > 0 && bmp.size() >= daten + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U) {
                png = ki::pngAusBgra(reinterpret_cast<const unsigned char*>(bmp.data() + daten), w, h, w * 4, 1024);
            }
            erledige(a, png.empty() ? ki::Ergebnis{true, "no picture", {}} : ki::Ergebnis{false, "", png});
            it = s.fotos.erase(it);
        } else if (a->fotoBilder > 120) {
            erledige(a, {true, "no picture", {}});
            it = s.fotos.erase(it);
        } else {
            ++it;
        }
    }
}

// --- Oberflaeche ----------------------------------------------------------------------------

void trennen() {
    Zustand& s = z();
    s.getrennt = true;
    s.http.halte();
    s.stdio.halte();
    s.chat->abbrechen = true;
    g_app->settings.mcpHttp = false;
    for (const auto& a : s.fragen) { erledige(a, {true, "The user disconnected the AI.", {}}); }
    s.fragen.clear();
    protokolliere("-", "disconnect", "AI disconnected by the user");
}

// Ein Befehl in Worten fuer die Rueckfrage ("Karten-Editor: Hohl machen"
// statt "map:CSGroom").
std::string befehlText(const ki::Befehl& b) {
    char t[400] = "";
    if (b.art == "map") {
        std::string name = b.wert;
        for (const karte::befehle::MenueEintrag& m : karte::befehle::menues()) {
            if (m.str != i18n::Str::Count && b.wert == m.befehl) {
                name.clear();
                for (const char c : std::string(ke::trK(m.str))) {
                    if (c != '&') { name += c; }
                }
                break;
            }
        }
        std::snprintf(t, sizeof(t), tr(Str::KiCmdMap), name.c_str());
    } else if (b.art == "script") {
        std::vector<tour::Aktion> as;
        if (tour::parseAktionen(b.wert, as, nullptr) && as.size() == 1) {
            const tour::Aktion& a = as[0];
            if (a.name == "neuerReiter") {
                std::snprintf(t, sizeof(t), "%s", tr(Str::KiCmdNewTab));
            } else if (a.name == "befehl") {
                std::snprintf(t, sizeof(t), tr(Str::KiCmdInsert), a.arg.c_str());
            } else if (a.name == "wert") {
                const std::size_t raute = a.arg.find('#');
                const std::size_t dp = a.arg.find(':', raute == std::string::npos ? 0 : raute);
                const std::size_t gl = a.arg.find('=', dp == std::string::npos ? 0 : dp);
                if (raute != std::string::npos && dp != std::string::npos && gl != std::string::npos) {
                    std::snprintf(t, sizeof(t), tr(Str::KiCmdValue), std::atoi(a.arg.substr(dp + 1, gl - dp - 1).c_str()) + 1,
                                  a.arg.substr(0, raute).c_str(), a.arg.substr(gl + 1).c_str());
                }
            } else if (a.name == "waehle") {
                std::snprintf(t, sizeof(t), tr(Str::KiCmdSelect), a.arg.c_str());
            } else if (a.name == "editor") {
                std::snprintf(t, sizeof(t), "%s", tr(Str::KiCmdEditor));
            } else if (a.name == "editorzu") {
                std::snprintf(t, sizeof(t), "%s", tr(Str::KiCmdEditorClose));
            }
        }
    } else if (b.art == "mode") {
        static const Str modi[] = {Str::ViewEvents, Str::ViewMap, Str::ViewModel, Str::ViewMapEditor};
        std::snprintf(t, sizeof(t), tr(Str::KiCmdMode), tr(modi[(b.wert[0] - '0') & 3]));
    } else if (b.art == "view") {
        static const Str ansichten[] = {Str::FlowClassic, Str::FlowGraph, Str::FlowTracks};
        std::snprintf(t, sizeof(t), tr(Str::KiCmdView), tr(ansichten[std::min(2, b.wert[0] - '0')]));
    }
    return t[0] != 0 ? std::string(t) : b.art + ":" + b.wert;
}

void frageZeichnen() {
    Zustand& s = z();
    while (!s.fragen.empty() && s.fragen.front()->verlassen) {
        erledige(s.fragen.front(), {true, "timed out", {}});
        s.fragen.pop_front();
    }
    if (s.fragen.empty()) { return; }
    const std::shared_ptr<Auftrag> a = s.fragen.front();
    const float fs = ImGui::GetFontSize();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    ImGui::SetNextWindowSize(ImVec2{fs * 28.0F, 0.0F}, ImGuiCond_Appearing);
    const std::string titel = std::string(tr(Str::KiConfirmTitle)) + "###kifrage";
    if (ImGui::Begin(titel.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextDisabled("%s", a->quelle.c_str());
        ImGui::PushTextWrapPos(fs * 27.0F);
        ImGui::TextUnformatted(tr(Str::KiConfirmIntro));
        // Die Zusammenfassung stammt von der KI: als Zitat, nicht als Programmtext.
        ImGui::TextColored(ImVec4{0.6F, 0.8F, 1.0F, 1.0F}, "\"%s\"", a->zusammenfassung.c_str());
        ImGui::PopTextWrapPos();
        for (const ki::Befehl& b : a->befehle) {
            const std::string text = befehlText(b);
            if (b.datei) {
                ImGui::TextColored(ImVec4{1.0F, 0.6F, 0.2F, 1.0F}, "! %s  (%s)", text.c_str(), tr(Str::KiCmdFile));
            } else {
                ImGui::BulletText("%s", text.c_str());
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s:%s", b.art.c_str(), b.wert.c_str()); }
        }
        if (ImGui::Button(tr(Str::KiRun))) {
            erledige(a, befehleAusfuehren(*a));
            s.fragen.pop_front();
        }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::KiReject))) {
            erledige(a, {true, "Rejected by the user.", {}});
            s.fragen.pop_front();
        }
    }
    ImGui::End();
}

void protokollZeichnen() {
    Zustand& s = z();
    if (!s.protokollOffen) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 34.0F, fs * 18.0F}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(tr(Str::KiLog)) + "###kiprotokoll";
    if (ImGui::Begin(titel.c_str(), &s.protokollOffen)) {
        if (s.protokoll.empty()) { ImGui::TextDisabled("%s", tr(Str::KiLogEmpty)); }
        for (const Protokoll& p : s.protokoll) {
            ImGui::TextDisabled("%s", p.zeit.c_str());
            ImGui::SameLine();
            ImGui::TextWrapped("%s | %s -> %s", p.quelle.c_str(), p.was.c_str(), p.ergebnis.c_str());
        }
    }
    ImGui::End();
}

bool verbunden() {
    Zustand& s = z();
    bool chatLaeuft = false;
    {
        std::lock_guard<std::mutex> l(s.chat->mx);
        chatLaeuft = s.chat->laeuft;
    }
    return s.http.laeuft() || s.stdio.laeuft() || chatLaeuft;
}

void symbolZeichnen() {
    Zustand& s = z();
    if (!verbunden()) { return; }
    const float fs = ImGui::GetFontSize();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x - fs * 0.5F, vp->WorkPos.y + fs * 2.2F}, ImGuiCond_Always, ImVec2{1.0F, 0.0F});
    ImGui::SetNextWindowBgAlpha(0.85F);
    if (ImGui::Begin("###kiverbunden", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav)) {
        ImGui::TextColored(ImVec4{0.3F, 0.9F, 0.4F, 1.0F}, "%s", tr(Str::KiConnected));
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::KiLog))) { s.protokollOffen = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::KiDisconnect))) { trennen(); }
    }
    ImGui::End();
}

void serverAbgleichen() {
    Zustand& s = z();
    if (!s.gestartet) {
        s.gestartet = true;
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        for (int i = 1; argv != nullptr && i < argc; ++i) {
            if (std::wcscmp(argv[i], L"--mcp") == 0) { s.stdioGewuenscht = true; }
        }
        if (argv != nullptr) { LocalFree(argv); }
        if (s.stdioGewuenscht && std::getenv("BHED_EDITORTEST") == nullptr) {
            const bool ok = s.stdio.starte([](const std::string& m) { return mcpBearbeiten(z().stdioSitzung, "MCP stdio", m); });
            protokolliere("MCP stdio", "start", ok ? "listening on stdin/stdout" : "no stdin");
        }
    }
    if (s.getrennt) { return; }
    const bool soll = g_app->settings.mcpHttp;
    if (soll && !s.http.laeuft() && s.httpFehler.empty()) {
        const std::string t = mcpToken(false);
        std::string f;
        if (!s.http.starte(g_app->settings.mcpPort, t, [](const std::string& m) { return mcpBearbeiten(z().httpSitzung, "MCP HTTP", m); }, &f)) {
            s.httpFehler = f.empty() ? std::string("?") : f;
            protokolliere("MCP HTTP", "start", "failed: " + s.httpFehler);
        } else {
            protokolliere("MCP HTTP", "start", "127.0.0.1:" + std::to_string(g_app->settings.mcpPort));
        }
    } else if (!soll && s.http.laeuft()) {
        s.http.halte();
        protokolliere("MCP HTTP", "stop", "");
    }
    if (!soll) { s.httpFehler.clear(); }
}

std::string exePfad() {
    wchar_t w[MAX_PATH] = L"";
    GetModuleFileNameW(nullptr, w, MAX_PATH);
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<std::size_t>(n > 0 ? n - 1 : 0), '\0');
    if (n > 0) { WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr); }
    return s;
}

// Fertige Einrichtungen fuer die bekannten MCP-Apps.
std::string einrichtung(int app, bool http) {
    const std::string exe = json::zeichenkette(exePfad());
    const std::string url = "http://127.0.0.1:" + std::to_string(g_app->settings.mcpPort) + "/mcp";
    const std::string tok = http ? mcpToken(false) : std::string();
    const std::string stdioJson = "{\n  \"mcpServers\": {\n    \"jkstudio\": {\n      \"command\": " + exe + ",\n      \"args\": [\"--mcp\"]\n    }\n  }\n}\n";
    switch (app) {
        case 0:   // Claude Desktop: claude_desktop_config.json
            return stdioJson;
        case 1:   // Claude Code
            return http ? "claude mcp add --transport http jkstudio " + url + " --header \"Authorization: Bearer " + tok + "\"\n"
                        : "claude mcp add jkstudio -- " + exe + " --mcp\n";
        case 2:   // Cursor: .cursor/mcp.json
            return http ? "{\n  \"mcpServers\": {\n    \"jkstudio\": {\n      \"url\": \"" + url + "\",\n      \"headers\": { \"Authorization\": \"Bearer " + tok +
                              "\" }\n    }\n  }\n}\n"
                        : stdioJson;
        default:  // VS Code: .vscode/mcp.json
            return http ? "{\n  \"servers\": {\n    \"jkstudio\": {\n      \"type\": \"http\",\n      \"url\": \"" + url +
                              "\",\n      \"headers\": { \"Authorization\": \"Bearer " + tok + "\" }\n    }\n  }\n}\n"
                        : "{\n  \"servers\": {\n    \"jkstudio\": {\n      \"type\": \"stdio\",\n      \"command\": " + exe + ",\n      \"args\": [\"--mcp\"]\n    }\n  }\n}\n";
    }
}

void chatStarten(const std::string& frage) {
    Zustand& s = z();
    std::string schluessel;
    if (!leseGeheim(kSchluesselDatei, schluessel) || schluessel.empty()) { return; }
    auto stand = std::make_shared<ChatStand>();
    stand->laeuft = true;
    s.chat = stand;
    s.getrennt = false;
    // Kontext: die passenden Eintraege des Offline-Index (als Daten markiert).
    Wert auszuege = Wert::alsListe();
    for (const HilfeTreffer& t : hilfeFrage(frage, 5)) {
        Wert e = Wert::alsObjekt();
        e.setze("id", Wert::alsText(t.id));
        e.setze("title", Wert::alsText(t.titel));
        e.setze("text", Wert::alsText(t.text));
        if (!t.pfad.empty()) { e.setze("where", Wert::alsText(t.pfad)); }
        auszuege.liste.push_back(std::move(e));
    }
    const std::string system = ki::systemText(json::schreibe(auszuege));
    const std::string modell = g_app->settings.kiModell;
    protokolliere("Chat", "ask " + modell, frage);
    std::thread([stand, frage, system, modell, schluessel]() mutable {
        const ki::Senden senden = [&](const std::string& koerper, int& status, std::string& antwort, std::string& fehler) {
            // Nur api.anthropic.com, nur TLS (WinHTTP prueft das Zertifikat).
            return ki::httpPost(ki::kApiHost, 443, true, ki::kApiPfad, ki::chatKopf(modell, schluessel), koerper, status, antwort, fehler, 90000);
        };
        const ki::ChatErgebnis e = ki::chat(modell, system, frage, senden,
                                            [](const std::string& n, const Wert& w) { return ausFaden("Chat", n, w); },
                                            [stand] { return stand->abbrechen.load(); });
        SecureZeroMemory(schluessel.data(), schluessel.size());
        std::lock_guard<std::mutex> l(stand->mx);
        stand->antwort = e.text;
        stand->fehler = e.fehler;
        stand->werkzeuge = e.werkzeuge;
        stand->laeuft = false;
    }).detach();
}

}  // namespace

// --- Oeffentlich -----------------------------------------------------------------------------

void kiAbschnitt(const char* frage, float breite) {
    Zustand& s = z();
    if (!s.schluesselGeprueft) {
        std::string k;
        s.schluesselDa = leseGeheim(kSchluesselDatei, k) && !k.empty();
        SecureZeroMemory(k.data(), k.size());
        s.schluesselGeprueft = true;
    }
    const float fs = ImGui::GetFontSize();
    if (ImGui::CollapsingHeader(tr(Str::KiSection))) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + breite);
        ImGui::TextDisabled("%s", tr(Str::KiExplain));
        ImGui::PopTextWrapPos();
        if (ImGui::SmallButton(tr(Str::KiSafety))) { handbuchOeffnen("helfer.html#sicher"); }
        ImGui::Checkbox(tr(Str::KiEnable), &g_app->settings.kiChat);
        if (g_app->settings.kiChat) {
            ImGui::TextDisabled("%s", tr(s.schluesselDa ? Str::KiKeyStored : Str::KiKeyNone));
            ImGui::SetNextItemWidth(fs * 14.0F);
            ImGui::InputTextWithHint("##kischluessel", tr(Str::KiKey), s.schluessel, sizeof(s.schluessel), ImGuiInputTextFlags_Password);
            ImGui::SameLine();
            if (ImGui::Button(tr(Str::KiKeySave)) && s.schluessel[0] != '\0') {
                s.schluesselDa = schreibeGeheim(kSchluesselDatei, s.schluessel);
                SecureZeroMemory(s.schluessel, sizeof(s.schluessel));
                protokolliere("Chat", "key", s.schluesselDa ? "stored (encrypted)" : "could not store");
            }
            if (s.schluesselDa) {
                ImGui::SameLine();
                if (ImGui::Button(tr(Str::KiKeyDelete))) {
                    loescheGeheim(kSchluesselDatei);
                    s.schluesselDa = false;
                    protokolliere("Chat", "key", "deleted");
                }
            }
            ImGui::SetNextItemWidth(fs * 14.0F);
            if (ImGui::BeginCombo(tr(Str::KiModel), g_app->settings.kiModell.c_str())) {
                for (std::size_t i = 0; i < ki::chatModelle().size(); ++i) {
                    char t[96];
                    if (i == 1) {
                        std::snprintf(t, sizeof(t), tr(Str::KiModelCheap), ki::chatModelle()[i].c_str());
                    } else {
                        std::snprintf(t, sizeof(t), "%s", ki::chatModelle()[i].c_str());
                    }
                    if (ImGui::Selectable(t, g_app->settings.kiModell == ki::chatModelle()[i])) { g_app->settings.kiModell = ki::chatModelle()[i]; }
                }
                ImGui::EndCombo();
            }
            ImGui::Checkbox(tr(Str::KiAutoRun), &g_app->settings.kiOhneNachfrage);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::KiAutoRunTip)); }
            ChatStand& c = *s.chat;
            bool laeuft = false;
            std::string antwort;
            std::string fehler;
            {
                std::lock_guard<std::mutex> l(c.mx);
                laeuft = c.laeuft;
                antwort = c.antwort;
                fehler = c.fehler;
            }
            if (laeuft) {
                ImGui::TextDisabled("%s", tr(Str::KiThinking));
                ImGui::SameLine();
                if (ImGui::Button(tr(Str::KiStop))) { c.abbrechen = true; }
            } else {
                ImGui::BeginDisabled(!s.schluesselDa || frage == nullptr || frage[0] == '\0');
                if (ImGui::Button(tr(Str::KiAsk))) { chatStarten(frage); }
                ImGui::EndDisabled();
            }
            if (!fehler.empty()) { ImGui::TextColored(ImVec4{1.0F, 0.5F, 0.4F, 1.0F}, tr(Str::KiError), fehler.c_str()); }
            if (!antwort.empty()) {
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + breite);
                ImGui::TextUnformatted(antwort.c_str());
                ImGui::PopTextWrapPos();
            }
            if (ImGui::SmallButton(tr(Str::KiLog))) { s.protokollOffen = true; }
        }
    }
    if (ImGui::CollapsingHeader(tr(Str::McpSection))) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + breite);
        ImGui::TextDisabled("%s", tr(Str::McpExplain));
        ImGui::PopTextWrapPos();
        static const char* const apps[] = {"Claude Desktop", "Claude Code", "Cursor", "VS Code"};
        for (int i = 0; i < 4; ++i) {
            char t[96];
            std::snprintf(t, sizeof(t), tr(Str::McpCopy), apps[i]);
            if (ImGui::SmallButton((std::string(t) + "##stdio" + std::to_string(i)).c_str())) {
                ImGui::SetClipboardText(einrichtung(i, false).c_str());
                s.kopiert = tr(Str::McpCopied);
            }
        }
        ImGui::Separator();
        if (ImGui::Checkbox(tr(Str::McpHttp), &g_app->settings.mcpHttp) && g_app->settings.mcpHttp) { s.getrennt = false; }
        ImGui::SetNextItemWidth(fs * 6.0F);
        int port = g_app->settings.mcpPort;
        if (ImGui::InputInt(tr(Str::McpPort), &port, 0) && port >= 1024 && port <= 65535 && !s.http.laeuft()) { g_app->settings.mcpPort = port; }
        if (s.http.laeuft()) { ImGui::TextDisabled(tr(Str::McpRunning), s.http.port()); }
        if (!s.httpFehler.empty()) { ImGui::TextColored(ImVec4{1.0F, 0.5F, 0.4F, 1.0F}, tr(Str::McpFailed), s.httpFehler.c_str()); }
        if (g_app->settings.mcpHttp) {
            for (int i = 1; i < 4; ++i) {
                char t[96];
                std::snprintf(t, sizeof(t), tr(Str::McpCopy), apps[i]);
                if (ImGui::SmallButton((std::string(t) + " (HTTP)##http" + std::to_string(i)).c_str())) {
                    ImGui::SetClipboardText(einrichtung(i, true).c_str());
                    s.kopiert = tr(Str::McpCopied);
                }
            }
            if (ImGui::SmallButton(tr(Str::McpNewToken))) {
                (void)mcpToken(true);
                s.http.halte();   // startet im naechsten Bild mit dem neuen Token
                protokolliere("MCP HTTP", "new token", "old token no longer works");
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::McpNewTokenTip)); }
        }
        if (!s.kopiert.empty()) { ImGui::TextDisabled("%s", s.kopiert.c_str()); }
    }
}

void kiBild() {
    if (g_app == nullptr) { return; }
    Zustand& s = z();
    serverAbgleichen();
    std::deque<std::shared_ptr<Auftrag>> neu;
    {
        std::lock_guard<std::mutex> l(s.mx);
        neu.swap(s.neu);
    }
    for (const auto& a : neu) {
        if (a->verlassen) { continue; }
        bearbeite(a);
    }
    fotosPruefen();
    frageZeichnen();
    protokollZeichnen();
    symbolZeichnen();
}

void kiStart(bool mcpStdio) { z().stdioGewuenscht = mcpStdio; }

KiSicht kiSicht() {
    Zustand& s = z();
    KiSicht v;
    v.httpLaeuft = s.http.laeuft();
    v.stdioLaeuft = s.stdio.laeuft();
    v.fragen = static_cast<int>(s.fragen.size());
    v.protokoll = static_cast<int>(s.protokoll.size());
    return v;
}

std::string kiTestToken() { return std::getenv("BHED_EDITORTEST") != nullptr ? mcpToken(false) : std::string(); }

void kiEnde() {
    Zustand& s = z();
    s.getrennt = true;
    s.chat->abbrechen = true;
    s.http.halte();
    s.stdio.halte();
}

}  // namespace bhed::gui
