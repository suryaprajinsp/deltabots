"""Use downloaded Epic rock geometry without importing desktop water shaders."""
import unreal as u
import json
from pathlib import Path
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
levels.load_level('/Game/DeltaBots/Levels/L_DeltaFrontier_v01')
rock=u.load_asset('/Game/WaterPlane/Environment/Props/SM_Rock')
if not isinstance(rock,u.StaticMesh):raise RuntimeError('Owned Water Planes rock is not installed.')
extent=rock.get_bounds().box_extent
count=0
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith('Frontier_') and a.get_actor_label().endswith('_ridge'):
        c=a.static_mesh_component
        old=c.get_material(0)
        s=a.get_actor_scale3d()
        c.set_static_mesh(rock);c.set_material(0,old)
        a.set_actor_scale3d(u.Vector(s.x*50/max(extent.x,1),s.y*50/max(extent.y,1),s.z*50/max(extent.z,1)))
        count+=1
if not levels.save_current_level():raise RuntimeError('Could not save asset pass')
out=Path(u.Paths.project_dir())/'Saved/DeltaBotsDevelopment/frontier-epic-assets.json'
out.write_text(json.dumps({'pack':'Water Planes by Epic Games','rock_replacements':count,
 'source_asset':'/Game/WaterPlane/Environment/Props/SM_Rock',
 'materials':'Existing lightweight biome materials retained for the mobile test',
 'water_material_integration':'pending mobile evaluation'},indent=2))
u.log('DELTA_EPIC_ROCKS_APPLIED '+str(count))
