// meshtex.cpp - Karten-Editor, Stufe 4 Punkt 3: das Plugin MeshTex ohne
// Oberflaeche (siehe meshtex.h).
//
// Portiert aus NetRadiant-Custom 20260114, contrib/meshtex/MeshEntity.cpp
// (Zeile fuer Zeile, auch die Eigenheiten: GeneralFunction nimmt alignRow fuer
// beide Ausrichtungen, MeshEntity.cpp:550-551), MainMenu.cpp (die
// Voreinstellungen), MeshEntityMessages.h, PluginUIMessages.h,
// PluginProperties.h.
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
#include "bhed/karte/meshtex.h"

#include "bhed/karte/patch.h"

#include <cfloat>
#include <cmath>
#include <cstdio>

namespace bhed::karte::meshtex {

const char* const kAbout = "MeshTex 3.0 beta\n\nAlign and scale textures on patch meshes\n\nJoel Baxter ( jl@neogeographica.com )";
const char* const kHilfe =
    "The Set S/T Scale, Get Info, and General Function dialogs will affect patch meshes that are selected when OK or Apply is clicked. For the other "
    "menu options, select the mesh(es) before selecting the option.\n\nhttp://neogeographica.com/site/pages/tools/meshtex.html";
const char* const kKeineNetze = "No valid patch meshes selected.";
const char* const kNurEinNetz = "Must select only one patch mesh for this function.";

namespace {

constexpr std::size_t INFO_BUFFER_SIZE = 1024;
constexpr float UNITS_ERROR_BOUND = 0.5F;

float SanitizeFloat(float f) { return f == -0.0F ? 0.0F : f; }

const int kNaturalAxis[2] = {0, 1};   // _naturalAxis: S_TEX_AXIS fuer Zeilen, T_TEX_AXIS fuer Spalten
const bool kRadiantScaleInverted[2] = {false, true};
const bool kRadiantTilesInverted[2] = {false, false};
const char* const kInfoSliceFormat[2] = {"On row %d:\n  S natural scale: %f\n  S tiles: %f\n  S min: %f\n  S max: %f\n\n",
                                         "On column %d:\n  T natural scale: %f\n  T tiles: %f\n  T min: %f\n  T max: %f\n\n"};
const char* const kInfoSliceInfscaleFormat[2] = {"On row %d:\n  S scale: (infinite)\n  S tiles: %f\n  S min: %f\n  S max: %f\n\n",
                                                 "On column %d:\n  T scale: (infinite)\n  T tiles: %f\n  T min: %f\n  T max: %f\n\n"};
const char* const kWarningSliceInfscale[2] = {
    "The selected reference row has an infinite scale (S does not vary along the row). The row information cannot be automatically transferred to the "
    "Set S/T Scale dialog.",
    "The selected reference column has an infinite scale (T does not vary along the column). The column information cannot be automatically "
    "transferred to the Set S/T Scale dialog."};
const char* const kErrorBadSlice[2] = {"Illegal row # specified.", "Illegal column # specified."};
const char* const kErrorSliceZeroscale[2] = {"A scale of zero cannot be applied; the S values of the row will not be changed.",
                                             "A scale of zero cannot be applied; the T values of the column will not be changed."};
const char* const kErrorSliceZerotiles[2] = {"A tile count of zero cannot be applied; the S values of the row will not be changed.",
                                             "A tile count of zero cannot be applied; the T values of the column will not be changed."};
const char* const kInfoMeshFormat =
    "Over entire mesh:\n  Rows: %d\n  S min: %f\n  S max: %f\n  Columns: %d\n  T min: %f\n  T max: %f\n\nWorldspace extents: (%.2f/%.2f,%.2f/%.2f,%.2f/%.2f)";

}  // namespace

const std::vector<Voreinstellung>& voreinstellungen() {   // MainMenu::MainMenu
    static const std::vector<Voreinstellung> v = {
        {"STMinMaxAlignAuto", &MeshEntity::MinMaxAlignAutoScale, ALL_TEX_AXES},
        {"SMinMaxAlignAuto", &MeshEntity::MinMaxAlignAutoScale, S_TEX_AXIS_ONLY},
        {"TMinMaxAlignAuto", &MeshEntity::MinMaxAlignAutoScale, T_TEX_AXIS_ONLY},
        {"SMinMaxAlignStretch", &MeshEntity::MinMaxAlignStretch, S_TEX_AXIS_ONLY},
        {"SMinMaxAlignShrink", &MeshEntity::MinMaxAlignShrink, S_TEX_AXIS_ONLY},
        {"TMinMaxAlignStretch", &MeshEntity::MinMaxAlignStretch, T_TEX_AXIS_ONLY},
        {"TMinMaxAlignShrink", &MeshEntity::MinMaxAlignShrink, T_TEX_AXIS_ONLY},
        {"SMinAlign", &MeshEntity::MinAlign, S_TEX_AXIS_ONLY},
        {"SMaxAlign", &MeshEntity::MaxAlign, S_TEX_AXIS_ONLY},
        {"TMinAlign", &MeshEntity::MinAlign, T_TEX_AXIS_ONLY},
        {"TMaxAlign", &MeshEntity::MaxAlign, T_TEX_AXIS_ONLY},
    };
    return v;
}

MeshEntity::MeshEntity(Patch& patch, int texturBreite, int texturHoehe, Meldungen& meldungen) : m_patch(patch), m_meldungen(meldungen) {
    m_meshData = patch.punkte();   // Patch_getControlPoints
    m_numSlices[ROW_SLICE_TYPE] = static_cast<unsigned>(patch.hoehe());
    m_numSlices[COL_SLICE_TYPE] = static_cast<unsigned>(patch.breite());
    const bool textur = texturBreite > 0 && texturHoehe > 0;
    if (textur) {
        m_naturalTexUnits[S_TEX_AXIS] = static_cast<float>(texturBreite) / 2.0F;
        m_naturalTexUnits[T_TEX_AXIS] = static_cast<float>(texturHoehe) / 2.0F;
    }
    if (m_numSlices[ROW_SLICE_TYPE] < 3 || m_numSlices[COL_SLICE_TYPE] < 3 || !textur) {
        m_valid = false;
        m_meldungen.push_back({2, "Unable to read patch mesh."});   // ERROR_BAD_MESH
        return;
    }
    m_valid = true;
    UpdatePosMinMax(X_POS_AXIS);
    UpdatePosMinMax(Y_POS_AXIS);
    UpdatePosMinMax(Z_POS_AXIS);
    m_texMinMaxDirty[S_TEX_AXIS] = true;
    m_texMinMaxDirty[T_TEX_AXIS] = true;
}

PatchPunkt& MeshEntity::punkt(unsigned row, unsigned col) { return m_meshData[(static_cast<std::size_t>(row) * m_numSlices[COL_SLICE_TYPE]) + col]; }
float MeshEntity::texcoord(unsigned row, unsigned col, int axis) const {
    return m_meshData[(static_cast<std::size_t>(row) * m_numSlices[COL_SLICE_TYPE]) + col].texcoord[static_cast<std::size_t>(axis)];
}
float& MeshEntity::texcoordRef(unsigned row, unsigned col, int axis) { return punkt(row, col).texcoord[static_cast<std::size_t>(axis)]; }
float MeshEntity::vertex(unsigned row, unsigned col, int axis) const {
    return m_meshData[(static_cast<std::size_t>(row) * m_numSlices[COL_SLICE_TYPE]) + col].vertex[static_cast<std::size_t>(axis)];
}
float& MeshEntity::element(Matrix& m, SliceType t, int slice, int index) {   // MatrixElement
    return t == ROW_SLICE_TYPE ? m(static_cast<unsigned>(slice), static_cast<unsigned>(index)) : m(static_cast<unsigned>(index), static_cast<unsigned>(slice));
}

void MeshEntity::GetInfo(const int* refRow, const int* refCol, const TexInfoCallback* rowTexInfoCallback, const TexInfoCallback* colTexInfoCallback) {
    std::string message;
    if (refRow != nullptr) { ReportSliceTexInfo(ROW_SLICE_TYPE, *refRow, static_cast<TextureAxis>(kNaturalAxis[ROW_SLICE_TYPE]), &message, rowTexInfoCallback); }
    if (refCol != nullptr) { ReportSliceTexInfo(COL_SLICE_TYPE, *refCol, static_cast<TextureAxis>(kNaturalAxis[COL_SLICE_TYPE]), &message, colTexInfoCallback); }
    UpdateTexMinMax(S_TEX_AXIS);
    UpdateTexMinMax(T_TEX_AXIS);
    char b[INFO_BUFFER_SIZE + 1];
    std::snprintf(b, sizeof(b), kInfoMeshFormat, m_numSlices[ROW_SLICE_TYPE], static_cast<double>(SanitizeFloat(m_texMin[S_TEX_AXIS])),
                  static_cast<double>(SanitizeFloat(m_texMax[S_TEX_AXIS])), m_numSlices[COL_SLICE_TYPE], static_cast<double>(SanitizeFloat(m_texMin[T_TEX_AXIS])),
                  static_cast<double>(SanitizeFloat(m_texMax[T_TEX_AXIS])), static_cast<double>(SanitizeFloat(m_posMin[X_POS_AXIS])),
                  static_cast<double>(SanitizeFloat(m_posMax[X_POS_AXIS])), static_cast<double>(SanitizeFloat(m_posMin[Y_POS_AXIS])),
                  static_cast<double>(SanitizeFloat(m_posMax[Y_POS_AXIS])), static_cast<double>(SanitizeFloat(m_posMin[Z_POS_AXIS])),
                  static_cast<double>(SanitizeFloat(m_posMax[Z_POS_AXIS])));
    message += b;
    if (message.size() > INFO_BUFFER_SIZE) { message.resize(INFO_BUFFER_SIZE); }
    m_meldungen.push_back({0, message});
}

void MeshEntity::MinAlign(TextureAxisSelection axes) { ProcessForAxes(&MeshEntity::MinAlignInt, axes); }
void MeshEntity::MaxAlign(TextureAxisSelection axes) { ProcessForAxes(&MeshEntity::MaxAlignInt, axes); }
void MeshEntity::MinMaxAlignAutoScale(TextureAxisSelection axes) { ProcessForAxes(&MeshEntity::MinMaxAlignAutoScaleInt, axes); }
void MeshEntity::MinMaxAlignStretch(TextureAxisSelection axes) { ProcessForAxes(&MeshEntity::MinMaxAlignStretchInt, axes); }
void MeshEntity::MinMaxAlignShrink(TextureAxisSelection axes) { ProcessForAxes(&MeshEntity::MinMaxAlignShrinkInt, axes); }

void MeshEntity::SetScale(SliceType sliceType, const SliceDesignation* alignSlice, const RefSliceDescriptor* refSlice, bool naturalScale,
                          float naturalScaleOrTiles) {
    const auto andere = static_cast<SliceType>(1 - sliceType);
    if (alignSlice != nullptr && !alignSlice->maxSlice) {
        if (alignSlice->index < 0 || alignSlice->index >= static_cast<int>(sliceSize(sliceType))) {
            m_meldungen.push_back({2, kErrorBadSlice[andere]});
            return;
        }
    }
    if (refSlice != nullptr && !refSlice->designation.maxSlice) {
        if (refSlice->designation.index < 0 || refSlice->designation.index >= static_cast<int>(m_numSlices[sliceType])) {
            m_meldungen.push_back({2, kErrorBadSlice[sliceType]});
            return;
        }
    }
    const auto axis = static_cast<TextureAxis>(kNaturalAxis[sliceType]);
    float rawScaleOrTiles = naturalScaleOrTiles;
    if (naturalScale) {
        if (rawScaleOrTiles == 0) {
            m_meldungen.push_back({2, kErrorSliceZeroscale[sliceType]});
            return;
        }
        if (kRadiantScaleInverted[sliceType]) { rawScaleOrTiles = -rawScaleOrTiles; }
        rawScaleOrTiles *= m_naturalTexUnits[axis];
    } else {
        if (rawScaleOrTiles == 0) {
            m_meldungen.push_back({2, kErrorSliceZerotiles[sliceType]});
            return;
        }
        if (kRadiantTilesInverted[sliceType]) { rawScaleOrTiles = -rawScaleOrTiles; }
    }
    const int alignSliceInt = InternalSliceDesignation(alignSlice, andere);
    RefSliceDescriptorInt descriptor;
    RefSliceDescriptorInt* refSliceInt = InternalRefSliceDescriptor(refSlice, sliceType, descriptor);
    Matrix surfaceValues(m_numSlices[ROW_SLICE_TYPE], m_numSlices[COL_SLICE_TYPE]);
    GenScaledDistanceValues(sliceType, alignSliceInt, refSliceInt, naturalScale, rawScaleOrTiles, surfaceValues);
    GenControlTexFromSurface(axis, surfaceValues);
    CommitChanges();
}

void MeshEntity::GeneralFunction(const GeneralFunctionFactors* sFactors, const GeneralFunctionFactors* tFactors, const SliceDesignation* alignRow,
                                 const SliceDesignation* alignCol, const RefSliceDescriptor* refRow, const RefSliceDescriptor* refCol, bool surfaceValues) {
    (void)alignCol;   // MeshEntity.cpp:551: InternalSliceDesignation( alignRow, COL_SLICE_TYPE ) - so bei NRC
    const int alignRowInt = InternalSliceDesignation(alignRow, ROW_SLICE_TYPE);
    const int alignColInt = InternalSliceDesignation(alignRow, COL_SLICE_TYPE);
    RefSliceDescriptorInt rowDescriptor;
    RefSliceDescriptorInt* refRowInt = InternalRefSliceDescriptor(refRow, ROW_SLICE_TYPE, rowDescriptor);
    RefSliceDescriptorInt colDescriptor;
    RefSliceDescriptorInt* refColInt = InternalRefSliceDescriptor(refCol, COL_SLICE_TYPE, colDescriptor);
    Matrix rowDistances(m_numSlices[ROW_SLICE_TYPE], m_numSlices[COL_SLICE_TYPE]);
    Matrix colDistances(m_numSlices[ROW_SLICE_TYPE], m_numSlices[COL_SLICE_TYPE]);
    if ((sFactors != nullptr && sFactors->rowDistance != 0.0F) || (tFactors != nullptr && tFactors->rowDistance != 0.0F)) {
        GenScaledDistanceValues(ROW_SLICE_TYPE, alignColInt, refRowInt, true, 1.0F, rowDistances);
    }
    if ((sFactors != nullptr && sFactors->colDistance != 0.0F) || (tFactors != nullptr && tFactors->colDistance != 0.0F)) {
        GenScaledDistanceValues(COL_SLICE_TYPE, alignRowInt, refColInt, true, 1.0F, colDistances);
    }
    if (sFactors != nullptr) { GeneralFunctionInt(*sFactors, S_TEX_AXIS, alignRowInt, alignColInt, surfaceValues, rowDistances, colDistances); }
    if (tFactors != nullptr) { GeneralFunctionInt(*tFactors, T_TEX_AXIS, alignRowInt, alignColInt, surfaceValues, rowDistances, colDistances); }
    CommitChanges();
}

void MeshEntity::UpdatePosMinMax(PositionAxis axis) {
    m_posMin[axis] = vertex(0, 0, axis);
    m_posMax[axis] = m_posMin[axis];
    for (unsigned r = 0; r < m_numSlices[ROW_SLICE_TYPE]; ++r) {
        for (unsigned c = 0; c < m_numSlices[COL_SLICE_TYPE]; ++c) {
            const float v = vertex(r, c, axis);
            if (v < m_posMin[axis]) { m_posMin[axis] = v; }
            if (v > m_posMax[axis]) { m_posMax[axis] = v; }
        }
    }
}

void MeshEntity::UpdateTexMinMax(TextureAxis axis) {
    if (!m_texMinMaxDirty[axis]) { return; }
    m_texMin[axis] = texcoord(0, 0, axis);
    m_texMax[axis] = m_texMin[axis];
    for (unsigned r = 0; r < m_numSlices[ROW_SLICE_TYPE]; ++r) {
        for (unsigned c = 0; c < m_numSlices[COL_SLICE_TYPE]; ++c) {
            const float v = texcoord(r, c, axis);
            if (v < m_texMin[axis]) { m_texMin[axis] = v; }
            if (v > m_texMax[axis]) { m_texMax[axis] = v; }
        }
    }
    m_texMinAligned[axis] = std::floor(m_texMin[axis]) == m_texMin[axis];
    m_texMaxAligned[axis] = std::floor(m_texMax[axis]) == m_texMax[axis];
    m_texMinMaxDirty[axis] = false;
}

// CreateUndoPoint (Patch_undoSave) + CommitChanges (Patch_controlPointsChanged)
void MeshEntity::CommitChanges() { m_patch.punkteSetzen(m_meshData); }

int MeshEntity::InternalSliceDesignation(const SliceDesignation* sliceDesignation, SliceType sliceType) {
    if (sliceDesignation != nullptr) {
        if (sliceDesignation->maxSlice) { return static_cast<int>(m_numSlices[sliceType]) - 1; }
        return sliceDesignation->index;
    }
    return 0;
}

MeshEntity::RefSliceDescriptorInt* MeshEntity::InternalRefSliceDescriptor(const RefSliceDescriptor* refSlice, SliceType sliceType,
                                                                          RefSliceDescriptorInt& refSliceInt) {
    if (refSlice != nullptr) {
        refSliceInt.totalLengthOnly = refSlice->totalLengthOnly;
        refSliceInt.index = static_cast<unsigned>(InternalSliceDesignation(&refSlice->designation, sliceType));
        return &refSliceInt;
    }
    return nullptr;
}

float MeshEntity::GetSliceTexScale(SliceType sliceType, int slice, TextureAxis axis, float tiles) {
    SlicePatchContext context;
    context.sliceType = sliceType;
    context.position = slice != 0 ? 1.0F - (static_cast<float>(slice & 0x1) / 2.0F) : 0.0F;
    context.edgeSlice[sliceType] = 2 * ((slice - 1) / 2);
    float cumulativeDistance = 0.0F;
    for (unsigned halfPatch = 2; halfPatch < sliceSize(sliceType); halfPatch += 2) {
        context.edgeSlice[1 - sliceType] = 2 * ((static_cast<int>(halfPatch) - 1) / 2);
        const float segmentLengthEstimate = EstimateSegmentLength(0.0F, 1.0F, context);
        cumulativeDistance += RefineSegmentLength(0.0F, 1.0F, context, segmentLengthEstimate, UNITS_ERROR_BOUND);
    }
    return cumulativeDistance / (tiles * m_naturalTexUnits[axis]);
}

bool MeshEntity::GetSliceTexInfo(SliceType sliceType, int slice, TextureAxis axis, SliceTexInfo& info) {
    if (slice < 0 || slice >= static_cast<int>(m_numSlices[sliceType])) {
        m_meldungen.push_back({2, kErrorBadSlice[sliceType]});
        return false;
    }
    const auto texAt = [&](int index) {
        return sliceType == ROW_SLICE_TYPE ? texcoord(static_cast<unsigned>(slice), static_cast<unsigned>(index), axis)
                                           : texcoord(static_cast<unsigned>(index), static_cast<unsigned>(slice), axis);
    };
    const float texBegin = texAt(0);
    const float texEnd = texAt(static_cast<int>(sliceSize(sliceType)) - 1);
    info.tiles = texEnd - texBegin;
    if (texBegin < texEnd) {
        info.min = texBegin;
        info.max = texEnd;
    } else {
        info.min = texEnd;
        info.max = texBegin;
    }
    info.scale = GetSliceTexScale(sliceType, slice, axis, info.tiles);
    return true;
}

void MeshEntity::ReportSliceTexInfo(SliceType sliceType, int slice, TextureAxis axis, std::string* message, const TexInfoCallback* texInfoCallback) {
    SliceTexInfo info;
    if (!GetSliceTexInfo(sliceType, slice, axis, info)) { return; }
    if (kRadiantScaleInverted[sliceType]) { info.scale = -info.scale; }
    if (kRadiantTilesInverted[sliceType]) { info.tiles = -info.tiles; }
    const bool infscale = info.scale > FLT_MAX || info.scale < -FLT_MAX;
    if (texInfoCallback != nullptr) {
        if (!infscale) {
            (*texInfoCallback)(SanitizeFloat(info.scale), SanitizeFloat(info.tiles));
        } else {
            m_meldungen.push_back({1, kWarningSliceInfscale[sliceType]});
        }
    }
    if (message != nullptr) {
        char b[INFO_BUFFER_SIZE + 1];
        const std::size_t rest = INFO_BUFFER_SIZE - std::min(message->size(), INFO_BUFFER_SIZE);
        if (!infscale) {
            std::snprintf(b, rest + 1, kInfoSliceFormat[sliceType], slice, static_cast<double>(SanitizeFloat(info.scale)),
                          static_cast<double>(SanitizeFloat(info.tiles)), static_cast<double>(SanitizeFloat(info.min)), static_cast<double>(SanitizeFloat(info.max)));
        } else {
            std::snprintf(b, rest + 1, kInfoSliceInfscaleFormat[sliceType], slice, static_cast<double>(SanitizeFloat(info.tiles)),
                          static_cast<double>(SanitizeFloat(info.min)), static_cast<double>(SanitizeFloat(info.max)));
        }
        *message += b;
    }
}

void MeshEntity::ProcessForAxes(InternalImpl internalImpl, TextureAxisSelection axes) {
    bool sChanged = false;
    bool tChanged = false;
    if (axes != T_TEX_AXIS_ONLY) { sChanged = (this->*internalImpl)(S_TEX_AXIS); }
    if (axes != S_TEX_AXIS_ONLY) { tChanged = (this->*internalImpl)(T_TEX_AXIS); }
    if (sChanged || tChanged) { CommitChanges(); }
}

void MeshEntity::Shift(TextureAxis axis, float shift) {
    for (unsigned r = 0; r < m_numSlices[ROW_SLICE_TYPE]; ++r) {
        for (unsigned c = 0; c < m_numSlices[COL_SLICE_TYPE]; ++c) { texcoordRef(r, c, axis) += shift; }
    }
    m_texMinMaxDirty[axis] = true;
}

void MeshEntity::Scale(TextureAxis axis, float scale) {
    UpdateTexMinMax(axis);
    for (unsigned r = 0; r < m_numSlices[ROW_SLICE_TYPE]; ++r) {
        for (unsigned c = 0; c < m_numSlices[COL_SLICE_TYPE]; ++c) {
            texcoordRef(r, c, axis) = ((texcoord(r, c, axis) - m_texMin[axis]) * scale) + m_texMin[axis];
        }
    }
    m_texMinMaxDirty[axis] = true;
}

bool MeshEntity::MinAlignInt(TextureAxis axis) {
    UpdateTexMinMax(axis);
    if (m_texMinAligned[axis]) { return false; }
    Shift(axis, std::ceil(m_texMin[axis]) - m_texMin[axis]);
    return true;
}

bool MeshEntity::MaxAlignInt(TextureAxis axis) {
    UpdateTexMinMax(axis);
    if (m_texMaxAligned[axis]) { return false; }
    Shift(axis, std::ceil(m_texMax[axis]) - m_texMax[axis]);
    return true;
}

bool MeshEntity::MinMaxAlignAutoScaleInt(TextureAxis axis) {
    UpdateTexMinMax(axis);
    if ((m_texMax[axis] - std::floor(m_texMax[axis])) < 0.5) { return MinMaxAlignStretchInt(axis); }
    return MinMaxAlignShrinkInt(axis);
}

bool MeshEntity::MinMaxAlignScale(TextureAxis axis, ScaleOperation op) {
    bool changed = MinAlignInt(axis);
    UpdateTexMinMax(axis);
    if (!m_texMaxAligned[axis]) {
        const float oldRepeats = m_texMax[axis] - m_texMin[axis];
        const float newRepeats = op == STRETCH_SCALE_OP ? std::floor(m_texMax[axis]) - m_texMin[axis] : std::ceil(m_texMax[axis]) - m_texMin[axis];
        Scale(axis, newRepeats / oldRepeats);
        changed = true;
    }
    return changed;
}

bool MeshEntity::MinMaxAlignStretchInt(TextureAxis axis) { return MinMaxAlignScale(axis, STRETCH_SCALE_OP); }
bool MeshEntity::MinMaxAlignShrinkInt(TextureAxis axis) { return MinMaxAlignScale(axis, SHRINK_SCALE_OP); }

float MeshEntity::SliceParametricSpeedComponent(PositionAxis axis, float t, const SlicePatchContext& context) const {
    float a = 1.0F - context.position;
    const float b = 2.0F * context.position * a;
    a *= a;
    const float c = context.position * context.position;
    const float d = (2.0F * t) - 2.0F;
    const float e = 2.0F - (4.0F * t);
    const float f = 2.0F * t;
    const auto sc = static_cast<unsigned>(context.edgeSlice[COL_SLICE_TYPE]);
    const auto sr = static_cast<unsigned>(context.edgeSlice[ROW_SLICE_TYPE]);
    if (context.sliceType == ROW_SLICE_TYPE) {
        return vertex(sr + 0, sc + 0, axis) * a * d + vertex(sr + 0, sc + 1, axis) * a * e + vertex(sr + 0, sc + 2, axis) * a * f +
               vertex(sr + 1, sc + 0, axis) * b * d + vertex(sr + 1, sc + 1, axis) * b * e + vertex(sr + 1, sc + 2, axis) * b * f +
               vertex(sr + 2, sc + 0, axis) * c * d + vertex(sr + 2, sc + 1, axis) * c * e + vertex(sr + 2, sc + 2, axis) * c * f;
    }
    return vertex(sr + 0, sc + 0, axis) * a * d + vertex(sr + 1, sc + 0, axis) * a * e + vertex(sr + 2, sc + 0, axis) * a * f +
           vertex(sr + 0, sc + 1, axis) * b * d + vertex(sr + 1, sc + 1, axis) * b * e + vertex(sr + 2, sc + 1, axis) * b * f +
           vertex(sr + 0, sc + 2, axis) * c * d + vertex(sr + 1, sc + 2, axis) * c * e + vertex(sr + 2, sc + 2, axis) * c * f;
}

float MeshEntity::SliceParametricSpeed(float t, const SlicePatchContext& context) const {
    const float x = SliceParametricSpeedComponent(X_POS_AXIS, t, context);
    const float y = SliceParametricSpeedComponent(Y_POS_AXIS, t, context);
    const float z = SliceParametricSpeedComponent(Z_POS_AXIS, t, context);
    return std::sqrt((x * x) + (y * y) + (z * z));
}

float MeshEntity::EstimateSegmentLength(float startPosition, float endPosition, const SlicePatchContext& context) const {
    static const float x[] = {0.0F, 0.1488743389F, 0.4333953941F, 0.6794095682F, 0.8650633666F, 0.9739065285F};
    static const float w[] = {0.0F, 0.2955242247F, 0.2692667193F, 0.2190863625F, 0.1494513491F, 0.0666713443F};
    const float xm = 0.5F * (endPosition + startPosition);
    const float xr = 0.5F * (endPosition - startPosition);
    float s = 0.0F;
    for (unsigned j = 1; j <= 5; ++j) {
        const float dx = xr * x[j];
        s += w[j] * (SliceParametricSpeed(xm + dx, context) + SliceParametricSpeed(xm - dx, context));
    }
    return std::fabs(s * xr);
}

float MeshEntity::RefineSegmentLength(float startPosition, float endPosition, const SlicePatchContext& context, float segmentLengthEstimate,
                                      float maxError) const {
    const float midPosition = (startPosition + endPosition) / 2.0F;
    float leftLength = EstimateSegmentLength(startPosition, midPosition, context);
    float rightLength = EstimateSegmentLength(midPosition, endPosition, context);
    if (std::fabs(segmentLengthEstimate - (leftLength + rightLength)) > maxError) {
        leftLength = RefineSegmentLength(startPosition, midPosition, context, leftLength, maxError / 2.0F);
        rightLength = RefineSegmentLength(midPosition, endPosition, context, rightLength, maxError / 2.0F);
    }
    return leftLength + rightLength;
}

void MeshEntity::GenControlTexFromSurface(TextureAxis axis, const Matrix& s) {
    const unsigned R = m_numSlices[ROW_SLICE_TYPE];
    const unsigned C = m_numSlices[COL_SLICE_TYPE];
    for (unsigned r = 0; r < R; r += 2) {
        for (unsigned c = 0; c < C; c += 2) { texcoordRef(r, c, axis) = s(r, c); }
    }
    for (unsigned r = 1; r < R; r += 2) {
        for (unsigned c = 0; c < C; c += 2) { texcoordRef(r, c, axis) = 2.0F * s(r, c) - (s(r - 1, c) + s(r + 1, c)) / 2.0F; }
    }
    for (unsigned r = 0; r < R; r += 2) {
        for (unsigned c = 1; c < C; c += 2) { texcoordRef(r, c, axis) = 2.0F * s(r, c) - (s(r, c - 1) + s(r, c + 1)) / 2.0F; }
    }
    for (unsigned r = 1; r < R; r += 2) {
        for (unsigned c = 1; c < C; c += 2) {
            texcoordRef(r, c, axis) = 4.0F * s(r, c) - (s(r, c - 1) + s(r, c + 1) + s(r - 1, c) + s(r + 1, c)) / 2.0F -
                                      (s(r - 1, c - 1) + s(r + 1, c + 1) + s(r - 1, c + 1) + s(r + 1, c - 1)) / 4.0F;
        }
    }
    m_texMinMaxDirty[axis] = true;
}

void MeshEntity::CopyControlTexFromValues(TextureAxis axis, const Matrix& values) {
    for (unsigned r = 0; r < m_numSlices[ROW_SLICE_TYPE]; ++r) {
        for (unsigned c = 0; c < m_numSlices[COL_SLICE_TYPE]; ++c) { texcoordRef(r, c, axis) = values(r, c); }
    }
    m_texMinMaxDirty[axis] = true;
}

void MeshEntity::GenSurfaceFromControlTex(TextureAxis axis, Matrix& s) const {
    const unsigned R = m_numSlices[ROW_SLICE_TYPE];
    const unsigned C = m_numSlices[COL_SLICE_TYPE];
    const auto t = [&](unsigned r, unsigned c) { return texcoord(r, c, axis); };
    for (unsigned r = 0; r < R; r += 2) {
        for (unsigned c = 0; c < C; c += 2) { s(r, c) = t(r, c); }
    }
    for (unsigned r = 1; r < R; r += 2) {
        for (unsigned c = 1; c < C; c += 2) {
            s(r, c) = t(r, c) / 4.0F + (t(r, c - 1) + t(r, c + 1) + t(r - 1, c) + t(r + 1, c)) / 8.0F +
                      (t(r - 1, c - 1) + t(r + 1, c + 1) + t(r - 1, c + 1) + t(r + 1, c - 1)) / 16.0F;
        }
    }
    for (unsigned r = 0; r < R; r += 2) {
        for (unsigned c = 1; c < C; c += 2) { s(r, c) = t(r, c) / 2.0F + (t(r, c - 1) + t(r, c + 1)) / 4.0F; }
    }
    for (unsigned r = 1; r < R; r += 2) {
        for (unsigned c = 0; c < C; c += 2) { s(r, c) = t(r, c) / 2.0F + (t(r - 1, c) + t(r + 1, c)) / 4.0F; }
    }
}

void MeshEntity::CopyValuesFromControlTex(TextureAxis axis, Matrix& values) const {
    for (unsigned r = 0; r < m_numSlices[ROW_SLICE_TYPE]; ++r) {
        for (unsigned c = 0; c < m_numSlices[COL_SLICE_TYPE]; ++c) { values(r, c) = texcoord(r, c, axis); }
    }
}

void MeshEntity::GenScaledDistanceValues(SliceType sliceType, int alignSlice, const RefSliceDescriptorInt* refSlice, bool rawScale, float rawScaleOrTiles,
                                         Matrix& values) {
    SlicePatchContext context;
    context.sliceType = sliceType;
    int firstSlice = 0;
    int lastSlice = 0;
    if (refSlice != nullptr && !refSlice->totalLengthOnly) {
        firstSlice = lastSlice = static_cast<int>(refSlice->index);
    } else {
        firstSlice = 0;
        lastSlice = static_cast<int>(m_numSlices[sliceType]) - 1;
    }
    const int groesse = static_cast<int>(sliceSize(sliceType));
    for (int slice = firstSlice; slice <= lastSlice; ++slice) {
        context.position = slice != 0 ? 1.0F - (static_cast<float>(slice & 0x1) / 2.0F) : 0.0F;
        context.edgeSlice[sliceType] = 2 * ((slice - 1) / 2);
        element(values, sliceType, slice, alignSlice) = 0.0F;
        float cumulativeDistance = 0.0F;
        float slicewiseFraction = static_cast<float>((alignSlice & 0x1) + 1) / 2.0F;
        for (int halfPatch = alignSlice + 1; halfPatch < groesse; ++halfPatch) {
            context.edgeSlice[1 - sliceType] = 2 * ((halfPatch - 1) / 2);
            const float segmentLengthEstimate = EstimateSegmentLength(slicewiseFraction - 0.5F, slicewiseFraction, context);
            cumulativeDistance += RefineSegmentLength(slicewiseFraction - 0.5F, slicewiseFraction, context, segmentLengthEstimate, UNITS_ERROR_BOUND);
            element(values, sliceType, slice, halfPatch) = cumulativeDistance;
            slicewiseFraction = 1.5F - slicewiseFraction;
        }
        cumulativeDistance = 0.0F;
        slicewiseFraction = static_cast<float>((alignSlice - 1) & 0x1) / 2.0F;
        for (int halfPatch = alignSlice - 1; halfPatch >= 0; --halfPatch) {
            context.edgeSlice[1 - sliceType] = 2 * ((halfPatch - 1) / 2);
            const float segmentLengthEstimate = EstimateSegmentLength(slicewiseFraction + 0.5F, slicewiseFraction, context);
            cumulativeDistance -= RefineSegmentLength(slicewiseFraction + 0.5F, slicewiseFraction, context, segmentLengthEstimate, UNITS_ERROR_BOUND);
            element(values, sliceType, slice, halfPatch) = cumulativeDistance;
            slicewiseFraction = 0.5F - slicewiseFraction;
        }
    }
    float refTotalLength = 1.0F;
    if (refSlice != nullptr) {
        refTotalLength = element(values, sliceType, static_cast<int>(refSlice->index), groesse - 1) - element(values, sliceType, static_cast<int>(refSlice->index), 0);
    }
    for (int slice = firstSlice; slice <= lastSlice; ++slice) {
        float scaleFactor = 0;
        if (rawScale) {
            scaleFactor = rawScaleOrTiles;
            if (refSlice != nullptr) {
                scaleFactor *= (element(values, sliceType, slice, groesse - 1) - element(values, sliceType, slice, 0)) / refTotalLength;
            }
        } else {
            scaleFactor = (element(values, sliceType, slice, groesse - 1) - element(values, sliceType, slice, 0)) / rawScaleOrTiles;
        }
        for (int halfPatch = 0; halfPatch < groesse; ++halfPatch) { element(values, sliceType, slice, halfPatch) /= scaleFactor; }
    }
    if (refSlice != nullptr && !refSlice->totalLengthOnly) {
        for (int slice = 0; slice < static_cast<int>(m_numSlices[sliceType]); ++slice) {
            for (int halfPatch = 0; halfPatch < groesse; ++halfPatch) {
                element(values, sliceType, slice, halfPatch) = element(values, sliceType, static_cast<int>(refSlice->index), halfPatch);
            }
        }
    }
}

void MeshEntity::GeneralFunctionInt(const GeneralFunctionFactors& factors, TextureAxis axis, int alignRow, int alignCol, bool surfaceValues,
                                    const Matrix& rowDistances, const Matrix& colDistances) {
    Matrix oldValues(m_numSlices[ROW_SLICE_TYPE], m_numSlices[COL_SLICE_TYPE]);
    Matrix newValues(m_numSlices[ROW_SLICE_TYPE], m_numSlices[COL_SLICE_TYPE]);
    if (factors.oldValue != 0.0F) {
        if (surfaceValues) {
            GenSurfaceFromControlTex(axis, oldValues);
        } else {
            CopyValuesFromControlTex(axis, oldValues);
        }
    }
    for (int r = 0; r < static_cast<int>(m_numSlices[ROW_SLICE_TYPE]); ++r) {
        for (int c = 0; c < static_cast<int>(m_numSlices[COL_SLICE_TYPE]); ++c) {
            const auto ur = static_cast<unsigned>(r);
            const auto uc = static_cast<unsigned>(c);
            newValues(ur, uc) = factors.oldValue * oldValues(ur, uc) + factors.rowDistance * rowDistances(ur, uc) + factors.colDistance * colDistances(ur, uc) +
                                factors.rowNumber * static_cast<float>(r - alignRow) + factors.colNumber * static_cast<float>(c - alignCol) + factors.constant;
        }
    }
    if (surfaceValues) {
        GenControlTexFromSurface(axis, newValues);
    } else {
        CopyControlTexFromValues(axis, newValues);
    }
}

}  // namespace bhed::karte::meshtex
