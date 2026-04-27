#!/bin/sh

set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEXINPUTS_PREFIX="/Users/dharmikpatel/Library/TinyTeX/texmf-dist/tex//:"

cd "$ROOT"
python3 scripts/generate_report_assets.py

cd "$ROOT/report"
TEXINPUTS="$TEXINPUTS_PREFIX" pdflatex -interaction=nonstopmode -halt-on-error report.tex
TEXINPUTS="$TEXINPUTS_PREFIX" pdflatex -interaction=nonstopmode -halt-on-error report.tex
