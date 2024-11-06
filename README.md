# Register-renamaer-for-721sim
Author: Tejashree Kulkarni
University: North Carolina State University
Professor: Eric Rotenberg
Course: ECE 721 - Advance Microarchitecture
Duration: January 2023 - May 2023

# Project Overview
This project implements the register file and renaming mechanism for a modern superscalar microarchitecture. The renamer module is a C++ class designed to handle out-of-order execution within the pipeline by managing logical-to-physical register mappings. The project’s core functions and data structures are pre-defined, ensuring consistency across implementations while allowing customization of the underlying logic.

The renamer class interfaces with a pipeline simulator provided by the instructor, forming a complete processor model (named 721sim). The objective is to achieve accurate and efficient register renaming, including the management of the free list and active list of physical registers, and to closely match the performance metrics of the instructor’s solution.

# Components
renamer.h
Defines the structure and interface of the renamer class. It includes key data structures for register renaming, such as the free list, active list, and checkpoints. The interface functions manage register allocations and handle the commit and recovery of registers during normal execution and branch misprediction recovery.

renamer.cc
This file contains the implementation of the renamer class. It includes code for managing the renaming process, register allocation, and freeing mechanisms, along with the functionality for checkpointing and restoring states as required for out-of-order execution.

Makefile
The Makefile compiles the renamer files (renamer.h and renamer.cc) and links them with lib721sim.a to create the final 721sim executable. Running make in the directory will produce the 721sim binary.

glue.cc
A provided file that connects the renamer class with the pipeline simulator. No modifications are required in this file, as it solely facilitates the integration between the renamer module and the simulator’s core components.

lib721sim.a
A pre-compiled library containing the remaining components of the processor simulator, which are linked with the renamer class to form a fully functioning simulation of the pipeline.

# Conclusion
The Register Renamer for 721sim is a sophisticated module designed for efficient out-of-order execution within a superscalar pipeline. This project demonstrates core principles of advanced microarchitecture, including register renaming, speculative execution, and out-of-order instruction handling.
