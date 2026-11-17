# Tasks

## 1. Duplicate-Input Validation

- [x] 1.1 Replace the separate `count()` and `insert()` with a range-based loop that checks `insert(...).second`; extend `transaction_tests.cpp` to cover distinct inputs, duplicate rejection, and unchanged `nDoS`. Verify with `make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu nud`, then run `./src/test_nu --run_test=transaction_tests/basic_transaction_tests --report_level=short` and `./src/test_nu --report_level=short`.

## 2. Focused Performance Measurement

- [x] 2.1 Benchmark the old and new duplicate-detection loops on identical input sequences under the same C++17 compiler settings. Verify equivalent accept/reject results, fewer comparator calls for distinct inputs, and record elapsed-time results without claiming a whole-chain speedup.

## 3. Historical Synchronization

- [x] 3.1 Validate the available historical data with the rebuilt node in an isolated data directory, as revised by the owner on 2026-11-17 instead of requiring full-network synchronization. Record the exclusive recovery-peer connection and partial replay, then reopen the retained database offline, verify the available local height and block hash, and shut down cleanly. State explicitly that the peer's full advertised height was not reached.

## 4. Scope Review

- [x] 4.1 Review the source, tests, OpenSpec status, and complete diff. Verify the four pre-existing user changes remain intact and only the proposed validation, regression test, and change artifacts were added.
