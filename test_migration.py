#!/usr/bin/env python3
"""
Test migration of plain text passwords to hashes
"""

import json
import hashlib
import base64
import secrets
import os

def generate_salt():
    return base64.b64encode(secrets.token_bytes(32)).decode('utf-8')

def pbkdf2_hash(password, salt):
    salt_bytes = salt.encode('utf-8')
    password_bytes = password.encode('utf-8')
    hash_bytes = hashlib.pbkdf2_hmac('sha256', password_bytes, salt_bytes, 10000)
    return base64.b64encode(hash_bytes).decode('utf-8')

def hash_password(password):
    salt = generate_salt()
    hash_val = pbkdf2_hash(password, salt)
    return f"{salt}${hash_val}"

def migrate_users():
    print("🔄 Migrating user passwords...")
    
    # Load users
    with open('users.json', 'r') as f:
        users = json.load(f)
    
    migrated = False
    
    for user in users:
        login = user['login']
        password = user['password']
        
        # Check if password needs migration (no '$' means plain text)
        if '$' not in password:
            print(f"   Migrating password for user: {login}")
            user['password'] = hash_password(password)
            migrated = True
        else:
            print(f"   Password already hashed for user: {login}")
    
    if migrated:
        # Save to both locations
        with open('users.json', 'w') as f:
            json.dump(users, f, indent=2)
        
        # Also save to config directory
        config_dir = os.path.expanduser('~/.config/TwojaFirma/EtykietyManager')
        os.makedirs(config_dir, exist_ok=True)
        
        with open(f'{config_dir}/users.json', 'w') as f:
            json.dump(users, f, indent=2)
        
        print("✅ Migration completed and saved to both locations")
        
        # Show new credentials
        print("\n🔑 New credentials:")
        for user in users:
            print(f"   {user['login']}: Use original password (now hashed)")
    else:
        print("ℹ️  No migration needed")

if __name__ == "__main__":
    migrate_users()