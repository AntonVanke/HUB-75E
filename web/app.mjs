import {groupThemes,selectVariant} from './theme-groups.mjs';
import { language, setLanguage, t, metadata, translatePage, translateText, translations } from './i18n.mjs';
const $ = (id) => document.getElementById(id);
const supported = 'serial' in navigator && window.isSecureContext;
import {creditsHTML} from './studio/credits.mjs';
import {nativeInfo} from './studio/native-package.mjs';
let releases=null;
const boards = {
 esp32:{label:'ESP32',note:'经典 ESP32 · 4 MB Flash · 无 PSRAM'},
 esp32s3:{label:'ESP32-S3',note:'本项目 S3 板 · 至少 8 MB Flash + 8 MB OPI PSRAM；不支持 N4'}
};

let port = null;
let reader = null;
let readTask = null;
let closing = false;
let connecting = false;
let sending = false;
let activeWrite = null;
let device = null;
let installedApps = [];
let themes = [];
let appInstalling = false;
let installCancelled = false;
let installTask = null;
let discoveryTask = null;
let scanning=false;
const serialWaiters = new Set();

function message(text, error = false) {
  $('config-message').textContent = translateText(text);
  $('config-message').classList.toggle('error', error);
}

function log(text) {
  text = translateText(text);
  const entry = document.createElement(text.length > 180 ? 'details' : 'div');
  const time = document.createElement('time');
  time.textContent = new Date().toLocaleTimeString(language(), { hour12: false });
  if (entry.tagName === 'DETAILS') {
    const summary = document.createElement('summary');
    summary.append(time, document.createTextNode(`${text.slice(0, 100)}…`));
    const full = document.createElement('pre');
    full.textContent = text;
    entry.append(summary, full);
  } else {
    entry.append(time, document.createTextNode(text));
  }
  $('serial-log').append(entry);
  while ($('serial-log').children.length > 200) $('serial-log').firstElementChild.remove();
  $('serial-log').scrollTop = $('serial-log').scrollHeight;
}

function panelAvailable() {
  if (!device) return false;
  if (Number(device.runtimeApi)>=4) return true;
  if (!String(device.chip).toUpperCase().includes('S3')) return false;
  const version = String(device.version).match(/^(\d+)\.(\d+)\.(\d+)/);
  if (!version) return false;
  return Number(version[1]) * 1000000 + Number(version[2]) * 1000 + Number(version[3]) >= 9007;
}

function updateConnection() {
  const connected = Boolean(port && !closing);
  $('connection-state').textContent = closing ? t('正在断开…') : connecting ? t('正在连接…') : connected ? t('USB 已连接') : t('未连接');
  $('connect').disabled = !supported || connected || connecting || closing;
  $('disconnect').disabled = !connected || sending;
  document.querySelectorAll('[data-connected]').forEach((button) => { button.disabled = !connected || sending || appInstalling || scanning; });
  $('panel-fields').disabled = !connected || !panelAvailable() || sending || appInstalling;
  for(const option of $('panel-driver').options)option.disabled=!!device&&!/S3/i.test(device.chip)&&['7','9'].includes(option.value);
  $('orientation-fields').disabled = !connected || !device?.features?.orientation || sending || appInstalling;
  $('auto-rotate').disabled = !device?.orientation?.autoRotateSupported;
  $('cancel-install-config').hidden = !appInstalling || !installTask;
  $('cancel-install').disabled = !appInstalling || !installTask;
  document.querySelectorAll('[data-theme-action]').forEach((button) => { button.disabled = !connected || sending || appInstalling || button.dataset.incompatible === 'true' || !device?.features?.serialAppInstall; });
  $('discover-devices').disabled = !connected || sending || appInstalling || !!discoveryTask || !device?.features?.discovery;
  $('install-button').disabled = !supported || connected || connecting || closing;
  $('panel-support').textContent = panelAvailable()
    ? t('保存屏幕设置后设备会重启，请稍后重新读取状态。')
    : t('读取设备状态后，ESP32-S3 v0.9.7 及以上固件可使用此设置。');
}

async function disconnect() {
  if (closing) return;
  if (installTask) {
    installCancelled = true;
    for (const waiter of [...serialWaiters]) waiter.reject(new Error(t('安装已取消，原应用已保留。')));
    await installTask.catch(() => {});
  }
  closing = true;
  updateConnection();
  const current = port;
  port = null;
  for (const waiter of [...serialWaiters]) waiter.reject(new Error(t('设备串口连接已结束。')));
  try {
    if (activeWrite) await activeWrite.catch(() => {});
    if (reader) await reader.cancel();
    if (readTask) await readTask;
    if (current) await current.close();
    readTask = null;
    if (current) {
      log(t('已释放配置串口。'));
      message(t('串口已断开，可进行在线烧录。'));
    }
  } catch (error) {
    log(t('断开串口：{0}', [error.message]));
    message(t('未能释放串口，请关闭占用后重试；在线烧录暂不可用。'), true);
    port = current;
    if (current?.readable && !reader) readTask = readSerial(current);
  } finally {
    closing = false;
    updateConnection();
  }
}

