# Design

## Context

See `proposal.md` for motivation. Both supported makefiles already select C++17. Production uses a 10,000-vote custodian window, and each `CVote` owns several containers.

`GenerateCurrencyCoinBases` only reads its vote argument. Its three production callers build a local vote vector before invoking it: `CBlock::CheckCustodianGrants`, `CreateNewBlock`, and the RPC block-construction path. No caller relies on an independently owned parameter or passes a function pointer.

`CParkRateVote::IsValid` validates the currency, then each rate, then duration uniqueness. Existing vote tests cover grant majorities, already-elected custodians, multiple units, protocol-v2 address ordering, and duplicate durations.

## Goals / Non-Goals

**Goals:**

- Make read-only ownership explicit and eliminate the vote-window copy.
- Express map entries directly without iteration macros or repeated `.first`/`.second` aliases.
- Preserve the exact traversal order, majority thresholds, tie behavior, early returns, and output-clearing behavior.

**Non-Goals:**

- Upgrade beyond C++17 or replace containers, dependencies, serialization, or monetary arithmetic.
- Refactor unrelated vote routines, transaction validation, build scripts, or UI.
- Remove the copy/sort in `GetPremium`: existing tests deliberately supply unsorted rates, so assuming ordered input would be unsafe.
- Contact the recovery VM, run cloud checks, or claim historical replay compatibility from unit tests.

## Decisions

1. **Borrow votes through `const std::vector<CVote>&`.** Update both the declaration and definition. The vector remains owned by each caller for the full call. This removes a deep copy without changing data lifetimes or adding an ownership abstraction.
2. **Modernize only the selected loops.** Use `const` references in range-based vote loops and `const auto&` structured bindings for the counter, per-unit grant, and sorted grant-entry loops. Keep the ordered maps, `CompareCustodianGrantByAddress`, sorting conditions, and transaction construction unchanged. Retain project naming rather than mixing in a repository-wide rename.
3. **Use the set insertion result for duration uniqueness.** Keep rate validation first, then reject when `insert(...).second` is false. Retain the same set, comparator, and plain `false` return. A hash container, bitset, or new helper adds unnecessary scope.
4. **Test preservation rather than invent new behavior.** Add small cases for nonconsecutive duplicate durations and unsorted distinct durations in both user-vote and in-block contexts. Check that grant generation does not mutate votes, including `nCoinAgeDestroyed`, which `CVote::operator==` intentionally omits. Cover empty-input output clearing and the existing zero-weight error path.
5. **Separate structural improvements from measured throughput.** The eliminated vector copy and redundant lookup are source-level facts. Compare captured legacy and modernized routines in a temporary local C++17 `-O2` harness using identical inputs, fixed mock time, and repeated samples. Check result equivalence before reporting median timings. Keep the harness/logs outside tracked product files and record its command, workload, and limitations with the results; do not infer a whole-node gain.

## Preservation Scenarios

- Empty votes return success and clear pre-existing output transactions.
- A zero-coin-age vote still returns failure without mutating the input.
- Existing weighted/count majorities, conflicting grant amounts, and already-elected exclusions produce the same outputs.
- NuBits/NuShares transaction ordering and protocol-v2 address-based output ordering remain unchanged.
- Empty or unsorted distinct valid park-rate durations remain valid; repeated durations remain invalid regardless of their position or rate.
- Invalid currency, compact duration, or rate remains invalid under the same protocol/context checks.

## Verification Plan

The implementing agent owns all verification and records exact commands, exit statuses, and relevant output:

1. Rebuild and run the current `vote_tests` before source changes to establish a baseline; add the focused preservation tests before refactoring.
2. Rebuild `test_nu` and `nud` after the refactor to catch declaration/definition and caller-linkage mismatches.
3. Run the final `vote_tests` and complete local unit suite once against the reconciled source state. Re-run only evidence made stale by later source edits.
4. Measure the isolated routines locally and report both behavioral equivalence and timing limitations. No timing assertion belongs in the unit suite.
5. Validate this OpenSpec change and inspect the final diff. Stage only this implementation's source, tests, and change artifacts; preserve all pre-existing work.

## Risks / Trade-offs

- **[Consensus-sensitive output order changes]** → Retain containers, comparator, sorting, arithmetic, and existing ordering tests.
- **[Missing mutation coverage]** → Compare the original input and explicitly check coin-age metadata, not only `CVote::operator==`.
- **[Stale object files after the signature change]** → Rebuild both executable targets; inspect the local build evidence.
- **[Microbenchmark overstates real gains]** → Report workload-specific results only, including inconclusive results.
- **[Unrelated dirty work is included]** → Keep the authorized `main` checkout and stage an explicit file list; do not stash, reset, or switch branches.

## Migration Plan

No chain, wallet, or database migration is required. Rebuild all consumers of the changed C++ declaration before using the new executable. If preservation checks fail, undo only this refactor's edits; leave pre-existing working-tree changes intact.

## Local Verification Evidence

Evidence directory: `/private/var/folders/9b/_93lfybj5992zvh8qn4ybgf00000gn/T/opencode/nubits-vote.ueyaE3`. It contains the captured legacy source/header, pre-existing diff/checksums, and command logs; it is not a product dependency.

Before source changes:

