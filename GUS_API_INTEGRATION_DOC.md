# Dokumentacja integracji systemu z API REGON (GUS)

## Przegląd systemu

System integracji z API REGON (GUS) umożliwia automatyczne pobieranie danych firm na podstawie numeru NIP wprowadzonego w formularzu zamówienia. Integracja wykorzystuje:

- Bibliotekę Python RegonAPI-1.3.1 
- Wrapper Python uruchamiany przez aplikację C++
- Komunikację JSON między C++ a wrapperem Python

## Architektura systemu

```
+-------------+      +------------+      +--------------+      +----------+
| Aplikacja   |      | GusClient  |      | Python       |      | API GUS  |
| Qt/C++      | ---> | (C++)      | ---> | Wrapper      | ---> | (REGON)  |
| (Formularz) |      |            |      |              |      |          |
+-------------+      +------------+      +--------------+      +----------+
                                              |
                                              V
                                        +----------------+
                                        | RegonAPI-1.3.1 |
                                        | (Biblioteka    |
                                        |  Python)       |
                                        +----------------+
```

## Komponenty systemu

### 1. GusClient (C++)

Klasa C++ odpowiedzialna za:
- Przyjmowanie numeru NIP do sprawdzenia
- Uruchamianie wrappera Python jako proces zewnętrzny
- Przetwarzanie wyniku JSON zwróconego przez wrapper
- Emisję sygnałów z danymi lub błędami

Plik: `network/gusclient.cpp`, `network/gusclient.h`

### 2. Wrapper Python

Skrypt Python odpowiedzialny za:
- Przyjmowanie parametrów z wiersza poleceń (NIP, klucz API)
- Komunikację z API GUS przez bibliotekę RegonAPI
- Zwracanie danych firmy w formacie JSON

Dostępne są dwie wersje wrappera:
- `python/gus_wrapper_simple.py` - uproszczona wersja zwracająca przykładowe dane
- `python/gus_wrapper_full.py` - pełna wersja korzystająca z biblioteki RegonAPI

### 3. Biblioteka RegonAPI-1.3.1

Biblioteka Python odpowiedzialna za:
- Uwierzytelnianie do API GUS
- Wyszukiwanie firm po NIP
- Pobieranie szczegółowych danych firm
- Obsługę błędów i formatowanie wyników

## Pliki pomocnicze

1. `install_python_deps.ps1` - skrypt PowerShell instalujący zależności Python
2. `update_wrappers.ps1` - skrypt PowerShell aktualizujący wrappery w katalogach build
3. `test_python_wrapper.ps1` - skrypt PowerShell testujący wrapper Python

## Konfiguracja i użytkowanie

### Konfiguracja środowiska

1. Zainstaluj Python 3.6+
2. Uruchom skrypt instalacyjny zależności:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\install_python_deps.ps1
   ```

### Aktualizacja wrapperów

Po zmianach w wrapperach Python uruchom:
```powershell
powershell -ExecutionPolicy Bypass -File .\update_wrappers.ps1
```

### Testowanie wrappera

Aby przetestować wrapper niezależnie od aplikacji C++:
```powershell
powershell -ExecutionPolicy Bypass -File .\test_python_wrapper.ps1
```

## Klucz API

Klucz API GUS: `c9f317f699c84d9e9f0f`

Klucz jest przechowywany w pliku `network/gusclient.cpp` jako stała `GusClient::API_KEY`.

## Diagnostyka i rozwiązywanie problemów

### Pliki logów

1. `python/gus_error.log` - szczegółowe informacje o błędach w wrapperze Python
2. `python/gus_debug.log` - informacje diagnostyczne o ścieżkach i konfiguracji

### Typowe problemy

1. **Błąd importu RegonAPI**:
   - Sprawdź czy zainstalowano zależności Python
   - Sprawdź czy biblioteka RegonAPI jest dostępna w ścieżce systemowej

2. **Wrapper zwraca kod błędu**:
   - Sprawdź logi błędów w `python/gus_error.log`
   - Sprawdź czy klucz API jest poprawny i aktualny

3. **Brak danych dla podanego NIP**:
   - Sprawdź czy NIP jest poprawny
   - Sprawdź czy firma istnieje w bazie GUS

## Dalszy rozwój

1. Implementacja cache'owania wyników zapytań dla często używanych numerów NIP
2. Optymalizacja czasu uruchamiania wrappera Python
3. Implementacja pełnego wsparcia dla API BIR1.2
4. Lepsze zarządzanie kluczami API (np. przechowywanie w zmiennych środowiskowych)
