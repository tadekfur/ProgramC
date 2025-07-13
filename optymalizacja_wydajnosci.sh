#!/bin/bash

# Skrypt Optymalizacji Wydajności Aplikacji EtykietyManager
# Autor: AI Assistant
# Data: 2024
# Wersja: 1.0

echo "🚀 Skrypt Optymalizacji Wydajności EtykietyManager"
echo "================================================="

# Sprawdź czy jesteśmy w głównym katalogu projektu
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Błąd: Nie znaleziono CMakeLists.txt"
    echo "   Uruchom skrypt z głównego katalogu projektu"
    exit 1
fi

# Funkcja do tworzenia kopii zapasowej
backup_file() {
    local file=$1
    if [ -f "$file" ]; then
        cp "$file" "${file}.backup.$(date +%Y%m%d_%H%M%S)"
        echo "✅ Utworzono kopię zapasową: ${file}.backup.$(date +%Y%m%d_%H%M%S)"
    fi
}

# Funkcja do czyszczenia niepotrzebnych czcionek
clean_fonts() {
    echo "🧹 Czyszczenie niepotrzebnych czcionek..."
    
    # Usuń duplikaty czcionek z katalogu utils
    if [ -d "utils" ]; then
        echo "   Usuwanie duplikatów czcionek z utils/"
        find utils/ -name "*.ttf" -not -name "DejaVuSans.ttf" -not -name "DejaVuSans-Bold.ttf" -delete
    fi
    
    # Usuń niepotrzebne czcionki z resources
    if [ -d "resources" ]; then
        echo "   Usuwanie niepotrzebnych czcionek z resources/"
        find resources/ -name "*.ttf" -not -name "DejaVuSans.ttf" -not -name "DejaVuSans-Bold.ttf" -delete
    fi
    
    # Usuń czcionki z build directory
    if [ -d "build" ]; then
        echo "   Usuwanie czcionek z build/"
        find build/ -name "*.ttf" -delete
    fi
    
    echo "✅ Czyszczenie czcionek zakończone"
}

# Funkcja do optymalizacji zasobów
optimize_resources() {
    echo "📦 Optymalizacja zasobów Qt..."
    
    if [ -f "resources/app.qrc" ]; then
        backup_file "resources/app.qrc"
        
        if [ -f "resources/app_optimized.qrc" ]; then
            cp "resources/app_optimized.qrc" "resources/app.qrc"
            echo "✅ Zastąpiono app.qrc zoptymalizowaną wersją"
        else
            echo "⚠️  Ostrzeżenie: Brak pliku app_optimized.qrc"
        fi
    fi
}

# Funkcja do optymalizacji CMakeLists.txt
optimize_cmake() {
    echo "⚙️  Optymalizacja CMakeLists.txt..."
    
    if [ -f "CMakeLists.txt" ]; then
        backup_file "CMakeLists.txt"
        
        if [ -f "CMakeLists_optimized.txt" ]; then
            cp "CMakeLists_optimized.txt" "CMakeLists.txt"
            echo "✅ Zastąpiono CMakeLists.txt zoptymalizowaną wersją"
        else
            echo "⚠️  Ostrzeżenie: Brak pliku CMakeLists_optimized.txt"
        fi
    fi
}

# Funkcja do analizy rozmiaru
analyze_size() {
    echo "📊 Analiza rozmiaru plików..."
    
    if [ -d "resources" ]; then
        echo "   Rozmiar katalogu resources:"
        du -sh resources/
    fi
    
    if [ -d "utils" ]; then
        echo "   Rozmiar katalogu utils:"
        du -sh utils/
    fi
    
    if [ -d "build" ]; then
        echo "   Rozmiar katalogu build:"
        du -sh build/
    fi
    
    echo "   Najwęższe pliki:"
    find . -name "*.ttf" -o -name "*.pdf" -o -name "*.exe" -o -name "*.dll" | xargs ls -lh | sort -k5 -hr | head -10
}

