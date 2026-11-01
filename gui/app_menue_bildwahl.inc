// app_menue_bildwahl.inc - rc608 Teil 3a: Bild und Video: erst der Rahmen, dann die Auswahl (in app_menue.cpp eingebunden).
// Eingefuegt wird ein Rahmen ohne Dateidialog. Das Fenster hier waehlt, was darin steht: Bilder und Shader aus Spiel und Projekt,
// Videos aus dem Spiel, dazu "Von Datei...". Abbrechen laesst den Rahmen stehen und legt keinen Schritt an.
//
// Geschrieben wird, was JKA und Movie Duels selbst nutzen und in SP und MP laeuft: WINDOW_STYLE_SHADER mit background; beim Video
// ein Shader mit videoMap (NOTIZEN_menue3). Ein Spielvideo kommt nicht ins Paket, nur sein Shader.

namespace {

void lsVideoSymbol(ImDrawList* dl, ImVec2 m, float g) {
    const ImU32 c = IM_COL32(0xE6, 0xE7, 0xEA, 255);
    dl->AddRect(ImVec2{m.x - g * 0.5F, m.y - g * 0.4F}, ImVec2{m.x + g * 0.5F, m.y + g * 0.4F}, c, 2.0F, 0, 1.5F);
    dl->AddTriangleFilled(ImVec2{m.x - g * 0.15F, m.y - g * 0.22F}, ImVec2{m.x - g * 0.15F, m.y + g * 0.22F}, ImVec2{m.x + g * 0.22F, m.y}, c);
}

bool istVideoRahmen(const Dokument& d, const Item& it) {
    Zustand& z = Z();
    if (it.window.background.empty()) {
        if (it.ende <= it.anfang || it.ende > d.puffer.text().size()) { return false; }
        return d.puffer.text().substr(it.anfang, it.ende - it.anfang).find(kVideoRahmenKennung) != std::string::npos;
    }
    shaderLaden();
    const auto s = z.shader.find(kleinPfad(it.window.background));
    return s != z.shader.end() && std::any_of(s->second.stages.begin(), s->second.stages.end(), [](const ShaderInfo::Stage& st) { return !st.video.empty(); });
}

// Das Zeichen im Text, dass ein neuer Rahmen auf ein Video wartet; mit der Auswahl faellt es weg
void videoKennungWeg(const Dokument& d, const Item& it, std::vector<Ersetzung>& e) {
    const std::string& t = d.puffer.text();
    const std::size_t p = t.find(kVideoRahmenKennung, it.anfang);
    if (p == std::string::npos || p >= it.ende) { return; }
    std::size_t a = p;
    while (a > 0 && (t[a - 1] == ' ' || t[a - 1] == '\t')) { --a; }
    std::size_t z = t.find('\n', p);
    z = z == std::string::npos ? t.size() : z + 1;
    e.push_back(Ersetzung{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(z), ""});
}

// Was gewaehlt ist, in den Rahmen; ein Rueckgaengig-Schritt, item < 0: der Hintergrund des Menues
void bildInRahmen(int item, const std::string& name) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return; }
    std::vector<Ersetzung> e;
    bool gut = setzeEigenschaft(d->datei, mi, item, "background", {inAnf(name)}, e);
    if (item >= 0) {
        const Item& it = d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(item)];
        if (it.window.style != WINDOW_STYLE_SHADER && it.window.style != WINDOW_STYLE_FILLED) {
            gut = setzeEigenschaft(d->datei, mi, item, "style", {konstante(d->datei, "WINDOW_STYLE_SHADER", WINDOW_STYLE_SHADER)}, e) && gut;
        }
        videoKennungWeg(*d, it, e);
    }
    if (gut) { aendere(std::move(e)); }
    else { ausMakroMelden(); }
}

// Der Shader eines Spielvideos kommt ins Paket, die Datei bleibt im Spiel
std::string videoShaderAnlegen(Dokument& d, const std::string& name) {
    Zustand& z = Z();
    const std::string shader = "menu/video/" + name;
    const std::string datei = paketShaderDatei(d);
    std::string text;
    if (const auto f = d.bilder.find(datei); f != d.bilder.end()) { text = f->second; }
    ShaderMap da;
    parseShaderScript(text, da);
    if (da.count(shader) == 0) {
        text += (text.empty() || text.back() == '\n' ? "" : "\n") + videoShaderText(shader, "video/" + name + ".roq");
        paketSetzen(d, {{datei, text}});
    }
    shaderLaden();
    parseShaderScript(text, z.shader);
    z.texturen.erase(shader);
    return shader;
}

