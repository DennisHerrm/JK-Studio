// --- mission1: vier Fehler aus shanks Test nach rc606 (BHED_EDITORTEST=mission1) ---
//
// Jeder Fehler wird im laufenden Programm nachgestellt (rot gegen den alten
// Stand), dann behoben und im selben Schritt geprueft:
//   1. Gesichtsanimation: spricht eine Figur (FACE_TALK0..4 nach der
//      Lautstaerke ihres Klangs, cg_players.cpp CG_G2PlayerHeadAnims), bleiben
//      alle Koerper in ihrer Animation - academy1/intro, Luke erklaert.
//   2. Gizmo beim Abspielen: nicht fett und dauerhaft im Bild.
//   3. Mitten im Lauf (Rosh, academy1/intro) verschoben: ein neuer Zwischenpunkt,
//      Anfang und Ende bleiben, ein Rueckgaengig-Schritt.
//   4. Schiff zwischen zwei Punkten gedreht (md_am_sith, Mustafar): ein neuer
//      Schluessel, Ort und Drehung des Anfangs bleiben.
// Daten: das reine JKA (BHED_JKA_BASIS, sonst der Steam-Ordner
// "GameData original/base") und die eingerichteten Spielordner (Movie Duels).
// Die Pruefungen zu 2 lesen die Fotos: BHED_FOTOS muss auf einen Ordner zeigen
// (ohne sind alle Werte -1 und die vier Gizmo-Pruefungen je Durchgang rot).
namespace mission1 {

struct Zustand {
    std::string jka;                    // Ordner des reinen JKA (base)
    std::string mdArchiv;               // assets1.pk3 eines eingerichteten Ordners
    int luke = -1;                      // Figur, die spricht
    double redeMs = -1.0;               // Zeit, zu der sie spricht (FACE_TALK)
    double stillMs = -1.0;              // Zeit davor, zu der niemand spricht
    std::string text;
};

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Die erste Figur, deren Name `teil` enthaelt (klein), -1 sonst.
int figurNamens(const std::string& teil) {
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        if (klein(g_app->scene.actors[i].name).find(teil) != std::string::npos) { return static_cast<int>(i); }
    }
    return -1;
}

// Die Knochen einer Figur, genau wie sie gezeichnet wird (figurBilderSetzen +
// figurKnochen, dieselbe Rechnung wie der GPU-Weg). `mitMund` false: ohne den
// Mundlauf (faceFrame -1) - der Vergleich zeigt, was der Mund veraendert.
bool figurWelt(std::size_t ai, double ms, bool mitMund, std::vector<BoneMatrix>& welt, ActorDraw& d) {
    if (ai >= g_app->actorAssets.size() || g_app->actorAssets.size() != g_app->scene.actors.size() ||
        !g_app->actorAssets[ai]) {
        return false;
    }
    const Actor& a = g_app->scene.actors[ai];
    const App::ActorAssets& assets = *g_app->actorAssets[ai];
    if (assets.model.empty()) { return false; }
    const ActorState st = a.at(ms);
    d = ActorDraw{};
    d.figur = static_cast<int>(ai);
    d.model = &assets.model;
    figurBilderSetzen(a, assets, st, ms, d);
    if (d.anim == nullptr) { return false; }
    figurKopfGier(st, d);
    ActorDraw r = d;
    if (!mitMund) { r.faceFrame = -1; }
    std::vector<BoneMatrix> hilf;
    return figurKnochen(r, welt, hilf) && !welt.empty();
}

// Die Knochen unter "face" (der Teilbaum, auf dem der Mundlauf liegt).
std::vector<char> gesichtTeil(const GlaAnimation& anim) {
    std::vector<char> t(anim.bones.size(), 0);
    const int f = boneIndex(anim, "face");
    if (f < 0) { return t; }
    t[static_cast<std::size_t>(f)] = 1;
    for (const int b : anim.topologicalOrder()) {
        const int p = anim.bones[static_cast<std::size_t>(b)].parent;
        if (p >= 0 && static_cast<std::size_t>(p) < t.size() && t[static_cast<std::size_t>(p)] != 0) {
            t[static_cast<std::size_t>(b)] = 1;
        }
    }
    return t;
}

float matrixAbstand(const BoneMatrix& a, const BoneMatrix& b) {
    float m = 0.0F;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) { m = std::max(m, std::fabs(a.m[r][c] - b.m[r][c])); }
    }
    return m;
}

struct KoerperBefund {
    int figuren = 0;          // Figuren mit Skelett
    int sprechen = 0;         // davon mit Mundlauf (faceFrame >= 0)
    int grundstellung = 0;    // Koerper in der Grundstellung (Root-Pose)
    float koerperMax = 0.0F;  // groesste Aenderung eines Koerperknochens durch den Mund
    float gesichtMax = 0.0F;  // groesste Aenderung eines Gesichtsknochens durch den Mund
    std::string text;
};

// Alle Figuren zur Zeit ms: was aendert der Mundlauf am Koerper, und steht ein
// Koerper in der Grundstellung (alle Knochen = basePose)?
KoerperBefund koerperBefund(double ms) {
    KoerperBefund k;
    for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
        std::vector<BoneMatrix> mit;
        std::vector<BoneMatrix> ohne;
        ActorDraw d;
        if (!figurWelt(ai, ms, true, mit, d) || !g_app->scene.actors[ai].at(ms).visible) { continue; }
        ActorDraw d2;
        if (!figurWelt(ai, ms, false, ohne, d2) || ohne.size() != mit.size()) { continue; }
        ++k.figuren;
        const std::vector<char> gesicht = gesichtTeil(*d.anim);
        float koerper = 0.0F;
        float gesichtD = 0.0F;
        float zurGrund = 0.0F;
        for (std::size_t b = 0; b < mit.size() && b < d.anim->bones.size(); ++b) {
            const float x = matrixAbstand(mit[b], ohne[b]);
            if (b < gesicht.size() && gesicht[b] != 0) {
                gesichtD = std::max(gesichtD, x);
            } else {
                koerper = std::max(koerper, x);
                zurGrund = std::max(zurGrund, matrixAbstand(mit[b], d.anim->bones[b].basePose));
            }
        }
        if (d.faceFrame >= 0) { ++k.sprechen; }
        if (zurGrund < 0.01F) { ++k.grundstellung; }
        k.koerperMax = std::max(k.koerperMax, koerper);
        if (d.faceFrame >= 0) { k.gesichtMax = std::max(k.gesichtMax, gesichtD); }
        if (d.faceFrame >= 0 || zurGrund < 0.01F || koerper > 0.01F) {
            char z[260];
            std::snprintf(z, sizeof(z), " [%s: Bild %d, Mund %d, Koerper durch Mund %.3f, Gesicht %.3f, zur Grundstellung %.3f]",
                          g_app->scene.actors[ai].name.c_str(), d.frame, d.faceFrame, static_cast<double>(koerper),
                          static_cast<double>(gesichtD), static_cast<double>(zurGrund));
            k.text += z;
        }
    }
    return k;
}

// Ein Bild der Fotos (BHED_FOTOS) lesen: 32 Bit BGRA, von oben (fotoSchreiben).
struct FotoBild {
    int w = 0;
    int h = 0;
    std::vector<unsigned char> px;
    bool lesen(const std::string& name) {
        const char* ordner = std::getenv("BHED_FOTOS");
        if (ordner == nullptr) { return false; }
        std::ifstream f(std::string(ordner) + "\\" + name + ".bmp", std::ios::binary);
        std::string d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (d.size() < 54 || d[0] != 'B' || d[1] != 'M') { return false; }
        std::int32_t ww = 0;
        std::int32_t hh = 0;
        std::memcpy(&ww, d.data() + 18, 4);
        std::memcpy(&hh, d.data() + 22, 4);
        w = ww;
        h = hh < 0 ? -hh : hh;
        if (w <= 0 || h <= 0 || d.size() < 54U + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U) { return false; }
        px.assign(d.begin() + 54, d.begin() + 54 + static_cast<std::ptrdiff_t>(w) * h * 4);
        return true;
    }
    // r, g, b an (x, y)
    std::array<int, 3> bei(int x, int y) const {
        if (x < 0 || y < 0 || x >= w || y >= h) { return {0, 0, 0}; }
        const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
        return {px[at + 2], px[at + 1], px[at + 0]};
    }
};

// Bildpunkte in den Farben der Gizmo-Achsen (rot 235/70/70, gruen 80/210/90,
// blau 70/130/255, hervorgehoben 255/245/140) in einem Rechteck.
int gizmoPunkte(const FotoBild& b, const ImRect& r) {
    int n = 0;
    for (int y = static_cast<int>(r.Min.y); y < static_cast<int>(r.Max.y); ++y) {
        for (int x = static_cast<int>(r.Min.x); x < static_cast<int>(r.Max.x); ++x) {
            const std::array<int, 3> p = b.bei(x, y);
            const auto nah = [&p](int r0, int g0, int b0) {
                return std::abs(p[0] - r0) + std::abs(p[1] - g0) + std::abs(p[2] - b0) < 40;
            };
            if (nah(235, 70, 70) || nah(80, 210, 90) || nah(70, 130, 255) || nah(255, 245, 140)) { ++n; }
        }
    }
    return n;
}

// Das Fadenkreuz in der Bildmitte (app_view3d_bedienung.inc: 255/210/60, Arme
// von gap bis arm links/rechts/oben/unten): Anteil der Armpunkte in seiner Farbe.
float fadenkreuzAnteil(const FotoBild& b) {
    const float mx = (g_app->kartenAnsicht[0] + g_app->kartenAnsicht[2]) * 0.5F;
    const float my = (g_app->kartenAnsicht[1] + g_app->kartenAnsicht[3]) * 0.5F;
    const float arm = ImGui::GetFontSize() * 0.55F;
    const float gap = arm * 0.35F;
    int n = 0;
    int treffer = 0;
    for (float d = gap + 1.0F; d < arm - 1.0F; d += 1.0F) {
        const int px[4][2] = {{static_cast<int>(mx - d), static_cast<int>(my)}, {static_cast<int>(mx + d), static_cast<int>(my)},
                              {static_cast<int>(mx), static_cast<int>(my - d)}, {static_cast<int>(mx), static_cast<int>(my + d)}};
        for (const auto& q : px) {
            // eine Zeile/Spalte Spielraum (Rundung der Bildmitte)
            bool da = false;
            for (int o = -1; o <= 1 && !da; ++o) {
                const bool waag = q[1] == static_cast<int>(my);
                const std::array<int, 3> c = b.bei(q[0] + (waag ? 0 : o), q[1] + (waag ? o : 0));
                da = std::abs(c[0] - 255) + std::abs(c[1] - 210) + std::abs(c[2] - 60) < 90;
            }
            ++n;
            treffer += da ? 1 : 0;
        }
    }
    return n > 0 ? static_cast<float>(treffer) / static_cast<float>(n) : 0.0F;
}

// Bildpunkte in den Farben der Bearbeitungshilfen: Auswahl/Umriss orange
// (255/165/40), Griffe gelb (240/200/60), Kameraschluessel blau (96/170/255).
int hilfsPunkte(const FotoBild& b, const ImRect& r) {
    int n = 0;
    for (int y = static_cast<int>(r.Min.y); y < static_cast<int>(r.Max.y); ++y) {
        for (int x = static_cast<int>(r.Min.x); x < static_cast<int>(r.Max.x); ++x) {
            const std::array<int, 3> p = b.bei(x, y);
            const auto nah = [&p](int r0, int g0, int b0) {
                return std::abs(p[0] - r0) + std::abs(p[1] - g0) + std::abs(p[2] - b0) < 30;
            };
            if (nah(255, 165, 40) || nah(240, 200, 60) || nah(96, 170, 255)) { ++n; }
        }
    }
    return n;
}

// Vor dem Laden einer anderen Mission: geaenderte Entities aus den Teilen
// davor vergessen - sonst fragt das Laden ("The entities of ... have unsaved
// changes") und haelt an. Die Skripte bleiben geaendert (die Frage beim Beenden).
void ungespeicherteEntitiesVergessen() {
    g_app->entitiesUngespeichert.clear();
    g_app->frageOffen = false;
}

int reiterMit(const std::string& teil) {
    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
        const bool aktiv = static_cast<int>(i) == g_app->activeTab;
        const std::string& n = aktiv ? g_app->shownName : g_app->tabs[i].shownName;
        const std::string& sp = aktiv ? g_app->skriptPfad : g_app->tabs[i].skriptPfad;
        if (klein(n).find(teil) != std::string::npos || klein(sp).find(teil) != std::string::npos) { return static_cast<int>(i); }
    }
    return -1;
}

}  // namespace mission1

namespace mission1 {

// Alle Befehle `name` (bei set: mit SET-Typ `typ`) im Skript.
std::vector<const Node*> befehle(const char* name, const char* typ) {
    std::vector<const Node*> out;
    std::function<void(const std::vector<Node>&)> geh = [&](const std::vector<Node>& ns) {
        for (const Node& x : ns) {
            if (x.kind == Node::Kind::Command && x.name == name && (typ == nullptr || (!x.args.empty() && x.args[0].text == typ))) {
                out.push_back(&x);
            }
            geh(x.children);
        }
    };
    geh(g_app->doc.script().nodes);
    return out;
}

// Alle Entities mit allen Schluesseln als Text (fuer den exakten Vergleich).
std::string entsText() {
    std::string t;
    for (const MapEntity& e : g_app->map.entities) {
        t += "{";
        for (const auto& [k, v] : e.keys) { t += k + "=" + v + ";"; }
        t += "}";
    }
    return t;
}

int entityNamens(const std::string& name) {
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].targetname == name) { return static_cast<int>(i); }
    }
    return -1;
}

float abstand3(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
float winkelAbstand3(const float* a, const float* b) {
    float m = 0.0F;
    for (int k = 0; k < 3; ++k) { m = std::max(m, std::fabs(std::remainder(a[k] - b[k], 360.0F))); }
    return m;
}
std::string v3(const float* v) {
    char t[80];
    std::snprintf(t, sizeof(t), "%.1f %.1f %.1f", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
    return t;
}

// Mit der linken Taste von `ort` um (dx, dy) ziehen; `probe` laeuft vor dem
// Loslassen, bei ruhender Maus (die Vorschau des Zuges).
Schritt ziehenMitProbe(const std::string& was, std::function<ImVec2()> ort, float dx, float dy, std::function<void()> probe) {
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) { *z = ort(); setzeMaus(*z); return false; }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{z->x + dx * f, z->y + dy * f});
                    return false;
                }
                if (b == 17) { probe(); return false; }
                if (b == 18) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 24;
            }};
}

}  // namespace mission1

