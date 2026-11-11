import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {themeUpdateText} from '../studio/releases.mjs';

test('status advertises the capabilities consumed by device and USB pages',()=>{
 const source=readFileSync(new URL('../embedded/demo/src/DeviceService.cpp',import.meta.url),'utf8');
 const status=source.slice(source.indexOf('String DeviceService::statusJson()'),source.indexOf('String DeviceService::trialJson()'));
 const keys=new Set();
 for(const file of ['../../web/app.mjs','../studio/device.html','../studio/diagnostics.mjs','../studio/setup-wizard.mjs','../studio/device-schedule.mjs','../studio/serial-device.mjs']){
  const ui=readFileSync(new URL(file,import.meta.url),'utf8');
  for(const match of ui.matchAll(/features\??\.(\w+)/g))keys.add(match[1]);
 }
 assert.ok(status.includes('\\"features\\":{'));
 for(const key of keys)assert.ok(status.includes('\\"'+key+'\\":'),'missing capability '+key);
 for(const key of ['orientation','sensorCapabilities','flashBytes','conditionControl'])assert.ok(status.includes('\\"'+key+'\\":'),'missing status payload '+key);
});

test('theme updates distinguish network/time/storage failures from success',()=>{
 assert.match(themeUpdateText({state:'waiting-time'}),/校准时间/);
 assert.match(themeUpdateText({state:'storage-limited',skipped:2}),/空间.*2 项/);
 assert.match(themeUpdateText({state:'retry',error:'package-download-or-hash-failed'}),/1 分钟.*SHA-256/);
 assert.doesNotMatch(themeUpdateText({state:'retry'}),/最新/);
 assert.match(themeUpdateText({state:'up-to-date'}),/全部安装/);
});
