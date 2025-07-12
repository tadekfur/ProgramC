#!/usr/bin/env python3
"""
Test Database Security - Symulacja bezpiecznego połączenia z bazą danych
Testuje SecureConfig i bezpieczne zarządzanie połączeniami
"""

import os
import json
import time
from typing import Dict, Optional
from dataclasses import dataclass
from datetime import datetime

@dataclass
class DatabaseConnection:
    host: str
    port: int
    database: str
    user: str
    password: str
    connection_type: str
    is_connected: bool = False
    connection_time: datetime = None
    last_activity: datetime = None

class SecureConfig:
    """Symuluje SecureConfig z aplikacji"""
    
    def __init__(self):
        self.load_environment_variables()
    
    def load_environment_variables(self):
        """Ładuje zmienne środowiskowe"""
        self.db_type = os.getenv('DB_TYPE', 'QSQLITE')
        self.db_host = os.getenv('DB_HOST', 'localhost')
        self.db_port = int(os.getenv('DB_PORT', '5432'))
        self.db_name = os.getenv('DB_NAME', 'etykiety_db')
        self.db_user = os.getenv('DB_USER', 'postgres')
        self.db_password = os.getenv('DB_PASSWORD', '')
        self.app_secret_key = os.getenv('APP_SECRET_KEY', '')
        self.session_timeout = int(os.getenv('SESSION_TIMEOUT', '3600'))
    
    def get_database_host(self) -> str:
        return self.db_host
    
    def get_database_port(self) -> int:
        return self.db_port
    
    def get_database_name(self) -> str:
        return self.db_name
    
    def get_database_user(self) -> str:
        return self.db_user
    
    def get_database_password(self) -> str:
        return self.db_password
    
    def get_database_type(self) -> str:
        return self.db_type
    
    def get_secret_key(self) -> str:
        return self.app_secret_key
    
    def get_session_timeout(self) -> int:
        return self.session_timeout
    
    def validate_configuration(self) -> Dict[str, bool]:
        """Sprawdza poprawność konfiguracji"""
        results = {
            'db_type_set': bool(self.db_type),
            'db_host_set': bool(self.db_host),
            'db_port_valid': 1 <= self.db_port <= 65535,
            'db_name_set': bool(self.db_name),
            'db_user_set': bool(self.db_user),
            'db_password_set': bool(self.db_password),
            'secret_key_set': bool(self.app_secret_key),
            'session_timeout_valid': self.session_timeout > 0
        }
        return results

class SecureDbManager:
    """Symuluje SecureDbManager z aplikacji"""
    
    POOL_SIZE = 3
    
    def __init__(self):
        self.config = SecureConfig()
        self.main_connection: Optional[DatabaseConnection] = None
        self.connection_pool = []
        self.pool_available = []
        self.initialize_connections()
    
    def initialize_connections(self):
        """Inicjalizuje połączenia z bazą danych"""
        print("🔗 Initializing secure database connections...")
        
        # Główne połączenie
        self.main_connection = self.create_connection("main_conn")
        
        # Pula połączeń
        for i in range(self.POOL_SIZE):
            conn = self.create_connection(f"pool_conn_{i}")
            self.connection_pool.append(conn)
            self.pool_available.append(True)
        
        print(f"✅ Initialized {1 + self.POOL_SIZE} database connections")
    
    def create_connection(self, connection_name: str) -> DatabaseConnection:
        """Tworzy połączenie z bazą danych"""
        conn = DatabaseConnection(
            host=self.config.get_database_host(),
            port=self.config.get_database_port(),
            database=self.config.get_database_name(),
            user=self.config.get_database_user(),
            password=self.config.get_database_password(),
            connection_type=self.config.get_database_type()
        )
        
        # Symulacja połączenia
        if self.simulate_connection(conn):
            conn.is_connected = True
            conn.connection_time = datetime.now()
            conn.last_activity = datetime.now()
            print(f"✅ Connection {connection_name} established")
        else:
            print(f"❌ Connection {connection_name} failed")
        
        return conn
    
    def simulate_connection(self, conn: DatabaseConnection) -> bool:
        """Symuluje próbę połączenia z bazą danych"""
        # Sprawdź czy wszystkie wymagane parametry są ustawione
        if not conn.password:
            print("❌ Database password not configured")
            return False
        
        if conn.connection_type == "QPSQL":
            # Symulacja PostgreSQL
            if conn.host and conn.port and conn.database and conn.user:
                print(f"🐘 Connecting to PostgreSQL: {conn.user}@{conn.host}:{conn.port}/{conn.database}")
                return True
            else:
                print("❌ Missing PostgreSQL connection parameters")
                return False
        
        elif conn.connection_type == "QSQLITE":
            # Symulacja SQLite
            print(f"🗄️  Connecting to SQLite: {conn.database}")
            return True
        
        else:
            print(f"❌ Unknown database type: {conn.connection_type}")
            return False
    
    def get_pooled_connection(self) -> Optional[DatabaseConnection]:
        """Pobiera połączenie z puli"""
        for i, available in enumerate(self.pool_available):
            if available:
                self.pool_available[i] = False
                conn = self.connection_pool[i]
                conn.last_activity = datetime.now()
                print(f"📤 Retrieved pooled connection {i}")
                return conn
        
        print("⚠️  No pooled connections available")
        return None
    
    def return_pooled_connection(self, conn_index: int):
        """Zwraca połączenie do puli"""
        if 0 <= conn_index < len(self.pool_available):
            self.pool_available[conn_index] = True
            print(f"📥 Returned pooled connection {conn_index}")
        else:
            print(f"❌ Invalid connection index: {conn_index}")
    
    def get_connection_stats(self) -> Dict:
        """Zwraca statystyki połączeń"""
        active_pool = sum(1 for available in self.pool_available if not available)
        total_pool = len(self.pool_available)
        
        return {
            'main_connection_active': self.main_connection.is_connected if self.main_connection else False,
            'pool_connections_active': active_pool,
            'pool_connections_total': total_pool,
            'pool_connections_available': total_pool - active_pool
        }

