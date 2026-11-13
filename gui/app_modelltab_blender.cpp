// app_modelltab_blender.cpp - der Model-Tab im Bedienmodus wie Blender (AP4 von MBENTWURF Teil B, rc612)
//
// Massstab: der freigegebene Entwurf plaene/modellblock_stufe0/entwurf_modelltab.html (Bedienmodus "wie Blender": #links ist die
// Werkzeugleiste, #rechts der Outliner mit 44 % ueber den Eigenschaften, im Kopf der Ansicht "Benutzer-Perspektive - <Modus>",
// unten rechts "N Seitenleiste - T Werkzeuge"; werkzeugeModus() nennt die Werkzeuge je Bereich) und shank 10.11.: "es geht ja
// nicht nur um controls like blender oder 3ds max sondern das aussehen" - Werkzeugleiste links (T), Outliner oben rechts,
// darunter der Eigenschaften-Editor mit senkrechten Reitern (AP3), N-Seitenleiste, Umschalten ohne Verlust, die Fensteranordnung
// je Bedienmodus. Abgleichtabelle: R16-R18, X2, X15, X28, F2, F17, P0b, P8, M0b, U0b, W0b, T0b, A0b, A11.
//
// Vorbild Blender (Handbuch 4.x, "3D Viewport > Toolbar / Sidebar", "Modes"; blender_default.py km_view3d_generic: T
// wm.context_toggle space_data.show_region_toolbar, N wm.context_toggle space_data.show_region_ui; Ctrl+Tab
// view3d.object_mode_pie_or_toggle; Werkzeug "Measure" der Werkzeugleiste). Eigene Zeichnung und Symbole (uiicons.cpp), keine
// Grafik von Blender. Der Max-Modus bleibt, wie er ist: dort steht links der Szenen-Explorer, rechts nur das Befehlsfeld.
//
// Die Werkzeuge der Bereiche kommen aus deren Aktionstabellen (Modellieren, UV, Malen) bzw. Schnittstellen (Weights:
// modellGewichte, Animation: modellAnimWerkzeug von AP10) - dieselbe Aktion wie Knopf, Taste, Suche und Rechtsklick.
#include "app_internal.h"
#include "app_modelltab_knochen.h"   // AP13K: die Form eines Knochens unter der Maus
#include "platform.h"
#include "textumbruch.h"

#include "bhed/modell/malen.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace bhed::gui {

