#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

// หน้าเว็บหลัก Mobile Responsive Dashboard (Minimalist Clean White Theme - No Emojis)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="th">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ระบบตรวจเช็คน้ำท่วม (Flood Monitor)</title>
  <link href="https://fonts.googleapis.com/css2?family=Prompt:wght@400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #f8fafc;
      --card-bg: #ffffff;
      --border: #e2e8f0;
      --text: #0f172a;
      --text-muted: #64748b;
      --primary: #2563eb;
      --success: #16a34a;
      --warning: #d97706;
      --danger: #dc2626;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Prompt', sans-serif; }
    body {
      background-color: var(--bg);
      color: var(--text);
      min-height: 100vh;
      padding: 16px;
    }
    .container { max-width: 720px; margin: 0 auto; }
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 16px 20px;
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      margin-bottom: 16px;
    }
    .header h1 { font-size: 1.15rem; font-weight: 600; color: var(--text); }
    .wifi-info { font-size: 0.8rem; color: var(--text-muted); margin-top: 2px; }
    .badge {
      font-size: 0.75rem;
      font-weight: 500;
      padding: 4px 10px;
      border-radius: 6px;
      background: #eff6ff;
      color: var(--primary);
      border: 1px solid #bfdbfe;
    }
    .alert-banner {
      display: none;
      padding: 12px 16px;
      background: #fef2f2;
      border: 1px solid #fecaca;
      color: var(--danger);
      border-radius: 8px;
      margin-bottom: 16px;
      font-size: 0.9rem;
      font-weight: 600;
    }
    .grid { display: flex; flex-direction: column; gap: 12px; }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 16px;
    }
    .card-top { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
    .node-title { font-size: 1rem; font-weight: 600; }
    .status-tag {
      font-size: 0.75rem;
      font-weight: 500;
      padding: 3px 8px;
      border-radius: 6px;
    }
    .status-normal { background: #f0fdf4; color: var(--success); border: 1px solid #bbf7d0; }
    .status-warning { background: #fffbeb; color: var(--warning); border: 1px solid #fde68a; }
    .status-critical { background: #fef2f2; color: var(--danger); border: 1px solid #fecaca; }
    .status-offline { background: #f1f5f9; color: var(--text-muted); border: 1px solid #e2e8f0; }
    
    .metrics { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 12px; }
    .metric-box { background: #f8fafc; padding: 10px 14px; border-radius: 8px; border: 1px solid #f1f5f9; }
    .metric-val { font-size: 1.4rem; font-weight: 700; color: var(--text); }
    .metric-lbl { font-size: 0.75rem; color: var(--text-muted); margin-top: 1px; }
    
    .progress-bar-bg {
      background: #e2e8f0;
      height: 6px;
      border-radius: 3px;
      overflow: hidden;
      margin-bottom: 12px;
    }
    .progress-bar-fill {
      height: 100%;
      border-radius: 3px;
      transition: width 0.4s ease;
    }
    .card-footer {
      display: flex;
      justify-content: space-between;
      font-size: 0.75rem;
      color: var(--text-muted);
      border-top: 1px solid var(--border);
      padding-top: 8px;
    }
    .btn {
      width: 100%;
      padding: 10px 16px;
      background: var(--primary);
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 0.9rem;
      font-weight: 500;
      cursor: pointer;
      margin-top: 14px;
    }
    .btn:active { opacity: 0.85; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div>
        <h1>ระบบตรวจเช็คน้ำท่วม (Flood Monitor)</h1>
        <div class="wifi-info" id="wifi-info">WiFi: กำลังเชื่อมต่อ...</div>
      </div>
      <div>
        <span class="badge" id="live-indicator">Online</span>
      </div>
    </div>

    <div class="alert-banner" id="flood-alert">
      แจ้งเตือน: ตรวจพบระดับน้ำท่วมเกินเกณฑ์ความปลอดภัย
    </div>

    <div class="grid" id="nodes-container"></div>

    <button class="btn" onclick="triggerPoll()">ดึงข้อมูลทุก Node ทันที (Manual Poll)</button>
  </div>

  <script>
    async function fetchData() {
      try {
        const res = await fetch('/api/data');
        const data = await res.json();
        renderDashboard(data);
      } catch (err) {
        console.error("API error:", err);
      }
    }

    function renderDashboard(data) {
      document.getElementById('wifi-info').innerText = `WiFi: ${data.wifiSsid} (${data.wifiIp})`;

      const alertBanner = document.getElementById('flood-alert');
      alertBanner.style.display = data.floodAlert ? 'block' : 'none';

      const container = document.getElementById('nodes-container');
      container.innerHTML = '';

      data.nodes.forEach(node => {
        let statusClass = 'status-offline';
        let statusText = 'ออฟไลน์ (Offline)';
        let barColor = '#94a3b8';

        if (node.online) {
          if (node.floodStatus === 2) {
            statusClass = 'status-critical';
            statusText = 'วิกฤตน้ำท่วม (Flood)';
            barColor = 'var(--danger)';
          } else if (node.floodStatus === 1) {
            statusClass = 'status-warning';
            statusText = 'เฝ้าระวัง (Warning)';
            barColor = 'var(--warning)';
          } else {
            statusClass = 'status-normal';
            statusText = 'ปกติ (Normal)';
            barColor = 'var(--success)';
          }
        }

        const card = document.createElement('div');
        card.className = 'card';
        card.innerHTML = `
          <div class="card-top">
            <span class="node-title">${node.name} [Node ${node.id}]</span>
            <span class="status-tag ${statusClass}">${statusText}</span>
          </div>
          <div class="metrics">
            <div class="metric-box">
              <div class="metric-val" style="color: ${barColor}">${node.online ? node.waterLevelCm : '-'} <span style="font-size:0.85rem; font-weight:normal; color:var(--text-muted)">cm</span></div>
              <div class="metric-lbl">ระดับความสูงน้ำ</div>
            </div>
            <div class="metric-box">
              <div class="metric-val">${node.online ? (node.batteryMv / 1000).toFixed(2) : '-'} <span style="font-size:0.85rem; font-weight:normal; color:var(--text-muted)">V</span></div>
              <div class="metric-lbl">แรงดันแบตเตอรี่</div>
            </div>
          </div>
          <div class="progress-bar-bg">
            <div class="progress-bar-fill" style="width: ${node.online ? node.waterPercent : 0}%; background-color: ${barColor}"></div>
          </div>
          <div class="card-footer">
            <span>RSSI: ${node.online ? node.rssi + ' dBm' : '-'} | SNR: ${node.online ? node.snr + ' dB' : '-'}</span>
            <span>อัปเดต: ${node.online ? node.lastSeenSec + ' วินาทีที่แล้ว' : 'ไม่ตอบสนอง'}</span>
          </div>
        `;
        container.appendChild(card);
      });
    }

    async function triggerPoll() {
      try {
        await fetch('/api/poll', { method: 'POST' });
        fetchData();
      } catch (e) {
        console.error(e);
      }
    }

    setInterval(fetchData, 2000);
    fetchData();
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H
