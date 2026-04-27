#ifndef GRAPH_OPS_H
#define GRAPH_OPS_H

#include "common.h"

int cover_size(const std::vector<char> &cover);
bool is_valid_cover(const Graph &g, const std::vector<char> &cover);
void prune_cover(const Graph &g, std::vector<char> &cover);
std::vector<char> approx_cover(const Graph &g);

#endif
