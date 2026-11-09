Feature: Nodes accept unknown currencies

  Scenario: An unknown currency is issued
    Given a network with node "Alice" able to mint
    And a node "Bob" able to mint and with a currency unknown to others

    When node "Bob" generates an UnknownCurrency address "cust"
    And node "Bob" votes an amount of "100,003" for custodian "cust"
    And node "Bob" finds enough blocks for a custodian to be elected
    Then all nodes should reach the same height

    When node "Bob" votes for an UnknownCurrency fee of 3.0
    And node "Bob" finds enough blocks for a fee vote to pass
    And node "Bob" finds enough blocks for the voted fee to be effective
    And node "Bob" generates an UnknownCurrency address "recipient"
    And node "Bob" sends "100" UnknownCurrency to "recipient" in transaction "tx"
    Then transaction "tx" on node "Alice" should have a fee of 3.0
    And all nodes should reach 1 transaction in memory pool

    When node "Alice" finds a block
    Then all nodes should reach 0 transactions in memory pool
    And node "Bob" should have a balance of "100,000" UnknownCurrency


    And node "Bob" votes a park rate of "0.00000001" UnknownCurrency for 32 blocks
    And node "Bob" finds enough blocks for her park rate vote to become the median park rate
    And node "Bob" resets his vote
    And node "Bob" finds enough blocks for the voted park rate to become effective
    Then the expected premium on node "Bob" for "50,000" UnknownCurrency parked for 32 blocks should be "0.0001"

    When node "Bob" parks "50,000" UnknownCurrency for 32 blocks with "cust" as unpark address
    Then all nodes should have 1 transaction in memory pool

    When node "Bob" finds 31 blocks
    And all nodes reach the same height
    And node "Bob" unparks UnknownCurrency
    And node "Bob" finds a block
    Then all nodes should reach the same height

    When node "Bob" unparks UnknownCurrency
    Then node "Bob" should have a balance of "99,999.9901" UnknownCurrency
    And "Bob" should have "0" UnknownCurrency parked

    When node "Bob" finds a block
    Then all nodes should reach the same height


    When node "Bob" sends a liquidity of "1000" buy and "2000" sell on unit "t" from address "cust" with identifier "1:"
    And node "Bob" sends a liquidity of "100" buy and "200" sell on unit "Y" from address "cust" with identifier "1:"
    Then node "Alice" should reach a total liquidity info of "1000" buy and "2000" sell on unit "t"
    And node "Alice" should reach a total liquidity info of "100" buy and "200" sell on unit "Y"
