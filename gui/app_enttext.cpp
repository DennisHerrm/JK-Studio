// app_enttext.cpp - rc609 W2b: Entity-Texteditor im Mission-Tab (Panel mission.enttext)
//
// Mike: "I like raw text". Die ganze Entity-Liste als .ent-Text mit Zeilennummern
// und Farben, Auswahl in beide Richtungen (Klick in einen Block waehlt die Entity
// in 3D, eine Wahl in 3D springt zum Block und hinterlegt ihn von '{' bis '}').
// "Uebernehmen" (Strg+Enter) liest den Text wie das Spiel (entTextPruefen) - bei
// einem Fehler kommt eine Meldung mit Zeile und nichts wird uebernommen; ein
// Rueckgaengig-Schritt je Uebernahme. Zeilen mit origin, angle(s),
// modelscale(_vec), light und _color wirken schon beim Tippen (BSP-Entity-Edit).
//
// Der Text ist das Dokument: Inspektor, Gizmo und Rueckgaengig aendern
// map.entities, abgleich() schreibt danach nur die betroffenen Zeilen um
// (entTextAbgleichen). Gespeichert wird dieser Text ("Entities speichern").
// Er ist Latin-1 wie die .ent; das Feld zeigt ihn als UTF-8 (F13).
#include "app_internal.h"
#include "fenster_dock.h"

#include "bhed/diag.h"
#include "bhed/enttext.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

constexpr const char* kFeld = "##enttextfeld";

struct Stueck {
    std::size_t von = 0;
    std::size_t bis = 0;
    ImU32 farbe = 0;
};

struct Zustand {
    std::string karte;                 // fuer welche Karte doc gilt
    std::string doc;                   // Latin-1, passt genau zu docStand
    std::vector<MapEntity> docStand;   // nach jedem Abgleich == map.entities
    std::string puffer;                // UTF-8 im Feld
    bool geaendert = false;            // getippt und noch nicht uebernommen
    bool neuLaden = false;             // das aktive Feld soll den Puffer neu lesen
    std::string meldung;
    bool meldungFehler = false;
    int fehlerZeile = -1;              // ab 0
    int gezeigt = -2;                  // pickedEntity, zu dem zuletzt gesprungen wurde
    int springeZeile = -1;
    int springeBilder = 0;
    bool liveSchritt = false;          // Live-Tippen: der Rueckgaengig-Schritt steht schon
    int liveEntity = -1;
    bool fokus = false;                // Panel hat den Fokus (letztes Bild)
    int marke = -1;                    // Schreibmarke (Byte im Puffer), zuletzt im aktiven Feld; -1 = keine
    bool markeSetzen = false;          // nach dem Einfuegen: das aktive Feld bekommt die Schreibmarke dahinter
    bool feldAktiv = false;
    // Zeilen je Entity (aus dem Puffer) und die Farben
    std::vector<int> blockVon;
    std::vector<int> blockBis;
    std::vector<std::vector<Stueck>> farben;
    std::vector<std::size_t> zeilenAnfang;
    bool zeilenAlt = true;
    // Suchen und Ersetzen
    bool sucheOffen = false;
    bool sucheFokus = false;
    bool ersetzenFokus = false;
    bool hervorheben = false;
    std::string suche;
    std::string ersetzen;
    int trefferNr = -1;
    // Lage des Texts (Bildschirm), fuer den Selbsttest
    ImVec2 ursprung{0, 0};
    float zeilenHoehe = 1.0F;
    ImRect feldRect;
    ImRect meldungRect;
};
Zustand& z() {
    static Zustand s;
    return s;
}
// Der letzte Text je Karte (Quelle beim Laden, danach jeder Abgleich).
std::map<std::string, std::string>& quellen() {
    static std::map<std::string, std::string> q;
    return q;
}

std::string bisNul(const std::string& t) { return t.substr(0, std::min(t.find('\0'), t.size())); }

void pufferSetzen(const std::string& utf8) {
    Zustand& s = z();
    s.puffer = utf8;
    s.neuLaden = true;
    s.zeilenAlt = true;
}

