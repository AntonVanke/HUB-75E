from geometry import *
import numpy as np,shapely,heapq,math,uuid
from scipy.ndimage import distance_transform_edt
step=2.;xs=np.arange(16.,2346.,step);ys=np.arange(16.,1930.,step);xx,yy=np.meshgrid(xs,ys);H,W=xx.shape;layers=['TopLayer','BottomLayer'];rows=json.loads(Path('routing/clean-pcb.json').read_text(encoding='utf8'))
ticket=max(a.get('ticket',0) for a,b in rows)+1

def append(t,b):
 global ticket
 a={'type':t,'ticket':ticket,'id':uuid.uuid4().hex[:16]};rows.append([a,b]);ticket+=1;return a['id']
def route(net,width,startgi):
 gs=groups(net)
 if len(gs)==1:print(net,'already connected');return
 src=gs[startgi];dst=[o for i,g in enumerate(gs) if i!=startgi for o in g]
 free=[];vf=[];sm=[];tm=[];obslist=[]
 for layer in layers:
  others=[o for o in objs if o['layer']==layer and o['net']!=net]
  obs=unary_union([o['g'].buffer(width/2+(12.2 if o['kind']=='fill' else 7.2),quad_segs=4) for o in others])
  viaobs=unary_union([o['g'].buffer(12.1+(12.2 if o['kind']=='fill' else 7.2),quad_segs=4) for o in others]+[o['g'].buffer(12.5,quad_segs=4) for o in objs if o['layer']==layer and o['kind']=='pad'])
  obslist.append(obs);free.append(~shapely.contains_xy(obs,xx,yy));vf.append(~shapely.contains_xy(viaobs,xx,yy))
  sm.append(shapely.contains_xy(unary_union([o['g'] for o in src if o['layer']==layer]),xx,yy)&free[-1])
  tm.append(shapely.contains_xy(unary_union([o['g'] for o in dst if o['layer']==layer]),xx,yy)&free[-1])
 free=np.asarray(free);sm=np.asarray(sm);tm=np.asarray(tm);viafree=vf[0]&vf[1]
 heuristic=distance_transform_edt(~(tm[0]|tm[1]))
 size=2*H*W;dist=np.full(size,np.inf);prev=np.full(size,-1,dtype=np.int32);q=[]
 def idx(l,y,x):return (l*H+y)*W+x
 for l,y,x in np.argwhere(sm):
  k=idx(l,y,x);dist[k]=0;heapq.heappush(q,(float(heuristic[y,x]),0,k))
 dirs=[(-1,0,1),(1,0,1),(0,-1,1),(0,1,1),(-1,-1,2**.5),(-1,1,2**.5),(1,-1,2**.5),(1,1,2**.5)]
 count=0;end=None
 while q:
  f,d,k=heapq.heappop(q)
  if d!=dist[k]:continue
  x=k%W;y=(k//W)%H;l=k//(W*H);count+=1
  if tm[l,y,x]:end=k;break
  for dy,dx,c in dirs:
   ny,nx=y+dy,x+dx
   if 0<=ny<H and 0<=nx<W and free[l,ny,nx] and free[l,y,nx] and free[l,ny,x]:
    nk=idx(l,ny,nx);nd=d+c
    if nd<dist[nk]:dist[nk]=nd;prev[nk]=k;heapq.heappush(q,(nd+float(heuristic[ny,nx]),nd,nk))
  if viafree[y,x] and free[1-l,y,x]:
   nk=idx(1-l,y,x);nd=d+35
   if nd<dist[nk]:dist[nk]=nd;prev[nk]=k;heapq.heappush(q,(nd+float(heuristic[y,x]),nd,nk))
 if end is None:raise RuntimeError((net,'no path',count))
 path=[];k=end
 while k>=0:path.append((int(k//(W*H)),float(xs[k%W]),float(ys[(k//W)%H])));k=int(prev[k])
 path.reverse();print(net,'visited',count,'points',len(path),'start/end',path[0],path[-1])
 simple=[path[0]];i=0
 while i<len(path)-1:
  if path[i+1][0]!=path[i][0]:simple.append(path[i+1]);i+=1;continue
  last=i+1
  while last+1<len(path) and path[last+1][0]==path[i][0]:last+=1
  chosen=None
  for j in range(last,i,-1):
   a,b=path[i],path[j];dx=b[1]-a[1];dy=b[2]-a[2];m=min(abs(dx),abs(dy));sx=1 if dx>=0 else -1;sy=1 if dy>=0 else -1
   bends=[(a[1]+sx*m,a[2]+sy*m),(b[1]-sx*m,b[2]-sy*m),(a[1],b[2]),(b[1],a[2])]
   for bend in bends:
    pts=[a[1:],bend,b[1:]];g=LineString(pts)
    if not g.intersects(obslist[a[0]]):chosen=(j,[a[:1]+bend,b]);break
   if chosen:break
  if not chosen:chosen=(i+1,[path[i+1]])
  i,more=chosen
  for v in more:
   if v!=simple[-1]:simple.append(v)
 print('corners',simple)
 for a,b in zip(simple,simple[1:]):
  if a[0]!=b[0]:
   id=append('VIA',dict(partitionId='',groupId=0,netName=net,ruleName='',centerX=a[1],centerY=a[2],holeDiameter=12,viaDiameter=24,viaType='NORMAL',topSolderExpansion=None,bottomSolderExpansion=None,locked=False,unusedInnerLayers=[],propagationDelay=0))
   for layer in layers:add(net,layer,Point(a[1:]).buffer(12),id,id,'via')
  elif a[1:]!=b[1:]:
   id=append('LINE',dict(partitionId='',groupId=0,netName=net,layerId=a[0]+1,startX=a[1],startY=a[2],endX=b[1],endY=b[2],width=width,locked=False,zIndex=-1));add(net,layers[a[0]],LineString([a[1:],b[1:]]).buffer(width/2),id,None,'track')
 print('remaining groups',list(map(len,groups(net))))
for net,width in [('+3V3',12),('+5V',20),('$CARD1_7',8),('$U1_13',6),('$U1_14',6)]:
 while len(groups(net))>1:
  route(net,width,len(groups(net))-1)
  Path('routing/clean-pcb.json').write_text(json.dumps(rows,ensure_ascii=False),encoding='utf8')
Path('routing/clean-pcb.json').write_text(json.dumps(rows,ensure_ascii=False),encoding='utf8')
