param([string]$Toolchain = "$env:USERPROFILE\.platformio\packages\toolchain-xtensa-esp32\bin")
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$output = Join-Path $project '.build\embedded'
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = Join-Path $project 'embedded\src'
$flags = @('-Os', '-ffunction-sections', '-fdata-sections', '-DLUA_INT_TYPE=LUA_INT_INT', '-I', $source, '-I', (Join-Path $source 'lua'))
$objects = @()
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $source 'lua') -Filter '*.c') {
    $object = Join-Path $output ($file.BaseName + '.o')
    & (Join-Path $Toolchain 'xtensa-esp32-elf-gcc.exe') @flags '-std=c99' '-c' $file.FullName '-o' $object
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $($file.Name)" }
    $objects += $object
}
foreach ($cpp in Get-ChildItem -LiteralPath $source -Filter '*.cpp') {
$runtimeObject = Join-Path $output ($cpp.BaseName + '.o')
& (Join-Path $Toolchain 'xtensa-esp32-elf-g++.exe') @flags '-std=gnu++11' '-fno-exceptions' '-fno-rtti' '-c' $cpp.FullName '-o' $runtimeObject
if ($LASTEXITCODE -ne 0) { throw 'ThemeRuntime compile failed' }
$objects += $runtimeObject
}
& (Join-Path $Toolchain 'xtensa-esp32-elf-ar.exe') 'rcs' (Join-Path $output 'libClockwiseTheme.a') @objects
if ($LASTEXITCODE -ne 0) { throw 'Archive failed' }
Write-Output "ESP32 compile OK: $($objects.Count) objects -> .build/embedded/libClockwiseTheme.a (no firmware upload)"
