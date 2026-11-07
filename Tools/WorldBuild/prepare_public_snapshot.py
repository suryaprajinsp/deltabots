"""Copy an explicit reviewed allowlist into the isolated public Git worktree."""
from pathlib import Path
import hashlib
import json
import shutil

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / 'Saved/PublicGit'
FILES = [
    '.gitattributes',
    'Tools/WorldBuild/build_frontier_preview.py',
    'Tools/WorldBuild/build_scarab_territory.py',
    'Tools/WorldBuild/prepare_public_snapshot.py',
]
for folder in ('ArtSource/DeltaFrontier', 'ArtSource/ThirdParty/Fab/SciFiTurret', 'Plugins/DeltaPhoneModes'):
    for path in (ROOT / folder).rglob('*'):
        if not path.is_file() or any(p in {'Binaries', 'Intermediate', '__pycache__'} for p in path.parts):
            continue
        # Publish original concept sources/renders, not internal review reports.
        if folder == 'ArtSource/DeltaFrontier' and path.suffix.lower() not in {'.blend', '.png'}:
            continue
        FILES.append(path.relative_to(ROOT).as_posix())

def main():
    if not (DEST / '.git').is_file():
        raise RuntimeError('Create the isolated public worktree first.')
    manifest = []
    for relative in sorted(set(FILES)):
        source, target = ROOT / relative, DEST / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        manifest.append({'path': relative, 'sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                         'basis': 'CC-BY-4.0; attribution in adjacent license.txt' if '/ThirdParty/' in relative
                         else 'Original project source or generated concept artwork'})
    (DEST / '.gitignore').write_text('Binaries/\nIntermediate/\nSaved/\nDerivedDataCache/\n__pycache__/\n*.pyc\n*.blend1\n.env*\n', encoding='utf-8')
    (DEST / 'PUBLIC_ASSETS.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    (DEST / 'README.md').write_text('''# Delta Bots — public development sources

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
''', encoding='utf-8')
    print(f'Prepared {len(manifest)} allowlisted files plus public documentation.')

if __name__ == '__main__':
    main()
