# Proposal

## Why

`CTransaction::CheckTransaction()` currently performs a `std::set` membership lookup and then a second lookup to insert every distinct transaction input. Using the insertion result removes the redundant search while preserving the existing duplicate-input decision and validation order.

## What Changes

- Replace the separate duplicate `count()` and `insert()` calls with one `insert(...).second` check.
- Use C++ range-based iteration for the input vector.
- Add focused regression coverage for distinct and duplicate inputs, including unchanged DoS scoring.
- Measure the isolated lookup change and run the existing transaction tests, full unit suite, build, and authorized historical synchronization check.
- Keep the set type and comparator, input order, rejection result, consensus rules, peer protocol, and serialized formats unchanged.

The owner revised historical verification on 2026-11-17 to testing the available
local data rather than requiring full-network synchronization. Partial replay
and an offline database reopen establish only compatibility with that prefix.

## Capabilities

### New Capabilities

None. This is an internal optimization and does not add observable behavior.

### Modified Capabilities

None. Transaction validity and peer-facing behavior remain unchanged. This change sets `skip_specs: true` because it changes only the implementation of an existing validation rule.

## Impact

The production edit is limited to the duplicate-input loop in `src/main.cpp`; regression coverage belongs in `src/test/transaction_tests.cpp`. The path runs during both mempool acceptance and block validation, so tests must preserve the result and `nDoS` value for duplicate inputs. A local benchmark will measure the removed tree lookup; an authorized replay against the recovery VM will check historical synchronization. No API, wire, database, or consensus-rule format changes are in scope.
