#include "io.h"

using namespace std;

// Small helper for trying multiple candidate paths when resolving an instance name.
static bool file_exists(const string &path) {
    ifstream in(path.c_str());
    return in.good();
}

// Extract the last path component so output filenames use only the dataset instance name.
static string basename_path(const string &path) {
    const size_t pos = path.find_last_of("/\\");
    return pos == string::npos ? path : path.substr(pos + 1);
}

// Drop a trailing extension such as ".in" before generating output filenames.
static string strip_extension(const string &name) {
    const size_t pos = name.find_last_of('.');
    return pos == string::npos ? name : name.substr(0, pos);
}

// Resolve user-facing instance arguments against the submission's expected dataset layout.
static string resolve_instance_path(const string &instance_arg) {
    vector<string> candidates;
    if (!instance_arg.empty()) {
        candidates.push_back(instance_arg);
        if (instance_arg.size() < 3 || instance_arg.substr(instance_arg.size() - 3) != ".in") {
            candidates.push_back(instance_arg + ".in");
        }
        const string base = strip_extension(basename_path(instance_arg));
        candidates.push_back(base + ".in");
        candidates.push_back("data/test/" + base + ".in");
        candidates.push_back("data/small/" + base + ".in");
        candidates.push_back("data/large/" + base + ".in");
    }

    for (size_t i = 0; i < candidates.size(); ++i) {
        if (file_exists(candidates[i])) {
            return candidates[i];
        }
    }
    return "";
}

string usage() {
    return "Usage: ./mvc -inst <instance> -alg [BnB|Approx|LS1|LS2] -time <cutoff> -seed <seed>";
}

// Parse and validate the exact CLI contract.
bool parse_args(int argc, char **argv, Options &opt) {
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            return false;
        }
        const string key = argv[i];
        const string value = argv[i + 1];
        if (key == "-inst") {
            opt.instance_arg = value;
        } else if (key == "-alg") {
            opt.algorithm = value;
        } else if (key == "-time") {
            opt.cutoff_label = value;
            opt.cutoff = atof(value.c_str());
        } else if (key == "-seed") {
            opt.seed = static_cast<unsigned int>(strtoul(value.c_str(), NULL, 10));
        } else {
            return false;
        }
    }

    if (opt.instance_arg.empty() || opt.algorithm.empty() || opt.cutoff_label.empty()) {
        return false;
    }
    if (opt.algorithm != "BnB" && opt.algorithm != "Approx" &&
        opt.algorithm != "LS1" && opt.algorithm != "LS2") {
        return false;
    }
    if (opt.cutoff <= 0.0) {
        return false;
    }

    opt.instance_path = resolve_instance_path(opt.instance_arg);
    if (opt.instance_path.empty()) {
        return false;
    }
    opt.instance_name = strip_extension(basename_path(opt.instance_path));
    return true;
}

// Read the edge list into adjacency-by-edge form so every algorithm can update coverage cheaply.
Graph read_graph(const string &path) {
    Graph g;
    ifstream in(path.c_str());
    if (!in) {
        throw runtime_error("Unable to open input file: " + path);
    }

    in >> g.n >> g.m;
    g.edges.reserve(g.m);
    g.adj_edges.assign(g.n + 1, vector<int>());
    g.degree.assign(g.n + 1, 0);

    for (int i = 0; i < g.m; ++i) {
        Edge e;
        in >> e.u >> e.v;
        g.edges.push_back(e);
        g.adj_edges[e.u].push_back(i);
        g.adj_edges[e.v].push_back(i);
        g.degree[e.u]++;
        g.degree[e.v]++;
    }
    return g;
}

static vector<int> cover_vertices(const vector<char> &cover) {
    vector<int> vertices;
    for (size_t v = 1; v < cover.size(); ++v) {
        if (cover[v]) {
            vertices.push_back(static_cast<int>(v));
        }
    }
    return vertices;
}

// Emit the solution file in the exact two-line format expected by the handout/autograder.
void write_solution_file(const string &path, const vector<char> &cover) {
    ofstream out(path.c_str());
    vector<int> vertices = cover_vertices(cover);
    out << vertices.size() << "\n";
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i) {
            out << ' ';
        }
        out << vertices[i];
    }
    out << "\n";
}

// Emit one best-so-far improvement per line for plotting and grading trace requirements.
void write_trace_file(const string &path, const vector<TracePoint> &trace) {
    ofstream out(path.c_str());
    out << fixed << setprecision(6);
    for (size_t i = 0; i < trace.size(); ++i) {
        out << trace[i].time_sec << ' ' << trace[i].value << "\n";
    }
}

// Centralize hard failures so main stays focused on the solver workflow.
void print_error_and_exit(const string &message) {
    cerr << message << "\n";
    exit(1);
}
