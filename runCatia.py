# -*- coding: utf-8 -*-
"""
runCatia.py - 启动 CATIA（绕过 CATSTART.exe，直接运行 CNEXT.exe）。

用法:
    python runCatia.py

说明:
    - CATSTART.exe 无法直接运行，改为解析 V5R18.txt 环境文件并设置环境变量后
      直接启动 CNEXT.exe（CATSTART 本质就是"解析 env -> 设置环境 -> 启动 CNEXT"）。
    - 优先使用工作区 win_b64\\code\\bin\\CNEXT.exe（与 B18 安装同源）。
    - 默认 CATIA 安装目录为 D:\\CATIA\\B18，可用环境变量 CATIA_INSTALL 覆盖；
      若安装路径不同，可从 CATEnv 的 *.txt 中自动探测。
"""
import os
import subprocess
import sys
import time

WORKSPACE = os.path.dirname(os.path.abspath(__file__))
ENV_FILE = os.path.join(WORKSPACE, 'V5R18.txt')
CNEXT_LOCAL = os.path.join(WORKSPACE, 'win_b64', 'code', 'bin', 'CNEXT.exe')
CATIA_INSTALL_DEFAULT = os.environ.get('CATIA_INSTALL', r'D:\CATIA\B18')
CATENV_DIR = os.environ.get('CATENV_DIR', r'D:\CATIA\CATEnv')
CNEXT_INSTALL = os.path.join(CATIA_INSTALL_DEFAULT, 'win_b64', 'code', 'bin', 'CNEXT.exe')

# CSIDL_* / DSKEY_* 特殊值 -> 实际环境变量
CSIDL_MAP = {
    'CSIDL_APPDATA': 'APPDATA',
    'CSIDL_LOCAL_APPDATA': 'LOCALAPPDATA',
    'DSKEY_TMPDIR': 'TEMP',
}


def _expand_csidl(value):
    """把值里的 CSIDL_* / DSKEY_* token 替换为真实路径前缀。"""
    for token, env in CSIDL_MAP.items():
        if token in value:
            base = os.environ.get(env, '')
            if env == 'TEMP':
                base = os.environ.get('TEMP', '') or os.environ.get('TMP', '')
            value = value.replace(token, base)
    if 'CSIDL_PERSONAL' in value:
        docs = os.path.join(os.environ.get('USERPROFILE', ''), 'Documents')
        value = value.replace('CSIDL_PERSONAL', docs)
    return value


def _expand_vars(value, parsed, max_iter=20):
    """展开 %VAR% 引用（优先用已解析的 env 变量，其次系统环境变量）。

    带迭代上限，防止 CLASSPATH=%CLASSPATH% 这类自引用造成死循环；
    自引用变量回退为系统环境变量（通常为空）。
    """
    for _ in range(max_iter):
        if '%' not in value:
            break
        start = value.find('%')
        end = value.find('%', start + 1)
        if end == -1:
            break
        name = value[start + 1:end]
        if name in parsed and ('%' + name + '%') in parsed[name]:
            # 自引用：parsed[name] 包含 %name% 自身 → 用系统值（通常为空）
            repl = os.environ.get(name, '')
        else:
            repl = parsed.get(name) or os.environ.get(name, '')
        value = value[:start] + repl + value[end + 1:]
    return value


def parse_env_file(path):
    """解析 V5R18.txt，返回 {KEY: VALUE}。"""
    parsed = {}
    with open(path, 'r', encoding='utf-8-sig') as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith('!'):
                continue
            if '=' not in line:
                continue
            key, value = line.split('=', 1)
            key = key.strip()
            if not key:
                continue
            parsed[key] = value
    return parsed


def find_first_dir_with_file(path_list, filename):
    """按 ';' 分隔的路径列表中，返回第一个包含 filename 的目录。"""
    for p in path_list.split(';'):
        p = p.strip()
        if p and os.path.exists(os.path.join(p, filename)):
            return p
    return None


