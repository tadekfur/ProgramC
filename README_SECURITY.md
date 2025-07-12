# EtykietyManager - Security Implementation

## Quick Start

1. **Set environment variables:**
   ```bash
   source setup_env.sh  # Linux/macOS
   setup_env.bat        # Windows
   ```

2. **Build application:**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run migration (if upgrading):**
   ```bash
   ./migrate_security
   ```

4. **Start application:**
   ```bash
   ./EtykietyManager
   ```

## Security Features

### ✅ Secure Configuration
- Database credentials from environment variables
- No hardcoded secrets in source code
- Secure key generation and storage

### ✅ Password Security
- PBKDF2 hashing with salt (10,000 iterations)
- Strong password requirements enforced
- Automatic migration from plain text passwords

### ✅ Session Management
- Configurable session timeout (default 1 hour)
- Automatic logout on inactivity
- Session status visible in UI
- Warning before session expiration

### ✅ Account Security
- Account lockout after 5 failed attempts
- 15-minute lockout period
- Failed attempt tracking and monitoring

### ✅ Database Security
- Connection pooling with secure credentials
- No plain text passwords in database
- Proper error handling and logging

## Environment Variables

| Variable | Description | Example |
|----------|-------------|---------|
| `DB_TYPE` | Database type | `QPSQL` or `QSQLITE` |
| `DB_HOST` | Database host | `localhost` |
| `DB_PORT` | Database port | `5432` |
| `DB_NAME` | Database name | `etykiety_db` |
| `DB_USER` | Database user | `postgres` |
| `DB_PASSWORD` | Database password | `your_secure_password` |
| `APP_SECRET_KEY` | Application secret key | `your_256_bit_secret` |
| `SESSION_TIMEOUT` | Session timeout in seconds | `3600` |

## Migration from Old System

If you're upgrading from the old insecure system:

1. **Backup your data** (done automatically)
2. **Run migration tool**: `./migrate_security`
3. **Verify migration**: `./migrate_security --verify`
4. **Test login** with existing credentials

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   LoginDialog   │───▶│ SecureUserManager │───▶│ PasswordManager │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ SessionManager  │    │   SecureConfig   │    │   PBKDF2 Hash   │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   MainWindow    │    │    DbManager     │    │   Database      │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## File Structure

```
utils/
├── secure_config.h/cpp      # Environment-based configuration
├── password_manager.h/cpp   # PBKDF2 password hashing
├── session_manager.h/cpp    # Session and activity management
└── secure_user_manager.h/cpp # Secure user operations

db/
└── dbmanager.cpp            # Updated for secure configuration

views/
└── login_dialog.cpp         # Updated for secure authentication

main.cpp                     # SessionManager integration
mainwindow.cpp/h             # Session UI and activity tracking
```

## Security Considerations

### Password Security
- Minimum 8 characters
- Requires uppercase, lowercase, digits, special characters
- PBKDF2 with 10,000 iterations
- Unique salt per password

### Session Security
- Automatic timeout on inactivity
- Session tokens are UUIDs
- Activity tracking on all user interactions
- Secure session storage

### Database Security
- No credentials in source code
- Environment-based configuration
- Connection pooling with proper cleanup
- SQL injection prevention

## Testing

Test the security implementation:

```bash
# Test environment variables
echo $DB_PASSWORD

# Test password hashing
./migrate_security --analyze

# Test session management
# (Login and wait for timeout)

# Test account lockout
# (Try wrong password 5 times)
```

## Production Deployment

For production deployment:

1. **Use strong passwords** for all accounts
2. **Secure environment variables** (use secrets management)
3. **Enable HTTPS** for web components
4. **Monitor failed login attempts**
5. **Regular security audits**
6. **Keep dependencies updated**

## Troubleshooting

### Common Issues

**Database connection failed:**
- Check environment variables
- Verify database is running
- Test connection manually

**Login failed:**
- Check if account is locked
- Verify password requirements
- Check migration completed

**Session expired immediately:**
- Check system clock
- Verify SESSION_TIMEOUT value
- Check application logs

### Debug Commands

```bash
# Check migration status
./migrate_security --analyze

# Verify security migration
./migrate_security --verify

# Check environment
env | grep DB_
env | grep APP_
```

## Support

For security issues or questions:
1. Check `SECURITY_MIGRATION_GUIDE.md`
2. Review application logs
3. Test with minimal configuration
4. Verify environment variables are set correctly