namespace {

ModellBlender g_bl;

ModellTab& mt() { return g_app->mtab; }

constexpr float kPi = 3.14159265358979323846F;
// Breite der Werkzeugleiste in Zeichenhoehen (Entwurf: 44 px bei 13 px Schrift, darin Knoepfe von 34 px) und des Streifens, wenn
// sie aus ist (Blender zeigt dann am Rand einen kleinen Pfeil).
constexpr float kLeisteBreite = 2.6F;
constexpr float kStreifenBreite = 1.0F;
// Seitenleiste: Vorgabe wie Blender (Sidebar etwa 18 Zeichen breit), hoechstens ein Drittel der Ansichten.
constexpr float kSeiteBreite = 15.0F;
const Str kBereichName[kModellBereiche] = {Str::MbBereichAnsehen, Str::MbBereichModellieren, Str::MbBereichUv,
                                           Str::MbBereichWeights, Str::MbBereichPainting,    Str::MbBereichAnimation};

// --- Werkzeuge -----------------------------------------------------------------------------------------------------------

struct Werkzeug {
    UiIcon icon = UiIcon::Mehr;
    std::string text;
    std::string hinweis;
    std::string taste;
    bool an = false;
    bool aktiv = true;
    std::function<void()> tun;
    // ENTSCHEID1: eine Werkzeuggruppe wie Blender (operator_menu_hold): der Knopf zeigt das zuletzt gewaehlte Glied, langes
    // Halten oder Ziehen oeffnet die Liste aller Glieder.
    std::string gruppe;
    std::vector<Werkzeug> glieder;
};

// Das gewaehlte Glied je Gruppe - wie Blenders ToolSelectPanelHelper._tool_group_active nur fuer die laufende Sitzung.
std::map<std::string, int> g_gruppenWahl;

// Aus den Gliedern den Knopf der Gruppe: das gewaehlte Glied; leuchtet, wenn eines der Glieder an ist.
Werkzeug gruppeAus(const std::string& name, std::vector<Werkzeug> glieder) {
    const auto it = g_gruppenWahl.find(name);
    const std::size_t n = it != g_gruppenWahl.end() ? static_cast<std::size_t>(std::clamp(it->second, 0, static_cast<int>(glieder.size()) - 1)) : 0U;
    Werkzeug w = glieder[n];
    w.gruppe = name;
    w.glieder = std::move(glieder);
    return w;
}

int inTabelle(int zahl, const char* (*text)(int), Str name) {
    const std::string n = tr(name);
    for (int i = 0; i < zahl; ++i) {
        if (n == text(i)) { return i; }
    }
    return -1;
}

void melde(const std::string& s) { mt().meldung = s; }

// FIX612C (P3-6): "Knochen waehlen" an/aus. Wie 3ds Max gibt es keine Werkzeugleiste, die den Zustand zeigt: dort ist es ein
// Umschalter wie "Abstand messen" (Meldung, Haken im Rechtsklick, Anzeige oben in der Ansicht; Q/W/E/R beenden es).
void knochenWaehlen(bool an) {
    if (an) {
        mt().bereich = ModellBereich::Ansehen;
        g_bl.werkzeug = 2;
        g_app->modellKnochen = true;   // die Knochen zeigen (Blender: nur sichtbare Knochen lassen sich waehlen)
    } else if (g_bl.werkzeug == 2) {
        g_bl.werkzeug = 0;
    }
    if (!modellBlender()) { melde(tr(an ? Str::BzKnochenAn : Str::BzKnochenAus)); }
}

// Die vier Werkzeuge jedes Bereichs (Blender: Select Box, Move, Rotate, Scale) - sie waehlen das Gizmo der Auswahl.
void basis(std::vector<Werkzeug>& w) {
    static const Str kText[4] = {Str::BzAuswaehlen, Str::BzBewegen, Str::BzDrehen, Str::BzSkalieren};
    static const Str kHint[4] = {Str::BzAuswaehlenHint, Str::BzBewegenHint, Str::BzDrehenHint, Str::BzSkalierenHint};
    static const UiIcon kIcon[4] = {UiIcon::BlAuswahl, UiIcon::Verschieben, UiIcon::Drehen, UiIcon::BlSkalieren};
    // die Tasten von JK Studio wie Blender: B Rahmen waehlen (view3d.select_box), G/R/S (transform.*)
    static const char* const kTaste[4] = {"B", "G", "R", "S"};
    const ModellTab& t = mt();
    const bool frei = t.bereich != ModellBereich::Ansehen || g_bl.werkzeug == 0;
    for (int i = 0; i < 4; ++i) {
        Werkzeug x;
        x.icon = kIcon[i];
        x.text = tr(kText[i]);
        x.hinweis = tr(kHint[i]);
        x.taste = kTaste[i];
        x.an = frei && t.gizmo == i;
        x.tun = [i] {
            mt().gizmo = i;
            g_bl.werkzeug = 0;
        };
        w.push_back(std::move(x));
    }
}

void fotoSpeichern() {
    const std::string p = platform::saveFileDialog(tr(Str::MbFoto), "Bitmap (*.bmp)|*.bmp", g_app->settings.lastDir,
                                                   fileName(directoryOf(g_app->modelPath)) + ".bmp");
    if (!p.empty() && modellFotoSpeichern(p, 1920)) { ++g_bl.fotos; }
}

// Ansehen/Pruefen (Entwurf: Messen, Flaeche waehlen, Knochen waehlen, Foto)
void ansehen(std::vector<Werkzeug>& w) {
    Werkzeug m;
    m.icon = UiIcon::BlMessen;
    m.text = tr(Str::BzMessen);
    m.hinweis = tr(Str::BzMessenHint);
    m.an = g_bl.werkzeug == 1;
    m.tun = [] {
        g_bl.werkzeug = 1;
        g_bl.messPunkte = 0;
        melde(tr(Str::BzMessenErster));
    };
    w.push_back(std::move(m));
    Werkzeug f;
    f.icon = UiIcon::MbFlaeche;
    f.text = tr(Str::BzFlaecheWaehlen);
    f.hinweis = tr(Str::BzFlaecheWaehlenHint);
    f.an = g_bl.werkzeug == 3;
    f.tun = [] {
        g_bl.werkzeug = 3;
        mt().gizmo = 0;
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
    };
    w.push_back(std::move(f));
    Werkzeug k;
    k.icon = UiIcon::MbKnochen;
    k.text = tr(Str::BzKnochenWaehlen);
    k.hinweis = tr(Str::BzKnochenWaehlenHint);
    k.an = g_bl.werkzeug == 2;
    k.aktiv = !g_app->anim.bones.empty();
    k.tun = [] {
        g_bl.werkzeug = 2;
        g_app->modellKnochen = true;   // die Knochen zeigen (Blender: nur sichtbare Knochen lassen sich waehlen)
    };
    w.push_back(std::move(k));
    Werkzeug b;
    b.icon = UiIcon::Kamera;
    b.text = tr(Str::MbFoto);
    b.hinweis = tr(Str::MbFotoHint);
    b.tun = [] { fotoSpeichern(); };
    w.push_back(std::move(b));
}

// Modellieren (Entwurf: Extrudieren, Einfuegen, Fase, Schleifenschnitt, Messer, Verschmelzen) - Blender Extrude Region (E),
// Inset Faces (I), Bevel (Strg+B: wie Max Chamfer an Kanten), Loop Cut (Strg+R), Knife (K), Merge (M); Tasten aus der Tabelle.
void modellieren(std::vector<Werkzeug>& w) {
    struct E {
        Str text;
        Str anzeige;   // der Name wie Blender, wo die Tabelle den Namen von 3ds Max fuehrt
        UiIcon icon;
        MeshWerkzeug wz;
    };
    // ENTSCHEID1: die Namen wie Blender 5.0 (space_toolsystem_toolbar.py: Extrude Region, Inset Faces, Bevel, Loop Cut, Knife; Merge
    // steht in Blender im Menue M) und die Gruppen wie dort (_tools EDIT_MESH: Extrude Region/... Individual, Knife/Bisect).
    static const E kListe[6] = {
        {Str::MdExtrude, Str::BeExtrude, UiIcon::MdExtrude, MeshWerkzeug::Extrude}, {Str::MdInset, Str::BeInset, UiIcon::MdInset, MeshWerkzeug::Inset},
        {Str::MdChamfer, Str::BzFase, UiIcon::MdChamfer, MeshWerkzeug::Chamfer},    {Str::MdLoopCut, Str::BeLoopCut, UiIcon::MdLoopCut, MeshWerkzeug::LoopCut},
        {Str::MdCut, Str::BzMesser, UiIcon::MdCut, MeshWerkzeug::Keins},           {Str::MdMerge, Str::MdMerge, UiIcon::MdMerge, MeshWerkzeug::Merge},
    };
    const ModellMesh& m = mt().mesh;
    for (const E& e : kListe) {
        const int i = inTabelle(modellMeshAktionZahl(), modellMeshAktionText, e.text);
        if (i < 0) { continue; }
        Werkzeug x;
        x.icon = e.icon;
        x.text = tr(e.anzeige);
        x.hinweis = modellMeshAktionHinweis(i);
        x.taste = modellMeshAktionTaste(i, true);
        x.an = e.wz == MeshWerkzeug::Keins ? m.cutAn : (m.kaestchenOffen && m.werkzeug == e.wz);
        x.tun = [i] { modellMeshAktionAusloesen(i); };
        if (e.wz == MeshWerkzeug::Extrude) {
            // Blender Extrude Region / Along Normals / Individual = die Arten Gruppe / Lokale Normale / Je Vieleck des Kaestchens
            // (wie 3ds Max Extrude Group / Local Normal / By Polygon)
            static const Str kText[3] = {Str::BeExtrude, Str::BeExtrudeNormalen, Str::BeExtrudeEinzeln};
            std::vector<Werkzeug> glieder;
            for (int art = 0; art < 3; ++art) {
                Werkzeug g = x;
                g.text = tr(kText[art]);
                if (art == 1) { g.hinweis = tr(Str::BeExtrudeNormalenHint); }
                if (art == 2) { g.hinweis = tr(Str::BeExtrudeEinzelnHint); }
                if (art > 0) { g.taste.clear(); }
                g.an = x.an && m.art == art;
                g.tun = [i, art] {
                    mt().mesh.art = art;
                    modellMeshAktionAusloesen(i);
                };
                glieder.push_back(std::move(g));
            }
            w.push_back(gruppeAus("extrude", std::move(glieder)));
            continue;
        }
        if (e.wz == MeshWerkzeug::Keins) {
            // Blender Knife / Bisect = Messer und QuickSlice (ein Schnitt mit einer gezogenen Linie durch die Auswahl)
            std::vector<Werkzeug> glieder{x};
            const int q = inTabelle(modellMeshAktionZahl(), modellMeshAktionText, Str::MdQuickSlice);
            if (q >= 0) {
                Werkzeug b;
                b.icon = UiIcon::MdQuickSlice;
                b.text = tr(Str::BeHalbieren);
                b.hinweis = modellMeshAktionHinweis(q);
                b.taste = modellMeshAktionTaste(q, true);
                b.an = m.quickAn;
                b.tun = [q] { modellMeshAktionAusloesen(q); };
                glieder.push_back(std::move(b));
            }
            w.push_back(gruppeAus("messer", std::move(glieder)));
            continue;
        }
        w.push_back(std::move(x));
    }
}

// UV (Entwurf: Naht, Abwickeln, Entspannen, Packen) - ENTSCHEID1: die Namen der Blender-Befehle (Mark Seam, Unwrap, Relax, Pack Islands)
void uv(std::vector<Werkzeug>& w) {
    struct E {
        Str text;
        UiIcon icon;
        Str anzeige;
    };
    static const E kListe[4] = {{Str::UvNahtMarkieren, UiIcon::UvSchere, Str::BeNaht}, {Str::UvAbwickeln, UiIcon::BlAbwickeln, Str::UvAbwickeln},
                                {Str::UvEntspannen, UiIcon::UvEntspannen, Str::BeEntspannen}, {Str::UvPacken, UiIcon::UvPacken, Str::BePacken}};
    for (const E& e : kListe) {
        const int i = inTabelle(modellUvAktionZahl(), modellUvAktionText, e.text);
        if (i < 0) { continue; }
        Werkzeug x;
        x.icon = e.icon;
        x.text = tr(e.anzeige);
        x.hinweis = modellUvAktionHinweis(i);
        x.taste = modellUvAktionTaste(i, true);
        // Blender: U oeffnet das Menue Abwickeln (blender_default.py km_uv_editor IMAGE_MT_uvs_unwrap) - der Knopf wickelt gleich ab
        if (x.taste.empty() && e.text == Str::UvAbwickeln) { x.taste = "U"; }
        x.tun = [i] { modellUvAktionAusloesen(i); };
        w.push_back(std::move(x));
    }
}

// Weights (Entwurf: Pinsel, Weichzeichnen, Verlauf, Gewicht holen) - wie die Gruppe Pinsel des Feldes (AP8, rolloutPinselBl); ENTSCHEID1:
// die Namen wie Blender 5.0 PAINT_WEIGHT (Brush, Blur, Gradient, Sample Weight)
void weights(std::vector<Werkzeug>& w) {
    ModellTab& t = mt();
    ModellGewichte& g = modellGewichte();
    const int glaetten = static_cast<int>(modell::PinselArt::Glaetten);
    const bool weich = g.werkzeug == 0 && t.pinselArt == glaetten;
    const auto malen = [] {
        mt().bereich = ModellBereich::Weights;
        if (!modellGewichte().malen) { modellGewichteMalenSetzen(true); }
        mt().gwAufnehmenKlick = false;
    };
    Werkzeug p;
    p.icon = UiIcon::GwPinsel;
    p.text = tr(Str::MgRPinsel);
    p.hinweis = tr(Str::MgMalenHint);
    p.an = g.malen && g.werkzeug == 0 && !weich && !t.gwAufnehmenKlick;
    p.tun = [malen] {
        malen();
        modellGewichteWerkzeug(0);
        if (mt().pinselArt == static_cast<int>(modell::PinselArt::Glaetten)) { mt().pinselArt = static_cast<int>(modell::PinselArt::Setzen); }
    };
    w.push_back(std::move(p));
    Werkzeug b;
    b.icon = UiIcon::GwMischen;
    b.text = tr(Str::MgWeich);
    b.hinweis = tr(Str::MbGwGlaettenHint);
    b.an = g.malen && weich && !t.gwAufnehmenKlick;
    b.tun = [malen] {
        malen();
        modellGewichteWerkzeug(0);
        mt().pinselArt = static_cast<int>(modell::PinselArt::Glaetten);
    };
    w.push_back(std::move(b));
    Werkzeug v;
    v.icon = UiIcon::GwVerlauf;
    v.text = tr(Str::BeVerlauf);
    v.hinweis = tr(Str::MgVerlaufHint);
    v.an = g.werkzeug == 2 && !t.gwAufnehmenKlick;
    v.tun = [] {
        mt().bereich = ModellBereich::Weights;
        mt().gwAufnehmenKlick = false;
        modellGewichteWerkzeug(2);
    };
    w.push_back(std::move(v));
    Werkzeug h;
    h.icon = UiIcon::GwTropfen;
    h.text = tr(Str::BeGewichtHolen);
    h.hinweis = tr(Str::MrGwAufnehmenHint);
    h.taste = "Shift+X";
    h.an = t.gwAufnehmenKlick;
    h.tun = [] {
        mt().bereich = ModellBereich::Weights;
        mt().gwAufnehmenKlick = !mt().gwAufnehmenKlick;
        if (mt().gwAufnehmenKlick) { melde(tr(Str::MrGwAufnehmenKlick)); }
    };
    w.push_back(std::move(h));
}

// Painting (Entwurf: Pinsel, Fuellen, Klonen, Wischen, Maske) - ENTSCHEID1: die Namen wie Blender 5.0 PAINT_TEXTURE (Brush, Fill, Clone,
// Smear, Mask; der erste heisst seit Blender 4.3 "Brush" statt "Draw")
void painting(std::vector<Werkzeug>& w) {
    using W = modell::texmalen::Werkzeug;
    struct E {
        Str text;
        UiIcon icon;
        W wz;
        Str anzeige;
    };
    static const E kListe[5] = {{Str::PtWzPinsel, UiIcon::MalPinsel, W::Pinsel, Str::MgRPinsel},
                                {Str::PtWzFuellen, UiIcon::MalFuellen, W::Fuellen, Str::BeFuellen},
                                {Str::PtWzKlonen, UiIcon::MalKlonen, W::Klonen, Str::PtWzKlonen},
                                {Str::PtWzWischen, UiIcon::MalWischen, W::Verwischen, Str::BeWischen},
                                {Str::PtWzMaske, UiIcon::MalMaske, W::Maske, Str::BeMaske}};
    for (const E& e : kListe) {
        const int i = inTabelle(modellMalenAktionZahl(), modellMalenAktionText, e.text);
        if (i < 0) { continue; }
        Werkzeug x;
        x.icon = e.icon;
        x.text = tr(e.anzeige);
        x.hinweis = modellMalenAktionHinweis(i);
        x.taste = modellMalenAktionTaste(i, true);
        x.an = mt().malen.werkzeug == static_cast<int>(e.wz);
        x.tun = [i] { modellMalenAktionAusloesen(i); };
        w.push_back(std::move(x));
    }
}

// Animation (Entwurf A11: Pose kopieren, Spiegeln, Ausduennen) - Schnittstelle von AP10
void animation(std::vector<Werkzeug>& w) {
    for (int i = 0; i < modellAnimWerkzeugZahl(); ++i) {
        const ModellAnimWerkzeug a = modellAnimWerkzeug(i);
        Werkzeug x;
        x.icon = a.icon;
        // ENTSCHEID1: die Namen der Blender-Befehle Copy Pose / Paste X-Flipped Pose (pose.copy, pose.paste flipped)
        const std::string name = a.text;
        x.text = name == tr(Str::MbRgKopieren) ? std::string(tr(Str::BeKopieren)) : (name == tr(Str::MbRgGespiegelt) ? std::string(tr(Str::BeGespiegelt)) : name);
        x.hinweis = a.hinweis;
        x.taste = a.taste;
        x.aktiv = a.aktiv;
        x.tun = [i] { (void)modellAnimWerkzeugAusloesen(i); };
        w.push_back(std::move(x));
    }
}

std::vector<Werkzeug> werkzeuge() {
    std::vector<Werkzeug> w;
    basis(w);
    switch (mt().bereich) {
        case ModellBereich::Ansehen: ansehen(w); break;
        case ModellBereich::Modellieren: modellieren(w); break;
        case ModellBereich::Uv: uv(w); break;
        case ModellBereich::Weights: weights(w); break;
        case ModellBereich::Painting: painting(w); break;
        case ModellBereich::Animation: animation(w); break;
    }
    return w;
}

void ausloesen(const Werkzeug& x) {
    if (!x.aktiv || !x.tun) { return; }
    x.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = x.text;
    g_app->modelDirty = true;
}

// --- Zeichnen ------------------------------------------------------------------------------------------------------------

float helligkeit(const ImVec4& c) { return c.x * 0.299F + c.y * 0.587F + c.z * 0.114F; }

void tooltip(const std::string& titel, const std::string& taste, const std::string& hinweis) {
    if (!ImGui::BeginTooltip()) { return; }
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
    cjkTextUnformatted(taste.empty() ? titel.c_str() : (titel + "   " + taste).c_str());
    if (!hinweis.empty()) { cjkTextDisabled("%s", hinweis.c_str()); }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// Die offene Werkzeuggruppe (ENTSCHEID1): ihre Glieder, der Name, wo das Menue steht und ob es durch Halten/Ziehen aufging
// (dann schliesst Loslassen ausserhalb des Menues es wie Blenders BLOCK_POPUP_HOLD).
std::vector<Werkzeug> g_gruppeGlieder;
std::string g_gruppeName;
ImVec2 g_gruppeOrt;
bool g_gruppeHalten = false;
constexpr const char* kGruppeMenue = "##blgruppe";
// Blender interface_handlers.cc: ein Knopf mit hold_func oeffnet sein Menue nach BUTTON_AUTO_OPEN_THRESH (0.2 s) Halten - oder sofort,
// wenn die Maus beim Halten weiter als die Ziehschwelle wandert (WM_event_drag_threshold: U.drag_threshold_mouse, ab Werk 3 Punkte,
// mal der Oberflaechengroesse).
constexpr float kHalteZeit = 0.2F;
constexpr float kZiehSchwelle = 3.0F;

// Ein Knopf der Werkzeugleiste: quadratisch, Symbol in der Mitte, das gewaehlte Werkzeug in der Betonungsfarbe (wie Blender blau);
// mit Namen (ENTSCHEID1, "Werkzeugnamen zeigen") so breit wie die Leiste, das Symbol links und der Name daneben. Eine Gruppe hat
// unten rechts ein kleines Dreieck und oeffnet ihre Glieder beim Halten oder Ziehen. Die Kennung ist der Text - Tooltip und
// Selbsttest finden ihn darueber.
bool leistenKnopf(const Werkzeug& x, float g, float breite) {
    const bool namen = g_app->settings.modellBlWerkzeugnamen;
    ImGui::BeginDisabled(!x.aktiv);
    const bool geklickt = ImGui::InvisibleButton(x.text.c_str(), ImVec2{namen ? breite : g, g});
    const bool ueber = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    const bool gehalten = ImGui::IsItemActive();
    ImGui::EndDisabled();
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float fs = ImGui::GetFontSize();
    const float rund = std::floor(fs * 0.22F);
    const ImVec4 beton = ui::betonungsFarbe();
    ImU32 farbe = ImGui::GetColorU32(uiIconFarbe(x.icon));
    ImU32 schrift = ImGui::GetColorU32(ImGuiCol_Text);
    if (x.an) {
        dl->AddRectFilled(a, b, ImGui::GetColorU32(beton), rund);
        farbe = helligkeit(beton) > 0.6F ? IM_COL32_BLACK : IM_COL32_WHITE;
        schrift = farbe;
    } else if (ueber && x.aktiv) {
        dl->AddRectFilled(a, b, ImGui::GetColorU32(gehalten ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered), rund);
    }
    if (!x.aktiv) {
        farbe = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        schrift = farbe;
    }
    const float ig = std::floor(g * 0.6F);
    uiIcon(dl, x.icon, ImVec2{std::floor(a.x + (g - ig) * 0.5F), std::floor(a.y + (g - ig) * 0.5F)}, ig, farbe);
    if (namen) {
        const ImVec2 tg = ImGui::CalcTextSize(x.text.c_str());
        dl->PushClipRect(a, b, true);
        dl->AddText(ImVec2{a.x + g + std::floor(fs * 0.1F), std::floor(a.y + (g - tg.y) * 0.5F)}, schrift, x.text.c_str());
        dl->PopClipRect();
        g_bl.namenGezeigt = true;
    }
    if (!x.glieder.empty()) {
        // das kleine Dreieck unten rechts (Blender zeichnet es an Knoepfen mit Haltefunktion)
        const float s = std::floor(std::max(g * 0.18F, 3.0F));
        const float r = std::floor(g * 0.08F);
        const ImVec2 p0{b.x - r, b.y - r - s};
        const ImVec2 p1{b.x - r, b.y - r};
        const ImVec2 p2{b.x - r - s, b.y - r};
        dl->AddTriangleFilled(p0, p1, p2, farbe);
        g_bl.gruppen.push_back(x.text);
        g_bl.dreiecke.emplace_back(ImVec2{p2.x, p0.y}, p1);
        if (gehalten) {
            const ImGuiIO& io = ImGui::GetIO();
            const float dauer = io.MouseDownDuration[0];
            const float zug = std::hypot(io.MousePos.x - io.MouseClickedPos[0].x, io.MousePos.y - io.MouseClickedPos[0].y);
            const bool gezogen = zug >= kZiehSchwelle * std::max(g_app->uiScale, 0.5F);
            if (dauer >= kHalteZeit || gezogen) {
                g_gruppeGlieder = x.glieder;
                g_gruppeName = x.gruppe;
                g_gruppeOrt = ImVec2{b.x + std::floor(fs * 0.2F), a.y};
                g_gruppeHalten = true;
                g_bl.gruppeOffen = x.text;
                g_bl.gruppeOffenNach = dauer;
                g_bl.gruppeGezogen = gezogen && dauer < kHalteZeit;
                // die Maus gehoert jetzt dem Menue: Loslassen auf einem Eintrag waehlt ihn (sonst bliebe die linke Taste bis zum
                // Loslassen dem Knopf, und der Eintrag saehe das Loslassen nicht)
                ImGui::ClearActiveID();
                ImGui::SetKeyOwner(ImGuiKey_MouseLeft, ImGuiKeyOwner_NoOwner);
                ImGui::OpenPopup(kGruppeMenue);
            }
        }
    }
    g_bl.knoepfe.emplace_back(x.text, ImRect{a, b});
    if (ueber && !gehalten) {
        tooltip(x.text, x.taste, x.glieder.empty() ? x.hinweis : x.hinweis + (x.hinweis.empty() ? "" : "\n") + tr(Str::BeGruppeHint));
    }
    return geklickt && x.aktiv;
}

// Die Glieder der offenen Gruppe als Menue rechts neben dem Knopf (Blender WM_MT_toolsystem_submenu: Symbol und Name, doppelt hoch).
void gruppenMenue() {
    if (ImGui::IsPopupOpen(kGruppeMenue)) { ImGui::SetNextWindowPos(g_gruppeOrt, ImGuiCond_Appearing); }
    if (!ImGui::BeginPopup(kGruppeMenue)) {
        g_bl.gruppeOffen.clear();
        g_bl.gruppeOffenNach = -1.0F;
        g_bl.gruppenEintraege.clear();
        g_gruppeHalten = false;
        return;
    }
    g_bl.gruppenEintraege.clear();
    const float fs = ImGui::GetFontSize();
    const float h = std::floor(ImGui::GetFrameHeight() * 2.0F);   // Blender: layout.scale_y = 2.0
    float breite = 0.0F;
    for (const Werkzeug& w : g_gruppeGlieder) { breite = std::max(breite, ImGui::CalcTextSize(w.text.c_str()).x); }
    breite += h + fs;
    bool gewaehlt = false;
    for (std::size_t i = 0; i < g_gruppeGlieder.size(); ++i) {
        const Werkzeug& w = g_gruppeGlieder[i];
        ImGui::PushID(static_cast<int>(i));
        const ImVec2 o = ImGui::GetCursorScreenPos();
        const bool klick = ImGui::Selectable("##glied", w.an, static_cast<ImGuiSelectableFlags>(ImGuiSelectableFlags_SelectOnRelease), ImVec2{breite, h});
        const bool ueber = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float ig = std::floor(h * 0.55F);
        uiIcon(dl, w.icon, ImVec2{std::floor(o.x + (h - ig) * 0.5F), std::floor(o.y + (h - ig) * 0.5F)}, ig, ImGui::GetColorU32(uiIconFarbe(w.icon)));
        const ImVec2 tg = ImGui::CalcTextSize(w.text.c_str());
        dl->AddText(ImVec2{o.x + h, std::floor(o.y + (h - tg.y) * 0.5F)}, ImGui::GetColorU32(ImGuiCol_Text), w.text.c_str());
        g_bl.gruppenEintraege.emplace_back(w.text, ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()});
        if (ueber) { tooltip(w.text, w.taste, w.hinweis); }
        if (klick && !gewaehlt) {
            g_gruppenWahl[g_gruppeName] = static_cast<int>(i);
            ausloesen(w);
            gewaehlt = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopID();
    }
    // durch Halten/Ziehen geoeffnet: Loslassen ausserhalb des Menues schliesst es ohne Wahl (Blender BLOCK_POPUP_HOLD)
    if (g_gruppeHalten && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!gewaehlt && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) { ImGui::CloseCurrentPopup(); }
        g_gruppeHalten = false;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// Das Menue der Werkzeugleiste und der Ansicht (Rechtsklick): Werkzeugleiste T, Seitenleiste N (wie Blenders Menue View) und
// "Werkzeugnamen zeigen" (ENTSCHEID1).
void leistenMenue() {
    if (ImGui::MenuItem(tr(Str::BzWerkzeugleiste), "T", g_bl.werkzeugleiste)) { g_bl.werkzeugleiste = !g_bl.werkzeugleiste; }
    ImGui::SetItemTooltip("%s", tr(Str::BzWerkzeugleisteHint));
    if (ImGui::MenuItem(tr(Str::BzSeitenleiste), "N", g_bl.seitenleiste)) { g_bl.seitenleiste = !g_bl.seitenleiste; }
    ImGui::SetItemTooltip("%s", tr(Str::BzSeitenleisteHint));
    if (ImGui::MenuItem(tr(Str::BeWerkzeugnamen), nullptr, g_app->settings.modellBlWerkzeugnamen)) {
        g_app->settings.modellBlWerkzeugnamen = !g_app->settings.modellBlWerkzeugnamen;
    }
    ImGui::SetItemTooltip("%s", tr(Str::BeWerkzeugnamenHint));
}

// Text um 90 Grad gedreht (von oben nach unten zu lesen, wie die Reiter der Seitenleiste von Blender).
void textSenkrecht(ImDrawList* dl, ImVec2 mitte, const char* text, ImU32 farbe) {
    const ImVec2 g = ImGui::CalcTextSize(text);
    dl->PushClipRect(ImVec2{mitte.x - g.x, mitte.y - g.x}, ImVec2{mitte.x + g.x, mitte.y + g.x}, false);
    const int von = dl->VtxBuffer.Size;
    dl->AddText(ImVec2{std::floor(mitte.x - g.x * 0.5F), std::floor(mitte.y - g.y * 0.5F)}, farbe, text);
    for (int i = von; i < dl->VtxBuffer.Size; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        const float dx = v.pos.x - mitte.x;
        const float dy = v.pos.y - mitte.y;
        v.pos = ImVec2{mitte.x - dy, mitte.y + dx};
    }
    dl->PopClipRect();
}

// --- Messen und Knochen waehlen ------------------------------------------------------------------------------------------

// Der Punkt der Oberflaeche unter der Maus: das vorderste Dreieck (modellDreieckUnter) und darin der Ort mit den
// perspektivisch richtigen Anteilen der drei Ecken (die Anteile auf dem Schirm, geteilt durch die Tiefe w jeder Ecke).
bool punktUnter(const ModellAnsicht& a, ImVec2 maus, float out[3]) {
    int teil = -1;
    int dr = -1;
    if (!modellDreieckUnter(a, maus, teil, dr)) { return false; }
    const auto& haut = modellHaut();
    const ModellTab& t = mt();
    if (teil < 0 || static_cast<std::size_t>(teil) >= haut.size() || static_cast<std::size_t>(teil) >= t.anzeige.surfaces.size()) { return false; }
    const GlmSurface& s = t.anzeige.surfaces[static_cast<std::size_t>(teil)];
    const std::size_t k = static_cast<std::size_t>(dr) * 3U;
    if (k + 2 >= s.indexes.size()) { return false; }
    const float* m = a.viewProj;
    ImVec2 p[3];
    float w[3];
    const std::array<float, 3>* e[3];
    for (std::size_t i = 0; i < 3; ++i) {
        const auto idx = static_cast<std::size_t>(s.indexes[k + i]);
        if (idx >= haut[static_cast<std::size_t>(teil)].size()) { return false; }
        e[i] = &haut[static_cast<std::size_t>(teil)][idx];
        const float* q = e[i]->data();
        w[i] = m[12] * q[0] + m[13] * q[1] + m[14] * q[2] + m[15];
        if (w[i] <= 1e-3F || !modellAufSchirm(a, q, p[i])) { return false; }
    }
    const float f = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[2].x - p[0].x) * (p[1].y - p[0].y);
    if (std::fabs(f) < 1e-6F) { return false; }
    const float u = ((p[1].x - maus.x) * (p[2].y - maus.y) - (p[2].x - maus.x) * (p[1].y - maus.y)) / f;
    const float v = ((p[2].x - maus.x) * (p[0].y - maus.y) - (p[0].x - maus.x) * (p[2].y - maus.y)) / f;
    const float l[3] = {u / w[0], v / w[1], (1.0F - u - v) / w[2]};
    const float summe = l[0] + l[1] + l[2];
    if (std::fabs(summe) < 1e-9F) { return false; }
    for (int c = 0; c < 3; ++c) { out[c] = (l[0] * (*e[0])[static_cast<std::size_t>(c)] + l[1] * (*e[1])[static_cast<std::size_t>(c)] + l[2] * (*e[2])[static_cast<std::size_t>(c)]) / summe; }
    return true;
}

// Der Knochen, dessen Gelenk auf dem Schirm am naechsten an der Maus liegt (hoechstens eine Zeichenhoehe), sichtbar und nicht
// eingefroren; -1 keiner.
int knochenUnter(const ModellAnsicht& a, ImVec2 maus) {
    // AP13K: zuerst die gezeichnete Form des Knochens (Oktaeder, Stick, Kasten ...), sonst wie bisher das naechste Gelenk
    if (const int form = modellKdKnochenUnter(a, maus); form >= 0) { return form; }
    const std::vector<BoneMatrix>& welt = g_app->modellWelt;
    const std::size_t n = std::min(welt.size(), g_app->anim.bones.size());
    const float grenze = ImGui::GetFontSize();
    float best = grenze * grenze;
    int wahl = -1;
    for (std::size_t b = 0; b < n; ++b) {
        if (modellKnochenVersteckt(static_cast<int>(b)) || modellKnochenEingefroren(static_cast<int>(b))) { continue; }
        const float p[3] = {welt[b].m[0][3], welt[b].m[1][3], welt[b].m[2][3]};
        ImVec2 s;
        if (!modellAufSchirm(a, p, s)) { continue; }
        const float d = (s.x - maus.x) * (s.x - maus.x) + (s.y - maus.y) * (s.y - maus.y);
        if (d < best) {
            best = d;
            wahl = static_cast<int>(b);
        }
    }
    return wahl;
}

float abstandVon(const float a[3], const float b[3]) { return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2])); }

