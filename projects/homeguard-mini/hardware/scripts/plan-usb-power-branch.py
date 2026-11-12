"""把开发板 USB 电源长支路移到底层，释放 U10 使能脚上方的逃线通道。"""
import copy
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys


def main():
    board_path, intent_path, prefix = map(Path, sys.argv[1:4])
    load = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
    board, intent = load(board_path), load(intent_path)
    if not board['capture']['identityVerified'] or board['capture']['documentUuid'] != intent['documentId']:
        raise ValueError('快照身份不符')
    tracks = [t for t in board['copper']['lines'] if t['net'] == 'USB_MCU_5V']
    pads = [p for c in board['components'] if c['designator'] == 'J11' for p in c['pads'] if p['padNumber'] == '1']
    if len(pads) != 1:
        raise ValueError('USB-A 电源脚身份不明')
    node = lambda x, y: (round(x, 1), round(y, 1))
    graph = {}
    for t in tracks:
        a, b = node(t['startX'], t['startY']), node(t['endX'], t['endY'])
        graph.setdefault(a, []).append((b, t))
        graph.setdefault(b, []).append((a, t))
    current = min(graph, key=lambda p: math.dist(p, (pads[0]['x'], pads[0]['y'])))
    if math.dist(current, (pads[0]['x'], pads[0]['y'])) > .2:
        raise ValueError('没有落在 USB-A 焊盘中心的铜路径端点')
    if len(graph.get(current, [])) != 1:
        raise ValueError('旧 USB-A 末端不是唯一支路')
    removed, previous = [], None
    while True:
        options = [(n, t) for n, t in graph[current] if n != previous]
        if len(options) != 1:
            break
        nxt, track = options[0]
        removed.append(track)
        previous, current = current, nxt
        if len(graph[current]) != 2:
            break
    if not 5 <= len(removed) <= 40 or len(graph[current]) < 3:
        raise ValueError('待移支路与预期拓扑不同')
    projected = copy.deepcopy(board)
    ids = {t['primitiveId'] for t in removed}
    projected['copper']['lines'] = [t for t in board['copper']['lines'] if t['primitiveId'] not in ids]
    projected['projection'] = {'kind': 'candidate-after-track-removal', 'source': str(board_path), 'removedTracks': sorted(ids)}
    projected_path = Path(str(prefix) + '.projected.json')
    request_path = Path(str(prefix) + '.request.json')
    request = {'nets': ['USB_MCU_5V'], 'terminals': {'USB_MCU_5V': ['J11.1', 'C25.1']}, 'widthMil': 40, 'allowExisting': True}
    for path, value in [(projected_path, projected), (request_path, request)]:
        with path.open('x', encoding='utf-8') as f:
            json.dump(value, f)
    child = str(prefix) + '.route'
    subprocess.run([sys.executable, '-X', 'utf8', str(Path(__file__).with_name('plan-bottom-escapes.py')),
                    str(projected_path), str(intent_path), str(request_path), child], check=True)
    playbook = load(Path(child + '.apply.json'))
    playbook['meta']['name'] = 'Move USB power branch to BOTTOM with 40mil trunk'
    playbook['steps'].insert(0, {'id': 'remove-old-usb-branch', 'action': 'pcb.route.delete',
                               'payload': {'kind': 'track', 'primitiveIds': sorted(ids)}})
    report = {'status': 'offline-verified', 'boardSha256': hashlib.sha256(board_path.read_bytes()).hexdigest(),
              'removedTracks': removed, 'branchJunction': current, 'newRouting': load(Path(child + '.json')), 'nativeDrcRequired': True}
    for suffix, value in [('.json', report), ('.apply.json', playbook)]:
        with Path(str(prefix) + suffix).open('x', encoding='utf-8') as f:
            json.dump(value, f, indent=2)
    print(json.dumps({'removed': len(removed), 'newSteps': len(playbook['steps'])}))


if __name__ == '__main__':
    main()
