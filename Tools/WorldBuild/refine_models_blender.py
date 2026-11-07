"""Original low-complexity rig variants informed by the user's visible references.
No Tripo geometry is downloaded or reconstructed as an exact replica.
"""
import bpy
from pathlib import Path
from mathutils import Vector
import json
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'ArtSource/DeltaFrontier/DeltaBot_Rig_Concept.blend'
OUT=ROOT/'ArtSource/DeltaFrontier/RefinedBots';OUT.mkdir(parents=True,exist_ok=True)

def make(master=False):
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
    rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
    parts=[o for o in bpy.data.objects if any(m.type=='ARMATURE' and m.object==rig for m in o.modifiers)]
    for o in list(bpy.data.objects):
        if o not in parts and o!=rig:bpy.data.objects.remove(o,do_unlink=True)
    rig.location=(0,0,0)
    rig.animation_data.action=None
    for b in rig.pose.bones:b.rotation_euler=(0,0,0);b.location=(0,0,0)
    def mat(name,color,emission=0):
        m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
        p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1)
        p.inputs['Metallic'].default_value=.65;p.inputs['Roughness'].default_value=.36
        p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=emission
        return m
    silver=mat('Refined silver',(.58,.65,.7));dark=mat('Joint graphite',(.025,.04,.055))
    energy=mat('Rhino magenta' if master else 'Delta cyan',(1,.015,.32) if master else (.015,.65,1),3)
    for o in parts:
        for i,m in enumerate(o.data.materials):
            if m and ('Cyan' in m.name):o.data.materials[i]=energy
    def weighted(o,bone,material):
        o.data.materials.append(material);g=o.vertex_groups.new(name=bone);g.add(list(range(len(o.data.vertices))),1,'REPLACE')
        o.parent=rig;mod=o.modifiers.new('Original rigid weighting','ARMATURE');mod.object=rig;parts.append(o)
        return o
    def plate(name,loc,scale,bone,material):
        bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.scale=scale
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        b=o.modifiers.new('Armor bevel','BEVEL');b.width=.035;b.segments=2
        return weighted(o,bone,material)
    for side,k in [('L',-1),('R',1)]:
        plate('Layered pauldron '+side,(k*.34,-.025,1.48),(.3,.35,.16),'upperarm_'+side,silver)
        plate('Chest wing '+side,(k*.19,-.19,1.36),(.2,.065,.15),'spine',silver)
        plate('Calf outer armor '+side,(k*.22,.02,.33),(.11,.27,.29),'shin_'+side,silver)
        plate('Shoulder energy '+side,(k*.34,-.205,1.48),(.18,.018,.035),'upperarm_'+side,energy)
    if master:
        # Original Rhino silhouette: forward forehead horn, broad pauldrons and
        # three retractable forearm blades per side; no animal face or tail.
        bpy.ops.mesh.primitive_cone_add(vertices=8,radius1=.09,radius2=.005,depth=.34,location=(0,-.20,1.84))
        horn=bpy.context.object;horn.name='Rhino helmet horn';horn.rotation_euler=(.55,0,0);weighted(horn,'head',silver)
        for side,k in [('L',-1),('R',1)]:
            plate('Rhino heavy forearm '+side,(k*.48,0,1.05),(.28,.31,.34),'forearm_'+side,silver)
            for f in (-1,0,1):
                plate('Rhino retractable blade '+side,(k*.48+f*.065,-.38,1.01),(.025,.48,.035),'forearm_'+side,energy)
        rig.scale=(1.25,1.17,1.15)
    name='RhinoMaster_Concept_v02' if master else 'DeltaBot_Concept_v02'
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts+[rig]:o.hide_set(False);o.hide_render=False;o.select_set(True)
    bpy.context.view_layer.objects.active=rig
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(name+'.blend')))
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_animations=True)
    scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.render.resolution_x=900;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
    scene.world.color=(.12,.14,.17)
    bpy.ops.object.light_add(type='AREA',location=(2,-4,5));bpy.context.object.data.energy=700;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=5
    bpy.ops.object.light_add(type='AREA',location=(-3,0,3));bpy.context.object.data.energy=500;bpy.context.object.data.size=4
    bpy.ops.object.camera_add(location=(3,-5,2.7));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=65;scene.camera=cam
    scene.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
    return {'name':name,'bones':len(rig.data.bones),'mesh_parts':len(parts),'status':'original rigid-weighted concept; not imported into gameplay yet'}
rows=[make(False),make(True)]
(OUT/'models.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows))

