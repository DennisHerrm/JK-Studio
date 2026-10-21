// kartemassenlast.cpp - Karten-Editor Stufe 1a, Punkt 3: Massenlast.
//
// Liest alle Karten, die q3map2 aus den Original-BSPs dekompiliert hat
// (q3map2 -convert -format map_bp bzw. -format map, Ordner
// <wurzel>/<spiel>/bp und <wurzel>/<spiel>/q3), und prueft je Karte:
//   1. der Leser kommt ohne Lesefehler durch,
//   2. schreiben -> lesen ergibt dieselben Strukturen: Entitaeten mit ihren
//      Schluesseln, Pinsel mit ihren beitragenden Flaechen (Ebenenpunkte,
//      Shader, Texdef/BP, Flags), Patches mit allen Kontrollpunkten,
//   3. ein zweites Schreiben ist bytegleich zum ersten; hat die Karte Pinsel
//      ohne beitragende Flaeche, schreibt NRC fuer sie nur "// brush N", die
//      Nummern ruecken beim naechsten Lesen nach - dann muss das dritte
//      Schreiben gleich dem zweiten sein.
// Gezaehlt (und ausgegeben) werden die Pinsel ohne gueltige Wicklung: entartet,
// weniger als vier beitragende Flaechen oder Euler V+F-E != 2 (buildBRep).
// Welche das sind und warum, rechnet tools/karte_pinsel_exakt.py mit
// Bruchzahlen nach (Liste mit dem Namensteil "*"). Dazu Laufzeit und
// Speicher (Windows: hoechster Arbeitsspeicher).
//
// Aufruf: kartemassenlast <wurzel> [<Namensteil: nur diese Karten, mit Einzelheiten> | *]
#include "bhed/karte/kartenformat.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif

using namespace bhed::karte;

namespace {

int g_fehler = 0;
std::string g_nur;   // optional: nur Karten, deren Name das enthaelt, mit Einzelheiten
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        if (g_fehler <= 60) { std::printf("FEHL  %s\n", was.c_str()); }
    }
}

std::string lies(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

double speicherMb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
    }
#endif
    return 0;
}

// Gleich wie geschrieben und gelesen: NaN (q3map2 schreibt fuer manche
// Patch-Punkte "nan", NRC liest und schreibt es so weiter) gilt als gleich NaN.
bool zahlGleich(float a, float b) { return a == b || (std::isnan(a) && std::isnan(b)); }

bool flaechenGleich(const Flaeche& a, const Flaeche& b) {
    const auto& pa = a.projektion();
    const auto& pb = b.projektion();
    bool g = a.ebenenPunkte() == b.ebenenPunkte() && a.shader() == b.shader() && a.flags().inhaltFlags == b.flags().inhaltFlags
          && a.flags().flaechenFlags == b.flags().flaechenFlags && a.flags().wert == b.flags().wert;
    for (int i = 0; i < 2; ++i) {
        g = g && pa.texdef.shift[i] == pb.texdef.shift[i] && pa.texdef.scale[i] == pb.texdef.scale[i];
        for (int j = 0; j < 3; ++j) { g = g && pa.bp.coords[i][j] == pb.bp.coords[i][j]; }
    }
    return g && pa.texdef.rotate == pb.texdef.rotate;
}

struct Stand {
    std::size_t karten = 0, entitaeten = 0, pinsel = 0, flaechen = 0, patches = 0;
    std::size_t ungueltig = 0, ohneBeitrag = 0, nichtBeitragendeFlaechen = 0, nurEuler = 0, mitLeerenPinseln = 0;
    double sekunden = 0;
};

