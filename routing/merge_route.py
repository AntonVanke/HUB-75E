from pathlib import Path
import re,json,uuid,collections
s=Path('routing/routed-v4.ses').read_text();ts=re.findall(r'"[^"\n]*"|\(|\)|[^\s()]+',s)
def parse(i):
 a=[];i+=1
 while ts[i]!=')':
  if ts[i]=='(':v,i=parse(i);a.append(v)
  else:a.append(ts[i]);i+=1
 return a,i+1
def ch(a,k):return [v for v in a if isinstance(v,list) and v[0]==k]
r,_=parse(0);nets=ch(ch(r,'routes')[0],'network_out')[0]
rows=json.loads(Path('routing/pcb.json').read_text(encoding='utf8'));ticket=max(a.get('ticket',0) for a,b in rows)+1
count=collections.Counter()
def add(t,b):
 global ticket
 rows.append([{'type':t,'ticket':ticket,'id':uuid.uuid4().hex[:16]},b]);ticket+=1;count[t]+=1
for n in ch(nets,'net'):
 net=n[1].strip('"')
 for w in ch(n,'wire'):
  if ch(w,'type') or not ch(w,'path'):continue
  p=ch(w,'path')[0];nums=[float(v)/1000 for v in p[2:]];width=nums[0];pts=list(zip(nums[1::2],nums[2::2]))
  for (x,y),(xx,yy) in zip(pts,pts[1:]):
   if (x,y)==(xx,yy):continue
   add('LINE',dict(partitionId='',groupId=0,netName=net,layerId=1 if p[1]=='TopLayer' else 2,startX=x,startY=y,endX=xx,endY=yy,width=width,locked=False,zIndex=-1))
 for v in ch(n,'via'):
  if ch(v,'type'):continue
  x,y=float(v[2])/1000,float(v[3])/1000
  if any(a['type']=='VIA' and abs(b['centerX']-x)<.05 and abs(b['centerY']-y)<.05 for a,b in rows):continue
  add('VIA',dict(partitionId='',groupId=0,netName=net,ruleName='',centerX=x,centerY=y,holeDiameter=12,viaDiameter=24,viaType='NORMAL',topSolderExpansion=None,bottomSolderExpansion=None,locked=False,unusedInnerLayers=[],propagationDelay=0))
source='\n'.join(json.dumps(a,separators=(',',':'),ensure_ascii=False)+'||'+json.dumps(b,separators=(',',':'),ensure_ascii=False)+'|' for a,b in rows)
Path('routing/clean-pcb-source.txt').write_text(source,encoding='utf8')
Path('routing/clean-pcb.json').write_text(json.dumps(rows,ensure_ascii=False),encoding='utf8')
orig=Path('routing/original.epru').read_text(encoding='utf8');start=orig.rfind('{"type":"DOCHEAD"')
Path('routing/clean-all-source.txt').write_text(orig[:start]+source,encoding='utf8')
print(count,'rows',len(rows),'pcb chars',len(source),'all chars',len(orig[:start]+source))
