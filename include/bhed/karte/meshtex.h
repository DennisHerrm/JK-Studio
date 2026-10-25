// meshtex.h - Karten-Editor, Stufe 4 Punkt 3: das Plugin MeshTex (Texturen
// auf Patch-Netzen ausrichten und skalieren) ohne Oberflaeche.
//
// Portiert aus NetRadiant-Custom 20260114, contrib/meshtex/MeshEntity.h/.cpp
// (MeshEntity: GetInfo, MinAlign, MaxAlign, MinMaxAlignAutoScale /
// Stretch / Shrink, SetScale, GeneralFunction samt der Laengenmessung auf
// der Flaeche: Gauss-Legendre mit 10 Punkten, rekursiv verfeinert),
// MeshEntityMessages.h (die Texte) und PluginProperties.h (About).
//
//   Copyright (C) 2012 Joel Baxter
//
//   This file is part of MeshTex. MeshTex is free software: you can
//   redistribute it and/or modify it under the terms of the GNU General Public
//   License as published by the Free Software Foundation, either version 2 of
//   the License, or (at your option) any later version. MeshTex is distributed
//   in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
//   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//   PURPOSE. See the GNU General Public License for more details (LICENSE).
#ifndef BHED_KARTE_MESHTEX_H
#define BHED_KARTE_MESHTEX_H

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {
class Patch;
struct PatchPunkt;
}

namespace bhed::karte::meshtex {

enum TextureAxisSelection { S_TEX_AXIS_ONLY, T_TEX_AXIS_ONLY, ALL_TEX_AXES };
enum SliceType { ROW_SLICE_TYPE = 0, COL_SLICE_TYPE = 1 };

struct GeneralFunctionFactors {
    float oldValue = 0, rowDistance = 0, colDistance = 0, rowNumber = 0, colNumber = 0, constant = 0;
};
struct SliceDesignation {
    bool maxSlice = false;
    int index = 0;
};
struct RefSliceDescriptor {
    SliceDesignation designation;
    bool totalLengthOnly = false;
};

// Die Kaesten von MeshVisitor (GenericPluginUI::InfoReportDialog "Mesh Info",
// WarningReportDialog "Warning", ErrorReportDialog "Error")
struct Meldung {
    int art = 0;   // 0 Info, 1 Warnung, 2 Fehler
    std::string text;
};
using Meldungen = std::vector<Meldung>;
using TexInfoCallback = std::function<void(float scale, float tiles)>;

// Ein MeshEntity auf einem Patch. texturBreite/-hoehe: qtexture_t des Shaders
// (0 = keine Textur: ungueltig). Aenderungen schreiben in den Patch (mit Undo).
class MeshEntity {
public:
    MeshEntity(Patch& patch, int texturBreite, int texturHoehe, Meldungen& meldungen);
    [[nodiscard]] bool IsValid() const { return m_valid; }
    void GetInfo(const int* refRow, const int* refCol, const TexInfoCallback* rowTexInfoCallback, const TexInfoCallback* colTexInfoCallback);
    void MinAlign(TextureAxisSelection axes);
    void MaxAlign(TextureAxisSelection axes);
    void MinMaxAlignAutoScale(TextureAxisSelection axes);
    void MinMaxAlignStretch(TextureAxisSelection axes);
    void MinMaxAlignShrink(TextureAxisSelection axes);
    void SetScale(SliceType sliceType, const SliceDesignation* alignSlice, const RefSliceDescriptor* refSlice, bool naturalScale, float naturalScaleOrTiles);
    void GeneralFunction(const GeneralFunctionFactors* sFactors, const GeneralFunctionFactors* tFactors, const SliceDesignation* alignRow,
                         const SliceDesignation* alignCol, const RefSliceDescriptor* refRow, const RefSliceDescriptor* refCol, bool surfaceValues);

private:
    enum TextureAxis { S_TEX_AXIS = 0, T_TEX_AXIS = 1 };
    enum PositionAxis { X_POS_AXIS = 0, Y_POS_AXIS = 1, Z_POS_AXIS = 2 };
    enum ScaleOperation { STRETCH_SCALE_OP, SHRINK_SCALE_OP };
    struct SlicePatchContext {
        SliceType sliceType = ROW_SLICE_TYPE;
        float position = 0;
        int edgeSlice[2] = {0, 0};
    };
    struct SliceTexInfo {
        float scale = 0, tiles = 0, min = 0, max = 0;
    };
    struct RefSliceDescriptorInt {
        unsigned index = 0;
        bool totalLengthOnly = false;
    };
    class Matrix {
    public:
        Matrix(unsigned x, unsigned y) : m_y(y), m_d(static_cast<std::size_t>(x) * y, 0.0F) {}
        float& operator()(unsigned x, unsigned y) { return m_d[(static_cast<std::size_t>(x) * m_y) + y]; }
        float operator()(unsigned x, unsigned y) const { return m_d[(static_cast<std::size_t>(x) * m_y) + y]; }

