"""Create a separate explorable biome blockout; retain prototype combat actors."""
import json
import math
import random
from pathlib import Path
import unreal as u

random.seed(815)
LEVEL = '/Game/DeltaBots/Levels/L_DeltaFrontier_v01'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
report_path=Path(u.Paths.project_dir())/'Saved/DeltaBotsDevelopment/frontier-build.json'
if report_path.exists():
    raise RuntimeError('Completed Frontier already exists; preserve manual edits.')
if u.EditorAssetLibrary.does_asset_exist(LEVEL):
    levels.load_level(LEVEL)
    if any(a.get_actor_label().startswith('Frontier_') for a in actors.get_all_level_actors()):
        raise RuntimeError('Saved Frontier edits detected; do not overwrite.')
elif not levels.new_level_from_template(LEVEL, '/Game/DeltaBots/Levels/L_DeltaBotsPrototype'):
    raise RuntimeError('Template map could not be opened.')
asset_tools = u.AssetToolsHelpers.get_asset_tools()
made = []

def material(name, rgb, metal=0, glow=0, opacity=1):
    path='/Game/DeltaBots/Frontier/Materials/' + name
    if u.EditorAssetLibrary.does_asset_exist(path):
        return u.load_asset(path)
    m=asset_tools.create_asset(name, '/Game/DeltaBots/Frontier/Materials', u.Material, u.MaterialFactoryNew())
    color=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant',u.LinearColor(*rgb,1))
    u.MaterialEditingLibrary.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    def scalar(value,prop):
        e=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant)
        e.set_editor_property('r',value)
        u.MaterialEditingLibrary.connect_material_property(e,'',prop)
    scalar(metal,u.MaterialProperty.MP_METALLIC)
    scalar(.7,u.MaterialProperty.MP_ROUGHNESS)
    if glow:
        u.MaterialEditingLibrary.connect_material_property(color,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    if opacity<1:
        m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
        scalar(opacity,u.MaterialProperty.MP_OPACITY)
        m.set_editor_property('two_sided',True)
    u.MaterialEditingLibrary.recompile_material(m)
    u.EditorAssetLibrary.save_loaded_asset(m)
    return m

M={
 'alloy':material('M_FrontierAlloy',(.12,.19,.24),.65),
 'cyan':material('M_FrontierCyan',(.015,.65,.9),.1,1),
 'sand':material('M_FrontierSand',(.47,.29,.13)),
 'snow':material('M_FrontierSnow',(.8,.9,.95)),
 'rock':material('M_FrontierBasalt',(.08,.105,.13)),
 'forest':material('M_FrontierForest',(.10,.25,.11)),
 'trunk':material('M_FrontierTrunk',(.15,.09,.045)),
 'water':material('M_FrontierWater',(.015,.18,.28),0,0,.32),
 'glass':material('M_FrontierGlass',(.05,.45,.6),0,0,.12),
}
SHAPES={n:u.load_asset('/Engine/BasicShapes/'+n) for n in ('Cube','Sphere','Cylinder')}

def shape(name,loc,size,mat='alloy',kind='Cube',collision=True):
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*loc))
    a.set_actor_label('Frontier_'+name)
    a.set_folder_path('DeltaFrontier/'+name.split('_')[0])
    c=a.static_mesh_component;c.set_static_mesh(SHAPES[kind]);c.set_material(0,M[mat])
    a.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
    if not collision:c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    c.set_editor_property('mobility',u.ComponentMobility.STATIC)
    made.append(a)
    return a

def sign(text,x,y,z=150):
    a=actors.spawn_actor_from_class(u.TextRenderActor,u.Vector(x,y,z))
    a.set_actor_label('Frontier_Sign_'+text)
    c=a.text_render;c.set_text(text);c.set_world_size(70);c.set_text_render_color(u.Color(50,220,255,255))
    a.set_actor_rotation(u.Rotator(0,180,0),False)
    made.append(a)

def city(name,cx,cy,floor=0):
    shape(name+'_platform',(cx,cy,floor-60),(7200,7200,120))
    for i in range(10):
        angle=i*math.tau/10
        x=cx+2600*math.cos(angle);y=cy+2600*math.sin(angle)
        h=700+(i%4)*350
        shape(name+'_tower',(x,y,floor+h/2),(650,650,h))
        shape(name+'_light',(x-330,y,floor+h*.65),(18,550,28),'cyan',collision=False)
    sign(name,cx-1700,cy,floor+180)

