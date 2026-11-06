// modell/dokument.cpp - das Modelldokument (siehe bhed/modell/dokument.h)
#include "bhed/modell/dokument.h"
#include "bhed/modell/gewichte.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>

namespace bhed::modell {
namespace {

Ecke eckeAus(const GlmVertex& v) {
    Ecke e;
    std::memcpy(e.ort, v.xyz, sizeof e.ort);
    std::memcpy(e.normale, v.normal, sizeof e.normale);
    std::memcpy(e.uv, v.st, sizeof e.uv);
    std::memcpy(e.knochen, v.bones, sizeof e.knochen);
    std::memcpy(e.gewicht, v.weights, sizeof e.gewicht);
    return e;
}

GlmVertex vertexAus(const Ecke& e) {
    GlmVertex v;
    std::memcpy(v.xyz, e.ort, sizeof v.xyz);
    std::memcpy(v.normal, e.normale, sizeof v.normal);
    std::memcpy(v.st, e.uv, sizeof v.st);
    std::memcpy(v.bones, e.knochen, sizeof v.bones);
    std::memcpy(v.weights, e.gewicht, sizeof v.weights);
    return v;
}

std::vector<Ecke> eckenAus(const std::vector<GlmVertex>& v) {
    std::vector<Ecke> e;
    e.reserve(v.size());
    for (const GlmVertex& x : v) { e.push_back(eckeAus(x)); }
    return e;
}

// Der Ort einer Ecke bitweise als Schluessel: Ecken an einer UV-Naht tragen
// denselben Ort (gleiche float-Bits), der Punkt ist derselbe.
struct OrtSchluessel {
    std::uint32_t b[3];
    bool operator==(const OrtSchluessel& o) const noexcept { return b[0] == o.b[0] && b[1] == o.b[1] && b[2] == o.b[2]; }
};
struct OrtHash {
    std::size_t operator()(const OrtSchluessel& k) const noexcept {
        std::size_t h = k.b[0];
        h = h * 1000003U ^ k.b[1];
        h = h * 1000003U ^ k.b[2];
        return h;
    }
};

// Die Kinder aus den Eltern, in der Reihenfolge der Flaechen.
void kinderAusEltern(Inhalt& in) {
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const int p = in.teile[i].eltern;
        if (p >= 0 && static_cast<std::size_t>(p) < in.teile.size() && static_cast<std::size_t>(p) != i) {
            in.teile[static_cast<std::size_t>(p)].kinder.push_back(static_cast<int>(i));
        }
    }
}

}  // namespace

void Netz::aufbauen(std::vector<Ecke> e, const std::vector<std::uint32_t>& indizes) {
    ecken = std::move(e);
    // Punkte: gleicher Ort, gleicher Punkt (in der Reihenfolge der Ecken nummeriert).
    punktVon.assign(ecken.size(), -1);
    punktZahl = 0;
    std::unordered_map<OrtSchluessel, int, OrtHash> orte;
    orte.reserve(ecken.size());
    for (std::size_t i = 0; i < ecken.size(); ++i) {
        OrtSchluessel k{};
        std::memcpy(k.b, ecken[i].ort, sizeof k.b);
        const auto [it, neu] = orte.emplace(k, punktZahl);
        if (neu) { ++punktZahl; }
        punktVon[i] = it->second;
    }
    const std::size_t dreiecke = indizes.size() / 3U;
    kanten.assign(dreiecke * 3U, Halbkante{});
    for (std::size_t t = 0; t < dreiecke; ++t) {
        for (std::size_t k = 0; k < 3; ++k) {
            const std::uint32_t idx = indizes[t * 3U + k];
            Halbkante& h = kanten[t * 3U + k];
            h.ecke = idx < ecken.size() ? static_cast<int>(idx) : 0;
            h.punkt = ecken.empty() ? -1 : punktVon[static_cast<std::size_t>(h.ecke)];
            h.naechste = static_cast<int>(t * 3U + (k + 1U) % 3U);
        }
    }
    // Gegenkanten: a->b trifft b->a. Haengen an einer Kante mehr als zwei
    // Dreiecke (oder zwei gleich gerichtete), bleibt der Rest offen.
    nichtMannigfaltig = 0;
    std::unordered_map<std::uint64_t, std::vector<int>> offen;
    offen.reserve(kanten.size());
    const auto schluessel = [](int a, int b) {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(a)) << 32U) | static_cast<std::uint32_t>(b);
    };
    for (std::size_t h = 0; h < kanten.size(); ++h) {
        const int a = kanten[h].punkt;
        const int b = kanten[static_cast<std::size_t>(kanten[h].naechste)].punkt;
        // Entartetes Dreieck (zwei Ecken auf einem Punkt): keine Nachbarn - sonst
        // traefe a->b die Kante b->a desselben Dreiecks.
        const std::size_t t0 = h - h % 3U;
        const int p0 = kanten[t0].punkt;
        const int p1 = kanten[t0 + 1U].punkt;
        const int p2 = kanten[t0 + 2U].punkt;
        if (a < 0 || p0 == p1 || p1 == p2 || p0 == p2) { continue; }
        auto it = offen.find(schluessel(b, a));
        if (it != offen.end() && !it->second.empty()) {
            const int g = it->second.back();
            it->second.pop_back();
            kanten[h].gegen = g;
            kanten[static_cast<std::size_t>(g)].gegen = static_cast<int>(h);
        } else {
            offen[schluessel(a, b)].push_back(static_cast<int>(h));
        }
    }
    // Nicht mannigfaltig: eine offen gebliebene Kante, deren Punktpaar noch
    // eine andere Kante traegt (drittes Dreieck oder gleiche Richtung).
    std::unordered_map<std::uint64_t, int> paare;
    const auto ungerichtet = [&](std::size_t h) {
        const int a = kanten[h].punkt;
        const int b = kanten[static_cast<std::size_t>(kanten[h].naechste)].punkt;
        return schluessel(std::min(a, b), std::max(a, b));
    };
    for (std::size_t h = 0; h < kanten.size(); ++h) { ++paare[ungerichtet(h)]; }
    for (std::size_t h = 0; h < kanten.size(); ++h) {
        if (kanten[h].gegen < 0 && paare[ungerichtet(h)] >= 2) { ++nichtMannigfaltig; }
    }
}

