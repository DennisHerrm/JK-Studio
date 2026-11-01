// app_menue_projekt.inc - Menue-Editor M5b Punkt 3 (in app_menue.cpp eingebunden): Projektordner
// wie die pk3 und Projektdatei *.jksmenu (bhed/menue/projekt.h), Sicherung alle 30 Sekunden mit
// Wiederherstellen nach einem Absturz, Export als pk3, Dateizuordnung nur nach Rueckfrage.

namespace {   // wie die Vorwaertserklaerungen oben in app_menue.cpp

constexpr double kSicherungAlle = 30.0;   // Sekunden zwischen zwei Sicherungen

// Das erste Menue eines neuen Projekts: Vollbild mit dunkelblauem Grund
constexpr const char* kStartMenue =
    "{\n\tmenuDef\n\t{\n\t\tname\t\tmein_menue\n\t\tfullscreen\t1\n\t\trect\t\t0 0 640 480\n\t\tvisible\t\t1\n\t\titemDef\n\t\t{\n\t\t\tname\t\thinter"
    "grund\n\t\t\tstyle\t\tWINDOW_STYLE_FILLED\n\t\t\tbackcolor\t0 0 0.25 1\n\t\t\trect\t\t0 0 640 480\n\t\t\tvisible\t\t1\n\t\t\tdecoration\n\t\t}\n\t}\n}\n";

fs::path projektOrdner() { return utf8Pfad(Z().projektDatei).parent_path(); }

bool istProjektDatei(const std::string& p) {
    const std::string k = kleinPfad(p), e = kProjektEndung;
    return k.size() > e.size() && k.compare(k.size() - e.size(), e.size(), e) == 0;
}

std::string liesGanz(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// Der Stand des Editors als Projekt (Menues = offene Dokumente, Paketdateien mit ihrem Menue)
Projekt projektAusZustand() {
    Zustand& z = Z();
    Projekt p = z.projekt;   // Profil und Export bleiben
    p.spiel = z.spiel;
    if (const Dokument* d = doc(); d != nullptr) { p.dialekt = d->datei.dialekt; }
    p.menues.clear();
    p.offen.clear();
    p.dateien.clear();
    for (const auto& od : z.docs) {
        p.menues.push_back(od->name);
        p.offen.push_back(od->name);
        for (const auto& [pfad, bytes] : od->bilder) { p.dateien[pfad] = od->name; }
    }
    p.aktiv = std::max(0, z.aktiv);
    p.aufloesung = z.aufloesung;
    p.balken = z.seitenv == Seitenverhaeltnis::Balken;
    p.sprache = z.sprache;
    p.cvars = z.vorschauCvars;
    return p;
}

// --- Mod-Profile (M5b Punkt 4) -------------------------------------------------

fs::path profilOrdner() { return utf8Pfad(platform::settingsDirectory()) / "menue_profile"; }

bool profilEingebaut(const ModProfil& p) {
    const auto& e = eingebauteProfile();
    return std::any_of(e.begin(), e.end(), [&](const ModProfil& x) { return x.name == p.name; });
}

// Eigene Profile: <Einstellungen>/menue_profile und der Projektordner (*.jksprofile)
std::vector<ModProfil> eigeneProfile() {
    std::vector<ModProfil> v;
    std::vector<fs::path> orte = {profilOrdner()};
    if (!Z().projektDatei.empty()) { orte.push_back(projektOrdner()); }
    for (const fs::path& o : orte) {
        std::error_code ec;
        for (fs::directory_iterator it(o, ec), ende; !ec && it != ende; it.increment(ec)) {
            if (!it->is_regular_file(ec) || kleinPfad(u8(it->path().extension())) != kProfilEndung) { continue; }
            ModProfil p;
            int zeile = 0;
            if (liesProfil(liesGanz(it->path()), p, zeile) && !p.name.empty()) { v.push_back(p); }
        }
    }
    return v;
}

std::shared_ptr<const ModProfil> aktivesProfil() {
    Zustand& z = Z();
    const std::string schl = z.projekt.profil + "|" + std::to_string(static_cast<int>(z.spiel));
    if (z.profil && z.profilFuer == schl) { return z.profil; }
    z.profilFuer = schl;
    z.profil = std::make_shared<const ModProfil>(eingebautesProfil(z.spiel));
    if (!z.projekt.profil.empty()) {
        for (const ModProfil& p : eingebauteProfile()) {
            if (p.name == z.projekt.profil) { z.profil = std::make_shared<const ModProfil>(p); }
        }
        for (const ModProfil& p : eigeneProfile()) {
            if (p.name == z.projekt.profil) { z.profil = std::make_shared<const ModProfil>(p); }
        }
    }
    return z.profil;
}

LeseOptionen leseOptionen(const std::string& pfad) {
    Zustand& z = Z();
    LeseOptionen o = optionenFuer(pfad, z.spiel, z.daten.lader());
    o.profil = aktivesProfil();
    o.weiterlesen = true;   // rc608: eine ungueltige Zeile laesst die uebrigen Elemente stehen
    return o;
}

// Ein anderes Profil: alles neu lesen (Schluessel, Menuelisten, Schriften)
void setzeProfil(const std::string& name) {
    Zustand& z = Z();
    z.projekt.profil = name;
    z.profilFuer.clear();
    z.schriftNamenGeladen = false;
    for (const auto& od : z.docs) { od->geparst = -1; }
    z.laufDatei = -1;
}

// Vorlage zum Ausfuellen (Name = Dateiname); danach in der Profilliste waehlbar
bool profilVorlageSchreiben(const std::string& ziel) {
    Zustand& z = Z();
    std::string d = ziel;
    if (kleinPfad(d).size() < 11 || kleinPfad(d).compare(kleinPfad(d).size() - 11, 11, kProfilEndung) != 0) { d += kProfilEndung; }
    const std::string name = fs::path(utf8Pfad(d)).stem().string();
    const bool ok = schreibeDatei(d, profilVorlage(name, z.spiel));
    char b[700];
    std::snprintf(b, sizeof(b), tr(ok ? Str::MeProfilVorlageOk : Str::MeNichtGespeichert), d.c_str());
    z.meldung = b;
    addStatus(b);
    return ok;
}

void zeichneProfilWahl() {
    const std::shared_ptr<const ModProfil> akt = aktivesProfil();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
    if (ImGui::BeginCombo(tr(Str::MeProfil), akt->name.c_str())) {
        for (const ModProfil& p : eingebauteProfile()) {
            if (ImGui::Selectable(p.name.c_str(), p.name == akt->name)) { setzeProfil(p.name); }
        }
        const std::vector<ModProfil> eigene = eigeneProfile();
        if (!eigene.empty()) { ImGui::Separator(); }
        for (const ModProfil& p : eigene) {
            if (ImGui::Selectable(p.name.c_str(), p.name == akt->name)) { setzeProfil(p.name); }
        }
        ImGui::EndCombo();
    }
    tipp(Str::MeProfilTip);
    if (ImGui::MenuItem(tr(Str::MeProfilVorlage))) {
        std::error_code ec;
        fs::create_directories(profilOrdner(), ec);
        const std::string ziel = platform::saveFileDialog(tr(Str::MeProfilVorlage), "JK Studio mod profile (*.jksprofile)|*.jksprofile|",
                                                          u8(profilOrdner()), "meine_mod.jksprofile");
        if (!ziel.empty()) { (void)profilVorlageSchreiben(ziel); }
    }
    tipp(Str::MeProfilVorlageTip);
}

// Schluessel aus dem Mod-Profil beim gewaehlten Element und beim Menue: beschriftet, mit dem Wert wie
// im Text (der Editor aendert sie nicht; bearbeiten im Text-Reiter)
void zeichneModSchluessel(Dokument& d) {
    const int mi = menuDerSeite(d);
    if (mi < 0) { return; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    std::vector<const Eigenschaft*> liste;
    for (const Eigenschaft& e : m.eigenschaften) {
        if (e.ausProfil) { liste.push_back(&e); }
    }
    for (const int i : d.auswahl) {
        if (i < 0 || static_cast<std::size_t>(i) >= m.items.size()) { continue; }
        for (const Eigenschaft& e : m.items[static_cast<std::size_t>(i)].eigenschaften) {
            if (e.ausProfil) { liste.push_back(&e); }
        }
    }
    if (liste.empty()) { return; }
    char kopf[200];
    std::snprintf(kopf, sizeof(kopf), tr(Str::MeProfilSchluessel), aktivesProfil()->name.c_str());
    gruppe("modschluessel", kopf, {}, [&d, &liste] {
        ImGui::TextDisabled("%s", tr(Str::MeProfilSchluesselTip));
        const std::string& t = d.datei.text;
        for (const Eigenschaft* e : liste) {
            std::uint32_t ende = e->kopf.ende;
            for (const Token& a : e->args) {
                if (a.datei == 0 && a.ende > ende) { ende = a.ende; }
            }
            std::string wert = e->kopf.datei == 0 && ende <= t.size() ? t.substr(e->kopf.ende, ende - e->kopf.ende) : std::string();
            for (char& c : wert) {
                if (c == '\r' || c == '\n' || c == '\t') { c = ' '; }
            }
            ImGui::BulletText("%s:%s", e->schluessel.c_str(), wert.c_str());
        }
    }, false);
}

// --- Sicherung ---------------------------------------------------------------

fs::path sicherungsOrt() {
    Zustand& z = Z();
    if (!z.projektDatei.empty()) { return projektOrdner() / kSicherungsOrdner; }
    return utf8Pfad(platform::settingsDirectory()) / "menue_autosave";
}

// Alle ungespeicherten Dokumente sichern; ohne Aenderungen die Sicherung entfernen
void sicherungJetzt() {
    Zustand& z = Z();
    std::error_code ec;
    const fs::path ort = sicherungsOrt();
    // eine gefundene, noch nicht beantwortete Sicherung an diesem Ort bleibt, wie sie ist
    if (!z.sicherungGefunden.empty() && utf8Pfad(z.sicherungGefunden) == ort) { return; }
    std::vector<SicherungsEintrag> liste;
    for (const auto& od : z.docs) {
        if (od->puffer.text() == od->gespeichert) { continue; }
        liste.push_back(SicherungsEintrag{od->name, od->platte, std::to_string(liste.size() + 1) + ".menu"});
    }
    fs::remove_all(ort, ec);
    if (liste.empty()) { return; }
    fs::create_directories(ort, ec);
    std::size_t k = 0;
    for (const auto& od : z.docs) {
        if (od->puffer.text() == od->gespeichert) { continue; }
        (void)schreibeDatei(u8(ort / liste[k++].datei), od->puffer.text());
    }
    (void)schreibeDatei(u8(ort / "autosave.txt"), sicherungText(liste, static_cast<std::int64_t>(std::time(nullptr))));
}

void sicherungPflegen() {
    Zustand& z = Z();
    const double jetzt = ImGui::GetTime();
    if (z.sicherungAm <= 0) { z.sicherungAm = jetzt; }
    if (jetzt - z.sicherungAm < kSicherungAlle) { return; }
    z.sicherungAm = jetzt;
    sicherungJetzt();
}

// Liegt an diesem Ort eine Sicherung? Dann fragen, ob sie zurueckkommen soll.
void sicherungPruefen(const fs::path& ort) {
    Zustand& z = Z();
    std::vector<SicherungsEintrag> liste;
    std::int64_t zeit = 0;
    if (!liesSicherung(liesGanz(ort / "autosave.txt"), liste, zeit)) { return; }
    z.sicherungGefunden = u8(ort);
    z.sicherungListe = liste;
    z.sicherungZeitpunkt = zeit;
    z.sicherungFrage = true;
}

void sicherungWiederherstellen() {
    Zustand& z = Z();
    const fs::path ort = utf8Pfad(z.sicherungGefunden);
    for (const SicherungsEintrag& e : z.sicherungListe) {
        const std::string text = liesGanz(ort / e.datei);
        std::error_code ec;
        // das Dokument wie zuletzt (Platte, sonst Spiel, sonst neu), dann der gesicherte Text als
        // Aenderung - ungespeichert und mit Rueckgaengig
        std::string vorher;
        if (!e.platte.empty() && fs::is_regular_file(utf8Pfad(e.platte), ec)) { vorher = liesGanz(utf8Pfad(e.platte)); }
        else if (!z.daten.lies(e.name, vorher)) { vorher.clear(); }
        if (!oeffne(e.name, e.platte, vorher)) { continue; }
        Dokument* d = doc();
        if (d == nullptr || d->puffer.text() == text) { continue; }
        d->puffer.ersetzeViele({Ersetzung{0, static_cast<std::uint32_t>(d->puffer.text().size()), text}});
        neuParsen(*d);
    }
    std::error_code ec;
    fs::remove_all(ort, ec);
    z.sicherungGefunden.clear();
    char b[300];
    std::snprintf(b, sizeof(b), trN(Str::MePrjWiederhergestellt, z.sicherungListe.size()), static_cast<int>(z.sicherungListe.size()));
    z.meldung = b;
    addStatus(b);
}

void zeichneSicherungFrage() {
    Zustand& z = Z();
    if (z.sicherungFrage) {
        ImGui::OpenPopup("##sicherung");
        z.sicherungFrage = false;
    }
    if (!ImGui::BeginPopupModal("##sicherung", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) { return; }
    char zeit[64] = "?";
    const std::time_t t = static_cast<std::time_t>(z.sicherungZeitpunkt);
    std::tm lt{};
    if (localtime_s(&lt, &t) == 0) { (void)std::strftime(zeit, sizeof(zeit), "%d.%m.%Y %H:%M", &lt); }
    ImGui::TextUnformatted(tr(Str::MePrjSicherungTitel));
    ImGui::Separator();
    char b[400];
    std::snprintf(b, sizeof(b), trN(Str::MePrjSicherungText, z.sicherungListe.size()), static_cast<int>(z.sicherungListe.size()), zeit);
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30);
    ImGui::TextWrapped("%s", b);
    for (const SicherungsEintrag& e : z.sicherungListe) { ImGui::BulletText("%s", e.name.c_str()); }
    ImGui::PopTextWrapPos();
    if (ImGui::Button(tr(Str::MePrjWiederherstellen))) {
        sicherungWiederherstellen();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MePrjVerwerfen))) {
        std::error_code ec;
        fs::remove_all(utf8Pfad(z.sicherungGefunden), ec);
        z.sicherungGefunden.clear();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// --- Projekt -----------------------------------------------------------------

bool speichereProjekt() {
    Zustand& z = Z();
    if (z.projektDatei.empty()) { return false; }
    const fs::path ordner = projektOrdner();
    char b[600];
    if (imSpielordner(u8(ordner))) {
        z.meldung = tr(Str::MeNieSpielordner);
        return false;
    }
    std::error_code ec;
    for (const std::string& u : projektUnterordner()) { fs::create_directories(ordner / u, ec); }
    bool ok = true;
    for (const auto& od : z.docs) {
        for (const auto& [pfad, bytes] : od->bilder) {
            const fs::path ziel = ordner / utf8Pfad(pfad);
            fs::create_directories(ziel.parent_path(), ec);
            ok = schreibeDatei(u8(ziel), bytes) && ok;
        }
    }
    for (const auto& od : z.docs) {
        const fs::path ziel = ordner / utf8Pfad(od->name);
        fs::create_directories(ziel.parent_path(), ec);
        if (schreibeDatei(u8(ziel), od->puffer.text())) {
            od->platte = u8(ziel);
            od->gespeichert = od->puffer.text();
        } else {
            ok = false;
        }
    }
    z.projekt = projektAusZustand();
    ok = schreibeDatei(z.projektDatei, projektText(z.projekt)) && ok;
    if (ok) {
        sicherungJetzt();   // alles gespeichert: die Sicherung faellt weg
        ++z.projektGespeichert;
        imLaufendenSpielNeuLaden();   // M7 Punkt 13
    }
    std::snprintf(b, sizeof(b), ok ? tr(Str::MePrjGespeichert) : tr(Str::MeNichtGespeichert), z.projektDatei.c_str());
    z.meldung = b;
    addStatus(b);
    return ok;
}

// Neues Projekt: die offenen Menues (falls welche offen sind) kommen hinein. M7: ohne offenes Menue
// beginnt es im gewaehlten Format - Mehrspieler im Menueordner der MP-Fassung (ui/jamp, ui/jk2mp,
// ui/MD_MP), dort liest die Engine menudef.h vorab (PC_LoadGlobalDefines)
std::string startMenuePfad(Dialekt d, Spiel s) {
    if (d == Dialekt::SP) { return "ui/mein_menue.menu"; }
    return s == Spiel::JK2 ? "ui/jk2mp/mein_menue.menu" : s == Spiel::MD ? "ui/MD_MP/mein_menue.menu" : "ui/jamp/mein_menue.menu";
}

bool neuesProjekt(const std::string& datei) {
    Zustand& z = Z();
    std::string d = datei;
    if (!istProjektDatei(d)) { d += kProjektEndung; }
    sicherungJetzt();   // eine Sicherung am alten Ort bleibt, bis sie geschrieben ist
    z.projektDatei = d;
    z.projekt = Projekt{};
    // ohne offenes Menue: ein leeres Vollbild-Menue zum Anfangen (Tour "Dein erstes eigenes Menue")
    if (z.docs.empty()) { (void)oeffne(startMenuePfad(z.neuDialekt, z.spiel), "", kStartMenue); }
    return speichereProjekt();
}

bool oeffneProjektJetzt(const std::string& datei) {
    Zustand& z = Z();
    datenPruefen();
    std::error_code ec;
    const fs::path pfad = utf8Pfad(datei);
    Projekt p;
    ProjektFehler f;
    char b[700];
    if (!fs::is_regular_file(pfad, ec)) {
        std::snprintf(b, sizeof(b), tr(Str::MePrjFehltDatei), datei.c_str());
        z.meldung = b;
        return false;
    }
    if (!liesProjekt(liesGanz(pfad), p, f)) {
        if (f.art == ProjektFehler::Art::ZuNeu) { std::snprintf(b, sizeof(b), tr(Str::MePrjZuNeu), f.wert.c_str()); }
        else if (f.art == ProjektFehler::Art::PfadAusserhalb) { std::snprintf(b, sizeof(b), tr(Str::MePrjPfadAusserhalb), f.zeile, f.wert.c_str()); }
        else { std::snprintf(b, sizeof(b), tr(Str::MePrjKeinProjekt), datei.c_str()); }
        z.meldung = b;
        addStatus(b);
        return false;
    }
    // ungespeicherte Arbeit geht nicht verloren: sie liegt in der Sicherung am bisherigen Ort
    sicherungJetzt();
    z.docs.clear();
    z.aktiv = -1;
    z.laufDatei = -1;
    z.projektDatei = u8(fs::absolute(pfad, ec));
    z.projekt = p;
    setzeProfil(p.profil);   // M5b Punkt 4: das Mod-Profil des Projekts
    const fs::path ordner = projektOrdner();
    for (const std::string& m : p.menues) {
        const fs::path mp = ordner / utf8Pfad(m);
        if (fs::is_regular_file(mp, ec)) { (void)oeffne(m, u8(mp), liesGanz(mp)); }
    }
    // alle uebrigen Dateien des Ordners sind das Paket - beim Menue, zu dem sie gehoeren
    for (fs::recursive_directory_iterator it(ordner, fs::directory_options::skip_permission_denied, ec), ende; it != ende; it.increment(ec)) {
        if (ec || !it->is_regular_file(ec)) { continue; }
        const std::string rel = u8(fs::relative(it->path(), ordner, ec));
        std::string r = rel;
        std::replace(r.begin(), r.end(), '\\', '/');
        if (!paketPfad(r) || std::find(p.menues.begin(), p.menues.end(), r) != p.menues.end()) { continue; }
        const auto eig = p.dateien.find(r);
        Dokument* ziel = z.docs.empty() ? nullptr : z.docs.front().get();
        for (const auto& od : z.docs) {
            if (eig != p.dateien.end() && od->name == eig->second) { ziel = od.get(); }
        }
        if (ziel != nullptr) { ziel->bilder[r] = liesGanz(it->path()); }
    }
    for (const auto& od : z.docs) { od->texteGelesen = false; }
    // Reiter und Vorschau wie gespeichert
    z.aktiv = z.docs.empty() ? -1 : 0;
    if (p.aktiv >= 0 && static_cast<std::size_t>(p.aktiv) < p.offen.size()) {
        for (std::size_t i = 0; i < z.docs.size(); ++i) {
            if (z.docs[i]->name == p.offen[static_cast<std::size_t>(p.aktiv)]) { z.aktiv = static_cast<int>(i); }
        }
    }
    z.aufloesung = p.aufloesung;
    z.seitenv = p.balken ? Seitenverhaeltnis::Balken : Seitenverhaeltnis::Strecken;
    if (!p.sprache.empty()) { setzeSprache(p.sprache); }
    z.vorschauCvars = p.cvars;
    std::snprintf(b, sizeof(b), trN(Str::MePrjGeoeffnet, z.docs.size()), fs::path(pfad).stem().string().c_str(), static_cast<int>(z.docs.size()));
    z.meldung = b;
    if (p.spiel != z.spiel) {
        const char* n[] = {"Jedi Academy", "Jedi Outcast", "Movie Duels"};
        std::snprintf(b, sizeof(b), tr(Str::MePrjAnderesSpiel), n[static_cast<int>(p.spiel)], n[static_cast<int>(z.spiel)]);
        z.meldung += "  " + std::string(b);
    }
    addStatus(z.meldung);
    sicherungPruefen(ordner / kSicherungsOrdner);
    return true;
}

bool exportiereProjekt() {
    Zustand& z = Z();
    if (z.projektDatei.empty()) { return false; }
    fs::path ziel = utf8Pfad(z.projekt.exportPk3.empty() ? std::string("zz_menu.pk3") : z.projekt.exportPk3);
    if (ziel.is_relative()) { ziel = projektOrdner() / ziel; }
    return exportiere(u8(ziel), z.projekt.nurAenderungen);
}

void zeichneZuordnungFrage() {
    Zustand& z = Z();
    if (z.zuordnungFrage) {
        ImGui::OpenPopup("##zuordnung");
        z.zuordnungFrage = false;
    }
    if (!ImGui::BeginPopupModal("##zuordnung", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) { return; }
    ImGui::TextUnformatted(tr(Str::MePrjZuordnen));
    ImGui::Separator();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30);
    ImGui::TextWrapped("%s", tr(Str::MePrjZuordnenFrage));
    ImGui::PopTextWrapPos();
    if (ImGui::Button(tr(Str::MePrjZuordnenJa))) {
        std::vector<platform::RegWert> werte;
        for (const RegEintrag& e : dateizuordnung(platform::executablePath(), tr(Str::MePrjDateityp))) { werte.push_back({e.schluessel, e.name, e.wert}); }
        const bool ok = platform::schreibeBenutzerRegistry(werte);
        z.meldung = tr(ok ? Str::MePrjZuordnenOk : Str::MePrjZuordnenFehler);
        addStatus(z.meldung);
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MePrjZuordnenNein))) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// M7: Format des Startmenues eines neuen Projekts (Einzelspieler / Mehrspieler)
void zeichneNeuDialekt() {
    Zustand& z = Z();
    int d = z.neuDialekt == Dialekt::MP ? 1 : 0;
    ImGui::RadioButton(tr(Str::MePrjSp), &d, 0);
    ImGui::SameLine();
    ImGui::RadioButton(tr(Str::MePrjMp), &d, 1);
    tipp(Str::MePrjDialektTip);
    z.neuDialekt = d == 1 ? Dialekt::MP : Dialekt::SP;
}

// Die beiden Wege zu einem Projekt, fuer den Leistenknopf und den Startbildschirm
void neuesProjektWaehlen() {
    Zustand& z = Z();
    const std::string ziel = z.projektDialog ? z.projektDialog(true)
                                             : platform::saveFileDialog(tr(Str::MePrjNeu), "JK Studio menu project (*.jksmenu)|*.jksmenu|", z.ordnerAusgabe,
                                                                        "mein_menue.jksmenu");
    if (!ziel.empty()) { (void)neuesProjekt(ziel); }
}

void projektOeffnenWaehlen() {
    Zustand& z = Z();
    const std::string ziel = z.projektDialog ? z.projektDialog(false)
                                             : platform::openFileDialog(tr(Str::MePrjOeffnen), "JK Studio menu project (*.jksmenu)|*.jksmenu|", z.ordnerAusgabe);
    if (!ziel.empty()) { (void)oeffneProjektJetzt(ziel); }
}

// Leistenknopf "Projekt" mit Menue
void zeichneProjektKnopf() {
    Zustand& z = Z();
    const std::string titel = z.projektDatei.empty() ? std::string(tr(Str::MePrjProjekt))
                                                     : std::string(tr(Str::MePrjProjekt)) + ": " + fs::path(utf8Pfad(z.projektDatei)).stem().string();
    if (leistenKnopf(titel.c_str(), UiIcon::Mission, tr(Str::MePrjTip))) { ImGui::OpenPopup("##projekt"); }
    if (ImGui::BeginPopup("##projekt")) {
        zeichneNeuDialekt();
        if (ImGui::MenuItem(tr(Str::MePrjNeu))) { neuesProjektWaehlen(); }
        if (ImGui::MenuItem(tr(Str::MePrjOeffnen))) { projektOeffnenWaehlen(); }
        const bool offen = !z.projektDatei.empty();
        if (ImGui::MenuItem(tr(Str::MePrjSpeichern), nullptr, false, offen)) { (void)speichereProjekt(); }
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::MePrjExport), nullptr, false, offen)) { (void)exportiereProjekt(); }
        if (offen) {
            ImGui::Checkbox(tr(Str::MeExportAenderungen), &z.projekt.nurAenderungen);
            ImGui::TextDisabled("%s", z.projekt.exportPk3.c_str());
            if (ImGui::MenuItem(tr(Str::MePrjExportZiel))) {
                const std::string ziel = platform::saveFileDialog(tr(Str::MePrjExportZiel), "pk3 (*.pk3)|*.pk3|", u8(projektOrdner()), z.projekt.exportPk3);
                if (!ziel.empty()) { z.projekt.exportPk3 = ziel; }
            }
            if (ImGui::MenuItem(tr(Str::MePrjOrdnerZeigen))) { platform::openInExplorer(u8(projektOrdner())); }
        }
        ImGui::Separator();
        zeichneProfilWahl();   // M5b Punkt 4
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::MePrjZuordnen))) { z.zuordnungFrage = true; }
        ImGui::EndPopup();
    }
    zeichneZuordnungFrage();
}

