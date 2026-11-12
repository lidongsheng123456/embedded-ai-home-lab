// 保守包络用于设计候选避让；最终制造净距仍由原生 DRC 和 Gerber 验证。
export const pointSegment=(p,a,b)=>{
  const d=b.map((v,k)=>v-a[k]),n=d[0]**2+d[1]**2;
  const t=n?Math.max(0,Math.min(1,((p[0]-a[0])*d[0]+(p[1]-a[1])*d[1])/n)):0;
  return Math.hypot(p[0]-a[0]-t*d[0],p[1]-a[1]-t*d[1]);
};
const cross=(a,b,c)=>(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
const intersects=(a,b,c,d)=>{
  if(Math.max(a[0],b[0])+1e-8<Math.min(c[0],d[0])||Math.max(c[0],d[0])+1e-8<Math.min(a[0],b[0])||Math.max(a[1],b[1])+1e-8<Math.min(c[1],d[1])||Math.max(c[1],d[1])+1e-8<Math.min(a[1],b[1]))return false;
  return cross(a,b,c)*cross(a,b,d)<=1e-8&&cross(c,d,a)*cross(c,d,b)<=1e-8;
};
export const segmentSegment=(a,b,c,d)=>intersects(a,b,c,d)?0:Math.min(pointSegment(a,c,d),pointSegment(b,c,d),pointSegment(c,a,b),pointSegment(d,a,b));
export const edges=p=>p.map((v,k)=>[v,p[(k+1)%p.length]]);
export const inside=(p,poly)=>{
  let hit=false;
  for(const [a,b] of edges(poly)){
    if(pointSegment(p,a,b)<1e-8)return true;
    if((a[1]>p[1])!==(b[1]>p[1])&&p[0]<(b[0]-a[0])*(p[1]-a[1])/(b[1]-a[1])+a[0])hit=!hit;
  }
  return hit;
};
export const pointPolygon=(p,poly)=>inside(p,poly)?0:Math.min(...edges(poly).map(([a,b])=>pointSegment(p,a,b)));
export const segmentPolygon=(a,b,poly)=>inside(a,poly)||inside(b,poly)?0:Math.min(...edges(poly).map(([c,d])=>segmentSegment(a,b,c,d)));
export const linearSource=source=>{
  if(!Array.isArray(source)||source[2]!=='L'||!source.slice(3).every(Number.isFinite))throw Error('需要完整线性多边形');
  const numbers=[...source.slice(0,2),...source.slice(3)],p=[];
  if(numbers.length%2)throw Error('坐标不配对');
  for(let k=0;k<numbers.length;k+=2)p.push(numbers.slice(k,k+2));
  if(p.length>1&&p[0][0]===p.at(-1)[0]&&p[0][1]===p.at(-1)[1])p.pop();
  if(p.length<3)throw Error('多边形点不足');return p;
};
export function padPolygon(p){
  if(p.availability?.shape!=='available'||p.specialPadPresent!==false)throw Error('焊盘形状缺测');
  const s=p.shape,a=(p.rotation??0)*Math.PI/180;
  const tr=([x,y])=>[p.x+x*Math.cos(a)-y*Math.sin(a),p.y+x*Math.sin(a)+y*Math.cos(a)];
  if(s[0]==='POLYGON')return linearSource(s[1]);
  if(['RECT','OVAL'].includes(s[0]))return [[-s[1]/2,-s[2]/2],[s[1]/2,-s[2]/2],[s[1]/2,s[2]/2],[-s[1]/2,s[2]/2]].map(tr);
  if(s[0]==='ELLIPSE')return Array.from({length:64},(_,k)=>tr([Math.cos(k*Math.PI/32)*s[1]/2/Math.cos(Math.PI/64),Math.sin(k*Math.PI/32)*s[2]/2/Math.cos(Math.PI/64)]));
  throw Error('不支持的焊盘形状：'+s[0]);
}
export function copperModel(board,intent){
  if(!board.capture?.identityVerified||board.capture.projectUuid!==intent.projectId||board.capture.documentUuid!==intent.documentId)throw Error('快照身份不符');
  if(!Object.values(board.copper.availability).every(v=>v==='available')||board.copper.arcs.length)throw Error('铜缺测或存在未支持的圆弧走线');
  if(board.copper.pours.some(p=>[1,2].includes(p.layer)))throw Error('外层铺铜尚不支持');
  const pads=board.components.flatMap(c=>c.pads.map(p=>({...p,ref:c.designator+'.'+p.padNumber,poly:padPolygon(p)})));
  const tracks=board.copper.lines.map(t=>({...t,a:[t.startX,t.startY],b:[t.endX,t.endY]}));
  const regions=board.copper.regions.filter(r=>r.ruleType.includes(5)).map(r=>({...r,poly:linearSource(r.source)}));
  const cuts=board.copper.fills.filter(f=>f.layer===12).map(f=>({...f,poly:linearSource(f.source)}));
  if(board.copper.fills.some(f=>[1,2].includes(f.layer)))throw Error('外层静态铜尚不支持');
  const inBoard=(p,r)=>{
    const x=p[0]*.0254,y=p[1]*.0254,m=(r+20)*.0254,w=intent.board.width,h=intent.board.height,c=intent.board.cornerRadius;
    if(x<m||x>w-m||y<m||y>h-m)return false;
    return [[c,c,-1,-1],[w-c,c,1,-1],[c,h-c,-1,1],[w-c,h-c,1,1]].every(([cx,cy,sx,sy])=>sx*(x-cx)<=0||sy*(y-cy)<=0||Math.hypot(x-cx,y-cy)<=c-m);
  };
  const safeSegment=(net,layer,a,b,width)=>{
    const r=width/2,e=1e-5;
    return inBoard(a,r)&&inBoard(b,r)
      &&pads.every(p=>p.net===net||![layer,12].includes(p.layer)||segmentPolygon(a,b,p.poly)>=r+8-e)
      &&tracks.every(t=>t.net===net||t.layer!==layer||segmentSegment(a,b,t.a,t.b)>=r+t.lineWidth/2+8-e)
      &&board.copper.vias.every(v=>v.net===net||pointSegment([v.x,v.y],a,b)>=r+v.diameter/2+8-e)
      &&regions.every(q=>![layer,12].includes(q.layer)||segmentPolygon(a,b,q.poly)>=r+q.lineWidth/2-e)
      &&cuts.every(q=>segmentPolygon(a,b,q.poly)>=r+q.lineWidth/2+20-e);
  };
  return {pads,tracks,regions,cuts,safeSegment,inBoard};
}
