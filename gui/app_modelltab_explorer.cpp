// app_modelltab_explorer.cpp - AP2 (MBENTWURF Teil B): Szenen-Explorer (wie 3ds Max) / Outliner (wie Blender)
//
// EIN Baum fuer alle Bereiche des Model-Tabs (shank 04.11.: "untereinander als EIN Baum"): oben das Modell, darunter
// Flaechen mit ihrer Hierarchie, Tags mit ihren Anbauten, Knochen, die Griffe des Control Rigs und die Sequenzen der
// animation.cfg. Umschaltbar "Nach Hierarchie / Nach Ebenen" (Blender: "Hierarchie / Sammlungen") - die Ebenen Koerper,
// Kappen, Tags, Knochen, Anbauten und die weiteren Detailstufen tragen Auge und Schneeflocke.
//
// Aufbau der Zeile (shank 10.11.: "wie die augen und das aufklappen angeordnet ist, sollte wie bei 3ds max sein"):
//   wie 3ds Max   [Pfeil][Auge][Symbol] Name ...            | Spalte "Frozen" (Schneeflocke); Spaltenkoepfe, in der Breite
//                 ziehbar, Klick sortiert. Das Auge steht links an der Zeile, eingerueckt mit ihr (Autodesk-Hilfe, Scene
//                 Explorer / Layer Explorer: "click the icon at the left side of any item to hide or unhide", "in the
//                 Frozen column, click the icon of any item to freeze or unfreeze", "sort ... by clicking a column
//                 heading" - help.autodesk.com/cloudhelp/2017/ENU/3DSMax/files/GUID-8E194B5E-221E-4896-93F8-18072DC6EB82.htm)
//   wie Blender   [Pfeil][Symbol] Name ...                  | Auge | Pfeil "waehlbar" rechts in festen Spalten, keine Koepfe,
//                 Suche und Filter-Ausklapper oben (Blender-Handbuch, Outliner > Interface: Restriction Columns "Visibility
//                 (eye icon)", "Selectability (mouse cursor icon)", Filter "Sort Alphabetically" -
//                 docs.blender.org/manual/en/4.0/editors/outliner/interface.html)
// Der Einzug je Ebene ist eine Zeichenhoehe (Entwurf: 13 px bei 13 px Schrift) und staucht sich bis auf 0,45, wenn der
// tiefste offene Knoten sonst keinen Platz fuer seinen Namen haette - der Baum rollt nie waagerecht; ein Name, der nicht
// passt, endet mit "..." und steht ganz im Tooltip.
//
// Auge = nur in der Ansicht ausblenden (Spiel unveraendert), Schneeflocke = einfrieren: sichtbar, aber in Baum und Ansicht
// nicht waehlbar (flaecheUnter / flaechenImRahmen in gui/app_modelltab_ansicht.cpp fragen modellFlaecheWaehlbar; ENTSCHEID1: ein
// Klick in der Ansicht geht durch eine eingefrorene Flaeche hindurch auf die dahinter).
// Gezeichnet wird ein flacher Zeilenvorrat mit ImGuiListClipper: _humanoid hat ueber 1800 Sequenzen, gebaut werden nur
// die sichtbaren Zeilen.
#include "app_internal.h"
#include "ui_menue.h"   // KLEIN2: ui::menue/popup - Menues bleiben im Fenster
#include "textumbruch.h"
#include "tour.h"

#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }
ModellExplorerZustand g_z;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool passt(const std::string& name, const std::string& such) { return such.empty() || klein(name).find(such) != std::string::npos; }

bool reiterKnopf(const char* text, bool gewaehlt) {
    if (gewaehlt) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
    const bool k = ImGui::Button(text);
    if (gewaehlt) { ImGui::PopStyleColor(); }
    return k;
}

// Der Tooltip des letzten Elements: nach der ueblichen Verzoegerung und umbrochen (SetItemTooltip schreibt eine einzige Zeile -
// die Hinweise der Umschalter waren damit 40 Zeichenhoehen breit).
void tipp(const char* text) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) || !ImGui::BeginTooltip()) { return; }
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 22.0F);
    cjkTextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

constexpr int kEbenenFest = 5;    // Koerper, Kappen, Tags, Knochen, Anbauten; danach die Detailstufen ab 2
constexpr int kFilterZahl = static_cast<int>(ExplorerFilter::Zahl);

// Eine Zeile des flachen Baums.
struct Zeile {
    ExplorerArt art = ExplorerArt::Modell;
    int index = -1;       // Teil, Knochen, Sequenz, Griff, Ebene, Anbau; Hinweis: welcher Text
    int tiefe = 0;
    int ebene = -1;       // die Ebene, in der die Zeile steht (eine Flaeche steht auch in ihrer LOD-Ebene: eigene Kennung)
    bool kinder = false;
    bool offen = false;
};

std::vector<Zeile> g_zeilen;                  // der Baum dieses Bildes
std::unordered_map<int, bool> g_offen;        // auf/zu je Knoten (fehlt: Vorgabe)
bool g_zeigen = false;                        // modellExplorerZeigen: diese Zeile ins Bild rollen
ExplorerArt g_zeigeArt = ExplorerArt::Modell;
int g_zeigeIndex = -1;
bool g_sucheFokus = false;                    // H: in das Suchfeld
std::string g_anbauZiel;                      // offenes Anbau-Fenster: Tag oder Knochen
bool g_anbauKnochen = false;
bool g_anbauOeffnen = false;
bool g_infoOeffnen = false;
int g_anker = -1;                             // zuletzt angeklickte Flaeche (Umschalt+Klick: Bereich)
std::string g_frostModell;                    // das Modell, fuer das frostTeil gilt
float g_nameBreite = 0.0F;                    // Breite der Namensspalte im letzten Bild

int schluessel(ExplorerArt a, int index, int ebene) { return static_cast<int>(a) * 4000000 + (ebene + 1) * 200000 + index + 1; }

bool istOffen(ExplorerArt a, int index, bool vorgabe) {
    const auto it = g_offen.find(schluessel(a, index, -1));
    return it != g_offen.end() ? it->second : vorgabe;
}

void setzeOffen(ExplorerArt a, int index, bool offen) { g_offen[schluessel(a, index, -1)] = offen; }

bool istKappe(const modell::Teil& x) { return !x.istTag() && (x.flaggen & modell::kG2Aus) != 0U; }

// Gruppen, auf die das Auge und die Schneeflocke einer Ebene oder eines Kopfs wirken.
enum class Gruppe { Koerper, Kappen, Tags, Flaechen, Knochen, Anbauten, Griffe, Keine };

bool teilIn(const modell::Teil& x, Gruppe g) {
    switch (g) {
        case Gruppe::Koerper: return !x.istTag() && !istKappe(x);
        case Gruppe::Kappen: return istKappe(x);
        case Gruppe::Tags: return x.istTag();
        case Gruppe::Flaechen: return !x.istTag();
        default: return false;
    }
}

// Die Listen auf die Groesse von Modell und Skelett; bei einem neuen Modell: Kappen eingefroren, Knoten auf Vorgabe.
void groessen() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    t.verstecktTeil.resize(in.teile.size(), 0);
    t.verstecktKnochen.resize(g_app->anim.bones.size(), 0);
    if (g_frostModell != t.dokModell) {
        g_frostModell = t.dokModell;
        g_z.frostTeil.assign(in.teile.size(), 0);
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (istKappe(in.teile[i])) { g_z.frostTeil[i] = 1; }   // Entwurf: "Kappen ab Werk eingefroren"
        }
        g_z.frostKnochen.assign(g_app->anim.bones.size(), 0);
        g_z.frostAnbauten = false;
        g_offen.clear();
        g_anker = -1;
    }
    g_z.frostTeil.resize(in.teile.size(), 0);
    g_z.frostKnochen.resize(g_app->anim.bones.size(), 0);
}

void versteckGeaendert() {
    ++mt().verstecktStand;
    g_app->modelDirty = true;
}

// Ein Teil und alle seine Kinder (Ghoul2-Hierarchie) auf `wert`.
void teilSetzen(std::vector<char>& liste, int teil, char wert, bool samtKindern, int tiefe) {
    const modell::Inhalt& in = mt().dok.inhalt();
    if (teil < 0 || static_cast<std::size_t>(teil) >= in.teile.size() || static_cast<std::size_t>(teil) >= liste.size() || tiefe > static_cast<int>(in.teile.size())) { return; }
    liste[static_cast<std::size_t>(teil)] = wert;
    if (!samtKindern) { return; }
    for (const int k : in.teile[static_cast<std::size_t>(teil)].kinder) { teilSetzen(liste, k, wert, true, tiefe + 1); }
}

// Ein Knochen (und sein Teilbaum) auf `wert`.
void knochenSetzen(std::vector<char>& liste, int knochen, char wert, bool samtKindern) {
    const GlaAnimation& a = g_app->anim;
    for (std::size_t b = 0; b < a.bones.size() && b < liste.size(); ++b) {
        int p = static_cast<int>(b);
        for (int schritt = 0; p >= 0 && schritt < 256; ++schritt) {
            if (p == knochen) {
                liste[b] = wert;
                break;
            }
            if (!samtKindern) { break; }
            p = a.bones[static_cast<std::size_t>(p)].parent;
            if (p >= 0 && static_cast<std::size_t>(p) >= a.bones.size()) { break; }
        }
    }
}

struct GruppenStand {
    int zahl = 0;
    int versteckt = 0;
    int frost = 0;
};

GruppenStand gruppenStand(Gruppe g) {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    GruppenStand s;
    if (g == Gruppe::Knochen) {
        s.zahl = static_cast<int>(g_app->anim.bones.size());
        for (int b = 0; b < s.zahl; ++b) {
            s.versteckt += modellKnochenVersteckt(b) ? 1 : 0;
            s.frost += modellKnochenEingefroren(b) ? 1 : 0;
        }
    } else if (g == Gruppe::Anbauten) {
        s.zahl = static_cast<int>(t.anbauten.size());
        for (const ModellAnbau& a : t.anbauten) { s.versteckt += a.versteckt ? 1 : 0; }
        s.frost = g_z.frostAnbauten ? s.zahl : 0;
    } else if (g == Gruppe::Griffe) {
        s.zahl = modellRigExplorerZahl();
        for (std::size_t i = 0; i < t.griffVersteckt.size() && static_cast<int>(i) < s.zahl; ++i) { s.versteckt += t.griffVersteckt[i] != 0 ? 1 : 0; }
    } else {
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (!teilIn(in.teile[i], g)) { continue; }
            ++s.zahl;
            s.versteckt += modellTeilVersteckt(static_cast<int>(i)) ? 1 : 0;
            s.frost += modellTeilEingefroren(static_cast<int>(i)) ? 1 : 0;
        }
    }
    return s;
}

void gruppeAuge(Gruppe g, bool zeigen) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    groessen();
    const char wert = zeigen ? 0 : 1;
    if (g == Gruppe::Knochen) {
        std::fill(t.verstecktKnochen.begin(), t.verstecktKnochen.end(), wert);
    } else if (g == Gruppe::Anbauten) {
        for (ModellAnbau& a : t.anbauten) { a.versteckt = !zeigen; }
    } else if (g == Gruppe::Griffe) {
        t.griffVersteckt.assign(static_cast<std::size_t>(modellRigExplorerZahl()), wert);
    } else {
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (teilIn(in.teile[i], g)) { t.verstecktTeil[i] = wert; }
        }
    }
    versteckGeaendert();
}

// Was eingefroren wird, ist nicht mehr gewaehlt (wie 3ds Max).
void auswahlOhneFrost() {
    ModellTab& t = mt();
    t.auswahl.erase(std::remove_if(t.auswahl.begin(), t.auswahl.end(), [](int i) { return modellTeilEingefroren(i); }), t.auswahl.end());
    if (modellTeilEingefroren(t.tagWahl)) { t.tagWahl = -1; }
    if (modellKnochenEingefroren(t.knochenWahl)) { t.knochenWahl = -1; }
    g_app->modelDirty = true;
}

void gruppeFrost(Gruppe g, bool an) {
    const modell::Inhalt& in = mt().dok.inhalt();
    groessen();
    const char wert = an ? 1 : 0;
    if (g == Gruppe::Knochen) {
        std::fill(g_z.frostKnochen.begin(), g_z.frostKnochen.end(), wert);
    } else if (g == Gruppe::Anbauten) {
        g_z.frostAnbauten = an;
    } else {
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (teilIn(in.teile[i], g)) { g_z.frostTeil[i] = wert; }
        }
    }
    auswahlOhneFrost();
}

Gruppe gruppeVon(ExplorerEbene e) {
    switch (e) {
        case ExplorerEbene::Koerper: return Gruppe::Koerper;
        case ExplorerEbene::Kappen: return Gruppe::Kappen;
        case ExplorerEbene::Tags: return Gruppe::Tags;
        case ExplorerEbene::Knochen: return Gruppe::Knochen;
        case ExplorerEbene::Anbauten: return Gruppe::Anbauten;
        default: return Gruppe::Keine;
    }
}

void meldeFrost(const std::string& name) {
    char z[320];
    std::snprintf(z, sizeof z, tr(modellBlender() ? Str::MbXNichtWaehlbarMeldung : Str::MbXEingefrorenMeldung), name.c_str());
    mt().meldung = z;
    addStatus(z);
}

// --- Sequenzen: welche Zeilen Gruppe, Suche und Sortierung durchlassen (nur neu bestimmt, wenn sich etwas aendert) ---------------

const char* const kSeqPraefix[] = {"", "BOTH_", "TORSO_", "LEGS_", "FACE_"};

