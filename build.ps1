$vsDevCmd = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat'
$vcpkgRoot = Join-Path $PSScriptRoot 'third_party\vcpkg'

if (-not (Test-Path -LiteralPath $vsDevCmd)) {
    throw "Visual Studio developer command script was not found: $vsDevCmd"
}

if (-not (Test-Path -LiteralPath (Join-Path $vcpkgRoot 'vcpkg.exe'))) {
    git clone --filter=blob:none https://github.com/microsoft/vcpkg.git $vcpkgRoot
    Push-Location $vcpkgRoot
    git checkout f781d9387e4684783e69e136e2e124ff4660bffc
    .\bootstrap-vcpkg.bat -disableMetrics
    Pop-Location
}

cmd /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 && cmake -S . -B build --fresh -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows && cmake --build build && ctest --test-dir build --output-on-failure"
