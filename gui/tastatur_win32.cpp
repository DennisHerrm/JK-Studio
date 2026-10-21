// tastatur_win32.cpp - Karten-Editor: Windows-Tastaturbelegung wie Qt.
//
// Nach Qt 5.15 (LGPL), qwindowskeymapper.cpp: setKbdState, toKeyOrUnicode,
// winceKeyBend (KeyTbl) und QWindowsKeyMapper::updatePossibleKeyCodes - so
// liest NetRadiant-Custom unter Windows seine Tastenkuerzel.
#include "tastatur.h"

#include "imgui.h"

#include <windows.h>

#include <cwctype>

namespace bhed::gui::tastatur {

namespace {

namespace kb = karte::befehle;

// ImGuiKey -> virtuelle Taste. Die Satzzeichen ordnet imgui_impl_win32 nach
// dem Scancode zu (ImGui_ImplWin32_KeyEventToImGuiKey, "Fallback to
// scancode"); fuer sie liefert MapVirtualKeyEx die Taste der Belegung.
UINT virtuelleTaste(int k, HKL hkl) {
    if (k >= ImGuiKey_A && k <= ImGuiKey_Z) { return static_cast<UINT>('A' + (k - ImGuiKey_A)); }
    if (k >= ImGuiKey_0 && k <= ImGuiKey_9) { return static_cast<UINT>('0' + (k - ImGuiKey_0)); }
    if (k >= ImGuiKey_F1 && k <= ImGuiKey_F24) { return static_cast<UINT>(VK_F1 + (k - ImGuiKey_F1)); }
    if (k >= ImGuiKey_Keypad0 && k <= ImGuiKey_Keypad9) { return static_cast<UINT>(VK_NUMPAD0 + (k - ImGuiKey_Keypad0)); }
    UINT scancode = 0;
    switch (k) {
    case ImGuiKey_Tab: return VK_TAB;
    case ImGuiKey_LeftArrow: return VK_LEFT;
    case ImGuiKey_RightArrow: return VK_RIGHT;
    case ImGuiKey_UpArrow: return VK_UP;
    case ImGuiKey_DownArrow: return VK_DOWN;
    case ImGuiKey_PageUp: return VK_PRIOR;
    case ImGuiKey_PageDown: return VK_NEXT;
    case ImGuiKey_Home: return VK_HOME;
    case ImGuiKey_End: return VK_END;
    case ImGuiKey_Insert: return VK_INSERT;
    case ImGuiKey_Delete: return VK_DELETE;
    case ImGuiKey_Backspace: return VK_BACK;
    case ImGuiKey_Space: return VK_SPACE;
    case ImGuiKey_Enter: return VK_RETURN;
    case ImGuiKey_KeypadEnter: return VK_RETURN;
    case ImGuiKey_Escape: return VK_ESCAPE;
    case ImGuiKey_Pause: return VK_PAUSE;
    case ImGuiKey_PrintScreen: return VK_SNAPSHOT;
    case ImGuiKey_KeypadDecimal: return VK_DECIMAL;
    case ImGuiKey_KeypadDivide: return VK_DIVIDE;
    case ImGuiKey_KeypadMultiply: return VK_MULTIPLY;
    case ImGuiKey_KeypadSubtract: return VK_SUBTRACT;
    case ImGuiKey_KeypadAdd: return VK_ADD;
    case ImGuiKey_Comma: return VK_OEM_COMMA;
    case ImGuiKey_Period: return VK_OEM_PERIOD;
    case ImGuiKey_GraveAccent: scancode = 41; break;
    case ImGuiKey_Minus: scancode = 12; break;
    case ImGuiKey_Equal: scancode = 13; break;
    case ImGuiKey_LeftBracket: scancode = 26; break;
    case ImGuiKey_RightBracket: scancode = 27; break;
    case ImGuiKey_Oem102: scancode = 86; break;
    case ImGuiKey_Backslash: scancode = 43; break;
    case ImGuiKey_Semicolon: scancode = 39; break;
    case ImGuiKey_Apostrophe: scancode = 40; break;
    case ImGuiKey_Slash: scancode = 53; break;
    default: return 0;
    }
    return MapVirtualKeyExW(scancode, MAPVK_VSC_TO_VK_EX, hkl);
}

// winceKeyBend: KeyTbl[vk] (qwindowskeymapper.cpp), soweit Tasten Zeichen
// haben koennen; Satzzeichen-Tasten (VK_OEM_*) sind dort Key_unknown.
std::uint32_t qtTasteFuer(UINT vk) {
    if (vk >= '0' && vk <= '9') { return vk; }
    if (vk >= 'A' && vk <= 'Z') { return vk; }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { return '0' + (vk - VK_NUMPAD0); }
    if (vk >= VK_F1 && vk <= VK_F24) { return kb::kKeyF1 + (vk - VK_F1); }
    switch (vk) {
    case VK_BACK: return kb::kKeyBackspace;
    case VK_TAB: return kb::kKeyTab;
    case VK_CLEAR: return 0x0100000b;
    case VK_RETURN: return kb::kKeyReturn;
    case VK_PAUSE: return 0x01000008;
    case VK_ESCAPE: return kb::kKeyEscape;
    case VK_SPACE: return 0x20;
    case VK_PRIOR: return kb::kKeyPageUp;
    case VK_NEXT: return kb::kKeyPageDown;
    case VK_END: return kb::kKeyEnd;
    case VK_HOME: return kb::kKeyHome;
    case VK_LEFT: return kb::kKeyLeft;
    case VK_UP: return kb::kKeyUp;
    case VK_RIGHT: return kb::kKeyRight;
    case VK_DOWN: return kb::kKeyDown;
    case VK_SNAPSHOT: return 0x01000009;
    case VK_INSERT: return kb::kKeyInsert;
    case VK_DELETE: return kb::kKeyDelete;
    case VK_MULTIPLY: return '*';
    case VK_ADD: return '+';
    case VK_SEPARATOR: return ',';
    case VK_SUBTRACT: return '-';
    case VK_DECIMAL: return '.';
    case VK_DIVIDE: return '/';
    default: return 0;   // Qt::Key_unknown
    }
}

// setKbdState (qwindowskeymapper.cpp): linke Umschalt- und Strg-Taste, rechtes
// Alt - "left Ctrl + right Alt is considered AltGraph".
void zustandSetzen(BYTE* kbd, bool shift, bool ctrl, bool alt) {
    kbd[VK_LSHIFT] = shift ? 0x80 : 0;
    kbd[VK_SHIFT] = shift ? 0x80 : 0;
    kbd[VK_LCONTROL] = ctrl ? 0x80 : 0;
    kbd[VK_CONTROL] = ctrl ? 0x80 : 0;
    kbd[VK_RMENU] = alt ? 0x80 : 0;
    kbd[VK_MENU] = alt ? 0x80 : 0;
}

// toKeyOrUnicode (qwindowskeymapper.cpp). Flag 4: ToUnicodeEx veraendert den
// Zustand der Tastatur nicht (tote Tasten bleiben unberuehrt).
std::uint32_t zeichen(UINT vk, UINT scancode, BYTE* kbd, HKL hkl) {
    wchar_t puffer[5] = {};
    int res = ToUnicodeEx(vk, scancode, kbd, puffer, 5, 4, hkl);
    // When Ctrl modifier is used ToUnicode does not return correct values.
    if (res == 0 && kbd[VK_CONTROL] != 0) {
        const BYTE strg = kbd[VK_CONTROL];
        kbd[VK_CONTROL] = 0;
        res = ToUnicodeEx(vk, scancode, kbd, puffer, 5, 4, hkl);
        kbd[VK_CONTROL] = strg;
    }
    std::uint32_t code = 0;
    if (res != 0) { code = static_cast<std::uint32_t>(std::towupper(puffer[0])); }
    // Qt::Key_*'s are not encoded below 0x20, so try again, and DEL keys
    // (0x7f) is encoded with a proper Qt::Key_ code
    if (code < 0x20 || code == 0x7f) { code = qtTasteFuer(vk); }
    return code;
}

}  // namespace

bool belegung(int imguiKey, kb::TastenBelegung& out, bool& ziffernblock, bool& erweitertesReturn) {
    const HKL hkl = GetKeyboardLayout(0);
    const UINT vk = virtuelleTaste(imguiKey, hkl);
    out = kb::TastenBelegung{};
    ziffernblock = false;
    erweitertesReturn = imguiKey == ImGuiKey_KeypadEnter;
    if (vk == 0) { return false; }
    const UINT scancode = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, hkl);
    BYTE kbd[256] = {};
    static const bool kZustand[8][3] = {{false, false, false}, {true, false, false}, {false, true, false},
                                        {true, true, false},   {false, false, true}, {true, false, true},
                                        {false, true, true},   {true, true, true}};
    for (int i = 0; i < 8; ++i) {
        zustandSetzen(kbd, kZustand[i][0], kZustand[i][1], kZustand[i][2]);
        out.qtKey[static_cast<std::size_t>(i)] = zeichen(vk, scancode, kbd, hkl);
    }
    // Add a fall back key for layouts which don't do composition and show
    // non-latin1 characters
    std::uint32_t rueckfall = qtTasteFuer(vk);
    if (rueckfall == 0) {
        if (vk != out.qtKey[0] && vk != out.qtKey[1] && vk < 0x5B && vk > 0x2F) { rueckfall = vk; }
    }
    out.qtKey[8] = rueckfall;
    out.vorhanden = true;
    ziffernblock = (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE) || erweitertesReturn;
    return true;
}

}  // namespace bhed::gui::tastatur
