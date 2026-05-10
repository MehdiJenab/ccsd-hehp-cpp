# CCSD Build and Test Instructions

## Quick Start

### Using Makefile
```bash
# Build the project
make

# Run quick test
make test-quick

# Run all tests
make test-all

# Clean build artifacts
make clean
```

### Using CMake
```bash
# Configure and build
mkdir build && cd build
cmake ..
make

# Run tests
ctest

# Run tests with verbose output
ctest --verbose

# Run specific test
ctest -R ccsd_test_np4

# Return to project root
cd ..
```

## Detailed Instructions

### Makefile Build System

#### Build Options
```bash
# Standard build
make

# Build without JSON support
make USE_JSON=0

# Build with custom compiler flags
make CXXFLAGS="-O3 -march=native"

# Build with debug symbols
make CXXFLAGS="-g -O0"
```

#### Running Tests
```bash
# Quick single test (np=4)
make test-quick

# Full test suite (np=2,4,8)
make test-all

# Python regression tests (if available)
make test

# Run application manually
make run
```

#### Available Targets
- `all` - Build the application (default)
- `run` - Build and run with 4 MPI processes
- `test-quick` - Run quick single test
- `test` - Run Python regression tests
- `test-all` - Run comprehensive test suite
- `clean` - Remove build artifacts
- `install` - Install to /usr/local/bin
- `help` - Show help message

### CMake Build System

#### Configuration Options
```bash
# Standard configuration
cmake -B build

# Specify build type
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Disable JSON support
cmake -B build -DUSE_JSON=OFF

# Disable testing
cmake -B build -DBUILD_TESTING=OFF

# Custom compiler
cmake -B build -DCMAKE_CXX_COMPILER=mpic++

# Custom install prefix
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/ccsd
```

#### Building
```bash
# Build from build directory
cd build
make

# Or build from project root
cmake --build build

# Build with multiple cores
cmake --build build -j8
```

#### Testing with CTest
```bash
cd build

# Run all tests
ctest

# Verbose output
ctest --verbose
ctest -V

# Run specific tests
ctest -R validation     # Run validation tests only
ctest -R np4           # Run np=4 tests only
ctest -L execution     # Run tests with 'execution' label

# Show test details
ctest --output-on-failure

# Run tests in parallel
ctest -j4

# Repeat tests (useful for timing)
ctest --repeat until-fail:10
```

#### Installation
```bash
# Install to configured prefix
cd build
sudo make install

# Or using cmake
cd build
sudo cmake --install .
```

## Test Structure

### Test Files
```
tests/
├── run_single_test.sh       # Single test runner
├── run_all_tests.sh         # Comprehensive test suite
└── run_mpi_regression.py    # Python regression tests (optional)
```

### Creating Test Directory
```bash
# Create tests directory
mkdir -p tests

# Copy test scripts (make them executable)
chmod +x tests/run_single_test.sh
chmod +x tests/run_all_tests.sh
```

## Expected Test Output

### Successful Test
```
Running CCSD test with 4 MPI processes...
✓ PASSED
  E(corr,CCSD) = -0.008225832259 (expected: -0.008225832259)
  E(CCSD) = -2.862598243 (expected: -2.862598243)
```

### Test Summary
```
=========================================
  Test Summary
=========================================
Total tests: 3
Passed: 3
Failed: 0
Success rate: 100.0%

✓ All tests passed!
```

## Troubleshooting

### MPI Not Found
```bash
# Install MPI (Ubuntu/Debian)
sudo apt-get install libopenmpi-dev openmpi-bin

# Install MPI (Fedora/RHEL)
sudo dnf install openmpi-devel

# Install MPI (macOS)
brew install open-mpi
```

### Jansson Not Found
```bash
# Install jansson (Ubuntu/Debian)
sudo apt-get install libjansson-dev

# Install jansson (Fedora/RHEL)
sudo dnf install jansson-devel

# Install jansson (macOS)
brew install jansson

# Or build without JSON
make USE_JSON=0
# or
cmake -B build -DUSE_JSON=OFF
```

### Test Failures
```bash
# Check executable exists
ls -l ccsd_code

# Run manually to see output
mpirun -np 4 ./ccsd_code

# Check MPI installation
mpirun --version

# Verify number of available cores
nproc
```

### Permission Denied
```bash
# Make test scripts executable
chmod +x tests/*.sh

# Make sure you can run mpirun
which mpirun
```

## Continuous Integration

### GitHub Actions Example
```yaml
name: CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libopenmpi-dev libjansson-dev
      - name: Build with Makefile
        run: make
      - name: Run tests
        run: make test-all
      - name: Build with CMake
        run: |
          mkdir build && cd build
          cmake ..
          make
      - name: Run CTest
        run: cd build && ctest --verbose
```

## Performance Notes

- Use `-O3` for maximum optimization: `make CXXFLAGS="-O3 -march=native"`
- For debugging, use: `make CXXFLAGS="-g -O0"`
- Test with different process counts to verify scaling
- The `--oversubscribe` flag allows more processes than cores (useful for testing)

## Additional Resources

- MPI Documentation: https://www.open-mpi.org/doc/
- CMake Documentation: https://cmake.org/documentation/
- CTest Documentation: https://cmake.org/cmake/help/latest/manual/ctest.1.html
