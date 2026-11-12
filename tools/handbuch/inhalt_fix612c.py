"""FIX612C (12.11., Codepruefung seit rc611 P3-5/P3-6): zwei Absaetze fuer die Seiten des Model-Tabs.

- `bloecke_animationsfeld()`: hinter den Absatz "rig" der Seite modell-animationsfeld (inhalt_ap10.py) - "Animation-Rig (nur FK)"
  backt die IK in FK-Schluessel, Rueckgaengig holt die IK zurueck, "Control Rig erstellen" danach.
- `bloecke_aufbau()`: hinter den Absatz ueber das Werkzeug Messen der Seite "Model-Tab: der Aufbau wie Blender" (inhalt_ap4.py) -
  "Knochen waehlen" wie 3ds Max als Umschalter.
Eigene Quelle wie die Pakete AP4/AP10; wer deren Seiten ins Handbuch haengt (AP14), haengt diese Bloecke mit an. Jeder Text einmal
in allen sechs Sprachen (en, de, zh, ja, es, cs); {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "nurfk": (
        "**{{AzRigFk}}** after **{{AzRigErstellen}}**: limbs that are on IK in a sequence are first baked into FK keys at every frame (in every sequence of the "
        "figure), then the IK tracks are removed - the motion stays exactly the same and every rotation control turns its bone. Ctrl+Z brings the IK tracks, "
        "the IK targets and poles back in one step. **{{AzRigErstellen}}** turns the IK targets on again; an existing animation stays on FK until you switch a "
        "limb to IK in **{{MbIkRollout}}** or use **{{AzAufRig}}**.",
        "**{{AzRigFk}}** nach **{{AzRigErstellen}}**: Glieder, die in einer Sequenz auf IK stehen, werden zuerst an jedem Bild in FK-Schlüssel gebacken (in jeder "
        "Sequenz der Figur), danach fallen die IK-Spuren weg - die Bewegung bleibt genau gleich, und jeder Dreh-Griff dreht seinen Knochen. Strg+Z holt IK-Spuren, "
        "IK-Ziele und Pole in einem Schritt zurück. **{{AzRigErstellen}}** schaltet die IK-Ziele wieder ein; eine vorhandene Animation bleibt auf FK, bis du ein "
        "Glied in **{{MbIkRollout}}** auf IK stellst oder **{{AzAufRig}}** nimmst.",
        "在 **{{AzRigErstellen}}** 之后使用 **{{AzRigFk}}**: 序列中处于 IK 的肢体会先在每一帧烘焙为 FK 关键帧 (角色的每个序列都会处理), 然后移除 IK 轨道 - 动作完全不变, "
        "每个旋转控制器都能旋转其骨骼。Ctrl+Z 一步恢复 IK 轨道、IK 目标和极向。**{{AzRigErstellen}}** 会重新打开 IK 目标; 现有动画保持 FK, 直到你在 **{{MbIkRollout}}** "
        "中把某个肢体切换为 IK 或使用 **{{AzAufRig}}**。",
        "**{{AzRigErstellen}}** の後の **{{AzRigFk}}**: シーケンスで IK になっている手足は、まず各フレームで FK キーにベイクされ (フィギュアのすべてのシーケンス)、その後 IK "
        "トラックが削除されます。動きはまったく変わらず、各回転コントロールがボーンを回します。Ctrl+Z で IK トラック、IK ターゲット、ポールが 1 ステップで戻ります。"
        "**{{AzRigErstellen}}** で IK ターゲットが再び有効になります。既存のアニメーションは、**{{MbIkRollout}}** で手足を IK に切り替えるか **{{AzAufRig}}** を使うまで FK のままです。",
        "**{{AzRigFk}}** después de **{{AzRigErstellen}}**: las extremidades que están en IK en una secuencia se hornean primero en claves FK en cada fotograma (en "
        "todas las secuencias de la figura) y después se quitan las pistas IK; el movimiento queda exactamente igual y cada control de giro gira su hueso. Ctrl+Z "
        "recupera en un paso las pistas IK, los objetivos IK y los polos. **{{AzRigErstellen}}** vuelve a activar los objetivos IK; una animación existente sigue "
        "en FK hasta que pases una extremidad a IK en **{{MbIkRollout}}** o uses **{{AzAufRig}}**.",
        "**{{AzRigFk}}** po **{{AzRigErstellen}}**: končetiny, které jsou v sekvenci na IK, se nejdřív v každém snímku zapečou do klíčů FK (ve všech sekvencích "
        "postavy), potom stopy IK zmizí - pohyb zůstane přesně stejný a každý otočný ovladač otáčí svou kostí. Ctrl+Z vrátí jedním krokem stopy IK, cíle IK a "
        "póly. **{{AzRigErstellen}}** cíle IK znovu zapne; existující animace zůstane na FK, dokud končetinu nepřepneš v **{{MbIkRollout}}** na IK nebo nepoužiješ "
        "**{{AzAufRig}}**."),
    "knochen": (
        "Like 3ds Max, **{{BzKnochenWaehlen}}** is a switch like **{{BzMessenMax}}**: Search all actions and the context menu of the view turn it on and off "
        "(with a check mark in the menu), and while it is on, the top of the view says so. Q, W, E and R end it, and so does changing from like Blender to "
        "like 3ds Max.",
        "Wie 3ds Max ist **{{BzKnochenWaehlen}}** ein Schalter wie **{{BzMessenMax}}**: Alle Aktionen suchen und der Rechtsklick der Ansicht schalten es an und "
        "aus (im Menü mit Haken), und solange es an ist, steht es oben in der Ansicht. Q, W, E und R beenden es, ebenso der Wechsel von wie Blender zu wie 3ds Max.",
        "像 3ds Max 时, **{{BzKnochenWaehlen}}** 与 **{{BzMessenMax}}** 一样是开关: “搜索所有操作”和视图的右键菜单可开关它 (菜单中带勾选标记), 开启期间视图顶部会显示提示。"
        "Q、W、E、R 会结束它, 从像 Blender 切换到像 3ds Max 时也会结束。",
        "3ds Max 風では **{{BzKnochenWaehlen}}** は **{{BzMessenMax}}** と同じくスイッチです: 「すべてのアクションを検索」とビューの右クリックメニューでオン/オフでき (メニューに"
        "チェックマーク)、オンの間はビューの上部に表示されます。Q、W、E、R で終了し、Blender 風から 3ds Max 風に切り替えたときも終了します。",
        "Como 3ds Max, **{{BzKnochenWaehlen}}** es un interruptor como **{{BzMessenMax}}**: Buscar todas las acciones y el menú contextual de la vista lo activan y "
        "desactivan (con una marca en el menú), y mientras está activo lo indica la parte superior de la vista. Q, W, E y R lo terminan, igual que cambiar de "
        "como Blender a como 3ds Max.",
        "Jako 3ds Max je **{{BzKnochenWaehlen}}** přepínač jako **{{BzMessenMax}}**: Vyhledat všechny akce a místní nabídka pohledu ho zapínají a vypínají (v "
        "nabídce se zaškrtnutím) a dokud je zapnutý, stojí to nahoře v pohledu. Q, W, E a R ho ukončí, stejně jako přepnutí z jako Blender na jako 3ds Max."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def bloecke_animationsfeld():
    """Hinter ("p", _t("rig")) der Seite modell-animationsfeld."""
    return [("p", _t("nurfk"))]


def bloecke_aufbau():
    """Hinter den Absatz ueber das Werkzeug Messen der Seite des Aufbaus wie Blender (inhalt_ap4.py)."""
    return [("p", _t("knochen"))]
