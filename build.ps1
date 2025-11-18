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

# Use -mconsole on Windows, no special flags on Unix
$LDFLAGS = if ($IsWindows) { "-mconsole" } else { "" }
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

    # Find all .c files in the src directory
    $sourceFiles = Get-ChildItem -Path "$SRC_DIR" -Recurse -Filter "*.c"

    $objectFiles = @()

    foreach ($file in $sourceFiles) {
        $relativePath = $file.FullName.Substring($SRC_DIR.Length + 1)
        $objectFile = "$BUILD_DIR/" + $relativePath.Replace("\\", "/").Replace(".c", ".o")
        $objectDir = Split-Path -Path $objectFile -Parent

        # Ensure the directory for the object file exists
        if (-not (Test-Path $objectDir)) {
            New-Item -ItemType Directory -Force -Path $objectDir | Out-Null
        }

        # Compile the .c file
        Write-Host "Compiling $($file.FullName)..."
        & $CC $CFLAGS.Split() -c $file.FullName -o $objectFile
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

        $objectFiles += $objectFile
    }

    # Link all object files
    Write-Host "Linking..."
    if ($LDFLAGS) {
        & $CC $objectFiles -o $TARGET $LDFLAGS.Split()
    } else {
        & $CC $objectFiles -o $TARGET
    }
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