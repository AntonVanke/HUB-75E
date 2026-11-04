export function setupWizard(container,{request,message,showPanel,refresh}){
 const $=s=>container.querySelector('[data-'+s+']');
 container.innerHTML=`<h2>首次设置</h2><p data-access></p><nav class="row" aria-label="首次设置步骤"><strong data-label></strong></nav><p data-help></p><div data-slot></div><form data-misc hidden><label>屏幕亮度<input name="brightness" type="number" min="1" max="255" value="80" required></label><label>时区<input name="timezone" value="CST-8" maxlength="63" required></label><p>完成后自动初始化空白应用存储；保留已经安装的应用。</p><button type="submit">完成设置并初始化存储</button></form><div class="row"><button data-back type="button" class="secondary">上一步</button><button data-next type="button">下一步</button><button data-skip type="button" class="secondary">跳过引导，手动配置</button></div>`;
 const cards=['wifi','panel-config'].map(id=>document.getElementById(id).closest('section'));
 const anchors=cards.map(card=>{const anchor=document.createComment('setup form');card.before(anchor);return anchor;});
 const driverLabel=document.getElementById('panel-driver').closest('label'),driverAnchor=document.createComment('driver selector');driverLabel.before(driverAnchor);
 let step=1,active=false,data={},busy=false,shown=-1;
 function restore(){driverAnchor.after(driverLabel);cards.forEach((card,i)=>anchors[i].after(card));}
 function draw(){
  const layout=shown!==(active?step:0);shown=active?step:0;if(layout)restore();const tabbar=document.querySelector('.settings-tabs');tabbar.hidden=active;container.hidden=!active;
  if(!active){if(layout)showPanel(location.hash.slice(1)||'overview');return;}
  document.querySelectorAll('[role="tabpanel"]').forEach(p=>p.hidden=true);
  $('label').textContent=['1 / 3 · 连接 Wi-Fi','2 / 3 · 确认屏幕驱动','3 / 3 · 其它设置'][step-1];
  $('help').textContent=['连接设备热点后扫描并选择家庭 Wi-Fi。连接成功后再进入下一步。','观察屏幕确认图形和 R/G/B 色彩。保存驱动会重启，页面重连后继续引导。','保存亮度、时区并准备应用存储；方向、自动亮度和其它功能可随后在设置页调整。'][step-1];
  if(layout&&step<3){$('slot').append(cards[step-1]);cards[step-1].classList.remove('narrow');if(step===2)document.getElementById('panel-config').querySelector('fieldset').prepend(driverLabel);}
  $('misc').hidden=step!==3;$('next').hidden=step===3;$('back').hidden=step===1;
  $('next').disabled=busy||(step===1?!data.connected:!data.setup?.panelChosen);
  $('access').textContent='热点：'+(data.setup?.ssid||'')+' · 密码：'+(data.setup?.password||'')+' · '+(data.setup?.url||'');
 }
 async function action(name){if(busy)return;busy=true;try{await request('setup',{method:'POST',body:new URLSearchParams({action:name})});active=false;draw();await refresh();message(name==='finish'?'首次设置完成，应用存储已就绪。':'已跳过引导，可在设置页手动配置。');}catch(e){message(e.message,true);}finally{busy=false;}}
 $('next').onclick=()=>{step=Math.min(3,step+1);draw();};$('back').onclick=()=>{step=Math.max(1,step-1);draw();};$('skip').onclick=()=>action('skip');
 $('misc').onsubmit=async e=>{e.preventDefault();if(busy)return;try{const body=new URLSearchParams(new FormData(e.target));body.set('theme',String(data.baseTheme??data.theme??-1));body.set('rotationMinutes',String(data.rotationMinutes||0));await request('config',{method:'POST',body});await action('finish');}catch(e){message(e.message,true);}};
 return {update(value){const was=active;data=value;active=Boolean(value.setup?.required&&value.setup?.wizard);if(active&&!was){step=value.setup.step||1;$('misc').elements.brightness.value=value.brightness||80;$('misc').elements.timezone.value=value.timezone||'CST-8';}draw();}};
}
