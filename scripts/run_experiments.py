#!/usr/bin/env python3

import csv
import os
import shutil
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESULTS_DIR = ROOT / "results"
OUTPUT_DIR = RESULTS_DIR / "outputs"
RUNS_CSV = RESULTS_DIR / "raw_runs.csv"
SUMMARY_CSV = RESULTS_DIR / "summary.csv"

ALGORITHMS = {
    "BnB": {"cutoff": 2, "seeds": [1]},
    "Approx": {"cutoff": 2, "seeds": [1]},
    "LS1": {"cutoff": 2, "seeds": list(range(1, 21))},
    "LS2": {"cutoff": 2, "seeds": list(range(1, 21))},
}

DATASETS = [
    ("test", ROOT / "data" / "test"),
    ("small", ROOT / "data" / "small"),
    ("large", ROOT / "data" / "large"),
]


def compile_binary():
    """Build the project exactly the way the autograder expects."""
    subprocess.run(
        "g++ -O2 *.cpp -o mvc",
        cwd=ROOT,
        shell=True,
        check=True,
    )


def discover_instances():
    """Return all released instances grouped by dataset family."""
    instances = []
    for family, folder in DATASETS:
        for path in sorted(folder.glob("*.in")):
            name = path.stem
            ref_path = folder / f"{name}.out"
            ref_value = int(ref_path.read_text().splitlines()[0].strip())
            instances.append(
                {
                    "family": family,
                    "path": path,
                    "name": name,
                    "ref_value": ref_value,
                }
            )
    return instances


def solution_name(instance_name, algorithm, cutoff, seed):
    """Mirror the naming convention from the project handout."""
    if algorithm in {"LS1", "LS2"}:
        return f"{instance_name}_{algorithm}_{cutoff}_{seed}.sol"
    return f"{instance_name}_{algorithm}_{cutoff}.sol"


def trace_name(instance_name, algorithm, cutoff, seed):
    """Trace files exist only for BnB and the local-search algorithms."""
    if algorithm not in {"BnB", "LS1", "LS2"}:
        return None
    if algorithm in {"LS1", "LS2"}:
        return f"{instance_name}_{algorithm}_{cutoff}_{seed}.trace"
    return f"{instance_name}_{algorithm}_{cutoff}.trace"


def run_one(binary_path, run_root, instance, algorithm, cutoff, seed):
    """Execute a single solver run and collect the artifacts needed for analysis."""
    run_dir = run_root / f"{instance['name']}_{algorithm}_{seed}"
    shutil.rmtree(run_dir, ignore_errors=True)
    run_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        str(binary_path),
        "-inst",
        str(instance["path"]),
        "-alg",
        algorithm,
        "-time",
        str(cutoff),
        "-seed",
        str(seed),
    ]

    start = time.perf_counter()
    subprocess.run(cmd, cwd=run_dir, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    runtime = time.perf_counter() - start

    sol_file = run_dir / solution_name(instance["name"], algorithm, cutoff, seed)
    trace_file = run_dir / trace_name(instance["name"], algorithm, cutoff, seed) if trace_name(instance["name"], algorithm, cutoff, seed) else None

    cover_size = int(sol_file.read_text().splitlines()[0].strip())
    rel_err = (cover_size - instance["ref_value"]) / float(instance["ref_value"])

    stored_sol = OUTPUT_DIR / sol_file.name
    shutil.copy2(sol_file, stored_sol)

    stored_trace = ""
    if trace_file and trace_file.exists():
        stored_trace_path = OUTPUT_DIR / trace_file.name
        shutil.copy2(trace_file, stored_trace_path)
        stored_trace = stored_trace_path.name

    return {
        "family": instance["family"],
        "instance": instance["name"],
        "algorithm": algorithm,
        "seed": seed,
        "cutoff": cutoff,
        "runtime_sec": runtime,
        "cover_size": cover_size,
        "ref_value": instance["ref_value"],
        "rel_err": rel_err,
        "solution_file": stored_sol.name,
        "trace_file": stored_trace,
    }


def aggregate_rows(rows):
    """Collapse raw runs into one row per instance and algorithm for the report tables."""
    grouped = {}
    for row in rows:
        key = (row["family"], row["instance"], row["algorithm"])
        grouped.setdefault(key, []).append(row)

    summary = []
    for key, group in sorted(grouped.items()):
        family, instance, algorithm = key
        runtime_avg = sum(row["runtime_sec"] for row in group) / len(group)
        size_avg = sum(row["cover_size"] for row in group) / len(group)
        rel_err_avg = sum(row["rel_err"] for row in group) / len(group)
        ref_value = group[0]["ref_value"]
        summary.append(
            {
                "family": family,
                "instance": instance,
                "algorithm": algorithm,
                "runs": len(group),
                "ref_value": ref_value,
                "avg_runtime_sec": runtime_avg,
                "avg_cover_size": size_avg,
                "avg_rel_err": rel_err_avg,
                "best_cover_size": min(row["cover_size"] for row in group),
                "worst_cover_size": max(row["cover_size"] for row in group),
            }
        )
    return summary


def write_csv(path, rows, fieldnames):
    """Write a stable CSV so later scripts can regenerate plots and tables deterministically."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main():
    shutil.rmtree(RESULTS_DIR, ignore_errors=True)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    compile_binary()
    instances = discover_instances()
    binary_path = ROOT / "mvc"
    run_root = RESULTS_DIR / "tmp_run"
    run_root.mkdir(parents=True, exist_ok=True)

    jobs = []
    for instance in instances:
        for algorithm, config in ALGORITHMS.items():
            for seed in config["seeds"]:
                jobs.append((instance, algorithm, config["cutoff"], seed))

    rows = []
    max_workers = min(6, os.cpu_count() or 1)
    with ThreadPoolExecutor(max_workers=max_workers) as pool:
        futures = [
            pool.submit(run_one, binary_path, run_root, instance, algorithm, cutoff, seed)
            for instance, algorithm, cutoff, seed in jobs
        ]
        for future in as_completed(futures):
            rows.append(future.result())

    rows.sort(key=lambda row: (row["family"], row["instance"], row["algorithm"], row["seed"]))
    summary = aggregate_rows(rows)

    write_csv(
        RUNS_CSV,
        rows,
        [
            "family",
            "instance",
            "algorithm",
            "seed",
            "cutoff",
            "runtime_sec",
            "cover_size",
            "ref_value",
            "rel_err",
            "solution_file",
            "trace_file",
        ],
    )
    write_csv(
        SUMMARY_CSV,
        summary,
        [
            "family",
            "instance",
            "algorithm",
            "runs",
            "ref_value",
            "avg_runtime_sec",
            "avg_cover_size",
            "avg_rel_err",
            "best_cover_size",
            "worst_cover_size",
        ],
    )

    shutil.rmtree(run_root, ignore_errors=True)


if __name__ == "__main__":
    main()
