import {readFileSync,writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';

const [observedPath,netsPath,out,...sourcePaths]=process.argv.slice(2);
if(!out || !sourcePaths.length) throw new Error('用法：node verify-project-connectivity.mjs observed.json nets.json out.json source-composition.json ...');
const load=p=>JSON.parse(readFileSync(p,'utf8'));
const observed=load(observedPath), audit=load(netsPath), sources=sourcePaths.map(p=>load(p).connectivity);
const requireState=(c,m)=>{if(!c)throw new Error(m);};
const expected=sources.flatMap(s=>s.components), actual=observed.components;
requireState(sources.every(s=>s.projectId===observed.projectId),'工程不一致');
requireState(actual.length===expected.length && new Set(actual.map(c=>c.ref)).size===actual.length,'器件集合或位号不一致');
requireState(new Set(actual.map(c=>c.uniqueId)).size===actual.length,'原生实例ID重复');
const netMap=s=>new Map(s.nets.map(n=>[n.id,n.name]));
const expectedPins=new Map(),actualPins=new Map();
function addPins(ir,dest) {
  const names=netMap(ir);
  for(const c of ir.components) {
    requireState(new Set(c.pins.map(p=>p.number)).size===c.pins.length,'物理引脚重复');
    for(const p of c.pins) {
      const edges=ir.connections.filter(e=>e.componentId===c.id && e.pinNumber===p.number);
      requireState(edges.length<=1,'重复连接');
      requireState(edges.length===1 || p.noConnected===true,'未说明的悬空引脚');
      requireState(!(edges.length && p.noConnected),'NC和连接并存');
      const net=edges.length?names.get(edges[0].netId):'';
      requireState(!edges.length || typeof net==='string','未知网络');
      dest.set(c.ref+':'+p.number,{name:p.name,net,nc:p.noConnected===true});
    }
  }
}
for(const s of sources)addPins(s,expectedPins);
addPins(observed,actualPins);
for(const c of expected) {
  const a=actual.find(q=>q.ref===c.ref);
  requireState(a && a.id===c.id && a.uniqueId===c.uniqueId && a.device.deviceUuid===c.device.deviceUuid && a.device.libraryUuid===c.device.libraryUuid && a.footprint===c.footprint,'身份或封装不一致：'+c.ref);
}
requireState(expectedPins.size===actualPins.size,'引脚总数不一致');
for(const [k,p] of expectedPins)requireState(JSON.stringify(p)===JSON.stringify(actualPins.get(k)),'逐脚网络不一致：'+k);
const netGroups=new Map();
for(const [key,p] of actualPins)if(p.net){if(!netGroups.has(p.net))netGroups.set(p.net,[]);netGroups.get(p.net).push(key);}
requireState(audit.ok===true && audit.totalNets===netGroups.size,'官方全工程网表审计缺失或不一致');
for(const [net,pins] of netGroups) {
  const a=audit.nets.find(n=>n.net===net);
  requireState(a && a.pins===pins.length,'官方网表脚数不一致：'+net);
  requireState(pins.length>=2,'单引脚网络：'+net);
}
const sha=p=>createHash('sha256').update(readFileSync(p)).digest('hex');
const result={status:'live-verified',scope:'project schematic connectivity and native netlist only',projectId:observed.projectId,
  parts:actual.length,physicalPins:actualPins.size,explicitNoConnect:[...actualPins.values()].filter(p=>p.nc).length,nets:netGroups.size,
  allNetsHaveMultiplePins:true,inputs:Object.fromEntries([observedPath,netsPath,...sourcePaths].map(p=>[p,sha(p)])),hardwareTested:false,pcbManufacturingAccepted:false};
writeFileSync(out,JSON.stringify(result,null,2)+'\n',{flag:'wx'});
console.log(result);