const std::vector<int>& sequenzZeilen(const std::string& such) {
    static std::vector<int> zeilen;
    static const AnimEntry* liste = nullptr;
    static std::size_t anzahl = 0;
    static int gruppe = -1;
    static int sortierung = -1;
    static std::string suche;
    static int reiterStand = -1;   // AP11: ein anderer Reiter hat eine andere Liste (auch an derselben Adresse)
    const int sort = g_z.sortierung == 1 || g_z.sortierung == 2 ? g_z.sortierung : 0;
    if (liste == g_app->animList.data() && anzahl == g_app->animList.size() && gruppe == g_z.seqGruppe && suche == such && sortierung == sort &&
        reiterStand == modellReiterStand()) {
        return zeilen;
    }
    reiterStand = modellReiterStand();
    liste = g_app->animList.data();
    anzahl = g_app->animList.size();
    gruppe = g_z.seqGruppe;
    suche = such;
    sortierung = sort;
    zeilen.clear();
    const char* praefix = kSeqPraefix[std::clamp(gruppe, 0, 4)];
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        const std::string& name = g_app->animList[i].name;
        if (gruppe > 0 && name.rfind(praefix, 0) != 0) { continue; }
        if (!such.empty() && klein(name).find(such) == std::string::npos) { continue; }
        zeilen.push_back(static_cast<int>(i));
    }
    if (sort != 0) {
        std::stable_sort(zeilen.begin(), zeilen.end(), [sort](int a, int b) {
            const int c = g_app->animList[static_cast<std::size_t>(a)].name.compare(g_app->animList[static_cast<std::size_t>(b)].name);
            return sort == 1 ? c < 0 : c > 0;
        });
    }
    return zeilen;
}

// --- Den Baum bauen ----------------------------------------------------------------------------------------------------

int knochenTiefe(int b) {
    const GlaAnimation& a = g_app->anim;
    int d = 0;
    for (int p = a.bones[static_cast<std::size_t>(b)].parent; p >= 0 && static_cast<std::size_t>(p) < a.bones.size() && d < 256; p = a.bones[static_cast<std::size_t>(p)].parent) { ++d; }
    return d;
}

// Geschwister nach der gewaehlten Sortierung (0: Reihenfolge der Datei).
template <typename Name, typename Frost>
void sortiere(std::vector<int>& v, Name name, Frost frost) {
    if (g_z.sortierung == 1 || g_z.sortierung == 2) {
        const bool auf = g_z.sortierung == 1;
        std::stable_sort(v.begin(), v.end(), [&](int a, int b) {
            const int c = klein(name(a)).compare(klein(name(b)));
            return auf ? c < 0 : c > 0;
        });
    } else if (g_z.sortierung == 3) {
        std::stable_sort(v.begin(), v.end(), [&](int a, int b) { return frost(a) && !frost(b); });
    }
}

struct Bauer {
    std::vector<Zeile>& z;
    const std::string& such;
    const modell::Inhalt& in;
    std::vector<std::vector<int>> knochenKinder;

    void zeile(ExplorerArt art, int index, int tiefe, int ebene, bool kinder, bool offen) const {
        Zeile r;
        r.art = art;
        r.index = index;
        r.tiefe = tiefe;
        r.ebene = ebene;
        r.kinder = kinder;
        r.offen = offen;
        z.push_back(r);
    }

    [[nodiscard]] bool flaecheErlaubt(int i) const {
        const modell::Teil& x = in.teile[static_cast<std::size_t>(i)];
        if (x.istTag()) { return false; }
        return !g_z.filterAus[static_cast<int>(istKappe(x) ? ExplorerFilter::Kappen : ExplorerFilter::Flaechen)];
    }

    void sortiereTeile(std::vector<int>& v) const {
        sortiere(v, [this](int i) { return in.teile[static_cast<std::size_t>(i)].name; }, [](int i) { return modellTeilEingefroren(i); });
    }

    void anbauten(const std::string& ziel, int tiefe, int ebene) const {
        if (g_z.filterAus[static_cast<int>(ExplorerFilter::Anbauten)]) { return; }
        const auto& l = mt().anbauten;
        for (std::size_t k = 0; k < l.size(); ++k) {
            if (l[k].ziel == ziel) { zeile(ExplorerArt::Anbau, static_cast<int>(k), tiefe, ebene, false, false); }
        }
    }

    // Eine Flaeche mit ihren Kindern; eine Zeile, die der Filter weglaesst, reicht ihre Kinder auf derselben Ebene durch.
    void flaeche(int i, int tiefe, int schutz) const {
        if (schutz > static_cast<int>(in.teile.size())) { return; }
        const modell::Teil& x = in.teile[static_cast<std::size_t>(i)];
        if (x.istTag()) { return; }
        std::vector<int> unter;
        for (const int k : x.kinder) {
            if (!in.teile[static_cast<std::size_t>(k)].istTag()) { unter.push_back(k); }
        }
        sortiereTeile(unter);
        if (!flaecheErlaubt(i)) {
            for (const int k : unter) { flaeche(k, tiefe, schutz + 1); }
            return;
        }
        bool hatKind = false;
        for (const int k : unter) { hatKind = hatKind || flaecheErlaubt(k) || !in.teile[static_cast<std::size_t>(k)].kinder.empty(); }
        const bool offen = hatKind && istOffen(ExplorerArt::Flaeche, i, true);
        zeile(ExplorerArt::Flaeche, i, tiefe, -1, hatKind, offen);
        if (offen) {
            for (const int k : unter) { flaeche(k, tiefe + 1, schutz + 1); }
        }
    }

    void flaechenFlach(Gruppe g, int tiefe, int ebene) const {
        std::vector<int> v;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (teilIn(in.teile[i], g) && flaecheErlaubt(static_cast<int>(i)) && passt(in.teile[i].name, such)) { v.push_back(static_cast<int>(i)); }
        }
        sortiereTeile(v);
        for (const int i : v) { zeile(ExplorerArt::Flaeche, i, tiefe, ebene, false, false); }
    }

    void tags(int tiefe, int ebene) const {
        std::vector<int> v;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (in.teile[i].istTag() && passt(in.teile[i].name, such)) { v.push_back(static_cast<int>(i)); }
        }
        sortiereTeile(v);
        for (const int i : v) {
            zeile(ExplorerArt::Tag, i, tiefe, ebene, false, false);
            anbauten(in.teile[static_cast<std::size_t>(i)].name, tiefe + 1, ebene);
        }
    }

    void knochen(int b, int tiefe, int schutz) const {
        const GlaAnimation& a = g_app->anim;
        if (schutz > static_cast<int>(a.bones.size())) { return; }
        std::vector<int> unter = knochenKinder[static_cast<std::size_t>(b)];
        sortiere(unter, [&a](int i) { return a.bones[static_cast<std::size_t>(i)].name; }, [](int i) { return modellKnochenEingefroren(i); });
        const bool flach = g_z.knochenFlach || !such.empty();
        if (!such.empty() && !passt(a.bones[static_cast<std::size_t>(b)].name, such)) {
            for (const int c : unter) { knochen(c, tiefe, schutz + 1); }
            return;
        }
        const bool hatKind = !flach && !unter.empty();
        const bool offen = hatKind && istOffen(ExplorerArt::Knochen, b, knochenTiefe(b) < 3);
        zeile(ExplorerArt::Knochen, b, tiefe, -1, hatKind, offen);
        anbauten(a.bones[static_cast<std::size_t>(b)].name, tiefe + 1, -1);
        if (flach || offen) {
            for (const int c : unter) { knochen(c, flach ? tiefe : tiefe + 1, schutz + 1); }
        }
    }

    void knochenAlle(int tiefe) const {
        const GlaAnimation& a = g_app->anim;
        std::vector<int> wurzeln;
        for (std::size_t b = 0; b < a.bones.size(); ++b) {
            const int p = a.bones[b].parent;
            if (p < 0 || static_cast<std::size_t>(p) >= a.bones.size() || static_cast<std::size_t>(p) == b) { wurzeln.push_back(static_cast<int>(b)); }
        }
        for (const int b : wurzeln) { knochen(b, tiefe, 0); }
    }

    void rig(int tiefe) const {
        const int n = modellRigExplorerZahl();
        if (n <= 0 || g_z.filterAus[static_cast<int>(ExplorerFilter::Griffe)]) { return; }
        const bool offen = istOffen(ExplorerArt::KopfRig, 0, true);
        zeile(ExplorerArt::KopfRig, 0, tiefe, -1, true, offen);
        if (!offen) { return; }
        for (int i = 0; i < n; ++i) {
            if (passt(modellRigExplorerName(i), such)) { zeile(ExplorerArt::Griff, i, tiefe + 1, -1, false, false); }
        }
    }

    void sequenzen(int tiefe) const {
        if (g_z.filterAus[static_cast<int>(ExplorerFilter::Sequenzen)]) { return; }
        if (g_app->animList.empty()) {   // "This model has no animations" (stand vorher in der eigenen Liste)
            zeile(ExplorerArt::Hinweis, -4, tiefe, -1, false, false);
            return;
        }
        const bool offen = istOffen(ExplorerArt::KopfSequenzen, 0, true);
        zeile(ExplorerArt::KopfSequenzen, 0, tiefe, -1, true, offen);
        if (!offen) { return; }
        zeile(ExplorerArt::SeqFilter, 0, tiefe + 1, -1, false, false);
        const std::vector<int>& l = sequenzZeilen(such);
        for (const int i : l) { zeile(ExplorerArt::Sequenz, i, tiefe + 1, -1, false, false); }
        if (l.empty()) { zeile(ExplorerArt::Hinweis, -2, tiefe + 1, -1, false, false); }
    }

    void nachHierarchie() const {
        const bool offen = istOffen(ExplorerArt::Modell, 0, true);
        zeile(ExplorerArt::Modell, 0, 0, -1, true, offen);
        if (!offen) { return; }
        const bool mitFlaechen = !g_z.filterAus[static_cast<int>(ExplorerFilter::Flaechen)];
        const bool mitKappen = !g_z.filterAus[static_cast<int>(ExplorerFilter::Kappen)];
        if (mitFlaechen || mitKappen) {
            const bool auf = istOffen(ExplorerArt::KopfFlaechen, 0, true);
            zeile(ExplorerArt::KopfFlaechen, 0, 1, -1, true, auf);
            if (auf && !such.empty()) {
                flaechenFlach(Gruppe::Flaechen, 2, -1);
            } else if (auf) {
                std::vector<int> wurzeln;
                for (std::size_t i = 0; i < in.teile.size(); ++i) {
                    if (in.teile[i].eltern < 0 && !in.teile[i].istTag()) { wurzeln.push_back(static_cast<int>(i)); }
                }
                sortiereTeile(wurzeln);
                for (const int i : wurzeln) { flaeche(i, 2, 0); }
            }
        }
        if (!g_z.filterAus[static_cast<int>(ExplorerFilter::Tags)]) {
            const bool auf = istOffen(ExplorerArt::KopfTags, 0, true);
            zeile(ExplorerArt::KopfTags, 0, 1, -1, true, auf);
            if (auf) { tags(2, -1); }
        }
        if (!g_app->anim.bones.empty() && !g_z.filterAus[static_cast<int>(ExplorerFilter::Knochen)]) {
            const bool auf = istOffen(ExplorerArt::KopfKnochen, 0, false) || !such.empty();
            zeile(ExplorerArt::KopfKnochen, 0, 1, -1, true, auf);
            if (auf) { knochenAlle(2); }
        }
        rig(1);
        sequenzen(1);
    }

    void nachEbenen() const {
        const int ebenen = kEbenenFest + std::max(2, in.stufenZahl - 1);
        for (int e = 0; e < ebenen; ++e) {
            static const ExplorerFilter kFilter[kEbenenFest] = {ExplorerFilter::Flaechen, ExplorerFilter::Kappen, ExplorerFilter::Tags, ExplorerFilter::Knochen,
                                                                ExplorerFilter::Anbauten};
            if (g_z.filterAus[static_cast<int>(e < kEbenenFest ? kFilter[e] : ExplorerFilter::Flaechen)]) { continue; }
            if (e == static_cast<int>(ExplorerEbene::Knochen) && g_app->anim.bones.empty()) { continue; }
            const bool offen = istOffen(ExplorerArt::Ebene, e, e < kEbenenFest);
            zeile(ExplorerArt::Ebene, e, 0, -1, true, offen);
            if (!offen) { continue; }
            const std::size_t vorher = z.size();
            switch (e) {
                case static_cast<int>(ExplorerEbene::Koerper): flaechenFlach(Gruppe::Koerper, 1, e); break;
                case static_cast<int>(ExplorerEbene::Kappen): flaechenFlach(Gruppe::Kappen, 1, e); break;
                case static_cast<int>(ExplorerEbene::Tags): tags(1, e); break;
                case static_cast<int>(ExplorerEbene::Knochen): knochenAlle(1); break;
                case static_cast<int>(ExplorerEbene::Anbauten):
                    for (std::size_t k = 0; k < mt().anbauten.size(); ++k) { zeile(ExplorerArt::Anbau, static_cast<int>(k), 1, e, false, false); }
                    break;
                default: {
                    const int stufe = e - kEbenenFest + 2;   // Detailstufe 2, 3, ...
                    if (stufe > in.stufenZahl) {
                        zeile(ExplorerArt::Hinweis, stufe, 1, e, false, false);
                    } else {
                        flaechenFlach(Gruppe::Flaechen, 1, e);
                    }
                    break;
                }
            }
            if (z.size() == vorher) { zeile(ExplorerArt::Hinweis, 0, 1, e, false, false); }
        }
        rig(0);
        sequenzen(0);
    }
};

void baueZeilen(const std::string& such) {
    g_zeilen.clear();
    const modell::Inhalt& in = mt().dok.inhalt();
    Bauer b{g_zeilen, such, in, {}};
    b.knochenKinder.resize(g_app->anim.bones.size());
    for (std::size_t k = 0; k < g_app->anim.bones.size(); ++k) {
        const int p = g_app->anim.bones[k].parent;
        if (p >= 0 && static_cast<std::size_t>(p) < b.knochenKinder.size() && static_cast<std::size_t>(p) != k) { b.knochenKinder[static_cast<std::size_t>(p)].push_back(static_cast<int>(k)); }
    }
    if (g_z.art == 0) {
        b.nachHierarchie();
    } else {
        b.nachEbenen();
    }
    if (!such.empty()) {   // die Suche findet nichts: sagen statt leer lassen
        bool treffer = false;
        for (const Zeile& r : g_zeilen) {
            treffer = treffer || r.art == ExplorerArt::Flaeche || r.art == ExplorerArt::Tag || r.art == ExplorerArt::Knochen || r.art == ExplorerArt::Griff ||
                      r.art == ExplorerArt::Sequenz || r.art == ExplorerArt::Anbau;
        }
        if (!treffer) { b.zeile(ExplorerArt::Hinweis, -3, 0, -1, false, false); }
    }
}

// --- Texte ----------------------------------------------------------------------------------------------------------------

// "kyle/model.glm": Ordner und Datei wie im Entwurf.
std::string modellName() {
    const std::string& p = g_app->modelPath;
    const std::size_t a = p.find_last_of("/\\");
    if (a == std::string::npos || a == 0) { return p; }
    const std::size_t b = p.find_last_of("/\\", a - 1);
    return b == std::string::npos ? p : p.substr(b + 1);
}

