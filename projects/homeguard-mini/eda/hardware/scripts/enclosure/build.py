"""从当次PCB回读和可编辑尺寸参数生成结构候选，不修改EDA工程。"""
import argparse
import hashlib
import json
import re
from pathlib import Path

import numpy as np
import trimesh
import solids as s


def read_json(path):
    data=json.loads(path.read_text(encoding='utf-8-sig'))
    return data.get('result',data)


def write_json(path, data):
    path.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf8')


def read_board(path, config):
    board=read_json(path);points=np.array(board['outline']['points'])*.0254
    size=points.max(axis=0)-points.min(axis=0)
    if not np.allclose(size,config['pcb']['size'],atol=.02):
        raise ValueError(f'PCB尺寸已变化：{size}，请重新检查机械参数')
    if not np.allclose(points.min(axis=0),[0,0],atol=.02):
        raise ValueError('PCB原点已变化，请重定安装孔和模块位置')
    parts={c['designator']:c for c in board['components']}
    for ref in ('J10','SW1','J12','J5','J14','J15','J16','J17','J18','J7','J8'):
        if ref not in parts:raise ValueError(f'缺少当前板器件 {ref}')
    positions={ref:[parts[ref][axis]*.0254 for axis in ('x','y')] for ref in parts}
    return board,parts,positions


def component_clearances(parts, models, config):
    model_map={c['designator']:c for c in models['components']}
    z=config['case']['rear']+config['pcb']['standoff']+config['pcb']['thickness']
    keepouts=[];heights=[]
    for ref,c in parts.items():
        name=(model_map.get(ref,{}).get('model') or {}).get('name','')
        match=re.search(r'(?:^|[-_])H(\d+(?:\.\d+)?)',name)
        height=config['componentHeights'].get(ref)
        source='provisional-clearance'
        if height is None and match:height=float(match[1]);source='model-name'
        if height is None:height=3;source='unknown-default'
        b=c['bbox'];x0,y0,x1,y1=[b[k]*.0254 for k in ('minX','minY','maxX','maxY')]
        keepouts.append((ref,s.box((x0+x1)/2,(y0+y1)/2,z,x1-x0,y1-y0,height)))
        heights.append({'ref':ref,'heightMm':height,'source':source})
    # 主控板并非载板上的实体器件，另外加入已知板形和假设高度。
    keepouts.append(('ESP32_MODULE_ASSUMED',s.box(50,50,z,29.94,59.15,24)))
    keepouts.append(('INMP441_ASSUMED',s.cylinder(10,40,z,15.8,16)))
    return keepouts,heights


def interference(parts, keepouts, config):
    """只证明回读包络与指定假设的净空，不给未知线束和温升签字。"""
    checks=[]
    rear=config['case']['rear'];bottom=rear+config['pcb']['standoff']
    slab=s.rounded(50,50,bottom,100,100,config['pcb']['thickness'],3)
    keepouts=[('PCB',slab)]+keepouts
    for name in ('main-body','rear-cover'):
        for ref,solid in keepouts:
            overlap=(parts[name]^solid).volume()
            if overlap>1e-5:checks.append({'part':name,'object':ref,'overlapMm3':overlap})
    for name in ('screen-bezel','camera-bezel','sensor-left','sensor-right'):
        overlap=(parts['main-body']^parts[name]).volume()
        if overlap>1e-5:checks.append({'part':'main-body','object':name,'overlapMm3':overlap})
    overlap=(parts['main-body']^parts['rear-cover']).volume()
    if overlap>1e-5:checks.append({'part':'main-body','object':'rear-cover','overlapMm3':overlap})
    return checks


