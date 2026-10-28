// ki.h - die freiwillige KI-Anbindung im Programm (Stufe H1)
//
// Ab Werk aus. Zwei Wege, dieselben Werkzeuge und dieselben Bestaetigungen:
//   - In-App-Chat mit eigenem Anthropic-API-Schluessel (Abschnitt im Helfer)
//   - MCP-Server fuer eigene KI-Apps (stdio mit --mcp, oder lokal ueber HTTP
//     nur auf 127.0.0.1 mit Token)
// Der reine Teil (Werkzeuge, JSON-RPC, Pruefungen) steht in include/bhed/ki.h.
#ifndef BHED_GUI_KI_H
#define BHED_GUI_KI_H

#include <string>

namespace bhed::gui {

// Im Helfer-Fenster: Abschnitt "Claude fragen" und "Eigene KI-App anschliessen".
void kiAbschnitt(const char* frage, float breite);
// Jedes Bild: Bestaetigungen, Protokoll, Verbindungs-Symbol, Arbeit der
// Werkzeuge im Hauptfaden.
void kiBild();
// Beim Start (main): --mcp gesetzt? Dann laeuft MCP ueber stdin/stdout.
void kiStart(bool mcpStdio);
// Beim Beenden: Server und Faeden anhalten.
void kiEnde();

}  // namespace bhed::gui

#endif
