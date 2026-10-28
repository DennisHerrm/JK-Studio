// hilfe.cpp - der Helfer: Index, Woerter, Rangfolge (Stufe H1, siehe bhed/hilfe.h)
#include "bhed/hilfe.h"

#include "bhed/karte/befehle.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace bhed::hilfe {
namespace {

#include "hilfe_daten.inc"

const Ort kOrte[] = {
#include "hilfe_orte.inc"
};

// --- Woerter -----------------------------------------------------------------------

// Ein Zeichen aus UTF-8 (ungueltige Bytes zaehlen einzeln).
unsigned zeichen(std::string_view s, std::size_t& i) {
    const auto b = static_cast<unsigned char>(s[i]);
    if (b < 0x80U) {
        ++i;
        return b;
    }
    std::size_t n = 0;
    unsigned cp = 0;
    if ((b & 0xE0U) == 0xC0U) {
        n = 1;
        cp = b & 0x1FU;
    } else if ((b & 0xF0U) == 0xE0U) {
        n = 2;
        cp = b & 0x0FU;
    } else if ((b & 0xF8U) == 0xF0U) {
        n = 3;
        cp = b & 0x07U;
    } else {
        ++i;
        return b;
    }
    if (i + n >= s.size()) {
        ++i;
        return b;
    }
    for (std::size_t k = 1; k <= n; ++k) {
        const auto c = static_cast<unsigned char>(s[i + k]);
        if ((c & 0xC0U) != 0x80U) {
            ++i;
            return b;
        }
        cp = (cp << 6U) | (c & 0x3FU);
    }
    i += n + 1;
    return cp;
}

bool istCjk(unsigned cp) {
    return (cp >= 0x3040U && cp <= 0x30FFU) || (cp >= 0x3400U && cp <= 0x4DBFU) || (cp >= 0x4E00U && cp <= 0x9FFFU) ||
           (cp >= 0xF900U && cp <= 0xFAFFU) || (cp >= 0xFF66U && cp <= 0xFF9FU);
}

// Ein Buchstabe ausserhalb von ASCII und CJK (e, n mit Akzent ...): gehoert zum Wort.
bool istFremdBuchstabe(unsigned cp) { return cp >= 0xC0U && cp < 0x3000U && cp != 0xD7U && cp != 0xF7U; }

void utf8(std::string& o, unsigned cp) {
    if (cp < 0x80U) {
        o += static_cast<char>(cp);
    } else if (cp < 0x800U) {
        o += static_cast<char>(0xC0U | (cp >> 6U));
        o += static_cast<char>(0x80U | (cp & 0x3FU));
    } else if (cp < 0x10000U) {
        o += static_cast<char>(0xE0U | (cp >> 12U));
        o += static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU));
        o += static_cast<char>(0x80U | (cp & 0x3FU));
    } else {
        o += static_cast<char>(0xF0U | (cp >> 18U));
        o += static_cast<char>(0x80U | ((cp >> 12U) & 0x3FU));
        o += static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU));
        o += static_cast<char>(0x80U | (cp & 0x3FU));
    }
}

const std::unordered_set<std::string>& stoppwoerter() {
    static const std::unordered_set<std::string> s = {
        "wie", "ich", "kann", "man", "ein", "eine", "einen", "einem", "einer", "der", "die", "das", "den", "dem", "des", "und",
        "oder", "mit", "im", "in", "zu", "zum", "zur", "auf", "fuer", "von", "vom", "mein", "meine", "meinen", "ist", "es",
        "wo", "was", "wann", "bitte", "mir", "mich", "dass", "nicht", "auch", "so", "da", "an", "am", "aus", "bei", "will",
        "moechte", "geht", "dies", "diese", "finde", "findet", "gibt", "the", "a", "an", "to", "how", "do", "does", "i", "can", "my", "of",
        "for", "with", "is", "what", "where", "when", "please", "me", "it", "and", "or", "on", "at", "want", "into",
        "this", "that", "there", "be", "get", "you", "your", "am", "are", "which", "should", "would", "could"};
    return s;
}

