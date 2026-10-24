// app_karte_cam.cpp - Karten-Editor: die 3D-Kamera (CamWnd).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/camwindow.cpp (CamWnd:
// Cam_Draw, die Darstellungsarten, Freelook mit FreezePointer, Rechtsklick-
// Uhr und Kontextmenue-Versuch, Mausrad, Camera_keyMove, die KeyEvents und
// diskreten Befehle, CameraFocusOnSelected, CenterView, Up/DownFloor, Cubic
// Clip, CameraModeNext/Prev, CameraSpeedInc/Dec, ShowStats, ShowWorkzone3d,
// ShowSize3d, das Fadenkreuz im Freelook, CamDrawSize), radiant/renderstate.cpp
// (die Zustaende $CAM_HIGHLIGHT, $CAM_OVERLAY, $CAM_WIRE, $CAM_WORKZONE und
// die aus Shadern: qer_trans, qer_alphafunc, cull, skyparms; die
// Sortierung), radiant/textures.cpp (LoadTextureRGBA: Durchschnittsfarbe,
// Mipmaps, anisotrop; die Skybox als Wuerfel), radiant/texmanip.cpp
// (R_ResampleTexture), plugins/shaders/shaders.cpp (notex.png /
// shadernotex.png), libs/gtkutil/cursor.h (FreezePointer),
// libs/gtkutil/accelerator.cpp (PressedKeys) und
// plugins/entity/namedentity.h (Namen in 3D bis 512 Einheiten).
// Die Rechnung steht in bhed/karte/kamera.h und kameraszene.h.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/image.h"
#include "bhed/karte/kameraszene.h"
#include "bhed/karte/manipulator.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "karte_gpu.h"
#include "tastatur.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

namespace bhed::gui::karteneditor {

namespace {

using karte::Farbe;
using karte::Kasten;
using karte::Vektor3f;

Editor& e() { return editor(); }

std::string klein(const std::string& s) {
    std::string r = s;
    for (char& c : r) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return r;
}

// --- Bilder (qtexture_t) ---------------------------------------------------------

struct Bild {
    bool fertig = false;
    void* srv = nullptr;        // texture_number
    void* himmel = nullptr;     // m_textureSkyBox (0 = nicht geladen)
    std::array<float, 3> farbe{1, 1, 1};   // qtexture_t::color
};

struct Auftrag {
    std::string schluessel;
    std::string bildName;   // m_textureName (qer_editorimage oder der Name)
    std::string himmel;     // m_skyBox
    bool standard = false;  // IsDefault: keine Shaderdefinition
    unsigned generation = 0;
};

struct Ergebnis {
    std::string schluessel;
    unsigned generation = 0;
    image::Image bild;
    std::array<float, 3> farbe{1, 1, 1};
    int himmelGroesse = 0;
    std::array<std::vector<unsigned char>, 6> seiten;
};

// R_ResampleTexture (texmanip.cpp:39-214), 4 Bytes je Punkt.
void lerpZeile(const unsigned char* in, unsigned char* out, int inB, int outB) {
    int alt = 0;
    const int schritt = static_cast<int>(static_cast<float>(inB) * 65536.0F / static_cast<float>(outB));
    const int ende = inB - 1;
    for (int j = 0, f = 0; j < outB; ++j, f += schritt) {
        const int xi = f >> 16;
        if (xi != alt) {
            in += (xi - alt) * 4;
            alt = xi;
        }
        if (xi < ende) {
            const int lerp = f & 0xFFFF;
            for (int k = 0; k < 4; ++k) { *out++ = static_cast<unsigned char>((((in[4 + k] - in[k]) * lerp) >> 16) + in[k]); }
        } else {   // last pixel of the line has no pixel to lerp to
            for (int k = 0; k < 4; ++k) { *out++ = in[k]; }
        }
    }
}

std::vector<unsigned char> neuAbtasten(const unsigned char* in, int inB, int inH, int outB, int outH) {
    std::vector<unsigned char> out(static_cast<std::size_t>(outB) * static_cast<std::size_t>(outH) * 4U);
    std::vector<unsigned char> z1(static_cast<std::size_t>(outB) * 4U);
    std::vector<unsigned char> z2(static_cast<std::size_t>(outB) * 4U);
    const int ende = inH - 1;
    const int b4 = inB * 4;
    const int schritt = static_cast<int>(static_cast<float>(inH) * 65536.0F / static_cast<float>(outH));
    const unsigned char* zeile = in;
    int alt = 0;
    lerpZeile(zeile, z1.data(), inB, outB);
    lerpZeile(zeile + (inH > 1 ? b4 : 0), z2.data(), inB, outB);
    unsigned char* o = out.data();
    for (int i = 0, f = 0; i < outH; ++i, f += schritt) {
        const int yi = f >> 16;
        if (yi < ende) {
            const int lerp = f & 0xFFFF;
            if (yi != alt) {
                zeile = in + (static_cast<std::ptrdiff_t>(b4) * yi);
                if (yi == alt + 1) {
                    z1 = z2;
                } else {
                    lerpZeile(zeile, z1.data(), inB, outB);
                }
                lerpZeile(zeile + b4, z2.data(), inB, outB);
                alt = yi;
            }
            for (int k = 0; k < outB * 4; ++k) {
                *o++ = static_cast<unsigned char>((((z2[static_cast<std::size_t>(k)] - z1[static_cast<std::size_t>(k)]) * lerp) >> 16) +
                                                  z1[static_cast<std::size_t>(k)]);
            }
        } else {
            if (yi != alt) {
                zeile = in + (static_cast<std::ptrdiff_t>(b4) * yi);
                if (yi == alt + 1) {
                    z1 = z2;
                } else {
                    lerpZeile(zeile, z1.data(), inB, outB);
                }
                alt = yi;
            }
            std::copy(z1.begin(), z1.end(), o);
            o += outB * 4;
        }
    }
    return out;
}

// QERApp_LoadImage: Bildmodule alphabetisch (jpg, png, tga), das erste
// gefundene gilt.
image::Image bildLaden(const karte::Dateisystem& fs, const std::string& name) {
    for (const char* typ : {"jpg", "png", "tga"}) {
        std::string daten;
        if (fs.dateiLesen(name + '.' + typ, daten)) {
            return image::decode(reinterpret_cast<const unsigned char*>(daten.data()), daten.size());
        }
    }
    return {};
}

image::Image dateiLaden(const std::string& pfad) {
    std::FILE* f = std::fopen(pfad.c_str(), "rb");
    if (f == nullptr) { return {}; }
    std::vector<unsigned char> d;
    unsigned char puffer[65536];
    std::size_t n = 0;
    while ((n = std::fread(puffer, 1, sizeof(puffer), f)) > 0) { d.insert(d.end(), puffer, puffer + n); }
    std::fclose(f);
    return image::decode(d.data(), d.size());
}

class Bildlager {
public:
    ~Bildlager() { anhalten(); }

    // Den Shader anfordern; nullptr, solange kein Spiel geladen ist.
    const Bild* holen(const std::string& shader) {
        Editor& ed = e();
        if (!ed.karteSpiel) { return nullptr; }
        pruefen();
        const std::string schl = klein(shader);
        const auto it = m_bilder.find(schl);
        if (it != m_bilder.end()) { return &it->second; }
        Bild& b = m_bilder[schl];
        const karte::ShaderEintrag* s = ed.karteSpiel->inventar.shaderFinden(shader);
        Auftrag a;
        a.schluessel = schl;
        a.bildName = s != nullptr ? s->texturName : shader;
        a.himmel = s != nullptr ? s->himmel : std::string();
        a.standard = s == nullptr;
        {
            std::lock_guard<std::mutex> l(m_sperre);
            a.generation = m_generation;
            m_auftraege.push_back(std::move(a));
            ++m_offen;
        }
        starten();
        m_cv.notify_one();
        return &b;
    }

    // Fertige Bilder auf die Grafikkarte (nur im Hauptfaden).
    void abholen() {
        pruefen();
        {   // SetTexParameters / SetTexAnisotropy nach den Einstellungen (auch nach einem neuen Geraet)
            Editor& ed = e();
            static std::size_t geraet = static_cast<std::size_t>(-1);
            if (!ed.abtasterGesetzt || geraet != kartegpu::geraetStand()) {
                kartegpu::abtasterSetzen(ed.texQual.modus, ed.texQual.anisotropie);
                ed.abtasterGesetzt = true;
                geraet = kartegpu::geraetStand();
            }
        }
        std::vector<Ergebnis> fertig;
        {
            std::lock_guard<std::mutex> l(m_sperre);
            fertig.swap(m_ergebnisse);
        }
        for (Ergebnis& r : fertig) {
            if (r.generation != m_generation) { continue; }
            const auto it = m_bilder.find(r.schluessel);
            if (it == m_bilder.end()) { continue; }
            Bild& b = it->second;
            b.farbe = r.farbe;
            if (r.bild.ok) {   // LoadTextureRGBA: Gamma der Einstellung, GL_TEXTURE_BASE_LEVEL
                const Editor& ed = e();
                if (ed.texQual.gamma != 1.0F) {
                    const auto tabelle = karte::gammaTabelle(ed.texQual.gamma);
                    auto& p = r.bild.rgba;
                    for (std::size_t i = 0; i + 3 < p.size(); i += 4) {
                        for (std::size_t j = 0; j < 3; ++j) { p[i + j] = tabelle[p[i + j]]; }
                    }
                }
                b.srv = kartegpu::bildAnlegen(r.bild.rgba.data(), r.bild.width, r.bild.height, ed.texQual.mipStufe);
            }
            if (r.himmelGroesse > 0) {
                const unsigned char* s[6];
                for (int i = 0; i < 6; ++i) { s[i] = r.seiten[static_cast<std::size_t>(i)].data(); }
                b.himmel = kartegpu::wuerfelAnlegen(s, r.himmelGroesse);
            }
            b.fertig = true;
            std::lock_guard<std::mutex> l(m_sperre);
            if (m_offen > 0) { --m_offen; }
        }
    }

