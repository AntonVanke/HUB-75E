const encoder=new TextEncoder();
const idPattern=/^[a-zA-Z0-9_-]{1,32}$/;
const packagePattern=/^[a-z][a-z0-9-]*(?:\.[a-z][a-z0-9-]*)+$/;
const versionPattern=/^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/;
const chips=['esp32','esp32s3'];
const sensorTypes=['temperature','humidity','imu'];
function text(value,name,maximum,fallback=''){
  const result=value??fallback;
  if(typeof result!=='string'||encoder.encode(result).length>maximum||/[\x00-\x08\x0b\x0c\x0e-\x1f]/.test(result))throw new Error(name+' 无效或超过 '+maximum+' UTF-8 字节');
  return result;
}
function integer(value,name,maximum=64*1024*1024){
  const result=value??0;
  if(!Number.isSafeInteger(result)||result<0||result>maximum)throw new Error(name+' 必须是 0–'+maximum+' 的整数');
  return result;
}
export function normalizeAppManifest(project={}){
  const id=project.id??'imported';if(!idPattern.test(id))throw new Error('应用 ID 必须为 1–32 个字母、数字、下划线或连字符');
  const packageName=text(project.packageName,'包名',63,'local.clockwise.'+id.toLowerCase().replaceAll('_','-'));
  if(!packagePattern.test(packageName))throw new Error('包名须为小写点分命名空间，例如 org.example.my-clock');
  const appVersion=text(project.appVersion,'应用版本',31,'1.0.0');if(!versionPattern.test(appVersion))throw new Error('应用版本须为语义版本，例如 1.0.0');
  const requested=project.requirements??{};
  if(!requested||typeof requested!=='object'||Array.isArray(requested))throw new Error('requirements 必须为对象');
  const allowed=requested.chips??chips;
  if(!Array.isArray(allowed)||!allowed.length||allowed.length>2||allowed.some(chip=>!chips.includes(chip))||new Set(allowed).size!==allowed.length)throw new Error('requirements.chips 须包含 esp32 或 esp32s3，且不能重复');
  const sensors=requested.sensors??[];
  if(!Array.isArray(sensors)||sensors.length>3||sensors.some(sensor=>!sensorTypes.includes(sensor))||new Set(sensors).size!==sensors.length)throw new Error('requirements.sensors 仅支持 temperature、humidity、imu，且不能重复');
  const targetFps=integer(requested.targetFps??project.fps??60,'目标帧率',240);if(!targetFps)throw new Error('目标帧率应为 1–240');
  const requirements={chips:[...allowed],psramBytes:integer(requested.psramBytes,'PSRAM 需求'),luaHeapBytes:integer(requested.luaHeapBytes,'Lua 加载内存'),luaWorkingBytes:integer(requested.luaWorkingBytes,'Lua 运行内存'),graphicsBytes:integer(requested.graphicsBytes,'图形内存'),targetFps};
  const native=project.runtime==='native';
  if(native)for(const key of ['codeBytes','dataBytes','stateBytes'])requirements[key]=integer(requested[key],key);
  if(sensors.length)requirements.sensors=[...sensors];
  if(requirements.luaHeapBytes&&requirements.luaWorkingBytes>requirements.luaHeapBytes)throw new Error('Lua 运行内存需求不能大于加载内存需求');
  const runtimeApi=integer(project.runtimeApi??1,'运行时 API',65535);if(!runtimeApi)throw new Error('运行时 API 版本须大于 0');
  const result={packageName,id,title:text(project.title,'应用名称',96,id),appVersion,description:text(project.description,'应用描述',384),author:text(project.author,'作者',64,'未署名'),runtimeApi,requirements,requirementsDeclared:!!(project.requirements&&requirements.luaHeapBytes&&requirements.luaWorkingBytes&&requirements.graphicsBytes),verification:{status:'unverified'}};
  if(native){if(project.nativeAbi!==1)throw new Error('需要原生主题 ABI 1');result.runtime='native';result.nativeAbi=1;result.requirementsDeclared=!!(requirements.codeBytes&&requirements.stateBytes&&requirements.graphicsBytes);delete requirements.luaHeapBytes;delete requirements.luaWorkingBytes;}
  if(encoder.encode(JSON.stringify(result)).length>2048)throw new Error('应用元数据上限 2048 字节');
  if(project.parameters){
    if(typeof project.parameters!=='object'||Array.isArray(project.parameters)||Object.keys(project.parameters).length>12)throw Error('Invalid parameters');
    result.parameters={};for(const [key,v] of Object.entries(project.parameters)){
      if(!/^[a-zA-Z0-9_]{1,23}$/.test(key)||!v||!['min','max','step','default'].every(k=>Number.isFinite(v[k]))||v.min>v.max||v.step<=0||v.default<v.min||v.default>v.max)throw Error('Invalid parameter '+key);
      result.parameters[key]={label:text(v.label,'parameter label',64,key),min:v.min,max:v.max,step:v.step,default:v.default};
    }
  }
  return result;
}
export function checkAppCompatibility(manifest,device={}){
  if(!manifest)return {compatible:true,known:false,reasons:[],warnings:['旧主题没有设备需求声明，运行能力尚未验证。']};
  const app=normalizeAppManifest(manifest),requirements=app.requirements,limits=device.limits||device,reasons=[],warnings=[];
  const model=String(device.chip||'').toLowerCase().replace(/[^a-z0-9]/g,''),chip=model.startsWith('esp32s3')?'esp32s3':/^esp32(?:s2|c\d|h\d|p\d)/.test(model)?model:model.startsWith('esp32')?'esp32':'';
  if(chip&&!requirements.chips.includes(chip))reasons.push('仅支持 '+requirements.chips.join(' / ')+'，当前设备为 '+device.chip);
  if(device.chip&&!chip)reasons.push('无法识别当前设备型号 '+device.chip);
  for(const sensor of requirements.sensors||[]){
    const available=Array.isArray(device.sensorCapabilities)?device.sensorCapabilities.includes(sensor):device.sensors?.[sensor==='imu'?'imuPresent':'ahtPresent'];
    if(available!==true)reasons.push('设备未确认支持传感器 '+sensor);
  }
  if(app.runtimeApi>(limits.runtimeApi??device.runtimeApi??1))reasons.push('需要运行时 API '+app.runtimeApi);
  if(app.runtime==='native')for(const key of ['codeBytes','dataBytes','stateBytes'])if(Number.isFinite(limits[key])&&requirements[key]>limits[key])reasons.push(key+' 超过设备限制');
  const physicalPsram=Math.ceil((limits.psramBytes||0)/(1024*1024))*1024*1024;
  if(requirements.psramBytes>physicalPsram)reasons.push('需要 '+requirements.psramBytes+' 字节 PSRAM');
  for(const [key,label] of [['luaHeapBytes','Lua 加载内存'],['luaWorkingBytes','Lua 运行内存'],['graphicsBytes','图形内存']]){
    if(requirements[key]&&Number.isFinite(limits[key])&&requirements[key]>limits[key])reasons.push(label+'需求 '+requirements[key]+' 字节超过设备配额 '+limits[key]);
  }
  if(requirements.targetFps>(limits.targetFpsMax??60))reasons.push('目标帧率超过设备能力');
  if(!app.requirementsDeclared)warnings.push('资源需求未完整声明，不能据此确认低内存适配。');
  warnings.push('尚未在此设备验证性能，目标帧率不是流畅度保证。');
  return {compatible:!reasons.length,known:app.requirementsDeclared,reasons,warnings};
}