def find_catia_install_cnext():
    """从 CATEnv\\*.txt 探测 CATInstallPath，返回其 CNEXT.exe 路径。"""
    if not os.path.isdir(CATENV_DIR):
        return None
    for name in sorted(os.listdir(CATENV_DIR)):
        if not name.lower().endswith('.txt'):
            continue
        try:
            with open(os.path.join(CATENV_DIR, name), 'r', encoding='utf-8-sig') as fh:
                for line in fh:
                    if not line.startswith('CATInstallPath='):
                        continue
                    install = line.split('=', 1)[1].strip().split(';')[0]
                    install = install.rstrip('\\/')
                    if install.lower().endswith('\\win_b64'):
                        install = install[:-len('\\win_b64')]
                    cnext = os.path.join(install, 'win_b64', 'code', 'bin', 'CNEXT.exe')
                    if os.path.exists(cnext):
                        return cnext
        except OSError:
            continue
    return None


def main():
    # 1. 路径检查
    if not os.path.exists(ENV_FILE):
        print('[FAIL] 找不到环境文件: %s' % ENV_FILE)
        sys.exit(1)
    cnext = None
    if os.path.exists(CNEXT_LOCAL):
        cnext = CNEXT_LOCAL
    elif os.path.exists(CNEXT_INSTALL):
        cnext = CNEXT_INSTALL
    else:
        cnext = find_catia_install_cnext()
    if not cnext:
        print('[FAIL] 找不到 CNEXT.exe。')
        print('       已检查: %s' % CNEXT_LOCAL)
        print('       已检查: %s' % CNEXT_INSTALL)
        print('       已检查: %s 下的 CATInstallPath' % CATENV_DIR)
        sys.exit(1)

    # 2-3. 解析环境文件并设置环境变量
    parsed = parse_env_file(ENV_FILE)
    for key, value in parsed.items():
        value = _expand_csidl(value)
        value = _expand_vars(value, parsed)
        os.environ[key] = value

    # 早期校验: CATDLLPath 必须能找到 CATApplicationFrame.dll，否则 CNEXT 会 0xC0000135
    catdll = os.environ.get('CATDLLPath', '')
    if not find_first_dir_with_file(catdll, 'CATApplicationFrame.dll'):
        print('[WARN] CATDLLPath 中找不到 CATApplicationFrame.dll，CATIA 可能无法启动。')
        print('       当前 CATDLLPath: %s' % catdll)
        print('       请确认 V5R18.txt 指向真实安装，例如 D:\\CATIA\\B18。')

    # 4. 启动 CNEXT.exe（GUI，不等待）
    #    stdout/stderr 重定向到 DEVNULL：CNEXT 会长期持有管道，
    #    若不重定向会导致调用方（终端/脚本）等待管道关闭而卡住。
    bin_dir = os.path.dirname(cnext)
    cmd = [cnext, '-env', 'V5R18', '-direnv', WORKSPACE]
    print('[INFO] 启动: %s' % ' '.join(cmd))
    print('[INFO] 工作目录: %s' % bin_dir)
    try:
        proc = subprocess.Popen(
            cmd, cwd=bin_dir,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
    except OSError as e:
        print('[FAIL] 启动失败: %s' % e)
        sys.exit(1)

    # 5. 验证：等 5 秒看进程是否存活（崩溃则立即退出）
    time.sleep(5)
    if proc.poll() is not None:
        rc = proc.returncode
        print('[FAIL] CNEXT 启动后立即退出 (exit=%s)。' % rc)
        if rc is not None and (rc & 0xFFFFFFFF) == 0xC0000135:
            print('       0xC0000135 = STATUS_DLL_NOT_FOUND: CATIA 安装路径不正确或缺少 DLL。')
            print('       请检查 V5R18.txt 的 CATDLLPath/PATH 是否包含真实安装目录。')
        else:
            print('       环境变量可能不完整，请检查 V5R18.txt 与本机 CATIA 安装路径。')
        sys.exit(1)
    print('[OK] CNEXT 运行中 (PID=%d)，CATIA 界面即将弹出。' % proc.pid)


if __name__ == '__main__':
    main()
