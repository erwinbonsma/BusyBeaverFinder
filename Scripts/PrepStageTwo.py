# Script to prepare input for Stage 2 of the 7x7 search
#
# It takes as input (on stdout) the programs assumed to hang in Stage 1 of the
# search. These programs should already have been canonized, with each line
# containing the following fields:
# - Program Spec
# - Spec of Canonized Interpreted Program
# - Size (in steps) of each program block
#
# It identifies programs that are equivalent and represents them by a program
# with minimal step counts per instruction.
#
# It outputs the following for sets of equivalent programs:
# - The number of programs in the set
# - Spec of Canonized Interpreted Program
# - Size (in steps) of each program block. It is the minimum of the step
#   sizes over all programs.
#
# When a program is unique, it outputs the following (to reduce space)
# - The number one (as the set size)
# - Program Spec

import fileinput

representative = {}
combined_steps = {}
counts = {}

for line in fileinput.input():
    fields = line.split("\t")
    assert (len(fields) == 3)

    key = fields[1]
    steps = [int(step) for step in fields[2].split()]
    if key in representative:
        counts[key] += 1
        combined_steps[key] = [
            min(a, b) for a, b in zip(combined_steps[key], steps)
        ]
    else:
        representative[key] = fields[0]
        counts[key] = 1
        combined_steps[key] = steps

for key, steps in combined_steps.items():
    if counts[key] == 1:
        print(f"1\t{representative[key]}")
    else:
        print("\t".join([
            str(counts[key]),
            key,
            " ".join(str(step) for step in combined_steps[key]),
        ]))
