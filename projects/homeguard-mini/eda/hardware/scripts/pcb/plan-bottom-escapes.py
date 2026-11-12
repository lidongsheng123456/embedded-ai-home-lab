"""SMD 短顶层逃线，经离焊盘过孔接底层；离线求解后独立复验才生成 typed 队列。"""
import hashlib
import heapq
import json
import math
from pathlib import Path
import sys

from pcb_geometry import CopperModel, LineString, Point
import numpy as np
import shapely


def astar(blocked, start, goal, budget=500000):
    h, w = blocked.shape
    sx, sy = start
    gx, gy = goal
    heuristic = lambda x, y: max(abs(x - gx), abs(y - gy)) + (math.sqrt(2) - 1) * min(abs(x - gx), abs(y - gy))
    queue, distance, parent = [(heuristic(sx, sy), 0, sx, sy)], {start: 0}, {}
    neighbors = [(x, y, math.hypot(x, y)) for x in (-1, 0, 1) for y in (-1, 0, 1) if x or y]
    visited = 0
    while queue:
        _, cost, x, y = heapq.heappop(queue)
        if cost > distance[(x, y)] + 1e-9:
            continue
        visited += 1
        if visited > budget:
            return None
        if (x, y) == goal:
            path = [goal]
            while path[-1] != start:
                path.append(parent[path[-1]])
            return path[::-1]
        for dx, dy, length in neighbors:
            nx, ny = x + dx, y + dy
            if not (0 <= nx < w and 0 <= ny < h) or blocked[ny, nx]:
                continue
            if dx and dy and (blocked[y, nx] or blocked[ny, x]):
                continue
            value = cost + length
            if value + 1e-9 < distance.get((nx, ny), math.inf):
                distance[(nx, ny)] = value
                parent[(nx, ny)] = (x, y)
                heapq.heappush(queue, (value + heuristic(nx, ny), value, nx, ny))
    return None


def compress(points):
    result = []
    for p in points:
        if result and math.dist(p, result[-1]) < 1e-7:
            continue
        while len(result) > 1:
            a, b = result[-2:]
            cross = (b[0] - a[0]) * (p[1] - b[1]) - (b[1] - a[1]) * (p[0] - b[0])
            dot = (b[0] - a[0]) * (p[0] - b[0]) + (b[1] - a[1]) * (p[1] - b[1])
            if abs(cross) > 1e-6 or dot <= 0:
                break
            result.pop()
        result.append(list(p))
    return result


def smooth45(path, legal):
    result, index = [path[0]], 0
    while index < len(path) - 1:
        chosen = None
        for j in range(len(path) - 1, index, -1):
            a, b = path[index], path[j]
            dx, dy = b[0] - a[0], b[1] - a[1]
            sx, sy = (1 if dx >= 0 else -1), (1 if dy >= 0 else -1)
            if abs(dx) >= abs(dy):
                mids = [[b[0] - sx * abs(dy), a[1]], [a[0] + sx * abs(dy), b[1]]]
            else:
                mids = [[a[0], b[1] - sy * abs(dx)], [b[0], a[1] + sy * abs(dx)]]
            for mid in mids:
                candidate = compress([a, mid, b])
                if all(legal([p, q]) for p, q in zip(candidate, candidate[1:])):
                    chosen = (j, candidate)
                    break
            if chosen:
                break
        if chosen is None:
            raise ValueError('连续路径平滑失败')
        index, candidate = chosen
        result.extend(candidate[1:])
    return compress(result)


def via_candidates(pad):
    directions = [(-1, 0), (1, 0), (0, 1), (0, -1)]
    directions += [(x / math.sqrt(2), y / math.sqrt(2)) for x in (-1, 1) for y in (-1, 1)]
    return [[round(pad['x'] + dx * r, 4), round(pad['y'] + dy * r, 4)]
            for r in (60, 80, 100, 120, 160, 200, 240, 300, 360, 420)
            for dx, dy in directions]


