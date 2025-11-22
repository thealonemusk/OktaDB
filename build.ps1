param(
    [string]$action = "build",
    [string]$mode = "debug"
)

$CC = "gcc"

if ($mode -eq "release") {
    $CFLAGS = "-Wall -Wextra -std=c11 -O2 -Isrc"
}
else {
    $CFLAGS = "-Wall -Wextra -std=c11 -g -Isrc"
}

# Use -mconsole on Windows, no special flags on Unix
$LDFLAGS = if ($IsWindows) { "-mconsole" } else { "" }
$SRC_DIR = "src"
$BUILD_DIR = "build"
$BIN_DIR = "bin"
$TARGET = "$BIN_DIR/oktadb.exe"

function Create-Directories {
    New-Item -ItemType Directory -Force -Path "$BUILD_DIR/storage" | Out-Null
    New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null
}

function Build-Project {
    Write-Host "Building OktaDB..." -ForegroundColor Green

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
    }
    else {
        & $CC $objectFiles -o $TARGET
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Build complete: $TARGET" -ForegroundColor Green
}

function Clean-Project {
    Write-Host "Cleaning build files..." -ForegroundColor Yellow
    if (Test-Path $BUILD_DIR) { Remove-Item -Recurse -Force $BUILD_DIR }
    if (Test-Path $BIN_DIR) { Remove-Item -Recurse -Force $BIN_DIR }
    if (Test-Path "test_runner.exe") { Remove-Item -Force "test_runner.exe" }
    if (Test-Path "test_results.log") { Remove-Item -Force "test_results.log" }
    if (Test-Path "tests/log") { Remove-Item -Recurse -Force "tests/log" }
    Write-Host "Clean complete" -ForegroundColor Green
}

function Run-Project {
    if (-not (Test-Path $TARGET)) {
        Write-Host "Building first..." -ForegroundColor Yellow
        Build-Project
    }
    Write-Host "Running OktaDB..." -ForegroundColor Green
    & $TARGET "test.db"
}

function Run-Tests {
    Write-Host "Building tests..." -ForegroundColor Green
    $testExe = "test_runner.exe"
    
    # Create log directory if it doesn't exist
    $logDir = "tests/log"
    if (-not (Test-Path $logDir)) {
        New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    }

    # Generate timestamped log filename
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $logFile = "$logDir/test_run_$timestamp.log"
    
    # Compile tests
    & $CC $CFLAGS.Split() -o $testExe tests/test_main.c tests/test_utility.c tests/test_db.c src/utility.c src/db_core.c src/hashtable.c
    
    if ($LASTEXITCODE -ne 0) { 
        Write-Host "Test compilation failed!" -ForegroundColor Red
        exit $LASTEXITCODE 
    }

    Write-Host "Running tests..." -ForegroundColor Green
    # Run tests and pipe output to both console and file
    & ./$testExe | Tee-Object -FilePath $logFile
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Tests failed!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    
    Write-Host "Test results saved to $logFile" -ForegroundColor Gray
}

switch ($action.ToLower()) {
    "build" { Build-Project }
    "clean" { Clean-Project }
    "run" { Run-Project }
    "test" { Run-Tests }
    "rebuild" { 
        Clean-Project
        Build-Project
    }
    default {
        Write-Host "Usage: .\build.ps1 [build|clean|run|test|rebuild] [debug|release]" -ForegroundColor Yellow
        Write-Host "  build   - Compile the project (default)"
        Write-Host "  clean   - Remove build files"
        Write-Host "  run     - Build and run the database"
        Write-Host "  test    - Build and run unit tests"
        Write-Host "  rebuild - Clean and build"
        Write-Host "  debug   - Build with debug flags (default)"
        Write-Host "  release - Build with optimization flags"
    }
}