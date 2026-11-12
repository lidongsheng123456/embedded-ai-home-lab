import {readFileSync,writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';

// 只呈现快照中真实铜与铣孔，不补画未布通连接；此图不是官方画布截图。
const [input,out]=process.argv.slice(2);
if(!out)throw Error('用法：node render-pcb-copper.mjs board.json output.svg');
const board=JSON.parse(readFileSync(input,'utf8'));
if(!board.capture?.identityVerified||!Object.values(board.copper.availability).every(v=>v==='available'))throw Error('板级快照缺测');
const esc=s=>String(s).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('"','&quot;');
const nativePath=(source,closed=true)=>{
  if(Array.isArray(source[0]))return source.map(s=>nativePath(s,closed)).join(' ');
  let x=source[0],y=source[1],d=`M${x},${y}`,k=2;
  while(k<source.length){
    const type=source[k];
    if(type==='L'){k++;continue;}
    if(type==='ARC'){
      const sweep=source[k+1],nx=source[k+2],ny=source[k+3];
      const r=Math.hypot(nx-x,ny-y)/(2*Math.sin(Math.abs(sweep)*Math.PI/360));
      if(!Number.isFinite(r)||r<=0)throw Error('不支持退化圆弧');
      d+=` A${r},${r} 0 ${Math.abs(sweep)>180?1:0},${sweep>0?1:0} ${nx},${ny}`;
      x=nx;y=ny;k+=4;
    }else if(Number.isFinite(type)&&Number.isFinite(source[k+1])){
      x=source[k];y=source[k+1];d+=` L${x},${y}`;k+=2;
    }else throw Error('未知原生路径标记');
  }
  return d+(closed?' Z':'');
};
const pad=p=>{
  const s=p.shape,a=p.rotation??0,tr=`translate(${p.x},${p.y}) rotate(${a})`;
  if(s[0]==='POLYGON')return `<path d="${nativePath(s[1])}"/>`;
  if(s[0]==='ELLIPSE')return `<ellipse transform="${tr}" rx="${s[1]/2}" ry="${s[2]/2}"/>`;
  if(['RECT','OVAL'].includes(s[0]))return `<rect transform="${tr}" x="${-s[1]/2}" y="${-s[2]/2}" width="${s[1]}" height="${s[2]}" rx="${s[0]==='OVAL'?Math.min(s[1],s[2])/2:s[3]??0}"/>`;
  throw Error('未知焊盘形状');
};
const w=100/.0254,h=w,gap=300,panelW=w+gap,panelH=h+gap;
const svg=[`<svg xmlns="http://www.w3.org/2000/svg" width="2200" height="2300" viewBox="-150 -220 ${panelW*2} ${panelH*2+200}"><rect x="-150" y="-220" width="${panelW*2}" height="${panelH*2+200}" fill="#f3f5f7"/><style>text{font-family:Arial,sans-serif}</style>`];
for(const [index,layer] of [1,2,15,16].entries()){
  const x=index%2*panelW,y=Math.floor(index/2)*panelH,color=layer===1?'#b74c36':layer===2?'#345fa7':'#b48c37';
  svg.push(`<g transform="translate(${x},${y})"><text x="0" y="-65" font-size="65" fill="#273748">${({1:'TOP / L1',2:'BOTTOM / L4',15:'INNER1 / L2 · GND',16:'INNER2 / L3 · GND'})[layer]}</text><g transform="translate(0,${h}) scale(1,-1)"><path d="M${board.outline.points.map(p=>p.join(',')).join(' L')} Z" fill="white" stroke="#7b8c96" stroke-width="4"/>`);
  for(const p of board.copper.poured.filter(p=>p.layer===layer))for(const f of p.fills){
    if(f.fill===true)svg.push(`<path d="${nativePath(f.source)}" fill="${color}" fill-rule="evenodd"/>`);
    else if(f.fill===false&&f.geometryKind==='stroked-thermal-spoke-path'&&f.lineWidth>0)svg.push(`<path d="${nativePath(f.source,false)}" fill="none" stroke="${color}" stroke-width="${f.lineWidth}" stroke-linecap="round"/>`);
    else throw Error('未知铺铜几何');
  }
  svg.push(`<g fill="${color}">`);
  for(const f of board.copper.fills.filter(f=>f.layer===layer))svg.push(`<path d="${nativePath(f.source)}"/>`);
  for(const c of board.components)for(const p of c.pads.filter(p=>p.layer===layer||p.layer===12))svg.push(pad(p));
  svg.push('</g>');
  for(const t of board.copper.lines.filter(t=>t.layer===layer))svg.push(`<path d="M${t.startX},${t.startY} L${t.endX},${t.endY}" fill="none" stroke="${color}" stroke-width="${t.lineWidth}" stroke-linecap="round"/>`);
  if(board.copper.arcs.length)throw Error('未实现圆弧走线显示');
  for(const v of board.copper.vias)svg.push(`<circle cx="${v.x}" cy="${v.y}" r="${v.diameter/2}" fill="${color}"/><circle cx="${v.x}" cy="${v.y}" r="${v.holeDiameter/2}" fill="white"/>`);
  for(const f of board.copper.fills.filter(f=>f.layer===12))svg.push(`<path d="${nativePath(f.source)}" fill="#f3f5f7" stroke="#7b8c96" stroke-width="4"/>`);
  svg.push('</g></g>');
}
const sha=createHash('sha256').update(readFileSync(input)).digest('hex');
svg.push(`<text x="0" y="${panelH*2-100}" font-size="39" fill="#344454">HomeGuard Mini R1 · actual copper readback · ${esc(board.capturedAt??'')}</text><text x="0" y="${panelH*2-40}" font-size="31" fill="#596575">Coordinates: top view for all layers. Not a manufacturing approval. SHA256 ${sha}</text></svg>`);
writeFileSync(out,svg.join('\n'),{flag:'wx'});
console.log({out,tracks:board.copper.lines.length,vias:board.copper.vias.length,pours:board.copper.pours.length});