std::vector<std::uint32_t> Netz::indizes() const {
    std::vector<std::uint32_t> out;
    out.reserve(kanten.size());
    for (const Halbkante& h : kanten) { out.push_back(static_cast<std::uint32_t>(h.ecke)); }
    return out;
}

int Netz::randKanten() const noexcept {
    int n = 0;
    for (const Halbkante& h : kanten) {
        if (h.gegen < 0) { ++n; }
    }
    return n;
}

std::vector<char> Netz::randEcken() const {
    // Beide Enden jeder offenen Kante - und alle Ecken auf denselben Punkten.
    std::vector<char> punkt(static_cast<std::size_t>(std::max(punktZahl, 0)), 0);
    for (const Halbkante& h : kanten) {
        if (h.gegen >= 0 || h.punkt < 0) { continue; }
        punkt[static_cast<std::size_t>(h.punkt)] = 1;
        punkt[static_cast<std::size_t>(kanten[static_cast<std::size_t>(h.naechste)].punkt)] = 1;
    }
    std::vector<char> rand(ecken.size(), 0);
    for (std::size_t i = 0; i < ecken.size(); ++i) { rand[i] = punkt[static_cast<std::size_t>(punktVon[i])]; }
    return rand;
}

bool Netz::pruefen(std::string* fehler) const {
    const auto falsch = [&](const std::string& was) {
        if (fehler != nullptr) { *fehler = was; }
        return false;
    };
    if (kanten.size() % 3U != 0U) { return falsch("Kantenzahl nicht durch 3 teilbar"); }
    if (punktVon.size() != ecken.size()) { return falsch("punktVon passt nicht zu den Ecken"); }
    const int n = static_cast<int>(kanten.size());
    for (int h = 0; h < n; ++h) {
        const Halbkante& k = kanten[static_cast<std::size_t>(h)];
        if (k.ecke < 0 || static_cast<std::size_t>(k.ecke) >= ecken.size()) { return falsch("Kante " + std::to_string(h) + ": Ecke ungueltig"); }
        if (k.punkt != punktVon[static_cast<std::size_t>(k.ecke)]) { return falsch("Kante " + std::to_string(h) + ": Punkt passt nicht zur Ecke"); }
        if (k.naechste < 0 || k.naechste >= n || k.naechste / 3 != h / 3) { return falsch("Kante " + std::to_string(h) + ": naechste nicht im Dreieck"); }
        const Halbkante& k2 = kanten[static_cast<std::size_t>(k.naechste)];
        const Halbkante& k3 = kanten[static_cast<std::size_t>(k2.naechste)];
        if (k3.naechste != h) { return falsch("Kante " + std::to_string(h) + ": naechste^3 ist nicht sie selbst"); }
        if (k.gegen >= 0) {
            if (k.gegen >= n) { return falsch("Kante " + std::to_string(h) + ": Gegenkante ungueltig"); }
            const Halbkante& g = kanten[static_cast<std::size_t>(k.gegen)];
            if (g.gegen != h) { return falsch("Kante " + std::to_string(h) + ": gegen(gegen) ist nicht sie selbst"); }
            if (g.punkt != k2.punkt || kanten[static_cast<std::size_t>(g.naechste)].punkt != k.punkt) {
                return falsch("Kante " + std::to_string(h) + ": Gegenkante laeuft nicht umgekehrt");
            }
            if (k.gegen / 3 == h / 3) { return falsch("Kante " + std::to_string(h) + ": Gegenkante im selben Dreieck"); }
        }
    }
    return true;
}

