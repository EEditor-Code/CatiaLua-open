# -*- coding: utf-8 -*-
"""
build_catialua.py - 通过 RadeBuildBridge 编译 CatiaLua，编译日志存到本地 temp 文件夹。

用法:
    python build_catialua.py                                  # 默认 mkmk -a -u 全构建
    python build_catialua.py --cmd "mkmk -u CatiaLuaCmd.m"    # 自定义构建命令（增量单模块）
    python build_catialua.py --timeout 7200                   # 超时秒数（默认 7200）

前提: VS2005 正在运行且 RadeBuildBridge Add-in 已加载（TCP 127.0.0.1:19528 在线）。
日志: <脚本所在目录>\\temp\\catialua_YYYYMMDD_HHMMSS.log
"""
import argparse
import datetime
import os
import socket
import subprocess
import sys
import time

BRIDGE_HOST = '127.0.0.1'
BRIDGE_PORT = 19528
DEFAULT_CMD = 'mkmk -a -u'
DEVENV_PATH = os.environ.get(
    'DEVENV_PATH',
    r'D:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE\devenv.exe')

# 终端输出按控制台编码打印，GBK 字节容错
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_DIR = os.path.join(SCRIPT_DIR, 'temp')


def make_log_path():
    """catialua_YYYYMMDD_HHMMSS.log，重名自动加 _2/_3 防覆盖。"""
    os.makedirs(LOG_DIR, exist_ok=True)
    stamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    path = os.path.join(LOG_DIR, 'catialua_%s.log' % stamp)
    n = 2
    while os.path.exists(path):
        path = os.path.join(LOG_DIR, 'catialua_%s_%d.log' % (stamp, n))
        n += 1
    return path


def log_line(fh, msg):
    line = '[%s] %s' % (datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'), msg)
    fh.write(line + '\n')
    fh.flush()


def connect():
    sock = socket.create_connection((BRIDGE_HOST, BRIDGE_PORT), timeout=10)
    sock.settimeout(None)
    return sock


def ensure_bridge(max_wait=120):
    """确保 bridge 在线；不在线则自动启动 VS2005 并轮询等待。返回 True/False。"""
    if _try_connect():
        return True
    print('[INFO] bridge 不在线，尝试启动 VS2005 ...')
    try:
        subprocess.Popen([DEVENV_PATH], cwd=os.path.dirname(DEVENV_PATH),
                         creationflags=subprocess.DETACHED_PROCESS)
    except OSError as e:
        print('[FAIL] 无法启动 VS2005: %s' % e)
        return False
    deadline = time.time() + max_wait
    while time.time() < deadline:
        if _try_connect():
            print('[OK] bridge 已上线')
            return True
        time.sleep(3)
    print('[FAIL] 等待 %ds 后 bridge 仍未上线（Add-in 加载失败？）' % max_wait)
    return False


def _try_connect():
    try:
        s = socket.create_connection((BRIDGE_HOST, BRIDGE_PORT), timeout=3)
        s.close()
        return True
    except OSError:
        return False


def send_one(cmd, timeout=15):
    """单命令短连接（bridge 协议：一连接一命令）。返回响应文本。"""
    sock = connect()
    sock.settimeout(timeout)
    try:
        sock.sendall((cmd + '\r\n').encode('utf-8'))
        chunks = []
        while True:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                break
            if not data:
                break
            chunks.append(data.decode('gbk', errors='replace'))
        return ''.join(chunks)
    finally:
        sock.close()


def send_and_stream(sock, cmd, fh, timeout):
    """发送命令，流式接收输出；返回 exit code（None=连接中断/超时）。"""
    sock.sendall((cmd + '\r\n').encode('utf-8'))
    sock.settimeout(timeout)

    buf = b''
    exit_code = None
    while True:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            log_line(fh, '[TIMEOUT after %ds]' % timeout)
            break
        except OSError:
            log_line(fh, '[CONNECTION CLOSED]')
            break
        if not data:
            log_line(fh, '[CONNECTION CLOSED BY BRIDGE]')
            break

        buf += data
        while b'\n' in buf:
            raw, buf = buf.split(b'\n', 1)
            line = raw.decode('gbk', errors='replace').rstrip('\r')
            # 终端显示（过滤空行保持界面干净）
            if line.strip():
                print(line)
            # 日志原样记录
            fh.write(line + '\n')
            fh.flush()

            if line.startswith('DONE '):
                try:
                    exit_code = int(line[5:])
                except ValueError:
                    exit_code = -1
                return exit_code
            if line.startswith('ERROR'):
                exit_code = -1
                return exit_code

    # 尾部残留（无换行结尾）
    if buf.strip():
        line = buf.decode('gbk', errors='replace').rstrip('\r')
        if line.strip():
            print(line)
        fh.write(line + '\n')
        fh.flush()
    return exit_code


def main():
    parser = argparse.ArgumentParser(description='Compile CatiaLua via RadeBuildBridge')
    parser.add_argument('--cmd', default=DEFAULT_CMD,
                        help='build command, default: %s' % DEFAULT_CMD)
    parser.add_argument('--timeout', type=int, default=7200,
                        help='socket timeout in seconds, default 7200')
    args = parser.parse_args()

    # 1. 确保 bridge 在线（不在线自动启动 VS2005）
    if not ensure_bridge():
        print('[FAIL] 无法连接 bridge (%s:%d)。' % (BRIDGE_HOST, BRIDGE_PORT))
        print('       请手动确认 VS2005 正在运行且 RadeBuildBridge Add-in 已加载。')
        sys.exit(1)
    sock = None

    log_path = make_log_path()
    fh = open(log_path, 'w', encoding='utf-8')
    log_line(fh, '===== CatiaLua build start =====')
    log_line(fh, 'Bridge: %s:%d' % (BRIDGE_HOST, BRIDGE_PORT))
    log_line(fh, 'Workspace: %s' % SCRIPT_DIR)
    log_line(fh, 'Command: BUILD %s' % args.cmd)
    log_line(fh, 'Log file: %s' % log_path)
    print('日志: %s' % log_path)
    start = datetime.datetime.now()

    try:
        # 2. 工作区确认/纠正（独立短连接）
        data = send_one('STATUS')
        print(data.strip())
        log_line(fh, data.strip())
        if 'workspace=' + SCRIPT_DIR not in data:
            print('工作区不匹配，发送 SET_WORKSPACE ...')
            data = send_one('SET_WORKSPACE %s' % SCRIPT_DIR)
            print(data.strip())
            log_line(fh, data.strip())

        # 3-4. 发送构建并流式输出（新连接）
        sock = connect()
        exit_code = send_and_stream(sock, 'BUILD ' + args.cmd, fh, args.timeout)
    finally:
        sock.close()

    # 6. 收尾
    dur = (datetime.datetime.now() - start).total_seconds()
    log_line(fh, '===== build finished, exit=%s, duration=%.0fs =====' % (exit_code, dur))
    fh.close()

    print('\n[exit=%s] 用时 %.0f 秒' % (exit_code, dur))
    if exit_code == 0:
        print('[OK] 构建完成: %s\\win_b64\\code\\bin\\CatiaLuaAddin.dll / CatiaLuaCmd.dll' % SCRIPT_DIR)
    else:
        print('[FAIL] 构建失败，详见日志: %s' % log_path)
    sys.exit(0 if exit_code == 0 else 1)


if __name__ == '__main__':
    main()