// --- Farben und Bloecke ------------------------------------------------------------
// Dieselben Tokenregeln wie der Leser (COM_Parse): Kommentare, Woerter, Anfuehrungszeichen.
void zeilenBerechnen() {
    Zustand& s = z();
    if (!s.zeilenAlt) { return; }
    s.zeilenAlt = false;
    const std::string& t = s.puffer;
    s.zeilenAnfang.assign(1, 0);
    for (std::size_t i = 0; i < t.size(); ++i) {
        if (t[i] == '\n') { s.zeilenAnfang.push_back(i + 1); }
    }
    s.farben.assign(s.zeilenAnfang.size(), {});
    const ImU32 kSchluessel = IM_COL32(120, 190, 255, 255);
    const ImU32 kWert = IM_COL32(230, 160, 120, 255);
    const ImU32 kKlammer = IM_COL32(255, 215, 0, 255);
    const ImU32 kKommentar = IM_COL32(106, 170, 85, 255);
    std::size_t zeile = 0;
    const auto setze = [&](std::size_t von, std::size_t bis, ImU32 f) {
        // ueber Zeilen verteilen
        while (von < bis) {
            while (zeile + 1 < s.zeilenAnfang.size() && s.zeilenAnfang[zeile + 1] <= von) { ++zeile; }
            const std::size_t ende = zeile + 1 < s.zeilenAnfang.size() ? std::min(bis, s.zeilenAnfang[zeile + 1] - 1) : bis;
            if (ende > von) { s.farben[zeile].push_back({von - s.zeilenAnfang[zeile], ende - s.zeilenAnfang[zeile], f}); }
            von = ende + 1;
        }
    };
    int tiefe = 0;
    bool schluessel = true;
    std::size_t i = 0;
    const std::size_t n = t.size();
    while (i < n) {
        const auto c = static_cast<unsigned char>(t[i]);
        if (c <= ' ') {
            ++i;
            continue;
        }
        if (t[i] == '/' && i + 1 < n && t[i + 1] == '/') {
            const std::size_t e = std::min(t.find('\n', i), n);
            setze(i, e, kKommentar);
            i = e;
            continue;
        }
        if (t[i] == '/' && i + 1 < n && t[i + 1] == '*') {
            const std::size_t e = t.find("*/", i + 2);
            const std::size_t bis = e == std::string::npos ? n : e + 2;
            setze(i, bis, kKommentar);
            i = bis;
            continue;
        }
        std::size_t e = i;
        if (t[i] == '"') {
            e = t.find('"', i + 1);
            e = e == std::string::npos ? n : e + 1;
        } else {
            do { ++e; } while (e < n && static_cast<unsigned char>(t[e]) > ' ');
        }
        if (t[i] == '{' && tiefe == 0) {
            setze(i, i + 1, kKlammer);
            tiefe = 1;
            schluessel = true;
        } else if (t[i] == '}' && tiefe == 1 && schluessel) {
            setze(i, i + 1, kKlammer);
            tiefe = 0;
        } else {
            setze(i, e, schluessel ? kSchluessel : kWert);
            schluessel = !schluessel;
        }
        i = e;
    }
    // Bloecke je Entity: der Leser auf dem Text (Latin-1, sonst wie er ist)
    std::string l1;
    if (!utf8NachLatin1(t, l1)) { l1 = t; }
    std::vector<MapEntity> ents;
    std::vector<EntBlockSpanne> sp;
    (void)parseEntitiesGenau(l1, ents, &sp);
    s.blockVon.clear();
    s.blockBis.clear();
    for (const EntBlockSpanne& b : sp) {
        s.blockVon.push_back(entTextZeile(l1, b.anf));
        s.blockBis.push_back(entTextZeile(l1, b.ende > 0 ? b.ende - 1 : 0));
    }
}

int entityAnZeile(int zeile) {
    zeilenBerechnen();
    const Zustand& s = z();
    for (std::size_t i = 0; i < s.blockVon.size(); ++i) {
        if (zeile >= s.blockVon[i] && zeile <= s.blockBis[i]) { return static_cast<int>(i); }
    }
    return -1;
}

// --- Abgleich mit map.entities -----------------------------------------------------
void neuAufbauen() {
    Zustand& s = z();
    s.karte = g_app->map.path;
    const auto q = quellen().find(s.karte);
    std::string doc = q != quellen().end() ? q->second : std::string();
    std::vector<MapEntity> st;
    if (doc.empty() || !parseEntitiesGenau(doc, st)) {
        doc = writeEntities(g_app->map.entities);
        st = g_app->map.entities;
    }
    if (!entListeGleich(st, g_app->map.entities)) { doc = entTextAbgleichen(doc, g_app->map.entities); }
    s.doc = doc;
    s.docStand = g_app->map.entities;
    quellen()[s.karte] = s.doc;
    s.geaendert = false;
    s.fehlerZeile = -1;
    s.meldung.clear();
    s.liveSchritt = false;
    s.gezeigt = -2;
    s.marke = -1;   // eine andere Karte: keine Schreibmarke
    pufferSetzen(latin1NachUtf8(s.doc));
}

