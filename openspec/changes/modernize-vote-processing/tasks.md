# Tasks

## 1. Focused Vote-Processing Refactor

- [x] 1.1 Capture the pre-existing diff and legacy routines without changing the checkout. Establish the local baseline with `make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu` and `./src/test_nu --run_test=vote_tests --report_level=short`; record exact exit statuses and any existing failures.
- [x] 1.2 Add preservation tests in `src/test/vote_tests.cpp` before refactoring: unsorted distinct/nonconsecutive duplicate durations in both validation contexts; unchanged grant inputs including coin-age metadata; empty-input output clearing; zero-weight rejection. Implement the const-reference declaration/definition, reference-based native loops/structured bindings in grant generation, and single-insertion duration check. Verify with the local `test_nu` build and `vote_tests` command from 1.1, including existing majority and protocol-v2 ordering cases.
- [x] 1.3 Compare captured legacy and modernized routines using identical inputs in a temporary local C++17 `-O2` benchmark, including a production-sized vote window and distinct/duplicate duration workloads. Verify equivalent return values and grant outputs with fixed mock time before timing; record the command, sample method, median results, and limitations in `design.md`, without a whole-node performance claim.

## 2. Local Integration and Scope Verification

- [x] 2.1 Build both consumers with `make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu nud`, then run `./src/test_nu --report_level=short` once against the final source. Record commands, exit statuses, and relevant results; report unavailable platform/GUI/historical replay coverage rather than claiming it.
- [x] 2.2 Run `openspec validate modernize-vote-processing --strict --no-interactive` and `git diff --check`; inspect the final diff against the captured baseline. Verify only the selected source/tests and this change's artifacts are staged, then create the required local commit without pushing or triggering CI.
