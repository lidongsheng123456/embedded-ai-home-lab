"""把已删除 USB-A 的供电支路改接开发板母座；规划顶底层铜，不写编辑器。"""
import copy
import hashlib
import json
from pathlib import Path
import sys
from collections import deque

from pcb_geometry import CopperModel
from importlib import import_module

# 共用既有寻路与连续几何复验，避免维护两套铜间距算法。
router = import_module("plan-bottom-escapes")


def reachable(blocked, start):
    """保守网格连通区只筛选候选，最终仍由连续铜几何重新检查。"""
    height, width = blocked.shape
    if blocked[start[1], start[0]]:
        return set()
    seen, queue = {start}, deque([start])
    while queue:
        x, y = queue.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            point = (x + dx, y + dy)
            nx, ny = point
            if 0 <= nx < width and 0 <= ny < height and point not in seen and not blocked[ny, nx]:
                seen.add(point)
                queue.append(point)
    return seen


def route_with_hop(model, net, source, endpoint, top, bottom, interior, legal, step, width, xx, yy):
    """从母座顶层出发，新增一个过孔接入原电源底层区域，不改其他网络。"""
    margin = width / 2 + step / router.math.sqrt(2) + .02
    inside = router.shapely.contains_xy(model.inside.buffer(-(20.1 + margin)), xx, yy)
    top_blocked = router.shapely.intersects_xy(top.buffer(margin), xx, yy) | ~inside
    bottom_blocked = router.shapely.intersects_xy(bottom.buffer(margin), xx, yy) | ~inside
    start = (round(source['x'] / step), round(source['y'] / step))
    nearest = (round(endpoint['x'] / step), round(endpoint['y'] / step))
    # 已有过孔不一定正好落在网格上；从附近可达节点用真实净距检查短尾线。
    goals = sorted(((nearest[0] + dx, nearest[1] + dy)
                    for dx in range(-3, 4) for dy in range(-3, 4)),
                   key=lambda p: router.math.hypot(p[0] * step - endpoint['x'], p[1] * step - endpoint['y']))
    goal = next((p for p in goals if not bottom_blocked[p[1], p[0]]
                 and legal([[p[0] * step, p[1] * step], [endpoint['x'], endpoint['y']]], bottom)), None)
    if goal is None:
        return None
    waypoint = dict(endpoint, x=goal[0] * step, y=goal[1] * step)
    top_area, bottom_area = reachable(top_blocked, start), reachable(bottom_blocked, goal)
    shared = top_area & bottom_area
    print(json.dumps({'topReachable': len(top_area), 'bottomReachable': len(bottom_area),
                      'sharedViaGridPoints': len(shared)}), flush=True)
    # 优先靠近已有电源过孔，减少底层占用；候选数量有上限，失败不写板。
    candidates = sorted(shared, key=lambda p: router.math.hypot(p[0] - goal[0], p[1] - goal[1]))[:300]
    for x, y in candidates:
        hop = {'ref': 'candidate-via', 'x': x * step, 'y': y * step, 'layer': 12}
        if not model.via_legal(router.Point(hop['x'], hop['y']), net, radius=16):
            continue
        a = router.route_target(model, net, source, hop, top, top, interior, legal, step, width, xx, yy)
        b = router.route_target(model, net, hop, waypoint, top, bottom, interior, legal, step, width, xx, yy)
        if a and b:
            tail = b['bottom'] + [[endpoint['x'], endpoint['y']]]
            tail = router.smooth45(tail, lambda points: legal(points, bottom))
            return {'from': source['ref'], 'to': endpoint['ref'], 'via': [hop['x'], hop['y']],
                    'top': a['bottom'], 'bottom': tail}
    return None


