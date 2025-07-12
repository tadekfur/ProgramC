# Jak skompilować EtykietyManager w Qt

## Opcja 1: Qt Creator (Zalecana)

### Krok 1: Przygotuj środowisko
1. Uruchom `setup_env.bat` (Windows) lub `source setup_env.sh` (Linux)
2. Otwórz Qt Creator
3. File → Open File or Project → wybierz `CMakeLists.txt`

### Krok 2: Konfiguracja
1. Qt Creator automatycznie wykryje CMake
2. Wybierz odpowiedni kit (MinGW lub MSVC)
3. Ustaw Build Directory na `build`

### Krok 3: Kompilacja
1. Kliknij Build → Build All
2. Lub naciśnij Ctrl+B

## Opcja 2: Visual Studio

### Krok 1: Przygotuj środowisko
1. Uruchom `setup_env.bat` z katalogu projektu
2. Otwórz Visual Studio
3. File → Open → Project/Solution → `build/EtykietyManager.vcxproj`

### Krok 2: Kompilacja
1. Wybierz konfigurację (Debug/Release)
2. Build → Build Solution
3. Lub naciśnij F7

## Opcja 3: Command Line (Jeśli masz cmake)

```bash
# Linux/WSL
source setup_env.sh
mkdir -p build
cd build
cmake ..
make

# Windows (z cmake)
setup_env.bat
mkdir build
cd build
cmake ..
cmake --build .
```

## Rozwiązywanie problemów

### Problem: Brak Qt6
- Zainstaluj Qt6 z https://www.qt.io/download
- Upewnij się, że CMAKE_PREFIX_PATH w CMakeLists.txt wskazuje na właściwą lokalizację Qt6

### Problem: Brak PostgreSQL
- Zainstaluj PostgreSQL development libraries
- Lub zmień DB_TYPE na QSQLITE w setup_env.bat/sh

### Problem: Błędy kompilacji
- Sprawdź czy wszystkie pliki z utils/ są obecne
- Upewnij się, że zmienne środowiskowe są ustawione

## Po kompilacji

1. Uruchom `migrate_security.py` aby zmigrować użytkowników
2. Przetestuj aplikację z nowymi funkcjami bezpieczeństwa
3. Sprawdź logi w czasie działania

## Pliki wynikowe

- **Windows**: `build/Debug/EtykietyManager.exe` lub `build/Release/EtykietyManager.exe`
- **Linux**: `build/EtykietyManager`