#!/bin/bash
set -e  # Exit immediately if any command fails

# ============================================================
# Auto-derive base simulation root directory from script location
# ============================================================

# Get absolute path of this script
SCRIPT_PATH="$(readlink -f "$0")"

# Get directory where this script resides (e.g., .../ns-3/simulator/sh-script)
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

# Traverse up 3 levels: sh-script → simulator → ns-3 → infiniflow
BASE_SIMULATION_ROOT="$(realpath "$SCRIPT_DIR/../../..")"

echo
echo "============================================================"
echo "📁 Auto-detected Simulation Root: $BASE_SIMULATION_ROOT"
echo "============================================================"
echo

# ============================================================
# Define target script paths relative to base root
# ============================================================

SCRIPT_ROCE="$BASE_SIMULATION_ROOT/ns-3/simulator/ns-3-roce/examples/SharedCredit-FC/2-motivation/script.sh"
SCRIPT_INFINIBAND="$BASE_SIMULATION_ROOT/ns-3/simulator/ns-3-infiniband/examples/SharedCredit-FC/2-motivation/script.sh"

mkdir -p "$BASE_SIMULATION_ROOT/ns-3/simulator/result/data/2-motivation"
# ============================================================
# Execute each script with clear status messages
# ============================================================
echo "▶️ Running ROCE motivation script..."
"$SCRIPT_ROCE" &
ROCE_PID=$!

echo "▶️ Running Infiniband motivation script..."
"$SCRIPT_INFINIBAND" &
IB_PID=$!

# 等待两个都完成
wait $ROCE_PID
ROCE_EXIT=$?

wait $IB_PID
IB_EXIT=$?

if [ $ROCE_EXIT -eq 0 ] && [ $IB_EXIT -eq 0 ]; then
    echo "✅ All motivation scripts executed successfully."
else
    echo "❌ Some scripts failed."
    exit 1
fi