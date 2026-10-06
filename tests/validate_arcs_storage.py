#!/usr/bin/env python3
"""ARCS 双 Flash/eMMC 实机回归；先备份测试区域，结束后恢复并校验。"""
import argparse
import hashlib
import json
import random
import re
import subprocess
import time
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cskburn', required=True)
parser.add_argument('--port', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--chip-id', required=True, help='预期 ARCS Chip ID，避免选错开发板')
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=False)
exe = str(Path(args.cskburn).resolve())
base = [exe, '-v', '-C', 'arcs', '-s', args.port, '--reset-strategy', 'dtr-boot',
        '--reset-attempts', '1', '--no-progress']
results = []
state = {'passed': False, 'restored': False, 'steps': results}

def md5(data):
    return hashlib.md5(data).hexdigest()

def run(name, options, *, emmc=False, baud=3000000, error=None):
    command = base + ['-b', str(baud)] + (['--emmc'] if emmc else []) + list(map(str, options))
    start = time.monotonic()
    log = out / (name + '.log')
    with log.open('w') as f:
        try:
            rc = subprocess.run(command, stdout=f, stderr=subprocess.STDOUT, timeout=600).returncode
        except subprocess.TimeoutExpired:
            rc = -999
    text = log.read_text(errors='replace')
    passed = rc == 0 if error is None else rc not in (0, -999) and error in text
    row = {'name': name, 'command': command, 'returncode': rc, 'passed': passed,
           'seconds': round(time.monotonic() - start, 3),
           'timings': re.findall(r'(?:Reading|Erasing|Writing|Verifying) took [^\n]+', text)}
    results.append(row)
    (out / 'results.json').write_text(json.dumps(state, ensure_ascii=False, indent=2) + '\n')
    print(name, 'PASS' if passed else 'FAIL', row['seconds'], flush=True)
    assert passed, text[-4000:]
    return text

def read(name, address, size, emmc=False, baud=3000000):
    path = out / (name + '.bin')
    text = run(name, ['--verify-all', '--read', f'{address}:{size}:{path}'], emmc=emmc, baud=baud)
    data = path.read_bytes()
    assert len(data) == size and md5(data) in text
    return data

def write(name, address, data, emmc=False, baud=3000000):
    path = out / (name + '.bin')
    path.write_bytes(data)
    text = run(name, ['--verify-all', address, path], emmc=emmc, baud=baud)
    assert md5(data) in text

probe = run('probe', ['--chip-id', '--flash-protection'])
assert args.chip_id.lower() in probe.lower() and '2 devices, 32 MB total' in probe
assert 'Flash read mode: stream' in probe
probe = run('emmc-info', ['--chip-id'], emmc=True)
assert args.chip_id.lower() in probe.lower()
state['emmc_info'] = re.findall(r'eMMC: [^\n]+', probe)
# 每颗 Flash 2 MiB；eMMC 低、高地址各 3 MiB，包含擦除组两侧保护数据。
regions = [(False, 0x600000, 2 << 20), (False, 0x1600000, 2 << 20),
           (True, 0, 3 << 20), (True, 0x80200000, 3 << 20)]
backups = []
for i, (emmc, address, size) in enumerate(regions):
    backups.append(read(f'backup-{i}', address, size, emmc))
state['backups'] = [{'emmc': r[0], 'address': r[1], 'size': r[2], 'md5': md5(d)}
                    for r, d in zip(regions, backups)]
rng = random.Random(20261006)
dirty = set()
current_flash = backups[:2].copy()
try:
    # 两种波特率、两种长度、两颗 Flash；读取同时校验设备 MD5。
    for baud in (1000000, 3000000):
        for size in (1 << 20, 2 << 20):
            for i in (0, 1):
                address = regions[i][1]
                name = f'flash-{i}-{baud}-{size}'
                data = rng.randbytes(size)
                dirty.add(i)
                write(name + '-write', address, data, baud=baud)
                assert read(name + '-read', address, size, baud=baud) == data
                other = 1 - i
                # 对另一颗同偏移区校验，确保无串片写入。
                text = run(name + '-peer', ['--verify', f'{regions[other][1]}:{regions[other][2]}'])
                expected = current_flash[other]
                assert md5(expected) in text
                current_flash[i] = data + current_flash[i][size:]
    # eMMC 的精确擦除验证包括 4 KiB 范围及跨扇区的 7 字节范围。
    for baud in (1000000, 3000000):
        for i in (2, 3):
            address, bank_size = regions[i][1:]
            name = f'emmc-{i}-{baud}'
            dirty.add(i)
            expected = bytearray(rng.randbytes(bank_size))
            write(name + '-prefill', address, expected, emmc=True, baud=baud)
            offset = 0x81000 if i == 2 else 0x811FD
            size = 4096 if i == 2 else 7
            run(name + '-erase', ['--erase', f'{address+offset}:{size}'], emmc=True, baud=baud)
            got = read(name + '-erased', address, bank_size, True, baud)
            fill = got[offset]
            assert fill in (0, 255) and got[offset:offset+size] == bytes([fill])*size
            expected[offset:offset+size] = bytes([fill])*size
            assert got == expected, '擦除修改了范围外的数据'
            # 非扇区对齐的 4099 字节写入，完整回读保护相邻内容。
            payload = bytes([0xC0, 0xDB, 0, 255]) * 1024 + b'END'
            write(name + '-tail-write', address + offset, payload, emmc=True, baud=baud)
            expected[offset:offset+len(payload)] = payload
            assert read(name + '-tail-read', address, bank_size, True, baud) == expected
    # CLI 拒绝零长度、跨协议边界、跨芯片边界，避免执行错误范围。
    run('reject-zero', ['--erase', '0:0'], emmc=True, error='E1001')
    run('reject-4g', ['--erase', '0xFFFFF000:8192'], emmc=True, error='E1006')
    run('reject-cross-chip', ['--erase', '0xFFF000:8192'], error='E1006')
    run('reject-flash-unaligned', ['--erase', '0x600001:7'], error='E1005')
    # 专门检查 D2 读取：短末块、特殊 SLIP 字节，不依赖调试器。
    pattern = bytes([0xC0, 0xDB]) * 2048 + b'END'
    write('flash-pattern', regions[0][1], pattern)
    assert read('flash-pattern-read', regions[0][1]+1, len(pattern)-1) == pattern[1:]
    state['passed'] = True
finally:
    restore_ok = True
    for i in sorted(dirty):
        emmc, address, size = regions[i]
        try:
            write(f'restore-{i}', address, backups[i], emmc=emmc)
            assert read(f'restore-{i}-read', address, size, emmc) == backups[i]
        except Exception as exc:
            restore_ok = False
            results.append({'name': f'restore-{i}-failure', 'passed': False, 'error': str(exc)})
    state['restored'] = restore_ok
    (out / 'results.json').write_text(json.dumps(state, ensure_ascii=False, indent=2) + '\n')
assert state['passed'] and state['restored']
print('ALL PASSED; test regions restored and compared', flush=True)
