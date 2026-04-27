#!/bin/sh

set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [ ! -x "$ROOT/mvc" ]; then
  g++ -O2 *.cpp -o mvc
fi

exec "$ROOT/mvc" "$@"
