import subprocess
import matplotlib.pyplot as plt
import os
from typing import List, Dict, Tuple
import time
import numpy as np

plt.rcParams['axes.unicode_minus'] = False

# Configuration paths
os.chdir("/Users/yun/Desktop/C_project2")
CLASS_PATH = "out"
DOCS_PATH = "docs"

# Configurable test programs
TEST_PROGRAMS = [
    "java -cp out JavaVecMulti",
    "java -Xint -cp out JavaVecMulti",
    # "java -cp out JavaVecMultiThread",
    "out/c-O0",
    "out/c-O1",
    "out/c-O2",
    "out/c-O3",
    # "out/c-Ofast",
    # "out/c-Ofast-omp"
]

TEST_CASE_FILE = "../randomCase.txt"

def generate_test_case(params: Dict):
    """Generate test cases"""
    cmd = [
        "java",
        "-cp", CLASS_PATH,
        "Main",
        str(params["type"]),
        str(params["is_simple"]).lower(),
        str(params["std"]).lower(),
        str(params["dimension"]),
        str(params["N"])
    ]
    subprocess.run(cmd, check=True)

def run_test(program: str) -> Tuple[float, str]:
    """Run a single test program and return (average time in ns, sum as string)"""
    result = subprocess.run(program.split(), capture_output=True, text=True, check=True)
    time_ns = None
    sum_str = None

    for line in result.stdout.splitlines():
        if "nano second" in line:
            time_ns = float(line.split(" ")[4])
        elif line.startswith("sum = "):
            sum_str = line.split(" = ")[1].strip()  # Get the entire sum value as string

    if time_ns is None or sum_str is None:
        raise ValueError(f"Failed to parse output for: {program}\nOutput:\n{result.stdout}")

    return time_ns, sum_str

def run_tests(params_list: List[Dict]) -> Dict[str, List[float]]:
    """Run all tests and collect results"""
    results = {prog: [] for prog in TEST_PROGRAMS}

    for params in params_list:
        print(f"\nRunning test with params: {params}")
        generate_test_case(params)
        time.sleep(1.0)

        for prog in TEST_PROGRAMS:
            time.sleep(0.1)
            try:
                elapsed, sum_val = run_test(prog)
                results[prog].append(elapsed)
                print(f"{prog.ljust(30)} Time: {elapsed:>10.1f} ns | Sum = {sum_val}")
            except Exception as e:
                print(f"Error running {prog}: {str(e)}")
                results[prog].append(float('nan'))

    return results

def run_ndim_constant_test(params_base: Dict) -> Tuple[List[str], Dict[str, List[float]]]:
    """Run tests with constant N*dimension"""
    constant = params_base["N"] * params_base["dimension"]
    dim_values = [
        params_base["dimension"] // 4,
        params_base["dimension"] // 2,
        params_base["dimension"],
        params_base["dimension"] * 2,
        params_base["dimension"] * 4
    ]

    results = {prog: [] for prog in TEST_PROGRAMS}
    dim_labels = []

    for dim in dim_values:
        N = constant // dim
        if N < 2:
            continue

        params = params_base.copy()
        params["dimension"] = dim
        params["N"] = N
        dim_labels.append(f"dim={dim}\nN={N}")

        generate_test_case(params)
        time.sleep(1.0)

        for prog in TEST_PROGRAMS:
            time.sleep(0.1)
            try:
                elapsed, sum_val = run_test(prog)
                results[prog].append(elapsed)
                print(f"{prog.ljust(30)} Time: {elapsed:>10.1f} ns | Sum = {sum_val}")
            except Exception as e:
                print(f"Error running {prog}: {str(e)}")
                results[prog].append(float('nan'))

    return dim_labels, results

def plot_results(params_list, results: Dict, x_param: str, fixed_params: Dict):
    """Plot performance results"""
    fig, ax = plt.subplots(figsize=(16, 8))

    title_map = {
        "is_simple": "Performance: Simple vs Complex Values",
        "type": "Performance Across Data Types",
        "std": "Impact of Standard Float Range",
        "dimension": "Impact of Vector Dimension",
        "N": "Impact of Vector Count",
        "ndim_constant": "Performance with Constant N×Dimension",
        "sync_growth": "Performance with Synchronized N and Dimension Growth"
    }
    title = title_map.get(x_param, f"Performance Comparison ({x_param})")

    fixed_text = "\n".join([f"{k}: {v}" for k, v in fixed_params.items()])
    plt.gcf().text(0.95, 0.05, fixed_text, ha='right', va='bottom', fontsize=10,
                  bbox=dict(facecolor='white', alpha=0.5))

    if x_param in ["ndim_constant", "sync_growth"]:
        x_values = range(len(params_list))
        x_labels = params_list
    else:
        x_values = [params[x_param] for params in params_list]
        x_labels = x_values

    if x_param in ["type", "std", "is_simple"]:
        bar_width = 0.15
        num_groups = len(results)
        index = np.arange(len(x_values))

        for i, (prog, times) in enumerate(results.items()):
            prog_name = prog.split('/')[-1] if '/' in prog else prog.split()[-1]
            pos = index + i * bar_width
            bars = ax.bar(pos, times, bar_width, label=prog_name, alpha=0.7)

            for bar in bars:
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width() / 2, height,
                       f'{height:.1f}', ha='center', va='bottom', fontsize=8)

        ax.set_xticks(index + bar_width * (num_groups - 1) / 2)
        ax.set_xticklabels(x_labels)
    else:
        for prog, times in results.items():
            prog_name = prog.split('/')[-1] if '/' in prog else prog.split()[-1]
            ax.plot(x_values, times, marker='o', label=prog_name)

            for x, y in zip(x_values, times):
                ax.text(x, y, f'{y:.1f}', ha='center', va='bottom', fontsize=8)

    ax.set_xlabel(x_param)
    ax.set_ylabel("Execution Time (ns)")
    ax.set_title(title)
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    ax.grid(True, linestyle='--', alpha=0.5)

    plot_file = f"{DOCS_PATH}/performance_{x_param}.png"
    plt.savefig(plot_file, bbox_inches='tight', dpi=300)
    print(f"Plot saved to {plot_file}")
    plt.close()

