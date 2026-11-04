// enttext.cpp - rc609 W2b: Entities als Text (siehe enttext.h)
#include "bhed/enttext.h"

#include "bhed/regelwerk.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace bhed {
namespace {

bool gleichKlein(const std::string& a, const char* b) {
    std::size_t i = 0;
    for (; i < a.size() && b[i] != '\0'; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return i == a.size() && b[i] == '\0';
}

struct Aenderung {
    std::size_t von = 0;
    std::size_t bis = 0;
    std::string neu;
};

// Zeilenanfang und -ende (Position des '\n' oder Textende) um pos.
std::size_t zeilenAnfang(const std::string& t, std::size_t pos) {
    const std::size_t nl = pos == 0 ? std::string::npos : t.rfind('\n', pos - 1);
    return nl == std::string::npos ? 0 : nl + 1;
}
std::size_t zeilenEnde(const std::string& t, std::size_t pos) {
    const std::size_t nl = t.find('\n', pos);
    return nl == std::string::npos ? t.size() : nl;
}
bool nurLeer(const std::string& t, std::size_t von, std::size_t bis) {
    for (std::size_t i = von; i < bis; ++i) {
        if (t[i] != ' ' && t[i] != '\t' && t[i] != '\r') { return false; }
    }
    return true;
}

// Ein Token im Stil des alten: in Anfuehrungszeichen, wenn das alte welche hatte
// oder der Wert sonst nicht als ein Wort gelesen wuerde (COM_Parse).
std::string token(const std::string& wert, bool hatteAnf) {
    bool wort = !hatteAnf && !wert.empty() && wert[0] != '{' && wert[0] != '}' && wert.find("//") == std::string::npos &&
                wert.find("/*") == std::string::npos;
    for (const char c : wert) {
        if (static_cast<signed char>(c) <= 32 || c == '"') { wort = false; }
    }
    return wort ? wert : "\"" + wert + "\"";
}
std::string paarZeile(const std::pair<std::string, std::string>& kv) { return "\"" + kv.first + "\" \"" + kv.second + "\""; }

// Bereich, der beim Entfernen von [von, bis) wegfaellt: die ganze Zeile, wenn
// sonst nichts darauf steht.
std::pair<std::size_t, std::size_t> wegBereich(const std::string& t, std::size_t von, std::size_t bis) {
    const std::size_t za = zeilenAnfang(t, von);
    const std::size_t ze = zeilenEnde(t, bis);
    if (nurLeer(t, za, von) && nurLeer(t, bis, ze)) { return {za, ze < t.size() ? ze + 1 : ze}; }
    std::size_t e = bis;
    while (e < ze && (t[e] == ' ' || t[e] == '\t')) { ++e; }
    return {von, e};
}

// Laengste gemeinsame Teilfolge zweier Folgen ueber gleich(i, j) - die Paare (i, j).
template <class Gleich>
std::vector<std::pair<int, int>> gemeinsam(int n, int m, Gleich gleich) {
    std::vector<std::pair<int, int>> r;
    int a = 0;
    while (a < n && a < m && gleich(a, a)) {
        r.emplace_back(a, a);
        ++a;
    }
    int en = n;
    int em = m;
    std::vector<std::pair<int, int>> ende;
    while (en > a && em > a && gleich(en - 1, em - 1)) {
        ende.emplace_back(en - 1, em - 1);
        --en;
        --em;
    }
    const int kn = en - a;
    const int km = em - a;
    // Grosse Mitten (Tausende geaenderte Entities auf einmal) ohne Tabelle: alles gilt als geaendert.
    if (kn > 0 && km > 0 && static_cast<long long>(kn) * km <= 4000000LL) {
        std::vector<std::uint16_t> l(static_cast<std::size_t>(kn + 1) * static_cast<std::size_t>(km + 1), 0);
        const auto at = [&](int i, int j) -> std::uint16_t& { return l[static_cast<std::size_t>(i) * static_cast<std::size_t>(km + 1) + static_cast<std::size_t>(j)]; };
        for (int i = kn - 1; i >= 0; --i) {
            for (int j = km - 1; j >= 0; --j) {
                at(i, j) = gleich(a + i, a + j) ? static_cast<std::uint16_t>(at(i + 1, j + 1) + 1) : std::max(at(i + 1, j), at(i, j + 1));
            }
        }
        int i = 0;
        int j = 0;
        while (i < kn && j < km) {
            if (gleich(a + i, a + j)) {
                r.emplace_back(a + i, a + j);
                ++i;
                ++j;
            } else if (at(i + 1, j) >= at(i, j + 1)) {
                ++i;
            } else {
                ++j;
            }
        }
    }
    r.insert(r.end(), ende.rbegin(), ende.rend());
    return r;
}

// Ein Block wird zu neu: nur die Paare, die sich unterscheiden.
void blockAngleichen(const std::string& t, const MapEntity& alt, const EntBlockSpanne& sp, const MapEntity& neu, std::vector<Aenderung>& aus) {
    const int n = static_cast<int>(alt.keys.size());
    const int m = static_cast<int>(neu.keys.size());
    const auto paare = gemeinsam(n, m, [&](int i, int j) { return alt.keys[static_cast<std::size_t>(i)].first == neu.keys[static_cast<std::size_t>(j)].first; });
    // Einrueckung der Paare (die des ersten Paars; sonst keine wie writeEntities).
    std::string einr;
    if (!sp.paare.empty()) {
        const std::size_t za = zeilenAnfang(t, sp.paare.front().kAnf);
        if (nurLeer(t, za, sp.paare.front().kAnf)) { einr = t.substr(za, sp.paare.front().kAnf - za); }
    }
    const std::size_t klammerZu = sp.ende - 1;   // die '}'
    // Neue Paare hinter dem Anker einfuegen: am Ende seiner Zeile als eigene Zeile,
    // steht die '}' auf derselben Zeile, direkt dahinter.
    const auto einfuegen = [&](std::size_t ankerEnde, const std::string& zeilen, bool mitUmbruch) {
        const std::size_t ze = zeilenEnde(t, ankerEnde);
        if (mitUmbruch && ze < klammerZu) {
            aus.push_back({ze, ze, zeilen});
        } else {
            std::string flach;
            for (std::size_t p = 0; p < zeilen.size(); ++p) {
                if (zeilen[p] == '\n') {
                    flach += ' ';
                    while (p + 1 < zeilen.size() && (zeilen[p + 1] == ' ' || zeilen[p + 1] == '\t')) { ++p; }
                } else {
                    flach += zeilen[p];
                }
            }
            aus.push_back({ankerEnde, ankerEnde, flach});
        }
    };
    int ai = 0;
    int nj = 0;
    std::size_t anker = sp.anf + 1;   // hinter '{'
    std::string offen;                // gesammelte neue Zeilen hinter dem Anker
    const auto loswerden = [&]() {
        if (!offen.empty()) {
            einfuegen(anker, offen, true);
            offen.clear();
        }
    };
    for (std::size_t q = 0; q <= paare.size(); ++q) {
        const int bisA = q < paare.size() ? paare[q].first : n;
        const int bisN = q < paare.size() ? paare[q].second : m;
        for (; ai < bisA; ++ai) {   // entfernt
            const EntPaarSpanne& ps = sp.paare[static_cast<std::size_t>(ai)];
            const auto [von, bis] = wegBereich(t, ps.kAnf, ps.wEnde);
            aus.push_back({von, bis, std::string()});
        }
        for (; nj < bisN; ++nj) { offen += "\n" + einr + paarZeile(neu.keys[static_cast<std::size_t>(nj)]); }
        loswerden();
        if (q == paare.size()) { break; }
        const EntPaarSpanne& ps = sp.paare[static_cast<std::size_t>(paare[q].first)];
        const std::string& wAlt = alt.keys[static_cast<std::size_t>(paare[q].first)].second;
        const std::string& wNeu = neu.keys[static_cast<std::size_t>(paare[q].second)].second;
        if (wAlt != wNeu) { aus.push_back({ps.wAnf, ps.wEnde, token(wNeu, t[ps.wAnf] == '"')}); }
        anker = ps.wEnde;
        ++ai;
        ++nj;
    }
}

}  // namespace

std::string latin1NachUtf8(const std::string& bytes) {
    std::string r;
    r.reserve(bytes.size() + bytes.size() / 8);
    for (const char ch : bytes) {
        const auto c = static_cast<unsigned char>(ch);
        if (c < 0x80) {
            r += ch;
        } else {
            r += static_cast<char>(0xC0 | (c >> 6));
            r += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return r;
}

bool utf8NachLatin1(const std::string& utf8, std::string& out, std::size_t* stelle, std::string* zeichen) {
    out.clear();
    out.reserve(utf8.size());
    for (std::size_t i = 0; i < utf8.size();) {
        const auto c = static_cast<unsigned char>(utf8[i]);
        if (c < 0x80) {
            out += static_cast<char>(c);
            ++i;
            continue;
        }
        // Laenge der Folge
        std::size_t len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        if (i + len > utf8.size()) { len = utf8.size() - i; }
        if (c == 0xC2 || c == 0xC3) {
            if (i + 1 < utf8.size() && (static_cast<unsigned char>(utf8[i + 1]) & 0xC0) == 0x80) {
                out += static_cast<char>(((c & 0x03) << 6) | (static_cast<unsigned char>(utf8[i + 1]) & 0x3F));
                i += 2;
                continue;
            }
        }
        if (stelle != nullptr) { *stelle = i; }
        if (zeichen != nullptr) { *zeichen = utf8.substr(i, len); }
        return false;
    }
    return true;
}

bool entTextLiveSchluessel(const std::string& key) {
    static const char* const kLive[] = {"origin", "angle", "angles", "modelscale", "modelscale_vec", "light", "_color"};
    return std::any_of(std::begin(kLive), std::end(kLive), [&key](const char* k) { return gleichKlein(key, k); });
}

bool entGleich(const MapEntity& a, const MapEntity& b) { return a.keys == b.keys; }

bool entListeGleich(const std::vector<MapEntity>& a, const std::vector<MapEntity>& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].keys != b[i].keys) { return false; }
    }
    return true;
}

