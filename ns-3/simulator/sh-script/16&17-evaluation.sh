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
# Define project groups
# ============================================================

# Single-script projects (4 total)
SINGLE_SCRIPT_PROJECTS=(
    "ns-3-bfc"
    "ns-3-infiniband"
    "ns-3-infiniflow-mmf"
    "ns-3-xpass"
)

# Multi-script project (1 total)
MULTI_SCRIPT_PROJECT="ns-3-roce"
MULTI_SCRIPT_DIR="$BASE_SIMULATION_ROOT/ns-3/simulator/$MULTI_SCRIPT_PROJECT/examples/SharedCredit-FC/16-large-scale"
MULTI_SCRIPTS=(
    "script-common.sh"
    "script-dcqcn.sh"
    "script-hpcc.sh"
)

# Common base path for all projects
BASE_PATH="$BASE_SIMULATION_ROOT/ns-3/simulator"

# ============================================================
# Execute single-script projects
# ============================================================

for project in "${SINGLE_SCRIPT_PROJECTS[@]}"; do
    SCRIPT_PATH="$BASE_PATH/$project/examples/SharedCredit-FC/16-large-scale/script-scale.sh"

    echo "▶️ Running SharedCredit-FC 16-large-scale script for project: $project"
    echo "   Script path: $SCRIPT_PATH"

    if [ -f "$SCRIPT_PATH" ]; then
        chmod +x "$SCRIPT_PATH" 2>/dev/null || true  # Ensure executable (ignore errors)
        "$SCRIPT_PATH"
    else
        echo "❌ Script not found: $SCRIPT_PATH"
        exit 1
    fi

    echo "✅ Completed: $project"
    echo "------------------------------------------------------------"
done

# ============================================================
# Execute multi-script project (RoCE)
# ============================================================

echo "▶️ Running SharedCredit-FC 16-large-scale scripts for project: $MULTI_SCRIPT_PROJECT"
echo "   Base directory: $MULTI_SCRIPT_DIR"

for script in "${MULTI_SCRIPTS[@]}"; do
    FULL_SCRIPT_PATH="$MULTI_SCRIPT_DIR/$script"

    echo "   ▶️ Running: $script"
    if [ -f "$FULL_SCRIPT_PATH" ]; then
        chmod +x "$FULL_SCRIPT_PATH" 2>/dev/null || true  # Ensure executable (ignore errors)
        "$FULL_SCRIPT_PATH"
    else
        echo "❌ Script not found: $FULL_SCRIPT_PATH"
        exit 1
    fi

    echo "   ✅ Completed: $script"
done

echo "✅ Completed multi-script project: $MULTI_SCRIPT_PROJECT"
echo "------------------------------------------------------------"

# ============================================================
# Final summary
# ============================================================

echo
echo "🎉 All SharedCredit-FC 16-large-scale scripts executed successfully."
echo "============================================================"