std::string einheiten(float d) {
    char z[64];
    std::snprintf(z, sizeof z, tr(Str::BzEinheiten), static_cast<double>(d));
    return z;
}

// --- Seitenleiste --------------------------------------------------------------------------------------------------------

// Die Beschriftung links, auf `platz` Punkte gekuerzt: passt sie nicht, endet sie mit ... wie in Blenders schmalen Panels (der ganze
// Text steht dann im Tooltip). Liefert den gezeichneten Text.
std::string gekuerzt(const std::string& text, float platz) {
    if (ImGui::CalcTextSize(text.c_str()).x <= platz) { return text; }
    const std::string punkte = "\xe2\x80\xa6";
    std::string s = text;
    while (!s.empty()) {
        // ein ganzes UTF-8-Zeichen entfernen
        std::size_t n = s.size() - 1;
        while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0U) == 0x80U) { --n; }
        s.erase(n);
        while (!s.empty() && s.back() == ' ') { s.pop_back(); }
        if (ImGui::CalcTextSize((s + punkte).c_str()).x <= platz) { return s + punkte; }
    }
    return punkte;
}

void feldTooltip(const char* text) {
    if (!ImGui::BeginTooltip()) { return; }
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
    cjkTextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// Eine Zeile wie Blenders Panels: Beschriftung rechtsbuendig links, der Wert in einem Feld rechts (nur zum Ablesen). Liefert das
// Rechteck des Felds.
ImRect seitenZeile(const std::string& name, const std::string& wert, bool nurLesen = false) {
    const float breite = ImGui::GetContentRegionAvail().x;
    const float links = std::floor(breite * 0.42F);
    const float fs = ImGui::GetFontSize();
    const ImVec2 o = ImGui::GetCursorScreenPos();
    const float h = std::floor(fs * 1.35F);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::string zeigt = gekuerzt(name, links - fs * 0.4F);
    const ImVec2 nb = ImGui::CalcTextSize(zeigt.c_str());
    dl->AddText(ImVec2{std::max(o.x, o.x + links - fs * 0.4F - nb.x), o.y + (h - nb.y) * 0.5F}, ImGui::GetColorU32(ImGuiCol_TextDisabled), zeigt.c_str());
    const ImVec2 fa{o.x + links, o.y};
    const ImVec2 fb{o.x + breite, o.y + h - 2.0F};
    dl->AddRectFilled(fa, fb, ImGui::GetColorU32(ImGuiCol_FrameBg), std::floor(fs * 0.2F));
    const ImVec2 wb = ImGui::CalcTextSize(wert.c_str());
    dl->PushClipRect(fa, fb, true);
    // ein nur ablesbares Zahlenfeld: der Wert mittig (wie die eingebbaren) und grau wie Blenders inaktive Felder
    const float wx = nurLesen ? std::floor((fa.x + fb.x - wb.x) * 0.5F) : fb.x - fs * 0.3F - wb.x;
    dl->AddText(ImVec2{std::max(fa.x + fs * 0.3F, wx), o.y + (h - 2.0F - wb.y) * 0.5F}, ImGui::GetColorU32(nurLesen ? ImGuiCol_TextDisabled : ImGuiCol_Text), wert.c_str());
    dl->PopClipRect();
    ImGui::Dummy(ImVec2{breite, h});
    if (ImGui::IsItemHovered() && zeigt != name && ImGui::GetIO().MousePos.x < fa.x) { ImGui::SetTooltip("%s", name.c_str()); }
    if (ImGui::IsItemHovered() && ImGui::CalcTextSize(wert.c_str()).x > fb.x - fa.x - fs * 0.6F) { ImGui::SetTooltip("%s", wert.c_str()); }
    g_bl.seiteText += (g_bl.seiteText.empty() ? "" : "|") + name + "=" + wert;
    return ImRect{fa, fb};
}

// Ueberschrift eines Panels (wie Blender: Pfeil und Titel auf einem leicht abgesetzten Balken)
bool seitenPanel(const char* titel) {
    const float fs = ImGui::GetFontSize();
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    const bool offen = ImGui::CollapsingHeader(titel, ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();
    if (offen) { ImGui::Dummy(ImVec2{1.0F, std::floor(fs * 0.15F)}); }
    return offen;
}

std::string zahl(float v) {
    char z[48];
    // ohne "-0.00": was auf zwei Stellen gerundet null ist, steht als 0.00
    std::snprintf(z, sizeof z, "%.2f", std::fabs(v) < 0.005F ? 0.0 : static_cast<double>(v));
    return z;
}

std::string grad(float v) {
    char z[48];
    std::snprintf(z, sizeof z, "%.1f\xc2\xb0", std::fabs(v) < 0.05F ? 0.0 : static_cast<double>(v));
    return z;
}

// Euler-Winkel XYZ (Blenders Vorgabe: erst X, dann Y, dann Z) aus der Drehung einer Knochenmatrix (Spalten normiert, Massstab
// heraus).
void eulerXyz(const BoneMatrix& b, float out[3]) {
    float r[3][3];
    for (int c = 0; c < 3; ++c) {
        const float l = std::sqrt(b.m[0][c] * b.m[0][c] + b.m[1][c] * b.m[1][c] + b.m[2][c] * b.m[2][c]);
        for (int z = 0; z < 3; ++z) { r[z][c] = l > 1e-6F ? b.m[z][c] / l : 0.0F; }
    }
    out[1] = std::asin(std::clamp(-r[2][0], -1.0F, 1.0F));
    out[0] = std::atan2(r[2][1], r[2][2]);
    out[2] = std::atan2(r[1][0], r[0][0]);
    for (int i = 0; i < 3; ++i) { out[i] *= 180.0F / kPi; }
}

// --- Seitenleiste eingebbar (ENTSCHEID1, shank 12.11. "ja Bitte wie in Blender") ---------------------------------------------
// Wie Blenders Zahlenfelder (UI_BTYPE_NUM): nach links/rechts ziehen aendert den Wert, ein Klick ohne Ziehen tippt ihn ein (Enter
// uebernimmt, Esc bricht ab), Strg+Z nimmt die ganze Eingabe in einem Schritt zurueck. Flaeche: Mitte und Abmessungen ihres Kastens
// in der Pose (wie Blenders Location/Dimensions bzw. Median im Bearbeitungsmodus) - verschiebt bzw. skaliert die Ecken wie das
// Gizmo. Griff des Rigs im Posenmodus: Ort und Drehung (Euler XYZ) des Knochens - wie G/R bzw. das Gizmo (modellRigZiehen), mit
// denselben Grenzen (modellRigZugMoeglich). Ein Knochen ausserhalb des Posenmodus steht nur zum Ablesen da (Blender zeigt den Knochen
// erst im Posenmodus).

// Das Feld, das gerade gezogen oder getippt wird.
struct SeitenZug {
    std::string id;
    float echt = 0.0F;          // der Wert beim Anfassen (ungerundet)
    float wert = 0.0F;          // der Wert, den das Feld zeigt
    std::uint64_t schritt = 0;  // Flaeche: Rueckgaengig-Schritt (0: noch keiner)
    modell::Inhalt vorher;      // Flaeche: der Inhalt beim Anfassen
    float mitte[3]{};
    float euler[3]{};           // Griff: die Drehung beim Anfassen (Euler XYZ)
    BoneMatrix dreh = BoneMatrix::identity();   // Griff: die Drehung beim Anfassen (Spalten normiert, ohne Ort)
};
SeitenZug g_zug;

struct FeldStatus {
    bool anfang = false;   // in diesem Bild angefasst
    bool wert = false;     // der Wert hat sich geaendert (neu)
    bool ende = false;     // in diesem Bild losgelassen (Enter, Esc, Klick daneben)
    float neu = 0.0F;
};

// Ein Zahlenfeld der Seitenleiste in der Form von seitenZeile. `jetzt` ist der Wert, wie er gerade ist; solange das Feld angefasst
// ist, zeigt es den eingegebenen. Nicht eingebbar: dieselbe Zeile zum Ablesen, der Tooltip sagt warum.
FeldStatus seitenFeld(const std::string& id, const std::string& name, float jetzt, bool eingebbar, float tempo, float kleinste, const char* format,
                      const std::string& anzeige, const char* gesperrt) {
    FeldStatus s;
    if (!eingebbar) {
        const ImRect r = seitenZeile(name, anzeige, true);
        if (ImGui::IsItemHovered() && gesperrt != nullptr && ImGui::GetIO().MousePos.x >= r.Min.x) { feldTooltip(gesperrt); }
        const float links = std::floor(r.Min.x - ImGui::GetItemRectMin().x);
        const std::string zeigt = gekuerzt(name, links - ImGui::GetFontSize() * 0.4F);
        g_bl.seitenFelder.push_back({id, r, false, true, zeigt, ImGui::CalcTextSize(zeigt.c_str()).x, links - ImGui::GetFontSize() * 0.4F});
        return s;
    }
    const float breite = ImGui::GetContentRegionAvail().x;
    const float links = std::floor(breite * 0.42F);
    const float fs = ImGui::GetFontSize();
    const float h = std::floor(fs * 1.35F);
    const ImVec2 o = ImGui::GetCursorScreenPos();
    const std::string zeigt = gekuerzt(name, links - fs * 0.4F);
    const ImVec2 nb = ImGui::CalcTextSize(zeigt.c_str());
    ImGui::GetWindowDrawList()->AddText(ImVec2{std::max(o.x, o.x + links - fs * 0.4F - nb.x), o.y + (h - nb.y) * 0.5F}, ImGui::GetColorU32(ImGuiCol_TextDisabled), zeigt.c_str());
    ImGui::Dummy(ImVec2{links, h});
    if (zeigt != name && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", name.c_str()); }
    ImGui::SameLine(0.0F, 0.0F);
    ImGui::SetNextItemWidth(breite - links);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{std::floor(fs * 0.3F), std::max(0.0F, std::floor((h - 2.0F - fs) * 0.5F))});
    ImGuiIO& io = ImGui::GetIO();
    const bool klickTippt = io.ConfigDragClickToInputText;
    io.ConfigDragClickToInputText = true;   // wie Blender: ein Klick ohne Ziehen tippt ein (nur fuer diese Felder)
    float v = g_zug.id == id ? g_zug.wert : jetzt;
    s.wert = ImGui::DragFloat(("##" + id).c_str(), &v, tempo, kleinste, kleinste > 0.0F ? 1e9F : 0.0F, format, kleinste > 0.0F ? ImGuiSliderFlags_AlwaysClamp : ImGuiSliderFlags_None);
    io.ConfigDragClickToInputText = klickTippt;
    ImGui::PopStyleVar();
    s.neu = v;
    // angefasst/losgelassen ueber IsItemActive: wird aus dem Ziehen ein Eintippen (Klick ohne Ziehen), meldet ImGui fuer dasselbe
    // Feld kurz "deaktiviert" (TempInputText raeumt die ActiveId und setzt sie neu) - das ist kein Loslassen
    const bool aktiv = ImGui::IsItemActive();
    s.anfang = aktiv && g_zug.id != id;
    s.ende = !aktiv && g_zug.id == id;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && !ImGui::IsItemActive()) { feldTooltip(tr(Str::BeFeldHint)); }
    g_bl.seitenFelder.push_back({id, ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()}, true, false, zeigt, nb.x, links - fs * 0.4F});
    g_bl.seiteText += (g_bl.seiteText.empty() ? "" : "|") + name + "=" + anzeige;
    return s;
}

