import {nativeInfo} from './native-package.mjs';
export class SerialDevice {
  constructor(onState=()=>{}){this.onState=onState;this.port=null;this.reader=null;this.pending=null;this.device={};this.busy=false;}
  async connect(selected){
    if(this.port)return this.device;
    if(!selected&&!globalThis.navigator?.serial)throw Error('请使用支持 Web Serial 的桌面 Chrome / Edge');
    const port=selected||await navigator.serial.requestPort();
    await port.open({baudRate:115200,flowControl:'none'});this.port=port;this.task=this.read(port);
    try{this.device=await this.request({cmd:'status'},d=>!!d.chip,12000);this.onState(this.device);return this.device;}
    catch(e){await this.disconnect();throw e;}
  }
  async read(port){
    const reader=port.readable.getReader();this.reader=reader;const decoder=new TextDecoder();let buffer='';
    try{while(this.port===port){const {value,done}=await reader.read();if(done)break;buffer+=decoder.decode(value,{stream:true});let end;
      while((end=buffer.indexOf('\n'))>=0){const line=buffer.slice(0,end);buffer=buffer.slice(end+1);const start=line.indexOf('CWJSON:');if(start<0)continue;try{const data=JSON.parse(line.slice(start+7));if(data.chip)this.device=data;this.pending?.receive(data);}catch{}}
      if(buffer.length>262144)throw Error('设备响应超过上限');
    }}catch(e){this.pending?.reject(e);}finally{reader.releaseLock();this.reader=null;this.pending?.reject(Error('串口已断开'));if(this.port===port){this.port=null;await port.close().catch(()=>{});this.device={};this.onState({});}}
  }
  request(command,match=d=>typeof d.ok==='boolean',timeout=15000){
    if(!this.port?.writable||this.pending)return Promise.reject(Error('请连接串口并等待当前操作完成'));
    const bytes=new TextEncoder().encode(JSON.stringify(command)+'\n');if(bytes.length>511)return Promise.reject(Error('串口命令过长'));
    return new Promise((resolve,reject)=>{let timer;const finish=(fn,data)=>{clearTimeout(timer);this.pending=null;fn(data);};
      this.pending={reject:e=>finish(reject,e),receive:d=>{if(d.ok===false||d.error)finish(reject,Error(d.error||d.result?.error||'设备拒绝操作'));else if(match(d))finish(resolve,d);}};
      timer=setTimeout(()=>this.pending?.reject(Error('设备响应超时')),timeout);
      const writer=this.port.writable.getWriter();writer.write(bytes).catch(e=>this.pending?.reject(e)).finally(()=>writer.releaseLock());
    });
  }
  async disconnect(){const port=this.port;this.port=null;this.pending?.reject(Error('串口已断开'));await this.reader?.cancel().catch(()=>{});await this.task;await port?.close().catch(()=>{});this.device={};this.onState({});}
  async install(bytes,progress=()=>{},expectedHash=''){
    if(this.busy)throw Error('应用正在安装');this.busy=true;let started=false;
    try{
      const device=await this.request({cmd:'status'},d=>!!d.chip);const info=nativeInfo(bytes),chip=/S3/i.test(device.chip)?'esp32s3':'esp32';
      if(device.packageVersion<3||!device.features?.serialAppInstall)throw Error('设备需要原生主题固件 1.0.1 或更新版本');
      if(!info.requirements.chips.includes(chip))throw Error('二进制芯片不匹配');
      if(bytes.length>Number(device.limits?.appBytes||1048576))throw Error('应用超出设备容量');
      if((info.requirements.sensors||[]).some(s=>!device.sensorCapabilities?.includes(s)))throw Error('设备缺少此主题要求的传感器');
      for(const k of ['codeBytes','dataBytes','stateBytes','graphicsBytes'])if(Number(info.requirements[k]||0)>Number(device.limits?.[k]??Infinity))throw Error('主题内存需求超出设备配额');
      const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),v=>v.toString(16).padStart(2,'0')).join('');
      if(expectedHash&&hash!==expectedHash)throw Error('主题 SHA-256 校验失败');
      await this.request({cmd:'app.install.begin',packageName:info.packageName,bytes:bytes.length,sha256:hash},d=>d.upload&&d.phase==='begin');started=true;
      for(let offset=0;offset<bytes.length;offset+=300){const chunk=bytes.subarray(offset,offset+300);await this.request({cmd:'app.install.chunk',offset,data:btoa(String.fromCharCode(...chunk))},d=>d.upload&&d.phase==='chunk'&&d.received===offset+chunk.length);progress(Math.round((offset+chunk.length)*100/bytes.length));}
      const result=await this.request({cmd:'app.install.finish'},d=>d.upload&&d.phase==='finish',45000);started=false;
      if(result.result?.error)throw Error(result.result.error);return info;
    }catch(e){if(started&&this.port)await this.request({cmd:'app.install.abort'},d=>d.upload&&d.phase==='abort',3000).catch(()=>{});throw e;}finally{this.busy=false;}
  }
}
