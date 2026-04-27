#!/bin/sh

set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SUBMISSION_DIR="$ROOT/submission"
CODE_DIR="$SUBMISSION_DIR/CourseProject-code"
ZIP_PATH="$SUBMISSION_DIR/CourseProject-code.zip"

rm -rf "$SUBMISSION_DIR"
mkdir -p "$CODE_DIR"
mkdir -p "$CODE_DIR/scripts" "$CODE_DIR/results/outputs" "$CODE_DIR/report"

for path in \
  README.md \
  algorithms.h \
  approx.cpp \
  bnb.cpp \
  common.h \
  graph_ops.cpp \
  graph_ops.h \
  io.cpp \
  io.h \
  local_search.cpp \
  mvc.cpp
do
  cp "$ROOT/$path" "$CODE_DIR/"
done

for path in \
  scripts/build_mvc.sh \
  scripts/run_mvc.sh
do
  cp "$ROOT/$path" "$CODE_DIR/scripts/"
done

cp "$ROOT/results/raw_runs.csv" "$CODE_DIR/results/"
cp "$ROOT/results/summary.csv" "$CODE_DIR/results/"
cp -R "$ROOT/results/outputs/." "$CODE_DIR/results/outputs/"
cp "$ROOT/report/report.pdf" "$SUBMISSION_DIR/CourseProject-report.pdf"

cat > "$SUBMISSION_DIR/UPLOAD_NOTES.txt" <<'EOF'
CourseProject-report.pdf
  Upload to Gradescope as the report submission.

CourseProject-code.zip
  Upload to Canvas as the code/output submission.
  This archive intentionally excludes the input data files, per the handout.

Remaining manual items outside this folder:
  1. Gradescope autograder submission for the program.
  2. Team evaluation submission, if applicable.
EOF

(
  cd "$SUBMISSION_DIR"
  zip -rq "$(basename "$ZIP_PATH")" "$(basename "$CODE_DIR")"
)