def main():
    board_path, intent_path, prefix = map(Path, sys.argv[1:4])
    load = lambda p: json.loads(p.read_text(encoding="utf-8-sig"))
    board, intent = load(board_path), load(intent_path)
    if any(c["designator"] == "J11" for c in board["components"]):
        raise ValueError("先从原理图正式删除旧 USB-A，并同步保存重开 PCB")
    net = "USB_MCU_5V"
    pads = [p for c in board["components"] if c["designator"] == "J7"
            for p in c["pads"] if p["padNumber"] == "21"]
    vias = [v for v in board["copper"]["vias"] if v["net"] == net]
    old = [t for t in board["copper"]["lines"] if t["net"] == net and t["layer"] == 2]
    if len(pads) != 1 or pads[0]["net"] != net or len(vias) != 1 or len(old) != 5:
        raise ValueError("母座网络、已有换层过孔或旧支路库存与本次基线不符")
    # 只删除旧 USB-A 的五段底层铜，U10→电容和顶层换层短线保持。
    projected = copy.deepcopy(board)
    removed_ids = {t["primitiveId"] for t in old}
    projected["copper"]["lines"] = [t for t in board["copper"]["lines"]
                                     if t["primitiveId"] not in removed_ids]
    projected["projection"] = {"kind": "candidate-after-obsolete-branch-removal",
                               "source": str(board_path), "removedTracks": sorted(removed_ids)}
    model = CopperModel(projected, intent)
    width, step = 40, 5
    source = dict(pads[0], ref="J7.21")
    via = vias[0]
    # 这是已存在的真实过孔端点，不在 PCB 快照中伪造焊盘或新增过孔。
    endpoint = {"ref": "existing-via:" + via["primitiveId"], "x": via["x"],
                "y": via["y"], "layer": 12}
    top, bottom = model.obstacles(net, 1), model.obstacles(net, 2)
    interior = model.inside.buffer(-(20.1 + width / 2))
    legal = lambda points, obstacles: (
        router.LineString(points).distance(obstacles) >= width / 2 - 1e-6
        and interior.covers(router.LineString(points)))
    coordinates = router.np.arange(0, 3940, step, dtype=float)
    xx, yy = router.np.meshgrid(coordinates, coordinates)
    route = router.route_target(model, net, source, endpoint, top, bottom,
                                interior, legal, step, width, xx, yy)
    if route is None:
        route = route_with_hop(model, net, source, endpoint, top, bottom,
                               interior, legal, step, width, xx, yy)
    if route is None:
        raise ValueError("有限搜索中没有找到符合净距的40mil路径，未生成写入队列")
    steps = [{"id": "remove-obsolete-usb-branch", "action": "pcb.route.delete",
              "payload": {"kind": "track", "primitiveIds": sorted(removed_ids)}}]
    new_via = route.get("via")
    if new_via:
        steps.append({"id": "header-supply-via", "action": "pcb.via.create",
                      "payload": {"net": net, "x": new_via[0], "y": new_via[1],
                                  "diameter": 32, "holeDiameter": 16}})
        for i, (a, b) in enumerate(zip(route["top"], route["top"][1:])):
            steps.append({"id": f"header-top-{i}", "action": "pcb.line.create",
                          "payload": {"net": net, "layer": 1, "lineWidth": width,
                                      "startX": a[0], "startY": a[1], "endX": b[0], "endY": b[1]}})
    for i, (a, b) in enumerate(zip(route["bottom"], route["bottom"][1:])):
        steps.append({"id": f"header-power-{i}", "action": "pcb.line.create",
                      "payload": {"net": net, "layer": 2, "lineWidth": width,
                                  "startX": a[0], "startY": a[1], "endX": b[0], "endY": b[1]}})
    steps.append({"id": "save-header-power", "action": "pcb.save", "assert": {"$.saved": "true"}})
    report = {"status": "offline-verified", "boardSha256": hashlib.sha256(board_path.read_bytes()).hexdigest(),
              "scope": "40mil母座供电候选；不覆盖整板DRC或实物供电", "removedTracks": old,
              "reusedVia": via,
              "route": route, "newVias": int(bool(new_via)), "clearanceMil": 8.1,
              "widthMil": width, "nativeDrcRequired": True}
    playbook = {"version": 1, "meta": {"name": "开发板母座供电支路",
                "project": intent["projectId"], "doc": intent["documentId"]}, "steps": steps}
    for suffix, data in [(".projected.json", projected), (".json", report), (".apply.json", playbook)]:
        with Path(str(prefix) + suffix).open("x", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    print(json.dumps({"removed": len(old), "segments": len(steps) - 2, "newVias": int(bool(new_via))}))


if __name__ == "__main__":
    main()
