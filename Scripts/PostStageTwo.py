# Script that post-processes the results of Stage 2 of the 7x7 search
#
# It takes as input (on stdout) the log from the search with as inputs a
# representative program for each set of equivalent programs.
#
# As command line argument it expects the path to the file that contains a
# description of all programs from Stage 1 that are assumed to hang, in
# canonized format:
# - Program Spec
# - Spec of Canonized Interpreted Program
# - Size (in steps) of each program block

import argparse
import sys


parser = argparse.ArgumentParser(
    prog='PostStageTwo',
    description='Processes Stage-2 search results')
parser.add_argument('program_list')
args = parser.parse_args()

prog_specs = set()
for line in sys.stdin:
    if not (line.startswith("SUC ") or line.startswith("ESC ")):
        continue
    fields = line.split()
    if len(fields) != 3:
        print(f"Failed to parse {line}")
        exit(1)
    prog_specs.add(fields[2])

canonical_specs = {}
with open(args.program_list) as f:
    for line in f:
        fields = line.split("\t")
        if len(fields) != 3:
            print(f"Failed to parse {line}")
            exit(1)
        if fields[0] in prog_specs:
            canonical_specs[fields[1]] = []
        if fields[1] in canonical_specs:
            canonical_specs[fields[1]].append(fields[0])

for canonical_spec, prog_specs in canonical_specs.items():
    print(f"# {canonical_spec}")
    for prog_spec in prog_specs:
        print(prog_spec)
