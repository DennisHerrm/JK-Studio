// patch.h - Karten-Editor: Bezier-Patches (patchDef2 / patchDef3).
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/patch.h,
// radiant/patch.cpp (Kontrollpunkte, setDims, Tesselierung: Kurvenbaeume,
// BuildVertexArray, TesselateSubMatrix, TesselateSubMatrixFixed), libs/render.h
// (ArbitraryMeshVertex, ArbitraryMeshTriangle_calcTangents), include/ipatch.h
// (PatchControl).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
//
// Die Anzeige-Tesselierung ist die von NRC: patchDef2 unterteilt adaptiv
// (eine Kurve wird geteilt, solange Bogenwinkel * Laenge den Schwellwert
// "Patch Subdivide Threshold", Vorgabe 4, uebersteigt; hoechstens 16 Stufen),
// patchDef3 fest mit subdivisions_x / _y Schritten je Teilfeld. q3map2 und die
// Engine tesselieren beim Kompilieren bzw. Zeichnen selbst - das hier ist nur
// das, was der Editor zeigt.
#ifndef BHED_KARTE_PATCH_H
#define BHED_KARTE_PATCH_H

#include "bhed/karte/mathe.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/rueckgaengig.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace bhed::karte {

constexpr std::size_t kMinPatchBreite = 3;   // MIN_PATCH_WIDTH
constexpr std::size_t kMinPatchHoehe = 3;    // MIN_PATCH_HEIGHT
// MAX_PATCH_WIDTH / _HEIGHT: Quake3-Patches 31 "matching q3map2"
// (patchmodule.cpp:58), Doom3 99.
constexpr std::size_t kMaxPatchBreite = 31;
constexpr std::size_t kMaxPatchHoehe = 31;
constexpr int kPatchUnterteilungVorgabe = 4;   // g_PatchSubdivideThreshold

// PatchControl (include/ipatch.h:211-216)
struct PatchPunkt {
    Vektor3f vertex;
    Vektor2f texcoord;
};

// ArbitraryMeshVertex (libs/render.h:791-804)
struct NetzPunkt {
    Vektor2f texcoord;
    Vektor3f normal;
    Vektor3f vertex;
    Vektor3f tangent;
    Vektor3f bitangent;
};

// PatchTesselation (patch.h:185-203): Dreiecksstreifen.
struct PatchTesselierung {
    std::vector<NetzPunkt> vertices;
    std::vector<unsigned int> indices;
    std::size_t numStrips = 0;
    std::size_t lenStrips = 0;
    std::vector<std::size_t> arrayWidth;
    std::size_t nArrayWidth = 0;
    std::vector<std::size_t> arrayHeight;
    std::size_t nArrayHeight = 0;
};

class Patch final : public Primitiv {
public:
    Patch();
    ~Patch() override;
    [[nodiscard]] Art art() const override { return Art::Patch; }
    [[nodiscard]] std::shared_ptr<Patch> kopie() const;

    // setDims (patch.cpp:156-186) - mit NRCs Fehler: bei gerader Hoehe wird
    // m_height statt h um eins verringert, h bleibt also gerade.
    void setzeGroesse(std::size_t w, std::size_t h);
    [[nodiscard]] std::size_t breite() const { return m_width; }
    [[nodiscard]] std::size_t hoehe() const { return m_height; }
    // ctrlAt( row, col ): Zeile r, Spalte c
    PatchPunkt& punkt(std::size_t r, std::size_t c) { return m_ctrl[(r * m_width) + c]; }
    [[nodiscard]] const PatchPunkt& punkt(std::size_t r, std::size_t c) const { return m_ctrl[(r * m_width) + c]; }
    [[nodiscard]] const std::vector<PatchPunkt>& punkte() const { return m_ctrl; }
    std::vector<PatchPunkt>& punkteRoh() { return m_ctrl; }

    [[nodiscard]] const std::string& shader() const { return m_shader; }
    void setzeShader(const std::string& name);   // SetShader (gleicher Name ohne Gross/klein: nichts)
    void ladeShader(const std::string& name) { m_shader = name; }

    bool patchDef3 = false;            // m_patchDef3
    std::size_t unterteilungX = 0;     // m_subdivisions_x
    std::size_t unterteilungY = 0;     // m_subdivisions_y

