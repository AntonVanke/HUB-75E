export function themePrompt({chip='esp32s3',id='my-clock',idea='',fps=60,sensors=[],english=false}={}){
 const env=chip==='esp32'?'esp32-64x64':'esp32s3-hub75';
 const request=english?'Create the native theme described below. Read the project and SDK before implementing.':'请实现下述 HUB-75E 原生二进制主题，先获取项目并阅读 SDK，再编写代码。';
 return `${request}

项目 / Repository: https://github.com/AntonVanke/HUB-75E
源码下载 / Source ZIP: https://github.com/AntonVanke/HUB-75E/archive/refs/heads/main.zip
独立 SDK / SDK ZIP: https://antonvanke.github.io/HUB-75E/native-sdk.zip
开发文档 / Documentation: https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/native/README.md
ABI: https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/native/include/cw_theme.h
主题示例 / Examples: https://github.com/AntonVanke/HUB-75E/tree/main/theme-studio/native/themes
安装页面 / USB installer: https://antonvanke.github.io/HUB-75E/studio/index.html

${english?'My idea':'我的主题想法'}:
${idea.trim()||(english?'Create a legible animated clock.':'制作清晰易读的动态时钟。')}

${english?'Target':'目标'}: ${chip==='esp32'?'ESP32, 4 MB Flash, no PSRAM':'ESP32-S3, 8+ MB Flash, 8 MB OPI PSRAM'}, 64 × 64 RGB565.
ID: ${id}; packageName: local.clockwise.${id}; targetFps: ${Math.min(chip==='esp32'?60:240,Math.max(1,Number(fps)||60))}.
${english?'Required sensors':'所需传感器'}: ${sensors.join(', ')||'none'}.

${english?`1. Locate an existing HUB-75E checkout, or git clone https://github.com/AntonVanke/HUB-75E.git into a new directory. Preserve unrelated edits. Run commands from its root. Use current VERSION and ABI files as authoritative.
2. Read theme-studio/native/README.md, theme-studio/native/include/cw_theme.h, build.py and the closest example. Create theme-studio/native/themes/${id}/theme.cpp and theme.json. Resources must ship inside CWP3, with no absolute filesystem paths.
3. Use C/C++ native machine code, CwTheme ABI 1 and runtime API 5. Implement start/update/draw/button/stop and cw_theme(). Use only CwApi imports. No Lua/JS VM, Arduino dependencies, direct GPIO, threads or network calls.
4. Package metadata includes id, unique packageName, title, appVersion, description, author and requirements. Declare real stateBytes >= sizeof(State), graphicsBytes including 8192-byte canvas, targetFps and actual sensors. Do not guess sensor data keys: inspect runtime/example implementation. Handle unavailable sensors and orientation changes.
5. Use dtMs and bounded fixed-step physics. No frame-time allocation or blocking I/O. Stream large assets with api->read and checked bounds. Ensure a safe stop/restart lifecycle. Chip-specific CWP packages are not interchangeable.
6. Check Python and PlatformIO/toolchain availability; install dependencies specified by theme-studio/embedded/demo/platformio.ini for ${env} if needed. Do not upload firmware.
7. Build from repository root: python theme-studio/native/build.py ${id} --chip ${chip}
8. Deliver theme-studio/native/dist/${id}-${chip}.cwp, complete source/resources, host preview and results actually tested. If hardware is unavailable, label FPS as a target, never measured. Explain connecting USB in the workshop and importing the CWP.
9. Do not change Wi-Fi, panel drivers or unrelated files; do not flash or publish unless explicitly requested.`:`1. 优先寻找已有 HUB-75E 工作区；没有则在新目录 git clone https://github.com/AntonVanke/HUB-75E.git。保留已有修改。以下命令均从仓库根目录执行，以当前 VERSION 和 ABI 文件为准。
2. 阅读 theme-studio/native/README.md、theme-studio/native/include/cw_theme.h、build.py 及最接近的示例。在 theme-studio/native/themes/${id}/ 创建 theme.cpp、theme.json 和素材。资源必须随 CWP3 打包，不依赖电脑绝对路径。
3. 使用 C/C++ 原生机器码、CwTheme ABI 1、运行时 API 5，实现 start/update/draw/button/stop 与 cw_theme()。只通过 CwApi 绘图和读取数据，不使用 Lua/JS VM、Arduino 库、GPIO、线程或网络。
4. 可调参数在 theme.json.parameters 声明 min/max/step/default，使用 api->data("setting.参数名")（按 ABI 补全 context 和默认值）读取。theme.json 包含 id、唯一 packageName、title、appVersion、description、author、requirements。stateBytes 不小于 sizeof(State)，graphicsBytes 包含 8192 字节画布，声明目标帧率和实际 sensors。传感器键名须查运行时/示例，不凭空编造；处理传感器缺失及方向变化。
5. 使用 dtMs 和有上限的固定步长物理；帧循环不申请内存、不阻塞。大素材使用 api->read 按需读取并检查边界。主题可安全停止、重新启动；ESP32 与 ESP32-S3 的包不能混用。
6. 检查 Python、PlatformIO 及交叉编译器；缺少时按 theme-studio/embedded/demo/platformio.ini 的 ${env} 环境安装依赖，不上传固件。
7. 编译：python theme-studio/native/build.py ${id} --chip ${chip}
8. 交付 theme-studio/native/dist/${id}-${chip}.cwp、完整源码/素材、电脑预览和实际执行的验证结果。无实机时 FPS 只写目标，不能声称实测达标。说明在主题工坊连接 USB 并导入 .cwp 的步骤。
9. 不修改 Wi-Fi、屏幕驱动或无关文件；未经明确要求不烧录、不发布。`}`;
}
