// terraingen.cpp - Karten-Editor, Stufe 8b Punkt 1: der Terrain Generator
// von NetRadiant-Custom ohne Oberflaeche (siehe terraingen.h).
//
// Portiert aus NetRadiant-Custom master (origin/master 8216133, nicht im
// Release 20260114), contrib/terrain_generator: noise.cpp, terrain_engine.cpp,
// brush_builder.cpp, terrain_generator.cpp (do_generate, query_sel,
// read_stored_size, write_stored_size, instance_entity). Zeile fuer Zeile
// dieselbe Rechnung in double, dieselben Aufrufe von rand() in derselben
// Reihenfolge - der ctest karte_terrain vergleicht Hoehenkarten und Pinsel
// mit NRCs eigenem Code (tests/karte/terrain_nrc).
//
//   Terrain Generator for NetRadiant-Custom
//   Copyright (C) 2026 vallz and vld ("Developed by vallz and vld", About-Text;
//   die Quelldateien tragen keinen eigenen Kopf, es gilt die Lizenz von
//   NetRadiant-Custom).
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; version 2 of the License. This program is distributed
//   in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
//   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//   PURPOSE. See the GNU General Public License for more details (LICENSE).
#include "bhed/karte/terraingen.h"

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/rueckgaengig.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <numbers>
#include <sstream>

