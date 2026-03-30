function handle(m)
   m:insert_header('X-Header1', 'Foo')
   m:insert_header('X-Header2', 'Bar')
   return m:exec_raw(os.getenv('QRELAY_RECORD_PROGRAM'), os.getenv('QRELAY_RECORD_FILE'), m.sender, unpack(m.recipients))
end

qmqp_listen(os.getenv('QRELAY_SOCKET_PATH'), handle)
