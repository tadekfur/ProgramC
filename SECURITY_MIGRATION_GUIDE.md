# Security Migration Guide

## Overview

This guide helps you migrate from the old insecure system to the new secure system implemented in EtykietyManager.

## What Changed

### 1. Database Configuration
- **Before**: Hardcoded credentials in `config.ini`
- **After**: Environment variables and secure configuration

### 2. Password Storage
- **Before**: Plain text passwords
- **After**: PBKDF2 hashed passwords with salt

### 3. Session Management
- **Before**: No session management
- **After**: Timed sessions with automatic logout

## Migration Steps

### Step 1: Set Environment Variables

Create environment variables for database configuration:

**Linux/macOS:**
```bash
source setup_env.sh
```

**Windows:**
```cmd
setup_env.bat
```

**Manual setup:**
```bash
export DB_TYPE="QPSQL"
export DB_HOST="localhost"
export DB_PORT="5432"
export DB_NAME="etykiety_db"
export DB_USER="postgres"
export DB_PASSWORD="your_secure_password"
export APP_SECRET_KEY="your_256_bit_secret_key"
export SESSION_TIMEOUT="3600"
```

### Step 2: Run Migration Tool

```bash
# Analyze current data
./migrate_security --analyze

# Create backup and migrate
./migrate_security

# Verify migration
./migrate_security --verify
```

### Step 3: Test New System

1. Start application
2. Log in with existing credentials
3. Verify session management works
4. Test automatic logout

## New Security Features

### Password Requirements
- Minimum 8 characters
- Must contain: uppercase, lowercase, digits, special characters
- Automatically validated on user creation/update

### Account Lockout
- Max 5 failed login attempts
- 15-minute lockout period
- Manual unlock by administrator

### Session Management
- Configurable session timeout (default 1 hour)
- Automatic logout on inactivity
- Session status in UI
- Warning before expiration

### Database Security
- No more hardcoded credentials
- Environment variable configuration
- Secure connection pooling

## Troubleshooting

### Database Connection Issues
1. Check environment variables are set correctly
2. Verify database credentials
3. Check network connectivity

### Migration Issues
1. Backup is created automatically
2. Use `--restore backup_file` to rollback
3. Check migration logs for errors

### Login Issues
1. Verify account is not locked
2. Check password requirements
3. Clear failed login attempts

## Files Changed

### New Files
- `utils/secure_config.h/cpp` - Secure configuration
- `utils/password_manager.h/cpp` - Password hashing
- `utils/session_manager.h/cpp` - Session management
- `utils/secure_user_manager.h/cpp` - Secure user management

### Modified Files
- `db/dbmanager.cpp` - Uses SecureConfig
- `main.cpp` - SessionManager integration
- `mainwindow.cpp/h` - Session UI and activity tracking
- `views/login_dialog.cpp` - Secure authentication
- `CMakeLists.txt` - New dependencies

### Deprecated Files
- `config.ini` → `config.ini.deprecated`
- `utils/usermanager.cpp/h` - Use SecureUserManager instead

## Security Best Practices

1. **Never commit secrets to version control**
2. **Use strong passwords for database**
3. **Regularly rotate secret keys**
4. **Monitor failed login attempts**
5. **Keep session timeouts reasonable**
6. **Use HTTPS for production deployments**

## Support

For issues or questions:
1. Check logs in application data directory
2. Run migration tool with `--analyze` and `--verify`
3. Review environment variable configuration
4. Test with minimal configuration first