// Euler XYZ (Grad) -> Drehung R = Rz * Ry * Rx (wie eulerXyz zurueck)
BoneMatrix ausEuler(const float e[3]) {
    const float x[3] = {1.0F, 0.0F, 0.0F};
    const float y[3] = {0.0F, 1.0F, 0.0F};
    const float z[3] = {0.0F, 0.0F, 1.0F};
    return modell::drehungUm(z, e[2]) * modell::drehungUm(y, e[1]) * modell::drehungUm(x, e[0]);
}

// Die Drehung einer Knochenmatrix ohne Massstab und Ort.
BoneMatrix nurDrehung(const BoneMatrix& b) {
    BoneMatrix r = BoneMatrix::identity();
    for (int c = 0; c < 3; ++c) {
        const float l = std::sqrt(b.m[0][c] * b.m[0][c] + b.m[1][c] * b.m[1][c] + b.m[2][c] * b.m[2][c]);
        for (int z = 0; z < 3; ++z) { r.m[z][c] = l > 1e-6F ? b.m[z][c] / l : 0.0F; }
    }
    return r;
}

BoneMatrix transponiert(const BoneMatrix& b) {
    BoneMatrix r = BoneMatrix::identity();
    for (int z = 0; z < 3; ++z) {
        for (int c = 0; c < 3; ++c) { r.m[z][c] = b.m[c][z]; }
    }
    return r;
}

