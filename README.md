# MESI-MultiProcessor-Model

**Academic simulation of a multiprocessor system with MESI cache coherence**  
Teaching-oriented model: 4 Processing Elements (PEs) with private 2-way caches, shared memory, and an interconnect implementing the MESI protocol. Includes examples to run parallel dot product benchmarks and collect metrics.

---

## Project Status
**Progress:** Week 1 — Planning and definition (documents and initial structure).  
**Current objective:** Deliver Milestone 1 with specific objectives, roles, requirements extraction, and bibliography.

---

## Short Description
Academic simulation of a multiprocessor system with MESI coherence (4 PEs, 2-way caches, write-back / write-allocate).

---

## Key Features
- 4 PEs, each with 8 64-bit registers.  
- Private cache per PE: 2-way set associative, 16 blocks × 32 bytes, write-allocate + write-back.  
- MESI Protocol: states and messages (BusRd, BusRdX, BusUpgr, Writeback...).  
- Simulated main memory: 512 64-bit positions.  
- Hardware-like model: components modeled with threads.  
- Metrics collection per PE: cache misses, invalidations, bus traffic, MESI transitions, R/W accesses.  
- Benchmark: parallel dot product and automatic validation.

---

## Requirements 
- **Recommended language:** C++ (recommended) or SystemVerilog (non-synthesizable).  
- **Suggested tools (C++):** CMake, g++/clang++.  
- **Operating system:** Linux / macOS / Windows (with development environment).  
> Note: Python is **not** allowed.

---

## Structure
```
/docs/                 # Documentation (milestone1.pdf, meeting notes, bibliography)
/diagrams/             # Diagrams (architecture.png)
/src/                  # Source code (simulator, modules, utilities)
/src/examples/         # ASM examples/benchmarks (dot product by segment)
/tests/                # Test scripts and validation
/scripts/              # Build / run / analysis scripts
/README.md
/LICENSE
```

---

## How to compile / run 
> Adjusts throughout the project.

### Example (C++ with CMake)
```bash
# clone repo
git clone <repo-url>
cd MESI-MultiProcessor-Model

# compile
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# run (placeholder)
./mp_mesi_simulator --load ../src/examples/pdot_seg.asm --mem-config ../config/mem.cfg
```

### Example (SystemVerilog - ModelSim/Questa)
```bash
vlog src/*.sv
vsim top_tb
```

---

## Key files to include in Milestone 1 delivery
- `docs/milestone1.pdf` — planning, objectives, roles, extracted technical requirements.  
- `diagrams/architecture.png` — high-level diagram (PEs, caches, interconnect, memory).  
- `docs/cache_mesi.md` — cache specification and MESI transition table.  
- `src/examples/pdot_seg.asm` — ASM example for a dot product segment.   
- `README.md` — Project instructions.  

---

## Checklist Milestone1 (paste in delivery ZIP)
- [ ] `milestone1.pdf` (cover, objectives, requirements, roles, plan, bibliography).  
- [ ] `diagrams/architecture.png`.  
- [ ] `docs/cache_mesi.md`.  
- [ ] `src/examples/pdot_seg.asm`.
- [ ] `README.md` updated.  
- [ ] First commit: `init: project structure - milestone1` and tag `milestone1`.  
- [ ] ZIP with everything and upload to Tec Digital before deadline.

---

## Development Conventions
- Main branch: `main` (always stable).  
- Working branches: `feature/<name>` (one task per branch).  
- Clear commit messages: `feat:`, `fix:`, `docs:`, `chore:`.  
- Open PRs for merges and use reviewers among team members.

---

## How to contribute
1. Fork → Clone → Create branch: `feature/<name>`.  
2. Make atomic and descriptive commits.  
3. Open PR towards `main` with description and tests performed.  
4. Assign reviewer and approve before merge.

---