async function selectTab(name, focus = false) {
  if (name === 'flash' && (port || reader)) await disconnect();
  document.querySelectorAll('[data-tab]').forEach((button) => {
    const selected = button.dataset.tab === name;
    button.setAttribute('aria-selected', String(selected));
    button.tabIndex = selected ? 0 : -1;
    $(`panel-${button.dataset.tab}`).hidden = !selected;
    if (selected && focus) button.focus();
  });
  history.replaceState(null, '', `#${name}`);
}

document.querySelectorAll('[data-tab]').forEach((button, index, buttons) => {
  button.addEventListener('click', () => selectTab(button.dataset.tab));
  button.addEventListener('keydown', (event) => {
    let next;
    if (event.key === 'ArrowRight') next = (index + 1) % buttons.length;
    if (event.key === 'ArrowLeft') next = (index + buttons.length - 1) % buttons.length;
    if (event.key === 'Home') next = 0;
    if (event.key === 'End') next = buttons.length - 1;
    if (next !== undefined) {
      event.preventDefault();
      selectTab(buttons[next].dataset.tab, true);
    }
  });
});
window.addEventListener('hashchange', () => {
  const name = location.hash.slice(1);
  if (['flash', 'themes', 'config'].includes(name)) selectTab(name);
});
if (['flash', 'themes', 'config'].includes(location.hash.slice(1))) selectTab(location.hash.slice(1));

function updateInstaller(){
 const key=document.querySelector('input[name="board"]:checked').value,board=boards[key],release=releases?.releases.find(r=>r.version===$('release-version').value),target=release?.targets[key];
 if(!target)return;
 const preserve=$('flash-mode').value==='update';$('installer').overrides={preserveData:preserve};$('installer').setAttribute('manifest',preserve?target.updateManifest:target.manifest);
 $('install-button').textContent=(preserve?'连接并更新 ':'连接并烧录 ')+board.label+' '+release.version;
 $('release-link').href='https://github.com/AntonVanke/HUB-75E/releases/tag/v'+release.version;
 $('board-note').textContent=board.note+'。'+(preserve?'更新保留 Wi-Fi、设置和应用；要求原设备使用本项目兼容分区，不会执行整片擦除。':'全新安装会重置 Wi-Fi 和设置。');
}
for(const radio of document.querySelectorAll('input[name="board"]'))radio.onchange=updateInstaller;
$('release-version').onchange=$('flash-mode').onchange=updateInstaller;
fetch('releases.json',{cache:'no-store'}).then(r=>{if(!r.ok)throw Error('版本目录读取失败');return r.json();}).then(data=>{releases=data;$('release-version').replaceChildren(...data.releases.map(r=>new Option(r.version,r.version)));$('release-version').value=data.latest;updateInstaller();}).catch(e=>{$('board-note').textContent=e.message;});
document.getElementById('credits').innerHTML=creditsHTML;
$('install-button').addEventListener('click', (event) => {
  if (port || reader || closing || connecting) {
    event.preventDefault();
    event.stopImmediatePropagation();
    $('flash-support').textContent = t('配置串口仍被占用，请在「设备配置」中断开后再烧录。');
  }
}, true);
$('flash-support').textContent = supported
  ? t('支持当前浏览器。点击按钮后选择串口，在安装窗口中继续。')
  : t('当前浏览器不支持 Web Serial。请使用桌面 Chrome / Edge，或下载 Release 中的离线烧录包。');
$('serial-support').textContent = supported
  ? t('本页不会主动切换 DTR / RTS 或复位设备。烧录与配置共用串口，切换至烧录时会释放连接。')
  : t('当前浏览器无法使用 USB 串口配置。请使用桌面 Chrome / Edge，或在同一局域网打开设备自己的后台。');

function bytes(value) {
  const size = Number(value);
  if (!Number.isFinite(size)) return t('未声明');
  if (size === 0) return '0';
  return size >= 1048576 ? `${(size / 1048576).toFixed(1).replace(/\.0$/, '')} MB` : `${Math.ceil(size / 1024)} KB`;
}

function relativeAsset(path, fallback) {
  const value = String(path || fallback).replace(/^\/+/, '');
  return /^(?:themes|applications|native)\/[a-zA-Z0-9_./-]+$/.test(value) && !value.split('/').includes('..') ? value : fallback;
}

function element(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined) node.textContent = text;
  return node;
}

function themeCompatibility(app) {
  if (!device) return t('连接 USB 并读取状态后可安装。');
  if (!device.features?.serialAppInstall) return t('USB 一键安装需要 v0.9.8 或更新固件。');
  const requirements = app.requirements || {};
  const chip = /S3/i.test(device.chip) ? 'esp32s3' : 'esp32';
  if (requirements.chips?.length && !requirements.chips.includes(chip)) return t('此应用不支持当前芯片。');
  if (Number(app.runtimeApi || 1) > Number(device.runtimeApi || 1)) return t('此应用需要更新的设备运行时。');
  const physicalPsram = Math.ceil(Number(device.limits?.psramBytes || 0) / 1048576) * 1048576;
  if (Number(requirements.psramBytes || 0) > physicalPsram) return t('设备 PSRAM 不满足应用要求。');
  for (const key of ['codeBytes', 'dataBytes', 'stateBytes', 'graphicsBytes']) {
    if (Number(requirements[key] || 0) > Number(device.limits?.[key] ?? Infinity)) return t('应用内存需求超出设备配额。');
  }
  if ((requirements.sensors || []).some((sensor) => !device.sensorCapabilities?.includes(sensor))) return t('设备缺少应用要求的真实传感器。');
  return '';
}

