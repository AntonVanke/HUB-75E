# 第三方来源

仓库根目录的 MIT LICENSE 不替代第三方代码、字体或素材的原有许可。

| 项目 | 用途与来源 |
| --- | --- |
| Clockwise | 原主题时钟项目与设计参考：[jnthas/clockwise](https://github.com/jnthas/clockwise) |
| ESP32 HUB75 MatrixPanel DMA | HUB75 DMA 扫描后端，工程锁定 3.0.12：[源码与许可证](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/tree/3.0.12) |
| Adafruit Protomatter | S3 独立显示后端，锁定 1.7.0：[源码与许可证](https://github.com/adafruit/Adafruit_Protomatter/tree/1.7.0) |
| Adafruit GFX / BusIO | 绘图与设备接口，依赖包保留各自 BSD / MIT 许可证：[GFX](https://github.com/adafruit/Adafruit-GFX-Library)、[BusIO](https://github.com/adafruit/Adafruit_BusIO) |
| Lua | 原生 Lua 解释器源代码，许可保留在 [LICENSE.html](theme-studio/embedded/src/lua/LICENSE.html)，[官方许可](https://www.lua.org/license.html) |
| Fengari / fengari-web | 浏览器 Lua 预览：[Fengari](https://github.com/fengari-lua/fengari)、[fengari-web](https://github.com/fengari-lua/fengari-web) |
| fflate | 浏览器与工具端 ZIP 处理：[源码与许可证](https://github.com/101arrowz/fflate) |
| esbuild | 浏览器代码打包：[源码与许可证](https://github.com/evanw/esbuild) |
| Nayuki QR Code generator | 首次配网 Wi-Fi 二维码，C 版 v1.8.0；源码版权与 MIT 许可保留于 `theme-studio/embedded/demo/src/qr/`：[源码与许可证](https://github.com/nayuki/QR-Code-generator/tree/v1.8.0/c) |
| ESP Web Tools | 在线 USB 烧录页面，内置官方 10.1.0 Web 组件并增加中文/英文文案；保留 `web/vendor/esp-web-tools-10.1.0/LICENSE` 与本地化变更说明：[源码与许可证](https://github.com/esphome/esp-web-tools) |

固件的 PlatformIO 依赖从各原项目获取，其版权头、许可证和版本声明保留在依赖包中。浏览器分发的库文件保留原有版权声明。

参考项目也包括 [ClockWise Plus](https://topyuan.top/clock/) 项目及其作者 [冯雪原 / MonsterYuan（yuan910715）](https://github.com/yuan910715)，其表盘展示、像素设计和教程是本项目的重要参考。

Design references include [ClockWise Plus](https://topyuan.top/clock/) and its author [MonsterYuan / 冯雪原 (yuan910715)](https://github.com/yuan910715) for the clock-face designs, pixel-art references and project documentation.

主题参考还包含公开像素表盘及游戏/影视题材；项目 JSON、脚本和已有资源说明中的作者、来源字段应继续保留。素材并无统一的开放许可，游戏名称、角色、商标及原始图像的权利归相应权利人。本文件不将这些素材重新许可为 MIT。

固件与开发工具链还使用 Espressif Arduino-ESP32 / ESP-IDF、esptool / esptool-js、PlatformIO、FreeRTOS、LittleFS、mbedTLS、lwIP 及 Xtensa GCC 工具链。马里奥关卡资源参考 FullScreenMario，原始素材与主题保留已有来源声明。以上依赖的许可见原项目及分发目录，不因本项目 MIT 许可改变。
