#include "algorithms.h"
#include "graph_ops.h"

// Return the approximation algorithm result in the common solver format.
SolveResult solve_approx(const Graph &g) {
    SolveResult result;
    result.cover = approx_cover(g);
    result.exact = false;
    return result;
}
