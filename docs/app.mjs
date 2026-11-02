import { language, setLanguage, t, metadata, translatePage, translateText, translations } from './i18n.mjs';
const $ = (id) => document.getElementById(id);
const supported = 'serial' in navigator && window.isSecureContext;
const boards = {
  esp32: { label: 'ESP32', version: '0.9.3', manifest: 'firmware/esp32-v0.9.3/manifest.json', tag: 'esp32-v0.9.3', note: t('适用于本项目经典 ESP32 接线与 4 MB Flash 分区。') },
  esp32s3: { label: 'ESP32-S3', version: '0.9.7', manifest: 'firmware/esp32s3-v0.9.7/manifest.json', tag: 'esp32s3-v0.9.7', note: t('仅适用于本项目 8 MB Flash + OPI PSRAM 的 S3 板；S3 N4（4 MB）不能刷此包。') },
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
  if (!device || !String(device.chip).toUpperCase().includes('S3')) return false;
  const version = String(device.version).match(/^(\d+)\.(\d+)\.(\d+)/);
  if (!version) return false;
  return Number(version[1]) * 1000000 + Number(version[2]) * 1000 + Number(version[3]) >= 9007;
}

function updateConnection() {
  const connected = Boolean(port && !closing);
  $('connection-state').textContent = closing ? t('正在断开…') : connecting ? t('正在连接…') : connected ? t('USB 已连接') : t('未连接');
  $('connect').disabled = !supported || connected || connecting || closing;
  $('disconnect').disabled = !connected || sending;
  document.querySelectorAll('[data-connected]').forEach((button) => { button.disabled = !connected || sending; });
  $('panel-fields').disabled = !connected || !panelAvailable() || sending;
  $('install-button').disabled = !supported || connected || connecting || closing;
  $('panel-support').textContent = panelAvailable()
    ? t('保存屏幕设置后设备会重启，请稍后重新读取状态。')
    : t('读取设备状态后，ESP32-S3 v0.9.7 及以上固件可使用此设置。');
}

async function disconnect() {
  if (closing) return;
  closing = true;
  updateConnection();
  const current = port;
  port = null;
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

document.querySelectorAll('input[name="board"]').forEach((radio) => {
  radio.addEventListener('change', () => {
    const board = boards[radio.value];
    $('installer').setAttribute('manifest', board.manifest);
    $('install-button').textContent = t('连接并烧录 {0}', [board.label]);
    $('release-link').href = `https://github.com/AntonVanke/HUB-75E/releases/tag/${board.tag}`;
    $('board-note').textContent = translateText(board.note);
  });
});
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
  return /^themes\/[a-zA-Z0-9_./-]+$/.test(value) && !value.split('/').includes('..') ? value : fallback;
}

function element(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined) node.textContent = text;
  return node;
}

function renderThemes() {
  const query = $('theme-search').value.trim().toLocaleLowerCase();
  const chip = $('theme-chip').value;
  const memory = $('theme-memory').value;
  const matches = themes.filter((app) => {
    const requirements = app.requirements || {};
    const text = [app.title, app.id, app.description, app.category, metadata(app, 'title'), metadata(app, 'description'), metadata(app, 'category')].join(' ').toLocaleLowerCase();
    return (!query || text.includes(query))
      && (chip === 'all' || requirements.chips?.includes(chip))
      && (memory === 'all' || (memory === 'none' ? requirements.psramBytes === 0 : Number(requirements.psramBytes) > 0));
  });
  const fragment = document.createDocumentFragment();
  for (const app of matches) {
    const requirements = app.requirements || {};
    const id = /^[a-zA-Z0-9_-]+$/.test(app.id) ? app.id : '';
    const card = element('article', 'card theme-card');
    const heading = element('div', 'card-heading');
    heading.append(element('h3', '', metadata(app, 'title')), element('span', 'version', `v${app.appVersion || '—'}`));
    card.append(heading, element('p', 'theme-description', metadata(app, 'description') || t('暂无描述')));
    const meta = element('div', 'theme-meta');
    meta.append(element('span', '', (requirements.chips || []).map((value) => value === 'esp32s3' ? 'ESP32-S3' : value.toUpperCase()).join(' / ') || t('芯片未声明')));
    meta.append(element('span', '', requirements.psramBytes === 0 ? t('无需 PSRAM') : `PSRAM ${bytes(requirements.psramBytes)}`));
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
      downloads.append(cwp, source);
    }
    const details = element('details');
    details.append(element('summary', '', t('需求与应用信息')));
    const list = element('dl', 'requirements');
    const rows = [
      [t('应用标识'), app.packageName || app.id], [t('作者'), app.author || '—'],
      [t('Lua 堆上限'), bytes(requirements.luaHeapBytes)], [t('Lua 工作内存'), bytes(requirements.luaWorkingBytes)],
      [t('图形内存'), bytes(requirements.graphicsBytes)], ['PSRAM', bytes(requirements.psramBytes)],
      [t('目标帧率'), requirements.targetFps ? `${requirements.targetFps} FPS` : t('未声明')],
      [t('应用大小'), bytes(app.bytes)], [t('实机验证'), app.verification?.status === 'verified' ? t('已标记验证') : t('尚未验证')],
    ];
    for (const [label, value] of rows) list.append(element('dt', '', label), element('dd', '', String(value || '—')));
    details.append(list);
    card.append(downloads, details);
    fragment.append(card);
  }
  $('theme-list').replaceChildren(fragment);
  $('theme-result').textContent = matches.length ? t('显示 {0} / {1} 款主题', [matches.length, themes.length]) : t('没有匹配的主题，请调整搜索或筛选条件。');
}

