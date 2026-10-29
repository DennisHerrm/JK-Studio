// karteteiler.cpp - Karten-Editor Stufe 1b: die Felder werden so gross wie in
// NetRadiant-Custom, weil Qts Rechnung nachgebaut ist (bhed/karte/teiler.h).
//
// Die Sollwerte sind aus dem Foto von NRC 20260114 gemessen (Bildschirm 2560 x
// 1600 bei 150 %, NRC maximiert; Qt 5.15 rundet 1,5 auf den Massstab 2, also
// logische Bildpunkte = Fotopunkte / 2): Client 1280 x 747, Menue 21,
// Werkzeugleiste 39, Statuszeile 21, Filterleiste rechts 39 breit, Griff 4.
//   Regular: XY 495 breit, 629 hoch; Konsole 33; Kamera 742 breit, 414 hoch;
//            Texturbrowser 248 hoch.
//   Statuszeile: Zaehler ab 655 (168 breit), Raster "G:16  F:BP  C:14
//            L:OFF  " = 124,11 Punkte, aufgerundet 125 (QLabel::sizeHint)
//            -> ab 1155; Befehl, Position, Textur teilen sich den Rest.
// Dazu Ziehen eines Griffs wie QSplitter::moveSplitter (Mindestgroessen,
// Einklappen unterhalb der halben Mindestgroesse) und Vergroessern danach
// (verteilt im Verhaeltnis der gezogenen Groessen).
#include "bhed/karte/teiler.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string text(const std::vector<int>& v) {
    std::string s;
    for (const int i : v) { s += (s.empty() ? "" : ",") + std::to_string(i); }
    return s;
}

Teiler zwei(int min0, int min1) {
    Teiler t;
    t.kinder.resize(2);
    t.kinder[0].minimum = min0;
    t.kinder[1].minimum = min1;
    return t;
}

void regular() {
    // m_hSplit: XY-Spalte | Kamera-Spalte, Platz 1280 - 39 (Filterleiste).
    Teiler h = zwei(0, 0);
    h.setzeGroessen({384, 576});
    h.anordnen(0, 1241);
    pruefe(h.groessen() == std::vector<int>{495, 742}, "m_hSplit {384,576} auf 1241: 495 | 742 (ist " + text(h.groessen()) + ")");
    pruefe(h.kinder[1].pos == 499, "Kamera-Spalte ab 499 (Griff 4)");
    // m_vSplit: XY ueber Konsole, Hoehe 747 - 21 - 39 - 21 = 666.
    Teiler v = zwei(0, 0);
    v.setzeGroessen({377, 20});
    v.anordnen(0, 666);
    pruefe(v.groessen() == std::vector<int>{629, 33}, "m_vSplit {377,20} auf 666: 629 / 33 (ist " + text(v.groessen()) + ")");
    Teiler v2 = zwei(0, 0);
    v2.setzeGroessen({250, 150});
    v2.anordnen(0, 666);
    pruefe(v2.groessen() == std::vector<int>{414, 248}, "m_vSplit2 {250,150} auf 666: 414 / 248 (ist " + text(v2.groessen()) + ")");
    // Ein anderes Fenster: dasselbe Verhaeltnis (Qt verteilt nach sizer).
    h.anordnen(0, 1001);
    pruefe(h.groessen() == std::vector<int>{399, 598}, "m_hSplit auf 1001: 399 | 598 (ist " + text(h.groessen()) + ")");
}

void split() {
    // eSplit: keine Groessen gesetzt (RestoreGuiState nur ohne
    // FloatingGroupDialog); verschachtelte QSplitter haben den Hinweis
    // (0, 0) - beide gleich gross.
    Teiler h = zwei(0, 0);
    h.kinder[0].hinweis = 0;
    h.kinder[1].hinweis = 0;
    h.anordnen(0, 1241);
    pruefe(h.groessen() == std::vector<int>{619, 618}, "Split m_hSplit ohne Groessen: 619 | 618 (ist " + text(h.groessen()) + ")");
    // GL-Fenster ohne gueltigen Hinweis, vorher 640 x 480 (ohne Eltern
    // erzeugt): gleich hoch.
    Teiler v = zwei(0, 0);
    for (auto& k : v.kinder) {
        k.hinweis = -1;
        k.startGroesse = 480;
    }
    v.anordnen(0, 666);
    pruefe(v.groessen() == std::vector<int>{331, 331}, "Split m_vSplit: 331 / 331 (ist " + text(v.groessen()) + ")");
}

