#pragma once
#include <atlstr.h>

//本文件内容以完全弃用

// 返回地图+HUD 的内嵌脚本，从主 CPP 中分离长串 JS
inline CStringA BuildHudOverlayJs()
{
	return CStringA(R"JS(
const mapEl = document.getElementById('map'); // 地图元素
const statusEl = document.getElementById('status'); // 状态元素
const hudCanvas = document.getElementById('hud'); // HUD元素
const hudCtx = hudCanvas ? hudCanvas.getContext('2d') : null; // HUD上下文
let hudDpr = window.devicePixelRatio || 1; // HUD缩放比例
let hudState = { pitch:0, roll:0, yaw:0, ias:0, tas:0, alt:0, nx:0, ny:0, nz:1 }; // HUD状态
if (!mapEl) { console.error('Map element not found!'); } else { statusEl.textContent = statusText; }
const tileSize = 256; // 瓦片大小
const minZoom = mapConfig.minZoom ?? 0; // 最小缩放级别
const maxZoom = mapConfig.maxZoom ?? 18; // 最大缩放级别
let zoom = Math.max(minZoom, Math.min(maxZoom, mapConfig.zoom ?? 10)); // 当前缩放级别
let center = { lat: mapConfig.centerLat ?? 0, lng: mapConfig.centerLng ?? 0 }; // 中心坐标
let dragging = false; // 拖拽状态
let dragStart = { x: 0, y: 0 }; // 拖拽起始点
let dragStartCenter = { lat: 0, lng: 0 }; // 拖拽起始中心坐标

function resizeHud() { // 调整HUD大小
  if (!hudCanvas || !hudCtx) return;
  hudDpr = window.devicePixelRatio || 1; // HUD缩放比例
  const w = hudCanvas.clientWidth || hudCanvas.offsetWidth || 360; // HUD宽度
  const h = hudCanvas.clientHeight || hudCanvas.offsetHeight || 260; // HUD高度
  hudCanvas.width = w * hudDpr; // HUD宽度
  hudCanvas.height = h * hudDpr; // HUD高度
  hudCtx.setTransform(hudDpr, 0, 0, hudDpr, 0, 0); // HUD缩放
}

function drawHud() { // 绘制HUD
  if (!hudCanvas || !hudCtx) return;
  const w = hudCanvas.clientWidth || 360;
  const h = hudCanvas.clientHeight || 260;
  hudCtx.clearRect(0, 0, w, h);
  if (!hudState) return;

  const pitch = hudState.pitch ?? 0;
  const roll = hudState.roll ?? 0;
  const yaw = hudState.yaw ?? 0;
  const centerX = w / 2;
  const centerY = h / 2;

  const sky = '#61a8ff';
  const ground = '#c79d59';
  const line = '#ffffff';
  const accent = '#00ff88';

  hudCtx.save();
  hudCtx.translate(centerX, centerY);
  hudCtx.rotate(roll * Math.PI / 180);
  const pitchPxPerDeg = 3.0;
  const pitchOffset = pitch * pitchPxPerDeg;
  hudCtx.translate(0, pitchOffset);

  hudCtx.fillStyle = sky;
  hudCtx.fillRect(-w, -h, w * 2, h);
  hudCtx.fillStyle = ground;
  hudCtx.fillRect(-w, 0, w * 2, h);

  hudCtx.strokeStyle = line;
  hudCtx.lineWidth = 2;
  hudCtx.beginPath();
  hudCtx.moveTo(-w, 0);
  hudCtx.lineTo(w, 0);
  hudCtx.stroke();

  hudCtx.strokeStyle = line;
  hudCtx.lineWidth = 1.5;
  for (let deg = -60; deg <= 60; deg += 10) {
    if (deg === 0) continue;
    const y = -deg * pitchPxPerDeg;
    const len = (deg % 20 === 0) ? 50 : 30;
    hudCtx.beginPath();
    hudCtx.moveTo(-len, y);
    hudCtx.lineTo(len, y);
    hudCtx.stroke();
    hudCtx.fillStyle = line;
    hudCtx.font = '12px Segoe UI,Arial';
    hudCtx.fillText(`${deg}`, len + 6, y + 4);
    hudCtx.fillText(`${deg}`, -len - 26, y + 4);
  }

  hudCtx.restore();

  hudCtx.strokeStyle = line;
  hudCtx.lineWidth = 2;
  hudCtx.beginPath();
  hudCtx.moveTo(centerX - 20, centerY);
  hudCtx.lineTo(centerX - 6, centerY);
  hudCtx.lineTo(centerX, centerY + 6);
  hudCtx.lineTo(centerX + 6, centerY);
  hudCtx.lineTo(centerX + 20, centerY);
  hudCtx.stroke();

  hudCtx.save();
  hudCtx.translate(centerX, centerY - 90);
  hudCtx.strokeStyle = line;
  hudCtx.lineWidth = 1.5;
  hudCtx.beginPath();
  hudCtx.arc(0, 0, 70, Math.PI, 2 * Math.PI);
  hudCtx.stroke();
  for (let deg of [-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60]) {
    const rad = (deg - 90) * Math.PI / 180;
    const r1 = 70;
    const r2 = (deg % 30 === 0) ? 82 : 76;
    hudCtx.beginPath();
    hudCtx.moveTo(r1 * Math.cos(rad), r1 * Math.sin(rad));
    hudCtx.lineTo(r2 * Math.cos(rad), r2 * Math.sin(rad));
    hudCtx.stroke();
  }
  hudCtx.fillStyle = accent;
  hudCtx.beginPath();
  hudCtx.moveTo(0, -90);
  hudCtx.lineTo(-8, -78);
  hudCtx.lineTo(8, -78);
  hudCtx.closePath();
  hudCtx.fill();
  hudCtx.restore();

  hudCtx.save();
  const tapeWidth = w - 40;
  const tapeX = 20;
  const tapeY = 10;
  hudCtx.strokeStyle = line;
  hudCtx.strokeRect(tapeX, tapeY, tapeWidth, 26);
  const heading = ((yaw % 360) + 360) % 360;
  const pxPerDeg = tapeWidth / 120;
  const midX = tapeX + tapeWidth / 2;
  hudCtx.fillStyle = line;
  hudCtx.font = '12px Segoe UI,Arial';
  for (let d = -60; d <= 60; d += 10) {
    const hdg = Math.round((heading + d + 360) % 360);
    const x = midX + d * pxPerDeg;
    hudCtx.beginPath();
    hudCtx.moveTo(x, tapeY + 2);
    hudCtx.lineTo(x, tapeY + ((d % 30 === 0) ? 12 : 8));
    hudCtx.stroke();
    if (d % 30 === 0) {
      const txt = hdg.toString().padStart(3, '0');
      hudCtx.fillText(txt, x - 10, tapeY + 24);
    }
  }
  hudCtx.fillStyle = accent;
  hudCtx.beginPath();
  hudCtx.moveTo(midX, tapeY + 2);
  hudCtx.lineTo(midX - 6, tapeY + 10);
  hudCtx.lineTo(midX + 6, tapeY + 10);
  hudCtx.closePath();
  hudCtx.fill();
  hudCtx.fillStyle = accent;
  hudCtx.font = '14px Segoe UI Semibold,Arial';
  hudCtx.fillText(`HDG ${heading.toFixed(0).padStart(3, '0')}`, tapeX + 8, tapeY + 22);
  hudCtx.restore();

  hudCtx.fillStyle = accent;
  hudCtx.font = '14px Segoe UI,Arial';
  hudCtx.fillText(`IAS ${hudState.ias?.toFixed(1) ?? '-'}`, 12, h - 72);
  hudCtx.fillText(`ALT ${hudState.alt?.toFixed(1) ?? '-'}`, 12, h - 52);
  hudCtx.fillText(`ROLL ${roll.toFixed(1)}`, 12, h - 32);
  hudCtx.fillText(`PITCH ${pitch.toFixed(1)}`, 12, h - 12);
}

window.addEventListener('resize', () => { resizeHud(); drawHud(); });
resizeHud();
drawHud();

if (window.chrome && window.chrome.webview) {
  window.chrome.webview.addEventListener('message', (e) => {
    hudState = e.data || null;
    drawHud();
  });
}

function latLngToPoint(lat, lng, zoomLevel) { // 将经纬度转换为点坐标
  const sin = Math.sin(lat * Math.PI / 180);
  const scale = tileSize * Math.pow(2, zoomLevel);
  const x = (lng + 180) / 360 * scale;
  const y = (0.5 - Math.log((1 + sin) / (1 - sin)) / (4 * Math.PI)) * scale;
  return { x, y };
}

function pointToLatLng(x, y, zoomLevel) { // 将点坐标转换为经纬度
  const scale = tileSize * Math.pow(2, zoomLevel);
  const lng = x / scale * 360 - 180;
  const n = Math.PI - 2 * Math.PI * y / scale;
  const lat = 180 / Math.PI * Math.atan(0.5 * (Math.exp(n) - Math.exp(-n)));
  return { lat, lng };
}
)JS");
}
