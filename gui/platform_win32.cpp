#include "platform.h"
#include "wache.h"

#include "bhed/diag.h"
#include "bhed/umzug.h"

#include <cstdlib>

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <commdlg.h>
#include <shlobj.h>

#include <algorithm>
#include <filesystem>
#include <cctype>
#include <cwchar>
#include <string>
#include <vector>

namespace bhed::platform {
namespace {

std::wstring toWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int n = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                                      static_cast<int>(text.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        out.data(), n);
    return out;
}

std::string toUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int n = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
                                      static_cast<int>(text.size()), nullptr, 0,
                                      nullptr, nullptr);
    std::string out(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        out.data(), n, nullptr, nullptr);
    return out;
}

// Die Dateidialoge - ueber den Common Item Dialog, nicht ueber
// GetOpenFileNameW.
//
// Microsoft sagt es deutlich: "Starting with Windows Vista, the Open and Save
// As common dialog boxes have been superseded by the Common Item Dialog. We
// recommended that you use the Common Item Dialog API instead."
//
// Drei Gruende, die hier zaehlen:
//
//   * Es gibt einen echten ORDNERDIALOG (FOS_PICKFOLDERS). Vorher musste man
//     eine .pk3 im gewuenschten Ordner auswaehlen, damit das Programm den
//     Ordner davon nehmen konnte - eine Kruecke, die im Quelltext auch so
//     kommentiert war.
//   * Der Dialog ist der, den der Benutzer aus jedem anderen Programm kennt:
//     groessenveraenderlich, mit Seitenleiste und zuletzt benutzten Orten.
//   * Der Startordner ist nur ein VORSCHLAG (SetDefaultFolder). Windows merkt
//     sich je Programm, wo man zuletzt war, und das ist meistens nuetzlicher.
//
// COM muss angemeldet sein, bevor das hier laeuft - CoInitializeEx steht in
// wWinMain. Fehlt es, gibt CoCreateInstance CO_E_NOTINITIALIZED zurueck und
// der Dialog geht stillschweigend gar nicht auf.

// Aus "Scripts (*.icarus)|*.icarus|All files (*.*)|*.*" die Paare bauen, die
// SetFileTypes erwartet. Die Zeichenketten muessen leben, solange der Dialog
// laeuft - deshalb bleiben sie im Aufrufer stehen.
struct FilterSpec {
    std::vector<std::wstring> texts;
    std::vector<COMDLG_FILTERSPEC> specs;
};

FilterSpec buildFilter(const char* filter) {
    FilterSpec out;
    if (filter == nullptr) {
        return out;
    }
    std::vector<std::string> parts;
    std::string cur;
    for (const char* p = filter; *p != '\0'; ++p) {
        if (*p == '|') {
            parts.push_back(cur);
            cur.clear();
        } else {
            cur += *p;
        }
    }
    if (!cur.empty()) {
        parts.push_back(cur);
    }
    // Paarweise: Name, Muster.
    out.texts.reserve(parts.size());
    for (const std::string& part : parts) {
        out.texts.push_back(toWide(part));
    }
    for (std::size_t i = 0; i + 1 < out.texts.size(); i += 2) {
        out.specs.push_back(COMDLG_FILTERSPEC{out.texts[i].c_str(),
                                              out.texts[i + 1].c_str()});
    }
    return out;
}