function renderThemes() {
  const query = $('theme-search').value.trim().toLocaleLowerCase();
  const chip = $('theme-chip').value;
  const memory = $('theme-memory').value;
  const matches = groupThemes(themes.filter((app) => {
    if(!app)return false;
    const requirements = app.requirements || {};
    const text = [app.title, app.id, app.description, app.category, metadata(app, 'title'), metadata(app, 'description'), metadata(app, 'category')].join(' ').toLocaleLowerCase();
    return (!query || text.includes(query))
      && (chip === 'all' || requirements.chips?.includes(chip))
      && (memory === 'all' || (memory === 'none' ? !requirements.psramBytes : Number(requirements.psramBytes) > 0));
  }));
  const fragment = document.createDocumentFragment();
  for (const variants of matches) {
    const app=selectVariant(variants,device?(/S3/i.test(device.chip)?'esp32s3':'esp32'):chip);
    if(!app)continue;
    const requirements = app.requirements || {};
    const id = /^[a-zA-Z0-9_-]+$/.test(app.id) ? app.id : '';
    const card = element('article', 'card theme-card');
    const heading = element('div', 'card-heading');
    heading.append(element('h3', '', metadata(app, 'title')), element('span', 'version', `v${app.appVersion || '—'}`));
    card.append(heading, element('p', 'theme-description', metadata(app, 'description') || t('暂无描述')));
    const meta = element('div', 'theme-meta');
    meta.append(element('span', '', [...new Set(variants.flatMap(v=>v.requirements?.chips||[]))].map((value) => value === 'esp32s3' ? 'ESP32-S3' : value.toUpperCase()).join(' / ') || t('芯片未声明')));
    meta.append(element('span', '', !requirements.psramBytes ? t('无需 PSRAM') : `PSRAM ${bytes(requirements.psramBytes)}`));
    if (app.category) meta.append(element('span', '', metadata(app, 'category')));
    card.append(meta);
    const downloads = element('div', 'theme-downloads');
    if (id) {
      const cwp = element('a', 'button secondary', t('↓ CWP 应用'));
      cwp.href = relativeAsset(app.path || app.file, `themes/apps/${id}.cwp`);
      cwp.download = `${id}.cwp`;
      const source = element('a', 'button secondary', t('↓ 源码 ZIP'));
      source.href = relativeAsset(app.archivePath, `themes/apps/${id}.zip`);
      source.download = `${id}.zip`;
      for(const variant of variants){const link=element('a','button secondary',`↓ ${variant.requirements?.chips?.includes('esp32s3')?'ESP32-S3':'ESP32'} · CWP`);link.href=relativeAsset(variant.path||variant.file,`themes/apps/${variant.id}.cwp`);link.download=`${variant.id}.cwp`;downloads.append(link);}if(app.archivePath)downloads.append(source);
      const installed = installedApps.find((item) => item.packageName === app.packageName);
      const action = element('button', 'button secondary', t(installed ? '卸载' : '安装到 USB 设备'));
      action.type = 'button';
      action.dataset.themeAction = app.packageName || '';
      const reason = installed ? '' : themeCompatibility(app);
      action.dataset.incompatible = String(Boolean(reason));
      action.disabled = !port || appInstalling || Boolean(reason) || !device?.features?.serialAppInstall;
      action.addEventListener('click', () => installed ? manageApplication(installed.packageName, true) : startThemeInstall(app));
      downloads.append(action);
      if (reason) card.append(element('p', 'theme-device-note', reason));
    }
    const details = element('details');
    details.append(element('summary', '', t('需求与应用信息')));
    const list = element('dl', 'requirements');
    const rows = [
      [t('应用标识'), app.packageName || app.id], [t('作者'), app.author || '—'],
      ['代码 / 数据', bytes(requirements.codeBytes)+' / '+bytes(requirements.dataBytes)], ['状态内存', bytes(requirements.stateBytes)],
      [t('图形内存'), bytes(requirements.graphicsBytes)], ['PSRAM', bytes(requirements.psramBytes)],
      [t('目标帧率'), requirements.targetFps ? `${requirements.targetFps} FPS` : t('未声明')],
      [t('应用大小'), bytes(app.bytes)], [t('实机验证'), app.verification?.status === 'verified' ? t('已标记验证') : t('尚未验证')],
      [t('传感器要求'), (requirements.sensors || []).map((sensor) => t({ temperature: '温度', humidity: '湿度', imu: 'IMU' }[sensor] || sensor)).join(' / ') || t('无')],
    ];
    for (const [label, value] of rows) list.append(element('dt', '', label), element('dd', '', String(value || '—')));
    details.append(list);
    card.append(downloads, details);
    fragment.append(card);
  }
  const visibleCount=fragment.childElementCount;
  $('theme-list').replaceChildren(fragment);
  $('theme-result').textContent = visibleCount ? t('显示 {0} / {1} 款主题', [visibleCount, groupThemes(themes).length]) : t('没有匹配的主题，请调整搜索或筛选条件。');
}

