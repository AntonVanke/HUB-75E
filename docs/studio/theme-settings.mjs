import {language} from './i18n.mjs';
export async function themeSettings(app,request){
 const en=language()==='en',data=await request(),dialog=document.createElement('dialog'),form=document.createElement('form');
 dialog.style.cssText='border:1px solid #cbdadf;border-radius:16px;padding:24px;width:460px;max-width:95vw;max-height:85vh;color:#173b45;background:#fff;box-shadow:0 20px 70px #0004';
 const title=document.createElement('h2');title.textContent=app.title;form.append(title);const fields=[];
 for(const [key,schema]of Object.entries(data.parameters||{})){
  const label=document.createElement('label'),input=document.createElement('input'),output=document.createElement('output');
  label.style.cssText='display:block;margin:16px 0';label.textContent=schema.label||key;input.type='range';input.min=schema.min;input.max=schema.max;input.step=schema.step;input.value=data.values[key]??schema.default;input.style.cssText='display:block;width:100%;accent-color:#168b88';input.oninput=()=>output.textContent=Number(input.value)+' '+(schema.unit||'');input.oninput();label.append(output,input);form.append(label);fields.push([key,input,schema]);
 }
 const result=document.createElement('p');result.setAttribute('role','status');form.append(result);
 const save=document.createElement('button'),reset=document.createElement('button'),close=document.createElement('button');save.textContent=en?'Save and apply':'保存并应用';reset.textContent=en?'Restore defaults':'恢复默认';close.textContent=en?'Cancel':'取消';save.type='submit';reset.type=close.type='button';
 for(const b of [save,reset,close])b.style.cssText='margin:6px;padding:9px 16px;border:0;border-radius:5px;background:#ddf0f1;color:#173b45;cursor:pointer';
 reset.onclick=()=>{for(const [,input,schema]of fields){input.value=schema.default;input.oninput();}};close.onclick=()=>dialog.close();
 form.onsubmit=async e=>{e.preventDefault();save.disabled=true;try{await request(Object.fromEntries(fields.map(([key,input])=>[key,Number(input.value)])));dialog.close();}catch(e){result.textContent=e.message;}finally{save.disabled=false;}};
 if(fields.length)form.append(save,reset);else result.textContent=en?'This app has no parameters.':'此应用没有可调参数。';form.append(close);dialog.append(form);document.body.append(dialog);dialog.onclose=()=>dialog.remove();dialog.showModal();
}