std::string runDialog(const char* title, const char* filter,
                      const std::string& startDir, const std::string& suggested,
                      bool save, bool folder) {
    // Im Selbsttest KEIN Dialog: er hielte den Lauf an (unsichtbar hinter
    // dem Testfenster). Wie "Abbrechen" behandeln und ins Protokoll.
    if (std::getenv("BHED_EDITORTEST") != nullptr) {
        diag::detail(std::string("Selbsttest: Dateidialog \"") + (title != nullptr ? title : "") +
                     "\" unterdrueckt (wie Abbrechen)");
        return {};
    }
    IFileDialog* dialog = nullptr;
    const CLSID clsid = save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    const IID iid = save ? IID_IFileSaveDialog : IID_IFileOpenDialog;
    if (FAILED(CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, iid,
                                reinterpret_cast<void**>(&dialog))) ||
        dialog == nullptr) {
        return {};
    }

    DWORD options = 0;
    if (SUCCEEDED(dialog->GetOptions(&options))) {
        options |= FOS_FORCEFILESYSTEM;   // keine virtuellen Orte
        if (folder) {
            options |= FOS_PICKFOLDERS;
        } else if (!save) {
            options |= FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST;
        } else {
            options |= FOS_OVERWRITEPROMPT;
        }
        dialog->SetOptions(options);
    }

    const std::wstring wtitle = toWide(title);
    dialog->SetTitle(wtitle.c_str());

    FilterSpec spec;
    if (!folder && filter != nullptr) {
        spec = buildFilter(filter);
        if (!spec.specs.empty()) {
            dialog->SetFileTypes(static_cast<UINT>(spec.specs.size()),
                                 spec.specs.data());
        }
    }

    if (!suggested.empty()) {
        dialog->SetFileName(toWide(suggested).c_str());
    }

    // Der Startordner ist ein VORSCHLAG: SetDefaultFolder statt
    // SetFolder. Damit hat der Ort, an dem der Benutzer zuletzt war, Vorrang
    // - so verhaelt sich jedes andere Programm auch.
    if (!startDir.empty()) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(
                toWide(startDir).c_str(), nullptr, IID_PPV_ARGS(&item))) &&
            item != nullptr) {
            dialog->SetDefaultFolder(item);
            item->Release();
        }
    }

    std::string result;
    // Der Dialog haelt die Hauptschleife an - das ist kein Haenger (rc596,
    // gui/wache.h).
    bhed::gui::wache::modalBeginn();
    const HRESULT gezeigt = dialog->Show(nullptr);
    bhed::gui::wache::modalEnde();
    if (SUCCEEDED(gezeigt)) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item)) && item != nullptr) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) &&
                path != nullptr) {
                result = toUtf8(path);
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dialog->Release();
    return result;
}

}  // namespace

std::string openFileDialog(const char* title, const char* filter,
                           const std::string& startDir) {
    return runDialog(title, filter, startDir, {}, false, false);
}

std::string pickFolder(const char* title, const std::string& startDir) {
    return runDialog(title, nullptr, startDir, {}, false, true);
}

std::string saveFileDialog(const char* title, const char* filter,
                           const std::string& startDir,
                           const std::string& suggestedName) {
    return runDialog(title, filter, startDir, suggestedName, true, false);
}

Answer askSaveChanges(const std::string& question, const char* title) {
    const int r = MessageBoxW(nullptr, toWide(question).c_str(), toWide(title).c_str(),
                              MB_YESNOCANCEL | MB_ICONQUESTION);
    if (r == IDYES) {
        return Answer::Yes;
    }
    if (r == IDNO) {
        return Answer::No;
    }
    return Answer::Cancel;
}

void showError(const std::string& message, const char* title) {
    // Im Selbsttest KEIN Fenster: es hielte den Lauf an, und shank saesse
    // davor. Stattdessen ins Protokoll - der Test sieht es dort.
    if (std::getenv("BHED_EDITORTEST") != nullptr) {
        diag::detail("Selbsttest: Fehlermeldung statt Dialog: " + message);
        return;
    }
    MessageBoxW(nullptr, toWide(message).c_str(), toWide(title).c_str(), MB_ICONERROR);
}

