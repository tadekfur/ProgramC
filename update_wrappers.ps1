# Skrypt aktualizacji wrapperów Python w katalogach build
Write-Output "Aktualizacja wrapperow Python w katalogach build..."

# Lokalizacja plików źródłowych
$sourceDir = Join-Path $PSScriptRoot "python"
$simpleWrapperPath = Join-Path $sourceDir "gus_wrapper_simple.py"
$fullWrapperPath = Join-Path $sourceDir "gus_wrapper_full.py"
$regonAPIPath = Join-Path $PSScriptRoot "RegonAPI-1.3.1"

# Docelowe katalogi
$buildDirs = @(
    (Join-Path $PSScriptRoot "build\Debug\python"),
    (Join-Path $PSScriptRoot "build\Release\python"),
    (Join-Path $PSScriptRoot "build\Desktop_Qt_6_9_1_MinGW_64_bit-Debug\python"),
    (Join-Path $PSScriptRoot "build\Desktop_Qt_6_9_1_MinGW_64_bit-Release\python")
)

# Funkcja kopiująca katalog
function Copy-DirectoryRecursive {
    param (
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path -Path $Destination)) {
        New-Item -ItemType Directory -Path $Destination | Out-Null
        Write-Output "Utworzono katalog $Destination"
    }

    # Kopiuj pliki
    Get-ChildItem -Path $Source -File | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $Destination -Force
    }

    # Kopiuj podkatalogi
    Get-ChildItem -Path $Source -Directory | ForEach-Object {
        $destSubDir = Join-Path -Path $Destination -ChildPath $_.Name
        Copy-DirectoryRecursive -Source $_.FullName -Destination $destSubDir
    }
}

# Sprawdź czy wybieramy wrapper prosty czy pełny
$useFullWrapper = $false
if (Test-Path -Path $fullWrapperPath) {
    $response = Read-Host "Czy użyć pełnego wrappera (y/n)? Pełny wrapper wymaga zainstalowanych zależności Python."
    if ($response -eq "y" -or $response -eq "Y") {
        $useFullWrapper = $true
        Write-Output "Wybrano pełny wrapper."
    } else {
        Write-Output "Wybrano prosty wrapper."
    }
}

foreach ($buildDir in $buildDirs) {
    # Utwórz katalog python jeśli nie istnieje
    if (-not (Test-Path -Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
        Write-Output "Utworzono katalog $buildDir"
    }
    
    # Kopiuj właściwy wrapper jako gus_wrapper.py
    if ($useFullWrapper) {
        if (Test-Path -Path $fullWrapperPath) {
            $destPath = Join-Path $buildDir "gus_wrapper.py"
            Copy-Item -Path $fullWrapperPath -Destination $destPath -Force
            Write-Output "Skopiowano pełny wrapper do $destPath"
            
            # Kopiuj również bibliotekę RegonAPI
            $destRegonAPIPath = Join-Path (Split-Path -Parent $buildDir) "RegonAPI-1.3.1"
            if (Test-Path -Path $regonAPIPath) {
                Copy-DirectoryRecursive -Source $regonAPIPath -Destination $destRegonAPIPath
                Write-Output "Skopiowano bibliotekę RegonAPI do $destRegonAPIPath"
            } else {
                Write-Warning "Nie znaleziono biblioteki RegonAPI w $regonAPIPath"
            }
        } else {
            Write-Warning "Nie znaleziono pełnego wrappera w $fullWrapperPath"
            
            # Użyj prostego wrappera jako fallback
            if (Test-Path -Path $simpleWrapperPath) {
                $destPath = Join-Path $buildDir "gus_wrapper.py"
                Copy-Item -Path $simpleWrapperPath -Destination $destPath -Force
                Write-Output "Skopiowano prosty wrapper (fallback) do $destPath"
            }
        }
    } else {
        # Użyj prostego wrappera
        if (Test-Path -Path $simpleWrapperPath) {
            $destPath = Join-Path $buildDir "gus_wrapper.py"
            Copy-Item -Path $simpleWrapperPath -Destination $destPath -Force
            Write-Output "Skopiowano prosty wrapper do $destPath"
        } else {
            Write-Warning "Nie znaleziono prostego wrappera w $simpleWrapperPath"
        }
    }
}

Write-Output "Aktualizacja wrapperow zakonczona!"
