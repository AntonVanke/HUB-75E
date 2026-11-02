import { language } from '../../i18n.mjs';

// ESP Web Tools 10.1.0 has no locale API. These are translations of its UI,
// not of the browser-owned Web Serial permission/port chooser.
export const vendorTranslations = {
  'No port selected': '未选择串口',
  "If you didn't select a port because you didn't see your device listed, try the following steps:": '如果没有看到你的设备而未选择串口，请尝试以下步骤：',
  'Make sure that the device is connected to this computer (the one that runs the browser that shows this website)': '确认设备连接到了当前打开此网页的电脑。',
  'Most devices have a tiny light when it is powered on. If yours has one, make sure it is on.': '多数设备通电后会亮起指示灯；如果你的设备有指示灯，请确认它已亮起。',
  'Make sure that the USB cable you use can be used for data and is not a power-only cable.': '确认 USB 线支持数据传输，而不只是充电线。',
  'If you are using a Linux flavor, make sure that your user is part of the': '如果使用 Linux，请确认当前用户属于',
  'group so it has permission to access the device.': '组，以获得设备访问权限。',
  'You may need to log out & back in or reboot to activate the new group access.': '添加用户组后，可能需要注销后重新登录或重启电脑才能生效。',
  'Make sure you have the right drivers installed. Below are the drivers for common chips used in ESP devices:': '确认已安装正确的 USB 串口驱动。常见 ESP 设备芯片的驱动如下：',
  'CP2102 drivers:': 'CP2102 驱动：', 'CH342, CH343, CH9102 drivers:': 'CH342、CH343、CH9102 驱动：', 'CH340, CH341 drivers:': 'CH340、CH341 驱动：',
  '(download via blue button with': '（点击带有', 'icon)': '图标的蓝色按钮下载）',
  'Try Again': '重试', 'Cancel': '取消', 'Close': '关闭', 'Connect': '连接', 'Error': '错误',
  'Refresh networks': '刷新网络', 'Console command': '控制台命令',
  'Connected to': '已连接至', 'Visit Device': '打开设备后台', 'Add to Home Assistant': '添加到 Home Assistant',
  'Connect to Wi-Fi': '连接 Wi-Fi', 'Change Wi-Fi': '更换 Wi-Fi', 'Logs & Console': '日志与控制台',
  'Fund Development': '支持项目开发', 'Erase User Data': '清空用户数据', 'Configure Wi-Fi': '配置 Wi-Fi',
  'Scanning for networks': '正在扫描网络', 'Trying to connect': '正在尝试连接', 'Unable to connect': '无法连接', 'Timeout': '连接超时',
  'Connect your device to the network to start using it.': '将设备连接到网络后即可开始使用。',
  'Network Name': '网络名称', 'Network': '网络', 'Password': '密码', 'Join other…': '连接其他网络…',
  'Skip': '跳过', 'Back': '返回', 'Device connected to the network!': '设备已连接到网络！', 'Continue': '继续',
  'Erase device': '整片擦除设备', 'Next': '下一步',
  'Do you want to erase the device before installing': '安装以下固件前是否整片擦除设备：',
  '? All data on the device will be lost.': '？设备上的所有数据都将丢失。',
  'Do you want to reset your device and erase all user data from your device?': '是否重置设备并清空所有用户数据？',
  'Confirm Installation': '确认安装', 'Your device is running': '设备当前运行', 'Do you want to': '是否要',
  'update to': '更新至', 'install': '安装', 'All data on the device will be erased.': '设备上的所有数据都将被擦除。',
  'Install': '安装', 'Update': '更新', 'Installing': '正在安装', 'Preparing installation': '正在准备安装',
  'Connecting': '正在连接', 'Erasing': '正在擦除', 'Wrapping up': '正在完成安装', 'This will take': '预计需要',
  'a minute': '1 分钟', '2 minutes': '2 分钟', 'Keep this page visible to prevent slow down': '请保持此页面可见，以免安装变慢',
  'Installation complete!': '安装完成！', 'Installation failed': '安装失败', 'Logs': '日志',
  'Reset Device': '重启设备', 'Download Logs': '下载日志', 'Disconnected': '设备已断开',
  'Serial port is not readable/writable. Close any other application using it and try again.': '串口无法读写。请关闭其他占用串口的应用后重试。',
  'Serial port is not ready. Close any other application using it and try again.': '串口尚未就绪。请关闭其他占用串口的应用后重试。',
  'Failed to download manifest': '无法下载固件清单',
  'Failed to initialize. Try resetting your device or holding the BOOT button while clicking INSTALL.': '无法初始化设备。请尝试重启设备，或按住 BOOT 按钮再点击「安装」。',
  'Failed to download firmware': '无法下载固件', 'Failed to fetch': '网络请求失败', 'NetworkError when attempting to fetch resource.': '网络请求失败。',
  'Terminal disconnected': '终端已断开', '[redacted]': '[已隐藏]',
};

const escape = (value) => value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
const replacements = Object.entries(vendorTranslations).sort(([a], [b]) => b.length - a.length).map(([en, zh]) => ({
  pattern: new RegExp(escape(en).replace(/\s+/g, '\\s+'), 'g'), zh,
}));

export function vendorText(value) {
  if (typeof value !== 'string') return value;
  if (language() === 'en') return Object.entries(vendorTranslations).find(([, zh]) => zh === value)?.[0] ?? value;
  if (vendorTranslations[value]) return vendorTranslations[value];
  return value
    .replace(/^Unknown error \((.*?)\)$/, '未知错误（$1）')
    .replace(/^Install (.+)$/, '安装 $1')
    .replace(/^Update (.+)$/, '更新 $1')
    .replace(/^Your (.*?) board is not supported\.$/, '不支持你的 $1 开发板。')
    .replace(/^Failed to download firmware: (.*)$/, '无法下载固件：$1')
    .replace(/^Failed to initialize\. (.*)$/, '初始化失败：$1')
    .replace(/^Failed to write to target RAM.*$/, '无法写入设备内存，请重新连接后重试。')
    .replace(/^Terminal disconnected: (.*)$/, '终端已断开：$1')
    .replace(/^Error: (.*)$/, '错误：$1');
}

// Keep a distinct, stable Lit template per locale. Only owned static template
// strings and explicit label/heading bindings are translated, never SSIDs,
// passwords, URLs, manifest names, or other device-provided values.
export function localizedHtml(html) {
  const cache = new WeakMap();
  return (strings, ...values) => {
    let templates = cache.get(strings);
    if (!templates) { templates = new Map(); cache.set(strings, templates); }
    let translated = templates.get(language());
    if (!translated) {
      translated = Array.from(strings, (text) => {
        if (language() === 'en') return text;
        text = text.replace(/\b(label|aria-label|title)="([^"]*)"/g, (_, attr, value) => `${attr}="${vendorText(value)}"`);
        return text.split(/(<[^>]*>)/g).map((part) => {
          if (part.startsWith('<') && part.endsWith('>')) return part;
          for (const { pattern, zh } of replacements) part = part.replace(pattern, () => zh);
          return part;
        }).join('');
      });
      translated.raw = translated;
      templates.set(language(), translated);
    }
    return html(translated, ...values.map((value, index) => /\.(?:label|heading)\s*=\s*$/.test(strings[index]) ? vendorText(value) : value));
  };
}

export function observeVendorLanguage(tagName) {
  window.addEventListener('clockwise:languagechange', () => {
    for (const dialog of document.querySelectorAll(tagName)) dialog.requestUpdate();
  });
}