void openWithDefaultApp(const std::string& path) {
    // "open" statt eines festen Programms: welcher Editor eingestellt ist,
    // entscheidet Windows.
    ShellExecuteW(nullptr, L"open", toWide(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

bool startBehavedWith(const std::string& datei) {
    std::vector<wchar_t> exe(MAX_PATH);
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, exe.data(),
                                           static_cast<DWORD>(exe.size()));
        if (n == 0) {
            return false;
        }
        if (n < exe.size()) {
            break;
        }
        exe.resize(exe.size() * 2);
    }
    const std::wstring arg = L"\"" + toWide(datei) + L"\"";
    const HINSTANCE r = ShellExecuteW(nullptr, L"open", exe.data(), arg.c_str(),
                                      nullptr, SW_SHOWNORMAL);
    // ShellExecute meldet Erfolg mit einem Wert ueber 32.
    return reinterpret_cast<INT_PTR>(r) > 32;
}

std::string settingsDirectory() {
    // Selbsttests, die gleichzeitig laufen, bekommen je einen eigenen Ordner
    // (rc601): sonst schreiben sie sich gegenseitig - und dem Anwender - die
    // behaved.cfg ueber.
    if (const wchar_t* eigen = _wgetenv(L"BHED_EINSTELLUNGEN"); eigen != nullptr && eigen[0] != L'\0') {
        CreateDirectoryW(eigen, nullptr);
        return toUtf8(eigen);
    }
    // Stufe 8c (Umbenennung in JK Studio): %APPDATA%\jkstudio statt %APPDATA%\behaved.
    // Beim ersten Start wird der alte Ordner kopiert (bhed/umzug.h), das Original
    // bleibt. BHED_APPDATA ersetzt im Test den Ordner %APPDATA% (der echte des
    // Anwenders wird dann nicht angefasst).
    std::string basis;
    if (const wchar_t* test = _wgetenv(L"BHED_APPDATA"); test != nullptr && test[0] != L'\0') {
        basis = toUtf8(test);
    } else {
        wchar_t* appdata = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
            basis = toUtf8(appdata);
            CoTaskMemFree(appdata);
        }
    }
    if (basis.empty()) { return {}; }
    const std::string dir = basis + "\\" + umzug::kNeuerOrdner;
    static bool versucht = false;
    if (!versucht) {
        versucht = true;
        const auto pfad = [](const std::string& s) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size())); };
        const umzug::Ergebnis e = umzug::uebernehmen(pfad(basis + "\\" + umzug::kAlterOrdner), pfad(dir));
        if (e.uebernommen || !e.fehler.empty()) {
            diag::detail("Einstellungen: " + std::string(umzug::kAlterOrdner) + " -> " + umzug::kNeuerOrdner + ": " +
                         (e.uebernommen ? std::to_string(e.dateien) + " Dateien kopiert, " + std::to_string(e.ausgelassen) + " ausgelassen (das Original bleibt)"
                                        : "Fehler: " + e.fehler));
        }
    }
    CreateDirectoryW(toWide(dir).c_str(), nullptr);
    return dir;
}


std::string ressource(const wchar_t* name) {
    HRSRC r = FindResourceW(nullptr, name, MAKEINTRESOURCEW(10));   // RT_RCDATA
    if (r == nullptr) { return {}; }
    HGLOBAL g = LoadResource(nullptr, r);
    const DWORD n = SizeofResource(nullptr, r);
    const void* p = g != nullptr ? LockResource(g) : nullptr;
    if (p == nullptr || n == 0) { return {}; }
    return std::string(static_cast<const char*>(p), n);
}


std::string dokumenteOrdner() {
    wchar_t* docs = nullptr;
    std::string dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs))) {
        dir = toUtf8(docs);
        CoTaskMemFree(docs);
    }
    return dir;
}

