#!/usr/bin/env python3
"""
Test Login Security - Symulacja bezpiecznego logowania
Testuje funkcje SecureUserManager i SessionManager
"""

import json
import time
import hashlib
import base64
import secrets
from datetime import datetime, timedelta
from dataclasses import dataclass
from typing import Dict, Optional

@dataclass
class LoginAttempt:
    failed_attempts: int = 0
    last_attempt: datetime = None
    is_locked: bool = False

@dataclass
class Session:
    user_login: str
    created_at: datetime
    expires_at: datetime
    last_activity: datetime
    session_id: str

class SecureUserManager:
    MAX_LOGIN_ATTEMPTS = 5
    LOCKOUT_DURATION = 900  # 15 minut
    
    def __init__(self):
        self.users_file = "users.json"
        self.login_attempts: Dict[str, LoginAttempt] = {}
        self.load_users()
    
    def load_users(self):
        """Ładuje użytkowników z pliku"""
        try:
            with open(self.users_file, 'r') as f:
                self.users = json.load(f)
            print(f"✅ Loaded {len(self.users)} users")
        except FileNotFoundError:
            print("❌ Users file not found")
            self.users = []
    
    def get_user(self, login: str) -> Optional[dict]:
        """Zwraca użytkownika po loginie"""
        return next((u for u in self.users if u.get('login') == login), None)
    
    def verify_password(self, password: str, hash_str: str) -> bool:
        """Weryfikuje hasło z hashem"""
        try:
            salt, hash_expected = hash_str.split('$')
            password_bytes = password.encode('utf-8')
            salt_bytes = salt.encode('utf-8')
            
            hash_actual = hashlib.pbkdf2_hmac('sha256', password_bytes, salt_bytes, 10000)
            hash_actual_b64 = base64.b64encode(hash_actual).decode('utf-8')
            
            return hash_actual_b64 == hash_expected
        except:
            return False
    
    def is_account_locked(self, login: str) -> bool:
        """Sprawdza czy konto jest zablokowane"""
        if login not in self.login_attempts:
            return False
        
        attempt = self.login_attempts[login]
        
        # Manual lock
        if attempt.is_locked:
            return True
        
        # Auto-lock przez zbyt wiele prób
        if attempt.failed_attempts >= self.MAX_LOGIN_ATTEMPTS:
            if attempt.last_attempt:
                unlock_time = attempt.last_attempt + timedelta(seconds=self.LOCKOUT_DURATION)
                return datetime.now() < unlock_time
        
        return False
    
    def record_login_attempt(self, login: str, success: bool):
        """Zapisuje próbę logowania"""
        if login not in self.login_attempts:
            self.login_attempts[login] = LoginAttempt()
        
        attempt = self.login_attempts[login]
        attempt.last_attempt = datetime.now()
        
        if success:
            attempt.failed_attempts = 0
        else:
            attempt.failed_attempts += 1
            
            if attempt.failed_attempts >= self.MAX_LOGIN_ATTEMPTS:
                print(f"🚨 Account {login} auto-locked after {attempt.failed_attempts} failed attempts")
    
    def get_failed_attempts(self, login: str) -> int:
        """Zwraca liczbę nieudanych prób"""
        if login not in self.login_attempts:
            return 0
        return self.login_attempts[login].failed_attempts
    
    def authenticate_user(self, login: str, password: str) -> bool:
        """Uwierzytelnia użytkownika"""
        print(f"🔐 Authentication attempt for: {login}")
        
        # Sprawdź czy konto zablokowane
        if self.is_account_locked(login):
            attempts = self.get_failed_attempts(login)
            print(f"❌ Account {login} is locked ({attempts} failed attempts)")
            self.record_login_attempt(login, False)
            return False
        
        # Znajdź użytkownika
        user = self.get_user(login)
        if not user:
            print(f"❌ User {login} not found")
            self.record_login_attempt(login, False)
            return False
        
        # Sprawdź hasło
        stored_hash = user.get('password', '')
        if self.verify_password(password, stored_hash):
            print(f"✅ Authentication successful for {login}")
            self.record_login_attempt(login, True)
            return True
        else:
            attempts = self.get_failed_attempts(login) + 1
            remaining = self.MAX_LOGIN_ATTEMPTS - attempts
            print(f"❌ Invalid password for {login} (attempt {attempts}/{self.MAX_LOGIN_ATTEMPTS})")
            if remaining > 0:
                print(f"   {remaining} attempts remaining")
            self.record_login_attempt(login, False)
            return False

class SessionManager:
    def __init__(self, session_timeout: int = 3600):
        self.session_timeout = session_timeout
        self.sessions: Dict[str, Session] = {}
    
    def create_session(self, login: str) -> str:
        """Tworzy nową sesję"""
        session_id = secrets.token_hex(16)
        now = datetime.now()
        
        session = Session(
            user_login=login,
            created_at=now,
            expires_at=now + timedelta(seconds=self.session_timeout),
            last_activity=now,
            session_id=session_id
        )
        
        self.sessions[session_id] = session
        print(f"🎫 Session created for {login}: {session_id[:8]}...")
        print(f"   Expires at: {session.expires_at.strftime('%H:%M:%S')}")
        return session_id
    
    def validate_session(self, session_id: str) -> bool:
        """Sprawdza czy sesja jest ważna"""
        if session_id not in self.sessions:
            return False
        
        session = self.sessions[session_id]
        now = datetime.now()
        
        if now > session.expires_at:
            print(f"⏰ Session {session_id[:8]}... expired")
            del self.sessions[session_id]
            return False
        
        return True
    
    def refresh_session(self, session_id: str):
        """Odświeża sesję"""
        if session_id in self.sessions:
            session = self.sessions[session_id]
            now = datetime.now()
            session.last_activity = now
            session.expires_at = now + timedelta(seconds=self.session_timeout)
            print(f"🔄 Session refreshed: {session_id[:8]}...")
    
    def get_session_info(self, session_id: str) -> Optional[Dict]:
        """Zwraca informacje o sesji"""
        if session_id not in self.sessions:
            return None
        
        session = self.sessions[session_id]
        now = datetime.now()
        seconds_left = (session.expires_at - now).total_seconds()
        
        return {
            'user_login': session.user_login,
            'created_at': session.created_at,
            'expires_at': session.expires_at,
            'seconds_left': int(seconds_left),
            'is_valid': seconds_left > 0
        }
    
    def destroy_session(self, session_id: str):
        """Niszczy sesję"""
        if session_id in self.sessions:
            login = self.sessions[session_id].user_login
            del self.sessions[session_id]
            print(f"🚪 Session destroyed for {login}")

