# The release zip: Elite Beat Ouendan: Music LIVES, the three games and the
# launcher, for players who bring their own cartridge dumps.
#
#   tools\make_release.ps1 [-Version 1.0.0] [-NoBuild] [-KeepRelease]
#
# Builds the three games in release mode (cmake/eba.cmake's EBA_RELEASE: no
# Customs, RetroAchievements or saved replays; no console window) and the
# launcher, then stages dist\eba-recomp-v<version>\ and zips it to
# dist\eba-recomp-v<version>-windows-x64.zip:
#
#   eba-recomp.exe           the launcher (run this)
#   EBA.exe OE1.exe OE2.exe  the games
#   SDL3.dll libiconv-2.dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll
#   README.txt LICENSE THIRD_PARTY.txt licenses\
#
# Nothing of the games goes in: the script stops if a ROM or a save is staged.
# Afterwards the build trees are put back in development mode (only the port's
# own sources rebuild), unless -KeepRelease. -NoBuild packages what is built.
param([string]$Version = "1.0.0", [switch]$NoBuild, [switch]$KeepRelease)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$bash = "C:\msys64\usr\bin\bash.exe"
$ucrt = "C:\msys64\ucrt64"
$rootUnix = "/" + $root.Substring(0, 1).ToLower() + ($root.Substring(2) -replace '\\', '/')
$games = [ordered]@{ EBA = "build/recomp"; OE1 = "build/oe1/recomp"; OE2 = "build/oe2/recomp" }

function Build-Tree([string]$tree, [string]$release) {
    & $bash -lc "cd '$rootUnix' && EBA_RELEASE=$release EBA_VERSION=$Version RECOMP_DIR=$tree tools/build.sh"
    if ($LASTEXITCODE -ne 0) { throw "the build of $tree failed" }
}

$stage = Join-Path $root "dist\eba-recomp-v$Version"
$zip = Join-Path $root "dist\eba-recomp-v$Version-windows-x64.zip"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage, (Join-Path $stage "licenses") | Out-Null

if (-not $NoBuild) {
    foreach ($id in $games.Keys) { Write-Host "release build: $id"; Build-Tree $games[$id] "ON" }
    Write-Host "launcher"
    & $bash -lc "cd '$rootUnix' && EBA_VERSION=$Version tools/build_launcher.sh"
    if ($LASTEXITCODE -ne 0) { throw "the launcher's build failed" }
}
foreach ($id in $games.Keys) {
    Copy-Item (Join-Path $root ($games[$id] + "\b\ndsrun.exe")) (Join-Path $stage "$id.exe")
}
Copy-Item (Join-Path $root "build\launcher\eba-recomp.exe") $stage
if (-not $NoBuild -and -not $KeepRelease) {
    foreach ($id in $games.Keys) { Write-Host "back to development: $id"; Build-Tree $games[$id] "OFF" }
}

foreach ($dll in "SDL3.dll", "libiconv-2.dll", "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll") {
    Copy-Item (Join-Path $ucrt "bin\$dll") $stage
}
# (no skins or HD rings in 1.0: eba.h kSkins)
$readme = (Get-Content -Raw (Join-Path $root "docs\README_RELEASE.txt")) -replace "@VERSION@", "v$Version"
$readme = ($readme -replace "`r`n", "`n") -replace "`n", "`r`n"
[IO.File]::WriteAllText((Join-Path $stage "README.txt"), $readme)
Copy-Item (Join-Path $root "LICENSE") (Join-Path $stage "LICENSE.txt")
Copy-Item (Join-Path $root "docs\THIRD_PARTY.txt") $stage
$lic = Join-Path $stage "licenses"
Copy-Item (Join-Path $root "ndskit\LICENSE") "$lic\ndskit.txt"
Copy-Item (Join-Path $root "third_party\imgui\LICENSE.txt") "$lic\imgui.txt"
Copy-Item (Join-Path $root "third_party\rcheevos\LICENSE") "$lic\rcheevos.txt"
Copy-Item "$ucrt\share\licenses\sdl3\LICENSE.txt" "$lic\SDL3.txt"
# stb_image: its licence is at the end of the header
$stb = Get-Content -Raw (Join-Path $root "third_party\stb\stb_image.h")
[IO.File]::WriteAllText("$lic\stb_image.txt", $stb.Substring($stb.IndexOf("ALTERNATIVE A - MIT License") - 200))
$hdr = Join-Path $root "launcher\header.png"
if (Test-Path $hdr) { Copy-Item $hdr $stage }
Copy-Item "$ucrt\share\licenses\libiconv\COPYING.LIB" "$lic\libiconv.txt"
Get-Content "$ucrt\share\licenses\libgcc\COPYING.RUNTIME", "$ucrt\share\licenses\libgcc\COPYING3" | Set-Content "$lic\gcc-runtime.txt" -Encoding utf8
Copy-Item "$ucrt\share\licenses\libwinpthread\COPYING" "$lic\winpthreads.txt"

# nothing of the games
$bad = Get-ChildItem -Recurse $stage -File | Where-Object { $_.Extension -in ".nds", ".sav", ".dsv", ".bin" -or $_.Name -eq "portable.txt" }
if ($bad) { throw "not for the package: $($bad.FullName -join ', ')" }

if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path "$stage\*" -DestinationPath $zip -CompressionLevel Optimal
$size = (Get-Item $zip).Length / 1MB
$sha = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
Write-Host ("{0}  {1:N1} MB  sha256 {2}" -f $zip, $size, $sha)
Get-ChildItem $stage -Recurse -File | ForEach-Object { "{0,10:N0}  {1}" -f $_.Length, $_.FullName.Substring($stage.Length + 1) }
