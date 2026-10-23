<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Semita · Control de giro</title>
<style>
  :root{--azul:#004D98;--grana:#A50044;--oro:#EDBB00;--fondo:#F3F5FA;--texto:#14213D;--ok:#1B7F4B}
  *{box-sizing:border-box}
  body{margin:0;font-family:system-ui,Segoe UI,Arial,sans-serif;background:var(--fondo);color:var(--texto)}
  header{background:linear-gradient(90deg,var(--azul) 50%,var(--grana) 50%);color:#fff;display:flex;align-items:center;gap:16px;padding:14px 20px}
  header img{height:64px}
  header h1{margin:0;font-size:1.4rem}
  main{max-width:960px;margin:0 auto;padding:20px;display:grid;gap:20px}
  .panel{background:#fff;border-radius:10px;padding:20px;border:1px solid #d9deea}
  .lectura{display:flex;flex-wrap:wrap;gap:24px;align-items:center;justify-content:space-between}
  .display{background:#0b1b2b;color:#3df5d0;font:700 3.2rem/1 "Courier New",monospace;padding:12px 22px;border-radius:8px;min-width:210px;text-align:right}
  .display small{font-size:1.2rem}
  .estado span{display:inline-block;padding:4px 12px;border-radius:99px;background:#e6eaf3;font-weight:600}
  .estado p{margin:8px 0}
  .on{background:#d5f0e0!important;color:var(--ok)}.off{background:#f8d7da!important;color:#8a1c2b}
  .botones{display:flex;flex-wrap:wrap;gap:12px}
  button{font:600 1rem inherit;padding:12px 22px;border:0;border-radius:8px;cursor:pointer;color:#fff;background:var(--azul)}
  button:focus-visible{outline:3px solid var(--oro);outline-offset:2px}
  button:disabled{opacity:.4;cursor:not-allowed}
  .stop{background:#c62828}.modo{background:var(--grana)}
  .modo.activo{background:var(--oro);color:#14213D}
  .equipo{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:16px;text-align:center}
  .equipo img{width:100%;aspect-ratio:1;object-fit:cover;border-radius:10px;background:#d9deea}
  #msg{min-height:1.2em;color:#8a1c2b;margin:10px 0 0}
</style>
</head>
<body>
<header>
  <img src="img/barcelona.png" alt="Escudo del F. C. Barcelona">
  <h1>Control de giro con sensor</h1>
</header>
<main>
  <section class="panel lectura" aria-live="polite">
    <div class="display"><span id="dist">---</span> <small>cm</small></div>
    <div class="estado">
      <p>Motor: <span id="motor">--</span></p>
      <p>Modo: <span id="modo">--</span></p>
      <p>Dispositivo: <span id="online">--</span></p>
    </div>
  </section>

  <section class="panel">
    <h2>Modo de operación</h2>
    <div class="botones">
      <button class="modo" id="btnManual">Modo manual</button>
      <button class="modo" id="btnAuto">Modo automático</button>
    </div>
  </section>

  <section class="panel">
    <h2>Control del motor</h2>
    <div class="botones">
      <button id="btnLeft" data-a="left">Girar a la izquierda</button>
      <button id="btnRight" data-a="right">Girar a la derecha</button>
      <button id="btnStop" class="stop" data-a="stop">Detener</button>
    </div>
    <p id="msg"></p>
  </section>

  <section class="panel">
    <h2>Integrantes</h2>
    <div class="equipo">
      <figure><img src="img/enrique.jpg" alt="Integrante 1"><figcaption>Enrique Alexander Solano Lopez SL223188</figcaption></figure>
      <figure><img src="img/FOTO.jpg" alt="Integrante 2"><figcaption>Cristian Alexis Diaz Ramos DR231900</figcaption></figure>
    </div>
  </section>
</main>

<script>
const API = "https://semita-api.onrender.com";   // <-- URL de tu API (Plataforma B)
const $ = id => document.getElementById(id);
const nombres = {left:"Girando a la izquierda", right:"Girando a la derecha", stop:"Detenido"};
let mode = "manual";

async function call(path, method="GET", body){
  const r = await fetch(API+path,{method,headers:{"Content-Type":"application/json"},body:body?JSON.stringify(body):undefined});
  const data = await r.json();
  if(!r.ok) throw new Error(data.error||"Error");
  return data;
}

async function refresh(){
  try{
    const s = await call("/api/status");
    mode = s.mode;
    $("dist").textContent = (s.distance==null||s.distance<0) ? "---" : s.distance.toFixed(1);
    $("motor").textContent = nombres[s.motor] || "--";
    $("modo").textContent = s.mode==="auto" ? "Automático" : "Manual";
    const o=$("online"); o.textContent=s.deviceOnline?"En línea":"Sin conexión"; o.className=s.deviceOnline?"on":"off";
    $("btnAuto").classList.toggle("activo", s.mode==="auto");
    $("btnManual").classList.toggle("activo", s.mode==="manual");
    document.querySelectorAll("[data-a]").forEach(b=>b.disabled = s.mode==="auto");
  }catch(e){ $("online").textContent="API no disponible"; $("online").className="off"; }
}

async function setMode(m){ try{ $("msg").textContent=""; await call("/api/mode","POST",{mode:m}); refresh(); }catch(e){ $("msg").textContent=e.message; } }
$("btnAuto").onclick = ()=>setMode("auto");
$("btnManual").onclick = ()=>setMode("manual");
document.querySelectorAll("[data-a]").forEach(b=>b.onclick=async()=>{
  try{ $("msg").textContent=""; await call("/api/motor","POST",{action:b.dataset.a}); refresh(); }
  catch(e){ $("msg").textContent=e.message; }
});

refresh(); setInterval(refresh, 500);
</script>
</body>
</html>