// Aenderungen von aussen (Inspektor, Gizmo, Rueckgaengig) in den Text.
void abgleich() {
    Zustand& s = z();
    if (s.karte != g_app->map.path) {
        neuAufbauen();
        return;
    }
    if (entListeGleich(s.docStand, g_app->map.entities)) { return; }
    const std::vector<MapEntity>& theirs = g_app->map.entities;
    if (s.geaendert) {
        // Ungetippte Aenderungen im Feld behalten: was von aussen kam, wird in den
        // getippten Stand uebertragen (je Entity und Schluessel).
        std::string l1;
        std::vector<MapEntity> mine;
        if (utf8NachLatin1(s.puffer, l1) && parseEntitiesGenau(l1, mine) && mine.size() == s.docStand.size() &&
            theirs.size() == s.docStand.size()) {
            for (std::size_t i = 0; i < theirs.size(); ++i) {
                if (entGleich(theirs[i], s.docStand[i])) { continue; }
                if (entGleich(mine[i], s.docStand[i])) {
                    mine[i] = theirs[i];
                    continue;
                }
                for (const auto& kv : theirs[i].keys) {
                    const std::string* alt = s.docStand[i].find(kv.first);
                    if (alt == nullptr || *alt != kv.second) { setKey(mine[i], kv.first, kv.second); }
                }
                for (const auto& kv : s.docStand[i].keys) {
                    if (theirs[i].find(kv.first) == nullptr) { setKey(mine[i], kv.first, std::string()); }
                }
            }
            pufferSetzen(latin1NachUtf8(entTextAbgleichen(l1, mine)));
        } else {
            s.geaendert = false;
            s.meldung = tr(Str::EtVerworfen);
            s.meldungFehler = true;
        }
    }
    s.doc = entTextAbgleichen(s.doc, theirs);
    s.docStand = theirs;
    quellen()[s.karte] = s.doc;
    s.liveSchritt = false;
    if (!s.geaendert) { pufferSetzen(latin1NachUtf8(s.doc)); }
}

std::string fehlerText(const EntTextFehler& f) {
    Str id = Str::EtFehlerKlammer;
    switch (f.art) {
        case EntFehler::KlammerErwartet: id = Str::EtFehlerKlammer; break;
        case EntFehler::OhneSchliessendeKlammer: id = Str::EtFehlerEof; break;
        case EntFehler::KlammerOhneWert: id = Str::EtFehlerOhneWert; break;
        case EntFehler::ZuVielePaare: id = Str::EtFehlerPaare; break;
        case EntFehler::ZuVieleZeichen: id = Str::EtFehlerZeichen; break;
        case EntFehler::OffenesAnfuehrungszeichen: id = Str::EtFehlerAnf; break;
        case EntFehler::ErsteNichtWorldspawn: id = Str::EtFehlerWorld; break;
        case EntFehler::Keiner: break;
    }
    char b[700];
    std::snprintf(b, sizeof(b), tr(id), f.zeile, f.token.substr(0, 200).c_str());
    return std::string(b) + " " + tr(Str::EtNichtsUebernommen);
}

void zuZeile(int zeile) {
    Zustand& s = z();
    s.springeZeile = zeile;
    s.springeBilder = 3;
}

// Getippt: die Live-Schluessel sofort in die Entities.
void liveVersuch() {
    Zustand& s = z();
    std::string l1;
    std::vector<MapEntity> neu;
    if (!utf8NachLatin1(s.puffer, l1) || !parseEntitiesGenau(l1, neu) || neu.size() != s.docStand.size()) { return; }
    std::vector<int> live;
    bool alles = true;
    for (std::size_t i = 0; i < neu.size(); ++i) {
        if (entGleich(neu[i], s.docStand[i])) { continue; }
        if (entTextNurLive(s.docStand[i], neu[i])) {
            live.push_back(static_cast<int>(i));
        } else {
            alles = false;
        }
    }
    if (live.empty()) { return; }
    std::vector<MapEntity> ziel = g_app->map.entities;
    for (const int i : live) { ziel[static_cast<std::size_t>(i)] = neu[static_cast<std::size_t>(i)]; }
    const bool weiter = s.liveSchritt && live.size() == 1 && live.front() == s.liveEntity;
    (void)entitiesErsetzen(std::move(ziel), !weiter);
    s.liveSchritt = true;
    s.liveEntity = live.size() == 1 ? live.front() : -1;
    s.docStand = g_app->map.entities;
    if (alles) {
        s.doc = l1;
        s.geaendert = false;
    } else {
        s.doc = entTextAbgleichen(s.doc, s.docStand);
    }
    quellen()[s.karte] = s.doc;
}

// --- Suchen ------------------------------------------------------------------------
std::vector<std::size_t> treffer() {
    std::vector<std::size_t> r;
    const Zustand& s = z();
    if (s.suche.empty()) { return r; }
    std::string t = s.puffer;
    std::string q = s.suche;
    for (char& c : t) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    for (char& c : q) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    for (std::size_t p = t.find(q); p != std::string::npos; p = t.find(q, p + q.size())) { r.push_back(p); }
    return r;
}

int zeileVonByte(std::size_t p) {
    zeilenBerechnen();
    const auto& a = z().zeilenAnfang;
    return static_cast<int>(std::upper_bound(a.begin(), a.end(), p) - a.begin()) - 1;
}

void auswahlImFeld(std::size_t von, std::size_t bis) {
    const ImGuiID id = ImGui::GetID(kFeld);
    if (ImGuiInputTextState* st = ImGui::GetInputTextState(id); st != nullptr && ImGui::GetActiveID() == id) {
        st->SetSelection(static_cast<int>(von), static_cast<int>(bis));
        st->CursorFollow = true;
    }
}

