// Shared Lua API reference.
const api = (name, zh, group, args, returns = null) => ({ name, zh, group, args, returns });
export const API = [
  api('font.draw','原生字体','字体动画',[['字体','mario'],['x',8],['y',20],['内容','12:34'],['颜色','#ffffff'],['缩放',1]]),
  api('media.draw','动画资源','字体动画',[['ID','zootopia'],['x',0],['y',0],['毫秒',0]]),
  api('media.release','释放当前动画缓存','字体动画',[['ID','zootopia']]),
  api('resource.size','资源文件大小','数据交互',[['ID','mario-worlds']],'Number'),
  api('resource.read','按需读取资源；单次最多 8192 字节','数据交互',[['ID','mario-worlds'],['偏移',0],['字节数',32]],'String'),
  api('memory.collect','切换场景时释放不再使用的 Lua 对象','数据交互',[],'Number'),
  api('resource.rle4','原生解压图块 RLE；高 4 位为重复次数减一，低 4 位为图块编号，最多 8192 字节','数据交互',[['数据','']],'String'),
  api('rng.seed','随机种子','数学',[['种子',42]]),
  api('rng.float','随机小数','数学',[],'Number'),
  api('nav.bfs','网格距离；紧凑结果使用 nav.at 读取','寻路',[['宽',3],['高',3],['地图','.........'],['起点',1],['紧凑结果',false]],'Table'),
  api('nav.solve','加权寻路；紧凑结果节省内存','寻路',[['宽',3],['高',3],['地图','.........'],['起点',1],['代价',[]],['紧凑结果',false]],'Table'),
  api('nav.at','读取紧凑网格；不可达返回 nil','寻路',[['网格结果',''],['格子编号',1]],'Number'),
  api('gfx.clear','清屏','绘图',[['颜色','#081524']]),
  api('gfx.softglow','RGB565 轻柔荧光','绘图',[['强度 0–255',58]]),
  api('gfx.wave','局部水波与热浪；先绘制背景','绘图',[['x',10],['y',35],['宽',32],['高',14],['相位 0–15',0],['位移 0–3',1],['波长 2–64',12],['明暗 0–24',6]]),
  api('gfx.daylight','等经纬地图晨昏着色','绘图',[['太阳赤纬 °',0],['太阳直射经度 °',0],['x',0],['y',2],['宽',64],['高',48]]),
  api('gfx.pixel','像素','绘图',[['x',0],['y',0],['颜色','#ffffff']]),
  api('gfx.rect','矩形','绘图',[['x',0],['y',0],['宽',20],['高',10],['颜色','#62e0c7']]),
  api('gfx.outline','边框','绘图',[['x',0],['y',0],['宽',64],['高',64],['颜色','#62e0c7']]),
  api('gfx.line','线','绘图',[['x1',0],['y1',0],['x2',63],['y2',63],['颜色','#ffffff']]),
  api('gfx.circle','圆','绘图',[['x',32],['y',32],['半径',10],['颜色','#ffe16a']]),
  api('gfx.triangle','三角形','绘图',[['x1',32],['y1',8],['x2',8],['y2',48],['x3',56],['y3',48],['颜色','#62e0c7']]),
  api('gfx.gradient','渐变','绘图',[['x',0],['y',0],['宽',64],['高',64],['上色','#07152b'],['下色','#cd6791']]),
  api('gfx.text','文字','文字时间',[['x',2],['y',2],['内容','HELLO'],['颜色','#ffffff'],['字号',1]]),
  api('gfx.text_width','文字宽度','文字时间',[['内容','HELLO'],['字号',1]],'Number'),
  api('gfx.clip','裁剪','图层精灵',[['x',0],['y',0],['宽',64],['高',64]]),
  api('gfx.reset_clip','取消裁剪','图层精灵',[]),
  api('layer.save','保存图层','图层精灵',[['名称','background']]),
  api('layer.draw','绘制图层','图层精灵',[['名称','background'],['x',0],['y',0]]),
  api('sprite.draw','精灵','图层精灵',[['名称','hero'],['x',24],['y',28],['帧',1],['缩放',1],['翻转',false]]),
  api('sprite.frames','精灵帧数','图层精灵',[['名称','hero']],'Number'),
  api('sprite.tilemap','原生图块地图；十六进制帧号，0 留空，只绘制可见区域','图层精灵',[['精灵','terrain'],['地图','1203'],['列数',2],['x',0],['y',0]]),
  api('fx.create','创建粒子','动画粒子',[['名称','stars'],['数量',32],['x',0],['y',0],['宽',64],['高',64],['横速',0],['纵速',8],['颜色','#a5d8ff'],['样式','point']]),
  api('fx.draw','绘制粒子','动画粒子',[['名称','stars']]),
  api('clock.format','时钟','文字时间',[['格式','%H:%M']],'String'),
  api('clock.value','时间值','文字时间',[['字段','second']],'Number'),
  api('clock.utc','UTC 时间值及年内日序','文字时间',[['字段','yearday']],'Number'),
  api('clock.millis','毫秒','文字时间',[],'Number'),
  api('data.get','数据','数据交互',[['键','temperature'],['默认',22]],'Any'),
  api('input.pressed','按键','数据交互',[['键','A']],'Boolean'),
  api('telemetry.set','状态','数据交互',[['名称','value'],['值',0]]),
  api('math.sin','正弦','数学',[['弧度',0]],'Number'),
  api('math.cos','余弦','数学',[['弧度',0]],'Number'),
  api('math.floor','取整','数学',[['数值',0]],'Number'),
  api('math.abs','绝对值','数学',[['数值',0]],'Number'),
  api('rng.int','随机数','数学',[['最小',0],['最大',63]],'Number'),
  api('util.clamp','限制','数学',[['值',0],['最小',0],['最大',63]],'Number'),
  api('util.lerp','插值','动画粒子',[['起点',0],['终点',63],['进度',0.5]],'Number'),
  api('util.wave','波动','动画粒子',[['时间',0],['周期',2000],['最小',0],['最大',10]],'Number'),
  api('util.frame','动画帧','动画粒子',[['时间',0],['帧率',8],['帧数',2]],'Number'),
  api('util.every','定时到达','动画粒子',[['名称','timer'],['间隔',1000]],'Boolean'),
  api('table.insert','列表添加','列表',[['列表',[]],['值',0]]),
  api('table.remove','列表删除','列表',[['列表',[]],['位置',1]],'Any')
];
export function assetLua(assets = []) {
  return assets.map(a => 'assets.define(' + [a.id,a.width,a.height,a.palette.join(','),a.frames.join('|'),a.encoding===8?2:1].map(v => typeof v === 'string' ? JSON.stringify(v) : v).join(',') + ')').join('\n');
}
export function bundleLua(project,libraries) {
  const separator='\n-- @cw-part\n',parts=[...project.assets.map(a=>assetLua([a])),...project.libraries.map(n=>libraries[n]),...(project.modules||[]),project.source];
  if(parts.some(p=>p.includes(separator)))throw new Error('脚本包含保留的分段标记');
  return '-- @cw-bundle 1\n'+parts.filter(p=>p.trim()).join(separator)+'\n';
}
