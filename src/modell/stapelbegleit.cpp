// modell/stapelbegleit.cpp - der Modifier-Stapel in einer Begleitdatei neben der .glm (ENTSCHEID3A B12). Format und Regeln in
// bhed/modell/stapelbegleit.h.
#include "bhed/modell/stapelbegleit.h"

#include "bhed/md3.h"
#include "bhed/modell/stapel.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>

namespace bhed::modell {
namespace {

constexpr const char* kKopf = "JKSTUDIO-STACK";

// Eine Zeile ab `pos` (ohne den Umbruch); false am Ende
bool zeile(const std::string& s, std::size_t& pos, std::string& aus) {
    if (pos >= s.size()) { return false; }
    const std::size_t nl = s.find('\n', pos);
    const std::size_t ende = nl == std::string::npos ? s.size() : nl;
    aus.assign(s, pos, ende - pos);
    if (!aus.empty() && aus.back() == '\r') { aus.pop_back(); }
    pos = nl == std::string::npos ? s.size() : nl + 1U;
    return true;
}

bool zahl(const std::string& t, long long& v) {
    if (t.empty()) { return false; }
    char* ende = nullptr;
    v = std::strtoll(t.c_str(), &ende, 10);
    return ende != nullptr && *ende == '\0';
}

}  // namespace

int BegleitDaten::eintraege() const {
    int n = 0;
    for (const TeilStapel& t : teile) { n += static_cast<int>(t.stapel.size()); }
    return n;
}

std::uint64_t begleitSumme(const std::string& bytes) {
    std::uint64_t h = 1469598103934665603ULL;
    for (const char c : bytes) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

std::string stapelBegleitName(const std::string& glmPfad) { return glmPfad + kBegleitEndung; }

std::string stapelBegleitSchreiben(const Inhalt& in, const std::string& glmBytes, const std::string* basisGlm,
                                   const std::vector<std::pair<std::string, int>>* drehModi) {
    // E3: auch ohne Stapel, wenn ein Knochen einen eigenen Drehmodus hat (sonst ginge er verloren)
    const bool dreh = drehModi != nullptr && !drehModi->empty();
    if (stapelLeer(in) && !dreh) { return {}; }
    std::string s;
    char z[200];
    std::snprintf(z, sizeof z, "%s %d\nmodell %llu %016llx\n", kKopf, kBegleitFassung, static_cast<unsigned long long>(glmBytes.size()),
                  static_cast<unsigned long long>(begleitSumme(glmBytes)));
    s += z;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const Teil& t = in.teile[i];
        if (t.stapel.empty()) { continue; }
        std::snprintf(z, sizeof z, "teil %zu %zu ", i, t.stapel.size());
        s += z;
        s += t.name;
        s += '\n';
        for (const Modifikator& m : t.stapel) {
            std::snprintf(z, sizeof z, "mod %d %u %d %d %d %d %d %d\n", static_cast<int>(m.art), m.kennung, m.an ? 1 : 0, m.datei ? 1 : 0, static_cast<int>(m.achse),
                          m.vonPlus ? 1 : 0, static_cast<int>(m.stufen), m.raenderHalten ? 1 : 0);
            s += z;
        }
    }
    if (dreh) {
        for (const auto& [knochen, modus] : *drehModi) {
            std::snprintf(z, sizeof z, "dreh %d ", modus);
            s += z;
            s += knochen;
            s += '\n';
        }
    }
    if (basisGlm != nullptr && !basisGlm->empty()) {
        std::snprintf(z, sizeof z, "basis %zu\n", basisGlm->size());
        s += z;
        s += *basisGlm;
        s += '\n';
    }
    s += "ende\n";
    return s;
}

BegleitFehler stapelBegleitLesen(const std::string& datei, const std::string& glmBytes, BegleitDaten& aus) {
    aus = BegleitDaten{};
    std::size_t pos = 0;
    std::string z;
    if (!zeile(datei, pos, z)) { return BegleitFehler::Kaputt; }
    {
        std::istringstream k(z);
        std::string kopf;
        long long fassung = 0;
        if (!(k >> kopf >> fassung) || kopf != kKopf) { return BegleitFehler::Kaputt; }
        if (fassung > kBegleitFassung) { return BegleitFehler::Fassung; }
    }
    if (!zeile(datei, pos, z)) { return BegleitFehler::Kaputt; }
    {
        std::istringstream g(z);
        std::string wort;
        unsigned long long groesse = 0;
        std::string summe;
        if (!(g >> wort >> groesse >> summe) || (wort != "glm" && wort != "modell") || summe.size() != 16U) { return BegleitFehler::Kaputt; }
        char* ende = nullptr;
        aus.glmGroesse = groesse;
        aus.glmSumme = std::strtoull(summe.c_str(), &ende, 16);
        if (ende == nullptr || *ende != '\0') { return BegleitFehler::Kaputt; }
    }
    bool fertig = false;
    while (!fertig && zeile(datei, pos, z)) {
        std::istringstream w(z);
        std::string art;
        w >> art;
        if (art == "ende") {
            fertig = true;
        } else if (art == "teil") {
            long long index = -1;
            long long zahlE = -1;
            std::string ti;
            std::string tz;
            if (!(w >> ti >> tz) || !zahl(ti, index) || !zahl(tz, zahlE) || index < 0 || zahlE < 0 || zahlE > 1000) { return BegleitFehler::Kaputt; }
            BegleitDaten::TeilStapel t;
            t.index = static_cast<int>(index);
            std::getline(w >> std::ws, t.name);
            for (long long k = 0; k < zahlE; ++k) {
                if (!zeile(datei, pos, z)) { return BegleitFehler::Kaputt; }
                std::istringstream m(z);
                std::string wort;
                long long v[8]{};
                if (!(m >> wort) || wort != "mod") { return BegleitFehler::Kaputt; }
                for (long long& x : v) {
                    std::string tok;
                    if (!(m >> tok) || !zahl(tok, x)) { return BegleitFehler::Kaputt; }
                }
                if (v[0] < 0 || v[0] > 1 || v[1] <= 0 || v[4] < 0 || v[4] > 2 || v[6] < 1 || v[6] > kGlaettenStufenMax) { return BegleitFehler::Kaputt; }
                Modifikator mod;
                mod.art = static_cast<ModArt>(v[0]);
                mod.kennung = static_cast<std::uint32_t>(v[1]);
                mod.an = v[2] != 0;
                mod.datei = v[3] != 0;
                mod.achse = static_cast<std::uint8_t>(v[4]);
                mod.vonPlus = v[5] != 0;
                mod.stufen = static_cast<std::uint8_t>(v[6]);
                mod.raenderHalten = v[7] != 0;
                t.stapel.push_back(mod);
            }
            aus.teile.push_back(std::move(t));
        } else if (art == "dreh") {   // E3 (Fassung 2): Drehmodus eines Knochens
            long long modus = -1;
            std::string tm;
            std::string knochen;
            if (!(w >> tm) || !zahl(tm, modus) || modus < 0 || modus > 7) { return BegleitFehler::Kaputt; }
            std::getline(w >> std::ws, knochen);
            if (knochen.empty()) { return BegleitFehler::Kaputt; }
            aus.drehModi.emplace_back(knochen, static_cast<int>(modus));
        } else if (art == "basis") {
            long long n = -1;
            std::string tn;
            if (!(w >> tn) || !zahl(tn, n) || n <= 0 || static_cast<unsigned long long>(n) > datei.size() - pos) { return BegleitFehler::Kaputt; }
            aus.basisGlm.assign(datei, pos, static_cast<std::size_t>(n));
            pos += static_cast<std::size_t>(n);
            if (pos < datei.size() && datei[pos] == '\n') { ++pos; }
        } else {
            return BegleitFehler::Kaputt;
        }
    }
    if (!fertig || (aus.teile.empty() && aus.drehModi.empty())) { return BegleitFehler::Kaputt; }
    // die .glm, zu der der Stapel gehoert: aenderte sie jemand ausserhalb, gilt er nicht mehr
    if (aus.glmGroesse != glmBytes.size() || aus.glmSumme != begleitSumme(glmBytes)) { return BegleitFehler::Alt; }
    return BegleitFehler::Keiner;
}

BegleitFehler stapelBegleitAnwenden(const BegleitDaten& d, Inhalt& in) {
    Inhalt neu;
    if (!d.basisGlm.empty()) {
        std::string fehler;
        if (in.herkunft == Herkunft::Md3) {
            // E3: ein .md3-Requisit - die Basis ist die .md3 ohne Stapel; sie traegt fuer das naechste Speichern Bilder und Tags
            Md3Model m;
            if (!readMd3(d.basisGlm, m, &fehler)) { return BegleitFehler::Kaputt; }
            ausMd3(m, neu, 0);
            neu.md3Datei = std::make_shared<const std::string>(d.basisGlm);
        } else if (!ausGlm(d.basisGlm, neu, &fehler)) {
            return BegleitFehler::Kaputt;
        }
        // die Basis ist dasselbe Modell vor dem Stapel: dieselben Flaechen in derselben Reihenfolge
        if (neu.teile.size() != in.teile.size()) { return BegleitFehler::Flaechen; }
        for (std::size_t i = 0; i < neu.teile.size(); ++i) {
            if (neu.teile[i].name != in.teile[i].name) { return BegleitFehler::Flaechen; }
        }
        neu.kinoBelegung = in.kinoBelegung;
    } else {
        neu = in;
    }
    for (const BegleitDaten::TeilStapel& t : d.teile) {
        if (t.index < 0 || static_cast<std::size_t>(t.index) >= neu.teile.size() || neu.teile[static_cast<std::size_t>(t.index)].name != t.name) {
            return BegleitFehler::Flaechen;
        }
        neu.teile[static_cast<std::size_t>(t.index)].stapel = t.stapel;
    }
    in = std::move(neu);
    return BegleitFehler::Keiner;
}

}  // namespace bhed::modell
