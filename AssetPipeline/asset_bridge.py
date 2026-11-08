"""Local, credential-free Blender to Unreal bridge. Tripo handles its own auth."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['prepare', 'import', 'status'])
    parser.add_argument('--project', choices=['deltabots', 'resonance'], default='deltabots')
    parser.add_argument('--input', type=Path)
    parser.add_argument('--name')
    parser.add_argument('--height', type=float, help='Optional intended height in metres; otherwise preserve scale')
    parser.add_argument('--triangles', type=int, default=15000)
    args = parser.parse_args()
    cfg = json.loads((ROOT/'pipeline.json').read_text())
    if args.action == 'status':
        print(json.dumps({'blender_installed': Path(cfg['blender']).is_file(),
            'unreal_installed': Path(cfg['unreal_editor']).is_file(),
            'projects': {k: bool(v['uproject'] and Path(v['uproject']).is_file()) for k,v in cfg['projects'].items()}}, indent=2))
        return
    if not args.name or not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,63}', args.name):
        parser.error('--name must be an asset name using letters, numbers and underscores')
    if args.triangles < 50 or (args.height is not None and not 0 < args.height <= 1000):
        parser.error('Invalid triangle budget or height')
    out = ROOT/'assets'/args.project/args.name
    if args.action == 'prepare':
        if not args.input or not args.input.is_file() or args.input.suffix.lower() not in {'.glb','.fbx'}:
            parser.error('--input must be an existing GLB or FBX')
        out.mkdir(parents=True, exist_ok=True)
        if (out/'asset.json').exists():
            parser.error('Asset already prepared; choose a new revision name to preserve its source')
        job = {'input': str(args.input.resolve()), 'output': str(out.resolve()),
               'name': args.name, 'height_m': args.height, 'triangle_budget': args.triangles,
               'project': args.project}
        job_path = out/'job.json'
        job_path.write_text(json.dumps(job,indent=2))
        subprocess.run([cfg['blender'],'--background','--factory-startup','--python-exit-code','1',
                        '--python',str(ROOT/'blender_prepare.py'),'--',str(job_path.resolve())], check=True)
    else:
        target = cfg['projects'][args.project]
        if not target['uproject'] or not Path(target['uproject']).is_file():
            parser.error('Project path is not configured; set it in pipeline.json before importing')
        if not (out/'asset.json').is_file():
            parser.error('Prepare and inspect the asset before importing')
        # A per-import commandlet file avoids passing unescaped Python through an exec string.
        job = {'manifest': str((out/'asset.json').resolve()), 'destination': target['content']+'/'+args.name}
        job_path = out/'unreal-job.json'; job_path.write_text(json.dumps(job,indent=2))
        runner = out/'run_import.py'
        runner.write_text('import runpy, sys\nsys.argv='+repr(['unreal_import.py',str(job_path.resolve())])+
                          '\nrunpy.run_path('+repr(str(ROOT/'unreal_import.py'))+',run_name="__main__")\n')
        # UE's commandlet parser splits some long -script paths at spaces even
        # when Windows quotes the argument. A short temporary runner avoids it.
        with tempfile.TemporaryDirectory(prefix='dzasset_') as temp:
            short_runner = Path(temp)/'import_asset.py'
            short_runner.write_text(runner.read_text())
            subprocess.run([cfg['unreal_editor'],target['uproject'],'-run=pythonscript',
                            '-script='+str(short_runner),'-unattended','-nullrhi','-nosplash','-stdout'],check=True)

if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as exc:
        sys.exit(exc.returncode)
