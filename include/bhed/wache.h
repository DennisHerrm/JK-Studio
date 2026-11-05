// wache.h - eine Wache meldet, wann ihr Traeger verschwindet oder neuen Inhalt bekommt
//
// Wer sich etwas nach seiner ADRESSE merkt (die Grafikkarte ihre Bilder und Puffer, gui/gpumap_win32.cpp), muss
// erfahren, wann diese Adresse nicht mehr dasselbe meint. Die Wache steckt im Traeger und meldet ihre eigene Adresse
// - beim Ende des Traegers und bei jeder Zuweisung (neuer Inhalt). Eine Kopie bekommt eine eigene Wache, die Kopie
// ist ein anderer Eintrag. `Melden` zeigt auf den Zuhoerer (nullptr: niemand hoert zu).
// reste2 Punkt 2 (TextureSet::Tex), rc610 R1 (GlmModel).
#ifndef BHED_WACHE_H
#define BHED_WACHE_H

namespace bhed {

template <void (**Melden)(const void* wache)>
struct AdressWache {
    AdressWache() = default;
    AdressWache(const AdressWache& /*andere*/) noexcept {}
    AdressWache(AdressWache&& /*andere*/) noexcept {}
    AdressWache& operator=(const AdressWache& /*andere*/) noexcept {
        melden();
        return *this;
    }
    AdressWache& operator=(AdressWache&& /*andere*/) noexcept {
        melden();
        return *this;
    }
    ~AdressWache() { melden(); }

private:
    void melden() const noexcept {
        if (*Melden != nullptr) { (*Melden)(this); }
    }
};

}  // namespace bhed
#endif