void ziehen() {
    Teiler h = zwei(0, 0);
    h.setzeGroessen({384, 576});
    h.anordnen(0, 1241);
    h.ziehen(600, 1);
    pruefe(h.groessen() == std::vector<int>{600, 637} && h.kinder[1].pos == 604, "Griff auf 600: 600 | 637");
    h.anordnen(0, 1000);
    // sizer jetzt 600:637 -> 996 * 600 / 1237 = 483.1
    pruefe(h.groessen() == std::vector<int>{483, 513}, "danach kleiner: im neuen Verhaeltnis 483 | 513 (ist " + text(h.groessen()) + ")");
    // Mindestgroesse und Einklappen: zweites Kind mindestens 50.
    Teiler v = zwei(0, 50);
    v.setzeGroessen({377, 20});
    v.anordnen(0, 666);
    // setSizes hebt den sizer auf die Mindestgroesse (setSizes_helper): 377:50.
    pruefe(v.groessen() == std::vector<int>{584, 78}, "setSizes hebt 20 auf die Mindestgroesse 50: 584 / 78 (ist " + text(v.groessen()) + ")");
    v.ziehen(640, 1);   // 28 ueber max (612), weniger als die halbe Mindestgroesse
    pruefe(v.groessen() == std::vector<int>{612, 50} && !v.kinder[1].eingeklappt, "knapp ueber dem Ende: bleibt bei der Mindestgroesse");
    v.ziehen(660, 1);   // 48 ueber max: mehr als die Haelfte und >= min(40, 50)
    pruefe(v.groessen() == std::vector<int>{662, 0} && v.kinder[1].eingeklappt, "weit genug: zweites Feld klappt ein (ist " + text(v.groessen()) + ")");
    // MaximizeView (mainframe.cpp:1566): setSizes({9999, 0})
    Teiler m = zwei(0, 0);
    m.setzeGroessen({9999, 0});
    m.anordnen(0, 1241);
    pruefe(m.groessen() == std::vector<int>{1237, 0} && !m.kinder[1].eingeklappt, "setSizes {9999,0}: erstes Feld fuellt, Griff bleibt");
}

void statuszeile() {
    // Befehl, Position, Zaehler (3 x Bild 16 + Zahl 30, Abstand 6 = 168),
    // Textur, Raster ("G:16  F:BP  C:14  L:OFF  " = 125).
    const std::vector<Spanne> s = statuszeileVerteilen({{0, 0, 1}, {0, 0, 1}, {168, 168, 0}, {0, 0, 1}, {125, 125, 0}}, 1280);
    std::vector<int> lagen;
    std::vector<int> breiten;
    for (const Spanne& x : s) {
        lagen.push_back(x.pos);
        breiten.push_back(x.groesse);
    }
    pruefe(lagen == std::vector<int>{2, 328, 655, 829, 1155}, "Statuszeile: Lagen 2, 328, 655, 829, 1155 (ist " + text(lagen) + ")");
    pruefe(breiten == std::vector<int>{320, 321, 168, 320, 125}, "Statuszeile: Breiten 320, 321, 168, 320, 125 (ist " + text(breiten) + ")");
}

void leisten() {
    // Filterleiste rechts: 5 | 6 | 5 | Reset | Region, Hide (19 Knoepfe, 4
    // Trenner); Plugin-Leiste darunter (bobToolz: Trenner am Anfang
    // versteckt, Polygon | 3 | 4 | 1).
    const LeistenEintrag k{31, false};
    const LeistenEintrag t{8, false};
    std::vector<LeistenEintrag> filter;
    for (int g : {5, 6, 5, 1, 2}) {
        if (!filter.empty()) { filter.push_back(t); }
        for (int i = 0; i < g; ++i) { filter.push_back(k); }
    }
    const std::vector<LeistenEintrag> plugin = {{8, true}, k, t, k, k, k, t, k, k, k, k, t, k};
    const LeistenErgebnis f0 = leisteAnordnen(filter, 9999);
    const LeistenErgebnis p0 = leisteAnordnen(plugin, 9999);
    pruefe(f0.minimum == 60 && f0.hinweis == 638, "Filterleiste: minimumSize 60, sizeHint 638 (ist " + std::to_string(f0.minimum) + ", " + std::to_string(f0.hinweis) + ")");
    pruefe(p0.minimum == 60 && p0.hinweis == 320, "Plugin-Leiste: minimumSize 60, sizeHint 320 (ist " + std::to_string(p0.minimum) + ", " + std::to_string(p0.hinweis) + ")");
    // Andockbereich rechts, 666 hoch: Filterleiste 606, Plugin-Leiste 60 (gemessen).
    const std::vector<Spanne> zeile = leistenZeile({{f0.minimum, f0.hinweis}, {p0.minimum, p0.hinweis}}, 666);
    pruefe(zeile.size() == 2 && zeile[0].pos == 0 && zeile[0].groesse == 606 && zeile[1].pos == 606 && zeile[1].groesse == 60,
           "rechter Andockbereich: Filterleiste 0..606, Plugin-Leiste 606..666");
    const LeistenErgebnis f = leisteAnordnen(filter, 606);
    pruefe(f.lage[0] == 13 && f.lage[6] == 176 && f.lage[13] == 370 && f.lage[19] == 533,
           "Filterleiste: Gruppen ab 13, 176, 370, Reset bei 533 (gemessen)");
    pruefe(f.erweiterung && f.erweiterungLage == 591 && f.ersterVersteckter == 21 && f.lage[21] == -1,
           "Filterleiste: Erweiterungsknopf bei 591, Region und Hide im Menue (ist " + std::to_string(f.erweiterungLage) + ", ab " + std::to_string(f.ersterVersteckter) + ")");
    const LeistenErgebnis p = leisteAnordnen(plugin, 60);
    pruefe(p.lage[1] == 13 && p.erweiterung && p.erweiterungLage == 45 && p.ersterVersteckter == 2,
           "Plugin-Leiste: Polygon bei 13, Erweiterung bei 45 (= 651 im Fenster), der Trenner weicht (ist " + std::to_string(p.erweiterungLage) + ", ab " + std::to_string(p.ersterVersteckter) + ")");
    // Hauptleiste quer: passt, keine Erweiterung, letzte Leiste bis zum Ende.
    const std::vector<Spanne> oben = leistenZeile({{60, 678}}, 1280);
    pruefe(oben.size() == 1 && oben[0].groesse == 1280, "obere Zeile: die einzige Leiste reicht bis zum Ende");
}

