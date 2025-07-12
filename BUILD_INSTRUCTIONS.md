# Build Instructions for EtykietyManager with Security

## Prerequisites

### Required Tools
- **CMake** 3.16 or higher
- **Qt6** development libraries
- **C++ compiler** (GCC 7+ or MSVC 2019+)
- **PostgreSQL** development libraries (optional)

### Windows Installation
```powershell
# Install Qt6
winget install Qt.Qt.6

# Install CMake
winget install Kitware.CMake

# Install Visual Studio Build Tools
winget install Microsoft.VisualStudio.2022.BuildTools
```

### Linux Installation
```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-sql-dev qt6-network-dev qt6-pdf-dev cmake build-essential libpq-dev

# Fedora/CentOS
sudo dnf install qt6-qtbase-devel qt6-qtbase-private-devel cmake gcc-c++ postgresql-devel
```

## Build Process

### Step 1: Environment Setup
```bash
# Set environment variables
source setup_env.sh

# Verify variables are set
echo "Database: $DB_TYPE at $DB_HOST:$DB_PORT/$DB_NAME"
echo "Session timeout: $SESSION_TIMEOUT seconds"
```

### Step 2: Configure Build
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Windows with Visual Studio
cmake .. -G "Visual Studio 17 2022" -A x64
```

### Step 3: Build Application
```bash
# Linux/macOS
make -j$(nproc)

# Windows
cmake --build . --config Release
```

### Step 4: Build Migration Tool
```bash
# From project root
mkdir migration_build && cd migration_build
cmake .. -DCMAKE_BUILD_TYPE=Release -f ../migration_CMakeLists.txt
make migrate_security
```

## Security Integration Checklist

### ✅ Files Added
- [x] `utils/secure_config.h/cpp`
- [x] `utils/password_manager.h/cpp`  
- [x] `utils/session_manager.h/cpp`
- [x] `utils/secure_user_manager.h/cpp`
- [x] `setup_env.sh` / `setup_env.bat`
- [x] `migrate_security.cpp`

### ✅ Files Modified
- [x] `db/dbmanager.cpp` - Uses SecureConfig
- [x] `main.cpp` - SessionManager integration
- [x] `mainwindow.cpp/h` - Session UI
- [x] `views/login_dialog.cpp` - Secure auth
- [x] `CMakeLists.txt` - New dependencies

### ✅ Files Deprecated
- [x] `config.ini` → `config.ini.deprecated`

## Testing Build

### 1. Verify Security Components
```bash
# Check security symbols in binary
nm EtykietyManager | grep -i secure
nm EtykietyManager | grep -i session
nm EtykietyManager | grep -i password
```

### 2. Test Database Connection
```bash
# Test with environment variables
./EtykietyManager --test-db-connection
```

### 3. Run Migration Tool
```bash
# Analyze current state
./migrate_security --analyze

# Run migration
./migrate_security

# Verify results
./migrate_security --verify
```

## Common Build Issues

### Missing Qt6 Components
```bash
# Check available Qt6 packages
apt list --installed | grep qt6

# Install missing components
sudo apt install qt6-pdf-dev qt6-webengine-dev
```

### PostgreSQL Issues
```bash
# Test database connection
psql -h localhost -p 5432 -U postgres -d etykiety_db

# Install PostgreSQL development headers
sudo apt install libpq-dev
```

### CMake Configuration Issues
```bash
# Clear CMake cache
rm -rf CMakeCache.txt CMakeFiles/

# Reconfigure with verbose output
cmake .. -DCMAKE_BUILD_TYPE=Release --verbose
```

## Production Build

### Optimized Release Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG" \
         -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Static Linking (if needed)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++"
```

## Next Steps After Build

1. **Run Migration**: `./migrate_security`
2. **Test Login**: Start app and test authentication
3. **Test Sessions**: Verify timeout and logout
4. **Test Security**: Try account lockout
5. **Deploy**: Follow production deployment guide

## Support

If build fails:
1. Check all prerequisites are installed
2. Verify Qt6 version compatibility
3. Check environment variables are set
4. Review CMake configuration output
5. Test database connection separately