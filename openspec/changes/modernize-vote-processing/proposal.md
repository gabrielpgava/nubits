# Proposal

## Why

The project already targets C++17, but vote processing still copies a complete vote window, performs redundant set lookups, and uses iteration macros that obscure container entries. A focused refactor can remove this work and improve readability without changing monetary or validation rules.

## What Changes

- Borrow the vote vector in `GenerateCurrencyCoinBases` through a const reference instead of copying votes and their nested containers.
- Use native range-based loops and C++17 structured bindings in that function, retaining the existing iteration and grant-output order.
- Detect duplicate durations in `CParkRateVote::IsValid` from the result of a single set insertion, after the existing rate validation.
- Extend focused regression coverage, build locally, and measure the isolated changes without claiming a whole-node speedup.
- Preserve consensus rules, arithmetic, error paths, serialized fields, and existing caller behavior.

## Capabilities

### New Capabilities

None. This refactor adds no externally observable behavior.

### Modified Capabilities

None. `skip_specs: true` records that existing behavior is unchanged; preservation scenarios belong in the design and regression tests.

## Impact

Production edits are limited to `src/vote.cpp` and the corresponding declaration in `src/vote.h`; tests belong in `src/test/vote_tests.cpp`. Callers in `src/main.cpp` and `src/bitcoinrpc.cpp` keep their source-level calls, but must be rebuilt because the C++ function signature changes. No dependency, language-standard, database, or protocol upgrade is needed. Existing uncommitted transaction-validation, build-script, makefile, and OpenSpec changes are outside this change.
