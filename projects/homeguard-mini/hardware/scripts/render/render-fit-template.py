"""按毫米生成插合模板；只表达实测器件包络、针中心和安装孔，不替代铜层制造图。"""
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / '.work' / 'python'))
import pymupdf


def main():
    board_path, intent_path, output = map(Path, sys.argv[1:4])
    if output.exists():
        raise ValueError('输出已存在，拒绝覆盖')
    board = json.loads(board_path.read_text(encoding='utf-8-sig'))
    intent = json.loads(intent_path.read_text(encoding='utf-8-sig'))
    if not board['capture']['identityVerified'] or board['capture']['documentUuid'] != intent['documentId']:
        raise ValueError('快照身份不符')
    doc = pymupdf.open()
    page = doc.new_page(width=595.276, height=841.89)
    scale = 72 / 25.4
    origin_x, origin_y = 25 * scale, 45 * scale
    height = intent['board']['height']

    def point(x, y):
        return pymupdf.Point(origin_x + x * scale, origin_y + (height - y) * scale)

    def text(x, y, value, size=8, chinese=False):
        page.insert_text((x * scale, y * scale), value, fontsize=size,
                         fontname='china-s' if chinese else 'helv', color=(.12, .18, .22))

    text(25, 17, 'HOMEGUARD MINI / R1 / FIT TEMPLATE', 14)
    text(25, 24, '请按“实际大小 / 100%”打印，关闭适应页面；先量下方 50 mm 标尺。', 9, True)
    text(25, 30, '核对开发板双排母座、USB 直角头、模块插头和安装螺丝；未完成实物验证。', 9, True)
    outline = [point(x * .0254, y * .0254) for x, y in board['outline']['points']]
    page.draw_polyline(outline + outline[:1], color=(.1, .2, .25), width=.5)
    for component in board['components']:
        box = component['bbox']
        rect = pymupdf.Rect(point(box['minX'] * .0254, box['maxY'] * .0254),
                            point(box['maxX'] * .0254, box['minY'] * .0254))
        page.draw_rect(rect, color=(.6, .68, .72), width=.2)
        if component['designator'].startswith('J'):
            page.insert_text(rect.top_left + (0, -2), component['designator'], fontsize=6)
        for pad in component['pads']:
            if pad['layer'] == 12:
                center = point(pad['x'] * .0254, pad['y'] * .0254)
                page.draw_circle(center, .16 * scale, color=(.1, .1, .1), fill=(.1, .1, .1), width=.1)
    for cut in board['copper']['fills']:
        if cut['layer'] != 12:
            continue
        source = cut['source']
        if source[2] != 'L' or not all(isinstance(v, (int, float)) for v in source[3:]):
            raise ValueError('铣孔不是预期线性轮廓')
        numbers = source[:2] + source[3:]
        points = [point(numbers[k] * .0254, numbers[k + 1] * .0254) for k in range(0, len(numbers), 2)]
        page.draw_polyline(points + points[:1], color=(.1, .1, .1), width=.3)
    keepout = intent['mechanical']['devboardKeepout']
    page.draw_rect(pymupdf.Rect(point(keepout['minX'], keepout['maxY']),
                              point(keepout['maxX'], keepout['minY'])),
                   color=(.25, .4, .5), dashes='[3 2]', width=.5)
    text(133, 61, '100 x 100 mm / R3', 9)
    text(133, 69, '4-layer FR-4', 9)
    text(133, 77, 'J7-J8: 25.40 mm', 9)
    text(133, 85, '22 x 2.54 mm / row', 9)
    text(133, 96, 'M3 cutouts: ~3.2 mm', 9)
    text(133, 110, 'TOP VIEW', 11)
    text(25, 157, '虚线为开发板预留包络；黑点为通孔焊盘中心，不表示钻孔直径。', 9, True)
    text(25, 164, '模板用于机械套合；制造仍以最终 Gerber 和钻孔文件为准。', 9, True)
    start, end = pymupdf.Point(25 * scale, 181 * scale), pymupdf.Point(75 * scale, 181 * scale)
    page.draw_line(start, end, width=.7)
    for x in [25, 75]:
        page.draw_line((x * scale, 179 * scale), (x * scale, 183 * scale), width=.7)
    text(40, 188, '50.00 mm', 10)
    text(25, 203, 'Readback SHA256:', 7)
    digest = hashlib.sha256(board_path.read_bytes()).hexdigest()
    text(25, 209, digest, 6)
    text(25, 222, '载板要求：USB-C 直角头外露不超过 9 mm；USB-A 护套不超过 18 x 28 mm。', 9, True)
    text(25, 229, '本设备用于学习和辅助提醒，不能替代认证烟雾、燃气或一氧化碳报警器。', 9, True)
    doc.save(output, garbage=4, deflate=True)
    print(json.dumps({'output': str(output), 'pageMm': [210, 297], 'boardMm': [100, 100],
                      'scaleBarMm': 50, 'hardwareTested': False}, ensure_ascii=False))


if __name__ == '__main__':
    main()
