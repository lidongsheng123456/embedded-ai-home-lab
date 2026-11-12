"""真实 PCB 快照的保守铜包络；圆角矩形取外包矩形，椭圆取外切多边形。"""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / '.work' / 'python'))
from shapely import affinity
from shapely.geometry import LineString, Point, Polygon, box
from shapely.ops import unary_union


def linear_polygon(source):
    if source[2] != 'L' or not all(isinstance(v, (int, float)) for v in source[3:]):
        raise ValueError('仅支持完整线性轮廓')
    values = source[:2] + source[3:]
    polygon = Polygon(list(zip(values[::2], values[1::2])))
    if not polygon.is_valid or polygon.is_empty:
        raise ValueError('无效多边形')
    return polygon


def pad_shape(pad):
    if pad['availability']['shape'] != 'available' or pad['specialPadPresent']:
        raise ValueError('焊盘形状缺测或存在特殊层')
    shape = pad['shape']
    kind = shape[0]
    if kind == 'POLYGON':
        return linear_polygon(shape[1])
    w, h = shape[1:3]
    if kind == 'RECT':
        geom = box(-w / 2, -h / 2, w / 2, h / 2)
    elif kind == 'OVAL':
        # 外切圆弧多边形避免离散后误增净距。
        r = min(w, h) / 2
        segment = LineString([(-(w - h) / 2, 0), ((w - h) / 2, 0)]) if w >= h else LineString([(0, -(h - w) / 2), (0, (h - w) / 2)])
        geom = segment.buffer(r / math.cos(math.pi / 128), quad_segs=32)
    elif kind == 'ELLIPSE':
        geom = affinity.scale(Point(0, 0).buffer(1 / math.cos(math.pi / 128), quad_segs=32), w / 2, h / 2)
    else:
        raise ValueError('未支持焊盘：' + kind)
    return affinity.translate(affinity.rotate(geom, pad.get('rotation', 0), origin=(0, 0)), pad['x'], pad['y'])


class CopperModel:
    def __init__(self, board, intent):
        capture = board['capture']
        if not capture['identityVerified'] or capture['projectUuid'] != intent['projectId'] or capture['documentUuid'] != intent['documentId']:
            raise ValueError('快照身份不符')
        copper = board['copper']
        if any(v != 'available' for v in copper['availability'].values()) or copper['arcs']:
            raise ValueError('铜缺测或存在未实现圆弧走线')
        if any(p['layer'] in (1, 2) for p in copper['pours']):
            raise ValueError('本规划器不支持外层铺铜')
        self.board = board
        self.pads = [(dict(p, ref=f"{c['designator']}.{p['padNumber']}"), pad_shape(p)) for c in board['components'] for p in c['pads']]
        self.tracks = [(t, LineString([(t['startX'], t['startY']), (t['endX'], t['endY'])])) for t in copper['lines']]
        self.vias = [(v, Point(v['x'], v['y'])) for v in copper['vias']]
        self.regions = [(r, linear_polygon(r['source'])) for r in copper['regions'] if 5 in r['ruleType']]
        self.cuts = [(f, linear_polygon(f['source'])) for f in copper['fills'] if f['layer'] == 12]
        if any(f['layer'] in (1, 2) for f in copper['fills']):
            raise ValueError('本规划器不支持外层填充铜')
        self.inside = Polygon(board['outline']['points'])
        self.extra_tracks = []
        self.extra_vias = []

    def obstacles(self, net, layer, clearance=8.1):
        shapes = [g.buffer(clearance) for p, g in self.pads if p['net'] != net and p['layer'] in (layer, 12)]
        shapes += [g.buffer(t['lineWidth'] / 2 + clearance) for t, g in self.tracks + self.extra_tracks if t['net'] != net and t['layer'] == layer]
        shapes += [g.buffer(v['diameter'] / 2 + clearance) for v, g in self.vias + self.extra_vias if v['net'] != net]
        shapes += [g.buffer(r['lineWidth'] / 2) for r, g in self.regions if r['layer'] in (layer, 12)]
        shapes += [g.buffer(f['lineWidth'] / 2 + 20.1) for f, g in self.cuts]
        return unary_union(shapes)

    def via_legal(self, point, net, radius=12):
        if not self.inside.buffer(-(20.1 + radius)).covers(point):
            return False
        if any(point.distance(g) < radius + (4 if p['net'] == net else 8.1) for p, g in self.pads):
            return False
        if any(point.distance(g) < radius + t['lineWidth'] / 2 + 8.1 for t, g in self.tracks + self.extra_tracks if t['net'] != net):
            return False
        if any(point.distance(g) < radius + v['diameter'] / 2 + 8.1 for v, g in self.vias + self.extra_vias):
            return False
        if any(point.distance(g) < radius + r['lineWidth'] / 2 for r, g in self.regions):
            return False
        return all(point.distance(g) >= radius + f['lineWidth'] / 2 + 20.1 for f, g in self.cuts)