// Grob gestutzt, damit "Kameras", "Kamera", "texture", "textures" gleich zaehlen.
std::string stutze(std::string w) {
    static const char* const endungen[] = {"ungen", "ung", "ern", "ing", "en", "er", "es", "ed", "e", "s"};
    if (w.size() <= 4) { return w; }
    for (const char* e : endungen) {
        const std::size_t n = std::char_traits<char>::length(e);
        if (w.size() >= n + 4 && w.compare(w.size() - n, n, e) == 0) { return w.substr(0, w.size() - n); }
    }
    return w;
}

// Lateinische Woerter (klein, Umlaute gefaltet, gestutzt) und CJK-Laeufe.
struct Zerlegt {
    std::vector<std::string> latein;
    std::vector<std::string> cjk;   // je ein Lauf
    std::string klein;              // der ganze Text klein (fuer CJK-Begriffe)
};

Zerlegt zerlegeText(std::string_view s) {
    Zerlegt z;
    z.klein.reserve(s.size());
    std::string wort;
    std::string lauf;
    const auto wortEnde = [&] {
        if (!wort.empty()) {
            if (stoppwoerter().count(wort) == 0U) { z.latein.push_back(stutze(wort)); }
            wort.clear();
        }
    };
    const auto laufEnde = [&] {
        if (!lauf.empty()) {
            z.cjk.push_back(lauf);
            lauf.clear();
        }
    };
    std::size_t i = 0;
    while (i < s.size()) {
        const unsigned cp = zeichen(s, i);
        if (cp < 0x80U) {
            const char c = static_cast<char>(cp >= 'A' && cp <= 'Z' ? cp + 32U : cp);
            z.klein += c;
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
                laufEnde();
                wort += c;
            } else {
                wortEnde();
                laufEnde();
            }
            continue;
        }
        // Umlaute und sz wie im Handbuch-Index gefaltet.
        const char* falt = nullptr;
        switch (cp) {
            case 0xE4U: case 0xC4U: falt = "ae"; break;
            case 0xF6U: case 0xD6U: falt = "oe"; break;
            case 0xFCU: case 0xDCU: falt = "ue"; break;
            case 0xDFU: falt = "ss"; break;
            default: break;
        }
        if (falt != nullptr) {
            laufEnde();
            wort += falt;
            z.klein += falt;
            continue;
        }
        utf8(z.klein, cp);
        if (istCjk(cp)) {
            wortEnde();
            utf8(lauf, cp);
        } else if (istFremdBuchstabe(cp)) {
            laufEnde();
            utf8(wort, cp);
        } else {
            wortEnde();
            laufEnde();
        }
    }
    wortEnde();
    laufEnde();
    return z;
}

// CJK: Einzel- und Doppelzeichen eines Laufs.
void cjkWoerter(const std::string& lauf, std::vector<std::string>& aus) {
    std::vector<std::string> zs;
    std::size_t i = 0;
    while (i < lauf.size()) {
        const std::size_t a = i;
        (void)zeichen(lauf, i);
        zs.push_back(lauf.substr(a, i - a));
    }
    for (std::size_t k = 0; k < zs.size(); ++k) {
        aus.push_back(zs[k]);
        if (k + 1 < zs.size()) { aus.push_back(zs[k] + zs[k + 1]); }
    }
}

struct Begriff {
    std::string name;   // "@kamera"
    std::vector<std::vector<std::string>> latein;   // je Variante ihre Woerter
    std::vector<std::string> cjk;
};

const std::vector<Begriff>& begriffe() {
    static const std::vector<Begriff> b = [] {
        std::vector<Begriff> v;
        for (const BegriffRoh& r : kBegriffe) {
            Begriff g;
            g.name = std::string("@") + r.name;
            std::string_view w(r.woerter);
            while (!w.empty()) {
                const std::size_t p = w.find('|');
                const std::string_view teil = w.substr(0, p);
                const Zerlegt z = zerlegeText(teil);
                if (!z.cjk.empty()) {
                    for (const std::string& c : z.cjk) { g.cjk.push_back(c); }
                } else if (!z.latein.empty()) {
                    g.latein.push_back(z.latein);
                }
                if (p == std::string_view::npos) { break; }
                w.remove_prefix(p + 1);
            }
            v.push_back(std::move(g));
        }
        return v;
    }();
    return b;
}

