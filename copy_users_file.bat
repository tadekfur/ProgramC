@echo off
REM Copy users.json to Windows AppData location

echo Copying users.json to AppData...

REM Create AppData directory structure
if not exist "%APPDATA%\TwojaFirma" mkdir "%APPDATA%\TwojaFirma"
if not exist "%APPDATA%\TwojaFirma\EtykietyManager" mkdir "%APPDATA%\TwojaFirma\EtykietyManager"

REM Copy users.json
copy "users.json" "%APPDATA%\TwojaFirma\EtykietyManager\users.json"

echo Users file copied to: %APPDATA%\TwojaFirma\EtykietyManager\users.json
echo.
echo USERS AVAILABLE:
echo   admin     / password: admin123
echo   operator  / password: operator123
echo.
echo You can now run the application and should see users in login dialog.
pause