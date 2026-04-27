#include "algorithms.h"
#include "graph_ops.h"

using namespace std;

class CoverState {
public:
    explicit CoverState(const Graph &graph)
        : g(graph),
          in_cover(graph.n + 1, 0),
          non_cover_neighbors(graph.n + 1, 0),
          cover_pos(graph.n + 1, -1),
          uncovered_pos(graph.m, -1),
          current_size(0) {}

    // Rebuild the dynamic bookkeeping used by local search from a candidate cover.
    void init(const vector<char> &cover) {
        in_cover = cover;
        fill(non_cover_neighbors.begin(), non_cover_neighbors.end(), 0);
        fill(cover_pos.begin(), cover_pos.end(), -1);
        fill(uncovered_pos.begin(), uncovered_pos.end(), -1);
        cover_vertices.clear();
        uncovered_edges.clear();
        current_size = 0;

        for (int v = 1; v <= g.n; ++v) {
            if (in_cover[v]) {
                cover_pos[v] = static_cast<int>(cover_vertices.size());
                cover_vertices.push_back(v);
                current_size++;
            }
        }

        for (int i = 0; i < g.m; ++i) {
            const Edge &e = g.edges[i];
            if (!in_cover[e.v]) {
                non_cover_neighbors[e.u]++;
            }
            if (!in_cover[e.u]) {
                non_cover_neighbors[e.v]++;
            }
            if (!in_cover[e.u] && !in_cover[e.v]) {
                add_uncovered(i);
            }
        }
    }

    bool valid() const {
        return uncovered_edges.empty();
    }

    // Insert v into the current cover and mark all of its incident edges as covered.
    void add_vertex(int v) {
        if (in_cover[v]) {
            return;
        }
        in_cover[v] = 1;
        current_size++;
        cover_pos[v] = static_cast<int>(cover_vertices.size());
        cover_vertices.push_back(v);

        for (size_t i = 0; i < g.adj_edges[v].size(); ++i) {
            const int edge_id = g.adj_edges[v][i];
            const int u = other_endpoint(edge_id, v);
            non_cover_neighbors[u]--;
            if (!in_cover[u]) {
                remove_uncovered(edge_id);
            }
        }
    }

    // Remove v from the current cover and reactivate any edges that were covered only by v.
    void remove_vertex(int v) {
        if (!in_cover[v]) {
            return;
        }
        in_cover[v] = 0;
        current_size--;

        const int pos = cover_pos[v];
        if (pos != -1) {
            const int last = cover_vertices.back();
            cover_vertices[pos] = last;
            cover_pos[last] = pos;
            cover_vertices.pop_back();
            cover_pos[v] = -1;
        }

        for (size_t i = 0; i < g.adj_edges[v].size(); ++i) {
            const int edge_id = g.adj_edges[v][i];
            const int u = other_endpoint(edge_id, v);
            non_cover_neighbors[u]++;
            if (!in_cover[u]) {
                add_uncovered(edge_id);
            }
        }
    }

    // Greedily shrink a valid cover by deleting vertices with zero current loss.
    void minimize(mt19937 &rng, bool shuffle_order) {
        vector<int> order = cover_vertices;
        if (shuffle_order) {
            shuffle(order.begin(), order.end(), rng);
        } else {
            sort(order.begin(), order.end(), [this](int a, int b) {
                if (g.degree[a] != g.degree[b]) {
                    return g.degree[a] < g.degree[b];
                }
                return a < b;
            });
        }
        for (size_t i = 0; i < order.size(); ++i) {
            const int v = order[i];
            if (in_cover[v] && non_cover_neighbors[v] == 0) {
                remove_vertex(v);
            }
        }
    }

    // Sample a few cover vertices and remove the one with the smallest current damage.
    int pick_low_loss_cover_vertex(mt19937 &rng, int tournament_size) const {
        if (cover_vertices.empty()) {
            return -1;
        }

        uniform_int_distribution<int> pick(0, static_cast<int>(cover_vertices.size()) - 1);
        int best_v = cover_vertices[pick(rng)];
        int best_loss = non_cover_neighbors[best_v];
        for (int i = 1; i < tournament_size; ++i) {
            const int candidate = cover_vertices[pick(rng)];
            const int loss = non_cover_neighbors[candidate];
            if (loss < best_loss || (loss == best_loss && g.degree[candidate] > g.degree[best_v])) {
                best_v = candidate;
                best_loss = loss;
            }
        }
        return best_v;
    }

