# Skrypt instalacji zaleznosci Python dla RegonAPI
Write-Output "Instalowanie zaleznosci Python dla RegonAPI..."

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

# Instaluj zaleznosci
Write-Output "Instalowanie wymaganych pakietow..."
& $pythonCommand -m pip install zeep
& $pythonCommand -m pip install requests
& $pythonCommand -m pip install beautifulsoup4
& $pythonCommand -m pip install lxml

# Instaluj RegonAPI z lokalnego katalogu
$regonApiPath = Join-Path $PSScriptRoot "RegonAPI-1.3.1"
if (Test-Path $regonApiPath) {
    Write-Output "Instalowanie RegonAPI z $regonApiPath..."
    & $pythonCommand -m pip install -e $regonApiPath
    
    Write-Output "Sprawdzanie instalacji..."
    $testImport = & $pythonCommand -c "
try:
    from RegonAPI import RegonAPI
    print('RegonAPI zaimportowany pomyslnie!')
except Exception as e:
    print(f'Blad importu RegonAPI: {e}')
"
    Write-Output $testImport
} else {
    Write-Error "Nie znaleziono katalogu RegonAPI-1.3.1 w $regonApiPath"
    exit 1
}

Write-Output "Instalacja zaleznosci zakonczona!"
