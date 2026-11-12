import {readFileSync,writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';

// 以真实焊盘/铜/禁布区规划短接地支线；矩形圆角按外接矩形保守避让。
const [boardPath,intentPath,out]=process.argv.slice(2);
if(!out)throw new Error('用法：node plan-ground-vias.mjs board.json intent.json output-prefix');
const read=p=>JSON.parse(readFileSync(p,'utf8'));
const board=read(boardPath),intent=read(intentPath),mm=1/.0254;
const requireState=(ok,m)=>{if(!ok)throw new Error(m);};
requireState(board.capture.identityVerified && board.capture.projectUuid===intent.projectId && board.capture.documentUuid===intent.documentId,'快照身份不符');
requireState(board.copperLayers===4 && Object.values(board.copper.availability).every(v=>v==='available'),'叠层或铜清单缺测');
requireState(board.copper.arcs.length===0 && board.copper.vias.length===0 && board.copper.pours.length===0,'只接受接地孔创建前的铜基线');
const pointSegment=(p,a,b)=>{
  const d=b.map((x,k)=>x-a[k]),n=d[0]**2+d[1]**2;
  const t=n?Math.max(0,Math.min(1,((p[0]-a[0])*d[0]+(p[1]-a[1])*d[1])/n)):0;
  return Math.hypot(p[0]-a[0]-t*d[0],p[1]-a[1]-t*d[1]);
};
const cross=(a,b,c)=>(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
const intersects=(a,b,c,d)=>{
  if(Math.max(a[0],b[0])+1e-8<Math.min(c[0],d[0]) || Math.max(c[0],d[0])+1e-8<Math.min(a[0],b[0]) || Math.max(a[1],b[1])+1e-8<Math.min(c[1],d[1]) || Math.max(c[1],d[1])+1e-8<Math.min(a[1],b[1]))return false;
  return cross(a,b,c)*cross(a,b,d)<=1e-8 && cross(c,d,a)*cross(c,d,b)<=1e-8;
};
const segmentSegment=(a,b,c,d)=>intersects(a,b,c,d)?0:Math.min(pointSegment(a,c,d),pointSegment(b,c,d),pointSegment(c,a,b),pointSegment(d,a,b));
const edges=p=>p.map((v,k)=>[v,p[(k+1)%p.length]]);
const inside=(p,poly)=>{
  let hit=false;
  for(const [a,b] of edges(poly)){
    if(pointSegment(p,a,b)<1e-8)return true;
    if((a[1]>p[1])!==(b[1]>p[1]) && p[0]<(b[0]-a[0])*(p[1]-a[1])/(b[1]-a[1])+a[0])hit=!hit;
  }
  return hit;
};
const pointPolygon=(p,poly)=>inside(p,poly)?0:Math.min(...edges(poly).map(([a,b])=>pointSegment(p,a,b)));
const segmentPolygon=(a,b,poly)=>inside(a,poly)||inside(b,poly)?0:Math.min(...edges(poly).map(([c,d])=>segmentSegment(a,b,c,d)));
const linearSource=source=>{
  requireState(Array.isArray(source) && source[2]==='L' && source.slice(3).every(Number.isFinite),'仅支持完整线性多边形源');
  const numbers=[...source.slice(0,2),...source.slice(3)],p=[];
  requireState(numbers.length%2===0,'多边形坐标不配对');
  for(let k=0;k<numbers.length;k+=2)p.push(numbers.slice(k,k+2));
  if(p.length>1 && p[0][0]===p.at(-1)[0] && p[0][1]===p.at(-1)[1])p.pop();
  requireState(p.length>=3,'多边形不足三点');return p;
};
const pads=board.components.flatMap(c=>c.pads.map(p=>{
  requireState(p.availability?.shape==='available' && p.specialPadPresent===false,'焊盘形状缺测或存在特殊层焊盘');
  const s=p.shape,angle=(p.rotation??0)*Math.PI/180;
  const transform=([x,y])=>[p.x+x*Math.cos(angle)-y*Math.sin(angle),p.y+x*Math.sin(angle)+y*Math.cos(angle)];
  let poly;
  if(s[0]==='POLYGON')poly=linearSource(s[1]);
  else if(s[0]==='RECT')poly=[[-s[1]/2,-s[2]/2],[s[1]/2,-s[2]/2],[s[1]/2,s[2]/2],[-s[1]/2,s[2]/2]].map(transform);
  else if(['ELLIPSE','OVAL'].includes(s[0])){
    // OVAL 用外接矩形；椭圆用外切 64 边形，避免内接近似制造假净距。
    if(s[0]==='OVAL')poly=[[-s[1]/2,-s[2]/2],[s[1]/2,-s[2]/2],[s[1]/2,s[2]/2],[-s[1]/2,s[2]/2]].map(transform);
    else poly=Array.from({length:64},(_,k)=>transform([Math.cos(k*Math.PI/32)*s[1]/2/Math.cos(Math.PI/64),Math.sin(k*Math.PI/32)*s[2]/2/Math.cos(Math.PI/64)]));
  }else throw new Error('未知焊盘形状：'+s[0]);
  return {...p,ref:c.designator,padRef:c.designator+'.'+p.padNumber,poly,center:[p.x,p.y],bodyCenter:[(c.bbox.minX+c.bbox.maxX)/2,(c.bbox.minY+c.bbox.maxY)/2]};
}));
const regions=board.copper.regions.filter(r=>[1,12].includes(r.layer)&&r.ruleType.includes(5)).map(r=>({poly:linearSource(r.source),radius:r.lineWidth/2}));
const cuts=board.copper.fills.filter(f=>f.layer===12).map(f=>({poly:linearSource(f.source),radius:f.lineWidth/2}));
const tracks=board.copper.lines.filter(t=>t.layer===1).map(t=>({...t,a:[t.startX,t.startY],b:[t.endX,t.endY]}));
const radius=12,clearance=8,edge=20,hole=12,stubWidth=8;
const vias=[],stubs=[],unresolved=[];
const withinBoard=p=>{
  const x=p[0]/mm,y=p[1]/mm,r=(radius+edge)/mm,w=intent.board.width,h=intent.board.height,c=intent.board.cornerRadius;
  if(x<r||x>w-r||y<r||y>h-r)return false;
  for(const [cx,cy,sx,sy] of [[c,c,-1,-1],[w-c,c,1,-1],[c,h-c,-1,1],[w-c,h-c,1,1]])if(sx*(x-cx)>0 && sy*(y-cy)>0 && Math.hypot(x-cx,y-cy)>c-r)return false;
  return true;
};
const legalVia=p=>withinBoard(p)
  && pads.every(q=>pointPolygon(p,q.poly)>=radius+(q.net==='GND'?4:clearance)-1e-6)
  && tracks.every(t=>t.net==='GND'||pointSegment(p,t.a,t.b)>=radius+t.lineWidth/2+clearance)
  && regions.every(r=>pointPolygon(p,r.poly)>=radius+r.radius)
  && cuts.every(r=>pointPolygon(p,r.poly)>=radius+r.radius+edge)
  && vias.every(v=>Math.hypot(p[0]-v.x,p[1]-v.y)>=radius*2+clearance);
const legalStub=(a,b)=>pads.every(q=>q.net==='GND'||segmentPolygon(a,b,q.poly)>=stubWidth/2+clearance-1e-6)
  && tracks.every(t=>t.net==='GND'||segmentSegment(a,b,t.a,t.b)>=stubWidth/2+t.lineWidth/2+clearance)
  && regions.every(r=>segmentPolygon(a,b,r.poly)>=stubWidth/2+r.radius)
  && cuts.every(r=>segmentPolygon(a,b,r.poly)>=stubWidth/2+r.radius+edge);
const grounds=pads.filter(p=>p.net==='GND'&&p.layer===1).sort((a,b)=>(a.ref.startsWith('C')?0:1)-(b.ref.startsWith('C')?0:1)||a.padRef.localeCompare(b.padRef));
for(const p of grounds){
  const reuse=vias.filter(v=>Math.hypot(v.x-p.x,v.y-p.y)<=2*mm && legalStub(p.center,[v.x,v.y])).sort((a,b)=>Math.hypot(a.x-p.x,a.y-p.y)-Math.hypot(b.x-p.x,b.y-p.y))[0];
  if(reuse){stubs.push({from:p.padRef,to:reuse.id,a:p.center,b:[reuse.x,reuse.y]});continue;}
  const candidates=[];
  for(const distance of [.7,1,1.3,1.6,2,2.4])for(let k=0;k<8;k++){
    const angle=k*Math.PI/4,v=[p.x+distance*mm*Math.cos(angle),p.y+distance*mm*Math.sin(angle)].map(v=>+v.toFixed(4));
    if(legalVia(v)&&legalStub(p.center,v)){
      const away=(v[0]-p.x)*(p.x-p.bodyCenter[0])+(v[1]-p.y)*(p.y-p.bodyCenter[1]);
      candidates.push({v,cost:distance-(away>0?.05:0)});
    }
  }
  candidates.sort((a,b)=>a.cost-b.cost);
  if(!candidates.length){unresolved.push(p.padRef);continue;}
  const v=candidates[0].v,id='ground-via-'+(vias.length+1);vias.push({id,x:v[0],y:v[1]});stubs.push({from:p.padRef,to:id,a:p.center,b:v});
}
const result={status:unresolved.length?'incomplete':'offline-verified',scope:'conservative TOP pad/track/keepout clearance, all-layer via-to-pad clearance, short GND stubs; native DRC still required',boardSha256:createHash('sha256').update(readFileSync(boardPath)).digest('hex'),vias,stubs,unresolved,clearanceMil:clearance,viaHoleMil:hole,viaDiameterMil:radius*2,stubWidthMil:stubWidth};
writeFileSync(out+'.json',JSON.stringify(result,null,2)+'\n',{flag:'wx'});
if(!unresolved.length){
  const steps=vias.map(v=>({id:v.id,action:'pcb.via.create',payload:{x:v.x,y:v.y,holeDiameter:hole,diameter:radius*2,net:'GND'}}));
  stubs.forEach((s,k)=>steps.push({id:'ground-stub-'+(k+1),action:'pcb.line.create',payload:{startX:s.a[0],startY:s.a[1],endX:s.b[0],endY:s.b[1],lineWidth:stubWidth,layer:1,net:'GND'}}));
  steps.push({id:'save-ground',action:'pcb.save',assert:{'$.saved':'true'}});
  writeFileSync(out+'.apply.json',JSON.stringify({version:1,meta:{name:'Short local GND vias',project:intent.projectId,doc:intent.documentId},steps},null,2)+'\n',{flag:'wx'});
}
console.log({status:result.status,vias:vias.length,stubs:stubs.length,unresolved});
if(unresolved.length)process.exitCode=1;
