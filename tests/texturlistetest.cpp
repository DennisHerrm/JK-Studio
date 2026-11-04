// texturlistetest.cpp - rc609 K: Texturen der Karte (src/karte/texturliste.cpp).
//
// Eine Karte mit Brushes und Patches (worldspawn und func_group) als Text:
//   - die Liste zaehlt wie die .map (unabhaengig aus dem Text gezaehlt), Gross/klein zusammen
//   - Ersetzen trifft alle Brush-Seiten und Patches, ein Rueckgaengig-Schritt; Rueckgaengig gibt
//     die .map Byte fuer Byte zurueck, Wiederholen wieder den ersetzten Stand
//   - gleiche Weltgroesse: Quake-Texdef scale * alt/neu, shift * neu/alt (q3map2 map.cpp
//     QuakeTextureVecs, surface.cpp:725); Brush Primitives behalten die Werte der .map, ohne
//     den Schalter rechnet setzeShader sie um (removeScale/addScale wie NRC)
//   - Umfang nur sichtbare / nur Auswahl, Waehlen, Ordner-Zuordnung mit Luecke
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texturliste.h"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_gut = 0;
void pruefe(bool ok, const std::string& was) {
    if (ok) {
        ++g_gut;
    } else {
        ++g_fehler;
        std::printf("FEHLER: %s\n", was.c_str());
    }
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// k_alt: 64 x 64, k_neu: 128 x 256, sonst 64 x 64
class Groessen final : public TexturGroessen {
public:
    std::pair<std::size_t, std::size_t> groesse(const std::string& shader) const override {
        if (klein(shader).rfind("textures/k_neu/", 0) == 0) { return {128, 256}; }
        return {64, 64};
    }
};

// Die sechs Ebenen eines Kastens (wie raumKarte im Selbsttest), td = Rest der Zeile nach den Punkten
std::string kasten(int x0, int y0, int z0, int x1, int y1, int z1, const std::vector<std::string>& td) {
    char z[6][160];
    std::snprintf(z[0], sizeof(z[0]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) ", x1, y1, z1, x1, y0, z1, x0, y1, z1);
    std::snprintf(z[1], sizeof(z[1]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) ", x1, y1, z1, x0, y1, z1, x1, y1, z0);
    std::snprintf(z[2], sizeof(z[2]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) ", x1, y1, z1, x1, y1, z0, x1, y0, z1);
    std::snprintf(z[3], sizeof(z[3]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) ", x0, y0, z0, x1, y0, z0, x0, y1, z0);
    std::snprintf(z[4], sizeof(z[4]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) ", x0, y0, z0, x0, y0, z1, x1, y0, z0);
    std::snprintf(z[5], sizeof(z[5]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) ", x0, y0, z0, x0, y1, z0, x0, y0, z1);
    std::string t = "{\n";
    for (int i = 0; i < 6; ++i) { t += std::string(z[i]) + td[static_cast<std::size_t>(i)] + "\n"; }
    return t + "}\n";
}
std::vector<std::string> sechs(const std::string& s) { return std::vector<std::string>(6, s); }

std::string patch(const std::string& shader, int x) {
    char t[600];
    std::snprintf(t, sizeof(t),
                  "{\npatchDef2\n{\n%s\n( 3 3 0 0 0 )\n(\n( ( %d 0 0 0 0 ) ( %d 64 32 0 0.5 ) ( %d 128 0 0 1 ) )\n"
                  "( ( %d 0 0 0.5 0 ) ( %d 64 32 0.5 0.5 ) ( %d 128 0 0.5 1 ) )\n( ( %d 0 0 1 0 ) ( %d 64 32 1 0.5 ) ( %d 128 0 1 1 ) )\n)\n}\n}\n",
                  shader.c_str(), x, x, x, x + 64, x + 64, x + 64, x + 128, x + 128, x + 128);
    return t;
}

std::string testKarte() {
    std::vector<std::string> b = sechs("k_alt/b 0 0 0 0.5 0.5 0 0 0");
    b[3] = "k_alt/a 0 0 0 0.5 0.5 0 0 0";
    return "// entity 0\n{\n\"classname\" \"worldspawn\"\n" + kasten(0, 0, 0, 64, 64, 64, sechs("k_alt/a 16 8 0 0.5 0.5 0 0 0")) +
           kasten(128, 0, 0, 192, 64, 64, b) + patch("k_alt/a", 256) + "}\n// entity 1\n{\n\"classname\" \"func_group\"\n" +
           kasten(0, 128, 0, 64, 192, 64, sechs("K_ALT/A 0 0 0 0.5 0.5 0 0 0")) + patch("k_alt/c", 512) + "}\n";
}

// Unabhaengig aus dem Text: Shader (klein, mit textures/) -> {Seiten, Patches}
std::map<std::string, std::pair<int, int>> textZaehlen(const std::string& text) {
    std::map<std::string, std::pair<int, int>> m;
    std::size_t p = 0;
    bool nachPatch = false;
    int klammer = 0;
    while (p < text.size()) {
        std::size_t q = text.find('\n', p);
        if (q == std::string::npos) { q = text.size(); }
        const std::string z = text.substr(p, q - p);
        p = q + 1;
        if (z == "patchDef2") {
            nachPatch = true;
            klammer = 0;
            continue;
        }
        if (nachPatch) {
            if (z == "{") {
                ++klammer;
                continue;
            }
            if (klammer == 1) {
                ++m["textures/" + klein(z)].second;
                nachPatch = false;
            }
            continue;
        }
        if (z.rfind("( ", 0) == 0 && z.rfind("( (", 0) != 0 && z.find(") ( ") != std::string::npos) {   // ( p ) ( p ) ( p ) shader ...
            std::size_t e = 0;
            for (int i = 0; i < 3; ++i) { e = z.find(')', e) + 1; }
            const std::size_t a = z.find_first_not_of(' ', e);
            ++m["textures/" + klein(z.substr(a, z.find(' ', a) - a))].first;
        }
    }
    return m;
}

const TexturZeile* finde(const std::vector<TexturZeile>& v, const std::string& s) {
    for (const TexturZeile& t : v) {
        if (klein(t.shader) == klein(s)) { return &t; }
    }
    return nullptr;
}

void listeUndErsetzen() {
    Groessen gr;
    Karte k;
    k.kontext.texturen = &gr;
    const LeseErgebnis le = karteLesen(testKarte(), k);
    pruefe(le.vollstaendig, "Testkarte gelesen");
    const std::string original = karteSchreiben(k);

    // Liste = .map
    const std::vector<TexturZeile> liste = texturListe(k, TexturUmfang::Alle);
    const auto soll = textZaehlen(original);
    bool gleich = liste.size() == soll.size();
    for (const TexturZeile& t : liste) {
        const auto it = soll.find(klein(t.shader));
        gleich = gleich && it != soll.end() && it->second.first == t.seiten && it->second.second == t.patches;
        std::printf("  %-20s Seiten %2d Brushes %d Patches %d\n", t.shader.c_str(), t.seiten, t.pinsel, t.patches);
    }
    for (const auto& [name, w] : soll) { std::printf("  .map: %s %d/%d\n", name.c_str(), w.first, w.second); }
    pruefe(gleich && soll.size() == 3, "Liste: Zahlen wie in der .map (3 Shader)");
    const TexturZeile* a = finde(liste, "textures/k_alt/a");
    pruefe(a != nullptr && a->seiten == 13 && a->pinsel == 3 && a->patches == 1 && a->shader == "textures/k_alt/a",
           "Liste: k_alt/a 13 Seiten, 3 Brushes, 1 Patch, K_ALT/A mitgezaehlt, erste Schreibweise");

    // Waehlen
    const auto [ws, wp] = texturListeWaehlen(k, "TEXTURES/k_alt/a", TexturUmfang::Alle, false);
    pruefe(ws == 13 && wp == 1 && k.auswahl.flaechen().size() == 13 && k.auswahl.anzahl() == 1, "Waehlen: 13 Brush-Seiten und 1 Patch");
    const auto [ws2, wp2] = texturListeWaehlen(k, "textures/k_alt/c", TexturUmfang::Alle, true);
    pruefe(ws2 == 0 && wp2 == 1 && k.auswahl.flaechen().size() == 13 && k.auswahl.anzahl() == 2, "Waehlen mit Strg: dazu");
    k.auswahl.leeren();
    k.auswahl.flaechenLeeren();

    // Ersetzen, ganze Karte, Texdef bleibt
    const std::size_t schritte = k.undo.abschluesse();
    const ErsetzenZahl n = texturListeErsetzen(k, {{"textures/k_alt/a", "textures/k_neu/a"}}, TexturUmfang::Alle, false);
    const std::string ersetzt = karteSchreiben(k);
    const auto nach = textZaehlen(ersetzt);
    pruefe(n.seiten == 13 && n.patches == 1 && k.undo.abschluesse() == schritte + 1 && k.undo.undoName().rfind("textureListReplace", 0) == 0,
           "Ersetzen: 13 Seiten, 1 Patch, ein Rueckgaengig-Schritt");
    pruefe(nach.count("textures/k_alt/a") == 0 && nach.at("textures/k_neu/a") == std::make_pair(13, 1) && nach.at("textures/k_alt/b").first == 5,
           "Ersetzen: in der .map kein k_alt/a mehr (Brushes und Patches), k_alt/b bleibt");
    pruefe(ersetzt.find("k_neu/a 16 8 0 0.5 0.5 0 0 0") != std::string::npos, "Ersetzen ohne Weltgroesse: Texdef bleibt (wie Find / Replace)");
    k.undo.undo();
    pruefe(karteSchreiben(k) == original, "Rueckgaengig: .map Byte fuer Byte wie vorher");
    k.undo.redo();
    pruefe(karteSchreiben(k) == ersetzt, "Wiederholen: wieder ersetzt");
    k.undo.undo();

    // Gleiche Weltgroesse (64x64 -> 128x256)
    (void)texturListeErsetzen(k, {{"textures/k_alt/a", "textures/k_neu/a"}}, TexturUmfang::Alle, true);
    const std::string welt = karteSchreiben(k);
    pruefe(welt.find("k_neu/a 32 32 0 0.25 0.125 0 0 0") != std::string::npos,
           "Weltgroesse: scale 0.5*64/128 = 0.25, 0.5*64/256 = 0.125; shift 16*2 = 32, 8*4 = 32");
    pruefe(welt.find("( ( 256 0 0 0 0 )") != std::string::npos || welt.find("( 256 0 0 0 0 )") != std::string::npos,
           "Weltgroesse: Patch-Texturkoordinaten bleiben");
    k.undo.undo();
    pruefe(karteSchreiben(k) == original, "Rueckgaengig nach Weltgroesse: Byte fuer Byte");

    // Nur sichtbare: der Brush der func_group versteckt
    std::shared_ptr<Primitiv> gruppenPinsel;
    std::shared_ptr<Entitaet> gruppe;
    for (const auto& e : k.entitaeten()) {
        if (e->wert("classname") == "func_group") {
            gruppe = e;
            for (const auto& p : e->kinder()) {
                if (p->art() == Primitiv::Art::Pinsel) { gruppenPinsel = p; }
            }
        }
    }
    pruefe(gruppenPinsel != nullptr, "func_group-Brush gefunden");
    if (gruppenPinsel) {
        gruppenPinsel->versteckt = true;
        const TexturZeile* s = finde(texturListe(k, TexturUmfang::Sichtbar), "textures/k_alt/a");
        pruefe(s != nullptr && s->seiten == 7 && s->pinsel == 2, "Nur sichtbare: der versteckte Brush zaehlt nicht (7 Seiten, 2 Brushes)");
        const ErsetzenZahl v = texturListeErsetzen(k, {{"textures/k_alt/a", "textures/k_neu/a"}}, TexturUmfang::Sichtbar, false);
        const auto nv = textZaehlen(karteSchreiben(k));
        pruefe(v.seiten == 7 && v.patches == 1 && nv.at("textures/k_alt/a").first == 6, "Nur sichtbare: der versteckte Brush behaelt k_alt/a");
        k.undo.undo();
        gruppenPinsel->versteckt = false;
    }

    // Nur Auswahl: der Patch der func_group und eine Seite des zweiten Brushes
    std::shared_ptr<Pinsel> zweiter;
    int nr = 0;
    for (const auto& p : k.welt()->kinder()) {
        if (p->art() == Primitiv::Art::Pinsel && nr++ == 1) { zweiter = std::static_pointer_cast<Pinsel>(p); }
    }
    if (zweiter && gruppe) {
        for (const auto& p : gruppe->kinder()) {
            if (p->art() == Primitiv::Art::Patch) { k.auswahl.waehlen(gruppe, p); }
        }
        k.auswahl.flaecheWaehlen(zweiter, zweiter->flaechen()[0]);
        const std::vector<TexturZeile> aus = texturListe(k, TexturUmfang::Auswahl);
        const TexturZeile* b = finde(aus, "textures/k_alt/b");
        const TexturZeile* c = finde(aus, "textures/k_alt/c");
        pruefe(aus.size() == 2 && b != nullptr && b->seiten == 1 && c != nullptr && c->patches == 1, "Nur Auswahl: eine Seite k_alt/b, ein Patch k_alt/c");
        k.auswahl.leeren();
        k.auswahl.flaechenLeeren();
        k.auswahl.waehlen(k.welt(), zweiter);   // der ganze Brush
        const TexturZeile* ab = finde(texturListe(k, TexturUmfang::Auswahl), "textures/k_alt/a");
        pruefe(ab != nullptr && ab->seiten == 1 && ab->pinsel == 1, "Nur Auswahl: ganzer Brush mit seiner k_alt/a-Seite");
        k.auswahl.leeren();
    }
    pruefe(zweiter != nullptr, "zweiter Brush gefunden");

    // Ordner ersetzen mit Luecke: k_neu hat a und b, c fehlt
    const auto da = [](const std::string& s) { return klein(s) == "textures/k_neu/a" || klein(s) == "textures/k_neu/b"; };
    pruefe(texturOrdnerName("k_alt") == "textures/k_alt/" && texturOrdnerName("textures\\k_alt\\") == "textures/k_alt/" &&
               texturOrdnerName("TEXTURES/x") == "TEXTURES/x/",
           "Ordnernamen: mit textures/ und '/'");
    const std::vector<OrdnerZuordnung> zu = texturOrdnerZuordnung(texturListe(k, TexturUmfang::Alle), "k_alt", "textures/k_neu/", da);
    bool luecke = zu.size() == 3;
    for (const OrdnerZuordnung& o : zu) {
        luecke = luecke && (klein(o.alt) == "textures/k_alt/c") == o.fehlt && klein(o.neu) == "textures/k_neu/" + klein(o.alt).substr(15);
    }
    pruefe(luecke, "Ordner-Zuordnung: a und b gefunden, c ist eine Luecke");
    std::vector<TexturErsatz> paare;
    for (const OrdnerZuordnung& o : zu) {
        if (!o.fehlt) { paare.push_back({o.alt, o.neu}); }
    }
    const std::size_t vor = k.undo.abschluesse();
    const ErsetzenZahl o = texturListeErsetzen(k, paare, TexturUmfang::Alle, false);
    const auto no = textZaehlen(karteSchreiben(k));
    pruefe(o.seiten == 18 && o.patches == 1 && k.undo.abschluesse() == vor + 1 && no.count("textures/k_alt/a") == 0 && no.count("textures/k_alt/b") == 0 &&
               no.at("textures/k_alt/c").second == 1,
           "Ordner ersetzen: 18 Seiten, 1 Patch in einem Schritt, die Luecke k_alt/c bleibt");
    k.undo.undo();
    pruefe(karteSchreiben(k) == original, "Rueckgaengig nach Ordner ersetzen: Byte fuer Byte");
    const std::size_t nachUndo = k.undo.abschluesse();   // Rueckgaengig zaehlt selbst mit
    const std::size_t stapel = k.undo.undoAnzahl();
    pruefe(texturListeErsetzen(k, {{"textures/k_alt/x", "textures/k_neu/x"}}, TexturUmfang::Alle, false).seiten == 0 &&
               k.undo.abschluesse() == nachUndo && k.undo.undoAnzahl() == stapel,
           "ohne Treffer kein Rueckgaengig-Schritt");
    pruefe(texturListeErsetzen(k, {{"textures/k_alt/a", "k_neu/a"}}, TexturUmfang::Alle, false).seiten == 0, "ungueltiger Name (ohne textures/) uebergangen");
}

// Brush Primitives: die .map haelt normierte Werte
void brushPrimitives() {
    Groessen gr;
    Karte k;
    k.kontext.texturen = &gr;
    const char* f[6][3] = {{"64 64 64", "64 0 64", "0 64 64"}, {"64 64 64", "0 64 64", "64 64 0"}, {"64 64 64", "64 64 0", "64 0 64"},
                           {"0 0 0", "64 0 0", "0 64 0"},       {"0 0 0", "0 0 64", "64 0 0"},       {"0 0 0", "0 64 0", "0 0 64"}};
    std::string t = "// entity 0\n{\n\"classname\" \"worldspawn\"\n// brush 0\n{\nbrushDef\n{\n";
    for (const auto& z : f) {
        t += std::string("( ") + z[0] + " ) ( " + z[1] + " ) ( " + z[2] + " ) ( ( 0.03125 0 0.25 ) ( 0 0.03125 0.5 ) ) k_alt/a 0 0 0\n";
    }
    t += "}\n}\n}\n";
    const LeseErgebnis le = karteLesen(t, k);
    pruefe(le.vollstaendig && k.kontext.format == PinselFormat::Quake3BP, "BP-Karte gelesen");
    const std::string original = karteSchreiben(k);
    (void)texturListeErsetzen(k, {{"textures/k_alt/a", "textures/k_neu/a"}}, TexturUmfang::Alle, true);
    const std::string welt = karteSchreiben(k);
    pruefe(welt.find("( ( 0.03125 0 0.25 ) ( 0 0.03125 0.5 ) ) k_neu/a") != std::string::npos, "BP mit Weltgroesse: Werte der .map bleiben");
    k.undo.undo();
    pruefe(karteSchreiben(k) == original, "BP: Rueckgaengig Byte fuer Byte");
    (void)texturListeErsetzen(k, {{"textures/k_alt/a", "textures/k_neu/a"}}, TexturUmfang::Alle, false);
    const std::string ohne = karteSchreiben(k);
    pruefe(ohne.find("( ( 0.015625 0 0.125 ) ( 0 0.0078125 0.125 ) ) k_neu/a") != std::string::npos,
           "BP ohne Weltgroesse: wie NRC (removeScale 64, addScale 128 x 256)");
    if (ohne.find("k_neu/a") != std::string::npos) {
        const std::size_t p = ohne.find("( (");
        std::printf("  BP ohne: %s\n", ohne.substr(p, ohne.find('\n', p) - p).c_str());
    }
}

}  // namespace

int main() {
    listeUndErsetzen();
    brushPrimitives();
    std::printf("texturliste: %d gut, %d FEHLER\n", g_gut, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
