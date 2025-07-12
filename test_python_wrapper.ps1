# Skrypt testujacy wrapper GUS
Write-Output "Testowanie wrappera Python RegonAPI..."

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
$wrapperPath = Join-Path $PSScriptRoot "python\gus_wrapper.py"
if (-not (Test-Path $wrapperPath)) {
    Write-Error "Nie znaleziono wrappera Python w $wrapperPath"
    exit 1
}

# Rzeczywisty NIP do testow (przykladowy NIP PGE Polska Grupa Energetyczna)
$testNIP = "8471767883"
$apiKey = "c9f317f699c84d9e9f0f"

Write-Output "Uruchamianie testu..."
Write-Output "Command: $pythonCommand $wrapperPath --nip $testNIP --api-key $apiKey --production --debug"
& $pythonCommand $wrapperPath --nip $testNIP --api-key $apiKey --production --debug

Write-Output "Test zakonczony!"