def main():
    board_path, intent_path, request_path, prefix = map(Path, sys.argv[1:5])
    load = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
    board, intent, request = load(board_path), load(intent_path), load(request_path)
    model, routes, unresolved = CopperModel(board, intent), [], []
    step, width = 5, request.get('widthMil', 8)
    if not 8 <= width <= 60:
        raise ValueError('线宽超出本规划器范围')
    coordinates = np.arange(0, 3940, step, dtype=float)
    xx, yy = np.meshgrid(coordinates, coordinates)
    for net in request['nets']:
        pads = [(p, g) for p, g in model.pads if p['net'] == net]
        if net in request.get('terminals', {}):
            wanted = set(request['terminals'][net])
            pads = [(p, g) for p, g in pads if p['ref'] in wanted]
            if len(pads) != len(wanted):
                raise ValueError('请求端点缺失')
        if len(pads) < 2 or (not request.get('allowExisting', False) and any(t['net'] == net for t, _ in model.tracks)):
            raise ValueError(f'{net} 需要至少两个焊盘且没有已有走线')
        through = [p for p, _ in pads if p['layer'] == 12]
        smd = [p for p, _ in pads if p['layer'] == 1]
        if len(through) + len(smd) != len(pads):
            raise ValueError('仅支持通孔和顶层 SMD')
        root = through[0] if through else next((p for p in smd if p['ref'] == request.get('roots', {}).get(net)), smd[0])
        top, bottom = model.obstacles(net, 1), model.obstacles(net, 2)
        interior = model.inside.buffer(-(20.1 + width / 2))
        legal = lambda points, obstacles: LineString(points).distance(obstacles) >= width / 2 - 1e-6 and interior.covers(LineString(points))
        source = dict(root)
        if not through:
            root_via = next((via for via in via_candidates(root)
                             if model.via_legal(Point(*via), net)
                             and legal([[root['x'], root['y']], via], top)), None)
            if root_via is None:
                unresolved.append(net)
                continue
            root_route = {'net': net, 'from': root['ref'], 'to': root['ref'], 'via': root_via,
                          'top': [[root['x'], root['y']], root_via], 'bottom': [root_via]}
            routes.append(root_route)
            model.extra_vias.append(({'net': net, 'diameter': 24}, Point(*root_via)))
            model.extra_tracks.append(({'net': net, 'layer': 1, 'lineWidth': width}, LineString(root_route['top'])))
            source['x'], source['y'] = root_via
        for target in [p for p, _ in pads if p['ref'] != root['ref']]:
            result = route_target(model, net, source, target, top, bottom, interior, legal, step, width, xx, yy)
            if result is None:
                unresolved.append(net + ':' + target['ref'])
                continue
            routes.append(result)
            if result['via'] is not None:
                model.extra_vias.append(({'net': net, 'diameter': 24}, Point(*result['via'])))
            for layer, key in [(1, 'top'), (2, 'bottom')]:
                for a, b in zip(result[key], result[key][1:]):
                    model.extra_tracks.append(({'net': net, 'layer': layer, 'lineWidth': width}, LineString([a, b])))
            print(json.dumps({'net': net, 'to': target['ref'], 'bottomSegments': len(result['bottom']) - 1, 'via': result['via']}), flush=True)
    report = {'status': 'incomplete' if unresolved else 'offline-verified', 'routes': routes, 'unresolved': unresolved,
              'clearanceMil': 8.1, 'widthMil': width, 'gridMil': step,
              'boardSha256': hashlib.sha256(board_path.read_bytes()).hexdigest(), 'nativeDrcRequired': True}
    with Path(str(prefix) + '.json').open('x', encoding='utf-8') as f:
        json.dump(report, f, indent=2)
    if unresolved:
        raise SystemExit(1)
    steps = []
    for k, route in enumerate(routes):
        if route['via'] is not None:
            x, y = route['via']
            steps.append({'id': f'via-{k}', 'action': 'pcb.via.create', 'payload': {'x': x, 'y': y, 'net': route['net'], 'diameter': 24, 'holeDiameter': 12}})
        for layer, key in [(1, 'top'), (2, 'bottom')]:
            for j, (a, b) in enumerate(zip(route[key], route[key][1:])):
                steps.append({'id': f'escape-{k}-{layer}-{j}', 'action': 'pcb.line.create', 'payload': {'net': route['net'], 'layer': layer, 'lineWidth': width, 'startX': a[0], 'startY': a[1], 'endX': b[0], 'endY': b[1]}})
    steps.append({'id': 'save-escapes', 'action': 'pcb.save', 'assert': {'$.saved': 'true'}})
    playbook = {'version': 1, 'meta': {'name': 'SMD to bottom copper tree', 'project': intent['projectId'], 'doc': intent['documentId']}, 'steps': steps}
    with Path(str(prefix) + '.apply.json').open('x', encoding='utf-8') as f:
        json.dump(playbook, f, indent=2)


def route_target(model, net, source, target, top, bottom, interior, legal, step, width, xx, yy):
        margin = width / 2 + step / math.sqrt(2) + .02
        blocked = shapely.intersects_xy(bottom.buffer(margin), xx, yy)
        blocked |= ~shapely.contains_xy(model.inside.buffer(-(20.1 + margin)), xx, yy)
        start = (round(source['x'] / step), round(source['y'] / step))
        start_points = [[source['x'], source['y']], [start[0] * step, source['y']], [start[0] * step, start[1] * step]]
        if blocked[start[1], start[0]] or not legal(start_points, bottom):
            raise ValueError('通孔起点无法安全接网格：' + net)
        selected = None
        for via in ([[target['x'], target['y']]] if target['layer'] == 12 else via_candidates(target)):
            if target['layer'] != 12 and (not model.via_legal(Point(*via), net) or not legal([[target['x'], target['y']], via], top)):
                continue
            goal = (round(via[0] / step), round(via[1] / step))
            tail = [[goal[0] * step, goal[1] * step], [via[0], goal[1] * step], via]
            if blocked[goal[1], goal[0]] or not legal(tail, bottom):
                continue
            grid = astar(blocked, start, goal)
            if grid is None:
                continue
            path = compress(start_points + [[x * step, y * step] for x, y in grid] + tail)
            path = smooth45(path, lambda points: legal(points, bottom))
            if not all(legal([a, b], bottom) for a, b in zip(path, path[1:])):
                raise ValueError('独立连续几何复验失败')
            selected = {'net': net, 'from': source['ref'], 'to': target['ref'], 'via': via if target['layer'] != 12 else None,
                        'top': [[target['x'], target['y']], via] if target['layer'] != 12 else [], 'bottom': path}
            break
        return selected


if __name__ == '__main__':
    main()
