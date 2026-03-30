#!/usr/bin/env python3

import os
import sys
import signal
import shutil
import socket
import subprocess
import time
from typing import Iterable

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} BUILD_DIR", file=sys.stderr)
    sys.exit(1)

build_directory = sys.argv[1]

test_directory = os.path.dirname(__file__)
config_directory = os.path.join(test_directory, 'config')
runtime_directory = os.path.join(build_directory, 'run')
socket_path = os.path.join(runtime_directory, 'qrelay.socket')
record_file = os.path.join(runtime_directory, 'record')

def make_netstring(payload: bytes) -> bytes:
    return str(len(payload)).encode('ascii') + b':' + payload + b','

def encode_qmqp(message: bytes, sender: bytes, recipients: Iterable[bytes]) -> bytes:
    return make_netstring(make_netstring(message) + make_netstring(sender) +
                          b''.join(map(lambda r: make_netstring(r), recipients)))

def submit_to_qrelay(socket_path: str, request: bytes) -> bytes:
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(socket_path)
    s.sendall(request)
    response = b''
    while True:
        chunk = s.recv(65536)
        if chunk == b'':
            break
        response += chunk
    return response

def run_tests(socket_path: str) -> None:
    sender = 'nobody@localhost'
    recipients = ('foo@example.com', 'bar@example.com')
    message = 'Subject: Hello!\n\nhello world\n'
    expected = f'''{sender}
{"\n".join(recipients)}

X-Header2: Bar
X-Header1: Foo
Received: from PID={os.getpid()} UID={os.geteuid()} with QMQP
Subject: Hello!

hello world
'''
    shutil.rmtree(record_file, ignore_errors=True)
    submit_to_qrelay(socket_path, encode_qmqp(message.encode('utf-8'), sender.encode('utf-8'), map(lambda x: x.encode('utf-8'), recipients)))
    with open(record_file, 'r', encoding='utf-8') as f:
        recorded = f.read()
    if recorded != expected:
        raise RuntimeError(f"Mismatch\nrecorded={recorded!r}\nexpected={expected!r}")

shutil.rmtree(runtime_directory, ignore_errors=True)
os.mkdir(runtime_directory)

process = subprocess.Popen(
    [
        os.path.join(build_directory, 'cm4all-qrelay'),
        '--config', os.path.join(config_directory, 'config.lua'),
    ],
    stdin=subprocess.DEVNULL,
    env={
        'QRELAY_SOCKET_PATH': socket_path,
        'QRELAY_RECORD_PROGRAM': os.path.join(test_directory, 'record.py'),
        'QRELAY_RECORD_FILE': record_file,
    },
)

# wait for startup to finish
time.sleep(0.2)

try:
    run_tests(socket_path)
finally:
    os.kill(process.pid, signal.SIGTERM)
    process.wait(10)
    process.kill()

    shutil.rmtree(runtime_directory, ignore_errors=True)
