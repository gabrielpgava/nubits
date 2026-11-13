# Network Recovery and Monetary Policy: Discussion Proposal

**Status: exploratory proposal, open to review and criticism.**

This document records a possible direction for recovering the Nu network. It is
not an approved specification, an implementation, or an announcement of an
activation date. The rules described below would require consensus changes and
a hard fork. Current software does not implement this proposal.

## 1. Context and objectives

The recovery effort currently has historical blockchain data but no known
usable private keys for historical NSR balances. An older node might return
with additional history; any such history must be verified before choosing a
recovery point. Inactivity does not prove that private keys are lost.

The immediate objectives are to:

- Resume block production without requiring access to historical NSR keys.
- Preserve historical balances and their ownership by the corresponding keys.
- Allow new participants to acquire NSR and contribute to network security.
- Give historical holders a functioning network to return to.
- Connect payment activity to compensation for maintaining the network.
- Allow monetary expansion and contraction under transparent rules.

Restoring a fiat peg is **not** an objective of this proposal. There is no
promise that NBT or any other network currency will maintain a fiat value,
exchange rate, redemption value, or investment return.

## 2. Proposed direction at a glance

| Component | Proposed direction |
| --- | --- |
| Normal block production | Proof of stake (PoS) |
| Recovery block production | Proof of work (PoW), permitted after prolonged PoS inactivity |
| New block subsidy | 5 NSR per valid block, whether PoS or PoW |
| Producer share of transaction fees | 75%, paid in the currency in which each fee was paid |
| Burned share of transaction fees | 25%, destroyed in that same currency |
| NSR-to-NBT issuance | Irreversibly destroy 5 NSR to create 1 NBT |
| Guaranteed reverse conversion | None |
| Fiat peg | None |
| Historical balances | Preserved; not confiscated for inactivity |

The fee recipients are **block producers performing staking or mining**, not
all holders and not wallets that merely hold coins. These are gross protocol
rewards; production does not guarantee a profit after costs or market changes.

## 3. PoS with a PoW recovery path

PoS would be the normal operating mode. PoW would provide a way to make progress
when PoS production stops, without requiring someone to recover old private
keys first. PoW and PoS blocks would follow the same proposed subsidy, fee split,
and monetary rules.

The initial idea was to enable PoW after "X blocks without PoS." If no blocks
are being produced, however, chain height cannot advance. The intended rule is
therefore an inactivity period equivalent to **X target block intervals**
without a valid PoS block, measured using a consensus-defined timing rule.

**X has not been selected.** A node's local wall clock alone cannot define a
universally valid chain rule. Timestamp constraints, permitted clock drift,
branch-relative activation, and treatment of the historical recovery point
must be specified and tested.

The following remain open:

- Initial PoW difficulty and adjustment when miners arrive or leave.
- When PoW becomes disabled after PoS returns, including resistance to repeated
  switching and deliberate withholding of PoS blocks.
- How competing PoW and PoS branches are scored and selected.
- Resistance to timestamp manipulation, reorganizations, and cheap attacks on
  a small network.
- Maturity and stake eligibility of newly mined NSR.
- A shared reward schedule that cannot reset when production mode changes.

Reported online stake, wallet counts, and address counts must not be assumed to
measure independently controlled, available staking power. The fallback is a
proposed liveness mechanism, not a demonstrated security guarantee.

## 4. Producer rewards and transaction fees

Each valid block would permit a **5 NSR subsidy**, replacing the current nominal
40 NSR PoS reward. The producer would additionally receive 75% of transaction
fees collected in the block, separately for every currency. The remaining 25%
would be burned, including for fees paid in NSR.

For example, a block containing 2 NSR and 4 NBT in transaction fees would pay:

- 5 newly issued NSR as the block subsidy.
- 1.5 existing NSR and 3 existing NBT as producer fee income.
- 0.5 NSR and 1 NBT destroyed as fee burns.

Fee payments transfer existing coins; they are not new issuance. Fees paid in
NBT must not authorize the creation of NSR or vice versa. Implementation would
need currency-specific payout accounting and validation, rather than treating
different assets as interchangeable values.

