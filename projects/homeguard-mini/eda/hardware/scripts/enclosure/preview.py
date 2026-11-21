"""从同一批真实外壳网格生成PNG和可旋转HTML，不另外画一张想象效果图。"""
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont
import solids as s


COLORS={'main-body':'#e9e4d9','rear-cover':'#c4c4b8','screen-bezel':'#3c4b4c',
        'camera-bezel':'#3c4b4c','sensor-left':'#d0d7c9','sensor-right':'#d0d7c9',
        'desktop-foot-left':'#8d9d8f','desktop-foot-right':'#8d9d8f',
        'pcb-placeholder':'#29735a','mcu-placeholder':'#29363a',
        'display-placeholder':'#143d3e','lens-placeholder':'#102428'}


def scene_items(parts,config):
    wanted=list(COLORS)[:8]
    items=[(name,parts[name]) for name in wanted]
    z=config['case']['rear']+config['pcb']['standoff']
    dx,dy=config['display']['center'];ox,oy=config['display']['windowOffset']
    cx,cy=config['camera']['center']
    items += [('pcb-placeholder',s.rounded(50,50,z,100,100,1.6,3)),
              ('mcu-placeholder',s.box(50,50,z+11,28,57,5)),
              ('display-placeholder',s.box(dx+ox,dy+oy,55.7,30,31,.2)),
              ('lens-placeholder',s.cylinder(cx,cy,54,10,4))]
    return items


def pack_scene(items):
    result=[]
    for name,solid in items:
        raw=solid.to_mesh()
        result.append({'name':name,'color':COLORS[name],
                       'vertices':np.asarray(raw.vert_properties)[:,:3].round(4).tolist(),
                       'faces':np.asarray(raw.tri_verts).tolist()})
    return result


def displacement(name, exploded):
    if not exploded:return np.zeros(3)
    if name=='rear-cover':return np.array([0,0,-35])
    if name in ('screen-bezel','display-placeholder'):return np.array([0,0,42])
    if name in ('camera-bezel','lens-placeholder'):return np.array([0,0,32])
    if name.startswith('sensor-'):return np.array([0,0,25])
    return np.zeros(3)


def font(size):
    for path in ('C:/Windows/Fonts/msyh.ttc','C:/Windows/Fonts/simhei.ttf','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'):
        if Path(path).exists():return ImageFont.truetype(path,size)
    return ImageFont.load_default()


def raster_triangle(pixels,depths,points,zvalues,rgb):
    """逐像素深度检查，避免长三角形按中心排序造成假穿孔。"""
    h,w=depths.shape
    lo=np.maximum(np.floor(points.min(axis=0)).astype(int),[0,0])
    hi=np.minimum(np.ceil(points.max(axis=0)).astype(int),[w-1,h-1])
    if np.any(hi<lo):return
    x,y=np.meshgrid(np.arange(lo[0],hi[0]+1)+.5,np.arange(lo[1],hi[1]+1)+.5)
    a,b,c=points;den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
    if abs(den)<1e-10:return
    u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/den
    v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/den
    t=1-u-v;z=u*zvalues[0]+v*zvalues[1]+t*zvalues[2]
    section=depths[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    mask=(u>=-1e-7)&(v>=-1e-7)&(t>=-1e-7)&(z>section)
    section[mask]=z[mask]
    pixels[lo[1]:hi[1]+1,lo[0]:hi[0]+1][mask]=rgb


def draw_model(img,scene,center,scale,exploded=False,internal=False):
    view=np.array([.65,-.6,1.8]);view/=np.linalg.norm(view)
    right=np.cross([0,1,0],view);right/=np.linalg.norm(right);up=np.cross(view,right)
    pixels=np.array(img);depths=np.full((img.height,img.width),-np.inf)
    for item in scene:
        name=item['name']
        if internal and name not in ('rear-cover','pcb-placeholder','mcu-placeholder'):continue
        if exploded and name.startswith('desktop-foot'):continue
        vertices=np.array(item['vertices'])+displacement(name,exploded)-[50,55,25]
        screen=np.stack((vertices@right,-vertices@up),axis=1)*scale+center
        for face in item['faces']:
            face=np.array(face);xyz=vertices[face];normal=np.cross(xyz[1]-xyz[0],xyz[2]-xyz[0]);length=np.linalg.norm(normal)
            if not length or normal@view<=0:continue
            shade=.58+.42*max(0,(normal/length)@np.array([-.25,-.35,.902]))
            rgb=tuple(int(int(item['color'][i:i+2],16)*shade) for i in (1,3,5))
            raster_triangle(pixels,depths,screen[face],xyz@view,rgb)
    img.paste(Image.fromarray(pixels))


def render_png(scene,destination):
    img=Image.new('RGB',(1800,1200),'#f6f5f0');draw=ImageDraw.Draw(img)
    draw.text((65,35),'HomeGuard Mini · 桌面外壳初版',font=font(34),fill='#263e39')
    draw.text((65,90),'圆角对称 / 可拆后盖 / 模块面板可更换',font=font(20),fill='#59675d')
    draw_model(img,scene,(470,595),4.3)
    draw_model(img,scene,(1310,595),3.85,exploded=True)
    draw=ImageDraw.Draw(img)
    draw.text((310,1050),'装配外观',font=font(25),fill='#263e39')
    draw.text((1190,1050),'拆装示意',font=font(25),fill='#263e39')
    draw.text((65,1130),'壳体140×140×54mm；面框最高56.4mm。屏幕、主控与镜头为示意占位，未做实物装配和温升验证。',font=font(19),fill='#697368')
    img.save(destination/'preview.png')


def render_delivery(parts,config,positions,destination):
    scene=pack_scene(scene_items(parts,config))
    render_png(scene,destination)
    template=Path(__file__).with_name('viewer.html').read_text(encoding='utf8')
    template=template.replace('__SCENE_JSON__',json.dumps(scene,separators=(',',':')))
    (destination/'preview.html').write_text(template,encoding='utf8')
