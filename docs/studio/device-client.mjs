// The embedded WebServer handles one connection at a time, including uploads.
export function createDeviceClient(getBase){
 let tail=Promise.resolve(),pending=0,resumeAt=0;
 const wait=ms=>new Promise(resolve=>setTimeout(resolve,ms));
 function enqueue(work){
  pending++;
  const job=tail.then(async()=>{const delay=resumeAt-Date.now();if(delay>0)await wait(delay);return work();});
  tail=job.catch(()=>{});
  return job.finally(()=>pending--);
 }
 function hold(ms=8000){resumeAt=Math.max(resumeAt,Date.now()+ms);}
 function request(path,options={}){
  const origin=getBase(),{responseType='json',...init}=options,read=!init.method||init.method==='GET';
  return enqueue(async()=>{
   if(origin!==getBase())throw new Error('设备地址已切换，请重新操作');
   if(globalThis.location?.protocol==='https:'&&origin.startsWith('http:'))throw new Error('请直接打开设备的 HTTP 后台；HTTPS 工坊请使用 USB 串口连接');
   for(let attempt=0;;attempt++){
    let response,text;
    try{
     response=await fetch(origin+'/api/'+path,{...init,credentials:'omit',cache:'no-store',signal:init.signal||AbortSignal.timeout(12000)});
     text=await response.text();
    }catch(error){
     if(init.signal?.aborted)throw error;
     if(read&&attempt<2){await wait(400*(attempt+1));continue;}
     throw new Error(read?'设备暂时无法连接，请确认仍在同一 Wi-Fi；页面会自动重连。':'保存响应中断，结果尚未确认；请先重新读取设置，再决定是否重试。');
    }
    let body;
    if(responseType==='text'&&response.ok)return text;
    try{body=JSON.parse(text);}catch{throw new Error('设备返回的数据不完整，请重新读取状态');}
    if(!response.ok||body.ok===false)throw new Error(body.error||'HTTP '+response.status);
    if(!read&&(path==='panel'||path==='restart'||path==='factory-reset'||path==='wifi'||path==='panel/trial'&&['start','next','stop','keep'].includes(init.body?.get('action'))))hold();
    return body;
   }
  });
 }
 return {request,enqueue,hold,get busy(){return pending>0||Date.now()<resumeAt;}};
}
