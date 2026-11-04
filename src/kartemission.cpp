// kartemission.cpp - rc609 B2: Mission <-> Map ohne Verlust (Regeln in kartemission.h)
#include "bhed/kartemission.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_set>

namespace bhed {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string wert(const MapEntity& e, const char* key) {
    const std::string* s = e.find(key);
    return s != nullptr ? *s : std::string();
}

// Unter welchen Namen die Entity angesprochen wird: targetname (G_Find FOFS(targetname)),
// script_targetname (affect, OJK code/game/g_spawn.cpp:328), NPC_targetname (:332).
std::vector<std::string> namenVon(const MapEntity& e) {
    std::vector<std::string> n;
    for (const char* k : {"targetname", "script_targetname", "NPC_targetname"}) {
        if (const std::string v = wert(e, k); !v.empty()) { n.push_back(klein(v)); }
    }
    return n;
}

// Schluessel, die auf einen targetname zeigen (Felder vom Typ Ziel in fields[],
// OJK code/game/g_spawn.cpp:245-249, :304-306, :333-335).
constexpr const char* kZielSchluessel[] = {"target", "target2", "target3", "target4", "targetJump", "closetarget",
                                           "opentarget", "paintarget", "NPC_target", "NPC_target2", "NPC_target4"};

struct Info {
    std::string klasse;
    std::string name;   // erster Name (klein), leer ohne
    bool hatOrt = false;
    float ort[3] = {0.0F, 0.0F, 0.0F};
    int modell = -1;    // *N
    bool hatGrenzen = false;
    ModellGrenzen grenzen{};
};

int modellNummer(const MapEntity& e) {
    const std::string m = wert(e, "model");
    if (m.size() < 2 || m[0] != '*') { return -1; }
    int n = 0;
    for (std::size_t i = 1; i < m.size(); ++i) {
        if (m[i] < '0' || m[i] > '9') { return -1; }
        n = n * 10 + (m[i] - '0');
        if (n > 1000000) { return -1; }
    }
    return n;
}

Info infoVon(const MapEntity& e, const std::vector<ModellGrenzen>& modelle) {
    Info i;
    i.klasse = klein(e.classname);
    const std::vector<std::string> n = namenVon(e);
    if (!n.empty()) { i.name = n.front(); }
    if (!e.origin.empty()) {
        std::istringstream s(e.origin);
        i.hatOrt = static_cast<bool>(s >> i.ort[0] >> i.ort[1] >> i.ort[2]);
    }
    i.modell = modellNummer(e);
    if (i.modell >= 0 && static_cast<std::size_t>(i.modell) < modelle.size()) {
        i.hatGrenzen = true;
        i.grenzen = modelle[static_cast<std::size_t>(i.modell)];
    }
    return i;
}

bool grenzenGleich(const ModellGrenzen& a, const ModellGrenzen& b) {
    for (std::size_t k = 0; k < 6; ++k) {
        if (std::fabs(a[k] - b[k]) > kOrtToleranz) { return false; }
    }
    return true;
}

bool ortGleich(const Info& a, const Info& b) {
    if (!a.hatOrt || !b.hatOrt) { return false; }
    for (int k = 0; k < 3; ++k) {
        if (std::fabs(a.ort[k] - b.ort[k]) > kOrtToleranz) { return false; }
    }
    return true;
}

float abstand2(const Info& a, const Info& b) {
    if (!a.hatOrt || !b.hatOrt) { return std::numeric_limits<float>::max(); }
    float s = 0.0F;
    for (int k = 0; k < 3; ++k) { s += (a.ort[k] - b.ort[k]) * (a.ort[k] - b.ort[k]); }
    return s;
}

// Regel 1 bis 4 aus kartemission.h. Liefert fuer jeden Index aus `welcheA` den Partner in B (-1: keiner);
// belegtB wird fortgeschrieben.
std::vector<int> zuordnen(const std::vector<Info>& a, const std::vector<int>& welcheA, const std::vector<Info>& b, std::vector<char>& belegtB) {
    std::vector<int> aus(a.size(), -1);
    for (int regel = 1; regel <= 4; ++regel) {
        for (const int ia : welcheA) {
            if (aus[static_cast<std::size_t>(ia)] >= 0) { continue; }
            const Info& x = a[static_cast<std::size_t>(ia)];
            int best = -1;
            float bestAbstand = std::numeric_limits<float>::max();
            for (std::size_t ib = 0; ib < b.size(); ++ib) {
                if (belegtB[ib] != 0) { continue; }
                const Info& y = b[ib];
                if (x.klasse != y.klasse) { continue; }
                bool ok = false;
                switch (regel) {
                case 1: ok = x.klasse == "worldspawn"; break;
                case 2: ok = !x.name.empty() && x.name == y.name; break;
                case 3: ok = x.name.empty() && y.name.empty() && x.hatGrenzen && y.hatGrenzen && grenzenGleich(x.grenzen, y.grenzen); break;
                default: ok = ortGleich(x, y); break;
                }
                if (!ok) { continue; }
                const float d = abstand2(x, y);
                if (best < 0 || d < bestAbstand) {
                    best = static_cast<int>(ib);
                    bestAbstand = d;
                }
                if (regel != 2) { break; }   // nur bei gleichem Namen entscheidet der Ort
            }
            if (best >= 0) {
                aus[static_cast<std::size_t>(ia)] = best;
                belegtB[static_cast<std::size_t>(best)] = 1;
            }
        }
    }
    return aus;
}

std::string anzeige(const MapEntity& e) {
    const std::vector<std::string> n = namenVon(e);
    if (!n.empty()) {
        for (const char* k : {"targetname", "script_targetname", "NPC_targetname"}) {
            if (const std::string v = wert(e, k); !v.empty()) { return e.classname + " \"" + v + "\""; }
        }
    }
    if (!e.origin.empty()) { return e.classname + " (" + e.origin + ")"; }
    return e.classname;
}

bool istModellSchluessel(const std::string& k, const std::string& v) { return klein(k) == "model" && !v.empty() && v[0] == '*'; }

// Die Schluessel, die die Mission gegenueber `alt` geaendert hat, auf `ziel` legen ("model" *N bleibt).
void aenderungenAuflegen(const MapEntity& alt, const MapEntity& mission, MapEntity& ziel) {
    std::vector<std::string> schluessel;
    std::set<std::string> gesehen;
    for (const auto* liste : {&alt.keys, &mission.keys}) {
        for (const auto& [k, v] : *liste) {
            if (gesehen.insert(klein(k)).second) { schluessel.push_back(k); }
        }
    }
    for (const std::string& k : schluessel) {
        const std::string* a = alt.find(k);
        const std::string* m = mission.find(k);
        const std::string av = a != nullptr ? *a : std::string();
        const std::string mv = m != nullptr ? *m : std::string();
        if (av == mv || istModellSchluessel(k, av) || istModellSchluessel(k, mv)) { continue; }
        setKey(ziel, k, mv);
    }
}

// Alle Schluessel einer eigenen Entity auf die gleiche der neuen Karte ("model" *N bleibt).
void schluesselAuflegen(const MapEntity& mission, MapEntity& ziel) {
    for (const auto& [k, v] : mission.keys) {
        const std::string* z = ziel.find(k);
        if (istModellSchluessel(k, v) || (z != nullptr && istModellSchluessel(k, *z))) { continue; }
        if (z == nullptr || *z != v) { setKey(ziel, k, v); }
    }
}

std::string keysSchluessel(const MapEntity& e) {
    std::string s;
    for (const auto& [k, v] : e.keys) {
        s += k;
        s += '\x1f';
        s += v;
        s += '\x1e';
    }
    return s;
}

}  // namespace

