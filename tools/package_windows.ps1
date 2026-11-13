[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version,
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [string]$MingwBin
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ([string]::IsNullOrWhiteSpace($MingwBin)) {
    $windeployqtCommand = Get-Command 'windeployqt-qt5.exe' -ErrorAction Stop
    $MingwBin = Split-Path -Path $windeployqtCommand.Source -Parent
}

$distDirectory = Join-Path $RepositoryRoot 'dist'
$runtimeDirectory = Join-Path $distDirectory 'windows-runtime'
$serverDirectory = Join-Path $distDirectory 'windows-server-staging'
$objdump = Join-Path $MingwBin 'objdump.exe'
$windeployqt = Join-Path $MingwBin 'windeployqt-qt5.exe'

function Assert-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required build/package file not found: $Path"
    }
}

function Copy-ApplicationFiles([string]$Destination, [bool]$IncludeGui) {
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null

    Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'src/nud.exe') -Destination $Destination
    Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'update_windows.bat') -Destination $Destination
    Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'COPYING') -Destination $Destination
    Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'doc/README_windows.txt') -Destination $Destination

    if ($IncludeGui) {
        Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'release/nu.exe') -Destination $Destination
    }

    Copy-Item -LiteralPath (Join-Path $MingwBin '..\etc\ssl\cert.pem') `
        -Destination (Join-Path $Destination 'cacert.pem')
}

function Get-ImportedDllNames([string]$BinaryPath) {
    $output = & $objdump -p $BinaryPath 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "objdump could not inspect $BinaryPath"
    }

    foreach ($line in $output) {
        if ($line -match '^\s*DLL Name:\s*(\S+)\s*$') {
            $Matches[1]
        }
    }
}

function Add-RequiredDlls([string]$StageDirectory, [string[]]$EntryPoints) {
    $queue = [System.Collections.Generic.Queue[string]]::new()
    foreach ($entryPoint in $EntryPoints) {
        $queue.Enqueue($entryPoint)
    }
    foreach ($dll in Get-ChildItem -LiteralPath $StageDirectory -Filter '*.dll' -File -Recurse) {
        $queue.Enqueue($dll.FullName)
    }

    $visited = @{}
    while ($queue.Count -gt 0) {
        $binary = $queue.Dequeue()
        $key = $binary.ToLowerInvariant()
        if ($visited.ContainsKey($key)) {
            continue
        }
        $visited[$key] = $true

        foreach ($dllName in Get-ImportedDllNames $binary) {
            if ($dllName -match '^(?i:api-ms-win-|ext-ms-win-)') {
                continue
            }

            $alreadyStaged = Get-ChildItem -LiteralPath $StageDirectory -Filter $dllName -File -Recurse |
                Select-Object -First 1
            if ($alreadyStaged) {
                $queue.Enqueue($alreadyStaged.FullName)
                continue
            }

            $systemDll = Join-Path (Join-Path $env:WINDIR 'System32') $dllName
            if (Test-Path -LiteralPath $systemDll -PathType Leaf) {
                continue
            }

            $sourceDll = Join-Path $MingwBin $dllName
            if (-not (Test-Path -LiteralPath $sourceDll -PathType Leaf)) {
                throw "Could not locate runtime dependency '$dllName' imported by '$binary'"
            }

            $destinationDll = Join-Path $StageDirectory $dllName
            Copy-Item -LiteralPath $sourceDll -Destination $destinationDll
            $queue.Enqueue($destinationDll)
        }
    }
}

foreach ($required in @(
    (Join-Path $RepositoryRoot 'release/nu.exe'),
    (Join-Path $RepositoryRoot 'src/nud.exe'),
    (Join-Path $RepositoryRoot 'update_windows.bat'),
    (Join-Path $MingwBin 'objdump.exe'),
    (Join-Path $MingwBin 'windeployqt-qt5.exe'),
    (Join-Path $MingwBin '..\etc\ssl\cert.pem')
)) {
    Assert-File $required
}

$projectVersion = (Select-String -Path (Join-Path $RepositoryRoot 'nu.pro') -Pattern '^VERSION\s*=\s*(\S+)').Matches.Groups[1].Value
if ($projectVersion -ne $Version) {
    throw "Tag version $Version does not match nu.pro version $projectVersion"
}

New-Item -ItemType Directory -Path $distDirectory -Force | Out-Null
foreach ($generatedDirectory in @($runtimeDirectory, $serverDirectory)) {
    if (Test-Path -LiteralPath $generatedDirectory) {
        Remove-Item -LiteralPath $generatedDirectory -Recurse -Force
    }
}

Copy-ApplicationFiles $runtimeDirectory $true
$env:PATH = "$MingwBin;$env:PATH"
& $windeployqt --release --compiler-runtime --no-translations --no-system-d3d-compiler --no-opengl-sw `
    (Join-Path $runtimeDirectory 'nu.exe')
if ($LASTEXITCODE -ne 0) {
    throw 'windeployqt failed to stage the Qt runtime'
}

Add-RequiredDlls $runtimeDirectory @(
    (Join-Path $runtimeDirectory 'nu.exe'),
    (Join-Path $runtimeDirectory 'nud.exe')
)

Copy-ApplicationFiles $serverDirectory $false
Add-RequiredDlls $serverDirectory @((Join-Path $serverDirectory 'nud.exe'))

$runtimeZip = Join-Path $distDirectory "Nu-$Version-windows-x64-runtime.zip"
$serverZip = Join-Path $distDirectory "Nu-$Version-windows-x64-server.zip"
foreach ($archive in @($runtimeZip, $serverZip)) {
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
}

Compress-Archive -Path (Join-Path $runtimeDirectory '*') -DestinationPath $runtimeZip -CompressionLevel Optimal
Compress-Archive -Path (Join-Path $serverDirectory '*') -DestinationPath $serverZip -CompressionLevel Optimal

Remove-Item -LiteralPath $serverDirectory -Recurse -Force
Write-Output "Created $runtimeZip"
Write-Output "Created $serverZip"
