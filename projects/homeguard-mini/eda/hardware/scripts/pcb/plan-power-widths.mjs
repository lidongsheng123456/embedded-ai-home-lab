import {readFileSync,writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {copperModel} from './pcb-copper-geometry.mjs';

// 原生路由器按细引脚缩窄整网；逐段计算可用宽度，保持原中心线与连接关系。
const [boardPath,intentPath,out]=process.argv.slice(2);
if(!out)throw Error('用法：node plan-power-widths.mjs board.json intent.json output-prefix');
const read=p=>JSON.parse(readFileSync(p,'utf8')),board=read(boardPath),intent=read(intentPath);
const model=copperModel(board,intent),changed=[],narrow=[];
const targets={'USB_VBUS':40,'FUSED_5V':40,'+5V':40,'USB_MCU_5V':40,'+3V3':24};
for(const t of model.tracks){
  const target=targets[t.net];
  if(!target||t.locked||t.lineWidth>=target-.01)continue;
  const length=Math.hypot(t.b[0]-t.a[0],t.b[1]-t.a[1]);
  const count=Math.max(1,Math.ceil(length/(.25/.0254))),pieces=[];
  for(let k=0;k<count;k++){
    const a=t.a.map((v,j)=>v+(t.b[j]-v)*k/count),b=t.a.map((v,j)=>v+(t.b[j]-v)*(k+1)/count);
    const options=[target,32,24,16,12].filter(w=>w>t.lineWidth+.01&&w<=target);
    const width=options.find(w=>model.safeSegment(t.net,t.layer,a,b,w))??t.lineWidth;
    if(pieces.length&&pieces.at(-1).width===width)pieces.at(-1).b=b;
    else pieces.push({a,b,width});
  }
  if(pieces.some(p=>p.width>t.lineWidth+.01))changed.push({original:t,pieces});
  for(const p of pieces)if(p.width<target-.01)narrow.push({net:t.net,layer:t.layer,width:p.width,lengthMm:Math.hypot(p.b[0]-p.a[0],p.b[1]-p.a[1])*.0254,a:p.a,b:p.b});
}
const steps=[];
if(changed.length)steps.push({id:'remove-source-thin-tracks',action:'pcb.route.delete',payload:{kind:'track',primitiveIds:changed.map(c=>c.original.primitiveId)}});
for(const [i,c] of changed.entries())for(const [j,p] of c.pieces.entries())steps.push({id:`wide-${i}-${j}`,action:'pcb.line.create',payload:{net:c.original.net,layer:c.original.layer,lineWidth:p.width,startX:p.a[0],startY:p.a[1],endX:p.b[0],endY:p.b[1]}});
steps.push({id:'save-widths',action:'pcb.save',assert:{'$.saved':'true'}});
const hash=createHash('sha256').update(readFileSync(boardPath)).digest('hex');
writeFileSync(out+'.json',JSON.stringify({status:'offline-verified',scope:'conservative 8mil clearance for widened track portions; retained original narrow portions depend on prior native DRC',boardSha256:hash,changed,narrow},null,2)+'\n',{flag:'wx'});
writeFileSync(out+'.apply.json',JSON.stringify({version:1,meta:{name:'Power trunk widening from actual copper',project:intent.projectId,doc:intent.documentId},steps},null,2)+'\n',{flag:'wx'});
console.log(JSON.stringify({changed:changed.length,steps:steps.length,narrowSegments:narrow.length,narrowLong:narrow.filter(p=>p.lengthMm>2)}));
