"""Create a separate Blender review scene for the first Beast territory."""
from pathlib import Path
import math
import bpy
from mathutils import Vector

root = Path(__file__).resolve().parents[2] / "ArtSource/DeltaFrontier"
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.render.engine = "CYCLES"
scene.cycles.samples = 12
scene.render.resolution_x, scene.render.resolution_y = 1280, 800
scene.world = bpy.data.worlds.new("Basin daylight")
scene.world.color = (.24, .33, .43)

def material(name, rgb, glow=0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*rgb, 1)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*rgb, 1)
    shader.inputs["Emission Color"].default_value = (*rgb, 1)
    shader.inputs["Emission Strength"].default_value = glow
    return mat

stone = material("Basalt", (.12, .17, .19))
salt = material("Mineral shelf", (.65, .53, .37))
water = material("Conductive brine", (.04, .35, .36))
armor = material("Scarab copper ceramic", (.72, .24, .075))
core = material("Scarab warning light", (1, .46, .04), 4)
safe = material("Safe route marker", (.02, .7, .85), 2)

def box(name, loc, size, mat):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    return obj

box("Basin bed", (0, 0, -1.3), (64, 54, 1), stone)
box("Brine hazard", (0, 0, -.52), (60, 46, .12), water)
box("Arrival and recovery shelf", (0, -20, -.3), (18, 10, .6), salt)
box("Scarab encounter shelf", (0, 10, -.3), (20, 18, .6), salt)
box("West sheltered bypass", (-15, -3, -.3), (5, 30, .6), salt)
box("South bypass connector", (-8, -17, -.3), (17, 5, .6), salt)
box("North bypass connector", (-8, 10, -.3), (17, 5, .6), salt)
for i in range(5):
    box(f"Exposed crossing {i+1}", (0, -13 + i*3, -.3), (5, 2.4, .6), salt)
for i, y in enumerate(range(-14, 11, 5)):
    box(f"Cover pier {i+1}", (-17, y, 1.4), (1.4, 2.5, 2.8), stone)
    box(f"Bypass marker {i+1}", (-13, y, .2), (.15, .3, .4), safe)
for side in (-1, 1):
    for i in range(7):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, location=(side*(27-i%2*2), -19+i*6, 1))
        rock = bpy.context.object
        rock.name = "Basalt boundary"
        rock.scale = (4, 4.5, 3+i%3)
        rock.data.materials.append(stone)

body = box("Scarab main chassis", (0, 10, 1.3), (3, 3.3, .8), stone)
shell = box("Scarab shield shell", (0, 10, 1.86), (3.45, 3.6, .42), armor)
vent = box("Exposed core vent", (0, 8.25, 1.45), (1.35, .3, .55), core)
for side in (-1, 1):
    for i in range(3):
        leg = box("Scarab articulated leg preview", (side*1.65, 9+i*.95, .84), (1.9, .22, .24), armor)
        leg.rotation_euler.y = side*.48
        box("Scarab support foot", (side*2.42, 9+i*.95, .18), (.42, .44, .36), stone)

# Readable charge, shield release, vulnerable hold, then reset.
for frame, tilt, glow in [(1,0,.6),(18,0,4),(27,-.48,6),(48,-.48,3),(65,0,.6)]:
    shell.rotation_euler.x = tilt
    shell.keyframe_insert(data_path="rotation_euler", frame=frame)
    vent.scale.z = .5 + glow/12
    vent.keyframe_insert(data_path="scale", frame=frame)
shell.animation_data.action.name = "Scarab_ShieldCharge_Expose_Recover"
scene.frame_start, scene.frame_end = 1, 65
scene.frame_set(31)
bpy.ops.object.light_add(type="SUN", rotation=(.52, -.5, -.42))
bpy.context.object.data.energy = 3.4
bpy.ops.object.camera_add(location=(39, -50, 38))
cam = bpy.context.object
cam.rotation_euler = (Vector((0, 1, 0)) - cam.location).to_track_quat("-Z", "Y").to_euler()
cam.data.lens = 38
scene.camera = cam
scene.render.filepath = str(root / "Review/renders/scarab-basin.png")
bpy.ops.wm.save_as_mainfile(filepath=str(root / "ScarabBasin_Concept.blend"))
bpy.ops.render.render(write_still=True)
