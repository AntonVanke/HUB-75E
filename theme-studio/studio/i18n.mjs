import {strings} from './i18n-strings.mjs';
import {themeTranslations} from './theme-translations.mjs';

const languageKey='clockwise.language';
let current=String(globalThis.navigator?.language||'zh-CN').toLowerCase().startsWith('zh')?'zh-CN':'en';
try{const saved=globalThis.localStorage?.getItem(languageKey);if(saved==='en'||saved==='zh-CN')current=saved;}catch{}
const listeners=new Set(),texts=new WeakMap(),attributes=new WeakMap();
const fragments=Object.entries(strings).sort((a,b)=>b[0].length-a[0].length);
const themeWords=new Map();
let observer,scheduled=false,started=false,pending=[];
export const language=()=>current;
export function translate(value){
  const source=String(value??'');
  if(current!=='en'||!/[\u3400-\u9fff]/.test(source))return source;
  if(themeWords.has(source))return themeWords.get(source);
  if(Object.hasOwn(strings,source))return strings[source];
  const trimmed=source.trim();
  if(Object.hasOwn(strings,trimmed))return source.slice(0,source.indexOf(trimmed))+strings[trimmed]+source.slice(source.indexOf(trimmed)+trimmed.length);
  let result=source.replace(/(\d+) 款主题/g,'$1 themes');
  for(const [zh,en] of [...themeWords,...fragments])if(result.includes(zh))result=result.split(zh).join(en);
  return result.replace(/周([日一二三四五六])/g,(_,day)=>['Sun','Mon','Tue','Wed','Thu','Fri','Sat']['日一二三四五六'.indexOf(day)])
    .replace(/星期([日一二三四五六])/g,(_,day)=>['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday']['日一二三四五六'.indexOf(day)])
    .replaceAll('「','“').replaceAll('」','”').replaceAll('；','; ').replaceAll('：',': ').replaceAll('，',', ').replaceAll('、',', ').replaceAll('。','.');
}
export const t=translate;
export function localizeTheme(theme){
  const official=!theme?.packageName||theme.packageName==='org.clockwise.'+theme.id;
  const translated=official&&themeTranslations[theme?.id];
  if(current!=='en'||!translated)return {...theme,title:translate(theme?.title||'基础时钟'),description:translate(theme?.description||''),category:translate(theme?.category||'自定义')};
  return {...theme,...translated};
}
export function registerThemes(themes){
  for(const theme of themes||[]){
    const translated=(!theme.packageName||theme.packageName==='org.clockwise.'+theme.id)&&themeTranslations[theme.id];if(!translated)continue;
    for(const key of ['title','description','category'])if(theme[key]&&translated[key])themeWords.set(theme[key],translated[key]);
  }
  refreshLanguage();
}
export function onLanguageChange(listener){listeners.add(listener);return()=>listeners.delete(listener);}
function ignored(element){return element?.closest('script,style,textarea,pre:not(#error),[data-no-translate],#language-toggle,#ssid,#wifi-password,#wifi-status,#data-message,#resource-ids,.asset-name')||element?.closest('#networks option[value]:not([value=""])');}
function renderText(node){
  if(!node.parentElement||ignored(node.parentElement))return;
  const value=node.nodeValue,previous=texts.get(node);
  const original=previous&&value===previous.rendered?previous.original:value;
  const rendered=translate(original);texts.set(node,{original,rendered});if(value!==rendered)node.nodeValue=rendered;
}
function renderAttributes(element){
  if(element.closest('script,style,pre:not(#error),[data-no-translate],#language-toggle')||element.closest('#networks option[value]:not([value=""])'))return;
  let records=attributes.get(element);if(!records){records={};attributes.set(element,records);}
  for(const attr of ['title','aria-label','placeholder','alt']){
    const value=element.getAttribute(attr);if(value===null)continue;
    const previous=records[attr],original=previous&&previous.rendered===value?previous.original:value,rendered=translate(original);
    records[attr]={original,rendered};if(value!==rendered)element.setAttribute(attr,rendered);
  }
}
function apply(root){
  if(!root)return;
  if(root.nodeType===3){renderText(root);return;}
  if(root.nodeType===1)renderAttributes(root);
  if(!root.querySelectorAll)return;
  for(const element of root.querySelectorAll('[title],[aria-label],[placeholder],[alt]'))renderAttributes(element);
  const walker=document.createTreeWalker(root,NodeFilter.SHOW_TEXT);
  while(walker.nextNode())renderText(walker.currentNode);
}
function guideRedirect(){
  if(typeof location==='undefined')return false;
  const english=/\/theme-guide\.en\.html$/.test(location.pathname),chinese=/\/theme-guide\.html$/.test(location.pathname);
  if((english&&current==='zh-CN')||(chinese&&current==='en')){
    const url=new URL(english?'theme-guide.html':'theme-guide.en.html',location.href);url.hash=location.hash;
    location.replace(url);return true;
  }
  return false;
}
export function refreshLanguage(){
  if(typeof document==='undefined')return;
  document.documentElement.lang=current;
  const control=document.getElementById('language-toggle');
  if(control){control.textContent=current==='en'?'简体中文':'English';control.setAttribute('aria-label',current==='en'?'Switch to Simplified Chinese':'Switch to English');}
  observer?.disconnect();apply(document);observe();
}
export function setLanguage(value){
  const next=value==='en'?'en':'zh-CN';if(next===current)return;
  current=next;try{localStorage.setItem(languageKey,current);}catch{}
  if(guideRedirect())return;
  for(const listener of listeners)listener(current);
  refreshLanguage();
  globalThis.dispatchEvent?.(new CustomEvent('clockwise:languagechange',{detail:{language:current}}));
}
function observe(){
  if(!observer||!document.body)return;
  observer.observe(document.documentElement,{subtree:true,childList:true,characterData:true,attributes:true,attributeFilter:['title','aria-label','placeholder','alt']});
}
export function initI18n(){
  if(started||typeof document==='undefined')return;started=true;
  if(guideRedirect())return;
  const style=document.createElement('style');style.textContent='.language-toggle{flex:none;margin-left:12px;padding:6px 10px;border:1px solid #ffffff80;border-radius:4px;color:#fff;background:#ffffff18;font:inherit;font-size:13px;line-height:1.4;cursor:pointer;white-space:nowrap}.language-toggle:hover{background:#ffffff30}.language-toggle:focus-visible{outline:2px solid #fff;outline-offset:3px}@media(max-width:600px){.language-toggle{margin-left:0}}';document.head.append(style);
  const header=document.querySelector('header');if(header){const button=document.createElement('button');button.type='button';button.id='language-toggle';button.className='language-toggle';button.onclick=()=>setLanguage(current==='en'?'zh-CN':'en');header.append(button);}
  observer=new MutationObserver(records=>{
    pending.push(...records);if(scheduled)return;scheduled=true;
    queueMicrotask(()=>{scheduled=false;pending.push(...observer.takeRecords());const batch=pending;pending=[];observer.disconnect();for(const record of batch){if(record.type==='attributes')renderAttributes(record.target);else if(record.type==='characterData')renderText(record.target);else for(const node of record.addedNodes)apply(node);}observe();});
  });
  refreshLanguage();
  globalThis.addEventListener?.('storage',event=>{if(event.key===languageKey)setLanguage(event.newValue);});
  globalThis.addEventListener?.('clockwise:languagechange',event=>{if(event.detail?.language!==current)setLanguage(event.detail?.language);});
}
if(typeof document!=='undefined'){
  if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',initI18n,{once:true});else initI18n();
}
