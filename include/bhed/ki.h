// ki.h - die freiwillige KI-Anbindung: Werkzeuge, MCP, Pruefungen, Chat (Stufe H1)
//
// shank: eine KI im Programm, "die dir aktiv zeigt, wie du das erreichst",
// und "dass sie fuer dich was macht in dem Programm" - ohne dass jemand ueber
// sie angegriffen werden kann. Dieser Teil ist rein (ohne Netz, ohne
// Oberflaeche) und in tests/kitest.cpp geprueft; Netz, DPAPI und Zufall stehen
// in src/ki_win32.cpp, die Ausfuehrung der Werkzeuge in gui/ki.cpp.
//
// Sicherheitskonzept (docs/intern/NOTIZEN_hilfe1.md, nach OWASP MCP Security
// Cheat Sheet und den Security Best Practices der MCP-Spezifikation):
//   - ab Werk aus; nur bewusst einschalten
//   - geringste Rechte: acht feste Werkzeuge; kein Dateisystem, keine Shell,
//     kein Netz, kein beliebiger Code. Aendern nur ueber die vorhandenen
//     Befehle des Programms (Menue/Tasten), nie Dateien speichern/loeschen.
//   - Werkzeugbeschreibungen stehen fest hier, nicht in Dateien
//   - Eingaben streng geprueft (bekannte Felder, Typen, Laengen, Muster)
//   - Ergebnisse mit Inhalten aus Karten, Skripten, Menues, Handbuch sind als
//     DATEN markiert (alsDaten), nie als Anweisung
//   - aendernde Auftraege nur nach Bestaetigung im Programm, hoechstens
//     kMaxBefehle Befehle je Auftrag, ein Auftrag = ein Rueckgaengig-Schritt
//   - HTTP nur auf 127.0.0.1, Host- und Origin-Pruefung (DNS-Rebinding),
//     Bearer-Token in konstanter Zeit, keine CORS-Freigaben, begrenzte
//     Groesse und Rate; stdio bevorzugt
//   - der API-Schluessel geht nur an api.anthropic.com (TLS, WinHTTP prueft
//     das Zertifikat), nie in Protokolle
//
// Teile der HTTP-Pruefung folgen der Pruefliste von WhiteoutFlakes
// (tools/basic_viewer/mcp/http_guard.cpp, Copyright (c) 2026 Fernando
// Sahmkow, BSD-3-Clause; siehe THIRD-PARTY.md) - neu geschrieben, ohne
// dessen Lua-Werkzeuge.
#ifndef BHED_KI_H
#define BHED_KI_H

#include "bhed/json.h"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bhed::ki {

// --- Werkzeuge ---------------------------------------------------------------------

enum class Wirkung { Lesen, Zeigen, Aendern };

struct Werkzeug {
    const char* name;
    const char* beschreibung;   // fest im Programm (Tool-Poisoning)
    const char* schema;         // JSON-Schema der Eingabe
    Wirkung wirkung;
};

constexpr std::size_t kMaxBefehle = 20;        // je Auftrag
constexpr std::size_t kMaxAnfrage = 1u << 20;  // Bytes je Nachricht

[[nodiscard]] const std::vector<Werkzeug>& werkzeuge();
[[nodiscard]] const Werkzeug* werkzeug(std::string_view name);
// Streng: nur bekannte Felder, richtige Typen, Laengen und Muster.
[[nodiscard]] bool pruefeEingabe(const Werkzeug& w, const json::Wert& eingabe, std::string* fehler);

