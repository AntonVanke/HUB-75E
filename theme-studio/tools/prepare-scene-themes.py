"""Pack 64x64 RGB565 scenery and Lua motion into the existing theme library."""
import json, struct, hashlib, urllib.request
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
RES=ROOT/'themes/resources'
def read(p): return json.loads((ROOT/p).read_text(encoding='utf-8'))
def write(p,d): (ROOT/p).write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
manifest=read('themes/resources/manifest.json')
catalog=read('themes/catalog.json')

def clip(name,im,source):
    pixels=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in im.convert('RGB').get_flattened_data()]
    body=bytearray();i=0
    while i<4096:
        j=i+1
        while j<4096 and pixels[j]==pixels[i]:j+=1
        if j-i>=3:body+=struct.pack('<HH',0x4000|j-i,pixels[i]);i=j;continue
        j=i+1
        while j<4096 and not (j+2<4096 and pixels[j]==pixels[j+1]==pixels[j+2]):j+=1
        body+=struct.pack('<H',j-i)+struct.pack('<'+'H'*(j-i),*pixels[i:j]);i=j
    data=struct.pack('<4sHHHHI',b'CWA1',64,64,1,16,1000)+struct.pack('<IIII',0,len(body),1000,1)+body
    (RES/(name+'.cwa')).write_bytes(data)
    entry=dict(id=name,path='/themes/resources/'+name+'.cwa',hashes=[hashlib.sha256(struct.pack('<4096H',*pixels)).hexdigest()],frames=1,duration=1000,bytes=len(data),source=source)
    manifest[:]=[m for m in manifest if m['id']!=name]+[entry]

def register(p):
    write('themes/gallery/'+p['id']+'.json',p)
    entry={k:p[k] for k in ['id','title','category','description','width','height','fidelity']}
    entry['path']='/themes/gallery/'+p['id']+'.json';entry['reference']=p.get('reference','')
    old=next((i for i,t in enumerate(catalog['themes']) if t['id']==p['id']),None)
    if old is None:catalog['themes'].append(entry)
    else:catalog['themes'][old]=entry

def asset(name,im):
    colors=sorted({c[:3] for c in im.get_flattened_data() if len(c)==3 or c[3]})
    assert len(colors)<=255
    pal={c:i for i,c in enumerate(colors)}
    return dict(id=name,width=im.width,height=im.height,encoding=8,palette=['#%02x%02x%02x'%c for c in colors],frames=[''.join('..' if len(c)==4 and not c[3] else '%02x'%pal[c[:3]] for c in im.get_flattened_data())])

# Preserve the original landscape and separate its warm cloud pixels from the sky.
z=Image.open(ROOT/'themes/upstream/reference/26_zelda_sunrise.gif').convert('RGB').resize((64,64),Image.Resampling.NEAREST)
clouds=Image.new('RGBA',(64,15))
for y in range(15):
    for x in range(64):
        r,g,b=z.getpixel((x,y))
        if r>110 and r>b*1.10 and g>100:
            clouds.putpixel((x,y),(r,g,b,255))
            z.putpixel((x,y),(48+y*5,155+y*2,226-y*3))
ImageDraw.Draw(z).rectangle((31,16,35,16),fill=z.getpixel((30,16)))
ImageDraw.Draw(z).rectangle((26,26,63,38),fill=z.getpixel((26,26)))
clip('zelda-sunrise',z,'Original reference landscape; clouds and birds are scripted separately')
p=read('themes/gallery/zelda-sunrise.json')
p.update(assets=[asset('cloud_left',clouds.crop((0,0,32,15))),asset('cloud_right',clouds.crop((32,0,64,15)))],source=(ROOT/'themes/scripts/zelda-sunrise.lua').read_text(encoding='utf-8'),description='原版风景 · 缓慢分层流云 · 独立滑翔与振翅',fidelity='静态风景与云层拆分，Lua 控制真实运动节奏')
register(p)

geo=ROOT/'themes/upstream/ne_110m_land.geojson'
if not geo.exists():
    urllib.request.urlretrieve('https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_110m_land.geojson',geo)
