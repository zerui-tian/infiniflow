from pathlib import Path
import argparse
import csv
import re

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.cm import ScalarMappable
from matplotlib.colors import PowerNorm
import matplotlib.patheffects as pe
import matplotlib as mpl
mpl.rcParams["pdf.fonttype"] = 42
mpl.rcParams["ps.fonttype"] = 42


TRACE_ORDER = ["W3_load40", "W3_load80", "W4_load40", "W4_load80"]

TRACE_LABEL = {
    "W3_load40": "WebSearch 40%",
    "W3_load80": "WebSearch 80%",
    "W4_load40": "Hadoop 40%",
    "W4_load80": "Hadoop 80%",
}

UNIT_TO_NS = {
    "ns": 1,
    "us": 1_000,
    "ms": 1_000_000,
    "s": 1_000_000_000,
}

# Mild low-end enhancement while keeping one shared color scale.
GAMMA = 0.75
FONT_SIZE = 17
FIG_WIDTH = 18
FIG_HEIGHT = 3.5
FIG_HEIGHT_NO_X_AXIS = 3.05
STAR_SIZE = 573.75
LEGEND_STAR_MARKERSIZE = 24.75

# Optional manual adjustment retained from the old script.
OVERRIDE_STAR = {
    ("500ns", "W4_load40"): (6, 14),
}


def delay_sort_key(name):
    match = re.fullmatch(r"(\d+(?:\.\d+)?)(ns|us|ms|s)", name)
    if not match:
        return (float("inf"), name)
    value = float(match.group(1)) * UNIT_TO_NS[match.group(2)]
    return (value, name)


def delay_from_csv_name(path):
    match = re.search(r"(\d+(?:\.\d+)?(?:ns|us|ms|s))", path.stem)
    if not match:
        return None
    return match.group(1)


def panel_label(delay, trace):
    return f"link delay {delay}\n{TRACE_LABEL.get(trace, trace)} load"


def merge_data(target, source):
    for delay, traces in source.items():
        for trace, points in traces.items():
            target.setdefault(delay, {}).setdefault(trace, {}).update(points)


def parse_csv_heatmap(path, metric_name, fallback_delay=None):
    with open(path, "r", encoding="utf-8-sig", newline="") as f:
        rows = list(csv.reader(f))

    parsed = {}
    current_delay = fallback_delay
    current_trace = None
    i = 0

    while i < len(rows):
        row = [x.strip() for x in rows[i]]
        first = row[0] if row else ""

        if first.startswith("DELAY:"):
            current_delay = first.split(":", 1)[1].strip()
            i += 1
            continue

        if first.startswith("TRACE:"):
            current_trace = first.split(":", 1)[1].strip()
            i += 1
            continue

        if first == metric_name:
            j = i + 1
            while j < len(rows):
                header = [x.strip() for x in rows[j]]
                if header and header[0].lower().startswith("qmax/qmin"):
                    break
                j += 1

            if j >= len(rows) or current_delay is None or current_trace is None:
                i += 1
                continue

            header = [x.strip() for x in rows[j]]
            qmins = [int(float(x)) for x in header[1:] if x.strip()]
            k = j + 1

            while k < len(rows):
                r = [x.strip() for x in rows[k]]

                if not r or all(x == "" for x in r):
                    break
                if r[0].startswith("TRACE:") or r[0].startswith("DELAY:") or r[0] == metric_name:
                    break

                try:
                    qmax = int(float(r[0]))
                except ValueError:
                    break

                for x, raw_value in enumerate(r[1:1 + len(qmins)]):
                    if raw_value == "":
                        value = np.nan
                    else:
                        value = float(raw_value)
                    parsed.setdefault(current_delay, {}).setdefault(current_trace, {})[
                        (qmins[x], qmax)
                    ] = value

                k += 1

            i = k
            continue

        i += 1

    return parsed