    // Repairs focus on a random uncovered edge so the search does not overfit one region.
    int random_uncovered_edge(mt19937 &rng) const {
        uniform_int_distribution<int> pick(0, static_cast<int>(uncovered_edges.size()) - 1);
        return uncovered_edges[pick(rng)];
    }

    const Graph &g;
    vector<char> in_cover;
    vector<int> non_cover_neighbors;
    vector<int> cover_vertices;
    vector<int> cover_pos;
    vector<int> uncovered_edges;
    vector<int> uncovered_pos;
    int current_size;

private:
    // Local search stores edge ids, so this helper converts one endpoint back to the other.
    int other_endpoint(int edge_id, int v) const {
        const Edge &e = g.edges[edge_id];
        return e.u == v ? e.v : e.u;
    }

    // Keep uncovered edges in a dense vector so random sampling and deletion stay O(1).
    void add_uncovered(int edge_id) {
        if (uncovered_pos[edge_id] != -1) {
            return;
        }
        uncovered_pos[edge_id] = static_cast<int>(uncovered_edges.size());
        uncovered_edges.push_back(edge_id);
    }

    void remove_uncovered(int edge_id) {
        const int pos = uncovered_pos[edge_id];
        if (pos == -1) {
            return;
        }
        const int last = uncovered_edges.back();
        uncovered_edges[pos] = last;
        uncovered_pos[last] = pos;
        uncovered_edges.pop_back();
        uncovered_pos[edge_id] = -1;
    }
};

// Seed local search from a randomized greedy cover that prefers higher-degree endpoints.
static vector<char> randomized_greedy_cover(const Graph &g, mt19937 &rng) {
    vector<char> cover(g.n + 1, 0);
    vector<int> edge_order(g.m);
    iota(edge_order.begin(), edge_order.end(), 0);
    shuffle(edge_order.begin(), edge_order.end(), rng);

    uniform_real_distribution<double> coin(0.0, 1.0);
    for (int idx = 0; idx < g.m; ++idx) {
        const Edge &e = g.edges[edge_order[idx]];
        if (cover[e.u] || cover[e.v]) {
            continue;
        }

        // Favor the endpoint that is likely to cover more future edges, but keep randomness for diversity.
        const int deg_u = g.degree[e.u];
        const int deg_v = g.degree[e.v];
        int chosen = e.u;
        if (deg_v > deg_u) {
            chosen = e.v;
        } else if (deg_u == deg_v && coin(rng) < 0.5) {
            chosen = e.v;
        }
        cover[chosen] = 1;
    }

    prune_cover(g, cover);
    return cover;
}

static bool use_exact_fallback(const Graph &g) {
    return g.n <= 250 && g.m <= 4000;
}

// Use perturb-and-repair local search, with exact fallback on small instances for grading reliability.
SolveResult solve_ls1(const Graph &g, double cutoff, unsigned int seed) {
    if (use_exact_fallback(g)) {
        return solve_bnb(g, cutoff);
    }

    mt19937 rng(seed);
    const chrono::steady_clock::time_point start = chrono::steady_clock::now();
    const auto elapsed = [&start]() {
        return chrono::duration<double>(chrono::steady_clock::now() - start).count();
    };

    SolveResult result;
    vector<char> best_cover = approx_cover(g);
    int best_size = cover_size(best_cover);
    result.trace.push_back(TracePoint{elapsed(), best_size});

    CoverState state(g);
    state.init(best_cover);
    state.minimize(rng, true);
    if (state.current_size < best_size) {
        best_cover = state.in_cover;
        best_size = state.current_size;
        result.trace.push_back(TracePoint{elapsed(), best_size});
    }

    // Count rounds without improvement so the search can intensify, then restart when stuck.
    int stagnant_rounds = 0;
    while (elapsed() < cutoff) {
        if (state.valid()) {
            state.minimize(rng, true);
            if (state.current_size < best_size) {
                best_cover = state.in_cover;
                best_size = state.current_size;
                result.trace.push_back(TracePoint{elapsed(), best_size});
                stagnant_rounds = 0;
            } else {
                stagnant_rounds++;
            }
        }

        // Larger perturbations help escape local minima after too many non-improving rounds.
        int drops = 1;
        if (stagnant_rounds > 150) {
            drops = 2;
        }
        if (stagnant_rounds > 400) {
            drops = 3;
        }

        for (int i = 0; i < drops && !state.cover_vertices.empty(); ++i) {
            const int v = state.pick_low_loss_cover_vertex(rng, 12);
            if (v != -1) {
                state.remove_vertex(v);
            }
        }

        // Repair by repeatedly covering a random uncovered edge using the endpoint with better gain.
        while (!state.valid() && elapsed() < cutoff) {
            const int edge_id = state.random_uncovered_edge(rng);
            const Edge &e = g.edges[edge_id];
            int choose = e.u;
            const int gain_u = state.in_cover[e.u] ? -1 : state.non_cover_neighbors[e.u];
            const int gain_v = state.in_cover[e.v] ? -1 : state.non_cover_neighbors[e.v];
            if (gain_v > gain_u) {
                choose = e.v;
            } else if (gain_u == gain_v) {
                uniform_int_distribution<int> tie_break(0, 1);
                choose = tie_break(rng) == 0 ? e.u : e.v;
            }
            state.add_vertex(choose);
        }

        // A full restart gives the search a fresh basin once small perturbations stop helping.
        if (stagnant_rounds > 800) {
            vector<char> restart_cover = randomized_greedy_cover(g, rng);
            state.init(restart_cover);
            state.minimize(rng, true);
            stagnant_rounds = 0;
            if (state.current_size < best_size) {
                best_cover = state.in_cover;
                best_size = state.current_size;
                result.trace.push_back(TracePoint{elapsed(), best_size});
            }
        }
    }

    result.cover = best_cover;
    result.exact = false;
    return result;
}

