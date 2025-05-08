import subprocess
import os
import time

# 配置路径
os.chdir("/Users/yun/Downloads/C_project2_1")
CLASS_PATH = "out"
SRC_PATH = "src"

JAVA_MAIN_CLASS = "Main"
JAVA_TEST_CLASS = "JavaVecMulti"

C_SOURCE = "CVecMulti.c"
C_COMPILE_OPTIONS = ["-O0", "-O1", "-O2", "-O3","-Ofast"]


def measure_compile_time(command, description):
    """测量并打印编译时间"""
    start_time = time.time()
    try:
        subprocess.run(command, shell=True, check=True)
        elapsed = time.time() - start_time
        print(f"{description}完成 - 耗时: {elapsed:.2f}秒")
        return elapsed
    except subprocess.CalledProcessError as e:
        print(f"{description}失败: {e}")
        return 0


def compile_c_programs():
    """编译不同优化级别的C程序并返回总时间"""
    total_time = 0
    for opt in C_COMPILE_OPTIONS:
        cmd = f"gcc {SRC_PATH}/{C_SOURCE} -o {CLASS_PATH}/c{opt} {opt}"
        total_time += measure_compile_time(cmd, f"C程序编译({opt})")
    cmd = f"/opt/homebrew/opt/llvm/bin/clang++ -fopenmp -Ofast {SRC_PATH}/CVecMultiThread.c -o {CLASS_PATH}/c-Ofast-omp"
    total_time += measure_compile_time(cmd, f"C程序编译(omp)")
    return total_time


def compile_java_programs():
    """编译Java程序并返回耗时"""
    totaltime = 0
    cmd = f"javac -d {CLASS_PATH} {SRC_PATH}/{JAVA_MAIN_CLASS}.java {SRC_PATH}/{JAVA_TEST_CLASS}.java {SRC_PATH}/{JAVA_TEST_CLASS}Thread.java"
    totaltime += measure_compile_time(cmd, "Java程序编译")
    return totaltime


def generate_c_assembly():
    """生成C程序的汇编代码用于分析并返回总时间"""
    total_time = 0
    for opt in C_COMPILE_OPTIONS:
        # 编译为目标文件
        cmd = f"gcc -g {opt} -c {SRC_PATH}/{C_SOURCE} -o {CLASS_PATH}/c_obj{opt}.o"
        total_time += measure_compile_time(cmd, f"生成目标文件({opt})")

        # 反汇编
        cmd = f"objdump -S {CLASS_PATH}/c_obj{opt}.o > {CLASS_PATH}/c_ass{opt}.s"
        total_time += measure_compile_time(cmd, f"生成汇编代码({opt})")
    return total_time


def print_summary(java_time, c_time, asm_time):
    """打印编译时间摘要"""
    print("\n=== 编译时间摘要 ===")
    print(f"Java编译总时间: {java_time:.2f}秒")
    print(f"C程序编译总时间: {c_time:.2f}秒")
    print(f"汇编生成总时间: {asm_time:.2f}秒")
    print(f"全部任务总时间: {java_time + c_time + asm_time:.2f}秒")


if __name__ == "__main__":
    print("开始编译过程...\n")

    # 确保输出目录存在
    os.makedirs(CLASS_PATH, exist_ok=True)

    # 执行编译并计时
    java_time = compile_java_programs()
    c_time = compile_c_programs()
    asm_time = generate_c_assembly()

    # 打印摘要
    print_summary(java_time, c_time, asm_time)
    print("\n所有编译任务完成!")