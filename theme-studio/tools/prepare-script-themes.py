"""Extract lossless sprite components for public Lua-driven themes."""
import json, hashlib, struct
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
def read(p): return json.loads((ROOT/p).read_text(encoding='utf-8-sig'))
def write(p, data): (ROOT/p).write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
def frames(name):
    source = Image.open(ROOT/'themes/upstream/reference'/name)
    result = []
    for i in range(source.n_frames):
        source.seek(i)
        result.append(source.convert('RGB').resize((64,64),Image.Resampling.NEAREST))
    return result
def asset(name, images):
    colors = sorted({p for im in images for p in im.get_flattened_data() if p is not None and len(p)==3 or p is not None and len(p)==4 and p[3]})
    colors = sorted({c[:3] for c in colors})
    assert 0 < len(colors) <= 255
    palette = {c:i for i,c in enumerate(colors)}
    digits=1 if len(colors)<=16 else 2
    return dict(id=name,width=images[0].width,height=images[0].height,encoding=4 if digits==1 else 8,
                palette=['#%02x%02x%02x'%c for c in colors],
                frames=[''.join('.'*digits if len(c)==4 and not c[3] else format(palette[c[:3]],'0'+str(digits)+'x') for c in im.get_flattened_data()) for im in images])

zoo = frames('21_zootopia.gif')
def heart(c): return c == (214,0,33)
base = zoo[0].copy()
for y in range(64):
    for x in range(64):
        if 23<=x<42 and 12<=y<27 and heart(base.getpixel((x,y))):
            candidates=[im.getpixel((x,y)) for im in zoo if not heart(im.getpixel((x,y)))]
            # The heart always covers the central gap; reconstruct card edge vertically.
            base.putpixel((x,y),candidates[0] if candidates else zoo[0].getpixel((x,39)))
draw=ImageDraw.Draw(base)
draw.rectangle((4,50,28,58),fill=zoo[0].getpixel((5,50)))
draw.rectangle((36,50,60,58),fill=zoo[0].getpixel((37,50)))
hearts=[]
for im in zoo:
    part=Image.new('RGBA',(19,15))
    for y in range(15):
        for x in range(19):
            c=im.getpixel((23+x,12+y))
            if heart(c):part.putpixel((x,y),(*c,255))
    if part.tobytes() not in [p.tobytes() for p in hearts]:hearts.append(part)
p=read('themes/gallery/zootopia.json')
p.update(assets=[asset('zoo_cards',[base]),asset('heart',hearts)],media=[],description='原版像素肖像 · Lua 心跳、独立眨眼、分钟互动',fidelity='原图像素资源，动画状态与随机间隔由 Lua 驱动')
write('themes/gallery/zootopia.json',p)
reference=[]
for i,im in enumerate(zoo):
    clean=im.copy();d=ImageDraw.Draw(clean)
    d.rectangle((4,50,28,58),fill=im.getpixel((5,50)))
    d.rectangle((36,50,60,58),fill=im.getpixel((37,50)))
    poses=[]
    for j,h in enumerate(hearts):
        composite=base.convert('RGBA');composite.alpha_composite(h,(23,12))
        if composite.convert('RGB').tobytes()==clean.tobytes():poses.append(j+1)
    assert poses, f'Zootopia reference frame {i} does not match sprite composition'
    raw=b''.join(struct.pack('<H',((r>>3)<<11)|((g>>2)<<5)|(b>>3)) for r,g,b in clean.get_flattened_data())
    reference.append(dict(pose=poses[0],sha256=hashlib.sha256(raw).hexdigest()))
write('themes/resources/zootopia-reference.json',reference)

pac=frames('02_pacman.gif')[0]
blue=pac.getpixel((0,0))
maze=Image.new('RGB',(64,64))
for y in range(64):
    for x in range(64):
        if pac.getpixel((x,y))==blue:maze.putpixel((x,y),blue)
grid=[]
for y in range(12):
    row=''
    for x in range(12):
        px,py=4+5*x,4+5*y
        blocked=maze.getpixel((px,py))!=(0,0,0) or (14<=px<=49 and 19<=py<=44)
        row+='#' if blocked else 'o' if x in [0,11] and y in [0,11] else '.'
    grid.append(row)
write('themes/resources/pacman-grid.json',grid)
# Bake the static maze's RGB565 glow once instead of filtering 4096 pixels each frame.
sharp=maze.copy()
for y in range(64):
    for x in range(64):
        c=sharp.getpixel((x,y))[2]>>3
        neighbors=sum(sharp.getpixel((max(0,min(63,nx)),max(0,min(63,ny))))[2]>>3 for nx,ny in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)])
        blue5=(c*(1020-4*36)+neighbors*36+510)//1020
        maze.putpixel((x,y),(0,0,round(blue5*255/31)))
p=read('themes/gallery/pacman.json')
p.update(assets=[asset('maze',[maze])],media=[],libraries=['pacman'],description='原版蓝黄迷宫 · 强化六色闪烁 · 自主觅食、避险与追击',fidelity='原版迷宫配色，Lua 实时寻路与强化动画')
write('themes/gallery/pacman.json',p)

for name in ['zootopia','super-mario','pacman']:
    path=f'themes/gallery/{name}.json'
    if (ROOT/path).exists():
        project=read(path)
        project['source']=(ROOT/f'themes/scripts/{name}.lua').read_text(encoding='utf-8')
        write(path,project)
print('Zootopia components:',len(hearts),'heart poses; maze:',grid)

