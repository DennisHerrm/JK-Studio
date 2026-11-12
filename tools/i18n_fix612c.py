# -*- coding: utf-8 -*-
"""Oberflaechentexte von FIX612C (12.11.: Funde P3-5 bis P3-10 der Codepruefung seit rc611).

P3-5: "Animation-Rig (nur FK)" backt die IK der Schichten in FK-Schluessel (Meldung; den Tooltip AzRigFkHint ergaenzt i18n_ap10.py).
P3-6: "Knochen waehlen" wie 3ds Max als Umschalter mit Meldung und Anzeige in der Ansicht (wie "Abstand messen").
Begriffe wie die vorhandenen Texte (AP4 "Knochen waehlen"/"Select bone", AP10 "Animation-Rig (nur FK)", ENT2 IK/FK).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- P3-5: nur FK ---
    ("AzNurFkGebacken",
     "FK-only rig: the IK of %d limbs in %d sequences is now FK keys - the motion stays the same. Ctrl+Z brings the IK back.",
     "Rig nur mit FK: die IK von %d Gliedern in %d Sequenzen sind jetzt FK-Schlüssel - die Bewegung bleibt gleich. Strg+Z holt die IK zurück.",
     "仅 FK 绑定: %d 个肢体的 IK (共 %d 个序列) 已转为 FK 关键帧 - 动作保持不变。Ctrl+Z 可恢复 IK。",
     "FK のみのリグ: %d 本の手足の IK (%d 個のシーケンス) を FK キーにしました - 動きは変わりません。Ctrl+Z で IK に戻せます。",
     "Rig solo FK: el IK de %d extremidades en %d secuencias ahora son claves FK; el movimiento no cambia. Ctrl+Z recupera el IK.",
     "Rig jen s FK: IK %d končetin v %d sekvencích jsou teď klíče FK - pohyb zůstává stejný. Ctrl+Z vrátí IK."),
    # --- P3-6: Knochen waehlen wie 3ds Max ---
    ("BzKnochenAn", "Select bone is on: a click near a joint selects its bone. Q or right-click > Select bone ends it.",
     "Knochen wählen ist an: ein Klick nahe an einem Gelenk wählt seinen Knochen. Q oder Rechtsklick > Knochen wählen beendet es.",
     "选择骨骼已开启: 在关节附近单击即可选择其骨骼。按 Q 或右键 > 选择骨骼可结束。",
     "ボーンを選択がオンです: 関節の近くをクリックするとそのボーンを選択します。Q または右クリック > ボーンを選択で終了します。",
     "Seleccionar hueso está activo: un clic cerca de una articulación selecciona su hueso. Q o clic derecho > Seleccionar hueso lo termina.",
     "Vybrat kost je zapnuto: klik poblíž kloubu vybere jeho kost. Q nebo pravé tlačítko > Vybrat kost to ukončí."),
    ("BzKnochenAus", "Select bone is off: a click selects surfaces again.", "Knochen wählen ist aus: ein Klick wählt wieder Flächen.",
     "选择骨骼已关闭: 单击再次选择表面。", "ボーンを選択がオフです: クリックで再びサーフェスを選択します。",
     "Seleccionar hueso está desactivado: un clic vuelve a seleccionar superficies.", "Vybrat kost je vypnuto: klik opět vybírá plochy."),
    ("BzKnochenAnzeige", "Select bone - Q ends", "Knochen wählen - Q beendet", "选择骨骼 - Q 结束", "ボーンを選択 - Q で終了",
     "Seleccionar hueso - Q termina", "Vybrat kost - Q ukončí"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
