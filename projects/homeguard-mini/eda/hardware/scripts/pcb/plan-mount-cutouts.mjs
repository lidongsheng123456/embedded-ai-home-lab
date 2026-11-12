import {readFileSync,writeFileSync} from 'node:fs';
import {linearSource} from './pcb-copper-geometry.mjs';

// 板材铣孔没有钻孔焊盘身份；提高轮廓分辨率，保证弦边内径不小于 3.2 mm。
const [boardPath,intentPath,out]=process.argv.slice(2);
if(!out)throw Error('用法：node plan-mount-cutouts.mjs board.json intent.json output-prefix');
const read=p=>JSON.parse(readFileSync(p,'utf8')),b=read(boardPath),i=read(intentPath);
if(!b.capture?.identityVerified||b.capture.documentUuid!==i.documentId||b.copper.availability.fills!=='available')throw Error('快照不完整');
const fills=b.copper.fills.filter(f=>f.layer===12);
if(fills.length!==4)throw Error('需要恰好四个旧铣孔');
const holes=fills.map(f=>{
  const p=linearSource(f.source),xs=p.map(v=>v[0]),ys=p.map(v=>v[1]);
  const x=(Math.min(...xs)+Math.max(...xs))/2,y=(Math.min(...ys)+Math.max(...ys))/2;
  if(!i.mechanical.mountCenters.some(c=>Math.hypot(c[0]-x*.0254,c[1]-y*.0254)<.001))throw Error('旧孔中心与设计不同');
  if(p.length!==16)throw Error('拒绝覆盖非旧版16边铣孔');
  const count=96,r=i.mechanical.mountDiameter/2/Math.cos(Math.PI/count)/.0254;
  const points=Array.from({length:count},(_,k)=>[x+r*Math.cos(k*2*Math.PI/count),y+r*Math.sin(k*2*Math.PI/count)].map(v=>+v.toFixed(4)));
  return {oldId:f.primitiveId,center:[x,y],points};
});
const steps=[{id:'replace-four-coarse-cutouts',action:'pcb.fill.delete',payload:{primitiveIds:holes.map(h=>h.oldId)}}];
for(const [k,h] of holes.entries())steps.push({id:'cutout-'+k,run:'pcb fill create',flags:{points:JSON.stringify(h.points),layer:12,locked:true}});
steps.push({id:'save-cutouts',action:'pcb.save',assert:{'$.saved':'true'}});
writeFileSync(out+'.json',JSON.stringify({status:'offline-verified',holes,minimumChordDiameterMm:3.2,maximumVertexDiameterMm:3.2/Math.cos(Math.PI/96),isDrillPad:false},null,2)+'\n',{flag:'wx'});
writeFileSync(out+'.apply.json',JSON.stringify({version:1,meta:{name:'96-side 3.2mm mounting cutouts',project:i.projectId,doc:i.documentId},steps},null,2)+'\n',{flag:'wx'});
console.log({holes:holes.length,steps:steps.length});
