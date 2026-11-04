# A game's play folder: the exe (ndsrun), the DLLs it needs, the ROM, a save,
# its customs and a launcher, in one folder that runs from anywhere. Run again
# after a rebuild: it refreshes the exe, the DLLs and the customs' files, and
# never overwrites the save or the customs' scores.
#
#   tools\make_play_folder.ps1 [-Game EBA|OE1|OE2] [-Dest play]
param([ValidateSet("EBA", "OE1", "OE2")] [string]$Game = "EBA", [string]$Dest = "")
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
if (-not $Dest) { $Dest = if ($Game -eq "EBA") { "play" } else { "play_" + $Game.ToLower() } }
$tree = @{ EBA = "build\recomp"; OE1 = "build\oe1\recomp"; OE2 = "build\oe2\recomp" }[$Game]
$title = @{ EBA = "Elite Beat Agents"; OE1 = "Osu! Tatakae! Ouendan"; OE2 = "Moero! Nekketsu Rhythm Damashii - Osu! Tatakae! Ouendan 2" }[$Game]
$orig = "F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\original_roms\" + @{
    EBA = "Elite Beat Agents (USA).sav"
    OE1 = "Osu! Tatakae! Ouendan (Japan).sav"
    OE2 = "Moero! Nekketsu Rhythm Damashii - Osu! Tatakae! Ouendan 2 (Japan).sav"
}[$Game]
$folder = $Dest                         # (PowerShell: $dest is the same variable)
$dest = Join-Path $root $folder
New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item (Join-Path $root "$tree\b\ndsrun.exe") (Join-Path $dest "$Game.exe") -Force
foreach ($dll in "SDL3.dll", "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll", "libiconv-2.dll") {
    Copy-Item (Join-Path "C:\msys64\ucrt64\bin" $dll) $dest -Force
}
if (-not (Test-Path (Join-Path $dest "$Game.nds"))) { Copy-Item (Join-Path $root "roms\$Game.nds") (Join-Path $dest "$Game.nds") }
if (-not (Test-Path (Join-Path $dest "$Game.sav"))) {
    $play = Join-Path $root ("build\" + $(if ($Game -eq "EBA") { "" } else { $Game.ToLower() + "\" }) + "play\$Game.sav")
    Copy-Item $(if (Test-Path $play) { $play } else { $orig }) (Join-Path $dest "$Game.sav")
}
# the customs (customs\ for EBA, customs_oe2\ ...), beside the ROM where ndsrun looks for them
$customs = Join-Path $root $(if ($Game -eq "EBA") { "customs" } else { "customs_" + $Game.ToLower() })
if (Test-Path $customs) {
    $to = Join-Path $dest "customs"
    New-Item -ItemType Directory -Force $to | Out-Null
    Get-ChildItem $customs -File | Where-Object { $_.Name -notin "scores.ini", "session.sav" } | Copy-Item -Destination $to -Force
    if (-not (Test-Path (Join-Path $to "scores.ini")) -and (Test-Path (Join-Path $customs "scores.ini"))) {
        Copy-Item (Join-Path $customs "scores.ini") $to
    }
    foreach ($d in Get-ChildItem $customs -Directory) {
        robocopy $d.FullName (Join-Path $to $d.Name) /E /NJH /NJS /NFL /NDL /NP | Out-Null
        if ($LASTEXITCODE -ge 8) { throw "robocopy failed on $($d.Name)" }
    }
}
# no skins for 1.0 (eba.h kSkins): a folder's old copy goes
$old = Join-Path $dest "skins"
if (Test-Path $old) { Remove-Item -Recurse -Force $old }
$bat = @"
@echo off
rem $title, recompiled: the game in a window with the options bar.
rem Progress: $Game.sav here. Settings: %APPDATA%\eba-recomp\$Game\settings.ini.
rem Customs: customs\ here (their scores: customs\scores.ini). The log of the last run: $Game.log here.
cd /d "%~dp0"
"%~dp0$Game.exe" "%~dp0$Game.nds" --window --save "%~dp0$Game.sav" --out "%~dp0." > "%~dp0$Game.log" 2>&1
"@
# cmd.exe wants CRLF line ends in a batch file
[IO.File]::WriteAllText((Join-Path $dest "Play $Game.bat"), (($bat -replace "`r`n", "`n") -replace "`n", "`r`n") + "`r`n")
# and one at the repository's top that calls it
$top = @"
@echo off
rem $title, recompiled: plays from the $folder folder (tools\make_play_folder.ps1 makes it).
call "%~dp0$folder\Play $Game.bat"
"@
[IO.File]::WriteAllText((Join-Path $root "Play $Game.bat"), (($top -replace "`r`n", "`n") -replace "`n", "`r`n") + "`r`n")
"play folder ready: $dest"
Get-ChildItem $dest | Select-Object Name, Length