void videoAusSpielInRahmen(int item, const std::string& name) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    bildInRahmen(item, videoShaderAnlegen(*d, name));
    Z().meldung.clear();
}

void auswahlOeffnen(int art, int item) {
    Zustand::BildWahl& w = Z().wahl;
    w.art = art;
    w.oeffnen = true;
    w.filter.clear();
    w.liste.clear();
    w.item = item;
    w.ziel = [item, art](const std::string& name) {
        if (art == 1) { videoAusSpielInRahmen(item, name); }
        else { bildInRahmen(item, name); }
    };
}

// Fuer einen Schritt in Events (setitembackground, setbackground): dasselbe Fenster, das Wort a bekommt den Namen
void auswahlFuerSchritt(int item, const std::string& schluessel, std::size_t k, std::size_t a, const std::vector<Baustein>& alle) {
    auswahlOeffnen(0, item);
    Z().wahl.ziel = [item, schluessel, k, a, alle](const std::string& name) {
        Dokument* d = doc();
        if (d == nullptr) { return; }
        std::vector<Baustein> liste = alle;
        liste[k].args[a] = name;
        ereignisSchreiben(*d, item, schluessel.c_str(), liste);
    };
}

// Von der Platte ins Paket wie bisher; der Name, den das Spiel dafuer liest
std::string bildAusDatei(Dokument& d, const std::string& pfad) {
    Zustand& z = Z();
    std::ifstream f(utf8Pfad(pfad), std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    std::string base = u8(utf8Pfad(pfad).stem());
    for (char& c : base) { c = (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') ? static_cast<char>(std::tolower(static_cast<unsigned char>(c))) : '_'; }
    const std::string ext = kleinPfad(u8(utf8Pfad(pfad).extension()));
    const std::string spiel = "gfx/menus/" + base + (ext == ".jpeg" ? std::string(".jpg") : ext);
    paketSetzen(d, {{spiel, s.str()}});
    z.texturen.erase("gfx/menus/" + base);
    char b[400];
    std::snprintf(b, sizeof(b), tr(Str::MeBildHinzu), u8(utf8Pfad(pfad).filename()).c_str(), spiel.c_str());
    z.meldung = b;
    return "gfx/menus/" + base;
}

void wahlListeBauen(const Dokument& d) {
    Zustand& z = Z();
    Zustand::BildWahl& w = z.wahl;
    std::set<std::string> s;
    const auto ohneEndung = [](std::string p) {
        const std::size_t punkt = p.rfind('.');
        if (punkt != std::string::npos && p.find('/', punkt) == std::string::npos) { p.resize(punkt); }
        return p;
    };
    const auto imPaket = [&d, &s, &ohneEndung](const std::string& anfang, std::initializer_list<const char*> enden) {
        for (const auto& [p, b] : d.bilder) {
            (void)b;
            const std::string k = kleinPfad(p);
            if (k.rfind(anfang, 0) != 0 && !anfang.empty()) { continue; }
            for (const char* e : enden) {
                const std::size_t n = std::strlen(e);
                if (k.size() >= n && k.compare(k.size() - n, n, e) == 0) { s.insert(ohneEndung(k)); }
            }
        }
    };
    if (w.art == 1) {
        for (const std::string& p : z.daten.liste("video/", {".roq"})) { s.insert(ohneEndung(kleinPfad(p))); }
        imPaket("video/", {".roq"});
        for (const std::string& v : s) { w.liste.push_back(v.substr(6)); }
    } else {
        // Was ein Menue als background nimmt: die Shader und Bilder der Oberflaeche, keine Welt-Texturen
        if (const auto dd = diagDaten()) {
            for (const std::string& k : dd->shader) {
                if (k.rfind("gfx/", 0) == 0 || k.rfind("menu/", 0) == 0 || k.rfind("ui/", 0) == 0) { s.insert(k); }
            }
        }
        for (const char* ordner : {"gfx/", "menu/"}) {
            for (const std::string& p : z.daten.liste(ordner, {".tga", ".jpg", ".png"})) { s.insert(ohneEndung(kleinPfad(p))); }
        }
        imPaket("", {".tga", ".jpg", ".jpeg", ".png"});
        w.liste.assign(s.begin(), s.end());
    }
}

void zeichneBildWahl() {
    Zustand& z = Z();
    Zustand::BildWahl& w = z.wahl;
    Dokument* d = doc();
    if (w.oeffnen) {
        ImGui::OpenPopup("###bildwahl");
        w.oeffnen = false;
    }
    w.istOffen = false;
    if (d == nullptr) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 42.0F, fs * 30.0F}, ImGuiCond_Appearing);
    const std::string titel = std::string(tr(w.art == 1 ? Str::MlWahlVideo : Str::MlWahlBild)) + "###bildwahl";
    if (!ImGui::BeginPopupModal(titel.c_str(), nullptr, ImGuiWindowFlags_NoCollapse)) { return; }
    w.istOffen = true;
    if (w.liste.empty()) { wahlListeBauen(*d); }
    const char* vonDatei = tr(Str::MlVonDatei);
    const char* abbrechen = tr(Str::EditorCancel);
    const float knoepfe = knopfBreite(vonDatei) + knopfBreite(abbrechen) + ImGui::GetStyle().ItemSpacing.x * 2.0F;
    ImGui::SetNextItemWidth(std::max(fs * 6.0F, ImGui::GetContentRegionAvail().x - knoepfe));
    if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
    (void)eingabe("##bw_suche", &w.filter, 0, tr(w.art == 1 ? Str::MlWahlSucheVideo : Str::MlWahlSucheBild));
    ImGui::SameLine();
    bool schliessen = false;
    if (ImGui::Button(vonDatei)) {
        const std::string pfad = z.bildDialog ? z.bildDialog()
                                              : platform::openFileDialog(tr(w.art == 1 ? Str::MeVideoImport : Str::MlVonDatei),
                                                                         w.art == 1 ? "Videos (*.mp4;*.mkv;*.avi;*.mov;*.webm;*.gif;*.roq)|*.mp4;*.mkv;*.avi;*.mov;*.webm;*.gif;*.roq|"
                                                                                    : "Images (*.png;*.jpg;*.tga)|*.png;*.jpg;*.jpeg;*.tga|",
                                                                         "");
        if (!pfad.empty()) {
            if (w.art == 1) {
                // Das vorhandene Fenster zum Umwandeln; der Rahmen behaelt Lage und Groesse
                if (w.item >= 0) { d->auswahl.assign(1, w.item); }
                z.videoQuelle = pfad;
                z.videoVerwendung = VideoVerwendung::Hintergrund;
                z.videoOffen = true;
            } else {
                w.ziel(bildAusDatei(*d, pfad));
            }
            schliessen = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(abbrechen) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { schliessen = true; }
    if (w.art == 1) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::MlVideoStumm));
        ImGui::PopTextWrapPos();
    }
    const std::string f = kleinPfad(w.filter);
    w.gezeigt.clear();
    for (const std::string& n : w.liste) {
        if (f.empty() || n.find(f) != std::string::npos) { w.gezeigt.push_back(n); }
    }
    ImGui::BeginChild("##bw_liste", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders);
    const float abstand = ImGui::GetStyle().ItemSpacing.x;
    const float kachel = std::floor(fs * 6.5F);
    const int spalten = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + abstand) / (kachel + abstand)));
    const int reihen = (static_cast<int>(w.gezeigt.size()) + spalten - 1) / spalten;
    const float zeile = kachel + fs * 1.4F;
    std::string gewaehlt;
    ImGuiListClipper clip;
    clip.Begin(reihen, zeile + ImGui::GetStyle().ItemSpacing.y);
    while (clip.Step()) {
        for (int r = clip.DisplayStart; r < clip.DisplayEnd; ++r) {
            for (int c = 0; c < spalten; ++c) {
                const std::size_t i = static_cast<std::size_t>(r) * static_cast<std::size_t>(spalten) + static_cast<std::size_t>(c);
                if (i >= w.gezeigt.size()) { break; }
                const std::string& n = w.gezeigt[i];
                if (c > 0) { ImGui::SameLine(); }
                const std::string id = "##bw_" + n;
                if (ImGui::InvisibleButton(id.c_str(), ImVec2{kachel, zeile})) { gewaehlt = n; }
                const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const bool ueber = ImGui::IsItemHovered();
                dl->AddRectFilled(a, b, IM_COL32(0x26, 0x28, 0x2D, 255), 4.0F);
                if (ueber) { dl->AddRect(a, b, IM_COL32(0x5A, 0x9B, 0xEA, 255), 4.0F); }
                const ImVec2 ba{a.x + fs * 0.2F, a.y + fs * 0.2F}, bb{b.x - fs * 0.2F, a.y + kachel - fs * 0.2F};
                if (w.art == 0) {
                    if (const Textur* t = textur(n)) {
                        const float bw = bb.x - ba.x, bh = bb.y - ba.y;
                        const float s = std::min(bw / static_cast<float>(std::max(1, t->w)), bh / static_cast<float>(std::max(1, t->h)));
                        const ImVec2 g{static_cast<float>(t->w) * s, static_cast<float>(t->h) * s};
                        const ImVec2 o{ba.x + (bw - g.x) * 0.5F, ba.y + (bh - g.y) * 0.5F};
                        dl->AddRectFilled(o, ImVec2{o.x + g.x, o.y + g.y}, IM_COL32(0, 0, 0, 255));
                        dl->AddImage(reinterpret_cast<ImTextureID>(t->tex), o, ImVec2{o.x + g.x, o.y + g.y});
                    }
                } else {
                    lsVideoSymbol(dl, ImVec2{(ba.x + bb.x) * 0.5F, (ba.y + bb.y) * 0.5F}, kachel * 0.45F);
                }
                const std::size_t sl = n.rfind('/');
                const std::string kurz = sl == std::string::npos ? n : n.substr(sl + 1);
                ImGui::RenderTextEllipsis(dl, ImVec2{a.x + fs * 0.2F, b.y - fs * 1.2F}, ImVec2{b.x - fs * 0.2F, b.y}, b.x - fs * 0.2F, kurz.c_str(), nullptr, nullptr);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", n.c_str()); }
            }
        }
    }
    clip.End();
    if (w.gezeigt.empty()) { ImGui::TextDisabled("%s", tr(Str::MlWahlNichts)); }
    ImGui::EndChild();
    if (!gewaehlt.empty()) {
        w.ziel(gewaehlt);
        schliessen = true;
    }
    if (schliessen) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// Rahmen ohne Bild: Der Platzhalter sagt, was zu tun ist; nur in der Vorschau, nie in der Datei. Das Spiel zeichnet hier den grauen Kasten des Standard-Shaders.
