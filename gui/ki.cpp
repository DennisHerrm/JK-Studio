// ki.cpp - die freiwillige KI-Anbindung im Programm (Stufe H1, siehe ki.h)
#include "ki.h"
#include "app_internal.h"

namespace bhed::gui {

void kiAbschnitt(const char* frage, float breite) {
    (void)frage;
    (void)breite;
}

void kiBild() {}

void kiStart(bool mcpStdio) { (void)mcpStdio; }

void kiEnde() {}

}  // namespace bhed::gui
