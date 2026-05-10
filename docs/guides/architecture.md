# CCSD Code Architecture

## Overview

This document describes the architecture of the CCSD (Coupled-Cluster Singles and Doubles) MPI implementation. The code is designed for high-performance parallel computation of quantum chemistry calculations.

## Design Principles

1. **Modularity**: Separation of concerns through distinct classes
2. **RAII**: Resource management through constructors/destructors
3. **Performance**: Optimized for MPI parallel execution
4. **Maintainability**: Clean interfaces and comprehensive documentation

## Architecture Diagram
```
┌─────────────────────────────────────────────────────────┐
│                    Main Application                      │
│                    (ccsd_code.cpp)                       │
└───────────┬─────────────────────────────────┬───────────┘
            │                                 │
            ▼                                 ▼
    ┌───────────────┐                ┌──────────────┐
    │   MpiClass    │                │ ParameterClass│
    │               │                │               │
    │ - rank        │                │ - config.json │
    │ - size        │                │ - parameters  │
    │ - init/final  │                │               │
    └───────────────┘                └──────────────┘
            │                                 │
            │         ┌──────────────┐        │
            └────────►│ VectorsClass │◄───────┘
                      │              │
                      │ - T1 vectors │
                      │ - T2 vectors │
                      │ - MPI dist.  │
                      └──────────────┘
```

## Core Components

### 1. MpiClass
**Responsibility**: MPI initialization and management

**Key Features**:
- RAII-based MPI lifecycle management
- Rank and size information
- Master process identification

**Dependencies**: MPI library

### 2. ParameterClass
**Responsibility**: Configuration and parameter management

**Key Features**:
- JSON-based configuration (optional)
- Molecular system parameters
- Basis set information

**Dependencies**: jansson library (optional)

### 3. VectorsClass
**Responsibility**: Tensor storage and distribution

**Key Features**:
- T1 and T2 amplitude storage
- MPI-based data distribution
- Memory-efficient tensor operations

**Dependencies**: MpiClass, ParameterClass

### 4. Main Application (ccsd_code.cpp)
**Responsibility**: CCSD algorithm implementation

**Key Features**:
- Iterative CCSD solver
- Energy calculation
- Convergence checking
- Result output

**Dependencies**: All above classes

## Data Flow
```
1. Configuration Loading
   config.json → ParameterClass → System Parameters

2. MPI Initialization
   MpiClass.init() → Rank/Size Distribution

3. Vector Setup
   ParameterClass → VectorsClass → Distributed Tensors

4. CCSD Iteration
   while (!converged):
     - Compute intermediates
     - Update T1, T2 amplitudes
     - Calculate energy
     - Check convergence
     - MPI synchronization

5. Results
   Energy → Master Process → Output
```

## Parallel Strategy

### Domain Decomposition
- T1 and T2 amplitudes distributed across MPI processes
- Each process handles a subset of tensor elements
- Load balancing based on process rank

### Communication Pattern
- **Collective Operations**: Energy reduction, convergence check
- **Point-to-Point**: Tensor element exchange (if needed)
- **Synchronization**: Barrier before energy calculation

### Scalability
- Designed for 2-16 processes
- Oversubscription supported for testing
- Communication overhead minimized

## File Organization
```
ccsd-hehp-cpp/
├── ccsd_code.cpp           # Main CCSD implementation
├── MpiClass.h              # MPI wrapper
├── ParameterClass.h        # Config with JSON
├── ParameterClass_NoJson.h # Config without JSON
├── VectorsClass.h          # Tensor storage
├── config.json             # Input configuration
├── CMakeLists.txt          # Build system
├── Makefile                # Alternative build
├── Doxyfile                # Documentation config
├── tests/                  # Test suite
│   ├── run_single_test.sh
│   ├── run_all_tests.sh
│   └── run_mpi_regression.py
└── docs/                   # Documentation
    └── architecture.md     # This file
```

## Build System

### CMake Integration
- Automatic dependency detection (MPI, jansson)
- CTest integration for automated testing
- Doxygen documentation generation
- Configurable build types (Debug/Release)

### Testing Infrastructure
- Unit tests for individual components
- Integration tests with different MPI configurations
- Regression tests for numerical accuracy
- Performance benchmarks

## Design Patterns Used

### 1. RAII (Resource Acquisition Is Initialization)
- **MpiClass**: MPI_Init in constructor, MPI_Finalize in destructor
- Ensures proper cleanup even with exceptions

### 2. Factory Pattern (implicit)
- VectorsClass creates distributed tensors based on parameters

### 3. Strategy Pattern (potential)
- Configurable solver strategies
- Different parameter readers (JSON vs. NoJSON)

## Performance Considerations

### Memory Management
- Stack allocation where possible
- Careful tensor sizing to avoid memory exhaustion
- Distributed storage across processes

### Computation
- Minimize MPI communication
- Vectorizable operations where possible
- Cache-friendly data access patterns

### I/O
- Master process handles file operations
- Minimal output during iteration
- Configurable verbosity

## Future Enhancements

1. **Checkpointing**: Save/restore intermediate states
2. **Alternative Methods**: CCSD(T), EOM-CCSD
3. **Better Load Balancing**: Dynamic work distribution
4. **GPU Acceleration**: CUDA/HIP backends
5. **Multiple Molecules**: Configuration-driven systems
6. **Performance Profiling**: Built-in timing instrumentation

## References

- MPI Standard: https://www.mpi-forum.org/
- CCSD Theory: Quantum chemistry literature
- Best Practices: Modern C++ Core Guidelines

## Contributing

When contributing to this codebase:
1. Follow existing code style
2. Add Doxygen comments for new functions/classes
3. Include unit tests for new features
4. Update this architecture document for major changes
5. Run full test suite before submitting
