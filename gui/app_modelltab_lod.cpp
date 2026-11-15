// app_modelltab_lod.cpp - CW-MODELL (15.11.2026, Clone-Wars-Kette Teil 3): Detailstufen im Model-Tab
//
// Liste des Bruders, Abschnitte 2, 7, 8: drei Detailstufen fuer Figuren, Waffen und Ausruestung; .md3 als name_1.md3 / name_2.md3,
// .glm in der Datei; Ecken und Teilflaechen je Stufe mit Warnung ueber den Grenzen; mit der Clone-Wars-Kette der Richtwert "niedrigste
// Stufe nur wenige hundert Ecken". Das Rechnen steht in bhed/modell/detailstufen.h (meshoptimizer v1.3, MIT).
//
//   Gruppe     wie 3ds Max ProOptimizer / MultiRes im Reiter Aendern, wie Blender Decimate im Reiter Modifier (Bereich Modellieren,
//              unter dem Modifier-Stapel): Vorlage, Zahl der Stufen, Anteil je Stufe (mit Ziel in Ecken), "Erzeugen" als ein
//              Rueckgaengig-Schritt, danach die Tabelle vorher/nachher; ein Klick auf eine Zeile zeigt die Stufe in der Ansicht
//   Anzeige    in "Detailstufe und Skin" (Ansehen/Pruefen) die Zahlen je Stufe mit den Grenzen des Zielspiels - bei einer .md3 auch
//              die Dateien name_1.md3 / name_2.md3, die neben ihr liegen
//   Speichern  eine .md3 mit erzeugten Stufen schreibt name_1.md3 (und name_2.md3) neben name.md3 (gui/app_modelltab_mb3.cpp);
//              eine .glm traegt die Stufen in der Datei
//   Finden     Modifier-Liste / Modifier hinzufuegen, Menue des Stapels, Rechtsklick der Ansicht, Aktionssuche, Tooltips
//
// Ab Werk wirkt nichts davon: ohne "Erzeugen" bleibt das Dokument, wie es war, und Speichern schreibt dieselben Bytes (ctest
// modell_cwvergleich). Was nur zur Clone-Wars-Kette gehoert (Richtwert, Warnungen danach), steht hinter dem Pruefprofil
// "Clone-Wars-Kette" (Einstellung OptimizerCloneWars, ab Werk aus).
#include "app_internal.h"
#include "ui_menue.h"
#include "textumbruch.h"

#include "bhed/modell/detailstufen.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/gewichte.h"
#include "bhed/sicherschreiben.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellLod g_lod;

ModellTab& mt() { return g_app->mtab; }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