int Inhalt::teilIndex(const std::string& n) const {
    for (std::size_t i = 0; i < teile.size(); ++i) {
        if (teile[i].name == n) { return static_cast<int>(i); }
    }
    return -1;
}

bool ausGlm(const std::string& bytes, Inhalt& out, std::string* fehler) {
    out = Inhalt{};
    GlmModel m0;
    if (!readGlm(bytes, m0, fehler, 0)) { return false; }
    out.herkunft = Herkunft::Glm;
    out.name = m0.name;
    out.glaPfad = m0.animFile;
    out.knochenZahl = m0.numBones;
    out.stufenZahl = std::max(m0.numLods, 1);
    out.jk2Umgewandelt = m0.jk2Umgewandelt;
    out.jkaUmgewandelt = m0.jkaUmgewandelt;
    out.teile.resize(m0.surfaces.size());
    for (std::size_t i = 0; i < m0.surfaces.size(); ++i) {
        const GlmSurface& s = m0.surfaces[i];
        Teil& t = out.teile[i];
        t.name = s.name;
        t.shader = s.shader;
        t.flaggen = s.flags;
        t.eltern = s.parent;
        t.stufen.resize(static_cast<std::size_t>(out.stufenZahl));
        t.stufen[0].aufbauen(eckenAus(s.verts), s.indexes);
    }
    kinderAusEltern(out);
    for (int stufe = 1; stufe < out.stufenZahl; ++stufe) {
        GlmModel ms;
        std::string f;
        if (!readGlm(bytes, ms, &f, stufe) || ms.surfaces.size() != out.teile.size()) {
            // Eine abgeschnittene Stufe: das Modell bis hierher behalten.
            out.stufenZahl = stufe;
            for (Teil& t : out.teile) { t.stufen.resize(static_cast<std::size_t>(stufe)); }
            if (fehler != nullptr) { *fehler = f; }
            break;
        }
        for (std::size_t i = 0; i < ms.surfaces.size(); ++i) {
            out.teile[i].stufen[static_cast<std::size_t>(stufe)].aufbauen(eckenAus(ms.surfaces[i].verts), ms.surfaces[i].indexes);
        }
    }
    return true;
}

void ausGlmModell(const GlmModel& m, Inhalt& out) {
    out = Inhalt{};
    out.herkunft = Herkunft::Glm;
    out.name = m.name;
    out.glaPfad = m.animFile;
    out.knochenZahl = m.numBones;
    out.stufenZahl = 1;
    out.jk2Umgewandelt = m.jk2Umgewandelt;
    out.jkaUmgewandelt = m.jkaUmgewandelt;
    out.teile.resize(m.surfaces.size());
    for (std::size_t i = 0; i < m.surfaces.size(); ++i) {
        const GlmSurface& s = m.surfaces[i];
        Teil& t = out.teile[i];
        t.name = s.name;
        t.shader = s.shader;
        t.flaggen = s.flags;
        t.eltern = s.parent;
        t.stufen.resize(1);
        t.stufen[0].aufbauen(eckenAus(s.verts), s.indexes);
    }
    kinderAusEltern(out);
}

void ausMd3(const Md3Model& m, Inhalt& out, int bild) {
    out = Inhalt{};
    out.herkunft = Herkunft::Md3;
    out.name = m.name;
    out.stufenZahl = 1;
    for (const Md3Surface& s : m.surfaces) {
        Teil t;
        t.name = s.name;
        t.shader = s.shader;
        const Md3Vertex* v = s.frame(bild);
        std::vector<Ecke> e;
        if (v != nullptr) {
            e.resize(static_cast<std::size_t>(s.numVerts));
            for (int i = 0; i < s.numVerts; ++i) {
                Ecke& x = e[static_cast<std::size_t>(i)];
                std::memcpy(x.ort, v[i].xyz, sizeof x.ort);
                std::memcpy(x.normale, v[i].normal, sizeof x.normale);
                std::memcpy(x.uv, v[i].st, sizeof x.uv);
                x.gewicht[0] = 1.0F;   // starr: alles an Knochen 0
            }
        }
        t.stufen.resize(1);
        t.stufen[0].aufbauen(std::move(e), s.indexes);
        out.teile.push_back(std::move(t));
    }
}

