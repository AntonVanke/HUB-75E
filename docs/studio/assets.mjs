export class AssetEditor{
  constructor(root,onChange){
    this.root=root;this.onChange=onChange;this.assets=[];this.index=0;this.frame=0;this.brush=0;
    root.innerHTML='<div class="asset-toolbar"><label>精灵 <select aria-label="精灵资源"></select></label><button data-action="new">新建 16×16</button><button data-action="import">导入 PNG</button><input type="file" accept="image/png" hidden><span class="asset-info"></span></div><div class="asset-body"><canvas width="16" height="16" aria-label="精灵像素编辑器"></canvas><div><label>资源名称 <input class="asset-name" maxlength="32" pattern="[A-Za-z0-9_-]+"></label><div class="palette"></div><label>当前颜色 <input class="asset-color" type="color"></label><button data-action="eraser">透明橡皮</button><div class="asset-frames"><button data-action="prev">上一帧</button><span class="frame-label"></span><button data-action="next">下一帧</button><button data-action="duplicate">复制为新帧</button></div><p>点击或拖动画布改像素；脚本使用 sprite.draw 绘制精灵。最多 255 色、32 帧，导入图像最长边缩放到 64。</p></div></div><p class="asset-error" role="alert"></p>';
    this.select=root.querySelector('select');this.canvas=root.querySelector('canvas');this.ctx=this.canvas.getContext('2d');
    for(const [action,label] of [['add-color','添加颜色'],['delete-frame','删除当前帧'],['delete','删除精灵']]){
      const b=document.createElement('button');b.dataset.action=action;b.textContent=label;
      root.querySelector(action==='delete'?'.asset-toolbar':'.asset-frames').append(b);
    }
    this.name=root.querySelector('.asset-name');this.color=root.querySelector('.asset-color');
    this.select.onchange=()=>{this.index=Number(this.select.value);this.frame=0;this.refresh();};
    this.name.onchange=()=>{const a=this.assets[this.index];if(!a)return;const name=this.name.value;if(!/^[\w-]{1,32}$/.test(name)||this.assets.some((v,i)=>i!==this.index&&v.id===name)){this.message('名称需唯一，仅用字母、数字、下划线或连字符');return;}a.id=name;this.refresh();onChange();};
    this.color.oninput=()=>{const a=this.assets[this.index];if(a&&this.brush>=0){a.palette[this.brush]=this.color.value;this.refresh();onChange();}};
    root.addEventListener('click',event=>{
      const action=event.target.dataset.action;if(!action)return;
      const a=this.assets[this.index];
      if(action==='new'){if(this.assets.length>=16){this.message('最多 16 个精灵');return;}this.assets.push({id:this.nextId(),width:16,height:16,palette:['#62e0c7','#ffe16a','#ffffff','#171d2e'],frames:['.'.repeat(256)]});this.index=this.assets.length-1;this.frame=0;this.refresh();onChange();}
      if(action==='import')root.querySelector('input[type=file]').click();
      if(!a)return;
      if(action==='eraser')this.brush=-1;
      if(action==='prev')this.frame=(this.frame+a.frames.length-1)%a.frames.length;
      if(action==='next')this.frame=(this.frame+1)%a.frames.length;
      if(action==='duplicate'){if(a.frames.length>=32){this.message('最多 32 帧');return;}a.frames.push(a.frames[this.frame]);this.frame=a.frames.length-1;onChange();}
      if(action==='delete'){this.assets.splice(this.index,1);this.index=Math.max(0,this.index-1);this.frame=0;onChange();}
      if(action==='delete-frame'){if(a.frames.length===1){this.message('至少保留一帧，可使用透明橡皮清空像素');return;}a.frames.splice(this.frame,1);this.frame=Math.min(this.frame,a.frames.length-1);onChange();}
      if(action==='add-color'){if(a.palette.length===255){this.message('最多 255 色');return;}if(a.palette.length===16&&a.encoding!==8){a.frames=a.frames.map(f=>[...f].map(c=>c==='.'?'..':'0'+c).join(''));a.encoding=8;}a.palette.push('#ffffff');this.brush=a.palette.length-1;onChange();}
      this.refresh();
    });
    let drawing=false;
    const paint=event=>{
      const a=this.assets[this.index];if(!a)return;
      const b=this.canvas.getBoundingClientRect(),x=Math.floor((event.clientX-b.left)*a.width/b.width),y=Math.floor((event.clientY-b.top)*a.height/b.height);
      if(x<0||y<0||x>=a.width||y>=a.height)return;
      const digits=a.encoding===8?2:1,pos=(y*a.width+x)*digits,text=a.frames[this.frame],v=this.brush<0?'.'.repeat(digits):this.brush.toString(16).toUpperCase().padStart(digits,'0');
      a.frames[this.frame]=text.slice(0,pos)+v+text.slice(pos+digits);this.draw();onChange();
    };
    this.canvas.onpointerdown=e=>{drawing=true;this.canvas.setPointerCapture(e.pointerId);paint(e);};
    this.canvas.onpointermove=e=>{if(drawing)paint(e);};this.canvas.onpointerup=this.canvas.onpointercancel=()=>drawing=false;
    root.querySelector('input[type=file]').onchange=async event=>{
      const file=event.target.files[0];if(!file)return;
      try{
        if(this.assets.length>=16)throw new Error('最多 16 个精灵');
        if(file.size>2097152)throw new Error('PNG 不得超过 2 MiB');
        const image=await createImageBitmap(file),scale=Math.min(1,64/image.width,64/image.height),w=Math.max(1,Math.round(image.width*scale)),h=Math.max(1,Math.round(image.height*scale));
        const canvas=document.createElement('canvas');canvas.width=w;canvas.height=h;const ctx=canvas.getContext('2d');ctx.imageSmoothingEnabled=false;ctx.drawImage(image,0,0,w,h);image.close();
        const bytes=ctx.getImageData(0,0,w,h).data,counts=new Map();
        const rgb=i=>'#'+[bytes[i],bytes[i+1],bytes[i+2]].map(v=>v.toString(16).padStart(2,'0')).join('');
        for(let i=0;i<bytes.length;i+=4)if(bytes[i+3]>=128){const c=rgb(i);counts.set(c,(counts.get(c)||0)+1);}
        const palette=[...counts].sort((a,b)=>b[1]-a[1]).slice(0,255).map(([c])=>c);if(!palette.length)palette.push('#ffffff');
        const colors=palette.map(c=>[1,3,5].map(i=>parseInt(c.slice(i,i+2),16)));let encoded='';
        for(let i=0;i<bytes.length;i+=4){if(bytes[i+3]<128){encoded+='..';continue;}let best=Infinity,index=0;colors.forEach((c,j)=>{const d=(c[0]-bytes[i])**2+(c[1]-bytes[i+1])**2+(c[2]-bytes[i+2])**2;if(d<best){best=d;index=j;}});encoded+=index.toString(16).toUpperCase().padStart(2,'0');}
        this.assets.push({id:this.nextId(),width:w,height:h,palette,encoding:8,frames:[encoded]});this.index=this.assets.length-1;this.frame=0;this.refresh();onChange();
      }catch(e){this.message(e.message);}finally{event.target.value='';}
    };
  }
  message(text){this.root.querySelector('.asset-error').textContent=text;}
  nextId(){let i=1;while(this.assets.some(a=>a.id==='sprite'+i))i++;return 'sprite'+i;}
  load(assets){this.assets=structuredClone(assets);this.index=this.frame=0;this.brush=0;this.refresh();}
  refresh(){
    this.message('');
    this.select.replaceChildren(...this.assets.map((a,i)=>new Option(a.id,i)));this.select.value=String(this.index);
    const a=this.assets[this.index];this.root.querySelector('.asset-body').hidden=!a;
    if(!a){this.root.querySelector('.asset-info').textContent='还没有精灵，可新建或导入';return;}
    if(this.brush>=a.palette.length)this.brush=0;
    this.name.value=a.id;this.root.querySelector('.asset-info').textContent=a.width+'×'+a.height+' · '+a.palette.length+' 色';
    this.root.querySelector('.frame-label').textContent=(this.frame+1)+' / '+a.frames.length;
    const palette=this.root.querySelector('.palette');palette.replaceChildren(...a.palette.map((c,i)=>{const b=document.createElement('button');b.style.background=c;b.ariaLabel='调色板颜色 '+(i+1);b.classList.toggle('selected',i===this.brush);b.onclick=()=>{this.brush=i;this.refresh();};return b;}));
    this.color.value=this.brush>=0?a.palette[this.brush]:'#000000';this.color.disabled=this.brush<0;
    this.draw();
  }
  draw(){
    const a=this.assets[this.index];if(!a)return;
    this.canvas.width=a.width;this.canvas.height=a.height;this.canvas.style.aspectRatio=a.width+'/'+a.height;
    const digits=a.encoding===8?2:1;
    for(let y=0;y<a.height;y++)for(let x=0;x<a.width;x++){const p=a.frames[this.frame].slice((y*a.width+x)*digits,(y*a.width+x+1)*digits);this.ctx.fillStyle=p[0]==='.'?((x+y)%2?'#e4edf2':'#c8d8e0'):a.palette[parseInt(p,16)];this.ctx.fillRect(x,y,1,1);}
  }
}
