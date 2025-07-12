#!/bin/bash

# Build script for security migration tool

echo "Building security migration tool..."

# Create build directory
mkdir -p migration_build
cd migration_build

# Run CMake for migration tool
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_MIGRATION_TOOL=ON

# Build the migration tool
make migrate_security

echo "Migration tool built successfully!"
echo "Usage: ./migration_build/migrate_security --help"