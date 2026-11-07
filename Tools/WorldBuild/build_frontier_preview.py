"""Blender API concept scene and original rigid-weighted Delta Bot rig.
Review source only: no production mesh export until the layout/form is approved.
Run: blender --background --python Tools/WorldBuild/build_frontier_preview.py
"""
import bpy, math, random, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'ArtSource/DeltaFrontier'; REVIEW=OUT/'Review'
random.seed(2401)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene
scene.unit_settings.system='METRIC'
scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world.color=(.3,.4,.5)
scene.view_settings.view_transform='AgX'

def mat(name,color,metal=0,rough=.6,glow=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if glow:p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=glow
 return m
M={n:mat(n,c,metal,rough,glow) for n,c,metal,rough,glow in [
 ('Ivory',(.72,.76,.69),.15,.6,0),('Graphite',(.045,.075,.095),.5,.45,0),
 ('Teal',(.025,.22,.24),.25,.5,0),('Clay',(.47,.23,.14),0,.9,0),
 ('Sand',(.37,.33,.23),0,1,0),('Grass',(.18,.29,.18),0,.95,0),
 ('Cyan',(.01,.7,.9),.15,.35,3),('Solar',(.025,.075,.15),.4,.28,0),
 ('Leaf',(.12,.34,.24),0,.9,0),('Orange',(.85,.25,.05),.1,.6,0) ]}
objects=[]
def cube(name,loc,size,material='Ivory',bevel=.06):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(M[material]);o['design_phase']='function_review';objects.append(o)
 if bevel:
  mod=o.modifiers.new('Manufactured edges','BEVEL');mod.width=min(bevel,min(size)/5);mod.segments=1
  o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
 return o
def ico(name,loc,size,material='Clay',sub=1):
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub,radius=1,location=loc);o=bpy.context.object;o.name=name;o.scale=size;o.data.materials.append(M[material]);objects.append(o);return o
def cyl(name,loc,radius,depth,material='Graphite',vertices=12):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc);o=bpy.context.object;o.name=name;o.data.materials.append(M[material]);objects.append(o);return o
def beam(name,a,b,r=.1,material='Graphite'):
 a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(a-b).length,material);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def camera(name,loc,target,lens=40):
 bpy.ops.object.camera_add(location=loc);o=bpy.context.object;o.name=name;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();o.data.lens=lens;o.data.clip_end=1200;return o
def label(text,loc,scale=1):
 bpy.ops.object.text_add(location=loc,rotation=(math.pi/2,0,0));o=bpy.context.object;o.name='Sign_'+text;o.data.body=text;o.data.align_x='CENTER';o.data.size=scale;o.data.extrude=.004;o.data.materials.append(M['Cyan']);return o

# A compact open exploration district; design asymmetry is intentional by biome.
cube('Continuous_ground',(0,0,-1),(240,200,2),'Grass',0)
cube('Hub_apron',(-70,-55,.08),(45,40,.16),'Sand')
cube('East_route',(10,-55,.12),(120,8,.24),'Sand')
cube('North_route',(-70,18,.12),(8,110,.24),'Sand')
cube('Ridge_route',(18,65,.12),(160,8,.24),'Sand')
cube('Return_route',(80,2,.12),(8,130,.24),'Sand')
cube('Central_route',(0,5,.12),(9,104,.24),'Sand')
zones=[{'id':'arrival','center':[-70,-55],'purpose':'Portal camp, safe spawn, workshop'},
 {'id':'solar','center':[-65,50],'purpose':'Solar yard and salvage routes'},
 {'id':'relay','center':[65,-50],'purpose':'Relay courtyard, ranged encounters'},
 {'id':'scarab','center':[65,58],'purpose':'Scarab territory and boss reserve'}]