// Der Name der GLA: die geladene, sonst die des Modells (ueber das Modell geladen traegt GlaAnimation keinen Namen).
std::string glaName() {
    const std::string& n = g_app->anim.name.empty() ? mt().dok.inhalt().glaPfad : g_app->anim.name;
    std::string f = fileName(n);
    if (!f.empty() && f.find('.') == std::string::npos) { f += ".gla"; }
    return f;
}

const char* ebenenName(int e) {
    switch (e) {
        case static_cast<int>(ExplorerEbene::Koerper): return tr(Str::MbXEbeneKoerper);
        case static_cast<int>(ExplorerEbene::Kappen): return tr(Str::MbEbeneKappen);
        case static_cast<int>(ExplorerEbene::Tags): return tr(Str::MbXArtTags);
        case static_cast<int>(ExplorerEbene::Knochen): return tr(Str::MbXArtKnochen);
        case static_cast<int>(ExplorerEbene::Anbauten): return tr(Str::MbXEbeneAnbauten);
        default: return "";
    }
}

// Der Status einer Flaeche im Spiel: 0 an, 1 aus, 2 aus samt Kindern (G2SURFACEFLAG_OFF / _NODESCENDANTS).
int flaechenStatus(const modell::G2Zustand& g2, int i) {
    const std::uint32_t f = modell::offFlags(mt().dok.inhalt(), g2, i);
    if ((f & modell::kG2Aus) == 0U) { return 0; }
    return (f & modell::kG2OhneKinder) != 0U ? 2 : 1;
}

std::string anbautenAn(const std::string& ziel) {
    std::string s;
    for (const ModellAnbau& a : mt().anbauten) {
        if (a.ziel != ziel) { continue; }
        if (!s.empty()) { s += ", "; }
        s += fileName(a.pfad);
    }
    return s.empty() ? std::string(tr(Str::MbXInfoKeine)) : s;
}

void infoZeile(std::string& s, Str was, const std::string& wert) { s += std::string(tr(was)) + ": " + wert + "\n"; }

std::string infoFlaeche(int i) {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    const modell::Teil& x = in.teile[static_cast<std::size_t>(i)];
    std::string s;
    const std::size_t stufe = std::min(static_cast<std::size_t>(std::max(t.lodJetzt, 0)), x.stufen.empty() ? 0U : x.stufen.size() - 1U);
    if (!x.stufen.empty()) {
        const modell::Netz& n = x.stufen[stufe];
        bool bezug[256] = {};
        int bezuege = 0;
        for (const modell::Ecke& e : n.ecken) {
            for (int k = 0; k < 4; ++k) {
                if (e.gewicht[k] > 0.0F && !bezug[e.knochen[k]]) {
                    bezug[e.knochen[k]] = true;
                    ++bezuege;
                }
            }
        }
        infoZeile(s, Str::MbXInfoEcken, std::to_string(n.ecken.size()));
        infoZeile(s, Str::MbXInfoDreiecke, std::to_string(n.dreiecke()));
        infoZeile(s, Str::MbXInfoKnochenbez, std::to_string(bezuege));
    }
    // Der Shader: die Zeile der Haut (.skin), sonst der Name aus der Hierarchie der Datei (oft leer)
    const std::string haut = static_cast<std::size_t>(i) < g_app->model.surfaces.size() ? g_app->model.surfaces[static_cast<std::size_t>(i)].texture : std::string();
    infoZeile(s, Str::MbXInfoShader, !haut.empty() ? haut : (x.shader.empty() ? std::string("-") : x.shader));
    infoZeile(s, Str::MbXInfoEltern, x.eltern >= 0 ? in.teile[static_cast<std::size_t>(x.eltern)].name : std::string("-"));
    static const Str kStatus[3] = {Str::MbFlAn, Str::MbFlAus, Str::MbFlAusOhneKinder};
    std::string status = tr(kStatus[flaechenStatus(t.g2, i)]);
    if (t.g2.wurzel == i && i != 0) { status += std::string(" \xc2\xb7 ") + tr(Str::MbWurzel); }
    infoZeile(s, Str::MbXInfoStatus, status);
    return s;
}

// Uebersicht im Kopf der Flaechen: wie viele das Spiel zeichnet und welche aus sind (vorher die Gruppen "Drawn" / "Off" des
// Umschalters "By layers"). Kappen zaehlen nicht mit - sie sind in der Datei aus.
std::string infoFlaechenKopf() {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    int an = 0;
    int aus = 0;
    std::string namen;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].istTag() || istKappe(in.teile[i])) { continue; }
        if (i < t.gezeichnet.size() && t.gezeichnet[i] != 0) {
            ++an;
            continue;
        }
        ++aus;
        if (aus <= 12) { namen += (namen.empty() ? std::string() : std::string(", ")) + in.teile[i].name; }
    }
    std::string s;
    infoZeile(s, Str::MbEbeneGezeichnet, std::to_string(an));
    infoZeile(s, Str::MbEbeneAus, std::to_string(aus) + (namen.empty() ? std::string() : " (" + namen + (aus > 12 ? ", ..." : "") + ")"));
    return s;
}

std::string infoTag(int i) {
    const modell::Inhalt& in = mt().dok.inhalt();
    const modell::Teil& x = in.teile[static_cast<std::size_t>(i)];
    std::string s;
    if (!x.stufen.empty() && !x.stufen[0].ecken.empty()) {
        const float* o = x.stufen[0].ecken[0].ort;
        char z[96];
        std::snprintf(z, sizeof z, "%.1f  %.1f  %.1f", static_cast<double>(o[0]), static_cast<double>(o[1]), static_cast<double>(o[2]));
        infoZeile(s, Str::MbXInfoLage, z);
    }
    infoZeile(s, Str::MbXInfoEltern, x.eltern >= 0 ? in.teile[static_cast<std::size_t>(x.eltern)].name : std::string("-"));
    infoZeile(s, Str::MbXInfoAnbauten, anbautenAn(x.name));
    return s;
}

std::string infoKnochen(int b) {
    const GlaAnimation& a = g_app->anim;
    const GlaBone& k = a.bones[static_cast<std::size_t>(b)];
    std::string s;
    int kinder = 0;
    for (const GlaBone& c : a.bones) { kinder += c.parent == b ? 1 : 0; }
    infoZeile(s, Str::MbXInfoNummer, std::to_string(b));
    infoZeile(s, Str::MbXInfoEltern, k.parent >= 0 && static_cast<std::size_t>(k.parent) < a.bones.size() ? a.bones[static_cast<std::size_t>(k.parent)].name : std::string("-"));
    infoZeile(s, Str::MbXInfoKinder, std::to_string(kinder));
    infoZeile(s, Str::MbXInfoAnbauten, anbautenAn(k.name));
    if (k.name == mt().sekundaerKnochen) { s += std::string(tr(Str::MbSekStart)) + "\n"; }
    if (modellSpielKnochen(k.name)) { s += std::string(tr(Str::MbSpielcode)) + "\n"; }
    return s;
}

// Die Sequenz reicht hinter das Ende der GLA (das Spiel kann sie nicht ganz abspielen).
bool sequenzZuLang(const AnimEntry& e) { return g_app->anim.bildZahl() > 0 && e.firstFrame + e.numFrames > g_app->anim.bildZahl(); }

std::string infoSequenz(int i) {
    const AnimEntry& e = g_app->animList[static_cast<std::size_t>(i)];
    char z[400];
    std::snprintf(z, sizeof z, trN(Str::ModelAnimRowHint, e.numFrames), e.numFrames, static_cast<double>(e.numFrames) / static_cast<double>(std::max(e.fps, 1)), e.fps);
    std::string s = std::string(z) + "\n";
    infoZeile(s, Str::MbXInfoBilder, std::to_string(e.firstFrame) + " - " + std::to_string(e.firstFrame + std::max(e.numFrames, 1) - 1));
    if (i == g_app->animIndex) { s += std::string(tr(Str::MbXSeqPrimaerHint)) + "\n"; }
    if (i == mt().sekundaer) {
        std::snprintf(z, sizeof z, tr(Str::MbXSeqSekundaerHint), mt().sekundaerKnochen.c_str());
        s += std::string(z) + "\n";
    }
    if (sequenzZuLang(e)) {
        std::snprintf(z, sizeof z, tr(Str::MbXSeqWarn), e.firstFrame, e.firstFrame + e.numFrames - 1, g_app->anim.bildZahl() - 1);
        s += std::string(z) + "\n";
    }
    return s;
}

void infoOeffnen(const std::string& name, const std::string& info) {
    g_z.infoText = name + "\n" + info;
    g_infoOeffnen = true;
}

// Das Fenster "Info": der Name und die Werte als Tabelle.
void infoFenster() {
    if (g_infoOeffnen) {
        ImGui::OpenPopup("###mtinfo");
        g_infoOeffnen = false;
    }
    g_z.infoOffen = ui::popup("###mtinfo");
    if (!g_z.infoOffen) { return; }
    const std::string& t = g_z.infoText;
    const std::size_t ende = t.find('\n');
    ImGui::TextUnformatted(t.substr(0, ende).c_str());
    ImGui::Separator();
    if (ende != std::string::npos && ImGui::BeginTable("##mtinfotab", 2, ImGuiTableFlags_SizingFixedFit)) {
        std::size_t a = ende + 1;
        while (a < t.size()) {
            std::size_t b = t.find('\n', a);
            if (b == std::string::npos) { b = t.size(); }
            const std::string zeile = t.substr(a, b - a);
            a = b + 1;
            if (zeile.empty()) { continue; }
            const std::size_t p = zeile.find(": ");
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (p == std::string::npos) {
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetFontSize() * 22.0F);
                cjkTextUnformatted(zeile.c_str());
                ImGui::PopTextWrapPos();
                continue;
            }
            cjkTextDisabled("%s", zeile.substr(0, p).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(zeile.substr(p + 2).c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndPopup();
}

// --- Auswahl --------------------------------------------------------------------------------------------------------------

void waehleFlaeche(int i) {
    ModellTab& t = mt();
    // ENTSCHEID3A (B7): wie 3ds Max (Scene Explorer) waehlt der Klick in der Liste auch eine eingefrorene Flaeche - in der Ansicht geht
    // er hindurch; wie Blender (Pfeil "waehlbar" aus) nicht
    const bool frostListe = modellEnt3aFrostInListeWaehlbar();
    if (!frostListe && !modellFlaecheWaehlbar(i, true)) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyShift && g_anker >= 0 && g_anker != i) {
        // Bereich in der Reihenfolge des Baums: alle Flaechenzeilen zwischen der zuletzt angeklickten und dieser
        int a = -1;
        int b = -1;
        for (std::size_t r = 0; r < g_zeilen.size(); ++r) {
            if (g_zeilen[r].art != ExplorerArt::Flaeche) { continue; }
            if (a < 0 && g_zeilen[r].index == g_anker) { a = static_cast<int>(r); }
            if (b < 0 && g_zeilen[r].index == i) { b = static_cast<int>(r); }
        }
        if (a >= 0 && b >= 0) {
            if (!io.KeyCtrl) { t.auswahl.clear(); }
            for (int r = std::min(a, b); r <= std::max(a, b); ++r) {
                const Zeile& z = g_zeilen[static_cast<std::size_t>(r)];
                if (z.art != ExplorerArt::Flaeche || (!frostListe && modellTeilEingefroren(z.index))) { continue; }
                if (std::find(t.auswahl.begin(), t.auswahl.end(), z.index) == t.auswahl.end()) { t.auswahl.push_back(z.index); }
            }
        }
    } else if (io.KeyCtrl) {
        const auto it = std::find(t.auswahl.begin(), t.auswahl.end(), i);
        if (it != t.auswahl.end()) {
            t.auswahl.erase(it);
        } else {
            t.auswahl.push_back(i);
        }
    } else {
        t.auswahl = {i};
    }
    if (!io.KeyShift) { g_anker = i; }
    t.tagWahl = -1;
    g_app->modelDirty = true;
}

void sequenzSekundaer(int i) {
    ModellTab& t = mt();
    t.sekundaer = i;
    t.sekFrame = g_app->animList[static_cast<std::size_t>(i)].firstFrame;
    t.sekZeit = 0.0F;
    g_app->modelDirty = true;
}

void sequenzKopieren(int i) {
    if (i < 0 || static_cast<std::size_t>(i) >= g_app->animList.size()) { return; }
    const std::string& n = g_app->animList[static_cast<std::size_t>(i)].name;
    ImGui::SetClipboardText(n.c_str());
    char kop[200];
    std::snprintf(kop, sizeof kop, tr(Str::ModelAnimCopied), n.c_str());
    addStatus(kop);
}

// --- Rechtsklick-Menues (wie ModView) ----------------------------------------------------------------------------------------

void frostPunkt(bool eingefroren, const std::function<void(bool)>& setzen) {
    if (ImGui::MenuItem(tr(eingefroren ? Str::MbXAuftauen : Str::MbXEinfrieren))) { setzen(!eingefroren); }
    tipp(tr(modellBlender() ? Str::MbXWaehlbarHint : Str::MbXFrostHint));
    modellEnt3aAuftauenPunkt();   // ENTSCHEID3A (B7): wie 3ds Max "Unfreeze Scene Explorer Selection", wenn Gewaehltes eingefroren ist
}

void flaechenMenue(int i, const std::string& name) {
    ModellTab& t = mt();
    if (ImGui::MenuItem(tr(Str::MbXInfo))) { infoOeffnen(name, infoFlaeche(i)); }
    modellAblageMenue(i);   // AP11 (X19): Kopieren (Strg+C), Einfuegen als Kind hiervon (Strg+V)
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::MbFlAn))) { modellFlaecheSchalten(i, 0U); }
    if (ImGui::MenuItem(tr(Str::MbFlAus))) { modellFlaecheSchalten(i, modell::kG2Aus); }
    if (ImGui::MenuItem(tr(Str::MbFlAusOhneKinder))) { modellFlaecheSchalten(i, modell::kG2Aus | modell::kG2OhneKinder); }
    if (ImGui::MenuItem(tr(Str::MbFlWurzel), nullptr, t.g2.wurzel == i)) { modellWurzelSetzen(i); }
    if (ImGui::MenuItem(tr(Str::MbXWurzelLoesen), nullptr, false, t.g2.wurzel != 0)) { modellWurzelSetzen(0); }
    ImGui::Separator();
    // AP11 (X19): Reihenfolge des Entwurfs - Alle _off-Flaechen aus, an, Alle auf Ursprungszustand
    if (ImGui::MenuItem(tr(Str::MbFlKappenAus))) { g_app->showCaps = false; }
    if (ImGui::MenuItem(tr(Str::MbFlKappenAn))) { g_app->showCaps = true; }
    if (ImGui::MenuItem(tr(Str::MbFlUrsprung))) {
        t.g2 = modell::G2Zustand{};
        t.hautAus.clear();   // die Haut gilt wieder
    }
    if (ImGui::MenuItem(tr(Str::MbFlAlleAn))) {
        const modell::Inhalt& in = t.dok.inhalt();
        for (std::size_t k = 0; k < in.teile.size(); ++k) {
            if (!in.teile[k].istTag()) { modellFlaecheSchalten(static_cast<int>(k), 0U); }
        }
    }
    ImGui::Separator();
    frostPunkt(modellTeilEingefroren(i), [i](bool an) { modellTeilEinfrieren(i, an, false); });
    g_app->modelDirty = true;
}