void weiter(int richtung) {
    Zustand& s = z();
    const std::vector<std::size_t> t = treffer();
    s.hervorheben = true;
    if (t.empty()) {
        s.trefferNr = -1;
        return;
    }
    const int n = static_cast<int>(t.size());
    s.trefferNr = s.trefferNr < 0 ? (richtung > 0 ? 0 : n - 1) : (s.trefferNr + richtung + n) % n;
    const std::size_t p = t[static_cast<std::size_t>(s.trefferNr)];
    zuZeile(zeileVonByte(p));
    auswahlImFeld(p, p + s.suche.size());
}

void ersetzen(bool alle) {
    Zustand& s = z();
    const std::vector<std::size_t> t = treffer();
    if (t.empty() || s.suche.empty()) { return; }
    std::string p = s.puffer;
    if (alle) {
        for (auto it = t.rbegin(); it != t.rend(); ++it) { p.replace(*it, s.suche.size(), s.ersetzen); }
        s.trefferNr = -1;
    } else {
        const int nr = std::clamp(s.trefferNr, 0, static_cast<int>(t.size()) - 1);
        p.replace(t[static_cast<std::size_t>(nr)], s.suche.size(), s.ersetzen);
        s.trefferNr = nr - 1;
    }
    pufferSetzen(p);
    s.geaendert = true;
    liveVersuch();
    if (!alle) { weiter(1); }
}

// --- Zeichnen ----------------------------------------------------------------------
int waechstText(ImGuiInputTextCallbackData* d) {
    if (d->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* t = static_cast<std::string*>(d->UserData);
        t->resize(static_cast<std::size_t>(d->BufTextLen));
        d->Buf = t->data();
    }
    return 0;
}

void kopfzeile() {
    Zustand& s = z();
    if (ImGui::Button(tr(Str::EtUebernehmen))) { (void)entTextUebernehmen(); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::EtUebernehmenHint)); }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EtSuchen))) {
        s.sucheOffen = true;
        s.sucheFokus = true;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::EtSuchenHint)); }
    ImGui::SameLine();
    ImGui::BeginDisabled(g_app->pickedEntity < 0);
    if (ImGui::Button(tr(Str::EtAusblenden))) { entTextAusblenden(); }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", tr(Str::EtAusblendenHint)); }
    ImGui::SameLine();
    ImGui::BeginDisabled(g_app->hiddenEntities.empty());
    if (ImGui::Button(tr(Str::EtAlleZeigen))) { entTextAlleEinblenden(); }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EtSpeichern))) {
        if (!s.geaendert || entTextUebernehmen()) { (void)entitiesSpeichern(); }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::EtSpeichernHint)); }
    if (!s.meldung.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextColored(s.meldungFehler ? ImVec4{1.0F, 0.45F, 0.4F, 1.0F} : ImVec4{0.6F, 0.85F, 0.6F, 1.0F}, "%s", s.meldung.c_str());
        s.meldungRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::PopTextWrapPos();
    } else if (s.geaendert) {
        ImGui::TextDisabled("%s", tr(Str::EtUngetippt));
    }
    if (!s.sucheOffen) { return; }
    const float fs = ImGui::GetFontSize();
    if (s.sucheFokus) {
        ImGui::SetKeyboardFocusHere();
        s.sucheFokus = false;
    }
    ImGui::SetNextItemWidth(fs * 12.0F);
    if (ImGui::InputTextWithHint("##etsuche", tr(Str::EtSuchbegriff), s.suche.data(), s.suche.capacity() + 1,
                                 ImGuiInputTextFlags_CallbackResize, waechstText, &s.suche)) {
        s.trefferNr = -1;
        s.hervorheben = true;
    }
    if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) { weiter(1); }
    ImGui::SameLine();
    if (s.ersetzenFokus) {
        ImGui::SetKeyboardFocusHere();
        s.ersetzenFokus = false;
    }
    ImGui::SetNextItemWidth(fs * 10.0F);
    (void)ImGui::InputTextWithHint("##etersetzen", tr(Str::EtErsetzenDurch), s.ersetzen.data(), s.ersetzen.capacity() + 1,
                                   ImGuiInputTextFlags_CallbackResize, waechstText, &s.ersetzen);
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EtWeiter))) { weiter(1); }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EtErsetzen))) { ersetzen(false); }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EtAlleErsetzen))) { ersetzen(true); }
    ImGui::SameLine();
    const std::size_t n = treffer().size();
    ImGui::TextDisabled(tr(Str::EtTreffer), s.trefferNr + 1, static_cast<int>(n));
}

