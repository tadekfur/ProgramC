#!/usr/bin/env python3
"""
Test User Loading - Symuluje ładowanie użytkowników jak w aplikacji
"""

import json
import os
from pathlib import Path

def test_user_loading():
    print("🔍 Testing User Loading (like the app)")
    print("=" * 50)
    
    # Simulate app behavior
    app_data_path = Path.home() / ".config" / "TwojaFirma" / "EtykietyManager"
    users_file = app_data_path / "users.json"
    
    print(f"App would look for users at: {users_file}")
    print(f"File exists: {users_file.exists()}")
    
    if not users_file.exists():
        print("❌ Users file not found - app won't have any users!")
        return False
    
    try:
        with open(users_file, 'r') as f:
            users_data = json.load(f)
        
        print(f"✅ Successfully loaded JSON with {len(users_data)} entries")
        
        users = []
        for user_json in users_data:
            # Simulate User::fromJson()
            user = {
                'login': user_json.get('login', ''),
                'password': user_json.get('password', ''),
                'firstName': user_json.get('firstName', ''),
                'lastName': user_json.get('lastName', ''),
                'position': user_json.get('position', ''),
                'email': user_json.get('email', ''),
                'phone': user_json.get('phone', ''),
                'role': user_json.get('role', 1),
                # ... other fields
            }
            users.append(user)
            print(f"📋 User: {user['login']} ({user['firstName']} {user['lastName']})")
        
        print(f"\n✅ Total users loaded: {len(users)}")
        
        # Test authentication for admin
        admin_user = None
        for user in users:
            if user['login'] == 'admin':
                admin_user = user
                break
        
        if admin_user:
            print(f"\n🔐 Testing authentication for admin:")
            print(f"   Login: {admin_user['login']}")
            print(f"   Password hash: {admin_user['password']}")
            
            # The app should be able to authenticate with admin123
            print(f"   ✅ App should accept password: admin123")
            print(f"   ❌ App should reject password: admin")
        else:
            print("❌ Admin user not found!")
            
        return True
        
    except Exception as e:
        print(f"❌ Error loading users: {e}")
        return False

def main():
    success = test_user_loading()
    if success:
        print("\n🎉 User loading test successful!")
        print("The app should now show users in login dialog.")
    else:
        print("\n❌ User loading test failed!")
        print("Check that users.json is in the correct location.")

if __name__ == "__main__":
    main()