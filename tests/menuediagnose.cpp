// menuediagnose.cpp - Menue-Editor M2: Live-Diagnose mit absichtlich kaputten,
// selbst gebauten Menues. Je Pruefung: das kaputte Menue meldet genau das an
// der richtigen Stelle (rot vor der Pruefung), das heile meldet nichts
// (gruen), und die Schnellkorrektur macht es heil.
#include "bhed/menue/diagnose.h"
#include "bhed/menue/menue.h"

#include <cstddef>
#include <cstdio>
#include <set>
#include <string>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
Datei lese(const std::string& text, Dialekt d = Dialekt::SP, Spiel s = Spiel::JA) {
    LeseOptionen o;
    o.dialekt = d;
    o.spiel = s;
    return lies(d == Dialekt::MP ? "ui/jamp/probe.menu" : "ui/probe.menu", text, o);
}
std::vector<Diagnose> diag(const std::string& text, const DiagKontext& k = DiagKontext{}, Dialekt d = Dialekt::SP) {
    return diagnose(lese(text, d), k);
}
const Diagnose* finde(const std::vector<Diagnose>& v, DiagArt art) {
    for (const Diagnose& x : v) {
        if (x.art == art) { return &x; }
    }
    return nullptr;
}
int zaehle(const std::vector<Diagnose>& v, DiagArt art) {
    int n = 0;
    for (const Diagnose& x : v) { n += x.art == art ? 1 : 0; }
    return n;
}
std::string stelle(const std::string& t, const Diagnose& x) { return t.substr(x.anfang, x.ende - x.anfang); }
std::string korrigiert(const std::string& t, const Korrektur& k) { return t.substr(0, k.anfang) + k.text + t.substr(k.ende); }
std::string zeigen(const std::vector<Diagnose>& v) {
    std::string s;
    for (const Diagnose& x : v) { s += "[" + std::to_string(static_cast<int>(x.art)) + " z" + std::to_string(x.zeile) + " " + x.text + "] "; }
    return s;
}

const char* const kHeil =
    "{\n"
    "\tmenuDef\n"
    "\t{\n"
    "\t\tname heil\n"
    "\t\trect 0 0 640 480\n"
    "\t\tonOpen { show knopf ; setfocus knopf }\n"
    "\t\titemDef\n"
    "\t\t{\n"
    "\t\t\tname knopf\n"
    "\t\t\tgroup gruppe\n"
    "\t\t\trect 10 10 100 20\n"
    "\t\t\tstyle WINDOW_STYLE_FILLED\n"
    "\t\t\tbackcolor 1 0 0 1\n"
    "\t\t\tvisible 1\n"
    "\t\t\taction { hide grup* ; open heil }\n"
    "\t\t}\n"
    "\t}\n"
    "}\n";
}  // namespace