mask=Image.new('1',(512,384));draw=ImageDraw.Draw(mask)
for f in json.loads(geo.read_text(encoding='utf-8'))['features']:
    g=f['geometry'];polys=g['coordinates'] if g['type']=='MultiPolygon' else [g['coordinates']]
    for poly in polys:
        for k,ring in enumerate(poly):draw.polygon([((lon+180)*512/360,(90-lat)*384/180) for lon,lat in ring],fill=1 if k==0 else 0)
mask=mask.resize((64,48),Image.Resampling.NEAREST)
world=Image.new('RGB',(64,64),(4,12,22))
for y in range(48):
    for x in range(64):
        land=mask.getpixel((x,y));polar=abs(y-23.5)>19
        world.putpixel((x,y+2),(184,207,169) if land and polar else (108,177,105) if land else (24,75,111))
clip('world-map',world,'Natural Earth 1:110m land, equirectangular projection, -180..180 / 90..-90')
p=read('themes/gallery/world-map.json')
p.update(source=(ROOT/'themes/scripts/world-map.lua').read_text(encoding='utf-8'),description='UTC 太阳赤纬与均时差 · 真实晨昏线及民用曙暮光',fidelity='Natural Earth 地理数据；NOAA 太阳位置近似，每分钟刷新')
register(p)

# id, title, palette accent, effect, count, bounds, velocity, custom motion
SCENES=[
('gta-vi','GTA · 罪城海岸','#ffb6d4','dot',4,(10,35,25,7),(.25,0),'traffic'),
('minecraft-village','Minecraft · 山谷村庄','#e7d59c','dot',9,(0,18,64,30),(.25,-.45),'smoke'),
('cyberpunk-night-city','赛博朋克 · 夜之城','#5ce7e4','rain',22,(0,0,64,49),(-3,20),'neon'),
('elden-ring','艾尔登法环 · 黄金树','#eaca79','dot',12,(0,5,64,43),(.7,1.2),'gold'),
('hollow-knight-silksong','空洞骑士 · 丝之歌','#8de9df','dot',13,(0,0,64,49),(.25,-.6),'silk'),
('stardew-valley','星露谷 · 秋日农场','#ffcf7f','dot',8,(0,10,64,38),(.65,1.1),'leaves'),
('terraria','泰拉瑞亚 · 空岛','#a8d4f8','dot',8,(28,26,8,23),(0,7),'waterfall'),
('genshin-impact','原神 · 蒙德城','#f2e8a9','dot',6,(0,12,64,34),(.9,-.35),'windmill'),
('final-fantasy-vii','最终幻想 VII · 米德加','#8bf3ba','dot',12,(18,15,40,32),(.2,-1.3),'mako'),
('black-myth-wukong','黑神话悟空 · 山寺','#f0c386','dot',8,(0,12,64,35),(.5,-.45),'mist'),
('monster-hunter','怪物猎人 · 古代森林','#b3d68f','dot',9,(0,15,64,32),(.4,-.6),'wyvern'),
('death-stranding','死亡搁浅 · 寂静山谷','#a4bdc1','rain',12,(0,0,64,49),(-.7,10),'beacon'),
('star-wars','星球大战 · 双日落','#f6bd7b','dot',5,(0,3,64,16),(.15,0),'speeder'),
('blade-runner','银翼杀手 · 雨夜城市','#69cee2','rain',21,(0,0,64,49),(-2,16),'spinner'),
('dune','沙丘 · 厄拉科斯','#efc291','dot',9,(0,29,64,18),(2,.25),'thopter'),
('lord-of-the-rings','指环王 · 瑞文戴尔','#e5cda1','dot',7,(0,12,64,35),(.4,.7),'river'),
('harry-potter','哈利波特 · 霍格沃茨','#e3d9fa','dot',9,(0,0,64,48),(.3,1),'owl'),
('avatar-pandora','阿凡达 · 潘多拉','#7af0df','dot',14,(0,0,64,48),(.2,-.5),'seeds'),
('stranger-things','怪奇物语 · 颠倒世界','#ee8f94','dot',13,(0,0,64,48),(.3,-.65),'storm'),
('wednesday','星期三 · 永封学院','#c8b2f0','dot',6,(0,8,64,40),(.25,-.4),'raven'),
('arcane','双城之战 · 皮城与祖安','#89deca','dot',10,(0,16,64,32),(.3,-.9),'hextech'),
('one-piece','海贼王 · 千阳号','#ffe3aa','dot',8,(0,35,64,13),(.45,.1),'gulls')]

