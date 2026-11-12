"""独立读取官方制造文件；仅在内存展开 G85，拒绝未知图元，不改原始 ZIP。"""
import hashlib
import itertools
import json
import math
from pathlib import Path
import re
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / '.work' / 'python'))
from gerbonara import ExcellonFile, GerberFile, LayerStack
from gerbonara.utils import MM
from shapely.geometry import LineString, Point, Polygon, GeometryCollection
from shapely.ops import unary_union
from shapely import affinity


def arc_points(start, end, center, clockwise):
    radius = math.dist(start, center)
    a = math.atan2(start[1] - center[1], start[0] - center[0])
    b = math.atan2(end[1] - center[1], end[0] - center[0])
    sweep = -((a - b) % math.tau) if clockwise else (b - a) % math.tau
    if math.dist(start, end) < 1e-8:
        sweep = -math.tau if clockwise else math.tau
    # 圆弧弦高小于约 1 微米，报告不声称精确解析曲线布尔运算。
    n = max(2, math.ceil(abs(sweep) / min(.05, math.sqrt(.008 / max(radius, .001)))))
    points = [(center[0] + radius * math.cos(a + sweep * i / n),
               center[1] + radius * math.sin(a + sweep * i / n)) for i in range(n + 1)]
    points[0], points[-1] = start, end
    return points


def primitive_shape(p):
    kind = type(p).__name__
    if kind == 'Circle':
        return Point(p.x, p.y).buffer(p.r, quad_segs=64)
    if kind == 'Line':
        return LineString([(p.x1, p.y1), (p.x2, p.y2)]).buffer(p.width / 2, quad_segs=64)
    if kind == 'Arc':
        return LineString(arc_points((p.x1, p.y1), (p.x2, p.y2), (p.cx, p.cy), p.clockwise)).buffer(p.width / 2)
    if kind == 'Rectangle':
        shape = Polygon([(-p.w / 2, -p.h / 2), (p.w / 2, -p.h / 2),
                         (p.w / 2, p.h / 2), (-p.w / 2, p.h / 2)])
        return affinity.translate(affinity.rotate(shape, p.rotation, use_radians=True), p.x, p.y)
    if kind == 'ArcPoly':
        points = []
        for a, b, (cw, center) in p.segments:
            points.extend([a] if cw is None else arc_points(a, b, center, cw)[:-1])
        shape = Polygon(points)
        if not shape.is_valid:
            # 字体轮廓常用自接触路径表示孔洞；按 even-odd 语义修复。
            shape = shape.buffer(0)
        return shape
    raise ValueError('未支持图元：' + kind)


def layer_shape(layer):
    result = GeometryCollection()
    primitives = (p for obj in layer.objects for p in obj.to_primitives(MM))
    for dark, group in itertools.groupby(primitives, lambda p: p.polarity_dark):
        shape = unary_union([primitive_shape(p) for p in group])
        result = result.union(shape) if dark else result.difference(shape)
    return result


def read_drill(raw):
    pattern = r'^(X[^\nG]+)G85(X[^\n]+)$'
    expanded, slots = re.subn(pattern, r'G00\1\nM15\nG01\2\nM16\nG05', raw, flags=re.M)
    return ExcellonFile.from_string(expanded, plated=True), slots