def discover_csv_data(input_dir, metric_name):
    if input_dir.is_file():
        csv_paths = [input_dir]
    else:
        csv_paths = sorted(input_dir.glob("*.csv"))

    data = {}
    files_seen = 0

    for path in csv_paths:
        parsed = parse_csv_heatmap(
            path,
            metric_name,
            fallback_delay=delay_from_csv_name(path),
        )
        if parsed:
            merge_data(data, parsed)
            files_seen += 1

    return data, files_seen


def make_grid(points):
    qmins = sorted({qmin for qmin, _ in points})
    qmaxs = sorted({qmax for _, qmax in points})

    grid = np.full((len(qmaxs), len(qmins)), np.nan, dtype=float)
    qmin_to_x = {qmin: x for x, qmin in enumerate(qmins)}
    qmax_to_y = {qmax: y for y, qmax in enumerate(qmaxs)}

    for (qmin, qmax), value in points.items():
        grid[qmax_to_y[qmax], qmin_to_x[qmin]] = value

    return qmins, qmaxs, grid


def rel_degradation(arr):
    finite = arr[np.isfinite(arr)]
    if finite.size == 0:
        return np.full_like(arr, np.nan), np.nan

    best = float(np.min(finite))
    if best <= 0:
        return arr - best, best
    return (arr - best) / best * 100.0, best


def choose_single_min_point(arr, qmins, qmaxs):
    """
    Pick one optimal point.
    If there are multiple minima, choose the one with the largest qmax,
    then the largest qmin.
    """
    finite = np.isfinite(arr)
    min_val = np.nanmin(arr)
    positions = np.argwhere(finite & np.isclose(arr, min_val, rtol=1e-10, atol=1e-12))

    candidates = []
    for y, x in positions:
        candidates.append((qmaxs[int(y)], qmins[int(x)], int(y), int(x)))

    candidates.sort(key=lambda t: (t[0], t[1]), reverse=True)
    qmax, qmin, y, x = candidates[0]
    return y, x, qmin, qmax


def build_panels(data, delay_order, trace_order):
    panels = {}
    rel_values = []
    global_max_candidates = []

    for delay in delay_order:
        for trace in trace_order:
            points = data.get(delay, {}).get(trace)
            if not points:
                continue

            qmins, qmaxs, values = make_grid(points)
            rel, best = rel_degradation(values)

            panels[(delay, trace)] = {
                "qmins": qmins,
                "qmaxs": qmaxs,
                "values": values,
                "rel": rel,
                "best": best,
            }

            rel_values.extend(rel[np.isfinite(rel)].tolist())
            panel_max = np.nanmax(rel)
            positions = np.argwhere(
                np.isfinite(rel) & np.isclose(rel, panel_max, rtol=1e-10, atol=1e-12)
            )

            for y, x in positions:
                global_max_candidates.append(
                    {
                        "delay": delay,
                        "trace": trace,
                        "y": int(y),
                        "x": int(x),
                        "qmax": qmaxs[int(y)],
                        "qmin": qmins[int(x)],
                        "value": float(panel_max),
                    }
                )

    if not rel_values:
        raise RuntimeError("No valid FCT data was parsed.")

    global_max_candidates.sort(
        key=lambda d: (d["value"], d["qmax"], d["qmin"]),
        reverse=True,
    )

    return panels, max(rel_values), global_max_candidates[0]