void anbauMenue(const std::string& ziel, bool knochen) {
    if (ImGui::MenuItem(tr(Str::MbAnbauen))) {
        g_anbauZiel = ziel;
        g_anbauKnochen = knochen;
        g_anbauOeffnen = true;
    }
    const bool hat = std::any_of(mt().anbauten.begin(), mt().anbauten.end(), [&](const ModellAnbau& a) { return a.ziel == ziel; });
    if (ImGui::MenuItem(tr(Str::MbLoesen), nullptr, false, hat)) {
        auto& l = mt().anbauten;
        l.erase(std::remove_if(l.begin(), l.end(), [&](const ModellAnbau& a) { return a.ziel == ziel; }), l.end());
        g_app->modelDirty = true;
    }
    if (knochen) {
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::MbSekStartSetzen), nullptr, mt().sekundaerKnochen == ziel)) {
            mt().sekundaerKnochen = ziel;
            g_app->modelDirty = true;
        }
    }
}

void sequenzMenue(int i) {
    ModellTab& t = mt();
    if (ImGui::MenuItem(tr(Str::MbAlsPrimaer))) { animWaehlen(i, true); }
    char sek[160];
    std::snprintf(sek, sizeof sek, tr(Str::MbAlsSekundaer), t.sekundaerKnochen.c_str());
    if (ImGui::MenuItem(sek, nullptr, t.sekundaer == i)) { sequenzSekundaer(i); }
    if (ImGui::MenuItem(tr(Str::MbSekundaerAus), nullptr, false, t.sekundaer >= 0)) {
        t.sekundaer = -1;
        g_app->modelDirty = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::MbInsSkriptMenue), "Ctrl+Shift+I")) { modellInsSkriptOeffnen(i); }
    if (ImGui::MenuItem(tr(Str::MbXSeqKopieren))) { sequenzKopieren(i); }
    tipp(tr(Str::HintCopy));
    if (ImGui::MenuItem(tr(Str::MbKgAufFreien))) { (void)modellAufFreienPlatz(i); }   // Stufe 2
    tipp(tr(Str::MbKgZiehen));
}

void gruppenMenue(Gruppe g) {
    if (ImGui::MenuItem(tr(Str::MbAllesEinblenden), modellBlender() ? "Alt+H" : nullptr)) { modellAllesEinblenden(); }
    if (g != Gruppe::Keine && g != Gruppe::Griffe) {
        const GruppenStand s = gruppenStand(g);
        frostPunkt(s.zahl > 0 && s.frost == s.zahl, [g](bool an) { gruppeFrost(g, an); });
    }
    if (ImGui::MenuItem(tr(Str::MbXAllesAuftauen))) { modellAllesAuftauen(); }
}

// Das Fenster "Modell anbauen": alle .glm der Spielarchive mit Suche.
void anbauFenster() {
    if (g_anbauOeffnen) {
        ImGui::OpenPopup("###mtanbau");
        g_anbauOeffnen = false;
    }
    char titel[200];
    std::snprintf(titel, sizeof titel, tr(Str::MbAnbauTitel), g_anbauZiel.c_str());
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 26.0F, fs * 26.0F}, ImGuiCond_Appearing);
    if (!ui::modalBegin((std::string(titel) + "###mtanbau").c_str(), nullptr)) { return; }
    static std::vector<std::string> modelle;
    if (modelle.empty()) {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".glm"})) { modelle.push_back(f.name); }
        }
        std::sort(modelle.begin(), modelle.end());
        modelle.erase(std::unique(modelle.begin(), modelle.end()), modelle.end());
    }
    static char suche[96] = "weapons2/saber";
    static bool ersetzen = true;
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##anbausuche", tr(Str::MbSuche), suche, sizeof suche);
    ImGui::Checkbox(tr(Str::MbAnbauErsetzen), &ersetzen);
    const std::string such = klein(suche);
    ImGui::BeginChild("##anbauliste", ImVec2{-1.0F, -(ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y)}, ImGuiChildFlags_Borders);
    std::vector<const std::string*> treffer;
    for (const std::string& m : modelle) {
        if (passt(m, such)) { treffer.push_back(&m); }
    }
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(treffer.size()));
    while (clipper.Step()) {
        for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r) {
            const std::string& m = *treffer[static_cast<std::size_t>(r)];
            if (ImGui::Selectable(m.c_str())) {
                (void)modellAnbauen(m, g_anbauZiel, g_anbauKnochen, ersetzen);
                ImGui::CloseCurrentPopup();
            }
        }
    }
    ImGui::EndChild();
    if (ui::knopfReihe(nullptr, tr(Str::EditorCancel)) == ui::Knopf::Abbrechen) { ImGui::CloseCurrentPopup(); }   // UIFORM: unten rechts, Esc
    ImGui::EndPopup();
}

// --- Zeichnen ----------------------------------------------------------------------------------------------------------------------

// Die Schrift auf einer gewaehlten Zeile: die des Farbschemas - ausser sie hebt sich vom Balken der Auswahl nicht ab (klassisches
// Schema: schwarze Schrift, dunkelblaue Auswahl), dann Weiss bzw. Schwarz.
ImU32 textAufAuswahl(ImU32 normal) {
    const auto hell = [](const ImVec4& c) { return c.x * 0.299F + c.y * 0.587F + c.z * 0.114F; };
    const ImVec4 balken = ImGui::GetStyleColorVec4(ImGuiCol_Header);
    if (std::fabs(hell(balken) - hell(ImGui::ColorConvertU32ToFloat4(normal))) > 0.35F) { return normal; }
    return hell(balken) < 0.5F ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
}

struct Masse {
    bool blender = false;
    float fs = 0.0F;
    float zeileH = 0.0F;
    float pfeilB = 0.0F;
    float augeB = 0.0F;
    float symB = 0.0F;
    float icon = 0.0F;      // Kantenlaenge der Symbole
    float einzug = 0.0F;
    bool merken = false;    // die Lage der Zeilen fuer den Selbsttest festhalten
    modell::G2Zustand g2;   // Ghoul2-Zustand, wie er gezeichnet wird (mit eingeschalteten Kappen)
};

// Ein kleiner Schalter in der Zeile (unsichtbarer Knopf, das Symbol zeichnet der Aufrufer).
bool schalter(const char* id, float x, float y, float b, float h) {
    ImGui::SetCursorScreenPos(ImVec2{x, y});
    return ImGui::InvisibleButton(id, ImVec2{b, h});
}

void symbol(UiIcon icon, float x, float y, const Masse& m, float b, ImU32 farbe) {
    uiIcon(ImGui::GetWindowDrawList(), icon, ImVec2{std::floor(x + (b - m.icon) * 0.5F), std::floor(y + (m.zeileH - m.icon) * 0.5F)}, m.icon, farbe);
}

ImU32 farbeVon(UiIcon icon, float alpha = 1.0F) {
    ImVec4 f = uiIconFarbe(icon);
    f.w *= alpha;
    return ImGui::GetColorU32(f);
}

// Das Auge einer Zeile: Klick schaltet, Strg/Alt+Klick zeigt nur dieses. `aktiv` false: ohne Wirkung (LOD-Ebene ohne Stufe).
bool augeSchalter(float x, float y, const Masse& m, bool versteckt, bool aktiv, Str hinweis, bool& solo) {
    ++mt().augenZaehler;
    const bool k = schalter("##au", x, y, m.augeB, m.zeileH) && aktiv;
    const bool ueber = ImGui::IsItemHovered();
    const UiIcon icon = versteckt ? UiIcon::Versteckt : UiIcon::Sichtbar;
    symbol(icon, x, y, m, m.augeB, farbeVon(icon, !aktiv ? 0.3F : (ueber ? 1.0F : (versteckt ? 0.55F : 0.8F))));
    tipp(tr(hinweis));
    solo = k && (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyAlt);
    return k;
}

// Die Schneeflocke (wie 3ds Max) bzw. der Pfeil "waehlbar" (wie Blender).
bool frostSchalter(float x, float y, float b, const Masse& m, bool eingefroren) {
    const bool k = schalter("##fr", x, y, b, m.zeileH);
    const bool ueber = ImGui::IsItemHovered();
    if (m.blender) {
        symbol(UiIcon::MbWaehlbar, x, y, m, b, farbeVon(UiIcon::MbWaehlbar, eingefroren ? 0.28F : (ueber ? 1.0F : 0.8F)));
    } else if (eingefroren || ueber) {
        symbol(UiIcon::MbFrost, x, y, m, b, farbeVon(UiIcon::MbFrost, eingefroren ? 1.0F : 0.45F));
    } else {
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{x + b * 0.5F, y + m.zeileH * 0.5F}, std::max(1.0F, m.fs * 0.07F), ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6F));
    }
    tipp(tr(m.blender ? Str::MbXWaehlbarHint : Str::MbXFrostHint));
    return k;
}

// Die Knopfzeile unter "Sequenzen": Gruppe nach dem Anfang des Namens, rechts "Ins Skript" fuer die gewaehlte.
void sequenzFilterZeile(float x, float y, float ende, const Masse& m) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{m.fs * 0.3F, std::max(0.0F, std::floor((m.zeileH - m.fs) * 0.5F) - 1.0F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{m.fs * 0.25F, 0.0F});
    static const char* const kNamen[] = {"*", "BOTH", "TORSO", "LEGS", "FACE"};
    const auto gruppenName = [](int k) { return k == 0 ? tr(Str::ModelAnimAll) : kNamen[k]; };
    ImGui::SetCursorScreenPos(ImVec2{x, y});
    // Klappliste statt fuenf Knoepfen: sie passt auch in eine schmale Spalte
    ImGui::SetNextItemWidth(std::clamp(ende - x, m.fs * 3.5F, m.fs * 7.0F));
    if (ImGui::BeginCombo("##seqgruppe", gruppenName(std::clamp(g_z.seqGruppe, 0, 4)))) {
        for (int k = 0; k < 5; ++k) {
            if (ImGui::Selectable(gruppenName(k), g_z.seqGruppe == k)) { g_z.seqGruppe = k; }
        }
        ImGui::EndCombo();
    }
    if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), "##seqgruppe"); }
    tipp(tr(Str::MbXSeqGruppeHint));
    ImGui::PopStyleVar(2);
}

struct ZeilenText {
    std::string name;      // normal geschrieben
    std::string zusatz;    // grau dahinter
    std::string voll;      // der ganze Name (Tooltip, Lage fuer den Selbsttest)
    std::string kennung;   // Beschriftung im Elementverzeichnis, wenn sie von `voll` abweicht
};

// Eine Kopfzeile "Surfaces (LOD 1, 18 drawn)": der Teil ab der Klammer steht grau.
void teileKopf(const std::string& text, ZeilenText& t) {
    t.voll = text;
    std::size_t p = text.find(" (");
    if (p == std::string::npos) { p = text.find("\xef\xbc\x88"); }   // volle Klammer der CJK-Texte
    t.name = p == std::string::npos ? text : text.substr(0, p);
    t.zusatz = p == std::string::npos ? std::string() : text.substr(text[p] == ' ' ? p + 1 : p);
}

ZeilenText beschrifte(const Zeile& r) {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    ZeilenText x;
    char kopf[200];
    switch (r.art) {
        case ExplorerArt::Modell:
            x.name = modellName();
            x.zusatz = tr(Str::MbRModell);
            x.voll = x.name;
            break;
        case ExplorerArt::KopfFlaechen: {
            int gezeichnet = 0;
            for (const char g : t.gezeichnet) { gezeichnet += g != 0 ? 1 : 0; }
            std::snprintf(kopf, sizeof kopf, tr(Str::MbFlaechen), t.lodJetzt + 1, gezeichnet);
            teileKopf(kopf, x);
            break;
        }
        case ExplorerArt::KopfTags: {
            int tags = 0;
            for (const modell::Teil& y : in.teile) { tags += y.istTag() ? 1 : 0; }
            std::snprintf(kopf, sizeof kopf, tr(Str::MbTags), tags);
            teileKopf(kopf, x);
            break;
        }
        case ExplorerArt::KopfKnochen:
            std::snprintf(kopf, sizeof kopf, tr(Str::MbKnochen), glaName().c_str(), static_cast<int>(g_app->anim.bones.size()));
            teileKopf(kopf, x);
            break;
        case ExplorerArt::KopfRig:
            std::snprintf(kopf, sizeof kopf, tr(Str::MbRgExplorer), modellRigExplorerZahl());
            teileKopf(kopf, x);
            break;
        case ExplorerArt::KopfSequenzen:
            std::snprintf(kopf, sizeof kopf, tr(Str::MbXSequenzen), static_cast<int>(g_app->animList.size()));
            teileKopf(kopf, x);
            break;
        case ExplorerArt::Ebene: {
            int zahl = 0;
            if (r.index < kEbenenFest) {
                x.name = ebenenName(r.index);
                zahl = gruppenStand(gruppeVon(static_cast<ExplorerEbene>(r.index))).zahl;
            } else {
                const int stufe = r.index - kEbenenFest + 2;
                std::snprintf(kopf, sizeof kopf, tr(Str::MbXEbeneLod), stufe);
                x.name = kopf;
                for (const modell::Teil& y : in.teile) { zahl += !y.istTag() && static_cast<int>(y.stufen.size()) >= stufe ? 1 : 0; }
            }
            x.zusatz = "(" + std::to_string(zahl) + ")";
            x.voll = x.name;
            x.kennung = x.name + " " + x.zusatz;   // "Bones (53)": "Bones" allein ist der Knopf der Werkzeugleiste
            break;
        }
        case ExplorerArt::Flaeche:
        case ExplorerArt::Tag:
            x.name = in.teile[static_cast<std::size_t>(r.index)].name;
            x.voll = x.name;
            break;
        case ExplorerArt::Knochen:
            x.name = g_app->anim.bones[static_cast<std::size_t>(r.index)].name;
            x.voll = x.name;
            break;
        case ExplorerArt::Anbau: {
            const ModellAnbau& a = t.anbauten[static_cast<std::size_t>(r.index)];
            x.name = fileName(a.pfad);
            if (r.ebene == static_cast<int>(ExplorerEbene::Anbauten)) { x.zusatz = a.ziel; }
            x.voll = x.name;
            break;
        }
        case ExplorerArt::Griff:
            x.name = modellRigExplorerName(r.index);
            x.voll = x.name;
            break;
        case ExplorerArt::Sequenz: {
            const AnimEntry& e = g_app->animList[static_cast<std::size_t>(r.index)];
            x.name = e.name;
            x.zusatz = std::to_string(e.numFrames);
            if (e.loopFrame >= 0) { x.zusatz += std::string(" \xc2\xb7 ") + tr(Str::MbXSeqSchleife); }
            x.voll = x.name;
            break;
        }
        case ExplorerArt::Hinweis:
            if (r.index >= 2) {
                std::snprintf(kopf, sizeof kopf, tr(Str::MbXEbeneLodLeer), r.index);
                x.zusatz = kopf;
            } else {
                x.zusatz = tr(r.index == -2 ? Str::ModelAnimNone : (r.index == -3 ? Str::MbXKeinTreffer : (r.index == -4 ? Str::ModelNoAnim : Str::MbXEbeneLeer)));
            }
            break;
        case ExplorerArt::SeqFilter: break;
    }
    return x;
}

