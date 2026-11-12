When(/^node "(.*?)" finds enough blocks for her park rate vote to become the median park rate$/) do |arg1|
  step "node \"#{arg1}\" finds 3 blocks received by all nodes"
end

When(/^node "(.*?)" finds enough blocks for the voted park rate to become effective$/) do |arg1|
  node = @nodes[arg1]
  protocol = node.info["protocolversion"]
  if protocol.to_f >= 2
    step "node \"#{arg1}\" finds 12 blocks received by all nodes"
  end
end

Then(/^the expected premium on node "(.*?)" for "(.*?)" (\S+) parked for (\d+) blocks should be "(.*?)"$/) do |arg1, arg2, unit_name, arg3, arg4|
  node = @nodes[arg1]
  amount = parse_number(arg2)
  blocks = arg3.to_i
  expected_premium = parse_number(arg4)

  actual_premium = parse_number(node.unit_rpc(unit(unit_name), 'getpremium', amount, blocks))
  begin
    expect(actual_premium).to eq(expected_premium)
  rescue Exception => e
    p node.unit_rpc(unit(unit_name), 'getparkvotes', node.rpc('getblockcount') - 12, 5)
    begin
      p node.unit_rpc(unit(unit_name), 'getparkrates')
    rescue
    end
    raise e
  end
end
