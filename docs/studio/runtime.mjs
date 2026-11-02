import { Graphics } from './graphics.mjs';
import { MediaClip } from './media.mjs';
import { GridSolver } from './navigation.mjs';

export class LuaRuntime extends Graphics {
  constructor(fengari, options = {}) {
    super();
    this.f = fengari;
    this.pixels = new Uint16Array(64 * 64);
    this.stats = {};
    this.date = options.date || (() => new Date());
    this.instructionLimit = options.instructionLimit || 800000;
    this.sourceLimit = options.sourceLimit || 256 * 1024;
    this.L = null;
    this.elapsed = 0;
    this.calls = 0;
    this.data = options.data || {}; this.keys = new Set(); this.handlers = new Map();
    this.now = this.date(); this.lastSecond = -1; this.lastMinute = -1;
    this.mediaResources=options.media||{};
    this.resources=options.resources||{};
    this.clips=new Map();
    this.randomSeed=(options.seed??Date.now())>>>0;
    this.navigator=new GridSolver();
  }
  load(kernel, source) {
    if (new TextEncoder().encode(kernel + source).length > this.sourceLimit) throw new Error('主题与运行库合计不能超过 '+Math.floor(this.sourceLimit/1024)+' KiB');
    this.close();
    const { lua, lauxlib, lualib, to_luastring: s } = this.f;
    const L = this.L = lauxlib.luaL_newstate();
    for (const [name, open] of [['_G', lualib.luaopen_base], ['math', lualib.luaopen_math], ['table', lualib.luaopen_table], ['string', lualib.luaopen_string]]) {
      lauxlib.luaL_requiref(L, s(name), open, 1); lua.lua_pop(L, 1);
    }
    for (const name of ['dofile', 'loadfile', 'load', 'collectgarbage', 'pcall', 'xpcall', 'print']) {
      lua.lua_pushnil(L); lua.lua_setglobal(L, s(name));
    }
    this.stats = {}; this.elapsed = 0; this.calls = 0; this.handlers.clear(); this.lastSecond = this.lastMinute = -1;
    this.assets.clear(); this.layers.clear(); this.effects.clear(); this.assetBytes = 0; this.clip(); this.now = this.date();
    const num = (i, fallback = 0) => lua.lua_isnoneornil(L, i) ? fallback : lauxlib.luaL_checknumber(L, i);
    const str = i => this.f.to_jsstring(lauxlib.luaL_checkstring(L, i));
    const col = i => this.color(lua.lua_type(L, i) === lua.LUA_TNUMBER ? num(i) : str(i));
    const table = (name, functions) => {
      lua.lua_newtable(L);
      for (const [key, fn] of Object.entries(functions)) {
        lua.lua_pushjsfunction(L, () => {
          try { return fn(); }
          catch (e) { return lauxlib.luaL_error(L, s('%s'), s(String(e.message || e))); }
        });
        lua.lua_setfield(L, -2, s(key));
      }
      lua.lua_setglobal(L, s(name));
    };
    table('gfx', {
      clear: () => { this.rect(0,0,64,64,col(1)); return 0; },
      pixel: () => { this.pixel(num(1), num(2), col(3)); return 0; },
      rect: () => { this.rect(num(1), num(2), num(3), num(4), col(5)); return 0; },
      line: () => { this.line(num(1),num(2),num(3),num(4),col(5));return 0; },
      circle: () => { this.circle(num(1),num(2),num(3),col(4));return 0; },
      outline: () => { this.outline(num(1),num(2),num(3),num(4),col(5));return 0; },
      triangle: () => { this.triangle(num(1),num(2),num(3),num(4),num(5),num(6),col(7));return 0; },
      gradient: () => { this.gradient(num(1),num(2),num(3),num(4),col(5),col(6));return 0; },
      clip: () => {this.clip(num(1),num(2),num(3),num(4));return 0;},
      reset_clip: () => {this.clip();return 0;},
      softglow: () => {this.softglow(num(1,80));return 0;},
      wave: () => {this.wave(num(1),num(2),num(3),num(4),num(5),num(6,1),num(7,12),num(8));return 0;},
      daylight: () => {this.daylight(num(1),num(2),num(3),num(4),num(5,64),num(6,48));return 0;},
      text: () => { this.text(num(1), num(2), str(3), col(4), num(5, 1)); return 0; },
      text_width: () => {lua.lua_pushnumber(L,this.textWidth(str(1),num(2,1)));return 1;}
    });
    table('assets',{define:()=>{this.defineAsset(str(1),num(2),num(3),str(4),str(5),num(6,1));return 0;}});
    table('resource',{
      rle4:()=>{const bytes=lauxlib.luaL_checklstring(L,1),packed=!!lua.lua_toboolean(L,2),hex='0123456789abcdef';let total=0;for(const b of bytes){total+=(b>>4)+1;if(total>8192)throw new Error('RLE4 output exceeds 8192 bytes');}const out=new Uint8Array(packed?Math.ceil(total/2):total);let at=0;for(const b of bytes){const count=(b>>4)+1;for(let j=0;j<count;j++,at++)if(packed)out[at>>1]|=(b&15)<<((at&1)*4);else out[at]=hex.charCodeAt(b&15);}lua.lua_pushlstring(L,out,out.length);return 1;},
      size:()=>{const b=this.resources[str(1)];if(!b)throw new Error('Resource not found: '+str(1));lua.lua_pushinteger(L,b.byteLength);return 1;},
      read:()=>{const b=this.resources[str(1)],offset=num(2),count=num(3);if(!b||!Number.isInteger(offset)||!Number.isInteger(count)||offset<0||count<0||count>8192||offset+count>b.byteLength)throw new Error('Resource read outside bounds (max 8192 bytes)');lua.lua_pushlstring(L,new Uint8Array(b.buffer||b,b.byteOffset||0,b.byteLength).subarray(offset,offset+count),count);return 1;}
    });
    table('memory',{collect:()=>{lua.lua_gc(L,lua.LUA_GCCOLLECT,0);lua.lua_pushinteger(L,0);return 1;}});
    const navigate=weighted=>{
      const w=num(1),h=num(2),map=str(3),start=num(4),n=w*h;
      if(weighted){
        lauxlib.luaL_checktype(L,5,lua.LUA_TTABLE);
        if(n>256)throw new Error('网格最多 256 格');
        for(let i=0;i<n;i++){lua.lua_rawgeti(L,5,i+1);const v=lua.lua_isnil(L,-1)?1:lauxlib.luaL_checknumber(L,-1);lua.lua_pop(L,1);if(!Number.isInteger(v)||v<0||v>1000000)throw new Error('路径代价必须是 0–1000000 的整数');this.navigator.costs[i]=v;}
      }
      const result=this.navigator.solve(w,h,map,start,weighted);
      const packed=lua.lua_toboolean(L,weighted?6:5);
      for(const name of weighted?['distance','first','parent']:['distance']){
        if(packed){
          let maximum=0;for(let i=0;i<result.n;i++)if(result.distance[i]!==0xffffffff)maximum=Math.max(maximum,result[name][i]);
          const width=maximum<255?1:maximum<65535?2:4,bytes=new Uint8Array(1+result.n*width);bytes[0]=width;
          for(let i=0;i<result.n;i++){
            const v=result.distance[i]===0xffffffff||(name!=='distance'&&!result[name][i])?0xffffffff:result[name][i];
            for(let b=0;b<width;b++)bytes[1+i*width+b]=(v>>>(b*8))&255;
          }
          lua.lua_pushlstring(L,bytes,bytes.length);continue;
        }
        lua.lua_createtable(L,result.n,0);
        for(let i=0;i<result.n;i++)if(result.distance[i]!==0xffffffff&&(name==='distance'||result[name][i])){lua.lua_pushnumber(L,result[name][i]);lua.lua_rawseti(L,-2,i+1);}
      }
      return weighted?3:1;
    };
    table('nav',{bfs:()=>navigate(false),solve:()=>navigate(true),at:()=>{
      const bytes=lauxlib.luaL_checklstring(L,1),i=num(2)-1;
      const width=bytes[0];
      if(![1,2,4].includes(width)||(bytes.length-1)%width||!Number.isInteger(i)||i<0||i>=(bytes.length-1)/width)throw new Error('无效的紧凑网格索引');
      const n=1+i*width;let v=0;for(let b=0;b<width;b++)v=(v|(bytes[n+b]<<(b*8)))>>>0;
      if(v===(width===1?255:width===2?65535:0xffffffff))lua.lua_pushnil(L);else lua.lua_pushnumber(L,v);return 1;
    }});
    table('font',{draw:()=>{this.fontText(str(1),num(2),num(3),str(4),col(5),num(6,1));return 0;}});
    table('media',{release:()=>{this.clips.delete(str(1));return 0;},draw:()=>{
      const id=str(1);let clip=this.clips.get(id);
      if(!clip){if(this.clips.size>=2)throw new Error('最多同时使用两个动画资源');if(!this.mediaResources[id])throw new Error('工程未声明动画资源：'+id);clip=new MediaClip(this.mediaResources[id]);this.clips.set(id,clip);}
      this.blit(clip.at(num(4,this.elapsed)),num(2),num(3));return 0;
    }});
    const random=()=>{let x=this.randomSeed||0x6d2b79f5;x^=x<<13;x^=x>>>17;x^=x<<5;this.randomSeed=x>>>0;return this.randomSeed/4294967296;};
    table('rng',{
      seed:()=>{const seed=num(1);if(!Number.isInteger(seed)||seed<0||seed>4294967295)throw new Error("随机种子必须是 uint32");this.randomSeed=seed>>>0;return 0;},
      int:()=>{const a=Math.floor(num(1)),b=Math.floor(num(2));if(a < -1000000||b>1000000||b<a||b-a>1000000)throw new Error('随机范围无效');lua.lua_pushnumber(L,a+Math.floor(random()*(b-a+1)));return 1;},
      float:()=>{lua.lua_pushnumber(L,random());return 1;}
    });
    table('sprite',{
      tilemap:()=>{
        const id=str(1),rows=num(6,0),map=rows?lauxlib.luaL_checklstring(L,2):str(2),cols=num(3),x=Math.floor(num(4)),y=Math.floor(num(5)),a=this.assets.get(id),clip=this.clipBox;
        if(!a||!Number.isInteger(cols)||cols<1||cols>4096||(rows?rows<1||!Number.isInteger(rows)||Math.ceil(cols*rows/2)!==map.length:map.length%cols)||map.length>65536)throw new Error('无效的图块地图');
        for(let r=Math.max(0,Math.floor((clip[1]-y)/a.h));r<Math.min(rows||map.length/cols,Math.ceil((clip[3]-y)/a.h));r++)
          for(let c=Math.max(0,Math.floor((clip[0]-x)/a.w));c<Math.min(cols,Math.ceil((clip[2]-x)/a.w));c++){
            const index=r*cols+c,frame=rows?(map[index>>1]>>((index&1)*4))&15:parseInt(map[index],16);if(frame>0)this.sprite(id,x+c*a.w,y+r*a.h,frame);
          }
        return 0;
      },
      draw:()=>{this.sprite(str(1),num(2),num(3),num(4,1),num(5,1),!!lua.lua_toboolean(L,6));return 0;},
      frames:()=>{const a=this.assets.get(str(1));if(!a)throw new Error('精灵不存在');lua.lua_pushnumber(L,a.frames.length);return 1;}
    });
    table('layer',{save:()=>{this.saveLayer(str(1));return 0;},draw:()=>{this.drawLayer(str(1),num(2),num(3));return 0;}});
    table('fx',{
      create:()=>{this.createEffect(str(1),num(2),num(3),num(4),num(5),num(6),num(7),num(8),col(9),str(10));return 0;},
      draw:()=>{this.drawEffect(str(1));return 0;}
    });
    table('data',{get:()=>{
      const value=this.data[str(1)];
      if(value==null||(typeof value==='number'&&!Number.isFinite(value)))lua.lua_pushvalue(L,2);
      else if(typeof value==='number')lua.lua_pushnumber(L,value);
      else if(typeof value==='boolean')lua.lua_pushboolean(L,value);
      else lua.lua_pushstring(L,s(String(value)));
      return 1;
    }});
    table('input',{pressed:()=>{lua.lua_pushboolean(L,this.keys.has(str(1)));return 1;}});
    table('clock', {
      format: () => {
        const d = this.now, pad = x => String(x).padStart(2, '0');
        const out = str(1).replace(/%[HMSdm]/g, x => ({'%H': pad(d.getHours()), '%M': pad(d.getMinutes()), '%S': pad(d.getSeconds()), '%d': pad(d.getDate()), '%m': pad(d.getMonth() + 1)})[x]);
        lua.lua_pushstring(L, s(out)); return 1;
      },
      millis: () => { lua.lua_pushnumber(L, this.elapsed); return 1; },
      utc: () => {
        const d=this.now, values={hour:d.getUTCHours(),minute:d.getUTCMinutes(),second:d.getUTCSeconds(),day:d.getUTCDate(),month:d.getUTCMonth()+1,year:d.getUTCFullYear(),weekday:d.getUTCDay(),yearday:Math.floor((Date.UTC(d.getUTCFullYear(),d.getUTCMonth(),d.getUTCDate())-Date.UTC(d.getUTCFullYear(),0,1))/86400000)+1};
        const value=values[str(1)];if(value===undefined)throw new Error('未知 UTC 字段');lua.lua_pushnumber(L,value);return 1;
      },
      value: () => {
        const d=this.now, values={hour:d.getHours(),minute:d.getMinutes(),second:d.getSeconds(),millisecond:d.getMilliseconds(),day:d.getDate(),month:d.getMonth()+1,year:d.getFullYear(),weekday:d.getDay()};
        const value=values[str(1)];if(value===undefined)throw new Error('未知时间字段');lua.lua_pushnumber(L,value);return 1;
      }
    });
    table('telemetry', {
      set: () => {
        const key = str(1).slice(0, 40);
        if (Object.keys(this.stats).length >= 40 && !(key in this.stats)) return 0;
        const type = lua.lua_type(L, 2);
        this.stats[key] = type === lua.LUA_TNUMBER ? num(2) : type === lua.LUA_TBOOLEAN ? !!lua.lua_toboolean(L, 2) : str(2).slice(0, 200);
        return 0;
      }
    });
    this.clips.clear();
    this.execute(kernel, '@libraries.lua');
    this.execute(source, '@theme.lua');
    for(const name of ['start','update','draw','second','minute','button']){
      lua.lua_getglobal(L,s(name));
      if(lua.lua_isfunction(L,-1))this.handlers.set(name,lauxlib.luaL_ref(L,lua.LUA_REGISTRYINDEX));
      else lua.lua_pop(L,1);
    }
    this.call('start');
    this.call('draw');
  }
  budget() {
    const { lua, lauxlib, to_luastring: s } = this.f;
    let count = 0;
    lua.lua_sethook(this.L, () => {
      count += 1000;
      if (count > this.instructionLimit) lauxlib.luaL_error(this.L, s('单次执行超出指令预算；请拆分循环到多帧执行'));
    }, lua.LUA_MASKCOUNT, 1000);
  }
  error() {
    const { lua, lauxlib, to_luastring: s, to_jsstring } = this.f;
    const raw = lua.lua_tostring(this.L, -1);
    const message = raw ? to_jsstring(raw) : 'Lua 执行错误';
    lauxlib.luaL_traceback(this.L, this.L, s(message), 1);
    const trace = to_jsstring(lua.lua_tostring(this.L, -1));
    lua.lua_settop(this.L, 0);
    return new Error(trace);
  }
  execute(source, name) {
    if(source.startsWith('-- @cw-bundle 1\n')){
      for(const part of source.slice(15).split('\n-- @cw-part\n'))this.execute(part,name);
      return;
    }
    const { lua, lauxlib, to_luastring: s } = this.f;
    if (lauxlib.luaL_loadbuffer(this.L, s(source), null, s(name)) !== lua.LUA_OK) throw this.error();
    this.budget();
    if (lua.lua_pcall(this.L, 0, 0, 0) !== lua.LUA_OK) throw this.error();
  }
  call(name, ...args) {
    const { lua, to_luastring: s } = this.f;
    if(!this.handlers.has(name))return;
    lua.lua_rawgeti(this.L,lua.LUA_REGISTRYINDEX,this.handlers.get(name));
    for (const arg of args) typeof arg==='string'?lua.lua_pushstring(this.L,s(arg)):lua.lua_pushnumber(this.L,arg);
    this.budget();
    if (lua.lua_pcall(this.L, args.length, 0, 0) !== lua.LUA_OK) throw this.error();
    this.calls++;
  }
  tick(dt = 40, draw = true) {
    this.elapsed += dt;
    this.now=this.date();this.updateEffects(dt);
    const t=this.now,minute=(((t.getFullYear()*12+t.getMonth()+1)*32+t.getDate())*24+t.getHours())*60+t.getMinutes(),second=minute*60+t.getSeconds();
    if(second!==this.lastSecond){this.lastSecond=second;this.call('second');}
    if(minute!==this.lastMinute){this.lastMinute=minute;this.call('minute');}
    this.call('update', dt);
    if (draw) this.call('draw');
  }
  close() {
    if (this.L) this.f.lua.lua_close(this.L);
    this.L = null;
  }
}