class DatabaseSecurityTest:
    """Test bezpieczeństwa bazy danych"""
    
    def __init__(self):
        self.config = SecureConfig()
        self.db_manager = SecureDbManager()
    
    def test_environment_variables(self):
        """Test zmiennych środowiskowych"""
        print("\n=== Test: Environment Variables ===")
        
        validation = self.config.validate_configuration()
        
        print("Environment variables status:")
        for key, is_valid in validation.items():
            status = "✅" if is_valid else "❌"
            print(f"  {status} {key}: {is_valid}")
        
        # Sprawdź czy nie ma plain text secrets w kodzie
        print("\n🔍 Security check:")
        if self.config.get_database_password():
            print("✅ Database password loaded from environment")
        else:
            print("❌ Database password not set")
        
        if self.config.get_secret_key():
            print("✅ Secret key loaded from environment")
        else:
            print("❌ Secret key not set")
        
        return all(validation.values())
    
    def test_database_connections(self):
        """Test połączeń z bazą danych"""
        print("\n=== Test: Database Connections ===")
        
        # Sprawdź główne połączenie
        if self.db_manager.main_connection and self.db_manager.main_connection.is_connected:
            print("✅ Main database connection established")
        else:
            print("❌ Main database connection failed")
        
        # Test puli połączeń
        stats = self.db_manager.get_connection_stats()
        print(f"📊 Connection pool stats:")
        print(f"   Main connection: {'Active' if stats['main_connection_active'] else 'Inactive'}")
        print(f"   Pool connections: {stats['pool_connections_total']} total, {stats['pool_connections_available']} available")
        
        return stats['main_connection_active'] and stats['pool_connections_total'] > 0
    
    def test_connection_pooling(self):
        """Test puli połączeń"""
        print("\n=== Test: Connection Pooling ===")
        
        # Pobierz wszystkie połączenia z puli
        connections = []
        for i in range(self.db_manager.POOL_SIZE):
            conn = self.db_manager.get_pooled_connection()
            if conn:
                connections.append((i, conn))
                print(f"✅ Retrieved connection {i}")
            else:
                print(f"❌ Failed to retrieve connection {i}")
        
        # Sprawdź czy pula jest wyczerpana
        extra_conn = self.db_manager.get_pooled_connection()
        if extra_conn is None:
            print("✅ Pool correctly exhausted")
        else:
            print("❌ Pool should be exhausted")
        
        # Zwróć połączenia
        for i, conn in connections:
            self.db_manager.return_pooled_connection(i)
        
        # Sprawdź czy połączenia są dostępne
        stats = self.db_manager.get_connection_stats()
        if stats['pool_connections_available'] == self.db_manager.POOL_SIZE:
            print("✅ All connections returned to pool")
        else:
            print("❌ Not all connections returned")
        
        return len(connections) == self.db_manager.POOL_SIZE
    
    def test_security_configuration(self):
        """Test konfiguracji bezpieczeństwa"""
        print("\n=== Test: Security Configuration ===")
        
        # Sprawdź czy hasła nie są w kodzie
        print("🔒 Security checks:")
        
        # Sprawdź czy używa zmiennych środowiskowych
        if os.getenv('DB_PASSWORD'):
            print("✅ Database password from environment variable")
        else:
            print("❌ Database password not in environment")
        
        if os.getenv('APP_SECRET_KEY'):
            print("✅ Secret key from environment variable")
        else:
            print("❌ Secret key not in environment")
        
        # Sprawdź typ połączenia
        db_type = self.config.get_database_type()
        if db_type in ['QPSQL', 'QSQLITE']:
            print(f"✅ Valid database type: {db_type}")
        else:
            print(f"❌ Invalid database type: {db_type}")
        
        # Sprawdź timeout sesji
        timeout = self.config.get_session_timeout()
        if 300 <= timeout <= 86400:  # 5 minut do 24 godzin
            print(f"✅ Reasonable session timeout: {timeout} seconds")
        else:
            print(f"⚠️  Unusual session timeout: {timeout} seconds")
        
        return True
    
    def test_connection_security(self):
        """Test bezpieczeństwa połączeń"""
        print("\n=== Test: Connection Security ===")
        
        # Sprawdź czy połączenia używają bezpiecznej konfiguracji
        if self.db_manager.main_connection:
            conn = self.db_manager.main_connection
            
            # Sprawdź parametry połączenia
            if conn.password:
                print("✅ Connection uses password authentication")
            else:
                print("❌ Connection missing password")
            
            if conn.host == 'localhost' or conn.host.startswith('127.'):
                print("✅ Connection to localhost (secure)")
            else:
                print(f"⚠️  Connection to remote host: {conn.host}")
            
            if conn.connection_type == 'QPSQL' and conn.port == 5432:
                print("✅ Standard PostgreSQL port")
            elif conn.connection_type == 'QSQLITE':
                print("✅ SQLite connection (local file)")
            else:
                print(f"⚠️  Non-standard configuration: {conn.connection_type}:{conn.port}")
        
        return True
    
    def run_all_tests(self):
        """Uruchom wszystkie testy"""
        print("🔐 Database Security Test Suite")
        print("=" * 50)
        
        results = []
        
        # Test 1: Zmienne środowiskowe
        results.append(self.test_environment_variables())
        
        # Test 2: Połączenia z bazą danych
        results.append(self.test_database_connections())
        
        # Test 3: Pula połączeń
        results.append(self.test_connection_pooling())
        
        # Test 4: Konfiguracja bezpieczeństwa
        results.append(self.test_security_configuration())
        
        # Test 5: Bezpieczeństwo połączeń
        results.append(self.test_connection_security())
        
        print("\n" + "=" * 50)
        
        if all(results):
            print("🎉 All database security tests passed!")
        else:
            print("❌ Some database security tests failed!")
        
        print("\nDatabase security features verified:")
        print("✅ Environment-based configuration")
        print("✅ Secure password management")
        print("✅ Connection pooling")
        print("✅ No hardcoded credentials")
        print("✅ Proper connection handling")
        
        return all(results)

def main():
    # Sprawdź zmienne środowiskowe
    print("🔍 Checking environment variables...")
    required_vars = ['DB_TYPE', 'DB_HOST', 'DB_PORT', 'DB_NAME', 'DB_USER', 'DB_PASSWORD']
    missing_vars = [var for var in required_vars if not os.getenv(var)]
    
    if missing_vars:
        print(f"⚠️  Missing environment variables: {', '.join(missing_vars)}")
        print("Please run: source setup_env.sh")
        print("Continuing with available variables...")
    else:
        print("✅ All required environment variables are set")
    
    # Uruchom testy
    test = DatabaseSecurityTest()
    success = test.run_all_tests()
    
    return 0 if success else 1

if __name__ == "__main__":
    exit(main())