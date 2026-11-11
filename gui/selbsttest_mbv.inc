// selbsttest_mbv.inc - mbvers: Verschieben, Drehen, Skalieren und Modellieren in einer Pose (BHED_EDITORTEST=modellblock,
// BHED_MODELL_NUR=mbv; Teile: mbvumform, mbvmesh)
//
//   shank 10.11.: "ein bug wenn ich was bewege, dann geht ein mesh nach links obwohl ich alles nach rechts bewege" (kyle in
//   BOTH_A2_TR_BL, der Kopf wanderte vom Koerper weg). Geprueft wird, was man sieht: jede gewaehlte Ecke bewegt sich in der
//   GEZEIGTEN (posierten) Ansicht genau um den gezogenen Weg in die gezogene Richtung, alle Flaechen gleich; Drehen und
//   Skalieren um die gezeigte Mitte; die gezeigten Normalen drehen mit.
//     ohne Animation  kyle, wie geladen (Bild 0 der _humanoid.gla): das ist NICHT die Bindepose (die Figur steht 24 bis 27
//                     tiefer, die Arme sind 48 Grad gedreht, die Beine 3 Grad) - Kopf und rechter Arm mit G/R/S und Zahl
//     Figur        kyle, BOTH_A2_TR_BL auf dem Bild mit den am staerksten gedrehten Knochen (cranium, Oberarme) angehalten
//     Auswahl      ganze Figur, nur der Kopf, Kopf und Koerper (Objektmodus, alle LOD-Stufen); Ecken des Kopfs (Unterobjekt)
//     je Ansicht   Oben, Vorne, Links, Perspektive: Gizmo an jedem sichtbaren Achsgriff, frei in der Bildebene (Mitte),
//                  Ebene XY (F8, Perspektive), Drehen (Ring der Blickachse bzw. Mitte), Skalieren (Mitte)
//     Zahl         G/R/S mit Achse und Zahl (Blender)
//     laufend      dieselben Wege bei laufender Animation (gegen die Pose, in der die Umformung gerechnet wurde)
//     danach       Rueckgaengig bitgleich wie vorher, Wiederholen bitgleich wie nachher; die .glm traegt die
//                  zurueckgerechneten Ecken (geschrieben, gelesen, verglichen)
//     Bindepose    dasselbe Skelett mit der Einheit als lokaler Matrix (exakt die Bindepose): der bisherige Rechenweg -
//                  G X 12 verschiebt jede Ecke der Datei um genau 12, die Normalen bleiben bitgleich
//     Modellieren  Extrude an der am staerksten gemischten Flaeche, Slice und QuickSlice in der gezeigten Ebene
//     Requisit     ein neues Requisit nach der Figur: ihr Skelett gilt ihm nicht (keine Pose, gezeigt an seinen Koordinaten)
// Eingebunden in selbsttest.cpp (nach selbsttest_mr.inc).
namespace mbv {

namespace w = modell::wof;

// --- Toleranzen: die Rechengenauigkeit, nichts dazu --------------------------------------------------------------------
// Gezeigte Orte: mb2::kRundungen float-Schritte der groessten Koordinate des Falls (hergeleitet in selbsttest_mb1a2.inc).
using mb2::kRundungen;
using mb2::schritt;
// Gezeigte Normalen (Laenge 1, ein float-Schritt 6e-8): die gezeigte Normale davor rundet einmal (double -> float), die
// danach zweimal (die Ecke der Datei, dann gezeigt), das Soll der Pruefung dreht in float mit drei Produkten und zwei
// Summen je Achse - zusammen unter 5 Schritten. Erlaubt 8 (4,8e-7).
constexpr float kTolNormale = 8.0F * 5.9604645e-8F;
// Bildpunkte: der Weg der Mitte auf dem Schirm gegen den Mausweg. Ein Schirmort entsteht in float aus vier Produkten und
// drei Summen je Zeile der Projektion, der Division durch w und dem Umrechnen auf das Fenster: je Koordinate hoechstens 12
// Rundungen von je einem halben float-Schritt des groessten Schirmorts, also 6 Schritte. Rechtwinklig gehen drei Orte ein
// (alte Mitte, neue Mitte, Achsrichtung): 18 Schritte (bei 2048 px 2,2e-3 px). In der Perspektive kommt der Massstab
// dazu (Bildpunkte je Einheit an alter Mitte, neuer Mitte und Griffspitze, jeder aus zwei Schirmorten): je Ort zwei mal 6
// Schritte auf seine Laenge, zwei Massstaebe im Verhaeltnis - der Mausweg mal 4 * 6 Schritte durch den kleinsten Massstab.
constexpr float kPxRundungen = 6.0F;

// Die groessten Abweichungen ueber alle Faelle (Schlusszeile)
struct Mess {
    float orte = 0.0F;      // in float-Schritten der groessten Koordinate des Falls
    float normale = 0.0F;   // absolut
    float px = 0.0F;        // rechtwinklige Ansichten
    float pxPersp = 0.0F;   // Perspektive
    float pxAnteil = 0.0F;  // die groesste Abweichung als Anteil ihrer Schranke
    int faelle = 0;
};

struct EckeNr {
    int teil = -1;
    int stufe = 0;
    int ecke = -1;
};

struct Stand {
    modell::Inhalt vorher;
    std::vector<EckeNr> ecken;   // die geprueften Ecken: im Objektmodus alle Stufen der gewaehlten Teile
    int stufe = 0;               // die gezeichnete Stufe (die Mitte der Auswahl)
    bool unterobjekt = false;
    int bild = 0;
    std::string fall;
    ImVec2 start{0.0F, 0.0F};    // der Zug: wo gedrueckt wurde, und der Mausweg
    ImVec2 maus{0.0F, 0.0F};
    int achse = -1;              // die Achse des Griffs dieses Falls (zur Laufzeit: die sichtbaren Achsen der Ansicht)
    bool zugOk = false;
    Mess mess;
};

ModellTab& t() { return g_app->mtab; }

int teilMit(const char* name) {
    const modell::Inhalt& in = t().dok.inhalt();
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

std::vector<int> gezeichneteTeile() {
    std::vector<int> v;
    for (std::size_t i = 0; i < t().gezeichnet.size() && i < t().dok.inhalt().teile.size(); ++i) {
        if (t().gezeichnet[i] != 0 && !t().dok.inhalt().teile[i].istTag()) { v.push_back(static_cast<int>(i)); }
    }
    return v;
}

const modell::Netz* netzVon(const modell::Inhalt& in, int teil, int stufe) {
    if (teil < 0 || static_cast<std::size_t>(teil) >= in.teile.size() || stufe < 0 || static_cast<std::size_t>(stufe) >= in.teile[static_cast<std::size_t>(teil)].stufen.size()) {
        return nullptr;
    }
    return &in.teile[static_cast<std::size_t>(teil)].stufen[static_cast<std::size_t>(stufe)];
}

// Der gezeigte Ort einer Ecke eines Inhalts in einer Pose - wie haut() die Ecken der Ansicht verformt (leer: Bindepose)
bool gezeigt(const modell::Inhalt& in, const EckeNr& o, const std::vector<BoneMatrix>& pose, float out[3]) {
    const modell::Netz* n = netzVon(in, o.teil, o.stufe);
    if (n == nullptr || o.ecke < 0 || static_cast<std::size_t>(o.ecke) >= n->ecken.size()) { return false; }
    modell::gezeigterOrt(pose, modell::anzeigeVertex(*n, static_cast<std::size_t>(o.ecke)), out);
    return true;
}

// Die gezeigte Normale (der erste Knochen dreht sie, wie die Ansicht und das Spiel)
void normaleGezeigt(const modell::Inhalt& in, const EckeNr& o, const std::vector<BoneMatrix>& pose, float out[3]) {
    const modell::Netz* n = netzVon(in, o.teil, o.stufe);
    const modell::Ecke& e = n->ecken[static_cast<std::size_t>(o.ecke)];
    if (pose.empty()) {
        std::memcpy(out, e.normale, sizeof e.normale);
        return;
    }
    double z[3];
    modell::normaleGezeigt(modell::mischung(pose, modell::anzeigeVertex(*n, static_cast<std::size_t>(o.ecke))), e.normale, z);
    for (int q = 0; q < 3; ++q) { out[q] = static_cast<float>(z[q]); }
}

// Die Auswahl eines Falls: 0 Figur, 1 Kopf, 2 Kopf und Koerper, 3 rechter Arm (Objektmodus), 4 jede dritte Ecke des Kopfs
const char* auswahlName(int art) {
    static const char* n[5] = {"Figur", "Kopf", "Kopf+Koerper", "rechter Arm", "Ecken des Kopfs"};
    return n[std::clamp(art, 0, 4)];
}

void waehle(int art) {
    ModellTab& m = t();
    const int kopf = teilMit("head");
    if (art == 4) {
        modellAuswahlArtSetzen(AuswahlArt::Ecke);
        m.auswahl = {kopf};
        m.eckenWahl.clear();
        if (const modell::Netz* n = netzVon(m.dok.inhalt(), kopf, m.lodJetzt)) {
            for (std::size_t e = 0; e < n->ecken.size(); e += 3U) { m.eckenWahl.push_back(modell::EckOrt{kopf, static_cast<int>(e)}); }
        }
        return;
    }
    modellAuswahlArtSetzen(AuswahlArt::Objekt);
    if (art == 0) {
        m.auswahl = gezeichneteTeile();
    } else if (art == 1) {
        m.auswahl = {kopf};
    } else if (art == 2) {
        m.auswahl = {kopf, teilMit("torso")};
    } else {
        m.auswahl = {teilMit("r_arm")};
    }
}

void merke(const std::shared_ptr<Stand>& st, const std::string& fall) {
    ModellTab& m = t();
    st->vorher = m.dok.inhalt();
    st->stufe = m.lodJetzt;
    st->fall = fall;
    st->unterobjekt = m.auswahlArt != AuswahlArt::Objekt;
    st->zugOk = false;
    st->ecken.clear();
    if (st->unterobjekt) {
        for (const modell::EckOrt& o : m.eckenWahl) { st->ecken.push_back(EckeNr{o.teil, st->stufe, o.ecke}); }
        return;
    }
    for (const int i : m.auswahl) {
        if (i < 0 || static_cast<std::size_t>(i) >= st->vorher.teile.size()) { continue; }
        const modell::Teil& teil = st->vorher.teile[static_cast<std::size_t>(i)];
        for (std::size_t s = 0; s < teil.stufen.size(); ++s) {
            for (std::size_t e = 0; e < teil.stufen[s].ecken.size(); ++e) { st->ecken.push_back(EckeNr{i, static_cast<int>(s), static_cast<int>(e)}); }
        }
    }
}

void drehung(int achse, float grad, float r[3][3]) {
    const float w0 = grad * 3.14159265F / 180.0F;
    const float c = std::cos(w0);
    const float s = std::sin(w0);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { r[i][j] = i == j ? 1.0F : 0.0F; }
    }
    const int u = (achse + 1) % 3;
    const int v = (achse + 2) % 3;
    r[u][u] = c;
    r[u][v] = -s;
    r[v][u] = s;
    r[v][v] = c;
}

// Bildpunkte je Einheit am Ort c (quer zur Blickrichtung) - wie laengeFuerPx der Ansicht: entlang "oben" der Ansicht
float pxJeEinheit(const ModellAnsicht& a, const float c[3], const float oben[3]) {
    const float q[3] = {c[0] + oben[0], c[1] + oben[1], c[2] + oben[2]};
    ImVec2 s0;
    ImVec2 s1;
    if (!modellAufSchirm(a, c, s0) || !modellAufSchirm(a, q, s1)) { return 0.0F; }
    return std::hypot(s1.x - s0.x, s1.y - s0.y);
}

// Die Weltachsen, deren Griff in der aktiven Ansicht zu greifen ist: sein Greifpunkt liegt weiter als eine Schrifthoehe von
// der Mitte (naeher greift die Mitte selbst: gizmoGriff, 0,54 Schrifthoehen). Eine Achse, die aus dem Bild zeigt, faellt weg.
std::vector<int> greifbareAchsen(ImVec2 mitte) {
    std::vector<int> v;
    for (int k = 0; k < 3; ++k) {
        ImVec2 g;
        if (modellGizmoGriffOrt(k + 1, g) && std::hypot(g.x - mitte.x, g.y - mitte.y) > ImGui::GetFontSize()) { v.push_back(k); }
    }
    return v;
}

// Was erwartet wird: art 1 verschieben um `wert` (wegBekannt) oder um einen gemeinsamen Weg (sonst), 2 drehen um `achse`
// mit `wert[0]` Grad, 3 skalieren um `wert`
struct Erwartung {
    int art = 1;
    bool wegBekannt = false;
    float wert[3]{};
    int achse = -1;          // Verschieben am Achsgriff / Drehachse; -2: die Achse des Zugs (Stand::achse)
    int ebene = -1;          // F8: die Achse, die bleibt
    int ansicht = 3;
    bool schirm = false;     // Verschieben per Griff bei stehender Pose: der Weg auf dem Schirm gegen den Mausweg
};

bool pruefeFall(Stand& st, const Erwartung& e0) {
    ModellTab& m = t();
    Erwartung e = e0;
    if (e.achse == -2) { e.achse = st.achse; }
    const std::vector<BoneMatrix>& k = m.umformKnochen;
    const modell::Inhalt& jetzt = m.dok.inhalt();
    std::vector<std::array<float, 3>> p0;
    std::vector<std::array<float, 3>> p1;
    std::vector<const EckeNr*> nr;
    float groesste = 1.0F;
    float mn[3] = {1e30F, 1e30F, 1e30F};
    float mx[3] = {-1e30F, -1e30F, -1e30F};
    for (const EckeNr& o : st.ecken) {
        std::array<float, 3> a{};
        std::array<float, 3> b{};
        if (!gezeigt(st.vorher, o, k, a.data()) || !gezeigt(jetzt, o, k, b.data())) { continue; }
        for (std::size_t q = 0; q < 3U; ++q) {
            groesste = std::max({groesste, std::fabs(a[q]), std::fabs(b[q])});
            if (o.stufe == st.stufe) {   // die Mitte der Auswahl: der Kasten der gezeichneten Stufe (auswahlMitte)
                mn[q] = std::min(mn[q], a[q]);
                mx[q] = std::max(mx[q], a[q]);
            }
        }
        p0.push_back(a);
        p1.push_back(b);
        nr.push_back(&o);
    }
    const float c[3] = {(mn[0] + mx[0]) * 0.5F, (mn[1] + mx[1]) * 0.5F, (mn[2] + mx[2]) * 0.5F};
    const float tol = kRundungen * schritt(groesste);
    float fehler = 0.0F;
    float weg[3] = {0.0F, 0.0F, 0.0F};
    float rot[3][3] = {{1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}};
    std::string zusatz;
    bool ok = !p0.empty() && !k.empty() && (st.zugOk || !e.schirm);
    if (e.art == 1) {
        if (e.wegBekannt) {
            std::memcpy(weg, e.wert, sizeof weg);
        } else {
            double summe[3] = {0.0, 0.0, 0.0};
            for (std::size_t i = 0; i < p0.size(); ++i) {
                for (std::size_t q = 0; q < 3U; ++q) { summe[q] += static_cast<double>(p1[i][q]) - static_cast<double>(p0[i][q]); }
            }
            for (int q = 0; q < 3; ++q) { weg[q] = static_cast<float>(summe[q] / static_cast<double>(p0.size())); }
        }
        for (std::size_t i = 0; i < p0.size(); ++i) {
            for (std::size_t q = 0; q < 3U; ++q) { fehler = std::max(fehler, std::fabs(p1[i][q] - p0[i][q] - weg[q])); }
        }
        // Richtung in der Welt: am Achsgriff nur entlang der Achse, in der Ebene ohne die dritte
        float quer = 0.0F;
        for (int q = 0; q < 3; ++q) {
            if ((e.achse >= 0 && q != e.achse) || (e.ebene >= 0 && q == e.ebene)) { quer = std::max(quer, std::fabs(weg[q])); }
        }
        ok = ok && quer <= tol && std::hypot(weg[0], std::hypot(weg[1], weg[2])) > 1.0F;
        char z[240];
        std::snprintf(z, sizeof z, "Weg %.4f %.4f %.4f, quer %.6f", static_cast<double>(weg[0]), static_cast<double>(weg[1]), static_cast<double>(weg[2]),
                      static_cast<double>(quer));
        zusatz = z;
        if (e.schirm) {
            // Bildschirm. Rechtwinklig: die Mitte wandert um den Mausweg (frei) bzw. um seinen Anteil entlang der Achse.
            // Perspektive: der Griff rechnet Bildpunkte in Einheiten mit dem Massstab an der Mitte um (gizmoZiehen:
            // wert = (maus * s) / |s|^2 * L, s = der Griff der Laenge L auf dem Schirm; frei: (dx * rechts - dy * oben) *
            // Einheit je px). Steht die Maus still, ist die Mitte dabei die neue - dann gilt am Achsgriff genau
            // Weg auf dem Schirm = Mausanteil * w(Griffspitze) / w(alte Mitte) (w: die Tiefe der Projektion; die Achse
            // ist auf dem Schirm eine Gerade, x(t) = a t / (1 + b t)), und frei genau der Weltweg aus dem Massstab.
            const ModellAnsicht& a = m.ansichten[e.ansicht];
            const bool persp = a.art == AnsichtArt::Perspektive;
            const float c1[3] = {c[0] + weg[0], c[1] + weg[1], c[2] + weg[2]};
            float px = 1e9F;
            float kleinsterMassstab = 1e30F;
            ImVec2 s0;
            ImVec2 s1;
            const bool da = modellAufSchirm(a, c, s0) && modellAufSchirm(a, c1, s1);
            float rechts[3] = {0.0F, 0.0F, 0.0F};
            float oben[3] = {0.0F, 0.0F, 0.0F};
            if (persp) {
                const Camera cam = g_app->modelCam;
                cam.right(rechts);
                cam.up(oben);
            }
            if (da && e.achse >= 0) {
                float ea[3] = {c[0], c[1], c[2]};
                ea[e.achse] += 10.0F;
                ImVec2 sa;
                if (modellAufSchirm(a, ea, sa)) {
                    const float ux = sa.x - s0.x;
                    const float uy = sa.y - s0.y;
                    const float l2 = ux * ux + uy * uy;
                    float d = l2 > 0.0F ? (st.maus.x * ux + st.maus.y * uy) / l2 : 0.0F;
                    if (persp) {
                        const float L = 70.0F * m.gizmoGroesse / std::max(pxJeEinheit(a, c1, oben), 1e-6F);
                        float spitze[3] = {c1[0], c1[1], c1[2]};
                        spitze[e.achse] += L;
                        d *= pxJeEinheit(a, c, oben) / std::max(pxJeEinheit(a, spitze, oben), 1e-6F);   // w(Spitze) / w(Mitte)
                        kleinsterMassstab = std::min({pxJeEinheit(a, c, oben), pxJeEinheit(a, c1, oben), pxJeEinheit(a, spitze, oben)});
                    }
                    px = std::hypot(s1.x - s0.x - ux * d, s1.y - s0.y - uy * d);
                }
            } else if (da && !persp) {
                px = std::hypot(s1.x - s0.x - st.maus.x, s1.y - s0.y - st.maus.y);
            } else if (da) {
                // frei in der Perspektive (mit F8: ohne die dritte Achse): der Weltweg, in Bildpunkten am neuen Ort
                const float jePx = 1.0F / std::max(pxJeEinheit(a, c1, oben), 1e-6F);
                kleinsterMassstab = pxJeEinheit(a, c1, oben);
                float ab = 0.0F;
                for (int q = 0; q < 3; ++q) {
                    const float soll = q == e.ebene ? 0.0F : (st.maus.x * rechts[q] - st.maus.y * oben[q]) * jePx;
                    ab = std::max(ab, std::fabs(weg[q] - soll));
                }
                px = ab / jePx;
            }
            const float schirmSchritt = kPxRundungen * schritt(std::max({1.0F, std::fabs(s0.x), std::fabs(s0.y), std::fabs(s1.x), std::fabs(s1.y)}));
            float tolPx = 3.0F * schirmSchritt;
            if (persp) { tolPx += std::hypot(st.maus.x, st.maus.y) * 4.0F * schirmSchritt / std::max(kleinsterMassstab, 1e-6F); }
            (persp ? st.mess.pxPersp : st.mess.px) = std::max(persp ? st.mess.pxPersp : st.mess.px, px);
            st.mess.pxAnteil = std::max(st.mess.pxAnteil, px / tolPx);
            ok = ok && px <= tolPx;
            std::snprintf(z, sizeof z, ", Maus %.0f %.0f, Bild %.4f px (erlaubt %.4f)", static_cast<double>(st.maus.x), static_cast<double>(st.maus.y), static_cast<double>(px),
                          static_cast<double>(tolPx));
            zusatz += z;
        }
    } else {
        if (e.art == 2) {
            drehung(e.achse, e.wert[0], rot);
        } else {
            for (int i = 0; i < 3; ++i) { rot[i][i] = e.wert[i]; }
        }
        for (std::size_t i = 0; i < p0.size(); ++i) {
            const float d[3] = {p0[i][0] - c[0], p0[i][1] - c[1], p0[i][2] - c[2]};
            for (std::size_t q = 0; q < 3U; ++q) {
                const float soll = c[q] + rot[q][0] * d[0] + rot[q][1] * d[1] + rot[q][2] * d[2];
                fehler = std::max(fehler, std::fabs(p1[i][q] - soll));
            }
        }
        char z[160];
        std::snprintf(z, sizeof z, "um die gezeigte Mitte %.3f %.3f %.3f", static_cast<double>(c[0]), static_cast<double>(c[1]), static_cast<double>(c[2]));
        zusatz = z;
        if (e.art == 3) {   // die Normalen bleiben beim gleichmaessigen Skalieren
            for (auto& zeile : rot) {
                for (float& x : zeile) { x = 0.0F; }
            }
            rot[0][0] = rot[1][1] = rot[2][2] = 1.0F;
        }
    }
    ok = ok && fehler <= tol;
    // die gezeigten Normalen: gedreht wie die Umformung (Verschieben und gleichmaessiges Skalieren: unveraendert). Im
    // Unterobjekt-Modus rechnet pfix M6 die Normalen an den verzerrten Dreiecken neu - dort: die der Datei bitgleich oder
    // neu gerechnet mit Laenge 1 in der Datei (gezeigt hat sie die Laenge, die der Knochen ihr gibt: seine Matrix aus
    // 16-Bit-Quaternionen ist nur auf 6e-6 laengentreu).
    float nFehler = 0.0F;
    for (const EckeNr* o : nr) {
        float n0[3];
        float n1[3];
        normaleGezeigt(st.vorher, *o, k, n0);
        normaleGezeigt(jetzt, *o, k, n1);
        if (st.unterobjekt) {
            const float* alt = netzVon(st.vorher, o->teil, o->stufe)->ecken[static_cast<std::size_t>(o->ecke)].normale;
            const float* neu = netzVon(jetzt, o->teil, o->stufe)->ecken[static_cast<std::size_t>(o->ecke)].normale;
            if (std::memcmp(alt, neu, sizeof(float) * 3U) != 0) { nFehler = std::max(nFehler, std::fabs(std::sqrt(neu[0] * neu[0] + neu[1] * neu[1] + neu[2] * neu[2]) - 1.0F)); }
            continue;
        }
        for (int q = 0; q < 3; ++q) { nFehler = std::max(nFehler, std::fabs(n1[q] - (rot[q][0] * n0[0] + rot[q][1] * n0[1] + rot[q][2] * n0[2]))); }
    }
    ok = ok && nFehler <= kTolNormale;
    // was nicht gewaehlt ist, bleibt bitgleich: die Orte aller anderen Ecken, im Objektmodus auch ihre Normalen
    std::size_t andere = 0;
    bool andereGleich = jetzt.teile.size() == st.vorher.teile.size();
    std::vector<std::vector<std::vector<char>>> gewaehlt(st.vorher.teile.size());
    for (const EckeNr& o : st.ecken) {
        auto& teil = gewaehlt[static_cast<std::size_t>(o.teil)];
        if (teil.size() <= static_cast<std::size_t>(o.stufe)) { teil.resize(static_cast<std::size_t>(o.stufe) + 1U); }
        auto& stufe = teil[static_cast<std::size_t>(o.stufe)];
        if (stufe.size() <= static_cast<std::size_t>(o.ecke)) { stufe.resize(static_cast<std::size_t>(o.ecke) + 1U, 0); }
        stufe[static_cast<std::size_t>(o.ecke)] = 1;
    }
    for (std::size_t ti = 0; andereGleich && ti < st.vorher.teile.size(); ++ti) {
        const auto& va = st.vorher.teile[ti].stufen;
        const auto& ja = jetzt.teile[ti].stufen;
        andereGleich = va.size() == ja.size();
        for (std::size_t s = 0; andereGleich && s < va.size(); ++s) {
            andereGleich = va[s].ecken.size() == ja[s].ecken.size();
            for (std::size_t ei = 0; andereGleich && ei < va[s].ecken.size(); ++ei) {
                if (s < gewaehlt[ti].size() && ei < gewaehlt[ti][s].size() && gewaehlt[ti][s][ei] != 0) { continue; }
                ++andere;
                andereGleich = std::memcmp(va[s].ecken[ei].ort, ja[s].ecken[ei].ort, sizeof va[s].ecken[ei].ort) == 0 &&
                               (st.unterobjekt || std::memcmp(va[s].ecken[ei].normale, ja[s].ecken[ei].normale, sizeof va[s].ecken[ei].normale) == 0);
            }
        }
    }
    ok = ok && andereGleich;
    st.mess.orte = std::max(st.mess.orte, fehler / schritt(groesste));
    st.mess.normale = std::max(st.mess.normale, nFehler);
    ++st.mess.faelle;
    char b[320];
    std::snprintf(b, sizeof b, "Knopftest: MBV %s - %zu Ecken, groesste Abweichung %.6f (%.1f Schritte von %.1e, erlaubt %.0f), Normalen %.2e, %zu andere Ecken %s, Pose %s, ",
                  st.fall.c_str(), p0.size(), static_cast<double>(fehler), static_cast<double>(fehler / schritt(groesste)), static_cast<double>(schritt(groesste)),
                  static_cast<double>(kRundungen), static_cast<double>(nFehler), andere, andereGleich ? "bitgleich" : "VERAENDERT", k.empty() ? "keine" : "ja");
    diag::detail(std::string(b) + zusatz);
    return ok;
}

// Orte und Normalen aller Ecken bitgleich
bool gleichWie(const modell::Inhalt& in, const modell::Inhalt& vorher) {
    if (in.teile.size() != vorher.teile.size()) { return false; }
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const auto& a = in.teile[i].stufen;
        const auto& b = vorher.teile[i].stufen;
        if (a.size() != b.size()) { return false; }
        for (std::size_t s = 0; s < a.size(); ++s) {
            if (a[s].ecken.size() != b[s].ecken.size()) { return false; }
            for (std::size_t e = 0; e < a[s].ecken.size(); ++e) {
                if (std::memcmp(a[s].ecken[e].ort, b[s].ecken[e].ort, sizeof b[s].ecken[e].ort) != 0 ||
                    std::memcmp(a[s].ecken[e].normale, b[s].ecken[e].normale, sizeof b[s].ecken[e].normale) != 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

// Rueckgaengig: das Dokument bitgleich wie vor dem Fall; Wiederholen: bitgleich wie danach; und wieder zurueck
bool zurueckUndVor(const Stand& st) {
    modell::Dokument& d = t().dok;
    const modell::Inhalt nachher = d.inhalt();
    const bool zurueck = d.rueckgaengig() && gleichWie(d.inhalt(), st.vorher);
    const bool vor = d.wiederholen() && gleichWie(d.inhalt(), nachher);
    const bool wieder = d.rueckgaengig() && gleichWie(d.inhalt(), st.vorher);
    g_app->modelDirty = true;
    return zurueck && vor && wieder && !gleichWie(nachher, st.vorher);
}

std::string ansichtText(int i) {
    static const char* n[4] = {"Oben", "Vorne", "Links", "Perspektive"};
    return n[std::clamp(i, 0, 3)];
}

// Mit der linken Taste von einer zur Laufzeit gerechneten Stelle um einen zur Laufzeit gerechneten ganzzahligen Weg
// ziehen (ImGui rundet Mausorte auf ganze Bildpunkte: so ist der Mausweg genau der gewollte), 16 Bilder still halten (der
// Griff rechnet in der Perspektive mit dem Massstab an der Mitte des Bilds davor - bei 20 Einheiten Weg in die Tiefe naehert
// sich der Weg je Bild nur um etwa drei Viertel), loslassen.
Schritt ziehe(const std::string& was, const std::shared_ptr<Stand>& st, std::function<bool(ImVec2&, ImVec2&)> plan) {
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    ImVec2 von{0.0F, 0.0F};
                    ImVec2 um{0.0F, 0.0F};
                    st->zugOk = plan(von, um);
                    if (!st->zugOk) {
                        meldeFehler("MBV: kein Griff fuer " + was);
                        return true;
                    }
                    st->start = von;
                    st->maus = ImVec2{std::round(um.x), std::round(um.y)};
                    setzeMaus(von);
                    return false;
                }
                if (b == 2) {
                    io.AddMouseButtonEvent(0, true);
                    return false;
                }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{st->start.x + st->maus.x * f, st->start.y + st->maus.y * f});
                    return false;
                }
                if (b == 31) {
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                return b >= 36;
            }};
}

// Ein Fall mit dem Gizmo: Auswahl, Ansicht, Modus; ziehen am Griff; pruefen; Rueckgaengig und Wiederholen.
// griff: 0..2 der n-te sichtbare Achsgriff der Ansicht, 3 die Mitte, 4 der Ring der Blickachse
void gizmoFall(std::vector<Schritt>& s, const std::shared_ptr<Stand>& st, int auswahl, int ansicht, int gizmo, int griff, int planAchse, float dx, float dy,
               const Erwartung& e0, const std::string& name) {
    const std::string fall = std::string(auswahlName(auswahl)) + ", " + ansichtText(ansicht) + ", " + name;
    s.push_back(tu("MBV vor: " + fall, [=] {
        ModellTab& m = t();
        waehle(auswahl);
        m.aktiv = ansicht;
        m.gizmo = gizmo;
        m.achse = planAchse;
        m.fokus = true;
        merke(st, fall);
    }));
    s.push_back(pause(0.1));
    s.push_back(ziehe("Gizmo: " + fall, st, [=](ImVec2& von, ImVec2& um) {
        const ModellAnsicht& a = t().ansichten[ansicht];
        float c[3];
        ImVec2 sc;
        if (!modellAuswahlMitte(c) || !modellAufSchirm(a, c, sc)) { return false; }
        const std::vector<int> sichtbar = greifbareAchsen(sc);
        st->achse = -1;
        um = ImVec2{dx, dy};
        if (griff <= 2) {   // der Achsgriff: 50 px entlang seiner Richtung auf dem Schirm, dazu 12 px quer (die fallen weg)
            if (static_cast<std::size_t>(griff) >= sichtbar.size()) { return false; }
            st->achse = sichtbar[static_cast<std::size_t>(griff)];
            if (!modellGizmoGriffOrt(st->achse + 1, von)) { return false; }
            const float l = std::hypot(von.x - sc.x, von.y - sc.y);
            const float ux = (von.x - sc.x) / l;
            const float uy = (von.y - sc.y) / l;
            um = ImVec2{50.0F * ux - 12.0F * uy, 50.0F * uy + 12.0F * ux};
            return true;
        }
        if (griff == 4) {   // der Ring um die Achse, die aus dem Bild zeigt
            for (int k = 0; k < 3; ++k) {
                if (std::find(sichtbar.begin(), sichtbar.end(), k) == sichtbar.end()) { st->achse = k; }
            }
            return st->achse >= 0 && modellGizmoRingOrt(st->achse, 45.0F, von);
        }
        von = sc;
        return true;
    }));
    s.push_back(pause(0.1));
    Erwartung e = e0;
    e.ansicht = ansicht;
    s.push_back(pruefe("MBV " + fall + ": in der gezeigten Pose wie gezogen", [=] { return pruefeFall(*st, e); }));
    s.push_back(pruefe("MBV " + fall + ": Rueckgaengig bitgleich wie vorher, Wiederholen bitgleich wie nachher", [=] {
        const bool ok = zurueckUndVor(*st);
        t().achse = -1;
        return ok;
    }));
}

// Ein Fall mit G/R/S und Zahl
void modalFall(std::vector<Schritt>& s, const std::shared_ptr<Stand>& st, int auswahl, int art, int achse, const char* zahl, const std::string& name,
               const std::function<bool()>& dabei = {}) {
    const std::string fall = std::string(auswahlName(auswahl)) + ", " + name;
    s.push_back(tu("MBV vor: " + fall, [=] {
        ModellTab& m = t();
        waehle(auswahl);
        m.aktiv = 3;
        m.fokus = true;
        merke(st, fall);
        if (!modellModalStarten(art)) { meldeFehler("MBV: G/R/S startet nicht: " + fall); }
        m.modal.achse = achse;
        m.modal.zahl = zahl;
    }));
    s.push_back(pause(0.15));
    s.push_back(tu("MBV G/R/S uebernehmen: " + fall, [] { modellModalEnde(true); }));
    Erwartung e;
    e.art = art;
    e.achse = achse;
    const float z = static_cast<float>(std::atof(zahl));
    if (art == 1) {
        e.wegBekannt = true;
        e.wert[achse] = z;
    } else if (art == 2) {
        e.wert[0] = z;
    } else {
        for (int q = 0; q < 3; ++q) { e.wert[q] = achse < 0 || achse == q ? z : 1.0F; }
    }
    s.push_back(pruefe("MBV " + fall + ": in der gezeigten Pose wie eingegeben", [=] { return pruefeFall(*st, e); }));
    if (dabei) { s.push_back(pruefe("MBV " + fall + ": die Datei", dabei)); }
    s.push_back(pruefe("MBV " + fall + ": Rueckgaengig bitgleich wie vorher, Wiederholen bitgleich wie nachher", [=] { return zurueckUndVor(*st); }));
}

// Das Bild der Animation mit den am staerksten gedrehten Knochen (cranium und Oberarme gegen die Bindepose)
int staerkstesBild(const AnimEntry& a) {
    const GlaAnimation& g = g_app->anim;
    const int b[3] = {boneIndex(g, "cranium"), boneIndex(g, "rhumerus"), boneIndex(g, "lhumerus")};
    int best = a.firstFrame;
    double bestW = -1.0;
    for (int f = a.firstFrame; f < a.firstFrame + a.numFrames; ++f) {
        std::vector<BoneMatrix> welt;
        g.worldMatrices(f, welt);
        double summe = 0.0;
        for (const int k : b) {
            if (k < 0 || static_cast<std::size_t>(k) >= welt.size()) { continue; }
            const BoneMatrix s = welt[static_cast<std::size_t>(k)] * g.bones[static_cast<std::size_t>(k)].basePoseInv;
            const double spur = static_cast<double>(s.m[0][0] + s.m[1][1] + s.m[2][2]);
            summe += std::acos(std::clamp((spur - 1.0) * 0.5, -1.0, 1.0));
        }
        if (summe > bestW) {
            bestW = summe;
            best = f;
        }
    }
    return best;
}

// Wie weit die Pose von der Bindepose weg ist: der groesste Drehwinkel (Grad) und der groesste Weg der Skin-Matrizen. Der
// Winkel aus dem schiefen Teil (Sinus) und der Spur (Kosinus): nahe 0 genau - acos der Spur allein macht aus der
// float-Rundung der Matrizen 0,02 bis 0,05 Grad.
std::string poseText(const std::vector<BoneMatrix>& pose) {
    double winkel = 0.0;
    double wegMax = 0.0;
    for (const BoneMatrix& s : pose) {
        const double spur = static_cast<double>(s.m[0][0]) + static_cast<double>(s.m[1][1]) + static_cast<double>(s.m[2][2]);
        const double ax[3] = {static_cast<double>(s.m[2][1]) - static_cast<double>(s.m[1][2]), static_cast<double>(s.m[0][2]) - static_cast<double>(s.m[2][0]),
                              static_cast<double>(s.m[1][0]) - static_cast<double>(s.m[0][1])};
        winkel = std::max(winkel, std::atan2(0.5 * std::sqrt(ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2]), (spur - 1.0) * 0.5) * 180.0 / 3.14159265358979);
        wegMax = std::max({wegMax, std::fabs(static_cast<double>(s.m[0][3])), std::fabs(static_cast<double>(s.m[1][3])), std::fabs(static_cast<double>(s.m[2][3]))});
    }
    char b[120];
    std::snprintf(b, sizeof b, "%zu Knochen, groesste Drehung %.6f Grad, groesster Weg %.6f", pose.size(), winkel, wegMax);
    return b;
}