def building(name,x,y,w,d,h):
 # Concept shell has a real 3 m entry; no decorative false doorway.
 cube(name+'_slab',(x,y,.2),(w+.6,d+.6,.4),'Graphite')
 cube(name+'_back',(x,y+d/2,h/2),(w,.4,h))
 cube(name+'_west',(x-w/2,y,h/2),(.4,d,h))
 cube(name+'_east',(x+w/2,y,h/2),(.4,d,h))
 side=(w-3)/2
 for sign in (-1,1):cube(name+'_front',(x+sign*(1.5+side/2),y-d/2,h/2),(side,.4,h))
 cube(name+'_lintel',(x,y-d/2,h-.4),(3,.4,.8),'Teal')
 cube(name+'_roof',(x,y,h+.15),(w+.8,d+.8,.3),'Teal')
 for xx in (-w*.3,0,w*.3):
  p=cube(name+'_solar',(x+xx,y,h+.55),(w*.25,d*.7,.12),'Solar');p.rotation_euler.x=.12
 cube(name+'_entry_light',(x,y-d/2-.23,h-.85),(2.8,.08,.06),'Cyan',0)

building('Workshop',-82,-42,14,12,5)
building('Research',-55,-42,12,10,6)
label('DELTA // FRONTIER',(-70,-49,4.5),1.1)
for x,y in [(-76,37),(-54,44),(54,-45),(76,-39),(48,45)]:building('Outpost',x,y,12,10,5)

# Portal silhouette, solar landmarks and low, readable tactical cover.
for x in (-76,-64):cube('Portal_pylon',(x,-65,4),(1.6,2,8),'Graphite');cube('Portal_edge',(x,-66.05,4),(.18,.08,6),'Cyan')
cube('Portal_bridge',(-70,-65,8),(13,2,1.2),'Ivory');cube('Portal_emitter',(-70,-65.95,7.5),(10,.1,.15),'Cyan')
for x,y in [(-80,66),(-53,66),(-80,54),(-53,54)]:
 for dx in (-3,3):beam('Solar_support',(x+dx,y,0),(x+dx,y,4),.12)
 p=cube('Solar_canopy',(x,y,4.2),(10,7,.2),'Solar');p.rotation_euler.x=.2
for x,y in [(-45,-55),(-18,-50),(18,-60),(45,-58),(80,-15),(75,18),(40,65),(12,61),(-30,68),(-70,5)]:
 cube('Tactical_cover',(x,y,.7),(3.2,1.2,1.4),'Teal')
 cube('Cover_cap',(x,y,1.44),(3.3,1.3,.12),'Ivory')
for i in range(42):
 x=random.uniform(-115,115);y=random.uniform(-95,95)
 if min(abs(y+55),abs(y-65),abs(x+70),abs(x-80),abs(x))<9:continue
 if any(math.hypot(x-z['center'][0],y-z['center'][1])<25 for z in zones):continue
 s=random.uniform(2,6);ico('Rock_cluster',(x,y,s*.4),(s,s*.8,s*.7))
for x,y in [(-101,-75),(-98,-25),(-35,-78),(32,-80),(103,-51),(108,20),(105,76),(-20,86),(-104,75)]:
 cyl('Tree_trunk',(x,y,2),.6,4,'Clay');ico('Tree_canopy',(x,y,6),(4,4,3),'Leaf',2)
for x,y in [(-112,-92),(-65,96),(25,96),(115,85),(118,-85),(-116,20)]:
 ico('Boundary_ridge',(x,y,6),(18,14,13),'Clay',1)
cube('Boss_reserved_pad',(65,58,.15),(25,25,.3),'Graphite')
for x in (54,76):cube('Boss_signal',(x,67,4),(.5,.5,8),'Orange')

# Original Delta Bot concept: rigid panels weighted to an articulated skeleton.
world_objects=set(bpy.data.objects)
bpy.ops.object.armature_add(location=(0,0,0));rig=bpy.context.object;rig.name='DeltaBot_Rig'
bpy.ops.object.mode_set(mode='EDIT');rig.data.edit_bones.remove(rig.data.edit_bones[0])
bones={}
def bone(n,a,b,parent=None):
 e=rig.data.edit_bones.new(n);e.head=a;e.tail=b
 if parent:e.parent=bones[parent]
 bones[n]=e
