// modell/animnamen.cpp - die Namen der animTable je Zielspiel (CW-MODELL) - siehe bhed/modell/animnamen.h
#include "bhed/modell/animnamen.h"

#include <cstddef>

#include "bhed/modell/g2.h"

namespace bhed::modell {
namespace {
#include "animnamen.inc"

template <std::size_t N>
std::vector<std::string> liste(const char* const (&a)[N]) {
    return std::vector<std::string>(a, a + N);
}
}  // namespace

const std::vector<std::string>& animNamenSpiel(Zielspiel s) {
    static const std::vector<std::string> jkaSp = liste(kAnimJkaSp);
    static const std::vector<std::string> jkaMp = liste(kAnimJkaMp);
    static const std::vector<std::string> jk2Sp = liste(kAnimJk2Sp);
    static const std::vector<std::string> md = liste(kAnimMdSp);
    switch (s) {
        case Zielspiel::JkaSp: return jkaSp;
        case Zielspiel::JkaMp: return jkaMp;
        case Zielspiel::Jk2Sp: return jk2Sp;
        case Zielspiel::MdSp:
        case Zielspiel::MdMp: return md;
    }
    return jkaSp;
}

}  // namespace bhed::modell
