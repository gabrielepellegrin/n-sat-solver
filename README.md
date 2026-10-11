# N-SAT Solver: The Impossible Catering

This repository contains a high-performance Boolean Satisfiability (SAT) solver written in C11. It was developed as the final project for the Algorithms and Data Structures course at Politecnico di Milano (Dipartimento di Elettronica, Informazione e Bioingegneria). The application evaluates the satisfiability of propositional logic formulas efficiently, strictly relying on the standard C library without external dependencies or multithreading.

## Problem Overview

The core objective is to solve the "Catering Impossibile" (Impossible Catering) problem. The scenario requires organizing a company dinner by finding a menu that fulfills the categorical requests of all participating employees.

* Each request operates as a Boolean clause representing a list of culinary options; at least one option per employee must be fulfilled.
* Employees are processed based on a strict hierarchical order provided in the input data.
* In cases where a global solution is mathematically unsatisfiable, the solver gracefully degrades the constraints by iteratively discarding the requests of the lowest-ranking employees until a satisfiable menu state is achieved.

## Algorithmic Architecture

The solver translates the logic into a Conjunctive Normal Form (CNF) formula and employs a custom engine inspired by modern SAT-solving techniques:

* **Conflict-Driven Clause Learning (CDCL):** Implements a non-chronological backtracking algorithm capable of analyzing and learning from conflicts (`conflict` and `learn` functions). This aggressively prunes the decision tree by appending learned clauses to the constraint set.


* **Two Watched Literals (TWL):** Employs the TWL data structure (`watch1`, `watch2`) to optimize Boolean Constraint Propagation (BCP). This prevents unnecessary traversals of clauses when variables are assigned.


* **VSIDS Branching Heuristic:** Utilizes the Variable State Independent Decaying Sum heuristic (`vbump`, `vdecay`) to dynamically prioritize the evaluation of variables frequently involved in logical conflicts. The scores are maintained within a custom Max-Heap implementation (`hup`, `hdown`) for constant-time maximum retrieval.


* **Custom Hash Map:** To handle string-based dish names efficiently, a fast, custom hashing algorithm maps input strings to internal integer IDs, avoiding standard library string lookup overhead during the solving phase.


* **Contiguous Memory Pooling:** Prevents memory fragmentation and allocation overhead by pre-allocating large blocks of memory (`dpool`, `literals`, `starts` arrays) and dynamically scaling them only when capacity limits are breached.



## Input and Output Specifications

Data is processed entirely via standard input and standard output streams.

* **Input Data:** The initial line defines the global set of available dishes, separated by spaces. Every subsequent line details an individual employee's request clause, ordered from highest to lowest rank. A minus sign (`-`) preceding a dish indicates a negative constraint (the employee specifically requests the dish not be served).
* **Standard Output:** The algorithm outputs `OK` if all constraints are universally satisfied. If unsatisfiable, it outputs `KO`, followed by iterative markers (e.g., `-1`, `-2`) indicating which hierarchical levels were dropped, culminating in an `OK` once the subset of constraints becomes satisfiable.

## Compilation and Execution

To compile and execute the solver locally on a POSIX-compliant system:

```bash
# Compilation using C11 standard and heavy optimizations
gcc -O3 -std=c11 -Wall -Wextra -o nsatsolver goat.c

# Execution
./nsatsolver < input.txt

```