// Einheiten je Bildpunkt beim Ziehen eines Orts: wie G und das Gizmo in der aktiven Ansicht (modellRigModalTasten)
float ortTempo() {
    const ModellTab& t = mt();
    const ModellAnsicht& a = t.ansichten[static_cast<std::size_t>(std::clamp(t.aktiv, 0, 3))];
    return 2.0F * std::max(a.halbHoehe, 1.0F) / std::max(a.groesse[1], 1.0F);
}
// Grad je Bildpunkt beim Ziehen einer Drehung: wie das Dreh-Gizmo und R (gizmoZiehen, modellRigModalTasten: 0,5 Grad je Punkt)
constexpr float kDrehTempo = 0.5F;
// Abmessungen: 1 % je Bildpunkt wie das Skalier-Gizmo (gizmoZiehen: Faktor 1 + dx * 0.01)
constexpr float kMassTempo = 0.01F;

float gerundet(float v, float stelle) { return std::fabs(v) < stelle ? 0.0F : v; }   // ohne "-0.00" (AP4 Fund 2)

void zugLoslassenFlaeche() {
    if (g_zug.schritt != 0 && std::fabs(g_zug.wert - g_zug.echt) < 1e-6F) {
        (void)mt().dok.abbrechen(g_zug.schritt);   // nichts geaendert (Esc): kein leerer Rueckgaengig-Schritt
        --g_bl.seitenSchritte;
    }
    g_zug = SeitenZug{};
}

void zugLoslassenRig() {
    ModellTab& t = mt();
    if (t.rigZieht) {
        if (std::fabs(g_zug.wert - g_zug.echt) < 1e-6F) {
            (void)t.dok.abbrechen(t.rigSchritt);
            --g_bl.seitenSchritte;
        }
        modellRigZiehenEnde();
    }
    g_zug = SeitenZug{};
}

