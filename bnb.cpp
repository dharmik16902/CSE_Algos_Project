#include "algorithms.h"
#include "graph_ops.h"

using namespace std;

class BranchAndBoundSolver {
public:
    BranchAndBoundSolver(const Graph &graph, double cutoff_seconds)
        : g(graph), cutoff(cutoff_seconds) {}

    // Run branch-and-bound with reduction and lower-bound pruning until solved or cutoff.
    SolveResult solve() {
        start = chrono::steady_clock::now();
        timed_out = false;
        trace.clear();

        State root;
        root.alive.assign(g.n + 1, 1);
        root.selected.assign(g.n + 1, 0);
        root.degree = g.degree;
        root.edges_left = g.m;
        root.selected_count = 0;
        root.match_used.assign(g.n + 1, 0);

        best_cover = approx_cover(g);
        best_size = cover_size(best_cover);
        record_trace(best_size);

        dfs(root);

        SolveResult result;
        result.cover = best_cover;
        result.trace = trace;
        result.exact = !timed_out;
        return result;
    }

private:
    // "alive" vertices are the undecided residual graph; "selected" is the partial cover built so far.
    struct State {
        vector<char> alive;
        vector<char> selected;
        vector<int> degree;
        vector<char> match_used;
        int edges_left = 0;
        int selected_count = 0;
    };

    const Graph &g;
    double cutoff;
    chrono::steady_clock::time_point start;
    bool timed_out = false;
    vector<char> best_cover;
    int best_size = 0;
    vector<TracePoint> trace;

    // Report elapsed wall-clock time so every recursive branch respects the cutoff.
    double elapsed() const {
        const chrono::duration<double> diff = chrono::steady_clock::now() - start;
        return diff.count();
    }

    bool out_of_time() {
        if (elapsed() >= cutoff) {
            timed_out = true;
            return true;
        }
        return false;
    }

    // Cache only strict improvements because the trace file records best-so-far solution updates.
    void record_trace(int value) {
        if (trace.empty() || value < trace.back().value) {
            TracePoint p;
            p.time_sec = elapsed();
            p.value = value;
            trace.push_back(p);
        }
    }

    void update_best(const vector<char> &cover, int size) {
        if (size < best_size) {
            best_size = size;
            best_cover = cover;
            record_trace(size);
        }
    }

    int other_endpoint(int edge_id, int v) const {
        const Edge &e = g.edges[edge_id];
        return e.u == v ? e.v : e.u;
    }

    // Remove v from the residual graph and update residual degrees and uncovered-edge count.
    void remove_alive_vertex(State &st, int v) const {
        if (!st.alive[v]) {
            return;
        }
        st.alive[v] = 0;
        for (size_t i = 0; i < g.adj_edges[v].size(); ++i) {
            const int edge_id = g.adj_edges[v][i];
            const int u = other_endpoint(edge_id, v);
            if (st.alive[u]) {
                st.degree[u]--;
                st.edges_left--;
            }
        }
        st.degree[v] = 0;
    }

    // Selecting a vertex means adding it to the cover and deleting all incident residual edges.
    void include_vertex(State &st, int v) const {
        if (!st.alive[v]) {
            return;
        }
        st.selected[v] = 1;
        st.selected_count++;
        remove_alive_vertex(st, v);
    }

    // Degree-1 reduction needs the unique residual neighbor of v, if one still exists.
    int find_one_alive_neighbor(const State &st, int v) const {
        for (size_t i = 0; i < g.adj_edges[v].size(); ++i) {
            const int edge_id = g.adj_edges[v][i];
            const int u = other_endpoint(edge_id, v);
            if (st.alive[u]) {
                return u;
            }
        }
        return -1;
    }

    // Exhaust the easy reductions before branching to keep the search tree small.
    void reduce_state(State &st) {
        bool changed = true;
        while (changed && !out_of_time()) {
            changed = false;
            for (int v = 1; v <= g.n; ++v) {
                if (!st.alive[v]) {
                    continue;
                }
                if (st.degree[v] == 0) {
                    remove_alive_vertex(st, v);
                    changed = true;
                } else if (st.degree[v] == 1) {
                    const int u = find_one_alive_neighbor(st, v);
                    if (u != -1) {
                        include_vertex(st, u);
                        changed = true;
                    }
                }
            }
        }
    }

    int matching_lower_bound(State &st) const {
        fill(st.match_used.begin(), st.match_used.end(), 0);
        int matching = 0;
        for (int i = 0; i < g.m; ++i) {
            const Edge &e = g.edges[i];

            // A maximal matching in the residual graph is a valid lower bound on cover size.
            if (st.alive[e.u] && st.alive[e.v] &&
                !st.match_used[e.u] && !st.match_used[e.v]) {
                st.match_used[e.u] = 1;
                st.match_used[e.v] = 1;
                matching++;
            }
        }
        return matching;
    }

    // Combine matching and degree bounds so pruning remains cheap but useful.
    int lower_bound(State &st) const {
        int max_degree = 0;
        for (int v = 1; v <= g.n; ++v) {
            if (st.alive[v] && st.degree[v] > max_degree) {
                max_degree = st.degree[v];
            }
        }

        int lb = st.selected_count + matching_lower_bound(st);
        if (st.edges_left > 0 && max_degree > 0) {
            lb = max(lb, st.selected_count + (st.edges_left + max_degree - 1) / max_degree);
        }
        return lb;
    }

    // Branch on a high-degree residual vertex to expose many edges early in the search tree.
    int choose_branch_vertex(const State &st) const {
        int best_v = -1;
        int best_deg = -1;
        for (int v = 1; v <= g.n; ++v) {
            if (st.alive[v] && st.degree[v] > best_deg) {
                best_deg = st.degree[v];
                best_v = v;
            }
        }
        return best_v;
    }

    // Branch on either selecting v or forcing all of its remaining neighbors into the cover.
    void dfs(State st) {
        if (out_of_time()) {
            return;
        }

        reduce_state(st);
        if (out_of_time()) {
            return;
        }

        if (st.edges_left == 0) {
            update_best(st.selected, st.selected_count);
            return;
        }
        if (st.selected_count >= best_size) {
            return;
        }
        if (lower_bound(st) >= best_size) {
            return;
        }

        const int v = choose_branch_vertex(st);
        if (v == -1) {
            update_best(st.selected, st.selected_count);
            return;
        }

        // Branch 1: put v in the cover.
        State include_branch = st;
        include_vertex(include_branch, v);
        if (include_branch.selected_count < best_size) {
            dfs(include_branch);
        }

        if (out_of_time()) {
            return;
        }

        // Branch 2: exclude v, which forces every residual neighbor of v into the cover.
        State exclude_branch = st;
        vector<int> neighbors;
        neighbors.reserve(exclude_branch.degree[v]);
        for (size_t i = 0; i < g.adj_edges[v].size(); ++i) {
            const int edge_id = g.adj_edges[v][i];
            const int u = other_endpoint(edge_id, v);
            if (exclude_branch.alive[u]) {
                neighbors.push_back(u);
            }
        }
        for (size_t i = 0; i < neighbors.size(); ++i) {
            include_vertex(exclude_branch, neighbors[i]);
        }
        remove_alive_vertex(exclude_branch, v);
        if (exclude_branch.selected_count < best_size) {
            dfs(exclude_branch);
        }
    }
};

SolveResult solve_bnb(const Graph &g, double cutoff) {
    BranchAndBoundSolver solver(g, cutoff);
    return solver.solve();
}
