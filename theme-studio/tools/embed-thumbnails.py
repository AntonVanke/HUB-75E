import json,io
from pathlib import Path
from PIL import Image
themes=json.loads(Path('themes/catalog.json').read_text(encoding='utf-8'))['themes']
out=['#pragma once\n#include <stddef.h>\n'];total=0
for i,t in enumerate(themes):
    im=Image.open('themes/thumbnails/'+t['id']+'.png').convert('RGB').resize((32,32),Image.Resampling.NEAREST).quantize(colors=24)
    buf=io.BytesIO();im.save(buf,format='PNG',optimize=True);b=buf.getvalue();total+=len(b)
    out.append('static const unsigned char THUMB_'+str(i)+'[]={'+','.join(map(str,b))+'};\n')
out.append('struct ThemeThumb {const unsigned char* data;size_t size;};\nstatic const ThemeThumb THEME_THUMBS[]={'+','.join('{THUMB_'+str(i)+',sizeof(THUMB_'+str(i)+')}' for i in range(len(themes)))+'};\n')
Path('embedded/demo/src/ThemeThumbs.h').write_text(''.join(out));print('Thumbnail flash bytes:',total)
