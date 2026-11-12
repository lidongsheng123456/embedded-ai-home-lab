"""调整接地支线释放限流脚的逃线空间，不改变器件布局。"""
import copy
import hashlib
import json
from pathlib import Path
import sys

from pcb_geometry import CopperModel, LineString, Point


def main():
    board_path, intent_path, prefix = map(Path, sys.argv[1:4])
    load = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
    board, intent = load(board_path), load(intent_path)
    model = CopperModel(board, intent)
    pads = {p['ref']: p for p, _ in model.pads}
    p8, p7, resistor = (pads[ref] for ref in ['U7.8', 'U7.7', 'R12.1'])
    if any(t['net'] == 'EFUSE_ILIM' for t, _ in model.tracks):
        raise ValueError('ILIM 已有铜，拒绝重放')
    old_vias = [v for v, _ in model.vias if v['net'] == 'GND' and abs(v['y'] - p8['y']) < .01 and 20 < v['x'] - p8['x'] < 35]
    if len(old_vias) != 1:
        raise ValueError('未找到唯一待调整接地孔')
    old_via = old_vias[0]
    ground_tracks = [t for t, g in model.tracks if t['net'] == 'GND' and g.distance(Point(old_via['x'], old_via['y'])) < .01]
    if len(ground_tracks) != 1:
        raise ValueError('接地孔被多个支线使用')
    remove_tracks = ground_tracks
    remove_vias = [old_via]
    projected = copy.deepcopy(board)
    projected['copper']['lines'] = [t for t in board['copper']['lines'] if t['primitiveId'] not in {t['primitiveId'] for t in remove_tracks}]
    projected['copper']['vias'] = [v for v in board['copper']['vias'] if v['primitiveId'] not in {v['primitiveId'] for v in remove_vias}]
    check = CopperModel(projected, intent)
    delta = old_via['x'] - p8['x']
    via = [p8['x'] + delta, p8['y'] + delta]
    if not check.via_legal(Point(*via), 'GND'):
        raise ValueError('新接地孔净距不足')
    tracks = [{'net': 'GND', 'layer': 1, 'lineWidth': 8, 'a': [p8['x'], p8['y']], 'b': via}]
    check.extra_vias.append(({'net': 'GND', 'diameter': 24}, Point(*via)))
    check.extra_tracks.append((tracks[0], LineString([tracks[0]['a'], tracks[0]['b']])))
    tracks.append({'net': 'EFUSE_ILIM', 'layer': 1, 'lineWidth': 8, 'a': [p7['x'], p7['y']], 'b': [resistor['x'], p7['y']]})
    if not next(g for p, g in check.pads if p['ref'] == 'R12.1').covers(Point(*tracks[-1]['b'])):
        raise ValueError('限流连接终点没有落在电阻铜内')
    for t in tracks:
        line = LineString([t['a'], t['b']])
        if line.distance(check.obstacles(t['net'], t['layer'])) < t['lineWidth'] / 2 - 1e-6:
            raise ValueError('新铜与异网净距不足：' + t['net'])
        if not check.inside.buffer(-(20.1 + t['lineWidth'] / 2)).covers(line):
            raise ValueError('新铜接近板边')
        if t is not tracks[0]:
            check.extra_tracks.append((t, line))
    steps = [{'id': 'remove-local-tracks', 'action': 'pcb.route.delete', 'payload': {'kind': 'track', 'primitiveIds': [t['primitiveId'] for t in remove_tracks]}},
             {'id': 'remove-local-vias', 'action': 'pcb.route.delete', 'payload': {'kind': 'via', 'primitiveIds': [v['primitiveId'] for v in remove_vias]}},
             {'id': 'new-gnd-via', 'action': 'pcb.via.create', 'payload': {'net': 'GND', 'x': via[0], 'y': via[1], 'diameter': 24, 'holeDiameter': 12}}]
    for k, t in enumerate(tracks):
        steps.append({'id': f'local-track-{k}', 'action': 'pcb.line.create', 'payload': {'net': t['net'], 'layer': t['layer'], 'lineWidth': t['lineWidth'], 'startX': t['a'][0], 'startY': t['a'][1], 'endX': t['b'][0], 'endY': t['b'][1]}})
    steps.append({'id': 'save-local-repair', 'action': 'pcb.save', 'assert': {'$.saved': 'true'}})
    report = {'status': 'offline-verified', 'boardSha256': hashlib.sha256(board_path.read_bytes()).hexdigest(), 'removedTracks': remove_tracks,
              'removedVias': remove_vias, 'newTracks': tracks, 'newGroundVia': via, 'clearanceMil': 8.1, 'nativeDrcRequired': True}
    for suffix, value in [('.json', report), ('.apply.json', {'version': 1, 'meta': {'name': 'Release ILIM with local ground stub adjustment', 'project': intent['projectId'], 'doc': intent['documentId']}, 'steps': steps})]:
        with Path(str(prefix) + suffix).open('x', encoding='utf-8') as f:
            json.dump(value, f, indent=2)
    print(json.dumps({'steps': len(steps), 'newGroundVia': via}))


if __name__ == '__main__':
    main()
