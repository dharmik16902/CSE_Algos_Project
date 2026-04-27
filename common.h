#ifndef COMMON_H
#define COMMON_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Edge {
    int u;
    int v;
};

struct Graph {
    int n = 0;
    int m = 0;
    std::vector<Edge> edges;
    std::vector<std::vector<int> > adj_edges;
    std::vector<int> degree;
};

struct TracePoint {
    double time_sec = 0.0;
    int value = 0;
};

struct SolveResult {
    std::vector<char> cover;
    std::vector<TracePoint> trace;
    bool exact = false;
};

struct Options {
    std::string instance_arg;
    std::string instance_path;
    std::string instance_name;
    std::string algorithm;
    std::string cutoff_label;
    double cutoff = 0.0;
    unsigned int seed = 1U;
};

#endif