// --- 3. Mitten im Lauf verschoben: ein Zwischenpunkt (academy1/intro, Rosh) ---
// teil: Teil des Figurennamens; inselProbe: den Hinweis "kein Weg" mit einem
// unverbundenen Navgoal pruefen (braucht ein Wegnetz); k: Kennung der Fotos.
std::vector<Schritt> mission1ZwischenSchritte(const std::string& teil, bool inselProbe, const std::string& k) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct LaufProbe {
        int figur = -1;
        int entity = -1;
        Path pfad;
        double t = -1.0;
        double ab = 0.0;
        double bis = 0.0;
        std::size_t navgoals = 0;
        std::string endName;
        float endOrt[3]{};
        float spawn[3]{};
        float endPos[3]{};
        std::size_t undo = 0;
        std::size_t entUndo = 0;
        std::size_t ents = 0;
        int griffe = 0;               // Laufziel-Griffe (gelb) der Figur vorher
        std::string skriptVor;
        std::string entsVor;
        std::string skriptNach;
        std::string entsNach;
    };
    auto lp = std::make_shared<LaufProbe>();
    const auto laufGriffe = [] {
        int n = 0;
        for (int i = 0; i < bahnGriffZahl(); ++i) { n += bahnGriffArt(i) == 1 ? 1 : 0; }
        return n;
    };
    add(tu("Zwischenpunkt: die Figur mitten in ihrem Lauf suchen", [=] {
        *lp = LaufProbe{};
        for (std::size_t ai = 0; ai < g_app->scene.actors.size() && lp->figur < 0; ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            if (mission1::klein(a.name).find(teil) == std::string::npos) { continue; }
            for (const ActorStep& st : a.steps) {
                if (st.kind != ActorStep::Kind::WalkTo || st.path.empty() || !st.fremd.empty()) { continue; }
                double ab = st.startMs;
                double bis = st.endMs;
                for (const ActorStep& w : a.steps) {
                    if (w.kind == ActorStep::Kind::WalkTo && w.path == st.path) {
                        ab = std::min(ab, w.startMs);
                        bis = std::max(bis, w.endMs);
                    }
                }
                if (bis - ab < 1500.0) { continue; }
                lp->figur = static_cast<int>(ai);
                lp->entity = entityFuerFigur(static_cast<int>(ai));
                lp->pfad = st.path;
                lp->ab = ab;
                lp->bis = bis;
                lp->t = std::floor((ab + bis) * 0.5 / 50.0) * 50.0;
                break;
            }
        }
        if (lp->figur < 0 || lp->entity < 0) { return; }
        const Node* n = nodeAt(g_app->doc.script(), lp->pfad);
        lp->endName = n != nullptr && n->args.size() >= 2 ? n->args[1].text : std::string();
        const int ee = mission1::entityNamens(lp->endName);
        if (ee >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(ee)], lp->endOrt); }
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(lp->entity)], lp->spawn);
        const ActorState e = g_app->scene.actors[static_cast<std::size_t>(lp->figur)].at(lp->bis + 1000.0);
        for (int k = 0; k < 3; ++k) { lp->endPos[k] = e.pos[k]; }
        lp->navgoals = mission1::befehle("set", "SET_NAVGOAL").size();
        lp->undo = g_app->doc.undoDepth();
        lp->entUndo = g_app->entUndo.size();
        lp->ents = g_app->map.entities.size();
        lp->skriptVor = writeScript(g_app->doc.script());
        lp->entsVor = mission1::entsText();
        diag::detail("Knopftest: Zwischenpunkt - " + g_app->scene.actors[static_cast<std::size_t>(lp->figur)].name + " laeuft " +
                     std::to_string(lp->ab) + " bis " + std::to_string(lp->bis) + " ms zu \"" + lp->endName + "\" (" +
                     mission1::v3(lp->endOrt) + "), angefasst bei " + std::to_string(lp->t) + " ms, Ende der Figur " +
                     mission1::v3(lp->endPos) + ", " + std::to_string(lp->navgoals) + " SET_NAVGOAL");
    }));
    add(pruefe("Zwischenpunkt: eine laufende Figur mit Laufziel gefunden", [=] {
        return lp->figur >= 0 && lp->entity >= 0 && !lp->endName.empty() && mission1::entityNamens(lp->endName) >= 0;
    }));
    add(tu("Zwischenpunkt: Kamera vor die Figur, ihr Spawner gewaehlt, Verschieben", [=] {
        FolgeZiel f;
        f.actor = lp->figur;
        f.entity = lp->entity;
        f.ms = lp->t;
        folgeKameraVor(f, 170.0F, 120.0F);
        entityAuswahlSetzen(lp->entity);
        g_app->gizmoMode = App::GizmoMode::Move;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(tu("Zwischenpunkt: Laufziel-Griffe vorher zaehlen", [=] { lp->griffe = laufGriffe(); }));
    add(foto("m1_zwischen_vorher" + k));
    add(ziehenAn("Zwischenpunkt: an der X-Achse des Gizmos ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(1, q);
        return q;
    }, 60.0F, 0.0F, 0));
    add(pause(0.6));
    add(foto("m1_zwischen_nachher" + k));
    add(pruefe("Zwischenpunkt: ein neues Laufziel dazwischen, Anfang (Spawner) und Ende (altes Laufziel) bleiben", [=] {
        prepareScene();
        const std::vector<const Node*> ng = mission1::befehle("set", "SET_NAVGOAL");
        int endeImSkript = 0;
        std::string namen;
        for (const Node* n : ng) {
            if (n->args.size() >= 2) {
                namen += " " + n->args[1].text;
                endeImSkript += n->args[1].text == lp->endName ? 1 : 0;
            }
        }
        const int ee = mission1::entityNamens(lp->endName);
        float endOrt[3]{};
        if (ee >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(ee)], endOrt); }
        float spawn[3]{};
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(lp->entity)], spawn);
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(lp->figur)];
        double bis = 0.0;
        for (const ActorStep& st : a.steps) {
            if (st.kind == ActorStep::Kind::WalkTo) { bis = std::max(bis, st.endMs); }
        }
        const ActorState e = a.at(bis + 1000.0);
        const float endAbw = mission1::abstand3(e.pos, lp->endPos);
        diag::detail("Knopftest: Zwischenpunkt - SET_NAVGOAL " + std::to_string(lp->navgoals) + " -> " + std::to_string(ng.size()) +
                     " (" + namen + " ), \"" + lp->endName + "\" " + std::to_string(endeImSkript) + "x im Skript, Entity " +
                     (ee >= 0 ? mission1::v3(endOrt) : std::string("FEHLT")) + ", Spawner " + mission1::v3(spawn) + " (vorher " +
                     mission1::v3(lp->spawn) + "), Figur am Ende " + mission1::v3(e.pos) + " (vorher " + mission1::v3(lp->endPos) +
                     ", Abstand " + std::to_string(endAbw) + "), Rueckgaengig Skript " + std::to_string(lp->undo) + " -> " +
                     std::to_string(g_app->doc.undoDepth()) + ", Entities " + std::to_string(lp->entUndo) + " -> " +
                     std::to_string(g_app->entUndo.size()));
        // Der Block der Figur und ihre Laeufe danach (zum Nachlesen).
        if (!lp->pfad.empty() && lp->pfad.front() < g_app->doc.script().nodes.size()) {
            Script blk;
            blk.nodes.push_back(g_app->doc.script().nodes[lp->pfad.front()]);
            std::string text = writeScript(blk);
            for (char& c : text) { c = (c == '\n' || c == '\r') ? '|' : c; }
            diag::detail("Knopftest: Zwischenpunkt - Block danach: " + text);
        }
        std::string laeufe;
        for (const ActorStep& st : a.steps) {
            if (st.kind != ActorStep::Kind::WalkTo) { continue; }
            char z2[160];
            std::snprintf(z2, sizeof(z2), " [%.0f-%.0f %s -> %s]", st.startMs, st.endMs, mission1::v3(st.from).c_str(),
                          mission1::v3(st.to).c_str());
            laeufe += z2;
        }
        diag::detail("Knopftest: Zwischenpunkt - Laeufe danach:" + laeufe);
        // Das Ende bleibt: im Block der Figur folgt auf das neue Laufziel das
        // alte, sein Griff steht weiter auf der Bahn, und die Figur laeuft vom
        // neuen Punkt aus weiter zu ihm (keine Meldung "findet keinen Weg").
        int neuVorAlt = 0;
        if (!lp->pfad.empty() && lp->pfad.front() < g_app->doc.script().nodes.size()) {
            int stelle = 0;
            int neuAn = -1;
            int altAn = -1;
            std::function<void(const std::vector<Node>&)> geh = [&](const std::vector<Node>& ns) {
                for (const Node& x : ns) {
                    if (x.kind == Node::Kind::Command && x.name == "set" && x.args.size() >= 2 && x.args[0].text == "SET_NAVGOAL") {
                        if (x.args[1].text == lp->endName) { altAn = stelle; }
                        else if (neuAn < 0) { neuAn = stelle; }
                    }
                    ++stelle;
                    geh(x.children);
                }
            };
            geh(g_app->doc.script().nodes[lp->pfad.front()].children);
            neuVorAlt = (neuAn >= 0 && altAn > neuAn) ? 1 : 0;
        }
        // Weiter heisst: nach der Ankunft am neuen Punkt ein Lauf, der von dort
        // wegfuehrt (mehr als 32 Einheiten).
        bool weiter = false;
        for (const ActorStep& st : a.steps) {
            weiter = weiter || (st.kind == ActorStep::Kind::WalkTo && st.startMs >= lp->t - 1000.0 && st.endMs - st.startMs > 200.0 &&
                                mission1::abstand3(st.from, st.to) > 32.0F);
        }
        const int griffe = laufGriffe();
        lp->skriptNach = writeScript(g_app->doc.script());
        lp->entsNach = mission1::entsText();
        diag::detail("Knopftest: Zwischenpunkt - neues Laufziel vor dem alten " + std::to_string(neuVorAlt) + ", Laufziel-Griffe " +
                     std::to_string(lp->griffe) + " -> " + std::to_string(griffe) + "; Vorschau: Lauf vom neuen Punkt zum alten Ziel " +
                     std::to_string(weiter) + ", Meldungen \"findet keinen Weg\" " + std::to_string(a.stehtOhneWeg.size()));
        return ng.size() == lp->navgoals + 1 && endeImSkript == 1 && ee >= 0 && mission1::abstand3(endOrt, lp->endOrt) < 0.01F &&
               mission1::abstand3(spawn, lp->spawn) < 0.01F && neuVorAlt == 1 && griffe == lp->griffe + 1 && weiter &&
               a.stehtOhneWeg.empty();
    }));
    // Der Hinweis "kein Weg": den neuen Punkt zur Probe zu einer Insel des
    // Wegnetzes machen (waypoint_navgoal mit NO_AUTO_CONNECT, wie die Kopie
    // bis mission1) - dann findet die Figur von dort keinen Weg.
    auto insel = std::make_shared<std::vector<MapEntity>>();
    if (inselProbe) {
    add(tu("Zwischenpunkt: Probe - der neue Punkt als unverbundener waypoint_navgoal", [=] {
        *insel = g_app->map.entities;
        for (MapEntity& e : g_app->map.entities) {
            if (e.classname == "ref_tag" && e.targetname != lp->endName && mission1::befehle("set", "SET_NAVGOAL").size() > lp->navgoals) {
                bool genutzt = false;
                for (const Node* n : mission1::befehle("set", "SET_NAVGOAL")) { genutzt = genutzt || (n->args.size() >= 2 && n->args[1].text == e.targetname); }
                if (!genutzt) { continue; }
                setKey(e, "classname", "waypoint_navgoal");
                e.classname = "waypoint_navgoal";
                setKey(e, "spawnflags", "4");
            }
        }
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.8));
    add(foto("m1_zwischen_keinweg" + k));
    add(pruefe("Zwischenpunkt: ohne Weg zeigt die Bahn den Hinweis \"kein Weg\" zum alten Ziel (statt es verschwinden zu lassen)", [=] {
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(lp->figur)];
        std::string t;
        for (const std::string& h : g_app->bahnKeinWeg) { t += " [" + h + "]"; }
        diag::detail("Knopftest: Zwischenpunkt - Probe Insel: Meldungen \"findet keinen Weg\" " + std::to_string(a.stehtOhneWeg.size()) +
                     ", Hinweise an der Bahn" + (t.empty() ? std::string(" keine") : t));
        bool zumEnde = false;
        for (const std::string& h : g_app->bahnKeinWeg) { zumEnde = zumEnde || h.find(lp->endName) != std::string::npos; }
        return !a.stehtOhneWeg.empty() && zumEnde;
    }));
    add(tu("Zwischenpunkt: Probe zurueck", [=] {
        g_app->map.entities = *insel;
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    }
    add(tu("Zwischenpunkt: Maus ueber die Karte", [] { setzeMaus(ansichtMitte()); }));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.5));
    add(pruefe("Zwischenpunkt: EIN Strg+Z stellt Skript und Entities exakt wie vorher her", [=] {
        const std::size_t ng = mission1::befehle("set", "SET_NAVGOAL").size();
        const bool skript = writeScript(g_app->doc.script()) == lp->skriptVor;
        const bool ents = mission1::entsText() == lp->entsVor;
        diag::detail("Knopftest: Zwischenpunkt - nach einem Strg+Z: SET_NAVGOAL " + std::to_string(ng) + " (vorher " +
                     std::to_string(lp->navgoals) + "), Entities " + std::to_string(g_app->map.entities.size()) + " (vorher " +
                     std::to_string(lp->ents) + "), Skript gleich " + std::to_string(skript) + ", Entities gleich " + std::to_string(ents));
        return skript && ents;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.5));
    add(pruefe("Zwischenpunkt: Strg+Y stellt den Zwischenpunkt exakt wieder her", [=] {
        const bool skript = writeScript(g_app->doc.script()) == lp->skriptNach;
        const bool ents = mission1::entsText() == lp->entsNach;
        diag::detail("Knopftest: Zwischenpunkt - nach Strg+Y: Skript gleich " + std::to_string(skript) + ", Entities gleich " +
                     std::to_string(ents));
        return skript && ents;
    }));
    add(tu("Zwischenpunkt: aufraeumen (alles zurueck)", [=] {
        while (g_app->entUndo.size() > lp->entUndo && entityRueckgaengig()) {}
        while (g_app->doc.undoDepth() > lp->undo && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.4));
    return s;
}

// --- 2. Gizmo beim Abspielen ---------------------------------------------------
// figurTeil: Teil des Figurennamens; k: Kennung der Fotos.
std::vector<Schritt> mission1GizmoSchritte(const std::string& figurTeil, const std::string& k) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct GizmoProbe {
        int entity = -1;
        int figur = -1;
        int vorher = -1;
        int spielt1 = -1;
        int spielt2 = -1;
        int nachher = -1;
    };
    auto gp = std::make_shared<GizmoProbe>();
    add(tu("Gizmo: eine Figur waehlen (Verschieben), Zeit 0, freie Ansicht", [=] {
        gp->figur = mission1::figurNamens(figurTeil);
        gp->entity = gp->figur >= 0 ? entityFuerFigur(gp->figur) : -1;
        FolgeZiel f;
        f.actor = gp->figur;
        f.entity = gp->entity;
        f.ms = 0.0;
        folgeKameraVor(f, 260.0F, 140.0F);
        g_app->playing = false;
        g_app->throughCamera = false;
        entityAuswahlSetzen(gp->entity);
        g_app->gizmoMode = App::GizmoMode::Move;
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(foto("m1_gizmo_vorher" + k));
    add(klick(tr(Str::TlPlay), "###main"));
    add(pause(0.8));
    add(foto("m1_gizmo_spielt1" + k));
    add(pause(0.8));
    add(foto("m1_gizmo_spielt2" + k));
    add(klick(tr(Str::TlStop), "###main"));
    add(pause(0.6));
    add(foto("m1_gizmo_nachher" + k));
    add(pruefe("Abspielen wie im Spiel (Figur gewaehlt): kein Fadenkreuz, keine Auswahl, Griffe, Marken - nach Stop wieder da", [=] {
        const ImRect r{ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}};
        const std::string namen[4] = {"m1_gizmo_vorher" + k, "m1_gizmo_spielt1" + k, "m1_gizmo_spielt2" + k, "m1_gizmo_nachher" + k};
        float kreuz[4]{};
        int hilfe[4]{};
        for (int i = 0; i < 4; ++i) {
            mission1::FotoBild b;
            if (b.lesen(namen[i])) {
                kreuz[i] = mission1::fadenkreuzAnteil(b);
                hilfe[i] = mission1::hilfsPunkte(b, r);
            } else {
                kreuz[i] = -1.0F;
                hilfe[i] = -1;
            }
        }
        char t[300];
        std::snprintf(t, sizeof(t),
                      "Knopftest: Abspielen (Figur) - Fadenkreuz vorher %.2f, beim Abspielen %.2f / %.2f, nach Stop %.2f; Hilfsfarben vorher %d, "
                      "beim Abspielen %d / %d, nach Stop %d",
                      static_cast<double>(kreuz[0]), static_cast<double>(kreuz[1]), static_cast<double>(kreuz[2]),
                      static_cast<double>(kreuz[3]), hilfe[0], hilfe[1], hilfe[2], hilfe[3]);
        diag::detail(t);
        return kreuz[0] > 0.25F && kreuz[3] > 0.25F && kreuz[1] >= 0.0F && kreuz[1] < 0.1F && kreuz[2] < 0.1F && hilfe[0] > 30 &&
               hilfe[1] >= 0 && hilfe[1] <= 25 && hilfe[2] <= 25 && hilfe[3] > 30;
    }));
    add(pruefe("Gizmo: beim Abspielen nicht fett und dauerhaft im Bild, nach Stop wie vorher", [=] {
        const ImRect r{ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}};
        int* ziel[4] = {&gp->vorher, &gp->spielt1, &gp->spielt2, &gp->nachher};
        const std::string namen[4] = {"m1_gizmo_vorher" + k, "m1_gizmo_spielt1" + k, "m1_gizmo_spielt2" + k, "m1_gizmo_nachher" + k};
        for (int i = 0; i < 4; ++i) {
            mission1::FotoBild b;
            *ziel[i] = b.lesen(namen[i]) ? mission1::gizmoPunkte(b, r) : -1;
        }
        diag::detail("Knopftest: Gizmo - Achsenfarben im Bild: vorher " + std::to_string(gp->vorher) + ", beim Abspielen " +
                     std::to_string(gp->spielt1) + " / " + std::to_string(gp->spielt2) + ", nach Stop " + std::to_string(gp->nachher));
        return gp->vorher > 50 && gp->spielt1 >= 0 && gp->spielt1 * 2 < gp->vorher && gp->spielt2 * 2 < gp->vorher &&
               std::abs(gp->nachher - gp->vorher) * 4 < gp->vorher;
    }));
    // Dasselbe mit der gewaehlten Kamera (Schluessel mit Gizmo).
    auto kp = std::make_shared<GizmoProbe>();
    add(tu("Gizmo: Kameraschluessel 2 waehlen (Verschieben), freie Ansicht davor", [=] {
        g_app->playing = false;
        entityAuswahlSetzen(-1);
        kameraSchluesselWaehlen(1, true);
        // genug Zeitleiste fuer zwei Fotos beim Abspielen
        g_app->playMs = std::min(g_app->playMs, std::max(0.0, g_app->timeline.durationMs - 3000.0));
        g_app->gizmoMode = App::GizmoMode::Move;
        g_app->throughCamera = false;
        const CameraState st = camStateAt(g_app->playMs);
        const float ziel[3] = {st.pos[0], st.pos[1], st.pos[2]};
        g_app->cam.pos[0] = st.pos[0] - 220.0F;
        g_app->cam.pos[1] = st.pos[1] - 70.0F;
        g_app->cam.pos[2] = st.pos[2] + 90.0F;
        aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
        g_app->mapDirty = true;
        diag::detail("Knopftest: Gizmo - Kamera gewaehlt " + std::to_string(g_app->cameraSelected) + ", Schluessel " +
                     std::to_string(g_app->selectedKey) + ", Zeit " + std::to_string(g_app->playMs));
    }));
    add(pause(0.6));
    add(foto("m1_kgizmo_vorher" + k));
    add(klick(tr(Str::TlPlay), "###main"));
    add(pause(0.8));
    add(foto("m1_kgizmo_spielt1" + k));
    add(pause(0.8));
    add(foto("m1_kgizmo_spielt2" + k));
    add(klick(tr(Str::TlStop), "###main"));
    add(pause(0.6));
    add(foto("m1_kgizmo_nachher" + k));
    add(pruefe("Abspielen wie im Spiel (Kamera gewaehlt): kein Fadenkreuz, keine Auswahl, Griffe, Marken - nach Stop wieder da", [=] {
        const ImRect r{ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}};
        const std::string namen[4] = {"m1_kgizmo_vorher" + k, "m1_kgizmo_spielt1" + k, "m1_kgizmo_spielt2" + k, "m1_kgizmo_nachher" + k};
        float kreuz[4]{};
        int hilfe[4]{};
        for (int i = 0; i < 4; ++i) {
            mission1::FotoBild b;
            if (b.lesen(namen[i])) {
                kreuz[i] = mission1::fadenkreuzAnteil(b);
                hilfe[i] = mission1::hilfsPunkte(b, r);
            } else {
                kreuz[i] = -1.0F;
                hilfe[i] = -1;
            }
        }
        char t[300];
        std::snprintf(t, sizeof(t),
                      "Knopftest: Abspielen (Kamera) - Fadenkreuz vorher %.2f, beim Abspielen %.2f / %.2f, nach Stop %.2f; Hilfsfarben vorher %d, "
                      "beim Abspielen %d / %d, nach Stop %d",
                      static_cast<double>(kreuz[0]), static_cast<double>(kreuz[1]), static_cast<double>(kreuz[2]),
                      static_cast<double>(kreuz[3]), hilfe[0], hilfe[1], hilfe[2], hilfe[3]);
        diag::detail(t);
        return kreuz[0] > 0.25F && kreuz[3] > 0.25F && kreuz[1] >= 0.0F && kreuz[1] < 0.1F && kreuz[2] < 0.1F && hilfe[0] > 30 &&
               hilfe[1] >= 0 && hilfe[1] <= 25 && hilfe[2] <= 25 && hilfe[3] > 30;
    }));
    add(pruefe("Gizmo: Kamera - beim Abspielen kein stehengebliebener Gizmo, nach Stop wie vorher", [=] {
        const ImRect r{ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}};
        int* ziel[4] = {&kp->vorher, &kp->spielt1, &kp->spielt2, &kp->nachher};
        const std::string namen[4] = {"m1_kgizmo_vorher" + k, "m1_kgizmo_spielt1" + k, "m1_kgizmo_spielt2" + k, "m1_kgizmo_nachher" + k};
        for (int i = 0; i < 4; ++i) {
            mission1::FotoBild b;
            *ziel[i] = b.lesen(namen[i]) ? mission1::gizmoPunkte(b, r) : -1;
        }
        diag::detail("Knopftest: Gizmo - Kamera, Achsenfarben im Bild: vorher " + std::to_string(kp->vorher) + ", beim Abspielen " +
                     std::to_string(kp->spielt1) + " / " + std::to_string(kp->spielt2) + ", nach Stop " + std::to_string(kp->nachher) +
                     ", gizmoShown " + std::to_string(g_app->gizmoShown));
        return kp->vorher > 50 && kp->spielt1 >= 0 && kp->spielt1 * 2 < kp->vorher && kp->spielt2 * 2 < kp->vorher &&
               std::abs(kp->nachher - kp->vorher) * 4 < kp->vorher;
    }));
    add(tu("Gizmo: Kamera abwaehlen", [] {
        g_app->cameraSelected = false;
        g_app->selectedKey = -1;
        g_app->mapDirty = true;
    }));
    return s;
}

// --- 4. Schiff zwischen zwei Punkten gedreht (md_am_sith) ------------------------
std::vector<Schritt> mission1DrehSchritte(std::shared_ptr<mission1::Zustand> z) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct SchiffProbe {
        int figur = -1;
        int entity = -1;
        double t = -1.0;
        double t0 = 0.0;
        double t1 = 0.0;
        int moves = 0;
        std::size_t undo = 0;
        float ang0[3]{};
        float pos0[3]{};
        float angE[3]{};
        float posE[3]{};
        float angT[3]{};
        float vorschauStart = -1.0F;   // Abweichung am Anfang waehrend des Zuges (Grad)
        float vorschauOrt = -1.0F;
    };
    auto sp = std::make_shared<SchiffProbe>();
    const auto zuege = [](int figur) {
        std::string t;
        if (figur < 0) { return t; }
        for (const ActorStep& st : g_app->scene.actors[static_cast<std::size_t>(figur)].steps) {
            if (st.kind != ActorStep::Kind::Glide && st.kind != ActorStep::Kind::Turn && st.kind != ActorStep::Kind::Face &&
                st.kind != ActorStep::Kind::Teleport) {
                continue;
            }
            const char* art = st.kind == ActorStep::Kind::Glide ? "move" : st.kind == ActorStep::Kind::Turn ? "rotate"
                              : st.kind == ActorStep::Kind::Face ? "face" : "teleport";
            char z2[300];
            std::snprintf(z2, sizeof(z2), " [%s %.0f-%.0f nach %s Winkel %s%s]", art, st.startMs, st.endMs, mission1::v3(st.to).c_str(),
                          mission1::v3(st.toAng).c_str(), st.mitWinkel ? "" : " (ohne)");
            t += z2;
        }
        return t;
    };
    add(tu("Drehen: Load mission mit assets1.pk3 des eingerichteten Spielordners", [=] { missionenAusArchiv(z->mdArchiv); }));
    add(warteBis("Drehen: md_am_sith in der Auswahl", [] { return missionsliste::hat("md_am_sith", true); }, 3500));
    add(waehleInListe("md_am_sith", false, 0, "##missionpick"));
    add(warteBis("Drehen: md_am_sith geladen", [] {
        return g_app->map.path == "maps/md_am_sith.bsp" && g_app->camTrackValid && g_app->tabs.size() > 1;
    }, 3000));
    add(pause(1.0));
    add(tu("Drehen: das Schiff (Brush-Modell) mitten in einer move-Fahrt suchen", [=] {
        *sp = SchiffProbe{};
        for (std::size_t ai = 0; ai < g_app->scene.actors.size() && sp->figur < 0; ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            if (a.brushModel <= 0) { continue; }
            for (const ActorStep& st : a.steps) {
                if (st.kind != ActorStep::Kind::Glide || st.path.empty() || !st.fremd.empty() || st.endMs - st.startMs < 1500.0) { continue; }
                const Node* n = nodeAt(g_app->doc.script(), st.path);
                if (n == nullptr || n->name != "move") { continue; }
                sp->figur = static_cast<int>(ai);
                sp->t0 = st.startMs;
                sp->t1 = st.endMs;
                sp->t = std::floor((st.startMs + st.endMs) * 0.5 / 50.0) * 50.0;
                break;
            }
            if (sp->figur < 0) { continue; }
            for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                const std::string* mdl = g_app->map.entities[i].find("model");
                if (mdl != nullptr && *mdl == "*" + std::to_string(a.brushModel)) { sp->entity = static_cast<int>(i); }
            }
        }
        if (sp->figur < 0 || sp->entity < 0) { return; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(sp->figur)];
        const ActorState s0 = a.at(sp->t0);
        const ActorState sE = a.at(sp->t1 + 1.0);
        const ActorState sT = a.at(sp->t);
        for (int k = 0; k < 3; ++k) {
            sp->ang0[k] = s0.angles[k];
            sp->pos0[k] = s0.pos[k];
            sp->angE[k] = sE.angles[k];
            sp->posE[k] = sE.pos[k];
            sp->angT[k] = sT.angles[k];
        }
        sp->moves = static_cast<int>(mission1::befehle("move", nullptr).size());
        sp->undo = g_app->doc.undoDepth();
        diag::detail("Knopftest: Drehen - Reiter " + g_app->shownName + ", Schiff " + a.name + " (Entity " +
                     std::to_string(sp->entity) + "), Fahrt " + std::to_string(sp->t0) + "-" + std::to_string(sp->t1) +
                     " ms, angefasst bei " + std::to_string(sp->t) + "; Anfang " + mission1::v3(sp->pos0) + " / " +
                     mission1::v3(sp->ang0) + ", Ende " + mission1::v3(sp->posE) + " / " + mission1::v3(sp->angE) + "; Schritte" +
                     zuege(sp->figur));
    }));
    add(pruefe("Drehen: ein Schiff mitten in einer move-Fahrt gefunden", [=] { return sp->figur >= 0 && sp->entity >= 0; }));
    add(tu("Drehen: Kamera vor das Schiff, gewaehlt, Drehmodus", [=] {
        FolgeZiel f;
        f.actor = sp->figur;
        f.entity = sp->entity;
        f.ms = sp->t;
        folgeKameraVor(f, 420.0F, 200.0F);
        entityAuswahlSetzen(sp->entity);
        g_app->gizmoMode = App::GizmoMode::Rotate;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("m1_drehen_vorher"));
    add(mission1::ziehenMitProbe("Drehen: am Gier-Ring des fliegenden Schiffs ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 50.0F, -25.0F, [=] {
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(sp->figur)];
        const ActorState s0 = a.at(sp->t0);
        sp->vorschauStart = mission1::winkelAbstand3(s0.angles, sp->ang0);
        sp->vorschauOrt = mission1::abstand3(s0.pos, sp->pos0);
        diag::detail("Knopftest: Drehen - waehrend des Zuges: Anfang " + mission1::v3(s0.pos) + " / " + mission1::v3(s0.angles) +
                     ", Schritte" + zuege(sp->figur));
    }));
    add(pause(0.6));
    add(foto("m1_drehen_nachher"));
    add(pruefe("Drehen: ein neuer Schluessel zur Zeit, Ort und Drehung von Anfang und Ende bleiben (auch waehrend des Zuges)", [=] {
        prepareScene();
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(sp->figur)];
        const ActorState s0 = a.at(sp->t0);
        const ActorState sE = a.at(sp->t1 + 1.0);
        const ActorState sT = a.at(sp->t + 1.0);
        const float dAnfangW = mission1::winkelAbstand3(s0.angles, sp->ang0);
        const float dAnfangO = mission1::abstand3(s0.pos, sp->pos0);
        const float dEndeW = mission1::winkelAbstand3(sE.angles, sp->angE);
        const float dEndeO = mission1::abstand3(sE.pos, sp->posE);
        const float dT = std::fabs(std::remainder(sT.angles[1] - sp->angT[1], 360.0F));
        const int moves = static_cast<int>(mission1::befehle("move", nullptr).size());
        char t[400];
        std::snprintf(t, sizeof(t),
                      "Knopftest: Drehen - moves %d -> %d; Anfang: Ort %.2f, Winkel %.2f (Zug: %.2f / %.2f); Ende: Ort %.2f, "
                      "Winkel %.2f; Gier zur Zeit %.1f -> %.1f; Schritte",
                      sp->moves, moves, static_cast<double>(dAnfangO), static_cast<double>(dAnfangW),
                      static_cast<double>(sp->vorschauOrt), static_cast<double>(sp->vorschauStart), static_cast<double>(dEndeO),
                      static_cast<double>(dEndeW), static_cast<double>(sp->angT[1]), static_cast<double>(sT.angles[1]));
        diag::detail(std::string(t) + zuege(sp->figur));
        return moves == sp->moves + 1 && dAnfangO < 0.05F && dAnfangW < 0.05F && sp->vorschauStart >= 0.0F && sp->vorschauStart < 0.05F &&
               sp->vorschauOrt < 0.05F && dEndeO < 0.05F && dEndeW < 0.05F && dT > 2.0F;
    }));
    add(tu("Drehen: zuruecknehmen", [=] {
        while (g_app->doc.undoDepth() > sp->undo && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->gizmoMode = App::GizmoMode::Move;
        g_app->mapDirty = true;
    }));
    add(pause(0.3));
    // Zweiter Fall: NACH der letzten Fahrt (bzw. in einer Pause) gedreht. Bis
    // mission1 drehte das die Entity selbst - also den Anfang.
    struct NachProbe {
        double t = -1.0;
        double ende = 0.0;     // Ende der letzten Fahrt
        int moves = 0;
        std::size_t undo = 0;
        std::size_t entUndo = 0;
        float ang0[3]{};
        float pos0[3]{};
        float angE[3]{};
        float posE[3]{};
        float entWinkel[3]{};
    };
    auto np = std::make_shared<NachProbe>();
    add(tu("Drehen nach der letzten Fahrt: Schiff gewaehlt, Zeit 1 s nach dem Ende, Drehmodus", [=] {
        *np = NachProbe{};
        if (sp->figur < 0) { return; }
        prepareScene();
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(sp->figur)];
        for (const ActorStep& st : a.steps) {
            if (st.kind == ActorStep::Kind::Glide && !st.path.empty() && st.fremd.empty()) { np->ende = std::max(np->ende, st.endMs); }
        }
        np->t = std::floor((np->ende + 1000.0) / 50.0) * 50.0;
        const ActorState s0 = a.at(0.0);
        const ActorState sE = a.at(np->ende);
        for (int k = 0; k < 3; ++k) {
            np->ang0[k] = s0.angles[k];
            np->pos0[k] = s0.pos[k];
            np->angE[k] = sE.angles[k];
            np->posE[k] = sE.pos[k];
        }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(sp->entity)];
        if (const std::string* w = e.find("angles")) {
            (void)std::sscanf(w->c_str(), "%f %f %f", &np->entWinkel[0], &np->entWinkel[1], &np->entWinkel[2]);
        } else if (const std::string* w1 = e.find("angle")) {
            np->entWinkel[1] = std::strtof(w1->c_str(), nullptr);
        }
        np->moves = static_cast<int>(mission1::befehle("move", nullptr).size());
        np->undo = g_app->doc.undoDepth();
        np->entUndo = g_app->entUndo.size();
        FolgeZiel f;
        f.actor = sp->figur;
        f.entity = sp->entity;
        f.ms = np->t;
        folgeKameraVor(f, 420.0F, 200.0F);
        entityAuswahlSetzen(sp->entity);
        g_app->gizmoMode = App::GizmoMode::Rotate;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("Drehen nach der letzten Fahrt: am Gier-Ring ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 50.0F, -25.0F, 0));
    add(pause(0.6));
    add(pruefe("Drehen nach der letzten Fahrt: ein neuer Schluessel, Anfang (Entity, Stand bei 0 s) und letzter Schluessel bleiben", [=] {
        prepareScene();
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(sp->figur)];
        const ActorState s0 = a.at(0.0);
        const ActorState sE = a.at(np->ende);   // genau am Ende der letzten Fahrt (danach beginnt der neue Schluessel)
        const ActorState sT = a.at(np->t + 1.0);
        float w[3]{};
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(sp->entity)];
        if (const std::string* ws = e.find("angles")) {
            (void)std::sscanf(ws->c_str(), "%f %f %f", &w[0], &w[1], &w[2]);
        } else if (const std::string* w1 = e.find("angle")) {
            w[1] = std::strtof(w1->c_str(), nullptr);
        }
        const float dEnt = mission1::winkelAbstand3(w, np->entWinkel);
        const float dA = std::max(mission1::winkelAbstand3(s0.angles, np->ang0), mission1::abstand3(s0.pos, np->pos0));
        const float dE = std::max(mission1::winkelAbstand3(sE.angles, np->angE), mission1::abstand3(sE.pos, np->posE));
        const float dT = mission1::winkelAbstand3(sT.angles, np->angE);
        const int moves = static_cast<int>(mission1::befehle("move", nullptr).size());
        char t[400];
        std::snprintf(t, sizeof(t),
                      "Knopftest: Drehen nach der letzten Fahrt (%.0f ms) - moves %d -> %d; Entity-Winkel %s -> %s; Anfang %.2f, "
                      "letzter Schluessel %.2f, zur Zeit gedreht %.1f Grad",
                      np->t, np->moves, moves, mission1::v3(np->entWinkel).c_str(), mission1::v3(w).c_str(), static_cast<double>(dA),
                      static_cast<double>(dE), static_cast<double>(dT));
        diag::detail(t);
        return np->t > 0.0 && moves == np->moves + 1 && dEnt < 0.01F && dA < 0.05F && dE < 0.05F && dT > 2.0F;
    }));
    add(tu("Drehen nach der letzten Fahrt: zuruecknehmen", [=] {
        while (g_app->entUndo.size() > np->entUndo && entityRueckgaengig()) {}
        while (g_app->doc.undoDepth() > np->undo && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->gizmoMode = App::GizmoMode::Move;
        g_app->mapDirty = true;
    }));
    add(pause(0.3));
    return s;
}

// --- Rueckgaengig/Wiederholen nach jedem Gizmo-Zug (Wunsch von shank) -------------
//
// Nach jedem Bewegen und Drehen mit dem Gizmo - Figur, Schiff, Kamera; vor dem
// ersten, zwischen zwei und nach dem letzten Punkt - stellt EIN Strg+Z alles
// exakt wie vorher her: Skripttext bytegleich, alle Entities, Anzahl, Zeiten,
// Orte und Winkel der Schluessel, die Vorschau am Anfang und zur Zeit. Strg+Y
// stellt die Aenderung exakt wieder her. Dazu Ketten aus drei Zuegen, die
// alle zurueck- und wieder vorgenommen werden.
namespace mission1 {

enum class ZielArt : std::uint8_t { Figur, Schiff, Kamera };

struct UndoZiel {
    ZielArt art = ZielArt::Figur;
    int figur = -1;     // Figur/Schiff: Nummer in scene.actors
    int entity = -1;    // Figur: Spawner, Schiff: die Brush-Entity
    int schluessel = 0; // Kamera: gewaehlter Schluessel
    double ms = 0.0;    // eingestellte Zeit
    std::string name;   // fuer den Bericht
    int wann = -1;      // 0 vor dem ersten, 1 zwischen zwei, 2 nach dem letzten Punkt
};

std::string zahl3(double v) {
    char t[32];
    std::snprintf(t, sizeof(t), "%.3f", v);
    return t;
}

struct UndoStand {
    std::string skript;
    std::string ents;
    std::string werte;   // Schluessel bzw. Wegpunkte des Ziels, lesbar
    std::string bild;    // Vorschau: Ort und Winkel am Anfang und zur Zeit
    bool gleich(const UndoStand& o) const { return skript == o.skript && ents == o.ents && werte == o.werte && bild == o.bild; }
};

UndoStand undoStand(const UndoZiel& z) {
    prepareScene();
    UndoStand s;
    s.skript = writeScript(g_app->doc.script());
    s.ents = entsText();
    if (z.art == ZielArt::Kamera) {
        const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
        s.werte = std::to_string(ks.size()) + " Schluessel:";
        for (const KameraSchluessel& k : ks) { s.werte += " [" + v3(k.pos) + " / " + (k.hasAng ? v3(k.ang) : std::string("-")) + "]"; }
        s.werte += " Fahrten:";
        for (const CamSegment& c : g_app->camTrack.segments) {
            if (c.kind == CamSegment::Kind::Move && c.fremd.empty()) { s.werte += " " + zahl3(c.startMs) + "-" + zahl3(c.endMs); }
        }
        const CameraState a = camStateAt(0.0);
        const CameraState b = camStateAt(z.ms);
        s.bild = v3(a.pos) + " / " + v3(a.angles) + " | " + v3(b.pos) + " / " + v3(b.angles);
        return s;
    }
    if (z.figur < 0 || static_cast<std::size_t>(z.figur) >= g_app->scene.actors.size()) { return s; }
    const Actor& a = g_app->scene.actors[static_cast<std::size_t>(z.figur)];
    int n = 0;
    std::string t;
    for (const ActorStep& st : a.steps) {
        if (st.kind == ActorStep::Kind::WalkTo && !st.teilweg) {
            t += " [lauf " + zahl3(st.startMs) + "-" + zahl3(st.endMs) + " -> " + v3(st.laufZiel) + "]";
            ++n;
        } else if (st.kind == ActorStep::Kind::Glide) {
            t += " [move " + zahl3(st.startMs) + "-" + zahl3(st.endMs) + " -> " + v3(st.to) + " / " + v3(st.toAng) + "]";
            ++n;
        } else if (st.kind == ActorStep::Kind::Wunschgier) {
            t += " [dreh " + zahl3(st.startMs) + " " + zahl3(st.value) + "]";
            ++n;
        }
    }
    s.werte = std::to_string(n) + " Punkte, Start " + v3(a.start) + " / " + v3(a.startAngles) + ":" + t;
    const ActorState a0 = a.at(0.0);
    const ActorState at = a.at(z.ms);
    s.bild = v3(a0.pos) + " / " + v3(a0.angles) + " | " + v3(at.pos) + " / " + v3(at.angles);
    return s;
}

// Ziel waehlen, Zeit setzen, freie Kamera davor.
void undoZielWaehlen(const UndoZiel& z, bool drehen) {
    g_app->playing = false;
    g_app->throughCamera = false;
    g_app->gizmoMode = drehen ? App::GizmoMode::Rotate : App::GizmoMode::Move;
    if (z.art == ZielArt::Kamera) {
        entityAuswahlSetzen(-1);
        kameraSchluesselWaehlen(z.schluessel, true);
        g_app->playMs = z.ms;
        g_app->gizmoZeitBezug = z.ms;
        const CameraState st = camStateAt(z.ms);
        const float ziel[3] = {st.pos[0], st.pos[1], st.pos[2]};
        g_app->cam.pos[0] = st.pos[0] - 220.0F;
        g_app->cam.pos[1] = st.pos[1] - 70.0F;
        g_app->cam.pos[2] = st.pos[2] + 90.0F;
        aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
    } else {
        g_app->cameraSelected = false;
        g_app->selectedKey = -1;
        FolgeZiel f;
        f.actor = z.figur;
        f.entity = z.entity;
        f.ms = z.ms;
        folgeKameraVor(f, z.art == ZielArt::Schiff ? 420.0F : 200.0F, z.art == ZielArt::Schiff ? 200.0F : 110.0F);
        entityAuswahlSetzen(z.entity);
    }
    g_app->mapDirty = true;
}

ImVec2 undoGriff(const UndoZiel& z, bool drehen) {
    ImVec2 q{-1.0F, -1.0F};
    if (z.art == ZielArt::Kamera) {
        if (!drehen) {
            const float x = g_app->gizmoSx[2][0] + (g_app->gizmoSx[2][1] - g_app->gizmoSx[2][0]) * 0.66F;
            const float y = g_app->gizmoSy[2][0] + (g_app->gizmoSy[2][1] - g_app->gizmoSy[2][0]) * 0.66F;
            return bildZuSchirm(x, y);
        }
        for (int t = 0; t < App::kRingPts; ++t) {
            if (g_app->gizmoRingSx[2][t] >= 0.0F) {
                return bildZuSchirm(g_app->gizmoRingSx[2][t], g_app->gizmoRingSy[2][t]);
            }
        }
        return q;
    }
    (void)entityGizmoPunkt(drehen ? 4 : 1, q);
    return q;
}

// Der Zustand aller Figuren, Objekte und der Kamera zu einer Zeit: Ort, Winkel,
// Animation, Sichtbarkeit; bei Objekten und Kamera dazu das Ende ihrer letzten
// Fahrt (die Zeit des Endpunkts).
struct Eintrag {
    std::string name;
    float pos[3]{};
    float ang[3]{};
    std::string anim;
    bool sichtbar = true;
    double endeMs = -1.0;
    bool laeuft = false;   // eine Figur mit Laufziel (Ankunft im Radius, siehe unten)
};
std::vector<Eintrag> szenenZustand(double ms) {
    prepareScene();
    std::vector<Eintrag> v;
    for (const Actor& a : g_app->scene.actors) {
        const ActorState st = a.at(ms);
        Eintrag e;
        e.name = a.name;
        for (int k = 0; k < 3; ++k) {
            e.pos[k] = st.pos[k];
            e.ang[k] = st.angles[k];
        }
        e.anim = st.animation;
        e.sichtbar = st.visible;
        for (const ActorStep& x : a.steps) {
            if (x.kind == ActorStep::Kind::Glide) { e.endeMs = std::max(e.endeMs, x.endMs); }
            e.laeuft = e.laeuft || x.kind == ActorStep::Kind::WalkTo;
        }
        v.push_back(e);
    }
    Eintrag k;
    k.name = "Kamera";
    const CameraState cs = camStateAt(ms);
    for (int q = 0; q < 3; ++q) {
        k.pos[q] = cs.pos[q];
        k.ang[q] = cs.angles[q];
    }
    k.sichtbar = cs.enabled;
    for (const CamSegment& c : g_app->camTrack.segments) {
        if (c.kind == CamSegment::Kind::Move && c.fremd.empty()) { k.endeMs = std::max(k.endeMs, c.endMs); }
    }
    v.push_back(k);
    return v;
}
// Wer weicht ab? Bytegleich verglichen; nur eine laufende Figur darf am Ende
// innerhalb des Ankunftsradius abweichen (NPC_ReachedGoal: sie haelt, sobald sie
// im Radius ist - ein anderer Weg kommt anders an), wenn `radiusFuer` sie nennt.
// Die Zeit des Endpunkts (endeMs) zaehlt nur fuer `ziel` selbst: haengt ein
// Ablauf an der Ankunft einer Figur (dowait), verschiebt ein anderer Weg alles
// danach - das ist das Spiel, kein Fehler des Werkzeugs.
std::vector<std::string> abweichungen(const std::vector<Eintrag>& vor, const std::vector<Eintrag>& nach, const std::string& radiusFuer,
                                      const std::string& ziel, float& radiusAbstand, std::string& t) {
    std::vector<std::string> wer;
    for (const Eintrag& a : vor) {
        const Eintrag* b = nullptr;
        for (const Eintrag& x : nach) {
            if (x.name == a.name) { b = &x; }
        }
        if (b == nullptr) {
            t += " " + a.name + "(fehlt)";
            wer.push_back(a.name);
            continue;
        }
        bool gleich = a.anim == b->anim && a.sichtbar == b->sichtbar && (a.name != ziel || a.endeMs == b->endeMs);
        for (int k = 0; k < 3; ++k) { gleich = gleich && a.pos[k] == b->pos[k] && a.ang[k] == b->ang[k]; }
        if (gleich) { continue; }
        if (a.name == radiusFuer && a.laeuft && a.anim == b->anim && a.sichtbar == b->sichtbar) {
            radiusAbstand = abstand3(a.pos, b->pos);
            if (radiusAbstand <= 16.0F) { continue; }
        }
        t += " " + a.name + "[" + v3(a.pos) + "/" + v3(a.ang) + " " + a.anim + " -> " + v3(b->pos) + "/" + v3(b->ang) + " " + b->anim + "]";
        wer.push_back(a.name);
    }
    return wer;
}

struct UndoProbe {
    std::vector<Eintrag> anfang;   // Zustand bei t=0 vorher
    std::vector<Eintrag> ende;     // ... bei der Endzeit der Szene vorher
    double endeMs = 0.0;
    UndoStand a;
    UndoStand b;
    UndoStand c;
    UndoStand d;
    UndoStand e;
};

}  // namespace mission1

// Ein Fall: Zug, Strg+Z, Strg+Y, Strg+Z - mit den Staenden dazwischen.
void mission1UndoFall(std::vector<Schritt>& s, std::function<mission1::UndoZiel()> ziel, const std::string& name, bool drehen) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<mission1::UndoZiel>();
    auto p = std::make_shared<mission1::UndoProbe>();
    const std::string was = name + (drehen ? " drehen" : " verschieben");
    add(tu("Undo " + was + ": waehlen", [=] {
        *z = ziel();
        mission1::undoZielWaehlen(*z, drehen);
    }));
    add(pause(0.5));
    add(tu("Undo " + was + ": Stand vorher", [=] {
        p->a = mission1::undoStand(*z);
        p->endeMs = g_app->timeline.durationMs;
        p->anfang = mission1::szenenZustand(0.0);
        p->ende = mission1::szenenZustand(p->endeMs);
    }));
    add(pause(0.2));
    add(ziehenAn("Undo " + was, [=] { return mission1::undoGriff(*z, drehen); }, drehen ? 50.0F : 60.0F, drehen ? -25.0F : -10.0F, 0));
    add(pause(0.5));
    add(pruefe("Undo " + was + ": Anfang (t=0) und Ende der Szene bleiben, wo sie bleiben muessen", [=] {
        p->b = mission1::undoStand(*z);
        const std::vector<mission1::Eintrag> b0 = mission1::szenenZustand(0.0);
        // das Ende der Szene NACH der Aenderung (ein laengerer Weg verlaengert sie)
        const std::vector<mission1::Eintrag> bE = mission1::szenenZustand(g_app->timeline.durationMs);
        float r0 = 0.0F;
        float rE = 0.0F;
        std::string d0;
        std::string dE;
        // vor dem ersten Punkt: am Anfang darf sich nur das Ziel aendern, am Ende niemand
        // (eine laufende Figur kommt im Radius an); zwischen zwei: weder am Anfang noch
        // am Ende jemand; nach dem letzten: am Anfang niemand, am Ende nur das Ziel.
        const std::vector<std::string> w0 = mission1::abweichungen(p->anfang, b0, std::string(), std::string(), r0, d0);
        const std::vector<std::string> wE = mission1::abweichungen(p->ende, bE, z->wann == 2 ? std::string() : z->name, z->name, rE, dE);
        const auto nurZiel = [&](const std::vector<std::string>& w) {
            return std::all_of(w.begin(), w.end(), [&](const std::string& n) { return n == z->name; });
        };
        const bool ok0 = z->wann == 0 ? nurZiel(w0) : w0.empty();
        const bool okE = z->wann == 2 ? nurZiel(wE) : wE.empty();
        diag::detail("Knopftest: Szene " + was + " (" + z->name + ", Endzeit " + std::to_string(p->endeMs) + " ms) - bei t=0 abweichend:" +
                     (d0.empty() ? std::string(" niemand") : d0) + "; am Ende abweichend:" + (dE.empty() ? std::string(" niemand") : dE) +
                     (rE > 0.0F ? "; " + z->name + " kommt " + std::to_string(rE) + " Einheiten anders an (im Radius)" : std::string()));
        return ok0 && okE;
    }));
    add(tu("Undo " + was + ": Maus ueber die Karte", [=] {
        setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 30.0F, g_app->kartenAnsicht[3] - 30.0F});
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.4));
    add(tu("Undo " + was + ": Stand nach Strg+Z", [=] { p->c = mission1::undoStand(*z); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.4));
    add(tu("Undo " + was + ": Stand nach Strg+Y", [=] { p->d = mission1::undoStand(*z); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.4));
    add(pruefe("Undo " + was + ": Strg+Z stellt alles exakt wie vorher her, Strg+Y die Aenderung", [=] {
        p->e = mission1::undoStand(*z);
        const bool geaendert = !p->b.gleich(p->a);
        const bool undo = p->c.gleich(p->a);
        const bool redo = p->d.gleich(p->b);
        const bool wieder = p->e.gleich(p->a);
        diag::detail("Knopftest: Undo " + was + " (" + z->name + ", " + std::to_string(z->ms) + " ms) - geaendert " +
                     std::to_string(geaendert) + ", Undo gleich vorher " + std::to_string(undo) + " (Skript " +
                     std::to_string(p->c.skript == p->a.skript) + ", Entities " + std::to_string(p->c.ents == p->a.ents) +
                     ", Vorschau " + std::to_string(p->c.bild == p->a.bild) + "), Redo gleich nachher " + std::to_string(redo) +
                     ", zweites Undo " + std::to_string(wieder));
        diag::detail("Knopftest: Undo " + was + " - vorher: " + p->a.werte + " | Bild " + p->a.bild);
        diag::detail("Knopftest: Undo " + was + " - nach Aenderung: " + p->b.werte + " | Bild " + p->b.bild);
        diag::detail("Knopftest: Undo " + was + " - nach Undo: " + p->c.werte + " | Bild " + p->c.bild);
        diag::detail("Knopftest: Undo " + was + " - nach Redo: " + p->d.werte + " | Bild " + p->d.bild);
        return geaendert && undo && redo && wieder;
    }));
}

// Drei Zuege hintereinander, dann alle zurueck und alle wieder.
void mission1UndoKette(std::vector<Schritt>& s, const std::vector<std::pair<std::function<mission1::UndoZiel()>, bool>>& zuege,
                       const std::string& name) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto anfang = std::make_shared<mission1::UndoStand>();
    auto ende = std::make_shared<mission1::UndoStand>();
    auto staende = std::make_shared<std::vector<mission1::UndoStand>>();
    auto bezug = std::make_shared<mission1::UndoZiel>();
    add(tu("Kette " + name + ": Stand vorher", [=] {
        *bezug = zuege.front().first();
        *anfang = mission1::undoStand(*bezug);
        staende->clear();
    }));
    for (std::size_t i = 0; i < zuege.size(); ++i) {
        auto z = std::make_shared<mission1::UndoZiel>();
        const bool drehen = zuege[i].second;
        const auto zielFn = zuege[i].first;
        add(tu("Kette " + name + ": Zug " + std::to_string(i + 1) + " waehlen", [=] {
            *z = zielFn();
            mission1::undoZielWaehlen(*z, drehen);
        }));
        add(pause(0.5));
        add(ziehenAn("Kette " + name + ": Zug " + std::to_string(i + 1), [=] { return mission1::undoGriff(*z, drehen); },
                     drehen ? 45.0F : 50.0F, drehen ? -20.0F : -12.0F, 0));
        add(pause(0.5));
        add(tu("Kette " + name + ": Stand nach Zug " + std::to_string(i + 1), [=] { staende->push_back(mission1::undoStand(*bezug)); }));
    }
    add(tu("Kette " + name + ": Stand am Ende, Maus ueber die Karte", [=] {
        *ende = mission1::undoStand(*bezug);
        setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 30.0F, g_app->kartenAnsicht[3] - 30.0F});
    }));
    for (std::size_t i = 0; i < zuege.size(); ++i) {
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
        add(pause(0.4));
    }
    auto zurueck = std::make_shared<mission1::UndoStand>();
    add(tu("Kette " + name + ": Stand nach allen Strg+Z", [=] { *zurueck = mission1::undoStand(*bezug); }));
    for (std::size_t i = 0; i < zuege.size(); ++i) {
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
        add(pause(0.4));
    }
    add(pruefe("Kette " + name + ": " + std::to_string(zuege.size()) + " Zuege - alle Strg+Z exakt zum Anfang, alle Strg+Y exakt zum Ende", [=] {
        const mission1::UndoStand wieder = mission1::undoStand(*bezug);
        bool alleGeaendert = staende->size() == zuege.size();
        for (std::size_t i = 0; i < staende->size(); ++i) {
            const mission1::UndoStand& vor = i == 0 ? *anfang : (*staende)[i - 1];
            alleGeaendert = alleGeaendert && (vor.skript != (*staende)[i].skript || vor.ents != (*staende)[i].ents);
        }
        diag::detail("Knopftest: Kette " + name + " - jeder Zug aendert " + std::to_string(alleGeaendert) + ", zurueck gleich Anfang " +
                     std::to_string(zurueck->gleich(*anfang)) + " (Skript " + std::to_string(zurueck->skript == anfang->skript) +
                     ", Entities " + std::to_string(zurueck->ents == anfang->ents) + "), wieder gleich Ende " +
                     std::to_string(wieder.gleich(*ende)) + "; Anfang: " + anfang->werte + " | Ende: " + ende->werte +
                     " | zurueck: " + zurueck->werte + " | wieder: " + wieder.werte);
        return alleGeaendert && zurueck->gleich(*anfang) && wieder.gleich(*ende);
    }));
    for (std::size_t i = 0; i < zuege.size(); ++i) {
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
        add(pause(0.3));
    }
}

