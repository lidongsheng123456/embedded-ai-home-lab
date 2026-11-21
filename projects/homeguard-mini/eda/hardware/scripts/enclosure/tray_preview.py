"""学习底座的真实网格预览；电子件仅显示回读包络和主控占位。"""
import json
import re
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from preview import draw_model, font
import solids as s


def scene_item(name, solid, color):
    mesh=solid.to_mesh()
    return {'name':name,'color':color,
            'vertices':np.asarray(mesh.vert_properties)[:,:3].round(4).tolist(),
            'faces':np.asarray(mesh.tri_verts).tolist()}


def render(tray, pcb, parts, config, destination):
    scene=[scene_item('rear-cover',tray,'#c9d6e4'),
           scene_item('pcb-placeholder',pcb,'#287553')]
    z=config['case']['floor']+config['pcb']['standoff']+config['pcb']['thickness']
    for ref,component in parts.items():
        if ref in ('J5','J14','J15','J16','J17','J18'):continue
        b=component['bbox'];x0,y0,x1,y1=[b[k]*.0254 for k in ('minX','minY','maxX','maxY')]
        h=8.5 if ref.startswith('J') else 2
        solid=s.box((x0+x1)/2,(y0+y1)/2,z,x1-x0,y1-y0,h)
        scene.append(scene_item(ref+'-envelope',solid,'#37454a' if ref.startswith('J') else '#b4aca0'))
    scene.append(scene_item('mcu-placeholder',s.box(50,50,z+8.5,29.94,59.15,1.6),'#315e70'))
    img=Image.new('RGB',(1800,1200),'#f6f5f0');draw=ImageDraw.Draw(img)
    draw.text((65,35),'HomeGuard Mini · 免费券学习保护底座',font=font(34),fill='#263e39')
    draw.text((65,90),'一体底座 / 顶部开放插拔 / 四角螺丝与螺母固定 / X树脂默认颜色',font=font(21),fill='#59675d')
    draw_model(img,scene,(470,600),4.8)
    draw_model(img,[item for item in scene if item['name']=='rear-cover'],(1310,600),4.8)
    draw=ImageDraw.Draw(img)
    draw.text((300,1020),'装板示意：原PCB保持100×100mm',font=font(23),fill='#263e39')
    draw.text((1180,1020),'底座单件：99×99×8.2mm',font=font(23),fill='#263e39')
    draw.text((65,1110),'底板98×98mm，四角柱使最大外形99mm；仅保护板底，不包侧边。电子件为包络示意，不打印。',font=font(19),fill='#697368')
    draw.text((65,1145),'用券按1款1件、X树脂、不打磨不喷砂；不保证透明，资格以平台审核为准。实物插合仍待确认。',font=font(19),fill='#697368')
    img.save(destination/'preview.png')
    template=Path(__file__).with_name('viewer.html').read_text(encoding='utf-8')
    template=re.sub(r'<title>.*?</title>','<title>免费券学习保护底座</title>',template)
    template=re.sub(r'<header>.*?</header>',
                    '<header><h1>HomeGuard Mini · 学习保护底座</h1><p>原PCB不缩放；底座一体打印，顶部开放。拖动旋转，滚轮缩放。</p></header>',template)
    template=template.replace('整机外观','装板示意').replace('拆装示意','拆装方向').replace('后盖与主板','底座与主板')
    footer='<footer><span class="tag">底座99×99×8.2mm</span><span class="tag">PCB 100×100mm</span><span class="tag">1款1件 / X树脂 / 默认颜色</span>'
    footer+='<p>仅保护板底，PCB四边各伸出98mm底板约1mm；螺母从底部装入，板顶螺丝固定。电子模块为包络或占位，不打印。不保证透明。</p>'
    footer+='<p>尺寸与体积已按券规则离线核对，壁厚检测、活动审核和实物插合仍待确认。不能将这个底座当作防水防尘外壳。</p></footer>'
    template=re.sub(r'<footer>.*?</footer>',footer,template,flags=re.S)
    template=template.replace('__SCENE_JSON__',json.dumps(scene,separators=(',',':')))
    (destination/'preview.html').write_text(template,encoding='utf-8')
