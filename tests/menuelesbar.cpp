// menuelesbar.cpp - Menue-Editor M6 Punkt 9: Lesbarkeit - zu kleine Schrift, Kontrast auf gefuellten
// Flaechen (WCAG 2), Text, der nur in einer Sprache nicht ins Feld passt. Gemessen mit einer
// kuenstlichen Schrift (jedes Zeichen 10 breit, Hoehe 20 bei textscale 1) wie menuevorschau.
#include "bhed/menue/lesbarkeit.h"

#include <cmath>
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
std::string fontdat() {
    std::string d;
    auto put = [&](const void* p, std::size_t n) { d.append(static_cast<const char*>(p), n); };
    for (int i = 0; i < 256; ++i) {
        const short w = 8, h = 16, adv = 10, off = 1;
        const int base = 14;
        const float s = static_cast<float>(i) / 256.0F, t = 0, s2 = s + 1.0F / 256.0F, t2 = 0.5F;
        put(&w, 2); put(&h, 2); put(&adv, 2); put(&off, 2); put(&base, 4);
        put(&s, 4); put(&t, 4); put(&s2, 4); put(&t2, 4);
    }
    const short ps = 20, hh = 20, asc = 16, desc = 4, kor = 0;
    put(&ps, 2); put(&hh, 2); put(&asc, 2); put(&desc, 2); put(&kor, 2);
    d.append(2, '\0');
    return d;
}
int zaehle(const std::vector<LesbarkeitsBefund>& v, LesbarArt a, int item) {
    int n = 0;
    for (const LesbarkeitsBefund& b : v) { n += (b.art == a && b.item == item) ? 1 : 0; }
    return n;
}
}  // namespace

int main() {
    pruefe(std::fabs(kontrast(Farbe{1, 1, 1, 1}, Farbe{0, 0, 0, 1}) - 21.0F) < 0.01F, "Kontrast weiss/schwarz 21:1");
    pruefe(std::fabs(kontrast(Farbe{0.5F, 0.5F, 0.5F, 1}, Farbe{0.5F, 0.5F, 0.5F, 1}) - 1.0F) < 0.001F, "Kontrast gleich 1:1");
    Schrift f;
    pruefe(ladeFontdat("probe", fontdat(), f), "Schrift");
    Umgebung u;
    u.schrift = [&](int) -> const Schrift* { return &f; };
    Texte en, de;
    en.werte["MENUS_OK"] = "Okay";          // 4 Zeichen = 40 breit
    de.werte["MENUS_OK"] = "Einverstanden";  // 13 Zeichen = 130 breit
    u.texte = &en;
    const std::string t =
        "{\n\tmenuDef\n\t{\n\t\tname m\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
        "\t\titemDef { name klein text \"winzig\" textscale 0.3 rect 10 10 200 20 visible 1 }\n"
        "\t\titemDef { name grau text \"grau\" style WINDOW_STYLE_FILLED backcolor 0.5 0.5 0.5 1 forecolor 0.6 0.6 0.6 1 rect 10 40 200 30 textscale 1 visible 1 }\n"
        "\t\titemDef { name ok text \"@MENUS_OK\" textscale 1 rect 10 80 60 30 visible 1 }\n"
        "\t\titemDef { name gut text \"Gut\" style WINDOW_STYLE_FILLED backcolor 0 0 0 1 forecolor 1 1 1 1 textscale 1 rect 10 120 200 30 visible 1 }\n"
        "\t\titemDef { name lang text \"Viel zu langer Text\" textscale 1 rect 10 160 40 30 visible 1 }\n"
        "\t}\n}\n";
    LeseOptionen o;
    const Datei d = lies("ui/lesbar.menu", t, o);
    pruefe(d.meldungen.empty(), "gelesen ohne Meldung");
    const std::vector<LesbarkeitsBefund> b = lesbarkeit(d, 0, u, {{"English", &en}, {"German", &de}});
    pruefe(zaehle(b, LesbarArt::SchriftKlein, 0) == 1, "textscale 0.3: Hoehe 6 unter 8");
    pruefe(zaehle(b, LesbarArt::KontrastGering, 1) == 1, "grau auf grau: Kontrast unter 3:1");
    pruefe(zaehle(b, LesbarArt::TextZuLang, 2) == 1 && b.back().sprache == "German", "@MENUS_OK passt englisch, deutsch nicht");
    pruefe(zaehle(b, LesbarArt::SchriftKlein, 3) + zaehle(b, LesbarArt::KontrastGering, 3) + zaehle(b, LesbarArt::TextZuLang, 3) == 0, "weiss auf schwarz: nichts");
    pruefe(zaehle(b, LesbarArt::TextZuLang, 4) == 0, "in keiner Sprache passend (Klickflaeche kleiner als Text): kein Sprachbefund");
    pruefe(lesbarkeit(d, 0, u, {{"English", &en}}).size() == 2, "eine Sprache: keine Laengenpruefung");
    if (g_fehler == 0) { std::printf("menue_lesbar: alles gruen\n"); }
    return g_fehler == 0 ? 0 : 1;
}
