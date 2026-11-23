# Verification Notes

Verified with GNU g++ using the repository Makefile.

Commands:
- `make clean`
- `make -j2`
- `make test`
- `python3 tools/gen_adders.py tests`
- `./sim tests/full_adder.net tests/full_adder.stim -o /tmp/fulladder.vcd`
- `./sim tests/ripple8.net tests/ripple8.stim`
- `./sim tests/cla8.net tests/cla8.stim`
- `./sim tests/counter.net tests/counter.stim`
- `./sim tests/loop_error.net tests/loop_error.stim` (expected exit code 2)

Results:
- Data-structure tests passed.
- Timing/VCD analysis tests passed.
- Circuit parsing and loop-detection tests passed.
- Full adder, ripple8, CLA8, and structural counter were parsed and analyzed.
- `loop_error.net` was rejected by the timing analyzer as a combinational loop.
- Measured critical-path delays in this implementation: full adder 4, ripple8 17, CLA8 10, structural counter output path 1.

Important limitation:
The counter netlist is included and accepted for structural/timing analysis, but DFF state transitions are not implemented. The CLI currently replays stimulus changes into VCD and reports timing; it does not claim to simulate sequential counter state.
