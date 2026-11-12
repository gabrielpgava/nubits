Feature: Ability to burn coins by destroying them in fees

  Scenario: Burn some NSR
    Given a network with nodes "Alice"
    And a node "Bob"

    When node "Bob" generates a new address "bob"
    And node "Alice" sends "100" NSR to "bob"
    And node "Alice" sends "50" NSR to "bob"
    And node "Alice" finds a block received by all other nodes
    Then node "Bob" should reach a balance of "150" NSR

    When node "Bob" burns "125" NSR
    Then node "Bob" should reach an unspent amount of "25" on address "bob" in 1 output
    And node "Alice" should reach 1 transaction in memory pool
    And the last NSR transaction on node "Bob" should have a fee of "125"

    When node "Alice" finds a block received by all other nodes
    Then node "Bob" should reach a balance of "25" NSR

  Scenario: Burn from a specific address
    Given a network with nodes "Alice"
    And a node "Bob"

    When node "Bob" generates a new address "burn address"
    When node "Bob" generates a new address "other address"
    And node "Alice" sends "50" NSR to "other address"
    And node "Alice" sends "100" NSR to "burn address"
    And node "Alice" finds a block received by all other nodes
    Then node "Bob" should reach a balance of "150" NSR

    When node "Bob" burns "30" NSR on address "burn address"
    Then node "Bob" should reach an unspent amount of "70" on address "burn address" in 1 output
    And node "Alice" should reach 1 transaction in memory pool
    And the last NSR transaction on node "Bob" should have a fee of "30"

    When node "Alice" finds a block received by all other nodes
    Then node "Bob" should reach a balance of "120" NSR
