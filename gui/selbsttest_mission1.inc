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
std::vector<Schritt> mission1ZwischenSchritte() {
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
    add(tu("Zwischenpunkt: Rosh mitten in seinem Lauf suchen", [=] {
        *lp = LaufProbe{};
        for (std::size_t ai = 0; ai < g_app->scene.actors.size() && lp->figur < 0; ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            if (mission1::klein(a.name).find("rosh") == std::string::npos) { continue; }
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
    add(pruefe("Zwischenpunkt: eine laufende Rosh-Figur mit Laufziel gefunden", [=] {
        return lp->figur >= 0 && lp->entity >= 0 && !lp->endName.empty() && mission1::entityNamens(lp->endName) >= 0;
    }));
    add(tu("Zwischenpunkt: Kamera vor Rosh, sein Spawner gewaehlt, Verschieben", [=] {
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
    add(foto("m1_zwischen_vorher"));
    add(ziehenAn("Zwischenpunkt: an der X-Achse des Gizmos ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(1, q);
        return q;
    }, 60.0F, 0.0F, 0));
    add(pause(0.6));
    add(foto("m1_zwischen_nachher"));
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
        // alte, und sein Griff steht weiter auf der Bahn. Ob die Figur vom neuen
        // Punkt aus weiterkommt, entscheidet die Wegsuche (NAV-Port) - berichtet,
        // nicht verlangt.
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
        bool weiter = false;
        for (const ActorStep& st : a.steps) {
            weiter = weiter || (st.kind == ActorStep::Kind::WalkTo && st.startMs > lp->t - 100.0 &&
                                mission1::abstand3(st.laufZiel, endOrt) < 1.0F && st.endMs - st.startMs > 200.0);
        }
        const int griffe = laufGriffe();
        lp->skriptNach = writeScript(g_app->doc.script());
        lp->entsNach = mission1::entsText();
        diag::detail("Knopftest: Zwischenpunkt - neues Laufziel vor dem alten " + std::to_string(neuVorAlt) + ", Laufziel-Griffe " +
                     std::to_string(lp->griffe) + " -> " + std::to_string(griffe) + "; Vorschau: Lauf vom neuen Punkt zum alten Ziel " +
                     std::to_string(weiter) + ", Meldungen \"findet keinen Weg\" " + std::to_string(a.stehtOhneWeg.size()));
        return ng.size() == lp->navgoals + 1 && endeImSkript == 1 && ee >= 0 && mission1::abstand3(endOrt, lp->endOrt) < 0.01F &&
               mission1::abstand3(spawn, lp->spawn) < 0.01F && neuVorAlt == 1 && griffe == lp->griffe + 1;
    }));
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
std::vector<Schritt> mission1GizmoSchritte() {
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
        gp->figur = mission1::figurNamens("luke");
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
    add(foto("m1_gizmo_vorher"));
    add(klick(tr(Str::TlPlay), "###main"));
    add(pause(0.8));
    add(foto("m1_gizmo_spielt1"));
    add(pause(0.8));
    add(foto("m1_gizmo_spielt2"));
    add(klick(tr(Str::TlStop), "###main"));
    add(pause(0.6));
    add(foto("m1_gizmo_nachher"));
    add(pruefe("Abspielen wie im Spiel (Figur gewaehlt): kein Fadenkreuz, keine Auswahl, Griffe, Marken - nach Stop wieder da", [=] {
        const ImRect r{ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}};
        const char* namen[4] = {"m1_gizmo_vorher", "m1_gizmo_spielt1", "m1_gizmo_spielt2", "m1_gizmo_nachher"};
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
        const char* namen[4] = {"m1_gizmo_vorher", "m1_gizmo_spielt1", "m1_gizmo_spielt2", "m1_gizmo_nachher"};
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
    add(foto("m1_kgizmo_vorher"));
    add(klick(tr(Str::TlPlay), "###main"));
    add(pause(0.8));
    add(foto("m1_kgizmo_spielt1"));
    add(pause(0.8));
    add(foto("m1_kgizmo_spielt2"));
    add(klick(tr(Str::TlStop), "###main"));
    add(pause(0.6));
    add(foto("m1_kgizmo_nachher"));
    add(pruefe("Abspielen wie im Spiel (Kamera gewaehlt): kein Fadenkreuz, keine Auswahl, Griffe, Marken - nach Stop wieder da", [=] {
        const ImRect r{ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]}, ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}};
        const char* namen[4] = {"m1_kgizmo_vorher", "m1_kgizmo_spielt1", "m1_kgizmo_spielt2", "m1_kgizmo_nachher"};
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
        const char* namen[4] = {"m1_kgizmo_vorher", "m1_kgizmo_spielt1", "m1_kgizmo_spielt2", "m1_kgizmo_nachher"};
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

struct UndoProbe {
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
    add(tu("Undo " + was + ": Stand vorher", [=] { p->a = mission1::undoStand(*z); }));
    add(pause(0.2));
    add(ziehenAn("Undo " + was, [=] { return mission1::undoGriff(*z, drehen); }, drehen ? 50.0F : 60.0F, drehen ? -25.0F : -10.0F, 0));
    add(pause(0.5));
    add(tu("Undo " + was + ": Stand nach der Aenderung, Maus ueber die Karte", [=] {
        p->b = mission1::undoStand(*z);
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
    }
    return z;
}

UndoZiel kameraZiel(int wann) {
    UndoZiel z;
    z.art = ZielArt::Kamera;
    z.name = "Kamera";
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
    // BHED_M1_TEIL: nur einzelne Teile (md, jka, zwischen, gizmo, drehen, undo -
    // durch Leerzeichen getrennt); ohne: alles. zwischen/gizmo brauchen jka.
    const std::string teile = std::getenv("BHED_M1_TEIL") != nullptr ? std::string(" ") + std::getenv("BHED_M1_TEIL") + " " : std::string();
    const auto mit = [&teile](const char* t) { return teile.empty() || teile.find(std::string(" ") + t + " ") != std::string::npos; };
    if (mit("md")) {
        for (Schritt& x : mission1GesichtSchritte(z, true)) { s.push_back(std::move(x)); }
    }
    if (mit("jka") || mit("zwischen") || mit("gizmo") || mit("undo")) {
        for (Schritt& x : mission1GesichtSchritte(z, false)) { s.push_back(std::move(x)); }
    }
    if (mit("zwischen")) {
        for (Schritt& x : mission1ZwischenSchritte()) { s.push_back(std::move(x)); }
    }
    if (mit("undo")) {
        for (Schritt& x : mission1UndoSchritteAcademy()) { s.push_back(std::move(x)); }
    }
    if (mit("gizmo")) {
        for (Schritt& x : mission1GizmoSchritte()) { s.push_back(std::move(x)); }
    }
    if (mit("drehen") || mit("undo")) {
        for (Schritt& x : mission1DrehSchritte(z)) { s.push_back(std::move(x)); }
    }
    if (mit("undo")) {
        for (Schritt& x : mission1UndoSchritteSchiff()) { s.push_back(std::move(x)); }
    }
    return s;
}
