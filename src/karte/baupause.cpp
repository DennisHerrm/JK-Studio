// baupause.cpp - Karten-Editor, vis2c: einen Bau anhalten und spaeter fortsetzen (siehe baupause.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114): vis.cpp LoadPortals (portalbytes aus der Zahl der Portale) fuer die
// Hoechstgroesse des VIS-Zwischenstands. Alles andere ist eigener Code zu den BehavEd-Erweiterungen von bhed_q3map2
// (visflow.cpp BhedVisState: Zwischenstand und Stoppdatei; bhed_lichtcache.cpp: der Licht-Cache).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/baupause.h"

#include "bhed/json.h"
#include "bhed/sicherschreiben.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::karte {

namespace {

namespace fs = std::filesystem;

fs::path pfadAus(const std::string& p) { return fs::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size())); }

std::string textAus(const fs::path& p) {
    const std::u8string u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}

std::string klein(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Anfang und Ende jedes Wortes eines Befehls (Anfuehrungszeichen halten zusammen)
std::vector<std::pair<std::size_t, std::size_t>> wortGrenzen(const std::string& befehl) {
    std::vector<std::pair<std::size_t, std::size_t>> w;
    bool inQuote = false;
    bool imWort = false;
    std::size_t anfang = 0;
    for (std::size_t i = 0; i <= befehl.size(); ++i) {
        const char c = i < befehl.size() ? befehl[i] : ' ';
        const bool trenner = (!inQuote || i == befehl.size()) && std::isspace(static_cast<unsigned char>(c)) != 0;
        if (c == '"') { inQuote = !inQuote; }
        if (!imWort && !trenner) {
            imWort = true;
            anfang = i;
        } else if (imWort && trenner) {
            imWort = false;
            w.emplace_back(anfang, i);
        }
    }
    return w;
}

bool hatWort(const std::string& befehl, const std::string& schalter) {
    for (const auto& [a, e] : wortGrenzen(befehl)) {
        if (klein(befehl.substr(a, e - a)) == schalter) { return true; }
    }
    return false;
}

bool dateiDa(const fs::path& p) {
    std::error_code ec;
    return fs::is_regular_file(p, ec);
}

bool kopieren(const fs::path& von, const fs::path& nach) {
    std::error_code ec;
    fs::copy_file(von, nach, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

// verschieben (ueber ein vorhandenes Ziel); zwischen Laufwerken kopieren und loeschen
bool verschieben(const fs::path& von, const fs::path& nach) {
    std::error_code ec;
    fs::remove(nach, ec);
    ec.clear();
    fs::rename(von, nach, ec);
    if (!ec) { return true; }
    if (!kopieren(von, nach)) { return false; }
    fs::remove(von, ec);
    return true;
}

void melde(std::string* fehler, const std::string& text) {
    if (fehler != nullptr) { *fehler = text; }
}

std::string hex64(std::uint64_t v) {
    char z[24];
    std::snprintf(z, sizeof(z), "%016llx", static_cast<unsigned long long>(v));
    return z;
}

}  // namespace

std::string bauPauseOrdner(const std::string& stamm) { return stamm + ".bhedpause"; }

std::string bauPauseStamm(const std::string& stamm) {
    const std::size_t p = stamm.find_last_of("/\\");
    return bauPauseOrdner(stamm) + "/" + (p == std::string::npos ? stamm : stamm.substr(p + 1));
}

std::string bauPauseText(const BauPause& p) {
    json::Wert w = json::Wert::alsObjekt();
    w.setze("fassung", json::Wert::alsZahl(1));
    w.setze("eintrag", json::Wert::alsText(p.eintrag));
    w.setze("karte", json::Wert::alsText(p.karte));
    w.setze("stamm", json::Wert::alsText(p.stamm));
    w.setze("mapDatei", json::Wert::alsText(p.mapDatei));
    json::Wert b = json::Wert::alsListe();
    for (const std::string& x : p.befehle) { b.liste.push_back(json::Wert::alsText(x)); }
    w.setze("befehle", std::move(b));
    w.setze("schritt", json::Wert::alsZahl(static_cast<double>(p.schritt)));
    w.setze("zeit", json::Wert::alsText(p.zeit));
    w.setze("vergangen", json::Wert::alsZahl(p.vergangen));
    json::Wert f = json::Wert::alsListe();
    for (const double d : p.fertig) { f.liste.push_back(json::Wert::alsZahl(d)); }
    w.setze("fertig", std::move(f));
    w.setze("visFertig", json::Wert::alsZahl(p.visFertig));
    w.setze("visAlle", json::Wert::alsZahl(p.visAlle));
    w.setze("lichtPaesse", json::Wert::alsZahl(p.lichtPaesse));
    w.setze("lichtPaesseVon", json::Wert::alsZahl(p.lichtPaesseVon));
    w.setze("kartenHash", json::Wert::alsText(hex64(p.kartenHash)));   // als Text: eine JSON-Zahl haelt keine 64 Bit
    return json::schreibe(w);
}

bool bauPauseAusText(const std::string& text, BauPause& p) {
    json::Wert w;
    if (!json::lies(text, w) || w.art != json::Wert::Art::Objekt || w.zahlVon("fassung", 0.0) != 1.0) { return false; }
    BauPause n;
    n.eintrag = w.textVon("eintrag");
    n.karte = w.textVon("karte");
    n.stamm = w.textVon("stamm");
    n.mapDatei = w.textVon("mapDatei");
    if (const json::Wert* b = w.feld("befehle"); b != nullptr && b->art == json::Wert::Art::Liste) {
        for (const json::Wert& x : b->liste) { n.befehle.push_back(x.text); }
    }
    const double schritt = w.zahlVon("schritt", -1.0);
    n.zeit = w.textVon("zeit");
    n.vergangen = w.zahlVon("vergangen", 0.0);
    if (const json::Wert* f = w.feld("fertig"); f != nullptr && f->art == json::Wert::Art::Liste) {
        for (const json::Wert& x : f->liste) { n.fertig.push_back(x.zahl); }
    }
    n.visFertig = static_cast<int>(w.zahlVon("visFertig", -1.0));
    n.visAlle = static_cast<int>(w.zahlVon("visAlle", 0.0));
    n.lichtPaesse = static_cast<int>(w.zahlVon("lichtPaesse", 0.0));
    n.lichtPaesseVon = static_cast<int>(w.zahlVon("lichtPaesseVon", 0.0));
    n.kartenHash = std::strtoull(w.textVon("kartenHash").c_str(), nullptr, 16);
    // ein Stand, mit dem sich nicht weiterbauen laesst, ist keiner
    if (n.stamm.empty() || n.mapDatei.empty() || n.befehle.empty() || schritt < 0.0 || schritt >= static_cast<double>(n.befehle.size()) ||
        n.fertig.size() != static_cast<std::size_t>(schritt)) {
        return false;
    }
    n.schritt = static_cast<std::size_t>(schritt);
    p = std::move(n);
    return true;
}

bool bauPauseLesen(const std::string& stamm, BauPause& p) {
    std::ifstream f(pfadAus(bauPauseOrdner(stamm)) / "stand.json", std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    return bauPauseAusText(s.str(), p);
}

bool bauPauseSchreiben(const BauPause& p, std::string* fehler) {
    return sicherSchreiben(bauPauseOrdner(p.stamm) + "/stand.json", bauPauseText(p), fehler);
}

long long bauPauseGroesse(const std::string& stamm) {
    const fs::path ordner = pfadAus(bauPauseOrdner(stamm));
    if (!dateiDa(ordner / "stand.json")) { return -1; }
    long long summe = 0;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(ordner, ec), ende; !ec && it != ende; it.increment(ec)) {
        std::error_code e2;
        if (it->is_regular_file(e2)) { summe += static_cast<long long>(it->file_size(e2)); }
    }
    return summe;
}

std::uint64_t bauDateiHash(const std::string& pfad) {
    std::ifstream f(pfadAus(pfad), std::ios::binary);
    if (!f) { return 0; }
    std::uint64_t h = 1469598103934665603ULL;   // FNV-1a 64: Startwert und Primzahl
    char puffer[65536];
    while (f.read(puffer, sizeof(puffer)) || f.gcount() > 0) {
        const std::streamsize n = f.gcount();
        for (std::streamsize i = 0; i < n; ++i) {
            h ^= static_cast<unsigned char>(puffer[i]);
            h *= 1099511628211ULL;
        }
    }
    return h == 0 ? 1 : h;
}

bool bauPauseSichern(const BauPause& p, std::string* fehler) {
    const fs::path ordner = pfadAus(bauPauseOrdner(p.stamm));
    const fs::path ziel = pfadAus(bauPauseStamm(p.stamm));
    const fs::path stamm = pfadAus(p.stamm);
    const auto mit = [](const fs::path& s, const char* endung) {
        fs::path x = s;
        x += endung;
        return x;
    };
    std::error_code ec;
    fs::remove_all(ordner, ec);
    ec.clear();
    fs::create_directories(ordner, ec);
    if (ec) {
        melde(fehler, textAus(ordner) + ": " + ec.message());
        return false;
    }
    const fs::path karte = pfadAus(p.mapDatei);
    if (!kopieren(karte, ordner / karte.filename())) {
        melde(fehler, "cannot copy " + p.mapDatei + " to " + textAus(ordner));
        fs::remove_all(ordner, ec);
        return false;
    }
    bool ok = true;
    if (dateiDa(mit(stamm, ".bhedprelight"))) {
        ok = verschieben(mit(stamm, ".bhedprelight"), mit(ziel, ".bsp"));
    } else if (dateiDa(mit(stamm, ".bsp"))) {
        ok = kopieren(mit(stamm, ".bsp"), mit(ziel, ".bsp"));
    }
    for (const char* e : {".prt", ".srf"}) {
        if (dateiDa(mit(stamm, e))) { ok = kopieren(mit(stamm, e), mit(ziel, e)) && ok; }
    }
    if (dateiDa(mit(stamm, ".bhedvisstate"))) { ok = verschieben(mit(stamm, ".bhedvisstate"), mit(ziel, ".bhedvisstate")) && ok; }
    bool lichtKommt = false;
    for (std::size_t i = p.schritt; i < p.befehle.size(); ++i) {
        lichtKommt = lichtKommt || (bauStufeAusBefehl(p.befehle[i]) == BauStufe::Licht && hatWort(p.befehle[i], "-bhedlightcache"));
    }
    if (lichtKommt && dateiDa(mit(stamm, ".bhedlight"))) {
        // q3map2 liest den Cache und ersetzt ihn am Ende durch eine neue Datei (remove, rename) - ein fester Link
        // teilt die Daten, ohne dass der eine Bau dem anderen etwas ueberschreibt
        fs::create_hard_link(mit(stamm, ".bhedlight"), mit(ziel, ".bhedlight"), ec);
        if (ec) { ok = kopieren(mit(stamm, ".bhedlight"), mit(ziel, ".bhedlight")) && ok; }
    }
    std::string grund;
    if (!ok || !bauPauseSchreiben(p, &grund)) {
        melde(fehler, "cannot write the saved state in " + textAus(ordner) + (grund.empty() ? std::string() : " (" + grund + ")"));
        fs::remove_all(ordner, ec);
        return false;
    }
    return true;
}

std::vector<std::string> bauPauseBefehleAlt(const BauPause& p) {
    const std::string von = p.stamm + ".";
    const std::string nach = bauPauseStamm(p.stamm) + ".";
    std::vector<std::string> b = p.befehle;
    for (std::string& x : b) {
        for (std::size_t i = x.find(von); i != std::string::npos; i = x.find(von, i + nach.size())) { x.replace(i, von.size(), nach); }
    }
    return b;
}

bool bauPauseAltFertig(const BauPause& p, std::string* fehler) {
    const fs::path ordner = pfadAus(bauPauseOrdner(p.stamm));
    const fs::path ziel = pfadAus(p.stamm).parent_path();
    const fs::path name = pfadAus(p.stamm).filename();
    const fs::path karte = pfadAus(p.mapDatei).filename();
    const std::u8string vorn = name.u8string() + u8".";
    std::error_code ec;
    bool ok = true;
    std::vector<fs::path> dateien;
    for (fs::directory_iterator it(ordner, ec), ende; !ec && it != ende; it.increment(ec)) { dateien.push_back(it->path()); }
    for (const fs::path& d : dateien) {
        const fs::path n = d.filename();
        std::error_code e2;
        if (fs::is_directory(d, e2)) {
            // externe Lightmaps: <name>/lm_0000.tga ...
            if (n == name) {
                fs::copy(d, ziel / n, fs::copy_options::recursive | fs::copy_options::overwrite_existing, e2);
                ok = ok && !e2;
            }
            continue;
        }
        if (n == karte || n == "stand.json" || n.u8string().compare(0, vorn.size(), vorn) != 0) { continue; }
        ok = verschieben(d, ziel / n) && ok;
    }
    if (!ok) {
        melde(fehler, "cannot move the results from " + textAus(ordner) + " to " + textAus(ziel));
        return false;
    }
    fs::remove_all(ordner, ec);
    return true;
}

bool bauPauseAktuellVorbereiten(const BauPause& p) {
    const fs::path alt = pfadAus(bauPauseStamm(p.stamm) + ".bhedvisstate");
    const fs::path neu = pfadAus(visStandDatei(p.stamm));
    bool zurueck = false;
    std::error_code ec;
    if (dateiDa(alt)) {
        std::error_code e1;
        std::error_code e2;
        const bool neuerDa = dateiDa(neu) && fs::last_write_time(neu, e1) > fs::last_write_time(alt, e2) && !e1 && !e2;
        if (!neuerDa) { zurueck = verschieben(alt, neu); }
    }
    fs::remove_all(pfadAus(bauPauseOrdner(p.stamm)), ec);
    return zurueck;
}

bool bauPauseVerwerfen(const std::string& stamm) {
    std::error_code ec;
    fs::remove_all(pfadAus(bauPauseOrdner(stamm)), ec);
    return !ec;
}

bool bauVorLichtSichern(const std::string& stamm) {
    const fs::path bsp = pfadAus(stamm + ".bsp");
    return dateiDa(bsp) && kopieren(bsp, pfadAus(stamm + ".bhedprelight"));
}

void bauVorLichtWeg(const std::string& stamm) {
    std::error_code ec;
    fs::remove(pfadAus(stamm + ".bhedprelight"), ec);
}

bool bauVorLichtZurueck(const std::string& stamm) {
    const fs::path s = pfadAus(stamm + ".bhedprelight");
    return dateiDa(s) && verschieben(s, pfadAus(stamm + ".bsp"));
}

bool bauLichtCacheRettenDatei(const std::string& stamm, const BauLichtCache& c) {
    const fs::path neu = pfadAus(stamm + ".bhedlight.neu");
    if (!dateiDa(neu)) { return false; }
    if (bauLichtCacheRetten(c)) { return verschieben(neu, pfadAus(stamm + ".bhedlight")); }
    std::error_code ec;
    fs::remove(neu, ec);
    return false;
}

std::string befehlMitVisStand(const std::string& befehl) {
    static const std::string kSchalter = "-bhedvisstate";
    const auto w = wortGrenzen(befehl);
    if (w.size() < 2 || bauStufeAusBefehl(befehl) != BauStufe::Vis || hatWort(befehl, kSchalter)) { return befehl; }
    if (klein(befehl.substr(w[0].first, w[0].second - w[0].first)).find("bhed_q3map2") == std::string::npos) { return befehl; }
    const std::size_t letzter = w.back().first;
    return befehl.substr(0, letzter) + kSchalter + " " + befehl.substr(letzter);
}

bool befehlVisAnhaltbar(const std::string& befehl) { return bauStufeAusBefehl(befehl) == BauStufe::Vis && hatWort(befehl, "-bhedvisstate"); }

std::string visStoppDatei(const std::string& stamm) { return stamm + ".bhedvisstop"; }

std::string visStandDatei(const std::string& stamm) { return stamm + ".bhedvisstate"; }

long long visStandHoechstBytes(const std::string& prt) {
    const VisBedarf v = visBedarfAusPrt(prt);
    if (!v.ok) { return 0; }
    const long long seiten = v.portale * 2;
    const long long portalbytes = ((seiten + 63) & ~63LL) >> 3;   // vis.cpp LoadPortals
    // "BHVST1", zwei Bloecke mit Laenge (Schluessel der Schalter unter 256 Bytes, die .prt), Woerter und Anzahl
    return 6 + 4 + 256 + 4 + static_cast<long long>(prt.size()) + 8 + seiten * (4 + portalbytes);
}

}  // namespace bhed::karte