void rahmenPlatzhalter(ImDrawList* dl, const Dokument& d, const Menu& m, const Abbildung& ab, ImVec2 aussen) {
    Zustand& z = Z();
    z.rahmenTexteGezeigt.clear();
    if (z.interaktion) { return; }
    const float fs = ImGui::GetFontSize();
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        const Item& it = m.items[i];
        if (!istBildRahmen(it) || !it.window.background.empty() || (it.window.flags & WINDOW_VISIBLE) == 0) { continue; }
        const Rechteck& r = it.window.rect;
        const ImVec2 a{aussen.x + ab.x(r.x), aussen.y + ab.y(r.y)}, b{aussen.x + ab.x(r.x + r.w), aussen.y + ab.y(r.y + r.h)};
        const char* text = tr(istVideoRahmen(d, it) ? Str::MlPlatzVideo : Str::MlPlatzBild);
        const float breit = b.x - a.x - fs * 0.5F;
        const ImVec2 g = ImGui::CalcTextSize(text, nullptr, false, breit);
        // Ein spaeterer Rahmen liegt ueber dem Text: Im Bild sieht man nur ihn, und zwei Texte uebereinander waeren unlesbar
        const ImVec2 ta{a.x + (b.x - a.x - g.x) * 0.5F, a.y + (b.y - a.y - g.y) * 0.5F}, tb{ta.x + g.x, ta.y + g.y};
        bool verdeckt = false;
        for (std::size_t j = i + 1; j < m.items.size() && !verdeckt; ++j) {
            const Item& o = m.items[j];
            if (!istBildRahmen(o) || (o.window.flags & WINDOW_VISIBLE) == 0) { continue; }
            const Rechteck& ro = o.window.rect;
            const ImVec2 oa{aussen.x + ab.x(ro.x), aussen.y + ab.y(ro.y)}, ob{aussen.x + ab.x(ro.x + ro.w), aussen.y + ab.y(ro.y + ro.h)};
            verdeckt = oa.x < tb.x && ob.x > ta.x && oa.y < tb.y && ob.y > ta.y;
        }
        if (verdeckt) { continue; }
        z.rahmenTexteGezeigt.push_back(static_cast<int>(i));
        dl->PushClipRect(a, b, true);
        dl->AddText(ImGui::GetFont(), fs, ta, IM_COL32(0xD8, 0xDB, 0xE2, 255), text, nullptr, breit);
        dl->PopClipRect();
    }
}

}  // namespace
