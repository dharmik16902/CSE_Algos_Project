#!/usr/bin/env python3

import csv
import math
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]
RESULTS_DIR = ROOT / "results"
OUTPUT_DIR = RESULTS_DIR / "outputs"
FIG_DIR = ROOT / "report" / "figures"
TABLE_DIR = ROOT / "report" / "tables"
RUNS_CSV = RESULTS_DIR / "raw_runs.csv"
SUMMARY_CSV = RESULTS_DIR / "summary.csv"

ALGORITHMS = ["BnB", "Approx", "LS1", "LS2"]
LS_ALGORITHMS = ["LS1", "LS2"]


def load_csv(path):
    """Load CSV data using only the standard library to avoid environment-specific pandas issues."""
    with path.open() as handle:
        return list(csv.DictReader(handle))


def parse_trace_file(filename):
    """Read a trace into sorted improvement events."""
    if not filename:
        return []
    path = OUTPUT_DIR / filename
    if not path.exists():
        return []
    points = []
    with path.open() as handle:
        for line in handle:
            parts = line.strip().split()
            if len(parts) == 2:
                points.append((float(parts[0]), int(parts[1])))
    points.sort()
    return points


def time_to_quality(trace_points, threshold):
    """Return the first time a run reaches the target cover-size threshold, if it ever does."""
    for timestamp, value in trace_points:
        if value <= threshold:
            return timestamp
    return None


def best_rel_err_by_time(trace_points, ref_value, cutoff):
    """Convert a trace into the best relative error visible at each time budget."""
    if not trace_points:
        return [(cutoff, math.inf)]

    best_curve = []
    for timestamp, value in trace_points:
        rel_err = (value - ref_value) / float(ref_value)
        best_curve.append((timestamp, rel_err))
    return best_curve


def build_summary_maps(summary_rows):
    """Index aggregate rows by instance and algorithm for easier table generation."""
    summary = {}
    for row in summary_rows:
        key = (row["family"], row["instance"], row["algorithm"])
        summary[key] = row
    return summary


def tex_escape(text):
    return text.replace("_", "\\_")


def format_triplet(size_value, rel_err, runtime):
    """Compact each algorithm into size / error / runtime to keep the report tables readable."""
    return f"{size_value:.2f} / {rel_err:.2f} / {runtime:.2f}"


def write_summary_table(filename, family, label, summary_map):
    """Generate a longtable containing the per-instance results requested in the handout."""
    instances = sorted({instance for fam, instance, _ in summary_map if fam == family})
    path = TABLE_DIR / filename
    with path.open("w") as handle:
        handle.write("\\begin{longtable}{lrrrrr}\n")
        handle.write(
            "\\caption{%s instances: size / rel.err / time(s).\\label{%s}}\\\\\n"
            % (tex_escape(family.capitalize()), label)
        )
        handle.write("\\toprule\n")
        handle.write("Instance & Ref & BnB & Approx & LS1 & LS2\\\\\n")
        handle.write("\\midrule\n")
        handle.write("\\endfirsthead\n")
        handle.write("\\toprule\n")
        handle.write("Instance & Ref & BnB & Approx & LS1 & LS2\\\\\n")
        handle.write("\\midrule\n")
        handle.write("\\endhead\n")
        for instance in instances:
            ref_value = int(summary_map[(family, instance, "BnB")]["ref_value"])
            cells = []
            for algorithm in ALGORITHMS:
                row = summary_map[(family, instance, algorithm)]
                cells.append(
                    format_triplet(
                        float(row["avg_cover_size"]),
                        float(row["avg_rel_err"]),
                        float(row["avg_runtime_sec"]),
                    )
                )
            handle.write(
                "%s & %d & %s & %s & %s & %s\\\\\n"
                % (tex_escape(instance), ref_value, cells[0], cells[1], cells[2], cells[3])
            )
        handle.write("\\bottomrule\n")
        handle.write("\\end{longtable}\n")


def write_metrics_tex(raw_rows, summary_rows):
    """Export report-ready macros so the LaTeX file can cite the measured results directly."""
    summary_map = build_summary_maps(summary_rows)
    large1_ls1 = float(summary_map[("large", "large1", "LS1")]["avg_cover_size"])
    large1_ls2 = float(summary_map[("large", "large1", "LS2")]["avg_cover_size"])
    large12_ls1 = float(summary_map[("large", "large12", "LS1")]["avg_cover_size"])
    large12_ls2 = float(summary_map[("large", "large12", "LS2")]["avg_cover_size"])

    counts = defaultdict(int)
    totals = defaultdict(int)
    for row in raw_rows:
        if row["family"] not in {"test", "small"}:
            continue
        if row["algorithm"] not in {"BnB", "LS1", "LS2"}:
            continue
        totals[row["algorithm"]] += 1
        if int(row["cover_size"]) == int(row["ref_value"]):
            counts[row["algorithm"]] += 1

    approx_total = 0
    approx_exact = 0
    for row in raw_rows:
        if row["algorithm"] == "Approx" and row["family"] in {"test", "small"}:
            approx_total += 1
            if int(row["cover_size"]) == int(row["ref_value"]):
                approx_exact += 1

    path = TABLE_DIR / "metrics.tex"
    with path.open("w") as handle:
        handle.write("\\newcommand{\\BnBPassRate}{%d/%d}\n" % (counts["BnB"], totals["BnB"]))
        handle.write("\\newcommand{\\LSOnePassRate}{%d/%d}\n" % (counts["LS1"], totals["LS1"]))
        handle.write("\\newcommand{\\LSTwoPassRate}{%d/%d}\n" % (counts["LS2"], totals["LS2"]))
        handle.write("\\newcommand{\\ApproxExactRate}{%d/%d}\n" % (approx_exact, approx_total))
        handle.write("\\newcommand{\\LargeOneLSOneAvg}{%.2f}\n" % large1_ls1)
        handle.write("\\newcommand{\\LargeOneLSTwoAvg}{%.2f}\n" % large1_ls2)
        handle.write("\\newcommand{\\LargeTwelveLSOneAvg}{%.2f}\n" % large12_ls1)
        handle.write("\\newcommand{\\LargeTwelveLSTwoAvg}{%.2f}\n" % large12_ls2)


