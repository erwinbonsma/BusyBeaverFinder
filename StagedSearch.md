This file shows via example command-lines how to carry out an optimised staged
search.

The upper-limit for Stage 2 (and Stage 3) is 10 million steps so that each
search runs relatively quickly.

# Stage 1 search

Run Stage 1 search:
```
Binaries/BusyBeaverFinder -w 7 -h 7 -d 500000 --max-steps 1000000 --max-search-steps 1000000 --max-hang-detection-steps 100000 --dump-period 10000 | tee log-stage1.txt
```

Extract all assumed hangs:
```
cat log-stage1.txt | grep "^ASS " | cut -d" " -f2 > programs-stage1-output.txt
```

# Stage 2 search

Process all assumed hangs from Stage 1 results:
```
cat programs-stage1-output.txt | Binaries/Canonizer > programs-canonized.txt
cat programs-canonized.txt | python3 Scripts/PrepStageTwo.py | tee programs-stage2-input.txt
cat programs-stage2-input.txt | grep -v "\t" > programs-stage2-input-unique.txt
cat programs-stage2-input.txt | grep "\t" > programs-stage2-input-equiv.txt
```

Execute unique programs:
```
Binaries/BusyBeaverFinder -w 7 -h 7 -d 5000000 --max-steps 10000000 --run-mode ONLYRUN --input-file programs-stage2-input-unique.txt --dump-period 10000 | tee log-stage2-unique.txt
```

Execute representative program for each equivalence set
```
Binaries/BusyBeaverFinder -w 7 -h 7 -d 5000000 --max-steps 10000000 --run-mode ONLYRUN --input-file programs-stage2-input-equiv.txt --dump-period 10000 | tee log-stage2-equiv.txt
```

Post-process the results:
```
cat log-stage2-equiv.txt | python3 Scripts/PostStageTwo.py programs-canonized.txt | tee programs-stage2b.txt
```

Follow up these results:
```
Binaries/BusyBeaverFinder -w 7 -h 7 -d 5000000 --max-steps 20000000 --run-mode ONLYRUN --input-file programs-stage2b.txt | tee log-stage2b-equiv.txt
```
Note: Increased program limit, as programs are known to terminate or escape,
but typically take longer to do so than the representative canonized program.

# Stage 3 search

Identify all programs for stage 3:
```
cat log-stage2-unique.txt log-stage2b-equiv.txt | grep "^ESC " > programs-stage3-input.txt
```
