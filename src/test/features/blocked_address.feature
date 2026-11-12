Feature: An address is blocked

  Scenario: Sending from a blocked address before limit block
    Given a network with node "Alice" and "Bob" able to mint
    And a node "Charlie"
    And node "Charlie" generates a NSR address "charlie"
    When node "Alice" sends "1000" NSR to "charlie"
    And node "Alice" finds a block
    And node "Alice" finds a block "A"
    And all nodes reach block "A"
    Then node "Charlie" should reach a balance of "1000" NSR

    When node "Alice" blocks the address "charlie" 2 blocks after block "A"
    And node "Charlie" sends "500" NSR to node "Alice"
    Then node "Alice" should reach 1 transaction in memory pool
    And node "Bob" should reach 1 transaction in memory pool

    When node "Bob" finds a block "B"
    Then node "Alice" reach block "A"

  Scenario: Sending from a blocked address just before limit block
    Given a network with node "Alice" and "Bob" able to mint
    And a node "Charlie"
    And node "Charlie" generates a NSR address "charlie"
    When node "Alice" sends "1000" NSR to "charlie"
    And node "Alice" finds a block
    And node "Alice" finds a block "A"
    And all nodes reach block "A"
    Then node "Charlie" should reach a balance of "1000" NSR

    When node "Alice" blocks the address "charlie" 1 block after block "A"
    And node "Charlie" sends "500" NSR to node "Alice"
    Then node "Alice" should reach 1 transactions in memory pool
    Then node "Bob" should reach 1 transaction in memory pool

    When node "Bob" finds a block "B"
    Then node "Alice" should stay at block "A"

    When node "Alice" finds a block "C"
    And node "Alice" finds a block "D"
    Then node "Bob" should reach block "D"

  Scenario: Sending from a blocked address after limit block
    Given a network with node "Alice" and "Bob" able to mint
    And a node "Charlie"
    And node "Charlie" generates a NSR address "charlie"
    When node "Alice" sends "1000" NSR to "charlie"
    And node "Alice" finds a block
    And node "Alice" finds a block "A"
    And all nodes reach block "A"
    Then node "Charlie" should reach a balance of "1000" NSR

    When node "Alice" blocks the address "charlie" 0 blocks after block "A"
    And node "Charlie" sends "500" NSR to node "Alice"
    Then node "Alice" should stay at 0 transactions in memory pool
    Then node "Bob" should reach 1 transaction in memory pool

    When node "Bob" finds a block "B"
    Then node "Alice" should stay at block "A"