std::vector<ModellGrenzen> bspModellGrenzen(const std::string& bsp) {
    std::vector<ModellGrenzen> aus;
    // dheader_t: ident, version, lumps[] je (fileofs, filelen) - LUMP_MODELS = 7
    constexpr std::size_t kLump = 7;
    constexpr std::size_t kKopf = 8;
    constexpr std::size_t kModell = 40;
    if (bsp.size() < kKopf + (kLump + 1) * 8) { return aus; }
    if (std::memcmp(bsp.data(), "RBSP", 4) != 0 && std::memcmp(bsp.data(), "IBSP", 4) != 0) { return aus; }
    std::int32_t ofs = 0;
    std::int32_t len = 0;
    std::memcpy(&ofs, bsp.data() + kKopf + kLump * 8, 4);
    std::memcpy(&len, bsp.data() + kKopf + kLump * 8 + 4, 4);
    if (ofs < 0 || len < 0 || static_cast<std::size_t>(ofs) + static_cast<std::size_t>(len) > bsp.size()) { return aus; }
    const std::size_t n = static_cast<std::size_t>(len) / kModell;
    aus.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        std::memcpy(aus[i].data(), bsp.data() + static_cast<std::size_t>(ofs) + i * kModell, 6 * sizeof(float));
    }
    return aus;
}

