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

mkdir -p "$BASE_SIMULATION_ROOT/ns-3/simulator/result/data/21-validating-prototype"

PROJECTS=(
    "ns-3-infiniflow-mmf"
)

# Relative path from each project root to the ablation study script
SCRIPT_REL_PATH="examples/SharedCredit-FC/21-validating-prototype/script.sh"

# ============================================================
# Execute each script with clear status messages
# ============================================================

for project in "${PROJECTS[@]}"; do
    SCRIPT_FULL_PATH="$BASE_SIMULATION_ROOT/ns-3/simulator/$project/$SCRIPT_REL_PATH"

    echo "▶️ Running ablation study script for project: $project"
    echo "   Script path: $SCRIPT_FULL_PATH"

    if [ -f "$SCRIPT_FULL_PATH" ]; then
        chmod +x "$SCRIPT_FULL_PATH" 2>/dev/null || true  # Ensure executable (ignore errors)
        "$SCRIPT_FULL_PATH"
    else
        echo "❌ Script not found: $SCRIPT_FULL_PATH"
        exit 1
    fi

    echo "✅ Completed: $project"
    echo "------------------------------------------------------------"
done