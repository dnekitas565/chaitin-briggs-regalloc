# Chaitin–Briggs register allocator (Part 1)

C++17, standard library only. Pipeline: TAC text → parse → CFG (+dominators, loops, depth) → liveness → interference graph → Briggs conservative coalescing → optimistic simplify/select → spill-code insertion (repeat until no spills) → emit + verify with the reference interpreter. Spill costs are either **static** (`10^loopDepth`) or **profile-guided** (block execution counts) — see `docs/NOVELTY.md`.

## Build, test, run
```sh
cmake -S . -B build -G Ninja        # or: mkdir build && cd build && cmake .. && make
cmake --build build
ctest --test-dir build              # 10 test binaries, 91 cases
./build/regalloc samples/straight_line.tac --dump-cfg --run
./build/regalloc samples/profile_demo.tac --k 4 --report
./build/regalloc samples/loop_high_pressure.tac --k 4 --cost profile --emit out.s --run
sh scripts/part1_table.sh           # sample x k x cost-model table (real measurements)
```
On the MinGW toolchain used during development, `CMakeLists.txt` links statically (see `docs/DECISIONS.md` #3).

## CLI
```
regalloc <file.tac> [--k N] [--cost static|profile] [--no-coalesce]
         [--dump-cfg] [--dump-loops] [--dump-liveness] [--dump-ig]
         [--emit out.s] [--report] [--run] [--csv]
```
`--k` ≥ 3 (default 4 when allocation is requested). `--dump-ig` also writes `ig.dot`. `--report` prints both cost models. `--csv` prints one row for scripts.

## The TAC language
One function per file, input-free, must terminate. `#` starts a comment.
```
x = 5        x = y        x = y + z   (also - * /)     x = load A     store A, x
label L:     jmp L        br x, L1, L2                 print x        ret x
spill R1 -> S0            reload R1 <- S0              (allocated output only)
```
Allocated output uses registers `R0..R{k-1}` and re-parses as ordinary TAC.

## Layout
| Path | Contents (owner) |
|---|---|
| `include/ir.h` | shared types (B+C) |
| `src/parser.cpp`, `interp.cpp`, `emit.cpp`, `main.cpp` | front end, interpreter, emit/report, CLI (C) |
| `src/cfg.cpp`, `liveness.cpp`, `interference.cpp`, `costmodel.cpp`, `spillcode.cpp` | analysis, costs, spill code (B) |
| `src/allocator.cpp` | coloring, coalescing, driver loop (A) |
| `tests/` | `test_framework.h` + 10 test files |
| `samples/` | `.tac` programs (`errors.tac` is intentionally invalid) |
| `scripts/part1_table.sh` | result table generator |
| `docs/` | `PROGRESS_REPORT_PART1.md`, `DECISIONS.md`, `NOVELTY.md` |

Targets: library `regalloc_core`, executable `regalloc`, tests `test_parser test_interp test_cfg test_ig test_cost test_alloc test_coalesce test_emit test_novelty test_integration`.

Part 2 (not included): live-range splitting, program generator and fuzzing, benchmark sweeps.
