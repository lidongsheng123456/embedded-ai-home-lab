import {readFileSync,writeFileSync} from 'node:fs';

// 对官方 OBJ 做正交投影，不补画器件；画家排序适于目视复核，不能作为碰撞求解器。
const [objPath,mtlPath,out]=process.argv.slice(2);
if(!out)throw new Error('用法：node render-obj-preview.mjs board.obj board.mtl preview.svg');
const materials=new Map();
let material;
for(const line of readFileSync(mtlPath,'utf8').split('\n')){
  const [key,...values]=line.trim().split(/\s+/);
  if(key==='newmtl'){material={color:[0.5,0.5,0.5]};materials.set(values[0],material);}
  if(key==='Kd' && material)material.color=values.map(Number);
}
const vertices=[],faces=[];
let current='';
for(const line of readFileSync(objPath,'utf8').split('\n')){
  const [key,...values]=line.trim().split(/\s+/);
  if(key==='v')vertices.push(values.map(Number));
  if(key==='usemtl')current=values[0];
  if(key==='f'){
    const points=values.map(v=>vertices[Number(v.split('/')[0])-1]);
    if(points.some(p=>!p || p.length!==3 || p.some(v=>!Number.isFinite(v))))throw new Error('非法 OBJ 顶点');
    for(let i=1;i+1<points.length;i++)faces.push({p:[points[0],points[i],points[i+1]],material:current});
  }
}
if(!faces.length)throw new Error('OBJ 没有面');
const transforms=[
  {name:'TOP',project:([x,y,z])=>[x,-y,z]},
  {name:'BOTTOM',project:([x,y,z])=>[-x,-y,-z]},
  {name:'ASSEMBLY',project:([x,y,z])=>[(x-y)*Math.SQRT1_2,-(x+y)/Math.sqrt(6)-z*Math.sqrt(2/3),(x+y+z)/Math.sqrt(3)]},
];
const svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1800" height="750" viewBox="0 0 1800 750"><rect width="1800" height="750" fill="#edf1f5"/>'];
for(const [index,view] of transforms.entries()){
  // 完整丝印模型可能有数十万顶点，逐点累积避免函数参数数量超限。
  let minX=Infinity,maxX=-Infinity,minY=Infinity,maxY=-Infinity;
  for(const vertex of vertices){
    const [x,y]=view.project(vertex);
    minX=Math.min(minX,x);maxX=Math.max(maxX,x);
    minY=Math.min(minY,y);maxY=Math.max(maxY,y);
  }
  const scale=Math.min(550/(maxX-minX),620/(maxY-minY)),dx=index*600+300-(minX+maxX)/2*scale,dy=365-(minY+maxY)/2*scale;
  const projected=faces.map(f=>({...f,q:f.p.map(view.project)}));
  projected.sort((a,b)=>a.q.reduce((s,p)=>s+p[2],0)-b.q.reduce((s,p)=>s+p[2],0));
  for(const f of projected){
    const [a,b,c]=f.p,ab=b.map((v,k)=>v-a[k]),ac=c.map((v,k)=>v-a[k]);
    const normal=[ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]];
    const length=Math.hypot(...normal);
    if(length<1e-10)continue;
    const facing=index===0?normal[2]:index===1?-normal[2]:normal[0]+normal[1]+normal[2];
    if(facing<=1e-12)continue;
    const light=.64+.36*Math.abs((normal[0]*.3-normal[1]*.4+normal[2]*.866)/length);
    const rgb=(materials.get(f.material)?.color??[.5,.5,.5]).map(v=>Math.round(Math.max(0,Math.min(1,v*light))*255));
    const xy=f.q.map(p=>[(p[0]*scale+dx).toFixed(2),(p[1]*scale+dy).toFixed(2)].join(',')).join(' ');
    svg.push(`<polygon points="${xy}" fill="rgb(${rgb.join(',')})"/>`);
  }
  svg.push(`<text x="${index*600+300}" y="700" text-anchor="middle" font-family="Arial" font-size="20" fill="#253448">${view.name}</text>`);
}
svg.push('<text x="900" y="735" text-anchor="middle" font-family="Arial" font-size="15" fill="#536176">Official OBJ projection / layout review only / external modules and cables excluded</text></svg>');
writeFileSync(out,svg.join('\n'),{flag:'wx'});
console.log({vertices:vertices.length,triangles:faces.length,output:out});
