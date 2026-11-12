import {readFileSync,writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';

// 只验证导入后的身份和焊盘网络；铜是否真的连通必须另做原生 DRC。
const [schematicPath,boardPath,outputPath]=process.argv.slice(2);
if(!outputPath)throw new Error('用法：node verify-pcb-connectivity.mjs schematic-ir.json board.json output.json');
const load=p=>JSON.parse(readFileSync(p,'utf8'));
const sch=load(schematicPath),board=load(boardPath);
const requireState=(ok,message)=>{if(!ok)throw new Error(message);};
requireState(board.capture?.identityVerified===true && board.capture.projectUuid===sch.projectId,'快照身份缺失或工程不一致');
requireState(board.capture.documentType==='pcb' && board.componentsAvailability==='available','不是完整 PCB 器件快照');
requireState(board.components.length===sch.components.length,'PCB 和原理图器件数量不同');
const unique=items=>new Set(items).size===items.length;
requireState(unique(board.components.map(c=>c.designator)) && unique(board.components.map(c=>c.uniqueId)),'PCB 位号或跨文档实例身份重复');
const names=new Map(sch.nets.map(n=>[n.id,n.name]));
let padCount=0,ncCount=0;
const netCounts=new Map();
for(const c of sch.components){
  const pcb=board.components.find(p=>p.designator===c.ref);
  requireState(pcb && pcb.uniqueId===c.uniqueId,'器件实例身份不一致：'+c.ref);
  requireState(pcb.availability?.pads==='available' && pcb.pads.length===c.pins.length,'焊盘缺失或数量不同：'+c.ref);
  requireState(unique(pcb.pads.map(p=>p.padNumber)),'焊盘编号重复：'+c.ref);
  for(const pin of c.pins){
    const pad=pcb.pads.find(p=>p.padNumber===pin.number);
    const edges=sch.connections.filter(e=>e.componentId===c.id && e.pinNumber===pin.number);
    requireState(edges.length<=1 && (edges.length===1 || pin.noConnected===true),'原理图连接不明确：'+c.ref+':'+pin.number);
    requireState(!(edges.length && pin.noConnected),'原理图 NC 与连接冲突');
    const net=edges.length?names.get(edges[0].netId):'';
    requireState(typeof net==='string' && pad && typeof pad.net==='string' && pad.net===net,'焊盘网络不同：'+c.ref+':'+pin.number);
    padCount++;
    if(pin.noConnected)ncCount++;
    if(net)netCounts.set(net,(netCounts.get(net)??0)+1);
  }
}
const sha=p=>createHash('sha256').update(readFileSync(p)).digest('hex');
const report={status:'live-verified',scope:'schematic-to-PCB component identity and pad-net assignment; does not prove routed copper connectivity',projectId:sch.projectId,documentId:board.capture.documentUuid,parts:board.components.length,pads:padCount,explicitNoConnect:ncCount,nets:netCounts.size,inputs:{schematic:{path:schematicPath,sha256:sha(schematicPath)},pcb:{path:boardPath,sha256:sha(boardPath)}},hardwareTested:false,pcbManufacturingAccepted:false};
writeFileSync(outputPath,JSON.stringify(report,null,2)+'\n',{flag:'wx'});
console.log(report);
