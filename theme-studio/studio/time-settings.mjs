export const timeStrings={
 '时间与校准':'Time and synchronization',
 '自动网络校时':'Automatic network time',
 'NTP 时间源':'NTP time server',
 '阿里云（默认）':'Alibaba Cloud (default)',
 '自定义 / 局域网 NTP':'Custom / LAN NTP',
 '服务器主机名或 IPv4':'Server hostname or IPv4',
 '保存校时设置':'Save time settings',
 '立即网络校时':'Synchronize now',
 '开机和固件更新重启后联网即校时，成功后每隔 24 小时校准；失败自动切换备用源，后台重试，不阻塞主题。':'Synchronize after Wi-Fi connects at boot or after a firmware update, then every 24 hours. Failed requests try backup servers in the background without blocking the theme.',
 '备用源：阿里云、Cloudflare（自动去重）。':'Backup servers: Alibaba Cloud and Cloudflare (duplicates skipped).',
 '手动日期时间（按设备时区）':'Manual date and time (device timezone)',
 '设置手动时间':'Set manual time',
 '使用电脑当前时间':'Use computer time',
 '请先关闭并保存自动校时，再手动设置时间。':'Disable and save automatic synchronization before setting time manually.',
 '不带电池 RTC 的设备完全断电后无法继续计时；手动模式下重新上电需要再次设时。':'Without a battery-backed RTC, time cannot advance while power is disconnected. Set the time again after a cold boot in manual mode.',
 '当前时间':'Current time',
 '尚未设时':'Time not set',
 '未连接设备或固件尚不支持时间设置':'Connect a device with time-settings support',
 '等待 Wi-Fi 后校时':'Waiting for Wi-Fi to synchronize',
 '正在网络校时':'Synchronizing with NTP',
 '已完成网络校时':'Synchronized with NTP',
 '时间源未响应，稍后自动重试':'Time servers did not respond; retrying shortly',
 '自动校时已关闭':'Automatic synchronization is off',
 '校时设置已保存':'Time settings saved',
 '已安排网络校时':'Network synchronization scheduled',
 '手动时间已设置':'Manual time set',
 '请选择有效的日期时间':'Select a valid date and time',
 '请填写 NTP 主机名或 IPv4 地址':'Enter an NTP hostname or IPv4 address',
 '请先连接设备':'Connect a device first',
 '请先开启并保存自动校时':'Enable and save automatic synchronization first',
 '时间设置保存失败':'Failed to save time settings',
 '请输入有效的 NTP 主机名或 IPv4 地址，不要包含 http:// 或端口':'Enter an NTP hostname or IPv4 address, without http:// or a port',
 '手动时间须在 2024–2037 年之间':'Manual time must be between 2024 and 2037',
 '日期时间无效':'Invalid date or time',
 '日期时间不存在，请检查日期和时区':'Date or time does not exist; check the date and timezone',
 '设置时间失败':'Failed to set time',
 '时间操作参数无效':'Invalid time command',
};