void karte(const std::filesystem::path& p, Stand& st) {
    const std::string name = p.parent_path().parent_path().filename().string() + "/" + p.parent_path().filename().string() + "/" +
                             p.filename().string();
    Karte k1;
    const LeseErgebnis e1 = karteLesen(textmodusLesen(lies(p)), k1);
    // Jede Meldung ausser der Formaterkennung zaehlt als Lesefehler (parse
    // error, bad texdef, unexpected end-of-line, discarding brush, ...).
    std::string meldung;
    for (const auto& m : e1.meldungen) {
        if (m.rfind("detectedFormat = ", 0) != 0 && meldung.empty()) { meldung = m; }
    }
    pruefe(e1.vollstaendig && meldung.empty(), name + ": ohne Lesefehler" + (meldung.empty() ? "" : " (" + meldung + ")"));
    ++st.karten;
    st.entitaeten += k1.entitaeten().size();
    const bool liste = g_nur == "*";   // Pinsel-Liste fuer die exakte Gegenrechnung
    static std::size_t s_zaehler = 0;
    std::size_t leerePinsel = 0;
    for (std::size_t ei = 0; ei < k1.entitaeten().size(); ++ei) {
        const auto& en = k1.entitaeten()[ei];
        for (std::size_t pi = 0; pi < en->kinder().size(); ++pi) {
            const auto& pr = en->kinder()[pi];
            if (pr->art() == Primitiv::Art::Patch) {
                ++st.patches;
                continue;
            }
            const auto& b = static_cast<const Pinsel&>(*pr);
            ++st.pinsel;
            st.flaechen += b.anzahl();
            b.bRep(k1.kontext);   // Wicklungen bauen (evaluateBRep)
            std::size_t beitragend = 0;
            for (const auto& f : b.flaechen()) { beitragend += f->traegtBei() ? 1 : 0; }
            st.nichtBeitragendeFlaechen += b.anzahl() - beitragend;
            const bool gueltig = !b.entartet(k1.kontext) && beitragend >= 4 && !b.eulerFehler(k1.kontext);
            if (liste && (!gueltig || ++s_zaehler % 500 == 0)) {
                std::printf("LISTE %s %zu %zu %d %zu\n", name.c_str(), ei, pi, gueltig ? 1 : 0, beitragend);
            }
            if (!gueltig) {
                ++st.ungueltig;
                if (!g_nur.empty() && !liste) {
                    std::printf("  ungueltig: Entitaet %s, %zu Flaechen, %zu beitragend, entartet %d, Euler %d\n",
                                en->klassenname().c_str(), b.anzahl(), beitragend, b.entartet(k1.kontext) ? 1 : 0,
                                b.eulerFehler(k1.kontext) ? 1 : 0);
                    for (const auto& f : b.flaechen()) {
                        const auto& e = f->ebenenPunkte();
                        const auto& eb = f->ebene();
                        std::printf("    ( %.17g %.17g %.17g ) ( %.17g %.17g %.17g ) ( %.17g %.17g %.17g ) %s\n"
                                    "      Ebene ( %.9g %.9g %.9g ) %.9g, Wicklung %zu Punkte\n",
                                    e[0][0], e[0][1], e[0][2], e[1][0], e[1][1], e[1][2], e[2][0], e[2][1], e[2][2],
                                    f->shader().c_str(), eb.a, eb.b, eb.c, eb.d, static_cast<std::size_t>(f->wicklung().numpoints));
                    }
                }
            }
            if (beitragend == 0) {
                ++st.ohneBeitrag;
                ++leerePinsel;
            }
            // NRC meldet dann nur "Final B-Rep: inconsistent vertex count" (brush.cpp:258)
            if (!b.entartet(k1.kontext) && beitragend >= 4 && b.eulerFehler(k1.kontext)) { ++st.nurEuler; }
        }
    }
    // schreiben -> lesen
    const std::string t1 = karteSchreiben(k1);
    Karte k2;
    const LeseErgebnis e2 = karteLesen(t1, k2);
    pruefe(e2.vollstaendig, name + ": geschriebene Karte wieder lesbar");
    const std::string t2 = karteSchreiben(k2);
    if (leerePinsel == 0) {
        pruefe(t1 == t2, name + ": zweites Schreiben bytegleich");
    } else {
        // Fuer einen Pinsel ohne beitragende Flaeche schreibt NRC nur
        // "// brush N" (brushtokens.h:575-653); beim naechsten Lesen faellt er
        // weg, die Nummern ruecken nach. Ab dann ist die Datei stabil.
        ++st.mitLeerenPinseln;
        Karte k3;
        const LeseErgebnis e3 = karteLesen(t2, k3);
        pruefe(e3.vollstaendig && karteSchreiben(k3) == t2, name + ": nach Wegfall der leeren Pinsel stabil");
    }
    if (!g_nur.empty() && g_nur != "*" && t1 != t2) {
        std::size_t i = 0;
        while (i < t1.size() && i < t2.size() && t1[i] == t2[i]) { ++i; }
        const std::size_t a = t1.rfind('\n', i == 0 ? 0 : i - 1);
        const std::size_t von = a == std::string::npos ? 0 : a + 1;
        std::printf("  erster Unterschied bei Byte %zu:\n  t1: %s\n  t2: %s\n", i, t1.substr(von, 160).c_str(),
                    t2.substr(von, 160).c_str());
    }
    // Strukturen: Entitaeten ohne verworfene leere Gruppen, Pinsel ohne die
    // ohne beitragende Flaeche, Flaechen nur beitragende
    std::vector<const Entitaet*> l1;
    for (const auto& en : k1.entitaeten()) {
        if (en->istBehaelter() && en->kinder().empty() && en->klassenname() != "worldspawn" && !en->hat("origin")) { continue; }
        l1.push_back(en.get());
    }
    bool gleich = l1.size() == k2.entitaeten().size();
    std::string wo;
    for (std::size_t i = 0; gleich && i < l1.size(); ++i) {
        wo = "Entitaet " + std::to_string(i);
        const Entitaet& a = *l1[i];
        const Entitaet& b = *k2.entitaeten()[i];
        gleich = a.schluessel() == b.schluessel();
        std::vector<const Primitiv*> pa;
        for (const auto& pr : a.kinder()) {
            if (pr->art() == Primitiv::Art::Pinsel && !static_cast<const Pinsel&>(*pr).hatBeitragendeFlaechen(k1.kontext)) { continue; }
            pa.push_back(pr.get());
        }
        gleich = gleich && pa.size() == b.kinder().size();
        for (std::size_t j = 0; gleich && j < pa.size(); ++j) {
            wo = "Entitaet " + std::to_string(i) + ", Primitiv " + std::to_string(j);
            const Primitiv& x = *pa[j];
            const Primitiv& y = *b.kinder()[j];
            gleich = x.art() == y.art();
            if (!gleich) { break; }
            if (x.art() == Primitiv::Art::Patch) {
                const auto& px = static_cast<const Patch&>(x);
                const auto& py = static_cast<const Patch&>(y);
                gleich = px.breite() == py.breite() && px.hoehe() == py.hoehe() && px.shader() == py.shader();
                for (std::size_t q = 0; gleich && q < px.punkte().size(); ++q) {
                    const auto& u = px.punkte()[q];
                    const auto& v = py.punkte()[q];
                    gleich = zahlGleich(u.vertex.x(), v.vertex.x()) && zahlGleich(u.vertex.y(), v.vertex.y())
                          && zahlGleich(u.vertex.z(), v.vertex.z()) && zahlGleich(u.texcoord.x(), v.texcoord.x())
                          && zahlGleich(u.texcoord.y(), v.texcoord.y());
                }
            } else {
                const auto& bx = static_cast<const Pinsel&>(x);
                const auto& by = static_cast<const Pinsel&>(y);
                std::vector<const Flaeche*> fx;
                for (const auto& f : bx.flaechen()) {
                    if (f->traegtBei()) { fx.push_back(f.get()); }
                }
                gleich = fx.size() == by.anzahl();
                for (std::size_t q = 0; gleich && q < fx.size(); ++q) { gleich = flaechenGleich(*fx[q], by[q]); }
            }
        }
    }
    pruefe(gleich, name + ": schreiben -> lesen ergibt dieselben Strukturen");
    if (!g_nur.empty() && g_nur != "*" && !gleich) {
        std::printf("  bei %s; Entitaeten: k1 %zu (ohne leere Gruppen %zu), k2 %zu\n", wo.c_str(), k1.entitaeten().size(), l1.size(),
                    k2.entitaeten().size());
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || !std::filesystem::is_directory(argv[1])) {
        std::printf("kartemassenlast: kein Ordner - uebersprungen\n");
        return 0;
    }
    const std::filesystem::path wurzel = argv[1];
    if (argc > 2) { g_nur = argv[2]; }
    std::vector<std::filesystem::path> ordner;
    for (const auto& spiel : std::filesystem::directory_iterator(wurzel)) {
        for (const char* fmt : {"bp", "q3"}) {
            if (std::filesystem::is_directory(spiel.path() / fmt)) { ordner.push_back(spiel.path() / fmt); }
        }
    }
    std::sort(ordner.begin(), ordner.end());
    Stand gesamt;
    for (const auto& o : ordner) {
        Stand st;
        const auto t0 = std::chrono::steady_clock::now();
        std::vector<std::filesystem::path> karten;
        for (const auto& d : std::filesystem::directory_iterator(o)) {
            if (d.path().extension() == ".map" && (g_nur.empty() || g_nur == "*" || d.path().filename().string().find(g_nur) != std::string::npos)) {
                karten.push_back(d.path());
            }
        }
        std::sort(karten.begin(), karten.end());
        for (const auto& p : karten) { karte(p, st); }
        st.sekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::printf("%s/%s: %zu Karten, %zu Entitaeten, %zu Pinsel (%zu Flaechen, davon %zu nicht beitragend), %zu Patches; "
                    "ungueltige Pinsel %zu (ohne jede beitragende Flaeche %zu, nur Euler-Meldung %zu), "
                    "%zu Karten mit leeren Pinseln; %.1f s\n",
                    o.parent_path().filename().string().c_str(), o.filename().string().c_str(), st.karten, st.entitaeten,
                    st.pinsel, st.flaechen, st.nichtBeitragendeFlaechen, st.patches, st.ungueltig, st.ohneBeitrag, st.nurEuler,
                    st.mitLeerenPinseln, st.sekunden);
        gesamt.karten += st.karten;
        gesamt.pinsel += st.pinsel;
        gesamt.ungueltig += st.ungueltig;
        gesamt.sekunden += st.sekunden;
    }
    std::printf("gesamt: %zu Karten, %zu Pinsel, %zu ungueltig, %.1f s, hoechster Arbeitsspeicher %.0f MB\n", gesamt.karten,
                gesamt.pinsel, gesamt.ungueltig, gesamt.sekunden, speicherMb());
    std::printf("kartemassenlast: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
