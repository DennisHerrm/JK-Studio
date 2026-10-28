// karteverbund.cpp - Stufe 9 Punkt 6: verknuepfte Prefabs (ctest karte_verbund)
//
// Ein Brush wird eine Verbund-Gruppe mit einer verknuepften Kopie 256 daneben. Ein zweiter
// Brush in der ersten Gruppe erscheint (im selben Befehl) auch in der Kopie, verschoben; ein
// Rueckgaengig nimmt beides zurueck; Wiederholen bringt beides; wird ein Exemplar als Ganzes
// verschoben, wandert nur sein Ursprung (die Kopie bleibt); geloest folgt nichts mehr.
#include <cstdio>
#include <memory>
#include <string>

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/prefabverbund.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/texdef.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
using namespace bhed::karte;

std::shared_ptr<Entitaet> gruppe(const Karte& k, int nr) {
    int i = 0;
    for (const auto& e : k.entitaeten()) {
        if (e->wert(kVerbundName).empty()) { continue; }
        if (i++ == nr) { return e; }
    }
    return nullptr;
}
std::shared_ptr<Pinsel> quader(const Karte& k, float x, float y, float z, float h) {
    auto p = std::make_shared<Pinsel>();
    Kasten b;
    b.origin = Vektor3f(x, y, z);
    b.extents = Vektor3f(h, h, h);
    TexturProjektion t;
    texdefStandard(k.kontext.art(), t);
    p->quaderBauen(b, "textures/test/holz", t, k.kontext);
    return p;
}
float mitteX(const Karte& k, const Primitiv& p) {
    return static_cast<const Pinsel&>(p).grenzen(k.kontext).origin[0];
}
}  // namespace

int main() {
    Karte k;
    (void)karteLesen("{\n\"classname\" \"worldspawn\"\n}\n", k);
    k.anmelden();
    PrefabVerbund v;
    k.undo.vorAbschluss = [&] { v.abgleichen(k, true); };
    {
        Befehl b(k.undo, "brush");
        auto p = quader(k, 32, 32, 32, 32);
        k.welt()->kindEinfuegen(p);
        k.auswahl.waehlen(k.welt(), p);
    }
    v.abgleichen(k, false);
    const auto kopie = verbundKopieAnlegen(k, "tisch", Vektor3f(256, 0, 0), 8.0F);
    v.abgleichen(k, false);
    const auto a = gruppe(k, 0);
    const auto c = gruppe(k, 1);
    expect("zwei Exemplare, je ein Brush, die Kopie 256 daneben, worldspawn leer",
           kopie && a && c && a->kinder().size() == 1 && c->kinder().size() == 1 && k.welt()->kinder().empty() &&
               mitteX(k, *c->kinder()[0]) - mitteX(k, *a->kinder()[0]) == 256.0F);
    expect("Ursprunge gesetzt (Mitte unten auf dem Raster)", a && a->wert(kVerbundUrsprung) == "32 32 0" && c->wert(kVerbundUrsprung) == "288 32 0");
    {
        Befehl b(k.undo, "brush in A");
        a->kindEinfuegen(quader(k, 32, 32, 96, 16));
    }
    v.abgleichen(k, false);
    expect("ein zweiter Brush in A erscheint auch in der Kopie, 256 verschoben",
           c->kinder().size() == 2 && mitteX(k, *c->kinder()[1]) == 288.0F);
    (void)k.undo.undo();
    v.abgleichen(k, false);
    expect("ein Rueckgaengig nimmt beides zurueck", a->kinder().size() == 1 && c->kinder().size() == 1);
    (void)k.undo.redo();
    v.abgleichen(k, false);
    expect("Wiederholen bringt beides", a->kinder().size() == 2 && c->kinder().size() == 2);
    {
        Befehl b(k.undo, "move A");
        Kontext kl = k.kontext;
        kl.texturLock = true;
        for (const auto& p : a->kinder()) { static_cast<Pinsel&>(*p).transformieren(verschiebungsMatrix(Vektor3f(0, 64, 0)), kl); }
    }
    v.abgleichen(k, false);
    expect("A als Ganzes verschoben: nur sein Ursprung wandert, die Kopie bleibt",
           a->wert(kVerbundUrsprung) == "32 96 0" && c->wert(kVerbundUrsprung) == "288 32 0" &&
               static_cast<const Pinsel&>(*c->kinder()[0]).grenzen(k.kontext).origin[1] == 32.0F);
    {
        Befehl b(k.undo, "brush in C");
        c->kindEinfuegen(quader(k, 288, 32, 200, 8));
    }
    v.abgleichen(k, false);
    expect("eine Aenderung an der Kopie folgt in A (am verschobenen Ursprung)",
           a->kinder().size() == 3 && static_cast<const Pinsel&>(*a->kinder()[2]).grenzen(k.kontext).origin[1] == 96.0F);
    k.auswahl.leeren();
    k.auswahl.waehlen(c, c->kinder()[0]);
    expect("loesen", verbundLoesen(k));
    v.abgleichen(k, false);
    {
        Befehl b(k.undo, "brush in A 2");
        a->kindEinfuegen(quader(k, 32, 96, 300, 8));
    }
    v.abgleichen(k, false);
    expect("geloest: die fruehere Kopie folgt nicht mehr", c->kinder().size() == 3 && a->kinder().size() == 4);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