std::string format(Str s, auto... a) {
    char b[900];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

void melden(const std::string& m) {
    mt().meldung = m;
    addStatus(m);
}

modell::Zielspiel zielspiel() { return static_cast<modell::Zielspiel>(mt().zielspiel); }

modell::ModellArt art() {
    const ModellLod& l = g_lod;
    if (l.vorlage >= 0 && l.vorlage < modell::kModellArten) { return static_cast<modell::ModellArt>(l.vorlage); }
    return modell::modellArtVon(mt().dok.inhalt(), g_app->modelPath);
}

int hoechsteStufen() { return mt().dok.inhalt().herkunft == modell::Herkunft::Md3 ? modell::kMd3StufenMax : modell::kGlmStufenMax; }

// Die Vorgabe der Anteile nach der Art, solange der Anwender sie nicht selbst gestellt hat ("Eigene").
void vorgabeAnwenden() {
    ModellLod& l = g_lod;
    if (l.vorlage == modell::kModellArten) { return; }
    const modell::StufenVorgabe v = modell::stufenVorgabe(art(), l.stufen);
    for (int i = 0; i < modell::kGlmStufenMax - 1; ++i) { l.anteil[i] = v.anteil[i]; }
}

modell::DetailOptionen optionen() {
    const ModellLod& l = g_lod;
    modell::DetailOptionen o;
    o.stufen = std::clamp(l.stufen, 2, hoechsteStufen());
    for (int i = 0; i < modell::kGlmStufenMax - 1; ++i) { o.anteil[i] = l.anteil[i]; }
    o.gewichtNormale = l.gewichtNormale;
    o.gewichtUv = l.gewichtUv;
    o.gewichtKnochen = l.gewichtKnochen;
    o.raenderHalten = true;
    o.eckenGrenze = modell::grenzen(zielspiel()).eckenJeFlaeche;
    o.schattenGrenze = modell::schattenEckenGrenze(zielspiel());
    o.richtwertLetzte = modellCloneWarsAn() && l.richtwertAn ? modell::cwRichtwert(art()) : 0;
    return o;
}

// Ecken von Stufe 0, die vereinfacht werden (ohne Tags, _off, _cap_)
int eckenVereinfachbar(const modell::Inhalt& in) {
    int n = 0;
    for (const modell::Teil& t : in.teile) {
        if (modell::stufenRolle(t) == modell::StufenRolle::Vereinfachen) { n += static_cast<int>(t.stufen[0].ecken.size()); }
    }
    return n;
}

std::string befundText(const modell::StufenBefund& b) {
    const modell::Grenzen& g = modell::grenzen(zielspiel());
    switch (b.art) {
        case modell::StufenBefundArt::Ecken: return format(Str::LodBfEcken, b.name.c_str(), b.stufe + 1, b.wert, g.name, b.grenze);
        case modell::StufenBefundArt::Schatten: return format(Str::LodBfSchatten, b.name.c_str(), b.stufe + 1, b.wert, b.grenze);
        case modell::StufenBefundArt::Md3Ecken: return format(Str::LodBfMd3Ecken, b.name.c_str(), b.stufe + 1, b.wert, b.grenze);
        case modell::StufenBefundArt::Md3Flaechen: return format(Str::LodBfMd3Flaechen, b.stufe + 1, b.wert, b.grenze);
        case modell::StufenBefundArt::Richtwert: return format(Str::LodBfRichtwert, b.stufe + 1, b.wert, b.grenze);
        case modell::StufenBefundArt::KeineStufen: return tr(Str::LodBfKeineStufen);
    }
    return {};
}

// Die Dateien name_1.md3 / name_2.md3 neben einer .md3 (von der Platte neben einer Datei auf der Platte, sonst aus den Spielordnern
// wie RE_RegisterModel) - nur fuer die Anzeige, solange das Dokument selbst keine Stufen traegt.
void md3DateienAbgleichen() {
    ModellLod& l = g_lod;
    const std::string schluessel = g_app->modelPath + "|" + std::to_string(mt().dok.ladeStand());
    if (l.md3Fuer == schluessel) { return; }
    l.md3Fuer = schluessel;
    l.md3Dateien.clear();
    l.md3Namen.clear();
    if (mt().dok.inhalt().herkunft != modell::Herkunft::Md3 || g_app->modelPath.empty()) { return; }
    std::error_code ec;
    const bool platte = std::filesystem::path(g_app->modelPath).is_absolute();
    for (int s = 1; s < modell::kMd3StufenMax; ++s) {
        const std::string p = modell::md3StufenPfad(g_app->modelPath, s);
        std::string bytes;
        bool da = false;
        if (platte) {
            if (std::filesystem::is_regular_file(p, ec)) {
                bytes = slurp(p);
                da = !bytes.empty();
            }
        } else {
            da = readFromArchives(p, bytes);
        }
        Md3Model m;
        if (!da || !readMd3(bytes, m)) { break; }   // RE_RegisterModel: ohne _1 keine _2 (sonst ERR_DROP)
        l.md3Dateien.push_back(modell::stufenZahlen(m));
        l.md3Namen.push_back(fileName(p));
    }
}

// Eine Zeile der Tabelle: Stufe, Ecken, Dreiecke, Teilflaechen (Klick zeigt die Stufe in der Ansicht)
void stufenTabelle(const std::vector<modell::StufenZahlen>& zahlen, const std::vector<modell::StufenZahlen>* vorher, const std::vector<std::string>* namen,
                   const std::vector<modell::StufenBefund>& befunde, bool waehlbar, const char* id) {
    ModellTab& t = mt();
    const ImGuiTableFlags f = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH;
    if (!ImGui::BeginTable(id, 4, f)) { return; }
    ImGui::TableSetupColumn(tr(Str::LodSpStufe));
    ImGui::TableSetupColumn(tr(Str::LodSpEcken));
    ImGui::TableSetupColumn(tr(Str::LodSpDreiecke));
    ImGui::TableSetupColumn(tr(Str::LodSpFlaechen));
    ImGui::TableHeadersRow();
    if (ImGui::TableGetHoveredRow() == 0) { ImGui::SetTooltip("%s", tr(Str::LodSpHint)); }   // die Kopfzeile: wie gezaehlt wird
    for (std::size_t s = 0; s < zahlen.size(); ++s) {
        const modell::StufenZahlen& z = zahlen[s];
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        std::string befund;
        ui::Ton ton = ui::Ton::Normal;
        for (const modell::StufenBefund& b : befunde) {
            if (b.stufe != static_cast<int>(s)) { continue; }
            befund += (befund.empty() ? "" : "\n") + befundText(b);
            const bool fehler = b.art == modell::StufenBefundArt::Ecken;
            ton = fehler ? ui::Ton::Fehler : (ton == ui::Ton::Fehler ? ton : ui::Ton::Warnung);
        }
        char stufe[96];
        std::snprintf(stufe, sizeof stufe, "LOD %d%s%s", static_cast<int>(s) + 1, namen != nullptr && s < namen->size() && !(*namen)[s].empty() ? " " : "",
                      namen != nullptr && s < namen->size() ? (*namen)[s].c_str() : "");
        const bool gewaehlt = waehlbar && t.lodJetzt == static_cast<int>(s);
        ImGui::PushID(static_cast<int>(s));
        if (waehlbar) {
            if (ImGui::Selectable(stufe, gewaehlt, ImGuiSelectableFlags_SpanAllColumns)) { t.lodWahl = static_cast<int>(s) + 1; }
        } else {
            ImGui::TextUnformatted(stufe);
        }
        ImGui::PopID();
        std::string hinweis = waehlbar ? std::string(tr(Str::LodZeileHint)) : std::string();
        if (!z.groessteName.empty()) { hinweis += (hinweis.empty() ? "" : "\n") + format(Str::LodGroesste, z.groessteName.c_str(), z.groesste); }
        if (!befund.empty()) { hinweis += "\n\n" + befund; }
        if (!hinweis.empty() && ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
            cjkTextUnformatted(hinweis.c_str(), hinweis.c_str() + hinweis.size());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        const auto zelle = [&](int wert, int alt, ui::Ton farbe) {
            ImGui::TableNextColumn();
            char b[64];
            if (alt >= 0 && alt != wert) {
                std::snprintf(b, sizeof b, "%d \xE2\x86\x92 %d", alt, wert);
            } else {
                std::snprintf(b, sizeof b, "%d", wert);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, ui::tonFarbe(farbe));
            ImGui::TextUnformatted(b);
            ImGui::PopStyleColor();
        };
        const modell::StufenZahlen* a = vorher != nullptr && s < vorher->size() ? &(*vorher)[s] : nullptr;
        zelle(z.ecken, a != nullptr ? a->ecken : -1, ton);
        zelle(z.dreiecke, a != nullptr ? a->dreiecke : -1, ui::Ton::Normal);
        zelle(z.flaechen, a != nullptr ? a->flaechen : -1, ui::Ton::Normal);
        g_lod.probeZeilen.push_back(std::string(stufe) + "|" + std::to_string(z.ecken) + "|" + std::to_string(z.dreiecke) + "|" + std::to_string(z.flaechen) +
                                    "|" + std::to_string(static_cast<int>(ton)));
    }
    ImGui::EndTable();
}

int richtwertJetzt() { return modellCloneWarsAn() ? modell::cwRichtwert(art()) : 0; }

}  // namespace

ModellLod& modellLod() { return g_lod; }

bool modellCloneWarsAn() {
    const auto it = g_app->settings.karte.find("OptimizerCloneWars");
    return it != g_app->settings.karte.end() && it->second == "1";
}

std::vector<modell::StufenBefund> modellLodBefunde() {
    const modell::Inhalt& in = mt().dok.inhalt();
    return modell::stufenPruefen(in, modell::grenzen(zielspiel()).eckenJeFlaeche, modell::schattenEckenGrenze(zielspiel()), richtwertJetzt());
}

bool modellLodErzeugen() {
    ModellTab& t = mt();
    if (t.dok.inhalt().leer() || g_app->model.empty()) { return false; }
    const modell::DetailOptionen o = optionen();
    modell::DetailBericht b;
    bool ok = false;
    (void)t.dok.aendern(tr(Str::LodSchritt), [&](modell::Inhalt& x) {
        // offene Mallisten: die Stufen aus dem, was das Spiel daraus macht (wie Speichern); Stufe 0 behaelt ihre Listen
        modell::Inhalt quelle = modell::malt(x) ? modell::wieGezeigt(x) : x;
        ok = modell::detailstufenErzeugen(quelle, o, &b);
        if (!ok) { return; }
        for (std::size_t i = 0; i < x.teile.size() && i < quelle.teile.size(); ++i) {
            modell::Netz null = std::move(x.teile[i].stufen[0]);
            x.teile[i].stufen = std::move(quelle.teile[i].stufen);
            x.teile[i].stufen[0] = std::move(null);
        }
        x.stufenZahl = quelle.stufenZahl;
        ++x.netzStand;
    });
    if (!ok) {
        (void)t.dok.abbrechen();
        return false;
    }
    g_lod.bericht = b;
    g_lod.berichtFuer = g_app->modelPath;
    g_lod.berichtStand = t.dok.stand();
    t.lodWahl = t.dok.inhalt().stufenZahl;   // die letzte Stufe in der Ansicht (Vorschau)
    g_app->modelDirty = true;
    const int vorher = b.nachher.empty() ? 0 : b.nachher[0].ecken;
    std::string m = format(Str::LodErzeugt, t.dok.inhalt().stufenZahl, vorher, b.nachher.empty() ? 0 : b.nachher.back().ecken,
                           static_cast<double>(b.abweichung));
    if (!b.verfehlt.empty()) { m += " " + format(Str::LodVerfehlt, static_cast<int>(b.verfehlt.size())); }
    melden(m);
    return true;
}

bool modellLodEntfernen() {
    ModellTab& t = mt();
    if (t.dok.inhalt().stufenZahl <= 1) { return false; }
    (void)t.dok.aendern(tr(Str::LodEntfernenSchritt), [](modell::Inhalt& x) { modell::detailstufenEntfernen(x); });
    t.lodWahl = 0;
    g_lod.bericht = {};
    g_lod.berichtFuer.clear();
    g_app->modelDirty = true;
    melden(tr(Str::LodEntfernt));
    return true;
}

void modellLodZeigen() {
    if (mt().bereich != ModellBereich::Modellieren) { mt().bereich = ModellBereich::Modellieren; }
    g_lod.zeigen = 2;
}

std::string modellLodMd3Schreiben(const std::string& ziel, const modell::Inhalt& datei) {
    if (datei.herkunft != modell::Herkunft::Md3 || datei.stufenZahl <= 1) { return {}; }
    std::string m;
    int geschrieben = 0;
    for (int s = 1; s < datei.stufenZahl && s < modell::kMd3StufenMax; ++s) {
        std::string bytes;
        modell::Md3SchreibBericht br;
        const std::string pfad = modell::md3StufenPfad(ziel, s);
        std::string fehler;
        if (!modell::md3StufeSchreiben(datei, s, bytes, modell::grenzen(zielspiel()).eckenJeFlaeche, &br)) {
            return format(Str::LodMd3Fehler, fileName(pfad).c_str(), modellFehlerText(br.fehlerArt).c_str());
        }
        if (!sicherSchreiben(pfad, bytes, &fehler)) { return format(Str::LodMd3Fehler, fileName(pfad).c_str(), fehler.c_str()); }
        ++geschrieben;
        m += (m.empty() ? "" : ", ") + fileName(pfad);
    }
    // eine aeltere name_2.md3 neben nur zwei Stufen: das Spiel laedt sie als Stufe 3 (RE_RegisterModel)
    std::error_code ec;
    for (int s = datei.stufenZahl; s < modell::kMd3StufenMax; ++s) {
        const std::string alt = modell::md3StufenPfad(ziel, s);
        if (std::filesystem::is_regular_file(alt, ec)) { m += " " + format(Str::LodMd3Alt, fileName(alt).c_str()); }
    }
    return geschrieben > 0 ? format(Str::LodMd3Geschrieben, m.c_str()) : std::string();
}

std::vector<std::string> modellLodMd3Vorhanden(const std::string& ziel, const modell::Inhalt& in) {
    std::vector<std::string> da;
    if (in.herkunft != modell::Herkunft::Md3 || in.stufenZahl <= 1) { return da; }
    std::error_code ec;
    for (int s = 1; s < in.stufenZahl && s < modell::kMd3StufenMax; ++s) {
        const std::string p = modell::md3StufenPfad(ziel, s);
        if (std::filesystem::exists(p, ec)) { da.push_back(fileName(p)); }
    }
    return da;
}

void drawModellLodGruppe() {
    ModellTab& t = mt();
    ModellLod& l = g_lod;
    const modell::Inhalt& in = t.dok.inhalt();
    // nichts zu vereinfachen (ein leeres Requisit, nur Tags) und keine Stufen: keine Gruppe (Platz nur, wenn gebraucht)
    if (in.leer() || g_app->model.empty() || (eckenVereinfachbar(in) == 0 && in.stufenZahl <= 1)) { return; }
    const bool bl = modellBlender();
    const char* titel = tr(bl ? Str::LodGruppeBl : Str::LodGruppeMax);
    if (l.zeigen > 0) {
        modellFeldReiter(MbReiter::Aendern, MbReiterBl::Modifier);
        ui::rolloutZeigen(titel);
        --l.zeigen;
    }
    // die Art des Modells (Vorgabe der Anteile) neu, wenn ein anderes Modell geladen ist
    if (l.artFuer != g_app->modelPath) {
        l.artFuer = g_app->modelPath;
        l.vorlage = -1;
        l.stufen = std::min(3, hoechsteStufen());
        vorgabeAnwenden();
    }
    l.probeBild = ImGui::GetFrameCount();
    l.probeZeilen.clear();
    char info[48];
    std::snprintf(info, sizeof info, tr(Str::LodKurzinfo), std::max(in.stufenZahl, 1));
    // ab Werk zugeklappt (die Kopfzeile nennt die Zahl der Stufen) - die Werkzeuge des Modellierens bleiben, wo sie waren; Menue, Suche und
    // "Detailstufen erzeugen ..." klappen sie auf
    if (!ui::rolloutBeginn(titel, {.kurzinfo = info, .zu = true, .reiter = modellReiter(MbReiter::Aendern, MbReiterBl::Modifier), .hinweis = tr(Str::LodGruppeHint)})) {
        return;
    }
    // Vorlage: nach der Art des Modells (Spieldateien von Jedi Academy gemessen) oder eigene Anteile
    const char* const vorlagen[4] = {tr(Str::LodVorFigur), tr(Str::LodVorAusruestung), tr(Str::LodVorRequisit), tr(Str::LodVorEigene)};
    const char* const vorlageHinweise[4] = {tr(Str::LodVorFigurHint), tr(Str::LodVorAusruestungHint), tr(Str::LodVorRequisitHint), tr(Str::LodVorEigeneHint)};
    int vorlage = l.vorlage >= 0 ? l.vorlage : static_cast<int>(art());
    ui::zeile(tr(Str::LodVorlage), tr(Str::LodVorlageHint));
    if (ui::segment("##lodvorlage", vorlagen, 4, &vorlage, 2, -1, vorlageHinweise, ui::feldBreite(), true)) {
        l.vorlage = vorlage;
        vorgabeAnwenden();
    }
    // Zahl der Stufen samt Stufe 0 (eine .md3 hat hoechstens 3: MD3_MAX_LODS)
    static const char* const kStufen[3] = {"2", "3", "4"};
    const char* const stufenHinweise[3] = {tr(Str::LodStufenHint), tr(Str::LodStufenHint), tr(Str::LodStufen4Hint)};
    int stufenWahl = std::clamp(l.stufen, 2, 4) - 2;
    ui::zeile(tr(Str::LodStufen), tr(Str::LodStufenHint));
    if (ui::segment("##lodstufen", kStufen, 3, &stufenWahl, 3, hoechsteStufen() - 1, stufenHinweise, ui::feldBreite())) {
        l.stufen = stufenWahl + 2;
        vorgabeAnwenden();
    }
    // Anteil je Stufe in Prozent der Ecken von Stufe 0 (ProOptimizer "Vertex %", Decimate "Ratio")
    const int n0 = eckenVereinfachbar(in);
    const int stufen = std::clamp(l.stufen, 2, hoechsteStufen());
    const int richtwert = richtwertJetzt();
    for (int s = 1; s < stufen; ++s) {
        float prozent = l.anteil[s - 1] * 100.0F;
        char text[96];
        std::snprintf(text, sizeof text, "%s###lodanteil%d", format(Str::LodAnteil, s + 1).c_str(), s);
        char fmt[64];
        std::snprintf(fmt, sizeof fmt, "%%.0f %%%% (%d)", static_cast<int>(std::lround(static_cast<double>(l.anteil[s - 1]) * n0)));
        if (ui::regler(text, &prozent, 5.0F, 100.0F, fmt, tr(bl ? Str::LodAnteilBlHint : Str::LodAnteilMaxHint))) {
            l.anteil[s - 1] = std::clamp(prozent, 5.0F, 100.0F) / 100.0F;
            l.vorlage = modell::kModellArten;   // eigene
        }
    }
    // Clone-Wars-Kette: Richtwert der letzten Stufe (nur mit dem Pruefprofil)
    if (richtwert > 0) {
        const std::string r = format(Str::LodRichtwert, richtwert);
        ui::schalter(r.c_str(), &l.richtwertAn, tr(Str::LodRichtwertHint));
    }
    // Erweitert: wie stark Normalen, UV und Knochengewichte gegen den Ort zaehlen
    if (ui::mehrBeginn(tr(Str::LodErweitert))) {
        (void)ui::regler(tr(Str::LodGwNormale), &l.gewichtNormale, 0.0F, 2.0F, "%.2f", tr(Str::LodGwNormaleHint));
        (void)ui::regler(tr(Str::LodGwUv), &l.gewichtUv, 0.0F, 2.0F, "%.2f", tr(Str::LodGwUvHint));
        if (in.herkunft == modell::Herkunft::Glm) { (void)ui::regler(tr(Str::LodGwKnochen), &l.gewichtKnochen, 0.0F, 2.0F, "%.2f", tr(Str::LodGwKnochenHint)); }
        ui::mehrEnde();
    }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::LodErzeugen), {.icon = UiIcon::MdCollapse, .hinweis = tr(Str::LodErzeugenHint), .haupt = true})) { (void)modellLodErzeugen(); }
    if (ui::werkzeugKnopf(tr(Str::LodEntfernen), {.icon = UiIcon::Loeschen, .hinweis = tr(Str::LodEntfernenHint), .aktiv = in.stufenZahl > 1})) {
        (void)modellLodEntfernen();
    }
    ui::rasterEnde();
    // die Stufen des Dokuments; nach "Erzeugen" mit den Zahlen davor
    std::vector<modell::StufenZahlen> zahlen;
    for (int s = 0; s < std::max(in.stufenZahl, 1); ++s) { zahlen.push_back(modell::stufenZahlen(in, s)); }
    const bool mitVorher = l.berichtFuer == g_app->modelPath && l.berichtStand == t.dok.stand() && !l.bericht.vorher.empty();
    stufenTabelle(zahlen, mitVorher ? &l.bericht.vorher : nullptr, nullptr, modellLodBefunde(), true, "##lodtab");
    if (mitVorher && l.bericht.abweichung > 0.0F) {
        ui::absatz(format(Str::LodAbweichung, static_cast<double>(l.bericht.abweichung)).c_str());
    }
    if (in.herkunft == modell::Herkunft::Md3 && in.stufenZahl > 1) { ui::absatz(tr(Str::LodMd3Speichern)); }
    ui::rolloutEnde();
}

