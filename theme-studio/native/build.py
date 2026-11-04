"""Compile C/C++ themes to chip-specific Xtensa machine code and CWP3 packages."""
import argparse, hashlib, json, os, struct, subprocess, zipfile, zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
IMPORTS = ['memcpy','memset','memmove','memcmp','strlen','strcmp','sqrtf','sinf','cosf','floorf','fmodf','snprintf']
def align(n, a=4): return (n+a-1)&-a
def convert_elf(blob, chip):
    if blob[:7] != b'\x7fELF\x01\x01\x01': raise ValueError('Expected ELF32 little endian')
    h=struct.unpack_from('<16sHHIIIIIHHHHHH',blob)
    if h[2]!=94 or h[1]!=3: raise ValueError('Expected Xtensa shared ELF')
    sections=[struct.unpack_from('<10I',blob,h[6]+i*h[11]) for i in range(h[12])]
    names=blob[sections[h[13]][4]:sections[h[13]][4]+sections[h[13]][5]]
    def name(s): return names[s[0]:].split(b'\0',1)[0].decode()
    segments=[bytearray(),bytearray()]; mapping=[]
    for s in sections:
        n=name(s)
        if n not in ['.text','.rodata','.data','.data.rel.ro','.bss','.got']: continue
        if not s[5]:continue
        group=0 if n=='.text' else 1
        if s[8]>16: raise ValueError('Alignment above 16 unsupported')
        offset=align(len(segments[group]),max(4,s[8]))
        segments[group]+=bytes(offset-len(segments[group]))
        segments[group]+=bytes(s[5]) if s[1]==8 else blob[s[4]:s[4]+s[5]]
        mapping.append((s[3],s[5],group,offset))
    def address(v):
        for start,size,group,offset in mapping:
            if start<=v<start+size:return (group<<30)|(offset+v-start)
        raise ValueError(f'Unmapped ELF address 0x{v:x}')
    reloc=[]
    for s in sections:
        if s[1]!=4:continue
        syms=sections[s[6]]; strings=sections[syms[6]]
        for at in range(s[4],s[4]+s[5],12):
            where,info,addend=struct.unpack_from('<IIi',blob,at);kind=info&255
            if kind in [0,2]:continue
            patch=address(where);group=patch>>30;off=patch&0x3fffffff
            if off%4 or off+4>len(segments[group]):raise ValueError('Unaligned relocation')
            if kind==5:target=address(struct.unpack_from('<I',segments[group],off)[0])
            elif kind in [1,3,4]:
                symbol=struct.unpack_from('<IIIBBH',blob,syms[4]+(info>>8)*16)
                if symbol[5]:target=address(symbol[1]+addend)
                else:
                    n=blob[strings[4]+symbol[0]:].split(b'\0',1)[0].decode()
                    if n not in IMPORTS or addend:raise ValueError(f'Unsupported native import: {n} + {addend}')
                    target=(2<<30)|IMPORTS.index(n)
            else:raise ValueError(f'Unsupported Xtensa relocation {kind}')
            reloc.append((patch,target))
    for seg in segments:seg+=bytes(align(len(seg))-len(seg))
    if not segments[0] or len(segments[0])>96*1024 or len(segments[1])>256*1024:raise ValueError('Native segment exceeds limit')
    body=segments[0]+segments[1]+b''.join(struct.pack('<II',*r) for r in reloc)
    header=struct.pack('<4s9I',b'CWN1',1,1 if chip=='esp32' else 2,len(segments[0]),len(segments[1]),address(h[4]),len(reloc),zlib.crc32(body),0,0)
    return header+body,len(segments[0]),len(segments[1])

def pack(manifest, code, resources, archive=b''):
    metadata=json.dumps(manifest,ensure_ascii=False,separators=(',',':')).encode()
    if len(metadata)>2048 or len(resources)>8:raise ValueError('Too many resources or metadata too long')
    offset=24+40*len(resources)+len(metadata)+len(code)+len(archive)
    directory=bytearray();data=bytearray()
    for name,content in resources.items():
        if not name or len(name)>31 or not all(c.isascii() and (c.isalnum() or c in '-_') for c in name):raise ValueError('Invalid resource id')
        directory+=struct.pack('<32sII',name.encode(),offset,len(content));offset+=len(content);data+=content
    return struct.pack('<4s5I',b'CWP3',len(code),len(archive),len(resources),len(metadata),0)+directory+metadata+code+archive+data

