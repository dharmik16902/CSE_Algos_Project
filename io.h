#ifndef IO_H
#define IO_H

#include "common.h"

std::string usage();
bool parse_args(int argc, char **argv, Options &opt);
Graph read_graph(const std::string &path);
void write_solution_file(const std::string &path, const std::vector<char> &cover);
void write_trace_file(const std::string &path, const std::vector<TracePoint> &trace);
void print_error_and_exit(const std::string &message);

#endif
