// karteebenen.cpp - Karten-Editor Stufe 2, Punkt 10: die Ebenen wie
// NetRadiant-Custom 20260114 (libs/layers.h, radiant/layerswindow.cpp).
//
// Sollwerte von Hand: die Ebenen liegen in Tiefensuche (update_ownIndices),
// neue Ebenen kommen ans Ende ihrer Elternliste und werden aktuell; Loeschen
// nimmt die Unterebenen mit und gibt deren Knoten der aktuellen Ebene (ist
// sie geloescht: der ersten obersten); Gruppenentitaeten haben keine Ebene.
#include "bhed/karte/ebenen.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <cstdio>
#include <memory>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::shared_ptr<Pinsel> quader(Karte& k, const std::shared_ptr<Entitaet>& ent, float x, int schicht) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(Vektor3f(x, 0, 0), Vektor3f(8, 8, 8)), "textures/a/b", proj, k.kontext);
    p->schicht = schicht;
    ent->kindEinfuegen(p);
    return p;
}

std::string namen(const Karte& k) {
    std::string s;
    for (const Schicht& x : k.schichten) { s += x.name + "(" + std::to_string(x.eltern) + ") "; }
    return s;
}

void baum() {
    Karte k;
    const auto welt = k.weltHolenOderAnlegen();
    int a = -1;
    (void)ebeneNeu(k, -1, "A", &a);
    pruefe(a == 1 && k.aktuelleSchicht == 1 && namen(k) == "0(-1) A(-1) ", "New Layer A: hinter 0, aktuell (" + namen(k) + ")");
    int a1 = -1;
    (void)ebeneNeu(k, a, "A1", &a1);
    int b = -1;
    (void)ebeneNeu(k, -1, "B", &b);
    pruefe(namen(k) == "0(-1) A(-1) A1(1) B(-1) " && b == 3, "A1 unter A, B oben: Tiefensuche (" + namen(k) + ")");
    const auto inA = quader(k, welt, 0, a);
    const auto inA1 = quader(k, welt, 100, a1);
    const auto inB = quader(k, welt, 200, b);
    int z = -1;
    const EbenenNummern n = ebeneNeu(k, 0, "Z", &z);
    pruefe(namen(k) == "0(-1) Z(0) A(-1) A1(2) B(-1) " && z == 1, "Z unter 0: alles danach rueckt (" + namen(k) + ")");
    pruefe(n.size() == 4 && n[0] == 0 && n[1] == 2 && n[2] == 3 && n[3] == 4, "Nummern alt -> neu");
    pruefe(inA->schicht == 2 && inA1->schicht == 3 && inB->schicht == 4 && k.aktuelleSchicht == 1, "Knoten umnummeriert, Z aktuell");
    // Delete A: A1 mit, die Knoten auf die aktuelle (Z)
    const EbenenNummern d = ebeneLoeschen(k, 2);
    pruefe(namen(k) == "0(-1) Z(0) B(-1) " && d.size() == 5 && d[2] == -1 && d[3] == -1 && d[4] == 2, "Delete A samt A1 (" + namen(k) + ")");
    pruefe(inA->schicht == 1 && inA1->schicht == 1 && inB->schicht == 2, "Knoten von A und A1 auf Z");
    // Delete der aktuellen: die erste oberste wird aktuell und nimmt die Knoten
    k.aktuelleSchicht = 2;
    (void)ebeneLoeschen(k, 2);
    pruefe(namen(k) == "0(-1) Z(0) " && k.aktuelleSchicht == 0 && inB->schicht == 0, "Delete der aktuellen B: Ziel 0");
    pruefe(ebeneLoeschen(k, 0).empty() && k.schichten.size() == 2, "die einzige oberste Ebene laesst sich nicht loeschen");
    // Ziehen und Ablegen: Z an die Wurzel vor 0
    (void)ebeneNeu(k, -1, "C", nullptr);
    (void)ebeneVerschieben(k, 1, -1, 0);
    pruefe(namen(k) == "Z(-1) 0(-1) C(-1) " && inA->schicht == 0 && inB->schicht == 1 && k.aktuelleSchicht == 2,
           "Z vor 0 abgelegt (" + namen(k) + ")");
    (void)ebeneVerschieben(k, 2, 0, 0);
    pruefe(namen(k) == "Z(-1) C(0) 0(-1) " && inB->schicht == 2 && k.aktuelleSchicht == 1, "C unter Z (" + namen(k) + ")");
    // Schreiben: layerdef in Tiefensuche, Eltern als Nummer
    const std::string t = karteSchreiben(k);
    pruefe(t.find("//@$& layerdef \"Z\" -1 0 0 0\n//@$& layerdef \"C\" 0 0 0 0\n//@$& layerdef \"0\" -1 0 0 0\n") != std::string::npos,
           "layerdef-Zeilen in Tiefensuche");
    pruefe(ebenenName("a\"b\"c") == "abc", "Name ohne '\"'");
    ebeneUmbenennen(k, 0, "Wald");
    ebeneFarbe(k, 0, {255, 0, 0});
    pruefe(k.schichten[0].name == "Wald" && k.schichten[0].farbe[0] == 255, "Rename / Color");
    pruefe(ebenenUnterbaum(k, 0).size() == 2 && ebenenTiefe(k, 1) == 1 && ebenenKinder(k, -1).size() == 2, "Unterbaum, Tiefe, Kinder");
}