int main() {
    // gruen: das heile Menue meldet nichts
    {
        const std::vector<Diagnose> v = diag(kHeil);
        pruefe(v.empty(), "Heil: keine Meldung " + zeigen(v));
    }
    // Klammern: eine } fehlt (itemDef) - die Einrueckung zeigt, wo
    {
        std::string t = kHeil;
        const std::size_t p = t.find("\t\t}\n\t}\n}");
        t.erase(p, 4);   // die } des itemDef weg
        const std::vector<Diagnose> v = diag(t);
        const Diagnose* x = finde(v, DiagArt::KlammerOffen);
        pruefe(x != nullptr, "Klammer offen: gemeldet " + zeigen(v));
        if (x != nullptr) {
            pruefe(x->zeile == 8, "Klammer offen: an der { des itemDef (Zeile 8), nicht am Dateiende: " + std::to_string(x->zeile));
            pruefe(x->partner, "Klammer offen: dazu die Stelle, an der sie fehlt");
            pruefe(x->korrekturen.size() == 1 && korrigiert(t, x->korrekturen[0]) == kHeil, "Klammer offen: Korrektur stellt das heile Menue her");
        }
    }
    // Klammern: eine } zu viel
    {
        std::string t = kHeil;
        const std::size_t p = t.find("\t\t\tvisible 1\n");
        t.insert(p, "\t\t\t}\n");
        const std::vector<Diagnose> v = diag(t);
        const Diagnose* x = finde(v, DiagArt::KlammerZuViel);
        pruefe(x != nullptr, "Klammer zu viel: gemeldet " + zeigen(v));
        if (x != nullptr) {
            pruefe(x->zeile == 14, "Klammer zu viel: an der eingeschobenen } (Zeile 14): " + std::to_string(x->zeile));
            pruefe(x->korrekturen.size() == 1 && korrigiert(t, x->korrekturen[0]).find("\t\t\t\n") != std::string::npos,
                   "Klammer zu viel: Korrektur entfernt sie");
        }
    }
    // Klammern: ( ... } falsch gepaart - beide Stellen
    {
        const std::string t = "{\nmenuDef\n{\nname a\nrect 0 0 640 480\nonOpen { show ( x }\n}\n}\n";
        const std::vector<Diagnose> v = diag(t);
        const Diagnose* x = finde(v, DiagArt::KlammerFalsch);
        pruefe(x != nullptr && x->partner && t[x->anfang] == '}' && t[x->partnerAnfang] == '(', "Klammer falsch gepaart: } und ( markiert " + zeigen(v));
    }
    // Anfuehrungszeichen bis ins naechste Zeile
    {
        std::string t = kHeil;
        const std::size_t p = t.find("name knopf");
        t.replace(p, 10, "name \"knopf");
        const std::vector<Diagnose> v = diag(t);
        const Diagnose* x = finde(v, DiagArt::AnfuehrungOffen);
        pruefe(x != nullptr && x->zeile == 9, "Anfuehrung offen: Zeile 9 " + zeigen(v));
        if (x != nullptr) {
            pruefe(x->korrekturen.size() == 1 && korrigiert(t, x->korrekturen[0]).find("name \"knopf\"\n") != std::string::npos,
                   "Anfuehrung offen: Korrektur schliesst am Zeilenende");
        }
    }
    // unbekanntes Schluesselwort: Wortlaut der Engine, genaue Stelle, Korrektur
    {
        std::string t = kHeil;
        const std::size_t p = t.find("backcolor");
        t.replace(p, 9, "backcolr");
        const std::vector<Diagnose> v = diag(t);
        const Diagnose* x = finde(v, DiagArt::Engine);
        pruefe(x != nullptr && x->text == "Unknown item keyword 'backcolr'", "Schluessel: Wortlaut der Engine " + zeigen(v));
        if (x != nullptr) {
            pruefe(stelle(t, *x) == "backcolr", "Schluessel: Stelle ist das Wort");
            pruefe(x->korrekturen.size() == 1 && x->korrekturen[0].text == "backcolor" && korrigiert(t, x->korrekturen[0]) == kHeil,
                   "Schluessel: Korrektur backcolor");
        }
        // je Dialekt: ein MP-Wort ist in SP unbekannt
        const std::vector<Diagnose> v2 = diag("{\nmenuDef\n{\nname a\nrect 0 0 640 480\nitemDef { name b rect 0 0 1 1 visible 1 \"mouseEnterText\" { } }\n}\n}\n");
        pruefe(finde(v2, DiagArt::Engine) == nullptr, "Schluessel: mouseEnterText kennt SP " + zeigen(v2));
    }
    // Zahl erwartet
    {
        std::string t = kHeil;
        const std::size_t p = t.find("rect 10 10 100 20");
        t.replace(p, 17, "rect 10 10 hundert 20");
        const std::vector<Diagnose> v = diag(t);
        const Diagnose* x = finde(v, DiagArt::KeineZahl);
        pruefe(x != nullptr && stelle(t, *x) == "hundert" && x->text == "rect", "Keine Zahl: rect 10 10 hundert 20 " + zeigen(v));
        // Namen aus menudef.h: Wert 0 ergibt dasselbe, sonst Warnung mit Korrektur auf die Zahl
        std::string f = kHeil;
        f.replace(f.find("\t\t\tvisible 1\n"), 0, "\t\t\tfeeder FEEDER_SAVEGAMES\n");
        pruefe(finde(diag(f), DiagArt::KeineZahl) == nullptr, "Keine Zahl: FEEDER_SAVEGAMES ist 0 - wie gemeint");
        f.replace(f.find("FEEDER_SAVEGAMES"), 16, "FEEDER_PLAYER_SPECIES");
        const std::vector<Diagnose> vf = diag(f);
        const Diagnose* y = finde(vf, DiagArt::KeineZahl);
        pruefe(y != nullptr && y->korrekturen.size() == 1 && y->korrekturen[0].text == "16", "Keine Zahl: FEEDER_PLAYER_SPECIES = 16, SP liest 0 " + zeigen(vf));
    }
    // doppelte Namen
    {
        std::string t = kHeil;
        const std::size_t p = t.rfind("\t}\n}");
        t.insert(p, "\t\titemDef { name knopf rect 0 0 5 5 visible 1 }\n");
        const std::vector<Diagnose> v = diag(t);
        const Diagnose* x = finde(v, DiagArt::NameDoppelt);
        pruefe(x != nullptr && x->item == 1 && x->schwere == Schwere::Warnung && stelle(t, *x) == "knopf", "Name doppelt: itemDef, den setfocus sucht " + zeigen(v));
        std::string ohneFokus = t;
        ohneFokus.replace(ohneFokus.find(" ; setfocus knopf"), 16, "");
        pruefe(finde(diag(ohneFokus), DiagArt::NameDoppelt) == nullptr, "Name doppelt: ohne setfocus gewollt (show/hide treffen alle)");
        std::string zwei = kHeil;
        zwei.insert(zwei.rfind('}'), "\tmenuDef\n\t{\n\t\tname HEIL\n\t\trect 0 0 640 480\n\t}\n");
        const std::vector<Diagnose> v2 = diag(zwei);
        const Diagnose* y = finde(v2, DiagArt::NameDoppelt);
        pruefe(y != nullptr && y->menu == 1 && y->schwere == Schwere::Warnung, "Name doppelt: menuDef (ohne Gross/Klein) " + zeigen(v2));
    }
    // Ziele, die es nicht gibt
    {
        std::string t = kHeil;
        t.replace(t.find("show knopf"), 10, "show knopp");
        t.replace(t.find("open heil"), 9, "open nirgends");
        DiagKontext k;
        k.menueBekannt = [](const std::string& n) { return n == "ingame"; };
        const std::vector<Diagnose> v = diag(t, k);
        pruefe(zaehle(v, DiagArt::ZielFehlt) == 2, "Ziel fehlt: show knopp und open nirgends " + zeigen(v));
        std::string u = kHeil;
        u.replace(u.find("open heil"), 9, "open ingame");
        pruefe(zaehle(diag(u, k), DiagArt::ZielFehlt) == 0, "Ziel: Menue aus einer anderen Datei");
        u.replace(u.find("hide grup*"), 10, "hide Grup*");
        pruefe(zaehle(diag(u, k), DiagArt::ZielFehlt) == 1, "Ziel: Platzhalter vergleicht mit Gross/Klein (Q_strncmp)");
        u.replace(u.find("open ingame"), 11, "close all");
        pruefe(zaehle(diag(u, k), DiagArt::ZielFehlt) == 1, "Ziel: close all schliesst alle (Script_Close)");
    }
    // Cvar-Tippfehler
    {
        const std::string t = "{\nmenuDef\n{\nname a\nrect 0 0 640 480\nitemDef { name b rect 0 0 9 9 visible 1 type ITEM_TYPE_YESNO cvar \"ui_subtitels\" }\n}\n}\n";
        DiagKontext k;
        k.bekannteCvars = {"ui_subtitles", "g_saber"};
        const std::vector<Diagnose> v = diag(t, k);
        const Diagnose* x = finde(v, DiagArt::CvarAehnlich);
        pruefe(x != nullptr && x->wert == "ui_subtitles" && stelle(t, *x) == "ui_subtitels", "Cvar aehnlich: ui_subtitels -> ui_subtitles " + zeigen(v));
        k.bekannteCvars = {"ui_volume"};
        pruefe(finde(diag(t, k), DiagArt::CvarAehnlich) == nullptr, "Cvar: nichts Aehnliches - keine Meldung");
    }
    // gemeinsame Quelle mit der Exportpruefung: nie sichtbar, fehlende Datei mit Pfad-Vorschlag
    {
        std::string t = kHeil;
        t.replace(t.find("rect 10 10 100 20"), 17, "rect 700 10 100 20");
        t.replace(t.find("\t\t\tvisible 1\n"), 0, "\t\t\tbackground \"gfx/menus/knopf_an\"\n");
        DiagKontext k;
        k.pruefung.gibtBild = [](const std::string& n) { return n == "gfx/menus/knopf_ein"; };
        k.pfadVorschlag = [](const std::string& p) { return p == "gfx/menus/knopf_an" ? std::string("gfx/menus/knopf_ein") : std::string(); };
        const std::vector<Diagnose> v = diag(t, k);
        bool nie = false, datei = false;
        for (const Diagnose& x : v) {
            if (x.art != DiagArt::Pruefung) { continue; }
            if (x.befund.art == BefundArt::NieSichtbar) { nie = stelle(t, x) == "rect"; }
            if (x.befund.art == BefundArt::DateiFehlt) {
                datei = stelle(t, x) == "\"gfx/menus/knopf_an\"" && x.korrekturen.size() == 1 && x.korrekturen[0].text == "\"gfx/menus/knopf_ein\"";
            }
        }
        pruefe(nie, "Pruefung: nie sichtbar an rect " + zeigen(v));
        pruefe(datei, "Pruefung: Datei fehlt mit Pfad-Vorschlag " + zeigen(v));
    }
    // Hilfen
    pruefe(abstand("backcolr", "BACKCOLOR") == 1 && aehnlichstes("visibel", {"visibelx", "xvisibel"}, 2).empty() &&
               aehnlichstes("visibel", {"visible", "border"}, 2) == "visible",
           "Abstand und eindeutiger Vorschlag");
    std::printf("menuediagnose: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
