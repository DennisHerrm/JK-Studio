// app_modelltab_ent3b.cpp - ENTSCHEID3B: Proben fuer den Selbsttest (siehe app_modelltab_ent3b.h)
#include "app_modelltab_ent3b.h"

namespace bhed::gui {

Ent3bProbe& ent3bProbe() {
    static Ent3bProbe p;
    return p;
}

}  // namespace bhed::gui
