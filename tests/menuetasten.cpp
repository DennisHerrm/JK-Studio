// menuetasten.cpp - Menue-Editor M7: Tastenbelegung wie im Spiel (cl_keys.cpp, cmd.cpp, ui_shared
// BindingFromName/Controls_GetKeyAssignment) fuer ITEM_TYPE_BIND.
#include "bhed/menue/tasten.h"

#include <cstdio>
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
}  // namespace

int main() {
    pruefe(tastenNummer("w") == 'W' && tastenNummer("W") == 'W' && tastenNummer("UPARROW") == 0xaa && tastenNummer("mouse1") == 0x8d,
           "Key_StringToKeynum: ein Zeichen -> upper, Namen ohne Gross/Klein");
    pruefe(tastenNummer("SHIFT_ENTER") == 0x0a && tastenNummer("0x41") == 0x41 && tastenNummer("gibtsnicht") == -1, "keynum, Hex, unbekannt");
    pruefe(tastenName('W') == "W" && tastenName(0x20) == "SPACE" && tastenName(0xaa) == "UPARROW" && tastenName(-1) == "<KEY NOT FOUND>",
           "Key_KeynumToString");
    Tastenbelegung b;
    configAusfuehren("bind q +altattack\nunbindall\nbind w +forward\nbind UPARROW \"+forward\"; bind MOUSE1 +attack\n"
                     "bind a +moveleft // Kommentar\nbind x \"say hallo\"\nbind TAB +scores ; bind KP_5 +forward\nexec weiter\n",
                     b, [](const std::string& p, std::string& out) {
                         if (p != "weiter.cfg") { return false; }
                         out = "bind e +use";
                         return true;
                     });
    const auto se = [](const std::string& k) { return k == "MENUS_KEYBIND_OR" ? std::string("oder") : std::string(); };
    pruefe(bindungsText(b, "+forward", Dialekt::SP, Spiel::JA, se) == "KP_5 oder W",
           "die ersten zwei Tasten in Nummernfolge (KP_5 0x15, W 0x57; UPARROW waere die dritte)");
    pruefe(bindungsText(b, "+attack", Dialekt::SP, Spiel::JA, se) == "MOUSE1" && bindungsText(b, "+use", Dialekt::SP, Spiel::JA, se) == "E",
           "eine Taste; exec weiter.cfg ausgefuehrt");
    pruefe(bindungsText(b, "+altattack", Dialekt::SP, Spiel::JA, se) == "???", "unbindall loescht vorher Gebundenes");
    pruefe(bindungsText(b, "say hallo", Dialekt::SP, Spiel::JA, se) == "???", "Befehl nicht in g_bindCommands: ???");
    pruefe(bindungsText(b, "+scores", Dialekt::MP, Spiel::JA, se) == "TAB" && bindungsText(b, "+scores", Dialekt::SP, Spiel::JA, se) == "???",
           "+scores nur in der MP-Liste");
    const auto freund = [](const std::string& k) { return k == "KEYNAMES_KEYNAME_MOUSE1" ? std::string("Maus 1") : std::string(); };
    pruefe(bindungsText(b, "+attack", Dialekt::SP, Spiel::JA, freund) == "Maus 1", "KEYNAMES_KEYNAME_* als Name");
    pruefe(vorgabeConfig(Dialekt::SP, Spiel::JA) == "default.cfg" && vorgabeConfig(Dialekt::MP, Spiel::JK2) == "mpdefault.cfg" &&
               vorgabeConfig(Dialekt::SP, Spiel::MD) == "MovieDuels-SP-default.cfg",
           "Vorgabe-Config wie Com_Init");
    std::printf("menuetasten: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
