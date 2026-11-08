$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$nodeDirectory=Join-Path $taskRoot 'work\asset-tools\node-v26.13.0-win-x64'
$tripoCommand=Join-Path $taskRoot 'work\asset-tools\tripo\node_modules\.bin\tripo.cmd'
if (-not (Test-Path -LiteralPath $tripoCommand)) { throw 'Local Tripo CLI is not installed.' }
$env:Path=$nodeDirectory+';'+$env:Path
$env:TRIPO_CLIENT_APP='codex'
$env:TRIPO_DISTRIBUTION_CHANNEL='codex_plugin'
& $tripoCommand @args
exit $LASTEXITCODE
