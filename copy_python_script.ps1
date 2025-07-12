# Skrypt kopiowania pliku gus_wrapper.py do katalogu build

$buildPaths = @(
    "build\Desktop_Qt_6_9_1_MinGW_64_bit-Debug\python",
    "build\Debug\python"
)

# Upewnij się, że katalogi docelowe istnieją
foreach ($path in $buildPaths) {
    if (-not (Test-Path -Path $path)) {
        New-Item -Path $path -ItemType Directory -Force
        Write-Host "Utworzono katalog: $path"
    }
}

# Kopiuj plik do katalogów docelowych
foreach ($path in $buildPaths) {
    $targetFile = "$path\gus_wrapper.py"
    Copy-Item -Path "python\gus_wrapper.py" -Destination $targetFile -Force
    Write-Host "Skopiowano gus_wrapper.py do: $targetFile"
}

Write-Host "Gotowe - plik gus_wrapper.py został skopiowany do katalogów build."
