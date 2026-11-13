// modell/ablage.cpp - Flaechen von einem Modell in ein anderes kopieren (siehe bhed/modell/ablage.h)
#include "bhed/modell/ablage.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace bhed::modell {
namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Was nur zur Datei der Quelle gehoert: der Schreiber nimmt Rohfelder, solange sie zu den Werten passen - in einem anderen Modell
// passen die Knochennummern der Datei nicht mehr, und die Ecknummern einer .md3 zeigen in eine andere Datei.
void dateiFelderWeg(Teil& t) {
    t.dateiName.clear();
    t.dateiShader.clear();
    t.dateiKinder.clear();
    t.dateiFlaeche = -1;
    t.kinder.clear();
    for (Netz& n : t.stufen) {
        n.dateiRefs.clear();
        n.dateiRefsAufgeloest.clear();
        n.ausDatei = false;
        for (Ecke& e : n.ecken) {
            e.ausDatei = false;
            e.dateiPack = 0;
            for (std::uint8_t& g : e.dateiGewicht) { g = 0; }
            e.dateiIndex = -1;
        }
    }
}

// Die Gewichte einer Ecke auf die Knochen des Ziels: gleiche Ziele werden ein Gewicht (Reihenfolge des ersten Auftretens).
void eckeUmlegen(Ecke& e, const std::vector<int>& weg) {
    std::uint8_t knochen[4]{};
    float gewicht[4]{};
    int n = 0;
    for (int k = 0; k < 4; ++k) {
        if (e.gewicht[k] <= 0.0F) { continue; }
        const int q = e.knochen[k];
        const int z = q >= 0 && static_cast<std::size_t>(q) < weg.size() ? weg[static_cast<std::size_t>(q)] : 0;
        int s = 0;
        while (s < n && knochen[s] != static_cast<std::uint8_t>(z)) { ++s; }
        if (s == n) {
            knochen[n] = static_cast<std::uint8_t>(z);
            ++n;
        }
        gewicht[s] += e.gewicht[k];
    }
    if (n == 0) {   // ohne Gewicht (kommt in Dateien vor): wie gelesen an Knochen 0
        knochen[0] = 0;
    }
    for (int k = 0; k < 4; ++k) {
        e.knochen[k] = knochen[k];
        e.gewicht[k] = gewicht[k];
    }
}

void listeUmlegen(GewichtListe& l, const std::vector<int>& weg) {
    GewichtListe neu;
    for (const KnochenGewicht& kg : l) {
        const int z = kg.knochen >= 0 && static_cast<std::size_t>(kg.knochen) < weg.size() ? weg[static_cast<std::size_t>(kg.knochen)] : 0;
        const auto it = std::find_if(neu.begin(), neu.end(), [z](const KnochenGewicht& x) { return x.knochen == z; });
        if (it != neu.end()) {
            it->gewicht += kg.gewicht;
        } else {
            neu.push_back(KnochenGewicht{z, kg.gewicht});
        }
    }
    l = std::move(neu);
}

void starr(Ecke& e) {
    for (int k = 0; k < 4; ++k) {
        e.knochen[k] = 0;
        e.gewicht[k] = k == 0 ? 1.0F : 0.0F;
    }
}

// Ein freier Name im Ziel: der alte, sonst mit _1, _2 ... (hoechstens 63 Zeichen; Ghoul2 vergleicht ohne gross/klein).
std::string freierName(const std::string& name, const std::set<std::string>& belegt) {
    constexpr std::size_t kMax = 63;
    std::string basis = name.substr(0, kMax);
    if (basis.empty()) { basis = "surface"; }
    if (belegt.count(klein(basis)) == 0U) { return basis; }
    for (int n = 1;; ++n) {
        const std::string zusatz = "_" + std::to_string(n);
        const std::string kandidat = basis.substr(0, kMax - zusatz.size()) + zusatz;
        if (belegt.count(klein(kandidat)) == 0U) { return kandidat; }
    }
}

}  // namespace