def plot_qrtd(raw_rows):
    """Generate QRTD plots for the two required focal large instances."""
    q_values = [0.00, 0.01, 0.02, 0.05]
    for instance in ["large1", "large12"]:
        fig, axes = plt.subplots(1, 2, figsize=(11, 4), sharey=True)
        for ax, algorithm in zip(axes, LS_ALGORITHMS):
            runs = [row for row in raw_rows if row["instance"] == instance and row["algorithm"] == algorithm]
            cutoff = float(runs[0]["cutoff"])
            ref_value = int(runs[0]["ref_value"])
            times = [i * cutoff / 100.0 for i in range(101)]

            for q in q_values:
                threshold = ref_value * (1.0 + q)
                solved = []
                for row in runs:
                    trace = parse_trace_file(row["trace_file"])
                    hit_time = time_to_quality(trace, threshold)
                    solved.append(hit_time)

                fractions = []
                for budget in times:
                    count = sum(1 for value in solved if value is not None and value <= budget)
                    fractions.append(count / float(len(solved)))
                ax.plot(times, fractions, label=f"q={int(q * 100)}%")

            ax.set_title(f"{algorithm} on {instance}")
            ax.set_xlabel("Runtime (s)")
            ax.set_ylim(0.0, 1.05)
            ax.grid(alpha=0.3)
        axes[0].set_ylabel("Fraction of runs solved")
        axes[1].legend(loc="lower right")
        fig.tight_layout()
        fig.savefig(FIG_DIR / f"qrtd_{instance}.png", dpi=200, bbox_inches="tight")
        plt.close(fig)


def plot_sqd(raw_rows):
    """Generate SQD plots by fixing time and measuring the fraction of runs below each relative error."""
    time_budgets = [0.5, 1.0, 2.0]
    rel_thresholds = [i / 100.0 for i in range(0, 11)]
    for instance in ["large1", "large12"]:
        fig, axes = plt.subplots(1, 2, figsize=(11, 4), sharey=True)
        for ax, algorithm in zip(axes, LS_ALGORITHMS):
            runs = [row for row in raw_rows if row["instance"] == instance and row["algorithm"] == algorithm]
            ref_value = int(runs[0]["ref_value"])

            per_run_curves = []
            for row in runs:
                trace = parse_trace_file(row["trace_file"])
                per_run_curves.append(best_rel_err_by_time(trace, ref_value, float(row["cutoff"])))

            for budget in time_budgets:
                fractions = []
                for threshold in rel_thresholds:
                    solved = 0
                    for curve in per_run_curves:
                        best = math.inf
                        for timestamp, rel_err in curve:
                            if timestamp <= budget:
                                best = min(best, rel_err)
                        if best <= threshold:
                            solved += 1
                    fractions.append(solved / float(len(per_run_curves)))
                ax.plot([100.0 * t for t in rel_thresholds], fractions, label=f"t={budget:.1f}s")

            ax.set_title(f"{algorithm} on {instance}")
            ax.set_xlabel("Relative error target (%)")
            ax.set_ylim(0.0, 1.05)
            ax.grid(alpha=0.3)
        axes[0].set_ylabel("Fraction of runs solved")
        axes[1].legend(loc="lower right")
        fig.tight_layout()
        fig.savefig(FIG_DIR / f"sqd_{instance}.png", dpi=200, bbox_inches="tight")
        plt.close(fig)


def plot_boxplots(raw_rows):
    """Generate box plots for time-to-target using a 5% quality threshold."""
    q_target = 0.05
    for instance in ["large1", "large12"]:
        data = []
        labels = []
        ref_value = None
        cutoff = None

        for algorithm in LS_ALGORITHMS:
            runs = [row for row in raw_rows if row["instance"] == instance and row["algorithm"] == algorithm]
            ref_value = int(runs[0]["ref_value"])
            cutoff = float(runs[0]["cutoff"])
            threshold = ref_value * (1.0 + q_target)
            values = []
            for row in runs:
                trace = parse_trace_file(row["trace_file"])
                hit_time = time_to_quality(trace, threshold)
                values.append(cutoff if hit_time is None else hit_time)
            data.append(values)
            labels.append(algorithm)

        fig, ax = plt.subplots(figsize=(6, 4))
        ax.boxplot(data, tick_labels=labels)
        ax.set_title(f"Time to reach 5% relative error on {instance}")
        ax.set_ylabel("Runtime (s)")
        ax.grid(axis="y", alpha=0.3)
        fig.tight_layout()
        fig.savefig(FIG_DIR / f"boxplot_{instance}.png", dpi=200, bbox_inches="tight")
        plt.close(fig)


def main():
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    TABLE_DIR.mkdir(parents=True, exist_ok=True)

    raw_rows = load_csv(RUNS_CSV)
    summary_rows = load_csv(SUMMARY_CSV)
    summary_map = build_summary_maps(summary_rows)

    write_summary_table("small_results.tex", "small", "tab:small", summary_map)
    write_summary_table("large_results.tex", "large", "tab:large", summary_map)
    write_metrics_tex(raw_rows, summary_rows)
    plot_qrtd(raw_rows)
    plot_sqd(raw_rows)
    plot_boxplots(raw_rows)


if __name__ == "__main__":
    main()