// Ziele in academy1/intro (Luke: drei Laeufe, Kamera) und md_am_sith (Schiff sta2, Kamera).
namespace mission1 {

// Die Figur mit den meisten eigenen Laeufen; Zeiten vor, zwischen, nach.
UndoZiel figurZiel(const std::string& teil, int wann) {
    UndoZiel z;
    z.art = ZielArt::Figur;
    z.figur = figurNamens(teil);
    if (z.figur < 0) { return z; }
    z.entity = entityFuerFigur(z.figur);
    z.name = g_app->scene.actors[static_cast<std::size_t>(z.figur)].name;
    std::vector<std::pair<double, double>> laeufe;
    for (const ActorStep& st : g_app->scene.actors[static_cast<std::size_t>(z.figur)].steps) {
        if (st.kind != ActorStep::Kind::WalkTo || st.path.empty() || !st.fremd.empty()) { continue; }
        if (!laeufe.empty() && laeufe.back().second >= st.startMs - 1.0) {
            laeufe.back().second = std::max(laeufe.back().second, st.endMs);
        } else {
            laeufe.emplace_back(st.startMs, st.endMs);
        }
    }
    if (laeufe.empty()) { return z; }
    z.wann = wann;
    if (wann == 0) { z.ms = 0.0; }   // der Anfang: vor allem, was das Skript mit der Figur tut
    if (wann == 1) { z.ms = std::floor((laeufe.front().first + laeufe.front().second) * 0.5 / 50.0) * 50.0; }
    if (wann == 2) { z.ms = std::floor((laeufe.back().second + 800.0) / 50.0) * 50.0; }
    return z;
}

UndoZiel schiffZiel(int wann) {
    UndoZiel z;
    z.art = ZielArt::Schiff;
    for (std::size_t ai = 0; ai < g_app->scene.actors.size() && z.figur < 0; ++ai) {
        const Actor& a = g_app->scene.actors[ai];
        if (a.brushModel <= 0) { continue; }
        double erst = -1.0;
        double letzt = -1.0;
        double mitte = -1.0;
        for (const ActorStep& st : a.steps) {
            if (st.kind != ActorStep::Kind::Glide || st.path.empty() || !st.fremd.empty()) { continue; }
            if (erst < 0.0) {
                erst = st.startMs;
                mitte = std::floor((st.startMs + st.endMs) * 0.5 / 50.0) * 50.0;
            }
            letzt = std::max(letzt, st.endMs);
        }
        if (erst < 0.0) { continue; }
        z.figur = static_cast<int>(ai);
        z.name = a.name;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* mdl = g_app->map.entities[i].find("model");
            if (mdl != nullptr && *mdl == "*" + std::to_string(a.brushModel)) { z.entity = static_cast<int>(i); }
        }
        z.ms = wann == 0 ? std::max(0.0, erst - 500.0) : wann == 1 ? mitte : std::floor((letzt + 1000.0) / 50.0) * 50.0;
        z.wann = wann;
    }
    return z;
}

