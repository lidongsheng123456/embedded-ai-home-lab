import assert from 'node:assert/strict';
import {compileUnwiredPage} from './compile-unwired-page.mjs';
// 合成最小库存使拒绝边界可在没有本机忽略目录、没有EDA连接时复现。
const device={libraryUuid:'library',uuid:'1234567890abcdef1234567890abcdef'};
const component={primitiveId:'part',componentType:'part',uniqueId:'native-id',designator:'J1',device,
  x:0,y:0,rotation:0,mirror:false,bbox:{minX:-5,minY:-5,maxX:5,maxY:5},pinsAvailable:true,
  pins:[{primitiveId:'pin',pinNumber:'1',pinName:'1',x:10,y:0,net:'',noConnected:false,otherProperty:{}}],
  name:'header',subPartName:'header.1',addIntoBom:true,addIntoPcb:true,manufacturer:'factory',manufacturerId:'header',supplier:'supplier',supplierId:'C1',
  otherProperty:{'EasyEDA Agent Component ID':'cmp-J1','EasyEDA Agent Role':'interface'},component:{},symbol:{},footprint:{}};
const pagePrimitives=Object.fromEntries(['arcs','attributes','buses','circles','components','objects','pins','polygons','rectangles','texts','wires'].map(k=>[k,[]]));
pagePrimitives.components=[{primitiveId:'part',componentType:'part'}];
pagePrimitives.pins=[{primitiveId:'pin',componentId:'part'}];
pagePrimitives.attributes=[{primitiveId:'attr',ParentPrimitiveId:'pin'}];
const before={ok:true,context:{projectUuid:'project',documentUuid:'page'},result:{wiresAvailable:true,wires:[],pagePrimitives,components:[component]}};
const plan={paperless:true,connectivity:{projectId:'project',documentId:'page',
  components:[{id:'cmp-J1',ref:'J1',uniqueId:'native-id',role:'interface',device:{libraryUuid:'library',deviceUuid:device.uuid},pins:[{number:'1',name:'1'}]}],
  modules:[{id:'interface',coreComponents:['cmp-J1']}],nets:[{name:'GND',role:'ground'}]},
  layout:{documentId:'page',placements:[{designator:'J1',x:0,y:0,rotation:0,mirror:false,bbox:component.bbox,pins:[{number:'1',name:'1',x:10,y:0,net:'GND'}]}],wires:[],flags:[],frames:[]}};
const good=compileUnwiredPage(plan,before);
assert.equal(good.steps[1].expectSchematic.pagePrimitives.pins.length,1);
assert(!good.steps.some(s=>/delete|clear|place/.test(s.action??'')));
const rejects=edit=>{const b=structuredClone(before);edit(b);assert.throws(()=>compileUnwiredPage(plan,b));};
rejects(b=>b.context.projectUuid='wrong');
rejects(b=>b.context.documentUuid='wrong');
rejects(b=>b.result.pagePrimitives.texts.push({primitiveId:'extra'}));
rejects(b=>delete b.result.pagePrimitives.pins);
rejects(b=>b.result.components[0].pins.pop());
rejects(b=>b.result.components[0].pins.push({...b.result.components[0].pins[0]}));
rejects(b=>b.result.components[0].pins[0].net='unexpected');
rejects(b=>b.result.components[0].pins[0].noConnected=true);
rejects(b=>b.result.components[0].device.uuid='wrong');
rejects(b=>b.result.components[0].uniqueId='wrong');
rejects(b=>b.result.pagePrimitives.pins[0].componentId='wrong');
rejects(b=>b.result.pagePrimitives.attributes[0].ParentPrimitiveId='wrong');
rejects(b=>b.result.wires.push({primitiveId:'extra'}));
rejects(b=>b.result.components[0].pins[0].x+=5);
console.log('通过：完整库存保留与14项拒绝边界');
