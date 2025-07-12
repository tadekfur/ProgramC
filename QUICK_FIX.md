# SZYBKA NAPRAWKA - PROSTE HASŁA

## Problem
Aplikacja nie może się zalogować z zahashowanymi hasłami.

## Rozwiązanie TYMCZASOWE
Przywróciłem proste hasła tekstowe:

### UŻYTKOWNICY:
- **admin** / hasło: **admin**
- **operator** / hasło: **operator**

### Lokalizacja plików:
- `users.json` - plik główny
- `~/.config/TwojaFirma/EtykietyManager/users.json` - plik dla aplikacji

## Instrukcja:
1. Uruchom `setup_env.bat` (Windows)
2. Otwórz projekt w Qt Creator
3. Kompiluj (Ctrl+B)
4. Zaloguj się: **admin** / **admin**

## Uwaga
To jest tymczasowe rozwiązanie. Aplikacja ma funkcję migracji haseł - automatycznie zahashuje hasła przy pierwszym logowaniu.

## Jeśli dalej nie działa:
Sprawdź czy aplikacja ładuje użytkowników z właściwego pliku - powinno być w logach "Loaded X users".