# Funkcja do czyszczenia plików tymczasowych
clean_temp() {
    echo "🧽 Czyszczenie plików tymczasowych..."
    
    # Usuń pliki cache
    find . -name "*.cache" -delete
    find . -name "*.tmp" -delete
    find . -name "*.temp" -delete
    
    # Usuń pliki Qt MOC
    find . -name "moc_*.cpp" -delete
    find . -name "ui_*.h" -delete
    find . -name "qrc_*.cpp" -delete
    
    # Usuń pliki CMake cache
    find . -name "CMakeCache.txt" -delete
    find . -name "cmake_install.cmake" -delete
    find . -type d -name "CMakeFiles" -exec rm -rf {} + 2>/dev/null || true
    
    echo "✅ Czyszczenie plików tymczasowych zakończone"
}

# Funkcja do weryfikacji optymalizacji
verify_optimization() {
    echo "🔍 Weryfikacja optymalizacji..."
    
    # Sprawdź czy istnieją zoptymalizowane pliki
    local errors=0
    
    if [ ! -f "resources/app.qrc" ]; then
        echo "❌ Brak pliku resources/app.qrc"
        errors=$((errors + 1))
    fi
    
    if [ ! -f "CMakeLists.txt" ]; then
        echo "❌ Brak pliku CMakeLists.txt"
        errors=$((errors + 1))
    fi
    
    if [ ! -f "pch.cpp" ]; then
        echo "⚠️  Brak pliku pch.cpp - dodaj precompiled headers"
    fi
    
    if [ $errors -eq 0 ]; then
        echo "✅ Weryfikacja przebiegła pomyślnie"
    else
        echo "❌ Znaleziono $errors błędów"
        return 1
    fi
}

# Funkcja do generowania raportu
generate_report() {
    echo "📋 Generowanie raportu optymalizacji..."
    
    local report_file="raport_optymalizacji_$(date +%Y%m%d_%H%M%S).txt"
    
    {
        echo "Raport Optymalizacji Wydajności"
        echo "==============================="
        echo "Data: $(date)"
        echo "Katalog: $(pwd)"
        echo ""
        echo "Analiza rozmiaru po optymalizacji:"
        du -sh resources/ utils/ build/ 2>/dev/null || echo "Niektóre katalogi nie istnieją"
        echo ""
        echo "Liczba plików .ttf:"
        find . -name "*.ttf" | wc -l
        echo ""
        echo "Największe pliki:"
        find . -type f -exec ls -lh {} + | sort -k5 -hr | head -20
    } > "$report_file"
    
    echo "✅ Raport zapisany w: $report_file"
}

# Menu główne
show_menu() {
    echo ""
    echo "Wybierz opcję:"
    echo "1. Kompletna optymalizacja (zalecane)"
    echo "2. Tylko czyszczenie czcionek"
    echo "3. Tylko optymalizacja zasobów"
    echo "4. Tylko optymalizacja CMake"
    echo "5. Analiza rozmiaru"
    echo "6. Czyszczenie plików tymczasowych"
    echo "7. Weryfikacja optymalizacji"
    echo "8. Generuj raport"
    echo "9. Wyjście"
    echo ""
    read -p "Wprowadź numer opcji (1-9): " choice
}

# Funkcja kompletnej optymalizacji
full_optimization() {
    echo "🎯 Rozpoczynam kompletną optymalizację..."
    
    clean_fonts
    optimize_resources
    optimize_cmake
    clean_temp
    verify_optimization
    generate_report
    
    echo ""
    echo "🎉 Kompletna optymalizacja zakończona!"
    echo "📊 Sprawdź plik ANALIZA_WYDAJNOSCI.md dla szczegółów"
    echo "⚠️  Pamiętaj o przetestowaniu aplikacji po optymalizacji"
}

# Główna pętla programu
main() {
    while true; do
        show_menu
        
        case $choice in
            1)
                full_optimization
                ;;
            2)
                clean_fonts
                ;;
            3)
                optimize_resources
                ;;
            4)
                optimize_cmake
                ;;
            5)
                analyze_size
                ;;
            6)
                clean_temp
                ;;
            7)
                verify_optimization
                ;;
            8)
                generate_report
                ;;
            9)
                echo "👋 Dziękuję za skorzystanie ze skryptu optymalizacji!"
                exit 0
                ;;
            *)
                echo "❌ Nieprawidłowa opcja. Wybierz numer od 1 do 9."
                ;;
        esac
        
        echo ""
        read -p "Naciśnij Enter aby kontynuować..."
    done
}

# Uruchom główną funkcję
main