void modellLodZahlen() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    if (in.leer()) { return; }
    std::vector<modell::StufenZahlen> zahlen;
    std::vector<std::string> namen;
    for (int s = 0; s < std::max(in.stufenZahl, 1); ++s) { zahlen.push_back(modell::stufenZahlen(in, s)); }
    std::vector<modell::StufenBefund> befunde = modellLodBefunde();
    bool waehlbar = true;
    if (in.herkunft == modell::Herkunft::Md3 && in.stufenZahl <= 1) {
        // die Dateien name_1.md3 / name_2.md3 daneben (nur Anzeige - das Dokument traegt sie nicht)
        md3DateienAbgleichen();
        if (!g_lod.md3Dateien.empty()) {
            namen.emplace_back();
            for (std::size_t i = 0; i < g_lod.md3Dateien.size(); ++i) {
                zahlen.push_back(g_lod.md3Dateien[i]);
                namen.push_back(g_lod.md3Namen[i]);
            }
            waehlbar = false;
        }
        // Grenzen der Dateien und der Richtwert gelten fuer alle Stufen (die letzte ist die letzte Datei)
        const int richtwert = richtwertJetzt();
        if (richtwert > 0) {
            befunde.erase(std::remove_if(befunde.begin(), befunde.end(), [](const modell::StufenBefund& b) {
                              return b.art == modell::StufenBefundArt::Richtwert || b.art == modell::StufenBefundArt::KeineStufen;
                          }),
                          befunde.end());
            if (zahlen.size() < 2U) { befunde.push_back({modell::StufenBefundArt::KeineStufen, 0, 1, 3, {}}); }
            if (zahlen.back().ecken > richtwert) {
                befunde.push_back({modell::StufenBefundArt::Richtwert, static_cast<int>(zahlen.size()) - 1, zahlen.back().ecken, richtwert, {}});
            }
        }
        const int grenze = modell::grenzen(zielspiel()).eckenJeFlaeche;
        for (std::size_t i = 0; i < g_lod.md3Dateien.size(); ++i) {
            if (g_lod.md3Dateien[i].groesste >= grenze) {
                befunde.push_back({modell::StufenBefundArt::Ecken, static_cast<int>(i) + 1, g_lod.md3Dateien[i].groesste, grenze - 1, g_lod.md3Dateien[i].groessteName});
            }
        }
    }
    g_lod.probeZeilen.clear();
    stufenTabelle(zahlen, nullptr, &namen, befunde, waehlbar, "##lodzahlen");
    // ohne Stufen: der Weg dorthin (Werkzeug "Detailstufen erzeugen")
    if (zahlen.size() < 2U && ui::werkzeugKnopf(tr(Str::LodZumWerkzeug), {.icon = UiIcon::MbLod, .hinweis = tr(Str::LodZumWerkzeugHint)})) { modellLodZeigen(); }
    for (const modell::StufenBefund& b : befunde) {
        if (b.art == modell::StufenBefundArt::Richtwert || b.art == modell::StufenBefundArt::KeineStufen) { ui::absatz(befundText(b).c_str(), ui::Ton::Warnung); }
    }
}

