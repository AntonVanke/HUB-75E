export class GridSolver {
  constructor(){this.distance=new Uint32Array(256);this.first=new Uint16Array(256);this.parent=new Uint16Array(256);this.used=new Uint8Array(256);this.costs=new Uint32Array(256);this.queue=new Uint16Array(256);}
  solve(w,h,map,start,weighted=false){
    const n=w*h;
    if(!Number.isInteger(w)||!Number.isInteger(h)||w<1||h<1||n>256||map.length!==n||!Number.isInteger(start)||start<1||start>n||map[start-1]==='#')throw new Error('网格最多 256 格，起点必须可通行');
    const d=this.distance,first=this.first,parent=this.parent,used=this.used,q=this.queue;
    d.fill(0xffffffff);first.fill(0);parent.fill(0);used.fill(0);d[--start]=0;
    let head=0,tail=1;q[0]=start;
    for(let step=0;step<n;step++){
      let current=-1,best=0xffffffff;
      if(weighted){for(let i=0;i<n;i++)if(!used[i]&&d[i]<best){current=i;best=d[i];}}
      else {if(head<tail){current=q[head++];best=d[current];}}
      if(current<0)break;used[current]=1;
      for(let k=0;k<4;k++){
        const next=k===0?current+1:k===1?current+w:k===2?current-1:current-w;
        if(next<0||next>=n||(k===0&&current%w===w-1)||(k===2&&current%w===0)||map[next]==='#')continue;
        const cost=best+(weighted?this.costs[next]:1);
        if(cost<d[next]){const fresh=d[next]===0xffffffff;d[next]=cost;first[next]=current===start?next+1:first[current];parent[next]=current+1;if(!weighted&&fresh)q[tail++]=next;}
      }
    }
    return {distance:d,first,parent,n};
  }
}
