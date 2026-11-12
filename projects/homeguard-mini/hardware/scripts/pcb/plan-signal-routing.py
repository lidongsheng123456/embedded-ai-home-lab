"""从真实铜的连通分量规划双面信号线；内层只保留参考地，不参与绕线。"""
import hashlib
import heapq
import importlib.util
import json
import math
import sys
from pathlib import Path

from pcb_geometry import CopperModel, LineString, Point, unary_union
spec = importlib.util.spec_from_file_location('bottom_escapes', Path(__file__).with_name('plan-bottom-escapes.py'))
escapes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(escapes)
compress, smooth45 = escapes.compress, escapes.smooth45

import numpy as np
import shapely


def copper_groups(model, net, width):
    items = []
    for p, g in model.pads:
        if p['net'] == net:
            items.append((set((1, 2) if p['layer'] == 12 else (p['layer'],)), g, p['ref']))
    for t, g in model.tracks + model.extra_tracks:
        if t['net'] == net:
            items.append(({t['layer']}, g.buffer(t['lineWidth'] / 2), 'track'))
    for v, g in model.vias + model.extra_vias:
        if v['net'] == net:
            items.append(({1, 2}, g.buffer(v['diameter'] / 2), 'via'))
    parents = list(range(len(items)))
    def root(i):
        while parents[i] != i:
            parents[i] = parents[parents[i]]
            i = parents[i]
        return i
    for i, (layers, geom, _) in enumerate(items):
        for j in range(i):
            if layers & items[j][0] and geom.distance(items[j][1]) < .02:
                parents[root(i)] = root(j)
    groups = {}
    for i, item in enumerate(items):
        groups.setdefault(root(i), []).append(item)
    return [{layer: unary_union([g for ls, g, _ in group if layer in ls])
             for layer in (1, 2)} for group in groups.values()]


def group_nodes(group, blocked, step):
    nodes = []
    for layer in (1, 2):
        g = group[layer]
        if g.is_empty:
            continue
        x0, y0, x1, y1 = g.bounds
        xs = np.arange(max(0, math.ceil(x0 / step)), min(blocked.shape[2], math.floor(x1 / step) + 1))
        ys = np.arange(max(0, math.ceil(y0 / step)), min(blocked.shape[1], math.floor(y1 / step) + 1))
        xx, yy = np.meshgrid(xs, ys)
        mask = shapely.contains_xy(g.buffer(-.05), xx * step, yy * step)
        mask &= ~blocked[layer - 1, yy, xx]
        nodes.extend((layer - 1, int(y), int(x)) for y, x in zip(yy[mask], xx[mask]))
    return nodes


def search(blocked, via_blocked, starts, goals, step, budget=3000000):
    if not starts or not goals:
        return None
    _, h, w = blocked.shape
    shape = blocked.shape
    goal_mask = np.zeros(shape, dtype=bool)
    goal_mask[tuple(np.array(goals).T)] = True
    gs = np.array(goals)
    lower, upper = gs[:, 1:].min(axis=0), gs[:, 1:].max(axis=0)
    def heuristic(y, x):
        dy = max(lower[0] - y, y - upper[0], 0)
        dx = max(lower[1] - x, x - upper[1], 0)
        return max(dx, dy) + (math.sqrt(2) - 1) * min(dx, dy)
    distance = np.full(shape, np.inf)
    parent = np.full(shape, -1, dtype=np.int64)
    queue = []
    for l, y, x in starts:
        distance[l, y, x] = 0
        heapq.heappush(queue, (heuristic(y, x), 0., l, y, x))
    neighbors = [(dy, dx, math.hypot(dx, dy)) for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dx or dy]
    count = 0
    while queue:
        _, cost, l, y, x = heapq.heappop(queue)
        if cost > distance[l, y, x] + 1e-8:
            continue
        count += 1
        if count > budget:
            return None
        if goal_mask[l, y, x]:
            result = [(l, y, x)]
            while parent[result[-1]] >= 0:
                result.append(tuple(np.unravel_index(parent[result[-1]], shape)))
            return result[::-1]
        options = [(l, y + dy, x + dx, d) for dy, dx, d in neighbors
                   if 0 <= y + dy < h and 0 <= x + dx < w
                   and not blocked[l, y + dy, x + dx]
                   and (not dy or not dx or not (blocked[l, y, x + dx] or blocked[l, y + dy, x]))]
        if not via_blocked[y, x] and not blocked[1 - l, y, x]:
            options.append((1 - l, y, x, 150 / step))
        for nl, ny, nx, delta in options:
            value = cost + delta
            if value + 1e-8 < distance[nl, ny, nx]:
                distance[nl, ny, nx] = value
                parent[nl, ny, nx] = (l * h + y) * w + x
                heapq.heappush(queue, (value + heuristic(ny, nx), value, nl, ny, nx))
    return None


