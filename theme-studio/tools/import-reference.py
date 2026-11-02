"""Build RGB565 delta clips from the author's public 64x64 gallery captures.
Recorded clock text is removed; Lua draws live time. No frame interpolation.
"""
import json, re, struct, hashlib
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
RES = ROOT / 'themes/resources'
RES.mkdir(exist_ok=True)
CAT = json.loads((ROOT/'themes/catalog.json').read_text(encoding='utf-8'))
# x,y,w,h, background sample coordinate (or explicit RGB). Only recorded text.
MASKS = {
 'pacman':[(11,21,42,13,'#000000'),(18,35,31,9,'#000000')],
 'world-map':[(0,51,64,13,(0,63))],
 'time-in-words':[(0,0,64,64,'#000000')],
 'clock-tower':[(28,22,10,26,(32,15))],
 'pokedex':[(39,16,25,15,(40,17))],
 'retro-computer':[(20,20,28,12,(20,19))],
 'snoopy':[(34,0,30,9,(34,9))],
 'nyan-cat':[(0,44,64,20,(0,63))],
 'transformer':[(48,1,16,28,(47,3))],
 'minecraft-torch':[(32,55,32,9,(34,54))],
 'coffee':[(3,4,58,14,'#111111')],
 'pepsi':[(0,43,64,17,'#000000')],
 'pikachu':[(12,7,40,14,(0,0))],
 'shar-pei-dog':[(6,1,52,17,(0,0))],
 'girl':[(38,0,26,54,(63,63))],
 'kirby':[(10,0,45,16,(0,0)),(0,55,38,9,(0,0))],
 'labubu-zimomo':[(1,1,37,13,(0,0))],
 'hello-kitty':[(9,47,49,17,'#000000')],
 'twinkle-twinkle':[(13,0,49,15,'#000000')],
 'zootopia':[(4,50,25,9,(5,50)),(36,50,25,9,(37,50))],
 'minecraft-village':[(14,52,36,12,(14,52))],
 'codex':[(0,45,64,18,'#000000')],
 'rainy-window':[(12,51,42,10,(10,60))],
 'gta-vi':[(9,51,46,13,(0,63))],
 'zelda-sunrise':[(26,26,38,13,(26,26))],
 'particle-time':[(4,18,58,25,'#000000'),(2,50,60,12,'#000000')]
}
IDS=['super-mario','pacman','world-map','time-in-words','clock-tower','pokedex','retro-computer','snoopy','nyan-cat','transformer','minecraft-torch','coffee','pepsi','pikachu','shar-pei-dog','girl','kirby','labubu-zimomo','hello-kitty','twinkle-twinkle','zootopia','minecraft-village','codex','rainy-window','gta-vi','zelda-sunrise','particle-time']
def rgb565(im):
 return [((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in im.convert('RGB').getdata()]
def encode(p, prev):
 out=bytearray();i=0
 while i<len(p):
  if prev is not None and p[i]==prev[i]:
   j=i+1
   while j<len(p) and p[j]==prev[j] and j-i<16383:j+=1
   out+=struct.pack('<H',0x8000|j-i);i=j;continue
  j=i+1
  while j<len(p) and p[j]==p[i] and j-i<16383:j+=1
  if j-i>=3:
   out+=struct.pack('<HH',0x4000|j-i,p[i]);i=j;continue
  j=i+1
  while j<len(p) and j-i<16383:
   if prev is not None and p[j]==prev[j]:break
   if j+2<len(p) and p[j]==p[j+1]==p[j+2]:break
   j+=1
  out+=struct.pack('<H',j-i)+struct.pack('<'+'H'*(j-i),*p[i:j]);i=j
 return out
def clip(name,frames,durations):
 records=[];body=bytearray();end=0;previous=None
 for i,im in enumerate(frames):
  pixels=rgb565(im);key=i%16==0
  data=encode(pixels,None if key else previous);end+=max(10,durations[i])
  records.append((len(body),len(data),end,1 if key else 0));body+=data;previous=pixels
 header=struct.pack('<4sHHHHI',b'CWA1',64,64,len(frames),16,end)
 table=b''.join(struct.pack('<IIII',*r) for r in records)
 path=RES/(name+'.cwa');path.write_bytes(header+table+body)
 return {'id':name,'path':'/themes/resources/'+path.name,'hashes':[hashlib.sha256(struct.pack('<4096H',*rgb565(f))).hexdigest() for f in frames],'frames':len(frames),'duration':end,'bytes':path.stat().st_size,'source':'https://github.com/yuan910715/clock_press/tree/main/docs/img/clockfaces'}
manifest=[]
for n,name in enumerate(IDS):
 if name in ['super-mario','smart-pacman']:continue
 path=next((ROOT/'themes/upstream/reference').glob(f'{n+1:02d}_*.gif'))
 gif=Image.open(path);frames=[];durations=[]
 for i in range(gif.n_frames):
  gif.seek(i);im=gif.convert('RGB').resize((64,64),Image.Resampling.NEAREST);original=im.copy();draw=ImageDraw.Draw(im)
  for x,y,w,h,bg in MASKS.get(name,[]):
   color=original.getpixel(bg) if isinstance(bg,tuple) else bg
   draw.rectangle((x,y,x+w-1,y+h-1),fill=color)
  frames.append(im);durations.append(gif.info.get('duration',100))
 manifest.append(clip(name,frames,durations))
(RES/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')

# Convert original C++ Mario RGB565 arrays and GFX font without redrawing them.
text=(ROOT/'themes/upstream/mario/gfx/assets.h').read_text()
text=re.sub(r'//[^\n]*','',text)
sizes={name:tuple(map(int,re.findall(r'\d+',body))) for name,body in re.findall(r'const byte (\w+)_SIZE\[2\]\s*=\s*\{([^}]+)',text)}
sizes.update(BLOCK=(19,19),BUSH=(21,9),CLOUD1=(13,12),CLOUD2=(13,12),HILL=(20,22),GROUND=(8,8))
assets=[]
constants=dict(re.findall(r'const unsigned short (\w+) = (0x[0-9a-fA-F]+);',text))
constants['_MASK']='0x000E'
for key,value in constants.items():text=re.sub(r'\b'+key+r'\b',value,text)
for name,size,body in re.findall(r'const (?:unsigned short|uint16_t) (\w+)\s*\[\s*(\d*)\s*\]\s*(?:PROGMEM\s*)?=\s*\{([^}]+)',text):
 pixels=[int(x,16) for x in re.findall(r'0x[\da-fA-F]+',body)]
 pixels += [0]*((int(size) if size else len(pixels))-len(pixels))
 if name not in sizes:continue
 colors=list(dict.fromkeys(pixels))
 if len(colors)>16:raise ValueError(name)
 palette=['#%02x%02x%02x'%(((c>>11)*255//31),(((c>>5)&63)*255//63),((c&31)*255//31)) for c in colors]
 # Expand bits by rounding upward to preserve all original RGB565 components.
 palette=['#%02x%02x%02x'%(((c>>11)*255+30)//31,(((c>>5)&63)*255+62)//63,((c&31)*255+30)//31) for c in colors]
 w,h=sizes[name]
 assets.append({'id':name.lower(),'width':w,'height':h,'palette':palette,'frames':[''.join(format(colors.index(c),'x') for c in pixels)]})
(RES/'mario-sprites.json').write_text(json.dumps(assets),encoding='utf-8')
text=(ROOT/'themes/upstream/mario/gfx/Super_Mario_Bros__24pt7b.h').read_text()
bitmap=[int(x,16) for x in re.findall(r'0x[\da-fA-F]+',text.split('Bitmaps[]')[1].split('};')[0])]
glyphs={}
for i,row in enumerate(re.findall(r'\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(-?\d+),\s*(-?\d+)\s*\}',text)):
 offset,w,h,advance,x,y=map(int,row);rows=[]
 for yy in range(h):
  bits=0
  for xx in range(w):
   p=yy*w+xx;bits=bits*2+((bitmap[offset+p//8]>>(7-p%8))&1)
  rows.append(bits)
 glyphs[chr(i+32)]={'w':w,'h':h,'advance':advance,'x':x,'y':y+6,'rows':rows}
fonts={'mario':{'height':7,'glyphs':glyphs}}
# Compact seven-segment clock font, drawn in native batches.
segments={'0':'abcdef','1':'bc','2':'abdeg','3':'abcdg','4':'bcfg','5':'acdfg','6':'acdefg','7':'abc','8':'abcdefg','9':'abcdfg'}
g={}
for ch,s in segments.items():
 rows=[[0]*7 for _ in range(11)]
 for seg in s:
  x,y,w,h={'a':(0,0,7,2),'b':(5,0,2,6),'c':(5,5,2,6),'d':(0,9,7,2),'e':(0,5,2,6),'f':(0,0,2,6),'g':(0,4,7,2)}[seg]
  for yy in range(y,y+h):
   for xx in range(x,x+w):rows[yy][xx]=1
 g[ch]={'w':7,'h':11,'advance':8,'x':0,'y':0,'rows':[int(''.join(map(str,r)),2) for r in rows]}
g[':']={'w':2,'h':11,'advance':3,'x':0,'y':0,'rows':[0,0,0,3,0,0,0,3,0,0,0]}
g[' ']={'w':0,'h':0,'advance':3,'x':0,'y':0,'rows':[]}
fonts['segment']={'height':11,'glyphs':g}
for name,file in [('sans','FreeSans9pt7b.h'),('bold','FreeSansBold9pt7b.h'),('italic','FreeSansBoldOblique9pt7b.h')]:
 text=(ROOT/'themes/upstream/fonts'/file).read_text()
 data=[int(x,16) for x in re.findall(r'0x[0-9a-fA-F]+',text.split('Bitmaps[]')[1].split('};')[0])]
 chars={}
 for i,row in enumerate(re.findall(r'\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(-?\d+),\s*(-?\d+)\s*\}',text)):
  ch=chr(i+32)
  if ch not in '0123456789: -':continue
  offset,w,h,advance,x,y=map(int,row);rows=[]
  for yy in range(h):
   bits=0
   for xx in range(w):
    k=yy*w+xx;bits=bits*2+((data[offset+k//8]>>(7-k%8))&1)
   rows.append(bits)
  chars[ch]={'w':w,'h':h,'advance':advance,'x':x,'y':y+12,'rows':rows}
 fonts[name]={'height':13,'glyphs':chars}
# Public pixel font reference; preserve the captured glyphs for 0/1/3/7/colon.
im=Image.open(ROOT/'themes/upstream/reference/12_coffee.gif').convert('RGB').resize((64,64),Image.Resampling.NEAREST)
pixel={}
for ch,glyph in fonts['mario']['glyphs'].items():
 if ch not in '0123456789: ':continue
 rows=[glyph['rows'][min(glyph['h']-1,y*glyph['h']//11)] if glyph['h'] else 0 for y in range(11)]
 pixel[ch]={'w':glyph['w'],'h':11,'advance':8 if ch!=':' else 3,'x':0,'y':0,'rows':rows}
for ch,x,w in [('1',6,7),('7',14,7),(':',22,2),('3',25,7),('0',33,7)]:
 rows=[]
 for y in range(5,16):
  bits=0
  for xx in range(x,x+w):
   rgb=im.getpixel((xx,y));bits=bits*2+int(rgb[0]>100 and rgb[1]>100)
  rows.append(bits)
 pixel[ch]={'w':w,'h':11,'advance':w+1,'x':0,'y':0,'rows':rows}
fonts['pixel']={'height':11,'glyphs':pixel}
# Thin segmented font for the 2x particle clock.
g={}
for ch,segments_on in segments.items():
 rows=[[0]*5 for _ in range(11)]
 for seg in segments_on:
  x,y,w,h={'a':(1,0,3,1),'b':(4,1,1,4),'c':(4,6,1,4),'d':(1,10,3,1),'e':(0,6,1,4),'f':(0,1,1,4),'g':(1,5,3,1)}[seg]
  for yy in range(y,y+h):
   for xx in range(x,x+w):rows[yy][xx]=1
 g[ch]={'w':5,'h':11,'advance':6,'x':0,'y':0,'rows':[int(''.join(map(str,r)),2) for r in rows]}
g[' ']={'w':0,'h':0,'advance':3,'x':0,'y':0,'rows':[]}
fonts['thin']={'height':11,'glyphs':g}
(ROOT/'studio/fonts.mjs').write_text('export const FONTS = '+json.dumps(fonts)+';\n',encoding='utf-8')
print(json.dumps({'clips':len(manifest),'clip_bytes':sum(x['bytes'] for x in manifest),'largest':[{'id':x['id'],'bytes':x['bytes']} for x in sorted(manifest,key=lambda x:x['bytes'],reverse=True)[:3]],'mario_sprites':len(assets)}))
