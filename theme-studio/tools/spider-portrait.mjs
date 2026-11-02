// Authored joint positions; only the compiled four-bit sprite sheets enter the firmware.
const W=32,H=40;
const poses=[
 {name:'watch',head:[17,17],ls:[12,24],rs:[21,23],hip:[18,30],le:[10,30],lh:[13,38],re:[26,27],rh:[28,32],lk:[7,28],lf:[5,38],rk:[25,32],rf:[28,38]},
 {name:'breath',head:[17,16],ls:[12,23],rs:[21,22],hip:[18,30],le:[10,30],lh:[13,38],re:[26,27],rh:[28,32],lk:[7,28],lf:[5,38],rk:[25,32],rf:[28,38]},
 {name:'coil',head:[17,20],ls:[12,26],rs:[21,26],hip:[16,31],le:[9,31],lh:[10,37],re:[25,31],rh:[28,37],lk:[8,33],lf:[5,38],rk:[25,33],rf:[26,38]},
 {name:'takeoff',head:[17,7],ls:[12,14],rs:[21,14],hip:[16,24],le:[7,18],lh:[4,22],re:[25,8],rh:[27,2],lk:[12,31],lf:[10,38],rk:[24,27],rf:[22,33]},
 {name:'cast',head:[16,10],ls:[12,17],rs:[21,17],hip:[15,27],le:[9,23],lh:[5,20],re:[25,11],rh:[28,4],lk:[7,29],lf:[3,35],rk:[22,32],rf:[23,38]},
 {name:'swing-in',head:[18,12],ls:[13,20],rs:[21,18],hip:[12,28],le:[8,21],lh:[7,15],re:[24,10],rh:[26,2],lk:[6,31],lf:[3,37],rk:[14,34],rf:[19,38]},
 {name:'swing-low',head:[14,11],ls:[10,18],rs:[18,18],hip:[17,28],le:[6,19],lh:[6,13],re:[21,10],rh:[23,2],lk:[13,33],lf:[9,38],rk:[24,32],rf:[29,37]},
 {name:'release',head:[17,11],ls:[12,18],rs:[21,18],hip:[17,27],le:[7,16],lh:[3,11],re:[26,15],rh:[29,10],lk:[9,27],lf:[9,34],rk:[24,26],rf:[25,33]},
 {name:'reach',head:[16,9],ls:[11,16],rs:[20,16],hip:[16,26],le:[6,20],lh:[3,24],re:[25,20],rh:[29,22],lk:[11,31],lf:[8,38],rk:[24,28],rf:[28,34]},
 {name:'land',head:[16,19],ls:[11,26],rs:[20,25],hip:[16,30],le:[7,31],lh:[7,38],re:[25,30],rh:[28,34],lk:[11,33],lf:[8,38],rk:[25,31],rf:[28,38]},
 {name:'rise',head:[17,13],ls:[12,20],rs:[21,20],hip:[16,29],le:[9,26],lh:[9,32],re:[25,25],rh:[27,29],lk:[9,32],lf:[5,38],rk:[24,31],rf:[26,38]},
 {name:'stand',head:[16,6],ls:[11,13],rs:[21,13],hip:[16,24],le:[8,20],lh:[10,26],re:[24,19],rh:[24,25],lk:[12,31],lf:[10,38],rk:[21,31],rf:[23,38]}
];
function paint(p){
 const a=Array(W*H).fill('.');
 const pixel=(x,y,c)=>{x=Math.round(x);y=Math.round(y);if(x>=0&&x<W&&y>=0&&y<H)a[y*W+x]=c;};
 const line=(v,w,c,r=0)=>{const n=Math.max(1,Math.ceil(Math.hypot(w[0]-v[0],w[1]-v[1])*2));for(let i=0;i<=n;i++){const x=v[0]+(w[0]-v[0])*i/n,y=v[1]+(w[1]-v[1])*i/n;for(let dy=-Math.ceil(r);dy<=r;dy++)for(let dx=-Math.ceil(r);dx<=r;dx++)if(dx*dx+dy*dy<=r*r+.1)pixel(x+dx,y+dy,c);}};
 const poly=(ps,c)=>{for(let y=0;y<H;y++)for(let x=0;x<W;x++){let inside=false;for(let i=0,j=ps.length-1;i<ps.length;j=i++)if((ps[i][1]>y)!==(ps[j][1]>y)&&x<(ps[j][0]-ps[i][0])*(y-ps[i][1])/(ps[j][1]-ps[i][1])+ps[i][0])inside=!inside;if(inside)pixel(x,y,c);}};
 const limb=(start,knee,end,front,leg=false)=>{
   line(start,knee,'0',leg?2.7:2.1);line(knee,end,'0',leg?2.2:1.8);
   line(start,knee,front?(leg?'4':'2'):(leg?'6':'1'),leg?1.8:1.3);line(knee,end,front?'2':'1',leg?1.5:1.1);
   const mid=[(knee[0]+end[0])/2,(knee[1]+end[1])/2];line(knee,mid,front?'4':'6',leg?1.1:.6);
   if(front){line([start[0]-1,start[1]], [knee[0]-1,knee[1]],leg?'5':'3');line([mid[0]-1,mid[1]], [end[0]-1,end[1]-1],'3');}
   if(leg)line([end[0]-1,end[1]], [end[0]+2,end[1]],front?'2':'1',.7);else pixel(end[0]+1,end[1]-1,front?'3':'2');
 };
 const [hx,hy]=p.hip;
 limb([hx-2,hy],p.lk,p.lf,false,true);limb(p.ls,p.le,p.lh,false);limb([hx+2,hy],p.rk,p.rf,true,true);
 poly([[p.ls[0]-1,p.ls[1]],[p.rs[0]+1,p.rs[1]],[hx+4,hy],[hx-4,hy]],'0');
 poly([[p.ls[0],p.ls[1]],[p.rs[0],p.rs[1]],[hx+3,hy-1],[hx-3,hy-1]],'2');
 poly([[p.ls[0],p.ls[1]+3],[hx-2,hy-4],[hx-3,hy],[hx-5,hy-2]],'4');
 poly([[p.rs[0],p.rs[1]+3],[hx+2,hy-4],[hx+3,hy],[hx+5,hy-2]],'4');
 line([hx-3,hy-1],[hx+3,hy-1],'1');limb(p.rs,p.re,p.rh,true);
 const cx=Math.round((p.ls[0]+p.rs[0])/2),cy=Math.round((p.ls[1]+p.rs[1])/2)+3;
 const ey=Math.min(cy,hy-4);line([cx,ey],[cx,ey+2],'7');
 line([cx,ey],[cx-2,ey-1],'7');line([cx,ey],[cx+2,ey-1],'7');
 line([cx,ey+1],[cx-2,ey+3],'7');line([cx,ey+1],[cx+2,ey+3],'7');
 const [x,y]=p.head;line([x,y+4],[cx,cy-3],'1',1);
 const mask=['..1221..','.123321.','12322321','10700771','12700721','.122221.','..1221..','...11...'];
 for(let r=0;r<8;r++)for(let c=0;c<8;c++)if(mask[r][c]!=='.')pixel(x-4+c,y-2+r,mask[r][c]);
 return a.join('');
}
export const heroPoses=poses;
export const heroPortrait={id:'peter',width:W,height:H,encoding:4,palette:['#080f24','#a72b43','#e93646','#ff7564','#205391','#4887bb','#18395f','#f3f6f4'],frames:poses.map(paint)};
