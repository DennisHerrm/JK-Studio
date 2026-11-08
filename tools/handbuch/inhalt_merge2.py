"""MERGE2: Hinweis auf der Seite "Die Karte" zu den Detailstufen der Figuren in der Vorschau (G2_ComputeLOD wie im Spiel).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mr4.py.
"""
from inhalt_basis import T

LOD = (
    "Figures are drawn with the detail level the game picks for them (G2_ComputeLOD): the farther away and the smaller "
    "(`scale` in the .npc), the coarser the mesh - a figure with scale 70 at 500 units already uses the second level, as in Movie "
    "Duels. The radius is the one of the game (the bounding box of the .npc, 40 by default), `r_lodscale` 10.",
    "Figuren werden mit der Detailstufe gezeichnet, die das Spiel für sie wählt (G2_ComputeLOD): je weiter weg und je kleiner "
    "(`scale` in der .npc), desto gröber das Netz - eine Figur mit scale 70 in 500 Einheiten nimmt schon die zweite Stufe, wie in "
    "Movie Duels. Der Radius ist der des Spiels (der Hüllkasten der .npc, sonst 40), `r_lodscale` 10.",
    "角色以游戏为其选择的细节级别绘制 (G2_ComputeLOD):越远、越小(.npc 中的 `scale`),网格越粗糙——scale 为 70 的角色在 500 "
    "单位处就已使用第二级,与 Movie Duels 相同。半径与游戏相同(.npc 的包围盒,默认 40),`r_lodscale` 为 10。",
    "フィギュアはゲームが選ぶ詳細レベルで描画されます (G2_ComputeLOD)。遠いほど、また小さいほど (.npc の `scale`) メッシュは粗くなり、"
    "scale 70 のフィギュアは 500 ユニットの距離で既に 2 番目のレベルを使います (Movie Duels と同じ)。半径はゲームと同じ (.npc の"
    "バウンディングボックス、既定 40)、`r_lodscale` は 10 です。",
    "Las figuras se dibujan con el nivel de detalle que el juego elige para ellas (G2_ComputeLOD): cuanto más lejos y más pequeñas "
    "(`scale` en el .npc), más tosca la malla; una figura con scale 70 a 500 unidades ya usa el segundo nivel, como en Movie Duels. "
    "El radio es el del juego (la caja envolvente del .npc, 40 por defecto), `r_lodscale` 10.",
    "Postavy se kreslí s úrovní detailu, kterou pro ně volí hra (G2_ComputeLOD): čím dál a čím menší (`scale` v .npc), tím hrubší "
    "síť - postava se scale 70 ve vzdálenosti 500 jednotek už používá druhou úroveň, jako v Movie Duels. Poloměr je stejný jako ve "
    "hře (obalový kvádr z .npc, jinak 40), `r_lodscale` 10.",
)


def lod_notiz():
    return ("note", T(*LOD[:4]))


ES = {LOD[0]: LOD[4]}
CS = {LOD[0]: LOD[5]}
