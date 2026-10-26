// karte_bau_plattform.h - Karten-Editor, Stufe 3: q3map2 starten und seinen
// Rueckkanal annehmen (Windows).
//
// NRC startet jeden Schritt mit Q_Exec (libs/commandlib.cpp: CreateProcess,
// CREATE_NEW_CONSOLE - ein Konsolenfenster je Schritt) und lauscht mit l_net
// (libs/l_net) auf Port 39000. Hier: ohne Fenster (CREATE_NO_WINDOW), die
// Ausgabe ueber eine Pipe lesbar, der Prozess in einem Job (er endet mit dem
// Editor), der Socket nur auf 127.0.0.1 (kein Dialog der Windows-Firewall);
// alles ohne zu warten, jedes Bild ein Blick (watchbsp.cpp: QTimer 25 ms).
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#ifndef BHED_GUI_KARTE_BAU_PLATTFORM_H
#define BHED_GUI_KARTE_BAU_PLATTFORM_H

#include <memory>
#include <string>

namespace bhed::gui::bauplattform {

class Prozess {
public:
    Prozess();
    ~Prozess();
    Prozess(const Prozess&) = delete;
    Prozess& operator=(const Prozess&) = delete;
    // befehlszeile wie bei CreateProcess( nullptr, cmdline, ... ); ordner leer =
    // der des Editors; ausgabe: stdout+stderr ueber eine Pipe (sonst NUL).
    bool starten(const std::string& befehlszeile, const std::string& ordner, bool ausgabe, std::string& fehler);
    [[nodiscard]] std::string lesen();     // was die Pipe gerade hat (ohne zu warten)
    [[nodiscard]] bool laeuft();
    [[nodiscard]] bool warten(int millisekunden);   // true, wenn er (inzwischen) zu Ende ist
    [[nodiscard]] int rueckgabe() const;   // nach dem Ende
    [[nodiscard]] bool gestartet() const;
    void beenden();                        // TerminateProcess (Abbrechen)
    void loslassen();                      // Handles schliessen, ohne zu beenden
    // Stufe 7 (Schnell-Backen): vor starten() - der Prozess laeuft mit Prioritaet "Leerlauf"
    void niedrigePrioritaet(bool an);

private:
    struct Teile;
    std::unique_ptr<Teile> m;
};

class Horcher {
public:
    Horcher();
    ~Horcher();
    Horcher(const Horcher&) = delete;
    Horcher& operator=(const Horcher&) = delete;
    bool lauschen(int port, std::string& fehler);   // Net_ListenSocket( 39000 ), nur 127.0.0.1
    [[nodiscard]] bool annehmen();                  // Net_Accept (ohne zu warten)
    // 1 Daten (angehaengt an daten), 0 nichts, 2 Verbindung zu, -1 Fehler
    int empfangen(std::string& daten);
    void schliessen();                              // Net_Disconnect beider Sockets
    [[nodiscard]] bool verbunden() const;

private:
    struct Teile;
    std::unique_ptr<Teile> m;
};

// Q_Exec( nullptr, cmd, ordner, false, false ): losgeloest starten (die Engine).
bool losgeloestStarten(const std::string& befehlszeile, const std::string& ordner);

}  // namespace bhed::gui::bauplattform

#endif