    [[nodiscard]] bool gueltig() const;   // isValid
    // controlPointsChanged: neu tesselieren und Grenzen bestimmen.
    void punkteGeaendert(int schwellwert = kPatchUnterteilungVorgabe);
    [[nodiscard]] const PatchTesselierung& tesselierung() const { return m_tess; }
    // Das Drahtgitter als Linienpaare (RenderablePatchWireframe::render,
    // patch.h:205-256; patchDef3: RenderablePatchFixedWireframe, Quad-Streifen
    // als Umrisse).
    void drahtgitter(std::vector<Vektor3f>& paare) const;
    [[nodiscard]] const Kasten& grenzen() const { return m_grenzen; }   // localAABB

    void transformieren(const Matrix4& m);   // transform + freezeTransform
    // undoSave, die Kontrollpunkte setzen, controlPointsChanged
    void punkteSetzen(const std::vector<PatchPunkt>& p);
    void einrasten(float raster);            // snapto
    void matrixUmkehren();                   // InvertMatrix
    void matrixTransponieren();              // TransposeMatrix
    // ProjectTexture( TextureProjection, normal ) (patch.cpp:1212-1228): st
    // aus der Projektion einer Flaeche (Texturgroesse aus dem Kontext).
    void texturProjizieren(TexturProjektion projektion, const Vektor3f& normal, const Kontext& k);
    // Patch_setTexture (seamless): jedes st durch die Matrix (undoSave,
    // controlPointsChanged).
    void texturkoordinatenTransformieren(const Matrix4& m);

    [[nodiscard]] std::unique_ptr<Memento> zustandSichern() const override;
    // importState (patch.h:937-961) mit onAllocate: die Punktwahl geht verloren.
    void zustandLaden(const Memento& m) override;
    void zustandZuruecksetzen(const Memento& m) override;   // revertTransform

    // PatchControlInstance::m_selectable (patch.h:1263-1283): Wahl der
    // Kontrollpunkte (Instanzzustand, nicht im Undo); allocate (patch.h:1411-1418)
    // legt sie bei jeder neuen Groesse leer an.
    std::vector<char> punktWahl;
    [[nodiscard]] bool punktGewaehlt(std::size_t i) const { return i < punktWahl.size() && punktWahl[i] != 0; }
    void punktWaehlen(std::size_t i, bool an) {
        komponentenWahlGeaendert();
        if (punktWahl.size() != m_ctrl.size()) { punktWahl.assign(m_ctrl.size(), 0); }
        if (i < punktWahl.size()) { punktWahl[i] = an ? 1 : 0; }
    }
    [[nodiscard]] bool punkteGewaehlt() const {   // selectedVertices
        for (const char c : punktWahl) {
            if (c != 0) { return true; }
        }
        return false;
    }

private:
    struct Kurvenbaum;   // BezierCurveTree
    void kurvenBauen(bool zeilen, int schwellwert);   // BuildTesselationCurves
    void netzBauen();                                 // BuildVertexArray
    void teilfeldFest(NetzPunkt* vertices, std::size_t strideX, std::size_t strideY, unsigned int flagsX,
                      unsigned int flagsY, PatchPunkt* teil[3][3]);   // TesselateSubMatrixFixed
    void teilfeld(const Kurvenbaum* bx, const Kurvenbaum* by, std::size_t offStartX, std::size_t offStartY,
                  std::size_t offEndX, std::size_t offEndY, std::size_t flagsX, std::size_t flagsY, const Vektor3f& left,
                  const Vektor3f& mid, const Vektor3f& right, const Vektor2f& texLeft, const Vektor2f& texMid,
                  const Vektor2f& texRight, bool transponiert);   // TesselateSubMatrix
    void tangentenSammeln(std::size_t index, Vektor3f tangentX[6], Vektor3f tangentY[6], Vektor2f tangentS[6],
                          Vektor2f tangentT[6], std::size_t index0, std::size_t index1);   // accumulateVertexTangentSpace

    std::string m_shader = kTexturPraefix;
    std::size_t m_width = 0;
    std::size_t m_height = 0;
    std::vector<PatchPunkt> m_ctrl;
    PatchTesselierung m_tess;
    std::vector<std::unique_ptr<Kurvenbaum>> m_kurvenU;   // m_curveTreeU
    std::vector<std::unique_ptr<Kurvenbaum>> m_kurvenV;   // m_curveTreeV
    Kasten m_grenzen;
};

}  // namespace bhed::karte
#endif