    [[nodiscard]] bool fertig() const {
        std::lock_guard<std::mutex> l(m_sperre);
        return m_offen == 0;
    }

    // RefreshShaders: die Bilder neu laden (unrealise/realise der Texturen).
    void neuLaden() {
        m_neu = true;
        pruefen();
    }

private:
    // Neues Spiel oder neues Grafikgeraet: alles vergessen.
    void pruefen() {
        Editor& ed = e();
        if (!m_neu && ed.karteSpiel == m_spiel && kartegpu::geraetStand() == m_geraet) { return; }
        m_neu = false;
        const bool gleichesGeraet = kartegpu::geraetStand() == m_geraet;
        for (auto& [n, b] : m_bilder) {
            (void)n;
            if (gleichesGeraet) {
                kartegpu::bildFreigeben(b.srv);
                kartegpu::bildFreigeben(b.himmel);
            }
        }
        m_bilder.clear();
        std::lock_guard<std::mutex> l(m_sperre);
        ++m_generation;
        m_auftraege.clear();
        m_ergebnisse.clear();
        m_offen = 0;
        m_spiel = ed.karteSpiel;
        m_geraet = kartegpu::geraetStand();
        m_ersatzOrdner = directoryOf(g_app->dataDir) + "/karte/bitmaps/";
    }

    void starten() {
        if (!m_faeden.empty()) { return; }
        const unsigned n = std::clamp(std::thread::hardware_concurrency(), 2U, 4U) - 1U;
        for (unsigned i = 0; i < n; ++i) {
            m_faeden.emplace_back([this] { arbeiten(); });
        }
    }

    void anhalten() {
        {
            std::lock_guard<std::mutex> l(m_sperre);
            m_ende = true;
        }
        m_cv.notify_all();
        for (std::thread& t : m_faeden) { t.join(); }
        m_faeden.clear();
    }

    void arbeiten() {
        for (;;) {
            Auftrag a;
            std::shared_ptr<Spielkontext> spiel;
            std::string ersatz;
            {
                std::unique_lock<std::mutex> l(m_sperre);
                m_cv.wait(l, [this] { return m_ende || !m_auftraege.empty(); });
                if (m_ende) { return; }
                a = std::move(m_auftraege.front());
                m_auftraege.pop_front();
                spiel = m_spiel;
                ersatz = m_ersatzOrdner;
            }
            Ergebnis r;
            r.schluessel = a.schluessel;
            r.generation = a.generation;
            if (spiel) {
                r.bild = bildLaden(spiel->fs, a.bildName);
                if (!r.bild.ok || r.bild.width <= 0 || r.bild.height <= 0) {
                    // shaders.cpp:953-962: notex.png ohne Definition, sonst shadernotex.png
                    r.bild = dateiLaden(ersatz + (a.standard ? "notex.png" : "shadernotex.png"));
                }
                if (r.bild.ok) {
                    // LoadTextureRGBA: Durchschnitt vor der Gamma-Korrektur
                    double summe[3] = {0, 0, 0};
                    const std::size_t n = static_cast<std::size_t>(r.bild.width) * static_cast<std::size_t>(r.bild.height);
                    for (std::size_t i = 0; i < n; ++i) {
                        for (std::size_t j = 0; j < 3; ++j) { summe[j] += r.bild.rgba[(i * 4) + j]; }
                    }
                    for (std::size_t j = 0; j < 3; ++j) { r.farbe[j] = static_cast<float>(summe[j] / (static_cast<double>(n) * 255.0)); }
                }
                if (!a.himmel.empty()) {
                    // qtexture_realise: _ft _bk _up _dn _rt _lf = +X -X +Y -Y +Z -Z,
                    // alle sechs noetig, auf die groesste Seite gebracht.
                    static const char* const endung[6] = {"_ft", "_bk", "_up", "_dn", "_rt", "_lf"};
                    std::array<image::Image, 6> s;
                    bool alle = true;
                    int groesse = 0;
                    for (std::size_t i = 0; i < 6; ++i) {
                        s[i] = bildLaden(spiel->fs, a.himmel + endung[i]);
                        alle = alle && s[i].ok && s[i].width > 0 && s[i].height > 0;
                        if (s[i].ok) { groesse = std::max({groesse, s[i].width, s[i].height}); }
                    }
                    if (alle) {
                        r.himmelGroesse = groesse;
                        for (std::size_t i = 0; i < 6; ++i) {
                            if (s[i].width != groesse || s[i].height != groesse) {
                                r.seiten[i] = neuAbtasten(s[i].rgba.data(), s[i].width, s[i].height, groesse, groesse);
                            } else {
                                r.seiten[i] = std::move(s[i].rgba);
                            }
                        }
                    }
                }
            }
            std::lock_guard<std::mutex> l(m_sperre);
            if (r.generation == m_generation) {
                m_ergebnisse.push_back(std::move(r));
            }
        }
    }