// Ein Befehl in run_commands: "map:<NRC-Befehl>", "script:<Tour-Aktion>",
// "mode:<0-3>", "view:<0-2>". Prueft nur die Form; ob es den Befehl gibt,
// prueft der Gastgeber.
struct Befehl {
    std::string art;    // map, script, mode, view
    std::string wert;
    bool datei = false; // oeffnet/speichert/schreibt/beendet: braucht IMMER eine Bestaetigung
};
[[nodiscard]] bool zerlegeBefehl(const std::string& s, Befehl& out, std::string* fehler);
// Braucht dieser Auftrag eine Rueckfrage? Immer, wenn ein Befehl Dateien
// beruehrt; sonst nur, wenn "ohne Nachfrage" aus ist.
[[nodiscard]] bool brauchtBestaetigung(const std::vector<Befehl>& befehle, bool ohneNachfrage);

struct Ergebnis {
    bool fehler = false;
    std::string text;      // fuer die KI (bei Programminhalten: alsDaten)
    std::string bildPng;   // PNG (roh), nur screenshot
};

// Der Gastgeber fuehrt ein Werkzeug aus (Eingabe ist schon geprueft).
using Ausfuehren = std::function<Ergebnis(const std::string& name, const json::Wert& eingabe)>;

// Inhalte aus Dateien des Anwenders als DATEN fuer die KI: ein JSON-Objekt
// mit dem Hinweis, dass nichts darin eine Anweisung ist.
[[nodiscard]] std::string alsDaten(const std::string& quelle, const json::Wert& inhalt);

// Base64 (fuer Bilder).
[[nodiscard]] std::string base64(const std::string& roh);
// Ein PNG (RGB, ungepackte Deflate-Bloecke - ohne zlib) aus BGRA-Zeilen, auf
// hoechstens `maxBreite` Pixel verkleinert (screenshot).
[[nodiscard]] std::string pngAusBgra(const unsigned char* bgra, int breite, int hoehe, int zeilenAbstand, int maxBreite);

// --- MCP (JSON-RPC 2.0) --------------------------------------------------------------

struct McpSitzung {
    bool initialisiert = false;
    std::string version;
    std::string client;
};
[[nodiscard]] const std::vector<std::string>& mcpVersionen();
// Eine Nachricht bearbeiten. Antwort leer bei einer Benachrichtigung.
[[nodiscard]] std::string mcpBearbeite(const std::string& nachricht, McpSitzung& s, const Ausfuehren& tu);

// --- HTTP (Streamable HTTP, nur 127.0.0.1) -------------------------------------------

struct HttpAnfrage {
    std::string methode;
    std::string pfad;
    std::vector<std::pair<std::string, std::string>> kopf;
    std::string koerper;
    [[nodiscard]] const std::string* feld(std::string_view name) const;   // ohne Gross/klein
};
// Anfragekopf und -koerper lesen. 0 = vollstaendig, 1 = mehr Bytes noetig,
// sonst ein HTTP-Fehlerstatus (400, 411, 413, 431).
[[nodiscard]] int leseHttp(const std::string& roh, HttpAnfrage& out, std::size_t hoechstens);
struct Urteil {
    int status = 0;   // 0 = durchlassen
    std::string grund;
};
// Host, Origin, Token, Methode, Pfad, Accept, Content-Type, Protokollfassung.
[[nodiscard]] Urteil pruefeHttp(const HttpAnfrage& a, int port, const std::string& token);
// Gleich in einer Zeit, die nicht davon abhaengt, wo sich die Zeichenketten unterscheiden.
[[nodiscard]] bool gleichesGeheimnis(std::string_view a, std::string_view b);
[[nodiscard]] std::string httpAntwort(int status, const std::string& koerper, const std::string& art = "application/json");

// Hoechstens n Anfragen je Sekunde (gleitend).
class Bremse {
public:
    explicit Bremse(int jeSekunde) : max_(jeSekunde) {}
    bool darf(double jetzt);

private:
    int max_;
    std::vector<double> zeiten_;
};

// --- Messages API (In-App-Chat) --------------------------------------------------------

constexpr const char* kApiHost = "api.anthropic.com";
constexpr const char* kApiPfad = "/v1/messages";
constexpr const char* kApiFassung = "2023-06-01";
[[nodiscard]] const std::vector<std::string>& chatModelle();   // Vorgabe zuerst

