# Usage Guide

## Building the Project

### Prerequisites
- C++17 compatible compiler
- MPI implementation (OpenMPI or MPICH)
- CMake 3.12+
- jansson library (optional)

### Build Steps

\`\`\`bash
mkdir build && cd build
cmake ..
make
\`\`\`

## Running CCSD Calculations

### Basic Execution

\`\`\`bash
mpirun -np 4 ./ccsd_code
\`\`\`

### With Different Process Counts

\`\`\`bash
# 2 processes
mpirun -np 2 ./ccsd_code

# 8 processes (with oversubscribe)
mpirun --oversubscribe -np 8 ./ccsd_code
\`\`\`

## Configuration

Edit \`config.json\` to modify molecular system parameters.

## Testing

\`\`\`bash
cd build
ctest --verbose
\`\`\`
