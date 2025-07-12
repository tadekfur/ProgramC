#!/bin/bash

# Skrypt do ustawienia zmiennych środowiskowych dla EtykietyManager
# Użyj: source setup_env.sh

# Database configuration
export DB_TYPE="QPSQL"              # lub "QSQLITE"
export DB_HOST="localhost"
export DB_PORT="5432"
export DB_NAME="etykiety_db"
export DB_USER="postgres"
export DB_PASSWORD="tadek123"  # Twoje aktualne hasło z config.ini

# Security settings
export APP_SECRET_KEY="EtykietyManager_SecretKey_2024_$(date +%s)"  # Unikalny klucz
export SESSION_TIMEOUT="3600"      # 1 hour

# Optional: Enable production mode
export PRODUCTION_MODE="true"

echo "Environment variables set for EtykietyManager"
echo "Database: $DB_TYPE at $DB_HOST:$DB_PORT/$DB_NAME"
echo "Session timeout: $SESSION_TIMEOUT seconds"

# Sprawdź czy hasło zostało zmienione
if [ "$DB_PASSWORD" = "your_secure_password_here" ]; then
    echo "WARNING: Please change DB_PASSWORD to a secure password!"
fi

if [ "$APP_SECRET_KEY" = "your-256-bit-secret-key-here" ]; then
    echo "WARNING: Please change APP_SECRET_KEY to a secure key!"
fi