"""Compile city artwork to the panel's 64x64 RGB565 asset format."""
import hashlib,json,struct
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[1];res=root/'themes/resources'
im=Image.open(res/'spider-city-source.png').convert('RGB').resize((64,64),Image.Resampling.BOX)
pixels=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in im.get_flattened_data()]
body=bytearray();i=0
while i<4096:
    j=i+1
    while j<4096 and pixels[j]==pixels[i]:j+=1
    if j-i>=3:body+=struct.pack('<HH',0x4000|j-i,pixels[i]);i=j;continue
    j=i+1
    while j<4096 and not (j+2<4096 and pixels[j]==pixels[j+1]==pixels[j+2]):j+=1
    body+=struct.pack('<H',j-i)+struct.pack('<'+'H'*(j-i),*pixels[i:j]);i=j
blob=struct.pack('<4sHHHHI',b'CWA1',64,64,1,16,1000)+struct.pack('<IIII',0,len(body),1000,1)+body
(res/'spider-city.cwa').write_bytes(blob)
manifest=json.loads((res/'manifest.json').read_text(encoding='utf-8'))
entry=dict(id='spider-city',path='/themes/resources/spider-city.cwa',hashes=[hashlib.sha256(struct.pack('<4096H',*pixels)).hexdigest()],frames=1,duration=1000,bytes=len(blob),source='Generated city background; imagegen; spider-city-source.png')
manifest=[m for m in manifest if m['id']!='spider-city']+[entry]
(res/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
