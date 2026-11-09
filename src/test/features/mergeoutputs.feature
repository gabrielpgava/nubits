Feature: An RPC commande `mergeoutputs` will merge multiple unspent outputs of the same address into a single output.
  Scenario: mergeoutputs applied to an address with 2 outputs
    Given a network with node "Alice" able to mint
    And a node "Bob" with an empty wallet
    When node "Bob" generates a new address "bob"
    And node "Alice" sends "4,000" NSR to "bob"
    And node "Alice" sends "1,000" NSR to "bob"
    And node "Alice" finds a block received by all nodes
    Then node "Bob" should have 2 NSR transaction
    And node "Bob" should have a balance of "5,000" NSR
    And address "bob" on node "Bob" should have 2 unspent outputs

    When node "Bob" merges its outputs
    Then node "Bob" should have 4 NSR transaction

    When node "Alice" reaches 1 transaction in memory pool
    And node "Alice" finds a block received by all nodes
    Then node "Bob" should have a balance of "4,999" NSR
    And address "bob" on node "Bob" should have 1 unspent output

  Scenario: mergeoutputs applied to an address with 1 output
    Given a network with node "Alice" able to mint
    And a node "Bob" with an empty wallet
    When node "Bob" generates a new address "bob"
    And node "Alice" sends "1,000" NSR to "bob"
    And node "Alice" finds a block received by all nodes
    Then node "Bob" should have 1 NSR transaction
    And node "Bob" should have a balance of "1,000" NSR
    And address "bob" on node "Bob" should have 1 unspent outputs

    When node "Bob" merges its outputs
    Then node "Bob" should have 1 NSR transaction
    And node "Bob" should have a balance of "1000" NSR

  Scenario: mergeoutputs of a specific address
    Given a network with node "Alice" able to mint
    And a node "Bob" with an empty wallet
    When node "Bob" generates a new address "bob"
    And node "Bob" generates a new address "bob2"
    And node "Alice" sends "1,000" NSR to "bob"
    And node "Alice" sends "1,000" NSR to "bob"
    And node "Alice" sends "1,000" NSR to "bob2"
    And node "Alice" sends "1,000" NSR to "bob2"
    And node "Alice" finds a block received by all nodes
    Then address "bob" on node "Bob" should have 2 unspent outputs
    And address "bob2" on node "Bob" should have 2 unspent outputs

    When node "Bob" merges outputs on address "bob"
    Then address "bob" on node "Bob" should have 1 unspent outputs
    And address "bob2" on node "Bob" should have 2 unspent outputs

  Scenario: mergeoutputs when shares are splitted
    Given a network with node "Alice" able to mint
    And a node "Bob" with an empty wallet
    When node "Bob" generates a new address "bob"
    And node "Alice" sends "4,000" NSR to "bob"
    And node "Alice" sends "1,000" NSR to "bob"
    And node "Alice" finds a block received by all nodes
    And node "Bob" sets the split shares to "500" NSR
    And node "Bob" merges its outputs
    Then address "bob" on node "Bob" should have 1 unspent output

  Scenario: mergeoutputs on a large number of outputs
    Given a network with node "Alice" able to mint and with multitx enabled
    And a node "Bob" with an empty wallet
    When node "Alice" sets the split shares to "10" NSR without change split
    And node "Bob" generates a new address "bob"
    And node "Alice" sends "100,000" NSR to "bob"
    And node "Alice" finds 5 blocks received by all nodes
    And node "Bob" reaches a balance of "100,000" NSR
    And node "Bob" merges its outputs with a long timeout
    Then address "bob" on node "Bob" should have 7 unspent output

  Scenario: mergeoutputs on locked shares
    Given a network with node "Alice" and "Bob" able to mint
    And node "Alice" finds 2 block received by all nodes
    Then the default address on node "Alice" should have 1000 unspent output
    When node "Alice" merges its outputs
    Then the default address on node "Alice" should have 3 unspent output
