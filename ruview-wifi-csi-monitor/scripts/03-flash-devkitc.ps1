param(
    [Parameter(Mandatory = $true)]
    [string]$Port
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$build = Join-Path $projectRoot 'vendor\RuView\firmware\esp32-csi-node\build'
$required = @(
    (Join-Path $build 'bootloader\bootloader.bin'),
    (Join-Path $build 'partition_table\partition-table.bin'),
    (Join-Path $build 'ota_data_initial.bin'),
    (Join-Path $build 'esp32-csi-node.bin')
)
foreach ($file in $required) {
    if (-not (Test-Path -LiteralPath $file)) { throw "Missing build output: $file" }
}

python -m esptool --chip esp32s3 --port $Port --baud 460800 `
    write_flash --flash_mode dio --flash_size 8MB `
    0x0 $required[0] `
    0x8000 $required[1] `
    0xf000 $required[2] `
    0x20000 $required[3]

if ($LASTEXITCODE -ne 0) { throw "Flash failed: exit $LASTEXITCODE" }
Write-Host 'Flash completed. Continue with provision.py in the tutorial.'
