#!/usr/bin/env python3
"""
Test aplikacji PBKDF2 implementation
"""

import json
import base64
import hashlib
import hmac

def qt_pbkdf2(password, salt, iterations):
    """
    Reprodukuje implementację PBKDF2 z aplikacji Qt
    """
    password_bytes = password.encode('utf-8')
    result = b''
    
    # Simplified PBKDF2 implementation jak w aplikacji
    for i in range(iterations):
        # QMessageAuthenticationCode mac(QCryptographicHash::Sha256);
        # mac.setKey(passwordBytes);
        # mac.addData(salt);
        # mac.addData(QByteArray::number(i));
        # result = mac.result();
        # passwordBytes = result; // Use result as key for next iteration
        
        data = salt + str(i).encode('utf-8')
        result = hmac.new(password_bytes, data, hashlib.sha256).digest()
        password_bytes = result  # Use result as key for next iteration
    
    return result

def verify_password_qt(password, hash_string):
    """
    Reprodukuje PasswordManager::verifyPassword
    """
    if not password or not hash_string:
        return False
    
    # Podziel hash na sól i hash
    parts = hash_string.split('$')
    if len(parts) != 2:
        return False
    
    salt = parts[0]
    expected_hash = parts[1]
    
    # Hashuj podane hasło z tą samą solą
    salt_bytes = salt.encode('utf-8')  # salt.toUtf8() w aplikacji
    calculated_hash = qt_pbkdf2(password, salt_bytes, 10000)
    
    # Porównaj hashe
    return base64.b64encode(calculated_hash).decode('utf-8') == expected_hash

def test_current_passwords():
    print("🔐 Testing current passwords with Qt implementation...")
    
    # Load current users
    with open('users.json', 'r') as f:
        users = json.load(f)
    
    test_passwords = ['admin', 'operator', 'admin123', 'operator123']
    
    for user in users:
        login = user['login']
        stored_hash = user['password']
        
        print(f"\n👤 Testing user: {login}")
        print(f"   Stored hash: {stored_hash}")
        
        for test_pass in test_passwords:
            result = verify_password_qt(test_pass, stored_hash)
            status = "✅" if result else "❌"
            print(f"   {status} Password '{test_pass}': {result}")
            
            if result:
                print(f"   🎉 WORKING PASSWORD FOR {login}: '{test_pass}'")
                break

if __name__ == "__main__":
    test_current_passwords()