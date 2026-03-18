#!/bin/bash
set -e  # Exit immediately on error

# --------------------- Configuration Section ---------------------
# List of ns-3 subdirectories to process
NS3_DIRS=(
    ns-3-bfc
    ns-3-dt
    ns-3-infiniband
    ns-3-infiniflow-mmf
    ns-3-infiniflow-notap
    ns-3-infiniflow-nothreshold
    ns-3-roce
    ns-3-xpass
)

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BASE_SIMULATION_ROOT="$( realpath "$SCRIPT_DIR/.." )"

# Relative path to config.sh file
CONFIG_SH_REL_PATH="examples/SharedCredit-FC/config.sh"
# ----------------------------------------------------------------

# Iterate through each ns-3 subdirectory
for dir in "${NS3_DIRS[@]}"; do
    echo "=================================================="
    echo "📂 Processing directory: $dir"
    echo "=================================================="

    # Absolute path
    dir_path="$BASE_SIMULATION_ROOT/ns-3/simulator/$dir"
    cd "$dir_path" || { echo "❌ Failed to enter directory: $dir_path"; exit 1; }

    if [ -f "./waf" ]; then
        echo "▶️ Running ./waf..."
        ./waf
    else
        echo "⚠️  ./waf not found"
    fi

    # ---------- Step 3: Update SIMULATION_ROOT in config.sh ----------
    config_sh_path="$dir_path/$CONFIG_SH_REL_PATH"

    if [ -f "$config_sh_path" ]; then
        sed -i "s|^SIMULATION_ROOT=.*|SIMULATION_ROOT=$BASE_SIMULATION_ROOT|" "$config_sh_path"
        echo "✅ Updated $config_sh_path"
    else
        echo "⚠️  $config_sh_path not found, skipping configuration update"
    fi

    echo "--------------------------------------------------"
done


# ---------- Step 4: Add execute permission to sh-script/*.sh ----------
SH_SCRIPT_DIR="$BASE_SIMULATION_ROOT/ns-3/simulator/sh-script"

if [ -d "$SH_SCRIPT_DIR" ]; then
    echo "▶️ Adding execute permission to all scripts in sh-script/"
    chmod +x "$SH_SCRIPT_DIR"/*.sh
    echo "✅ All .sh files in sh-script are now executable"
else
    echo "⚠️  Directory $SH_SCRIPT_DIR not found"
fi


echo "🎉 All directories processed successfully!"