# The Customs Studio tester kit: the chart editor (Customs Studio), the public
# toolkit it runs, and the three games' development builds (Customs mode on,
# unlike the release's), in one folder that works from anywhere. The tester
# brings their own cartridge dumps (play\EBA.nds, play_oe1\OE1.nds,
# play_oe2\OE2.nds); nothing of the games is in the kit.
#
#   tools\make_studio_kit.ps1 [-Version 1.0.0-rc1] [-Toolkit "F:\...\rom_hacking\tools"] [-SkipStudioBuild]
#
# Steps: the toolkit's own make_release.py stages its public tree (an allowlist,
# verified: no ROM-derived files); studio\build_studio.ps1 builds the Studio
# exe (PyInstaller; -SkipStudioBuild reuses studio\dist); the games' dev
# ndsrun.exe builds go in as EBA.exe/OE1.exe/OE2.exe with the DLLs (the start
# save Customs plays from, the game makes itself on the first Play); the guide
# docs\CUSTOMS_STUDIO_TESTER.md is the kit's README. Then the zip:
# dist\eba-recomp-customs-studio-<version>-windows-x64.zip
param(
    [string]$Version = "1.0.0-rc1",
    [string]$Toolkit = "F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\tools",
    [switch]$SkipStudioBuild
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$name = "eba-recomp-customs-studio-$Version"
$kit = Join-Path $root "build\studio_kit\$name"
$ucrt = "C:\msys64\ucrt64"
if (Test-Path $kit) { Remove-Item -Recurse -Force $kit }
New-Item -ItemType Directory -Force $kit | Out-Null

Write-Host "toolkit (public tree)"
py -3 (Join-Path $Toolkit "make_release.py") --out (Join-Path $kit "toolkit")
if ($LASTEXITCODE -ne 0) { throw "the toolkit's release verifier failed: nothing packaged" }

if (-not $SkipStudioBuild) {
    Write-Host "Customs Studio (PyInstaller)"
    & (Join-Path $root "studio\build_studio.ps1") -Toolkit $Toolkit
}
$studio = Join-Path $root "studio\dist\Customs Studio"
if (-not (Test-Path (Join-Path $studio "Customs Studio.exe"))) { throw "no Customs Studio.exe in $studio" }
robocopy $studio (Join-Path $kit "Customs Studio") /E /NJH /NJS /NFL /NDL /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed on the Studio" }
# its settings: the toolkit and the games, relative to the Studio's folder (studio.py resolves them)
$ini = "# Customs Studio: where the toolkit's tools folder and the games (play, play_oe1, play_oe2) are`r`n" +
       "# (relative to this folder; or full paths)`r`n[paths]`r`ntoolkit = ..\toolkit\rom_hacking\tools`r`nrecomp = ..`r`n"
[IO.File]::WriteAllText((Join-Path $kit "Customs Studio\studio.ini"), $ini)   # no BOM

$games = @(
    @{ id = "EBA"; folder = "play";     tree = "build\recomp";     title = "Elite Beat Agents" },
    @{ id = "OE1"; folder = "play_oe1"; tree = "build\oe1\recomp"; title = "Osu! Tatakae! Ouendan" },
    @{ id = "OE2"; folder = "play_oe2"; tree = "build\oe2\recomp"; title = "Osu! Tatakae! Ouendan 2" }
)
foreach ($g in $games) {
    $d = Join-Path $kit $g.folder
    New-Item -ItemType Directory -Force (Join-Path $d "customs") | Out-Null
    $exe = Join-Path $root "$($g.tree)\b\ndsrun.exe"
    if (Select-String -Path (Join-Path $root "$($g.tree)\b\CMakeCache.txt") -Pattern "EBA_RELEASE:BOOL=ON" -Quiet) {
        throw "$($g.tree) is a release build (no Customs): rebuild it in development mode first"
    }
    Copy-Item $exe (Join-Path $d "$($g.id).exe")
    foreach ($dll in "SDL3.dll", "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll", "libiconv-2.dll") {
        Copy-Item (Join-Path "$ucrt\bin" $dll) $d
    }
    # (no start save shipped: the game makes customs\pristine.sav itself on the first Play, from its own first boot)
    $id = $g.id
    $bat = "@echo off`r`n" +
           "rem $($g.title), recompiled (development build: Customs mode on).`r`n" +
           "rem Needs your own cartridge dump beside it, named $id.nds. Progress: $id.sav here; the log of the last run: $id.log.`r`n" +
           "cd /d `"%~dp0`"`r`n" +
           "if not exist `"%~dp0$id.nds`" (echo Put your dump of the game beside this file, named $id.nds & pause & exit /b 1)`r`n" +
           "`"%~dp0$id.exe`" `"%~dp0$id.nds`" --window --save `"%~dp0$id.sav`" --out `"%~dp0.`" > `"%~dp0$id.log`" 2>&1`r`n"
    [IO.File]::WriteAllText((Join-Path $d "Play $id.bat"), $bat)
}
Copy-Item (Join-Path $root "docs\CUSTOMS_STUDIO_TESTER.md") (Join-Path $kit "README.md")
Copy-Item (Join-Path $root "LICENSE") (Join-Path $kit "LICENSE.txt")
Copy-Item (Join-Path $root "docs\THIRD_PARTY.txt") (Join-Path $kit "THIRD_PARTY.txt")
# nothing of the games: no ROM, no extracted data (the toolkit's verifier checked its tree; the rest here)
$bad = Get-ChildItem $kit -Recurse -File | Where-Object { $_.Extension -in ".nds", ".seq_", ".oml_", ".sdat", ".strm", ".wav", ".mp3" }
if ($bad) { throw "game-derived files in the kit: $($bad.FullName -join ', ')" }

$zip = Join-Path $root "dist\$name-windows-x64.zip"
New-Item -ItemType Directory -Force (Join-Path $root "dist") | Out-Null
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $kit -DestinationPath $zip -CompressionLevel Optimal
"kit: $kit"
"zip: $zip ($([math]::Round((Get-Item $zip).Length / 1MB, 1)) MB)"
