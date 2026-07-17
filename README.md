# MESI-MultiProcessor-Model

Author(s): `Kevin Rodríguez Lanuza and Saúl Gómez Ramírez`  
Course: CE-4302 — Computer Architecture II (II-2025)  
Professors: Ronald García Fernández

---

## One-line summary
We implemented an academic simulator of a 4-PE multiprocessor with private 2-way caches and MESI coherence. The simulator computes a parallel double-precision dot product, gathers cache/bus/memory metrics and supports an optional interactive stepping mode.

---

## What is in this repo
- `src/` — simulator source (PEs, caches, bus, memory, asm loader, optional step controller).  
- `include/` — `config.h` and other public headers (adjust sizes / addresses here).  
- `tests/` — optional tests (e.g. `mesi_stress.c`).  
- `Makefile` — build rules (see usage below).  
- Example ASM in `src/asm/` for the dot product program.  
- `docs/` — project documentation.  

---

## Requirements / Dependencies
- Linux with GCC/Clang and `make`.  
- `gcc` with `pthread` support.  
- `make` (GNU make).  

---

## Quick build & run (copy/paste)

### 1. Build normally
```bash
# clean and build
make clean
make -j4
```
This produces the main binary mp_mesi.
### 2. Build with stepping support (interactive)


``` bash
# enable the step controller at compile time
make clean
make -j4 STEP_CONTROL=1
```
When compiled with STEP_CONTROL=1 the binary accepts --step at runtime to enable interactive stepping.
### 3. Build the stress test (optional)
``` bash
make mesi_stress
#runs the stress test binary ./mesi_stress
```


### Run examples
#### Simple run (default)
``` bash
./mp_mesi
```
Output (full trace + metrics) is redirected to a file `ms_<timestamp>.txt` in the current directory. The program prints the name of the file at startup:
```bash
[MAIN] Logging en ms_20250921-153012.txt
```
#### Run with a custom run id
```bash
./mp_mesi run01
# creates file: ms_run01.txt
```
#### Interactive stepping (per-instruction)

##### 1. Build with `STEP_CONTROL=1` (see above).
##### 2. Run:
``` bash
./mp_mesi --step
```
Controls (typed into the program's stdin while it runs):

* Press ENTER — advance one step (one instruction for each PE that hits the step interval).

* Type c + ENTER — continue (disable stepping, program runs normally).

* Type q + ENTER — quit stepping (also continues execution).

Note: stepping is global control for threads; each PE calls step_control_maybe_pause(instr_count) once per instruction. The step controller will block threads at the requested interval until the user advances.
#### Run the stress test (multi-thread heavy traffic)
``` bash
./mesi_stress
# output is printed to terminal (or file if you redirect)
```


#### Where the logs go

   * By default the program redirects stdout and stderr to `ms_<id>.txt` (timestamp or provided id).

   * Use tail `-f ms_<id>.txt` to watch logs live while the binary runs (in another terminal).

### How to reduce `VECTOR_SIZE` for faster tests

Two options:

#### 1) Quick local edit
Open include/config.h and change:
``` c
#define VECTOR_SIZE 256   // (replace with a smaller value e.g. 16)
```

Then make clean && make.

#### 2) Build-time define (temporary)
You can override compile flags when invoking make:
``` bash
make clean
make -j4 CFLAGS+=' -DVECTOR_SIZE=16'
```
This avoids editing the header permanently. Note: the code and other constants (segment addresses) are defined assuming values in `include/config.h`; if you pick a very small `VECTOR_SIZE` ensure segment sizes still make sense.
### Important operational notes / caveats (must-read)

   * `mem_load_data` alignment unit: alignment is interpreted as number of doubles, not bytes. Example: `alignment=4` aligns to 4 double positions (4 × 8 bytes). Tests with nonzero offset are recommended to confirm the behavior. We left explicit logs and warnings in `memory.c` for this.

   * Segment safety: `mem_load_data` aligns the base inside the target segment; it checks segment capacity and aborts on overflow. If you change segment base addresses in `include/config.h`, re-check capacity calculations.

   * Stepping caveat: Stepping requires reading from stdin. If you redirect stdout/stderr to a file, interactively stepping from the same terminal still works, but if you run the program fully detached (no stdin) the stepping controller will not receive input.

   * Race conditions & testing: The MESI implementation and bus handlers have been exercised with a stress test, but heavy contention corner cases are hard to exhaustively prove—please run mesi_stress under different seeds and vector sizes when validating.

   * Do not submit binaries — the course specification forbids binary submissions. Submit source + README + docs.

### What the log contains (how to read it)

The log file contains:

   * `[PEx]` traces: PC, opcode, registers, `LOAD/STORE/ALU` traces.

   * `[BUS]` events: enqueued signals (`BusRd`, `BusRdX`, `BusUpgr)`, handler entries.

   * `[DEBUG]` memory reads/writes: segment, offset, physical addr, value.

   * Final flush writes to memory and per-PE partial sums.

   * `[MÉTRICAS - PEx]` section: read misses, write misses, writebacks, invalidations, MESI state transitions.

   * `[TRÁFICO DEL BUS]` summary: counts of `BusRd`, `BusRdX`, `BusUpgr`, `BusWB` per PE and total processed messages.

Example (fragment):
``` sharp
[BUS] Señal BusRd encolada por PE3 para Addr=10
[BUS] Handler BusRd (PE3, addr=10)
[DEBUG] Leyendo en memoria: Segmento=0, Offset=8, Addr=8, Valor=2004992.000000
[PE3] CACHE HIT: Addr=8 Set=2 Way=0 State=Shared
...
[MAIN] parcial[0] = 87360.000000
[MAIN] Producto punto calculado: 5334252.000000
[MAIN] ✅ Producto punto correcto.
```
### How we validated the project

   * Correctness: dot-product run compares computed result to a deterministic expected sum.

   * Stress: mesi_stress runs multiple threads issuing random reads/writes to force coherence activity; `metrics/logs` show writebacks, invalidations and MESI transitions.

   * We recommend running with different `VECTOR_SIZE` and repeating `mesi_stress` to exercise edge cases.

### How to run the grading demo (recommended sequence for the professor)

   * `make clean && make -j4 `

   * `./mp_mesi --step through instructions interactively; press ENTER for each step or c to continue.`

   * Inspect `ms_demo01.txt` (tail or open) and verify ✅ Producto punto correcto. and metrics.

   * `make mesi_stress && ./mesi_stress` — run stress test and inspect metrics at the end.

### What to turn in (submission checklist)

   * src/ — full sources (no binaries).

   * include/ — headers.

   * Makefile — build rules.

   * `README.md` 

   * src/asm/program.asm and any example asm used for the dot product.

### Contact / reproducibility

   * OS tested on: Arch Linux (gcc 12/13).

