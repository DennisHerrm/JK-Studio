// karteqttext.cpp - Karten-Editor Stufe 3, Punkt 0: Textbreiten wie Qt 5 bei NRC.
//
// Die Dialoge waren 1-4 Qt-Punkte schmaler als die NRC-Fotos, weil wir mit GDI
// (Tahoma 11 px, gehintete Vorschuebe, ohne Unterschneidung) gemessen haben.
// Qt 5.15 misst bei aktiver High-DPI-Skalierung mit DirectWrite in
// Entwurfsmassen (gui/qttext_win32.cpp). Die Erwartungen sind unabhaengig
// davon aus den Schriftdateien gerechnet (fontTools: hmtx-Vorschuebe und die
// Paare des GPOS-Features 'kern' - HarfBuzz nimmt GPOS, die alte 'kern'-Tabelle
// nur ohne GPOS - je auf 1/64 gerundet wie QFixed, dazu die negative rechte
// Peilung des letzten Zeichens aus glyf; Skript SP\s3\werk\qt.py) und am Foto
// belegt: Find Brush (dlg_Misc_Find_brush_0.png) hat die Felder bei 86 =
// 11 + 69 + 6, also ist "Entity number" in Tahoma 11 nach Qt 69 breit (GDI: 67).
//
// Aufruf: karteqttext
#include "platform.h"

#include <cstdio>
#include <string>
#include <utility>

namespace {

int g_fehler = 0;

void pruefe(const char* text, const wchar_t* schrift, float pixel, int breite, int vorschub) {
    const int b = bhed::platform::qtTextBreite(text, schrift, pixel, false);
    const int v = bhed::platform::qtTextVorschub(text, schrift, pixel, false);
    const bool ok = b == breite && (vorschub < 0 || v == vorschub);
    if (!ok) { ++g_fehler; }
    std::printf("%s  %-50s %ls %.0f: boundingRect %d (soll %d), horizontalAdvance %d (soll %d)\n", ok ? "OK  " : "FEHL", text, schrift,
                static_cast<double>(pixel), b, breite, v, vorschub);
}

}  // namespace

int main() {
    // Dialogschrift (DEFAULT_GUI_FONT = Tahoma, 11 logische Punkte)
    pruefe("Entity number", L"Tahoma", 11.0F, 69, 68);   // Find Brush: Feld bei 86 im Foto
    pruefe("Brush number", L"Tahoma", 11.0F, 69, 68);
    pruefe("Sides:", L"Tahoma", 11.0F, 30, 29);
    pruefe("Subdivisions:", L"Tahoma", 11.0F, 64, 63);
    pruefe("Truncate", L"Tahoma", 11.0F, 43, 43);         // "Tr" steht nur in der alten 'kern'-Tabelle, nicht in GPOS
    pruefe("Normal", L"Tahoma", 11.0F, 35, 35);
    pruefe("X", L"Tahoma", 11.0F, 7, 6);
    pruefe("Close", L"Tahoma", 11.0F, 26, 26);
    pruefe("Find", L"Tahoma", 11.0F, 21, 20);
    pruefe("Command", L"Tahoma", 11.0F, 50, 49);
    pruefe("Thickness:", L"Tahoma", 11.0F, 52, 51);
    pruefe("OK", L"Tahoma", 11.0F, 15, 14);               // 'K' hat eine negative rechte Peilung
    pruefe("Cancel", L"Tahoma", 11.0F, 32, 32);
    // Menueschrift (lfMenuFont = Segoe UI 9 pt, 12 logische Punkte)
    pruefe("Paste To Camera", L"Segoe UI", 12.0F, 88, 87);   // "To" und "Pa" unterschnitten (GDI ohne: 89)
    pruefe("Complete list of Q3map2 command line parameters", L"Segoe UI", 12.0F, 275, 274);
    pruefe("Arbitrary rotation", L"Segoe UI", 12.0F, 92, 92);
    pruefe("", L"Tahoma", 11.0F, 0, 0);
    // Spanisch und Tschechisch: jedes Zeichen hat eine eigene Glyphe in den
    // Schriften, mit denen der Karten-Editor misst (Menues Segoe UI, Dialoge
    // Tahoma normal und fett) - kein Ersatzkaestchen, keine Ersatzschrift.
    // Der Selbsttest prueft dieselben Zeichen in den ImGui-Schriften.
    {
        const char* const zeichen =
            "\xc3\xb1\xc3\xa1\xc3\xa9\xc3\xad\xc3\xb3\xc3\xba\xc3\xbc\xc2\xbf\xc2\xa1"          // es klein: n, a, e, i, o, u, u mit Zeichen, ? und ! umgekehrt
            "\xc3\x91\xc3\x81\xc3\x89\xc3\x8d\xc3\x93\xc3\x9a\xc3\x9c"                            // es gross: dieselben
            "\xc4\x9b\xc5\xa1\xc4\x8d\xc5\x99\xc5\xbe\xc3\xbd\xc5\xaf\xc4\x8f\xc5\xa5\xc5\x88"  // cs klein: e, s, c, r, z, y, u, d, t, n mit Zeichen
            "\xc4\x9a\xc5\xa0\xc4\x8c\xc5\x98\xc5\xbd\xc3\x9d\xc5\xae\xc4\x8e\xc5\xa4\xc5\x87";  // cs gross: dieselben
        const std::pair<const wchar_t*, bool> schriften[] = {{L"Segoe UI", false}, {L"Tahoma", false}, {L"Tahoma", true}};
        for (const auto& [schrift, fett] : schriften) {
            const std::string fehlt = bhed::platform::qtFehlendeZeichen(zeichen, schrift, fett);
            const bool ok = fehlt.empty();
            if (!ok) { ++g_fehler; }
            std::printf("%s  Spanisch/Tschechisch in %ls%s: %s\n", ok ? "OK  " : "FEHL", schrift, fett ? " fett" : "",
                        ok ? "alle Zeichen da" : ("es fehlen " + fehlt).c_str());
        }
        const bool gegenprobe = bhed::platform::qtFehlendeZeichen("\xe4\xb8\xad", L"Tahoma", false) == "\xe4\xb8\xad";
        if (!gegenprobe) { ++g_fehler; }
        std::printf("%s  Gegenprobe: Tahoma hat kein chinesisches Zeichen\n", gegenprobe ? "OK  " : "FEHL");
    }
    std::printf("%s: %d Fehler\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
