#!/usr/bin/env python3
import argparse
import csv
import re
from pathlib import Path
from collections import Counter, defaultdict

import numpy as np


MODEL_NAMES = ["W3", "W4"]
LOAD_NUMS = ["40", "80"]
TRACE_ORDER = [f"{model}_load{load}" for model in MODEL_NAMES for load in LOAD_NUMS]

DELAY_LABELS = ["500ns", "1us", "2us"]

# Must match the current 20-parameter-sweeping bash sweep:
#   QMIN_MIN=4, QMAX_MAX=36, Q_STEP=2
# Bash condition is qmin < QMAX_MAX, so qmin = 4, 6, ..., 34.
# qmax runs from qmin + 2 to 36.
QMIN_MIN = 4
QMAX_MAX = 36
Q_STEP = 2

FCT_NAME_RE = re.compile(
    r"^(?P<trace>.+)_qmin(?P<qmin>\d+)_qmax(?P<qmax>\d+)\.txt$"
)

METRICS = [
    ("mean", "Mean FCT Slowdown"),
    ("p95", "P95 FCT Slowdown"),
    ("p99", "P99 FCT Slowdown"),
]


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Analyze FCT slowdown for the current 20-parameter-sweeping sweep and "
            "write one matrix CSV per delay: 500ns, 1us, and 2us. "
            "Slowdown = max(1.0, column[6] / column[7])."
        )
    )
    parser.add_argument(
        "root",
        nargs="?",
        default=None,
        help=(
            "Path to the 20-parameter-sweeping/infiniflow output directory, "
            "or directly to its fct directory. If omitted, the script tries "
            "<repo-root>/ns-3/simulator/result/data/20-parameter-sweeping/infiniflow."
        ),
    )
    parser.add_argument(
        "--repo-root",
        default=None,
        help=(
            "Repository root used to build the default output path: "
            "<repo-root>/ns-3/simulator/result/data/20-parameter-sweeping/infiniflow."
        ),
    )
    parser.add_argument(
        "--out-dir",
        default=None,
        help=(
            "Directory for CSV files. Default: <infiniflow-output-root>/csv. "
            "If root points directly to fct, default is <fct-parent>/csv."
        ),
    )
    parser.add_argument(
        "--prefix",
        default="fct_slowdown",
        help="CSV filename prefix. Default: fct_slowdown.",
    )
    parser.add_argument(
        "--delays",
        nargs="+",
        choices=DELAY_LABELS,
        default=DELAY_LABELS,
        help="Delay labels to analyze. Default: 500ns 1us 2us.",
    )
    parser.add_argument(
        "--max-warning-details",
        type=int,
        default=20,
        help="Maximum number of individual missing/empty files to print. Default: 20.",
    )
    parser.add_argument(
        "--fail-on-missing",
        action="store_true",
        help="Return non-zero exit code if any expected FCT file is missing or empty.",
    )
    return parser.parse_args()


def default_output_root(repo_root):
    return (
        Path(repo_root)
        / "ns-3"
        / "simulator"
        / "result"
        / "data"
        / "20-parameter-sweeping"
        / "infiniflow"
    )


def resolve_output_root(args):
    if args.root:
        return Path(args.root).expanduser().resolve()

    if args.repo_root:
        return default_output_root(Path(args.repo_root).expanduser().resolve())

    script_dir = Path(__file__).resolve().parent
    return default_output_root(script_dir.parent.parent.parent)


def normalize_fct_root(output_root):
    return output_root if output_root.name == "fct" else output_root / "fct"


def default_csv_dir(output_root, fct_root):
    if output_root.name == "fct":
        return fct_root.parent / "csv"
    return output_root / "csv"


def get_sweep_params():
    qmin_list = list(range(QMIN_MIN, QMAX_MAX, Q_STEP))
    return qmin_list, QMAX_MAX, Q_STEP


def expected_fct_files(fct_root, delays):
    qmin_list, qmax_max, q_step = get_sweep_params()

    for delay in delays:
        for trace in TRACE_ORDER:
            for qmin in qmin_list:
                for qmax in range(qmin + q_step, qmax_max + 1, q_step):
                    yield {
                        "delay": delay,
                        "trace": trace,
                        "qmin": qmin,
                        "qmax": qmax,
                        "path": fct_root / delay / f"{trace}_qmin{qmin}_qmax{qmax}.txt",
                    }


def calc_fct_slowdown_stats(fct_path):
    if not fct_path.exists():
        return {
            "status": "missing",
            "count": 0,
            "mean": np.nan,
            "p95": np.nan,
            "p99": np.nan,
            "malformed_lines": 0,
        }

    if fct_path.stat().st_size == 0:
        return {
            "status": "empty",
            "count": 0,
            "mean": np.nan,
            "p95": np.nan,
            "p99": np.nan,
            "malformed_lines": 0,
        }

    slowdowns = []
    malformed_lines = 0

    with fct_path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if not line.strip():
                continue

            cols = line.split()
            if len(cols) <= 7:
                malformed_lines += 1
                continue

            try:
                raw = float(cols[6]) / float(cols[7])
            except (ValueError, ZeroDivisionError):
                malformed_lines += 1
                continue

            slowdowns.append(max(1.0, raw))

    if not slowdowns:
        return {
            "status": "no_valid_rows",
            "count": 0,
            "mean": np.nan,
            "p95": np.nan,
            "p99": np.nan,
            "malformed_lines": malformed_lines,
        }

    arr = np.asarray(slowdowns, dtype=float)
    return {
        "status": "ok",
        "count": int(arr.size),
        "mean": float(np.mean(arr)),
        "p95": float(np.percentile(arr, 95)),
        "p99": float(np.percentile(arr, 99)),
        "malformed_lines": malformed_lines,
    }