FlaechenAblage ablageAus(const Inhalt& in, const std::vector<int>& wahl, const std::vector<std::string>& bilder,
                         const std::vector<std::string>& knochen, const std::vector<int>& knochenEltern, const std::string& quelle) {
    FlaechenAblage a;
    std::vector<int> w;
    for (const int i : wahl) {
        if (i >= 0 && static_cast<std::size_t>(i) < in.teile.size()) { w.push_back(i); }
    }
    std::sort(w.begin(), w.end());
    w.erase(std::unique(w.begin(), w.end()), w.end());
    for (const int i : w) {
        Teil t = in.teile[static_cast<std::size_t>(i)];
        const auto p = std::find(w.begin(), w.end(), t.eltern);
        t.eltern = p != w.end() ? static_cast<int>(p - w.begin()) : -1;
        dateiFelderWeg(t);
        a.teile.push_back(std::move(t));
        a.bilder.push_back(static_cast<std::size_t>(i) < bilder.size() ? bilder[static_cast<std::size_t>(i)] : std::string());
    }
    a.knochen = knochen;
    a.knochenEltern = knochenEltern;
    a.knochenEltern.resize(a.knochen.size(), -1);
    a.quelle = quelle;
    a.stufen = in.stufenZahl;
    return a;
}

EinfuegeFehler einfuegen(Inhalt& ziel, const FlaechenAblage& a, int eltern, const std::vector<std::string>& zielKnochen, const Grenzen& g,
                         EinfuegeBericht& bericht) {
    bericht = EinfuegeBericht{};
    if (a.leer()) { return EinfuegeFehler::AblageLeer; }
    if (ziel.herkunft == Herkunft::Leer && ziel.teile.empty()) { return EinfuegeFehler::ZielLeer; }
    if (eltern >= static_cast<int>(ziel.teile.size()) || eltern < -1) { return EinfuegeFehler::ElternUngueltig; }
    const bool zielMd3 = ziel.herkunft == Herkunft::Md3;
    if (!zielMd3 && zielKnochen.empty() && !a.knochen.empty()) { return EinfuegeFehler::ZielOhneSkelett; }

    // Knochen der Quelle -> Knochen des Ziels: gleicher Name, sonst der naechste Eltern mit Namen im Ziel, sonst Knochen 0
    std::vector<int> weg(a.knochen.size(), 0);
    std::vector<char> ersetzt(a.knochen.size(), 0);
    if (!zielMd3) {
        std::vector<std::string> zk(zielKnochen.size());
        for (std::size_t i = 0; i < zielKnochen.size(); ++i) { zk[i] = klein(zielKnochen[i]); }
        const auto finde = [&](std::size_t q) {
            const auto it = std::find(zk.begin(), zk.end(), klein(a.knochen[q]));
            return it != zk.end() ? static_cast<int>(it - zk.begin()) : -1;
        };
        for (std::size_t q = 0; q < a.knochen.size(); ++q) {
            int z = finde(q);
            if (z < 0) {
                ersetzt[q] = 1;
                int p = a.knochenEltern[q];
                for (std::size_t schritt = 0; p >= 0 && static_cast<std::size_t>(p) < a.knochen.size() && schritt < a.knochen.size(); ++schritt) {
                    z = finde(static_cast<std::size_t>(p));
                    if (z >= 0) { break; }
                    p = a.knochenEltern[static_cast<std::size_t>(p)];
                }
                if (z < 0) { z = 0; }
            }
            weg[q] = z;
        }
    }
    // welche Knochen der Quelle tragen Gewicht (nur die nennt der Bericht)
    std::vector<char> benutzt(a.knochen.size(), 0);
    for (const Teil& t : a.teile) {
        for (const Netz& n : t.stufen) {
            for (const Ecke& e : n.ecken) {
                for (int k = 0; k < 4; ++k) {
                    if (e.gewicht[k] > 0.0F && e.knochen[k] < a.knochen.size()) { benutzt[e.knochen[k]] = 1; }
                }
            }
            for (const GewichtListe& l : n.mal) {
                for (const KnochenGewicht& kg : l) {
                    if (kg.gewicht > 0.0F && kg.knochen >= 0 && static_cast<std::size_t>(kg.knochen) < a.knochen.size()) {
                        benutzt[static_cast<std::size_t>(kg.knochen)] = 1;
                    }
                }
            }
        }
    }
    for (std::size_t q = 0; q < a.knochen.size(); ++q) {
        if (ersetzt[q] != 0 && benutzt[q] != 0) {
            const int z = weg[q];
            bericht.ersatz.emplace_back(a.knochen[q], static_cast<std::size_t>(z) < zielKnochen.size() ? zielKnochen[static_cast<std::size_t>(z)] : std::string());
        }
    }
    bericht.zielOhneSkelett = zielMd3;
    bericht.quelleOhneSkelett = !zielMd3 && a.knochen.empty();

    // Namen und Kennungen des Ziels
    std::set<std::string> belegt;
    for (const Teil& t : ziel.teile) { belegt.insert(klein(t.name)); }
    std::uint32_t kennung = 0;
    for (const Teil& t : ziel.teile) {
        for (const Modifikator& m : t.stapel) { kennung = std::max(kennung, m.kennung); }
    }
    const int stufen = std::max(ziel.stufenZahl, 1);
    bericht.stufenWeg = std::max(a.stufen - stufen, 0);
    bericht.stufenAufgefuellt = std::max(stufen - std::max(a.stufen, 1), 0);

    const int anfang = static_cast<int>(ziel.teile.size());
    std::vector<Teil> neu;
    neu.reserve(a.teile.size());
    for (std::size_t k = 0; k < a.teile.size(); ++k) {
        Teil t = a.teile[k];
        const std::string alt = t.name;
        t.name = freierName(alt, belegt);
        belegt.insert(klein(t.name));
        if (t.name != alt) { bericht.umbenannt.emplace_back(alt, t.name); }
        if (k < a.bilder.size() && !a.bilder[k].empty()) { t.shader = a.bilder[k].substr(0, 63); }
        t.hautBild = t.shader;   // die Anzeige nimmt es statt einer Haut des Ziels (modellFlaechenAbgleichen)
        t.eltern = t.eltern >= 0 ? anfang + t.eltern : eltern;
        t.kinder.clear();
        for (Modifikator& m : t.stapel) { m.kennung = ++kennung; }
        // Detailstufen wie im Ziel: ueberzaehlige fallen weg, fehlende bekommen die groebste der Quelle
        if (t.stufen.empty()) { t.stufen.emplace_back(); }
        if (static_cast<int>(t.stufen.size()) > stufen) { t.stufen.resize(static_cast<std::size_t>(stufen)); }
        while (static_cast<int>(t.stufen.size()) < stufen) { t.stufen.push_back(t.stufen.back()); }
        for (Netz& n : t.stufen) {
            for (Ecke& e : n.ecken) {
                if (zielMd3) {
                    starr(e);
                } else if (a.knochen.empty()) {
                    starr(e);   // die Quelle ist starr (.md3): ganz an Knochen 0 des Ziels
                } else {
                    eckeUmlegen(e, weg);
                }
            }
            for (GewichtListe& l : n.mal) {
                if (zielMd3 || a.knochen.empty()) {
                    l.clear();
                } else {
                    listeUmlegen(l, weg);
                }
            }
            if (zielMd3 || a.knochen.empty()) { n.mal.clear(); }
        }
        neu.push_back(std::move(t));
    }
    for (std::size_t k = 0; k < neu.size(); ++k) {
        const int i = anfang + static_cast<int>(k);
        ziel.teile.push_back(std::move(neu[k]));
        bericht.neu.push_back(i);
    }
    for (const int i : bericht.neu) {
        const int p = ziel.teile[static_cast<std::size_t>(i)].eltern;
        if (p >= 0 && p < static_cast<int>(ziel.teile.size())) { ziel.teile[static_cast<std::size_t>(p)].kinder.push_back(i); }
    }
    // Grenzen des Zielspiels je neuer Flaeche und Stufe (Tags werden nie gezeichnet)
    for (const int i : bericht.neu) {
        const Teil& t = ziel.teile[static_cast<std::size_t>(i)];
        if (t.istTag()) { continue; }
        for (std::size_t s = 0; s < t.stufen.size(); ++s) {
            for (Befund b : pruefeMass(mass(t.stufen[s]), g)) {
                b.flaeche = i;
                b.stufe = static_cast<int>(s);
                b.name = t.name;
                bericht.befunde.push_back(std::move(b));
            }
        }
    }
    return EinfuegeFehler::Keiner;
}

}  // namespace bhed::modell