UndoZiel kameraZiel(int wann) {
    UndoZiel z;
    z.art = ZielArt::Kamera;
    z.name = "Kamera";
    z.wann = wann;
    const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
    if (ks.size() < 3U) { return z; }
    std::vector<const CamSegment*> fahrten;
    for (const CamSegment& c : g_app->camTrack.segments) {
        if (c.kind == CamSegment::Kind::Move && c.fremd.empty()) { fahrten.push_back(&c); }
    }
    if (wann == 0) {
        z.schluessel = 0;
        kameraSchluesselWaehlen(0, true);
        z.ms = g_app->playMs;
    } else if (wann == 1) {
        z.schluessel = 1;
        kameraSchluesselWaehlen(1, true);
        // mitten in der Fahrt, die am Schluessel 1 endet
        for (const CamSegment* c : fahrten) {
            if (std::fabs(c->endMs - g_app->playMs) < 1.0 && c->endMs - c->startMs > 400.0) {
                z.ms = std::floor((c->startMs + c->endMs) * 0.5 / 50.0) * 50.0;
            }
        }
        if (z.ms <= 0.0) { z.ms = g_app->playMs; }
    } else {
        z.schluessel = static_cast<int>(ks.size()) - 1;
        kameraSchluesselWaehlen(z.schluessel, true);
        z.ms = g_app->playMs + 500.0;
    }
    return z;
}

}  // namespace mission1

