// nrc_shim.h - Test-Ersatz fuer die Radiant-Schnittstellen, die
// contrib/terrain_generator/brush_builder.cpp (NetRadiant-Custom master,
// unveraendert daneben) benutzt: _QERFaceData, GlobalBrushCreator,
// GlobalEntityClassManager, GlobalEntityCreator, GlobalSceneGraph,
// Node_getTraversable, Entity_setSelected, globalErrorStream.
// Nur fuer den ctest karte_terrain: er zeichnet auf, was NRCs Erbauer anlegt,
// und vergleicht es mit dem Port (src/karte/terraingen.cpp).
//
// NetRadiant-Custom: GNU General Public License, Version 2 (LICENSE).
#ifndef NRC_SHIM_H
#define NRC_SHIM_H

#include <algorithm>
#include <cmath>
#include <deque>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

struct DoubleVector3 {
    double v[3] = {0, 0, 0};
    double& operator[](int i) { return v[i]; }
    const double& operator[](int i) const { return v[i]; }
};
struct texdef_t {   // include/itexdef.h
    float shift[2] = {0, 0};
    float rotate = 0;
    float scale[2] = {1, 1};
};
class _QERFaceData {   // include/ibrush.h
public:
    DoubleVector3 m_p0, m_p1, m_p2;
    texdef_t m_texdef;
    const char* m_shader = "";
    int contents = 0;
    int flags = 0;
    int value = 0;
};

namespace scene {
struct Node {
    bool entity = false;
    bool root = false;
    std::vector<Node*> kinder;
    std::vector<_QERFaceData> faces;
    std::vector<std::string> shader;   // m_shader kopiert (const char* lebt nicht)
};
struct Instance {
    Node* node = nullptr;
};
class Path {
public:
    explicit Path(Node& n) { m_v.push_back(&n); }
    void push(Node& n) { m_v.push_back(&n); }
    Node& top() const { return *m_v.back(); }
private:
    std::vector<Node*> m_v;
};
}  // namespace scene

template <typename T> T& makeReference(T& t) { return t; }

struct NrcAufzeichnung {
    std::deque<scene::Node> knoten;
    scene::Node wurzel;
    std::vector<scene::Node*> gewaehlt;
    std::ostringstream fehler;
    void leeren() {
        knoten.clear();
        wurzel = scene::Node{};
        wurzel.root = true;
        gewaehlt.clear();
        fehler.str("");
    }
};
inline NrcAufzeichnung& nrcAufzeichnung() {
    static NrcAufzeichnung a;
    return a;
}

class NodeSmartReference {
public:
    explicit NodeSmartReference(scene::Node& n) : m_n(&n) {}
    operator scene::Node&() const { return *m_n; }
private:
    scene::Node* m_n;
};

struct Traversable {
    scene::Node* eltern;
    void insert(scene::Node& n) { eltern->kinder.push_back(&n); }
};
inline Traversable* Node_getTraversable(scene::Node& n) {
    static Traversable t;
    t.eltern = &n;
    return &t;
}

struct BrushCreatorShim {
    scene::Node& createBrush() {
        nrcAufzeichnung().knoten.emplace_back();
        return nrcAufzeichnung().knoten.back();
    }
    bool Brush_addFace(scene::Node& brush, const _QERFaceData& f) {
        brush.faces.push_back(f);
        brush.shader.emplace_back(f.m_shader);
        return true;
    }
};
inline BrushCreatorShim& GlobalBrushCreator() {
    static BrushCreatorShim b;
    return b;
}
struct EntityClass {};
struct EntityClassManagerShim {
    EntityClass* findOrInsert(const char*, bool) {
        static EntityClass e;
        return &e;
    }
};
inline EntityClassManagerShim& GlobalEntityClassManager() {
    static EntityClassManagerShim m;
    return m;
}
struct EntityCreatorShim {
    scene::Node& createEntity(EntityClass*) {
        nrcAufzeichnung().knoten.emplace_back();
        nrcAufzeichnung().knoten.back().entity = true;
        return nrcAufzeichnung().knoten.back();
    }
};
inline EntityCreatorShim& GlobalEntityCreator() {
    static EntityCreatorShim c;
    return c;
}
struct SceneGraphShim {
    scene::Node& root() { return nrcAufzeichnung().wurzel; }
    scene::Instance* find(const scene::Path& p) {
        static scene::Instance i;
        i.node = &p.top();
        return &i;
    }
};
inline SceneGraphShim& GlobalSceneGraph() {
    static SceneGraphShim s;
    return s;
}
inline void Entity_setSelected(scene::Instance& i, bool) { nrcAufzeichnung().gewaehlt.push_back(i.node); }
inline void SceneChangeNotify() {}
inline std::ostream& globalErrorStream() { return nrcAufzeichnung().fehler; }

#endif