void tasten() {
    Zustand& s = z();
    if (!s.fokus) { return; }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Enter) || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_KeypadEnter)) {
        (void)entTextUebernehmen();
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F)) {
        s.sucheOffen = true;
        s.sucheFokus = true;
        s.hervorheben = true;
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_H)) {   // wie Notepad++ (und der Menue-Editor): Ersetzen
        s.sucheOffen = true;
        s.ersetzenFokus = true;
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_F3)) { weiter(1); }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Shift | ImGuiKey_F3)) { weiter(-1); }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Alt | ImGuiKey_H)) { entTextAlleEinblenden(); }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {   // Esc: Hervorhebung weg (auch im Suchfeld)
        s.hervorheben = false;
        s.sucheOffen = false;
    }
    if (!s.feldAktiv && !ImGui::GetIO().WantTextInput && ImGui::IsKeyChordPressed(ImGuiKey_H)) { entTextAusblenden(); }
}

void textFeld(const ImVec2& platz) {
    Zustand& s = z();
    zeilenBerechnen();
    ImFont* font = ImGui::GetFont();
    const float fsz = ImGui::GetFontSize();
    const int zeilenZahl = static_cast<int>(s.zeilenAnfang.size());
    char nr[16];
    std::snprintf(nr, sizeof(nr), "%d", zeilenZahl);
    const float nummernB = font->CalcTextSizeA(fsz, 1e9F, 0.0F, nr).x + fsz * 0.8F;
    const ImVec2 start = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2{start.x + nummernB, start.y});
    const ImGuiID id = ImGui::GetID(kFeld);
    ImGuiWindow* fenster = ImGui::GetCurrentWindow();
    char kn[512];
    std::snprintf(kn, sizeof(kn), "%s/%s_%08X", fenster->Name, kFeld, id);
    if (s.springeZeile >= 0 && s.springeBilder > 0) {
        if (ImGuiWindow* kind = ImGui::FindWindowByName(kn)) {
            const float sichtbar = kind->InnerClipRect.GetHeight();
            ImGui::SetScrollY(kind, std::max(0.0F, static_cast<float>(s.springeZeile) * ImGui::GetTextLineHeight() - sichtbar * 0.3F));
        }
        if (--s.springeBilder == 0) { s.springeZeile = -1; }
    }
    // Esc: Hervorhebung weg - und NICHT ImGuis "Text auf den Stand beim Anklicken zuruecksetzen".
    s.feldAktiv = ImGui::GetActiveID() == id;
    if (s.feldAktiv && !s.markeSetzen) {
        if (const ImGuiInputTextState* st = ImGui::GetInputTextState(id)) { s.marke = st->GetCursorPos(); }
    }
    if (s.feldAktiv && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        ImGui::SetKeyOwner(ImGuiKey_Escape, ImGui::GetID("##enttextesc"));
        s.hervorheben = false;
        s.sucheOffen = false;
    }
    // Strg+Enter (Uebernehmen, tasten()): das Feld bleibt aktiv - ImGui beendet ein mehrzeiliges
    // Feld sonst damit (imgui_widgets.cpp, is_ctrl_enter ohne CtrlEnterForNewLine), und nach einer
    // Fehlermeldung ginge das Weitertippen ins Leere.
    if (s.feldAktiv && ImGui::GetIO().KeyCtrl) {
        for (const ImGuiKey k : {ImGuiKey_Enter, ImGuiKey_KeypadEnter}) {
            if (ImGui::IsKeyPressed(k, false)) { ImGui::SetKeyOwner(k, ImGui::GetID("##enttextenter")); }
        }
    }
    if (s.neuLaden) {
        if (ImGuiInputTextState* st = ImGui::GetInputTextState(id); st != nullptr && s.feldAktiv) {
            st->ReloadUserBufAndKeepSelection();
            if (s.markeSetzen) { st->SetSelection(s.marke, s.marke); }
        }
        s.neuLaden = false;
        s.markeSetzen = false;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));   // der Text kommt gefaerbt darueber
    const bool neu = ImGui::InputTextMultiline(kFeld, s.puffer.data(), s.puffer.capacity() + 1, ImVec2{platz.x - nummernB, platz.y},
                                               ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize, waechstText, &s.puffer);
    ImGui::PopStyleColor();
    s.feldRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    const bool geklickt = ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    if (neu) {
        s.geaendert = true;
        s.zeilenAlt = true;
        s.fehlerZeile = -1;
        s.meldung.clear();
        liveVersuch();
        zeilenBerechnen();
    }
    ImGuiWindow* kind = ImGui::FindWindowByName(kn);
    if (kind == nullptr) { return; }
    const float zh = ImGui::GetTextLineHeight();
    s.zeilenHoehe = zh;
    ImVec2 o = kind->DC.CursorStartPos;
    ImGuiInputTextState* st = ImGui::GetInputTextState(id);
    if (st != nullptr) { o.x -= st->Scroll.x; }
    s.ursprung = o;
    // Klick in einen Block: die Entity waehlen (wie ein Klick in 3D)
    if (geklickt && st != nullptr) {
        const int zeile = zeileVonByte(static_cast<std::size_t>(std::max(st->GetCursorPos(), 0)));
        const int e = entityAnZeile(zeile);
        if (e >= 0 && e < static_cast<int>(g_app->map.entities.size()) && e != g_app->pickedEntity) {
            entityAuswahlSetzen(e);
            s.gezeigt = e;
        }
    }
    ImDrawList* dl = kind->DrawList;
    dl->PushClipRect(kind->InnerClipRect.Min, kind->InnerClipRect.Max, true);
    const int erste = std::max(0, static_cast<int>((kind->InnerClipRect.Min.y - o.y) / zh));
    const int letzte = std::min(zeilenZahl, static_cast<int>((kind->InnerClipRect.Max.y - o.y) / zh) + 1);
    const float links = kind->InnerClipRect.Min.x;
    const float rechts = kind->InnerClipRect.Max.x;
    // Hinterlegung der gewaehlten Entity von '{' bis '}'
    const int pe = g_app->pickedEntity;
    if (pe >= 0 && static_cast<std::size_t>(pe) < s.blockVon.size()) {
        const float y0 = o.y + static_cast<float>(s.blockVon[static_cast<std::size_t>(pe)]) * zh;
        const float y1 = o.y + static_cast<float>(s.blockBis[static_cast<std::size_t>(pe)] + 1) * zh;
        dl->AddRectFilled(ImVec2{links, y0}, ImVec2{rechts, y1}, IM_COL32(40, 120, 60, 110));
    }
    if (s.fehlerZeile >= 0) {
        const float y0 = o.y + static_cast<float>(s.fehlerZeile) * zh;
        dl->AddRectFilled(ImVec2{links, y0}, ImVec2{rechts, y0 + zh}, IM_COL32(170, 40, 40, 130));
    }
    if (s.hervorheben && !s.suche.empty()) {
        const std::vector<std::size_t> t = treffer();
        for (std::size_t k = 0; k < t.size(); ++k) {
            const int zl = zeileVonByte(t[k]);
            if (zl < erste || zl >= letzte) { continue; }
            const char* za = s.puffer.c_str() + s.zeilenAnfang[static_cast<std::size_t>(zl)];
            const float x0 = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, za, s.puffer.c_str() + t[k]).x;
            const float x1 = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, za, s.puffer.c_str() + t[k] + s.suche.size()).x;
            const float y = o.y + static_cast<float>(zl) * zh;
            const bool aktuell = static_cast<int>(k) == s.trefferNr;
            dl->AddRectFilled(ImVec2{x0, y}, ImVec2{x1, y + zh}, aktuell ? IM_COL32(230, 160, 40, 200) : IM_COL32(200, 180, 60, 110));
        }
    }
    for (int i = erste; i < letzte; ++i) {
        const std::size_t a = s.zeilenAnfang[static_cast<std::size_t>(i)];
        const char* za = s.puffer.c_str() + a;
        const float y = o.y + static_cast<float>(i) * zh;
        const std::size_t ende = static_cast<std::size_t>(i) + 1 < s.zeilenAnfang.size() ? s.zeilenAnfang[static_cast<std::size_t>(i) + 1] - 1 : s.puffer.size();
        dl->AddText(font, fsz, ImVec2{o.x, y}, ImGui::GetColorU32(ImGuiCol_Text), za, s.puffer.c_str() + ende);
        for (const Stueck& p : s.farben[static_cast<std::size_t>(i)]) {
            const float x = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, za, za + p.von).x;
            // gefaerbt ueber den Grundtext: dieselben Glyphen an derselben Stelle
            dl->AddText(font, fsz, ImVec2{x, y}, p.farbe, za + p.von, za + p.bis);
        }
    }
    if (st != nullptr && s.feldAktiv) {   // der Cursor (die Textfarbe ist durchsichtig)
        const std::size_t p = std::min<std::size_t>(static_cast<std::size_t>(std::max(st->GetCursorPos(), 0)), s.puffer.size());
        const int zl = zeileVonByte(p);
        const char* za = s.puffer.c_str() + s.zeilenAnfang[static_cast<std::size_t>(zl)];
        const float x = o.x + font->CalcTextSizeA(fsz, 1e9F, 0.0F, za, s.puffer.c_str() + p).x;
        const float y = o.y + static_cast<float>(zl) * zh;
        dl->AddLine(ImVec2{x, y}, ImVec2{x, y + zh}, IM_COL32(230, 230, 230, 255), 1.0F);
    }
    dl->PopClipRect();
    // links die Zeilennummern
    ImDrawList* ad = ImGui::GetWindowDrawList();
    ad->PushClipRect(ImVec2{start.x, kind->InnerClipRect.Min.y}, ImVec2{start.x + nummernB, kind->InnerClipRect.Max.y}, true);
    for (int i = erste; i < letzte; ++i) {
        char n[16];
        std::snprintf(n, sizeof(n), "%d", i + 1);
        const float b = font->CalcTextSizeA(fsz, 1e9F, 0.0F, n).x;
        const bool fehler = i == s.fehlerZeile;
        ad->AddText(font, fsz, ImVec2{start.x + nummernB - b - fsz * 0.4F, o.y + static_cast<float>(i) * zh},
                    fehler ? IM_COL32(255, 110, 100, 255) : IM_COL32(130, 130, 130, 255), n);
    }
    ad->PopClipRect();
}

}  // namespace

