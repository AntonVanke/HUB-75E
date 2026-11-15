export function eventLogControls(root,{request,message,translate=t=>t}){
  root.innerHTML='<h2>重要事件日志</h2><p>只保留最近 16 条启动、主动重启、更新和关键故障记录；重启后仍可读取。不记录 Wi-Fi 密码、逐帧数据或正常的版本检查。</p><div class="row"><button type="button" data-read>读取日志</button><button type="button" data-export disabled>导出日志</button></div><p data-state role="status"></p><ol data-events style="max-height:320px;overflow:auto;padding-left:24px"></ol>';
  const read=root.querySelector('[data-read]'),save=root.querySelector('[data-export]'),state=root.querySelector('[data-state]'),list=root.querySelector('[data-events]');let data;
  const crash=document.createElement('details'),title=document.createElement('summary'),report=document.createElement('pre');
  crash.hidden=true;title.textContent=translate('最近一次崩溃详情');report.style.cssText='white-space:pre-wrap;overflow-wrap:anywhere';crash.append(title,report);root.append(crash);
  const labels={'power-on':'上电','software-reset':'软件重启','external-reset':'外部复位',panic:'程序异常',brownout:'供电欠压','task-watchdog':'任务看门狗','interrupt-watchdog':'中断看门狗',watchdog:'看门狗',boot:'启动',restart:'主动重启','update-start':'开始更新','update-complete':'更新完成','update-failed':'更新失败','update-check-failed':'版本检查失败','theme-failed':'主题错误','panel-failed':'屏幕错误'};
  read.onclick=async()=>{read.disabled=true;try{
    data=await request('logs');state.textContent=translate('本次启动原因：')+translate(labels[data.resetReason]||data.resetReason)+(data.persistent?'':translate('；日志存储不可用'));
    crash.hidden=!data.crash;
    if(data.crash){const c=data.crash;report.textContent=[
      '#'+c.boot+' · v'+c.version+' · '+(c.sampleEpoch?new Date(c.sampleEpoch*1000).toLocaleString():translate('时间未校准')),
      'Core '+c.core+' · Exception '+c.exception+' · EXCCAUSE '+c.cause,
      'PC '+c.pc+' · SP '+c.sp+' · EXCVADDR '+c.address,
      'Backtrace: '+c.backtrace.join(' → ')+(c.truncated?' ('+translate('回溯可能不完整')+')':''),
      translate('最近接口：')+(c.request.route||'—')+' · '+(c.request.active?translate('处理中'):translate('已返回'))+' · +'+c.request.at+'s',
      translate('崩溃前采样：')+'+'+c.sampleUptime+'s · '+translate('主题')+' #'+c.theme+' · generation '+c.generation,
      'Heap '+c.heap+' B · minimum '+c.minimumHeap+' B · largest '+c.largestBlock+' B',
      'Stack free: service '+c.serviceStackFree+' B · render '+c.render.stackFree+' B',
      'ELF SHA256: '+c.elfSha256,
      translate('内存和主题为最近一次采样；最近接口不一定是崩溃原因。'),
      ...(!c.contextValid||!c.render.valid?[translate('部分上下文尚未采样或采样被打断，请结合回溯判断。')]:[])
    ].join('\n');crash.open=true;}
    list.replaceChildren(...data.events.map(event=>{const li=document.createElement('li');li.style.marginBottom='10px';li.textContent=(event.epoch?new Date(event.epoch*1000).toLocaleString():translate('时间未校准'))+' · #'+event.boot+' +'+event.uptime+'s · '+translate(labels[event.code]||event.code)+' · '+event.detail+' · '+Math.round(event.heap/1024)+' KiB';return li;}));save.disabled=false;
  }catch(e){message(e.message,true);}finally{read.disabled=false;}};
  save.onclick=()=>{const url=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download='clockyr-events.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);};
}
