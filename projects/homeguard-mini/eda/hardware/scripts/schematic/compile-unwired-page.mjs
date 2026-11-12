import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { pathToFileURL } from 'node:url';

const classes = ['arcs','attributes','buses','circles','components','objects','pins','polygons','rectangles','texts','wires'];
const instanceFields = ['uniqueId','name','subPartName','addIntoBom','addIntoPcb','manufacturer','manufacturerId','supplier','supplierId','otherProperty','component','symbol','footprint'];
const requireState = (condition, message) => { if (!condition) throw new Error(message); };
const equal = (a,b) => JSON.stringify(a) === JSON.stringify(b);
const close = (a,b) => Number.isFinite(a) && Number.isFinite(b) && Math.abs(a-b)<1e-6;
const pick = (object, keys) => Object.fromEntries(keys.map(k => {
  requireState(Object.hasOwn(object,k), '缺少状态：'+k); return [k,object[k]];
}));

// 只接受完整未接线页和经正式 compose 重新生成的计划；不支持清页、NC 或姿态变换。
export function compileUnwiredPage(plan, before) {
  const {projectId:project,documentId:doc} = plan.connectivity;
  const r=before.result, layout=plan.layout;
  requireState(before.ok && before.context?.projectUuid===project && before.context?.documentUuid===doc,'工程或页面不一致');
  requireState(plan.paperless===true && layout.documentId===doc,'仅支持已去图框页');
  requireState(r.wiresAvailable===true && Array.isArray(r.wires) && r.wires.length===0,'页面已有导线或缺测');
  requireState(equal(Object.keys(r.pagePrimitives??{}).sort(),classes),'完整11类图元清单必需');
  const page=r.pagePrimitives, seen=new Set();
  for(const key of classes) {
    requireState(Array.isArray(page[key]),'图元缺测：'+key);
    for(const row of page[key]) {
      requireState(row.primitiveId && !seen.has(row.primitiveId),'图元身份缺失或重复'); seen.add(row.primitiveId);
    }
    if(!['components','attributes','pins'].includes(key)) requireState(page[key].length===0,'页面含额外图形：'+key);
  }
  requireState(r.components.length===plan.connectivity.components.length && r.components.length===layout.placements.length,'器件集合不一致');
  requireState(page.components.length===r.components.length,'图元器件集合不一致');
  const refs=new Set(), ids=new Set(), pinOwners=new Map();
  for(const c of r.components) {
    requireState(c.componentType==='part' && !refs.has(c.designator) && !ids.has(c.primitiveId),'存在非器件或重复位号');
    refs.add(c.designator); ids.add(c.primitiveId);
    const target=plan.connectivity.components.find(q=>q.ref===c.designator);
    const p=layout.placements.find(q=>q.designator===c.designator);
    requireState(target && p && c.uniqueId===target.uniqueId && c.device?.uuid===target.device.deviceUuid && c.device.libraryUuid===target.device.libraryUuid,'器件库或实例身份不一致');
    requireState(c.otherProperty['EasyEDA Agent Component ID']===target.id && c.otherProperty['EasyEDA Agent Role']===target.role,'稳定ID或功能归属不一致');
    requireState(c.rotation===p.rotation && c.mirror===p.mirror,'未测量姿态被拒绝');
    requireState(c.pinsAvailable===true && c.pins.length===target.pins.length && c.pins.length===p.pins.length,'物理引脚缺失或新增');
    const numbers=new Set();
    for(const pin of c.pins) {
      const t=target.pins.find(q=>q.number===pin.pinNumber), g=p.pins.find(q=>q.number===pin.pinNumber);
      requireState(!numbers.has(pin.pinNumber) && pin.primitiveId && !pinOwners.has(pin.primitiveId),'重复引脚'); numbers.add(pin.pinNumber);
      requireState(t && g && t.name===pin.pinName && g.name===pin.pinName && pin.net==='' && pin.noConnected===false && t.noConnected!==true,'引脚名称、NC或未接线状态不一致');
      requireState(close(pin.x-c.x,g.x-p.x) && close(pin.y-c.y,g.y-p.y),'引脚几何不一致');
      pinOwners.set(pin.primitiveId,c.primitiveId);
    }
    pick(c,instanceFields);
  }
  requireState(page.pins.length===pinOwners.size && page.pins.every(p=>pinOwners.get(p.primitiveId)===p.componentId),'引脚库存归属不一致');
  requireState(page.components.every(c=>ids.has(c.primitiveId) && c.componentType==='part'),'器件库存身份不一致');
  requireState(page.attributes.every(a=>ids.has(a.ParentPrimitiveId) || pinOwners.has(a.ParentPrimitiveId)),'存在孤立属性');
  const expectation=(moved,connected)=>({exactParts:true,parts:Object.fromEntries(r.components.map(c=>{
    const p=layout.placements.find(q=>q.designator===c.designator);
    return [c.designator,{primitiveId:c.primitiveId,device:pick(c.device,['libraryUuid','uuid']),
      ...pick(moved?p:c,['x','y','rotation','mirror','bbox']),instance:pick(c,instanceFields),
      pins:Object.fromEntries(c.pins.map(pin=>{
        const g=p.pins.find(q=>q.number===pin.pinNumber);
        return [pin.pinNumber,{name:pin.pinName,x:moved?g.x:pin.x,y:moved?g.y:pin.y,
          net:connected?g.net:'',noConnected:false,persistent:pick(pin,['otherProperty','pinNumber','primitiveId'])}];
      }))}];
  }))});
  const read={includePins:true,includeBBox:true,includeDeviceIdentity:true,includeWires:true,includePagePrimitives:true,includeConnectivitySummary:true};
  const steps=[{id:'unique-designators',action:'schematic.components.list',payload:{allPages:true,tagPages:true},expectSchematic:{designatorsOnly:true,parts:Object.fromEntries(r.components.map(c=>[c.designator,{primitiveId:c.primitiveId,pins:null}]))}},
    {id:'complete-source-inventory',action:'schematic.components.list',payload:read,expectSchematic:{...expectation(false,false),pagePrimitives:page}}];
  for(const c of r.components) {
    const p=layout.placements.find(p=>p.designator===c.designator);
    steps.push({id:'move-'+c.designator,action:'schematic.component.modify',payload:{primitiveId:c.primitiveId,preserveInstance:true,patch:pick(p,['x','y','rotation','mirror'])},assert:{'$.instancePreserved':'==true'}});
  }
  steps.push({id:'physical-pins-before-wiring',action:'schematic.components.list',payload:read,expectSchematic:{...expectation(true,false),drawing:{wires:[],flags:[]}}});
  for(const [i,w] of layout.wires.entries()) steps.push({id:'wire-'+i,action:'schematic.wire.create',payload:{points:w.points}});
  for(const [i,f] of layout.flags.entries()) steps.push({id:'marker-'+i,action:'schematic.power.connect_pin',payload:pick(f,['direction','kind','net','offset','pinX','pinY'])});
  for(const m of plan.connectivity.modules) steps.push({id:'group-'+m.id,run:'sch group create',flags:{'if-absent':true,name:m.id,members:plan.connectivity.components.filter(c=>[...m.coreComponents,...(m.peripheralComponents??[])].includes(c.id)).map(c=>c.ref).join(',')}});
  const frameData=JSON.stringify({schemaVersion:1,documentId:doc,frames:layout.frames});
  steps.push({id:'frames',run:'sch frame apply',flags:{data:frameData}},{id:'check-frames',run:'sch frame check',flags:{data:frameData}});
  const final={...expectation(true,true),drawing:{wires:layout.wires,flags:layout.flags},ownership:{componentIds:Object.fromEntries(plan.connectivity.components.map(c=>[c.ref,c.id])),modules:plan.connectivity.modules,netRoles:Object.fromEntries(plan.connectivity.nets.map(n=>[n.name,n.role]))}};
  steps.push({id:'all-pins-and-real-drawing',action:'schematic.components.list',payload:read,expectSchematic:final},
    {id:'electrical',action:'schematic.check',assert:{'$.passed':'true'}},{id:'gate',run:'sch gate',flags:{json:true}},
    {id:'save',action:'schematic.save',assert:{'$.saved':'true'}},
    {id:'saved-instance-verification',action:'schematic.components.list',payload:read,expectSchematic:final});
  return {version:1,requireFullExecution:true,meta:{name:'Complete unwired page with full native inventory',project,doc},defaults:{timeoutSec:90,retry:0,continueOnError:false},steps};
}

if(process.argv[1] && import.meta.url===pathToFileURL(process.argv[1]).href) {
  const [planPath,beforePath,out]=process.argv.slice(2);
  requireState(out,'用法：node compile-unwired-page.mjs plan.json before.json queue.json');
  const load=p=>JSON.parse(readFileSync(p,'utf8'));
  const queue=compileUnwiredPage(load(planPath),load(beforePath));
  const inputs=Object.fromEntries([planPath,beforePath].map(p=>[p,createHash('sha256').update(readFileSync(p)).digest('hex')]));
  writeFileSync(out+'.inputs.json',JSON.stringify(inputs,null,2)+'\n',{flag:'wx'});
  writeFileSync(out,JSON.stringify(queue,null,2)+'\n',{flag:'wx'});
  console.log({steps:queue.steps.length,out});
}
