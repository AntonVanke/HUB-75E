# 磁流体时钟

横排 HH:MM，圆弧磁极形成连续液体笔画。磁性液体本身组成数字；倾斜使其下垂流动，快速晃动产生脱离和液丝，停止后回到虚拟数字磁极。A 开关磁场，B 重置。

采用 176 个粒子、240 Hz 固定步长、空间网格邻域、双密度压力、粘性、边界碰撞、外磁场梯度和软化的磁偶极相互作用。旋转、重力和运动惯性来自 IMU，液面用连续密度场及高光渲染。帧路径没有动态分配；状态约 28.5 KiB。ESP32-S3 目标 120 FPS，ESP32 构建上限 60 FPS，实际设备帧率需实机测量。

这是针对 64×64 屏幕的二维视觉/物理近似，并非完整三维磁流体力学或工程测量模型。虚拟磁极按时间变化；没有用固定动画替代 IMU 响应。

流体方法参考 [Clavet、Beaudoin、Poulin：Particle-based Viscoelastic Fluid Simulation (2005)](https://diglib.eg.org/items/7cad7994-b781-40ce-ad04-0bf61dc94279)。磁性液体的实验行为参考 [Ferrofluid drop impacts and Rosensweig peak formation in a non-uniform magnetic field (2023)](https://pubs.rsc.org/en/Content/ArticleLanding/2023/SM/D3SM00701D)。实现代码为本项目自写。

电脑验证：`python theme-studio/native/tests/check.py ferrofluid-clock`，包括晃动/回吸、分钟换形、长帧、陀螺仪惯性、四向旋转、IMU 失效，以及自动亮度的存在性和断线回退。

造型参考：[Rhei / Damjan Stankovic](https://www.damjanstankovic.com/#rhei)、[Ferrolic / Zelf Koelman](https://vimeo.com/116510462)。仅参考作品的液体成字与运动设计，未分发原作照片、视频或代码。

1.1.0 主题：重画圆弧磁极、横排时间、平滑密度表面与边缘覆盖、抑制颗粒高光；换字时磁极平滑迁移，剧烈晃动暂时降低约束，松手回吸。
