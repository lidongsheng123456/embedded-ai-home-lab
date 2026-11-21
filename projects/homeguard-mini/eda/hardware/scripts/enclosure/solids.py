"""外壳实体；坐标沿用PCB的毫米坐标，Z=0为后盖外表面。"""
import manifold3d as m


def box(x, y, z, w, h, d):
    return m.Manifold.cube((w, h, d)).translate((x-w/2, y-h/2, z))


def cylinder(x, y, z, diameter, depth):
    return m.Manifold.cylinder(depth, diameter/2, circular_segments=64).translate((x,y,z))


def rounded(x, y, z, w, h, depth, radius):
    profile = m.CrossSection.square((w-2*radius,h-2*radius), center=True).offset(radius, circular_segments=64)
    return profile.extrude(depth).translate((x,y,z))


def union(items):
    return m.Manifold.batch_boolean(items, m.OpType.Add) if items else m.Manifold()


def subtract(solid, cuts):
    return solid - union(cuts)


def front_mount(config, center, plate, opening, screws):
    """面板压在正面，用独立螺丝装拆；螺丝孔放在通孔之外。"""
    x,y=center; depth=config['case']['depth']; front=config['case']['front']
    aperture=rounded(x,y,depth-front-0.1,*opening,front+0.2,2)
    bosses=[cylinder(sx,sy,depth-8,6,8) for sx,sy in screws]
    pilots=[cylinder(sx,sy,depth-8.1,1.8,8.3) for sx,sy in screws]
    return bosses, [aperture]+pilots


def plate_screws(x, y, offsets):
    return [(x+dx,y+dy) for dx in (-offsets[0],offsets[0]) for dy in (-offsets[1],offsets[1])]


def vent_panel(config, center):
    x,y=center; z=config['case']['depth']; p=config['sensorPanels']; w,h=p['plate']
    panel=rounded(x,y,z,w,h,2.4,3)
    cuts=[cylinder(x,y+dy,z-0.1,2.5,2.6) for dy in (-p['screwSpacing']/2,p['screwSpacing']/2)]
    cuts += [rounded(x,y+dy,z-0.1,12,1.8,2.6,.8) for dy in range(-12,13,4)]
    return subtract(panel,cuts)


def screen_panel(config):
    p=config['display'];x,y=p['center'];z=config['case']['depth'];w,h=p['plate']
    panel=rounded(x,y,z,w,h,2.4,4)
    screws=plate_screws(x,y,p['screwOffsets'])
    cuts=[cylinder(sx,sy,z-.1,2.5,2.6) for sx,sy in screws]
    ox,oy=p['windowOffset']
    cuts.append(rounded(x+ox,y+oy,z-.1,*p['window'],2.6,.5))
    # 三边定位留0.6mm装配余量，排针侧敞开，避免夹到玻璃与焊点。
    for dx in (-17.4,17.4):
        panel += box(x+dx,y,z-4.4,1.6,32,4.5)
    panel += box(x,y-20.4,z-4.4,36.4,1.6,4.5)
    for dx in (-22,22):
        panel += cylinder(x+dx,y,z-5,6,5.2)
        cuts.append(cylinder(x+dx,y,z-5.1,1.8,5.2))
    return subtract(panel,cuts)


def camera_panel(config):
    p=config['camera'];x,y=p['center'];z=config['case']['depth']
    panel=rounded(x,y,z,*p['plate'],2.4,4)
    cuts=[cylinder(x,y,z-.1,p['windowDiameter'],2.6)]
    cuts += [cylinder(sx,sy,z-.1,2.5,2.6) for sx,sy in plate_screws(x,y,[17.5,16.5])]
    # 固定带穿过四条长槽，容纳尚未量准的镜头板，不假造安装孔。
    cuts += [box(x+dx,y+dy,z-.1,2,10,2.6) for dx in (-17,17) for dy in (-7,7)]
    return subtract(panel,cuts)


def rear_cover(config):
    c=config['case'];p=config['pcb'];x,y=c['center'];floor=c['rear']
    cover=rounded(x,y,0,c['width'],c['height'],floor,c['radius'])
    # 止口只定位，不用卡扣承受反复拆装。
    outer=rounded(x,y,floor,c['width']-5.4,c['height']-5.4,2.8,c['radius']-2.7)
    inner=rounded(x,y,floor-.1,c['width']-8.2,c['height']-8.2,3,c['radius']-4.1)
    cover += outer-inner
    cuts=[]
    for sx,sy in p['mounts']:
        cover += cylinder(sx,sy,floor,p['pillarDiameter'],p['standoff'])
        cuts.append(cylinder(sx,sy,floor+1,p['pilotDiameter'],p['standoff']+.2))
    for sx,sy in config['caseScrews']:
        cuts += [cylinder(sx,sy,-.1,3.4,floor+.2),cylinder(sx,sy,-.1,6.2,1.1)]
        cuts.append(cylinder(sx,sy,floor,9.6,3.1))
    # 后盖透气孔避开主板固定柱，底部不用螺丝头充当桌脚。
    for sx in (24,76):
        for sy in range(29,72,7):
            cuts.append(rounded(sx,sy,-.1,12,2,2.6,1))
    return subtract(cover,cuts)


