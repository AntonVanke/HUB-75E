param(
    [Parameter(Mandatory=$true)][string]$Port,
    [int]$Baud = 460800,
    [string]$Python = "python"
)
$ErrorActionPreference = "Stop"
$image = Join-Path $PSScriptRoot "merged.bin"
& $Python -m esptool --chip esp32s3 --port $Port --baud $Baud --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size 8MB 0x0 $image
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
