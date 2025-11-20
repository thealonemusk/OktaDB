# Testing OktaDB

OktaDB includes a unit testing framework to ensure code stability and correctness. This document outlines how to run tests, where to find logs, and how to add new tests.

## Running Tests

You can run the unit tests using the build script (Windows) or Make (Linux/Mac).

### Windows (PowerShell)

```powershell
.\build.ps1 test
```

This command will:
1.  Compile the test runner (`test_runner.exe`).
2.  Run the tests.
3.  Display the output in the console.
4.  Save the output to a timestamped log file in `tests/log/`.

### Linux/macOS

```bash
make test
```

This command will compile and run the tests, displaying the output in the console.

## Test Logs

On Windows, test execution logs are automatically saved to the `tests/log/` directory. The filenames are timestamped for easy tracking:

```
tests/log/test_run_YYYYMMDD_HHMMSS.log
```

## Adding New Tests

The testing framework is located in the `tests/` directory and uses a minimal header-only framework (`minunit.h`).

### Steps to Add a Test

1.  **Create a new test file** (or add to an existing one) in `tests/`.
    *   Example: `tests/test_new_feature.c`
2.  **Include the necessary headers**:
    ```c
    #include "minunit.h"
    #include "../src/your_header.h"
    ```
3.  **Write test functions**:
    ```c
    static const char *test_my_feature() {
        mu_assert("error message if check fails", condition_to_check);
        return 0;
    }
    ```
4.  **Register the test** in `tests/test_main.c`:
    *   Add a forward declaration for your test suite runner (if separate).
    *   Or simply add your test functions to `all_tests()` if you are modifying `test_utility.c`.
    *   *Note: Currently, `test_main.c` calls `all_tests()` which is defined in `test_utility.c`. To scale this, you would typically create a header file for your new test suite and call its runner from `all_tests` or `main`.*

### Example Structure

```
tests/
├── minunit.h           # Testing framework
├── test_main.c         # Entry point
├── test_utility.c      # Tests for utility.c
└── log/                # Test logs
```