bool entTextNurLive(const MapEntity& alt, const MapEntity& neu) {
    if (alt.keys.size() != neu.keys.size()) { return false; }
    bool anders = false;
    for (std::size_t i = 0; i < alt.keys.size(); ++i) {
        if (alt.keys[i].first != neu.keys[i].first) { return false; }
        if (alt.keys[i].second != neu.keys[i].second) {
            if (!entTextLiveSchluessel(alt.keys[i].first)) { return false; }
            anders = true;
        }
    }
    return anders;
}

std::string entTextAbgleichen(const std::string& text, const std::vector<MapEntity>& neu) {
    std::vector<MapEntity> alt;
    std::vector<EntBlockSpanne> sp;
    if (!parseEntitiesGenau(text, alt, &sp)) { return writeEntities(neu); }
    const int n = static_cast<int>(alt.size());
    const int m = static_cast<int>(neu.size());
    const auto paare = gemeinsam(n, m, [&](int i, int j) { return entGleich(alt[static_cast<std::size_t>(i)], neu[static_cast<std::size_t>(j)]); });
    std::vector<Aenderung> aus;
    // Neue Bloecke hinter das Ende der Zeile des vorigen Blocks (vor dem ersten:
    // an den Anfang seiner Zeile; ohne Bloecke ans Ende des Texts).
    const auto blockEinfuegen = [&](int vorigerAlt, const std::string& bloecke) {
        if (vorigerAlt >= 0) {
            const std::size_t ze = zeilenEnde(text, sp[static_cast<std::size_t>(vorigerAlt)].ende);
            if (ze < text.size()) {
                aus.push_back({ze + 1, ze + 1, bloecke});
            } else {
                aus.push_back({ze, ze, "\n" + bloecke});
            }
        } else if (!sp.empty()) {
            const std::size_t za = zeilenAnfang(text, sp.front().anf);
            aus.push_back({za, za, bloecke});
        } else {
            const bool umbruch = !text.empty() && text.back() != '\n';
            aus.push_back({text.size(), text.size(), (umbruch ? "\n" : "") + bloecke});
        }
    };
    int ai = 0;
    int nj = 0;
    int voriger = -1;
    for (std::size_t q = 0; q <= paare.size(); ++q) {
        const int bisA = q < paare.size() ? paare[q].first : n;
        const int bisN = q < paare.size() ? paare[q].second : m;
        // In der Luecke: der Reihe nach geaendert, Rest entfernt bzw. neu.
        std::string neueBloecke;
        while (ai < bisA || nj < bisN) {
            if (ai < bisA && nj < bisN) {
                blockAngleichen(text, alt[static_cast<std::size_t>(ai)], sp[static_cast<std::size_t>(ai)], neu[static_cast<std::size_t>(nj)], aus);
                voriger = ai;
                ++ai;
                ++nj;
            } else if (ai < bisA) {
                const auto [von, bis] = wegBereich(text, sp[static_cast<std::size_t>(ai)].anf, sp[static_cast<std::size_t>(ai)].ende);
                aus.push_back({von, bis, std::string()});
                ++ai;
            } else {
                neueBloecke += writeEntities({neu[static_cast<std::size_t>(nj)]});
                ++nj;
            }
        }
        if (!neueBloecke.empty()) { blockEinfuegen(voriger, neueBloecke); }
        if (q == paare.size()) { break; }
        voriger = paare[q].first;
        ai = paare[q].first + 1;
        nj = paare[q].second + 1;
    }
    // Von hinten nach vorn einsetzen; an derselben Stelle in der Reihenfolge des Anlegens.
    std::stable_sort(aus.begin(), aus.end(), [](const Aenderung& x, const Aenderung& y) { return x.von > y.von; });
    std::string r = text;
    for (std::size_t k = 0; k < aus.size();) {
        std::size_t e = k;
        std::string zusammen;
        while (e < aus.size() && aus[e].von == aus[k].von && aus[e].bis == aus[e].von) {
            zusammen += aus[e].neu;
            ++e;
        }
        if (e > k) {
            r.insert(aus[k].von, zusammen);
            k = e;
            continue;
        }
        r.replace(aus[k].von, aus[k].bis - aus[k].von, aus[k].neu);
        ++k;
    }
    return r;
}

