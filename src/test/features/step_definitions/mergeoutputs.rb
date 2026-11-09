Then(/^address "(.*?)" on node "(.*?)" should have (\d+) unspent outputs?$/) do |arg1, arg2, arg3|
  address = @addresses.fetch(arg1, arg1)
  node = @nodes.fetch(arg2)
  expected = arg3.to_i
  unspent = nil
  begin
    wait_for do
      unspent = node.rpc("listunspent", 0)
      unspent_on_address = unspent.select { |u| u["address"] == address }
      expect(unspent_on_address.size).to eq(expected)
    end
  rescue RSpec::Expectations::ExpectationNotMetError
    require 'pp'
    pp @addresses
    pp unspent
    pp node.rpc("listtransactions")
    raise
  end
end

Then(/^the default address on node "(.*?)" should have (\d+) unspent output$/) do |arg1, arg2|
  address = @nodes[arg1].rpc("getaddressesbyaccount", "").last
  step %Q[address "#{address}" on node "#{arg1}" should have #{arg2} unspent outputs]
end

When(/^node "(.*?)" merges its outputs$/) do |arg1|
  @nodes.fetch(arg1).rpc("mergeoutputs")
end

When(/^node "(.*?)" merges its outputs with a long timeout$/) do |arg1|
  @nodes.fetch(arg1).long_unit_rpc('S', "mergeoutputs")
end

When(/^node "(.*?)" merges outputs on address "(.*?)"$/) do |arg1, arg2|
  @nodes.fetch(arg1).rpc("mergeoutputs", @addresses.fetch(arg2, arg2))
end