- `make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu`: exit 0 (`baseline-build.log`).
- `./src/test_nu --run_test=vote_tests --report_level=short`: exit 0; 17 vote cases and 34,734 assertions passed (`baseline-votes.log`).

Preservation tests and refactor:

- Before changing production code, the same build command exited 0, and `./src/test_nu --run_test=vote_tests/<case> --report_level=short` exited 0 individually for `park_rate_duration_validation`, `currency_coin_bases_preserve_votes`, and `currency_coin_bases_reject_zero_weight` (`preservation-build.log`, `before-<case>.log`).
- Characterization pins the existing post-v2 distinction: unknown currencies are accepted in blocks but not user votes. A preliminary test expectation was corrected before the refactor; the currency rules themselves were not changed.
- After refactoring, the build and full `vote_tests` command above both exited 0: 20 cases and 34,770 assertions passed (`refactor-build.log`, `refactor-votes.log`). The build log confirms recompilation of `main.cpp`, `bitcoinrpc.cpp`, and `vote.cpp`.
- The zero-weight case deliberately logs `ERROR: vote with 0 coin age destroyed`; its rejection assertion passes. Recompiling RPC code emits three deprecated-copy warnings from Boost iostreams internals, which are outside this refactor.

### Isolated Benchmark

Environment: Apple M3 Pro, Darwin arm64, Apple clang 21.0.0; C++17, `-O2`, no LTO. The temporary makefile reuses the project's flags/libraries and rebuilt production objects, excluding the daemon entry point and test entry point. Legacy bodies are extracted from the captured source, with only names/namespacing changed; the by-value vector parameter is retained. The legacy park-rate method is attached to a trivial derived class, letting both methods read the same base data.

Commands from the repository root (all exited 0):

```sh
EVIDENCE_DIR=/private/var/folders/9b/_93lfybj5992zvh8qn4ybgf00000gn/T/opencode/nubits-vote.ueyaE3
python3 "$EVIDENCE_DIR/extract-legacy.py"
make -C src -f "$EVIDENCE_DIR/benchmark.mk" vote-benchmark
"$EVIDENCE_DIR/vote-benchmark"
```

Before timing, assertions compare successful grant returns and byte-for-byte serialized transaction vectors at `SetMockTime(2000000000)`. Park-rate returns match for pre-v2, v2, and current protocols in both validation contexts. All equivalence assertions passed.

Each measurement uses three warm-up calls per implementation, then the median of 11 batches, alternating which implementation runs first. Inputs are constructed before timing; grant output vectors are reused. A volatile checksum consumes results. Grant batches use 2,000 / 200 / 20 calls for 100 / 1,000 / 10,000 votes; park-rate batches use 50,000 calls.

Each synthetic grant vote contains four custodian entries (two NuBits, two NuShares), one three-rate park vote, two motions, two fee entries, and nonzero varying coin age. The elected-custodian map is empty. Park inputs contain 8 or 30 distinct durations in permuted order; duplicate cases append the first duration with another rate.

| Workload | Size | Legacy median (µs/call) | Modern median (µs/call) | Legacy / modern |
| --- | ---: | ---: | ---: | ---: |
| Grants | 100 votes | 16.600 | 7.341 | 2.261× |
| Grants | 1,000 votes | 172.188 | 63.762 | 2.700× |
| Grants | 10,000 votes | 1,710.377 | 627.254 | 2.727× |
| Distinct park durations | 8 | 0.159 | 0.143 | 1.108× |
| Late duplicate duration | 8 + duplicate | 0.165 | 0.146 | 1.129× |
| Distinct park durations | 30 | 0.648 | 0.545 | 1.189× |
| Late duplicate duration | 30 + duplicate | 0.655 | 0.548 | 1.196× |

Evidence: `benchmark-environment.log`, `benchmark-build.log`, and `benchmark-results.csv`, ending with `equivalence=passed,checksum=2346586`. These are isolated, synthetic, warm-cache measurements on one machine, not a whole-node or synchronization speedup. Different vote contents, allocation behavior, hardware, or system load can change the ratios. The harness and logs remain temporary, outside the tracked product.

### Final Integration

- `make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu nud`: exit 0 (`final-build.log`). The rebuilt test executable remains current; the daemon entry point is recompiled and `src/nud` linked against the updated core objects.
- `./src/test_nu --report_level=short`: exit 0; 86/86 cases and 44,949/44,949 assertions passed (`final-tests.log`, lines 223–225). This is the single full-suite run against the final source.
- Coverage is local macOS arm64 compilation and unit tests. Linux, Intel macOS, GUI builds/playtesting, historical-chain replay, and live recovery-node synchronization were not performed. No daemon was started or remote CI invoked.

### Scope and Artifact Validation

- `openspec validate modernize-vote-processing --strict --no-interactive`: exit 0; the deliberate `skip_specs` declaration is accepted.
- `git diff --check`: exit 0. Checksums of all six pre-existing modified files match the baseline; their combined `git diff` also matches `pre-existing.patch` byte-for-byte.
- Commit scope is `src/vote.cpp`, `src/vote.h`, `src/test/vote_tests.cpp`, and this change's four planning/tracking artifacts. Temporary benchmarks, build outputs, unrelated changes, and `reduce-duplicate-input-search` remain outside the commit.