    private:
        unsigned m_y;
        std::vector<float> m_d;
    };
    using InternalImpl = bool (MeshEntity::*)(TextureAxis axis);

    PatchPunkt& punkt(unsigned row, unsigned col);
    [[nodiscard]] float texcoord(unsigned row, unsigned col, int axis) const;
    float& texcoordRef(unsigned row, unsigned col, int axis);
    [[nodiscard]] float vertex(unsigned row, unsigned col, int axis) const;
    static float& element(Matrix& m, SliceType t, int slice, int index);
    [[nodiscard]] unsigned sliceSize(SliceType t) const { return m_numSlices[1 - t]; }
    void UpdatePosMinMax(PositionAxis axis);
    void UpdateTexMinMax(TextureAxis axis);
    void CommitChanges();
    int InternalSliceDesignation(const SliceDesignation* sliceDesignation, SliceType sliceType);
    RefSliceDescriptorInt* InternalRefSliceDescriptor(const RefSliceDescriptor* refSlice, SliceType sliceType, RefSliceDescriptorInt& refSliceInt);
    float GetSliceTexScale(SliceType sliceType, int slice, TextureAxis axis, float tiles);
    bool GetSliceTexInfo(SliceType sliceType, int slice, TextureAxis axis, SliceTexInfo& info);
    void ReportSliceTexInfo(SliceType sliceType, int slice, TextureAxis axis, std::string* message, const TexInfoCallback* texInfoCallback);
    void ProcessForAxes(InternalImpl internalImpl, TextureAxisSelection axes);
    void Shift(TextureAxis axis, float shift);
    void Scale(TextureAxis axis, float scale);
    bool MinAlignInt(TextureAxis axis);
    bool MaxAlignInt(TextureAxis axis);
    bool MinMaxAlignAutoScaleInt(TextureAxis axis);
    bool MinMaxAlignScale(TextureAxis axis, ScaleOperation op);
    bool MinMaxAlignStretchInt(TextureAxis axis);
    bool MinMaxAlignShrinkInt(TextureAxis axis);
    float SliceParametricSpeedComponent(PositionAxis axis, float t, const SlicePatchContext& context) const;
    float SliceParametricSpeed(float t, const SlicePatchContext& context) const;
    float EstimateSegmentLength(float startPosition, float endPosition, const SlicePatchContext& context) const;
    float RefineSegmentLength(float startPosition, float endPosition, const SlicePatchContext& context, float segmentLengthEstimate, float maxError) const;
    void GenControlTexFromSurface(TextureAxis axis, const Matrix& surfaceValues);
    void CopyControlTexFromValues(TextureAxis axis, const Matrix& values);
    void GenSurfaceFromControlTex(TextureAxis axis, Matrix& surfaceValues) const;
    void CopyValuesFromControlTex(TextureAxis axis, Matrix& values) const;
    void GenScaledDistanceValues(SliceType sliceType, int alignSlice, const RefSliceDescriptorInt* refSlice, bool rawScale, float rawScaleOrTiles,
                                 Matrix& values);
    void GeneralFunctionInt(const GeneralFunctionFactors& factors, TextureAxis axis, int alignRow, int alignCol, bool surfaceValues,
                            const Matrix& rowDistances, const Matrix& colDistances);

    Patch& m_patch;
    Meldungen& m_meldungen;
    std::vector<PatchPunkt> m_meshData;   // PatchControlMatrix( getHeight(), getWidth() ): (Zeile, Spalte)
    bool m_valid = false;
    float m_naturalTexUnits[2] = {0, 0};
    unsigned m_numSlices[2] = {0, 0};     // [ROW] Zeilen, [COL] Spalten
    bool m_texMinMaxDirty[2] = {true, true};
    float m_texMin[2] = {0, 0};
    float m_texMax[2] = {0, 0};
    bool m_texMinAligned[2] = {false, false};
    bool m_texMaxAligned[2] = {false, false};
    float m_posMin[3] = {0, 0, 0};
    float m_posMax[3] = {0, 0, 0};
};

// MainMenu: die Befehle der Voreinstellungen ("S&T Align Auto" ... "T Max Align")
struct Voreinstellung {
    const char* befehl;   // "STMinMaxAlignAuto" (auch der Name des Undo-Schritts)
    void (MeshEntity::*funktion)(TextureAxisSelection);
    TextureAxisSelection achsen;
};
[[nodiscard]] const std::vector<Voreinstellung>& voreinstellungen();

extern const char* const kAbout;   // DIALOG_ABOUT_MSG ohne HTML
extern const char* const kHilfe;   // DIALOG_HELP_MSG ohne HTML
extern const char* const kKeineNetze;   // DIALOG_NOMESHES_MSG
extern const char* const kNurEinNetz;   // DIALOG_MULTIMESHES_ERROR

}  // namespace bhed::karte::meshtex

#endif
