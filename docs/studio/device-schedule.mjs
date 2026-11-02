const minutes=s=>{if(!/^([01]\d|2[0-3]):[0-5]\d$/.test(s))throw new Error('时间格式为 HH:MM');return Number(s.slice(0,2))*60+Number(s.slice(3));};
const hhmm=n=>String(Math.floor(n/60)).padStart(2,'0')+':'+String(n%60).padStart(2,'0');
const weekdays='日一二三四五六';
const dayNames=days=>days===127?'每天':days===62?'工作日':days===65?'周末':Array.from({length:7},(_,i)=>days&(1<<i)?'周'+weekdays[i]:'').filter(Boolean).join('、')||'未选择星期';

export function scheduleCoverage(rules){
  const owner=new Int8Array(7*1440).fill(-1),overlaps=new Set(),fullDays=[];
  rules.forEach((rule,index)=>{
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
  const now=clock.hour*3600+clock.minute*60+(clock.second||0),events=[];
  const activeRuleAt=at=>{
    const offset=Math.floor(at/86400),day=(clock.weekday+offset%7+7)%7,minute=(at-offset*86400)/60;
    return plan.rules.findIndex(rule=>rule.start<rule.end?minute>=rule.start&&minute<rule.end&&!!(rule.days&(1<<day)):(minute>=rule.start&&!!(rule.days&(1<<day)))||(minute<rule.end&&!!(rule.days&(1<<((day+6)%7)))));
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
    plan.rules.forEach((rule,index)=>{if(rule.days&(1<<day)){add(offset*86400+rule.start*60,'start',index);add(offset*86400+rule.end*60+(rule.start>rule.end?86400:0),'end',index);}});
    if(plan.mode===2&&offset>=0)for(const time of plan.times)add(offset*86400+time*60,'daily');
  }
  if(plan.mode===1&&source==='interval'&&Number.isFinite(remaining)&&remaining>=0)add(now+Math.max(1,remaining),'interval');
  return events.sort((a,b)=>a.seconds-b.seconds)[0]||null;
}

export function scheduleControls($,getBase,request,message){
  let themes=[],dirty=false,saving=false,loadedBase='',statusBase='',lastStatus={},savedPlan=null;
  const titles=new Map([[-1,'基础时钟']]),inputs=new WeakMap();
  const title=id=>titles.get(Number(id))||'未安装的主题 '+id;
  const menu=()=>themes.find(theme=>theme.title.includes('菜单')||theme.id?.includes('menu'));
  function options(select,current){
    select.replaceChildren(...themes.map(theme=>new Option(theme.title,theme.index)));
    if(current!==undefined&&!themes.some(theme=>Number(theme.index)===Number(current))){const option=new Option(title(current)+'（已卸载，请重新选择）',current);option.disabled=true;select.add(option);}
    select.value=current??themes[0]?.index??'';
  }
  function ruleData(box,strict=false){
    const {start,end,theme,week}=inputs.get(box),days=[...week.querySelectorAll('input:checked')].reduce((mask,input)=>mask|(1<<Number(input.value)),0);
    let a,b;try{a=minutes(start.value);b=minutes(end.value);}catch(error){if(strict)throw error;}
    if(strict&&(!days||a===b))throw new Error('每个指定时段都需要适用星期和不同的开始、结束时间');
    if(strict&&!themes.some(item=>Number(item.index)===Number(theme.value)))throw new Error('指定时段使用了已卸载的主题，请重新选择');
    return {start:a,end:b,theme:Number(theme.value),days};
  }
  function overview(){
    const source=lastStatus.playbackSource,normal=title(lastStatus.baseTheme??lastStatus.theme??-1);
    const descriptions={window:'指定时段优先 · '+title(lastStatus.theme),interval:'默认播放 · 按间隔轮换',daily:'默认播放 · 每天定点切换',fixed:'默认播放 · 固定主题「'+normal+'」','waiting-time':'等待网络校时；当前保持默认主题'};
    $('playback-state').textContent=descriptions[source]||'正在读取播放安排';
    const next=lastStatus.timeValid?nextScheduleEvent(savedPlan,lastStatus.clock,lastStatus.rotationRemainingSeconds,source):null;
    const labels={start:'指定时段开始',end:'指定时段结束',daily:'每日定点切换',interval:'间隔轮换'};
    if(next){const day=next.dayOffset===0?'今天':next.dayOffset===1?'明天':'周'+weekdays[(lastStatus.clock.weekday+next.dayOffset)%7];$('next-switch').textContent=day+' '+hhmm(next.minute)+' · '+labels[next.kind];}
    else $('next-switch').textContent=lastStatus.timeValid?(savedPlan?.mode===0?'未安排下一事件，保持固定主题':'查看播放计划'):'校时后显示下一计划事件';
  }
  function update(){
    const mode=Number($('schedule-mode').value);
    $('schedule-interval-field').hidden=mode!==1;$('schedule-times-field').hidden=mode!==2;$('schedule-rotation-options').hidden=mode===0;
    $('schedule-interval').disabled=mode!==1;$('schedule-times').disabled=mode!==2;
    for(const input of $('schedule-rotation-options').querySelectorAll('input,select'))input.disabled=mode===0;
    $('pool-count').textContent='已选 '+$('schedule-pool').querySelectorAll('input:checked').length+' 个';
    $('schedule-dirty').textContent=dirty?'有未保存的修改':'已保存到设备';$('schedule-save').disabled=!dirty||saving;
    const rules=[...$('schedule-rules').children].map((box,index)=>{
      const rule=ruleData(box),{start,end,summary}=inputs.get(box);
      summary.textContent='时段 '+(index+1)+' · '+(start.value||'开始时间')+'–'+(rule.start>rule.end?'次日 ':'')+(end.value||'结束时间')+' · '+title(rule.theme)+' · '+dayNames(rule.days);
      return rule;
    });
    const fixed=title(lastStatus.baseTheme??lastStatus.theme??-1);
    const normal=mode===1?'每隔 '+($('schedule-interval').value||'…')+' 分钟在勾选主题中轮换':mode===2?'每天 '+($('schedule-times').value||'指定时间')+' 切换到下一个勾选主题':'保持固定主题「'+fixed+'」，不自动轮换';
    $('schedule-summary').textContent=(rules.length?'指定时段优先；其它时间：':'默认播放：')+normal+'。';
    const coverage=scheduleCoverage(rules),warnings=[];
    if(coverage.overlaps.length)warnings.push('时段 '+coverage.overlaps.join('；')+' 有重叠，重叠部分按列表上方的时段优先。');
    if(coverage.fullDays.length===7)warnings.push('指定时段已覆盖每一天的全天，默认播放方式没有执行时间。');
    else if(coverage.fullDays.length)warnings.push(coverage.fullDays.map(day=>'周'+weekdays[day]).join('、')+' 已被指定时段覆盖全天。');
    if(mode&&$('schedule-pool').querySelectorAll('input:checked').length<2)warnings.push('轮换至少选择两个主题，才能看到主题变化。');
    if(rules.some(rule=>!themes.some(item=>Number(item.index)===rule.theme)))warnings.push('有时段使用已卸载的主题，请重新选择后保存。');
    $('schedule-warnings').textContent=warnings.join('\n');$('schedule-warnings').hidden=!warnings.length;
  }
  function changed(){dirty=true;update();}
  function addRule(a=660,b=750,t,days=127,open=true){
    if(!themes.length)throw new Error('请先安装主题应用，再添加指定时段');
    if($('schedule-rules').children.length>=12)throw new Error('最多 12 个指定时段');
    const box=document.createElement('details'),summary=document.createElement('summary'),fields=document.createElement('div');
    box.className='schedule-rule';box.open=open;summary.className='rule-summary';fields.className='rule-fields';
    const start=document.createElement('input'),end=document.createElement('input'),theme=document.createElement('select'),week=document.createElement('div');
    start.type=end.type='time';start.value=hhmm(a);end.value=hhmm(b);options(theme,t??themes[0].index);week.className='rule-days';
    for(let day=0;day<7;day++){const label=document.createElement('label'),input=document.createElement('input');input.type='checkbox';input.value=day;input.checked=!!(days&(1<<day));input.setAttribute('aria-label','星期'+weekdays[day]);label.append(input,'周'+weekdays[day]);week.append(label);}
    for(const [name,input] of [['开始时间',start],['结束时间',end],['指定主题',theme]]){const label=document.createElement('label');label.append(name,input);fields.append(label);}
    const dayLabel=document.createElement('div');dayLabel.className='rule-week';dayLabel.append('适用星期',week);fields.append(dayLabel);
    const remove=document.createElement('button');remove.type='button';remove.className='secondary';remove.textContent='删除时段';remove.onclick=()=>{box.remove();changed();};fields.append(remove);
    inputs.set(box,{start,end,theme,week,summary});box.append(summary,fields);$('schedule-rules').append(box);
    if(open)changed();else update();
  }
  function renderPool(selected){
    const items=themes.concat([...selected].filter(id=>!themes.some(theme=>String(theme.index)===id)).map(id=>({index:id,title:title(id)+'（已卸载）'})));
    $('schedule-pool').replaceChildren(...items.map(theme=>{
      const label=document.createElement('label'),check=document.createElement('input'),name=document.createElement('span');label.className='theme-choice';check.type='checkbox';check.value=theme.index;check.checked=selected.has(String(theme.index));check.setAttribute('aria-label',theme.title);name.textContent=theme.title;label.append(check,name);return label;
    }));
  }
  $('add-rule').onclick=()=>{try{addRule();}catch(error){message(error.message,true);}};
  $('lunch-rule').onclick=()=>{try{const item=menu();if(!item)throw new Error('请先安装菜单主题，再添加中午时段');addRule(660,750,item.index);}catch(error){message(error.message,true);}};
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
      const rules=[...$('schedule-rules').children].map((box,index)=>{try{const rule=ruleData(box,true);return [rule.start,rule.end,rule.theme,rule.days].join(',');}catch(error){box.open=true;throw new Error('时段 '+(index+1)+'：'+error.message);}});
      const schedule=[[mode,interval,$('schedule-order').value].join(','),pool.join(','),times.join(','),...rules,''].join('\n');
      saving=true;fieldset.disabled=true;update();await request('schedule',{method:'POST',body:new URLSearchParams({schedule})});
      await loadSchedule(themes,{force:true});$('rotation').value=mode===1?interval:0;message('播放安排已保存并从设备重新读取。');
    }catch(error){dirty=true;message(error.message,true);}finally{saving=false;fieldset.disabled=wasDisabled;update();}
  };
  async function loadSchedule(list,{force=false}={}){
    const base=getBase(),sameDevice=base===loadedBase,selected=new Set([...$('schedule-pool').querySelectorAll('input:checked')].map(input=>input.value));
    if(!sameDevice){dirty=false;if(statusBase!==base)lastStatus={};titles.clear();titles.set(-1,'基础时钟');}
    themes=list.filter(theme=>Number(theme.index)>=0);for(const theme of list)titles.set(Number(theme.index),theme.title);
    $('add-rule').disabled=!themes.length;$('lunch-rule').disabled=!menu();
    if(dirty&&sameDevice&&!force){renderPool(selected);for(const box of $('schedule-rules').children){const input=inputs.get(box).theme;options(input,input.value);}update();return;}
    const response=await fetch(base+'/api/schedule',{credentials:'omit'});if(!response.ok)throw new Error('无法读取播放安排');
    const lines=(await response.text()).split('\n'),[mode,interval,random]=lines[0].split(',');
    loadedBase=base;savedPlan={mode:Number(mode),interval:Number(interval)||1,times:(lines[2]||'').split(',').filter(Boolean).map(Number),rules:lines.slice(3).filter(Boolean).map(row=>{const [start,end,theme,days]=row.split(',').map(Number);return {start,end,theme,days};})};
    $('schedule-mode').value=mode;$('schedule-interval').value=savedPlan.interval;$('schedule-order').value=random;$('schedule-times').value=savedPlan.times.map(hhmm).join(',');
    renderPool(new Set((lines[1]||'').split(',').filter(Boolean)));$('schedule-rules').replaceChildren();
    for(const rule of savedPlan.rules)addRule(rule.start,rule.end,rule.theme,rule.days,false);
    dirty=false;update();overview();
  }
  loadSchedule.status=data=>{lastStatus=data;statusBase=getBase();update();overview();};
  return loadSchedule;
}
