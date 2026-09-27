"""
Script that executes Stage 2 of the Busy Beaver search.

It processes the input of the PrepStageTwo script.
It executes the candidate programs as binary code.
For this it first transpiles the program to C source code.
Next it compiles it to binary code.
"""

import argparse
from collections import defaultdict
import os
import time
import sys
import subprocess

parser = argparse.ArgumentParser(
    prog='RunStageTwo',
    description='Executes all Stage-2 programs (as compiled binaries)')
parser.add_argument("--max-steps", type=int, default=10_000_000)
parser.add_argument("--data-size", type=int, default=1_000_000)
parser.add_argument("--num-unrolls", type=int, default=16)
parser.add_argument("--report-period", type=int, default=1000)
parser.add_argument("--transpiler", type=str)
parser.add_argument("--work-dir", type=str, default="~/tmp/BusyBeaver/transpiled")
parser.add_argument("--result-file", type=str, default="results.txt")
args = parser.parse_args()


def exec(cmd):
    result = subprocess.run(cmd)
    if result.returncode:
        print(result)
        exit(-1)


def process(line):
    fields = line.strip().split("\t")
    count = int(fields[0])
    assert len(fields) == (2 if count == 1 else 3)
    program = fields[1]

    prog_id = i % 10
    prog_name = f"prog-{prog_id}"

    src_file = os.path.join(args.work_dir, f"{prog_name}.c")
    transpile_cmd = [
        args.transpiler,
        "--max-steps", str(args.max_steps),
        "--datasize", str(args.data_size),
        "--loopunrolls", str(args.num_unrolls),
        "--outfile", src_file,
    ]

    if count == 1:
        transpile_cmd.extend([
            "--program", program,
        ])
    else:
        transpile_cmd.extend([
            "--program", program,
            "--block-sizes", fields[2],
        ])

    result = subprocess.run(transpile_cmd)
    exec(transpile_cmd)

    bin_file = os.path.join(args.work_dir, prog_name)
    compile_cmd = ["gcc", src_file, "-o", bin_file]
    exec(compile_cmd)

    run_cmd = [bin_file]
    result = subprocess.run(run_cmd, text=True, capture_output=True)
    if result.returncode >= 0:
        steps = result.stdout.strip().split("\t")[1].split("=")[1]
        returncode = result.returncode
    else:
        print(result)
        exit(-1)

    rm_cmd = ["rm", src_file, bin_file]
    exec(rm_cmd)

    return program, returncode, steps


max_steps = 0
counts = defaultdict(int)
start_time = time.time()
with open(args.result_file, "a") as f:
    for i, line in enumerate(sys.stdin):
        program, returncode, steps = process(line)
        print("\t".join([program, str(returncode), steps]), file=f)
        counts[returncode] += 1

        if returncode == 0:
            max_steps = max(max_steps, steps)

        if (i + 1) % args.report_period == 0 or returncode == 0:
            print(
                f"elapsed={int(time.time() - start_time)}",
                f"total={i + 1}",
                f"{max_steps=}",
                " ".join([
                    f"#{key}={counts[key]}"
                    for key in sorted(counts.keys())
                ])
            )