// Die Begriffe eines zerlegten Texts.
void begriffeIn(const Zerlegt& z, std::vector<std::string>& aus) {
    std::unordered_set<std::string> menge(z.latein.begin(), z.latein.end());
    for (const Begriff& b : begriffe()) {
        bool da = false;
        for (const auto& variante : b.latein) {
            if (variante.size() == 1) {
                const std::string& w = variante[0];
                if (menge.count(w) != 0U) {
                    da = true;
                } else if (w.size() >= 4) {
                    // Teil eines zusammengesetzten Worts: "Kamerafahrt", "Spielerstart"
                    for (const std::string& t : z.latein) {
                        if (t.size() >= w.size() + 3 && (t.compare(0, w.size(), w) == 0 || t.compare(t.size() - w.size(), w.size(), w) == 0)) {
                            da = true;
                            break;
                        }
                    }
                }
            } else {
                da = std::all_of(variante.begin(), variante.end(), [&](const std::string& w) { return menge.count(w) != 0U; });
            }
            if (da) { break; }
        }
        if (!da) {
            for (const std::string& c : b.cjk) {
                if (z.klein.find(c) != std::string::npos) {
                    da = true;
                    break;
                }
            }
        }
        if (da) { aus.push_back(b.name); }
    }
}

// Markierungen der Handbuchtexte weg (**, `, <kbd>, [[..|..]]).
std::string ohneMarken(const std::string& t) {
    std::string o;
    o.reserve(t.size());
    for (std::size_t i = 0; i < t.size(); ++i) {
        if (t.compare(i, 5, "<kbd>") == 0) {
            i += 4;
            continue;
        }
        if (t.compare(i, 6, "</kbd>") == 0) {
            i += 5;
            continue;
        }
        const char c = t[i];
        if (c == '*' || c == '`' || c == '[' || c == ']') { continue; }
        o += c;
    }
    return o;
}

// Ein Tour-Text mit den Beschriftungen ({{Kennung}}) in der Sprache.
std::string klartext(const std::string& roh, i18n::Language sp) {
    std::string o;
    for (const tour::Stueck& s : tour::zerlege(roh)) {
        if (s.art == tour::Stueck::Art::Kennung) {
            i18n::Str id{};
            o += i18n::strVonName(s.inhalt, &id) ? std::string(i18n::trIn(sp, id)) : s.inhalt;
        } else {
            o += s.inhalt;
        }
    }
    return o;
}

// Eine Beschriftung ohne "##..." und ohne die drei Punkte am Ende.
std::string beschriftung(const char* t) {
    std::string s(t == nullptr ? "" : t);
    if (const std::size_t p = s.find("##"); p != std::string::npos) { s.resize(p); }
    while (!s.empty() && (s.back() == '.' || s.back() == ' ')) { s.pop_back(); }
    std::string o;
    for (const char c : s) {
        if (c != '&') { o += c; }
    }
    return o;
}

std::string kennungLabel(const std::string& name, i18n::Language sp) {
    i18n::Str id{};
    return i18n::strVonName(name, &id) ? beschriftung(i18n::trIn(sp, id)) : std::string();
}

// "kette" (">" getrennt) als Anzeige "File > New" und als englische Woerter.
void kettenText(const std::string& kette, i18n::Language sp, std::string& pfad, std::string& titel, std::string& englisch) {
    std::size_t a = 0;
    while (a <= kette.size()) {
        std::size_t e = kette.find('>', a);
        if (e == std::string::npos) { e = kette.size(); }
        const std::string k = kette.substr(a, e - a);
        const std::string l = kennungLabel(k, sp);
        if (!pfad.empty()) { pfad += " > "; }
        pfad += l;
        titel = l;
        englisch += kennungLabel(k, i18n::Language::English) + " ";
        a = e + 1;
    }
}

std::string ersterSatz(const std::string& t, std::size_t hoechstens = 220) {
    std::string s = t;
    for (const char* ende : {". ", "。", "! ", "? "}) {
        const std::size_t p = s.find(ende);
        if (p != std::string::npos && p > 10) { s.resize(p + std::char_traits<char>::length(ende)); }
    }
    if (s.size() > hoechstens) {
        std::size_t p = hoechstens;
        while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xC0U) == 0x80U) { --p; }
        s = s.substr(0, p) + "...";
    }
    while (!s.empty() && s.back() == ' ') { s.pop_back(); }
    return s;
}

