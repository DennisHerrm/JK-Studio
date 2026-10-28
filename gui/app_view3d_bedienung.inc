// app_view3d_bedienung.inc - aus gui/app_view3d.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): Bedienung der Vorschau - Entity-Gruppen, Platzieren, Ansichten, Zeitleiste, Spielfenster
//
// Eingebunden an genau dieser Stelle von gui/app_view3d.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: Gruppen der Entity-Liste und der Marken in der Ansicht; Platzieren-Leiste (rc590); Gemerkte Ansichten (rc590): Strg+1..9 merkt, 1..9 springt hin; Der bearbeitete Schluessel geht mit der Zeitleiste; Das Spielfenster in der Ansicht (Paket kamera, rc601)

// --- Gruppen der Entity-Liste und der Marken in der Ansicht ---------------
namespace {
const EntityKat kKategorien[] = {
    {Str::EntityKatTags, 80, 200, 255, UiIcon::Marke},
    {Str::EntityKatNav, 90, 220, 120, UiIcon::Wegpunkt},
    {Str::EntityKatLook, 255, 160, 60, UiIcon::Zielen},
    {Str::EntityKatNpc, 200, 200, 90, UiIcon::Figur},
    {Str::EntityKatRunner, 220, 90, 220, UiIcon::Skript},
    {Str::EntityKatFx, 255, 110, 80, UiIcon::Effekt},
    {Str::EntityKatSound, 150, 170, 255, UiIcon::Ton},
    {Str::EntityKatMover, 150, 165, 190, UiIcon::Tuer},
    {Str::EntityKatBreak, 190, 130, 80, UiIcon::Bruch},
    {Str::EntityKatWaypoints, 110, 220, 240, UiIcon::Pfad},
};
}  // namespace

int entityKategorien() { return static_cast<int>(sizeof(kKategorien) / sizeof(kKategorien[0])); }

const EntityKat& entityKategorieInfo(int k) {
    return kKategorien[static_cast<std::size_t>(std::clamp(k, 0, entityKategorien() - 1))];
}

int entityKategorie(const MapEntity& e) {
    const std::string& c = e.classname;
    if (c == "ref_tag") { return 0; }
    if (c.rfind("waypoint_navgoal", 0) == 0) { return 1; }
    if (c == "target_position" || c == "info_notnull") { return 2; }
    if (c.rfind("NPC_", 0) == 0) { return 3; }
    if (c == "target_scriptrunner") { return 4; }
    if (c == "fx_runner") { return 5; }
    if (c == "target_speaker" || c == "target_play_music") { return 6; }
    if (c == "func_breakable" || c == "misc_model_breakable") { return 8; }
    if (c.rfind("func_", 0) == 0 && (!e.targetname.empty() || e.find("script_targetname") != nullptr)) { return 7; }
    if (c == "waypoint" || c == "waypoint_small" || c == "point_combat") { return 9; }
    return -1;
}

UiIcon entityIcon(const MapEntity& e) {
    const int kat = entityKategorie(e);
    if (kat >= 0) { return entityKategorieInfo(kat).icon; }
    const std::string& c = e.classname;
    if (c.rfind("trigger_", 0) == 0) { return UiIcon::Blitz; }
    if (c.rfind("info_player", 0) == 0) { return UiIcon::Mission; }
    if (c == "target_level_change") { return UiIcon::Karte; }
    if (c == "light" || c.rfind("light_", 0) == 0) { return UiIcon::Licht; }
    if (c == "waypoint" || c.rfind("waypoint_", 0) == 0) { return UiIcon::Wegpunkt; }
    if (c.rfind("func_", 0) == 0) { return UiIcon::Tuer; }
    if (c.rfind("target_", 0) == 0) { return UiIcon::Blitz; }
    return UiIcon::Entity;
}

// Unter welchem Namen ein Skript die Entity anspricht: eine Figur ueber
// NPC_targetname, sonst script_targetname, sonst targetname.
std::string entitySkriptName(const MapEntity& e) {
    // NPC_targetname gilt nur fuer NPC-Spawner (NPC_Spawn_Do gibt ihn der
    // gespawnten Figur). Ein misc_model_breakable mit npc_targetname (das
    // X-Wing in t2_wedge) heisst fuer ICARUS weiter script_targetname.
    if (e.classname.rfind("NPC_", 0) == 0 || e.classname.rfind("npc_", 0) == 0) {
        if (const std::string* n = e.find("NPC_targetname"); n != nullptr && !n->empty()) { return *n; }
    }
    if (const std::string* n = e.find("script_targetname"); n != nullptr && !n->empty()) { return *n; }
    return e.targetname;
}

// Wo steht die Entity? origin, sonst die Mitte ihres Brush-Modells.
bool entityOrt(const MapEntity& e, float out[3]) {
    if (!e.origin.empty()) {
        // Nur drei lesbare Zahlen gelten. Ein von Hand verdorbenes "x y"
        // lieferte vorher true mit Zufallswerten (Missionsbau-Test 11.10.).
        float p[3] = {0.0F, 0.0F, 0.0F};
        std::istringstream is(e.origin);
        if (!(is >> p[0] >> p[1] >> p[2]) || !std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) {
            return false;
        }
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        return true;
    }
    return false;
}

void entityVersatz(int ei, float d[3]) {
    d[0] = d[1] = d[2] = 0.0F;
    const auto it = g_app->entVersatz.find(ei);
    if (it != g_app->entVersatz.end()) {
        for (int k = 0; k < 3; ++k) { d[k] = it->second[static_cast<std::size_t>(k)]; }
    }
}

bool entityOrtAnzeige(const MapEntity& e, float out[3]) {
    if (!entityOrt(e, out)) { return false; }
    const auto& alle = g_app->map.entities;
    if (!alle.empty() && &e >= alle.data() && &e < alle.data() + alle.size()) {
        float d[3];
        entityVersatz(static_cast<int>(&e - alle.data()), d);
        for (int k = 0; k < 3; ++k) { out[k] += d[k]; }
    }
    return true;
}

float entityGierVersatz(int ei) {
    const auto it = g_app->entGierVersatz.find(ei);
    return it != g_app->entGierVersatz.end() ? it->second : 0.0F;
}

float entityGierAnzeige(const MapEntity& e) {
    const std::string* a = e.find("angle");
    float w = a != nullptr ? std::strtof(a->c_str(), nullptr) : 0.0F;
    const auto& alle = g_app->map.entities;
    if (!alle.empty() && &e >= alle.data() && &e < alle.data() + alle.size()) {
        w += entityGierVersatz(static_cast<int>(&e - alle.data()));
    }
    return w;
}

int figurFuerEntity(int ei) {
    if (ei < 0 || static_cast<std::size_t>(ei) >= g_app->map.entities.size()) { return -1; }
    const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(ei)];
    if (const std::string* mdl = e.find("model"); mdl != nullptr && mdl->size() > 1 && (*mdl)[0] == '*') {
        const int nr = std::atoi(mdl->c_str() + 1);
        for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
            if (nr > 0 && g_app->scene.actors[ai].brushModel == nr) { return static_cast<int>(ai); }
        }
        return -1;
    }
    for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
        if (g_app->scene.actors[ai].brushModel <= 0 && entityFuerFigur(static_cast<int>(ai)) == ei) {
            return static_cast<int>(ai);
        }
    }
    return -1;
}

int entityFuerFigur(int figur) {
    if (figur < 0 || static_cast<std::size_t>(figur) >= g_app->scene.actors.size()) { return -1; }
    auto klein = [](std::string s) {
        for (char& ch : s) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
        return s;
    };
    // rc602: der Spawner, aus dem die Szene die Figur gebaut hat - auch fuer
    // eine Kartenfigur ohne Namen ("NPC_Droid_R2D2#245").
    const int ke = g_app->scene.actors[static_cast<std::size_t>(figur)].kartenEntity;
    if (ke >= 0 && static_cast<std::size_t>(ke) < g_app->map.entities.size()) { return ke; }
    const std::string name = klein(g_app->scene.actors[static_cast<std::size_t>(figur)].name);
    if (name.empty()) { return -1; }
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const bool npc = e.classname.rfind("NPC_", 0) == 0 || e.classname.rfind("npc_", 0) == 0;
        if (npc && klein(entitySkriptName(e)) == name) { return static_cast<int>(i); }
    }
    return -1;
}

// Die freie Kamera so stellen, dass die Entity vor einem liegt.
void zurEntityFliegen(std::size_t i) {
    if (i >= g_app->map.entities.size()) {
        return;
    }
    float p[3];
    if (!entityOrt(g_app->map.entities[i], p)) {
        return;
    }
    const float gier = g_app->cam.angles[1] * 3.14159265F / 180.0F;
    g_app->cam.pos[0] = p[0] - std::cos(gier) * 180.0F;
    g_app->cam.pos[1] = p[1] - std::sin(gier) * 180.0F;
    g_app->cam.pos[2] = p[2] + 70.0F;
    aimAngles(g_app->cam.pos, p, g_app->cam.angles);
    g_app->throughCamera = false;
    g_app->mapDirty = true;
}

namespace {
Node befehl(const char* name, std::initializer_list<std::pair<Arg::Kind, std::string>> args,
            const char* erstesTypeset = "") {
    Node n;
    n.kind = Node::Kind::Command;
    n.name = name;
    bool erstes = true;
    for (const auto& [k, t] : args) {
        Arg a;
        a.kind = k;
        a.text = t;
        if (erstes) {
            a.typeset = erstesTypeset;
            erstes = false;
        }
        n.args.push_back(a);
    }
    return n;
}

void einfuegen(std::vector<Node> ns) {
    Path at;
    if (!g_app->doc.insertAfterAll(g_app->selectedPath, std::move(ns), &at)) {
        return;
    }
    g_app->selectedPath = at;
    rebuildTree();
    g_app->camTrackValid = false;
    g_app->mapDirty = true;
}
}  // namespace

// Das Rechtsklickmenue einer Entity.
void entityMenue(std::size_t i) {
    const MapEntity& e = g_app->map.entities[i];
    const std::string name = entitySkriptName(e);
    const int kat = entityKategorie(e);
    if (ImGui::MenuItem(tr(Str::EntityFlyTo))) {
        zurEntityFliegen(i);
    }
    if (!name.empty() && ImGui::MenuItem(tr(Str::EntityCopyName))) {
        ImGui::SetClipboardText(name.c_str());
    }
    if (name.empty()) {
        return;
    }
    ImGui::Separator();
    const std::string q = "\"" + name + "\"";
    if (kat == 0) {
        // Genau so stehen die Kameras in den Missionen: auf einer Marke.
        if (ImGui::MenuItem(tr(Str::EntityCamToTag))) {
            einfuegen({befehl("camera", {{Arg::Kind::Ident, "MOVE"},
                                         {Arg::Kind::Expr, "tag( " + q + ", ORIGIN )"},
                                         {Arg::Kind::Number, "0.000"}}, "CAMERA_COMMANDS"),
                       befehl("camera", {{Arg::Kind::Ident, "PAN"},
                                         {Arg::Kind::Expr, "tag( " + q + ", ANGLES )"},
                                         {Arg::Kind::Vector, "0.000 0.000 0.000"},
                                         {Arg::Kind::Number, "0.000"}}, "CAMERA_COMMANDS")});
        }
    }
    if (kat == 0 || kat == 1) {
        if (ImGui::MenuItem(tr(Str::EntityAsNavgoal))) {
            einfuegen({befehl("set", {{Arg::Kind::String, "SET_NAVGOAL"}, {Arg::Kind::String, name}}, "SET_TYPES")});
        }
    }
    if (kat == 0 || kat == 1 || kat == 2 || kat == 3) {
        if (ImGui::MenuItem(tr(Str::EntityAsLook))) {
            einfuegen({befehl("set", {{Arg::Kind::String, "SET_LOOK_TARGET"}, {Arg::Kind::String, name}}, "SET_TYPES")});
        }
        if (ImGui::MenuItem(tr(Str::EntityAsWatch))) {
            einfuegen({befehl("set", {{Arg::Kind::String, "SET_WATCHTARGET"}, {Arg::Kind::String, name}}, "SET_TYPES")});
        }
    }
    if (kat == 3 || kat == 7 || kat == 8) {
        if (ImGui::MenuItem(tr(Str::EntityAffect))) {
            Node a = befehl("affect", {{Arg::Kind::String, name}, {Arg::Kind::Ident, "FLUSH"}});
            a.args[1].typeset = "AFFECT_TYPE";
            a.hasBlock = true;
            einfuegen({a});
        }
    }
    if (!e.targetname.empty() && ImGui::MenuItem(tr(Str::EntityUse))) {
        einfuegen({befehl("use", {{Arg::Kind::String, e.targetname}})});
    }
}

// Was zur angeklickten Entity gehoert.
//
// Hier geht es ueber ein reines Anzeigewerkzeug hinaus: SomaZ' Editor zeigt
// die Entity, wir zeigen zusaetzlich, WO DAS SKRIPT SIE BENUTZT. Genau
// dafuer sitzt die Karte in einem Skripteditor - ein Wegpunkt allein sagt
// wenig, die Zeile, die ihn anspricht, sagt alles.
void drawPickedEntity() {
    // Am Ende der Transportleiste (rc588), nur wenn eine Entity gewaehlt ist -
    // der Hinweis "anklicken" steht im Inspektor.
    if (g_app->pickedEntity < 0 ||
        static_cast<std::size_t>(g_app->pickedEntity) >=
            g_app->map.entities.size()) {
        return;
    }
    const MapEntity& e =
        g_app->map.entities[static_cast<std::size_t>(g_app->pickedEntity)];
    const std::string name = e.targetname.empty() ? e.classname : e.targetname;
    ImGui::SameLine(0.0F, ImGui::GetFontSize() * 1.2F);
    {
        const float g = ImGui::GetFontSize();
        const ImVec2 q = ImGui::GetCursorScreenPos();
        const float y = q.y + (ImGui::GetFrameHeight() - g) * 0.5F;
        uiIcon(ImGui::GetWindowDrawList(), UiIcon::Entity, ImVec2{q.x, std::floor(y)}, g,
               ImGui::GetColorU32(uiIconFarbe(UiIcon::Entity)));
        ImGui::Dummy(ImVec2{g, ImGui::GetFrameHeight()});
        ImGui::SameLine(0.0F, ImGui::GetStyle().ItemInnerSpacing.x);
    }
    ImGui::AlignTextToFramePadding();
    ImGui::Text(tr(Str::PickedEntity), name.c_str(), e.classname.c_str());

    if (e.targetname.empty()) {
        return;
    }

    // Die Zeilen suchen, die diesen Namen als Argument tragen. Ein Name kann
    // in mehreren Befehlen vorkommen - SET_NAVGOAL, SET_LOOK_TARGET, use.
    std::vector<Path> hits;
    for (std::size_t i = 0; i < g_app->rows.size(); ++i) {
        const Node* n = nodeAt(g_app->doc.script(), g_app->rows[i].path);
        if (n == nullptr) {
            continue;
        }
        bool match = false;
        for (const Arg& a : n->args) {
            if (a.text == e.targetname) {
                match = true;
            }
        }
        if (match) {
            hits.push_back(g_app->rows[i].path);
        }
    }

    ImGui::SameLine();
    if (hits.empty()) {
        ImGui::TextDisabled("%s", tr(Str::PickedUnused));
        return;
    }
    ImGui::TextDisabled(tr(Str::PickedUses), static_cast<int>(hits.size()));

    // Anklickbar: zur Zeile im Baum springen.
    for (std::size_t k = 0; k < hits.size() && k < 6; ++k) {
        ImGui::SameLine();
        ImGui::PushID(static_cast<int>(k));
        char label[24];
        std::snprintf(label, sizeof(label), "%d", static_cast<int>(k) + 1);
        if (ImGui::SmallButton(label)) {
            g_app->selectedPath = hits[k];
            g_app->selection.assign(1, hits[k]);
            g_app->scrollToSelected = true;
            for (std::size_t i = 0; i < g_app->rows.size(); ++i) {
                if (g_app->rows[i].path == hits[k]) {
                    g_app->selected = static_cast<int>(i);
                    break;
                }
            }
        }
        ImGui::PopID();
    }
}

// --- Platzieren-Leiste (rc590) ---------------------------------------------------
//
// Wie "Place Actors" in Unreal: Kacheln mit den Entities, die man fuer eine
// Szene braucht. Ein Klick waehlt eine Kachel, der naechste Klick in die Karte
// setzt sie (Umschalt: weitere); oder die Kachel in die Karte ziehen.
namespace {
struct Kachel {
    const char* klasse;
    UiIcon icon;
    Str titel;
};
const Kachel kKacheln[] = {
    {"NPC_spawner", UiIcon::Figur, Str::EntKindNpc},
    {"waypoint_navgoal", UiIcon::Wegpunkt, Str::EntKindNav},
    {"ref_tag", UiIcon::Marke, Str::EntKindTag},
    {"target_position", UiIcon::Zielen, Str::EntKindPos},
    {"target_scriptrunner", UiIcon::Skript, Str::EntKindRunner},
    {"fx_runner", UiIcon::Effekt, Str::EntKindFx},
    {"target_speaker", UiIcon::Ton, Str::EntKindSpeaker},
    {"waypoint", UiIcon::Pfad, Str::EntKindWaypoint},
    {"info_player_start", UiIcon::Mission, Str::EntKindStart},
};
}  // namespace

void platzierLeiste() {
    if (g_app->geo.empty() || g_app->camViewActive || g_app->kameraPilot) { return; }
    const float fs = ImGui::GetFontSize();
    const float hh = ImGui::GetFrameHeight();
    const int n = static_cast<int>(sizeof(kKacheln) / sizeof(kKacheln[0]));
    const float hoehe = static_cast<float>(n) * (hh + 2.0F) + fs * 0.6F;
    const float y = std::max(g_app->kartenAnsicht[1] + fs * 3.0F,
                             (g_app->kartenAnsicht[1] + g_app->kartenAnsicht[3] - hoehe) * 0.5F);
    const ImVec2 merk = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2{g_app->kartenAnsicht[0] + fs * 0.4F, y});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.10F, 0.11F, 0.14F, 0.85F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{fs * 0.2F, fs * 0.3F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0F, 2.0F});
    if (ImGui::BeginChild("##platzieren", ImVec2{0.0F, 0.0F},
                          ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders |
                              ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar)) {
        for (const Kachel& k : kKacheln) {
            const bool an = g_app->platzierKlasse == k.klasse;
            char hilfe[300];
            std::snprintf(hilfe, sizeof(hilfe), tr(Str::PlaceHint), k.klasse);
            if (iconKnopf(tr(k.titel), k.icon, hilfe, an, true, true)) {
                g_app->platzierKlasse = an ? std::string() : std::string(k.klasse);
                g_app->platzierZiehen = false;
            }
            // In die Karte ziehen
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 8.0F)) {
                g_app->platzierKlasse = k.klasse;
                g_app->platzierZiehen = true;
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(merk);
    ImGui::Dummy(ImVec2{0.0F, 0.0F});   // Anker: ImGui will nach dem Cursorsprung ein Element
}

void platzierMaus() {
    if (g_app->platzierKlasse.empty() || g_app->animZug) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    const bool imBild = io.MousePos.x >= g_app->kartenAnsicht[0] && io.MousePos.x < g_app->kartenAnsicht[2] &&
                        io.MousePos.y >= g_app->kartenAnsicht[1] && io.MousePos.y < g_app->kartenAnsicht[3];
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        g_app->platzierKlasse.clear();
        g_app->platzierZiehen = false;
        return;
    }
    // Vorschau: ein Kreuz dort, wo die Entity landen wuerde.
    if (imBild) {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const float r = ImGui::GetFontSize() * 0.6F;
        dl->AddCircle(io.MousePos, r, IM_COL32(255, 200, 80, 230), 0, 2.0F);
        dl->AddLine(ImVec2{io.MousePos.x - r * 1.6F, io.MousePos.y}, ImVec2{io.MousePos.x + r * 1.6F, io.MousePos.y},
                    IM_COL32(255, 200, 80, 200));
        dl->AddLine(ImVec2{io.MousePos.x, io.MousePos.y - r * 1.6F}, ImVec2{io.MousePos.x, io.MousePos.y + r * 1.6F},
                    IM_COL32(255, 200, 80, 200));
        ImGui::SetTooltip(tr(Str::PlaceClickHint), g_app->platzierKlasse.c_str());
    }
    if (g_app->platzierZiehen) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (imBild && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                entityPlatzieren(g_app->platzierKlasse, io.MousePos.x, io.MousePos.y);
                g_app->wegKlickVerbraucht = true;
            }
            g_app->platzierKlasse.clear();
            g_app->platzierZiehen = false;
        }
        return;
    }
    if (imBild && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        entityPlatzieren(g_app->platzierKlasse, io.MousePos.x, io.MousePos.y);
        g_app->wegKlickVerbraucht = true;   // dieser Klick waehlt nichts anderes aus
        if (!io.KeyShift) { g_app->platzierKlasse.clear(); }
    }
}

// --- Gemerkte Ansichten (rc590): Strg+1..9 merkt, 1..9 springt hin ----------------
void ansichtsMarkenTasten() {
    if (ImGui::GetIO().WantTextInput || g_app->geo.empty() || g_app->throughCamera) { return; }
    auto& marken = g_app->ansichtsMarken[g_app->map.path];
    for (int k = 0; k < 9; ++k) {
        const ImGuiKey taste = static_cast<ImGuiKey>(ImGuiKey_1 + k);
        if (!ImGui::IsKeyPressed(taste, false)) { continue; }
        char t[96];
        if (ImGui::GetIO().KeyCtrl) {
            marken[static_cast<std::size_t>(k)].da = true;
            // Paket kamera, rc601 (Review): im Pilot ohne dessen engen
            // Bildwinkel - sonst blieb die freie Kamera nach dem Sprung eng.
            marken[static_cast<std::size_t>(k)].cam = freieKamera();
            std::snprintf(t, sizeof(t), tr(Str::ViewMarkSaved), k + 1, k + 1);
            meldung(t, "");
        } else if (marken[static_cast<std::size_t>(k)].da) {
            g_app->cam = marken[static_cast<std::size_t>(k)].cam;
            g_app->mapDirty = true;
            std::snprintf(t, sizeof(t), tr(Str::ViewMarkJump), k + 1);
            addStatus(t);
        }
    }
}

// Die Kopfzeile der Kartenansicht (rc586).
//
// Links die Mission: Neu, Laden, Speichern als Icons, daneben ihr Name und ein
// Punkt, solange etwas ungespeichert ist - wie der Stern im Reiter. Dann
// "Karten und Dateien" (Karte oeffnen, aus einer .pk3, .ent) als Klappmenue
// und die Skripte der Karte. Rechts die Ansicht (ansichtsSchalter).
//
// Vorher standen sieben gleich grosse Textknoepfe in einer Reihe - shank:
// "moderner und intuitiv". Unreal und Hammer zeigen oben nur das Wichtigste.
namespace {
// Zwei Kartenkennungen gleich? Gross/klein und \ gegen / egal.
bool gleicheKarte(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        char x = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
        char y = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
        if (x == '\\') { x = '/'; }
        if (y == '\\') { y = '/'; }
        if (x != y) { return false; }
    }
    return true;
}
// Liegt die Karte der Mission vorn? Die Mission gilt ueber alle Reiter
// (Speichern nimmt ihre Karte auch aus einem anderen), aber Name und Punkt
// fuer Ungespeichertes gehoeren nur zu ihrer Karte.
bool missionKarteVorn() {
    return g_app->missionAktiv && !g_app->mission.karte.empty() &&
           gleicheKarte(g_app->map.path, g_app->mission.karte);
}
}  // namespace

std::string leistenTitel() {
    if (missionKarteVorn()) {
        return g_app->mission.name;
    }
    // Sonst der Name der Karte vorn: "maps/academy1.bsp" -> "academy1".
    std::string n = g_app->map.path;
    const std::size_t schnitt = n.find_last_of("/\\");
    if (schnitt != std::string::npos) { n = n.substr(schnitt + 1); }
    const std::size_t punkt = n.find_last_of('.');
    if (punkt != std::string::npos) { n.resize(punkt); }
    return n;
}

void drawMapToolbar() {
    // Senkrecht MITTIG im Streifen: der Streifen ist eine Knopfzeile plus
    // Zeilenabstand hoch, und ohne das sass alles oben (shank, 11.10.).
    {
        const float rest = ImGui::GetWindowHeight() - ImGui::GetFrameHeight();
        if (rest > 1.0F) { ImGui::SetCursorPosY(std::floor(rest * 0.5F)); }
    }
    const float sp = ImGui::GetStyle().ItemSpacing.x;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{sp * 0.5F, ImGui::GetStyle().ItemSpacing.y});
    if (iconKnopf(tr(Str::MissionNew), UiIcon::Neu, tr(Str::MissionNewHint))) {
        g_app->neueMissionOffen = true;
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::LoadMission), UiIcon::Oeffnen, tr(Str::LoadMissionHint))) {
        loadMissionArchive();
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::MissionSave), UiIcon::Speichern,
                  tr(g_app->missionAktiv ? Str::MissionSaveHint : Str::MissionKeine), false, g_app->missionAktiv)) {
        g_app->missionSpeichernOffen = true;
    }
    ImGui::PopStyleVar();

    // Der Name der Mission - mit Punkt fuer Ungespeichertes.
    ImGui::SameLine(0.0F, sp);
    ImGui::AlignTextToFramePadding();
    if (missionKarteVorn()) {
        const bool offen = missionUngespeichert();
        uiIcon(ImGui::GetWindowDrawList(), UiIcon::Mission,
               ImVec2{ImGui::GetCursorScreenPos().x,
                      ImGui::GetCursorScreenPos().y + (ImGui::GetFrameHeight() - ImGui::GetFontSize()) * 0.5F},
               ImGui::GetFontSize(), ImGui::GetColorU32(uiIconFarbe(UiIcon::Mission)));
        ImGui::Dummy(ImVec2{ImGui::GetFontSize(), ImGui::GetFontSize()});
        ImGui::SameLine(0.0F, sp * 0.5F);
        ImGui::TextUnformatted(g_app->mission.name.c_str());
        if (ImGui::IsItemHovered()) {
            char t[900];
            std::snprintf(t, sizeof(t), tr(Str::MissionProject), g_app->mission.projektOrdner.c_str());
            ImGui::SetTooltip("%s%s%s", t, offen ? "\n" : "", offen ? tr(Str::MissionUnsavedHint) : "");
        }
        if (offen) {
            ImGui::SameLine(0.0F, sp * 0.6F);
            const ImVec2 q = ImGui::GetCursorScreenPos();
            const float r = ImGui::GetFontSize() * 0.22F;
            ImGui::GetWindowDrawList()->AddCircleFilled(
                ImVec2{q.x + r, q.y + ImGui::GetFrameHeight() * 0.5F}, r, ImGui::GetColorU32(ImGuiCol_PlotHistogram));
            ImGui::Dummy(ImVec2{r * 2.0F, ImGui::GetFrameHeight()});
        }
    } else if (g_app->missionAktiv && !g_app->map.path.empty()) {
        // Vorn liegt eine andere Karte als die der Mission (rc592: hier stand
        // weiter der Name der Mission).
        const std::string titel = leistenTitel();
        uiIcon(ImGui::GetWindowDrawList(), UiIcon::Karte,
               ImVec2{ImGui::GetCursorScreenPos().x,
                      ImGui::GetCursorScreenPos().y + (ImGui::GetFrameHeight() - ImGui::GetFontSize()) * 0.5F},
               ImGui::GetFontSize(), ImGui::GetColorU32(uiIconFarbe(UiIcon::Karte)));
        ImGui::Dummy(ImVec2{ImGui::GetFontSize(), ImGui::GetFontSize()});
        ImGui::SameLine(0.0F, sp * 0.5F);
        ImGui::TextUnformatted(titel.c_str());
        if (ImGui::IsItemHovered()) {
            char t[900];
            std::snprintf(t, sizeof(t), tr(Str::MapNotMission), g_app->map.path.c_str(), g_app->mission.name.c_str());
            ImGui::SetTooltip("%s", t);
        }
    } else {
        ImGui::TextDisabled("%s", tr(Str::MissionNoneShort));
    }

    // Karten und Dateien
    ImGui::SameLine(0.0F, sp * 1.5F);
    if (iconTextKnopf(tr(Str::MapMore), UiIcon::Karte, tr(Str::MapMoreHint)) || g_app->kartenMehrOeffnen) {
        g_app->kartenMehrOeffnen = false;
        ImGui::OpenPopup("##kartenmehr");
    }
    if (ImGui::BeginPopup("##kartenmehr")) {
        if (ImGui::MenuItem(tr(Str::MissionOpenFolder))) { missionsOrdnerWaehlen(); }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MissionOpenFolderHint)); }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::OpenMapFile))) { doLoadMap(); }
        if (ImGui::MenuItem(tr(Str::OpenMapPk3))) {
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
            g_app->pk3Kind = 1;
            refreshPk3List();
            g_app->pk3Open = true;
        }
        // Die .ent-Datei ersetzt die Entities der Karte. Movie Duels arbeitet
        // so: die .bsp bleibt unveraendert, die Entities kommen daneben.
        if (ImGui::MenuItem(tr(Str::OpenEnt))) { openEntFile(); }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::OpenEntHint));
        }
        ImGui::EndPopup();
    }
    // Die Skripte der Karte als Klappliste - so findet man die Sequenz zur
    // Karte, ohne den Pfad zu kennen.
    if (!g_app->map.scripts.empty()) {
        ImGui::SameLine(0.0F, sp * 0.5F);
        char label[64];
        std::snprintf(label, sizeof(label), "%s (%d)", tr(Str::MapScripts), static_cast<int>(g_app->map.scripts.size()));
        ImGui::SetNextItemWidth(ImGui::CalcTextSize(label).x + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0F);
        if (ImGui::BeginCombo("##mapscripts", label, ImGuiComboFlags_HeightLarge)) {
            drawMapScriptList();
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::MapScriptsHint));
        }
    }

    // Rechts: die Ansicht. Ist kein Platz mehr, rueckt sie direkt hinter den Rest.
    const float rechts = ansichtsSchalter(true);
    const float x = ImGui::GetWindowContentRegionMax().x - rechts;
    ImGui::SameLine();
    // SameLine mit Abstand vom Zeilenanfang statt SetCursorPosX (lint_imgui_context).
    ImGui::SameLine(std::max(x, ImGui::GetCursorPosX()));
    ansichtsSchalter(false);
}

// Die Zeitleiste unter der Kartenansicht.
//
// Sie zeigt die Kamerabahn des Skripts: wann welcher Befehl laeuft und wo
// die Kamera dabei steht. Der Zeiger laesst sich ziehen, und beim Abspielen
// bewegt sich die Ansicht mit - das ist der Punkt, fuer den das Ganze
// gebaut wird: sieht die Fahrt gut aus, passt sie zur Laenge des Satzes.
// Der Griff an der rechten Kante eines Blocks.
//
// Gezogen wird die DAUER. Waehrend des Ziehens wandert nur die Vorschau;
// erst beim Loslassen geht es ueber replaceAt() ins Skript und damit in
// den Rueckgaengig-Speicher - dieselbe Regel wie beim Gizmo.
//
// Gibt true zurueck, wenn dieser Block gerade gezogen wird; dann zeichnet
// der Aufrufer seine rechte Kante an der Vorschauposition statt am Wert
// aus dem Skript.
bool kantenGriff(const Path& pfad, double startMs, double endMs,
                 const ImVec2& a, const ImVec2& b, double msJeBildpunkt) {
    if (pfad.empty()) {
        return false;
    }
    const Node* n = nodeAt(g_app->doc.script(), pfad);
    if (n == nullptr || durationArgIndex(*n) < 0) {
        return false;   // keine Dauer - also kein Griff
    }

    const bool ichZiehe = g_app->tlDragging && g_app->tlDragPath == pfad;
    const float greifW = std::max(4.0F, ImGui::GetFontSize() * 0.35F);
    const ImVec2 gA{b.x - greifW, a.y};
    const ImVec2 gB{b.x + greifW, b.y};
    const bool drauf = ImGui::IsMouseHoveringRect(gA, gB);

    if (drauf || ichZiehe) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (drauf && !g_app->tlDragging &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // Nur VORMERKEN - ein spaeter gezeichneter Block an derselben Stelle
        // liegt oben und gewinnt (siehe tlGriffKandidat).
        App::TlKandidat& k = g_app->tlGriffKandidat;
        k.da = true;
        k.pfad = pfad;
        k.startMs = startMs;
        k.endMs = endMs;
    }
    if (!ichZiehe) {
        return false;
    }

    // Neues Ende aus der Mausbewegung. Mindestens eine Millisekunde: eine
    // Dauer von null laesst die Engine den Befehl ueberspringen, und das
    // ist beim Ziehen nie gemeint.
    g_app->tlDragEndMs =
        std::max(g_app->tlDragStartMs + 1.0,
                 g_app->tlDragEndMs +
                     static_cast<double>(ImGui::GetIO().MouseDelta.x) *
                         msJeBildpunkt);

    ImGui::BeginTooltip();
    ImGui::Text(tr(Str::TlDragDur),
                (g_app->tlDragEndMs - g_app->tlDragStartMs) / 1000.0);
    ImGui::EndTooltip();

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        double dauer = g_app->tlDragEndMs - g_app->tlDragStartMs;
        // Ein wait ist im Spiel erst im Bild NACH seinem Ablauf fertig
        // (stempel + dauer < jetzt, TaskManager.cpp:1100): die Leiste zeigt
        // es also ein Spielbild (50 ms) laenger, als der Wert sagt. Beim
        // Zurueckschreiben dieses Bild wieder abziehen - sonst wuerde jedes
        // Ziehen den Wert um 50 ms verfaelschen.
        if (n->name == "wait") {
            dauer = std::max(1.0, dauer - 50.0);
        }
        diag::detail("Zeitleiste: Kante losgelassen, neue Dauer " + std::to_string(dauer) + " ms");
        Node neu = *n;
        const int idx = durationArgIndex(neu);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.3f", dauer);
        neu.args[static_cast<std::size_t>(idx)].text = buf;
        if (g_app->doc.replaceAt(pfad, neu)) {
            rebuildTree();
            g_app->camTrackValid = false;   // Zeiten haben sich geaendert
            g_app->mapDirty = true;
        }
        g_app->tlDragging = false;
        g_app->tlDragPath.clear();
    }
    return true;
}

// Einen Befehl in der Zeit verschieben (siehe "Einen Block verschieben" in
// drawTimeline). Gibt zurueck, ob sich etwas geaendert hat.
bool verschiebeImAblauf(const Path& p, double delta, bool nurDieser) {
    if (p.empty() || std::fabs(delta) < 1.0) {
        return false;
    }
    const Script& s = g_app->doc.script();
    const std::vector<Node>* liste = &s.nodes;
    const Path eltern(p.begin(), p.end() - 1);
    if (!eltern.empty()) {
        const Node* e = nodeAt(s, eltern);
        if (e == nullptr) {
            return false;
        }
        liste = &e->children;
    }
    const std::size_t idx = p.back();
    if (idx >= liste->size()) {
        return false;
    }
    const auto zahlWait = [](const Node& n) {
        return n.kind == Node::Kind::Command && n.name == "wait" && !n.args.empty() &&
               n.args[0].kind == Arg::Kind::Number;
    };
    // Auch das blockiert, ist aber keine Zahl: dahinter wird eingefuegt.
    const auto grenze = [](const Node& n) {
        return n.kind == Node::Kind::Command &&
               (n.name == "wait" || n.name == "waitsignal" || n.name == "dowait");
    };
    std::size_t g = idx;
    while (g > 0 && !grenze((*liste)[g - 1])) {
        --g;
    }
    const bool davorZahl = g > 0 && zahlWait((*liste)[g - 1]);
    const double wert = davorZahl ? std::atof((*liste)[g - 1].args[0].text.c_str()) : 0.0;
    double d = delta;
    if (wert + d < 0.0) {
        d = -wert;   // weiter nach vorn geht es nicht: das wait ist aufgebraucht
    }
    if (std::fabs(d) < 1.0) {
        setStatus(tr(Str::TlMoveNone), 0, 0);
        return false;
    }
    // Das wait danach (fuer Alt).
    std::size_t n = idx + 1;
    while (n < liste->size() && !grenze((*liste)[n])) {
        ++n;
    }
    const bool danachZahl = n < liste->size() && zahlWait((*liste)[n]);
    const auto mitWert = [](Node x, double v) {
        char b[32];
        std::snprintf(b, sizeof(b), "%.3f", std::max(0.0, v));
        x.args[0].text = b;
        return x;
    };
    bool ok = false;
    if (davorZahl) {
        std::vector<std::pair<Path, Node>> ersatz;
        Path pw = eltern;
        pw.push_back(g - 1);
        ersatz.emplace_back(pw, mitWert((*liste)[g - 1], wert + d));
        if (nurDieser && danachZahl) {
            Path pn = eltern;
            pn.push_back(n);
            const double wn = std::atof((*liste)[n].args[0].text.c_str());
            ersatz.emplace_back(pn, mitWert((*liste)[n], wn - d));
        }
        ok = g_app->doc.replaceMany(ersatz);
    } else if (d > 0.0) {
        // Kein wait davor: eines einfuegen.
        Node w;
        w.kind = Node::Kind::Command;
        w.name = "wait";
        Arg a;
        a.kind = Arg::Kind::Number;
        w.args.push_back(a);
        Path pg = eltern;
        pg.push_back(g);
        ok = g_app->doc.insertBefore(pg, mitWert(w, d));
        if (ok && nurDieser && danachZahl) {
            Path pn = eltern;
            pn.push_back(n + 1);   // eins weiter: davor kam eine Zeile dazu
            const Node* nn = nodeAt(g_app->doc.script(), pn);
            if (nn != nullptr) {
                const double wn = std::atof(nn->args[0].text.c_str());
                (void)g_app->doc.replaceAt(pn, mitWert(*nn, wn - d));
            }
        }
    }
    if (ok) {
        diag::detail("Zeitleiste: Befehl um " + std::to_string(static_cast<int>(d)) + " ms verschoben" +
                     (nurDieser ? " (nur dieser)" : " (alles danach mit)"));
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }
    return ok;
}

// Passiert in dieser Spur nach dem Anfang noch etwas? "Ruhig" heisst: alle
// Befehle liegen bei 0 und haben keine Dauer (etwa ein einzelnes set zu
// Beginn). Solche Spuren blendet die Zeitleiste auf Wunsch aus.
bool spurIstRuhig(const TimelineTrack& spur) {
    for (const TimelineEvent& e : spur.events) {
        if (e.endMs > 1.0 || e.startMs > 1.0) {
            return false;
        }
    }
    return true;
}

void klaengeVorladen() {
    if (!g_app->playAudio) {
        return;
    }
    diag::Step step("Klaenge vorladen");
    int anzahl = 0;
    for (const TimelineTrack& track : g_app->timeline.tracks) {
        for (const TimelineEvent& e : track.events) {
            std::string datei;
            if (e.kind == TimelineEvent::Kind::Sound && !e.sound.empty()) {
                datei = e.sound;
            } else if (e.kind == TimelineEvent::Kind::Use) {
                bool istMusik = false;
                datei = useSoundFor(e.target, &istMusik);
            }
            if (!datei.empty() && klangFuerWiedergabe(datei) != nullptr) {
                ++anzahl;
            }
        }
    }
    tonVorladen();   // rc601 ton: Schwerter, Lautsprecher, Tueren, Zerbrechen
    diag::info(std::to_string(anzahl) + " Klaenge bereit");
}

void drawTimeline() {
    {
        // Fuer Strg+Rad (siehe tlUnterMaus): steht die Maus ueber der Leiste?
        const ImVec2 o = ImGui::GetCursorScreenPos();
        const ImVec2 v = ImGui::GetContentRegionAvail();
        g_app->tlUnterMaus = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                             ImGui::IsMouseHoveringRect(o, ImVec2{o.x + v.x, o.y + v.y}, false);
    }
    if (!g_app->camTrackValid) {
        // Ueber prepareScene(), damit es NUR EINE Stelle gibt, die die
        // Ansicht aufbaut. Vorher baute diese hier die Szene und leerte
        // die Modelle, und das Zeichnen lud sie kurz darauf nach - zwei
        // Haelften desselben Vorgangs an zwei Orten.
        //
        // Teuer ist nur das erste Mal: die Modelle liegen danach in
        // modelCache, die .npc-Dateien sind ueber npcMapRead abgehakt.
        // Nach einer Skriptaenderung kostet der Aufbau also fast nichts.
        prepareScene();
    }
    const CameraTrack& t = g_app->camTrack;
    // Ohne Kamerabefehle bleibt die Leiste stehen, nur ohne Wirkung - sonst
    // springt die Anordnung, sobald ein Skript geladen wird.
    const bool haveTrack = !t.segments.empty();
    ImGui::BeginDisabled(!haveTrack);
    if (!haveTrack) {
        // Leerzustand (rc588): statt eines grauen Reglers ohne Wirkung ein Satz,
        // was fehlt, und der Knopf, der es anlegt - wie die leeren Ansichten
        // in Unreal und Blender ("Add a camera to get started").
        ImGui::EndDisabled();
        const float fs0 = ImGui::GetFontSize();
        const float hoch = std::max(ImGui::GetContentRegionAvail().y, fs0 * 3.0F);
        const ImVec2 o = ImGui::GetCursorScreenPos();
        const float breit = ImGui::GetContentRegionAvail().x;
        ImDrawList* dl0 = ImGui::GetWindowDrawList();
        const float ig = std::floor(fs0 * 1.6F);
        const char* satz = tr(Str::TlNone);
        const ImVec2 ts = ImGui::CalcTextSize(satz);
        const float knopfB = ImGui::CalcTextSize(tr(Str::MapInsertCamera)).x + ImGui::GetFrameHeight() * 1.6F;
        const float zeileB = ig + fs0 * 0.6F + ts.x + fs0 * 1.2F + knopfB;
        const float x0 = o.x + std::max(0.0F, (breit - zeileB) * 0.5F);
        const float y0 = o.y + std::max(0.0F, (hoch - ImGui::GetFrameHeight()) * 0.5F);
        uiIcon(dl0, UiIcon::Kamera, ImVec2{std::floor(x0), std::floor(y0 + (ImGui::GetFrameHeight() - ig) * 0.5F)}, ig,
               ImGui::GetColorU32(uiIconFarbe(UiIcon::Kamera)));
        ImGui::SetCursorScreenPos(ImVec2{x0 + ig + fs0 * 0.6F, y0});
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", satz);
        ImGui::SameLine(0.0F, fs0 * 1.2F);
        if (iconTextKnopf(tr(Str::MapInsertCamera), UiIcon::Plus, tr(Str::CamNewShotHint),
                          !g_app->geo.empty() && !g_app->camViewActive)) {
            kameraNeueEinstellung();
        }
        return;
    }
    ImGui::EndDisabled();

    // Wieder einsteigen, wo die Zeitmarke steht.
    //
    // Gemeldet: "wenn ich die Musik pausiere und wieder laufen lasse, hoert
    // sie auf zu spielen."
    //
    // Zwei Luecken, beide an derselben Stelle:
    //
    //   1. Der Abspielknopf stieg GAR NICHT wieder ein. Er setzte nur
    //      audioUpTo, und getroffen wird nur, was NEU anfaengt - ein Stueck,
    //      das laengst begonnen hatte, blieb stumm.
    //   2. Der Tonschalter stieg zwar ein, kannte aber nur Kind::Sound.
    //      Musik ist ein Kind::Use (sie haengt an einer Entity der Karte,
    //      seit rc255), fiel also auch dort durch.
    //
    // Deshalb blieb die Musik weg, bis das naechste use kam - in
    // intro_jedi liegen zwischen mus1 bei 0 ms und mus2 bei 48600 ms
    // achtundvierzig Sekunden.
    //
    // Beide Faelle rufen jetzt DIESE Funktion. Zwei Wege, die dasselbe
    // tun sollen, laufen sonst auseinander.
    const auto wiedereinstieg = [&]() {
        if (!g_app->playAudio) {
            return;
        }
        // rc601 ton: JEDER noch laufende sound-Befehl steigt an seiner Stelle
        // wieder ein - am Ort, im eigenen Kanal, mit PVS und Pegel wie beim
        // Ausloesen (tonWiedereinstieg). Vorher nur der letzte, mit voller
        // Lautstaerke und ohne Raum: ein ferner Satz klang wie neben einem.
        // Musik und Stimme laufen weiter NEBENeinander (eigene Geraete).
        tonWiedereinstieg();
        // Musik: das letzte target_play_music vor dem Zeiger (aus der
        // aufgeloesten use-Kette des Nachbaus), sonst die Levelmusik ab 0.
        // Musik laeuft endlos (Schleife) - also kein "schon zu Ende".
        double musikAb = 0.0;
        std::string musikDatei = weltMusik();
        for (const Ablauf::Benutzung& b : g_app->ablauf.benutzt) {
            if (b.ms > g_app->playMs) {
                continue;
            }
            bool istMusik = false;
            const std::string datei = useSoundFor(b.name, &istMusik);
            if (istMusik && !datei.empty() && b.ms >= musikAb) {
                musikAb = b.ms;
                musikDatei = datei;
            }
        }
        if (!musikDatei.empty()) {
            spieleMusik(musikDatei, g_app->playMs - musikAb);
        }
    };

    // --- Die Transportleiste (rc588) -----------------------------------------
    //
    // shank (11.10.): "kann man die timeline unten auch modernisieren und
    // vorallem die leiste mit sound ueber der timeline ... auch wegen den icons
    // so das alles verstaendlicher ist" und "ich habe da noch haessliche
    // checkboxen ueber der timleine". Jetzt Icon-Schalter wie in der
    // Kartenleiste, in Gruppen: Abspielen und Zeit | Schleife, Tempo | Klang,
    // Mitfahren | Zoom | Spuren | neue Kameraeinstellung. Die Beschriftungen
    // bleiben als Kennung und Tooltip-Titel (der Selbsttest klickt sie).
    const float fsT = ImGui::GetFontSize();
    const float gruppe = fsT * 0.9F;
    const auto trenner = [&]() {
        ImGui::SameLine(0.0F, gruppe * 0.5F);
        const ImVec2 q = ImGui::GetCursorScreenPos();
        const float hh = ImGui::GetFrameHeight();
        ImGui::GetWindowDrawList()->AddLine(ImVec2{q.x, q.y + hh * 0.2F}, ImVec2{q.x, q.y + hh * 0.8F},
                                            ImGui::GetColorU32(ImGuiCol_Separator));
        ImGui::Dummy(ImVec2{1.0F, hh});
        ImGui::SameLine(0.0F, gruppe * 0.5F);
    };
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{fsT * 0.2F, ImGui::GetStyle().ItemSpacing.y});
    if (iconKnopf(tr(Str::TlRewind), UiIcon::ZumAnfang, tr(Str::TlRewindHint))) {
        g_app->playMs = 0.0;
        // -1, damit ein Klang bei 0 ms beim naechsten Abspielen kommt.
        g_app->audioUpTo = -1.0;
        g_app->audio.stopAll();
        g_app->laufendeSchleifen.clear();
        tonZuruecksetzen();   // rc601 ton: auch der Raumklang
        g_app->musicAudio.stopAll();
        g_app->mapDirty = true;
    }
    ImGui::SameLine();
    if (iconKnopf(g_app->playing ? tr(Str::TlStop) : tr(Str::TlPlay), g_app->playing ? UiIcon::Pause : UiIcon::Abspielen,
                  tr(Str::TlPlayHint), g_app->playing)) {
        g_app->playing = !g_app->playing;
        if (!g_app->playing) {
            g_app->audio.stopAll();
            g_app->laufendeSchleifen.clear();
            tonZuruecksetzen();   // rc601 ton: auch der Raumklang
            g_app->musicAudio.stopAll();
        } else {
            // Erst alle Klaenge laden, DANN die Uhr laufen lassen. Vorher
            // wurde jeder Klang erst geladen, wenn er dran war; das Bild
            // stand 130-215 ms, und die Zeit sprang danach um genau so viel
            // vor (Selbsttest: 1582 ms Skriptzeit in 955 ms).
            klaengeVorladen();
            g_app->uhrAussetzen = 2;   // dieses und das naechste Bild
        }
        // Am Ende angekommen? Dann von vorn.
        if (g_app->playing && g_app->playMs >= t.durationMs - 1.0) {
            g_app->playMs = 0.0;
        }
        // Ab hier zaehlen: sonst kaeme beim Fortsetzen alles nach, was
        // waehrend der Pause im Fenster lag.
        g_app->audioUpTo = g_app->playing ? g_app->playMs - 0.001 : g_app->playMs;
        if (g_app->playing) {
            wiedereinstieg();
        }
    }
    // Die Zeit, gross und lesbar; ein Klick schaltet Sekunden/Bilder um (wie
    // in 3ds Max). Vorher ein eigener Knopf "Seconds" und daneben die Zeit
    // in kleiner grauer Schrift.
    ImGui::SameLine(0.0F, gruppe * 0.6F);
    {
        char jetzt[32];
        char gesamt[32];
        if (g_app->timelineFrames) {
            const double fps = static_cast<double>(std::max(1, g_app->timelineFps));
            std::snprintf(jetzt, sizeof(jetzt), "%d", static_cast<int>(g_app->playMs / 1000.0 * fps));
            std::snprintf(gesamt, sizeof(gesamt), " / %d", static_cast<int>(t.durationMs / 1000.0 * fps));
        } else {
            const auto mmss = [](char* out, std::size_t n, double ms, const char* vor) {
                const int cs = static_cast<int>(std::max(0.0, ms) / 10.0);
                std::snprintf(out, n, "%s%02d:%02d.%02d", vor, cs / 6000, (cs / 100) % 60, cs % 100);
            };
            mmss(jetzt, sizeof(jetzt), g_app->playMs, "");
            mmss(gesamt, sizeof(gesamt), t.durationMs, " / ");
        }
        const ImVec2 t1 = ImGui::CalcTextSize(jetzt);
        const ImVec2 t2 = ImGui::CalcTextSize(gesamt);
        const float hh = ImGui::GetFrameHeight();
        const float pad = fsT * 0.4F;
        if (ImGui::InvisibleButton(tr(g_app->timelineFrames ? Str::TlUnitFrames : Str::TlUnitSeconds),
                                   ImVec2{t1.x + t2.x + pad * 2.0F, hh})) {
            g_app->timelineFrames = !g_app->timelineFrames;
        }
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        ImDrawList* dlT = ImGui::GetWindowDrawList();
        dlT->AddRectFilled(a, b,
                           ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBg),
                           ImGui::GetStyle().FrameRounding + 2.0F);
        const float ty = a.y + (hh - t1.y) * 0.5F;
        dlT->AddText(ImVec2{a.x + pad, ty}, ImGui::GetColorU32(ImGuiCol_Text), jetzt);
        dlT->AddText(ImVec2{a.x + pad + t1.x, ty}, ImGui::GetColorU32(ImGuiCol_TextDisabled), gesamt);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::TlUnitHint)); }
    }
    trenner();
    // --- Tempo und Schleife (Sequencer) --------------------------------
    if (iconKnopf(tr(Str::TlLoop), UiIcon::Schleife, tr(Str::TlLoopHint), g_app->schleife, true, true)) {
        g_app->schleife = !g_app->schleife;
    }
    ImGui::SameLine();
    {
        static const float kTempi[] = {0.25F, 0.5F, 1.0F, 2.0F};
        char tempo[16];
        std::snprintf(tempo, sizeof(tempo), "%gx", static_cast<double>(g_app->abspielTempo));
        ImGui::SetNextItemWidth(fsT * 3.4F);
        if (ImGui::BeginCombo("##tempo", tempo, ImGuiComboFlags_NoArrowButton)) {
            for (const float v : kTempi) {
                char n[16];
                std::snprintf(n, sizeof(n), "%gx", static_cast<double>(v));
                if (ImGui::Selectable(n, v == g_app->abspielTempo)) {
                    g_app->abspielTempo = v;
                    if (v != 1.0F) {
                        g_app->audio.stopAll();
                        g_app->laufendeSchleifen.clear();
                        tonZuruecksetzen();   // rc601 ton: auch der Raumklang
                        g_app->musicAudio.stopAll();
                    }
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::TlSpeedHint));
        }
    }
    trenner();
    if (iconKnopf(tr(Str::TlAudio), UiIcon::Ton, tr(Str::TlAudioHint), g_app->playAudio, true, true)) {
        g_app->playAudio = !g_app->playAudio;
        if (!g_app->playAudio) {
            g_app->audio.stopAll();
            g_app->laufendeSchleifen.clear();
            tonZuruecksetzen();   // rc601 ton: auch der Raumklang
            g_app->musicAudio.stopAll();
        } else if (g_app->playing) {
            // Beim EINschalten dort einsteigen, wo die Einstellung steht.
            //
            // Vorher passierte hier nichts: getroffen wird nur, was NEU
            // anfaengt, und der laufende Klang hatte schon begonnen. Also
            // blieb es still bis zum naechsten - beim Anwender fuenf
            // Sekunden, waehrend die Szene weiterlief.
            //
            // Der SPAETESTE Klang gewinnt: liegen zwei uebereinander, ist
            // der zuletzt begonnene der, den man hoert.
            wiedereinstieg();
        }
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::TlFollow), UiIcon::Folgen, tr(Str::TlFollowHint), g_app->followCam, true, true)) {
        g_app->followCam = !g_app->followCam;
        g_app->mapDirty = true;
    }
    trenner();
    // --- Zoom: verkleinern, Menue mit Stufen, vergroessern -----------------
    //
    // Radeln allein genuegt nicht: man sieht dem Rad nicht an, dass es
    // etwas tut, und feste Stufen trifft man damit nie genau. Der Knopf
    // zeigt zugleich den aktuellen Grad an - damit ist beides sichtbar,
    // die Moeglichkeit und der Zustand.
    {
        // Beim Umstellen die MITTE festhalten - sonst springt man beim
        // Stufenwechsel an den Anfang zurueck und sucht die Stelle wieder,
        // die man gerade ansah.
        const double gesamtM = std::max({g_app->timeline.practicalEndMs, t.durationMs, 1.0});
        const auto stellen = [&](double neuerZoom, double mitteMs) {
            const double zz = std::clamp(neuerZoom, 1.0, App::kMaxZoom);
            const double sp = gesamtM / zz;
            g_app->timelineZoom = zz;
            g_app->timelineViewStartMs = std::clamp(mitteMs - sp * 0.5, 0.0, std::max(0.0, gesamtM - sp));
        };
        const double mitteJetzt =
            g_app->timelineViewStartMs + gesamtM / std::max(g_app->timelineZoom, 1.0) * 0.5;
        if (iconKnopf(tr(Str::TlZoomOut), UiIcon::ZoomMinus, nullptr, false, g_app->timelineZoom > 1.0)) {
            stellen(g_app->timelineZoom / 2.0, mitteJetzt);
        }
        ImGui::SameLine();
        char knopf[48];
        if (g_app->timelineZoom > 1.0) {
            char grad[24];
            std::snprintf(grad, sizeof(grad), tr(Str::TlZoomFactor), g_app->timelineZoom);
            std::snprintf(knopf, sizeof(knopf), "%s: %s", tr(Str::TlZoomMenu), grad);
        } else {
            std::snprintf(knopf, sizeof(knopf), "%s: %s", tr(Str::TlZoomMenu), tr(Str::TlFit));
        }
        if (ImGui::Button(knopf)) {
            ImGui::OpenPopup("##zoommenu");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::TlZoomHint));
        }
        ImGui::SameLine();
        if (iconKnopf(tr(Str::TlZoomIn), UiIcon::ZoomPlus, nullptr, false, g_app->timelineZoom < App::kMaxZoom)) {
            // Um den Zeiger herum, wenn er im Bild ist - dorthin will man.
            const double sicht = gesamtM / std::max(g_app->timelineZoom, 1.0);
            const bool zeigerDa =
                g_app->playMs >= g_app->timelineViewStartMs && g_app->playMs <= g_app->timelineViewStartMs + sicht;
            stellen(g_app->timelineZoom * 2.0, zeigerDa ? g_app->playMs : mitteJetzt);
        }
        if (ImGui::BeginPopup("##zoommenu")) {
            if (ImGui::MenuItem(tr(Str::TlFit), nullptr, g_app->timelineZoom <= 1.0)) {
                g_app->timelineZoom = 1.0;
                g_app->timelineViewStartMs = 0.0;
            }
            ImGui::Separator();
            for (const double stufe : {2.0, 4.0, 8.0, 16.0, 32.0}) {
                char be[24];
                std::snprintf(be, sizeof(be), tr(Str::TlZoomFactor), stufe);
                if (ImGui::MenuItem(be, nullptr, std::fabs(g_app->timelineZoom - stufe) < 0.01)) {
                    stellen(stufe, mitteJetzt);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem(tr(Str::TlZoomToPlay))) {
                stellen(std::max(g_app->timelineZoom, 8.0), g_app->playMs);
            }
            ImGui::EndPopup();
        }
    }
    trenner();
    // Je Figur eine Zeile, und die ruhigen Spuren (nach dem Anfang passiert
    // darin nichts mehr - in intro_jedi 6 von 10).
    if (iconKnopf(tr(Str::TlTracks), UiIcon::Zeilen, tr(Str::TlTracksHint), g_app->showTracks, true, true)) {
        g_app->showTracks = !g_app->showTracks;
        g_app->mapDirty = true;
    }
    {
        int ruhig = 0;
        for (const TimelineTrack& tr2 : g_app->timeline.tracks) {
            if (spurIstRuhig(tr2)) { ++ruhig; }
        }
        if (ruhig > 0 && g_app->showTracks) {
            ImGui::SameLine();
            char lab[64];
            std::snprintf(lab, sizeof(lab), tr(Str::TlIdleTracks), ruhig);
            if (iconKnopf(lab, UiIcon::Ruhig, tr(Str::TlIdleTracksHint), g_app->zeigeRuhigeSpuren, true, true)) {
                g_app->zeigeRuhigeSpuren = !g_app->zeigeRuhigeSpuren;
            }
            ImGui::SameLine(0.0F, 2.0F);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%d", ruhig);
        }
        // waits als eigene Bloecke (rc597, sonst ausgeblendet).
        if (g_app->showTracks) {
            ImGui::SameLine();
            if (iconKnopf(tr(Str::TlWaits), UiIcon::Pause, tr(Str::TlWaitsHint), g_app->tlWaitsZeigen, true, true)) {
                g_app->tlWaitsZeigen = !g_app->tlWaitsZeigen;
            }
        }
    }
    trenner();
    // Eine Kameraeinstellung an der jetzigen Ansicht - vorher ein Textknopf
    // in einer eigenen Zeile ueber der Leiste.
    if (iconTextKnopf(tr(Str::MapInsertCamera), UiIcon::Kamera, tr(Str::CamNewShotHint),
                      !g_app->geo.empty() && !g_app->camViewActive)) {
        kameraNeueEinstellung();
    }
    drawPickedEntity();
    ImGui::PopStyleVar();

    if (g_app->playing) {
        // audioUpTo statt "der Stand vom letzten Bild".
        //
        // Der Unterschied ist genau ein Klang: einer bei 0 ms. Mit
        // "startMs > vorheriger Stand" ist die Bedingung beim ersten Bild
        // 0 > 0 - also nie wahr, und der erste Satz einer Sequenz bliebe
        // stumm. audioUpTo beginnt bei -1, und damit faellt auch die Null
        // ins Fenster.
        if (g_app->uhrAussetzen > 0) {
            --g_app->uhrAussetzen;   // Vorladezeit ist keine Abspielzeit
        } else {
            g_app->playMs += static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0 *
                             static_cast<double>(g_app->abspielTempo);
        }
        // Klaenge ausloesen, die in dieses Zeitfenster fallen.
        //
        // Ueber das FENSTER, nicht ueber "steht der Zeiger genau darauf":
        // bei sechzig Bildern je Sekunde liegen sechzehn Millisekunden
        // zwischen zwei Abfragen, ein Klang faellt sonst durch.
        if (g_app->playAudio && g_app->abspielTempo == 1.0F) {
            for (const TimelineTrack& track : g_app->timeline.tracks) {
                for (const TimelineEvent& e : track.events) {
                    if (e.startMs <= g_app->audioUpTo ||
                        e.startMs > g_app->playMs) {
                        continue;
                    }
                    if (e.kind == TimelineEvent::Kind::Sound &&
                        !e.sound.empty()) {
                        spieleSkriptKlang(track.entity, e.sound, e.kanal, e.startMs);
                    }
                    // rc601 ton: Kind::Saber spielt hier nichts mehr.
                    // Q3_SetSaberActive (Q3_Interface.cpp:6852 ff. MD) ruft
                    // nur ps.SaberActivate()/SaberDeactivate() - es gibt KEIN
                    // soundOff dafuer. soundOn spielt CG_Player, sobald eine
                    // Klinge bei Laenge 0 anfaengt und die Figur gezeichnet
                    // wird (tonBild): eine unsichtbare Figur zuendet nicht
                    // hoerbar, ein zweites "true" ist still.
                }
            }
            // --- Die Levelmusik (worldspawn "music") ab 0 ms --------------
            if (g_app->audioUpTo < 0.0 && g_app->playMs >= 0.0) {
                const std::string welt = weltMusik();
                if (!welt.empty()) {
                    spieleMusik(welt, g_app->playMs);
                }
            }
            // --- use: Musik ---------------------------------------------
            //
            // Aus dem Nachbau, nicht aus der Zeitleiste: dort stehen nur die
            // use-Befehle des Skripts, hier auch alles, was ueber
            // target_relay, target_delay, target_counter und trigger_*
            // weitergereicht wird.
            //
            // rc601 ton: NUR noch die Musik. Die target_speaker kommen aus
            // MoverSim::lautsprecher (tonEreignisse) - vorher spielte jedes
            // use den "noise" der ERSTEN Entity mit dem Namen (auch eines
            // func_breakable oder trigger_*), und ohne origin voll und ohne
            // Ort; ein Schleifen-Lautsprecher klang dazu einmal extra.
            for (const Ablauf::Benutzung& b : g_app->ablauf.benutzt) {
                if (b.ms <= g_app->audioUpTo || b.ms > g_app->playMs) {
                    continue;
                }
                bool musik = false;
                const std::string datei = useSoundFor(b.name, &musik);
                if (musik && !datei.empty()) {
                    spieleMusik(datei, 0.0);
                }
            }
            // --- Die Kartenereignisse (rc601 ton) ------------------------
            //
            // target_speaker, Tueren (START/END), ROFF-Notizen, Zerbrechen
            // und fx_runner - jeder nach seinem Weg in der Engine: ueber den
            // Snapshot (nur im PVS) oder direkt, am Ort oder an der Entity,
            // im richtigen Kanal. Siehe tonEreignisse (app_ton.cpp).
            tonEreignisse(g_app->audioUpTo, g_app->playMs);
            // rc601 saber: saberhitwall, saberblock, hitwater im selben Fenster.
            saberTonEreignisse(g_app->audioUpTo, g_app->playMs);
        }
        // --- Jedes Bild: nachregeln und Dauerklaenge (rc601 ton) ---------
        //
        // Wie S_Respatialize (snd_dma.cpp:2571 ff. MD): jeder laufende Klang
        // bekommt seinen Pegel neu (Hoerer und Quelle bewegen sich), ein
        // stummer Nicht-Stimmkanal endet, Stimmen laufen still weiter; die
        // Schleifen (Schwertsummen, Lautsprecher, Fahrgeraeusche) werden
        // frisch gebaut und je Datei zusammengelegt (S_AddLoopSounds); dazu
        // der Zuendklang wie CG_Player. Vorher: Pegel einmal beim Start,
        // Schleifen auf 120 s ausgerollt und nie nachgeregelt.
        if (g_app->playAudio && g_app->abspielTempo == 1.0F) {
            tonBild();
        }
        g_app->audioUpTo = g_app->playMs;
        if (g_app->playMs >= t.durationMs) {
            if (g_app->schleife && t.durationMs > 1.0) {
                // Von vorn, ohne anzuhalten - und die Klaenge ab 0 wieder.
                g_app->playMs = 0.0;
                g_app->audioUpTo = -1.0;
                g_app->audio.stopAll();
                g_app->laufendeSchleifen.clear();
                tonZuruecksetzen();   // rc601 ton: auch der Raumklang
                g_app->musicAudio.stopAll();
            } else {
                g_app->playMs = t.durationMs;
                g_app->playing = false;
            }
        }
        g_app->mapDirty = true;
    }

    // --- EINE Namensspalte fuer alles ------------------------------------
    //
    // Zeiger, Balken und Lineal begannen am linken Rand, die Spuren erst
    // hinter ihren Namen. Damit stand ueber jeder Spur eine Zeitachse, die
    // um die Namensbreite verschoben war - die Zahl 20 stand nicht ueber
    // der Sekunde 20. Gemeldet als "die timeline startet schon vor den
    // scripts".
    //
    // Jetzt gibt es EINE Kante, an der alles beginnt. Sie richtet sich nach
    // dem laengsten Namen, damit auch "bigguard1" hineinpasst, bleibt aber
    // in Grenzen - sonst frisst die Namensspalte die Zeitachse auf.
    float labelW = ImGui::GetFontSize() * 6.0F;
    for (const TimelineTrack& tr2 : g_app->timeline.tracks) {
        const std::string nm = tr2.entity.empty() ? std::string(tr(Str::TlScript))
                                                  : tr2.entity;
        labelW = std::max(labelW, ImGui::CalcTextSize(nm.c_str()).x);
    }
    labelW = std::min(labelW + ImGui::GetStyle().ItemSpacing.x * 2.0F,
                      ImGui::GetContentRegionAvail().x * 0.25F);

    // Der Zeiger steht seit 27.09. IM LINEAL: klicken oder ziehen setzt die
    // Zeit (wie im Unreal-Sequencer und in jedem Schnittprogramm). Der
    // eigene Schieberegler darueber kostete eine Zeile und war ein zweites
    // Bedienelement fuer dieselbe Sache.
    ImGui::Dummy(ImVec2{labelW, 1.0F});
    ImGui::SameLine(0.0F, 0.0F);
    // Die ROLLLEISTE der Spurenliste mit einrechnen.
    //
    // Die Spuren stehen in einem Kindfenster, das senkrecht rollt - seine
    // Rollleiste nimmt sich Platz vom INHALT. Zeiger und Lineal darueber
    // haben keine und reichten deshalb ein Stueck weiter nach rechts als
    // die Balken darunter. Gemeldet als "die timeline rechts ist bisschen
    // zu lang mit dem runter scrollen".
    //
    // Der Platz wird IMMER abgezogen, nicht nur wenn gerade gerollt wird -
    // sonst wandert die rechte Kante, sobald eine Spur dazukommt oder
    // wegfaellt, und die Zahlen im Lineal springen mit.
    const float rollBreite = ImGui::GetStyle().ScrollbarSize;
    ImGui::SetNextItemWidth(-rollBreite);
    // Der Regler laeuft ueber den SICHTBAREN Bereich. Sonst waere beim
    // Hineinzoomen der Griff auf einem Zehntel Bildpunkt zu treffen.
    // Hinter dem Ende 5 % Luft: sonst liegt die Endkante des letzten Blocks
    // genau am rechten Rand, und man kann ihn kaum greifen und verlaengern.
    const double ablaufEnde = std::max({g_app->timeline.practicalEndMs, t.durationMs, 1.0});
    const double rGesamt = ablaufEnde * 1.05;
    const double rSpanne = rGesamt / std::clamp(g_app->timelineZoom, 1.0, App::kMaxZoom);
    const double rStart = std::clamp(g_app->timelineViewStartMs, 0.0,
                                     std::max(0.0, rGesamt - rSpanne));
    const ImVec2 barMin = ImGui::GetCursorScreenPos();
    const float w = std::max(40.0F, ImGui::GetContentRegionAvail().x - rollBreite);
    (void)rStart;

    // Der UEBERSICHTSBALKEN stand frueher HIER - zwischen Zeiger und
    // Lineal. Gemeldet: "da ist was ueber unserer Timeline, alles wo Dinge
    // wie camera sind sollte unter der Timeline sein."
    //
    // Er ist jetzt hinter das Lineal gerueckt. Der Grund ist nicht nur
    // Geschmack: der Balken zeigt Kamerabefehle, und die Kamerazeilen der
    // Spurenliste zeigen dieselben Befehle noch einmal einzeln. Steht der
    // Balken oben, liegt eine Kameraanzeige ueber der Zeitachse und eine
    // darunter - zwei Orte fuer dieselbe Sache, getrennt durch die
    // Beschriftung. Unten stehen sie beieinander.

    // --- Die Spuren ------------------------------------------------------
    //
    // Eine Zeile je affect-Ziel. Das ist der Teil, fuer den die Zeitleiste
    // wirklich da ist: man sieht, wann welche Figur spricht, und kann den
    // Schnitt darauf abstimmen.
    //
    // In intro_jedi.txt sind das zehn Spuren - das Skript selbst mit 79
    // Kamerabefehlen, dazu anakin1, barriss1 und die Wachen. Die Klaenge
    // liegen bei 20,2 s (barriss1), 23,0 s (anakin1) und so fort.
    // --- Das Lineal ------------------------------------------------------
    //
    // Bilder ODER Sekunden, umschaltbar - in 3ds Max geht beides, und
    // beides wird gebraucht: Sekunden fuer die Laenge einer Einstellung,
    // Bilder fuer das genaue Setzen eines Schluessels. JKA rechnet intern
    // in Millisekunden; die Bildzahl ist eine reine Anzeige.
    {
        // Hoeher als frueher (0,9 Zeilen), und mit eigenem Grund.
        //
        // Das Rad wirkt ueber DIESEM Streifen - bei einer knappen Zeile
        // Hoehe trifft man ihn kaum und weiss auch nicht, dass dort etwas
        // zu holen ist. Gemeldet als "was fuer ein Lineal? wie geht das?".
        // Jetzt ist er anderthalb Zeilen hoch und hebt sich ab, damit man
        // sieht, dass er ein Bedienelement ist.
        const float lh = ImGui::GetFontSize() * 1.5F;
        ImVec2 rp = ImGui::GetCursorScreenPos();
        rp.x = barMin.x;   // dieselbe Kante wie Zeiger, Balken und Spuren
        ImDrawList* rl = ImGui::GetWindowDrawList();
        // Ein eigener Grund, damit der Streifen als Leiste zu erkennen ist.
        rl->AddRectFilled(ImVec2{rp.x, rp.y},
                          ImVec2{rp.x + w, rp.y + lh},
                          IM_COL32(34, 37, 46, 255));
        // Nur den SICHTBAREN Bereich beschriften. Vorher lief das Lineal
        // immer von null bis zum Ende - beim Zoomen haetten die Zahlen
        // nicht mehr zu den Balken darunter gepasst.
        const double gesamt = rGesamt;
        const double zSpanne = gesamt / std::clamp(g_app->timelineZoom, 1.0, App::kMaxZoom);
        const double zStart =
            std::clamp(g_app->timelineViewStartMs, 0.0,
                       std::max(0.0, gesamt - zSpanne));
        const double fps = static_cast<double>(std::max(1, g_app->timelineFps));
        // Schrittweite so waehlen, dass die Beschriftungen nicht kleben:
        // etwa alle 80 Bildpunkte eine. Die Stufen sind die gewohnten -
        // 1, 2, 5, 10, ... - damit runde Zahlen dastehen.
        // In der eingestellten Einheit: erste und letzte sichtbare Marke.
        const double jeMs = g_app->timelineFrames ? (fps / 1000.0) : (1.0 / 1000.0);
        const double vonE = zStart * jeMs;
        const double bisE = (zStart + zSpanne) * jeMs;
        const double sichtbar = std::max(bisE - vonE, 1.0e-6);
        // Schrittweite in den gewohnten Stufen 1, 2, 5, 10 ... - beim
        // Hineinzoomen wird sie von selbst feiner, weil "sichtbar" kleiner
        // wird. Genau dafuer ist die Rechnung da.
        double schritt = 1.0;
        // Wie viel Platz braucht EINE Beschriftung? Gemessen, nicht
        // geraten: bei 144 dpi ist "1234.5" deutlich breiter als die 80
        // Bildpunkte, mit denen hier frueher gerechnet wurde - und dann
        // stehen die Zahlen ineinander, wie im gemeldeten Bild.
        const float platz =
            ImGui::CalcTextSize("0000.0").x + ImGui::GetFontSize();
        const double gewuenscht =
            std::max(1.0e-6, sichtbar / std::max(w / platz, 1.0F));
        if (gewuenscht < 1.0) {
            // Die kleinste Stufe, die NOCH GROSS GENUG ist.
            //
            // Vorher lief die Schleife, bis der Schritt KLEINER war als
            // gewuenscht - also eine Stufe zu fein, und damit standen
            // doppelt so viele Zahlen da wie Platz war. Jetzt wird nur
            // halbiert, solange die Haelfte noch reicht.
            while (schritt * 0.5 >= gewuenscht) { schritt *= 0.5; }
        } else {
            while (schritt < gewuenscht) {
                if (schritt * 2.0 >= gewuenscht) { schritt *= 2.0; break; }
                if (schritt * 5.0 >= gewuenscht) { schritt *= 5.0; break; }
                schritt *= 10.0;
            }
        }
        const double ersteE = std::ceil(vonE / schritt) * schritt;
        for (double e = ersteE; e <= bisE + schritt * 0.001; e += schritt) {
            const auto x =
                rp.x + static_cast<float>((e - vonE) / sichtbar) * w;
            rl->AddLine(ImVec2{x, rp.y + lh * 0.45F}, ImVec2{x, rp.y + lh},
                        IM_COL32(120, 130, 150, 255));
            char be[24];
            // Bei starkem Zoom sind ganze Zahlen zu grob. So viele
            // Nachkommastellen, wie der Schritt braucht: bei 0,25 zwei -
            // mit einer stand "0.2, 0.5, 0.8" da, wo 0,25, 0,5, 0,75 gemeint
            // war (Zeitleistentest 27.09.).
            int stellen = 0;
            while (stellen < 3) {
                const double f = schritt * std::pow(10.0, stellen);
                if (std::fabs(f - std::round(f)) < 1.0e-6) { break; }
                ++stellen;
            }
            // Sekunden als m:ss (rc588) - "1:05" liest sich schneller als "65".
            if (!g_app->timelineFrames && stellen == 0 && e >= 60.0) {
                const int sek = static_cast<int>(std::lround(e));
                std::snprintf(be, sizeof(be), "%d:%02d", sek / 60, sek % 60);
            } else {
                std::snprintf(be, sizeof(be), "%.*f", stellen, e);
            }
            rl->AddText(ImVec2{x + 2.0F, rp.y - 1.0F},
                        IM_COL32(150, 160, 180, 255), be);
        }
        // --- Zoomen und Verschieben ---------------------------------
        //
        // Mausrad ueber dem Lineal vergroessert um den PUNKT UNTER DER
        // MAUS - so, wie es Blender und jedes Schnittprogramm machen.
        // Der Zeitwert dort bleibt stehen, alles andere rueckt darum
        // herum auseinander. Ohne diesen Bezug zoomt man ins Blaue und
        // muss danach suchen.
        //
        // Mittlere Taste ziehen schiebt den Ausschnitt.
        g_app->tlLinealY = rp.y + lh * 0.5F;
        ImGui::InvisibleButton("##zeitlineal", ImVec2{w, lh});
        // Klicken oder Ziehen mit der linken Taste setzt die Zeit.
        if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const double anteil = std::clamp<double>(
                (ImGui::GetIO().MousePos.x - rp.x) / std::max(w, 1.0F), 0.0, 1.0);
            const double ms = std::clamp(zStart + anteil * zSpanne, 0.0, gesamt);
            g_app->playMs = ms;
            g_app->playing = false;
            // Beim Ziehen nichts nachtroepfeln lassen.
            g_app->audioUpTo = ms;
            g_app->audio.stopAll();
            g_app->laufendeSchleifen.clear();
            tonZuruecksetzen();   // rc601 ton: auch der Raumklang
            g_app->musicAudio.stopAll();
            g_app->mapDirty = true;
        }
        // Der Zeiger im Lineal: eine Spitze oben und eine Linie, dazu das
        // Ende des Ablaufs als rote Marke.
        {
            const auto px = rp.x + static_cast<float>((g_app->playMs - zStart) / zSpanne) * w;
            if (px >= rp.x - 1.0F && px <= rp.x + w + 1.0F) {
                // Der Zeiger als Marke mit der Zeit darin (rc588), wie in
                // Unreals Sequencer - man sieht die Zeit dort, wo man hinschaut.
                const ImU32 akz = IM_COL32(255, 160, 50, 255);
                char zt[24];
                const int cs = static_cast<int>(std::max(0.0, g_app->playMs) / 10.0);
                if (g_app->timelineFrames) {
                    std::snprintf(zt, sizeof(zt), "%d", static_cast<int>(g_app->playMs / 1000.0 * fps));
                } else {
                    std::snprintf(zt, sizeof(zt), "%d:%02d.%02d", cs / 6000, (cs / 100) % 60, cs % 100);
                }
                const ImVec2 ts = ImGui::CalcTextSize(zt);
                const float pb = ts.x + ImGui::GetFontSize() * 0.5F;
                const float kx = std::clamp(px - pb * 0.5F, rp.x, rp.x + w - pb);
                const float kh = std::min(lh, ts.y + 2.0F);
                rl->AddRectFilled(ImVec2{kx, rp.y}, ImVec2{kx + pb, rp.y + kh}, akz, kh * 0.35F);
                rl->AddText(ImVec2{kx + (pb - ts.x) * 0.5F, rp.y + (kh - ts.y) * 0.5F}, IM_COL32(25, 20, 10, 255), zt);
                rl->AddLine(ImVec2{px, rp.y + kh}, ImVec2{px, rp.y + lh}, akz, 2.0F);
            }
            const auto ex = rp.x + static_cast<float>((ablaufEnde - zStart) / zSpanne) * w;
            if (ex >= rp.x && ex <= rp.x + w) {
                rl->AddLine(ImVec2{ex, rp.y}, ImVec2{ex, rp.y + lh}, IM_COL32(230, 80, 80, 230), 2.0F);
            }
        }
        if (ImGui::IsItemHovered()) {
            const float rad = ImGui::GetIO().MouseWheel;
            if (rad != 0.0F) {
                const float mausX = ImGui::GetIO().MousePos.x;
                const double anteil =
                    std::clamp<double>((mausX - rp.x) / std::max(w, 1.0F), 0.0, 1.0);
                const double unterMaus = zStart + anteil * zSpanne;
                const double neu =
                    std::clamp(g_app->timelineZoom * std::pow(1.25, rad), 1.0, App::kMaxZoom);
                const double neueSpanne = gesamt / neu;
                g_app->timelineZoom = neu;
                g_app->timelineViewStartMs =
                    std::clamp(unterMaus - anteil * neueSpanne, 0.0,
                               std::max(0.0, gesamt - neueSpanne));
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                const float dx = ImGui::GetIO().MouseDelta.x;
                g_app->timelineViewStartMs =
                    std::clamp(g_app->timelineViewStartMs -
                                   static_cast<double>(dx) / std::max(w, 1.0F) *
                                       zSpanne,
                               0.0, std::max(0.0, gesamt - zSpanne));
            }
            // Die Zeit unter der Maus - eine feine Linie und die Zahl, damit
            // man vor dem Klick sieht, wohin der Zeiger springt (rc588).
            const float mx = ImGui::GetIO().MousePos.x;
            const double unter = zStart + std::clamp<double>((mx - rp.x) / std::max(w, 1.0F), 0.0, 1.0) * zSpanne;
            rl->AddLine(ImVec2{mx, rp.y}, ImVec2{mx, rp.y + lh}, IM_COL32(255, 255, 255, 90), 1.0F);
            char zu[32];
            if (g_app->timelineFrames) {
                std::snprintf(zu, sizeof(zu), "%d", static_cast<int>(unter / 1000.0 * fps));
            } else {
                const int cs = static_cast<int>(std::max(0.0, unter) / 10.0);
                std::snprintf(zu, sizeof(zu), "%d:%02d.%02d", cs / 6000, (cs / 100) % 60, cs % 100);
            }
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(zu);
            ImGui::TextDisabled("%s", tr(Str::TlZoomHint));
            ImGui::EndTooltip();
        }
    }

    // --- Der Uebersichtsbalken -------------------------------------------
    //
    // Eine Zeile, in der alle Kamerabefehle nebeneinander liegen: blau
    // fahren, orange schwenken, gruen zoomen. Darunter stehen dieselben
    // Befehle noch einmal einzeln je Art - der Balken ist der Blick aufs
    // Ganze, die Zeilen sind die Einzelheiten.
    //
    // Er steht seit rc242 UNTER dem Lineal. Vorher lag er darueber, also
    // auf der falschen Seite der Zeitachse.
    //
    // Seit 27.09. nur noch, wenn die Spuren AUS sind: sonst zeigen die
    // Kamerazeilen darunter genau dasselbe, und die Zeile fehlte den Spuren.
    if (!g_app->showTracks || g_app->timeline.tracks.empty()) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float h = ImGui::GetFontSize() * 0.5F;
        // Der Anker kommt vom CURSOR, nicht mehr von der Unterkante des
        // Zeigers.
        //
        // Das ist der Fallstrick aus Abschnitt 4.1 des Handouts: etwas
        // messen, das man selbst verschoben hat. Bliebe hier barMax.y+2
        // stehen, zeichnete der Balken weiterhin an der alten Stelle -
        // ueber dem Lineal -, waehrend die Dummy-Zeile unten Platz
        // freihielte. Man saehe zwei Fehler auf einmal und keinen davon
        // erklaert.
        //
        // Die x-Kante dagegen MUSS von barMin kommen: sie ist dieselbe
        // Kante, an der Zeiger, Lineal und Spuren beginnen.
        ImVec2 top = ImGui::GetCursorScreenPos();
        top.x = barMin.x;
        top.y += 2.0F;
        dl->AddRectFilled(top, ImVec2{top.x + w, top.y + h},
                          IM_COL32(30, 33, 42, 255));
        for (const CamSegment& seg : t.segments) {
            if (t.durationMs <= 0.0) {
                break;
            }
            // Gegen das ZOOMFENSTER rechnen, nicht gegen die Gesamtdauer.
            //
            // Gemeldet: "es laeuft asynchron mit dem anderen darunter."
            //
            // Der Zeiger darueber laeuft von rStart bis rStart+rSpanne -
            // dem Ausschnitt, den man gerade sieht. Diese Balken rechneten
            // gegen t.durationMs, die GESAMTdauer. Zwei Massstaebe
            // uebereinander: solange der ganze Ablauf zu sehen ist, faellt
            // es kaum auf, sobald man hineinzoomt, laufen sie auseinander.
            //
            // Auch bei "Zoom: All" stimmten sie nicht genau, weil rSpanne
            // sich nach der Zeitleiste richtet und durationMs nach der
            // Kamerabahn - das sind nicht dieselben Zahlen.
            auto x0 = static_cast<float>((seg.startMs - rStart) / rSpanne) * w;
            auto x1 = static_cast<float>((seg.endMs - rStart) / rSpanne) * w;
            // Beschneiden, sobald hineingezoomt ist: ein Abschnitt, der vor
            // dem Ausschnitt beginnt, haette sonst einen negativen Anfang
            // und zeichnete nach links aus der Leiste heraus.
            if (x1 < 0.0F || x0 > w) {
                continue;
            }
            x0 = std::max(x0, 0.0F);
            x1 = std::min(x1, w);
            ImU32 col = IM_COL32(90, 110, 140, 255);
            switch (seg.kind) {
                case CamSegment::Kind::Move: col = IM_COL32(96, 170, 255, 255); break;
                case CamSegment::Kind::Pan:  col = IM_COL32(255, 190, 80, 255); break;
                case CamSegment::Kind::Zoom: col = IM_COL32(140, 220, 140, 255); break;
                default: break;
            }
            // Ein Sprung hat keine Dauer - trotzdem zwei Punkte breit
            // zeichnen, sonst ist er unsichtbar.
            dl->AddRectFilled(ImVec2{top.x + x0, top.y},
                              ImVec2{top.x + std::max(x1, x0 + 2.0F), top.y + h},
                              col);
        }
        // Der weisse Strich im Balken lief noch im ALTEN Massstab.
        //
        // rc241 hat die farbigen Abschnitte auf das Zoomfenster umgestellt
        // und diese eine Zeile stehenlassen - sie rechnete weiter
        // playMs/durationMs, also gegen die Gesamtdauer. Beim Hineinzoomen
        // stand der Strich damit woanders als die Abschnitte, durch die er
        // laufen soll. Derselbe Fehler, dieselbe Leiste, nur eine Zeile
        // weiter unten uebersehen.
        const auto px = static_cast<float>((g_app->playMs - rStart) / rSpanne) * w;
        if (px >= 0.0F && px <= w) {
            dl->AddLine(ImVec2{top.x + px, top.y - 2.0F},
                        ImVec2{top.x + px, top.y + h + 2.0F},
                        IM_COL32(255, 255, 255, 220), 1.5F);
        }
        ImGui::Dummy(ImVec2{w, h + 4.0F});
    }

    if (!g_app->showTracks || g_app->timeline.tracks.empty()) {
        return;
    }

    // Die Gesamtdauer der SPUREN, nicht die der Kamerabahn: eine Figur kann
    // nach dem letzten Kamerabefehl noch sprechen.
    const double span = rGesamt;
    // Zeilen so hoch wie eine Textzeile - vorher waren es 0,72 davon, und
    // die Namen standen gequetscht uebereinander.
    const float rowH = ImGui::GetTextLineHeightWithSpacing();
    // labelW steht schon oben - eine Kante fuer alles.
    const float trackW = std::max(w, 40.0F);

    // --- Der sichtbare Ausschnitt ----------------------------------------
    //
    // Alles unterhalb rechnet ab jetzt in DIESEM Ausschnitt, nicht mehr in
    // der Gesamtdauer. Die Vergroesserung wird geklemmt, damit man weder
    // hinter das Ende noch vor den Anfang rutscht - sonst sucht man die
    // Leiste und findet Leere.
    g_app->timelineZoom = std::clamp(g_app->timelineZoom, 1.0, App::kMaxZoom);
    const double sichtSpanne = span / g_app->timelineZoom;
    g_app->timelineViewStartMs =
        std::clamp(g_app->timelineViewStartMs, 0.0, std::max(0.0, span - sichtSpanne));
    const double sichtStart = g_app->timelineViewStartMs;
    // Zeit -> Bildpunkt. EINE Stelle, an der die Umrechnung steht; vorher
    // stand sie sechsmal ausgeschrieben da, und beim Zoom haetten sechs
    // Abschriften auseinanderlaufen muessen.
    const auto xAt = [&](double ms) {
        return barMin.x +
               static_cast<float>((ms - sichtStart) / sichtSpanne) * trackW;
    };
    g_app->tlFeldX = barMin.x;
    g_app->tlFeldW = trackW;
    g_app->tlSichtStart = sichtStart;
    g_app->tlSichtSpanne = sichtSpanne;
    g_app->tlZeilenY.clear();
    // Die Spuren fuellen, was uebrig ist. Wer die Leiste hochzieht, sieht
    // MEHR Spuren - genau das ist der Sinn der ziehbaren Kante. Vorher
    // waren es feste 4,6 Zeilen, egal wie viel Platz da war.
    const float restH = std::max(rowH, ImGui::GetContentRegionAvail().y - 2.0F);
    // Die Kamerazeilen zaehlen mit - sonst waere der Platz zu knapp
    // berechnet und die Spuren darunter waeren nicht mehr erreichbar.
    // Gezaehlt wird ueber die vorkommenden ARTEN, nicht ueber eine
    // angenommene Zahl: die Aufzaehlung hat neun Werte, gezeigt werden
    // sieben (Enable und Disable sind Zustaende, keine Bewegung).
    int camZeilenZahl = 0;
    for (const CamSegment::Kind art :
         {CamSegment::Kind::Move, CamSegment::Kind::Pan,
          CamSegment::Kind::Zoom, CamSegment::Kind::Roll,
          CamSegment::Kind::Fade, CamSegment::Kind::Shake,
          CamSegment::Kind::Follow}) {
        for (const CamSegment& seg : t.segments) {
            if (seg.kind == art) {
                ++camZeilenZahl;
                break;
            }
        }
    }
    // --- Spuren je Figur, wie im Unreal Sequencer (rc597) -------------------
    //
    // shank: "es ist fuer jeden Character alles in eine Leiste gestopft ...
    // schau mal nach, wie Unreal Engine das handhabt, so dass es logisch ist
    // und man auch aufklappen kann". Jede Figur ist eine Gruppe; aufgeklappt
    // bekommt jede Art von Befehl ihre Spur. Die waits sind nur der Kleber,
    // der die Befehle in die Zeit setzt - sie erscheinen nur auf Wunsch.
    //   0 Bewegung  SET_NAVGOAL, SET_WALKING/RUNNING, SET_ORIGIN, move, rotate ...
    //   1 Animation SET_ANIM_*
    //   2 Blick     SET_LOOK_TARGET, SET_WATCHTARGET, SET_VIEWTARGET
    //   3 Ton       sound, use mit Klang
    //   4 Zustand   alles andere (Waffe, Klinge, Verhalten, use ...)
    //   5 Ablauf    wait, signal, waitsignal
    const auto kategorieVon = [](const TimelineEvent& e) -> int {
        std::string l = e.label;
        for (char& ch : l) { ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch))); }
        const auto hat = [&](const char* t) { return l.find(t) != std::string::npos; };
        switch (e.kind) {
            case TimelineEvent::Kind::Wait: return 5;
            case TimelineEvent::Kind::Move: return 0;
            case TimelineEvent::Kind::Sound: return 3;
            case TimelineEvent::Kind::Use: return useSoundFor(e.target, nullptr).empty() ? 4 : 3;
            case TimelineEvent::Kind::Saber: return 4;
            case TimelineEvent::Kind::Set:
                if (hat("SET_ANIM")) { return 1; }
                if (hat("LOOK_TARGET") || hat("WATCHTARGET") || hat("VIEWTARGET")) { return 2; }
                if (hat("NAVGOAL") || hat("WALKING") || hat("RUNNING") || hat("ORIGIN") || hat("SET_ANGLES") ||
                    hat("WALKSPEED") || hat("RUNSPEED") || hat("YAWSPEED") || hat("LOCKYAW") || hat("CROUCH")) {
                    return 0;
                }
                return 4;
            default:
                if (l.rfind("SIGNAL", 0) == 0 || l.rfind("WAITSIGNAL", 0) == 0 || l.rfind("WAIT", 0) == 0) { return 5; }
                if (hat("PLAY_ROFF")) { return 0; }   // eine Bahn (rc599)
                return 4;
        }
    };
    struct KatInfo {
        Str name;
        UiIcon icon;
        ImU32 farbe;
    };
    static const KatInfo kKat[6] = {
        {Str::TlKatBewegung, UiIcon::Pfad, IM_COL32(90, 220, 190, 255)},
        {Str::TlKatAnimation, UiIcon::Animieren, IM_COL32(150, 200, 255, 255)},
        {Str::TlKatBlick, UiIcon::Zielen, IM_COL32(255, 210, 120, 255)},
        {Str::TlKatTon, UiIcon::Ton, IM_COL32(255, 120, 200, 255)},
        {Str::TlKatZustand, UiIcon::Einstellungen, IM_COL32(190, 160, 90, 255)},
        {Str::TlKatAblauf, UiIcon::Pause, IM_COL32(120, 126, 145, 255)},
    };
    // Welche Unterspuren eine Figur hat (fuer die Hoehe und zum Zeichnen).
    const auto katDerSpur = [&](const TimelineTrack& tr2) {
        std::array<bool, 6> hatKat{};
        for (const TimelineEvent& e : tr2.events) { hatKat[static_cast<std::size_t>(kategorieVon(e))] = true; }
        for (const Actor& ac : g_app->scene.actors) {
            if (ac.name != tr2.entity) { continue; }
            for (const ActorStep& st : ac.steps) {
                if (st.kind == ActorStep::Kind::WalkTo || st.kind == ActorStep::Kind::Glide) { hatKat[0] = true; }
            }
        }
        if (!g_app->tlWaitsZeigen) { hatKat[5] = false; }
        return hatKat;
    };
    int sichtbareSpuren = 0;
    for (const TimelineTrack& tr2 : g_app->timeline.tracks) {
        if (g_app->zeigeRuhigeSpuren || !spurIstRuhig(tr2)) {
            ++sichtbareSpuren;
            if (!tr2.entity.empty() && g_app->tlOffen.count(tr2.entity) != 0U) {
                for (const bool b : katDerSpur(tr2)) { sichtbareSpuren += b ? 1 : 0; }
            }
        }
    }
    const int kameraZeilen = camZeilenZahl > 0 ? 1 + (g_app->tlKameraOffen ? camZeilenZahl : 0) : 0;
    // Je Zeile auch der Abstand zwischen zwei Elementen - ohne ihn fehlte
    // bei vielen Zeilen unten eine ganze (rc597: die Kameragruppe hat eine
    // Kopfzeile mehr, und die Skriptzeile war abgeschnitten).
    const float noetig =
        (rowH + ImGui::GetStyle().ItemSpacing.y) * (static_cast<float>(sichtbareSpuren) +
                                                    static_cast<float>(kameraZeilen)) +
        8.0F;
    // Volle Breite UND immer eine Rollleiste: dann ist der Inhalt genau so
    // breit wie Zeiger und Lineal darueber, und die Kanten fallen zusammen.
    ImGui::BeginChild("tracks", ImVec2{w + labelW + rollBreite,
                                       std::min(restH, noetig)},
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImDrawList* tl = ImGui::GetWindowDrawList();

    // --- Die Kamera in ihre STUFEN aufgeteilt ----------------------------
    //
    // Vorher lag die ganze Kamerabahn in einem einzigen Streifen: Fahrt,
    // Schwenk und Zoom uebereinander in derselben Zeile, und wo zwei
    // gleichzeitig laufen, sah man nur den letzten. Gemeldet als "die
    // Camera ist ein grosser Balken - wenn es mehrere kleine Abschnitte
    // hat, sollten die besser aufgeteilt sein".
    //
    // Jetzt bekommt jeder Unterbefehl seine eigene Zeile - wie die Spuren
    // eines Schnittprogramms. Eine Zeile bleibt leer, wenn ihr Unterbefehl
    // im Skript nicht vorkommt; das haelt die Leiste kurz.
    struct CamZeile {
        CamSegment::Kind kind;
        const char* name;
        ImU32 farbe;
        Str anzeige;   // was der Anwender liest (rc588): "Kamerafahrt" statt "camera MOVE"
    };
    static const CamZeile camZeilen[] = {
        {CamSegment::Kind::Move, "MOVE", IM_COL32(96, 170, 255, 255), Str::TlCamMove},
        {CamSegment::Kind::Pan, "PAN", IM_COL32(255, 190, 80, 255), Str::TlCamPan},
        {CamSegment::Kind::Zoom, "ZOOM", IM_COL32(140, 220, 140, 255), Str::TlCamZoom},
        {CamSegment::Kind::Roll, "ROLL", IM_COL32(200, 140, 255, 255), Str::TlCamRoll},
        {CamSegment::Kind::Fade, "FADE", IM_COL32(230, 230, 230, 255), Str::TlCamFade},
        {CamSegment::Kind::Shake, "SHAKE", IM_COL32(255, 120, 120, 255), Str::TlCamShake},
        {CamSegment::Kind::Follow, "FOLLOW", IM_COL32(120, 220, 220, 255), Str::TlCamFollow},
    };
    // --- Der Spurkopf (rc588) ---------------------------------------------
    //
    // shank: "ich bin nicht zufrieden mit der timeline auch wegen den icons
    // so das alles verstaendlicher ist". Vorher stand dort grauer Text
    // ("camera MOVE"). Jetzt ein Kopf wie in Unreals Sequencer: Icon in der
    // Farbe der Spur, lesbarer Name, gewaehlt mit orangem Rand links.
    const ImU32 akzent = IM_COL32(255, 160, 50, 255);
    int zeilenNr = 0;
    // rc597: mit Einzug (Unterspuren) und Pfeil zum Auf- und Zuklappen
    // (pfeil: -1 keiner, 0 zu, 1 offen) - wie im Unreal Sequencer.
    const auto spurKopf = [&](ImVec2 p, UiIcon icon, ImU32 iconFarbe, const char* text, bool an, float einzug = 0.0F,
                              int pfeil = -1) {
        const ImVec2 e{barMin.x - 3.0F, p.y + rowH - 1.0F};
        const bool ueber = ImGui::IsMouseHoveringRect(p, e);
        const ImU32 grund = an      ? IM_COL32(64, 54, 38, 255)
                            : ueber ? IM_COL32(48, 52, 64, 255)
                                    : ((zeilenNr % 2) != 0 ? IM_COL32(33, 36, 45, 255) : IM_COL32(38, 41, 51, 255));
        tl->AddRectFilled(ImVec2{p.x + einzug, p.y + 1.0F}, e, grund, 3.0F);
        if (an) { tl->AddRectFilled(ImVec2{p.x + einzug, p.y + 1.0F}, ImVec2{p.x + einzug + 3.0F, e.y}, akzent, 1.5F); }
        float x = p.x + einzug + 7.0F;
        if (pfeil >= 0) {
            const float h = std::floor(rowH * 0.32F);
            const ImVec2 m{std::floor(x + h * 0.5F), std::floor(p.y + rowH * 0.5F)};
            const ImU32 pf = IM_COL32(200, 205, 220, 255);
            if (pfeil == 1) {
                tl->AddTriangleFilled(ImVec2{m.x - h * 0.6F, m.y - h * 0.35F}, ImVec2{m.x + h * 0.6F, m.y - h * 0.35F},
                                      ImVec2{m.x, m.y + h * 0.45F}, pf);
            } else {
                tl->AddTriangleFilled(ImVec2{m.x - h * 0.35F, m.y - h * 0.6F}, ImVec2{m.x - h * 0.35F, m.y + h * 0.6F},
                                      ImVec2{m.x + h * 0.45F, m.y}, pf);
            }
            x += h + 5.0F;
        }
        const float g = std::floor(rowH * 0.66F);
        uiIcon(tl, icon, ImVec2{std::floor(x), std::floor(p.y + (rowH - g) * 0.5F)}, g, iconFarbe);
        tl->PushClipRect(p, e, true);
        tl->AddText(ImVec2{std::floor(x + 5.0F + g), std::floor(p.y + (rowH - ImGui::GetFontSize()) * 0.5F)},
                    an ? IM_COL32(245, 245, 250, 255) : IM_COL32(190, 197, 212, 255), text);
        tl->PopClipRect();
        return ueber;
    };
    // Bloecke: abgerundet, beim Ueberfahren heller, gewaehlt weiss umrandet.
    const auto heller = [](ImU32 c) {
        const auto k = [&](int shift) {
            return static_cast<ImU32>(std::min(255, static_cast<int>((c >> shift) & 0xFFU) + 40)) << shift;
        };
        return k(IM_COL32_R_SHIFT) | k(IM_COL32_G_SHIFT) | k(IM_COL32_B_SHIFT) | (c & IM_COL32_A_MASK);
    };
    const float rund = std::max(2.0F, rowH * 0.22F);
    const auto zeilenGrund = [&](ImVec2 p) {
        tl->AddRectFilled(ImVec2{barMin.x, p.y + 1.0F}, ImVec2{barMin.x + trackW, p.y + rowH - 1.0F},
                          (zeilenNr % 2) != 0 ? IM_COL32(26, 29, 37, 255) : IM_COL32(30, 33, 42, 255));
    };
    // Eine Zeile anfangen und abschliessen (rc597, fuer Gruppen und Unterspuren).
    const auto zeilenAnfang = [&](ImVec2 p) {
        zeilenGrund(p);
        tl->PushClipRect(ImVec2{barMin.x, p.y}, ImVec2{barMin.x + trackW, p.y + rowH}, true);
    };
    const auto zeilenEnde = [&](ImVec2 p) {
        tl->AddLine(ImVec2{xAt(ablaufEnde), p.y}, ImVec2{xAt(ablaufEnde), p.y + rowH}, IM_COL32(230, 80, 80, 110), 1.0F);
        const float lx2 = xAt(g_app->playMs);
        tl->AddLine(ImVec2{lx2, p.y}, ImVec2{lx2, p.y + rowH}, akzent, 1.5F);
        tl->PopClipRect();
        ImGui::Dummy(ImVec2{w, rowH});
        ++zeilenNr;
    };
    g_app->tlUeberlappungen = 0;
    struct RoteStelle {
        float x0;
        float x1;
        ImVec2 a;   // der ganze Block, fuer den Rahmen
        ImVec2 b;
    };
    // Die Kamera als aufklappbare Gruppe (rc597): zugeklappt eine Zeile mit
    // allen Abschnitten in ihren Farben.
    if (camZeilenZahl > 0) {
        const ImVec2 kp = ImGui::GetCursorScreenPos();
        g_app->tlZeilenY.emplace_back("camera", kp.y + rowH * 0.5F);
        const bool camAn = g_app->cameraSelected;
        const bool kUeber = spurKopf(kp, UiIcon::Kamera, IM_COL32(96, 170, 255, 255), tr(Str::TlKamera), camAn, 0.0F,
                                     g_app->tlKameraOffen ? 1 : 0);
        if (kUeber) {
            ImGui::SetTooltip("%s\n%s", tr(Str::TlCamRowHint), tr(Str::TlAufklappen));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (ImGui::GetIO().MousePos.x < kp.x + rowH * 1.1F) {
                    g_app->tlKameraOffen = !g_app->tlKameraOffen;
                } else {
                    g_app->cameraSelected = true;
                    g_app->selectedActor = -1;
                    g_app->pickedEntity = -1;
                    if (g_app->selectedKey < 0) { g_app->selectedKey = schluesselFuerZeit(g_app->playMs); }
                    g_app->mapDirty = true;
                }
            }
        }
        zeilenAnfang(kp);
        for (const CamSegment& seg : t.segments) {
            ImU32 f = IM_COL32(120, 126, 145, 255);
            for (const CamZeile& cz : camZeilen) {
                if (cz.kind == seg.kind) { f = cz.farbe; }
            }
            const float sx0 = xAt(seg.startMs);
            const float sx1 = std::max(xAt(seg.endMs), sx0 + 3.0F);
            tl->AddRectFilled(ImVec2{sx0, kp.y + rowH * 0.3F}, ImVec2{sx1, kp.y + rowH * 0.7F},
                              (f & 0x00FFFFFFU) | (0xC0U << IM_COL32_A_SHIFT), 2.0F);
        }
        zeilenEnde(kp);
    }
    for (const CamZeile& cz : camZeilen) {
        if (!g_app->tlKameraOffen) { break; }
        std::vector<RoteStelle> roteStellen;
        bool hat = false;
        for (const CamSegment& seg : t.segments) {
            if (seg.kind == cz.kind) { hat = true; break; }
        }
        if (!hat) {
            continue;
        }
        const ImVec2 cp = ImGui::GetCursorScreenPos();
        char nm[24];
        std::snprintf(nm, sizeof(nm), "camera %s", cz.name);
        g_app->tlZeilenY.emplace_back(nm, cp.y + rowH * 0.5F);
        // Der NAME ist anklickbar: er waehlt die Kamera an, und damit
        // erscheint ihre Bahn in der Ansicht. Angewaehlt steht er hell.
        const bool camAn = g_app->cameraSelected;
        if (spurKopf(cp, UiIcon::Kamera, cz.farbe, tr(cz.anzeige), camAn, rowH * 0.9F)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(tr(Str::TlCamRowHint));
            ImGui::TextDisabled("%s", nm);
            ImGui::EndTooltip();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                g_app->cameraSelected = true;
                g_app->selectedActor = -1;
                g_app->pickedEntity = -1;
                if (g_app->selectedKey < 0) { g_app->selectedKey = schluesselFuerZeit(g_app->playMs); }
                g_app->mapDirty = true;
            }
        }
        zeilenGrund(cp);
        // Ab hier NUR noch im Zeitfeld zeichnen.
        //
        // Beim Zoomen liegen Bloecke ausserhalb des Ausschnitts - links
        // davon und rechts davon. Ohne Beschneiden malen sie ueber die
        // Namensspalte und ueber den rechten Rand hinaus; im Bild stand
        // "camer" statt "camera PAN", weil ein Balken darueberlag.
        tl->PushClipRect(ImVec2{barMin.x, cp.y},
                         ImVec2{barMin.x + trackW, cp.y + rowH}, true);
        for (const CamSegment& seg : t.segments) {
            if (seg.kind != cz.kind) {
                continue;
            }
            const float sx0 = xAt(seg.startMs);
            // Wird gerade an DIESEM Block gezogen? Dann folgt seine rechte
            // Kante der Maus statt dem Wert aus dem Skript.
            const bool zieht =
                g_app->tlDragging && g_app->tlDragPath == seg.path;
            const float sx1 = xAt(zieht ? g_app->tlDragEndMs : seg.endMs);
            const ImVec2 a{sx0, cp.y + 2.0F};
            const ImVec2 b{std::max(sx1, sx0 + 3.0F), cp.y + rowH - 2.0F};
            const bool blockUeber = ImGui::IsMouseHoveringRect(a, ImVec2{b.x + 2.0F, b.y});
            tl->AddRectFilled(a, b, blockUeber || zieht ? heller(cz.farbe) : cz.farbe, rund);
            if (!seg.path.empty() && seg.path == g_app->selectedPath) {
                tl->AddRect(ImVec2{a.x - 1.0F, a.y - 1.0F}, ImVec2{b.x + 1.0F, b.y + 1.0F}, IM_COL32(255, 255, 255, 255),
                            rund, 0, 2.0F);
            }
            // --- Laeuft er in den naechsten hinein? ----------------------
            //
            // Die Engine uebernimmt Ort/Winkel/Bildwinkel erst, wenn ein
            // Uebergang FERTIG ist; ein neuer, der vorher beginnt, rechnet
            // vom Anfang des alten - im Spiel springt die Kamera zurueck.
            // Rot schraffiert, was sich ueberlappt, auch schon beim Ziehen.
            const double ende = zieht ? g_app->tlDragEndMs : seg.endMs;
            const CamSegment* naechster = nullptr;
            const auto gleicheGruppe = [&](const CamSegment& o) {
                const bool schwenkA = seg.kind == CamSegment::Kind::Pan ||
                                      (seg.kind == CamSegment::Kind::Roll && seg.alleAchsen);
                const bool schwenkB = o.kind == CamSegment::Kind::Pan ||
                                      (o.kind == CamSegment::Kind::Roll && o.alleAchsen);
                return schwenkA ? schwenkB : (o.kind == seg.kind);
            };
            if (seg.kind == CamSegment::Kind::Move || seg.kind == CamSegment::Kind::Pan ||
                seg.kind == CamSegment::Kind::Zoom || seg.kind == CamSegment::Kind::Roll) {
                for (const CamSegment& o : t.segments) {
                    if (&o != &seg && gleicheGruppe(o) && o.startMs > seg.startMs + 0.5 &&
                        (naechster == nullptr || o.startMs < naechster->startMs)) {
                        naechster = &o;
                    }
                }
            }
            const bool ueberlappt = naechster != nullptr && naechster->startMs < ende - 0.5;
            if (ueberlappt) {
                // Erst NACH allen Bloecken der Zeile zeichnen - sonst liegt der
                // naechste Block darueber, und die Schraffur ist verdeckt.
                ++g_app->tlUeberlappungen;
                roteStellen.push_back({std::max(a.x, xAt(naechster->startMs)), b.x, a, b});
            }
            // --- Beschriftung: wohin, welcher Winkel, welche Marke --------
            if (b.x - a.x > ImGui::GetFontSize() * 2.0F) {
                if (const Node* cn = nodeAt(g_app->doc.script(), seg.path);
                    cn != nullptr && cn->args.size() >= 2) {
                    std::string was = cn->args[1].text;
                    // tag( "cam1b", ORIGIN ) -> cam1b
                    const std::size_t q1 = was.find('"');
                    const std::size_t q2 = (q1 == std::string::npos) ? q1 : was.find('"', q1 + 1);
                    if (q1 != std::string::npos && q2 != std::string::npos && was.find("tag") != std::string::npos) {
                        was = was.substr(q1 + 1, q2 - q1 - 1);
                    } else {
                        // Zahlen kuerzen: "-1762.000 -146.000 1076.000" -> "-1762 -146 1076"
                        std::string kurz;
                        std::istringstream zs(was);
                        std::string teil;
                        while (zs >> teil) {
                            char* endp = nullptr;
                            const double v = std::strtod(teil.c_str(), &endp);
                            if (endp != nullptr && *endp == '\0') {
                                char b2[24];
                                std::snprintf(b2, sizeof(b2), "%g", std::round(v * 10.0) / 10.0);
                                teil = b2;
                            }
                            if (teil != "<" && teil != ">") {
                                if (!kurz.empty()) { kurz += " "; }
                                kurz += teil;
                            }
                        }
                        was = kurz;
                    }
                    tl->PushClipRect(a, b, true);
                    tl->AddText(ImVec2{a.x + 3.0F, a.y}, IM_COL32(16, 18, 24, 230), was.c_str());
                    tl->PopClipRect();
                }
            }
            kantenGriff(seg.path, seg.startMs,
                        zieht ? g_app->tlDragEndMs : seg.endMs, a, b,
                        sichtSpanne / std::max(trackW, 1.0F));
            tl->AddLine(ImVec2{a.x, a.y}, ImVec2{a.x, b.y},
                        IM_COL32(20, 22, 28, 255), 1.0F);
            if (ImGui::IsMouseHoveringRect(a, ImVec2{b.x + 2.0F, b.y})) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", tr(cz.anzeige));
                ImGui::SameLine();
                ImGui::TextDisabled("%s", nm);
                if (ueberlappt) {
                    ImGui::TextColored(ImVec4{1.0F, 0.45F, 0.45F, 1.0F}, tr(Str::TlOverlap), cz.name,
                                       naechster->startMs / 1000.0);
                }
                if (g_app->timelineFrames) {
                    const double fps =
                        static_cast<double>(std::max(1, g_app->timelineFps));
                    ImGui::TextDisabled(
                        tr(Str::TlSpanFrames),
                        static_cast<int>(seg.startMs / 1000.0 * fps),
                        static_cast<int>(seg.endMs / 1000.0 * fps));
                } else {
                    ImGui::TextDisabled(tr(Str::TlSpanSeconds),
                                        seg.startMs / 1000.0,
                                        seg.endMs / 1000.0);
                }
                ImGui::EndTooltip();
                if (!g_app->tlDragging &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // Zum Zeitpunkt springen UND die Kamera anwaehlen -
                    // sonst sieht man zwar die Stellung, aber nicht die
                    // Bahn, auf der sie dorthin kommt. Ausgefuehrt am Ende
                    // der Spuren (tlKlickKandidat).
                    App::TlKandidat& k = g_app->tlKlickKandidat;
                    k.da = true;
                    k.pfad = seg.path;
                    k.startMs = seg.startMs;
                    k.kamera = true;
                    k.figur = -1;
                }
            }
        }
        // Die Ueberlappungen OBENAUF: rot schraffiert, der Block rot umrandet.
        // Seit rc588 ruhiger: der ueberlappende Teil leicht rot, unten ein
        // roter Strich und ein Warnzeichen - vorher eine grelle Schraffur, die
        // aussah wie ein Fehler im Programm statt wie ein Hinweis aufs Skript.
        for (const RoteStelle& rs : roteStellen) {
            const ImU32 rot = IM_COL32(235, 70, 70, 255);
            tl->PushClipRect(ImVec2{rs.x0, rs.a.y}, ImVec2{rs.x1, rs.b.y}, true);
            tl->AddRectFilled(ImVec2{rs.x0, rs.a.y}, ImVec2{rs.x1, rs.b.y}, IM_COL32(235, 70, 70, 70));
            tl->AddRectFilled(ImVec2{rs.x0, rs.b.y - 3.0F}, ImVec2{rs.x1, rs.b.y}, rot);
            const float g = std::floor(rowH * 0.6F);
            if (rs.x1 - rs.x0 > g + 4.0F) {
                uiIcon(tl, UiIcon::Warnung, ImVec2{std::floor(rs.x0 + 2.0F), std::floor(rs.a.y + 1.0F)}, g,
                       IM_COL32(255, 200, 80, 255));
            }
            tl->PopClipRect();
        }
        const float clx = xAt(g_app->playMs);
        tl->AddLine(ImVec2{clx, cp.y}, ImVec2{clx, cp.y + rowH}, akzent, 1.5F);
        tl->PopClipRect();
        ImGui::Dummy(ImVec2{w, rowH});
        ++zeilenNr;
    }

    const int auswahlFigur = g_app->pickedEntity >= 0 ? figurFuerEntity(g_app->pickedEntity) : -1;
    g_app->tlAuswahlZeile.clear();
    g_app->tlBewegungsBalken = 0;
    g_app->tlAusloeser = 0;
    g_app->tlBalken.clear();
    g_app->tlWaitBloecke = 0;
    g_app->tlZeilenH = rowH;
    g_app->tlKopfX = ImGui::GetCursorScreenPos().x;
    if (auswahlFigur < 0 && g_app->selectedActor < 0) { g_app->tlRollFigur = -1; }
    const auto zeichneEreignis = [&](const TimelineEvent& e, const ImVec2 p0, const int figIdx,
                                     const TimelineTrack& track) {
        const float x0 = barMin.x;
        (void)x0;
        if (e.kind == TimelineEvent::Kind::Wait) { ++g_app->tlWaitBloecke; }
            // Die Kamerabefehle des Skripts haben ihre eigenen Zeilen oben -
            // in der Script-Zeile lagen sie noch einmal, UNTER den waits.
            if (track.entity.empty() && e.kind == TimelineEvent::Kind::Camera) {
                return;
            }
            // Nur zeigen, was beim Abstimmen zaehlt. Ein SET_PARM ist
            // richtig, aber uninteressant; ein Klang und eine Bewegung
            // nicht.
            // Jede Art ihre Farbe - auch Warten und Setzen. Vorher fielen
            // die unter "default: continue", und damit sah eine Spur wie
            // ein einziger langer Balken aus: man sah, DASS etwas laeuft,
            // aber nicht WAS und nicht, wo eines aufhoert und das naechste
            // anfaengt.
            ImU32 col = 0;
            switch (e.kind) {
                case TimelineEvent::Kind::Sound:
                    col = IM_COL32(255, 120, 200, 255);
                    break;
                // Ein use, das einen Klang ausloest, bekommt dieselbe
                // Farbe wie ein sound - fuer den Anwender ist es dasselbe:
                // hier faengt etwas an zu klingen. Ein use ohne Klang
                // (Tuer, Ausloeser, Speicherpunkt) bleibt gedeckt.
                case TimelineEvent::Kind::Use:
                    col = useSoundFor(e.target, nullptr).empty()
                              ? IM_COL32(120, 130, 150, 255)
                              : IM_COL32(255, 120, 200, 255);
                    break;
                // Die Klinge klingt immer - notfalls mit der Vorgabe.
                case TimelineEvent::Kind::Saber:
                    col = IM_COL32(255, 120, 200, 255);
                    break;
                case TimelineEvent::Kind::Camera:
                    col = IM_COL32(96, 170, 255, 255);
                    break;
                case TimelineEvent::Kind::Move:
                    col = IM_COL32(140, 220, 140, 255);
                    break;
                case TimelineEvent::Kind::Wait:
                    // Warten gedeckt und dunkel: es ist eine Luecke, keine
                    // Handlung - aber eine, die man SEHEN will, weil sie
                    // den Takt bestimmt.
                    col = IM_COL32(70, 74, 90, 255);
                    break;
                case TimelineEvent::Kind::Set:
                    col = IM_COL32(190, 160, 90, 255);
                    break;
                default:
                    col = IM_COL32(110, 116, 132, 255);
                    break;
            }
            // Aus einem ANDEREN Skript (run, use, spawnscript): blasser -
            // man sieht, dass es passiert, aber es ist nicht diese Datei.
            if (!e.fremd.empty()) {
                col = (col & 0x00FFFFFFU) | (0x70U << IM_COL32_A_SHIFT);
            }
            const float ex0 = xAt(e.startMs);
            const bool zieht = g_app->tlDragging && g_app->tlDragPath == e.path;
            const float ex1 = xAt(zieht ? g_app->tlDragEndMs : e.endMs);
            // Mindestens drei Bildpunkte breit, damit ein Befehl ohne Dauer
            // (ein Klang, ein set) ueberhaupt zu treffen ist.
            const ImVec2 a{ex0, p0.y + 2.0F};
            const ImVec2 b{std::max(ex1, ex0 + 3.0F), p0.y + rowH - 2.0F};
            const bool blockUeber = ImGui::IsMouseHoveringRect(a, ImVec2{b.x + 2.0F, b.y});
            tl->AddRectFilled(a, b, blockUeber || zieht ? heller(col) : col, rund);
            if (!e.path.empty() && e.fremd.empty() && e.path == g_app->selectedPath) {
                tl->AddRect(ImVec2{a.x - 1.0F, a.y - 1.0F}, ImVec2{b.x + 1.0F, b.y + 1.0F}, IM_COL32(255, 255, 255, 255),
                            rund, 0, 2.0F);
            }
            kantenGriff(e.path, e.startMs,
                        zieht ? g_app->tlDragEndMs : e.endMs, a, b,
                        sichtSpanne / std::max(trackW, 1.0F));
            // Eine dunkle Kante an den Anfang: damit zwei Bloecke, die
            // aneinanderstossen, als ZWEI zu erkennen sind. Ohne sie
            // verschmelzen sie zu einem Balken.
            tl->AddLine(ImVec2{a.x, a.y}, ImVec2{a.x, b.y},
                        IM_COL32(20, 22, 28, 255), 1.0F);

            // Die Beschriftung hinein, wenn Platz ist. Das ist der
            // Unterschied zwischen "da laeuft etwas" und "da laeuft PAN".
            if (b.x - a.x > ImGui::GetFontSize() * 2.0F && !e.label.empty()) {
                tl->PushClipRect(a, b, true);
                tl->AddText(ImVec2{a.x + 3.0F, a.y}, IM_COL32(16, 18, 24, 230),
                            e.label.c_str());
                tl->PopClipRect();
            }

            // Ueberfahren zeigt Anfang UND Ende - in der Einheit, die
            // gerade eingestellt ist.
            if (ImGui::IsMouseHoveringRect(a, ImVec2{b.x + 2.0F, b.y})) {
                ImGui::BeginTooltip();
                if (g_app->timelineFrames) {
                    const double fps =
                        static_cast<double>(std::max(1, g_app->timelineFps));
                    ImGui::Text("%s", e.label.c_str());
                    ImGui::TextDisabled(
                        tr(Str::TlSpanFrames),
                        static_cast<int>(e.startMs / 1000.0 * fps),
                        static_cast<int>(e.endMs / 1000.0 * fps));
                } else {
                    ImGui::Text("%s", e.label.c_str());
                    ImGui::TextDisabled(tr(Str::TlSpanSeconds),
                                        e.startMs / 1000.0, e.endMs / 1000.0);
                }
                if (!e.sound.empty()) {
                    ImGui::TextDisabled("%s", e.sound.c_str());
                }
                if (!e.fremd.empty()) {
                    ImGui::TextColored(ImVec4{1.0F, 0.8F, 0.4F, 1.0F}, tr(Str::TlFromScript), e.fremd.c_str());
                }
                ImGui::TextDisabled("%s", tr(Str::TlClickHint));
                ImGui::EndTooltip();
                // Anklicken springt zur Zeile im Skript UND setzt den
                // Zeiger dorthin. Damit wird aus der Anzeige ein Werkzeug:
                // man sucht in der Leiste und landet im Text.
                // NICHT springen, wenn gerade an einer Kante gezogen wird.
                // Der Griff liegt im Block, und ein Klick darauf soll die
                // Dauer aendern - nicht zugleich die Ansicht umstellen.
                if (!g_app->tlDragging &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    App::TlKandidat& k = g_app->tlKlickKandidat;
                    k.da = true;
                    k.pfad = e.path;
                    k.startMs = e.startMs;
                    k.kamera = false;
                    k.figur = figIdx;
                }
            }
    };
    const auto zeichneBewegung = [&](const TimelineTrack& track, const ImVec2 p0, const int figIdx,
                                     const bool figAn) {
        (void)track;
        // Unterwegs (rc597): je Gehen (SET_NAVGOAL) und Fliegen (move) ein
        // Band am unteren Rand der Zeile, kraeftig bei der gewaehlten Figur.
        // Anfassen und ziehen verschiebt den Befehl in der Zeit (das wait
        // davor, wie bei jedem Block); Rechtsklick auf einen Lauf: gehen oder
        // rennen. Der ausloesende Befehl bekommt einen hellen Rahmen.
        if (figIdx >= 0 && static_cast<std::size_t>(figIdx) < g_app->scene.actors.size()) {
            const float hb = std::max(4.0F, rowH * 0.28F);
            const ImU32 tuerkis = IM_COL32(90, 220, 190, 255);
            const std::vector<ActorStep>& schritte = g_app->scene.actors[static_cast<std::size_t>(figIdx)].steps;
            for (std::size_t si = 0; si < schritte.size(); ++si) {
                const ActorStep& st = schritte[si];
                const bool lauf = st.kind == ActorStep::Kind::WalkTo;
                // rc599: auch die ROFF-Bahnen (play ( "PLAY_ROFF", ... )).
                const bool roff = st.kind == ActorStep::Kind::Roff;
                if ((!lauf && !roff && st.kind != ActorStep::Kind::Glide) || !(st.endMs > st.startMs)) { continue; }
                // rc601 (bewegung): ein Weg kann aus mehreren Stuecken
                // bestehen (anderes Tempo unterwegs, scene.cpp) - EIN Band je
                // SET_NAVGOAL, vom ersten Stueck bis zum letzten.
                double bandEnde = st.endMs;
                float weg = 0.0F;
                {
                    float w2 = 0.0F;
                    for (int k = 0; k < 3; ++k) { w2 += (st.to[k] - st.from[k]) * (st.to[k] - st.from[k]); }
                    weg = std::sqrt(w2);
                }
                if (lauf) {
                    bool fortsetzung = false;
                    for (std::size_t sj = 0; sj < si; ++sj) {
                        const ActorStep& v = schritte[sj];
                        fortsetzung = fortsetzung || (v.kind == ActorStep::Kind::WalkTo && v.teilweg &&
                                                      v.path == st.path && std::fabs(v.endMs - st.startMs) < 1.0);
                    }
                    if (fortsetzung) { continue; }
                    for (std::size_t sj = si + 1; sj < schritte.size(); ++sj) {
                        const ActorStep& v = schritte[sj];
                        if (v.kind == ActorStep::Kind::WalkTo && v.path == st.path && std::fabs(v.startMs - bandEnde) < 1.0) {
                            bandEnde = v.endMs;
                            float w2 = 0.0F;
                            for (int k = 0; k < 3; ++k) { w2 += (v.to[k] - v.from[k]) * (v.to[k] - v.from[k]); }
                            weg += std::sqrt(w2);
                        }
                    }
                }
                const ImVec2 a{xAt(st.startMs), p0.y + rowH - 2.0F - hb};
                const ImVec2 b{std::max(xAt(bandEnde), a.x + 3.0F), p0.y + rowH - 2.0F};
                const bool ueber = ImGui::IsMouseHoveringRect(a, b);
                const bool istZiel = !st.path.empty() && st.path == g_app->selectedPath;
                ImU32 f = figAn ? IM_COL32(90, 220, 190, 235) : IM_COL32(90, 200, 170, 110);
                if (ueber || istZiel) { f = IM_COL32(150, 245, 220, 255); }
                tl->AddRectFilled(a, b, f, 2.0F);
                g_app->tlBalken.push_back(App::TlBalken{st.path, a, b, lauf, figIdx});
                if (ueber) {
                    ImGui::BeginTooltip();
                    if (roff) {
                        ImGui::Text("ROFF  %s  -  %.2f s, %.0f", st.text.c_str(), (st.endMs - st.startMs) / 1000.0,
                                    static_cast<double>(weg));
                    } else {
                        ImGui::Text(tr(Str::TlBewegung),
                                    tr(lauf ? (st.text == "BOTH_WALK1" ? Str::BahnGehtJetzt : Str::BahnRenntJetzt) : Str::TlFlug),
                                    (bandEnde - st.startMs) / 1000.0, static_cast<double>(weg));
                    }
                    if (lauf) { ImGui::TextDisabled("%s", tr(Str::BahnZielMenue)); }
                    ImGui::EndTooltip();
                    if (!st.path.empty() && st.fremd.empty() && !g_app->tlDragging && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        App::TlKandidat& k = g_app->tlKlickKandidat;
                        k.da = true;
                        k.pfad = st.path;
                        k.startMs = st.startMs;
                        k.kamera = false;
                        k.figur = figIdx;
                    }
                    if (lauf && !st.path.empty() && st.fremd.empty() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                        g_app->bahnMenuePfad = st.path;
                        ImGui::OpenPopup("##gangart_tl");
                    }
                }
                if (!figAn) { continue; }
                ++g_app->tlBewegungsBalken;
                for (const TimelineEvent& e : track.events) {
                    if (st.path.empty() || e.path != st.path) { continue; }
                    const float ax = xAt(e.startMs);
                    const float bx = std::max(xAt(e.endMs), ax + 3.0F);
                    tl->AddRect(ImVec2{ax - 2.0F, p0.y + 1.0F}, ImVec2{bx + 2.0F, p0.y + rowH - 1.0F}, tuerkis, rund, 0, 2.0F);
                    tl->AddTriangleFilled(ImVec2{ax - 4.0F, p0.y + 1.0F}, ImVec2{ax + 4.0F, p0.y + 1.0F},
                                          ImVec2{ax, p0.y + 6.0F}, tuerkis);
                    ++g_app->tlAusloeser;
                    break;
                }
            }
        }
    };
    for (const TimelineTrack& track : g_app->timeline.tracks) {
        if (!g_app->zeigeRuhigeSpuren && spurIstRuhig(track)) {
            continue;
        }
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const std::string name = track.entity.empty() ? std::string(tr(Str::TlScript))
                                                      : track.entity;
        g_app->tlZeilenY.emplace_back(name, p0.y + rowH * 0.5F);
        // Der Name waehlt die FIGUR an - dann zeigt die Ansicht ihren
        // Laufweg, so wie sie es fuer die Kamera schon tut.
        int figIdx = -1;
        if (!track.entity.empty()) {
            for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
                if (g_app->scene.actors[ai].name == track.entity) {
                    figIdx = static_cast<int>(ai);
                    break;
                }
            }
        }
        // Gewaehlt ist die Figur auch, wenn ihre ENTITY gewaehlt ist (Spawner,
        // Schiff) - rc597.
        const bool figAn = (figIdx >= 0 && (figIdx == g_app->selectedActor || figIdx == auswahlFigur));
        if (figAn) {
            g_app->tlAuswahlZeile = name;
            if (g_app->tlRollFigur != figIdx) { g_app->tlOffen.insert(name); }   // neu gewaehlt: aufklappen
            // Neu gewaehlt? Dann die Zeile ins Bild rollen - sonst lag sie oft
            // unter Kamera- und Skriptzeile verborgen (rc597).
            if (g_app->tlRollFigur != figIdx) {
                ImGui::SetScrollHereY(0.4F);
                g_app->tlRollFigur = figIdx;
            }
        }
        const bool skript = track.entity.empty();
        const bool offen = !skript && g_app->tlOffen.count(name) != 0U;
        const bool kopfUeber = spurKopf(p0, skript ? UiIcon::Skript : UiIcon::Figur,
                                        ImGui::GetColorU32(uiIconFarbe(skript ? UiIcon::Skript : UiIcon::Figur)),
                                        name.c_str(), figAn, 0.0F, skript ? -1 : (offen ? 1 : 0));
        if (!skript && kopfUeber && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            ImGui::GetIO().MousePos.x < p0.x + rowH * 1.1F) {
            // Der Pfeil klappt auf und zu.
            if (offen) { g_app->tlOffen.erase(name); } else { g_app->tlOffen.insert(name); }
        } else if (figIdx >= 0 && kopfUeber) {
            ImGui::SetTooltip("%s\n%s", tr(Str::TlActorHint), tr(Str::TlAufklappen));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // Nochmal derselbe Name hebt die Auswahl auf - dieselbe
                // Regel wie im Skriptbaum seit rc167.
                g_app->selectedActor = figAn ? -1 : figIdx;
                g_app->cameraSelected = false;
                g_app->mapDirty = true;
            }
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (offen) { g_app->tlOffen.erase(name); } else { g_app->tlOffen.insert(name); }
            }
        }
        zeilenAnfang(p0);
        if (skript) {
            // Die Skriptzeile: ihre Befehle, die waits nur auf Wunsch.
            for (const TimelineEvent& e : track.events) {
                if (kategorieVon(e) == 5 && !g_app->tlWaitsZeigen && e.kind == TimelineEvent::Kind::Wait) { continue; }
                zeichneEreignis(e, p0, figIdx, track);
            }
        } else {
            // Zusammenfassung wie im Sequencer: wo etwas passiert, in der
            // Farbe seiner Spur. Anklicken waehlt den Befehl.
            for (const TimelineEvent& e : track.events) {
                const int k = kategorieVon(e);
                if (k == 5 && !g_app->tlWaitsZeigen) { continue; }
                const float ax = xAt(e.startMs);
                const float bx = std::max(xAt(e.endMs), ax + 3.0F);
                const ImVec2 a{ax, p0.y + rowH * 0.28F};
                const ImVec2 b{bx, p0.y + rowH * 0.72F};
                const bool ueber = ImGui::IsMouseHoveringRect(ImVec2{a.x - 1.0F, p0.y}, ImVec2{b.x + 1.0F, p0.y + rowH});
                ImU32 f = kKat[static_cast<std::size_t>(k)].farbe;
                if (!ueber) { f = (f & 0x00FFFFFFU) | (0xB0U << IM_COL32_A_SHIFT); }
                tl->AddRectFilled(a, b, f, 2.0F);
                if (!e.path.empty() && e.fremd.empty() && e.path == g_app->selectedPath) {
                    tl->AddRect(ImVec2{a.x - 1.0F, a.y - 1.0F}, ImVec2{b.x + 1.0F, b.y + 1.0F}, IM_COL32(255, 255, 255, 255), 2.0F, 0, 1.5F);
                }
                if (ueber) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(e.label.c_str());
                    ImGui::TextDisabled(tr(Str::TlSpanSeconds), e.startMs / 1000.0, e.endMs / 1000.0);
                    ImGui::EndTooltip();
                    if (!e.path.empty() && !g_app->tlDragging && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        App::TlKandidat& kk = g_app->tlKlickKandidat;
                        kk.da = true;
                        kk.pfad = e.path;
                        kk.startMs = e.startMs;
                        kk.kamera = false;
                        kk.figur = figIdx;
                    }
                }
            }
            if (!offen) { zeichneBewegung(track, p0, figIdx, figAn); }
        }
        zeilenEnde(p0);
        // Aufgeklappt: je Art eine Unterspur.
        if (offen) {
            const std::array<bool, 6> hatKat = katDerSpur(track);
            for (int k = 0; k < 6; ++k) {
                if (!hatKat[static_cast<std::size_t>(k)]) { continue; }
                const KatInfo& ki = kKat[static_cast<std::size_t>(k)];
                const ImVec2 q = ImGui::GetCursorScreenPos();
                g_app->tlZeilenY.emplace_back(name + "/" + tr(ki.name), q.y + rowH * 0.5F);
                (void)spurKopf(q, ki.icon, ki.farbe, tr(ki.name), figAn, rowH * 0.9F);
                zeilenAnfang(q);
                for (const TimelineEvent& e : track.events) {
                    if (kategorieVon(e) == k) { zeichneEreignis(e, q, figIdx, track); }
                }
                if (k == 0) { zeichneBewegung(track, q, figIdx, figAn); }
                zeilenEnde(q);
            }
        }
    }

    if (ImGui::BeginPopup("##gangart_tl")) {
        gangartMenueInhalt();
        ImGui::EndPopup();
    }
    // --- Klick und Kantengriff: der OBERSTE Block gewinnt -----------------
    //
    // Beide wurden beim Zeichnen nur vorgemerkt; der zuletzt gezeichnete
    // Block liegt oben. Eine gepackte Kante geht vor dem Klick auf den Block.
    if (g_app->tlGriffKandidat.da) {
        const App::TlKandidat k = g_app->tlGriffKandidat;
        g_app->tlDragging = true;
        g_app->tlDragPath = k.pfad;
        g_app->tlDragStartMs = k.startMs;
        g_app->tlDragEndMs = k.endMs;
        const Node* gn = nodeAt(g_app->doc.script(), k.pfad);
        diag::detail("Zeitleiste: Kante von \"" + std::string(gn != nullptr ? gn->name : "?") + "\" gepackt (" +
                     std::to_string(k.startMs) + " - " + std::to_string(k.endMs) + " ms)");
    } else if (g_app->tlKlickKandidat.da) {
        const App::TlKandidat k = g_app->tlKlickKandidat;
        // Anklicken springt zur Zeile im Skript UND setzt den Zeiger dorthin -
        // auch bei einem Kamerablock (vorher nur bei den Spuren).
        if (!k.pfad.empty()) {
            selectByPath(k.pfad);
        }
        if (k.kamera) {
            g_app->cameraSelected = true;
            g_app->selectedActor = -1;
        } else if (k.figur >= 0) {
            g_app->selectedActor = k.figur;
            g_app->cameraSelected = false;
        }
        g_app->playMs = k.startMs;
        g_app->playing = false;
        g_app->audioUpTo = k.startMs;
        g_app->audio.stopAll();
        g_app->laufendeSchleifen.clear();
        tonZuruecksetzen();   // rc601 ton: auch der Raumklang
        g_app->musicAudio.stopAll();
        g_app->mapDirty = true;
        // Vielleicht wird daraus ein Verschieben: merken, wo es anfing.
        if (!k.pfad.empty()) {
            g_app->tlZiehPfad = k.pfad;
            g_app->tlZiehMausX = ImGui::GetIO().MousePos.x;
            g_app->tlZiehStartMs = k.startMs;
            g_app->tlZiehMsJePx = sichtSpanne / std::max(1.0, static_cast<double>(trackW));
            g_app->tlZiehAktiv = false;
            g_app->tlZiehDelta = 0.0;
        }
    }
    g_app->tlGriffKandidat = App::TlKandidat{};
    g_app->tlKlickKandidat = App::TlKandidat{};

    // --- Einen Block verschieben -------------------------------------------
    //
    // shank: "hast du eingebaut, dass man die Clips unten nehmen und
    // verschieben kann?" Jetzt ja. Ein ICARUS-Skript ist eine Folge: ein
    // Befehl beginnt, wenn das wait davor abgelaufen ist. Einen Block
    // verschieben heisst also, dieses wait laenger oder kuerzer zu machen -
    // alles danach rueckt mit (Ripple, wie im Sequencer). Mit Alt bleibt der
    // Rest stehen: das wait danach wird um dasselbe kuerzer. Im Raster der
    // Spiellogik (50 ms).
    if (!g_app->tlZiehPfad.empty()) {
        const float dx = ImGui::GetIO().MousePos.x - g_app->tlZiehMausX;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!g_app->tlZiehAktiv && std::fabs(dx) > 4.0F && !g_app->tlDragging) {
                g_app->tlZiehAktiv = true;
            }
            if (g_app->tlZiehAktiv) {
                g_app->tlZiehDelta = std::round(static_cast<double>(dx) * g_app->tlZiehMsJePx / 50.0) * 50.0;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                const float gx = xAt(g_app->tlZiehStartMs + g_app->tlZiehDelta);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const ImVec2 fo = ImGui::GetWindowPos();
                const ImVec2 fg = ImGui::GetWindowSize();
                dl->AddLine(ImVec2{gx, fo.y}, ImVec2{gx, fo.y + fg.y}, IM_COL32(255, 210, 90, 230), 2.0F);
                ImGui::BeginTooltip();
                ImGui::Text(tr(Str::TlMoveDelta), g_app->tlZiehDelta / 1000.0);
                ImGui::TextDisabled("%s", tr(ImGui::GetIO().KeyAlt ? Str::TlMoveSlide : Str::TlMoveRipple));
                ImGui::EndTooltip();
            }
        } else {
            if (g_app->tlZiehAktiv && std::fabs(g_app->tlZiehDelta) >= 1.0) {
                verschiebeImAblauf(g_app->tlZiehPfad, g_app->tlZiehDelta, ImGui::GetIO().KeyAlt);
            }
            g_app->tlZiehPfad.clear();
            g_app->tlZiehAktiv = false;
            g_app->tlZiehDelta = 0.0;
        }
    }

    // --- Bildschritt per Tastatur (wie in jedem Schnittprogramm) ----------
    //
    // Links/rechts ein Spielbild (50 ms), mit Umschalt eine Sekunde, Pos1
    // und Ende an Anfang und Ende. Nur, solange die Maus ueber der Leiste
    // steht - sonst gehoeren die Pfeile dem Baum (handleTreeKeys).
    g_app->tlUeberMaus = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if (g_app->tlUeberMaus && !ImGui::GetIO().WantTextInput) {
        const double schritt = ImGui::GetIO().KeyShift ? 1000.0 : 50.0;
        double ziel = g_app->playMs;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) { ziel += schritt; }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) { ziel -= schritt; }
        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) { ziel = 0.0; }
        if (ImGui::IsKeyPressed(ImGuiKey_End, false)) { ziel = t.durationMs; }
        ziel = std::clamp(ziel, 0.0, std::max(0.0, t.durationMs));
        if (ziel != g_app->playMs) {
            g_app->playMs = ziel;
            g_app->playing = false;
            g_app->audioUpTo = ziel;
            g_app->audio.stopAll();
            g_app->laufendeSchleifen.clear();
            tonZuruecksetzen();   // rc601 ton: auch der Raumklang
            g_app->musicAudio.stopAll();
            g_app->mapDirty = true;
        }
    }

    // --- Zoomen auch UEBER DEN SPUREN ------------------------------------
    //
    // Dort verbringt man die Zeit, nicht auf dem Lineal. Mit STRG, damit
    // das Rad allein weiterhin senkrecht rollt - bei vielen Spuren braucht
    // man das.
    //
    // Dieselbe Rechnung wie am Lineal: vergroessert wird um den Punkt unter
    // der Maus.
    if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl) {
        const float rad2 = ImGui::GetIO().MouseWheel;
        if (rad2 != 0.0F) {
            const float mausX = ImGui::GetIO().MousePos.x;
            const double anteil = std::clamp<double>(
                (mausX - barMin.x) / std::max(trackW, 1.0F), 0.0, 1.0);
            const double unterMaus = sichtStart + anteil * sichtSpanne;
            const double neu = std::clamp(
                g_app->timelineZoom * std::pow(1.25, rad2), 1.0, App::kMaxZoom);
            const double neueSpanne = span / neu;
            g_app->timelineZoom = neu;
            g_app->timelineViewStartMs =
                std::clamp(unterMaus - anteil * neueSpanne, 0.0,
                           std::max(0.0, span - neueSpanne));
        }
    }
    ImGui::EndChild();
}

// Die Seitenleiste, RECHTS neben der Ansicht.
//
// EIN Helfer fuer beide Faelle - mit und ohne Karte. Vorher gab es zwei
// Fassungen: die richtige am Ende von drawMapView, und eine Notloesung im
// Zweig "keine Karte geladen", die die Einstellungen quer ueber die ganze
// Flaeche legte. Gemeldet als "die version wo die map nicht geladen wird
// ist komplett falsch".
//
// Zwei Fassungen desselben Dings laufen immer auseinander. Jetzt gibt es
// nur noch eine, und der leere Fall sieht aus wie der volle.
// Wie breit ist die Leiste? Dieselbe Rechnung fuer beide Faelle.
float mapSidebarWidth(float verfuegbar) {
    // Seit rc586 gibt es die Leiste nicht mehr - die Einstellungen stehen als
    // Icons in der Kopfzeile (ansichtsSchalter). Das Bild bekommt die Breite.
    (void)verfuegbar;
    return 0.0F;
}

// --- Der bearbeitete Schluessel geht mit der Zeitleiste ---------------------
//
// Ist die Kamera angewaehlt und Verschieben oder Drehen an, gilt immer der
// Schluessel der eingestellten ZEIT (der letzte MOVE, der begonnen hat).
// Zieht man die Zeitleiste, wandert er mit - ausser es steht eine noch
// nicht geschriebene Aenderung an; die bleibt an ihrem Schluessel, bis sie
// geschrieben oder verworfen ist. Waehrend eines Zuges am Gizmo nie.
void schluesselMitZeit() {
    const std::size_t anzahl = kameraSchluessel(g_app->doc.script()).size();
    if (g_app->selectedKey >= 0 && static_cast<std::size_t>(g_app->selectedKey) >= anzahl) {
        // Die Zeile gibt es nicht mehr (geloescht, Rueckgaengig).
        g_app->selectedKey = anzahl == 0 ? -1 : static_cast<int>(anzahl) - 1;
        for (int k = 0; k < 3; ++k) {
            g_app->gizmoOffset[k] = 0.0F;
            g_app->gizmoAngles[k] = 0.0F;
        }
    }
    if (!g_app->cameraSelected || g_app->gizmoMode == App::GizmoMode::Select ||
        g_app->gizmoAxis >= 0 || g_app->gizmoRotating ||
        std::fabs(g_app->playMs - g_app->gizmoZeitBezug) <= 0.5) {
        return;
    }
    g_app->gizmoZeitBezug = g_app->playMs;
    const bool offen = g_app->gizmoOffset[0] != 0.0F || g_app->gizmoOffset[1] != 0.0F ||
                       g_app->gizmoOffset[2] != 0.0F || g_app->gizmoAngles[0] != 0.0F ||
                       g_app->gizmoAngles[1] != 0.0F || g_app->gizmoAngles[2] != 0.0F;
    if (offen) {
        return;
    }
    const int k = schluesselFuerZeit(g_app->playMs);
    if (k != g_app->selectedKey) {
        g_app->selectedKey = k;
        g_app->mapDirty = true;
    }
}

void spurenVerteilt(std::size_t n, const std::function<void(std::size_t)>& je) {
    BHED_MESS("Strahltests");
    if (n == 0U) {
        return;
    }
    // Der erste vorweg: beim ersten Strahl entsteht die Spurhilfe der Karte,
    // und darauf sollen nicht alle Faeden zugleich warten.
    je(0U);
    // Der Rest auf die festen Arbeitsfaeden (bhed/arbeiter.h, rc596) - jeder
    // holt sich den naechsten Strahl selbst, sie sind sehr verschieden teuer.
    arbeiter::verteilt(n - 1U, [&je](std::size_t i) { je(i + 1U); }, 16U);
}

// Balken und Vollbild-Blende ueber das GPU-Bild (rc594). Dieselben Zahlen
// wie drawLetterbox/drawVollblende (bhed/mapview.h): Balken je ein Zehntel
// der Bildhoehe mal Anteil, schwarz mit Deckung Anteil; die Blende darueber.
// ImGui mischt "Quelle ueber Ziel" - nacheinander gezeichnet ergibt das
// dasselbe wie vorher das vorgemischte Ueberlagerungsbild.
void blendeUeberlagern(ImDrawList* dl, ImVec2 a, ImVec2 b, int bildHoehe) {
    if (bildHoehe <= 0 || b.y <= a.y) {
        return;
    }
    const float skal = (b.y - a.y) / static_cast<float>(bildHoehe);
    const float anteil = std::clamp(g_app->blendeBalken, 0.0F, 1.0F);
    // Fuer den Selbsttest (Paket kamera, rc601): wo Balken und Blende liegen.
    g_app->blendeFlaeche[0] = a.x;
    g_app->blendeFlaeche[1] = a.y;
    g_app->blendeFlaeche[2] = b.x;
    g_app->blendeFlaeche[3] = b.y;
    g_app->blendeBalkenPx = 0.0F;
    if (anteil > 0.0F) {
        const int bar = std::max(1, static_cast<int>(static_cast<float>(bildHoehe / 10) * anteil + 0.5F));
        const auto deck = static_cast<int>(std::lround(255.0F * anteil));
        const float hoch = static_cast<float>(bar) * skal;
        g_app->blendeBalkenPx = hoch;
        dl->AddRectFilled(a, ImVec2{b.x, a.y + hoch}, IM_COL32(0, 0, 0, deck));
        dl->AddRectFilled(ImVec2{a.x, b.y - hoch}, b, IM_COL32(0, 0, 0, deck));
    }
    const float* f = g_app->blendeFarbe;
    const float al = std::clamp(f[3], 0.0F, 1.0F);
    if (al > 0.0F) {
        const auto kanal = [](float v) { return static_cast<int>(std::lround(255.0F * std::clamp(v, 0.0F, 1.0F))); };
        dl->AddRectFilled(a, b, IM_COL32(kanal(f[0]), kanal(f[1]), kanal(f[2]), kanal(al)));
    }
}

// --- Das Spielfenster in der Ansicht (Paket kamera, rc601) -------------------
//
// Gemeldet vom Tester: "I think camera FOV is slightly off". Das Spiel
// rechnet den Bildwinkel fuer sein Fenster (CG_CalcFOVFromX), die Ansicht hat
// ein beliebiges Seitenverhaeltnis. Also liegt das Spielbild als Rahmen mittig
// in der Ansicht (SpielRahmen, bhed/mapview.h), und was draussen liegt, deckt
// die Ueberlagerung ab: beim Blick durch die Kamera und beim Abspielen
// schwarz - dort gibt es im Spiel nichts -, in der freien Ansicht und im
// Pilot halbdurchsichtig mit einem duennen Rand als Hilfe.

bool movieDuelsOrdner(const std::string& verzeichnis) {
    // Dieselbe Erkennung wie standardZielPk3 (app_mission.cpp) und die
    // Figurenvorgaben (app_animation.cpp): ein Ordner "...\md" oder einer
    // mit "movie duels" im Namen.
    std::string d = verzeichnis;
    for (char& c : d) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    while (!d.empty() && (d.back() == '\\' || d.back() == '/')) { d.pop_back(); }
    const bool mdOrdner = d.size() >= 3 && (d.compare(d.size() - 3, 3, "\\md") == 0 || d.compare(d.size() - 3, 3, "/md") == 0);
    return mdOrdner || d.find("movie duels") != std::string::npos;
}

float spielFensterSeiten() {
    const float s = g_app->settings.spielSeiten;
    if (s > 0.0F && std::isfinite(s)) { return s; }
    // 0: wie das Spiel ab Werk (rc601 integ, nach dem Regelwerk) - Movie
    // Duels startet mit r_mode -2, der Desktopaufloesung von Bildschirm 0
    // (sdl_window.cpp:779/398, :350 MD); OpenJK mit r_mode 4 = 800x600
    // (sdl_window.cpp:746), also 4:3.
    static float bildschirm = 0.0F;
    if (bildschirm <= 0.0F) { bildschirm = platform::hauptBildschirmSeiten(); }
    return spielSeitenWirksam(0.0F, regelwerkMovieDuels(), bildschirm);
}

bool spielFovAspectAdjust() {
    // Automatisch: Movie Duels setzt "1" (cg_main.cpp:555), OpenJK "0"
    // (cg_main.cpp:355), das Original von Raven kennt die Korrektur nicht.
    // rc601 integ: "automatisch" folgt dem Regelwerk (bhed/regelwerk.h).
    return fovAspectAdjustWirksam(g_app->settings.fovAspectAdjust, regelwerkMovieDuels());
}

SpielFenster spielFensterJetzt() {
    SpielFenster s;
    s.seiten = spielFensterSeiten();
    s.fovAspectAdjust = spielFovAspectAdjust();
    return s;
}

void spielRahmenRechteck(ImVec2 a, ImVec2 b, ImVec2& ra, ImVec2& rb) {
    const float bb = (b.x - a.x) * std::clamp(g_app->spielRahmen.breite, 0.0F, 1.0F);
    const float bh = (b.y - a.y) * std::clamp(g_app->spielRahmen.hoehe, 0.0F, 1.0F);
    ra = ImVec2{a.x + ((b.x - a.x) - bb) * 0.5F, a.y + ((b.y - a.y) - bh) * 0.5F};
    rb = ImVec2{ra.x + bb, ra.y + bh};
}

void spielRahmenUeberlagern(ImDrawList* dl, ImVec2 a, ImVec2 b) {
    const int deck = std::clamp(g_app->spielRahmenDeckung, 0, 255);
    if (dl == nullptr || deck <= 0 || b.x <= a.x || b.y <= a.y) { return; }
    ImVec2 ra;
    ImVec2 rb;
    spielRahmenRechteck(a, b, ra, rb);
    const ImU32 farbe = IM_COL32(0, 0, 0, deck);
    // Vier Streifen um den Rahmen (oben, unten, links, rechts).
    if (ra.y > a.y) { dl->AddRectFilled(a, ImVec2{b.x, ra.y}, farbe); }
    if (rb.y < b.y) { dl->AddRectFilled(ImVec2{a.x, rb.y}, b, farbe); }
    if (ra.x > a.x) { dl->AddRectFilled(ImVec2{a.x, ra.y}, ImVec2{ra.x, rb.y}, farbe); }
    if (rb.x < b.x) { dl->AddRectFilled(ImVec2{rb.x, ra.y}, ImVec2{b.x, rb.y}, farbe); }
    if (deck < 255 && (ra.x > a.x + 0.5F || ra.y > a.y + 0.5F || rb.x < b.x - 0.5F || rb.y < b.y - 0.5F)) {
        // Hilfe in der freien Ansicht: ein duenner Rand ums Spielbild.
        dl->AddRect(ra, rb, IM_COL32(255, 255, 255, 110), 0.0F, 0, 1.0F);
    }
}

// Im Klappfenster "Ansicht einstellen": Seitenverhaeltnis des Spiels,
// cg_fovAspectAdjust und der Rahmen in der freien Ansicht.
void spielFensterEinstellungen() {
    ImGui::SeparatorText(tr(Str::GameScreen));
    const float feld = ImGui::GetFontSize() * 14.0F;
    // Die Vorgaben: 4:3 (r_mode 4 in OpenJK und das Original), 16:10, 16:9
    // und 21:9 - 2560x1080 ist 64:27. 0 = wie der Hauptbildschirm.
    struct Wahl {
        float seiten;
        const char* name;
    };
    static const Wahl wahl[] = {
        {4.0F / 3.0F, "4:3"}, {16.0F / 10.0F, "16:10"}, {16.0F / 9.0F, "16:9"}, {64.0F / 27.0F, "21:9 (2560x1080)"}};
    char bildschirm[96];
    if (regelwerkMovieDuels()) {
        std::snprintf(bildschirm, sizeof(bildschirm), tr(Str::GameScreenMonitor),
                      static_cast<double>(platform::hauptBildschirmSeiten() > 0.0F ? platform::hauptBildschirmSeiten()
                                                                                   : 16.0F / 9.0F));
    } else {
        // rc601 integ: unter OpenJK heisst "wie ab Werk" r_mode 4 (800x600).
        std::snprintf(bildschirm, sizeof(bildschirm), "%s", tr(Str::GameScreenAuto43));
    }
    float& seiten = g_app->settings.spielSeiten;
    std::string jetzt = bildschirm;
    for (const Wahl& w : wahl) {
        if (std::fabs(seiten - w.seiten) < 0.001F) { jetzt = w.name; }
    }
    if (seiten > 0.0F && jetzt == bildschirm) {
        char eigen[32];
        std::snprintf(eigen, sizeof(eigen), "%.3f:1", static_cast<double>(seiten));
        jetzt = eigen;
    }
    ImGui::SetNextItemWidth(feld);
    if (ImGui::BeginCombo(tr(Str::GameScreenAspect), jetzt.c_str())) {
        if (ImGui::Selectable(bildschirm, seiten <= 0.0F)) {
            seiten = 0.0F;
            g_app->mapDirty = true;
        }
        for (const Wahl& w : wahl) {
            if (ImGui::Selectable(w.name, std::fabs(seiten - w.seiten) < 0.001F)) {
                seiten = w.seiten;
                g_app->mapDirty = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::GameScreenAspectHint)); }
    // cg_fovAspectAdjust: automatisch (nach den Spielordnern), aus, an.
    int& adjust = g_app->settings.fovAspectAdjust;
    const bool autoAn = regelwerkMovieDuels();   // rc601 integ: nach dem Regelwerk
    const char* namen[3] = {tr(autoAn ? Str::GameFovAdjustAutoOn : Str::GameFovAdjustAutoOff), tr(Str::GameFovAdjustOff),
                            tr(Str::GameFovAdjustOn)};
    const int werte[3] = {-1, 0, 1};
    int jetztNr = 0;
    for (int i = 0; i < 3; ++i) {
        if (adjust == werte[i]) { jetztNr = i; }
    }
    ImGui::SetNextItemWidth(feld);
    if (ImGui::BeginCombo(tr(Str::GameFovAdjust), namen[jetztNr])) {
        for (int i = 0; i < 3; ++i) {
            if (ImGui::Selectable(namen[i], i == jetztNr)) {
                adjust = werte[i];
                g_app->mapDirty = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::GameFovAdjustHint)); }
    if (ImGui::Checkbox(tr(Str::GameFrameFree), &g_app->settings.spielRahmenFrei)) { g_app->mapDirty = true; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::GameFrameFreeHint)); }
}

void drawMapView() {
    BHED_MESS("Kartenansicht");
    schluesselMitZeit();
    // OHNE Karte kein frueher Ausstieg mehr.
    //
    // Vorher stand hier ein return - und die Seitenleiste, die seit rc173
    // am ENDE dieser Funktion gezeichnet wird, kam damit nie an die Reihe.
    // Gemeldet als "wo ist die timeline und die map optionen?": ohne
    // geladene Karte war die ganze rechte Spalte weg, samt dem Knopf, mit
    // dem man sie zurueckholt.
    //
    // Jetzt steht statt des Bildes nur der Hinweis, und die Leiste kommt
    // trotzdem - abgeblendet, wie alles andere ohne Karte.
    // Beim Moduswechsel EIN Bild lang nichts zeichnen (rc515).
    //
    // Ob dieses Bild das Einschwingbild ist, entscheidet nicht mehr diese
    // Funktion, sondern `moduswechselBild` - gesetzt dort, wo ALLE Modi
    // vorbeikommen. Vorher stand die Merkstelle hier drin, und diese
    // Funktion laeuft nur im Karten-Modus: Karte -> Events -> Karte griff
    // der Uebersprung beim zweiten Mal nicht mehr.
    if (g_app->moduswechselBild) {
        ImGui::Dummy(ImGui::GetContentRegionAvail());
        return;
    }

    if (g_app->geo.empty()) {
        // Dieselbe Anordnung wie mit Karte: links das Bildfeld, rechts die
        // Leiste. Statt des Bildes steht ein Platzhalter derselben Groesse
        // - so springt beim Laden einer Karte nichts, und der Anwender
        // sieht schon vorher, wo was liegen wird.
        const ImVec2 leer = ImGui::GetContentRegionAvail();
        const float leisteW0 = mapSidebarWidth(leer.x);
        g_app->mapSidebarW = leisteW0;
        const float bildW = std::max(64.0F, leer.x - leisteW0);
        g_app->dbgBildW = bildW;
        const float bildH = std::max(64.0F, leer.y - 2.0F);
        // Statt "No map loaded" die Startflaeche (gui/app_mission.cpp): drei
        // Wege hinein und die zuletzt bearbeiteten Missionen.
        drawStartflaeche(bildW, bildH);
        return;
    }

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    // Die Seitenleiste nimmt sich ihren Platz VOM BILD, nicht von der
    // Hoehe. Eingeklappt bleibt nur ein schmaler Knopf stehen - so wie in
    // Blender, wo die Leiste ganz verschwindet und ein kleiner Pfeil sie
    // zurueckholt.
    const float leisteW = mapSidebarWidth(avail.x);
    g_app->mapSidebarW = leisteW;
    const int w = std::max(64, static_cast<int>(avail.x - leisteW));
    // Nur noch EINE Zeile Bedienung unter dem Bild: Ort und Kameraknopf.
    // Alles andere steht rechts oder in der Zeitleiste.
    const int h = std::max(64, static_cast<int>(avail.y) -
                               static_cast<int>(ImGui::GetFrameHeightWithSpacing() * 1.4F));
    // Paket kamera, rc601: das Spielfenster, und wie breit die Ansicht ist.
    const SpielFenster spiel = spielFensterJetzt();
    const float ansichtSeiten = static_cast<float>(w) / static_cast<float>(std::max(h, 1));

    // --- Umsehen und Fliegen ---------------------------------------------
    //
    // Rechte Maustaste zum Umsehen, wie in jedem Editor. Die linke bleibt
    // frei fuers Anklicken.
    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                         !g_app->throughCamera;
    ImGuiIO& io = ImGui::GetIO();

    // Bedienung wie im Ansichtsfenster von 3ds Max:
    //   mittlere Taste        schieben
    //   Alt + mittlere Taste  umkreisen
    //   Rad                   zoomen
    //   rechte Taste + WASD   fliegen
    //
    // Zum SETZEN einer Kamera will man ein Ziel umkreisen und sehen, wie es
    // aus verschiedenen Richtungen aussieht. Fliegen ist gut, um irgendwo
    // hinzukommen, aber umstaendlich, um sich um etwas herumzubewegen.
    if (hovered) { ansichtsMarkenTasten(); }
    // Draufsicht (rc589): schieben statt drehen, das Rad hebt und senkt die
    // Kamera (die Schnitthoehe bleibt), WASD schiebt in Bildrichtung.
    if (g_app->draufsicht) {
        g_app->cam.angles[0] = 89.0F;
        g_app->cam.angles[2] = 0.0F;
        const float bodenZ = g_app->draufSchnitt - 160.0F;
        const float hoehe = std::max(64.0F, g_app->cam.pos[2] - bodenZ);
        const float bildH = std::max(1.0F, g_app->kartenAnsicht[3] - g_app->kartenAnsicht[1]);
        const float jePunkt = 2.0F * hoehe * std::tan(g_app->cam.fovY * 3.14159265F / 360.0F) / bildH;
        const float gier = g_app->cam.angles[1] * 3.14159265F / 180.0F;
        const float vx = std::cos(gier);   // "oben" im Bild
        const float vy = std::sin(gier);
        const auto schiebe = [&](float rechts, float vor) {
            g_app->cam.pos[0] += vx * vor + vy * rechts;
            g_app->cam.pos[1] += vy * vor - vx * rechts;
        };
        if (hovered && (ImGui::IsMouseDown(ImGuiMouseButton_Middle) || ImGui::IsMouseDown(ImGuiMouseButton_Right))) {
            schiebe(-io.MouseDelta.x * jePunkt, io.MouseDelta.y * jePunkt);
        }
        if (hovered && io.MouseWheel != 0.0F) {
            g_app->cam.pos[2] = std::max(g_app->draufSchnitt + 64.0F,
                                         g_app->cam.pos[2] - io.MouseWheel * hoehe * 0.15F);
        }
        if (hovered) {
            const float schritt = (io.KeyShift ? 3.0F : 1.0F) * hoehe * 1.2F * std::max(io.DeltaTime, 0.001F);
            if (ImGui::IsKeyDown(ImGuiKey_W)) { schiebe(0.0F, schritt); }
            if (ImGui::IsKeyDown(ImGuiKey_S)) { schiebe(0.0F, -schritt); }
            if (ImGui::IsKeyDown(ImGuiKey_D)) { schiebe(schritt, 0.0F); }
            if (ImGui::IsKeyDown(ImGuiKey_A)) { schiebe(-schritt, 0.0F); }
        }
    } else if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        if (io.KeyAlt) {
            g_app->orbit.turn(g_app->cam, -io.MouseDelta.x * 0.3F,
                              -io.MouseDelta.y * 0.3F);
        } else {
            g_app->orbit.pan(g_app->cam, io.MouseDelta.x, io.MouseDelta.y);
        }
    }
    if (hovered && io.MouseWheel != 0.0F && !g_app->draufsicht) {
        g_app->orbit.zoom(g_app->cam, io.MouseWheel);
    }
    if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Right) && !g_app->draufsicht) {
        g_app->cam.angles[1] -= io.MouseDelta.x * 0.25F;
        g_app->cam.angles[0] = std::clamp(
            g_app->cam.angles[0] - io.MouseDelta.y * 0.25F, -89.0F, 89.0F);
    }
    if (hovered && !g_app->draufsicht) {
        const float base = io.KeyShift ? 40.0F : 12.0F;
        const float step = base * std::max(io.DeltaTime, 0.001F) * 60.0F;
        float fwd = 0.0F;
        float side = 0.0F;
        float vert = 0.0F;
        if (ImGui::IsKeyDown(ImGuiKey_W)) { fwd += step; }
        if (ImGui::IsKeyDown(ImGuiKey_S)) { fwd -= step; }
        if (ImGui::IsKeyDown(ImGuiKey_D)) { side += step; }
        if (ImGui::IsKeyDown(ImGuiKey_A)) { side -= step; }
        if (ImGui::IsKeyDown(ImGuiKey_E)) { vert += step; }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) { vert -= step; }
        if (fwd != 0.0F || side != 0.0F || vert != 0.0F) {
            g_app->cam.move(fwd, side, vert);
        }
    }

    // --- Zeichnen ---------------------------------------------------------
    // Die Groesse des Renderziels setzt der Block weiter unten; hier wird
    // nur bemerkt, dass sich das FENSTER geaendert hat.
    if (g_app->mapViewW != w || g_app->mapViewH != h) {
        g_app->mapViewW = w;
        g_app->mapViewH = h;
        g_app->mapDirty = true;
    }
    // --- Pilot: mit dem ZOOM der Einstellung (Paket kamera, rc601) ---------
    //
    // Bisher uebernahm "Steuern" nur Ort und Winkel des Schluessels; der
    // Bildwinkel blieb der der freien Ansicht (73,7 senkrecht, FOV 90 bei
    // 4:3) - bei ZOOM 30 sah man weit mehr als das Spiel. Jetzt der
    // Bildwinkel der Einstellung im Spielfenster, in JEDEM Bild neu: die
    // Ansicht kann sich waehrend des Steuerns in der Groesse aendern.
    // Gesetzt wird g_app->cam selbst, damit Klicks und Ueberlagerungen mit
    // derselben Projektion rechnen; kameraPilotBeenden stellt den Bildwinkel
    // der freien Ansicht wieder her.
    g_app->spielRahmen = SpielRahmen{};
    g_app->spielRahmenDeckung = 0;
    if (g_app->kameraPilot) {
        float fov = 90.0F;   // CAMERA_DEFAULT_FOV
        const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
        if (g_app->camTrackValid && g_app->pilotSchluessel >= 0 &&
            static_cast<std::size_t>(g_app->pilotSchluessel) < ks.size()) {
            const double t = einstellungsZeit(g_app->camTrack, ks[static_cast<std::size_t>(g_app->pilotSchluessel)].path);
            if (t >= 0.0) { fov = g_app->camTrack.at(t).fovX; }
        }
        g_app->spielRahmen = setzeSpielFov(g_app->cam, fov, ansichtSeiten, spiel);
        g_app->spielRahmenDeckung = kSpielRahmenHalb;
    }
    // Durch die gewaehlte Kamera schauen: Stellung und Blickwinkel kommen
    // dann aus dem Skript, nicht aus der freien Kamera.
    Camera useCam = g_app->cam;
    ShotView shot;
    // Beim Abspielen bestimmt die Bahn die Stellung, nicht die Auswahl.
    const bool playingNow = g_app->playing && g_app->followCam &&
                            g_app->camTrackValid &&
                            !g_app->camTrack.segments.empty();
    const bool through = !playingNow && g_app->throughCamera &&
                         shotAtSelection(shot);
    // --- Durch die Kamera, auch OHNE Auswahl ------------------------------
    //
    // Gemeldet: "Through Camera hat nichts geaendert, sieht genau aus wie
    // vorher."
    //
    // Es hat auch nichts geaendert. Die Ansicht sprang bisher nur an, wenn
    // im Skriptbaum ein Kamerabefehl AUSGEWAEHLT war - oder waehrend des
    // Abspielens. Steht der Zeiger still und ist nichts angeklickt, blieb
    // die freie Kamera stehen, und daneben stand ein Hinweis, den man
    // leicht uebersieht.
    //
    // Das ist die falsche Vorgabe. Wer das Haekchen setzt, will sehen, was
    // die Kamera AN DIESER STELLE sieht - der Zeitzeiger sagt ja bereits,
    // welche Stelle gemeint ist. Die Auswahl bleibt als Uebersteuerung: sie
    // geht vor, damit man einen einzelnen Befehl gezielt anschauen kann.
    const bool amZeiger = !playingNow && !through && g_app->throughCamera &&
                          g_app->camTrackValid &&
                          !g_app->camTrack.segments.empty();
    // Fuer das Band merken. Ist keine der beiden Arten aktiv, muessen
    // Beschriftung und Bildwinkel zurueckgesetzt werden - sonst steht
    // neben dem Haekchen noch der Name der letzten Kamera.
    g_app->camViewActive = through || amZeiger;
    if (!g_app->camViewActive) {
        g_app->camViewLabel.clear();
        g_app->camViewFov = 90.0F;
    }
    if (playingNow || amZeiger) {
        // camStateAt statt camTrack.at: darin steckt FOLLOW.
        const CameraState st = camStateAt(g_app->playMs);
        // Paket kamera, rc601: vor ENABLE und nach DISABLE zeigt das Spiel
        // den Spieler mit cg_fov 80 (sichtFovX, camtrack.h), und gerechnet
        // wird im Spielfenster (CG_CalcFOVFromX), nicht in der Ansicht.
        const float fov = sichtFovX(st);
        g_app->camViewFov = fov;
        for (int k = 0; k < 3; ++k) {
            useCam.pos[k] = st.pos[k];
            useCam.angles[k] = st.angles[k];
        }
        g_app->spielRahmen = setzeSpielFov(useCam, fov, ansichtSeiten, spiel);
        g_app->spielRahmenDeckung = 255;
        g_app->camViewLabel = tr(Str::MapCamAtPlayhead);
    } else if (through) {
        g_app->camViewLabel = shot.label;
        g_app->camViewFov = shot.fovX;
        // Die Vorschau am Gizmo gilt AUCH hier.
        //
        // Sonst zieht man am Gizmo, und der Durchblick zeigt weiter die
        // alte Einstellung - also gerade das Bild, das man beurteilen
        // will, bleibt stehen. Nur wenn die Kamera angewaehlt ist; sonst
        // schwenkte der Durchblick, waehrend man anderswo arbeitet.
        const bool vorschau = g_app->cameraSelected;
        for (int k = 0; k < 3; ++k) {
            useCam.pos[k] = shot.pos[k];
            useCam.angles[k] = shot.haveAng ? shot.ang[k] : 0.0F;
            if (vorschau) {
                useCam.pos[k] += g_app->gizmoOffset[k];
                useCam.angles[k] += g_app->gizmoAngles[k];
            }
        }
        // Paket kamera, rc601: im Spielfenster, mit Rahmen.
        g_app->spielRahmen = setzeSpielFov(useCam, shot.fovX, ansichtSeiten, spiel);
        g_app->spielRahmenDeckung = 255;
        g_app->mapDirty = true;   // haengt an der Auswahl, also jedes Bild pruefen
    } else if (!g_app->kameraPilot && g_app->settings.spielRahmenFrei && !g_app->draufsicht) {
        // Paket kamera, rc601: in der freien Ansicht halbdurchsichtig, was
        // eine Kamera an dieser Stelle im Spiel zeigte (FOV 90 nach ENABLE) -
        // "Kamera hier einfuegen" uebernimmt genau diesen Rahmen.
        g_app->spielRahmen = spielRahmenFreieSicht(g_app->cam, ansichtSeiten, spiel);
        g_app->spielRahmenDeckung = kSpielRahmenHalb;
    }

    // Hat sich die Kamera bewegt?
    g_app->gezeigteKamera = useCam;

    const float now[7] = {g_app->cam.pos[0], g_app->cam.pos[1], g_app->cam.pos[2],
                          g_app->cam.angles[0], g_app->cam.angles[1],
                          g_app->cam.angles[2], g_app->orbit.distance};
    for (int k = 0; k < 7; ++k) {
        if (std::fabs(now[k] - g_app->lastCam[k]) > 0.0001F) {
            g_app->mapDirty = true;
        }
        g_app->lastCam[k] = now[k];
    }

    // --- Wackeln aus Effekten -------------------------------------------
    //
    // Der Wackler aus dem SKRIPT (camera SHAKE) sitzt in camtrack.cpp und
    // wirkt schon lange. Effekte koennen dasselbe ausloesen - 73
    // CameraShake-Primitive in den .efx aus assets1 -, und das fehlte.
    //
    // Angewandt wird es HIER, nach allen Kamerazweigen, damit es die freie
    // Ansicht genauso trifft wie den Durchblick: eine Explosion neben dem
    // Betrachter wackelt, egal durch welche Kamera man sieht.
    if (g_app->showEffects && !g_app->effectRunners.empty()) {
        const float amp =
            effectShakeAt(g_app->effectRunners, g_app->playMs, useCam.pos);
        if (amp > 0.001F) {
            // DERSELBE zeitabgeleitete Wert wie in camtrack.cpp. Echter
            // Zufall waere in einer Vorschau, die man zurueckspult, Unruhe
            // statt Wiedergabetreue - man wuesste nie, ob eine Aenderung
            // von einem selbst kommt oder vom Wuerfel.
            auto wobble = [&](int axis) {
                auto h = static_cast<std::uint32_t>(
                    static_cast<std::int64_t>(g_app->playMs * 3.0) +
                    static_cast<std::int64_t>(axis) * 7919);
                h *= 2654435761U;
                h ^= h >> 15U;
                h *= 2246822519U;
                h ^= h >> 13U;
                return (static_cast<float>(h >> 8U) / 8388608.0F) - 1.0F;
            };
            for (int k = 0; k < 3; ++k) {
                useCam.pos[k] += wobble(k) * amp;
            }
            for (int k = 0; k < 2; ++k) {   // kein ROLL, wie in der Engine
                useCam.angles[k] += wobble(k + 3) * amp;
            }
            g_app->mapDirty = true;   // sonst steht das Bild still
        }
    }

    // --- Bewegt sich die Kamera? ----------------------------------------
    //
    // Verglichen wird die tatsaechlich benutzte Kamera, nicht `g_app->cam` -
    // eine Skriptfahrt bewegt sie ebenso wie die Maus.
    bool kameraBewegt = !g_app->haveLastCam;
    for (int k = 0; k < 3 && !kameraBewegt; ++k) {
        if (useCam.pos[k] != g_app->lastCamPos[k] ||
            useCam.angles[k] != g_app->lastCamAng[k]) {
            kameraBewegt = true;
        }
    }
    for (int k = 0; k < 3; ++k) {
        g_app->lastCamPos[k] = useCam.pos[k];
        g_app->lastCamAng[k] = useCam.angles[k];
    }
    g_app->haveLastCam = true;
    if (kameraBewegt) {
        g_app->stillCount = 0;
    } else if (g_app->stillCount < 1) {
        // Gerade stehengeblieben: EIN Bild in voller Aufloesung nachlegen.
        ++g_app->stillCount;
        g_app->mapDirty = true;
    }
    // Das Renderziel hat die Groesse der Ansicht. (Frueher rasterte der
    // Software-Zeichner waehrend der Bewegung groeber; der GPU-Weg braucht
    // das nicht.)
    {
        const int rw = std::max(64, w);
        const int rh = std::max(64, h);
        if (g_app->mapImage.width != rw || g_app->mapImage.height != rh) {
            g_app->mapImage.width = rw;
            g_app->mapImage.height = rh;
            g_app->mapDirty = true;
        }
    }

    // --- Erst aufbauen, dann ueber die Grafikkarte zeichnen ---------------
    //
    // In diesem Block steckt beides: die Szene und das Effektnetz AUFBAUEN,
    // und sie zeichnen. Gezeichnet wird nur noch ueber die Grafikkarte; der
    // Software-Rasterer ist entfernt (rc568).
    //
    // --- Beim Laden NICHT zeichnen ------------------------------------
    //
    // Das Netz wird aufgebaut, waehrend die Ansicht weiterlaeuft. Ein halbes
    // Netz an die Grafikkarte zu geben fuehrte zum Absturz (gemeldet beim
    // Laden einer Mission). Solange es nicht fertig ist, bleibt `mapDirty`
    // stehen, und das naechste Bild versucht es wieder.
    // `mapBereit` wird dort gesetzt, wo das Netz ausgetauscht wird - nicht
    // aus dem Netz erraten. Siehe App::mapBereit.
    const bool netzFertig = g_app->mapBereit && !g_app->mesh.batches.empty() &&
                            !g_app->mesh.verts.empty() &&
                            !g_app->mesh.indexes.empty();
    const bool gpuBereit = gpu::verfuegbar() && netzFertig;
    // Die Figuren dieses Bildes, wenn der GPU-Weg laeuft. Sie werden im
    // Block gesammelt und am ENDE gezeichnet - nach der Karte, damit sie auf
    // deren Tiefe treffen.
    std::vector<ActorDraw> gpuFiguren;
    bool gezeichnet = false;   // hat die Grafikkarte in diesem Bild gezeichnet?
    if (g_app->mapDirty) {
        // rc603: fehlen die Modelle der Figuren (rescanGamePaths hat sie
        // geleert - neue Spielordner, etwa die einer Mission eines anderen
        // Spiels), JETZT neu laden, bevor dieses Bild Effekte und bewegliche
        // Teile sammelt. Das Nachladen stand weiter unten, bei den Figuren -
        // nach dem Sammeln: loadMapModels leert md3Meshes, und die schon
        // gesammelten Mover zeigten in die geleerte Tabelle. Selbsttest
        // missionsliste (t2_trip, dann zurueck zu Movie Duels): Absturz in
        // zeichneKarte, gpumap_win32.cpp:5400, beim Lesen eines freigegebenen
        // Netzes.
        if (g_app->showActors && !g_app->scene.actors.empty() &&
            g_app->actorAssets.size() != g_app->scene.actors.size()) {
            loadActorModels();
            kinoSkeletteAbgleichen();
            preloadSaberAssets();
            loadMapEffects();
            loadMapModels();
        }
        // Die Blende kommt ganz zuletzt, in die Ueberlagerung - nach Figuren
        // und Effekten. Sonst uebermalen die sie wieder: "kann man durch die
        // schwarzen balken noch durchsehen wenn die cutscene laeuft".
        // Auch "am Abspielzeiger" (Through camera ohne Auswahl) ist ein Blick
        // durch die Kamera - mit Blende, wie im Spiel (Kartentest 27.09.).
        const bool blende = g_app->camViewActive || playingNow;
        const auto t0 = std::chrono::steady_clock::now();
        App::FrameCost kosten;
        // Die bewegten Teile sammeln: Tueren, Plattformen, Raumschiffe.
        //
        // Ihre Geometrie steht in der .bsp an ihrem GEBAUTEN Platz. Was das
        // Skript aendert, ist die Verschiebung dagegen - deshalb aktueller
        // Ort minus Startort, und nicht der Ort selbst. Bei t=0 ist beides
        // gleich, und nichts verrutscht.
        // Alles, was in diesem Bild an Effekten laeuft - Runner, Schuesse
        // und Folgeeffekte aller Stufen. Die Lichter weiter unten brauchen
        // dieselbe Liste: eine Explosion als Aufpralleffekt (rocket/
        // explosion.efx hat ein `Light`) soll die Figur daneben genauso
        // anstrahlen wie ein Runner.
        std::vector<EffectInstance> alleEffekte;

        // --- Effekte ------------------------------------------------------
        //
        // Die Partikel werden zu Vierecken und reisen als bewegliches Teil
        // durch denselben Aufsammelschritt wie alles andere. Kein zweiter
        // Zeichenweg - siehe bhed/efxdraw.h.
        //
        // Die Achsen der Kamera braucht es, weil die Vierecke senkrecht zur
        // Blickrichtung stehen. Sie kommen aus derselben Rechnung wie im
        // Zeichner: vorn aus den Winkeln, rechts = vorn x oben.
        // --- Das Effektnetz fuer EINE Kamera (rc597) -------------------------
        //
        // Die Vierecke stehen quer zum Blick, und CParticle::Cull laesst weg,
        // was hinter dem Auge liegt. Die Kameravorschau rechts unten bekam
        // deshalb das Netz der Hauptansicht: "wenn ich meine hauptkamera weg
        // drehe werden die effecte nicht mehr gerendert in der kamera rechts
        // unten". Jetzt baut jede Ansicht ihr eigenes - aus derselben Liste.
        const auto baueEffektNetz = [&](const std::vector<EffectInstance>& alle, const Camera& kam, int bw, int bh) {
            // Paket kamera, rc601: die Achsen aus Camera::forward/right/up -
            // MIT Rollwinkel, wie cg.refdef.viewaxis (AnglesToAxis). Danach
            // richtet der Renderer die Sprites aus (RB_SurfaceSprite,
            // tr_surface.cpp:161-189: viewParms.ori.axis[1] und [2]). Vorher
            // eine eigene Rechnung ohne Roll: bei camera ROLL lagen die
            // Vierecke sonst schraeg zum Bild.
            float fwd[3];
            float rgt[3];
            float upv[3];
            kam.forward(fwd);
            kam.right(rgt);
            kam.up(upv);
            EffectCull auge;
            for (int k = 0; k < 3; ++k) {
                auge.viewer[k] = kam.pos[k];
            }
            auge.haveViewer = true;
            auge.sichtTabelle = false;
            BspMesh netz =
                // Die Kartengeometrie mit: ohne sie kann ein Partikel mit
                // `usePhysics` nicht abprallen - siehe efx::sim::flugbahn.
                // --- Die Keulung ist NOCH NICHT eingeschaltet -----------
                //
                // `EffectCull` steht bereit (bhed/efxdraw.h) und spart
                // gemessen 337 auf 0,6 ms. Sie wird trotzdem nicht
                // uebergeben, weil sie in drei Probeansichten SICHTBARE
                // Effekte weggenommen hat - auch mit einem Freiraum von
                // 4000 Einheiten.
                //
                // Der Grund: die Sichtbarkeitstabelle beantwortet die Frage
                // "sieht Raum A Raum B". Ein Effekt ist aber kein Punkt -
                // die Teilchen von volcano.efx fliegen 1200 Einheiten weit
                // und reichen damit in Raeume, die der Runner selbst nie
                // beruehrt. Und eine Vorschaukamera steht oft AUSSERHALB
                // der Karte, wo der Cluster nichts taugt.
                //
                // Eine Keulung, die manchmal etwas wegnimmt, ist schlimmer
                // als eine, die es nicht gibt: sie erzeugt Fehler, die man
                // fuer Effektfehler haelt und woanders sucht.
                //
                // Das AUGE wird trotzdem uebergeben - ohne die Tabelle
                // (sichtTabelle = false). Ohne Auge gibt es weder die
                // Bandbreite nach RB_SurfaceLine noch die Richtung eines
                // Blitzes (RB_SurfaceElectricity) noch CParticle::Cull.
                buildEffectMesh(alle, g_app->playMs, rgt, upv,
                                &g_app->effectShaders, &g_app->geo, &auge);
            // --- Der Vollbildblitz (Flash) ------------------------------
            //
            // Als Sprite IM Effektnetz, nicht in der 2D-Ueberlagerung: nur so
            // gelten Shader und Mischart der .efx, und der Blitz liegt wie im
            // Spiel UNTER den Kinobalken (siehe appendScreenFlashes).
            {
                const float seiten = static_cast<float>(bw) /
                                     static_cast<float>(std::max(bh, 1));
                const float fovX =
                    2.0F *
                    std::atan(std::tan(kam.fovY * 3.14159265F / 360.0F) *
                              seiten) *
                    180.0F / 3.14159265F;
                const std::vector<ScreenFlash> blitze =
                    effectScreenFlashesAt(alle, g_app->playMs, kam.pos, fwd);
                appendScreenFlashes(netz, blitze, kam.pos, fwd,
                                    rgt, upv, fovX, &g_app->effectShaders);
                // rc601 saber review: die Brandspuren (CG_AddMarks).
                saberSpurenAnhaengen(netz, g_app->playMs);
            }
            return netz;
        };
        // rc601 saber review: die Schwerttakte bis zu diesem Bild rechnen - auch
        // mit ausgeschalteten Effekten, denn die Zeichnung der Klingen (Laenge,
        // Kuerzen an Waenden) liest sie weiter unten. Vorher geschah das nur
        // ueber saberEffekteBei, also nur mit sichtbaren Effekten.
        // rc603 fix603: Teilzeiten ungeglaettet (FrameCost::rawSaber ...).
        const auto msSeit = [](std::chrono::steady_clock::time_point a) {
            return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - a).count();
        };
        const auto tSab0 = std::chrono::steady_clock::now();
        saberRechneBis(g_app->playMs);
        kosten.rawSaber = msSeit(tSab0);
        // rc601 saber review 2c: die Kamera dieses Bildes fuer den Klirrblitz,
        // der als 2D-Bild ueber den Kinobalken kommt (blendeUeberlagern).
        saberBlitzAnsicht(0, &useCam, w, h, g_app->playMs);
        // rc601 saber: Funken, Kochen und Klirren der Lichtschwerter - auch in
        // Karten ohne fx_runner (die Bedingung darunter liess sie sonst weg).
        const auto tSfx0 = std::chrono::steady_clock::now();
        const std::vector<EffectInstance> schwertFx =
            g_app->showEffects ? saberEffekteBei(g_app->playMs) : std::vector<EffectInstance>{};
        kosten.rawSaberFx = msSeit(tSfx0);
        const auto tSp0 = std::chrono::steady_clock::now();
        const bool spurenDa = g_app->showEffects && saberSpurenDa(g_app->playMs);
        kosten.rawSpuren = msSeit(tSp0);
        if (g_app->showEffects &&
            (!g_app->effectRunners.empty() || !schwertFx.empty() || spurenDa)) {
            // Die Schuesse kommen dazu - dieselbe Liste, derselbe
            // Zeichenweg. Ein eigener waere ein zweiter Rasterisierer.
            std::vector<EffectInstance> alle = g_app->effectRunners;
            const std::vector<EffectInstance> schuesse =
                shotEffectsAt(g_app->playMs);
            alle.insert(alle.end(), schuesse.begin(), schuesse.end());
            alle.insert(alle.end(), schwertFx.begin(), schwertFx.end());   // rc601 saber

            // --- Und was daraus entsteht - ueber ALLE Stufen ------------
            //
            // impactFx, deathFx, emitFx und playfx starten weitere Effekte
            // (FxPrimitives.cpp:318, :109, :1477; FxScheduler.cpp:1953).
            // `effectFollowUpsAt` rechnet sie Teilchen fuer Teilchen und
            // Stufe fuer Stufe; die Dateien schlaegt die Anwendung nach.
            //
            // Hier stand bis rc568 "NUR EINE STUFE", mit der Begruendung,
            // keine der Vorlagen brauche mehr. Gezaehlt ueber die 649
            // Dateien von base und MD reichen Ketten aber bis vier Stufen
            // (exegol/exegol_lightning_strikes_rift), und Truemmer, die
            // rauchen, brauchen schon zwei. Die Bremse steckt jetzt in
            // effectFollowUpsAt: kMaxFolgeTiefe und kMaxFolgeEffekte (die
            // 1200 von MAX_EFFECTS).
            //
            // Jeder Folgeeffekt traegt seinen Samen vom ausloesenden
            // Teilchen (EffectImpact::seed) - derselbe Aufprall zeigt also
            // bei jedem Bild und nach dem Zurueckspringen dieselben Funken.
            {
                const std::vector<EffectInstance> folgen = effectFollowUpsAt(
                    alle, g_app->playMs, &g_app->geo,
                    [](const std::string& name) { return effectByName(name); });
                alle.insert(alle.end(), folgen.begin(), folgen.end());
            }
            if (schuesse.size() != g_app->lastShotFxCount) {
                g_app->lastShotFxCount = schuesse.size();
                char sz[110];
                std::snprintf(sz, sizeof(sz),
                              "Schusseffekte bei %.0f ms: %zu",
                              g_app->playMs, schuesse.size());
                diag::detail(sz);
            }
            const auto tE0 = std::chrono::steady_clock::now();
            g_app->effectMesh = baueEffektNetz(alle, useCam, w, h);
            // Und was an Emittern haengt - hier, wo `alle` steht.
            g_app->emitterFlug =
                emitterModelsAt(alle, g_app->playMs, &g_app->geo);
            alleEffekte = std::move(alle);
            kosten.effects = std::chrono::duration<float, std::milli>(
                                 std::chrono::steady_clock::now() - tE0)
                                 .count();
        } else {
            g_app->effectMesh = BspMesh{};
        }
        std::vector<MoverDraw> movers;
        // Die Stellung je Brush-Modell in diesem Bild - fuer ihr "model2" (rc592).
        std::unordered_map<int, MoverDraw> brushStand;

        // --- .md3-Modelle -------------------------------------------------
        //
        // Auch sie reisen als bewegliches Teil durch denselben
        // Aufsammelschritt. Der Unterschied zu den Brush-Modellen: ihre
        // Ecken stehen im Modellraum, nicht in Weltkoordinaten. Deshalb ist
        // pivot der Nullpunkt und offset der Ort der Entity.
        //
        // Animierte Modelle - 163 der 451 Dateien - laufen mit dem
        // Abspielzeiger. Die Bildrate im Spiel ist ueblicherweise 20 Bilder
        // je Sekunde; genau danach richtet sich die Auswahl.
        for (std::size_t mi = 0; mi < g_app->mapModels.size(); ++mi) {
            const App::MapModel& mm = g_app->mapModels[mi];
            // Bewegt es ein Skript? Dann steht es, wo die Szene es hat.
            const int fig = (mi < g_app->modellFigur.size()) ? g_app->modellFigur[mi] : -1;
            ActorState figStand;
            const bool bewegt = fig >= 0 && static_cast<std::size_t>(fig) < g_app->scene.actors.size();
            if (bewegt) {
                figStand = g_app->scene.actors[static_cast<std::size_t>(fig)].at(g_app->playMs);
                if (!figStand.visible) {
                    continue;
                }
            }
            if (mm.anBrush > 0) {
                continue;   // model2 eines Movers - unten mit seinem Brush
            }
            // Ein misc_model_breakable nach use: Schadensmodell, _u1 oder
            // nichts (MoverSim::modellAt, misc_model_breakable_die).
            const MoverSim::ModellStand bruch =
                g_app->moverSim.modellAt(mm.entity, g_app->playMs);
            if (bruch.bekannt && !bruch.sichtbar) {
                continue;
            }
            std::string pfad = mm.path;
            int basis = mm.shaderBase;
            if (bruch.bekannt && bruch.modell != mm.path) {
                const auto nb = g_app->nebenModelle.find(bruch.modell);
                if (nb == g_app->nebenModelle.end()) {
                    continue;   // _d1 fehlt: die Engine zeigt Modell 0, also nichts
                }
                pfad = bruch.modell;
                basis = nb->second;
            }
            const auto have = g_app->md3Files.find(pfad);
            if (have == g_app->md3Files.end()) {
                continue;
            }
            int frame = 0;
            const int bilder = have->second.numFrames;
            // Nach dem Bruch steht die Animation (s.frame = 0).
            if (bilder > 1 && !bruch.angehalten && !mm.standbild) {
                frame = static_cast<int>(g_app->playMs * 0.020) % bilder;
            }
            const std::string key = pfad + "#" + std::to_string(frame);
            auto netz = g_app->md3Meshes.find(key);
            if (netz == g_app->md3Meshes.end()) {
                netz = g_app->md3Meshes
                           .emplace(key, md3ToMesh(have->second, frame,
                                                   basis))
                           .first;
            }
            if (netz->second.indexes.empty()) {
                continue;
            }
            MoverDraw d;
            d.mesh = &netz->second;
            for (int k = 0; k < 3; ++k) {
                d.offset[k] = bewegt ? figStand.pos[k] : mm.origin[k];
                d.pivot[k] = 0.0F;
                d.scale[k] = mm.scale[k];
            }
            d.pitch = bewegt ? figStand.angles[0] : mm.angles[0];
            d.yaw = bewegt ? figStand.angles[1] : mm.angles[1];
            d.roll = bewegt ? figStand.angles[2] : mm.angles[2];
            d.entity = static_cast<int>(mm.entity);
            movers.push_back(d);
        }

        // --- Die Bruchstuecke (CG_Chunks -> LE_FRAGMENT) ---------------
        //
        // Wo jedes Stueck gerade fliegt oder liegt, rechnet MoverSim
        // (truemmerAt). Das Netz je Modell einmal, gedreht und skaliert
        // ueber den MoverDraw - wie die Modelle an den Emittern.
        //
        // OFFEN: das Ausblenden der letzten Sekunde (RF_ALPHA_FADE) - ein
        // MoverDraw kennt keine Deckkraft. Das Stueck verschwindet am Ende
        // seiner Lebenszeit auf einmal.
        for (const Truemmer& tr : g_app->moverSim.truemmer()) {
            const TruemmerStand st = truemmerAt(tr, g_app->playMs);
            if (!st.sichtbar) {
                continue;
            }
            const auto nb = g_app->nebenModelle.find(tr.modell);
            const auto datei = g_app->md3Files.find(tr.modell);
            if (nb == g_app->nebenModelle.end() || datei == g_app->md3Files.end()) {
                continue;
            }
            const std::string key = tr.modell + "#0";
            auto netz = g_app->md3Meshes.find(key);
            if (netz == g_app->md3Meshes.end()) {
                netz = g_app->md3Meshes
                           .emplace(key, md3ToMesh(datei->second, 0, nb->second))
                           .first;
            }
            if (netz->second.indexes.empty()) {
                continue;
            }
            MoverDraw d;
            d.mesh = &netz->second;
            for (int k = 0; k < 3; ++k) {
                d.offset[k] = st.ort[k];
                d.pivot[k] = 0.0F;
                d.scale[k] = st.radius;   // le->radius auf allen drei Achsen
            }
            d.pitch = st.winkel[0];
            d.yaw = st.winkel[1];
            d.roll = st.winkel[2];
            movers.push_back(d);
        }

        // --- Die Modelle an den Emittern -----------------------------
        //
        // "Emitters don't draw themselves, but they may need to add an
        // attached model" (FxPrimitives.cpp:1375). Rauch seit rc323, Ort
        // und Drehung seit rc324 - hier wird endlich gezeichnet.
        for (const EmitterModelDraw& em : g_app->emitterFlug) {
            const auto wo = g_app->emitterModels.find(em.modelPath);
            if (wo == g_app->emitterModels.end()) {
                continue;   // beim Laden nicht gefunden, schon gemeldet
            }
            const auto datei = g_app->md3Files.find(em.modelPath);
            if (datei == g_app->md3Files.end()) {
                continue;
            }
            // Das Netz je Modell einmal bauen, wie bei den Kartenmodellen.
            // Gedreht wird ueber den MoverDraw, NICHT durch ein neues Netz
            // je Bild - sonst baute ein taumelndes Modell sechzig Netze je
            // Sekunde.
            const std::string key = em.modelPath + "#0";
            auto netz = g_app->md3Meshes.find(key);
            if (netz == g_app->md3Meshes.end()) {
                netz = g_app->md3Meshes
                           .emplace(key, md3ToMesh(datei->second, 0,
                                                   wo->second.shaderBase))
                           .first;
            }
            if (netz->second.indexes.empty()) {
                continue;
            }
            MoverDraw d;
            d.mesh = &netz->second;
            for (int k = 0; k < 3; ++k) {
                d.offset[k] = em.origin[k];
                d.pivot[k] = 0.0F;
            }
            // Die Reihenfolge im efx ist Nick/Gier/Roll, wie bei allen
            // Winkeln in Quake-Engines.
            d.pitch = em.angles[0];
            d.yaw = em.angles[1];
            d.roll = em.angles[2];
            // Der Massstab aus der size-Kurve: CEmitter::Draw skaliert die
            // Achsen mit mRefEnt.radius (FxPrimitives.cpp:1394). Bis rc568
            // stand hier nichts, und jedes Modell hatte Groesse eins.
            for (int k = 0; k < 3; ++k) {
                d.scale[k] = em.scale;
            }
            movers.push_back(d);
        }

        {
            // JEDES Untermodell wird gezeichnet, nicht nur die bewegten:
            // die Welt (Modell 0) enthaelt sie ja nicht mehr. Wer kein
            // Skript hat, steht mit Verschiebung null an seinem Platz -
            // also genau dort, wo er vorher auch stand.
            // Wo steht welches Brush-Modell?
            //
            // Ein Brush-Modell mit einem "origin"-Schluessel hat seine
            // Geometrie RELATIV dazu gespeichert - q3map2 zieht den
            // Ursprung ab, sobald ein origin-Brush im Gebilde steckt. Die
            // Engine addiert ihn beim Zeichnen wieder.
            //
            // Genau das fehlte: md_am_sith hat vier solche Gebilde
            // (armpiece1/2, plat1 und das Raumschiff sta2), und alle vier
            // wurden am NULLPUNKT der Karte gezeichnet statt an ihrem
            // Platz. Bei sta2 fiel es auf, weil es sich auch noch bewegen
            // sollte - "die raumschiff animation fehlt".
            //
            // Modell 125 hat Ausmasse von -118 bis 84 um die Null, der
            // Eintrag dazu "origin" "-1514 7376 645". Ohne die Addition
            // liegt das Schiff 7000 Einheiten daneben.
            std::unordered_map<int, std::array<float, 3>> modellOrt;
            for (const MapEntity& e : g_app->map.entities) {
                const std::string* mdl = e.find("model");
                if (mdl == nullptr || mdl->empty() || (*mdl)[0] != '*') {
                    continue;
                }
                int nummer = 0;
                if (!parseInt(mdl->substr(1), nummer)) {
                    continue;
                }
                std::array<float, 3> o{0.0F, 0.0F, 0.0F};
                if (!e.origin.empty()) {
                    std::istringstream is(e.origin);
                    is >> o[0] >> o[1] >> o[2];
                }
                modellOrt[nummer] = o;
            }
            std::vector<bool> schonDa(g_app->geo.models.size(), false);
            // rc604: Brush-Modelle entfernter Entities (notsingle/Schwierigkeit,
            // G_SpawnGEntityFromSpawnVars) gibt es im Spiel nicht.
            for (const MapEntity& e : g_app->map.entities) {
                const std::string* mdl = e.find("model");
                int nummer = 0;
                if (mdl == nullptr || mdl->size() < 2 || (*mdl)[0] != '*' || !parseInt(mdl->substr(1), nummer)) { continue; }
                if (nummer > 0 && static_cast<std::size_t>(nummer) < schonDa.size() && !gespawntImSpiel(e)) {
                    schonDa[static_cast<std::size_t>(nummer)] = true;
                }
            }
            brushStand.clear();
            for (const Actor& a : g_app->scene.actors) {
                if (a.brushModel <= 0 || !a.haveStart ||
                    static_cast<std::size_t>(a.brushModel) >=
                        schonDa.size()) {
                    continue;   // 0 waere die Welt selbst
                }
                schonDa[static_cast<std::size_t>(a.brushModel)] = true;
            }
            for (std::size_t k = 1; k < g_app->geo.models.size(); ++k) {
                if (schonDa[k]) {
                    continue;   // kommt gleich mit seiner Bewegung
                }
                const auto have = g_app->brushMeshes.find(static_cast<int>(k));
                if (have == g_app->brushMeshes.end()) {
                    g_app->brushMeshes.emplace(
                        static_cast<int>(k),
                        // Der Himmelsschalter gilt auch fuer bewegliche
                        // Teile - siehe die Vorgabe in bspgeo.h.
                        buildModelMesh(g_app->geo, static_cast<int>(k),
                                       g_app->meshDetail,
                                       !g_app->showSky));
                }
                const BspMesh& bm = g_app->brushMeshes[static_cast<int>(k)];
                if (bm.indexes.empty()) {
                    continue;
                }
                MoverDraw m;
                m.mesh = &bm;
                m.brush = static_cast<int>(k);
                // Tueren, func_wall & Co.: der Stand aus der Mover-
                // Simulation (bhed/mover.h) - Ort, Winkel, sichtbar.
                const MoverSim::Stand ms =
                    g_app->moverSim.at(static_cast<int>(k), g_app->playMs);
                if (ms.bekannt) {
                    if (!ms.sichtbar) {
                        continue;   // func_wall/func_usable START_OFF
                    }
                    for (int q = 0; q < 3; ++q) {
                        m.offset[q] = ms.origin[q];
                    }
                    m.pitch = ms.angles[0];
                    m.yaw = ms.angles[1];
                    m.roll = ms.angles[2];
                    movers.push_back(m);
                    brushStand[static_cast<int>(k)] = m;
                    continue;
                }
                // Der Ursprung der Entity ist die Verschiebung. Fuer die
                // allermeisten Modelle ist er null - dann steht es, wo es
                // steht, wie bisher.
                const auto ort = modellOrt.find(static_cast<int>(k));
                if (ort != modellOrt.end()) {
                    for (int q = 0; q < 3; ++q) {
                        m.offset[q] = ort->second[static_cast<std::size_t>(q)];
                    }
                }
                movers.push_back(m);
                brushStand[static_cast<int>(k)] = m;
            }
        }
        if (g_app->showActors && !g_app->scene.actors.empty()) {
            for (const Actor& a : g_app->scene.actors) {
                if (a.brushModel <= 0 || !a.haveStart) {
                    continue;   // 0 waere die Welt selbst
                }
                const auto have = g_app->brushMeshes.find(a.brushModel);
                if (have == g_app->brushMeshes.end()) {
                    g_app->brushMeshes.emplace(
                        a.brushModel,
                        buildModelMesh(g_app->geo, a.brushModel, g_app->meshDetail));
                }
                const BspMesh& bm = g_app->brushMeshes[a.brushModel];
                if (bm.indexes.empty()) {
                    continue;
                }
                const ActorState st = a.at(g_app->playMs);
                if (!st.visible) {
                    continue;
                }
                MoverDraw m;
                m.mesh = &bm;
                m.brush = a.brushModel;
                // Verschoben wird um den AKTUELLEN Ursprung, nicht um die
                // Differenz zum Startort.
                //
                // Der Grund ist derselbe wie oben: die Geometrie liegt
                // relativ zum Ursprung. "Aktueller Ort minus Startort"
                // stimmte nur scheinbar - beim Start kam null heraus, und
                // damit stand das Schiff am Nullpunkt der Karte statt an
                // seinem Platz. Gedreht wird um die Null der Geometrie,
                // also um den Ursprung der Entity - dort liegt er.
                for (int k = 0; k < 3; ++k) {
                    m.offset[k] = st.pos[k];
                    m.pivot[k] = 0.0F;
                }
                // Die Winkel ABSOLUT, mit Nicken und Rollen - so zeichnet
                // die Engine currentAngles. Vorher nur die Gier, und die
                // relativ zum Kartenwinkel: kippende Schiffe, Luken und
                // Zugbruecken standen still, und ein rotate auf eine Tuer
                // mit "angle" tat nichts.
                m.pitch = st.angles[0];
                m.yaw = st.angles[1];
                m.roll = st.angles[2];
                movers.push_back(m);
                brushStand[a.brushModel] = m;
            }
        }
        // --- "model2" der Brush-Mover (rc592) -----------------------------
        //
        // Dieselbe Stellung wie ihr Brush (Ort und alle drei Winkel) - wie
        // CG_Mover das Zweitmodell an lerpOrigin/lerpAngles zeichnet. Ist der
        // Brush gerade nicht zu sehen (START_OFF), fehlt auch das Modell.
        for (const App::MapModel& mm : g_app->mapModels) {
            if (mm.anBrush <= 0) { continue; }
            const auto st = brushStand.find(mm.anBrush);
            if (st == brushStand.end()) { continue; }
            const auto have = g_app->md3Files.find(mm.path);
            if (have == g_app->md3Files.end()) { continue; }
            const std::string key = mm.path + "#0";
            auto netz = g_app->md3Meshes.find(key);
            if (netz == g_app->md3Meshes.end()) {
                netz = g_app->md3Meshes.emplace(key, md3ToMesh(have->second, 0, mm.shaderBase)).first;
            }
            if (netz->second.indexes.empty()) { continue; }
            MoverDraw d = st->second;
            d.mesh = &netz->second;
            d.entity = static_cast<int>(mm.entity);
            for (int k = 0; k < 3; ++k) {
                d.pivot[k] = 0.0F;
                d.scale[k] = 1.0F;
            }
            movers.push_back(d);
        }
        // --- Wo die Entities gerade stehen (rc597) -------------------------
        //
        // Marke, Kasten und Gizmo folgen dem, was Szene und Mover-Simulation
        // bewegen: Brush-Modelle ueber ihre Nummer, Figuren ueber den Namen
        // ihres Spawners.
        // Zieht man gerade ein Schiff oder ein Modell, geht es live mit.
        if (g_app->entZug.aktiv) {
            float zv[3];
            float zg = 0.0F;
            const int zi = g_app->entZug.index;
            if (zugVersatz(zi, zv, zg)) {
                int nr = -1;
                if (const std::string* mdl = g_app->map.entities[static_cast<std::size_t>(zi)].find("model");
                    mdl != nullptr && mdl->size() > 1 && (*mdl)[0] == '*') {
                    nr = std::atoi(mdl->c_str() + 1);
                }
                float dn = 0.0F;
                float dr = 0.0F;
                (void)zugKippen(zi, dn, dr);
                for (MoverDraw& mv : movers) {
                    if (mv.entity == zi || (nr > 0 && mv.brush == nr)) {
                        for (int k = 0; k < 3; ++k) { mv.offset[k] += zv[k]; }
                        mv.yaw += zg;
                        mv.pitch += dn;
                        mv.roll += dr;
                    }
                }
            }
        }
        // --- rc604: surfaceSprites - die Halme dieses Bildes ----------------
        //
        // Wie die Engine je Bild auf der CPU (RB_DrawSurfaceSprites), fuer
        // die Kamera, durch die das Bild entsteht. rangescalefactor
        // (tr_surfacesprites.cpp:129-161): ein engerer Bildwinkel als der
        // erste (der Spieler, cg_fov 80) zeigt die Halme entsprechend weiter.
        // rc605: erst nach den Movern - auch die Flaechen der Brush-Modelle
        // tragen Halme, mit der Stellung dieses Bildes.
        {
            const auto tG0 = std::chrono::steady_clock::now();
            float grasReichweite = 1.0F;
            if (playingNow || amZeiger || through) {
                grasReichweite = gras::reichweiteFaktor(spielSicht(g_app->camViewFov, spiel).fovX,
                                                        spielSicht(80.0F, spiel).fovX);
            }
            baueGrasNetz(useCam, grasReichweite, &movers);
            kosten.effects += std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - tG0).count();
        }
        g_app->entVersatz.clear();
        g_app->entGierVersatz.clear();
        g_app->pickMover.clear();
        {
            std::unordered_map<int, int> entityVonBrush;
            for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                const MapEntity& e = g_app->map.entities[i];
                const std::string* mdl = e.find("model");
                if (mdl == nullptr || mdl->size() < 2 || (*mdl)[0] != '*') { continue; }
                const int nr = std::atoi(mdl->c_str() + 1);
                if (nr <= 0) { continue; }
                entityVonBrush[nr] = static_cast<int>(i);
                const auto st = brushStand.find(nr);
                float o[3];
                if (st == brushStand.end() || !entityOrt(e, o)) { continue; }
                const std::array<float, 3> v{st->second.offset[0] - o[0], st->second.offset[1] - o[1],
                                             st->second.offset[2] - o[2]};
                if (std::fabs(v[0]) + std::fabs(v[1]) + std::fabs(v[2]) > 0.5F) {
                    g_app->entVersatz[static_cast<int>(i)] = v;
                }
            }
            if (g_app->showActors) {
                // Die Blickrichtung jeder Figur (auch eines Schiffs mit Skript).
                const auto gier = [](int ei, const ActorState& st) {
                    if (ei < 0 || !st.visible) { return; }
                    const std::string* w = g_app->map.entities[static_cast<std::size_t>(ei)].find("angle");
                    const float schl = w != nullptr ? std::strtof(w->c_str(), nullptr) : 0.0F;
                    const float d = std::remainder(st.angles[1] - schl, 360.0F);
                    if (std::fabs(d) > 0.5F) { g_app->entGierVersatz[ei] = d; }
                };
                for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
                    const Actor& a = g_app->scene.actors[ai];
                    if (!a.haveStart) { continue; }
                    if (a.brushModel > 0) {
                        const auto it = entityVonBrush.find(a.brushModel);
                        if (it != entityVonBrush.end()) { gier(it->second, a.at(g_app->playMs)); }
                        continue;
                    }
                    const int ei = entityFuerFigur(static_cast<int>(ai));
                    if (ei < 0) { continue; }
                    gier(ei, a.at(g_app->playMs));
                    // Bis zur Figur selbst - ihr Startort in der Szene ist
                    // nicht immer der Spawner (ani1 in md_am_sith).
                    const ActorState st = a.at(g_app->playMs);
                    float o[3];
                    if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(ei)], o)) { continue; }
                    const std::array<float, 3> v{st.pos[0] - o[0], st.pos[1] - o[1], st.pos[2] - o[2]};
                    if (st.visible && std::fabs(v[0]) + std::fabs(v[1]) + std::fabs(v[2]) > 0.5F) {
                        g_app->entVersatz[ei] = v;
                    }
                }
            }
            // Waehrend des Ziehens bleibt der Versatz vom Anfassen - sonst
            // sprang der Gizmo zwischen altem und neuem Stand hin und her.
            if (g_app->entZug.aktiv && g_app->entZug.index >= 0 && g_app->entZug.zwischen) {
                // Neuer Bahnpunkt (rc597): der Gizmo steht am gezogenen Punkt.
                float o[3];
                if (entityOrt(g_app->map.entities[static_cast<std::size_t>(g_app->entZug.index)], o)) {
                    const float* k = g_app->entZug.zwOrt;
                    g_app->entVersatz[g_app->entZug.index] = {k[0] - o[0], k[1] - o[1], k[2] - o[2]};
                }
            } else if (g_app->entZug.aktiv && g_app->entZug.index >= 0) {
                if (g_app->entZug.gierVersatz != 0.0F) {
                    g_app->entGierVersatz[g_app->entZug.index] = g_app->entZug.gierVersatz;
                } else {
                    g_app->entGierVersatz.erase(g_app->entZug.index);
                }
                const float* v = g_app->entZug.versatz;
                if (v[0] != 0.0F || v[1] != 0.0F || v[2] != 0.0F) {
                    g_app->entVersatz[g_app->entZug.index] = {v[0], v[1], v[2]};
                } else {
                    g_app->entVersatz.erase(g_app->entZug.index);
                }
            }
            for (const MoverDraw& mv : movers) {
                App::PickMover pm;
                pm.mesh = mv.mesh;
                pm.entity = mv.entity >= 0 ? mv.entity : -1;
                if (pm.entity < 0 && mv.brush > 0) {
                    const auto it = entityVonBrush.find(mv.brush);
                    if (it != entityVonBrush.end()) { pm.entity = it->second; }
                }
                if (pm.entity < 0 || pm.mesh == nullptr) { continue; }
                gpu::baueMoverWelt(mv.pivot, mv.offset, mv.yaw, mv.pitch, mv.roll, mv.pitch != 0.0F || mv.roll != 0.0F,
                                   pm.welt, mv.scale);
                g_app->pickMover.push_back(pm);
            }
        }
        // Die Figuren NACH der Karte: sie benutzen deren Tiefenpuffer und
        // verschwinden dadurch hinter Waenden.
        const auto tS0 = std::chrono::steady_clock::now();
        // Die Lichter, die Effekte gerade werfen - EINMAL fuer alle
        // Figuren, nicht je Figur neu. In md_ga_jedi laufen 28 fx_runner;
        // sie je Figur durchzugehen waere 28 mal 62 Durchlaeufe je Bild.
        const std::vector<EffectLight> fxLichter =
            g_app->showEffects
                ? effectLightsAt(alleEffekte.empty() ? g_app->effectRunners
                                                     : alleEffekte,
                                 g_app->playMs)
                : std::vector<EffectLight>{};
        // Dieselben Lichter fuer die FIGUREN - dazu die Schwerter des
        // letzten Bildes, und alles OHNE den Helligkeitsfaktor. Der steht
        // fuer R_ColorShiftLightingBytes und hebt nur das Lichtgitter; die
        // dynamischen Lichter rechnet R_SetupEntityLighting erst danach
        // obendrauf (tr_light.cpp:435 ff.). Weil behaved das Gitterlicht
        // erst beim Zeichnen mit mapBrightness malnimmt, werden sie hier
        // vorab durch denselben Faktor geteilt.
        std::vector<EffectLight> figurLichter = fxLichter;
        if (g_app->showDynLights) {
            figurLichter.insert(figurLichter.end(), g_app->schwertLichter.begin(),
                                g_app->schwertLichter.end());
        }
        {
            const float durch = 1.0F / std::max(g_app->mapBrightness, 0.01F);
            for (EffectLight& l : figurLichter) {
                for (float& c : l.color) { c *= durch; }
            }
        }
        if (g_app->showActors && !g_app->scene.actors.empty()) {
            if (g_app->actorAssets.size() != g_app->scene.actors.size()) {
                loadActorModels();
                kinoSkeletteAbgleichen();
                // Griffe und Klingenbilder gleich mit - siehe
                // preloadSaberAssets. Sonst fallen sie mitten in ein Bild.
                preloadSaberAssets();
                // Die Effekte der Karte gleich mit: sie haengen an den
                // Entities, nicht am Skript, und aendern sich erst mit der
                // naechsten Karte.
                loadMapEffects();
                // Die .md3-Modelle danach: sie haengen ihre Texturen hinten
                // an textures.byShader an, und die Karte muss vorher stehen.
                loadMapModels();
            }
            std::vector<ActorDraw> draws;
            draws.reserve(g_app->scene.actors.size());
            for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
                const Actor& a = g_app->scene.actors[ai];
                if (ai >= g_app->actorAssets.size()) {
                    break;
                }
                const App::ActorAssets& assets = *g_app->actorAssets[ai];
                if (assets.model.empty() || !a.haveStart) {
                    continue;
                }
                const ActorState st = a.at(g_app->playMs);
                if (!st.visible) {
                    continue;
                }
                // rc602: eine Figur ausserhalb des PVS der Ansicht zeichnet auch
                // das Spiel nicht - der Server schickt sie gar nicht
                // (SV_AddEntitiesVisibleFromPoint, sv_snapshot.cpp; Kasten wie
                // SV_LinkEntity um 1 erweitert, sv_world.cpp:296 ff.). Seit die
                // Figuren vom Kartenstart dazukommen (md_am_sith: 39 statt 29),
                // kostete das Rechnen ihrer Knochen sonst jedes Bild.
                // kastenImSnapshot ist vorsichtig: Auge im Fels oder ohne
                // Sichtbarkeitstabelle gilt alles als sichtbar.
                if (!g_app->draufsicht && !g_app->entZug.aktiv) {
                    float mn[3];
                    float mx[3];
                    for (int k = 0; k < 2; ++k) {
                        mn[k] = st.pos[k] - a.halbBreite - 1.0F;
                        mx[k] = st.pos[k] + a.halbBreite + 1.0F;
                    }
                    mn[2] = st.pos[2] - 24.0F - 1.0F;   // DEFAULT_MINS_2
                    mx[2] = st.pos[2] + std::max(a.hoeheOben, 8.0F) + 1.0F;
                    if (!kastenImSnapshot(g_app->geo, useCam.pos, mn, mx)) {
                        continue;
                    }
                }
                ActorDraw d;
                d.figur = static_cast<int>(ai);
                d.model = &assets.model;
                d.textures = (assets.textures.found != 0) ? &assets.textures
                                                          : nullptr;
                // Das Skelett der FIGUR, nicht das der Modellansicht.
                //
                // Vorher stand hier g_app->anim - das von Hand geladene.
                // Wer eine Mission oeffnet, hat dort nichts geladen, also
                // war es leer, und jede Figur blieb in Ruhelage stehen.
                // Der Pfad der .gla steht im Kopf des Modells; skeletonFor()
                // holt sie und die animation.cfg dazu.
                // rc601 saber: herausgezogen (figurBilderSetzen), damit die
                // Schwertproben (app_saber.cpp) dieselben Bilder rechnen.
                figurBilderSetzen(a, assets, st, g_app->playMs, d);
                for (int k = 0; k < 3; ++k) {
                    d.pos[k] = st.pos[k];
                }
                d.yaw = st.angles[1];
                // Zieht man gerade ihren Spawner, geht sie mit (rc597) - "wenn
                // ich den character bewege, shaked der gizmo, statt den
                // character in real time zu bewegen". Die Szene baut erst beim
                // Loslassen neu.
                if (g_app->entZug.aktiv) {
                    float zv[3];
                    float zg = 0.0F;
                    if (zugVersatz(entityFuerFigur(static_cast<int>(ai)), zv, zg)) {
                        for (int k = 0; k < 3; ++k) { d.pos[k] += zv[k]; }
                        d.yaw += zg;
                    }
                }
                if (g_app->figurGezeichnet.size() != g_app->scene.actors.size()) {
                    g_app->figurGezeichnet.assign(g_app->scene.actors.size(), {0.0F, 0.0F, 0.0F});
                }
                g_app->figurGezeichnet[ai] = {d.pos[0], d.pos[1], d.pos[2]};
                // Das Licht an ihrem Standort, aus dem Gitter der Karte.
                //
                // Die Engine nimmt dafuer den URSPRUNG der Entity
                // (R_SetupEntityLighting, tr_light.cpp:401), nicht die
                // Mitte des Modells - eine Figur wird also nach dem Licht
                // an ihren Fuessen beleuchtet. Das ist grob, aber es ist,
                // was das Spiel zeigt.
                d.light = sampleLightGrid(g_app->geo, d.pos);
                // Dazu die Lichter, die Effekte gerade werfen. Die Engine
                // rechnet sie an derselben Stelle drauf
                // (R_SetupEntityLighting, tr_light.cpp:435 ff.).
                addDynamicLights(d.light, d.pos, figurLichter);
                d.schattenRadius = schattenRadiusFuer(a.npcType, &d.schattenFuesse);
                npcSkalaFuer(a.npcType, d.skala);

                // --- Waffe, Lichtschwerter, Kinomodelle -------------
                //
                // Alles aus dem Zustand der Figur (ActorState::hand,
                // saber, kino) - siehe src/ausruestung.cpp. Der GRIFF haengt,
                // solange er angehaengt ist, auch mit ausgeschalteter
                // Klinge; die KLINGEN nur, solange sie laenger als 0.5 sind.
                anbautenFuer(a, st, g_app->playMs, d);
                // --- Wohin der Kopf schaut ---------------------------
                //
                // Die Richtung zum Blickziel MINUS der Richtung, in die
                // der Koerper schaut - der Kopf dreht sich ja gegen den
                // Koerper, nicht gegen die Welt. Beschnitten auf die 64
                // Grad, die die Engine zulaesst; wer weiter schauen soll,
                // braucht ein SET_WATCHTARGET.
                figurKopfGier(st, d);   // rc601 saber: herausgezogen
                d.yawOffset = static_cast<float>(g_app->actorYaw);
                draws.push_back(d);
            }
            if (!draws.empty()) {
                kosten.scene = std::chrono::duration<float, std::milli>(
                                   std::chrono::steady_clock::now() - tS0)
                                   .count();
                const auto tA0 = std::chrono::steady_clock::now();
                // Merken - gezeichnet wird am Ende des Blocks, nach der
                // Karte, damit die Figuren auf deren Tiefe treffen.
                gpuFiguren = draws;
                kosten.actors = std::chrono::duration<float, std::milli>(
                                    std::chrono::steady_clock::now() - tA0)
                                    .count();
            }
        }
        const auto tG0 = std::chrono::steady_clock::now();
        // Ohne Gizmos auch keine Klickziele: sonst bleiben die Stellen des
        // letzten freien Bildes stehen, und ein Klick in den Kamerablick
        // waehlt etwas, das dort gar nicht zu sehen ist.
        if (g_app->camViewActive || playingNow) {
            g_app->markers.clear();
            g_app->keyMarks.clear();
            g_app->actorMarks.clear();
            g_app->cameraSx = -1.0F;
            g_app->cameraSy = -1.0F;
        }

        const auto t1 = std::chrono::steady_clock::now();
        const auto ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
        kosten.gizmos =
            std::chrono::duration<float, std::milli>(t1 - tG0).count();
        kosten.total = ms;
        // Geglaettet: eine ungeglaettete Anzeige springt zu stark.
        g_app->mapFrameMs = (g_app->mapFrameMs <= 0.0F)
                                ? ms
                                : g_app->mapFrameMs * 0.9F + ms * 0.1F;
        // Dieselbe Glaettung fuer die Einzelteile, damit sie sich mit der
        // Gesamtzeit vergleichen lassen.
        auto glatt = [](float& alt2, float neu2) {
            alt2 = (alt2 <= 0.0F) ? neu2 : alt2 * 0.9F + neu2 * 0.1F;
        };
        glatt(g_app->frameCost.scene, kosten.scene);
        glatt(g_app->frameCost.effects, kosten.effects);
        glatt(g_app->frameCost.map, kosten.map);
        glatt(g_app->frameCost.actors, kosten.actors);
        glatt(g_app->frameCost.glow, kosten.glow);
        glatt(g_app->frameCost.gizmos, kosten.gizmos);
        glatt(g_app->frameCost.total, kosten.total);
        // Ungeglaettet daneben - siehe FrameCost::rawMap.
        g_app->frameCost.rawMap = kosten.map;
        g_app->frameCost.rawTotal = kosten.total;
        g_app->frameCost.rawSaber = kosten.rawSaber;
        g_app->frameCost.rawSaberFx = kosten.rawSaberFx;
        g_app->frameCost.rawSpuren = kosten.rawSpuren;
        g_app->frameCost.rawEffects = kosten.effects;
        g_app->frameCost.rawScene = kosten.scene;

        // --- Die Aufschluesselung ins Protokoll --------------------------
        //
        // Gefragt: "hast du jetzt eingebaut im Debug-Log, was wie viel
        // kostet und warum?"
        //
        // Bis hierher stand sie nur im Mauszeiger-Kasten und hinter einem
        // Menuepunkt. Beim Abspielen muesste man also gleichzeitig
        // hinsehen - und genau dort ist es am langsamsten.
        //
        // Jetzt schreibt sie sich selbst, sobald sich die Gesamtzeit um
        // mehr als ein Fuenftel aendert. Nicht in jedem Bild: bei sechzig
        // Bildern je Sekunde waere das Protokoll unlesbar, und die Zeiten
        // schwanken ohnehin staendig um ein paar Prozent. Dasselbe Mass
        // wie beim Keulen und bei den Effektlichtern.
        //
        // Das "warum" steht mit dabei: die Zeile nennt jede Stufe einzeln,
        // dazu die Groesse des Bildes und die Zahl der Figuren. Ohne die
        // beiden weiss man zwar, WAS teuer ist, aber nicht WOMIT es
        // verglichen gehoert.
        {
            // --- ROH melden, nicht geglaettet ----------------------------
            //
            // Aus einem Protokoll: die Zahl "Szene" lief ueber ZWOELF
            // Zeilen von 490 auf 0,1 herunter, jede Zeile ein Verhaeltnis
            // von 0,729 zur vorigen. Das ist keine Messung, das ist meine
            // Glaettung (0,9 je Bild), die EIN teures Bild vierzig Bilder
            // lang nachhallen laesst - und meine Regel "melde bei 20 %
            // Aenderung" hat jeden Abklingschritt als Neuigkeit gemeldet.
            //
            // Ein Messgeraet, das seinen eigenen Nachhall als Befund
            // ausgibt, ist schlimmer als keines: man sucht dann nach einer
            // Ursache fuer etwas, das langst vorbei ist.
            //
            // Fuer die ANZEIGE bleibt die Glaettung richtig - eine
            // springende Zahl liest sich nicht. Fuers PROTOKOLL zaehlt der
            // rohe Wert DIESES Bildes: ein Ausreisser gibt dann genau eine
            // Zeile, und die naechste sagt schon wieder die Wahrheit.
            const App::FrameCost& k = kosten;
            // WICHTIG: die ganze Messung sitzt innerhalb von `mapDirty`.
            // Wird das Bild nicht neu gezeichnet, laufen weder Karte noch
            // Figuren - und dann stuenden hier lauter Nullen, die wie ein
            // Befund aussaehen und keiner sind. Deshalb wird nur
            // geschrieben, wenn wirklich gezeichnet wurde.
            const bool ersteMal = g_app->lastFrameMsLogged < 0.0F;
            // Dreissig Prozent UND mindestens drei Millisekunden. Ohne
            // die zweite Bedingung meldete ein Sprung von 0,3 auf 0,4 ms
            // genauso laut wie einer von 20 auf 60.
            const bool starkAnders =
                !ersteMal &&
                std::fabs(k.total - g_app->lastFrameMsLogged) >
                    std::max(g_app->lastFrameMsLogged * 0.3F, 3.0F);
            if ((ersteMal && k.total > 0.0F) || starkAnders) {
                g_app->lastFrameMsLogged = k.total;
                char zz[320];
                std::snprintf(
                    zz, sizeof(zz),
                    "Zeit je Bild %.1f ms (%.0f/s): Karte %.1f, Figuren %.1f,"
                    " Effekte %.1f, Szene %.1f, Gluehen %.1f,"
                    " Gizmos %.1f"
                    "  |  Bild %dx%d, %zu Figuren, %s",
                    static_cast<double>(k.total),
                    (k.total > 0.01F) ? 1000.0 / static_cast<double>(k.total)
                                      : 0.0,
                    static_cast<double>(k.map), static_cast<double>(k.actors),
                    static_cast<double>(k.effects),
                    static_cast<double>(k.scene),
                    static_cast<double>(k.glow),
                    static_cast<double>(k.gizmos), g_app->mapImage.width,
                    g_app->mapImage.height, g_app->scene.actors.size(),
                    g_app->playing ? "spielt" : "steht");
                diag::detail(zz);
            }
        }
        // --- Der GPU-Weg -------------------------------------------------
        //
        // HIER und nicht weiter oben: an dieser Stelle sind Szene und
        // Effektnetz fertig aufgebaut. Vor rc400 stand der Aufruf vor dem
        // Block, und das Effektnetz existierte noch gar nicht.
        //
        if (gpuBereit) {
            // Lichtschalter und Flackern zur Zeit des Bildes (rc593).
            lichtStileZuZeit();
            // Vorwaermen (rc596): Shader und Effektbilder anlegen, sobald Karte
            // oder Effekte neu sind - nicht erst beim ersten Gebrauch mitten im
            // Fliegen (816 ms Ruckler, Lichtshader). Einmal je Stand.
            {
                const std::size_t stand = reinterpret_cast<std::uintptr_t>(&g_app->mesh) ^
                                          (g_app->mesh.batches.size() * 1000003U) ^
                                          (g_app->textures.byShader.size() * 7919U) ^
                                          (g_app->effectShaders.size() * 104729U) ^
                                          (static_cast<std::size_t>(g_app->texCacheStand) << 20U);
                if (stand != g_app->vorgewaermt) {
                    g_app->vorgewaermt = stand;
                    std::vector<int> plaetze;
                    plaetze.reserve(g_app->effectShaders.size());
                    for (const auto& [name, platz] : g_app->effectShaders) {
                        (void)name;
                        plaetze.push_back(platz);
                    }
                    diag::Step step("Shader vorwaermen");
                    const int neu = gpu::vorwaermen(g_app->mesh, &g_app->textures, plaetze);
                    diag::detail("Vorgewaermt: " + std::to_string(neu) + " Shader neu, " +
                                 std::to_string(plaetze.size()) + " Effektbilder");
                }
            }
            gpu::neuesBild();
            {
                // Hintergrund: die Farbe des globalen Nebels, wie RB_DrawBuffer -
                // unabhaengig vom Nebelschalter (r_drawfog), wie im Spiel.
                float hg[3] = {0.0F, 0.0F, 0.0F};
                for (const BspGeometry::Nebel& n : g_app->geo.nebel) {
                    if (!n.global || !n.gueltig) { continue; }
                    std::string k = n.shader;
                    for (char& ch : k) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
                    const auto it = g_app->nebelMap.find(k);
                    if (it != g_app->nebelMap.end()) {
                        for (int c = 0; c < 3; ++c) { hg[c] = it->second.farbe[c]; }
                    }
                    break;
                }
                gpu::setzeHintergrund(hg[0], hg[1], hg[2]);
            }
            const auto tGpu0 = std::chrono::steady_clock::now();
            // --- Ein GPU-Bild aus einer Kamera (rc592) -----------------------
            //
            // Der ganze Durchgang als eine Funktion: einmal fuer die Ansicht,
            // und - wenn eingeschaltet - ein zweites Mal klein fuer die
            // Bild-im-Bild-Vorschau der Skriptkamera (nebenbild, Zielsatz 1).
            // Das Nebenbild misst nicht, glueht nicht und zaehlt nicht mit.
            // Was ist gewaehlt (fuer den Umriss, rc597)? Entities direkt
            // (Kartenmodelle), ihr Brush-Modell ("model" "*N") und ihr
            // Skriptname - an dem haengen die Figuren der Szene.
            struct Auswahl {
                std::set<int> entities;
                std::set<int> brushes;
                std::set<std::string> namen;
                int figurNr = -1;
                [[nodiscard]] bool leer() const { return entities.empty() && brushes.empty() && namen.empty() && figurNr < 0; }
                [[nodiscard]] bool mover(const MoverDraw& m) const {
                    return (m.entity >= 0 && entities.count(m.entity) != 0U) || (m.brush > 0 && brushes.count(m.brush) != 0U);
                }
                [[nodiscard]] bool figur(int nr) const {
                    if (nr < 0) { return false; }
                    if (nr == figurNr) { return true; }
                    if (static_cast<std::size_t>(nr) >= g_app->scene.actors.size()) { return false; }
                    std::string n = g_app->scene.actors[static_cast<std::size_t>(nr)].name;
                    for (char& ch : n) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
                    return namen.count(n) != 0U;
                }
            };
            Auswahl auswahl;
            for (const int i : entityAuswahl()) {
                if (i < 0 || static_cast<std::size_t>(i) >= g_app->map.entities.size()) { continue; }
                const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(i)];
                auswahl.entities.insert(i);
                if (const std::string* mdl = e.find("model"); mdl != nullptr && mdl->size() > 1 && (*mdl)[0] == '*') {
                    auswahl.brushes.insert(std::atoi(mdl->c_str() + 1));
                }
                std::string n = entitySkriptName(e);
                for (char& ch : n) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
                if (!n.empty()) { auswahl.namen.insert(n); }
            }
            auswahl.figurNr = g_app->selectedActor;
            struct UmrissFigur {
                const GlmModel* model = nullptr;
                const ModelTextures* textures = nullptr;
                std::vector<BoneMatrix> knochen;
                float welt[16];
            };
            std::vector<UmrissFigur> umrissFiguren;
            // Das Effektnetz einer Nebenansicht mit eigener Kamera (rc597);
            // nullptr = das der Hauptansicht.
            BspMesh effektNetzVorschau;
            const BspMesh* effektNetzBild = nullptr;
            auto gpuBild = [&](const Camera& bildCam, const int bildW, const int bildH, const bool nebenbild) {
                float vp[16];
                // Draufsicht: die vordere Ebene liegt auf der Schnitthoehe -
                // alles darueber (Decken, Daecher) faellt weg.
                const float nah = (g_app->draufsicht && !g_app->throughCamera)
                                      ? std::max(4.0F, bildCam.pos[2] - g_app->draufSchnitt)
                                      : 4.0F;
                gpu::baueViewProj(bildCam, bildW, bildH, nah, kartenFern() + (nah > 4.0F ? nah : 0.0F), vp);
                // --- Spiegeldurchgang (rc593) ---------------------------------
                //
                // Statt der gespiegelten Kamera (die sich mit Winkeln nicht
                // beschreiben laesst - sie ist seitenverkehrt) wird die WELT
                // an der Spiegelebene gespiegelt: vp x R. Die Kameraposition
                // An die Zeichner geht weiter die ECHTE Kameraposition: sie
                // waehlen damit auch die sichtbaren Blaetter (PVS), und die
                // gespiegelte liegt hinter dem Spiegel im Leeren.
                const bool spiegeln = nebenbild && g_app->imSpiegelDurchgang;
                const App::NebenSicht& ns = g_app->nebenSicht;
                auto spiegle = [&](float* v3) {
                    const float alt[3] = {v3[0], v3[1], v3[2]};
                    for (int z = 0; z < 3; ++z) {
                        v3[z] = ns.achsen[z * 3 + 0] * alt[0] + ns.achsen[z * 3 + 1] * alt[1] + ns.achsen[z * 3 + 2] * alt[2];
                    }
                };
                if (spiegeln) {
                    gpu::multipliziere(vp, ns.welt, vp);
                    gpu::setzeSpiegelung(true, ns.schnitt, ns.umkehren);
                }
                // Die Kameraachsen fuer den Himmel - aus derselben Quelle
                // wie der Rasterer, sonst zeigt der Himmel woandershin.
                float gvorn[3];
                float grechts[3];
                float ghoch[3];
                Camera hc = bildCam;
                hc.forward(gvorn);
                hc.right(grechts);
                hc.up(ghoch);
                if (spiegeln) {
                    // Der Himmel blickt im Spiegel/Portal in die abgebildete Richtung.
                    spiegle(gvorn);
                    spiegle(grechts);
                    spiegle(ghoch);
                }
                const float gfokus =
                    1.0F / std::tan(bildCam.fovY * 3.14159265F / 180.0F * 0.5F);
                std::string f;
                const float zeit = static_cast<float>(g_app->playMs) * 0.001F;
                // Alle vier werden jetzt ADDIERT (zwei Durchgaenge),
                // deshalb hier auf null - auch gpuAufrufe und
                // gpuUebersprungen, die vorher zugewiesen wurden.
                g_app->gpuKarte = g_app->gpuMover = 0;
                g_app->gpuFiguren2 = g_app->gpuGluehen = 0;
                g_app->gpuEffekte = 0;
                g_app->gpuAufrufe = 0;
                g_app->gpuUebersprungen = 0;
                // Die Klammer um das ganze Bild. Sie muss VOR der ersten
                // Abschnittsmarke stehen; ohne sie wird gar nicht gemessen.
                if (!nebenbild) { gpu::bildBeginnt(); }
                if (!nebenbild) { g_app->grasNebelAufrufe = 0; }

                // --- Das Bild laeuft ZWEIMAL ueber alle Quellen ---------
                //
                // Erst alles Deckende, dann alles Gemischte. Vorher lief es
                // einmal, Quelle fuer Quelle: Karte ganz, dann Mover ganz.
                // Jede Liste war IN SICH richtig sortiert - und trotzdem
                // standen die gemischten Kartenflaechen vor den deckenden
                // Tueren. Das war der Fehler "Tueren verschwinden hinter
                // durchsichtigen Flaechen".
                //
                // Die Engine hat ihn nicht, weil sie gar nicht nach Quellen
                // zeichnet: R_AddDrawSurf legt Welt, Mover und Figuren in
                // EINE Liste mit einem Sortierschluessel aus der
                // Sortierstufe des Shaders, und R_RadixSort sortiert einmal
                // ueber alles (tr_main.cpp). Das hier ist derselbe
                // Schluessel mit zwei Stufen statt acht.
                //
                // Die Figuren laufen nur im ERSTEN Durchgang: der GPU-Weg
                // zeichnet sie durchweg deckend (holeBlend(Blend::Opaque)),
                // es gibt bei ihnen nichts zu teilen.
                std::vector<gpu::KlingenQuad> klingen;
                // Die Schwertlichter dieses Bildes - CG_DoSaberLight je
                // Schwert, gesammelt beim Zeichnen der Klingen.
                std::vector<gpu::WeltLicht> schwertLichter;
                // Die Figuren fuer die Schattenarten 2 und 3 - erst nach
                // allem Deckenden zu zeichnen, deshalb gesammelt.
                std::vector<gpu::FigurSchatten> figurSchatten;
                // Die Fuesse des AT-ST fuer die beiden kleinen Flecken.
                std::vector<std::array<float, 3>> fussSchatten;
                // rc604 surfaceSprites: die Halme (netzId 3, je Bild neu). Die
                // Engine zeichnet sie im Shader ihrer FLAECHE - Gras ist deckend
                // sortiert (erste Stufe deckend), also kommen auch gemischte
                // Halme (SRC_ALPHA mit depthWrite) VOR Wasser, Glas und Effekten:
                // deckende im ersten Durchgang nach den Effekten, gemischte am
                // Anfang des zweiten.
                auto zeichneGras = [&](const gpu::Lage lage, const bool erst, std::string& f) {
                    // Die Halme (netzId 3, je Bild neu).
                    // Deckend mit Alphatest im ersten, gemischt im zweiten
                    // Durchgang - wie jede andere Stufe. Kameravorschau und
                    // Himmelsportal bekommen ihre eigenen (die Engine rechnet
                    // je Sicht); im Spiegel fehlen sie noch.
                    if (nebenbild && erst) {
                        g_app->grasNetzNeben = BspMesh{};
                        if (!g_app->imSpiegelDurchgang) {
                            float r = g_app->grasReichweite;
                            if (!g_app->imPortalDurchgang) {
                                // Kameravorschau: ihr Bild IST das Spielbild -
                                // rangescalefactor aus fov_y gegen cg_fov 80.
                                const float tanJetzt = std::tan(bildCam.fovY * 3.14159265F / 360.0F);
                                const float tanStd = std::tan(spielSicht(80.0F, spiel).fovY * 3.14159265F / 360.0F);
                                r = (tanJetzt > 1.0e-6F) ? tanStd / tanJetzt : 1.0F;
                            }
                            baueGrasNetzNeben(bildCam, r);
                        }
                    }
                    const BspMesh& grasNetz = nebenbild ? g_app->grasNetzNeben : g_app->grasNetz;
                    if (!grasNetz.indexes.empty()) {
                        std::string fg;
                        const int ng = gpu::zeichneKarte(
                            grasNetz, &g_app->textures, &g_app->geo, vp, bildCam.pos, zeit,
                            g_app->mapBrightness, g_app->mapMinLight, 3, gvorn, grechts, ghoch, gfokus, &fg,
                            &g_app->gpuUebersprungen, lage);
                        g_app->gpuAufrufe += ng;
                        g_app->gpuKarte += ng;   // die Halme gehoeren zur Karte
                        if (f.empty()) { f = fg; }
                        // Der Nebel der Halme (SQuickSprite::Flush, zweiter
                        // Aufruf mit tr.fogImage und GLS_DEPTHFUNC_EQUAL).
                        if (!g_app->imSpiegelDurchgang && g_app->showFog && !g_app->geo.nebel.empty()) {
                            static const std::vector<gpu::FigurSchatten> kKeineFiguren;
                            std::string fn;
                            const int nn = gpu::zeichneNebel(grasNetz, &g_app->textures, &g_app->geo, vp, zeit,
                                                             bildCam.pos, nebelListe(), kKeineFiguren, &fn, nullptr,
                                                             -1, erst ? 1 : 2);
                            g_app->gpuAufrufe += nn;
                            if (!nebenbild) { g_app->grasNebelAufrufe += nn; }
                            if (f.empty()) { f = fn; }
                        }
                    }
                };
                for (int durchgang = 0; durchgang < 2; ++durchgang) {
                    const gpu::Lage lage = (durchgang == 0)
                                               ? gpu::Lage::Deckend
                                               : gpu::Lage::Gemischt;
                    const bool erst = (durchgang == 0);

                    if (!erst) { zeichneGras(lage, erst, f); }   // rc604: gemischte Halme vor Wasser und Glas
                    gpu::beginneMessung(erst ? gpu::Abschnitt::KarteDeckend
                                             : gpu::Abschnitt::KarteGemischt);
                    const int nk = gpu::zeichneKarte(
                        g_app->mesh, &g_app->textures, &g_app->geo, vp,
                        bildCam.pos, zeit, g_app->mapBrightness,
                        g_app->mapMinLight, 0, gvorn, grechts, ghoch, gfokus,
                        &f, &g_app->gpuUebersprungen, lage);
                    g_app->gpuAufrufe += nk;
                    g_app->gpuKarte += nk;
                    gpu::beendeMessung(erst ? gpu::Abschnitt::KarteDeckend
                                            : gpu::Abschnitt::KarteGemischt);

                    // Das Effektnetz als ZWEITES Netz - es loescht nicht und
                    // trifft damit auf die Tiefe der Karte.
                    const BspMesh& effektNetz =
                        (nebenbild && effektNetzBild != nullptr) ? *effektNetzBild : g_app->effectMesh;
                    if (nebenbild) { g_app->nebenEffektEcken = static_cast<int>(effektNetz.verts.size()); }
                    if (!effektNetz.indexes.empty()) {
                        gpu::beginneMessung(
                            erst ? gpu::Abschnitt::EffekteDeckend
                                 : gpu::Abschnitt::EffekteGemischt);
                        std::string f2;
                        const int ne = gpu::zeichneKarte(
                            effektNetz, &g_app->textures, &g_app->geo,
                            vp, bildCam.pos, zeit, g_app->mapBrightness,
                            g_app->mapMinLight, 1, gvorn, grechts, ghoch,
                            gfokus, &f2, &g_app->gpuUebersprungen, lage);
                        g_app->gpuAufrufe += ne;
                        g_app->gpuEffekte += ne;
                        gpu::beendeMessung(
                            erst ? gpu::Abschnitt::EffekteDeckend
                                 : gpu::Abschnitt::EffekteGemischt);
                        if (f.empty()) { f = f2; }
                    }
                    // rc604: deckende Halme hier; die gemischten schon am
                    // Anfang des zweiten Durchgangs (siehe zeichneGras).
                    if (erst) { zeichneGras(lage, erst, f); }

                    // Die Mover: Tueren, Plattformen, Schiffe. `movers`
                    // enthaelt AUCH das Effektnetz und die Figuren - die
                    // haben kein eigenes Netz aus brushMeshes und sind hier
                    // schon gezeichnet.
                    gpu::setzeSchritt("Mover");
                    gpu::beginneMessung(erst ? gpu::Abschnitt::MoverDeckend
                                             : gpu::Abschnitt::MoverGemischt);
                    for (const MoverDraw& mv : movers) {
                        if (mv.mesh == nullptr || mv.mesh->indexes.empty() ||
                            mv.mesh == &g_app->effectMesh) {
                            continue;
                        }
                        float mw[16];
                        // Dieselbe Bedingung wie der Rasterer
                        // (mapview.cpp:1254): getaumelt wird nur, wenn Nick
                        // oder Roll ungleich null sind.
                        const bool taumelt =
                            (mv.pitch != 0.0F || mv.roll != 0.0F);
                        gpu::baueMoverWelt(mv.pivot, mv.offset, mv.yaw,
                                           mv.pitch, mv.roll, taumelt, mw, mv.scale);
                        std::string fm;
                        const int nm = gpu::zeichneMover(
                            *mv.mesh, &g_app->textures, &g_app->geo, vp, mw,
                            bildCam.pos, gvorn, grechts, ghoch, gfokus, zeit,
                            g_app->mapBrightness, g_app->mapMinLight, &fm,
                            &g_app->gpuUebersprungen, lage);
                        g_app->gpuAufrufe += nm;
                        g_app->gpuMover += nm;
                        if (f.empty()) { f = fm; }
                    }
                    gpu::beendeMessung(erst ? gpu::Abschnitt::MoverDeckend
                                            : gpu::Abschnitt::MoverGemischt);

                    if (!erst || g_app->gpuAufrufe <= 0) {
                        continue;
                    }
                    // --- Die Figuren, im deckenden Durchgang -----------
                    gpu::setzeSchritt("Figuren vorbereiten");
                    gpu::beginneMessung(gpu::Abschnitt::Figuren);
                    // rc603: CPU-Zeit der Figuren (Selbsttest figurleistung).
                    const auto figurCpuAb = std::chrono::steady_clock::now();
                    // rc602: Leuchtstufen der Figuren gehen mit ins Gluehen -
                    // nur im Hauptbild und nur, wenn es nachher gezeichnet wird
                    // (dieselbe Bedingung wie beim Gluehen unten).
                    gpu::gluehenFigurenBeginnen(!nebenbild && g_app->showGlow && g_app->textures.found > 0 &&
                                                    !g_app->imSpiegelDurchgang && !g_app->imPortalDurchgang,
                                                320, 240);
                    std::vector<BoneMatrix> welt;
                    std::vector<BoneMatrix> hilf;
                    std::vector<BoneMatrix> skin;
                    umrissFiguren.clear();
                    // --- Was liegt auf den Figuren? --------------------
                    //
                    // Einmal je Bild neu, damit der Bericht den Stand von
                    // eben zeigt und nicht den vom Laden.
                    g_app->figurArten.clear();
                    int figurNr = -1;   // Schluessel der Schwertspuren
                    for (const ActorDraw& a : gpuFiguren) {
                        ++figurNr;
                        if (a.model == nullptr) {
                            continue;
                        }
                        skin.clear();
                        // DIESELBE Funktion wie der Rasterer
                        // (mapview.cpp, figurKnochen). Vorher stand hier
                        // ein blosses worldMatrices(a.frame, welt) - ohne
                        // Vorrang von Ober- und Unterkoerper, ohne Mund,
                        // ohne Mischung zwischen zwei Bildern, ohne
                        // Uebergang zwischen zwei Animationen und ohne
                        // Kopfwinkel. Das ergab eine falsche Haltung und
                        // ein Springen, wo der Rasterer weich blendet.
                        if (figurKnochen(a, welt, hilf)) {
                            gpu::setzeSchritt("Knochenmatrizen");
                            gpu::baueKnochenmatrizen(welt, a.anim->bones, skin);
                        }
                        // --- ZUSAMMENFASSEN, nicht aufzaehlen -----------
                        //
                        // Die erste Fassung schrieb eine Zeile je Flaeche und
                        // brach bei 60 ab. Ein einziger Battledroid hat 26
                        // Flaechen - drei davon haben die Liste gefuellt, und
                        // die Hologramme auf dem Tisch, wegen derer sie
                        // gebaut wurde, standen gar nicht drin.
                        //
                        // Interessant ist nicht die einzelne Flaeche, sondern
                        // die KOMBINATION aus Shader, Skin und Mischart. Von
                        // denen gibt es in einer Karte ein Dutzend.
                        if (a.textures != nullptr) {
                            for (std::size_t si = 0;
                                 si < a.model->surfaces.size(); ++si) {
                                const GlmSurface& sf = a.model->surfaces[si];
                                if (!sf.isVisible(false)) { continue; }
                                const char* misch = "deckend";
                                bool ohneShader = true;
                                if (si < a.textures->bySurface.size()) {
                                    const TextureSet::Tex& tx =
                                        a.textures->bySurface[si];
                                    ohneShader = tx.empty();
                                    switch (tx.blend) {
                                        case BlendMode::Add: misch = "additiv"; break;
                                        case BlendMode::Alpha: misch = "alpha"; break;
                                        case BlendMode::AddAlpha: misch = "additiv-alpha"; break;
                                        case BlendMode::Filter: misch = "filter"; break;
                                        default: break;
                                    }
                                }
                                const std::string zeile =
                                    "shader=\"" + sf.shader + "\"  skin=\"" +
                                    sf.texture + "\"  " + misch +
                                    (ohneShader ? "  OHNE TEXTUR" : "");
                                ++g_app->figurArten[zeile];
                            }
                        }
                        float fw[16];
                        // --- Die Groesse aus der .npc ----------------------
                        //
                        // CG_Player (cg_players.cpp der Mod): die Achsen der
                        // Figur mal modelScale, und der Ursprung um
                        // 24 * (modelScale[2] - 1) gesenkt - der Ursprung
                        // liegt 24 ueber den Fuessen, so bleiben sie am Boden.
                        // Wegen des 90-Grad-Zuschlags in baueFigurWelt ist
                        // die Spalte 1 die Vorwaertsachse (modelScale[0]) und
                        // Spalte 0 die seitliche (modelScale[1]).
                        figurWeltMatrix(a, fw);   // rc601 saber: herausgezogen
                        // Das Licht an ihrem Standort (Gitter + Effekt-
                        // lichter), mal Helligkeit und auf 0..1 - wie im
                        // Rasterer (renderActors).
                        gpu::FigurLicht licht;
                        // rc602 kartenpruefung: fuer die Shaderstufen - Auge, Zeit
                        // und ent->e.axis = AnglesToAxis(0, yaw, 0) der Figur
                        // (ohne den Zeichnerausgleich von baueFigurWelt).
                        for (int k = 0; k < 3; ++k) { licht.auge[k] = bildCam.pos[k]; }
                        licht.zeit = zeit;
                        licht.besitzer = a.figur;   // rc603 Durchsicht (Selbsttest)
                        {
                            const float gier = a.yaw * 3.14159265F / 180.0F;
                            const float cg = std::cos(gier);
                            const float sg = std::sin(gier);
                            const float ax[3][3] = {{cg, sg, 0.0F}, {-sg, cg, 0.0F}, {0.0F, 0.0F, 1.0F}};
                            std::memcpy(licht.achsen, ax, sizeof(ax));
                            licht.achsenGesetzt = true;
                        }
                        if (a.light.ok) {
                            licht.gitter = true;
                            for (int k = 0; k < 3; ++k) {
                                licht.umgebung[k] =
                                    a.light.ambient[k] * g_app->mapBrightness / 255.0F;
                                licht.gerichtet[k] =
                                    a.light.directed[k] * g_app->mapBrightness / 255.0F;
                                licht.richtung[k] = a.light.dir[k];
                            }
                        }
                        std::string ff;
                        const int n = gpu::zeichneFigur(
                            *a.model, a.textures, skin, fw, vp, &ff, &licht,
                            g_app->showCaps);
                        if (!nebenbild && auswahl.figur(a.figur)) {
                            UmrissFigur uf;
                            uf.model = a.model;
                            uf.textures = a.textures;
                            uf.knochen = skin;
                            std::copy(fw, fw + 16, uf.welt);
                            umrissFiguren.push_back(std::move(uf));
                        }
                        g_app->gpuAufrufe += n;
                        g_app->gpuFiguren2 += n;
                        if (n == 0 && f.empty()) { f = ff; }

                        // --- Die Fuesse des AT-ST (CG_PlayerShadow) ----------
                        //
                        // sideOrigin = Bolzen "*l_foot"/"*r_foot", 30 nach oben
                        // ("fudge up a bit for coplaner"), Radius 28. Nur die
                        // Flecken - fuer Art 2/3 zaehlt allein die Ebene.
                        if (g_app->schattenArt == 1 && a.schattenFuesse && a.anim != nullptr &&
                            !welt.empty()) {
                            for (const char* bolzen : {"*l_foot", "*r_foot"}) {
                                const int sf = surfaceIndex(*a.model, bolzen);
                                BoneMatrix bm{};
                                if (sf < 0 || !boltMatrix(*a.model, sf, welt, *a.anim, bm)) {
                                    continue;
                                }
                                std::array<float, 3> p{};
                                for (int r = 0; r < 3; ++r) {
                                    p[static_cast<std::size_t>(r)] =
                                        fw[r * 4 + 0] * bm.m[0][3] + fw[r * 4 + 1] * bm.m[1][3] +
                                        fw[r * 4 + 2] * bm.m[2][3] + fw[r * 4 + 3];
                                }
                                p[2] += 30.0F;
                                fussSchatten.push_back(p);
                            }
                        }

                        // --- Schattenart 2 und 3: die Ebene darunter ------
                        //
                        // Dieselbe Spur wie fuer den Fleck (CG_PlayerShadow).
                        // Art 3 braucht einen Treffer, Art 2 zeichnet auch
                        // ohne - dann mit shadowPlane 0, wie die Engine.
                        // Nur vor der Kamera bis r_shadowRange (bInShadowRange,
                        // tr_ghoul2.cpp: Abstand entlang der Blickrichtung).
                        bool mitSchatten = false;
                        float schattenEbene = 0.0F;
                        if ((g_app->schattenArt == 2 || g_app->schattenArt == 3) &&
                            a.schattenRadius > 0.0F) {
                            const float vorAus = (a.pos[0] - bildCam.pos[0]) * gvorn[0] +
                                                 (a.pos[1] - bildCam.pos[1]) * gvorn[1] +
                                                 (a.pos[2] - bildCam.pos[2]) * gvorn[2];
                            if (vorAus < 1000.0F) {
                                gpu::BlobSchatten spur;
                                const bool getroffen =
                                    blobSchattenFuer(a.pos, a.schattenRadius, spur);
                                if (getroffen) {
                                    schattenEbene = spur.ort[2] + 1.0F;
                                }
                                mitSchatten = getroffen || g_app->schattenArt == 2;
                            }
                        }
                        float schattenLicht[3] = {0.0F, 0.0F, 1.0F};
                        if (licht.gitter) {
                            for (int k = 0; k < 3; ++k) { schattenLicht[k] = licht.richtung[k]; }
                        }
                        // Steht die Figur im Nebel, bekommt sie ihn auch.
                        const int figurNebel =
                            g_app->showFog ? nebelFuer(a.pos) : -1;
                        if (mitSchatten || figurNebel >= 0) {
                            gpu::FigurSchatten fs;
                            fs.model = a.model;
                            fs.textures = a.textures;
                            fs.knochen = skin;
                            std::memcpy(fs.welt, fw, sizeof(fs.welt));
                            for (int k = 0; k < 3; ++k) { fs.richtung[k] = schattenLicht[k]; }
                            fs.ebene = schattenEbene;
                            fs.kappen = g_app->showCaps;
                            fs.schatten = mitSchatten;
                            fs.nebel = figurNebel;
                            figurSchatten.push_back(std::move(fs));
                        }

                        // --- Was die Figur in den Haenden haelt ----------
                        //
                        // Waffe, Schwertgriff(e) und Kinomodelle haengen
                        // starr an einem Bolzen der Figur - "*r_hand" oder
                        // "*l_hand" (g_client.cpp:1628 f.). Klinge N beginnt
                        // am Bolzen "*bladeN" des Griffs und laeuft in dessen
                        // NEGATIVE x-Richtung (CG_AddSaberBladeGo,
                        // cg_players.cpp:13767 ff.).
                        if (a.anim != nullptr && !welt.empty()) {
                            for (int ai2 = 0; ai2 < a.anbauten; ++ai2) {
                                const AnbauDraw& an =
                                    a.anbau[static_cast<std::size_t>(ai2)];
                                if (an.model == nullptr) {
                                    continue;
                                }
                                BoneMatrix hand{};
                                // Fehlt der Bolzen, haengt die Engine NICHTS an
                                // ("if (bolt_num == -1) return",
                                // wp_saber.cpp:748) - also auch hier nicht.
                                const int handSurf = surfaceIndex(*a.model, an.bolzen);
                                if (!boltMatrix(*a.model, handSurf, welt, *a.anim, hand)) {
                                    continue;
                                }
                                float gw2[16];
                                gpu::baueGriffWelt(fw, hand, gw2);
                                static const std::vector<BoneMatrix> kStarr;
                                std::string fh;
                                // rc603 Durchsicht: die Achsen der FIGUR, nicht die
                                // des Griffs. Waffe, Griff und Kinomodell stehen in
                                // der Ghoul2-Liste der Figur (G2API_InitGhoul2Model
                                // auf ent->ghoul2 + G2API_AttachG2Model,
                                // wp_saber.cpp:812-816, Q3_Interface.cpp:6345-6351
                                // MD; wp_saber.cpp:407, Q3_Interface.cpp:5897 OJ)
                                // und werden mit ihrem refEntity gezeichnet - die
                                // Bolzenmatrix ist nur die Wurzel der Knochen
                                // (R_AddGhoulSurfaces, tr_ghoul2.cpp:2860-2866 MD,
                                // 2656 OJ). backEnd.ori ist damit die Stellung der
                                // Figur, und tcGen environment
                                // (RB_CalcEnvironmentTexCoords, tr_shade_calc.cpp:868
                                // MD / 902 OJ) rechnet Normale und Blick in IHREN
                                // Achsen. rc602 nahm die Spalten von gw2 - der Chrom
                                // jedes Schwertgriffs drehte mit der Hand.
                                const int nh = gpu::zeichneFigur(
                                    *an.model, an.textures, kStarr, gw2, vp, &fh,
                                    &licht, g_app->showCaps);
                                g_app->gpuAufrufe += nh;
                                g_app->gpuFiguren2 += nh;
                                // Waffe und Griff gehoeren zur selben Entity und
                                // werfen denselben Schatten mit.
                                if (mitSchatten || figurNebel >= 0) {
                                    gpu::FigurSchatten fs;
                                    fs.model = an.model;
                                    fs.textures = an.textures;
                                    std::memcpy(fs.welt, gw2, sizeof(fs.welt));
                                    for (int k = 0; k < 3; ++k) { fs.richtung[k] = schattenLicht[k]; }
                                    fs.ebene = schattenEbene;
                                    fs.kappen = g_app->showCaps;
                                    fs.schatten = mitSchatten;
                                    fs.nebel = figurNebel;
                                    figurSchatten.push_back(std::move(fs));
                                }
                                // In die Welt: dieselbe 4x4 wie der Griff.
                                const auto nachWelt = [&gw2](const float in[3], float o[3]) {
                                    for (int r = 0; r < 3; ++r) {
                                        o[r] = gw2[r * 4 + 0] * in[0] + gw2[r * 4 + 1] * in[1] +
                                               gw2[r * 4 + 2] * in[2] + gw2[r * 4 + 3];
                                    }
                                };
                                // CG_DoSaberLight: je Schwert EIN Licht.
                                // Bei einer Klinge in ihrer Mitte, Radius
                                // doppelte Laenge; bei mehreren im Mittel
                                // der Spitzen, nach Laenge gewichtete Farbe,
                                // Radius der groesste Spitzenabstand.
                                float lMitte[3] = {0.0F, 0.0F, 0.0F};
                                float lFarbe[3] = {0.0F, 0.0F, 0.0F};
                                float lErsteFarbe[3] = {0.0F, 0.0F, 0.0F};
                                float lSpitzenSumme[3] = {0.0F, 0.0F, 0.0F};
                                std::vector<std::array<float, 3>> lSpitzen;
                                float lGesamt = 0.0F;
                                float lDurchmesser = 0.0F;
                                for (int kn = 0; kn < an.klingen; ++kn) {
                                    const KlingeDraw& kd =
                                        an.klinge[static_cast<std::size_t>(kn)];
                                    float wl[3];
                                    float tl[3];
                                    if (!klingenStrecke(*an.model, an.lage, kd.nummer,
                                                        kd.laenge, wl, tl)) {
                                        continue;
                                    }
                                    float wA[3];
                                    float wB[3];
                                    nachWelt(wl, wA);
                                    nachWelt(tl, wB);
                                    // rc601 saber: die Laenge dieses Clientbildes
                                    // (an Waenden gekuerzt, CG_AddSaberBladeGo);
                                    // eine Klinge mit Laenge 0 steht nicht.
                                    if (!saberKlingeZeichnen(a.figur, an.saberFach, kd.nummer, wA, wB, kd.laenge)) {
                                        continue;
                                    }
                                    if (lSpitzen.empty()) {
                                        for (int k = 0; k < 3; ++k) {
                                            lMitte[k] = 0.5F * (wA[k] + wB[k]);
                                            lErsteFarbe[k] = kd.licht[k];
                                        }
                                    }
                                    lSpitzen.push_back({wB[0], wB[1], wB[2]});
                                    for (int k = 0; k < 3; ++k) {
                                        lFarbe[k] += kd.laenge * kd.licht[k];
                                        lSpitzenSumme[k] += wB[k];
                                    }
                                    lGesamt += kd.laenge;
                                    lDurchmesser = std::max(lDurchmesser, kd.laenge * 2.0F);
                                    // Erst der breite Glanz in der Klingen-
                                    // farbe, dann der schmale, fast weisse
                                    // Kern (Radius / 3) - CG_DoSaber.
                                    // Die Spur zuerst (sie liegt hinter der Klinge).
                                    // Schluessel nach der Figur in der SZENE, nicht nach
                                    // der Stelle unter den sichtbaren - die verschob sich,
                                    // sobald eine Figur davor erschien oder verschwand.
                                    schwertSpur((static_cast<std::uint64_t>(a.figur >= 0 ? a.figur : figurNr) << 16) ^
                                                    (static_cast<std::uint64_t>(ai2) << 8) ^
                                                    static_cast<std::uint64_t>(kn),
                                                wA, wB, kd, klingen);
                                    float radien[2] = {kd.radius, kd.radius / 3.0F};
                                    if (regelwerkJediOutcast()) {
                                        // rc604 fix604 (JK2 Stufe 3): CG_DoSaber von Jedi Outcast
                                        // (codeJK2/cgame/cg_players.cpp:4250-4278): radiusmult =
                                        // 1 + 2/length, solange die Klinge kuerzer als lengthMax
                                        // ist; Glanz (2,8 + flrand(-1,1)*0,2), Kern (1,0 +
                                        // flrand(-1,1)*0,2). Gewuerfelt aus Takt, Figur und
                                        // Klinge, damit ein angehaltenes Bild stillsteht.
                                        const float mult = kd.laenge < kd.laengeMax ? 1.0F + (2.0F / kd.laenge) : 1.0F;
                                        std::uint32_t w2 = static_cast<std::uint32_t>(static_cast<std::int64_t>(g_app->playMs / 50.0));
                                        w2 = w2 * 2654435761U ^ static_cast<std::uint32_t>((a.figur + 7) * 7919 + ai2 * 131 + kn * 17);
                                        w2 ^= w2 >> 15;
                                        w2 *= 2246822519U;
                                        w2 ^= w2 >> 13;
                                        const float z1 = static_cast<float>(w2 & 0xFFFFU) / 32768.0F - 1.0F;
                                        const float z2 = static_cast<float>((w2 >> 16) & 0xFFFFU) / 32768.0F - 1.0F;
                                        radien[0] = (2.8F + z1 * 0.2F) * mult;
                                        radien[1] = (1.0F + z2 * 0.2F) * mult;
                                    }
                                    for (int lage2 = 0; lage2 < 2; ++lage2) {
                                        gpu::KlingenQuad q;
                                        gpu::baueKlingenband(wA, wB, bildCam.pos, grechts,
                                                             radien[lage2], q.ecke);
                                        for (int k = 0; k < 3; ++k) {
                                            q.farbe[k] = (lage2 == 0)
                                                             ? static_cast<float>(kd.farbe[k])
                                                             : static_cast<float>(std::min(
                                                                   255, 190 + kd.farbe[k] / 4));
                                        }
                                        q.tex = (lage2 == 0) ? kd.glowTex : kd.coreTex;
                                        // rc602: die Leuchtklinge (*_glow, Stufe mit glow) geht ins Gluehen.
                                        q.gluehen = lage2 == 0;
                                        klingen.push_back(q);
                                    }
                                }
                                if (lGesamt > 0.0F && !lSpitzen.empty()) {
                                    gpu::WeltLicht wl;
                                    if (lSpitzen.size() == 1U) {
                                        for (int k = 0; k < 3; ++k) {
                                            wl.ort[k] = lMitte[k];
                                            wl.farbe[k] = lErsteFarbe[k];
                                        }
                                    } else {
                                        const float anzahl = static_cast<float>(lSpitzen.size());
                                        for (int k = 0; k < 3; ++k) {
                                            wl.ort[k] = lSpitzenSumme[k] / anzahl;
                                            wl.farbe[k] = lFarbe[k] / lGesamt;
                                        }
                                        for (const auto& p1 : lSpitzen) {
                                            for (const auto& p2 : lSpitzen) {
                                                const float dx = p1[0] - p2[0];
                                                const float dy = p1[1] - p2[1];
                                                const float dz = p1[2] - p2[2];
                                                lDurchmesser = std::max(
                                                    lDurchmesser,
                                                    std::sqrt(dx * dx + dy * dy + dz * dz));
                                            }
                                        }
                                    }
                                    // + Q_flrand(0,1) * 8: das Flackern. Aus
                                    // Takt, Figur und Griff gewuerfelt, damit
                                    // ein angehaltenes Bild stillsteht.
                                    std::uint32_t wurf = static_cast<std::uint32_t>(
                                        static_cast<std::int64_t>(g_app->playMs / 50.0));
                                    wurf = wurf * 2654435761U ^
                                           static_cast<std::uint32_t>(schwertLichter.size() * 40503U + 17U);
                                    wurf ^= wurf >> 15;
                                    wurf *= 2246822519U;
                                    wurf ^= wurf >> 13;
                                    const float zufall = static_cast<float>(wurf & 0xFFFFU) / 65536.0F;
                                    wl.radius = lDurchmesser + zufall * 8.0F;
                                    schwertLichter.push_back(wl);
                                }
                            }
                        }
                    }
                    // rc602 kartenpruefung: die Ghoul2-Gegenstaende (Waffen am
                    // Boden), starr wie ein Griff, mit dem Licht des Gitters an
                    // ihrem Ort (R_SetupEntityLighting wie bei den Figuren).
                    if (g_app->showActors) {
                        static const std::vector<BoneMatrix> kStarrGegenstand;
                        for (const App::GegenstandGlm& gg : g_app->gegenstandGlm) {
                            if (gg.versteckt) { continue; }
                            // rc604: misc_trip_mine - EF_NODRAW bis zum use, nach
                            // WP_Explode + 50 ms frei (laserfalleLauf).
                            if (const auto fs = g_app->fallenSicht.find(gg.entity); fs != g_app->fallenSicht.end() &&
                                (g_app->playMs < fs->second.first ||
                                 (fs->second.second >= 0.0 && g_app->playMs >= fs->second.second))) {
                                continue;
                            }
                            const App::ActorAssets* as = gegenstandGlmFuer(gg.pfad, gg.bildIndex);   // rc604 fix604b (B3)
                            if (as == nullptr) { continue; }
                            gpu::FigurLicht gl;
                            for (int k = 0; k < 3; ++k) { gl.auge[k] = bildCam.pos[k]; }
                            gl.zeit = zeit;
                            const auto lg = sampleLightGrid(g_app->geo, gg.ort);
                            if (lg.ok) {
                                gl.gitter = true;
                                for (int k = 0; k < 3; ++k) {
                                    gl.umgebung[k] = lg.ambient[k] * g_app->mapBrightness / 255.0F;
                                    gl.gerichtet[k] = lg.directed[k] * g_app->mapBrightness / 255.0F;
                                    gl.richtung[k] = lg.dir[k];
                                }
                            }
                            std::string fg;
                            // rc604: Wachgeschuetz/Ionenkanone mit ihrer Knochenanimation.
                            const std::vector<BoneMatrix>* knochen = &kStarrGegenstand;
                            std::vector<BoneMatrix> animWelt;
                            std::vector<BoneMatrix> animSkin;
                            const auto ps = g_app->pasSchwenks.find(gg.entity);   // rc605 FU4
                            if (const auto ga = g_app->geschuetzAnims.find(gg.entity);
                                ga != g_app->geschuetzAnims.end() &&
                                geschuetzKnochen(*as, ga->second, g_app->playMs, animWelt, animSkin,
                                                 ps != g_app->pasSchwenks.end() ? &ps->second : nullptr)) {
                                knochen = &animSkin;
                            }
                            const int ng = gpu::zeichneFigur(as->model, &as->textures, *knochen, gg.welt, vp,
                                                             &fg, &gl, g_app->showCaps);
                            g_app->gpuAufrufe += ng;
                            g_app->gpuFiguren2 += ng;
                        }
                    }
                    gpu::gluehenFigurenEnde();
                    gpu::beendeMessung(gpu::Abschnitt::Figuren);
                    g_app->figurCpuMsSumme +=
                        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - figurCpuAb).count();

                    // --- Dynamisches Licht und Blobschatten ------------
                    //
                    // Nach allem Deckenden und den Figuren, vor allem
                    // Durchscheinenden - siehe gpu::zeichneLichtUndSchatten.
                    std::vector<gpu::WeltLicht> weltLichter;
                    if (g_app->showDynLights) {
                        for (const EffectLight& el : fxLichter) {
                            gpu::WeltLicht wl;
                            for (int k = 0; k < 3; ++k) {
                                wl.ort[k] = el.origin[k];
                                wl.farbe[k] = el.color[k];
                            }
                            wl.radius = el.radius;
                            weltLichter.push_back(wl);
                        }
                        weltLichter.insert(weltLichter.end(), schwertLichter.begin(),
                                           schwertLichter.end());
                    }
                    std::vector<gpu::BlobSchatten> blobs;
                    if (g_app->schattenArt == 1) {
                        for (const ActorDraw& a : gpuFiguren) {
                            if (a.model == nullptr || a.schattenRadius <= 0.0F) {
                                continue;
                            }
                            // cg_shadowCullDistance = r_shadowRange, 1000.
                            const float dx = a.pos[0] - bildCam.pos[0];
                            const float dy = a.pos[1] - bildCam.pos[1];
                            const float dz = a.pos[2] - bildCam.pos[2];
                            if (dx * dx + dy * dy + dz * dz > 1000.0F * 1000.0F) {
                                continue;
                            }
                            gpu::BlobSchatten b;
                            if (blobSchattenFuer(a.pos, a.schattenRadius, b)) {
                                blobs.push_back(b);
                            }
                        }
                        for (const std::array<float, 3>& p : fussSchatten) {
                            gpu::BlobSchatten b;
                            if (blobSchattenFuer(p.data(), 28.0F, b)) {
                                blobs.push_back(b);
                            }
                        }
                    }
                    if (!g_app->imSpiegelDurchgang && (!weltLichter.empty() || !blobs.empty())) {
                        gpu::setzeSchritt("Licht und Schatten");
                        std::string fl;
                        const int nl = gpu::zeichneLichtUndSchatten(
                            g_app->mesh, &g_app->textures, &g_app->geo, vp, zeit,
                            weltLichter, blobs,
                            gfxBild("gfx/2d/dlight"), gfxBild("gfx/damage/shadow"), &fl);
                        g_app->gpuAufrufe += nl;
                        g_app->gpuKarte += nl;
                        if (f.empty()) { f = fl; }
                    }
                    // Tueren, Plattformen, Schiffe: das Licht wie auf der
                    // Welt, im Raum des Movers (R_TransformDlights).
                    // Abziehbilder legt die Engine nur auf die Welt.
                    const auto moverWelt = [](const MoverDraw& mv, float* mw) {
                        const bool taumelt = (mv.pitch != 0.0F || mv.roll != 0.0F);
                        gpu::baueMoverWelt(mv.pivot, mv.offset, mv.yaw, mv.pitch,
                                           mv.roll, taumelt, mw, mv.scale);
                    };
                    if (!weltLichter.empty()) {
                        static const std::vector<gpu::BlobSchatten> kKeine;
                        for (const MoverDraw& mv : movers) {
                            if (mv.mesh == nullptr || mv.mesh->indexes.empty() ||
                                mv.mesh == &g_app->effectMesh) {
                                continue;
                            }
                            float mw[16];
                            moverWelt(mv, mw);
                            std::string fl;
                            const int nl = gpu::zeichneLichtUndSchatten(
                                *mv.mesh, &g_app->textures, &g_app->geo, vp, zeit,
                                weltLichter, kKeine, gfxBild("gfx/2d/dlight"), nullptr,
                                &fl, mw);
                            g_app->gpuAufrufe += nl;
                            g_app->gpuMover += nl;
                        }
                    }
                    // Der Nebel gehoert zu jeder Flaeche (RB_FogPass nach
                    // ihren Stufen und dem dynamischen Licht); die
                    // Figurenschatten kommen erst danach (SS_STENCIL_SHADOW,
                    // RB_ShadowFinish).
                    g_app->nebelAufrufe = 0;
                    if (!g_app->imSpiegelDurchgang && g_app->showFog && !g_app->geo.nebel.empty()) {
                        gpu::setzeSchritt("Nebel");
                        std::string fn;
                        const int nn = gpu::zeichneNebel(
                            g_app->mesh, &g_app->textures, &g_app->geo, vp, zeit,
                            bildCam.pos, nebelListe(), figurSchatten, &fn);
                        g_app->nebelAufrufe = nn;
                        g_app->gpuAufrufe += nn;
                        g_app->gpuKarte += nn;
                        if (f.empty()) { f = fn; }
                        // Die Mover: Brush-Modelle mit dem Nebel ihrer
                        // Flaechen, Kartenmodelle mit dem ihres Ursprungs.
                        const std::vector<gpu::NebelGpu> nebel = nebelListe();
                        static const std::vector<gpu::FigurSchatten> kKeineFiguren;
                        for (const MoverDraw& mv : movers) {
                            if (mv.mesh == nullptr || mv.mesh->indexes.empty() ||
                                mv.mesh == &g_app->effectMesh) {
                                continue;
                            }
                            float mw[16];
                            moverWelt(mv, mw);
                            std::string fm;
                            const int nm = gpu::zeichneNebel(
                                *mv.mesh, &g_app->textures, &g_app->geo, vp, zeit,
                                bildCam.pos, nebel, kKeineFiguren, &fm, mw,
                                nebelFuer(mv.offset));
                            g_app->nebelAufrufe += nm;
                            g_app->gpuAufrufe += nm;
                            g_app->gpuMover += nm;
                        }
                    }
                    if (!g_app->imSpiegelDurchgang && !figurSchatten.empty()) {
                        gpu::setzeSchritt("Figurenschatten");
                        std::string fs2;
                        const int nsFigur = gpu::zeichneFigurSchatten(figurSchatten,
                                                                 g_app->schattenArt, vp, &fs2);
                        g_app->gpuAufrufe += nsFigur;
                        g_app->gpuFiguren2 += nsFigur;
                        if (f.empty()) { f = fs2; }
                    }
                    g_app->lichtAufrufe = static_cast<int>(weltLichter.size());
                    g_app->schattenAnzahl = static_cast<int>(
                        g_app->schattenArt == 1 ? blobs.size() : figurSchatten.size());
                    // Fuer die Figuren des naechsten Bildes.
                    g_app->schwertLichter.clear();
                    for (const gpu::WeltLicht& wl : schwertLichter) {
                        EffectLight el;
                        for (int k = 0; k < 3; ++k) {
                            el.origin[k] = wl.ort[k];
                            el.color[k] = wl.farbe[k];
                        }
                        el.radius = wl.radius;
                        g_app->schwertLichter.push_back(el);
                    }
                }
                // --- Die Klingen nach allem anderen --------------------
                //
                // Durchscheinend und additiv: sie kommen nach der Karte,
                // den Movern und den Figuren, damit sie ueber allem liegen,
                // das sie nicht verdeckt - wie im Rasterer.
                // Das Wetter im selben Durchgang: additiv, tiefengetestet.
                if (!g_app->imPortalDurchgang && !g_app->imSpiegelDurchgang) { wetterTeilchen(klingen, bildCam); }
                if (!g_app->imSpiegelDurchgang) flareSchilde(klingen, bildCam, static_cast<float>(bildW) / static_cast<float>(std::max(bildH, 1)));
                if (!klingen.empty()) {
                    std::string fk;
                    const int nk2 = gpu::zeichneKlingen(klingen, vp, &fk);
                    g_app->gpuAufrufe += nk2;
                    g_app->gpuFiguren2 += nk2;
                    if (nk2 == 0 && f.empty()) { f = fk; }
                }
                // --- Das Gluehen, ganz zuletzt -------------------------
                if (!nebenbild && g_app->gpuAufrufe > 0 && g_app->showGlow &&
                    g_app->textures.found > 0) {
                    gpu::beginneMessung(gpu::Abschnitt::Gluehen);
                    std::string fg;
                    // rc602: auch die Brush-Entities und Kartenmodelle - in
                    // derselben Stellung wie oben bei zeichneMover. Im Spiel
                    // gehen alle Flaechen mit glow-Stufe in den Gluehpuffer
                    // (RB_DrawSurfs, g_bRenderGlowingObjects), nicht nur die
                    // Welt: in md_am_sith das Palpatine-Hologramm, der
                    // Holotisch, die Kraftfelder und Lichtleisten.
                    std::vector<gpu::GluehQuelle> gluehMover;
                    for (const MoverDraw& mv : movers) {
                        if (mv.mesh == nullptr || mv.mesh->indexes.empty() || mv.mesh == &g_app->effectMesh) {
                            continue;
                        }
                        gpu::GluehQuelle q;
                        q.mesh = mv.mesh;
                        gpu::baueMoverWelt(mv.pivot, mv.offset, mv.yaw, mv.pitch, mv.roll,
                                           mv.pitch != 0.0F || mv.roll != 0.0F, q.welt, mv.scale);
                        gluehMover.push_back(q);
                    }
                    // rc602 kartenpruefung: die Effektteilchen mit glow-Stufe dazu.
                    gpu::gluehenEffekte(g_app->showEffects ? &g_app->effectMesh : nullptr);
                    gpu::gluehenKlingen(&klingen);   // rc602: die Leuchtklingen dazu
                    g_app->gpuGluehen = gpu::zeichneGluehen(
                                        g_app->mesh, &g_app->textures,
                                        &g_app->geo, vp, zeit,
                                        320, 240,
                                        true, &fg,
                                        (g_app->showEffects && g_app->rauchImGluehen) ? &g_app->effectMesh : nullptr,
                                        gluehMover);
                    gpu::beendeMessung(gpu::Abschnitt::Gluehen);
                    if (f.empty()) {
                        f = fg;
                    }
                }
                // --- Umriss der Auswahl (rc597) ---------------------------
                //
                // Wie in Unreal: die gewaehlten Schiffe, Tueren, Modelle und
                // Figuren bekommen eine Linie um ihren sichtbaren Rand. Nur im
                // Hauptbild, nicht beim Blick durch die Kamera.
                if (!nebenbild && !g_app->camViewActive && !auswahl.leer()) {
                    bool etwas = !umrissFiguren.empty();
                    for (const MoverDraw& mv : movers) { etwas = etwas || auswahl.mover(mv); }
                    if (etwas && gpu::umrissBeginnen()) {
                        for (const MoverDraw& mv : movers) {
                            if (!auswahl.mover(mv) || mv.mesh == nullptr) { continue; }
                            float mw[16];
                            gpu::baueMoverWelt(mv.pivot, mv.offset, mv.yaw, mv.pitch, mv.roll,
                                               mv.pitch != 0.0F || mv.roll != 0.0F, mw, mv.scale);
                            std::string fu;
                            int ueb = 0;
                            (void)gpu::zeichneMover(*mv.mesh, &g_app->textures, &g_app->geo, vp, mw, bildCam.pos, gvorn,
                                                    grechts, ghoch, gfokus, zeit, g_app->mapBrightness,
                                                    g_app->mapMinLight, &fu, &ueb, gpu::Lage::Alles);
                        }
                        for (const UmrissFigur& uf : umrissFiguren) {
                            std::string fu;
                            (void)gpu::zeichneFigur(*uf.model, uf.textures, uf.knochen, uf.welt, vp, &fu, nullptr,
                                                    g_app->showCaps);
                        }
                        const float orange[4] = {1.0F, 0.66F, 0.12F, 1.0F};
                        gpu::umrissBeenden(orange, std::max(1.0F, std::round(1.6F * ImGui::GetFontSize() / 13.0F)));
                        ++g_app->umrissBilder;
                    }
                }
                // Schliesst das Bild. Danach darf keine Marke mehr kommen.
                if (!nebenbild) {
                    gpu::bildFertig();
                    gezeichnet = true;
                }
                // --- Die Debugschicht abfragen -------------------------
                //
                // Sie laeuft seit rc425 und schreibt an den
                // Windows-Debugger - gelesen hat ihre Warteschlange bis
                // rc447 niemand. Jede Beanstandung von Direct3D war damit
                // unsichtbar, obwohl sie erzeugt wurde.
                //
                // Gleiche Meldungen kommen nur einmal durch (siehe
                // gpu::debugMeldungen), also kostet das Sammeln nichts.
                for (const std::string& m : gpu::debugMeldungen()) {
                    diag::detail("Direct3D: " + m);
                    if (g_app->d3dMeldungen.size() < 200U) {
                        g_app->d3dMeldungen.push_back(m);
                    }
                }
                if (spiegeln) { gpu::setzeSpiegelung(false, nullptr, false); }
                g_app->gpuFehler = f;
                if (g_app->gpuAufrufe <= 0 && g_app->gpuFehler.empty()) {
                    g_app->gpuFehler = "kein Zeichenaufruf abgesetzt";
                }
            };
            const int gw = g_app->mapImage.width;
            const int gh = g_app->mapImage.height;
            gpu::setzeSchritt("Ziel anlegen");
            // --- Ein Nebenbild in einen anderen Zielsatz (rc592/rc593) ---------
            //
            // Kameravorschau und Himmelsportal zeichnen mit derselben Funktion
            // in ihren eigenen Satz. Was ein Nebenbild an Zaehlern veraendert,
            // wird zurueckgesetzt - Statuszeile und Selbsttests sehen nur die
            // Ansicht.
            auto nebenBild = [&](const Camera& kam, int nw, int nh, int satz, const char* schritt) {
                const int merkKarte = g_app->gpuKarte;
                const int merkMover = g_app->gpuMover;
                const int merkFiguren = g_app->gpuFiguren2;
                const int merkGluehen = g_app->gpuGluehen;
                const int merkEffekte = g_app->gpuEffekte;
                const int merkAufrufe = g_app->gpuAufrufe;
                const int merkUeber = g_app->gpuUebersprungen;
                const std::string merkFehler = g_app->gpuFehler;
                const int merkLicht = g_app->lichtAufrufe;
                const int merkNebel = g_app->nebelAufrufe;
                const int merkSchatten = g_app->schattenAnzahl;
                std::map<std::string, int> merkArten;
                std::swap(merkArten, g_app->figurArten);
                const int merkFlares = g_app->flareZahl;
                const int merkWetter = g_app->wetterTeilchenZahl;
                const int merkSpur = g_app->spurBaender;
                bool gezeichnetNeben = false;
                gpu::waehleZiel(satz);
                if (gpu::bereiteZiel(nw, nh)) {
                    gpu::setzeSchritt(schritt);
                    gpuBild(kam, nw, nh, true);
                    gezeichnetNeben = g_app->gpuAufrufe > 0;
                }
                gpu::waehleZiel(0);
                g_app->gpuKarte = merkKarte;
                g_app->gpuMover = merkMover;
                g_app->gpuFiguren2 = merkFiguren;
                g_app->gpuGluehen = merkGluehen;
                g_app->gpuEffekte = merkEffekte;
                g_app->gpuAufrufe = merkAufrufe;
                g_app->gpuUebersprungen = merkUeber;
                g_app->gpuFehler = merkFehler;
                g_app->lichtAufrufe = merkLicht;
                g_app->nebelAufrufe = merkNebel;
                g_app->schattenAnzahl = merkSchatten;
                std::swap(merkArten, g_app->figurArten);
                g_app->flareZahl = merkFlares;
                g_app->wetterTeilchenZahl = merkWetter;
                g_app->spurBaender = merkSpur;
                return gezeichnetNeben;
            };
            // --- Himmelsportal (rc593, misc_skyportal) -----------------------
            //
            // CG_DrawSkyBoxPortal: erst die Szene vom Ort des Portals aus, mit
            // demselben Blick und Bildwinkel, samt Himmelsbox; die Hauptszene
            // zeichnet dann keinen Himmel (RDF_SKYBOXPORTAL) - dort sieht man
            // das Portalbild. Ohne Himmel (Schalter) oder in der Draufsicht
            // nicht.
            float portalOrt[3];
            const bool mitPortal = g_app->showSky && !g_app->draufsicht && himmelPortalOrt(portalOrt);
            auto portalFuer = [&](const Camera& kam, int nw, int nh) {
                if (!mitPortal) { return false; }
                Camera pk = kam;
                for (int k = 0; k < 3; ++k) { pk.pos[k] = portalOrt[k]; }
                g_app->imPortalDurchgang = true;
                const bool ok = nebenBild(pk, nw, nh, 2, "Himmelsportal");
                g_app->imPortalDurchgang = false;
                return ok;
            };
            g_app->himmelPortalGezeichnet = false;
            g_app->spiegelGezeichnet = false;
            if (gpu::bereiteZiel(gw, gh)) {
                // --- Spiegel (rc593): das Spiegelbild zuerst, in Satz 3 ------
                if (!g_app->draufsicht && spiegelFuer(useCam, g_app->spiegelEbene)) {
                    g_app->imSpiegelDurchgang = true;
                    const bool ok = nebenBild(useCam, gw, gh, 3, "Spiegel");
                    g_app->imSpiegelDurchgang = false;
                    if (ok) {
                        g_app->spiegelGezeichnet = true;
                        gpu::setzeSpiegelBild(3);
                    }
                }
                if (portalFuer(useCam, gw, gh)) {
                    g_app->himmelPortalGezeichnet = true;
                    gpu::setzeHimmelPortal(2);
                }
                gpuBild(useCam, gw, gh, false);
                gpu::setzeHimmelPortal(-1);
                gpu::setzeSpiegelBild(-1);
                // --- Bild-im-Bild der gewaehlten Kamera (rc592) --------------
                //
                // Ein Drittel der Breite, in Zielsatz 1 - mit eigenem
                // Himmelsportal aus ihrer Sicht. Paket kamera, rc601: im
                // Seitenverhaeltnis des SPIELFENSTERS, nicht der Ansicht - es
                // zeigt genau das Spielbild.
                g_app->vorschauGezeigt = false;
                saberBlitzAnsicht(1, nullptr, 0, 0, 0.0);   // rc601 saber review 2c
                Camera vorschau;
                const float aspekt = spiel.seiten;
                if (vorschauKamera(vorschau, aspekt)) {
                    g_app->vorschauFovY = vorschau.fovY;   // Selbsttest (Paket kamera, rc601)
                    int pw = std::clamp(gw / 3, 160, 640);
                    int ph = static_cast<int>(static_cast<float>(pw) / aspekt);
                    if (ph < 90) {
                        // Mindesthoehe - dann breiter, damit das Bild das
                        // Seitenverhaeltnis des Spiels behaelt (rc601).
                        ph = 90;
                        pw = static_cast<int>(std::lround(90.0F * aspekt));
                    }
                    if (portalFuer(vorschau, pw, ph)) { gpu::setzeHimmelPortal(2); }
                    saberBlitzAnsicht(1, &vorschau, pw, ph, g_app->playMs);   // rc601 saber review 2c
                    if (g_app->showEffects && !alleEffekte.empty()) {
                        effektNetzVorschau = baueEffektNetz(alleEffekte, vorschau, pw, ph);
                        effektNetzBild = &effektNetzVorschau;
                        ++g_app->vorschauEffektNetze;
                        g_app->vorschauEffektEcken = static_cast<int>(effektNetzVorschau.verts.size());
                    }
                    const bool vorschauDa = nebenBild(vorschau, pw, ph, 1, "Kameravorschau");
                    effektNetzBild = nullptr;
                    if (vorschauDa) {
                        g_app->vorschauGezeigt = true;
                        for (int k = 0; k < 3; ++k) {
                            g_app->vorschauPos[k] = vorschau.pos[k];
                            g_app->vorschauWinkel[k] = vorschau.angles[k];
                        }
                        g_app->vorschauW = pw;
                        g_app->vorschauH = ph;
                        ++g_app->vorschauBilder;
                    }
                    gpu::setzeHimmelPortal(-1);
                }
            } else {
                g_app->gpuFehler = "Renderziel nicht anlegbar";
            }
            // NICHT leeren: dann schweigt die Marke genau dann, wenn der
            // Absturz nach einem gelungenen Bild kommt - und das war beim
            // zweiten Versuch der Fall. Ein Wort, das "fertig" sagt, ist
            // eine Auskunft; ein leeres ist keine.
            gpu::setzeSchritt("Bild fertig, ausserhalb des GPU-Wegs");
            const float msStufe = static_cast<float>(
                std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - tGpu0).count());
            // Die Anzeige "N fps" kommt aus mapFrameMs, und das setzt sonst
            // nur der Rasterer - beim GPU-Weg staende dort die Zahl des
            // letzten Rasterbildes.
            g_app->mapFrameMs = (g_app->mapFrameMs <= 0.0F)
                                    ? msStufe
                                    : g_app->mapFrameMs * 0.9F + msStufe * 0.1F;
            g_app->frameCost.rawMap = msStufe;
        }
        // rc603 fix603: die Pruefsumme dieses Bildes (App::bildSumme), nach dem
        // Zeichnen (die Klingen meldet erst der Zeichenweg). Nicht
        // darin: die Schwertspuren (g_spuren, vom vorigen Bild abhaengig wie
        // im Spiel, "lastTime > cg.time") und das Licht der Figuren (die
        // Schwertlichter des vorigen Bildes).
        if (g_app->bildSummeAn) {
            std::uint64_t summe = 1469598103934665603ULL;
            const auto dazu = [&summe](const void* p, std::size_t n) {
                const auto* b = static_cast<const unsigned char*>(p);
                for (std::size_t i = 0; i < n; ++i) { summe = (summe ^ b[i]) * 1099511628211ULL; }
            };
            const auto teil = [&summe](std::size_t nr) {
                g_app->bildSumme[nr] = summe;
                summe = 1469598103934665603ULL;
            };
            const BspMesh& en = g_app->effectMesh;
            for (const BspVertex& v : en.verts) {
                dazu(v.xyz, sizeof(v.xyz));
                dazu(v.st, sizeof(v.st));
                dazu(v.colour, sizeof(v.colour));
            }
            if (!en.indexes.empty()) { dazu(en.indexes.data(), en.indexes.size() * sizeof(en.indexes[0])); }
            for (const BspMesh::Batch& b : en.batches) {
                dazu(&b.shader, sizeof(b.shader));
                dazu(&b.firstIndex, sizeof(b.firstIndex));
                dazu(&b.numIndexes, sizeof(b.numIndexes));
            }
            teil(0);
            const std::vector<SaberGezeichnet> klingen =
                saberGezeichnetesBildNr() == ImGui::GetFrameCount() ? saberGezeichnetesBild() : std::vector<SaberGezeichnet>{};
            for (const SaberGezeichnet& k : klingen) {
                dazu(&k.figur, sizeof(k.figur));
                dazu(&k.fach, sizeof(k.fach));
                dazu(&k.klinge, sizeof(k.klinge));
                dazu(k.wurzel, sizeof(k.wurzel));
                dazu(k.spitze, sizeof(k.spitze));
            }
            teil(1);
            for (const ActorDraw& d : gpuFiguren) {
                dazu(&d.figur, sizeof(d.figur));
                dazu(&d.frame, sizeof(d.frame));
                dazu(&d.frameNext, sizeof(d.frameNext));
                dazu(&d.frameFraction, sizeof(d.frameFraction));
                dazu(&d.prevFrame, sizeof(d.prevFrame));
                dazu(&d.blendLerp, sizeof(d.blendLerp));
                dazu(d.pos, sizeof(d.pos));
                dazu(&d.yaw, sizeof(d.yaw));
                dazu(&d.headYaw, sizeof(d.headYaw));
                dazu(&d.torsoFrame, sizeof(d.torsoFrame));
                dazu(&d.torsoNext, sizeof(d.torsoNext));
                dazu(&d.torsoFraction, sizeof(d.torsoFraction));
                dazu(&d.torsoPrevFrame, sizeof(d.torsoPrevFrame));
                dazu(&d.torsoBlendLerp, sizeof(d.torsoBlendLerp));
                dazu(&d.faceFrame, sizeof(d.faceFrame));
                dazu(&d.anbauten, sizeof(d.anbauten));
            }
            teil(2);
            for (const App::PickMover& pm : g_app->pickMover) {
                dazu(&pm.entity, sizeof(pm.entity));
                dazu(pm.welt, sizeof(pm.welt));
            }
            teil(3);
            // Teil 4: der Klirrblitz - saberBlitzZeichnen2D kommt erst nach den
            // Balken (weiter unten) und mischt ihn dann hier hinein.
            teil(4);
            g_app->bildSummeMs = g_app->playMs;
            g_app->bildSummeBild = ImGui::GetFrameCount();
        }
        // --- Auf dem GPU-Weg: dieselbe Ueberlagerung als eigenes Bild ------
        //
        // Gefunden beim Durchsehen aller Kartenfunktionen (27.09.): auf dem
        // GPU-Weg - shanks Einstellung - fehlten Entitykreuze, Kamera,
        // Kamerabahn, Schluessel, Gizmos, Figurenwege und die Blende ganz.
        // drawCameraGizmos schreibt in mapImage.rgba, und das fuellt dort
        // niemand; es brach sofort ab. Damit wurden auch die Klickziele
        // (markers, keyMarks, actorMarks, cameraSx) nie gesetzt - Anklicken
        // und Gizmos gingen ins Leere.
        //
        // Jetzt zeichnet es in ein durchsichtiges Bild derselben Groesse,
        // das ueber das GPU-Bild gelegt wird. Verdeckt wird wie beim
        // Rasterer: die Tiefe kommt aus dem GPU-Tiefenpuffer zurueck.
        g_app->blendeZeigen = false;
        if (gpuBereit && g_app->gpuAufrufe > 0 && blende) {
            // Balken und Vollbild-Blende aus dem Kamerastand, wie im Spiel:
            // nach ENABLE blenden die Balken in einer Sekunde ein, nach
            // DISABLE aus; FADE ueberdeckt alles (CGCam_DrawWideScreen).
            // Beim Blick auf eine einzelne Einstellung (Auswahl) - oder in
            // einem Skript ganz ohne ENABLE - volle Balken wie bisher.
            //
            // Bis rc593 wurden sie hier Bildpunkt fuer Bildpunkt in ein
            // durchsichtiges Bild in Fenstergroesse gemalt und das jedes Bild
            // hochgeladen - bei 2500 x 1300 ein Fuenftel bis die Haelfte der
            // Bildzeit einer Kamerafahrt. Gizmos gibt es beim Blick durch die
            // Kamera nicht, das Bild trug also nur diese Rechtecke; die
            // zeichnet jetzt die Grafikkarte (blendeUeberlagern), mit
            // derselben Mischung "Quelle ueber Ziel".
            float balken = 1.0F;
            float vollblende[4] = {0, 0, 0, 0};
            const bool zeitleiste = playingNow || (g_app->camViewActive && g_app->selectedPath.empty());
            bool hatEnable = false;
            for (const CamSegment& sg : g_app->camTrack.segments) {
                if (sg.kind == CamSegment::Kind::Enable) { hatEnable = true; break; }
            }
            if (zeitleiste && hatEnable) {
                const CameraState cs = g_app->camTrack.at(g_app->playMs);
                balken = cs.bars;
                for (int k = 0; k < 4; ++k) { vollblende[k] = cs.fadeColor[k]; }
            }
            g_app->blendeZeigen = true;
            g_app->blendeBalken = balken;
            for (int k = 0; k < 4; ++k) { g_app->blendeFarbe[k] = vollblende[k]; }
            g_app->gizmoZeigen = false;
            g_app->gizmoBild.rgba.clear();
            g_app->gizmoBild.depth.clear();
        } else if (gpuBereit && g_app->gpuAufrufe > 0) {
            MapImage& ov = g_app->gizmoBild;
            ov.width = g_app->mapImage.width;
            ov.height = g_app->mapImage.height;
            const std::size_t n = static_cast<std::size_t>(ov.width) *
                                  static_cast<std::size_t>(ov.height);
            // --- Nur leeren und hochladen, was sich aendert (rc594) ----------
            //
            // Vorher jedes Bild das ganze Bild genullt und ganz hochgeladen
            // (13 MB bei 2500 x 1300). Jetzt: das Rechteck des vorigen Bildes
            // leeren, zeichnen, und alt + neu hochladen. Die Textur hat damit
            // danach genau den Inhalt von ov.rgba, wie vorher.
            int* alt = g_app->gizmoBoxAlt;
            const bool ganz = ov.rgba.size() != n * 4U;
            if (ganz) {
                ov.rgba.assign(n * 4U, 0);
            } else if (alt[2] > alt[0] && alt[3] > alt[1]) {
                const int x0 = std::clamp(alt[0], 0, ov.width);
                const int x1 = std::clamp(alt[2], 0, ov.width);
                const int y1 = std::clamp(alt[3], 0, ov.height);
                for (int y = std::clamp(alt[1], 0, ov.height); y < y1; ++y) {
                    std::uint8_t* zeile = ov.rgba.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(ov.width) +
                                                            static_cast<std::size_t>(x0)) * 4U;
                    std::fill(zeile, zeile + static_cast<std::size_t>(x1 - x0) * 4U, std::uint8_t{0});
                }
            }
            ov.depth.clear();
            int* box = g_app->gizmoBox;
            box[0] = 0;
            box[1] = 0;
            box[2] = 0;
            box[3] = 0;
            // Ohne Blende (die hat ihren eigenen Zweig oben) ist das die
            // freie Ansicht - mit Gizmos. Beim Bewegen die Tiefe des vorigen
            // Bildes (siehe gpu::tiefeAbbilden).
            g_app->gizmoTiefeNah = 4.0F;
            g_app->gizmoTiefeFern = kartenFern();
            if (!gpu::tiefeAbbilden(g_gizmoTiefe, ov.width, ov.height, kameraBewegt)) {
                g_gizmoTiefe = gpu::TiefenSicht{};   // ohne Tiefe: alles obenauf
            }
            drawCameraGizmos(ov);
            if (g_gizmoTiefe.daten != nullptr) {
                gpu::tiefeFreigeben();
                g_gizmoTiefe = gpu::TiefenSicht{};
            }
            g_app->gizmoZeigen = true;
            if (g_app->gizmoTexture == nullptr || g_app->gizmoTexW != ov.width ||
                g_app->gizmoTexH != ov.height) {
                g_app->gizmoTexture = render::createTexture(ov.rgba.data(), ov.width, ov.height);
                g_app->gizmoTexW = ov.width;
                g_app->gizmoTexH = ov.height;
            } else if (ganz) {
                render::updateTexture(g_app->gizmoTexture, ov.rgba.data(), ov.width, ov.height);
            } else {
                int u[4] = {box[0], box[1], box[2], box[3]};
                if (alt[2] > alt[0] && alt[3] > alt[1]) {
                    if (u[2] <= u[0]) {
                        for (int k = 0; k < 4; ++k) { u[k] = alt[k]; }
                    } else {
                        u[0] = std::min(u[0], alt[0]);
                        u[1] = std::min(u[1], alt[1]);
                        u[2] = std::max(u[2], alt[2]);
                        u[3] = std::max(u[3], alt[3]);
                    }
                }
                render::updateTextureBereich(g_app->gizmoTexture, ov.rgba.data(), ov.width, ov.height,
                                             u[0], u[1], u[2], u[3]);
            }
            for (int k = 0; k < 4; ++k) { alt[k] = box[k]; }
        } else {
            g_app->gizmoZeigen = false;
        }
        // Stand das Netz noch nicht bereit, im naechsten Bild wieder
        // versuchen. Ohne Grafikkarte gibt es nichts zu versuchen.
        g_app->mapDirty = gpu::verfuegbar() && !gpuBereit;
    }

    if (ImGui::GetIO().Framerate > 0.0F) {
        g_app->frameCost.frame = 1000.0F / ImGui::GetIO().Framerate;
    }
    // --- Jedes wirklich gezeichnete Bild notieren ------------------------
    //
    // Nur wenn dieses Bild neu gezeichnet wurde.
    // Im Stillstand passiert nichts, und genau das war der Fehler des
    // frueheren Knopfes: er mass den Leerlauf.
    if (g_app->logFrameCost && gezeichnet) {
        ++g_app->frameCostTick;
        if (g_app->frameCostTick % 20 == 0) {
            const App::FrameCost& k = g_app->frameCost;
            char z[300];
            std::snprintf(z, sizeof(z),
                          "Zeit je Bild #%d: Karte %.1f, Figuren %.1f, "
                          "Effekte %.1f, Szene %.1f, Gluehen %.1f, "
                          "Gizmos %.1f, gemessen %.1f, "
                          "Bild wirklich %.1f ms | ROH: Karte %.1f, "
                          "gemessen %.1f",
                          g_app->frameCostTick,
                          static_cast<double>(k.map),
                          static_cast<double>(k.actors),
                          static_cast<double>(k.effects),
                          static_cast<double>(k.scene),
                          static_cast<double>(k.glow),
                          static_cast<double>(k.gizmos),
                          static_cast<double>(k.total),
                          static_cast<double>(k.frame),
                          static_cast<double>(k.rawMap),
                          static_cast<double>(k.rawTotal));
            diag::info(z);
        }
    }
    // Das Renderziel selbst wird gezeigt - es wurde direkt bemalt und muss
    // nicht erst hochgeladen werden.
    void* zeigeTextur = nullptr;
    if (gpu::zielTextur() != nullptr && g_app->gpuAufrufe > 0) {
        zeigeTextur = gpu::zielTextur();
    }
    if (!gpu::verfuegbar()) {
        // Ohne Direct3D 11 (OpenGL-Rueckfall oder ein Bau ohne D3D) gibt es
        // keine Kartenansicht mehr - der Software-Rasterer ist entfernt.
        // Sagen statt schwarz lassen.
        const ImVec2 hier = ImGui::GetCursorPos();
        ImGui::Dummy(ImVec2{static_cast<float>(w), static_cast<float>(h)});
        ImGui::SetCursorPos(ImVec2{hier.x + 12.0F, hier.y + 12.0F});
        ImGui::TextWrapped("%s", tr(Str::MapNeedsD3d));
    }
    if (zeigeTextur != nullptr) {
        const ImVec2 topLeft = ImGui::GetCursorScreenPos();
        const ImVec2 imgTopLeft = ImGui::GetCursorScreenPos();
        ImGui::Image(reinterpret_cast<ImTextureID>(zeigeTextur),
                     ImVec2{static_cast<float>(w), static_cast<float>(h)});
        // Die Ueberlagerung des GPU-Wegs obenauf (siehe oben).
        if (g_app->gizmoZeigen && g_app->gizmoTexture != nullptr) {
            ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(g_app->gizmoTexture),
                                                 ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        }
        if (g_app->blendeZeigen) {
            // Paket kamera, rc601: Balken und Blende im RAHMEN des Spielbilds
            // - CGCam_DrawWideScreen fuellt in 640x480 virtuellen Punkten, also
            // ueber das ganze Spielfenster (cg_camera.cpp:1328/1329/1336
            // OpenJK, :1293/1294/1303 Movie Duels). Ein Balken ist ein Zehntel
            // der Hoehe DES SPIELBILDS (bar_height_dest = 480/10, :86).
            ImVec2 ra;
            ImVec2 rb;
            spielRahmenRechteck(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ra, rb);
            blendeUeberlagern(ImGui::GetWindowDrawList(), ra, rb,
                              std::max(1, static_cast<int>(std::lround(static_cast<float>(g_app->mapImage.height) *
                                                                       g_app->spielRahmen.hoehe))));
        }
        // rc601 saber review 2c: der Klirrblitz NACH den Balken, wie CG_Draw2D.
        saberBlitzZeichnen2D(0, ImGui::GetWindowDrawList(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                             g_app->blendeZeigen && g_app->blendeBalken > 0.0F);
        {
            const ImVec2 a = ImGui::GetItemRectMin();
            const ImVec2 b = ImGui::GetItemRectMax();
            g_app->kartenAnsicht[0] = a.x;
            g_app->kartenAnsicht[1] = a.y;
            g_app->kartenAnsicht[2] = b.x;
            g_app->kartenAnsicht[3] = b.y;
        }
        // Eine gewaehlte Entity ueber den Boden ziehen (gui/app_mission.cpp).
        // Reihenfolge: die Rauten der Animation zuerst - sie liegen oft genau
        // auf dem Ursprung der Entity, und sonst griffe das Entity-Ziehen den
        // Klick und verschoebe das Schiff statt des Schluessels.
        g_app->wegKlickVerbraucht = false;
        animMaus();
        bahnGriffeMaus();
        entityZiehen(w > 0 ? static_cast<float>(g_app->mapImage.width) / static_cast<float>(w) : 1.0F);
        wegWerkzeugMaus(w > 0 ? static_cast<float>(g_app->mapImage.width) / static_cast<float>(w) : 1.0F);
        platzierMaus();
        skriptFokusAuffrischen();
        entityUeberlagerung(w > 0 ? static_cast<float>(g_app->mapImage.width) / static_cast<float>(w) : 1.0F);
        if (g_app->skriptFokus) { skriptSchild(); } else { g_app->skriptSchildText.clear(); }
        bahnGriffeZeichnen();
        kameraBahnZeichnen();
        wegUeberlagerung();
        animUeberlagerung();
        // --- Namen an Entities und Figuren -------------------------------
        if (g_app->zeigeNamen) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 a = ImGui::GetItemRectMin();
            dl->PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
            const auto schrift = [&](float x, float y, const std::string& text, ImU32 farbe) {
                const ImVec2 p{a.x + x + 6.0F, a.y + y - ImGui::GetFontSize() * 0.5F};
                dl->AddText(ImVec2{p.x + 1.0F, p.y + 1.0F}, IM_COL32(0, 0, 0, 200), text.c_str());
                dl->AddText(p, farbe, text.c_str());
            };
            if (g_app->showEntities) {
                for (const App::PickedMarker& m : g_app->markers) {
                    if (m.entity >= g_app->map.entities.size()) {
                        continue;
                    }
                    const MapEntity& e = g_app->map.entities[m.entity];
                    const std::string name = entitySkriptName(e);
                    const int kat = entityKategorie(e);
                    const EntityKat& ek = entityKategorieInfo(std::max(kat, 0));
                    schrift(m.sx, m.sy, name.empty() ? e.classname : name, IM_COL32(ek.r, ek.g, ek.b, 230));
                }
            }
            for (const App::NamensMarke& f : g_app->figurNamen) {
                schrift(f.sx, f.sy, f.name, IM_COL32(255, 255, 255, 240));
            }
            dl->PopClipRect();
        }

        // --- target_print: der Text in der Bildmitte --------------------
        //
        // CG_DrawCenterString (cg_text.cpp:695): im 640x480-Raster
        // waagrecht mittig, die Zeilen um y = 480 * 0.25 gestapelt
        // (CG_CenterPrint_f), weiss, Schrift "ergoec" mit pointSize 20 mal
        // cg_textprintscale 0.5 - also 10 von 480 Zeilen hoch. 3 s zu
        // sehen, die letzten 200 ms blendet er aus.
        //
        // Waehrend einer Skriptkamera zeichnet die Engine ihn NICHT
        // (CG_Draw2D: in_camera -> frueher Ausstieg, cg_draw.cpp:9451);
        // endet die Kamera vor Ablauf der drei Sekunden, erscheint der Rest.
        // Und nur, was der Spieler ausgeloest hat, kommt ueberhaupt an
        // (MoverSim::bildschirmtextAt beachtet das schon).
        //
        // OFFEN: "@REFERENZ"-Texte bleiben roh - behaved liest die
        // StringPackages (strings/*.str) nicht; die Engine zeigt dann
        // ebenfalls den rohen Text, nur mit Fehlermeldung.
        if (const Bildschirmtext* bt = g_app->moverSim.bildschirmtextAt(g_app->playMs)) {
            const CameraState cs = g_app->camTrack.at(g_app->playMs);
            const float deck = bildschirmtextDeckkraft(*bt, g_app->playMs);
            if (!cs.enabled && deck > 0.0F) {
                const ImVec2 a = ImGui::GetItemRectMin();
                const ImVec2 b = ImGui::GetItemRectMax();
                const float breite = b.x - a.x;
                const float hoehe = b.y - a.y;
                const float zeile = std::max(10.0F, hoehe * 10.0F / 480.0F);
                std::vector<std::string> zeilen;
                {
                    std::string rest = bt->text;
                    for (;;) {
                        const std::size_t nl = rest.find('\n');
                        zeilen.push_back(rest.substr(0, nl));
                        if (nl == std::string::npos) { break; }
                        rest = rest.substr(nl + 1);
                    }
                }
                ImFont* schrift = ImGui::GetFont();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const auto alpha = static_cast<int>(deck * 255.0F);
                float y = a.y + hoehe * 0.25F - static_cast<float>(zeilen.size()) * zeile * 0.5F;
                for (const std::string& z : zeilen) {
                    const float tw = schrift->CalcTextSizeA(zeile, 1.0e9F, 0.0F, z.c_str()).x;
                    const float x = a.x + (breite - tw) * 0.5F;
                    // Ohne Schatten: CG_DrawCenterString gibt kein
                    // STYLE_DROPSHADOW mit - Weiss auf hellem Grund ist im
                    // Spiel genauso schlecht zu lesen.
                    dl->AddText(schrift, zeile, ImVec2{x, y}, IM_COL32(255, 255, 255, alpha), z.c_str());
                    y += zeile;
                }
            }
        }

        // --- Anklicken einer Entity ------------------------------------
        //
        // SomaZ' BSP-Entity-Edit macht das ueber einen Nebenpuffer mit
        // Kennungen (ogl_fbo.py): jede Entity wird in einer eigenen Farbe
        // gezeichnet, beim Klick liest man den Bildpunkt.
        //
        // Unser Zeichner ist Software, da geht es einfacher: die Marken
        // merken sich beim Zeichnen ihren Bildschirmort, und der Klick nimmt
        // die naechstgelegene. Das ist zudem angenehmer zu treffen - eine
        // Marke ist ein duennes Kreuz, ein Nebenpuffer verlangt einen
        // Treffer auf den Strich.
        // --- Das Gizmo anfassen und ziehen ----------------------------
        //
        // Vor der uebrigen Klickbehandlung, denn ein Griff am Gizmo soll
        // NICHT zugleich die Auswahl aendern.
        //
        // Das Umrechnen Mausweg -> Weltweg geht ueber die Achse selbst: sie
        // liegt als Strecke im Bild vor (Anfang und Ende gemerkt), und ihre
        // Laenge in Welteinheiten ist bekannt. Der Mausweg wird auf die
        // Bildrichtung der Achse PROJIZIERT und im selben Verhaeltnis
        // umgerechnet. Damit stimmt die Bewegung aus jedem Blickwinkel, und
        // eine Achse, die fast auf den Betrachter zeigt, bewegt sich von
        // selbst kaum - genau richtig, denn dort kann man nicht zielen.
        {
            const ImVec2 m2 = ImGui::GetIO().MousePos;
            // --- Maus in RASTERkoordinaten ---------------------------
            //
            // Das Renderziel und das angezeigte Bild koennen verschieden gross
            // sein (Fensterkante, 64-Punkte-Untergrenze). Alles, was der Zeichner an
            // Bildschirmlagen liefert - Marker, Gizmoachsen - steht dann in
            // der kleineren Aufloesung, die Maus aber in der grossen.
            //
            // Ohne diese Umrechnung greift man waehrend einer Fahrt daneben,
            // und zwar genau um den Faktor.
            const float rasterSkal =
                (w > 0) ? static_cast<float>(g_app->mapImage.width) /
                              static_cast<float>(w)
                        : 1.0F;
            const float mx2 = (m2.x - imgTopLeft.x) * rasterSkal;
            const float my2 = (m2.y - imgTopLeft.y) * rasterSkal;
            // Ueber welcher Achse steht die Maus? Nur zum Aufleuchten.
            //
            // Ohne das raet man, ob man trifft - gemeldet als "wenn ich bei
            // der Kamera ueber die Linien hover sollten sie aufleuchten das
            // man weiss man ist genau drueber".
            auto abstandZurAchse = [&](int ax) {
                if (g_app->gizmoSx[ax][1] < 0.0F ||
                    g_app->gizmoSx[ax][0] < 0.0F) {
                    return 1.0e30F;
                }
                const float x0 = g_app->gizmoSx[ax][0];
                const float y0 = g_app->gizmoSy[ax][0];
                const float vx = g_app->gizmoSx[ax][1] - x0;
                const float vy = g_app->gizmoSy[ax][1] - y0;
                const float vv = vx * vx + vy * vy;
                float t = 0.0F;
                if (vv > 0.0F) {
                    t = ((mx2 - x0) * vx + (my2 - y0) * vy) / vv;
                    t = std::clamp(t, 0.0F, 1.0F);
                }
                const float dx = x0 + vx * t - mx2;
                const float dy = y0 + vy * t - my2;
                return dx * dx + dy * dy;
            };
            // Der Abstand zum RING einer Achse - kuerzester Abstand zu
            // irgendeinem seiner Teilstuecke. Der Drehmodus zeichnet Ringe
            // und keine Achsenstrecken; der Test muss messen, was zu sehen
            // ist. Vorher mass er gegen die (unsichtbaren, veralteten)
            // Strecken des Verschiebens - deshalb "schwer greifbar".
            auto abstandZumRing = [&](int ax) {
                float best = 1.0e30F;
                for (int t = 0; t + 1 < App::kRingPts; ++t) {
                    const float x0 = g_app->gizmoRingSx[ax][t];
                    const float y0 = g_app->gizmoRingSy[ax][t];
                    const float x1 = g_app->gizmoRingSx[ax][t + 1];
                    const float y1 = g_app->gizmoRingSy[ax][t + 1];
                    if (x0 < 0.0F || x1 < 0.0F) {
                        continue;
                    }
                    const float vx = x1 - x0;
                    const float vy = y1 - y0;
                    const float vv = vx * vx + vy * vy;
                    float tt = 0.0F;
                    if (vv > 0.0F) {
                        tt = ((mx2 - x0) * vx + (my2 - y0) * vy) / vv;
                        tt = std::clamp(tt, 0.0F, 1.0F);
                    }
                    const float dx = x0 + vx * tt - mx2;
                    const float dy = y0 + vy * tt - my2;
                    const float d = dx * dx + dy * dy;
                    if (d < best) {
                        best = d;
                    }
                }
                return best;
            };
            const float reach2 = ImGui::GetFontSize() * 1.1F;
            if (g_app->gizmoAxis < 0) {
                // Was vorher galt - fuer den Vergleich unten.
                const int vorher = g_app->gizmoHover;
                g_app->gizmoHover = -1;
                float best2 = reach2 * reach2;
                const bool dreht =
                    (g_app->gizmoMode == App::GizmoMode::Rotate);
                // Das Kaestchen in der Mitte zuerst - es liegt oben. Nur
                // im Verschieben: der Drehmodus hat keines.
                if (!dreht && g_app->gizmoShown &&
                    g_app->gizmoSx[0][0] >= 0.0F) {
                    const float dx = g_app->gizmoSx[0][0] - mx2;
                    const float dy = g_app->gizmoSy[0][0] - my2;
                    const float mitte = ImGui::GetFontSize() * 0.7F;
                    if (dx * dx + dy * dy < mitte * mitte) {
                        g_app->gizmoHover = App::kGizmoFree;
                    }
                }
                if (g_app->gizmoShown && g_app->gizmoHover < 0) {
                    for (int ax = 0; ax < 3; ++ax) {
                        const float d = dreht ? abstandZumRing(ax)
                                              : abstandZurAchse(ax);
                        if (d < best2) {
                            best2 = d;
                            g_app->gizmoHover = ax;
                        }
                    }
                    // Der Ring um die Hochachse auch im Verschieben (rc597).
                    if (!dreht && g_app->kamYawRingInMove) {
                        const float d = abstandZumRing(2);
                        if (d < best2) {
                            best2 = d;
                            g_app->gizmoHover = App::kGizmoYawRing;
                        }
                    }
                }
                // Hat sich etwas geaendert? Dann NEU ZEICHNEN.
                //
                // Das war der Grund, warum das Aufleuchten "inkonsistent"
                // wirkte: die Ansicht wird nur neu gezeichnet, wenn
                // mapDirty gesetzt ist. Ohne das blieb das Bild stehen -
                // die Achse leuchtete erst auf, wenn zufaellig etwas
                // anderes ein Neuzeichnen ausloeste, und blieb dann
                // leuchten, wenn man wegfuhr.
                if (g_app->gizmoHover != vorher) {
                    g_app->mapDirty = true;
                }
            }

            if (g_app->gizmoMode == App::GizmoMode::Move && g_app->gizmoAxis < 0 && g_app->gizmoShown &&
                g_app->cameraSelected && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                g_app->gizmoHover == App::kGizmoYawRing) {
                // Der Ring im Verschieben: drehen um die Hochachse, danach
                // wieder verschieben (rc597).
                g_app->gizmoMode = App::GizmoMode::Rotate;
                g_app->gizmoHover = 2;
                g_app->kamRingAusMove = true;
            }
            if (g_app->gizmoMode == App::GizmoMode::Move &&
                g_app->gizmoAxis < 0 && g_app->gizmoShown && g_app->cameraSelected &&
                ImGui::IsItemHovered() &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                g_app->gizmoHover >= 0 && g_app->gizmoHover != App::kGizmoYawRing) {
                // Mitten in einer Fahrt: erst dort einen Schluessel (rc597).
                g_app->kameraAutoKeyEingefuegt =
                    g_app->kameraGizmoZwischen && kameraSchluesselEinfuegen(g_app->playMs);
g_app->gizmoAxis = g_app->gizmoHover;
                g_app->gizmoDragSx = mx2;
                g_app->gizmoDragSy = my2;
                for (int k = 0; k < 3; ++k) {
                    g_app->gizmoStart[k] = g_app->gizmoOffset[k];
                }
                // Den Bezug EINFRIEREN - siehe app_internal.h.
                if (g_app->gizmoAxis < App::kGizmoFree) {
                    const int ax = g_app->gizmoAxis;
                    for (int e = 0; e < 2; ++e) {
                        g_app->gizmoFixSx[e] = g_app->gizmoSx[ax][e];
                        g_app->gizmoFixSy[e] = g_app->gizmoSy[ax][e];
                    }
                    g_app->gizmoFixLen = g_app->gizmoWorldLen;
                    // Auch die RICHTUNG einfrieren: im lokalen System zeigt
                    // die Achse nicht mehr entlang X, Y oder Z.
                    for (int a = 0; a < 3; ++a) {
                        for (int k = 0; k < 3; ++k) {
                            g_app->gizmoAxisDir[a][k] = g_app->gizmoDirNow[a][k];
                        }
                    }
                } else {
                    // Freie Bewegung: die Bildebene der Kamera festhalten.
                    g_app->cam.right(g_app->gizmoFreeRight);
                    g_app->cam.up(g_app->gizmoFreeUp);
                    // Wie viele Welteinheiten sind ein Bildpunkt? Von der
                    // Achse ablesen, die im Bild am LAENGSTEN ist - die ist
                    // am wenigsten verkuerzt und damit am verlaesslichsten.
                    float beste = 0.0F;
                    for (int ax = 0; ax < 3; ++ax) {
                        if (g_app->gizmoSx[ax][1] < 0.0F) { continue; }
                        const float dx =
                            g_app->gizmoSx[ax][1] - g_app->gizmoSx[ax][0];
                        const float dy =
                            g_app->gizmoSy[ax][1] - g_app->gizmoSy[ax][0];
                        const float l = std::sqrt(dx * dx + dy * dy);
                        if (l > beste) { beste = l; }
                    }
                    g_app->gizmoFreeScale =
                        (beste > 1.0F) ? (g_app->gizmoWorldLen / beste) : 0.0F;
                }
            }
            // --- Drehen ---------------------------------------------------
            //
            // Waagerecht ziehen dreht: 180 Bildpunkte sind 90 Grad. Welche
            // Achse, sagt derselbe Ueberfahr-Test wie beim Verschieben -
            // die Ringe liegen dort, wo die Achsen laegen.
            if (g_app->gizmoMode == App::GizmoMode::Rotate) {
                if (!g_app->gizmoRotating && g_app->gizmoShown && g_app->cameraSelected &&
                    ImGui::IsItemHovered() &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                    g_app->gizmoHover >= 0 &&
                    g_app->gizmoHover < App::kGizmoFree) {
                    g_app->kameraAutoKeyEingefuegt =
                        g_app->kameraGizmoZwischen && kameraSchluesselEinfuegen(g_app->playMs);
    g_app->gizmoRotating = true;
                    g_app->gizmoAxis = g_app->gizmoHover;
                    g_app->gizmoRotStartSx = mx2;
                    g_app->gizmoRotStartSy = my2;
                    for (int k = 0; k < 3; ++k) {
                        g_app->gizmoRotStart[k] = g_app->gizmoAngles[k];
                    }
                    // Achsen und Ausgangswinkel EINFRIEREN - dasselbe
                    // Prinzip wie beim Verschieben (rc156). Im Weltsystem
                    // wandert die Kameraachse waehrend der Drehung; wuerde
                    // man jedes Bild neu ablesen, koppelte das zurueck.
                    for (int k = 0; k < 3; ++k) {
                        g_app->gizmoAngBase[k] = g_app->gizmoAngNow[k];
                    }
                    for (int a = 0; a < 3; ++a) {
                        for (int k = 0; k < 3; ++k) {
                            g_app->gizmoAxisDir[a][k] = g_app->gizmoDirNow[a][k];
                        }
                    }
                }
                if (g_app->gizmoRotating) {
                    const float mdx2 = mx2 - g_app->gizmoRotStartSx;
                    const int ax = std::clamp(g_app->gizmoAxis, 0, 2);
                    for (int k = 0; k < 3; ++k) {
                        g_app->gizmoAngles[k] = g_app->gizmoRotStart[k];
                    }
                    const float grad = mdx2 * 0.5F;
                    if (g_app->gizmoRotSpace == App::GizmoSpace::Local) {
                        // PAN ist Pitch, Yaw, Roll - in dieser Reihenfolge,
                        // wie ueberall in Quake-Engines. Achse 0 ist rechts
                        // (Pitch), 1 ist oben (Yaw), 2 ist vorwaerts
                        // (Roll). Im lokalen System entspricht jede Achse
                        // GENAU einem dieser Werte - deshalb kommt man hier
                        // ohne Umrechnung aus.
                        g_app->gizmoAngles[ax] = g_app->gizmoRotStart[ax] + grad;
                    } else {
                        // Weltsystem: um X, Y oder Z der Karte drehen.
                        //
                        // Eine Weltdrehung laesst sich NICHT auf einen der
                        // drei Eulerwerte abbilden - sie aendert je nach
                        // Blickrichtung zwei oder drei davon. Also den Weg
                        // ueber die Orientierung nehmen: Basis bauen, um
                        // die Weltachse drehen, Winkel zurueckrechnen.
                        // anglestest haelt fest, dass dieser Rundlauf
                        // stimmt - samt Drehrichtung und Roll-Vorzeichen.
                        Camera basis;
                        for (int k = 0; k < 3; ++k) {
                            basis.angles[k] = g_app->gizmoAngBase[k] +
                                              g_app->gizmoRotStart[k];
                        }
                        float fB[3];
                        float uB[3];
                        basis.forward(fB);
                        basis.up(uB);
                        float fN[3];
                        float uN[3];
                        rotateAboutAxis(fB, g_app->gizmoAxisDir[ax], grad, fN);
                        rotateAboutAxis(uB, g_app->gizmoAxisDir[ax], grad, uN);
                        float neuW[3];
                        anglesFromBasis(fN, uN, neuW);
                        // gizmoAngles ist ein ZUSATZ zum Skriptwert, also
                        // die Differenz bilden.
                        for (int k = 0; k < 3; ++k) {
                            g_app->gizmoAngles[k] =
                                neuW[k] - g_app->gizmoAngBase[k];
                        }
                    }
                    g_app->mapDirty = true;
                    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        g_app->gizmoRotating = false;
                        g_app->gizmoAxis = -1;
                        // Loslassen schreibt (rc588) - wie bei den Entities.
                        {
                            const bool geschrieben = kameraVorschauSchreiben();
                            if (g_app->kameraAutoKeyEingefuegt && !geschrieben) {
                                (void)g_app->doc.undo();
                                rebuildTree();
                                g_app->camTrackValid = false;
                                g_app->mapDirty = true;
                            }
                            g_app->kameraAutoKeyEingefuegt = false;
                        }
                        if (g_app->kamRingAusMove) {
                            g_app->gizmoMode = App::GizmoMode::Move;
                            g_app->kamRingAusMove = false;
                        }
                    }
                }
            }

            if (g_app->gizmoMode == App::GizmoMode::Move &&
                g_app->gizmoAxis >= 0) {
                const float mdx = mx2 - g_app->gizmoDragSx;
                const float mdy = my2 - g_app->gizmoDragSy;
                if (g_app->gizmoAxis == App::kGizmoFree) {
                    // In der Bildebene: rechts mal Mausweg nach rechts,
                    // oben mal Mausweg nach OBEN - der Bildschirm zaehlt
                    // nach unten, daher das Minus.
                    for (int k = 0; k < 3; ++k) {
                        g_app->gizmoOffset[k] =
                            g_app->gizmoStart[k] +
                            (g_app->gizmoFreeRight[k] * mdx -
                             g_app->gizmoFreeUp[k] * mdy) *
                                g_app->gizmoFreeScale;
                    }
                    g_app->mapDirty = true;
                } else {
                    const int ax = g_app->gizmoAxis;
                    const float sdx =
                        g_app->gizmoFixSx[1] - g_app->gizmoFixSx[0];
                    const float sdy =
                        g_app->gizmoFixSy[1] - g_app->gizmoFixSy[0];
                    const float slen2 = sdx * sdx + sdy * sdy;
                    if (slen2 > 1.0F && g_app->gizmoFixLen > 0.0F) {
                        const float t = (mdx * sdx + mdy * sdy) / slen2;
                        // Entlang der eingefrorenen Richtung. Im
                        // Weltsystem ist das der Einheitsvektor der Achse,
                        // also genau das bisherige gizmoOffset[ax] += ...
                        for (int k = 0; k < 3; ++k) {
                            g_app->gizmoOffset[k] =
                                g_app->gizmoStart[k] +
                                g_app->gizmoAxisDir[ax][k] * t *
                                    g_app->gizmoFixLen;
                        }
                        g_app->mapDirty = true;
                    }
                }
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    g_app->gizmoAxis = -1;
                    // Loslassen schreibt ins Skript (rc588). Vorher stand die
                    // Verschiebung als Vorschau da, bis man unten "Apply"
                    // fand - anders als bei den Entities, und leicht vergessen.
                    {
                            const bool geschrieben = kameraVorschauSchreiben();
                            if (g_app->kameraAutoKeyEingefuegt && !geschrieben) {
                                (void)g_app->doc.undo();
                                rebuildTree();
                                g_app->camTrackValid = false;
                                g_app->mapDirty = true;
                            }
                            g_app->kameraAutoKeyEingefuegt = false;
                        }
                }
            }
        }

        // Kein return: der Rest der Ansicht - Fadenkreuz, Zustandszeile -
        // soll weiter gezeichnet werden. Nur die Auswahl darf ein Griff am
        // Gizmo nicht aendern, und dafuer genuegt diese eine Bedingung.
        // --- Rechtsklick: die Betriebsart waehlen ---------------------
        //
        // Wie in 3ds Max, wo Auswaehlen, Verschieben und Drehen drei
        // getrennte Werkzeuge sind. Ohne diese Trennung ist jeder Klick in
        // die Naehe einer Achse ein Verschieben - und man kann nichts mehr
        // anklicken, ohne etwas zu bewegen.
        // Nur bei einem KURZEN Rechtsklick, nicht nach dem Drehen.
        //
        // Mit der rechten Maustaste dreht man die Kamera. BeginPopupContextItem
        // oeffnet aber beim LOSLASSEN - also auch nach einem langen Zug ueber
        // das halbe Bild. Gemeldet als: "wenn ich das gedrueckt halte und los
        // lasse oeffnet sich das rechtsklick menu".
        //
        // ImGui fuehrt mit, wie weit die Maus seit dem Druecken gewandert ist:
        // io.MouseDragMaxDistanceSqr. Ist das mehr als ein paar Bildpunkte,
        // war es ein Zug und kein Klick. Genau diese Groesse benutzt ImGui
        // selbst fuer seine Zugschwelle - sie steht in imgui.h und braucht
        // keine interne Kopfdatei.
        {
            const ImGuiIO& io2 = ImGui::GetIO();
            const float grenze = io2.MouseDragThreshold * io2.MouseDragThreshold;
            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
                io2.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] <= grenze &&
                g_app->bahnMenueBild != ImGui::GetFrameCount()) {   // ein Bahngriff hat ihn schon
                ImGui::OpenPopup("gizmomodus");
                // Fuer "Neue Entity hier": wo geklickt wurde.
                g_app->rechtsklickBild[0] = io2.MousePos.x;
                g_app->rechtsklickBild[1] = io2.MousePos.y;
            }
        }
        if (ImGui::BeginPopup("gizmomodus")) {
            // Eine Entity an die Stelle setzen, auf die man geklickt hat -
            // der schnellste Weg, eine Figur oder ein Laufziel hinzustellen.
            ImGui::BeginDisabled(g_app->geo.empty() || g_app->camViewActive);
            if (ImGui::BeginMenu(tr(Str::EntNewHere))) {
                entityNeuMenue(true);
                ImGui::EndMenu();
            }
            ImGui::EndDisabled();
            // Eine Kameraeinstellung an der jetzigen Ansicht (rc588).
            ImGui::BeginDisabled(g_app->geo.empty() || g_app->camViewActive);
            if (ImGui::MenuItem(tr(Str::CamNewShot))) { kameraNeueEinstellung(); }
            ImGui::EndDisabled();
            ImGui::Separator();
            if (ImGui::MenuItem(tr(Str::GizmoSelect), nullptr,
                                g_app->gizmoMode == App::GizmoMode::Select)) {
                g_app->gizmoMode = App::GizmoMode::Select;
                g_app->mapDirty = true;
            }
            if (ImGui::MenuItem(tr(Str::GizmoMove), nullptr,
                                g_app->gizmoMode == App::GizmoMode::Move)) {
                g_app->gizmoMode = App::GizmoMode::Move;
                // Ist eine Kamera ausgewaehlt, aber noch kein Schluessel,
                // gleich den ersten nehmen - sonst steht man im
                // Verschiebemodus ohne etwas zu verschieben.
                // Der Schluessel, der zur eingestellten ZEIT gilt - nicht
                // der erste. Vorher sprang die Kamera beim Umschalten auf
                // den ersten MOVE, egal wo die Zeitleiste stand.
                if (g_app->cameraSelected && g_app->selectedKey < 0) {
                    g_app->selectedKey = schluesselFuerZeit(g_app->playMs);
                    g_app->gizmoZeitBezug = g_app->playMs;
                }
                g_app->mapDirty = true;
            }
            // Drehen heisst, den PAN-Befehl zu aendern - das gehoert zum
            // Rueckschreiben und kommt damit zusammen. Der Eintrag steht
            // schon da, abgeblendet, damit klar ist, dass er vorgesehen ist.
            if (ImGui::MenuItem(tr(Str::GizmoRotate), nullptr,
                                g_app->gizmoMode == App::GizmoMode::Rotate)) {
                g_app->gizmoMode = App::GizmoMode::Rotate;
                // Der Schluessel, der zur eingestellten ZEIT gilt - nicht
                // der erste. Vorher sprang die Kamera beim Umschalten auf
                // den ersten MOVE, egal wo die Zeitleiste stand.
                if (g_app->cameraSelected && g_app->selectedKey < 0) {
                    g_app->selectedKey = schluesselFuerZeit(g_app->playMs);
                    g_app->gizmoZeitBezug = g_app->playMs;
                }
                g_app->mapDirty = true;
            }
            // --- Bezugssystem ------------------------------------------
            //
            // Wie die Liste "Reference Coordinate System" in 3ds Max, und
            // wie dort JE UMFORMUNG eigen: "If you change this pull down
            // list for move, that does not change it for rotate." Welche
            // gerade gilt, richtet sich nach dem eingestellten Modus.
            if (g_app->gizmoMode != App::GizmoMode::Select) {
                ImGui::Separator();
                const bool dreht = (g_app->gizmoMode == App::GizmoMode::Rotate);
                App::GizmoSpace& raum =
                    dreht ? g_app->gizmoRotSpace : g_app->gizmoMoveSpace;
                if (ImGui::MenuItem(tr(Str::GizmoSpaceWorld), nullptr,
                                    raum == App::GizmoSpace::World)) {
                    raum = App::GizmoSpace::World;
                    g_app->mapDirty = true;
                }
                if (ImGui::MenuItem(tr(Str::GizmoSpaceLocal), nullptr,
                                    raum == App::GizmoSpace::Local)) {
                    raum = App::GizmoSpace::Local;
                    g_app->mapDirty = true;
                }
            }
            ImGui::EndPopup();
        }

        // Angefasst zum Ziehen (entityZiehen)? Dann bleibt die Auswahl, wie
        // sie ist - ein Klick auf das Gewaehlte waehlt nicht ab.
        if (g_app->gizmoAxis < 0 && !g_app->entZug.kandidat && !g_app->wegKlickVerbraucht && ImGui::IsItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 m = ImGui::GetIO().MousePos;
            // Siehe die Notiz weiter oben zur Rastergroesse.
            const float rasterSkal2 =
                (w > 0) ? static_cast<float>(g_app->mapImage.width) /
                              static_cast<float>(w)
                        : 1.0F;
            const float mx = (m.x - imgTopLeft.x) * rasterSkal2;
            const float my = (m.y - imgTopLeft.y) * rasterSkal2;
            const float reach = ImGui::GetFontSize() * 1.2F;
            float best = reach * reach;
            int found = -1;
            for (const App::PickedMarker& mk : g_app->markers) {
                const float dx = mk.sx - mx;
                const float dy = mk.sy - my;
                const float d = dx * dx + dy * dy;
                if (d < best) {
                    best = d;
                    found = static_cast<int>(mk.entity);
                }
            }
            // Die Kamera mitpruefen - sie ist nicht in markers, weil sie zu
            // keiner Entity gehoert.
            // Die Kamera trifft man grosszuegiger als eine Entity.
            //
            // Gemeldet als "irgendwie laesst sich die Kamera auch nicht so
            // gut selecten". Der Grund: sie wurde mit derselben engen
            // Reichweite geprueft wie ein Entity-Kreuz, obwohl ihr Rahmen
            // ein vielfach groesseres Gebilde ist. Jetzt zaehlt der ganze
            // Bereich, den das Gizmo einnimmt - "kann man die am ganzen
            // gizmo selecten" war genau der Vorschlag.
            bool traf = false;
            if (g_app->cameraSx >= 0.0F) {
                const float dx = g_app->cameraSx - mx;
                const float dy = g_app->cameraSy - my;
                const float weit = reach * 2.5F;
                if (dx * dx + dy * dy < weit * weit) {
                    traf = true;
                }
            }
            // Erst die Schluessel auf der Bahn - sie liegen oben.
            //
            // Sie sind nur da, wenn die Kamera ausgewaehlt ist; dann aber
            // haben sie Vorrang vor der Kamera selbst, weil man sie
            // gezielt trifft.
            int keyHit = -1;
            if (g_app->cameraSelected) {
                float bestKey = reach * reach;
                for (std::size_t ki = 0; ki < g_app->keyMarks.size(); ++ki) {
                    const float dx = g_app->keyMarks[ki].sx - mx;
                    const float dy = g_app->keyMarks[ki].sy - my;
                    const float d = dx * dx + dy * dy;
                    if (d < bestKey) {
                        bestKey = d;
                        keyHit = static_cast<int>(ki);
                    }
                }
            }
            // Welche FIGUR liegt unter der Maus? Aus den Marken, die das
            // Zeichnen hinterlassen hat. Die Kamera und ihre Schluessel
            // haben Vorrang - sie sind kleiner und schwerer zu treffen.
            int figHit = -1;
            {
                const float reichweite = ImGui::GetFontSize() * 1.6F;
                float bestF = reichweite * reichweite;
                for (const App::ActorMark& am : g_app->actorMarks) {
                    const float dx = am.sx - mx;
                    const float dy = am.sy - my;
                    const float d = dx * dx + dy * dy;
                    if (d < bestF) {
                        bestF = d;
                        figHit = am.index;
                    }
                }
            }

            if (keyHit >= 0) {
                g_app->selectedKey =
                    (g_app->selectedKey == keyHit) ? -1 : keyHit;
                g_app->gizmoZeitBezug = g_app->playMs;

                // BEIDE Vorschauen verwerfen. Vorher nur die Verschiebung -
                // eine angefangene Drehung wanderte zum naechsten Schluessel
                // mit und landete beim Schreiben dort.
                for (int k = 0; k < 3; ++k) {
                    g_app->gizmoOffset[k] = 0.0F;
                    g_app->gizmoAngles[k] = 0.0F;
                }
                g_app->gizmoAxis = -1;
                // Und die zugehoerige Skriptzeile mit auswaehlen - so
                // sieht man sofort, welcher Befehl gemeint ist.
                if (g_app->selectedKey >= 0) {
                    const Path& kp2 =
                        g_app->keyMarks[static_cast<std::size_t>(keyHit)].path;
                    selectByPath(kp2);
                }
            } else if (figHit >= 0) {
                // Eine FIGUR getroffen - sie anwaehlen, damit ihr Laufweg
                // erscheint. Nochmal dieselbe hebt die Auswahl auf.
                g_app->selectedActor =
                    (g_app->selectedActor == figHit) ? -1 : figHit;
                g_app->cameraSelected = false;
                g_app->gizmoShown = false;
                // Mit ihr ihr Spawner (rc597): Eigenschaften, Gizmo, Umriss.
                entityAuswahlSetzen(g_app->selectedActor >= 0 ? entityFuerFigur(g_app->selectedActor) : -1);
                g_app->mapDirty = true;
            } else if (traf) {
                // Nochmal darauf klicken hebt die Auswahl wieder auf.
                g_app->cameraSelected = !g_app->cameraSelected;
                g_app->selectedActor = -1;   // entweder Kamera oder Figur
                g_app->pickedEntity = -1;
                // Ohne Kamera kein Gizmo - sonst faengt der Ueberfahr-Test
                // weiter unsichtbare Ringe und Achsen.
                if (!g_app->cameraSelected) {
                    g_app->gizmoShown = false;
                }
                // Beim Auswaehlen gleich den naechstgelegenen Schluessel
                // mitnehmen.
                //
                // Sonst brauchte es ZWEI Klicks, bis die Achsen erschienen:
                // einen fuer die Kamera, einen fuer den Schluessel. Genau
                // so gemeldet - "das passiert erst wenn ich 2 mal mit links
                // klick drauf druecke".
                //
                // Genommen wird der, der auf dem Bildschirm am naechsten
                // liegt: man hat ja gerade dorthin geklickt.
                g_app->selectedKey = -1;
                // Der Schluessel, der zur eingestellten ZEIT gilt. Vorher der
                // auf dem Bildschirm naechstgelegene - bei einer Bahn, die an
                // sich selbst vorbeilaeuft, oft ein ganz anderer.
                if (g_app->cameraSelected) {
                    g_app->selectedKey = schluesselFuerZeit(g_app->playMs);
                    g_app->gizmoZeitBezug = g_app->playMs;
                    const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
                    if (g_app->selectedKey >= 0 &&
                        static_cast<std::size_t>(g_app->selectedKey) < ks.size()) {
                        const Path& kp2 = ks[static_cast<std::size_t>(g_app->selectedKey)].path;
                        if (!kp2.empty()) {
                            g_app->selectedPath = kp2;
                            g_app->selection.clear();
                            g_app->selection.push_back(kp2);
                            rebuildTree();
                            g_app->scrollToSelected = true;
                        }
                    }
                }
                for (int k = 0; k < 3; ++k) {
                    g_app->gizmoAngles[k] = 0.0F;
                }
                for (int k = 0; k < 3; ++k) {
                    g_app->gizmoOffset[k] = 0.0F;
                }
            } else {
                // Weder Kamera noch Schluessel noch Figur getroffen.
                //
                // Keine Marke? Dann das Modell unter der Maus - ein Schiff,
                // eine Tuer, ein Kartenmodell waehlt seine Entity (rc597).
                if (found < 0) { found = moverUnterMaus(m.x, m.y); }
                //
                // Wurde auch keine Entity getroffen (found ist -1), war das
                // ein Klick INS LEERE - und der hebt alles auf. "wenn ich
                // etwas selected habe und dann daneben klicke kann es
                // abgewaehlt werden?" Ja, und zwar alles auf einmal: es
                // ist der natuerliche Weg, eine Auswahl loszuwerden, ohne
                // sich zu merken, was gerade woran haengt.
                //
                // Das Gizmo geht mit: ohne Auswahl faengt sein
                // Ueberfahr-Test sonst weiter unsichtbare Achsen und Ringe
                // (derselbe Fall wie in rc167).
                g_app->selectedKey = -1;
                g_app->cameraSelected = false;
                // Strg/Umschalt nimmt hinzu oder heraus (rc590); ins Leere mit
                // gedrueckter Taste laesst die Auswahl stehen (dort beginnt
                // vielleicht ein Rahmen, der hinzufuegt).
                const bool zusatz = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;
                if (zusatz && found >= 0) {
                    entityAuswahlUmschalten(found);
                } else if (!zusatz) {
                    entityAuswahlSetzen(found);
                }
                if (found < 0 && !zusatz) {
                    g_app->selectedActor = -1;
                    g_app->gizmoShown = false;
                    g_app->gizmoAxis = -1;
                    for (int k = 0; k < 3; ++k) {
                        g_app->gizmoOffset[k] = 0.0F;
                        g_app->gizmoAngles[k] = 0.0F;
                    }
                }
            }
            g_app->mapDirty = true;
        }

        // --- Fadenkreuz in der Bildmitte ------------------------------
        //
        // Beim Setzen einer Kamera sieht man sonst nicht genau, wohin sie
        // zeigt: die Mitte des Bildes IST die Blickrichtung, aber ohne
        // Marke schaetzt man sie.
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 mid{topLeft.x + static_cast<float>(w) * 0.5F,
                         topLeft.y + static_cast<float>(h) * 0.5F};
        const float arm = ImGui::GetFontSize() * 0.55F;
        const float gap = arm * 0.35F;
        const ImU32 col = IM_COL32(255, 210, 60, 200);
        const ImU32 shadow = IM_COL32(0, 0, 0, 140);
        for (int pass = 0; pass < 2; ++pass) {
            // Erst schwarz mit Versatz, dann farbig - so bleibt das Kreuz
            // auf hellem wie auf dunklem Grund sichtbar.
            const float o = (pass == 0) ? 1.0F : 0.0F;
            const ImU32 c = (pass == 0) ? shadow : col;
            dl->AddLine(ImVec2{mid.x - arm + o, mid.y + o},
                        ImVec2{mid.x - gap + o, mid.y + o}, c, 1.0F);
            dl->AddLine(ImVec2{mid.x + gap + o, mid.y + o},
                        ImVec2{mid.x + arm + o, mid.y + o}, c, 1.0F);
            dl->AddLine(ImVec2{mid.x + o, mid.y - arm + o},
                        ImVec2{mid.x + o, mid.y - gap + o}, c, 1.0F);
            dl->AddLine(ImVec2{mid.x + o, mid.y + gap + o},
                        ImVec2{mid.x + o, mid.y + arm + o}, c, 1.0F);
        }

        // --- Name der Entity unter dem Zeiger -------------------------
        //
        // 977 Zielnamen sind zu viele, um sie alle einzublenden. Der unter
        // dem Zeiger genuegt - und der ist genau der, den man gerade in ein
        // SET_NAVGOAL schreiben will.
        // Seit rc586 zeigt entityUeberlagerung ein Namensschild an der Entity
        // selbst (wie Unreal) - der Tooltip nur noch, wenn das nicht greift.
        if (g_app->showEntities && !g_app->camViewActive && ImGui::IsItemHovered() && g_app->entHover < 0) {
            const ImVec2 m = ImGui::GetIO().MousePos;
            const MapEntity* best = nullptr;
            float bestDist = 18.0F;   // Fangbereich in Bildpunkten

            float fwd[3];
            float rgt[3];
            float upv[3];
            g_app->cam.forward(fwd);
            g_app->cam.right(rgt);
            g_app->cam.up(upv);
            const float focal =
                1.0F / std::tan(g_app->cam.fovY * 3.14159265F / 360.0F);
            const float halfW = static_cast<float>(w) * 0.5F;
            const float halfH = static_cast<float>(h) * 0.5F;

            for (const MapEntity& e : g_app->map.entities) {
                if (e.origin.empty() || e.targetname.empty()) {
                    continue;
                }
                float p[3] = {0, 0, 0};
                std::istringstream is(e.origin);
                is >> p[0] >> p[1] >> p[2];
                const float d[3] = {p[0] - g_app->cam.pos[0], p[1] - g_app->cam.pos[1],
                                    p[2] - g_app->cam.pos[2]};
                const float z = d[0] * fwd[0] + d[1] * fwd[1] + d[2] * fwd[2];
                if (z < 1.0F) {
                    continue;
                }
                const float sx = topLeft.x + halfW +
                                 (d[0] * rgt[0] + d[1] * rgt[1] + d[2] * rgt[2]) *
                                     focal / z * halfH;
                const float sy = topLeft.y + halfH -
                                 (d[0] * upv[0] + d[1] * upv[1] + d[2] * upv[2]) *
                                     focal / z * halfH;
                const float dist = std::sqrt((sx - m.x) * (sx - m.x) +
                                             (sy - m.y) * (sy - m.y));
                if (dist < bestDist) {
                    bestDist = dist;
                    best = &e;
                }
            }
            if (best != nullptr) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(best->targetname.c_str());
                ImGui::TextDisabled("%s", best->classname.c_str());
                ImGui::TextDisabled("%s", best->origin.c_str());
                ImGui::EndTooltip();
            }
        }
        // Paket kamera, rc601: was ausserhalb des Spielbilds liegt, zuletzt
        // abdecken - auch ueber Namen und Marken, die dort stuenden.
        spielRahmenUeberlagern(ImGui::GetWindowDrawList(), ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]},
                               ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]});
    }

    // --- Die Kamera: Hervorheben, Schild, Leiste (rc588) ------------------
    //
    // GANZ am Ende: die Leiste ist ein Kindfenster, und danach meint
    // IsItemHovered() nicht mehr das Bild. Weiter oben aufgerufen, gingen
    // Klick und Rechtsklick ins Leere (Kameratest: "Klick Move nicht gefunden").
    {
        const float bildB = g_app->kartenAnsicht[2] - g_app->kartenAnsicht[0];
        kameraVorschauZeigen();
        kameraUeberlagerung(bildB > 0.0F ? static_cast<float>(g_app->mapImage.width) / bildB : 1.0F);
        // Die Leiste Verschieben | Drehen der Entity - aus demselben Grund hier.
        entityGizmoLeiste();
    }
    platzierLeiste();
    // Draufsicht: die Schnitthoehe oben rechts (rc589).
    if (g_app->draufsicht && !g_app->geo.empty()) {
        const ImVec2 merk = ImGui::GetCursorScreenPos();
        const float fs = ImGui::GetFontSize();
        ImGui::SetCursorScreenPos(ImVec2{g_app->kartenAnsicht[2] - fs * 15.5F, g_app->kartenAnsicht[1] + fs * 0.5F});
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.10F, 0.11F, 0.14F, 0.88F});
        if (ImGui::BeginChild("##draufsicht", ImVec2{fs * 15.0F, 0.0F},
                              ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
            ImGui::SetNextItemWidth(-1.0F);
            float schnitt = g_app->draufSchnitt;
            if (ImGui::DragFloat("##schnitt", &schnitt, 4.0F, -16384.0F, 16384.0F, tr(Str::MapTopCut))) {
                const float d = schnitt - g_app->draufSchnitt;
                g_app->draufSchnitt = schnitt;
                g_app->cam.pos[2] += d;   // Abstand zur Kamera bleibt
                g_app->mapDirty = true;
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::MapTopCutHint)); }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(merk);
        ImGui::Dummy(ImVec2{0.0F, 0.0F});   // Anker: ImGui will nach dem Cursorsprung ein Element
    }
}

// Die Bedienung der Karte - UNTER allen Spalten, ueber die volle Breite.
//
// Sie stand in der Kartenspalte und musste sich deren Breite teilen. Im
// Band ist reichlich Platz, und die Ansicht darueber verliert nur die
// Hoehe, die wirklich gebraucht wird - genauso wie beim Modell.
// Die Einstellungen der Ansicht - in einer Spalte am rechten Rand.
//
// Sie standen als drei Zeilen Kaestchen UNTER dem Bild. Das kostete
// dauerhaft drei Zeilen Hoehe - genau den Platz, den die Zeitleiste
// braucht. Blender loest das mit einer Seitenleiste am rechten Rand des
// Ansichtsfensters, die sich mit N ein- und ausklappen laesst; hier
// dasselbe, mit einem Knopf statt einer Taste.
//
// Die Reihenfolge ist die alte geblieben, nur untereinander statt
// nebeneinander: wer sie gewohnt ist, findet alles am selben Platz in der
// Liste.