bone('root',(0,0,0),(0,0,.1))
bone('pelvis',(0,0,.9),(0,0,1.05),'root');bone('spine',(0,0,1.05),(0,0,1.42),'pelvis');bone('head',(0,0,1.42),(0,0,1.8),'spine')
for side,s in [('L',-1),('R',1)]:
 bone('upperarm_'+side,(s*.27,0,1.42),(s*.43,0,1.15),'spine');bone('forearm_'+side,(s*.43,0,1.15),(s*.46,-.02,.91),'upperarm_'+side)
 bone('hand_'+side,(s*.46,-.02,.91),(s*.46,-.1,.84),'forearm_'+side)
 bone('thigh_'+side,(s*.15,0,.93),(s*.15,0,.52),'pelvis');bone('shin_'+side,(s*.15,0,.52),(s*.15,0,.12),'thigh_'+side);bone('foot_'+side,(s*.15,0,.12),(s*.15,-.22,.1),'shin_'+side)
bpy.ops.object.mode_set(mode='OBJECT')
parts=[]
def skin(o,b):
 g=o.vertex_groups.new(name=b);g.add(list(range(len(o.data.vertices))),1,'REPLACE');o.parent=rig
 mod=o.modifiers.new('Delta articulation','ARMATURE');mod.object=rig;parts.append(o);return o
def panel(n,p,s,b,material='Ivory',bevel=.05):return skin(cube(n,p,s,material,bevel),b)
panel('Pelvis',(0,0,.96),(.42,.28,.22),'pelvis','Graphite')
panel('Chest',(0,0,1.25),(.55,.30,.43),'spine')
panel('Chest_inlay',(0,-.163,1.28),(.32,.025,.27),'spine','Graphite')
mesh=bpy.data.meshes.new('DownwardDelta');mesh.from_pydata([(-.11,-.185,1.36),(.11,-.185,1.36),(0,-.185,1.17)],[],[(0,1,2)]);mesh.update()
emblem=bpy.data.objects.new('Downward_Delta_Core',mesh);bpy.context.collection.objects.link(emblem);mesh.materials.append(M['Cyan']);skin(emblem,'spine')
panel('Rounded_helmet',(0,0,1.64),(.42,.35,.36),'head',bevel=.11)
panel('Dark_visor',(0,-.177,1.65),(.34,.035,.16),'head','Graphite',.05)
panel('Eyes',(0,-.2,1.65),(.24,.018,.025),'head','Cyan',.005)
panel('Delta_pack',(0,.22,1.26),(.33,.18,.38),'spine','Graphite')
for side,s in [('L',-1),('R',1)]:
 panel('Shoulder_'+side,(s*.31,0,1.41),(.22,.30,.20),'upperarm_'+side)
 panel('Upperarm_'+side,(s*.37,0,1.27),(.16,.19,.25),'upperarm_'+side,'Graphite')
 panel('Forearm_'+side,(s*.45,-.01,1.04),(.20,.23,.28),'forearm_'+side)
 panel('Hand_'+side,(s*.46,-.04,.87),(.14,.20,.14),'hand_'+side,'Graphite')
 panel('Thigh_'+side,(s*.15,0,.73),(.23,.28,.35),'thigh_'+side)
 panel('Knee_'+side,(s*.15,-.1,.52),(.19,.16,.14),'shin_'+side,'Graphite')
 panel('Shin_'+side,(s*.15,0,.31),(.20,.24,.31),'shin_'+side)
 panel('Boot_'+side,(s*.15,-.08,.1),(.23,.4,.18),'foot_'+side,'Graphite')
 panel('Leg_light_'+side,(s*.15,-.128,.31),(.035,.018,.22),'shin_'+side,'Cyan',.005)
