"""保留官方原始表，并按实际 PCB 逐项核对后生成标准逗号分隔装配表。"""
import csv
import hashlib
import io
import json
from pathlib import Path
import shutil
import sys


def read_table(path):
    raw = path.read_bytes()
    encoding = 'utf-16' if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig'
    with io.StringIO(raw.decode(encoding), newline='') as stream:
        sample = stream.read(2048)
        stream.seek(0)
        return list(csv.DictReader(stream, delimiter='\t' if '\t' in sample.splitlines()[0] else ','))


def write_table(path, fields, rows):
    with path.open('x', encoding='utf-8-sig', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    bom_path, board_path, out = map(Path, sys.argv[1:4])
    board = json.loads(board_path.read_text(encoding='utf-8-sig'))
    parts = {c['designator']: c for c in board['components']}
    bom, placement = read_table(bom_path), read_table(out / 'placement-official.csv')
    assert len(placement) == len(parts) == 79
    assert {r['Designator'] for r in placement} == set(parts)
    bom_parts = {}
    for row in bom:
        refs = row['Designator'].split(',')
        assert int(row['Quantity']) == len(refs)
        assert row['Supplier Part'].startswith('C') and row['Supplier Part'][1:].isdigit()
        for ref in refs:
            assert ref not in bom_parts
            assert row['Manufacturer Part'] == parts[ref]['device']
            bom_parts[ref] = row
    assert set(bom_parts) == set(parts)
    mid_offsets = []
    for row in placement:
        part = parts[row['Designator']]
        number = lambda key: float(row[key].removesuffix('mm'))
        assert abs(number('Ref X') - part['x'] * .0254) < .002
        assert abs(number('Ref Y') - part['y'] * .0254) < .002
        delta = (float(row['Rotation']) - part['rotation'] + 180) % 360 - 180
        assert abs(delta) < .01
        assert row['Layer'] == 'T' and part['layer'] == 1
        if abs(number('Mid X') - number('Ref X')) > .002 or abs(number('Mid Y') - number('Ref Y')) > .002:
            mid_offsets.append({'ref': row['Designator'], 'dxMm': number('Mid X') - number('Ref X'),
                                'dyMm': number('Mid Y') - number('Ref Y')})
    fields = ['Comment', 'Designator', 'Footprint', 'Quantity', 'LCSC Part #', 'Manufacturer Part']
    rows = [dict(zip(fields, [r['Comment'], r['Designator'], r['Footprint'], r['Quantity'],
                             r['Supplier Part'], r['Manufacturer Part']])) for r in bom]
    write_table(out / 'bom-all.csv', fields, rows)
    smd = {r['Designator'] for r in placement if r['SMD'] == 'Yes'}
    smd_rows = []
    for row in rows:
        refs = [ref for ref in row['Designator'].split(',') if ref in smd]
        if refs:
            smd_rows.append(dict(row, Designator=','.join(refs), Quantity=len(refs)))
    write_table(out / 'bom-smd.csv', fields, smd_rows)
    cpl_fields = ['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation']
    write_table(out / 'placement-smd.csv', cpl_fields,
                [{key: ('Top' if key == 'Layer' else row[key]) for key in cpl_fields}
                 for row in placement if row['Designator'] in smd])
    shutil.copy2(bom_path, out / 'bom-official.tsv')
    report = {'passed': True, 'parts': len(parts), 'bomGroups': len(bom), 'smdParts': len(smd),
              'throughHoleParts': sorted(set(parts) - smd), 'midCenterOffsets': mid_offsets,
              'scope': 'BOM 位号、数量、实际 MPN/C 编号完整性；坐标参考点、板面、旋转与新鲜 PCB 逐项对账。吸取中心保留官方输出，贴片厂需核对极性及角度。',
              'sources': {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in [bom_path, board_path, out / 'placement-official.csv']}}
    (out / 'assembly-verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
