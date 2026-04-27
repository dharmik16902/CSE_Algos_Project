#!/bin/sh

set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
g++ -O2 *.cpp -o mvc
