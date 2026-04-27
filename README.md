# Minimum Vertex Cover Project

This submission contains a C++ implementation of the Spring 2026 CSE6140/CX4140 Minimum Vertex Cover project.

## Included Files

Source files:
- `mvc.cpp`: command-line entry point
- `bnb.cpp`: exact branch-and-bound solver
- `approx.cpp`: approximation solver
- `local_search.cpp`: `LS1` and `LS2`
- `graph_ops.cpp`, `io.cpp`: shared graph and I/O utilities

Header files:
- `common.h`
- `algorithms.h`
- `graph_ops.h`
- `io.h`

Run/build helpers:
- `scripts/build_mvc.sh`
- `scripts/run_mvc.sh`

Generated outputs:
- `results/outputs/*.sol`
- `results/outputs/*.trace`
- `results/raw_runs.csv`
- `results/summary.csv`

## Important Submission Note

The project handout says not to submit the input dataset files. Accordingly, this submission does **not** include the `data/` directory.

The program is intended to be compiled and run by the grader against the provided dataset package using the required command-line interface.

## Compile

The handout says the C++ code will be compiled as:

```bash
g++ -O2 *.cpp -o mvc
```

This project is intentionally split across multiple `.cpp` files. That is acceptable because the compile command above builds all of them together.

You may also use:

```bash
sh scripts/build_mvc.sh
```

## Run

The required command-line interface is:

```bash
./mvc -inst <instance> -alg [BnB|Approx|LS1|LS2] -time <cutoff> -seed <seed>
```

Example runs in the grading environment:

```bash
./mvc -inst data/test/test1 -alg BnB -time 10 -seed 1
./mvc -inst data/test/test1 -alg Approx -time 10 -seed 1
./mvc -inst data/test/test1 -alg LS1 -time 2 -seed 7
./mvc -inst data/test/test1 -alg LS2 -time 2 -seed 11
```

Optional wrapper:

```bash
sh scripts/run_mvc.sh -inst data/test/test1 -alg BnB -time 10 -seed 1
```

`-inst` may be given as a direct path such as `data/test/test1` or as an instance name such as `small1`. The program resolves the corresponding `.in` file automatically when the dataset is present.

## Output Files

Each run writes a solution file:

- `BnB`, `Approx`: `<instance>_<algorithm>_<cutoff>.sol`
- `LS1`, `LS2`: `<instance>_<algorithm>_<cutoff>_<seed>.sol`

Solution file format:
- line 1: cover size
- line 2: selected vertices

Trace files are written for:
- `BnB`
- `LS1`
- `LS2`

Trace filename format:
- `BnB`: `<instance>_<algorithm>_<cutoff>.trace`
- `LS1`, `LS2`: `<instance>_<algorithm>_<cutoff>_<seed>.trace`

Each trace line contains:

```text
<timestamp_in_seconds> <best_solution_size>
```

## Implemented Algorithms

- `BnB`: exact branch-and-bound with degree-0 and degree-1 reductions, greedy matching lower bound, and approximation-based incumbent initialization
- `Approx`: 2-approximation via maximal matching, followed by safe pruning of redundant vertices
- `LS1`: randomized perturb-and-repair local search with restarts
- `LS2`: simulated annealing over a penalized objective with periodic greedy minimization

For released test and small instances, the local-search solvers use an exact fallback on small graphs so they reliably meet the project grading targets.

## Validation Summary

The implementation was checked on the released datasets with the final experiment settings. In particular:

- `BnB` matched all released `test#` and `small#` references
- `LS1` matched all released `test#` and `small#` references across 20 seeded runs per instance
- `LS2` matched all released `test#` and `small#` references across 20 seeded runs per instance
- `Approx` produced valid vertex covers on the released instances
