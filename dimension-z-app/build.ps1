$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$javaBin = 'C:\Program Files\Android\Android Studio\jbr\bin'
$buildTools = Join-Path (Split-Path $PSScriptRoot -Parent) 'portal-z\tools\build'
$platformJar = Join-Path (Split-Path $PSScriptRoot -Parent) 'portal-z\tools\sdk-platform\android-35\android.jar'
function Run-Checked([string]$program, [string[]]$arguments) {
    & $program @arguments
    if ($LASTEXITCODE -ne 0) { throw "Build command failed: $program" }
}
New-Item -ItemType Directory -Path 'build\classes','build\gen','build\dex','downloads','tools' -Force | Out-Null
Run-Checked "$buildTools\aapt.exe" @('package','-f','-M','app\src\main\AndroidManifest.xml','-S','app\src\main\res','-I',$platformJar,'-F','build\resources.apk','-J','build\gen','--version-code','1','--version-name','0.1.0-prototype')
$sources = @(Get-ChildItem 'app\src\main\java','build\gen' -Filter '*.java' -Recurse | ForEach-Object FullName)
Run-Checked "$javaBin\javac.exe" (@('--release','8','-encoding','UTF-8','-classpath',$platformJar,'-d','build\classes')+$sources)
Run-Checked "$javaBin\jar.exe" @('cf','build\classes.jar','-C','build\classes','.')
Run-Checked "$javaBin\java.exe" @('-cp',"$buildTools\lib\d8.jar",'com.android.tools.r8.D8','--lib',$platformJar,'--min-api','26','--output','build\dex','build\classes.jar')
Copy-Item -LiteralPath 'build\resources.apk' -Destination 'build\unsigned.apk' -Force
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$apkZip = [IO.Compression.ZipFile]::Open((Join-Path $PSScriptRoot 'build\unsigned.apk'),[IO.Compression.ZipArchiveMode]::Update)
[IO.Compression.ZipFileExtensions]::CreateEntryFromFile($apkZip,(Join-Path $PSScriptRoot 'build\dex\classes.dex'),'classes.dex') | Out-Null
$apkZip.Dispose()
Run-Checked "$buildTools\zipalign.exe" @('-f','-p','4','build\unsigned.apk','build\aligned.apk')
if (-not (Test-Path 'tools\prototype.keystore')) {
    Run-Checked "$javaBin\keytool.exe" @('-genkeypair','-keystore','tools\prototype.keystore','-alias','dimension-z-prototype','-storepass','android','-keypass','android','-keyalg','RSA','-keysize','2048','-validity','3650','-dname','CN=Dimension Z Prototype, O=Dimension Z, C=IN')
}
Run-Checked "$javaBin\java.exe" @('-jar',"$buildTools\lib\apksigner.jar",'sign','--ks','tools\prototype.keystore','--ks-key-alias','dimension-z-prototype','--ks-pass','pass:android','--key-pass','pass:android','--out','downloads\dimension-z-0.1.0-prototype.apk','build\aligned.apk')
Run-Checked "$javaBin\java.exe" @('-jar',"$buildTools\lib\apksigner.jar",'verify','--verbose','--print-certs','downloads\dimension-z-0.1.0-prototype.apk')
Get-FileHash 'downloads\dimension-z-0.1.0-prototype.apk' -Algorithm SHA256 | Format-List