async function loadThemes() {
  try {
    const version=$('app-release')?.value;const response = await fetch(version?'applications/'+version+'/catalog.json':'theme-catalog.json');
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();
    themes = Array.isArray(data) ? data : data.apps;
    if (!Array.isArray(themes)) throw new Error(t('目录格式无效'));
    $('theme-count').textContent = t('{0} 款', [groupThemes(themes).length]);
    $('theme-tab-count').textContent = groupThemes(themes).length;
    renderThemes();
  } catch (error) {
    $('theme-result').textContent = t('主题目录暂时无法读取：{0}。可前往 GitHub 查看应用文件。', [error.message]);
  }
}
['theme-search', 'theme-chip', 'theme-memory'].forEach((id) => $(id).addEventListener('input', renderThemes));
loadThemes();

function selectDeviceTheme(value) {
  const number = Number(value);
  if (!Number.isInteger(number) || number < -1 || number > 63) return;
  if (![...$('device-theme').options].some((option) => option.value === String(number))) {
    $('device-theme').append(new Option(t('当前主题（读取应用后显示名称）'), number));
  }
  $('device-theme').value = String(number);
}

function updateApps(apps, quiet = false) {
  installedApps = apps.filter((app) => app.installed === true && Number.isInteger(app.index) && app.index >= 0 && app.index < 64);
  const current = $('device-theme').value;
  $('device-theme').replaceChildren(new Option(t('内置基础时钟'), '-1'));
  for (const app of installedApps) $('device-theme').append(new Option(metadata(app, 'title'), app.index));
  selectDeviceTheme(current);
  const managed = $('managed-app').value;
  $('managed-app').replaceChildren(new Option(t('请选择已安装应用'), ''), ...installedApps.map((app) => new Option(metadata(app, 'title'), app.packageName || '')));
  if (installedApps.some((app) => app.packageName === managed)) $('managed-app').value = managed;
  if (device) $('status-theme').textContent = themeLabel(device.baseTheme ?? device.theme);
  if (!quiet) log(t('已读取应用：{0} 款已安装应用。', [installedApps.length]));
  if (themes.length) renderThemes();
}

function themeLabel(index) {
  if (Number(index) === -1) return t('内置基础时钟');
  const app = installedApps.find((item) => item.index === Number(index));
  return app ? metadata(app, 'title') : t('主题 #{0}', [index]);
}

function updateStatus(data) {
  device = data;
  $('serial-setup').hidden=!data.setup?.required||!data.setup?.wizard;
  $('finish-setup').disabled=!data.connected||!data.setup?.panelChosen;
  for (const key of ['chip', 'version', 'ip', 'brightness', 'timezone']) $('status-' + key).textContent = String(data[key] ?? '—');
  const theme = data.baseTheme ?? data.theme;
  $('status-theme').textContent = themeLabel(theme);
  if (Number.isInteger(data.brightness)) $('brightness').value = data.brightness;
  if (typeof data.timezone === 'string') $('timezone').value = data.timezone;
  if (Number.isInteger(data.rotationMinutes)) $('rotation').value = data.rotationMinutes;
  selectDeviceTheme(theme);
  if (data.panel && ['RGB', 'RBG', 'GRB', 'GBR', 'BRG', 'BGR'].includes(data.panel.colorOrder)) $('panel-rgb').value = data.panel.colorOrder;
  if (Number.isInteger(data.panel?.driver)) $('panel-driver').value = data.panel.driver;
  if (typeof data.ssid === 'string') $('wifi-ssid').value = data.ssid;
  if (data.orientation) {
    $('manual-rotation').value = String(data.orientation.manualRotation ?? 270);
    $('auto-rotate').checked = Boolean(data.orientation.autoRotate);
    $('orientation-state').textContent = t('当前方向：{0}°；{1}', [data.orientation.rotationDegrees ?? data.orientation.manualRotation ?? 270, data.orientation.autoRotateSupported ? data.orientation.imuValid ? t('IMU 数据有效') : t('IMU 数据暂不可用，使用手动角度') : t('此设备不支持自动重力方向')]);
  }
  const ip = String(data.ip || '');
  const validIP = /^(?:\d{1,3}\.){3}\d{1,3}$/.test(ip) && ip.split('.').every((part) => Number(part) <= 255) && ip !== '0.0.0.0';
  $('device-link').hidden = !validIP;
  if (validIP) $('device-link').href = `http://${ip}/`;
  log(t('设备状态：{0} · v{1} · IP {2} · 亮度 {3} · 默认主题 {4} · 时区 {5}', [data.chip, data.version, validIP ? ip : t('未分配'), data.brightness, theme, data.timezone]));
  message(t('已读取设备状态。编辑后点击对应的保存按钮写入。'));
  updateConnection();
  if (themes.length) renderThemes();
}

