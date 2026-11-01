// app_menue_ereignisse.inc - Menue-Editor M5 Punkt 2: Ereignisse je Element wie in Unreal UMG
// (in app_menue.cpp eingebunden). Bausteine schreiben den Skripttext des Ereignisses; jede
// Aenderung ist ein Rueckgaengig-Schritt und steht sofort im Text-Reiter.

namespace {

const Str kEreignisNamen[] = {Str::MeEvHover, Str::MeEvHoverEnde, Str::MeEvKlick, Str::MeEvFokus, Str::MeEvFokusEnde, Str::MeEvEnter};

std::string skriptVon(const Item& it, int art) {
    switch (art) {
        case 0: return it.mouseEnter;
        case 1: return it.mouseExit;
        case 2: return it.action;
        case 3: return it.onFocus;
        case 4: return it.leaveFocus;
        default: return it.accept;
    }
}

void ereignisSchreiben(Dokument& d, int item, const char* schluessel, const std::vector<Baustein>& b) {
    std::vector<Ersetzung> e;
    if (skriptSetzen(d.datei, menuDerSeite(d), item, schluessel, zusammensetzen(b), e)) { aendere(std::move(e)); }
    else { ausMakroMelden(); }
}

// Probehoeren ueber denselben Weg wie die Klaenge der Vorschau (Ton-Schalter, stumm im Selbsttest)
void probehoeren(const std::string& klang) {
    if (klang.empty()) { return; }
    Z().lauf.klaenge.push_back(klang);
    spieleKlaenge();
}

// Vorgabe-Woerter eines neuen Bausteins
Baustein neuerBaustein(const std::string& befehl, const Item& it, const Menu& m) {
    Baustein b;
    b.befehl = befehl;
    const std::string& n = it.window.name;
    const Rechteck& r = it.window.rectClient;
    if (befehl == "show" || befehl == "hide" || befehl == "fadein" || befehl == "fadeout" || befehl == "setfocus") { b.args = {n}; }
    else if (befehl == "setitemcolor") { b.args = {n, "forecolor", "1", "1", "1", "1"}; }
    else if (befehl == "setcolor") { b.args = {"forecolor", "1", "1", "1", "1"}; }
    else if (befehl == "setitemrect") { b.args = {n, zahlText(r.x), zahlText(r.y), zahlText(r.w), zahlText(r.h)}; }
    else if (befehl == "setitemtext") { b.args = {n, "text"}; }
    else if (befehl == "transition") {
        b.args = {n, zahlText(r.x), zahlText(r.y), zahlText(r.w), zahlText(r.h), zahlText(r.x - 4), zahlText(r.y - 4), zahlText(r.w + 8),
                  zahlText(r.h + 8), "10", "1"};
    } else if (befehl == "play" || befehl == "playlooped") { b.args = {"sound/interface/button1.wav"}; }
    else if (befehl == "open" || befehl == "close") { b.args = {m.window.name}; }
    else if (befehl == "setcvar") { b.args = {"ui_name", "1"}; }
    else if (befehl == "setcvartocvar") { b.args = {"ui_ziel", "ui_quelle"}; }
    else if (befehl == "uiScript") { b.args = {"character"}; }
    else if (befehl == "exec") { b.args = {"echo hallo"}; }
    b.zitiert.assign(b.args.size(), befehl == "play" || befehl == "playlooped" || befehl == "exec");
    return b;
}

}  // namespace
