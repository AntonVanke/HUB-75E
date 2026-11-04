import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {gunzipSync} from 'node:zlib';
import {Script} from 'node:vm';

test('embedded pages preserve JavaScript replacement tokens and UTF-8',()=>{
  for(const name of ['DevicePage']){
    const header=readFileSync(new URL('../embedded/demo/src/'+name+'.h',import.meta.url),'utf8');
    const html=gunzipSync(Buffer.from(header.match(/\{([\d,]+)\}/)[1].split(',').map(Number))).toString('utf8');
    const scripts=[...html.matchAll(/<script[^>]*>([\s\S]*?)<\/script>/g)];
    assert.equal(scripts.length,1,name+' must contain only the bundled script');
    assert.doesNotThrow(()=>new Script(scripts[0][1]),name+' bundle must parse after HTML embedding');
    assert.ok(!html.includes('\ufffd'),'UTF-8 must not contain replacement characters');
    if(name==='DevicePage'){
      assert.match(html,/<meta name="clockwise-device" content="1">/);
      assert.match(html,/id="connection-form" hidden/);
      assert.ok(html.includes('设备管理'));
      assert.ok(!html.includes('<script type="module">'),'replacement must not interpolate the original module via $&');
    }
  }
});
