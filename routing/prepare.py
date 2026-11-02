from pathlib import Path
import re,json
s=Path('enet/Autorouter_PCB1_2026-10-27.dsn').read_text()
tokens=re.findall(r'"[^"\n]*"|\(|\)|[^\s()]+',s)
def parse(i):
 out=[];i+=1
 while tokens[i]!=')':
  if tokens[i]=='(': v,i=parse(i);out.append(v)
  else:out.append(tokens[i]);i+=1
 return out,i+1
root,_=parse(0)
def children(n,t):return [x for x in n if isinstance(x,list) and x[0]==t]
lib=children(root,'library')[0];image=children(lib,'image')[0];network=children(root,'network')[0];wiring=children(root,'wiring')[0]
# EasyEDA writes vias twice, as vias and as pins. Fills become concave pads,
# which Specctra padstacks require to be convex; represent them as copper areas.
stacks={x[1]:x for x in children(lib,'padstack')}
fills={'a6b6b2531233704','a7cd0987044ebd9','28c8b770c1a7f88b','3898791392598da','55f0887e9e7b1e5','d463b9b2c10b94d','f4a17f9f5c1bf04','5a043b9c8dc19e4','bb8d51416f07ccfb'}
netof={p[3:]:n[1] for n in children(network,'net') for p in children(n,'pins')[0][1:]}
remove=set()
for p in children(image,'pin'):
 if p[1]=='pd5d91fff309254ba':remove.add(p[2])
 if p[2] in fills:
  remove.add(p[2]);shape=children(stacks[p[1]],'shape')[0][1]
  if shape[0]!='polygon':raise ValueError(shape)
  xy=[float(v) for v in shape[3:]]; pts=[]
  for x,y in zip(xy[::2],xy[1::2]):
   v=(round(x+float(p[3]),4),round(y+float(p[4]),4))
   if not pts or pts[-1]!=v:pts.append(v)
  if pts[0]==pts[-1]:pts.pop()
  wiring.append(['wire',['polygon',shape[1],'0']+[str(v) for pt in pts for v in pt],['net',netof[p[2]]],['type','protect'],['clearance_class','fixedfill']])
image[:]=[x for x in image if not (isinstance(x,list) and x[0]=='pin' and x[2] in remove)]
for n in children(network,'net'):
 p=children(n,'pins')[0];p[:]=[p[0]]+[x for x in p[1:] if x[3:] not in remove]
for c in children(network,'class'):
 c[2]=c[1]
 for r in children(c,'rule'):
  for cl in children(r,'clearance'):cl[1]='7'
  for w in children(r,'width'):w[1]=str({'+5V':20,'+3V3':12,'$D2_1':40}.get(c[1],8))
struct=children(root,'structure')[0]
for r in children(struct,'rule'):
 for cl in children(r,'clear'):cl[1]='7'
struct.append(['rule',['clearance','12.1',['type','default_fixedfill']]])
struct.append(['rule',['clearance','12.1',['type','area_wire']],['clearance','12.1',['type','area_via']],['clearance','12.1',['type','area_pin']]])
struct.append(['keepout','antenna_top',['rect','TopLayer','1140','1940','1850','1968.5']])
struct.append(['keepout','antenna_bottom',['rect','BottomLayer','1140','1940','1850','1968.5']])
def dump(n,lev=0):
 if not isinstance(n,list):return n
 if all(not isinstance(x,list) for x in n):return '('+' '.join(n)+')'
 return '('+' '.join(dump(x,lev+1) if not isinstance(x,list) else '\n'+'  '*(lev+1)+dump(x,lev+1) for x in n)+')'
Path('routing/prepared.dsn').write_text(dump(root),encoding='utf-8')
print('Removed synthetic pins:',len(remove),'nets',len(children(network,'net')),'wires',len(children(wiring,'wire')))