# A continuous test island, ~400 m across. Biomes are early geometry blockouts.
shape('Ground',(3000,0,-160),(44000,36000,200),'rock')
city('NEXUS_CITY',0,7000)
for cx,cy,title,mat in ((-10500,0,'RHINO_DESERT','sand'),(-10500,10500,'FROST_RIDGE','snow'),
                        (7000,10000,'CANOPY_FOREST','forest'),(13000,-7000,'TIDAL_BASIN','sand')):
    shape(title+'_ground',(cx,cy,-25),(10500,10000,100),mat)
    sign(title,cx-2000,cy)
    for i in range(9):
        a=i*math.tau/9
        px=cx+4200*math.cos(a);py=cy+4100*math.sin(a)
        shape(title+'_ridge',(px,py,350),(1700,1400,1400),mat,'Sphere')
for i in range(32):
    x=random.uniform(3000,11200);y=random.uniform(7000,13500)
    # Keep a walkable east-west trail through the forest.
    if abs(y-10000)<500:continue
    shape('Forest_trunk',(x,y,330),(100,100,700),'trunk','Cylinder')
    shape('Forest_crown',(x,y,850),(650,650,750),'forest','Sphere',False)

city('SKY_HARBOR',9500,0,1800)
# Walkable gently rising causeway; no flight system is implied by the display jets.
for i in range(12):
    a=shape('Sky_ramp',(4000+i*350,0,50+i*150),(430,800,65))
    a.set_actor_rotation(u.Rotator(23,0,0),False)
shape('Tidal_water',(13000,-7000,180),(10000,9500,18),'water',collision=False)
city('ABYSS_DOME',10500,-6500,0)
shape('Abyss_dome',(10500,-6500,900),(7600,7600,3400),'glass','Sphere',False)
shape('Abyss_causeway',(5900,-3900,40),(8500,950,100))
sign('ABYSS WALKWAY - WATER VISUAL TEST',6000,-3900)

# Vehicle silhouettes are explicitly non-drivable prototypes at this milestone.
for x,y in ((-1200,5900),(1300,5900)):
    shape('Transport_hovercar',(x,y,160),(520,240,160))
    shape('Transport_canopy',(x+40,y,260),(200,190,100),'glass')
    for side in (-1,1):shape('Transport_thruster',(x-140,y+side*135,100),(120,45,45),'cyan',collision=False)
shape('Transport_jet',(9700,0,2150),(800,180,180))
shape('Transport_wings',(9700,0,2150),(220,900,40))
sign('TRANSPORT DESIGN BAY - DISPLAY PROTOTYPES',-2000,5100)

# Imported Fab model is staged as a static exhibit; select its largest mesh so
# disconnected imported subparts are not incorrectly treated as a complete turret.
turret_root='/Game/DeltaBots/ThirdParty/Fab/SciFiTurret'
turret_meshes=[]
for path in u.EditorAssetLibrary.list_assets(turret_root,recursive=True):
    obj=u.load_asset(path)
    if isinstance(obj,u.StaticMesh):turret_meshes.append(obj)
if turret_meshes:
    mesh=max(turret_meshes,key=lambda m:m.get_bounds().sphere_radius)
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(-1000,7100,80))
    a.set_actor_label('Frontier_FabTurret_Component_Study')
    a.static_mesh_component.set_static_mesh(mesh)
    made.append(a)

# Reuse the existing tested enemy class as optional test targets, away from spawn.
beast=u.load_class(None,'/Game/DeltaBots/Enemies/BP_DeltaBeast.BP_DeltaBeast_C')
if beast:
    for x,y in ((-9000,1800),(7000,11000),(11000,-7200)):
        a=actors.spawn_actor_from_class(beast,u.Vector(x,y,120));a.set_actor_label('Frontier_TestBeast');made.append(a)

if not levels.save_current_level():raise RuntimeError('Map save failed')
report={'level':LEVEL,'actors_added':len(made),'library_mesh_components_available':len(turret_meshes),
        'status':'geometry blockout; runtime testing pending','vehicle_status':'static displays',
        'water_status':'visual water and dome; no swimming yet',
        'biomes':['Nexus City','Rhino Desert','Frost Ridge','Canopy Forest','Sky Harbor','Abyss Dome']}
out=Path(u.Paths.project_dir())/'Saved/DeltaBotsDevelopment/frontier-build.json'
out.write_text(json.dumps(report,indent=2))
u.log('DELTA_FRONTIER_SAVED '+json.dumps(report))
