// lesbarkeit.cpp - Menue-Editor M6 Punkt 9: Lesbarkeit (siehe lesbarkeit.h).
//
// Eigener Code.
#include "bhed/menue/lesbarkeit.h"

#include <algorithm>
#include <cmath>

namespace bhed::menue {

namespace {

float kanal(float c) {
    c = std::clamp(c, 0.0F, 1.0F);
    return c <= 0.03928F ? c / 12.92F : std::pow((c + 0.055F) / 1.055F, 2.4F);
}

float leuchtdichte(const Farbe& f) { return 0.2126F * kanal(f.r) + 0.7152F * kanal(f.g) + 0.0722F * kanal(f.b); }

// Der Text, wie das Spiel ihn zeigt (@-Schluessel ueber StringEd; unbekannt = "")
std::string imSpiel(const Item& it, const Texte* t) {
    if (t == nullptr || it.text.empty() || it.text[0] != '@') { return it.text; }
    return t->imSpiel(it.text);
}

}  // namespace

float kontrast(const Farbe& a, const Farbe& b) {
    const float la = leuchtdichte(a), lb = leuchtdichte(b);
    return (std::max(la, lb) + 0.05F) / (std::min(la, lb) + 0.05F);
}

std::vector<LesbarkeitsBefund> lesbarkeit(const Datei& d, int menu, const Umgebung& u, const std::vector<SprachTexte>& sprachen) {
    std::vector<LesbarkeitsBefund> r;
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return r; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        const Item& it = m.items[i];
        const std::string text = imSpiel(it, u.texte);
        if (text.empty() || (it.window.flags & WINDOW_VISIBLE) == 0) { continue; }
        const Rechteck t = textRechteck(it, text, u);
        if (t.h > 0 && t.h < kMindestTexthoehe) { r.push_back({LesbarArt::SchriftKlein, static_cast<int>(i), t.h, kMindestTexthoehe, {}}); }
        if (it.window.style == WINDOW_STYLE_FILLED && it.window.backColor.a >= 0.5F) {
            const float k = kontrast(it.window.foreColor, it.window.backColor);
            if (k < kMindestKontrast) { r.push_back({LesbarArt::KontrastGering, static_cast<int>(i), k, kMindestKontrast, {}}); }
        }
        // Laenge je Sprache: nur, wenn der Text in einer Sprache ins Feld passt und in einer anderen nicht
        const Rechteck& f = it.window.rect;
        if (sprachen.size() < 2 || f.w <= 0 || (it.window.flags & (WINDOW_WRAPPED | WINDOW_AUTOWRAPPED)) != 0) { continue; }
        bool passtIrgendwo = false;
        std::vector<std::pair<std::string, float>> zuLang;
        for (const SprachTexte& s : sprachen) {
            Umgebung us = u;
            us.texte = s.texte;
            const std::string tx = imSpiel(it, s.texte);
            if (tx.empty()) { continue; }
            const Rechteck q = textRechteck(it, tx, us);
            if (q.w <= 0) { continue; }
            if (q.x >= f.x - 0.5F && q.x + q.w <= f.x + f.w + 0.5F) {
                passtIrgendwo = true;
            } else {
                zuLang.emplace_back(s.sprache, q.w);
            }
        }
        if (!passtIrgendwo) { continue; }
        for (const auto& [sp, w] : zuLang) { r.push_back({LesbarArt::TextZuLang, static_cast<int>(i), w, f.w, sp}); }
    }
    return r;
}

}  // namespace bhed::menue
