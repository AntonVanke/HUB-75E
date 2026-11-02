from pathlib import Path
import re,json,collections
from shapely.geometry import Polygon,LineString,Point,box
from shapely.affinity import translate
from shapely.ops import unary_union,nearest_points
from shapely.strtree import STRtree

def read_sexp(path):
 t=re.findall(r'"[^"\n]*"|\(|\)|[^\s()]+',Path(path).read_text(encoding='utf8'))
 def parse(i):
  a=[];i+=1
  while t[i]!=')':
   if t[i]=='(':v,i=parse(i);a.append(v)
   else:a.append(t[i]);i+=1
  return a,i+1
 return parse(0)[0]
def ch(a,k):return [v for v in a if isinstance(v,list) and v[0]==k]
def shape(s,scale=1):
 k=s[0];v=[float(x)*scale for x in s[2:]]
 if k=='polygon':return Polygon(list(zip(v[1::2],v[2::2]))).buffer(0)
 if k=='path':return LineString(list(zip(v[1::2],v[2::2]))).buffer(v[0]/2)
 if k=='circle':return Point(v[1:] if len(v)>1 else [0,0]).buffer(v[0]/2)
 if k=='rect':return box(*v)
 raise ValueError(s)
r=read_sexp('routing/prepared.dsn');lib=ch(r,'library')[0];im=ch(lib,'image')[0];nets=ch(r,'network')[0];st={s[1]:s for s in ch(lib,'padstack')};netof={p:n[1].strip('"') for n in ch(nets,'net') for p in ch(n,'pins')[0][1:]}
objs=[]
def add(net,layer,g,label,bridge=None,kind=None):objs.append(dict(net=net,layer=layer,g=g,label=label,bridge=bridge,kind=kind))
for p in ch(im,'pin'):
 net=netof.get('u1-'+p[2]);ss=ch(st[p[1]],'shape')
 for sh in ss:
  s=sh[1];g=translate(shape(s),float(p[3]),float(p[4]));add(net,s[1],g,p[2],p[2] if len(ss)>1 else None,'pad')
for w in ch(ch(r,'wiring')[0],'wire'):
 n=ch(w,'net')[0][1].strip('"');s=[v for v in w if isinstance(v,list) and v[0] in ('polygon','path')][0]
 add(n,s[1],shape(s),'fixed',None,'fill' if s[0]=='polygon' else 'track')
rows=json.loads(Path('routing/clean-pcb.json').read_text(encoding='utf8'));orig=json.loads(Path('routing/pcb.json').read_text(encoding='utf8'));origids={a.get('id') for a,b in orig}
for a,b in rows:
 if a['type']=='LINE' and a.get('id') not in origids:
  add(b['netName'],'TopLayer' if b['layerId']==1 else 'BottomLayer',LineString([(b['startX'],b['startY']),(b['endX'],b['endY'])]).buffer(b['width']/2),a['id'],None,'track')
 if a['type']=='VIA':
  for layer in ['TopLayer','BottomLayer']:add(b['netName'],layer,Point(b['centerX'],b['centerY']).buffer(b['viaDiameter']/2),a['id'],a['id'],'via')

def groups(net):
 oo=[o for o in objs if o['net']==net];par=list(range(len(oo)))
 def find(x):
  while x!=par[x]:par[x]=par[par[x]];x=par[x]
  return x
 def join(a,b):par[find(a)]=find(b)
 for layer in ['TopLayer','BottomLayer']:
  ii=[i for i,o in enumerate(oo) if o['layer']==layer];tree=STRtree([oo[i]['g'] for i in ii])
  for i in ii:
   for k in tree.query(oo[i]['g'],predicate='dwithin',distance=.12):join(i,ii[k])
 bridges={}
 for i,o in enumerate(oo):
  if o['bridge']:
   if o['bridge'] in bridges:join(i,bridges[o['bridge']])
   bridges[o['bridge']]=i
 gg=collections.defaultdict(list)
 for i,o in enumerate(oo):gg[find(i)].append(o)
 return sorted(gg.values(),key=len,reverse=True)
if __name__=='__main__':
 for net in ['+3V3','+5V']:
  gs=groups(net);print(net,'groups',list(map(len,gs)))
  for gi,g in enumerate(gs):
   print('GROUP',gi,'pads',[(o['label'],tuple(round(v,2) for v in o['g'].centroid.coords[0])) for o in g if o['kind']=='pad'])
   if gi:
    best=min((a['g'].distance(b['g']),a,b) for a in g for b in gs[0] if a['layer']==b['layer'])
    print('NEAREST',best[0],best[1]['label'],best[2]['label'],[p.coords[0] for p in nearest_points(best[1]['g'],best[2]['g'])])
 bad=[]
 for a in [o for o in objs if o['kind']=='fill']:
  for b in objs:
   if b['kind'] in ('track','via') and b['net']!=a['net'] and b['layer']==a['layer']:
    d=a['g'].distance(b['g'])
    if d<11.9:bad.append((round(d,3),a['net'],b['net'],b['label']))
 print('fill conflicts',bad)
