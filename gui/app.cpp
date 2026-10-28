// app.cpp - die Oberflaeche, mit Dear ImGui 1.92.
//
// Die Aufteilung folgt Dialog 102 aus BehavEd.exe. Die Zahlen unten sind
// Dialogeinheiten aus der PE-Ressource, nicht geschaetzt:
//   Fenster 663 x 449, Events 7..198, Script Flow 206..578, Knopfspalte 585
// Umgerechnet wird ueber die Basiseinheiten von MS Sans Serif 8pt (6/13), und
// zusaetzlich ueber die Bildschirmskalierung - bei 144 dpi also 1,5.
//
// Der Event-Editor wird zur Laufzeit aus der Parameterangabe gebaut, so wie
// im Original. Rasterzahlen am Bildschirmfoto gemessen (11.08.2026):
//   Feld 167 x 14 dlu, angehaengter Knopf ~33, Abstand 9, erstes Feld bei 100,
//   Knopfzeile 88 unter der Feldzeile.
//
// Kein Text steht hier fest verdrahtet - alles geht durch i18n::tr(). Der
// Pruefer tools/lint_i18n.py schlaegt an, wenn doch.

#include <filesystem>
#include <chrono>
#include <ctime>
#include <fstream>
#include "bhed/messpunkt.h"
#include "bhed/changelog.h"
#include "bhed/fassung.h"
#include "bhed/image.h"   // Stufe 8c: das Logo von JK Studio (PNG-Ressource)
#include "update.h"
#include "app_internal.h"
#include "gpumap.h"
#include "elemente.h"   // rc601 tour: das Element-Register (auch ohne Selbsttest)
#include "tour.h"       // rc601 tour: Tutorials im Programm
#include "karteneditor.h"   // Stufe 1b: der Arbeitsbereich Karten-Editor

#include <atomic>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <set>
#include <sstream>
#include <thread>
#include <string>
#include <vector>
#include "bhed/utf8pfad.h"

namespace bhed::gui {

// Der eine Zustand. Deklariert in app_internal.h, damit app_view3d.cpp ihn
// ebenfalls sieht.
App* g_app = nullptr;

float dx(float v) { return v * kDluX * g_app->uiScale; }
float dy(float v) { return v * kDluY * g_app->uiScale; }




namespace {



// Vorwaertsangaben: die Dateiverwaltung steht weiter unten, wird aber schon
// von den Tastenkuerzeln gebraucht.
void doOpen();
void doNew();
bool doSave();
bool doSaveAs();
// Ausgewaehlt schlaegt Ueberfahren.
//
// ImGui entscheidet andersherum, imgui_widgets.cpp, Selectable():
//
//     ImU32 col = GetColorU32((held && highlighted) ? ImGuiCol_HeaderActive
//                             : highlighted ? ImGuiCol_HeaderHovered
//                                           : ImGuiCol_Header);
//
// Die Maus gewinnt also immer. Bei getrennten Farben - grau fuer
// ueberfahren, blau fuer gewaehlt - heisst das: faehrt man ueber die
// gewaehlte Zeile, wird sie grau, und man sieht nicht mehr, was markiert
// ist. Fuer gewaehlte Zeilen ziehen wir die Hoverfarbe deshalb auf die
// Auswahlfarbe.
struct KeepSelected {
    bool pushed = false;
    // aktiv == false heisst: diese Liste hat die Auswahl nicht zuletzt
    // bekommen. Dann wird gedaempft gezeichnet, so wie Windows es mit einer
    // Liste ohne Fokus macht - man sieht noch, was gewaehlt war, aber es
    // draengt sich nicht mehr auf.
    explicit KeepSelected(bool selected, bool aktiv = true) : pushed(selected) {
        if (!pushed) {
            return;
        }
        ImVec4 h = ImGui::GetStyleColorVec4(ImGuiCol_Header);
        if (!aktiv) {
            const ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            // Zur Hintergrundfarbe hin verblassen.
            h.x = (h.x + bg.x) * 0.5F;
            h.y = (h.y + bg.y) * 0.5F;
            h.z = (h.z + bg.z) * 0.5F;
            ImGui::PushStyleColor(ImGuiCol_Header, h);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, h);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, h);
            pushed3_ = true;
            return;
        }
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, h);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, h);
    }
    ~KeepSelected() {
        if (pushed) {
            ImGui::PopStyleColor(pushed3_ ? 3 : 2);
        }
    }
    bool pushed3_ = false;

    KeepSelected(const KeepSelected&) = delete;
    KeepSelected& operator=(const KeepSelected&) = delete;
    KeepSelected(KeepSelected&&) = delete;
    KeepSelected& operator=(KeepSelected&&) = delete;
};

void insertCommand(const Command& c);
void moveSelection(bool up);
void insertMacro(const Macro& m);

// --- Aus der Ereignisliste in den Baum ziehen ---------------------------
//
// Die Nutzlast traegt nur, WAS gezogen wird - nicht wohin. Das Ziel weiss
// der Baum selbst, aus der Stelle, an der die Maus losgelassen wird.
//
// Eine Zahl statt eines Zeigers: die Nutzlast wird von ImGui kopiert und
// ueberlebt das Bild, in dem sie gesetzt wurde. Ein Zeiger in
// g_app->db.commands waere gueltig, aber die Nummer ist ehrlicher.
struct NewDrag {
    int kind = 0;    // 0 = Befehl, 1 = Makro
    int index = 0;   // Nummer in db.commands bzw. db.macroBodies
};

// Wohin, bezogen auf die Zielzeile.
enum class DropWhere { Before, Into, After };

// Wohin faellt, was ueber dieser Zeile losgelassen wird? Drei Zonen, fuer
// BEIDE Arten zu ziehen (neu aus der Ereignisliste und Umordnen im Baum):
// oberes Drittel davor, Mitte hinein (nur bei einem Block), unteres Drittel
// dahinter. shank: "kannst du dieselbe Logik auch machen ... wenn man im
// Script Flow selbst rumschiebt? entweder darunter, darueber oder darauf".
//
// Unter einem AUFGEKLAPPTEN Block mit Inhalt steht die Linie genau ueber
// seinem ersten Kind - dort heisst es deshalb auch "hinein, an den Anfang".
// Hinter den ganzen Block kommt man ueber die Zeile nach dem Block (oben).
struct Ablage {
    DropWhere wo = DropWhere::After;
    bool alsLinie = true;    // Linie (dazwischen) statt Rahmen (darauf)
    bool linieOben = false;
};
Ablage ablageZone(float anteil, bool block, bool aufMitInhalt) {
    Ablage z;
    if (anteil < 0.3F) {
        z.wo = DropWhere::Before;
        z.linieOben = true;
    } else if (block && anteil <= 0.7F) {
        z.wo = DropWhere::Into;
        z.alsLinie = false;
    } else if (block && aufMitInhalt) {
        z.wo = DropWhere::Into;   // Linie unter der Zeile = vor dem ersten Kind
    } else {
        z.wo = DropWhere::After;
    }
    return z;
}
void zeichneAblage(const Ablage& z, ImVec2 a, ImVec2 b) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 mark = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
    if (!z.alsLinie) {
        dl->AddRect(a, b, mark, 0.0F, 0, 2.0F);
    } else {
        const float y = z.linieOben ? a.y : b.y;
        dl->AddLine(ImVec2{a.x, y}, ImVec2{b.x, y}, mark, 2.0F);
    }
}

void dropNewAt(const NewDrag& what, const Path& target, DropWhere where);
void moveSelectionByKey(int delta);
void handleTreeKeys();
// Die Symboltexturen stehen weiter unten, werden aber schon vom
// Ueber-Dialog gebraucht.
void* logoTextureId();
void* jkLogoTextureId(int& b, int& h);
extern const char* kScriptFilter;
void loadPath(const std::string& p);
int reiterMitPfadI(const std::string& p);
namespace {
std::string pfadSchluessel(const std::string& p);   // steht weiter unten, ebenso verschachtelt
}  // namespace
bool speichereReiterI(int tab);
void oeffneSkriptAusSpeicherI(const std::string& data, const std::string& name);
std::string descFor(const Command& c);
void doSaveAll();
ImGuiKeyChord chordFor(keys::Action a);
void doBackup();
void doRestore();
void springeZuTreffer();
void findNext(int step);
void doUndo();
void clearSelection();
void doRedo();
const char* stepName(const char* key);
void addGamePath();
void applyTheme(const theme::Theme& t);

// Fuer InputText mit std::string: ImGui meldet die neue Laenge, der
// String waechst mit (wie misc/cpp/imgui_stdlib.cpp).
int textFeldWaechst(ImGuiInputTextCallbackData* d) {
    if (d->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* text = static_cast<std::string*>(d->UserData);
        text->resize(static_cast<std::size_t>(d->BufTextLen));
        d->Buf = text->data();
    }
    return 0;
}

float dx(float v) { return v * kDluX * g_app->uiScale; }
float dy(float v) { return v * kDluY * g_app->uiScale; }

ImVec4 col(const theme::Color& c) { return ImVec4{c.r, c.g, c.b, c.a}; }

// Massstab anwenden - nach der Anleitung in ImGuis FAQ.
//
// Zwei Regeln, die ich zuerst beide verletzt hatte:
//
//  1. Die Schrift wird NICHT in der Zielgroesse gebacken. Seit 1.92 rastert
//     ImGui dynamisch: Grundgroesse in style.FontSizeBase, Massstab in
//     style.FontScaleDpi. Wer beides multipliziert - Schrift mit 16*1,5
//     backen UND FontScaleDpi=1,5 setzen - bekommt 36 Punkt statt 24.
//  2. style.ScaleAllSizes() ist ausdruecklich "call once!". Es multipliziert
//     die Abstaende jedes Mal weiter. Wer den Massstab aendern will, muss
//     laut FAQ "reset the style and call this again". Ohne das bleiben die
//     Polster klein, waehrend die Schrift waechst - dann sieht es gequetscht
//     aus, genau wie gemeldet.
void applyScale(float total) {
    // Vom Urzustand aus, nicht vom bereits skalierten.
    ImGui::GetStyle() = ImGuiStyle{};
    if (g_app->activeTheme != nullptr) {
        applyTheme(*g_app->activeTheme);
    }
    ImGuiStyle& s = ImGui::GetStyle();
    s.ScaleAllSizes(total);
    s.FontSizeBase = 16.0F;

    // ImGui trennt die beiden Massstaebe, und das tun wir jetzt auch:
    //
    //   FontScaleDpi   der Bildschirm - kommt von Windows
    //   FontScaleMain  die Einstellung des Benutzers
    //
    // In imgui.h steht zu FontScaleMain ausdruecklich "may be set by
    // application once, or exposed to end-user" - genau dafuer ist er da.
    // Vorher stand das Produkt in FontScaleDpi; das ergab dieselbe Groesse,
    // vermischte aber zwei Dinge, die verschiedene Ursachen haben.
    s.FontScaleDpi = g_app->dpiScale;
    s.FontScaleMain = g_app->uiScale;
    g_app->appliedScale = total;
}

void applyTheme(const theme::Theme& t) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.Colors[ImGuiCol_WindowBg] = col(t.window);
    s.Colors[ImGuiCol_ChildBg] = col(t.child);
    s.Colors[ImGuiCol_PopupBg] = col(t.window);
    s.Colors[ImGuiCol_Text] = col(t.text);
    s.Colors[ImGuiCol_TextDisabled] = col(t.textDisabled);
    s.Colors[ImGuiCol_Button] = col(t.button);
    s.Colors[ImGuiCol_ButtonHovered] = col(t.buttonHovered);
    s.Colors[ImGuiCol_ButtonActive] = col(t.buttonActive);
    s.Colors[ImGuiCol_FrameBg] = col(t.frame);
    s.Colors[ImGuiCol_FrameBgHovered] = col(t.frameHovered);
    s.Colors[ImGuiCol_FrameBgActive] = col(t.frameHovered);
    s.Colors[ImGuiCol_Border] = col(t.border);
    s.Colors[ImGuiCol_Header] = col(t.header);
    // Ueberfahren ist NICHT dasselbe wie ausgewaehlt.
    //
    // Hier stand dreimal dieselbe Farbe. Damit sah jede Zeile, ueber der
    // gerade die Maus stand, aus wie die gewaehlte - und in einer langen
    // Liste weiss man dann nicht mehr, was wirklich markiert ist.
    //
    // Ueberfahren bekommt jetzt das neutrale Grau der Knopfleiste, gewaehlt
    // behaelt die Betonungsfarbe. Gedrueckt liegt dazwischen.
    s.Colors[ImGuiCol_HeaderHovered] = col(t.buttonHovered);
    s.Colors[ImGuiCol_HeaderActive] = col(t.buttonActive);
    s.Colors[ImGuiCol_CheckMark] = col(t.accent);
    s.Colors[ImGuiCol_TitleBgActive] = col(t.accent);
    s.Colors[ImGuiCol_PlotHistogram] = col(t.accent);
    // Ohne diese blieb die Menueleiste in ImGuis Vorgabefarbe - im hellen
    // Aussehen ein dunkler Balken quer ueber dem Fenster.
    s.Colors[ImGuiCol_MenuBarBg] = col(t.window);
    s.Colors[ImGuiCol_TitleBg] = col(t.window);
    s.Colors[ImGuiCol_TitleBgCollapsed] = col(t.window);
    s.Colors[ImGuiCol_Separator] = col(t.border);
    s.Colors[ImGuiCol_SeparatorHovered] = col(t.accent);
    s.Colors[ImGuiCol_SeparatorActive] = col(t.accent);
    s.Colors[ImGuiCol_ScrollbarBg] = col(t.window);
    s.Colors[ImGuiCol_ScrollbarGrab] = col(t.button);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = col(t.buttonHovered);
    s.Colors[ImGuiCol_ScrollbarGrabActive] = col(t.buttonActive);
    s.Colors[ImGuiCol_SliderGrab] = col(t.accent);
    s.Colors[ImGuiCol_SliderGrabActive] = col(t.accent);
    s.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4{0.0F, 0.0F, 0.0F, 0.35F};
    // Bei der klassischen Farbgebung steht die Auswahl auf dunkelblau -
    // dort muss der Text weiss sein, sonst ist er unlesbar.
    s.Colors[ImGuiCol_NavCursor] = col(t.accent);

    // Windows 95 hatte harte Ecken; alles andere darf gerundet sein.
    const float r = t.raisedBorders ? 0.0F : 3.0F;
    s.FrameRounding = r;
    s.WindowRounding = r;
    s.ChildRounding = r;
    s.PopupRounding = r;
    s.GrabRounding = r;
    s.FrameBorderSize = 1.0F;
    s.WindowBorderSize = 1.0F;
    s.ChildBorderSize = 1.0F;
}

// Ein Knopf in der rechten Spalte: volle Breite der Gruppe.


// --- Event-Editor ------------------------------------------------------

// Fuer einen VORHANDENEN Knoten: seine Werte, nicht die Vorgaben.

// `editorFields` ist seit rc468 in `src/edit.cpp`.
//
// Sie entscheidet, WAS im Fenster steht - und lag in der Oberflaeche, wo
// sie `g_app` las. Damit war die eine Funktion, die die Fenster aufbaut,
// die einzige, die keine Probe erreichen konnte.
//
// Jetzt nimmt sie die Werte als Argument. `tests/editorall.cpp` baut damit
// die Felder jedes Befehls fuer JEDEN Eintrag jeder Klappliste auf.


const char* hintFor(const Param& p) {
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

// Eine Klappliste aus einer Typmenge.
// `comboEdit` ist bereits in `app_internal.h` deklariert (Zeile 1514) - eine
// zweite Ankuendigung hier waere die zweite Fassung derselben Signatur.
// Eine Klappliste ueber einen Typsatz.
//
// Sie zeichnete frueher ein blankes `BeginCombo` - ohne Filterzeile. Das
// Feld oben im Ereignisfenster hatte eine, die Helferzeilen darunter nicht:
// shank sah dieselbe Liste "Parm strings" einmal mit Suchfeld und einmal
// ohne.
//
// Zwei Fassungen derselben Bedienung. Jetzt stuetzt sich diese hier auf
// `comboEdit`, und damit haben ALLE Typklapplisten den Filter - auch die
// vier Aufrufe in `drawHelperRows` und jeder, der spaeter dazukommt.
bool typeSetCombo(const char* id, const char* set, std::string* value,
                  float width) {
    const TypeSet* ts = g_app->db.typeset(set);
    const ComboEditResult r =
        comboEdit(id, *value, (ts != nullptr) ? &ts->entries : nullptr, width);
    return r.edited || r.picked;
}

// Die Helferzeilen unter einem Feld - Bilder 4 und 6 des Originals.
//
// Sie bauen die drei Ausdruecke, die ICARUS kennt. Die Signatur von get()
// steht in der behaved.bhc selbst, in einer auskommentierten Zeile:
//     //get( %t="DECLARE_TYPE", %s="variablename" );
//     //# OF NO USE BY ITSELF ... but is still usable inside other commands
//
// Abweichung vom Original, bewusst: dort hat die Tag-Zeile nur eine
// Klappliste, der Name kommt von woanders. Hier steht ein eigenes Feld
// dafuer - sonst kann man tag() ohne Umweg ueber die Tastatur nicht bauen.
// Sorgt dafuer, dass alle Reihen des Helfers so viele Eintraege haben wie
// es Felder gibt. EINE Stelle - sonst waeren es sechs Bedingungen, die
// auseinanderlaufen koennen.
void helferGross(std::size_t n) {
    // NUR WACHSEN. `resize` schrumpft auch, und genau daran ist die
    // Bedienung gescheitert:
    //
    // Aufgerufen wird sie mit `i + 1` - fuer Spalte 0 also mit 1. Ein
    // `resize(1)` auf eine Reihe mit drei Eintraegen wirft die hinteren
    // beiden WEG. In jedem Bild.
    //
    // Was man in Spalte 2 oder 3 auswaehlte, war beim naechsten Bild
    // wieder die Vorgabe: shank sah die Klappliste aufgehen, konnte sie
    // bedienen - und nichts blieb stehen. "angles laesst sich nicht
    // auswaehlen."
    //
    // Es war nie die Ausrichtung. Die drei Runden davor haben ein zweites,
    // echtes Problem behoben (die Bloecke ueberlappten wirklich), aber
    // nicht dieses.
    if (n <= g_app->helpGetType.size()) {
        return;
    }
    g_app->helpGetType.resize(n, "FLOAT");
    g_app->helpGetName.resize(n, "SET_PARM1");
    // Wie im Original: Tag schreibt ohne Eingabe tag( "targetname", ORIGIN).
    g_app->helpTagName.resize(n, "targetname");
    g_app->helpTagType.resize(n, "ORIGIN");
    g_app->helpRangeLow.resize(n, 0.0F);
    g_app->helpRangeHigh.resize(n, 1.0F);
}

// Leerzeichen vorn und hinten weg. Es gibt in dieser Datei bisher keine
// solche Hilfe; die Leser in `src/` haben ihre eigene.
std::string ohneRand(const std::string& t) {
    std::size_t a = 0;
    std::size_t b = t.size();
    while (a < b && (t[a] == ' ' || t[a] == '\t')) { ++a; }
    while (b > a && (t[b - 1] == ' ' || t[b - 1] == '\t')) { --b; }
    return t.substr(a, b - a);
}

// Die Helferzeilen aus den Feldwerten zurueckgewinnen.
//
// Der Helfer baut aus vier Zeilen einen Ausdruck und schreibt ihn ins
// Feld. Gespeichert wird das FELD; die vier Zeilen sind reine Bedienung
// und standen beim naechsten Oeffnen wieder auf den Vorgaben - auch wenn
// im Feld `get( FLOAT, "SET_PARM1" )` stand.
//
// Die drei Formen werden an genau einer Stelle gebaut (siehe die Knoepfe
// Get, Tag und Rnd), also lassen sie sich an genau einer Stelle
// zurueckgewinnen. Was nicht passt, bleibt unangetastet: ein von Hand
// getippter Ausdruck soll die Zeilen nicht auf geratene Werte stellen.
// Schluessel fuer die gemerkten Helferzeilen: "if/0", "set/1" ...
std::string helferSchluessel(const std::string& befehl, std::size_t feld) {
    return befehl + "/" + std::to_string(feld);
}

// Die gemerkten Helferzeilen fuer diesen Befehl holen.
//
// Sie stehen in den Einstellungen, nicht im Skript - siehe die Begruendung
// bei `Settings::helfer`. Zuerst diese, DANACH das, was im Feld steht: ein
// `get( ... )` im Feld ist die staerkere Wahrheit, weil es tatsaechlich
// gespeichert wurde.
void helferAusEinstellungen(const std::string& befehl) {
    helferGross(g_app->editorValues.size());
    for (std::size_t i = 0; i < g_app->editorValues.size(); ++i) {
        const auto it = g_app->settings.helfer.find(helferSchluessel(befehl, i));
        if (it == g_app->settings.helfer.end()) {
            continue;
        }
        // Typ|Name|Marke|Art|von|bis
        std::vector<std::string> teile;
        std::string rest = it->second;
        for (;;) {
            const std::size_t k = rest.find('|');
            if (k == std::string::npos) { teile.push_back(rest); break; }
            teile.push_back(rest.substr(0, k));
            rest = rest.substr(k + 1);
        }
        if (teile.size() < 6) {
            continue;   // alte oder kaputte Zeile - lieber nichts anfassen
        }
        g_app->helpGetType[i] = teile[0];
        g_app->helpGetName[i] = teile[1];
        g_app->helpTagName[i] = teile[2];
        g_app->helpTagType[i] = teile[3];
        g_app->helpRangeLow[i] = static_cast<float>(std::atof(teile[4].c_str()));
        g_app->helpRangeHigh[i] = static_cast<float>(std::atof(teile[5].c_str()));
    }
}

// Und zurueckschreiben - beim Ok und beim Abbrechen, damit die Wahl auch
// dann bleibt, wenn am Skript nichts geaendert wurde.
void helferInEinstellungen(const std::string& befehl) {
    for (std::size_t i = 0; i < g_app->editorValues.size() &&
                            i < g_app->helpGetType.size(); ++i) {
        char zahl[64];
        std::snprintf(zahl, sizeof(zahl), "%.3f|%.3f",
                      static_cast<double>(g_app->helpRangeLow[i]),
                      static_cast<double>(g_app->helpRangeHigh[i]));
        g_app->settings.helfer[helferSchluessel(befehl, i)] =
            g_app->helpGetType[i] + "|" + g_app->helpGetName[i] + "|" +
            g_app->helpTagName[i] + "|" + g_app->helpTagType[i] + "|" + zahl;
    }
}

void helferAusWert() {
    helferGross(g_app->editorValues.size());
    for (std::size_t i = 0; i < g_app->editorValues.size(); ++i) {
        const std::string& v = g_app->editorValues[i];
        // Steht im Feld selbst ein SET-Name, ist er der Vorschlag fuer Get -
        // so macht es das Original nach "Expr!": bei den Makrozeilen
        // set ( "SET_BEHAVIOR_STATE", ... ) steht darunter
        // [SET_BEHAVIOR_STATE v] [Get] statt SET_PARM1.
        if (const TypeSet* st = g_app->db.typeset("SET_TYPES");
            st != nullptr && !v.empty() && st->find(v) != nullptr) {
            g_app->helpGetName[i] = v;
            continue;
        }
        // get( TYP, "NAME" )
        if (v.rfind("get(", 0) == 0) {
            const std::size_t k = v.find(',');
            const std::size_t a1 = v.find('"');
            const std::size_t a2 = (a1 == std::string::npos)
                                       ? std::string::npos
                                       : v.find('"', a1 + 1);
            if (k != std::string::npos && a1 != std::string::npos &&
                a2 != std::string::npos && k > 4) {
                g_app->helpGetType[i] = ohneRand(v.substr(4, k - 4));
                g_app->helpGetName[i] = v.substr(a1 + 1, a2 - a1 - 1);
            }
            continue;
        }
        // tag( "NAME", ART )
        if (v.rfind("tag(", 0) == 0) {
            const std::size_t a1 = v.find('"');
            const std::size_t a2 = (a1 == std::string::npos)
                                       ? std::string::npos
                                       : v.find('"', a1 + 1);
            const std::size_t k = v.find(',', (a2 == std::string::npos) ? 0 : a2);
            const std::size_t zu = v.rfind(')');
            if (a1 != std::string::npos && a2 != std::string::npos &&
                k != std::string::npos && zu != std::string::npos && zu > k) {
                g_app->helpTagName[i] = v.substr(a1 + 1, a2 - a1 - 1);
                g_app->helpTagType[i] = ohneRand(v.substr(k + 1, zu - k - 1));
            }
            continue;
        }
        // random( VON, BIS )
        if (v.rfind("random(", 0) == 0) {
            float von = 0.0F;
            float bis = 0.0F;
            if (std::sscanf(v.c_str(), "random( %f , %f )", &von, &bis) == 2) {
                g_app->helpRangeLow[i] = von;
                g_app->helpRangeHigh[i] = bis;
            }
            continue;
        }
    }
}

// Die Helferzeilen EINER Spalte - Bilder 4 und 6 des Originals.
//
// Sie bauen die drei Ausdruecke, die ICARUS kennt. Die Signatur von get()
// steht in der behaved.bhc selbst, in einer auskommentierten Zeile:
//     //get( %t="DECLARE_TYPE", %s="variablename" );
//
// Abweichung vom Original, bewusst: dort hat die Tag-Zeile nur eine
// Klappliste, der Name kommt von woanders. Hier steht ein eigenes Feld
// dafuer - sonst kann man tag() ohne Umweg ueber die Tastatur nicht bauen.
// Die drei Ausdruecke der Helferknoepfe - Schreibweise wie das Original und
// wie Ravens 1510 Skripte (tag( "X", ORIGIN) 950-mal, get( FLOAT, "X")
// 245-mal, random( N, N ) 188-mal): kein Leerzeichen vor der schliessenden
// Klammer bei get und tag, Zahlen ohne angehaengte Nullen (%g).
//
// Bis rc568 schrieb behaved `get( FLOAT, "X" )` und `random( 0.000, 1.000 )`.
// Gueltig, aber anders als alles, was BehavEd je gespeichert hat.
std::string getAusdruck(std::size_t i) {
    return "get( " + g_app->helpGetType[i] + ", \"" + g_app->helpGetName[i] + "\")";
}
std::string tagAusdruck(std::size_t i) {
    return "tag( \"" + g_app->helpTagName[i] + "\", " + g_app->helpTagType[i] + ")";
}

void drawHelperRows(std::size_t i, std::string* target, const Param* nurGet) {
    // --- Die Spalte ist so breit wie IHR Feld ---------------------------
    //
    // Hier stand `GetFontSize() * 10` - eine erfundene Zahl. Zwei solche
    // Klapplisten plus Knopf sind breiter als das Feld darueber, und die
    // Bloecke der Nachbarspalten ueberlappten sich.
    //
    // Sichtbar auf shanks Bild: "SET_PAR FLOAT" - die abgeschnittene
    // Klappliste der ersten Spalte und die der zweiten uebereinander. Und
    // wer oben liegt, bekommt den Klick: "I can only change the first
    // parameter."
    //
    // Also aus der GEMESSENEN Feldbreite ableiten. Eine Zeile traegt zwei
    // Klapplisten und einen Knopf; der Knopf bekommt, was er braucht, der
    // Rest wird geteilt.
    // --- EINE Klappliste je Zeile, wie im Original ----------------------
    //
    // Der eigentliche Unterschied zu Bild 2 war nicht der Abstand, sondern
    // der AUFBAU: im Original steht in jeder Zeile EINE Klappliste ueber
    // die volle Spaltenbreite -
    //
    //     [FLOAT              v]
    //     [SET_PARM1          v] [Get]
    //     [ORIGIN             v] [Tag]
    //     [0.0] .. range .. [1.0] [Rnd]
    //
    // - waehrend ich zwei nebeneinander gequetscht habe. Deshalb war alles
    // eng und "SET_PA" abgeschnitten, obwohl die Spaltenbreite stimmte.
    //
    // Die Spalte darf ausserdem breiter sein als ihr Feld: die
    // Klapplisteneintraege sind laenger als die meisten Werte. Sie
    // bestimmt dann die Fensterbreite mit - genau das, worum shank
    // gebeten hat.
    // Die ZELLE gibt die Breite vor.
    //
    // Seit rc488 stehen die Bloecke in einer Tabelle. Deren Zelle kennt
    // ihre Breite genau - eine eigene Rechnung daneben waere wieder die
    // zweite Fassung, und genau daran ist die Ausrichtung schon zweimal
    // gescheitert.
    //
    // `GetContentRegionAvail` ist hier die einzige Wahrheit; `editorFeldW`
    // hat die Spalte bereits bestimmt, als die Tabelle angelegt wurde.
    const float spalte = ImGui::GetContentRegionAvail().x;
    const float knopfW =
        std::max({ImGui::CalcTextSize(tr(Str::EditorGet)).x,
                  ImGui::CalcTextSize(tr(Str::EditorTag)).x,
                  ImGui::CalcTextSize(tr(Str::EditorRnd)).x}) +
        ImGui::GetStyle().FramePadding.x * 2.0F;
    const float abstand = ImGui::GetStyle().ItemSpacing.x;
    // Breite einer Klappliste, die sich die Zeile mit einem Knopf teilt.
    const float w = std::max(ImGui::GetFontSize() * 4.0F,
                             spalte - knopfW - abstand);

    helferGross(i + 1U);
    ImGui::PushID(static_cast<int>(i));

    // Die Klapplisten schreiben SOFORT ins Feld - wenn dort schon ein
    // Ausdruck dieser Form steht.
    //
    // shank zu rc553: "wenn ich param 7 setze und OK druecke und das
    // Fenster oeffne steht wieder Param 1 drin."
    //
    // Sein Protokoll zeigt, dass der WERT sauber hin und zurueck laeuft:
    //
    //   Editor Ok  "use": [0] "get(STRING,"targetname")"
    //   Editor auf "use": [0] "get(STRING,"targetname")" Helfer=STRING/...
    //
    // Der Wert wurde also gespeichert. Nur eben der alte: die Klapplisten
    // fuetterten bisher NUR den Knopf. Wer den Namen umstellte und dann
    // "Ok" drueckte, hat das Feld nie angefasst - und beim naechsten
    // Oeffnen liest die Rueckgewinnung wieder den alten Namen heraus.
    //
    // Aus seiner Sicht: "die Helferzeilen werden nicht gespeichert." Aus
    // Sicht des Programms: sie wurden nie irgendwohin geschrieben.
    //
    // Jetzt bauen Typ und Name den Ausdruck neu, sobald man sie aendert -
    // aber NUR, wenn im Feld schon ein `get(` steht. Sonst wuerde ein von
    // Hand getippter Ausdruck beim blossen Anfassen einer Klappliste
    // ueberschrieben, und das waere schlimmer als das Ausgangsproblem.
    const bool istGet = target != nullptr && target->rfind("get(", 0) == 0;
    const bool istTag = target != nullptr && target->rfind("tag(", 0) == 0;
    const bool istRnd = target != nullptr && target->rfind("random(", 0) == 0;

    // --- "Helper": nur die Zeilen, die zum Typ des Feldes passen ---------
    //
    // "Only the get fields that match the type of the expected data will be
    // available. If the expected data type is a float you also have the
    // choice of using the random function" (Handbuch). Am Original
    // abgelesen (27.09., set mit SET_PARM1 / SET_HEALTH / SET_ORIGIN):
    //
    //     <str>          [SET_PARM1 v] [Get]
    //     <int>/<float>  [SET_PARM1 v] [Get]   [0.0] .. range .. [1.0] [Rnd]
    //     <vec>          [SET_PARM1 v] [Get]   [ORIGIN v] [Tag]
    //
    // Keine Typliste: der Typ von get() folgt aus dem Feld.
    bool zeigeTyp = true;
    bool zeigeTag = true;
    bool zeigeRnd = true;
    if (nurGet != nullptr) {
        const bool zahl =
            nurGet->kind == Param::Kind::Float || nurGet->kind == Param::Kind::Int;
        const bool vektor = nurGet->kind == Param::Kind::Vector;
        g_app->helpGetType[i] = zahl ? "FLOAT" : (vektor ? "VECTOR" : "STRING");
        zeigeTyp = false;
        zeigeTag = vektor;
        zeigeRnd = zahl;
    }

    // Zeile 1: der Typ, allein und ueber die volle Breite.
    ImGui::SetNextItemWidth(spalte);
    if (zeigeTyp && typeSetCombo("##htype", "DECLARE_TYPE", &g_app->helpGetType[i],
                     spalte) &&
        istGet) {
        *target = getAusdruck(i);
    }

    // Zeile 2: der Name, dahinter Get.
    ImGui::SetNextItemWidth(w);
    if (typeSetCombo("##hname", "SET_TYPES", &g_app->helpGetName[i], w) &&
        istGet) {
        *target = getAusdruck(i);
    }
    ImGui::SameLine(0.0F, abstand);
    if (ImGui::Button(tr(Str::EditorGet), ImVec2{knopfW, 0.0F})) {
        *target = getAusdruck(i);
    }

    if (zeigeTag) {
    // Zeile 3: Tag-Name, Tag-Art und der Knopf - in EINER Zeile.
    //
    // shank: "viele sind so aufgebaut, wo oben eine leere Spalte ist und
    // unten das Tag. Ich will es aber nebeneinander. Das sieht besser aus
    // und ist fuer alle besser."
    //
    // rc486 hatte beide auf eigene Zeilen gelegt, weil das Original in
    // jeder Zeile EINE Klappliste hat. Stimmt - aber dort kommt der
    // Tag-Name gar nicht vor, das Feld ist unsere Zutat. Zwei halbe Zeilen
    // dafuer aufzuwenden war meine Auslegung, nicht die des Originals.
    //
    //     [Tag-Name        ] [ORIGIN      v] [Tag]
    //
    // Die beiden teilen sich, was neben dem Knopf uebrig bleibt.
    const float halb = std::max(ImGui::GetFontSize() * 3.0F,
                                (spalte - knopfW - abstand * 2.0F) * 0.5F);
    ImGui::SetNextItemWidth(halb);
    {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s", g_app->helpTagName[i].c_str());
        if (ImGui::InputText("##htagname", buf, sizeof(buf))) {
            g_app->helpTagName[i] = buf;
            if (istTag) {
                *target = tagAusdruck(i);
            }
        }
    }
    ImGui::SameLine(0.0F, abstand);
    ImGui::SetNextItemWidth(halb);
    if (typeSetCombo("##htagtype", "TAG_TYPE", &g_app->helpTagType[i], halb) &&
        istTag) {
        *target = tagAusdruck(i);
    }
    ImGui::SameLine(0.0F, abstand);
    if (ImGui::Button(tr(Str::EditorTag), ImVec2{knopfW, 0.0F})) {
        *target = tagAusdruck(i);
    }

    }  // zeigeTag

    if (zeigeRnd) {
    // Zeile 5: der Bereich.
    const float textW = ImGui::CalcTextSize(tr(Str::EditorRange)).x;
    const float zahlW = std::max(ImGui::GetFontSize() * 3.0F,
                                 (spalte - knopfW - textW - abstand * 3.0F) *
                                     0.5F);
    ImGui::SetNextItemWidth(zahlW);
    if (ImGui::InputFloat("##hlow", &g_app->helpRangeLow[i], 0.0F, 0.0F,
                          "%.3f") &&
        istRnd) {
        char rb[96];
        std::snprintf(rb, sizeof(rb), "random( %g, %g )",
                      static_cast<double>(g_app->helpRangeLow[i]),
                      static_cast<double>(g_app->helpRangeHigh[i]));
        *target = rb;
    }
    ImGui::SameLine(0.0F, abstand);
    ImGui::TextUnformatted(tr(Str::EditorRange));
    ImGui::SameLine(0.0F, abstand);
    ImGui::SetNextItemWidth(zahlW);
    if (ImGui::InputFloat("##hhigh", &g_app->helpRangeHigh[i], 0.0F, 0.0F,
                          "%.3f") &&
        istRnd) {
        char rb[96];
        std::snprintf(rb, sizeof(rb), "random( %g, %g )",
                      static_cast<double>(g_app->helpRangeLow[i]),
                      static_cast<double>(g_app->helpRangeHigh[i]));
        *target = rb;
    }
    ImGui::SameLine(0.0F, abstand);
    if (ImGui::Button(tr(Str::EditorRnd), ImVec2{knopfW, 0.0F})) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "random( %g, %g )",
                      static_cast<double>(g_app->helpRangeLow[i]),
                      static_cast<double>(g_app->helpRangeHigh[i]));
        *target = buf;
    }
    }  // zeigeRnd
    // Die rechte Kante der Helferzeilen, gemessen - dieselbe Spalte wie
    // oben. Sind beide Zahlen gleich, stimmt die Ausrichtung.
    //
    if (i == 0 && nurGet == nullptr) {
        g_app->dbgHelferEnde = ImGui::GetItemRectMax().x;
        g_app->dbgKnopfW = knopfW;
    }
    ImGui::PopID();
}


// --- Dateifelder: "..." (Browse) und "Play" -------------------------------
//
// Ein Feld mit Dateifilter traegt in der .bhc bzw. den Kopfdateien Ravens
// Pfad dazu, in zwei Schreibweisen:
//
//     !!"W:\game\base\scripts\!!#*.txt"        SET_SPAWNSCRIPT u. a.
//     !!"W:\game\base\!!sound\voice\*.wav;*.mp3"   CHAN_VOICE
//
// Vor dem zweiten "!!" steht der Basisordner, dahinter (nach einem
// moeglichen "#") Unterordner und Maske. Raven hatte das Spiel auf W:\;
// gemeint ist der Teil hinter "\base\" - bei uns die Spielordner.
bool spew(const std::string& path, const std::string& text);

struct DateiFilter {
    std::string basisRel;     // "scripts\" oder ""
    std::string maskeOrdner;  // "sound\voice\" oder ""
    std::string maske;        // "*.wav;*.mp3"
};

std::string kleinSchreiben(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

DateiFilter zerlegeFilter(const std::string& f) {
    DateiFilter d;
    const std::size_t bang = f.find("!!");
    const std::string basis = (bang == std::string::npos) ? std::string() : f.substr(0, bang);
    std::string rest = (bang == std::string::npos) ? f : f.substr(bang + 2);
    if (!rest.empty() && rest[0] == '#') {
        rest.erase(0, 1);
    }
    const std::size_t b = kleinSchreiben(basis).find("\\base\\");
    d.basisRel = (b == std::string::npos) ? std::string() : basis.substr(b + 6);
    const std::size_t s = rest.find_last_of("\\/");
    if (s == std::string::npos) {
        d.maske = rest;
    } else {
        d.maskeOrdner = rest.substr(0, s + 1);
        d.maske = rest.substr(s + 1);
    }
    return d;
}

bool istSkriptFilter(const DateiFilter& d) {
    const std::string m = kleinSchreiben(d.maske);
    return m.find(".txt") != std::string::npos || m.find(".ibi") != std::string::npos;
}

// Wo die Dateien dieses Feldes liegen koennen, mit abschliessendem "\".
// Fuer Skripte zuerst der Skriptpfad aus den Einstellungen.
std::vector<std::string> dateiBasen(const DateiFilter& d) {
    std::vector<std::string> out;
    auto dazu = [&out](std::string p) {
        if (p.empty()) { return; }
        if (p.back() != '\\' && p.back() != '/') { p += '\\'; }
        out.push_back(p);
    };
    if (istSkriptFilter(d)) {
        dazu(g_app->settings.scriptPath);
    }
    for (const GamePath& gp : g_app->gamePaths) {
        std::string p = gp.directory;
        if (!p.empty() && p.back() != '\\' && p.back() != '/') { p += '\\'; }
        dazu(p + d.basisRel);
    }
    return out;
}

bool dateiDa(const std::string& p) {
    std::error_code ec;
    return !p.empty() && std::filesystem::exists(::bhed::utf8Pfad(p), ec);
}

// "..." - Datei waehlen und RELATIV eintragen.
//
// Das Original oeffnete dafuer einen Dateidialog ("Browse to file", "All
// files|*.*"); bei behaved tat der Knopf bisher nichts. Hier startet der
// Dialog im passenden Unterordner, bietet die Maske des Feldes an (und
// alle Dateien), und das Ergebnis steht so da, wie das Spiel es erwartet:
//
//     Klang   sound/voice/kyle/hello.mp3     relativ zum Spielordner
//     Skript  md_twj/s3                      relativ zu scripts\, ohne Endung
//
// Die Engine laedt Skripte als "scripts/<name>" plus .ibi - eine Endung
// im Feld waere falsch.
void durchsuchen(const Param& p, std::string& wert) {
    const DateiFilter d = zerlegeFilter(p.filter);
    const std::vector<std::string> basen = dateiBasen(d);
    std::string start;
    for (const std::string& b : basen) {
        if (dateiDa(b + d.maskeOrdner)) {
            start = b + d.maskeOrdner;
            break;
        }
    }
    if (start.empty() && !basen.empty()) { start = basen.front(); }
    std::string filter;
    if (!d.maske.empty() && d.maske != "*.*") {
        filter = d.maske + "|" + d.maske + "|";
    }
    filter += "All files (*.*)|*.*";
    const std::string wahl = platform::openFileDialog(tr(Str::BrowseTitle),
                                                      filter.c_str(), start);
    if (wahl.empty()) {
        return;
    }
    std::string rel;
    const std::string wahlKlein = kleinSchreiben(wahl);
    for (const std::string& b : basen) {
        const std::string bk = kleinSchreiben(b);
        if (wahlKlein.compare(0, bk.size(), bk) == 0) {
            rel = wahl.substr(b.size());
            break;
        }
    }
    if (rel.empty()) {
        // Ausserhalb der Spielordner: vom Unterordner der Maske an nehmen,
        // falls er im Pfad vorkommt ("...\sound\voice\x.mp3"), sonst den
        // ganzen Pfad - dann sieht man wenigstens, was gewaehlt wurde.
        const std::string marke = kleinSchreiben(
            "\\" + (d.maskeOrdner.empty() ? d.basisRel : d.maskeOrdner));
        const std::size_t at = (marke.size() > 1) ? wahlKlein.rfind(marke) : std::string::npos;
        rel = (at == std::string::npos) ? wahl : wahl.substr(at + 1);
    }
    for (char& ch : rel) {
        if (ch == '\\') { ch = '/'; }
    }
    if (istSkriptFilter(d)) {
        const std::size_t punkt = rel.find_last_of('.');
        const std::size_t strich = rel.find_last_of('/');
        if (punkt != std::string::npos &&
            (strich == std::string::npos || punkt > strich)) {
            rel.erase(punkt);
        }
    }
    diag::detail("Durchsuchen: \"" + wahl + "\" -> \"" + rel + "\"");
    wert = rel;
}

// "Play" - je nach Art des Feldes (Original: der "run"-Knopf).
//
//   Skript   in einer NEUEN behaved-Instanz oeffnen, wie das Original
//            ("start ... BehavEd.exe"). Nicht in einem Reiter dieses
//            Fensters: der Ereigniseditor bearbeitet gerade eine Zeile im
//            aktuellen Skript, und ein Reiterwechsel darunter liesse "Ok"
//            ins falsche Skript schreiben. Liegt es nur in einem Archiv, wird
//            es dafuer als Datei in %TEMP% abgelegt.
//   Video    (.roq, .bik) mit dem Programm, das Windows dafuer vorsieht.
//   Klang    wie bisher im eigenen Mischer, auch aus den Archiven - das
//            kann behaved besser als das Original.
void abspielen(const Param& p, const std::string& wertRoh) {
    const DateiFilter d = zerlegeFilter(p.filter);
    std::string wert = wertRoh;
    for (char& ch : wert) {
        if (ch == '/') { ch = '\\'; }
    }
    const std::string maske = kleinSchreiben(d.maske);
    const bool video = maske.find(".roq") != std::string::npos ||
                       maske.find(".bik") != std::string::npos;
    if (!istSkriptFilter(d) && !video) {
        playSound(wertRoh);
        return;
    }
    std::vector<std::string> kandidaten;
    for (const std::string& b : dateiBasen(d)) {
        if (istSkriptFilter(d)) {
            kandidaten.push_back(b + wert + ".txt");
            kandidaten.push_back(b + wert);
        } else {
            kandidaten.push_back(b + d.maskeOrdner + wert);
            kandidaten.push_back(b + wert);
        }
    }
    for (const std::string& k : kandidaten) {
        if (!dateiDa(k)) { continue; }
        diag::detail("Play: \"" + k + "\"");
        if (istSkriptFilter(d)) {
            if (!platform::startBehavedWith(k)) {
                addStatus("Unable to launch BehavEd on this machine!");
            }
        } else {
            platform::openWithDefaultApp(k);
        }
        return;
    }
    // Skripte liegen bei Movie Duels meist in den Archiven.
    if (istSkriptFilter(d)) {
        std::string imArchiv = "scripts/" + wertRoh + ".txt";
        for (char& ch : imArchiv) {
            if (ch == '\\') { ch = '/'; }
        }
        for (const GamePath& gp : g_app->gamePaths) {
            for (const Pk3& arc : gp.archives) {
                const Pk3Entry* e = arc.find(kleinSchreiben(imArchiv));
                if (e == nullptr) { e = arc.find(imArchiv); }
                std::string inhalt;
                if (e == nullptr || !readPk3File(arc, *e, inhalt)) { continue; }
                std::error_code ec;
                const std::filesystem::path ordner =
                    std::filesystem::temp_directory_path(ec) / "behaved";
                std::filesystem::create_directories(ordner, ec);
                std::string datei = wertRoh;
                for (char& ch : datei) {
                    if (ch == '/' || ch == '\\') { ch = '_'; }
                }
                // u8string() ist seit C++20 ein std::u8string.
                const std::u8string ziel8 = (ordner / (datei + ".txt")).u8string();
                const std::string ziel(ziel8.begin(), ziel8.end());
                if (spew(ziel, inhalt)) {
                    diag::detail("Play: \"" + imArchiv + "\" aus " + arc.path + " -> " + ziel);
                    if (!platform::startBehavedWith(ziel)) {
                        addStatus("Unable to launch BehavEd on this machine!");
                    }
                    return;
                }
            }
        }
    }
    char z[300];
    std::snprintf(z, sizeof(z), "Unable to find file \"%s\"", wertRoh.c_str());
    addStatus(z);
}

void drawEditor() {
    if (!g_app->editorOpen || g_app->editorCmd == nullptr) { return; }
    const Command& c = *g_app->editorCmd;

    // Der Titel nennt den Befehl: "Event editor - set ( variablename, value )"
    // statt immer nur "Event editor". Bei zehn offenen Fenstern hintereinander
    // ist das der Unterschied zwischen Wissen und Raten.
    //
    // "###editor" haengt die Kennung des Fensters fest. Ohne das leitet ImGui
    // sie aus dem Titel ab - und der aendert sich mit jedem Befehl, womit das
    // Popup als NEUES Fenster gaelte und sofort wieder zuginge.
    // Param traegt keinen Namen - die Beschriftung steht in der Signatur.
    // Die zeigt der Baum ohnehin, also nehmen wir sie hier auch: aus
    // "set ( <str>, <str> )" wird der Titel "Event editor - set".
    std::string title = std::string(tr(Str::EditorTitle)) + " - " + c.name;
    {
        const std::string kurz = descFor(c);
        if (!kurz.empty()) {
            title += "  (" + kurz + ")";
        }
    }
    title += "###editor";

    // --- Alles, was die BREITE bestimmt, steht VOR `Begin` fest ----------
    //
    // shank: "wenn ich if oeffne dann shaked der Bildschirm immer."
    //
    // Von rc543 bis rc567 wurde das Fenster aus dem VORIGEN Bild gemittelt:
    // Reihe messen, Versatz daraus, Fenster waechst, nachruecken, Breite
    // lernen. Jede Runde fing einen Fall und liess einen neuen offen. Der
    // Selbsttest (BHED_EDITORTEST) zeigt, warum, beim dreifeldrigen `if`:
    //
    //   Bild 2  verborgen   Reihe 1044  (Zellen noch gerechnet)
    //   Bild 3  1299 breit  Reihe 1160
    //   Bild 4  1414 breit  Reihe 1276  <- Versatz aus 1160, schiesst ueber
    //   Bild 5  1357 breit
    //   Bild 6  1299 breit
    //
    // Die Reihe war im Vorbild schmaler als jetzt, ihr Versatz schob sie
    // nach rechts, das Fenster wuchs, und Beschreibung und Knoepfe (gegen
    // die Fensterbreite gemittelt) liessen es halbierend zurueckschwingen.
    //
    // ImGuis Autor zu genau diesem Fall: bei AlwaysAutoResize ist die
    // Breite im ersten Bild undefiniert - "You can't really have both ...
    // set an explicit width via SetNextWindowSize(ImVec2(400,0))".
    //
    // Hier geht das vollstaendig: jede Spaltenbreite folgt aus Text und
    // Schrift, und die Tabelle setzt feste Spalten genau so, wie bestellt
    // (imgui_tables.cpp: `WidthAuto = InitStretchWeightOrWidth` bei
    // WidthFixed|NoResize, Spaltenabstand 2 * CellPadding.x bei
    // NoPadOuterX ohne Rahmen). Also wird die Fensterbreite AUSGERECHNET und
    // mit `ImGuiCond_Always` vorgegeben. Nichts im Fenster liest mehr die
    // Fensterbreite zurueck - es gibt keinen Kreis, der schwingen koennte.
    // Die Hoehe bleibt automatisch (0).
    const std::vector<Param> fields =
        editorFields(c, g_app->db, g_app->editorValues);
    g_app->editorValues.resize(fields.size());

    // Werte nachziehen, wenn sich eine Auswahl geaendert hat.
    //
    // Nur die Felder DAHINTER, und nur die, bei denen sich auch etwas
    // geaendert hat: wer von SET_PARM1 auf SET_PARM2 wechselt, hat weiter
    // ein Textfeld und soll seinen getippten Wert behalten. Wer auf
    // SET_HEALTH wechselt, bekommt ein Zahlenfeld - und dort waere
    // "DEFAULT" schlicht falsch.
    if (g_app->editorRefreshFrom >= 0) {
        const auto from = static_cast<std::size_t>(g_app->editorRefreshFrom);
        // "Neu auswerten" fasst AUCH das erste Feld an.
        //
        // Beim Nachziehen nach einer Auswahl ist `from + 1` richtig: wer
        // Feld 0 aendert, will die Felder DAHINTER neu haben, nicht sein
        // eigenes. Der Knopf setzt aber `editorRefreshFrom = 0` und meint
        // "alles" - und traf damit Feld 0 nie.
        //
        // shank zu rc542, mit Bild: nach "Re-Evaluate" stehen Feld 2 und 3
        // auf ihren Vorgaben, Feld 1 zeigt weiter `random( 0.000, 1.000 )`.
        const std::size_t ab = g_app->editorForceRefresh ? 0U : from + 1U;
        for (std::size_t k = ab; k < fields.size(); ++k) {
            // Die AUSWAHL selbst fasst "Re-Evaluate" nicht an.
            //
            // shank, 27.09., mit Bild: "If you click the Re-Evaluate button
            // it will reset the selected command to SET_PARM1. It should
            // only reset its parameters." - bei SET_WALKING sprang Feld 0
            // auf den ersten Eintrag der Liste.
            //
            // Das Original (Abgleich, alle Befehle): SET_WALKING, CHAN_VOICE,
            // STRING (declare), ENABLE und PLAY_ROFF bleiben stehen; neu
            // aufgebaut werden nur die Felder dahinter. Also: die fuehrende
            // Klappliste und jede Auswahl mit eigenen Feldern auslassen.
            if (g_app->editorForceRefresh && fields[k].kind == Param::Kind::TypeSet &&
                (feldKnopf(c, k) == FeldKnopf::Keiner ||
                 auswahlMitFeldern(g_app->db, fields[k].typeset))) {
                continue;
            }
            const bool kindChanged =
                k >= g_app->editorKinds.size() ||
                g_app->editorKinds[k] != static_cast<int>(fields[k].kind);
            const std::string want = defaultValueFor(fields[k], g_app->db);
            const bool defChanged =
                k >= g_app->editorDefs.size() || g_app->editorDefs[k] != want;
            if (kindChanged || defChanged || g_app->editorForceRefresh) {
                // defaultValueFor statt fields[k].def: in der .bhc steht bei
                // affect der TYPNAME "AFFECT_TYPE" als Platzhalter. Der rohe
                // Text landete so im Skript - richtig ist FLUSH, der erste
                // Eintrag der Typmenge.
                g_app->editorValues[k] = want;
            }
        }
        g_app->editorRefreshFrom = -1;
        g_app->editorForceRefresh = false;
    }
    // Fuer den naechsten Vergleich merken.
    g_app->editorKinds.assign(fields.size(), 0);
    g_app->editorDefs.assign(fields.size(), std::string{});
    for (std::size_t k = 0; k < fields.size(); ++k) {
        g_app->editorKinds[k] = static_cast<int>(fields[k].kind);
        g_app->editorDefs[k] = defaultValueFor(fields[k], g_app->db);
    }
    // Leere Felder bekommen ihre Vorgabe schon hier - ihre Breite zaehlt.
    for (std::size_t k = 0; k < fields.size(); ++k) {
        if (g_app->editorValues[k].empty()) {
            g_app->editorValues[k] = fields[k].def;
        }
    }

    // Befehlsname, rechtsbuendig bis 91 dlu - dort endet der STATIC aus
    // Dialog 137, und genau dort faengt das erste Feld an.
    std::string upper = c.name;
    for (char& ch : upper) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    const float labelW = dx(91);
    const float tw = ImGui::CalcTextSize(upper.c_str()).x;

    // Die Beschreibung: die des gewaehlten Eintrags, sonst die des Befehls.
    std::string help = descFor(c);
    if (!c.params.empty() && c.params[0].kind == Param::Kind::TypeSet &&
        !g_app->editorValues.empty()) {
        const TypeSet* ts = g_app->db.typeset(c.params[0].typeset);
        if (ts != nullptr) {
            const TypeEntry* e = ts->find(g_app->editorValues[0]);
            if (e != nullptr && !e->desc.empty()) { help = e->desc; }
        }
    }
    if (help.empty()) { help = tr(Str::EditorNoHelp); }

    const ImGuiStyle& edStil = ImGui::GetStyle();
    const float linksrand = edStil.WindowPadding.x;
    const float breiteHilfe =
        ImGui::CalcTextSize(("( " + help + " )").c_str()).x;
    const float btnW = std::max(dx(kEdOkW),
                                ImGui::CalcTextSize(tr(Str::EditorReEvaluate)).x +
                                    edStil.FramePadding.x * 2.0F);
    const float breiteKnoepfe = btnW * 3.0F + edStil.ItemSpacing.x * 2.0F;
    // Das Fenster muss mindestens so breit sein wie sein Titel plus
    // Schliessknopf - sonst schneidet ImGui den Titel ab, und der Rest
    // liest sich linksbuendig (shank zu `rem`).
    const float breiteTitel = ImGui::CalcTextSize(title.c_str()).x +
                              edStil.FramePadding.x * 2.0F +
                              ImGui::GetFontSize() * 2.5F +
                              edStil.WindowPadding.x * 2.0F;

    // --- EINE Spalteneinteilung fuer die Reihe UND die Helfer -----------
    //
    // Die Spaltenbreite ist das Groessere aus Feld und Helfer, und beide
    // setzen sich an dieselbe Stelle. `editorFeldX` ist hier noch relativ
    // zum Tabellenanfang; der Versatz kommt nach `Begin` dazu.
    //
    // Abgerundet, weil die Tabelle es auch tut (`WidthGiven =
    // ImTrunc(WidthRequest)`) - sonst verrechnete sich die Summe unten um
    // einen Punkt je Spalte.
    g_app->editorFeldX.assign(fields.size(), 0.0F);
    g_app->editorFeldW.assign(fields.size(), 0.0F);
    // Wie schmal eine Spalte werden darf, wenn die Reihe nicht ins Fenster
    // passt (siehe unten).
    // Zwei Stufen: erst bis auf den INHALT (so kurze Felder wie "0" zuerst
    // abgeben), dann bis auf die harte Grenze.
    std::vector<float> inhaltW(fields.size(), 0.0F);
    std::vector<float> mindestW(fields.size(), 0.0F);
    const float spaltenAbstand = edStil.CellPadding.x * 2.0F;
    // Breite einer Klappliste (Typmenge). Sie hat KEINEN Expr!-Knopf, und
    // `comboEdit` setzt seine Breite selbst - dieselbe Formel hier und beim
    // Zeichnen. Vorher reservierte die Spalte die Knopfbreite trotzdem: der
    // Selbsttest mass bei `affect` und `camera ENABLE` 63 Punkte Luft am
    // Ende der Reihe, und die Reihe stand um die Haelfte davon schief.
    auto auswahlBreite = [](const std::string& wert) {
        return std::max(dx(kEdFieldW), ImGui::CalcTextSize(wert.c_str()).x +
                                           ImGui::GetFrameHeight() * 2.0F);
    };
    float tabelleW = 0.0F;
    {
        const float knopfB =
            std::max({ImGui::CalcTextSize(tr(Str::EditorGet)).x,
                      ImGui::CalcTextSize(tr(Str::EditorTag)).x,
                      ImGui::CalcTextSize(tr(Str::EditorRnd)).x,
                      ImGui::CalcTextSize(tr(Str::EditorExpr)).x,
                      ImGui::CalcTextSize(tr(Str::EditorHelper)).x,
                      ImGui::CalcTextSize(tr(Str::EditorRevert)).x}) +
            edStil.FramePadding.x * 2.0F;
        for (std::size_t k = 0; k < fields.size(); ++k) {
            const float ausText =
                ImGui::CalcTextSize(g_app->editorValues[k].c_str()).x +
                ImGui::GetFrameHeight() * 1.5F;
            // Die 17 Zeichenhoehen fuer einen Helferblock zaehlen NUR, wenn
            // dieser Helfer auch offen ist - bei `loop` mit "-1" wurde die
            // Spalte sonst ohne Grund breit.
            const bool helferOffen =
                (k < g_app->editorIsExpr.size()) &&
                (g_app->editorIsExpr[k] != 0);
            // Felder mit Dateifilter (z.B. `sound`) haben ZWEI weitere
            // Knoepfe: Durchsuchen und Abspielen.
            const float zusatzB =
                !fields[k].filter.empty()
                    ? (ImGui::CalcTextSize(tr(Str::EditorBrowse)).x +
                       ImGui::CalcTextSize(tr(Str::EditorPlay)).x +
                       edStil.FramePadding.x * 4.0F)
                    : 0.0F;
            // Eine Klappliste ist nur dann eine Klappliste, wenn sie nicht
            // auf Ausdruck umgestellt ist - dann ist sie ein Textfeld.
            const bool alsListe =
                fields[k].kind == Param::Kind::TypeSet && !helferOffen;
            const bool mitKnopf = feldKnopf(c, k) != FeldKnopf::Keiner;
            const float breit = std::floor(
                alsListe
                    ? auswahlBreite(g_app->editorValues[k]) + (mitKnopf ? knopfB : 0.0F)
                    : std::max({dx(kEdFieldW) + knopfB + zusatzB,
                                ausText + knopfB + zusatzB,
                                helferOffen ? ImGui::GetFontSize() * 17.0F
                                            : 0.0F}));
            mindestW[k] = std::min(
                breit, alsListe ? ImGui::GetFrameHeight() + ImGui::GetFontSize() * 3.0F +
                                      (mitKnopf ? knopfB : 0.0F)
                                : ImGui::GetFontSize() * 4.0F + knopfB + zusatzB);
            inhaltW[k] = std::clamp(alsListe ? ImGui::CalcTextSize(g_app->editorValues[k].c_str()).x +
                                                   ImGui::GetFrameHeight() * 2.0F + (mitKnopf ? knopfB : 0.0F)
                                             : ausText + knopfB + zusatzB,
                                    mindestW[k], breit);
            if (k > 0) { tabelleW += spaltenAbstand; }
            g_app->editorFeldX[k] = tabelleW;
            g_app->editorFeldW[k] = breit;
            tabelleW += breit;
        }
    }
    // Die Reihe beginnt an der Beschriftung (rechtsbuendig in `labelW`) und
    // endet an der letzten Spalte. Die Tabelle beginnt bei `dx(kEdLeft)`.
    const float istX = labelW - tw;
    // --- Passt die Reihe nicht ins Hauptfenster: Felder schmaler ----------
    //
    // shank: "when viewing some commands with a lot of parameters like
    // camera pan, it doesn't fit in the window. Maybe the width could
    // auto-adjust to fit within the window width?" camera PAN hat vier
    // Felder, zwei davon mit Helper-Knopf - bei 150 % ragte das Fenster
    // links und rechts hinaus. Zuerst gibt jede Spalte ab, was sie ueber
    // ihrem Inhalt hat ("0" fast alles, ein Vektor wenig); reicht das nicht,
    // anteilig bis zur harten Grenze. Passt alles, bleibt es wie bisher.
    const float platz = ImGui::GetMainViewport()->WorkSize.x - ImGui::GetFontSize() * 2.0F;
    for (const std::vector<float>* grenze : {&inhaltW, &mindestW}) {
        const float reiheVorher = (dx(kEdLeft) - istX) + tabelleW + linksrand * 2.0F;
        float spiel = 0.0F;
        for (std::size_t k = 0; k < fields.size(); ++k) {
            spiel += std::max(0.0F, g_app->editorFeldW[k] - (*grenze)[k]);
        }
        if (reiheVorher <= platz || spiel <= 0.0F) {
            continue;
        }
        const float anteil = std::min(1.0F, (reiheVorher - platz) / spiel);
        tabelleW = 0.0F;
        for (std::size_t k = 0; k < fields.size(); ++k) {
            float& w = g_app->editorFeldW[k];
            w = std::floor(w - std::max(0.0F, w - (*grenze)[k]) * anteil);
            if (k > 0) { tabelleW += spaltenAbstand; }
            g_app->editorFeldX[k] = tabelleW;
            tabelleW += w;
        }
    }
    const float reiheW = (dx(kEdLeft) - istX) + tabelleW;
    // Titel und Hilfetext machen das Fenster nicht breiter als der Platz:
    // der Titel wird abgeschnitten, die Hilfe bricht um (unten).
    const float fensterW = std::ceil(std::max({std::min(std::max(breiteTitel, breiteHilfe + linksrand * 2.0F),
                                                        platz),
                                               reiheW + linksrand * 2.0F,
                                               breiteKnoepfe + linksrand * 2.0F}));

    ImGui::OpenPopup(title.c_str());
    ImGui::SetNextWindowSize(ImVec2{fensterW, 0.0F}, ImGuiCond_Always);

    // --- Die MITTE festhalten, jedes Bild ------------------------------
    //
    // Mit dem Drehpunkt 0.5/0.5 setzt ImGui das Fenster um einen Punkt
    // herum - und zwar NACH der Groessenrechnung desselben Bildes
    // (imgui.cpp, `window_pos_with_pivot`). Waechst es, waechst es also in
    // alle Richtungen gleich, im selben Bild, ohne Nachruecken.
    //
    // Gemerkt wird die Mitte, nicht eine Stelle. Wer das Fenster zieht,
    // verschiebt die Mitte: waehrend des Ziehens setzen wir nichts, danach
    // gilt die neue Mitte.
    ImGuiWindow* edFenster = ImGui::FindWindowByName("###editor");
    // `MovingWindow` ist das angeklickte Fenster - bei einem Klick in die
    // Klappliste ein Kindfenster. Gezogen wird aber immer das oberste.
    const bool wirdGezogen = edFenster != nullptr &&
                             GImGui->MovingWindow != nullptr &&
                             GImGui->MovingWindow->RootWindow == edFenster;
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        if (g_app->editorMitte.x < 0.0F) {
            g_app->editorMitte = ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F,
                                        vp->WorkPos.y + vp->WorkSize.y * 0.5F};
        }
        if (!wirdGezogen) {
            // Im sichtbaren Bereich halten, soweit es hineinpasst. Die Hoehe
            // ist die des vorigen Bildes - die einzige, die es gibt.
            const float hoehe = (edFenster != nullptr) ? edFenster->Size.y : 0.0F;
            ImVec2 m = g_app->editorMitte;
            if (fensterW < vp->WorkSize.x) {
                m.x = std::clamp(m.x, vp->WorkPos.x + fensterW * 0.5F,
                                 vp->WorkPos.x + vp->WorkSize.x - fensterW * 0.5F);
            }
            if (hoehe > 0.0F && hoehe < vp->WorkSize.y) {
                m.y = std::clamp(m.y, vp->WorkPos.y + hoehe * 0.5F,
                                 vp->WorkPos.y + vp->WorkSize.y - hoehe * 0.5F);
            }
            ImGui::SetNextWindowPos(m, ImGuiCond_Always, ImVec2{0.5F, 0.5F});
        }
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2{0.5F, 0.5F});
    const bool offen =
        ImGui::BeginPopupModal(title.c_str(), &g_app->editorOpen,
                               ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();
    if (!offen) {
        return;
    }
    if (wirdGezogen) {
        const ImVec2 o = ImGui::GetWindowPos();
        const ImVec2 gr = ImGui::GetWindowSize();
        g_app->editorMitte = ImVec2{o.x + gr.x * 0.5F, o.y + gr.y * 0.5F};
    }
    if (ImGui::IsWindowAppearing()) {
        const ImGuiViewport* vpl = ImGui::GetMainViewport();
        char ze[300];
        std::snprintf(ze, sizeof(ze),
                      "Ereignisfenster \"%s\" geoeffnet: Breite %.0f (Titel %.0f, "
                      "Reihe %.0f, Hilfe %.0f, Knoepfe %.0f), Mitte %.0f/%.0f, "
                      "Arbeitsflaeche %.0fx%.0f ab %.0f/%.0f",
                      c.name.c_str(), static_cast<double>(fensterW),
                      static_cast<double>(breiteTitel),
                      static_cast<double>(reiheW),
                      static_cast<double>(breiteHilfe),
                      static_cast<double>(breiteKnoepfe),
                      static_cast<double>(g_app->editorMitte.x),
                      static_cast<double>(g_app->editorMitte.y),
                      static_cast<double>(vpl->WorkSize.x),
                      static_cast<double>(vpl->WorkSize.y),
                      static_cast<double>(vpl->WorkPos.x),
                      static_cast<double>(vpl->WorkPos.y));
        diag::detail(ze);
    }

    // --- Die Reihe mittig ------------------------------------------------
    //
    // `sollX` ist die ZIELSTELLE der Beschriftung, `istX` die Stelle, an
    // der sie ohne Versatz stuende. Ist die Reihe das breiteste Element,
    // steht sie am Rand; sonst mittig im Fenster - wie Beschreibung und
    // Knopfleiste, gegen dieselbe, feste Breite.
    const float sollX = std::max(linksrand, (fensterW - reiheW) * 0.5F);
    const float versatz = sollX - istX;
    for (float& fx : g_app->editorFeldX) {
        fx += versatz + dx(kEdLeft);
    }

    ImGui::BeginGroup();   // die Reihe als Ganzes, um sie zu messen
    ImGui::SetCursorPosX(versatz + labelW - tw);
    // Senkrecht auf die Mitte des Feldes daneben.
    //
    // Gemeldet mit Bild: "Text isn't aligned vertically either." Ein
    // reiner Text ist so hoch wie die Schrift, ein Eingabefeld hat oben und
    // unten den Rahmenabstand dazu - nebeneinander gesetzt sitzt der Text
    // deshalb zu hoch.
    //
    // AlignTextToFramePadding schiebt ihn um genau diesen Abstand nach
    // unten; dafuer ist es da.
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(upper.c_str());
    // Wo die Reihe WIRKLICH beginnt - an der Beschriftung, nicht am
    // Gruppenanfang.
    //
    // Die Gruppe fasst alles zusammen, was seit BeginGroup gezeichnet
    // wurde, und ihr Kasten beginnt am Fensterrand. Maesse man von dort,
    // steckte der Versatz in der gemessenen Breite - und das Fenster
    // schaukelte sich Bild um Bild weiter auf.
    const float reiheAb = ImGui::GetItemRectMin().x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(versatz + dx(kEdLeft));
    // ANKER. Ohne ihn stuerzt `else` ab.
    //
    // shank: "wenn ich die else Zeile oeffnen will crasht das Programm."
    //
    //   IMGUI-ZUSICHERUNG: Code uses SetCursorPos() to extend
    //   window/parent boundaries. Please submit an item e.g. Dummy()
    //   afterwards.
    //
    // `else` hat KEINE Parameter. Die Feldschleife darunter laeuft also
    // nicht, und nach dem Cursorsprung kommt kein Element mehr - genau der
    // Fall, den ImGui abfaengt.
    //
    // Bei jedem anderen Befehl faellt es nicht auf, weil das erste Feld
    // sofort folgt. Das ist die Sorte Fehler, die nur der leere Fall zeigt.
    //
    // Er bleibt auch nach rc506 noetig: bei `else` und `flush` hat die
    // Tabelle NULL Spalten und wird gar nicht angelegt.
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
    // ... und auf derselben Zeile weiter, sonst begaenne die Tabelle eine
    // Zeile zu tief und die Beschriftung staende wieder allein darueber
    // (rc505).
    ImGui::SameLine(0.0F, 0.0F);

    // --- Feldreihe und Helfer in EINER Tabelle --------------------------
    //
    // shank: "loop ist nicht untereinander, affect ebenfalls nicht, run
    // dasselbe."
    //
    // Die Helferbloecke standen seit rc488 in einer Tabelle, die Feldreihe
    // darueber nicht - sie setzte ihre Spalten weiter von Hand. Zwei
    // Verfahren fuer dieselbe Einteilung, und genau deshalb sassen sie
    // nicht uebereinander.
    //
    // Ich habe das viermal nachgebessert (rc485 Breite, rc486 Nachruecken,
    // rc487 gemeinsame Einteilung, rc505 Anker). Jede Runde besser, keine
    // gelöst - weil beide Zeilen getrennt rechneten.
    //
    // Jetzt EINE Tabelle mit zwei Zeilen:
    //
    //     Zeile 1   [Feld] [Revert]  ·  <expr> ..... was <str>
    //     Zeile 2   FLOAT / SET_PARM1+Get / Tag-Zeile / Bereich
    //
    // Die Ausrichtung ist damit keine Rechnung mehr, sondern eine
    // Eigenschaft der Tabelle: dieselbe Spalte, dieselbe Kante.
    //
    // `editorFeldW` bleibt - es bestimmt die Spaltenbreiten. `editorFeldX`
    // wird nicht mehr zum Setzen gebraucht, aber weiter gefuellt, weil der
    // Debug-Auszug es zeigt.
    //
    // Die Aussenbreite ist die ausgerechnete Summe, nicht "der Rest des
    // Fensters": sonst haengt die Tabelle doch wieder an der Fensterbreite,
    // und im ersten Bild (Fenster noch schmal) klemmt ImGui die Spalten -
    // gemessen: Zelle 401 statt 408.
    const int edSpalten = static_cast<int>(fields.size());
    if (edSpalten > 0 &&
        !ImGui::BeginTable("##editorreihe", edSpalten,
                           ImGuiTableFlags_NoSavedSettings |
                               ImGuiTableFlags_NoPadOuterX,
                           ImVec2{tabelleW, 0.0F})) {
        ImGui::EndGroup();
        return;
    }
    if (edSpalten > 0) {
        for (int k = 0; k < edSpalten; ++k) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed |
                                            ImGuiTableColumnFlags_NoResize,
                                    g_app->editorFeldW[
                                        static_cast<std::size_t>(k)]);
        }
        ImGui::TableNextRow();
    }
    // Eingabe/Escape, WAEHREND ein Textfeld aktiv war - siehe unten beim
    // InputText.
    bool feldEnter = false;
    bool feldEscape = false;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        ImGui::TableNextColumn();
        const Param& p = fields[i];
        std::string& v = g_app->editorValues[i];
        if (v.empty()) { v = p.def; }
        ImGui::PushID(static_cast<int>(i));
        const float x = ImGui::GetCursorPosX();

        ImGui::BeginGroup();
        // Die Spaltenbreite steht schon fest; das Feld bekommt sie minus
        // seinem Knopf. Nicht neu ausrechnen - das war die zweite Fassung.
        const float knopfB =
            std::max({ImGui::CalcTextSize(tr(Str::EditorGet)).x,
                      ImGui::CalcTextSize(tr(Str::EditorTag)).x,
                      ImGui::CalcTextSize(tr(Str::EditorRnd)).x,
                      ImGui::CalcTextSize(tr(Str::EditorExpr)).x,
                      ImGui::CalcTextSize(tr(Str::EditorHelper)).x,
                      ImGui::CalcTextSize(tr(Str::EditorRevert)).x}) +
            ImGui::GetStyle().FramePadding.x * 2.0F;
        const FeldKnopf knopfArt = feldKnopf(c, i);
        const bool isExpr = (i < g_app->editorIsExpr.size() &&
                             g_app->editorIsExpr[i] != 0);
        // Die Zellbreite IST die bestellte Spaltenbreite: feste Spalten
        // ohne Groessenaenderung bekommen genau ihre Vorgabe (abgerundet wie
        // oben), und das Zellenpolster liegt bei NoPadOuterX AUSSERHALB
        // davon (imgui_tables.cpp, `WorkMinX`/`WorkMaxX`).
        //
        // Hier wurde gemessen (rc488), gedeckelt (rc551) und im
        // Einschwingen gerechnet (rc566) - drei Fassungen derselben Zahl, und
        // jede Messung hing an der Fensterbreite des vorigen Bildes. Seit die
        // Tabelle ihre Aussenbreite vorgegeben bekommt, sind Messung und
        // Rechnung gleich; die Rechnung braucht nur kein Vorbild.
        const float zelleW = (i < g_app->editorFeldW.size())
                                 ? g_app->editorFeldW[i]
                                 : ImGui::GetContentRegionAvail().x;
        if (i == 0) {
            // Gemessen, nur zum Vergleich im Protokoll.
            g_app->dbgZelle0 = ImGui::GetContentRegionAvail().x;
            g_app->dbgBestellt0 = zelleW;
            g_app->dbgKnopfB = knopfB;
        }
        // Auch hier die Zusatzknoepfe abziehen, sonst laufen sie ueber die
        // Zellkante.
        const float zusatzHier =
            p.filter.empty()
                ? 0.0F
                : (ImGui::CalcTextSize(tr(Str::EditorBrowse)).x +
                   ImGui::CalcTextSize(tr(Str::EditorPlay)).x +
                   ImGui::GetStyle().FramePadding.x * 4.0F);
        // KEIN ItemSpacing abziehen.
        //
        // Gefunden durch Nachrechnen, nicht durch Raten:
        //
        //   Feldzeile   feldBreite + SameLine(0, 0) + knopfB
        //   Helferzeile w + SameLine(0, abstand) + knopfW
        //
        // Die Helferzeile setzt den Abstand und zieht ihn ab - beides. Die
        // Feldzeile zog ihn ab und setzte ihn NICHT (`SameLine(0, 0)` in
        // Zeile 1017). Sie endete also genau um `ItemSpacing.x` zu frueh,
        // und das sind bei shanks 144 dpi die vierzehn Punkte, die er
        // gemessen hat.
        //
        // Abziehen ODER setzen - nicht eins von beidem. Hier gilt: nicht
        // abziehen, denn `SameLine(0, 0)` steht schon da und die
        // Zusatzknoepfe haengen ebenfalls ohne Abstand daran.
        // Zielnamen aus der Karte: bei SET_CAMERA_GROUP die Kameragruppen,
        // sonst die Zielnamen. Der Pfeil dafuer sitzt in derselben Zelle und
        // muss von der Feldbreite ab - sonst schob er "Expr!" ueber die
        // Zellkante hinaus.
        const std::vector<std::string>* kartenListe = nullptr;
        if (!g_app->map.empty() && p.kind == Param::Kind::String) {
            const bool wantsGroups =
                !g_app->editorValues.empty() &&
                g_app->editorValues[0] == "SET_CAMERA_GROUP";
            kartenListe = wantsGroups ? &g_app->map.cameraGroups
                                      : &g_app->map.targetNames;
            if (kartenListe->empty()) { kartenListe = nullptr; }
        }
        const float pfeilB =
            (kartenListe != nullptr) ? ImGui::GetFrameHeight() : 0.0F;
        const float feldBreite =
            std::max(ImGui::GetFontSize() * 4.0F,
                     zelleW - knopfB - zusatzHier - pfeilB);
        ImGui::SetNextItemWidth(feldBreite);
        // Der Knopf neben dem Feld - fuer beide Arten gleich, siehe
        // feldKnopf. Eine Klappliste auf Ausdruck wird zum Textfeld (wie
        // FLUSH bei affect im Original nach "Expr!").
        // Aufrufer pruefen `knopfArt != FeldKnopf::Keiner` selbst (kein
        // `return` hier: lint_endtable sieht die Tabelle drumherum).
        auto feldKnopfZeichnen = [&]() {
            // Rueckweg immer "Revert". Das Original zeigt "Reset!" nur,
            // solange ein Feld hinter der Auswahl noch NICHT ausgewertet ist
            // (SET_WALKING mit "true" als <str> direkt nach dem Oeffnen);
            // nach "Re-Evaluate" heisst er dort auch "Revert". Bei uns ist
            // jedes Feld schon beim Oeffnen ausgewertet (Klappliste true/
            // false) - es gibt diesen Zwischenzustand nicht.
            const Str beschriftung =
                isExpr ? Str::EditorRevert
                       : (knopfArt == FeldKnopf::Helfer ? Str::EditorHelper
                                                        : Str::EditorExpr);
            if (ImGui::Button(tr(beschriftung), ImVec2{knopfB, 0.0F})) {
                if (i >= g_app->editorIsExpr.size()) {
                    g_app->editorIsExpr.resize(fields.size(), 0);
                }
                if (isExpr) {
                    g_app->editorIsExpr[i] = 0;
                } else {
                    g_app->editorIsExpr[i] = 1;
                }
            }
        };
        if (p.kind == Param::Kind::TypeSet && !isExpr) {
            const TypeSet* ts = g_app->db.typeset(p.typeset);
            // Tippbar, nicht nur waehlbar: Movie Duels bringt eigene
            // Animationen mit, die in Ravens Kopfdateien nicht stehen.
            const ComboEditResult r =
                comboEdit("##v", v, ts != nullptr ? &ts->entries : nullptr,
                          std::min(auswahlBreite(v),
                                   zelleW - (knopfArt != FeldKnopf::Keiner ? knopfB : 0.0F)));
            // NUR bei einer echten Auswahl die Folgefelder zuruecksetzen.
            //
            // Bei jedem Tastendruck waere falsch: das Zuruecksetzen holt die
            // Vorgabewerte und ueberschreibt damit auch dieses Feld. Der
            // getippte Text waere sofort wieder weg - genau das machte das
            // Feld unbenutzbar.
            if (r.picked) {
                g_app->editorRefreshFrom = static_cast<int>(i);
            }
            if (knopfArt != FeldKnopf::Keiner) {
                ImGui::SameLine(0, 0);
                feldKnopfZeichnen();
            }
            // Ein Wert, den die Typmenge nicht kennt, ist erlaubt - Movie
            // Duels braucht das fuer seine eigenen Animationen. Aber ein
            // Hinweis gehoert daneben: viel haeufiger ist es ein Tippfehler.
            //
            // Derselbe Fall, den der Pruefer als V004 meldet; hier steht er
            // sofort da statt erst beim Pruefen.
            if (ts != nullptr && !v.empty() && ts->find(v) == nullptr) {
                ImGui::SameLine();
                ImGui::TextDisabled("%s", tr(Str::MsgUnknownValue));
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", tr(Str::MsgUnknownValueHint));
                }
            }
        } else {
            // Ohne feste Laenge: der Puffer waechst mit (CallbackResize).
            // Vorher 256 Byte - wer in einem langen print ein Zeichen
            // loeschte, bekam nur die ersten 254 ins Skript zurueck, und
            // anhaengen ging gar nicht (Code-Pruefung 03.10., Selbsttest
            // "pruefung").
            (void)ImGui::InputText("##v", v.data(), v.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
                                   textFeldWaechst, &v);
            // Eingabe bestaetigt und Escape bricht ab - auch wenn der
            // Schreibzeiger noch im Feld steht.
            //
            // Der Knopftest (rc568) tippte in `print` und drueckte Eingabe:
            // der Editor blieb offen, der Wert kam nie ins Skript. Das Feld
            // holt sich beide Tasten selbst (`Shortcut(ImGuiKey_Enter, ..,
            // id)` in InputTextEx), die Route gehoert dann ihm, und das
            // `Shortcut` des Editors weiter unten geht leer aus. Man musste
            // zweimal druecken - ein Windows-Dialog und das Original
            // schliessen beim ersten Mal.
            //
            // Das Feld wird in genau dem Bild verlassen, in dem die Taste
            // kommt. `IsKeyPressed` fragt die Taste selbst, nicht die Route.
            if (ImGui::IsItemDeactivated()) {
                if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                    ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
                    feldEnter = true;
                } else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
                    feldEscape = true;
                }
            }
            ImGui::SameLine(0, 0);
            if (!p.filter.empty()) {
                if (ImGui::Button(tr(Str::EditorBrowse))) { durchsuchen(p, v); }
                ImGui::SameLine(0, 0);
                if (ImGui::Button(tr(Str::EditorPlay))) { abspielen(p, v); }
                ImGui::SameLine(0, 0);
            }
            // Zielnamen aus der Karte anbieten. Bei SET_CAMERA_GROUP sind
            // es die Kameragruppen - dieselbe Herkunft, andere Spalte.
            if (kartenListe != nullptr) {
                const std::vector<std::string>& list = *kartenListe;
                if (ImGui::ArrowButton("##map", ImGuiDir_Down)) {
                    ImGui::OpenPopup("mappick");
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", tr(Str::MapPick));
                }
                if (ImGui::BeginPopup("mappick")) {
                    for (const std::string& t : list) {
                        if (ImGui::Selectable(t.c_str())) {
                            v = t;
                        }
                    }
                    ImGui::EndPopup();
                }
                ImGui::SameLine(0, 0);
            }
            // FESTE Breite, dieselbe wie in den Helferzeilen.
            //
            // shank: "die sind einen Ticken zu lang, dass nicht alles sauber
            // untereinander ist."
            //
            // Die Helferknoepfe (Get, Tag, Rnd) werden mit `ImVec2{knopfW,
            // 0}` gezeichnet und enden damit genau an der Spaltenkante.
            // Dieser hier nahm seine EIGENE Textbreite - "Revert" ist
            // schmaler als das Maximum, also endete die Feldzeile ein paar
            // Punkte frueher als die Helferzeilen darunter.
            //
            // `knopfB` ist bereits das Maximum ueber Get, Tag, Rnd, Expr!
            // und Revert - dieselbe Zahl, aus der auch die Feldbreite
            // folgt. Sie hier auch zu BENUTZEN schliesst die Luecke.
            // `editorIsExpr[i]` ist seit rc465 die einzige Wahrheit.
            // "Helper" stand bis rc568 nur an Dateifeldern; das Original
            // setzt ihn an JEDES Feld hinter einer fuehrenden Klappliste
            // (declare, set, sound, play) - siehe feldKnopf.
            if (knopfArt != FeldKnopf::Keiner) { feldKnopfZeichnen(); }
            // Die rechte Kante der Feldzeile, gemessen. Nur Spalte 0 -
            // mehr braucht der Vergleich nicht.
            if (i == 0) {
                g_app->dbgFeldEnde = ImGui::GetItemRectMax().x;
            }
        }
        // Typangabe unter dem Feld, mittig - so steht es im Original.
        // Ist das Feld auf Ausdruck umgestellt, zeigt es zusaetzlich, was es
        // vorher war: "<expr> ..... was <str>".
        std::string hint = hintFor(p);
        // Ein Textfeld der Signatur, in dem im Skript eine Zahl oder ein
        // Vektor steht, zeigt dessen Art - wie das Original bei
        // set ( "SET_HEALTH", 5 ) ohne Markierung: "<int>" unter der 5,
        // "<vec>" unter < 1 2 3 >. Gespeichert wird ohnehin in dieser Art
        // (argForParam uebernimmt sie vom alten Argument).
        if (p.kind == Param::Kind::String && !g_app->editorInsert) {
            const Node* alt = nodeAt(g_app->doc.script(), g_app->editorPath);
            if (alt != nullptr && i < alt->args.size()) {
                const Arg& a = alt->args[i];
                if (a.kind == Arg::Kind::Vector) {
                    hint = "<vec>";
                } else if (a.kind == Arg::Kind::Number) {
                    hint = a.text.find('.') == std::string::npos ? "<int>" : "<float>";
                }
            }
        }
        if (i < g_app->editorIsExpr.size() && g_app->editorIsExpr[i] != 0) {
            hint = std::string("<expr> ") + tr(Str::EditorWas) + " " + hint;
        }
        const float hw = ImGui::CalcTextSize(hint.c_str()).x;
        ImGui::SetCursorPosX(x + (std::min(dx(kEdFieldW), zelleW) - hw) * 0.5F);
        ImGui::TextDisabled("%s", hint.c_str());
        ImGui::EndGroup();

        if (i + 1 < fields.size()) {
            ImGui::SameLine(0, dx(kEdGap));
        } else {
            // Nach dem LETZTEN Feld noch etwas Luft nach rechts.
            //
            // Gewuenscht: "maybe adjust the width of each box slightly, also
            // have small padding on the right." Ohne das endet die Reihe
            // buendig an der Fensterkante, und es sieht abgeschnitten aus -
            // links steht die Beschriftungsspalte, rechts nichts.
            //
            // Der Abstand zwischen den Spalten kommt jetzt von der
            // Tabelle - das leere Element dafuer ist ueberfluessig.
        }
        ImGui::PopID();
    }
    ImGui::EndGroup();
    // Jetzt ist die Reihe gezeichnet und laesst sich messen - NUR zum
    // Vergleich mit der Rechnung, nichts liest den Wert zurueck. Weicht
    // beides ab, steht die Reihe schief, aber sie schwingt nicht.
    g_app->editorRowW = ImGui::GetItemRectMax().x - reiheAb;
    {
        static float letzteAbweichung = 0.0F;
        const float abw = g_app->editorRowW - reiheW;
        if (std::fabs(abw - letzteAbweichung) > 0.5F) {
            letzteAbweichung = abw;
            char zr[200];
            std::snprintf(zr, sizeof(zr),
                          "Ereigniseditor Reihe \"%s\": gerechnet %.0f, "
                          "gemessen %.0f, Abweichung %.0f",
                          c.name.c_str(), static_cast<double>(reiheW),
                          static_cast<double>(g_app->editorRowW),
                          static_cast<double>(abw));
            diag::detail(zr);
        }
    }

    // --- Zweite ZEILE derselben Tabelle: die Helfer ---------------------
    //
    // Vorher war das eine eigene Tabelle (rc488) unter einer von Hand
    // gesetzten Feldreihe. Zwei Tabellen koennen sich nicht ausrichten;
    // zwei ZEILEN einer Tabelle koennen gar nicht anders.
    if (edSpalten > 0) {
        bool welche = false;
        for (std::size_t i = 0; i < g_app->editorIsExpr.size(); ++i) {
            if (g_app->editorIsExpr[i] != 0) { welche = true; break; }
        }
        if (welche) {
            ImGui::TableNextRow();
            for (int k = 0; k < edSpalten; ++k) {
                ImGui::TableNextColumn();
                const std::size_t idx = static_cast<std::size_t>(k);
                if (idx >= g_app->editorIsExpr.size() ||
                    idx >= g_app->editorValues.size() ||
                    g_app->editorIsExpr[idx] == 0) {
                    continue;
                }
                drawHelperRows(idx, &g_app->editorValues[idx],
                               feldKnopf(c, idx) == FeldKnopf::Helfer ? &fields[idx]
                                                                      : nullptr);
            }
        }
        ImGui::EndTable();

        // --- Die Kanten ins Protokoll ---------------------------------
        //
        // Hier, NICHT in `drawHelperRows`: dort lief die Zeile nur, wenn
        // ausgerechnet Spalte 0 aufgeklappt war. shanks Protokoll hatte
        // deshalb null Editorzeilen - er hatte den Helfer der zweiten
        // Spalte geoeffnet.
        //
        // An dieser Stelle stehen beide Kanten fest, egal welche Spalte
        // offen ist.
        {
            const float diff = g_app->dbgHelferEnde - g_app->dbgFeldEnde;
            if (g_app->dbgHelferEnde > 0.0F &&
                std::abs(diff - g_app->dbgLetzteDiff) > 0.5F) {
                g_app->dbgLetzteDiff = diff;
                char zed[220];
                std::snprintf(zed, sizeof(zed),
                              "Ereigniseditor Spalte 0: Helfer %d/%d, "
                              "Zelle %.0f, "
                              "bestellt %.0f, knopfB "
                              "%.0f, knopfW %.0f, FeldEnde %.0f, HelferEnde "
                              "%.0f, Differenz %.0f",
                              // Steht der Ausdruckshelfer offen, und wie
                              // viele Felder kennt der Editor gerade?
                              //
                              // shanks rc566-Protokoll, einfeldriges `if`,
                              // EIN Oeffnen:
                              //
                              //   Zelle 408  bestellt 408   eingeschwungen
                              //   Zelle 313  bestellt 314   <- springt
                              //   gewachsen 459x415 -> 412x263
                              //
                              // 408 ist `Zeichenhoehe * 17` - der Beitrag
                              // des Helferblocks. 415-263 = 152 sind seine
                              // vier Zeilen. Er verschwindet also MITTEN im
                              // Oeffnen, und mit ihm die bestellte Breite.
                              //
                              // Warum, sagt keine Zahl im Protokoll:
                              // `editorIsExpr` wird nur vom Expr!/Revert-
                              // Knopf gesetzt, und der wurde nicht
                              // gedrueckt. Diese zwei Zahlen sagen es.
                              (!g_app->editorIsExpr.empty() &&
                               g_app->editorIsExpr[0] != 0) ? 1 : 0,
                              static_cast<int>(g_app->editorIsExpr.size()),
                              static_cast<double>(g_app->dbgZelle0),
                              static_cast<double>(g_app->dbgBestellt0),
                              static_cast<double>(g_app->dbgKnopfB),
                              static_cast<double>(g_app->dbgKnopfW),
                              static_cast<double>(g_app->dbgFeldEnde),
                              static_cast<double>(g_app->dbgHelferEnde),
                              static_cast<double>(diff));
                diag::detail(zed);
            }
        }
    }

    // Hilfetext: die Beschreibung des gewaehlten Eintrags, sonst die des
    // Befehls. Beides steht als '#'-Kommentar in .bhc bzw. Kopfdatei.
    ImGui::Dummy(ImVec2{0, dy(10)});
    // Ohne den willkuerlichen Versatz von 8 Punkten, der hier stand: er zog
    // die Zeile nach links, ohne dass etwas dafuer sprach. Mittig heisst
    // mittig, und die Untergrenze ist der Fensterrand - dieselbe Regel wie
    // bei der Knopfzeile.
    //
    // Der Text traegt seine eigenen Klammern; die zaehlen bei CalcTextSize
    // nicht mit, weil sie erst im Format dazukommen. Deshalb wird die
    // Breite MIT Klammern gemessen.
    //
    // Gemittelt gegen `fensterW` - dieselbe ausgerechnete Breite wie die
    // Reihe, nicht `GetWindowWidth()` des laufenden Fensters.
    if (breiteHilfe + linksrand * 2.0F <= fensterW) {
        ImGui::SetCursorPosX(std::max((fensterW - breiteHilfe) * 0.5F, linksrand));
        ImGui::Text("( %s )", help.c_str());
    } else {
        // Breiter als das Fenster (siehe `platz`): umbrechen statt abschneiden.
        ImGui::SetCursorPosX(linksrand);
        ImGui::PushTextWrapPos(fensterW - linksrand);
        ImGui::TextWrapped("( %s )", help.c_str());
        ImGui::PopTextWrapPos();
    }

    // Kein fester Abstand mehr. Die erste Fassung setzte hier
    // dy(kEdBtnRow - 40) - ein Mass aus Dialog 137, das fuer MS Sans Serif
    // 8pt gilt. Mit unserer groesseren Schrift schob es die Knopfzeile aus
    // dem Fenster, und man sah von "Ok" nur noch die obere Haelfte.
    ImGui::Dummy(ImVec2{0, ImGui::GetTextLineHeight()});
    ImGui::Separator();
    ImGui::Dummy(ImVec2{0, ImGui::GetStyle().ItemSpacing.y});

    // Knopfzeile mittig unter den Feldern, wie im Original.
    // Eingabe bestaetigt, Escape bricht ab - wie in jedem Dialog.
    //
    // Die Tasten werden VOR der Cursorverschiebung geprueft, und Escape
    // verlaesst die Funktion NICHT vorzeitig.
    //
    // Der erste Anlauf tat beides falsch: erst SetCursorPosX, dann bei
    // Escape sofort EndPopup und return. Damit war der Cursor verschoben,
    // ohne dass ein Element abgeschickt wurde - und ImGui bricht mit einer
    // Zusicherung ab:
    //
    //     Code uses SetCursorPos() to extend window/parent boundaries.
    //     Please submit an item e.g. Dummy() afterwards.
    //
    // Das Fenster wird jetzt regulaer fertig gezeichnet; geschlossen wird
    // erst am Ende.
    // Shortcut() statt IsKeyPressed().
    //
    // Der Unterschied steht in imgui.h: IsKeyPressed hat KEINE Nebenwirkung,
    // Shortcut meldet eine Route an - "several callers may register interest
    // in a shortcut, and only one owner gets it".
    //
    // Das loest genau das Problem, das ich vorher mit einem Flag umgangen
    // habe: der Editor schloss mit Eingabe, und handleTreeKeys sah dieselbe
    // Taste im selben Bild und oeffnete ihn wieder. Mit einer Route bekommt
    // das Fenster mit dem Fokus die Taste, und der Baum sieht sie nicht.
    const bool enterPressed = ImGui::Shortcut(ImGuiKey_Enter) ||
                              ImGui::Shortcut(ImGuiKey_KeypadEnter) ||
                              feldEnter;
    const bool escPressed = ImGui::Shortcut(ImGuiKey_Escape) || feldEscape;

    // Untergrenze ist der FENSTERRAND, nicht die Beschriftungsspalte.
    //
    // Vorher stand hier dx(kEdLeft) - der Abstand, an dem die Eingabefelder
    // beginnen. Bei einem schmalen Fenster (print, use, kill, free, signal:
    // ein einziges Feld) liegt die mittige Stelle DAVOR, und die Grenze
    // schob die Knopfzeile genau dorthin, wo das Feld anfaengt. Deshalb sass
    // sie rechts der Mitte - und zwar umso weiter, je schmaler das Fenster.
    //
    // Bei breiten Befehlen wie camera(PAN) fiel es nicht auf, weil die
    // Mitte dort ohnehin rechts von der Beschriftungsspalte liegt.
    //
    // Der Rand ist die richtige Grenze: er sagt nur "nicht aus dem Fenster
    // heraus" und mischt sich sonst nicht ein.
    ImGui::SetCursorPosX(std::max((fensterW - breiteKnoepfe) * 0.5F, linksrand));

    if (ImGui::Button(tr(Str::EditorOk), ImVec2{btnW, 0.0F}) || enterPressed) {
        Node n;
        n.kind = Node::Kind::Command;
        n.name = c.name;
        n.hasBlock = c.block;
        for (std::size_t i = 0; i < fields.size() && i < g_app->editorValues.size(); ++i) {
            if (i < g_app->editorIsExpr.size() && g_app->editorIsExpr[i] != 0) {
                Arg a;
                a.kind = Arg::Kind::Expr;
                a.text = g_app->editorValues[i];
                n.args.push_back(a);
                continue;
            }
            // Das bisherige Argument mitgeben, sonst schreibt der Editor
            // Schreibweisen um, die der Nutzer gar nicht angefasst hat -
            // genau das, was tests/edittour.cpp fuer den Kern ausschliesst.
            const Node* old = g_app->editorInsert
                                  ? nullptr
                                  : nodeAt(g_app->doc.script(), g_app->editorPath);
            // Das alte Argument NUR mitgeben, wenn der Ausdrucksmodus
            // unveraendert ist.
            //
            // `argForParam` liefert bei gleichem Text das alte Argument
            // unveraendert zurueck - samt seiner Art. Das ist richtig und
            // Absicht: ein Editor darf beim blossen Oeffnen und Schliessen
            // nichts umschreiben.
            //
            // Ein Druck auf "Revert" IST aber ein Anfassen. Der Text
            // bleibt dabei derselbe, also kam das alte Argument mit
            // `Kind::Expr` zurueck, und beim naechsten Oeffnen stand wieder
            // "Expr!" da.
            //
            // shank zu rc551: "if I click the 'Expr!' button and then the
            // 'Revert' button, it works as expected. But if I close the
            // window and open it again, the 'Expr!' button is active
            // despite clicking 'Revert' before."
            //
            // Hat er den Modus umgestellt, wird das Argument neu gebaut.
            // Sonst bleibt alles beim Alten.
            const bool modusGeaendert =
                i < g_app->editorWarExpr.size() &&
                i < g_app->editorIsExpr.size() &&
                g_app->editorWarExpr[i] != g_app->editorIsExpr[i];
            const Arg* prev = (old != nullptr && i < old->args.size() &&
                               !modusGeaendert)
                                  ? &old->args[i]
                                  : nullptr;
            n.args.push_back(argForParam(fields[i], g_app->editorValues[i],
                                         g_app->db, prev));
        }
        if (g_app->editorInsert) {
            if (g_app->doc.insertAfter(g_app->selectedPath, n)) {
                if (!g_app->selectedPath.empty()) {
                    ++g_app->selectedPath.back();   // Auswahl auf das Neue
                }
                rebuildTree();
            }
        } else if (g_app->doc.replaceAt(g_app->editorPath, n)) {
            rebuildTree();
        }
        // Und was er GESCHRIEBEN hat. Zusammen mit der Zeile beim Oeffnen
        // laesst sich damit sagen, ob ein Wert unterwegs verlorengeht.
        {
            std::string z = "Editor Ok \"" + n.name + "\":";
            for (std::size_t i = 0; i < n.args.size(); ++i) {
                z += " [" + std::to_string(i) + "] \"" + n.args[i].text +
                     "\"" +
                     (n.args[i].kind == Arg::Kind::Expr ? " (Ausdruck)" : "");
            }
            diag::detail(z);
        }
        helferInEinstellungen(c.name);
        g_app->editorOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EditorCancel), ImVec2{btnW, 0.0F}) || escPressed) {
        // Auch beim Abbrechen merken.
        //
        // Die Helferzeilen aendern das Skript nicht - es gibt also nichts
        // abzubrechen. Wer sie einstellt und dann doch Abbrechen drueckt,
        // soll seine Wahl beim naechsten Oeffnen trotzdem vorfinden.
        helferInEinstellungen(c.name);
        g_app->editorOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    // "Neu auswerten": alle Felder hinter der ersten Auswahl auf die
    // Vorgabewerte zuruecksetzen, die zur aktuellen Wahl gehoeren. Vorher
    // war der Knopf dauerhaft ausgegraut - genau das, was er tun soll,
    // passierte nirgends.
    if (ImGui::Button(tr(Str::EditorReEvaluate), ImVec2{btnW, 0.0F})) {
        g_app->editorRefreshFrom = 0;
        g_app->editorForceRefresh = true;
        // Auch den Ausdrucksmodus und die Helferzeilen zuruecknehmen.
        //
        // rc544 setzte nur die WERTE zurueck. Auf shanks Bild steht danach
        // weiter "Revert" am Feld, und darunter FLOAT / SET_PARM1 /
        // ORIGIN / 0.000 .. 1.000 - der Befehl sieht also unveraendert
        // aus, obwohl der Wert zurueckgesetzt wurde.
        //
        // ABER den Ausdrucksmodus stehen lassen.
        //
        // rc545 hat ihn mit zurueckgenommen ("wie frisch eingefuegt"). Das
        // war zu viel: damit klappen die Helferzeilen zu, und wer sie
        // gerade benutzt, verliert seine Ansicht.
        //
        // shank zu rc545: "wenn ich re evaluate druecke springt das fenster
        // wieder zurueck in die form obwohl es vorher aufgeklappt war, das
        // sollte nicht passieren - es sollte die Werte zuruecksetzen sich
        // aber nicht zuklappen."
        //
        // Also nur die WERTE: das Feld selbst (oben) und die Helferzeilen
        // darunter. `helferGross` fuellt gleich wieder mit den Vorgaben
        // FLOAT / SET_PARM1 / ORIGIN / 0.0 .. 1.0 nach - sichtbar, offen,
        // zurueckgesetzt.
        g_app->helpGetType.clear();
        g_app->helpGetName.clear();
        g_app->helpTagName.clear();
        g_app->helpTagType.clear();
        g_app->helpRangeLow.clear();
        g_app->helpRangeHigh.clear();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tr(Str::EditorReEvalHint));
    }

    // rc601 tour (Review): das Tutorial-Fenster im Begin-Stapel dieses
    // Modals zeichnen - sonst nimmt es bei offenem Editor keine Klicks an
    // (ImGui sperrt die Maus fuer Fenster ausserhalb, gui/tour.cpp).
    tourImModal();

    ImGui::EndPopup();
}

// --- Suchen (Dialog 140 des Originals) ---------------------------------

// Die Suche - ein Fenster, das OFFEN BLEIBT.
//
// Vorher war es ein modaler Dialog, der beim ersten Treffer zuging. Das ist
// genau das, was beim Suchen stoert: man will den zweiten und dritten
// Treffer sehen, ohne das Fenster jedes Mal neu zu oeffnen.
//
// Notepad++ macht es so, und daran halte ich mich: das Fenster bleibt
// stehen, hat "Weiter" und "Zurueck", und F3 / Umschalt+F3 tun dasselbe.
// Auch der Rest ist von dort uebernommen - Ctrl+F setzt den Schreibzeiger
// zurueck ins Suchfeld, wenn das Fenster schon offen ist, und eine
// erfolglose Suche wird angezeigt statt verschluckt.
// "Find similar": das Suchfenster oeffnen und mit Name und Wert fuellen.
void sucheVorbelegen(const std::string& name, const std::string& text) {
    g_app->findVorName = name;
    g_app->findVorText = text;
    g_app->findVorbelegen = true;
    fensterZeigen(g_app->findOpen, "###find");
}

void drawFind() {
    if (!g_app->findOpen) {
        return;
    }
    // KEIN modaler Dialog mehr: waehrend gesucht wird, soll man im Baum
    // weiterarbeiten koennen.
    ImGui::SetNextWindowSize(ImVec2{dx(230), 0.0F}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(tr(Str::ActFind)) + "###find";
    if (!ImGui::Begin(titel.c_str(), &g_app->findOpen,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }
    static char named[128];
    static char contains[128];
    static char ersatz[128];
    // Der Selbsttest setzt den Ersatztext von aussen.
    if (!g_app->findErsatz.empty() && g_app->findErsatz != ersatz) {
        std::snprintf(ersatz, sizeof(ersatz), "%s", g_app->findErsatz.c_str());
    }
    // "Find similar": die Angaben des Befehls eintragen und gleich zaehlen,
    // damit "k of N" sofort dasteht und F3 weitersucht.
    if (g_app->findVorbelegen) {
        g_app->findVorbelegen = false;
        std::snprintf(named, sizeof(named), "%s", g_app->findVorName.c_str());
        std::snprintf(contains, sizeof(contains), "%s", g_app->findVorText.c_str());
        g_app->findWhole = true;
        g_app->findLetzte.named = named;
        g_app->findLetzte.contains = contains;
        g_app->findLetzte.wholeString = true;
        g_app->findHits = findAll(g_app->doc.script(), g_app->findLetzte);
        g_app->findAt = 0;
        for (std::size_t h = 0; h < g_app->findHits.size(); ++h) {
            if (g_app->findHits[h] == g_app->selectedPath) {
                g_app->findAt = static_cast<int>(h);
                break;
            }
        }
    }

    // Ctrl+F bei offenem Fenster: zurueck ins Suchfeld.
    const bool nochmalGeoeffnet =
        ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F);
    if (ImGui::IsWindowAppearing() || nochmalGeoeffnet) {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(dx(160));
    const bool enterInName = ImGui::InputText(
        tr(Str::FindNamed), named, sizeof(named),
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SetNextItemWidth(dx(160));
    const bool enterInText = ImGui::InputText(
        tr(Str::FindContains), contains, sizeof(contains),
        ImGuiInputTextFlags_EnterReturnsTrue);
    // --- Ersetzen, wie Notepad++ ----------------------------------------
    //
    // shank: "in the find window, let us replace the item containing text.
    // Would be useful for cameras, as some are reused throughout the
    // cutscene". Ersetzt wird genau, was "Item containing" findet.
    ImGui::SetNextItemWidth(dx(160));
    if (ImGui::InputText(tr(Str::FindReplaceWith), ersatz, sizeof(ersatz))) {
        g_app->findErsatz = ersatz;
    }
    ImGui::Checkbox(tr(Str::FindWhole), &g_app->findWhole);
    ImGui::SameLine();
    ImGui::Checkbox(tr(Str::FindInSelection), &g_app->findInSelection);

    // Neu suchen - wenn sich die Eingabe geaendert hat oder man es verlangt.
    // Die Angaben aus dem Fenster uebernehmen - gesucht wird in findNext.
    auto angabenMerken = [&]() {
        g_app->findLetzte.named = named;
        g_app->findLetzte.contains = contains;
        g_app->findLetzte.wholeString = g_app->findWhole;
    };

    // Hoehe 0: die Hoehe, die zum Text passt.
    //
    // dy(kEdFieldH) ist fuer EINGABEFELDER gedacht, wo sie zur Zeilenhoehe
    // stimmen muss. An einem Knopf faellt sie bei 144 dpi zu hoch aus - im
    // Suchfenster faellt es nicht auf, weil man es ziehen kann, im
    // Nachfragedialog schnitt sie die Knoepfe ab. Hier gleichgezogen, damit
    // beide Fenster dieselbe Knopfhoehe haben.
    // Alle Knoepfe gleich breit - so breit wie die laengste Beschriftung,
    // mindestens kEdOkW. Mit fester Breite stand bei groesserer Schrift
    // "Find previou" da.
    float knopfB = dx(kEdOkW);
    for (const Str s : {Str::FindNextBtn, Str::FindPrevBtn, Str::FindClose,
                        Str::FindReplaceBtn, Str::FindReplaceAll}) {
        knopfB = std::max(knopfB, ImGui::CalcTextSize(tr(s)).x +
                                      ImGui::GetStyle().FramePadding.x * 2.0F);
    }
    // F3 / Umschalt+F3 (bzw. was darauf belegt ist) auch IM Suchfeld, wie
    // in Notepad++. Ist ein Feld aktiv, sperrt handleShortcuts alle
    // Kuerzel (WantTextInput) - dann uebernimmt das Fenster selbst. Ohne
    // aktives Feld tut es handleShortcuts; so wirkt der Druck genau einmal.
    const bool imFeld = ImGui::GetIO().WantTextInput &&
                        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const ImGuiKeyChord f3 = chordFor(keys::Action::FindRepeat);
    const ImGuiKeyChord f3zurueck = chordFor(keys::Action::FindPrevious);
    const bool f3weiter = imFeld && f3 != ImGuiKey_None && ImGui::IsKeyChordPressed(f3);
    const bool f3rueck = imFeld && f3zurueck != ImGuiKey_None && ImGui::IsKeyChordPressed(f3zurueck);
    const bool weiter =
        ImGui::Button(tr(Str::FindNextBtn), ImVec2{knopfB, 0.0F}) ||
        enterInName || enterInText || f3weiter;
    ImGui::SameLine();
    const bool zurueck =
        ImGui::Button(tr(Str::FindPrevBtn), ImVec2{knopfB, 0.0F}) || f3rueck;
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::FindClose), ImVec2{knopfB, 0.0F})) {
        g_app->findOpen = false;
    }

    // Die Ersetzen-Knoepfe. Ohne "Item containing" gibt es nichts zu
    // ersetzen - dann grau.
    ImGui::BeginDisabled(contains[0] == '\0');
    const bool einen =
        ImGui::Button(tr(Str::FindReplaceBtn), ImVec2{knopfB, 0.0F});
    ImGui::SameLine();
    const bool alle =
        ImGui::Button(tr(Str::FindReplaceAll), ImVec2{knopfB, 0.0F});
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", tr(Str::FindReplaceHint));
    }
    const bool alleDocs = ImGui::Button(tr(Str::FindReplaceAllDocs));
    ImGui::EndDisabled();
    if (einen || alle || alleDocs) {
        g_app->findLetzte.named = named;
        g_app->findLetzte.contains = contains;
        g_app->findLetzte.wholeString = g_app->findWhole;
        g_app->findErsatz = ersatz;
        if (einen) {
            (void)ersetzeEinen();
        } else {
            (void)ersetzeAlle(alleDocs);
        }
    }

    // Was gerade gilt - so sieht man sofort, ob es Treffer gibt und wo man
    // steht. Notepad++ blinkt stattdessen; eine Zeile ist deutlicher.
    if (g_app->findHits.empty()) {
        ImGui::TextDisabled("%s", tr(Str::FindNone));
    } else {
        ImGui::Text(tr(Str::FindAtOf), g_app->findAt + 1,
                    static_cast<int>(g_app->findHits.size()));
    }

    if (weiter || zurueck) {
        angabenMerken();
        findNext(zurueck ? -1 : 1);
    }
    ImGui::End();
}

// Zum naechsten Treffer springen - F3 im Original.
// Zum aktuellen Treffer springen - SICHTBAR.
//
// Herausgezogen, weil es jetzt drei Aufrufer gibt: die erste Suche, Weiter
// und Zurueck. Ohne die Mehrfachauswahl und scrollToSelected sieht man
// nichts davon, dass etwas gefunden wurde - genau der Fehler aus rc145.
void springeZuTreffer() {
    if (g_app->findHits.empty()) {
        return;
    }
    selectByPath(g_app->findHits[static_cast<std::size_t>(g_app->findAt)]);
}

// Weitersuchen wie im Original: jedes Mal FRISCH und AB DER AUSWAHL.
//
// Am laufenden BehavEd.exe beobachtet: von Zeile 6, 9, 12 aus springt es
// jeweils zum naechsten Treffer DAHINTER, vom Ende aus zum ersten
// (Umlauf). Umschalt+F3 entsprechend rueckwaerts.
//
// Vorher begann behaved immer beim ersten Treffer und lief dann eine
// einmal gemerkte Liste ab. Nach einer Aenderung am Skript zeigte die auf
// alte Stellen.
//
// Wege vergleichen wie die Reihenfolge im Baum: [1] < [1,0] < [2].
void findNext(int step) {
    const FindOptions& o = g_app->findLetzte;
    if (o.named.empty() && o.contains.empty()) {
        fensterZeigen(g_app->findOpen, "###find");   // noch nichts gesucht: erst fragen
        return;
    }
    g_app->findHits = findAll(g_app->doc.script(), o);
    if (g_app->findHits.empty()) {
        addStatus(tr(Str::FindNone));
        return;
    }
    const std::vector<Path>& h = g_app->findHits;
    const Path& ab = g_app->selectedPath;
    const int n = static_cast<int>(h.size());
    int ziel = -1;
    if (step >= 0) {
        for (int i = 0; i < n; ++i) {
            if (ab.empty() || ab < h[static_cast<std::size_t>(i)]) {
                ziel = i;
                break;
            }
        }
        if (ziel < 0) { ziel = 0; }          // Umlauf zum ersten
    } else {
        for (int i = n - 1; i >= 0; --i) {
            if (!ab.empty() && h[static_cast<std::size_t>(i)] < ab) {
                ziel = i;
                break;
            }
        }
        if (ziel < 0) { ziel = n - 1; }      // Umlauf zum letzten
    }
    g_app->findAt = ziel;
    springeZuTreffer();
}

// --- Tastenkuerzel, Tabelle aus ACCELERATOR 135 ------------------------

// Die Kennung des Knotens an einem Weg.
//
// Vergibt vorher, falls noetig: ein GERADE eingefuegter Knoten traegt noch
// 0, und `setOpen(0)` taete nichts. Der Durchlauf ist billig und aendert
// nichts, wenn schon alles vergeben ist.
// Sagen, WARUM eine Bewegung nichts getan hat - auf dem Schirm, nicht nur
// im Protokoll.
//
// shanks rc535-Protokoll: `Baum: "else" steht schon am Ende - nichts
// getan`. Er hat gezogen, es passierte nichts, und der Grund stand in
// einer Datei, die er beim Arbeiten nicht sieht.
//
// Handout Abschnitt 4: ein Werkzeug, das bei einer sinnlosen Geste einfach
// nichts tut, laesst den Benutzer glauben, es sei kaputt.
//
// `Keine` schweigt mit Absicht: nicht jede fehlgeschlagene Bewegung hat
// einen Grund, den man dem Benutzer erklaeren kann.
void zeigeAbweisung() {
    switch (g_app->doc.letzteAbweisung()) {
        case Document::Abweisung::SchonAmEnde:
            addStatus(tr(Str::MoveNoEnd));
            break;
        case Document::Abweisung::SchonGanzOben:
            addStatus(tr(Str::MoveNoTop));
            break;
        case Document::Abweisung::ZielGleichHerkunft:
            addStatus(tr(Str::MoveNoSame));
            break;
        case Document::Abweisung::ZielImGezogenen:
            addStatus(tr(Str::MoveNoSelf));
            break;
        case Document::Abweisung::ZielIstKeinBlock:
            addStatus(tr(Str::MoveNoBlock));
            break;
        case Document::Abweisung::Keine:
        case Document::Abweisung::WegUngueltig:
            break;
    }
}

// --- Nach dem Verschieben das Verschobene markieren ----------------------
//
// shank, 27.09.: "The command(s) I moved should probably be highlighted.
// Otherwise, I lose track of what I just moved." Bis rc568 setzte das
// Ablegen die Auswahl auf die ANKERZEILE (r.path) und leerte die
// Mehrfachauswahl - markiert war danach etwas anderes als das Gezogene.
//
// Wege taugen dafuer nicht, sie aendern sich durch die Bewegung selbst.
// Die Kennung bleibt (Document::vergibKennungen) - also vorher die
// Kennungen merken, nachher ihre neuen Wege suchen.
// (kennungenVon / markiereKennungen: app_internal.h, Definition am Dateiende.)

[[nodiscard]] Kennung kennungFuer(const Path& p) {
    if (p.empty()) {
        return 0;
    }
    g_app->doc.vergibKennungen();
    const Node* n = nodeAt(g_app->doc.script(), p);
    return n != nullptr ? n->kennung : 0;
}

void handleTreeKeys() {
    // Stufe 1b: im Karten-Editor gelten NRCs Tasten, nicht die des Baums.
    if (karteneditor::aktiv()) {
        return;
    }
    // Nur wenn kein Dialog offen ist und nichts getippt wird - sonst
    // wandert die Auswahl, waehrend man einen Wert eingibt.
    //
    // dialogAtFrameStart: siehe handleShortcuts. Auch die Pfeiltasten
    // sollen nicht in dem Bild wandern, in dem ein Dialog schliesst.
    if (g_app->dialogAtFrameStart || g_app->findAtFrameStart ||
        g_app->editorOpen || g_app->pk3Open ||
        g_app->prefsOpen || g_app->aboutOpen || g_app->missionPickOpen ||
        g_app->findOpen || g_app->askSaveOpen || g_app->frageOffen ||
        ImGui::GetIO().WantTextInput || g_app->tlUeberMaus) {
        return;
    }
    // Mit ALT gehoeren die Pfeiltasten NICHT hierher.
    //
    // Alt+Pfeil verschiebt Befehle (Str::ActMoveUp/-Down). Ohne diese
    // Abfrage feuerte zusaetzlich die blanke Pfeiltaste, und die setzt die
    // Auswahl ueber selectRow(..., ctrl=false, ...) auf EINE Zeile zurueck.
    // Danach verschob das Kuerzel nur noch diese eine - genau der Fehler,
    // den die Gemeinschaft gemeldet hat: "It skips lines and doesn't work
    // if you have multiple commands selected. Clicking the actual Move up
    // and Move down buttons works perfectly."
    //
    // Der Knopf rief immer schon dasselbe moveSelection() auf. Der
    // Unterschied lag nie im Verschieben, sondern darin, dass die
    // Tastenfassung sich vorher selbst die Auswahl zerschossen hat.
    const bool altGedrueckt = ImGui::GetIO().KeyAlt;
    if (!altGedrueckt && ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
        moveSelectionByKey(1);
    }
    if (!altGedrueckt && ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
        moveSelectionByKey(-1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown, true)) {
        moveSelectionByKey(10);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp, true)) {
        moveSelectionByKey(-10);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
        moveSelectionByKey(-static_cast<int>(g_app->rows.size()));
    }
    if (ImGui::IsKeyPressed(ImGuiKey_End, false)) {
        moveSelectionByKey(static_cast<int>(g_app->rows.size()));
    }
    // Links und rechts klappen einen Block zu und auf - wie in jedem Baum.
    if (!g_app->selectedPath.empty()) {
        const Node* n = nodeAt(g_app->doc.script(), g_app->selectedPath);
        if (n != nullptr && n->hasBlock) {
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
                g_app->expanded.setOpen(kennungFuer(g_app->selectedPath), true);
                rebuildTree();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
                g_app->expanded.setOpen(kennungFuer(g_app->selectedPath), false);
                rebuildTree();
            }
        }
    }
    // Ebenfalls ueber eine Route: so kann der Editor sie beanspruchen,
    // wenn er offen ist, ohne dass hier ein Flag noetig waere.
    if (ImGui::Shortcut(ImGuiKey_Enter) && !g_app->selectedPath.empty()) {
        openEditorForNode(g_app->selectedPath);
    }
}

// --- Tastenbelegung ----------------------------------------------------
//
// Ein ImGuiKeyChord ist Taste und Zusatztasten in EINER Zahl
// (ImGuiMod_Ctrl | ImGuiKey_S). Genau in dieser Form wird sie auch
// gespeichert - imgui.h sagt zu GetKeyName ausdruecklich: "not meant to be
// saved persistently nor compared".
//
// Die Vorgaben kommen, wo es sie gibt, aus ACCELERATOR 135 des Originals.
// Was Raven nicht hatte, steht darunter und ist als Zutat gekennzeichnet.
ImGuiKeyChord defaultChord(keys::Action a) {
    // Find auf Strg+F - VOR der Tabelle des Originals. Die Begruendung steht
    // unten beim switch; dort war sie wirkungslos, weil die Tabelle
    // (ACCELERATOR 135: Strg+F3) zuerst griff. Gemeldet: shank wollte Find
    // selbst auf Strg+F legen, und nach "Reset all to defaults" stand
    // wieder Strg+F3 da.
    if (a == keys::Action::Find) {
        return ImGuiMod_Ctrl | ImGuiKey_F;
    }
    struct Bind { ImGuiKey imgui; int vk; };
    static const Bind kVk[] = {
        {ImGuiKey_Delete, 0x2E}, {ImGuiKey_Space, 0x20}, {ImGuiKey_Backspace, 0x08},
        {ImGuiKey_Enter, 0x0D},  {ImGuiKey_Insert, 0x2D}, {ImGuiKey_F3, 0x72},
        {ImGuiKey_A, 'A'}, {ImGuiKey_B, 'B'}, {ImGuiKey_C, 'C'}, {ImGuiKey_O, 'O'},
        {ImGuiKey_R, 'R'}, {ImGuiKey_S, 'S'}, {ImGuiKey_V, 'V'}, {ImGuiKey_X, 'X'},
        {ImGuiKey_T, 'T'},
        {ImGuiKey_KeypadAdd, 0x6B}, {ImGuiKey_KeypadSubtract, 0x6D},
    };
    for (const keys::Shortcut& sc : keys::table()) {
        if (sc.action != a) {
            continue;
        }
        for (const Bind& b : kVk) {
            if (b.vk != sc.key) {
                continue;
            }
            ImGuiKeyChord c = b.imgui;
            if (sc.mod == keys::Mod::Ctrl) { c |= ImGuiMod_Ctrl; }
            if (sc.mod == keys::Mod::Alt) { c |= ImGuiMod_Alt; }
            if (sc.mod == keys::Mod::Shift) { c |= ImGuiMod_Shift; }
            return c;
        }
    }
    // Nicht in der Ressource - unsere Zutaten.
    switch (a) {
        // Die Suche auf Ctrl+F.
        //
        // ACCELERATOR 135 des Originals sagt Ctrl+F3, und die Ressource
        // bleibt sonst ueberall die Vorlage. Hier nicht: Ctrl+F ist seit
        // dreissig Jahren die Suchtaste in Windows, und wer sie drueckt,
        // denkt nicht darueber nach. Ctrl+F3 ist in BehavEd ein Ueberbleibsel
        // aus einer Zeit, in der Ctrl+F noch nicht gesetzt war.
        //
        // keytest vergleicht weiterhin die TABELLE gegen die Ressource -
        // dort steht Ctrl+F3 unveraendert. Nur die wirkende Belegung ist
        // hier eine andere, und das ist eine bewusste Entscheidung, keine
        // Abweichung aus Versehen.
        case keys::Action::Find:     return ImGuiMod_Ctrl | ImGuiKey_F;
        case keys::Action::MoveUp:   return ImGuiMod_Alt | ImGuiKey_UpArrow;
        case keys::Action::MoveDown: return ImGuiMod_Alt | ImGuiKey_DownArrow;
        case keys::Action::Undo:     return ImGuiMod_Ctrl | ImGuiKey_Z;
        case keys::Action::Redo:     return ImGuiMod_Ctrl | ImGuiKey_Y;
        // Wie Notepad++.
        case keys::Action::BookmarkToggle: return ImGuiMod_Ctrl | ImGuiKey_F2;
        case keys::Action::BookmarkNext:   return ImGuiKey_F2;
        case keys::Action::BookmarkPrev:   return ImGuiMod_Shift | ImGuiKey_F2;
        // Wie in jedem Editor mit Reitern.
        case keys::Action::SaveAll: return ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S;
        // Hilfe wie in Windows-Programmen ueblich.
        case keys::Action::Guide: return ImGuiKey_F1;
        default: return ImGuiKey_None;
    }
}

// Was JETZT gilt: Vorgabe, sofern die Einstellungen nichts anderes sagen.
ImGuiKeyChord chordFor(keys::Action a) {
    for (const keys::Bindable& b : keys::bindable()) {
        if (b.action != a) {
            continue;
        }
        for (const Settings::KeyBinding& kb : g_app->settings.keyBindings) {
            if (kb.action == b.name) {
                return kb.chord;   // 0 heisst ausdruecklich "keine Taste"
            }
        }
        break;
    }
    return defaultChord(a);
}

// Eine eigene Belegung eintragen oder wieder entfernen.
void setChord(keys::Action a, ImGuiKeyChord c) {
    const char* name = nullptr;
    for (const keys::Bindable& b : keys::bindable()) {
        if (b.action == a) { name = b.name; break; }
    }
    if (name == nullptr) {
        return;
    }
    auto& v = g_app->settings.keyBindings;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].action != name) {
            continue;
        }
        if (c == defaultChord(a)) {
            v.erase(v.begin() + static_cast<std::ptrdiff_t>(i));  // wieder Vorgabe
        } else {
            v[i].chord = c;
        }
        return;
    }
    if (c != defaultChord(a)) {
        v.push_back({name, c});
    }
}

// Auswahl eine Zeile hoch oder runter.
//
// Frei stehend, nicht mehr als Lambda im Knopfstreifen: seit "Move up" und
// "Move down" eine Taste haben (Alt+Pfeil), braucht auch die
// Tastenbehandlung sie.
// Einen Baumweg als "0/2/1" schreiben - die Ebene sieht man am Schraegstrich.
std::string wegAlsText(const std::vector<std::size_t>& w) {
    std::string t;
    for (std::size_t x : w) {
        if (!t.empty()) {
            t += "/";
        }
        t += std::to_string(x);
    }
    return t.empty() ? std::string("(oben)") : t;
}

void moveSelection(bool up) {
    const Path& sel = g_app->selectedPath;
        std::vector<Path> m = selectionOrCurrent();
        if (m.size() > 1) {
            if (g_app->doc.moveAll(m, up)) {
                g_app->selection = m;
                g_app->selectedPath = m.back();
                rebuildTree();
            }
            return;
        }
        // Die neue Stelle kommt VON der Bewegung, nicht aus einer eigenen
        // Rechnung.
        //
        // Vorher stand hier `--selectedPath.back()`, und das stimmt nur,
        // solange der Knoten seine Ebene behaelt. Seit rc516 kann er den
        // Block verlassen: dann ist `back()` gleich 0, es griff KEIN Zweig,
        // und die Auswahl zeigte hinterher auf den falschen Knoten. Nach
        // unten wurde sogar falsch hochgezaehlt.
        //
        // Dieselbe Lehre wie bei den Spaltenbreiten: erfragen statt
        // herleiten.
        // Sagen, wenn es eine MAKROZEILE ist.
        //
        // shank hat es jetzt viermal versucht, und in jedem seiner
        // Protokolle steht `Tiefe 1` - er bewegt Makrozeilen, keine
        // Blockkinder. Das Modell arbeitet richtig (nachgestellt: ein Block
        // mit einem einzigen Kind laesst sich ueber alle drei Wege
        // herausholen), aber das Programm hat geschwiegen.
        //
        // Ein Werkzeug, das bei einer sinnlosen Geste einfach nichts tut,
        // laesst den Benutzer glauben, es sei kaputt. Es soll sagen, warum.
        for (const Row& zeile : g_app->rows) {
            if (zeile.path == sel && zeile.ausMakro) {
                diag::detail("Bewegen: \"" + zeile.name +
                             "\" ist eine MAKROZEILE (Weg " +
                             wegAlsText(sel) +
                             ") - ein Makro ist kein Block, seine Befehle "
                             "stehen daneben. Es gibt nichts, woraus sie "
                             "herauskommen koennten.");
                break;
            }
        }
        Path nachher;
        const bool ok = up ? g_app->doc.moveUp(sel, &nachher)
                           : g_app->doc.moveDown(sel, &nachher);
        if (!ok) {
            zeigeAbweisung();
            return;
        }
        // Den Aufklapp-Zustand mitziehen.
        //
        // Ohne das klappt ein offener Block zu, sobald ein Befehl an ihm
        // vorbeizieht - der Zustand haengt an Wegen, und die verschieben
        g_app->selectedPath = nachher;
    g_app->selection.assign(1, g_app->selectedPath);
    rebuildTree();
}

// Die Beschriftung der Taste einer Aktion, fuer Menue und Sprechblasen.
const char* chordName(keys::Action a) {
    const ImGuiKeyChord c = chordFor(a);
    return c == ImGuiKey_None ? nullptr : ImGui::GetKeyChordName(c);
}

// Die Beschriftung einer Aktion im Einstellungsfenster.
Str labelFor(keys::Action a) {
    switch (a) {
        case keys::Action::Delete:       return Str::ActDelete;
        case keys::Action::Clone:        return Str::ActClone;
        case keys::Action::Copy:         return Str::ActCopy;
        case keys::Action::Cut:          return Str::ActCut;
        case keys::Action::CutAlt:       return Str::ActCutAlt;
        case keys::Action::ExpandNode:   return Str::ActExpandNode;
        case keys::Action::CollapseNode: return Str::ActCollapseNode;
        case keys::Action::ExpandAll:    return Str::ActExpandAll;
        case keys::Action::CollapseAll:  return Str::ActCollapseAll;
        case keys::Action::Paste:        return Str::ActPaste;
        case keys::Action::CommentOut:   return Str::ActRem;
        case keys::Action::Uncomment:    return Str::ActUnrem;
        case keys::Action::Find:         return Str::ActFind;
        case keys::Action::FindRepeat:   return Str::ActFindNext;
        case keys::Action::FindPrevious: return Str::ActFindPrev;
        case keys::Action::MoveUp:       return Str::ActMoveUp;
        case keys::Action::MoveDown:     return Str::ActMoveDown;
        case keys::Action::Undo:         return Str::ActUndo;
        case keys::Action::Redo:         return Str::ActRedo;
        case keys::Action::BookmarkToggle: return Str::ActBookmarkToggle;
        case keys::Action::BookmarkNext:   return Str::ActBookmarkNext;
        case keys::Action::BookmarkPrev:   return Str::ActBookmarkPrev;
        case keys::Action::EditItem:     return Str::ActEdit2;
        case keys::Action::InsertItem:   return Str::ActInsert;
        case keys::Action::Open:         return Str::ActOpen;
        case keys::Action::Save:         return Str::ActSave;
        case keys::Action::SaveAs:       return Str::ActSaveAs;
        case keys::Action::SaveAll:      return Str::ActSaveAll;
        case keys::Action::Backup:       return Str::ActBackup;
        case keys::Action::Restore:      return Str::ActRestore;
        case keys::Action::Guide:        return Str::HelpGuide;
        default:                         return Str::ActDelete;
    }
}

// REM umschalten - EINE Fassung fuer Ruecktaste, Rechtsklick und Knopf.
//
// Auf einer auskommentierten Zeile holt es die Anweisung zurueck, zu der die
// Zeile gehoert (samt ihrem Block, ohne getrennt auskommentierte Nachbarn),
// und waehlt sie aus. Sonst kommentiert es aus. Original: "Toggle REMark
// status of current line (BACKSPACE ...)".
bool remUmschalten(const Path& weg) {
    const Node* n = nodeAt(g_app->doc.script(), weg);
    if (n == nullptr) {
        return false;
    }
    if (n->kind == Node::Kind::LineComment) {
        Path anfang;
        if (!g_app->doc.uncomment(weg, nullptr, &anfang)) {
            return false;
        }
        g_app->selectedPath = anfang;
        g_app->selection.assign(1, anfang);
        rebuildTree();
        return true;
    }
    if (!g_app->doc.commentOut(weg)) {
        return false;
    }
    rebuildTree();
    return true;
}

// Numpad +/- (Original: Befehle 32774 und 32776).
//
// Am laufenden BehavEd.exe beobachtet: Num+ auf dem obersten Knoten zeigt
// 46 statt 5 Zeilen - es klappt den Eintrag SAMT allen Unterpunkten auf,
// wie TVE_EXPAND auf jeder Ebene. Num- klappt nur den Eintrag selbst zu;
// was darunter offen war, bleibt es fuer das naechste Aufklappen.
void klappeTeilbaum(const Path& weg, bool auf) {
    const Node* n = weg.empty() ? nullptr : nodeAt(g_app->doc.script(), weg);
    if (n == nullptr) {
        return;
    }
    g_app->doc.vergibKennungen();
    if (!auf || n->kind == Node::Kind::LineComment) {
        // Ein auskommentierter Block (Kopfzeile "//(BHVDREM)") klappt ueber
        // die Kennung seiner Kopfzeile - siehe tree.cpp.
        g_app->expanded.setOpen(n->kennung, auf);
        rebuildTree();
        return;
    }
    std::function<void(const Node&)> oeffne = [&](const Node& k) {
        if (k.hasBlock || k.kind == Node::Kind::Macro) {
            g_app->expanded.setOpen(k.kennung, true);
        }
        for (const Node& kind : k.children) {
            oeffne(kind);
        }
    };
    oeffne(*n);
    rebuildTree();
}

void handleShortcuts() {
    // Stufe 1b: im Karten-Editor gelten NRCs Tastenkuerzel (Befehlstabelle des
    // Arbeitsbereichs) und nicht die von BehavEd - "Kuerzel gelten nur,
    // solange der Karten-Tab den Fokus hat", und umgekehrt.
    if (karteneditor::aktiv()) {
        return;
    }
    // Zwei Dinge auf einmal:
    //
    // 1. dialogAtFrameStart - ein Dialog, der GERADE mit Eingabe geschlossen
    //    hat, hat die Taste schon verbraucht. Ohne diese Abfrage machte
    //    Enter im Editor genau das Falsche: zu und sofort wieder auf.
    //    Anders als handleTreeKeys nimmt diese Stelle IsKeyPressed statt
    //    Shortcut, also gibt es keine Route, die das Popup beanspruchen
    //    koennte - die Wache muss es hier selbst tun.
    //
    // 2. prefsOpen, pk3Open, aboutOpen, missionPickOpen standen bisher
    //    nicht in der Liste. Enter loeste damit auch dann "Bearbeiten" aus,
    //    wenn die Einstellungen offen waren.
    // Bei offenem Find-Fenster NUR Rueckgaengig und Wiederholen - nach
    // "Replace All" will man sofort mit Strg+Z zurueck, wie in Notepad++.
    // Alles andere bleibt gesperrt (Enter im Suchfeld darf nicht
    // "Bearbeiten" ausloesen), und Tippen sperrt ohnehin alles.
    const bool nurRueckgaengig = g_app->findOpen || g_app->findAtFrameStart;
    if (ImGui::GetIO().WantTextInput || g_app->dialogAtFrameStart ||
        g_app->kuerzelWartet ||
        g_app->editorOpen || g_app->askSaveOpen ||
        g_app->frageOffen || g_app->prefsOpen ||
        g_app->pk3Open || g_app->aboutOpen || g_app->missionPickOpen) {
        return;   // waehrend einer Eingabe nicht dazwischenfunken
    }
    // Strg+Umschalt+Z gilt weiterhin als Wiederherstellen, auch wenn die
    // Belegung von Redo geaendert wurde - das erwartet jeder.
    // Strg+Umschalt+S sichert alle Reiter - so heisst es in jedem Editor,
    // der Reiter kennt. Vor Strg+S geprueft, weil ImGui den kleineren
    // Akkord sonst zuerst greifen laesst.
    // Strg+Umschalt+S (Alle sichern) steht jetzt in der Belegung - siehe
    // keys::Action::SaveAll. ImGui vergleicht die Zusatztasten genau, Strg+S
    // greift also nicht mit, wenn Umschalt gedrueckt ist.
    // Die Alt-Kuerzel der Knoepfe im Original ("&New", "&Open", "&Save",
    // "Sa&ve", "&Prefs", "E&xit", "&MRU", "&Find", "&Delete"). ImGui
    // unterstreicht keine Buchstaben, die Tasten wirken trotzdem. Alt+B
    // bleibt Backup - so steht es im Accelerator, und der geht in einem
    // Windows-Dialog vor.
    {
        const bool alt = ImGui::GetIO().KeyAlt && !ImGui::GetIO().KeyCtrl &&
                         !nurRueckgaengig;
        if (alt) {
            if (ImGui::IsKeyPressed(ImGuiKey_N, false)) { doNew(); return; }
            if (ImGui::IsKeyPressed(ImGuiKey_O, false)) { doOpen(); return; }
            if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                if (doSave()) { addStatus(tr(Str::MsgSaved)); }
                return;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
                if (doSaveAs()) { addStatus(tr(Str::MsgSaved)); }
                return;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_P, false)) { g_app->prefsOpen = true; return; }
            if (ImGui::IsKeyPressed(ImGuiKey_M, false)) { g_app->mruOeffnen = true; return; }
            if (ImGui::IsKeyPressed(ImGuiKey_F, false)) { fensterZeigen(g_app->findOpen, "###find"); return; }
            if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
                if (confirmQuit()) {
                    g_app->quitConfirmed = true;
                    g_app->wantQuit = true;
                }
                return;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_D, false)) {
                // Wie der Knopf Delete: auf alle ausgewaehlten Zeilen.
                const std::vector<Path> m = selectionOrCurrent();
                const bool ok = m.size() > 1
                                    ? g_app->doc.removeAll(m)
                                    : (!g_app->selectedPath.empty() &&
                                       g_app->doc.removeAt(g_app->selectedPath));
                if (ok) {
                    g_app->selection.clear();
                    g_app->selectedPath.clear();
                    rebuildTree();
                }
                return;
            }
        }
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)) {
        doRedo();
        return;
    }

    const Path& sel = g_app->selectedPath;
    // Ueber die BELEGUNG laufen, nicht ueber eine feste Tastenliste. Damit
    // wirkt jede Aenderung aus dem Einstellungsfenster sofort, und eine
    // Aktion ohne Taste (chord 0) faellt von selbst heraus.
    for (const keys::Bindable& bind : keys::bindable()) {
        // Bei offenem Suchfenster nur, was dort nichts anrichten kann:
        // Rueckgaengig/Wiederholen und - wie in Notepad++ - F3/Umschalt+F3
        // zum Weitersuchen. Gebraucht nach "Find similar": das Fenster ist
        // offen, und F3 soll zum naechsten gleichen Befehl springen.
        if (nurRueckgaengig && bind.action != keys::Action::Undo &&
            bind.action != keys::Action::Redo && bind.action != keys::Action::FindRepeat &&
            bind.action != keys::Action::FindPrevious) {
            continue;
        }
        const ImGuiKeyChord chord = chordFor(bind.action);
        if (chord == ImGuiKey_None || !ImGui::IsKeyChordPressed(chord)) {
            continue;
        }
        g_app->letzteKuerzelAktion = bind.action;
        ++g_app->kuerzelZaehler;
        if (g_app->kuerzelNurMelden &&
            (bind.action == keys::Action::Open || bind.action == keys::Action::SaveAs ||
             bind.action == keys::Action::SaveAll)) {
            continue;   // Selbsttest: gemeldet, nicht ausgefuehrt
        }
        switch (bind.action) {
            case keys::Action::MoveUp:   moveSelection(true); break;
            case keys::Action::MoveDown: moveSelection(false); break;
            case keys::Action::BookmarkToggle: lesezeichenUmschalten(selectionOrCurrent()); break;
            case keys::Action::BookmarkNext:   lesezeichenSpringen(true); break;
            case keys::Action::BookmarkPrev:   lesezeichenSpringen(false); break;
            case keys::Action::Undo: {
                // Ueber der Karte oder der Entity-Liste: die letzte
                // Entity-Aenderung (eigener Verlauf, gui/app_mission.cpp).
                if (entityFokus() && entityRueckgaengig()) {
                    break;
                }
                // Erst die Gizmo-Vorschau, dann das Skript.
                //
                // Gemeldet als "undo geht nicht strg + z". Der Grund war,
                // dass eine verschobene Kamera noch gar nicht im Skript
                // steht - es gibt also nichts rueckgaengig zu machen, und
                // doUndo() nahm stattdessen die letzte ECHTE Aenderung
                // zurueck. Verwirrender geht es kaum.
                //
                // Jetzt raeumt Strg+Z zuerst die Vorschau weg. Das ist,
                // was der Anwender in dem Moment meint - und danach wirkt
                // es wieder auf das Skript.
                // BEIDE Vorschauen zaehlen: die Verschiebung UND die
                // Drehung. Gemeldet als "Undo funktioniert nicht dabei" -
                // gedreht war eine Kamera, geprueft wurde nur der Versatz.
                if (g_app->gizmoOffset[0] != 0.0F ||
                    g_app->gizmoOffset[1] != 0.0F ||
                    g_app->gizmoOffset[2] != 0.0F ||
                    g_app->gizmoAngles[0] != 0.0F ||
                    g_app->gizmoAngles[1] != 0.0F ||
                    g_app->gizmoAngles[2] != 0.0F) {
                    for (int k = 0; k < 3; ++k) {
                        g_app->gizmoOffset[k] = 0.0F;
                        g_app->gizmoAngles[k] = 0.0F;
                    }
                    g_app->gizmoAxis = -1;
                    g_app->gizmoRotating = false;
                    g_app->mapDirty = true;
                    break;
                }
                doUndo();
                break;
            }
            case keys::Action::Redo:
                if (entityFokus() && entityWiederholen()) {
                    break;
                }
                doRedo();
                break;
            case keys::Action::Delete: {
                // Ueber der Karte mit gewaehlter Entity: die Entity.
                if (entityFokus() && g_app->pickedEntity >= 0 && entityAuswahlLoeschen()) {
                    break;
                }
                const std::vector<Path> m = selectionOrCurrent();
                const bool ok = m.size() > 1 ? g_app->doc.removeAll(m)
                                             : g_app->doc.removeAt(sel);
                if (ok) {
                    g_app->selection.clear();
                    g_app->selectedPath.clear();
                    rebuildTree();
                }
                break;
            }
            case keys::Action::Clone: {
                // Auch mehrere auf einmal - wie Kopieren und Loeschen.
                const std::vector<Path> m = selectionOrCurrent();
                const bool ok = m.size() > 1 ? g_app->doc.cloneAll(m)
                                             : g_app->doc.cloneAt(sel);
                if (ok) { rebuildTree(); }
                break;
            }
            case keys::Action::Copy:
                kopieren(false);
                break;
            case keys::Action::Cut:
            case keys::Action::CutAlt:   // Strg+T - im Original ein zweites Cut
                kopieren(true);
                break;
            case keys::Action::ExpandNode:
                klappeTeilbaum(sel, true);
                break;
            case keys::Action::CollapseNode:
                klappeTeilbaum(sel, false);
                break;
            case keys::Action::ExpandAll:
                g_app->expanded.openAll();
                rebuildTree();
                break;
            case keys::Action::CollapseAll:
                g_app->expanded.closeAll();
                rebuildTree();
                break;
            case keys::Action::Paste:
                einfuegen(sel);
                break;
            case keys::Action::CommentOut: {
                // Umschalten wie der Knopf REM - Original: "Toggle REMark
                // status of current line (BACKSPACE ...)". Vorher konnte die
                // Ruecktaste nur auskommentieren.
                (void)remUmschalten(sel);
                break;
            }
            case keys::Action::Uncomment: {
                // Strg+Ruecktaste: nur zuruecknehmen, nie auskommentieren.
                const Node* n = nodeAt(g_app->doc.script(), sel);
                if (n != nullptr && n->kind == Node::Kind::LineComment) {
                    (void)remUmschalten(sel);
                }
                break;
            }
            case keys::Action::EditItem: openEditorForNode(sel); break;
            case keys::Action::InsertItem:
                // Einfg fuegt den links gewaehlten Befehl ein - dasselbe wie
                // der Doppelklick in der Ereignisliste.
                if (g_app->selectedCommand >= 0 &&
                    g_app->selectedCommand < static_cast<int>(g_app->db.commands.size())) {
                    insertCommand(g_app->db.commands[
                        static_cast<std::size_t>(g_app->selectedCommand)]);
                }
                break;
            case keys::Action::Open: doOpen(); break;
            case keys::Action::Save: (void)doSave(); break;
            case keys::Action::SaveAll: doSaveAll(); break;
            case keys::Action::SaveAs: (void)doSaveAs(); break;
            case keys::Action::Find: fensterZeigen(g_app->findOpen, "###find"); break;
            case keys::Action::FindRepeat: findNext(1); break;
            case keys::Action::FindPrevious: findNext(-1); break;
            // Beide stehen im Kuerzelfenster und liessen sich belegen - die
            // Belegung tat aber nichts, weil sie hier fehlten. Gefunden vom
            // Selbsttest, der jede Belegung aendert und ausprobiert.
            case keys::Action::Backup: doBackup(); break;
            case keys::Action::Restore: doRestore(); break;
            case keys::Action::Guide: handbuchOeffnen(); break;
            default: break;
        }
    }
}

// --- Einstellungen (Dialog 131 des Originals) --------------------------
//
// Uebernommen sind die Felder, die es heute noch gibt. Weggelassen: alles
// zu SourceSafe (drei Felder und ein Schalter) - das System ist lange fort,
// und die Browse-Knoepfe dafuer waren schon im Original ausgegraut. Ebenso
// die fuenf Knoepfe "Set All Options to TREK/SOF2/JK2/JKA/XMen default":
// dahinter stehen Raven-interne Pfade, die es nirgends mehr gibt.
// Die Felder des Einstellungsfensters zurueckspielen ("Cancel", X).
void prefsZurueck() {
    const Settings& v = g_app->prefsVorher;
    Settings& st = g_app->settings;
    st.scriptPath = v.scriptPath;
    st.commandFile = v.commandFile;
    st.sourcePath = v.sourcePath;
    st.ibizePath = v.ibizePath;
    st.reopenLastFile = v.reopenLastFile;
    st.alphaSortPulldowns = v.alphaSortPulldowns;
    st.alternativeIcons = v.alternativeIcons;
    st.queryOnDiscard = v.queryOnDiscard;
    st.dialogUsesLastDir = v.dialogUsesLastDir;
    // rc601 ton: die Mischpegel der Vorschau
    st.tonEffekte = v.tonEffekte;
    st.tonStimmen = v.tonStimmen;
    st.tonMusik = v.tonMusik;
    // rc601 (bewegung, Durchsicht 2): baut die Szene neu, wenn sie sich aenderte.
    schwierigkeitWaehlen(v.schwierigkeit);
    // rc601 integ: das Regelwerk (regelwerkPruefen baut neu).
    st.regelwerk = v.regelwerk;
}

void drawPrefs() {
    if (!g_app->prefsOpen) {
        // Geschlossen, ohne OK (Cancel oder X): alten Stand zurueck.
        if (g_app->prefsHatVorher) {
            prefsZurueck();
            g_app->prefsHatVorher = false;
        }
        return;
    }
    if (!g_app->prefsHatVorher) {
        g_app->prefsVorher = g_app->settings;
        g_app->prefsHatVorher = true;
    }
    ImGui::OpenPopup(tr(Str::PrefsTitle));
    if (!ImGui::BeginPopupModal(tr(Str::PrefsTitle), &g_app->prefsOpen,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    Settings& st = g_app->settings;
    const float w = ImGui::GetFontSize() * 24.0F;

    auto pathRow = [&](Str label, std::string* value, bool folder, const char* filter) {
        ImGui::TextUnformatted(tr(label));
        char buf[512];
        std::snprintf(buf, sizeof(buf), "%s", value->c_str());
        ImGui::PushID(tr(label));
        ImGui::SetNextItemWidth(w);
        if (ImGui::InputText("##v", buf, sizeof(buf))) {
            *value = buf;
        }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::PrefsBrowse))) {
            // Fuer Ordner gibt es keinen eigenen Dialog: eine Datei darin
            // auswaehlen und den Ordner davon nehmen. Das ist weniger
            // elegant als ein Ordnerdialog, funktioniert aber ueberall.
            const std::string p = platform::openFileDialog(tr(label), filter, *value);
            if (!p.empty()) {
                *value = folder ? directoryOf(p) : p;
            }
        }
        ImGui::PopID();
    };

    ImGui::SeparatorText(tr(Str::PrefsDirectories));
    pathRow(Str::PrefsScriptPath, &st.scriptPath, true, kScriptFilter);
    pathRow(Str::PrefsCommandFile, &st.commandFile, false,
            "Command description (*.bhc)|*.bhc|All files (*.*)|*.*");
    pathRow(Str::PrefsSourcePath, &st.sourcePath, true,
            "Engine headers (*.h)|*.h|All files (*.*)|*.*");
    pathRow(Str::PrefsIbizePath, &st.ibizePath, false,
            "IBIZE (IBIZE.EXE)|IBIZE.EXE;*.exe|All files (*.*)|*.*");

    ImGui::TextDisabled("%s", tr(Str::PrefsNote));
    if (ImGui::Button(tr(Str::PrefsReload))) {
        CommandDb fresh;
        std::vector<LoadDiag> ld;
        // rc604: ohne eigene .bhc der Befehlssatz des Regelwerks (data/jk2 bei
        // Jedi Outcast, befehlssatzFuerRegelwerk).
        const std::string satz = g_app->befehlssatzOrdner.empty() ? g_app->dataDir : g_app->befehlssatzOrdner;
        const std::string bhc = st.commandFile.empty()
                                    ? satz + "/behaved.bhc"
                                    : st.commandFile;
        const std::string dir = st.sourcePath.empty() ? satz : st.sourcePath;
        if (loadCommandDb(bhc, dir, fresh, ld)) {
            (void)loadCommandDb(directoryOf(bhc) + "/supplement.bhc", dir, fresh, ld);
            g_app->db = std::move(fresh);
            rebuildTree();
            char msg[200];
            std::snprintf(msg, sizeof(msg), tr(Str::MsgModelLoaded),
                          static_cast<int>(g_app->db.commands.size()),
                          static_cast<int>(g_app->db.typesets.size()),
                          static_cast<int>(g_app->db.macros.size()));
        } else {
            platform::showError(bhc, tr(Str::AppTitle));
        }
    }

    ImGui::SeparatorText(tr(Str::GroupApplication));
    ImGui::Checkbox(tr(Str::PrefsReopen), &st.reopenLastFile);
    ImGui::Checkbox(tr(Str::PrefsAlphaSort), &st.alphaSortPulldowns);
    ImGui::Checkbox(tr(Str::PrefsAltIcons), &st.alternativeIcons);
    ImGui::Checkbox(tr(Str::PrefsQuery), &st.queryOnDiscard);
    ImGui::Checkbox(tr(Str::PrefsLastDir), &st.dialogUsesLastDir);

    // --- rc601 ton: die Mischpegel der Vorschau ---------------------------
    //
    // Wie die Klang-Cvars der Engine (s_volume, s_volumeVoice,
    // s_musicvolume), mit deren Vorgaben. Wirkt sofort auf alles, was
    // gerade klingt (tonBild regelt jedes Bild nach); die Musik ab dem
    // naechsten Stueck. Cancel stellt zurueck (prefsZurueck).
    ImGui::SeparatorText(tr(Str::PrefsTonPegel));
    ImGui::SetNextItemWidth(w * 0.45F);
    ImGui::SliderFloat(tr(Str::PrefsTonEffekte), &st.tonEffekte, 0.0F, 1.0F, "%.2f");
    ImGui::SetNextItemWidth(w * 0.45F);
    ImGui::SliderFloat(tr(Str::PrefsTonStimmen), &st.tonStimmen, 0.0F, 1.0F, "%.2f");
    ImGui::SetNextItemWidth(w * 0.45F);
    ImGui::SliderFloat(tr(Str::PrefsTonMusik), &st.tonMusik, 0.0F, 1.0F, "%.2f");
    if (ImGui::Button(tr(Str::PrefsTonSpielwerte))) {
        const sound::Mischpegel vorgabe;
        st.tonEffekte = vorgabe.effekte;
        st.tonStimmen = vorgabe.stimmen;
        st.tonMusik = vorgabe.musik;
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w);
    ImGui::TextDisabled("%s", tr(Str::PrefsTonHinweis));
    ImGui::PopTextWrapPos();
    // rc601 (bewegung, Durchsicht 2): die Schwierigkeit wie g_spskill. Sie
    // aendert, wie schnell manche NPCs drehen und laufen (NPC_Begin,
    // NPC_SetMiscDefaultData) - die Vorschau baut die Szene dann neu.
    ImGui::SeparatorText(tr(Str::PrefsGroupPreview));
    {
        // rc601 integ: die Spielregeln (OpenJK / Movie Duels), automatisch nach
        // den Spielordnern. regelwerkPruefen (jedes Bild) baut die Szene neu.
        // rc604: dazu Jedi Outcast (eigene Spielordner, Befehlssatz data/jk2).
        const int erkannt = regelwerkSpiel(kRegelwerkAuto, regelwerkOrdnerErkannt(), regelwerkJediOutcast());
        char autoName[160];
        std::snprintf(autoName, sizeof(autoName), tr(Str::PrefsRegelAuto),
                      erkannt == kRegelwerkMovieDuels    ? tr(Str::PrefsRegelMd)
                      : erkannt == kRegelwerkJediOutcast ? tr(Str::PrefsRegelJk2)
                                                         : tr(Str::PrefsRegelOpenJK));
        const char* const regeln[4] = {autoName, tr(Str::PrefsRegelOpenJK), tr(Str::PrefsRegelMd),
                                       tr(Str::PrefsRegelJk2)};
        int r = std::clamp(st.regelwerk, 0, 3);
        ImGui::SetNextItemWidth(w * 0.5F);
        if (ImGui::Combo(tr(Str::PrefsRegelwerk), &r, regeln, 4)) {
            st.regelwerk = r;
            (void)regelwerkPruefen();
        }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::PrefsRegelNote)); }
    }
    {
        // rc601 integ: Eintrag 0 = nie gewaehlt (-1) - die Vorgabe des
        // Regelwerks (MD Jedi-Ritter, OpenJK Padawan); dahinter 0..3.
        const char* const namen[4] = {tr(Str::DiffPadawan), tr(Str::DiffJedi), tr(Str::DiffKnight),
                                      tr(Str::DiffMaster)};
        char vorgabe[160];
        std::snprintf(vorgabe, sizeof(vorgabe), tr(Str::PrefsDifficultyAuto),
                      namen[schwierigkeitWirksam(-1, regelwerkMovieDuels())]);
        const char* const stufen[5] = {vorgabe, namen[0], namen[1], namen[2], namen[3]};
        int n = std::clamp(st.schwierigkeit, -1, 3) + 1;
        ImGui::SetNextItemWidth(w * 0.5F);
        if (ImGui::Combo(tr(Str::PrefsDifficulty), &n, stufen, 5)) { schwierigkeitWaehlen(n - 1); }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::PrefsDifficultyNote)); }
    }

    ImGui::Dummy(ImVec2{0, ImGui::GetStyle().ItemSpacing.y});
    ImGui::Separator();
    if (ImGui::Button(tr(Str::EditorOk), ImVec2{ImGui::GetFontSize() * 5.0F, 0.0F})) {
        g_app->prefsOpen = false;
        g_app->prefsHatVorher = false;   // uebernommen
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    // Wie im Original: OK und Cancel. Cancel spielt den Stand vom Oeffnen
    // zurueck (im naechsten Bild, siehe oben) - das X ebenso.
    if (ImGui::Button(tr(Str::EditorCancel), ImVec2{ImGui::GetFontSize() * 5.0F, 0.0F})) {
        g_app->prefsOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// Die Spielordner als EIGENES Fenster.
//
// Sie standen mitten in den Einstellungen, zwischen den Verzeichnissen und
// den Tastenkuerzeln. Wer sie sucht, scrollt daran vorbei - und wer die
// Tasten sucht, scrollt an den Ordnern vorbei. Drei Sachen, drei Fenster.
// --- Was die offenen Skripte miteinander verbindet -----------------------
//
// Seit eine Mission ALLE ihre Skripte oeffnet (rc236), stellt sich die
// Frage, wie sie zusammenspielen. Im OpenJK-Quelltext nachgesehen:
//
//   * Signale liegen in EINER Tabelle auf der Icarus-Instanz, nicht je
//     Skript. Eines sendet, ein anderes wartet - so verzahnt man sie.
//   * Die Kamera ist EINE einzige (camera_t client_camera). Zwei Skripte,
//     die sie gleichzeitig anfassen, ueberschreiben einander.
//
// Diese Uebersicht zeigt beides. Die Auswertung selbst steht im Kern
// (interplay.h) und hat eine eigene Probe - hier wird nur gezeichnet.
void drawInterplay() {
    if (!g_app->interplayOpen) {
        return;
    }
    const std::string t =
        std::string(tr(Str::InterplayTitle)) + "###interplay";
    // Startgroesse und Untergrenze. Ohne beides nahm ImGui seine Vorgabe
    // von rund 160 x 190 Punkten - Titel und Inhalt abgeschnitten. Gemeldet:
    // "manche von den menu fenster sind zu klein".
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 44.0F,
                                    ImGui::GetFontSize() * 26.0F},
                             ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2{ImGui::GetFontSize() * 30.0F, ImGui::GetFontSize() * 14.0F},
        ImVec2{FLT_MAX, FLT_MAX});
    if (!ImGui::Begin(t.c_str(), &g_app->interplayOpen)) {
        ImGui::End();
        return;
    }

    // Alle offenen Skripte einsammeln. Der AKTIVE Reiter hat das lebende
    // Dokument, die anderen ihr geparktes - dieselbe Unterscheidung wie
    // ueberall sonst.
    std::vector<NamedFacts> alle;
    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
        const bool aktiv = (static_cast<int>(i) == g_app->activeTab);
        const std::string nm =
            aktiv ? (g_app->shownName.empty() ? std::string("unnamed.txt")
                                              : g_app->shownName)
                  : (g_app->tabs[i].shownName.empty()
                         ? std::string("unnamed.txt")
                         : g_app->tabs[i].shownName);
        const Script& sc = aktiv ? g_app->doc.script() : g_app->tabs[i].doc.script();
        alle.push_back(NamedFacts{nm, scanScript(sc)});
    }

    if (alle.empty()) {
        ImGui::TextUnformatted(tr(Str::InterplayNoScripts));
        ImGui::End();
        return;
    }

    // --- Die Kamera zuerst: sie ist der haerteste Fall -------------------
    ImGui::SeparatorText(tr(Str::InterplayCamera));
    const std::vector<std::string> kam = cameraScripts(alle);
    if (kam.empty()) {
        ImGui::TextDisabled("-");
    } else {
        for (const std::string& k : kam) {
            ImGui::BulletText("%s", k.c_str());
        }
        if (kam.size() > 1) {
            ImGui::TextColored(ImVec4{1.0F, 0.6F, 0.2F, 1.0F}, "%s",
                               tr(Str::InterplayCameraWarn));
        }
    }

    // --- Und die Signale -------------------------------------------------
    ImGui::SeparatorText(tr(Str::InterplaySignals));
    const std::vector<SignalLink> sig = linkSignals(alle);
    if (sig.empty()) {
        ImGui::TextDisabled("-");
    }
    for (const SignalLink& l : sig) {
        std::string von;
        for (const std::string& x : l.senders) {
            von += (von.empty() ? "" : ", ") + x;
        }
        std::string nach;
        for (const std::string& x : l.waiters) {
            nach += (nach.empty() ? "" : ", ") + x;
        }

        // Die halben Faelle stehen hervorgehoben: ein Signal ohne
        // Empfaenger geht ins Leere, eines ohne Sender laesst ein Skript
        // ewig warten. Beides ist ein Hinweis, kein Urteil - der fehlende
        // Teil kann in einem Skript stehen, das gerade nicht offen ist.
        const bool halb = l.senders.empty() || l.waiters.empty();
        if (halb) {
            ImGui::TextColored(ImVec4{1.0F, 0.6F, 0.2F, 1.0F}, "%s",
                               l.name.c_str());
        } else {
            ImGui::TextUnformatted(l.name.c_str());
        }
        ImGui::SameLine();
        if (l.senders.empty()) {
            ImGui::TextDisabled("- %s", tr(Str::InterplayStuck));
        } else if (l.waiters.empty()) {
            ImGui::TextDisabled("- %s (%s)", tr(Str::InterplayOrphan),
                                von.c_str());
        } else {
            // Der Pfeil ist ein Zeichen, kein Wort - er braucht keine
            // Uebersetzung. Der Pruefer sieht aber nur festen Text im
            // Format, also aus den Namen zusammensetzen.
            const std::string zeile = von + "  ->  " + nach;
            ImGui::TextDisabled("%s", zeile.c_str());
        }
    }

    ImGui::End();
}

void drawGamePaths() {
    if (!g_app->pathsOpen) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 58.0F,
                                    ImGui::GetFontSize() * 12.0F},
                             ImGuiCond_FirstUseEver);
    // Eine Untergrenze, sonst laesst sich das Fenster auf Fingernagelgroesse
    // ziehen und ist dann nicht mehr zu bedienen. Dieselbe Vorsorge wie beim
    // Meldungsfenster, hier hatte ich sie vergessen.
    ImGui::SetNextWindowSizeConstraints(
        ImVec2{ImGui::GetFontSize() * 40.0F, ImGui::GetFontSize() * 16.0F},
        ImVec2{FLT_MAX, FLT_MAX});
    const std::string titel = std::string(tr(Str::PathsTitle)) + "###paths";
    // Lange Pfade: waagerecht rollen statt abschneiden. Bei der Untergrenze
    // von 40 Zeichen war die Zeile 342 Punkte breiter als das Fenster.
    if (!ImGui::Begin(titel.c_str(), &g_app->pathsOpen,
                      ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::End();
        return;
    }
    Settings& st = g_app->settings;
    // rc603: gelten gerade die Spielordner einer geladenen Mission (eines
    // anderen Spiels), steht das hier - mit dem Weg zurueck. Die Liste
    // darunter bleibt die eingerichtete, gespeicherte.
    if (!g_app->sitzungsSpielordner.empty()) {
        ImGui::TextColored(ImVec4{0.90F, 0.75F, 0.35F, 1.0F}, "%s", tr(Str::PathsSessionNote));
        for (const std::string& o : g_app->sitzungsSpielordner) {
            ImGui::BulletText("%s", o.c_str());
        }
        if (ImGui::SmallButton(tr(Str::PathsSessionBack))) {
            setzeSitzungsSpielordner({});
        }
    }
    const bool sitzung = !g_app->sitzungsSpielordner.empty();
    // rc604: zwei Listen - Jedi Academy / Movie Duels und Jedi Outcast. Es
    // gilt die des Regelwerks (eingerichteteSpielordner); "Add folder" legt
    // einen JK2-Ordner von selbst in die zweite (addGamePathDirectory).
    const auto zeichneListe = [&](std::vector<std::string>& liste, bool aktiv) {
        // Die Reihenfolge ist keine Kosmetik.
        //
        // Movie Duels ERSETZT Dateien des Originalspiels unter demselben Namen -
        // models/players/_humanoid/_humanoid.gla gibt es zweimal, mit 21376 und
        // mit 30384 Bildern. Wer die Ordner falsch herum stehen hat, laedt die
        // falsche, und ihm fehlt jede Animation jenseits der 21376. Deshalb
        // steht hier, WER GEWINNT, und deshalb laesst sich die Reihenfolge in
        // beide Richtungen aendern.
        for (std::size_t i = 0; i < liste.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            const bool letzter = (i + 1 == liste.size());

            ImGui::BeginDisabled(i == 0);
            if (ImGui::SmallButton(tr(Str::PrefsUp))) {
                std::swap(liste[i], liste[i - 1]);
                g_app->sitzungsSpielordner.clear();   // rc603
                rescanGamePaths();
                refreshPk3List();
                ImGui::EndDisabled();
                ImGui::PopID();
                break;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            // Nach unten: fehlte bisher. Wer den letzten Ordner nach oben
            // bringen wollte, musste alle anderen einzeln hochschieben.
            ImGui::BeginDisabled(letzter);
            if (ImGui::SmallButton(tr(Str::PrefsDown))) {
                std::swap(liste[i], liste[i + 1]);
                g_app->sitzungsSpielordner.clear();   // rc603
                rescanGamePaths();
                refreshPk3List();
                ImGui::EndDisabled();
                ImGui::PopID();
                break;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::SmallButton(tr(Str::PrefsRemove))) {
                liste.erase(liste.begin() + static_cast<std::ptrdiff_t>(i));
                g_app->sitzungsSpielordner.clear();   // rc603
                rescanGamePaths();
                refreshPk3List();
                ImGui::PopID();
                break;
            }
            ImGui::SameLine();
            // Den Ordner im Explorer zeigen. Ein Pfad in einer Zeile sagt nicht,
            // ob wirklich das drinsteht, was man erwartet - ein Blick hinein
            // schon. openWithDefaultApp oeffnet bei einem Ordner den Explorer.
            if (ImGui::SmallButton(tr(Str::PrefsOpenFolder))) {
                platform::openWithDefaultApp(liste[i]);
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(liste[i].c_str());

            // Was in diesem Ordner gefunden wurde - sonst raet man, ob er stimmt.
            // rc603: nur, wenn die eingerichteten gelten (sonst gehoeren die
            // eingelesenen Ordner zur Sitzung).
            if (aktiv && i < g_app->gamePaths.size()) {
                const GamePath& gp = g_app->gamePaths[i];
                ImGui::SameLine();
                if (gp.archives.empty()) {
                    // Ein Ordner ohne Archive ist fast immer ein Tippfehler im
                    // Pfad. Das soll auffallen.
                    ImGui::TextColored(ImVec4{0.90F, 0.75F, 0.35F, 1.0F}, "%s",
                                       tr(Str::PrefsEmptyPath));
                } else {
                    ImGui::TextDisabled(tr(Str::PrefsArchives),
                                        static_cast<int>(gp.archives.size()),
                                        gp.fileCount);
                }
            }
            // Und wer bei Namensgleichheit gewinnt.
            if (letzter && liste.size() > 1) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4{0.45F, 0.80F, 0.45F, 1.0F}, "- %s",
                                   tr(Str::PrefsWins));
            }
            ImGui::PopID();
        }
    };
    const bool jk2Liste = &eingerichteteSpielordner() == &st.gamePathsJk2;
    ImGui::SeparatorText(tr(Str::PrefsGamePaths));
    ImGui::TextDisabled("%s", tr(Str::PrefsGamePathsNote));
    zeichneListe(st.gamePaths, !sitzung && !jk2Liste);
    ImGui::SeparatorText(tr(Str::PathsJk2));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetFontSize() * 38.0F);
    ImGui::TextDisabled("%s", tr(Str::PathsJk2Note));
    ImGui::PopTextWrapPos();
    ImGui::PushID("jk2");
    zeichneListe(st.gamePathsJk2, !sitzung && jk2Liste);
    ImGui::PopID();
    if (ImGui::Button(tr(Str::Pk3AddPath))) { addGamePath(); }

    ImGui::End();
}

// Welche Zeile gerade auf eine Taste wartet. Nicht in der Funktion, damit
// ein geschlossenes Fenster die Aufnahme sicher beenden kann.
keys::Action g_kuerzelZeile = keys::Action::Unhandled;

// Und die Tastenkuerzel ebenso.
void drawKeyBindings() {
    if (!g_app->keysOpen) {
        // Zu heisst: nichts wartet mehr. Sonst bliebe kuerzelWartet stehen
        // und sperrte alle Kuerzel bis zum Neustart.
        g_kuerzelZeile = keys::Action::Unhandled;
        g_app->kuerzelWartet = false;
        return;
    }
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 42.0F,
                                    ImGui::GetFontSize() * 30.0F},
                             ImGuiCond_FirstUseEver);
    // Eine Untergrenze, sonst laesst sich das Fenster auf Fingernagelgroesse
    // ziehen und ist dann nicht mehr zu bedienen. Dieselbe Vorsorge wie beim
    // Meldungsfenster, hier hatte ich sie vergessen.
    ImGui::SetNextWindowSizeConstraints(
        ImVec2{ImGui::GetFontSize() * 32.0F, ImGui::GetFontSize() * 20.0F},
        ImVec2{FLT_MAX, FLT_MAX});
    const std::string titel = std::string(tr(Str::KeysTitle)) + "###keys";
    if (!ImGui::Begin(titel.c_str(), &g_app->keysOpen)) {
        ImGui::End();
        return;
    }
    // --- Tastenkuerzel ---------------------------------------------------
    //
    // Dear ImGui bringt dafuer nichts Fertiges mit - nachgesehen in imgui.h
    // und in ocornuts Ausgabe 5862, wo genau danach gefragt wird. Was es
    // gibt, sind die Bausteine, und die reichen:
    //
    //   ImGuiKeyChord     Taste und Zusatztasten in EINER Zahl
    //   GetKeyChordName   Beschriftung dafuer
    //   IsKeyChordPressed pruefen, ob sie gedrueckt wurde
    //
    // Zum Aufnehmen der neuen Taste laeuft man ueber die benannten Tasten
    // (ImGuiKey_NamedKey_BEGIN bis _END) und nimmt die erste gedrueckte.
    ImGui::SeparatorText(tr(Str::KeysTitle));
    ImGui::TextWrapped("%s", tr(Str::KeysHint));

    // Welche Zeile wartet gerade auf eine Taste? Nur eine zur Zeit.
    keys::Action& waiting = g_kuerzelZeile;

    if (ImGui::BeginTable("keys", 2,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(tr(Str::KeysAction));
        ImGui::TableSetupColumn(tr(Str::KeysShortcut));
        ImGui::TableHeadersRow();
        for (const keys::Bindable& b : keys::bindable()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(tr(labelFor(b.action)));
            ImGui::TableNextColumn();
            ImGui::PushID(b.name);

            const ImGuiKeyChord now = chordFor(b.action);
            char shown[64];
            if (waiting == b.action) {
                std::snprintf(shown, sizeof(shown), "%s", tr(Str::KeysPress));
            } else if (now == ImGuiKey_None) {
                std::snprintf(shown, sizeof(shown), "%s", tr(Str::KeysNone));
            } else {
                std::snprintf(shown, sizeof(shown), "%s",
                              ImGui::GetKeyChordName(now));
            }
            // Die Kennung haengt an der AKTION, nicht an der Beschriftung
            // ("###k_<name>"): so bleibt der Knopf derselbe, waehrend er
            // "Press a key..." zeigt, und der Selbsttest findet jede Zeile.
            const std::string knopf = std::string(shown) + "###k_" + b.name;
            if (ImGui::Button(knopf.c_str(), ImVec2{ImGui::GetFontSize() * 9.0F, 0.0F})) {
                waiting = (waiting == b.action) ? keys::Action::Unhandled
                                                : b.action;
            }
            // Doppelt belegt? Dann daneben sagen, von wem.
            if (now != ImGuiKey_None && waiting != b.action) {
                for (const keys::Bindable& o : keys::bindable()) {
                    if (o.action == b.action || chordFor(o.action) != now) {
                        continue;
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled(tr(Str::KeysClash), tr(labelFor(o.action)));
                    break;
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // Klick ausserhalb bricht die Aufnahme ab. Vorher lief sie weiter,
    // auch wenn das Fenster verdeckt war - jeder naechste Tastendruck wurde
    // zur neuen Belegung, und die Kuerzel wirkten "kaputt".
    if (waiting != keys::Action::Unhandled &&
        !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        waiting = keys::Action::Unhandled;
    }
    g_app->kuerzelWartet = (waiting != keys::Action::Unhandled);
    if (waiting != keys::Action::Unhandled) {
        // Esc bricht ab, Rueckschritt loescht die Belegung.
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            waiting = keys::Action::Unhandled;
        } else if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
            setChord(waiting, ImGuiKey_None);
            waiting = keys::Action::Unhandled;
        } else {
            for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
                const auto key = static_cast<ImGuiKey>(k);
                // Zusatztasten allein sind keine Belegung.
                //
                // AUCH die Sammeltasten ImGuiKey_ReservedForModCtrl & Co.:
                // sie liegen im Bereich der benannten Tasten und gelten als
                // gedrueckt, sobald Strg gedrueckt ist. Gemeldet: "I'm
                // trying to change the find keyboard shortcut to CTRL + F
                // but I can't" - aufgenommen wurde "Ctrl+ModCtrl", bevor
                // das F ueberhaupt kam. Maustasten ebenso wenig: der Klick
                // auf den Knopf selbst ist keine Belegung.
                if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl ||
                    key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift ||
                    key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt ||
                    key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper ||
                    key == ImGuiKey_ReservedForModCtrl ||
                    key == ImGuiKey_ReservedForModShift ||
                    key == ImGuiKey_ReservedForModAlt ||
                    key == ImGuiKey_ReservedForModSuper ||
                    (key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseWheelY)) {
                    continue;
                }
                if (!ImGui::IsKeyPressed(key, false)) {
                    continue;
                }
                ImGuiKeyChord c = key;
                if (ImGui::GetIO().KeyCtrl) { c |= ImGuiMod_Ctrl; }
                if (ImGui::GetIO().KeyAlt) { c |= ImGuiMod_Alt; }
                if (ImGui::GetIO().KeyShift) { c |= ImGuiMod_Shift; }
                setChord(waiting, c);
                waiting = keys::Action::Unhandled;
                break;
            }
        }
    }

    if (ImGui::Button(tr(Str::KeysReset))) {
        g_app->settings.keyBindings.clear();
        waiting = keys::Action::Unhandled;
        g_app->kuerzelWartet = false;
    }

    ImGui::End();
}

// AUSSERHALB des namenlosen Namensraums - der Selbsttest ruft sie
// (lint_link.py).
}  // namespace

// Ein Modell aus den Archiven oeffnen - fuer den pk3-Browser und den
// Selbsttest (BHED_MODELL). Gibt false zurueck, wenn es sich nicht lesen
// liess; `fehler` sagt dann, warum.
bool oeffneModellAusArchiv(const std::string& name, std::string* fehler) {
    std::string data;
    if (!readFromArchives(name, data)) {
        if (fehler != nullptr) { *fehler = "nicht in den Archiven"; }
        return false;
    }
    diag::Step ms("Modell laden: " + name);
    GlmModel gm;
    std::string gerr;
    if (!readGlm(data, gm, &gerr)) {
        ms.fail(gerr);
        if (fehler != nullptr) { *fehler = gerr; }
        return false;
    }
    {
        const std::string dir = directoryOf(name);
        // ALLE Haeute im selben Ordner sammeln, nicht nur die
        // Vorgabe.
        //
        // Vorher geschah das nur beim Laden von der PLATTE -
        // aus einem Archiv blieb die Liste leer, und die
        // Auswahl fehlte ganz. Genau der haeufigere Weg.
        g_app->modelSkins.clear();
        g_app->modelSkinIndex = -1;
        g_app->modelDir = dir;
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& sf : findByExtension(gp, {".skin"})) {
                if (directoryOf(sf.name) != dir) {
                    continue;
                }
                const std::string base = fileName(sf.name);
                if (std::find(g_app->modelSkins.begin(),
                              g_app->modelSkins.end(), base) ==
                    g_app->modelSkins.end()) {
                    g_app->modelSkins.push_back(base);
                }
            }
        }
        std::sort(g_app->modelSkins.begin(), g_app->modelSkins.end());
        for (const char* which : {"model_default.skin", "model.skin"}) {
            std::string skin;
            if (readFromArchives(dir + "/" + which, skin)) {
                applySkin(skin, gm);
                break;
            }
        }
        g_app->model = std::move(gm);
        g_app->modelPath = name;
        // Kamera vor die Figur, auf halbe Hoehe.
        for (int k = 0; k < 3; ++k) {
            g_app->modelCam.pos[k] =
                (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F;
        }
        float size = 0.0F;
        for (int k = 0; k < 3; ++k) {
            size = std::max(size,
                            g_app->model.maxs[k] - g_app->model.mins[k]);
        }
        g_app->modelOrbit.distance = std::max(size * 1.15F, 24.0F);
        g_app->modelCam.angles[0] = 0.0F;
        g_app->modelCam.angles[1] = 180.0F;
        {
            // "vorn" statt "f": weiter oben steht schon ein
            // f fuer die gefundene Datei.
            float vorn[3];
            g_app->modelCam.forward(vorn);
            for (int k = 0; k < 3; ++k) {
                g_app->modelCam.pos[k] =
                    (g_app->model.mins[k] +
                     g_app->model.maxs[k]) * 0.5F -
                    vorn[k] * g_app->modelOrbit.distance;
            }
        }
        g_app->anim = GlaAnimation{};
        g_app->animList.clear();
        g_app->animIndex = -1;
        g_app->modelDirty = true;
        g_app->leftMode = 2;
        loadModelTextures();
        // Das Skelett gleich mitsuchen - der Pfad steht im
        // Modell.
        loadSkeletonForModel();
    }
    return true;
}

namespace {

void drawPk3Browser() {
    if (!g_app->pk3Open) {
        return;
    }
    ImGui::OpenPopup(tr(Str::Pk3Title));
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 40.0F,
                                    ImGui::GetFontSize() * 26.0F},
                             ImGuiCond_FirstUseEver);
    // Untergrenze: ohne sie liess sich der Browser auf 48 x 48 Punkte
    // zusammenziehen (Fenstertest, "laesst sich nicht zu klein ziehen").
    ImGui::SetNextWindowSizeConstraints(
        ImVec2{ImGui::GetFontSize() * 36.0F, ImGui::GetFontSize() * 18.0F},
        ImVec2{FLT_MAX, FLT_MAX});
    if (!ImGui::BeginPopupModal(tr(Str::Pk3Title), &g_app->pk3Open)) {
        return;
    }

    if (ImGui::Button(tr(Str::Pk3AddPath))) { addGamePath(); }
    ImGui::SameLine();
    for (int k = 0; k < 3; ++k) {
        const Str label = (k == 0) ? Str::Pk3Scripts
                        : (k == 1) ? Str::Pk3Maps
                                   : Str::Pk3Models;
        if (ImGui::RadioButton(tr(label), g_app->pk3Kind == k)) {
            g_app->pk3Kind = k;
            refreshPk3List();
        }
        if (k < 2) {
            ImGui::SameLine();
        }
    }

    // Archivfilter. Nur der Dateiname, nicht der ganze Pfad - sonst ist die
    // Liste breiter als das Fenster.
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0F);
    const std::string shown = g_app->pk3Archive.empty()
                                  ? std::string(tr(Str::Pk3AllArchives))
                                  : fileName(g_app->pk3Archive);
    if (ImGui::BeginCombo(tr(Str::Pk3Archive), shown.c_str())) {
        if (ImGui::Selectable(tr(Str::Pk3AllArchives), g_app->pk3Archive.empty())) {
            g_app->pk3Archive.clear();
            refreshPk3List();
        }
        for (const GamePath& gp : g_app->gamePaths) {
            for (const Pk3& a : gp.archives) {
                const std::string base = fileName(a.path);
                const bool sel = (g_app->pk3Archive == a.path);
                if (ImGui::Selectable(base.c_str(), sel)) {
                    g_app->pk3Archive = a.path;
                    refreshPk3List();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(a.path.c_str());
                    ImGui::TextDisabled(tr(Str::Pk3ArchiveInfo),
                                        static_cast<int>(a.entries.size()));
                    ImGui::EndTooltip();
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();

    int archives = 0;
    for (const GamePath& gp : g_app->gamePaths) {
        archives += static_cast<int>(gp.archives.size());
    }
    ImGui::Text(tr(Str::Pk3Found), archives, static_cast<int>(g_app->pk3Files.size()));
    if (g_app->gamePaths.empty()) {
        ImGui::TextDisabled("%s", tr(Str::Pk3None));
    }

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0F);
    ImGui::InputText(tr(Str::Pk3Filter), g_app->pk3Filter, sizeof(g_app->pk3Filter));

    // Bei Yavin2 sind es 370 Zielnamen und hier schnell tausende Dateien -
    // ohne Filter findet man nichts.
    std::string needle = g_app->pk3Filter;
    for (char& c : needle) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    ImGui::BeginChild("list", ImVec2{0, -ImGui::GetFrameHeightWithSpacing()},
                      ImGuiChildFlags_Borders);
    for (std::size_t fi = 0; fi < g_app->pk3Files.size(); ++fi) {
        const FoundFile& f = g_app->pk3Files[fi];
        if (!needle.empty()) {
            std::string low = f.name;
            for (char& c : low) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (low.find(needle) == std::string::npos) {
                continue;
            }
        }
        // Die STELLE als Kennung, nicht den Namen: dieselbe Datei kann in
        // mehreren Archiven liegen (models/players/kyle/model.glm steckt in
        // assets1.pk3 UND in einem Mod-Archiv). Gleiche Namen ergaeben
        // gleiche Kennungen, und ImGui meldet zu Recht einen Zusammenstoss.
        ImGui::PushID(static_cast<int>(fi));
        if (ImGui::Selectable(f.name.c_str(), false,
                              ImGuiSelectableFlags_AllowDoubleClick) &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            std::string data;
            if (readFromArchives(f.name, data)) {
                if (g_app->pk3Kind == 2) {
                    // Ein Modell. Die .skin dazu suchen wir gleich mit: sie
                    // liegt neben dem Modell und heisst model_<name>.skin.
                    std::string gerr;
                    if (!oeffneModellAusArchiv(f.name, &gerr)) {
                        platform::showError(f.name + "\n" + gerr, tr(Str::AppTitle));
                    }
                } else if (g_app->pk3Kind == 1) {
                    // Derselbe Weg wie beim Oeffnen von der Platte:
                    // Entities, Geometrie und Texturen.
                    diag::Step mapStep("Karte aus Archiv: " + f.name);
                    vergissEntities(f.name);
                    loadMapFromBytes(data, f.name);
                    // Eine Karte im Archiv hat keinen Pfad auf der Platte;
                    // beim naechsten Start liesse sie sich so nicht wieder
                    // finden. Deshalb nicht merken.
                    g_app->settings.mapPath.clear();
                } else {
                    const std::string dat = data;
                    const std::string nm = f.name;
                    withUnsaved([dat, nm] {
                        // .ibi kommt als Binaerdatei - erst zurueckuebersetzen.
                        oeffneSkriptAusSpeicherI(dat, nm);
                    });
                }
                g_app->pk3Open = false;
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(tr(Str::Pk3FromArchive), f.archive.c_str());
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    if (ImGui::Button(tr(Str::EditorCancel))) {
        g_app->pk3Open = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// --- Changelog --------------------------------------------------------
//
// shank (11.10.): "ein Changelog-Fenster ... dass die Leute wissen, was wir
// gefixt haben, ueber die Zeit hinweg, auch in jeder Sprache, die ich
// anbiete." Die Eintraege stehen in tools/changelog.txt (vier Sprachen je
// Fassung) und werden mit tools/gen_changelog.py in die exe gebaut - ein
// fehlendes data/-Verzeichnis kann sie nicht verlieren.
void drawChangelog() {
    if (!g_app->changelogOpen) {
        return;
    }
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{std::min(ImGui::GetFontSize() * 40.0F, vp->WorkSize.x * 0.9F),
                                    std::min(ImGui::GetFontSize() * 30.0F, vp->WorkSize.y * 0.85F)},
                             ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    bool offen = true;
    if (ImGui::Begin((std::string(tr(Str::ChangelogTitle)) + "###changelog").c_str(), &offen,
                     ImGuiWindowFlags_NoCollapse)) {
        char kopf[300];
        std::snprintf(kopf, sizeof(kopf), tr(Str::ChangelogIntro), kFassung);
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", kopf);
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        const int sprache = static_cast<int>(i18n::currentLanguage());
        const float knopfH = ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("##eintraege", ImVec2{0.0F, -knopfH})) {
            const changelog::Eintrag* e = changelog::eintraege();
            for (std::size_t i = 0; i < changelog::anzahl(); ++i) {
                const bool installiert = std::strcmp(e[i].fassung, kFassung) == 0;
                std::string titel = std::string(e[i].fassung) + "  -  " + e[i].datum;
                if (installiert) { titel += std::string("  (") + tr(Str::ChangelogInstalled) + ")"; }
                titel += "###v" + std::to_string(i);
                // Die neuesten zwei offen, der Rest zugeklappt.
                ImGui::SetNextItemOpen(i < 2, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader(titel.c_str())) {
                    const char* t = e[i].text[(sprache >= 0 && sprache < 4) ? sprache : 0];
                    ImGui::PushTextWrapPos(0.0F);
                    for (const char* a = t; *a != '\0';) {
                        const char* ende = std::strchr(a, '\n');
                        const std::string punkt = ende != nullptr ? std::string(a, ende) : std::string(a);
                        ImGui::Bullet();
                        ImGui::TextWrapped("%s", punkt.c_str());
                        if (ende == nullptr) { break; }
                        a = ende + 1;
                    }
                    ImGui::PopTextWrapPos();
                    ImGui::Spacing();
                }
            }
        }
        ImGui::EndChild();
        if (ImGui::Button(tr(Str::ChangelogClose))) { offen = false; }
    }
    ImGui::End();
    if (!offen) { g_app->changelogOpen = false; }
}

}  // namespace

// Das Handbuch (rc599): handbuch/<sprache>/index.html neben behaved.exe -
// beim Entwickeln auch eine Ebene hoeher. Geoeffnet im Standardbrowser.
void handbuchOeffnen(const char* seite) {
    // Im Selbsttest nie einen Browser oeffnen - der Tastentest drueckt jede
    // Belegung, auch F1 (rc599).
    if (std::getenv("BHED_EDITORTEST") != nullptr) {
        addStatus(std::string("(Selbsttest) Handbuch angefragt: ") + seite);
        return;
    }
    const char* code = "en";
    switch (i18n::currentLanguage()) {
        case i18n::Language::German: code = "de"; break;
        case i18n::Language::ChineseSimplified: code = "zh"; break;
        case i18n::Language::Japanese: code = "ja"; break;
        default: break;
    }
    const std::string exe = platform::executableDirectory();
    for (const std::string& basis : {exe, exe + "\\.."}) {
        const std::string datei = basis + "\\handbuch\\" + code + "\\" + seite;
        std::error_code ec;
        if (std::filesystem::exists(::bhed::utf8Pfad(datei), ec)) {
            platform::openWithDefaultApp(datei);
            return;
        }
    }
    char t[700];
    std::snprintf(t, sizeof(t), tr(Str::GuideMissing), (exe + "\\handbuch").c_str());
    addStatus(t);
}

namespace {

void drawAbout() {
    if (!g_app->aboutOpen) {
        return;
    }
    ImGui::OpenPopup(tr(Str::AboutTitle));
    if (!ImGui::BeginPopupModal(tr(Str::AboutTitle), &g_app->aboutOpen,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    // Stufe 8c: das Logo von JK Studio (shanks Entwurf), in der Groesse der
    // Schrift mitwachsend; darunter Name, Fassung, Untertitel und die Bereiche.
    {
        int lb = 0;
        int lh = 0;
        if (void* jk = jkLogoTextureId(lb, lh); jk != nullptr && lh > 0) {
            const float h = ImGui::GetFontSize() * 10.0F;
            ImGui::Image(reinterpret_cast<ImTextureID>(jk), ImVec2{h * static_cast<float>(lb) / static_cast<float>(lh), h});
        }
    }
    ImGui::Text("%s 1.0.0-%s", tr(Str::AppTitle), kFassung);
    ImGui::TextUnformatted(tr(Str::AboutSubtitle));
    ImGui::TextUnformatted(tr(Str::AboutModules));
    ImGui::Separator();
    // Das Logo aus BITMAP 133 des Originals (der Bereich BehavEd ist sein Nachbau),
    // in der Groesse der Schrift mitwachsend.
    if (void* logo = logoTextureId(); logo != nullptr) {
        const float scale = ImGui::GetFontSize() / 16.0F;
        ImGui::Image(reinterpret_cast<ImTextureID>(logo),
                     ImVec2{static_cast<float>(icons::kLogoWidth) * scale,
                            static_cast<float>(icons::kLogoHeight) * scale});
    }
    ImGui::TextUnformatted(tr(Str::AboutOriginal));
    ImGui::Separator();

    // Woran das Modell gemessen ist. Das gehoert hierher, weil es die
    // Frage beantwortet, der man einem Nachbau sonst nicht ansieht:
    // stimmt er ueberhaupt?
    ImGui::TextUnformatted(tr(Str::AboutVerified));
    ImGui::BulletText(tr(Str::AboutModel),
                      static_cast<int>(g_app->db.commands.size()),
                      static_cast<int>(g_app->db.typesets.size()),
                      static_cast<int>(g_app->db.macros.size()));
    ImGui::BulletText("%s", tr(Str::AboutResources));
    ImGui::BulletText("%s", tr(Str::AboutIbi));
    ImGui::BulletText("%s", tr(Str::AboutIcarus));

    ImGui::Separator();
    ImGui::TextDisabled("%s", tr(Str::AboutNotAffiliated));
    if (ImGui::Button(tr(Str::AboutClose), ImVec2{ImGui::GetFontSize() * 6.0F, 0.0F})) {
        g_app->aboutOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::ChangelogMenu))) {
        g_app->aboutOpen = false;
        g_app->changelogOpen = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::AboutGuide))) { handbuchOeffnen(); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::GuideHint)); }
    ImGui::EndPopup();
}

// --- Hauptfenster ------------------------------------------------------

// --- Dateiverwaltung ---------------------------------------------------

// Movie Duels legt seine Skripte als .txt ab - so heisst die Datei auch im
// Bildschirmfoto ("intro_jedi.txt"). Beide Endungen anbieten.
extern const char* kScriptFilter;
// .ibi gehoert dazu.
//
// Gelesen werden sie laengst - openScriptFromMemory erkennt sie am INHALT
// ("IBI" am Anfang), nicht an der Endung, weil sie in Movie Duels mal .IBI
// und mal .ibi heissen. Nur im Oeffnen-Dialog standen sie nicht, und damit
// waren sie ueber "Open" nicht erreichbar: gemeldet als "Can only open .txt
// and .icarus file".
//
// Eine eigene Zeile fuer .ibi zusaetzlich zur gemeinsamen, damit man im
// Dialog gezielt nur die kompilierten sehen kann.
// Es gibt genau ZWEI Arten, und .icarus gehoert nicht dazu.
//
// Vom Anwender richtiggestellt: ".ibi = uebersetztes Skript, das das Spiel
// liest. .txt = so sichert man sein Skript, und behaved kann es wieder
// oeffnen." Mehr nicht.
//
// ".icarus" stand hier als vermeintlicher Name der Sprache - die Sprache
// heisst so, die Dateien nicht. Das Original bot ausserdem .py an; das
// benutzt niemand, und es steht deshalb nur im gemeinsamen Filter.
const char* kScriptFilter =
    "ICARUS scripts (*.txt;*.ibi)|*.txt;*.ibi|"
    "Script source (*.txt)|*.txt|"
    "Compiled scripts (*.ibi)|*.ibi|"
    "All files (*.*)|*.*";




// Die Beschreibung eines Befehls - mit Ersatz, wo die .bhc schweigt.
//
// Vier Befehle haben dort keine: camera, task, do und play. Das ist eine
// Luecke in Ravens Datei, kein Fehler bei uns - das Original zeigt ebenso
// nichts. Weil aber die anderen fuenfundzwanzig eine haben, sieht es nach
// einem Fehler aus, und gerade bei camera erfaehrt man nichts ueber den
// wichtigsten Befehl im Skript.
//
// Was in der .bhc steht, hat immer Vorrang. Der Ersatz greift nur bei
// Leere - eine spaetere .bhc mit eigenen Texten setzt sich also durch.
std::string descFor(const Command& c) {
    if (!c.desc.empty()) {
        return c.desc;
    }
    if (c.name == "camera") { return tr(Str::DescCamera); }
    if (c.name == "task") { return tr(Str::DescTask); }
    if (c.name == "do") { return tr(Str::DescDo); }
    if (c.name == "play") { return tr(Str::DescPlay); }
    return std::string();
}

bool spew(const std::string& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(f);
}

// Eine Zeile in die Statusliste, mit laufender Nummer wie im Original.


bool doSaveAs();

// --- Speichern schreibt die QUELLE, Uebersetzen das Ergebnis -----------
//
// So macht es BehavEd, und so will es shank:
//
//     Speichern   -> .txt   (der lesbare Quelltext)
//     Uebersetzen -> .ibi   UND die .txt dazu
//
// Bis rc458 schrieb `doSave` den Quelltext in `g_app->path` - was immer
// dort stand. Wer eine .ibi geoeffnet hatte, bekam den TEXT in einer Datei
// namens .ibi geschrieben: das Ergebnis war weg, und die Quelle hiess
// falsch. shank: "when I do save as to save my script as a .txt file, it
// doesn't work. It only saves as a .ibi file."
//
// Die .txt wird nicht mehr gebraucht - dieses Programm liest .ibi
// unmittelbar -, aber sie ist im Editor die einzige Fassung, die man in
// einem Texteditor ansehen und aendern kann. Deshalb bleibt sie.
std::string mitEndung(const std::string& pfad, const char* neu) {
    const std::size_t punkt = pfad.find_last_of('.');
    const std::size_t schraeg = pfad.find_last_of("/\\");
    // Ein Punkt im Verzeichnisnamen ist keine Endung.
    const bool hatEndung =
        (punkt != std::string::npos) &&
        (schraeg == std::string::npos || punkt > schraeg);
    return (hatEndung ? pfad.substr(0, punkt) : pfad) + neu;
}

// --- Rollende Sicherung der letzten 10 Speicherungen ----------------------
//
// shank, 02.10.: "Might be a good idea to have a backup of the .txt file
// after each save ... a rolling backup system of the last 10 saves/compiles
// named backup1.txt, backup2.txt etc." - neben der .exe, in
// backup/<Skriptname>/. backup1.txt ist die neueste. Gleicher Inhalt wie die
// letzte Sicherung (Kompilieren ohne Aenderung) gibt keine neue.
//
// Zwei Skripte mit gleichem Namen aus verschiedenen Ordnern (intro.txt gibt
// es in fast jedem Missionsordner) teilten sich bisher EINEN Ordner - das
// eine schob die Sicherungen des anderen hinaus (Code-Pruefung 03.10.). Jetzt
// merkt sich jeder Ordner in quelle.txt, wem er gehoert; ein anderes Skript
// gleichen Namens bekommt <Name>~2, ~3 ... Ein Ordner ohne quelle.txt (von
// vor rc582) gehoert dem ersten, der wieder speichert.
const char* const kSicherungsMarke = "/quelle.txt";

std::string sicherungsOrdner(const std::string& quelle) {
    std::string name = fileName(quelle);
    const std::size_t punkt = name.find_last_of('.');
    if (punkt != std::string::npos && punkt > 0) { name.resize(punkt); }
    if (name.empty()) { name = "unnamed"; }
    const std::string basis = platform::executableDirectory() + "/backup/" + name;
    const std::string schluessel = pfadSchluessel(quelle);
    for (int n = 1; n <= 99; ++n) {
        const std::string ordner = n == 1 ? basis : basis + "~" + std::to_string(n);
        std::error_code ec;
        if (!std::filesystem::exists(::bhed::utf8Pfad(ordner), ec)) {
            return ordner;
        }
        const std::string marke = ordner + kSicherungsMarke;
        if (!std::filesystem::exists(::bhed::utf8Pfad(marke), ec)) {
            return ordner;
        }
        if (pfadSchluessel(slurp(marke)) == schluessel) {
            return ordner;
        }
    }
    return basis;
}

void sicherungAnlegen(const std::string& quelle, const std::string& inhalt) {
    namespace fs = std::filesystem;
    constexpr int kAnzahl = 10;
    std::error_code ec;
    const std::string ordner = sicherungsOrdner(quelle);
    fs::create_directories(::bhed::utf8Pfad(ordner), ec);
    if (!fs::exists(::bhed::utf8Pfad(ordner + kSicherungsMarke), ec)) {
        (void)spew(ordner + kSicherungsMarke, quelle);
    }
    const auto datei = [&](int n) { return ordner + "/backup" + std::to_string(n) + ".txt"; };
    if (fs::exists(::bhed::utf8Pfad(datei(1)), ec) && slurp(datei(1)) == inhalt) {
        return;
    }
    fs::remove(::bhed::utf8Pfad(datei(kAnzahl)), ec);
    for (int n = kAnzahl - 1; n >= 1; --n) {
        if (fs::exists(::bhed::utf8Pfad(datei(n)), ec)) {
            fs::rename(::bhed::utf8Pfad(datei(n)), ::bhed::utf8Pfad(datei(n + 1)), ec);
        }
    }
    if (!spew(datei(1), inhalt)) {
        diag::detail("Sicherung: " + datei(1) + " nicht schreibbar");
        return;
    }
    diag::detail("Sicherung: " + datei(1));
}

void sicherungsOrdnerOeffnen() {
    const std::string o = g_app->path.empty() ? platform::executableDirectory() + "/backup"
                                              : sicherungsOrdner(g_app->path);
    std::error_code ec;
    std::filesystem::create_directories(::bhed::utf8Pfad(o), ec);
    platform::openInExplorer(o);
}

bool doSave() {
    if (g_app->path.empty()) {
        return doSaveAs();
    }
    // IMMER als .txt. Auch wenn eine .ibi geoeffnet war - dann entsteht die
    // Quelle daneben, mit demselben Namen.
    const std::string quelle = mitEndung(g_app->path, ".txt");
    // Schreibgeschuetzt? Das Original bietet an, den Schutz aufzuheben:
    // "The file "%s" is write-protected ... Do you want me to
    // un-writeprotect it so you can save over it? ('No' will abort the
    // save)". behaved meldete nur einen Fehler.
    {
        std::error_code ec;
        const std::filesystem::path fp = ::bhed::utf8Pfad(quelle);
        if (std::filesystem::exists(fp, ec) &&
            (std::filesystem::status(fp, ec).permissions() &
             std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
            char text[900];
            std::snprintf(text, sizeof(text), tr(Str::AskUnprotect), quelle.c_str());
            // Den Reiter ueber seinen PFAD wiederfinden, nicht "den aktiven":
            // bei Save all ist bis zur Antwort laengst ein anderer vorn, und
            // "Ja" sicherte dann diesen (ohne Pfad: Save-As-Dialog) statt der
            // geschuetzten Datei (Code-Pruefung 03.10.).
            const std::string reiterPfad = g_app->path;
            frage(text, [fp, reiterPfad] {
                std::error_code ec2;
                std::filesystem::permissions(fp, std::filesystem::perms::owner_write,
                                             std::filesystem::perm_options::add, ec2);
                if (ec2) {
                    addStatus("Failed to remove write protect, aborting...");
                    return;
                }
                const int reiter = reiterMitPfadI(reiterPfad);
                if (reiter < 0) {
                    addStatus("(The tab was closed, aborting save)");
                    return;
                }
                if (speichereReiterI(reiter)) {
                    addStatus(tr(Str::MsgSaved));
                }
            }, [] { addStatus("(File was not write-enabled, aborting save)"); });
            return false;
        }
    }
    const std::string inhalt = writeScript(g_app->doc.script());
    if (!spew(quelle, inhalt)) {
        platform::showError(quelle, tr(Str::AppTitle));
        return false;
    }
    sicherungAnlegen(quelle, inhalt);
    // Eigene ROFF-Bahnen (rc599): die .rof-Dateien entstehen mit dem Skript.
    (void)roffDateienSchreiben(quelle, g_app->doc.script());
    g_app->doc.offeneBloeckeGeschlossen();   // die Datei hat jetzt ihre '}'
    // Ab jetzt ist die Quelle die Datei, an der gearbeitet wird. Sonst
    // schriebe das naechste Speichern wieder neben die .ibi.
    if (quelle != g_app->path) {
        g_app->path = quelle;
        g_app->settings.noteRecent(quelle);
    }
    g_app->doc.markSaved();
    // Aenderungsrand: was bis hierher orange war, wird gruen. Und die
    // Lesezeichen gelten ab jetzt fuer diese Fassung der Datei.
    aenderungenGespeichert();
    lesezeichenMerken();
    return true;
}

// Das eigentliche "Save As" ohne den Dialog - der Selbsttest ruft es mit
// einem festen Pfad (ein Windows-Dialog wuerde den Lauf anhalten).
//
// Zwei Regeln (Code-Pruefung 03.10., beide im Selbsttest nachgestellt):
//  - Ist die Datei in einem ANDEREN Reiter offen, nicht darueber schreiben.
//    Sonst standen zwei Reiter mit demselben Pfad da, und der naechste Save
//    im anderen ueberschrieb diese Arbeit wieder. Gespeichert wird als .txt -
//    also auch die .txt-Fassung des Namens pruefen.
//  - Der neue Pfad gilt erst, wenn das Speichern geklappt hat. Vorher stand
//    nach einem gescheiterten Save As der Reiter auf einer Datei, die es
//    nicht gibt, und sie stand in der Liste der zuletzt geoeffneten.
bool speichernUnterPfadI(const std::string& p) {
    for (const std::string& ziel : {p, mitEndung(p, ".txt")}) {
        const int offen = reiterMitPfadI(ziel);
        if (offen >= 0 && offen != g_app->activeTab) {
            platform::showError("\"" + ziel + "\" is open in another tab.\n"
                                "Close it there first, or choose a different name.",
                                tr(Str::AppTitle));
            return false;
        }
    }
    const std::string vorher = g_app->path;
    g_app->path = p;
    if (doSave()) {
        g_app->settings.noteRecent(g_app->path);
        return true;
    }
    // Fragt doSave gerade nach dem Schreibschutz, gehoert der neue Name zu
    // dieser Frage - "Ja" speichert unter ihm.
    if (!g_app->frageOffen) {
        g_app->path = vorher;
    }
    return false;
}

bool doSaveAs() {
    const std::string p = platform::saveFileDialog(
        tr(Str::FileSaveAs), kScriptFilter,
        g_app->path.empty() ? g_app->settings.scriptPath : directoryOf(g_app->path),
        g_app->path.empty() ? "unnamed.txt" : fileName(g_app->path));
    if (p.empty()) {
        return false;
    }
    return speichernUnterPfadI(p);
}


// Die Karte wegraeumen.
//
// Herausgezogen aus dem Menueeintrag "Karte schliessen", weil es jetzt
// einen zweiten Aufrufer gibt: den Reiterwechsel. Ein Reiter ohne Karte
// soll auch keine zeigen.
//
// Die Zuordnung effectShaders zeigt in textures.byShader; wird die Liste
// geleert, sind die Nummern wertlos, und ein Partikel truege die Textur
// einer Wand. Deshalb muss sie MIT weg.
void clearMap() {
    vergissEffektBahnen();
    g_app->map = MapData{};
    g_app->geo = BspGeometry{};
    g_app->mesh = BspMesh{};
    g_app->textures = TextureSet{};
    g_app->grasStufen.clear();   // rc604 surfaceSprites
    g_app->grasNetz = BspMesh{};
    ++g_app->grasStand;
    gpu::vergissKarte();
    g_app->effectShaders.clear();
    g_app->effectRunners.clear();
    g_app->moverSim = MoverSim{};
    g_app->moverKlaenge.clear();
    g_app->effects.clear();
    g_app->mapModels.clear();
    g_app->nebenModelle.clear();
    g_app->brushMeshes.clear();
    g_app->md3Files.clear();
    g_app->md3Meshes.clear();
    g_app->settings.mapPath.clear();
    // Alles, was an der Karte hing, mit weg: Klickziele, Auswahl (als
    // Index!), Kamera- und Schluesselwahl, die GPU-Ueberlagerung. Ohne
    // Karte zeichnet die Ansicht nur den Platzhalter, also wurde nichts
    // davon mehr ueberschrieben (Kartentest 27.09.).
    g_app->markers.clear();
    g_app->keyMarks.clear();
    g_app->actorMarks.clear();
    g_app->cameraSx = -1.0F;
    g_app->cameraSy = -1.0F;
    g_app->pickedEntity = -1;
    g_app->hiddenEntities.clear();
    g_app->cameraSelected = false;
    g_app->selectedKey = -1;
    g_app->mapBereit = false;
    g_app->gizmoZeigen = false;
    g_app->camTrackValid = false;
    g_app->mapDirty = true;
}

// --- Reiter: parken, holen, umschalten -----------------------------------
//
// parkActive() legt das lebende Dokument in seinen Reiter zurueck,
// takeTab() holt eines heraus. Dazwischen wird der Baum neu gebaut, weil
// rows/selected daran haengen.
void parkActive(bool layoutMerken = true) {
    if (g_app->tabs.empty()) {
        return;
    }
    const std::size_t at = static_cast<std::size_t>(
        std::clamp(g_app->activeTab, 0,
                   static_cast<int>(g_app->tabs.size()) - 1));
    App::Parked& t = g_app->tabs[at];
    t.doc = g_app->doc;
    t.path = g_app->path;
    t.selection = g_app->selection;
    t.selectedPath = g_app->selectedPath;
    t.selected = g_app->selected;
    t.expanded = g_app->expanded;
    t.shownName = g_app->shownName;
    t.skriptPfad = g_app->skriptPfad;
    // Die Karte nur merken, wenn sie IHM gehoert - siehe ownMap.
    if (t.ownMap) {
        t.mapPath = g_app->settings.mapPath;
        t.mapId = g_app->map.path;
        t.spielordner = g_app->sitzungsSpielordner;   // rc604
        t.spielordnerGemerkt = true;
    }
    // Die Sicht mitparken.
    for (int k = 0; k < 3; ++k) {
        t.camPos[k] = g_app->cam.pos[k];
        t.camAng[k] = g_app->cam.angles[k];
    }
    t.playMs = g_app->playMs;
    t.camSaved = true;
    // Die Feldaufteilung gehoert dem Reiter - aber NUR beim echten
    // Reiterwechsel. Der Fokuswechsel zwischen Feldern parkt auch, und
    // dabei darf sich die Aufteilung nicht bewegen: sonst klappte ein
    // Klick ins Nachbarfeld die Ansicht um.
    if (layoutMerken) {
        // In den BESITZER, nicht in den aktiven Reiter.
        //
        // Sonst bekommt jeder Reiter, in dem man nur kurz gearbeitet hat,
        // die Aufteilung angehaengt - und ploetzlich sind alle vier
        // geteilt, obwohl nur einer es sein sollte.
        const int besitzer =
            std::clamp(g_app->homeTab, 0,
                       static_cast<int>(g_app->tabs.size()) - 1);
        App::Parked& h = g_app->tabs[static_cast<std::size_t>(besitzer)];
        h.splitCount = g_app->splitCount;
        h.splitFocus = g_app->focusPane;
        for (int i = 0; i < App::kMaxSplit; ++i) {
            h.splitTabs[i] = (i == g_app->focusPane)
                                 ? g_app->activeTab
                                 : g_app->splitPanes[i].tab;
        }
    }
}

void takeTab(int index, bool rollen = true, bool layout = true) {
    if (g_app->tabs.empty()) {
        return;
    }
    const int n = static_cast<int>(g_app->tabs.size());
    g_app->activeTab = std::clamp(index, 0, n - 1);
    App::Parked& t =
        g_app->tabs[static_cast<std::size_t>(g_app->activeTab)];
    g_app->doc = t.doc;
    g_app->path = t.path;
    g_app->selection = t.selection;
    g_app->selectedPath = t.selectedPath;
    g_app->selected = t.selected;
    g_app->expanded = t.expanded;
    g_app->shownName = t.shownName;
    g_app->skriptPfad = t.skriptPfad;

    // Die Aufteilung DIESES Reiters uebernehmen. Ein Reiter, der nie
    // geteilt war, bringt EIN Feld mit - genau das Gewuenschte beim
    // Wechsel "in den nebenan".
    if (layout) {
        const int n2 = static_cast<int>(g_app->tabs.size());
        g_app->splitCount = std::clamp(t.splitCount, 1, App::kMaxSplit);
        g_app->focusPane =
            std::clamp(t.splitFocus, 0, g_app->splitCount - 1);
        for (int i = 0; i < App::kMaxSplit; ++i) {
            g_app->splitPanes[i].tab =
                std::clamp(t.splitTabs[i], 0, std::max(0, n2 - 1));
            g_app->splitPanes[i].dirty = true;
        }
    }

    // Die Karte nachziehen - aber nur, wenn es eine andere ist.
    //
    // Beim Laden einer Mission zeigen alle Reiter auf dieselbe .bsp; dann
    // faellt hier nichts an. Ein neuer, leerer Reiter hat keine, und dann
    // wird die alte weggeraeumt - sonst steht dort die Karte des vorigen
    // Skripts, und das war genau die Meldung.
    // --- Eine Karte, mehrere Skripte -------------------------------------
    //
    // Gewuenscht: "eine Map kann mehrere Skripte haben und drueber laufen,
    // deshalb duerfen sich mehrere Skripte doch eine teilen. Wuerde aber
    // auch gerne behalten, dass man pro Tab einzelne aufmachen kann."
    //
    // Beides geht, wenn man unterscheidet, ob ein Reiter eine EIGENE Karte
    // hat:
    //
    //   Reiter MIT eigener Karte (Mission geladen, .bsp geoeffnet)
    //       -> beim Hinwechseln wird sie geholt. Sein gutes Recht.
    //   Reiter OHNE eigene Karte (Skript einfach so geoeffnet)
    //       -> die vorhandene bleibt stehen. Er hat keine Meinung dazu,
    //          und die des Arbeitsbereichs ist besser als gar keine.
    //
    // Vorher wurde auch der zweite Fall behandelt, als haette er eine
    // Meinung - naemlich "keine" - und die Karte flog weg. Beim
    // Zurueckwechseln wurde sie neu gelesen: 35 bis 50 ms, hin und zurueck,
    // bei jedem Klick.
    //
    // Verglichen wird die KENNUNG, nicht der Plattenpfad: eine Mission aus
    // einem .pk3 hat gar keinen.
    const std::string karteVorher = g_app->map.path;
    // rc604: erst die Spielordner seines Spiels (setzeSitzungsSpielordner liest
    // nur neu ein, wenn sie andere sind), dann seine Karte aus ihnen.
    if (t.spielordnerGemerkt && !t.mapId.empty()) {
        setzeSitzungsSpielordner(t.spielordner);
    }
    if (!t.mapId.empty() && t.mapId != g_app->map.path) {
        char zeile[300];
        std::snprintf(zeile, sizeof(zeile),
                      "Reiter %d: Karte wechseln von \"%s\" nach \"%s\" "
                      "(Pfad \"%s\")",
                      index, g_app->map.path.c_str(), t.mapId.c_str(),
                      t.mapPath.c_str());
        diag::detail(zeile);
        if (!t.mapPath.empty()) {
            loadMap(t.mapPath);
        } else if (!loadMapFromArchive(t.mapId)) {
            // Aus dem Archiv nicht mehr lesbar - dann lieber gar keine als
            // die falsche.
            clearMap();
        }
    }
    // Und die Sicht wiederherstellen - aber nur, wenn dieser Reiter schon
    // einmal eine hatte. Ein frischer bekommt die Ansicht, die das Laden
    // der Karte einstellt, und nicht die Nullstellung.
    //
    // Auf DERSELBEN Karte bleibt die Kamera, wo sie ist (rc597): "wenn sich
    // eine map die ent teilt von allen scripts waere es nicht besser, dass
    // wenn ich die reiter wechsel alle dieselbe kamera teilen?" Die Zeit
    // bleibt je Reiter - jedes Skript hat seine eigene Zeitleiste.
    if (t.camSaved) {
        if (karteVorher.empty() || g_app->map.path != karteVorher) {
            for (int k = 0; k < 3; ++k) {
                g_app->cam.pos[k] = t.camPos[k];
                g_app->cam.angles[k] = t.camAng[k];
            }
        }
        g_app->playMs = t.playMs;
        g_app->playing = false;
        g_app->mapDirty = true;
    }
    rebuildTree();
    // Zur Auswahl rollen: ja beim REITERWECHSEL, nein beim Fokuswechsel.
    //
    // Wechselt man den Reiter, will man sehen, wo man dort stehengeblieben
    // ist. Wechselt man nur den Fokus von einem Feld zum anderen, hat sich
    // im Inhalt nichts geaendert - und ein Sprung zur Auswahl wirkt wie ein
    // Fehler. Genau so wurde es gemeldet: "springt am anderen fenster etwas
    // umher".
    // ... aber NUR, wenn der Reiter noch keine gemerkte Rollposition hat.
    //
    // shank, 27.09.: "beim allersten mal ziehen springt er einmal". Die
    // gemerkte Position stellt rollpositionHolen exakt wieder her; das
    // Zentrieren auf die Auswahl schob sie danach noch einmal weg. Und war
    // im Reiter nichts gewaehlt, blieb das Flag liegen - bis zum ersten
    // Klick auf eine Zeile, die dann in die Mitte sprang.
    if (rollen && g_app->tabScroll.find(tabKennung(index)) == g_app->tabScroll.end()) {
        g_app->scrollToSelected = true;
    }
    g_app->mapDirty = true;
}

// Auf einen anderen Reiter wechseln.
void switchTab(int index, bool rollen = true, bool layout = true) {
    if (index == g_app->activeTab) {
        return;
    }
    // Die Zwischenablage gehoert dem ANWENDER (App::ablage, rc601) - beim
    // Wechsel muss nichts mehr mitgenommen werden. Bis rc600 trug dieser
    // Wechsel sie hinueber; geschlossene oder neue Reiter verloren sie trotzdem.
    parkActive(layout);
    takeTab(index, rollen, layout);
    // Das Feld mit dem Fokus zeigt jetzt diesen Reiter - aber NUR beim
    // Fokuswechsel.
    //
    // Wird die Aufteilung mitgeholt (layout), dann steht die Belegung
    // schon in ihr, und dieses Nachziehen wuerde sie zerstoeren: das
    // fokussierte Feld bekaeme den Reiter, den ein anderes Feld ohnehin
    // schon zeigt. Genau so entstand die Dopplung - zweimal dasselbe
    // Skript nebeneinander, "was nicht gehen sollte".
    if (!layout) {
        g_app->splitPanes[std::clamp(g_app->focusPane, 0, App::kMaxSplit - 1)]
            .tab = g_app->activeTab;
    }
    // --- Kein Feld zweimal dasselbe Skript --------------------------------
    //
    // Ein Riegel, kein Kommentar: die Dopplung ist auf mehreren Wegen
    // entstanden (rc179 ueber neue Felder, jetzt ueber den Reiterwechsel),
    // und sie faellt im Bild kaum auf - zwei gleiche Baeume nebeneinander
    // sehen aus wie Absicht.
    //
    // Das FOKUSSIERTE Feld behaelt seinen Reiter; ein anderes, das denselben
    // zeigt, bekommt den ersten freien. Gibt es keinen, bleibt es stehen -
    // dann sind mehr Felder als Reiter offen, und darueber beschwert sich
    // die Selbstpruefung aus rc218.
    {
        const int fp2 = std::clamp(g_app->focusPane, 0, App::kMaxSplit - 1);
        for (int i2 = 0; i2 < g_app->splitCount; ++i2) {
            if (i2 == fp2) {
                continue;
            }
            bool doppelt = false;
            for (int j2 = 0; j2 < g_app->splitCount; ++j2) {
                if (j2 != i2 &&
                    g_app->splitPanes[j2].tab == g_app->splitPanes[i2].tab) {
                    doppelt = true;
                    break;
                }
            }
            if (!doppelt) {
                continue;
            }
            for (int k2 = 0; k2 < static_cast<int>(g_app->tabs.size()); ++k2) {
                bool belegt = false;
                for (int j2 = 0; j2 < g_app->splitCount; ++j2) {
                    if (g_app->splitPanes[j2].tab == k2) {
                        belegt = true;
                        break;
                    }
                }
                if (!belegt) {
                    char z2[200];
                    std::snprintf(z2, sizeof(z2),
                                  "Feld %d zeigte denselben Reiter wie ein "
                                  "anderes - auf Reiter %d umgestellt",
                                  i2, k2);
                    diag::detail(z2);
                    g_app->splitPanes[i2].tab = k2;
                    break;
                }
            }
        }
    }
    for (auto& pn : g_app->splitPanes) {
        pn.dirty = true;
    }
}

// Einen neuen Reiter anlegen und hineinwechseln.
//
// Der aktuelle wird vorher geparkt - er bleibt also erhalten, samt
// Rueckgaengig-Speicher und Auswahl. Genau das war der Wunsch: "Best to
// only have 1 instance of the program open."
void addTab(bool inheritMap) {
    parkActive();
    App::Parked frisch;
    // Ein Reiter, der aus einem geladenen Skript entsteht, erbt die Karte:
    // beim Laden einer Mission gehoeren alle Skripte zu derselben .bsp.
    //
    // Ein Reiter, den man mit dem Pluszeichen anlegt, erbt sie NICHT - er
    // ist ein neues, leeres Blatt. Genau das war die Meldung: "wenn ich
    // einen neuen tab öffne ist die map vom letzten map noch geladen".
    if (inheritMap) {
        frisch.mapPath = g_app->settings.mapPath;
        frisch.mapId = g_app->map.path;
        frisch.spielordner = g_app->sitzungsSpielordner;   // rc604
        frisch.spielordnerGemerkt = true;
    }
    g_app->tabs.push_back(frisch);
    g_app->activeTab = static_cast<int>(g_app->tabs.size()) - 1;
    // Ein neuer Reiter ist auch der neue Arbeitsbereich - sonst bliebe das
    // Band auf dem alten stehen, obwohl man gerade anderswo landet.
    g_app->homeTab = g_app->activeTab;
    // Die Karte JETZT mit dem neuen Reiter in Einklang bringen.
    //
    // Die Absicht stand schon da - ein neuer Reiter erbt die Karte nicht -,
    // aber sie stand nur in `frisch.mapPath`. Weggeraeumt wurde die alte
    // nie: addTab setzt activeTab von Hand und geht NICHT ueber takeTab(),
    // wo die Karte sonst nachgezogen wird. Also blieb sie stehen, bis man
    // den Reiter einmal verliess und zurueckkam - dann fiel sie ploetzlich
    // weg. Gemeldet als "ich habe einen neuen tab erstellt und die map vom
    // ersten tab ist immer noch drin".
    //
    // Hier und nicht beim Aufrufer, damit beide Wege - das Pluszeichen und
    // "Neu" - dasselbe tun und nicht auseinanderlaufen koennen.
    if (frisch.mapId != g_app->map.path) {
        if (frisch.mapId.empty()) {
            clearMap();
        } else if (!frisch.mapPath.empty()) {
            loadMap(frisch.mapPath);
        }
    }
    // Der neue Reiter bringt sein eigenes, ungeteiltes Fenster mit.
    g_app->splitCount = 1;
    g_app->focusPane = 0;
    g_app->splitPanes[0].tab = g_app->activeTab;
    for (auto& pn : g_app->splitPanes) {
        pn.dirty = true;
    }
}

// Einen Reiter wirklich wegnehmen - ohne zu fragen.
void dropTab(int index) {
    const int n = static_cast<int>(g_app->tabs.size());
    if (index < 0 || index >= n) {
        return;
    }
    // --- Den aktiven Reiter VORHER parken ---------------------------------
    //
    // shank, 02.10.: "I closed some scripts/tabs I was no longer using and
    // went back to continue working on the script but it had reset. All my
    // changes were lost with no undo/redo history."
    //
    // Der aktive Reiter lebt in g_app->doc; seine Kopie in tabs[] ist der
    // Stand beim letzten Reiterwechsel. Unten holt takeTab() den Reiter aus
    // dieser Kopie zurueck - ohne Parken vorher war alles seit dem letzten
    // Wechsel weg, samt Undo. Und lag der geschlossene Reiter LINKS, rutschte
    // die Nummer nicht nach: geholt wurde der Nachbar.
    const bool andererReiter = (index != g_app->activeTab);
    if (andererReiter) {
        parkActive(false);
    }
    // Wird der AKTIVE geschlossen, der nicht der Besitzer des Bands ist
    // (geteilte Ansicht: der Fokus steht in einem anderen Feld als dem
    // Arbeitsreiter), dann danach in den Besitzer - nicht in den Nachbarn.
    // Sonst zeigte das Band den Arbeitsreiter, bearbeitet wurde aber der
    // Nachbar (Code-Pruefung 03.10.).
    int nachher = -1;
    if (!andererReiter && g_app->homeTab != index && g_app->homeTab >= 0 && g_app->homeTab < n) {
        nachher = (g_app->homeTab > index) ? g_app->homeTab - 1 : g_app->homeTab;
    }
    g_app->tabs.erase(g_app->tabs.begin() + index);
    if (g_app->tabs.empty()) {
        g_app->tabs.push_back(App::Parked{});
    }
    // Die gemerkten Aufteilungen ALLER Reiter nachziehen: sie stehen als
    // Reiter-NUMMERN in Parked::splitTabs. Ohne das zeigte ein Feld, wenn man
    // in einen geteilten Reiter zurueckkam, das Skript rechts neben dem
    // gemeinten (Selbsttest "Belegung", 03.10.). Ein Feld, das den
    // geschlossenen Reiter zeigte, faellt auf den Besitzer zurueck.
    for (std::size_t k = 0; k < g_app->tabs.size(); ++k) {
        App::Parked& t = g_app->tabs[k];
        for (int& st : t.splitTabs) {
            if (st > index) {
                --st;
            } else if (st == index) {
                st = static_cast<int>(k);
            }
        }
    }
    if (andererReiter && index < g_app->activeTab) {
        --g_app->activeTab;
    }
    if (nachher >= 0) {
        g_app->activeTab = nachher;
    }
    g_app->activeTab = std::clamp(g_app->activeTab, 0,
                                  static_cast<int>(g_app->tabs.size()) - 1);
    // Der Besitzer der Aufteilung genauso - er ist eine Reiternummer, und
    // die verschieben sich beim Loeschen (dieselbe Falle wie bei den
    // Feldern, siehe unten).
    if (g_app->homeTab > index) {
        --g_app->homeTab;
    }
    g_app->homeTab = std::clamp(g_app->homeTab, 0,
                                static_cast<int>(g_app->tabs.size()) - 1);
    // Die Aufteilung mit schrumpfen lassen.
    //
    // Bleibt sie stehen, zeigt ein Feld nach dem Schliessen zwangslaeufig
    // dasselbe Skript wie ein anderes - genau die Dopplung, die es nicht
    // geben soll. Die Felder zeigen ausserdem auf Reiternummern, und die
    // haben sich beim Loeschen verschoben.
    const int uebrig = static_cast<int>(g_app->tabs.size());
    g_app->splitCount = std::clamp(g_app->splitCount, 1,
                                   std::max(1, uebrig));
    g_app->focusPane = std::clamp(g_app->focusPane, 0, g_app->splitCount - 1);
    for (auto& pn : g_app->splitPanes) {
        // Wer rechts vom geschlossenen stand, rueckt eins nach links.
        if (pn.tab > index) { --pn.tab; }
        pn.tab = std::clamp(pn.tab, 0, std::max(0, uebrig - 1));
        pn.dirty = true;
    }
    // layout=false: die Aufteilung ist eben schon nachgefuehrt.
    takeTab(g_app->activeTab, true, !andererReiter);
}

// Einen Reiter schliessen - mit Nachfrage, falls noetig.
//
// Der ungesicherte Reiter wird zuerst SICHTBAR gemacht und dann gefragt.
// Man soll sehen, worueber man entscheidet.
void closeTab(int index) {
    const int n = static_cast<int>(g_app->tabs.size());
    if (index < 0 || index >= n) {
        return;
    }
    // --- Ein ANDERER, ungesicherter Reiter -------------------------------
    //
    // Er wird nach vorn geholt, damit man sieht, worueber man entscheidet -
    // und danach geht es ZURUECK in den Reiter, in dem man gearbeitet hat.
    //
    // shank, 03.10. (zum zweiten Mal): "I closed some scripts/tabs I was no
    // longer using and went back to continue working on the script but it
    // had reset." Hier stand nur switchTab(index): der Besitzer des Bands
    // (homeTab) blieb auf dem Arbeitsreiter, das lebende Dokument aber nicht.
    // Nach "Nein" holte dropTab den NACHBARN des geschlossenen hervor -
    // waehrend das Band weiter den Arbeitsreiter markierte. Ein Klick darauf
    // tat nichts (er war ja schon "gewaehlt"); man stand in einem fremden
    // Skript unter dem Namen des eigenen.
    if (index != g_app->activeTab &&
        g_app->tabs[static_cast<std::size_t>(index)].doc.dirty()) {
        const int zurueck = g_app->activeTab;
        switchTab(index);
        g_app->homeTab = index;   // das Band zeigt, wonach gefragt wird
        const int idx = index;
        withUnsaved(
            [idx, zurueck] {
                dropTab(idx);
                // Der Arbeitsreiter ist eins nach links gerueckt, wenn der
                // geschlossene vor ihm lag.
                const int ziel = (zurueck > idx) ? zurueck - 1 : zurueck;
                if (ziel >= 0 && ziel < static_cast<int>(g_app->tabs.size())) {
                    switchTab(ziel);
                    g_app->homeTab = ziel;
                }
            },
            [zurueck] {
                // Abgebrochen: der gefragte Reiter bleibt offen, und man
                // kehrt trotzdem in seinen Arbeitsreiter zurueck.
                if (zurueck >= 0 && zurueck < static_cast<int>(g_app->tabs.size())) {
                    switchTab(zurueck);
                    g_app->homeTab = zurueck;
                }
            });
        return;
    }
    if (index == g_app->activeTab) {
        const int idx = index;
        withUnsaved([idx] { dropTab(idx); });
        return;
    }
    dropTab(index);
}

// --- Ist dieses Skript schon offen? --------------------------------------
//
// shank, 03.10.: "All my changes were lost with no undo/redo history. It
// was like I just opened the file again." Genau das konnte passieren: wer
// ein Skript, das schon in einem Reiter steht, noch einmal oeffnete (Datei
// oeffnen, zuletzt geoeffnet, Doppelklick im Explorer, die Mission noch
// einmal laden), bekam einen ZWEITEN Reiter mit dem Stand von der Platte -
// ohne Verlauf und ohne Aenderungsrand. Schloss man danach den aelteren,
// war die Arbeit weg; hatte man gespeichert, "nur" der Verlauf.
//
// Wie jedes Programm mit Reitern (Notepad++, VS Code): ist es schon offen,
// wird dorthin gewechselt.
namespace {
std::string pfadSchluessel(const std::string& p) {
    std::error_code ec;
    const std::filesystem::path c =
        std::filesystem::weakly_canonical(::bhed::utf8Pfad(p), ec);
    std::string s = p;
    if (!ec) {
        const auto u = c.u8string();
        s.assign(u.begin(), u.end());
    }
    for (char& ch : s) {
        if (ch == '\\') { ch = '/'; }
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

// Der ICARUS-Pfad, wie openScriptFromMemory ihn aus dem Namen macht:
// "md_ga/intro_jedi.txt" -> "md_ga/intro_jedi", klein geschrieben.
std::string skriptSchluessel(const std::string& name) {
    std::string sp = name;
    for (char& ch : sp) {
        if (ch == '\\') { ch = '/'; }
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    const auto punkt = sp.find_last_of('.');
    const auto strich = sp.find_last_of('/');
    if (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) {
        sp.resize(punkt);
    }
    return sp;
}
}  // namespace

int reiterMitPfadI(const std::string& p) {
    if (p.empty()) {
        return -1;
    }
    const std::string k = pfadSchluessel(p);
    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
        const bool aktiv = (static_cast<int>(i) == g_app->activeTab);
        const std::string& tp = aktiv ? g_app->path : g_app->tabs[i].path;
        if (!tp.empty() && pfadSchluessel(tp) == k) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int reiterMitSkriptI(const std::string& name) {
    const std::string k = skriptSchluessel(name);
    if (k.empty()) {
        return -1;
    }
    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
        const bool aktiv = (static_cast<int>(i) == g_app->activeTab);
        const std::string& tp = aktiv ? g_app->path : g_app->tabs[i].path;
        const std::string& sp = aktiv ? g_app->skriptPfad : g_app->tabs[i].skriptPfad;
        if (tp.empty() && !sp.empty() && skriptSchluessel(sp) == k) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Zum schon offenen Reiter wechseln und es in der Statuszeile sagen.
void zumOffenenReiter(int index, const std::string& name) {
    switchTab(index);
    g_app->homeTab = index;
    char buf[300];
    std::snprintf(buf, sizeof(buf), tr(Str::MsgAlreadyOpen), name.c_str());
    addStatus(buf);
    diag::detail("Schon offen in Reiter " + std::to_string(index) + ": " + name);
}

// Ein Skript aus dem Speicher - aber nur, wenn es nicht schon offen ist.
// Fuer die Wege, auf denen der BENUTZER oeffnet (Archiv, Mission); der
// Selbsttest ruft openScriptFromMemory weiter direkt.
void oeffneSkriptAusSpeicherI(const std::string& data, const std::string& name) {
    const int offen = reiterMitSkriptI(name);
    if (offen >= 0) {
        zumOffenenReiter(offen, name);
        return;
    }
    openScriptFromMemory(data, name);
}

// Was der Leser beim Oeffnen nicht verstanden hat, in die Statusliste - in
// der eingestellten Sprache. Vorher wurden die Meldungen verworfen
// ("(void)readScript"), und man erfuhr erst beim Uebersetzen (oder nie),
// dass eine Zeile nicht verstanden wurde (@Seph, MP-Bericht 08.10.).
void leseMeldungenZeigen(const std::vector<Diag>& d, const std::string& name) {
    for (const Diag& x : d) {
        const char* muster = nullptr;
        if (x.code == "L001") { muster = tr(Str::LmsgL001); }
        else if (x.code == "L002") { muster = tr(Str::LmsgL002); }
        else if (x.code == "L003") { muster = tr(Str::LmsgL003); }
        else if (x.code == "L004") { muster = tr(Str::LmsgL004); }
        else if (x.code == "L005") { muster = tr(Str::LmsgL005); }
        else if (x.code == "L006") { muster = tr(Str::LmsgL006); }
        if (muster == nullptr) { continue; }
        char text[300];
        std::snprintf(text, sizeof(text), muster, x.arg.c_str());
        char zeile[380];
        std::snprintf(zeile, sizeof(zeile), tr(Str::ReadNoteLine), name.c_str(), x.line, text);
        addStatus(zeile);
    }
}

void loadPath(const std::string& p) {
    diag::Step step("Skript oeffnen: " + p);
    // Schon offen? Dann dorthin - siehe reiterMitPfad.
    if (const int offen = reiterMitPfadI(p); offen >= 0) {
        zumOffenenReiter(offen, fileName(p));
        return;
    }
    const std::string src = slurp(p);
    if (src.empty()) {
        platform::showError(p, tr(Str::AppTitle));
        return;
    }
    Script sc;
    std::vector<Diag> d;
    // --- Erst schauen, WAS in der Datei steht -----------------------------
    //
    // Gemeldet: eine .ibi laesst sich oeffnen, aber danach ist nichts drin.
    //
    // Der Grund: hier stand nur readScript(). Eine .ibi ist aber keine
    // Textdatei, sondern uebersetzter Bytecode - der Textleser findet darin
    // keinen einzigen Befehl und liefert ein leeres Skript. Kein Absturz,
    // keine Meldung, nur ein leerer Baum.
    //
    // Der Weg ueber den Speicher (aus einer Mission, openScriptFromMemory)
    // konnte es laengst; nur dieser hier nicht. Wieder zwei Wege, die
    // dasselbe leisten sollten, und einer tat es nur halb.
    //
    // Erkannt wird am INHALT, nicht an der Endung: eine .ibi kann anders
    // heissen, und eine .txt kann uebersetzt sein. Genau so macht es der
    // andere Weg auch.
    const bool istIbi = src.size() > 4 && src.compare(0, 3, "IBI") == 0;
    if (istIbi) {
        std::vector<IbiBlock> bl;
        if (readIbi(src, bl, d)) {
            (void)decompile(bl, g_app->db, sc, d);
        }
        diag::detail("Geoeffnet als IBI: " + std::to_string(bl.size()) +
                     " Bloecke -> " + std::to_string(sc.nodes.size()) +
                     " Knoten");
    } else {
        (void)readScript(src, sc, d);
    }
    leseMeldungenZeigen(d, fileName(p));
    // In einen NEUEN Reiter, ausser der aktuelle ist noch unberuehrt.
    //
    // Sonst verliert man beim Oeffnen, woran man gerade arbeitet - und
    // genau das war der Grund fuer mehrere Programmfenster: "this is how I
    // usually work with multiple scripts open where I copy and paste
    // commands between them."
    //
    // Ein leerer, ungeaenderter Reiter wird wiederverwendet: wer gerade
    // gestartet hat, will keinen zweiten.
    const bool leerUndSauber =
        g_app->path.empty() && !g_app->doc.dirty() &&
        g_app->doc.script().nodes.empty();
    if (!leerUndSauber) {
        addTab(true);
        takeTab(g_app->activeTab);
    }
    g_app->doc = Document{sc};
    g_app->path = p;
    g_app->shownName.clear();   // der Pfad genuegt
    g_app->skriptPfad.clear();  // kommt aus dem Pfad
    g_app->selectedPath.clear();
    g_app->selection.clear();
    g_app->selected = -1;
    g_app->expanded = Expanded{};
    g_app->settings.noteRecent(p);
    // Frisch geladen: kein Aenderungsrand, die gemerkten Lesezeichen zurueck.
    aenderungsStandNeu();
    lesezeichenLaden();
    rebuildTree();
    setStatus(tr(Str::MsgLoaded), static_cast<int>(g_app->rows.size()), 0);
}

void doOpenOhneFrage() {
    withUnsaved([] {
    const std::string start = (g_app->settings.dialogUsesLastDir &&
                               !g_app->settings.lastDir.empty())
                                  ? g_app->settings.lastDir
                                  : g_app->settings.scriptPath;
    const std::string p = platform::openFileDialog(tr(Str::FileOpen), kScriptFilter, start);
    if (!p.empty()) {
        g_app->settings.lastDir = directoryOf(p);
        loadPath(p);
    }
    });
}

// Oeffnen fragt NICHT mehr "Open?".
//
// Das Original tat es mit "Yes/No query on Open/New/Exit". shank: "I don't
// think a confirmation window is needed for opening a script". Oeffnen
// verwirft nichts - ungespeicherte Aenderungen fragt withUnsaved ohnehin
// ab, und der Dateidialog laesst sich abbrechen. New und Exit fragen
// weiterhin, wenn die Einstellung an ist.
void doOpen() {
    doOpenOhneFrage();
}

void neuesSkript();

// "New?" - nur mit "Yes/No query on Open/New/Exit" (Original).
void doNew() {
    const bool leerUndSauber =
        g_app->path.empty() && g_app->shownName.empty() &&
        !g_app->doc.dirty() && g_app->doc.script().nodes.empty();
    if (leerUndSauber) {
        return;
    }
    if (g_app->settings.queryOnDiscard) {
        frage(tr(Str::AskNew), [] { neuesSkript(); });
        return;
    }
    neuesSkript();
}

void neuesSkript() {
    // In einen NEUEN Reiter, wie beim Oeffnen.
    //
    // Gemeldet als: "If I click the New button in the Actions panel, it
    // should open a new tab with an empty script." Richtig - seit es Reiter
    // gibt, ist das Ueberschreiben des aktuellen die falsche Handlung.
    //
    // Ein leerer, unberuehrter Reiter wird wiederverwendet: wer gerade
    // gestartet hat, braucht keinen zweiten. Und weil dann nichts
    // wegzuwerfen ist, entfaellt auch die Frage.
    const bool leerUndSauber =
        g_app->path.empty() && g_app->shownName.empty() &&
        !g_app->doc.dirty() && g_app->doc.script().nodes.empty();
    if (leerUndSauber) {
        return;
    }
    // Die Karte NICHT erben: ein leeres Skript hat keinen Bezug zu der
    // Karte, die im vorigen Reiter stand.
    addTab(false);
    g_app->doc = Document{Script{}};
    g_app->path.clear();
    g_app->shownName.clear();
    g_app->skriptPfad.clear();
    g_app->selectedPath.clear();
    g_app->selection.clear();
    g_app->selected = -1;
    g_app->expanded = Expanded{};
    aenderungsStandNeu();
    g_app->lesezeichen.erase(tabKennung(g_app->activeTab));
    rebuildTree();
}

// Alle Reiter sichern.
//
// Gewuenscht als "Save All". Der aktive wird zuletzt gesichert, damit man
// am Ende dort steht, wo man angefangen hat - ein Reiterwechsel mitten im
// Sichern waere sonst eine unangenehme Ueberraschung.
//
// Reiter ohne Pfad (aus einem Archiv geladen) werden UEBERSPRUNGEN: dorthin
// laesst sich nicht zurueckschreiben, und ein Dateidialog je Reiter waere
// das Gegenteil von "alle sichern".
// Einen Reiter sichern, ohne dass sich die Ansicht bewegt: aktiver Reiter,
// Band, Feldaufteilung und Fokus stehen danach wie vorher. Save all ging
// ueber switchTab - das holte die Aufteilung jedes Reiters mit, und aus zwei
// Feldern wurde eines (Code-Pruefung 03.10.).
bool speichereReiterI(int tab) {
    if (tab == g_app->activeTab) {
        return doSave();
    }
    if (tab < 0 || tab >= static_cast<int>(g_app->tabs.size())) {
        return false;
    }
    const int aktiv = g_app->activeTab;
    const int band = g_app->homeTab;
    const int fokus = g_app->focusPane;
    const int felder = g_app->splitCount;
    int belegung[App::kMaxSplit];
    for (int k = 0; k < App::kMaxSplit; ++k) { belegung[k] = g_app->splitPanes[k].tab; }
    switchTab(tab, false, false);
    const bool ok = doSave();
    switchTab(aktiv, false, false);
    g_app->homeTab = band;
    g_app->focusPane = fokus;
    g_app->splitCount = felder;
    for (int k = 0; k < App::kMaxSplit; ++k) {
        g_app->splitPanes[k].tab = belegung[k];
        g_app->splitPanes[k].dirty = true;
    }
    return ok;
}

void doSaveAll() {
    int gesichert = 0;
    int ohnePfad = 0;
    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
        const bool aktiv = (static_cast<int>(i) == g_app->activeTab);
        const bool schmutzig =
            aktiv ? g_app->doc.dirty() : g_app->tabs[i].doc.dirty();
        const bool hatPfad =
            aktiv ? !g_app->path.empty() : !g_app->tabs[i].path.empty();
        if (!schmutzig) {
            continue;
        }
        if (!hatPfad) {
            ++ohnePfad;
            continue;
        }
        if (speichereReiterI(static_cast<int>(i))) {
            ++gesichert;
        }
    }
    setStatus(tr(Str::MsgSavedAll), gesichert, ohnePfad);
}

// "Append" des Originals: ein zweites Skript hinten anhaengen.
void doAppend() {
    const std::string p = platform::openFileDialog(tr(Str::FileAppend), kScriptFilter,
                                                   g_app->settings.scriptPath);
    if (p.empty()) {
        return;
    }
    const std::string daten = slurp(p);
    Script other;
    std::vector<Diag> d;
    // Der Filter bietet .ibi an - also auch .ibi lesen (wie beim Oeffnen).
    if (daten.size() > 4 && daten.compare(0, 3, "IBI") == 0) {
        std::vector<IbiBlock> bl;
        if (readIbi(daten, bl, d)) {
            (void)decompile(bl, g_app->db, other, d);
        }
    } else {
        (void)readScript(daten, other, d);
    }
    // Die Trennzeilen des Originals davor:
    //     "   Appended File (%s) follows...  "
    //     "============================="
    // Hier als Kommentarzeilen - sie gehoeren nicht zum Skriptablauf.
    std::vector<Node> neu;
    {
        Node kopf;
        kopf.kind = Node::Kind::LineComment;
        kopf.raw = "//   Appended File (" + fileName(p) + ") follows...  ";
        Node linie;
        linie.kind = Node::Kind::LineComment;
        linie.raw = "//=============================";
        neu.push_back(std::move(kopf));
        neu.push_back(std::move(linie));
    }
    for (const Node& n : other.nodes) {
        neu.push_back(n);
    }
    Path at;
    if (!g_app->doc.script().nodes.empty()) {
        at.push_back(g_app->doc.script().nodes.size() - 1);
    }
    // EIN Rueckgaengig-Schritt fuer die ganze angehaengte Datei.
    (void)g_app->doc.insertAfterAll(at, neu, nullptr);
    rebuildTree();
}

#include "app_anordnung.inc"   // Stufe 8d: pk3, Undo-Liste, Klaenge, Karte, Symbole, Anordnung, Compile-Knopf
}  // namespace
#include "app_ablage.inc"   // Stufe 8d: Zwischenablage, Dateigrenzen, Berichte, Himmelsshader
namespace {
// --- Mausrad ueber der Reiterleiste (rc601) ----------------------------------
//
// Gemeldet (Tester, 17.10.): "It would be nice if I could use the scroll wheel
// on the file tabs to move left and right so I can view all the files."
// ImGui rollt eine FittingPolicyScroll-Leiste nur mit dem WAAGERECHTEN Rad oder
// Umschalt+Rad: TabBarLayout liest `MouseWheelRequestAxisSwap ? MouseWheel :
// MouseWheelH` (imgui_widgets.cpp:10225). Das normale Rad tat hier nichts.
// Jetzt: Rad runter = nach rechts, eine Raste = 1/3 der sichtbaren Breite,
// genau wie Umschalt+Rad. Aufrufen direkt nach BeginTabBar und VOR dem ersten
// BeginTabItem: dort laeuft TabBarLayout, klemmt ScrollingTarget
// (TabBarScrollClamp) und laesst ScrollingAnim im selben Bild hinterhergleiten.
void reiterLeisteMausrad(ImGuiTabBar* leiste) {
    if (leiste == nullptr) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    // Strg+Rad zoomt die Oberflaeche (oben in draw), Umschalt+Rad rollt ImGui selbst.
    if (io.KeyCtrl || io.MouseWheelRequestAxisSwap) { return; }
    // Werte des letzten Bildes: BeginTabBar setzt BarRect neu, nicht aber
    // ScrollingRectMinX/MaxX und ScrollButtonEnabled (= "laeuft ueber").
    const float sichtbar = leiste->ScrollingRectMaxX - leiste->ScrollingRectMinX;
    if (!leiste->ScrollButtonEnabled || sichtbar <= 0.0F) { return; }
    // Nur ueber dem rollbaren Teil der Leiste (wie ImGuis eigener Zweig) und
    // nur, wenn das Hauptfenster selbst unter der Maus liegt - kein
    // schwebendes Fenster, keine offene Liste, kein Modal darueber.
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) ||
        !ImGui::IsMouseHoveringRect(ImVec2{leiste->ScrollingRectMinX, leiste->BarRect.Min.y},
                                    ImVec2{leiste->ScrollingRectMaxX, leiste->BarRect.Max.y})) {
        return;
    }
    // Wie ImGui: erst pruefen, dann besitzen - mit DERSELBEN Kennung
    // (leiste->ID), damit Umschalt+Rad nicht ausgesperrt wird.
    if (io.MouseWheel != 0.0F && ImGui::TestKeyOwner(ImGuiKey_MouseWheelY, leiste->ID)) {
        leiste->ScrollingTargetDistToVisibility = 0.0F;   // gleiten statt springen
        // Nur unten klemmen; die genaue Obergrenze (Listen- und Pfeilknoepfe
        // abgezogen) setzt TabBarScrollClamp in TabBarLayout gleich danach.
        leiste->ScrollingTarget = std::max(0.0F, leiste->ScrollingTarget - io.MouseWheel * sichtbar / 3.0F);
    }
    ImGui::SetKeyOwner(ImGuiKey_MouseWheelY, leiste->ID);
}
}  // namespace

void draw() {
    BHED_MESS("Oberflaeche");
    if (g_app == nullptr) { return; }
    // Einmal: Reste eines Updates wegraeumen, und im Hintergrund nachsehen.
    dateienVonAussenOeffnen();
    static bool updateGestartet = false;
    if (!updateGestartet) {
        updateGestartet = true;
        updater::beimStart();
    }
    // rc601 integ: Regelwerk (OpenJK / Movie Duels) anwenden, wenn sich
    // Einstellung, Schwierigkeit oder Spielordner geaendert haben.
    (void)regelwerkPruefen();
    // Stufe 3: ein laufendes Bauen im Karten-Editor weiterfuehren (q3map2 und
    // sein Rueckkanal) - auch, wenn gerade ein anderer Arbeitsbereich offen ist.
    karteneditor::bauSchritt();

    // --- Den Klangstrom nachfuellen ------------------------------------
    //
    // Gemeldet: "der Sound ging ganz kurz, und danach reisst es ab."
    //
    // Das war der fehlende Teil von rc335. Der Mischer schreibt zwei
    // Bloecke im Voraus - rund 92 ms - und danach muss jemand nachlegen.
    // `update()` wurde aber NUR aus `play()` gerufen. Solange kein neuer
    // Klang begann, lief die Ausgabe leer, und der Rest fiel weg.
    //
    // Ein durchlaufender Strom braucht einen durchlaufenden Antrieb.
    // Hier ist er: einmal je Bild, unabhaengig davon, ob gerade etwas
    // gestartet wird.
    //
    // Beide Geraete, nicht nur eines - Musik laeuft ueber ein eigenes.
    static bool tonGestartet = false;
    if (!tonGestartet) {
        tonGestartet = true;
        g_app->tonVorab = std::async(std::launch::async, [] {
            App::TonVorab t;
            // Dasselbe feste Format wie playSound (kRate, kChannels).
            (void)t.ton.open(44100, 2);
            (void)t.musik.open(44100, 2);
            return t;
        });
    }
    tonUebernehmen(false);
    g_app->audio.update();
    g_app->musicAudio.update();

    // Hier ist der erste Ort, an dem ImGui sicher einen Kontext hat.
    // createApp() und loadSettings() laufen VOR CreateContext(); ein
    // ImGui::GetStyle() dort loest "No current context" aus und das
    // Programm bricht beim Start ab, bevor ein Fenster erscheint.
    ensureIcons();

    // Zustand der Dialoge festhalten, BEVOR einer davon zeichnet und sich
    // dabei selbst schliesst. Begruendung steht bei der Feldbeschreibung
    // in app_internal.h.
    // Find getrennt: dort gehen Rueckgaengig/Wiederholen durch (Ersetzen).
    g_app->findAtFrameStart = g_app->findOpen;
    g_app->dialogAtFrameStart = g_app->editorOpen ||
                                g_app->askSaveOpen ||
                                g_app->prefsOpen || g_app->pk3Open ||
                                g_app->aboutOpen || g_app->missionPickOpen;

    // Strg+Mausrad vergroessert und verkleinert, wie in jedem Editor - nur
    // nicht ueber der Zeitleiste: dort zoomt Strg+Rad die Spuren (rc599,
    // vorher geschah beides zugleich). Im Karten-Editor auch nicht (Stufe 7
    // Punkt 6b): dort gehoert das Rad mit und ohne Strg den Ansichten wie bei
    // NRC (XYWnd zoomt, CamWnd faehrt, der Texturbrowser rollt).
    if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0F &&
        !ImGui::GetIO().WantTextInput && !g_app->tlUnterMaus && g_app->leftMode != 3) {
        g_app->uiScale = std::clamp(
            g_app->uiScale + ImGui::GetIO().MouseWheel * 0.1F, 0.5F, 3.0F);
    }
    g_app->tlUnterMaus = false;   // die Zeitleiste setzt es in diesem Bild neu

    const float total = g_app->dpiScale * g_app->uiScale;
    if (g_app->themePending || std::fabs(total - g_app->appliedScale) > 0.001F) {
        applyScale(total);
        g_app->themePending = false;
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    // Titel wie im Original: "unnamed.txt *"
    std::string title = g_app->path.empty() ? std::string("unnamed.txt")
                                            : fileName(g_app->path);
    if (g_app->doc.dirty()) {
        title += " *";
    }
    title += "###main";   // ImGui-Kennung stabil halten
    // Stufe 3 Punkt 0b ("Eine Leiste, die umschaltet", Entscheidung shank): im
    // Modus Karten-Editor ist die Menueleiste dieses Fensters die von
    // NetRadiant-Custom - 21 Qt-Punkte hoch in NRCs Schrift. ImGui legt die Hoehe
    // beim Begin fest, darum Schrift und Polster schon hier.
    const bool kartenLeiste = (g_app->leftMode == 3);
    if (kartenLeiste) { karteneditor::hauptleisteStilAn(); }
    ImGui::Begin(title.c_str(), nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar |
                 // Das Hauptfenster rollt nie. shank: "wenn ich vor und
                 // zurueck scrolle bewegen sich die Tabs" - der Inhalt war in
                 // der Kartenansicht 14 Punkte zu hoch (siehe computeLayout),
                 // und das Mausrad rollte die ganze Oberflaeche.
                 ImGuiWindowFlags_NoScrollWithMouse |
                 // --- Nie VOR die schwebenden Fenster ------------------
                 //
                 // Gemeldet: "If I click outside the window, then it
                 // closes/hides, and I am unable to open it again" - bei
                 // Notes, Keyboard shortcuts, Find und der Rueckfrage.
                 // Ein Klick auf das Hauptfenster holte es nach VORN; das
                 // schwebende Fenster lag dahinter, sein Offen-Merker
                 // stand weiter, und der Knopf setzte nur, was schon
                 // gesetzt war. Beim Einstellungsfenster sperrte der
                 // Merker obendrein alle Tastenkuerzel ("Reset all to
                 // defaults breaks all keyboard shortcuts").
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (kartenLeiste) {
        karteneditor::hauptleisteStilAus();
        karteneditor::zeichneHauptmenue();
    } else {
        drawMenu();
    }
    // --- Die Reiterleiste, ganz oben --------------------------------------
    //
    // Ueber Events, Script Flow und Actions - so, wie du es beschrieben
    // hast: "tabs sollten überhalb von Event und scripts und so
    // untergebracht sein".
    //
    // ImGui bringt dafuer BeginTabBar mit. Verwendet werden drei Merkmale:
    //   Reorderable   die Reiter lassen sich mit der Maus umsortieren
    //   FittingPolicyScroll  bei vielen Reitern wird gerollt, nicht
    //                        gequetscht - Namen bleiben lesbar; das Mausrad
    //                        ueber der Leiste rollt waagerecht (rc601)
    //   TabListPopupButton   der Pfeil links listet alle offenen Skripte
    //
    // Der Stern hinter dem Namen ist derselbe wie im Fenstertitel.
    //
    // Im Karten-Editor gibt es keine Skript-Reiter (Stufe 3 Punkt 0b): der
    // Editor beginnt direkt unter der Menueleiste.
    if (!g_app->tabs.empty() && !kartenLeiste) {
        // KEIN ImGuiTabBarFlags_AutoSelectNewTabs: das Programm waehlt einen
        // neuen Reiter selbst (addTab setzt homeTab), und die Nachfuehrung
        // unten bringt ImGui dorthin. Mit der Fahne hielt die Leiste einen
        // gerade angelegten Reiter fuer einen KLICK und wechselte im
        // naechsten Bild dorthin - auch wenn das Programm inzwischen woanders
        // stand (Selbsttest "Doppelt", 03.10.: Datei schon offen, behaved
        // wechselt hin, die Leiste zieht zum neuen Reiter zurueck).
        if (ImGui::BeginTabBar("##skripte",
                               ImGuiTabBarFlags_Reorderable |
                                   ImGuiTabBarFlags_FittingPolicyScroll |
                                   ImGuiTabBarFlags_TabListPopupButton)) {
            reiterLeisteMausrad(ImGui::GetCurrentTabBar());   // VOR dem ersten BeginTabItem
            int wechselZu = -1;
            int schliesse = -1;
            // --- ImGuis eigene Auswahl dem Programm nachfuehren -------------
            //
            // ImGui merkt sich selbst, welcher Reiter gewaehlt ist, und
            // meldet ihn jedes Bild. Wechselt das PROGRAMM den Reiter (drei
            // Skripte auf einmal geoeffnet, ein Reiter geschlossen, Fokus
            // aus einem Feld der Aufteilung), steht ImGui noch auf dem alten
            // - und die Meldung unten zog das Programm dorthin zurueck:
            // "ploetzlich in einem anderen Tab" (Selbsttest beim Nachstellen
            // von shanks Datenverlust, 02.10.). Deshalb: weicht ImGuis Wahl
            // vom Besitzer ab, bekommt dessen Reiter SetSelected, und in
            // diesem Bild ist ImGuis Meldung kein Klick.
            static int imguiWahl = -1;
            const bool nachfuehren = (imguiWahl != g_app->homeTab);
            int gemeldet = -1;
            for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                // Fuer NAME und Zustand gilt weiter der aktive Reiter: nur
                // er hat das lebende Dokument. Fuer die AUSWAHL im Band
                // gilt der Besitzer - siehe unten.
                const bool aktiv = (static_cast<int>(i) == g_app->activeTab);
                const std::string& tp =
                    aktiv ? g_app->path : g_app->tabs[i].path;
                const std::string& tn =
                    aktiv ? g_app->shownName : g_app->tabs[i].shownName;
                const bool schmutzig =
                    aktiv ? g_app->doc.dirty() : g_app->tabs[i].doc.dirty();
                std::string name = !tp.empty()  ? fileName(tp)
                                   : !tn.empty() ? tn
                                                 : std::string("unnamed.txt");
                if (schmutzig) {
                    name += " *";
                }
                // "###" haengt die Kennung an der Nummer fest: sonst
                // wandert sie mit dem Namen, und ImGui verliert beim
                // Speichern die Zuordnung.
                name += "###tab" + std::to_string(i);
                bool offen = true;
                const ImGuiTabItemFlags fahnenReiter =
                    (nachfuehren && static_cast<int>(i) == g_app->homeTab) ? ImGuiTabItemFlags_SetSelected
                                                                           : ImGuiTabItemFlags_None;
                if (ImGui::BeginTabItem(name.c_str(), &offen, fahnenReiter)) {
                    gemeldet = static_cast<int>(i);
                    // Verglichen wird mit dem BESITZER, nicht mit dem
                    // aktiven Reiter.
                    //
                    // ImGui meldet JEDES BILD, welcher Reiter ausgewaehlt
                    // ist. Stand hier "aktiv", dann zog das Band den
                    // aktiven Reiter sofort zurueck, sobald ein Klick in
                    // ein Vergleichsfeld ihn verschoben hatte - und man
                    // landete "ploetzlich in einem anderen Tab".
                    //
                    // Der Besitzer aendert sich nur, wenn man wirklich
                    // einen Reiter anklickt. Damit bleibt das Band ruhig,
                    // waehrend man in den Feldern arbeitet.
                    if (static_cast<int>(i) != g_app->homeTab && !nachfuehren) {
                        wechselZu = static_cast<int>(i);
                    }
                    ImGui::EndTabItem();
                }
                if (!offen) {
                    schliesse = static_cast<int>(i);
                }
            }
            // Ein Pluszeichen zum Anlegen - wie im Browser.
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing |
                                              ImGuiTabItemFlags_NoTooltip)) {
                // Ohne Karte - ein neues, leeres Blatt.
                addTab(false);
                takeTab(g_app->activeTab);
            }
            ImGui::EndTabBar();
            imguiWahl = gemeldet;
            // ERST nach EndTabBar handeln - ein Umschalten mitten in der
            // Leiste zieht ImGui den Boden unter den Fuessen weg.
            if (schliesse >= 0) {
                closeTab(schliesse);
            } else if (wechselZu >= 0) {
                // ERST wechseln, DANN den Besitzer setzen.
                //
                // Andersherum war es falsch: switchTab parkt die aktuelle
                // Aufteilung in den Besitzer - und wenn der schon der NEUE
                // war, bekam der frische Reiter die vier Felder
                // angehaengt, die er nie hatte. Danach holte takeTab genau
                // die wieder hervor. Gemeldet als "wenn ich die tabs
                // wechsel, habe ich immer noch die 4er ansicht".
                //
                // In dieser Reihenfolge stimmt beides: geparkt wird in den
                // ALTEN Besitzer, geholt wird die Aufteilung des neuen.
                switchTab(wechselZu);
                g_app->homeTab = wechselZu;
            }
        }
    }

    const Layout l = computeLayout();

    // --- Was gilt gerade? -------------------------------------------------
    //
    // Reiter, Felder und Groessen ins Detailprotokoll, aber nur bei einer
    // AENDERUNG. Damit laesst sich nachvollziehen, was beim Oeffnen mehrerer
    // Skripte und beim Ziehen der Trennlinien wirklich geschieht - statt es
    // aus Bildschirmfotos zu erschliessen.
    {
        App::LayoutLog jetzt;
        jetzt.tabs = static_cast<int>(g_app->tabs.size());
        jetzt.activeTab = g_app->activeTab;
        jetzt.splitCount = g_app->splitCount;
        jetzt.focusPane = g_app->focusPane;
        for (int i = 0; i < App::kMaxSplit; ++i) {
            jetzt.paneTabs[i] = g_app->splitPanes[i].tab;
        }
        jetzt.frameW = static_cast<int>(l.total);
        jetzt.frameH = static_cast<int>(l.topH);
        jetzt.eventsW = static_cast<int>(l.eventsW);
        jetzt.flowW = static_cast<int>(l.flowW);
        jetzt.toolH = static_cast<int>(l.toolH);
        jetzt.tooSmall = l.tooSmall;
        jetzt.fracX = static_cast<int>(g_app->splitFracX * 1000.0F);
        jetzt.fracY = static_cast<int>(g_app->splitFracY * 1000.0F);
        jetzt.tlFrac = static_cast<int>(g_app->timelineFrac * 1000.0F);

        const App::LayoutLog& alt = g_app->lastLayoutLog;
        bool reiterAnders =
            jetzt.tabs != alt.tabs || jetzt.activeTab != alt.activeTab ||
            jetzt.splitCount != alt.splitCount ||
            jetzt.focusPane != alt.focusPane;
        for (int i = 0; i < App::kMaxSplit; ++i) {
            if (jetzt.paneTabs[i] != alt.paneTabs[i]) { reiterAnders = true; }
        }
        // Groessen erst ab EINEM Bildpunkt Unterschied - beim Ziehen aendern
        // sie sich sonst in jedem Bild um Bruchteile, und das Protokoll
        // liefe voll.
        const bool groessenAnders =
            jetzt.frameW != alt.frameW || jetzt.frameH != alt.frameH ||
            jetzt.eventsW != alt.eventsW || jetzt.flowW != alt.flowW ||
            jetzt.toolH != alt.toolH || jetzt.tooSmall != alt.tooSmall ||
            jetzt.fracX != alt.fracX ||
            jetzt.fracY != alt.fracY || jetzt.tlFrac != alt.tlFrac;

        if (reiterAnders) {
            char z[300];
            std::snprintf(z, sizeof(z),
                          "Reiter: %d offen, Arbeitsbereich %d, bearbeitet "
                          "wird %d | Felder: %d, Fokus %d, belegt mit "
                          "%d %d %d %d",
                          jetzt.tabs, g_app->homeTab, jetzt.activeTab,
                          jetzt.splitCount,
                          jetzt.focusPane, jetzt.paneTabs[0], jetzt.paneTabs[1],
                          jetzt.paneTabs[2], jetzt.paneTabs[3]);
            diag::detail(z);
            // Und was in den Feldern steht - der haeufigste Zweifel ist,
            // ob zwei Felder dasselbe Skript zeigen.
            for (int i = 0; i < jetzt.splitCount; ++i) {
                const int nr = g_app->splitPanes[i].tab;
                const char* name =
                    (nr >= 0 && static_cast<std::size_t>(nr) <
                                    g_app->tabs.size())
                        ? (g_app->tabs[static_cast<std::size_t>(nr)]
                                   .shownName.empty()
                               ? "(ohne Namen)"
                               : g_app->tabs[static_cast<std::size_t>(nr)]
                                     .shownName.c_str())
                        : "(kein Reiter)";
                std::snprintf(z, sizeof(z), "   Feld %d zeigt Reiter %d: %s", i,
                              nr, name);
                diag::detail(z);
            }
        }
        // Den Moduswechsel EINMAL je Bild feststellen - hier, weil diese
        // Stelle in jedem Modus laeuft.
        g_app->moduswechselBild = (g_app->letzterModus != g_app->leftMode);
        // Die Zeitleiste laeuft nur in der Kartenansicht weiter. Wer sie
        // beim Abspielen verlaesst, hatte eine stehende Uhr und weiter
        // laufenden Ton - also anhalten, wie "Stop" (Kartentest 27.09.).
        if (g_app->moduswechselBild && g_app->letzterModus == 1 && g_app->playing) {
            g_app->playing = false;
            g_app->audio.stopAll();
            g_app->laufendeSchleifen.clear();
            tonZuruecksetzen();   // rc601 ton: auch der Raumklang
            g_app->musicAudio.stopAll();
        }
        if (g_app->moduswechselBild) {
            char zm[140];
            std::snprintf(zm, sizeof(zm),
                          "Moduswechsel %d -> %d: dieses Bild wird "
                          "uebersprungen (Einschwingen)",
                          g_app->letzterModus, g_app->leftMode);
            diag::detail(zm);
            g_app->letzterModus = g_app->leftMode;
        }
        if (groessenAnders || g_app->dumpAngefordert) {
            g_app->dumpAngefordert = false;
            char z[300];
            std::snprintf(z, sizeof(z),
                          "Groessen: Fenster %dx%d | Ereignisse %d, Skripte "
                          "%d, Band %d | Raster %d/%d, Zeitleiste %d (Promille)",
                          jetzt.frameW, jetzt.frameH, jetzt.eventsW,
                          jetzt.flowW, jetzt.toolH, jetzt.fracX, jetzt.fracY,
                          jetzt.tlFrac);
            diag::detail(z);

            // --- ALLES, was wir je gemessen haben ------------------------
            //
            // shank: "wenn ich dir sage debuggen, dann will ich, dass er es
            // in eines der Logs schreibt. Also adde alles, wo ich dir mal
            // gesagt habe debugge es ins Log. Jede Position des UI,
            // Funktionen, alles."
            //
            // Bis hierher landete nur die Zeile darueber im Protokoll. Alles
            // andere, was in dieser Sitzung gemessen wurde, stand auf dem
            // Bildschirm - den ich nicht sehe. Jede dieser Zahlen hat uns
            // mindestens eine Runde gekostet:
            //
            //   splitEvents/Map/Model  drei Modi, drei Werte (rc494)
            //   frameH negativ         "alles Uebrige minus Fuss" (rc471)
            //   buttonsW/minButtonsW   Actions-Spalte (rc474, rc481)
            //   headerH je Modus       Knopfhoehe gegen Textzeile (rc478)
            //   ZelleW/restW/BildW     die Luecke neben der Karte (rc500)
            //   modeBarW               Modusleiste in der Tabelle (rc501)
            //
            // Auch nur bei Aenderung - `groessenAnders` deckt das ab.
            char z2[400];
            std::snprintf(z2, sizeof(z2),
                          "  Aufteilung: Modus %d | splitEvents %.4f, "
                          "splitMap %.4f, splitModel %.4f, splitButtons %.4f",
                          g_app->leftMode,
                          static_cast<double>(g_app->settings.splitEvents),
                          static_cast<double>(g_app->settings.splitMap),
                          static_cast<double>(g_app->settings.splitModel),
                          static_cast<double>(g_app->settings.splitButtons));
            diag::detail(z2);
            std::snprintf(z2, sizeof(z2),
                          "  Hoehen: headerH %.0f, fussH %.0f, frameH %.0f "
                          "(negativ = Rest minus Fuss), statusH %.0f, "
                          "toolH %.0f",
                          static_cast<double>(l.headerH),
                          static_cast<double>(l.fussH),
                          static_cast<double>(l.frameH),
                          static_cast<double>(l.statusH),
                          static_cast<double>(l.toolH));
            diag::detail(z2);
            std::snprintf(z2, sizeof(z2),
                          "  Breiten: buttonsW %.0f, minButtonsW %.0f, "
                          "modeBarW %.0f | Karte: ZelleW %.0f, restW %.0f, "
                          "BildW %.0f, SeitenspalteW %.0f",
                          static_cast<double>(l.buttonsW),
                          static_cast<double>(l.minButtonsW),
                          static_cast<double>(g_app->modeBarW),
                          static_cast<double>(g_app->dbgZelleW),
                          static_cast<double>(g_app->dbgRestW),
                          static_cast<double>(g_app->dbgBildW),
                          static_cast<double>(g_app->mapSidebarW));
            diag::detail(z2);
            std::snprintf(z2, sizeof(z2),
                          "  Stil: Zeichenhoehe %.0f, FramePadding.x %.0f, "
                          "CellPadding.x %.0f, ItemSpacing.x %.0f, "
                          "WindowPadding.x %.0f",
                          static_cast<double>(ImGui::GetFontSize()),
                          static_cast<double>(ImGui::GetStyle().FramePadding.x),
                          static_cast<double>(ImGui::GetStyle().CellPadding.x),
                          static_cast<double>(ImGui::GetStyle().ItemSpacing.x),
                          static_cast<double>(
                              ImGui::GetStyle().WindowPadding.x));
            diag::detail(z2);
        }
        // --- Und die REGELN nachprüfen ------------------------------------
        //
        // Ein Protokoll sagt, was gilt - nicht, ob es richtig ist. Diese
        // Bedingungen muessen immer gelten; bricht eine, liegt ein Fehler
        // vor, und er soll sich selbst melden statt beim Anwender
        // aufzufallen.
        //
        // Alle vier stammen aus echten Fehlern dieser Sitzung.
        if (reiterAnders) {
            char z[300];
            // rc179: neue Felder bekamen immer Reiter 0.
            for (int i = 0; i < jetzt.splitCount; ++i) {
                for (int j = i + 1; j < jetzt.splitCount; ++j) {
                    if (g_app->splitPanes[i].tab == g_app->splitPanes[j].tab) {
                        std::snprintf(z, sizeof(z),
                                      "   FEHLER: Feld %d und %d zeigen "
                                      "denselben Reiter %d",
                                      i, j, g_app->splitPanes[i].tab);
                        diag::detail(z);
                    }
                }
            }
            // rc184: nach dem Schliessen zeigte ein Feld ins Leere.
            for (int i = 0; i < jetzt.splitCount; ++i) {
                const int nr = g_app->splitPanes[i].tab;
                if (nr < 0 || static_cast<std::size_t>(nr) >=
                                  g_app->tabs.size()) {
                    std::snprintf(z, sizeof(z),
                                  "   FEHLER: Feld %d zeigt auf Reiter %d, es "
                                  "gibt aber nur %d",
                                  i, nr, jetzt.tabs);
                    diag::detail(z);
                }
            }
            if (jetzt.focusPane < 0 || jetzt.focusPane >= jetzt.splitCount) {
                std::snprintf(z, sizeof(z),
                              "   FEHLER: Fokus liegt auf Feld %d, es gibt "
                              "aber nur %d",
                              jetzt.focusPane, jetzt.splitCount);
                diag::detail(z);
            }
            // rc184: mehr Felder als Skripte.
            if (jetzt.splitCount > jetzt.tabs) {
                std::snprintf(z, sizeof(z),
                              "   FEHLER: %d Felder bei nur %d Reitern",
                              jetzt.splitCount, jetzt.tabs);
                diag::detail(z);
            }
        }
        if (groessenAnders) {
            char z[300];
            // rc202: die Skriptspalte klemmte, bevor die Symbolansicht
            // erreichbar war. Null waere schlimmer - dann kaeme man nicht
            // zurueck.
            if (jetzt.flowW <= 0 || jetzt.eventsW <= 0) {
                std::snprintf(z, sizeof(z),
                              "   FEHLER: eine Spalte ist auf null "
                              "(Ereignisse %d, Skripte %d)",
                              jetzt.eventsW, jetzt.flowW);
                diag::detail(z);
            }
            // rc195: das Band darf das Fenster nicht ueberragen.
            //
            // Die Bedingung stand frueher als `toolH > frameH + toolH` da -
            // algebraisch dasselbe wie `frameH < 0`, nur schwerer zu lesen.
            // Und sie kann seit rc258 nicht mehr anschlagen, weil frameH
            // bei null abgefangen wird. Gefragt wird deshalb den Vermerk,
            // den die Berechnung selbst hinterlaesst.
            if (jetzt.tooSmall) {
                diag::detail("   Fenster zu klein: nach Statuszeile, Band, "
                             "Reiterleiste und Kopfzeile bleibt nichts "
                             "uebrig. Die Hoehen sind bei null abgefangen.");
            }
        }
        if (reiterAnders || groessenAnders) {
            g_app->lastLayoutLog = jetzt;
        }

        // --- Wohin ging die letzte BEARBEITUNG? ---------------------------
        //
        // Beim Arbeiten an vier Skripten nebeneinander ist das DIE Frage:
        // landet die Aenderung in dem Skript, in das ich geklickt habe?
        //
        // Die Tiefe des Rueckgaengig-Stapels ist der verlaessliche Anzeiger:
        // sie waechst genau dann, wenn wirklich etwas geaendert wurde - und
        // nicht schon beim Anklicken oder Rollen.
        //
        // Beim REITERWECHSEL wechselt auch der Stapel; dann wird nur der
        // neue Stand gemerkt, ohne eine Bearbeitung zu melden.
        {
            const std::size_t tiefe = g_app->doc.undoDepth();
            if (g_app->activeTab != g_app->lastEditTab) {
                g_app->lastEditTab = g_app->activeTab;
                g_app->lastUndoDepth = tiefe;
            } else if (tiefe != g_app->lastUndoDepth) {
                const bool mehr = tiefe > g_app->lastUndoDepth;
                g_app->lastUndoDepth = tiefe;
                const std::string name =
                    !g_app->path.empty() ? fileName(g_app->path)
                    : !g_app->shownName.empty() ? g_app->shownName
                                                : std::string("unnamed.txt");
                char z[300];
                std::snprintf(z, sizeof(z),
                              "Bearbeitung: Feld %d, Reiter %d (%s), Stapel "
                              "%zu (%s)",
                              g_app->focusPane, g_app->activeTab, name.c_str(),
                              tiefe, mehr ? "Schritt dazu"
                                          : "rueckgaengig oder verworfen");
                diag::detail(z);

                // Die Regel, die das Ganze traegt: das FOKUSSIERTE Feld muss
                // den aktiven Reiter zeigen. Gilt sie nicht, ginge eine
                // Aenderung in ein anderes Skript als das angeklickte - und
                // das waere der schlimmste Fehler, den dieses Fenster haben
                // kann.
                const int fp =
                    std::clamp(g_app->focusPane, 0, App::kMaxSplit - 1);
                if (g_app->splitCount > 1 &&
                    g_app->splitPanes[fp].tab != g_app->activeTab) {
                    std::snprintf(z, sizeof(z),
                                  "   FEHLER: Feld %d zeigt Reiter %d, "
                                  "bearbeitet wurde aber Reiter %d",
                                  fp, g_app->splitPanes[fp].tab,
                                  g_app->activeTab);
                    diag::detail(z);
                }
            }
        }
    }
    // Bei der KARTE eine eigene Aufteilung.
    //
    // Die Knopfspalte "Actions" gehoert zum Skript, nicht zur Karte - dort
    // steht sie nur im Weg. Stattdessen:
    //
    //     Modusleiste | Ebenen | Karte | Skriptbaum
    //
    // Die Ebenen links, weil man sie liest wie eine Gliederung; der
    // Skriptbaum rechts, weil er beim Anklicken einer Entity die Antwort
    // gibt. Alles andere zur Karte steht im Band darunter.
    // --- Die Spalten als TABELLE ----------------------------------------
    //
    // Vorher: `drawEventsList`, Teiler, `drawFlowArea`, Teiler,
    // `drawButtonColumns` - aneinandergereiht mit `SameLine(0, 0)`, jede
    // Breite von Hand gerechnet, jeder Teiler selbst gezeichnet.
    //
    // Drei Runden sind an der Frage gescheitert, warum die Spalten nicht
    // gleich hoch enden (rc469, rc470, rc471). Der Grund war nie ein
    // einzelner Zahlendreher, sondern dass ueberhaupt gerechnet wurde:
    // sechs Werte aus einem Dutzend Summanden, und jede Aenderung muss an
    // jeder Stelle nachgezogen werden.
    //
    // ImGui hat dafuer Tabellen (seit 1.80). Sie rechnen die Breiten,
    // bringen die Trennlinien samt Ziehen mit - und eine Tabellenzeile ist
    // EINE Zeile: alle Zellen enden dort, wo sie endet. Nicht "sollten
    // gleich hoch sein", sondern koennen gar nicht anders.
    //
    // Was damit wegfaellt: `l.eventsW`, `l.flowW`, `l.buttonsW`, `l.rest`,
    // die beiden `splitter()`-Aufrufe und die gespeicherten
    // Aufteilungsbruechte - ImGui merkt sich die Spaltenbreiten selbst.
    //
    // Bei der KARTE hat die Knopfspalte nichts zu suchen: sie gehoert zum
    // Skript. Dann sind es zwei Spalten statt drei.
    //
    // Stufe 1b: der Karten-Editor ist eine eigene Oberflaeche - Modusleiste
    // links, rechts daneben das ganze NRC-Fenster (drawKartenEditorBereich).
    if (g_app->leftMode == 3) {
        drawKartenEditorBereich(l);
    } else {
        const bool mitKnoepfen = (g_app->leftMode != 1);
        const int spalten = mitKnoepfen ? 3 : 2;
        // --- KEINE gespeicherten Spaltenbreiten -------------------------
        //
        // ImGui legt Spaltenbreiten in der `imgui.ini` ab und stellt sie
        // beim naechsten Start wieder her. Das klingt hilfreich und ist
        // hier die Ursache eines Fehlers, den shank zweimal gesehen hat:
        //
        // Er hatte die Knopfspalte gezogen, als sie das noch zuliess. Die
        // Breite steht seither in seiner .ini. rc481 hat die Spalte auf
        // `NoResize` gesetzt - die gespeicherte Breite gilt trotzdem
        // weiter, und auf dem letzten Bild ist die Spalte auf sechzig
        // Punkte gequetscht: "Dele", "Dup", "Past".
        //
        // Die Recherche zu rc481 hatte genau das benannt: eine
        // veraenderbare Spalte "hands the column width to ImGui's table
        // state, which initialises once and then auto-fits or RESTORES FROM
        // SAVED SETTINGS".
        //
        // behaved speichert seine Aufteilung selbst, in den eigenen
        // Einstellungen. Eine zweite Fassung davon in der .ini ist genau
        // das Muster, das hier schon achtmal auseinandergelaufen ist.
        // Deshalb: nicht speichern.
        const ImGuiTableFlags fahnen =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_NoPadOuterX |
            ImGuiTableFlags_NoSavedSettings;
        // Die Hoehe ist NEGATIV - "alles Uebrige minus Fuss", siehe
        // computeLayout. Die Tabelle erbt sie an alle Zellen.
        // --- Mindestbreiten, damit nichts verschwindet ------------------
        //
        // Mit `splitter()` sind auch dessen Klammern weggefallen. shank:
        // "ich kann Script Flow komplett ueber das Fenster links ziehen,
        // und Actions kann ich zu weit nach rechts und zu weit nach links
        // ziehen."
        //
        // ImGui klemmt Spalten auf `table->MinColumnWidth`, und das entsteht
        // in `BeginTable` aus `FramePadding.x` (imgui_tables.cpp:905). Die
        // Vorgabe sind rund vier Punkte - praktisch keine Grenze.
        //
        // Fuer FESTE Spalten gilt das Vierfache (Zeile 1047), fuer gedehnte
        // das Einfache (1093). Mit `GetFontSize() * 3` ergibt das am
        // kopflosen Pruefstand:
        //
        //     gedehnt   39   Events und Script Flow koennen nicht mehr
        //                    verschwinden
        //     fest     156   die Actions-Spalte bleibt breit genug fuer
        //                    "Treeview Options"
        //
        // Der Griff ueber `FramePadding` ist ein Umweg - ImGui bietet keine
        // Mindestbreite je Spalte an. Deshalb wird er sofort wieder
        // zurueckgenommen, damit er nicht auch fuer den Inhalt gilt.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2{ImGui::GetFontSize() * 3.0F,
                                   ImGui::GetStyle().FramePadding.y});
        // --- EINE TABELLEN-ID JE MODUS -------------------------------------
        //
        // shank: "Script Flow ist immer noch ueber alle 3 Instanzen
        // verbunden."
        //
        // Er hat recht, und rc494 hat nur die halbe Ursache behoben. Drei
        // eigene Einstellungswerte nuetzen nichts, solange ImGui die
        // Spaltenbreite woanders haelt:
        //
        //   `TableSetupColumn(label, flags, init_width_or_weight)`
        //
        // Der Parameter heisst INIT_width_or_weight. Er gilt beim ANLEGEN.
        // Danach steht die Breite im Tabellenzustand, und der haengt an der
        // Tabellen-ID:
        //
        //   imgui_tables.cpp:358   table = g.Tables.GetOrAddByKey(id);
        //
        // Eine ID, ein Zustand. Alle drei Modi benutzten "##spalten" und
        // teilten sich deshalb die gezogene Breite - egal, was in den
        // Einstellungen stand.
        //
        // Drei Anordnungen sind drei Tabellen. Genau das, was shank schon
        // in rc479 gesagt hat: "alle 3 Modi als separate UIs betrachten."
        const char* const modusId = (g_app->leftMode == 1)   ? "##spaltenMap"
                                    : (g_app->leftMode == 2) ? "##spaltenModel"
                                                             : "##spaltenEvents";

        // Die FENSTERBREITE gehoert in die Kennung der Tabelle.
        //
        // Warum: `TableSetupColumn` uebernimmt die vorgegebene Breite nur,
        // solange die Tabelle sich einrichtet - in ImGuis Quelltext steht
        // sie unter `if (table->IsInitializing)`. Danach gehoert die Breite
        // der Tabelle, und was wir ihr sagen, wird verworfen.
        //
        // Das erklaert shanks Protokoll bis auf den Punkt:
        //
        //   Zeichenhoehe 24 -> minEvents = 24 * 14        = 336
        //   erstes Bild, Fenster 1485: 1485 * 0.2094      = 311
        //                              auf die Untergrenze angehoben -> 336
        //   spaeter gemeldet: jetzt = 336 + 12 Polster    = 348
        //   und 348 / 2536                                = 0.1372
        //
        // Die Tabelle richtet sich also ein, WAEHREND das Fenster noch auf
        // seiner kleinen Startgroesse steht. Dort greift die Untergrenze,
        // und dieser Notwert bleibt fuer immer stehen. Beim naechsten Start
        // wird er als "so hat er gezogen" gespeichert - Runde um Runde
        // dasselbe.
        //
        // rc556 wartete zwei Bilder. Das half nicht und konnte nicht helfen:
        // es ist kein Nachhinken, sondern Eigentum.
        //
        // Aendert sich die Fensterbreite, aendert sich die Kennung, und die
        // Tabelle richtet sich neu ein - mit dem Anteil, den der Benutzer
        // eingestellt hat. Beim ZIEHEN aendert sich die Fensterbreite nicht,
        // die Kennung bleibt, und die Tabelle behaelt seinen Zug.
        //
        // `gesamtB` weiter unten ist dieselbe Zahl, steht hier aber noch
        // nicht - also an dieser Stelle selbst messen.
        char idPuffer[64];
        std::snprintf(idPuffer, sizeof(idPuffer), "%s#%d", modusId,
                      static_cast<int>(ImGui::GetContentRegionAvail().x));
        const char* const tabellenId = idPuffer;

        // KEINE Hoehe fuer die Tabelle.
        //
        // Gemessen: `outer_size.y` wirkt bei Tabellen nur mit `ScrollY`.
        // Ohne die Fahne wird eine negative Hoehe stillschweigend
        // ignoriert - die Reservierung fiel weg, und die Spalten liefen
        // sechs Punkte UNTER den Innenrand.
        //
        // Richtig ist: die Tabelle passt sich an, und die Kindfenster in
        // den Zellen halten den Fuss frei.
        const bool offen =
            ImGui::BeginTable(tabellenId, spalten, fahnen, ImVec2{0.0F, 0.0F});
        // Sofort zurueck: die groessere Polsterung galt nur fuer das Anlegen
        // der Tabelle, nicht fuer die Knoepfe darin.
        ImGui::PopStyleVar();
        if (offen) {
            // --- Die Gewichte haengen am MODUS --------------------------
            //
            // Meine erste Fassung nahm feste 0,22 und 0,58 - und die
            // Kartenansicht steckte damit in der schmalen Spalte, weil sie
            // in DERSELBEN Zelle liegt wie die Ereignisliste. Auf shanks
            // Bild ist die Karte nur noch ein Streifen.
            //
            // Die Aufteilung war schon immer eine andere je Modus
            // (`splitEvents` gegen `splitMap`, computeLayout). Die Zahlen
            // bleiben, sie sind jetzt nur Gewichte der Tabelle statt selbst
            // ausgerechneter Breiten.
            // JE MODUS ein eigener Wert.
            //
            // Vorher `leftMode != 0 ? splitMap : splitEvents` - Map und
            // Model teilten sich einen. Wer im einen zog, verstellte den
            // anderen.
            const float linksAnteil =
                (g_app->leftMode == 1)   ? g_app->settings.splitMap
                : (g_app->leftMode == 2) ? g_app->settings.splitModel
                                         : g_app->settings.splitEvents;
            const float gesamtB = ImGui::GetContentRegionAvail().x;
            // Die vorige Breite MERKEN, nicht gleich ueberschreiben - sonst
            // ist der Vergleich unten immer wahr und die Pruefung wertlos.
            const float vorigeB = g_app->letzteFensterB;
            // --- FEST vorn, GEDEHNT in der Mitte, FEST hinten ------------
            //
            // Die Anleitung zu den Groessenregeln (imgui_tables.cpp, Kopf):
            //
            //   "Fixed Columns will generally obtain their requested width
            //    (unless the table cannot fit them all). Stretch Columns
            //    will share the remaining width. [...] The typical use of
            //    mixing sizing policies is: any number of LEADING Fixed
            //    columns, followed by one or two TRAILING Stretch columns."
            //
            // Eine Mindestbreite fuer GEDEHNTE Spalten gibt es nicht - das
            // ist eine offene Anfrage im ImGui-Projekt (Issue #5685). Am
            // kopflosen Pruefstand nachgemessen: bei 900 Punkten
            // Fensterbreite wurden die gedehnten auf 159 und 565
            // zusammengequetscht, waehrend die FESTE Spalte ihre 142 hielt.
            //
            // Also ist Events fest, und die Grenze setze ich selbst - beim
            // Uebergeben, nicht beim Ziehen. Dann gilt sie auch, wenn das
            // Fenster schmaler wird.
            // --- Die Untergrenze haengt am MODUS -------------------------
            //
            // 14 Zeichenhoehen reichen fuer eine Ereignisliste. In den
            // Modi Map und Model steckt in derselben Spalte aber die ganze
            // Ansicht: Modusleiste, Ebenen, das Bild UND die
            // Einstellungsspalte rechts davon. Bei 14 passt davon nichts
            // hinein - auf shanks Bild fehlt die Karte einfach.
            //
            // Erschwerend: die Rueckkopplung aus rc484 hat den kleinen Wert
            // schon in seine Einstellungen geschrieben. rc490 hat das
            // Schrumpfen gestoppt, den gespeicherten Wert aber nicht
            // repariert. Eine Untergrenze beim BENUTZEN tut beides.
            const float minEvents = ImGui::GetFontSize() *
                                    ((g_app->leftMode == 0) ? 14.0F : 46.0F);
            // Der Script Flow behaelt mindestens so viel - sonst zieht man
            // ihn oder die Actions ueber die Nachbarspalte (shank: "das
            // Script-Flow-Fenster laesst sich komplett ueber das Fenster vom
            // Renderer ziehen").
            const float minFlow = ImGui::GetFontSize() * 10.0F;
            const float maxEvents =
                std::max(minEvents, gesamtB - l.buttonsW - minFlow);
            const float breiteEvents =
                std::clamp(gesamtB * linksAnteil, minEvents, maxEvents);
            // NoResize: gezogen wird ueber eigene Griffe (unten, nach
            // EndTable). ImGuis eigenes Ziehen verschiebt Breiten zwischen
            // festen und gedehnten Nachbarn und liess sich nicht zuverlaessig
            // begrenzen - im Test quetschte es Actions auf 97 statt 184 und
            // die Karte auf 0 (27.09.).
            ImGui::TableSetupColumn("Events",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                    breiteEvents);
            // Auch der Script Flow FEST, als Rest ausgerechnet (siehe unten):
            // bei gemischten festen und gedehnten Spalten verteilt ImGui den
            // Rest selbst und gab der linken Spalte 37 Punkte mehr als
            // gewuenscht - genau die fehlten dem Script Flow (27.09.).
            ImGui::TableSetupColumn("Flow",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                    std::max(minFlow, gesamtB - breiteEvents - (mitKnoepfen ? l.buttonsW : 0.0F) -
                                                          g_app->spaltenUeberhang));
            if (mitKnoepfen) {
                // Feste Breite, und zwar die GEWUENSCHTE, nicht die
                // kleinstmoegliche: `minButtonsW` ist die Untergrenze, bei
                // der die Beschriftungen gerade noch hineinpassen. Damit
                // waren "Treeview Options" und "Fold Macros" abgeschnitten.
                // --- Ziehbar, aber mit Klammern ------------------------
                //
                // rc481 hatte `NoResize` gesetzt. Das war zu grob: der
                // Griff zwischen Script Flow und Actions gehoert in ImGui
                // zur LINKEN Spalte, und die ist gedehnt. Wer ihn anfasst,
                // verschiebt deshalb den Teiler zwischen Events und Script
                // Flow - shank: "wenn ich den Actions-Slider waehle,
                // bewegt sich der linke Slider."
                //
                // Beides geht. Was die drei Fehler verursacht hat, war
                // nicht das Ziehen, sondern was daneben fehlte:
                //
                //   Untergrenze   `MinColumnWidth * 4` = 156, aus der
                //                 Polsterung (rc480) - die Spalte kann
                //                 nicht mehr verschwinden
                //   Nachbarn      ImGui haelt sie mindestens 39 breit
                //                 (imgui_tables.cpp:1571), solange
                //                 `NoKeepColumnsVisible` NICHT gesetzt ist
                //   Gedaechtnis   `NoSavedSettings` (rc482) - keine alte
                //                 Breite aus der .ini
                //
                // Eine echte Hoechstbreite bietet ImGui nicht an. Wer sie
                // braucht, muss den Griff selbst zeichnen und die Breite
                // selbst halten - das war frueher `splitter()`. Solange
                // die Nachbarn sichtbar bleiben, reicht die Klammer von
                // ImGui.
                ImGui::TableSetupColumn("Actions",
                                        ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                        l.buttonsW);
            }
            // --- Die Breiten JEDES Bild aus den Anteilen, geklemmt ----------
            //
            // Actions: mindestens so breit, dass die Beschriftungen passen,
            // hoechstens 45 %. Links: Events 14, Karte/Model 46 Zeichen. Der
            // Script Flow behaelt in jedem Fall minFlow (plus die Zellraender,
            // sonst fehlen ihm genau die). Gezogen wird an eigenen Griffen,
            // die die Anteile aendern - siehe nach EndTable.
            {
                // Die Zellraender und Trennlinien, im letzten Bild GEMESSEN
                // (Gesamtbreite minus Summe der Spalten) - nicht geschaetzt.
                const float spiel = g_app->spaltenUeberhang;
                // --- Kein Teiler schiebt eine andere Spalte weg -------------
                //
                // shank, 28.09., mit Protokoll: "wenn die linke Spalte zu nah
                // an der rechten Actions ist, dann passiert das". Stand der
                // Script Flow schon auf seiner Mindestbreite, durfte Actions
                // trotzdem wachsen (ihre Grenze rechnete mit der MINDESTbreite
                // der linken Spalte) - und schob Events weg, ohne dass deren
                // Anteil sich aenderte. Wurde Actions wieder schmaler, sprang
                // Events von selbst zurueck. Der linke Teiler dagegen stiess an.
                //
                // Jetzt fuer beide gleich: die linke Spalte zuerst, geklemmt so,
                // dass Actions ihre Mindestbreite behaelt; dann Actions, geklemmt
                // an das, was neben der linken Spalte und dem Script Flow
                // wirklich frei ist. Beim Ziehen gilt fuer links dasselbe mit der
                // aktuellen Actions-Breite - wer den Script Flow auf seine
                // Mindestbreite drueckt, stoesst an, und die dritte Spalte bleibt
                // stehen.
                const float minB = mitKnoepfen ? l.minButtonsW : 0.0F;
                const float maxLAnordnung = std::max(minEvents, gesamtB - minB - minFlow - spiel);
                const float zielL = std::floor(std::clamp(gesamtB * linksAnteil, minEvents, maxLAnordnung));
                float zielB = 0.0F;
                if (mitKnoepfen) {
                    const float maxB = std::max(
                        l.minButtonsW, std::min(gesamtB * 0.45F, gesamtB - zielL - minFlow - spiel));
                    // Ganze Bildpunkte: ImGui rundet WidthGiven ab, und ein
                    // gebrochener Rest schwang ueber die gemessenen
                    // Zellraender von Bild zu Bild um einen Punkt hin und
                    // her (Ruhetest "Karte nach dem Laden", 28.09.).
                    zielB = std::floor(std::clamp(gesamtB * g_app->settings.splitButtons, l.minButtonsW, maxB));
                    g_app->spaltenMin[2] = l.minButtonsW;
                    g_app->spaltenMax[2] = maxB;
                }
                const float maxL = std::max(minEvents, gesamtB - zielB - minFlow - spiel);
                g_app->spaltenMin[0] = minEvents;
                g_app->spaltenMax[0] = maxL;
                g_app->spaltenMin[1] = minFlow;
                g_app->spaltenGesamt = gesamtB;
                // Erst ab dem zweiten Bild: vorher hat die Tabelle ihre
                // Mindestspaltenbreite noch nicht berechnet, und ImGui sichert
                // das zu (imgui_tables.cpp, TableSetColumnWidth).
                if (ImGuiTable* tabG = ImGui::GetCurrentTable(); tabG != nullptr && tabG->MinColumnWidth > 0.0F) {
                    // Der Script Flow ist der Rest - so fuellen die drei
                    // Spalten die Breite genau.
                    const float zielF = std::floor(std::max(minFlow, gesamtB - zielL - zielB - spiel));
                    // DIREKT setzen, nicht ueber TableSetColumnWidth: das
                    // klemmt jede Spalte auf WidthMax aus dem VORIGEN Bild.
                    // Wuchs Actions beim Ziehen des rechten Teilers, war ihr
                    // Hoechstwert noch der alte (der Script Flow war im
                    // letzten Bild breiter) - sie bekam weniger, der Script
                    // Flow schrumpfte voll, und rechts neben Actions blieb
                    // eine Luecke, die mit jeder Mausbewegung wuchs und
                    // schrumpfte (shank, 28.09.). Ausserdem kehrt es frueh
                    // zurueck, wenn die GEGEBENE Breite schon stimmt, und
                    // laesst dann einen falschen Wunsch stehen. Die drei
                    // Ziele sind oben geklemmt und fuellen die Breite genau;
                    // alle Spalten sind fest (WidthFixed), also nimmt
                    // TableUpdateLayout WidthRequest, wie es ist.
                    const float ziele[3] = {zielL, zielF, zielB};
                    for (int k = 0; k < spalten && k < 3; ++k) {
                        const float z = std::max(ziele[k], tabG->MinColumnWidth);
                        if (std::fabs(tabG->Columns[k].WidthRequest - z) > 0.5F) {
                            tabG->Columns[k].WidthRequest = z;
                            tabG->Columns[k].WidthAuto = z;
                        }
                    }
                    g_app->spaltenZiel[1] = zielF;
                    g_app->spaltenZiel[0] = zielL;
                    g_app->spaltenZiel[2] = zielB;
                    for (int k = 0; k < spalten && k < 3; ++k) {
                        g_app->spaltenReq[k] = tabG->Columns[k].WidthRequest;
                        g_app->spaltenGiven[k] = tabG->Columns[k].WidthGiven;
                    }
                }
            }
            ImGui::TableNextRow();
            // Was die Anordnung jetzt wirklich ist (fuer den Selbsttest), und
            // wie viel Zellraender und Trennlinien kosten.
            if (const ImGuiTable* tabI = ImGui::GetCurrentTable(); tabI != nullptr) {
                float summe = 0.0F;
                for (int k = 0; k < spalten; ++k) { summe += tabI->Columns[k].WidthGiven; }
                if (summe > 1.0F) {
                    // Erst bei einer Aenderung von mindestens ZWEI Punkten
                    // uebernehmen: sonst regelt die Messung ihre eigene
                    // Rundung nach, und die Actions-Knoepfe zappelten nach
                    // dem Kartenladen zwischen 281 und 282 Punkten Breite hin
                    // und her (Ruhetest "Karte nach dem Laden", 28.09. - der
                    // Fehler bestand schon vor dieser Nacht).
                    //
                    // NICHT "Gesamtbreite minus Summe": das misst eine Luecke
                    // rechts mit, und weil der Script Flow um genau diesen Wert
                    // schmaler gerechnet wird, bestaetigte sich eine einmal zu
                    // grosse Zahl jedes Bild selbst. Beim Ziehen des rechten
                    // Teilers wuchs sie so bis an die Klemme (6 Zeichen), alle
                    // Spalten rueckten nach links, und erst Minimieren setzte
                    // sie zurueck (shank, 28.09., mit Bildern). Gezaehlt wird
                    // jetzt nur, was die Spalten SELBST an Rand verbrauchen
                    // (Ausdehnung minus Inhalt), plus der linke Rahmen fuer
                    // beide Seiten.
                    const int letzte = spalten - 1;
                    float rand = 2.0F * std::max(0.0F, tabI->Columns[0].MinX - tabI->OuterRect.Min.x);
                    for (int k = 0; k < spalten; ++k) {
                        rand += (tabI->Columns[k].MaxX - tabI->Columns[k].MinX) - tabI->Columns[k].WidthGiven;
                    }
                    const float neu = std::clamp(std::round(rand), 0.0F, ImGui::GetFontSize() * 6.0F);
                    if (std::fabs(neu - g_app->spaltenUeberhang) >= 2.0F) {
                        g_app->spaltenUeberhang = neu;
                    }
                    // Fuer den Selbsttest: was rechts neben der letzten Spalte
                    // frei bleibt (soll nur der Rahmen sein).
                    g_app->spaltenLuecke = tabI->OuterRect.Max.x - tabI->Columns[letzte].MaxX;
                }
                const int n = mitKnoepfen ? 3 : 2;
                for (int k = 0; k < 3; ++k) {
                    g_app->spaltenB[k] = (k < n) ? tabI->Columns[k].MaxX - tabI->Columns[k].MinX : 0.0F;
                }
                g_app->teilerX[0] = tabI->Columns[0].MaxX;
                g_app->teilerX[1] = mitKnoepfen ? tabI->Columns[1].MaxX : 0.0F;
                g_app->teilerAnzahl = mitKnoepfen ? 2 : 1;
            }

            ImGui::TableNextColumn();
            // Wer gezogen hat, aendert die Spaltenbreite. Die ZELLE kennt
            // sie - ein oeffentliches `TableGetColumnWidth` gibt es nicht.
            //
            // Zurueckgelesen und als Anteil gesichert wird sie in jedem
            // Bild: "clamped on every read rather than only when dragged",
            // damit ein schmaleres Fenster die Spalte verkleinert statt die
            // Nachbarn leerzuraeumen.
            // --- Nur sichern, wenn sich die Breite WIRKLICH geaendert hat
            //
            // shanks Protokoll, zwei Starts hintereinander:
            //
            //   geladen:   Ereignisse 0.743, Karte 0.871
            //   gesichert: Ereignisse 0.404, Karte 0.900
            //
            // Beide verstellt, obwohl er nur angeschaut hat. 0.900 ist
            // exakt meine obere Klemmgrenze - der Wert ist dagegengelaufen.
            //
            // Zwei Fehler von mir:
            //
            //   rc490  `IsMouseDown` greift bei JEDEM Linksklick irgendwo im
            //          Fenster, nicht nur beim Ziehen des Teilers. Jeder
            //          Klick auf einen Knopf schrieb die Breite neu.
            //   rc490  `+ CellPadding.x * 2` korrigiert zu VIEL. Vorher
            //          driftete der Wert nach unten, jetzt nach oben. Ich
            //          habe die Richtung getauscht statt die Drift zu
            //          beenden.
            //
            // Die Drift endet erst, wenn gar nicht geschrieben wird, solange
            // sich nichts geaendert hat. Gezogen wurde genau dann, wenn die
            // Tabelle eine ANDERE Breite hat als die, die wir ihr vorgegeben
            // haben. Ohne Ziehen sind beide gleich, und dann bleibt der
            // Wert unangetastet - egal wie oft das Bild neu entsteht.
            {
                const float jetzt = ImGui::GetContentRegionAvail().x +
                                    ImGui::GetStyle().CellPadding.x * 2.0F;
                // Eine Zeichenhoehe Spielraum: Rundung und Polster sollen
                // nicht als Ziehen durchgehen.
                const float spielraum = ImGui::GetFontSize();
                // Und NICHT, waehrend sich das Fenster aendert.
                //
                // shanks Detailprotokoll:
                //
                //   Fenster 1485x929  | Ereignisse  511
                //   Fenster 2536x1326 | Ereignisse  928
                //   Fenster 2536x1056 | Ereignisse 2260
                //
                // Er hat das Fenster vergroessert, nicht den Teiler gezogen.
                // Eine feste Spalte behaelt dabei ihre Pixelbreite; der
                // Anteil aendert sich von allein, und mein Vergleich hielt
                // das fuer ein Ziehen.
                //
                // Antwort auf seine Frage "soll ich nicht ziehen?": doch.
                // Nur soll das FENSTER nichts verstellen.
                // ZWEI Bilder Ruhe, nicht eines.
                //
                // Gemessen in shanks rc555-Protokoll, unmittelbar nachdem
                // das Fenster von 1485 auf 2536 gewachsen ist:
                //
                //   Teiler geschrieben: jetzt 348, vorgegeben 572,
                //                       gesamt 2536, vorige 2536
                //
                //   vorgegeben  = 0.2256 * 2536 = 572
                //   jetzt       = 348, davon 12 Zellenpolster -> 336
                //   0.2256 * 1485 (die ALTE Breite)            -> 335
                //
                // Die Tabelle meldete also noch die Breite, die sie beim
                // schmalen Fenster hatte: eine ImGui-Tabelle uebernimmt
                // eine neue Spaltenbreite erst im NAECHSTEN Bild.
                //
                // Die Pruefung von rc490 sah nur ein Bild zurueck. Im Bild
                // nach der Aenderung galt das Fenster als stabil, die
                // Tabelle hinkte aber noch hinterher - und die Differenz
                // wurde als Ziehen gedeutet. Ergebnis:
                //
                //   Anordnung geladen:   Ereignisse 0.226
                //   Anordnung gesichert: Ereignisse 0.137
                //
                // Beim Ziehen aendert sich die Fensterbreite nicht, der
                // Zaehler laeuft also weiter hoch - echtes Ziehen bleibt
                // erkennbar.
                if (vorigeB > 0.0F && std::abs(gesamtB - vorigeB) < 1.0F) {
                    if (g_app->fensterStabilSeit < 1000) {
                        ++g_app->fensterStabilSeit;
                    }
                } else {
                    g_app->fensterStabilSeit = 0;
                }
                const bool fensterStabil = g_app->fensterStabilSeit >= 2;
                // UND: ImGui muss sagen, dass gezogen wird.
                //
                // rc499 hatte das schon einmal und es ist mir bei einem
                // spaeteren Umbau abhandengekommen. `lint_rueckkopplung`
                // haelt in seiner eigenen Begruendung fest, warum es die
                // einzige taugliche Schranke ist:
                //
                //   rc490  IsMouseDown          jeder Klick zaehlte
                //   rc497  Breitenvergleich     Fenstergroesse zaehlt mit
                //   rc498  Fenster stabil       der Rest ratscht weiter
                //
                // Alle drei haben VERSUCHT zu erkennen, ob gezogen wurde.
                // Keine hat gefragt. `ResizedColumn` ist die Antwort:
                // wird nicht gezogen, steht dort -1.
                const ImGuiTable* tab = ImGui::GetCurrentTable();
                const bool wirdGezogen =
                    (tab != nullptr) && (tab->ResizedColumn != -1);
                if (gesamtB > 0.0F && fensterStabil && wirdGezogen &&
                    std::abs(jetzt - breiteEvents) > spielraum) {
                    const float anteil =
                        std::clamp(jetzt / gesamtB, 0.05F, 0.9F);
                    if (g_app->leftMode == 1) {
                        g_app->settings.splitMap = anteil;
                    } else if (g_app->leftMode == 2) {
                        g_app->settings.splitModel = anteil;
                    } else {
                        g_app->settings.splitEvents = anteil;
                    }
                    // WARUM hier geschrieben wurde.
                    //
                    // shanks rc554-Protokoll: der Anteil sprang bei
                    // GLEICHER Fensterbreite von 0.2094 auf 0.1372, ohne
                    // dass er den Teiler angefasst hat - und genau dieser
                    // Wert wurde beim Beenden gesichert:
                    //
                    //   Anordnung geladen:   Ereignisse 0.209
                    //   Anordnung gesichert: Ereignisse 0.137
                    //
                    // Die drei Schutzmassnahmen von rc490 greifen also
                    // nicht: das Fenster war stabil, und die Tabelle meldete
                    // trotzdem eine andere Breite als vorgegeben. Woher der
                    // Unterschied kommt, sagt keine Zahl im Protokoll - also
                    // steht sie jetzt drin.
                    char zt[220];
                    std::snprintf(zt, sizeof(zt),
                                  "Teiler geschrieben: Modus %d, ruhig seit "
                                  "%d Bildern, jetzt %.0f, "
                                  "vorgegeben %.0f, gesamt %.0f, vorige "
                                  "Fensterbreite %.0f -> Anteil %.4f",
                                  g_app->leftMode,
                                  g_app->fensterStabilSeit,
                                  static_cast<double>(jetzt),
                                  static_cast<double>(breiteEvents),
                                  static_cast<double>(gesamtB),
                                  static_cast<double>(vorigeB),
                                  static_cast<double>(anteil));
                    diag::detail(zt);
                }
                g_app->letzteFensterB = gesamtB;
            }
            drawEventsList(l);

            ImGui::TableNextColumn();
            drawFlowArea(l);

            if (mitKnoepfen) {
                ImGui::TableNextColumn();
                // Und die Actions-Spalte GENAUSO zuruecklesen.
                //
                // shank zu rc557: "also die linke Seite events wird
                // gespeichert, nur die rechte Seite bei actions wird nicht
                // gespeichert." Sein Protokoll bestaetigt beides:
                //
                //   Anordnung geladen:   Ereignisse 0.210
                //   Anordnung gesichert: Ereignisse 0.210     <- haelt
                //
                // und in der gesicherten Zeile taucht `splitButtons` gar
                // nicht auf.
                //
                // Der Grund: `settings.splitButtons` wird geschrieben,
                // gelesen und BENUTZT - aber nirgends aus einem Zug
                // zurueckgeschrieben. Die Spalte ist ziehbar, der Zug
                // landete nur nie in den Einstellungen. Die linke Seite
                // hatte diesen Weg seit rc490, die rechte nie.
                //
                // Dieselben drei Schutzpruefungen wie links: nur bei
                // ruhigem Fenster, nur bei echtem Unterschied, und mit
                // Klammern.
                {
                    const float jetztK = ImGui::GetContentRegionAvail().x +
                                         ImGui::GetStyle().CellPadding.x * 2.0F;
                    const float spielraumK = ImGui::GetFontSize();
                    // `fensterStabil` steht in einem Block, der weiter
                    // oben schon geschlossen hat - hier ist es nicht mehr
                    // sichtbar. Der Zaehler selbst schon: er gehoert zum
                    // Programmzustand.
                    // Dieselbe Frage wie links: zieht jemand gerade?
                    const ImGuiTable* tabK = ImGui::GetCurrentTable();
                    const bool zugK =
                        (tabK != nullptr) && (tabK->ResizedColumn != -1);
                    if (gesamtB > 0.0F && g_app->fensterStabilSeit >= 2 &&
                        zugK &&
                        std::abs(jetztK - l.buttonsW) > spielraumK) {
                        g_app->settings.splitButtons =
                            std::clamp(jetztK / gesamtB, 0.03F, 0.60F);
                        char zk[200];
                        std::snprintf(zk, sizeof(zk),
                                      "Actions geschrieben: ruhig seit %d "
                                      "Bildern, jetzt %.0f, vorgegeben %.0f, "
                                      "gesamt %.0f -> Anteil %.4f",
                                      g_app->fensterStabilSeit,
                                      static_cast<double>(jetztK),
                                      static_cast<double>(l.buttonsW),
                                      static_cast<double>(gesamtB),
                                      static_cast<double>(
                                          g_app->settings.splitButtons));
                        diag::detail(zk);
                    }
                }
                drawButtonColumns(l);
            }
            ImGui::EndTable();
            // Die GANZE Tabelle: erst nach EndTable steht ihre Hoehe fest -
            // vorher (nach TableNextRow) war es nur die Kopfzeile.
            g_app->tabelleOben = ImGui::GetItemRectMin().y;
            g_app->tabelleUnten = ImGui::GetItemRectMax().y;
            g_app->teilerY = (g_app->tabelleOben + g_app->tabelleUnten) * 0.5F;
            // --- Die eigenen Griffe auf den Trennlinien --------------------
            //
            // Wie die Kante der Zeitleiste: ein unsichtbarer Knopf auf der
            // Linie, der beim Ziehen den ANTEIL verschiebt. Die Klemmung
            // oben sorgt dafuer, dass keine Spalte unter ihre Grenze geht -
            // man stoesst an, statt ueber die Nachbarspalte zu ziehen.
            if (g_app->spaltenGesamt > 1.0F && g_app->tabelleUnten > g_app->tabelleOben) {
                const ImVec2 zurueck = ImGui::GetCursorScreenPos();
                const float griffB = std::max(4.0F, ImGui::GetStyle().ItemSpacing.x);
                const float gB = g_app->spaltenGesamt;
                for (int k = 0; k < g_app->teilerAnzahl; ++k) {
                    ImGui::SetCursorScreenPos(ImVec2{g_app->teilerX[k] - griffB, g_app->tabelleOben});
                    ImGui::InvisibleButton(k == 0 ? "##spalteLinks" : "##spalteRechts",
                                           ImVec2{griffB * 2.0F, g_app->tabelleUnten - g_app->tabelleOben});
                    const bool aktiv = ImGui::IsItemActive();
                    if (ImGui::IsItemActivated()) {
                        diag::detail(std::string("Teiler ") + (k == 0 ? "links" : "rechts") + " gepackt bei " +
                                     std::to_string(g_app->teilerX[k]));
                        g_app->teilerStartMaus = ImGui::GetIO().MousePos.x;
                        g_app->teilerStartBreite = g_app->spaltenZiel[k == 0 ? 0 : 2];
                    }
                    if (ImGui::IsItemHovered() || aktiv) {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                        ImGui::GetWindowDrawList()->AddLine(
                            ImVec2{g_app->teilerX[k], g_app->tabelleOben},
                            ImVec2{g_app->teilerX[k], g_app->tabelleUnten},
                            ImGui::GetColorU32(aktiv ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered), 2.0F);
                    }
                    if (aktiv) {
                        // ABSOLUT: Breite beim Anfassen plus der Weg der Maus
                        // seitdem. Vorher wurde jedes Bild die GEMESSENE
                        // Breite (samt Zellraendern) plus MouseDelta
                        // genommen - die Raender kamen so in jedem Bild
                        // dazu, die Spalte wuchs auch bei ruhiger Maus bis an
                        // die Grenze, und der Teiler lief der Maus davon
                        // (shank: "es wird entweder zu klein oder zu gross").
                        // Jetzt bleibt der Teiler unter der Maus, und an
                        // einer Grenze stoesst er an, bis die Maus zurueckkommt.
                        const float weg = ImGui::GetIO().MousePos.x - g_app->teilerStartMaus;
                        if (k == 0) {
                            float& anteil = (g_app->leftMode == 1)   ? g_app->settings.splitMap
                                            : (g_app->leftMode == 2) ? g_app->settings.splitModel
                                                                     : g_app->settings.splitEvents;
                            const float w = std::clamp(g_app->teilerStartBreite + weg, g_app->spaltenMin[0],
                                                       std::max(g_app->spaltenMin[0], g_app->spaltenMax[0]));
                            // +0.5: zielL rundet ab (std::floor), sonst fehlte
                            // bei manchen Breiten ein Punkt.
                            anteil = (w + 0.5F) / gB;
                        } else {
                            const float w = std::clamp(g_app->teilerStartBreite - weg, g_app->spaltenMin[2],
                                                       std::max(g_app->spaltenMin[2], g_app->spaltenMax[2]));
                            g_app->settings.splitButtons = (w + 0.5F) / gB;
                        }
                    }
                }
                ImGui::SetCursorScreenPos(zurueck);
            }
        }
    }

    // Die Bedienleiste UNTER allen drei Spalten, ueber die volle Breite.
    if (l.toolH > 0.0F && g_app->leftMode != 3) {
        // OHNE eigenen Hintergrund: ein Kasten mit eigener Farbe sieht wie
        // ein zusaetzlicher dunkler Balken aus. Die Bedienung soll auf der
        // Flaeche stehen, die ohnehin da ist.
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0, 0, 0, 0});
        // KEINE Rollbalken: das Band hat eine feste Hoehe, und ein Balken
        // wuerde sie von innen auffressen. Was nicht hineinpasst, wird
        // abgeschnitten - das faellt auf und laesst sich beheben, ein
        // stiller Hoehenverlust nicht.
        // --- Die ZIEHKANTE ueber dem Band --------------------------------
        //
        // Seit rc173 stellte man die Hoehe der Zeitleiste ueber einen
        // Prozentregler ein. Das ist umstaendlich: in Blender zieht man die
        // Kante zwischen zwei Bereichen, und genau das erwartet man hier
        // auch.
        //
        // VOR dem Kindfenster gezeichnet, nicht danach: der Griff liegt
        // ueber dem Band, und ein Kindfenster faengt Klicks in seinem
        // Bereich ab. Dieselbe Reihenfolge-Frage wie bei den Trennlinien
        // des Rasters in rc179 - dort war es genau andersherum, weil der
        // Griff DARIN lag.
        if (g_app->leftMode == 1) {
            const float kanteH =
                std::max(4.0F, ImGui::GetStyle().ItemSpacing.y);
            ImGui::InvisibleButton("##tlkante", ImVec2{-FLT_MIN, kanteH});
            g_app->tlKanteX = (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5F;
            g_app->tlKanteY = (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5F;
            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            if (ImGui::IsItemActivated()) {
                g_app->tlKanteStartMaus = ImGui::GetIO().MousePos.y;
                g_app->tlKanteStartHoehe = g_app->tlHoeheIst;
            }
            if (ImGui::IsItemActive() && g_app->tlPlatz > 1.0F) {
                // ABSOLUT: Hoehe beim Anfassen plus Weg der Maus, geteilt
                // durch denselben Platz, aus dem die Hoehe entsteht. Vorher
                // MouseDelta durch die FENSTERhoehe - die Kante lief langsamer
                // als die Maus, und unter der Mindesthoehe sammelte sich ein
                // Rest an, den man erst zurueckziehen musste.
                // Nach OBEN ziehen macht die Leiste GROESSER.
                const float weg = ImGui::GetIO().MousePos.y - g_app->tlKanteStartMaus;
                const float hoehe = g_app->tlKanteStartHoehe - weg;
                g_app->timelineFrac = std::clamp(hoehe / g_app->tlPlatz, 0.08F, 0.60F);
            }
            // Eine sichtbare Linie: einen Griff, den man nicht sieht,
            // findet niemand (siehe rc193, das Lineal).
            const ImVec2 kA = ImGui::GetItemRectMin();
            const ImVec2 kB = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2{kA.x, (kA.y + kB.y) * 0.5F - 1.0F},
                ImVec2{kB.x, (kA.y + kB.y) * 0.5F + 1.0F},
                ImGui::GetColorU32(ImGui::IsItemHovered() || ImGui::IsItemActive()
                                       ? ImGuiCol_SliderGrabActive
                                       : ImGuiCol_Separator));
        }
        ImGui::BeginChild("toolband", ImVec2{-FLT_MIN, l.toolH},
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
        if (g_app->leftMode == 2) {
            drawModelPanel();
        } else if (g_app->leftMode == 1) {
            // Alles zur Karte im Band, ueber die volle Breite: Bedienung,
            // die angeklickte Entity und die Zeitleiste.
            //
            // Die Ebenenliste steht NICHT hier, sondern als eigene Spalte
            // links neben der Karte - sie ist eine Gliederung und gehoert
            // neben das Bild, nicht darunter.
            // Seit rc588 nur noch die Zeitleiste: Koordinaten und "Insert
            // camera here" standen in einer eigenen Zeile darueber - die
            // Kamera-Einstellung ist jetzt ein Knopf der Transportleiste, die
            // angeklickte Entity steht an deren Ende (drawPickedEntity).
            drawTimeline();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // --- Die Aufteilung in Zahlen ---------------------------------------
    //
    // shank: "das mit dem Scaling nervt einfach so, kannst du Debug
    // einbauen fuer das Menue auch?"
    //
    // Zwoelf Runden lang habe ich Breiten geraten, er hat gebaut, und wir
    // haben Bilder verglichen. `tools/layoutcheck.cpp` misst das inzwischen
    // hier - aber nur den Aufbau, den es nachstellt, und nicht seine
    // Einstellungen auf seinem Bildschirm.
    //
    // Das hier zeigt DIESELBEN Zahlen im laufenden Programm. Wenn wieder
    // etwas klemmt, steht die Antwort da, statt dass wir sie erraten.
    if (g_app->layoutOpen) {
        if (ImGui::Begin(tr(Str::DebugLayoutTitle), &g_app->layoutOpen,
                         ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings)) {
            const ImGuiStyle& st = ImGui::GetStyle();
            // Nur Namen und Zahlen - Prosa gehoert uebersetzt, und ein
            // Debug-Auszug soll ablesbar sein, nicht erklaerend.
            ImGui::Text("leftMode         %d", g_app->leftMode);
            ImGui::Text("Fenster          %.0f x %.0f", ImGui::GetIO().DisplaySize.x,
                        ImGui::GetIO().DisplaySize.y);
            ImGui::Separator();
            ImGui::Text("splitEvents      %.4f", static_cast<double>(
                                                     g_app->settings.splitEvents));
            ImGui::Text("splitMap         %.4f",
                        static_cast<double>(g_app->settings.splitMap));
            ImGui::Text("splitModel       %.4f",
                        static_cast<double>(g_app->settings.splitModel));
            ImGui::Separator();
            ImGui::Text("ZelleW           %.0f",
                        static_cast<double>(g_app->dbgZelleW));
            ImGui::Text("restW            %.0f",
                        static_cast<double>(g_app->dbgRestW));
            ImGui::Text("BildW            %.0f",
                        static_cast<double>(g_app->dbgBildW));
            ImGui::Text("mapSidebarW      %.0f",
                        static_cast<double>(g_app->mapSidebarW));
            ImGui::Separator();
            // Ereignisfenster, Spalte 0. Die Differenz ist die Antwort:
            // 0 = beide Zeilen enden gleich.
            ImGui::Text("ed ZelleW        %.0f",
                        static_cast<double>(g_app->dbgZelle0));
            ImGui::Text("ed knopfB        %.0f",
                        static_cast<double>(g_app->dbgKnopfB));
            ImGui::Text("ed knopfW        %.0f",
                        static_cast<double>(g_app->dbgKnopfW));
            ImGui::Text("ed FeldEnde      %.0f",
                        static_cast<double>(g_app->dbgFeldEnde));
            ImGui::Text("ed HelferEnde    %.0f",
                        static_cast<double>(g_app->dbgHelferEnde));
            ImGui::Text("ed Differenz     %.0f",
                        static_cast<double>(g_app->dbgHelferEnde -
                                            g_app->dbgFeldEnde));
            ImGui::Text("combo MinBreite  %.0f",
                        static_cast<double>(g_app->dbgComboMinB));
            ImGui::Text("combo ListenH    %.0f",
                        static_cast<double>(g_app->dbgComboListenH));
            ImGui::Text("splitButtons     %.4f", static_cast<double>(
                                                     g_app->settings.splitButtons));
            ImGui::Separator();
            ImGui::Text("buttonsW         %.0f", static_cast<double>(l.buttonsW));
            ImGui::Text("minButtonsW      %.0f",
                        static_cast<double>(l.minButtonsW));
            ImGui::Text("headerH          %.0f", static_cast<double>(l.headerH));
            ImGui::Text("fussH            %.0f", static_cast<double>(l.fussH));
            ImGui::Text("frameH           %.0f", static_cast<double>(l.frameH));
            ImGui::Text("statusH          %.0f", static_cast<double>(l.statusH));
            ImGui::Separator();
            ImGui::Text("minEvents        %.0f",
                        static_cast<double>(
                            ImGui::GetFontSize() *
                            ((g_app->leftMode == 0) ? 14.0F : 46.0F)));
            ImGui::Text("FramePadding.x   %.0f",
                        static_cast<double>(st.FramePadding.x));
            ImGui::Text("CellPadding.x    %.0f",
                        static_cast<double>(st.CellPadding.x));
            ImGui::Text("Zeichenhoehe     %.0f",
                        static_cast<double>(ImGui::GetFontSize()));

        }
        ImGui::End();
    }

    drawStatus(l);
    drawUndoListe();
    updater::zeichneFenster();
    drawChangelog();
    drawMessages();
    drawGamePaths();
    drawInterplay();
    drawKeyBindings();

    // Die Speichernfrage hat VORRANG - und zwar allein.
    //
    // Jedes dieser Fenster ruft `ImGui::OpenPopup` in JEDEM Bild. ImGui
    // haelt nur EIN Klappfenster je Ebene: wer dort etwas anderes
    // aufmacht, schliesst das vorherige. Standen zwei davon gleichzeitig
    // offen, machte jedes Bild abwechselnd das eine auf und das andere zu.
    // Keines lebte lang genug, um einen Klick anzunehmen.
    //
    // shank zu rc544: "wenn ich ein kleines Fenster offen habe, also zum
    // beispiel event und druecke rechts oben auf das programm x um es zu
    // schliessen, schliesst sich der event editor aber das grosse fenster
    // bleibt offen und ich kann nichts mehr anklicken und auch das
    // hauptfenster nicht mehr schliessen. ist nur noch mit taskmanager
    // schliessbar."
    //
    // Genau so: `confirmQuit()` gibt false zurueck, solange `askSaveOpen`
    // steht ("Ist gerade eine Frage offen, nicht noch eine anstossen"), und
    // die Frage laesst sich nicht beantworten, weil der Ereigniseditor sie
    // im naechsten Bild wieder zudrueckt. WM_CLOSE wird also fuer immer
    // abgelehnt.
    //
    // Deshalb: liegt die Speichernfrage an, wird sonst nichts gezeichnet.
    // Sie ist die einzige Frage, deren Antwort das Programm braucht.
    if (g_app->askSaveOpen) {
        drawAskSave();
    } else if (g_app->frageOffen) {
        // Dieselbe Regel: eine Frage zur Zeit, sonst schliessen sich die
        // Klappfenster gegenseitig.
        drawFrage();
    } else {
        drawEditor();
        drawFind();
        drawPrefs();
        drawPk3Browser();
        drawMissionPicker();
        drawNeueMission();
        drawMissionSpeichern();
        drawKartenStatistik();
        drawWegFenster();
        drawNpcBrowser();
        drawAnimFenster();
        drawEinblendungen();
        drawAbout();
    }
    // rc601 tour: Tutorial-Fenster, Hervorhebung und Schrittpruefung - VOR
    // selbsttest(), damit der Test die Knoepfe des Fensters im selben Bild
    // findet (das Register wird erst danach geleert).
    tourBild();
    selbsttest();   // nur mit BHED_EDITORTEST, siehe selbsttest.cpp
    // rc601 tour: das Element-Register leeren und die Stufe fuer das naechste
    // Bild setzen (Voll im Selbsttest, Gefiltert waehrend einer Tour, sonst
    // Aus). Bis rc600 leerte selbsttest() es selbst.
    elemente::bildEnde();
    handleShortcuts();
    handleTreeKeys();
    karteneditor::autosaveJedeSekunde();   // Karten-Editor: EverySecondTimer (QE_CheckAutoSave)
    // rc601 ton (Review): als LETZTES im Bild - nach allem, was das Abspielen
    // anhalten kann. Schleifen, die tonBild in diesem Bild nicht gemeldet
    // hat, enden (vorher summte ein Schwert nach dem Ende der Zeitleiste
    // endlos weiter).
    tonWache();
    ImGui::End();
}

// Startgroesse des Fensters in Pixeln, aus der Ressource.
void defaultWindowSize(int* w, int* h) {
    *w = static_cast<int>(kDialogW * kDluX);
    *h = static_cast<int>(kDialogH * kDluY);
}

FeldKnopf feldKnopf(const Command& c, std::size_t feld) {
    const bool fuehrend =
        !c.params.empty() && c.params[0].kind == Param::Kind::TypeSet;
    if (fuehrend) {
        return feld == 0 ? FeldKnopf::Keiner : FeldKnopf::Helfer;
    }
    return FeldKnopf::Expr;
}

bool auswahlMitFeldern(const CommandDb& db, const std::string& typeset) {
    const TypeSet* ts = db.typeset(typeset);
    if (ts == nullptr) { return false; }
    for (const TypeEntry& e : ts->entries) {
        if (!e.params.empty()) { return true; }
    }
    return false;
}

namespace {
bool sucheKennung(const std::vector<Node>& ns, Kennung k, Path& weg) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        weg.push_back(i);
        if (ns[i].kennung == k || sucheKennung(ns[i].children, k, weg)) {
            return true;
        }
        weg.pop_back();
    }
    return false;
}
}  // namespace

std::vector<Kennung> kennungenVon(const std::vector<Path>& wege) {
    g_app->doc.vergibKennungen();
    std::vector<Kennung> ks;
    for (const Path& p : wege) {
        const Node* n = nodeAt(g_app->doc.script(), p);
        if (n != nullptr && n->kennung != 0) { ks.push_back(n->kennung); }
    }
    return ks;
}

void markiereKennungen(const std::vector<Kennung>& ks) {
    g_app->doc.vergibKennungen();
    std::vector<Path> neu;
    for (const Kennung k : ks) {
        Path weg;
        if (sucheKennung(g_app->doc.script().nodes, k, weg)) {
            neu.push_back(weg);
            // Der Block, in dem es jetzt liegt, muss offen sein - sonst ist
            // die Markierung unsichtbar.
            Path eltern = weg;
            while (eltern.size() > 1) {
                eltern.pop_back();
                const Node* e = nodeAt(g_app->doc.script(), eltern);
                if (e != nullptr && e->kennung != 0) {
                    g_app->expanded.setOpen(e->kennung, true);
                }
            }
        }
    }
    g_app->selection = neu;
    g_app->selectedPath = neu.empty() ? Path{} : neu.front();
    // Bewusst KEIN scrollToSelected: abgelegt wird, wo man hinschaut, und
    // die Ansicht soll stehen bleiben (shank: "The view should not scroll").
}

void reiterSchliessen(int index) { closeTab(index); }
bool einzelnenSchrittZuruecknehmen(std::size_t i, std::string* bericht) { return schrittEinzelnZurueck(i, bericht); }

// --- Lesezeichen und Aenderungsrand --------------------------------------
//
// shank, 27.09.: "be able to mark a command/line in the script ... Like in
// notepad++" und "Having the highlight on the side could also be nice to
// have to show what was changed between saves and the current session."
//
// Vorbild ist Notepad++ (Handbuch, "Searching > Bookmarks" und "Editing >
// Change History"):
//   Lesezeichen   Klick in den Rand oder Strg+F2; F2 / Umschalt+F2 springen
//                 zum naechsten / vorigen, am Ende wieder von vorn.
//   Rand          orange = seit dem letzten Speichern geaendert,
//                 gruen  = in dieser Sitzung geaendert und gespeichert,
//                 blassblau = nach dem Speichern per Rueckgaengig wieder wie
//                 beim Oeffnen. Nach dem Laden ist nichts markiert.
namespace {

// Der eigene Text eines Knotens - ohne Kinder, mit allem, was man sieht.
std::string eigenerText(const Node& n) {
    std::string t = std::to_string(static_cast<int>(n.kind)) + "|" + n.name + "|" + n.raw + "|";
    for (const Arg& x : n.args) {
        t += std::to_string(static_cast<int>(x.kind)) + x.typeset + ":" + x.text +
             (x.locked ? "!" : "") + "\x1f";
    }
    if (n.hasBlock) { t += "{"; }
    return t;
}

void knotenListe(const std::vector<Node>& ns, std::vector<std::pair<Kennung, std::string>>& out) {
    for (const Node& n : ns) {
        if (n.kind != Node::Kind::Blank) {
            out.emplace_back(n.kennung, eigenerText(n));
        }
        knotenListe(n.children, out);
    }
}

std::vector<std::pair<Kennung, std::string>> knotenListe(const Script& s) {
    std::vector<std::pair<Kennung, std::string>> v;
    knotenListe(s.nodes, v);
    return v;
}

// Welche Knoten sind gegenueber `stand` anders? Neu, im Text geaendert oder
// VERSCHOBEN. Verschoben heisst: nicht in der laengsten Folge, die in beiden
// Staenden in derselben Reihenfolge steht - wie ein Zeilenvergleich, der
// eine verschobene Zeile als geaendert zeigt und die anderen nicht. Die
// Kennungen sind eindeutig, also genuegt die laengste steigende Teilfolge
// der alten Positionen (O(n log n)).
std::set<Kennung> geaendertGegen(const std::vector<std::pair<Kennung, std::string>>& stand,
                                 const std::vector<std::pair<Kennung, std::string>>& jetzt) {
    std::map<Kennung, std::size_t> wo;
    for (std::size_t i = 0; i < stand.size(); ++i) { wo[stand[i].first] = i; }
    std::set<Kennung> anders;
    std::vector<std::size_t> idx;    // Index in `jetzt`
    std::vector<std::size_t> pos;    // Position in `stand`
    for (std::size_t j = 0; j < jetzt.size(); ++j) {
        const Kennung k = jetzt[j].first;
        const auto it = wo.find(k);
        if (k == 0 || it == wo.end() || stand[it->second].second != jetzt[j].second) {
            anders.insert(k);
            continue;
        }
        idx.push_back(j);
        pos.push_back(it->second);
    }
    // Laengste steigende Teilfolge von `pos`, mit Rueckverfolgung.
    std::vector<std::size_t> ende;          // Index in pos des Endes einer Folge der Laenge l+1
    std::vector<std::ptrdiff_t> vor(pos.size(), -1);
    for (std::size_t i = 0; i < pos.size(); ++i) {
        std::size_t lo = 0;
        std::size_t hi = ende.size();
        while (lo < hi) {
            const std::size_t mid = (lo + hi) / 2;
            if (pos[ende[mid]] < pos[i]) { lo = mid + 1; } else { hi = mid; }
        }
        if (lo > 0) { vor[i] = static_cast<std::ptrdiff_t>(ende[lo - 1]); }
        if (lo == ende.size()) { ende.push_back(i); } else { ende[lo] = i; }
    }
    std::vector<bool> drin(pos.size(), false);
    if (!ende.empty()) {
        for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(ende.back()); i >= 0; i = vor[static_cast<std::size_t>(i)]) {
            drin[static_cast<std::size_t>(i)] = true;
        }
    }
    for (std::size_t i = 0; i < pos.size(); ++i) {
        if (!drin[i]) { anders.insert(jetzt[idx[i]].first); }
    }
    return anders;
}

}  // namespace

// Die Befehle beim Oeffnen, je Kennung, ohne Blockinhalt.
void originaleMerken(const std::vector<Node>& ns, std::map<Kennung, Node>& out) {
    for (const Node& n : ns) {
        if (n.kind != Node::Kind::Blank && n.kennung != 0) {
            Node k = n;
            k.children.clear();
            out[n.kennung] = std::move(k);
        }
        originaleMerken(n.children, out);
    }
}

void aenderungsStandNeu() {
    const int uid = tabKennung(g_app->activeTab);
    if (uid < 0) { return; }
    g_app->doc.vergibKennungen();
    auto& st = g_app->aenderungen[uid];
    st.geladen = knotenListe(g_app->doc.script());
    st.gespeichert = st.geladen;
    st.original.clear();
    originaleMerken(g_app->doc.script().nodes, st.original);
}

// "Revert to original": welche der gewaehlten Befehle sind INHALTLICH anders
// als beim Oeffnen, und womit werden sie ersetzt? Nicht dabei: neue Befehle
// (es gab kein Original), nur verschobene (ihr Inhalt ist gleich),
// Makrozeilen (ihre Zahl haengt an den Befehlen dahinter) und Zeilen, deren
// Art sich geaendert hat (REM - dafuer gibt es den REM-Knopf).
std::vector<std::pair<Path, Node>> zuruecksetzbar(const std::vector<Path>& wege) {
    std::vector<std::pair<Path, Node>> out;
    const int uid = tabKennung(g_app->activeTab);
    const auto it = g_app->aenderungen.find(uid);
    if (uid < 0 || it == g_app->aenderungen.end()) {
        return out;
    }
    g_app->doc.vergibKennungen();
    for (const Path& p : wege) {
        const Node* n = nodeAt(g_app->doc.script(), p);
        if (n == nullptr || n->kennung == 0 || n->kind == Node::Kind::Macro || n->kind == Node::Kind::Blank) {
            continue;
        }
        const auto o = it->second.original.find(n->kennung);
        if (o == it->second.original.end() || o->second.kind != n->kind || o->second.hasBlock != n->hasBlock) {
            continue;
        }
        if (eigenerText(o->second) != eigenerText(*n)) {
            out.emplace_back(p, o->second);
        }
    }
    return out;
}

std::string originalZeile(const Path& weg, bool* neu) {
    if (neu != nullptr) { *neu = false; }
    const int uid = tabKennung(g_app->activeTab);
    const auto it = g_app->aenderungen.find(uid);
    if (uid < 0 || it == g_app->aenderungen.end()) {
        return {};
    }
    g_app->doc.vergibKennungen();
    const Node* n = nodeAt(g_app->doc.script(), weg);
    if (n == nullptr || n->kennung == 0 || n->kind == Node::Kind::Macro || n->kind == Node::Kind::Blank) {
        return {};
    }
    const auto o = it->second.original.find(n->kennung);
    if (o == it->second.original.end()) {
        if (neu != nullptr) { *neu = true; }
        return {};
    }
    const std::vector<std::pair<Path, Node>> z = zuruecksetzbar({weg});
    return z.empty() ? std::string() : rowText(z.front().second, g_app->treeOpt);
}

std::string originalKurz(const std::vector<std::pair<Path, Node>>& zurueck) {
    if (zurueck.empty()) {
        return {};
    }
    std::string zeile = rowText(zurueck.front().second, g_app->treeOpt);
    // Sehr lange Zeilen (ein langer print-Text) kuerzen - das Menue soll
    // nicht ueber den Bildschirm wachsen.
    constexpr std::size_t kMax = 70;
    if (zeile.size() > kMax) {
        zeile = zeile.substr(0, kMax) + "...";
    }
    if (zurueck.size() > 1) {
        zeile += "  (+" + std::to_string(zurueck.size() - 1) + ")";
    }
    return zeile;
}

// shank: "I think it would look better if it were on one line" - grau
// "Original:", gleich daneben der Befehl.
void originalVorschau(const std::vector<std::pair<Path, Node>>& zurueck) {
    ++g_app->vorschauGezeichnet;
    ImGui::TextDisabled("%s", tr(Str::RevertPreviewLabel));
    ImGui::SameLine();
    ImGui::TextUnformatted(originalKurz(zurueck).c_str());
}

void aenderungenGespeichert() {
    const int uid = tabKennung(g_app->activeTab);
    if (uid < 0) { return; }
    g_app->doc.vergibKennungen();
    auto it = g_app->aenderungen.find(uid);
    if (it == g_app->aenderungen.end()) {
        aenderungsStandNeu();
        return;
    }
    it->second.gespeichert = knotenListe(g_app->doc.script());
    // Gleich neu rechnen - sonst bleibt orange stehen, bis zur naechsten
    // Aenderung (der Selbsttest hat es gefunden).
    g_app->zeilenMarke = zeilenMarken(g_app->activeTab, g_app->doc.script(), g_app->rows);
    for (auto& pn : g_app->splitPanes) { pn.dirty = true; }
}

std::vector<std::uint8_t> zeilenMarken(int tab, const Script& s, const std::vector<Row>& rows) {
    std::vector<std::uint8_t> m(rows.size(), kMarkeKeine);
    const int uid = tabKennung(tab);
    if (uid < 0 || !g_app->settings.changeHistory) { return m; }
    const std::vector<std::pair<Kennung, std::string>> jetzt = knotenListe(s);
    auto it = g_app->aenderungen.find(uid);
    if (it == g_app->aenderungen.end()) {
        // Zum ersten Mal gesehen: das ist der Stand beim Oeffnen.
        auto& st = g_app->aenderungen[uid];
        st.geladen = jetzt;
        st.gespeichert = jetzt;
        originaleMerken(s.nodes, st.original);
        return m;
    }
    const std::set<Kennung> gegenGeladen = geaendertGegen(it->second.geladen, jetzt);
    const std::set<Kennung> gegenGespeichert = geaendertGegen(it->second.gespeichert, jetzt);
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const Kennung k = rows[i].kennung;
        if (k == 0) { continue; }
        const bool l = gegenGeladen.count(k) != 0;
        const bool sp = gegenGespeichert.count(k) != 0;
        m[i] = (l && sp) ? kMarkeGeaendert
             : l         ? kMarkeGespeichert
             : sp        ? kMarkeZurueck
                         : kMarkeKeine;
    }
    return m;
}

bool hatLesezeichen(int tab, Kennung k) {
    const auto it = g_app->lesezeichen.find(tabKennung(tab));
    return it != g_app->lesezeichen.end() && it->second.count(k) != 0;
}

void lesezeichenUmschalten(const std::vector<Path>& wege) {
    const int uid = tabKennung(g_app->activeTab);
    if (uid < 0) { return; }
    g_app->doc.vergibKennungen();
    auto& lz = g_app->lesezeichen[uid];
    for (const Path& p : wege) {
        const Node* n = nodeAt(g_app->doc.script(), p);
        if (n == nullptr || n->kennung == 0) { continue; }
        if (lz.erase(n->kennung) == 0) { lz.insert(n->kennung); }
    }
    // Ein unveraendertes Dokument IST die Datei - dann gleich merken, sonst
    // beim naechsten Speichern (die Zeilennummern gelten fuer die Datei).
    if (!g_app->path.empty() && !g_app->doc.dirty()) {
        lesezeichenMerken();
    }
}

void lesezeichenSpringen(bool vorwaerts) {
    const int uid = tabKennung(g_app->activeTab);
    const auto it = g_app->lesezeichen.find(uid);
    g_app->doc.vergibKennungen();
    // Die gesetzten Lesezeichen in Dateireihenfolge, mit ihren Wegen.
    std::vector<Path> ziele;
    std::function<void(const std::vector<Node>&, Path&)> lauf =
        [&](const std::vector<Node>& ns, Path& weg) {
            for (std::size_t i = 0; i < ns.size(); ++i) {
                weg.push_back(i);
                if (it != g_app->lesezeichen.end() && ns[i].kennung != 0 &&
                    it->second.count(ns[i].kennung) != 0) {
                    ziele.push_back(weg);
                }
                lauf(ns[i].children, weg);
                weg.pop_back();
            }
        };
    Path w;
    lauf(g_app->doc.script().nodes, w);
    if (ziele.empty()) {
        addStatus(tr(Str::MsgNoBookmarks));
        return;
    }
    // Vom aktuellen Knoten aus; ohne Auswahl von vorn bzw. von hinten.
    const Path& hier = g_app->selectedPath;
    const Path* treffer = nullptr;
    if (vorwaerts) {
        for (const Path& z : ziele) { if (hier.empty() || hier < z) { treffer = &z; break; } }
        if (treffer == nullptr) { treffer = &ziele.front(); }   // Umlauf
    } else {
        for (auto r = ziele.rbegin(); r != ziele.rend(); ++r) {
            if (!hier.empty() && *r < hier) { treffer = &*r; break; }
        }
        if (treffer == nullptr) { treffer = &ziele.back(); }    // Umlauf
    }
    selectByPath(*treffer);
}

void lesezeichenAlleWeg() {
    g_app->lesezeichen.erase(tabKennung(g_app->activeTab));
    if (!g_app->path.empty()) {
        g_app->settings.lesezeichen.erase(g_app->path);
    }
}

void lesezeichenMerken() {
    if (g_app->path.empty()) { return; }
    const auto it = g_app->lesezeichen.find(tabKennung(g_app->activeTab));
    g_app->doc.vergibKennungen();
    const std::vector<std::pair<Kennung, std::string>> liste = knotenListe(g_app->doc.script());
    std::string nummern;
    if (it != g_app->lesezeichen.end()) {
        for (std::size_t i = 0; i < liste.size(); ++i) {
            if (it->second.count(liste[i].first) != 0) {
                if (!nummern.empty()) { nummern += ","; }
                nummern += std::to_string(i);
            }
        }
    }
    if (nummern.empty()) {
        g_app->settings.lesezeichen.erase(g_app->path);
    } else {
        g_app->settings.lesezeichen[g_app->path] = nummern;
    }
}

void lesezeichenLaden() {
    const int uid = tabKennung(g_app->activeTab);
    if (uid < 0) { return; }
    g_app->lesezeichen.erase(uid);
    const auto it = g_app->settings.lesezeichen.find(g_app->path);
    if (g_app->path.empty() || it == g_app->settings.lesezeichen.end()) { return; }
    g_app->doc.vergibKennungen();
    const std::vector<std::pair<Kennung, std::string>> liste = knotenListe(g_app->doc.script());
    auto& lz = g_app->lesezeichen[uid];
    std::size_t a = 0;
    const std::string& t = it->second;
    while (a < t.size()) {
        std::size_t b = t.find(',', a);
        if (b == std::string::npos) { b = t.size(); }
        const std::size_t n = static_cast<std::size_t>(std::atoi(t.substr(a, b - a).c_str()));
        if (n < liste.size() && liste[n].first != 0) { lz.insert(liste[n].first); }
        a = b + 1;
    }
}

int tabKennung(int tab) {
    if (tab < 0 || tab >= static_cast<int>(g_app->tabs.size())) { return -1; }
    int& u = g_app->tabs[static_cast<std::size_t>(tab)].uid;
    if (u == 0) { u = g_app->naechsteTabUid++; }
    return u;
}

void fensterZeigen(bool& offen, const char* id) {
    offen = true;
    ImGui::SetWindowFocus(id);
}

int ersetzeEinen() {
    const FindOptions& o = g_app->findLetzte;
    if (o.contains.empty()) {
        return 0;
    }
    // Steht die Auswahl auf einem Treffer? Dann ihn ersetzen - sonst erst
    // hinspringen (Notepad++: der erste Druck findet, der zweite ersetzt).
    const std::vector<Path> treffer = findAll(g_app->doc.script(), o);
    const Path sel = g_app->selectedPath;
    int zahl = 0;
    if (std::find(treffer.begin(), treffer.end(), sel) != treffer.end()) {
        if (const Node* alt = nodeAt(g_app->doc.script(), sel)) {
            Node neu = *alt;
            zahl = ersetzeInKnoten(neu, o, g_app->findErsatz);
            if (zahl > 0 && g_app->doc.replaceAt(sel, std::move(neu))) {
                rebuildTree();
            }
        }
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), tr(Str::FindReplacedN), zahl);
    addStatus(buf);
    findNext(1);
    return zahl;
}

int ersetzeAlle(bool alleDokumente) {
    const FindOptions& o = g_app->findLetzte;
    if (o.contains.empty()) {
        return 0;
    }
    int zahl = 0;
    int dokumente = 0;
    // Der aktive Reiter - ein Rueckgaengig-Schritt fuer alles.
    {
        Script kopie = g_app->doc.script();
        const std::vector<Path> nurIn =
            g_app->findInSelection ? selectionOrCurrent() : std::vector<Path>{};
        const int n = ersetzeImSkript(kopie, o, g_app->findErsatz, nurIn);
        if (n > 0) {
            g_app->doc.replaceAll(std::move(kopie), "replace");
            rebuildTree();
            zahl += n;
            ++dokumente;
        }
    }
    // Die anderen Reiter: ihr geparktes Dokument, ebenfalls je ein Schritt.
    // "In selection" gilt dort nicht - die Auswahl gehoert dem aktiven.
    if (alleDokumente) {
        for (int i = 0; i < static_cast<int>(g_app->tabs.size()); ++i) {
            if (i == g_app->activeTab) {
                continue;
            }
            Document& d = g_app->tabs[static_cast<std::size_t>(i)].doc;
            Script kopie = d.script();
            const int n = ersetzeImSkript(kopie, o, g_app->findErsatz);
            if (n > 0) {
                d.replaceAll(std::move(kopie), "replace");
                zahl += n;
                ++dokumente;
            }
        }
    }
    char buf[96];
    if (alleDokumente) {
        std::snprintf(buf, sizeof(buf), tr(Str::FindReplacedDocs), zahl, dokumente);
    } else {
        std::snprintf(buf, sizeof(buf), tr(Str::FindReplacedN), zahl);
    }
    addStatus(buf);
    g_app->findHits = findAll(g_app->doc.script(), o);
    return zahl;
}

ImGuiKeyChord kuerzelVon(keys::Action a) { return chordFor(a); }
const char* kuerzelName(keys::Action a) { return chordName(a); }

// --- Rechtsklick und Ablegen in Graph und Spuren -------------------------------
//
// shank (14.10.): "das jedes Tutorial in jedem Modus erstellt werden kann".
// Bis hier liess sich in Graph und Spuren nur waehlen und ziehen - Werte,
// Loeschen, Verschieben gingen nur in Classic. Dieses Menue ist das der
// Classic-Zeile ohne die Mehrfachauswahl-Feinheiten; es arbeitet auf der
// Zeile `p` des offenen Skripts (die Auswahl wird darauf gesetzt).
// `editor` wird gesetzt statt den Editor sofort zu oeffnen: er ist ein
// Modal und darf nicht im Kontextmenue aufgehen.
bool befehlsMenue(const Path& p, bool& editor) {
    if (p.empty() || nodeAt(g_app->doc.script(), p) == nullptr) {
        return false;
    }
    if (g_app->selectedPath != p) {
        g_app->selectedPath = p;
        g_app->selection.assign(1, p);
    }
    bool geaendert = false;
    if (ImGui::MenuItem(tr(Str::ActEdit), chordName(keys::Action::EditItem))) { editor = true; }
    if (ImGui::MenuItem(tr(Str::ActBookmarkToggle), chordName(keys::Action::BookmarkToggle))) {
        lesezeichenUmschalten(std::vector<Path>{p});
    }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::ActClone), chordName(keys::Action::Clone))) { geaendert = g_app->doc.cloneAt(p); }
    if (ImGui::MenuItem(tr(Str::ActCopy), chordName(keys::Action::Copy))) { kopiereWeg(p, false); }
    if (ImGui::MenuItem(tr(Str::ActCut), chordName(keys::Action::Cut))) {
        kopiereWeg(p, true);
        geaendert = true;
    }
    if (ImGui::MenuItem(tr(Str::ActPaste), chordName(keys::Action::Paste))) {
        einfuegen(p);
        geaendert = true;
    }
    ImGui::Separator();
    for (const bool hoch : {true, false}) {
        if (ImGui::MenuItem(tr(hoch ? Str::MoveUp : Str::MoveDown), chordName(hoch ? keys::Action::MoveUp : keys::Action::MoveDown))) {
            Path nachher;
            if (hoch ? g_app->doc.moveUp(p, &nachher) : g_app->doc.moveDown(p, &nachher)) {
                geaendert = true;
                if (!nachher.empty()) {
                    g_app->selectedPath = nachher;
                    g_app->selection.assign(1, nachher);
                }
            } else {
                zeigeAbweisung();
            }
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::ActRem), chordName(keys::Action::CommentOut))) { geaendert = remUmschalten(p); }
    if (ImGui::MenuItem(tr(Str::ActDelete), chordName(keys::Action::Delete))) { geaendert = g_app->doc.removeAt(p); }
    ImGui::Separator();
    // Neue Befehle kommen hinter diese Zeile (bei einem offenen Block
    // hinein) - wie der Doppelklick in der Ereignisliste.
    if (ImGui::BeginMenu(tr(Str::GroupEvents))) {
        for (const Command& c : g_app->db.commands) {
            if (ImGui::MenuItem((c.name + "  " + signatureText(c)).c_str())) {
                insertCommand(c);
                geaendert = true;
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::MsgMacros))) {
        for (const Macro& mm : g_app->db.macroBodies) {
            if (ImGui::MenuItem(mm.name.c_str())) {
                insertMacro(mm);
                geaendert = true;
            }
        }
        ImGui::EndMenu();
    }
    if (geaendert) {
        rebuildTree();
        g_app->camTrackValid = false;
    }
    return geaendert;
}

// Ein Befehl oder Makro aus der Ereignisliste, in Graph oder Spuren
// abgelegt: vor oder hinter die Zeile `p` des offenen Skripts, bei leerem
// Weg ans Ende.
bool neuAblegenAn(const ImGuiPayload* pl, const Path& p, bool davor) {
    if (pl == nullptr || pl->DataSize != static_cast<int>(sizeof(NewDrag))) {
        return false;
    }
    NewDrag neu{};
    std::memcpy(&neu, pl->Data, sizeof(neu));
    const auto vorher = g_app->doc.stand();
    dropNewAt(neu, p, p.empty() ? DropWhere::After : (davor ? DropWhere::Before : DropWhere::After));
    if (g_app->doc.stand() == vorher) {
        return false;
    }
    rebuildTree();
    g_app->camTrackValid = false;
    return true;
}

// Stufe 8c: das Logo von JK Studio fuer den Selbsttest (app_internal.h)
void* jkLogoTextur(int& b, int& h) { return jkLogoTextureId(b, h); }

}  // namespace bhed::gui