def main():
    board_path, intent_path, request_path, prefix = map(Path, sys.argv[1:5])
    load = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
    board, intent, request = load(board_path), load(intent_path), load(request_path)
    model = CopperModel(board, intent)
    step, width = request.get('gridMil', 5), request.get('widthMil', 8)
    if step not in (2.5, 5) or width < 8:
        raise ValueError('非法网格或线宽')
    axis = np.arange(0, 3940, step)
    xx, yy = np.meshgrid(axis, axis)
    interior = model.inside.buffer(-(20.1 + width / 2))
    routes, unresolved = [], []
    for net in request['nets']:
        for attempt in range(50):
            groups = copper_groups(model, net, width)
            if len(groups) < 2:
                print(json.dumps({'net': net, 'connected': True}), flush=True)
                break
            obstacles = [model.obstacles(net, layer) for layer in (1, 2)]
            margin = width / 2 + request.get('gridSafetyMil', step / math.sqrt(2) + .02)
            blocked = np.array([shapely.intersects_xy(g.buffer(margin), xx, yy) for g in obstacles])
            blocked |= ~shapely.contains_xy(model.inside.buffer(-(20.1 + margin)), xx, yy)
            # 过孔禁止钻在焊盘内；同时核对两外层、既有过孔、铣孔和全层禁布区。
            vg = unary_union([g.buffer(16 if p['net'] == net else 20.1) for p, g in model.pads]
                 + [g.buffer(t['lineWidth'] / 2 + 20.1) for t, g in model.tracks + model.extra_tracks if t['net'] != net]
                 + [g.buffer(v['diameter'] / 2 + 20.1) for v, g in model.vias + model.extra_vias]
                 + [g.buffer(r['lineWidth'] / 2 + 12) for r, g in model.regions]
                 + [g.buffer(f['lineWidth'] / 2 + 32.1) for f, g in model.cuts])
            via_blocked = shapely.intersects_xy(vg, xx, yy)
            via_blocked |= ~shapely.contains_xy(model.inside.buffer(-32.1), xx, yy)
            nodes = [group_nodes(g, blocked, step) for g in groups]
            pairs = sorted((min(groups[i][l].distance(groups[j][l]) for l in (1, 2)
                                if not groups[i][l].is_empty and not groups[j][l].is_empty), i, j)
                           for i in range(len(groups)) for j in range(i))
            path = None
            for _, i, j in pairs:
                path = search(blocked, via_blocked, nodes[i], nodes[j], step)
                if path:
                    break
            if path is None:
                unresolved.append(net)
                print(json.dumps({'net': net, 'unresolved': len(groups), 'nodes': list(map(len, nodes))}), flush=True)
                break
            segments, vias = [], []
            start = 0
            for k in range(1, len(path) + 1):
                if k < len(path) and path[k][0] == path[start][0]:
                    continue
                layer = int(path[start][0]) + 1
                points = compress([[float(x * step), float(y * step)] for _, y, x in path[start:k]])
                legal = lambda ps: interior.covers(LineString(ps)) and LineString(ps).distance(obstacles[layer - 1]) >= width / 2 - 1e-6
                if len(points) > 1:
                    points = smooth45(points, legal)
                    for a, b in zip(points, points[1:]):
                        if not legal([a, b]):
                            raise ValueError('连续几何复验失败')
                        segments.append({'net': net, 'layer': layer, 'lineWidth': width,
                                         'startX': a[0], 'startY': a[1], 'endX': b[0], 'endY': b[1]})
                if k < len(path):
                    _, y, x = path[k]
                    point = Point(x * step, y * step)
                    if not model.via_legal(point, net):
                        raise ValueError('过孔独立复验失败')
                    vias.append({'net': net, 'x': float(x * step), 'y': float(y * step), 'diameter': 24, 'holeDiameter': 12})
                start = k
            for t in segments:
                model.extra_tracks.append((t, LineString([(t['startX'], t['startY']), (t['endX'], t['endY'])])))
            for v in vias:
                model.extra_vias.append((v, Point(v['x'], v['y'])))
            routes.append({'net': net, 'lines': segments, 'vias': vias})
            print(json.dumps({'net': net, 'lines': len(segments), 'vias': len(vias), 'groupsBefore': len(groups)}), flush=True)
        else:
            raise ValueError('连通分量未收敛')
    report = {'status': 'incomplete' if unresolved else 'offline-verified', 'routes': routes, 'unresolved': unresolved,
              'boardSha256': hashlib.sha256(board_path.read_bytes()).hexdigest(), 'nativeDrcRequired': True}
    Path(str(prefix) + '.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    steps = []
    for r in routes:
        for action, key in [('pcb.via.create', 'vias'), ('pcb.line.create', 'lines')]:
            for payload in r[key]:
                steps.append({'id': f'route-{len(steps)}', 'action': action, 'payload': payload})
    steps.append({'id': 'save', 'action': 'pcb.save', 'assert': {'$.saved': 'true'}})
    with Path(str(prefix) + '.apply.json').open('x', encoding='utf-8') as f:
        json.dump({'version': 1, 'meta': {'name': '双面信号线连续几何复验', 'project': intent['projectId'], 'doc': intent['documentId']}, 'steps': steps}, f, indent=2)


if __name__ == '__main__':
    main()
