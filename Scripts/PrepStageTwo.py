# Script to prepare input for Stage 2 of the 7x7 search
#
# It takes as input (on stdout) the programs assumed to hang in Stage 1 of the
# search. These programs should already have been canonized, with each line
# containing the following fields:
# - Program Spec
# - Spec of Canonized Interpreted Program
# - Size (in steps) of each program block
#
# It separates the programs in two batches. The first batch contains all
# programs that are functionally unique (for which there is no equivalent
# program). These are listed by their Program Spec.
#
# The second batch contains a representative program for each set of equivalent
# programs. Each line contains the following fields:
# - Program Spec for the first program in the set. This can be used as ID to
#   identifying the equivalence set from the logs produced by the Stage 2
#   search. Furthermore, it enables quick inspection using the 2LBB Runner.
# - Spec of Canonized Interpreted Program
# - Size (in steps) of each program block. It is the minimum of the step
#   sizes over all programs.

import sys

combined_steps = {}
representative = {}
duplicates = set()

for line in sys.stdin:
    fields = line.split("\t")
    assert (len(fields) == 3)

    key = fields[1]
    steps = [int(step) for step in fields[2].split()]
    if key in representative:
        duplicates.add(key)
        combined_steps[key] = [
            min(a, b) for a, b in zip(combined_steps[key], steps)
        ]
    else:
        combined_steps[key] = steps
        representative[key] = fields[0]

for key, spec in representative.items():
    if not key in duplicates:
        print(spec)
for key in duplicates:
    print("\t".join([
        representative[key],
        key,
        " ".join(str(step) for step in combined_steps[key])
    ]))
