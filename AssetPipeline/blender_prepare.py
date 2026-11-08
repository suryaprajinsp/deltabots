"""Run in a fresh background Blender; preserves the incoming rig and clips."""
import bpy
import hashlib
import json
from mathutils import Vector
from pathlib import Path
import shutil
import sys

job = json.loads(Path(sys.argv[sys.argv.index('--')+1]).read_text())
source, out = Path(job['input']), Path(job['output'])
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
if source.suffix.lower() == '.glb':
    bpy.ops.import_scene.gltf(filepath=str(source))
else:
    bpy.ops.import_scene.fbx(filepath=str(source))
objects = list(bpy.context.scene.objects)
meshes = [obj for obj in objects if obj.type=='MESH']
if not meshes:
    raise RuntimeError('No mesh objects in the source asset')
rigged = any(obj.type=='ARMATURE' for obj in objects)
# FBX/Unreal requires skin weights for rigid parts parented directly to bones.
for obj in meshes:
    if obj.parent and obj.parent.type=='ARMATURE' and obj.parent_type=='BONE':
        armature, bone, matrix = obj.parent, obj.parent_bone, obj.matrix_world.copy()
        obj.parent_type='OBJECT'; obj.parent=armature; obj.matrix_world=matrix
        group=obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
        group.add(list(range(len(obj.data.vertices))),1.0,'REPLACE')
        if not any(mod.type=='ARMATURE' for mod in obj.modifiers):
            obj.modifiers.new(name='Rigid skin',type='ARMATURE').object=armature
points = [obj.matrix_world @ Vector(v) for obj in meshes for v in obj.bound_box]
lo = Vector(tuple(min(v[i] for v in points) for i in range(3)))
hi = Vector(tuple(max(v[i] for v in points) for i in range(3)))
if hi.z-lo.z <= 1e-6:
    raise RuntimeError('Asset has no measurable height')
scale = job['height_m']/(hi.z-lo.z) if job['height_m'] else 1.0
# Transform root objects together, including armatures and animated children.
for obj in objects:
    if obj.parent is None:
        obj.location = (obj.location-Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z)))*scale
        obj.scale *= scale
bpy.context.view_layer.update()
triangles = 0
for obj in meshes:
    obj.data.calc_loop_triangles(); triangles += len(obj.data.loop_triangles)
warnings = []
if triangles > job['triangle_budget']:
    warnings.append('Triangle budget exceeded; review topology/LODs before mobile use')
if rigged:
    warnings.append('Rig preserved; skeleton and animation retargeting require review')
if not any(obj.data.uv_layers for obj in meshes):
    warnings.append('No UV layers; material/texture work required')
shutil.copy2(source, out/('source'+source.suffix.lower()))
bpy.ops.wm.save_as_mainfile(filepath=str(out/(job['name']+'.blend')))
fbx = out/(job['name']+'.fbx')
bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=False, object_types={'MESH','ARMATURE','EMPTY'},
    axis_forward='-Y',axis_up='Z',add_leaf_bones=False,bake_anim=True,path_mode='COPY',embed_textures=True,
    mesh_smooth_type='FACE')
bpy.ops.export_scene.gltf(filepath=str(out/(job['name']+'.glb')),export_format='GLB')
manifest = {'name':job['name'],'project':job['project'],'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
    'fbx':str(fbx),'triangles':triangles,'triangle_budget':job['triangle_budget'],'rigged':rigged,
    'animation_actions':len(bpy.data.actions),'scale_applied':scale,'warnings':warnings,
    'rights':'Review source task and input rights before public redistribution', 'status':'prepared_for_review'}
(out/'asset.json').write_text(json.dumps(manifest,indent=2))
print('ASSET_PREPARED '+json.dumps(manifest))
