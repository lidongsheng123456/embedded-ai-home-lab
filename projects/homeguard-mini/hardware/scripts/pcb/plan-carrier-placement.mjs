import {readFileSync,writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';

// 所有投影从官方实测锚点/边界/焊盘计算，不能把外形中心当成库原点。
const [boardPath,intentPath,out]=process.argv.slice(2);
if(!out)throw new Error('用法：node plan-carrier-placement.mjs board.json intent.json output-prefix');
const load=p=>JSON.parse(readFileSync(p,'utf8'));
const board=load(boardPath),intent=load(intentPath),scale=1/0.0254;
const requireState=(ok,m)=>{if(!ok)throw new Error(m);};
requireState(board.capture.projectUuid===intent.projectId && board.capture.documentUuid===intent.documentId,'工程或PCB不一致');
requireState(board.components.length===Object.keys(intent.placements).length,'器件集合不一致');
requireState(board.routedLines===0 && board.copper.vias.length===0,'只允许尚未布线的板');
const rotate=(x,y,degrees)=>{
  const n=((degrees%360)+360)%360;
  if(n===0)return [x,y];if(n===90)return [-y,x];if(n===180)return [-x,-y];if(n===270)return [y,-x];
  throw new Error('只允许直角姿态');
};
const parts=board.components.map(c=>{
  const [cx,cy,rotation]=intent.placements[c.designator],delta=rotation-c.rotation;
  requireState(c.bbox && c.pads.length>0 && c.layer===1,'器件几何缺测或非顶面');
  const center=[(c.bbox.minX+c.bbox.maxX)/2,(c.bbox.minY+c.bbox.maxY)/2];
  const move=(x,y)=>{const [rx,ry]=rotate(x-center[0],y-center[1],delta);return [cx*scale+rx,cy*scale+ry];};
  let [x,y]=move(c.x,c.y);
  let pins=c.pads.map(p=>{const [px,py]=move(p.x,p.y);return {...p,x:px,y:py,rotation:((p.rotation??0)+delta+360)%360};});
  const target=intent.mechanical.headerPin1Targets[c.designator];
  let dx=0,dy=0;
  if(target){const p=pins.find(p=>(p.padNumber??p.number)==='1');requireState(p,'母座1脚缺失');dx=target[0]*scale-p.x;dy=target[1]*scale-p.y;x+=dx;y+=dy;pins=pins.map(p=>({...p,x:p.x+dx,y:p.y+dy}));}
  const corners=[[c.bbox.minX,c.bbox.minY],[c.bbox.minX,c.bbox.maxY],[c.bbox.maxX,c.bbox.minY],[c.bbox.maxX,c.bbox.maxY]].map(([x,y])=>move(x,y)).map(([x,y])=>[x+dx,y+dy]);
  const bbox={minX:Math.min(...corners.map(p=>p[0])),minY:Math.min(...corners.map(p=>p[1])),maxX:Math.max(...corners.map(p=>p[0])),maxY:Math.max(...corners.map(p=>p[1]))};
  return {...c,x,y,rotation,pads:pins,bbox};
});
const gap=(a,b)=>Math.hypot(Math.max(a.minX-b.maxX,b.minX-a.maxX,0),Math.max(a.minY-b.maxY,b.minY-a.maxY,0))/scale;
const issues=[];
for(let i=0;i<parts.length;i++)for(let j=i+1;j<parts.length;j++) {
  const g=gap(parts[i].bbox,parts[j].bbox);
  if(g<intent.mechanical.minimumComponentGap-1e-6)issues.push({kind:'component-gap',a:parts[i].designator,b:parts[j].designator,gapMm:g});
}
const toMil=b=>Object.fromEntries(Object.entries(b).map(([k,v])=>[k,v*scale]));
// 插座外形不包含插头护套。插头尺寸是采购上限，必须单独投影检查。
const plug=intent.mechanical.usbOutputPlug;
requireState(plug?.connector==='J11' && plug.rotation===0,'USB-A 插头包络仅支持已核对的朝下姿态');
const socket=parts.find(c=>c.designator===plug.connector);
requireState(socket?.rotation===plug.rotation,'USB-A 插座姿态与插头包络不符');
const mouth=socket.bbox.minY/scale;
const outputPlugEnvelope={minX:plug.centerX-plug.width/2,maxX:plug.centerX+plug.width/2,minY:mouth-plug.externalLength,maxY:mouth+plug.insertionOverlap};
const mating=intent.mechanical.connectorMatingEnvelopes;
requireState(mating?.inflation>=0 && Array.isArray(mating.connectorRefs),'缺少配对插头投影约束');
const matingEnvelopes=mating.connectorRefs.map(ref=>{
  const c=parts.find(c=>c.designator===ref);requireState(c,'配对插头无对应插座：'+ref);
  const b=c.bbox,d=mating.inflation*scale;
  return {ref,bbox:{minX:b.minX-d,minY:b.minY-d,maxX:b.maxX+d,maxY:b.maxY+d}};
});
for(const e of matingEnvelopes){
  for(const c of parts)if(c.designator!==e.ref && gap(c.bbox,e.bbox)<0.5)issues.push({kind:'connector-mating-envelope',connector:e.ref,ref:c.designator});
  if(gap(e.bbox,toMil(outputPlugEnvelope))<0.5)issues.push({kind:'connector-mating-to-usb-output',connector:e.ref});
}
for(let a=0;a<matingEnvelopes.length;a++)for(let b=a+1;b<matingEnvelopes.length;b++)if(gap(matingEnvelopes[a].bbox,matingEnvelopes[b].bbox)<0.5)issues.push({kind:'mating-envelopes-overlap',a:matingEnvelopes[a].ref,b:matingEnvelopes[b].ref});
requireState(outputPlugEnvelope.minX>=0 && outputPlugEnvelope.maxX<=intent.board.width && outputPlugEnvelope.minY>=0 && outputPlugEnvelope.maxY<=intent.board.height,'USB-A 插头包络出板');
if(gap(toMil(outputPlugEnvelope),toMil(intent.mechanical.devboardKeepout))<0.5 || gap(toMil(outputPlugEnvelope),toMil(intent.mechanical.usbPlugKeepout))<0.5)issues.push({kind:'usb-envelopes-overlap'});
for(const c of parts) {
  const b=c.bbox;
  if(b.minX<0 || b.maxX>intent.board.width*scale || b.maxY>intent.board.height*scale || b.minY<(c.designator==='J10'?-intent.mechanical.usbInputBodyOverhangAllowed*scale:0))issues.push({kind:'off-board',ref:c.designator});
  if(!['J7','J8'].includes(c.designator) && gap(b,toMil(intent.mechanical.devboardKeepout))<0.5)issues.push({kind:'devboard-envelope',ref:c.designator});
  if(!['J7','J8'].includes(c.designator) && gap(b,toMil(intent.mechanical.usbPlugKeepout))<0.5)issues.push({kind:'usb-plug-envelope',ref:c.designator});
  if(c.designator!==plug.connector && gap(b,toMil(outputPlugEnvelope))<0.5)issues.push({kind:'usb-output-plug-envelope',ref:c.designator});
  for(const [x,y] of intent.mechanical.mountCenters) {
    const d=Math.hypot(Math.max(b.minX-x*scale,x*scale-b.maxX,0),Math.max(b.minY-y*scale,y*scale-b.maxY,0))/scale;
    if(d<intent.mechanical.fastenerRadius)issues.push({kind:'fastener-clearance',ref:c.designator,center:[x,y],gapMm:d});
  }
}
const hash=p=>createHash('sha256').update(readFileSync(p)).digest('hex');
const projected={...board,components:parts};
const report={status:issues.length?'candidate-rejected':'offline-verified',scope:'placement bboxes, declared mechanical envelopes; no copper routing proof',outputPlugEnvelope,issues,boardSha256:hash(boardPath),intentSha256:hash(intentPath)};
writeFileSync(out+'.json',JSON.stringify({report,projected},null,2)+'\n',{flag:'wx'});
const esc=t=>String(t).replaceAll('&','&amp;').replaceAll('<','&lt;');
const svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="1200" viewBox="-5 -5 110 110"><rect x="0" y="0" width="100" height="100" rx="3" fill="#193b38" stroke="#81c3ac" stroke-width=".3"/>'];
const keep=intent.mechanical.devboardKeepout;
svg.push(`<rect x="${keep.minX}" y="${100-keep.maxY}" width="${keep.maxX-keep.minX}" height="${keep.maxY-keep.minY}" fill="#547078" opacity=".5"/>`);
for(const k of [intent.mechanical.usbPlugKeepout,outputPlugEnvelope])svg.push(`<rect x="${k.minX}" y="${100-k.maxY}" width="${k.maxX-k.minX}" height="${k.maxY-k.minY}" fill="#a57c32" opacity=".4" stroke="#ffcd72" stroke-width=".2"/>`);
for(const [x,y] of intent.mechanical.mountCenters)svg.push(`<circle cx="${x}" cy="${100-y}" r="4" fill="none" stroke="#7b9691" stroke-width=".2"/><circle cx="${x}" cy="${100-y}" r="1.6" fill="#f6f8fc"/>`);
for(const c of parts){const b=c.bbox;svg.push(`<rect x="${b.minX/scale}" y="${100-b.maxY/scale}" width="${(b.maxX-b.minX)/scale}" height="${(b.maxY-b.minY)/scale}" fill="${c.designator.startsWith('J')?'#36525d':'#4b6b5f'}" stroke="#c3ba9c" stroke-width=".12"/>`);
 for(const p of c.pads)svg.push(`<circle cx="${p.x/scale}" cy="${100-p.y/scale}" r=".25" fill="${p.net==='GND'?'#a8c9e3':'#edce82'}"/>`);
 svg.push(`<text x="${(b.minX+b.maxX)/2/scale}" y="${100-(b.minY+b.maxY)/2/scale}" fill="white" font-size="1.25" text-anchor="middle" font-family="Arial">${esc(c.designator)}</text>`);}
svg.push('</svg>');writeFileSync(out+'.svg',svg.join('\n'),{flag:'wx'});
if(!issues.length){const steps=parts.map(c=>({id:'place-'+c.designator,action:'pcb.component.modify',payload:{primitiveId:c.primitiveId,patch:{x:c.x,y:c.y,rotation:c.rotation}}}));steps.push({id:'save-placement',action:'pcb.save',assert:{'$.saved':'true'}});writeFileSync(out+'.apply.json',JSON.stringify({version:1,meta:{name:'HomeGuard mechanical and functional placement',project:intent.projectId,doc:intent.documentId},steps},null,2)+'\n',{flag:'wx'});}
console.log(report);
if(issues.length)process.exitCode=1;
