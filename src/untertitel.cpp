// untertitel.cpp - Untertitel wie das Spiel (siehe bhed/untertitel.h)
//
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#include "bhed/untertitel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace bhed::untertitel {
namespace {

std::string gross(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

std::vector<std::string> pakete(const std::string& karte, Spiel spiel, const std::function<bool(const std::string&)>& paketDa) {
    // mapname aus der serverinfo: ohne "maps/" und ".bsp"; stripLevelName[0]
    // ist alles nach dem letzten '/', gross (Q_strupr).
    std::string name = karte;
    const std::size_t strich = name.find_last_of("/\\");
    if (strich != std::string::npos) { name = name.substr(strich + 1); }
    const std::size_t punkt = name.find('.');
    if (punkt != std::string::npos) { name.resize(punkt); }
    name = gross(name);
    if (name.empty()) { return {}; }
    std::vector<std::string> p{name};
    const auto zusatz = [&p, &name] {
        // "additional SP files needed for some levels..."
        if (name == "KEJIM_BASE" || name == "KEJIM_POST") { p = {name, "ARTUS_MINE"}; }
        if (name == "DOOM_DETENTION" || name == "DOOM_SHIELDS") { p = {name, "DOOM_COMM"}; }
        if (name == "DOOM_COMM") { p = {name, "CAIRN_BAY"}; }
        if (name == "NS_STARPAD") { p = {name, "ARTUS_TOPSIDE", "BESPIN_UNDERCITY"}; }
        if (name == "BESPIN_PLATFORM") { p = {name, "BESPIN_UNDERCITY"}; }
    };
    const auto umbenennen = [&name]() -> bool {
        if (name == "YAVIN_FINAL" || name == "YAVIN_SWAMP") { name = "YAVIN_CANYON"; return true; }
        if (name == "YAVIN_TRIAL") { name = "YAVIN_TEMPLE"; return true; }
        if (name == "VALLEY") { name = "ARTUS_TOPSIDE"; return true; }
        return false;
    };
    switch (spiel) {
        case Spiel::JediAcademy:
            // code/cgame/cg_servercmds.cpp: "JKA..." - der JK2-Teil ist auskommentiert.
            if (name == "YAVIN1B") { p.emplace_back("YAVIN1"); }
            break;
        case Spiel::MovieDuels:
            // cgame/cg_servercmds.cpp: YAVIN1B, sonst die JK2-Karten ohne cgi_SP_Register.
            if (name == "YAVIN1B") {
                p.emplace_back("YAVIN1");
            } else if (umbenennen()) {
                p = {name};
            } else {
                zusatz();
            }
            break;
        case Spiel::JediOutcast:
            // codeJK2/cgame/cg_servercmds.cpp: umbenannt nur, wenn das Paket der
            // Karte nicht laedt; die Zusatzpakete nur, wenn es laedt.
            if (!paketDa || !paketDa(name)) {
                if (umbenennen()) { p = {name}; }
            } else {
                zusatz();
            }
            break;
    }
    return p;
}

std::string referenz(const std::string& klang) {
    // PlayIcarusSound: Q_strlwr, COM_StripExtension; CG_CaptionText: strrchr '/'.
    std::string n = klang;
    for (char& c : n) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (c == '\\') { c = '/'; }
    }
    const std::size_t strich = n.find_last_of('/');
    const std::size_t punkt = n.find_last_of('.');
    if (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) { n.resize(punkt); }
    if (strich == std::string::npos) { return {}; }
    return n.substr(strich + 1);
}

std::string text(const std::string& klang, const std::vector<std::string>& pakete, const std::map<std::string, std::string>& werte,
                 std::string* schluessel) {
    const std::string ref = gross(referenz(klang));
    if (ref.empty()) { return {}; }
    if (ref[0] == '#') {
        // "the striped package name is already built in"
        const std::string k = ref.substr(1);
        if (schluessel != nullptr) { *schluessel = k; }
        const auto it = werte.find(k);
        return it != werte.end() ? it->second : std::string();
    }
    for (const std::string& p : pakete) {
        const std::string k = p + "_" + ref;
        if (schluessel != nullptr && schluessel->empty()) { *schluessel = k; }
        const auto it = werte.find(k);
        if (it != werte.end() && !it->second.empty()) {
            if (schluessel != nullptr) { *schluessel = k; }
            return it->second;
        }
    }
    return {};
}

std::string jk2Reinigen(std::string t) {
    for (char& c : t) {
        const auto u = static_cast<unsigned char>(c);
        if (u == 0x92 || u == 0x91) { c = '\''; }
        else if (u == 0x93 || u == 0x94) { c = '"'; }
        else if (u == 0x0B || u == 0x85) { c = '.'; }   // 0x85: "can't do in-string replace ... so just forget it"
        else if (u == 0x96 || u == 0x97) { c = '-'; }
        else if (u == 0x09) { c = ' '; }
    }
    for (std::size_t i; (i = t.find("?.")) != std::string::npos;) { t[i + 1] = ' '; }
    return t;
}

namespace {

constexpr int kSchirmBreite = 640;    // SCREEN_WIDTH
constexpr int kSchirmHoehe = 480;     // SCREEN_HEIGHT
constexpr int kMaxZeilen = 32;        // MAX_CAPTIONTEXT
constexpr std::size_t kZeilenPuffer = 256;   // captionText[][256]

// AnyLanguage_ReadCharFromString, westliche Sprachen: ein Byte; nach diesen
// Zeichen darf eine Zeile brechen.
bool satzzeichenDanach(unsigned char c) { return c == '!' || c == '?' || c == ',' || c == '.' || c == ';' || c == ':'; }

}  // namespace

int gSubtitlesVorgabe(Spiel spiel) noexcept { return spiel == Spiel::JediOutcast ? 2 : 0; }

bool wirdGesendet(const Klangfall& k) noexcept {
    // g_skippingcin: beim Ueberspringen einer Zwischensequenz nie - die
    // Vorschau ueberspringt nicht.
    if (k.gSubtitles == 1 || k.npcMitUntertiteln) {
        if (k.imKino) { return true; }
        const double grenze = k.gedaempft ? 350.0 * 350.0 : 1200.0 * 1200.0;
        return k.rundfunk || k.abstandQuadrat < grenze;
    }
    return k.gSubtitles == 2 && k.imKino;
}

std::array<float, 4> textFarbe(const std::string& name) {
    const std::string n = gross(name);
    if (n == "BLACK") { return {0, 0, 0, 1}; }
    if (n == "RED") { return {1, 0, 0, 1}; }
    if (n == "GREEN") { return {0, 1, 0, 1}; }
    if (n == "YELLOW") { return {1, 1, 0, 1}; }
    if (n == "BLUE") { return {0, 0, 1, 1}; }
    if (n == "CYAN") { return {0, 1, 1, 1}; }
    if (n == "MAGENTA") { return {1, 0, 1, 1}; }
    return {1, 1, 1, 1};   // WHITE und alles andere
}

float mdGroesse(float cgTextprintscale) noexcept { return std::clamp(cgTextprintscale, 0.25F, 2.0F); }

int breite(const menue::Schrift& f, const std::string& text, float scale, Spiel spiel) {
    if (spiel != Spiel::JediOutcast) { return menue::schriftBreite(f, text, scale); }
    // JK2_MODE: jedes Zeichen zaehlt, 0x0A beginnt neu, ceilf.
    float maxW = 0.0F;
    float diese = 0.0F;
    for (const char ch : text) {
        const auto c = static_cast<unsigned char>(ch);
        if (c == 0x0A) {
            diese = 0.0F;
        } else {
            diese += static_cast<float>(f.glyph[c].horizAdvance) * scale;
            maxW = std::max(maxW, diese);
        }
    }
    return static_cast<int>(std::ceil(maxW));
}

Untertitel setzen(const std::string& text, int klangMs, double startMs, bool imKino, float balkenZiel, const menue::Schrift& schrift,
                  Spiel spiel, float mdGroesseJetzt) {
    Untertitel u;
    u.startMs = startMs;
    const bool md = spiel == Spiel::MovieDuels;
    u.schriftGroesse = md ? mdGroesse(mdGroesseJetzt) : 1.0F;
    const int fontHeight = menue::schriftHoehe(schrift, u.schriftGroesse);   // westlich: nicht * 1.4
    if (imKino) {
        u.textY = static_cast<int>(static_cast<float>(kSchirmHoehe) - balkenZiel / 2.0F);
    } else {
        u.textY = static_cast<int>(0.88F * (static_cast<float>(kSchirmHoehe) - static_cast<float>(fontHeight) * 1.5F));
    }
    const int laenge = std::max(1, static_cast<int>(text.size()));   // strlen, fuer die Zeit
    u.buchstabenMs = klangMs / laenge;
    // Zeilen brechen - Positionen im Text wie die Zeiger s, holds,
    // psBestLineBreakSrcPos und psLastGood_s.
    std::vector<std::string> z(1);
    std::size_t s = 0;
    std::size_t holds = 0;
    std::size_t best = 0;
    std::size_t i = 0;
    while (s < text.size() && (!md || i < kMaxZeilen - 1) && i < kMaxZeilen) {
        const std::size_t letzteGute = s;
        const auto c = static_cast<unsigned char>(text[s]);
        const bool satzzeichen = satzzeichenDanach(c);
        ++s;
        if (c == ' ' && z[i].empty()) {
            ++holds;
            continue;   // ein Leerzeichen am Zeilenanfang faellt weg
        }
        if (z[i].size() < kZeilenPuffer - 1) { z[i] += static_cast<char>(c); }   // Q_strcat
        if (c == '\n') {
            z[i].pop_back();
            ++i;
            z.emplace_back();
            holds = s;
            best = s;
        } else if (breite(schrift, z[i], u.schriftGroesse, spiel) >= kSchirmBreite || (md && z[i].size() >= kZeilenPuffer - 4)) {
            if (best == holds) { best = letzteGute; }   // keine Bruchstelle: nach dem letzten Zeichen
            // Nach einem uebersprungenen Leerzeichen kann best vor holds liegen;
            // das Spiel schreibt dann vor die Zeile und kuerzt sie nicht.
            if (best >= holds) { z[i].resize(std::min(z[i].size(), best - holds)); }
            holds = best;
            s = best;
            ++i;
            z.emplace_back();
        }
        if (satzzeichen || c == ' ') { best = s; }
    }
    u.zeilen = std::move(z);
    std::size_t halten = u.zeilen[0].size();
    if (u.zeilen.size() > 1) { halten += u.zeilen[1].size(); }
    u.ersteSeiteBis = startMs + static_cast<double>(halten) * static_cast<double>(u.buchstabenMs);
    return u;
}

std::vector<Zeile> zeigen(const Untertitel& u, double ms, const menue::Schrift& schrift, Spiel spiel, bool imKino, float balkenZiel) {
    if (ms < u.startMs || u.zeilen.empty()) { return {}; }
    const auto zeile = [&u](std::size_t n) -> const std::string& {
        static const std::string leer;
        return n < u.zeilen.size() ? u.zeilen[n] : leer;
    };
    // cg.captionNextTextTime < cg.time: das Spiel blaettert im ersten
    // Clientbild NACH captionNextTextTime und rechnet die naechste Seite ab
    // dessen cg.time. Die Bilder liegen ab der Ankunft (cg.captionTextTime)
    // alle kClientBildMs.
    const auto erstesBildNach = [&u](double t) {
        return u.startMs + (std::floor((t - u.startMs) / kClientBildMs) + 1.0) * kClientBildMs;
    };
    std::size_t aktuell = 0;
    double wechsel = erstesBildNach(u.ersteSeiteBis);
    while (wechsel <= ms) {
        aktuell += 2;
        if (aktuell >= u.zeilen.size()) { return {}; }
        const std::size_t halten = zeile(aktuell).size() + zeile(aktuell + 1).size();
        wechsel = erstesBildNach(wechsel + static_cast<double>(halten) * static_cast<double>(u.buchstabenMs));
    }
    const bool md = spiel == Spiel::MovieDuels;
    const float groesse = u.schriftGroesse;
    const int fontHeight = menue::schriftHoehe(schrift, groesse);
    const bool zweiZeilen = !zeile(aktuell + 1).empty();
    int y = static_cast<int>(static_cast<float>(u.textY) - static_cast<float>(fontHeight) * (zweiZeilen ? 1.0F : 0.5F));
    y -= md ? static_cast<int>(4.0F * groesse) : 4;
    int schritt = fontHeight;
    if (md && groesse > 1.0F) {
        // (2026-10-13): grosse Schrift bleibt im Bild und in den Balken.
        const int zeilen = zweiZeilen ? 2 : 1;
        if (imKino && zeilen == 2 && fontHeight * 2 > static_cast<int>(balkenZiel)) {
            schritt = static_cast<int>(static_cast<float>(fontHeight) * 0.85F);
        }
        const int unten = y + (zeilen - 1) * schritt + fontHeight;
        if (unten > kSchirmHoehe - 2) { y -= unten - (kSchirmHoehe - 2); }
    }
    std::vector<Zeile> aus;
    for (std::size_t n = aktuell; n < aktuell + 2; ++n) {
        const int w = breite(schrift, zeile(n), groesse, spiel);
        if (w != 0) {
            aus.push_back(Zeile{zeile(n), (kSchirmBreite - w) / 2, y});
            y += schritt;
        }
    }
    return aus;
}

}  // namespace bhed::untertitel
