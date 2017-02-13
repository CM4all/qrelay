qrelay.listen('/var/run/cm4all/qrelay.sock', function(m)
  return m:connect('localhost')
end
