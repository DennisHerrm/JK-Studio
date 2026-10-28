// textpuffer.cpp - Menue-Editor M1 Punkt 3: Textpuffer, Syntaxfarben,
// Aenderungsrand, Vervollstaendigung.
//
// Eigener Code. Der Aenderungsrand nimmt dasselbe Verfahren wie der Rand im
// Skript-Editor (gui/app.cpp geaendertGegen: laengste gemeinsame Teilfolge),
// hier ueber Zeilen statt ueber Baumknoten.
#include "bhed/menue/textpuffer.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <unordered_map>

namespace bhed::menue {

namespace {

bool wortZeichen(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '@';
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string regexEscape(const std::string& s) {
    static const std::string spezial = R"(\^$.|?*+()[]{})";
    std::string r;
    for (const char c : s) {
        if (spezial.find(c) != std::string::npos) { r += '\\'; }
        r += c;
    }
    return r;
}

}  // namespace

Textpuffer::Textpuffer(std::string text) { setzeText(std::move(text)); }

void Textpuffer::setzeText(std::string text) {
    text_ = std::move(text);
    undo_.clear();
    redo_.clear();
    cursor.assign(1, Cursor{});
    zeilenNeu();
    ++version_;
}

void Textpuffer::zeilenNeu() {
    anfang_.clear();
    anfang_.push_back(0);
    for (std::size_t i = 0; i < text_.size(); ++i) {
        if (text_[i] == '\n') { anfang_.push_back(static_cast<std::uint32_t>(i + 1)); }
    }
}

std::uint32_t Textpuffer::zeilenAnfang(int z) const {
    if (z < 0) { return 0; }
    if (z >= zeilen()) { return static_cast<std::uint32_t>(text_.size()); }
    return anfang_[static_cast<std::size_t>(z)];
}

std::uint32_t Textpuffer::zeilenEnde(int z) const {
    if (z < 0) { return 0; }
    if (z + 1 >= zeilen()) { return static_cast<std::uint32_t>(text_.size()); }
    std::uint32_t e = anfang_[static_cast<std::size_t>(z) + 1] - 1;   // auf dem \n
    if (e > anfang_[static_cast<std::size_t>(z)] && text_[e - 1] == '\r') { --e; }
    return e;
}

std::string_view Textpuffer::zeile(int z) const {
    const std::uint32_t a = zeilenAnfang(z);
    return std::string_view(text_).substr(a, zeilenEnde(z) - a);
}

Pos Textpuffer::pos(std::uint32_t o) const {
    o = std::min<std::uint32_t>(o, static_cast<std::uint32_t>(text_.size()));
    const auto it = std::upper_bound(anfang_.begin(), anfang_.end(), o);
    const int z = static_cast<int>(it - anfang_.begin()) - 1;
    return Pos{z, static_cast<int>(o - anfang_[static_cast<std::size_t>(z)])};
}

std::uint32_t Textpuffer::off(Pos p) const {
    if (p.zeile < 0) { return 0; }
    if (p.zeile >= zeilen()) { return static_cast<std::uint32_t>(text_.size()); }
    const std::uint32_t a = zeilenAnfang(p.zeile);
    const std::uint32_t e = zeilenEnde(p.zeile);
    return std::min<std::uint32_t>(a + static_cast<std::uint32_t>(std::max(0, p.spalte)), e);
}

std::string Textpuffer::zeilenUmbruch() const { return text_.find("\r\n") != std::string::npos ? "\r\n" : "\n"; }

// --- Cursor ------------------------------------------------------------------

void Textpuffer::einCursor(std::uint32_t o, bool auswahlBehalten) {
    o = std::min<std::uint32_t>(o, static_cast<std::uint32_t>(text_.size()));
    const std::uint32_t anker = auswahlBehalten && !cursor.empty() ? cursor.front().anker : o;
    cursor.assign(1, Cursor{o, anker});
}

void Textpuffer::cursorDazu(std::uint32_t o) {
    o = std::min<std::uint32_t>(o, static_cast<std::uint32_t>(text_.size()));
    for (const Cursor& c : cursor) {
        if (c.pos == o) { return; }
    }
    cursor.push_back(Cursor{o, o});
}

void Textpuffer::bewege(int dz, int ds, bool auswahl) {
    for (Cursor& c : cursor) {
        if (!auswahl && c.auswahl() && dz == 0) {
            c.pos = c.anker = (ds < 0) ? c.von() : c.bis();
            continue;
        }
        Pos p = pos(c.pos);
        if (dz != 0) {
            p.zeile = std::clamp(p.zeile + dz, 0, zeilen() - 1);
            c.pos = off(p);
        } else if (ds < 0 && c.pos > 0) {
            c.pos -= (c.pos >= 2 && text_[c.pos - 1] == '\n' && text_[c.pos - 2] == '\r') ? 2U : 1U;
        } else if (ds > 0 && c.pos < text_.size()) {
            c.pos += (text_[c.pos] == '\r' && c.pos + 1 < text_.size() && text_[c.pos + 1] == '\n') ? 2U : 1U;
        }
        if (!auswahl) { c.anker = c.pos; }
    }
}

void Textpuffer::zeilenAnfangEnde(bool ende, bool auswahl) {
    for (Cursor& c : cursor) {
        const int z = pos(c.pos).zeile;
        if (ende) {
            c.pos = zeilenEnde(z);
        } else {
            // wie VS Code: erst zum ersten Nicht-Leerzeichen, dann ganz an den Anfang
            std::uint32_t a = zeilenAnfang(z);
            std::uint32_t erstes = a;
            while (erstes < zeilenEnde(z) && (text_[erstes] == ' ' || text_[erstes] == '\t')) { ++erstes; }
            c.pos = (c.pos == erstes) ? a : erstes;
        }
        if (!auswahl) { c.anker = c.pos; }
    }
}

void Textpuffer::wortSprung(bool vor, bool auswahl) {
    for (Cursor& c : cursor) {
        std::uint32_t p = c.pos;
        if (vor) {
            while (p < text_.size() && !wortZeichen(text_[p])) { ++p; }
            while (p < text_.size() && wortZeichen(text_[p])) { ++p; }
        } else {
            while (p > 0 && !wortZeichen(text_[p - 1])) { --p; }
            while (p > 0 && wortZeichen(text_[p - 1])) { --p; }
        }
        c.pos = p;
        if (!auswahl) { c.anker = c.pos; }
    }
}

void Textpuffer::allesWaehlen() { cursor.assign(1, Cursor{static_cast<std::uint32_t>(text_.size()), 0}); }

Treffer Textpuffer::wortBei(std::uint32_t o) const {
    std::uint32_t a = std::min<std::uint32_t>(o, static_cast<std::uint32_t>(text_.size()));
    std::uint32_t e = a;
    while (a > 0 && wortZeichen(text_[a - 1])) { --a; }
    while (e < text_.size() && wortZeichen(text_[e])) { ++e; }
    return Treffer{a, e};
}

std::string Textpuffer::auswahlText() const {
    std::string s;
    std::vector<Cursor> c = cursor;
    std::sort(c.begin(), c.end(), [](const Cursor& a, const Cursor& b) { return a.von() < b.von(); });
    for (std::size_t i = 0; i < c.size(); ++i) {
        if (!c[i].auswahl()) { continue; }
        if (!s.empty()) { s += zeilenUmbruch(); }
        s += text_.substr(c[i].von(), c[i].bis() - c[i].von());
    }
    return s;
}

bool Textpuffer::naechstesVorkommenDazu() {
    Cursor& letzter = cursor.back();
    if (!letzter.auswahl()) {
        const Treffer w = wortBei(letzter.pos);
        if (w.anfang == w.ende) { return false; }
        letzter.anker = w.anfang;
        letzter.pos = w.ende;
        return true;
    }
    const std::string such = text_.substr(letzter.von(), letzter.bis() - letzter.von());
    std::size_t p = text_.find(such, letzter.bis());
    if (p == std::string::npos) { p = text_.find(such); }
    while (p != std::string::npos) {
        bool schon = false;
        for (const Cursor& c : cursor) {
            if (c.von() == p) { schon = true; }
        }
        if (!schon) {
            cursor.push_back(Cursor{static_cast<std::uint32_t>(p + such.size()), static_cast<std::uint32_t>(p)});
            return true;
        }
        p = text_.find(such, p + 1);
        if (p == std::string::npos) { return false; }
    }
    return false;
}

void Textpuffer::alleVorkommen() {
    const Cursor c = cursor.back();
    Treffer w{c.von(), c.bis()};
    if (!c.auswahl()) { w = wortBei(c.pos); }
    if (w.anfang == w.ende) { return; }
    SuchOptionen o;
    o.gross = true;
    o.wort = !c.auswahl();
    const std::vector<Treffer> t = suche(text_.substr(w.anfang, w.ende - w.anfang), o);
    if (t.empty()) { return; }
    cursor.clear();
    for (const Treffer& x : t) { cursor.push_back(Cursor{x.ende, x.anfang}); }
}

// --- Aendern -----------------------------------------------------------------

void Textpuffer::anwenden(std::vector<Ersetzung> e, bool tippen) {
    if (e.empty()) { return; }
    std::sort(e.begin(), e.end(), [](const Ersetzung& a, const Ersetzung& b) { return a.anfang < b.anfang; });
    Schritt s;
    s.cursorVor = cursor;
    s.tippen = tippen;
    // Rueckweg: die Spannen im neuen Text
    std::int64_t versatz = 0;
    for (const Ersetzung& x : e) {
        Ersetzung z;
        z.anfang = static_cast<std::uint32_t>(static_cast<std::int64_t>(x.anfang) + versatz);
        z.ende = static_cast<std::uint32_t>(z.anfang + x.neu.size());
        z.neu = text_.substr(x.anfang, x.ende - x.anfang);
        s.zurueck.push_back(z);
        versatz += static_cast<std::int64_t>(x.neu.size()) - static_cast<std::int64_t>(x.ende - x.anfang);
    }
    // Cursor verschieben: jeder Cursor nach einer Ersetzung wandert mit
    auto neuePos = [&](std::uint32_t p) {
        std::int64_t v = 0;
        for (const Ersetzung& x : e) {
            if (p >= x.ende) {
                v += static_cast<std::int64_t>(x.neu.size()) - static_cast<std::int64_t>(x.ende - x.anfang);
            } else if (p > x.anfang) {
                return static_cast<std::uint32_t>(static_cast<std::int64_t>(x.anfang) + v + static_cast<std::int64_t>(x.neu.size()));
            }
        }
        return static_cast<std::uint32_t>(static_cast<std::int64_t>(p) + v);
    };
    for (Cursor& c : cursor) {
        c.pos = neuePos(c.pos);
        c.anker = neuePos(c.anker);
    }
    for (auto it = e.rbegin(); it != e.rend(); ++it) { text_.replace(it->anfang, it->ende - it->anfang, it->neu); }
    s.hin = std::move(e);
    s.cursorNach = cursor;
    // Tippen zusammenfassen: ein Zeichen nach dem anderen ist ein Schritt
    if (tippen && !undo_.empty() && undo_.back().tippen && undo_.back().cursorNach.size() == s.cursorVor.size() &&
        s.hin.size() == 1 && undo_.back().hin.size() == 1 && s.hin[0].anfang == s.hin[0].ende &&
        undo_.back().zurueck[0].ende == s.hin[0].anfang && s.hin[0].neu.find('\n') == std::string::npos) {
        Schritt& v = undo_.back();
        v.hin[0].neu += s.hin[0].neu;
        v.zurueck[0].ende += static_cast<std::uint32_t>(s.hin[0].neu.size());
        v.cursorNach = s.cursorNach;
    } else {
        undo_.push_back(std::move(s));
        if (undo_.size() > 1000) { undo_.erase(undo_.begin()); }
    }
    redo_.clear();
    zeilenNeu();
    ++version_;
}

void Textpuffer::ersetze(std::uint32_t a, std::uint32_t e, const std::string& neu) {
    anwenden({Ersetzung{a, e, neu}}, false);
}

void Textpuffer::ersetzeViele(std::vector<Ersetzung> e) { anwenden(std::move(e), false); }

void Textpuffer::tippe(const std::string& s) {
    std::vector<Ersetzung> e;
    for (const Cursor& c : cursor) { e.push_back(Ersetzung{c.von(), c.bis(), s}); }
    anwenden(std::move(e), true);
    for (Cursor& c : cursor) { c.anker = c.pos; }
}

void Textpuffer::loescheZurueck() {
    std::vector<Ersetzung> e;
    for (const Cursor& c : cursor) {
        if (c.auswahl()) {
            e.push_back(Ersetzung{c.von(), c.bis(), ""});
        } else if (c.pos > 0) {
            const std::uint32_t n = (c.pos >= 2 && text_[c.pos - 1] == '\n' && text_[c.pos - 2] == '\r') ? 2U : 1U;
            e.push_back(Ersetzung{c.pos - n, c.pos, ""});
        }
    }
    anwenden(std::move(e), false);
}

void Textpuffer::loescheVor() {
    std::vector<Ersetzung> e;
    for (const Cursor& c : cursor) {
        if (c.auswahl()) {
            e.push_back(Ersetzung{c.von(), c.bis(), ""});
        } else if (c.pos < text_.size()) {
            const std::uint32_t n = (text_[c.pos] == '\r' && c.pos + 1 < text_.size() && text_[c.pos + 1] == '\n') ? 2U : 1U;
            e.push_back(Ersetzung{c.pos, c.pos + n, ""});
        }
    }
    anwenden(std::move(e), false);
}

void Textpuffer::neueZeile() {
    std::vector<Ersetzung> e;
    for (const Cursor& c : cursor) {
        const int z = pos(c.von()).zeile;
        std::string einr;
        for (const char ch : zeile(z)) {
            if (ch != ' ' && ch != '\t') { break; }
            einr += ch;
        }
        // nach einer { eine Stufe tiefer
        std::uint32_t p = c.von();
        while (p > zeilenAnfang(z) && (text_[p - 1] == ' ' || text_[p - 1] == '\t')) { --p; }
        if (p > 0 && text_[p - 1] == '{') { einr += '\t'; }
        e.push_back(Ersetzung{c.von(), c.bis(), zeilenUmbruch() + einr});
    }
    anwenden(std::move(e), false);
    for (Cursor& c : cursor) { c.anker = c.pos; }
}

void Textpuffer::einruecken(bool aus) {
    std::set<int> zs;
    for (const Cursor& c : cursor) {
        const int a = pos(c.von()).zeile;
        int b = pos(c.bis()).zeile;
        if (b > a && pos(c.bis()).spalte == 0) { --b; }
        for (int z = a; z <= b; ++z) { zs.insert(z); }
    }
    std::vector<Ersetzung> e;
    for (const int z : zs) {
        const std::uint32_t a = zeilenAnfang(z);
        if (!aus) {
            e.push_back(Ersetzung{a, a, "\t"});
        } else if (a < text_.size() && text_[a] == '\t') {
            e.push_back(Ersetzung{a, a + 1, ""});
        } else {
            std::uint32_t n = 0;
            while (n < 4 && a + n < text_.size() && text_[a + n] == ' ') { ++n; }
            if (n > 0) { e.push_back(Ersetzung{a, a + n, ""}); }
        }
    }
    anwenden(std::move(e), false);
}

void Textpuffer::kommentar() {
    std::set<int> zs;
    for (const Cursor& c : cursor) {
        for (int z = pos(c.von()).zeile; z <= pos(c.bis()).zeile; ++z) { zs.insert(z); }
    }
    bool alle = true;
    for (const int z : zs) {
        const std::string_view l = zeile(z);
        const std::size_t p = l.find_first_not_of(" \t");
        if (p != std::string_view::npos && l.substr(p, 2) != "//") { alle = false; }
    }
    std::vector<Ersetzung> e;
    for (const int z : zs) {
        const std::string_view l = zeile(z);
        const std::size_t p = l.find_first_not_of(" \t");
        if (p == std::string_view::npos) { continue; }
        const std::uint32_t o = zeilenAnfang(z) + static_cast<std::uint32_t>(p);
        if (alle) { e.push_back(Ersetzung{o, o + 2, ""}); }
        else { e.push_back(Ersetzung{o, o, "//"}); }
    }
    anwenden(std::move(e), false);
}

bool Textpuffer::rueckgaengig() {
    if (undo_.empty()) { return false; }
    Schritt s = std::move(undo_.back());
    undo_.pop_back();
    std::vector<Ersetzung> z = s.zurueck;
    std::sort(z.begin(), z.end(), [](const Ersetzung& a, const Ersetzung& b) { return a.anfang > b.anfang; });
    for (const Ersetzung& x : z) { text_.replace(x.anfang, x.ende - x.anfang, x.neu); }
    cursor = s.cursorVor;
    redo_.push_back(std::move(s));
    zeilenNeu();
    ++version_;
    return true;
}

bool Textpuffer::wiederholen() {
    if (redo_.empty()) { return false; }
    Schritt s = std::move(redo_.back());
    redo_.pop_back();
    std::vector<Ersetzung> h = s.hin;
    std::sort(h.begin(), h.end(), [](const Ersetzung& a, const Ersetzung& b) { return a.anfang > b.anfang; });
    for (const Ersetzung& x : h) { text_.replace(x.anfang, x.ende - x.anfang, x.neu); }
    cursor = s.cursorNach;
    undo_.push_back(std::move(s));
    zeilenNeu();
    ++version_;
    return true;
}

// --- Suchen ------------------------------------------------------------------

std::vector<Treffer> Textpuffer::suche(const std::string& muster, const SuchOptionen& o, std::string* fehler) const {
    std::vector<Treffer> t;
    if (fehler != nullptr) { fehler->clear(); }
    if (muster.empty()) { return t; }
    std::string m = o.regex ? muster : regexEscape(muster);
    if (o.wort) { m = "\\b(?:" + m + ")\\b"; }
    try {
        auto f = std::regex::ECMAScript;
        if (!o.gross) { f |= std::regex::icase; }
        const std::regex re(m, f);
        for (auto it = std::sregex_iterator(text_.begin(), text_.end(), re); it != std::sregex_iterator(); ++it) {
            const auto a = static_cast<std::uint32_t>(it->position(0));
            const auto l = static_cast<std::uint32_t>(it->length(0));
            if (l == 0) { continue; }
            t.push_back(Treffer{a, a + l});
            if (t.size() > 100000) { break; }
        }
    } catch (const std::regex_error& e) {
        if (fehler != nullptr) { *fehler = e.what(); }
        t.clear();
    }
    return t;
}

int Textpuffer::ersetzeAlle(const std::string& muster, const std::string& ersatz, const SuchOptionen& o) {
    const std::vector<Treffer> t = suche(muster, o);
    if (t.empty()) { return 0; }
    std::vector<Ersetzung> e;
    std::string m = o.regex ? muster : regexEscape(muster);
    if (o.wort) { m = "\\b(?:" + m + ")\\b"; }
    auto f = std::regex::ECMAScript;
    if (!o.gross) { f |= std::regex::icase; }
    const std::regex re(m, f);
    for (const Treffer& x : t) {
        const std::string stueck = text_.substr(x.anfang, x.ende - x.anfang);
        const std::string neu = o.regex ? std::regex_replace(stueck, re, ersatz, std::regex_constants::format_first_only) : ersatz;
        e.push_back(Ersetzung{x.anfang, x.ende, neu});
    }
    anwenden(std::move(e), false);
    return static_cast<int>(t.size());
}

bool Textpuffer::ersetzeEinen(const std::string& muster, const std::string& ersatz, const SuchOptionen& o) {
    const std::vector<Treffer> t = suche(muster, o);
    if (t.empty()) { return false; }
    const Cursor c = cursor.front();
    for (const Treffer& x : t) {
        if (x.anfang == c.von() && x.ende == c.bis()) {
            std::string neu = ersatz;
            if (o.regex) {
                std::string m = muster;
                if (o.wort) { m = "\\b(?:" + m + ")\\b"; }
                auto f = std::regex::ECMAScript;
                if (!o.gross) { f |= std::regex::icase; }
                neu = std::regex_replace(text_.substr(x.anfang, x.ende - x.anfang), std::regex(m, f), ersatz,
                                         std::regex_constants::format_first_only);
            }
            ersetze(x.anfang, x.ende, neu);
            break;
        }
    }
    // den naechsten Treffer waehlen
    const std::vector<Treffer> t2 = suche(muster, o);
    for (const Treffer& x : t2) {
        if (x.anfang >= cursor.front().pos) {
            cursor.assign(1, Cursor{x.ende, x.anfang});
            return true;
        }
    }
    if (!t2.empty()) { cursor.assign(1, Cursor{t2.front().ende, t2.front().anfang}); }
    return true;
}

// --- Struktur ----------------------------------------------------------------

namespace {
// Ueberspringt Kommentare und Strings: liefert je Byte, ob es "Code" ist.
std::vector<bool> codeMaske(const std::string& t) {
    std::vector<bool> m(t.size(), true);
    std::size_t i = 0;
    while (i < t.size()) {
        if (t[i] == '/' && i + 1 < t.size() && t[i + 1] == '/') {
            while (i < t.size() && t[i] != '\n') { m[i++] = false; }
        } else if (t[i] == '/' && i + 1 < t.size() && t[i + 1] == '*') {
            while (i < t.size() && !(t[i] == '*' && i + 1 < t.size() && t[i + 1] == '/')) { m[i++] = false; }
            if (i < t.size()) { m[i++] = false; }
            if (i < t.size()) { m[i++] = false; }
        } else if (t[i] == '"') {
            m[i++] = false;
            while (i < t.size() && t[i] != '"' && t[i] != '\n') { m[i++] = false; }
            if (i < t.size() && t[i] == '"') { m[i++] = false; }
        } else {
            ++i;
        }
    }
    return m;
}
}  // namespace

std::optional<std::uint32_t> Textpuffer::partnerKlammer(std::uint32_t o) const {
    static const std::string auf = "{([";
    static const std::string zu = "})]";
    const std::vector<bool> m = codeMaske(text_);
    auto pruefe = [&](std::uint32_t p) -> std::optional<std::uint32_t> {
        if (p >= text_.size() || !m[p]) { return std::nullopt; }
        const char c = text_[p];
        std::size_t k = auf.find(c);
        if (k != std::string::npos) {
            int tiefe = 0;
            for (std::size_t i = p; i < text_.size(); ++i) {
                if (!m[i]) { continue; }
                if (text_[i] == auf[k]) { ++tiefe; }
                else if (text_[i] == zu[k] && --tiefe == 0) { return static_cast<std::uint32_t>(i); }
            }
            return std::nullopt;
        }
        k = zu.find(c);
        if (k != std::string::npos) {
            int tiefe = 0;
            for (std::size_t i = p + 1; i-- > 0;) {
                if (!m[i]) { continue; }
                if (text_[i] == zu[k]) { ++tiefe; }
                else if (text_[i] == auf[k] && --tiefe == 0) { return static_cast<std::uint32_t>(i); }
            }
        }
        return std::nullopt;
    };
    if (auto r = pruefe(o)) { return r; }
    if (o > 0) { return pruefe(o - 1); }
    return std::nullopt;
}

std::vector<Falte> Textpuffer::falten() const {
    std::vector<Falte> f;
    const std::vector<bool> m = codeMaske(text_);
    struct Offen {
        std::uint32_t p;
        std::string kopf;
        std::size_t index;
    };
    std::vector<Offen> st;
    for (std::size_t i = 0; i < text_.size(); ++i) {
        if (!m[i]) { continue; }
        if (text_[i] == '{') {
            // das Wort davor (auch auf der Zeile darueber)
            std::size_t e = i;
            while (e > 0 && (static_cast<unsigned char>(text_[e - 1]) <= ' ' || !m[e - 1])) { --e; }
            std::size_t a = e;
            while (a > 0 && wortZeichen(text_[a - 1])) { --a; }
            Falte x;
            x.von = pos(static_cast<std::uint32_t>(i)).zeile;
            x.kopf = text_.substr(a, e - a);
            if (a < e && pos(static_cast<std::uint32_t>(a)).zeile < x.von) { x.von = pos(static_cast<std::uint32_t>(a)).zeile; }
            st.push_back(Offen{static_cast<std::uint32_t>(i), x.kopf, f.size()});
            f.push_back(x);
        } else if (text_[i] == '}' && !st.empty()) {
            Falte& x = f[st.back().index];
            x.bis = pos(static_cast<std::uint32_t>(i)).zeile;
            // name "..." im Block (erste Ebene)
            const std::string block = text_.substr(st.back().p, i - st.back().p);
            const std::string kl = klein(block);
            const std::size_t n = kl.find("name");
            if (n != std::string::npos && (x.kopf == "menuDef" || klein(x.kopf) == "menudef" || klein(x.kopf) == "itemdef")) {
                std::size_t p = n + 4;
                while (p < block.size() && static_cast<unsigned char>(block[p]) <= ' ') { ++p; }
                std::size_t q = p;
                if (q < block.size() && block[q] == '"') {
                    ++p;
                    q = block.find('"', p);
                    if (q == std::string::npos) { q = block.size(); }
                } else {
                    while (q < block.size() && static_cast<unsigned char>(block[q]) > ' ') { ++q; }
                }
                x.name = block.substr(p, q - p);
            }
            st.pop_back();
        }
    }
    f.erase(std::remove_if(f.begin(), f.end(), [](const Falte& x) { return x.bis <= x.von; }), f.end());
    return f;
}

// --- Syntaxfarben ------------------------------------------------------------

const std::vector<std::string>& skriptBefehle() {
    static const std::vector<std::string> v = {
        "activate", "close", "decrementfeeder", "defer", "delay", "disable", "exec", "fadein", "fadeout", "hide",
        "incrementfeeder", "open", "openGoToMenu", "orbit", "play", "playlooped", "playVoice", "rundeferred",
        "scale", "setasset", "setbackground", "setcolor", "setcvar", "setcvartocvar", "setfocus", "setitembackground",
        "setitemcolor", "setitemcolorcvar", "setitemexec", "setitemflag", "setitemrect", "setitemrectcvar",
        "setitemtext", "setplayermodel", "setteamcolor", "show", "showMenu", "stopVoice", "transition", "transition2",
        "transition3", "uiScript"};
    return v;
}

std::vector<Farbstueck> faerbe(const std::string& t, Dialekt d, Spiel s, const std::map<std::string, long>* defines) {
    std::unordered_map<std::string, Farbklasse> worte;
    for (const std::string& k : menuSchluessel(d, s)) { worte[klein(k)] = Farbklasse::MenuWort; }
    for (const std::string& k : itemSchluessel(d, s)) {
        auto it = worte.find(klein(k));
        if (it == worte.end()) { worte[klein(k)] = Farbklasse::ItemWort; }
    }
    for (const std::string& k : skriptBefehle()) {
        if (worte.find(klein(k)) == worte.end()) { worte[klein(k)] = Farbklasse::Skript; }
    }
    for (const char* b : {"menudef", "itemdef", "assetglobaldef", "loadmenu"}) { worte[b] = Farbklasse::Block; }
    std::vector<Farbstueck> v;
    std::size_t i = 0;
    auto push = [&](std::size_t a, std::size_t e, Farbklasse k) {
        v.push_back(Farbstueck{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(e), k});
    };
    bool zeilenAnfang = true;
    while (i < t.size()) {
        const char c = t[i];
        if (c == '\n') {
            zeilenAnfang = true;
            ++i;
            continue;
        }
        if (static_cast<unsigned char>(c) <= ' ') {
            ++i;
            continue;
        }
        const std::size_t a = i;
        if (c == '/' && i + 1 < t.size() && t[i + 1] == '/') {
            while (i < t.size() && t[i] != '\n') { ++i; }
            push(a, i, Farbklasse::Kommentar);
            continue;
        }
        if (c == '/' && i + 1 < t.size() && t[i + 1] == '*') {
            i += 2;
            while (i < t.size() && !(t[i] == '*' && i + 1 < t.size() && t[i + 1] == '/')) { ++i; }
            i = std::min(t.size(), i + 2);
            push(a, i, Farbklasse::Kommentar);
            continue;
        }
        if (c == '#' && zeilenAnfang) {
            while (i < t.size() && t[i] != '\n') {
                if (t[i] == '/' && i + 1 < t.size() && t[i + 1] == '/') { break; }
                ++i;
            }
            push(a, i, Farbklasse::Praeprozessor);
            zeilenAnfang = false;
            continue;
        }
        zeilenAnfang = false;
        if (c == '"') {
            ++i;
            while (i < t.size() && t[i] != '"' && t[i] != '\n') { ++i; }
            if (i < t.size() && t[i] == '"') { ++i; }
            const bool menus = i > a + 2 && t[a + 1] == '@';
            push(a, i, menus ? Farbklasse::MenuText : Farbklasse::String);
            continue;
        }
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == ';') {
            push(a, a + 1, Farbklasse::Klammer);
            ++i;
            continue;
        }
        while (i < t.size() && static_cast<unsigned char>(t[i]) > ' ' && t[i] != '{' && t[i] != '}' && t[i] != '"' && t[i] != ';' &&
               !(t[i] == '/' && i + 1 < t.size() && (t[i + 1] == '/' || t[i + 1] == '*'))) {
            ++i;
        }
        if (i == a) {
            ++i;
            continue;
        }
        const std::string w = t.substr(a, i - a);
        Farbklasse k = Farbklasse::Text;
        const char w0 = w[0];
        if ((w0 >= '0' && w0 <= '9') || ((w0 == '-' || w0 == '.') && w.size() > 1)) {
            k = Farbklasse::Zahl;
        } else if (w0 == '@') {
            k = Farbklasse::MenuText;
        } else {
            const auto it = worte.find(klein(w));
            if (it != worte.end()) {
                k = it->second;
            } else if (w.compare(0, 10, "ITEM_TYPE_") == 0 || w.compare(0, 13, "WINDOW_STYLE_") == 0 ||
                       w.compare(0, 11, "ITEM_ALIGN_") == 0 || w.compare(0, 7, "WINDOW_") == 0 ||
                       w.compare(0, 7, "FEEDER_") == 0 || w.compare(0, 15, "ITEM_TEXTSTYLE_") == 0 ||
                       (defines != nullptr && defines->count(w) != 0)) {
                k = Farbklasse::Konstante;
            }
        }
        push(a, i, k);
    }
    return v;
}

// --- Aenderungsrand ------------------------------------------------------------

ZeilenDiff zeilenDiff(const std::string& alt, const std::string& neu) {
    auto teile = [](const std::string& s) {
        std::vector<std::string> z;
        std::size_t a = 0;
        while (a <= s.size()) {
            std::size_t e = s.find('\n', a);
            if (e == std::string::npos) { e = s.size(); }
            std::string l = s.substr(a, e - a);
            if (!l.empty() && l.back() == '\r') { l.pop_back(); }
            z.push_back(l);
            a = e + 1;
        }
        return z;
    };
    const std::vector<std::string> A = teile(alt);
    const std::vector<std::string> B = teile(neu);
    ZeilenDiff d;
    d.geaendert.assign(B.size(), 1);
    // gemeinsamer Anfang und gemeinsames Ende vorweg (meist fast alles)
    std::size_t pre = 0;
    while (pre < A.size() && pre < B.size() && A[pre] == B[pre]) { d.geaendert[pre++] = 0; }
    std::size_t sa = A.size();
    std::size_t sb = B.size();
    while (sa > pre && sb > pre && A[sa - 1] == B[sb - 1]) {
        d.geaendert[--sb] = 0;
        --sa;
    }
    // Mitte: LCS (Laengen-Tabelle, begrenzt)
    const std::size_t n = sa - pre;
    const std::size_t m = sb - pre;
    if (n > 0 && m > 0 && n * m <= 4000000) {
        std::vector<std::uint16_t> L((n + 1) * (m + 1), 0);
        for (std::size_t i = n; i-- > 0;) {
            for (std::size_t j = m; j-- > 0;) {
                L[i * (m + 1) + j] = (A[pre + i] == B[pre + j])
                                         ? static_cast<std::uint16_t>(L[(i + 1) * (m + 1) + j + 1] + 1)
                                         : std::max(L[(i + 1) * (m + 1) + j], L[i * (m + 1) + j + 1]);
            }
        }
        std::size_t i = 0, j = 0;
        while (i < n && j < m) {
            if (A[pre + i] == B[pre + j]) {
                d.geaendert[pre + j] = 0;
                ++i;
                ++j;
            } else if (L[(i + 1) * (m + 1) + j] >= L[i * (m + 1) + j + 1]) {
                d.geloeschtVor.push_back(static_cast<int>(pre + j));
                ++i;
            } else {
                ++j;
            }
        }
        if (i < n) { d.geloeschtVor.push_back(static_cast<int>(pre + j)); }
    } else if (n > 0 && m == 0) {
        d.geloeschtVor.push_back(static_cast<int>(pre));
    }
    std::sort(d.geloeschtVor.begin(), d.geloeschtVor.end());
    d.geloeschtVor.erase(std::unique(d.geloeschtVor.begin(), d.geloeschtVor.end()), d.geloeschtVor.end());
    return d;
}

std::vector<std::string> vorschlaege(const std::string& praefix, const std::vector<std::string>& kandidaten, std::size_t max) {
    std::vector<std::string> anfang, enthalten;
    std::set<std::string> gesehen;
    const std::string p = klein(praefix);
    for (const std::string& k : kandidaten) {
        const std::string kk = klein(k);
        if (kk == p || !gesehen.insert(kk).second) { continue; }
        if (kk.compare(0, p.size(), p) == 0) { anfang.push_back(k); }
        else if (!p.empty() && kk.find(p) != std::string::npos) { enthalten.push_back(k); }
    }
    auto ord = [](const std::string& a, const std::string& b) { return klein(a) < klein(b); };
    std::sort(anfang.begin(), anfang.end(), ord);
    std::sort(enthalten.begin(), enthalten.end(), ord);
    anfang.insert(anfang.end(), enthalten.begin(), enthalten.end());
    if (anfang.size() > max) { anfang.resize(max); }
    return anfang;
}

}  // namespace bhed::menue