// Hat diese Art ein Auge / eine Schneeflocke, und worauf wirken sie?
Gruppe kopfGruppe(const Zeile& r) {
    switch (r.art) {
        case ExplorerArt::KopfFlaechen: return Gruppe::Flaechen;
        case ExplorerArt::KopfTags: return Gruppe::Tags;
        case ExplorerArt::KopfKnochen: return Gruppe::Knochen;
        case ExplorerArt::KopfRig: return Gruppe::Griffe;
        case ExplorerArt::Ebene: return r.index < kEbenenFest ? gruppeVon(static_cast<ExplorerEbene>(r.index)) : Gruppe::Keine;
        default: return Gruppe::Keine;
    }
}

void zeichneZeile(const Zeile& r, const Masse& m) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    // FIX612C (P3-7): Probe fuer den Selbsttest - eine Zeile, deren Eintrag es nicht mehr gibt
    if ((r.art == ExplorerArt::Anbau && static_cast<std::size_t>(r.index) >= t.anbauten.size()) || (r.art == ExplorerArt::Griff && r.index >= modellRigExplorerZahl())) {
        ++fix612cProbe().explorerHinterEnde;
        return;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::TableNextRow(ImGuiTableRowFlags_None, m.zeileH);
    ImGui::TableSetColumnIndex(0);
    ImGui::PushID(schluessel(r.art, r.index, r.ebene));
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const float zellenEnde = p0.x + ImGui::GetContentRegionAvail().x;
    g_nameBreite = zellenEnde - p0.x;
    const float y = p0.y;
    const float textY = std::floor(y + (m.zeileH - m.fs) * 0.5F);
    float x = p0.x + static_cast<float>(r.tiefe) * m.einzug;
    ExplorerLage lage;
    lage.art = r.art;
    lage.index = r.index;
    lage.tiefe = r.tiefe;
    lage.y0 = y;
    lage.y1 = y + m.zeileH;
    lage.xZelleEnde = zellenEnde;
    lage.kinder = r.kinder;
    lage.offen = r.offen;

    if (r.art == ExplorerArt::SeqFilter) {
        sequenzFilterZeile(x + m.pfeilB, y, zellenEnde, m);
        ImGui::PopID();
        return;
    }
    const ZeilenText text = beschrifte(r);
    if (r.art == ExplorerArt::Hinweis) {
        ImGui::SetCursorScreenPos(ImVec2{x + m.pfeilB, y});
        ImGui::Dummy(ImVec2{std::max(zellenEnde - x - m.pfeilB, 4.0F), m.zeileH});
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::RenderTextEllipsis(dl, ImVec2{x + m.pfeilB, textY}, ImVec2{zellenEnde, y + m.zeileH}, zellenEnde, text.zusatz.c_str(), nullptr, nullptr);
        ImGui::PopStyleColor();
        ImGui::PopID();
        return;
    }

    // --- Zustand der Zeile -------------------------------------------------------------------------------------------------
    const Gruppe gruppe = kopfGruppe(r);
    const GruppenStand stand = gruppe != Gruppe::Keine ? gruppenStand(gruppe) : GruppenStand{};
    bool hatAuge = false;
    bool hatFrost = false;
    bool versteckt = false;
    bool eingefroren = false;
    bool gewaehlt = false;
    bool grau = false;
    bool augeAktiv = true;
    Str augeHinweis = Str::MbAugeHint;
    UiIcon icon = UiIcon::MbModell;
    ImU32 iconFarbe = 0;
    switch (r.art) {
        case ExplorerArt::Modell: icon = UiIcon::MbModell; break;
        case ExplorerArt::KopfFlaechen: icon = UiIcon::MbLod; break;
        case ExplorerArt::KopfTags: icon = UiIcon::MbTag; break;
        case ExplorerArt::KopfKnochen: icon = UiIcon::MbKnochen; break;
        case ExplorerArt::KopfRig: icon = UiIcon::MbRig; break;
        case ExplorerArt::KopfSequenzen: icon = UiIcon::MbSequenz; break;
        case ExplorerArt::Ebene: {
            static const UiIcon kIcons[kEbenenFest] = {UiIcon::MbFlaecheKinder, UiIcon::MbKappe, UiIcon::MbTag, UiIcon::MbKnochen, UiIcon::MbAnbau};
            icon = r.index < kEbenenFest ? kIcons[r.index] : UiIcon::MbLod;
            if (r.index >= kEbenenFest) {   // Detailstufe: das Auge zeigt diese Stufe in der Ansicht
                const int stufe = r.index - kEbenenFest + 2;
                hatAuge = true;
                augeAktiv = stufe <= in.stufenZahl;
                versteckt = t.lodJetzt + 1 != stufe;
                augeHinweis = Str::MbXLodAugeHint;
            }
            break;
        }
        case ExplorerArt::Flaeche: {
            const int status = flaechenStatus(m.g2, r.index);
            icon = status == 0 ? UiIcon::MbFlAn : (status == 1 ? UiIcon::MbFlAus : UiIcon::MbG2Aus);
            lage.marke = status | (t.g2.wurzel == r.index && r.index != 0 ? 8 : 0);
            hatAuge = true;
            hatFrost = true;
            versteckt = modellTeilVersteckt(r.index);
            eingefroren = modellTeilEingefroren(r.index);
            gewaehlt = std::find(t.auswahl.begin(), t.auswahl.end(), r.index) != t.auswahl.end();
            grau = !(static_cast<std::size_t>(r.index) < t.gezeichnet.size() && t.gezeichnet[static_cast<std::size_t>(r.index)] != 0);
            break;
        }
        case ExplorerArt::Tag:
            icon = UiIcon::MbTag;
            hatAuge = true;
            hatFrost = true;
            versteckt = modellTeilVersteckt(r.index);
            eingefroren = modellTeilEingefroren(r.index);
            gewaehlt = t.tagWahl == r.index;
            break;
        case ExplorerArt::Knochen:
            icon = g_app->anim.bones[static_cast<std::size_t>(r.index)].parent < 0 ? UiIcon::MbWurzel : UiIcon::MbKnochen;
            hatAuge = true;
            hatFrost = true;
            versteckt = modellKnochenVersteckt(r.index);
            eingefroren = modellKnochenEingefroren(r.index);
            gewaehlt = t.knochenWahl == r.index;
            break;
        case ExplorerArt::Anbau:
            icon = UiIcon::MbAnbau;
            hatAuge = true;
            versteckt = t.anbauten[static_cast<std::size_t>(r.index)].versteckt;
            eingefroren = g_z.frostAnbauten;
            break;
        case ExplorerArt::Griff:
            modellRigExplorerSymbol(r.index, icon, iconFarbe);
            hatAuge = true;
            t.griffVersteckt.resize(static_cast<std::size_t>(std::max(modellRigExplorerZahl(), 0)), 0);
            versteckt = static_cast<std::size_t>(r.index) < t.griffVersteckt.size() && t.griffVersteckt[static_cast<std::size_t>(r.index)] != 0;
            gewaehlt = t.griffWahl == r.index;
            break;
        case ExplorerArt::Sequenz: {
            const bool primaer = r.index == g_app->animIndex;
            const bool sekundaer = r.index == t.sekundaer;
            icon = primaer || sekundaer ? UiIcon::Schloss : UiIcon::MbSeqSpiel;
            gewaehlt = primaer || sekundaer;
            lage.marke = (primaer ? 1 : 0) | (sekundaer ? 2 : 0) | (sequenzZuLang(g_app->animList[static_cast<std::size_t>(r.index)]) ? 4 : 0);
            break;
        }
        default: break;
    }
    if (gruppe != Gruppe::Keine) {
        hatAuge = true;
        hatFrost = gruppe != Gruppe::Griffe;
        versteckt = stand.zahl > 0 && stand.versteckt == stand.zahl;
        eingefroren = stand.zahl > 0 && stand.frost == stand.zahl;
        augeHinweis = Str::MbXAugeArtHint;
    }
    if (iconFarbe == 0) { iconFarbe = farbeVon(icon); }
    grau = grau || versteckt || eingefroren;

    // --- Pfeil -----------------------------------------------------------------------------------------------------------
    if (r.kinder) {
        lage.xPfeil = x;
        if (schalter("##pf", x, y, m.pfeilB, m.zeileH)) { setzeOffen(r.art, r.index, !r.offen); }
        const ImU32 pf = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        const float a = m.fs * 0.26F;
        const ImVec2 c{std::floor(x + m.pfeilB * 0.5F), std::floor(y + m.zeileH * 0.5F)};
        if (r.offen) {
            dl->AddTriangleFilled(ImVec2{c.x - a, c.y - a * 0.6F}, ImVec2{c.x + a, c.y - a * 0.6F}, ImVec2{c.x, c.y + a * 0.75F}, pf);
        } else {
            dl->AddTriangleFilled(ImVec2{c.x - a * 0.6F, c.y - a}, ImVec2{c.x - a * 0.6F, c.y + a}, ImVec2{c.x + a * 0.75F, c.y}, pf);
        }
    }
    x += m.pfeilB;

    // --- Auge (wie 3ds Max links an der Zeile, wie Blender rechts in seiner Spalte) -----------------------------------------------
    const auto augeKlick = [&](bool solo) {
        switch (r.art) {
            case ExplorerArt::Flaeche:
            case ExplorerArt::Tag: modellTeilAuge(r.index, solo); break;
            case ExplorerArt::Knochen: modellKnochenAuge(r.index, solo); break;
            case ExplorerArt::Anbau:
                t.anbauten[static_cast<std::size_t>(r.index)].versteckt = !versteckt;
                g_app->modelDirty = true;
                break;
            case ExplorerArt::Griff:
                t.griffVersteckt[static_cast<std::size_t>(r.index)] = versteckt ? 0 : 1;
                g_app->modelDirty = true;
                break;
            case ExplorerArt::Ebene:
                if (r.index >= kEbenenFest) {   // Detailstufe zeigen; noch ein Klick: automatisch wie im Spiel
                    const int stufe = r.index - kEbenenFest + 2;
                    t.lodWahl = t.lodWahl == stufe ? 0 : stufe;
                    g_app->modelDirty = true;
                    break;
                }
                [[fallthrough]];
            default:
                if (gruppe != Gruppe::Keine) { gruppeAuge(gruppe, versteckt); }
                break;
        }
    };
    const auto frostKlick = [&] {
        const bool an = !eingefroren;
        const bool samt = ImGui::GetIO().KeyShift;
        if (gruppe != Gruppe::Keine) {
            gruppeFrost(gruppe, an);
        } else if (r.art == ExplorerArt::Knochen) {
            modellKnochenEinfrieren(r.index, an, samt);
        } else {
            modellTeilEinfrieren(r.index, an, samt);
        }
    };
    if (hatAuge && !m.blender) {
        lage.xAuge = x;
        bool solo = false;
        if (augeSchalter(x, y, m, versteckt, augeAktiv, augeHinweis, solo)) { augeKlick(solo); }
        x += m.augeB;
    }

    // --- Symbol -------------------------------------------------------------------------------------------------------------
    lage.xSymbol = x;
    ++t.symbolZaehler;
    symbol(icon, x, y, m, m.symB, iconFarbe);
    x += m.symB;
    if (r.art == ExplorerArt::Sequenz && (lage.marke & 3) != 0) {   // Marke P / S hinter dem Schloss
        const char* marke = (lage.marke & 1) != 0 ? "P" : "S";
        dl->AddText(ImVec2{x, textY}, farbeVon(UiIcon::Schloss), marke);
        x += ImGui::CalcTextSize("P").x + m.fs * 0.3F;
    }

    // --- Name: der Knopf der Zeile (Klick waehlt, Rechtsklick oeffnet das Menue) ---------------------------------------------------------
    lage.xName = x;
    ImGui::SetCursorScreenPos(ImVec2{x, y});
    const bool klick = ImGui::InvisibleButton("##n", ImVec2{std::max(zellenEnde - x, 4.0F), m.zeileH});
    const bool ueber = ImGui::IsItemHovered();
    if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), text.kennung.empty() ? text.voll.c_str() : text.kennung.c_str()); }
    if (r.art == ExplorerArt::Sequenz) { modellSequenzZiehen(r.index); }   // Stufe 2: auf einen Platz der Karten-GLA ziehen
    if (gewaehlt || ueber) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(gewaehlt ? (ueber ? ImGuiCol_HeaderHovered : ImGuiCol_Header) : ImGuiCol_HeaderHovered, gewaehlt ? 1.0F : 0.45F));
    }
    const ImVec2 groesse = ImGui::CalcTextSize(text.name.c_str());
    const bool fett = r.art == ExplorerArt::Modell;
    ImU32 textFarbe = ImGui::GetColorU32(grau ? ImGuiCol_TextDisabled : ImGuiCol_Text);
    if (gewaehlt) { textFarbe = textAufAuswahl(textFarbe); }
    lage.gekuerzt = x + groesse.x > zellenEnde + 0.5F;
    lage.xNameEnde = std::min(x + groesse.x, zellenEnde);
    if (lage.gekuerzt) {
        ImGui::PushStyleColor(ImGuiCol_Text, textFarbe);
        ImGui::RenderTextEllipsis(dl, ImVec2{x, textY}, ImVec2{zellenEnde, y + m.zeileH}, zellenEnde, text.name.c_str(), nullptr, &groesse);
        ImGui::PopStyleColor();
    } else {
        dl->AddText(ImVec2{x, textY}, textFarbe, text.name.c_str());
        if (fett) { dl->AddText(ImVec2{x + 1.0F, textY}, textFarbe, text.name.c_str()); }
        float nx = x + groesse.x + (fett ? 1.0F : 0.0F) + m.fs * 0.4F;
        const auto dahinter = [&](UiIcon i, ImU32 f) {   // ein kleines Zeichen hinter dem Namen
            if (nx + m.icon <= zellenEnde) { uiIcon(dl, i, ImVec2{std::floor(nx), std::floor(y + (m.zeileH - m.icon) * 0.5F)}, m.icon, f); }
            nx += m.icon + m.fs * 0.2F;
        };
        if ((lage.marke & 8) != 0 && r.art == ExplorerArt::Flaeche) { dahinter(UiIcon::MbAnker, farbeVon(UiIcon::MbAnker)); }
        if (r.art == ExplorerArt::Knochen) {
            const std::string& kn = g_app->anim.bones[static_cast<std::size_t>(r.index)].name;
            if (kn == t.sekundaerKnochen) {
                dahinter(UiIcon::MbStern, farbeVon(UiIcon::MbStern));
                const char* s = tr(Str::MbSekStart);
                if (nx < zellenEnde) { ImGui::RenderTextEllipsis(dl, ImVec2{nx, textY}, ImVec2{zellenEnde, y + m.zeileH}, zellenEnde, s, nullptr, nullptr); }
                nx += ImGui::CalcTextSize(s).x + m.fs * 0.4F;
            }
            if (modellSpielKnochen(kn) && nx + m.fs * 0.4F <= zellenEnde) {   // dieser Knochen wird vom Spielcode gesucht
                dl->AddCircleFilled(ImVec2{nx + m.fs * 0.18F, y + m.zeileH * 0.5F}, m.fs * 0.18F, ImGui::GetColorU32(ImGuiCol_PlotHistogram));
            }
        }
        if (!text.zusatz.empty() && nx < zellenEnde) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            const ImVec2 zg = ImGui::CalcTextSize(text.zusatz.c_str());
            ImGui::RenderTextEllipsis(dl, ImVec2{nx, textY}, ImVec2{zellenEnde, y + m.zeileH}, zellenEnde, text.zusatz.c_str(), nullptr, &zg);
            ImGui::PopStyleColor();
            nx += zg.x + m.fs * 0.4F;
        }
        if ((lage.marke & 4) != 0 && r.art == ExplorerArt::Sequenz) { dahinter(UiIcon::Warnung, farbeVon(UiIcon::Fehler)); }
        if (eingefroren && m.blender == false && r.art == ExplorerArt::Anbau) { dahinter(UiIcon::MbFrost, farbeVon(UiIcon::MbFrost)); }
    }

    // --- Klick, Tooltip, Menue ----------------------------------------------------------------------------------------------------
    if (ueber && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {   // die Info erst unter der Maus bestimmen (zaehlt Ecken und Knochenbezuege)
        std::string info;
        switch (r.art) {
            case ExplorerArt::Flaeche:
                info = infoFlaeche(r.index);
                if (lage.marke >= 8) { info += std::string(tr(Str::MbXAnkerHint)) + "\n"; }
                if (static_cast<std::size_t>(r.index) < t.gezeichnet.size() && t.gezeichnet[static_cast<std::size_t>(r.index)] == 0) { info += std::string(tr(Str::MbG2AusHint)) + "\n"; }
                break;
            case ExplorerArt::Tag: info = infoTag(r.index); break;
            case ExplorerArt::Knochen: info = infoKnochen(r.index); break;
            case ExplorerArt::Sequenz: info = infoSequenz(r.index); break;
            case ExplorerArt::Griff: info = modellRigExplorerHinweis(r.index); break;
            case ExplorerArt::KopfRig: info = tr(Str::MbRgExplorerHint); break;
            case ExplorerArt::KopfFlaechen: info = infoFlaechenKopf(); break;
            case ExplorerArt::Ebene:
                if (r.index == static_cast<int>(ExplorerEbene::Koerper)) { info = infoFlaechenKopf(); }
                break;
            case ExplorerArt::Anbau: info = t.anbauten[static_cast<std::size_t>(r.index)].pfad + "\n" + t.anbauten[static_cast<std::size_t>(r.index)].ziel; break;
            default: break;
        }
        // ENTSCHEID3A (B7): wie 3ds Max ist die eingefrorene Zeile in der Liste waehlbar - der Tooltip sagt es
        if (eingefroren && modellEnt3aFrostInListeWaehlbar() && (r.art == ExplorerArt::Flaeche || r.art == ExplorerArt::Tag || r.art == ExplorerArt::Knochen)) {
            info += std::string(tr(Str::EaFrostListeHint)) + "\n";
        }
        if (lage.gekuerzt || !info.empty()) {
            const std::string tip = text.voll + (info.empty() ? std::string() : "\n" + info);
            if (ImGui::BeginTooltip()) {
                ImGui::PushTextWrapPos(m.fs * 24.0F);
                cjkTextUnformatted(tip.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
            g_z.tooltip = tip;
        }
    }
    if (ueber && r.art == ExplorerArt::Sequenz && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { modellInsSkriptOeffnen(r.index); }
    if (klick) {
        switch (r.art) {
            case ExplorerArt::Flaeche: waehleFlaeche(r.index); break;
            case ExplorerArt::Tag:
                if (eingefroren && !modellEnt3aFrostInListeWaehlbar()) {   // ENTSCHEID3A (B7): wie 3ds Max waehlbar
                    meldeFrost(text.name);
                } else {
                    t.tagWahl = r.index;
                    t.an.tags = true;   // Entwurf: ein Klick auf einen Tag blendet die Tags ein
                    g_app->modelDirty = true;
                }
                break;
            case ExplorerArt::Knochen:
                if (eingefroren && !modellEnt3aFrostInListeWaehlbar()) {   // ENTSCHEID3A (B7): wie 3ds Max waehlbar
                    meldeFrost(text.name);
                } else {
                    t.knochenWahl = r.index;
                    g_app->modellKnochen = true;   // Entwurf: ein Klick auf einen Knochen blendet die Knochen ein
                    g_app->modelDirty = true;
                }
                break;
            case ExplorerArt::Griff: modellRigGriffWaehlen(r.index); break;
            case ExplorerArt::Sequenz:
                if (g_z.torsoSekundaer && text.name.rfind("TORSO_", 0) == 0) {
                    sequenzSekundaer(r.index);   // wie ModView: TORSO_... auf den Oberkoerper, die Primaersequenz bleibt
                    g_app->animPlaying = true;
                } else {
                    animWaehlen(r.index, true);
                }
                break;
            default:
                if (r.kinder) { setzeOffen(r.art, r.index, !r.offen); }   // Kopfzeilen: der Klick auf den Namen klappt
                break;
        }
    }
    if (ui::popupKontext("##m")) {
        switch (r.art) {
            case ExplorerArt::Flaeche: flaechenMenue(r.index, text.name); break;
            case ExplorerArt::Tag:
                if (ImGui::MenuItem(tr(Str::MbXInfo))) { infoOeffnen(text.name, infoTag(r.index)); }
                ImGui::Separator();
                anbauMenue(text.name, false);
                ImGui::Separator();
                frostPunkt(eingefroren, [&](bool an) { modellTeilEinfrieren(r.index, an, false); });
                break;
            case ExplorerArt::Knochen:
                if (ImGui::MenuItem(tr(Str::MbXInfo))) { infoOeffnen(text.name, infoKnochen(r.index)); }
                ImGui::Separator();
                anbauMenue(text.name, true);
                ImGui::Separator();
                frostPunkt(eingefroren, [&](bool an) { modellKnochenEinfrieren(r.index, an, false); });
                break;
            case ExplorerArt::Anbau:
                if (ImGui::MenuItem(tr(Str::MbLoesen))) {
                    t.anbauten.erase(t.anbauten.begin() + r.index);
                    g_app->modelDirty = true;
                }
                break;
            case ExplorerArt::Sequenz: sequenzMenue(r.index); break;
            case ExplorerArt::Griff: modellRigExplorerGriffMenue(r.index); break;
            case ExplorerArt::KopfRig: modellRigExplorerKopfMenue(); break;
            case ExplorerArt::KopfSequenzen:
                if (ImGui::MenuItem(tr(Str::MbXTorsoSekundaer), nullptr, g_z.torsoSekundaer)) { g_z.torsoSekundaer = !g_z.torsoSekundaer; }
                break;
            case ExplorerArt::KopfKnochen:
                if (ImGui::MenuItem(tr(Str::MbXKnochenFlach), nullptr, g_z.knochenFlach)) { g_z.knochenFlach = !g_z.knochenFlach; }
                ImGui::Separator();
                gruppenMenue(gruppe);
                break;
            default: gruppenMenue(gruppe); break;
        }
        ImGui::EndPopup();
    }

    // --- Spalten rechts ----------------------------------------------------------------------------------------------------------
    if (m.blender) {
        ImGui::TableSetColumnIndex(1);
        if (hatAuge) {
            const ImVec2 q = ImGui::GetCursorScreenPos();
            lage.xAuge = q.x;
            bool solo = false;
            if (augeSchalter(q.x, y, m, versteckt, augeAktiv, augeHinweis, solo)) { augeKlick(solo); }
        }
        if (g_z.spalteWaehlbar) {
            ImGui::TableSetColumnIndex(2);
            if (hatFrost) {
                const ImVec2 q = ImGui::GetCursorScreenPos();
                lage.xFrost = q.x;
                if (frostSchalter(q.x, y, m.augeB, m, eingefroren)) { frostKlick(); }
            }
        }
    } else {
        ImGui::TableSetColumnIndex(1);
        if (hatFrost) {
            const ImVec2 q = ImGui::GetCursorScreenPos();
            const float b = std::max(ImGui::GetContentRegionAvail().x, m.augeB);
            lage.xFrost = q.x + (b - m.augeB) * 0.5F;
            if (frostSchalter(q.x, y, b, m, eingefroren)) { frostKlick(); }
        }
    }
    ImGui::PopID();
    if (m.merken && y + m.zeileH > ImGui::GetWindowPos().y && y < ImGui::GetWindowPos().y + ImGui::GetWindowHeight()) {
        lage.name = text.voll;
        lage.versteckt = versteckt;
        lage.eingefroren = eingefroren;
        lage.gewaehlt = gewaehlt;
        g_z.lage.push_back(std::move(lage));
    }
}

// Die Zeile, die modellExplorerZeigen verlangt hat, ins Bild rollen (im Fenster der Tabelle aufzurufen).
void rolleZurZeile(float zeileH) {
    if (!g_zeigen) { return; }
    g_zeigen = false;
    const bool kopf = g_zeigeArt != ExplorerArt::Flaeche && g_zeigeArt != ExplorerArt::Tag && g_zeigeArt != ExplorerArt::Knochen && g_zeigeArt != ExplorerArt::Sequenz &&
                      g_zeigeArt != ExplorerArt::Griff && g_zeigeArt != ExplorerArt::Ebene;
    for (std::size_t r = 0; r < g_zeilen.size(); ++r) {
        if (g_zeilen[r].art != g_zeigeArt || (!kopf && g_zeilen[r].index != g_zeigeIndex)) { continue; }
        const float y = static_cast<float>(r) * zeileH;
        const float sicht = ImGui::GetWindowHeight();
        if (kopf || g_zeigeArt == ExplorerArt::Ebene) {
            ImGui::SetScrollY(y);
        } else if (y < ImGui::GetScrollY() + zeileH || y > ImGui::GetScrollY() + sicht - zeileH * 4.0F) {
            ImGui::SetScrollY(std::max(0.0F, y - sicht * 0.35F));
        }
        return;
    }
}

// Die Sortierung der Tabelle (Klick auf einen Spaltenkopf) und g_z.sortierung im Gleichschritt halten.
void sortierungAbgleichen() {
    ImGuiTable* tab = ImGui::GetCurrentTable();
    ImGuiTableSortSpecs* sp = ImGui::TableGetSortSpecs();
    if (tab == nullptr || sp == nullptr) { return; }
    int ausTabelle = 0;
    if (sp->SpecsCount > 0) { ausTabelle = sp->Specs[0].ColumnIndex == 1 ? 3 : (sp->Specs[0].SortDirection == ImGuiSortDirection_Descending ? 2 : 1); }
    if (sp->SpecsDirty) {
        g_z.sortierung = ausTabelle;
        sp->SpecsDirty = false;
    } else if (ausTabelle != g_z.sortierung) {   // von aussen gesetzt (Vorgabe, Selbsttest)
        if (g_z.sortierung == 0) {
            for (int c = 0; c < tab->ColumnsCount; ++c) { tab->Columns[c].SortOrder = -1; }
            tab->IsSortSpecsDirty = true;
        } else {
            ImGui::TableSetColumnSortDirection(g_z.sortierung == 3 ? 1 : 0, g_z.sortierung == 2 ? ImGuiSortDirection_Descending : ImGuiSortDirection_Ascending, false);
        }
    }
}

struct FilterArt {
    ExplorerFilter art;
    Str text;
    UiIcon icon;
};

const FilterArt* filterArten() {
    static const FilterArt k[kFilterZahl] = {
        {ExplorerFilter::Flaechen, Str::MbXArtFlaechen, UiIcon::MbFlaeche},   {ExplorerFilter::Kappen, Str::MbXArtKappen, UiIcon::MbKappe},
        {ExplorerFilter::Tags, Str::MbXArtTags, UiIcon::MbTag},               {ExplorerFilter::Knochen, Str::MbXArtKnochen, UiIcon::MbKnochen},
        {ExplorerFilter::Sequenzen, Str::MbXArtSequenzen, UiIcon::MbSequenz}, {ExplorerFilter::Anbauten, Str::MbXArtAnbauten, UiIcon::MbAnbau},
        {ExplorerFilter::Griffe, Str::MbXArtGriffe, UiIcon::MbRig},
    };
    return k;
}

void filterSchalten(int art, bool nurDiese) {
    if (nurDiese) {
        for (int k = 0; k < kFilterZahl; ++k) { g_z.filterAus[k] = k != art; }
    } else {
        g_z.filterAus[art] = !g_z.filterAus[art];
    }
}

// Die zwei Knoepfe fuer die gewaehlte Sequenz (vorher ueber der eigenen Animationsliste): Namen kopieren, ins Skript.
void sequenzKnoepfe() {
    const int sel = g_app->animIndex;
    const bool hat = sel >= 0 && static_cast<std::size_t>(sel) < g_app->animList.size();
    if (iconKnopf(tr(Str::ActCopy), UiIcon::Duplizieren, tr(Str::HintCopy), false, hat)) { sequenzKopieren(sel); }
    ImGui::SameLine();
    // "Ins Skript" oeffnet sein eigenes Fenster (Ziel, Figur, Befehl, Vorschau) - der Model-Tab hat keinen Skriptbaum
    if (iconKnopf(tr(Str::ModelAnimToScript), UiIcon::Skript, tr(Str::MbInsSkriptHint), false, hat)) { modellInsSkriptOeffnen(sel); }
}

// Kopf des Explorers: Umschalter, Suche, Filter (wie 3ds Max eine Symbolreihe, wie Blender ein Ausklapper neben der Suche).
void zeichneKopf(bool blender) {
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{fs * 0.35F, 2.0F});
    if (reiterKnopf(tr(blender ? Str::MbXHierarchie : Str::MbNachHierarchie), g_z.art == 0)) { g_z.art = 0; }
    tipp(tr(Str::MbXHierarchieHint));
    ImGui::SameLine(0.0F, 2.0F);
    if (reiterKnopf(tr(blender ? Str::MbXSammlungen : Str::MbNachEbenen), g_z.art == 1)) { g_z.art = 1; }
    tipp(tr(Str::MbXEbenenHint));
    if (blender && !g_app->animList.empty()) {   // wie Blender: rechts in der Kopfzeile des Outliners
        const float b = ImGui::GetFrameHeight() * 2.0F + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SameLine();
        const float frei = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x;
        if (frei >= b) {
            ImGui::Dummy(ImVec2{frei - b, 1.0F});   // Abstandhalter: die zwei Knoepfe stehen am rechten Rand
            ImGui::SameLine();
            sequenzKnoepfe();
        } else {
            ImGui::NewLine();
            sequenzKnoepfe();
        }
    }
    ImGui::PopStyleVar();
    if (g_sucheFokus) {
        ImGui::SetKeyboardFocusHere();
        g_sucheFokus = false;
    }
    g_z.filterReihe = !blender;
    // Zeigt der Filter nur die Sequenzen, sucht das Feld Animationen ("Search animation...", wie die fruehere eigene Liste)
    bool nurSequenzen = !g_z.filterAus[static_cast<int>(ExplorerFilter::Sequenzen)];
    for (int k = 0; k < kFilterZahl; ++k) { nurSequenzen = nurSequenzen && (k == static_cast<int>(ExplorerFilter::Sequenzen) || g_z.filterAus[k]); }
    const char* suchHinweis = tr(nurSequenzen ? Str::ModelAnimSearch : Str::MbSuche);
    if (blender) {
        const float knopf = ImGui::GetFrameHeight();
        ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - knopf - ImGui::GetStyle().ItemSpacing.x, fs * 3.0F));
        ImGui::InputTextWithHint("##mtsuche", suchHinweis, t.suche, sizeof t.suche);
        ImGui::SameLine();
        bool gefiltert = g_z.knochenFlach || g_z.sortierung != 0;
        for (const bool f : g_z.filterAus) { gefiltert = gefiltert || f; }
        const char* filterName = tr(Str::MbXFilter);   // (nicht im Ortskatalog des Helfers: den Ausklapper gibt es nur wie Blender)
        if (iconKnopf(filterName, UiIcon::MbFilter, tr(Str::MbXFilterHint), gefiltert)) { ImGui::OpenPopup("##mtfilter"); }
        if (ui::popup("##mtfilter")) {
            for (int k = 0; k < kFilterZahl; ++k) {
                bool an = !g_z.filterAus[k];
                if (ImGui::Checkbox(tr(filterArten()[k].text), &an)) { filterSchalten(k, ImGui::GetIO().KeyCtrl); }
            }
            ImGui::Separator();
            ImGui::Checkbox(tr(Str::MbXKnochenFlach), &g_z.knochenFlach);
            bool sort = g_z.sortierung == 1 || g_z.sortierung == 2;
            if (ImGui::Checkbox(tr(Str::MbXSortName), &sort)) { g_z.sortierung = sort ? 1 : 0; }
            ImGui::Checkbox(tr(Str::MbXSpalteWaehlbarZeigen), &g_z.spalteWaehlbar);
            ImGui::Checkbox(tr(Str::MbXTorsoSekundaer), &g_z.torsoSekundaer);
            ImGui::EndPopup();
        }
        return;
    }
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##mtsuche", suchHinweis, t.suche, sizeof t.suche);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0F, ImGui::GetStyle().ItemSpacing.y});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{2.0F, 2.0F});
    for (int k = 0; k < kFilterZahl; ++k) {
        if (k > 0) { ImGui::SameLine(); }
        char hinweis[240];
        std::snprintf(hinweis, sizeof hinweis, tr(Str::MbXFilterArtHint), tr(filterArten()[k].text));
        // "Filter: Bones": die Art allein ("Bones") heisst schon ein Knopf der Werkzeugleiste - Selbsttest, Handbuch und Helfer
        // finden Knoepfe ueber ihre Beschriftung
        const std::string name = std::string(tr(Str::MbXFilter)) + ": " + tr(filterArten()[k].text);
        if (iconKnopf(name.c_str(), filterArten()[k].icon, hinweis, !g_z.filterAus[k], true, true)) { filterSchalten(k, ImGui::GetIO().KeyCtrl); }
    }
    if (!g_app->animList.empty()) {
        ImGui::SameLine(0.0F, fs * 0.6F);
        sequenzKnoepfe();
    }
    ImGui::PopStyleVar(2);
}

