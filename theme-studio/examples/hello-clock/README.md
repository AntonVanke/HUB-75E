# 星点时钟示例

这是一个不依赖外部素材、64×64的低内存主题源码包。每秒随机换色，星点按经过时间缓慢移动；A键换色，B键开关星点。未校时显示`SYNC...`。

在 `theme-studio` 中运行 `npm run dev`，打开本地工坊，导入本文件夹即可预览。电脑预览使用电脑时间，A/B按钮模拟输入。

```powershell
node tools/theme-app.mjs pack examples/hello-clock --out .build/hello-clock.cwp
node tools/theme-app.mjs check .build/hello-clock.cwp
node tools/theme-app.mjs check .build/hello-clock.cwp --device http://172.16.11.5
```

最后一条只读检查设备兼容性，不上传。安装时，在设备后台的“主题应用”中选择CWP、原始文件夹或其ZIP。修改 `theme.json` 的 `packageName` 才表示一个新应用；更新同一应用时保留包名并增加 `appVersion`。

清单中的内存值是留有余量的作者声明，`fps/targetFps` 请求30FPS，不是设备性能认证。发布前请查看目标设备的实际加载峰值、运行内存和帧时间。

完整文档：[主题应用开发手册](../../docs/theme-authoring.md)、[应用包格式](../../docs/theme-app-format.md)。
