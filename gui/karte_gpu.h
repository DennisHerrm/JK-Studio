// karte_gpu.h - Karten-Editor: Zeichnen ueber Direct3D 11.
//
// NRC zeichnet jede Ansicht mit OpenGL in ein eigenes FBO mit MSAA
// (xywindow.cpp:477-496, camwindow.cpp:1606-1623) und blendet es ein. Hier
// dasselbe mit Direct3D 11: je Ansicht ein Renderziel (mit Mehrfachabtastung,
// aufgeloest in eine Textur, die ImGui zeigt). Linien in Weltkoordinaten mit
// einer Matrix (Projektion x Modelview), Breite 1 oder breiter (glLineWidth:
// ueber einen Geometrie-Shader zu Rechtecken in Bildpunkten).
//
// Ohne Direct3D (OpenGL-Rueckfall der Oberflaeche) zeichnet die Oberflaeche
// dieselben Linien mit ImGui (verfuegbar() == false).
#ifndef BHED_GUI_KARTE_GPU_H
#define BHED_GUI_KARTE_GPU_H

#include "bhed/karte/xyansicht.h"

#include <cstddef>
#include <vector>

namespace bhed::gui::kartegpu {

[[nodiscard]] bool verfuegbar();

// Ein Linienpuffer auf der Grafikkarte (z. B. alle Pinselkanten einer
// Ansicht) - einmal fuellen, oft zeichnen.
class Puffer {
public:
    Puffer() = default;
    ~Puffer();
    Puffer(const Puffer&) = delete;
    Puffer& operator=(const Puffer&) = delete;
    void setzen(const std::vector<karte::Linie>& linien);
    [[nodiscard]] std::size_t anzahl() const { return m_anzahl; }

private:
    friend void zeichnen(const Puffer& p, const float* matrix, float breite);
    void* m_vb = nullptr;      // ID3D11Buffer*
    std::size_t m_platz = 0;   // Linien, fuer die der Puffer Platz hat
    std::size_t m_anzahl = 0;
    unsigned m_stand = ~0U;    // render::texturStand() beim Anlegen
};

// In das Renderziel "kennung" zeichnen (anlegen bzw. neu anlegen bei anderer
// Groesse/MSAA). hinten: Loeschfarbe RGBA 0..1.
bool beginnen(int kennung, int breite, int hoehe, int msaa, const float* hinten);
// matrix: 4x4 spaltenweise (wie OpenGL: Clip = M * Welt).
void zeichnen(const Puffer& p, const float* matrix, float breite);
// Einmal-Linien (Raster, Overlay) ueber einen gemeinsamen Puffer.
void zeichnen(const std::vector<karte::Linie>& linien, const float* matrix, float breite);
// Aufloesen und die vorigen Renderziele wiederherstellen.
void beenden();
// Die Textur eines Renderziels fuer ImGui::Image (nullptr, wenn keine).
[[nodiscard]] void* textur(int kennung);
void freigeben();

}  // namespace bhed::gui::kartegpu

#endif
