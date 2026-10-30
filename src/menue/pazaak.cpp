// pazaak.cpp - Menue-Editor M7 Punkt 14: Pazaak-Ownerdraws von Movie Duels (Herkunft und Lizenz im
// Kopf von include/bhed/menue/pazaak.h).
//
//   Movie Duels / OpenJK: GNU General Public License version 2 (LICENSE).
#include "bhed/menue/pazaak.h"

#include <cmath>
#include <cstring>

namespace bhed::menue {

namespace {

constexpr float kSchrift = 0.5F;   // PZK_FONT_SCALE
constexpr Farbe kKarte{1, 1, 1, 0.8F}, kKarteSteht{0.6F, 0.6F, 0.6F, 0.8F}, kWeiss{1, 1, 1, 1};   // cardColor, cardColorStand, pzkWhite
constexpr int NORMAL_1 = 0, NORMAL_10 = 9, PLUS_1 = 10, PLUS_6 = 15, MINUS_1 = 16, MINUS_6 = 21, FLIP_1 = 22, FLIP_6 = 27,
              TIEBREAKER = 28, DOUBLE = 29, FLIP12 = 30, ZWEI_VIER = 31, DREI_SECHS = 32, BACK = -2;

// Pazaak_CacheShaders mit vidWidth > 800 (die _m-Bilder)
std::string kartenBild(int id, int param) {   // Pazaak_GetCardShader
    const std::string p = "gfx/minigames/pazaak/cards/";
    if (id == BACK) { return p + "back_m.png"; }
    if (id >= NORMAL_1 && id <= NORMAL_10) { return p + "green_m.png"; }
    if (id >= PLUS_1 && id <= PLUS_6) { return p + "blue_m.png"; }
    if (id >= MINUS_1 && id <= MINUS_6) { return p + "red_m.png"; }
    if (id >= FLIP_1 && id <= FLIP_6) { return p + (param == 1 || param == 3 ? "multi_neg_m.png" : "multi_pos_m.png"); }
    if (id >= TIEBREAKER && id <= DREI_SECHS) { return p + "yellow_m.png"; }
    return {};
}

std::string zahl(const char* f, int n) {
    char b[16];
    std::snprintf(b, sizeof(b), f, n);
    return b;
}

std::string neutralerWert(int id) {   // Pazaak_GetNeutralCardValue
    if (id >= NORMAL_1 && id <= NORMAL_10) { return zahl("%i", id - NORMAL_1 + 1); }
    if (id >= PLUS_1 && id <= PLUS_6) { return zahl("+%i", id - PLUS_1 + 1); }
    if (id >= MINUS_1 && id <= MINUS_6) { return zahl("-%i", id - MINUS_1 + 1); }
    if (id >= FLIP_1 && id <= FLIP_6) { return zahl("+/-%i", id - FLIP_1 + 1); }
    switch (id) {
        case TIEBREAKER: return "+/-1T";
        case DOUBLE: return "D";
        case FLIP12: return "+/-1/2";
        case ZWEI_VIER: return "2&4";
        case DREI_SECHS: return "3&6";
        default: return "";
    }
}

std::string kartenWert(int id, int param) {   // Pazaak_GetCardValue
    if (id >= NORMAL_1 && id <= NORMAL_10) { return zahl(param != 0 ? "-%i" : "%i", id - NORMAL_1 + 1); }
    if (id >= PLUS_1 && id <= PLUS_6) { return zahl(param != 0 ? "-%i" : "+%i", id - PLUS_1 + 1); }
    if (id >= MINUS_1 && id <= MINUS_6) { return zahl(param != 0 ? "+%i" : "-%i", id - MINUS_1 + 1); }
    if (id >= FLIP_1 && id <= FLIP_6) { return zahl(param == 1 || param == 2 ? "-%i" : "+%i", id - FLIP_1 + 1); }
    switch (id) {
        case TIEBREAKER: return param != 0 ? "-1T" : "+1T";
        case DOUBLE: return param != 0 ? zahl("%i", param) : "D";
        case FLIP12: return param == 0 ? "+1" : param == 1 ? "-1" : param == 2 ? "+2" : param == 3 ? "-2" : "";
        case ZWEI_VIER: return param != 0 ? "0" : "2&4";
        case DREI_SECHS: return param != 0 ? "0" : "3&6";
        default: return "";
    }
}

struct Maler {
    const PazaakKontext& k;
    std::vector<OdTeil>& out;

