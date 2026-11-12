Feature: The multitx option allows multiple transactions to be sent if only one would be too large
  Scenario: A small enough transaction
    Given a network with node "Alice" able to mint
    And a node "Bob" with an empty wallet
    When node "Alice" sets the split shares to "1" NSR without change split
    And node "Bob" generates a new address "bob"
    And node "Alice" sends "5,000" NSR to "bob"
    And node "Alice" finds a block received by all nodes
    Then node "Bob" should have 1 NSR transaction
    And node "Bob" should have a balance of "5,000" NSR

  Scenario: A too large transaction without the multitx option
    Given a network with node "Alice" able to mint
    And a node "Bob" with an empty wallet
    When node "Alice" sets the split shares to "1" NSR without change split
    And node "Bob" generates a new address "bob"
    Then node "Alice" should fail to send "10,000" NSR to "bob" because the transaction would be too big

  Scenario: A too large transaction with the multitx option
    Given a network with node "Alice" able to mint and with multitx enabled
    And a node "Bob" with an empty wallet
    When node "Alice" sets the split shares to "1" NSR without change split
    And node "Bob" generates a new address "bob"
    And node "Alice" sends "10,000" NSR to "bob"
    And node "Alice" finds a block received by all nodes
    Then node "Bob" should have 2 NSR transaction
    And node "Bob" should have a balance of "10,000" NSR

  Scenario: A very large transaction with the multitx option
    Given a network with node "Alice" able to mint and with multitx enabled
    And a node "Bob" with an empty wallet
    When node "Alice" sets the split shares to "1" NSR without change split
    And node "Bob" generates a new address "bob"
    And node "Alice" sends "100,000" NSR to "bob"
    And node "Alice" sets the split shares to "10,000" NSR without change split
    And node "Alice" finds 8 blocks received by all nodes
    Then node "Bob" should have 14 NSR transaction
    And node "Bob" should have a balance of "100,000" NSR

  Scenario: Send to many without the multitx option
    Given a network with node "Alice" able to mint
    And a node "Bob" with an empty wallet
    And a node "Charles" with an empty wallet
    When node "Alice" sets the split shares to "1" NSR without change split
    And node "Bob" generates a new address "bob"
    And node "Charles" generates a new address "charles"
    Then node "Alice" should fail to sends these NSR because the transaction is too big:
      | Address | Amount |
      | bob     |  8,000 |
      | charles |  2,000 |

  Scenario: Send to many with the multitx option
    Given a network with node "Alice" able to mint and with multitx enabled
    And a node "Bob" with an empty wallet
    And a node "Charles" with an empty wallet
    When node "Alice" sets the split shares to "1" NSR without change split
    And node "Bob" generates a new address "bob"
    And node "Charles" generates a new address "charles"
    And node "Alice" sends these NSR:
      | Address | Amount |
      | bob     |  8,000 |
      | charles |  2,000 |
    And node "Alice" finds a block received by all nodes

    Then node "Bob" should have 2 NSR transaction
    And the 1st transaction should be a receive of "4000" to "bob"
    And the 2nd transaction should be a receive of "4000" to "bob"
    And node "Bob" should have a balance of "8,000" NSR

    And node "Charles" should have 2 NSR transaction
    And the 1st transaction should be a receive of "1000" to "charles"
    And the 2nd transaction should be a receive of "1000" to "charles"
    And node "Charles" should have a balance of "2,000" NSR
