// selbsttest_grafik.inc - D3D12 Stufe 1 (Modus "grafik"): Wahl der Grafikschnittstelle und Rueckfall
//
// Kurz, zum Wiederholen je Schnittstelle:
//   - es laeuft, was verlangt ist: ohne BHED_D3D12 die Vorgabe Direct3D 12 (rc612), BHED_D3D12=1 Direct3D 12, =0 Direct3D 11;
//     mit BHED_D3D12_FEHLER=<stelle> (dll, geraet, swap, abgeber, imgui) scheitert der Start von Direct3D 12 an dieser Stelle
//     wie auf einem Rechner ohne Direct3D 12 - dann muss Direct3D 11 tragen, und alles Folgende laeuft darauf; absturz spielt
//     die Startmarke eines abgestuerzten Starts (nur ohne BHED_D3D12), neuaufbau einen Neuaufbau nach dem Geraeteverlust
//     ohne D3D12-Geraet
//   - die 3D-Ansichten haben ihr Geraet, eine eigene Textur laesst sich anlegen und ersetzen
//   - Ansicht > Grafikschnittstelle stellt die Einstellung fuer den naechsten Start und nennt im Tooltip, was laeuft
//   - Fotos der Oberflaeche fuer den Bildvergleich Direct3D 11 gegen Direct3D 12 (BHED_FOTOS): Start, Menue Ansicht,
//     Untermenue mit Tooltip, die Mischarten der Oberflaeche als festes Muster, About-Fenster
namespace grafik {
// Die gespielte Fehlerstelle (BHED_D3D12_FEHLER), leer ohne
inline std::string fehlerStelle() {
    const char* f = std::getenv("BHED_D3D12_FEHLER");
    return f != nullptr ? std::string(f) : std::string();
}
// Laeuft nach dem Start Direct3D 12? Ohne BHED_D3D12 die Vorgabe der Einstellung (Direct3D 12); jede gespielte Stelle des
// Starts laesst Direct3D 11 tragen (absturz nur, wenn die Einstellung entscheidet); neuaufbau trifft erst den Neuaufbau.
inline bool startD3d12() {
    const char* w = std::getenv("BHED_D3D12");
    const std::string f = fehlerStelle();
    const bool gewuenscht = w != nullptr ? w[0] == '1' : Settings{}.grafikD3d12;
    if (!gewuenscht) { return false; }
    if (f == "absturz") { return w != nullptr; }
    return f.empty() || f == "neuaufbau";
}
}  // namespace grafik