    float breite(const std::string& t) const { return k.breite ? static_cast<float>(k.breite(t, 1.0F)) : 0.0F; }
    void bild(float x, float y, float w, float h, const std::string& s, const Farbe& c) {
        OdTeil p;
        p.art = OdTeil::Art::Bild;
        p.x = x;
        p.y = y;
        p.w = w;
        p.h = h;
        p.inhalt = s;
        p.farbe = c;
        out.push_back(std::move(p));
    }
    // PZK_DrawText: R_Font_DrawString((int)x, (int)y, ..., "ergoec" | (schatten ? STYLE_DROPSHADOW : 0), -1, scale)
    void text(float x, float y, const std::string& t, const Farbe& c, float sc, bool schatten = false) {
        OdTeil p;
        p.x = static_cast<float>(static_cast<int>(x));
        p.y = static_cast<float>(static_cast<int>(y));
        p.inhalt = t;
        p.farbe = c;
        p.scale = sc;
        p.schrift = "ergoec";
        p.stil = schatten ? 3 : 0;   // ITEM_TEXTSTYLE_SHADOWED = STYLE_DROPSHADOW
        out.push_back(std::move(p));
    }
    void kartenText(const std::string& t, float x, float y, float w, float h) {   // Pazaak_DrawCardText
        const float width = breite(t) * kSchrift;
        text(x + w / 4 + (w / 4 - width / 2), y + h * 0.25F, t, kWeiss, kSchrift);
    }
    float puls() const { return 0.8F + 0.2F * std::fabs(std::sin(static_cast<float>(k.zeitMs) / 200.F)); }

