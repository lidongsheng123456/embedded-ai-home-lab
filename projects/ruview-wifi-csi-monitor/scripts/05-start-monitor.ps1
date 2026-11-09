param(
    [int]$HttpPort = 8080,
    [int]$UdpPort = 5005,
    [switch]$NoBrowser
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$server = Join-Path $projectRoot 'server\monitor-server.js'
$runtime = Join-Path $projectRoot '.runtime'
$stdout = Join-Path $runtime 'monitor.stdout.log'
$stderr = Join-Path $runtime 'monitor.stderr.log'

if (-not (Test-Path -LiteralPath $server)) { throw "Monitor server not found: $server" }
$node = (Get-Command node.exe -ErrorAction Stop).Source

New-Item -ItemType Directory -Force -Path $runtime | Out-Null
Get-CimInstance Win32_Process -Filter "Name = 'node.exe'" |
    Where-Object { $_.CommandLine -like '*monitor-server.js*' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force }

Start-Sleep -Milliseconds 300
$udpOwner = Get-NetUDPEndpoint -LocalPort $UdpPort -ErrorAction SilentlyContinue | Select-Object -First 1
if ($udpOwner) {
    $process = Get-CimInstance Win32_Process -Filter "ProcessId = $($udpOwner.OwningProcess)"
    throw "UDP $UdpPort is already occupied by PID $($udpOwner.OwningProcess): $($process.CommandLine)"
}

$env:RUVIEW_HTTP_PORT = [string]$HttpPort
$env:RUVIEW_UDP_PORT = [string]$UdpPort
Start-Process -FilePath $node `
    -ArgumentList @($server) `
    -WorkingDirectory $projectRoot `
    -WindowStyle Hidden `
    -RedirectStandardOutput $stdout `
    -RedirectStandardError $stderr

$url = "http://127.0.0.1:$HttpPort"
for ($attempt = 0; $attempt -lt 30; $attempt++) {
    try {
        Invoke-WebRequest -UseBasicParsing -Uri "$url/healthz" -TimeoutSec 1 | Out-Null
        if (-not $NoBrowser) { Start-Process $url }
        Write-Host "RuView monitor started: $url"
        exit 0
    }
    catch { Start-Sleep -Milliseconds 200 }
}

throw "Monitor did not start. Check $stderr"
