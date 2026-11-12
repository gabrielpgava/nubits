Feature: Shareholders can vote for the minimum transaction fee of each unit
  Scenario: NSR fee changed by vote
    Given a network with nodes "Alice" and "Bob" able to mint
    And a node "Charlie" with an empty wallet
    When node "Alice" votes for an NSR fee of 0.2
    And node "Alice" finds enough blocks for a fee vote to pass
    And node "Alice" finds enough blocks for the voted fee to be effective
    Then the NSR fee should be 0.2
    When all nodes reach the same height
    And node "Charlie" generates an NSR address "charlie"
    And node "Bob" sends "3,000" NSR to "charlie" in transaction "tx"
    Then transaction "tx" on node "Charlie" should have a fee of 0.2
    And all nodes should reach 1 transaction in memory pool
    When node "Alice" finds a block
    Then node "Charlie" should have a balance of "3,000" NSR

  Scenario Outline: Currency fee changed by vote
    Given a network with nodes "Alice" and "Bob" able to mint
    And a node "Charlie" with an empty wallet
    Then the <currency> fee should be 0.01

    When node "Bob" generates a <currency> address "cust"
    And node "Alice" votes an amount of "1,000,000" for custodian "cust"
    And node "Alice" finds blocks until custodian "cust" is elected
    And all nodes reach the same height
    And node "Alice" votes for a <currency> fee of 0.05
    And node "Alice" finds enough blocks for a fee vote to pass
    And node "Alice" finds enough blocks for the voted fee to be effective
    Then the <currency> fee should be 0.05

    When all nodes reach the same height
    And node "Charlie" generates a <currency> address "charlie"
    And node "Bob" sends "1" <currency> to "charlie" in transaction "tx"
    Then transaction "tx" on node "Charlie" should have a fee of 0.05
    And all nodes should reach 1 transaction in memory pool

    When node "Alice" finds a block
    Then node "Charlie" should have a balance of "1" <currency>
    And node "Bob" should have a balance of "999,998.95" <currency>

    Examples:
      | currency |
      | US-NBT   |
      | CN-NBT   |
      | EU-NBT   |
      | X-NBT    |
