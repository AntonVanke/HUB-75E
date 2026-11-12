import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {fetchUpdateJson,downloadUpdateAsset,normalizeUpdateSource,updateAsset,updateBases} from '../studio/update-sources.mjs';

test('mirror root validation and source order preserve HTTPS and relative asset paths',()=>{
  assert.equal(normalizeUpdateSource(' https://updates.example.cn/clockyr '),'https://updates.example.cn/clockyr/');
  for(const bad of ['http://example.cn','https://user:pwd@example.cn/','https://example.cn/?x=1','https://example.cn/../x','https://example.cn/%2e/'])assert.throws(()=>normalizeUpdateSource(bad));
  for(const bad of ['../app.bin','/app.bin','https://evil.cn/a','a?b'])assert.throws(()=>updateAsset(bad,'https://example.cn/'));
  assert.equal(updateBases('https://example.cn/')[0],'https://example.cn/');
  assert.equal(new Set(updateBases('',updateBases()[0])).size,3);
  assert.throws(()=>normalizeUpdateSource('https://example.cn/'+ 'a'.repeat(173)),/过长/);
});

test('catalog tries mirror, skips network failures and invalid JSON, and returns effective origin',async()=>{
  const urls=[];
  const result=await fetchUpdateJson('theme-catalog.json',{mirror:'https://updates.example.cn/',validate:v=>v.format===3,fetcher:async url=>{
    urls.push(url.href);if(urls.length===1)throw Error('Failed to fetch');
    return Response.json(urls.length===2?{format:2}:{format:3,apps:[]});
  }});
  assert.equal(urls.length,3);assert.match(urls[0],/^https:\/\/updates.example.cn\/theme-catalog.json\?t=/);
  assert.match(result.base,/jsdelivr/);
});

test('complete network failure cannot report up-to-date',async()=>{
  let calls=0;await assert.rejects(fetchUpdateJson('theme-catalog.json',{fetcher:async()=>{calls++;throw Error('offline');}}),/尚未确认是否为最新版本/);
  assert.equal(calls,3);
});

test('package fallbacks keep the chosen manifest hash, rejecting stale and oversized mirrors',async()=>{
  const body=new TextEncoder().encode('correct package');const hash=b=>createHash('sha256').update(b).digest('hex');
  const asset={path:'applications/1.10.0/theme.cwp',bytes:body.length,sha256:hash(body)};let calls=0;
  const result=await downloadUpdateAsset(asset,hash,{mirror:'https://example.cn/',fetcher:async()=>{
    calls++;return new Response(calls===1?'x'.repeat(body.length):calls===2?'too large'.repeat(10):body);
  }});
  assert.equal(calls,3);assert.deepEqual(result,body);
  await assert.rejects(downloadUpdateAsset(asset,hash,{fetcher:async()=>new Response('broken')}),/原应用保持不变/);
});