MOTION={
 'traffic': """gfx.wave(14,33,30,12,q,1,10,6)
  for i=0,2 do local x=(t*(2+i*.4)+phase+i*19)%68-4
    gfx.line(x,47-i*2,x+2,47-i*2,i==1 and "#ea648c" or "#ffd4a3") end
  local x=(t*.8+phase)%72-4;gfx.line(x,17,x+2,18,"#695772");gfx.line(x+2,18,x+4,17,"#695772")""",
 'smoke': """gfx.wave(27,35,23,14,q,1,12,4)
  for i=0,3 do local h=(t*1.5+i*3)%12;local x=48+math.sin(h*.5+t*.4)
    gfx.line(x,27-h,x+1,27-h,"#a7aaa2") end
  if math.floor(t*2)%3~=0 then gfx.rect(51,29,2,2,"#e8b86b") end""",
 'neon': """gfx.wave(17,37,32,13,q,1,8,8)
  local n=math.floor(t*3)%17
  if n~=0 and n~=2 then gfx.line(6,18,6,23,"#e4529e");gfx.line(50,13,50,18,"#69d9df") end
  local x=(t*3+phase)%72-4;gfx.line(x,30,x+3,30,"#d46eae");gfx.pixel(x+3,29,"#aee9ed")""",
 'gold': """gfx.wave(19,39,31,11,q,1,12,5)
  for i=0,4 do local y=32-(t*1.6+i*7)%28;local x=26+i*4+math.sin(t*.6+i)*2
    gfx.pixel(x,y,"#ffea9d");if (math.floor(t*3)+i)%4==0 then gfx.pixel(x+1,y,"#fff4cc") end end""",
 'silk': """gfx.wave(27,37,24,13,q,1,10,5)
  for i=0,2 do local x=15+i*16+math.sin(t*.6+i)
    gfx.line(x,0,x,6+i*2,"#6f9baf");gfx.pixel(x,7+i*2,"#bedee7") end
  local x=31+math.sin(t*.9)*6;local y=23+math.sin(t*1.3)*2
  gfx.pixel(x,y,"#a9d5df");gfx.pixel(x-1,y-math.floor(t*4)%2,"#719eb5")""",
 'leaves': """gfx.wave(10,34,14,16,q,1,10,5)
  for i=0,4 do local y=9+(t*2+i*8)%38;local x=9+i*10+math.sin(t*.7+i)*3
    gfx.line(x,y,x+1,y+math.floor(t*2+i)%2,"#d99c51") end""",
 'waterfall': """gfx.wave(31,24,4,26,(t*12)%16,1,8,7)
  gfx.wave(41,32,4,18,(t*10+4)%16,1,8,5)
  for i=0,4 do local y=26+(t*9+i*5)%23;gfx.line(32,y,32,y+2,"#abd3e5") end
  local x=(t*1.3+phase)%72-4;gfx.line(x,13,x+2,14,"#6e6289");gfx.line(x+2,14,x+4,13,"#6e6289")""",
 'windmill': """gfx.wave(14,37,36,13,q,1,12,6)
  local a=t*.7;local c=math.cos(a)*3;local s=math.sin(a)*3
  gfx.line(39-c,31-s,39+c,31+s,"#e2dfc8");gfx.line(39-s,31+c,39+s,31-c,"#e2dfc8")
  local x=(t*1.5+phase)%72-4;local y=13+math.sin(t*.4)
  gfx.line(x,y,x+2,y+1,"#6c9dbb");gfx.line(x+2,y+1,x+4,y,"#6c9dbb")""",
 'mako': """gfx.wave(29,18,6,16,(t*7)%16,0,12,7)
  gfx.wave(46,24,5,15,(t*6+8)%16,0,10,7)
  for i=0,3 do local h=(t*2+i*5)%18;gfx.line(32+math.sin(h*.4),27-h,33+math.sin(h*.4),27-h,"#74b9a5") end
  if math.floor(t*2)%5~=0 then gfx.pixel(43,24,"#aaf1b4");gfx.pixel(39,29,"#7ce3b8") end""",
 'mist': """gfx.wave(37,33,13,16,q,1,14,4)
  for i=0,2 do local x=(t*.7+phase+i*21)%80-12;local y=23+i*5
    gfx.line(x,y,x+5,y,"#8f9c96");gfx.line(x+2,y-1,x+4,y-1,"#9ca79f") end""",
 'wyvern': """gfx.wave(21,36,29,14,q,1,12,5)
  local x=(t*2.2+phase)%78-7;local y=13+math.sin(t*.4)*2;local w=math.floor(t*3)%3-1
  gfx.line(x-2,y,x+2,y,"#456d76");gfx.line(x-4,y-2*w,x,y,"#456d76");gfx.line(x,y,x+4,y-2*w,"#456d76")
  gfx.pixel(x+3,y-1,"#456d76")""",
 'beacon': """gfx.wave(25,37,28,13,q,1,14,4)
  gfx.wave(24,23,3,8,(t*10)%16,0,8,6)
  local b=math.floor(t*3)%9
  if b<2 then gfx.rect(46,31,2,1,"#eacfa0") end
  local x=(t*.45+phase)%80-12;gfx.line(x,20,x+7,20,"#909fa9")""",
 'speeder': """gfx.wave(14,34,43,5,(t*3)%16,1,14,0)
  local x=(t*4+phase)%80-8
  gfx.line(x,43,x+6,43,"#835b53");gfx.line(x+1,42,x+4,42,"#c99877");gfx.pixel(x+4,41,"#574c4b")
  gfx.line(x-3,44,x-1,44,"#c38462")
  for i=0,2 do local x=(t*1.8+i*21+phase)%64;gfx.pixel(x,47+i,"#b27c63") end""",
 'spinner': """gfx.wave(20,39,31,11,q,1,8,7)
  local x=(t*2.8+phase)%78-7
  gfx.line(x,16,x+5,16,"#304962");gfx.line(x+1,15,x+3,15,"#647e92")
  gfx.pixel(x,17,"#79d5d8");gfx.pixel(x+5,17,"#ebaf9b")
  if math.floor(t*2)%7~=0 then gfx.line(52,22,52,28,"#92c9d6") end""",
 'thopter': """gfx.wave(13,32,45,9,(t*3)%16,1,14,0)
  local x=(t*2+phase)%80-8;local y=16+math.sin(t*.5);local w=math.floor(t*6)%2*2-1
  gfx.line(x-2,y,x+2,y,"#725745");gfx.line(x-4,y-w,x+3,y+w,"#725745")
  gfx.line(x-3,y+w,x+3,y-w,"#8d6c50")
  for i=0,3 do local x=(t*5+i*17+phase)%72-4;gfx.line(x,44+i*2,x+2,44+i*2,"#c79a6a") end""",
 'river': """gfx.wave(22,37,29,13,q,1,10,6)
  gfx.wave(50,26,3,17,(t*10)%16,1,8,7)
  for i=0,3 do local y=28+(t*8+i*4)%15;gfx.pixel(51,y,"#c6e0e4") end
  local x=(t*1.4+phase)%72-4;gfx.line(x,15,x+2,16,"#688b9e");gfx.line(x+2,16,x+4,15,"#688b9e")""",
 'owl': """gfx.wave(11,40,43,10,q,1,10,6)
  local x=(t*2+phase)%76-6;local y=12+math.sin(t*.5)*2;local w=math.floor(t*3)%3-1
  gfx.rect(x,y,2,2,"#d0d0dc");gfx.line(x-2,y+w,x,y,"#aaafca");gfx.line(x+1,y,x+3,y+w,"#aaafca")
  if math.floor(t)%4<2 then gfx.pixel(40,28,"#edc895");gfx.pixel(48,30,"#dfbb89") end""",
 'seeds': """gfx.wave(23,37,30,13,q,1,12,7)
  for i=0,4 do local y=46-(t*1.4+i*8)%39;local x=10+i*10+math.sin(t*.6+i)*3
    gfx.pixel(x,y,"#c4efe8");gfx.pixel(x,y+1,"#66bfb8") end
  local a=t*.5;gfx.pixel(14+math.sin(a)*3,35,"#e699d8")""",
 'storm': """gfx.wave(18,41,34,9,q,1,10,5)
  if t>=nextFlash then flashEnd=t+.22;nextFlash=t+rng.int(6,12) end
  if t<flashEnd then
    gfx.line(34,5,30,11,"#ef838c");gfx.line(30,11,33,12,"#ef838c");gfx.line(33,12,28,19,"#ef838c")
  end
  for i=0,2 do local x=(t*.6+phase+i*22)%80-10;gfx.line(x,28+i*2,x+6,28+i*2,"#4c3448") end""",
 'raven': """gfx.wave(34,25,7,9,(t*2)%16,0,16,7)
  for i=0,1 do local x=(t*2+phase+i*28)%76-6;local y=13+i*5+math.sin(t*.5+i);local w=math.floor(t*4+i)%3-1
    gfx.line(x-2,y-w,x,y,"#302b40");gfx.line(x,y,x+2,y-w,"#302b40") end
  local x=(t*.6+phase)%80-12;gfx.line(x,45,x+10,45,"#726d86")""",
 'hextech': """gfx.wave(22,39,30,11,q,1,10,6)
  local a=t*1.4;local c=math.cos(a)*3;local s=math.sin(a)*3
  gfx.pixel(39+c,26+s,"#aff8f2");gfx.pixel(39-c,26-s,"#73d6ea")
  for i=0,2 do local h=(t*1.5+i*5)%14;gfx.pixel(47+math.sin(h*.5),46-h,"#6ab3a8") end""",
 'gulls': """gfx.wave(11,36,44,14,q,1,10,7)
  for i=0,2 do local x=(t*2+phase+i*21)%76-6;local y=10+i*4+math.sin(t*.5+i);local w=math.floor(t*2+i)%3-1
    gfx.line(x-2,y-w,x,y,"#f8e7cc");gfx.line(x,y,x+2,y-w,"#f8e7cc") end
  for i=0,2 do local x=17+(t*1.3+i*12)%34;gfx.line(x,40+i*3,x+2,40+i*3,"#83cfdb") end"""
}

