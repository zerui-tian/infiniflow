import os
import numpy as np

QMIN_LIST   = [2, 3, 4, 5]
DELTA_LIST  = [2, 3, 4, 5]   # qmax = qmin + delta
TRACE_LIST  = ["W4_load40", "W4_load80"]

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

DATA_ROOT = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/15-evaluation")

def calc_mean_fct_slowdown(fct_path):
    if not os.path.exists(fct_path):
        print(f"[WARN] File not found: {fct_path}")
        return np.nan

    slowdowns = []
    try:
        with open(fct_path, "r") as f:
            for line in f:
                if not line.strip():
                    continue
                cols = line.split()
                raw = float(cols[6]) / float(cols[7])
                slowdowns.append(max(1.0, raw))
    except Exception as e:
        print(f"[WARN] Parse error: {fct_path}, err={e}")
        return np.nan

    return np.mean(slowdowns) if slowdowns else np.nan

def get_fct_file(trace, qmin, delta):
    qmax = qmin + delta
    fname = f"qmin{qmin}_qmax{qmax}.txt"
    
    file_path = os.path.join(DATA_ROOT, trace, "fct", "qmin_qmax", fname)
    return file_path

for TRACE in TRACE_LIST:
    print(f"\n{'='*60}")
    print(f"TRACE: {TRACE}")
    print('='*60)
    
    trace_path = os.path.join(DATA_ROOT, TRACE)
    if not os.path.exists(trace_path):
        print(f"[ERROR] Trace directory not found: {trace_path}")
        continue
        
    data = {}
    for delta in DELTA_LIST:
        vals = []
        for qmin in QMIN_LIST:
            fct_file = get_fct_file(TRACE, qmin, delta)
            vals.append(calc_mean_fct_slowdown(fct_file))
        data[delta] = vals

    print("\n" + "FCT Slowdown Results")
    print("-" * 40)
    header = f"{'qmin/qmax':<12}"
    for delta in DELTA_LIST:
        header += f"  Δ={delta:<8}"
    print(header)
    print("-" * 40)
    
    for i, qmin in enumerate(QMIN_LIST):
        row = f"qmin={qmin:<7}"
        for delta in DELTA_LIST:
            val = data[delta][i]
            if np.isnan(val):
                row += f"  {'NaN':<8}"
            else:
                row += f"  {val:<8.4f}"
        print(row)