// --- Nach aussen -------------------------------------------------------------------

bool eingabeNachLatin1(const std::string& utf8, std::string& out) {
    std::size_t stelle = 0;
    std::string zeichen;
    if (utf8NachLatin1(utf8, out, &stelle, &zeichen)) { return true; }
    char b[400];
    std::snprintf(b, sizeof(b), tr(Str::EtNichtLatin1Feld), zeichen.c_str());
    meldung(b, std::string(), true);
    diag::warn("Entity-Text: Zeichen nicht in Latin-1 (" + zeichen + ")");
    return false;
}

void entTextQuelle(const std::string& karte, const std::string& text) {
    quellen()[karte] = bisNul(text);
    if (z().karte == karte) { z().karte.clear(); }   // beim naechsten Abgleich neu aus der Quelle
}

const std::string& entTextFuerSpeichern() {
    abgleich();
    return z().doc;
}

bool entTextUebernehmen() {
    Zustand& s = z();
    abgleich();
    std::string l1;
    std::size_t stelle = 0;
    std::string zeichen;
    if (!utf8NachLatin1(s.puffer, l1, &stelle, &zeichen)) {
        s.fehlerZeile = zeileVonByte(stelle);
        char b[500];
        std::snprintf(b, sizeof(b), tr(Str::EtNichtLatin1), s.fehlerZeile + 1, zeichen.c_str());
        s.meldung = std::string(b) + " " + tr(Str::EtNichtsUebernommen);
        s.meldungFehler = true;
        zuZeile(s.fehlerZeile);
        return false;
    }
    std::vector<MapEntity> neu;
    std::vector<EntBlockSpanne> sp;
    EntTextFehler f;
    if (!entTextPruefen(l1, neu, &sp, &f)) {
        s.fehlerZeile = std::max(0, f.zeile - 1);
        s.meldung = fehlerText(f);
        s.meldungFehler = true;
        zuZeile(s.fehlerZeile);
        diag::detail("Entity-Text: Fehler in Zeile " + std::to_string(f.zeile) + ", nichts uebernommen");
        return false;
    }
    if (!entListeGleich(neu, g_app->map.entities)) {
        (void)entitiesErsetzen(std::move(neu), true);
    } else if (l1 != s.doc) {
        g_app->entitiesUngespeichert.insert(g_app->map.path);   // nur Kommentare oder Leerraum: trotzdem zu speichern
    }
    s.doc = l1;
    s.docStand = g_app->map.entities;
    quellen()[s.karte] = s.doc;
    s.geaendert = false;
    s.fehlerZeile = -1;
    s.liveSchritt = false;
    s.zeilenAlt = true;
    char b[200];
    std::snprintf(b, sizeof(b), trN(Str::EtUebernommen, g_app->map.entities.size()), static_cast<int>(g_app->map.entities.size()));
    s.meldung = b;
    s.meldungFehler = false;
    diag::detail("Entity-Text: uebernommen, " + std::to_string(g_app->map.entities.size()) + " Entities");
    return true;
}