def build(folder, chip, output):
    m=json.loads((folder/'theme.json').read_text(encoding='utf-8'))
    compiler=Path(os.environ.get('PLATFORMIO_CORE_DIR',str(Path.home()/'.platformio')))/'packages'/f'toolchain-xtensa-{chip}'/'bin'/f'xtensa-{chip}-elf-g++.exe'
    if not compiler.exists():compiler=compiler.with_suffix('')
    dest=ROOT/'build'/chip/folder.name;dest.mkdir(parents=True,exist_ok=True)
    sources=sorted([*folder.glob('*.cpp'),*folder.glob('*.c')])
    if not sources:raise ValueError('No C/C++ source files')
    elf=dest/'theme.elf'
    flags=['-O2','-std=c++11','-fPIC','-fvisibility=hidden','-mlongcalls','-mtext-section-literals','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-ffunction-sections','-fdata-sections','-nostdlib','-shared','-Wl,-Bsymbolic','-Wl,-e,cw_theme','-Wl,--gc-sections','-I'+str(ROOT/'include')]
    subprocess.run([str(compiler),*flags,*map(str,sources),'-lgcc','-o',str(elf)],check=True)
    code,exec_size,data_size=convert_elf(elf.read_bytes(),chip)
    (dest/'theme.cwn').write_bytes(code)
    m.update(runtime='native',nativeAbi=1,runtimeApi=4)
    r=m.setdefault('requirements',{});r['chips']=[chip];r['codeBytes']=exec_size;r['dataBytes']=data_size
    r['targetFps']=min(r.get('targetFps',60),60 if chip=='esp32' else 240)
    resources={k:(folder/v).read_bytes() for k,v in m.pop('resources',{}).items()}
    output.mkdir(parents=True,exist_ok=True)
    packed=pack(m,code,resources)
    filename=f'{m["id"]}-{chip}.cwp';(output/filename).write_bytes(packed)
    print(f'{filename}: code={exec_size}, data={data_size}, package={len(packed)}')
    return dict(m,file=filename,bytes=len(packed),sha256=hashlib.sha256(packed).hexdigest())

def main():
    p=argparse.ArgumentParser();p.add_argument('theme',nargs='?',default='all');p.add_argument('--chip',choices=['esp32','esp32s3','all'],default='all');p.add_argument('--output',type=Path,default=ROOT/'dist');a=p.parse_args()
    folders=sorted((ROOT/'themes').glob('*/theme.json')) if a.theme=='all' else [Path(a.theme)/'theme.json' if Path(a.theme).is_dir() else ROOT/'themes'/a.theme/'theme.json']
    entries=[]
    for f in folders:
        for chip in ['esp32','esp32s3'] if a.chip=='all' else [a.chip]:entries.append(build(f.parent,chip,a.output))
    (a.output/'catalog.json').write_text(json.dumps({'format':3,'apps':[x for x in entries if x['id']!='image-clock']},ensure_ascii=False,indent=2),encoding='utf-8')
    if a.theme=='all' and a.chip=='all':
        order=['mario-1-1','pacman','gravity-liquid','gravity-maze']
        catalog=['#pragma once','#ifdef HUB75_S3_BOARD']
        binary=['#pragma once','struct NativeBin {const char* id;const unsigned char* bytes;unsigned size;};','#ifdef HUB75_S3_BOARD']
        for chip in ['esp32s3','esp32']:
            if chip=='esp32':catalog.append('#else');binary.append('#else')
            rows=[]
            for i,id in enumerate(order+['image-clock']):
                row=next(x.copy() for x in entries if x['id']==id and x['requirements']['chips']==[chip]);row['index']=i;row['path']='/api/apps/builtin?id='+id
                if id!='image-clock':rows.append(row)
                blob=(a.output/row['file']).read_bytes()
                binary.append('static const unsigned char NATIVE_BIN_'+str(i)+'[]={'+','.join(map(str,blob))+'};')
            catalog.append('static const char THEME_TITLES[] = R"CWNATIVE('+json.dumps(rows,ensure_ascii=False,separators=(',',':'))+')CWNATIVE";')
        catalog.append('#endif');binary.extend(['#endif','static const NativeBin NATIVE_BINS[]={'])
        for i,id in enumerate(order+['image-clock']):binary.append('{"'+id+'",NATIVE_BIN_'+str(i)+',sizeof(NATIVE_BIN_'+str(i)+')},')
        binary.append('};')
        target=ROOT.parent/'embedded/demo/src'
        (target/'NativeCatalog.h').write_text('\n'.join(catalog)+'\n',encoding='utf-8')
        (target/'NativeBins.h').write_text('\n'.join(binary)+'\n',encoding='utf-8')
if __name__=='__main__':main()