void seiteObjekt() {
    ModellTab& t = mt();
    if (seitenPanel(tr(Str::BzTransform))) {
        // ein Griff des Rigs bzw. ein Knochen: Ort und Drehung des Knochens in der Pose
        int knochen = -1;
        std::string name;
        const bool griff = t.bereich == ModellBereich::Animation && t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size();
        if (griff) {
            knochen = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)].knochen;
            if (knochen >= 0 && static_cast<std::size_t>(knochen) < g_app->anim.bones.size()) {
                char z[160];
                std::snprintf(z, sizeof z, tr(Str::BzGriff), g_app->anim.bones[static_cast<std::size_t>(knochen)].name.c_str());
                name = z;
            }
        } else if (t.knochenWahl >= 0 && static_cast<std::size_t>(t.knochenWahl) < g_app->anim.bones.size()) {
            knochen = t.knochenWahl;
            char z[160];
            std::snprintf(z, sizeof z, tr(Str::BzKnochen), g_app->anim.bones[static_cast<std::size_t>(knochen)].name.c_str());
            name = z;
        }
        const char* achse[3] = {"X", "Y", "Z"};
        const auto beschriftung = [&](Str was, int i) { return std::string(i == 0 ? tr(was) : "") + " " + achse[i]; };
        const auto undoName = [&](Str was, int i) {
            char z[200];
            std::snprintf(z, sizeof z, tr(Str::BeUndoSeite), (std::string(tr(was)) + " " + achse[i]).c_str());
            return std::string(z);
        };
        if (knochen >= 0 && static_cast<std::size_t>(knochen) < g_app->modellWelt.size()) {
            const BoneMatrix& b = g_app->modellWelt[static_cast<std::size_t>(knochen)];
            (void)seitenZeile(tr(Str::BzGewaehlt), name);
            const bool ortFrei = griff && modellRigZugMoeglich(1, nullptr);
            const bool drehFrei = griff && modellRigZugMoeglich(2, nullptr);
            const char* gesperrt = griff ? tr(Str::BeFeldGesperrt) : tr(Str::BeFeldNurPose);
            for (int i = 0; i < 3; ++i) {
                const std::string id = "ort" + std::to_string(i);
                const float jetzt = b.m[i][3];
                const FeldStatus s = seitenFeld(id, beschriftung(Str::BzOrt, i), gerundet(jetzt, 0.005F), ortFrei, ortTempo(), 0.0F, "%.2f", zahl(jetzt), gesperrt);
                if (s.anfang) {
                    g_zug = SeitenZug{};
                    g_zug.id = id;
                    g_zug.echt = jetzt;
                    g_zug.wert = gerundet(jetzt, 0.005F);
                }
                if (s.wert && g_zug.id == id) {
                    g_zug.wert = s.neu;
                    if (!t.rigZieht) { ++g_bl.seitenSchritte; }
                    float w[3] = {0.0F, 0.0F, 0.0F};
                    w[i] = s.neu - g_zug.echt;
                    modellRigZiehen(1, w);
                }
                if (s.ende && g_zug.id == id) { zugLoslassenRig(); }
            }
            float e[3];
            eulerXyz(b, e);
            for (int i = 0; i < 3; ++i) {
                const std::string id = "dreh" + std::to_string(i);
                const FeldStatus s = seitenFeld(id, beschriftung(Str::BzDrehung, i), gerundet(e[i], 0.05F), drehFrei, kDrehTempo, 0.0F, "%.1f\xc2\xb0", grad(e[i]), gesperrt);
                if (s.anfang) {
                    g_zug = SeitenZug{};
                    g_zug.id = id;
                    g_zug.echt = e[i];
                    g_zug.wert = gerundet(e[i], 0.05F);
                    std::copy(e, e + 3, g_zug.euler);
                    g_zug.dreh = nurDrehung(b);
                }
                if (s.wert && g_zug.id == id) {
                    g_zug.wert = s.neu;
                    if (!t.rigZieht) { ++g_bl.seitenSchritte; }
                    float ziel[3] = {g_zug.euler[0], g_zug.euler[1], g_zug.euler[2]};
                    ziel[i] = g_zug.euler[i] + (s.neu - g_zug.echt);
                    // die Drehung, die den Knochen von seiner Drehung beim Anfassen auf die eingegebene bringt (in Weltachsen)
                    modellRigDrehen(ausEuler(ziel) * transponiert(g_zug.dreh));
                }
                if (s.ende && g_zug.id == id) { zugLoslassenRig(); }
            }
        } else if (!t.auswahl.empty()) {
            // Flaechen: Mitte und Abmessungen des Kastens in der Pose
            std::string namen;
            if (t.auswahl.size() == 1 && static_cast<std::size_t>(t.auswahl.front()) < t.anzeige.surfaces.size()) {
                namen = t.anzeige.surfaces[static_cast<std::size_t>(t.auswahl.front())].name;
            } else {
                char z[96];
                std::snprintf(z, sizeof z, trN(Str::BzFlaechen, static_cast<int>(t.auswahl.size())), static_cast<int>(t.auswahl.size()));
                namen = z;
            }
            (void)seitenZeile(tr(Str::BzGewaehlt), namen);
            float mn[3];
            float mx[3];
            if (modellGrenzen(true, mn, mx)) {
                float m[3];
                for (int i = 0; i < 3; ++i) { m[i] = (mn[i] + mx[i]) * 0.5F; }
                const auto anfassen = [&](const std::string& id, float echt, float zeigt) {
                    g_zug = SeitenZug{};
                    g_zug.id = id;
                    g_zug.echt = echt;
                    g_zug.wert = zeigt;
                    g_zug.vorher = t.dok.inhalt();
                    std::copy(m, m + 3, g_zug.mitte);
                };
                const auto schritt = [&](Str was, int i) {
                    if (g_zug.schritt == 0) {
                        g_zug.schritt = t.dok.aendern(undoName(was, i), [](modell::Inhalt&) {});
                        ++g_bl.seitenSchritte;
                    }
                };
                for (int i = 0; i < 3; ++i) {
                    const std::string id = "ort" + std::to_string(i);
                    const FeldStatus s = seitenFeld(id, beschriftung(Str::BzOrt, i), gerundet(m[i], 0.005F), true, ortTempo(), 0.0F, "%.2f", zahl(m[i]), nullptr);
                    if (s.anfang) { anfassen(id, m[i], gerundet(m[i], 0.005F)); }
                    if (s.wert && g_zug.id == id) {
                        g_zug.wert = s.neu;
                        schritt(Str::BzOrt, i);
                        float w[3] = {0.0F, 0.0F, 0.0F};
                        w[i] = s.neu - g_zug.echt;
                        modellAuswahlUmformen(g_zug.vorher, 1, g_zug.mitte, w);
                    }
                    if (s.ende && g_zug.id == id) { zugLoslassenFlaeche(); }
                }
                for (int i = 0; i < 3; ++i) {
                    const std::string id = "masse" + std::to_string(i);
                    const float d = mx[i] - mn[i];
                    // eine flache Flaeche (Abmessung 0) laesst sich nicht skalieren - wie das Gizmo
                    const FeldStatus s = seitenFeld(id, beschriftung(Str::BzMasse, i), gerundet(d, 0.005F), d > 1e-4F, std::max(d * kMassTempo, 0.001F), 0.001F, "%.2f",
                                                    zahl(d), nullptr);
                    if (s.anfang) { anfassen(id, d, gerundet(d, 0.005F)); }
                    if (s.wert && g_zug.id == id && g_zug.echt > 1e-4F) {
                        g_zug.wert = s.neu;
                        schritt(Str::BzMasse, i);
                        float f[3] = {1.0F, 1.0F, 1.0F};
                        f[i] = s.neu / g_zug.echt;
                        modellAuswahlUmformen(g_zug.vorher, 3, g_zug.mitte, f);
                    }
                    if (s.ende && g_zug.id == id) { zugLoslassenFlaeche(); }
                }
            }
        } else {
            (void)seitenZeile(tr(Str::BzGewaehlt), tr(Str::BzNichtsGewaehlt));
        }
    }
    if (seitenPanel(tr(Str::BzMessen))) {
        float d = 0.0F;
        if (modellBlAbstand(d)) {
            (void)seitenZeile(tr(Str::BzAbstand), einheiten(d));
        } else {
            ImGui::PushTextWrapPos(0.0F);
            cjkTextDisabled("%s", tr(Str::BzKeineMessung));
            ImGui::PopTextWrapPos();
        }
    }
}

void seiteAnsicht() {
    if (!seitenPanel(tr(Str::BzReiterAnsicht))) { return; }
    const float fs = ImGui::GetFontSize();
    (void)seitenZeile(tr(Str::BzSichtfeld), grad(g_app->modelCam.fovY));
    ImGui::SetNextItemWidth(-1.0F);
    float fov = g_app->modelCam.fovY;
    if (ImGui::SliderFloat("##blfov", &fov, 10.0F, 120.0F, "%.0f\xc2\xb0")) {
        g_app->modelCam.fovY = fov;
        g_app->modelDirty = true;
    }
    ImGui::SetItemTooltip("%s", tr(Str::BzSichtfeld));
    const ModellAnsicht& a = mt().ansichten[static_cast<std::size_t>(std::clamp(mt().aktiv, 0, 3))];
    (void)seitenZeile(tr(Str::BzAnsichtArt), a.art == AnsichtArt::Perspektive ? tr(Str::MkBlUserPersp) : tr(Str::MkBlUserOrtho));
    const float b = ImGui::GetContentRegionAvail().x;
    if (ImGui::Button(tr(Str::MkBlPersp), ImVec2{b, 0.0F})) { modellPerspektiveUmschalten(); }
    ImGui::SetItemTooltip("%s", "Numpad 5");
    if (ImGui::Button(tr(Str::MkBlGewaehlte), ImVec2{b, 0.0F})) { (void)modellNavBlEinpassen(true, false); }
    ImGui::SetItemTooltip("%s", "Numpad .");
    if (ImGui::Button(tr(Str::MkBlAlles), ImVec2{b, 0.0F})) { (void)modellNavBlEinpassen(false, false); }
    ImGui::SetItemTooltip("%s", "Home");
    ImGui::Dummy(ImVec2{1.0F, fs * 0.2F});
}

}  // namespace

ModellBlender& modellBl() { return g_bl; }

float modellLinksSpalte() {
    if (!modellBlender()) { return kModellSpalte; }
    if (!g_bl.werkzeugleiste) { return kStreifenBreite; }
    if (!g_app->settings.modellBlWerkzeugnamen) { return kLeisteBreite; }
    // ENTSCHEID1 "Werkzeugnamen zeigen": so breit, dass neben dem Symbol der laengste Name des Bereichs ganz steht (Blender zeigt die
    // Namen, wenn die Leiste breiter als 120 Punkte ist - space_toolsystem_common.py _layout_generator_detect_from_region)
    const float fs = std::max(ImGui::GetFontSize(), 1.0F);
    float laengster = 0.0F;
    for (const Werkzeug& w : werkzeuge()) { laengster = std::max(laengster, ImGui::CalcTextSize(w.text.c_str()).x); }
    return kLeisteBreite + laengster / fs + 0.8F;
}

const char* modellLinksTitel() { return modellBlender() ? tr(Str::BzWerkzeugleiste) : modellExplorerTitel(); }

int modellBlWerkzeugZahl() { return static_cast<int>(werkzeuge().size()); }

const char* modellBlWerkzeugText(int i) {
    static std::string text;
    const std::vector<Werkzeug> w = werkzeuge();
    text = i >= 0 && static_cast<std::size_t>(i) < w.size() ? w[static_cast<std::size_t>(i)].text : std::string();
    return text.c_str();
}

bool modellBlWerkzeugAn(int i) {
    const std::vector<Werkzeug> w = werkzeuge();
    return i >= 0 && static_cast<std::size_t>(i) < w.size() && w[static_cast<std::size_t>(i)].an;
}

bool modellBlWerkzeugAusloesen(int i) {
    const std::vector<Werkzeug> w = werkzeuge();
    if (i < 0 || static_cast<std::size_t>(i) >= w.size() || !w[static_cast<std::size_t>(i)].aktiv) { return false; }
    ausloesen(w[static_cast<std::size_t>(i)]);
    return true;
}

void modellBlGruppenVorgabe() { g_gruppenWahl.clear(); }

bool modellBlAbstand(float& abstand) {
    if (g_bl.messPunkte < 2) { return false; }
    abstand = abstandVon(g_bl.messA, g_bl.messB);
    return true;
}