void openInExplorer(const std::string& folder) {
    ShellExecuteW(nullptr, L"open", toWide(folder).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

std::pair<int, int> hauptfensterGroesse(int breite, int hoehe) {
    // Das sichtbare Hauptfenster DIESES Fadens (die Oberflaeche laeuft im
    // Hauptfaden, der Selbsttest auch).
    HWND fenster = nullptr;
    EnumThreadWindows(
        GetCurrentThreadId(),
        [](HWND h, LPARAM l) -> BOOL {
            if (IsWindowVisible(h) != 0 && GetWindow(h, GW_OWNER) == nullptr) {
                *reinterpret_cast<HWND*>(l) = h;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&fenster));
    if (fenster == nullptr) {
        return {0, 0};
    }
    RECT r{};
    GetWindowRect(fenster, &r);
    const std::pair<int, int> vorher{r.right - r.left, r.bottom - r.top};
    if (IsZoomed(fenster) != 0) {
        ShowWindow(fenster, SW_SHOWNOACTIVATE);
    }
    SetWindowPos(fenster, nullptr, 0, 0, breite, hoehe, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    return vorher;
}

std::uintptr_t starteZweiteInstanz(const std::string& argument) {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring zeile = L"\"" + std::wstring(exe) + L"\"";
    if (!argument.empty()) {
        zeile += L" \"" + toWide(argument) + L"\"";
    }
    // Die Umgebung ohne BHED_EDITORTEST.
    std::wstring umgebung;
    if (wchar_t* alle = GetEnvironmentStringsW()) {
        for (const wchar_t* e = alle; *e != L'\0'; e += std::wcslen(e) + 1) {
            if (std::wcsncmp(e, L"BHED_EDITORTEST=", 16) == 0) {
                continue;
            }
            umgebung.append(e);
            umgebung.push_back(L'\0');
        }
        FreeEnvironmentStringsW(alle);
    }
    umgebung.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWMINNOACTIVE;   // kein Fokusdiebstahl
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(nullptr, zeile.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                       umgebung.data(), nullptr, &si, &pi) == 0) {
        return 0;
    }
    CloseHandle(pi.hThread);
    return reinterpret_cast<std::uintptr_t>(pi.hProcess);
}

long zweiteInstanzStand(std::uintptr_t griff) {
    if (griff == 0) {
        return -2;
    }
    const HANDLE h = reinterpret_cast<HANDLE>(griff);
    if (WaitForSingleObject(h, 0) != WAIT_OBJECT_0) {
        return -1;
    }
    DWORD code = 0;
    GetExitCodeProcess(h, &code);
    return static_cast<long>(code);
}

void zweiteInstanzSchliessen(std::uintptr_t griff, bool beenden) {
    if (griff == 0) {
        return;
    }
    const HANDLE h = reinterpret_cast<HANDLE>(griff);
    if (beenden && WaitForSingleObject(h, 0) != WAIT_OBJECT_0) {
        TerminateProcess(h, 99);
        WaitForSingleObject(h, 5000);
    }
    CloseHandle(h);
}

// --- rc601 release: ein anderes Programm starten (Selbsttest) -----------
//
// Wie starteZweiteInstanz, aber fuer eine andere Datei (die Kopie der exe im
// Ordner ohne data\base) und mit eigenen Variablen - der Selbsttest gibt der
// Kopie einen eigenen Einstellungsordner (BHED_EINSTELLUNGEN), damit sie
// weder den des Laufs noch %APPDATA%\behaved anfasst.
std::uintptr_t starteProgramm(const std::string& exe, const std::string& argumente,
                              const std::vector<std::string>& setzen) {
    const std::wstring pfad = toWide(exe);
    std::wstring zeile = L"\"" + pfad + L"\"";
    if (!argumente.empty()) {
        zeile += L" " + toWide(argumente);
    }
    std::vector<std::wstring> neu;
    for (const std::string& v : setzen) {
        neu.push_back(toWide(v));
    }
    // Die eigene Umgebung ohne BHED_EDITORTEST und ohne die Namen aus
    // `setzen`; danach die neuen Werte. Sortiert (ohne Ruecksicht auf
    // Gross-/Kleinschreibung), so wie Windows einen Umgebungsblock erwartet.
    std::vector<std::wstring> alle;
    if (wchar_t* block = GetEnvironmentStringsW()) {
        for (const wchar_t* e = block; *e != L'\0'; e += std::wcslen(e) + 1) {
            bool weg = std::wcsncmp(e, L"BHED_EDITORTEST=", 16) == 0;
            for (const std::wstring& n : neu) {
                const std::size_t gleich = n.find(L'=');
                if (gleich != std::wstring::npos && _wcsnicmp(e, n.c_str(), gleich + 1) == 0) {
                    weg = true;
                }
            }
            if (!weg) {
                alle.emplace_back(e);
            }
        }
        FreeEnvironmentStringsW(block);
    }
    for (const std::wstring& n : neu) {
        alle.push_back(n);
    }
    std::sort(alle.begin(), alle.end(),
              [](const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) < 0; });
    std::wstring umgebung;
    for (const std::wstring& e : alle) {
        umgebung.append(e);
        umgebung.push_back(L'\0');
    }
    umgebung.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWMINNOACTIVE;   // kein Fokusdiebstahl
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(pfad.c_str(), zeile.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                       umgebung.data(), nullptr, &si, &pi) == 0) {
        return 0;
    }
    CloseHandle(pi.hThread);
    return reinterpret_cast<std::uintptr_t>(pi.hProcess);
}

std::string executableDirectory() {
    // GetModuleFileNameW mit wachsendem Puffer: der Pfad kann laenger als
    // MAX_PATH sein, und die Funktion sagt das nur durch Abschneiden.
    std::wstring buf(MAX_PATH, L'\0');
    for (int attempt = 0; attempt < 5; ++attempt) {
        const DWORD n = GetModuleFileNameW(nullptr, buf.data(),
                                           static_cast<DWORD>(buf.size()));
        if (n == 0) {
            return {};
        }
        if (n < buf.size()) {
            buf.resize(n);
            break;
        }
        buf.resize(buf.size() * 2);
    }
    const std::size_t slash = buf.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return {};
    }
    return toUtf8(buf.substr(0, slash));
}

