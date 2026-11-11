# Erzeugt src/modell/rig_vorlagen.inc: Rig-Vorlagen fuer das Retargeting (Modell-Block Stufe 5).
#   von Hand: JKA _humanoid (53), JK2 _humanoid (72), HumanIK/Mixamo, Biped, CAT (Base Human)
#   erzeugt:  die uebrigen eigenen Skelette des Basisspiels aus rig_vorlagen/vorschlag_*.txt (Ausgabe von modellretargettest
#             --vorschlag), von Hand durchgesehen (Korrekturen unten in KORREKTUR); rig_vorlagen/gla/uebersicht.json: Name, Massstab,
#             Bilder und Knochen jeder GLA der Spieldaten (JKA, JK2, Movie Duels)
# Aufruf (im Repo): python tools/modell/rig_vorlagen_gen.py   - schreibt src/modell/rig_vorlagen.inc
import os, sys, json, struct

HIER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "rig_vorlagen")
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def humanoid(jk2):
    e = [("model_root", "W", "M", 0, 0), ("pelvis", "P", "M", 0, 0), ("Motion", "M", "M", 0, 0),
         ("lower_lumbar", "S", "M", 0, 0), ("upper_lumbar", "S", "M", 0, 1), ("thoracic", "S", "M", 0, 2),
         ("cervical", "H", "M", 0, 0), ("cranium", "K", "M", 0, 0)]
    for s, S in (("l", "L"), ("r", "R")):
        e += [(s + "femurYZ", "B", S, 0, 0), (s + "tibia", "B", S, 0, 1), (s + "talus", "B", S, 0, 2), (s + "femurX", "t", S, 0, 0),
              (s + "clavical", "C", S, 0, 0), (s + "humerus", "A", S, 0, 0), (s + "radius", "A", S, 0, 1), (s + "hand", "A", S, 0, 2),
              (s + "humerusX", "t", S, 0, 0), (s + "radiusX", "t", S, 0, 0)]
        if not jk2:
            e.append((s + "tail", "T", S, 0, 0))   # Rockschoesse nur in JKA
        if jk2:
            e.append((s + "tarsal", "B", S, 0, 3))
            for d in range(1, 6):
                for j in range(1, 4):
                    e.append(("%s_d%d_j%d" % (s, d, j), "F", S, d, j - 1))
        else:
            for d in (1, 2, 4):
                for j in (1, 2):
                    e.append(("%s_d%d_j%d" % (s, d, j), "F", S, d, j - 1))
    return e

# HumanIK-Namen (MotionBuilder SDK, tasks/3ds Max Biped Template.py, bipedMap) - Mixamo benutzt sie mit "mixamorig:" davor
def humanik():
    e = [("Hips", "P", "M", 0, 0), ("Spine", "S", "M", 0, 0), ("Spine1", "S", "M", 0, 1), ("Spine2", "S", "M", 0, 2),
         ("Neck", "H", "M", 0, 0), ("Head", "K", "M", 0, 0)]
    for s, S in (("Left", "L"), ("Right", "R")):
        e += [(s + "UpLeg", "B", S, 0, 0), (s + "Leg", "B", S, 0, 1), (s + "Foot", "B", S, 0, 2), (s + "ToeBase", "B", S, 0, 3),
              (s + "Shoulder", "C", S, 0, 0), (s + "Arm", "A", S, 0, 0), (s + "ForeArm", "A", S, 0, 1), (s + "Hand", "A", S, 0, 2),
              (s + "UpLegRoll", "t", S, 0, 0), (s + "ArmRoll", "t", S, 0, 0), (s + "ForeArmRoll", "t", S, 0, 0)]
        for n, f in enumerate(("Thumb", "Index", "Middle", "Ring", "Pinky")):
            for j in (1, 2, 3):
                e.append(("%sHand%s%d" % (s, f, j), "F", S, n + 1, j - 1))
    return e

