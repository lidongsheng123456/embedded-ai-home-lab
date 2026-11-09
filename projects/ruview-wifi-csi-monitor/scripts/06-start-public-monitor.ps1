param(
    [int]$HttpPort = 8080,
    [int]$UdpPort = 5005
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $projectRoot '.runtime'
$stdout = Join-Path $runtime 'cloudflared.stdout.log'
$stderr = Join-Path $runtime 'cloudflared.stderr.log'
$pidFile = Join-Path $runtime 'cloudflared.pid'

New-Item -ItemType Directory -Force -Path $runtime | Out-Null

function Find-Cloudflared {
    $command = Get-Command cloudflared.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $wingetLink = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Links\cloudflared.exe'
    if (Test-Path -LiteralPath $wingetLink) { return $wingetLink }
    $packageRoot = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages'
    return Get-ChildItem -Path $packageRoot -Filter cloudflared.exe -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object FullName -like '*Cloudflare.cloudflared*' | Select-Object -First 1 -ExpandProperty FullName
}

$cloudflared = Find-Cloudflared
if (-not $cloudflared) {
    Write-Host 'Installing the official Cloudflare Tunnel client with WinGet (installer hash is verified by WinGet)...'
    & winget install --id Cloudflare.cloudflared --exact --source winget --scope user --silent `
        --accept-package-agreements --accept-source-agreements --disable-interactivity
    if ($LASTEXITCODE -ne 0) { throw "WinGet cloudflared installation failed with exit code $LASTEXITCODE." }
    $cloudflared = Find-Cloudflared
    if (-not $cloudflared) { throw 'cloudflared was installed but its executable could not be located.' }
}

if (Test-Path -LiteralPath $pidFile) {
    $oldPid = [int](Get-Content -LiteralPath $pidFile -ErrorAction SilentlyContinue)
    $oldProcess = Get-Process -Id $oldPid -ErrorAction SilentlyContinue
    if ($oldProcess -and $oldProcess.Path -eq $cloudflared) { Stop-Process -Id $oldPid -Force }
    Remove-Item -LiteralPath $pidFile -Force -ErrorAction SilentlyContinue
}

$alphabet = 'ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789!@#$%'
$random = [Security.Cryptography.RandomNumberGenerator]::Create()
$bytes = New-Object byte[] 20
$random.GetBytes($bytes)
$random.Dispose()
$password = -join ($bytes | ForEach-Object { $alphabet[$_ % $alphabet.Length] })
$env:RUVIEW_AUTH_USER = 'viewer'
$env:RUVIEW_AUTH_PASSWORD = $password

& (Join-Path $PSScriptRoot '05-start-monitor.ps1') -HttpPort $HttpPort -UdpPort $UdpPort -NoBrowser

Remove-Item -LiteralPath $stdout,$stderr -Force -ErrorAction SilentlyContinue
$tunnel = Start-Process -FilePath $cloudflared `
    -ArgumentList @('tunnel','--url',"http://127.0.0.1:$HttpPort",'--no-autoupdate') `
    -WorkingDirectory $projectRoot -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$tunnel.Id | Set-Content -LiteralPath $pidFile -Encoding ascii

$publicUrl = $null
for ($attempt = 0; $attempt -lt 60; $attempt++) {
    Start-Sleep -Milliseconds 500
    $log = ((Get-Content -LiteralPath $stdout,$stderr -Raw -ErrorAction SilentlyContinue) -join "`n")
    if ($log -match 'https://[a-z0-9-]+\.trycloudflare\.com') { $publicUrl = $Matches[0]; break }
    if ($tunnel.HasExited) { throw "Cloudflare Tunnel exited early. Check $stderr" }
}
if (-not $publicUrl) { throw "Timed out waiting for a public URL. Check $stderr" }

Write-Host ''
Write-Host 'Public monitor is ready (keep this computer and ESP32 powered on):' -ForegroundColor Green
Write-Host "URL:      $publicUrl"
Write-Host 'Username: viewer'
Write-Host "Password: $password"
Write-Host 'This temporary URL changes after the tunnel is restarted.' -ForegroundColor Yellow
Start-Process $publicUrl