std::vector<std::string> listDirectory(const std::string& dir) {
    std::vector<std::string> out;
    if (dir.empty()) {
        return out;
    }
    // FindFirstFileW mit Sternchen. Unterordner werden uebersprungen: es geht
    // um Dateien NEBEN dem Modell, nicht um einen Baum.
    const std::wstring pattern = toWide(dir) + L"\\*";
    WIN32_FIND_DATAW data{};
    HANDLE h = FindFirstFileW(pattern.c_str(), &data);
    if (h == INVALID_HANDLE_VALUE) {
        return out;
    }
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        out.push_back(toUtf8(data.cFileName));
    } while (FindNextFileW(h, &data) != 0);
    FindClose(h);
    return out;
}


// --- RenderDoc ---------------------------------------------------------
//
// Nur die eine Funktion von Hand erklaert, statt renderdoc_app.h
// mitzuliefern. Die Fassungsnummer 1.0.0 ist die aelteste, die
// `TriggerCapture` kennt; RenderDoc gibt auch bei neueren Fassungen eine
// abwaertsvertraegliche Tabelle zurueck.
namespace {

struct RenderDocApi110 {
    void* rest1[6];
    void (*TriggerCapture)();
    // Der Rest der Tabelle wird nicht angefasst.
};

using GetApiFn = int (*)(int version, void** outAPIPointers);

RenderDocApi110* holeRenderDoc() {
    static RenderDocApi110* api = nullptr;
    static bool versucht = false;
    if (versucht) {
        return api;
    }
    versucht = true;
    // GetModuleHandle, NICHT LoadLibrary: wir wollen RenderDoc nicht
    // nachtraeglich hineinladen - das ginge ohnehin nicht -, sondern nur
    // wissen, ob es schon da ist.
    HMODULE m = GetModuleHandleA("renderdoc.dll");
    if (m == nullptr) {
        return nullptr;
    }
    auto* hole = reinterpret_cast<GetApiFn>(
        reinterpret_cast<void*>(GetProcAddress(m, "RENDERDOC_GetAPI")));
    if (hole == nullptr) {
        return nullptr;
    }
    void* p = nullptr;
    // 10100 = eRENDERDOC_API_Version_1_1_0
    if (hole(10100, &p) != 1) {
        return nullptr;
    }
    api = static_cast<RenderDocApi110*>(p);
    return api;
}

}  // namespace

bool renderDocDa() { return holeRenderDoc() != nullptr; }

void renderDocAufnehmen() {
    RenderDocApi110* api = holeRenderDoc();
    if (api != nullptr && api->TriggerCapture != nullptr) {
        api->TriggerCapture();
    }
}

// --- Paket kamera, rc601 -------------------------------------------------
//
// SM_CXSCREEN/SM_CYSCREEN: Breite und Hoehe des HAUPTbildschirms - der, den
// SDL als Bildschirm 0 meldet und dessen Desktopaufloesung Movie Duels bei
// r_mode -2 nimmt. Ohne DPI-Bewusstsein liefert Windows skalierte Werte, das
// Seitenverhaeltnis bleibt dabei dasselbe.
float hauptBildschirmSeiten() {
    const int b = GetSystemMetrics(SM_CXSCREEN);
    const int h = GetSystemMetrics(SM_CYSCREEN);
    return (b > 0 && h > 0) ? static_cast<float>(b) / static_cast<float>(h) : 0.0F;
}

bool zeigerLage(int& x, int& y) {
    POINT p{};
    if (GetCursorPos(&p) == 0) { return false; }
    x = p.x;
    y = p.y;
    return true;
}