void auswahl() {
    Karte k;
    const auto welt = k.weltHolenOderAnlegen();
    int a = -1;
    (void)ebeneNeu(k, -1, "A", &a);
    int a1 = -1;
    (void)ebeneNeu(k, a, "A1", &a1);
    const auto p0 = quader(k, welt, 0, 0);
    const auto pa = quader(k, welt, 100, a);
    const auto pa1 = quader(k, welt, 200, a1);
    auto gruppe = k.neueEntitaet("func_group", true);
    k.entitaetEinfuegen(gruppe);
    const auto ga = quader(k, gruppe, 300, a);
    auto licht = k.neueEntitaet("light", false);
    licht->schicht = 0;
    k.entitaetEinfuegen(licht);
    // Move to layer: Pinsel und Punktentitaet, nicht die Gruppe
    k.auswahl.waehlen(welt, p0);
    k.auswahl.waehlen(licht);
    k.auswahl.waehlen(gruppe);
    auswahlAufEbene(k, a1);
    pruefe(p0->schicht == a1 && licht->schicht == a1 && gruppe->schicht == kSchichtKeine, "Move to layer: Pinsel und Licht, die Gruppe nicht");
    p0->schicht = 0;
    licht->schicht = 0;
    k.auswahl.leeren();
    // Select (LayerSelectWalker) ueber A: nur A selbst
    ebenenWaehlen(k, {a}, true);
    pruefe(k.auswahl.anzahl() == 2 && k.auswahl.gewaehlt(welt.get(), pa.get()) && k.auswahl.gewaehlt(gruppe.get(), ga.get()), "Select A: zwei Pinsel");
    ebenenWaehlen(k, {a}, false);
    pruefe(k.auswahl.anzahl() == 0, "Alt: abwaehlen");
    // Strg+Q: die Ebene des Gewaehlten und alle darunter
    k.auswahl.waehlen(welt, pa);
    auswahlAufEbenenErweitern(k);
    pruefe(k.auswahl.anzahl() == 3 && k.auswahl.gewaehlt(welt.get(), pa1.get()) && !k.auswahl.gewaehlt(welt.get(), p0.get()),
           "Expand Selection To Layers: A und A1");
    // Verstecken: A (mit A1 vom Browser: items_iterate_recursively)
    ebenenSichtbar(k, ebenenUnterbaum(k, a), false);
    pruefe(!pa->sichtbar() && !pa1->sichtbar() && !ga->sichtbar() && p0->sichtbar() && k.auswahl.anzahl() == 0, "Hide A: versteckt und abgewaehlt");
    pruefe(gruppe->ebeneVersteckt && !welt->ebeneVersteckt, "die Gruppe (alles versteckt) mit, worldspawn nie");
    ebenenWaehlen(k, {a}, true);
    pruefe(k.auswahl.anzahl() == 0, "versteckte Knoten werden nicht gewaehlt");
    ebenenSichtbar(k, ebenenUnterbaum(k, a), true);
    pruefe(pa->sichtbar() && pa1->sichtbar() && ga->sichtbar() && !gruppe->ebeneVersteckt, "Show A: alles wieder da");
    // alle verstecken: worldspawn bleibt
    std::vector<int> alle;
    for (std::size_t i = 0; i < k.schichten.size(); ++i) { alle.push_back(static_cast<int>(i)); }
    ebenenSichtbar(k, alle, false);
    pruefe(!p0->sichtbar() && !licht->sichtbar() && welt->sichtbar(), "Hide All Layers: worldspawn sichtbar");
    ebenenSichtbar(k, alle, true);
    pruefe(p0->sichtbar() && licht->sichtbar(), "Show All Layers");
}

}  // namespace

int main() {
    baum();
    auswahl();
    std::printf("karteebenen: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