    std::map<std::string, Bild> m_bilder;
    std::shared_ptr<Spielkontext> m_spiel;
    unsigned m_geraet = ~0U;
    std::string m_ersatzOrdner;
    mutable std::mutex m_sperre;
    std::condition_variable m_cv;
    std::deque<Auftrag> m_auftraege;
    std::vector<Ergebnis> m_ergebnisse;
    std::size_t m_offen = 0;
    unsigned m_generation = 0;
    std::vector<std::thread> m_faeden;
    bool m_ende = false;
    bool m_neu = false;
};

Bildlager& lager() {
    static Bildlager l;
    return l;
}

// --- Szene auf der Grafikkarte ----------------------------------------------------

struct Bereich {
    std::size_t erstes = 0;
    std::size_t anzahl = 0;
};

struct Stapel3 {
    karte::StapelArt art = karte::StapelArt::Shader;
    std::string shader;
    Farbe farbe = 0xFFFFFFFFU;
    Bereich dreiecke;
    Bereich kanten;
    Bereich kantenGewaehlt;
    Bereich linien;
    std::size_t objekte = 0;      // ShowStats
    std::size_t transforms = 0;
};

struct Szene3 {
    std::size_t stand = static_cast<std::size_t>(-1);
    bool mitKanten = false;
    std::vector<Stapel3> stapel;
    std::vector<kartegpu::Punkt3> dreieckDaten;
    std::vector<kartegpu::LinienPunkt> linienDaten;
    kartegpu::Puffer3 dreiecke;
    kartegpu::Puffer3 linien;
    bool gpuAktuell = false;
    std::vector<kartegpu::Punkt3> tint;               // $CAM_HIGHLIGHT
    std::vector<kartegpu::LinienPunkt> tintKanten;    // ... im Drahtmodus (Polygon als Linien)
    std::vector<kartegpu::LinienPunkt> tintLinien;    // ... GL_LINES
    std::vector<kartegpu::LinienPunkt> overlay;       // $CAM_OVERLAY
    std::vector<karte::KameraName> namen;
    std::size_t dreieckAnzahl = 0;
    std::size_t linienPunkte = 0;     // hochgeladene Linienpunkte (die Kopie ist danach frei)
    std::size_t tintObjekte = 0;      // ShowStats
    std::size_t tintTransforms = 0;
    std::size_t overlayObjekte = 0;
    std::size_t overlayTransforms = 0;
    std::size_t wahlPrimitive = 0;
};
// Punkt 12: zwei Teile - die Geometrie (Pinsel und Patches, nur bei einer
// Aenderung der Karte neu) und der Rest (Punktentitaeten, Namen, Auswahl).
Szene3 g_geo;
Szene3 g_szene;

kartegpu::Punkt3 punkt3(const karte::KameraPunkt& p, bool weiss) {
    kartegpu::Punkt3 q{};
    q.pos[0] = p.pos[0];
    q.pos[1] = p.pos[1];
    q.pos[2] = p.pos[2];
    q.normale[0] = p.normale[0];
    q.normale[1] = p.normale[1];
    q.normale[2] = p.normale[2];
    q.st[0] = p.st[0];
    q.st[1] = p.st[1];
    q.farbe = weiss ? 0xFFFFFFFFU : p.farbe;
    return q;
}

void kanteDazu(const karte::KameraKante& k, bool weiss, std::vector<kartegpu::LinienPunkt>& aus) {
    kartegpu::LinienPunkt a{};
    a.pos[0] = k.a[0];
    a.pos[1] = k.a[1];
    a.pos[2] = k.a[2];
    a.farbe = weiss ? 0xFFFFFFFFU : k.fa;
    a.extra[0] = k.c[0];
    a.extra[1] = k.c[1];
    a.extra[2] = k.c[2];
    a.extra[3] = k.polygon ? 1.0F : 0.0F;
    kartegpu::LinienPunkt b = a;
    b.pos[0] = k.b[0];
    b.pos[1] = k.b[1];
    b.pos[2] = k.b[2];
    b.farbe = weiss ? 0xFFFFFFFFU : k.fb;
    aus.push_back(a);
    aus.push_back(b);
}

void szeneBauen(Szene3& s, karte::KameraTeil teil, bool mitKanten) {
    Editor& ed = e();
    karte::KameraSzenenOptionen o;
    o.teil = teil;
    o.kanten = mitKanten;
    o.winkel = ed.anzeige.winkel;
    o.namen = ed.anzeige.namen;
    o.lichtRadien = ed.anzeige.lichtRadien;
    o.zielnamen = ed.ent.zielnamen;
    karte::KameraSzene roh = karte::kameraSzeneBauen(ed.karte, o);
    s.stapel.clear();
    s.dreieckDaten.clear();
    s.linienDaten.clear();
    s.mitKanten = mitKanten;
    for (karte::KameraStapel& st : roh.stapel) {
        Stapel3 z;
        z.art = st.art;
        z.shader = st.shader;
        z.farbe = st.farbe;
        z.objekte = st.objekte;
        z.transforms = st.transforms;
        z.dreiecke.erstes = s.dreieckDaten.size();
        for (const karte::KameraPunkt& p : st.dreiecke) { s.dreieckDaten.push_back(punkt3(p, false)); }
        z.dreiecke.anzahl = s.dreieckDaten.size() - z.dreiecke.erstes;
        const auto bereich = [&](const std::vector<karte::KameraKante>& v, Bereich& b) {
            b.erstes = s.linienDaten.size();
            for (const karte::KameraKante& k : v) { kanteDazu(k, false, s.linienDaten); }
            b.anzahl = s.linienDaten.size() - b.erstes;
        };
        bereich(st.kanten, z.kanten);
        bereich(st.kantenGewaehlt, z.kantenGewaehlt);
        bereich(st.linien, z.linien);
        s.stapel.push_back(std::move(z));
        st.dreiecke = {};
        st.kanten = {};
        st.kantenGewaehlt = {};
    }
    s.tint.clear();
    for (const karte::KameraPunkt& p : roh.wahlDreiecke) { s.tint.push_back(punkt3(p, true)); }
    s.tintKanten.clear();
    s.overlay.clear();
    for (const karte::KameraKante& k : roh.wahlKanten) {
        kanteDazu(k, true, s.overlay);
        if (k.polygon) { kanteDazu(k, true, s.tintKanten); }
    }
    s.tintLinien.clear();
    for (const karte::KameraKante& k : roh.wahlTintLinien) { kanteDazu(k, true, s.tintLinien); }
    s.namen = std::move(roh.namen);
    s.dreieckAnzahl = roh.dreieckAnzahl;
    s.linienPunkte = s.linienDaten.size();
    s.tintObjekte = roh.tintObjekte;
    s.tintTransforms = roh.tintTransforms;
    s.overlayObjekte = roh.overlayObjekte;
    s.overlayTransforms = roh.overlayTransforms;
    s.wahlPrimitive = roh.wahlPrimitive;
    s.gpuAktuell = false;
}

}  // namespace

std::string kameraSzeneMass() {
    char z[400];
    const auto mb = [](const Szene3& s) {
        return (s.dreieckDaten.capacity() * sizeof(kartegpu::Punkt3) + s.linienDaten.capacity() * sizeof(kartegpu::LinienPunkt)) /
               (1024 * 1024);
    };
    std::snprintf(z, sizeof(z),
                  "Kamera-Szene: Geometrie %zu Dreiecke, %zu Linienpunkte, %zu Stapel; Rest %zu Dreiecke, %zu Stapel; im Speicher %zu MB",
                  g_geo.dreieckAnzahl, g_geo.linienPunkte, g_geo.stapel.size(), g_szene.dreieckAnzahl, g_szene.stapel.size(),
                  mb(g_geo) + mb(g_szene));
    return z;
}

namespace {

// FloorHeightWalker: alle sichtbaren Knoten ausser Wurzel und Gruppen.
std::vector<karte::EtagenKandidat> etagenKandidaten() {
    Editor& ed = e();
    std::vector<karte::EtagenKandidat> v;
    const karte::Kontext& k = ed.karte.kontext;
    for (const auto& ent : ed.karte.entitaeten()) {
        for (const auto& p : ent->kinder()) {
            const bool gew = ed.karte.auswahl.gewaehlt(ent.get(), p.get());
            if (p->art() == karte::Primitiv::Art::Pinsel) {
                v.push_back({static_cast<const karte::Pinsel&>(*p).grenzen(k), gew});
            } else {
                v.push_back({static_cast<const karte::Patch&>(*p).grenzen(), gew});
            }
        }
        if (!ent->istBehaelter()) {
            const karte::EntitaetsKlasse* kl = ent->klasse();
            Vektor3f o(0, 0, 0);
            (void)std::sscanf(ent->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            const Vektor3f mn = kl != nullptr ? kl->mins : Vektor3f(-8, -8, -8);
            const Vektor3f mx = kl != nullptr ? kl->maxs : Vektor3f(8, 8, 8);
            v.push_back({karte::kastenAusMinMax(o + mn, o + mx), ed.karte.auswahl.gewaehlt(ent.get())});
        }
    }
    return v;
}

// --- KeyEvents (camwindow.cpp:1332-1433) ------------------------------------------

struct TastenEreignis {
    const char* name;
    unsigned flag;
    bool frei;   // CamWnd_Add_Handlers_FreeMove, sonst CamWnd_Move_Enable (nicht diskret)
};
const TastenEreignis kTastenEreignisse[] = {
    {"CameraForward", karte::kBewegVor, false},          {"CameraBack", karte::kBewegZurueck, false},
    {"CameraLeft", karte::kBewegDrehLinks, false},       {"CameraRight", karte::kBewegDrehRechts, false},
    {"CameraStrafeRight", karte::kBewegRechts, false},   {"CameraStrafeLeft", karte::kBewegLinks, false},
    {"CameraUp", karte::kBewegHoch, false},              {"CameraDown", karte::kBewegRunter, false},
    {"CameraAngleUp", karte::kBewegNeigHoch, false},     {"CameraAngleDown", karte::kBewegNeigRunter, false},
    {"CameraFreeMoveForward", karte::kBewegVor, true},   {"CameraFreeMoveBack", karte::kBewegZurueck, true},
    {"CameraFreeMoveLeft", karte::kBewegLinks, true},    {"CameraFreeMoveRight", karte::kBewegRechts, true},
    {"CameraFreeMoveForward2", karte::kBewegVor, true},  {"CameraFreeMoveBack2", karte::kBewegZurueck, true},
    {"CameraFreeMoveLeft2", karte::kBewegLinks, true},   {"CameraFreeMoveRight2", karte::kBewegRechts, true},
    {"CameraFreeMoveUp", karte::kBewegHoch, true},       {"CameraFreeMoveDown", karte::kBewegRunter, true},
    {"CameraFreeFocus", karte::kBewegFokus, true},
};

bool ereignisVerbunden(const TastenEreignis& t) {
    const Editor& ed = e();
    return t.frei ? ed.kamFenster.frei : (!ed.kamFenster.frei && !ed.kameraE.diskret);
}

// Das verbundene KeyEvent zur Taste key ohne Zusatztasten (nullptr: keins).
const TastenEreignis* ereignisFuer(int key) {
    Editor& ed = e();
    befehle::TastenBelegung kb;
    bool ziffernblock = false;
    bool erweitert = false;
    if (!tastenBelegung(key, kb, ziffernblock, erweitert) || ziffernblock) { return nullptr; }
    const std::vector<befehle::Taste> moeglich = befehle::moeglicheTasten(kb, 0, erweitert);
    for (const TastenEreignis& t : kTastenEreignisse) {
        if (!ereignisVerbunden(t)) { continue; }
        const befehle::Taste taste = ed.belegung.taste(t.name);
        if (taste == 0) { continue; }
        if (std::find(moeglich.begin(), moeglich.end(), taste) != moeglich.end()) { return &t; }
    }
    return nullptr;
}

void tasteRunter(const TastenEreignis& t) {
    Editor& ed = e();
    if (ed.kamera.flagsSetzen(t.flag)) { ed.kamFenster.tastenUhr = ImGui::GetTime(); }
    if (t.flag == karte::kBewegFokus) { ed.kamera.fokusVersatz = 0; }   // Camera_Focus_KeyDown
}

void tasteHoch(const TastenEreignis& t) {
    Editor& ed = e();
    if (ed.kamera.flagsLoeschen(t.flag)) { ed.kamFenster.tastenUhr = -1.0; }
    if (t.flag == karte::kBewegFokus) { ed.kamera.fokusVersatz = 0; }   // Camera_Focus_KeyUp
}

// --- Freelook (camwindow.cpp:1097-1177, 1768-1803) --------------------------------

ImVec2 fensterMitte() {
    // FreezePointer::getCenter: rect().center() in logischen Punkten
    const KameraFenster& f = e().kamFenster;
    const float qt = std::max(1.0F, std::round(g_app->dpiScale));
    const int b = static_cast<int>(std::lround(static_cast<float>(f.breite) / qt));
    const int h = static_cast<int>(std::lround(static_cast<float>(f.hoehe) / qt));
    return ImVec2{f.bildX + (static_cast<float>((b - 1) / 2) * qt), f.bildY + (static_cast<float>((h - 1) / 2) * qt)};
}

void freelookBeginnen() {
    Editor& ed = e();
    KameraFenster& f = ed.kamFenster;
    f.frei = true;
    if (ed.kamera.flagsLoeschen(karte::kBewegAlle)) { f.tastenUhr = -1.0; }
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    const ImVec2 m = fensterMitte();
    if (!imTest) {
        ImGui::TeleportMousePos(m);   // QCursor::setPos( getCenter() )
        f.letzteX = m.x;
        f.letzteY = m.y;
    } else {
        f.letzteX = ImGui::GetIO().MousePos.x;
        f.letzteY = ImGui::GetIO().MousePos.y;
    }
}

// context_menu_show (camwindow.cpp:1097-1102): das Entitaetenmenue der
// aktiven 2D-Ansicht (ActiveXY()->OnContextMenu()), g_bCamEntityMenu = true.
void kontextmenueZeigen() {
    e().kamFenster.kontextmenue = true;
    entitaetenMenueOeffnen(true);
}

// camera_orbit_init (camwindow.cpp:1112-1123): Drehpunkt unter dem Zeiger.
// Scene_Intersect mit Treffer (BestPointSelector) kommt mit der Auswahl
// (Punkt 7); ohne Treffer 256 vor der Nahebene.
void orbitBeginnen(float x, float y) {   // camera_orbit_init
    Editor& ed = e();
    karte::Kamera& k = ed.kamera;
    const karte::Vektor2f p = k.geraetePunkt(x, y);
    const karte::GeraeteVektor eps(8.F / static_cast<float>(k.breite), 8.F / static_cast<float>(k.hoehe));   // device epsilon
    k.orbitBeginnen(karte::szeneSchnitt(ed.karte, ed.blicke[3], p, eps));   // Scene_Intersect
}

void zahlText(char* z, std::size_t n, float f) { std::snprintf(z, n, "%g", static_cast<double>(f)); }

}  // namespace

void freelookBeenden() {
    Editor& ed = e();
    KameraFenster& f = ed.kamFenster;
    if (!f.frei) { return; }
    f.frei = false;
    if (ed.kamera.flagsLoeschen(karte::kBewegAlle)) { f.tastenUhr = -1.0; }
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    if (!imTest) { ImGui::TeleportMousePos(fensterMitte()); }   // unfreeze_pointer( true )
}

bool tastenEreignisNachgebaut(const std::string& name) {
    return std::any_of(std::begin(kTastenEreignisse), std::end(kTastenEreignisse),
                       [&name](const TastenEreignis& t) { return name == t.name; });
}

bool befehlVerbunden(const std::string& name) {
    const befehle::Befehl* b = befehle::finden(name);
    if (b == nullptr || (b->art & befehle::kKeyEvent) == 0U) { return true; }
    // CamWnd_Move_Discrete_Enable: die diskreten Befehle nur ausserhalb des
    // Freelooks und bei "Discrete movement"; reine KeyEvents nie als Befehl.
    if ((b->art & befehle::kCommand) != 0U) { return e().kameraE.diskret && !e().kamFenster.frei; }
    return false;
}

bool kameraTasteVerbraucht(int key) {
    Editor& ed = e();
    KameraFenster& f = ed.kamFenster;
    const auto k = static_cast<ImGuiKey>(key);
    bool verbraucht = false;
    if (ImGui::IsKeyPressed(k, true)) {
        // PressedKeys_key_press: nur ohne Zusatztasten (auch ohne KeypadModifier)
        if (qtZusatztasten() == 0) {
            const TastenEreignis* t = ereignisFuer(key);
            const bool neu = std::find(f.tasten.begin(), f.tasten.end(), key) == f.tasten.end();
            if (neu && ImGui::IsKeyPressed(k, false)) {
                f.tasten.push_back(key);
                if (t != nullptr) { tasteRunter(*t); }
            }
            verbraucht = t != nullptr;   // auch bei der Wiederholung
        }
    }
    if (ImGui::IsKeyReleased(k)) {
        // PressedKeys_key_release: unabhaengig von Zusatztasten
        const auto it = std::find(f.tasten.begin(), f.tasten.end(), key);
        if (it != f.tasten.end()) {
            f.tasten.erase(it);
            if (const TastenEreignis* t = ereignisFuer(key)) {
                tasteHoch(*t);
                verbraucht = true;
            }
        }
    }
    return verbraucht;
}

void kameraTastenLoslassen() {
    Editor& ed = e();
    KameraFenster& f = ed.kamFenster;
    // Keys_releaseAll
    for (const int key : f.tasten) {
        if (const TastenEreignis* t = ereignisFuer(key)) { tasteHoch(*t); }
    }
    f.tasten.clear();
}

bool kameraBrauchtBilder() {
    const Editor& ed = e();
    return ed.kamera.bewegung != 0U || ed.kamFenster.frei || !lager().fertig();
}

bool kameraBilderFertig() {
    lager().abholen();
    return lager().fertig();
}

bool kameraPixel(int x, int y, unsigned char rgba[4]) { return kartegpu::pixel(200, x, y, rgba); }

void* texturBild(const std::string& shader) {
    const Bild* b = lager().holen(shader);
    return b != nullptr && b->fertig ? b->srv : nullptr;
}

void texturBilderAbholen() { lager().abholen(); }

bool shaderZustand(const std::string& shader, kartegpu::Zustand3& z, bool& durchsichtig) {   // OpenGLShader::construct (Textured)
    const Bild* b = lager().holen(shader);
    if (b == nullptr || !b->fertig) { return false; }
    Editor& ed = e();
    const karte::ShaderEintrag* se = ed.karteSpiel ? ed.karteSpiel->inventar.shaderFinden(shader) : nullptr;
    const unsigned flags = se != nullptr ? se->flags : 0U;
    durchsichtig = (flags & karte::QER_TRANS) != 0U;
    z.licht = true;
    z.wegschneiden = (flags & karte::QER_CULL) == 0U || se->cull == 1;   // eCullBack
    if ((flags & karte::QER_ALPHATEST) != 0U) {
        z.alphaFunktion = se->alphaFunc;
        z.alphaRef = se->alphaRef;
    }
    if (durchsichtig) {
        z.mischen = kartegpu::Mischen::Alpha;
        z.tiefe = kartegpu::TiefenTest::KleinerGleich;
        z.tiefeSchreiben = false;
    }
    z.bild = b->srv;
    z.farbe[0] = 1;
    z.farbe[1] = 1;
    z.farbe[2] = 1;
    z.farbe[3] = durchsichtig ? se->trans : 1.0F;
    return true;
}

void texturBilderNeuLaden() { lager().neuLaden(); }

bool kameraTexturFarbe(const std::string& shader, float rgb[3]) {
    const Bild* b = lager().holen(shader);
    if (b == nullptr || !b->fertig) { return false; }
    rgb[0] = b->farbe[0];
    rgb[1] = b->farbe[1];
    rgb[2] = b->farbe[2];
    return true;
}

// --- Befehle --------------------------------------------------------------------

void kameraBefehl(const std::string& name) {
    Editor& ed = e();
    karte::Kamera& k = ed.kamera;
    karte::KameraEinstellungen& c = ed.kameraE;
    if (name == "CenterView") {
        k.winkelZuruecksetzen();
    } else if (name == "CameraFocusOnSelected") {
        k.setzeUrsprung(k.fokusPosition(ed.workzone));
    } else if (name == "ToggleCubicClip") {
        c.kubisch = !c.kubisch;   // Camera_SetFarClip
        k.projektionAktualisieren(c);
        einstellungenSpeichern();
    } else if (name == "CubicClipZoomIn") {
        karte::kubikHinein(c);
        k.projektionAktualisieren(c);
        einstellungenSpeichern();
    } else if (name == "CubicClipZoomOut") {
        karte::kubikHinaus(c);
        k.projektionAktualisieren(c);
        einstellungenSpeichern();
    } else if (name == "UpFloor" || name == "DownFloor") {
        (void)k.etageWechseln(name == "UpFloor", etagenKandidaten());
    } else if (name == "CameraModeNext") {
        c.modus = karte::naechsterModus(c.modus);
        einstellungenSpeichern();
    } else if (name == "CameraModePrev") {
        c.modus = karte::vorherigerModus(c.modus);
        einstellungenSpeichern();
    } else if (name == "CameraSpeedInc" || name == "CameraSpeedDec") {
        // CameraSpeed_increase/_decrease (camwindow.cpp:2474-2485): Text als
        // Ausgabe, die Zahl als Warnung - auf einer Zeile
        if (name == "CameraSpeedInc") {
            karte::tempoHoeher(c);
            konsoleSchreiben(KonsolenZeile::Art::Ausgabe, " ++Camera Move Speed: ");
        } else {
            karte::tempoNiedriger(c);
            konsoleSchreiben(KonsolenZeile::Art::Ausgabe, " --Camera Move Speed: ");
        }
        konsoleSchreiben(KonsolenZeile::Art::Warnung, std::to_string(c.tempo) + "\n");
        einstellungenSpeichern();
    } else if (name == "ShowStats") {
        c.zeigeStatistik = !c.zeigeStatistik;
        einstellungenSpeichern();
    } else if (name == "ShowWorkzone3d") {
        c.zeigeWorkzone = !c.zeigeWorkzone;
        einstellungenSpeichern();
    } else if (name == "ShowSize3d") {
        c.zeigeGroesse = !c.zeigeGroesse;
        einstellungenSpeichern();
    } else if (name == "CameraForward") {
        k.vorDiskret();
    } else if (name == "CameraBack") {
        k.zurueckDiskret();
    } else if (name == "CameraLeft") {
        k.drehLinksDiskret();
    } else if (name == "CameraRight") {
        k.drehRechtsDiskret();
    } else if (name == "CameraStrafeRight") {
        k.rechtsDiskret();
    } else if (name == "CameraStrafeLeft") {
        k.linksDiskret();
    } else if (name == "CameraUp") {
        k.hochDiskret();
    } else if (name == "CameraDown") {
        k.runterDiskret();
    } else if (name == "CameraAngleUp") {
        k.neigHochDiskret();
    } else if (name == "CameraAngleDown") {
        k.neigRunterDiskret();
    }
}

// --- Zeichnen und Bedienen ----------------------------------------------------------

void kameraZeichnen() {
    Editor& ed = e();
    karte::Kamera& k = ed.kamera;
    const karte::KameraEinstellungen& c = ed.kameraE;
    KameraFenster& f = ed.kamFenster;
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    const auto beginn = std::chrono::steady_clock::now();
    const ImVec2 o = ImGui::GetWindowPos();
    const ImVec2 groesse = ImGui::GetWindowSize();
    f.bildX = o.x;
    f.bildY = o.y;
    f.breite = std::max(1, static_cast<int>(groesse.x));
    f.hoehe = std::max(1, static_cast<int>(groesse.y));
    k.breite = f.breite;
    k.hoehe = f.hoehe;
    k.projektionAktualisieren(c);
    k.modelviewAktualisieren();
    const double jetzt = ImGui::GetTime();
    // Selection_UpdateWorkzone: Select_GetBounds = getBoundsSelected
    if (ed.karte.auswahl.anzahl() != 0) {
        const Kasten b = karte::auswahlGrenzenPivot(ed.karte);
        if (karte::kastenGueltig(b)) { ed.workzone = b; }
    }
    ed.bearbeitung.arbeitsbereich = ed.workzone;
    // m_view (Camera_updateProjection/-Modelview) und onSizeChanged
    ed.blicke[3].bauen(k.projektion, k.modelview, static_cast<std::size_t>(f.breite), static_cast<std::size_t>(f.hoehe));
    beobachterGroesse(3, f.breite, f.hoehe);
    // Camera_keyMove: ein Schritt je Durchlauf mit gemessener Zeit
    if (k.bewegung != 0U) {
        float dt = f.tastenUhr >= 0.0 ? static_cast<float>(jetzt - f.tastenUhr) : 0.0F;
        if (dt == 0.0F) { dt = 0.008F; }   // some reasonable move at the very start
        f.tastenUhr = jetzt;
        k.tastenSteuern(dt, c, ed.workzone);
    }

    // --- Szene
    lager().abholen();
    const bool mitKanten = c.modus == karte::KameraModus::Draht || c.modus == karte::KameraModus::TexturDraht;
    const std::size_t stand = szeneStand();
    const std::size_t geoStand = geometrieStand();
    const auto t0 = std::chrono::steady_clock::now();
    bool neu = false;
    if (g_geo.stand != geoStand || g_geo.mitKanten != mitKanten) {
        szeneBauen(g_geo, karte::KameraTeil::Geometrie, mitKanten);
        g_geo.stand = geoStand;
        ++ed.geoNeubauten;
        neu = true;
    }
    if (g_szene.stand != stand || g_szene.mitKanten != mitKanten) {
        szeneBauen(g_szene, karte::KameraTeil::Rest, mitKanten);
        g_szene.stand = stand;
        ++ed.restNeubauten;
        neu = true;
    }
    if (neu) { ed.kameraNeubauMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }
    Szene3& s = g_szene;

    // --- Schrift wie die 2D-Ansichten (Myriad Pro 8 pt -> Segoe UI)
    ImFont* ansichtSchrift = schrift();
    const float schriftPx = 8.0F * 96.0F / 72.0F * 1.33F;
    ImGui::PushFont(ansichtSchrift, schriftPx / std::max(0.01F, g_app->dpiScale * g_app->uiScale));
    const float schriftHoehe = std::round(ImGui::GetFontSize());

    const karte::Matrix4 vp = k.sicht();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const auto& hg = ed.farben.kameraHinten;
    const float hinten[4] = {hg[0], hg[1], hg[2], 0.0F};
    // Renderer_GetStats: Objekte, Zustaende, Matrix-Wechsel dieses Bilds
    std::size_t prims = 0;
    std::size_t zustaende = 0;
    std::size_t transforms = 0;
    const auto zaehlen = [&](std::size_t objekte, std::size_t wechsel) {
        if (objekte == 0) { return; }
        prims += objekte;
        transforms += wechsel;
        ++zustaende;
    };
    if (kartegpu::verfuegbar() && kartegpu::beginnen(200, f.breite, f.hoehe, c.msaa, hinten, true)) {
        for (Szene3* q : {&g_geo, &g_szene}) {
            if (q->gpuAktuell && ((!q->dreiecke.gueltig() && q->dreieckAnzahl > 0) || (!q->linien.gueltig() && q->linienPunkte > 0))) {
                q->stand = static_cast<std::size_t>(-1);   // Grafikgeraet neu: im naechsten Bild neu bauen
            }
            if (!q->gpuAktuell) {
                q->dreiecke.setzen(q->dreieckDaten.data(), q->dreieckDaten.size() * sizeof(kartegpu::Punkt3));
                q->linien.setzen(q->linienDaten.data(), q->linienDaten.size() * sizeof(kartegpu::LinienPunkt));
                q->gpuAktuell = true;
                // Punkt 12: die Daten liegen jetzt auf der Grafikkarte - die Kopien
                // im Speicher freigeben (duel_mos_espa: rund 190 MB).
                std::vector<kartegpu::Punkt3>().swap(q->dreieckDaten);
                std::vector<kartegpu::LinienPunkt>().swap(q->linienDaten);
            }
        }
        const bool fuellen = c.modus != karte::KameraModus::Draht;
        const bool texturen = c.modus == karte::KameraModus::Textur || c.modus == karte::KameraModus::TexturDraht;
        kartegpu::Zustand3 basis;
        basis.matrix = &vp[0];
        basis.vpn[0] = k.vpn[0];
        basis.vpn[1] = k.vpn[1];
        basis.vpn[2] = k.vpn[2];
        basis.auge[0] = k.ursprung[0];
        basis.auge[1] = k.ursprung[1];
        basis.auge[2] = k.ursprung[2];
        const auto farbeSetzen = [](kartegpu::Zustand3& z, float r, float g, float b, float a) {
            z.farbe[0] = r;
            z.farbe[1] = g;
            z.farbe[2] = b;
            z.farbe[3] = a;
        };
        // Ein Shader-Stapel: der Zustand aus OpenGLShader::construct (Shader)
        // UND der globale Zustand der Darstellungsart.
        const auto shaderZiehen = [&](const Szene3& q, const Stapel3& st, bool durchsichtig) {
            const Bild* b = lager().holen(st.shader);
            if (b == nullptr || !b->fertig) { return; }
            const karte::ShaderEintrag* se = ed.karteSpiel ? ed.karteSpiel->inventar.shaderFinden(st.shader) : nullptr;
            const unsigned flags = se != nullptr ? se->flags : 0U;
            const bool trans = (flags & karte::QER_TRANS) != 0U;
            if (trans != durchsichtig) { return; }
            kartegpu::Zustand3 z = basis;
            const bool himmel = b->himmel != nullptr;   // m_shader->getSkyBox()->texture_number != 0
            const float alpha = trans ? se->trans : 1.0F;
            if (himmel) {
                // RENDER_FILL | CULLFACE | TEXTURE | DEPTHTEST | DEPTHWRITE | COLOURWRITE | PROGRAM
                z.wegschneiden = true;
                z.tiefe = kartegpu::TiefenTest::Kleiner;
                z.tiefeSchreiben = true;
                farbeSetzen(z, b->farbe[0], b->farbe[1], b->farbe[2], 1.0F);
                if (texturen) { z.himmel = b->himmel; }
            } else {
                z.licht = true;
                z.wegschneiden = (flags & karte::QER_CULL) == 0U || se->cull == 1;   // eCullBack
                if ((flags & karte::QER_ALPHATEST) != 0U) {
                    z.alphaFunktion = se->alphaFunc;
                    z.alphaRef = se->alphaRef;
                }
                if (trans) {
                    z.mischen = kartegpu::Mischen::Alpha;
                    z.tiefe = kartegpu::TiefenTest::KleinerGleich;
                    z.tiefeSchreiben = false;
                }
                if (texturen) {
                    z.bild = b->srv;
                    farbeSetzen(z, 1, 1, 1, alpha);
                } else {
                    farbeSetzen(z, b->farbe[0], b->farbe[1], b->farbe[2], alpha);
                }
            }
            zaehlen(st.objekte, st.transforms);
            if (fuellen) {
                kartegpu::dreiecke(q.dreiecke, st.dreiecke.erstes, st.dreiecke.anzahl, z);
            } else {
                // ohne RENDER_FILL: GL_LINE, ohne Licht und Textur
                z.licht = false;
                z.bild = nullptr;
                z.himmel = nullptr;
                farbeSetzen(z, b->farbe[0], b->farbe[1], b->farbe[2], alpha);
                kartegpu::linien(q.linien, st.kanten.erstes, st.kanten.anzahl, z);
                kartegpu::linien(q.linien, st.kantenGewaehlt.erstes, st.kantenGewaehlt.anzahl, z);
            }
        };
        // "(r g b)" und "{r g b}"
        const auto farbStapelZiehen = [&](const Szene3& q, const Stapel3& st) {
            kartegpu::Zustand3 z = basis;
            z.licht = fuellen;
            z.wegschneiden = true;
            if (st.art == karte::StapelArt::Additiv) {
                z.mischen = kartegpu::Mischen::Additiv;
                z.tiefeSchreiben = false;
            }
            if (fuellen) {
                kartegpu::dreiecke(q.dreiecke, st.dreiecke.erstes, st.dreiecke.anzahl, z);
            } else {
                kartegpu::linien(q.linien, st.kanten.erstes, st.kanten.anzahl, z);
                kartegpu::linien(q.linien, st.kantenGewaehlt.erstes, st.kantenGewaehlt.anzahl, z);
            }
            kartegpu::linien(q.linien, st.linien.erstes, st.linien.anzahl, z);   // GL_LINES (Pfeile)
            zaehlen(st.objekte, st.transforms);
        };
        const Szene3* const teile[2] = {&g_geo, &g_szene};
        // eSortFullbright: deckende Shader, Himmel, Fuellungen
        for (const Szene3* q : teile) {
            for (const Stapel3& st : q->stapel) {
                if (st.art == karte::StapelArt::Shader) {
                    shaderZiehen(*q, st, false);
                } else if (st.art == karte::StapelArt::Fuellung) {
                    farbStapelZiehen(*q, st);
                }
            }
        }
        // eSortTranslucent: zuerst ohne Textur (Additiv), dann qer_trans
        for (const Szene3* q : teile) {
            for (const Stapel3& st : q->stapel) {
                if (st.art == karte::StapelArt::Additiv) { farbStapelZiehen(*q, st); }
            }
        }
        for (const Szene3* q : teile) {
            for (const Stapel3& st : q->stapel) {
                if (st.art == karte::StapelArt::Shader) { shaderZiehen(*q, st, true); }
            }
        }
        const auto& sel = ed.farben.auswahl3d;
        // gewaehlte Flaechen (Komponenten): Polygone, Kanten, Schwerpunkte
        std::vector<kartegpu::Punkt3> flaechenTint;
        std::vector<kartegpu::LinienPunkt> flaechenDraht;
        for (const auto& fw : ed.karte.auswahl.flaechen()) {
            const auto pinsel = fw.pinsel.lock();
            const auto fl = fw.flaeche.lock();
            if (!pinsel || !fl || !pinsel->sichtbar()) { continue; }
            pinsel->bRep(ed.karte.kontext);
            if (!fl->traegtBei()) { continue; }
            const karte::Wicklung& w = fl->wicklung();
            const karte::Vektor3d& nd = fl->ebene().normal();
            const Vektor3f n(static_cast<float>(nd[0]), static_cast<float>(nd[1]), static_cast<float>(nd[2]));
            std::vector<karte::KameraPunkt> ecken;
            for (std::size_t i = 0; i < w.numpoints; ++i) {
                ecken.push_back({Vektor3f(w[i].vertex), n, w[i].texcoord, 0xFFFFFFFFU});
            }
            std::vector<karte::KameraPunkt> dreiecke;
            std::vector<karte::KameraKante> kanten;
            karte::polygonFuellung(ecken, dreiecke, &kanten);
            for (const auto& d : dreiecke) { flaechenTint.push_back(punkt3(d, true)); }
            for (const auto& kk : kanten) { kanteDazu(kk, true, flaechenDraht); }
        }
        // eSortHighlight: $CAM_HIGHLIGHT (color_selbrushes3d, 0.3)
        if (c.faerben) {
            kartegpu::Zustand3 z = basis;
            farbeSetzen(z, sel[0], sel[1], sel[2], 0.3F);
            z.mischen = kartegpu::Mischen::Alpha;
            z.tiefe = kartegpu::TiefenTest::KleinerGleich;
            z.tiefeSchreiben = false;
            z.wegschneiden = true;
            if (fuellen) {
                kartegpu::dreiecke(s.tint, z);
            } else {
                kartegpu::linien(s.tintKanten, z);
            }
            kartegpu::linien(s.tintLinien, z);
            zaehlen(s.tintObjekte, s.tintTransforms);
            if (fuellen) {
                kartegpu::dreiecke(flaechenTint, z);
            } else {
                kartegpu::linien(flaechenDraht, z);
            }
        }
        // eSortOverlayFirst: verdeckte Kanten gestrichelt grau
        {
            kartegpu::Zustand3 z = basis;
            farbeSetzen(z, 0.75F, 0.75F, 0.75F, 1.0F);
            z.tiefe = kartegpu::TiefenTest::Groesser;
            z.tiefeSchreiben = false;
            z.wegschneiden = true;
            z.versatz = true;
            z.muster = 0xAAAAU;
            z.musterFaktor = 2;
            kartegpu::linien(s.overlay, z);
            zaehlen(s.overlayObjekte, s.overlayTransforms);
        }
        // eSortOverlayFirst + 1: $CAM_WIRE (Textured+Wire), dann $CAM_OVERLAY
        if (c.modus == karte::KameraModus::TexturDraht) {
            kartegpu::Zustand3 z = basis;
            farbeSetzen(z, 0.75F, 0.75F, 0.75F, 1.0F);
            z.tiefe = kartegpu::TiefenTest::KleinerGleich;
            z.tiefeSchreiben = false;
            z.wegschneiden = true;
            std::size_t objekte = 0;
            for (const Szene3* q : teile) {
                for (const Stapel3& st : q->stapel) {
                    if (st.art == karte::StapelArt::Shader) {
                        // die weissen Eckenfarben der Pinselflaechen: grau aus dem Zustand
                        kartegpu::linien(q->linien, st.kanten.erstes, st.kanten.anzahl, z);
                        objekte += st.objekte;
                    }
                }
            }
            zaehlen(objekte - std::min(objekte, s.wahlPrimitive), 1);
        }
        {
            kartegpu::Zustand3 z = basis;
            farbeSetzen(z, 1, 1, 1, 1);
            z.tiefe = kartegpu::TiefenTest::KleinerGleich;
            z.tiefeSchreiben = true;
            z.wegschneiden = true;
            z.versatz = true;
            kartegpu::linien(s.overlay, z);
            zaehlen(s.overlayObjekte, s.overlayTransforms);
        }
        // $CAM_FACEWIRE ("Selected faces wire"): verdeckt (eSortOverlayFirst + 1,
        // GL_GREATER, ohne Strichel - nicht im globalstate), dann sichtbar (+ 2)
        if (c.flaechenDraht && !flaechenDraht.empty()) {
            kartegpu::Zustand3 z = basis;
            farbeSetzen(z, sel[0], sel[1], sel[2], 1.0F);
            z.wegschneiden = true;
            z.versatz = true;
            z.tiefe = kartegpu::TiefenTest::Groesser;
            z.tiefeSchreiben = false;
            kartegpu::linien(flaechenDraht, z);
            z.tiefe = kartegpu::TiefenTest::KleinerGleich;
            z.tiefeSchreiben = true;
            kartegpu::linien(flaechenDraht, z);
        }
        // Komponenten (BrushInstance::renderComponents/-Selected, PatchInstance):
        // $LATTICE und $POINT (eSortControlFirst), $DEEPPOINT mit der Abfrage,
        // ob ein Punkt verdeckt ist (colour_occluded), $SELPOINT (+ 1), ohne Tiefentest
        {
            const KomponentenZeichnung komp = komponentenZeichnung(ed.blicke[3]);
            kartegpu::zeichnen(komp.gitter, &vp[0], 1.0F);
            kartegpu::punkte(komp.punkte, &vp[0], 6.0F);
            std::vector<karte::Linie> verdeckt = komp.tiefPunkte;
            for (auto& l : verdeckt) { l.farbe = karte::farbeAus(74.0F / 255.0F, 150.0F / 255.0F, 75.0F / 255.0F); }
            kartegpu::punkte(verdeckt, &vp[0], 6.0F, kartegpu::TiefenTest::Groesser);
            kartegpu::punkte(komp.tiefPunkte, &vp[0], 6.0F, kartegpu::TiefenTest::KleinerGleich);
            kartegpu::punkte(komp.gewaehlt, &vp[0], 4.0F);
        }
        // eSortOverlayFirst + 3: $CAM_WORKZONE
        if (c.zeigeWorkzone && ed.karte.auswahl.anzahl() != 0) {
            Kasten b;
            if (auswahlGrenzen(b)) {
                std::vector<karte::KameraKante> rot;
                std::vector<karte::KameraKante> weiss;
                karte::workzone3d(b, ed.raster.groesse(), rot, weiss);
                std::vector<kartegpu::LinienPunkt> lr;
                std::vector<kartegpu::LinienPunkt> lw;
                for (const auto& x : rot) { kanteDazu(x, false, lr); }
                for (const auto& x : weiss) { kanteDazu(x, false, lw); }
                kartegpu::Zustand3 z = basis;
                z.mischen = kartegpu::Mischen::Alpha;
                z.tiefe = kartegpu::TiefenTest::KleinerGleich;
                z.tiefeSchreiben = true;
                z.wegschneiden = false;
                z.versatz = true;
                z.versatzFaktor = -2;   // glPolygonOffset( -2, 2 )
                z.versatzEinheiten = 2;
                kartegpu::linien(lr, z);
                z.versatzFaktor = 1;    // glPolygonOffset( 1, -1 )
                z.versatzEinheiten = -1;
                kartegpu::linien(lw, z);
                zaehlen(1, 1);
            }
        }
        // BrushClipPlane::render mit RENDER_FILL ($CLIPPER_OVERLAY, eSortOverlayFirst):
        // gefuellt in color_clipper, Wegschneiden, Tiefe schreiben ohne Tiefentest;
        // die Polygon-Strichel fehlen im globalstate der Kamera
        {
            std::vector<kartegpu::Punkt3> clip;
            const auto& fc = ed.farben.clipper;
            for (const ClipperFlaeche& cf : clipperFlaechen()) {
                for (std::size_t i = 1; i + 1 < cf.punkte.size(); ++i) {   // Winding_Draw als Faecher
                    for (const std::size_t j : {std::size_t{0}, i, i + 1}) {
                        kartegpu::Punkt3 q{};
                        q.pos[0] = cf.punkte[j][0];
                        q.pos[1] = cf.punkte[j][1];
                        q.pos[2] = cf.punkte[j][2];
                        q.normale[0] = cf.normale[0];
                        q.normale[1] = cf.normale[1];
                        q.normale[2] = cf.normale[2];
                        q.farbe = 0xFFFFFFFFU;
                        clip.push_back(q);
                    }
                }
            }
            kartegpu::Zustand3 zc = basis;
            farbeSetzen(zc, fc[0], fc[1], fc[2], 1.0F);
            zc.tiefe = kartegpu::TiefenTest::Aus;
            zc.tiefeSchreiben = true;
            zc.wegschneiden = true;
            kartegpu::dreiecke(clip, zc);
        }
        // GlobalSelectionSystem().renderSolid mit dem CamRenderer: die
        // Strichel-Paesse fehlen im globalstate der Kamera (RENDER_LINESTIPPLE,
        // RENDER_POLYGONSTIPPLE) - die verdeckten Teile erscheinen voll.
        {
            const karte::ManipZeichnung mz = manipulatorZeichnung(3);
            const auto farbe = [](const karte::ManipFarbe& m) {
                return karte::farbeAus(static_cast<float>(m.r) / 255.0F, static_cast<float>(m.g) / 255.0F,
                                       static_cast<float>(m.b) / 255.0F, static_cast<float>(m.a) / 255.0F);
            };
            const auto linie = [](const Vektor3f& a, const Vektor3f& b, Farbe fa, const Vektor3f* dritte,
                                  std::vector<kartegpu::LinienPunkt>& aus) {
                kartegpu::LinienPunkt p{};
                p.pos[0] = a[0];
                p.pos[1] = a[1];
                p.pos[2] = a[2];
                p.farbe = fa;
                if (dritte != nullptr) {   // Kante eines Polygons im Linienmodus
                    p.extra[0] = (*dritte)[0];
                    p.extra[1] = (*dritte)[1];
                    p.extra[2] = (*dritte)[2];
                    p.extra[3] = 1.0F;
                }
                kartegpu::LinienPunkt q = p;
                q.pos[0] = b[0];
                q.pos[1] = b[1];
                q.pos[2] = b[2];
                aus.push_back(p);
                aus.push_back(q);
            };
            // $POINT (eSortControlFirst): Linien ohne Tiefentest
            std::vector<kartegpu::LinienPunkt> ohne;
            for (const auto& l : mz.ohneTiefe) { linie(l.a, l.b, farbe(l.farbe), nullptr, ohne); }
            kartegpu::Zustand3 zo = basis;
            zo.tiefe = kartegpu::TiefenTest::Aus;
            zo.tiefeSchreiben = false;
            zo.wegschneiden = false;
            // UV-Werkzeug: $uvtool/patchtexture (eSortOverlayLast: RENDER_FILL, TEXTURE,
            // LIGHTING, SMOOTH, ohne Tiefentest) - der Patch mit seiner Textur im UV-Raum
            for (const auto& netz : mz.texturNetze) {
                const Bild* b = lager().holen(netz.shader);
                std::vector<kartegpu::Punkt3> dreiecke;
                for (std::size_t i = 0; i < netz.punkte.size(); ++i) {
                    kartegpu::Punkt3 q{};
                    q.pos[0] = netz.punkte[i][0];
                    q.pos[1] = netz.punkte[i][1];
                    q.pos[2] = netz.punkte[i][2];
                    q.normale[0] = netz.normale[0];
                    q.normale[1] = netz.normale[1];
                    q.normale[2] = netz.normale[2];
                    q.st[0] = netz.st[i][0];
                    q.st[1] = netz.st[i][1];
                    q.farbe = 0xFFFFFFFFU;
                    dreiecke.push_back(q);
                }
                kartegpu::Zustand3 zt = basis;
                zt.tiefe = kartegpu::TiefenTest::Aus;
                zt.tiefeSchreiben = false;
                zt.wegschneiden = false;
                zt.licht = true;
                zt.bild = (b != nullptr && b->fertig) ? b->srv : nullptr;
                kartegpu::dreiecke(dreiecke, zt);
            }
            kartegpu::linien(ohne, zo);
            // UV-Werkzeug: $BLENDLINE (eSortGUI0 - 1): mit Alpha gemischt, ohne Tiefentest
            {
                std::vector<kartegpu::LinienPunkt> blend;
                for (const auto& l : mz.blendLinien) { linie(l.a, l.b, farbe(l.farbe), nullptr, blend); }
                kartegpu::Zustand3 zb = zo;
                zb.mischen = kartegpu::Mischen::Alpha;
                kartegpu::linien(blend, zb);
            }
            // $WIRE_OVERLAY, $FLATSHADE_OVERLAY, $PLANE_WIRE_OVERLAY
            std::vector<kartegpu::LinienPunkt> draht;
            for (const auto& l : mz.draht) { linie(l.a, l.b, farbe(l.farbe), nullptr, draht); }
            std::vector<kartegpu::LinienPunkt> flachKanten;
            std::vector<kartegpu::Punkt3> flach;
            for (const auto& d : mz.flach) {
                if (fuellen) {
                    for (std::size_t i = 0; i < 3; ++i) {
                        kartegpu::Punkt3 q{};
                        q.pos[0] = d.p[i][0];
                        q.pos[1] = d.p[i][1];
                        q.pos[2] = d.p[i][2];
                        q.normale[0] = d.n[i][0];
                        q.normale[1] = d.n[i][1];
                        q.normale[2] = d.n[i][2];
                        q.farbe = farbe(d.farbe);
                        flach.push_back(q);
                    }
                } else {   // ohne RENDER_FILL: Dreieckskanten (GL_LINE) mit Wegschneiden
                    for (std::size_t i = 0; i < 3; ++i) { linie(d.p[i], d.p[(i + 1) % 3], farbe(d.farbe), &d.p[(i + 2) % 3], flachKanten); }
                }
            }
            const Farbe gelb = karte::farbeAus(1, 1, 0);   // $PLANE_WIRE_OVERLAY m_colour
            std::vector<kartegpu::LinienPunkt> ebenen;
            for (const auto& poly : mz.ebenen) {
                for (std::size_t i = 0; i < poly.size(); ++i) {
                    linie(poly[i], poly[(i + 1) % poly.size()], gelb, &poly[(i + 2) % poly.size()], ebenen);
                }
            }
            for (const auto& l : mz.ebenenLinien) { linie(l.a, l.b, gelb, nullptr, ebenen); }
            std::vector<kartegpu::LinienPunkt> pivot;   // $PIVOT (RenderablePivot des UV-Werkzeugs)
            for (const auto& l : mz.pivotLinien) { linie(l.a, l.b, farbe(l.farbe), nullptr, pivot); }
            // MiscModel::renderSolid: ausgewaehlt der RenderablePivot (16) im Weltraum
            // (Pivot2World_worldSpace: Weltachsen, bildschirmfeste Groesse)
            if (ed.karte.kontext.modelle != nullptr) {
                const karte::Blick& bk = ed.blicke[3];
                for (const auto& w : ed.karte.auswahl.elemente()) {
                    const auto ent = w.entitaet.lock();
                    if (!ent || !w.primitiv.expired() || ent->istBehaelter() || ent->klasse() == nullptr || !ent->klasse()->miscmodelIs ||
                        !ent->sichtbar()) {
                        continue;
                    }
                    karte::Matrix4 lz;
                    karte::pivotWeltRaum(lz, karte::verschiebungsMatrix(karte::entitaetUrsprung(*ent)), bk.modelview(), bk.projektion(), bk.bildfeld());
                    const karte::Vektor3f po = karte::transformierterPunkt(lz, karte::Vektor3f(0, 0, 0));
                    linie(po, karte::transformierterPunkt(lz, karte::Vektor3f(16, 0, 0)), karte::farbeAus(1, 0, 0), nullptr, pivot);
                    linie(po, karte::transformierterPunkt(lz, karte::Vektor3f(0, 16, 0)), karte::farbeAus(0, 1, 0), nullptr, pivot);
                    linie(po, karte::transformierterPunkt(lz, karte::Vektor3f(0, 0, 16)), karte::farbeAus(0, 0, 1), nullptr, pivot);
                }
            }
            for (const bool sichtbar : {false, true}) {   // eSortGUI0 (verdeckt), dann eSortGUI1
                {   // $PIVOT: glLineWidth( 2 ), GL_LEQUAL bzw. verdeckt GL_GREATER (Strichel nicht im globalstate)
                    kartegpu::Zustand3 zp = basis;
                    zp.tiefe = sichtbar ? kartegpu::TiefenTest::KleinerGleich : kartegpu::TiefenTest::Groesser;
                    zp.tiefeSchreiben = sichtbar;
                    zp.wegschneiden = false;
                    zp.breite = 2;
                    kartegpu::linien(pivot, zp);
                }
                kartegpu::Zustand3 z = basis;
                z.tiefe = sichtbar ? kartegpu::TiefenTest::KleinerGleich : kartegpu::TiefenTest::Groesser;
                z.tiefeSchreiben = true;
                z.wegschneiden = false;
                kartegpu::linien(draht, z);
                kartegpu::Zustand3 zf = z;
                zf.wegschneiden = true;   // RENDER_CULLFACE
                if (fuellen) {
                    zf.licht = true;   // RENDER_LIGHTING
                    kartegpu::dreiecke(flach, zf);
                } else {
                    kartegpu::linien(flachKanten, zf);
                }
                kartegpu::Zustand3 ze = z;
                if (sichtbar) {   // RENDER_OFFSETLINE, glLineWidth( 2 )
                    ze.versatz = true;
                    ze.breite = 2;
                }
                kartegpu::linien(ebenen, ze);
            }
            // $BIGPOINT (eSortGUI1 + 1): ohne Tiefentest, 6 Bildpunkte
            std::vector<karte::Linie> punkte;
            for (const auto& pk : mz.grossePunkte) { punkte.push_back(karte::Linie{pk.p, pk.p, farbe(pk.farbe)}); }
            kartegpu::punkte(punkte, &vp[0], 6.0F);
        }
        // Fadenkreuz im Freelook (camwindow.cpp:2080-2100): Ursprung links
        // oben, y nach unten; schwarz, dann weiss um (-1,-1) versetzt.
        if (f.frei) {
            const float bw = static_cast<float>(f.breite);
            const float bh = static_cast<float>(f.hoehe);
            karte::Matrix4 ortho = karte::kEinheitsMatrix;
            ortho[0] = 2.0F / bw;
            ortho[5] = -2.0F / bh;
            ortho[10] = -1.0F / 100.0F;
            ortho[12] = -1.0F;
            ortho[13] = 1.0F;
            const float cx = static_cast<float>((f.breite + 1) / 2) + 0.5F;
            const float cy = static_cast<float>((f.hoehe + 1) / 2) + 0.5F;
            constexpr float a = 6.5F;
            constexpr float bb = 2.5F;
            for (const float g : {0.0F, 1.0F}) {
                const Farbe col = karte::farbeAus(g, g, g);
                const std::vector<karte::Linie> l = {
                    {Vektor3f(cx - g, cy + a - g, 0), Vektor3f(cx - g, cy + bb - g, 0), col},
                    {Vektor3f(cx - g, cy - a - g, 0), Vektor3f(cx - g, cy - bb - g, 0), col},
                    {Vektor3f(cx + a - g, cy - g, 0), Vektor3f(cx + bb - g, cy - g, 0), col},
                    {Vektor3f(cx - a - g, cy - g, 0), Vektor3f(cx - bb - g, cy - g, 0), col}};
                kartegpu::zeichnen(l, &ortho[0], 1.0F);
            }
        }
        // XORRectangle::render nach dem Bild
        const karte::AuswahlRechteck& r = ed.beobachter[3].rechteck;
        kartegpu::auswahlRechteck(r.min[0], r.min[1], r.max[0], r.max[1], static_cast<int>(r.art));
        kartegpu::beenden();
        dl->AddImage(reinterpret_cast<ImTextureID>(kartegpu::textur(200)), o, ImVec2{o.x + groesse.x, o.y + groesse.y});
    } else {
        dl->AddRectFilled(o, ImVec2{o.x + groesse.x, o.y + groesse.y},
                          IM_COL32(static_cast<int>(hg[0] * 255), static_cast<int>(hg[1] * 255), static_cast<int>(hg[2] * 255), 255));
    }

    // --- Texte ($TEXT: Ursprung in der Fenstermitte, y nach oben; die linke
    // untere Ecke sitzt auf dem projizierten Punkt)
    dl->PushClipRect(o, ImVec2{o.x + groesse.x, o.y + groesse.y}, true);
    const karte::Matrix4 bf = k.bildfeld();
    const auto aufBild = [&](const Vektor3f& p, float& x, float& y) {
        const float w = (vp[3] * p[0]) + (vp[7] * p[1]) + (vp[11] * p[2]) + vp[15];
        if (w < 0.005F) { return false; }   // w < 0: behind nearplane
        const float nx = ((vp[0] * p[0]) + (vp[4] * p[1]) + (vp[8] * p[2]) + vp[12]) / w;
        const float ny = ((vp[1] * p[0]) + (vp[5] * p[1]) + (vp[9] * p[2]) + vp[13]) / w;
        x = o.x + (static_cast<float>(f.breite) / 2.0F) + (nx * bf[0]);
        y = o.y + (static_cast<float>(f.hoehe) / 2.0F) - (ny * bf[5]);
        return true;
    };
    const auto text = [&](float x, float y, ImU32 farbe, const std::string& t) {
        const ImVec2 at{x, y - schriftHoehe};
        dl->AddText(ImVec2{at.x + 1, at.y + 1}, IM_COL32(0, 0, 0, 255), t.c_str());
        dl->AddText(at, farbe, t.c_str());
    };
    {   // ClipperPoint::render: die Namen der Clipper-Punkte (drawChar)
        const karte::ManipZeichnung mzt = manipulatorZeichnung(3);
        for (const auto& t : mzt.texte) {
            float x = 0;
            float y = 0;
            if (aufBild(t.p, x, y)) { dl->AddText(ImVec2{x, y - schriftHoehe}, IM_COL32(t.farbe.r, t.farbe.g, t.farbe.b, t.farbe.a), t.text.c_str()); }
        }
    }
    // Namen (namedentity.h:119-160): normal bis g_showNamesDist (Vorgabe 512)
    const double weit = static_cast<double>(ed.ent.namenAbstand);
    for (const karte::KameraName& n : s.namen) {
        if (n.art == 0 && karte::laengeQuadrat(n.pos - k.ursprung) > weit * weit) { continue; }
        float x = 0;
        float y = 0;
        if (!aufBild(n.pos, x, y)) { continue; }
        const ImU32 farbe = n.art == 1 ? IM_COL32(255, 255, 0, 255) : n.art == 2 ? IM_COL32(255, 128, 0, 255) : n.farbe;
        text(x, y, farbe, n.text);
    }
    // Groessenanzeige (CamDrawSize): Mitte der Kanten auf der Minimum-Seite
    if (c.zeigeGroesse && ed.karte.auswahl.anzahl() != 0) {
        Kasten b;
        if (auswahlGrenzen(b) && (b.extents[0] != 0 || b.extents[1] != 0 || b.extents[2] != 0)) {
            const Vektor3f p[3] = {b.origin - karte::kAchsen[1] * b.extents - karte::kAchsen[2] * b.extents,
                                   b.origin - karte::kAchsen[0] * b.extents - karte::kAchsen[2] * b.extents,
                                   b.origin - karte::kAchsen[0] * b.extents - karte::kAchsen[1] * b.extents};
            const auto& ax = ed.farben.achseX;
            const auto& ay = ed.farben.achseY;
            const auto& az = ed.farben.achseZ;
            const Farbe farben[3] = {karte::farbeAus(ax[0], ax[1], ax[2]), karte::farbeAus(ay[0], ay[1], ay[2]),
                                     karte::farbeAus(az[0], 178.0F / 255.0F, az[2])};   // hack to make default blue visible better
            for (std::size_t i = 0; i < 3; ++i) {
                float x = 0;
                float y = 0;
                if (!aufBild(p[i], x, y)) { continue; }
                char z[32];
                zahlText(z, sizeof(z), b.extents[i] * 2);
                text(x, y, farben[i], z);
            }
        }
    }
    // ShowStats: unten links
    if (c.zeigeStatistik) {
        const double f2f = f.letzteStatistik >= 0.0 ? (jetzt - f.letzteStatistik) * 1000.0 : 0.0;
        f.letzteStatistik = jetzt;
        char z[160];
        std::snprintf(z, sizeof(z), "prims: %zu | states: %zu | transforms: %zu | msec: %d | f2f: %d", prims, zustaende, transforms,
                      static_cast<int>(f.zeichenMs), static_cast<int>(f2f));
        dl->AddText(ImVec2{o.x + 1, o.y + groesse.y - schriftHoehe}, IM_COL32(255, 255, 255, 255), z);
    }
    dl->PopClipRect();
    ImGui::PopFont();
    f.zeichenMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - beginn).count();

