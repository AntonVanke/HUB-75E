export function setupWizard(container,{request,message,showPanel,refresh}){
 const $=key=>container.querySelector('[data-'+key+']');
 container.innerHTML=`<h2>首次设置</h2><p data-access></p><nav class="row" aria-label="首次设置步骤"><strong data-label></strong></nav><p data-help></p><div data-slot></div>
 <form data-misc hidden>
 <label>固定亮度<input name="brightness" type="number" min="1" max="255" value="80" required></label>
 <label>时区<input name="timezone" list="setup-zones" value="CST-8" maxlength="63" required></label>
 <datalist id="setup-zones"><option value="CST-8">中国 / 新加坡 · UTC+8</option><option value="UTC0">UTC</option><option value="JST-9">日本 / 韩国 · UTC+9</option><option value="EST5EDT,M3.2.0,M11.1.0">美国东部（夏令时）</option></datalist>
 <label><input name="autoBrightness" type="checkbox" style="width:auto;display:inline">根据环境光自动调节</label><small data-light-state></small>
 <label>手动方向<select name="manualRotation"><option>0</option><option>90</option><option>180</option><option>270</option></select></label>
 <label><input name="autoRotate" type="checkbox" style="width:auto;display:inline">根据重力自动旋转</label><small data-gravity-state></small>
 <button type="submit">保存并继续配网</button></form>
 <p data-result role="status"></p><div class="row"><button data-back type="button" class="secondary">上一步</button><button data-next type="button">下一步</button><button data-finish type="button">完成设置并初始化存储</button><button data-skip type="button" class="secondary">跳过引导，手动配置</button></div>`;
 const cards=['panel-config','wifi'].map(id=>document.getElementById(id).closest('section'));
 const anchors=cards.map(card=>{const anchor=document.createComment('setup form');card.before(anchor);return anchor;});
 const driverLabel=document.getElementById('panel-driver').closest('label');
 const driverAnchor=document.createComment('driver selector');driverLabel.before(driverAnchor);
 let step=1,active=false,data={},busy=false,shown=-1,miscDirty=false;
 function restore(){driverAnchor.after(driverLabel);cards.forEach((card,i)=>anchors[i].after(card));}
 function draw(){
  const layout=shown!==(active?step:0);shown=active?step:0;
  if(layout)restore();
  document.querySelector('.settings-tabs').hidden=active;container.hidden=!active;
  if(!active){if(layout)showPanel(location.hash.slice(1)||'overview');return;}
  document.querySelectorAll('[role="tabpanel"]').forEach(p=>p.hidden=true);
  $('label').textContent=['1 / 3 · 确认屏幕驱动','2 / 3 · 其它设置','3 / 3 · 连接 Wi-Fi'][step-1];
  $('help').textContent=[
   '确认画面和 RGB 色彩后保存屏幕设置，继续下一步。保留当前轮测驱动无需重启；更改驱动或时序将在完成配网后统一应用。',
   '设置时区、亮度和重力自动旋转，保存后继续配网。已保存的进度在刷新或断电后恢复。',
   '扫描并连接当前地点的 Wi-Fi，成功后自动完成引导并初始化空白应用存储。热点将在联网稳定 30 秒后关闭。'
  ][step-1];
  if(layout&&step!==2){const card=cards[step===1?0:1];$('slot').append(card);card.classList.remove('narrow');if(step===1)document.getElementById('panel-config').querySelector('fieldset').prepend(driverLabel);}
  $('misc').hidden=step!==2;$('next').hidden=step!==1;$('back').hidden=step===1;
  $('next').disabled=busy||!data.setup?.panelChosen;$('back').disabled=busy;$('skip').disabled=busy;
  $('finish').hidden=step!==3;$('finish').disabled=busy||!data.connected;
  $('access').textContent=(data.setup?.ssid||'')+' · '+(data.setup?.password||'')+' · '+(data.setup?.url||'');
  $('result').textContent=step===3&&data.connected?'Wi-Fi 已连接，正在完成设置…':'';
 }
 async function action(name){
  if(busy)return;
  busy=true;draw();
  try{await request('setup',{method:'POST',body:new URLSearchParams({action:name})});await refresh();}
  catch(e){message(e.message,true);}
  finally{busy=false;draw();}
 }
 $('next').onclick=()=>action('settings');
 $('back').onclick=()=>action(step===3?'settings':'panel');
 $('skip').onclick=()=>action('skip');$('finish').onclick=()=>action('finish');
 $('misc').oninput=()=>miscDirty=true;
 $('misc').onsubmit=async e=>{
  e.preventDefault();if(busy)return;
  busy=true;draw();
  try{
   const form=e.target.elements;
   await request('config',{method:'POST',body:new URLSearchParams({brightness:form.brightness.value,timezone:form.timezone.value,theme:data.baseTheme??data.theme??-1,rotationMinutes:data.rotationMinutes||0})});
   if(data.brightnessControl?.supported&&data.brightnessControl?.present){
    const light=data.brightnessControl;
    await request('brightness',{method:'POST',body:new URLSearchParams({enabled:form.autoBrightness.checked,minimum:light.minimum,maximum:light.maximum,dark:light.dark,bright:light.bright})});
   }
   if(data.features?.orientation===1)await request('orientation',{method:'POST',body:new URLSearchParams({manualRotation:form.manualRotation.value,autoRotate:form.autoRotate.checked})});
   await request('setup',{method:'POST',body:new URLSearchParams({action:'wifi'})});
   miscDirty=false;await refresh();
  }catch(err){message(err.message,true);}
  finally{busy=false;draw();}
 };
 return {update(value){
  data=value;active=Boolean(value.setup?.required&&value.setup?.wizard);step=Math.max(1,Math.min(3,Number(value.setup?.step)||1));
  const form=$('misc').elements,light=value.brightnessControl,gravity=value.orientation;
  form.autoBrightness.disabled=!light?.supported||!light?.present;form.autoRotate.disabled=!gravity?.autoRotateSupported;
  form.manualRotation.disabled=value.features?.orientation!==1;
  $('light-state').textContent=form.autoBrightness.disabled?'未检测到光敏电阻，使用固定亮度。':'已检测到光敏电阻。';
  $('gravity-state').textContent=form.autoRotate.disabled?'当前设备不支持重力自动旋转。':'可根据重力调整显示方向。';
  if(!miscDirty){form.brightness.value=value.brightness??80;form.timezone.value=value.timezone||'CST-8';form.autoBrightness.checked=!!light?.enabled;form.manualRotation.value=String(gravity?.manualRotation??270);form.autoRotate.checked=!!gravity?.autoRotate;}
  draw();
 }};
}