def main():
    archive_path, board_path, output = map(Path, sys.argv[1:4])
    output.mkdir(exist_ok=False)
    board = json.loads(board_path.read_text(encoding='utf-8-sig'))
    mapping = {'Gerber_TopLayer.GTL': ('top', 'copper'), 'Gerber_BottomLayer.GBL': ('bottom', 'copper'),
               'Gerber_InnerLayer1.G1': ('inner_1', 'copper'), 'Gerber_InnerLayer2.G2': ('inner_2', 'copper'),
               'Gerber_TopSilkscreenLayer.GTO': ('top', 'silk'), 'Gerber_BottomSilkscreenLayer.GBO': ('bottom', 'silk'),
               'Gerber_TopSolderMaskLayer.GTS': ('top', 'mask'), 'Gerber_BottomSolderMaskLayer.GBS': ('bottom', 'mask'),
               'Gerber_TopPasteMaskLayer.GTP': ('top', 'paste'), 'Gerber_BoardOutlineLayer.GKO': ('mechanical', 'outline')}
    with zipfile.ZipFile(archive_path) as z:
        assert z.testzip() is None
        layers = {target: GerberFile.from_string(z.read(source).decode()) for source, target in mapping.items()}
        drill, slots = read_drill(z.read('Drill_PTH_Through.DRL').decode())
        via_drill, _ = read_drill(z.read('Drill_PTH_Through_Via.DRL').decode())
        inventory = {n: {'bytes': len(z.read(n)), 'sha256': hashlib.sha256(z.read(n)).hexdigest()} for n in z.namelist()}
    shapes = {key: layer_shape(layer) for key, layer in layers.items() if key != ('mechanical', 'outline')}
    tolerant_copper = {side: shapes[(side, 'copper')].buffer(.003) for side in ('top', 'bottom')}
    track_missing = []
    for track in board['copper']['lines']:
        side = {1: 'top', 2: 'bottom'}[track['layer']]
        centerline = LineString([(track['startX'] * .0254, track['startY'] * .0254),
                                 (track['endX'] * .0254, track['endY'] * .0254)])
        if not tolerant_copper[side].covers(centerline):
            track_missing.append(track['primitiveId'])
    plane_checks = {}
    for side in ('inner_1', 'inner_2'):
        polygons = list(getattr(shapes[(side, 'copper')], 'geoms', [shapes[(side, 'copper')]]))
        main_plane = max(polygons, key=lambda p: p.area)
        missed_ground, shorted_signal = [], []
        for via in board['copper']['vias']:
            in_plane = main_plane.buffer(.001).covers(Point(via['x'] * .0254, via['y'] * .0254))
            if via['net'] == 'GND' and not in_plane:
                missed_ground.append(via['primitiveId'])
            if via['net'] != 'GND' and in_plane:
                shorted_signal.append(via['primitiveId'])
        plane_checks[side] = {'mainAreaMm2': main_plane.area, 'missingGroundVia': missed_ground,
                             'signalViaTouchesMainPlane': shorted_signal}
    stack = LayerStack(layers, drill_pth=drill)
    for side in ('top', 'bottom'):
        (output / f'pcb-{side}.svg').write_text(str(stack.to_pretty_svg(side=side)), encoding='utf8')
    drill_centers = [(obj.x, obj.y) for obj in drill.objects if type(obj).__name__ == 'Flash']
    via_centers = [(obj.x, obj.y) for obj in via_drill.objects if type(obj).__name__ == 'Flash']
    expected = [(v['x'] * .0254, v['y'] * .0254) for v in board['copper']['vias']]
    expected += [(p['x'] * .0254, p['y'] * .0254) for c in board['components'] for p in c['pads'] if p['layer'] == 12 and p.get('hole', [None])[0] == 'ROUND']
    # SDK 的部分 hole 尺寸有比例问题，直径从制造文件与原厂封装核对，不用 SDK 猜孔径。
    missing = [p for p in expected if min(math.dist(p, q) for q in drill_centers) > .002]
    extra = [p for p in drill_centers if min(math.dist(p, q) for q in expected) > .002]
    via_missing = [p for p in expected[:len(board['copper']['vias'])] if min(math.dist(p, q) for q in via_centers) > .002]
    silk = {}
    for side in ('top', 'bottom'):
        collision = shapes[(side, 'silk')].intersection(shapes[(side, 'mask')])
        silk[side] = {'maskOverlapAreaMm2': collision.area, 'overlaps': [list(g.bounds) for g in getattr(collision, 'geoms', [collision]) if g.area > .0001]}
    # 外框按连通的中心线分成五个闭环：一条外边与四个铣孔。
    segments = []
    for obj in layers[('mechanical', 'outline')].objects:
        if type(obj).__name__ == 'Line':
            points = [(obj.x1, obj.y1), (obj.x2, obj.y2)]
        elif type(obj).__name__ == 'Arc':
            points = arc_points((obj.x1, obj.y1), (obj.x2, obj.y2), (obj.x1 + obj.cx, obj.y1 + obj.cy), obj.clockwise)
        else:
            raise ValueError('未知板框图元')
        segments.append(LineString(points))
    from shapely.ops import polygonize
    loops = sorted(polygonize(unary_union(segments)), key=lambda p: p.area, reverse=True)
    # polygonize 外环可能把内环作为孔；面积和包络分别记录。
    report = {'archiveSha256': hashlib.sha256(archive_path.read_bytes()).hexdigest(),
              'boardSha256': hashlib.sha256(board_path.read_bytes()).hexdigest(), 'files': inventory,
              'layers': {f'{k[0]}-{k[1]}': {'objects': len(layers[k].objects), 'areaMm2': s.area, 'boundsMm': s.bounds} for k, s in shapes.items()},
              'drill': {'roundCount': len(drill_centers), 'viaCount': len(via_centers), 'slots': slots,
                        'sizesMm': sorted(drill.drill_sizes()), 'missing': missing, 'extra': extra, 'viaMissing': via_missing,
                        'viaFileIsSubset': all(min(math.dist(p, q) for q in drill_centers) < .002 for p in via_centers)},
              'outlineLoops': [{'boundsMm': p.bounds, 'areaMm2': p.area} for p in loops], 'silkscreen': silk,
              'copper': {'missingTrackCenterlines': track_missing, 'planes': plane_checks},
              'limitations': ['圆弧为弦高约 1µm 的离散几何；字形自接触路径用 buffer(0) 归一化。',
                              'G85 仅在内存转换为等价铣槽指令；原始 ZIP 未修改。',
                              '钻孔中心按 PCB 快照逐一核对，孔径独立记录；本报告不替代板厂 CAM 或首板实测。']}
    report['scope'] = '板框闭环、钻孔中心及槽数量、走线中心线导出覆盖和两内层主地铜过孔接触；丝印交叠另行复核'
    report['passed'] = not (missing or extra or via_missing or track_missing) and slots == 4 and len(loops) == 5
    report['passed'] &= all(not (p['missingGroundVia'] or p['signalViaTouchesMainPlane']) for p in plane_checks.values())
    (output / 'manufacturing-inspection.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf8')
    print(json.dumps({k: v for k, v in report.items() if k not in ('files', 'layers')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
