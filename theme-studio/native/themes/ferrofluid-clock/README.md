# 磁流体时钟 · v1.5.0

白色背板、透明油液、黑色横排 HH:MM；圆弧磁极使磁性液体本身形成连续笔画。倾斜时液滴受有效重力和浮力影响，快速晃动产生拉伸、缩颈与分离，停止后受油阻力减速并回到数字磁场。A 开关磁场，B 重置。关闭磁场后液滴仍受油阻力、界面张力和重力作用，并逐渐下沉聚集。

采用 252 个流体采样粒子、240 Hz 固定步长、空间网格邻域、双密度压力、粘性、边界碰撞、有界外磁场梯度和软化磁偶极作用。油介质以低阶流动模型保留短暂惯性并衰减；拖曳取决于液滴相对油流的速度。浮力降低沉降，晃动不再人为降低磁场强度。旋转、重力和惯性输入来自 IMU，连续密度表面、细微高光和倾斜投影表达浸液质感；不添加水面波浪或随机装饰气泡。

帧路径没有动态分配；实际状态 33,680 字节，声明预算 36 KiB。仅支持 ESP32-S3，目标 120 FPS；电脑计时不能代表 S3 实际帧率，本版本尚未进行实机帧率测量。

后台参数共七项：磁力、重力、油阻尼、摇散力度、凝聚、成字时间，以及新增的「油中浮力」（0–0.95，默认 0.7）。浮力为抵消重力的比例，便于调节视觉沉降速度，不是对某种具体油液的测量值。保持原包名 `org.clockwise.ferrofluid-clock`，可直接更新应用，无需更新固件。旧固件内置离线包随固件版本保留，在线目录提供新版应用。

这是针对 64×64 屏幕的二维视觉/物理近似，并非完整两相 Navier–Stokes 与 Maxwell 联立求解或工程测量模型。假设黑色磁流体与透明油液互不相溶，油腔充满；并非任意油基磁流体都能在任意油中保持液滴。虚拟磁极按时间变化，没有固定动画替代 IMU 响应，也没有叠加静止字体掩盖流体运动。

本次依据 [Ferrotec 磁流体技术说明](https://ferrofluid.ferrotec.com/technology/) 区分外磁场吸引与液体惯性；依据 [Latikka 等，Ferrofluid Microdroplet Splitting for Population-Based Microfluidics and Interfacial Tensiometry (2020)](https://pmc.ncbi.nlm.nih.gov/articles/PMC7375242/) 参考不相溶油液中的拉伸、缩颈、分裂和界面张力竞争。普通磁流体撤去外磁场后没有持续的宏观磁化，黏聚液滴不等于磁性纳米粒子团聚。

流体方法参考 [Clavet、Beaudoin、Poulin：Particle-based Viscoelastic Fluid Simulation (2005)](https://diglib.eg.org/items/7cad7994-b781-40ce-ad04-0bf61dc94279)。磁性液体的实验行为参考 [Ferrofluid drop impacts and Rosensweig peak formation in a non-uniform magnetic field (2023)](https://pubs.rsc.org/en/Content/ArticleLanding/2023/SM/D3SM00701D)。实现代码为本项目自写。

电脑验证：`python theme-studio/native/tests/check.py ferrofluid-clock`。独立主题测试覆盖白底黑字对比度、晃动/回聚、浮力沉降差异、油流衰减、分钟换形、长帧、陀螺仪惯性、四向旋转、极值参数和 IMU 失效。输出 30 秒真实 C++ 帧记录到 `native/build/host/ferrofluid.rgb565`。

造型参考：[Rhei / Damjan Stankovic](https://www.damjanstankovic.com/#rhei)、[Ferrolic / Zelf Koelman](https://vimeo.com/116510462)。仅参考作品的液体成字与运动设计，未分发原作照片、视频或代码。

1.1.0 主题：重画圆弧磁极、横排时间、平滑密度表面与边缘覆盖、抑制颗粒高光；换字时磁极平滑迁移，剧烈晃动暂时降低约束，松手回吸。

历史 1.2.0：采用暖金底板、侧光与背板投影；液滴从底部大液团逐个拉伸成字，换字仅重塑改变的数字。1.5.0 已替换为白底黑色浸油效果。每个数字固定粒子组，防止换字跨位流失；受像素密度和二维模型限制，不宣称与实拍逐像素等同。参考视频仅本地分析，不随项目发布。

English: v1.5.0 renders black ferrofluid digits in a completely filled transparent oil cell over a white backing. Buoyancy reduces settling; drag is relative to a decaying oil flow. Shaking produces inertia without artificially weakening the magnetic field. Seven parameters include the new buoyancy ratio (default 0.7). The 252-marker solver has a fixed 240 Hz step, targets 120 FPS on ESP32-S3 only, and uses 33,680 state bytes with no per-frame allocations. This is a reduced 2D model, not a full two-phase magnetohydrodynamic solver. Host tests and recorded C++ frames are available; physical-device FPS has not been measured for this version.
