# InfiniFlow: ns-3 Simulation Code 

This repository contains the ns-3 simulation code for **InfiniFlow**, as used in our SIGCOMM submission.  
The implementation is based on **ns-3.39** and supports reproducible experiments for evaluating
credit allocation behavior and flow throughput under controlled network scenarios.

---

## 1. Artifact Overview

**Artifact type:** Simulation-based evaluation  
**Purpose:** Reproduce the key experimental results reported in the paper  
**Claims supported by this artifact:**

- Credit allocation behavior under InfiniFlow
- Flow throughput trends under different traffic conditions

The artifact automatically runs ns-3 simulations and generates the figures used for evaluation.

---

## 2. Environment Requirements

### Software

- Linux environment (tested on Ubuntu)
- GCC / G++ with C++17 support
- CMake and Ninja (required by ns-3.39)
- Python 3 (for result processing and plotting)

---

## 3. Build and Configure ns-3

After cloning the repository, navigate to the ns-3 root directory:

```bash
cd ${PATH_TO_INFINIFLOW_FOLDER}/ns-3/simulator/ns-3.39
```

### Configure ns-3

Modify the environment variables in the following file to your path to InfiniFlow folder.

```
${PATH_TO_INFINIFLOW_FOLDER}/ns-3/simulator/ns-3.39/examples/infiniflow/config.sh
```

Run the configuration script:

```bash
./configure.sh
```

### Build ns-3

Compile the simulator:

```bash
./waf
```

## 4. Running InfiniFlow Example Experiment

We use the microbenchmark corresponding to Figure 14 in our paper as an example for this open-source code.
Navigate to the InfiniFlow experiment directory:

```bash
cd ${PATH_TO_INFINIFLOW_FOLDER}/ns-3/simulator/ns-3.39/examples/infiniflow
```

### Configure and Run Experiments

1. Open `config.sh` and modify all path-related variables to match **your local directory layout**.
2. Run the experiment script:

```
./script.sh
```

All experiments will be executed automatically without further user interaction.

## 5. Output and Results

All results are generated in the following directory:

```
examples/infiniflow/_result/
```

The main output figures are:

- **Infiniflow_inflight_stacked_buffer_usage.png**
  *Credit Allocation Ratio*
- **InfiniFlow_sending_rate.png**
  *Flow Throughput*

These figures correspond directly to the evaluation metrics presented in the paper.

