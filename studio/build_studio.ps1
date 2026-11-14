# Customs Studio as an .exe (PyInstaller, one folder): studio\dist\Customs Studio\
# The toolkit stays .py on disk (studio.ini says where); its imports are bundled.
#
#   studio\build_studio.ps1 [-Toolkit "F:\...\rom_hacking\tools"]
# -DistDir: where it goes, under studio\ (make_release.ps1: dist_release, so an open development Studio is left alone)
param([string]$Toolkit = "F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\tools", [string]$DistDir = "dist")
$ErrorActionPreference = "Continue"
$here = $PSScriptRoot
py -3.13 -W ignore (Join-Path $here "make_deps.py") $Toolkit
Push-Location $here
try {
    py -3.13 -W ignore -m PyInstaller --noconfirm --clean --onedir --console --name "Customs Studio" `
        --add-data "$(Join-Path $here 'studio.js');." --hidden-import toolkit_deps --hidden-import tkinter --hidden-import tkinter.filedialog --hidden-import PIL.Image --hidden-import PIL.BmpImagePlugin --hidden-import PIL.PngImagePlugin --hidden-import PIL.JpegImagePlugin --hidden-import PIL.WebPImagePlugin --hidden-import PIL.GifImagePlugin --collect-submodules ndspy --collect-all unicorn --collect-all webview --collect-all clr_loader --collect-all pythonnet `
        --distpath (Join-Path $here $DistDir) --workpath (Join-Path $here "build_$DistDir") --specpath (Join-Path $here "build_$DistDir") `
        studio.py
} finally { Pop-Location }
$app = Join-Path $here "$DistDir\Customs Studio"
# as python.exe runs: no Control Flow Guard (unicorn's JIT), its stack (patch_exe.py)
py -3.13 (Join-Path $here "patch_exe.py") (Join-Path $app "Customs Studio.exe")
# where the toolkit and the recomp are, for this PC
$ini = "[paths]`r`ntoolkit = $Toolkit`r`nrecomp = $(Split-Path $here -Parent)`r`n"
[IO.File]::WriteAllText((Join-Path $app "studio.ini"), $ini)
"Customs Studio: $app"
