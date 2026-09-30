"""
Script that executes Stage 2 of the Busy Beaver search.

It processes the input of the PrepStageTwo script.
It executes the candidate programs as binary code.
For this it first transpiles the program to C source code.
Next it compiles it to binary code.
"""

import argparse
from collections import defaultdict
from dataclasses import dataclass
import os
import time
import sys
import subprocess
from typing import Optional

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
    result = subprocess.run(cmd, text=True, capture_output=True)
    if result.returncode:
        print(result)
        print(result.stdout)
        print(result.stderr)
        exit(-1)


@dataclass
class Task:
    prog_name: str
    program: str
    block_sizes: Optional[str]
    src_file: str
    bin_file: str
    returncode: Optional[int] = None
    num_steps: Optional[int] = None


NUM_PIPELINE_STAGES = 4


class Runner:
    """
    Executes tasks in a pipeline set-up with the following stages:
    1. Transpile (creates source)
    2. Compile (creates binary)
    3. Run (executes binary)
    4. Clean-up (removes source and binary)

    Every execution step it executes each stage, but for a different task.

    This set-up is used to occassional glitches where the file produced by the previous
    pipeline stage was apparently not yet available as input for the next stage.
    """

    def __init__(self, options):
        self.work_dir = options.work_dir
        self.transpile_base_cmd = [
            options.transpiler,
            "--max-steps", str(options.max_steps),
            "--datasize", str(options.data_size),
            "--loopunrolls", str(options.num_unrolls),
        ]

        self.tasks = []
        self.num_pending = 0
        self.head_pos = 0
        self.tasks = [None] * NUM_PIPELINE_STAGES

    def schedule(self, line):
        assert (self.tasks[self.head_pos] is None)

        fields = line.strip().split("\t")
        count = int(fields[0])
        assert len(fields) == (2 if count == 1 else 3)
        program = fields[1]

        prog_name = f"prog-{self.head_pos}"
        self.num_pending += 1
        task = Task(
            prog_name=prog_name,
            program=program,
            block_sizes=fields[2] if count > 1 else None,
            src_file=os.path.join(self.work_dir, f"{prog_name}.c"),
            bin_file=os.path.join(self.work_dir, prog_name)
        )
        self.tasks[self.head_pos] = task

    def _transpile(self, task):
        if task is None:
            return

        transpile_cmd = self.transpile_base_cmd[:]
        transpile_cmd.extend(["--outfile", task.src_file, "--program", task.program])
        if task.block_sizes:
            transpile_cmd.extend(["--block-sizes", task.block_sizes])

        exec(transpile_cmd)

    def _compile(self, task):
        if task is None:
            return

        compile_cmd = ["gcc", task.src_file, "-o", task.bin_file]

        exec(compile_cmd)

    def _run(self, task):
        if task is None:
            return

        run_cmd = [task.bin_file]

        result = subprocess.run(run_cmd, text=True, capture_output=True)
        if result.returncode >= 0:
            task.num_steps = int(result.stdout.strip().split("\t")[1].split("=")[1])
            task.returncode = result.returncode
        else:
            print(result)
            exit(-1)

    def _rm(self, task):
        if task is None:
            return

        rm_cmd = ["rm", task.src_file, task.bin_file]

        exec(rm_cmd)

    def execute(self) -> Optional[Task]:
        """
        Executes each pipeline stage once. Returns the task (if any) that completed.
        """
        self._rm(self.tasks[(self.head_pos + 1) % NUM_PIPELINE_STAGES])
        self._run(self.tasks[(self.head_pos + 2) % NUM_PIPELINE_STAGES])
        self._compile(self.tasks[(self.head_pos + 3) % NUM_PIPELINE_STAGES])
        self._transpile(self.tasks[self.head_pos])

        self.head_pos = (self.head_pos + 1) % NUM_PIPELINE_STAGES
        task = self.tasks[self.head_pos]
        self.tasks[self.head_pos] = None

        if task:
            self.num_pending -= 1
        return task

    def is_done(self):
        return self.num_pending == 0


runner = Runner(args)

max_steps = 0
counts = defaultdict(int)
start_time = time.time()
with open(args.result_file, "a") as f:
    def handle_result(result: Task):
        global max_steps
        if result:
            print("\t".join([
                result.program,
                str(result.returncode),
                str(result.num_steps)
            ]), file=f)
            counts[result.returncode] += 1

            if result.returncode == 0:
                max_steps = max(max_steps, result.num_steps)

    for i, line in enumerate(sys.stdin):
        runner.schedule(line)

        handle_result(runner.execute())

        if (i + 1) % args.report_period == 0:
            print(
                f"elapsed={int(time.time() - start_time)}",
                f"total={i + 1}",
                f"{max_steps=}",
                " ".join([
                    f"#{key}={counts[key]}"
                    for key in sorted(counts.keys())
                ])
            )

    while not runner.is_done():
        handle_result(runner.execute())
