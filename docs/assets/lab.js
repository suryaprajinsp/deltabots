(()=>{'use strict';
const host=document.querySelector('#lab'),hud=document.querySelector('#hud'),error=document.querySelector('#lab-error');
if(!window.THREE){hud.textContent='3D engine unavailable.';error.hidden=false;error.textContent='The 3D library could not load. The other review pages are still available.';return}
let renderer;try{renderer=new THREE.WebGLRenderer({antialias:devicePixelRatio<2,alpha:false})}catch(e){hud.textContent='WebGL is unavailable.';error.hidden=false;error.textContent='Try a browser with WebGL enabled to run this experiment.';return}
renderer.setPixelRatio(Math.min(devicePixelRatio,1.5));renderer.setClearColor(0xa9d3ce);host.prepend(renderer.domElement);
const scene=new THREE.Scene();scene.fog=new THREE.Fog(0xa9d3ce,65,190);const camera=new THREE.PerspectiveCamera(65,1,.1,240);
scene.add(new THREE.HemisphereLight(0xe4fff3,0x475748,2));const sun=new THREE.DirectionalLight(0xffe6c1,2.5);sun.position.set(-30,80,25);scene.add(sun);
const colors={ivory:0xc3cbbc,dark:0x243d42,teal:0x246d70,cyan:0x3de7ed,sand:0xb8aa85,grass:0x709378,clay:0xa46d50,orange:0xf6a141,solar:0x1a3a59,ice:0x86c6ca};
const materials=Object.fromEntries(Object.entries(colors).map(([k,v])=>[k,new THREE.MeshLambertMaterial({color:v})]));const blocks=[];
function box(x,y,z,w,h,d,color='ivory',solid=false){const o=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),materials[color]);o.position.set(x,y,z);scene.add(o);if(solid)blocks.push({x,z,w:w/2+.45,d:d/2+.45});return o}
box(0,-.5,0,360,1,360,'grass');box(0,.01,-35,110,.05,8,'sand');box(-45,.01,0,8,.05,100,'sand');box(0,.01,40,110,.05,8,'sand');box(45,.01,0,8,.05,100,'sand');box(0,.01,0,8,.05,75,'sand');
// Hazard floor colors communicate the zones before dense visual dressing.
box(-35,.03,35,35,.08,30,'orange');box(35,.03,-30,30,.08,28,'ice');box(38,.03,37,30,.08,26,'teal');
function building(x,z,w=9,d=8,elevation=0){box(x,2.5+elevation,z,w,5,d,'ivory',true);box(x,5.15+elevation,z,w+.4,.3,d+.4,'teal');box(x,5.45+elevation,z,w*.8,.18,d*.65,'solar');box(x,2+elevation,z-d/2-.02,2.2,3,.05,'dark');box(x,4.05+elevation,z-d/2-.06,2.4,.12,.06,'cyan')}
for(const p of [[-48,-25],[-30,-25],[-42,32],[-22,35],[30,-30],[48,-23],[40,38]])building(...p);
for(const [x,z] of [[-20,-34],[15,-39],[42,0],[20,40],[-43,8],[-8,20]])box(x,.7,z,3,1.4,1.4,'teal',true);
for(const x of [-49,-41]){box(x,3,-43,1,6,1.2,'dark',true);box(x,3,-43.65,.15,5,.08,'cyan')}box(-45,6,-43,9,1,1.2,'ivory');
// Low-poly environment uses shared materials and simple geometry.
let seed=17;function rnd(){seed=(seed*1664525+1013904223)>>>0;return seed/4294967296}
for(let i=0;i<40;i++){let x=rnd()*130-65,z=rnd()*130-65;if(Math.min(Math.abs(z+35),Math.abs(z-40),Math.abs(x+45),Math.abs(x-45),Math.abs(x))<8)continue;const rock=new THREE.Mesh(new THREE.IcosahedronGeometry(1.5+rnd()*2,0),materials.clay);rock.position.set(x,1,z);rock.scale.y=.6;scene.add(rock)}
for(const [x,z] of [[-62,-50],[-62,15],[-20,60],[58,55],[60,-50]]){box(x,2+elevation,z,.6,4,.6,'clay');const crown=new THREE.Mesh(new THREE.IcosahedronGeometry(3,1),materials.teal);crown.position.set(x,5,z);scene.add(crown)}

