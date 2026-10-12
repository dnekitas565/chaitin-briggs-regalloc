#!/bin/sh
# Owner: C
# Prints a markdown table: sample x k x cost model -> spills, executed spill/reload ops, moves removed, registers used.
# Every number comes from running the regalloc CLI (--csv). Usage: scripts/part1_table.sh [path/to/regalloc]
BIN="${1:-build/regalloc}"
SAMPLES="straight_line branchy loop_high_pressure loop_splittable move_chain profile_demo"
KS="3 4 6 8"

echo "| sample | k | cost | spills | executed spills | executed reloads | moves removed | registers used | output |"
echo "|---|---|---|---|---|---|---|---|---|"
for s in $SAMPLES; do
  for k in $KS; do
    for c in static profile; do
      "$BIN" "samples/$s.tac" --k "$k" --cost "$c" --csv | awk -F, -v s="$s" '{
        printf "| %s | %s | %s | %s | %s | %s | %s | %s | %s |\n", s, $2, $3, $4, $5, $6, $7, $8, $9 }'
    done
  done
done