std::string schluessel(const Eintrag& e) {
    if (e.art == Art::Tour) { return "tour:" + e.tour + "@" + std::to_string(e.ab); }
    if (e.art == Art::Seite) {
        const std::size_t r = e.seite.find('#');
        return "seite:" + e.seite.substr(0, r);
    }
    return e.id;
}

}  // namespace

std::vector<std::string> woerter(const std::string& text) {
    const Zerlegt z = zerlegeText(text);
    std::vector<std::string> aus = z.latein;
    for (const std::string& l : z.cjk) { cjkWoerter(l, aus); }
    begriffeIn(z, aus);
    return aus;
}

std::string schrittTitel(const std::string& roh) {
    const std::size_t a = roh.find("**");
    if (a != std::string::npos) {
        const std::size_t e = roh.find("**", a + 2);
        if (e != std::string::npos && e > a + 2) { return roh.substr(a + 2, e - a - 2); }
    }
    return ersterSatz(ohneMarken(roh), 60);
}

// --- Index --------------------------------------------------------------------------

void Index::hinzu(Eintrag e) { eintraege_.push_back(std::move(e)); }

const Eintrag* Index::finde(const std::string& id) const {
    for (const Eintrag& e : eintraege_) {
        if (e.id == id) { return &e; }
    }
    return nullptr;
}

void Index::fertig() {
    // Doppelte Kennungen: die spaeteren ergaenzen nur die Suchwoerter.
    std::vector<Eintrag> neu;
    std::unordered_map<std::string, std::size_t> wo;
    for (Eintrag& e : eintraege_) {
        const auto it = wo.find(e.id);
        if (it != wo.end()) {
            Eintrag& alt = neu[it->second];
            alt.woerter += " " + e.woerter;
            if (alt.bereich.empty()) { alt.bereich = e.bereich; }
            continue;
        }
        wo.emplace(e.id, neu.size());
        neu.push_back(std::move(e));
    }
    eintraege_ = std::move(neu);
    doku_.assign(eintraege_.size(), Doku{});
    df_.clear();
    double summe = 0.0;
    for (std::size_t i = 0; i < eintraege_.size(); ++i) {
        const Eintrag& e = eintraege_[i];
        Doku& d = doku_[i];
        const auto feld = [&d](const std::string& t, double gewicht) {
            for (const std::string& w : woerter(t)) {
                d.tf[w] += gewicht;
                d.laenge += gewicht;
            }
        };
        feld(e.titel, 3.0);
        feld(e.pfad, 1.5);
        feld(e.woerter, 2.0);
        feld(e.text, 1.0);
        for (const auto& [w, n] : d.tf) {
            (void)n;
            df_[w] += 1.0;
        }
        summe += d.laenge;
    }
    mittel_ = eintraege_.empty() ? 1.0 : std::max(1.0, summe / static_cast<double>(eintraege_.size()));
    vokabel_.clear();
    vokabel_.reserve(df_.size());
    for (const auto& [w, n] : df_) {
        (void)n;
        vokabel_.push_back(w);
    }
    std::sort(vokabel_.begin(), vokabel_.end());
}

