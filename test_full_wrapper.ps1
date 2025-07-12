# Skrypt testujący pełny wrapper GUS API
Write-Output "Testowanie pełnego wrappera Python RegonAPI..."

# Sprawdz czy Python jest zainstalowany
$pythonCommand = $null
$commands = @("python", "python3")

foreach ($cmd in $commands) {
    try {
        $version = & $cmd --version 2>&1
        if ($version -match "Python 3") {
            $pythonCommand = $cmd
            Write-Output ("Znaleziono " + $cmd + ": " + $version)
            break
        }
    } catch {
        # Komenda nie istnieje
    }
}

if ($null -eq $pythonCommand) {
    Write-Error "Nie znaleziono Python 3. Prosze zainstalowac Python 3 i uruchomic skrypt ponownie."
    exit 1
}

# Lokalizacja wrappera
$wrapperPath = Join-Path $PSScriptRoot "python\gus_wrapper_full.py"
if (-not (Test-Path -Path $wrapperPath)) {
    Write-Error "Nie znaleziono wrappera Python w $wrapperPath"
    exit 1
}

# Lista testowych numerów NIP
$testNIPs = @(
    "5261040828", # Play
    "8471767883", # PGE
    "6381016402"  # Inny testowy NIP
)
$apiKey = "c9f317f699c84d9e9f0f"

foreach ($testNIP in $testNIPs) {
    Write-Output "===================================================="
    Write-Output "Testowanie NIP: $testNIP"
    Write-Output "----------------------------------------------------"
    Write-Output "Command: $pythonCommand $wrapperPath --nip $testNIP --api-key $apiKey --production --debug"
    & $pythonCommand $wrapperPath --nip $testNIP --api-key $apiKey --production --debug
    Write-Output "===================================================="
    Write-Output ""
    
    # Sprawdź plik logu błędów jeśli istnieje
    $errorLogPath = Join-Path (Split-Path -Parent $wrapperPath) "gus_error.log"
    if (Test-Path -Path $errorLogPath) {
        Write-Output "Zawartość logu błędów:"
        Get-Content -Path $errorLogPath | ForEach-Object { Write-Output $_ }
        Write-Output ""
    }
    
    # Sprawdź plik logu debug jeśli istnieje
    $debugLogPath = Join-Path (Split-Path -Parent $wrapperPath) "gus_debug.log"
    if (Test-Path -Path $debugLogPath) {
        Write-Output "Zawartość logu debug:"
        Get-Content -Path $debugLogPath | ForEach-Object { Write-Output $_ }
        Write-Output ""
    }
}

Write-Output "Wszystkie testy zakonczone!"
