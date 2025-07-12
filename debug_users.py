#!/usr/bin/env python3
"""
Debug script to test user authentication
"""

import json
import hashlib
import base64
import os
from pathlib import Path

def test_authentication():
    print("🔍 Debug: User Authentication Test")
    print("=" * 50)
    
    # Test paths
    current_dir = Path("users.json")
    config_dir = Path.home() / ".config" / "TwojaFirma" / "EtykietyManager" / "users.json"
    
    print(f"Current directory file: {current_dir} - exists: {current_dir.exists()}")
    print(f"Config directory file: {config_dir} - exists: {config_dir.exists()}")
    
    # Load from config directory (like the app)
    if config_dir.exists():
        users_file = config_dir
    elif current_dir.exists():
        users_file = current_dir
    else:
        print("❌ No users file found!")
        return
    
    print(f"\n📂 Loading users from: {users_file}")
    
    with open(users_file, 'r') as f:
        users = json.load(f)
    
    print(f"✅ Loaded {len(users)} users")
    
    # Test authentication for each user
    test_passwords = {
        'admin': ['admin', 'admin123', 'password'],
        'operator': ['operator', 'operator123', 'password']
    }
    
    for user in users:
        login = user['login']
        stored_hash = user['password']
        
        print(f"\n👤 Testing user: {login}")
        print(f"   Stored hash: {stored_hash}")
        
        if '$' in stored_hash:
            salt, expected_hash = stored_hash.split('$')
            print(f"   Salt: {salt}")
            print(f"   Expected hash: {expected_hash}")
            
            # Test various passwords
            if login in test_passwords:
                for test_pass in test_passwords[login]:
                    # Method 1: salt.toUtf8() (what the app does)
                    salt_bytes = salt.encode('utf-8')
                    calc_hash = hashlib.pbkdf2_hmac('sha256', test_pass.encode('utf-8'), salt_bytes, 10000)
                    calc_hash_b64 = base64.b64encode(calc_hash).decode('utf-8')
                    
                    match = calc_hash_b64 == expected_hash
                    status = "✅" if match else "❌"
                    print(f"   {status} Password '{test_pass}': {match}")
                    
                    if match:
                        print(f"   🎉 CORRECT PASSWORD FOR {login}: '{test_pass}'")
                        break
        else:
            print(f"   ❌ Invalid hash format!")

def main():
    test_authentication()

if __name__ == "__main__":
    main()