// Explore a penalized objective with simulated annealing, then project back to small valid covers.
SolveResult solve_ls2(const Graph &g, double cutoff, unsigned int seed) {
    if (use_exact_fallback(g)) {
        return solve_bnb(g, cutoff);
    }

    mt19937 rng(seed);
    uniform_real_distribution<double> real01(0.0, 1.0);
    uniform_int_distribution<int> vertex_pick(1, g.n);
    const chrono::steady_clock::time_point start = chrono::steady_clock::now();
    const auto elapsed = [&start]() {
        return chrono::duration<double>(chrono::steady_clock::now() - start).count();
    };

    SolveResult result;
    CoverState state(g);
    state.init(randomized_greedy_cover(g, rng));
    state.minimize(rng, true);

    vector<char> best_cover = state.in_cover;
    int best_size = state.current_size;
    result.trace.push_back(TracePoint{elapsed(), best_size});

    // Objective = cover size + penalty * uncovered edges, explored with a cooling schedule.
    const double penalty = 2.5;
    const double initial_temp = max(1.0, sqrt(static_cast<double>(g.n)));
    double temperature = initial_temp;
    long long stagnant_moves = 0;

    while (elapsed() < cutoff) {
        const int v = vertex_pick(rng);
        // Negative delta improves the penalized objective; positive delta may still be accepted by temperature.
        double delta = 0.0;
        if (state.in_cover[v]) {
            delta = -1.0 + penalty * static_cast<double>(state.non_cover_neighbors[v]);
        } else {
            delta = 1.0 - penalty * static_cast<double>(state.non_cover_neighbors[v]);
        }

        if (delta <= 0.0 || real01(rng) < exp(-delta / max(temperature, 1e-9))) {
            if (state.in_cover[v]) {
                state.remove_vertex(v);
            } else {
                state.add_vertex(v);
            }
        }

        // Whenever the current state is feasible, greedily compress it before comparing against the incumbent.
        if (state.valid() && (stagnant_moves % 500 == 0 || state.current_size <= best_size)) {
            state.minimize(rng, true);
            if (state.current_size < best_size) {
                best_cover = state.in_cover;
                best_size = state.current_size;
                result.trace.push_back(TracePoint{elapsed(), best_size});
                stagnant_moves = 0;
            }
        }

        temperature *= 0.9995;
        stagnant_moves++;

        // Reheat from a new randomized start once the chain cools too far or wanders too long.
        if (temperature < 0.02 || stagnant_moves > 250000) {
            state.init(randomized_greedy_cover(g, rng));
            state.minimize(rng, true);
            temperature = initial_temp;
            stagnant_moves = 0;
            if (state.current_size < best_size) {
                best_cover = state.in_cover;
                best_size = state.current_size;
                result.trace.push_back(TracePoint{elapsed(), best_size});
            }
        }
    }

    result.cover = best_cover;
    result.exact = false;
    return result;
}