    void feld(int slot, float x, float y, float w, float h) {   // SJE_Pazaak_DrawCardSlot
        if (slot < 1 || slot > 18) { return; }
        const PazaakZustand& z = k.zustand;
        const Farbe c = (slot > 9 ? z.stehtGegner : z.stehtSpieler) ? kKarteSteht : kKarte;
        const PazaakKarte& kk = z.feld[static_cast<std::size_t>(slot - 1)];
        const std::string b = kartenBild(kk.id, kk.param);
        if (b.empty()) { return; }
        bild(x, y, w, h, b, c);
        kartenText(kartenWert(kk.id, kk.param), x, y, w, h);
    }
    void wahl(int slot, float x, float y, float w, float h) {   // SJE_Pazaak_DrawSelCardSlot (ohne Maus darueber)
        const PazaakZustand& z = k.zustand;
        if (slot < 0 || slot > 22 || z.karten[static_cast<std::size_t>(slot)] == 0) { return; }
        const std::string b = kartenBild(slot + PLUS_1, 0);
        if (b.empty()) { return; }
        bild(x, y, w, h, b, kKarte);
        kartenText(neutralerWert(slot + PLUS_1), x, y, w, h);
        int imDeck = 0;
        for (const int s : z.seitendeck) { imDeck += s == slot + PLUS_1 ? 1 : 0; }
        text(x + 7, y - 2, zahl("%i", z.karten[static_cast<std::size_t>(slot)] - imDeck), kWeiss, kSchrift, true);
    }
    void seitendeck(int slot, float x, float y, float w, float h) {   // SJE_Pazaak_DrawSideDeckSlot
        if (slot < 0 || slot > 9) { return; }
        const int id = k.zustand.seitendeck[static_cast<std::size_t>(slot)];
        const std::string b = kartenBild(id, 0);
        if (b.empty()) { return; }
        bild(x, y, w, h, b, kKarte);
        kartenText(neutralerWert(id), x, y, w, h);
    }
    void hand(int slot, float x, float y, float w, float h) {   // SJE_Pazaak_DrawHandSlot
        if (slot < 1 || slot > 8) { return; }
        const PazaakKarte& kk = k.zustand.hand[static_cast<std::size_t>(slot - 1)];
        const std::string b = kartenBild(kk.id, kk.param);
        if (b.empty()) { return; }
        bild(x, y, w, h, b, kKarte);
        kartenText(kartenWert(kk.id, kk.param), x, y, w, h);
    }
    void namen(int spieler, float x, float y) {   // SJE_Pazaak_DrawNames: Schatten mit 1,02-facher Groesse
        const std::string& t = spieler == 1 ? k.zustand.nameSpieler : k.zustand.nameGegner;
        if (spieler == 2) { x -= breite(t) * kSchrift; }
        std::string sauber;   // Q_StripColor
        for (std::size_t i = 0; i < t.size(); ++i) {
            if (t[i] == '^' && i + 1 < t.size() && t[i + 1] != '^') {
                ++i;
                continue;
            }
            sauber += t[i];
        }
        text(x - 0.6F, y - 0.75F, sauber, Farbe{0.15F, 0.15F, 0.15F, 1}, kSchrift * 1.02F);
        text(x, y, t, kWeiss, kSchrift);
    }
    void punkte(int spieler, float x, float y) {   // SJE_Pazaak_DrawPoints
        const PazaakZustand& z = k.zustand;
        std::string t;
        if (spieler == 1) {
            t = z.stehtSpieler ? zahl("( %i )", z.punkteSpieler) : zahl("( ^5%i ^7)", z.punkteSpieler);
            x -= breite(t) * kSchrift;
        } else {
            t = z.stehtGegner ? zahl("( %i )", z.punkteGegner) : zahl("( ^5%i ^7)", z.punkteGegner);
        }
        text(x, y, t, kWeiss, kSchrift);
    }
    void dialog(int zeile, float x, float y, float w) {   // SJE_Pazaak_DrawDialog
        const PazaakZustand& z = k.zustand;
        if (z.dialog1.empty() && z.dialog2.empty()) { return; }   // PDlgData.InUse
        const std::string& t = zeile == 1 ? z.dialog1 : z.dialog2;
        x += w / 2 - breite(t) * kSchrift / 2;
        text(x + 1, y + 1, t, Farbe{0, 0, 0, 0.2F}, kSchrift);
        text(x, y, t, kWeiss, kSchrift);
    }
    void zeit(float x, float y, float w) {   // SJE_Pazaak_DrawTimeout
        const int tr = k.zustand.restZeit;
        if (tr <= 0) { return; }
        const std::string t = tr <= 3 ? zahl("^1%i", tr) : zahl("%i", tr);
        x += w / 2 - breite(t) * 0.6F / 2;
        text(x, y, t, kWeiss, 0.6F);
    }
    void warten(float x, float y, float w, float h, const std::string& shader) {   // SJE_Pazaak_DrawWaiting
        if (!k.zustand.wartet || shader.empty()) { return; }
        bild(x, y, w, h, shader, Farbe{1, 1, 1, std::fabs(std::sin(static_cast<float>(-k.zeitMs) / 400.0F))});
    }