def draw_delay_figure(delay, trace_order, panels, norm, global_max, output_dir):
    show_x_axis = delay == "2us"
    fig_height = FIG_HEIGHT if show_x_axis else FIG_HEIGHT_NO_X_AXIS

    fig, axes = plt.subplots(
        nrows=1,
        ncols=len(trace_order),
        figsize=(FIG_WIDTH, fig_height),
        dpi=220,
        sharex=True,
        sharey=True,
        constrained_layout=True,
    )

    if len(trace_order) == 1:
        axes = [axes]

    for c, trace in enumerate(trace_order):
        ax = axes[c]
        panel = panels.get((delay, trace))

        if panel is None:
            ax.set_axis_off()
            ax.text(
                0.5,
                0.5,
                f"{TRACE_LABEL.get(trace, trace)}\nmissing",
                transform=ax.transAxes,
                ha="center",
                va="center",
                fontsize=FONT_SIZE,
            )
            continue

        qmins = panel["qmins"]
        qmaxs = panel["qmaxs"]
        values = panel["values"]
        rel = panel["rel"]
        best = panel["best"]

        ax.imshow(
            np.ma.masked_invalid(rel),
            origin="lower",
            aspect="auto",
            norm=norm,
            cmap="viridis",
            interpolation="nearest",
        )

        if (delay, trace) in OVERRIDE_STAR:
            qmin_target, qmax_target = OVERRIDE_STAR[(delay, trace)]
            if qmin_target in qmins and qmax_target in qmaxs:
                sx = qmins.index(qmin_target)
                sy = qmaxs.index(qmax_target)
                star_qmin, star_qmax = qmin_target, qmax_target
            else:
                sy, sx, star_qmin, star_qmax = choose_single_min_point(
                    values,
                    qmins,
                    qmaxs,
                )
        else:
            sy, sx, star_qmin, star_qmax = choose_single_min_point(values, qmins, qmaxs)

        star = ax.scatter(
            [sx],
            [sy],
            marker="*",
            s=STAR_SIZE,
            c="red",
            edgecolors="black",
            linewidths=0.7,
            zorder=5,
        )
        star.set_path_effects([pe.withStroke(linewidth=1.3, foreground="black")])

        if delay == global_max["delay"] and trace == global_max["trace"]:
            text_x = max(0.5, global_max["x"] - 2.5)
            text_y = max(0.5, global_max["y"] - 3)
            txt = ax.annotate(
                f"{global_max['value']:.2f}%",
                xy=(global_max["x"], global_max["y"]),
                xytext=(text_x, text_y),
                textcoords="data",
                ha="left",
                va="top",
                fontsize=FONT_SIZE,
                color="black",
                zorder=6,
                arrowprops={
                    "arrowstyle": "->",
                    "color": "black",
                    "linewidth": 1.2,
                    "shrinkA": 2,
                    "shrinkB": 2,
                },
            )

        label_txt = ax.text(
            0.98,
            0.04,
            panel_label(delay, trace),
            transform=ax.transAxes,
            ha="right",
            va="bottom",
            fontsize=FONT_SIZE,
            color="black",
            zorder=6,
        )

        xticks = np.arange(0, len(qmins), 2)
        yticks = np.arange(0, len(qmaxs), 2)

        ax.set_xticks(xticks)
        ax.set_yticks(yticks)
        ax.set_yticklabels([str(qmaxs[i]) for i in yticks], fontsize=FONT_SIZE)

        if show_x_axis:
            ax.set_xticklabels([str(qmins[i]) for i in xticks], fontsize=FONT_SIZE)
            ax.set_xlabel("qmin (packets)", fontsize=FONT_SIZE)
        else:
            ax.set_xticklabels([])

        ax.tick_params(length=0)

        if c == 0:
            ax.set_ylabel("qmax (packets)", fontsize=FONT_SIZE)

        for spine in ax.spines.values():
            spine.set_visible(True)
            spine.set_color("black")
            spine.set_linewidth(1.2)

    # fig.suptitle(
    #     f"{delay}: {METRIC_LABEL} sensitivity to qmin/qmax",
    #     fontsize=FONT_SIZE,
    #     fontweight="bold",
    # )

    png_path = output_dir / f"mean_fct_heatmap_{delay}.png"
    pdf_path = output_dir / f"mean_fct_heatmap_{delay}.pdf"
    fig.savefig(png_path, bbox_inches="tight", pad_inches=0.12)
    fig.savefig(pdf_path, bbox_inches="tight", pad_inches=0.12)
    plt.close(fig)

    return png_path, pdf_path


