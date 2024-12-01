scripts\USDDevShell.ps1

$vsWhere = "${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"   
$vsInstallationPath = & $vsWhere -latest -property installationPath
Write-Output "$vsInstallationPath"
Import-Module "$vsInstallationPath\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath $vsInstallationPath -SkipAutomaticLocation

Write-Output "Building USD: Monolithic Debug (This could take a while)"
python downloads\OpenUSD_24_11\OpenUSD-24.11\build_scripts\build_usd.py --generator "Visual Studio 17 2022" --build-variant debug --build-monolithic build\OpenUSD\Debug

Write-Output "Build USD: Monolithic RelWithDebugInfo (This could take a while)"
python downloads\OpenUSD_24_11\OpenUSD-24.11\build_scripts\build_usd.py --generator "Visual Studio 17 2022" --build-variant release --build-monolithic build\OpenUSD\Release
deactivate