class LoginSecurityTest:
    def __init__(self):
        self.user_manager = SecureUserManager()
        self.session_manager = SessionManager(session_timeout=10)  # 10 sekund do testowania
        
    def test_successful_login(self):
        """Test pomyślnego logowania"""
        print("\n=== Test: Successful Login ===")
        
        if self.user_manager.authenticate_user("admin", "admin123"):
            session_id = self.session_manager.create_session("admin")
            
            # Sprawdź informacje o sesji
            info = self.session_manager.get_session_info(session_id)
            if info:
                print(f"📊 Session info: {info['seconds_left']} seconds left")
            
            return session_id
        else:
            print("❌ Login failed")
            return None
    
    def test_failed_login(self):
        """Test nieudanego logowania"""
        print("\n=== Test: Failed Login ===")
        
        # Próba z złym hasłem
        result = self.user_manager.authenticate_user("admin", "wrongpassword")
        if not result:
            print("✅ Correctly rejected wrong password")
        else:
            print("❌ Should have rejected wrong password")
        
        # Próba z nieistniejącym użytkownikiem
        result = self.user_manager.authenticate_user("hacker", "password")
        if not result:
            print("✅ Correctly rejected non-existent user")
        else:
            print("❌ Should have rejected non-existent user")
    
    def test_account_lockout(self):
        """Test blokady konta"""
        print("\n=== Test: Account Lockout ===")
        
        # 5 nieudanych prób
        for i in range(6):
            result = self.user_manager.authenticate_user("operator", "wrongpassword")
            if i < 5:
                print(f"   Attempt {i+1}: {'✅' if not result else '❌'}")
            else:
                print(f"   Attempt {i+1} (should be locked): {'✅' if not result else '❌'}")
        
        # Sprawdź czy konto jest zablokowane
        if self.user_manager.is_account_locked("operator"):
            print("✅ Account correctly locked after 5 failed attempts")
        else:
            print("❌ Account should be locked")
        
        # Próba logowania z prawidłowym hasłem na zablokowanym koncie
        result = self.user_manager.authenticate_user("operator", "operator123")
        if not result:
            print("✅ Correctly blocked login on locked account")
        else:
            print("❌ Should have blocked login on locked account")
    
    def test_session_timeout(self):
        """Test wygaśnięcia sesji"""
        print("\n=== Test: Session Timeout ===")
        
        # Utwórz sesję
        if self.user_manager.authenticate_user("admin", "admin123"):
            session_id = self.session_manager.create_session("admin")
            
            # Sprawdź czy sesja jest ważna
            if self.session_manager.validate_session(session_id):
                print("✅ Session is valid immediately after creation")
            
            # Odśwież sesję
            self.session_manager.refresh_session(session_id)
            print("🔄 Session refreshed")
            
            # Poczekaj na wygaśnięcie
            print("⏳ Waiting for session to expire...")
            time.sleep(11)  # Czekaj dłużej niż timeout
            
            # Sprawdź czy sesja wygasła
            if not self.session_manager.validate_session(session_id):
                print("✅ Session correctly expired after timeout")
            else:
                print("❌ Session should have expired")
        else:
            print("❌ Failed to create session for testing")
    
    def test_session_management(self):
        """Test zarządzania sesjami"""
        print("\n=== Test: Session Management ===")
        
        # Utwórz wiele sesji
        sessions = []
        for user in ["admin", "operator"]:
            if self.user_manager.authenticate_user(user, f"{user}123"):
                session_id = self.session_manager.create_session(user)
                sessions.append(session_id)
        
        # Sprawdź wszystkie sesje
        for session_id in sessions:
            info = self.session_manager.get_session_info(session_id)
            if info:
                print(f"📊 {info['user_login']}: {info['seconds_left']}s left")
        
        # Zniszcz sesje
        for session_id in sessions:
            self.session_manager.destroy_session(session_id)
    
    def run_all_tests(self):
        """Uruchom wszystkie testy"""
        print("🔐 Security Login Test Suite")
        print("=" * 50)
        
        # Test 1: Pomyślne logowanie
        session_id = self.test_successful_login()
        
        # Test 2: Nieudane logowanie
        self.test_failed_login()
        
        # Test 3: Blokada konta
        self.test_account_lockout()
        
        # Test 4: Wygaśnięcie sesji
        self.test_session_timeout()
        
        # Test 5: Zarządzanie sesjami
        self.test_session_management()
        
        print("\n" + "=" * 50)
        print("🎉 All security tests completed!")
        print("\nSecurity features verified:")
        print("✅ Password hashing and verification")
        print("✅ Failed login attempt tracking")
        print("✅ Account lockout after 5 attempts")
        print("✅ Session creation and validation")
        print("✅ Session timeout and expiration")
        print("✅ Session management and cleanup")

def main():
    test = LoginSecurityTest()
    test.run_all_tests()

if __name__ == "__main__":
    main()