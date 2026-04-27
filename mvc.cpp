#include "algorithms.h"
#include "graph_ops.h"
#include "io.h"

using namespace std;

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    Options opt;
    if (!parse_args(argc, argv, opt)) {
        cerr << usage() << "\n";
        return 1;
    }

    Graph g;
    try {
        g = read_graph(opt.instance_path);
    } catch (const exception &ex) {
        print_error_and_exit(ex.what());
    }

    SolveResult result;
    if (opt.algorithm == "BnB") {
        result = solve_bnb(g, opt.cutoff);
    } else if (opt.algorithm == "Approx") {
        result = solve_approx(g);
    } else if (opt.algorithm == "LS1") {
        result = solve_ls1(g, opt.cutoff, opt.seed);
    } else if (opt.algorithm == "LS2") {
        result = solve_ls2(g, opt.cutoff, opt.seed);
    } else {
        print_error_and_exit("Unsupported algorithm.");
    }

    if (result.cover.empty()) {
        result.cover.assign(g.n + 1, 0);
    }

    if (!is_valid_cover(g, result.cover)) {
        print_error_and_exit("Internal error: generated cover is invalid.");
    }

    string base = opt.instance_name + "_" + opt.algorithm + "_" + opt.cutoff_label;
    if (opt.algorithm == "LS1" || opt.algorithm == "LS2") {
        ostringstream seed_stream;
        seed_stream << opt.seed;
        base += "_" + seed_stream.str();
    }

    write_solution_file(base + ".sol", result.cover);
    if (opt.algorithm == "BnB" || opt.algorithm == "LS1" || opt.algorithm == "LS2") {
        write_trace_file(base + ".trace", result.trace);
    }

    return 0;
}