function receiveLine(line) {
  const marker = line.indexOf('CWJSON:');
  if (marker === -1) return;
  try {
    const data = JSON.parse(line.slice(marker + 7).trim());
    for (const waiter of [...serialWaiters]) waiter.receive(data);
    if (data && typeof data === 'object' && !Array.isArray(data)) {
      if (data.upload) return;
      if (data.discovery) { renderDiscovery(data); return; }
      if (data.chip && data.version && 'brightness' in data) updateStatus(data);
      else if (Array.isArray(data.apps)) updateApps(data.apps);
      else if (typeof data.ok === 'boolean') {
        // Wi-Fi and AP responses can contain secrets. Only the firmware's short result text is displayed.
        const result = String(data.ok ? data.message || t('设备已确认操作。') : data.error || t('设备拒绝了操作。'));
        let safe = /password|wifiPass|passwd|密码\s*[:：=]/i.test(result) ? t('设备已返回操作结果，敏感内容已省略。') : translateText(result);
        if (language() === 'en' && /[\u4e00-\u9fff]/.test(safe)) {
          log(t('固件原始响应：{0}', [safe]));
          safe = t('设备返回了未翻译的固件响应。');
        }
        message(safe, !data.ok);
        log(t('{0}：{1}', [data.ok ? t('设备确认') : t('设备错误'), safe]));
      }
    }
  } catch {
    log(t('收到无法解析的 CWJSON 响应，已省略原始内容。'));
  }
}

async function readSerial(current) {
  const decoder = new TextDecoder();
  let buffer = '';
  try {
    reader = current.readable.getReader();
    while (port === current) {
      const { value, done } = await reader.read();
      if (done) break;
      buffer += decoder.decode(value, { stream: true });
      let end;
      while ((end = buffer.indexOf('\n')) !== -1) {
        receiveLine(buffer.slice(0, end).replace(/\r$/, ''));
        buffer = buffer.slice(end + 1);
      }
      if (buffer.length > 262144) { buffer = ''; log(t('设备单行数据过长，已丢弃。')); }
    }
  } catch (error) {
    if (!closing) { message(t('串口读取中断：{0}', [error.message]), true); log(t('串口读取中断。')); }
  } finally {
    reader?.releaseLock();
    reader = null;
    if (!closing && port === current) {
      port = null;
      for (const waiter of [...serialWaiters]) waiter.reject(new Error(t('设备串口连接已结束。')));
      await current.close().catch(() => {});
      updateConnection();
      log(t('设备串口连接已结束。'));
    }
  }
}

async function send(command, quiet = false) {
  if (!port || !port.writable || sending) throw new Error(t('请先连接 USB 串口，并等待当前操作完成。'));
  const json = JSON.stringify(command);
  const data = new TextEncoder().encode(json + '\n');
  if (data.length > 511) throw new Error(t('配置内容过长，设备每条命令最多接收 511 字节。'));
  sending = true;
  updateConnection();
  let writer;
  try {
    writer = port.writable.getWriter();
    activeWrite = writer.write(data);
    await activeWrite;
    if (!quiet) {
      log(t('发送：{0}{1}', [command.cmd, command.cmd === 'wifi' ? t('（凭据不记录）') : '']));
      message(t('命令已发送，等待设备回应。'));
    }
  } finally {
    writer?.releaseLock();
    activeWrite = null;
    sending = false;
    updateConnection();
  }
}

function requestSerial(command, matches, timeout = 15000) {
  return new Promise((resolve, reject) => {
    let timer;
    const finish = (callback, value) => { clearTimeout(timer); serialWaiters.delete(waiter); callback(value); };
    const waiter = {
      reject: (error) => finish(reject, error),
      receive(data) {
        if (!data || typeof data !== 'object') return;
        if (data.ok === false || data.error) return finish(reject, new Error(translateText(data.error || data.result?.error || t('设备拒绝了操作。'))));
        if (matches(data)) finish(resolve, data);
      },
    };
    serialWaiters.add(waiter);
    timer = setTimeout(() => waiter.reject(new Error(t('等待设备响应超时，请重新读取状态。'))), timeout);
    send(command, true).catch(waiter.reject);
  });
}

async function fetchBoundedPackage(url, maximum) {
  const response = await fetch(url, { signal: AbortSignal.timeout(30000) });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  if (Number(response.headers.get('content-length') || 0) > maximum) throw new Error(t('应用包超出设备容量或安装空间不足'));
  const stream = response.body?.getReader();
  if (!stream) throw new Error(t('浏览器无法流式读取应用包。'));
  const chunks = []; let size = 0;
  try {
    for (;;) {
      const { value, done } = await stream.read();
      if (done) break;
      size += value.length;
      if (size > maximum) throw new Error(t('应用包超出设备容量或安装空间不足'));
      if (installCancelled) throw new Error(t('安装已取消，原应用已保留。'));
      chunks.push(value);
    }
  } finally { await stream.cancel().catch(() => {}); stream.releaseLock(); }
  if (size < 24) throw new Error(t('应用包内容不完整'));
  const bytes = new Uint8Array(size); let offset = 0;
  for (const chunk of chunks) { bytes.set(chunk, offset); offset += chunk.length; }
  return bytes;
}

