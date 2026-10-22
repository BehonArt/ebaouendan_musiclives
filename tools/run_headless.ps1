# Run a game's recompiled build headless from a fresh copy of the real save,
# then print the runtime's log lines. Taps are lower-screen coordinates.
#
#   tools\run_headless.ps1 -Game OE2 -VBlanks 2600 -Script "1850:tap:128:150" -Shots "2200,2600"
param(
    [ValidateSet("EBA", "OE1", "OE2")] [string]$Game = "EBA",
    [int]$VBlanks = 1800,
    [string]$Script = "",
    [string]$Shots = "",
    [int]$ShotEvery = 0,
    [string]$Out = "",
    [switch]$Wav
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
$saves = "F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\original_roms"
$save = @{
    EBA = "Elite Beat Agents (USA).sav"
    OE1 = "Osu! Tatakae! Ouendan (Japan).sav"
    OE2 = "Moero! Nekketsu Rhythm Damashii - Osu! Tatakae! Ouendan 2 (Japan).sav"
}[$Game]
$tree = @{ EBA = "build\recomp"; OE1 = "build\oe1\recomp"; OE2 = "build\oe2\recomp" }[$Game]
if (-not $Out) { $Out = "build\" + $Game.ToLower() + "_run" }
$Out = Join-Path $root $Out
Remove-Item -Recurse -Force $Out -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $Out | Out-Null
Copy-Item (Join-Path $saves $save) (Join-Path $Out "$Game.sav")

$cmdArgs = @((Join-Path $root "roms\$Game.nds"), "--out", $Out, "--save", (Join-Path $Out "$Game.sav"), "--vblanks", $VBlanks)
if ($Script) { $cmdArgs += @("--input", $Script) }
if ($Shots) { $cmdArgs += @("--shots", $Shots) }
if ($ShotEvery) { $cmdArgs += @("--shot-every", $ShotEvery) }
if ($Wav) { $cmdArgs += @("--wav", (Join-Path $Out "a.wav")) }
$exe = Join-Path $root "$tree\b\ndsrun.exe"
$log = Join-Path $Out "ndsrun.log"
$p = Start-Process -FilePath $exe -ArgumentList ($cmdArgs | ForEach-Object { '"' + $_ + '"' }) -NoNewWindow -Wait -PassThru `
    -RedirectStandardError $log -RedirectStandardOutput (Join-Path $Out "stdout.txt")
"exit $($p.ExitCode)"
Get-Content $log | Where-Object { $_ -match "^\[" }
