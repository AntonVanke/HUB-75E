import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {SerialDevice} from '../studio/serial-device.mjs';
import {themePrompt} from '../studio/theme-prompt.mjs';
const bytes=new Uint8Array(readFileSync(new URL('../native/dist/pacman-esp32s3.cwp',import.meta.url)));
function fakePort({failChunk=false,chip='ESP32-S3'}={}){
 let feed,received=0;const commands=[],data=[];
 const status={chip,version:'1.2.0',packageVersion:3,features:{serialAppInstall:1,wifiScan:1},sensorCapabilities:[],limits:{appBytes:1048576}};
 const port={commands,data,open:async()=>{},close:async()=>{},readable:new ReadableStream({start(c){feed=c;}}),writable:new WritableStream({write(raw){
  assert.ok(raw.length<=511);const cmd=JSON.parse(new TextDecoder().decode(raw));commands.push(cmd);let response;
  if(cmd.cmd==='status')response=status;
  else if(cmd.cmd==='wifi.scan')response={state:'ready',networks:[{ssid:'中文 Wi-Fi',rssi:-43,open:false}]};
  else if(cmd.cmd==='app.install.begin')response={ok:true,upload:true,phase:'begin'};
  else if(cmd.cmd==='app.install.chunk'){if(failChunk)response={ok:false,error:'write failed'};else{assert.equal(cmd.offset,received);const part=Uint8Array.from(atob(cmd.data),c=>c.charCodeAt(0));data.push(...part);received+=part.length;response={ok:true,upload:true,phase:'chunk',received};}}
  else response={ok:true,upload:true,phase:cmd.cmd.split('.').at(-1)};
  const encoded=new TextEncoder().encode('[SENSORS] ignored log\nCWJSON:'+JSON.stringify(response)+'\n');
  // Split UTF-8 and protocol lines across reads, as a real serial port can.
  for(let i=0;i<encoded.length;i+=11)feed.enqueue(encoded.slice(i,i+11));
 }})};return port;
}
test('USB protocol installs all bytes in acknowledged bounded chunks and decodes network names',async()=>{
 const port=fakePort(),client=new SerialDevice();await client.connect(port);const scan=await client.request({cmd:'wifi.scan'},d=>!!d.networks);assert.equal(scan.networks[0].ssid,'中文 Wi-Fi');let progress=0;await client.install(bytes,p=>progress=p);assert.deepEqual(new Uint8Array(port.data),bytes);assert.equal(progress,100);assert.equal(port.commands.at(-1).cmd,'app.install.finish');await client.disconnect();
});
test('wrong chip is rejected before begin, failed chunk aborts the transaction',async()=>{
 let port=fakePort({chip:'ESP32'}),client=new SerialDevice();await client.connect(port);await assert.rejects(client.install(bytes),/芯片/);assert.ok(!port.commands.some(c=>c.cmd==='app.install.begin'));await client.disconnect();
 port=fakePort({failChunk:true});client=new SerialDevice();await client.connect(port);await assert.rejects(client.install(bytes),/write failed/);assert.equal(port.commands.at(-1).cmd,'app.install.abort');assert.equal(client.busy,false);await client.disconnect();
});
test('generated prompt supplies project, ABI, build path, chip limits and user idea',()=>{
 const s=themePrompt({chip:'esp32',id:'test-water',idea:'可交互的水',fps:120,sensors:['imu']});assert.ok(s.includes('https://github.com/AntonVanke/HUB-75E'));assert.ok(s.includes('theme-studio/native/include/cw_theme.h'));assert.ok(s.includes('test-water --chip esp32'));assert.ok(s.includes('targetFps: 60'));assert.ok(s.includes('可交互的水'));assert.ok(s.includes('所需传感器: imu'));
});
