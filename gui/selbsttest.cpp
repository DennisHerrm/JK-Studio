// selbsttest.cpp - behaved prueft seinen Ereigniseditor und seine Knoepfe
//
// Aufruf:  set BHED_EDITORTEST=1  und behaved.exe starten.
//
// Ohne die Umgebungsvariable tut diese Datei nichts ausser einer Abfrage
// beim ersten Bild.
//
// Zwei Teile, nacheinander, danach beendet sich das Programm selbst:
//
//   1. RUHE    - der Ereigniseditor wird fuer eine Reihe von Befehlen
//                geoeffnet, und jedes sichtbare Bild, in dem sich Stelle
//                oder Groesse aendert, zaehlt als Ruck (rc568).
//   2. KNOEPFE - der Test klickt mit simulierter Maus auf Knoepfe, tippt in
//                Felder und prueft danach das SKRIPT: steht dort, was der
//                Knopf verspricht? Gefunden wird jeder Knopf ueber seine
//                Beschriftung und an der Stelle geklickt, an der ImGui ihn
//                gezeichnet hat. Ein Knopf, der abgeschnitten oder verdeckt
//                ist, faellt damit ebenfalls auf.
//
// Alles steht im Detailprotokoll: `Selbsttest ...` und `Knopftest ...`.
//
// Woher der Test die Knoepfe kennt: ImGui meldet mit
// IMGUI_ENABLE_TEST_ENGINE (gui/imgui_config.h) jedes Element mit Kennung,
// Beschriftung und Rechteck an die Haken - aber nur, solange
// `TestEngineHookItems` gesetzt ist. Seit rc601 (Paket tour) stehen Haken und
// Register in gui/elemente.{h,cpp}; der Test meldet dort jedes Bild die Stufe
// "Voll" an, eine Tutorial-Tour nur ihre Beschriftungen ("Gefiltert").
//
// Was NICHT geklickt wird: alles, was einen Windows-Dateidialog oeffnet
// oder Dateien schreibt (Open, Append, Save, Save As, Save all, Export,
// Backup, Restore, Compile). Ein Dateidialog haelt die Nachrichtenschleife
// an, und der Test kaeme nie weiter.

#include "update.h"
#include "app_internal.h"
#include "elemente.h"   // rc601 tour: das Element-Register
#include "tour.h"       // rc601 tour: Tutorials im Programm (tourSicht)
#include "backend.h"
#include "karte_gpu.h"
#include "karte_bau_plattform.h"   // Stufe 3 Punkt 4: Port 39000 belegen
#include "karte_echtlicht.h"         // Stufe 7: Echtzeit-Licht
#include "platform.h"
#include "wache.h"
#include "gpumap.h"
#include "bhed/changelog.h"
#include "bhed/entdefs.h"
#include "bhed/navlauf.h"
#include "bhed/regelwerk.h"
#include "bhed/saberkontakt.h"
#include "bhed/roffanim.h"
#include "bhed/kartenpruefung.h"   // rc602
#include "karteneditor.h"           // Stufe 1b: der Karten-Editor
#include "karte_qtdialog.h"          // Stufe 4: Bausteine der Plugin-Dialoge
#include "bhed/karte/optimierer.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/kollision.h"
#include "bhed/karte/ebenen.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/modellbrowser.h"
#include "bhed/karte/region.h"
#include "bhed/karte/filter.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/lichtschaetzung.h"   // Stufe 9c: Lichtmessung auf dekompilierten Karten

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "bhed/utf8pfad.h"

namespace bhed::gui {

namespace {

// --- Was ImGui in diesem Bild gezeichnet hat -----------------------------
//
// Paket tour (rc601): das Register steht jetzt in gui/elemente.{h,cpp} - die
// Tutorials im Programm brauchen es auch ausserhalb des Selbsttests. Die
// Namen hier bleiben als Weiterleitungen, damit jede Schrittliste unveraendert
// weiterlaeuft (g_elemente, finde, findeKennung, ganzSichtbar ...).
using Element = elemente::Element;

std::vector<Element>& g_elemente = elemente::alle();

// rc601 tour: nicht leer = der Selbsttest meldet fuer das naechste Bild nur
// diese Beschriftungen an (Stufe Gefiltert) statt alles - so prueft er die
// Stufe, mit der die Tutorials im normalen Betrieb laufen.
std::vector<std::string> g_registerFilter;

// Die simulierte Mausstelle. Sie wird nicht nur einmal gemeldet, sondern in
// JEDEM Bild noch einmal, unmittelbar nach der Win32-Anbindung (siehe
// selbsttestVorBild). Grund: hat das Fenster den Fokus und steht die echte
// Maus nicht darueber, meldet ImGui_ImplWin32_UpdateMouseData jedes Bild die
// echte Stelle aus GetCursorPos - und die kam nach unserer und gewann. Ein
// Lauf mit Fokus hatte so 82 Fehlklicks ("unter der Maus liegt kein
// Fenster"), einer ohne Fokus keinen einzigen.
bool g_mausAn = false;
ImVec2 g_maus{};

void setzeMaus(ImVec2 p) {
    g_mausAn = true;
    g_maus = p;
    ImGui::GetIO().AddMousePosEvent(p.x, p.y);
}

// Die Beschriftung ohne den "##..."-Teil - so, wie sie auf dem Knopf steht.
// "###" haengt eine feste Kennung an, "##" eine versteckte.
std::string sichtbarerText(const char* label) { return elemente::sichtbarerText(label); }

}  // namespace

// Die ImGui-Haken rufen jetzt elemente::merkeElement/merkeText (gui/elemente.cpp,
// dort auch die Behebung fuer Reiter: Beschriftung vor dem ItemAdd). app.cpp
// meldet Klapplisten weiter ueber diese beiden Namen.
void selbsttestMerkeElement(ImGuiContext* ctx, ImGuiID id, const ImRect& bb) { elemente::merkeElement(ctx, id, bb); }

void selbsttestMerkeText(ImGuiContext* ctx, ImGuiID id, const char* label) { elemente::merkeText(ctx, id, label, 0); }

namespace {

// --- Kleine Werkzeuge -----------------------------------------------------

// Findet das `nte` Element mit dieser Beschriftung. Verglichen wird mit dem
// ganzen Text ("##v" findet Eingabefelder) und mit dem sichtbaren ("Ok").
// `fenster` ist ein Teil des Fensternamens, leer = egal. `nachbar`: nur
// Treffer auf derselben Zeile ("+" gibt es zweimal: den Reiterknopf oben und
// "Expand all" neben "-"). Der Kern steht in gui/elemente.cpp.
const Element* finde(const std::string& label, const std::string& fenster,
                     int nte, bool praefix, const std::string& nachbar = "") {
    return elemente::finde(label, fenster, nte, praefix, nachbar);
}

[[maybe_unused]] bool aufZeileVon(const Element& e, const std::string& nachbar,
                                  const std::string& fenster) {
    return elemente::aufZeileMit(e, nachbar, fenster);
}

const Element* findeKennung(ImGuiID id) { return elemente::findeKennung(id); }

// Liegt das Element ganz im sichtbaren Bereich (Spiel: der halbe
// Zeilenabstand, siehe gui/elemente.cpp)?
bool ganzSichtbar(const Element& e) { return elemente::ganzImBild(e); }

// "aktiv: <Beschriftung>, unter der Maus: <Beschriftung>" - fuer Meldungen,
// wenn ein Klick nicht ankam.
std::string wasImGuiSieht() {
    ImGuiContext& g = *GImGui;
    auto name = [](ImGuiID id) -> std::string {
        if (id == 0) { return "-"; }
        const Element* e = findeKennung(id);
        if (e == nullptr) { return "?"; }
        return e->label.empty() ? "(ohne Namen) in " + e->innen : e->label + " in " + e->innen;
    };
    return "aktiv: " + name(g.ActiveId) + ", unter der Maus: " + name(g.HoveredId) +
           ", Fenster " + (g.HoveredWindow != nullptr ? g.HoveredWindow->Name : "-");
}

// Eine Datei schreiben (spew in app.cpp liegt im namenlosen Namensraum).
bool spewDatei(const std::string& pfad, const std::string& text) {
    std::FILE* f = std::fopen(pfad.c_str(), "wb");
    if (f == nullptr) { return false; }
    const bool ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
    std::fclose(f);
    return ok;
}

int g_ok = 0;
int g_fehler = 0;

void meldeOk(const std::string& text) {
    ++g_ok;
    diag::detail("Knopftest OK: " + text);
}

void meldeFehler(const std::string& text) {
    ++g_fehler;
    diag::detail("Knopftest FEHLER: " + text);
}

void pruefeWahr(bool bedingung, const std::string& text) {
    if (bedingung) { meldeOk(text); } else { meldeFehler(text); }
}

std::string skriptText() { return writeScript(g_app->doc.script()); }

// Weg zum `nte` Befehl dieses Namens auf oberster Ebene.
bool wegZu(const std::string& befehl, int nte, Path& weg) {
    int gefunden = 0;
    const std::vector<Node>& n = g_app->doc.script().nodes;
    for (std::size_t i = 0; i < n.size(); ++i) {
        if (n[i].kind == Node::Kind::Command && n[i].name == befehl) {
            if (gefunden == nte) {
                weg = Path{i};
                return true;
            }
            ++gefunden;
        }
    }
    return false;
}

const Node* knoten(const std::string& befehl, int nte) {
    Path weg;
    return wegZu(befehl, nte, weg) ? nodeAt(g_app->doc.script(), weg) : nullptr;
}

int anzahl(const std::string& befehl) {
    int z = 0;
    for (const Node& n : g_app->doc.script().nodes) {
        if (n.kind == Node::Kind::Command && n.name == befehl) { ++z; }
    }
    return z;
}

std::string argText(const std::string& befehl, int nte, std::size_t feld) {
    const Node* k = knoten(befehl, nte);
    if (k == nullptr || feld >= k->args.size()) { return "<fehlt>"; }
    return k->args[feld].text;
}

std::string wert(std::size_t feld) {
    return feld < g_app->editorValues.size() ? g_app->editorValues[feld]
                                             : std::string("<fehlt>");
}

bool istAusdruck(std::size_t feld) {
    return feld < g_app->editorIsExpr.size() && g_app->editorIsExpr[feld] != 0;
}

// --- Schritte -------------------------------------------------------------
//
// Ein Schritt laeuft ueber mehrere Bilder: `tun(b)` bekommt die Nummer des
// Bildes innerhalb des Schritts und meldet mit true, dass er fertig ist.
// Mausereignisse brauchen ein Bild, bis ImGui sie sieht - deshalb Stelle,
// Druecken und Loslassen in getrennten Bildern.

struct Schritt {
    std::string text;
    std::function<bool(int)> tun;
    // rc601: Schritte mit eigener Zeitgrenze in Sekunden (das Update laedt
    // seit rc601 ein Zip von ~110 MB) - fuer sie gilt die Grenze von 4000
    // Bildern je Schritt nicht.
    bool eigeneZeitgrenze = false;
};

// Schritte hinter dem laufenden einhaengen (Definition weiter unten).
void schritteEinfuegen(std::vector<Schritt> neu);
// Warten, bis eine Bedingung gilt (Definition weiter unten).
Schritt warteBis(const std::string& was, std::function<bool()> bedingung, int maxBilder);
bool leseVektor(const std::string& text, float out[3]);
// Die Frage mit Nein beantworten (Definition weiter unten).
Schritt antworteNein();

// "Yes/No query" des Benutzers - der Test setzt es an und stellt es am Ende
// zurueck, weil die Einstellungen beim Beenden gespeichert werden.
bool g_queryVorher = true;
// Die Skriptansicht des Benutzers (Classic/Graph/Spuren) - der Test braucht
// Classic und gibt sie am Ende zurueck (rc599: mit Graph in der behaved.cfg
// fanden die Klicks im Baum nichts, 443 Fehler im vollen Lauf).
int g_flussVorher = -1;

Schritt klick(const std::string& label, const std::string& fenster = "",
              int nte = 0, bool praefix = false,
              const std::string& nachbar = "", int maustaste = 0,
              float griffAnteil = 0.5F) {
    auto ziel = std::make_shared<ImVec2>();
    auto kennung = std::make_shared<ImGuiID>(0);
    const std::string was = "Klick \"" + label + "\"" +
                            (nte > 0 ? " #" + std::to_string(nte) : std::string());
    return {was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    const Element* e =
                        finde(label, fenster, nte, praefix, nachbar);
                    if (e == nullptr) {
                        meldeFehler(was + ": nicht gefunden");
                        return true;
                    }
                    // Mehrdeutige Beschriftung? Dann alle Treffer zeigen -
                    // sonst klickt der Test still den falschen.
                    if (label.rfind("##", 0) != 0 && nte == 0 &&
                        finde(label, fenster, 1, praefix, nachbar) != nullptr) {
                        for (int k = 0;; ++k) {
                            const Element* x =
                                finde(label, fenster, k, praefix, nachbar);
                            if (x == nullptr) { break; }
                            char z[220];
                            std::snprintf(z, sizeof(z),
                                          "Knopftest: \"%s\" #%d in \"%s\" bei %.0f/%.0f",
                                          x->label.c_str(), k, x->fenster.c_str(),
                                          static_cast<double>(x->rect.Min.x),
                                          static_cast<double>(x->rect.Min.y));
                            diag::detail(z);
                        }
                    }
                    if (!ganzSichtbar(*e)) {
                        char z[200];
                        std::snprintf(z, sizeof(z),
                                      ": nicht ganz sichtbar (%.0f..%.0f, "
                                      "sichtbar %.0f..%.0f)",
                                      static_cast<double>(e->rect.Min.x),
                                      static_cast<double>(e->rect.Max.x),
                                      static_cast<double>(e->clip.Min.x),
                                      static_cast<double>(e->clip.Max.x));
                        meldeFehler(was + z);
                    }
                    // Waagerecht wahlweise nicht mittig: bei Eingabefeldern
                    // mit Beschriftung rechts daneben liegt die Mitte auf
                    // der Beschriftung, und die nimmt keinen Klick.
                    *ziel = ImVec2{e->rect.Min.x + e->rect.GetWidth() * griffAnteil,
                                   e->rect.GetCenter().y};
                    *kennung = e->id;
                    setzeMaus(*ziel);
                    return false;
                }
                if (b == 1) { io.AddMouseButtonEvent(maustaste, true); return false; }
                if (b == 2) {
                    // Jetzt ist die Maustaste unten angekommen: das Element
                    // muss aktiv sein. Sonst traf der Klick etwas anderes.
                    // Rechtsklick aktiviert nichts - dort zaehlt, dass die
                    // Maus ueber dem Element steht.
                    ImGuiContext& g = *GImGui;
                    const bool getroffen = (maustaste == 0)
                                               ? g.ActiveId == *kennung
                                               : g.HoveredIdPreviousFrame == *kennung;
                    if (!getroffen) {
                        meldeFehler(was + ": Klick kam nicht an (" + wasImGuiSieht() + ")");
                    }
                    io.AddMouseButtonEvent(maustaste, false);
                    return false;
                }
                return b >= 5;
            }};
}

// --- Ziehen mit der Maus -----------------------------------------------
//
// Packen, ueber die Ziehschwelle bewegen, zum Ziel fahren, dort ein paar
// Bilder stehen (das Ziel muss den Zug erst annehmen), loslassen.
Schritt ziehe(const std::string& von, const std::string& vonFenster,
              const std::string& nach, const std::string& nachFenster,
              int vonNte = 0, int nachNte = 0, float griffAnteil = 0.5F,
              bool ablegen = true, ImVec2 zielVersatz = ImVec2{0.0F, 0.0F},
              bool vonPraefix = false) {
    auto start = std::make_shared<ImVec2>();
    const std::string was = "Ziehe \"" + von + "\" (" + vonFenster + ") auf \"" +
                            nach + "\" (" + nachFenster + ")";
    return {was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    const Element* e = finde(von, vonFenster, vonNte, vonPraefix);
                    if (e == nullptr) {
                        meldeFehler(was + ": Quelle nicht gefunden");
                        return true;
                    }
                    *start = ImVec2{e->rect.Min.x + e->rect.GetWidth() * griffAnteil,
                                    e->rect.GetCenter().y};
                    setzeMaus(*start);
                    return false;
                }
                if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                if (b == 2) {
                    diag::detail("Knopftest: Zug gepackt - " + wasImGuiSieht());
                }
                if (b >= 2 && b <= 6) {
                    // In kleinen Schritten ueber die Ziehschwelle.
                    setzeMaus(ImVec2{start->x + 4.0F * static_cast<float>(b - 1),
                                     start->y + 2.0F * static_cast<float>(b - 1)});
                    return false;
                }
                if (b >= 7 && b <= 12) {
                    const Element* z = finde(nach, nachFenster, nachNte, false);
                    if (z == nullptr) {
                        if (b == 12) {
                            meldeFehler(was + ": Ziel nicht gefunden");
                            io.AddMouseButtonEvent(0, false);
                            return true;
                        }
                        return false;
                    }
                    const ImVec2 m = z->rect.GetCenter();
                    setzeMaus(ImVec2{m.x + zielVersatz.x, m.y + zielVersatz.y});
                    return false;
                }
                if (b == 13) {
                    // Ein Teiler ist kein Ziehen-und-Ablegen - dort gibt es
                    // keine Nutzlast, nur das gepackte Element.
                    if (ablegen && !GImGui->DragDropActive) {
                        meldeFehler(was + ": kein Zug aktiv vor dem Loslassen (" +
                                    wasImGuiSieht() + ")");
                    } else {
                        diag::detail(std::string("Knopftest: Nutzlast \"") +
                                     GImGui->DragDropPayload.DataType + "\", " +
                                     wasImGuiSieht());
                    }
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                return b >= 17;
            }};
}

// --- Ruhe des Hauptfensters -----------------------------------------------
//
// `bilder` lang jedes Bild die Rechtecke aller Elemente im Hauptfenster
// vergleichen. Nach `schonung` Bildern darf sich keines mehr bewegen.
Schritt stabil(const std::string& wann, int bilder = 60, int schonung = 3) {
    struct Stand {
        std::vector<std::pair<ImGuiID, ImRect>> vorher;
        int unruhig = 0;
        std::string erster;
    };
    auto st = std::make_shared<Stand>();
    return {"Ruhe " + wann, [=](int b) {
                std::vector<std::pair<ImGuiID, ImRect>> jetzt;
                for (const Element& e : g_elemente) {
                    if (e.id == 0 || e.fenster.find("###main") == std::string::npos) {
                        continue;
                    }
                    jetzt.emplace_back(e.id, e.rect);
                }
                std::sort(jetzt.begin(), jetzt.end(),
                          [](const auto& x, const auto& y) { return x.first < y.first; });
                if (b > schonung && !st->vorher.empty()) {
                    bool anders = jetzt.size() != st->vorher.size();
                    for (std::size_t q = 0; q < jetzt.size() && !anders; ++q) {
                        const ImRect& r0 = st->vorher[q].second;
                        const ImRect& r1 = jetzt[q].second;
                        if (jetzt[q].first != st->vorher[q].first ||
                            std::fabs(r0.Min.x - r1.Min.x) > 0.5F ||
                            std::fabs(r0.Min.y - r1.Min.y) > 0.5F ||
                            std::fabs(r0.Max.x - r1.Max.x) > 0.5F ||
                            std::fabs(r0.Max.y - r1.Max.y) > 0.5F) {
                            anders = true;
                            if (st->erster.empty()) {
                                const Element* e = findeKennung(jetzt[q].first);
                                char z[200];
                                std::snprintf(z, sizeof(z),
                                              "Bild %d: \"%s\" %.0f/%.0f-%.0f/%.0f -> %.0f/%.0f-%.0f/%.0f", b,
                                              e != nullptr ? e->label.c_str() : "?",
                                              static_cast<double>(r0.Min.x), static_cast<double>(r0.Min.y),
                                              static_cast<double>(r0.Max.x), static_cast<double>(r0.Max.y),
                                              static_cast<double>(r1.Min.x), static_cast<double>(r1.Min.y),
                                              static_cast<double>(r1.Max.x), static_cast<double>(r1.Max.y));
                                st->erster = z;
                                if (e != nullptr) {
                                    st->erster += " in \"" + e->fenster + "\"";
                                    if (ImGuiWindow* fw = ImGui::FindWindowByName(e->fenster.c_str()); fw != nullptr) {
                                        char zz[160];
                                        std::snprintf(zz, sizeof(zz), " (Fenster %.0fx%.0f, Rollleiste %d, Inhalt %.0f)",
                                                      static_cast<double>(fw->Size.x), static_cast<double>(fw->Size.y),
                                                      fw->ScrollbarY ? 1 : 0, static_cast<double>(fw->ContentSize.y));
                                        st->erster += zz;
                                    }
                                }
                            }
                        }
                    }
                    if (anders) { ++st->unruhig; }
                }
                st->vorher = std::move(jetzt);
                if (b < bilder) { return false; }
                char z[160];
                std::snprintf(z, sizeof(z), "%s: %d unruhige Bilder von %d",
                              wann.c_str(), st->unruhig, bilder - schonung);
                pruefeWahr(st->unruhig == 0,
                           std::string(z) + (st->erster.empty() ? "" : ", zuerst " + st->erster));
                return true;
            }};
}

// Klick auf den Schliessknopf in der Titelzeile des Ereignisfensters. Er
// hat keine Beschriftung; seine Kennung ist "#CLOSE" im Fenster.
Schritt klickSchliessen() {
    auto ziel = std::make_shared<ImVec2>();
    return {"Klick Schliessknopf", [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    ImGuiWindow* w = ImGui::FindWindowByName("###editor");
                    const Element* e =
                        (w != nullptr) ? findeKennung(ImHashStr("#CLOSE", 0, w->ID))
                                       : nullptr;
                    if (e == nullptr) {
                        meldeFehler("Schliessknopf nicht gefunden");
                        return true;
                    }
                    *ziel = e->rect.GetCenter();
                    setzeMaus(*ziel);
                    return false;
                }
                if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                if (b == 2) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 5;
            }};
}

Schritt taste(ImGuiKey k, const std::string& name) {
    return {"Taste " + name, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) { io.AddKeyEvent(k, true); return false; }
                if (b == 1) { io.AddKeyEvent(k, false); return false; }
                return b >= 4;
            }};
}

// Eine Taste mit Modifikator (Strg, Alt) - oder ohne (ImGuiKey_None).
Schritt tasteMit(ImGuiKey mod, ImGuiKey k, const std::string& name) {
    return {"Taste " + name, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    if (mod != ImGuiKey_None) { io.AddKeyEvent(mod, true); }
                    return false;
                }
                if (b == 1) { io.AddKeyEvent(k, true); return false; }
                if (b == 2) { io.AddKeyEvent(k, false); return false; }
                if (b == 3) {
                    if (mod != ImGuiKey_None) { io.AddKeyEvent(mod, false); }
                    return false;
                }
                return b >= 6;
            }};
}

// Alles im aktiven Feld markieren und ersetzen.
Schritt tippe(const std::string& text) {
    return {"Tippe \"" + text + "\"", [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    io.AddKeyEvent(ImGuiMod_Ctrl, true);
                    io.AddKeyEvent(ImGuiKey_A, true);
                    return false;
                }
                if (b == 1) {
                    io.AddKeyEvent(ImGuiKey_A, false);
                    io.AddKeyEvent(ImGuiMod_Ctrl, false);
                    return false;
                }
                // Markiertes loeschen - sonst liesse sich ein Feld nicht
                // leeren (leerer Text ersetzt nichts).
                if (b == 2) { io.AddKeyEvent(ImGuiKey_Backspace, true); return false; }
                if (b == 3) { io.AddKeyEvent(ImGuiKey_Backspace, false); return false; }
                if (b == 4) { io.AddInputCharactersUTF8(text.c_str()); return false; }
                return b >= 7;
            }};
}

Schritt pruefe(const std::string& text, std::function<bool()> bedingung) {
    return {"Pruefe " + text, [=](int) {
                pruefeWahr(bedingung(), text);
                return true;
            }};
}

Schritt tu(const std::string& text, std::function<void()> was) {
    return {text, [=](int b) {
                if (b == 0) { was(); }
                return b >= 3;
            }};
}

Schritt oeffne(const std::string& befehl, int nte) {
    const std::string was = "Oeffne " + befehl + " #" + std::to_string(nte);
    return {was, [=](int b) {
                if (b == 0) {
                    Path weg;
                    if (!wegZu(befehl, nte, weg)) {
                        meldeFehler(was + ": Befehl nicht im Skript");
                        return true;
                    }
                    openEditorForNode(weg);
                    return false;
                }
                ImGuiWindow* w = ImGui::FindWindowByName("###editor");
                if (w != nullptr && w->Active && !w->Hidden && b >= 3) { return true; }
                if (b > 30) {
                    meldeFehler(was + ": Editor erscheint nicht");
                    return true;
                }
                return false;
            }};
}

Schritt waehle(const std::string& befehl, int nte) {
    return tu("Waehle " + befehl + " #" + std::to_string(nte), [=]() {
        Path weg;
        if (wegZu(befehl, nte, weg)) { selectByPath(weg); }
    });
}

// --- Die Klickpruefung ----------------------------------------------------

// --- Verhalten wie im Original: Suche, Zwischenablage, Speichernfrage ----
//
// Allein aufrufbar mit BHED_EDITORTEST=original (wenige Sekunden).
std::vector<Schritt> originalSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<std::string>();
    auto zahl = std::make_shared<int>(0);
    // ---- Suche wie im Original ------------------------------------------
    // "called" nur der Befehlsname, "containing" jedes Argument einzeln,
    // Weitersuchen ab der Auswahl, am Ende Umlauf.
    auto treffer = std::make_shared<std::vector<Path>>();
    auto gesehen = std::make_shared<std::vector<Path>>();
    add(waehle("rem", 0));
    add(klick(tr(Str::ActFind)));
    add(pruefe("Suche: Whole-string ist vorgewaehlt", [] { return g_app->findWhole; }));
    add(tu("Messung nach dem Oeffnen", [] {
        diag::detail("Knopftest: Suchfenster offen " + std::to_string(g_app->findOpen) +
                     ", " + wasImGuiSieht());
    }));
    add(tippe("wait"));   // das Feld "called" hat beim Oeffnen den Fokus
    add(tu("Messung nach dem Tippen", [] {
        diag::detail("Knopftest: " + wasImGuiSieht());
    }));
    add(Schritt{"Suche: alle Treffer von \"wait\" der Reihe nach", [=](int) {
        FindOptions o;
        o.named = "wait";
        *treffer = findAll(g_app->doc.script(), o);
        gesehen->clear();
        std::vector<Schritt> neu;
        for (std::size_t k = 0; k <= treffer->size(); ++k) {
            neu.push_back(klick(tr(Str::FindNextBtn), "###find"));
            neu.push_back(tu("Treffer merken",
                             [=] { gesehen->push_back(g_app->selectedPath); }));
        }
        neu.push_back(tu("Messung Suche", [=] {
            std::string z = "Knopftest: gemerkt called=\"" + g_app->findLetzte.named +
                            "\" containing=\"" + g_app->findLetzte.contains + "\", Treffer " +
                            std::to_string(treffer->size()) + ", gesehen:";
            for (const Path& p : *gesehen) {
                z += " [";
                for (std::size_t q = 0; q < p.size(); ++q) {
                    z += (q ? "," : "") + std::to_string(p[q]);
                }
                z += "]";
            }
            diag::detail(z);
        }));
        neu.push_back(pruefe("Suche \"called wait\": jeder Treffer ab rem der Reihe nach, "
                             "dann Umlauf zum ersten", [=] {
            if (treffer->size() < 2 || gesehen->size() != treffer->size() + 1) {
                return false;
            }
            for (std::size_t k = 0; k < treffer->size(); ++k) {
                if ((*gesehen)[k] != (*treffer)[k]) { return false; }
            }
            return gesehen->back() == treffer->front();
        }));
        neu.push_back(klick(tr(Str::FindPrevBtn), "###find"));
        neu.push_back(pruefe("Suche zurueck: vom ersten Treffer Umlauf zum letzten",
                             [=] { return g_app->selectedPath == treffer->back(); }));
        schritteEinfuegen(std::move(neu));
        return true;
    }});
    add(tippe(""));   // "called" leeren
    add(klick(tr(Str::FindContains), "###find", 0, false, "", 0, 0.15F));
    add(tippe("100.000"));
    add(waehle("rem", 0));
    add(klick(tr(Str::FindNextBtn), "###find"));
    add(pruefe("Suche \"containing 100.000\": trifft ein wait ( 100.000 ) im Block", [] {
        const Node* n = nodeAt(g_app->doc.script(), g_app->selectedPath);
        return n != nullptr && n->name == "wait" && !n->args.empty() &&
               n->args[0].text == "100.000" && g_app->selectedPath.size() == 2;
    }));
    // Nach "Find next" hat der Knopf den Fokus - erst wieder ins Feld.
    add(klick(tr(Str::FindContains), "###find", 0, false, "", 0, 0.15F));
    add(tippe("wait"));
    add(tu("Auswahl merken", [=] { *gesehen = {g_app->selectedPath}; }));
    add(klick(tr(Str::FindNextBtn), "###find"));
    add(pruefe("Suche \"containing wait\" findet nichts - der Befehlsname zaehlt dort nicht",
               [=] { return g_app->findHits.empty() && g_app->selectedPath == gesehen->front(); }));
    add(tu("Suchfenster schliessen", [] { g_app->findOpen = false; }));

    // ---- Windows-Zwischenablage wie im Original --------------------------
    add(waehle("print", 0));
    add(klick(tr(Str::ActCopy)));
    add(pruefe("Copy schreibt \"//(BHVD)\" + Zeile in die Windows-Zwischenablage", [] {
        const char* c = ImGui::GetClipboardText();
        const std::string s = c != nullptr ? c : "";
        return s.rfind("//(BHVD)\r\nprint (", 0) == 0;
    }));
    add(tu("Auswahl aufheben", [] {
        g_app->selection.clear();
        g_app->selectedPath.clear();
        g_app->selected = -1;
    }));
    add(klick(tr(Str::ActCopy)));
    add(pruefe("Copy ohne Auswahl fragt \"Copy entire script?\"", [] {
        return g_app->frageOffen && g_app->frageText == tr(Str::AskCopyAll);
    }));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pruefe("... und legt das ganze Skript in die Zwischenablage", [] {
        const char* c = ImGui::GetClipboardText();
        const std::string s = c != nullptr ? c : "";
        // Das Testskript beginnt mit einer Leerzeile - sie gehoert dazu.
        return s.rfind("//(BHVD)\r\n", 0) == 0 && s.find("rem (") != std::string::npos &&
               s.find("print (") != std::string::npos &&
               g_app->doc.clipboard().size() == g_app->doc.script().nodes.size();
    }));
    add(tu("Skript merken", [=] { *vorher = skriptText(); *zahl = anzahl("print"); }));
    add(tu("Text wie von BehavEd in die Zwischenablage",
           [] { ImGui::SetClipboardText("//(BHVD)\r\nprint ( \"von aussen\" );\r\n"); }));
    add(waehle("print", 0));
    add(klick(tr(Str::ActPaste)));
    add(pruefe("Paste von \"//(BHVD)\"-Text: ohne Frage eingefuegt", [=] {
        bool da = false;
        for (const Node& n : g_app->doc.script().nodes) {
            if (n.name == "print" && !n.args.empty() && n.args[0].text == "von aussen") { da = true; }
        }
        return !g_app->frageOffen && da;
    }));
    add(klick(tr(Str::EditUndo)));
    add(tu("Fremden Text in die Zwischenablage",
           [] { ImGui::SetClipboardText("wait ( 333.000 );"); }));
    add(klick(tr(Str::ActPaste)));
    add(pruefe("Paste fremden Texts fragt \"You seem to be pasting ...\"", [] {
        return g_app->frageOffen && g_app->frageText == tr(Str::AskPasteForeign);
    }));
    add(klick(tr(Str::AnswerNo), "###frage"));
    add(pruefe("... \"No\" fuegt nichts ein", [=] { return skriptText() == *vorher; }));
    add(klick(tr(Str::ActPaste)));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pruefe("... \"Yes\" liest den Text als Skript und fuegt wait ( 333.000 ) ein", [] {
        bool da = false;
        for (const Node& n : g_app->doc.script().nodes) {
            if (n.name == "wait" && !n.args.empty() && n.args[0].text == "333.000") { da = true; }
        }
        return da;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z nimmt das Einfuegen zurueck", [=] { return skriptText() == *vorher; }));

    // ---- Speichernfrage auch ohne "Yes/No query" --------------------------
    add(tu("Yes/No query aus, Skript aendern", [] {
        g_app->settings.queryOnDiscard = false;
        Path weg;
        if (wegZu("print", 0, weg)) { (void)g_app->doc.cloneAt(weg); rebuildTree(); }
    }));
    add(tu("Etwas anstossen, das Aenderungen verwerfen koennte",
           [] { withUnsaved([] { diag::detail("Knopftest: withUnsaved ohne Frage weiter"); }); }));
    add(pruefe("Ungespeicherte Aenderungen: Speichernfrage kommt auch ohne Yes/No query",
               [] { return g_app->askSaveOpen; }));
    add(klick(tr(Str::EditorCancel), tr(Str::AppTitle)));
    add(tu("Yes/No query wieder an, Aenderung zuruecknehmen", [] {
        g_app->settings.queryOnDiscard = true;
        (void)g_app->doc.undo();
        rebuildTree();
    }));
    add(pruefe("Skript wieder wie vorher", [=] { return skriptText() == *vorher; }));

    // ---- Numpad + / - und Strg+T (ACCELERATOR 135) ----------------------
    auto zeilen = std::make_shared<std::size_t>(0);
    add(tu("Alles zu, erstes if waehlen", [=] {
        g_app->expanded.closeAll();
        rebuildTree();
        Path weg;
        if (wegZu("if", 0, weg)) { selectByPath(weg); }
        *zeilen = g_app->rows.size();
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_KeypadAdd, "Num+"));
    add(pruefe("Num+ klappt den gewaehlten Block samt Inhalt auf", [=] {
        return g_app->rows.size() > *zeilen;
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_KeypadSubtract, "Num-"));
    add(pruefe("Num- klappt ihn wieder zu", [=] { return g_app->rows.size() == *zeilen; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_KeypadAdd, "Strg+Num+"));
    add(tu("Zeilen nach Strg+Num+ merken", [=] {
        diag::detail("Knopftest: Zeilen zu " + std::to_string(*zeilen) + ", alles auf " +
                     std::to_string(g_app->rows.size()));
    }));
    add(pruefe("Strg+Num+ klappt ALLES auf (mehr als nur ein Block)", [=] {
        return g_app->rows.size() > *zeilen + 1;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_KeypadSubtract, "Strg+Num-"));
    add(pruefe("Strg+Num- klappt alles zu", [=] { return g_app->rows.size() == *zeilen; }));
    add(tu("Skript merken", [=] { *vorher = skriptText(); *zahl = anzahl("print"); }));
    add(waehle("print", 0));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_T, "Strg+T"));
    add(pruefe("Strg+T schneidet aus wie Strg+X", [=] { return anzahl("print") == *zahl - 1; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo nach Strg+T: Skript wie vorher", [=] { return skriptText() == *vorher; }));

    // ---- Backup / Restore wie im Original -------------------------------
    auto datei = std::make_shared<std::string>(platform::executableDirectory() +
                                               "\\selbsttest-backup.txt");
    add(tu("Pfad setzen, alte Sicherungen weg", [=] {
        std::remove((*datei).c_str());
        std::remove((platform::executableDirectory() + "\\selbsttest-backup.bak").c_str());
        g_app->path = *datei;
        *vorher = skriptText();
    }));
    add(klick(tr(Str::FileRestore)));
    add(pruefe("Restore ohne Sicherung: keine Frage (\"No Backup file available...\")",
               [] { return !g_app->frageOffen; }));
    add(klick(tr(Str::FileBackup)));
    add(pruefe("Backup schreibt x.bak (nicht x.txt.bak) mit dem aktuellen Stand", [=] {
        const std::string bak = platform::executableDirectory() + "\\selbsttest-backup.bak";
        return slurp(bak) == skriptText();
    }));
    add(tu("Skript aendern", [] {
        Path weg;
        if (wegZu("print", 0, weg)) { (void)g_app->doc.cloneAt(weg); rebuildTree(); }
    }));
    add(klick(tr(Str::FileRestore)));
    add(pruefe("Restore fragt immer: \"This RESTORE will overwrite ...\"", [] {
        return g_app->frageOffen && g_app->frageText == tr(Str::RestoreAsk);
    }));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pruefe("Restore stellt den gesicherten Stand her", [=] { return skriptText() == *vorher; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo nach Restore: der geaenderte Stand ist wieder da",
               [=] { return anzahl("print") == *zahl + 1; }));
    add(klick(tr(Str::EditUndo)));
    add(tu("Sicherung wegraeumen", [=] {
        std::remove((platform::executableDirectory() + "\\selbsttest-backup.bak").c_str());
    }));

    // ---- Compile mit Fehler: keine .ibi ----------------------------------
    add(tu("Unbekannten Befehl einfuegen", [=] {
        std::remove((platform::executableDirectory() + "\\selbsttest-backup.ibi").c_str());
        Node n;
        n.kind = Node::Kind::Command;
        n.name = "gibtsnicht";
        Arg a;
        a.kind = Arg::Kind::Number;
        a.text = "1";
        n.args.push_back(a);
        (void)g_app->doc.insertAfter(Path{}, n);
        rebuildTree();
    }));
    add(pruefe("Compile-Knopf ist gross (mindestens doppelte Knopfhoehe)", [=] {
        const Element* e = finde(tr(Str::Compile), "", 0, false);
        return e != nullptr && e->rect.GetHeight() >= ImGui::GetFrameHeight() * 2.0F;
    }));
    add(klick(tr(Str::Compile)));
    add(pruefe("Compile mit Fehler schreibt KEINE .ibi", [=] {
        std::error_code ec;
        const bool ibi = std::filesystem::exists(
            ::bhed::utf8Pfad(platform::executableDirectory() + "\\selbsttest-backup.ibi"), ec);
        return !ibi;
    }));
    add(pruefe("Compile mit Fehler: Knopf zeigt kurz \"Failed\"",
               [] { return g_app->kompiliertFehlerBis > ImGui::GetTime() &&
                           finde(tr(Str::Compile), "", 0, false) == nullptr; }));
    add(klick(tr(Str::EditUndo)));
    // Der Knopf behaelt seine Kennung "###compile" in jedem Zustand.
    add(klick("###compile"));
    add(pruefe("Compile ohne Fehler schreibt die .ibi", [=] {
        std::error_code ec;
        return std::filesystem::exists(
            ::bhed::utf8Pfad(platform::executableDirectory() + "\\selbsttest-backup.ibi"), ec);
    }));
    add(pruefe("Compile ohne Fehler: Knopf zeigt \"Compiled\" mit Haken",
               [] { return g_app->kompiliertBis > ImGui::GetTime() &&
                           finde(tr(Str::Compile), "", 0, false) == nullptr; }));
    add(warteBis("Knopf zurueck auf \"Compile!\"",
                 [] { return ImGui::GetTime() > g_app->kompiliertBis &&
                             finde(tr(Str::Compile), "", 0, false) != nullptr; }, 3000));
    add(pruefe("nach 1,5 s steht wieder \"Compile!\" da",
               [] { return finde(tr(Str::Compile), "", 0, false) != nullptr; }));
    add(tu("Testdateien wegraeumen, Pfad leeren", [=] {
        std::remove((*datei).c_str());
        std::remove((platform::executableDirectory() + "\\selbsttest-backup.ibi").c_str());
        auto& r = g_app->settings.recent;
        r.erase(std::remove(r.begin(), r.end(), *datei), r.end());
        g_app->path.clear();
    }));

    // ---- Alpha-sort -----------------------------------------------------
    add(tu("Alpha-sort an", [] { g_app->settings.alphaSortPulldowns = true; }));
    add(oeffne("set", 0));
    add(klick("##combo", "###editor"));
    add(pruefe("Alpha-sort: die Klappliste steht alphabetisch", [] {
        std::vector<std::pair<float, std::string>> z;
        for (const Element& e : g_elemente) {
            if (e.fenster.find("##Combo") == std::string::npos) { continue; }
            if (e.label.empty() || e.label.rfind("##", 0) == 0) { continue; }
            if (e.label.find("###") != std::string::npos) { continue; }
            z.emplace_back(e.rect.Min.y, e.label);
        }
        std::sort(z.begin(), z.end());
        if (z.size() < 3) { return false; }
        for (std::size_t q = 1; q < z.size(); ++q) {
            std::string a = z[q - 1].second;
            std::string b = z[q].second;
            for (char& ch : a) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
            for (char& ch : b) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
            if (b < a) {
                diag::detail("Knopftest: nicht sortiert: " + z[q - 1].second + " vor " + z[q].second);
                return false;
            }
        }
        return true;
    }));
    // EIN Escape schliesst die Liste (wie eine Windows-Klappliste).
    add(tasteMit(ImGuiKey_None, ImGuiKey_Escape, "Escape (Liste zu)"));
    add(pruefe("Escape schliesst die Klappliste mit einem Druck", [] {
        for (const Element& e : g_elemente) {
            if (e.fenster.find("##Combo") != std::string::npos) { return false; }
        }
        return g_app->editorOpen;
    }));
    add(klick(tr(Str::EditorCancel), "###editor"));
    add(tu("Alpha-sort aus", [] { g_app->settings.alphaSortPulldowns = false; }));

    // ---- Prefs: Cancel spielt zurueck, Ok uebernimmt ---------------------
    auto merk = std::make_shared<bool>(false);
    add(tu("Alpha-sort merken", [=] { *merk = g_app->settings.alphaSortPulldowns; }));
    add(klick(tr(Str::AppPrefs)));
    add(klick(tr(Str::PrefsAlphaSort), tr(Str::PrefsTitle)));
    add(pruefe("Prefs: Haekchen umgeschaltet", [=] {
        return g_app->settings.alphaSortPulldowns != *merk;
    }));
    add(klick(tr(Str::EditorCancel), tr(Str::PrefsTitle)));
    add(pruefe("Prefs Cancel: alter Stand zurueck",
               [=] { return !g_app->prefsOpen && g_app->settings.alphaSortPulldowns == *merk; }));
    add(klick(tr(Str::AppPrefs)));
    add(klick(tr(Str::PrefsAlphaSort), tr(Str::PrefsTitle)));
    add(klick(tr(Str::EditorOk), tr(Str::PrefsTitle)));
    add(pruefe("Prefs Ok: Aenderung bleibt",
               [=] { return !g_app->prefsOpen && g_app->settings.alphaSortPulldowns != *merk; }));
    add(tu("Alpha-sort zurueck", [=] { g_app->settings.alphaSortPulldowns = *merk; }));

    // ---- Alt-Kuerzel -----------------------------------------------------
    add(tasteMit(ImGuiMod_Alt, ImGuiKey_F, "Alt+F"));
    add(pruefe("Alt+F oeffnet die Suche (\"&Find\")", [] { return g_app->findOpen; }));
    add(tu("Suchfenster schliessen", [] { g_app->findOpen = false; }));

    // ---- Schreibgeschuetzte Datei ----------------------------------------
    auto nurLesen = std::make_shared<std::string>(platform::executableDirectory() +
                                                  "\\selbsttest-nurlesen.txt");
    add(tu("Schreibgeschuetzte Datei anlegen", [=] {
        std::error_code ec;
        const std::filesystem::path fp = ::bhed::utf8Pfad(*nurLesen);
        std::filesystem::permissions(fp, std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::add, ec);
        (void)spewDatei(*nurLesen, "//Generated by BehavEd\r\n");
        // ALLE Schreibrechte weg: erst dann setzt MSVC das Windows-Attribut
        // "Schreibgeschuetzt".
        std::filesystem::permissions(fp, std::filesystem::perms::owner_write |
                                             std::filesystem::perms::group_write |
                                             std::filesystem::perms::others_write,
                                     std::filesystem::perm_options::remove, ec);
        g_app->path = *nurLesen;
    }));
    add(klick(tr(Str::FileSave)));
    add(pruefe("Speichern einer schreibgeschuetzten Datei fragt nach dem Entsperren", [] {
        return g_app->frageOffen &&
               g_app->frageText.find("write-protected") != std::string::npos;
    }));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pruefe("... \"Yes\": Schutz aufgehoben und gespeichert", [=] {
        return slurp(*nurLesen) == skriptText();
    }));
    add(tu("Testdatei wegraeumen", [=] {
        std::error_code ec;
        std::filesystem::permissions(::bhed::utf8Pfad(*nurLesen),
                                     std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::add, ec);
        std::remove((*nurLesen).c_str());
        auto& r = g_app->settings.recent;
        r.erase(std::remove(r.begin(), r.end(), *nurLesen), r.end());
        g_app->path.clear();
    }));

    // ---- REM wie im Original ---------------------------------------------
    // Blockstruktur bleibt im Baum, Ruecktaste schaltet um, Un-REM nimmt nur
    // die eigene Anweisung zurueck.
    auto remWeg = std::make_shared<Path>();
    auto sucheZeile = [](const std::string& anfang) -> const Row* {
        for (const Row& r : g_app->rows) {
            if (r.what == Row::What::Comment && r.text.rfind(anfang, 0) == 0) { return &r; }
        }
        return nullptr;
    };
    add(tu("Alles zu, erstes if waehlen, Skript merken", [=] {
        g_app->expanded.closeAll();
        rebuildTree();
        Path weg;
        if (wegZu("if", 0, weg)) { selectByPath(weg); }
        *vorher = skriptText();
        *zahl = anzahl("if");
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_Backspace, "Ruecktaste (REM)"));
    add(pruefe("REM: das if ist auskommentiert", [=] { return anzahl("if") == *zahl - 1; }));
    add(pruefe("REM im Baum: EINE klappbare Zeile \"/////////////  if ...\", keine {-Zeile", [=] {
        const Row* kopf = sucheZeile("/////////////  if");
        const bool klammer = sucheZeile("/////////////  {") != nullptr ||
                             sucheZeile("/////////////  }") != nullptr;
        return kopf != nullptr && kopf->childCount > 0 && !kopf->open && !klammer;
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_KeypadAdd, "Num+ (REM-Block auf)"));
    add(pruefe("REM-Block aufgeklappt: sein wait steht eingerueckt darunter", [=] {
        const Row* kopf = sucheZeile("/////////////  if");
        const Row* kind = sucheZeile("/////////////  wait");
        return kopf != nullptr && kind != nullptr && kind->depth == kopf->depth + 1;
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_Backspace, "Ruecktaste (Un-REM)"));
    add(pruefe("Ruecktaste auf der REM-Zeile: if zurueck und ausgewaehlt, Skript wie vorher", [=] {
        const Node* n = nodeAt(g_app->doc.script(), g_app->selectedPath);
        return skriptText() == *vorher && n != nullptr && n->name == "if";
    }));
    add(waehle("set", 0));
    add(tasteMit(ImGuiKey_None, ImGuiKey_Backspace, "Ruecktaste (REM set)"));
    add(tu("REM-Zeile von set merken", [=] { *remWeg = g_app->selectedPath; }));
    add(waehle("wait", 0));
    add(tasteMit(ImGuiKey_None, ImGuiKey_Backspace, "Ruecktaste (REM wait)"));
    add(tu("Zurueck auf die REM-Zeile von set", [=] {
        g_app->selectedPath = *remWeg;
        g_app->selection.assign(1, *remWeg);
        rebuildTree();
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_Backspace, "Ruecktaste (Un-REM set)"));
    add(pruefe("Un-REM auf set laesst den getrennt auskommentierten Nachbarn wait REM", [=] {
        return anzahl("set") == 1 && anzahl("wait") == 0;
    }));
    add(tu("Dreimal Rueckgaengig (REM set, REM wait, Un-REM set)", [] {
        (void)g_app->doc.undo();
        (void)g_app->doc.undo();
        (void)g_app->doc.undo();
        rebuildTree();
    }));
    add(pruefe("Rueckgaengig: Skript wie vorher", [=] { return skriptText() == *vorher; }));

    return s;
}

std::vector<Schritt> knopfSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Zwischen den Schritten gemerkte Zustaende.
    auto vorher = std::make_shared<std::string>();
    auto zeilen = std::make_shared<std::size_t>(0);
    auto zahl = std::make_shared<int>(0);
    const std::string ed = "###editor";

    // ---- Ereigniseditor: Ok, Abbrechen, Escape, Eingabe, Schliessen -----
    add(oeffne("print", 0));
    add(klick("##v", ed));
    add(tippe("Hallo Test"));
    add(pruefe("print: Feld zeigt den getippten Text",
               [] { return wert(0) == "Hallo Test"; }));
    add(klick(tr(Str::EditorOk), ed));
    add(pruefe("print: Ok schliesst den Editor", [] { return !g_app->editorOpen; }));
    add(pruefe("print: Ok schreibt den Wert ins Skript",
               [] { return argText("print", 0, 0) == "Hallo Test"; }));

    add(tu("Skript merken", [=] { *vorher = skriptText(); }));
    add(oeffne("print", 0));
    add(klick("##v", ed));
    add(tippe("Wegwerfen"));
    add(klick(tr(Str::EditorCancel), ed));
    add(pruefe("print: Cancel schliesst den Editor", [] { return !g_app->editorOpen; }));
    add(pruefe("print: Cancel laesst das Skript unveraendert",
               [=] { return skriptText() == *vorher; }));

    add(oeffne("print", 0));
    add(klick("##v", ed));
    add(tippe("Escape-Text"));
    add(taste(ImGuiKey_Escape, "Escape"));
    add(pruefe("print: Escape schliesst den Editor", [] { return !g_app->editorOpen; }));
    add(pruefe("print: Escape laesst das Skript unveraendert",
               [=] { return skriptText() == *vorher; }));

    add(oeffne("print", 0));
    add(klickSchliessen());
    add(pruefe("print: Schliessknopf schliesst den Editor",
               [] { return !g_app->editorOpen; }));
    add(pruefe("print: Schliessknopf laesst das Skript unveraendert",
               [=] { return skriptText() == *vorher; }));

    add(oeffne("print", 0));
    add(klick("##v", ed));
    add(tippe("Per Eingabe"));
    add(taste(ImGuiKey_Enter, "Eingabe"));
    add(pruefe("print: Eingabe schliesst den Editor", [] { return !g_app->editorOpen; }));
    add(pruefe("print: Eingabe schreibt den Wert ins Skript",
               [] { return argText("print", 0, 0) == "Per Eingabe"; }));

    // ---- Expr! und Revert ------------------------------------------------
    add(oeffne("print", 0));
    add(klick(tr(Str::EditorExpr), ed));
    add(pruefe("print: Expr! schaltet auf Ausdruck", [] { return istAusdruck(0); }));
    add(pruefe("print: Expr! zeigt die Helferzeilen (Get/Tag/Rnd)", [=] {
        return finde(tr(Str::EditorGet), ed, 0, false) != nullptr &&
               finde(tr(Str::EditorTag), ed, 0, false) != nullptr &&
               finde(tr(Str::EditorRnd), ed, 0, false) != nullptr;
    }));
    add(klick(tr(Str::EditorRevert), ed));
    add(pruefe("print: Revert schaltet zurueck", [] { return !istAusdruck(0); }));
    add(pruefe("print: Revert blendet die Helferzeilen aus",
               [=] { return finde(tr(Str::EditorGet), ed, 0, false) == nullptr; }));
    add(klick(tr(Str::EditorCancel), ed));

    // ---- Helferknoepfe Get / Tag / Rnd -----------------------------------
    add(tu("Skript merken", [=] { *vorher = skriptText(); }));
    add(oeffne("if", 0));
    add(pruefe("if/3: alle drei Felder sind Ausdruecke",
               [] { return istAusdruck(0) && istAusdruck(1) && istAusdruck(2); }));
    add(klick(tr(Str::EditorRnd), ed, 0));
    add(pruefe("if/3: Rnd schreibt random( ... ) in Feld 0",
               [] { return wert(0).rfind("random(", 0) == 0; }));
    add(klick(tr(Str::EditorGet), ed, 0));
    add(pruefe("if/3: Get schreibt get( ... ) in Feld 0",
               [] { return wert(0).rfind("get(", 0) == 0; }));
    add(klick(tr(Str::EditorTag), ed, 0));
    add(pruefe("if/3: Tag schreibt tag( ... ) in Feld 0",
               [] { return wert(0).rfind("tag(", 0) == 0; }));
    // Genau die Schreibweise des Originals (am laufenden BehavEd abgelesen,
    // 27.09.) - mit den Vorgaben FLOAT / SET_PARM1 / targetname / ORIGIN /
    // 0 .. 1. Die Werte werden erst gesetzt: behaved merkt sich die letzte
    // Helferwahl je Befehl, die Vorbelegung haengt also von frueheren
    // Laeufen ab.
    add(tu("Helfer auf die Vorgaben", [] {
        g_app->helpGetType[0] = "FLOAT";
        g_app->helpGetName[0] = "SET_PARM1";
        g_app->helpTagName[0] = "targetname";
        g_app->helpTagType[0] = "ORIGIN";
        g_app->helpRangeLow[0] = 0.0F;
        g_app->helpRangeHigh[0] = 1.0F;
    }));
    add(klick(tr(Str::EditorGet), ed, 0));
    add(pruefe("if/3: Get wie im Original: get( FLOAT, \"SET_PARM1\")",
               [] { return wert(0) == "get( FLOAT, \"SET_PARM1\")"; }));
    add(klick(tr(Str::EditorTag), ed, 0));
    add(pruefe("if/3: Tag wie im Original: tag( \"targetname\", ORIGIN)",
               [] { return wert(0) == "tag( \"targetname\", ORIGIN)"; }));
    add(klick(tr(Str::EditorRnd), ed, 0));
    add(pruefe("if/3: Rnd wie im Original: random( 0, 1 )",
               [] { return wert(0) == "random( 0, 1 )"; }));
    add(tu("Bereich 5 .. 12.5", [] {
        g_app->helpRangeLow[0] = 5.0F;
        g_app->helpRangeHigh[0] = 12.5F;
    }));
    add(klick(tr(Str::EditorRnd), ed, 0));
    add(pruefe("if/3: Rnd mit 5 .. 12.5 wie im Original: random( 5, 12.5 )",
               [] { return wert(0) == "random( 5, 12.5 )"; }));
    add(klick(tr(Str::EditorTag), ed, 0));
    add(klick(tr(Str::EditorRnd), ed, 2));
    add(pruefe("if/3: Rnd der dritten Spalte trifft Feld 2, nicht Feld 0",
               [] { return wert(2).rfind("random(", 0) == 0 &&
                           wert(0).rfind("tag(", 0) == 0; }));
    add(klick(tr(Str::EditorCancel), ed));
    add(pruefe("if/3: Cancel verwirft die Helferwerte",
               [=] { return skriptText() == *vorher; }));

    // ---- Re-Evaluate -----------------------------------------------------
    add(oeffne("set", 0));
    add(klick("##v", ed, 0));
    add(tippe("geaendert"));
    add(pruefe("set: Feld 1 zeigt den getippten Text",
               [] { return wert(1) == "geaendert"; }));
    auto auswahl0 = std::make_shared<std::string>();
    add(tu("Auswahl merken", [=] { *auswahl0 = wert(0); }));
    add(klick(tr(Str::EditorReEvaluate), ed));
    add(pruefe("set: Re-Evaluate setzt Feld 1 auf die Vorgabe zurueck",
               [] { return wert(1) != "geaendert"; }));
    add(pruefe("set: Re-Evaluate laesst die Auswahl (Feld 0) stehen",
               [=] { return wert(0) == *auswahl0; }));
    add(klick(tr(Str::EditorCancel), ed));

    // ---- Klappliste ------------------------------------------------------
    add(oeffne("affect", 0));
    add(pruefe("affect: Feld 1 steht auf FLUSH", [] { return wert(1) == "FLUSH"; }));
    add(klick("##combo", ed));
    add(klick("INSERT", "", 0, true));
    add(pruefe("affect: Auswahl INSERT steht im Feld",
               [] { return wert(1) == "INSERT"; }));
    add(klick(tr(Str::EditorOk), ed));
    add(pruefe("affect: Ok schreibt INSERT ins Skript",
               [] { return argText("affect", 0, 1) == "INSERT"; }));

    // ---- Befehle ohne Felder und mit Dateifeld ---------------------------
    add(tu("Skript merken", [=] { *vorher = skriptText(); }));
    add(oeffne("else", 0));
    add(klick(tr(Str::EditorOk), ed));
    add(pruefe("else: Ok ohne Felder stuerzt nicht und aendert nichts",
               [=] { return !g_app->editorOpen && skriptText() == *vorher; }));

    add(oeffne("sound", 0));
    // "..." (Browse) NICHT klicken: seit rc568 oeffnet es wie im Original
    // einen Windows-Dateidialog, und der haelt den Lauf an, bis jemand ihn
    // schliesst. Geprueft wird nur, dass der Knopf da und ganz sichtbar ist.
    add(pruefe("sound: Knopf \"...\" (Browse) ist da und ganz sichtbar", [=] {
        const Element* e = finde(tr(Str::EditorBrowse), ed, 0, false);
        return e != nullptr && ganzSichtbar(*e);
    }));
    add(klick(tr(Str::EditorPlay), ed));
    add(pruefe("sound: Play laesst den Editor offen",
               [] { return g_app->editorOpen; }));
    add(klick(tr(Str::EditorCancel), ed));

    add(oeffne("camera", 0));
    add(klick(tr(Str::EditorOk), ed));
    add(pruefe("camera PAN: Ok ohne Aenderung laesst das Skript gleich",
               [=] { return skriptText() == *vorher; }));

    // ---- Aktionsspalte ---------------------------------------------------
    add(tu("Skript merken", [=] { *vorher = skriptText(); }));
    add(waehle("print", 0));
    add(klick(tr(Str::ActClone)));
    add(pruefe("Clone: print zweimal im Skript", [] { return anzahl("print") == 2; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo: Skript wie vorher", [=] { return skriptText() == *vorher; }));
    add(klick(tr(Str::EditRedo)));
    add(pruefe("Redo: print wieder zweimal", [] { return anzahl("print") == 2; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo nach Redo: Skript wie vorher",
               [=] { return skriptText() == *vorher; }));

    add(waehle("wait", 0));
    add(tu("Anzahl merken", [=] { *zahl = anzahl("wait"); }));
    add(klick(tr(Str::ActDelete)));
    add(pruefe("Delete: ein wait weniger", [=] { return anzahl("wait") == *zahl - 1; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo nach Delete: Skript wie vorher",
               [=] { return skriptText() == *vorher; }));

    add(waehle("print", 0));
    add(klick(tr(Str::ActCopy)));
    add(klick(tr(Str::ActPaste)));
    add(pruefe("Copy + Paste: print zweimal", [] { return anzahl("print") == 2; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo nach Paste: Skript wie vorher",
               [=] { return skriptText() == *vorher; }));

    add(waehle("print", 0));
    add(klick(tr(Str::ActCut)));
    add(pruefe("Cut: print weg", [] { return anzahl("print") == 0; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo nach Cut: Skript wie vorher",
               [=] { return skriptText() == *vorher; }));

    add(waehle("print", 0));
    add(klick(tr(Str::ActRem)));
    add(pruefe("Rem: print ist auskommentiert", [] { return anzahl("print") == 0; }));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Undo nach Rem: Skript wie vorher",
               [=] { return skriptText() == *vorher; }));

    add(waehle("set", 0));
    add(tu("Stelle merken", [=] {
        Path weg;
        *zahl = wegZu("set", 0, weg) ? static_cast<int>(weg[0]) : -1;
    }));
    add(klick(tr(Str::MoveUp)));
    add(pruefe("Up: set eine Stelle hoeher", [=] {
        Path weg;
        return wegZu("set", 0, weg) && static_cast<int>(weg[0]) == *zahl - 1;
    }));
    add(klick(tr(Str::MoveDown)));
    add(pruefe("Down: set wieder an alter Stelle",
               [=] { return skriptText() == *vorher; }));

    add(klick(tr(Str::ActFind)));
    add(pruefe("Find: Suchfenster offen", [] { return g_app->findOpen; }));
    add(tu("Suchfenster schliessen", [] { g_app->findOpen = false; }));

    // Beim Laden sind Bloecke zu. Erst alles auf, dann alles zu - und die
    // Zahlen ins Protokoll, damit ein Fehler sagt, was er gesehen hat.
    add(tu("Zeilen merken", [=] {
        *zeilen = g_app->rows.size();
        diag::detail("Knopftest: Zeilen vorher " + std::to_string(*zeilen));
    }));
    add(klick(tr(Str::TreeExpandAll), "", 0, false, tr(Str::TreeCollapseAll)));
    add(pruefe("Expand all: mehr Zeilen im Baum", [=] {
        const Node* k = knoten("if", 0);
        diag::detail("Knopftest: Zeilen nach Expand " +
                     std::to_string(g_app->rows.size()) + ", if#0 Kinder " +
                     std::to_string(k != nullptr ? k->children.size() : 0U) +
                     ", Kennung " + std::to_string(k != nullptr ? k->kennung : 0U) +
                     ", offen " +
                     std::to_string(k != nullptr && g_app->expanded.isOpen(k->kennung)));
        return g_app->rows.size() > *zeilen;
    }));
    add(klick(tr(Str::TreeCollapseAll)));
    add(pruefe("Collapse all: wieder so wenige Zeilen wie vorher", [=] {
        diag::detail("Knopftest: Zeilen nach Collapse " +
                     std::to_string(g_app->rows.size()));
        return g_app->rows.size() == *zeilen;
    }));

    add(klick(tr(Str::TreeShowTypes)));
    add(pruefe("Show Types: eingeschaltet", [] { return g_app->treeOpt.showTypes; }));
    add(klick(tr(Str::TreeShowTypes)));
    add(pruefe("Show Types: wieder aus", [] { return !g_app->treeOpt.showTypes; }));
    add(tu("gFloats merken", [=] { *zahl = g_app->treeOpt.gFloats ? 1 : 0; }));
    add(klick(tr(Str::TreeGFloats)));
    add(pruefe("%g floats: umgeschaltet",
               [=] { return (g_app->treeOpt.gFloats ? 1 : 0) != *zahl; }));
    add(klick(tr(Str::TreeGFloats)));
    add(pruefe("%g floats: zurueck",
               [=] { return (g_app->treeOpt.gFloats ? 1 : 0) == *zahl; }));
    add(tu("Makros merken", [=] { *zahl = g_app->treeOpt.foldMacros ? 1 : 0; }));
    add(klick(tr(Str::TreeFoldMacros)));
    add(pruefe("Fold macros: umgeschaltet",
               [=] { return (g_app->treeOpt.foldMacros ? 1 : 0) != *zahl; }));
    add(klick(tr(Str::TreeFoldMacros)));
    add(pruefe("Fold macros: zurueck",
               [=] { return (g_app->treeOpt.foldMacros ? 1 : 0) == *zahl; }));

    add(klick(tr(Str::AppPrefs)));
    add(pruefe("Prefs: Einstellungen offen", [] { return g_app->prefsOpen; }));
    add(tu("Einstellungen schliessen", [] { g_app->prefsOpen = false; }));
    add(klick(tr(Str::AppAbout)));
    add(pruefe("About: Fenster offen", [] { return g_app->aboutOpen; }));
    add(tu("About schliessen", [] { g_app->aboutOpen = false; }));

    add(pruefe("Skript nach allen Aktionen wie vorher",
               [=] { return skriptText() == *vorher; }));

    // Suche, Windows-Zwischenablage, Speichernfrage: originalSchritte().
    for (Schritt& x : originalSchritte()) { s.push_back(std::move(x)); }

    // New zuletzt: es oeffnet einen neuen Reiter.
    add(tu("Reiter merken", [=] { *zahl = static_cast<int>(g_app->tabs.size()); }));
    add(klick(tr(Str::FileNew)));
    add(pruefe("New: fragt \"New?\" (Yes/No query an)", [] {
        return g_app->frageOffen && g_app->frageText == tr(Str::AskNew);
    }));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pruefe("New: ein leeres Skript im neuen Reiter", [=] {
        return static_cast<int>(g_app->tabs.size()) == *zahl + 1 &&
               g_app->doc.script().nodes.empty();
    }));

    return s;
}

// --- Jeder Befehl: eintragen, speichern, laden, pruefen, aendern ---------
//
// shank: "bei jedem Event parameter testen alles einzutragen dann speichern
// und wieder oeffnen jeden paramter zu aendern filter auszuprobieren."
//
// Fuer JEDEN Befehl der .bhc (auch beide Fassungen von if, set, ...):
//
//   1. einfuegen wie ein Klick in die Ereignisliste, Editor oeffnen
//   2. jedes Feld fuellen: Textfelder antippen und tippen, Klapplisten
//      aufklappen, im Suchfeld FILTERN, pruefen, dass nur Passendes
//      uebrig ist, und den Treffer anklicken
//   3. Ok - im Skript muss jeder Wert stehen
//
// Danach fuer alle zusammen: Pfad setzen, den echten Save-Knopf druecken,
// die Datei neu laden (byte-gleich?), jeden Befehl wieder oeffnen und jeden
// Wert vergleichen. Dann Runde 2 mit anderen Werten, noch einmal speichern,
// laden, vergleichen.
//
// Welche Felder es gibt, steht erst NACH der Wahl im ersten Feld fest
// (`set` mit SET_ANIM_BOTH hat andere Felder als mit SET_PARM1, `camera`
// PAN andere als ENABLE). Deshalb wird Feld fuer Feld entschieden: ein
// Schritt schaut nach, was im Editor steht, und haengt die Schritte fuer
// genau dieses Feld und den naechsten Nachschauer hinter sich ein.

std::vector<Schritt>* g_liste = nullptr;
std::size_t g_einfuegeAn = 0;

void schritteEinfuegen(std::vector<Schritt> neu) {
    if (g_liste == nullptr) { return; }
    const std::size_t n = neu.size();
    g_liste->insert(g_liste->begin() + static_cast<std::ptrdiff_t>(g_einfuegeAn),
                    std::make_move_iterator(neu.begin()),
                    std::make_move_iterator(neu.end()));
    g_einfuegeAn += n;
}

struct BefehlsPlan {
    std::vector<const Command*> befehle;
    // Je Befehl: fester Eintrag fuer die erste Klappliste (leer = frei
    // gewaehlt). Runde 3 geht damit jeden Eintrag einmal durch.
    std::vector<std::string> zwang;
    std::vector<std::vector<std::string>> erwartet;   // je Befehl, je Feld
    std::string datei;
};

std::string klein(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

std::string befehlsName(const BefehlsPlan& p, std::size_t i) {
    std::string n = p.befehle[i]->name + "/" +
                    std::to_string(p.befehle[i]->params.size());
    if (i < p.zwang.size() && !p.zwang[i].empty()) { n += " " + p.zwang[i]; }
    return n + " (#" + std::to_string(i) + ")";
}

// Der i-te Befehl auf oberster Ebene. Leerzeilen zaehlen nicht - nach dem
// Laden stehen hinter Bloecken welche.
bool befehlsWeg(std::size_t i, Path& weg) {
    std::size_t gefunden = 0;
    const std::vector<Node>& n = g_app->doc.script().nodes;
    for (std::size_t k = 0; k < n.size(); ++k) {
        if (n[k].kind != Node::Kind::Command) { continue; }
        if (gefunden == i) {
            weg = Path{k};
            return true;
        }
        ++gefunden;
    }
    return false;
}

std::size_t befehlsZahl() {
    std::size_t z = 0;
    for (const Node& n : g_app->doc.script().nodes) {
        if (n.kind == Node::Kind::Command) { ++z; }
    }
    return z;
}

// Was in ein Textfeld getippt wird. Runde 0 und 1 verschieden, damit das
// Aendern sichtbar ist. Formate wie beim Speichern: Zahlen mit drei
// Nachkommastellen, Vektoren ohne Klammern.
std::string testwert(const Param& p, int runde, std::size_t i, std::size_t k,
                     bool ausdruck) {
    const bool a = (runde == 0);
    if (p.kind == Param::Kind::Op) { return a ? ">" : "<"; }
    if (ausdruck) { return a ? "random( 1.000, 2.000 )" : "random( 3.000, 4.000 )"; }
    switch (p.kind) {
        case Param::Kind::Int:    return a ? "7" : "12";
        case Param::Kind::Float:  return a ? "1.500" : "2.250";
        case Param::Kind::Vector: return a ? "1.000 2.000 3.000" : "4.000 5.000 6.000";
        default: break;
    }
    if (!p.filter.empty()) { return a ? "sound/test_a.mp3" : "sound/test_b.mp3"; }
    return std::string(a ? "test_a_" : "test_b_") + std::to_string(i) + "_" +
           std::to_string(k);
}

std::string vergleiche(const std::vector<std::string>& soll,
                       const std::vector<std::string>& ist) {
    if (soll.size() != ist.size()) {
        return "Feldzahl " + std::to_string(ist.size()) + " statt " +
               std::to_string(soll.size());
    }
    for (std::size_t k = 0; k < soll.size(); ++k) {
        if (soll[k] != ist[k]) {
            return "Feld " + std::to_string(k) + ": \"" + ist[k] + "\" statt \"" +
                   soll[k] + "\"";
        }
    }
    return {};
}

std::vector<std::string> argTexte(const Node* n) {
    std::vector<std::string> v;
    if (n != nullptr) {
        for (const Arg& a : n->args) { v.push_back(a.text); }
    }
    return v;
}

Schritt oeffneBefehl(std::shared_ptr<BefehlsPlan> plan, std::size_t i) {
    const std::string was = "Oeffne " + befehlsName(*plan, i);
    return {was, [=](int b) {
                if (b == 0) {
                    Path weg;
                    if (!befehlsWeg(i, weg)) {
                        meldeFehler(was + ": nicht im Skript");
                        return true;
                    }
                    openEditorForNode(weg);
                    return false;
                }
                ImGuiWindow* w = ImGui::FindWindowByName("###editor");
                if (w != nullptr && w->Active && !w->Hidden && b >= 3) { return true; }
                if (b > 30) {
                    meldeFehler(was + ": Editor erscheint nicht");
                    return true;
                }
                return false;
            }};
}

// Sieht nach, welches Feld `k` gerade ist, und haengt die Schritte dafuer
// samt dem Nachschauer fuer `k + 1` hinter sich ein.
Schritt fuelleAb(std::shared_ptr<BefehlsPlan> plan, std::size_t i, int runde,
                 std::size_t k) {
    const std::string was = "Fuelle " + befehlsName(*plan, i) + " Feld " +
                            std::to_string(k);
    return {was, [=](int) {
        if (!g_app->editorOpen || g_app->editorCmd == nullptr) {
            meldeFehler(was + ": Editor ist zu");
            return true;
        }
        const std::vector<Param> f =
            editorFields(*g_app->editorCmd, g_app->db, g_app->editorValues);
        std::vector<std::string>& erw = plan->erwartet[i];
        if (k >= f.size()) {
            erw.resize(f.size());
            return true;
        }
        if (erw.size() < f.size()) { erw.resize(f.size()); }
        // Das wievielte Textfeld / die wievielte Klappliste ist Feld k?
        int nText = 0;
        int nListe = 0;
        for (std::size_t q = 0; q < k; ++q) {
            if (f[q].kind == Param::Kind::TypeSet) { ++nListe; } else { ++nText; }
        }
        const Param& p = f[k];
        const std::string ed = "###editor";
        const std::string feld = befehlsName(*plan, i) + " Feld " + std::to_string(k);
        std::vector<Schritt> neu;
        const TypeSet* ts = (p.kind == Param::Kind::TypeSet)
                                ? g_app->db.typeset(p.typeset)
                                : nullptr;
        if (p.locked) {
            erw[k] = wert(k);
        } else if (ts != nullptr && !ts->entries.empty()) {
            // Ein Eintrag, der noch nicht gewaehlt ist - fest aus Befehl,
            // Feld und Runde, damit jeder Lauf dasselbe tut.
            const std::size_t n = ts->entries.size();
            std::size_t idx = (i * 31U + k * 7U + static_cast<std::size_t>(runde) * 13U) % n;
            if (ts->entries[idx].name == wert(k) && n > 1) { idx = (idx + 1U) % n; }
            const bool gezwungen =
                k == 0 && i < plan->zwang.size() && !plan->zwang[i].empty();
            const std::string ziel = gezwungen ? plan->zwang[i] : ts->entries[idx].name;
            // Gefiltert wird klein geschrieben: die Suche soll Gross- und
            // Kleinschreibung nicht unterscheiden.
            const std::string filter = klein(ziel);
            neu.push_back(klick("##combo", ed, nListe));
            neu.push_back(tippe(filter));
            neu.push_back(pruefe(feld + ": Filter \"" + filter + "\" zeigt nur Passendes",
                                 [=] {
                bool zielDa = false;
                int zeilen = 0;
                for (const Element& e : g_elemente) {
                    if (e.fenster.find("##Combo") == std::string::npos) { continue; }
                    if (e.label.empty() || e.label.rfind("##", 0) == 0) { continue; }
                    if (e.label.find("###eigen") != std::string::npos) { continue; }
                    ++zeilen;
                    if (e.label == ziel) { zielDa = true; }
                    if (klein(e.label).find(filter) == std::string::npos) {
                        diag::detail("Knopftest: Filter liess \"" + e.label + "\" stehen");
                        return false;
                    }
                }
                return zielDa && zeilen > 0 &&
                       static_cast<std::size_t>(zeilen) <= n;
            }));
            neu.push_back(klick(ziel, "##Combo"));
            neu.push_back(pruefe(feld + ": \"" + ziel + "\" gewaehlt",
                                 [=] { return wert(k) == ziel; }));
            neu.push_back(tu(feld + " merken", [=] { plan->erwartet[i][k] = ziel; }));
        } else {
            const std::string w = testwert(p, runde, i, k, istAusdruck(k));
            neu.push_back(klick("##v", ed, nText));
            neu.push_back(tippe(w));
            neu.push_back(pruefe(feld + ": \"" + w + "\" steht im Feld", [=] {
                if (wert(k) != w) {
                    diag::detail("Knopftest: im Feld steht \"" + wert(k) + "\", " +
                                 wasImGuiSieht());
                }
                return wert(k) == w;
            }));
            neu.push_back(tu(feld + " merken", [=] { plan->erwartet[i][k] = w; }));
        }
        neu.push_back(fuelleAb(plan, i, runde, k + 1));
        schritteEinfuegen(std::move(neu));
        return true;
    }};
}

// Alle Befehle wieder oeffnen und jeden Wert mit dem Erwarteten vergleichen.
void pruefeAlle(std::vector<Schritt>& s, std::shared_ptr<BefehlsPlan> plan,
                const std::string& wann) {
    for (std::size_t i = 0; i < plan->befehle.size(); ++i) {
        s.push_back(oeffneBefehl(plan, i));
        s.push_back(pruefe(befehlsName(*plan, i) + ": " + wann, [=] {
            const std::string d = vergleiche(plan->erwartet[i], g_app->editorValues);
            if (!d.empty()) { diag::detail("Knopftest: " + d); }
            return d.empty();
        }));
        s.push_back(klick(tr(Str::EditorCancel), "###editor"));
    }
}

// Speichern ueber den echten Save-Knopf, dann neu laden.
void speichernUndLaden(std::vector<Schritt>& s, std::shared_ptr<BefehlsPlan> plan,
                       const std::string& runde) {
    auto inhalt = std::make_shared<std::string>();
    s.push_back(tu("Pfad setzen", [=] { g_app->path = plan->datei; }));
    s.push_back(klick(tr(Str::FileSave)));
    s.push_back(pruefe("Save (" + runde + "): Datei geschrieben, Skript gilt als gespeichert",
                       [=] {
        *inhalt = slurp(plan->datei);
        return !inhalt->empty() && !g_app->doc.dirty();
    }));
    s.push_back(tu("Datei neu laden", [=] { ladeSkriptDatei(plan->datei); }));
    s.push_back(pruefe("Laden (" + runde + "): Skript byte-gleich mit der Datei",
                       [=] { return writeScript(g_app->doc.script()) == *inhalt; }));
    s.push_back(pruefe("Laden (" + runde + "): alle Befehle da", [=] {
        return befehlsZahl() == plan->befehle.size();
    }));
}

std::vector<Schritt> befehlSchritte() {
    std::vector<Schritt> s;
    auto plan = std::make_shared<BefehlsPlan>();
    for (const Command& c : g_app->db.commands) { plan->befehle.push_back(&c); }
    plan->erwartet.resize(plan->befehle.size());
    plan->datei = platform::executableDirectory() + "\\selbsttest-befehle.txt";
    diag::detail("Knopftest: Befehlstest mit " +
                 std::to_string(plan->befehle.size()) + " Befehlen");

    s.push_back(tu("Leeres Skript fuer den Befehlstest", [] {
        openScriptFromMemory("//Generated by BehavEd\n", "befehle.txt");
        g_app->selectedPath.clear();
        g_app->selection.clear();
    }));
    // Runde 1: einfuegen und fuellen.
    for (std::size_t i = 0; i < plan->befehle.size(); ++i) {
        const Command* c = plan->befehle[i];
        s.push_back(tu("Einfuegen " + befehlsName(*plan, i),
                       [=] { fuegeBefehlEin(*c); }));
        s.push_back(oeffneBefehl(plan, i));
        s.push_back(fuelleAb(plan, i, 0, 0));
        s.push_back(klick(tr(Str::EditorOk), "###editor"));
        s.push_back(pruefe(befehlsName(*plan, i) + ": Ok schreibt alle Werte", [=] {
            Path weg;
            const Node* n = befehlsWeg(i, weg) ? nodeAt(g_app->doc.script(), weg) : nullptr;
            const std::string d = vergleiche(plan->erwartet[i], argTexte(n));
            if (!d.empty()) { diag::detail("Knopftest: " + d); }
            return n != nullptr && n->name == c->name && d.empty();
        }));
    }
    speichernUndLaden(s, plan, "Runde 1");
    pruefeAlle(s, plan, "nach Speichern und Laden alle Werte (Runde 1)");

    // Runde 2: jeden Wert aendern.
    for (std::size_t i = 0; i < plan->befehle.size(); ++i) {
        s.push_back(oeffneBefehl(plan, i));
        s.push_back(fuelleAb(plan, i, 1, 0));
        s.push_back(klick(tr(Str::EditorOk), "###editor"));
        s.push_back(pruefe(befehlsName(*plan, i) + ": Ok schreibt alle geaenderten Werte",
                           [=] {
            Path weg;
            const Node* n = befehlsWeg(i, weg) ? nodeAt(g_app->doc.script(), weg) : nullptr;
            const std::string d = vergleiche(plan->erwartet[i], argTexte(n));
            if (!d.empty()) { diag::detail("Knopftest: " + d); }
            return d.empty();
        }));
    }
    speichernUndLaden(s, plan, "Runde 2");
    pruefeAlle(s, plan, "nach Speichern und Laden alle geaenderten Werte (Runde 2)");

    // Aufraeumen: Testdatei weg, auch aus der Liste der zuletzt geoeffneten.
    s.push_back(tu("Testdatei entfernen", [=] {
        std::remove(plan->datei.c_str());
        auto& r = g_app->settings.recent;
        r.erase(std::remove(r.begin(), r.end(), plan->datei), r.end());
    }));

    // Runde 3: JEDER Eintrag der ersten Klappliste jedes Befehls.
    //
    // Runde 1 und 2 waehlen je Befehl einen Eintrag. `set` hat aber rund
    // dreihundert SET_-Typen, `camera` ein Dutzend Unterbefehle, und jeder
    // bringt eigene Felder mit (Zahl, Text, Vektor, wieder eine Klappliste).
    // Hier wird jeder davon einmal gewaehlt, alle Folgefelder gefuellt,
    // gespeichert, geladen und verglichen.
    auto alle = std::make_shared<BefehlsPlan>();
    for (const Command& c : g_app->db.commands) {
        if (c.params.empty() || c.params[0].kind != Param::Kind::TypeSet ||
            c.params[0].locked) {
            continue;
        }
        const TypeSet* ts = g_app->db.typeset(c.params[0].typeset);
        if (ts == nullptr) { continue; }
        for (const TypeEntry& e : ts->entries) {
            alle->befehle.push_back(&c);
            alle->zwang.push_back(e.name);
        }
    }
    alle->erwartet.resize(alle->befehle.size());
    alle->datei = platform::executableDirectory() + "\\selbsttest-eintraege.txt";
    diag::detail("Knopftest: Eintragstest mit " +
                 std::to_string(alle->befehle.size()) + " Eintraegen");
    s.push_back(tu("Leeres Skript fuer den Eintragstest", [] {
        openScriptFromMemory("//Generated by BehavEd\n", "eintraege.txt");
        g_app->selectedPath.clear();
        g_app->selection.clear();
    }));
    for (std::size_t i = 0; i < alle->befehle.size(); ++i) {
        const Command* c = alle->befehle[i];
        s.push_back(tu("Einfuegen " + befehlsName(*alle, i),
                       [=] { fuegeBefehlEin(*c); }));
        s.push_back(oeffneBefehl(alle, i));
        s.push_back(fuelleAb(alle, i, 0, 0));
        s.push_back(klick(tr(Str::EditorOk), "###editor"));
        s.push_back(pruefe(befehlsName(*alle, i) + ": Ok schreibt alle Werte", [=] {
            Path weg;
            const Node* n = befehlsWeg(i, weg) ? nodeAt(g_app->doc.script(), weg) : nullptr;
            const std::string d = vergleiche(alle->erwartet[i], argTexte(n));
            if (!d.empty()) { diag::detail("Knopftest: " + d); }
            return n != nullptr && d.empty();
        }));
    }
    speichernUndLaden(s, alle, "Eintraege");
    pruefeAlle(s, alle, "nach Speichern und Laden alle Werte (Eintraege)");
    s.push_back(tu("Testdatei entfernen", [=] {
        std::remove(alle->datei.c_str());
        auto& r = g_app->settings.recent;
        r.erase(std::remove(r.begin(), r.end(), alle->datei), r.end());
    }));
    return s;
}

// --- Geteilte Ansicht ------------------------------------------------------
//
// shank: "teste noch split view von scripts, dass die Fenster stabil bleiben
// und dann auch, dass man von einem script zum anderen ... rueber ziehen kann,
// copieren und einfuegen, die Sachen oeffnen kann, die Werte alle richtig
// gespeichert sind."
//
// Zwei eigene Skripte A und B. B ist aktiv und steht im Hauptfeld (Feld 0),
// A im Nebenfeld (Feld 1). Geprueft wird jeweils das SKRIPT beider Reiter,
// nicht das Bild.

const char* const kSplitA =
    "//Generated by BehavEd\n"
    "\n"
    "print ( \"A1\" );\n"
    "wait ( 111.000 );\n"
    "affect ( \"a_ent\", /*@AFFECT_TYPE*/ FLUSH )\n"
    "{\n"
    "\twait ( 112.000 );\n"
    "}\n"
    "\n";

const char* const kSplitB =
    "//Generated by BehavEd\n"
    "\n"
    "print ( \"B1\" );\n"
    "set ( /*@SET_TYPES*/ \"SET_PARM1\", \"b_wert\" );\n";

struct Geteilt {
    int a = -1;           // Reiter von A
    int b = -1;           // Reiter von B
    int c = -1;           // leeres Skript im Hauptfeld
    int d = -1;           // leeres Skript im Nebenfeld
    std::string dateiA;
    std::string dateiB;
    std::string textA;    // was gespeichert werden sollte
    std::string textB;
};

const Script& skriptVon(int tab) {
    if (tab == g_app->activeTab || tab < 0 ||
        tab >= static_cast<int>(g_app->tabs.size())) {
        return g_app->doc.script();
    }
    return g_app->tabs[static_cast<std::size_t>(tab)].doc.script();
}

// Die Befehlsnamen auf oberster Ebene, als "print wait set".
std::string folge(int tab) {
    std::string s;
    for (const Node& n : skriptVon(tab).nodes) {
        if (n.kind != Node::Kind::Command) { continue; }
        if (!s.empty()) { s += ' '; }
        s += n.name;
    }
    return s;
}

std::string erstesArg(int tab, const std::string& befehl) {
    for (const Node& n : skriptVon(tab).nodes) {
        if (n.kind == Node::Kind::Command && n.name == befehl && !n.args.empty()) {
            return n.args[0].text;
        }
    }
    return "<fehlt>";
}

// In einer aufgeklappten Liste waehlen - notfalls erst rollen.
//
// Eine Klappliste zeigt acht Eintraege. Steht der gesuchte weiter unten,
// ist er nicht gezeichnet und damit nicht zu finden; ein Mensch wuerde das
// Mausrad drehen. Genau das: ueber die Liste fahren, drei Rasten nach unten,
// neu suchen. Hoechstens zehnmal.
Schritt waehleInListe(const std::string& label, bool praefix, int versuch = 0,
                      const std::string& fenster = "##Combo") {
    return {"Waehle \"" + label + "\" in der Liste", [=](int b) {
                if (const Element* e = finde(label, fenster, 0, praefix)) {
                    // rc603: steht der Eintrag nur halb im Bild (am Rand der
                    // rollenden Liste), erst ganz hineinrollen - sonst meldet
                    // klick "nicht ganz sichtbar".
                    if (!ganzSichtbar(*e) && b < 8) {
                        if (b == 0) {
                            if (ImGuiWindow* w = ImGui::FindWindowByName(e->innen.c_str())) {
                                const float h = e->rect.GetHeight();
                                float d = 0.0F;
                                if (e->rect.Max.y > w->InnerClipRect.Max.y - h * 0.5F) {
                                    d = e->rect.Max.y - w->InnerClipRect.Max.y + h;
                                } else if (e->rect.Min.y < w->InnerClipRect.Min.y + h * 0.5F) {
                                    d = e->rect.Min.y - w->InnerClipRect.Min.y - h;
                                }
                                ImGui::SetScrollY(w, w->Scroll.y + d);
                            }
                        }
                        return false;
                    }
                    schritteEinfuegen({klick(label, fenster, 0, praefix)});
                    return true;
                }
                if (versuch >= 10) {
                    meldeFehler("Liste: \"" + label + "\" auch nach dem Rollen nicht da");
                    return true;
                }
                if (b == 0) {
                    // rc603: bevorzugt ein Eintrag in einem KINDfenster - dort
                    // rollt die Liste (die Missionsauswahl hat Kopf und Filter
                    // ueber einer eigenen Liste; das Rad ueber dem Filter rollte
                    // nichts).
                    const Element* irgendeins = nullptr;
                    for (const Element& e : g_elemente) {
                        if (e.fenster.find(fenster) == std::string::npos) {
                            continue;
                        }
                        if (irgendeins == nullptr) {
                            irgendeins = &e;
                        }
                        if (e.innen != e.fenster) {
                            irgendeins = &e;
                            break;
                        }
                    }
                    if (irgendeins == nullptr) {
                        meldeFehler("Liste: keine Liste offen fuer \"" + label + "\"");
                        return true;
                    }
                    setzeMaus(irgendeins->rect.GetCenter());
                    return false;
                }
                if (b == 1) {
                    ImGui::GetIO().AddMouseWheelEvent(0.0F, -3.0F);
                    return false;
                }
                if (b < 4) { return false; }
                schritteEinfuegen({waehleInListe(label, praefix, versuch + 1, fenster)});
                return true;
            }};
}

// Menue File -> Split view -> n
void splitMenue(std::vector<Schritt>& s, int n) {
    s.push_back(klick(tr(Str::MenuFile)));
    s.push_back(klick(tr(Str::SplitToggle), "##Menu"));
    s.push_back(klick(std::to_string(n), "##Menu"));
}

std::vector<Schritt> geteiltSchritte() {
    std::vector<Schritt> s;
    auto g = std::make_shared<Geteilt>();
    g->dateiA = platform::executableDirectory() + "\\selbsttest-split_a.txt";
    g->dateiB = platform::executableDirectory() + "\\selbsttest-split_b.txt";

    s.push_back(tu("Skript A oeffnen", [=] {
        openScriptFromMemory(kSplitA, "split_a.txt");
        g->a = g_app->activeTab;
    }));
    s.push_back(tu("Skript B oeffnen", [=] {
        openScriptFromMemory(kSplitB, "split_b.txt");
        g->b = g_app->activeTab;
        diag::detail("Knopftest: A = Reiter " + std::to_string(g->a) +
                     ", B = Reiter " + std::to_string(g->b));
    }));

    // ---- Zwei Felder --------------------------------------------------
    splitMenue(s, 2);
    s.push_back(pruefe("Split 2: zwei Felder, Fokus im ersten, B aktiv", [=] {
        diag::detail("Knopftest: Felder " + std::to_string(g_app->splitCount) +
                     ", Fokus " + std::to_string(g_app->focusPane) + ", aktiv " +
                     std::to_string(g_app->activeTab));
        return g_app->splitCount == 2 && g_app->focusPane == 0 &&
               g_app->activeTab == g->b;
    }));
    s.push_back(stabil("Split 2 nach dem Umschalten"));
    // A ins Nebenfeld holen - ueber die Auswahl oben im Feld.
    s.push_back(klick("##splitpick1", ""));
    s.push_back(waehleInListe("split_a.txt", false));
    s.push_back(pruefe("Nebenfeld zeigt A", [=] { return g_app->splitPanes[1].tab == g->a; }));
    s.push_back(stabil("Split 2 mit A im Nebenfeld"));
    s.push_back(pruefe("Ausgang: A = \"print wait affect\", B = \"print set\"", [=] {
        return folge(g->a) == "print wait affect" && folge(g->b) == "print set";
    }));

    // ---- Ziehen Nebenfeld -> Hauptfeld ---------------------------------
    s.push_back(ziehe("wait", "/pane1_", "print", "/pane0_"));
    s.push_back(pruefe("Ziehen A -> B: wait hinter print in B, A unveraendert", [=] {
        diag::detail("Knopftest: A = " + folge(g->a) + " | B = " + folge(g->b));
        return folge(g->b) == "print wait set" && erstesArg(g->b, "wait") == "111.000" &&
               folge(g->a) == "print wait affect";
    }));
    // ---- Ziehen Hauptfeld -> Nebenfeld ---------------------------------
    s.push_back(ziehe("set", "/pane0_", "print", "/pane1_"));
    s.push_back(pruefe("Ziehen B -> A: set hinter print in A, B unveraendert", [=] {
        diag::detail("Knopftest: A = " + folge(g->a) + " | B = " + folge(g->b));
        return folge(g->a) == "print set wait affect" &&
               erstesArg(g->a, "set") == "SET_PARM1";
    }));
    s.push_back(stabil("Split 2 nach dem Ziehen"));

    // ---- Kopieren im Nebenfeld (Rechtsklick), Einfuegen im Hauptfeld ---
    s.push_back(klick("affect", "/pane1_", 0, false, "", 1));
    s.push_back(klick(tr(Str::ActCopy), "##Popup"));
    s.push_back(klick("print", "/pane0_"));
    s.push_back(klick(tr(Str::ActPaste)));
    s.push_back(pruefe("Kopieren aus A, Einfuegen in B: affect samt Rumpf hinter print", [=] {
        diag::detail("Knopftest: A = " + folge(g->a) + " | B = " + folge(g->b));
        bool rumpf = false;
        for (const Node& n : skriptVon(g->b).nodes) {
            if (n.name == "affect") {
                rumpf = n.hasBlock && n.children.size() == 1 &&
                        n.children[0].name == "wait" && !n.children[0].args.empty() &&
                        n.children[0].args[0].text == "112.000";
            }
        }
        return folge(g->b).rfind("print affect", 0) == 0 && rumpf &&
               erstesArg(g->b, "affect") == "a_ent";
    }));

    // ---- Kopieren im Hauptfeld, Fokuswechsel, Einfuegen in A ----------
    // Erst eine andere Zeile: print ist vom Einfuegen noch ausgewaehlt, und
    // ein zweiter Klick auf eine ausgewaehlte Zeile hebt die Auswahl auf.
    s.push_back(klick("set", "/pane0_"));
    s.push_back(klick("print", "/pane0_"));
    s.push_back(pruefe("print in B ausgewaehlt", [=] {
        const Node* n = nodeAt(g_app->doc.script(), g_app->selectedPath);
        return n != nullptr && n->name == "print";
    }));
    s.push_back(klick(tr(Str::ActCopy)));
    s.push_back(klick("wait", "/pane1_"));   // Klick ins Nebenfeld -> Fokus
    s.push_back(pruefe("Fokuswechsel: Feld 1 hat den Fokus, A ist aktiv", [=] {
        return g_app->focusPane == 1 && g_app->activeTab == g->a;
    }));
    s.push_back(stabil("Split 2 nach dem Fokuswechsel"));
    s.push_back(klick(tr(Str::ActPaste)));
    s.push_back(pruefe("Einfuegen in A: ein zweites print (\"B1\")", [=] {
        diag::detail("Knopftest: A = " + folge(g->a) + " | B = " + folge(g->b));
        int prints = 0;
        bool b1 = false;
        for (const Node& n : skriptVon(g->a).nodes) {
            if (n.name == "print") {
                ++prints;
                if (!n.args.empty() && n.args[0].text == "B1") { b1 = true; }
            }
        }
        return prints == 2 && b1;
    }));
    s.push_back(pruefe("Nach dem Fokuswechsel: B unversehrt im Nebenfeld", [=] {
        return g_app->splitPanes[0].tab == g->b &&
               folge(g->b).rfind("print affect", 0) == 0;
    }));

    // ---- Wert im Editor aendern, im jeweils anderen Skript pruefen -----
    s.push_back(oeffne("wait", 0));
    s.push_back(klick("##v", "###editor"));
    s.push_back(tippe("222.000"));
    s.push_back(klick(tr(Str::EditorOk), "###editor"));
    s.push_back(pruefe("Editor in A: wait = 222.000, B behaelt 111.000", [=] {
        return erstesArg(g->a, "wait") == "222.000" &&
               erstesArg(g->b, "wait") == "111.000";
    }));

    // ---- Speichern mit "Save all", neu laden ---------------------------
    s.push_back(tu("Pfade fuer A und B setzen", [=] {
        for (int tab : {g->a, g->b}) {
            const std::string& d = (tab == g->a) ? g->dateiA : g->dateiB;
            if (tab == g_app->activeTab) {
                g_app->path = d;
            } else {
                g_app->tabs[static_cast<std::size_t>(tab)].path = d;
            }
        }
        g->textA = writeScript(skriptVon(g->a));
        g->textB = writeScript(skriptVon(g->b));
    }));
    s.push_back(klick(tr(Str::ActSaveAll)));
    s.push_back(pruefe("Save all: beide Dateien genau wie die Skripte", [=] {
        return slurp(g->dateiA) == g->textA && slurp(g->dateiB) == g->textB;
    }));
    s.push_back(stabil("Split 2 nach Save all"));
    s.push_back(tu("B neu laden", [=] { ladeSkriptDatei(g->dateiB); }));
    s.push_back(pruefe("B neu geladen: byte-gleich, Werte da", [=] {
        return writeScript(g_app->doc.script()) == g->textB &&
               erstesArg(g_app->activeTab, "wait") == "111.000" &&
               erstesArg(g_app->activeTab, "affect") == "a_ent";
    }));
    s.push_back(oeffne("set", 0));
    s.push_back(pruefe("B neu geladen: set im Editor = SET_PARM1 / b_wert", [] {
        return wert(0) == "SET_PARM1" && wert(1) == "b_wert";
    }));
    s.push_back(klick(tr(Str::EditorCancel), "###editor"));
    s.push_back(tu("A neu laden", [=] { ladeSkriptDatei(g->dateiA); }));
    s.push_back(pruefe("A neu geladen: byte-gleich, wait = 222.000", [=] {
        return writeScript(g_app->doc.script()) == g->textA &&
               erstesArg(g_app->activeTab, "wait") == "222.000";
    }));

    // ---- Freie Flaeche und leere Skripte --------------------------------
    //
    // shank, mit Bildern: "ich kann nichts in ein leeres Feld ziehen ... auch
    // nichts in das leere Feld unter den anderen, dass es darunter
    // eingefuegt wird."
    //
    // Zwei leere Skripte C (Hauptfeld) und D (Nebenfeld), dazu A mit
    // Inhalt. Gezogen wird jeweils auf eine Stelle weit unter der
    // Skriptauswahl des Feldes - dort steht keine Zeile.
    s.push_back(tu("Leeres Skript D oeffnen", [=] {
        openScriptFromMemory("//Generated by BehavEd\n", "split_d.txt");
        g->d = g_app->activeTab;
    }));
    s.push_back(tu("Leeres Skript C oeffnen", [=] {
        openScriptFromMemory("//Generated by BehavEd\n", "split_c.txt");
        g->c = g_app->activeTab;
        diag::detail("Knopftest: C = Reiter " + std::to_string(g->c) +
                     ", D = Reiter " + std::to_string(g->d) + ", Fokus " +
                     std::to_string(g_app->focusPane));
    }));
    // Ein neuer Reiter bringt seinen eigenen, ungeteilten Arbeitsbereich mit
    // (addTab) - also fuer C wieder zwei Felder.
    splitMenue(s, 2);
    s.push_back(klick("##splitpick1", ""));
    s.push_back(waehleInListe("selbsttest-split_a", true));
    s.push_back(pruefe("Hauptfeld zeigt das leere C, Nebenfeld A", [=] {
        return g_app->focusPane == 0 && g_app->activeTab == g->c &&
               g_app->splitPanes[1].tab == g->a && folge(g->c).empty();
    }));
    const ImVec2 tief{0.0F, 250.0F};
    s.push_back(ziehe("wait", "/pane1_", "##splitpick0", "", 0, 0, 0.5F, true, tief));
    s.push_back(pruefe("Ziehen A -> leeres C: wait steht in C", [=] {
        diag::detail("Knopftest: C = " + folge(g->c));
        return folge(g->c) == "wait" && erstesArg(g->c, "wait") == "222.000";
    }));
    s.push_back(ziehe("set", "/pane1_", "##splitpick0", "", 0, 0, 0.5F, true, tief));
    s.push_back(pruefe("Ziehen A -> freie Flaeche unter C: set ans Ende", [=] {
        diag::detail("Knopftest: C = " + folge(g->c));
        return folge(g->c) == "wait set";
    }));
    s.push_back(ziehe("flush  ", "", "##splitpick0", "", 0, 0, 0.5F, true, tief, true));
    s.push_back(pruefe("Ereignisliste -> freie Flaeche unter C: flush ans Ende", [=] {
        diag::detail("Knopftest: C = " + folge(g->c));
        return folge(g->c) == "wait set flush";
    }));
    s.push_back(ziehe("wait", "/pane0_", "##splitpick0", "", 0, 0, 0.5F, true, tief));
    s.push_back(pruefe("Im selben Skript auf die freie Flaeche: wait ans Ende verschoben", [=] {
        diag::detail("Knopftest: C = " + folge(g->c));
        return folge(g->c) == "set flush wait";
    }));
    s.push_back(klick("##splitpick1", ""));
    s.push_back(waehleInListe("split_d.txt", false));
    s.push_back(pruefe("Nebenfeld zeigt das leere D", [=] {
        return g_app->splitPanes[1].tab == g->d && folge(g->d).empty();
    }));
    s.push_back(ziehe("set", "/pane0_", "##splitpick1", "", 0, 0, 0.5F, true, tief));
    s.push_back(pruefe("Ziehen C -> leeres D: set steht in D, C unveraendert", [=] {
        diag::detail("Knopftest: C = " + folge(g->c) + " | D = " + folge(g->d));
        return folge(g->d) == "set" && folge(g->c) == "set flush wait";
    }));
    s.push_back(ziehe("flush  ", "", "##splitpick1", "", 0, 0, 0.5F, true, tief, true));
    s.push_back(pruefe("Ereignisliste -> freie Flaeche unter D: flush ans Ende", [=] {
        diag::detail("Knopftest: D = " + folge(g->d));
        return folge(g->d) == "set flush";
    }));
    s.push_back(ziehe("print  ", "", "set", "/pane1_", 0, 0, 0.5F, true, ImVec2{0.0F, 0.0F}, true));
    s.push_back(pruefe("Ereignisliste -> Zeile im Nebenfeld: print hinter set", [=] {
        diag::detail("Knopftest: D = " + folge(g->d));
        return folge(g->d) == "set print flush";
    }));
    s.push_back(ziehe("wait", "/pane0_", "set", "/pane1_"));
    s.push_back(pruefe("Ziehen C -> Zeile in D: wait hinter set", [=] {
        diag::detail("Knopftest: D = " + folge(g->d));
        return folge(g->d) == "set wait print flush";
    }));
    s.push_back(stabil("Split 2 nach dem Ablegen auf freie Flaechen"));

    // ---- Drei und vier Felder, Teiler ziehen ---------------------------
    splitMenue(s, 3);
    s.push_back(pruefe("Split 3: drei Felder", [] { return g_app->splitCount == 3; }));
    s.push_back(stabil("Split 3 nach dem Umschalten"));
    // Nebenfeld -> Nebenfeld: A in Feld 2 holen, dann aus D (Feld 1)
    // hinueber ziehen - einmal auf eine Zeile, einmal auf die freie Flaeche.
    s.push_back(klick("##splitpick2", ""));
    s.push_back(waehleInListe("selbsttest-split_a", true));
    s.push_back(pruefe("Feld 1 zeigt D, Feld 2 zeigt A", [=] {
        return g_app->splitPanes[1].tab == g->d && g_app->splitPanes[2].tab == g->a;
    }));
    auto vorA = std::make_shared<std::string>();
    s.push_back(tu("A merken", [=] { *vorA = folge(g->a); }));
    s.push_back(ziehe("flush", "/pane1_", "print", "/pane2_"));
    s.push_back(pruefe("Nebenfeld D -> Zeile im Nebenfeld A: flush hinter print", [=] {
        diag::detail("Knopftest: A = " + folge(g->a));
        return folge(g->a).rfind("print flush", 0) == 0;
    }));
    s.push_back(ziehe("print", "/pane1_", "##splitpick2", "", 0, 0, 0.5F, true,
                      ImVec2{0.0F, 250.0F}));
    s.push_back(pruefe("Nebenfeld D -> freie Flaeche in A: print ans Ende", [=] {
        diag::detail("Knopftest: A = " + folge(g->a));
        const std::string f = folge(g->a);
        return f.size() >= 5 && f.compare(f.size() - 5, 5, "print") == 0;
    }));
    s.push_back(stabil("Split 3 nach dem Ziehen zwischen Nebenfeldern"));
    splitMenue(s, 4);
    s.push_back(pruefe("Split 4: vier Felder", [] { return g_app->splitCount == 4; }));
    s.push_back(stabil("Split 4 nach dem Umschalten"));
    auto frac = std::make_shared<float>(0.0F);
    s.push_back(tu("Teilerstand merken", [=] { *frac = g_app->splitFracX; }));
    // In der Luecke greifen: die rechte Haelfte des Griffs liegt unter dem
    // rechten Feld, und dort nimmt das Feld die Maus.
    s.push_back(ziehe("##splitx", "###main", "##splitpick1", "", 0, 0, 0.25F, false));
    s.push_back(pruefe("Teiler ziehen verschiebt die Felder", [=] {
        return std::fabs(g_app->splitFracX - *frac) > 0.01F;
    }));
    s.push_back(stabil("Split 4 nach dem Teilerzug"));
    splitMenue(s, 1);
    s.push_back(pruefe("Zurueck auf ein Feld", [] { return g_app->splitCount == 1; }));
    s.push_back(stabil("Ein Feld nach dem Zurueckschalten"));

    s.push_back(tu("Split-Testdateien entfernen", [=] {
        std::remove(g->dateiA.c_str());
        std::remove(g->dateiB.c_str());
        auto& r = g_app->settings.recent;
        r.erase(std::remove(r.begin(), r.end(), g->dateiA), r.end());
        r.erase(std::remove(r.begin(), r.end(), g->dateiB), r.end());
    }));
    return s;
}

// --- Karte: Mission laden, Layout, Kamera, Zeitleiste ----------------------
//
// shank: "load mission und nimm die mission hier (md_am_sith) ... wenn ich
// vor und zurueck scrolle bewegen sich die Tabs ... wir sollten alles in maps
// jetzt testen, dass alles geht und alles richtig ist, Cameras richtig
// bewegt und rotiert werden usw."
//
// --- Abgleich mit dem Original: jeder Befehl, jedes Feld, jeder Helfer ---
//
// BHED_EDITORTEST=vergleich. Laedt dieselben 27 Befehle, die das Original
// in scratchpad/original/reeval/alle.txt bekommen hat, oeffnet jeden, und
// schreibt je Zustand eine Zeile "Vergleich ..." ins Protokoll: Felder mit
// Art, Wert, Typangabe und Knopf, dazu alle beschrifteten Elemente des
// Fensters. Danach jeden Expr!/Helper-Knopf einzeln (und wieder zurueck).
// Keine Pruefungen - die Zeilen werden neben die des Originals gelegt.
const char* const kVergleich =
    "//Generated by BehavEd\n"
    "rem ( \"comment\" );\n"
    "flush (  );\n"
    "if ( $get( FLOAT, \"SET_HEALTH\")$, $=$, $1$ )\n{\n}\n"
    "else (  )\n{\n}\n"
    "loop ( -1 )\n{\n}\n"
    "affect ( \"npc_kyle1\", /*@AFFECT_TYPE*/ FLUSH )\n{\n}\n"
    "run ( \"areis/intro2\" );\n"
    "wait ( 1000.000 );\n"
    "waitsignal ( \"kyleNav1\" );\n"
    "signal ( \"luke1 look at kyle1\" );\n"
    "sound ( /*@CHANNELS*/ CHAN_VOICE, \"sound/chars/kyle/01kyk006.mp3\" );\n"
    "move ( < 0.000 0.000 0.000 >, < 0.000 0.000 0.000 >, 1000.000 );\n"
    "rotate ( < 0.000 0.000 0.000 >, 4000.000 );\n"
    "use ( \"students_stand\" );\n"
    "kill ( \"break_glass\" );\n"
    "remove ( \"npc_student1\" );\n"
    "print ( \"text\" );\n"
    "declare ( /*@DECLARE_TYPE*/ STRING, \"strParameter2\" );\n"
    "free ( \"spokeWithPrisoners1\" );\n"
    "set ( /*@SET_TYPES*/ \"SET_WALKING\", \"true\" );\n"
    "set ( \"variablename\", \"value\" );\n"
    "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
    "task ( \"kyle1_talk1\" )\n{\n}\n"
    "do ( \"kyle1 line 2\" );\n"
    "wait ( \"kyle1 line 2\" );\n"
    "dowait ( \"kyle1 line 1\" );\n"
    "play ( /*@PLAY_TYPES*/ \"PLAY_ROFF\", \"kor2/roffs/lsend_ceiling01\" );\n";

const char* artHinweis(const Param& p) {
    switch (p.kind) {
        case Param::Kind::String: return "<str>";
        case Param::Kind::Int: return "<int>";
        case Param::Kind::Float: return "<float>";
        case Param::Kind::Vector: return "<vec>";
        case Param::Kind::Range: return "<range>";
        case Param::Kind::TypeSet: return "<enum>";
        default: return "<expr>";
    }
}

std::string beschreibeEditor(const std::string& wann) {
    std::string z = "Vergleich " + wann + ":";
    if (!g_app->editorOpen || g_app->editorCmd == nullptr) { return z + " Editor zu"; }
    const std::vector<Param> f =
        editorFields(*g_app->editorCmd, g_app->db, g_app->editorValues);
    for (std::size_t i = 0; i < f.size(); ++i) {
        const bool ex = istAusdruck(i);
        z += " [" + std::to_string(i) + " " +
             (f[i].kind == Param::Kind::TypeSet ? "Combo" : "Edit") + " '" + wert(i) +
             "' " + (ex ? "<expr>" : artHinweis(f[i])) + "]";
    }
    // Vorbelegung der Helferzeilen je Feld im Ausdrucksmodus.
    for (std::size_t i = 0; i < f.size() && i < g_app->helpGetType.size(); ++i) {
        if (!istAusdruck(i)) { continue; }
        char zahl[64];
        std::snprintf(zahl, sizeof(zahl), "%g,%g", static_cast<double>(g_app->helpRangeLow[i]),
                      static_cast<double>(g_app->helpRangeHigh[i]));
        z += " H" + std::to_string(i) + "(" + g_app->helpGetType[i] + "," +
             g_app->helpGetName[i] + "," + g_app->helpTagType[i] + "," + zahl + ")";
    }
    z += " |";
    for (const Element& e : g_elemente) {
        if (e.label.empty() || e.fenster.find("###editor") == std::string::npos) { continue; }
        const std::string s = sichtbarerText(e.label.c_str());
        z += " " + (s.empty() ? e.label : s);
    }
    return z;
}

// Ein Knoten des Vergleichs: Weg und lesbarer Name ("set#3", "walkOnly/2").
struct VergleichsZiel {
    Path weg;
    std::string name;
};

void sammleZiele(const std::vector<Node>& ns, Path& weg, const std::string& vor,
                 std::vector<VergleichsZiel>& out) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        const Node& n = ns[i];
        if (n.kind != Node::Kind::Command && n.kind != Node::Kind::Macro) { continue; }
        weg.push_back(i);
        const std::string name = vor + (n.kind == Node::Kind::Macro ? "[" + n.name + "]" : n.name) +
                                 "@" + std::to_string(i);
        out.push_back({weg, name});
        sammleZiele(n.children, weg, name + "/", out);
        weg.pop_back();
    }
}

// Den Editor fuer einen WEG oeffnen - auch fuer Zeilen in Makros und fuer
// Makrokoepfe (dort oeffnet das Original keinen Editor).
Schritt oeffneWeg(const Path& weg, const std::string& name) {
    return {"Oeffne " + name, [=](int b) {
                if (b == 0) {
                    openEditorForNode(weg);
                    return false;
                }
                if (!g_app->editorOpen) {
                    if (b > 10) {
                        diag::detail("Vergleich " + name + " AUF: kein Editor");
                        return true;
                    }
                    return false;
                }
                ImGuiWindow* w = ImGui::FindWindowByName("###editor");
                return w != nullptr && w->Active && !w->Hidden && b >= 3;
            }};
}

std::vector<Schritt> vergleichSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // BHED_VERGLEICHSDATEI: ein Skript, das das ORIGINAL gespeichert hat
    // (mit allen Makros). Sonst die 27 Befehle von oben.
    const char* datei = std::getenv("BHED_VERGLEICHSDATEI");
    const std::string pfad = datei != nullptr ? datei : "";
    add(tu("Vergleichsskript laden", [pfad] {
        if (pfad.empty()) {
            openScriptFromMemory(kVergleich, "vergleich.txt");
        } else {
            ladeSkriptDatei(pfad);
        }
    }));
    add({"Ziele sammeln", [](int b) {
             if (b < 3) { return false; }
             std::vector<VergleichsZiel> ziele;
             Path weg;
             sammleZiele(g_app->doc.script().nodes, weg, "", ziele);
             diag::detail("Vergleich: " + std::to_string(ziele.size()) + " Zeilen");
             std::vector<Schritt> neu;
             for (const VergleichsZiel& z : ziele) {
                 const std::string name = z.name;
                 neu.push_back(oeffneWeg(z.weg, name));
                 neu.push_back({"Beschreibe " + name, [name](int) {
                     if (!g_app->editorOpen || g_app->editorCmd == nullptr) { return true; }
                     diag::detail(beschreibeEditor(name + " AUF"));
                     // Je Feld mit Knopf: umschalten, beschreiben, zurueck.
                     std::vector<Schritt> knoepfe;
                     const std::vector<Param> f =
                         editorFields(*g_app->editorCmd, g_app->db, g_app->editorValues);
                     for (std::size_t i = 0; i < f.size(); ++i) {
                         if (feldKnopf(*g_app->editorCmd, i) == FeldKnopf::Keiner) { continue; }
                         const std::string k = name + " Feld " + std::to_string(i);
                         knoepfe.push_back(tu("Knopf " + k, [i, n2 = f.size()] {
                             if (g_app->editorIsExpr.size() < n2) {
                                 g_app->editorIsExpr.resize(n2, 0);
                             }
                             g_app->editorIsExpr[i] = 1;
                         }));
                         knoepfe.push_back({"Beschreibe " + k, [k](int) {
                                                diag::detail(beschreibeEditor(k + " KNOPF"));
                                                return true;
                                            }});
                         knoepfe.push_back(
                             tu("Zurueck " + k, [i] { g_app->editorIsExpr[i] = 0; }));
                     }
                     knoepfe.push_back({"Schliessen " + name, [](int b2) {
                         if (b2 == 0) { g_app->editorOpen = false; }
                         return b2 >= 3;
                     }});
                     schritteEinfuegen(std::move(knoepfe));
                     return true;
                 }});
             }
             schritteEinfuegen(std::move(neu));
             return true;
         }});
    return s;
}

// --- Ziehen in LANGEN Listen: die Ansicht bleibt, das Gezogene ist markiert
//
// shank, 27.09.: "Dragging a command scrolls the view up ... The view should
// not scroll and the command(s) I moved should probably be highlighted."
// Nur bei langen Listen - bei kurzen gibt es nichts zu rollen. Und: "du
// musst das auch testen, wenn split view an ist".
//
// Allein aufrufbar mit BHED_EDITORTEST=ziehen.
std::string langesSkript(int basis = 1000) {
    std::string s = "//Generated by BehavEd\n";
    char z[64];
    for (int k = 1; k <= 150; ++k) {
        std::snprintf(z, sizeof(z), "wait ( %d.000 );\n", basis + k);
        s += z;
    }
    return s;
}

struct ZiehProbe {
    std::string fenster;          // ImGui-Name des Baumfensters
    float scrollVorher = 0.0F;
    float abweichung = 0.0F;      // groesste Abweichung nach dem Ablegen
    float nachbarVorher = 0.0F;   // Rollposition des anderen Feldes
    float nachbarAbweichung = 0.0F;
    std::string nachbarFenster;
    std::vector<Kennung> erwartet;
    std::vector<Path> wegeVorher;
    ImVec2 von{}, nach{};
    bool ok = false;
};

ImGuiWindow* baumFenster(const std::string& teil) {
    for (const Element& e : g_elemente) {
        if (e.innen.find(teil) != std::string::npos && e.label == "wait") {
            return ImGui::FindWindowByName(e.innen.c_str());
        }
    }
    return nullptr;
}

// Eine halbe Sekunde nichts tun. Zwei Zuege kurz hintereinander an
// derselben Stelle wertet ImGui als Doppelklick - und der oeffnet den
// Editor, der dann jeden weiteren Klick abfaengt.
Schritt pause(double sekunden) {
    auto bis = std::make_shared<double>(-1.0);
    return {"Pause", [=](int b) {
                if (b == 0) { *bis = ImGui::GetTime() + sekunden; }
                return ImGui::GetTime() >= *bis;
            }};
}

// Das Baumfenster auf einen Anteil seines Rollbereichs stellen (1 = unten).
Schritt rolleBaum(const std::string& teil, float anteil) {
    return {"Rolle " + teil + " auf " + std::to_string(anteil), [=](int b) {
                ImGuiWindow* w = baumFenster(teil);
                if (w == nullptr) {
                    if (b > 20) { meldeFehler("Rolle: Baumfenster " + teil + " nicht gefunden"); return true; }
                    return false;
                }
                if (b < 3) { ImGui::SetScrollY(w, w->ScrollMax.y * anteil); return false; }
                return b >= 12;
            }};
}

// Eine sichtbare "wait"-Zeile auf eine andere ziehen - gezaehlt von unten
// im sichtbaren Bereich. Danach 40 Bilder lang die Rollposition messen.
Schritt zieheSichtbar(std::shared_ptr<ZiehProbe> p, const std::string& teil,
                      int vonAbUnten, int nachAbUnten, const std::string& nachbar = "") {
    const std::string was = "Ziehe in " + teil + " Zeile -" + std::to_string(vonAbUnten) +
                            " auf -" + std::to_string(nachAbUnten);
    return {was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    std::vector<const Element*> sicht;
                    for (const Element& e : g_elemente) {
                        if (e.label != "wait" || e.innen.find(teil) == std::string::npos) { continue; }
                        if (e.rect.Min.y >= e.clip.Min.y && e.rect.Max.y <= e.clip.Max.y) {
                            sicht.push_back(&e);
                        }
                    }
                    std::sort(sicht.begin(), sicht.end(), [](const Element* a, const Element* c) {
                        return a->rect.Min.y < c->rect.Min.y;
                    });
                    const int n = static_cast<int>(sicht.size());
                    if (n <= std::max(vonAbUnten, nachAbUnten)) {
                        meldeFehler(was + ": zu wenig sichtbare Zeilen (" + std::to_string(n) + ")");
                        p->ok = false;
                        return true;
                    }
                    const Element* q = sicht[static_cast<std::size_t>(n - 1 - vonAbUnten)];
                    const Element* z = sicht[static_cast<std::size_t>(n - 1 - nachAbUnten)];
                    p->fenster = q->innen;
                    p->von = ImVec2{q->rect.Min.x + 10.0F, q->rect.GetCenter().y};
                    p->nach = ImVec2{z->rect.Min.x + 10.0F, z->rect.GetCenter().y};
                    ImGuiWindow* w = ImGui::FindWindowByName(p->fenster.c_str());
                    p->scrollVorher = (w != nullptr) ? w->Scroll.y : 0.0F;
                    p->abweichung = 0.0F;
                    p->nachbarAbweichung = 0.0F;
                    p->nachbarFenster.clear();
                    if (!nachbar.empty()) {
                        if (ImGuiWindow* nw = baumFenster(nachbar)) {
                            p->nachbarFenster = nw->Name;
                            p->nachbarVorher = nw->Scroll.y;
                        }
                    }
                    p->ok = true;
                    setzeMaus(p->von);
                    return false;
                }
                if (!p->ok) { return true; }
                if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 2 && b <= 6) {
                    setzeMaus(ImVec2{p->von.x + 3.0F * static_cast<float>(b - 1),
                                     p->von.y - 2.0F * static_cast<float>(b - 1)});
                    return false;
                }
                if (b == 7) {
                    // Was gezogen wird: die Mehrfachauswahl oder die eine Zeile.
                    const std::vector<Path> m =
                        g_app->selection.size() > 1 ? g_app->selection
                                                    : std::vector<Path>{g_app->dragFrom};
                    p->erwartet = kennungenVon(m);
                    p->wegeVorher = m;
                }
                if (b >= 7 && b <= 12) {
                    const float f = static_cast<float>(b - 6) / 6.0F;
                    setzeMaus(ImVec2{p->von.x + (p->nach.x - p->von.x) * f,
                                     p->von.y + (p->nach.y - p->von.y) * f});
                    return false;
                }
                if (b == 13) {
                    if (!GImGui->DragDropActive) {
                        meldeFehler(was + ": kein Zug aktiv vor dem Loslassen (" + wasImGuiSieht() + ")");
                    }
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                // Jedes Bild nach dem Loslassen: steht die Ansicht?
                if (ImGuiWindow* w = ImGui::FindWindowByName(p->fenster.c_str())) {
                    p->abweichung = std::max(p->abweichung, std::fabs(w->Scroll.y - p->scrollVorher));
                }
                if (!p->nachbarFenster.empty()) {
                    if (ImGuiWindow* nw = ImGui::FindWindowByName(p->nachbarFenster.c_str())) {
                        p->nachbarAbweichung =
                            std::max(p->nachbarAbweichung, std::fabs(nw->Scroll.y - p->nachbarVorher));
                    }
                }
                return b >= 55;
            }};
}

void pruefeZug(std::vector<Schritt>& s, std::shared_ptr<ZiehProbe> p, const std::string& wo,
               std::size_t anzahl, bool nachbar = false) {
    s.push_back(pruefe(wo + ": Ansicht springt nicht (Rollposition bleibt)", [=] {
        char z[160];
        std::snprintf(z, sizeof(z), "Knopftest: %s Rollposition vorher %.0f, groesste Abweichung %.1f",
                      wo.c_str(), static_cast<double>(p->scrollVorher), static_cast<double>(p->abweichung));
        diag::detail(z);
        return p->ok && p->scrollVorher > 1.0F && p->abweichung < 1.0F;
    }));
    s.push_back(pruefe(wo + ": das Gezogene ist markiert (" + std::to_string(anzahl) + ")", [=] {
        if (p->erwartet.size() != anzahl || g_app->selection.size() != anzahl) {
            diag::detail("Knopftest: erwartet " + std::to_string(p->erwartet.size()) +
                         ", markiert " + std::to_string(g_app->selection.size()));
            return false;
        }
        const std::vector<Kennung> jetzt = kennungenVon(g_app->selection);
        for (const Kennung k : p->erwartet) {
            if (std::find(jetzt.begin(), jetzt.end(), k) == jetzt.end()) { return false; }
        }
        return true;
    }));
    s.push_back(pruefe(wo + ": und es ist wirklich verschoben", [=] {
        return !g_app->selection.empty() && !p->wegeVorher.empty() &&
               g_app->selection.front() != p->wegeVorher.front();
    }));
    if (nachbar) {
        s.push_back(pruefe(wo + ": das andere Feld springt auch nicht", [=] {
            char z[160];
            std::snprintf(z, sizeof(z), "Knopftest: Nachbarfeld %s Abweichung %.1f",
                          p->nachbarFenster.c_str(), static_cast<double>(p->nachbarAbweichung));
            diag::detail(z);
            return !p->nachbarFenster.empty() && p->nachbarAbweichung < 1.0F;
        }));
    }
}

// Die drei untersten Zeilen des Skripts markieren (bei ganz unten gerollt
// sichtbar).
Schritt markiereUntereDrei() {
    return tu("Die drei untersten Zeilen markieren", [] {
        const std::size_t n = g_app->doc.script().nodes.size();
        g_app->selection = {Path{n - 3}, Path{n - 2}, Path{n - 1}};
        g_app->selectedPath = Path{n - 2};
        rebuildTree();
    });
}

std::vector<Schritt> ziehSchritte() {
    std::vector<Schritt> s;
    auto p = std::make_shared<ZiehProbe>();
    auto tab = std::make_shared<int>(-1);
    // Zwei verschiedene lange Skripte: das Nebenfeld zeigt das zweite. Ein
    // Reiter in zwei Feldern ist kein vorgesehener Zustand (das Programm
    // meldet ihn selbst als Fehler).
    s.push_back(tu("Zweites langes Skript oeffnen (fuer das Nebenfeld)", [=] {
        openScriptFromMemory(langesSkript(5000).c_str(), "lang_b.txt");
        *tab = g_app->activeTab;
    }));
    s.push_back(tu("Langes Skript oeffnen (150 Zeilen)", [=] {
        openScriptFromMemory(langesSkript().c_str(), "lang.txt");
        g_app->selection.clear();
        g_app->selectedPath.clear();
    }));
    s.push_back(pause(0.4));

    // ---- Eine Ansicht ------------------------------------------------
    s.push_back(rolleBaum("/pane0_", 1.0F));
    s.push_back(pause(0.5));
    s.push_back(zieheSichtbar(p, "/pane0_", 1, 5));
    pruefeZug(s, p, "Ganz unten, eine Zeile", 1);
    s.push_back(rolleBaum("/pane0_", 0.5F));
    s.push_back(pause(0.5));
    s.push_back(zieheSichtbar(p, "/pane0_", 2, 8));
    pruefeZug(s, p, "Mitte, eine Zeile", 1);
    s.push_back(rolleBaum("/pane0_", 1.0F));
    s.push_back(markiereUntereDrei());
    s.push_back(pause(0.5));
    s.push_back(zieheSichtbar(p, "/pane0_", 1, 7));
    pruefeZug(s, p, "Ganz unten, drei Zeilen", 3);
    s.push_back(klick(tr(Str::EditUndo)));
    s.push_back(klick(tr(Str::EditUndo)));
    s.push_back(klick(tr(Str::EditUndo)));

    // ---- Geteilte Ansicht: beide Felder zeigen das lange Skript ---------
    splitMenue(s, 2);
    s.push_back(tu("Nebenfeld zeigt das zweite lange Skript, Teiler in die Mitte", [=] {
        g_app->splitPanes[1].tab = *tab;
        // Im Gesamtlauf hat der Split-Test den Teiler verschoben; ein zu
        // schmales Feld zeigt nur Symbole ohne Namen.
        diag::detail("Knopftest: Teiler stand bei " + std::to_string(g_app->splitFracX));
        g_app->splitFracX = 0.5F;
        g_app->splitFracY = 0.5F;
    }));
    s.push_back(pause(0.4));
    s.push_back(rolleBaum("/pane0_", 1.0F));
    s.push_back(rolleBaum("/pane1_", 0.6F));
    s.push_back(pause(0.5));
    s.push_back(zieheSichtbar(p, "/pane0_", 1, 5, "/pane1_"));
    pruefeZug(s, p, "Split, Feld 1, eine Zeile", 1, true);
    s.push_back(markiereUntereDrei());
    s.push_back(pause(0.5));
    s.push_back(zieheSichtbar(p, "/pane0_", 1, 6, "/pane1_"));
    pruefeZug(s, p, "Split, Feld 1, drei Zeilen", 3, true);
    // Fokus ins zweite Feld: dort ziehen, das erste darf nicht springen.
    s.push_back(tu("Fokus ins zweite Feld", [] { g_app->focusRequest = 1; }));
    s.push_back(pause(0.4));
    s.push_back(rolleBaum("/pane1_", 1.0F));
    s.push_back(pause(0.5));
    s.push_back(zieheSichtbar(p, "/pane1_", 1, 5, "/pane0_"));
    pruefeZug(s, p, "Split, Feld 2, eine Zeile", 1, true);
    s.push_back(tu("Fokus zurueck ins erste Feld", [] { g_app->focusRequest = 0; }));
    s.push_back(pause(0.4));
    splitMenue(s, 1);
    s.push_back(pause(0.4));
    return s;
}

// --- Breit: mehrere Reiter, Bloecke, Klick-dann-Ziehen, Reiter schliessen --
//
// shank, 27.09.: "beim allersten mal ziehen springt er einmal danach ist es
// gut - du musst testen an mehreren Stellen und in mehreren Tabs ob es
// irgendwann passiert". Ursache war ein liegengebliebenes "zur Auswahl
// rollen" aus dem Reiterwechsel: der erste KLICK auf eine Zeile zog sie in
// die Mitte. Der erste Ziehtest hat das nicht gesehen, weil er ohne Klick
// zog. Hier wird deshalb wie von Hand gearbeitet: anklicken, kurz warten,
// ziehen - und ab dem Mausdruck jedes Bild gemessen.
std::string realesSkript(int basis) {
    std::string s = "//Generated by BehavEd\n";
    char z[160];
    auto waits = [&](int von, int bis) {
        for (int k = von; k <= bis; ++k) {
            std::snprintf(z, sizeof(z), "wait ( %d.000 );\n", basis + k);
            s += z;
        }
    };
    waits(1, 30);
    s += "affect ( \"blockA\", /*@AFFECT_TYPE*/ FLUSH )\n{\n";
    for (int j = 1; j <= 20; ++j) {
        std::snprintf(z, sizeof(z), "\tset ( /*@SET_TYPES*/ \"SET_PARM1\", \"a%d\" );\n", j);
        s += z;
    }
    s += "}\n\n";
    waits(31, 60);
    s += "affect ( \"blockB\", /*@AFFECT_TYPE*/ FLUSH )\n{\n";
    for (int j = 1; j <= 20; ++j) {
        std::snprintf(z, sizeof(z),
                      "\tcamera ( /*@CAMERA_COMMANDS*/ MOVE, < %d.000 0.000 0.000 >, 0 );\n", j);
        s += z;
    }
    s += "}\n\n";
    waits(61, 90);
    s += "//$\"walkOnly\"@5\n"
         "set ( /*!*/ \"SET_BEHAVIOR_STATE\", /*!*/ \"BS_DEFAULT\" );\n"
         "set ( /*!*/ \"SET_WALKING\", /*!*/ \"true\" );\n"
         "set ( /*!*/ \"SET_RUNNING\", /*!*/ \"false\" );\n"
         "set ( /*!*/ \"SET_IGNOREALERTS\", /*!*/ \"true\" );\n"
         "set ( /*!*/ \"SET_LOOK_FOR_ENEMIES\", /*!*/ \"false\" );\n";
    waits(91, 120);
    return s;
}

// Eine sichtbare Zeile: Name und die wievielte von oben (>= 0) bzw. von
// unten (-1 = die letzte).
struct Wahl {
    std::string name;
    int nte = 0;
};

const Element* sichtbareZeile(const std::string& teil, const Wahl& w) {
    std::vector<const Element*> v;
    for (const Element& e : g_elemente) {
        if (e.label != w.name || e.innen.find(teil) == std::string::npos) { continue; }
        if (e.rect.Min.y >= e.clip.Min.y && e.rect.Max.y <= e.clip.Max.y) { v.push_back(&e); }
    }
    std::sort(v.begin(), v.end(),
              [](const Element* a, const Element* b) { return a->rect.Min.y < b->rect.Min.y; });
    const int n = static_cast<int>(v.size());
    const int i = w.nte >= 0 ? w.nte : n + w.nte;
    return (i >= 0 && i < n) ? v[static_cast<std::size_t>(i)] : nullptr;
}

// Die `nte` Zeile dieses Namens in g_app->rows auf `anteil` der Fensterhoehe
// rollen (0 = oben, 0.5 = Mitte).
Schritt rolleZuZeile(const std::string& teil, const std::string& name, int nte, float anteil) {
    return {"Rolle " + teil + " zu " + name + " #" + std::to_string(nte), [=](int b) {
                ImGuiWindow* w = baumFenster(teil);
                if (w == nullptr) {
                    if (b > 30) { meldeFehler("Rolle: Baumfenster " + teil + " fehlt"); return true; }
                    return false;
                }
                if (b < 3) {
                    int gez = 0;
                    int zeile = -1;
                    for (std::size_t i = 0; i < g_app->rows.size(); ++i) {
                        if (g_app->rows[i].name == name && gez++ == nte) { zeile = static_cast<int>(i); break; }
                    }
                    // Zeilenschritt aus zwei sichtbaren Zeilen
                    float schritt = ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y;
                    const float y = static_cast<float>(zeile) * schritt -
                                    w->InnerRect.GetHeight() * anteil;
                    ImGui::SetScrollY(w, std::clamp(y, 0.0F, w->ScrollMax.y));
                    return false;
                }
                return b >= 12;
            }};
}

struct Handlung {
    std::string fenster;
    float start = 0.0F;
    float abweichung = 0.0F;
    std::vector<Kennung> erwartet;
    std::vector<Path> wegeVorher;
    ImVec2 von{}, nach{};
    int phase = 0;
    int phaseBild = 0;
    double warteBis = 0.0;
    bool ok = false;
    // Die Action-Leiste: Breite des Delete-Knopfs und ob die Spalte eine
    // Rollleiste hat. shank, 27.09.: "manchmal beim verschieben springt die
    // action leiste und wird groesser".
    float leisteMin = 1e9F, leisteMax = 0.0F;
    int rollleisteAn = 0, rollleisteAus = 0;
    // Der UNTERSTE Knopf: rutscht etwas in der Leiste nach unten (eine
    // eingeschobene Zeile), sieht man es nur hier - Delete steht oben.
    float exitYMin = 1e9F, exitYMax = -1e9F;
    // Verliert das Fenster waehrend der Handlung den Fokus (Klick in ein
    // anderes Programm), setzt ImGui die Maustasten zurueck und der Zug
    // bricht ab - von aussen, nicht durch behaved. Dann wird wiederholt.
    bool fokusWeg = false;
    int wiederholt = 0;
};

void messeLeiste(Handlung& h) {
    if (const Element* e = finde(tr(Str::ActDelete), "buttons", 0, false)) {
        h.leisteMin = std::min(h.leisteMin, e->rect.GetWidth());
        h.leisteMax = std::max(h.leisteMax, e->rect.GetWidth());
        if (ImGuiWindow* w = ImGui::FindWindowByName(e->innen.c_str())) {
            (w->ScrollbarY ? h.rollleisteAn : h.rollleisteAus)++;
        }
    }
    if (const Element* e = finde(tr(Str::AppExit), "buttons", 0, false)) {
        h.exitYMin = std::min(h.exitYMin, e->rect.Min.y);
        h.exitYMax = std::max(h.exitYMax, e->rect.Min.y);
    }
}

// Anklicken (optional), warten, ziehen (optional). Gemessen wird ab dem
// ERSTEN Mausdruck bis 40 Bilder nach dem Loslassen.
Schritt handlung(std::shared_ptr<Handlung> h, const std::string& teil, Wahl quelle,
                 Wahl ziel, bool klickZuerst, bool ziehen, const std::string& was) {
    return {was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                auto messen = [&] {
                    if (ImGuiWindow* w = ImGui::FindWindowByName(h->fenster.c_str())) {
                        h->abweichung = std::max(h->abweichung, std::fabs(w->Scroll.y - h->start));
                    }
                    messeLeiste(*h);
                };
                if (b == 0) {
                    const Element* q = sichtbareZeile(teil, quelle);
                    const Element* z = ziehen ? sichtbareZeile(teil, ziel) : q;
                    if (q == nullptr || z == nullptr) {
                        meldeFehler(was + ": Zeile nicht sichtbar (" + quelle.name + "/" + ziel.name + ")");
                        h->ok = false;
                        return true;
                    }
                    h->fenster = q->innen;
                    h->von = ImVec2{q->rect.Min.x + 12.0F, q->rect.GetCenter().y};
                    h->nach = ImVec2{z->rect.Min.x + 12.0F, z->rect.GetCenter().y};
                    ImGuiWindow* w = ImGui::FindWindowByName(h->fenster.c_str());
                    h->start = (w != nullptr) ? w->Scroll.y : 0.0F;
                    h->abweichung = 0.0F;
                    h->leisteMin = 1e9F;
                    h->leisteMax = 0.0F;
                    h->rollleisteAn = 0;
                    h->rollleisteAus = 0;
                    h->exitYMin = 1e9F;
                    h->exitYMax = -1e9F;
                    h->fokusWeg = false;
                    messeLeiste(*h);
                    h->erwartet.clear();
                    h->wegeVorher.clear();
                    h->phase = klickZuerst ? 0 : 2;
                    h->phaseBild = 0;
                    h->ok = true;
                    setzeMaus(h->von);
                    return false;
                }
                if (!h->ok) { return true; }
                messen();
                if (ImGui::GetIO().AppFocusLost) { h->fokusWeg = true; }
                const int pb = h->phaseBild++;
                switch (h->phase) {
                    case 0:   // Klick
                        if (pb == 0) { io.AddMouseButtonEvent(0, true); return false; }
                        if (pb == 1) { io.AddMouseButtonEvent(0, false); return false; }
                        h->phase = 1;
                        h->phaseBild = 0;
                        h->warteBis = ImGui::GetTime() + 0.45;   // kein Doppelklick
                        return false;
                    case 1:   // warten, dabei messen
                        if (ImGui::GetTime() < h->warteBis) { return false; }
                        if (!ziehen) { h->phase = 4; h->phaseBild = 0; return false; }
                        h->phase = 2;
                        h->phaseBild = 0;
                        setzeMaus(h->von);
                        return false;
                    case 2:   // Ziehen
                        if (pb == 0) { setzeMaus(h->von); return false; }
                        if (pb == 1) { io.AddMouseButtonEvent(0, true); return false; }
                        if (pb == 2 || pb == 4) {
                            char z[260];
                            std::snprintf(z, sizeof(z),
                                          "Knopftest: Druck Bild %d - Maus ImGui %.0f/%.0f, soll %.0f/%.0f, "
                                          "Taste %d, %s",
                                          pb, static_cast<double>(io.MousePos.x),
                                          static_cast<double>(io.MousePos.y),
                                          static_cast<double>(h->von.x), static_cast<double>(h->von.y),
                                          io.MouseDown[0] ? 1 : 0, wasImGuiSieht().c_str());
                            diag::detail(z);
                        }
                        if (pb >= 2 && pb <= 6) {
                            setzeMaus(ImVec2{h->von.x + 3.0F * static_cast<float>(pb - 1),
                                             h->von.y + ((h->nach.y > h->von.y) ? 2.0F : -2.0F) *
                                                            static_cast<float>(pb - 1)});
                            return false;
                        }
                        if (pb == 7) {
                            const std::vector<Path> m =
                                g_app->selection.size() > 1 ? g_app->selection
                                                            : std::vector<Path>{g_app->dragFrom};
                            h->erwartet = kennungenVon(m);
                            h->wegeVorher = m;
                        }
                        if (pb >= 7 && pb <= 14) {
                            const float f = static_cast<float>(pb - 6) / 8.0F;
                            setzeMaus(ImVec2{h->von.x + (h->nach.x - h->von.x) * f,
                                             h->von.y + (h->nach.y - h->von.y) * f});
                            return false;
                        }
                        if (!GImGui->DragDropActive) {
                            meldeFehler(was + ": kein Zug aktiv (" + wasImGuiSieht() + ")");
                        }
                        io.AddMouseButtonEvent(0, false);
                        h->phase = 4;
                        h->phaseBild = 0;
                        return false;
                    default:  // nachmessen
                        if (pb < 40) { return false; }
                        if (h->fokusWeg && h->wiederholt < 3) {
                            // Ein abgebrochener Zug hat nichts verschoben -
                            // dieselbe Handlung einfach noch einmal.
                            ++h->wiederholt;
                            diag::detail("Knopftest: " + was +
                                         " - Fenster verlor den Fokus, Handlung wird wiederholt");
                            h->ok = false;
                            schritteEinfuegen({pause(0.5),
                                               handlung(h, teil, quelle, ziel, klickZuerst, ziehen, was)});
                        } else {
                            h->wiederholt = 0;
                        }
                        return true;
                }
            }};
}

void pruefeHandlung(std::vector<Schritt>& s, std::shared_ptr<Handlung> h, const std::string& was,
                    bool verschoben) {
    s.push_back(pruefe(was + ": Ansicht springt nicht", [=] {
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: %s - Rollposition %.0f, groesste Abweichung %.1f",
                      was.c_str(), static_cast<double>(h->start), static_cast<double>(h->abweichung));
        diag::detail(z);
        return h->ok && h->abweichung < 1.0F;
    }));
    s.push_back(pruefe(was + ": Action-Leiste bleibt gleich breit", [=] {
        char z[200];
        std::snprintf(z, sizeof(z),
                      "Knopftest: %s - Delete-Knopf %.0f..%.0f breit, Exit bei y %.0f..%.0f, "
                      "Rollleiste an %d / aus %d Bilder",
                      was.c_str(), static_cast<double>(h->leisteMin),
                      static_cast<double>(h->leisteMax), static_cast<double>(h->exitYMin),
                      static_cast<double>(h->exitYMax), h->rollleisteAn, h->rollleisteAus);
        diag::detail(z);
        return h->leisteMax > 0.0F && h->leisteMax - h->leisteMin < 0.5F &&
               h->exitYMax - h->exitYMin < 0.5F &&
               (h->rollleisteAn == 0 || h->rollleisteAus == 0);
    }));
    if (verschoben) {
        s.push_back(pruefe(was + ": das Gezogene ist markiert und verschoben", [=] {
            if (h->erwartet.empty() || g_app->selection.size() != h->erwartet.size()) { return false; }
            const std::vector<Kennung> jetzt = kennungenVon(g_app->selection);
            for (const Kennung k : h->erwartet) {
                if (std::find(jetzt.begin(), jetzt.end(), k) == jetzt.end()) { return false; }
            }
            return g_app->selection.front() != h->wegeVorher.front();
        }));
    }
}

// Rollposition des Reiters merken / nach dem Wechsel pruefen.
struct ReiterRolle {
    int tab = -1;
    int uid = -1;
    float scroll = 0.0F;
};

Schritt merkeRolle(std::shared_ptr<ReiterRolle> r, const std::string& teil) {
    return tu("Rollposition merken", [=] {
        r->tab = g_app->activeTab;
        r->uid = tabKennung(g_app->activeTab);
        ImGuiWindow* w = baumFenster(teil);
        r->scroll = (w != nullptr) ? w->Scroll.y : -1.0F;
        diag::detail("Knopftest: Reiter " + std::to_string(r->tab) + " (uid " +
                     std::to_string(r->uid) + ") steht bei " + std::to_string(r->scroll));
    });
}

// Zum Reiter mit dieser festen Kennung wechseln und pruefen, dass er seine
// eigene Rollposition zurueckbekommt und 30 Bilder lang behaelt.
void wechsleUndPruefe(std::vector<Schritt>& s, std::shared_ptr<ReiterRolle> r,
                      const std::string& teil, const std::string& was) {
    auto maxAbw = std::make_shared<float>(0.0F);
    s.push_back(tu("Wechsel zu Reiter uid " + was, [=] {
        for (int i = 0; i < static_cast<int>(g_app->tabs.size()); ++i) {
            if (tabKennung(i) == r->uid) { g_app->tabRequest = i; }
        }
    }));
    s.push_back({"Rollposition nach dem Wechsel messen", [=](int b) {
                     if (b < 3) { *maxAbw = 0.0F; return false; }
                     if (ImGuiWindow* w = baumFenster(teil)) {
                         *maxAbw = std::max(*maxAbw, std::fabs(w->Scroll.y - r->scroll));
                     }
                     return b >= 33;
                 }});
    s.push_back(pruefe(was + ": Reiter kommt mit SEINER Rollposition zurueck und bleibt dort", [=] {
        char z[160];
        std::snprintf(z, sizeof(z), "Knopftest: %s erwartet %.0f, groesste Abweichung %.1f",
                      was.c_str(), static_cast<double>(r->scroll), static_cast<double>(*maxAbw));
        diag::detail(z);
        return tabKennung(g_app->activeTab) == r->uid && *maxAbw < 1.0F;
    }));
}

// Der Satz Handlungen fuer einen Reiter in einem Feld.
void handlungenFuerReiter(std::vector<Schritt>& s, const std::string& teil, const std::string& wer) {
    auto h = std::make_shared<Handlung>();
    auto alles = [&](Wahl q, Wahl z, bool klick, bool ziehen, const std::string& was, bool verschoben) {
        s.push_back(pause(0.5));
        s.push_back(handlung(h, teil, q, z, klick, ziehen, wer + ": " + was));
        pruefeHandlung(s, h, wer + ": " + was, verschoben);
    };
    // Ganz unten
    s.push_back(rolleBaum(teil, 1.0F));
    alles({"wait", -1}, {"wait", -1}, true, false, "unten nur anklicken", false);
    alles({"wait", -2}, {"wait", -6}, true, true, "unten anklicken, dann nach oben ziehen", true);
    alles({"wait", -9}, {"wait", -3}, false, true, "unten ohne Klick nach unten ziehen", true);
    // Drei markierte Zeilen: dabei erschien frueher "3 selected" MITTEN in
    // der Action-Leiste und schob alles darunter eine Zeile tiefer.
    s.push_back(markiereUntereDrei());
    alles({"wait", -2}, {"wait", -8}, false, true, "unten drei markierte Zeilen ziehen", true);
    // Mitte, Block A sichtbar
    s.push_back(rolleZuZeile(teil, "affect", 0, 0.3F));
    alles({"set", 2}, {"wait", 0}, true, true, "Kind aus Block A herausziehen", true);
    alles({"wait", -1}, {"affect", 0}, true, true, "Zeile in Block A hineinziehen", true);
    alles({"set", -1}, {"set", 0}, true, true, "innerhalb Block A nach oben", true);
    alles({"set", 1}, {"set", -2}, false, true, "innerhalb Block A nach unten", true);
    // Oben
    s.push_back(rolleBaum(teil, 0.0F));
    alles({"wait", 3}, {"wait", 0}, true, true, "ganz oben anklicken und ziehen", true);
    // Block B, weiter unten, Kamerazeilen
    s.push_back(rolleZuZeile(teil, "affect", 1, 0.5F));
    alles({"camera", 4}, {"camera", 0}, true, true, "Kamerazeile in Block B nach oben", true);
    alles({"camera", 1}, {"wait", -1}, true, true, "Kamerazeile aus Block B nach unten", true);
}

std::vector<Schritt> ziehBreitSchritte() {
    std::vector<Schritt> s;
    auto rA = std::make_shared<ReiterRolle>();
    auto rB = std::make_shared<ReiterRolle>();
    auto rC = std::make_shared<ReiterRolle>();
    s.push_back(tu("Drei lange Skripte mit Bloecken oeffnen", [] {
        openScriptFromMemory(realesSkript(1000).c_str(), "reiter_a.txt");
        g_app->expanded.openAll();
        rebuildTree();
    }));
    s.push_back(pause(0.4));
    handlungenFuerReiter(s, "/pane0_", "Reiter A");
    s.push_back(rolleBaum("/pane0_", 0.2F));
    s.push_back(merkeRolle(rA, "/pane0_"));

    s.push_back(tu("Reiter B oeffnen", [] {
        openScriptFromMemory(realesSkript(2000).c_str(), "reiter_b.txt");
        g_app->expanded.openAll();
        rebuildTree();
    }));
    s.push_back(pause(0.4));
    handlungenFuerReiter(s, "/pane0_", "Reiter B");
    s.push_back(rolleBaum("/pane0_", 0.55F));
    s.push_back(merkeRolle(rB, "/pane0_"));

    s.push_back(tu("Reiter C oeffnen", [] {
        openScriptFromMemory(realesSkript(3000).c_str(), "reiter_c.txt");
        g_app->expanded.openAll();
        rebuildTree();
    }));
    s.push_back(pause(0.4));
    handlungenFuerReiter(s, "/pane0_", "Reiter C");
    s.push_back(rolleBaum("/pane0_", 0.9F));
    s.push_back(merkeRolle(rC, "/pane0_"));

    // Hin und her: jeder kommt mit seiner Position zurueck, und der erste
    // Klick danach springt nicht.
    wechsleUndPruefe(s, rA, "/pane0_", "A");
    {
        auto h = std::make_shared<Handlung>();
        s.push_back(pause(0.5));
        s.push_back(handlung(h, "/pane0_", {"wait", 5}, {"wait", 5}, true, false,
                             "Reiter A nach dem Wechsel: erster Klick"));
        pruefeHandlung(s, h, "Reiter A nach dem Wechsel: erster Klick", false);
        s.push_back(pause(0.5));
        s.push_back(handlung(h, "/pane0_", {"wait", 8}, {"wait", 2}, true, true,
                             "Reiter A nach dem Wechsel: erster Zug"));
        pruefeHandlung(s, h, "Reiter A nach dem Wechsel: erster Zug", true);
    }
    s.push_back(merkeRolle(rA, "/pane0_"));
    wechsleUndPruefe(s, rC, "/pane0_", "C");
    wechsleUndPruefe(s, rB, "/pane0_", "B");
    wechsleUndPruefe(s, rA, "/pane0_", "A (zweites Mal)");

    // Reiter B schliessen: C rueckt eine Nummer auf und darf B's Position
    // NICHT erben.
    s.push_back(tu("Reiter B schliessen", [=] {
        for (int i = 0; i < static_cast<int>(g_app->tabs.size()); ++i) {
            if (tabKennung(i) == rB->uid) { reiterSchliessen(i); break; }
        }
    }));
    s.push_back(antworteNein());
    s.push_back(pause(0.3));
    wechsleUndPruefe(s, rC, "/pane0_", "C nach dem Schliessen von B");
    {
        auto h = std::make_shared<Handlung>();
        s.push_back(pause(0.5));
        s.push_back(handlung(h, "/pane0_", {"wait", 4}, {"wait", 1}, true, true,
                             "Reiter C nach dem Schliessen: Klick und Zug"));
        pruefeHandlung(s, h, "Reiter C nach dem Schliessen: Klick und Zug", true);
    }

    // Geteilte Ansicht: A links, C rechts, in beiden Feldern arbeiten.
    // Erst auf A wechseln, DANN teilen: die Aufteilung gehoert dem Reiter,
    // in dem sie eingeschaltet wird ("Arbeitsbereich") - ein Wechsel auf
    // einen anderen Reiter danach verlaesst sie.
    s.push_back(tu("Zu Reiter A", [=] {
        for (int i = 0; i < static_cast<int>(g_app->tabs.size()); ++i) {
            if (tabKennung(i) == rA->uid) { g_app->tabRequest = i; }
        }
    }));
    s.push_back(pause(0.3));
    splitMenue(s, 2);
    s.push_back(tu("Feld 2 zeigt C, Teiler Mitte", [=] {
        g_app->splitFracX = 0.5F;
        for (int i = 0; i < static_cast<int>(g_app->tabs.size()); ++i) {
            if (tabKennung(i) == rC->uid) { g_app->splitPanes[1].tab = i; }
        }
    }));
    s.push_back(pause(0.5));
    handlungenFuerReiter(s, "/pane0_", "Split Feld 1");
    s.push_back(tu("Fokus ins zweite Feld", [] { g_app->focusRequest = 1; }));
    s.push_back(pause(0.5));
    handlungenFuerReiter(s, "/pane1_", "Split Feld 2");
    s.push_back(tu("Fokus zurueck", [] { g_app->focusRequest = 0; }));
    s.push_back(pause(0.3));
    splitMenue(s, 1);
    s.push_back(pause(0.3));
    return s;
}

// --- Lesezeichen und Aenderungsrand (wie Notepad++) ------------------------
//
// Allein aufrufbar mit BHED_EDITORTEST=lesezeichen.
Kennung kennungAn(const Path& p) {
    g_app->doc.vergibKennungen();
    const Node* n = nodeAt(g_app->doc.script(), p);
    return n != nullptr ? n->kennung : 0;
}

// Weg der `n`ten Zeile dieses Namens im Baum (0 = die erste).
Path wegDerZeile(const std::string& name, int n) {
    int z = 0;
    for (const Row& row : g_app->rows) {
        if (row.name == name && z++ == n) { return row.path; }
    }
    return Path{};
}

int markeVon(Kennung k) {
    for (std::size_t i = 0; i < g_app->rows.size() && i < g_app->zeilenMarke.size(); ++i) {
        if (g_app->rows[i].kennung == k) { return g_app->zeilenMarke[i]; }
    }
    return -1;
}

std::string markenText() {
    std::string s;
    for (std::size_t i = 0; i < g_app->zeilenMarke.size(); ++i) {
        if (g_app->zeilenMarke[i] != 0) {
            s += std::to_string(i) + ":" + std::to_string(g_app->zeilenMarke[i]) + " ";
        }
    }
    return s.empty() ? "(keine)" : s;
}

// In den Rand links neben der `nte` sichtbaren Zeile dieses Namens klicken.
Schritt klickRand(const std::string& name, int nte) {
    auto ziel = std::make_shared<ImVec2>();
    return {"Klick in den Rand neben " + name + " #" + std::to_string(nte), [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    const Element* e = sichtbareZeile("/pane0_", {name, nte});
                    if (e == nullptr) { meldeFehler("Rand: Zeile nicht sichtbar"); return true; }
                    *ziel = ImVec2{g_app->rinneX0 + ImGui::GetFontSize() * 0.4F, e->rect.GetCenter().y};
                    setzeMaus(*ziel);
                    return false;
                }
                if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                if (b == 2) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 5;
            }};
}

// Ein Foto des fertigen Bildes, wenn BHED_FOTOS gesetzt ist (Ordner).
Schritt foto(const std::string& name) {
    return {"Foto " + name, [=](int b) {
                const char* ordner = std::getenv("BHED_FOTOS");
                if (ordner == nullptr) { return true; }
                if (b == 2) { render::fotoAnfordern(std::string(ordner) + "\\" + name + ".bmp"); }
                return b >= 5;
            }};
}

// --- Die Vektorsymbole zur Ansicht: alle Groessen, beide Farbsaetze ---------
std::vector<Schritt> iconSchritte() {
    std::vector<Schritt> s;
    static const char* const namen[] = {
        "I_BRACE", "I_EVENT", "I_MACRO", "I_SOUND", "I_CAMERA", "I_ROTATE", "I_REMOVE",
        "I_SET", "I_MOVE", "I_IF", "I_LOOP", "I_DO", "I_WAIT", "I_DOWAIT", "I_SIGNAL",
        "I_WAITSIGNAL", "I_FLUSH", "I_WAITCLOCK"};
    auto fenster = [](bool hell) {
        ImGui::SetNextWindowPos(ImVec2{0.0F, 0.0F});
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, hell ? ImVec4{0.95F, 0.95F, 0.95F, 1.0F}
                                                      : ImVec4{0.12F, 0.12F, 0.12F, 1.0F});
        ImGui::PushStyleColor(ImGuiCol_Text, hell ? ImVec4{0, 0, 0, 1} : ImVec4{1, 1, 1, 1});
        ImGui::Begin("##iconprobe", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float groessen[] = {16.0F, 24.0F, 32.0F, 48.0F, 96.0F};
        float y = 20.0F;
        int n = 0;
        for (const char* name : namen) {
            const float spalte = (n % 2 == 0) ? 20.0F : 1280.0F;
            if (n % 2 == 0 && n > 0) { y += 110.0F; }
            float x = spalte;
            dl->AddText(ImVec2{x, y}, ImGui::GetColorU32(ImGuiCol_Text), name);
            x += 200.0F;
            for (const float g : groessen) {
                (void)vektorIcon(dl, name, ImVec2{x, y + 96.0F - g}, g, false);
                x += g + 16.0F;
            }
            (void)vektorIcon(dl, name, ImVec2{x + 20.0F, y}, 96.0F, true);   // zweiter Satz
            ++n;
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
    };
    for (int hell = 0; hell < 2; ++hell) {
        s.push_back({hell ? "Symbole hell" : "Symbole dunkel", [=](int b) {
                         fenster(hell != 0);
                         if (b == 3) {
                             if (const char* ordner = std::getenv("BHED_FOTOS")) {
                                 render::fotoAnfordern(std::string(ordner) + (hell ? "\\icons_hell.bmp"
                                                                                   : "\\icons_dunkel.bmp"));
                             }
                         }
                         return b >= 8;
                     }});
    }
    return s;
}

std::vector<Schritt> lesezeichenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto datei = std::make_shared<std::string>(platform::executableDirectory() +
                                               "\\selbsttest-lesezeichen.txt");
    auto k3 = std::make_shared<Kennung>(0);
    auto k8 = std::make_shared<Kennung>(0);
    auto kz = std::make_shared<Kennung>(0);
    add(tu("Testdatei schreiben und oeffnen", [=] {
        std::remove((*datei).c_str());
        g_app->settings.lesezeichen.erase(*datei);
        spewDatei(*datei, realesSkript(7000));
        ladeSkriptDatei(*datei);
        g_app->expanded.openAll();
        rebuildTree();
    }));
    add(pause(0.3));
    add(rolleBaum("/pane0_", 0.0F));
    add(pruefe("Nach dem Laden: kein Aenderungsrand", [] {
        diag::detail("Knopftest: Rand nach dem Laden " + markenText());
        for (std::uint8_t m : g_app->zeilenMarke) { if (m != 0) { return false; } }
        return !g_app->zeilenMarke.empty();
    }));

    // ---- Lesezeichen setzen: Strg+F2 und Klick in den Rand --------------
    auto w3 = std::make_shared<Path>();
    auto w8 = std::make_shared<Path>();
    add(tu("Dritte wait-Zeile waehlen", [=] {
        *w3 = wegDerZeile("wait", 2);
        *w8 = wegDerZeile("wait", 8);
        selectByPath(*w3);
        *k3 = kennungAn(*w3);
        *k8 = kennungAn(*w8);
    }));
    add(pause(0.2));
    add(rolleBaum("/pane0_", 0.0F));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_F2, "Strg+F2"));
    add(pruefe("Strg+F2 setzt ein Lesezeichen auf die gewaehlte Zeile",
               [=] { return hatLesezeichen(g_app->activeTab, *k3); }));
    add(klickRand("wait", 8));
    add(pruefe("Klick in den Rand setzt ein Lesezeichen auf diese Zeile",
               [=] { return hatLesezeichen(g_app->activeTab, *k8); }));
    add(pruefe("Der Klick in den Rand waehlt die Zeile NICHT (wie Notepad++)",
               [=] { return g_app->selectedPath == *w3; }));
    add(pruefe("Unveraendertes Dokument: beide Lesezeichen sofort gemerkt", [=] {
        const auto it = g_app->settings.lesezeichen.find(*datei);
        diag::detail("Knopftest: gemerkt \"" +
                     (it == g_app->settings.lesezeichen.end() ? std::string("-") : it->second) + "\"");
        return it != g_app->settings.lesezeichen.end() &&
               std::count(it->second.begin(), it->second.end(), ',') == 1;
    }));

    // ---- F2 / Umschalt+F2 mit Umlauf -----------------------------------
    add(tu("Ganz oben waehlen", [] { selectByPath(Path{0}); }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_F2, "F2"));
    add(pruefe("F2 springt zum ersten Lesezeichen", [=] { return g_app->selectedPath == *w3; }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_F2, "F2"));
    add(pruefe("F2 springt zum naechsten", [=] { return g_app->selectedPath == *w8; }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_F2, "F2"));
    add(pruefe("F2 am Ende beginnt wieder oben", [=] { return g_app->selectedPath == *w3; }));
    add(tasteMit(ImGuiMod_Shift, ImGuiKey_F2, "Umschalt+F2"));
    add(pruefe("Umschalt+F2 am Anfang springt ans Ende", [=] { return g_app->selectedPath == *w8; }));
    add(foto("1_lesezeichen"));
    add(klickRand("wait", 8));
    add(pruefe("Zweiter Klick in den Rand entfernt das Lesezeichen",
               [=] { return !hatLesezeichen(g_app->activeTab, *k8); }));
    add(klickRand("wait", 8));

    // ---- Ueber Schliessen und Oeffnen erhalten -------------------------
    add(tu("Reiter schliessen und Datei wieder oeffnen", [=] {
        reiterSchliessen(g_app->activeTab);
        ladeSkriptDatei(*datei);
        g_app->expanded.openAll();
        rebuildTree();
    }));
    add(pause(0.3));
    add(pruefe("Nach dem Wiederoeffnen stehen die Lesezeichen wieder auf denselben Zeilen", [=] {
        *k3 = kennungAn(*w3);
        *k8 = kennungAn(*w8);
        int zahl = 0;
        const auto it = g_app->lesezeichen.find(tabKennung(g_app->activeTab));
        if (it != g_app->lesezeichen.end()) { zahl = static_cast<int>(it->second.size()); }
        return zahl == 2 && hatLesezeichen(g_app->activeTab, *k3) &&
               hatLesezeichen(g_app->activeTab, *k8);
    }));

    // ---- Aenderungsrand: orange, gruen, blau ---------------------------
    add(rolleBaum("/pane0_", 0.0F));
    add(tu("Zeile 10 waehlen", [=] { selectByPath(Path{10}); *kz = kennungAn(Path{10}); }));
    add(klick(tr(Str::MoveDown)));
    add(pruefe("Nach dem Verschieben: genau eine Zeile orange (geaendert)", [=] {
        diag::detail("Knopftest: Rand nach Move down " + markenText());
        int orange = 0, sonst = 0;
        for (std::uint8_t m : g_app->zeilenMarke) { (m == 1 ? orange : (m != 0 ? sonst : orange)) += (m != 0); }
        return orange == 1 && sonst == 0;
    }));
    add(foto("2_orange_verschoben"));
    add(pruefe("Lesezeichen bleiben beim Bearbeiten an ihren Zeilen",
               [=] { return hatLesezeichen(g_app->activeTab, *k3) && hatLesezeichen(g_app->activeTab, *k8); }));
    add(klick(tr(Str::FileSave)));
    add(pruefe("Nach dem Speichern: dieselbe Zeile gruen (geaendert und gespeichert)", [=] {
        diag::detail("Knopftest: Rand nach Save " + markenText());
        int gruen = 0, sonst = 0;
        for (std::uint8_t m : g_app->zeilenMarke) { if (m == 2) { ++gruen; } else if (m != 0) { ++sonst; } }
        return gruen == 1 && sonst == 0;
    }));
    add(foto("3_gruen_gespeichert"));
    add(klick(tr(Str::EditUndo)));
    add(pruefe("Rueckgaengig nach dem Speichern: blassblau (wieder wie beim Oeffnen)", [=] {
        diag::detail("Knopftest: Rand nach Undo " + markenText());
        int blau = 0, sonst = 0;
        for (std::uint8_t m : g_app->zeilenMarke) { if (m == 3) { ++blau; } else if (m != 0) { ++sonst; } }
        return blau == 1 && sonst == 0;
    }));
    add(tu("Eine Zeile waehlen", [=] { selectByPath(wegDerZeile("wait", 4)); }));
    add(klick(tr(Str::ActClone)));
    add(pruefe("Neu eingefuegte Zeile (Duplicate) ist orange", [=] {
        diag::detail("Knopftest: Rand nach Duplicate " + markenText());
        return std::count(g_app->zeilenMarke.begin(), g_app->zeilenMarke.end(), 1) == 1;
    }));
    add(foto("4_blau_und_orange"));
    add(tu("Aenderungsrand ausschalten (View)", [] {
        g_app->settings.changeHistory = false;
        rebuildTree();
    }));
    add(pruefe("Ausgeschaltet: kein Rand", [] {
        for (std::uint8_t m : g_app->zeilenMarke) { if (m != 0) { return false; } }
        return true;
    }));
    add(tu("Aenderungsrand wieder an", [] {
        g_app->settings.changeHistory = true;
        rebuildTree();
    }));

    // ---- Alle entfernen ueber das Menue --------------------------------
    add(klick(tr(Str::MenuEdit)));
    add(klick(tr(Str::MenuBookmarks), "##Menu"));
    add(klick(tr(Str::ActBookmarkClear), "##Menu"));
    add(pruefe("Edit > Bookmarks > Clear all: keine Lesezeichen, nichts mehr gemerkt", [=] {
        return !hatLesezeichen(g_app->activeTab, *k3) && !hatLesezeichen(g_app->activeTab, *k8) &&
               g_app->settings.lesezeichen.count(*datei) == 0;
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_F2, "F2 ohne Lesezeichen"));
    add(klick(tr(Str::EditUndo)));
    add(tu("Aufraeumen: Testdatei weg, Reiter schliessen", [=] {
        g_app->doc.markSaved();
        reiterSchliessen(g_app->activeTab);
        std::remove((*datei).c_str());
        g_app->settings.lesezeichen.erase(*datei);
        auto& rc = g_app->settings.recent;
        rc.erase(std::remove(rc.begin(), rc.end(), *datei), rc.end());
    }));
    add(pause(0.3));
    return s;
}

// --- Maushelfer fuer die Kartenansicht ------------------------------------
// Ein Bildpunkt des GERASTERTEN Bildes (Marker, Schluessel) auf den Schirm.
ImVec2 bildZuSchirm(float sx, float sy) {
    const float w = static_cast<float>(std::max(1, g_app->mapImage.width));
    const float h = static_cast<float>(std::max(1, g_app->mapImage.height));
    return ImVec2{g_app->kartenAnsicht[0] + sx / w * (g_app->kartenAnsicht[2] - g_app->kartenAnsicht[0]),
                  g_app->kartenAnsicht[1] + sy / h * (g_app->kartenAnsicht[3] - g_app->kartenAnsicht[1])};
}
ImVec2 ansichtMitte() {
    return ImVec2{(g_app->kartenAnsicht[0] + g_app->kartenAnsicht[2]) * 0.5F,
                  (g_app->kartenAnsicht[1] + g_app->kartenAnsicht[3]) * 0.5F};
}

// Klick (oder Doppelklick) an eine berechnete Stelle.
Schritt klickAn(const std::string& was, std::function<ImVec2()> ort, int taste = 0, bool doppelt = false) {
    auto z = std::make_shared<ImVec2>();
    auto frist = std::make_shared<std::array<float, 2>>();
    return {"Klick " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                // Doppelklick: die Ereignisse liegen in Bild 2..5; in langsamen
                // Laeufen (ASan) dauert das laenger als die 0,30 s von ImGui, darum
                // gilt fuer diesen Schritt eine weite Frist, danach wieder die alte.
                // ImGui verlangt MouseSingleClickDelay > MouseDoubleClickTime.
                if (b == 0) {
                    *z = ort();
                    setzeMaus(*z);
                    if (doppelt) {
                        *frist = {io.MouseDoubleClickTime, io.MouseSingleClickDelay};
                        io.MouseSingleClickDelay = 31.0f;
                        io.MouseDoubleClickTime = 30.0f;
                        // Frueherer Klick zaehlt nicht mit (sonst wird der erste
                        // Klick eines zweiten Doppelklicks zum dritten).
                        io.MouseClickedTime[taste] = -1.0e9;
                        io.MouseClickedLastCount[taste] = 0;
                    }
                    return false;
                }
                if (b == 2) { io.AddMouseButtonEvent(taste, true); return false; }
                if (b == 3) { io.AddMouseButtonEvent(taste, false); return false; }
                if (doppelt && b == 4) { io.AddMouseButtonEvent(taste, true); return false; }
                if (doppelt && b == 5) { io.AddMouseButtonEvent(taste, false); return false; }
                if (b < 9) { return false; }
                if (doppelt) {
                    io.MouseDoubleClickTime = (*frist)[0];
                    io.MouseSingleClickDelay = (*frist)[1];
                }
                return true;
            }};
}

// Mit gedrueckter Taste von einer Stelle um (dx, dy) ziehen.
Schritt ziehenAn(const std::string& was, std::function<ImVec2()> ort, float dx, float dy, int taste,
                 ImGuiKey mod = ImGuiKey_None) {
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) { *z = ort(); setzeMaus(*z); return false; }
                if (b == 1 && mod != ImGuiKey_None) { io.AddKeyEvent(mod, true); return false; }
                if (b == 2) { io.AddMouseButtonEvent(taste, true); return false; }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{z->x + dx * f, z->y + dy * f});
                    return false;
                }
                if (b == 15) { io.AddMouseButtonEvent(taste, false); return false; }
                if (b == 16 && mod != ImGuiKey_None) { io.AddKeyEvent(mod, false); return false; }
                return b >= 20;
            }};
}

// Ziehen und danach `halte` Bilder STILL festhalten, erst dann loslassen.
// So faellt auf, wenn ein Griff bei ruhiger Maus weiterlaeuft (die
// Spaltenteiler taten das bis zum 28.09.: jedes Bild kamen die Zellraender
// dazu).
// `spur`: wenn gesetzt, die kleinste und groesste Luecke rechts neben der
// letzten Hauptspalte ueber ALLE Bilder des Zuges (App::spaltenLuecke).
// `schritte`: in wie vielen Bildern die Maus den Weg zuruecklegt (1 = ein
// Ruck, wie eine schnelle echte Maus bei wenigen Bildern je Sekunde).
Schritt ziehenHalten(const std::string& was, std::function<ImVec2()> ort, float dx, float dy, int halte,
                     std::shared_ptr<std::pair<float, float>> spur = nullptr, int schritte = 12) {
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe und halte " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (spur && b >= 3) {
                    spur->first = std::min(spur->first, g_app->spaltenLuecke);
                    spur->second = std::max(spur->second, g_app->spaltenLuecke);
                }
                if (b == 0) {
                    *z = ort();
                    setzeMaus(*z);
                    if (spur) { *spur = {g_app->spaltenLuecke, g_app->spaltenLuecke}; }
                    return false;
                }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 3 && b <= 14) {
                    const float f = std::min(1.0F, static_cast<float>(b - 2) / static_cast<float>(std::max(1, schritte)));
                    setzeMaus(ImVec2{z->x + dx * f, z->y + dy * f});
                    return false;
                }
                if (b == 15 + halte) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 20 + halte;
            }};
}

// Mausrad an einer Stelle.
Schritt radAn(const std::string& was, std::function<ImVec2()> ort, float klicks) {
    return {"Rad " + was, [=](int b) {
                if (b == 0) { setzeMaus(ort()); return false; }
                if (b == 2) { ImGui::GetIO().AddMouseWheelEvent(0.0F, klicks); return false; }
                return b >= 8;
            }};
}

// Eine Taste `bilder` Bilder lang halten, Maus ueber der Ansicht.
Schritt halteTaste(const std::string& was, ImGuiKey k, int bilder) {
    return {"Halte " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) { setzeMaus(ansichtMitte()); return false; }
                if (b == 1) { io.AddKeyEvent(k, true); return false; }
                if (b == 1 + bilder) { io.AddKeyEvent(k, false); return false; }
                return b >= bilder + 5;
            }};
}

// Einmal durchzeichnen lassen (mapDirty abgearbeitet), dann pruefen.
Schritt gezeichnet(const std::string& was) {
    return warteBis(was + " - neu gezeichnet", [] { return !g_app->mapDirty; }, 600);
}

// Die freie Kamera auf den ersten MOVE-Schluessel richten.
void kameraVorDenSchluessel() {
    for (const CamSegment& sg : g_app->camTrack.segments) {
        if (sg.kind != CamSegment::Kind::Move) { continue; }
        g_app->cam.pos[0] = sg.to[0] - 240.0F;
        g_app->cam.pos[1] = sg.to[1];
        g_app->cam.pos[2] = sg.to[2] + 80.0F;
        g_app->cam.angles[0] = 18.0F;
        g_app->cam.angles[1] = 0.0F;
        g_app->cam.angles[2] = 0.0F;
        break;
    }
    g_app->mapDirty = true;
}

Path ersterKameraweg(const char* art) {
    for (std::size_t i = 0; i < g_app->doc.script().nodes.size(); ++i) {
        const Node& n = g_app->doc.script().nodes[i];
        if (n.name == "camera" && !n.args.empty() && n.args[0].text == art) { return Path{i}; }
    }
    return Path{};
}

std::size_t diagZeilen() { return diag::lines().size(); }

// --- Karte: jede Funktion (27.09.) ------------------------------------------
//
// shank: "schau bitte jedes feature bei map an ... ein umfangreiches map
// testen nach fehlern". Laeuft nach karteSchritte() (Mission md_am_sith ist
// geladen). Seit rc568 gibt es nur noch den Weg ueber die Grafikkarte.
int ueberlagerungsPunkte() {
    int n = 0;
    const std::vector<std::uint8_t>& px = g_app->gizmoBild.rgba;
    for (std::size_t i = 3; i < px.size(); i += 4) {
        if (px[i] != 0) { ++n; }
    }
    return n;
}

// Ein neues Bild anfordern und zwanzig Bilder warten. Hiess bis rc568
// `zeichenweg(bool)` und schaltete zwischen Grafikkarte und Rasterer um.
Schritt neuZeichnen() {
    return {"Neu zeichnen", [=](int b) {
                if (b == 0) {
                    g_app->gpuFehler.clear();
                    g_app->mapDirty = true;
                }
                return b >= 20;
            }};
}

std::vector<Schritt> kartenBreitSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Freie Ansicht vor dem ersten Kameraschluessel", [=] {
        g_app->throughCamera = false;
        g_app->playing = false;
        g_app->showEntities = true;
        // Die freie Kamera 240 Einheiten hinter und 80 ueber den ersten
        // MOVE-Schluessel, Blick darauf: dort muessen Kameramodell und Bahn
        // gross im Bild stehen.
        for (const CamSegment& sg : g_app->camTrack.segments) {
            if (sg.kind != CamSegment::Kind::Move) { continue; }
            g_app->cam.pos[0] = sg.to[0] - 240.0F;
            g_app->cam.pos[1] = sg.to[1];
            g_app->cam.pos[2] = sg.to[2] + 80.0F;
            g_app->cam.angles[0] = 18.0F;   // leicht nach unten
            g_app->cam.angles[1] = 0.0F;    // nach +x
            g_app->cam.angles[2] = 0.0F;
            break;
        }
        g_app->mapDirty = true;
    }));

    // ---- Ueberlagerung (Entitykreuze, Kamera, Bahnen) --------------------
    add(neuZeichnen());
    add(pruefe("Entitykreuze sind da und anklickbar (markers)", [=] {
        diag::detail("Knopftest: markers " + std::to_string(g_app->markers.size()) +
                     ", Ueberlagerung " + std::to_string(ueberlagerungsPunkte()) + " Punkte, GPU-Aufrufe " +
                     std::to_string(g_app->gpuAufrufe));
        return !g_app->markers.empty();
    }));
    add(pruefe("Die Ueberlagerung hat Inhalt und liegt ueber dem Bild", [] {
        return g_app->gpuAufrufe > 0 && g_app->gizmoZeigen && ueberlagerungsPunkte() > 50;
    }));
    add(foto("karte_gpu_frei"));
    add(tu("Kamera auswaehlen (Bahn und Schluessel zeigen)", [] {
        g_app->cameraSelected = true;
        g_app->mapDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("Ausgewaehlte Kamera zeigt ihre Schluessel (keyMarks)", [=] {
        diag::detail("Knopftest: keyMarks " + std::to_string(g_app->keyMarks.size()));
        return !g_app->keyMarks.empty();
    }));
    add(pruefe("Mit ausgewaehlter Kamera mehr Ueberlagerung (Bahn, Schluessel)", [] {
        diag::detail("Knopftest: GPU-Ueberlagerung mit Kamera " + std::to_string(ueberlagerungsPunkte()));
        return ueberlagerungsPunkte() > 200;
    }));
    add(foto("karte_gpu_kamera"));
    add(tu("Kamera abwaehlen", [] { g_app->cameraSelected = false; g_app->mapDirty = true; }));
    add(pause(0.3));

    // ======== Alle Funktionen, auf dem Weg des Benutzers ======================
    auto logAb = std::make_shared<std::size_t>(0);
    add(tu("Protokollstand merken, Kamera vor den Schluessel", [=] {
        *logAb = diagZeilen();
        kameraVorDenSchluessel();
    }));
    add(gezeichnet("Ausgangsbild"));

    // ---- Seitenleiste: jeder Schalter an/aus, jedes Mal ein neues Bild ------
    struct Schalter { Str text; bool* wert; };
    const std::vector<Schalter> schalter = {
        {Str::MapEntities, &g_app->showEntities}, {Str::MapEffects, &g_app->showEffects},
        {Str::MapGlow, &g_app->showGlow},         {Str::MapActors, &g_app->showActors},
        {Str::MapSky, &g_app->showSky},           {Str::MapNames, &g_app->zeigeNamen},
        {Str::MapDynLights, &g_app->showDynLights}, {Str::MapFog, &g_app->showFog}};
    for (const Schalter& sw : schalter) {
        const std::string name = tr(sw.text);
        bool* wert = sw.wert;
        auto vorher = std::make_shared<bool>(false);
        add(tu("Stand " + name + " merken", [=] { *vorher = *wert; }));
        add(klick(name));
        add(gezeichnet(name + " umgeschaltet"));
        add(pruefe("Seitenleiste: " + name + " schaltet um und zeichnet neu",
                   [=] { return *wert != *vorher && g_app->gpuFehler.empty(); }));
        add(klick(name));
        add(gezeichnet(name + " zurueck"));
        add(pruefe("Seitenleiste: " + name + " zurueck", [=] { return *wert == *vorher; }));
    }
    // Die vier Schattenarten (cg_shadows 0..3): jede zeichnet ohne Fehler und
    // ohne Beanstandung der Debugschicht - Art 2 benutzt Geometrie-Shader und
    // Stencil, Art 3 einen eigenen Vertex-Shader.
    auto d3dVorher = std::make_shared<std::size_t>(0);
    add(tu("Schattenart: Stand merken", [=] { *d3dVorher = g_app->d3dMeldungen.size(); }));
    for (int art = 0; art <= 3; ++art) {
        const std::string n = std::to_string(art);
        add(tu("Schattenart " + n, [=] { g_app->schattenArt = art; g_app->mapDirty = true; }));
        add(gezeichnet("Schattenart " + n));
        add(pruefe("Seitenleiste: Schattenart " + n + " zeichnet ohne Fehler", [=] {
            // rc602: WAS schiefging, steht sonst nirgends im Protokoll.
            diag::detail("Knopftest: Schattenart " + n + " - gpuFehler \"" + g_app->gpuFehler + "\", D3D-Meldungen " +
                         std::to_string(g_app->d3dMeldungen.size() - *d3dVorher));
            return g_app->gpuFehler.empty() && g_app->d3dMeldungen.size() == *d3dVorher;
        }));
    }
    add(tu("Schattenart zurueck auf rund", [] { g_app->schattenArt = 1; g_app->mapDirty = true; }));
    add(gezeichnet("Schattenart rund"));
    // Wirkung einzeln
    add(tu("Entities aus", [] { g_app->showEntities = false; g_app->mapDirty = true; }));
    add(gezeichnet("ohne Entities"));
    add(pruefe("Entities aus: keine Kreuze und nichts anklickbar", [] { return g_app->markers.empty(); }));
    add(tu("Entities an", [] { g_app->showEntities = true; g_app->mapDirty = true; }));
    auto stapelMitHimmel = std::make_shared<std::size_t>(0);
    add(tu("Stapel mit Himmel merken, Himmel aus", [=] {
        *stapelMitHimmel = g_app->mesh.batches.size();
        g_app->showSky = true;
    }));
    add(klick(tr(Str::MapSky)));
    add(gezeichnet("Himmel aus"));
    add(pruefe("Sky aus baut das Netz neu (weniger Stapel)", [=] {
        diag::detail("Knopftest: Stapel mit Himmel " + std::to_string(*stapelMitHimmel) + ", ohne " +
                     std::to_string(g_app->mesh.batches.size()));
        return !g_app->showSky && g_app->mesh.batches.size() < *stapelMitHimmel && g_app->mapBereit;
    }));
    add(klick(tr(Str::MapSky)));
    add(gezeichnet("Himmel an"));
    add(pruefe("Glow liefert Gluehen (GPU-Aufrufe)", [] {
        diag::detail("Knopftest: gpuGluehen " + std::to_string(g_app->gpuGluehen));
        return !g_app->showGlow || g_app->gpuGluehen > 0;
    }));
    add(pruefe("Figuren: Modelle geladen", [] {
        int mit = 0;
        for (const auto& a : g_app->actorAssets) { if (!a->model.empty()) { ++mit; } }
        diag::detail("Knopftest: Figuren " + std::to_string(g_app->scene.actors.size()) + ", mit Modell " +
                     std::to_string(mit));
        return mit > 0;
    }));
    // Detail: Regler nach links ziehen baut das Netz neu
    auto detail0 = std::make_shared<int>(0);
    add(tu("Detail merken", [=] { *detail0 = g_app->meshDetail; }));
    add(tu("Ansicht einstellen: Klappfenster auf", [] { g_app->ansichtEinstellungenOeffnen = true; }));
    add(pause(0.3));
    add(ziehenAn("Detail-Regler nach links", [] {
        const Element* e = finde(tr(Str::MapDetail), "", 0, false);
        return e != nullptr ? ImVec2{e->rect.Max.x - 4.0F, e->rect.GetCenter().y} : ImVec2{0, 0};
    }, -400.0F, 0.0F, 0));
    add(gezeichnet("Detail geaendert"));
    add(pruefe("Detail-Regler aendert die Stufe und das Netz steht wieder", [=] {
        diag::detail("Knopftest: Detail " + std::to_string(*detail0) + " -> " + std::to_string(g_app->meshDetail));
        return g_app->meshDetail != *detail0 && g_app->mapBereit && !g_app->mesh.batches.empty();
    }));
    add(tu("Detail zurueck", [=] { g_app->meshDetail = *detail0; }));
    add(tu("Ansicht einstellen: Klappfenster auf", [] { g_app->ansichtEinstellungenOeffnen = true; }));
    add(pause(0.3));
    add(ziehenAn("Detail-Regler nach rechts", [] {
        const Element* e = finde(tr(Str::MapDetail), "", 0, false);
        return e != nullptr ? ImVec2{e->rect.Min.x + 4.0F, e->rect.GetCenter().y} : ImVec2{0, 0};
    }, 600.0F, 0.0F, 0));
    add(gezeichnet("Detail wieder hoch"));
    // Das Klappfenster schliessen (Esc), wie ein Anwender - solange es offen
    // ist, gehoeren Maus und Tasten ihm, nicht der Ansicht.
    // (Esc erreicht es nur mit Fokus, den es nach dem Ziehen nicht mehr hat;
    // ein Anwender klickt daneben - hier wird es direkt geschlossen.)
    add(tu("Klappfenster schliessen", [] {
        if (GImGui->OpenPopupStack.Size > 0) { ImGui::ClosePopupToLevel(0, true); }
    }));
    add(tu("Maus in die Ansicht", [=] { setzeMaus(ansichtMitte()); }));
    add(pause(0.2));

    // ---- Navigation in der freien Ansicht --------------------------------
    auto pos0 = std::make_shared<std::array<float, 3>>();
    auto ang0 = std::make_shared<std::array<float, 3>>();
    auto merke = [=]() {
        return tu("Kamera merken", [=] {
            for (int k = 0; k < 3; ++k) { (*pos0)[k] = g_app->cam.pos[k]; (*ang0)[k] = g_app->cam.angles[k]; }
        });
    };
    auto bewegt = [=](const std::string& was, bool ort, bool winkel) {
        return pruefe("Navigation: " + was, [=] {
            float dp = 0, da = 0;
            for (int k = 0; k < 3; ++k) {
                dp += std::fabs(g_app->cam.pos[k] - (*pos0)[k]);
                da += std::fabs(g_app->cam.angles[k] - (*ang0)[k]);
            }
            char z[400];
            std::snprintf(z, sizeof(z), "Knopftest: %s - Ort %.1f, Winkel %.1f (unter der Maus: %s, Klappfenster %s, durch Kamera %s)",
                          was.c_str(), static_cast<double>(dp), static_cast<double>(da),
                          GImGui->HoveredWindow != nullptr ? GImGui->HoveredWindow->Name : "-",
                          ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) ? "offen" : "zu",
                          g_app->throughCamera ? "an" : "aus");
            diag::detail(z);
            return (!ort || dp > 1.0F) && (!winkel || da > 0.5F);
        });
    };
    add(merke()); add(halteTaste("W", ImGuiKey_W, 20)); add(bewegt("W faehrt vor", true, false));
    add(merke()); add(halteTaste("D", ImGuiKey_D, 20)); add(bewegt("D faehrt seitwaerts", true, false));
    add(merke()); add(halteTaste("E", ImGuiKey_E, 20)); add(bewegt("E faehrt hoch", true, false));
    add(merke()); add(ziehenAn("rechte Maustaste (Umsehen)", ansichtMitte, 120.0F, 40.0F, 1));
    add(bewegt("rechte Maustaste dreht den Blick", false, true));
    add(merke()); add(ziehenAn("mittlere Maustaste (Schieben)", ansichtMitte, 120.0F, 60.0F, 2));
    add(bewegt("mittlere Maustaste schiebt", true, false));
    add(merke()); add(radAn("Zoom", ansichtMitte, 3.0F));
    add(bewegt("Mausrad zoomt", true, false));
    add(merke()); add(ziehenAn("Alt + mittlere Maustaste (Umkreisen)", ansichtMitte, 150.0F, 0.0F, 2, ImGuiMod_Alt));
    add(bewegt("Alt + mittlere Maustaste umkreist", true, true));

    // ---- Anklicken: Entity, Kamera, Schluessel ----------------------------
    add(tu("Kamera zurueck vor den Schluessel", [] { kameraVorDenSchluessel(); g_app->cameraSelected = false; }));
    add(gezeichnet("vor dem Schluessel"));
    auto zielEntity = std::make_shared<std::size_t>(0);
    add(klickAn("auf ein Entitykreuz", [=] {
        // Das Kreuz, das am weitesten von Kamera, Figuren und Schluesseln
        // weg liegt - die haben beim Klick Vorrang.
        if (g_app->markers.empty()) { return ImVec2{0, 0}; }
        float best = -1.0F;
        const App::PickedMarker* wahl = nullptr;
        for (const auto& m : g_app->markers) {
            float d = 1e9F;
            auto nah = [&](float x, float y) { d = std::min(d, (m.sx - x) * (m.sx - x) + (m.sy - y) * (m.sy - y)); };
            nah(g_app->cameraSx, g_app->cameraSy);
            for (const auto& am : g_app->actorMarks) { nah(am.sx, am.sy); }
            for (const auto& km : g_app->keyMarks) { nah(km.sx, km.sy); }
            for (const auto& o : g_app->markers) { if (&o != &m) { nah(o.sx, o.sy); } }
            const bool drin = m.sx > 20 && m.sy > 20 && m.sx < g_app->mapImage.width - 20 &&
                              m.sy < g_app->mapImage.height - 20;
            if (drin && d > best) { best = d; wahl = &m; }
        }
        if (wahl == nullptr) { return ImVec2{0, 0}; }
        *zielEntity = wahl->entity;
        return bildZuSchirm(wahl->sx, wahl->sy);
    }));
    add(pruefe("Klick auf ein Entitykreuz waehlt diese Entity", [=] {
        diag::detail("Knopftest: pickedEntity " + std::to_string(g_app->pickedEntity) + ", erwartet " +
                     std::to_string(*zielEntity));
        return g_app->pickedEntity == static_cast<int>(*zielEntity);
    }));
    add(tu("Maus auf die Kamera", [] { setzeMaus(bildZuSchirm(g_app->cameraSx, g_app->cameraSy)); }));
    add(pause(0.3));
    add(tu("Was liegt unter der Maus?", [] {
        const ImVec2 m = ImGui::GetIO().MousePos;
        diag::detail("Knopftest: Maus " + std::to_string(m.x) + "/" + std::to_string(m.y) + ", Fenster " +
                     std::string(GImGui->HoveredWindow != nullptr ? GImGui->HoveredWindow->Name : "-") + ", Bild " +
                     std::to_string(g_app->kartenAnsicht[0]) + ".." + std::to_string(g_app->kartenAnsicht[2]) + " / " +
                     std::to_string(g_app->kartenAnsicht[1]) + ".." + std::to_string(g_app->kartenAnsicht[3]) +
                     ", Hover " + std::to_string(g_app->entHover) + "/" + std::to_string(g_app->kameraHover) +
                     ", Griff " + std::to_string(g_app->entGizmoHover));
    }));
    add(klickAn("auf die Kamera", [] { return bildZuSchirm(g_app->cameraSx, g_app->cameraSy); }));
    add(pruefe("Klick auf die Kamera waehlt sie (Bahn und Schluessel)", [] {
        diag::detail("Knopftest: Kamera gewaehlt " + std::to_string(g_app->cameraSelected) + ", Entity " +
                     std::to_string(g_app->pickedEntity) + ", Figur " + std::to_string(g_app->selectedActor) +
                     ", Kamera bei " + std::to_string(g_app->cameraSx) + "/" + std::to_string(g_app->cameraSy) +
                     ", Schluessel " + std::to_string(g_app->keyMarks.size()) + ", Zug " +
                     std::to_string(g_app->entZug.kandidat) + ", Hover " + std::to_string(g_app->entHover) + "/" +
                     std::to_string(g_app->kameraHover));
        return g_app->cameraSelected && !g_app->keyMarks.empty();
    }));
    add(gezeichnet("Kamera gewaehlt"));
    auto zielKey = std::make_shared<Path>();
    add(klickAn("auf einen Schluessel", [=] {
        if (g_app->keyMarks.size() < 2) { return ImVec2{0, 0}; }
        const auto& k = g_app->keyMarks[1];
        *zielKey = k.path;
        return bildZuSchirm(k.sx, k.sy);
    }));
    add(pruefe("Klick auf einen Schluessel waehlt ihn und seine Skriptzeile",
               [=] { return !zielKey->empty() && g_app->selectedPath == *zielKey; }));

    // ---- Gizmo: Rechtsklick-Menue, Verschieben, Zurueckschreiben, Undo ------
    add(klickAn("rechts kurz (Gizmomenue)", ansichtMitte, 1));
    add(pruefe("Rechtsklick oeffnet das Gizmomenue", [] {
        return finde(tr(Str::GizmoMove), "##Popup", 0, false) != nullptr;
    }));
    add(klick(tr(Str::GizmoMove), "##Popup"));
    add(pruefe("Gizmo: Modus Verschieben", [] { return g_app->gizmoMode == App::GizmoMode::Move; }));
    auto moveVorher = std::make_shared<std::string>();
    add(tu("Gizmo um 32 Einheiten verschieben (wie gezogen)", [=] {
        const Path w = !g_app->keyMarks.empty() && g_app->selectedKey >= 0 &&
                               static_cast<std::size_t>(g_app->selectedKey) < g_app->keyMarks.size()
                           ? g_app->keyMarks[static_cast<std::size_t>(g_app->selectedKey)].path
                           : Path{};
        const Node* n = w.empty() ? nullptr : nodeAt(g_app->doc.script(), w);
        *moveVorher = (n != nullptr && n->args.size() > 1) ? n->args[1].text : "";
        g_app->gizmoOffset[0] = 32.0F;
        g_app->mapDirty = true;
    }));
    add(gezeichnet("Gizmo-Vorschau"));
    add(tu("Schreiben wie beim Loslassen (rc588: kein Apply-Knopf mehr)", [] { kameraVorschauSchreiben(); }));
    add(pruefe("Write to script: der MOVE-Vektor ist geaendert", [=] {
        const Path w = g_app->selectedKey < 0 || static_cast<std::size_t>(g_app->selectedKey) >= g_app->keyMarks.size()
                           ? Path{}
                           : g_app->keyMarks[static_cast<std::size_t>(g_app->selectedKey)].path;
        const Node* n = w.empty() ? nullptr : nodeAt(g_app->doc.script(), w);
        const std::string jetzt = (n != nullptr && n->args.size() > 1) ? n->args[1].text : "";
        diag::detail("Knopftest: MOVE vorher \"" + *moveVorher + "\" nachher \"" + jetzt + "\"");
        return !moveVorher->empty() && jetzt != *moveVorher && g_app->gizmoOffset[0] == 0.0F;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z nimmt das Zurueckschreiben zurueck", [=] {
        const Path w = g_app->selectedKey < 0 || static_cast<std::size_t>(g_app->selectedKey) >= g_app->keyMarks.size()
                           ? Path{}
                           : g_app->keyMarks[static_cast<std::size_t>(g_app->selectedKey)].path;
        const Node* n = w.empty() ? nullptr : nodeAt(g_app->doc.script(), w);
        return n != nullptr && n->args.size() > 1 && n->args[1].text == *moveVorher;
    }));
    add(klickAn("rechts kurz (Gizmomenue)", ansichtMitte, 1));
    add(klick(tr(Str::GizmoSelect), "##Popup"));
    // Unten links: oben links steht seit rc588 die Kameraleiste.
    add(klickAn("ins Leere", [] { return ImVec2{g_app->kartenAnsicht[0] + 20.0F, g_app->kartenAnsicht[3] - 20.0F}; }));
    add(pruefe("Klick ins Leere hebt Kamera- und Entitywahl auf",
               [] { return !g_app->cameraSelected && g_app->pickedEntity < 0; }));

    // ---- Insert camera here ----------------------------------------------
    auto anzahl0 = std::make_shared<std::size_t>(0);
    add(tu("Erste Skriptzeile waehlen", [=] {
        selectByPath(Path{0});
        *anzahl0 = g_app->doc.script().nodes.size();
    }));
    add(klick(tr(Str::MapInsertCamera)));
    add(pruefe("Insert camera here: MOVE und PAN mit der Kamerastelle dahinter", [=] {
        const auto& ns = g_app->doc.script().nodes;
        if (ns.size() != *anzahl0 + 2 || ns.size() < 3) { return false; }
        const Node& a = ns[1];
        const Node& b = ns[2];
        float v[3];
        const bool ok = a.name == "camera" && a.args.size() >= 2 && a.args[0].text == "MOVE" &&
                        b.name == "camera" && b.args.size() >= 2 && b.args[0].text == "PAN" &&
                        leseVektor(a.args[1].text, v) && std::fabs(v[0] - g_app->cam.pos[0]) < 1.0F;
        return ok;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z nimmt die eingefuegte Kamera zurueck",
               [=] { return g_app->doc.script().nodes.size() == *anzahl0; }));

    // ---- Entityliste links -------------------------------------------------
    auto marker0 = std::make_shared<std::size_t>(0);
    add(tu("Kreuze merken", [=] { kameraVorDenSchluessel(); }));
    add(gezeichnet("Kreuze"));
    add(tu("Anzahl Kreuze merken", [=] { *marker0 = g_app->markers.size(); }));
    add(klick("O", "", 0));
    add(gezeichnet("Klasse ausgeblendet"));
    add(pruefe("Auge einer Klasse blendet sie aus (weniger Kreuze)", [=] {
        diag::detail("Knopftest: Kreuze " + std::to_string(*marker0) + " -> " + std::to_string(g_app->markers.size()) +
                     ", ausgeblendete Klassen " + std::to_string(g_app->hiddenClasses.size()));
        return !g_app->hiddenClasses.empty() && g_app->markers.size() < *marker0;
    }));
    add(klick(tr(Str::EntityShowAll)));
    add(gezeichnet("alle wieder"));
    add(pruefe("Show all zeigt alles wieder", [=] {
        return g_app->hiddenClasses.empty() && g_app->hiddenEntities.empty() && g_app->markers.size() == *marker0;
    }));

    // ---- Zeitleiste --------------------------------------------------------
    add(tu("Zeitleiste auf Anfang", [] { g_app->playMs = 0.0; g_app->playing = false; }));
    add(klick(tr(Str::TlPlay), "###main"));
    add(pause(0.6));
    add(pruefe("Play: die Zeit laeuft", [] { return g_app->playing && g_app->playMs > 100.0; }));
    add(klick(tr(Str::TlStop), "###main"));
    add(pruefe("Stop haelt an", [] { return !g_app->playing; }));
    auto ms0 = std::make_shared<double>(0);
    add(tu("Zeit merken", [=] { *ms0 = g_app->playMs; }));
    add(pause(0.3));
    add(pruefe("Angehalten bleibt die Zeit stehen", [=] { return g_app->playMs == *ms0; }));
    add(klick(tr(Str::TlRewind), "###main"));
    add(pruefe("To start: Zeit auf null", [] { return g_app->playMs < 0.5; }));
    for (const auto& [sw, wert] : std::vector<std::pair<Str, bool*>>{
             {Str::TlAudio, &g_app->playAudio}, {Str::TlFollow, &g_app->followCam}, {Str::TlTracks, &g_app->showTracks}}) {
        auto v0 = std::make_shared<bool>(false);
        const std::string name = tr(sw);
        bool* w = wert;
        add(tu("Stand " + name, [=] { *v0 = *w; }));
        add(klick(name, "###main"));
        add(pruefe("Zeitleiste: " + name + " schaltet um", [=] { return *w != *v0; }));
        add(klick(name, "###main"));
        add(pruefe("Zeitleiste: " + name + " zurueck", [=] { return *w == *v0; }));
    }
    auto einheit0 = std::make_shared<bool>(false);
    add(tu("Einheit merken", [=] { *einheit0 = g_app->timelineFrames; }));
    add(klick(g_app->timelineFrames ? tr(Str::TlUnitFrames) : tr(Str::TlUnitSeconds), "###main"));
    add(pruefe("Frames/Seconds schaltet die Anzeige um", [=] { return g_app->timelineFrames != *einheit0; }));
    add(tu("Einheit zurueck", [=] { g_app->timelineFrames = *einheit0; }));
    add(klick(std::string(tr(Str::TlZoomMenu)) + ":", "###main", 0, true));
    add(klick("4x", "##Popup", 0, true));
    add(pruefe("Zoom-Menue: 4x", [] { return std::fabs(g_app->timelineZoom - 4.0F) < 0.01F; }));
    add(klick(std::string(tr(Str::TlZoomMenu)) + ":", "###main", 0, true));
    add(klick(tr(Str::TlFit), "##Popup"));
    add(pruefe("Zoom-Menue: All", [] { return std::fabs(g_app->timelineZoom - 1.0F) < 0.01F; }));
    // Der Zeiger steht seit 27.09. im Lineal: dort ziehen setzt die Zeit.
    add(ziehenAn("Abspielzeiger (im Lineal)", [] {
        const Element* e = finde("##zeitlineal", "###main", 0, false);
        return e != nullptr ? ImVec2{e->rect.Min.x + 6.0F, e->rect.GetCenter().y} : ImVec2{0, 0};
    }, 300.0F, 0.0F, 0));
    add(pruefe("Abspielzeiger ziehen setzt die Zeit (angehalten)", [] {
        diag::detail("Knopftest: playMs nach dem Ziehen " + std::to_string(g_app->playMs));
        return g_app->playMs > 100.0 && !g_app->playing;
    }));

    // ---- Reiterwechsel: die Karte bleibt, jeder Reiter hat seine Kamera ----
    auto reiter0 = std::make_shared<int>(-1);
    add(tu("Zu einem anderen Missionsreiter", [=] {
        *reiter0 = g_app->activeTab;
        g_app->tabRequest = (g_app->activeTab + 1) % static_cast<int>(g_app->tabs.size());
    }));
    add(pause(0.4));
    add(pruefe("Reiterwechsel: Karte weiter geladen und gezeichnet",
               [] { return !g_app->geo.empty() && g_app->mapBereit; }));
    add(tu("Zurueck zum ersten Reiter", [=] { g_app->tabRequest = *reiter0; }));
    add(pause(0.4));

    // ---- Through camera "am Abspielzeiger": keine falschen Klickziele ----
    add(tu("Keine Kamerazeile waehlen, Through camera an", [] {
        selectByPath(Path{0});
        g_app->playMs = 2000.0;
        g_app->throughCamera = true;
        g_app->mapDirty = true;
    }));
    add(gezeichnet("Kamerablick am Zeiger"));
    add(pruefe("Through camera am Zeiger: Kamerablick aktiv, keine Gizmos/Klickziele der freien Kamera", [] {
        return g_app->camViewActive && g_app->markers.empty() && g_app->keyMarks.empty() &&
               g_app->cameraSx < 0.0F;
    }));
    add(tu("Through camera aus", [] { g_app->throughCamera = false; g_app->mapDirty = true; }));
    add(gezeichnet("frei"));

    // ---- Effekte haengen an der Karte, nicht an den Figuren ---------------
    auto effekteMit = std::make_shared<std::size_t>(0);
    auto reiterMit = std::make_shared<int>(-1);
    add(tu("Effekte merken", [=] {
        // Ohne die fx_runner, die erst ein use des Skripts startet
        // (START_OFF/ONESHOT, MoverSim::fxStarts) und ohne die Brucheffekte
        // (bruchFxZahl) - die haengen am Skript und fehlen in einem anderen
        // Reiter zu Recht.
        *effekteMit = g_app->effectRunners.size() - g_app->moverSim.fxStarts().size() - g_app->bruchFxZahl -
                      g_app->roffFxZahl;
        *reiterMit = g_app->activeTab;
        diag::detail("Knopftest: Effekte mit Figuren " + std::to_string(*effekteMit) + ", Figuren " +
                     std::to_string(g_app->scene.actors.size()));
        // Ein Reiter derselben Mission, dessen Skript keine Figuren hat.
        for (int i = 0; i < static_cast<int>(g_app->tabs.size()); ++i) {
            if (i == g_app->activeTab) { continue; }
            const Script& sc = g_app->tabs[static_cast<std::size_t>(i)].doc.script();
            bool figuren = false;
            for (const Node& n : sc.nodes) { if (n.name == "affect") { figuren = true; } }
            if (!figuren && !sc.nodes.empty()) { g_app->tabRequest = i; break; }
        }
    }));
    add(pause(0.6));
    add(pruefe("Reiter ohne Figuren: die Effekte der Karte (ohne die per use gestarteten) sind trotzdem geladen", [=] {
        diag::detail("Knopftest: Reiter " + std::to_string(g_app->activeTab) + ", Figuren " +
                     std::to_string(g_app->scene.actors.size()) + ", Effekte " +
                     std::to_string(g_app->effectRunners.size()));
        return g_app->activeTab == *reiterMit ||
               g_app->effectRunners.size() - g_app->moverSim.fxStarts().size() - g_app->bruchFxZahl -
                   g_app->roffFxZahl ==
               *effekteMit;
    }));
    add(tu("Zurueck zum Reiter mit Figuren", [=] { g_app->tabRequest = *reiterMit; }));
    add(pause(0.5));

    // ---- Klaenge nach dem Ende der Kamerabahn? ---------------------------
    add(pruefe("Befund: Klaenge hinter dem Abspielende (werden nie gespielt)", [] {
        int danach = 0;
        const double ende = g_app->camTrack.durationMs;
        for (const TimelineTrack& tr : g_app->timeline.tracks) {
            for (const TimelineEvent& e : tr.events) {
                if (e.kind == TimelineEvent::Kind::Sound && e.startMs > ende + 1.0 &&
                    e.startMs < g_app->timeline.practicalEndMs + 1.0) {
                    if (++danach <= 5) {
                        diag::detail("Knopftest: Klang nach dem Kameraende: " + e.sound + " bei " +
                                     std::to_string(e.startMs) + " ms (Ende " + std::to_string(ende) + ")");
                    }
                }
            }
        }
        diag::detail("Knopftest: Kameraende " + std::to_string(ende) + " ms, praktisches Ende " +
                     std::to_string(g_app->timeline.practicalEndMs) + " ms, Klaenge danach " +
                     std::to_string(danach));
        return danach == 0;
    }));

    // ---- Karte verlassen beim Abspielen haelt an --------------------------
    add(tu("Abspielen und in die Skriptansicht wechseln", [] {
        g_app->playMs = 0.0;
        g_app->playing = true;
    }));
    add(pause(0.3));
    add(tu("Modus Events", [] { g_app->leftMode = 0; }));
    add(pause(0.2));
    add(pruefe("Karte verlassen haelt das Abspielen an (keine stehende Uhr mit laufendem Ton)",
               [] { return !g_app->playing; }));
    add(tu("Zurueck in die Kartenansicht", [] { g_app->leftMode = 1; }));
    add(pause(0.3));

    // ---- Debug-Menue ohne RenderDoc ---------------------------------------
    add(pause(0.3));
    // Im Hauptfenster: ImGuis unsichtbares Fenster "Debug##Default" traegt
    // denselben Namen und liegt bei 60/60 - mitten auf der Entityliste.
    add(klick(tr(Str::MenuDebug), "###main"));
    add(pruefe("Debug > Show layout numbers ist da (ohne RenderDoc bedienbar)", [] {
        return finde(tr(Str::DebugLayout), "##Menu", 0, false) != nullptr;
    }));
    add(klick(tr(Str::DebugLayout), "##Menu"));
    add(pruefe("Show layout numbers oeffnet das Fenster", [] { return g_app->layoutOpen; }));
    add(tu("Layoutfenster zu", [] { g_app->layoutOpen = false; }));

    // ---- Entityliste: Klasse aufklappen, Mitglied waehlen -----------------
    auto mitglied = std::make_shared<std::string>();
    auto mitgliedIndex = std::make_shared<int>(-1);
    add(tu("Erstes benanntes waypoint_navgoal suchen", [=] {
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            if (e.classname == "waypoint_navgoal" && !e.targetname.empty()) {
                *mitglied = e.targetname;
                *mitgliedIndex = static_cast<int>(i);
                break;
            }
        }
        g_app->pickedEntity = -1;
    }));
    // Das + vor der Gruppe (rc588) klappt auf, das - wieder zu.
    add(klickAn("+/- vor \"Laufziele\"", [] {
        const Element* gruppe = finde(tr(Str::EntityKatNav), "", 0, false);
        for (int k = 0; gruppe != nullptr; ++k) {
            const Element* e = finde("##klapp", "", k, false);
            if (e == nullptr) { break; }
            if (std::fabs(e->rect.GetCenter().y - gruppe->rect.GetCenter().y) < 4.0F) { return e->rect.GetCenter(); }
        }
        return ImVec2{0.0F, 0.0F};
    }));
    add(pruefe("+ vor \"Laufziele\" klappt die Gruppe auf", [] { return g_app->expandedClass == "kat1"; }));
    add(klickAn("+/- vor \"Laufziele\"", [] {
        const Element* gruppe = finde(tr(Str::EntityKatNav), "", 0, false);
        for (int k = 0; gruppe != nullptr; ++k) {
            const Element* e = finde("##klapp", "", k, false);
            if (e == nullptr) { break; }
            if (std::fabs(e->rect.GetCenter().y - gruppe->rect.GetCenter().y) < 4.0F) { return e->rect.GetCenter(); }
        }
        return ImVec2{0.0F, 0.0F};
    }));
    add(pruefe("- klappt sie wieder zu", [] { return g_app->expandedClass.empty(); }));
    add(klick(tr(Str::EntityKatNav), "", 0, true));
    add(pruefe("Klick auf die Gruppe \"Laufziele\" klappt sie auf",
               [] { return g_app->expandedClass == "kat1"; }));
    add({"Klick auf das Mitglied", [=](int b) {
             static Schritt k;
             if (b == 0) { k = klick(*mitglied); }
             return k.tun(b);
         }});
    add(pruefe("Klick auf ein Mitglied der Liste waehlt diese Entity", [=] {
        diag::detail("Knopftest: Mitglied " + *mitglied + " (" + std::to_string(*mitgliedIndex) + "), gewaehlt " +
                     std::to_string(g_app->pickedEntity));
        return *mitgliedIndex >= 0 && g_app->pickedEntity == *mitgliedIndex;
    }));
    // Rechtsklick auf das Mitglied: "Als Laufziel einfuegen" setzt ein
    // set ( SET_NAVGOAL, <name> ) hinter die gewaehlte Zeile.
    auto zeilen0 = std::make_shared<std::size_t>(0);
    add(tu("Zeilen vor dem Einfuegen zaehlen", [=] { *zeilen0 = g_app->rows.size(); }));
    add({"Rechtsklick auf das Mitglied", [=](int b) {
             static Schritt k;
             if (b == 0) { k = klick(*mitglied, "", 0, false, "", 1); }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(klick(tr(Str::EntityAsNavgoal)));
    add(pause(0.3));
    add(pruefe("Rechtsklick > Als Laufziel einfuegen setzt SET_NAVGOAL mit dem Namen ein", [=] {
        bool da = false;
        std::function<void(const std::vector<Node>&)> such = [&](const std::vector<Node>& ns) {
            for (const Node& n : ns) {
                if (n.name == "set" && n.args.size() >= 2 && n.args[0].text == "SET_NAVGOAL" &&
                    n.args[1].text == *mitglied) {
                    da = true;
                }
                such(n.children);
            }
        };
        such(g_app->doc.script().nodes);
        diag::detail("Knopftest: Zeilen " + std::to_string(*zeilen0) + " -> " + std::to_string(g_app->rows.size()));
        return da && g_app->rows.size() == *zeilen0 + 1;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z nimmt das eingefuegte Laufziel zurueck", [=] { return g_app->rows.size() == *zeilen0; }));
    // Doppelklick fliegt hin: die freie Kamera steht danach nahe der Entity.
    add({"Doppelklick auf das Mitglied", [=](int b) {
             static Schritt k;
             if (b == 0) {
                 const Element* el = finde(*mitglied, "", 0, false, "");
                 const ImVec2 c = (el != nullptr) ? el->rect.GetCenter() : ImVec2{0.0F, 0.0F};
                 k = klickAn("Doppelklick auf " + *mitglied, [c] { return c; }, 0, true);
             }
             return k.tun(b);
         }});
    add(pause(0.3));
    add(pruefe("Doppelklick auf eine Entity fliegt die Kamera hin", [=] {
        if (*mitgliedIndex < 0) { return false; }
        float p[3];
        if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(*mitgliedIndex)], p)) { return false; }
        const float dx = p[0] - g_app->cam.pos[0];
        const float dy = p[1] - g_app->cam.pos[1];
        const float d = std::sqrt(dx * dx + dy * dy);
        diag::detail("Knopftest: Abstand Kamera - Entity " + std::to_string(d));
        return d < 250.0F;
    }));
    add(tu("Klasse wieder zu", [] { g_app->expandedClass.clear(); }));

    // ---- Gizmo drehen und zurueckschreiben --------------------------------
    add(tu("Kamera vor den Schluessel, Kamera waehlen", [] {
        kameraVorDenSchluessel();
        g_app->cameraSelected = false;
    }));
    add(gezeichnet("vor dem Schluessel"));
    add(klickAn("auf die Kamera", [] { return bildZuSchirm(g_app->cameraSx, g_app->cameraSy); }));
    add(klickAn("rechts kurz (Gizmomenue)", ansichtMitte, 1));
    add(klick(tr(Str::GizmoRotate), "##Popup"));
    add(pruefe("Gizmo: Modus Drehen", [] { return g_app->gizmoMode == App::GizmoMode::Rotate; }));
    // rc599: "bei Drehen ein Rotation-Target" - die drei Ringe muessen da sein
    // und jeder einzeln greifen.
    add(gezeichnet("Drehringe"));
    add(foto("kamera_drehringe"));
    for (int achse = 0; achse < 3; ++achse) {
        add(tu("Kamera-Drehring " + std::to_string(achse) + ": Maus drauf", [achse] {
            // Der Punkt des Rings, der am weitesten von den anderen beiden liegt.
            int best = -1;
            float bestAbst = -1.0F;
            for (int t = 0; t + 1 < App::kRingPts; ++t) {
                const float x = g_app->gizmoRingSx[achse][t];
                const float y = g_app->gizmoRingSy[achse][t];
                if (x < 0.0F) { continue; }
                float naechst = 1.0e9F;
                for (int b = 0; b < 3; ++b) {
                    if (b == achse) { continue; }
                    for (int u = 0; u + 1 < App::kRingPts; ++u) {
                        if (g_app->gizmoRingSx[b][u] < 0.0F) { continue; }
                        const float dx = g_app->gizmoRingSx[b][u] - x;
                        const float dy = g_app->gizmoRingSy[b][u] - y;
                        naechst = std::min(naechst, dx * dx + dy * dy);
                    }
                }
                if (naechst > bestAbst) {
                    bestAbst = naechst;
                    best = t;
                }
            }
            if (best >= 0) { setzeMaus(bildZuSchirm(g_app->gizmoRingSx[achse][best], g_app->gizmoRingSy[achse][best])); }
            g_app->mapDirty = true;
        }));
        add(pause(0.25));
        add(pruefe("Kamera-Drehring " + std::to_string(achse) + " greift", [achse] {
            diag::detail("Knopftest: Kamera-Drehring " + std::to_string(achse) + " - unter der Maus " +
                         std::to_string(g_app->gizmoHover));
            return g_app->gizmoHover == achse;
        }));
    }
    auto panVorher = std::make_shared<std::string>();
    auto panWeg = std::make_shared<Path>();
    add(tu("Um 15 Grad drehen (wie gezogen)", [=] {
        // Das PAN nach dem gewaehlten Schluessel (bis zum naechsten MOVE).
        panWeg->clear();
        if (g_app->selectedKey >= 0 && static_cast<std::size_t>(g_app->selectedKey) < g_app->keyMarks.size()) {
            Path w = g_app->keyMarks[static_cast<std::size_t>(g_app->selectedKey)].path;
            const auto& ns = g_app->doc.script().nodes;
            for (std::size_t i = w.empty() ? 0 : w.back() + 1; w.size() == 1 && i < ns.size(); ++i) {
                if (ns[i].name != "camera" || ns[i].args.empty()) { continue; }
                if (ns[i].args[0].text == "MOVE") { break; }
                if (ns[i].args[0].text == "PAN") { *panWeg = Path{i}; break; }
            }
        }
        const Node* n = panWeg->empty() ? nullptr : nodeAt(g_app->doc.script(), *panWeg);
        *panVorher = (n != nullptr && n->args.size() > 1) ? n->args[1].text : "";
        g_app->gizmoAngles[1] = 15.0F;
        g_app->mapDirty = true;
    }));
    add(gezeichnet("Drehvorschau"));
    add(tu("Schreiben wie beim Loslassen (rc588: kein Apply-Knopf mehr)", [] { kameraVorschauSchreiben(); }));
    add(pruefe("Write to script (Drehung): das PAN nach dem Schluessel ist geaendert", [=] {
        const Node* n = panWeg->empty() ? nullptr : nodeAt(g_app->doc.script(), *panWeg);
        const std::string jetzt = (n != nullptr && n->args.size() > 1) ? n->args[1].text : "";
        diag::detail("Knopftest: PAN vorher \"" + *panVorher + "\" nachher \"" + jetzt + "\"");
        return !panVorher->empty() && jetzt != *panVorher;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z nimmt die Drehung zurueck", [=] {
        const Node* n = panWeg->empty() ? nullptr : nodeAt(g_app->doc.script(), *panWeg);
        return n != nullptr && n->args.size() > 1 && n->args[1].text == *panVorher;
    }));
    add(klickAn("rechts kurz (Gizmomenue)", ansichtMitte, 1));
    add(klick(tr(Str::GizmoSelect), "##Popup"));
    add(tu("Kamera abwaehlen", [] { g_app->cameraSelected = false; g_app->mapDirty = true; }));

    // ---- Karte schliessen, ueber den .pk3-Browser neu laden ---------------
    add(klick(tr(Str::MenuFile), "###main"));
    add(klick(tr(Str::MapClose), "##Menu"));
    add(pause(0.3));
    add(pruefe("File > Close map: keine Karte mehr, Hinweis 'No map loaded'", [] {
        return g_app->geo.empty() && g_app->map.empty() && g_app->markers.empty();
    }));
    add(tu("Karten & Dateien: Menue auf", [] { g_app->kartenMehrOeffnen = true; }));
    add(pause(0.3));
    add(klick(tr(Str::OpenMapPk3)));
    add(pause(0.3));
    add(pruefe("From .pk3... oeffnet den Browser mit Karten", [] { return g_app->pk3Open && g_app->pk3Kind == 1; }));
    add(klick(tr(Str::Pk3Filter)));
    add(tippe("md_am_sith"));
    add(pause(0.3));
    add(klick("maps/md_am_sith.bsp", "", 0, false, "", 0, 0.3F));
    add({"Doppelklick auf maps/md_am_sith.bsp", [](int b) {
             static Schritt k;
             if (b == 0) { k = klickAn("maps/md_am_sith.bsp", [] {
                                 const Element* e = finde("maps/md_am_sith.bsp", "", 0, false);
                                 return e != nullptr ? e->rect.GetCenter() : ImVec2{0, 0};
                             }, 0, true); }
             return k.tun(b);
         }});
    add(warteBis("Karte aus dem Browser geladen", [] { return !g_app->geo.empty() && g_app->mapBereit; }, 2000));
    add(tu("Browser zu", [] { g_app->pk3Open = false; }));
    add(pause(0.5));
    add(gezeichnet("neu geladene Karte"));
    add(pruefe("Neu geladene Karte zeichnet ohne Fehler", [] {
        diag::detail("Knopftest: nach dem Neuladen mapDirty " + std::to_string(g_app->mapDirty) + ", mapBereit " +
                     std::to_string(g_app->mapBereit) + ", gpuFehler \"" + g_app->gpuFehler + "\", Stapel " +
                     std::to_string(g_app->mesh.batches.size()));
        return !g_app->mapDirty && g_app->gpuFehler.empty() && g_app->mapBereit;
    }));

    // ---- Protokoll: keine Fehler waehrend der Kartenfunktionen ------------
    add(pruefe("Protokoll: kein Schritt mit FEHLER waehrend der Kartenfunktionen", [=] {
        const auto& z = diag::lines();
        int fehler = 0;
        for (std::size_t i = *logAb; i < z.size(); ++i) {
            if (z[i].find("  FEHLER: ") != std::string::npos) {
                if (++fehler <= 10) { diag::detail("Knopftest: im Protokoll: " + z[i]); }
            }
        }
        return fehler == 0;
    }));
    return s;
}

// --- Schwebende Fenster: Klick ausserhalb, wieder oeffnen ---------------
//
// shank: "If there is a window open and I click outside of the window, it
// disappears, but I am unable to open/view it again" - bei Notes, Keyboard
// shortcuts, Find. Das Hauptfenster schob sich bei jedem Klick nach vorn
// (ohne ImGuiWindowFlags_NoBringToFrontOnFocus), der Offen-Merker blieb
// stehen, und der Knopf setzte nur, was schon gesetzt war.
//
// Geprueft wird die Z-Reihenfolge selbst (GImGui->Windows, hinten nach
// vorn), nicht ein Bild: das Fenster muss nach dem Klick ausserhalb VOR dem
// Hauptfenster liegen, und nach Schliessen und Oeffnen wieder da sein.
namespace {
int fensterRang(const char* id) {
    ImGuiWindow* w = ImGui::FindWindowByName(id);
    if (w == nullptr) {
        return -1;
    }
    ImGuiContext& g = *ImGui::GetCurrentContext();
    for (int i = 0; i < g.Windows.Size; ++i) {
        if (g.Windows[i] == w->RootWindow) {
            return i;
        }
    }
    return -1;
}

bool fensterSichtbarVorn(const char* id) {
    ImGuiWindow* w = ImGui::FindWindowByName(id);
    const int r = fensterRang(id);
    const int h = fensterRang("###main");
    return w != nullptr && w->WasActive && !w->Hidden && r > h && h >= 0;
}

// Ein Punkt im Hauptfenster, der NICHT im Fenster `id` liegt: die
// Statuszeile unten, sonst ein Stueck weiter links.
ImVec2 ausserhalbVon(const char* id) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGuiWindow* w = ImGui::FindWindowByName(id);
    const ImVec2 kandidaten[] = {
        {vp->WorkPos.x + vp->WorkSize.x * 0.62F, vp->WorkPos.y + vp->WorkSize.y * 0.985F},
        {vp->WorkPos.x + vp->WorkSize.x * 0.35F, vp->WorkPos.y + vp->WorkSize.y * 0.985F},
        {vp->WorkPos.x + vp->WorkSize.x * 0.90F, vp->WorkPos.y + vp->WorkSize.y * 0.60F},
    };
    for (const ImVec2& k : kandidaten) {
        if (w == nullptr || !w->Rect().Contains(k)) {
            return k;
        }
    }
    return kandidaten[0];
}
}  // namespace

// Nach dem Zusammendruecken: nicht kleiner als benutzbar. Mindestens rund
// 20 Zeichen breit und 7 Zeilen hoch, und nichts seitlich abgeschnitten
// (ScrollMax.x > 0 hiesse: der Inhalt ist breiter als das Fenster). Lange
// Listen duerfen senkrecht rollen.
bool fensterNichtZuKlein(ImGuiWindow* w, const std::string& n) {
    if (w == nullptr) { return false; }
    const float fs = ImGui::GetFontSize();
    char z[240];
    std::snprintf(z, sizeof(z),
                  "Knopftest: %s zusammengedrueckt: %.0f x %.0f (min %.0f x %.0f), Rollbereich x %.0f",
                  n.c_str(), w->Size.x, w->Size.y, fs * 20.0F, fs * 7.0F, w->ScrollMax.x);
    diag::detail(z);
    // Ein Fenster, das sich selbst an den Inhalt anpasst (Find, Layout),
    // laesst sich gar nicht kleiner ziehen - es besteht, wenn nichts
    // abgeschnitten ist. Waagerecht rollen zu koennen gilt als nicht
    // abgeschnitten.
    const bool rollt = (w->Flags & ImGuiWindowFlags_HorizontalScrollbar) != 0;
    const bool abgeschnitten = w->ScrollMax.x > 0.5F && !rollt;
    if ((w->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0) {
        return !abgeschnitten;
    }
    return w->Size.x >= fs * 20.0F && w->Size.y >= fs * 7.0F && !abgeschnitten;
}

Schritt zusammendruecken(const std::string& n, std::function<ImGuiWindow*()> fenster) {
    return tu(n + ": auf 40 x 40 zusammendruecken", [=] {
        if (ImGuiWindow* w = fenster()) {
            ImGui::SetWindowSize(w, ImVec2{40.0F, 40.0F});
        }
    });
}

// --- Kuerzelfenster: eine Zeile finden und anklicken -----------------------
//
// Jeder Knopf traegt "###k_<name>" (app.cpp). Seine Beschriftung meldet ImGui
// aber nur, solange er SICHTBAR ist - also wird das Fenster von oben nach
// unten gerollt, bis die Zeile ganz im Bild steht.
const Element* findeEndung(const std::string& endung, const std::string& fenster) {
    for (const Element& e : g_elemente) {
        if (e.label.size() < endung.size() ||
            e.label.compare(e.label.size() - endung.size(), endung.size(), endung) != 0) {
            continue;
        }
        if (!fenster.empty() && e.fenster.find(fenster) == std::string::npos &&
            e.innen.find(fenster) == std::string::npos) {
            continue;
        }
        return &e;
    }
    return nullptr;
}

Schritt kuerzelZeileSuchen(const std::string& name) {
    return {"Kuerzelzeile " + name + " ins Bild rollen", [=](int b) {
                ImGuiWindow* w = ImGui::FindWindowByName("###keys");
                if (w == nullptr) { return b > 60; }
                const Element* e = findeEndung("###k_" + name, "###keys");
                if (e != nullptr && e->rect.Min.y >= w->InnerRect.Min.y &&
                    e->rect.Max.y <= w->InnerRect.Max.y - 4.0F && b > 1) {
                    return true;
                }
                if (b == 0) {
                    ImGui::SetScrollY(w, 0.0F);
                } else if (b % 2 == 0) {
                    ImGui::SetScrollY(w, w->Scroll.y + w->InnerRect.GetHeight() * 0.4F);
                }
                if (b > 120) {
                    diag::detail("Knopftest: Kuerzelzeile " + name + " nicht gefunden");
                    return true;
                }
                return false;
            }};
}

Schritt kuerzelZeileKlicken(const std::string& name) {
    return klickAn("Kuerzelzeile " + name, [=] {
        const Element* e = findeEndung("###k_" + name, "###keys");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    });
}

// Eine Taste mit beliebig vielen Modifikatoren (Strg, Umschalt, Alt).
Schritt tasteKombi(ImGuiKeyChord chord, const std::string& name) {
    return {"Taste " + name, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                const auto key = static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);
                const bool strg = (chord & ImGuiMod_Ctrl) != 0;
                const bool umsch = (chord & ImGuiMod_Shift) != 0;
                const bool alt = (chord & ImGuiMod_Alt) != 0;
                if (b == 0) {
                    if (strg) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
                    if (umsch) { io.AddKeyEvent(ImGuiMod_Shift, true); }
                    if (alt) { io.AddKeyEvent(ImGuiMod_Alt, true); }
                    return false;
                }
                if (b == 1) { io.AddKeyEvent(key, true); return false; }
                if (b == 2) { io.AddKeyEvent(key, false); return false; }
                if (b == 3) {
                    if (strg) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
                    if (umsch) { io.AddKeyEvent(ImGuiMod_Shift, false); }
                    if (alt) { io.AddKeyEvent(ImGuiMod_Alt, false); }
                    return false;
                }
                return b >= 6;
            }};
}

// --- JEDE Belegung aendern und ausprobieren ------------------------------
//
// shank: "kannst du in die tests einbauen, dass nicht nur Suche in den
// Hotkeys geaendert und getestet wird, sondern jedes andere auch".
//
// Je Aktion: die Zeile im Kuerzelfenster ANKLICKEN, eine neue, eindeutige
// Kombination DRUECKEN, pruefen, dass sie aufgenommen ist, das Fenster
// schliessen, die neue Kombination druecken und die WIRKUNG pruefen, dann
// die alte druecken - die darf nichts mehr ausloesen. Am Ende "Reset all to
// defaults" und alle Vorgaben wieder da.
//
// Open und Save as oeffnen einen Windows-Dialog, Save all saehe andere
// Reiter - fuer die drei meldet behaved im Test nur, was ausgeloest wurde
// (App::kuerzelNurMelden).
//
// Gefunden damit: Backup und Restore liessen sich belegen, die Belegung tat
// aber nichts.
const char* const kKuerzelSkript =
    "rem ( \"kuerzel\" );\n"
    "camera ( /*@CAMERA_COMMANDS*/ MOVE, < 10.000 20.000 40.000 >, 0 );\n"
    "wait ( 100.000 );\n"
    "camera ( /*@CAMERA_COMMANDS*/ MOVE, < 10.000 20.000 40.000 >, 0 );\n"
    "wait ( 200.000 );\n"
    "camera ( /*@CAMERA_COMMANDS*/ MOVE, < 50.000 20.000 40.000 >, 0 );\n"
    "affect ( \"x\", /*@AFFECT_TYPE*/ FLUSH )\n"
    "{\n"
    "\twait ( 300.000 );\n"
    "}\n"
    "wait ( 400.000 );\n"
    "//(BHVDREM)  wait ( 999.000 );\n";

std::string kuerzelDatei() {
    return (std::filesystem::current_path() / "selbsttest_kuerzel.txt").string();
}

void kuerzelSkriptZurueck(const Path& auswahl) {
    Script sc;
    std::vector<Diag> d;
    (void)readScript(std::string("//Generated by BehavEd\n") + kKuerzelSkript, sc, d);
    (void)g_app->doc.replaceAll(sc, "kuerzeltest");
    g_app->selection.clear();
    selectByPath(auswahl);
    g_app->findOpen = false;
    g_app->editorOpen = false;
    g_app->keysOpen = false;
    g_app->letzteKuerzelAktion = keys::Action::Unhandled;
}

std::string kuerzelBak() {
    return (std::filesystem::current_path() / "selbsttest_kuerzel.bak").string();
}

std::size_t kuerzelKnoten() { return g_app->doc.script().nodes.size(); }

std::vector<Schritt> kuerzelAlleSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vorgaben = std::make_shared<std::map<int, ImGuiKeyChord>>();
    add(tu("Kuerzeltest: eigenes Skript mit Datei, Vorgaben merken", [=] {
        g_app->settings.keyBindings.clear();   // von den Vorgaben aus
        {
            std::ofstream f(kuerzelDatei(), std::ios::binary);
            f << "//Generated by BehavEd\n" << kKuerzelSkript;
        }
        std::error_code ec;
        std::filesystem::remove(kuerzelBak(), ec);
        openScriptFromMemory(std::string("//Generated by BehavEd\n") + kKuerzelSkript,
                             "selbsttest_kuerzel.txt");
        g_app->path = kuerzelDatei();
        g_app->kuerzelNurMelden = true;
        for (const keys::Bindable& b : keys::bindable()) {
            (*vorgaben)[static_cast<int>(b.action)] = kuerzelVon(b.action);
        }
    }));
    add(pause(0.5));

    const ImGuiKeyChord gruppen[3] = {ImGuiMod_Ctrl | ImGuiMod_Shift, ImGuiMod_Ctrl | ImGuiMod_Alt,
                                      ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiMod_Alt};
    int nr = 0;
    for (const keys::Bindable& b : keys::bindable()) {
        const keys::Action a = b.action;
        const std::string name = b.name;
        const ImGuiKeyChord neu = gruppen[(nr / 12) % 3] | static_cast<ImGuiKey>(ImGuiKey_F1 + nr % 12);
        ++nr;
        const std::string neuName = ImGui::GetKeyChordName(neu);

        // --- Belegen, ueber das Fenster ----------------------------------
        add(tu(name + ": Kuerzelfenster oeffnen", [] { fensterZeigen(g_app->keysOpen, "###keys"); }));
        add(pause(0.25));
        add(kuerzelZeileSuchen(name));
        add(kuerzelZeileKlicken(name));
        add(pause(0.15));
        add(pruefe(name + ": die Zeile wartet auf eine Taste", [] { return g_app->kuerzelWartet; }));
        add(tasteKombi(neu, neuName));
        add(pause(0.15));
        add(pruefe(name + ": neue Belegung " + neuName + " aufgenommen", [=] {
            return kuerzelVon(a) == neu;
        }));

        // --- Vorbereiten, ausloesen, Wirkung pruefen -----------------------
        auto vorher = std::make_shared<std::size_t>(0);
        std::function<void()> vorbereiten;
        std::function<bool()> wirkung;
        switch (a) {
            case keys::Action::Delete:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] { return kuerzelKnoten() == 8U; };
                break;
            case keys::Action::Clone:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] { return kuerzelKnoten() == 10U; };
                break;
            case keys::Action::Copy:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] { return kuerzelKnoten() == 9U && g_app->letzteKuerzelAktion == keys::Action::Copy; };
                break;
            case keys::Action::Cut:
            case keys::Action::CutAlt:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] { return kuerzelKnoten() == 8U; };
                break;
            case keys::Action::Paste:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); kopieren(false); };
                wirkung = [] { return kuerzelKnoten() == 10U; };
                break;
            case keys::Action::CommentOut:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] {
                    const auto& ns = g_app->doc.script().nodes;
                    return ns.size() > 2 && ns[2].kind == Node::Kind::LineComment;
                };
                break;
            case keys::Action::Uncomment:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{8}); };
                wirkung = [] {
                    const auto& ns = g_app->doc.script().nodes;
                    return ns.size() > 8 && ns[8].kind == Node::Kind::Command && ns[8].name == "wait";
                };
                break;
            case keys::Action::Find:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] { return g_app->findOpen; };
                break;
            case keys::Action::FindRepeat:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{0});
                    g_app->findLetzte = FindOptions{};
                    g_app->findLetzte.named = "wait";
                };
                wirkung = [] { return g_app->selectedPath == Path{2}; };
                break;
            case keys::Action::FindPrevious:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{4});
                    g_app->findLetzte = FindOptions{};
                    g_app->findLetzte.named = "wait";
                };
                wirkung = [] { return g_app->selectedPath == Path{2}; };
                break;
            case keys::Action::ExpandNode:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{6}); g_app->expanded.closeAll(); rebuildTree(); };
                wirkung = [] { return g_app->expanded.isOpen(kennungAn(Path{6})); };
                break;
            case keys::Action::CollapseNode:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{6}); g_app->expanded.openAll(); rebuildTree(); };
                wirkung = [] { return !g_app->expanded.isOpen(kennungAn(Path{6})); };
                break;
            case keys::Action::ExpandAll:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{0}); g_app->expanded.closeAll(); rebuildTree(); };
                wirkung = [] { return g_app->expanded.isOpen(kennungAn(Path{6})); };
                break;
            case keys::Action::CollapseAll:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{0}); g_app->expanded.openAll(); rebuildTree(); };
                wirkung = [] { return !g_app->expanded.isOpen(kennungAn(Path{6})); };
                break;
            case keys::Action::MoveUp:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{3}); };
                wirkung = [] {
                    const auto& ns = g_app->doc.script().nodes;
                    return ns.size() > 3 && ns[2].name == "camera" && ns[3].name == "wait";
                };
                break;
            case keys::Action::MoveDown:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] {
                    const auto& ns = g_app->doc.script().nodes;
                    return ns.size() > 3 && ns[2].name == "camera" && ns[3].name == "wait";
                };
                break;
            case keys::Action::Undo:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); (void)g_app->doc.removeAt(Path{2}); rebuildTree(); };
                wirkung = [] { return kuerzelKnoten() == 9U; };
                break;
            case keys::Action::Redo:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{2});
                    (void)g_app->doc.removeAt(Path{2});
                    (void)g_app->doc.undo();
                    rebuildTree();
                };
                wirkung = [] { return kuerzelKnoten() == 8U; };
                break;
            case keys::Action::BookmarkToggle:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] { return hatLesezeichen(g_app->activeTab, kennungAn(Path{2})); };
                break;
            case keys::Action::BookmarkNext:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{4});
                    lesezeichenUmschalten({Path{4}});
                    selectByPath(Path{0});
                };
                wirkung = [] { return g_app->selectedPath == Path{4}; };
                break;
            case keys::Action::BookmarkPrev:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{4});
                    lesezeichenUmschalten({Path{4}});
                    selectByPath(Path{7});
                };
                wirkung = [] { return g_app->selectedPath == Path{4}; };
                break;
            case keys::Action::EditItem:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [] { return g_app->editorOpen; };
                break;
            case keys::Action::InsertItem:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{2});
                    for (std::size_t c = 0; c < g_app->db.commands.size(); ++c) {
                        if (g_app->db.commands[c].name == "wait") {
                            g_app->selectedCommand = static_cast<int>(c);
                            break;
                        }
                    }
                };
                wirkung = [] { return kuerzelKnoten() == 10U; };
                break;
            case keys::Action::Save:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{2});
                    (void)g_app->doc.removeAt(Path{7});   // anders als die Datei
                    rebuildTree();
                };
                wirkung = [] {
                    std::ifstream f(kuerzelDatei(), std::ios::binary);
                    std::ostringstream ss;
                    ss << f.rdbuf();
                    return !g_app->doc.dirty() && ss.str().find("400.000") == std::string::npos;
                };
                break;
            case keys::Action::Backup:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{2});
                    std::error_code ec;
                    std::filesystem::remove(kuerzelBak(), ec);
                };
                wirkung = [] { return std::filesystem::exists(kuerzelBak()); };
                break;
            case keys::Action::Restore:
                vorbereiten = [] {
                    kuerzelSkriptZurueck(Path{2});
                    std::ofstream f(kuerzelBak(), std::ios::binary);
                    f << "//Generated by BehavEd\n" << kKuerzelSkript;
                };
                wirkung = [] { return g_app->frageOffen; };   // fragt, wie das Original
                break;
            case keys::Action::Open:
            case keys::Action::SaveAs:
            case keys::Action::SaveAll:
            default:
                vorbereiten = [] { kuerzelSkriptZurueck(Path{2}); };
                wirkung = [a] { return g_app->letzteKuerzelAktion == a; };
                break;
        }
        add(tu(name + ": vorbereiten", [=] { vorbereiten(); *vorher = kuerzelKnoten(); }));
        add(pause(0.2));
        add(tasteKombi(neu, neuName));
        add(pause(0.2));
        add(pruefe(name + ": " + neuName + " loest die Aktion aus und wirkt", [=] {
            const auto& ns = g_app->doc.script().nodes;
            std::string letzte;
            if (!ns.empty()) {
                letzte = ns.back().name + "/" + std::to_string(static_cast<int>(ns.back().kind)) + "/" +
                         ns.back().raw;
            }
            diag::detail("Knopftest: " + name + " - Knoten vorher " + std::to_string(*vorher) + ", nachher " +
                         std::to_string(kuerzelKnoten()) + ", ausgeloest " +
                         std::to_string(static_cast<int>(g_app->letzteKuerzelAktion)) + ", letzter Knoten " +
                         letzte + ", Auswahl " +
                         (g_app->selectedPath.empty() ? std::string("-")
                                                      : std::to_string(g_app->selectedPath.back())));
            return g_app->letzteKuerzelAktion == a && wirkung();
        }));
        if (a == keys::Action::Restore) {
            add(klick(tr(Str::AnswerNo)));
            add(pause(0.2));
            add(pruefe("Restore: Nein laesst das Skript stehen", [] {
                return !g_app->frageOffen && kuerzelKnoten() == 9U;
            }));
        }
        // --- Die alte Belegung loest nichts mehr aus ---------------------
        const ImGuiKeyChord alt = kuerzelVon(a) == neu ? (*vorgaben)[static_cast<int>(a)] : ImGuiKey_None;
        (void)alt;
        add(tu(name + ": aufraeumen", [] {
            g_app->findOpen = false;
            g_app->editorOpen = false;
            kuerzelSkriptZurueck(Path{2});
        }));
        add(pause(0.15));
        add({name + ": alte Belegung druecken", [=](int b) {
                 // Die Vorgabe dieser Aktion - erst beim Lauf bekannt.
                 static ImGuiKeyChord merke = ImGuiKey_None;
                 if (b == 0) {
                     merke = (*vorgaben)[static_cast<int>(a)];
                     g_app->letzteKuerzelAktion = keys::Action::Unhandled;
                     if (merke == ImGuiKey_None) { return true; }
                 }
                 ImGuiIO& io = ImGui::GetIO();
                 const auto key = static_cast<ImGuiKey>(merke & ~ImGuiMod_Mask_);
                 const bool strg = (merke & ImGuiMod_Ctrl) != 0;
                 const bool umsch = (merke & ImGuiMod_Shift) != 0;
                 const bool alt2 = (merke & ImGuiMod_Alt) != 0;
                 if (b == 0) {
                     if (strg) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
                     if (umsch) { io.AddKeyEvent(ImGuiMod_Shift, true); }
                     if (alt2) { io.AddKeyEvent(ImGuiMod_Alt, true); }
                     return false;
                 }
                 if (b == 1) { io.AddKeyEvent(key, true); return false; }
                 if (b == 2) { io.AddKeyEvent(key, false); return false; }
                 if (b == 3) {
                     if (strg) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
                     if (umsch) { io.AddKeyEvent(ImGuiMod_Shift, false); }
                     if (alt2) { io.AddKeyEvent(ImGuiMod_Alt, false); }
                     return false;
                 }
                 return b >= 6;
             }});
        add(pruefe(name + ": die alte Belegung loest es nicht mehr aus", [=] {
            return g_app->letzteKuerzelAktion != a;
        }));
        add(tu(name + ": aufraeumen", [] {
            g_app->findOpen = false;
            g_app->editorOpen = false;
            g_app->frageOffen = false;
        }));
    }

    // --- Reset all to defaults: alles wieder wie vorher -----------------
    add(tu("Kuerzelfenster nach vorn", [] { fensterZeigen(g_app->keysOpen, "###keys"); }));
    add(pause(0.3));
    add(tu("Kuerzelfenster ans Ende rollen", [] {
        if (ImGuiWindow* w = ImGui::FindWindowByName("###keys")) {
            ImGui::SetScrollY(w, w->ScrollMax.y + 1000.0F);
        }
    }));
    add(pause(0.3));
    add(klick(tr(Str::KeysReset), "###keys"));
    add(pause(0.2));
    add(pruefe("Reset all to defaults: JEDE Aktion hat wieder ihre Vorgabe", [=] {
        int anders = 0;
        for (const keys::Bindable& b : keys::bindable()) {
            if (kuerzelVon(b.action) != (*vorgaben)[static_cast<int>(b.action)]) {
                diag::detail(std::string("Knopftest: nach Reset anders: ") + b.name);
                ++anders;
            }
        }
        return anders == 0;
    }));
    add(tu("Kuerzeltest aufraeumen: Fenster zu, Dateien weg", [] {
        g_app->keysOpen = false;
        g_app->kuerzelNurMelden = false;
        g_app->doc.markSaved();
        std::error_code ec;
        std::filesystem::remove(kuerzelDatei(), ec);
        std::filesystem::remove(kuerzelBak(), ec);
        std::filesystem::remove(kuerzelDatei() + ".bak", ec);
    }));
    add(pause(0.3));
    return s;
}

// --- "Find similar" und "Highlight identical commands" ----------------------
// --- Wuensche vom 27.09. ------------------------------------------------------
//
// * Ein ANGEKLICKTER Block (affect, if, else, loop, task) nimmt neue Befehle
//   aus der Ereignisliste und aus der Ablage auf - ans Ende. Ein Block, der
//   nur angewaehlt ist, weil er gerade eingefuegt wurde, nimmt NICHTS auf
//   (die Klebrigkeit aus rc532 bleibt draussen).
// * Compiler-Meldungen in der eingestellten Sprache, mit dem Befehl davor, und
//   die schuldige Zeile wird angewaehlt ("! =" -> Hinweis auf "!").
// * "Last compile: <Datum Uhrzeit>" in der Statuszeile.
std::vector<Schritt> wunschSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto befehl = [](const char* name) -> const Command* {
        for (const Command& c : g_app->db.commands) {
            if (c.name == name) { return &c; }
        }
        return nullptr;
    };
    const auto kinder = [](const Path& p) -> std::size_t {
        const Node* n = nodeAt(g_app->doc.script(), p);
        return n != nullptr ? n->children.size() : 0U;
    };
    add(tu("Wunsch: Skript mit einem affect-Block", [] {
        openScriptFromMemory("//Generated by BehavEd\n"
                             "affect ( \"blockA\", /*@AFFECT_TYPE*/ FLUSH )\n{\n\twait ( 100.000 );\n}\n"
                             "wait ( 200.000 );\n",
                             "wunsch.txt");
        g_app->expanded.openAll();   // hinein nur in einen AUFGEKLAPPTEN Block
        rebuildTree();
        g_app->selectedPath.clear();
        g_app->selection.clear();
        g_app->auswahlDurchEinfuegen.clear();
    }));
    add(pause(0.4));
    add(klick("affect", "##flowarea"));
    add(pause(0.2));
    add(pruefe("Wunsch: der Block ist per Klick angewaehlt", [] { return g_app->selectedPath == Path{0}; }));
    add(tu("Wunsch: print aus der Ereignisliste", [=] {
        if (const Command* c = befehl("print")) { fuegeBefehlEin(*c); }
    }));
    add(pause(0.2));
    add(pruefe("Wunsch: angeklickter, offener Block - der Befehl landet DARIN, am Anfang (wie das Original)", [=] {
        const Node* n = nodeAt(g_app->doc.script(), Path{0, 0});
        diag::detail("Knopftest: Wunsch - Auswahl " + std::to_string(g_app->selectedPath.size()) + " Ebenen, Kinder " +
                     std::to_string(kinder(Path{0})));
        return n != nullptr && n->name == "print" && g_app->selectedPath == (Path{0, 0});
    }));
    add(tu("Wunsch: noch ein wait", [=] {
        if (const Command* c = befehl("wait")) { fuegeBefehlEin(*c); }
    }));
    add(pause(0.2));
    add(pruefe("Wunsch: der naechste folgt dahinter, ebenfalls im Block", [=] {
        return kinder(Path{0}) == 3U && g_app->selectedPath == (Path{0, 1});
    }));
    // Die Klebrigkeit aus rc532 darf NICHT zurueckkommen.
    add(tu("Wunsch: das wait unten waehlen", [] { selectByPath(Path{1}); }));
    add(pause(0.2));
    add(tu("Wunsch: loop aus der Ereignisliste", [=] {
        if (const Command* c = befehl("loop")) { fuegeBefehlEin(*c); }
    }));
    add(tu("Wunsch: gleich danach ein wait", [=] {
        if (const Command* c = befehl("wait")) { fuegeBefehlEin(*c); }
    }));
    add(pause(0.2));
    add(pruefe("Wunsch: ein gerade eingefuegter Block nimmt den naechsten Befehl NICHT auf", [=] {
        std::string namen;
        for (const Node& n : g_app->doc.script().nodes) { namen += " " + (n.name.empty() ? std::string("-") : n.name); }
        diag::detail("Knopftest: Wunsch - oberste Ebene:" + namen);
        const Node* l = nodeAt(g_app->doc.script(), Path{2});
        const Node* w = nodeAt(g_app->doc.script(), Path{3});
        return l != nullptr && l->name == "loop" && l->children.empty() && w != nullptr && w->name == "wait";
    }));
    // Einfuegen aus der Ablage in einen angeklickten Block.
    add(tu("Wunsch: ein wait in die Ablage", [] {
        Node n;
        n.kind = Node::Kind::Command;
        n.name = "wait";
        Arg a;
        a.kind = Arg::Kind::Number;
        a.text = "300.000";
        n.args.push_back(a);
        g_app->ablage = std::vector<Node>{n};   // die Ablage des Programms (rc601)
        g_app->ablageText.clear();
        ImGui::SetClipboardText("");
    }));
    add(klick("affect", "##flowarea"));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_V, "Strg+V"));
    add(pause(0.3));
    add(pruefe("Wunsch: Einfuegen in den angeklickten Block - an den Anfang", [=] {
        const Node* n = nodeAt(g_app->doc.script(), Path{0, 0});
        diag::detail("Knopftest: Wunsch - affect hat " + std::to_string(kinder(Path{0})) + " Kinder");
        return kinder(Path{0}) == 4U && n != nullptr && n->name == "wait" && !n->args.empty() &&
               n->args[0].text == "300.000";
    }));

    // --- Compiler-Meldung: "! =" ---------------------------------------------
    add(tu("Wunsch: Skript mit if ( ..., ! =, ... )", [] {
        openScriptFromMemory("//Generated by BehavEd\n"
                             "if ( $get( FLOAT, \"SET_PARM1\")$, $! =$, $0$ )\n{\n\tprint ( \"a\" );\n}\n",
                             "wunsch2.txt");
        g_app->selectedPath.clear();
        g_app->selection.clear();
        g_app->statusLines.clear();
    }));
    add(pause(0.4));
    add(klick(tr(Str::Compile)));
    add(pause(0.4));
    add(pruefe("Wunsch: die Meldung nennt den Befehl und sagt, was gilt (Sprache der Oberflaeche)", [] {
        char soll[300];
        std::snprintf(soll, sizeof(soll), tr(Str::CmsgC004), "! =");
        // Die Statuszeilen tragen vorn ihren Zaehler "(n) : ".
        const std::string erwartet = std::string(") : if : ") + soll;
        bool da = false;
        for (const std::string& z : g_app->statusLines) {
            diag::detail("Knopftest: Wunsch - Status \"" + z + "\"");
            if (z.size() >= erwartet.size() &&
                z.compare(z.size() - erwartet.size(), erwartet.size(), erwartet) == 0) { da = true; }
        }
        return da;
    }));
    add(pruefe("Wunsch: nach dem Fehlschlag ist die schuldige Zeile angewaehlt", [] {
        return g_app->selectedPath == Path{0};
    }));

    // --- "Last compile" in der Statuszeile -----------------------------------
    auto datei = std::make_shared<std::string>(platform::executableDirectory() + "\\selbsttest_zeit.txt");
    add(tu("Wunsch: Skript mit Pfad (im Programmordner) laden", [=] {
        std::remove((platform::executableDirectory() + "\\selbsttest_zeit.ibi").c_str());
        std::ofstream f(*datei, std::ios::binary);
        f << "//Generated by BehavEd\r\n\r\nwait ( 100.000 );\r\n";
        f.close();
        ladeSkriptDatei(*datei);
    }));
    // 2,5 s: nach dem missglueckten Kompilieren oben zeigt der Knopf zwei
    // Sekunden lang "Compile failed" statt "Compile!".
    add(pause(2.5));
    add(pruefe("Wunsch: ohne .ibi steht keine Zeit da", [] { return g_app->letzteKompilierung.empty(); }));
    add(klick(tr(Str::Compile)));
    add(pause(1.5));
    add(pruefe("Wunsch: nach dem Kompilieren steht \"Last compile\" mit heutigem Datum unten", [] {
        const std::time_t jetzt = std::time(nullptr);
        std::tm lokal{};
        (void)localtime_s(&lokal, &jetzt);
        char heute[16];
        std::strftime(heute, sizeof(heute), "%Y-%m-%d", &lokal);
        diag::detail("Knopftest: Wunsch - Statuszeile \"" + g_app->letzteKompilierung + "\"");
        return g_app->letzteKompilierung.find(heute) != std::string::npos;
    }));
    add(tu("Wunsch: Dateien aufraeumen", [=] {
        std::remove(datei->c_str());
        std::remove((platform::executableDirectory() + "\\selbsttest_zeit.ibi").c_str());
    }));
    // Die Hervorhebung gleicher Befehle - fuers Auge (BHED_FOTOS): sie soll
    // erst am Namen beginnen, nicht unter dem Symbol, und blass sein.
    add(tu("Wunsch: wiederholte Kameras, Hervorhebung an", [] {
        openScriptFromMemory(std::string("//Generated by BehavEd\n") + kKuerzelSkript, "hervorhebung.txt");
        g_app->settings.highlightSame = true;
        selectByPath(Path{1});
    }));
    add(pause(0.5));
    add(foto("gleiche_hervorhebung"));
    add(tu("Wunsch: Hervorhebung wieder aus", [] { g_app->settings.highlightSame = false; }));
    return s;
}

// --- Ziehen im Script Flow: drei Zonen (27.09.) ------------------------------
//
// shank: "kannst du dieselbe Logik auch machen ... wenn man im Script Flow
// selbst rumschiebt? entweder darunter, darueber oder darauf". Echte
// Mauszuege auf das obere Drittel (davor), die Mitte (hinein bei einem Block,
// sonst dahinter) und das untere Drittel (dahinter; unter einem offenen Block
// mit Inhalt: an seinen Anfang).
std::string baumFlach() {
    std::string out;
    const std::function<void(const std::vector<Node>&)> geh = [&](const std::vector<Node>& ns) {
        for (const Node& n : ns) {
            if (n.kind == Node::Kind::Macro) {
                if (!out.empty()) { out += " "; }
                out += "$" + n.name + "@" + std::to_string(n.count);
                continue;
            }
            if (n.kind != Node::Kind::Command) { continue; }
            if (!out.empty()) { out += " "; }
            out += n.name;
            if (n.hasBlock) {
                out += "{";
                geh(n.children);
                out += " }";
            }
        }
    };
    geh(g_app->doc.script().nodes);
    return out;
}

std::vector<Schritt> zonenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Zonen: Skript laden, alles aufgeklappt", [] {
        openScriptFromMemory("//Generated by BehavEd\n"
                             "affect ( \"blockA\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                             "\tprint ( \"p\" );\n\tkill ( \"k\" );\n}\n"
                             "signal ( \"s\" );\nremove ( \"r\" );\n",
                             "zonen.txt");
        g_app->expanded.openAll();
        rebuildTree();
        g_app->selectedPath.clear();
        g_app->selection.clear();
    }));
    add(pause(0.4));
    const float d = ImGui::GetFontSize() * 0.42F;   // gut ins obere/untere Drittel
    const auto zug = [&](const std::string& von, const std::string& nach, float dy, const std::string& soll,
                         const std::string& was) {
        add(ziehe(von, "##flowarea", nach, "##flowarea", 0, 0, 0.5F, true, ImVec2{0.0F, dy}));
        add(pause(0.3));
        add(pruefe("Zonen: " + was, [=] {
            const std::string ist = baumFlach();
            diag::detail("Knopftest: Zonen - " + was + ": " + ist);
            return ist == soll;
        }));
    };
    zug("signal", "affect", -d, "signal affect{ print kill } remove", "oben auf dem Block = DAVOR");
    zug("signal", "affect", 0.0F, "affect{ signal print kill } remove", "Mitte des Blocks = HINEIN (an den Anfang)");
    zug("signal", "kill", d, "affect{ print kill signal } remove", "unten auf einer Zeile im Block = DAHINTER");
    zug("remove", "kill", 0.0F, "affect{ print kill remove signal }", "Mitte einer Zeile ohne Block = DAHINTER");
    zug("kill", "print", -d, "affect{ kill print remove signal }", "oben auf einer Zeile = DAVOR");
    zug("signal", "affect", d, "affect{ signal kill print remove }",
        "unten an einem OFFENEN Block mit Inhalt = an seinen Anfang (da steht die Linie)");
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Zonen: Strg+Z nimmt den letzten Zug zurueck", [] {
        return baumFlach() == "affect{ kill print remove signal }";
    }));

    // Makros (standOnly & Co.): im Original ein Behaelter wie ein Block -
    // "wenn man es auf das Plus zieht, wird es reingesetzt, und zwar ganz
    // oben" (shank, am Original getestet).
    add(tu("Zonen: Skript mit einem Makro laden", [] {
        openScriptFromMemory("//Generated by BehavEd\n"
                             "signal ( \"s\" );\n//$\"standOnly\"@2\nset ( \"a\", \"1\" );\nset ( \"b\", \"1\" );\n"
                             "remove ( \"r\" );\n",
                             "zonen_makro.txt");
        g_app->expanded.openAll();
        rebuildTree();
        g_app->selectedPath.clear();
        g_app->selection.clear();
    }));
    add(pause(0.4));
    zug("remove", "standOnly", 0.0F, "signal $standOnly@3 remove set set",
        "Mitte der Makrozeile = HINEIN, ganz oben (das Makro zaehlt mit)");
    zug("remove", "standOnly", -d, "signal remove $standOnly@2 set set", "oben auf der Makrozeile = DAVOR (wieder heraus)");
    zug("signal", "standOnly", d, "remove $standOnly@3 signal set set",
        "unten an einem OFFENEN Makro mit Inhalt = an seinen Anfang");
    zug("standOnly", "remove", -d, "$standOnly@3 signal set set remove",
        "die Makrozeile zieht ihre Befehle mit");
    // Hinzufuegen aus der Ereignisliste: offenes Makro angeklickt = hinein,
    // ganz oben; zugeklapptes = dahinter (am Original nachgesehen).
    const auto befehl = [](const char* name) -> const Command* {
        for (const Command& c : g_app->db.commands) {
            if (c.name == name) { return &c; }
        }
        return nullptr;
    };
    add(pause(0.4));
    add(tu("Zonen: die Makrozeile anwaehlen", [] {
        g_app->auswahlDurchEinfuegen.clear();
        selectByPath(Path{0});
    }));
    add(pause(0.2));
    add(tu("Zonen: print hinzufuegen (Makro offen)", [=] {
        g_app->doc.vergibKennungen();
        if (const Node* m = nodeAt(g_app->doc.script(), g_app->selectedPath)) { g_app->expanded.setOpen(m->kennung, true); }
        if (const Command* c = befehl("print")) { fuegeBefehlEin(*c); }
    }));
    add(pause(0.2));
    add(pruefe("Zonen: offenes Makro angeklickt - Hinzufuegen setzt hinein, ganz oben", [] {
        diag::detail("Knopftest: Zonen - Hinzufuegen offen: " + baumFlach());
        return baumFlach() == "$standOnly@4 print signal set set remove" && g_app->selectedPath == Path{1};
    }));
    add(klick("standOnly", "##flowarea"));
    add(pause(0.2));
    add(tu("Zonen: Makro zuklappen, kill hinzufuegen", [=] {
        g_app->doc.vergibKennungen();
        if (const Node* m = nodeAt(g_app->doc.script(), g_app->selectedPath)) { g_app->expanded.setOpen(m->kennung, false); }
        rebuildTree();
        if (const Command* c = befehl("kill")) { fuegeBefehlEin(*c); }
    }));
    add(pause(0.2));
    add(pruefe("Zonen: zugeklapptes Makro angeklickt - Hinzufuegen setzt DAHINTER", [] {
        diag::detail("Knopftest: Zonen - Hinzufuegen zu: " + baumFlach());
        return baumFlach() == "$standOnly@4 print signal set set kill remove";
    }));
    return s;
}

// --- "Revert to original" im Rechtsklickmenue (27.09.) --------------------------
std::vector<Schritt> revertSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto zeileVon = [](const Path& p) -> int {
        for (std::size_t i = 0; i < g_app->rows.size(); ++i) {
            if (g_app->rows[i].path == p) { return static_cast<int>(i); }
        }
        return -1;
    };
    add(tu("Revert: Skript laden", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 100.000 );\nwait ( 200.000 );\nprint ( \"p\" );\n",
                             "revert.txt");
        g_app->selectedPath.clear();
        g_app->selection.clear();
    }));
    add(pause(0.4));
    add(tu("Revert: das zweite wait aendern (wie im Editor)", [] {
        if (const Node* n = nodeAt(g_app->doc.script(), Path{1})) {
            Node neu = *n;
            neu.args[0].text = "999.000";
            (void)g_app->doc.replaceAt(Path{1}, neu);
            rebuildTree();
        }
    }));
    add(pause(0.3));
    add(pruefe("Revert: der geaenderte Befehl ist orange markiert", [=] {
        const int z = zeileVon(Path{1});
        return z >= 0 && static_cast<std::size_t>(z) < g_app->zeilenMarke.size() &&
               g_app->zeilenMarke[static_cast<std::size_t>(z)] == kMarkeGeaendert;
    }));
    add(pruefe("Revert: ein unveraenderter Befehl bietet nichts zum Zuruecksetzen", [] {
        return zuruecksetzbar({Path{2}}).empty() && zuruecksetzbar({Path{0}}).empty();
    }));
    // --- Die Vorschau: was war das Original? ----------------------------
    add(pruefe("Revert-Vorschau: der geaenderte Befehl zeigt sein Original (200)", [] {
        const std::string o = originalZeile(Path{1});
        diag::detail("Knopftest: Revert-Vorschau - \"" + o + "\"");
        return o.find("wait") != std::string::npos && o.find("200") != std::string::npos &&
               o.find("999") == std::string::npos;
    }));
    add(pruefe("Revert-Vorschau: ein unveraenderter Befehl zeigt nichts", [] {
        return originalZeile(Path{0}).empty() && originalZeile(Path{2}).empty();
    }));
    add(tu("Revert-Vorschau: Maus auf den Rand der geaenderten Zeile", [=] {
        g_app->randVorschauZeile = -1;
        const int z = zeileVon(Path{1});
        if (z >= 0 && static_cast<std::size_t>(z) < g_app->randMitte.size()) {
            setzeMaus(g_app->randMitte[static_cast<std::size_t>(z)]);
        }
    }));
    add(pause(0.8));
    add(foto("revert_vorschau_rand"));
    add(pruefe("Revert-Vorschau: Hovern ueber dem Rand zeigt den Hinweis mit dem Original", [=] {
        return g_app->randVorschauZeile == zeileVon(Path{1}) && g_app->vorschauGezeichnet > 0;
    }));
    add(tu("Revert-Vorschau: Maus weg, Zaehler zuruecksetzen", [] {
        setzeMaus(ImVec2{5.0F, 5.0F});
        g_app->vorschauGezeichnet = 0;
    }));
    add(pause(0.2));
    add(klick("wait", "##flowarea", 1, false, "", 1));   // Rechtsklick auf das zweite wait
    add(pause(0.3));
    add(foto("revert_vorschau_menue"));
    add(pruefe("Revert-Vorschau: das Rechtsklickmenue zeigt das Original unter \"Revert to original\"", [] {
        return g_app->vorschauGezeichnet > 0;
    }));
    add(klick(tr(Str::ActRevertOriginal), "##Popup"));
    add(pause(0.3));
    add(pruefe("Revert: der Befehl steht wieder wie beim Oeffnen, ohne Marke", [=] {
        const Node* n = nodeAt(g_app->doc.script(), Path{1});
        const int z = zeileVon(Path{1});
        const bool ohneMarke = z >= 0 && static_cast<std::size_t>(z) < g_app->zeilenMarke.size() &&
                               g_app->zeilenMarke[static_cast<std::size_t>(z)] == kMarkeKeine;
        diag::detail("Knopftest: Revert - jetzt " + (n != nullptr && !n->args.empty() ? n->args[0].text : "?") +
                     ", Marke " + (z >= 0 ? std::to_string(g_app->zeilenMarke[static_cast<std::size_t>(z)]) : "?"));
        return n != nullptr && !n->args.empty() && n->args[0].text == "200.000" && ohneMarke;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Revert: Strg+Z holt die Aenderung zurueck (ein Schritt)", [] {
        const Node* n = nodeAt(g_app->doc.script(), Path{1});
        return n != nullptr && !n->args.empty() && n->args[0].text == "999.000";
    }));

    // Blau: geaendert, GESPEICHERT (gruen), dann zurueck auf den Stand beim
    // Oeffnen - so wie in Notepad++. Ohne das Speichern dazwischen gibt es
    // kein Blau: dann ist die Zeile einfach wieder wie die Datei.
    auto datei = std::make_shared<std::string>(platform::executableDirectory() + "\\selbsttest_revert.txt");
    add(tu("Revert: Skript mit Pfad laden", [=] {
        std::ofstream f(*datei, std::ios::binary);
        f << "//Generated by BehavEd\r\n\r\nwait ( 100.000 );\r\nwait ( 200.000 );\r\n";
        f.close();
        ladeSkriptDatei(*datei);
    }));
    add(pause(0.5));
    // Die Datei beginnt mit einer Leerzeile - also den Weg des ZWEITEN wait
    // suchen, statt ihn anzunehmen.
    auto zweites = std::make_shared<Path>();
    add(tu("Revert: das zweite wait aendern und speichern", [=] {
        int n = 0;
        for (std::size_t i = 0; i < g_app->doc.script().nodes.size(); ++i) {
            if (g_app->doc.script().nodes[i].name == "wait" && ++n == 2) { *zweites = Path{i}; }
        }
        if (const Node* k = nodeAt(g_app->doc.script(), *zweites)) {
            Node neu = *k;
            neu.args[0].text = "999.000";
            (void)g_app->doc.replaceAt(*zweites, neu);
            rebuildTree();
        }
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
    add(pause(0.4));
    add(pruefe("Revert: nach dem Speichern ist die Zeile gruen", [=] {
        const int z = zeileVon(*zweites);
        return z >= 0 && g_app->zeilenMarke[static_cast<std::size_t>(z)] == kMarkeGespeichert;
    }));
    add(klick("wait", "##flowarea", 1, false, "", 1));
    add(pause(0.3));
    add(klick(tr(Str::ActRevertOriginal), "##Popup"));
    add(pause(0.3));
    add(pruefe("Revert: zurueckgesetzt nach dem Speichern = BLAU (wieder wie beim Oeffnen)", [=] {
        const int z = zeileVon(*zweites);
        diag::detail("Knopftest: Revert - Marke nach Speichern und Zuruecksetzen " +
                     (z >= 0 ? std::to_string(g_app->zeilenMarke[static_cast<std::size_t>(z)]) : "?"));
        return z >= 0 && g_app->zeilenMarke[static_cast<std::size_t>(z)] == kMarkeZurueck;
    }));
    add(tu("Revert: aufraeumen", [=] {
        g_app->doc.markSaved();   // keine Speichernfrage beim Beenden
        std::remove(datei->c_str());
    }));
    return s;
}

}  // namespace
// rc601 release: definiert in gui/app.cpp; einen Kopf dafuer gibt es nicht
// (main_win32.cpp erklaert es genauso selbst).
bool loadModel(App* a, const std::string& dataDir, std::string* error);
namespace {

// --- rc601 release: die .exe ALLEIN (BHED_EDITORTEST=eigenstaendig) ---------
//
// shank: "bei jedem Release sollte es auch eine .exe geben, die gebaut wird,
// dass die Leute sie runterladen und nutzen koennen". Nachgestellt wie beim
// Anwender: eine Kopie DIESER exe, benannt wie die im Release
// (BehavEd-Remake-...-windows-x64.exe), in einem Ordner OHNE data\base, mit
// eigenem Einstellungsordner. Gestartet mit --pruefe-daten (kein Fenster,
// kein Ton) muss sie mit 0 enden und die eingebauten Daten in IHREN
// Einstellungsordner gelegt haben - nichts neben sich. Bis rc600 kam
// stattdessen das Meldungsfenster "behaved.bhc not found in ...": der Start
// endete nicht, bis dieser Schritt ihn beendete.
//
// Dazu die Meldung selbst: fehlen die Daten wirklich, sagt sie in der
// eingestellten Sprache, was zu tun ist (bis rc600 nur englisch und
// technisch).
std::vector<Schritt> eigenstaendigSchritte() {
    namespace fs = std::filesystem;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto u8 = [](const fs::path& p) {
        const std::u8string t = p.u8string();
        return std::string(t.begin(), t.end());
    };
    // Der Programmordner als Pfad (UTF-8 -> path ohne das veraltete u8path).
    const auto programmOrdner = [] {
        const std::string o = platform::executableDirectory();
        return fs::path(std::u8string(reinterpret_cast<const char8_t*>(o.data()), o.size()));
    };
    // Stufe 8c: die laufende Programmdatei (jkstudio.exe; aeltere Laufordner: behaved.exe)
    const auto programmDatei = [programmOrdner] {
        std::error_code ec;
        const fs::path neu = programmOrdner() / "jkstudio.exe";
        return fs::exists(neu, ec) ? neu : programmOrdner() / "behaved.exe";
    };
    auto basis = std::make_shared<fs::path>();
    auto kopie = std::make_shared<fs::path>();
    auto griff = std::make_shared<std::uintptr_t>(0);
    auto start = std::make_shared<std::chrono::steady_clock::time_point>();
    auto code = std::make_shared<long>(-3);
    add(tu("Eigenstaendig: Kopie der exe in einen Ordner ohne data\\base legen", [=] {
        const fs::path hier = programmOrdner();
        *basis = hier / "eigenstaendig";
        std::error_code ec;
        fs::remove_all(*basis, ec);
        fs::create_directories(*basis / "prog", ec);
        fs::create_directories(*basis / "einst", ec);
        *kopie = *basis / "prog" / "JK-Studio-selbsttest-windows-x64.exe";
        fs::copy_file(programmDatei(), *kopie, fs::copy_options::overwrite_existing, ec);
        diag::detail("Knopftest: Eigenstaendig - Kopie " + u8(*kopie) + (ec ? " FEHLT: " + ec.message() : ""));
    }));
    add(pruefe("Eigenstaendig: die Kopie liegt allein im Ordner (kein data daneben)", [=] {
        std::error_code ec;
        return fs::exists(*kopie, ec) && !fs::exists(*basis / "prog" / "data", ec);
    }));
    add(tu("Eigenstaendig: die Kopie mit --pruefe-daten starten", [=] {
        // BHED_KEIN_MELDUNGSFENSTER (Durchsicht): erkennt die Kopie
        // --pruefe-daten einmal nicht mehr, endet sie leise mit 2, statt ein
        // Meldungsfenster mit Klang auf den Schirm zu stellen.
        *griff = platform::starteProgramm(u8(*kopie), "--pruefe-daten",
                                          {"BHED_EINSTELLUNGEN=" + u8(*basis / "einst"),
                                           "BHED_KEIN_MELDUNGSFENSTER=1", "BHED_EINZEL_AUS=1"});
        *start = std::chrono::steady_clock::now();
        *code = -3;
        diag::detail(std::string("Knopftest: Eigenstaendig - gestartet, Griff ") + (*griff != 0 ? "da" : "FEHLT"));
    }));
    // Hoechstens 8 s: laenger darf kein Schritt dauern (nach 4000 Bildern
    // gilt er als haengend), und mehr als eine Sekunde braucht das Auspacken
    // von 156 KB nie.
    add(warteBis("Eigenstaendig: die Kopie endet von selbst", [=] {
        const long st = platform::zweiteInstanzStand(*griff);
        const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - *start).count();
        if (st != -1 || sek > 8.0) {
            *code = st;
            platform::zweiteInstanzSchliessen(*griff, true);   // laeuft sie noch: nur DIESEN Kindprozess
            *griff = 0;
            diag::detail("Knopftest: Eigenstaendig - nach " + std::to_string(sek) + " s: " +
                         (st == -1 ? std::string("lief noch (Meldungsfenster?), beendet") : "Code " + std::to_string(st)));
            return true;
        }
        return false;
    }, 100000));
    add(pruefe("Eigenstaendig: --pruefe-daten endet mit 0 (Befehlsmodell geladen, kein Meldungsfenster)",
               [=] { return *code == 0; }));
    add(pruefe("Eigenstaendig: die eingebauten Daten liegen im Einstellungsordner der Kopie, Byte fuer Byte wie data\\base", [=] {
        const fs::path quelle = programmOrdner() / "data" / "base";
        const fs::path ziel = *basis / "einst" / "data" / "base";
        std::error_code ec;
        int gleich = 0;
        int anders = 0;
        for (const auto& e : fs::directory_iterator(quelle, ec)) {
            if (!e.is_regular_file()) { continue; }
            const auto lies = [](const fs::path& p) {
                std::ifstream f(p, std::ios::binary);
                return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            };
            const fs::path p = ziel / e.path().filename();
            if (fs::exists(p, ec) && lies(p) == lies(e.path())) {
                ++gleich;
            } else {
                ++anders;
                diag::detail("Knopftest: Eigenstaendig - fehlt oder anders: " + u8(p));
            }
        }
        diag::detail("Knopftest: Eigenstaendig - " + std::to_string(gleich) + " Dateien gleich, " +
                     std::to_string(anders) + " fehlen oder anders");
        return gleich > 0 && anders == 0;
    }));
    add(pruefe("Eigenstaendig: nichts neben die Kopie geschrieben (kein data-Ordner)", [=] {
        std::error_code ec;
        return !fs::exists(*basis / "prog" / "data", ec);
    }));
    add(pruefe("Eigenstaendig: fehlen die Daten, sagt die Meldung auf Deutsch, was zu tun ist", [=] {
        // loadModel wie beim Start, aber mit einem Ordner ohne Daten. Es
        // laesst das geladene Modell in Ruhe (loadCommandDb scheitert vor dem
        // ersten Eintrag); Ordner, eigene .bhc und Sprache kommen zurueck.
        const i18n::Language spracheVorher = i18n::currentLanguage();
        const std::string ordnerVorher = g_app->dataDir;
        const std::string bhcVorher = g_app->settings.commandFile;
        g_app->settings.commandFile.clear();
        i18n::setLanguage(i18n::Language::German);
        const std::string fehlt = u8(*basis / "fehlt" / "data" / "base");
        std::string meldung;
        const bool geladen = loadModel(g_app, fehlt, &meldung);
        char soll[2048];
        std::snprintf(soll, sizeof(soll), i18n::trIn(i18n::Language::German, Str::DataMissing), fehlt.c_str());
        i18n::setLanguage(spracheVorher);
        g_app->dataDir = ordnerVorher;
        g_app->settings.commandFile = bhcVorher;
        diag::detail("Knopftest: Eigenstaendig - Meldung: " + meldung);
        return !geladen && meldung == soll;
    }));
    // --- rc601 release (Durchsicht): Auspacken scheitert --------------------
    //
    // Durchsicht: liess sich das Eingebaute nicht auspacken (Platte voll,
    // Einstellungsordner schreibgeschuetzt oder umgeleitet), zeigte das
    // Meldungsfenster nur DataMissing ("das ganze Zip entpacken ...") mit dem
    // Ordner neben der .exe - irrefuehrend fuer die eigenstaendige .exe, und
    // der Grund stand nur im Protokoll. Nachgestellt mit einer zweiten Kopie,
    // in deren Einstellungsordner eine DATEI "data" liegt: data\base laesst
    // sich dort nicht anlegen. Normal gestartet (ohne --pruefe-daten, also
    // der Weg bis zum Meldungsfenster), mit BHED_KEIN_MELDUNGSFENSTER: die
    // Kopie schreibt die Meldung nur in ihr Protokoll und endet sofort mit 2.
    auto basis2 = std::make_shared<fs::path>();
    auto kopie2 = std::make_shared<fs::path>();
    auto griff2 = std::make_shared<std::uintptr_t>(0);
    auto start2 = std::make_shared<std::chrono::steady_clock::time_point>();
    auto code2 = std::make_shared<long>(-3);
    add(tu("Eigenstaendig: zweite Kopie, deren Einstellungsordner kein data\\base zulaesst, normal starten", [=] {
        *basis2 = *basis / "gesperrt";
        std::error_code ec;
        fs::create_directories(*basis2 / "prog", ec);
        fs::create_directories(*basis2 / "einst", ec);
        *kopie2 = *basis2 / "prog" / "JK-Studio-selbsttest-windows-x64.exe";
        fs::copy_file(programmDatei(), *kopie2, fs::copy_options::overwrite_existing, ec);
        {
            std::ofstream sperre(*basis2 / "einst" / "data", std::ios::binary);
            sperre << "eine Datei, kein Ordner\n";
        }
        {
            std::ofstream cfg(*basis2 / "einst" / "behaved.cfg", std::ios::binary);
            cfg << "# behaved\nlanguage=de\n";
        }
        *griff2 = platform::starteProgramm(u8(*kopie2), "",
                                           {"BHED_EINSTELLUNGEN=" + u8(*basis2 / "einst"),
                                            "BHED_KEIN_MELDUNGSFENSTER=1", "BHED_EINZEL_AUS=1"});
        *start2 = std::chrono::steady_clock::now();
        *code2 = -3;
        diag::detail(std::string("Knopftest: Eigenstaendig gesperrt - gestartet, Griff ") +
                     (*griff2 != 0 ? "da" : "FEHLT") + (ec ? ", Kopie FEHLT: " + ec.message() : ""));
    }));
    add(warteBis("Eigenstaendig: die gesperrte Kopie endet von selbst (kein Meldungsfenster)", [=] {
        const long st = platform::zweiteInstanzStand(*griff2);
        const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - *start2).count();
        if (st != -1 || sek > 8.0) {
            *code2 = st;
            platform::zweiteInstanzSchliessen(*griff2, true);   // laeuft sie noch: nur DIESEN Kindprozess
            *griff2 = 0;
            diag::detail("Knopftest: Eigenstaendig gesperrt - nach " + std::to_string(sek) + " s: " +
                         (st == -1 ? std::string("lief noch (Meldungsfenster?), beendet") : "Code " + std::to_string(st)));
            return true;
        }
        return false;
    }, 100000));
    add(pruefe("Eigenstaendig: ohne auspackbare Daten endet der Start mit 2, ohne Meldungsfenster",
               [=] { return *code2 == 2; }));
    add(pruefe("Eigenstaendig: die Meldung nennt den Ordner, in den ausgepackt werden sollte, und den Grund (Deutsch)", [=] {
        // Die Kopie protokolliert neben sich (jkstudio.log), Zeilenenden egal.
        std::string log;
        {
            std::ifstream f(*basis2 / "prog" / "jkstudio.log", std::ios::binary);
            log.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        }
        const auto vereinheitlicht = [](std::string t) {
            t.erase(std::remove(t.begin(), t.end(), '\r'), t.end());
            std::replace(t.begin(), t.end(), '/', '\\');
            return t;
        };
        log = vereinheitlicht(log);
        char teil1[2048];
        std::snprintf(teil1, sizeof(teil1), i18n::trIn(i18n::Language::German, Str::DataMissing),
                      u8(*basis2 / "prog" / "data" / "base").c_str());
        // Der Grund kommt vom System (je nach Windows-Sprache) - verlangt ist
        // nur, dass einer dasteht: alles bis zu seinem Platzhalter muss passen.
        char teil2[2048];
        std::snprintf(teil2, sizeof(teil2), i18n::trIn(i18n::Language::German, Str::DataUnpackFailed),
                      u8(*basis2 / "einst" / "data" / "base").c_str(), "\x01");
        std::string vorGrund = teil2;
        vorGrund.resize(vorGrund.find('\x01'));
        const std::string soll = vereinheitlicht(std::string(teil1) + "\n\n" + vorGrund);
        const std::size_t pos = log.find(soll);
        const std::size_t grund = pos == std::string::npos ? pos : pos + soll.size();
        const bool mitGrund = grund != std::string::npos && grund < log.size() && log[grund] != '\n';
        const std::size_t von = log.find("Startmeldung:");
        diag::detail("Knopftest: Eigenstaendig gesperrt - Meldung im Protokoll der Kopie: " +
                     (von == std::string::npos ? std::string("KEINE") : log.substr(von, 900)));
        return pos != std::string::npos && mitGrund;
    }));
    add(tu("Eigenstaendig: aufraeumen", [=] {
        std::error_code ec;
        fs::remove_all(*basis, ec);
    }));
    return s;
}

#include "selbsttest_programm.inc"   // Stufe 8d: Programm - Updater, Reiter, Speichern, Code-Pruefung, Missionen, Jedi Outcast
#include "selbsttest_vorschau_a.inc"   // Stufe 8d: Vorschau I - Sicherung, Teiler, Kamera, Kartenpruefung, Fotos
#include "selbsttest_vorschau_b.inc"   // Stufe 8d: Vorschau II - Bewegung, OpenJK/Movie Duels, Gras, Funde rc604/rc605, Tueren, Spulen
#include "selbsttest_werkzeuge.inc"   // Stufe 8d: Werkzeuge - Wegnetz, Drehen, Kopieren, Model-Tab, Indizes
#include "selbsttest_karte1bis2.inc"   // Stufe 8d: Karten-Editor Stufe 1b und 2
#include "selbsttest_karte3bis5.inc"   // Stufe 8d: Karten-Editor Stufe 3 bis 5 (mit karte6/karte8)
#include "selbsttest_handbuch.inc"   // Stufe 8d: grosse Karte, Stufe 8g und die Bildlaeufe des Handbuchs
// --- Teil 1: Ruhe beim Oeffnen --------------------------------------------

struct RuheFall {
    const char* name;      // nur fuers Protokoll
    const char* befehl;    // Befehlsname im Testskript
    int nte;               // der wievielte Befehl dieses Namens (ab 0)
    bool ausdruckAn;       // nach dem Einschwingen Expr! druecken
    bool ziehen;           // mit simulierter Maus an der Titelzeile ziehen
};

constexpr RuheFall kRuheFaelle[] = {
    {"if/3", "if", 0, false, false},
    {"if/1", "if", 1, false, false},
    {"set/2", "set", 0, false, false},
    {"if/1+Expr", "if", 1, true, false},
    {"if/3+Zug", "if", 0, false, true},
    {"camera/PAN", "camera", 0, false, false},
    {"camera/ENA", "camera", 1, false, false},
    {"sound", "sound", 0, false, false},
    {"affect", "affect", 0, false, false},
    {"else", "else", 0, false, false},
    {"print", "print", 0, false, false},
    {"wait", "wait", 0, false, false},
    {"if/3+Expr", "if", 0, true, false},
};
constexpr int kRuheFallZahl =
    static_cast<int>(sizeof(kRuheFaelle) / sizeof(kRuheFaelle[0]));
constexpr int kRuheRunden = 2;     // 0: erstes Oeffnen, 1: zweites
constexpr int kBilderOffen = 60;
constexpr int kBilderPause = 6;
constexpr int kVorlauf = 30;       // Fenster, Schriften, Einstellungen

// Beide Fassungen von `if`, dazu Befehle ohne Helfer zum Vergleich.
const char* const kSkript =
    "//Generated by BehavEd\n"
    "\n"
    "rem ( \"Selbsttest\" );\n"
    "if ( $get( FLOAT, \"rand\")$, $>$, $1$ )\n"
    "{\n"
    "\twait ( 100.000 );\n"
    "}\n"
    "\n"
    "if ( $random( 0, 1 ) > 0.500000$ )\n"
    "{\n"
    "\twait ( 100.000 );\n"
    "}\n"
    "\n"
    // Mit Markierung, wie BehavEd sie schreibt: ohne /*@SET_TYPES*/
    // oeffnet das Original set als Variablen-set (zwei Textfelder).
    "set ( /*@SET_TYPES*/ \"SET_PARM1\", \"abc\" );\n"
    "wait ( 1000.000 );\n"
    "camera ( /*@CAMERA_COMMANDS*/ PAN, < -10.000 107.000 0.000 >, "
    "< 0.000 0.000 0.000 >, 0 );\n"
    "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
    "sound ( CHAN_VOICE_GLOBAL, \"sound/md_twj/s3_kick.mp3\" );\n"
    "affect ( \"anakin2\", /*@AFFECT_TYPE*/ FLUSH )\n"
    "{\n"
    "\twait ( 100.000 );\n"
    "}\n"
    "\n"
    "if ( $get( FLOAT, \"rand\")$, $>$, $2$ )\n"
    "{\n"
    "\twait ( 100.000 );\n"
    "}\n"
    "\n"
    "else (  )\n"
    "{\n"
    "\twait ( 100.000 );\n"
    "}\n"
    "\n"
    "print ( \"Ahsoka trusted you\" );\n";

struct Ruhe {
    int fall = 0;
    int runde = 0;
    int imFall = 0;        // Bilder im laufenden Fall
    bool vorSichtbar = false;
    ImVec2 vorPos{};
    ImVec2 vorGroesse{};
    int sichtbar = 0;
    int ruck = 0;
    int ruckGesamt = 0;
    ImVec2 griff{};        // Ziehen: wo die Maus die Titelzeile packt
    ImVec2 vorZiehen{};    // Ziehen: Fensterstelle davor
    int nachLoslassen = 0; // Ziehen: Aenderungen nach dem Loslassen
    std::string bericht;
};

// Ein Bild des Ruhetests. Liefert true, wenn alle Faelle durch sind.
bool ruheBild(Ruhe& t) {
    if (t.runde >= kRuheRunden) { return true; }
    const RuheFall& f = kRuheFaelle[t.fall];
    if (t.imFall == 0) {
        Path weg;
        if (!wegZu(f.befehl, f.nte, weg)) {
            diag::detail(std::string("Selbsttest: Befehl fehlt fuer ") + f.name);
            t.runde = kRuheRunden;
            return true;
        }
        openEditorForNode(weg);
        t.vorSichtbar = false;
        t.sichtbar = 0;
        t.ruck = 0;
    }
    ++t.imFall;

    if (t.imFall <= kBilderOffen) {
        if (f.ausdruckAn && t.imFall == 30 && !g_app->editorIsExpr.empty()) {
            // Expr! auf Feld 0 umschalten - wie ein Klick. Das Wachsen
            // danach ist gewollt; gezaehlt wird, wie oft es SPRINGT.
            g_app->editorIsExpr[0] = g_app->editorIsExpr[0] != 0 ? 0 : 1;
            diag::detail("Selbsttest: Expr! umgeschaltet");
        }
        ImGuiWindow* w = ImGui::FindWindowByName("###editor");
        const bool zu = (w == nullptr || !w->Active || w->Hidden);
        // Ziehen wie mit der Maus: Titelzeile packen, zehn Bilder lang je
        // fuenf Punkte nach rechts, loslassen. Das Fenster muss um fuenfzig
        // Punkte wandern und danach dort bleiben.
        if (f.ziehen && !zu) {
            ImGuiIO& io = ImGui::GetIO();
            if (t.imFall == 20) {
                t.griff = ImVec2{w->Pos.x + w->Size.x * 0.5F,
                                 w->Pos.y + w->TitleBarHeight * 0.5F};
                t.vorZiehen = w->Pos;
                setzeMaus(t.griff);
            } else if (t.imFall == 21) {
                io.AddMouseButtonEvent(0, true);
            } else if (t.imFall >= 22 && t.imFall <= 31) {
                setzeMaus(ImVec2{t.griff.x + 5.0F * static_cast<float>(t.imFall - 21),
                                 t.griff.y});
            } else if (t.imFall == 32) {
                io.AddMouseButtonEvent(0, false);
            }
        }
        if (!zu) {
            const bool anders =
                t.vorSichtbar &&
                (std::fabs(w->Pos.x - t.vorPos.x) >= 0.5F ||
                 std::fabs(w->Pos.y - t.vorPos.y) >= 0.5F ||
                 std::fabs(w->Size.x - t.vorGroesse.x) >= 0.5F ||
                 std::fabs(w->Size.y - t.vorGroesse.y) >= 0.5F);
            if (!t.vorSichtbar || anders) {
                char z[260];
                std::snprintf(z, sizeof(z),
                              "Selbsttest %s R%d Bild %d: %.0f/%.0f %.0fx%.0f "
                              "expr0 %d zelle %.0f bestellt %.0f reihe %.0f",
                              f.name, t.runde, t.imFall,
                              static_cast<double>(w->Pos.x),
                              static_cast<double>(w->Pos.y),
                              static_cast<double>(w->Size.x),
                              static_cast<double>(w->Size.y),
                              istAusdruck(0) ? 1 : 0,
                              static_cast<double>(g_app->dbgZelle0),
                              static_cast<double>(g_app->dbgBestellt0),
                              static_cast<double>(g_app->editorRowW));
                diag::detail(z);
            }
            if (anders) { ++t.ruck; }
            if (anders && f.ziehen && t.imFall > 35) { ++t.nachLoslassen; }
            ++t.sichtbar;
            t.vorSichtbar = true;
            t.vorPos = w->Pos;
            t.vorGroesse = w->Size;
        }
        return false;
    }
    if (t.imFall == kBilderOffen + 1) {
        char z[200];
        if (f.ziehen) {
            ImGuiWindow* w = ImGui::FindWindowByName("###editor");
            const ImVec2 weg = (w != nullptr)
                                   ? ImVec2{w->Pos.x - t.vorZiehen.x,
                                            w->Pos.y - t.vorZiehen.y}
                                   : ImVec2{0.0F, 0.0F};
            std::snprintf(z, sizeof(z),
                          "Selbsttest %-10s R%d: gezogen um %.0f/%.0f "
                          "(erwartet 50/0), %d Aenderungen nach dem Loslassen\n",
                          f.name, t.runde, static_cast<double>(weg.x),
                          static_cast<double>(weg.y), t.nachLoslassen);
            // Das Ziehen selbst ist gewollt - gezaehlt wird nur, was danach
            // noch wackelt.
            t.ruck = t.nachLoslassen;
            t.nachLoslassen = 0;
        } else {
            std::snprintf(z, sizeof(z),
                          "Selbsttest %-10s R%d: %2d Rucke in %d sichtbaren Bildern\n",
                          f.name, t.runde, t.ruck, t.sichtbar);
        }
        t.bericht += z;
        // Ein gewolltes Wachsen nach Expr! ist ein Sprung - er zaehlt mit,
        // soll aber einer bleiben.
        t.ruckGesamt += t.ruck;
        g_app->editorOpen = false;
        return false;
    }
    if (t.imFall > kBilderOffen + kBilderPause) {
        t.imFall = 0;
        if (++t.fall >= kRuheFallZahl) {
            t.fall = 0;
            ++t.runde;
        }
    }
    return false;
}

// --- Ablauf ---------------------------------------------------------------

struct Ablauf {
    int bild = 0;
    int teil = 0;          // 0 Vorlauf, 1 Ruhe, 2 Knoepfe, 3 fertig
    Ruhe ruhe;
    std::vector<Schritt> schritte;
    std::size_t schritt = 0;
    int imSchritt = 0;
    int fehlerVorher = 0;  // Protokollfehler vor dem Test (Zusicherungen)
};

Ablauf g_ablauf;

}  // namespace

void selbsttestVorBild() {
    if (karteneditor::zeigerSim().an) {   // Stufe 7 Punkt 6c: der simulierte Zeiger (wie Windows)
        karteneditor::zeigerSimVorBild();
        g_maus = ImVec2{karteneditor::zeigerSim().x, karteneditor::zeigerSim().y};
        return;
    }
    if (g_mausAn) {
        ImGui::GetIO().AddMousePosEvent(g_maus.x, g_maus.y);
    }
}

void selbsttestVorBackend() { karteneditor::zeigerSimVorBackend(); }

void selbsttest() {
    static const char* const modus = std::getenv("BHED_EDITORTEST");
    if (modus == nullptr || g_app == nullptr) { return; }
    // BHED_EDITORTEST=geteilt: nur die geteilte Ansicht (schnell, zum
    // Wiederholen). Jeder andere Wert: alles.
    static const bool nurGeteilt = std::strcmp(modus, "geteilt") == 0;
    static const bool nurOriginal = std::strcmp(modus, "original") == 0;
    static const bool nurKarte = std::strcmp(modus, "karte") == 0;
    static const bool nurVergleich = std::strcmp(modus, "vergleich") == 0;
    static const bool nurZiehen = std::strcmp(modus, "ziehen") == 0;
    static const bool nurLesezeichen = std::strcmp(modus, "lesezeichen") == 0;
    static const bool nurIcons = std::strcmp(modus, "icons") == 0;
    static const bool nurLava = std::strcmp(modus, "lava") == 0;
    static const bool nurHolo = std::strcmp(modus, "holo") == 0;
    static const bool nurOrt = std::strcmp(modus, "ort") == 0;
    static const bool nurNpcKlasse = std::strcmp(modus, "npcklasse") == 0;   // rc602
    static const bool nurTuerLauf = std::strcmp(modus, "tuerlauf") == 0;   // rc603 fix603
    static const bool nurSpulen = std::strcmp(modus, "spulen") == 0;   // rc603 fix603
    static const bool nurGluehen = std::strcmp(modus, "gluehen") == 0;   // rc602
    static const bool nurGras = std::strcmp(modus, "gras") == 0;   // rc604 surfaceSprites
    static const bool nurFigurLeistung = std::strcmp(modus, "figurleistung") == 0;   // rc603
    static const bool nurFigurStufen = std::strcmp(modus, "figurstufen") == 0;   // rc603
    static const bool nurKamera = std::strcmp(modus, "kamera") == 0;
    // Nur der Umriss der Auswahl (rc597).
    static const bool nurUmriss = std::strcmp(modus, "umriss") == 0;
    static const bool nurWeg = std::strcmp(modus, "wegpunkte") == 0;
    static const bool nurModell = std::strcmp(modus, "modell") == 0;
    // Stufe 1b: der Karten-Editor (NetRadiant-Custom).
    static const bool nurKarteneditor = std::strcmp(modus, "karteneditor") == 0;
    static const bool nurAlleKarten = std::strcmp(modus, "allekarten") == 0;
    static const bool nurKartenpruefung = std::strcmp(modus, "kartenpruefung") == 0;   // rc602
    // rc602: die Funde der Kartenpruefung (Gegenstaende, Effektgluehen) - "kartenfunde".
    static const bool nurGegenstaende = std::strcmp(modus, "gegenstaende") == 0 || std::strcmp(modus, "kartenfunde") == 0;
    static const bool nurFix604b = std::strcmp(modus, "fix604b") == 0;   // rc604 fix604b: die neuen Kartenproben
    static const bool nurLaserfalle = std::strcmp(modus, "laserfalle") == 0;   // rc604: nur die Laserfallen
    static const bool nurModellImFesten = std::strcmp(modus, "modellimfesten") == 0;   // Stufe 6: OpenJK#1358
    static const bool nurGeschuetzAnim = std::strcmp(modus, "geschuetzanim") == 0;   // rc604: Wachgeschuetz, Ionenkanone
    static const bool nurZielstrahl = std::strcmp(modus, "zielstrahl") == 0;   // rc604: fx_target_beam
    static const bool nurEffektGrund = std::strcmp(modus, "effektgrund") == 0;   // rc604: Grundstufe der Effektshader
    static const bool nurFigurDeform = std::strcmp(modus, "figurdeform") == 0;   // rc604: deformVertexes an Figuren
    static const bool nurRest605 = std::strcmp(modus, "rest605") == 0;   // rc605: die Restpunkte (BHED_REST)
    static const bool nurUmzug = std::strcmp(modus, "umzug") == 0;   // Stufe 8c: Einstellungen aus %APPDATA%\behaved uebernommen
    static const bool nurAblauf = std::strcmp(modus, "ablauf") == 0;
    static const bool nurZeitleiste = std::strcmp(modus, "zeitleiste") == 0;
    static const bool nurFenster = std::strcmp(modus, "fenster") == 0;
    static const bool nurUpdate = std::strcmp(modus, "update") == 0;
    // rc601 release: die .exe allein, ohne data\base daneben (eigene Kopie).
    static const bool nurEigenstaendig = std::strcmp(modus, "eigenstaendig") == 0;
    static const bool nurKuerzel = std::strcmp(modus, "kuerzel") == 0;
    static const bool nurSzenen = std::strcmp(modus, "szenen") == 0;
    // Nur die Reiterpruefungen (Schliessen, doppelt Oeffnen, Ungesichertes) -
    // schnell nachzustellen, ohne den ganzen Lauf.
    static const bool nurReiter = std::strcmp(modus, "reiter") == 0;
    // Kopieren und Einfuegen im Skriptbaum (rc599).
    static const bool nurAblage = std::strcmp(modus, "ablage") == 0;
    // ROFF-Bahnen bauen und aendern (rc599).
    static const bool nurRoff = std::strcmp(modus, "roff") == 0;
    // Bilder fuer das Handbuch (rc599).
    static const bool nurHandbuch = std::strcmp(modus, "handbuch") == 0;
    // Ein zweiter Start reicht an diesen weiter (braucht BHED_EINZEL).
    static const bool nurEinzel = std::strcmp(modus, "einzel") == 0;
    // Mission laden mit leerem Reiter vorn (braucht die Spieldaten).
    static const bool nurMission = std::strcmp(modus, "mission") == 0;
    // Die Funde der Code-Pruefung vom 03.10., jeder im Programm nachgestellt.
    static const bool nurPruefung = std::strcmp(modus, "pruefung") == 0;
    // Eine eigene Mission aus einer pk3 laden und fotografieren (BHED_MISSION_ARCHIV, ..._NAME, BHED_ZEIT).
    static const bool nurMissionFoto = std::strcmp(modus, "missionfoto") == 0;
    // Zuletzt geoeffnete Missionen (rc599).
    static const bool nurLetzte = std::strcmp(modus, "letzte") == 0;
    // rc603: Load mission nennt die Missionen des ganzen Spiels.
    static const bool nurMissionsliste = std::strcmp(modus, "missionsliste") == 0;
    // rc604: Jedi Outcast als drittes Regelwerk (Original-JK2-Daten).
    static const bool nurJk2 = std::strcmp(modus, "jk2") == 0;
    // Missionsbau im Map-Tab: jeder Knopf, Kombinationen, Zufall (11.10.).
    static const bool nurMissionsbau = std::strcmp(modus, "missionsbau") == 0;
    // rc601 ton: der Vorschauton wie im Spiel (Hoerer, PVS, Schleifen, Pegel).
    static const bool nurTon = std::strcmp(modus, "ton") == 0;
    ImGuiContext& g = *ImGui::GetCurrentContext();
    g.TestEngineHookItems = true;
    // Stufe 7 Punkt 6d: doppelte Kennungen in diesem Bild - im Map-Editor ein
    // FEHLER (je Fenster und Kennung einmal), sonst ein Hinweis im Protokoll.
    elemente::idPruefungAn(true);
    {
        static std::set<std::pair<std::string, ImGuiID>> gemeldet;
        for (const elemente::IdKonflikt& k : elemente::idKonflikte()) {
            if (!gemeldet.insert({k.fenster, k.id}).second) { continue; }
            std::string label;
            for (const elemente::Element& el : elemente::alle()) {
                if (el.id == k.id && !el.label.empty()) { label = el.label; }
            }
            char z[160];
            std::snprintf(z, sizeof(z), "Kennung 0x%08X bei %.0f %.0f", k.id, static_cast<double>(k.rect.Min.x), static_cast<double>(k.rect.Min.y));
            const std::string text = "ID-Konflikt in \"" + k.fenster + "\": " + z + (label.empty() ? std::string() : " (\"" + label + "\")");
            if (g_app != nullptr && g_app->leftMode == 3) {
                meldeFehler(text);
            } else {
                diag::detail("Knopftest: Hinweis " + text);
            }
        }
    }
    // rc601 tour: fuer das naechste Bild anmelden (elemente::bildEnde setzt danach
    // TestEngineHookItems nach der Stufe).
    if (g_registerFilter.empty()) {
        elemente::wunschVoll();
    } else {
        elemente::wunschLabels(g_registerFilter);
    }
    Ablauf& a = g_ablauf;
    ++a.bild;

    if (a.teil == 0) {
        if (a.bild == 1) {
            // Der Test erwartet die Zusatzfragen ("New?", "Exit?"). Den
            // Stand des Benutzers merken - er wird beim Beenden gespeichert.
            g_queryVorher = g_app->settings.queryOnDiscard;
            g_app->settings.queryOnDiscard = true;
            g_flussVorher = g_app->settings.flussAnsicht;
            g_app->settings.flussAnsicht = 0;
            for (auto& feld : g_app->splitPanes) { feld.ansicht = 0; }
            // Kein Ton aus dem Lautsprecher, solange getestet wird.
            g_app->stumm = true;
        }
        // Eine Startfrage ("Auto-load most recent file?", nach einem Absturz
        // "didn't exit BehavEd legally") wuerde jeden Klick des Tests
        // abfangen - mit Nein beantworten, der Test laedt seine Skripte
        // selbst.
        if (g_app->frageOffen && a.bild < kVorlauf) {
            diag::detail("Selbsttest: Startfrage mit Nein beantwortet: " + g_app->frageText);
            g_app->frageOffen = false;
            g_app->frageJa = nullptr;
            g_app->frageNein = nullptr;
        }
        if (a.bild == kVorlauf) {
            openScriptFromMemory(kSkript, "selbsttest.txt");
            diag::detail("Selbsttest: Skript geladen");
        }
        if (a.bild >= kVorlauf + 5) {
            a.teil = 1;
            if (nurZeitleiste) {
                a.schritte = zeitleisteSchritte();
                for (Schritt& x : tonSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 ton
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurTon) {
                // rc601 ton
                a.schritte = tonSchritte();
                for (Schritt& x : regelwerkSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 integ
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurAblauf) {
                a.schritte = ablaufSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurMissionsbau) {
                a.schritte = missionsbauSchritte();
                for (Schritt& x : saberSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 saber
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurHandbuch) {
                a.schritte = handbuchSchritte();
                // rc603: BHED_HB_BIS=<bildname> - nur bis zu diesem Foto (ein
                // einzelnes Bild neu aufnehmen, etwa h_mission_wahl).
                if (const char* bis = std::getenv("BHED_HB_BIS")) {
                    for (std::size_t i = 0; i < a.schritte.size(); ++i) {
                        if (a.schritte[i].text == std::string("Foto ") + bis) {
                            a.schritte.resize(i + 1);
                            break;
                        }
                    }
                }
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurJk2) {
                a.schritte = jk2Schritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurMissionsliste) {
                a.schritte = missionsListeSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurLetzte) {
                a.schritte = letzteMissionSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurMissionFoto) {
                a.schritte = missionFotoSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurPruefung) {
                a.schritte = pruefungSchritte();
                for (Schritt& x : neustartSchritte()) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurMission) {
                a.schritte = missionReiterSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurEinzel) {
                a.schritte = einzelSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurReiter) {
                a.schritte = kopfSchritte();
                for (Schritt& x : belegungSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : reiterSchliessenSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : tourRegisterSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 tour
                for (Schritt& x : tourSchritte()) { a.schritte.push_back(std::move(x)); }           // rc601 tour
                for (Schritt& x : tourPruefungSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 tour (Pruefung)
                for (Schritt& x : reiterRadSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : schutzBeendenSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurSzenen) {
                // rc601 integ: der Regelwerk-Schalter zuerst - die ROFF-Klang-Probe
                // braucht eine Ansicht ohne geladene Karte (PVS), die Szenen laden welche.
                a.schritte = regelwerkSchritte();
                for (Schritt& x : szenenSchritte()) { a.schritte.push_back(std::move(x)); }
                // rc601 bewegung: Laufen und Drehen im Programm.
                for (Schritt& x : bewegungSchritte()) { a.schritte.push_back(std::move(x)); }
                // rc601 bewegung, Durchsicht 2: Schwierigkeit.
                for (Schritt& x : bewegungReview2Schritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurAlleKarten) {
                a.schritte = alleKartenSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurFix604b) {
                a.schritte = fix604bSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurRest605) {
                a.schritte = rest605Schritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurUmzug) {
                a.schritte = umzugSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurFigurDeform) {
                a.schritte = figurDeformSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurEffektGrund) {
                a.schritte = effektGrundstufeSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurZielstrahl) {
                a.schritte = zielStrahlSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurGeschuetzAnim) {
                a.schritte = geschuetzAnimSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurModellImFesten) {
                a.schritte = modellImFestenSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurLaserfalle) {
                a.schritte = tripMineSchritte();
                for (Schritt& x : hurtUnsterblichSchritte()) { a.schritte.push_back(std::move(x)); }   // rc605 (Ende)
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurGegenstaende) {
                // rc602: Gegenstaende der Karte (md3 und glm).
                a.schritte = gegenstaendeModusSchritte();
                for (Schritt& x : fix604bSchritte()) { a.schritte.push_back(std::move(x)); }   // rc604 fix604b
                for (Schritt& x : effektGluehenSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : festeModelleSchritte()) { a.schritte.push_back(std::move(x)); }
                // rc604 fix604: in einem leeren Reiter - dort stehen alle Figuren vom
                // Kartenstart (leererReiterMitKarte).
                for (Schritt& x : leererReiterMitKarte("md_am_sith", std::make_shared<int>(-1))) { a.schritte.push_back(std::move(x)); }
                a.schritte.push_back(pause(0.5));
                for (Schritt& x : figurStufenSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : figurAnbauAchsenSchritte("md_am_sith")) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : figurStufenDurchsichtSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : figurStufenSpielordnerSchritte()) { a.schritte.push_back(std::move(x)); }   // Zusammenfuehrung rc603
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurFigurStufen) {
                // rc603: nur die Figurstufen in md_am_sith (Bezugsweg, schneller
                // Stufenweg, jede Figurenart, geteilte Bilder) - wie im Modus kartenfunde.
                // rc604 fix604: in einem leeren Reiter - dort stehen alle Figuren vom
                // Kartenstart (leererReiterMitKarte).
                for (Schritt& x : leererReiterMitKarte("md_am_sith", std::make_shared<int>(-1))) { a.schritte.push_back(std::move(x)); }
                a.schritte.push_back(pause(0.5));
                for (Schritt& x : figurStufenSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : figurAnbauAchsenSchritte("md_am_sith")) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : figurStufenDurchsichtSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : figurStufenSpielordnerSchritte()) { a.schritte.push_back(std::move(x)); }   // Zusammenfuehrung rc603
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurFigurLeistung) {
                // rc603: was die Figurstufen kosten - dasselbe Bild ohne Stufen,
                // im Bezugsweg und im schnellen Stufenweg, abwechselnd.
                a.schritte = figurLeistungSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurKartenpruefung) {
                // rc602: jede Karte gegen das Spiel (tools/kartenpruefung.py).
                a.schritte = kartenPruefungSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurKuerzel) {
                a.schritte = aehnlichSchritte();
                for (Schritt& x : wunschSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : zonenSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : revertSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : undoListeSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : reiterSchliessenSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : sicherungSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : kuerzelAlleSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurUpdate) {
                a.schritte = updateSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurEigenstaendig) {
                a.schritte = eigenstaendigSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurFenster) {
                a.schritte = teilerSchritte();
                for (Schritt& x : fensterSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurIcons) {
                a.schritte = iconSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurLesezeichen) {
                a.schritte = lesezeichenSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurZiehen) {
                a.schritte = ziehSchritte();
                for (Schritt& x : ziehBreitSchritte()) { a.schritte.push_back(std::move(x)); }
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurVergleich) {
                a.schritte = vergleichSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurKarte || nurLava || nurHolo || nurOrt || nurKamera || nurWeg || nurUmriss || nurRoff || nurNpcKlasse || nurGluehen || nurTuerLauf || nurSpulen || nurGras) {
                a.schritte = karteSchritte();
                if (nurSpulen) {
                    // rc603 fix603: nur laden, dann vor- und zurueckspulen
                    // (intro2_sith), dann md_dwtdl_jedi mit cin3_jedi.
                    a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 8));
                    for (Schritt& x : spulenSchritte()) { a.schritte.push_back(std::move(x)); }
                    for (Schritt& x : spulenSchwertSchritte()) { a.schritte.push_back(std::move(x)); }
                } else if (nurTuerLauf) {
                    // rc603 fix603: nur laden, dann die Tuer-Laeufe; mit
                    // BHED_MISSION=<karte> eine andere Mission, nur die Uebersicht.
                    a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 8));
                    if (const char* mi = std::getenv("BHED_MISSION")) {
                        a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 2));
                        // rc603 fix603: eine Liste oder "alle" - erst nach dem Start
                        // bekannt (die Spielordner), also als eingefuegte Schritte.
                        const std::string wunsch = mi;
                        a.schritte.push_back({"Tuer-Lauf: Missionen " + wunsch, [wunsch](int) {
                                                  const std::vector<std::string> liste = tuerLaufMissionen(wunsch);
                                                  std::string namen;
                                                  for (const std::string& k : liste) { namen += " " + k; }
                                                  diag::detail("Knopftest: Tuer-Lauf - " + std::to_string(liste.size()) + " Missionen:" + namen);
                                                  std::vector<Schritt> neu;
                                                  for (const std::string& karte : liste) {
                                                      auto ab = std::make_shared<int>(0);
                                                      for (Schritt& x : tuerLaufLadenSchritte(karte, ab)) { neu.push_back(std::move(x)); }
                                                      // BHED_PROBE="<reiter> <figur> <x> <y> <bis ms>": ein Weg genau.
                                                      if (const char* pr = std::getenv("BHED_PROBE")) {
                                                          std::istringstream is(pr);
                                                          std::string rt;
                                                          std::string fg;
                                                          float px = 0.0F;
                                                          float py = 0.0F;
                                                          double pb = 0.0;
                                                          if (is >> rt >> fg >> px >> py >> pb) { tuerlauf::probe(neu, rt, fg, px, py, pb); }
                                                      }
                                                      for (Schritt& x : tuerLaufAlleSchritte(ab, karte, liste.size() > 1)) { neu.push_back(std::move(x)); }
                                                  }
                                                  schritteEinfuegen(std::move(neu));
                                                  return true;
                                              }});
                    } else {
                        for (Schritt& x : tuerLaufSchritte()) { a.schritte.push_back(std::move(x)); }
                        for (Schritt& x : tuerLaufAlleSchritte(std::make_shared<int>(0), "md_am_sith", false)) { a.schritte.push_back(std::move(x)); }
                    }
                } else if (nurNpcKlasse) {
                    // rc602: nur laden, dann der Droide (npcKlasseSchritte).
                    a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 8));
                    for (Schritt& x : npcKlasseSchritte()) { a.schritte.push_back(std::move(x)); }
                } else if (nurGras) {
                    // rc604: nur laden, dann yavin1b mit Gras.
                    a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 8));
                    for (Schritt& x : grasSchritte()) { a.schritte.push_back(std::move(x)); }
                } else if (nurGluehen) {
                    // rc602: nur laden, dann das Gluehen der Brush-Entities.
                    a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 8));
                    for (Schritt& x : gluehenMoverSchritte()) { a.schritte.push_back(std::move(x)); }
                    for (Schritt& x : fxRunnerStartSchritte()) { a.schritte.push_back(std::move(x)); }
                    for (Schritt& x : gluehenFigurSchritte()) { a.schritte.push_back(std::move(x)); }
                } else if (nurRoff) {
                    a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 8));
                    for (Schritt& x : roffSchritte()) { a.schritte.push_back(std::move(x)); }
                } else if (nurLava || nurHolo || nurOrt || nurKamera || nurWeg || nurUmriss) {
                    // Nur laden (die ersten Schritte von karteSchritte bis
                    // "Kartenansicht gezeichnet"), dann Lava oder Holotisch.
                    a.schritte.resize(std::min<std::size_t>(a.schritte.size(), 8));
                    for (Schritt& x : (nurLava ? lavaSchritte() : nurHolo ? holoSchritte()
                                       : nurKamera ? kameraSchritte() : nurWeg ? wegSchritte()
                                       : nurUmriss ? umrissSchritte() : ortSchritte())) {
                        a.schritte.push_back(std::move(x));
                    }
                    if (nurUmriss) {
                        for (Schritt& x : folgeSchritte()) { a.schritte.push_back(std::move(x)); }
                    }
                    // Paket kamera, rc601: Bildwinkel, Spielfenster, Rollen, PATH.
                    if (nurKamera) {
                        for (Schritt& x : kameraSpielfensterSchritte()) { a.schritte.push_back(std::move(x)); }
                        for (Schritt& x : kameraPfadSchritte()) { a.schritte.push_back(std::move(x)); }
                        // Paket kamera, rc601 (Review): Pilot-Zoom, Vorschau 80, Gizmo-Grundlage.
                        for (Schritt& x : kameraReviewSchritte()) { a.schritte.push_back(std::move(x)); }
                        // rc601 ton: PVS in der geladenen Karte
                        for (Schritt& x : tonKarteSchritte()) { a.schritte.push_back(std::move(x)); }
                    }
                    if (nurWeg) {
                        for (Schritt& x : kartenWerkzeugSchritte()) { a.schritte.push_back(std::move(x)); }
                        for (Schritt& x : animSchritte()) { a.schritte.push_back(std::move(x)); }
                        for (Schritt& x : indexSchritte()) { a.schritte.push_back(std::move(x)); }
                        for (Schritt& x : lichtSchritte()) { a.schritte.push_back(std::move(x)); }
                    }
                } else {
                    for (Schritt& x : kartenBreitSchritte()) { a.schritte.push_back(std::move(x)); }
                    for (Schritt& x : kameraSchritte()) { a.schritte.push_back(std::move(x)); }
                    // Paket kamera, rc601: Bildwinkel, Spielfenster, Rollen, PATH.
                    for (Schritt& x : kameraSpielfensterSchritte()) { a.schritte.push_back(std::move(x)); }
                    for (Schritt& x : kameraPfadSchritte()) { a.schritte.push_back(std::move(x)); }
                    // Paket kamera, rc601 (Review): Pilot-Zoom, Vorschau 80, Gizmo-Grundlage.
                    for (Schritt& x : kameraReviewSchritte()) { a.schritte.push_back(std::move(x)); }
                    for (Schritt& x : tonKarteSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 ton
                    // rc601 bewegung, teil 3: Tueren zur Zeit fuer die Wege.
                    for (Schritt& x : bewegungTeil3Schritte()) { a.schritte.push_back(std::move(x)); }
                    // rc602: NPC-Klassen mit eigenem NPC_type (der Droide in md_am_sith).
                    for (Schritt& x : npcKlasseSchritte()) { a.schritte.push_back(std::move(x)); }
                    // rc603 fix603: Anakin durch die Tuer (ani_walk, intro2_sith).
                    for (Schritt& x : tuerLaufSchritte()) { a.schritte.push_back(std::move(x)); }
                    // rc603 fix603: intro2_sith vor- und zurueckspulen (Bildzeiten, Pruefsummen).
                    for (Schritt& x : spulenSchritte()) { a.schritte.push_back(std::move(x)); }
                    // rc602: Gluehen auf Brush-Entities/Kartenmodellen/Figuren und
                    // der erste fx_runner-Durchgang - gleich nach dem Laden, bevor
                    // die Kartenfunktionen Entities verschieben und aendern.
                    {
                        std::vector<Schritt> gEffekte = effekteKartenstartSchritte();
                        const std::size_t an = std::min<std::size_t>(a.schritte.size(), 8);
                        a.schritte.insert(a.schritte.begin() + static_cast<std::ptrdiff_t>(an),
                                          std::make_move_iterator(gEffekte.begin()), std::make_move_iterator(gEffekte.end()));
                    }
                }
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurModell) {
                a.schritte = modellSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurKarteneditor) {
                a.schritte = karteneditorSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurAblage) {
                a.schritte = ablageSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurOriginal) {
                openScriptFromMemory(kSkript, "original.txt");
                a.schritte = originalSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            } else if (nurGeteilt) {
                a.schritte = geteiltSchritte();
                for (Schritt& x : endeSchritte(false)) { a.schritte.push_back(std::move(x)); }
                a.teil = 2;
            }
        }
    } else if (a.teil == 1) {
        if (ruheBild(a.ruhe)) {
            char z[120];
            std::snprintf(z, sizeof(z), "Selbsttest Ruhe fertig: %d Rucke gesamt",
                          a.ruhe.ruckGesamt);
            diag::detail(a.ruhe.bericht);
            diag::detail(z);
            // Frisches Skript fuer die Knoepfe - der Ruhetest hat nichts
            // geaendert, aber die Klickpruefung soll nicht davon abhaengen.
            openScriptFromMemory(kSkript, "knopftest.txt");
            a.schritte = knopfSchritte();
            for (Schritt& x : befehlSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : geteiltSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : ziehSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : ziehBreitSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : lesezeichenSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : fensterSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : aehnlichSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : wunschSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : zonenSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : revertSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : undoListeSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : reiterSchliessenSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : ablageSchliessenSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : tourRegisterSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 tour
            for (Schritt& x : tourSchritte()) { a.schritte.push_back(std::move(x)); }           // rc601 tour
            for (Schritt& x : tourPruefungSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 tour (Pruefung)
            for (Schritt& x : reiterRadSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : sicherungSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : zeitleisteSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : tonSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 ton
            // rc601 integ: der zentrale Schalter OpenJK / Movie Duels (neben dem
            // Ton: die ROFF-Klang-Probe braucht dieselbe Ansicht ohne Karte).
            for (Schritt& x : regelwerkSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : teilerSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : kuerzelAlleSchritte()) { a.schritte.push_back(std::move(x)); }
            // rc601 release: die .exe allein (Kopie ohne data\base daneben).
            for (Schritt& x : eigenstaendigSchritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : saberSchritte()) { a.schritte.push_back(std::move(x)); }   // rc601 saber
            // rc601 bewegung: Laufen und Drehen im Programm (stellt Karte und
            // Skript danach zurueck).
            for (Schritt& x : bewegungSchritte()) { a.schritte.push_back(std::move(x)); }
            // rc601 bewegung, Durchsicht 2: Schwierigkeit.
            for (Schritt& x : bewegungReview2Schritte()) { a.schritte.push_back(std::move(x)); }
            // rc601 bewegung: Schwerkraft (nur mit geladener Karte).
            for (Schritt& x : schwerkraftSchritte()) { a.schritte.push_back(std::move(x)); }
            // rc601 bewegung, teil 3: Tueren zur Zeit fuer die Wege (echte Karte).
            for (Schritt& x : bewegungTeil3Schritte()) { a.schritte.push_back(std::move(x)); }
            for (Schritt& x : endeSchritte(true)) { a.schritte.push_back(std::move(x)); }
            a.teil = 2;
        }
    } else if (a.teil == 2) {
        if (a.schritt < a.schritte.size()) {
            // Kopien, keine Verweise: ein Schritt darf weitere hinter sich
            // einfuegen (fuelleAb), und dabei zieht der Vektor um.
            const std::string text = a.schritte[a.schritt].text;
            // Dauerbeobachtung der Action-Leiste: jede Aenderung von Ort
            // oder Groesse des Delete-Knopfs, mit dem laufenden Schritt.
            // shank, 27.09.: "manchmal beim verschieben springt die action
            // leiste und wird groesser".
            {
                static ImRect vorher;
                static bool gesehen = false;
                if (const Element* e = finde(tr(Str::AppExit), "buttons", 0, false)) {
                    const ImRect& jetzt = e->rect;
                    if (gesehen && (std::fabs(jetzt.Min.x - vorher.Min.x) > 0.5F ||
                                    std::fabs(jetzt.Min.y - vorher.Min.y) > 0.5F ||
                                    std::fabs(jetzt.GetWidth() - vorher.GetWidth()) > 0.5F ||
                                    std::fabs(jetzt.GetHeight() - vorher.GetHeight()) > 0.5F)) {
                        char z[300];
                        std::snprintf(z, sizeof(z),
                                      "Leiste: Exit %.0f/%.0f %.0fx%.0f -> %.0f/%.0f %.0fx%.0f "
                                      "(Schritt \"%s\", Bild %d)",
                                      static_cast<double>(vorher.Min.x), static_cast<double>(vorher.Min.y),
                                      static_cast<double>(vorher.GetWidth()), static_cast<double>(vorher.GetHeight()),
                                      static_cast<double>(jetzt.Min.x), static_cast<double>(jetzt.Min.y),
                                      static_cast<double>(jetzt.GetWidth()), static_cast<double>(jetzt.GetHeight()),
                                      text.c_str(), a.imSchritt);
                        diag::detail(z);
                    }
                    vorher = jetzt;
                    gesehen = true;
                }
                // Tooltips (Hinweise beim Draufzeigen) mitschreiben - sie
                // gehen neben der Leiste auf, wenn die Testmaus auf einem
                // Knopf stehen bleibt.
                static bool tipVorher = false;
                bool tip = false;
                for (ImGuiWindow* w : GImGui->Windows) {
                    if (w->Active && !w->Hidden && (w->Flags & ImGuiWindowFlags_Tooltip) != 0 &&
                        std::strncmp(w->Name, "##Tooltip", 9) == 0) {
                        tip = true;
                        if (!tipVorher) {
                            char z[260];
                            std::snprintf(z, sizeof(z),
                                          "Tooltip: %.0f/%.0f %.0fx%.0f (Schritt \"%s\", %s)",
                                          static_cast<double>(w->Pos.x), static_cast<double>(w->Pos.y),
                                          static_cast<double>(w->Size.x), static_cast<double>(w->Size.y),
                                          text.c_str(), wasImGuiSieht().c_str());
                            diag::detail(z);
                        }
                    }
                }
                tipVorher = tip;
                // Fokuswechsel des Programmfensters: verliert es den Fokus
                // (Klick in ein anderes Programm), setzt ImGui die
                // Maustasten zurueck - ein laufender Zug bricht dann ab.
                static bool fokusVorher = true;
                const bool fokus = !ImGui::GetIO().AppFocusLost;
                if (fokus != fokusVorher) {
                    diag::detail(std::string("Fokus des Programmfensters: ") +
                                 (fokus ? "zurueck" : "VERLOREN") + " (Schritt \"" + text + "\")");
                    fokusVorher = fokus;
                }
            }
            const std::function<bool(int)> tun = a.schritte[a.schritt].tun;
            g_liste = &a.schritte;
            g_einfuegeAn = a.schritt + 1U;
            if (a.imSchritt == 0) { diag::detail("Knopftest: " + text); }
            if (tun(a.imSchritt)) {
                ++a.schritt;
                a.imSchritt = 0;
            } else {
                ++a.imSchritt;
                // Kein Schritt darf ewig laufen - aber Warten (Laden,
                // Abspielen) darf bei ~400 Bildern je Sekunde ein paar
                // Sekunden dauern.
                if (a.imSchritt > 4000 && !a.schritte[a.schritt].eigeneZeitgrenze) {
                    meldeFehler(text + ": haengt");
                    ++a.schritt;
                    a.imSchritt = 0;
                }
            }
        } else {
            // Nach "No" haette das Programm schon zu sein.
            meldeFehler("Beenden: Programm laeuft nach \"No\" weiter");
            a.teil = 3;
            g_app->quitConfirmed = true;
            g_app->wantQuit = true;
        }
    }
    // rc601 tour: das Register leert elemente::bildEnde() (gui/app.cpp, nach
    // selbsttest()) - es wird auch ohne Selbsttest gebraucht (Tutorials).
}

}  // namespace bhed::gui

// --- Die Haken, die ImGui mit IMGUI_ENABLE_TEST_ENGINE aufruft -----------
//
// rc601 tour: stehen jetzt in gui/elemente.cpp (das Register laeuft auch
// ausserhalb des Selbsttests, gefiltert, fuer die Tutorials im Programm).