async function loadThemes() {
  try {
    const response = await fetch('theme-catalog.json');
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();
    themes = Array.isArray(data) ? data : data.apps;
    if (!Array.isArray(themes)) throw new Error(t('目录格式无效'));
    $('theme-count').textContent = t('{0} 款', [themes.length]);
    $('theme-tab-count').textContent = themes.length;
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
  if (device) $('status-theme').textContent = themeLabel(device.baseTheme ?? device.theme);
  if (!quiet) log(t('已读取应用：{0} 款已安装应用。', [installedApps.length]));
}

function themeLabel(index) {
  if (Number(index) === -1) return t('内置基础时钟');
  const app = installedApps.find((item) => item.index === Number(index));
  return app ? metadata(app, 'title') : t('主题 #{0}', [index]);
}

function updateStatus(data) {
  device = data;
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
  const ip = String(data.ip || '');
  const validIP = /^(?:\d{1,3}\.){3}\d{1,3}$/.test(ip) && ip.split('.').every((part) => Number(part) <= 255) && ip !== '0.0.0.0';
  $('device-link').hidden = !validIP;
  if (validIP) $('device-link').href = `http://${ip}/`;
  log(t('设备状态：{0} · v{1} · IP {2} · 亮度 {3} · 默认主题 {4} · 时区 {5}', [data.chip, data.version, validIP ? ip : t('未分配'), data.brightness, theme, data.timezone]));
  message(t('已读取设备状态。编辑后点击对应的保存按钮写入。'));
  updateConnection();
}

function receiveLine(line) {
  const marker = line.indexOf('CWJSON:');
  if (marker === -1) return;
  try {
    const data = JSON.parse(line.slice(marker + 7).trim());
    if (data && typeof data === 'object' && !Array.isArray(data)) {
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
      await current.close().catch(() => {});
      updateConnection();
      log(t('设备串口连接已结束。'));
    }
  }
}

async function send(command) {
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
    log(t('发送：{0}{1}', [command.cmd, command.cmd === 'wifi' ? t('（凭据不记录）') : '']));
    message(t('命令已发送，等待设备回应。'));
  } finally {
    writer?.releaseLock();
    activeWrite = null;
    sending = false;
    updateConnection();
  }
}

async function action(operation) {
  try { await operation(); }
  catch (error) { message(error.message, true); log(t('操作失败：{0}', [error.message])); }
}

$('connect').addEventListener('click', () => action(async () => {
  if (!supported || port || connecting || closing) return;
  connecting = true;
  updateConnection();
  let selected;
  try {
    selected = await navigator.serial.requestPort();
    await selected.open({ baudRate: 115200, dataBits: 8, stopBits: 1, parity: 'none', flowControl: 'none' });
    port = selected;
    device = null;
    installedApps = [];
    readTask = readSerial(selected);
    log(t('串口已连接，115200 baud。'));
    message(t('USB 串口已连接。点击「读取状态」和「读取应用」获取当前设置。'));
    if (!$('panel-config').hidden) updateConnection();
    else await disconnect();
  } catch (error) {
    if (error.name === 'NotFoundError') message(t('未选择串口。'));
    else throw error;
  } finally {
    connecting = false;
    updateConnection();
  }
}));
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
  $('brightness').value = settings.brightness;
  $('timezone').value = settings.timezone;
  $('rotation').value = settings.rotationMinutes;
  selectDeviceTheme(settings.theme);
  if (settings.panel) { $('panel-rgb').value = settings.panel.colorOrder; $('panel-driver').value = settings.panel.driver; }
  message(t('配置已填入草稿，尚未发送。请核对后分别保存显示设置与屏幕设置。'));
  log(t('已导入非密码配置草稿。'));
}));
function refreshLanguage() {
  translatePage();
  $('language-select').value = language();
  const board = boards[document.querySelector('input[name="board"]:checked').value];
  $('install-button').textContent = t('连接并烧录 {0}', [board.label]);
  $('board-note').textContent = translateText(board.note);
  if (themes.length) {
    $('theme-count').textContent = t('{0} 款', [themes.length]);
    renderThemes();
  }
  updateApps(installedApps, true);
  updateConnection();
}
$('language-select').addEventListener('change', (event) => setLanguage(event.target.value));
window.addEventListener('clockwise:languagechange', refreshLanguage);
document.querySelectorAll('form').forEach((form) => { form.noValidate = true; });
refreshLanguage();