// Leerer Zustand: noch kein Menue offen. rc608 (shank: "schoener wie beim Mission tab"): dieselben
// Bauteile wie die Startflaeche des Mission-Tabs.
void zeichneProjektLeer() {
    const StartSpalte sp = startKopf("##menuestart", ImGui::GetContentRegionAvail(), tr(Str::MeStartTitel), tr(Str::MeKeinMenue));
    const float fs = ImGui::GetFontSize();
    ImGui::SetCursorPosX(sp.links);
    ImGui::PushTextWrapPos(sp.links + sp.breite);
    ImGui::TextDisabled("%s", tr(Str::MePrjLeer));
    ImGui::Dummy(ImVec2{0.0F, fs * 0.8F});
    // Einzelspieler oder Mehrspieler: gilt fuer das neue Projekt
    ImGui::SetCursorPosX(sp.links);
    ImGui::TextUnformatted(tr(Str::MeStartFormat));
    ImGui::SetCursorPosX(sp.links);
    zeichneNeuDialekt();
    ImGui::SetCursorPosX(sp.links);
    ImGui::TextDisabled("%s", tr(Str::MeStartFormatSatz));
    ImGui::PopTextWrapPos();
    ImGui::Dummy(ImVec2{0.0F, fs * 0.6F});

    const StartKarte karten[] = {
        {tr(Str::MePrjNeu), tr(Str::MeStartNeuSub), UiIcon::Neu, tr(Str::MePrjNeuTip)},
        {tr(Str::MePrjOeffnen), tr(Str::MeStartOeffnenSub), UiIcon::Oeffnen, nullptr},
    };
    const int wahl = startKarten(sp, karten, 2);
    if (wahl == 0) { neuesProjektWaehlen(); }
    if (wahl == 1) { projektOeffnenWaehlen(); }
    ImGui::Dummy(ImVec2{0.0F, fs * 0.6F});

    const StartKnopf hilfe[] = {
        {tr(Str::EmptyShowHow), UiIcon::Zielen, tr(Str::MeStartZeigenTip)},
        {tr(Str::MeStartHandbuch), UiIcon::Liste, tr(Str::MeStartHandbuchTip)},
    };
    const int weg = startKnopfZeile(sp, tr(Str::StartGuide), hilfe, 2);
    if (weg == 0) { (void)tourStarten("tut-menue", 1); }   // H2: der Weg als Tour
    if (weg == 1) { handbuchOeffnen("menue_editor.html"); }
    ImGui::EndChild();
}

}  // namespace
