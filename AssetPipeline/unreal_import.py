"""Import prepared FBX into a new folder; never overwrites an existing asset."""
import json
from pathlib import Path
import sys
import unreal as u

job = json.loads(Path(sys.argv[1]).read_text())
manifest_path = Path(job['manifest'])
asset = json.loads(manifest_path.read_text())
if u.EditorAssetLibrary.does_directory_exist(job['destination']):
    raise RuntimeError('Destination exists; choose a new asset revision')
task = u.AssetImportTask()
task.set_editor_property('filename',asset['fbx'])
task.set_editor_property('destination_path',job['destination'])
task.set_editor_property('automated',True)
task.set_editor_property('replace_existing',False)
task.set_editor_property('save',True)
options = u.FbxImportUI()
options.set_editor_property('automated_import_should_detect_type',False)
options.set_editor_property('mesh_type_to_import',u.FBXImportType.FBXIT_SKELETAL_MESH if asset['rigged'] else u.FBXImportType.FBXIT_STATIC_MESH)
options.set_editor_property('import_as_skeletal',asset['rigged'])
options.set_editor_property('import_animations',asset['rigged'] and asset['animation_actions']>0)
options.set_editor_property('import_materials',True)
options.set_editor_property('import_textures',True)
task.set_editor_property('options',options)
task.set_editor_property('factory',u.FbxFactory())
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
paths = list(task.get_editor_property('imported_object_paths'))
if not paths:
    raise RuntimeError('Unreal produced no imported objects')
types = {path: u.load_asset(path).get_class().get_name() for path in paths}
if asset['rigged'] and 'SkeletalMesh' not in types.values():
    raise RuntimeError('Rigged input did not produce a skeletal mesh; inspect the import before use')
if not u.EditorAssetLibrary.save_directory(job['destination'],only_if_is_dirty=False,recursive=True):
    raise RuntimeError('Failed to save imported assets and their generated skeleton/animations')
paths=list(u.EditorAssetLibrary.list_assets(job['destination'],recursive=True,include_folder=False))
types={path:u.load_asset(path).get_class().get_name() for path in paths}
(manifest_path.parent/'unreal-import.json').write_text(json.dumps({'objects':paths,'types':types},indent=2))
u.log('ASSET_IMPORT_SUCCESS '+str(paths))
