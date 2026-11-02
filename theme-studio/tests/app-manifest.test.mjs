import test from 'node:test';
import assert from 'node:assert/strict';
import {strFromU8,zipSync,strToU8} from 'fflate';
import {normalizeAppManifest,checkAppCompatibility} from '../studio/app-manifest.mjs';
import {exportPackage,importPackage,readZip,devicePackage} from '../studio/theme-package.mjs';

const requirements={chips:['esp32','esp32s3'],psramBytes:0,luaHeapBytes:65536,luaWorkingBytes:32768,graphicsBytes:16384,targetFps:30};
const project={id:'example',packageName:'org.example.clock',title:'示例时钟',appVersion:'1.2.3',description:'包含动画与资源的时钟。',author:'作者',runtimeApi:1,requirements,width:64,height:64,assets:[],libraries:[],source:'function draw() gfx.clear(0) end',resourceData:{map:[1,2,3]},resources:['map']};

test('CWP2 exposes bounded metadata without decompressing its editable source archive',async()=>{
  const {zip}=await exportPackage(project),imported=await importPackage(readZip(zip));
  assert.equal(imported.packageName,project.packageName);assert.equal(imported.appVersion,'1.2.3');assert.deepEqual(imported.requirements,requirements);
  assert.equal(imported.fps,30);assert.equal(imported.verification.status,'unverified');
  const packed=devicePackage(project,{},zip),view=new DataView(packed.buffer),metadataStart=24+40*view.getUint32(12,true),metadataSize=view.getUint32(16,true);
  assert.equal(strFromU8(packed.subarray(0,4)),'CWP2');assert.equal(view.getUint32(20,true),0);assert.ok(metadataSize<=2048);
  const metadata=JSON.parse(strFromU8(packed.subarray(metadataStart,metadataStart+metadataSize)));
  assert.deepEqual(metadata,normalizeAppManifest(project));assert.equal(metadata.requirementsDeclared,true);
  const sourceStart=metadataStart+metadataSize,archiveStart=sourceStart+view.getUint32(4,true),resourceStart=view.getUint32(56,true);
  assert.deepEqual(packed.subarray(archiveStart,archiveStart+view.getUint32(8,true)),zip);
  assert.deepEqual([...packed.subarray(resourceStart)],[1,2,3]);
});

test('device admission checks actual chip families, memory quotas, API and physical PSRAM',()=>{
  const classic={chip:'ESP32-D0WD',limits:{runtimeApi:1,psramBytes:0,luaHeapBytes:131072,luaWorkingBytes:65536,graphicsBytes:65536,targetFpsMax:60}};
  assert.equal(checkAppCompatibility(project,classic).compatible,true);
  assert.equal(checkAppCompatibility({...project,requirements:{...requirements,chips:['esp32s3']}},classic).compatible,false);
  assert.equal(checkAppCompatibility({...project,runtimeApi:2},classic).compatible,false);
  assert.equal(checkAppCompatibility({...project,requirements:{...requirements,luaWorkingBytes:70000,luaHeapBytes:100000}},classic).compatible,false);
  assert.equal(checkAppCompatibility(project,{...classic,chip:'ESP32-C3'}).compatible,false);
  const s3={chip:'ESP32-S3',limits:{...classic.limits,psramBytes:8386247}};
  assert.equal(checkAppCompatibility({...project,requirements:{...requirements,psramBytes:8388608}},s3).compatible,true);
  assert.ok(checkAppCompatibility(project,classic).warnings.some(text=>text.includes('不是流畅度保证')));
});

test('legacy manifests remain importable with unknown requirements while new metadata rejects bad identities',async()=>{
  const files={'theme.json':strToU8(JSON.stringify({format:'clockwise-theme-package',version:1,id:'old-clock',width:64,height:64,entry:'main.lua'})),'main.lua':strToU8('function draw() end')};
  const imported=await importPackage(readZip(zipSync(files)));
  assert.equal(imported.packageName,'local.clockwise.old-clock');assert.equal(imported.requirementsDeclared,false);
  assert.equal(checkAppCompatibility(null).known,false);
  assert.throws(()=>normalizeAppManifest({...project,packageName:'No Namespace'}),/包名/);
  assert.throws(()=>normalizeAppManifest({...project,title:'中'.repeat(33)}),/96 UTF-8/);
  assert.throws(()=>normalizeAppManifest({...project,appVersion:'latest'}),/语义版本/);
  assert.throws(()=>normalizeAppManifest({...project,requirements:{...requirements,targetFps:61}}),/帧率/);
  assert.throws(()=>normalizeAppManifest({...project,requirements:{...requirements,luaHeapBytes:10,luaWorkingBytes:11}}),/运行内存/);
});