// Original browser geometry for reviewing the six district palettes.
const districts={nexus:[0,0,100],desert:[-110,0,0],snow:[-100,0,-100],forest:[100,0,-100],sky:[100,16,0],abyss:[100,0,100]};
for(const [name,[x,y,z]] of Object.entries(districts)){
 box(x,y+.02,z,65,.1,65,name==='desert'?'sand':name==='snow'?'ice':name==='forest'?'grass':'teal');
 if(['nexus','sky','abyss'].includes(name))for(let i=0;i<8;i++){const a=i*Math.PI/4;building(x+Math.cos(a)*24,z+Math.sin(a)*24,7,7,y);}
 if(name==='desert'||name==='snow')for(let i=0;i<7;i++){const a=i*6.28/7;const r=new THREE.Mesh(new THREE.IcosahedronGeometry(7,0),materials[name==='snow'?'ivory':'clay']);r.scale.y=1.8;r.position.set(x+Math.cos(a)*30,5,z+Math.sin(a)*30);scene.add(r)}
 if(name==='forest')for(let i=0;i<15;i++){const px=x+(rnd()-.5)*55,pz=z+(rnd()-.5)*55;box(px,3,pz,.8,6,.8,'clay');const c=new THREE.Mesh(new THREE.IcosahedronGeometry(3,1),materials.teal);c.position.set(px,7,pz);scene.add(c)}
 if(name==='abyss'){const dome=new THREE.Mesh(new THREE.SphereGeometry(32,24,12),new THREE.MeshLambertMaterial({color:0x35bad1,transparent:true,opacity:.15,side:THREE.DoubleSide}));dome.position.set(x,0,z);scene.add(dome)}
}
const bot=new THREE.Group();scene.add(bot);const limbs=[];
function part(x,y,z,w,h,d,c,parent=bot){const p=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),materials[c]);p.position.set(x,y,z);parent.add(p);return p}
part(0,1.2,0,.58,.55,.32,'ivory');part(0,1.7,0,.42,.36,.36,'ivory');part(0,1.72,-.19,.34,.14,.02,'dark');part(0,1.73,-.207,.25,.025,.02,'cyan');
const delta=new THREE.Mesh(new THREE.ConeGeometry(.13,.24,3),materials.cyan);delta.rotation.z=Math.PI;delta.rotation.y=Math.PI/2;delta.position.set(0,1.25,-.19);bot.add(delta);
for(const s of [-1,1]){let leg=new THREE.Group();leg.position.set(s*.16,.9,0);bot.add(leg);part(0,-.38,0,.21,.73,.25,'ivory',leg);part(0,-.78,-.07,.23,.17,.38,'dark',leg);limbs.push(leg);let arm=new THREE.Group();arm.position.set(s*.4,1.45,0);bot.add(arm);part(0,-.3,0,.2,.6,.24,'ivory',arm);limbs.push(arm)}
part(-.4,.98,-.43,.06,.045,.7,'cyan');part(.4,1.03,-.18,.17,.17,.3,'dark');
const cells=[];for(const [x,z] of [[-35,23],[32,-45],[45,47]]){const c=new THREE.Mesh(new THREE.OctahedronGeometry(.65),materials.cyan);c.position.set(x,1.5,z);scene.add(c);cells.push(c)}
const keys=new Set();let yaw=0,pitch=.2,third=true,position=new THREE.Vector3(-45,0,-35),battery=100,health=100,found=0,complete=false,drag=null,clock=new THREE.Clock(),elapsed=0;
document.querySelectorAll('[data-zone]').forEach(b=>b.onclick=()=>{const p=districts[b.dataset.zone];position.set(...p);battery=health=100;});
function resize(){camera.aspect=host.clientWidth/host.clientHeight;camera.updateProjectionMatrix();renderer.setSize(host.clientWidth,host.clientHeight,false)}new ResizeObserver(resize).observe(host);resize();
function toggleView(){third=!third;document.querySelector('#view').textContent=third?'Third person':'First person'}document.querySelector('#view').onclick=toggleView;
function reset(){position.set(-45,0,-35);battery=health=100;found=0;complete=false;cells.forEach(c=>c.visible=true)}document.querySelector('#reset').onclick=reset;
document.addEventListener('keydown',e=>{if(/INPUT|TEXTAREA|SELECT/.test(e.target.tagName))return;if(['w','a','s','d','ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].includes(e.key)){keys.add(e.key);e.preventDefault()}if(e.key.toLowerCase()==='v'&&!e.repeat)toggleView()});document.addEventListener('keyup',e=>keys.delete(e.key));window.addEventListener('blur',()=>{keys.clear();drag=null});document.addEventListener('visibilitychange',()=>{if(document.hidden)keys.clear();clock.getDelta()});
for(const b of document.querySelectorAll('[data-move]')){b.onpointerdown=e=>{keys.add(b.dataset.move);b.setPointerCapture(e.pointerId);e.preventDefault()};b.onpointerup=b.onpointercancel=()=>keys.delete(b.dataset.move)}
renderer.domElement.onpointerdown=e=>{drag={id:e.pointerId,x:e.clientX,y:e.clientY};renderer.domElement.setPointerCapture(e.pointerId)};renderer.domElement.onpointermove=e=>{if(!drag||drag.id!==e.pointerId)return;yaw-=(e.clientX-drag.x)*.005;pitch=Math.max(-.65,Math.min(.8,pitch+(e.clientY-drag.y)*.004));drag.x=e.clientX;drag.y=e.clientY};renderer.domElement.onpointerup=renderer.domElement.onpointercancel=()=>drag=null;
function collides(x,z){return blocks.some(b=>Math.abs(x-b.x)<b.w&&Math.abs(z-b.z)<b.d)}
let lastHud=0;function frame(){requestAnimationFrame(frame);if(document.hidden)return;const dt=Math.min(clock.getDelta(),.05);elapsed+=dt;
let forward=Number(keys.has('w')||keys.has('ArrowUp'))-Number(keys.has('s')||keys.has('ArrowDown')),side=Number(keys.has('d')||keys.has('ArrowRight'))-Number(keys.has('a')||keys.has('ArrowLeft'));const pad=Array.from(navigator.getGamepads?navigator.getGamepads():[]).find(Boolean);if(pad){const dead=v=>Math.abs(v)<.18?0:Math.sign(v)*(Math.abs(v)-.18)/.82;side+=dead(pad.axes[0]||0);forward-=dead(pad.axes[1]||0);yaw-=dead(pad.axes[2]||0)*dt*1.8;pitch=Math.max(-.65,Math.min(.8,pitch+dead(pad.axes[3]||0)*dt*1.4));}const len=Math.max(1,Math.hypot(forward,side));forward/=len;side/=len;
const hot=position.x<-15&&position.z>16,ice=position.x>20&&position.z<-15,camp=Math.hypot(position.x+45,position.z+35)<10,speed=(ice?3.5:5)*(battery<15?.65:1);let dx=(-Math.sin(yaw)*forward+Math.cos(yaw)*side)*speed*dt,dz=(-Math.cos(yaw)*forward-Math.sin(yaw)*side)*speed*dt;
const nx=Math.max(-170,Math.min(170,position.x+dx)),nz=Math.max(-170,Math.min(170,position.z+dz));if(!collides(nx,position.z))position.x=nx;if(!collides(position.x,nz))position.z=nz;
if(!complete){battery=Math.max(0,Math.min(100,battery+dt*(camp?14:hot?-2.5:-.3)));if(battery===0)health=Math.max(0,health-dt*4);if(camp)health=Math.min(100,health+dt*8)}if(health<=0)reset();
bot.position.copy(position);bot.rotation.y=yaw;bot.visible=third;limbs.forEach((l,i)=>l.rotation.x=(forward||side)?Math.sin(elapsed*9+(i<2?0:Math.PI))*.45*(i%2?1:-1):0);
cells.forEach((c,i)=>{c.rotation.y+=dt;c.position.y=1.5+Math.sin(elapsed*2+i)*.2;if(c.visible&&c.position.distanceTo(position.clone().setY(c.position.y))<1.8){c.visible=false;found++;battery=Math.min(100,battery+20)}});if(found===3&&camp)complete=true;
const target=position.clone().add(new THREE.Vector3(0,third?1.25:1.65,0));if(third){let desired=target.clone().add(new THREE.Vector3(Math.sin(yaw)*4,2+pitch*2,Math.cos(yaw)*4));let delta=desired.clone().sub(target);const ray=new THREE.Raycaster(target,delta.clone().normalize(),.15,delta.length());const hits=ray.intersectObjects(scene.children,false).filter(h=>h.object!==bot&&h.distance>.25);if(hits.length)desired=target.clone().add(delta.normalize().multiplyScalar(Math.max(.5,hits[0].distance-.25)));camera.position.copy(desired);camera.lookAt(target)}else{camera.position.copy(target);camera.lookAt(target.clone().add(new THREE.Vector3(-Math.sin(yaw),-pitch,-Math.cos(yaw))))}
if(elapsed-lastHud>.15){lastHud=elapsed;hud.textContent=`${complete?'EXTRACTION COMPLETE':camp?'PORTAL CAMP · RECHARGING':hot?'SOLAR REACH · HEAT':ice?'FRACTURE BASIN · SLOW GROUND':'DELTA FRONTIER'}\nBattery ${Math.round(battery)}% · Health ${Math.round(health)}\nEnergy cells ${found}/3${found===3&&!complete?' · Return to camp':''}`;hud.style.whiteSpace='pre-line'}renderer.render(scene,camera)}frame();
})();
