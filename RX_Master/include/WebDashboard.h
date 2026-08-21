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
    
    .metrics { margin-bottom: 12px; }
    .metric-box { background: #f8fafc; padding: 12px 16px; border-radius: 8px; border: 1px solid #f1f5f9; }
    .metric-val { font-size: 1.6rem; font-weight: 700; color: var(--text); }
    .metric-lbl { font-size: 0.8rem; color: var(--text-muted); margin-top: 2px; }
    
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
    
    /* Settings Section */
    .settings-card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 16px;
      margin-top: 14px;
    }
    .settings-header {
      font-size: 0.95rem;
      font-weight: 600;
      color: var(--text);
      margin-bottom: 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .settings-sub {
      font-size: 0.75rem;
      color: var(--text-muted);
      font-weight: normal;
    }
    .settings-list {
      display: flex;
      flex-direction: column;
      gap: 10px;
    }
    .settings-item {
      background: #f8fafc;
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 12px;
    }
    .settings-item-title {
      font-size: 0.85rem;
      font-weight: 600;
      color: var(--text);
      margin-bottom: 8px;
    }
    .settings-row {
      display: flex;
      gap: 8px;
      align-items: flex-end;
    }
    .form-group {
      flex: 1;
    }
    .form-label {
      display: block;
      font-size: 0.75rem;
      color: var(--text-muted);
      margin-bottom: 4px;
    }
    .form-input {
      width: 100%;
      padding: 7px 10px;
      border: 1px solid var(--border);
      border-radius: 6px;
      font-size: 0.85rem;
      background: #ffffff;
      color: var(--text);
    }
    .form-input:focus {
      outline: none;
      border-color: var(--primary);
    }
    .btn-save-sm {
      padding: 7px 14px;
      background: var(--primary);
      color: white;
      border: none;
      border-radius: 6px;
      font-size: 0.8rem;
      font-weight: 500;
      cursor: pointer;
      white-space: nowrap;
      height: 35px;
    }
    .btn-save-sm:active { opacity: 0.85; }

    .btn-group { display: flex; gap: 10px; margin-top: 14px; flex-wrap: wrap; }
    .btn {
      flex: 1;
      min-width: 200px;
      padding: 11px 16px;
      background: var(--primary);
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 0.9rem;
      font-weight: 500;
      cursor: pointer;
      text-align: center;
      transition: opacity 0.2s;
    }
    .btn-secondary {
      background: #0284c7;
    }
    .btn:active { opacity: 0.85; }
    .toast {
      display: none;
      position: fixed;
      bottom: 24px;
      left: 50%;
      transform: translateX(-50%);
      background: #0f172a;
      color: white;
      padding: 10px 20px;
      border-radius: 8px;
      font-size: 0.85rem;
      box-shadow: 0 4px 12px rgba(0,0,0,0.15);
      z-index: 999;
    }
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

    <!-- Threshold Configuration Section -->
    <div class="settings-card">
      <div class="settings-header">
        <span>ตั้งค่าเกณฑ์ระดับน้ำเตือนภัย (Threshold Settings)</span>
        <span class="settings-sub">บันทึกลง Flash NVS อัตโนมัติ</span>
      </div>
      <div class="settings-list" id="settings-container">
        <!-- Generated by JavaScript -->
      </div>
    </div>

    <div class="btn-group">
      <button class="btn" onclick="triggerPoll()">ดึงข้อมูลทุก Node ทันที (Manual Poll)</button>
      <button class="btn btn-secondary" onclick="triggerTelegramTest()">ทดสอบแจ้งเตือน Telegram</button>
    </div>

    <div class="toast" id="toast"></div>
  </div>

  <script>
    let settingsInitialized = false;

    function showToast(msg, duration = 3000) {
      const toast = document.getElementById('toast');
      toast.innerText = msg;
      toast.style.display = 'block';
      setTimeout(() => { toast.style.display = 'none'; }, duration);
    }

    async function fetchData() {
      try {
        const res = await fetch('/api/data');
        const data = await res.json();
        renderDashboard(data);
        if (!settingsInitialized) {
          renderSettings(data.nodes);
          settingsInitialized = true;
        }
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
              <div class="metric-lbl">ระดับความสูงน้ำ (เกณฑ์: เฝ้าระวัง ≥ ${node.warnThresholdCm || 100} cm, วิกฤต ≥ ${node.critThresholdCm || 200} cm)</div>
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

    function renderSettings(nodes) {
      const container = document.getElementById('settings-container');
      container.innerHTML = '';

      nodes.forEach(node => {
        const item = document.createElement('div');
        item.className = 'settings-item';
        item.innerHTML = `
          <div class="settings-item-title">${node.name} (Node ${node.id})</div>
          <div class="settings-row">
            <div class="form-group">
              <label class="form-label" for="warn-${node.id}">เฝ้าระวัง (Warning cm)</label>
              <input type="number" id="warn-${node.id}" class="form-input" value="${node.warnThresholdCm || 100}" min="1" max="999">
            </div>
            <div class="form-group">
              <label class="form-label" for="crit-${node.id}">วิกฤต (Critical cm)</label>
              <input type="number" id="crit-${node.id}" class="form-input" value="${node.critThresholdCm || 200}" min="1" max="999">
            </div>
            <button class="btn-save-sm" onclick="saveThreshold(${node.id})">บันทึก</button>
          </div>
        `;
        container.appendChild(item);
      });
    }

    async function saveThreshold(nodeId) {
      const warnInput = document.getElementById(`warn-${nodeId}`);
      const critInput = document.getElementById(`crit-${nodeId}`);

      const warnVal = parseInt(warnInput.value);
      const critVal = parseInt(critInput.value);

      if (isNaN(warnVal) || isNaN(critVal) || warnVal <= 0 || critVal <= 0) {
        showToast('กรุณากรอกตัวเลขเกณฑ์ระดับน้ำที่ถูกต้อง');
        return;
      }
      if (critVal <= warnVal) {
        showToast('ระดับวิกฤต ต้องมากกว่า ระดับเฝ้าระวัง');
        return;
      }

      try {
        const params = new URLSearchParams();
        params.append('nodeId', nodeId);
        params.append('warnCm', warnVal);
        params.append('critCm', critVal);

        const res = await fetch('/api/settings', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: params.toString()
        });
        const json = await res.json();
        if (json.status === 'ok') {
          showToast(`บันทึกเกณฑ์ Node ${nodeId} ลง Flash NVS สำเร็จ!`);
          fetchData();
        } else {
          showToast('บันทึกไม่สำเร็จ: ' + (json.msg || ''));
        }
      } catch (err) {
        console.error(err);
        showToast('เกิดข้อผิดพลาดในการเชื่อมต่อ');
      }
    }

    async function triggerPoll() {
      try {
        await fetch('/api/poll', { method: 'POST' });
        showToast('กำลังสั่งดึงข้อมูลทุก Node...');
        fetchData();
      } catch (e) {
        console.error(e);
        showToast('เกิดข้อผิดพลาดในการดึงข้อมูล');
      }
    }

    async function triggerTelegramTest() {
      try {
        const res = await fetch('/api/telegram/test', { method: 'POST' });
        const json = await res.json();
        if (json.status === 'ok') {
          showToast('ส่งข้อความทดสอบเข้า Telegram สำเร็จ!');
        } else {
          showToast('ข้อผิดพลาด: ' + (json.msg || 'ไม่สามารถส่งข้อความได้'));
        }
      } catch (e) {
        console.error(e);
        showToast('ไม่สามารถเชื่อมต่อ ESP32 ได้');
      }
    }

    setInterval(fetchData, 2000);
    fetchData();
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H