std::vector<Schritt> grafikSchritte() {
    std::vector<Schritt> s;
    const auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(pruefe("Grafikschnittstelle: ab Werk ist Direct3D 12 eingestellt (rc612), Direct3D 11 der Rueckfall", [] { return Settings{}.grafikD3d12; }));
    add(pruefe("Grafikschnittstelle: es laeuft, was verlangt ist", [] {
        const std::string f = grafik::fehlerStelle();
        const bool gespielt = !f.empty();
        const bool d3d12 = grafik::startD3d12();
        diag::detail(std::string("Knopftest: Grafikschnittstelle laeuft: ") + render::backendName(render::current()) + ", Karte " +
                     render::geraeteKarte() + (gespielt ? ", gespielter Fehler an der Stelle " + f : std::string()));
        return render::current() == (d3d12 ? render::Backend::Direct3D12 : render::Backend::Direct3D11);
    }));
    add(pruefe("Grafikschnittstelle: abgestuerzter Start mit Direct3D 12 (Startmarke) - Rueckfall gemeldet, Einstellung Direct3D 11", [] {
        const bool absturz = grafik::fehlerStelle() == "absturz" && std::getenv("BHED_D3D12") == nullptr;
        diag::detail(std::string("Knopftest: Startmarke - Rueckfall gemeldet ") + (grafikRueckfallGemeldet() ? "ja" : "nein") + ", Einstellung " +
                     (g_app->settings.grafikD3d12 ? "Direct3D 12" : "Direct3D 11"));
        return absturz ? grafikRueckfallGemeldet() && !g_app->settings.grafikD3d12 : !grafikRueckfallGemeldet();
    }));
    add(warteBis("Grafikschnittstelle: mehr als 60 Bilder gezeigt (dann loescht der Start die Marke)", [] { return ImGui::GetFrameCount() > 60; }, 600));
    add(pruefe("Grafikschnittstelle: nach dem Start liegt keine Startmarke mehr (d3d12_start.marke)", [] {
        const std::string einst = platform::settingsDirectory();
        std::error_code ec;
        const bool da = !einst.empty() &&
                        std::filesystem::exists(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(einst.data()), einst.size())) / "d3d12_start.marke", ec);
        diag::detail(std::string("Knopftest: Startmarke nach ") + std::to_string(ImGui::GetFrameCount()) + " Bildern " + (da ? "noch da" : "weg"));
        return ImGui::GetFrameCount() > 60 && !da;
    }));
    add(pruefe("Editor-Texturen rechnen ihre Mip-Stufen ab Werk wie das Spiel (R_MipMap; Entscheidung 11.11., beide Schnittstellen)", [] {
        const char* v = std::getenv("BHED_KARTE_MIPS");
        return (v != nullptr && v[0] != '\0') || kartegpu::mipsWieSpiel();
    }));
    add(pruefe("Grafikschnittstelle: die Angaben zum Geraet nennen sie", [] {
        const render::GeraeteInfo i = render::geraeteInfo();
        diag::detail("Knopftest: Geraet " + i.version + ", " + i.karte);
        return !i.karte.empty() && i.version.find(render::current() == render::Backend::Direct3D12 ? "Direct3D 12" : "Direct3D 11") != std::string::npos;
    }));
    add(pruefe("Grafikschnittstelle: die 3D-Ansichten haben Geraet und Kontext", [] { return render::d3dDevice() != nullptr && render::d3dContext() != nullptr; }));
    add(pruefe("Grafikschnittstelle: eine eigene Textur anlegen, ersetzen, teilweise ersetzen", [] {
        std::vector<unsigned char> rgba(64U * 64U * 4U, 200);
        void* t = render::createTexture(rgba.data(), 64, 64);
        if (t == nullptr) { return false; }
        std::fill(rgba.begin(), rgba.end(), static_cast<unsigned char>(90));
        render::updateTexture(t, rgba.data(), 64, 64);
        render::updateTextureBereich(t, rgba.data(), 64, 64, 8, 8, 40, 24);
        return true;
    }));
    add(pause(0.3));
    add(foto("grafik_start"));
    add(klick(tr(Str::MenuView), "###main"));
    add(pause(0.3));
    add(foto("grafik_ansicht"));   // das Menue Ansicht mit dem neuen Eintrag, noch ohne Tooltip
    add(klick(tr(Str::GrafikMenu), "##Menu"));
    add(pause(0.3));
    // Hier steht der Tooltip mit "laeuft jetzt: Direct3D 11/12" im Bild - dieses Foto unterscheidet sich je Schnittstelle um
    // genau diese Ziffer (Bildvergleich: grafik_start, grafik_ansicht und grafik_about muessen gleich sein).
    add(foto("grafik_menue"));
    add(klick(tr(Str::GrafikD3d12), "##Menu"));
    add(pause(0.3));
    add(pruefe("Ansicht > Grafikschnittstelle: Direct3D 12 gewaehlt (Einstellung fuer den naechsten Start)", [] { return g_app->settings.grafikD3d12; }));
    add(pruefe("... die laufende Schnittstelle bleibt bis zum Neustart", [] {
        return render::current() == (grafik::startD3d12() ? render::Backend::Direct3D12 : render::Backend::Direct3D11);
    }));
    add(klick(tr(Str::MenuView), "###main"));
    add(pause(0.3));
    add(klick(tr(Str::GrafikMenu), "##Menu"));
    add(pause(0.3));
    add(klick(tr(Str::GrafikD3d11), "##Menu"));
    add(pause(0.3));
    add(pruefe("Ansicht > Grafikschnittstelle: Direct3D 11 gewaehlt (Rueckfall)", [] { return !g_app->settings.grafikD3d12; }));
    add(pruefe("Die Einstellung steht in der Einstellungsdatei (grafikD3d12)", [] {
        Settings aus = g_app->settings;
        aus.grafikD3d12 = false;
        Settings gelesen;
        return writeSettings(aus).find("grafikD3d12") != std::string::npos && readSettings(writeSettings(aus), gelesen) && !gelesen.grafikD3d12 &&
               Settings{}.grafikD3d12;
    }));
    add(klick(tr(Str::MenuView), "###main"));
    add(pause(0.3));
    add(klick(tr(Str::GrafikMenu), "##Menu"));
    add(pause(0.3));
    add(klick(tr(Str::GrafikD3d12), "##Menu"));
    add(pause(0.3));
    add(pruefe("Ansicht > Grafikschnittstelle: zurueck auf Direct3D 12 (Vorgabe)", [] { return g_app->settings.grafikD3d12; }));
    // Die Mischarten der Oberflaeche (Menue-Editor: render::zeichneStufe mit jeder blendFunc, zeichneAdditiv fuer den Klirrblitz,
    // zeichneOhneMischen fuer Bilder der 3D-Ansichten) als festes Muster ueber einem Farbverlauf: nichts darin haengt an der Zeit,
    // das Foto muss unter Direct3D 11 und Direct3D 12 Bildpunkt fuer Bildpunkt gleich sein. Je Feld ein Viereck mit einer
    // Faktorpaarung (Zeile Quelle, Spalte Ziel, bhed::BlendFactor 0..10); links wiederholt (tcMod), rechts geklemmt (clampMap),
    // die Texturkoordinaten gehen ueber den Rand hinaus.
    add({"Foto grafik_mischarten", [](int b) {
             static void* muster = nullptr;
             static unsigned stand = ~0U;
             if (muster == nullptr || stand != render::texturStand()) {
                 std::vector<unsigned char> rgba(64U * 64U * 4U);
                 for (unsigned y = 0; y < 64U; ++y) {
                     for (unsigned x = 0; x < 64U; ++x) {
                         unsigned char* q = rgba.data() + (y * 64U + x) * 4U;
                         const bool karo = ((x / 8U) + (y / 8U)) % 2U == 0U;
                         q[0] = static_cast<unsigned char>(x * 4U + 3U);
                         q[1] = static_cast<unsigned char>(y * 4U + 3U);
                         q[2] = karo ? static_cast<unsigned char>(230) : static_cast<unsigned char>(40);
                         q[3] = static_cast<unsigned char>(((x + y) * 2U) & 255U);
                     }
                 }
                 muster = render::createTexture(rgba.data(), 64, 64);
                 stand = render::texturStand();
             }
             if (muster == nullptr) { return true; }
             ImDrawList* dl = ImGui::GetForegroundDrawList();
             const ImVec2 o = ImGui::GetMainViewport()->Pos;
             const float feld = 40.0F;
             const ImVec2 a0(o.x + 60.0F, o.y + 80.0F);
             const ImVec2 a1(a0.x + 2.0F * 11.0F * feld + 60.0F, a0.y + 11.0F * feld + 3.0F * feld);
             dl->AddRectFilledMultiColor(a0, a1, IM_COL32(20, 60, 200, 255), IM_COL32(220, 40, 30, 255), IM_COL32(240, 230, 60, 255), IM_COL32(30, 200, 90, 255));
             for (int seite = 0; seite < 2; ++seite) {
                 for (int q = 0; q < 11; ++q) {
                     for (int z = 0; z < 11; ++z) {
                         const float x = a0.x + static_cast<float>(seite) * (11.0F * feld + 60.0F) + static_cast<float>(z) * feld;
                         const float y = a0.y + static_cast<float>(q) * feld;
                         const ImVec2 p[4] = {ImVec2(x + 2.0F, y + 2.0F), ImVec2(x + feld - 2.0F, y + 2.0F), ImVec2(x + feld - 2.0F, y + feld - 2.0F), ImVec2(x + 2.0F, y + feld - 2.0F)};
                         const ImVec2 uv[4] = {ImVec2(-0.25F, -0.25F), ImVec2(1.25F, -0.25F), ImVec2(1.25F, 1.25F), ImVec2(-0.25F, 1.25F)};
                         render::zeichneStufe(dl, muster, p, uv, IM_COL32(255, 220, 200, 200), q, z, seite == 1);
                     }
                 }
             }
             const float yu = a0.y + 11.0F * feld + 0.5F * feld;
             for (int i = 0; i < 6; ++i) {   // additiv mit sechs Eckfarben
                 const float x = a0.x + static_cast<float>(i) * 2.0F * feld;
                 render::zeichneAdditiv(dl, muster, ImVec2(x + 2.0F, yu), ImVec2(x + 2.0F * feld - 2.0F, yu + 2.0F * feld),
                                        IM_COL32(40 * i + 40, 255 - 40 * i, 128, 255 - 30 * i));
             }
             const ImVec2 n0(a0.x + 13.0F * feld, yu);
             render::zeichneOhneMischen(dl);   // ohne Mischen: das Alpha des Bildes zaehlt nicht
             dl->AddImage(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(muster)), n0, ImVec2(n0.x + 4.0F * feld, n0.y + 2.0F * feld));
             dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
             dl->AddImage(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(muster)), ImVec2(n0.x + 4.5F * feld, n0.y), ImVec2(n0.x + 8.5F * feld, n0.y + 2.0F * feld));
             const char* ordner = std::getenv("BHED_FOTOS");
             if (ordner != nullptr && b == 2) { render::fotoAnfordern(std::string(ordner) + "\\grafik_mischarten.bmp"); }
             return b >= 5;
         }});
    // Die Mip-Stufen der Editor-Texturen (kartegpu::bildAnlegen: ID3D11DeviceContext::GenerateMips) als Bild: vier Texturen mit
    // festem Inhalt (Rauschen, Verlauf, harte Kanten; auch Groessen, die keine Zweierpotenz sind), jede Stufe nebeneinander.
    // Direct3D 11 rechnet die Stufen im Treiber, unter Direct3D 12 rechnet sie der Abgeber - das Bild muss gleich sein.
    // D3D12 Runde 2: dasselbe mit dem Schalter kartegpu::mipsWieSpiel (Stufen wie das Spiel, R_MipMap) - dann rechnet weder
    // der Treiber noch der Abgeber, und jede Stufe ist Byte fuer Byte die von TextureSet::Tex::buildMips (abweichend zaehlt).
    static const auto mipBild = [](bool wieSpiel, const char* datei) -> long {
        const char* o = std::getenv("BHED_FOTOS");
        const bool schreiben = o != nullptr && o[0] != '\0';
        if (!kartegpu::verfuegbar()) { return 0; }
        const bool vorher = kartegpu::mipsWieSpiel();
        kartegpu::mipsWieSpiel(wieSpiel);
        long abweichend = 0;
        const int groessen[4][2] = {{64, 64}, {100, 60}, {37, 21}, {256, 16}};
        stufe9c::Bild alle;
        alle.breite = 512;
        alle.hoehe = 64 + 60 + 21 + 16;
        alle.rgba.assign(static_cast<std::size_t>(alle.breite) * static_cast<std::size_t>(alle.hoehe) * 4U, 255);
        std::uint32_t zufall = 2463534242U;
        int oben = 0;
        for (const auto& g : groessen) {
            const int w = g[0];
            const int h = g[1];
            std::vector<unsigned char> rgba(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    unsigned char* q = rgba.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
                    zufall ^= zufall << 13U;
                    zufall ^= zufall >> 17U;
                    zufall ^= zufall << 5U;
                    if (x < w / 3) {   // Rauschen
                        q[0] = static_cast<unsigned char>(zufall);
                        q[1] = static_cast<unsigned char>(zufall >> 8U);
                        q[2] = static_cast<unsigned char>(zufall >> 16U);
                        q[3] = static_cast<unsigned char>(zufall >> 24U);
                    } else if (x < 2 * w / 3) {   // Verlauf
                        q[0] = static_cast<unsigned char>((x * 255) / w);
                        q[1] = static_cast<unsigned char>((y * 255) / h);
                        q[2] = static_cast<unsigned char>(((x + y) * 255) / (w + h));
                        q[3] = static_cast<unsigned char>(255 - (x * 255) / w);
                    } else {   // harte Kanten
                        const bool an = ((x / 3) + (y / 2)) % 2 == 0;
                        q[0] = an ? static_cast<unsigned char>(255) : static_cast<unsigned char>(0);
                        q[1] = an ? static_cast<unsigned char>(1) : static_cast<unsigned char>(254);
                        q[2] = static_cast<unsigned char>((x % 2) * 255);
                        q[3] = an ? static_cast<unsigned char>(128) : static_cast<unsigned char>(127);
                    }
                }
            }
            void* bild = kartegpu::bildAnlegen(rgba.data(), w, h);
            TextureSet::Tex soll;   // die Stufen des Spiels
            soll.width = w;
            soll.height = h;
            soll.rgba = rgba;
            soll.buildMips();
            int links = 0;
            int stufen = 1;
            for (int stufe = 0; bild != nullptr && stufe < stufen; ++stufe) {
                std::vector<unsigned char> s;
                int sw = 0;
                int sh = 0;
                if (!kartegpu::texturLesen(bild, stufe, s, sw, sh, stufen)) {
                    ++abweichend;
                    break;
                }
                if (wieSpiel) {
                    int rw = 0;
                    int rh = 0;
                    const std::uint8_t* r = soll.levelData(stufe, rw, rh);
                    if (r == nullptr || rw != sw || rh != sh || stufen != static_cast<int>(soll.mips.size()) + 1) {
                        ++abweichend;
                    } else {
                        for (std::size_t i = 0; i < s.size(); ++i) { abweichend += s[i] != r[i] ? 1 : 0; }
                    }
                }
                for (int y = 0; y < sh && oben + y < alle.hoehe; ++y) {
                    for (int x = 0; x < sw && links + x < alle.breite; ++x) {
                        // Farbe und Alpha getrennt nebeneinander koennte das Bild verdoppeln - das Alpha geht in den Blaukanal jeder
                        // zweiten Stufe ein: so prueft der Vergleich (er liest nur RGB) alle vier Kanaele.
                        const unsigned char* q = s.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(sw) + static_cast<std::size_t>(x)) * 4U;
                        unsigned char* z = alle.rgba.data() + (static_cast<std::size_t>(oben + y) * static_cast<std::size_t>(alle.breite) + static_cast<std::size_t>(links + x)) * 4U;
                        z[0] = q[0];
                        z[1] = q[1];
                        z[2] = (stufe % 2 == 1) ? q[3] : q[2];
                        z[3] = 255;
                    }
                }
                links += sw + 1;
            }
            if (bild == nullptr) { ++abweichend; }
            if (bild != nullptr) { kartegpu::bildFreigeben(bild); }
            oben += h;
        }
        kartegpu::mipsWieSpiel(vorher);
        if (schreiben) { stufe9c::bmpSchreiben(std::string(o) + "\\" + datei, alle); }
        return abweichend;
    };
    add(tu("Grafikschnittstelle: Mip-Stufen der Editor-Texturen als Bild (grafik_mipstufen)", [] { (void)mipBild(false, "grafik_mipstufen.bmp"); }));
    add(pruefe("Grafikschnittstelle: mit dem Schalter \"Mip-Stufen wie das Spiel\" ist jede Stufe einer Editor-Textur die von R_MipMap (grafik_mipstufen_spiel)",
               [] { return !kartegpu::verfuegbar() || mipBild(true, "grafik_mipstufen_spiel.bmp") == 0; }));
    // Ein Fenster der Oberflaeche ohne 3D-Ansicht (About) fuer den Bildvergleich
    add(klick(tr(Str::MenuHelp)));
    add(pause(0.3));
    add(klick(tr(Str::HelpAbout), "##Menu"));
    add(pause(0.5));
    add(pruefe("Help > About ist offen", [] { return g_app->aboutOpen; }));
    add(foto("grafik_about"));
    add(klick(tr(Str::AboutClose), tr(Str::AboutTitle)));
    add(pause(0.3));
    // Geraeteverlust (rc596-Weg) unter der laufenden Schnittstelle: Neuaufbau. Mit BHED_D3D12_FEHLER=neuaufbau kommt danach kein
    // D3D12-Geraet - dann traegt Direct3D 11 (vorher: drei Versuche, dann Ende des Programms).
    auto aufbauVorher = std::make_shared<int>(0);
    auto vorher = std::make_shared<render::Backend>(render::Backend::Direct3D11);
    add(tu("Grafikschnittstelle: Grafikgeraet verlieren (gespielt)", [=] {
        *aufbauVorher = g_app->grafikNeuAufbauten;
        *vorher = render::current();
        render::geraetVerlorenSpielen();
    }));
    add(warteBis("Grafikschnittstelle: Grafik neu aufgebaut", [=] { return g_app->grafikNeuAufbauten > *aufbauVorher; }, 300));
    add(pause(0.3));
    add(pruefe("Grafikschnittstelle: nach dem Geraeteverlust laeuft wieder eine Schnittstelle (Direct3D 11, wenn Direct3D 12 nicht mehr kommt)", [=] {
        const bool rueckfall = *vorher == render::Backend::Direct3D12 && grafik::fehlerStelle() == "neuaufbau";
        const render::Backend soll = rueckfall ? render::Backend::Direct3D11 : *vorher;
        diag::detail(std::string("Knopftest: nach dem Neuaufbau ") + render::backendName(render::current()) + " (vorher " + render::backendName(*vorher) + ")");
        return render::current() == soll && render::d3dDevice() != nullptr && render::d3dContext() != nullptr;
    }));
    add(pruefe("Grafikschnittstelle: nach dem Neuaufbau eine eigene Textur anlegen", [] {
        std::vector<unsigned char> rgba(32U * 32U * 4U, 120);
        return render::createTexture(rgba.data(), 32, 32) != nullptr;
    }));
    add(pause(0.3));
    add(foto("grafik_neuaufbau"));
    return s;
}
