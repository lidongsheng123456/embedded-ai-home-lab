$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$firmwarePath = Join-Path $projectRoot 'vendor\RuView\firmware\esp32-csi-node'

if (-not (Test-Path -LiteralPath $firmwarePath)) {
    throw 'RuView source not found. Run scripts\01-clone-ruview.ps1 first.'
}

docker info *> $null
if ($LASTEXITCODE -ne 0) {
    throw 'Docker Desktop is not running. Start Docker Desktop, wait until it reports Engine running, then retry.'
}
$mount = $firmwarePath.Replace('\', '/')
$command = "rm -rf build sdkconfig && idf.py -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.devkitc' set-target esp32s3 && idf.py -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.devkitc' build"

docker run --rm `
    -v "${mount}:/project" `
    -w /project `
    espressif/idf:v5.4 `
    bash -c $command

if ($LASTEXITCODE -ne 0) { throw "Firmware build failed: exit $LASTEXITCODE" }
Write-Host 'DevKitC firmware build completed.'