std::vector<Treffer> Index::frage(const std::string& anfrage, std::size_t hoechstens) const {
    std::vector<Treffer> aus;
    if (eintraege_.empty()) { return aus; }
    // Gewichtete Woerter der Frage: Begriffe zaehlen mehr, unbekannte
    // lateinische Woerter suchen ueber ihre Vorsilbe ("kompil" -> "kompilier").
    std::map<std::string, double> q;
    for (const std::string& w : woerter(anfrage)) {
        const double g = (!w.empty() && w[0] == '@') ? 1.5 : 1.0;
        if (df_.count(w) != 0U) {
            q[w] = std::max(q[w], g);
            continue;
        }
        if (w.size() < 4 || static_cast<unsigned char>(w[0]) >= 0x80U) { continue; }
        auto it = std::lower_bound(vokabel_.begin(), vokabel_.end(), w);
        int n = 0;
        for (; it != vokabel_.end() && it->compare(0, w.size(), w) == 0 && n < 6; ++it, ++n) { q[*it] = std::max(q[*it], 0.6); }
        if (n == 0 && w.size() >= 6) {
            // auch umgekehrt: das Wort beginnt mit einem bekannten ("pinselwerkzeug" -> "pinsel")
            for (std::size_t l = w.size() - 1; l >= 5; --l) {
                const std::string kurz = w.substr(0, l);
                if (df_.count(kurz) != 0U) {
                    q[kurz] = std::max(q[kurz], 0.5);
                    break;
                }
            }
        }
    }
    if (q.empty()) { return aus; }
    const double nAlle = static_cast<double>(eintraege_.size());
    constexpr double k1 = 1.2;
    constexpr double b = 0.75;
    std::vector<Treffer> alle;
    for (std::size_t i = 0; i < eintraege_.size(); ++i) {
        const Doku& d = doku_[i];
        double wert = 0.0;
        for (const auto& [w, g] : q) {
            const auto it = d.tf.find(w);
            if (it == d.tf.end()) { continue; }
            const double df = df_.at(w);
            const double idf = std::log(1.0 + (nAlle - df + 0.5) / (df + 0.5));
            const double tf = it->second;
            wert += g * idf * tf * (k1 + 1.0) / (tf + k1 * (1.0 - b + b * d.laenge / mittel_));
        }
        if (wert <= 0.0) { continue; }
        const Eintrag& e = eintraege_[i];
        if (e.art == Art::Tour) { wert *= 1.15; }
        if (!e.woerter.empty() && e.art != Art::Ort) { wert *= 1.1; }
        alle.push_back({i, wert});
    }
    std::sort(alle.begin(), alle.end(), [](const Treffer& x, const Treffer& y) {
        return x.wert != y.wert ? x.wert > y.wert : x.eintrag < y.eintrag;
    });
    std::set<std::string> gesehen;
    for (const Treffer& t : alle) {
        if (!gesehen.insert(schluessel(eintraege_[t.eintrag])).second) { continue; }
        aus.push_back(t);
        if (aus.size() >= hoechstens) { break; }
    }
    return aus;
}

std::vector<std::string> zielKennungen() {
    std::vector<std::string> v;
    for (const ZielRoh& z : kZiele) { v.emplace_back(z.id); }
    return v;
}

// --- Ortskatalog ---------------------------------------------------------------------

const std::vector<Ort>& orte() {
    static const std::vector<Ort> v = [] {
        std::vector<Ort> o(std::begin(kOrte), std::end(kOrte));
        // Die Menues des Karten-Editors (NetRadiant-Custom, karte_menues.py).
        static std::vector<std::string> texte;   // haelt die Zeichenketten
        std::vector<std::pair<std::string, std::string>> roh;   // ziel, kette
        std::vector<std::string> stapel;
        for (const karte::befehle::MenueEintrag& m : karte::befehle::menues()) {
            using karte::befehle::MenueArt;
            const bool beschriftet = m.str != i18n::Str::Count;
            if (m.art == MenueArt::Menue || m.art == MenueArt::Unter) {
                stapel.push_back(beschriftet ? i18n::strName(m.str) : std::string());
                continue;
            }
            if (m.art == MenueArt::Ende) {
                if (!stapel.empty()) { stapel.pop_back(); }
                continue;
            }
            if (m.art != MenueArt::Punkt && m.art != MenueArt::Haken && m.art != MenueArt::Ruf) { continue; }
            if (!beschriftet || stapel.empty() || std::any_of(stapel.begin(), stapel.end(), [](const std::string& s) { return s.empty(); })) {
                continue;
            }
            std::string kette;
            for (const std::string& s : stapel) { kette += s + ">"; }
            kette += i18n::strName(m.str);
            std::string hinweis = std::string("KeT") + m.befehl;
            i18n::Str h{};
            if (!i18n::strVonName(hinweis, &h)) { hinweis.clear(); }
            roh.emplace_back(kette, hinweis);
        }
        texte.reserve(roh.size() * 3);
        for (const auto& [kette, hinweis] : roh) {
            texte.push_back("menue:" + kette);
            const char* ziel = texte.back().c_str();
            texte.push_back(kette);
            const char* k = texte.back().c_str();
            texte.push_back(hinweis);
            const char* h = texte.back().c_str();
            o.push_back(Ort{"karte", ziel, k, h, "modus:3 < knopf:ViewMapEditor@modebar"});
        }
        return o;
    }();
    return v;
}