struct Aktion {
    Str text;
    Str hinweis;
    void (*tun)();
};

const std::vector<Aktion>& aktionen() {
    static const std::vector<Aktion> k = {
        {Str::MbXAktNachHierarchie, Str::MbXHierarchieHint, [] { g_z.art = 0; }},
        {Str::MbXAktNachEbenen, Str::MbXEbenenHint, [] { g_z.art = 1; }},
        {Str::MbXAktZuSequenzen, Str::MbXHierarchieHint, [] {
             g_z.filterAus[static_cast<int>(ExplorerFilter::Sequenzen)] = false;
             modellExplorerZeigen(ExplorerArt::KopfSequenzen, 0);
         }},
        {Str::MbXAktFilter, Str::MbXFilterHint, [] {
             for (bool& f : g_z.filterAus) { f = false; }
         }},
        {Str::MbXAktAuswahlFrieren, Str::MbXFrostHint, [] {
             const std::vector<int> wahl = mt().auswahl;
             for (const int i : wahl) { modellTeilEinfrieren(i, true, false); }
         }},
        {Str::MbXAktAuftauen, Str::MbXFrostHint, [] { modellAllesAuftauen(); }},
        {Str::MbXAktSeqKopieren, Str::HintCopy, [] { sequenzKopieren(g_app->animIndex); }},
        {Str::MbXAktSeqInsSkript, Str::MbInsSkriptHint, [] {
             if (g_app->animIndex >= 0) { modellInsSkriptOeffnen(g_app->animIndex); }
         }},
        {Str::EaAuswahlAuftauen, Str::EaAuswahlAuftauenHint, [] { modellEnt3aAuswahlAuftauen(); }},   // ENTSCHEID3A (B7)
    };
    return k;
}

