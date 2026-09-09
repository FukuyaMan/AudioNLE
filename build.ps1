[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'

$vcpkgRoot = Join-Path $PSScriptRoot 'third_party\vcpkg'
$vcpkgConfigurationPath = Join-Path $PSScriptRoot 'vcpkg-configuration.json'
$buildDirectory = Join-Path $PSScriptRoot 'build'
$vst3HelperDirectory = Join-Path $buildDirectory 'vst3_helpers'

if (-not (Test-Path -LiteralPath $vcpkgConfigurationPath)) {
    throw "vcpkg configuration was not found: $vcpkgConfigurationPath"
}

$vcpkgConfiguration = Get-Content -Raw -LiteralPath $vcpkgConfigurationPath | ConvertFrom-Json
$vcpkgCommit = $vcpkgConfiguration.'default-registry'.baseline
if ([string]::IsNullOrWhiteSpace($vcpkgCommit)) {
    throw 'vcpkg-configuration.json does not declare a default-registry baseline.'
}

if (-not (Test-Path -LiteralPath $vcpkgRoot)) {
    git clone --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git $vcpkgRoot
    if ($LASTEXITCODE -ne 0) { throw 'Failed to clone repository-local vcpkg.' }
}

if (-not (Test-Path -LiteralPath (Join-Path $vcpkgRoot '.git'))) {
    throw "Repository-local vcpkg is not a Git checkout: $vcpkgRoot"
}

$currentVcpkgCommit = (git -C $vcpkgRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Failed to identify the repository-local vcpkg revision.' }

if ($currentVcpkgCommit -ne $vcpkgCommit) {
    $vcpkgChanges = git -C $vcpkgRoot status --porcelain
    if ($LASTEXITCODE -ne 0) { throw 'Failed to inspect the repository-local vcpkg checkout.' }
    if ($vcpkgChanges) {
        throw "Repository-local vcpkg has uncommitted changes and is not pinned to $vcpkgCommit."
    }

    git -C $vcpkgRoot fetch --depth=1 origin $vcpkgCommit
    if ($LASTEXITCODE -ne 0) { throw "Failed to fetch pinned vcpkg revision $vcpkgCommit." }
    git -C $vcpkgRoot checkout --detach $vcpkgCommit
    if ($LASTEXITCODE -ne 0) { throw "Failed to check out pinned vcpkg revision $vcpkgCommit." }
}

$vcpkgExe = Join-Path $vcpkgRoot 'vcpkg.exe'
$bootstrapStamp = Join-Path $vcpkgRoot '.audionle-vcpkg-bootstrap-commit'
$bootstrappedCommit = if (Test-Path -LiteralPath $bootstrapStamp) {
    (Get-Content -Raw -LiteralPath $bootstrapStamp).Trim()
} else {
    ''
}

if (-not (Test-Path -LiteralPath $vcpkgExe) -or $bootstrappedCommit -ne $vcpkgCommit) {
    Push-Location $vcpkgRoot
    try {
        .\bootstrap-vcpkg.bat -disableMetrics
        if ($LASTEXITCODE -ne 0) { throw 'Failed to bootstrap repository-local vcpkg.' }
        Set-Content -NoNewline -LiteralPath $bootstrapStamp -Value $vcpkgCommit
    } finally {
        Pop-Location
    }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio locator was not found: $vswhere"
}

$vsInstall = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1).Trim()
if ([string]::IsNullOrWhiteSpace($vsInstall)) {
    throw 'Visual Studio with the MSVC x64 tools was not found.'
}

$vsDevCmd = Join-Path $vsInstall 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path -LiteralPath $vsDevCmd)) {
    throw "Visual Studio developer command script was not found: $vsDevCmd"
}

$developerEnvironment = cmd.exe /c "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'Failed to initialize the Visual Studio x64 developer environment.' }
foreach ($entry in $developerEnvironment) {
    $separator = $entry.IndexOf('=')
    if ($separator -gt 0) {
        Set-Item -Path "Env:$($entry.Substring(0, $separator))" -Value $entry.Substring($separator + 1)
    }
}

if (Test-Path -LiteralPath $vst3HelperDirectory) {
    Remove-Item -LiteralPath $vst3HelperDirectory -Recurse -Force
}
& cmake -S $PSScriptRoot -B $buildDirectory --fresh -G Ninja "-DCMAKE_BUILD_TYPE=$Configuration" '-DCMAKE_POLICY_VERSION_MINIMUM=3.5' "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot\scripts\buildsystems\vcpkg.cmake" '-DVCPKG_TARGET_TRIPLET=x64-windows'
if ($LASTEXITCODE -ne 0) { throw 'CMake configure (including vcpkg manifest install) failed.' }

& cmake --build $buildDirectory --config $Configuration
if ($LASTEXITCODE -ne 0) { throw 'CMake build failed.' }

if (-not $SkipTests) {
    & ctest --test-dir $buildDirectory --output-on-failure --no-tests=error
    if ($LASTEXITCODE -ne 0) { throw 'CTest failed.' }
}
