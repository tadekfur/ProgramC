# Fix Build Cache Issue

## Problem
Po usunięciu `utils/usermanager.cpp` i `utils/usermanager.h`, CMake/Ninja nadal próbował je kompilować, powodując błąd:
```
ninja: error: 'C:/ProgramC/utils/usermanager.cpp', needed by 'C:/ProgramC/build/Desktop_Qt_6_9_1_MinGW_64_bit-Debug/EtykietyManager_autogen/mocs_compilation.cpp', missing and no known rule to make it
```

## Rozwiązanie
Cache CMake zawierał stare informacje o usuniętych plikach. Usunięto wszystkie pliki cache:

### Automatyczne czyszczenie (wykonane)
```bash
# Usuń katalogi build
find . -name "build" -type d -exec rm -rf {} + 2>/dev/null || true

# Usuń pliki cache CMake
rm -rf CMakeCache.txt CMakeFiles/ cmake_install.cmake

# Usuń pliki build Visual Studio/Ninja
rm -rf *.vcxproj *.vcxproj.filters *.sln build.ninja rules.ninja .ninja_deps .ninja_log

# Usuń pozostałe pliki cmake
rm -rf .cmake qtcsettings.cmake
```

### Manualne czyszczenie w Qt Creator
1. **Build → Clean All**
2. **Build → Run CMake**
3. **Build → Build All**

### Manualne czyszczenie w Visual Studio
1. **Build → Clean Solution**
2. **Build → Rebuild Solution**

## Teraz aplikacja powinna się kompilować bez błędów!

### Kroki do kompilacji:
1. **Środowisko**: Uruchom `setup_env.bat` (Windows) lub `source setup_env.sh` (Linux)
2. **Kompilacja**: Otwórz projekt w Qt Creator lub Visual Studio
3. **Build**: Ctrl+B (Qt Creator) lub F7 (Visual Studio)

### Dostępni użytkownicy:
- **admin** / hasło: **admin123**
- **operator** / hasło: **operator123**

## Uwagi:
- Plik `utils/usermanager.cpp` został usunięty bo był duplikatem funkcjonalności
- Aplikacja teraz używa tylko `SecureUserManager` 
- Metody `User::toJson()` i `User::fromJson()` są w `models/user.cpp`