Feature: Sending liquidity info with an identifier

  Scenario: Liquidity info with indentifier
    Given a network with nodes "Alice" and "Custodian" able to mint
    When node "Custodian" generates a NuBit address "cust"
    And node "Alice" votes an amount of "1,000,000" for custodian "cust"
    And node "Alice" finds blocks until custodian "cust" is elected
    And all nodes reach the same height
    And node "Custodian" sends a liquidity of "1000" buy and "2000" sell on unit "B" from address "cust" with identifier "1:"
    Then node "Alice" should reach a total liquidity info of "1000" buy and "2000" sell on unit "B"
    And 1 second passes
    And node "Custodian" sends a liquidity of "10" buy and "5" sell on unit "B" from address "cust" with identifier "2:"
    Then node "Alice" should reach a total liquidity info of "1010" buy and "2005" sell on unit "B"

    When node "Custodian" restarts
    And 1 second passes
    And node "Custodian" sends a liquidity of "1" buy and "2" sell on unit "B" from address "cust" with identifier "1:"
    Then node "Alice" should reach a total liquidity info of "11" buy and "7" sell on unit "B"

  Scenario: Liquidity info on all currencies
    Given a network with nodes "Alice" and "Custodian" able to mint
    When node "Custodian" generates a US-NBT address "us"
    And node "Custodian" generates a CN-NBT address "cn"
    And node "Custodian" generates a EU-NBT address "eu"
    And node "Custodian" generates a X-NBT address "x"
    And node "Alice" votes an amount of "1,000,000" for custodian "us"
    And node "Alice" votes an amount of "1,000,000" for custodian "cn"
    And node "Alice" votes an amount of "1,000,000" for custodian "eu"
    And node "Alice" votes an amount of "1,000,000" for custodian "x"
    And node "Alice" finds blocks until custodian "us" is elected
    And all nodes reach the same height
    And node "Custodian" sends a liquidity of "1000" buy and "2000" sell on unit "B" from address "us" with identifier "1:"
    And node "Custodian" sends a liquidity of "50" buy and "80" sell on unit "Y" from address "cn" with identifier "1:"
    Then node "Alice" should reach a total liquidity info of "1000" buy and "2000" sell on unit "B"
    And node "Alice" should reach a total liquidity info of "50" buy and "80" sell on unit "Y"
    And 1 second passes
    And node "Custodian" sends a liquidity of "10" buy and "5" sell on unit "E" from address "eu" with identifier "2:"
    And node "Custodian" sends a liquidity of "999" buy and "789" sell on unit "X" from address "x" with identifier "2:"
    And node "Custodian" sends a liquidity of "123" buy and "0" sell on unit "Y" from address "cn" with identifier "2:"
    And node "Custodian" sends a liquidity of "100" buy and "666" sell on unit "Y" from address "cn" with identifier "1:"
    Then node "Alice" should reach a total liquidity info of "1000" buy and "2000" sell on unit "B"
    And node "Alice" should reach a total liquidity info of "223" buy and "666" sell on unit "Y"
    And node "Alice" should reach a total liquidity info of "10" buy and "5" sell on unit "E"
    And node "Alice" should reach a total liquidity info of "999" buy and "789" sell on unit "X"

  Scenario Outline: Custodian can send liquidity info in any currency
    Given a network with nodes "Alice" and "Custodian" able to mint
    When node "Custodian" generates a <grant currency> address "cust"
    And node "Alice" votes an amount of "1,000,000" for custodian "cust"
    And node "Alice" finds blocks until custodian "cust" is elected
    And all nodes reach the same height
    And node "Custodian" sends a liquidity of "1000" buy and "2000" sell on unit "<liquidity unit>" from address "cust" with identifier "1:"
    Then node "Alice" should reach a total liquidity info of "1000" buy and "2000" sell on unit "<liquidity unit>"

    Examples:
      | grant currency | liquidity unit |
      | NSR            | Y              |
      | US-NBT         | X              |
      | X-NBT          | E              |
      | EU-NBT         | B              |

  Scenario: Liquidity info in different currencies do not overlap
    Given a network with nodes "Alice" and "Custodian" able to mint
    When node "Custodian" generates a CN-NBT address "cust"
    And node "Alice" votes an amount of "1,000,000" for custodian "cust"
    And node "Alice" finds blocks until custodian "cust" is elected
    And all nodes reach the same height
    And node "Custodian" sends a liquidity of "1000" buy and "2000" sell on unit "B" from address "cust" with identifier "1:"
    And node "Custodian" sends a liquidity of "100" buy and "200" sell on unit "E" from address "cust" with identifier "1:"
    Then node "Alice" should reach a total liquidity info of "1000" buy and "2000" sell on unit "B"
    And node "Alice" should reach a total liquidity info of "100" buy and "200" sell on unit "E"

  Scenario: Sending invalid liquidity info
    Given a network with nodes "Alice" and "Custodian" able to mint
    When node "Custodian" generates a NuBit address "cust"
    And node "Alice" votes an amount of "1,000,000" for custodian "cust"
    And node "Alice" finds blocks until custodian "cust" is elected
    And all nodes reach the same height
    Then sending from node "Custodian" a liquidity of "100" buy and "200" sell on unit "B" from address "cust" with identifier "✓" should fail

  Scenario: Sending too many liquidity info
    Given a network with nodes "Alice" and "Custodian" able to mint
    And the maximum identifier per custodian is 5
    When node "Custodian" generates a NuBit address "cust"
    And node "Alice" votes an amount of "1,000,000" for custodian "cust"
    And node "Alice" finds blocks until custodian "cust" is elected
    And all nodes reach the same height
    And node "Custodian" sends these liquidity info from address "cust" on unit "B" at 1 second interval:
      | Buy | Sell | Identifier |
      |   1 |    1 | 1          |
      |   1 |    1 | 2          |
      |   1 |    1 | 3          |
      |   1 |    1 | 4          |
      |   1 |    1 | 5          |
    Then node "Alice" should have these liquidity info from custodian "cust" on unit "B":
      | Buy | Sell | Identifier |
      |   1 |    1 | 1          |
      |   1 |    1 | 2          |
      |   1 |    1 | 3          |
      |   1 |    1 | 4          |
      |   1 |    1 | 5          |
    And node "Custodian" sends these liquidity info from address "cust" on unit "B" at 1 second interval:
      | Buy | Sell | Identifier |
      |  10 |    1 | 1          |
      |   1 |    1 | 6          |
    Then node "Alice" should have these liquidity info from custodian "cust" on unit "B":
      | Buy | Sell | Identifier |
      |  10 |    1 | 1          |
      |   1 |    1 | 3          |
      |   1 |    1 | 4          |
      |   1 |    1 | 5          |
      |   1 |    1 | 6          |
