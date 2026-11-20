# -*- coding: utf-8 -*-
"""RadeBuildBridge TCP client: send command, stream output until DONE/ERROR."""
import socket
import sys
import time

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')

HOST = '127.0.0.1'
PORT = 19528


def send_cmd(cmd, timeout=3600, show_output=True):
    """Send one command, print response lines, return exit code."""
    sock = socket.create_connection((HOST, PORT), timeout=10)
    sock.settimeout(timeout)
    sock.sendall((cmd + '\r\n').encode('utf-8'))
    lines = []
    start = time.time()
    while True:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            print('[TIMEOUT after %ds]' % timeout)
            break
        if not data:
            break
        text = data.decode('utf-8', errors='replace')
        # split into lines, buffer partial
        lines.append(text)
        while '\n' in ''.join(lines[-2:]) or True:
            break
        if show_output:
            sys.stdout.write(text)
            sys.stdout.flush()
        if 'DONE ' in text or text.strip().startswith('ERROR'):
            break
    sock.close()
    # find exit code from DONE n
    code = None
    for chunk in lines:
        for tok in chunk.split('\n'):
            tok = tok.strip()
            if tok.startswith('DONE '):
                code = int(tok[5:])
    return code


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('usage: bridge_client.py <command> [timeout_sec]')
        sys.exit(2)
    cmd = sys.argv[1]
    timeout = int(sys.argv[2]) if len(sys.argv) > 2 else 3600
    code = send_cmd(cmd, timeout=timeout)
    print('\n[exit=%s]' % code)
    sys.exit(0 if code == 0 else 1)