    // --- Maus (CamGLWidget: mousePressEvent / mouseMoveEvent / mouseReleaseEvent / wheelEvent)
    const ImGuiIO& io = ImGui::GetIO();
    const bool ueber = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                       !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    const bool keine = !io.KeyShift && !io.KeyCtrl && !io.KeyAlt;
    const bool nurAlt = io.KeyAlt && !io.KeyShift && !io.KeyCtrl;
    // Fokus verloren: der Freelook endet (FreezePointer: FocusOut)
    if (f.frei && (io.AppFocusLost || !aktiv())) { freelookBeenden(); }
    // Fensterpunkte der Maus; im Freelook die Mitte (windowvector_for_widget_centre)
    const int dpr = std::max(1, static_cast<int>(std::lround(g_app->dpiScale)));   // QRect::center in logischen Punkten
    const float mausX = f.frei ? static_cast<float>(((f.breite / dpr - 1) / 2) * dpr) : io.MousePos.x - o.x;
    const float mausY = f.frei ? static_cast<float>(((f.hoehe / dpr - 1) / 2) * dpr) : io.MousePos.y - o.y;
    karte::Zusatz zusatz;   // modifiers_for_state
    zusatz.shift = io.KeyShift;
    zusatz.strg = io.KeyCtrl;
    zusatz.alt = io.KeyAlt;
    {
        const ImGuiMouseButton knoepfe[3] = {ImGuiMouseButton_Left, ImGuiMouseButton_Right, ImGuiMouseButton_Middle};
        for (int b = 0; b < 3; ++b) {
            // selection_button_press (ORBIT_EVENT: Alt+M2 nicht)
            if (ImGui::IsMouseClicked(knoepfe[b]) && (ueber || f.frei) && !(b == 1 && nurAlt)) {
                beobachterDruck(3, mausX, mausY, b, zusatz);
            }
            if (ImGui::IsMouseReleased(knoepfe[b]) && (ueber || f.frei || ed.beobachter[3].mausUnten)) {   // selection_button_release
                beobachterLoslassen(3, mausX, mausY, b, zusatz);
            }
        }
        // selection_motion (DeferredMotion2: einmal je Bild); im Freelook
        // selection_motion_freemove ueber FreezePointer
        if (!f.frei && (ueber || ed.beobachter[3].mausUnten) && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
            beobachterBewegung(3, mausX, mausY);
        }
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && (ueber || f.frei)) {
        const bool m2 = keine;      // M2_EVENT
        const bool m2alt = nurAlt;  // ORBIT_EVENT
        const double vergangen = (jetzt - f.rechtsUhr) * 1000.0;
        if (!f.frei) {
            // enable_freelook_button_press
            if (m2 || m2alt) {
                f.freiBetreten = true;
                if (m2 && karte::kontextmenueVersuch(vergangen)) {
                    kontextmenueZeigen();
                } else {
                    if (m2alt) { orbitBeginnen(io.MousePos.x - o.x, io.MousePos.y - o.y); }
                    freelookBeginnen();
                    f.rechtsUhr = jetzt;
                    f.rechtsWeg = 0;
                }
            }
        } else {
            // disable_freelook_button_press
            if (m2 || m2alt) {
                f.freiBetreten = false;
                if (m2 && karte::kontextmenueVersuch(vergangen)) {
                    freelookBeenden();
                    kontextmenueZeigen();
                } else {
                    if (m2alt) { orbitBeginnen(io.MousePos.x - o.x, io.MousePos.y - o.y); }
                    f.rechtsUhr = jetzt;
                    f.rechtsWeg = 0;
                }
            }
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && f.frei && (keine || nurAlt)) {
        // disable_freelook_button_release
        k.orbit = false;
        if (karte::freelookEndetBeimLoslassen((jetzt - f.rechtsUhr) * 1000.0, f.rechtsWeg, f.freiBetreten)) { freelookBeenden(); }
    }
    if (f.frei) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);   // BlankCursor
        // FreezePointer: ganzzahlige Deltas in logischen Qt-Punkten,
        // DeferredMotionDelta2 sammelt sie je Durchlauf.
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        if (ImGui::IsMousePosValid()) {
            const int dx = static_cast<int>(std::lround((io.MousePos.x - f.letzteX) / qt));
            const int dy = static_cast<int>(std::lround((io.MousePos.y - f.letzteY) / qt));
            if (dx != 0 || dy != 0) {
                k.strafeAusMaus(io.KeyAlt, io.KeyCtrl, io.KeyShift, ImGui::IsMouseDown(ImGuiMouseButton_Left),
                                ImGui::IsMouseDown(ImGuiMouseButton_Right), c.strafeModus);
                k.mausBewegen(dx, dy, c);
                f.rechtsWeg += std::sqrt(static_cast<double>((dx * dx) + (dy * dy)));   // selection_motion_freemove
                {   // incMouseMove + onMouseMotion( Mitte )
                    FensterBeobachter& fb = ed.beobachter[3];
                    // NRC: logische Deltas, Geraete-Breite
                    const karte::GeraeteVektor d(static_cast<float>(dx) * 2.F / static_cast<float>(fb.breite),
                                                 static_cast<float>(dy) * 2.F / static_cast<float>(fb.hoehe));
                    fb.bewegEnde = fb.bewegEnde - d;
                    if (fb.mausUnten) { fb.bewegStart = fb.bewegStart - d; }
                    beobachterBewegung(3, mausX, mausY);
                }
                f.letzteX += static_cast<float>(dx) * qt;
                f.letzteY += static_cast<float>(dy) * qt;
                if (!imTest) {
                    const ImVec2 m = fensterMitte();
                    ImGui::TeleportMousePos(m);
                    f.letzteX = m.x;
                    f.letzteY = m.y;
                }
            }
        }
    }
    // Mausrad: ein Schritt je Ereignis
    if (ueber || f.frei) {
        for (const int r : radEreignisse()) {
            k.rad(r, io.MousePos.x - o.x, io.MousePos.y - o.y, f.frei, c);
        }
    }
}

}  // namespace bhed::gui::karteneditor