// Der Text einer Aktion in "Alle Aktionen suchen". Die Sequenzen hiessen in der eigenen Liste "Animations" - die Aktion nennt
// auch dieses Wort, damit die Suche sie weiter findet.
std::string aktionText(const Aktion& a) {
    std::string s = tr(a.text);
    if (a.text == Str::MbXAktZuSequenzen) { s += std::string(" (") + tr(Str::ModelAnimations) + ")"; }
    return s;
}

}  // namespace

// --- Schnittstelle ----------------------------------------------------------------------------------------------------------------

ModellExplorerZustand& modellExplorer() { return g_z; }

Fix612cProbe& fix612cProbe() {
    static Fix612cProbe p;
    return p;
}

const char* modellExplorerTitel() { return tr(modellBlender() ? Str::MbXOutliner : Str::MbExplorer); }

void modellExplorerSucheFokus() { g_sucheFokus = true; }

// AP5 (X18): dieselbe Info wie der Tooltip der Zeile und "Info" im Rechtsklick - fuer die Gruppe "Auswahl" im rechten Feld
std::string modellExplorerInfo(ExplorerArt art, int index) {
    const modell::Inhalt& in = mt().dok.inhalt();
    switch (art) {
        case ExplorerArt::Flaeche: return index >= 0 && static_cast<std::size_t>(index) < in.teile.size() ? infoFlaeche(index) : std::string();
        case ExplorerArt::Tag: return index >= 0 && static_cast<std::size_t>(index) < in.teile.size() ? infoTag(index) : std::string();
        case ExplorerArt::Knochen: return index >= 0 && static_cast<std::size_t>(index) < g_app->anim.bones.size() ? infoKnochen(index) : std::string();
        case ExplorerArt::Sequenz: return index >= 0 && static_cast<std::size_t>(index) < g_app->animList.size() ? infoSequenz(index) : std::string();
        default: return {};
    }
}

void modellExplorerVorgabe() {
    g_z.art = 0;
    for (bool& f : g_z.filterAus) { f = false; }
    g_z.knochenFlach = false;
    g_z.sortierung = 0;
    g_z.torsoSekundaer = false;
    g_z.seqGruppe = 0;
    g_z.spalteWaehlbar = true;
    g_frostModell = "\x01";   // kein Modell heisst so: groessen() stellt Einfrieren und Knoten auf die Vorgabe
    groessen();
    g_app->modelDirty = true;
}

void modellExplorerAufklappen(ExplorerArt art, int index, bool offen) { setzeOffen(art, art == ExplorerArt::Ebene || art == ExplorerArt::Flaeche || art == ExplorerArt::Knochen ? index : 0, offen); }

void modellExplorerKnochenAlleAuf(bool offen) {
    setzeOffen(ExplorerArt::KopfKnochen, 0, offen || istOffen(ExplorerArt::KopfKnochen, 0, false));
    for (std::size_t b = 0; b < g_app->anim.bones.size(); ++b) {
        if (offen) {
            setzeOffen(ExplorerArt::Knochen, static_cast<int>(b), true);
        } else {
            g_offen.erase(schluessel(ExplorerArt::Knochen, static_cast<int>(b), -1));   // zurueck auf die Vorgabe (die ersten drei Ebenen offen)
        }
    }
}

void modellExplorerZeigen(ExplorerArt art, int index) {
    const modell::Inhalt& in = mt().dok.inhalt();
    setzeOffen(ExplorerArt::Modell, 0, true);
    switch (art) {
        case ExplorerArt::Flaeche:
            setzeOffen(ExplorerArt::KopfFlaechen, 0, true);
            if (index >= 0 && static_cast<std::size_t>(index) < in.teile.size()) {
                int schutz = 0;
                for (int p = in.teile[static_cast<std::size_t>(index)].eltern; p >= 0 && static_cast<std::size_t>(p) < in.teile.size() && schutz < 512; p = in.teile[static_cast<std::size_t>(p)].eltern) {
                    setzeOffen(ExplorerArt::Flaeche, p, true);
                    ++schutz;
                }
                setzeOffen(ExplorerArt::Ebene, static_cast<int>(istKappe(in.teile[static_cast<std::size_t>(index)]) ? ExplorerEbene::Kappen : ExplorerEbene::Koerper), true);
            }
            break;
        case ExplorerArt::Tag:
            setzeOffen(ExplorerArt::KopfTags, 0, true);
            setzeOffen(ExplorerArt::Ebene, static_cast<int>(ExplorerEbene::Tags), true);
            break;
        case ExplorerArt::Knochen:
            setzeOffen(ExplorerArt::KopfKnochen, 0, true);
            setzeOffen(ExplorerArt::Ebene, static_cast<int>(ExplorerEbene::Knochen), true);
            if (index >= 0 && static_cast<std::size_t>(index) < g_app->anim.bones.size()) {
                int schutz = 0;
                for (int p = g_app->anim.bones[static_cast<std::size_t>(index)].parent; p >= 0 && static_cast<std::size_t>(p) < g_app->anim.bones.size() && schutz < 256;
                     p = g_app->anim.bones[static_cast<std::size_t>(p)].parent) {
                    setzeOffen(ExplorerArt::Knochen, p, true);
                    ++schutz;
                }
            }
            break;
        case ExplorerArt::Griff: setzeOffen(ExplorerArt::KopfRig, 0, true); break;
        case ExplorerArt::Sequenz: setzeOffen(ExplorerArt::KopfSequenzen, 0, true); break;
        default: break;
    }
    g_zeigen = true;
    g_zeigeArt = art;
    g_zeigeIndex = index;
}

bool modellTeilVersteckt(int teil) {
    const ModellTab& t = mt();
    return teil >= 0 && static_cast<std::size_t>(teil) < t.verstecktTeil.size() && t.verstecktTeil[static_cast<std::size_t>(teil)] != 0;
}

bool modellKnochenVersteckt(int knochen) {
    const ModellTab& t = mt();
    return knochen >= 0 && static_cast<std::size_t>(knochen) < t.verstecktKnochen.size() && t.verstecktKnochen[static_cast<std::size_t>(knochen)] != 0;
}