def export_mesh(solid, path):
    if str(solid.status())!='Error.NoError':raise ValueError(f'{path.name}: {solid.status()}')
    raw=solid.to_mesh()
    mesh=trimesh.Trimesh(vertices=np.asarray(raw.vert_properties)[:,:3],faces=raw.tri_verts,process=True)
    # STL重读独立核实网格方向、闭合性及体积，不以布尔运算成功代替。
    mesh.export(path)
    check=trimesh.load_mesh(path,process=True)
    if not check.is_watertight or not check.is_winding_consistent or check.volume<=0:
        raise ValueError(f'{path.name}: STL不是正体积闭合网格')
    if abs(check.volume-solid.volume())>max(.02,solid.volume()*2e-5):
        raise ValueError(f'{path.name}: STL与实体体积不一致')
    shells=len(solid.decompose())
    if shells!=1:raise ValueError(f'{path.name}: 存在{shells}个不相连实体')
    return {'file':path.name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
            'triangles':len(check.faces),'boundsMm':check.bounds.tolist(),
            'volumeMm3':float(check.volume),'watertight':True,'windingConsistent':True,'connectedBodies':shells}


def make_parts(config, positions):
    left,right=config['sensorPanels']['centers']
    return {'main-body':s.main_body(config,positions),'rear-cover':s.rear_cover(config),
            'screen-bezel':s.screen_panel(config),'camera-bezel':s.camera_panel(config),
            'sensor-left':s.vent_panel(config,left),'sensor-right':s.vent_panel(config,right),
            'screen-retainer':s.retaining_bar(),'universal-tray':s.universal_tray(),
            'screen-fit-coupon':s.screen_fit_coupon(config),'radar-fit-coupon':s.radar_fit_coupon(config),
            'desktop-foot-left':s.desktop_foot(8),'desktop-foot-right':s.desktop_foot(92)}


def save_print_orientations(parts, destination, config):
    """打印文件平放床面；主壳翻面，减少前面板悬空。"""
    destination.mkdir()
    reports=[]
    for name,solid in parts.items():
        if name in ('main-body','screen-bezel','camera-bezel','sensor-left','sensor-right'):
            solid=solid.rotate((180,0,0))
        if name.startswith('desktop-foot'):solid=solid.rotate((0,90,0))
        bounds=np.array(solid.bounding_box()).reshape(2,3)
        solid=solid.translate((-bounds[0][0],-bounds[0][1],-bounds[0][2]))
        reports.append(export_mesh(solid,destination/f'{name}.stl'))
    return reports


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('board','models','intent','out'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists():raise FileExistsError('输出目录已存在，请使用新的目录，避免覆盖旧模型')
    config=read_json(args.intent);board,components,positions=read_board(args.board,config)
    models=read_json(args.models);parts=make_parts(config,positions)
    keepouts,heights=component_clearances(components,models,config)
    collisions=interference(parts,keepouts,config)
    if collisions:raise ValueError(json.dumps(collisions,ensure_ascii=False))
    args.out.mkdir(parents=True)
    reports=save_print_orientations(parts,args.out/'print',config)
    assembly=args.out/'assembly';assembly.mkdir()
    for name,solid in parts.items():export_mesh(solid,assembly/f'{name}.stl')
    manifest={'status':'candidate-unverified','geometryChecks':'pass','revision':config['revision'],
              'boardSha256':hashlib.sha256(args.board.read_bytes()).hexdigest(),
              'intentSha256':hashlib.sha256(args.intent.read_bytes()).hexdigest(),
              'pcbCapture':board.get('capture'),'caseMm':[config['case'][k] for k in ('width','height','depth')],
              'frontWithBezelsMm':config['case']['depth']+2.4,'files':reports,'interferences':collisions,
              'componentHeightEvidence':heights,'heightAssumption':config['heightAssumption'],
              'limitations':[config['printGate'],'只检查主壳、后盖与回读器件包络、假设主控和麦克风净空。',
                             '未证明屏幕玻璃、相机镜头、线束、喇叭、外接模块、热区温升、打印公差和雷达性能。']}
    write_json(args.out/'manifest.json',manifest)
    write_json(args.out/'intent.json',config)
    from preview import render_delivery
    render_delivery(parts,config,positions,args.out)
    print(json.dumps({'out':str(args.out),'stlCount':len(reports),'geometryChecks':'pass',
                      'status':'candidate-unverified'},ensure_ascii=False))


if __name__=='__main__':main()
