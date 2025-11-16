# build.ps1 - PowerShell build script
param(
    [string]$action = "build",
    [string]$mode = "debug"
)

$CC = "gcc"

if ($mode -eq "release") {
    $CFLAGS = "-Wall -Wextra -std=c11 -O2 -Isrc"
} else {
    $CFLAGS = "-Wall -Wextra -std=c11 -g -Isrc"
}

$LDFLAGS = "-mconsole"
$SRC_DIR = "src"
$BUILD_DIR = "build"
$BIN_DIR = "bin"
$TARGET = "$BIN_DIR/mydb.exe"

function Create-Directories {
    New-Item -ItemType Directory -Force -Path "$BUILD_DIR/storage" | Out-Null
    New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null
}

function Build-Project {
    Write-Host "Building MyDB..." -ForegroundColor Green

    Create-Directories

    # Compile main.c
    Write-Host "Compiling main.c..."
    & $CC $CFLAGS.Split() -c "$SRC_DIR/main.c" -o "$BUILD_DIR/main.o"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    # Compile storage.c
    Write-Host "Compiling storage.c..."
    & $CC $CFLAGS.Split() -c "$SRC_DIR/storage/storage.c" -o "$BUILD_DIR/storage.o"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    # Compile types.c
    Write-Host "Compiling types.c..."
    & $CC $CFLAGS.Split() -c "$SRC_DIR/common/custom.c" -o "$BUILD_DIR/custom.o"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    # Link
    Write-Host "Linking..."
    & $CC "$BUILD_DIR/main.o" "$BUILD_DIR/storage.o" "$BUILD_DIR/custom.o" $LDFLAGS.Split() -o $TARGET
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Build complete: $TARGET" -ForegroundColor Green
}

function Clean-Project {
    Write-Host "Cleaning build files..." -ForegroundColor Yellow
    if (Test-Path $BUILD_DIR) { Remove-Item -Recurse -Force $BUILD_DIR }
    if (Test-Path $BIN_DIR) { Remove-Item -Recurse -Force $BIN_DIR }
    Write-Host "Clean complete" -ForegroundColor Green
}

function Run-Project {
    if (-not (Test-Path $TARGET)) {
        Write-Host "Building first..." -ForegroundColor Yellow
        Build-Project
    }
    Write-Host "Running MyDB..." -ForegroundColor Green
    & $TARGET "test.db"
}

switch ($action.ToLower()) {
    "build" { Build-Project }
    "clean" { Clean-Project }
    "run" { Run-Project }
    "rebuild" { 
        Clean-Project
        Build-Project
    }
    default {
        Write-Host "Usage: .\build.ps1 [build|clean|run|rebuild] [debug|release]" -ForegroundColor Yellow
        Write-Host "  build   - Compile the project (default)"
        Write-Host "  clean   - Remove build files"
        Write-Host "  run     - Build and run the database"
        Write-Host "  rebuild - Clean and build"
        Write-Host "  debug   - Build with debug flags (default)"
        Write-Host "  release - Build with optimization flags"
    }
}