When(/^node "(.*?)" burns "(.*?)" (\w+)$/) do |arg1, arg2, unit_name|
  @nodes[arg1].rpc("burn", parse_number(arg2), unit(unit_name))
end

When(/^node "(.*?)" burns "(.*?)" (\w+) on address "(.*?)"$/) do |arg1, arg2, unit_name, arg3|
  @nodes[arg1].rpc("burn", parse_number(arg2), unit(unit_name), "", @addresses[arg3])
end

Then(/^the last (\w+) transaction on node "(.*?)" should have a fee of "(.*?)"$/) do |unit_name, arg1, arg2|
  node = @nodes[arg1]
  unit = unit(unit_name)

  transactions = node.unit_rpc(unit, "listtransactions", "*")
  expect(transactions).not_to be_empty

  transaction = transactions.last
  begin
    expect(-transaction["fee"]).to eq(parse_number(arg2))
  rescue Exception
    require 'pp'
    pp transaction
    raise
  end
end