# 3ds Max Biped: "<Biped> <Teil>" (bipedMap wie oben; Endung mit Leerzeichen davor)
def biped():
    e = [(" Pelvis", "P", "M", 0, 0), (" Spine", "S", "M", 0, 0), (" Spine1", "S", "M", 0, 1), (" Spine2", "S", "M", 0, 2),
         (" Neck", "H", "M", 0, 0), (" Head", "K", "M", 0, 0)]
    for S in ("L", "R"):
        e += [(" %s Thigh" % S, "B", S, 0, 0), (" %s Calf" % S, "B", S, 0, 1), (" %s Foot" % S, "B", S, 0, 2), (" %s Toe0" % S, "B", S, 0, 3),
              (" %s Clavicle" % S, "C", S, 0, 0), (" %s UpperArm" % S, "A", S, 0, 0), (" %s Forearm" % S, "A", S, 0, 1),
              (" %s Hand" % S, "A", S, 0, 2), (" %s ForeTwist" % S, "t", S, 0, 0), ("%sUpArmTwist" % S, "t", S, 0, 0),
              ("%sThighTwist" % S, "t", S, 0, 0)]
        for n in range(5):
            for j, suf in enumerate(("", "1", "2")):
                e.append((" %s Finger%d%s" % (S, n, suf), "F", S, n + 1, j))
    return e

# CAT "Base Human" (3ds Max 2016 plugcfg_ln/CAT/CATRigs/Base Human.rig): CharacterName + Glied + Knochen
def cat():
    e = [("Pelvis", "P", "M", 0, 0), ("Spine1", "S", "M", 0, 0), ("Spine2", "S", "M", 0, 1), ("Spine3", "S", "M", 0, 2),
         ("Spine4", "S", "M", 0, 3), ("Ribcage", "S", "M", 0, 4), ("Neck1", "H", "M", 0, 0), ("Neck2", "H", "M", 0, 1), ("Head", "K", "M", 0, 0)]
    for S in ("L", "R"):
        e += [(S + "Thigh", "B", S, 0, 0), (S + "Calf", "B", S, 0, 1), (S + "Foot", "B", S, 0, 2), (S + "Collarbone", "C", S, 0, 0),
              (S + "Upperarm", "A", S, 0, 0), (S + "Forearm", "A", S, 0, 1), (S + "Palm", "A", S, 0, 2)]
    return e

def zeilen(name, datei, knochen, massstab, endung, eintraege, kenn):
    o = ["inline const Eintrag kV_%s[] = {" % kenn]
    for n, a, s, nr, st in eintraege:
        o.append('    {"%s", \'%s\', \'%s\', %d, %d},' % (n, a, s, nr, st))
    o.append("};")
    return o, '    {"%s", "%s", %d, %.4fF, %s, kV_%s, static_cast<int>(sizeof kV_%s / sizeof kV_%s[0])},' % (
        name, datei, knochen, massstab, "true" if endung else "false", kenn, kenn, kenn)

uebersicht = json.load(open(os.path.join(HIER, "gla", "uebersicht.json")))
def info(spiel, datei):
    for x in uebersicht[spiel]:
        if x["datei"] == datei:
            return x
    return None

teile = []
liste = []
j = info("jka", "models/players/_humanoid/_humanoid.gla")
k = info("jk2", "models/players/_humanoid/_humanoid.gla")
for args in (("_humanoid (JKA)", j["datei"], j["knochen"], j["scale"], False, humanoid(False), "jka"),
             ("_humanoid (JK2)", k["datei"], k["knochen"], k["scale"], False, humanoid(True), "jk2"),
             ("HumanIK / Mixamo", "", 0, 0.0, True, humanik(), "humanik"),
             ("3ds Max Biped", "", 0, 0.0, True, biped(), "biped"),
             ("3ds Max CAT (Base Human)", "", 0, 0.0, True, cat(), "cat")):
    a, b = zeilen(*args)
    teile += a
    liste.append(b)