def main():
    base_params = {
        "type": 5,
        "is_simple": False,
        "std": False,
        "dimension": 5000,
        "N": 10000
    }

    # # Test 1: Vary dimension
    # params_dimension = []
    # for dim in [10, 50, 100, 200, 500]:
    #     p = base_params.copy()
    #     p["dimension"] = dim
    #     params_dimension.append(p)
    #
    # print("\nRunning dimension tests...")
    # results_dim = run_tests(params_dimension)
    # fixed_params = {k: v for k, v in base_params.items() if k != "dimension"}
    # plot_results(params_dimension, results_dim, "dimension", fixed_params)
    #
    # Test 2: Vary N
    # params_N = []
    # for n in [500, 1000, 2000, 5000, 10000]:
    #     p = base_params.copy()
    #     p["N"] = n
    #     params_N.append(p)
    #
    # print("\nRunning vector count tests...")
    # results_N = run_tests(params_N)
    # fixed_params = {k: v for k, v in base_params.items() if k != "N"}
    # plot_results(params_N, results_N, "N", fixed_params)

    # Test 3: Vary type
    params_type = []
    for t in [1, 2, 3, 4, 5]:
        p = base_params.copy()
        p["type"] = t
        params_type.append(p)

    print("\nRunning data type tests...")
    results_type = run_tests(params_type)
    fixed_params = {k: v for k, v in base_params.items() if k != "type"}
    plot_results(params_type, results_type, "type", fixed_params)

    # # Test 4: Simple vs complex
    # print("\nRunning simple vs complex value tests (float)...")
    # params_simple = [
    #     {**base_params, "type": 5, "is_simple": False},
    #     {**base_params, "type": 5, "is_simple": True}
    # ]
    #
    # print("Test parameters:")
    # for i, p in enumerate(params_simple):
    #     print(f"{i + 1}. type=float, is_simple={p['is_simple']}")
    #
    # results_simple = run_tests(params_simple)
    # fixed_params = {k: v for k, v in base_params.items() if k not in ["is_simple", "type"]}
    # fixed_params["data_type"] = "float"
    # plot_results(params_simple, results_simple, "is_simple", fixed_params)
    #
    # # Test 5: Standard range comparison
    # params_std = []
    # for std in [False, True]:
    #     p = base_params.copy()
    #     p["std"] = std
    #     p["type"] = 4  # Use double for std test
    #     params_std.append(p)
    #
    # print("\nRunning standard range tests...")
    # results_std = run_tests(params_std)
    # fixed_params = {k: v for k, v in base_params.items() if k not in ["std", "type"]}
    # fixed_params["type"] = "double"
    # plot_results(params_std, results_std, "std", fixed_params)
    #
    # # Test 6: Constant N×dim
    # print("\nRunning constant N×dimension tests...")
    # dim_labels, results_ndim = run_ndim_constant_test(base_params)
    # fixed_params = base_params.copy()
    # fixed_params["N×dim"] = base_params["N"] * base_params["dimension"]
    # plot_results(dim_labels, results_ndim, "ndim_constant", fixed_params)
    #
    # # Test 7: Synchronized N and dimension growth
    # print("\nRunning synchronized N and dimension increase tests...")
    # params_sync = []
    # sync_ratios = [
    #     (50, 10),
    #     (75, 15),
    #     (100, 20),
    #     (300, 60),
    #     (500, 100),
    #     (750, 150),
    #     (1000, 200)
    # ]
    #
    # for n, dim in sync_ratios:
    #     p = base_params.copy()
    #     p["N"] = n
    #     p["dimension"] = dim
    #     params_sync.append(p)
    #
    # results_sync = run_tests(params_sync)
    # sync_labels = [f"N={n}\ndim={d}" for n, d in sync_ratios]
    # fixed_params = {k: v for k, v in base_params.items() if k not in ["N", "dimension"]}
    # fixed_params["sync growth"] = "N and dim all grows"
    # plot_results(sync_labels, results_sync, "sync_growth", fixed_params)

if __name__ == "__main__":
    print("Available test programs:")
    for i, prog in enumerate(TEST_PROGRAMS, 1):
        print(f"{i}. {prog}")

    main()