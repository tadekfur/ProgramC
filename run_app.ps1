# Skrypt uruchamiający aplikację z poprawnymi ustawieniami kodowania
Write-Output "Uruchamianie aplikacji z poprawnymi ustawieniami kodowania..."

# Ustaw zmienne środowiskowe dla poprawnego kodowania
$env:PYTHONIOENCODING = "utf-8"
$env:PYTHONLEGACYWINDOWSSTDIO = "utf-8"

# Ścieżki do potencjalnych plików wykonywalnych
$exePaths = @(
    (Join-Path $PSScriptRoot "build\Debug\EtykietyManager.exe"),
    (Join-Path $PSScriptRoot "build\Release\EtykietyManager.exe"),
    (Join-Path $PSScriptRoot "build\Desktop_Qt_6_9_1_MinGW_64_bit-Debug\EtykietyManager.exe"),
    (Join-Path $PSScriptRoot "build\Desktop_Qt_6_9_1_MinGW_64_bit-Release\EtykietyManager.exe")
)

# Znajdź najnowszy plik wykonywalny
$latestExe = $null
$latestTime = [DateTime]::MinValue

foreach ($path in $exePaths) {
    if (Test-Path -Path $path) {
        $fileInfo = Get-Item -Path $path
        if ($fileInfo.LastWriteTime -gt $latestTime) {
            $latestExe = $path
            $latestTime = $fileInfo.LastWriteTime
        }
    }
}

if ($null -eq $latestExe) {
    Write-Error "Nie znaleziono pliku wykonywalnego aplikacji."
    exit 1
}

Write-Output "Znaleziono aplikację: $latestExe"
Write-Output "Data kompilacji: $latestTime"
Write-Output "Uruchamianie..."

# Uruchom aplikację
Start-Process -FilePath $latestExe