void drawModellWerkzeugleiste(float hoehe) {
    const float fs = ImGui::GetFontSize();
    if (g_bl.bild != ImGui::GetFrameCount()) {
        g_bl.knoepfe.clear();
        g_bl.gruppen.clear();
        g_bl.dreiecke.clear();
        g_bl.namenGezeigt = false;
        g_bl.bild = ImGui::GetFrameCount();
    }
    g_bl.trennstrich = -1;
    if (!g_bl.werkzeugleiste) {
        // aus: ein Streifen mit dem Pfeil, der sie wieder zeigt (wie Blender am Rand der Ansicht)
        const ImVec2 o = ImGui::GetCursorScreenPos();
        const float b = std::max(ImGui::GetContentRegionAvail().x, 4.0F);
        if (ImGui::InvisibleButton("##blleisteauf", ImVec2{b, std::floor(fs * 1.6F)})) { g_bl.werkzeugleiste = true; }
        const bool ueber = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (ueber) { dl->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_ButtonHovered), 3.0F); }
        const ImVec2 m{o.x + b * 0.5F, o.y + fs * 0.8F};
        const float s = fs * 0.28F;
        dl->AddTriangleFilled(ImVec2{m.x - s * 0.6F, m.y - s}, ImVec2{m.x + s * 0.7F, m.y}, ImVec2{m.x - s * 0.6F, m.y + s}, ImGui::GetColorU32(ImGuiCol_Text));
        if (ueber) { ImGui::SetTooltip("%s", tr(Str::BzLeisteAufHint)); }
        g_bl.leiste = ImRect{o, ImVec2{o.x + b, o.y + hoehe}};
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::BeginChild("mtwerkzeugleiste", ImVec2{-1.0F, hoehe}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    g_bl.leiste = ImRect{ImGui::GetWindowPos(), ImVec2{ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y}};
    const float breite = std::floor(std::max(ImGui::GetContentRegionAvail().x, fs));
    const float luecke = std::floor(fs * 0.15F);
    const std::vector<Werkzeug> w = werkzeuge();
    // so gross wie die Spalte breit ist; reicht die Hoehe nicht fuer alle Knoepfe (kleines Fenster), kleiner und mittig. Mit Namen
    // bleibt das Symbol so gross wie in der schmalen Leiste, der Knopf reicht ueber die ganze Breite.
    const bool namen = g_app->settings.modellBlWerkzeugnamen;
    const float quadrat = namen ? std::min(breite, std::floor(fs * kLeisteBreite)) : breite;
    const float n = static_cast<float>(w.size());
    const float passt = (ImGui::GetContentRegionAvail().y - luecke * 2.0F - luecke * n) / std::max(n, 1.0F);
    const float g = std::floor(std::clamp(passt, std::min(fs * 1.3F, quadrat), quadrat));
    const float rand = namen ? 0.0F : std::floor((breite - g) * 0.5F);
    const float strich = namen ? breite : g;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, luecke});
    if (rand > 0.0F) { ImGui::Indent(rand); }
    for (std::size_t i = 0; i < w.size(); ++i) {
        if (i == 4) {
            // Trennstrich zwischen den vier Werkzeugen jedes Bereichs und denen des Bereichs (Entwurf: <hr>)
            const ImVec2 o = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2{strich, luecke * 2.0F});
            ImGui::GetWindowDrawList()->AddLine(ImVec2{o.x + strich * 0.15F, o.y + luecke}, ImVec2{o.x + strich * 0.85F, o.y + luecke}, ImGui::GetColorU32(ImGuiCol_Separator));
            g_bl.trennstrich = 4;
        }
        if (leistenKnopf(w[i], g, breite)) { ausloesen(w[i]); }
        if (ImGui::BeginPopupContextItem()) {
            leistenMenue();
            ImGui::EndPopup();
        }
    }
    if (rand > 0.0F) { ImGui::Unindent(rand); }
    ImGui::PopStyleVar();
    gruppenMenue();
    if (ImGui::BeginPopupContextWindow("##blleistemenue", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        leistenMenue();
        ImGui::EndPopup();
    }
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { mt().klickDrinBild = ImGui::GetFrameCount(); }
    ImGui::EndChild();
}

float drawModellBlOutliner(float breite, float hoehe) {
    const float fs = ImGui::GetFontSize();
    const float trennH = std::floor(fs * 0.5F);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float unten = std::min(fs * 6.0F, hoehe * 0.5F);
    const float oben = std::floor(std::clamp(hoehe * g_bl.outlinerAnteil, unten, std::max(std::max(hoehe - fs * 8.0F, hoehe * 0.3F), unten)));
    ImGui::BeginGroup();
    ImGui::TextUnformatted(modellExplorerTitel());
    const float titelH = ImGui::GetCursorScreenPos().y - a.y;
    drawModellExplorer(std::max(oben - titelH, fs * 3.0F));
    ImGui::EndGroup();
    g_bl.outliner = ImRect{a, ImVec2{a.x + breite, ImGui::GetItemRectMax().y}};
    // die Trennkante zwischen Outliner und Eigenschaften (ziehbar wie die Grenze zweier Bereiche in Blender)
    const ImVec2 t0 = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##bloutlinertrenner", ImVec2{std::max(breite, 4.0F), trennH});
    const bool ueber = ImGui::IsItemHovered();
    const bool zieht = ImGui::IsItemActive();
    if (ueber || zieht) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        const float y = std::floor(t0.y + trennH * 0.5F);
        ImGui::GetWindowDrawList()->AddLine(ImVec2{t0.x, y}, ImVec2{t0.x + breite, y}, ImGui::GetColorU32(zieht ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered), 2.0F);
    }
    if (ueber && !zieht) { ImGui::SetItemTooltip("%s", tr(Str::BzTrennerHint)); }
    if (zieht && hoehe > 1.0F) { g_bl.outlinerAnteil = std::clamp((ImGui::GetIO().MousePos.y - a.y - trennH * 0.5F) / hoehe, 0.15F, 0.8F); }
    return std::max(hoehe - (ImGui::GetCursorScreenPos().y - a.y), fs * 4.0F);
}

float modellSeitenleisteBreite(float frei) {
    if (!modellBlender() || !g_bl.seitenleiste) { return 0.0F; }
    const float fs = ImGui::GetFontSize();
    return std::floor(std::min(fs * kSeiteBreite, frei / 3.0F));
}

void drawModellSeitenleiste(ImVec2 groesse) {
    const float fs = ImGui::GetFontSize();
    g_bl.seiteText.clear();
    g_bl.seitenFelder.clear();
    const ImVec2 o = ImGui::GetCursorScreenPos();
    g_bl.seite = ImRect{o, ImVec2{o.x + groesse.x, o.y + groesse.y}};
    // die Reiter am rechten Rand, senkrecht beschriftet (Blender: Item, Tool, View)
    const float reiterB = std::floor(fs * 1.5F);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const Str reiter[2] = {Str::BzReiterObjekt, Str::BzReiterAnsicht};
    float y = o.y;
    const ImVec4 beton = ui::betonungsFarbe();
    for (int i = 0; i < 2; ++i) {
        const char* text = tr(reiter[i]);
        const float lang = std::floor(ImGui::CalcTextSize(text).x + fs * 1.2F);
        ImGui::SetCursorScreenPos(ImVec2{o.x + groesse.x - reiterB, y});
        if (ImGui::InvisibleButton(text, ImVec2{reiterB, lang})) { g_bl.seitenReiter = i; }
        const bool an = g_bl.seitenReiter == i;
        const bool ueber = ImGui::IsItemHovered();
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        dl->AddRectFilled(a, b, an ? ImGui::GetColorU32(beton) : ImGui::GetColorU32(ueber ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBg), std::floor(fs * 0.2F));
        textSenkrecht(dl, ImVec2{(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F}, text, an ? (helligkeit(beton) > 0.6F ? IM_COL32_BLACK : IM_COL32_WHITE) : ImGui::GetColorU32(ImGuiCol_Text));
        y += lang + std::floor(fs * 0.15F);
    }
    ImGui::SetCursorScreenPos(o);
    ImGui::BeginChild("mtseiteninhalt", ImVec2{std::max(groesse.x - reiterB - fs * 0.3F, fs * 4.0F), groesse.y}, ImGuiChildFlags_None);
    if (g_bl.seitenReiter == 0) {
        seiteObjekt();
    } else {
        seiteAnsicht();
    }
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { mt().klickDrinBild = ImGui::GetFrameCount(); }
    ImGui::EndChild();
}

void modellBlUeberlagerung(int n, const ModellAnsicht& a, ImDrawList* dl) {
    const ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    if (n == t.aktiv) {
        g_bl.hinweis = ImRect{};
        if (!modellBlender()) {
            g_bl.modusFeld = ImRect{};
            g_bl.modusText.clear();
        }
    }
    // die Messung (Werkzeug Messen in Ansehen/Pruefen): Punkte, Strecke und der Abstand in der Mitte
    if (t.bereich == ModellBereich::Ansehen && g_bl.werkzeug == 1 && g_bl.messPunkte > 0) {
        ImVec2 pa;
        ImVec2 pb;
        const bool da = modellAufSchirm(a, g_bl.messA, pa);
        const bool db = g_bl.messPunkte > 1 && modellAufSchirm(a, g_bl.messB, pb);
        const ImU32 gelb = IM_COL32(255, 210, 60, 255);
        if (da && db) {
            dl->AddLine(pa, pb, IM_COL32(0, 0, 0, 160), 4.0F);
            dl->AddLine(pa, pb, gelb, 2.0F);
            const std::string text = einheiten(abstandVon(g_bl.messA, g_bl.messB));
            const ImVec2 g = ImGui::CalcTextSize(text.c_str());
            const ImVec2 m{std::floor((pa.x + pb.x - g.x) * 0.5F), std::floor((pa.y + pb.y) * 0.5F - g.y - fs * 0.3F)};
            dl->AddRectFilled(ImVec2{m.x - 4.0F, m.y - 2.0F}, ImVec2{m.x + g.x + 4.0F, m.y + g.y + 2.0F}, IM_COL32(20, 20, 24, 210), 3.0F);
            dl->AddText(m, gelb, text.c_str());
        }
        if (da) { dl->AddCircleFilled(pa, fs * 0.22F, gelb); }
        if (db) { dl->AddCircleFilled(pb, fs * 0.22F, gelb); }
    }
    // FIX612C (P3-6): wie 3ds Max steht oben in der Mitte, dass "Knochen waehlen" an ist (ein Klick waehlt dann keine Flaeche)
    if (!modellBlender() && n == t.aktiv && t.bereich == ModellBereich::Ansehen && g_bl.werkzeug == 2) {
        const char* text = tr(Str::BzKnochenAnzeige);
        const ImVec2 g = ImGui::CalcTextSize(text);
        if (g.x + fs * 2.0F < a.groesse[0]) {
            const ImVec2 p{std::floor(a.lo[0] + (a.groesse[0] - g.x) * 0.5F), std::floor(a.lo[1] + fs * 2.2F)};
            dl->AddRectFilled(ImVec2{p.x - fs * 0.4F, p.y - fs * 0.15F}, ImVec2{p.x + g.x + fs * 0.4F, p.y + g.y + fs * 0.15F}, IM_COL32(20, 20, 24, 210), 3.0F);
            dl->AddText(p, IM_COL32(255, 210, 60, 255), text);
            fix612cProbe().knochenHinweisBild = ImGui::GetFrameCount();
        }
    }
    // unten rechts der Hinweis auf die beiden Leisten (Entwurf: in den Bereichen ausser Ansehen/Pruefen, dort steht die Detailstufe)
    if (modellBlender() && n == t.aktiv && t.bereich != ModellBereich::Ansehen) {
        const char* text = tr(Str::BzHinweisAnsicht);
        const ImVec2 g = ImGui::CalcTextSize(text);
        if (g.x + fs * 2.0F < a.groesse[0]) {
            const ImVec2 p{std::floor(a.lo[0] + a.groesse[0] - g.x - fs * 0.5F), std::floor(a.lo[1] + a.groesse[1] - g.y - fs * 0.35F)};
            dl->AddText(p, IM_COL32(200, 200, 200, 170), text);
            g_bl.hinweis = ImRect{p, ImVec2{p.x + g.x, p.y + g.y}};
        }
    }
}

bool modellBlKlick(const ModellAnsicht& a, bool klick, ImVec2 start, ImVec2 ende) {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Ansehen || (g_bl.werkzeug != 1 && g_bl.werkzeug != 2)) { return false; }
    if (g_bl.werkzeug == 2) {
        const int b = knochenUnter(a, ende);
        t.knochenWahl = b;
        if (b >= 0) { modellExplorerZeigen(ExplorerArt::Knochen, b); }
        g_app->modelDirty = true;
        return true;
    }
    float p[3];
    float q[3];
    if (!klick) {
        // gezogen wie Blenders Lineal: von start nach ende
        if (!punktUnter(a, start, p) || !punktUnter(a, ende, q)) {
            melde(tr(Str::BzDaneben));
            return true;
        }
        std::copy(p, p + 3, g_bl.messA);
        std::copy(q, q + 3, g_bl.messB);
        g_bl.messPunkte = 2;
    } else if (!punktUnter(a, ende, p)) {
        melde(tr(Str::BzDaneben));
        return true;
    } else if (g_bl.messPunkte != 1) {
        std::copy(p, p + 3, g_bl.messA);
        g_bl.messPunkte = 1;
        melde(tr(Str::BzMessenZweiter));
        return true;
    } else {
        std::copy(p, p + 3, g_bl.messB);
        g_bl.messPunkte = 2;
    }
    melde(std::string(tr(Str::BzAbstand)) + ": " + einheiten(abstandVon(g_bl.messA, g_bl.messB)));
    g_app->modelDirty = true;
    return true;
}

