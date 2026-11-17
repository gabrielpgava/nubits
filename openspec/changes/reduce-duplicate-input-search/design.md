# Design

## Context

See proposal.md for motivation. `CTransaction::CheckTransaction()` stores input outpoints in a local `set<COutPoint>`, currently calling `count()` and then `insert()` for each input. `COutPoint` ordering defines equality by transaction hash and output index. The project compiles as C++17.

## Goals / Non-Goals

**Goals:**

- Visit `vin` in its existing order with a range-based `for`.
- Detect duplicates from the boolean result of one set insertion.
- Preserve the set's comparator, first-duplicate rejection, return value, `nDoS`, and all checks surrounding the loop.
- Measure lookup-count and elapsed-time changes without adding a production dependency.

**Non-Goals:**

- Replace the ordered set, alter input ordering, or change transaction contents.
- Change consensus rules, block or mempool acceptance, peer penalties, wire encoding, or database formats.
- Modernize unrelated Boost iteration macros or other validation code.

## Decisions

1. **Use `insert(...).second` as the duplicate test.** A successful insertion means the outpoint was absent and validation continues. A failed insertion means it was already present and the function returns `false`, as it does today. This avoids the separate tree search while keeping the same container and ordering relation. Replacing the set with a hash container was rejected because it changes collision and resource behavior and is unnecessary for this optimization.
2. **Use a C++ range-based loop with `const CTxIn&`.** `vin` is a vector, so range iteration preserves its existing element order and avoids the legacy `BOOST_FOREACH` wrapper without copying inputs.
3. **Keep rejection side effects unchanged.** Duplicate detection remains a plain `return false`; it must not call `DoS()` or alter the order of neighboring validation checks.
4. **Measure the mechanism separately from application timing.** Compare the old and new loops on identical outpoint sequences, record comparator counts and elapsed time, and report timing even if it is inconclusive. Do not infer a whole-chain synchronization speedup from the local lookup reduction.
5. **Verify network compatibility with historical replay.** Use the authorized recovery peer after the local build and tests. Compare the synchronized chain height and block hashes with the available reference; report any unavailable history or replay limitation rather than claiming end-to-end equivalence.

## Risks / Trade-offs

- **[Consensus acceptance or penalty changes]** → Keep the comparator, input order, plain `false` return, and surrounding checks unchanged; test distinct and duplicate inputs and compare `nDoS`.
- **[Historical blocks fail to synchronize]** → Replay against the authorized recovery peer and compare accepted heights and hashes before claiming sync compatibility.
- **[Microbenchmark overstates real workload gains]** → Report operation counts separately from elapsed time and make no percentage or end-to-end throughput claim unless measurements support it.

## Migration Plan

No persistent data migration is required. Build and run the focused transaction test and full local unit suite, measure the isolated loop, and replay against the authorized recovery peer. If behavior, penalties, or historical chain results differ, revert the loop change and retain the baseline.

## Measured Benchmark

A standalone C++17 `-O2` microbenchmark used real `COutPoint` values and a counting comparator that delegates ordering to `COutPoint::operator<`. The legacy and single-insertion loops ran on identical distinct and duplicate sequences; each result reports comparator calls per run and the median of five elapsed-time samples, with 40 repetitions per sample.

| Workload | Legacy comparisons/run | Single-insertion comparisons/run | Legacy median (ns/run) | New median (ns/run) | Ratio |
| --- | ---: | ---: | ---: | ---: | ---: |
| 16 distinct inputs | 236 | 118 | 1,130 | 745 | 1.517x |
| 256 distinct inputs | 10,892 | 5,446 | 41,462 | 24,035 | 1.725x |
| 1,024 distinct inputs | 59,564 | 29,782 | 223,671 | 126,362 | 1.770x |
| Duplicate at input 64 | 1,738 | 872 | 6,876 | 4,182 | 1.644x |
| Duplicate at input 512 | 25,701 | 12,855 | 95,622 | 55,701 | 1.717x |
| Duplicate at input 1,023 | 59,502 | 29,756 | 226,801 | 128,062 | 1.771x |

The counts confirm that the separate membership lookup is removed for these workloads. The elapsed-time ratios apply only to this isolated microbenchmark; they do not establish a transaction-validation throughput or whole-chain synchronization speedup.

## Local Verification (2026-11-17)

- Rebuilt `nud` locally from the current checkout using the macOS makefile with output paths redirected into an isolated scratch directory: exit status 0. This includes the pending `autoupdate.o` linkage fix.
- Reused the full suite result from the preceding warning-cleanup verification, which built the same core sources and pending transaction regression: 86/86 cases and 44,964 assertions passed, exit status 0. No core sources changed afterward.
- Re-ran `test_nu --run_test=transaction_tests/basic_transaction_tests --report_level=short`: 1/1 selected case and 4/4 assertions passed, exit status 0.
- `bash -n build_gui_mac.sh build_mac.sh run_localnet.sh`, `git diff --check`, and `openspec validate reduce-duplicate-input-search --strict --no-interactive` passed.
- Historical replay remains a separate acceptance check; local unit tests and a successful TCP connection alone do not satisfy task 3.1.

### Historical Replay Progress

The isolated rebuilt node established a protocol connection exclusively to
`188.68.55.35:7890`, with DNS seeds, discovery, listening, staking, mining, and
automatic updates disabled. The peer advertised height 1,858,590 through the
legacy `getpeerinfo` field `height` (not `startingheight`), protocol version
3,000,000. RPC monitoring recorded 180,910 accepted local blocks before the
long-running execution became unavailable. Its termination cause was not
established. The isolated database was retained for resumption in bounded
segments; task 3.1 remains incomplete until the advertised height is reached
and the corresponding local block hash is recorded.

### Owner-Approved Verification Scope and Result

On 2026-11-17 the owner explicitly replaced full-network synchronization with
testing the data already available. This supersedes the full-height acceptance
criterion above without changing consensus or skipping production validation.

The rebuilt node reopened the retained isolated database with `-connect=0`,
`-dnsseed=0`, discovery/listening/mining/staking/updates disabled and no connected
peers. RPC reported height **276,448**, genesis hash
`000003cc2da5a0a289ad0a590c20a8b975219ddc1204efd169e947dd4cbad73f`,
and tip hash
`425dc848075f81d0ed7883cc23d0612b545b28e7fc6e4a6d015ae21c124413e5`.
The offline verification ran from 17:51:53 to 17:52:04 UTC and shutdown returned
exit status 0. Evidence is retained in the local `nubits-warnings.0johrG`
verification directory as `offline-result.json` and `offline-node.log`.

This verifies partial historical replay and an offline reopen, not a fresh
revalidation of every stored block or synchronization to the peer's advertised
height 1,858,590. No full-chain equivalence or synchronization speedup is claimed.
