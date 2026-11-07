import {language,localizeTheme,registerThemes,onLanguageChange} from './i18n.mjs';
const minutes=s=>{if(!/^([01]\d|2[0-3]):[0-5]\d$/.test(s))throw new Error('时间格式为 HH:MM');return Number(s.slice(0,2))*60+Number(s.slice(3));};
const hhmm=n=>String(Math.floor(n/60)).padStart(2,'0')+':'+String(n%60).padStart(2,'0');
const weekdays='日一二三四五六';
const weekday=i=>language()==='en'?['Sun','Mon','Tue','Wed','Thu','Fri','Sat'][i]:'周'+weekdays[i];
const dayNames=days=>language()==='en'?(days===127?'Every day':days===62?'Weekdays':days===65?'Weekends':Array.from({length:7},(_,i)=>days&(1<<i)?weekday(i):'').filter(Boolean).join(', ')||'No days selected'):(days===127?'每天':days===62?'工作日':days===65?'周末':Array.from({length:7},(_,i)=>days&(1<<i)?'周'+weekdays[i]:'').filter(Boolean).join('、')||'未选择星期');

export function parseRule(row){const [start,end,theme,days,kind=0,threshold=20,enabled=1]=row.split(',').map(Number);return {start,end,theme,days,kind,threshold,enabled:enabled===1};}
export function encodeRule(rule,extended=true){const values=[rule.start,rule.end,rule.theme,rule.days];if(extended)values.push(rule.kind||0,rule.threshold??20,rule.enabled===false?0:1);return values.join(',');}

export function scheduleCoverage(rules){
  const owner=new Int8Array(7*1440).fill(-1),overlaps=new Set(),fullDays=[];
  rules.forEach((rule,index)=>{
    if(rule.kind||rule.enabled===false)return;
    const {start,end,days}=rule;if(!Number.isInteger(start)||!Number.isInteger(end)||start<0||start>1439||end<0||end>1439||start===end)return;
    for(let day=0;day<7;day++)if(days&(1<<day))for(let offset=0,length=(end-start+1440)%1440;offset<length;offset++){
      const at=(day*1440+start+offset)%owner.length;
      if(owner[at]>=0)overlaps.add(owner[at]+1+'、'+(index+1));else owner[at]=index;
    }
  });
  for(let day=0;day<7;day++)if(owner.subarray(day*1440,(day+1)*1440).every(value=>value>=0))fullDays.push(day);
  return {overlaps:[...overlaps],fullDays};
}
export function nextScheduleEvent(plan,clock,remaining,source){
  if(!plan||!clock||!Number.isInteger(clock.weekday))return null;
  if(plan.rules.some(r=>r.kind&&r.enabled!==false))return null;
  const now=clock.hour*3600+clock.minute*60+(clock.second||0),events=[];
  const activeRuleAt=at=>{
    const offset=Math.floor(at/86400),day=(clock.weekday+offset%7+7)%7,minute=(at-offset*86400)/60;
    return plan.rules.findIndex(rule=>rule.enabled!==false&&(rule.start<rule.end?minute>=rule.start&&minute<rule.end&&!!(rule.days&(1<<day)):(minute>=rule.start&&!!(rule.days&(1<<day)))||(minute<rule.end&&!!(rule.days&(1<<((day+6)%7))))));
  };
  const add=(at,kind,index)=>{
    const seconds=at-now;if(seconds<=0||seconds>7*86400)return;
    const after=activeRuleAt(at);
    if(kind==='daily'||kind==='interval'){if(after>=0)return;}
    else{const before=activeRuleAt(at-1);if(before===after||(kind==='start'?index!==after:index!==before))return;}
    events.push({seconds,kind,index,dayOffset:Math.floor(at/86400),minute:Math.floor((at%86400)/60)});
  };
  for(let offset=-1;offset<=7;offset++){
    const day=(clock.weekday+offset+7)%7;
    plan.rules.forEach((rule,index)=>{if(rule.enabled!==false&&(rule.days&(1<<day))){add(offset*86400+rule.start*60,'start',index);add(offset*86400+rule.end*60+(rule.start>rule.end?86400:0),'end',index);}});
    if(plan.mode===2&&offset>=0)for(const time of plan.times)add(offset*86400+time*60,'daily');
  }
  if(plan.mode===1&&source==='interval'&&Number.isFinite(remaining)&&remaining>=0)add(now+Math.max(1,remaining),'interval');
  return events.sort((a,b)=>a.seconds-b.seconds)[0]||null;
}