const char* modellBlModusName(ModellBereich b) {
    switch (b) {
        case ModellBereich::Ansehen: return tr(Str::MbBlObjektModus);
        case ModellBereich::Modellieren:
        case ModellBereich::Uv: return tr(Str::MbBlEditModus);
        case ModellBereich::Weights: return tr(Str::BzModusWeight);
        case ModellBereich::Painting: return tr(Str::BzModusTexture);
        case ModellBereich::Animation: return tr(Str::BzModusPose);
    }
    return "";
}

float modellBlModusKopf(ImDrawList* dl, int n, float x, float y, float hoehe, const char* ansichtName) {
    ModellTab& t = mt();
    const ImU32 leise = IM_COL32(200, 200, 200, 170);
    const std::string vorn = std::string(ansichtName) + " \xc2\xb7";
    const ImVec2 vg = ImGui::CalcTextSize(vorn.c_str());
    // der Modus als Ausklapper (Blender: das Feld "Object Mode" im Kopf, Strg+Tab)
    const char* modus = modellBlModusName(t.bereich);
    const std::string feld = std::string(modus) + "  v";
    const ImVec2 g = ImGui::CalcTextSize(feld.c_str());
    // Platz bis Overlays/Shading am rechten Rand (dieselbe Breite wie dort in drawModellKopfzeile): in einer schmalen Ansicht faellt
    // erst der Name der Ansicht weg, dann der Modus
    const ModellAnsicht& a = t.ansichten[static_cast<std::size_t>(std::clamp(n, 0, 3))];
    const float rechts = ImGui::CalcTextSize(tr(Str::MkBlOverlays)).x + 8.0F + 6.0F + hoehe * 3.0F + 6.0F + ImGui::CalcTextSize("v").x + 8.0F + 4.0F + 12.0F;
    const float frei = a.lo[0] + a.groesse[0] - rechts - x;
    const bool mitName = 6.0F + vg.x + 4.0F + g.x + 8.0F <= frei;
    if (n == t.aktiv) {
        g_bl.modusFeld = ImRect{};
        g_bl.modusText.clear();
    }
    if (!mitName && g.x + 8.0F > frei) { return x; }
    if (mitName) {
        dl->AddText(ImVec2{x + 6.0F, y + (hoehe - vg.y) * 0.5F}, leise, vorn.c_str());
        x += 6.0F + vg.x + 4.0F;
    }
    const ImVec2 o{x, y};
    const ImVec2 gr{g.x + 8.0F, hoehe};
    char id[24];
    std::snprintf(id, sizeof id, "##blmodus%d", n);
    ImGui::SetCursorScreenPos(o);
    ImGui::InvisibleButton(id, gr, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool ueber = ImGui::IsItemHovered();
    char popup[24];
    std::snprintf(popup, sizeof popup, "##blmodusmenue%d", n);
    const bool offen = ImGui::IsPopupOpen(popup);
    dl->AddRectFilled(o, ImVec2{o.x + gr.x, o.y + gr.y}, offen ? IM_COL32(255, 255, 255, 60) : (ueber ? IM_COL32(255, 255, 255, 40) : IM_COL32(0, 0, 0, 70)), 3.0F);
    dl->AddText(ImVec2{o.x + 4.0F, o.y + (hoehe - g.y) * 0.5F}, IM_COL32(235, 235, 235, 255), feld.c_str());
    if (ueber) { ImGui::SetItemTooltip("%s", tr(Str::BzModusHint)); }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        t.aktiv = n;
        t.klickDrinBild = ImGui::GetFrameCount();
        ImGui::OpenPopup(popup);
    }
    if (ImGui::IsPopupOpen(popup)) { ImGui::SetNextWindowPos(ImVec2{o.x, o.y + gr.y}, ImGuiCond_Appearing); }
    if (ImGui::BeginPopup(popup)) {
        // rechts daneben der Bereich, in den der Modus fuehrt (wie die Kuerzel-Spalte eines Menues)
        struct M {
            const char* name;
            ModellBereich b;
        };
        const M liste[5] = {{tr(Str::MbBlObjektModus), ModellBereich::Ansehen},
                            {tr(Str::MbBlEditModus), t.bereich == ModellBereich::Uv ? ModellBereich::Uv : ModellBereich::Modellieren},
                            {tr(Str::BzModusWeight), ModellBereich::Weights},
                            {tr(Str::BzModusTexture), ModellBereich::Painting},
                            {tr(Str::BzModusPose), ModellBereich::Animation}};
        for (const M& m : liste) {
            if (ImGui::MenuItem(m.name, tr(kBereichName[static_cast<int>(m.b)]), std::string(m.name) == modus)) {
                t.bereich = m.b;
                g_app->modelDirty = true;
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    float ende = o.x + gr.x;
    std::string sichtfeld;
    if (t.bereich == ModellBereich::Ansehen && a.art == AnsichtArt::Perspektive) {
        char z[48];
        std::snprintf(z, sizeof z, "\xc2\xb7 FOV %.0f\xc2\xb0", static_cast<double>(g_app->modelCam.fovY));
        const ImVec2 fg = ImGui::CalcTextSize(z);
        if (ende + 6.0F + fg.x <= a.lo[0] + a.groesse[0] - rechts) {
            dl->AddText(ImVec2{ende + 6.0F, y + (hoehe - fg.y) * 0.5F}, leise, z);
            ende += 6.0F + fg.x;
            sichtfeld = z;
        }
    }
    if (n == t.aktiv) {
        g_bl.modusFeld = ImRect{o, ImVec2{o.x + gr.x, o.y + gr.y}};
        g_bl.modusText = (mitName ? vorn + " " + modus : std::string(modus)) + (sichtfeld.empty() ? std::string() : " " + sichtfeld);
    }
    return ende;
}

bool modellBlTasten() {
    if (!modellBlender()) { return false; }
    ModellTab& t = mt();
    const auto gezaehlt = [&](Str text) {
        ++t.tastenZaehler;
        t.letzteTaste = tr(text);
        g_app->modelDirty = true;
        return true;
    };
    // blender_default.py km_view3d_generic: T show_region_toolbar, N show_region_ui (wm.context_toggle)
    if (ImGui::IsKeyChordPressed(ImGuiKey_T)) {
        g_bl.werkzeugleiste = !g_bl.werkzeugleiste;
        return gezaehlt(Str::BzWerkzeugleiste);
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_N)) {
        g_bl.seitenleiste = !g_bl.seitenleiste;
        return gezaehlt(Str::BzSeitenleiste);
    }
    return false;
}

bool modellBlAktionenSuche(const std::string& such) {
    struct Z {
        Str text;
        Str hinweis;
        const char* taste;
        bool nurBlender;
        std::function<void()> tun;
    };
    const Z liste[5] = {
        {Str::BzWerkzeugleiste, Str::BzWerkzeugleisteHint, "T", true, [] { g_bl.werkzeugleiste = !g_bl.werkzeugleiste; }},
        {Str::BzSeitenleiste, Str::BzSeitenleisteHint, "N", true, [] { g_bl.seitenleiste = !g_bl.seitenleiste; }},
        {Str::BeWerkzeugnamen, Str::BeWerkzeugnamenHint, "", true, [] { g_app->settings.modellBlWerkzeugnamen = !g_app->settings.modellBlWerkzeugnamen; }},
        {Str::BzMessenMax, Str::BzMessenHint, "", false, [] {
             mt().bereich = ModellBereich::Ansehen;
             g_bl.werkzeug = 1;
             g_bl.messPunkte = 0;
             melde(tr(Str::BzMessenErster));
         }},
        {Str::BzKnochenWaehlen, Str::BzKnochenWaehlenHint, "", false,
         [] { knochenWaehlen(modellBlender() || mt().bereich != ModellBereich::Ansehen || g_bl.werkzeug != 2); }},
    };
    const auto klein = [](std::string s) {
        for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return s;
    };
    bool ausgeloest = false;
    for (const Z& z : liste) {
        if (z.nurBlender && !modellBlender()) { continue; }
        std::string zeile = tr(z.text);
        if (z.taste[0] != '\0') { zeile += std::string(" (") + z.taste + ")"; }
        if (!such.empty() && klein(zeile).find(klein(such)) == std::string::npos) { continue; }
        ImGui::PushID(&z);
        if (modellAktionZeile(zeile.c_str()) && !ausgeloest) {
            z.tun();
            ++mt().tastenZaehler;
            mt().letzteTaste = tr(z.text);
            g_app->modelDirty = true;
            ausgeloest = true;
        }
        ImGui::SetItemTooltip("%s", tr(z.hinweis));
        ImGui::PopID();
    }
    return ausgeloest;
}

void modellBlKontextMenue() {
    ImGui::Separator();
    if (modellBlender()) { leistenMenue(); }
    const bool messen = mt().bereich == ModellBereich::Ansehen && g_bl.werkzeug == 1;
    if (ImGui::MenuItem(tr(Str::BzMessenMax), nullptr, messen)) {
        if (messen) {
            g_bl.werkzeug = 0;
        } else {
            mt().bereich = ModellBereich::Ansehen;
            g_bl.werkzeug = 1;
            g_bl.messPunkte = 0;
            melde(tr(Str::BzMessenErster));
        }
    }
    ImGui::SetItemTooltip("%s", tr(Str::BzMessenHint));
    if (!modellBlender()) {   // FIX612C (P3-6): wie Blender steht das Werkzeug in der Leiste (T)
        const bool knochen = mt().bereich == ModellBereich::Ansehen && g_bl.werkzeug == 2;
        if (ImGui::MenuItem(tr(Str::BzKnochenWaehlen), nullptr, knochen, !g_app->anim.bones.empty())) { knochenWaehlen(!knochen); }
        ImGui::SetItemTooltip("%s", tr(Str::BzKnochenWaehlenHint));
    }
}

}  // namespace bhed::gui
