# Dimension Z asset bridge

Shared local workflow for Delta Bots and Resonance. Tripo CLI owns authentication;
no API key is stored in the game, Blender scripts, Unreal Content or this directory.
Node LTS and Tripo CLI 0.7.2 are installed under the workspace's work/asset-tools.

## Tripo

Run `./AssetPipeline/tripo.ps1 doctor` to check login, network and balance.
Run generation commands with `--dry-run --json` first. Review the input, exact
model, each processing step and credit estimate before submitting a paid task.
The CLI uses API credits, separately from Studio subscription credits. Current
paid Studio accounts may claim a one-time benefit at https://studio.tripo3d.ai/cli.

Generation output includes model.glb, preview.png and task.json. Inspect the preview
and preserve task.json for provenance. The CLI can also download an existing API
task without regenerating it. Studio links may not resolve under an API account;
export those models normally from Studio and use the exported GLB/FBX locally.

## Blender preparation

Use the Unreal-bundled Python or another Python 3:

```powershell
python AssetPipeline/asset_bridge.py status
python AssetPipeline/asset_bridge.py prepare --project deltabots --input MODEL.glb --name Bot_v01 --height 1.8
python AssetPipeline/asset_bridge.py import --project deltabots --name Bot_v01
```

Preparation runs a separate Blender process, preserves rigs/actions, centers the
asset at its feet and optionally sets height in metres. It saves the source,
editable .blend, GLB, FBX and an audit manifest under assets/PROJECT/NAME.
It flags triangle-budget overshoots rather than automatically damaging rigged
topology. It does not retarget skeletons or automatically place models in a level.
Texture embedding and animation playback need review on each actual source asset.

Unreal imports into a new /Game/DeltaBots/Generated/NAME folder, refuses to overwrite
existing folders and records imported object paths. Close the editor before
commandlet imports to avoid conflicting edits, then reopen or refresh Content.
Review scale, materials, skeleton, collisions, LODs and clips before gameplay use.

The Resonance target is reserved but disabled until its actual project path is
provided in pipeline.json. Existing Delta Bots levels and pawn assignments are
not replaced by an import. No account-wide MCP server or network listener is
needed: Blender bpy and Unreal Python perform local asset operations.

Public Git publishing is a separate review step. Check input ownership and output
rights; never publish Epic/Fab library source assets or credentials automatically.
