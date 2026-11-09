param(
    [string]$Port = 'COM4',
    [Parameter(Mandatory = $true)]
    [string]$Ssid,
    [Parameter(Mandatory = $true)]
    [string]$TargetIp,
    [int]$TargetPort = 5005,
    [int]$NodeId = 1
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$provision = Join-Path $projectRoot 'vendor\RuView\firmware\esp32-csi-node\provision.py'
$python = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\python.exe'

if (-not (Test-Path -LiteralPath $python)) { throw "PlatformIO Python not found: $python" }
if (-not (Test-Path -LiteralPath $provision)) { throw "Provisioning script not found: $provision" }

$password = Read-Host 'Wi-Fi password (hidden)' -AsSecureString
$passwordPtr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($password)
$passwordFile = Join-Path $env:TEMP ("ruview-wifi-{0}.txt" -f [guid]::NewGuid().ToString('N'))

try {
    $plainPassword = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($passwordPtr)
    $utf8WithoutBom = New-Object Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($passwordFile, $plainPassword, $utf8WithoutBom)
    icacls $passwordFile /inheritance:r /grant:r "$($env:USERNAME):(R,W)" | Out-Null

    $env:PYTHONIOENCODING = 'utf-8'
    & $python $provision `
        --port $Port `
        --chip esp32s3 `
        --ssid $Ssid `
        --password-file $passwordFile `
        --target-ip $TargetIp `
        --target-port $TargetPort `
        --node-id $NodeId `
        --reset

    if ($LASTEXITCODE -ne 0) { throw "Provisioning failed: exit $LASTEXITCODE" }
}
finally {
    if ($passwordPtr -ne [IntPtr]::Zero) {
        [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($passwordPtr)
    }
    $plainPassword = $null
    $password = $null
    if (Test-Path -LiteralPath $passwordFile) {
        Remove-Item -LiteralPath $passwordFile -Force
    }
}

Write-Host 'Provisioning completed; temporary password file removed.'
