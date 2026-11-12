When(/^node "(.*?)" blocks the address "(.*?)" (\d+) blocks? after block "(.*?)"$/) do |arg1, arg2, arg3, arg4|
  node = @nodes[arg1]
  address = @addresses[arg2]
  distance = arg3.to_i
  block = node.rpc("getblock", @blocks[arg4])
  height = block["height"] + distance
  node.rpc("blockaddress", height, address)
end