void figurLaden(std::vector<Schritt>& s) {
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { s.push_back(std::move(x)); }
    s.push_back(tu("MBV: vier Ansichten, keine Animation, die Perspektive schraeg von der Seite (Gier 215, Nick 20: alle drei Achsgriffe greifbar)", [] {
        ModellTab& m = t();
        m.vierAnsichten = true;
        m.bereich = ModellBereich::Ansehen;
        g_app->animPlaying = false;
        // Die Blickrichtung fest, nicht als Schritt von der Startansicht: geschrieben wurde der Test gegen die Startansicht bis
        // rc611 (Gier 180) plus 35/20. Seit MBKOPF beginnt die Perspektive vor dem Gesicht (Gier 90); mit plus 35 zeigen die
        // Griffe X und Y beide nach rechts und liegen am Kopf am Schirm aufeinander - der Zug am Y-Griff griff dort den X-Griff
        // (zusammengefuehrter Stand, Lauf z1_mbv: 2 FEHLER, "quer" so gross wie der Weg).
        g_app->modelOrbit.turn(g_app->modelCam, 215.0F - g_app->modelCam.angles[1], 20.0F - g_app->modelCam.angles[0]);
        diag::detail("Knopftest: MBV - kyle, " + std::to_string(m.dok.inhalt().teile.size()) + " Teile, head " + std::to_string(teilMit("head")) + ", torso " +
                     std::to_string(teilMit("torso")) + ", r_arm " + std::to_string(teilMit("r_arm")) + ", " + std::to_string(m.dok.inhalt().stufenZahl) + " Stufen");
    }));
    s.push_back(pause(0.4));
}

