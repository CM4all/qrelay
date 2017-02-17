qmqp_listen(systemd, function(m)
  return m:connect('localhost')
end)