# Durchsicht (07.11., Hierarchie und Orte mit skel.py gelesen): "weg" Knochen aus den Ketten nehmen (folgen dann ihrem Fuss /
# Ort), "setze" Eintraege ersetzen oder neu
KORREKTUR = {
    # Fuesse und Zehen unter grandmaster_null: das Bein ist Oberschenkel, Unterschenkel, Fuss, vordere Zehe; Ferse und die
    # anderen Zehen folgen dem Fuss; der Schwanz ist eine Kette (mid_ kein Finger), Fettlappen eigene Anhaenge
    "rancor": {"weg": ["L_heel_bone", "L_inside_toe", "L_outside_toe", "R_heel_bone", "R_inside_toe", "R_outside_toe"],
               "setze": [("R_foot_bone", "B", "R", 0, 2), ("R_front_toe", "B", "R", 0, 3), ("L_foot_bone", "B", "L", 0, 2),
                         ("L_front_toe", "B", "L", 0, 3), ("base_tail", "T", "M", 0, 0), ("mid_tail", "T", "M", 0, 1),
                         ("end_tail", "T", "M", 0, 2), ("back_fat", "T", "M", 1, 0), ("neck_fat", "T", "M", 2, 0)]},
    # L_foot_bone liegt genau auf ltalus (beide unter model_root): das Bein endet am Knoechel, L_foot_bone folgt ihm
    "wampa": {"weg": ["L_foot_bone", "R_foot_bone"], "setze": [("ltalus", "B", "L", 0, 2), ("rtalus", "B", "R", 0, 2)]},
    # kein Becken im Namen: lower_spine traegt den Oberkoerper (Beine unter model_root), die Fersen folgen dem Fuss
    "tauntaun": {"weg": ["L_heel_bone", "R_heel_bone"], "setze": [("lower_spine", "P", "M", 0, 0), ("upper_spine", "S", "M", 0, 0)]},
    "howler": {"weg": ["L_Heel", "R_Heel"],
               "setze": [("Spine_01", "P", "M", 0, 0), ("Spine_02", "S", "M", 0, 0), ("Spine_03", "S", "M", 0, 1), ("Spine_04", "S", "M", 0, 2),
                         ("L_Hand", "A", "L", 0, 2), ("R_Hand", "A", "R", 0, 2)]},
    "protocol": {"weg": ["mc5", "mc7"]},
    "assassin_droid": {"weg": ["Left_heel_bone", "Right_heel_bone"]},
    "rockettrooper": {"weg": ["left_foot_bone", "Right_foot_bone"]},
    "saber_droid": {"weg": ["Left_heel_bone", "Right_heel_bone"], "setze": [("ltalus", "B", "L", 0, 2), ("rtalus", "B", "R", 0, 2)]},
}
def korrigiert(name, datei, eintr):
    k = KORREKTUR.get(name, {})
    weg = set(k.get("weg", []))
    setze = k.get("setze", [])
    neu = {x[0] for x in setze}
    aus = [e for e in eintr if e[0] not in weg and e[0] not in neu]
    aus += setze
    return aus

# erzeugte Vorlagen der uebrigen Skelette (modellretargettest --vorschlag), durchgesehen
for d in sorted(os.listdir(HIER)):
    if not (d.startswith("vorschlag_") and d.endswith(".txt")):
        continue
    zeilen_ = [l.rstrip("\n") for l in open(os.path.join(HIER, d), encoding="utf-8") if l.strip()]
    kopf = zeilen_[0].split("\t")   # datei, knochen, massstab, name
    eintr = []
    for l in zeilen_[1:]:
        n, a, s, nr, st = l.split("\t")
        eintr.append((n, a, s, int(nr), int(st)))
    eintr = korrigiert(kopf[3], kopf[0], eintr)
    kenn = "".join(c if c.isalnum() else "_" for c in kopf[3])
    a, b = zeilen(kopf[3], kopf[0], int(kopf[1]), float(kopf[2]), False, eintr, "g_" + kenn)
    teile += a
    liste.append(b)

text = ["// rig_vorlagen.inc - erzeugt von tools/modell/rig_vorlagen_gen.py (Modell-Block Stufe 5): Rig-Vorlagen fuer das Retargeting.",
        "// JKA/JK2 _humanoid von Hand; HumanIK/Mixamo und Biped nach MotionBuilder SDK (tasks/3ds Max Biped Template.py, bipedMap);",
        "// CAT nach 3ds Max 2016 CATRigs/Base Human.rig; die eigenen Skelette des Basisspiels (Jedi Academy, GameData/base) aus dem",
        "// Kettenvorschlag (rtVorschlag), von Hand durchgesehen. Eingebunden in retarget.cpp.",
        "// art: P Becken, W Wurzel, M Bewegung, S Wirbel, H Hals, K Kopf, C Schluesselbein, A Arm, B Bein, F Finger, T Anhang, t Drehknochen"]
text += teile
text.append("inline const Vorlage kVorlagen[] = {")
text += liste
text.append("};")
open(os.path.join(REPO, "src", "modell", "rig_vorlagen.inc"), "w", encoding="utf-8", newline="\n").write("\n".join(text) + "\n")
print(len(liste), "Vorlagen")
