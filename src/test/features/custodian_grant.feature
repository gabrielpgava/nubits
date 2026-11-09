Feature: New currencies can be issued by vote

  Scenario Outline: An amount is granted
    Given a network at protocol <protocol> with nodes "Alice" and "Bob" able to mint

    When node "Bob" generates a <currency> address "cust"
    And node "Alice" votes an amount of "1,000" for custodian "cust"
    And node "Alice" finds enough blocks for a custodian to be elected
    Then node "Bob" should reach a balance of "<expected balance>" <currency>

    Examples:
      | protocol | currency | expected balance |
      | 2.0      | US-NBT   | 1,000            |
      | 2.0      | CN-NBT   | 0                |
      | 3        | CN-NBT   | 1,000            |
