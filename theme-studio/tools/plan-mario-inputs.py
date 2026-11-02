import sys, pathlib, json, struct, time, subprocess, hashlib
root=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'.build/python-lua'))
from lupa.lua53 import LuaRuntime
if len(sys.argv)>1 and sys.argv[1]=='--all':
 subprocess.run(['node','tools/compile-mario-worlds.mjs','--geometry'],cwd=root,check=True)
 geometry=(root/'themes/resources/mario-worlds.cwr').read_bytes();areas=[]
 for l in range(1,33):
  off=struct.unpack_from('<I',geometry,8+(l-1)*4)[0]
  for a in range(1,geometry[off]+1):
   subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),str(l),str(a),'30000','1.5'],cwd=root,check=True)
   pos,n=struct.unpack_from('<IH',geometry,off+1+(a-1)*6);raw=geometry[off+pos:off+pos+n]
   rle,points=struct.unpack_from('<HH',raw,10);end=20+rle+points*6+raw[14]*6+raw[15]*6+raw[18]*10
   p=json.loads((root/('.build/mario-inputs/%02d-%02d.json'%(l,a))).read_text())
   areas.append(dict(level=l,area=a,geometrySha256=hashlib.sha256(raw[:end]).hexdigest(),frames=p['frames'],segments=p['segments']))
 (root/'themes/data/mario-autoplay.json').write_text(json.dumps(dict(version=1,engine='lua-64hz-inputs-v2',areas=areas),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 subprocess.run(['node','tools/compile-mario-worlds.mjs'],cwd=root,check=True)
 sys.exit(0)
level=int(sys.argv[1]) if len(sys.argv)>1 else 3
area=int(sys.argv[2]) if len(sys.argv)>2 else 1
limit=int(sys.argv[3]) if len(sys.argv)>3 else 150000
weight=float(sys.argv[4]) if len(sys.argv)>4 else 1.5
bank=(root/'themes/resources/mario-worlds.cwr').read_bytes()
lua=LuaRuntime(encoding=None,unpack_returned_tuples=True)
lua.globals().bank_read=lambda offset,count:bank[int(offset):int(offset+count)]
lua.globals().planner_progress=lambda *a:print('progress',level,area,*a,flush=True)
lua.execute(b'''resource={read=function(_,o,n)return bank_read(o,n)end,rle4=function(s,packed)local t={}local hex="0123456789abcdef"for i=1,#s do local b=s:byte(i);t[i]=string.rep(hex:sub(b%16+1,b%16+1),b//16+1)end local raw=table.concat(t);if not packed then return raw end;t={};for i=1,#raw,2 do local a=tonumber(raw:sub(i,i),16);local b=tonumber(raw:sub(i+1,i+1),16)or 0;t[#t+1]=string.char(a+b*16)end;return table.concat(t)end}
memory={collect=function()collectgarbage()end};clock={format=function()return "12:00"end};telemetry={set=function()end};gfx=setmetatable({},{__index=function()return function()end end});sprite=gfx
''')
source=(root/'themes/scripts/mario-1-1.lua').read_text(encoding='utf-8-sig')
source=source.replace('spawn();actors();control()','spawn();actors();planner_control()')
search=(root/'tools/mario-autoplay-search.lua').read_text(encoding='utf-8-sig')
source=source.replace('game.physics=physics',search+'\nfunction planner_state() return state end\ngame.physics=physics')
lua.execute(source.encode())
lua.execute(('start();game.load(%d,%d);while planner_state()~="run" do game.physics() end'%(level,area)).encode())
begin=time.time()
prefix=[]
if level==32 and area==1:
 values=lua.globals().planner_search(limit,weight,None,570,112)
 if values[0] is not None:
  prefix=[[int(values[0][i][1]),int(values[0][i][2])] for i in range(1,len(values[0])+1)]
  waits=lua.execute(b'local n=0;game.command=5;while not (game.platforms[1].x<=708 and game.platforms[1].dir<0) do game.physics();n=n+1;assert(planner_state()=="run" and n<1500,"platform approach failed") end;return n')
  if waits: prefix.append([5,waits])
  values=lua.globals().planner_search(limit,weight,1)
  if values[0] is None: raise RuntimeError('Could not board platform')
  prefix += [[int(values[0][i][1]),int(values[0][i][2])] for i in range(1,len(values[0])+1)]
  waits=lua.execute(b'local n=0;game.command=5;while game.platforms[1].x<780 do game.physics();n=n+1;assert(planner_state()=="run" and n<1200,"platform ride failed") end;return n')
  if waits: prefix.append([5,waits])
  values=lua.globals().planner_search(limit,weight)
else: values=lua.globals().planner_search(limit,weight)
path,expanded,steps,x,queued=values[:5]
if path is None:
 if len(values)>5:
  partial=[[int(values[5][i][1]),int(values[5][i][2])] for i in range(1,len(values[5])+1)]
  (root/('.build/mario-partial-%02d-%02d.json'%(level,area))).write_text(json.dumps(partial))
 print(json.dumps(dict(level=level,area=area,success=False,expanded=expanded,steps=steps,max_x=x,queued=queued,elapsed=time.time()-begin)),flush=True)
 sys.exit(2)
segments=[]
for c,n in prefix+[[int(path[i][1]),int(path[i][2])] for i in range(1,len(path)+1)]:
 if segments and segments[-1][0]==c:segments[-1][1]+=n
 else:segments.append([c,n])
result=dict(level=level,area=area,success=True,expanded=expanded,steps=steps,max_x=x,queued=queued,elapsed=time.time()-begin,frames=sum(s[1] for s in segments),segments=segments)
out=root/'.build/mario-inputs';out.mkdir(exist_ok=True)
(out/('%02d-%02d.json'%(level,area))).write_text(json.dumps(result),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='segments'}),flush=True)
