$vsDevCmd = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat'

if (-not (Test-Path -LiteralPath $vsDevCmd)) {
    throw "Visual Studio developer command script was not found: $vsDevCmd"
}

cmd /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure"