def draw_legend(norm, output_dir):
    fig = plt.figure(figsize=(18, 1.35), dpi=220)
    cax = fig.add_axes([0.28, 0.50, 0.6, 0.28])
    sm = ScalarMappable(norm=norm, cmap="viridis")
    sm.set_array([])
    cbar = fig.colorbar(sm, cax=cax, orientation="horizontal")
    cbar.set_label(
        "Relative degradation from best (%)",
        fontsize=FONT_SIZE,
        labelpad=8,
    )
    cbar.ax.tick_params(labelsize=FONT_SIZE, length=2)

    star_ax = fig.add_axes([0.01, 0.18, 0.35, 0.66])
    star_ax.set_axis_off()
    star_ax.plot(
        0.10,
        0.52,
        marker="*",
        linestyle="None",
        markersize=16.5,
        markerfacecolor="red",
        markeredgecolor="black",
        transform=star_ax.transAxes,
    )
    star_ax.text(
        0.18,
        0.52,
        "optimal parameter setting",
        transform=star_ax.transAxes,
        ha="left",
        va="center",
        fontsize=FONT_SIZE,
        color="black",
    )

    png_path = output_dir / "mean_fct_heatmap_legend.png"
    pdf_path = output_dir / "mean_fct_heatmap_legend.pdf"
    fig.savefig(png_path, bbox_inches="tight", pad_inches=0.08)
    fig.savefig(pdf_path, bbox_inches="tight", pad_inches=0.08)
    plt.close(fig)

    return png_path, pdf_path



def parse_args():
    script_dir = Path(__file__).resolve().parent
    simulator_dir = script_dir.parent
    default_input_dir = (
        simulator_dir / "result" / "data" / "20-parameter-sweeping" / "infiniflow" / "csv"
    )
    default_output_dir = simulator_dir / "result" / "graph"

    parser = argparse.ArgumentParser(
        description="Draw qmin/qmax FCT sensitivity heatmaps from summary CSV files.",
    )
    parser.add_argument(
        "--input-dir",
        type=Path,
        default=default_input_dir,
        help="Directory containing fct_slowdown_*.csv files, or one CSV file.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_output_dir,
        help="Directory for generated figures.",
    )
    parser.add_argument(
        "--metric",
        choices=["mean", "p95", "p99"],
        default="mean",
        help="FCT slowdown metric used in each heatmap cell.",
    )
    parser.add_argument(
        "--delays",
        nargs="*",
        default=None,
        help="Delay folders to plot. Default: all folders under input-dir.",
    )
    return parser.parse_args()


METRIC_LABELS = {
    "mean": "Mean FCT Slowdown",
    "p95": "P95 FCT Slowdown",
    "p99": "P99 FCT Slowdown",
}
METRIC_LABEL = METRIC_LABELS["mean"]


def main():
    global METRIC_LABEL

    args = parse_args()
    METRIC_LABEL = METRIC_LABELS[args.metric]

    if not args.input_dir.exists():
        raise FileNotFoundError(f"Input directory not found: {args.input_dir}")

    args.output_dir.mkdir(parents=True, exist_ok=True)

    data, files_seen = discover_csv_data(args.input_dir, METRIC_LABEL)

    if args.delays is None:
        delay_order = sorted(data.keys(), key=delay_sort_key)
    else:
        delay_order = args.delays

    panels, global_vmax, global_max = build_panels(data, delay_order, TRACE_ORDER)
    if global_vmax <= 0:
        global_vmax = 1.0

    norm = PowerNorm(gamma=GAMMA, vmin=0, vmax=global_vmax)

    saved = []
    for delay in delay_order:
        if not any((delay, trace) in panels for trace in TRACE_ORDER):
            continue
        saved.extend(draw_delay_figure(delay, TRACE_ORDER, panels, norm, global_max, args.output_dir))

    saved.extend(draw_legend(norm, args.output_dir))

    print(f"Parsed files: {files_seen}")
    for path in saved:
        print(f"Saved: {path}")
    print(
        "Global maximum: "
        f"{global_max['value']:.4f}% at "
        f"{global_max['delay']} {global_max['trace']} "
        f"qmin={global_max['qmin']} qmax={global_max['qmax']}"
    )


if __name__ == "__main__":
    main()