void skriptVerweiseSammeln(const Script& sc, const std::string& skript, std::vector<SkriptVerweis>& aus) {
    struct Gang {
        static void nodes(const std::vector<Node>& ns, const std::string& skript, std::vector<SkriptVerweis>& aus) {
            for (const Node& n : ns) {
                if (n.kind != Node::Kind::Command) { continue; }
                const std::string b = klein(n.name);
                if ((b == "affect" || b == "use" || b == "kill" || b == "remove") && !n.args.empty() && n.args[0].kind == Arg::Kind::String &&
                    !n.args[0].text.empty()) {
                    aus.push_back({n.args[0].text, skript, b});
                }
                nodes(n.children, skript, aus);
            }
        }
    };
    Gang::nodes(sc.nodes, skript, aus);
}

bool KarteMissionErgebnis::gebrochen() const {
    return std::any_of(befunde.begin(), befunde.end(), [](const KarteMissionBefund& b) { return b.art != KarteMissionBefund::Art::ModellAngepasst; });
}

KarteMissionErgebnis karteMissionAbgleich(const std::vector<MapEntity>& alt, const std::vector<ModellGrenzen>& altModelle,
                                          const std::vector<MapEntity>& mission, const std::vector<MapEntity>& neu,
                                          const std::vector<ModellGrenzen>& neuModelle, const std::vector<SkriptVerweis>& skriptVerweise) {
    KarteMissionErgebnis erg;
    std::vector<Info> iAlt;
    std::vector<Info> iMission;
    std::vector<Info> iNeu;
    for (const MapEntity& e : alt) { iAlt.push_back(infoVon(e, altModelle)); }
    for (const MapEntity& e : mission) { iMission.push_back(infoVon(e, altModelle)); }   // *N der Mission: alte Nummern
    for (const MapEntity& e : neu) { iNeu.push_back(infoVon(e, neuModelle)); }

    // alt -> neu (was der Map-Tab daraus gemacht hat)
    std::vector<char> neuBelegt(neu.size(), 0);
    std::vector<int> alleAlt(alt.size());
    for (std::size_t i = 0; i < alt.size(); ++i) { alleAlt[i] = static_cast<int>(i); }
    const std::vector<int> altZuNeu = zuordnen(iAlt, alleAlt, iNeu, neuBelegt);

    // mission -> alt: zuerst Schluessel fuer Schluessel gleich, dann nach der Regel
    std::vector<int> missionZuAlt(mission.size(), -1);
    std::vector<char> altBelegt(alt.size(), 0);
    {
        std::map<std::string, std::vector<int>> nachKeys;
        for (std::size_t i = alt.size(); i-- > 0;) { nachKeys[keysSchluessel(alt[i])].push_back(static_cast<int>(i)); }
        for (std::size_t j = 0; j < mission.size(); ++j) {
            const auto it = nachKeys.find(keysSchluessel(mission[j]));
            if (it == nachKeys.end() || it->second.empty()) { continue; }
            missionZuAlt[j] = it->second.back();
            altBelegt[static_cast<std::size_t>(it->second.back())] = 1;
            it->second.pop_back();
        }
    }
    std::vector<char> genau(mission.size(), 0);
    std::vector<int> rest;
    for (std::size_t j = 0; j < mission.size(); ++j) {
        if (missionZuAlt[j] >= 0) {
            genau[j] = 1;
        } else {
            rest.push_back(static_cast<int>(j));
        }
    }
    {
        const std::vector<int> z = zuordnen(iMission, rest, iAlt, altBelegt);
        for (const int j : rest) { missionZuAlt[static_cast<std::size_t>(j)] = z[static_cast<std::size_t>(j)]; }
    }

    std::vector<MapEntity> erg0 = neu;
    std::vector<char> weg(neu.size(), 0);
    std::vector<int> missionZuErg0(mission.size(), -1);

    // Von der Mission geloescht
    for (std::size_t i = 0; i < alt.size(); ++i) {
        if (altBelegt[i] != 0) { continue; }
        if (const int n = altZuNeu[i]; n >= 0) {
            weg[static_cast<std::size_t>(n)] = 1;
            ++erg.entfernt;
        }
    }
    const auto modellPruefen = [&](const MapEntity& e) {
        const int k = modellNummer(e);
        if (k < 0) { return; }
        KarteMissionBefund b;
        b.was = "*" + std::to_string(k);
        b.wo = anzeige(e);
        if (static_cast<std::size_t>(k) >= neuModelle.size()) {
            b.art = KarteMissionBefund::Art::ModellFehlt;
            erg.befunde.push_back(b);
        } else if (static_cast<std::size_t>(k) < altModelle.size() &&
                   !grenzenGleich(altModelle[static_cast<std::size_t>(k)], neuModelle[static_cast<std::size_t>(k)])) {
            b.art = KarteMissionBefund::Art::ModellVerschoben;
            erg.befunde.push_back(b);
        }
    };

    // Eigene der Mission: Kandidaten sind die Entities der neuen Karte, die keiner alten entsprechen
    std::vector<int> eigene;
    for (std::size_t j = 0; j < mission.size(); ++j) {
        if (missionZuAlt[j] < 0) { eigene.push_back(static_cast<int>(j)); }
    }
    // Regel 3 gegen die neue Karte: *N der Mission mit den alten Grenzen
    const std::vector<int> eigeneZuNeu = zuordnen(iMission, eigene, iNeu, neuBelegt);

    for (std::size_t j = 0; j < mission.size(); ++j) {
        const int i = missionZuAlt[j];
        if (i >= 0) {
            const int n = altZuNeu[static_cast<std::size_t>(i)];
            if (genau[j] != 0) {
                if (n >= 0) {
                    missionZuErg0[j] = n;
                    ++erg.unveraendert;
                }
                continue;
            }
            if (n >= 0) {
                aenderungenAuflegen(alt[static_cast<std::size_t>(i)], mission[j], erg0[static_cast<std::size_t>(n)]);
                missionZuErg0[j] = n;
                ++erg.geaendert;
                const int ka = modellNummer(alt[static_cast<std::size_t>(i)]);
                const int kn = modellNummer(neu[static_cast<std::size_t>(n)]);
                if (ka >= 0 && kn >= 0 && ka != kn) {
                    erg.befunde.push_back({KarteMissionBefund::Art::ModellAngepasst, "*" + std::to_string(ka), anzeige(mission[j]), "*" + std::to_string(kn)});
                }
            } else {
                missionZuErg0[j] = static_cast<int>(erg0.size());
                erg0.push_back(mission[j]);
                weg.push_back(0);
                erg.befunde.push_back({KarteMissionBefund::Art::EntityNichtInKarte, anzeige(mission[j]), anzeige(mission[j]), {}});
                modellPruefen(mission[j]);
            }
            continue;
        }
        if (const int n = eigeneZuNeu[j]; n >= 0) {
            schluesselAuflegen(mission[j], erg0[static_cast<std::size_t>(n)]);
            missionZuErg0[j] = n;
            ++erg.eigeneSchonDa;
        } else {
            missionZuErg0[j] = static_cast<int>(erg0.size());
            erg0.push_back(mission[j]);
            weg.push_back(0);
            ++erg.eigeneAngehaengt;
            modellPruefen(mission[j]);
        }
    }

    // Geloeschte heraus, Nummern nachziehen
    std::vector<int> erg0ZuErg(erg0.size(), -1);
    for (std::size_t k = 0; k < erg0.size(); ++k) {
        if (weg[k] != 0) { continue; }
        erg0ZuErg[k] = static_cast<int>(erg.entities.size());
        erg.entities.push_back(std::move(erg0[k]));
    }
    erg.neuVonMission.resize(mission.size(), -1);
    for (std::size_t j = 0; j < mission.size(); ++j) {
        if (missionZuErg0[j] >= 0) { erg.neuVonMission[j] = erg0ZuErg[static_cast<std::size_t>(missionZuErg0[j])]; }
    }

    // Bezuege: was vorher auf eine Entity zeigte und jetzt auf keine
    std::unordered_set<std::string> vorher;
    std::unordered_set<std::string> nachher;
    for (const MapEntity& e : mission) {
        for (const std::string& n : namenVon(e)) { vorher.insert(n); }
    }
    for (const MapEntity& e : erg.entities) {
        for (const std::string& n : namenVon(e)) { nachher.insert(n); }
    }
    std::set<std::tuple<int, std::string, std::string>> schon;
    const auto melde = [&](const std::string& name, const std::string& wo) {
        const std::string k = klein(name);
        if (vorher.count(k) == 0U || nachher.count(k) != 0U) { return; }
        if (!schon.insert({0, k, wo}).second) { return; }
        erg.befunde.push_back({KarteMissionBefund::Art::NameFehlt, name, wo, {}});
    };
    for (const MapEntity& e : erg.entities) {
        for (const char* key : kZielSchluessel) {
            if (const std::string v = wert(e, key); !v.empty()) { melde(v, anzeige(e) + ": " + key); }
        }
    }
    for (const SkriptVerweis& v : skriptVerweise) { melde(v.name, v.skript + ": " + v.befehl); }
    return erg;
}

}  // namespace bhed
