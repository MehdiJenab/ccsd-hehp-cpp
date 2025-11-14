# CCSD in C++ with MPI Parallelization

This repository provides a C++ implementation of Coupled Cluster with Singles and Doubles (CCSD) for the HeH⁺ molecule, translated from the educational Python code published by Joshua Goings on his CCSD tutorial webpage.
The implementation closely follows the mathematical structure and loop organization used in Goings’ Python reference, enabling transparent comparison between the two versions.

## References

T. Daniel Crawford and Henry F. Schaefer III, “An introduction to coupled cluster theory for computational chemists,” *Reviews in Computational Chemistry*, vol. 14, pp. 33–136, 2007.

Original Python tutorial:
- J. Goings, “Coupled Cluster with Singles and Doubles (CCSD) in Python,” https://joshuagoings.com/2013/07/17/coupled-cluster-with-singles-and-doubles-ccsd-in-python/

## Overview

The code computes the CCSD correlation energy for HeH⁺ using:
- Custom 2D and 4D tensor containers
- JSON input via Jansson
- Optional MPI parallelization
- Direct implementation of the CCSD iterative equations

## Repository Structure

ccsd_code.cpp
include/
  ParameterClass.h
  ParameterClass_NoJson.h
  VectorsClass.h
  MpiClass.h
config.json

## Dependencies

- C++17 or newer
- OpenMPI
- Jansson library

## Compiling

With JSON:
mpic++ ccsd_code.cpp -o ccsd_code -ljansson

Without JSON:
replace ParameterClass.h with ParameterClass_NoJson.h
mpic++ ccsd_code.cpp -o ccsd_code

## Running

mpirun -np 4 ./ccsd_code

## Input Format (config.json)

{
    "dim": ...,
    "Nelec": ...,
    "orbital_energy": [...],
    "ENUC": ...,
    "EN": ...,
    "integrals": [[i,j,k,l,value], ...]
}

## License

Original C++ translation. Does not include Python code. MIT License.
