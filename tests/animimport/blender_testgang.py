# Blender (--background --factory-startup): eigene Testszene fuer den Animations-Import von JK Studio.
# Skelett mit HumanIK/Mixamo-Namen (mixamorig:...), Figur schaut nach -Y (Blender-Vorderansicht), ein Gang auf der Stelle
# mit Fuss-IK (Standfuss gleitet gleichmaessig nach hinten, wie die Gaenge des Spiels), gebacken; Ausgabe FBX, glTF (glb),
# BVH, COLLADA (falls vorhanden) und je Bild die Weltorte der Knochenkoepfe (Blender-Koordinaten, Meter) als JSON.
#   blender -b --factory-startup --python blender_testgang.py -- <ausgabeordner>
import json
import math
import os
import sys

import bpy
from mathutils import Vector

aus = sys.argv[sys.argv.index("--") + 1]
os.makedirs(aus, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.fps = 30
sc.frame_start = 1
sc.frame_end = 31   # 30 Bilder Zyklus + Naht

P = "mixamorig:"
# (Name, Eltern, Kopf, Schwanz) in Metern, Blender: +Z oben, Figur schaut nach -Y, links = +X
K = [
    ("Hips", None, (0, 0, 1.00), (0, 0, 1.10)),
    ("Spine", "Hips", (0, 0, 1.10), (0, 0, 1.25)),
    ("Spine1", "Spine", (0, 0, 1.25), (0, 0, 1.38)),
    ("Spine2", "Spine1", (0, 0, 1.38), (0, 0, 1.50)),
    ("Neck", "Spine2", (0, 0, 1.50), (0, 0, 1.60)),
    ("Head", "Neck", (0, 0, 1.60), (0, 0, 1.78)),
]
for s, x in (("Left", 1), ("Right", -1)):
    K += [
        (s + "Shoulder", "Spine2", (0.04 * x, 0, 1.46), (0.17 * x, 0, 1.46)),
        (s + "Arm", s + "Shoulder", (0.17 * x, 0, 1.46), (0.45 * x, 0, 1.46)),
        (s + "ForeArm", s + "Arm", (0.45 * x, 0, 1.46), (0.70 * x, 0, 1.46)),
        (s + "Hand", s + "ForeArm", (0.70 * x, 0, 1.46), (0.80 * x, 0, 1.46)),
        (s + "UpLeg", "Hips", (0.10 * x, 0, 1.00), (0.10 * x, -0.01, 0.55)),
        (s + "Leg", s + "UpLeg", (0.10 * x, -0.01, 0.55), (0.10 * x, 0.0, 0.10)),
        (s + "Foot", s + "Leg", (0.10 * x, 0.0, 0.10), (0.10 * x, -0.12, 0.03)),
        (s + "ToeBase", s + "Foot", (0.10 * x, -0.12, 0.03), (0.10 * x, -0.20, 0.03)),
    ]
arm = bpy.data.armatures.new("Armature")
ob = bpy.data.objects.new("Armature", arm)
sc.collection.objects.link(ob)
bpy.context.view_layer.objects.active = ob
bpy.ops.object.mode_set(mode="EDIT")
for n, p, h, t in K:
    b = arm.edit_bones.new(P + n)
    b.head, b.tail = Vector(h), Vector(t)
    if p:
        b.parent = arm.edit_bones[P + p]
        b.use_connect = False
bpy.ops.object.mode_set(mode="POSE")

# IK-Ziele der Fuesse (leere Objekte) und Pole
ziele = {}
for s, x in (("Left", 1), ("Right", -1)):
    e = bpy.data.objects.new("Ziel" + s, None)
    sc.collection.objects.link(e)
    ziele[s] = e
    c = ob.pose.bones[P + s + "Leg"].constraints.new("IK")
    c.target = e
    c.chain_count = 2
    pol = bpy.data.objects.new("Pol" + s, None)
    pol.location = (0.10 * x, -1.0, 0.55)
    sc.collection.objects.link(pol)
    c.pole_target = pol
    c.pole_angle = -math.pi / 2

N = 30
schritt = 0.30   # halbe Schrittlaenge (m)
stand = 0.6      # Standphase (Anteil)
for f in range(N + 1):
    ph = f / N
    sc.frame_set(f + 1)
    for s, verz in (("Left", 0.0), ("Right", 0.5)):
        q = (ph + verz) % 1.0
        x = 0.10 if s == "Left" else -0.10
        if q < stand:   # Stand: von vorn (-Y) gleichmaessig nach hinten (+Y)
            y = -schritt + 2 * schritt * q / stand
            z = 0.10
        else:           # Schwung: zurueck nach vorn, gehoben
            u = (q - stand) / (1 - stand)
            y = schritt - 2 * schritt * (0.5 - 0.5 * math.cos(math.pi * u))
            z = 0.10 + 0.08 * math.sin(math.pi * u)
        e = ziele[s]
        e.location = (x, y, z)
        e.keyframe_insert("location", frame=f + 1)
    hb = ob.pose.bones[P + "Hips"]
    hb.location = (0.0, 0.0, -0.03 + 0.02 * math.cos(4 * math.pi * ph))   # Wippen (lokal: Knochen-Y = Welt-Z)
    hb.keyframe_insert("location", frame=f + 1)
    for s, verz in (("Left", 0.5), ("Right", 0.0)):
        a = ob.pose.bones[P + s + "Arm"]
        a.rotation_mode = "XYZ"
        a.rotation_euler = (0.0, 0.0, 0.0)
        a.rotation_euler[2] = 0.4 * math.sin(2 * math.pi * (ph + verz)) * (1 if s == "Left" else -1)
        a.keyframe_insert("rotation_euler", frame=f + 1)

# Backen (sichtbare Stellung, ohne Constraints)
bpy.context.view_layer.objects.active = ob
ob.select_set(True)
bpy.ops.nla.bake(frame_start=1, frame_end=N + 1, only_selected=False, visual_keying=True, clear_constraints=True, bake_types={"POSE"})
ob.animation_data.action.name = "Walk"
# nur die gebackene Aktion exportieren (die ungebackene der Fuss-IK-Ziele und die alte des Skeletts weg)
for ac in [ac for ac in bpy.data.actions if ac.name != "Walk"]:
    bpy.data.actions.remove(ac)
for e in list(ziele.values()):
    bpy.data.objects.remove(e)
for o in [o for o in sc.objects if o.name.startswith("Pol")]:
    bpy.data.objects.remove(o)

# Referenz: Weltorte der Knochenkoepfe je Bild (Blender-Welt, Meter)
ref = {"fps": sc.render.fps, "bilder": [], "namen": [P + n for n, _, _, _ in K]}
for f in range(1, N + 2):
    sc.frame_set(f)
    ref["bilder"].append([list(ob.matrix_world @ ob.pose.bones[n].head) for n in ref["namen"]])
json.dump(ref, open(os.path.join(aus, "testgang_ref.json"), "w"), indent=0)

bpy.ops.object.mode_set(mode="OBJECT")
ob.select_set(True)
bpy.ops.export_scene.fbx(filepath=os.path.join(aus, "testgang.fbx"), use_selection=False, add_leaf_bones=False,
                         bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                         bake_anim_simplify_factor=0.0, armature_nodetype="NULL", primary_bone_axis="Y", secondary_bone_axis="X")
bpy.ops.export_scene.gltf(filepath=os.path.join(aus, "testgang.glb"), export_format="GLB", export_animations=True,
                          export_frame_range=True, export_force_sampling=True, export_optimize_animation_size=False)
bpy.context.view_layer.objects.active = ob
bpy.ops.export_anim.bvh(filepath=os.path.join(aus, "testgang.bvh"), frame_start=1, frame_end=N + 1, global_scale=100.0, rotate_mode="ZXY")
if hasattr(bpy.ops.wm, "collada_export"):
    try:
        bpy.ops.wm.collada_export(filepath=os.path.join(aus, "testgang.dae"), include_animations=True, sampling_rate=1,
                                  keep_keyframes=False, export_animation_type_selection="sample")
    except Exception as ex:   # noqa: BLE001
        print("COLLADA:", ex)
print("FERTIG", sorted(os.listdir(aus)))