panel('Wrist_sword_emitter',(-.46,-.14,1.03),(.15,.14,.2),'forearm_L','Graphite')
panel('Wrist_laser_blade',(-.46,-.48,1.03),(.06,.62,.045),'forearm_L','Cyan',.01)
panel('Wrist_gun',(.46,-.16,1.04),(.15,.29,.14),'forearm_R','Graphite')
panel('Gun_emitter',(.46,-.31,1.04),(.07,.025,.06),'forearm_R','Cyan',.01)

actions=[]
for name,length,swing in [('Idle',48,.025),('Walk',32,.38),('Run',24,.65),('Jump',32,.25)]:
 rig.animation_data_create();rig.animation_data.action=None
 for f in range(1,length+2,4):
  phase=(f-1)/length*math.tau
  for p in rig.pose.bones:p.rotation_mode='XYZ';p.rotation_euler=(0,0,0);p.location=(0,0,0)
  for side,s in [('L',1),('R',-1)]:
   rig.pose.bones['thigh_'+side].rotation_euler.x=math.sin(phase)*s*swing
   rig.pose.bones['shin_'+side].rotation_euler.x=max(0,-math.sin(phase)*s)*swing*.8
   rig.pose.bones['upperarm_'+side].rotation_euler.x=-math.sin(phase)*s*swing*.65
  rig.pose.bones['spine'].location.z=abs(math.sin(phase))*swing*.04
  if name=='Jump':rig.pose.bones['root'].location.z=max(0,math.sin((f-1)/length*math.pi))*.45
  for p in rig.pose.bones:p.keyframe_insert('rotation_euler',frame=f);p.keyframe_insert('location',frame=f)
 action=rig.animation_data.action;action.name='Delta_'+name;action.use_fake_user=True;actions.append(action.name)
rig.animation_data.action=bpy.data.actions['Delta_Idle'];scene.frame_set(1)
rig.location=(-70,-58,.4)

bpy.ops.object.light_add(type='SUN',location=(0,0,80));sun=bpy.context.object;sun.rotation_euler=(.5,-.4,-.45);sun.data.energy=3;sun.data.angle=.08
bpy.ops.object.light_add(type='AREA',location=(0,-30,90));bpy.context.object.data.energy=1700;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=100
worldcam=camera('World_review',(150,-190,150),(0,0,0),43)
scene.camera=worldcam
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'DeltaFrontier_Concept.blend'))
scene.render.filepath=str(REVIEW/'renders/world-overview.png');bpy.ops.render.render(write_still=True)
# Actual geometry projection, no generic floor-plan icons.
worldcam.location=(0,0,270);worldcam.rotation_euler=(0,0,0);worldcam.data.type='ORTHO';worldcam.data.ortho_scale=265
scene.render.resolution_x=1600;scene.render.resolution_y=1400
scene.render.filepath=str(REVIEW/'plans/lower-plan.png');bpy.ops.render.render(write_still=True)
# Isolated character review; retain the real rig and all clips in a second source.
for o in list(bpy.data.objects):
 if o not in parts and o!=rig and o.type not in ('LIGHT','CAMERA'):o.hide_render=True
rig.location=(0,0,0)
cube('Character_plinth',(0,0,-.08),(3,3,.16),'Graphite')
cam=camera('Character_review',(3,-5,2.5),(0,0,1.0),70);scene.camera=cam
scene.render.resolution_x=1000;scene.render.resolution_y=1200
scene.render.filepath=str(REVIEW/'renders/delta-bot-rig.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'DeltaBot_Rig_Concept.blend'))
report={'stage':'function_review','production_exported':False,'zones':zones,'rig':{'bones':len(rig.data.bones),'weighted_parts':len(parts),'actions':actions},'world_meshes':len([o for o in world_objects if o.type=='MESH']),'triangles':sum(len(o.data.polygons)*2 for o in world_objects if o.type=='MESH'),'sources':['original procedural Blender geometry'],'approval':'pending'}
(REVIEW/'preview-report.json').write_text(json.dumps(report,indent=2))
print('DELTA_FRONTIER_PREVIEW_OK',json.dumps(report))