void entTextAusblenden() {
    for (const int i : entityAuswahl()) {
        if (i >= 0 && static_cast<std::size_t>(i) < g_app->map.entities.size()) { g_app->hiddenEntities.insert(static_cast<std::size_t>(i)); }
    }
    g_app->mapDirty = true;
}

void entTextAlleEinblenden() {
    g_app->hiddenEntities.clear();
    g_app->mapDirty = true;
}

bool entTextHatFokus() { return g_app->leftMode == 1 && g_app->settings.entTextOffen && z().fokus; }

void drawEntTextInhalt() {
    Zustand& s = z();
    s.fokus = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (g_app->map.empty()) {
        ImGui::TextDisabled("%s", tr(Str::EtKeineKarte));
        return;
    }
    abgleich();
    // Eine Wahl in 3D: zum Block springen
    if (g_app->pickedEntity != s.gezeigt) {
        s.gezeigt = g_app->pickedEntity;
        s.liveSchritt = false;
        zeilenBerechnen();
        if (s.gezeigt >= 0 && static_cast<std::size_t>(s.gezeigt) < s.blockVon.size()) { zuZeile(s.blockVon[static_cast<std::size_t>(s.gezeigt)]); }
    }
    tasten();
    kopfzeile();
    textFeld(ImGui::GetContentRegionAvail());
}

void drawEntTextFenster() {
    if (g_app->leftMode != 1 || !g_app->settings.entTextOffen) {
        z().fokus = false;
        return;
    }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 46.0F, fs * 36.0F}, ImGuiCond_FirstUseEver);
    if (dock::fensterBeginnen("mission.enttext")) { drawEntTextInhalt(); }
    dock::fensterEnde();
}