std::vector<Schritt> mission1UndoSchritteAcademy() {
    std::vector<Schritt> s;
    for (int wann = 0; wann < 3; ++wann) {
        const char* w = wann == 0 ? "vor dem ersten" : wann == 1 ? "zwischen" : "nach dem letzten";
        for (const bool drehen : {false, true}) {
            mission1UndoFall(s, [wann] { return mission1::figurZiel("luke", wann); }, std::string("Figur ") + w, drehen);
        }
    }
    for (int wann = 0; wann < 3; ++wann) {
        const char* w = wann == 0 ? "vor dem ersten" : wann == 1 ? "zwischen" : "nach dem letzten";
        for (const bool drehen : {false, true}) {
            mission1UndoFall(s, [wann] { return mission1::kameraZiel(wann); }, std::string("Kamera academy1 ") + w, drehen);
        }
    }
    mission1UndoKette(s, {{[] { return mission1::figurZiel("luke", 1); }, false},
                          {[] { return mission1::figurZiel("luke", 2); }, true},
                          {[] { return mission1::kameraZiel(1); }, false}},
                      "academy1");
    return s;
}

std::vector<Schritt> mission1UndoSchritteSchiff() {
    std::vector<Schritt> s;
    for (int wann = 0; wann < 3; ++wann) {
        const char* w = wann == 0 ? "vor dem ersten" : wann == 1 ? "zwischen" : "nach dem letzten";
        for (const bool drehen : {false, true}) {
            mission1UndoFall(s, [wann] { return mission1::schiffZiel(wann); }, std::string("Schiff ") + w, drehen);
        }
    }
    for (int wann = 0; wann < 3; ++wann) {
        const char* w = wann == 0 ? "vor dem ersten" : wann == 1 ? "zwischen" : "nach dem letzten";
        for (const bool drehen : {false, true}) {
            mission1UndoFall(s, [wann] { return mission1::kameraZiel(wann); }, std::string("Kamera md_am_sith ") + w, drehen);
        }
    }
    mission1UndoKette(s, {{[] { return mission1::schiffZiel(1); }, true},
                          {[] { return mission1::schiffZiel(2); }, false},
                          {[] { return mission1::kameraZiel(1); }, false}},
                      "md_am_sith");
    return s;
}