void animationAn(std::vector<Schritt>& s, const std::shared_ptr<Stand>& st) {
    s.push_back(tu("MBV: BOTH_A2_TR_BL auf dem Bild mit den am staerksten gedrehten Knochen", [st] {
        const int i = mb1::animNach("BOTH_A2_TR_BL");
        if (i < 0) {
            meldeFehler("MBV: BOTH_A2_TR_BL fehlt");
            return;
        }
        animWaehlen(i, false);
        g_app->animPlaying = false;
        st->bild = staerkstesBild(g_app->animList[static_cast<std::size_t>(i)]);
        g_app->animFrame = st->bild;
        g_app->animTime = 0.0F;
    }));
    s.push_back(pause(0.4));
    s.push_back(pruefe("MBV: kyle in der Pose (nicht die Bindepose), Kopf, Koerper und rechter Arm gefunden", [st] {
        diag::detail("Knopftest: MBV - BOTH_A2_TR_BL Bild " + std::to_string(st->bild) + ": " + poseText(modellBearbeitungsPose()));
        return !modellBearbeitungsPose().empty() && teilMit("head") >= 0 && teilMit("torso") >= 0 && teilMit("r_arm") >= 0;
    }));
}

// Die .glm traegt die zurueckgerechneten Ecken: schreiben (wie Speichern), in den Laufordner, lesen, vergleichen
bool dateiWieDokument() {
    const modell::Inhalt& in = t().dok.inhalt();
    std::string bytes;
    if (!modell::glmSchreiben(in, bytes)) {
        diag::detail("Knopftest: MBV Datei - glmSchreiben lehnt ab");
        return false;
    }
    const std::string pfad = mb3::lauf("mbv_kopf.glm");
    {
        std::ofstream f(pfad, std::ios::binary);
        f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    const std::string gelesen = slurp(pfad);
    bool ok = gelesen == bytes && !bytes.empty();
    std::size_t ecken = 0;
    for (int stufe = 0; ok && stufe < in.stufenZahl; ++stufe) {
        GlmModel m;
        std::string fehler;
        ok = readGlm(gelesen, m, &fehler, stufe);
        const GlmModel soll = modell::zuGlm(in, stufe);
        ok = ok && m.surfaces.size() == soll.surfaces.size();
        for (std::size_t f = 0; ok && f < m.surfaces.size(); ++f) {
            ok = m.surfaces[f].verts.size() == soll.surfaces[f].verts.size();
            for (std::size_t v = 0; ok && v < m.surfaces[f].verts.size(); ++v) {
                ok = std::memcmp(m.surfaces[f].verts[v].xyz, soll.surfaces[f].verts[v].xyz, sizeof soll.surfaces[f].verts[v].xyz) == 0 &&
                     std::memcmp(m.surfaces[f].verts[v].normal, soll.surfaces[f].verts[v].normal, sizeof soll.surfaces[f].verts[v].normal) == 0;
                ++ecken;
            }
        }
    }
    diag::detail("Knopftest: MBV Datei - " + std::to_string(bytes.size()) + " Bytes, " + std::to_string(ecken) + " Ecken (Ort und Normale) gelesen wie im Dokument: " +
                 (ok ? "ja" : "NEIN"));
    return ok;
}

std::vector<Schritt> umformSchritte() {
    std::vector<Schritt> s;
    auto st = std::make_shared<Stand>();
    figurLaden(s);
    // --- ohne Animation: Bild 0 der .gla ist eine Pose wie jede andere ---
    s.push_back(pruefe("MBV ohne Animation: kyle steht in Bild 0 der .gla - das ist nicht die Bindepose", [] {
        diag::detail("Knopftest: MBV - ohne Animation (Bild " + std::to_string(g_app->animFrame) + "): " + poseText(modellBearbeitungsPose()));
        return g_app->animIndex < 0 && !modellBearbeitungsPose().empty();
    }));
    for (const int aw : {1, 3}) {
        modalFall(s, st, aw, 1, 0, "12", "ohne Animation: G X 12");
        modalFall(s, st, aw, 2, 1, "25", "ohne Animation: R Y 25");
        modalFall(s, st, aw, 3, -1, "1.25", "ohne Animation: S 1.25");
        Erwartung e;
        e.achse = -2;
        e.schirm = true;
        gizmoFall(s, st, aw, 3, 1, 0, -1, 0.0F, 0.0F, e, "ohne Animation: Verschieben am ersten Achsgriff");
    }
    // --- in der Pose, angehalten ---
    animationAn(s, st);
    for (int aw = 0; aw < 3; ++aw) {
        for (int an = 0; an < 4; ++an) {
            for (int g = 0; g < (an == 3 ? 3 : 2); ++g) {
                Erwartung e;
                e.achse = -2;
                e.schirm = true;
                gizmoFall(s, st, aw, an, 1, g, -1, 0.0F, 0.0F, e, "Verschieben am Achsgriff " + std::to_string(g + 1));
            }
            {
                Erwartung e;
                e.schirm = true;
                gizmoFall(s, st, aw, an, 1, 3, -1, 40.0F, 30.0F, e, "Verschieben frei (Mitte)");
            }
            if (an == 3) {
                Erwartung e;
                e.ebene = 2;   // F8 XY: Z bleibt
                e.schirm = true;
                gizmoFall(s, st, aw, an, 1, 3, 3, 40.0F, 30.0F, e, "Verschieben in der Ebene XY (F8)");
            }
            {
                Erwartung e;
                e.art = 2;
                e.achse = an == 3 ? 2 : -2;   // Perspektive: die Mitte dreht um Z (gizmoZiehen)
                e.wert[0] = 40.0F * 0.5F;     // gizmoZiehen: dx * 0,5 Grad
                gizmoFall(s, st, aw, an, 2, an == 3 ? 3 : 4, -1, 40.0F, 0.0F, e, "Drehen");
            }
            {
                Erwartung e;
                e.art = 3;
                e.wert[0] = e.wert[1] = e.wert[2] = 1.0F + 30.0F * 0.01F;   // gizmoZiehen: 1 + dx * 0,01
                gizmoFall(s, st, aw, an, 3, 3, -1, 30.0F, 0.0F, e, "Skalieren");
            }
        }
        modalFall(s, st, aw, 1, 0, "12", "G X 12", aw == 1 ? std::function<bool()>(dateiWieDokument) : std::function<bool()>());
        modalFall(s, st, aw, 1, 2, "-8", "G Z -8");
        modalFall(s, st, aw, 2, 1, "25", "R Y 25");
        modalFall(s, st, aw, 3, -1, "1.25", "S 1.25");
    }
    // Unterobjekt: jede dritte Ecke des Kopfs
    {
        Erwartung e;
        e.achse = -2;
        e.schirm = true;
        gizmoFall(s, st, 4, 3, 1, 0, -1, 0.0F, 0.0F, e, "Verschieben am ersten Achsgriff");
        modalFall(s, st, 4, 1, 1, "5", "G Y 5");
        modalFall(s, st, 4, 2, 2, "30", "R Z 30");
    }
    // --- laufend: dieselben Wege, die Animation spielt (gegen die Pose, in der die Umformung gerechnet wurde) ---
    // ein Zehntel des Tempos: die Griffe liegen beim Druecken noch unter der Maus (zwischen Ort und Druck zwei Bilder)
    s.push_back(tu("MBV: Animation laeuft (Tempo 0,1)", [] {
        g_app->animSpeed = 0.1F;
        g_app->animPlaying = true;
    }));
    s.push_back(pause(0.3));
    for (int aw = 0; aw < 3; ++aw) {
        Erwartung e;
        e.achse = -2;
        gizmoFall(s, st, aw, 3, 1, 0, -1, 0.0F, 0.0F, e, "laufend: Verschieben am ersten Achsgriff");
        Erwartung d;
        d.art = 2;
        d.achse = 2;
        d.wert[0] = 20.0F;
        gizmoFall(s, st, aw, 3, 2, 3, -1, 40.0F, 0.0F, d, "laufend: Drehen");
        modalFall(s, st, aw, 1, 1, "10", "laufend: G Y 10");
        modalFall(s, st, aw, 3, -1, "0.8", "laufend: S 0.8");
    }
    s.push_back(tu("MBV: Animation angehalten", [st] {
        g_app->animPlaying = false;
        g_app->animSpeed = 1.0F;
        g_app->animFrame = st->bild;
        g_app->animTime = 0.0F;
    }));
    s.push_back(pause(0.2));
    s.push_back(pruefe("MBV Messwerte: die groessten Abweichungen aller Faelle", [st] {
        char b[320];
        std::snprintf(b, sizeof b,
                      "Knopftest: MBV Messwerte - %d Faelle, Orte %.1f float-Schritte (erlaubt %.0f), Normalen %.2e (erlaubt %.2e), Bild rechtwinklig %.4f px, Perspektive %.4f px, hoechstens %.0f %% der Schranke des Falls",
                      st->mess.faelle, static_cast<double>(st->mess.orte), static_cast<double>(kRundungen), static_cast<double>(st->mess.normale),
                      static_cast<double>(kTolNormale), static_cast<double>(st->mess.px), static_cast<double>(st->mess.pxPersp), static_cast<double>(st->mess.pxAnteil * 100.0F));
        diag::detail(b);
        return st->mess.faelle > 0;
    }));
    // --- exakt die Bindepose: dasselbe Skelett, ein Bild mit der Einheit als lokaler Matrix jedes Knochens ---
    auto bind = std::make_shared<Stand>();
    s.push_back(tu("MBV Bindepose: das Skelett mit einem Bild in der Grundstellung (lokale Matrizen die Einheit)", [] {
        GlaAnimation r;
        r.name = g_app->anim.name;
        r.skeletonName = g_app->anim.skeletonName;
        r.dateiName = g_app->anim.dateiName;
        r.scale = g_app->anim.scale;
        r.bones = g_app->anim.bones;
        r.numFrames = 1;
        r.roh.assign(r.bones.size(), BoneMatrix::identity());
        g_app->anim = std::move(r);
        g_app->animList.clear();
        g_app->animIndex = -1;
        g_app->animFrame = 0;
        g_app->animTime = 0.0F;
        t().sekundaer = -1;
        g_app->modelDirty = true;
    }));
    s.push_back(pause(0.4));
    s.push_back(pruefe("MBV Bindepose: erkannt (keine Pose zum Umrechnen), obwohl die Skin-Matrizen nur gerundet die Einheit sind", [] {
        std::vector<BoneMatrix> skin;
        gpu::baueKnochenmatrizen(g_app->modellWelt, g_app->anim.bones, skin);
        std::size_t bitgleich = 0;
        const BoneMatrix eins = BoneMatrix::identity();
        for (const BoneMatrix& x : skin) { bitgleich += std::memcmp(&x, &eins, sizeof x) == 0 ? 1U : 0U; }
        diag::detail("Knopftest: MBV Bindepose - " + poseText(skin) + ", " + std::to_string(bitgleich) + " Skin-Matrizen bitgleich die Einheit");
        return !skin.empty() && modellBearbeitungsPose().empty();
    }));
    s.push_back(tu("MBV Bindepose: Kopf, G X 12", [bind] {
        ModellTab& m = t();
        waehle(1);
        m.aktiv = 3;
        m.fokus = true;
        merke(bind, "Bindepose: G X 12");
        if (!modellModalStarten(1)) { meldeFehler("MBV: G startet nicht (Bindepose)"); }
        m.modal.achse = 0;
        m.modal.zahl = "12";
    }));
    s.push_back(pause(0.15));
    s.push_back(tu("MBV Bindepose: G uebernehmen", [] { modellModalEnde(true); }));
    s.push_back(pruefe("MBV Bindepose: der bisherige Rechenweg - jede Ecke des Kopfs in der Datei um genau 12 in X, Y, Z und Normalen bitgleich", [bind] {
        const modell::Inhalt& jetzt = t().dok.inhalt();
        bool ok = t().umformKnochen.empty() && !bind->ecken.empty();
        for (const EckeNr& o : bind->ecken) {
            const modell::Ecke& a = netzVon(bind->vorher, o.teil, o.stufe)->ecken[static_cast<std::size_t>(o.ecke)];
            const modell::Ecke& b = netzVon(jetzt, o.teil, o.stufe)->ecken[static_cast<std::size_t>(o.ecke)];
            ok = ok && b.ort[0] == a.ort[0] + 12.0F && b.ort[1] == a.ort[1] && b.ort[2] == a.ort[2] && std::memcmp(a.normale, b.normale, sizeof a.normale) == 0;
        }
        diag::detail("Knopftest: MBV Bindepose - " + std::to_string(bind->ecken.size()) + " Ecken, Pose der Umformung " + (t().umformKnochen.empty() ? "keine" : "ja"));
        return ok;
    }));
    s.push_back(pruefe("MBV Bindepose: Rueckgaengig bitgleich wie vorher, Wiederholen bitgleich wie nachher", [bind] { return zurueckUndVor(*bind); }));
    return s;
}

// --- Modellieren in der Pose -------------------------------------------------------------------------------------

ModellMesh& mz() { return g_app->mtab.mesh; }

// Die Punkte des Arbeitsnetzes gezeigt (Pose der Ansicht)
std::vector<std::array<float, 3>> netzGezeigt(const w::PolyNetz& n) {
    std::vector<std::array<float, 3>> p(n.punkte.size());
    const std::vector<BoneMatrix>& k = modellBearbeitungsPose();
    for (std::size_t i = 0; i < n.punkte.size(); ++i) {
        std::memcpy(p[i].data(), n.punkte[i].ort, sizeof(float) * 3U);
        if (i >= n.punktEcke.size()) { continue; }
        const auto [teil, ecke] = n.punktEcke[i];
        (void)gezeigt(t().dok.inhalt(), EckeNr{teil, t().lodJetzt, ecke}, k, p[i].data());
    }
    return p;
}

// Die Schranke der Rechengenauigkeit fuer die Punkte eines Netzes (wie pruefeFall)
float netzTol(const std::vector<std::array<float, 3>>& p) {
    float groesste = 1.0F;
    for (const auto& x : p) { groesste = std::max({groesste, std::fabs(x[0]), std::fabs(x[1]), std::fabs(x[2])}); }
    return kRundungen * schritt(groesste);
}

// Wie viele Punkte gezeigt auf der Ebene achse = wert liegen
int aufEbene(int achse, float wert) {
    modellMeshNetzAktualisieren();
    const w::PolyNetz& n = mz().netz;
    const auto p = netzGezeigt(n);
    const float tol = netzTol(p);
    int z = 0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (!n.punkte[i].weg && std::fabs(p[i][static_cast<std::size_t>(achse)] - wert) <= tol) { ++z; }
    }
    return z;
}