struct Aufruf {
    std::string id;
    std::string name;
    json::Wert eingabe;
};
struct ChatAntwort {
    std::string stopGrund;
    std::string text;
    std::vector<Aufruf> aufrufe;
    json::Wert inhalt;   // die ganze content-Liste (zum Zurueckschicken, mit thinking)
};
[[nodiscard]] std::string chatKoerper(const std::string& modell, const std::string& system, const std::vector<json::Wert>& nachrichten);
// Die Kopfzeilen der Anfrage (der Schluessel kommt als x-api-key dazu).
[[nodiscard]] std::vector<std::pair<std::string, std::string>> chatKopf(const std::string& modell, const std::string& schluessel);
[[nodiscard]] bool leseChatAntwort(const std::string& json, ChatAntwort& out, std::string* fehler);
// Der feste Systemtext mit Handbuch-Ausschnitten als markierte Daten.
[[nodiscard]] std::string systemText(const std::string& auszuegeJson);

// Senden: Koerper hin, Status und Antwort zurueck.
using Senden = std::function<bool(const std::string& koerper, int& status, std::string& antwort, std::string& fehler)>;
struct ChatErgebnis {
    std::string text;
    std::vector<std::string> werkzeuge;   // was die KI aufgerufen hat
    std::string fehler;
};
// Frage -> Antwort, Werkzeuge -> Ergebnisse -> ... (hoechstens `runden`).
// `abbrechen` wird vor jeder Runde gefragt (Not-Aus).
[[nodiscard]] ChatErgebnis chat(const std::string& modell, const std::string& system, const std::string& frage, const Senden& senden,
                                const Ausfuehren& tu, const std::function<bool()>& abbrechen, int runden = 6);

// --- Windows (src/ki_win32.cpp) --------------------------------------------------------

// DPAPI (CryptProtectData, an den angemeldeten Benutzer gebunden).
[[nodiscard]] bool schuetzen(const std::string& klar, std::string& blob);
[[nodiscard]] bool oeffnen(const std::string& blob, std::string& klar);
// 32 Zufallsbytes (BCryptGenRandom) als 64 Hexzeichen.
[[nodiscard]] std::string zufallsToken();
// POST ueber WinHTTP. `sicher` = TLS mit Zertifikatspruefung (Vorgabe von WinHTTP).
[[nodiscard]] bool httpPost(const std::string& host, int port, bool sicher, const std::string& pfad,
                            const std::vector<std::pair<std::string, std::string>>& kopf, const std::string& koerper, int& status,
                            std::string& antwort, std::string& fehler, int zeitlimitMs = 60000);

// Der lokale MCP-Server (nur 127.0.0.1). Bearbeiten laeuft im Serverfaden.
class HttpServer {
public:
    using Bearbeiten = std::function<std::string(const std::string& nachricht)>;
    HttpServer() = default;
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    ~HttpServer();
    bool starte(int port, const std::string& token, Bearbeiten b, std::string* fehler);
    void halte();
    [[nodiscard]] bool laeuft() const;
    [[nodiscard]] int port() const { return port_; }
    [[nodiscard]] int abgewiesen() const;   // Zahl abgewiesener Anfragen

private:
    struct Innen;
    Innen* innen_ = nullptr;
    int port_ = 0;
};

// stdio: Zeilen von stdin lesen, Antworten nach stdout (eigener Faden).
class StdioServer {
public:
    using Bearbeiten = std::function<std::string(const std::string& nachricht)>;
    StdioServer() = default;
    StdioServer(const StdioServer&) = delete;
    StdioServer& operator=(const StdioServer&) = delete;
    ~StdioServer();
    bool starte(Bearbeiten b);
    void halte();
    [[nodiscard]] bool laeuft() const;

private:
    struct Innen;
    Innen* innen_ = nullptr;
};

}  // namespace bhed::ki

#endif