async function installTheme(app) {
  const reason = themeCompatibility(app);
  if (!port || reason) throw new Error(reason || t('请先连接 USB 串口，并等待当前操作完成。'));
  if (appInstalling) return;
  appInstalling = true; installCancelled = false; updateConnection();
  $('theme-install-state').hidden = false;
  $('theme-install-progress').value = 0;
  $('theme-install-message').textContent = t('正在下载：{0}', [metadata(app, 'title')]);
  log(t('开始安装：{0}', [metadata(app, 'title')]));
  let started = false;
  try {
    const bytes = await fetchBoundedPackage(relativeAsset(app.path || app.file, `themes/apps/${app.id}.cwp`), Number(device.limits?.appBytes || 1048576));
    const info=nativeInfo(bytes);if(device.packageVersion<3)throw Error('请先更新到原生固件 1.0.1');if(!info.requirements.chips.includes(/S3/i.test(device.chip)?'esp32s3':'esp32'))throw Error('二进制芯片不匹配');
    const header = new DataView(bytes.buffer), metadataOffset = 24 + header.getUint32(12, true) * 40, metadataLength = header.getUint32(16, true);
    if (metadataLength > 2048 || metadataOffset + metadataLength > bytes.length) throw new Error(t('应用包内容不完整'));
    const manifest = JSON.parse(new TextDecoder().decode(bytes.subarray(metadataOffset, metadataOffset + metadataLength)));
    if (manifest.packageName !== app.packageName) throw new Error(t('下载应用包与请求的包名不一致'));
    const hash = [...new Uint8Array(await crypto.subtle.digest('SHA-256', bytes))].map((value) => value.toString(16).padStart(2, '0')).join('');
    if(app.sha256&&hash!==app.sha256)throw Error('应用 SHA-256 校验失败');
    if (installCancelled) throw new Error(t('安装已取消，原应用已保留。'));
    await requestSerial({ cmd: 'app.install.begin', packageName: app.packageName, bytes: bytes.length, sha256: hash }, (data) => data.upload && data.phase === 'begin');
    started = true;
    for (let offset = 0; offset < bytes.length; offset += 300) {
      if (installCancelled) throw new Error(t('安装已取消，原应用已保留。'));
      const chunk = bytes.subarray(offset, offset + 300);
      const data = btoa(String.fromCharCode(...chunk));
      await requestSerial({ cmd: 'app.install.chunk', offset, data }, (result) => result.upload && result.phase === 'chunk' && result.received === offset + chunk.length);
      const percentage = Math.floor((offset + chunk.length) * 100 / bytes.length);
      $('theme-install-progress').value = percentage;
      $('theme-install-message').textContent = t('正在通过 USB 安装 {0}：{1}%', [metadata(app, 'title'), percentage]);
    }
    const result = await requestSerial({ cmd: 'app.install.finish' }, (data) => data.upload && data.phase === 'finish', 45000);
    started = false;
    message(translateText(result.result?.message || t('应用已安装。')));
    $('theme-install-message').textContent = t('已安装：{0}', [metadata(app, 'title')]);
    log(t('已安装：{0}', [metadata(app, 'title')]));
    await requestSerial({ cmd: 'apps' }, (data) => Array.isArray(data.apps));
    await requestSerial({ cmd: 'status' }, (data) => Boolean(data.chip && data.version));
  } catch (error) {
    if (started && port) await requestSerial({ cmd: 'app.install.abort' }, (data) => data.upload && data.phase === 'abort', 3000).catch(() => {});
    $('theme-install-message').textContent = translateText(error.message);
    throw error;
  } finally { appInstalling = false; updateConnection(); }
}

function startThemeInstall(app) {
  if (appInstalling || installTask) return;
  action(async () => {
    installTask = installTheme(app);
    updateConnection();
    try { await installTask; } finally { installTask = null; }
  });
}

async function manageApplication(packageName, uninstall = false) {
  return action(async () => {
    if (!packageName || appInstalling) throw new Error(t('请选择已安装应用'));
    appInstalling = true; updateConnection();
    try {
      await requestSerial({ cmd: uninstall ? 'app.uninstall' : 'app.run', packageName }, (data) => typeof data.ok === 'boolean' && !data.upload);
      await requestSerial({ cmd: 'apps' }, (data) => Array.isArray(data.apps));
      await requestSerial({ cmd: 'status' }, (data) => Boolean(data.chip && data.version));
    } finally { appInstalling = false; updateConnection(); }
  });
}

function renderDiscovery(data) {
  $('discovery-state').textContent = data.scanning ? t('正在发现同网设备…') : t('发现 {0} 台项目设备。', [(data.devices || []).length]);
  if (data.truncated) $('discovery-state').textContent += ' ' + t('结果达到设备数量上限，列表可能不完整。');
  const rows = [];
  for (const device of data.devices || []) {
    const ip = String(device.ip || '');
    if (!/^(?:\d{1,3}\.){3}\d{1,3}$/.test(ip) || !ip.split('.').every((part) => Number(part) <= 255)) continue;
    const link = element('a', '', `${ip} ↗`);
    link.href = `http://${ip}/`; link.target = '_blank'; link.rel = 'noopener noreferrer';
    link.append(element('small', '', `${device.chip || 'ESP32'} · v${device.version || '—'} · ${device.id || ''}`));
    rows.push(link);
  }
  $('discovered-devices').replaceChildren(...rows);
}

