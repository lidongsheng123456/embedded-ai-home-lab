#pragma once

const char HOME_PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HomeGuard Mini</title><style>
*{box-sizing:border-box}body{margin:0;background:#f3f6fa;color:#172033;font-family:system-ui,"Microsoft YaHei",sans-serif}
main{max-width:920px;margin:auto;padding:22px}h1{font-size:26px;margin:0 0 4px}.sub{color:#64748b;margin-bottom:20px}
.status{padding:16px 18px;border-radius:14px;background:#fff;margin-bottom:14px;border-left:7px solid #22a06b;box-shadow:0 4px 16px #0f172a12}
.status.warn{border-color:#e09b24}.status.danger{border-color:#dc3545}.status b{font-size:22px}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}
.card{background:#fff;border-radius:14px;padding:16px;box-shadow:0 4px 16px #0f172a12}.label{color:#64748b;font-size:14px}.value{font-weight:700;font-size:27px;margin-top:6px}.unit{font-size:14px;color:#64748b}
.actions{display:flex;gap:10px;margin-top:15px;flex-wrap:wrap}button{border:0;border-radius:10px;padding:11px 16px;font-size:15px;cursor:pointer;background:#2463eb;color:#fff}button.secondary{background:#475569}
.note{font-size:13px;color:#64748b;margin-top:18px}@media(max-width:650px){.grid{grid-template-columns:1fr 1fr}.value{font-size:23px}}
</style></head><body><main><h1>HomeGuard Mini</h1><div class="sub">一体化 AI 租房安全管家</div>
<div id="status" class="status"><b>正在连接设备…</b><div id="reason"></div></div>
<section class="grid">
<div class="card"><div class="label">人体存在</div><div class="value" id="presence">--</div></div>
<div class="card"><div class="label">CO₂</div><div class="value"><span id="co2">--</span> <span class="unit">ppm</span></div></div>
<div class="card"><div class="label">温度</div><div class="value"><span id="temp">--</span> <span class="unit">°C</span></div></div>
<div class="card"><div class="label">湿度</div><div class="value"><span id="humidity">--</span> <span class="unit">%RH</span></div></div>
<div class="card"><div class="label">光照</div><div class="value"><span id="light">--</span> <span class="unit">lux</span></div></div>
<div class="card"><div class="label">声音指数</div><div class="value" id="sound">--</div></div>
<div class="card"><div class="label">AI 异常分数</div><div class="value" id="score">--</div></div>
<div class="card"><div class="label">学习样本</div><div class="value" id="samples">--</div></div>
<div class="card"><div class="label">运行时间</div><div class="value"><span id="uptime">--</span> <span class="unit">分钟</span></div></div>
</section><div class="actions"><button onclick="learn()">重新学习正常环境</button><button class="secondary" onclick="silence()">静音 10 分钟</button></div>
<p class="note">本设备只能提供辅助提醒，不能替代消防、燃气、一氧化碳等符合国家标准的安全报警器。</p>
</main><script>
const $=id=>document.getElementById(id);async function refresh(){try{const d=await fetch('/api/status').then(r=>r.json());
['co2','temp','humidity','light','sound','score','samples'].forEach(k=>$(k).textContent=d[k]);$('presence').textContent=d.presence?'有人':'无人';$('uptime').textContent=Math.floor(d.uptime/60);
const box=$('status');box.className='status '+(d.level===2?'danger':d.level===1?'warn':'');box.querySelector('b').textContent=d.state;$('reason').textContent=d.reason;
}catch(e){$('status').className='status danger';$('status').querySelector('b').textContent='设备连接失败'}}
async function learn(){if(confirm('将清除已有模型，并重新收集约 10 分钟正常数据，确定吗？')){await fetch('/api/learn',{method:'POST'});refresh()}}
async function silence(){await fetch('/api/silence',{method:'POST'});refresh()}refresh();setInterval(refresh,2000);
</script></body></html>
)HTML";
