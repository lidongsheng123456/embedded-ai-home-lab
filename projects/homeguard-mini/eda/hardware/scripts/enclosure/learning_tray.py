"""生成用券尺寸内的学习保护底座；保留原载板和顶部插拔空间。"""
import argparse
import hashlib
import json
import math
from pathlib import Path

import manifold3d as m
import numpy as np
import trimesh

from build import export_mesh, read_board, read_json, write_json
import solids as s


def make_tray(config, positions):
    c=config['case'];p=config['pcb'];f=config['fasteners'];x,y=c['center']
    # 不做围住100mm板的侧壁，否则模型必然超过免费券的100mm上限。
    tray=s.rounded(x,y,0,c['width'],c['height'],c['floor'],c['radius'])
    cuts=[]
    for sx,sy in p['mounts']:
        tray += s.cylinder(sx,sy,c['floor']-.1,p['pillarDiameter'],p['standoff']+.1)
        cuts.append(s.cylinder(sx,sy,-.1,f['clearanceDiameter'],c['floor']+p['standoff']+.2))
        # 普通六角螺母从底面放入；通孔配机牙螺丝，避免自攻使树脂开裂。
        radius=f['nutAcrossFlats']/math.sqrt(3)
        nut=m.Manifold.cylinder(f['nutPocketDepth']+.1,radius,circular_segments=6)
        cuts.append(nut.translate((sx,sy,-.1)))
    return s.subtract(tray,cuts)


def board_slab(config):
    p=config['pcb'];z=config['case']['floor']+p['standoff']
    slab=s.rounded(50,50,z,*p['size'],p['thickness'],3)
    return s.subtract(slab,[s.cylinder(x,y,z-.1,3.2,p['thickness']+.2) for x,y in p['mounts']])


def check_geometry(tray, board, parts, config, positions):
    pcb=board_slab(config);checks=[]
    surface=config['case']['floor']+config['pcb']['standoff']+config['pcb']['thickness']
    for ref,component in parts.items():
        b=component['bbox'];x0,y0,x1,y1=[b[k]*.0254 for k in ('minX','minY','maxX','maxY')]
        if x1<=x0 or y1<=y0:raise ValueError(f'{ref}包络不可用')
        solid=s.box((x0+x1)/2,(y0+y1)/2,surface,x1-x0,y1-y0,config['componentEnvelopeHeight'])
        checks.append((ref,solid))
        if ref in config['accessRefs']:
            checks.append((ref+'插拔通道',s.box((x0+x1)/2,(y0+y1)/2,surface,
                           x1-x0+1.2,y1-y0+1.2,50)))
    checks.append(('PCB',pcb))
    # 焊脚包络避开四个安装孔禁布区域；深度是假设，单独写入交付限制。
    bottom=surface-config['pcb']['thickness']
    underside=s.box(50,50,bottom-config['undersideAllowance'],100,100,config['undersideAllowance'])
    underside=s.subtract(underside,[s.cylinder(x,y,0,config['pcb']['pillarDiameter']+.4,bottom+.1)
                                    for x,y in config['pcb']['mounts']])
    checks.append(('底面焊脚假设包络',underside))
    ux,uy=positions[config['usb']['ref']];c=config['case'];edge=c['center'][1]-c['height']/2
    checks.append(('USB插头假设通道',s.box(ux,(edge-6+uy)/2,config['usb']['openingBottom'],
                   config['usb']['openingWidth']-1,uy-edge+6,20)))
    overlaps=[{'object':name,'overlapMm3':(tray^solid).volume()} for name,solid in checks]
    if any(r['overlapMm3']>1e-5 for r in overlaps):raise ValueError(json.dumps(overlaps,ensure_ascii=False))
    return pcb,overlaps


def verify_connected(path):
    mesh=trimesh.load_mesh(path,process=True);parent=list(range(len(mesh.faces)))
    def find(i):
        while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
        return i
    for a,b in mesh.face_adjacency:parent[find(int(a))]=find(int(b))
    if len({find(i) for i in range(len(parent))})!=1:raise ValueError('STL存在不相连网格')
    if abs(mesh.bounds[0,2])>.001:raise ValueError('打印文件未放在Z=0')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('board','intent','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists():raise FileExistsError('请使用新输出目录，避免覆盖现有模型')
    config=read_json(args.intent);board,parts,positions=read_board(args.board,config)
    tray=make_tray(config,positions);pcb,checks=check_geometry(tray,board,parts,config,positions)
    args.out.mkdir(parents=True)
    bounds=np.array(tray.bounding_box()).reshape(2,3)
    printable=tray.translate(tuple(-bounds[0]))
    report=export_mesh(printable,args.out/'learning-base.stl')
    verify_connected(args.out/'learning-base.stl')
    size=[b-a for a,b in zip(*report['boundsMm'])]
    size_ok=all(s<=v for s,v in zip(size,config['coupon']['maxSizeMm']))
    volume_ok=report['volumeMm3']/1000<=config['coupon']['maxTotalVolumeCm3']
    if not size_ok or not volume_ok:raise ValueError('底座超过免费券尺寸或体积上限')
    manifest={'status':'candidate-unverified','geometryChecks':'offline-verified',
              'revision':config['revision'],'unit':'mm','sizeMm':size,'file':report,
              'boardSha256':hashlib.sha256(args.board.read_bytes()).hexdigest(),
              'intentSha256':hashlib.sha256(args.intent.read_bytes()).hexdigest(),
              'pcbCapture':board.get('capture'),'checkedObjects':checks,
              'material':config['material'],'coupon':{**config['coupon'],
              'sizeWithinLimit':size_ok,'volumeWithinLimit':volume_ok,
              'volumeCm3':report['volumeMm3']/1000},'limitations':config['limitations']}
    write_json(args.out/'manifest.json',manifest)
    from tray_preview import render
    render(tray,pcb,parts,config,args.out)
    print(json.dumps({'out':str(args.out),'sizeMm':size,'volumeCm3':report['volumeMm3']/1000,
                      'checkedObjects':len(checks),'couponGeometryWithinLimits':True,
                      'couponApplicable':'pending-platform-review'},ensure_ascii=False))


if __name__=='__main__':main()