// --- Selbsttest ---------------------------------------------------------------------
const std::string& entTextPuffer() { return z().puffer; }
const std::string& entTextDoc() { return z().doc; }
bool entTextGeaendert() { return z().geaendert; }
int entTextFehlerZeile() { return z().fehlerZeile; }
const std::string& entTextMeldung() { return z().meldung; }
int entTextTrefferNr() { return z().trefferNr; }
int entTextTrefferZahl() { return static_cast<int>(treffer().size()); }
bool entTextHervorhebung() { return z().hervorheben; }
void entTextHinterlegt(int& von, int& bis) {
    zeilenBerechnen();
    const Zustand& s = z();
    const int pe = g_app->pickedEntity;
    von = bis = -1;
    if (pe >= 0 && static_cast<std::size_t>(pe) < s.blockVon.size()) {
        von = s.blockVon[static_cast<std::size_t>(pe)];
        bis = s.blockBis[static_cast<std::size_t>(pe)];
    }
}
int entTextBlockZeile(int entity) {
    zeilenBerechnen();
    const Zustand& s = z();
    return entity >= 0 && static_cast<std::size_t>(entity) < s.blockVon.size() ? s.blockVon[static_cast<std::size_t>(entity)] : -1;
}
// Bildschirmpunkt in Zeile zeile (ab 0), spalte Zeichen weit eingerueckt; (-1,-1) wenn nicht sichtbar.
ImVec2 entTextZeilenOrt(int zeile, int spalte) {
    const Zustand& s = z();
    const float y = s.ursprung.y + (static_cast<float>(zeile) + 0.5F) * s.zeilenHoehe;
    const float x = s.ursprung.x + ImGui::GetFontSize() * 0.5F * static_cast<float>(spalte) + 2.0F;
    if (y < s.feldRect.Min.y || y > s.feldRect.Max.y) { return ImVec2{-1.0F, -1.0F}; }
    return ImVec2{x, y};
}
int entTextErsteSichtbareZeile() {
    const Zustand& s = z();
    return static_cast<int>(std::max(0.0F, (s.feldRect.Min.y - s.ursprung.y) / std::max(1.0F, s.zeilenHoehe)));
}
void entTextMeldungOrt(ImVec2& a, ImVec2& b) {
    a = z().meldungRect.Min;
    b = z().meldungRect.Max;
}
// Mittelklick in 3D (BSP-Entity-Edit): "x y z" des getroffenen Punkts an der Schreibmarke einfuegen
// (gerundet wie die origin-Werte der Karten). Ohne Schreibmarke (das Feld war noch nie aktiv) nichts.
bool entTextKoordinatenEinfuegen(const float p[3]) {
    Zustand& s = z();
    if (!g_app->settings.entTextOffen || s.marke < 0) { return false; }
    char b[64];
    std::snprintf(b, sizeof(b), "%ld %ld %ld", std::lround(p[0]), std::lround(p[1]), std::lround(p[2]));
    const std::size_t an = std::min(static_cast<std::size_t>(s.marke), s.puffer.size());
    std::string neu = s.puffer;
    neu.insert(an, b);
    pufferSetzen(neu);
    s.marke = static_cast<int>(an + std::strlen(b));
    s.markeSetzen = true;
    s.geaendert = true;
    s.zeilenAlt = true;
    s.fehlerZeile = -1;
    s.meldung.clear();
    liveVersuch();
    diag::detail(std::string("Entity-Text: Mittelklick, \"") + b + "\" an Byte " + std::to_string(an));
    return true;
}

void entTextMittelklick() {
    if (!g_app->settings.entTextOffen || g_app->camViewActive || g_app->geo.empty()) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    const bool imBild = io.MousePos.x >= g_app->kartenAnsicht[0] && io.MousePos.x < g_app->kartenAnsicht[2] && io.MousePos.y >= g_app->kartenAnsicht[1] &&
                        io.MousePos.y < g_app->kartenAnsicht[3] && ImGui::IsWindowHovered();
    // ein Klick, kein Ziehen (Ziehen mit der mittleren Taste verschiebt die Ansicht)
    if (!imBild || !ImGui::IsMouseReleased(ImGuiMouseButton_Middle) ||
        io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Middle] >= io.MouseDragThreshold * io.MouseDragThreshold) {
        return;
    }
    TraceTreffer t;
    if (kartenStrahlUnterMaus(io.MousePos.x, io.MousePos.y, t)) { (void)entTextKoordinatenEinfuegen(t.point); }
}

int entTextMarke() { return z().marke; }

ImVec2 entTextKopfOrt() {
    const Zustand& s = z();
    return ImVec2{s.feldRect.Max.x - ImGui::GetFontSize() * 2.0F, s.feldRect.Min.y - ImGui::GetFontSize() * 0.6F};
}

}  // namespace bhed::gui
