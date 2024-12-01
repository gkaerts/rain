param (
    [string]$Configuration = "Debug"
)

# Set up where to find Python (used for finding python libraries and includes)
# Should NOT point to our Python venv
$PythonPath = (Get-Command python).Path
$PythonDir = Split-Path $PythonPath -Parent
$env:RN_PYTHON_DIR = "$PythonDir"

build\python\venv\Scripts\Activate.ps1

# Point to our OpenUSD build for the current configuration
$cwd = (Get-Item .).Fullname
$OpenUSDDir = "$cwd\build\OpenUSD\$Configuration"
$env:Path = "$OpenUSDDir\bin;$OpenUSDDir\lib;$env:Path"
$env:PYTHONPATH = "$OpenUSDDir\lib\python\;$env:PYTHONPATH"

# Tell USD where to find our plugins
$env:PXR_PLUGINPATH_NAME = "$cwd\build\win64\$Configuration\gen\usd_plugins\rn\;$env:PXR_PLUGINPATH_NAME"