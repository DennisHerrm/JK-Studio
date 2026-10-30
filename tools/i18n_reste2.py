# -*- coding: utf-8 -*-
"""Oberflaechentexte der Liste reste2 fuer tools/gen_i18n.py.

Punkt 6: die GPU-Tabelle im Statistikfenster der Karte (gui/app_view3d.cpp
drawKartenStatistik) stand in jeder Sprache deutsch.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
RESTE2 = [
    ("MapGpuCalls", "GPU: %d {draw call|draw calls}, %d skipped", "GPU: %d {Aufruf|Aufrufe}, %d übersprungen",
     "GPU:%d 次绘制调用,%d 次跳过", "GPU:描画呼び出し %d 回、スキップ %d 回"),
    ("MapGpuStatesNone", "States: none (as before rc434)", "Zustände: keine (wie vor rc434)", "状态:无(与 rc434 之前相同)",
     "ステート:なし(rc434 より前と同じ)"),
    ("MapGpuStatesDepth", "States: depth only", "Zustände: nur Tiefe", "状态:仅深度", "ステート:深度のみ"),
    ("MapGpuStatesCull", "States: culling only", "Zustände: nur Keulen", "状态:仅剔除", "ステート:カリングのみ"),
    ("MapGpuStatesBoth", "States: depth and culling", "Zustände: Tiefe und Keulen", "状态:深度和剔除", "ステート:深度とカリング"),
    ("MapGpuStatesNoD3d", "built without Direct3D", "ohne Direct3D gebaut", "未使用 Direct3D 构建", "Direct3D なしでビルド"),
    ("MapGpuMap", "Map", "Karte", "地图", "マップ"),
    ("MapGpuEffects", "Effects", "Effekte", "特效", "エフェクト"),
    ("MapGpuMovers", "Movers", "Mover", "移动体", "ムーバー"),
    ("MapGpuFigures", "Figures", "Figuren", "角色", "キャラクター"),
    ("MapGpuGlow", "Glow", "Glühen", "辉光", "グロー"),
    ("MapGpuRow", "%s: %d {call|calls}, %.2f ms", "%s: %d {Aufruf|Aufrufe}, %.2f ms", "%s:%d 次调用,%.2f 毫秒",
     "%s:%d 回、%.2f ミリ秒"),
    ("MapGpuRowNoTime", "%s: %d {call|calls}", "%s: %d {Aufruf|Aufrufe}", "%s:%d 次调用", "%s:%d 回"),
]
