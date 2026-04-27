#include "graph_ops.h"

using namespace std;

// Count how many vertices are currently selected in a 1-indexed cover bitset.
int cover_size(const vector<char> &cover) {
    int total = 0;
    for (size_t v = 1; v < cover.size(); ++v) {
        total += static_cast<int>(cover[v]);
    }
    return total;
}

// Verify that every edge has at least one endpoint inside the reported cover.
bool is_valid_cover(const Graph &g, const vector<char> &cover) {
    for (int i = 0; i < g.m; ++i) {
        const Edge &e = g.edges[i];
        if (!cover[e.u] && !cover[e.v]) {
            return false;
        }
    }
    return true;
}

// Remove any selected vertex whose incident edges are already covered by its neighbors.
void prune_cover(const Graph &g, vector<char> &cover) {
    vector<int> order;
    order.reserve(g.n);
    for (int v = 1; v <= g.n; ++v) {
        if (cover[v]) {
            order.push_back(v);
        }
    }
    sort(order.begin(), order.end(), [&g](int a, int b) {
        if (g.degree[a] != g.degree[b]) {
            return g.degree[a] < g.degree[b];
        }
        return a < b;
    });

    for (size_t i = 0; i < order.size(); ++i) {
        const int v = order[i];
        if (!cover[v]) {
            continue;
        }

        // Vertex v is redundant only if every incident edge is still covered after removing it.
        bool removable = true;
        for (size_t j = 0; j < g.adj_edges[v].size(); ++j) {
            const Edge &e = g.edges[g.adj_edges[v][j]];
            const int u = (e.u == v ? e.v : e.u);
            if (!cover[u]) {
                removable = false;
                break;
            }
        }
        if (removable) {
            cover[v] = 0;
        }
    }
}

// Build the standard 2-approximation from a maximal matching, then prune redundant vertices.
vector<char> approx_cover(const Graph &g) {
    vector<char> in_cover(g.n + 1, 0);
    vector<char> blocked(g.n + 1, 0);

    for (int i = 0; i < g.m; ++i) {
        const Edge &e = g.edges[i];

        // Once an endpoint is blocked, this edge is already represented in the maximal matching.
        if (!blocked[e.u] && !blocked[e.v]) {
            blocked[e.u] = 1;
            blocked[e.v] = 1;
            in_cover[e.u] = 1;
            in_cover[e.v] = 1;
        }
    }

    prune_cover(g, in_cover);
    return in_cover;
}
