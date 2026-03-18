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
# Define target evaluation script paths
# ============================================================

EVAL_DIR="$BASE_SIMULATION_ROOT/ns-3/simulator/ns-3-infiniflow-mmf/examples/SharedCredit-FC/15-evaluation"

SCRIPT_BUFFER="$EVAL_DIR/script-buffer.sh"
SCRIPT_QM="$EVAL_DIR/script-qm.sh"
SCRIPT_VC_SATURATION="$EVAL_DIR/script-vc-saturation.sh"

# ============================================================
# Execute each script in order with clear status messages
# ============================================================

echo "▶️ Running evaluation script: script-buffer.sh"
if [ -f "$SCRIPT_BUFFER" ]; then
    chmod +x "$SCRIPT_BUFFER" 2>/dev/null || true  # Ensure executable (ignore errors)
    "$SCRIPT_BUFFER"
else
    echo "❌ Script not found: $SCRIPT_BUFFER"
    exit 1
fi

echo
echo "▶️ Running evaluation script: script-qm.sh"
if [ -f "$SCRIPT_QM" ]; then
    chmod +x "$SCRIPT_QM" 2>/dev/null || true
    "$SCRIPT_QM"
else
    echo "❌ Script not found: $SCRIPT_QM"
    exit 1
fi

echo
echo "▶️ Running evaluation script: script-vc-saturation.sh"
if [ -f "$SCRIPT_VC_SATURATION" ]; then
    chmod +x "$SCRIPT_VC_SATURATION" 2>/dev/null || true
    "$SCRIPT_VC_SATURATION"
else
    echo "❌ Script not found: $SCRIPT_VC_SATURATION"
    exit 1
fi

echo
echo "✅ All 15-evaluation scripts executed successfully for ns-3-infiniflow-mmf."
echo "============================================================"