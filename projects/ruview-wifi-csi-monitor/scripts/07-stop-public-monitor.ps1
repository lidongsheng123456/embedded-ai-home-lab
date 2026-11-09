$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$pidFile = Join-Path $projectRoot '.runtime\cloudflared.pid'

if (Test-Path -LiteralPath $pidFile) {
    $tunnelPid = [int](Get-Content -LiteralPath $pidFile)
    $process = Get-Process -Id $tunnelPid -ErrorAction SilentlyContinue
    if ($process -and $process.Name -eq 'cloudflared') {
        Stop-Process -Id $tunnelPid -Force
        Write-Host 'Public tunnel stopped.'
    }
    Remove-Item -LiteralPath $pidFile -Force
} else {
    Write-Host 'No project public tunnel is running.'
}
