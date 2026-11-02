from pathlib import Path
import json,zipfile,uuid
r=json.loads(Path('routing/clean-pcb.json').read_text(encoding='utf8'));p=json.loads(Path('routing/with-pour-pcb.json').read_text(encoding='utf8'))
for a,b in p:
 if a['type']=='POUR':
  for layer in [1,2]:
   bb=dict(b,layerId=layer,name='GND_TOP' if layer==1 else 'GND_BOTTOM');bb['path']=[['R',0,1929.1339,2362.2047,1929.1339,0,0]]
   r.append([{'type':'POUR','ticket':33000+layer,'id':uuid.uuid4().hex[:16]},bb])
src='\n'.join(json.dumps(a,separators=(',',':'),ensure_ascii=False)+'||'+json.dumps(b,separators=(',',':'),ensure_ascii=False)+'|' for a,b in r).rstrip('|')
orig=Path('routing/original.epru').read_text(encoding='utf8');out=orig[:orig.rfind('{"type":"DOCHEAD"')]+src
Path('routing/clean-with-pours.epru').write_text(out,encoding='utf8')
with zipfile.ZipFile('enet/PCB1.epro2') as z,zipfile.ZipFile('routing/HUB75-routed.epro2','w',zipfile.ZIP_DEFLATED) as o:
 for n in z.namelist():o.writestr(n,out.encode() if n=='PCB1.epru' else z.read(n))
print('created',Path('routing/HUB75-routed.epro2').stat().st_size)
