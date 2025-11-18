# CCSD in C++ with MPI Parallelization

This repository provides a C++ implementation of Coupled Cluster with Singles
and Doubles (CCSD) for the HeH⁺ molecule, translated from the educational Python
code published by Joshua Goings. The translation mirrors the mathematical
structure and loop ordering of the reference so that numerical comparisons are
straightforward.

## References

- T. Daniel Crawford and Henry F. Schaefer III, “An introduction to coupled
  cluster theory for computational chemists,” *Reviews in Computational
  Chemistry*, vol. 14, pp. 33–136, 2007.
- J. Goings, “Coupled Cluster with Singles and Doubles (CCSD) in Python,”
  https://joshuagoings.com/2013/07/17/coupled-cluster-with-singles-and-doubles-ccsd-in-python/

## Overview

The code computes CCSD correlation energies for HeH⁺ using:

- Custom 2D/4D tensor containers
- JSON input via Jansson (optional)
- MPI parallelization through OpenMPI
- A direct, readable transcription of the CCSD amplitude equations

## Repository Structure

- `ccsd_code.cpp`
- `ParameterClass.h`, `ParameterClass_NoJson.h`, `VectorsClass.h`, `MpiClass.h`
- `config.json`
- `tests/run_mpi_regression.py`
- Build tooling: `Makefile`, `CMakeLists.txt`

## Dependencies

- C++17-capable compiler
- OpenMPI (provides `mpic++`, `mpirun`)
- [Jansson](https://digip.org/jansson/) (only when `USE_JSON` is enabled)
- CMake ≥ 3.12 (if using the CMake workflow)
 - Python 3 (optional, for regression script)

## Build Options

Two build systems are available; both expose similar configuration knobs so you
can pick the one that best fits your workflow.

### Quick Start

```
# Make
make

# or CMake (out-of-source)
cmake -S . -B build
cmake --build build
```

### Makefile workflow

- `make` builds `ccsd_code` with JSON support.
- `make run` launches `mpirun --oversubscribe -np 4 ./ccsd_code`.
- `make test` runs the Python regression script (see below).
- `make clean` removes the executable.

Override toolchain/flags as needed:

```
make CXX=mpicxx CXXFLAGS="-O3 -std=c++20" USE_JSON=0
```

Setting `USE_JSON=0` omits the `-ljansson` link; useful if you temporarily build
against `ParameterClass_NoJson.h`.

### CMake workflow

```
cmake -S . -B build -DUSE_JSON=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Key cache variables:

- `USE_JSON` (ON by default) – links against Jansson and defines `USE_JSON`.
- `BUILD_TESTING` (ON by default) – enables `ctest` integration and copies
  `config.json` plus extra input files to the build directory.
- Standard CMake knobs such as `CMAKE_CXX_COMPILER`, `CMAKE_CXX_FLAGS`, and
  `CMAKE_BUILD_TYPE` behave as usual. Example:

```
cmake -S . -B build -DCMAKE_CXX_COMPILER=mpicxx -DCMAKE_BUILD_TYPE=Debug -DUSE_JSON=OFF
cmake --build build --target ccsd_code
```

After configuring, run tests via:

```
cmake --build build
cd build && ctest
```

CMake defines execution/validation tests for multiple MPI sizes, optionally
oversubscribes when cores are scarce, and can run the Python regression script
if Python is available.

## Running

```
mpirun -np 4 ./ccsd_code
```

On machines with fewer slots than requested ranks, pass `--oversubscribe` so
OpenMPI allows the extra processes.

## Automated Testing

```
python3 tests/run_mpi_regression.py
# or
make test
# or
(cd build && ctest)    # after configuring with CMake
```

The regression script and the CTest configuration both run the executable with
multiple process counts (2, 4, 8 by default), using `--oversubscribe` to keep
the tests portable. They assert that the reported correlation and total energies
match the established reference values; command-line arguments (`--np`,
`--executable`, `--tol`, etc.) let you customize the checks.

## Input Format (`config.json`)

```
{
    "dim": ...,
    "Nelec": ...,
    "orbital_energy": [...],
    "ENUC": ...,
    "EN": ...,
    "integrals": [[i,j,k,l,value], ...]
}
```

## License

Original C++ translation. Does not include Python code. MIT License.