export function scheduleControls($,getBase,request,message){
  let themes=[],dirty=false,saving=false,loadedBase='',statusBase='',lastStatus={},savedPlan=null;
  const titles=new Map([[-1,'基础时钟']]),inputs=new WeakMap();
  const title=id=>{const theme=themes.find(theme=>Number(theme.index)===Number(id));return theme?localizeTheme(theme).title:titles.get(Number(id))||'未安装的主题 '+id;};
  function options(select,current){
    select.replaceChildren(...themes.map(theme=>new Option(localizeTheme(theme).title,theme.index)));
    if(current!==undefined&&!themes.some(theme=>Number(theme.index)===Number(current))){const option=new Option(title(current)+'（已卸载，请重新选择）',current);option.disabled=true;select.add(option);}
    select.value=current??themes[0]?.index??'';
  }
  function ruleData(box,strict=false){
    const {start,end,theme,week,kind,threshold,enabled}=inputs.get(box),days=[...week.querySelectorAll('input:checked')].reduce((mask,input)=>mask|(1<<Number(input.value)),0),type=Number(kind.value),level=Number(threshold.value);
    let a,b;try{a=minutes(start.value);b=minutes(end.value);}catch(error){if(strict)throw error;}
    if(strict&&!type&&(!days||a===b))throw new Error('每个指定时段都需要适用星期和不同的开始、结束时间');
    if(strict&&(![0,1,2].includes(type)||(type&&threshold.value==='')||!Number.isInteger(level)||level<0||level>100))throw new Error('环境光阈值应为 0–100%');
    if(strict&&type&&!lastStatus.brightnessControl?.supported)throw new Error('当前设备未配置光敏电阻，不能使用环境光条件');
    if(strict&&!themes.some(item=>Number(item.index)===Number(theme.value)))throw new Error('条件使用了已卸载的主题，请重新选择');
    return {start:type?0:a,end:type?0:b,theme:Number(theme.value),days:type?127:days,kind:type,threshold:level,enabled:enabled.checked};
  }
  function overview(){
    const source=lastStatus.playbackSource,normal=title(lastStatus.baseTheme??lastStatus.theme??-1);
    const descriptions={condition:'条件 '+lastStatus.conditionControl?.activeRule+' 已命中 · '+title(lastStatus.theme),window:'指定时段优先 · '+title(lastStatus.theme),interval:'默认播放 · 按间隔轮换',daily:'默认播放 · 每天定点切换',fixed:'默认播放 · 固定主题「'+normal+'」','waiting-time':'等待网络校时；当前保持默认主题'};
    $('playback-state').textContent=descriptions[source]||'正在读取播放安排';
    const next=lastStatus.timeValid?nextScheduleEvent(savedPlan,lastStatus.clock,lastStatus.rotationRemainingSeconds,source):null;
    const labels={start:'指定时段开始',end:'指定时段结束',daily:'每日定点切换',interval:'间隔轮换'};
    if(next){const day=next.dayOffset===0?'今天':next.dayOffset===1?'明天':weekday((lastStatus.clock.weekday+next.dayOffset)%7);$('next-switch').textContent=day+' '+hhmm(next.minute)+' · '+labels[next.kind];}
    else $('next-switch').textContent=savedPlan?.rules.some(r=>r.kind&&r.enabled!==false)?'随环境光条件变化，无法预知切换时间':lastStatus.timeValid?(savedPlan?.mode===0?'未安排下一事件，保持固定主题':'查看条件控制'):'校时后显示下一计划事件';
  }
  function update(){
    const mode=Number($('schedule-mode').value);
    $('schedule-interval-field').hidden=mode!==1;$('schedule-times-field').hidden=mode!==2;$('schedule-rotation-options').hidden=mode===0;
    $('schedule-interval').disabled=mode!==1;$('schedule-times').disabled=mode!==2;
    for(const input of $('schedule-rotation-options').querySelectorAll('input,select'))input.disabled=mode===0;
    $('pool-count').textContent='已选 '+$('schedule-pool').querySelectorAll('input:checked').length+' 个';
    $('schedule-dirty').textContent=dirty?'有未保存的修改':'已保存到设备';$('schedule-save').disabled=!dirty||saving;
    const light=lastStatus.conditionControl;
    $('condition-light').textContent=light?.lightValid?'当前环境光：'+light.lightPercent+'%'+(lastStatus.brightnessControl?.obstruction?.held?' · 遮挡期间使用遮挡前读数':''):'环境光读数暂不可用；时间条件仍可使用';
    const rules=[...$('schedule-rules').children].map((box,index)=>{
      const rule=ruleData(box),{start,end,summary,timeLabels,dayLabel,levelLabel,kind,up,down}=inputs.get(box);
      timeLabels.forEach(label=>label.hidden=!!rule.kind);dayLabel.hidden=!!rule.kind;levelLabel.hidden=!rule.kind;
      for(const option of kind.options)if(Number(option.value))option.disabled=!lastStatus.features?.conditionControl||!lastStatus.brightnessControl?.supported;
      up.disabled=index===0;down.disabled=index===$('schedule-rules').children.length-1;
      const when=rule.kind?('环境光'+(rule.kind===1?'低于 ':'高于 ')+rule.threshold+'%'):(start.value||'开始时间')+'–'+(rule.start>rule.end?'次日 ':'')+(end.value||'结束时间')+' · '+dayNames(rule.days);
      summary.textContent='条件 '+(index+1)+(rule.enabled?'':'（已停用）')+' · '+when+' → '+title(rule.theme);
      box.style.borderColor=!dirty&&lastStatus.conditionControl?.activeRule===index+1?'#248fa7':'';
      return rule;
    });
    const fixed=title(lastStatus.baseTheme??lastStatus.theme??-1);
    const normal=mode===1?'每隔 '+($('schedule-interval').value||'…')+' 分钟在勾选主题中轮换':mode===2?'每天 '+($('schedule-times').value||'指定时间')+' 切换到下一个勾选主题':'保持固定主题「'+fixed+'」，不自动轮换';
    $('schedule-summary').textContent=(rules.some(r=>r.enabled)?'从上到下匹配条件；均不满足时：':'默认播放：')+normal+'。';
    const coverage=scheduleCoverage(rules),warnings=[];
    if(coverage.overlaps.length)warnings.push('时段 '+coverage.overlaps.join('；')+' 有重叠，重叠部分按列表上方的时段优先。');
    if(coverage.fullDays.length===7)warnings.push('指定时段已覆盖每一天的全天，默认播放方式没有执行时间。');
    else if(coverage.fullDays.length)warnings.push(coverage.fullDays.map(day=>'周'+weekdays[day]).join('、')+' 已被指定时段覆盖全天。');
    if(mode&&$('schedule-pool').querySelectorAll('input:checked').length<2)warnings.push('轮换至少选择两个主题，才能看到主题变化。');
    if(rules.some(rule=>!themes.some(item=>Number(item.index)===rule.theme)))warnings.push('有时段使用已卸载的主题，请重新选择后保存。');
    if(rules.some(r=>r.kind&&r.enabled)&&!light?.lightValid)warnings.push('环境光条件正在等待有效传感器读数。');
    if(!lastStatus.timeValid&&rules.some(r=>!r.kind&&r.enabled))warnings.push('时间条件正在等待设备校时，环境光条件不受影响。');
    $('schedule-warnings').textContent=warnings.join('\n');$('schedule-warnings').hidden=!warnings.length;
  }
  function changed(){dirty=true;update();}
  function addRule(a=1320,b=420,t,days=127,open=true,type=0,level=20,on=true){
    if(!themes.length)throw new Error('请先安装主题应用，再添加条件');
    if($('schedule-rules').children.length>=12)throw new Error('最多 12 个条件');
    const box=document.createElement('details'),summary=document.createElement('summary'),fields=document.createElement('div');
    box.className='schedule-rule';box.open=open;summary.className='rule-summary';fields.className='rule-fields';
    const start=document.createElement('input'),end=document.createElement('input'),theme=document.createElement('select'),week=document.createElement('div');
    const kind=document.createElement('select'),threshold=document.createElement('input'),enabled=document.createElement('input');
    kind.append(new Option('处于指定时间段',0),new Option('环境光低于',1),new Option('环境光高于',2));kind.value=type;
    threshold.type='number';threshold.min=0;threshold.max=100;threshold.step=1;threshold.value=level;enabled.type='checkbox';enabled.style.width='auto';enabled.checked=on;enabled.disabled=!lastStatus.features?.conditionControl;
    const label=(name,input)=>{const node=document.createElement('label');node.append(name,input);fields.append(node);return node;};
    label('启用条件',enabled);label('触发条件',kind);const levelLabel=label('环境光阈值（%）',threshold);
    start.type=end.type='time';start.value=hhmm(a);end.value=hhmm(b);options(theme,t??themes[0].index);week.className='rule-days';
    kind.onchange=()=>{if(kind.value==='0'&&start.value===end.value){start.value='22:00';end.value='07:00';}};
    for(let day=0;day<7;day++){const label=document.createElement('label'),input=document.createElement('input'),name=document.createElement('span');input.type='checkbox';input.value=day;input.checked=!!(days&(1<<day));input.setAttribute('aria-label',weekday(day));name.dataset.weekday=day;name.textContent=weekday(day);label.append(input,name);week.append(label);}
    const timeLabels=[label('开始时间',start),label('结束时间',end)];label('播放主题',theme);
    const dayLabel=document.createElement('div');dayLabel.className='rule-week';dayLabel.append('适用星期',week);fields.append(dayLabel);
    const actions=document.createElement('div');actions.className='row';fields.append(actions);
    const button=(name,action)=>{const node=document.createElement('button');node.type='button';node.className='secondary';node.textContent=name;node.onclick=()=>{action();changed();};actions.append(node);return node;};
    const up=button('上移',()=>{if(box.previousElementSibling)box.previousElementSibling.before(box);});
    const down=button('下移',()=>{if(box.nextElementSibling)box.nextElementSibling.after(box);});
    button('删除条件',()=>box.remove());
    inputs.set(box,{start,end,theme,week,summary,kind,threshold,enabled,timeLabels,dayLabel,levelLabel,up,down});box.append(summary,fields);$('schedule-rules').append(box);
    if(open)changed();else update();
  }
  function renderPool(selected){
    const items=themes.concat([...selected].filter(id=>!themes.some(theme=>String(theme.index)===id)).map(id=>({index:id,title:title(id)+'（已卸载）'})));
    $('schedule-pool').replaceChildren(...items.map(theme=>{
      const label=document.createElement('label'),check=document.createElement('input'),name=document.createElement('span');label.className='theme-choice';check.type='checkbox';check.value=theme.index;check.checked=selected.has(String(theme.index));check.setAttribute('aria-label',localizeTheme(theme).title);name.textContent=localizeTheme(theme).title;label.append(check,name);return label;
    }));
  }
  $('add-rule').onclick=()=>{try{addRule();}catch(error){message(error.message,true);}};
  $('dark-rule').onclick=()=>{try{addRule(0,0,undefined,127,true,1,20);}catch(error){message(error.message,true);}};
  $('schedule-form').addEventListener('input',changed);$('schedule-form').addEventListener('change',changed);
  $('schedule-form').onsubmit=async event=>{
    event.preventDefault();if(saving)return;
    const fieldset=$('schedule-form').querySelector('fieldset'),wasDisabled=fieldset.disabled;
    try{
      const mode=Number($('schedule-mode').value);if(![0,1,2].includes(mode))throw new Error('请选择默认播放方式');
      const selected=[...$('schedule-pool').querySelectorAll('input:checked')].map(input=>input.value),pool=selected.filter(id=>themes.some(theme=>String(theme.index)===id));
      if(mode&&(!pool.length||pool.length!==selected.length))throw new Error('请选择已安装的轮换主题；已卸载的主题需要取消勾选');
      const interval=mode===1?Number($('schedule-interval').value):(savedPlan?.interval||1);
      if(mode===1&&(!Number.isInteger(interval)||interval<1||interval>1440))throw new Error('轮换间隔应为 1–1440 分钟');
      const times=mode===2?[...new Set($('schedule-times').value.split(',').map(value=>value.trim()).filter(Boolean).map(minutes))]:(savedPlan?.times||[]);
      if(mode===2&&!times.length)throw new Error('请填写每天切换主题的时间');if(times.length>16)throw new Error('每日最多设置 16 个切换时间');
      const rules=[...$('schedule-rules').children].map((box,index)=>{try{return encodeRule(ruleData(box,true),!!lastStatus.features?.conditionControl);}catch(error){box.open=true;throw new Error('条件 '+(index+1)+'：'+error.message);}});
      const schedule=[[mode,interval,$('schedule-order').value].join(','),pool.join(','),times.join(','),...rules,''].join('\n');
      saving=true;fieldset.disabled=true;update();await request('schedule',{method:'POST',body:new URLSearchParams({schedule})});
      await loadSchedule(themes,{force:true});$('rotation').value=mode===1?interval:0;message('播放安排已保存并从设备重新读取。');
    }catch(error){dirty=true;message(error.message,true);}finally{saving=false;fieldset.disabled=wasDisabled;update();}
  };
  async function loadSchedule(list,{force=false}={}){
    const base=getBase(),sameDevice=base===loadedBase,selected=new Set([...$('schedule-pool').querySelectorAll('input:checked')].map(input=>input.value));
    if(!sameDevice){dirty=false;if(statusBase!==base)lastStatus={};titles.clear();titles.set(-1,'基础时钟');}
    themes=list.filter(theme=>Number(theme.index)>=-1);registerThemes(list);for(const theme of list)titles.set(Number(theme.index),theme.title);
    $('add-rule').disabled=!themes.length;$('dark-rule').disabled=!themes.length||!lastStatus.features?.conditionControl||!lastStatus.brightnessControl?.supported;
    if(dirty&&sameDevice&&!force){renderPool(selected);for(const box of $('schedule-rules').children){const input=inputs.get(box).theme;options(input,input.value);}update();return;}
    const lines=(await request('schedule',{responseType:'text'})).split('\n'),[mode,interval,random]=lines[0].split(',');
    loadedBase=base;savedPlan={mode:Number(mode),interval:Number(interval)||1,times:(lines[2]||'').split(',').filter(Boolean).map(Number),rules:lines.slice(3).filter(Boolean).map(parseRule)};
    $('schedule-mode').value=mode;$('schedule-interval').value=savedPlan.interval;$('schedule-order').value=random;$('schedule-times').value=savedPlan.times.map(hhmm).join(',');
    renderPool(new Set((lines[1]||'').split(',').filter(Boolean)));$('schedule-rules').replaceChildren();
    for(const rule of savedPlan.rules)addRule(rule.start,rule.end,rule.theme,rule.days,false,rule.kind,rule.threshold,rule.enabled);
    dirty=false;update();overview();
  }
  loadSchedule.status=data=>{lastStatus=data;statusBase=getBase();update();overview();};
  onLanguageChange(()=>{const selected=new Set([...$('schedule-pool').querySelectorAll('input:checked')].map(input=>input.value));renderPool(selected);for(const box of $('schedule-rules').children){const input=inputs.get(box).theme;options(input,input.value);for(const node of box.querySelectorAll('[data-weekday]')){node.textContent=weekday(Number(node.dataset.weekday));node.previousElementSibling?.setAttribute('aria-label',node.textContent);}}update();overview();});
  return loadSchedule;
}