namespace mission1 {

// Alle Bilder einer .gla samt Kino-Anhang (die gueltigen Bildnummern).
int alleBilder(const GlaAnimation& a) {
    int n = a.numFrames;
    for (const GlaAnimation* x = a.anhang; x != nullptr; x = x->anhang) { n += x->numFrames; }
    return n;
}

struct PoseBefund {
    bool geprueft = false;   // ohne Oberkoerperlauf und Uebergang: mit der Animation verglichen
    float abweichung = 0.0F; // groesste Abweichung eines Koerperknochens von seinem Bild
    bool imBereich = true;   // alle benutzten Bilder innerhalb der .gla samt Anhang
    bool kino = false;       // das Koerperbild liegt im Kino-Anhang
};

// Was gezeichnet wird (figurKnochen) gegen das, was die Animation in diesem
// Bild sagt (worldMatrices/worldMatricesLerp ungekappt, mit dem Mundlauf).
PoseBefund poseBefund(std::size_t ai, double ms) {
    PoseBefund b;
    std::vector<BoneMatrix> welt;
    ActorDraw d;
    if (!figurWelt(ai, ms, true, welt, d) || d.anim == nullptr) { return b; }
    const int n = alleBilder(*d.anim);
    for (const int f : {d.frame, d.frameNext, d.torsoFrame, d.torsoNext, d.faceFrame, d.prevFrame, d.torsoPrevFrame}) {
        b.imBereich = b.imBereich && f < n;
    }
    b.kino = d.frame >= d.anim->numFrames;
    if (d.torsoFrame >= 0 || (d.prevFrame >= 0 && d.blendLerp < 1.0F) || d.headYaw != 0.0F) { return b; }
    std::vector<BoneFrameOverride> over;
    const int gesicht = boneIndex(*d.anim, "face");
    if (d.faceFrame >= 0 && gesicht >= 0) { over.push_back(BoneFrameOverride{gesicht, d.faceFrame}); }
    std::vector<BoneMatrix> soll;
    if (d.frameNext >= 0 && d.frameFraction > 0.001F) {
        d.anim->worldMatricesLerp(d.frame, d.frameNext, d.frameFraction, over, soll);
    } else {
        d.anim->worldMatrices(d.frame, over, soll);
    }
    if (soll.size() != welt.size()) { return b; }
    b.geprueft = true;
    const std::vector<char> gt = gesichtTeil(*d.anim);
    for (std::size_t k = 0; k < welt.size(); ++k) {
        if (k < gt.size() && gt[k] != 0) { continue; }
        b.abweichung = std::max(b.abweichung, matrixAbstand(welt[k], soll[k]));
    }
    return b;
}

}  // namespace mission1

// --- Waechter: alle SP-Karten mit eigenem Kino-Skelett (_humanoid_<karte>) ---------
//
// Jede Figur, die eine Animation spielt, steht in dem Bild, das die Animation
// sagt - auch wenn es im Kino-Anhang liegt (vorher gekappt: T-Pose). Je Karte
// der Reiter, der beim Laden vorn ist, alle 500 ms.
std::vector<Schritt> mission1KinoSchritte(std::shared_ptr<mission1::Zustand> z) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto karten = std::make_shared<std::vector<std::string>>();
    auto bericht = std::make_shared<std::string>();
    auto fehler = std::make_shared<int>(0);
    add(tu("Kino: die Karten mit _humanoid_<karte> im reinen JKA", [=] {
        mission1::ungespeicherteEntitiesVergessen();
        karten->clear();
        Pk3 a1;
        if (readPk3Directory(z->jka + "\\assets1.pk3", a1)) {
            for (const Pk3Entry& e : a1.entries) {
                const std::string n = mission1::klein(e.name);
                const std::string vor = "models/players/_humanoid_";
                if (n.rfind(vor, 0) != 0 || n.size() < 5 || n.compare(n.size() - 4, 4, ".gla") != 0) { continue; }
                const std::string karte = n.substr(vor.size(), n.find('/', vor.size()) - vor.size());
                if (std::find(karten->begin(), karten->end(), karte) == karten->end()) { karten->push_back(karte); }
            }
        }
        // nur Karten, die es als maps/<karte>.bsp gibt (level.mapname)
        std::set<std::string> bsps;
        for (const char* a : {"assets0.pk3", "assets1.pk3", "assets2.pk3", "assets3.pk3"}) {
            Pk3 pk;
            if (!readPk3Directory(z->jka + "\\" + a, pk)) { continue; }
            for (const Pk3Entry& e : pk.entries) { bsps.insert(mission1::klein(e.name)); }
        }
        karten->erase(std::remove_if(karten->begin(), karten->end(),
                                     [&bsps](const std::string& k) { return bsps.count("maps/" + k + ".bsp") == 0; }),
                      karten->end());
        std::string t;
        for (const std::string& k : *karten) { t += " " + k; }
        diag::detail("Knopftest: Kino - " + std::to_string(karten->size()) + " Karten:" + t);
    }));
    for (int i = 0; i < 40; ++i) {
        add(tu("Kino: Karte " + std::to_string(i + 1) + " laden", [=] {
            if (static_cast<std::size_t>(i) >= karten->size()) { return; }
            g_app->missionPickOpen = false;
            missionenAusArchiv(z->jka + "\\assets1.pk3", (*karten)[static_cast<std::size_t>(i)]);
        }));
        // Warten, bis die Karte da ist; dann der intro-Reiter (sonst der vorderste).
        const auto geladen = [=] {
            if (static_cast<std::size_t>(i) >= karten->size()) { return true; }
            const std::string soll = "maps/" + (*karten)[static_cast<std::size_t>(i)] + ".bsp";
            return mission1::klein(g_app->map.path) == soll && g_app->camTrackValid;
        };
        Schritt warten{"Kino: Karte " + std::to_string(i + 1) + " geladen (ohne Mission ausgelassen)", [=](int b) { return geladen() || b > 8000; }};
        warten.eigeneZeitgrenze = true;
        add(std::move(warten));
        add(tu("Kino: Karte " + std::to_string(i + 1) + " intro-Reiter", [=] {
            if (static_cast<std::size_t>(i) >= karten->size() || !geladen()) { return; }
            // der intro-Reiter DIESER Mission (die Reiter frueherer Karten bleiben offen)
            for (const Mission& mi : g_app->missions) {
                if (mi.name != (*karten)[static_cast<std::size_t>(i)]) { continue; }
                for (const Mission::Script& sc : mi.scripts) {
                    const std::string r = mission1::klein(sc.ref);
                    if (r.size() >= 6 && r.compare(r.size() - 6, 6, "/intro") == 0 && reiterFuerSkriptRef(sc.ref) >= 0) {
                        g_app->tabRequest = reiterFuerSkriptRef(sc.ref);
                    }
                }
            }
        }));
        add(pause(1.0));
        add({"Kino: Karte " + std::to_string(i + 1) + " Szene", [=](int b) { return (g_app->camTrackValid && jk2probe::figurenGeladen()) || b > 1500; }});
        add(tu("Kino: Karte " + std::to_string(i + 1) + " pruefen", [=] {
            if (static_cast<std::size_t>(i) >= karten->size()) { return; }
            const std::string soll = "maps/" + (*karten)[static_cast<std::size_t>(i)] + ".bsp";
            if (mission1::klein(g_app->map.path) != soll) {
                *bericht += "; " + (*karten)[static_cast<std::size_t>(i)] + ": nicht geladen";
                diag::detail("Knopftest: Kino - " + (*karten)[static_cast<std::size_t>(i)] + ": nicht geladen (keine Mission)");
                return;
            }
            prepareScene();
            const double ende = std::min(std::max(g_app->timeline.durationMs, 1000.0), 240000.0);
            int proben = 0;
            int kino = 0;
            int falsch = 0;
            std::string erste;
            for (double ms = 0.0; ms < ende; ms += 500.0) {
                for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
                    if (!g_app->scene.actors[ai].at(ms).visible) { continue; }
                    const mission1::PoseBefund b = mission1::poseBefund(ai, ms);
                    if (!b.geprueft && b.imBereich) { continue; }
                    ++proben;
                    kino += b.kino ? 1 : 0;
                    if (!b.imBereich || b.abweichung > 0.001F) {
                        ++falsch;
                        if (erste.size() < 300) {
                            erste += " " + g_app->scene.actors[ai].name + "@" + std::to_string(static_cast<int>(ms)) + "(" +
                                     std::to_string(b.abweichung) + ")";
                        }
                    }
                }
            }
            *fehler += falsch;
            const std::string zeile = (*karten)[static_cast<std::size_t>(i)] + " (" + g_app->shownName + "): " + std::to_string(proben) +
                                      " Proben, " + std::to_string(kino) + " im Kino-Anhang, " + std::to_string(falsch) + " falsch" + erste;
            *bericht += "; " + zeile;
            diag::detail("Knopftest: Kino - " + zeile);
        }));
    }
    add(pruefe("Kino: auf allen Karten mit eigenem Kino-Skelett steht jede Figur im Bild ihrer Animation (auch aus dem Anhang)", [=] {
        diag::detail("Knopftest: Kino - Summe " + std::to_string(karten->size()) + " Karten, " + std::to_string(*fehler) + " falsche Proben");
        return !karten->empty() && *fehler == 0;
    }));
    return s;
}

// --- Missionen mit Skripten einer ANDEREN Karte (academy4, academy6, vjun1) -------
//
// academy4 und academy6 benutzen die Skripte academy2/studentspawn*, vjun1
// die von hoth2 und common. War die andere Mission vorher geladen, nahm
// "Load mission" deren schon offene Reiter mit - samt deren Karte - und
// schaltete auf den ersten: die neue Mission erschien nicht.
std::vector<Schritt> mission1LadenSchritte(std::shared_ptr<mission1::Zustand> z) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::pair<const char*, const char*> paare[] = {{"academy2", "academy4"}, {"academy2", "academy6"}, {"hoth2", "vjun1"}};
    for (const auto& [vorher, karte] : paare) {
        const std::string v = vorher;
        const std::string k = karte;
        add(tu("Laden: " + v + " (teilt Skripte mit " + k + ")", [=] {
            mission1::ungespeicherteEntitiesVergessen();
            g_app->missionPickOpen = false;
            missionenAusArchiv(z->jka + "\\assets1.pk3", v);
        }));
        add(warteBis("Laden: " + v + " geladen", [=] { return g_app->map.path == "maps/" + v + ".bsp" && g_app->camTrackValid; }, 3000));
        add(tu("Laden: danach " + k, [=] {
            mission1::ungespeicherteEntitiesVergessen();
            g_app->missionPickOpen = false;
            missionenAusArchiv(z->jka + "\\assets1.pk3", k);
        }));
        add(pause(2.0));
        add(pruefe("Laden: " + k + " nach " + v + " - die Karte von " + k + " ist geladen, ihre Skripte haben Reiter mit dieser Karte", [=] {
            int skripte = 0;
            int mitKarte = 0;
            for (const Mission& mi : g_app->missions) {
                if (mi.name != k) { continue; }
                for (const Mission::Script& sc : mi.scripts) {
                    ++skripte;
                    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                        const bool aktiv = static_cast<int>(i) == g_app->activeTab;
                        const std::string r = mission1::klein(aktiv ? g_app->skriptPfad : g_app->tabs[i].skriptPfad);
                        const std::string karteDes = aktiv ? g_app->map.path : g_app->tabs[i].mapId;
                        if (r.find(mission1::klein(sc.ref)) != std::string::npos && karteDes == "maps/" + k + ".bsp") {
                            ++mitKarte;
                            break;
                        }
                    }
                }
            }
            diag::detail("Knopftest: Laden - " + k + " nach " + v + ": Karte jetzt " + g_app->map.path + ", aktiver Reiter " + g_app->shownName +
                         ", " + std::to_string(mitKarte) + " von " + std::to_string(skripte) + " Skripten mit Reiter dieser Karte");
            return g_app->map.path == "maps/" + k + ".bsp" && skripte > 0 && mitKarte == skripte;
        }));
    }
    return s;
}