void modellLodMenueZeilen() {
    const modell::Inhalt& in = mt().dok.inhalt();
    const bool da = !in.leer() && !g_app->model.empty();
    if (ImGui::MenuItem(tr(modellBlender() ? Str::LodGruppeBl : Str::LodGruppeMax), nullptr, false, da)) { modellLodZeigen(); }
    ImGui::SetItemTooltip("%s", tr(Str::LodGruppeHint));
    if (ImGui::MenuItem(tr(Str::LodErzeugen), nullptr, false, da)) { (void)modellLodErzeugen(); }
    ImGui::SetItemTooltip("%s", tr(Str::LodErzeugenHint));
    if (ImGui::MenuItem(tr(Str::LodEntfernen), nullptr, false, da && in.stufenZahl > 1)) { (void)modellLodEntfernen(); }
    ImGui::SetItemTooltip("%s", tr(Str::LodEntfernenHint));
}

bool modellLodAktionenSuche(const std::string& such) {
    if (mt().dok.inhalt().leer() || g_app->model.empty()) { return false; }
    struct Eintrag {
        std::string text;
        const char* hinweis;
        bool (*tun)();
    };
    const std::vector<Eintrag> liste = {
        {tr(modellBlender() ? Str::LodGruppeBl : Str::LodGruppeMax), tr(Str::LodGruppeHint), [] {
             modellLodZeigen();
             return true;
         }},
        {tr(Str::LodErzeugen), tr(Str::LodErzeugenHint), [] { return modellLodErzeugen(); }},
        {tr(Str::LodEntfernen), tr(Str::LodEntfernenHint), [] { return modellLodEntfernen(); }},
    };
    bool ausgeloest = false;
    for (const Eintrag& e : liste) {
        std::string k = e.text;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (!such.empty() && k.find(such) == std::string::npos) { continue; }
        if (modellAktionZeile(e.text.c_str(), static_cast<int>(ModellBereich::Modellieren))) {
            (void)e.tun();
            ++mt().tastenZaehler;
            mt().letzteTaste = e.text;
            g_app->modelDirty = true;
            ausgeloest = true;
        }
        ImGui::SetItemTooltip("%s", e.hinweis);
    }
    return ausgeloest;
}

}  // namespace bhed::gui