def trace_sort_key(trace):
    return TRACE_ORDER.index(trace) if trace in TRACE_ORDER else len(TRACE_ORDER)


def fmt_cell(row, metric):
    status = row["status"]

    if status == "missing":
        return "MISS"
    if status == "empty":
        return "EMPTY"
    if status == "no_valid_rows":
        return "NaN"

    value = row[metric]
    return "NaN" if np.isnan(value) else f"{value:.4f}"


def write_delay_csv(rows, delay, csv_path):
    delay_rows = [row for row in rows if row["delay"] == delay]
    csv_path.parent.mkdir(parents=True, exist_ok=True)

    with csv_path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.writer(f)
        writer.writerow([f"DELAY: {delay}"])
        writer.writerow(["MISS=missing file", "EMPTY=zero-byte file", "NaN=no valid parsed rows"])
        writer.writerow([])

        for trace in sorted({row["trace"] for row in delay_rows}, key=trace_sort_key):
            trace_rows = [row for row in delay_rows if row["trace"] == trace]
            qmins = sorted({row["qmin"] for row in trace_rows})
            qmaxs = sorted({row["qmax"] for row in trace_rows})
            by_pair = {(row["qmin"], row["qmax"]): row for row in trace_rows}

            writer.writerow([f"TRACE: {trace}"])
            writer.writerow([])

            for metric, title in METRICS:
                writer.writerow([title])
                writer.writerow(["qmax/qmin"] + qmins)

                for qmax in qmaxs:
                    csv_row = [qmax]
                    for qmin in qmins:
                        if qmax <= qmin:
                            csv_row.append("")
                        else:
                            row = by_pair.get((qmin, qmax))
                            csv_row.append(fmt_cell(row, metric) if row else "MISS")
                    writer.writerow(csv_row)

                writer.writerow([])

            writer.writerow([])


def print_warnings(rows, max_details):
    bad_rows = [row for row in rows if row["status"] != "ok"]
    malformed_rows = [row for row in rows if row["malformed_lines"] > 0]

    if not bad_rows and not malformed_rows:
        print("No missing, empty, or malformed FCT files detected.")
        return

    if bad_rows:
        print("\n[WARN] Missing/empty/unusable FCT files detected:")
        summary = defaultdict(Counter)
        for row in bad_rows:
            summary[(row["delay"], row["trace"])][row["status"]] += 1

        for delay in DELAY_LABELS:
            for trace in TRACE_ORDER:
                counts = summary.get((delay, trace))
                if not counts:
                    continue
                parts = ", ".join(f"{status}={count}" for status, count in sorted(counts.items()))
                print(f"[WARN] {delay} {trace}: {parts}")

        if max_details > 0:
            print(f"[WARN] Showing first {min(max_details, len(bad_rows))} problematic files:")
            for row in bad_rows[:max_details]:
                print(f"[WARN] {row['status']}: {row['path']}")
            if len(bad_rows) > max_details:
                print(f"[WARN] ... {len(bad_rows) - max_details} more problematic files not shown.")

    if malformed_rows:
        total_malformed = sum(row["malformed_lines"] for row in malformed_rows)
        print(f"\n[WARN] Skipped malformed/parse-error lines: {total_malformed}")
        for row in malformed_rows[:max_details]:
            print(f"[WARN] malformed_lines={row['malformed_lines']}: {row['path']}")
        if len(malformed_rows) > max_details:
            print(f"[WARN] ... {len(malformed_rows) - max_details} more files with malformed lines not shown.")


def main():
    args = parse_args()

    output_root = resolve_output_root(args)
    fct_root = normalize_fct_root(output_root)

    if not fct_root.exists():
        print(f"[ERROR] FCT directory not found: {fct_root}")
        print("Pass /path/to/20-parameter-sweeping/infiniflow or /path/to/20-parameter-sweeping/infiniflow/fct.")
        return 1

    csv_dir = Path(args.out_dir).expanduser().resolve() if args.out_dir else default_csv_dir(output_root, fct_root)
    csv_dir.mkdir(parents=True, exist_ok=True)

    rows = []
    for item in expected_fct_files(fct_root, args.delays):
        stats = calc_fct_slowdown_stats(item["path"])
        rows.append({**item, **stats, "path": str(item["path"])})

    rows.sort(
        key=lambda row: (
            DELAY_LABELS.index(row["delay"]),
            trace_sort_key(row["trace"]),
            row["qmax"],
            row["qmin"],
        )
    )

    print(f"Analyzed FCT root: {fct_root}")
    print_warnings(rows, args.max_warning_details)

    written = []
    for delay in args.delays:
        csv_path = csv_dir / f"{args.prefix}_{delay}.csv"
        write_delay_csv(rows, delay, csv_path)
        written.append(csv_path)

    print("\nCSV written:")
    for path in written:
        print(f"  {path}")

    has_bad_rows = any(row["status"] != "ok" for row in rows)
    return 1 if args.fail_on_missing and has_bad_rows else 0


if __name__ == "__main__":
    raise SystemExit(main())