GlmModel zuGlm(const Inhalt& in, int stufe) {
    GlmModel m;
    m.name = in.name;
    m.animFile = in.glaPfad;
    m.numBones = in.knochenZahl;
    m.numLods = in.herkunft == Herkunft::Glm ? in.stufenZahl : 0;
    m.jk2Umgewandelt = in.jk2Umgewandelt;
    m.jkaUmgewandelt = in.jkaUmgewandelt;
    const std::size_t s = static_cast<std::size_t>(std::clamp(stufe, 0, std::max(in.stufenZahl - 1, 0)));
    m.surfaces.resize(in.teile.size());
    bool erste = true;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const Teil& t = in.teile[i];
        GlmSurface& g = m.surfaces[i];
        g.name = t.name;
        g.shader = t.shader;
        g.flags = t.flaggen;
        g.parent = t.eltern;
        if (s >= t.stufen.size()) { continue; }
        const Netz& n = t.stufen[s];
        g.verts.reserve(n.ecken.size());
        for (std::size_t ei = 0; ei < n.ecken.size(); ++ei) {
            const Ecke& e = n.ecken[ei];
            g.verts.push_back(vertexAus(e));
            // 1a-2: waehrend des Malens zeichnet die Figur, was das Spiel aus der Malliste machte.
            if (ei < n.mal.size()) {
                Ecke spiel = e;
                if (aufEcke(n.mal[ei], spiel)) { g.verts.back() = vertexAus(spiel); }
            }
            // Grenzen wie readGlm: ueber alle Ecken aller Flaechen der Stufe.
            for (int k = 0; k < 3; ++k) {
                if (erste) {
                    m.mins[k] = m.maxs[k] = e.ort[k];
                } else {
                    m.mins[k] = std::min(m.mins[k], e.ort[k]);
                    m.maxs[k] = std::max(m.maxs[k], e.ort[k]);
                }
            }
            erste = false;
        }
        g.indexes = n.indizes();
        // readGlm liest zu Stufe 0 die groebste mit (Spuren mit traceLod 10).
        if (s == 0 && in.herkunft == Herkunft::Glm && in.stufenZahl > 1 && in.stufenZahl <= 16 && t.stufen.size() == static_cast<std::size_t>(in.stufenZahl)) {
            const Netz& grob = t.stufen.back();
            g.grobVerts.reserve(grob.ecken.size());
            for (const Ecke& e : grob.ecken) { g.grobVerts.push_back(vertexAus(e)); }
            g.grobIndexes = grob.indizes();
        }
    }
    return m;
}

void Dokument::laden(Inhalt neu) {
    jetzt_ = std::move(neu);
    zurueck_.clear();
    vor_.clear();
    ++stand_;
}

void Dokument::aendern(const std::string& was, const std::function<void(Inhalt&)>& f) {
    zurueck_.emplace_back(was, jetzt_);
    if (zurueck_.size() > kTiefe) { zurueck_.erase(zurueck_.begin()); }
    vor_.clear();
    f(jetzt_);
    ++stand_;
}

void Dokument::vorschau(const std::function<void(Inhalt&)>& f) {
    f(jetzt_);
    ++stand_;
}

bool Dokument::rueckgaengig() {
    if (zurueck_.empty()) { return false; }
    vor_.emplace_back(zurueck_.back().first, std::move(jetzt_));
    jetzt_ = std::move(zurueck_.back().second);
    zurueck_.pop_back();
    ++stand_;
    return true;
}

bool Dokument::abbrechen() {
    if (zurueck_.empty()) { return false; }
    jetzt_ = std::move(zurueck_.back().second);
    zurueck_.pop_back();
    ++stand_;
    return true;
}

bool Dokument::wiederholen() {
    if (vor_.empty()) { return false; }
    zurueck_.emplace_back(vor_.back().first, std::move(jetzt_));
    jetzt_ = std::move(vor_.back().second);
    vor_.pop_back();
    ++stand_;
    return true;
}

}  // namespace bhed::modell
