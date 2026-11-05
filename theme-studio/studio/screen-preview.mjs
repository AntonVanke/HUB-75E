export function decodeScreenFrame(frame){
 const source=frame instanceof ArrayBuffer?new Uint8Array(frame):frame instanceof Uint8Array?frame:Uint8Array.from(atob(frame.data||''),c=>c.charCodeAt(0));
 const rgb=new Uint8ClampedArray(64*64*4);let pixel=0;
 const put=(lo,hi,count)=>{if(!count||pixel+count>4096)throw Error('屏幕数据长度无效');const v=lo|hi<<8,r=(v>>11)*255/31,g=((v>>5)&63)*255/63,b=(v&31)*255/31;for(let i=0;i<count;i++){const at=pixel++*4;rgb[at]=r;rgb[at+1]=g;rgb[at+2]=b;rgb[at+3]=255;}};
 if(frame.encoding==='rle565'){if(source.length%3)throw Error('屏幕数据不完整');for(let i=0;i<source.length;i+=3)put(source[i+1],source[i+2],source[i]);}
 else{if(frame.encoding&&frame.encoding!=='rgb565le')throw Error('未知屏幕编码');if(source.length!==8192)throw Error('屏幕数据不完整');for(let i=0;i<source.length;i+=2)put(source[i],source[i+1],1);}
 if(pixel!==4096)throw Error('屏幕数据不完整');return rgb;
}
export function screenPreview(container,{readFrame,ready=()=>true,busy=()=>false,message=()=>{},translate=s=>s}){
 const t=translate;
 container.innerHTML='<h3>'+t('实时屏幕')+'</h3><div class="screen-preview-layout"><canvas width="64" height="64" aria-label="'+t('当前主题画面')+'"></canvas><div><p>'+t('读取设备正在绘制的 64 × 64 画面，不在电脑重新模拟。')+'</p><div class="row"><button data-preview-start type="button">'+t('开始查看')+'</button><button data-preview-stop type="button" disabled>'+t('停止查看')+'</button></div><p data-preview-state role="status">'+t('默认关闭；安装期间和页面隐藏时自动暂停预览。')+'</p><small>'+t('预览速度由 USB / 网络传输决定，与屏幕的主题帧率不同；不反映实物接线、扫描闪烁和亮度。')+'</small></div></div>';
 const $=s=>container.querySelector(s),ctx=$('canvas').getContext('2d');let running=false,timer,inFlight=false,epoch=0;
 function stop(){running=false;epoch++;clearTimeout(timer);$('[data-preview-start]').disabled=false;$('[data-preview-stop]').disabled=true;}
 async function poll(){if(!running)return;if(document.hidden||container.closest('[hidden]')||(container.closest('details')&&!container.closest('details').open)||busy()){timer=setTimeout(poll,500);return;}
  if(!ready()){stop();$('[data-preview-state]').textContent=t('请连接设备并更新至 1.5.0 或更新固件。');return;}
  if(inFlight){timer=setTimeout(poll,200);return;}inFlight=true;const token=epoch,at=performance.now();
  try{const frame=await readFrame();if(!running||token!==epoch)return;ctx.putImageData(new ImageData(decodeScreenFrame(frame),64,64),0,0);$('[data-preview-state]').textContent=t('实时画面')+' · '+Math.round(performance.now()-at)+' ms';}
  catch(e){stop();$('[data-preview-state]').textContent=e.message;message(e.message,true);}
  finally{inFlight=false;if(running)timer=setTimeout(poll,Math.max(200,500-(performance.now()-at)));}
 }
 $('[data-preview-start]').onclick=()=>{if(running)return;running=true;epoch++;$('[data-preview-start]').disabled=true;$('[data-preview-stop]').disabled=false;poll();};
 $('[data-preview-stop]').onclick=()=>{stop();$('[data-preview-state]').textContent=t('预览已停止');};return {stop};
}
