param (
    [string]$Configuration = "Debug"
)

scripts\USDDevShell.ps1 -Configuration "$Configuration"
msbuild -m build/win64/rain.sln /p:Configuration=$Configuration /NoWarn:MSB8065
