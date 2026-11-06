param(
    [switch]$UseProxy,
    [string]$Ref = 'v0.8.17-esp32'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$vendorRoot = Join-Path $projectRoot 'vendor'
$repoPath = Join-Path $vendorRoot 'RuView'

if ($UseProxy) {
    $env:HTTP_PROXY = 'http://127.0.0.1:7892'
    $env:HTTPS_PROXY = 'http://127.0.0.1:7892'
}

if (Test-Path -LiteralPath $repoPath) {
    Write-Host "RuView already exists: $repoPath"
    Write-Host 'Run git pull inside it if you intentionally want to update.'
    exit 0
}

New-Item -ItemType Directory -Force -Path $vendorRoot | Out-Null
git clone --depth 1 --branch $Ref https://github.com/ruvnet/RuView.git $repoPath
Write-Host "Downloaded RuView $Ref to $repoPath"
