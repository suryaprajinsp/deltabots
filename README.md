# Delta Bots — public development sources

Original Dimension Z / Delta Bots concept models, procedural Blender scene and
rig builders, and the custom Android phone-mode plugin source.

This is a curated source snapshot, not the complete playable Unreal project.
Unreal Content packages, Epic/Fab library packs without confirmed source
redistribution permission, local account metadata, build outputs and connection
settings are excluded. The private local project history is not published.

## Included artwork

`ArtSource/DeltaFrontier` contains original early concept scenes, a Delta Bot rig,
Scarab Basin and review renders. These are blockout concepts, not final characters.
The two Blender scripts under `Tools/WorldBuild` regenerate the original concepts.

`ArtSource/ThirdParty/Fab/SciFiTurret` contains the separately licensed **Sci fi
turret** by **NavXGen game assets**, under **CC BY 4.0**. Its original attribution,
source URL and license link are preserved in that folder's `license.txt`.
This asset's license does not apply to original Delta Bots work.

## Reuse and status

No blanket open-source license is granted for original Delta Bots code, characters
or artwork; rights remain with their respective owners. Separate third-party
licenses apply only to the files they identify.

Binary model and texture files use Git LFS. Clone with Git LFS installed and run
`git lfs pull` to retrieve them. `PUBLIC_ASSETS.json` records the reviewed files
and hashes. Plugin source is experimental; physical phone/controller and Unreal
integration testing are still required. No new gameplay or device test is implied
by publishing this snapshot.