    // SJE_Pazaak_HowToWords: Woerter mit breiterem Abstand (Leerzeichen der Schrift + 3)
    float woerter(const std::string& t, std::size_t len, float x, float y, const Farbe& c, float sc, bool zeichnen) {
        const float abstand = breite(" ") * sc + 3.0F;
        float width = 0;
        std::size_t i = 0;
        int anzahl = 0;
        while (i < len) {
            while (i < len && t[i] == ' ') { ++i; }
            if (i >= len) { break; }
            std::size_t n = 0;
            while (i + n < len && t[i + n] != ' ' && n < 127) { ++n; }
            const std::string w = t.substr(i, n);
            if (anzahl++ != 0) { width += abstand; }
            if (zeichnen) { text(x + width, y, w, c, sc, true); }
            width += breite(w) * sc;
            i += n;
        }
        return width;
    }
    void anleitung(float x, float y, float w) {   // SJE_Pazaak_DrawHowTo
        static constexpr const char* kText[] = {
            "#Goal",
            "End each set closer to 20 than your opponent without going over. The first player to win 3 sets wins the match.",
            "#Your side deck",
            "Click 10 of your cards to put them in your side deck, then press Continue. At the start of the match 4 of them "
            "are dealt into your hand at random. They must last the whole match: a card you play is gone.",
            "#Your turn",
            "A card from 1 to 10 is dealt onto your board. You may then play one card from your hand. Press End Turn to go "
            "on, or Stand to keep your total for the rest of the set.",
            "#Winning a set",
            "Over 20 at the end of your turn and you lose the set. Fill your board with 9 cards without going over and you "
            "win it. When both players stand, the higher total wins. A tie has no winner, unless a tiebreaker card was played.",
            "#The cards",
            "Blue cards add, red cards subtract, and the +/- cards can be played either way. Gold cards: +/-1T is a "
            "tiebreaker, D doubles your last card, +/-1/2 counts as 1 or 2. The 24 card flips the sign of the 2s and 4s on "
            "your board, the 36 card those of the 3s and 6s."};
        constexpr Farbe kTitel{0.35F, 0.8F, 1.0F, 1.0F};
        const std::string titel = "HOW TO PLAY";
        const float tw = woerter(titel, titel.size(), 0, 0, kTitel, 0.75F, false);
        woerter(titel, titel.size(), x + w / 2 - tw / 2, y, kTitel, 0.75F, true);
        y += 24.0F;
        for (const char* q : kText) {
            const std::string p0 = q;
            if (p0[0] == '#') {
                y += 4.0F;
                woerter(p0.substr(1), p0.size() - 1, x, y, kTitel, 0.54F, true);
                y += 14.5F;
                continue;
            }
            std::size_t p = 0;
            while (p < p0.size()) {
                const std::string rest = p0.substr(p);
                std::size_t len = 0, fit = 0;
                while (len < rest.size()) {
                    std::size_t end = len;
                    while (end < rest.size() && rest[end] == ' ') { ++end; }
                    while (end < rest.size() && rest[end] != ' ') { ++end; }
                    if (fit != 0 && woerter(rest, end, 0, 0, kWeiss, 0.47F, false) > w) { break; }
                    fit = end;
                    len = end;
                }
                if (fit == 0) { fit = len; }
                woerter(rest, fit, x, y, kWeiss, 0.47F, true);
                y += 12.0F;
                p += fit;
                while (p < p0.size() && p0[p] == ' ') { ++p; }
            }
        }
    }
};

}  // namespace

bool pazaakOwnerDraw(int id, float x, float y, float w, float h, const std::string& shader, const PazaakKontext& k, std::vector<OdTeil>& out) {
    if (id < 2000 || id >= 3000) { return false; }   // PZK_OD_BASE .. + 1000
    const int art = (id - 2000) / 100;
    const int nr = (id - 2000) % 100;
    Maler m{k, out};
    switch (art) {
        case 0: m.feld(nr, x, y, w, h); break;
        case 1: m.hand(nr, x, y, w, h); break;
        case 2: m.namen(nr, x, y); break;
        case 3: m.punkte(nr, x, y); break;
        case 4: m.dialog(nr, x, y, w); break;
        case 5: m.zeit(x, y, w); break;
        case 6: m.wahl(nr, x, y, w, h); break;
        case 7: m.seitendeck(nr, x, y, w, h); break;
        case 8: m.warten(x, y, w, h, shader); break;
        case 9: m.anleitung(x, y, w); break;
        default: break;
    }
    return true;
}

}  // namespace bhed::menue