export function timeControls(container,{request,refresh,message,busy:transportBusy=()=>false,translate=t=>t}){
 const t=translate;
 container.innerHTML=`<h2>${t('时间与校准')}</h2><p data-current></p><p data-state role="status"></p>
 <form data-settings><fieldset data-fields disabled>
 <label><input data-auto type="checkbox" style="width:auto;display:inline"> ${t('自动网络校时')}</label>
 <label>${t('NTP 时间源')}<select data-preset><option value="ntp.cloud.aliyuncs.com">${t('阿里云（默认）')}</option><option value="time.cloudflare.com">Cloudflare</option><option value="pool.ntp.org">NTP Pool</option><option value="custom">${t('自定义 / 局域网 NTP')}</option></select></label>
 <label data-custom hidden>${t('服务器主机名或 IPv4')}<input data-server maxlength="253" spellcheck="false" autocomplete="off"></label>
 <p><small>${t('开机和固件更新重启后联网即校时，成功后每隔 24 小时校准；失败自动切换备用源，后台重试，不阻塞主题。')}</small></p>
 <p><small>${t('备用源：阿里云、Cloudflare（自动去重）。')}</small></p>
 <div class="row"><button type="submit">${t('保存校时设置')}</button><button data-sync type="button" class="secondary">${t('立即网络校时')}</button></div></fieldset></form>
 <form data-manual><fieldset data-manual-fields disabled><label>${t('手动日期时间（按设备时区）')}<input data-date type="datetime-local" step="1" min="2024-01-01T00:00:00" max="2037-12-31T23:59:59" required></label>
 <div class="row"><button type="submit">${t('设置手动时间')}</button><button data-computer type="button" class="secondary">${t('使用电脑当前时间')}</button></div></fieldset></form>
 <p><small>${t('请先关闭并保存自动校时，再手动设置时间。')}</small></p><p><small>${t('不带电池 RTC 的设备完全断电后无法继续计时；手动模式下重新上电需要再次设时。')}</small></p>`;
 const $=key=>container.querySelector('[data-'+key+']');let latest,dirty=false,dateDirty=false,busy=false,timer;
 function fields(){
  $('fields').disabled=busy||!latest?.supported;
  $('manual-fields').disabled=busy||!latest?.supported||latest.autoSync;
  $('sync').disabled=!latest?.autoSync||dirty;
  $('preset').disabled=!$('auto').checked;$('server').disabled=!$('auto').checked;
  $('custom').hidden=$('preset').value!=='custom';
 }
 const update=s=>{
  latest=s;$('current').textContent=t('当前时间')+'：'+(s?.valid?s.localTime:t('尚未设时'))+(s?.timezone?' · '+s.timezone:'');
  const labels={'waiting-network':'等待 Wi-Fi 后校时',syncing:'正在网络校时',synced:'已完成网络校时',retry:'时间源未响应，稍后自动重试',manual:'自动校时已关闭'};
  $('state').textContent=s?.supported?t(labels[s.state]||s.state)+(s.state==='syncing'&&s.activeServer?' · '+s.activeServer:''):t('未连接设备或固件尚不支持时间设置');
  if(s&&!dirty){$('auto').checked=s.autoSync;$('server').value=s.server;$('preset').value=['ntp.cloud.aliyuncs.com','time.cloudflare.com','pool.ntp.org'].includes(s.server)?s.server:'custom';}
  if(s?.valid&&!dateDirty)$('date').value=s.localTime.replace(' ','T');fields();
  clearTimeout(timer);
  if(s?.supported&&['syncing','waiting-network','retry'].includes(s.state))timer=setTimeout(poll,s.state==='syncing'?1500:5000);
 };
 async function poll(){
  if(busy||transportBusy()||document.hidden||container.closest('[hidden]')){timer=setTimeout(poll,3000);return;}
  try{update(await request('time'));}catch{timer=setTimeout(poll,5000);}
 }
 $('settings').oninput=()=>{dirty=true;fields();};$('preset').onchange=()=>{dirty=true;fields();};
 $('date').oninput=()=>dateDirty=true;
 async function send(values,notice){
  if(busy)return;busy=true;fields();
  try{const s=await request('time',{method:'POST',body:new URLSearchParams(values)});dirty=false;dateDirty=false;update(s);message(t(notice));await refresh();}
  catch(e){message(t(e.message),true);}finally{busy=false;fields();}
 }
 $('settings').onsubmit=e=>{e.preventDefault();const server=($('preset').value==='custom'?$('server').value:$('preset').value).trim();if(!server){message(t('请填写 NTP 主机名或 IPv4 地址'),true);return;}send({action:'save',autoSync:$('auto').checked,server},'校时设置已保存');};
 $('sync').onclick=()=>send({action:'sync'},'已安排网络校时');
 $('manual').onsubmit=e=>{e.preventDefault();let localTime=$('date').value;if(!localTime){message(t('请选择有效的日期时间'),true);return;}if(localTime.length===16)localTime+=':00';send({action:'manual',localTime},'手动时间已设置');};
 $('computer').onclick=()=>send({action:'manual',epoch:Math.floor(Date.now()/1000)},'手动时间已设置');
 update(null);return {update};
}