The exact integer rounding rule for the 75/25 split, the treatment of fees too
small to split, and prevention of rounding-based burn avoidance remain to be
specified. Minimum fees should protect network resources without making
ordinary payments unnecessarily expensive.

## 5. Creating NBT by destroying NSR

The proposed conversion is:

```text
Destroy 5 NSR permanently -> create 1 NBT
```

The NSR must actually leave the spendable supply. Sending them to a treasury,
locking them temporarily, or paying them to a producer is not equivalent.
Burn and issuance must be atomically validated by consensus so neither can
happen without the other. Transaction fees are separate from this conversion
amount; their exact application to a conversion must be specified.

The ratio is an **issuance rule**, not a market-price guarantee. There would be
no protocol guarantee to redeem 1 NBT for 5 NSR and no automatic reverse minting
of NSR. Fractional conversions, precision, and minimum conversion amounts need
explicit rules.

This document does not set conversion ratios for the other network currencies.
Their denominations and issuance rules require separate review; assigning the
same ratio to all of them would not establish their former fiat values.

Existing alternative issuance paths, including custodian grants and any other
applicable issuance or premium mechanisms, must be audited. Their treatment
must be explicit so they cannot silently bypass the intended monetary rules.

## 6. Market incentives and limitations

If selling 1 NBT yields more than the total cost of acquiring and destroying
5 NSR, including fees, slippage, and risks, market participants may find it
profitable to convert. This can create purchasing demand for NSR, burn NSR, and
increase NBT supply. Participation in that arbitrage would be permitted rather
than treated as abuse.

Competition may narrow the opportunity, but rapid correction is not guaranteed.
A recovering network may have little liquidity, limited exchange access, and
large differences between quoted and executable prices.

The mechanism is asymmetric: if NBT trades below its creation cost, conversion
can stop, but there is no guaranteed reverse arbitrage or redemption floor.
Existing NBT can continue to trade below that cost. Conversion demand must not
be assumed to exist merely because NSR is issued as a block reward.

The intended economic relationship is:

```text
NSR issuance -> compensation for block production
Demand for new NBT -> purchases and destruction of NSR
NBT usage -> producer fee income and destruction of some NBT
```

NSR serves security and governance, while NBT and other currencies are intended
for use and payments. Actual demand still requires useful services, reliable
transfers, usable wallets, and willing counterparties. Scarcity alone does not
create those conditions.

## 7. Monetary accounting and supply limits

Considering only the proposed rules, let:

- `B` be the number of blocks produced in a period, across both production modes.
- `C` be the quantity of NBT created by conversion in that period.
- `D_NSR` and `D_NBT` be the actual fee burns in each currency after rounding.

Then:

```text
Change in NSR supply = 5 * B - 5 * C - D_NSR
Change in NBT supply = C - D_NBT
```

One NBT created per block offsets the 5 NSR subsidy before NSR fee burns.
Greater conversion can contract NSR supply; lower conversion can allow it to
grow. NBT supply contracts when its burns exceed its issuance. Historical coins
with unavailable keys remain part of recorded supply; this proposal does not
pretend to know which keys are lost.

At an illustrative one-block-per-minute average, 5 NSR per block would create
approximately **2,628,000 NSR per 365-day year before burns**. This is an 87.5%
reduction from a 40 NSR subsidy per block, not a guaranteed annual inflation
rate or an absolute cap. A hybrid design must retain an overall production-rate
target rather than accidentally doubling issuance through parallel modes.

**No final hard supply cap or subsidy sunset has been agreed.** A permanent
5 NSR subsidy permits unbounded cumulative issuance if blocks continue; burns
can offset it but are not guaranteed to do so. A ceiling on outstanding supply,
a lifetime issuance budget, and a per-period issuance limit are different
policies and must not be conflated. If an outstanding-supply cap allows burns
to reopen issuance capacity, reissuance must still have a defined rate limit.

