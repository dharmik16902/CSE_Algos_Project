#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "common.h"

SolveResult solve_bnb(const Graph &g, double cutoff);
SolveResult solve_approx(const Graph &g);
SolveResult solve_ls1(const Graph &g, double cutoff, unsigned int seed);
SolveResult solve_ls2(const Graph &g, double cutoff, unsigned int seed);

#endif
