@echo off
REM Setup environment variables for EtykietyManager
REM Run this before building in Visual Studio

echo Setting up environment variables for EtykietyManager...

set DB_TYPE=QPSQL
set DB_HOST=localhost
set DB_PORT=5432
set DB_NAME=etykiety_db
set DB_USER=postgres
set DB_PASSWORD=tadek123
set APP_SECRET_KEY=EtykietyManager_SecretKey_2024_%RANDOM%
set SESSION_TIMEOUT=3600

echo Environment variables set:
echo   DB_TYPE=%DB_TYPE%
echo   DB_HOST=%DB_HOST%
echo   DB_PORT=%DB_PORT%
echo   DB_NAME=%DB_NAME%
echo   DB_USER=%DB_USER%
echo   SESSION_TIMEOUT=%SESSION_TIMEOUT%

echo.
echo Ready to build in Visual Studio\!
echo.
pause
EOF < /dev/null
