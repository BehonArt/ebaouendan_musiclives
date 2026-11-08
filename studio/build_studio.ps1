# Customs Studio as an .exe (PyInstaller, one folder): studio\dist\Customs Studio\
# The toolkit stays .py on disk (studio.ini says where); its imports are bundled.
#
#   studio\build_studio.ps1 [-Toolkit "F:\...\rom_hacking\tools"]
param([string]$Toolkit = "F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\tools")
$ErrorActionPreference = "Continue"
$here = $PSScriptRoot
py -3 -W ignore (Join-Path $here "make_deps.py") $Toolkit
Push-Location $here
try {
    py -3 -W ignore -m PyInstaller --noconfirm --clean --onedir --console --name "Customs Studio" `
        --add-data "$(Join-Path $here 'studio.js');." --hidden-import toolkit_deps --hidden-import tkinter --hidden-import tkinter.filedialog --collect-submodules ndspy --collect-all unicorn --collect-all webview --collect-all clr_loader --collect-all pythonnet `
        --distpath (Join-Path $here "dist") --workpath (Join-Path $here "build") --specpath (Join-Path $here "build") `
        studio.py
} finally { Pop-Location }
$app = Join-Path $here "dist\Customs Studio"
# as python.exe runs: no Control Flow Guard (unicorn's JIT), its stack (patch_exe.py)
py -3 (Join-Path $here "patch_exe.py") (Join-Path $app "Customs Studio.exe")
# where the toolkit and the recomp are, for this PC
$ini = "[paths]`r`ntoolkit = $Toolkit`r`nrecomp = $(Split-Path $here -Parent)`r`n"
[IO.File]::WriteAllText((Join-Path $app "studio.ini"), $ini)
"Customs Studio: $app"