// Laden von academy1 (md: ueber die eingerichteten Spielordner, sonst das reine
// JKA) und die Probe der Gesichtsanimation.
std::vector<Schritt> mission1GesichtSchritte(std::shared_ptr<mission1::Zustand> z, bool md) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string k = md ? "MD" : "JKA";
    add(tu("Mission1 " + k + ": Load mission (" + k + ")", [=] { missionenAusArchiv(md ? z->mdArchiv : z->jka + "\\assets1.pk3"); }));
    add(warteBis("Mission1 " + k + ": academy1 in der Auswahl", [] { return missionsliste::hat("academy1", true); }, 3500));
    add(waehleInListe("academy1", false, 0, "##missionpick"));
    add(warteBis("Mission1 " + k + ": academy1 geladen", [] {
        return g_app->map.path == "maps/academy1.bsp" && g_app->camTrackValid && g_app->missionAktiv;
    }, 3000));
    // --- Werden die Skripte der Story-Karte vollstaendig geladen? -----------
    add(pruefe("Mission1 " + k + ": academy1 - jedes Skript der Karte hat seinen Reiter, intro ist dabei", [] {
        std::string fehlt;
        int n = 0;
        for (const Mission& m : g_app->missions) {
            if (m.name != "academy1") { continue; }
            for (const Mission::Script& sc : m.scripts) {
                ++n;
                if (reiterFuerSkriptRef(sc.ref) < 0) { fehlt += " " + sc.ref; }
            }
        }
        diag::detail("Knopftest: Mission1 - academy1: " + std::to_string(n) + " Skriptverweise, " +
                     std::to_string(g_app->tabs.size()) + " Reiter, ohne Reiter:" + (fehlt.empty() ? std::string(" keiner") : fehlt));
        return n > 0 && fehlt.empty() && mission1::reiterMit("intro") >= 0;
    }));
    add(tu("Mission1 " + k + ": Reiter academy1/intro", [] { g_app->tabRequest = mission1::reiterMit("intro"); }));
    add(pause(1.0));
    add(warteBis("Mission1 " + k + ": Szene von intro", [] {
        return g_app->camTrackValid && mission1::klein(g_app->shownName).find("intro") != std::string::npos &&
               mission1::figurNamens("luke") >= 0 && jk2probe::figurenGeladen();
    }, 1500));
    add(tu("Mission1 " + k + ": Figuren und Schritte der Szene", [] {
        std::string t;
        for (const Actor& a : g_app->scene.actors) {
            int laeufe = 0;
            for (const ActorStep& st : a.steps) { laeufe += st.kind == ActorStep::Kind::WalkTo ? 1 : 0; }
            t += " " + a.name + "(" + a.npcType + "," + std::to_string(laeufe) + " Laeufe)";
        }
        diag::detail("Knopftest: Mission1 - intro, Figuren:" + t);
    }));
    // --- 1. Gesichtsanimation -------------------------------------------------
    add(tu("Gesicht " + k + ": eine Zeit suchen, zu der Luke spricht (FACE_TALK), und eine stille davor", [=] {
        z->luke = mission1::figurNamens("luke");
        z->redeMs = -1.0;
        z->stillMs = -1.0;
        const double ende = std::min(std::max(g_app->timeline.durationMs, 1000.0), 300000.0);
        for (double ms = 0.0; ms < ende && z->luke >= 0; ms += 50.0) {
            std::vector<BoneMatrix> w;
            ActorDraw d;
            if (!mission1::figurWelt(static_cast<std::size_t>(z->luke), ms, true, w, d)) { continue; }
            if (d.faceFrame < 0) {
                z->stillMs = ms;
            } else if (ms > 1000.0 && z->stillMs >= 0.0) {
                // ein Stueck in den Satz hinein - dort ist der Mund offen
                z->redeMs = ms + 400.0;
                break;
            }
        }
        diag::detail("Knopftest: Gesicht " + k + " - Luke Figur " + std::to_string(z->luke) + ", spricht bei " +
                     std::to_string(z->redeMs) + " ms, still bei " + std::to_string(z->stillMs) + " ms");
    }));
    add(pruefe("Gesicht " + k + ": Luke spricht in intro (FACE_TALK aus der Lautstaerke seines Klangs)", [=] { return z->redeMs > 0.0; }));
    add(tu("Gesicht " + k + ": durch die Kamera, still", [=] {
        g_app->playing = false;
        g_app->throughCamera = true;
        g_app->playMs = z->stillMs;
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(foto("m1_gesicht_still_" + k));
    add(tu("Gesicht " + k + ": durch die Kamera, Luke spricht", [=] {
        g_app->playMs = z->redeMs;
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(foto("m1_gesicht_rede_" + k));
    add(pruefe("Gesicht " + k + ": beim Sprechen bleibt jeder Koerper in seiner Animation, nur das Gesicht bewegt sich", [=] {
        const mission1::KoerperBefund still = mission1::koerperBefund(z->stillMs);
        const mission1::KoerperBefund rede = mission1::koerperBefund(z->redeMs);
        char t[300];
        std::snprintf(t, sizeof(t),
                      "Knopftest: Gesicht - still: %d Figuren, %d in Grundstellung; Rede: %d Figuren, %d sprechen, %d in "
                      "Grundstellung, Koerper durch Mund %.4f, Gesicht durch Mund %.4f;",
                      still.figuren, still.grundstellung, rede.figuren, rede.sprechen, rede.grundstellung,
                      static_cast<double>(rede.koerperMax), static_cast<double>(rede.gesichtMax));
        diag::detail(std::string(t) + " [" + k + "]" + rede.text);
        return rede.figuren > 1 && rede.sprechen >= 1 && rede.grundstellung == 0 && rede.koerperMax < 0.001F &&
               rede.gesichtMax > 0.01F;
    }));
    add(tu("Gesicht " + k + ": freie Ansicht nah vor Luke, er spricht", [=] {
        FolgeZiel f;
        f.actor = z->luke;
        f.entity = z->luke >= 0 ? entityFuerFigur(z->luke) : -1;
        f.ms = z->redeMs;
        folgeKameraVor(f, 150.0F, 40.0F);
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(foto("m1_gesicht_nah_" + k));
    // Die ganze Szene: zu keiner Zeit steht ein Koerper in der Grundstellung,
    // und kein Mundlauf aendert einen Koerperknochen (alle Sprecher, alle Saetze).
    // shank (mit Foto): bei 0:16.72 (Kamera lukewalk2) laeuft Luke die Treppe
    // in T-Pose herunter.
    add(tu("Gesicht " + k + ": Luke auf der Treppe, 16720 ms, durch die Kamera", [=] {
        g_app->playing = false;
        g_app->throughCamera = true;
        g_app->playMs = 16720.0;
        g_app->mapDirty = true;
        if (z->luke >= 0) {
            const Actor& a = g_app->scene.actors[static_cast<std::size_t>(z->luke)];
            const ActorState st = a.at(16720.0);
            std::vector<BoneMatrix> w;
            ActorDraw d;
            const bool ok = mission1::figurWelt(static_cast<std::size_t>(z->luke), 16720.0, true, w, d);
            const std::string skel = g_app->actorAssets.size() > static_cast<std::size_t>(z->luke) && g_app->actorAssets[static_cast<std::size_t>(z->luke)]
                                         ? g_app->actorAssets[static_cast<std::size_t>(z->luke)]->model.animFile : std::string("-");
            diag::detail("Knopftest: Gesicht " + k + " - Luke bei 16720 ms: Animation \"" + st.animation + "\" (vorher \"" + st.prevAnimation +
                         "\"), Oberkoerper \"" + st.torsoAnimation + "\", Skelett \"" + skel + "\", Bild " + std::to_string(d.frame) +
                         ", Oberkoerperbild " + std::to_string(d.torsoFrame) + ", Mund " + std::to_string(d.faceFrame) + ", Knochen " +
                         std::to_string(ok ? w.size() : 0));
        }
    }));
    add(pause(0.6));
    add(foto("m1_treppe_" + k));
    add(pruefe("Gesicht " + k + ": Luke auf der Treppe spielt BOTH_CIN_1 aus dem Kino-Skelett, nicht in T-Pose", [=] {
        if (z->luke < 0) { return false; }
        const mission1::PoseBefund b = mission1::poseBefund(static_cast<std::size_t>(z->luke), 16720.0);
        std::vector<BoneMatrix> w;
        ActorDraw d;
        (void)mission1::figurWelt(static_cast<std::size_t>(z->luke), 16720.0, true, w, d);
        const bool kinoMund = d.anim != nullptr && d.faceFrame >= d.anim->numFrames;
        diag::detail("Knopftest: Gesicht " + k + " - Treppe: Bild " + std::to_string(d.frame) + " von " +
                     std::to_string(d.anim != nullptr ? mission1::alleBilder(*d.anim) : 0) + " (Grundskelett " +
                     std::to_string(d.anim != nullptr ? d.anim->numFrames : 0) + "), im Anhang " + std::to_string(b.kino) +
                     ", verglichen " + std::to_string(b.geprueft) + ", Abweichung vom Bild der Animation " + std::to_string(b.abweichung) +
                     ", Mund " + std::to_string(d.faceFrame) + (kinoMund ? " (FACE_TALK aus dem Kino-Skelett)" : ""));
        return b.geprueft && b.imBereich && b.abweichung < 0.001F;
    }));
    add(pruefe("Gesicht " + k + ": ueber die ganze Szene (alle 250 ms) kein Koerper in Grundstellung, Mund nur im Gesicht", [=] {
        const double ende = std::min(std::max(g_app->timeline.durationMs, 1000.0), 300000.0);
        int zeiten = 0;
        int mitRede = 0;
        int fehler = 0;
        std::string erste;
        float koerperMax = 0.0F;
        for (double ms = 250.0; ms < ende; ms += 250.0) {
            const mission1::KoerperBefund b = mission1::koerperBefund(ms);
            ++zeiten;
            mitRede += b.sprechen > 0 ? 1 : 0;
            koerperMax = std::max(koerperMax, b.koerperMax);
            if (b.grundstellung > 0 || b.koerperMax > 0.001F) {
                ++fehler;
                if (erste.size() < 600) { erste += " @" + std::to_string(static_cast<int>(ms)) + b.text; }
            }
        }
        diag::detail("Knopftest: Gesicht " + k + " - Szene: " + std::to_string(zeiten) + " Zeiten, " + std::to_string(mitRede) +
                     " mit Sprecher, " + std::to_string(fehler) + " mit Koerper in Grundstellung oder vom Mund bewegt (max " +
                     std::to_string(koerperMax) + ")" + erste);
        return zeiten > 10 && mitRede > 0 && fehler == 0;
    }));
    // Beim echten Abspielen durch die Kamera: Bilder, waehrend gesprochen wird.
    add(tu("Gesicht " + k + ": Abspielen ab kurz vor Lukes Satz, durch die Kamera", [=] {
        g_app->throughCamera = true;
        g_app->playMs = std::max(0.0, z->redeMs - 600.0);
        g_app->mapDirty = true;
    }));
    add(klick(tr(Str::TlPlay), "###main"));
    add(pause(1.2));
    add(foto("m1_gesicht_spielt_" + k));
    add(klick(tr(Str::TlStop), "###main"));
    add(pause(0.3));
    add(tu("Gesicht " + k + ": freie Ansicht", [] {
        g_app->throughCamera = false;
        g_app->mapDirty = true;
    }));
    return s;
}

// --- Eine selbst gebaute Mini-Szene (shank: nichts auf vorhandene Karten oder
// Skripte zugeschnitten) ------------------------------------------------------
//
// Eine neue Mission "m1szene" (New mission auf der Geometrie von md_am_sith),
// eigene Entities (Figur m1_held, Laufziel m1_ziel, Schiff m1_schiff), eigenes
// Skript, und ein eigenes Kino-Skelett models/players/_humanoid_m1szene/ in
// einer pk3 im Laufordner. Darin dieselben Proben: Kino-Animation, Zwischen-
// punkt, Hilfen beim Abspielen, Drehen nach der letzten Fahrt, Rueckgaengig.
namespace mission1 {
struct MiniZustand {
    float von[3]{};       // Lauf einer Figur der Grundkarte (begehbar)
    float bis[3]{};
    std::string typ;      // ihr NPC_type
    std::string skelett;  // ihr Skelett (Pfad ohne Endung)
    std::string schiffModell;   // "*N" des Schiffs der Grundkarte
    float flug[2][3]{};   // zwei Ziele seiner Fahrten
    bool bereit = false;
    std::string fehler;
};
}  // namespace mission1

std::vector<Schritt> mission1MiniSchritte(std::shared_ptr<mission1::Zustand> z) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto m = std::make_shared<mission1::MiniZustand>();
    add(tu("Mini: Grundkarte md_am_sith laden", [=] {
        mission1::ungespeicherteEntitiesVergessen();
        g_app->missionPickOpen = false;
        missionenAusArchiv(z->mdArchiv, "md_am_sith");
    }));
    add(warteBis("Mini: Grundkarte geladen", [] {
        return g_app->map.path == "maps/md_am_sith.bsp" && g_app->camTrackValid && jk2probe::figurenGeladen();
    }, 4000));
    add(tu("Mini: begehbare Strecke, Schiff und seine Fahrten der Grundkarte merken", [=] {
        prepareScene();
        for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            for (const ActorStep& st : a.steps) {
                if (m->typ.empty() && st.kind == ActorStep::Kind::WalkTo && !a.npcType.empty() && mission1::abstand3(st.from, st.to) > 150.0F &&
                    mission1::abstand3(st.from, st.to) < 380.0F && ai < g_app->actorAssets.size() && g_app->actorAssets[ai]) {
                    for (int k = 0; k < 3; ++k) {
                        m->von[k] = st.from[k];
                        m->bis[k] = st.to[k];
                    }
                    m->typ = a.npcType;
                    m->skelett = g_app->actorAssets[ai]->model.animFile;
                }
            }
            if (a.brushModel > 0 && m->schiffModell.empty()) {
                int n = 0;
                for (const ActorStep& st : a.steps) {
                    if (st.kind != ActorStep::Kind::Glide || n >= 2) { continue; }
                    for (int k = 0; k < 3; ++k) { m->flug[n][k] = st.to[k]; }
                    ++n;
                }
                if (n == 2) { m->schiffModell = "*" + std::to_string(a.brushModel); }
            }
        }
        diag::detail("Knopftest: Mini - Strecke " + mission1::v3(m->von) + " -> " + mission1::v3(m->bis) + " (" + m->typ + "), Schiff " +
                     m->schiffModell + " nach " + mission1::v3(m->flug[0]) + " / " + mission1::v3(m->flug[1]));
    }));
    add(tu("Mini: eigenes Kino-Skelett _humanoid_m1szene als pk3 in einen eigenen Spielordner", [=] {
        // Die .gla des Skeletts der Figur (gleiche Knochen, sonst haengt es die
        // Engine nicht an) und eine eigene animation.cfg mit BOTH_CIN_1 - den
        // Bildern von BOTH_STAND1 aus der .cfg des Skeletts.
        std::string gla;
        std::string grundCfg;
        const std::string ordner = m->skelett.substr(0, m->skelett.find_last_of('/'));
        bool da = readFromArchives(m->skelett + ".gla", gla) &&
                  (readFromArchives(m->skelett + ".cfg", grundCfg) || readFromArchives(ordner + "/animation.cfg", grundCfg));
        std::string cfg;
        std::istringstream zeilen(grundCfg);
        std::string zeile;
        while (da && std::getline(zeilen, zeile)) {
            std::istringstream w(zeile);
            std::string name;
            w >> name;
            if (name == "BOTH_STAND1") {
                std::string rest;
                std::getline(w, rest);
                cfg = "BOTH_CIN_1\t" + rest + "\n";
            }
        }
        da = da && !cfg.empty();
        const std::string spiel = mb::verbinde(mb::laufOrdner(), "m1spiel");
        std::error_code ec;
        std::filesystem::create_directories(::bhed::utf8Pfad(spiel), ec);
        std::string f;
        const bool ok = da && writePk3(mb::verbinde(spiel, "zz_m1szene.pk3"),
                                       {{"models/players/_humanoid_m1szene/_humanoid_m1szene.gla", gla},
                                        {"models/players/_humanoid_m1szene/animation.cfg", cfg}},
                                       &f);
        if (ok) {
            g_app->settings.gamePaths.push_back(spiel);
            rescanGamePaths();
            refreshPk3List();
        }
        m->fehler = ok ? std::string() : "Kino-pk3: " + f;
        diag::detail("Knopftest: Mini - Kino-Skelett geschrieben " + std::to_string(ok) + " (" + std::to_string(gla.size()) + " Bytes .gla)");
    }));
    add(tu("Mini: New mission m1szene auf der Grundkarte, eigene Entities und eigenes Skript", [=] {
        const std::string wurzel = mb::verbinde(mb::laufOrdner(), "missionen");
        _putenv_s("BHED_MISSIONEN", wurzel.c_str());
        std::string f;
        if (!neueMissionAnlegen("maps/md_am_sith.bsp", "m1szene", "m1szene", &f)) {
            m->fehler += " New mission: " + f;
            return;
        }
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* mdl = g_app->map.entities[i].find("model");
            if (mdl != nullptr && *mdl == m->schiffModell) { (void)entityKeySetzen(static_cast<int>(i), "script_targetname", "m1_schiff"); }
        }
        const int held = neueEntityAn("NPC_spawner", m->von);
        const int ziel = neueEntityAn("waypoint_navgoal", m->bis);
        if (held < 0 || ziel < 0) {
            m->fehler += " Entities nicht angelegt";
            return;
        }
        (void)entityKeySetzen(held, "NPC_type", m->typ);
        (void)entityKeySetzen(held, "NPC_targetname", "m1_held");
        (void)entityKeySetzen(ziel, "targetname", "m1_ziel");
        // Drei Kameraschluessel entlang der Strecke (hinter und ueber ihr).
        const auto kam = [&](const float* o, float dy) {
            const float k[3] = {o[0] - 220.0F, o[1] + dy, o[2] + 110.0F};
            return mission1::v3(k);
        };
        float mitte[3];
        for (int q = 0; q < 3; ++q) { mitte[q] = (m->von[q] + m->bis[q]) * 0.5F; }
        const std::string eigen = std::string(R"(//Generated by BehavEd

camera ( ENABLE );
camera ( MOVE, < )") + kam(m->von, 0.0F) + R"( >, 0.000 );
camera ( PAN, < 15.000 0.000 0.000 >, < 0.000 0.000 0.000 >, 0.000 );

affect ( "m1_held", FLUSH )
{
	set ( "SET_ANIM_BOTH", "BOTH_CIN_1" );
	set ( "SET_ANIM_HOLDTIME_BOTH", 1500 );
	wait ( 2000.000 );
	set ( "SET_WALKING", "true" );
	set ( "SET_NAVGOAL", "m1_ziel" );
	wait ( 8000.000 );
}

affect ( "m1_schiff", FLUSH )
{
	move ( < )" + mission1::v3(m->flug[0]) + R"( >, < 0.000 0.000 0.000 >, 3000.000 );
	wait ( 3000.000 );
	move ( < )" + mission1::v3(m->flug[1]) + R"( >, < 0.000 90.000 0.000 >, 3000.000 );
	wait ( 3000.000 );
}

wait ( 1000.000 );
camera ( MOVE, < )" + kam(mitte, 80.0F) + R"( >, 4000.000 );
camera ( PAN, < 15.000 20.000 0.000 >, < 0.000 0.000 0.000 >, 4000.000 );
wait ( 4000.000 );
camera ( MOVE, < )" + kam(m->bis, 0.0F) + R"( >, 4000.000 );
camera ( PAN, < 15.000 -20.000 0.000 >, < 0.000 0.000 0.000 >, 4000.000 );
wait ( 7000.000 );
camera ( DISABLE );
)";
        Script neu;
        std::vector<Diag> d;
        if (!readScript(eigen, neu, d)) {
            m->fehler += " Skript nicht lesbar";
            return;
        }
        g_app->doc.ersetzeSkript(std::move(neu), "m1szene");
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
        m->bereit = true;
    }));
    add(warteBis("Mini: Szene von m1szene", [=] {
        return !m->bereit || (g_app->camTrackValid && mission1::figurNamens("m1_held") >= 0 && jk2probe::figurenGeladen());
    }, 2000));
    add(pause(0.5));
    add(pruefe("Mini: m1szene - eigene Figur, eigenes Schiff, eigenes Skript, eigenes Kino-Skelett angehaengt", [=] {
        prepareScene();
        const int held = mission1::figurNamens("m1_held");
        int schiff = -1;
        for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
            if (g_app->scene.actors[i].brushModel > 0 && g_app->scene.actors[i].name == "m1_schiff") { schiff = static_cast<int>(i); }
        }
        diag::detail("Knopftest: Mini - Karte " + g_app->map.path + ", Figur " + std::to_string(held) + ", Schiff " + std::to_string(schiff) +
                     (m->fehler.empty() ? std::string() : ", Fehler:" + m->fehler));
        return m->bereit && m->fehler.empty() && g_app->map.path == "maps/m1szene.bsp" && held >= 0 && schiff >= 0;
    }));
    add(pruefe("Mini: m1_held spielt BOTH_CIN_1 aus _humanoid_m1szene im Bild der Animation (keine T-Pose)", [=] {
        const int held = mission1::figurNamens("m1_held");
        if (held < 0) { return false; }
        const mission1::PoseBefund b = mission1::poseBefund(static_cast<std::size_t>(held), 800.0);
        std::vector<BoneMatrix> w;
        ActorDraw d;
        (void)mission1::figurWelt(static_cast<std::size_t>(held), 800.0, true, w, d);
        diag::detail("Knopftest: Mini - m1_held bei 800 ms: Animation \"" + g_app->scene.actors[static_cast<std::size_t>(held)].at(800.0).animation +
                     "\", Bild " + std::to_string(d.frame) + " (Grundskelett " + std::to_string(d.anim != nullptr ? d.anim->numFrames : 0) +
                     "), im Anhang " + std::to_string(b.kino) + ", Abweichung " + std::to_string(b.abweichung));
        return b.geprueft && b.kino && b.abweichung < 0.001F;
    }));
    for (Schritt& x : mission1ZwischenSchritte("m1_held", false, "_mini")) { s.push_back(std::move(x)); }
    for (Schritt& x : mission1GizmoSchritte("m1_held", "_mini")) { s.push_back(std::move(x)); }
    // Drehen nach der letzten Fahrt: der Anfang bleibt, ein neuer Schluessel.
    struct Nach {
        mission1::UndoZiel z;
        int moves = 0;
        float ang0[3]{};
        float angE[3]{};
        double ende = 0.0;
        std::string entWinkel;
    };
    auto np = std::make_shared<Nach>();
    add(tu("Mini: m1_schiff nach der letzten Fahrt waehlen, Drehmodus", [=] {
        np->z = mission1::schiffZiel(2);
        if (np->z.figur < 0) { return; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(np->z.figur)];
        for (const ActorStep& st : a.steps) {
            if (st.kind == ActorStep::Kind::Glide && !st.path.empty() && st.fremd.empty()) { np->ende = std::max(np->ende, st.endMs); }
        }
        const ActorState s0 = a.at(0.0);
        const ActorState sE = a.at(np->ende);
        for (int k = 0; k < 3; ++k) {
            np->ang0[k] = s0.angles[k];
            np->angE[k] = sE.angles[k];
        }
        const std::string* w = g_app->map.entities[static_cast<std::size_t>(np->z.entity)].find("angles");
        np->entWinkel = w != nullptr ? *w : std::string();
        np->moves = static_cast<int>(mission1::befehle("move", nullptr).size());
        mission1::undoZielWaehlen(np->z, true);
    }));
    add(pause(0.5));
    add(ziehenAn("Mini: am Gier-Ring des Schiffs ziehen", [=] { return mission1::undoGriff(np->z, true); }, 50.0F, -25.0F, 0));
    add(pause(0.6));
    add(pruefe("Mini: m1_schiff nach der letzten Fahrt gedreht - neuer Schluessel, Anfang und letzter Schluessel bleiben", [=] {
        if (np->z.figur < 0) { return false; }
        prepareScene();
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(np->z.figur)];
        const ActorState s0 = a.at(0.0);
        const ActorState sE = a.at(np->ende);
        const ActorState sT = a.at(np->z.ms + 1.0);
        const std::string* w = g_app->map.entities[static_cast<std::size_t>(np->z.entity)].find("angles");
        const std::string entJetzt = w != nullptr ? *w : std::string();
        const int moves = static_cast<int>(mission1::befehle("move", nullptr).size());
        const float dA = mission1::winkelAbstand3(s0.angles, np->ang0);
        const float dE = mission1::winkelAbstand3(sE.angles, np->angE);
        const float dT = mission1::winkelAbstand3(sT.angles, np->angE);
        diag::detail("Knopftest: Mini - Drehen nach der letzten Fahrt (" + std::to_string(np->z.ms) + " ms): moves " + std::to_string(np->moves) +
                     " -> " + std::to_string(moves) + ", Entity-Winkel \"" + np->entWinkel + "\" -> \"" + entJetzt + "\", Anfang " +
                     std::to_string(dA) + ", letzter Schluessel " + std::to_string(dE) + ", zur Zeit gedreht " + std::to_string(dT));
        return moves == np->moves + 1 && entJetzt == np->entWinkel && dA < 0.05F && dE < 0.05F && dT > 2.0F;
    }));
    add(tu("Mini: zuruecknehmen", [] {
        setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 30.0F, g_app->kartenAnsicht[3] - 30.0F});
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.4));
    // Rueckgaengig/Wiederholen in der Mini-Szene
    mission1UndoFall(s, [] { return mission1::figurZiel("m1_held", 1); }, "Mini Figur zwischen", false);
    mission1UndoFall(s, [] { return mission1::figurZiel("m1_held", 2); }, "Mini Figur nach dem letzten", true);
    mission1UndoFall(s, [] { return mission1::schiffZiel(1); }, "Mini Schiff zwischen", true);
    mission1UndoFall(s, [] { return mission1::schiffZiel(2); }, "Mini Schiff nach dem letzten", false);
    mission1UndoFall(s, [] { return mission1::kameraZiel(1); }, "Mini Kamera zwischen", false);
    mission1UndoKette(s, {{[] { return mission1::figurZiel("m1_held", 1); }, false},
                          {[] { return mission1::schiffZiel(2); }, true},
                          {[] { return mission1::kameraZiel(1); }, false}},
                      "m1szene");
    return s;
}

std::vector<Schritt> mission1Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<mission1::Zustand>();
    add(tu("Mission1: Ausgangslage (reines JKA, eingerichtete Spielordner)", [=] {
        const char* e = std::getenv("BHED_JKA_BASIS");
        z->jka = e != nullptr ? e : "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Jedi Academy\\GameData original\\base";
        for (const std::string& o : g_app->settings.gamePaths) {
            std::error_code ec;
            if (z->mdArchiv.empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(o + "\\assets1.pk3"), ec)) {
                z->mdArchiv = o + "\\assets1.pk3";
            }
        }
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        diag::detail("Knopftest: Mission1 - reines JKA \"" + z->jka + "\", eingerichtet \"" + z->mdArchiv + "\"");
    }));
    // BHED_M1_TEIL: nur einzelne Teile (md, jka, zwischen, gizmo, drehen, undo, undoa = nur academy1, kino, mini, laden -
    // durch Leerzeichen getrennt); ohne: alles. zwischen/gizmo brauchen jka.
    const std::string teile = std::getenv("BHED_M1_TEIL") != nullptr ? std::string(" ") + std::getenv("BHED_M1_TEIL") + " " : std::string();
    const auto mit = [&teile](const char* t) { return teile.empty() || teile.find(std::string(" ") + t + " ") != std::string::npos; };
    if (mit("md")) {
        for (Schritt& x : mission1GesichtSchritte(z, true)) { s.push_back(std::move(x)); }
    }
    if (mit("jka") || mit("zwischen") || mit("gizmo") || mit("undo") || mit("undoa")) {
        for (Schritt& x : mission1GesichtSchritte(z, false)) { s.push_back(std::move(x)); }
    }
    if (mit("zwischen")) {
        for (Schritt& x : mission1ZwischenSchritte("rosh", true, "")) { s.push_back(std::move(x)); }
    }
    if (mit("undo") || mit("undoa")) {
        for (Schritt& x : mission1UndoSchritteAcademy()) { s.push_back(std::move(x)); }
    }
    if (mit("gizmo")) {
        for (Schritt& x : mission1GizmoSchritte("luke", "")) { s.push_back(std::move(x)); }
    }
    if (mit("laden")) {
        for (Schritt& x : mission1LadenSchritte(z)) { s.push_back(std::move(x)); }
    }
    if (mit("kino")) {
        for (Schritt& x : mission1KinoSchritte(z)) { s.push_back(std::move(x)); }
    }
    if (mit("drehen") || mit("undo")) {
        for (Schritt& x : mission1DrehSchritte(z)) { s.push_back(std::move(x)); }
    }
    if (mit("undo")) {
        for (Schritt& x : mission1UndoSchritteSchiff()) { s.push_back(std::move(x)); }
    }
    if (mit("mini")) {
        for (Schritt& x : mission1MiniSchritte(z)) { s.push_back(std::move(x)); }
    }
    return s;
}