for id,title,accent,effect,count,bounds,velocity,motion in SCENES:
    art=ROOT/'themes/artwork'/f'{id}.png'
    if not art.exists():continue
    im=Image.open(art).convert('RGB').resize((64,64),Image.Resampling.BOX).quantize(colors=64,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE).convert('RGB')
    im.save(ROOT/'themes/artwork'/f'{id}-64.png')
    clip(id,im,'Generated original scenery: themes/artwork/prompts.json')
    source='''local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",COUNT,BOUNDS,VELOCITY,"ACCENT","EFFECT")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("ID",0,0,0)
  local q=(t*5+phase)%16
  MOTION
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"ACCENT",2)
end
'''.replace('COUNT',str(count)).replace('BOUNDS',','.join(map(str,bounds))).replace('VELOCITY',','.join(map(str,velocity))).replace('ACCENT',accent).replace('EFFECT',effect).replace('MOTION',MOTION[motion]).replace('ID',id)
    (ROOT/'themes/scripts'/f'{id}.lua').write_text(source,encoding='utf-8')
    p=dict(format='clockwise-theme-project',version=3,id=id,title=title,width=64,height=64,fps=60,reference='',category='游戏风景' if SCENES.index((id,title,accent,effect,count,bounds,velocity,motion))<12 else '影视风景',description=title.split(' · ')[1]+' · 局部水波、场景动效与随机相位',sourceMode='lua',libraries=['core'],assets=[],media=[id],data={},source=source,fidelity='64×64 像素风景，Lua 场景运动与原生 RGB565 局部特效')
    register(p)
for id in ['pacman','girl','pikachu']:register(read('themes/gallery/'+id+'.json'))
write('themes/resources/manifest.json',manifest);write('themes/catalog.json',catalog)
print('Theme count:',len(catalog['themes']),'single-frame resources:',sum(m['frames']==1 for m in manifest))
