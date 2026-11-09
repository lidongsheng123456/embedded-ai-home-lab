param(
    [Parameter(Mandatory = $true)]
    [string]$Subnet
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$v2Path = Join-Path $projectRoot 'vendor\RuView\v2'

if (-not (Test-Path -LiteralPath $v2Path)) {
    throw 'RuView source not found. Run scripts\01-clone-ruview.ps1 first.'
}

Push-Location $v2Path
try {
    cargo run -p wifi-densepose-sensing-server --release -- `
        --source esp32 `
        --udp-port 5005 `
        --udp-bind 0.0.0.0 `
        --udp-allow $Subnet `
        --http-port 3000 `
        --ws-port 3001 `
        --ui-path ..\ui
} finally {
    Pop-Location
}