Different currencies cannot be added together as raw units to measure a single
monetary base. A 5 NSR burn and 1 NBT issuance do not establish an 80% reduction
in economic value. Supply growth also does not directly determine price
inflation, purchasing power, or market capitalization.

## 8. Risks and questions for review

- **Security funding:** are 5 NSR plus fee income enough to attract producers
  during periods with few transactions and little market value?
- **Stake availability:** can conversion burns and limited accessible historical
  balances leave too little usable NSR for a healthy PoS network?
- **Concentration:** how are rewards affected by large miners, large stakers,
  pooling, and the return of a historical wallet with substantial NSR?
- **Producer incentives:** how does the fee split affect self-transactions,
  censorship, fee manipulation, and attempts to reorganize fee-rich blocks?
- **Governance:** can a small active electorate authorize issuance that defeats
  the rules, and which safeguards should be enforced directly by consensus?
- **Asset liquidity:** do fees paid in multiple currencies provide useful
  compensation, or merely balances producers cannot spend or sell?
- **Recovery history:** which verified block anchors the fork, and how should
  later-discovered historical branches be evaluated before activation?
- **Compatibility:** what replay protection and client upgrade process are
  required? Old clients will not automatically accept the new rules.
- **Supply policy:** should there be an absolute cap, declining subsidy, or a
  bounded ongoing subsidy? How is security financed when issuance is limited?

No dormant balances should be confiscated on the assumption that their keys
are lost. No reward, conversion ratio, or fee split guarantees token value,
decentralization, continuous production, or sustainable security.

## 9. Evaluation before implementation or activation

Community review should challenge both the economic assumptions and the
consensus mechanism. Before activation, a concrete specification and test
network should cover at least:

1. Zero accessible historical stake and a completely stalled starting chain.
2. Loss and return of most PoS or PoW participants.
3. Timestamp manipulation, partitions, competing branches, and mode switching.
4. Conservation of value by currency, exact burns, and invalid payout rejection.
5. Atomic conversions, rounding, fractional amounts, and issuance bypasses.
6. Low activity, high conversion demand, weak liquidity, and falling demand.
7. Return of substantial historical balances and concentrated participation.
8. Long-run supply trajectories and producer revenue under multiple assumptions.

The intended next step is discussion and simulation, not immediate deployment.

## 10. Research references and their limits

These references inform the discussion; none validates the specific 5 NSR
subsidy, 5:1 conversion ratio, 75/25 fee split, or proposed PoW fallback.

- **Cong, Li, and Wang (2021), _Tokenomics: Dynamic Adoption and Valuation_,
  Review of Financial Studies.** A model connecting transactional demand,
  adoption, network effects, and token valuation. It does not imply that
  restricting supply creates utility or that this design will attract users.
  <https://www.financetheory.org/papers/tokenomics-dynamic-adoption-and-valuation>
- **Brunnermeier, James, and Landau (2021), _The digitalisation of money_,
  BIS Working Paper 941.** Discusses separating monetary functions and the role
  of digital networks. It is not proof that a two-token design is superior.
  <https://www.bis.org/publ/work941.htm>
- **Roughgarden (2020), _Transaction Fee Mechanism Design for the Ethereum
  Blockchain: An Economic Analysis of EIP-1559_.** Analyzes fee-mechanism
  incentives, including a burned base fee. A fixed percentage split is not
  equivalent to EIP-1559 and does not inherit its results automatically.
  <https://arxiv.org/abs/2012.00854>
- **Carlsten, Kalodner, Weinberg, and Narayanan (2016), _On the Instability of
  Bitcoin Without the Block Reward_.** Studies incentive risks of fee-dominated
  rewards in a Bitcoin/PoW setting. Its conclusions cannot be directly assumed
  to hold for Nu's PoS or a new hybrid consensus.
  <https://www.cs.princeton.edu/~arvindn/publications/mining_CCS.pdf>

Feedback is welcome on the
[NuBits community forum](https://discuss.nubits.com/) or through this
repository's discussion and review channels. Please distinguish proposed
economic goals from consensus rules that have actually been specified and
tested.
