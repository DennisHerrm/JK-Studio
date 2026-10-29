# -*- coding: utf-8 -*-
"""Oberflaechentexte des Menue-Editors, Stufe M3, fuer tools/gen_i18n.py."""

# (Kennung, English, Deutsch, 中文, 日本語)
MENUE3 = [
    # Punkt 7: eigene Texte in den asiatischen Kodierungen der Engine
    ("MeTextNichtDarstellbar", "%s: the game cannot show these characters, stored as ?: %s",
     "%s: Diese Zeichen kann das Spiel nicht darstellen, gespeichert als ?: %s",
     "%s:游戏无法显示这些字符,已存为 ?:%s", "%s: ゲームはこれらの文字を表示できません。? として保存しました: %s"),
    # Punkt 5: HUD-Editor
    ("MeHudWerte", "HUD values (as cgame reads them)", "HUD-Werte (wie cgame sie liest)", "HUD 数值(cgame 读取的值)", "HUD の値(cgame が読む値)"),
    ("MeHudWie", "Draw like the game (CG_DrawHUD)", "Wie im Spiel zeichnen (CG_DrawHUD)", "像游戏中一样绘制(CG_DrawHUD)", "ゲームと同じように描く(CG_DrawHUD)"),
    ("MeHudGesundheit", "Health", "Gesundheit", "生命值", "体力"),
    ("MeHudGesundheitMax", "Max health", "Gesundheit max.", "最大生命值", "最大体力"),
    ("MeHudRuestung", "Armor", "Rüstung", "护甲", "アーマー"),
    ("MeHudMacht", "Force", "Macht", "原力", "フォース"),
    ("MeHudMachtMax", "Max force", "Macht max.", "最大原力", "最大フォース"),
    ("MeHudMachtBekannt", "Knows Force powers", "Kennt Machtkräfte", "掌握原力能力", "フォースパワーを習得"),
    ("MeHudSaebel", "Lightsaber in hand", "Lichtschwert in der Hand", "手持光剑", "ライトセーバーを装備"),
    ("MeHudSaebelStil", "Saber style", "Säbelstil", "光剑风格", "セーバースタイル"),
    ("MeHudMunition", "Ammo", "Munition", "弹药", "弾薬"),
    ("MeHudMunitionMax", "Max ammo", "Munition max.", "最大弹药", "最大弾薬"),
    ("MeHudFadenkreuz", "Crosshair (cg_drawCrosshair)", "Fadenkreuz (cg_drawCrosshair)", "准星(cg_drawCrosshair)", "照準(cg_drawCrosshair)"),
    ("MeHudFadenkreuzGroesse", "Crosshair size", "Fadenkreuzgröße", "准星大小", "照準の大きさ"),
    ("MeHudNichtNachgebildet", "This game's cgame draws its HUD differently (multiplayer, Movie Duels) - only the menus are shown.",
     "Das cgame dieses Spiels zeichnet sein HUD anders (Mehrspieler, Movie Duels) - gezeigt werden nur die Menüs.",
     "该游戏的 cgame 以不同方式绘制 HUD(多人模式、Movie Duels)——仅显示菜单。",
     "このゲームの cgame は HUD を別の方法で描きます(マルチプレイヤー、Movie Duels)。メニューだけを表示します。"),
]
