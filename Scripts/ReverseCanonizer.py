"""
Script to expand files with canonized programs.
Each line with a canonized program is replaced by a line for every equivalent program.
The resulting output can then be processed further, e.g. to find out how many steps are actually
required to execute each program.
"""

import argparse
import sys
from collections import defaultdict

parser = argparse.ArgumentParser(
    prog="ReverseCanonizer",
    description="Expands canonized output"
)
parser.add_argument('-c', '--canonizer-table')
args = parser.parse_args()

# Identify possible duplicates first
counts = defaultdict(int)
with open(args.canonizer_table) as fp:
    for line in fp:
        fields = line.split("\t")
        assert (len(fields) == 3)

        canonized = fields[1]
        counts[hash(canonized)] += 1

# For possible duplicates store the programs
lookup = defaultdict(list)
with open(args.canonizer_table) as fp:
    for line in fp:
        fields = line.split("\t")

        canonized = fields[1]
        if counts[hash(canonized)] > 1:
            orig = fields[0]
            lookup[canonized].append(orig)

# Process standard in to expand canonized programs
for line in sys.stdin:
    line = line.strip()
    fields = line.split("\t", maxsplit=1)
    spec = fields[0]
    if " " not in spec:
        # Non-canonized program. Pass as is
        print(line)
        continue

    origs = lookup[spec]
    # Only programs with duplicates should be canonized
    assert len(origs) >= 2
    for orig in origs:
        if len(fields) > 1:
            print("\t".join([orig, fields[1]]))
        else:
            print(orig)