$('manage-run').addEventListener('click', () => manageApplication($('managed-app').value));
$('manage-uninstall').addEventListener('click', () => manageApplication($('managed-app').value, true));
for (const id of ['cancel-install', 'cancel-install-config']) $(id).addEventListener('click', () => {
  installCancelled = true;
  for (const waiter of [...serialWaiters]) waiter.reject(new Error(t('安装已取消，原应用已保留。')));
});
$('discover-devices').addEventListener('click', () => {
  if (discoveryTask) return;
  action(async () => {
    discoveryTask = (async () => {
      try {
        await requestSerial({ cmd: 'discover', action: 'start' }, (data) => Boolean(data.discovery));
        const deadline = Date.now() + 6500;
        while (port && Date.now() < deadline) {
          await new Promise((resolve) => setTimeout(resolve, 1000));
          const data = await requestSerial({ cmd: 'discover' }, (data) => Boolean(data.discovery));
          if (!data.scanning) break;
        }
      } finally { discoveryTask = null; updateConnection(); }
    })();
    updateConnection(); await discoveryTask;
  });
});
$('orientation-form').addEventListener('submit', (event) => {
  event.preventDefault();
  action(async () => {
    await requestSerial({ cmd: 'orientation', manualRotation: Number($('manual-rotation').value), autoRotate: $('auto-rotate').checked }, (data) => typeof data.ok === 'boolean');
    await requestSerial({ cmd: 'status' }, (data) => Boolean(data.chip && data.version));
  });
});

async function action(operation) {
  try { await operation(); }
  catch (error) { message(error.message, true); log(t('操作失败：{0}', [error.message])); }
}

async function connectSerial(preselected) {
  if (!supported || port || connecting || closing) return;
  connecting = true;
  updateConnection();
  let selected;
  try {
    selected = preselected || await navigator.serial.requestPort();
    await selected.open({ baudRate: 115200, dataBits: 8, stopBits: 1, parity: 'none', flowControl: 'none' });
    port = selected;
    device = null;
    installedApps = [];
    readTask = readSerial(selected);
    log(t('串口已连接，115200 baud。'));
    await requestSerial({cmd:'status'},d=>Boolean(d.chip&&d.version),15000);
    await requestSerial({cmd:'apps'},d=>Array.isArray(d.apps));
    message(t('USB 已连接，可搜索 Wi-Fi 并配置屏幕驱动。'));
    if (!$('panel-config').hidden) updateConnection();
    else await disconnect();
  } catch (error) {
    if (error.name === 'NotFoundError') message(t('未选择串口。'));
    else throw error;
  } finally {
    connecting = false;
    updateConnection();
  }
}
$('connect').addEventListener('click',()=>action(()=>connectSerial()));
window.addEventListener('hub75-configure',e=>action(async()=>{await selectTab('config');await connectSerial(e.detail.port);}));
$('disconnect').addEventListener('click', () => action(disconnect));
$('read-status').addEventListener('click', () => action(() => send({ cmd: 'status' })));
$('read-apps').addEventListener('click', () => action(() => send({ cmd: 'apps' })));
$('clear-log').addEventListener('click', () => $('serial-log').replaceChildren());

function settingsDraft() {
  const brightness = Number($('brightness').value);
  const theme = Number($('device-theme').value);
  const rotationMinutes = Number($('rotation').value);
  const timezone = $('timezone').value.trim();
  if (!Number.isInteger(brightness) || brightness < 1 || brightness > 255) throw new Error(t('亮度须为 1–255 的整数。'));
  if (!Number.isInteger(theme) || theme < -1 || theme > 63) throw new Error(t('请选择有效的默认主题。'));
  if (!Number.isInteger(rotationMinutes) || rotationMinutes < 0 || rotationMinutes > 1440) throw new Error(t('轮换间隔须为 0–1440 的整数。'));
  if (!timezone || timezone.length > 63 || !/^[A-Za-z0-9+\-,:.<>/]+$/.test(timezone)) throw new Error(t('请输入有效的 POSIX 时区，例如 CST-8。'));
  return { brightness, theme, timezone, rotationMinutes };
}