// Stufe 9c: die Leisten an den Raendern (Vorgabe NRC: Hauptleiste oben, Filter- und
// Plugin-Leiste rechts) und verschoben; Text der Einstellung hin und zurueck
void andocken() {
    std::vector<LeistenPlatz> l(3);
    l[0] = {LeistenRand::Oben, 0, true, 300, 900};
    l[1] = {LeistenRand::Rechts, 0, true, 200, 500};
    l[2] = {LeistenRand::Rechts, 1, true, 50, 80};
    LeistenAnordnung a = leistenAndocken(l, 0, 21, 1280, 700, 39);
    pruefe(a.leisten[0].da && a.leisten[0].waagerecht && a.leisten[0].y == 21 && a.leisten[0].b == 1280, "Vorgabe: Hauptleiste oben ueber die ganze Breite");
    pruefe(a.leisten[1].x == 1241 && a.leisten[1].y == 60 && !a.leisten[1].waagerecht && a.leisten[2].y == 60 + a.leisten[1].h, "Vorgabe: Filter- und Plugin-Leiste rechts untereinander");
    pruefe(a.x == 0 && a.y == 60 && a.b == 1241 && a.h == 661, "Vorgabe: Mitte 1241 x 661 (ist " + std::to_string(a.b) + " x " + std::to_string(a.h) + ")");
    l[0].rand = LeistenRand::Links;
    l[2].rand = LeistenRand::Unten;
    a = leistenAndocken(l, 0, 21, 1280, 700, 39);
    pruefe(a.leisten[0].x == 0 && a.leisten[0].y == 21 && !a.leisten[0].waagerecht && a.leisten[0].h == 661, "Hauptleiste links, zwischen oben und unten");
    pruefe(a.leisten[2].waagerecht && a.leisten[2].y == 21 + 700 - 39 && a.leisten[2].b == 1280, "Plugin-Leiste unten ueber die ganze Breite");
    pruefe(a.x == 39 && a.y == 21 && a.b == 1280 - 78 && a.h == 700 - 39, "Mitte zwischen links, rechts und unten");
    l[1].rand = LeistenRand::Links;
    l[1].reihenfolge = -1;
    a = leistenAndocken(l, 0, 21, 1280, 700, 39);
    pruefe(a.leisten[1].y == 21 && a.leisten[0].y == 21 + a.leisten[1].h && a.b == 1241, "zwei Leisten links in ihrer Reihenfolge, rechts frei");
    std::vector<std::pair<std::string, LeistenPlatz>> n = {{"haupt", l[0]}, {"filter", l[1]}, {"plugin", l[2]}};
    const std::string s = leistenText(n);
    std::vector<std::pair<std::string, LeistenPlatz>> m = {{"haupt", {}}, {"filter", {}}, {"plugin", {}}};
    pruefe(s == "haupt=links:0;filter=links:-1;plugin=unten:1" && leistenLesen(s, m) && m[0].second.rand == LeistenRand::Links &&
               m[1].second.reihenfolge == -1 && m[2].second.rand == LeistenRand::Unten,
           "Einstellung BhedToolbarDock hin und zurueck (" + s + ")");
    pruefe(!leistenLesen("haupt=mitte:0", m), "unbekannter Rand: nicht uebernommen");
}

}  // namespace

int main() {
    andocken();
    regular();
    split();
    ziehen();
    statuszeile();
    leisten();
    std::printf("karteteiler: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
