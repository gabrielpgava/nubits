Feature: The balance is correctly reported when shares have been used to mint

  Scenario: A shareholder finds a block and gets the balance
    Given a network with nodes "Alice" and "Bob"
    And the NSR balance of node "Alice" is noted

    When node "Alice" finds a block received by all other nodes
    And node "Bob" finds enough blocks to mature a Proof of Stake block
    Then the NSR balance of node "Alice" should have increased by "40"