$('settings-form').addEventListener('submit', (event) => {
  event.preventDefault();
  action(() => send({ cmd: 'config', ...settingsDraft() }));
});
$('wifi-form').addEventListener('submit', (event) => {
  event.preventDefault();
  action(async () => {
    const ssid = $('wifi-ssid').value;
    const password = $('wifi-password').value;
    const ssidBytes = new TextEncoder().encode(ssid).length;
    if (ssidBytes < 1 || ssidBytes > 32 || /[\x00-\x1f]/.test(ssid)) throw new Error(t('网络名称的 UTF-8 长度须为 1–32 字节，且不含控制字符；一个中文字符通常占 3 字节。'));
    if (password.length > 63 || (password.length > 0 && password.length < 8)) throw new Error(t('Wi-Fi 密码须为 8–63 个字符，开放网络请留空。'));
    try { await send({ cmd: 'wifi', ssid, password }); }
    finally { $('wifi-password').value = ''; }
  });
});
$('panel-form').addEventListener('submit', (event) => {
  event.preventDefault();
  action(async () => {
    if (!panelAvailable()) throw new Error(t('此设置需要 ESP32-S3 v0.9.7 或以上固件，请先读取状态。'));
    await send({ cmd: 'panel', colorOrder: $('panel-rgb').value, driver: Number($('panel-driver').value) });
  });
});
$('export-settings').addEventListener('click', () => action(async () => {
  const settings = settingsDraft();
  if (panelAvailable()) settings.panel = { colorOrder: $('panel-rgb').value, driver: Number($('panel-driver').value) };
  if (device?.features?.orientation) settings.orientation = { manualRotation: Number($('manual-rotation').value), autoRotate: $('auto-rotate').checked };
  const file = new Blob([JSON.stringify({ format: 'clockwise-settings', version: 1, settings }, null, 2) + '\n'], { type: 'application/json' });
  const url = URL.createObjectURL(file);
  const link = document.createElement('a');
  link.href = url;
  link.download = 'clockwise-settings.json';
  link.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
  message(t('已导出非密码配置。'));
}));
$('import-settings').addEventListener('click', () => $('settings-file').click());
$('settings-file').addEventListener('change', () => action(async () => {
  const file = $('settings-file').files[0];
  $('settings-file').value = '';
  if (!file) return;
  if (file.size > 65536) throw new Error(t('配置文件过大，请选择导出的配置 JSON。'));
  const data = JSON.parse(await file.text());
  const settings = data.settings || data;
  if (!settings || typeof settings !== 'object' || Array.isArray(settings)) throw new Error(t('配置文件格式无效。'));
  if (!Number.isInteger(settings.brightness) || settings.brightness < 1 || settings.brightness > 255
    || !Number.isInteger(settings.theme) || settings.theme < -1 || settings.theme > 63
    || !Number.isInteger(settings.rotationMinutes) || settings.rotationMinutes < 0 || settings.rotationMinutes > 1440
    || typeof settings.timezone !== 'string' || !/^[A-Za-z0-9+\-,:.<>/]{1,63}$/.test(settings.timezone)) throw new Error(t('配置字段无效，未导入任何设置。'));
  if (settings.panel && (!['RGB', 'RBG', 'GRB', 'GBR', 'BRG', 'BGR'].includes(settings.panel.colorOrder)
    || !Number.isInteger(settings.panel.driver) || settings.panel.driver < 0 || settings.panel.driver > 9)) throw new Error(t('屏幕配置无效，未导入任何设置。'));
  if (settings.orientation && (![0, 90, 180, 270].includes(settings.orientation.manualRotation) || typeof settings.orientation.autoRotate !== 'boolean')) throw new Error(t('屏幕配置无效，未导入任何设置。'));
  $('brightness').value = settings.brightness;
  $('timezone').value = settings.timezone;
  $('rotation').value = settings.rotationMinutes;
  selectDeviceTheme(settings.theme);
  if (settings.panel) { $('panel-rgb').value = settings.panel.colorOrder; $('panel-driver').value = settings.panel.driver; }
  if (settings.orientation) { $('manual-rotation').value = settings.orientation.manualRotation; $('auto-rotate').checked = settings.orientation.autoRotate; }
  message(t('配置已填入草稿，尚未发送。请核对后分别保存对应设置。'));
  log(t('已导入非密码配置草稿。'));
}));
function refreshLanguage() {
  translatePage();
  $('language-select').value = language();
  const board = boards[document.querySelector('input[name="board"]:checked').value];
  $('install-button').textContent = t('连接并烧录 {0}', [board.label]);
  $('board-note').textContent = translateText(board.note);
  if (themes.length) {
    $('theme-count').textContent = t('{0} 款', [groupThemes(themes).length]);
    renderThemes();
  }
  updateApps(installedApps, true);
  updateConnection();
}
$('language-select').addEventListener('change', (event) => setLanguage(event.target.value));
window.addEventListener('clockwise:languagechange', refreshLanguage);
document.querySelectorAll('form').forEach((form) => { form.noValidate = true; });
refreshLanguage();

fetch('releases.json',{cache:'no-store'}).then(r=>r.json()).then(data=>{$('app-release').replaceChildren(...data.releases.map(r=>new Option(r.version,r.version)));$('app-release').value=data.latest;$('app-release').onchange=loadThemes;}).catch(()=>{});

$('wifi-networks').onchange=()=>{if($('wifi-networks').value)$('wifi-ssid').value=$('wifi-networks').value;};

$('wifi-scan').onclick=()=>action(async()=>{
 if(scanning)return;if(!device?.features?.wifiScan)throw Error(t('请更新至 1.2.0 或更新固件以使用 USB Wi-Fi 扫描。'));
 scanning=true;$('wifi-scan').disabled=true;$('wifi-scan-state').textContent=t('正在搜索…');
 try{const until=Date.now()+22000;let result;
  do{result=await requestSerial({cmd:'wifi.scan'},d=>d.state==='scanning'||Array.isArray(d.networks));if(Array.isArray(result.networks))break;await new Promise(r=>setTimeout(r,400));}while(Date.now()<until);
  if(!Array.isArray(result.networks))throw Error(t('搜索超时，请重试'));
  const unique=new Map();for(const n of result.networks.sort((a,b)=>b.rssi-a.rssi))if(n.ssid&&!unique.has(n.ssid))unique.set(n.ssid,n);
  $('wifi-networks').replaceChildren(new Option(t('请选择网络'),''),...Array.from(unique.values(),n=>new Option(n.ssid+' · '+n.rssi+' dBm'+(n.open?'':' 🔒'),n.ssid)));
  $('wifi-scan-state').textContent=unique.size+' '+t('个网络');
 }catch(e){$('wifi-scan-state').textContent=t(e.message);throw e;}finally{scanning=false;updateConnection();}
});

for(const operation of ['finish','skip'])$(operation+'-setup').onclick=()=>action(async()=>{await requestSerial({cmd:'setup',action:operation},data=>typeof data.ok==='boolean',30000);await requestSerial({cmd:'status'},data=>Boolean(data.chip&&data.version));});