void modellTeilAuge(int teil, bool solo) {
    ModellTab& t = mt();
    groessen();
    if (solo) {
        std::fill(t.verstecktTeil.begin(), t.verstecktTeil.end(), static_cast<char>(1));
        teilSetzen(t.verstecktTeil, teil, 0, true, 0);
    } else {
        teilSetzen(t.verstecktTeil, teil, modellTeilVersteckt(teil) ? 0 : 1, true, 0);
    }
    versteckGeaendert();
}

void modellKnochenAuge(int knochen, bool solo) {
    ModellTab& t = mt();
    groessen();
    if (solo) {
        std::fill(t.verstecktKnochen.begin(), t.verstecktKnochen.end(), static_cast<char>(1));
        knochenSetzen(t.verstecktKnochen, knochen, 0, true);
    } else {
        knochenSetzen(t.verstecktKnochen, knochen, modellKnochenVersteckt(knochen) ? 0 : 1, true);
    }
    versteckGeaendert();
}

void modellAllesEinblenden() {
    ModellTab& t = mt();
    t.verstecktTeil.assign(t.verstecktTeil.size(), 0);
    t.verstecktKnochen.assign(t.verstecktKnochen.size(), 0);
    for (ModellAnbau& a : t.anbauten) { a.versteckt = false; }
    versteckGeaendert();
}

void modellAuswahlAusblenden(bool nichtGewaehlte) {
    ModellTab& t = mt();
    groessen();
    for (std::size_t i = 0; i < t.verstecktTeil.size(); ++i) {
        const bool gewaehlt = std::find(t.auswahl.begin(), t.auswahl.end(), static_cast<int>(i)) != t.auswahl.end();
        if (gewaehlt != nichtGewaehlte) { t.verstecktTeil[i] = 1; }
    }
    if (!nichtGewaehlte) { t.auswahl.clear(); }   // Blender: H blendet die Auswahl aus, sie ist danach leer
    versteckGeaendert();
}

bool modellTeilEingefroren(int teil) { return teil >= 0 && static_cast<std::size_t>(teil) < g_z.frostTeil.size() && g_z.frostTeil[static_cast<std::size_t>(teil)] != 0; }

bool modellKnochenEingefroren(int knochen) {
    return knochen >= 0 && static_cast<std::size_t>(knochen) < g_z.frostKnochen.size() && g_z.frostKnochen[static_cast<std::size_t>(knochen)] != 0;
}

void modellTeilEinfrieren(int teil, bool an, bool samtKindern) {
    groessen();
    teilSetzen(g_z.frostTeil, teil, an ? 1 : 0, samtKindern, 0);
    auswahlOhneFrost();
}

void modellKnochenEinfrieren(int knochen, bool an, bool samtKindern) {
    groessen();
    knochenSetzen(g_z.frostKnochen, knochen, an ? 1 : 0, samtKindern);
    auswahlOhneFrost();
}

void modellEbeneEinfrieren(ExplorerEbene e, bool an) {
    if (gruppeVon(e) != Gruppe::Keine) { gruppeFrost(gruppeVon(e), an); }
}

void modellEbeneAuge(ExplorerEbene e, bool zeigen) {
    if (gruppeVon(e) != Gruppe::Keine) { gruppeAuge(gruppeVon(e), zeigen); }
}

void modellAllesAuftauen() {
    groessen();
    std::fill(g_z.frostTeil.begin(), g_z.frostTeil.end(), static_cast<char>(0));
    std::fill(g_z.frostKnochen.begin(), g_z.frostKnochen.end(), static_cast<char>(0));
    g_z.frostAnbauten = false;
    g_app->modelDirty = true;
}

bool modellFlaecheWaehlbar(int teil, bool melden) {
    if (!modellTeilEingefroren(teil)) { return true; }
    if (melden) { meldeFrost(mt().dok.inhalt().teile[static_cast<std::size_t>(teil)].name); }
    return false;
}

int modellExplorerAktionZahl() { return static_cast<int>(aktionen().size()); }
const char* modellExplorerAktionText(int i) {
    static std::string puffer;
    puffer = aktionText(aktionen()[static_cast<std::size_t>(i)]);
    return puffer.c_str();
}
void modellExplorerAktionAusloesen(int i) { aktionen()[static_cast<std::size_t>(i)].tun(); }

bool modellExplorerAktionenSuche(const std::string& such) {
    bool ausgeloest = false;
    for (const Aktion& a : aktionen()) {
        const std::string text = aktionText(a);
        if (!passt(text, such)) { continue; }
        ImGui::PushID(&a);
        if (modellAktionZeile(text.c_str()) && !ausgeloest) {
            a.tun();
            ausgeloest = true;
        }
        tipp(tr(a.hinweis));
        ImGui::PopID();
    }
    return ausgeloest;
}

void drawModellExplorer(float hoehe) {
    ModellTab& t = mt();
    ImGui::BeginChild("mtexplorer", ImVec2{-1.0F, hoehe}, ImGuiChildFlags_Borders);
    g_z.lage.clear();
    g_z.tooltip.clear();
    g_z.koepfe = false;
    g_z.filterReihe = false;
    g_z.fussEbenen = false;
    g_z.fussHinweis = false;
    g_z.zeilen = 0;
    if (g_app->model.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        cjkTextDisabled("%s", tr(Str::ModelNoneH2));
        ImGui::PopTextWrapPos();
        // H2: ohne Modell zeigt ein Knopf den Weg (Tour "Ein Figurenmodell ansehen" ab Schritt 2).
        if (ImGui::SmallButton(tr(Str::EmptyShowHow))) { (void)tourStarten("k-modell", 1); }
        ImGui::EndChild();
        return;
    }
    modellAnzeigeAktualisieren();
    modellKlickPruefen();
    groessen();
    t.symbolZaehler = 0;
    t.augenZaehler = 0;
    const bool blender = modellBlender();
    if (g_app->animScrollen) {   // die gewaehlte Sequenz ins Bild (Laden, NPC-Browser, Selbsttest)
        modellExplorerZeigen(ExplorerArt::Sequenz, g_app->animIndex);
        g_app->animScrollen = false;
    }
    zeichneKopf(blender);
    const std::string such = klein(t.suche);
    baueZeilen(such);
    g_z.zeilen = static_cast<int>(g_zeilen.size());

    Masse m;
    m.blender = blender;
    m.fs = ImGui::GetFontSize();
    m.zeileH = std::floor(m.fs * 1.3F);
    m.pfeilB = std::floor(m.fs * 0.85F);
    m.augeB = std::floor(m.fs * 1.1F);
    m.symB = std::floor(m.fs * 1.15F);
    m.icon = std::floor(m.fs * 0.95F);
    m.merken = GImGui->TestEngineHookItems;
    m.g2 = g_app->showCaps ? modell::mitKappen(t.dok.inhalt(), t.g2) : t.g2;
    // Einzug je Ebene: eine Zeichenhoehe (Entwurf); gestaucht, wenn der tiefste Knoten sonst keinen Platz fuer seinen Namen haette
    int tiefste = 0;
    for (const Zeile& r : g_zeilen) { tiefste = std::max(tiefste, r.tiefe); }
    m.einzug = m.fs;
    if (g_nameBreite > 0.0F && tiefste > 0) {
        const float fest = m.pfeilB + (blender ? 0.0F : m.augeB) + m.symB;
        m.einzug = std::clamp((g_nameBreite - fest - m.fs * 5.0F) / static_cast<float>(tiefste), m.fs * 0.45F, m.fs);
    }
    m.einzug = std::floor(m.einzug);
    g_z.einzug = m.einzug;

    // Fusszeilen (Entwurf): der Satz zu Auge und Schneeflocke (nach Ebenen) und der Bedienhinweis - nur, wenn Platz ist
    const float breite = ImGui::GetContentRegionAvail().x;
    const char* fussEbenen = tr(blender ? Str::MbXEbenenFussBlender : Str::MbXEbenenFuss);
    const float abstand = ImGui::GetStyle().ItemSpacing.y;
    // (Platz nur, wenn dem Baum genug Zeilen bleiben: der Bedienhinweis ab 12, der Satz zu den Ebenen ab 16 Zeilen; beides steht
    // auch in den Tooltips der Schalter)
    const float frei = ImGui::GetContentRegionAvail().y;
    const float hHinweis = cjkTextSize(tr(Str::MbXFuss), nullptr, false, breite).y + abstand;
    const float hEbenen = cjkTextSize(fussEbenen, nullptr, false, breite).y + abstand;
    const bool mitFuss = frei - hHinweis >= m.zeileH * 12.0F;
    const bool mitEbenen = g_z.art == 1 && mitFuss && frei - hHinweis - hEbenen >= m.zeileH * 16.0F;
    const float fussH = (mitFuss ? hHinweis : 0.0F) + (mitEbenen ? hEbenen : 0.0F);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{std::floor(m.fs * 0.2F), 0.0F});
    // Kopf und Linien der Tabelle in den Farben des Farbschemas (ImGuis Vorgabe ist dunkel - im hellen Schema ein schwarzer Balken)
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImGui::GetStyleColorVec4(ImGuiCol_Border));
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImGui::GetStyleColorVec4(ImGuiCol_Border));
    const int spalten = blender ? (g_z.spalteWaehlbar ? 3 : 2) : 2;
    ImGuiTableFlags flaggen = ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX;
    if (!blender) { flaggen |= ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortTristate | ImGuiTableFlags_BordersInnerV; }
    if (ImGui::BeginTable(blender ? "mtbaum##b" : "mtbaum", spalten, flaggen, ImVec2{0.0F, -fussH - 1.0F})) {
        if (blender) {
            ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##auge", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, m.augeB);
            if (g_z.spalteWaehlbar) { ImGui::TableSetupColumn("##waehlbar", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, m.augeB); }
        } else {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(tr(Str::MbXSpalteName), ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_PreferSortAscending);
            ImGui::TableSetupColumn("##frostkopf", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, std::floor(m.fs * 1.9F));
            sortierungAbgleichen();
            // Spaltenkoepfe wie der Scene Explorer: "Name" und "Frozen" (als Schneeflocke, der Text steht im Tooltip)
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableSetColumnIndex(0);
            ImGui::TableHeader(tr(Str::MbXSpalteName));
            if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), "##mtkopfname"); }
            tipp(tr(Str::MbXSpalteNameHint));
            ImGui::TableSetColumnIndex(1);
            const ImVec2 k0 = ImGui::GetCursorScreenPos();
            ImGui::TableHeader("##frostkopf");
            if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), "##frostkopf"); }
            tipp((std::string(tr(Str::MbXSpalteFrost)) + "\n" + tr(Str::MbXSpalteFrostHint)).c_str());
            uiIcon(ImGui::GetWindowDrawList(), UiIcon::MbFrost, ImVec2{k0.x, std::floor(k0.y + (ImGui::GetTextLineHeight() - m.icon) * 0.5F)}, m.icon, farbeVon(UiIcon::MbFrost, 0.85F));
            g_z.koepfe = true;
        }
        rolleZurZeile(m.zeileH);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ImGui::GetStyle().ItemSpacing.x, 0.0F});
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(g_zeilen.size()), m.zeileH);
        if (m.merken) {
            // Selbsttest: wie der alte Baum alle Zeilen ausser den Sequenzen bauen (das Elementverzeichnis findet sie dann auch
            // ausserhalb des Sichtbaren); im Alltag nur die sichtbaren.
            int ohneSequenzen = static_cast<int>(g_zeilen.size());
            for (std::size_t r = 0; r < g_zeilen.size(); ++r) {
                if (g_zeilen[r].art == ExplorerArt::Sequenz) {
                    ohneSequenzen = static_cast<int>(r);
                    break;
                }
            }
            if (ohneSequenzen > 0) { clipper.IncludeItemsByIndex(0, ohneSequenzen); }
        }
        // FIX612C (P3-7): ein Befehl im Rechtsklick einer Zeile kann die Liste kuerzer machen (Anbau loesen, Rig nur FK) - dann
        // die Schleife verlassen, die Zeilen dahinter truegen Nummern hinter dem Ende; das naechste Bild baut den Baum neu
        const std::size_t anbauten = t.anbauten.size();
        const int griffe = modellRigExplorerZahl();
        bool veraltet = false;
        while (!veraltet && clipper.Step()) {
            for (int r = clipper.DisplayStart; r < clipper.DisplayEnd && !veraltet; ++r) {
                zeichneZeile(g_zeilen[static_cast<std::size_t>(r)], m);
                veraltet = t.anbauten.size() != anbauten || modellRigExplorerZahl() != griffe;
            }
        }
        if (veraltet) {
            clipper.End();
            g_app->modelDirty = true;
        }
        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    if (mitFuss) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushTextWrapPos(0.0F);
        if (mitEbenen) {
            cjkTextWrapped("%s", fussEbenen);
            g_z.fussEbenen = true;
        }
        cjkTextWrapped("%s", tr(Str::MbXFuss));
        g_z.fussHinweis = true;
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
    anbauFenster();
    infoFenster();
    ImGui::EndChild();
}


// AP11 (Modelle in Tabs): Einfrieren, offene Knoten und der Anker der Bereichswahl gehoeren dem Modell; Art, Filter und Sortierung
// des Baums gelten fuer alle Reiter (wie die Anzeige-Schalter). Fenster und Menues des alten Modells gehen zu.
void modellExplorerTauschen(ModellFachZeiger& f) {
    struct Fach : ModellFach {
        std::vector<char> frostTeil;
        std::vector<char> frostKnochen;
        bool frostAnbauten = false;
        std::unordered_map<int, bool> offen;
        std::string frostModell;
        int anker = -1;
    };
    if (!f) { f = std::make_unique<Fach>(); }
    Fach& x = static_cast<Fach&>(*f);
    std::swap(g_z.frostTeil, x.frostTeil);
    std::swap(g_z.frostKnochen, x.frostKnochen);
    std::swap(g_z.frostAnbauten, x.frostAnbauten);
    std::swap(g_offen, x.offen);
    std::swap(g_frostModell, x.frostModell);
    std::swap(g_anker, x.anker);
    g_zeilen.clear();
    g_z.lage.clear();
    g_z.infoOffen = false;
    g_z.infoText.clear();
    g_infoOeffnen = false;
    g_anbauOeffnen = false;
    g_anbauZiel.clear();
    g_zeigen = false;
}

}  // namespace bhed::gui