def main_body(config, positions):
    c=config['case'];x,y=c['center'];z=c['depth'];wall=c['wall'];rear=c['rear']
    body=rounded(x,y,rear,c['width'],c['height'],z-rear,c['radius'])
    cavity=rounded(x,y,rear-.1,c['width']-2*wall,c['height']-2*wall,z-c['front']-rear+.1,c['radius']-wall)
    body -= cavity
    cuts=[];bosses=[]
    for sx,sy in config['caseScrews']:
        bosses.append(cylinder(sx,sy,rear,9,10))
        cuts.append(cylinder(sx,sy,rear-.1,2.6,10.2))
    p=config['display'];a,b=front_mount(config,p['center'],p['plate'],p['mountOpening'],plate_screws(*p['center'],p['screwOffsets']))
    bosses+=a;cuts+=b
    p=config['camera'];a,b=front_mount(config,p['center'],p['plate'],p['mountOpening'],plate_screws(*p['center'],[17.5,16.5]))
    bosses+=a;cuts+=b
    p=config['sensorPanels']
    for sx,sy in p['centers']:
        screws=[(sx,sy+dy) for dy in (-p['screwSpacing']/2,p['screwSpacing']/2)]
        a,b=front_mount(config,[sx,sy],p['plate'],p['mountOpening'],screws);bosses+=a;cuts+=b
    for sx,sy in config['audioGrilles']['centers']:
        for dy in range(-8,9,4):
            cuts.append(rounded(sx,sy+dy,z-2.5,15,1.8,2.7,.8))
    # 雷达前表面保留连续的1.2mm非金属塑料，不开金属网孔。
    rx,ry=config['radar']['center']
    cuts.append(rounded(rx,ry,z-2.5,20,26,2.5-config['radar']['skin'],2))
    for sx in (rx-10.8,rx+10.8):
        bosses.append(box(sx,ry,z-9,2,24,7.8))
    lx,ly=config['light']['center'];cuts.append(cylinder(lx,ly,z-2.5,config['light']['windowDiameter'],2.7))
    bx,by=positions['SW1'];cuts.append(cylinder(bx,by,z-2.5,config['button']['accessDiameter'],2.7))
    mx,my=config['microphone']['center'];cuts.append(cylinder(mx,my,z-2.5,config['microphone']['accessDiameter'],2.7))
    ux,uy=positions['J10'];uw,uh=config['usb']['opening'];yc=y-c['height']/2
    cuts.append(box(ux,(yc+uy)/2,config['usb']['zCenter']-uh/2,uw,uy-yc+10,uh))
    # 上边与侧边的空气出口；壳体不做密封，走线不穿过螺丝柱。
    for sx in (16,30,70,84):
        cuts.append(box(sx,y+c['height']/2,28,8,7,3))
    for sy in (45,53,61,69,77,85):
        for sx in (x-c['width']/2,x+c['width']/2):
            cuts.append(box(sx,sy,29,7,3,13))
    # 热区挡板从板上插头上方开始，右侧向通风壁敞开。
    bosses += [box(88,65,25,1.8,40,26.8),box(103,44.5,25,31.8,1.8,26.8),box(103,85.5,25,31.8,1.8,26.8)]
    return subtract(body+union(bosses),cuts)


def retaining_bar():
    bar=rounded(0,0,0,13,8,2.4,1)
    return bar-rounded(-2,0,-.1,5,2.5,2.6,1.2)


def universal_tray():
    tray=rounded(0,0,0,28,34,2.4,2)
    cuts=[rounded(dx,0,-.1,2.5,23,2.6,1.2) for dx in (-10,10)]
    cuts.append(rounded(0,0,-.1,12,18,2.6,1))
    return subtract(tray,cuts)


def screen_fit_coupon(config):
    p=config['display']
    return rounded(0,0,0,40,46,2.4,2)-rounded(0,0,-.1,p['module'][0]+1.2,p['module'][1]+1.2,2.6,.5)


def radar_fit_coupon(config):
    w,h=config['radar']['module']
    return rounded(0,0,0,w+5,h+5,2.4,2)-rounded(0,0,-.1,w+1,h+1,2.6,1)


def desktop_foot(x):
    """两只独立桌脚抬高底部，给向下插入的USB线留出约24mm转弯空间。"""
    base=box(x,-33.5,-12,10,5,80)
    saddle=box(x,-10,-.4,10,4,54.8)
    legs=[box(x,-18,zz,10,34,3) for zz in (-3.4,54.4)]
    return base+saddle+union(legs)
