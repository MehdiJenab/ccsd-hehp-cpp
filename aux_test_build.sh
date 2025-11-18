#!/bin/bash
# Diagnostic script to debug test failures

echo "========================================="
echo "  CCSD Test Diagnostics"
echo "========================================="
echo ""

# Check if we're in build directory
if [ -f "../ccsd_code.cpp" ]; then
    echo "✓ Running from build directory"
    BUILD_DIR="."
    ROOT_DIR=".."
elif [ -f "./ccsd_code.cpp" ]; then
    echo "✓ Running from root directory"
    BUILD_DIR="./build"
    ROOT_DIR="."
else
    echo "✗ Cannot determine project structure"
    exit 1
fi

echo ""
echo "--- Checking executable ---"
if [ -f "$BUILD_DIR/ccsd_code" ]; then
    echo "✓ Executable exists: $BUILD_DIR/ccsd_code"
    ls -lh "$BUILD_DIR/ccsd_code"
else
    echo "✗ Executable not found in $BUILD_DIR"
    exit 1
fi

echo ""
echo "--- Checking for input files ---"
for file in *.json *.dat *.inp input* params*; do
    if [ -f "$file" ]; then
        echo "✓ Found: $file"
    fi
done

# Check in root directory too
if [ "$BUILD_DIR" != "." ]; then
    echo ""
    echo "--- Checking root directory for input files ---"
    for file in $ROOT_DIR/*.json $ROOT_DIR/*.dat $ROOT_DIR/*.inp $ROOT_DIR/input* $ROOT_DIR/params*; do
        if [ -f "$file" ]; then
            echo "✓ Found: $(basename $file)"
        fi
    done
fi

echo ""
echo "--- Testing executable with 2 processes ---"
echo "Command: mpirun -np 2 $BUILD_DIR/ccsd_code"
echo ""

# Capture both stdout and stderr
OUTPUT=$(mpirun -np 2 "$BUILD_DIR/ccsd_code" 2>&1)
EXIT_CODE=$?

echo "Exit code: $EXIT_CODE"
echo ""
echo "--- Output ---"
echo "$OUTPUT"
echo ""

if [ $EXIT_CODE -ne 0 ]; then
    echo "✗ Execution failed with exit code $EXIT_CODE"
    echo ""
    echo "--- Possible issues ---"
    echo "1. Missing input files (check if program needs .json, .dat, or other input)"
    echo "2. Program expects to run from specific directory"
    echo "3. MPI configuration issue"
    echo "4. Missing dependencies"
    echo ""
    
    # Check if error mentions file not found
    if echo "$OUTPUT" | grep -q -i "no such file\|cannot open\|not found"; then
        echo "⚠ Looks like a missing file issue"
        echo ""
        echo "Suggestion: Copy input files to build directory or run from root:"
        echo "  cd $ROOT_DIR"
        echo "  mpirun -np 2 $BUILD_DIR/ccsd_code"
    fi
else
    echo "✓ Execution successful"
    echo ""
    
    # Check if output contains expected patterns
    if echo "$OUTPUT" | grep -q "E(corr,CCSD)"; then
        echo "✓ Output contains E(corr,CCSD)"
        ECORR=$(echo "$OUTPUT" | grep "E(corr,CCSD)" | awk '{print $3}')
        echo "  Value: $ECORR"
    else
        echo "✗ Output does NOT contain E(corr,CCSD)"
    fi
    
    if echo "$OUTPUT" | grep -q "E(CCSD)"; then
        echo "✓ Output contains E(CCSD)"
        ECCSD=$(echo "$OUTPUT" | grep "E(CCSD)" | grep -v "corr" | awk '{print $3}')
        echo "  Value: $ECCSD"
    else
        echo "✗ Output does NOT contain E(CCSD)"
    fi
fi

echo ""
echo "--- MPI Information ---"
mpirun --version 2>&1 | head -3

echo ""
echo "--- Current directory contents ---"
ls -la

echo ""
echo "========================================="
echo "  Diagnostics Complete"
echo "========================================="