bool zeigerImFenster(void* fenster, int& x, int& y) {
    POINT p{};
    if (fenster == nullptr || GetCursorPos(&p) == 0 || ScreenToClient(static_cast<HWND>(fenster), &p) == 0) { return false; }
    x = p.x;
    y = p.y;
    return true;
}

bool zeigerImFensterSetzen(void* fenster, int x, int y) {
    POINT p{x, y};
    if (fenster == nullptr || ClientToScreen(static_cast<HWND>(fenster), &p) == 0) { return false; }
    return SetCursorPos(p.x, p.y) != 0;
}

int gdiTextBreite(const std::string& utf8, const wchar_t* schrift, int pixel, bool fett) {
    static HDC dc = CreateCompatibleDC(nullptr);
    struct Eintrag {
        std::wstring name;
        int pixel;
        bool fett;
        HFONT font;
    };
    static std::vector<Eintrag> schriften;
    if (dc == nullptr) { return -1; }
    HFONT font = nullptr;
    for (const Eintrag& e : schriften) {
        if (e.pixel == pixel && e.fett == fett && e.name == schrift) { font = e.font; }
    }
    if (font == nullptr) {
        font = CreateFontW(-pixel, 0, 0, 0, fett ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, schrift);
        if (font == nullptr) { return -1; }
        schriften.push_back({schrift, pixel, fett, font});
    }
    SelectObject(dc, font);
    const std::wstring w = toWide(utf8);
    SIZE s{};
    if (GetTextExtentPoint32W(dc, w.c_str(), static_cast<int>(w.size()), &s) == 0) { return -1; }
    return static_cast<int>(s.cx);
}

long long heapBelegt() {
    HEAP_SUMMARY s{};
    s.cb = sizeof(s);
    if (HeapSummary(GetProcessHeap(), 0, &s) == 0) { return -1; }
    return static_cast<long long>(s.cbAllocated);
}

bool bildschirmFarbe(int x, int y, unsigned char rgb[3]) {
    HDC dc = GetDC(nullptr);
    if (dc == nullptr) { return false; }
    const COLORREF c = GetPixel(dc, x, y);
    ReleaseDC(nullptr, dc);
    if (c == CLR_INVALID) { return false; }
    rgb[0] = GetRValue(c);
    rgb[1] = GetGValue(c);
    rgb[2] = GetBValue(c);
    return true;
}