GlmVertex alsVertex(const modell::Ecke& e) {
    GlmVertex v;
    std::memcpy(v.xyz, e.ort, sizeof v.xyz);
    std::memcpy(v.bones, e.knochen, sizeof v.bones);
    std::memcpy(v.weights, e.gewicht, sizeof v.weights);
    return v;
}

struct MeshStand {
    modell::Inhalt vorher;
    int vieleck = -1;
    std::vector<std::array<float, 3>> vieleckOrte;   // gezeigt, vor dem Extrude
    float normale[3]{};
    float ebene = 0.0F;
    float vorgabeNeben = 0.0F;   // Slice: die Vorgabe der Ebene gegen die gezeigte Mitte
    int punkteVorher = 0;
};

std::vector<Schritt> modellierSchritte() {
    std::vector<Schritt> s;
    auto st = std::make_shared<Stand>();
    auto ms = std::make_shared<MeshStand>();
    figurLaden(s);
    animationAn(s, st);
    s.push_back(tu("MBV Modellieren: Bereich Modellieren, die Flaeche des Kopfs/Koerpers mit der staerksten Knochenmischung", [ms] {
        ModellTab& m = t();
        m.bereich = ModellBereich::Modellieren;
        modellMeshNetzAktualisieren();
        const w::PolyNetz& n = mz().netz;
        const std::vector<BoneMatrix>& k = modellBearbeitungsPose();
        const int kopf = teilMit("head");
        const int koerper = teilMit("torso");
        double best = -1.0;
        for (std::size_t f = 0; !k.empty() && f < n.flaechen.size(); ++f) {
            const w::PFlaeche& fl = n.flaechen[f];
            if (fl.weg || (fl.teil != kopf && fl.teil != koerper)) { continue; }
            // Mischung: der groesste Unterschied der gemischten Matrizen der Ecken (bei einem starren Vieleck 0)
            double d = 0.0;
            for (std::size_t i = 1; i < fl.ecken.size(); ++i) {
                const modell::Mischung ma = modell::mischung(k, alsVertex(fl.ecken[0].e));
                const modell::Mischung mb = modell::mischung(k, alsVertex(fl.ecken[i].e));
                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) { d = std::max(d, std::fabs(ma.a[r][c] - mb.a[r][c])); }
                }
            }
            if (d > best) {
                best = d;
                ms->vieleck = static_cast<int>(f);
            }
        }
        ms->vieleckOrte.clear();
        const auto p = netzGezeigt(n);
        if (ms->vieleck >= 0) {
            const w::PFlaeche& fl = n.flaechen[static_cast<std::size_t>(ms->vieleck)];
            for (const w::PEcke& c : fl.ecken) { ms->vieleckOrte.push_back(p[static_cast<std::size_t>(c.punkt)]); }
            // die gezeigte Normale nach aussen (gegen Newell: Uhrzeigersinn von aussen, mesh_netz.cpp)
            double nx = 0.0;
            double ny = 0.0;
            double nz = 0.0;
            const std::size_t z = ms->vieleckOrte.size();
            for (std::size_t i = 0; i < z; ++i) {
                const auto& a = ms->vieleckOrte[i];
                const auto& b = ms->vieleckOrte[(i + 1) % z];
                nx += (static_cast<double>(a[1]) - static_cast<double>(b[1])) * (static_cast<double>(a[2]) + static_cast<double>(b[2]));
                ny += (static_cast<double>(a[2]) - static_cast<double>(b[2])) * (static_cast<double>(a[0]) + static_cast<double>(b[0]));
                nz += (static_cast<double>(a[0]) - static_cast<double>(b[0])) * (static_cast<double>(a[1]) + static_cast<double>(b[1]));
            }
            const double l = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (l > 0.0) {
                ms->normale[0] = static_cast<float>(-nx / l);
                ms->normale[1] = static_cast<float>(-ny / l);
                ms->normale[2] = static_cast<float>(-nz / l);
            }
        }
        char b[120];
        std::snprintf(b, sizeof b, "Knopftest: MBV Modellieren - Vieleck %d, Mischung %.3f", ms->vieleck, best);
        diag::detail(b);
        md1d::waehle(w::PAuswahl{{}, {}, {ms->vieleck}}, AuswahlArt::Dreieck);
        ms->vorher = m.dok.inhalt();
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 6.0F;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
    }));
    s.push_back(pause(0.3));
    s.push_back(pruefe("MBV Modellieren: Extrude 6 - die neue Kappe liegt gezeigt um 6 entlang der gezeigten Normale", [ms] {
        modellMeshNetzAktualisieren();
        const w::PolyNetz& n = mz().netz;
        const auto p = netzGezeigt(n);
        const w::PAuswahl& a = mz().auswahl;
        if (a.flaechen.size() != 1U || ms->vieleckOrte.empty()) { return false; }
        const w::PFlaeche& fl = n.flaechen[static_cast<std::size_t>(a.flaechen[0])];
        // jede Ecke der Kappe liegt 6 ueber einer alten Ecke (die Seiten senkrecht)
        float fehler = 0.0F;
        for (const w::PEcke& c : fl.ecken) {
            const auto& q = p[static_cast<std::size_t>(c.punkt)];
            float naechst = 1e30F;
            for (const auto& o : ms->vieleckOrte) {
                float d = 0.0F;
                for (std::size_t k = 0; k < 3U; ++k) { d = std::max(d, std::fabs(q[k] - o[k] - 6.0F * ms->normale[k])); }
                naechst = std::min(naechst, d);
            }
            fehler = std::max(fehler, naechst);
        }
        const float tol = netzTol(p);
        char b[160];
        std::snprintf(b, sizeof b, "Knopftest: MBV Extrude - %zu Ecken, groesste Abweichung %.6f (erlaubt %.6f)", fl.ecken.size(), static_cast<double>(fehler),
                      static_cast<double>(tol));
        diag::detail(b);
        return fehler <= tol && fl.ecken.size() == ms->vieleckOrte.size();
    }));
    s.push_back(pruefe("MBV Modellieren: Extrude Rueckgaengig bitgleich", [ms] {
        const bool ok = t().dok.rueckgaengig() && gleichWie(t().dok.inhalt(), ms->vorher);
        modellMeshNetzAktualisieren();
        return ok;
    }));
    // Slice: Ebene Z durch die gezeigte Mitte des Kopfs
    s.push_back(tu("MBV Modellieren: Slice Z durch die gezeigte Mitte des Kopfs (Polygone des Kopfs gewaehlt)", [ms] {
        modellMeshNetzAktualisieren();
        const w::PolyNetz& n = mz().netz;
        const auto p = netzGezeigt(n);
        const int kopf = teilMit("head");
        w::PAuswahl a;
        float lo = 1e30F;
        float hi = -1e30F;
        for (std::size_t f = 0; f < n.flaechen.size(); ++f) {
            if (n.flaechen[f].weg || n.flaechen[f].teil != kopf) { continue; }
            a.flaechen.push_back(static_cast<int>(f));
            for (const w::PEcke& c : n.flaechen[f].ecken) {
                lo = std::min(lo, p[static_cast<std::size_t>(c.punkt)][2]);
                hi = std::max(hi, p[static_cast<std::size_t>(c.punkt)][2]);
            }
        }
        ms->ebene = std::round((lo + hi) * 0.5F * 4.0F) / 4.0F + 0.125F;
        ms->punkteVorher = aufEbene(2, ms->ebene);
        md1d::waehle(a, AuswahlArt::Dreieck);
        mz().achse = 2;
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Slice);
        ms->vorgabeNeben = mz().schnittOrt - 0.5F * (lo + hi);   // die Vorgabe: die gezeigte Mitte der Auswahl
        mz().schnittOrt = ms->ebene;
        mz().neuPlanen = true;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
    }));
    s.push_back(pause(0.3));
    s.push_back(pruefe("MBV Modellieren: Slice - die Vorgabe ist die gezeigte Mitte, die neuen Punkte liegen gezeigt auf der Ebene", [ms] {
        const int z = aufEbene(2, ms->ebene);
        diag::detail("Knopftest: MBV Slice - Ebene z " + std::to_string(ms->ebene) + ", Punkte darauf vorher " + std::to_string(ms->punkteVorher) + ", nachher " + std::to_string(z) +
                     ", Vorgabe neben der gezeigten Mitte um " + std::to_string(ms->vorgabeNeben) + ", " + mz().letzterBefehl);
        return z >= ms->punkteVorher + 8 && ms->vorgabeNeben == 0.0F;
    }));
    s.push_back(tu("MBV Modellieren: Slice zurueck", [] {
        t().dok.rueckgaengig();
        modellMeshNetzAktualisieren();
    }));
    // QuickSlice: eine senkrechte Linie in der Ansicht von vorn - die Ebene x = const
    s.push_back(tu("MBV Modellieren: QuickSlice in der Ansicht von vorn, senkrecht durch die gezeigte Mitte des Kopfs", [ms] {
        ModellTab& m = t();
        m.aktiv = 1;
        modellMeshNetzAktualisieren();
        const w::PolyNetz& n = mz().netz;
        const auto p = netzGezeigt(n);
        const int kopf = teilMit("head");
        float lo = 1e30F;
        float hi = -1e30F;
        for (std::size_t i = 0; i < p.size(); ++i) {
            if (n.punkte[i].weg || n.punkte[i].teil != kopf) { continue; }
            lo = std::min(lo, p[i][0]);
            hi = std::max(hi, p[i][0]);
        }
        const ModellAnsicht& a = m.ansichten[1];
        const float o[3] = {(lo + hi) * 0.5F, 0.0F, 0.0F};
        const float u[3] = {(lo + hi) * 0.5F, 0.0F, 50.0F};
        ImVec2 s0;
        ImVec2 s1;
        (void)modellAufSchirm(a, o, s0);
        (void)modellAufSchirm(a, u, s1);
        s1.x = s0.x;   // genau senkrecht: die Ebene x = const (rechtwinklige Ansicht)
        ms->ebene = o[0];
        md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        ms->punkteVorher = aufEbene(0, ms->ebene);
        (void)modellMeshQuickSlice(a, s0, s1);
    }));
    s.push_back(pause(0.3));
    s.push_back(pruefe("MBV Modellieren: QuickSlice - die neuen Punkte liegen gezeigt auf der Ebene der Linie", [ms] {
        const int z = aufEbene(0, ms->ebene);
        diag::detail("Knopftest: MBV QuickSlice - Ebene x " + std::to_string(ms->ebene) + ", Punkte darauf vorher " + std::to_string(ms->punkteVorher) + ", nachher " +
                     std::to_string(z) + ", " + mz().letzterBefehl);
        return z >= ms->punkteVorher + 8;
    }));
    s.push_back(tu("MBV Modellieren: QuickSlice zurueck, Bereich Ansehen", [] {
        t().dok.rueckgaengig();
        t().bereich = ModellBereich::Ansehen;
    }));
    // --- ein Modell ohne Skelett nach der Figur: ihr Skelett bleibt geladen (g_app->anim), gilt ihm aber nicht ---
    s.push_back(tu("MBV Requisit: nach der Figur (in der Pose) ein neues Requisit mit einem Kasten", [] {
        std::snprintf(mz().requisitName, sizeof mz().requisitName, "%s", "mbv_kasten");
        (void)modellNeuesRequisit();
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Create);
        modellMeshUebernehmen(false);
    }));
    s.push_back(pause(0.4));
    s.push_back(pruefe("MBV Requisit: keine Pose - die Ansicht zeigt den Kasten an seinen Koordinaten, die Werkzeuge rechnen ohne das Skelett der Figur davor", [] {
        const modell::Inhalt& in = t().dok.inhalt();
        const auto& h = modellHaut();
        if (in.teile.empty() || in.teile[0].stufen.empty() || h.empty() || in.teile[0].stufen[0].ecken.empty() || h[0].size() != in.teile[0].stufen[0].ecken.size()) { return false; }
        const std::vector<modell::Ecke>& ecken = in.teile[0].stufen[0].ecken;
        float neben = 0.0F;
        float mn[3] = {1e30F, 1e30F, 1e30F};
        float mx[3] = {-1e30F, -1e30F, -1e30F};
        for (std::size_t i = 0; i < ecken.size(); ++i) {
            for (std::size_t k = 0; k < 3U; ++k) {
                neben = std::max(neben, std::fabs(h[0][i][k] - ecken[i].ort[k]));
                mn[k] = std::min(mn[k], ecken[i].ort[k]);
                mx[k] = std::max(mx[k], ecken[i].ort[k]);
            }
        }
        char b[260];
        std::snprintf(b, sizeof b, "Knopftest: MBV Requisit - %zu Ecken, Kasten der Datei %.3f %.3f %.3f bis %.3f %.3f %.3f, gezeigt neben den Koordinaten um %.6f, Skelett im Programm %zu Knochen, Pose zum Bearbeiten %s",
                      ecken.size(), static_cast<double>(mn[0]), static_cast<double>(mn[1]), static_cast<double>(mn[2]), static_cast<double>(mx[0]), static_cast<double>(mx[1]),
                      static_cast<double>(mx[2]), static_cast<double>(neben), g_app->anim.bones.size(), modellBearbeitungsPose().empty() ? "keine" : "ja");
        diag::detail(b);
        // der Kasten steht auf dem Boden um die Mitte (Create: Vorgabe), gezeigt genau dort
        return neben == 0.0F && modellBearbeitungsPose().empty() && !g_app->anim.bones.empty() && mn[2] == 0.0F && mn[0] == -mx[0] && mn[1] == -mx[1];
    }));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "mbv") {
        std::vector<Schritt> s = umformSchritte();
        for (Schritt& x : modellierSchritte()) { s.push_back(std::move(x)); }
        return s;
    }
    if (n == "mbvumform") { return umformSchritte(); }
    if (n == "mbvmesh") { return modellierSchritte(); }
    return {};
}

}  // namespace mbv