// --- Der ganze Index ------------------------------------------------------------------

Index baue(const tour::Touren& touren, const std::vector<tour::Abschnitt>& handbuch, i18n::Language sprache) {
    Index ix;
    // Tutorials: die Tour und jeder Schritt einzeln.
    for (const tour::Tour& t : touren.liste) {
        const std::string bereich = t.id.rfind("tut-karte", 0) == 0 ? "karte" : "skript";
        Eintrag e;
        e.art = Art::Tour;
        e.id = "tour:" + t.id;
        e.titel = klartext(t.titel, sprache);
        e.text = ohneMarken(klartext(t.kurz, sprache));
        e.bereich = bereich;
        e.tour = t.id;
        e.seite = t.seite;
        ix.hinzu(e);
        const std::vector<tour::Schritt>* ss = t.schritte("classic");
        if (ss == nullptr) { continue; }
        for (std::size_t i = 0; i < ss->size(); ++i) {
            const std::string text = klartext((*ss)[i].text, sprache);
            Eintrag s;
            s.art = Art::Tour;
            s.id = "tour:" + t.id + "@" + std::to_string(i);
            s.titel = e.titel + " " + std::to_string(i + 1) + ": " + ohneMarken(schrittTitel(text));
            s.text = ohneMarken(text);
            s.bereich = bereich;
            s.tour = t.id;
            s.ab = static_cast<int>(i);
            s.seite = t.seite;
            ix.hinzu(std::move(s));
        }
    }
    // Handbuch: jede Seite (mit dem ersten Abschnitt) und jeder Abschnitt.
    std::set<std::string> seiten;
    for (const tour::Abschnitt& a : handbuch) {
        const std::string text = ohneMarken(a.text);
        if (seiten.insert(a.seite).second) {
            Eintrag p;
            p.art = Art::Seite;
            p.id = "seite:" + a.seite;
            p.titel = a.titel;
            p.text = text;
            p.seite = a.seite;
            ix.hinzu(std::move(p));
        }
        if (a.anker.empty()) { continue; }
        Eintrag e;
        e.art = Art::Seite;
        e.id = "seite:" + a.seite + "#" + a.anker;
        e.titel = a.ueber.empty() ? a.titel : a.titel + " - " + a.ueber;
        e.text = text;
        e.seite = a.seite + "#" + a.anker;
        ix.hinzu(std::move(e));
    }
    // Orte: Menuepunkte, Modusleiste, Einstellungen, Menues des Karten-Editors.
    for (const Ort& o : orte()) {
        Eintrag e;
        e.art = Art::Ort;
        e.id = std::string("ort:") + o.ziel;
        std::string englisch;
        kettenText(o.kette, sprache, e.pfad, e.titel, englisch);
        if (e.titel.empty()) { continue; }
        if (o.hinweis != nullptr && o.hinweis[0] != '\0') {
            e.text = kennungLabel(o.hinweis, sprache);
            englisch += kennungLabel(o.hinweis, i18n::Language::English);
        }
        e.woerter = englisch;
        e.bereich = o.bereich;
        e.ziel = o.ziel;
        e.voraus = o.voraus;
        ix.hinzu(std::move(e));
    }
    // Haeufige Ziele: Woerter, wie Leute fragen, an den Eintrag haengen.
    std::set<std::string> ids;
    for (const Eintrag& e : ix.eintraege()) { ids.insert(e.id); }
    for (const ZielRoh& z : kZiele) {
        if (ids.count(z.id) == 0U) { continue; }
        Eintrag e;
        e.id = z.id;
        e.woerter = z.woerter;
        e.bereich = z.bereich;
        ix.hinzu(std::move(e));
    }
    ix.fertig();
    return ix;
}

}  // namespace bhed::hilfe
