Then(/^node "(.*?)" should fail to send "(.*?)" (\S+) to "(.*?)" because the transaction would be too big$/) do |arg1, arg2, unit_name, arg3|
  expect {
    @nodes[arg1].unit_rpc unit(unit_name), "sendtoaddress", @addresses.fetch(arg3, arg3), parse_number(arg2)
  }.to raise_error(RuntimeError, /The generated transaction is too big/)
end

