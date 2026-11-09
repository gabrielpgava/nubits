Given(/^a node "(.*?)" able to mint and with a currency unknown to others$/) do |arg1|
  name = arg1
  options = {
    name: name,
    image: "nunet#{@network_suffix}/e",
    links: @nodes.values.map(&:name),
    args: {
      debug: true,
      timetravel: timeshift,
      unknowncurrency: true,
    },
  }
  node = CoinContainer.new(options)
  @nodes[name] = node
  node.wait_for_boot
end