bool entTextPruefen(const std::string& text, std::vector<MapEntity>& out, std::vector<EntBlockSpanne>* spannen, EntTextFehler* fehler) {
    std::vector<EntBlockSpanne> eigene;
    std::vector<EntBlockSpanne>* sp = spannen != nullptr ? spannen : &eigene;
    if (!parseEntitiesGenau(text, out, sp, fehler)) {
        // Der eigentliche Grund ist meist ein vergessenes Anfuehrungszeichen weiter oben.
        if (fehler != nullptr && fehler->zeileMehrzeilig > 0) {
            fehler->art = EntFehler::OffenesAnfuehrungszeichen;
            fehler->zeile = fehler->zeileMehrzeilig;
        }
        return false;
    }
    // Strenger als das Spiel: ein Wert in Anfuehrungszeichen ueber ein Zeilenende liest
    // COM_Parse ohne Fehler (q_shared.cpp:346-364), gemeint ist er fast nie - setKey
    // kann ihn gar nicht erzeugen. Der Editor meldet ihn als offenes Anfuehrungszeichen.
    for (const EntBlockSpanne& b : *sp) {
        for (const EntPaarSpanne& p : b.paare) {
            for (const std::size_t a : {p.kAnf, p.wAnf}) {
                const std::size_t e = a == p.kAnf ? p.kEnde : p.wEnde;
                if (text[a] == '"' && text.find('\n', a) < e) {
                    if (fehler != nullptr) {
                        fehler->art = EntFehler::OffenesAnfuehrungszeichen;
                        fehler->zeile = entTextZeile(text, a) + 1;
                        fehler->token.clear();
                    }
                    return false;
                }
            }
        }
    }
    if (out.empty()) { return true; }
    // SP_worldspawn: G_SpawnString("classname") - der ERSTE Schluessel dieses
    // Namens (JK2: genaue Schreibweise), verglichen mit Q_stricmp.
    const bool genau = regelwerkJediOutcast();
    const std::string* cls = nullptr;
    for (const auto& kv : out.front().keys) {
        if (genau ? kv.first == "classname" : gleichKlein(kv.first, "classname")) {
            cls = &kv.second;
            break;
        }
    }
    if (cls == nullptr || !gleichKlein(*cls, "worldspawn")) {
        if (fehler != nullptr) {
            fehler->art = EntFehler::ErsteNichtWorldspawn;
            fehler->zeile = !sp->empty() ? entTextZeile(text, sp->front().anf) + 1 : 1;
            fehler->token = cls != nullptr ? *cls : std::string();
        }
        return false;
    }
    return true;
}

int entTextZeile(const std::string& text, std::size_t pos) {
    return static_cast<int>(std::count(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(std::min(pos, text.size())), '\n'));
}

}  // namespace bhed