namespace {

HWND hauptfenster() {
    HWND fenster = nullptr;
    EnumThreadWindows(
        GetCurrentThreadId(),
        [](HWND h, LPARAM l) -> BOOL {
            if (IsWindowVisible(h) != 0 && GetWindow(h, GW_OWNER) == nullptr) {
                *reinterpret_cast<HWND*>(l) = h;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&fenster));
    return fenster;
}

bool g_vollbild = false;
LONG g_stilVorher = 0;
WINDOWPLACEMENT g_lageVorher{};

}  // namespace

bool vollbildUmschalten() {
    HWND h = hauptfenster();
    if (h == nullptr) { return g_vollbild; }
    if (!g_vollbild) {
        g_stilVorher = GetWindowLongW(h, GWL_STYLE);
        g_lageVorher.length = sizeof(g_lageVorher);
        if (GetWindowPlacement(h, &g_lageVorher) == 0) { return false; }
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi) == 0) { return false; }
        // QWindowsWindow::setWindowState( Qt::WindowFullScreen ): WS_POPUP, Bildschirmgroesse;
        // setHasBorderInFullScreen: WS_BORDER
        SetWindowLongW(h, GWL_STYLE, (g_stilVorher & ~static_cast<LONG>(WS_OVERLAPPEDWINDOW)) | static_cast<LONG>(WS_POPUP | WS_BORDER));
        SetWindowPos(h, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
        g_vollbild = true;
    } else {
        SetWindowLongW(h, GWL_STYLE, g_stilVorher);
        WINDOWPLACEMENT wp = g_lageVorher;
        if (wp.showCmd == SW_SHOWNORMAL) { wp.showCmd = SW_SHOWNOACTIVATE; }
        SetWindowPlacement(h, &wp);
        SetWindowPos(h, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
        g_vollbild = false;
    }
    return g_vollbild;
}

bool istVollbild() { return g_vollbild; }

bool hauptfensterRechteck(int& x, int& y, int& b, int& h) {
    HWND f = hauptfenster();
    RECT r{};
    if (f == nullptr || GetWindowRect(f, &r) == 0) { return false; }
    x = r.left;
    y = r.top;
    b = r.right - r.left;
    h = r.bottom - r.top;
    return true;
}

bool hauptfensterVorne() {
    HWND f = hauptfenster();
    return f != nullptr && GetForegroundWindow() == f;
}

void taskleisteBlinken() {
    HWND f = hauptfenster();
    if (f == nullptr) { return; }
    FLASHWINFO fi{};
    fi.cbSize = sizeof(fi);
    fi.hwnd = f;
    fi.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    fi.uCount = 0;
    fi.dwTimeout = 0;
    FlashWindowEx(&fi);
}

bool bildschirmRechteck(int& x, int& y, int& b, int& h) {
    HWND f = hauptfenster();
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (f == nullptr || GetMonitorInfoW(MonitorFromWindow(f, MONITOR_DEFAULTTONEAREST), &mi) == 0) { return false; }
    x = mi.rcMonitor.left;
    y = mi.rcMonitor.top;
    b = mi.rcMonitor.right - mi.rcMonitor.left;
    h = mi.rcMonitor.bottom - mi.rcMonitor.top;
    return true;
}

std::vector<std::string> schriftFamilien() {
    std::vector<std::string> namen;
    HDC dc = GetDC(nullptr);
    if (dc == nullptr) { return namen; }
    LOGFONTW lf{};
    lf.lfCharSet = DEFAULT_CHARSET;
    EnumFontFamiliesExW(
        dc, &lf,
        [](const LOGFONTW* l, const TEXTMETRICW*, DWORD, LPARAM p) -> int {
            if (l->lfFaceName[0] != L'@') { reinterpret_cast<std::vector<std::string>*>(p)->push_back(toUtf8(l->lfFaceName)); }
            return 1;
        },
        reinterpret_cast<LPARAM>(&namen), 0);
    ReleaseDC(nullptr, dc);
    std::sort(namen.begin(), namen.end(), [](const std::string& a, const std::string& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) {
            return std::tolower(static_cast<unsigned char>(x)) < std::tolower(static_cast<unsigned char>(y));
        });
    });
    namen.erase(std::unique(namen.begin(), namen.end()), namen.end());
    return namen;
}

std::string schriftDatei(const std::string& familie, bool fett, bool kursiv) {
    std::wstring name = toWide(familie);
    if (fett) { name += L" Bold"; }
    if (kursiv) { name += L" Italic"; }
    const std::wstring schluessel[2] = {name + L" (TrueType)", name + L" (OpenType)"};
    for (HKEY wurzel : {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER}) {
        HKEY k = nullptr;
        if (RegOpenKeyExW(wurzel, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &k) != ERROR_SUCCESS) { continue; }
        for (const std::wstring& s : schluessel) {
            wchar_t wert[MAX_PATH] = {};
            DWORD groesse = sizeof(wert);
            DWORD typ = 0;
            if (RegQueryValueExW(k, s.c_str(), nullptr, &typ, reinterpret_cast<BYTE*>(wert), &groesse) == ERROR_SUCCESS && typ == REG_SZ) {
                RegCloseKey(k);
                std::wstring pfad = wert;
                if (pfad.find(L'\\') == std::wstring::npos) {   // nur der Dateiname: im Fonts-Ordner
                    wchar_t ordner[MAX_PATH] = {};
                    GetWindowsDirectoryW(ordner, MAX_PATH);
                    pfad = std::wstring(ordner) + L"\\Fonts\\" + pfad;
                }
                return toUtf8(pfad);
            }
        }
        RegCloseKey(k);
    }
    return {};
}

bool befehlStarten(const std::string& befehlszeile) {
    std::wstring zeile = toWide(befehlszeile);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    // Q_Exec( 0, command, 0, bCreateConsole = true, waitfor = false )
    if (CreateProcessW(nullptr, zeile.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi) == 0) { return false; }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

}  // namespace bhed::platform
