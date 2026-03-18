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

mkdir -p "$BASE_SIMULATION_ROOT/ns-3/simulator/result/data/3-motivation"
# ============================================================
# Define target script paths relative to base root
# ============================================================

# Infiniband: single script in 3-motivation/
SCRIPT_INFINIBAND="$BASE_SIMULATION_ROOT/ns-3/simulator/ns-3-infiniband/examples/SharedCredit-FC/3-motivation/script-common.sh"

# RoCE: three scripts in 3-motivation/ (in order)
ROCE_SCRIPTS=(
    "$BASE_SIMULATION_ROOT/ns-3/simulator/ns-3-roce/examples/SharedCredit-FC/3-motivation/script-common.sh"
    "$BASE_SIMULATION_ROOT/ns-3/simulator/ns-3-roce/examples/SharedCredit-FC/3-motivation/script-dcqcn.sh"
    "$BASE_SIMULATION_ROOT/ns-3/simulator/ns-3-roce/examples/SharedCredit-FC/3-motivation/script-hpcc.sh"
)

# ============================================================
# Execute Infiniband script
# ============================================================

echo "▶️ Running Infiniband 3-motivation script..."
if [ -f "$SCRIPT_INFINIBAND" ]; then
    chmod +x "$SCRIPT_INFINIBAND" 2>/dev/null || true  # Ensure executable (ignore errors)
    "$SCRIPT_INFINIBAND"
else
    echo "❌ Infiniband 3-motivation script not found: $SCRIPT_INFINIBAND"
    exit 1
fi

echo
echo "============================================================"
echo "▶️ Starting ROCE 3-motivation scripts (in sequence)..."
echo "============================================================"
echo

# ============================================================
# Execute RoCE scripts in order
# ============================================================

for script in "${ROCE_SCRIPTS[@]}"; do
    echo "------------------------------------------------------------"
    echo "▶️ Running ROCE script: $(basename "$script")"
    echo "------------------------------------------------------------"
    
    if [ -f "$script" ]; then
        chmod +x "$script" 2>/dev/null || true  # Ensure executable (ignore errors)
        "$script"
    else
        echo "❌ ROCE script not found: $script"
        exit 1
    fi
    
    echo "✅ Completed: $(basename "$script")"
    echo
done

echo "============================================================"
echo "✅ All 3-motivation scripts executed successfully."
echo "============================================================"