namespace bhed::karte::terraingen {

const int kQuadrate[8] = {8, 16, 32, 64, 128, 256, 512, 1024};
const int kRasterSchritte[11] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
const char* const kAbout = "Terrain Generator\n\nProcedural CSG generation tool for id Tech 3 engines.\n\nDeveloped by vallz and vld";
const char* const kStandardTextur = "textures/";
const char* const kGroessenSchluessel = "_terraingen_size";

// ---------------------------------------------------------------------------
// noise.cpp
// ---------------------------------------------------------------------------
namespace {

const int kPerm[256] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,
    23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,
    174, 20,  125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230,
    220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169,
    200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
    255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,
    221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228,
    251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157,
    184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,
    215, 61,  156, 180};

// perlin_p / simplex_perm: beide aus derselben Tabelle verdoppelt (perlin_init, simplex_init)
struct Perm512 {
    int p[512];
    Perm512() {
        for (int i = 0; i < 256; ++i) { p[i] = p[i + 256] = kPerm[i]; }
    }
};
const Perm512& perm() {
    static const Perm512 t;
    return t;
}

double perlin_fade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
double perlin_lerp(double t, double a, double b) { return a + t * (b - a); }
double perlin_grad(int hash, double x, double y) {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

const int grad3[12][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}, {1, 0}, {-1, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {0, 1}, {0, -1}};

int simplex_fast_floor(double x) {
    int xi = static_cast<int>(x);
    return x < xi ? xi - 1 : xi;
}
double simplex_dot(const int g[2], double x, double y) { return g[0] * x + g[1] * y; }

}  // namespace

double perlinNoise(double x, double y) {
    const int* perlin_p = perm().p;
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    double u = perlin_fade(x);
    double v = perlin_fade(y);
    int A = perlin_p[X] + Y;
    int B = perlin_p[X + 1] + Y;
    return perlin_lerp(v, perlin_lerp(u, perlin_grad(perlin_p[A], x, y), perlin_grad(perlin_p[B], x - 1.0, y)),
                       perlin_lerp(u, perlin_grad(perlin_p[A + 1], x, y - 1.0), perlin_grad(perlin_p[B + 1], x - 1.0, y - 1.0)));
}

double simplexNoise(double x, double y) {
    const int* simplex_perm = perm().p;
    const double F2 = 0.5 * (std::sqrt(3.0) - 1.0);
    const double G2 = (3.0 - std::sqrt(3.0)) / 6.0;

    double s = (x + y) * F2;
    int i = simplex_fast_floor(x + s);
    int j = simplex_fast_floor(y + s);

    double t = (i + j) * G2;
    double x0 = x - (i - t);
    double y0 = y - (j - t);

    int i1 = x0 > y0 ? 1 : 0;
    int j1 = x0 > y0 ? 0 : 1;

    double x1 = x0 - i1 + G2;
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + 2.0 * G2;
    double y2 = y0 - 1.0 + 2.0 * G2;

    int ii = i & 255;
    int jj = j & 255;
    int gi0 = simplex_perm[ii + simplex_perm[jj]] % 12;
    int gi1 = simplex_perm[ii + i1 + simplex_perm[jj + j1]] % 12;
    int gi2 = simplex_perm[ii + 1 + simplex_perm[jj + 1]] % 12;

    double t0 = 0.5 - x0 * x0 - y0 * y0;
    double n0 = t0 < 0.0 ? 0.0 : (t0 * t0 * t0 * t0) * simplex_dot(grad3[gi0], x0, y0);

    double t1 = 0.5 - x1 * x1 - y1 * y1;
    double n1 = t1 < 0.0 ? 0.0 : (t1 * t1 * t1 * t1) * simplex_dot(grad3[gi1], x1, y1);

    double t2 = 0.5 - x2 * x2 - y2 * y2;
    double n2 = t2 < 0.0 ? 0.0 : (t2 * t2 * t2 * t2) * simplex_dot(grad3[gi2], x2, y2);

    return 70.0 * (n0 + n1 + n2);
}

// ---------------------------------------------------------------------------
// terrain_engine.cpp
// ---------------------------------------------------------------------------
namespace {

double round2(double v) { return std::round(v * 100.0) / 100.0; }

double random_double(const Zufall& z) { return static_cast<double>(z()) / static_cast<double>(kRandMax); }

double sample_noise(NoiseType noise_type, double x, double y, const Zufall& z) {
    switch (noise_type) {
    case NoiseType::Perlin: return perlinNoise(x, y);
    case NoiseType::Simplex: return simplexNoise(x, y);
    default: return random_double(z) * 2.0 - 1.0;
    }
}

// repair_thin_triangles (terrain_engine.cpp:30-79)
void repair_thin_triangles(TerrainMap& height_map, const BrushData& target, double step_x, double step_y) {
    constexpr double MIN_AREA_FRACTION = 0.15;
    const double min_area = MIN_AREA_FRACTION * 0.5 * step_x * step_y;

    auto tri_area = [](const GridPoint& a, const GridPoint& b, const GridPoint& c) -> double {
        return 0.5 * std::abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
    };
    auto lookup = [&](double kx, double ky) -> GridPoint* {
        auto it = height_map.find({round2(kx), round2(ky)});
        return it != height_map.end() ? &it->second : nullptr;
    };
    auto revert = [&](double gx, double gy) {
        if (GridPoint* p = lookup(gx, gy)) {
            p->x = gx;
            p->y = gy;
        }
    };

    int x_index = 0;
    for (double x = target.min_x; x < target.max_x - 0.01; x += step_x, ++x_index) {
        int y_index = 0;
        for (double y = target.min_y; y < target.max_y - 0.01; y += step_y, ++y_index) {
            double mx = x + step_x < target.max_x ? x + step_x : target.max_x;
            double my = y + step_y < target.max_y ? y + step_y : target.max_y;

            GridPoint* bl = lookup(x, y);
            GridPoint* tl = lookup(x, my);
            GridPoint* br = lookup(mx, y);
            GridPoint* tr = lookup(mx, my);
            if (!bl || !tl || !br || !tr) { continue; }

            bool alt_dir = ((x_index + y_index) % 2) != 0;
            bool bad = !alt_dir ? (tri_area(*bl, *tl, *br) < min_area || tri_area(*tr, *br, *tl) < min_area)
                                : (tri_area(*tl, *tr, *bl) < min_area || tri_area(*tr, *br, *bl) < min_area);

            if (bad) {
                revert(x, y);
                revert(x, my);
                revert(mx, y);
                revert(mx, my);
            }
        }
    }
}

}  // namespace

BrushData make_manual_brush_data(double width, double length, double height) {
    BrushData b;
    b.min_x = -width / 2.0;
    b.max_x = width / 2.0;
    b.min_y = -length / 2.0;
    b.max_y = length / 2.0;
    b.min_z = 0.0;
    b.max_z = height;
    b.width_x = width;
    b.length_y = length;
    b.height_z = height;
    return b;
}

void adjust_bounds_to_fit_grid(BrushData& target, double step_x, double step_y) {
    double new_width = std::max(step_x, std::round(target.width_x / step_x) * step_x);
    double new_length = std::max(step_y, std::round(target.length_y / step_y) * step_y);

    if (std::abs(target.width_x - new_width) > 0.001 || std::abs(target.length_y - new_length) > 0.001) {
        double diff_x = new_width - target.width_x;
        double diff_y = new_length - target.length_y;

        target.min_x = std::round(target.min_x - diff_x / 2.0);
        target.max_x = target.min_x + new_width;
        target.min_y = std::round(target.min_y - diff_y / 2.0);
        target.max_y = target.min_y + new_length;
        target.width_x = new_width;
        target.length_y = new_length;
    }
}

TerrainMap generate_height_map(const BrushData& target, double step_x, double step_y, ShapeType shape_type, double shape_height, double variance,
                               double frequency, NoiseType noise_type, double terrace_step, Axis axis, bool jitter_grid, double grid_step,
                               const Zufall& zufall) {
    TerrainMap height_map;

    double seed_x = random_double(zufall) * 10000.0;
    double seed_y = random_double(zufall) * 10000.0;

    constexpr double JITTER_FRACTION = 0.35;

    for (double x = target.min_x; x <= target.max_x + 0.01; x += step_x) {
        for (double y = target.min_y; y <= target.max_y + 0.01; y += step_y) {
            double nx = target.width_x > 0 ? (x - target.min_x) / target.width_x : 0.0;
            double ny = target.length_y > 0 ? (y - target.min_y) / target.length_y : 0.0;

            double na = (axis == Axis::X) ? nx : ny;

            double center_dist = std::min(1.0, std::sqrt((nx - 0.5) * (nx - 0.5) + (ny - 0.5) * (ny - 0.5)) / 0.5);

            double base_z = 0.0;
            switch (shape_type) {
            case ShapeType::Hill: {
                base_z = shape_height * 0.5 * (1.0 + std::cos(center_dist * std::numbers::pi));
                break;
            }
            case ShapeType::Crater: {
                base_z = shape_height * 0.5 * (1.0 - std::cos(center_dist * std::numbers::pi));
                break;
            }
            case ShapeType::Ridge: {
                double dist = std::min(1.0, std::abs(na - 0.5) / 0.5);
                base_z = shape_height * 0.5 * (1.0 + std::cos(dist * std::numbers::pi));
                break;
            }
            case ShapeType::Slope: {
                base_z = shape_height * na;
                break;
            }
            case ShapeType::Volcano: {
                double mountain = shape_height * 0.5 * (1.0 + std::cos(center_dist * std::numbers::pi));
                double crater_dist = std::min(1.0, center_dist / 0.35);
                double crater = (shape_height * 0.7) * 0.5 * (1.0 + std::cos(crater_dist * std::numbers::pi));
                base_z = mountain - crater;
                break;
            }
            case ShapeType::Valley: {
                double dist = std::min(1.0, std::abs(na - 0.5) / 0.5);
                base_z = shape_height * 0.5 * (1.0 - std::cos(dist * std::numbers::pi));
                break;
            }
            default: break;
            }

            double shape_z = target.max_z + base_z;
            if (shape_type != ShapeType::Flat && terrace_step > 0.0) { shape_z = std::floor(shape_z / terrace_step) * terrace_step; }

            double noise_z = 0.0;
            if (variance > 0.0) {
                if (noise_type == NoiseType::Random) {
                    noise_z = (random_double(zufall) * (variance * 2.0)) - variance;
                } else {
                    noise_z = sample_noise(noise_type, (x + seed_x) * frequency, (y + seed_y) * frequency, zufall) * variance;
                }
            }

            double snapped_noise = std::round(noise_z / grid_step) * grid_step;
            double final_z = std::round(shape_z + snapped_noise);

            constexpr double MIN_THICKNESS = 4.0;
            final_z = std::max(final_z, target.min_z + MIN_THICKNESS);

            auto grid_jitter = [&](double step) -> double {
                int max_steps = static_cast<int>(std::floor((JITTER_FRACTION * step) / grid_step));
                if (max_steps <= 0) { return 0.0; }
                int steps = static_cast<int>(random_double(zufall) * (2 * max_steps + 1)) - max_steps;
                return std::clamp(steps, -max_steps, max_steps) * grid_step;
            };

            double jx = x, jy = y;
            if (jitter_grid) {
                const bool interior_x = (x > target.min_x + 0.01 && x < target.max_x - 0.01);
                const bool interior_y = (y > target.min_y + 0.01 && y < target.max_y - 0.01);
                if (interior_x) { jx = std::round(x + grid_jitter(step_x)); }
                if (interior_y) { jy = std::round(y + grid_jitter(step_y)); }
            }

            height_map[{round2(x), round2(y)}] = GridPoint{jx, jy, final_z};
        }
    }

    if (jitter_grid) { repair_thin_triangles(height_map, target, step_x, step_y); }

    return height_map;
}

TunnelMaps generate_tunnel_height_maps(const BrushData& target, double step_x, double step_y, double cave_height, double slope_height,
                                       double variance, double frequency, NoiseType noise_type, double terrace_step, Axis axis, double grid_step,
                                       const Zufall& zufall) {
    TunnelMaps result;

    double seed_floor_x = random_double(zufall) * 10000.0;
    double seed_floor_y = random_double(zufall) * 10000.0;
    double seed_ceil_x = random_double(zufall) * 10000.0;
    double seed_ceil_y = random_double(zufall) * 10000.0;
    double seed_wall_l = random_double(zufall) * 10000.0;
    double seed_wall_r = random_double(zufall) * 10000.0;

    const bool along_y = (axis == Axis::Y);
    const double center_cs = along_y ? (target.min_x + target.max_x) / 2.0 : (target.min_y + target.max_y) / 2.0;
    const double half_cs = along_y ? target.width_x / 2.0 : target.length_y / 2.0;

    for (double x = target.min_x; x <= target.max_x + 0.01; x += step_x) {
        for (double y = target.min_y; y <= target.max_y + 0.01; y += step_y) {
            double cs_pos = along_y ? x : y;
            double t = half_cs > 0 ? std::min(1.0, std::abs(cs_pos - center_cs) / half_cs) : 0.0;
            double blend = 1.0 - std::sqrt(std::max(0.0, 1.0 - t * t));

            double along_pos = along_y ? (target.length_y > 0 ? (y - target.min_y) / target.length_y : 0.0)
                                       : (target.width_x > 0 ? (x - target.min_x) / target.width_x : 0.0);
            double base_z = target.max_z + slope_height * along_pos;
            double floor_base = base_z + blend * (cave_height * 0.25);
            double ceil_base = base_z + cave_height - blend * (cave_height * 0.25);

            if (terrace_step > 0.0) {
                floor_base = std::floor(floor_base / terrace_step) * terrace_step;
                ceil_base = std::ceil(ceil_base / terrace_step) * terrace_step;
            }

            double floor_noise = 0.0, ceil_noise = 0.0;
            if (variance > 0.0) {
                if (noise_type == NoiseType::Random) {
                    floor_noise = random_double(zufall) * variance;
                    ceil_noise = random_double(zufall) * variance;
                } else {
                    floor_noise = std::abs(sample_noise(noise_type, (x + seed_floor_x) * frequency, (y + seed_floor_y) * frequency, zufall)) * variance;
                    ceil_noise = std::abs(sample_noise(noise_type, (x + seed_ceil_x) * frequency, (y + seed_ceil_y) * frequency, zufall)) * variance;
                }
            }

            double snapped_floor_noise = std::round(floor_noise / grid_step) * grid_step;
            double snapped_ceil_noise = std::round(ceil_noise / grid_step) * grid_step;

            double floor_z = floor_base + snapped_floor_noise;
            double ceil_z = ceil_base - snapped_ceil_noise;

            if (floor_z > ceil_z) {
                double mid = (floor_z + ceil_z) / 2.0;
                floor_z = mid;
                ceil_z = mid;
            }

            result.floor_map[{round2(x), round2(y)}] = std::round(floor_z);
            result.ceiling_map[{round2(x), round2(y)}] = std::round(ceil_z);
        }
    }

    double total_wall_height = cave_height + std::abs(slope_height);
    int num_z_steps = std::max(1, static_cast<int>(std::round(total_wall_height / step_x)));
    double step_z = total_wall_height / num_z_steps;
    double wall_min_z = target.max_z + std::min(0.0, slope_height);
    double wall_max_z = wall_min_z + total_wall_height;
    result.step_z = step_z;

    double along_min = along_y ? target.min_y : target.min_x;
    double along_max = along_y ? target.max_y : target.max_x;
    double along_step = along_y ? step_y : step_x;
    double wall_lo = along_y ? target.min_x : target.min_y;
    double wall_hi = along_y ? target.max_x : target.max_y;

    for (double u = along_min; u <= along_max + 0.01; u += along_step) {
        for (double z = wall_min_z; z <= wall_max_z + 0.01; z += step_z) {
            double ru = round2(u);
            double rz = round2(z);

            double wall_noise = 0.0;
            if (variance > 0.0) {
                if (noise_type == NoiseType::Random) {
                    wall_noise = random_double(zufall) * variance;
                } else {
                    wall_noise = std::abs(sample_noise(noise_type, (u + seed_wall_l) * frequency, (z + seed_wall_l) * frequency, zufall)) * variance;
                }
            }
            result.left_wall_map[{ru, rz}] = std::round(wall_lo + std::round(wall_noise / grid_step) * grid_step);

            wall_noise = 0.0;
            if (variance > 0.0) {
                if (noise_type == NoiseType::Random) {
                    wall_noise = random_double(zufall) * variance;
                } else {
                    wall_noise = std::abs(sample_noise(noise_type, (u + seed_wall_r) * frequency, (z + seed_wall_r) * frequency, zufall)) * variance;
                }
            }
            result.right_wall_map[{ru, rz}] = std::round(wall_hi - std::round(wall_noise / grid_step) * grid_step);
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// brush_builder.cpp
// ---------------------------------------------------------------------------
namespace {

constexpr unsigned int BRUSH_DETAIL_FLAG = 27;
constexpr unsigned int BRUSH_DETAIL_MASK = (1U << BRUSH_DETAIL_FLAG);

void fill_face(FaceData& face, double x0, double y0, double z0, double x1, double y1, double z1, double x2, double y2, double z2, const char* shader) {
    face.p0[0] = x0;
    face.p0[1] = y0;
    face.p0[2] = z0;
    face.p1[0] = x1;
    face.p1[1] = y1;
    face.p1[2] = z1;
    face.p2[0] = x2;
    face.p2[1] = y2;
    face.p2[2] = z2;
    face.shader = shader;
    face.scale[0] = 0.03125F;   // matches ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) in .map
    face.scale[1] = 0.03125F;
    face.shift[0] = 0;
    face.shift[1] = 0;
    face.rotate = 0;
    face.contents = static_cast<int>(BRUSH_DETAIL_MASK);
    face.flags = 0;
    face.value = 0;
}

// GlobalBrushCreator().createBrush() / Brush_addFace / Node_getTraversable( entity )->insert( brush )
struct Pinsel1 {
    std::vector<FaceData> flaechen;
    void add(const FaceData& f) { flaechen.push_back(f); }
};
void insert(Bau& bau, int entity, Pinsel1& brush) { bau.gruppen[static_cast<std::size_t>(entity)].push_back(std::move(brush.flaechen)); }

int create_func_group(Bau& bau) {
    bau.gruppen.emplace_back();
    return static_cast<int>(bau.gruppen.size()) - 1;
}

void select_generated(Bau& bau, int node) { bau.gewaehlt.push_back(node); }

void insert_brush_into(Bau& bau, int entity, double min_z, double base_max_z, const GridPoint& bl, const GridPoint& tl, const GridPoint& br,
                       const GridPoint& tr, const char* top_tex, const char* caulk, bool split_diagonally, bool alt_dir) {
    if (!split_diagonally) {
        Pinsel1 brush;
        FaceData face;
        fill_face(face, bl.x, bl.y, base_max_z, tl.x, tl.y, base_max_z, br.x, br.y, base_max_z, top_tex);
        brush.add(face);
        fill_face(face, bl.x, bl.y, min_z, br.x, br.y, min_z, tl.x, tl.y, min_z, caulk);
        brush.add(face);
        fill_face(face, br.x, br.y, min_z, br.x, br.y, base_max_z, tr.x, tr.y, min_z, caulk);
        brush.add(face);
        fill_face(face, bl.x, bl.y, min_z, tl.x, tl.y, min_z, bl.x, bl.y, base_max_z, caulk);
        brush.add(face);
        fill_face(face, tl.x, tl.y, min_z, tr.x, tr.y, min_z, tl.x, tl.y, base_max_z, caulk);
        brush.add(face);
        fill_face(face, bl.x, bl.y, min_z, bl.x, bl.y, base_max_z, br.x, br.y, min_z, caulk);
        brush.add(face);
        insert(bau, entity, brush);
    } else if (!alt_dir) {
        {
            Pinsel1 brush;
            FaceData face;
            fill_face(face, bl.x, bl.y, bl.z, tl.x, tl.y, tl.z, br.x, br.y, br.z, top_tex);
            brush.add(face);
            fill_face(face, bl.x, bl.y, min_z, br.x, br.y, min_z, tl.x, tl.y, min_z, caulk);
            brush.add(face);
            fill_face(face, bl.x, bl.y, min_z, tl.x, tl.y, min_z, bl.x, bl.y, base_max_z, caulk);
            brush.add(face);
            fill_face(face, bl.x, bl.y, min_z, bl.x, bl.y, base_max_z, br.x, br.y, min_z, caulk);
            brush.add(face);
            fill_face(face, br.x, br.y, min_z, br.x, br.y, base_max_z, tl.x, tl.y, min_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
        {
            Pinsel1 brush;
            FaceData face;
            fill_face(face, tr.x, tr.y, tr.z, br.x, br.y, br.z, tl.x, tl.y, tl.z, top_tex);
            brush.add(face);
            fill_face(face, tr.x, tr.y, min_z, tl.x, tl.y, min_z, br.x, br.y, min_z, caulk);
            brush.add(face);
            fill_face(face, br.x, br.y, min_z, br.x, br.y, base_max_z, tr.x, tr.y, min_z, caulk);
            brush.add(face);
            fill_face(face, tl.x, tl.y, min_z, tr.x, tr.y, min_z, tl.x, tl.y, base_max_z, caulk);
            brush.add(face);
            fill_face(face, tl.x, tl.y, min_z, tl.x, tl.y, base_max_z, br.x, br.y, min_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
    } else {
        {
            Pinsel1 brush;
            FaceData face;
            fill_face(face, tl.x, tl.y, tl.z, tr.x, tr.y, tr.z, bl.x, bl.y, bl.z, top_tex);
            brush.add(face);
            fill_face(face, tl.x, tl.y, min_z, bl.x, bl.y, min_z, tr.x, tr.y, min_z, caulk);
            brush.add(face);
            fill_face(face, bl.x, bl.y, min_z, tl.x, tl.y, min_z, bl.x, bl.y, base_max_z, caulk);
            brush.add(face);
            fill_face(face, tl.x, tl.y, min_z, tr.x, tr.y, min_z, tl.x, tl.y, base_max_z, caulk);
            brush.add(face);
            fill_face(face, tr.x, tr.y, min_z, bl.x, bl.y, min_z, bl.x, bl.y, base_max_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
        {
            Pinsel1 brush;
            FaceData face;
            fill_face(face, tr.x, tr.y, tr.z, br.x, br.y, br.z, bl.x, bl.y, bl.z, top_tex);
            brush.add(face);
            fill_face(face, tr.x, tr.y, min_z, bl.x, bl.y, min_z, br.x, br.y, min_z, caulk);
            brush.add(face);
            fill_face(face, br.x, br.y, min_z, br.x, br.y, base_max_z, tr.x, tr.y, min_z, caulk);
            brush.add(face);
            fill_face(face, bl.x, bl.y, min_z, bl.x, bl.y, base_max_z, br.x, br.y, min_z, caulk);
            brush.add(face);
            fill_face(face, bl.x, bl.y, min_z, tr.x, tr.y, min_z, bl.x, bl.y, base_max_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
    }
}

void insert_floor_ceil_brush(Bau& bau, int entity, double x, double y, double mx, double my, double z_bl, double z_tl, double z_br, double z_tr,
                             double min_z, double solid_top, const char* top_tex, const char* caulk, bool is_ceiling, bool alt_dir) {
    FaceData face;

    if (!alt_dir) {
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fill_face(face, x, y, z_bl, x, my, z_tl, mx, y, z_br, top_tex);
                brush.add(face);
                fill_face(face, x, y, min_z, mx, y, min_z, x, my, min_z, caulk);
                brush.add(face);
            } else {
                fill_face(face, x, y, z_bl, mx, y, z_br, x, my, z_tl, top_tex);
                brush.add(face);
                fill_face(face, x, y, solid_top, x, my, solid_top, mx, y, solid_top, caulk);
                brush.add(face);
            }
            fill_face(face, x, y, min_z, x, my, min_z, x, y, solid_top, caulk);
            brush.add(face);
            fill_face(face, x, y, min_z, x, y, solid_top, mx, y, min_z, caulk);
            brush.add(face);
            fill_face(face, mx, y, min_z, mx, y, solid_top, x, my, min_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fill_face(face, mx, my, z_tr, mx, y, z_br, x, my, z_tl, top_tex);
                brush.add(face);
                fill_face(face, mx, my, min_z, x, my, min_z, mx, y, min_z, caulk);
                brush.add(face);
            } else {
                fill_face(face, mx, my, z_tr, x, my, z_tl, mx, y, z_br, top_tex);
                brush.add(face);
                fill_face(face, mx, my, solid_top, mx, y, solid_top, x, my, solid_top, caulk);
                brush.add(face);
            }
            fill_face(face, mx, y, min_z, mx, y, solid_top, mx, my, min_z, caulk);
            brush.add(face);
            fill_face(face, x, my, min_z, mx, my, min_z, x, my, solid_top, caulk);
            brush.add(face);
            fill_face(face, x, my, min_z, x, my, solid_top, mx, y, min_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
    } else {
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fill_face(face, x, my, z_tl, mx, my, z_tr, x, y, z_bl, top_tex);
                brush.add(face);
                fill_face(face, x, my, min_z, x, y, min_z, mx, my, min_z, caulk);
                brush.add(face);
            } else {
                fill_face(face, x, my, z_tl, x, y, z_bl, mx, my, z_tr, top_tex);
                brush.add(face);
                fill_face(face, x, my, solid_top, mx, my, solid_top, x, y, solid_top, caulk);
                brush.add(face);
            }
            fill_face(face, x, y, min_z, x, my, min_z, x, y, solid_top, caulk);
            brush.add(face);
            fill_face(face, x, my, min_z, mx, my, min_z, x, my, solid_top, caulk);
            brush.add(face);
            fill_face(face, mx, my, min_z, x, y, min_z, x, y, solid_top, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fill_face(face, mx, my, z_tr, mx, y, z_br, x, y, z_bl, top_tex);
                brush.add(face);
                fill_face(face, mx, my, min_z, x, y, min_z, mx, y, min_z, caulk);
                brush.add(face);
            } else {
                fill_face(face, mx, my, z_tr, x, y, z_bl, mx, y, z_br, top_tex);
                brush.add(face);
                fill_face(face, mx, my, solid_top, mx, y, solid_top, x, y, solid_top, caulk);
                brush.add(face);
            }
            fill_face(face, mx, y, min_z, mx, y, solid_top, mx, my, min_z, caulk);
            brush.add(face);
            fill_face(face, x, y, min_z, x, y, solid_top, mx, y, min_z, caulk);
            brush.add(face);
            fill_face(face, x, y, min_z, mx, my, min_z, x, y, solid_top, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
    }
}

void insert_wall_brush(Bau& bau, int entity, double x_bl, double x_tl, double x_br, double x_tr, double gu, double gmx_u, double gz, double gmx_z,
                       double outer, double limit, const char* top_tex, const char* caulk, bool is_left, bool alt_dir, Axis axis) {
    FaceData face;
    const bool is_ceiling = !is_left;
    const double min_z = is_left ? outer : limit;
    const double solid_top = is_left ? limit : outer;

    const double x = gu, y = gz, mx = gmx_u, my = gmx_z;
    const double z_bl = x_bl, z_tl = x_tl, z_br = x_br, z_tr = x_tr;

    auto fw = [&](double fx0, double fy0, double fz0, double fx1, double fy1, double fz1, double fx2, double fy2, double fz2, const char* shader) {
        if (axis == Axis::Y) {
            fill_face(face, fz0, fx0, fy0, fz1, fx1, fy1, fz2, fx2, fy2, shader);
        } else {
            fill_face(face, fx0, fz0, fy0, fx2, fz2, fy2, fx1, fz1, fy1, shader);
        }
    };

    if (!alt_dir) {
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fw(x, y, z_bl, x, my, z_tl, mx, y, z_br, top_tex);
                brush.add(face);
                fw(x, y, min_z, mx, y, min_z, x, my, min_z, caulk);
                brush.add(face);
            } else {
                fw(x, y, z_bl, mx, y, z_br, x, my, z_tl, top_tex);
                brush.add(face);
                fw(x, y, solid_top, x, my, solid_top, mx, y, solid_top, caulk);
                brush.add(face);
            }
            fw(x, y, min_z, x, my, min_z, x, y, solid_top, caulk);
            brush.add(face);
            fw(x, y, min_z, x, y, solid_top, mx, y, min_z, caulk);
            brush.add(face);
            fw(mx, y, min_z, mx, y, solid_top, x, my, min_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fw(mx, my, z_tr, mx, y, z_br, x, my, z_tl, top_tex);
                brush.add(face);
                fw(mx, my, min_z, x, my, min_z, mx, y, min_z, caulk);
                brush.add(face);
            } else {
                fw(mx, my, z_tr, x, my, z_tl, mx, y, z_br, top_tex);
                brush.add(face);
                fw(mx, my, solid_top, mx, y, solid_top, x, my, solid_top, caulk);
                brush.add(face);
            }
            fw(mx, y, min_z, mx, y, solid_top, mx, my, min_z, caulk);
            brush.add(face);
            fw(x, my, min_z, mx, my, min_z, x, my, solid_top, caulk);
            brush.add(face);
            fw(x, my, min_z, x, my, solid_top, mx, y, min_z, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
    } else {
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fw(x, my, z_tl, mx, my, z_tr, x, y, z_bl, top_tex);
                brush.add(face);
                fw(x, my, min_z, x, y, min_z, mx, my, min_z, caulk);
                brush.add(face);
            } else {
                fw(x, my, z_tl, x, y, z_bl, mx, my, z_tr, top_tex);
                brush.add(face);
                fw(x, my, solid_top, mx, my, solid_top, x, y, solid_top, caulk);
                brush.add(face);
            }
            fw(x, y, min_z, x, my, min_z, x, y, solid_top, caulk);
            brush.add(face);
            fw(x, my, min_z, mx, my, min_z, x, my, solid_top, caulk);
            brush.add(face);
            fw(mx, my, min_z, x, y, min_z, x, y, solid_top, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
        {
            Pinsel1 brush;
            if (!is_ceiling) {
                fw(mx, my, z_tr, mx, y, z_br, x, y, z_bl, top_tex);
                brush.add(face);
                fw(mx, my, min_z, x, y, min_z, mx, y, min_z, caulk);
                brush.add(face);
            } else {
                fw(mx, my, z_tr, x, y, z_bl, mx, y, z_br, top_tex);
                brush.add(face);
                fw(mx, my, solid_top, mx, y, solid_top, x, y, solid_top, caulk);
                brush.add(face);
            }
            fw(mx, y, min_z, mx, y, solid_top, mx, my, min_z, caulk);
            brush.add(face);
            fw(x, y, min_z, x, y, solid_top, mx, y, min_z, caulk);
            brush.add(face);
            fw(x, y, min_z, mx, my, min_z, x, y, solid_top, caulk);
            brush.add(face);
            insert(bau, entity, brush);
        }
    }
}

std::string zahlText(double v) {   // std::ostream << double (Vorgabe: 6 signifikante Stellen)
    std::ostringstream s;
    s << v;
    return s.str();
}

}  // namespace

void build_terrain_brushes(const BrushData& target, double step_x, double step_y, const TerrainMap& height_map, const char* top_texture,
                           bool split_diagonally, Bau& bau) {
    const int entity = create_func_group(bau);

    const char* caulk = "textures/common/caulk";
    double min_z = target.min_z;
    double base_max_z = target.max_z;

    auto r2 = [](double v) { return std::round(v * 100.0) / 100.0; };

    int x_index = 0;
    for (double x = target.min_x; x < target.max_x - 0.01; x += step_x, ++x_index) {
        int y_index = 0;
        for (double y = target.min_y; y < target.max_y - 0.01; y += step_y, ++y_index) {
            double mx = x + step_x < target.max_x ? x + step_x : target.max_x;
            double my = y + step_y < target.max_y ? y + step_y : target.max_y;

            auto lookup = [&](double kx, double ky) -> GridPoint {
                auto it = height_map.find({r2(kx), r2(ky)});
                return it != height_map.end() ? it->second : GridPoint{kx, ky, min_z};
            };

            GridPoint bl = lookup(x, y);
            GridPoint tl = lookup(x, my);
            GridPoint br = lookup(mx, y);
            GridPoint tr = lookup(mx, my);

            bool alt_dir = ((x_index + y_index) % 2) != 0;

            insert_brush_into(bau, entity, min_z, base_max_z, bl, tl, br, tr, top_texture, caulk, split_diagonally, alt_dir);
        }
    }

    select_generated(bau, entity);
}

void build_tunnel_brushes(const BrushData& target, double step_x, double step_y, const TunnelMaps& maps, const char* top_texture,
                          double cave_height, double slope_height, Axis axis, Bau& bau) {
    const int floor_entity = create_func_group(bau);
    const int ceil_entity = create_func_group(bau);
    const int lwall_entity = create_func_group(bau);
    const int rwall_entity = create_func_group(bau);

    const char* caulk = "textures/common/caulk";
    double min_z = target.min_z;
    double max_ceil_z = target.max_z + cave_height + std::max(0.0, slope_height);
    double ceil_solid_top = max_ceil_z + target.height_z;

    auto r2 = [](double v) { return std::round(v * 100.0) / 100.0; };

    int x_index = 0;
    for (double x = target.min_x; x < target.max_x - 0.01; x += step_x, ++x_index) {
        int y_index = 0;
        for (double y = target.min_y; y < target.max_y - 0.01; y += step_y, ++y_index) {
            double mx = x + step_x < target.max_x ? x + step_x : target.max_x;
            double my = y + step_y < target.max_y ? y + step_y : target.max_y;

            auto safe_at = [&](const HeightMap& m, double kx, double ky, double fallback) -> double {
                auto it = m.find({r2(kx), r2(ky)});
                if (it == m.end()) {
                    bau.fehler.push_back("TerrainGenerator: height map missing key (" + zahlText(kx) + ", " + zahlText(ky) + ") - using fallback\n");
                    return fallback;
                }
                return it->second;
            };

            double f_bl = safe_at(maps.floor_map, x, y, min_z);
            double f_tl = safe_at(maps.floor_map, x, my, min_z);
            double f_br = safe_at(maps.floor_map, mx, y, min_z);
            double f_tr = safe_at(maps.floor_map, mx, my, min_z);
            double c_bl = safe_at(maps.ceiling_map, x, y, max_ceil_z);
            double c_tl = safe_at(maps.ceiling_map, x, my, max_ceil_z);
            double c_br = safe_at(maps.ceiling_map, mx, y, max_ceil_z);
            double c_tr = safe_at(maps.ceiling_map, mx, my, max_ceil_z);

            bool alt_dir = ((x_index + y_index) % 2) != 0;

            insert_floor_ceil_brush(bau, floor_entity, x, y, mx, my, f_bl, f_tl, f_br, f_tr, min_z, max_ceil_z, top_texture, caulk, false, alt_dir);
            insert_floor_ceil_brush(bau, ceil_entity, x, y, mx, my, c_bl, c_tl, c_br, c_tr, min_z, ceil_solid_top, top_texture, caulk, true, alt_dir);
        }
    }

    double wall_min_z = target.max_z + std::min(0.0, slope_height);
    double wall_max_z = wall_min_z + cave_height + std::abs(slope_height);
    double step_z = maps.step_z;

    const bool along_y = (axis == Axis::Y);
    const double along_min = along_y ? target.min_y : target.min_x;
    const double along_max = along_y ? target.max_y : target.max_x;
    const double along_step = along_y ? step_y : step_x;
    const double outer_lo = along_y ? target.min_x - step_x : target.min_y - step_y;
    const double outer_hi = along_y ? target.max_x + step_x : target.max_y + step_y;
    const double limit = along_y ? (target.min_x + target.max_x) / 2.0 : (target.min_y + target.max_y) / 2.0;

    int gu_index = 0;
    for (double gu = along_min; gu < along_max - 0.01; gu += along_step, ++gu_index) {
        int gz_index = 0;
        for (double gz = wall_min_z; gz < wall_max_z - 0.01; gz += step_z, ++gz_index) {
            double gmx_u = gu + along_step < along_max ? gu + along_step : along_max;
            double gmx_z = gz + step_z < wall_max_z ? gz + step_z : wall_max_z;

            auto safe_wall = [&](const HeightMap& m, double ku, double kz, double fallback) -> double {
                auto it = m.find({r2(ku), r2(kz)});
                if (it == m.end()) {
                    bau.fehler.push_back("TerrainGenerator: wall map missing key (" + zahlText(ku) + ", " + zahlText(kz) + ") - using fallback\n");
                    return fallback;
                }
                return it->second;
            };

            double lx_bl = safe_wall(maps.left_wall_map, gu, gz, limit);
            double lx_tl = safe_wall(maps.left_wall_map, gu, gmx_z, limit);
            double lx_br = safe_wall(maps.left_wall_map, gmx_u, gz, limit);
            double lx_tr = safe_wall(maps.left_wall_map, gmx_u, gmx_z, limit);

            double rx_bl = safe_wall(maps.right_wall_map, gu, gz, limit);
            double rx_tl = safe_wall(maps.right_wall_map, gu, gmx_z, limit);
            double rx_br = safe_wall(maps.right_wall_map, gmx_u, gz, limit);
            double rx_tr = safe_wall(maps.right_wall_map, gmx_u, gmx_z, limit);

            bool alt_dir = ((gu_index + gz_index) % 2) != 0;

            insert_wall_brush(bau, lwall_entity, lx_bl, lx_tl, lx_br, lx_tr, gu, gmx_u, gz, gmx_z, outer_lo, limit, top_texture, caulk, true, alt_dir, axis);
            insert_wall_brush(bau, rwall_entity, rx_bl, rx_tl, rx_br, rx_tr, gu, gmx_u, gz, gmx_z, outer_hi, limit, top_texture, caulk, false, alt_dir, axis);
        }
    }

    select_generated(bau, floor_entity);
    select_generated(bau, ceil_entity);
    select_generated(bau, lwall_entity);
    select_generated(bau, rwall_entity);
}

// ---------------------------------------------------------------------------
// terrain_generator.cpp: dispatch "Generate Terrain"
// ---------------------------------------------------------------------------
namespace {

// instance_entity: die Entitaet eines gewaehlten Knotens (der Knoten selbst,
// wenn er eine Entitaet ist, sonst sein Elternteil)
std::shared_ptr<Entitaet> instanzEntitaet(const AuswahlElement& a) { return a.entitaet.lock(); }

// std::ostream << double wie globalOutputStream (6 signifikante Stellen)
std::string g6(double v) { return zahlText(v); }

}  // namespace

AuswahlGrenzen auswahlGrenzen(const Karte& k, double minGroesse) {
    const Kasten b = auswahlGrenzenPivot(k);   // GlobalSelectionSystem().getBoundsSelected()
    AuswahlGrenzen s;
    s.x0 = b.origin[0] - b.extents[0];
    s.x1 = b.origin[0] + b.extents[0];
    s.y0 = b.origin[1] - b.extents[1];
    s.y1 = b.origin[1] + b.extents[1];
    s.z0 = b.origin[2] - b.extents[2];
    s.z1 = b.origin[2] + b.extents[2];
    s.valid = (s.x1 - s.x0 >= minGroesse && s.y1 - s.y0 >= minGroesse);
    return s;
}

bool gespeicherteGroesse(const Karte& k, double& w, double& l, double& h) {
    for (const AuswahlElement& a : k.auswahl.elemente()) {   // foreachSelected, erster Treffer
        const auto e = instanzEntitaet(a);
        if (!e) { continue; }
        const std::string v = e->wert(kGroessenSchluessel);
        if (!v.empty() && std::sscanf(v.c_str(), "%lf %lf %lf", &w, &l, &h) == 3 && w >= 1.0 && l >= 1.0) { return true; }
    }
    return false;
}

bool schraegeAusAuswahl(const Karte& k, const Werte& w, double& hoehe) {
    const ShapeType st = static_cast<ShapeType>(w.form);
    if (!(w.ziel == 0 && (st == ShapeType::Slope || st == ShapeType::SlopeTunnel))) { return false; }
    double sw = 0, sl = 0, sh = 0;
    if (gespeicherteGroesse(k, sw, sl, sh)) {
        hoehe = sh;
        return true;
    }
    const AuswahlGrenzen s = auswahlGrenzen(k);
    if (s.valid) {
        hoehe = s.z1 - s.z0;
        return true;
    }
    return false;
}

Ergebnis erzeugen(Karte& k, const Werte& w, const Zufall& zufall) {
    Ergebnis erg;
    if (w.ziel == 0) {
        if (k.auswahl.anzahl() == 0) {
            erg.fehler = {"Terrain Generator - No Selection",
                          "No brush is selected.\n\nPlease select a brush in the viewport first,\nor switch the Target to \"Manual Size\"."};
            return erg;
        }
        if (!auswahlGrenzen(k).valid) {
            erg.fehler = {"Terrain Generator - Brush Too Small",
                          "The selected brush is too small.\n\nThe terrain area must be at least 64 x 64 units.\nPlease select a larger brush,\nor switch the "
                          "Target to \"Manual Size\"."};
            return erg;
        }
    }
    // texture_edit->text().trimmed().isEmpty()
    std::string textur = w.textur;
    {
        const auto anf = textur.find_first_not_of(" \t\r\n\v\f");
        const auto ende = textur.find_last_not_of(" \t\r\n\v\f");
        if (anf == std::string::npos) {
            erg.fehler = {"Terrain Generator - No Texture",
                          "No texture specified.\n\nPlease enter a texture path or use the Pick button\nto select one from the texture browser."};
            return erg;
        }
        (void)ende;
    }

    // --- Read parameters ---
    const bool use_manual = (w.ziel == 1);
    const bool advanced = w.erweitert;
    const double step_x = advanced ? w.schrittX : kQuadrate[std::clamp(w.quadratIndex, 0, 7)];
    const double step_y = advanced ? w.schrittY : kQuadrate[std::clamp(w.quadratIndex, 0, 7)];
    const ShapeType shape = static_cast<ShapeType>(w.form);
    const Axis axis = static_cast<Axis>(w.achse);
    const NoiseType noise = static_cast<NoiseType>(w.rauschen);
    const bool jitter_grid = w.unregelmaessig;
    const double tun_height = w.tunnelHoehe;
    const double variance = w.varianz;
    const double frequency = w.frequenz;
    const double terrace = w.terrasse;
    const double grid_step = kRasterSchritte[std::clamp(w.rasterIndex, 0, 10)];
    const char* texture = textur.c_str();   // texture_edit->text() (ungekuerzt)

    BrushData target;
    AuswahlGrenzen used_bounds{};
    bool used_bounds_valid = false;

    if (use_manual) {
        target = make_manual_brush_data(w.manuellB, w.manuellL, w.manuellH);
    } else {
        double sw = 0, sl = 0, sh = 0;
        AuswahlGrenzen s{};
        if (gespeicherteGroesse(k, sw, sl, sh)) {
            const AuswahlGrenzen live = auswahlGrenzen(k);
            if (live.valid) {
                const double cx = (live.x0 + live.x1) * 0.5;
                const double cy = (live.y0 + live.y1) * 0.5;
                s.x0 = cx - sw * 0.5;
                s.x1 = cx + sw * 0.5;
                s.y0 = cy - sl * 0.5;
                s.y1 = cy + sl * 0.5;
                s.z0 = live.z0;
                s.z1 = live.z0 + sh;
                s.valid = true;
            }
        } else {
            s = auswahlGrenzen(k, step_x < step_y ? step_x : step_y);
        }
        if (s.valid) {
            target.min_x = s.x0;
            target.max_x = s.x1;
            target.min_y = s.y0;
            target.max_y = s.y1;
            target.min_z = s.z0;
            const bool is_slope = (shape == ShapeType::Slope || shape == ShapeType::SlopeTunnel);
            const double top_h = is_slope ? std::max(w.formHoehe, 64.0) : std::max(s.z1 - s.z0, 64.0);
            target.max_z = s.z0 + top_h;
            target.width_x = target.max_x - target.min_x;
            target.length_y = target.max_y - target.min_y;
            target.height_z = target.max_z - target.min_z;
            used_bounds = s;
            used_bounds_valid = true;
        } else {
            target = make_manual_brush_data(w.manuellB, w.manuellL, w.manuellH);
        }
    }

    const bool slope_from_sel = !use_manual && (shape == ShapeType::Slope || shape == ShapeType::SlopeTunnel);
    const double shape_height = slope_from_sel ? -(w.formHoehe - 64.0) : w.formHoehe;

    Befehl undo(k.undo, "terrainGenerator.generate");

    // --- Delete the selection brush now that its bounds are captured ---
    if (!use_manual) {
        while (k.auswahl.anzahl() > 0) {
            const AuswahlElement a = k.auswahl.elemente().back();   // ultimateSelected
            const auto ent = a.entitaet.lock();
            const auto prim = a.primitiv.lock();
            if (ent && prim) {
                // Path_deleteTop( path ): das Primitiv aus seiner Entitaet
                k.auswahl.abwaehlen(ent, prim);
                (void)ent->kindEntfernen(prim.get());
                // eine leer gewordene Gruppen-Entitaet mit entfernen (nie worldspawn)
                const std::string& c = ent->klassenname();
                if (c != "worldspawn" && ent->istBehaelter() && ent->kinder().empty()) { (void)k.entitaetEntfernen(ent.get()); }
            } else if (ent) {
                // die Entitaet selbst (Punktentitaet oder ganze Gruppe)
                k.auswahl.abwaehlen(ent);
                (void)k.entitaetEntfernen(ent.get());
            } else {
                k.auswahl.aufraeumen();
            }
            k.auswahlBereinigen();
        }
    }

    adjust_bounds_to_fit_grid(target, step_x, step_y);

    k.auswahl.leeren();   // GlobalSelectionSystem().setSelectedAll( false )

    const bool is_tunnel = (shape == ShapeType::Tunnel || shape == ShapeType::SlopeTunnel);

    erg.ausgabe.push_back(std::string("TerrainGenerator: generating ") + (is_tunnel ? "tunnel" : "terrain") + " - bounds (" + g6(target.width_x) + " x " +
                          g6(target.length_y) + " x " + g6(target.height_z) + "), step (" + g6(step_x) + " x " + g6(step_y) + ")" +
                          ", texture: " + texture + "\n");

    Bau bau;
    if (is_tunnel) {
        const double cave_height = (shape == ShapeType::SlopeTunnel) ? tun_height : shape_height;
        const double slope_height = (shape == ShapeType::SlopeTunnel) ? shape_height : 0;
        const double tunnel_terrace = (shape == ShapeType::SlopeTunnel) ? terrace : 0.0;
        auto maps = generate_tunnel_height_maps(target, step_x, step_y, cave_height, slope_height, variance, frequency, noise, tunnel_terrace, axis,
                                                grid_step, zufall);
        build_tunnel_brushes(target, step_x, step_y, maps, texture, cave_height, slope_height, axis, bau);
    } else {
        bool split_diagonally = (variance > 0 || shape != ShapeType::Flat || jitter_grid);
        auto height_map = generate_height_map(target, step_x, step_y, shape, shape_height, variance, frequency, noise, terrace, axis, jitter_grid,
                                              grid_step, zufall);
        build_terrain_brushes(target, step_x, step_y, height_map, texture, split_diagonally, bau);
    }

    // Die Gruppen in die Karte: create_func_group fuegt die Entitaet sofort in
    // die Wurzel ein, die Pinsel kommen danach dazu (Brush_addFace mit contents).
    std::vector<std::shared_ptr<Entitaet>> gruppen;
    for (const auto& g : bau.gruppen) {
        auto ent = k.neueEntitaet("func_group", true);
        k.entitaetEinfuegen(ent);
        for (const auto& pinsel : g) {
            auto p = std::make_shared<Pinsel>();
            for (const FaceData& f : pinsel) {
                TexturProjektion proj;   // TextureProjection( m_texdef, brushprimit_texdef_t(), 0, 0 )
                proj.texdef.shift[0] = f.shift[0];
                proj.texdef.shift[1] = f.shift[1];
                proj.texdef.rotate = f.rotate;
                proj.texdef.scale[0] = f.scale[0];
                proj.texdef.scale[1] = f.scale[1];
                proj.basisS = Vektor3f(0, 0, 0);
                proj.basisT = Vektor3f(0, 0, 0);
                (void)p->ebeneHinzufuegen(Vektor3d(f.p0[0], f.p0[1], f.p0[2]), Vektor3d(f.p1[0], f.p1[1], f.p1[2]), Vektor3d(f.p2[0], f.p2[1], f.p2[2]),
                                          f.shader, proj, k.kontext, f.contents);
            }
            p->schicht = static_cast<int>(k.aktuelleSchicht);
            ent->kindEinfuegen(p);
            ++erg.pinsel;
        }
        gruppen.push_back(ent);
    }
    erg.gruppen = static_cast<int>(gruppen.size());
    // select_generated: Entity_setSelected waehlt bei einer Gruppe ihre Pinsel
    for (const int gi : bau.gewaehlt) {
        const auto& ent = gruppen[static_cast<std::size_t>(gi)];
        for (const auto& kind : ent->kinder()) { k.auswahl.waehlen(ent, kind); }
    }
    erg.fehlerStrom = bau.fehler;
    erg.ausgabe.push_back("TerrainGenerator: generation complete\n");

    // write_stored_size: der Fussabdruck an der Entitaet jedes gewaehlten Knotens
    if (used_bounds_valid) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%g %g %g", used_bounds.x1 - used_bounds.x0, used_bounds.y1 - used_bounds.y0, used_bounds.z1 - used_bounds.z0);
        for (const AuswahlElement& a : k.auswahl.elemente()) {
            if (const auto e = instanzEntitaet(a)) { e->setzen(kGroessenSchluessel, buf); }
        }
    }
    erg.ok = true;
    return erg;
}

}  // namespace bhed::karte::terraingen
