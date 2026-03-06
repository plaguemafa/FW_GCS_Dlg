// map.js - 离线地图 + HUD 渲染
// 说明：
// 1) mapConfig / statusText 由 C++ 在 canvas.html 占位符处注入
// 2) C++ 调用 PostWebMessageAsJson 推送 HUD 数据，JS 在 webview message 事件里接收并更新 hudState
// 3) HUD 绘制集中在 drawHud，拆分多个小函数，方便你单独调整配色、位置、尺寸
// 本文件编码变动不需要重新编译，webview本身支持直接加载js
//（C++端数据推送前处理，此处接取数据不做gain）

// DOM 获取
const mapEl = document.getElementById('map');      // 地图容器（瓦片背景）
const statusEl = document.getElementById('status'); // 状态提示
const mouseCoordTipEl = document.getElementById('mouseCoordTip'); // 鼠标位置经纬度提示
const hudCanvas = document.getElementById('hud');   // HUD 画布
const hudCtx = hudCanvas ? hudCanvas.getContext('2d') : null;
let hudDpr = window.devicePixelRatio || 1;          // 设备像素比，用于抗锯齿

// 地图飞机标识Canvas（覆盖在地图上）
let aircraftCanvas = null;
let aircraftCtx = null;

// 底部信息栏Canvas（覆盖在整个主界面上）
let bottomInfoCanvas = null;
let bottomInfoCtx = null;

// 报警弹窗Canvas（覆盖在最顶层）
let alarmCanvas = null;
let alarmCtx = null;

// HUD组2 Canvas（数据参数显示区域）- 第一步：只添加变量声明
let hudGroup2Canvas = null;
let hudGroup2Ctx = null;

// HUD组3 Canvas（开关状态显示区域）- 第一步：只添加变量声明
let hudGroup3Canvas = null;
let hudGroup3Ctx = null;

// 顶层图层中央横幅报警数据（根据 UdpData.h 中的协议定义）
const alarmNames = [
    '\u7535\u6c60\u7535\u538b\u4f4e\u62a5\u8b66',      // 电池电压低报警 (alarmStatus_B0)
    '\u9ad8\u5ea6\u62a5\u8b66',                        // 高度报警 (alarmStatus_B1)
    '\u6cb9\u91cf\u4f4e\u62a5\u8b66',                  // 油量低报警 (alarmStatus_B2)
    '\u8f6c\u901f\u5f02\u5e38\u62a5\u8b66',            // 转速异常报警 (alarmStatus_B3)
    '\u7a7a\u901f\u5f02\u5e38\u62a5\u8b66',            // 空速异常报警 (alarmStatus_B4)
    'GPS\u5b9a\u4f4d\u7cbe\u5ea6\u4f4e\u62a5\u8b66'    // GPS定位精度低报警 (alarmStatus_B5)
];
// activeAlarms: [{ index: number, lastSeen: number }]
let activeAlarms = [];

// 通信丢包中央横幅报警（UDP 连接下超时未收包触发，样式与协议报警一致，居中偏下）
// phase: 'idle' | 'showing' | 'countdown'；countdown 时 10s 后自动关闭或点击关闭
let packetLossAlarm = { show: false, phase: 'idle', countdownEnd: 0, countdownSeconds: 10, intervalId: null };

// HUD 状态（由原生推送的数据结构，可按需扩展）
let hudState = {
    pitch: 0, roll: 0, yaw: 0,     // 姿态
    ias: 0, tas: 0, alt: 0,        // 速度/高度
    mach: 0, aoa: 0, g: 1.0,       // 马赫/攻角/过载
    rpm: 0                          // 发动机转速
};

// HUD层 主界面底部信息栏数据（根据 UdpData.h 中的协议定义）
let hudBottomInfoData = {
    gpsGroundSpeed: 0,
    gpsVerticalSpeed: 0,
    gpsHour: 0,
    gpsMinute: 0,
    gpsSecond: 0
};

// HUD组2数据（根据 UdpData.h 中的协议定义,排序见UdpData.h协议注释，此不重复）
let hudGroup2Data = {
    throttle: 0,              // (1-1) 油门控制
    batteryVoltage: 0,        // (1-2) 电池电压
    fuelRemaining: 0,         // (2-1) 剩余油量
    engineTemp: 0,            // (2-2) 发动机缸温
    satelitesNum: 0,          //  卫星收星数
    gpsStatus: 0,             //  卫星定位状态
    navStatus: 0,             //  导航状态
    targetWaypoint: 0,        //  目标航点
    distanceToGo: 0,          //  待飞距
    crossTrackError: 0,       //  偏航距
    commandHeading: 0,        //  应飞航向
    courseDeviation: 0,       //  偏航角
    commandSpeed: 0,          //  应飞速度
    commandAltitude: 0,       //  应飞高度
    commandTime: 0,           //  应飞时间
    payloadType: 0,           //  载荷类型
    ammoRemaining: 0,         //  剩余弹量
    selfTestResult: 0         //  自检结果
};

// HUD组3数据（根据 UdpData.h 中的协议定义）
let hudGroup3Data = {
    switchStatus_B1: 0,       //  发动机启动状态
    switchStatus_B4: 0,       //  关车状态
    switchStatus_B2: 0,       //  盘旋状态
    switchStatus_B3: 0,       //  归航状态
    switchStatus_B0: 0,       //  发动机并网状态
    switchStatus_B6: 0,       //  开伞状态
    switchStatus_B5: 0,       //  起落架收放状态
    switchStatus_B7: 0        //  夜航灯开关状态
};

// 飞机位置和航向数据（从UDP协议接收）
let aircraftData = {
    longitude: null,   // 经度（度，C++端已缩放）
    latitude: null,    // 纬度（度，C++端已缩放）
    course: null       // 航向角（度，0-359，C++端已缩放）
};

// 目标位置和航向数据（从UDP协议接收）
// 根据 UdpData.h：
//   targetLongitude (65) - 目标经度 (int32_t) - HUD层 地图目标标识位置驱动2
//   targetLatitude (66) - 目标纬度 (int32_t) - HUD层 地图目标标识位置驱动2
//   targetCourse (69) - 目标航向 (int16_t) - HUD层 地图目标标识方向驱动2
let targetData = {
    longitude: null,   // 经度（度，C++端已缩放）
    latitude: null,    // 纬度（度，C++端已缩放）
    course: null       // 航向角（度，0-359，C++端已缩放）
};

// 飞机轨迹数据（用于绘制轨迹连线）
let aircraftTrail = [];
const MAX_TRAIL_POINTS = 1000000;  // 最大轨迹点数
let lastTrailSaveTime = 0;  // 上次保存轨迹点的时间戳（毫秒）
const TRAIL_SAVE_INTERVAL_MS = 500;  // 轨迹点保存间隔（毫秒）

// 鼠标经纬度显示开关（由原生菜单控制）
let mouseCoordEnabled = false;
// 目标点地图选点模式：为 true 时左键点击地图将把该点经纬度回传给 C++，写入 Page2 目标点编辑框
let mapPickTargetEnabled = false;
// 开伞点地图选点模式：为 true 时左键点击地图将把该点经纬度回传给 C++，写入 Page2 开伞点编辑框
let mapPickParachuteEnabled = false;
// 发射点地图选点模式：为 true 时左键点击地图将把该点经纬度回传给 C++，写入 Page2 发射点编辑框
let mapPickLaunchEnabled = false;
// 航路点地图选点模式：为 true 时左键点击地图将把该点经纬度回传给 C++，填充 Page2 航路点列表
let mapPickWaypointEnabled = false;
// 航点连线显示开关：为 true 时在地图上按航点序号连线
let waypointConnectEnabled = false;

// 开伞点选点结果（用户地图单击后存储，用于在地图上绘制伞形图标）
let parachutePoint = { lat: null, lng: null };
// 目标点选点结果（用户地图单击后存储，用于在地图上绘制准星图标）
let targetPickPoint = { lat: null, lng: null };
// 发射点选点结果（用户地图单击后存储，用于在地图上绘制山字箭头图标）
let launchPickPoint = { lat: null, lng: null };
// 航路点选点结果列表（每次点击追加，用于在地图上绘制“数字+外空心圆”图标，序号即航点号）
let waypointPickPoints = [];

function setMouseCoordEnabled(enabled) {
    mouseCoordEnabled = !!enabled;
    if (!mouseCoordTipEl) return;
    mouseCoordTipEl.style.visibility = mouseCoordEnabled ? 'visible' : 'hidden';
}

if (!mapEl) {
    console.error('Map element not found!');
} else {
    statusEl.textContent = statusText; // 注入的状态文本
}

// 瓦片相关（地图背景）
const tileSize = 256;                                // 单张瓦片像素尺寸
const maxZoom = mapConfig.maxZoom ?? 18;             // 全局最大缩放（含局部精细图）
// 缩小下限：硬编码至少为 1，不依赖注入，确保缩小限制一定生效
const FLOOR_ZOOM = 1;
const baseMinZoom = Math.max(FLOOR_ZOOM, mapConfig.baseMinZoom ?? mapConfig.minZoom ?? 0);
const baseMaxZoom = mapConfig.baseMaxZoom ?? maxZoom; // 底图最大层级，无局部精细区域时放大上限
const localBounds = (typeof localMapBounds !== 'undefined' && Array.isArray(localMapBounds)) ? localMapBounds : []; // 局部图范围列表，由 C++ 注入
let zoom = Math.max(baseMinZoom, Math.min(maxZoom, mapConfig.zoom ?? 10)); // 当前缩放（限制在底图最小～全局最大之间）
let center = { lat: mapConfig.centerLat ?? 0, lng: mapConfig.centerLng ?? 0 }; // 当前中心经纬度

// 根据当前视口中心计算有效最大 zoom：无局部精细图覆盖时仅允许放大到底图最大层级
function getEffectiveMaxZoom(lat, lng) {
    let effective = baseMaxZoom;
    for (let i = 0; i < localBounds.length; i++) {
        const b = localBounds[i];
        if (lat >= b.minLat && lat <= b.maxLat && lng >= b.minLng && lng <= b.maxLng) {
            effective = Math.max(effective, b.maxZoom);
        }
    }
    return Math.min(effective, maxZoom);
}

// 鼠标拖拽相关状态
let dragging = false;                                // 是否正在拖动
let dragStart = { x: 0, y: 0 };                      // 鼠标按下时坐标
let dragStartCenter = { lat: 0, lng: 0 };       // 按下时的地图中心

// 适配屏幕尺寸：按 DPR 放大
function resizeHud() {
    if (!hudCanvas || !hudCtx) return;
    hudDpr = window.devicePixelRatio || 1;
    // 读取 CSS 尺寸，按 DPR 放大像素避免模糊
    //HUD画布大小（像素单位）  基于父级容器大小（见html的HUD对象定义），360px*360px
    const w = hudCanvas.clientWidth || hudCanvas.offsetWidth || 360;  
    const h = (hudCanvas.clientHeight || hudCanvas.offsetHeight || 360);
    hudCanvas.width = w * hudDpr;
    hudCanvas.height = h * hudDpr;
    hudCtx.setTransform(hudDpr, 0, 0, hudDpr, 0, 0);
}

// 绘制天空/地面 + 地平线 + 俯仰刻度
// 参数：
//   w/h        : 画布尺寸（像素，已按 DPR 缩放）
//   centerX/Y  : 姿态中心点（地平线居中）
//   pitch/roll : 俯仰/横滚角（度）
//   colors     : 配色对象 {sky, ground, line, text, ringDot, horizon}
function drawBackground(w, h, centerX, centerY, pitch, roll, colors) {
    hudCtx.save();
    hudCtx.translate(centerX, centerY);
    hudCtx.rotate(roll * Math.PI / 180);
    const pitchPxPerDeg = 3.5;               // 刻度间距，单位：像素/度
    const pitchOffset = pitch * pitchPxPerDeg;
    hudCtx.translate(0, pitchOffset);

    // 根据画布尺寸、滚转、俯仰计算背景半宽/半高，使旋转后仍完全覆盖画布（航向仅影响符号，不改变覆盖范围）
    const rollRad = roll * Math.PI / 180;
    const cr = Math.abs(Math.cos(rollRad));
    const sr = Math.abs(Math.sin(rollRad));
    const halfW = w / 2;
    const halfH = h / 2;
    const bgHalfW = halfW * cr + halfH * sr + 2;   // 水平半宽 + 2px 余量
    const bgHalfH = halfW * sr + halfH * cr + Math.abs(pitchOffset) + 2; // 竖直半高 + 2px 余量

    // 填满整个HUD的天空和地面
    hudCtx.fillStyle = colors.sky;
    hudCtx.fillRect(-bgHalfW, -bgHalfH, bgHalfW * 2, bgHalfH);
    hudCtx.fillStyle = colors.ground;
    hudCtx.fillRect(-bgHalfW, 0, bgHalfW * 2, bgHalfH);

    // // 地平线：短绿线  不绘制地平线
    // hudCtx.strokeStyle = colors.horizon;
    // hudCtx.lineWidth = 2;
    // const shortLen = 100;
    // hudCtx.beginPath();
    // hudCtx.moveTo(-shortLen, 0);
    // hudCtx.lineTo(-10, 0);
    // hudCtx.moveTo(10, 0);
    // hudCtx.lineTo(shortLen, 0);
    // hudCtx.stroke();

    // 俯仰刻度
    hudCtx.strokeStyle = colors.line;
    hudCtx.fillStyle = colors.text;
    hudCtx.font = '10px Consolas, monospace';
    hudCtx.lineWidth = 1.5;
    const pitchScaleRangeDeg = 30; // 仅显示当前俯仰±30°范围
    const pitchMinorStep = 2;      // 2n 小刻度
    const pitchMajorStep = 10;     // 10n 大刻度

    // 动态刻度：以当前 pitch 为中心，绘制 [pitch-30, pitch+30]
    const minDeg = Math.floor((pitch - pitchScaleRangeDeg) / pitchMinorStep) * pitchMinorStep;
    const maxDeg = Math.ceil((pitch + pitchScaleRangeDeg) / pitchMinorStep) * pitchMinorStep;

    for (let deg = minDeg; deg <= maxDeg; deg += pitchMinorStep) {
        // if (deg === 0) continue; // 0°（地平线）当前版本不绘制

        const isMajor = (Math.abs(deg) % pitchMajorStep === 0);
        const y = -deg * pitchPxPerDeg;
        const len = isMajor ? 35 : 27; // 长短参考当前版本

        hudCtx.beginPath();
        hudCtx.moveTo(-len, y);
        hudCtx.lineTo(-10, y);
        hudCtx.moveTo(10, y);
        hudCtx.lineTo(len, y);
        hudCtx.stroke();

        if (isMajor) {
            const label = `${deg}`; // 负数自动带 '-'，符合“负刻度带符号”
            hudCtx.textAlign = 'right';
            hudCtx.fillText(label, -len - 4, y + 3);
            hudCtx.textAlign = 'left';
            hudCtx.fillText(label, len + 4, y + 3);
        }
    }

    hudCtx.restore();
}

// 飞机固定符号（居中机翼线）
// 参数：cx, cy 为符号中心；colors.line 用于描边
function drawAircraftSymbol(cx, cy, colors) {
    hudCtx.strokeStyle = colors.line;
    hudCtx.lineWidth = 2;
    hudCtx.beginPath();
    hudCtx.moveTo(cx - 36, cy);
    hudCtx.lineTo(cx - 13, cy);
    hudCtx.lineTo(cx - 8, cy + 8);
    hudCtx.lineTo(cx, cy + 4);
    hudCtx.lineTo(cx + 8, cy + 8);
    hudCtx.lineTo(cx + 13, cy);
    hudCtx.lineTo(cx + 36, cy);
    hudCtx.stroke();

    hudCtx.beginPath();
    hudCtx.arc(cx, cy, 3, 0, Math.PI * 2);
    hudCtx.stroke();
}

// 航向带
// 参数：
//   centerX : 航向带居中 X
//   y       : 顶部 Y 坐标
//   width   : 航向带宽度
//   height  : 航向带高度
//   heading : 当前航向（度，0-359）
//   colors  : 配色
function drawHeadingTape(centerX, y, width, height, heading, colors) {
    // 保存上下文状态，避免被其他绘制影响
    hudCtx.save();
    
    const left = centerX - width / 2;
    hudCtx.strokeStyle = colors.line;
    hudCtx.lineWidth = 1;
    // 只绘制左、右、底三条边，去掉顶部横线
    hudCtx.beginPath();
    hudCtx.moveTo(left, y + height);        // 左下角
    hudCtx.lineTo(left, y + 7);                 // 左上角（但不画顶部）
    hudCtx.moveTo(left + width, y + 7);         // 右上角
    hudCtx.lineTo(left + width, y + height); // 右下角
    hudCtx.moveTo(left, y + height);        // 回到左下角
    hudCtx.lineTo(left + width, y + height); // 底部横线
    hudCtx.stroke();

    const pxPerDeg = width / 90; // 90度范围
    const norm = (deg) => ((Math.round(deg) % 360) + 360) % 360;

    hudCtx.fillStyle = colors.text;
    hudCtx.font = '10px Consolas, monospace';
    hudCtx.textAlign = 'center';
    hudCtx.strokeStyle = colors.line;

    // 1) 每1度小刻度（跳过 5n、10n 位置，由步骤2、3绘制）
    for (let d = -45; d <= 45; d += 1) {
        const actualHdg = norm(heading + d);
        if (actualHdg % 5 === 0) continue;  // 5n、10n 处跳过
        const x = centerX + d * pxPerDeg;
        if (x < left || x > left + width) continue;
        hudCtx.beginPath();
        hudCtx.moveTo(x, y + height);
        hudCtx.lineTo(x, y + height - 2);
        hudCtx.stroke();
    }

    const toD = (degVal) => {
        let d = ((degVal - heading + 360) % 360);
        if (d > 180) d -= 360;
        return d;
    };

    // 2) 5n 中刻度（5、15、25…），跳过 10n，直接计算位置
    for (let n = 0; n < 36; n++) {
        const deg5n = n * 10 + 5;  // 5, 15, 25, ..., 355
        const d = toD(deg5n);
        if (d < -45 || d > 45) continue;
        const x = centerX + d * pxPerDeg;
        if (x < left || x > left + width) continue;
        hudCtx.beginPath();
        hudCtx.moveTo(x, y + height);
        hudCtx.lineTo(x, y + height - 5);  // 中等长度，介于 1° 与 10n 之间
        hudCtx.stroke();
    }

    // 3) 10n 长刻度+数字
    const labelY = y + height - 14;
    for (let n = 0; n < 36; n++) {
        const deg10n = n * 10;
        const d = toD(deg10n);
        if (d < -45 || d > 45) continue;
        const x = centerX + d * pxPerDeg;
        if (x < left || x > left + width) continue;
        hudCtx.beginPath();
        hudCtx.moveTo(x, y + height);
        hudCtx.lineTo(x, y + height - 8);
        hudCtx.stroke();
        hudCtx.fillStyle = colors.text;
        hudCtx.fillText(deg10n.toString().padStart(3, '0'), x, labelY);
    }

    // 指示三角（固定在中心）
    hudCtx.fillStyle = colors.line;  //三角形颜色
    hudCtx.beginPath();
    hudCtx.moveTo(centerX, y + height + 2);
    hudCtx.lineTo(centerX - 5, y + height + 8);
    hudCtx.lineTo(centerX + 5, y + height + 8);
    hudCtx.closePath();
    hudCtx.fill();
    
    // 恢复上下文状态
    hudCtx.restore();
}

// 数值 + 环形点阵（空速 / 高度）
// 参数：
//   x, y     : 圆心
//   value    : 显示的数值字符串
//   label    : 标题文本（IAS / ALT）
//   colors   : 配色（text, ringDot）
//   options  : {radius, dotCount, dotRadius} 可选
function drawValueRing(x, y, value, label, colors, options = {}) {
    const radius = options.radius ?? 34;
    const dotCount = options.dotCount ?? 24;
    const dotRadius = options.dotRadius ?? 2.2;

    // 小白点
    hudCtx.fillStyle = colors.ringDot;
    for (let i = 0; i < dotCount; i++) {
        const t = i / dotCount * Math.PI * 2;
        const dx = Math.cos(t) * radius;
        const dy = Math.sin(t) * radius;
        hudCtx.beginPath();
        hudCtx.arc(x + dx, y + dy, dotRadius, 0, Math.PI * 2);
        hudCtx.fill();
    }

    // 文本
    hudCtx.fillStyle = colors.text;
    hudCtx.font = '10px Consolas, monospace';
    hudCtx.textAlign = 'center';
    hudCtx.fillText(label, x, y - 18);

    hudCtx.font = '14px Consolas, monospace';
    hudCtx.fillText(value, x, y + 5);
}

// 底部文本数据（马赫 / AOA / G / RPM）
// 参数：w/h 画布尺寸；data:{mach,aoa,g,rpm}；colors.text 用于文字
function drawBottomData(w, h, data, colors) {
    hudCtx.fillStyle = colors.text;
    hudCtx.font = '11px Consolas, monospace';
    hudCtx.textAlign = 'left';
    const lx = 40;
    const ly = h - 108;
    hudCtx.fillText(` M  ${data.mach}`, lx, ly);
    hudCtx.fillText(`AoA ${data.aoa}`, lx, ly + 14);
    hudCtx.fillText(` G  ${data.g}`, lx, ly + 28);

    hudCtx.textAlign = 'right';
    const rx = w - 40;
    const ry = h - 90;
    hudCtx.fillText(`RPM ${data.rpm}`, rx, ry);
}

// 滚转刻度半圆（顶部，正切于画布顶部）
// 参数：
//   cx      : 圆心 X（画布中心）
//   topY    : 画布顶部 Y（通常为 0）
//   radius  : 半径（直径 = 360 * 0.95 = 342，半径 = 171）
//   roll    : 当前横滚角（度）
//   colors  : 配色（line）
function drawRollScale(cx, topY, radius, roll, colors) {
    hudCtx.save();
    // 圆心在 (cx, topY + radius)，这样半圆最高点正好在 topY（正切于顶部）
    hudCtx.translate(cx, topY + radius);
    hudCtx.strokeStyle = colors.line;
    hudCtx.lineWidth = 1;

    // 绘制固定的指示三角（始终在HUD顶部中心，不随roll旋转）
    // 三角形固定在顶部中心（deg=0的位置），指向圆心
    hudCtx.fillStyle = colors.line;
    hudCtx.beginPath();
    hudCtx.moveTo(0, -radius + 4);  // 在顶部附近（-radius 是顶部）
    hudCtx.lineTo(-5, -radius + 12);
    hudCtx.lineTo(5, -radius + 12);
    hudCtx.closePath();
    hudCtx.fill();

    // 刻度弧和刻度线：固定在HUD顶部，不随roll旋转
    // 绘制下半圆（从 π 到 2π），圆心在局部坐标 (0, 0)
    hudCtx.beginPath();
    hudCtx.arc(0, 0, radius, 1.07* Math.PI, 0.97*2 * Math.PI); //0.97修正基线长度（仅美观微调）
    hudCtx.stroke();

    // 刻度线：从半圆弧向外延伸
    hudCtx.fillStyle = colors.text;
    hudCtx.font = '10px Consolas, monospace';
    hudCtx.textAlign = 'center';
    
    // 刻度范围：-60到+60度（相对于当前roll角）
    const scaleRange = 60;  // 刻度范围的一半（±60度）
    
    // 计算要显示的滚转角范围（以当前roll为中心）
    const minDisplayRoll = roll - scaleRange;  // 例如：roll=15时，minDisplayRoll=-45
    const maxDisplayRoll = roll + scaleRange;  // 例如：roll=15时，maxDisplayRoll=75
    
    // 计算需要显示的10的倍数范围
    const minDisplayRoll10 = Math.floor(minDisplayRoll / 10) * 10;  // 向下取整到10的倍数
    const maxDisplayRoll10 = Math.ceil(maxDisplayRoll / 10) * 10;   // 向上取整到10的倍数
    
    // 先绘制所有刻度线
    // 基线内圈的小刻度：固定的，每1度一个短刻度
    // 基线外圈的长刻度：根据displayRoll动态显示（10的倍数显示长刻度，5的倍数显示中等刻度）
    for (let deg = -scaleRange; deg <= scaleRange; deg += 1) {
        // 角度转换：0度在顶部，顺时针为正（-90度偏移使0度在顶部）
        const rad = (deg - 90) * Math.PI / 180;
        const r1 = radius - 10;                   // 刻度起点（弧内侧）
        const rBase = radius;                     // 基线位置（半圆弧）
        
        // 计算这个刻度位置对应的显示值（相对于当前roll角度）
        let displayRoll = deg + roll;
        // 归一化到-180到180范围
        while (displayRoll > 180) displayRoll -= 360;
        while (displayRoll < -180) displayRoll += 360;
        
        // 基线内圈：所有位置都画短刻度（固定的，不随roll变化）
        hudCtx.strokeStyle = colors.line;
        hudCtx.beginPath();
        hudCtx.moveTo(r1 * Math.cos(rad), r1 * Math.sin(rad));
        hudCtx.lineTo(rBase * Math.cos(rad), rBase * Math.sin(rad));
        hudCtx.stroke();
        
        // 基线外圈：根据displayRoll的值决定是否画长刻度或中等刻度（动态的，随roll变化）
        // 使用取模运算判断，避免浮点数精度问题
        // 先四舍五入到最近的整数，然后判断是否是5或10的倍数
        const roundedRoll = Math.round(displayRoll);
        const mod10 = Math.abs(roundedRoll) % 10;
        const mod5 = Math.abs(roundedRoll) % 5;
        
        if (mod10 === 0) {
            // displayRoll是10的倍数：画长刻度（用于显示数字的位置）
            hudCtx.beginPath();
            hudCtx.moveTo(rBase * Math.cos(rad), rBase * Math.sin(rad));
            hudCtx.lineTo((rBase + 10) * Math.cos(rad), (rBase + 10) * Math.sin(rad));
            hudCtx.stroke();
        } else if (mod5 === 0) {
            // displayRoll是5的倍数但不是10的倍数：画中等刻度
            hudCtx.beginPath();
            hudCtx.moveTo(rBase * Math.cos(rad), rBase * Math.sin(rad));
            hudCtx.lineTo((rBase + 6) * Math.cos(rad), (rBase + 6) * Math.sin(rad));
            hudCtx.stroke();
        }
    }
    
    // 然后绘制数字标签：遍历所有10的倍数显示值
    for (let displayRoll10 = minDisplayRoll10; displayRoll10 <= maxDisplayRoll10; displayRoll10 += 10) {
        // 计算对应的刻度位置 deg = displayRoll10 - roll
        let deg = displayRoll10 - roll;
        
        // 归一化deg到-scaleRange到scaleRange范围
        while (deg > scaleRange) deg -= 360;
        while (deg < -scaleRange) deg += 360;
        
        // 如果deg在显示范围内，绘制数字
        if (deg >= -scaleRange && deg <= scaleRange) {
            // 角度转换：0度在顶部，顺时针为正（-90度偏移使0度在顶部）
            const rad = (deg - 90) * Math.PI / 180;
            const r2 = radius + 10;  // 数字位置（最长刻度线末端）
            
            hudCtx.fillStyle = colors.text;
            // 归一化显示值到-180到180范围
            let normalizedRoll = displayRoll10;
            while (normalizedRoll > 180) normalizedRoll -= 360;
            while (normalizedRoll < -180) normalizedRoll += 360;
            
            const labelX = (r2 + 10) * Math.cos(rad);
            const labelY = (r2 + 10) * Math.sin(rad);
            // 负值保留 '-' 号（不显示 '+')，避免出现 -0
            const label = (Object.is(normalizedRoll, -0) || normalizedRoll === 0) ? '0' : normalizedRoll.toString();
            hudCtx.fillText(label, labelX, labelY + 4);
        }
    }

    hudCtx.restore();
}

function drawHud() {
    if (!hudCanvas || !hudCtx) return;
    const w = hudCanvas.clientWidth || 360; 
    const h = (hudCanvas.clientHeight || 360); // 与 resizeHud 保持一致
    hudCtx.clearRect(0, 0, w, h);
    if (!hudState) return;

    const pitch = hudState.pitch ?? 0;
    const roll = hudState.roll ?? 0;
    const yaw = hudState.yaw ?? 0;
    const ias = hudState.ias ?? 0;
    const alt = hudState.alt ?? 0;
    const mach = hudState.mach ?? 0;
    const aoa = hudState.aoa ?? 0;
    const g = hudState.g ?? 1.0;
    const rpm = hudState.rpm ?? 0;

    const centerX = w / 2;
    // 布局：地平线居中；滚转在顶部；航向带靠底部
    const centerY = h / 2;      // 姿态中心（地平线在此高度）
    const rollY = 27;            // 滚转刻度顶部Y（画布顶部，正切）
    const rollRadius = w * 0.95 / 2; // 半径 = 直径(360*0.95) / 2 = 171
    const headingY = h - 34;    // 底部航向带 Y（避免与姿态区重叠）

    const colors = {
        // 典型PFD配色：亮蓝天空、土褐地面；线条/文字白，地平线短绿线
        sky: '#5fa8d3',
        ground: '#8b5a2b',
        line: '#f8f8f8',
        text: '#f8f8f8',
        ringDot: '#ffffff',
        horizon: '#6bff6b'
    };

    // 背景（天空/地面）填满整个HUD
    drawBackground(w, h, centerX, centerY, pitch, roll, colors);

    // 滚转刻度（顶部，正切于画布顶部，直径 = 360 * 0.95 = 342）
    drawRollScale(centerX, rollY, rollRadius, roll, colors);

    // 航向带（靠底部上方）
    drawHeadingTape(centerX, headingY, 240, 22, ((yaw % 360) + 360) % 360, colors);

    // 飞机符号
    drawAircraftSymbol(centerX, centerY, colors);

    // 左右数值环
    drawValueRing(60, centerY, Math.round(ias).toString(), 'IAS', colors);
    drawValueRing(w - 60, centerY, Math.round(alt).toString(), 'ALT', colors);

    // 底部数据
    drawBottomData(w, h, {
        mach: mach.toFixed(2),
        aoa: aoa.toFixed(1),
        g: g.toFixed(1),
        rpm: Math.round(rpm)
    }, colors);
    
    // 绘制底部信息栏（地速、垂直速度、GPS时间、累计距离）- 在整个主界面底部
    const convertHudBottomInfoData = (rawData) => {
        return {
            groundSpeed: rawData.gpsGroundSpeed || 0,
            verticalSpeed: rawData.gpsVerticalSpeed || 0,
            gpsHour: rawData.gpsHour || 0,
            gpsMinute: rawData.gpsMinute || 0,
            gpsSecond: rawData.gpsSecond || 0
        };
    };
    const bottomInfoData = convertHudBottomInfoData(hudBottomInfoData);
    drawBottomInfoBar(window.innerWidth, window.innerHeight, bottomInfoData, colors);
    
    // 第五步：绘制HUD组2和HUD组3（只绘制背景和边框）
    drawHudGroup2();
    drawHudGroup3();
}

// 第三步：初始化HUD组2 Canvas（只获取元素和context，不绘制）
function initHudGroup2Canvas() {
    if (!hudGroup2Canvas) {
        hudGroup2Canvas = document.getElementById('hudGroup2');
        if (hudGroup2Canvas) {
            hudGroup2Ctx = hudGroup2Canvas.getContext('2d');
            resizeHudGroup2Canvas();  // 第四步：初始化时调整尺寸
        }
    }
}

// 第三步：初始化HUD组3 Canvas（只获取元素和context，不绘制）
function initHudGroup3Canvas() {
    if (!hudGroup3Canvas) {
        hudGroup3Canvas = document.getElementById('hudGroup3');
        if (hudGroup3Canvas) {
            hudGroup3Ctx = hudGroup3Canvas.getContext('2d');
            resizeHudGroup3Canvas();  // 第四步：初始化时调整尺寸
        }
    }
}

// 第四步：调整HUD组2 Canvas尺寸（按DPR缩放，避免模糊）
function resizeHudGroup2Canvas() {
    if (!hudGroup2Canvas || !hudGroup2Ctx) return;
    const dpr = window.devicePixelRatio || 1;
    const w = hudGroup2Canvas.clientWidth || 600;
    const h = hudGroup2Canvas.clientHeight || 360;
    hudGroup2Canvas.width = w * dpr;
    hudGroup2Canvas.height = h * dpr;
    hudGroup2Ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}

// 第四步：调整HUD组3 Canvas尺寸（按DPR缩放，避免模糊）
function resizeHudGroup3Canvas() {
    if (!hudGroup3Canvas || !hudGroup3Ctx) return;
    const dpr = window.devicePixelRatio || 1;
    const w = hudGroup3Canvas.clientWidth || 600;
    const h = hudGroup3Canvas.clientHeight || 200;
    hudGroup3Canvas.width = w * dpr;
    hudGroup3Canvas.height = h * dpr;
    hudGroup3Ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}

// 第五步：绘制HUD组2（只绘制背景和边框，不绘制复杂内容）
// 第六步：添加网格布局和标签
function drawHudGroup2() {
    if (!hudGroup2Canvas || !hudGroup2Ctx) {
        initHudGroup2Canvas();
        if (!hudGroup2Ctx) return;
    }
    
    resizeHudGroup2Canvas();
    
    const w = hudGroup2Canvas.clientWidth || 600;
    const h = hudGroup2Canvas.clientHeight || 360;
    hudGroup2Ctx.clearRect(0, 0, w, h);
    
    // 背景
    hudGroup2Ctx.fillStyle = 'rgba(50, 50, 50, 0.8)';
    hudGroup2Ctx.fillRect(0, 0, w, h);
    
    // 边框
    hudGroup2Ctx.strokeStyle = 'rgba(200, 200, 200, 0.5)';
    hudGroup2Ctx.lineWidth = 1;
    hudGroup2Ctx.strokeRect(0, 0, w, h);
    
    // 第六步：网格布局：9行2列（根据UdpData.h协议）
    const rows = 9;
    const cols = 2;
    const cellWidth = (w - 20) / cols;  // 留出左右边距
    const cellHeight = (h - 20) / rows; // 留出上下边距
    const startX = 10;
    const startY = 10;
    
    // HUD组2数据标签和数据字段映射（根据UdpData.h协议注释）- 使用Unicode转义序列
    // 格式：9行2列的二维数组，[行][列] = [标签, 数据字段]
    // 协议格式(n-m)表示第n行第m列
    const dataFields = [
        // 第1行
        [
            ['\u6cb9\u95e8', 'throttle'],        // (1-1) 油门控制
            ['\u7535\u6c60\u7535\u538b', 'batteryVoltage']  // (1-2) 电池电压
        ],
        // 第2行
        [
            ['\u6cb9\u91cf', 'fuelRemaining'],  // (2-1) 剩余油量
            ['\u53d1\u52a8\u673a\u7f38\u6e29', 'engineTemp']  // (2-2) 发动机缸温
        ],
        // 第3行
        [
            ['\u536b\u661f\u6570\u76ee', 'satelitesNum'],  // (3-1) 卫星收星数
            ['\u5b9a\u4f4d\u7cbe\u5ea6', 'gpsStatus']       // (3-2) 卫星定位状态
        ],
        // 第4行
        [
            ['\u5bfc\u822a\u72b6\u6001', 'navStatus'],      // (4-1) 导航状态
            ['\u76ee\u6807\u822a\u70b9', 'targetWaypoint']  // (4-2) 目标航点
        ],
        // 第5行
        [
            ['\u5f85\u98de\u8ddd', 'distanceToGo'],         // (5-1) 待飞距
            ['\u504f\u822a\u8ddd', 'crossTrackError']       // (5-2) 偏航距
        ],
        // 第6行
        [
            ['\u5e94\u98de\u822a\u5411', 'commandHeading'], // (6-1) 应飞航向
            ['\u504f\u822a\u89d2', 'courseDeviation']       // (6-2) 偏航角
        ],
        // 第7行
        [
            ['\u5e94\u98de\u901f\u5ea6', 'commandSpeed'],   // (7-1) 应飞速度
            ['\u5e94\u98de\u9ad8\u5ea6', 'commandAltitude'] // (7-2) 应飞高度
        ],
        // 第8行
        [
            ['\u5e94\u98de\u65f6\u95f4', 'commandTime'],    // (8-1) 应飞时间
            ['\u8f7d\u8377\u7c7b\u578b', 'payloadType']      // (8-2) 载荷类型
        ],
        // 第9行
        [
            ['\u5269\u4f59\u5f39\u91cf', 'ammoRemaining'],  // (9-1) 剩余弹量
            ['\u81ea\u68c0\u7ed3\u679c', 'selfTestResult']  // (9-2) 自检结果
        ]
    ];
    //HUD组2的字体大小配置
    hudGroup2Ctx.font = '14px Consolas, monospace';
    hudGroup2Ctx.textAlign = 'left';
    hudGroup2Ctx.textBaseline = 'middle';
    
    for (let row = 0; row < rows; row++) {
        for (let col = 0; col < cols; col++) {
            const [label, field] = dataFields[row][col];
            
            // 如果该位置有数据，则绘制
            if (label && field) {
                const x = startX + col * cellWidth;
                const y = startY + row * cellHeight;
                
                // 标签背景（浅灰色）
                hudGroup2Ctx.fillStyle = 'rgba(150, 150, 150, 0.6)';
                hudGroup2Ctx.fillRect(x, y, cellWidth * 0.4, cellHeight);
                
                // 标签文字
                hudGroup2Ctx.fillStyle = '#000000';
                hudGroup2Ctx.fillText(label, x + 5, y + cellHeight / 2);
                
                // 数据显示框（绿色矩形条）
                const dataBoxX = x + cellWidth * 0.4 + 2;
                const dataBoxWidth = cellWidth * 0.6 - 2;
                hudGroup2Ctx.fillStyle = '#00ff00';  // 绿色
                hudGroup2Ctx.fillRect(dataBoxX, y + 2, dataBoxWidth, cellHeight - 4);
                
                // 数据显示框边框
                hudGroup2Ctx.strokeStyle = 'rgba(255, 255, 255, 0.3)';
                hudGroup2Ctx.lineWidth = 1;
                hudGroup2Ctx.strokeRect(dataBoxX, y + 2, dataBoxWidth, cellHeight - 4);
                
                // 数据值（从hudGroup2Data读取）
                const value = hudGroup2Data[field] !== undefined ? hudGroup2Data[field] : null;
                hudGroup2Ctx.fillStyle = '#000000';
                hudGroup2Ctx.textAlign = 'center';
                const displayText = value !== null ? String(value) : '---';
                hudGroup2Ctx.fillText(displayText, dataBoxX + dataBoxWidth / 2, y + cellHeight / 2);
                hudGroup2Ctx.textAlign = 'left';
            }
        }
    }
}

// 第五步：绘制HUD组3（只绘制背景和边框，不绘制复杂内容）
// 第六步：添加网格布局和标签
function drawHudGroup3() {
    if (!hudGroup3Canvas || !hudGroup3Ctx) {
        initHudGroup3Canvas();
        if (!hudGroup3Ctx) return;
    }
    
    resizeHudGroup3Canvas();
    
    const w = hudGroup3Canvas.clientWidth || 600;
    const h = hudGroup3Canvas.clientHeight || 200;
    hudGroup3Ctx.clearRect(0, 0, w, h);
    
    // 背景
    hudGroup3Ctx.fillStyle = 'rgba(50, 50, 50, 0.8)';
    hudGroup3Ctx.fillRect(0, 0, w, h);
    
    // 边框
    hudGroup3Ctx.strokeStyle = 'rgba(200, 200, 200, 0.5)';
    hudGroup3Ctx.lineWidth = 1;
    hudGroup3Ctx.strokeRect(0, 0, w, h);
    
    // 第六步：网格布局：4行2列（根据UdpData.h协议，HUD组3只有8个字段）
    const rows = 4;
    const cols = 2;
    const cellWidth = (w - 20) / cols;
    const cellHeight = (h - 20) / rows;
    const startX = 10;
    const startY = 10;
    
    // HUD组3按钮标签和数据字段映射（根据UdpData.h协议注释）- 使用Unicode转义序列
    // 格式：4行2列的二维数组，[行][列] = [标签, 数据字段]
    // 协议格式(n-m)表示第n行第m列
    const dataFields = [
        // 第1行
        [
            ['\u53d1\u52a8\u673a\u542f\u52a8', 'switchStatus_B1'],  // (1-1) 发动机启动状态
            ['\u5173\u8f66', 'switchStatus_B4']                     // (1-2) 关车状态
        ],
        // 第2行
        [
            ['\u76f8\u65cb', 'switchStatus_B2'],                    // (2-1) 盘旋状态
            ['\u5f52\u822a', 'switchStatus_B3']                     // (2-2) 归航状态
        ],
        // 第3行
        [
            ['\u53d1\u52a8\u673a\u5e76\u7f51', 'switchStatus_B0'],  // (3-1) 发动机并网状态
            ['\u5f00\u4f1e', 'switchStatus_B6']                     // (3-2) 开伞状态
        ],
        // 第4行
        [
            ['\u8d77\u843d\u67b6', 'switchStatus_B5'],              // (4-1) 起落架收放状态
            ['\u591c\u822a\u706f', 'switchStatus_B7']               // (4-2) 夜航灯开关状态
        ]
    ];
    //HUD组3的字体大小配置
    hudGroup3Ctx.font = '14px Consolas, monospace';
    hudGroup3Ctx.textAlign = 'center';
    hudGroup3Ctx.textBaseline = 'middle';
    
    for (let row = 0; row < rows; row++) {
        for (let col = 0; col < cols; col++) {
            const [label, field] = dataFields[row][col];
            
            // 如果该位置有数据，则绘制
            if (label && field) {
                const x = startX + col * cellWidth;
                const y = startY + row * cellHeight;
                const value = hudGroup3Data[field] !== undefined ? hudGroup3Data[field] : 0;
                const isActive = value === 1;
                
                // 按钮背景（激活时亮色，未激活时浅灰色）
                hudGroup3Ctx.fillStyle = isActive ? 'rgba(100, 200, 100, 0.8)' : 'rgba(150, 150, 150, 0.6)';
                hudGroup3Ctx.fillRect(x + 2, y + 2, cellWidth - 4, cellHeight - 4);
                
                // 按钮边框
                hudGroup3Ctx.strokeStyle = 'rgba(255, 255, 255, 0.5)';
                hudGroup3Ctx.lineWidth = 1;
                hudGroup3Ctx.strokeRect(x + 2, y + 2, cellWidth - 4, cellHeight - 4);
                
                // 按钮文字
                hudGroup3Ctx.fillStyle = '#000000';
                hudGroup3Ctx.fillText(label, x + cellWidth / 2, y + cellHeight / 2);
            }
        }
    }
}

// 初始化 HUD 尺寸并绘制一次（窗口尺寸变化时也重绘）
resizeHud();
drawHud();

// 接收 native HUD / 控制 数据（C++ PostWebMessageAsJson 推送）
if (window.chrome && window.chrome.webview) {
    window.chrome.webview.addEventListener('message', (e) => {
        const receivedData = e.data || null;
        if (!receivedData) {
            return;
        }

        // 通信丢包报警：C++ 检测到平均 10 倍间隔未收包时显示
        if (receivedData.command === 'showPacketLossAlarm') {
            if (packetLossAlarm.intervalId) clearInterval(packetLossAlarm.intervalId);
            packetLossAlarm.show = true;
            packetLossAlarm.phase = 'showing';
            packetLossAlarm.intervalId = null;
            drawAlarmPopups();
            return;
        }
        if (receivedData.command === 'packetLossAlarmRecovered') {
            packetLossAlarm.phase = 'countdown';
            packetLossAlarm.countdownSeconds = receivedData.countdownSeconds || 10;
            packetLossAlarm.countdownEnd = Date.now() + packetLossAlarm.countdownSeconds * 1000;
            if (packetLossAlarm.intervalId) clearInterval(packetLossAlarm.intervalId);
            packetLossAlarm.intervalId = setInterval(() => {
                if (Date.now() >= packetLossAlarm.countdownEnd) {
                    clearInterval(packetLossAlarm.intervalId);
                    packetLossAlarm.intervalId = null;
                    packetLossAlarm.show = false;
                    packetLossAlarm.phase = 'idle';
                }
                drawAlarmPopups();
            }, 500);
            drawAlarmPopups();
            return;
        }
        if (receivedData.command === 'hidePacketLossAlarm') {
            if (packetLossAlarm.intervalId) clearInterval(packetLossAlarm.intervalId);
            packetLossAlarm.intervalId = null;
            packetLossAlarm.show = false;
            packetLossAlarm.phase = 'idle';
            drawAlarmPopups();
            return;
        }

        // 处理 UDP 断开后清除所有 HUD/地图显示数据为 0
        if (receivedData.command === 'clearHudData') {
            hudState = { pitch: 0, roll: 0, yaw: 0, ias: 0, tas: 0, alt: 0, mach: 0, aoa: 0, g: 1.0, rpm: 0 };
            hudBottomInfoData = { gpsGroundSpeed: 0, gpsVerticalSpeed: 0, gpsHour: 0, gpsMinute: 0, gpsSecond: 0 };
            hudGroup2Data = { throttle: 0, batteryVoltage: 0, fuelRemaining: 0, engineTemp: 0, satelitesNum: 0, gpsStatus: 0, navStatus: 0, targetWaypoint: 0, distanceToGo: 0, crossTrackError: 0, commandHeading: 0, courseDeviation: 0, commandSpeed: 0, commandAltitude: 0, commandTime: 0, payloadType: 0, ammoRemaining: 0, selfTestResult: 0 };
            hudGroup3Data = { switchStatus_B1: 0, switchStatus_B4: 0, switchStatus_B2: 0, switchStatus_B3: 0, switchStatus_B0: 0, switchStatus_B6: 0, switchStatus_B5: 0, switchStatus_B7: 0 };
            aircraftData = { longitude: null, latitude: null, course: null };
            targetData = { longitude: null, latitude: null, course: null };
            aircraftTrail = [];
            activeAlarms = [];
            if (packetLossAlarm.intervalId) clearInterval(packetLossAlarm.intervalId);
            packetLossAlarm.intervalId = null;
            packetLossAlarm.show = false;
            packetLossAlarm.phase = 'idle';
            drawHud();
            if (hudGroup2Canvas && hudGroup2Ctx) drawHudGroup2();
            if (hudGroup3Canvas && hudGroup3Ctx) drawHudGroup3();
            if (bottomInfoCanvas && bottomInfoCtx) drawBottomInfoBar(window.innerWidth, window.innerHeight, { groundSpeed: 0, verticalSpeed: 0, gpsHour: 0, gpsMinute: 0, gpsSecond: 0 }, {});
            if (aircraftCanvas && aircraftCtx) {
                drawAircraftOnMap(aircraftData.latitude, aircraftData.longitude, aircraftData.course);
                drawTargetOnMap(targetData.latitude, targetData.longitude, targetData.course);
                drawParachuteOnMap(parachutePoint.lat, parachutePoint.lng);
                drawTargetPickOnMap(targetPickPoint.lat, targetPickPoint.lng);
                drawLaunchPickOnMap(launchPickPoint.lat, launchPickPoint.lng);
                drawWaypointPickOnMap();
                drawWaypointConnectOnMap();
            }
            drawAlarmPopups();
            return;
        }

        // 处理控制类消息（例如：鼠标经纬度开关、目标点/航路点选点模式）
        if (receivedData.command === 'setMouseCoord') {
            setMouseCoordEnabled(!!receivedData.enabled);
            return;
        }
        if (receivedData.command === 'setMapPickTarget') {
            mapPickTargetEnabled = !!receivedData.enabled;
            if (mapEl) mapEl.style.cursor = (mapPickTargetEnabled || mapPickParachuteEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
            return;
        }
        if (receivedData.command === 'setMapPickParachute') {
            mapPickParachuteEnabled = !!receivedData.enabled;
            if (mapEl) mapEl.style.cursor = (mapPickTargetEnabled || mapPickParachuteEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
            return;
        }
        if (receivedData.command === 'setMapPickLaunch') {
            mapPickLaunchEnabled = !!receivedData.enabled;
            if (mapEl) mapEl.style.cursor = (mapPickTargetEnabled || mapPickParachuteEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
            return;
        }
        if (receivedData.command === 'setMapPickWaypoint') {
            mapPickWaypointEnabled = !!receivedData.enabled;
            // enabled=true 时，默认清空本地航点标识；但若 keepExisting=true（例如“否：继承历史继续选点”），则保留现有航点标识与编号
            const keepExisting = !!receivedData.keepExisting;
            if (receivedData.enabled && !keepExisting) waypointPickPoints = [];
            if (mapEl) mapEl.style.cursor = (mapPickTargetEnabled || mapPickParachuteEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
            return;
        }
        if (receivedData.command === 'setWaypointConnectVisible') {
            waypointConnectEnabled = !!receivedData.enabled;
            if (aircraftCanvas && aircraftCtx) {
                drawAircraftOnMap(aircraftData.latitude, aircraftData.longitude, aircraftData.course);
                drawTargetOnMap(targetData.latitude, targetData.longitude, targetData.course);
                drawParachuteOnMap(parachutePoint.lat, parachutePoint.lng);
                drawTargetPickOnMap(targetPickPoint.lat, targetPickPoint.lng);
                drawLaunchPickOnMap(launchPickPoint.lat, launchPickPoint.lng);
                drawWaypointPickOnMap();
            }
            return;
        }

        // 其余视为 HUD / 地图数据
        hudState = receivedData;
        
        // 更新底部信息栏数据
        if (receivedData.gpsGroundSpeed !== undefined) {
            hudBottomInfoData.gpsGroundSpeed = receivedData.gpsGroundSpeed;
        }
        if (receivedData.gpsVerticalSpeed !== undefined) {
            hudBottomInfoData.gpsVerticalSpeed = receivedData.gpsVerticalSpeed;
        }
        if (receivedData.gpsHour !== undefined) {
            hudBottomInfoData.gpsHour = receivedData.gpsHour;
        }
        if (receivedData.gpsMinute !== undefined) {
            hudBottomInfoData.gpsMinute = receivedData.gpsMinute;
        }
        if (receivedData.gpsSecond !== undefined) {
            hudBottomInfoData.gpsSecond = receivedData.gpsSecond;
        }

        // 更新HUD组2数据（根据UdpData.h协议）
        if (receivedData.throttle !== undefined) hudGroup2Data.throttle = receivedData.throttle;
        if (receivedData.batteryVoltage !== undefined) hudGroup2Data.batteryVoltage = receivedData.batteryVoltage;
        if (receivedData.fuelRemaining !== undefined) hudGroup2Data.fuelRemaining = receivedData.fuelRemaining;
        if (receivedData.engineTemp !== undefined) hudGroup2Data.engineTemp = receivedData.engineTemp;
        if (receivedData.satelitesNum !== undefined) hudGroup2Data.satelitesNum = receivedData.satelitesNum;
        if (receivedData.gpsStatus !== undefined) hudGroup2Data.gpsStatus = receivedData.gpsStatus;
        if (receivedData.navStatus !== undefined) hudGroup2Data.navStatus = receivedData.navStatus;
        if (receivedData.targetWaypoint !== undefined) hudGroup2Data.targetWaypoint = receivedData.targetWaypoint;
        if (receivedData.distanceToGo !== undefined) hudGroup2Data.distanceToGo = receivedData.distanceToGo;
        if (receivedData.crossTrackError !== undefined) hudGroup2Data.crossTrackError = receivedData.crossTrackError;
        if (receivedData.commandHeading !== undefined) hudGroup2Data.commandHeading = receivedData.commandHeading;
        if (receivedData.courseDeviation !== undefined) hudGroup2Data.courseDeviation = receivedData.courseDeviation;
        if (receivedData.commandSpeed !== undefined) hudGroup2Data.commandSpeed = receivedData.commandSpeed;
        if (receivedData.commandAltitude !== undefined) hudGroup2Data.commandAltitude = receivedData.commandAltitude;
        if (receivedData.commandTime !== undefined) hudGroup2Data.commandTime = receivedData.commandTime;
        if (receivedData.payloadType !== undefined) hudGroup2Data.payloadType = receivedData.payloadType;
        if (receivedData.ammoRemaining !== undefined) hudGroup2Data.ammoRemaining = receivedData.ammoRemaining;
        if (receivedData.selfTestResult !== undefined) hudGroup2Data.selfTestResult = receivedData.selfTestResult;

        // 更新HUD组3数据（根据UdpData.h协议）
        if (receivedData.switchStatus_B1 !== undefined) hudGroup3Data.switchStatus_B1 = receivedData.switchStatus_B1;
        if (receivedData.switchStatus_B4 !== undefined) hudGroup3Data.switchStatus_B4 = receivedData.switchStatus_B4;
        if (receivedData.switchStatus_B2 !== undefined) hudGroup3Data.switchStatus_B2 = receivedData.switchStatus_B2;
        if (receivedData.switchStatus_B3 !== undefined) hudGroup3Data.switchStatus_B3 = receivedData.switchStatus_B3;
        if (receivedData.switchStatus_B0 !== undefined) hudGroup3Data.switchStatus_B0 = receivedData.switchStatus_B0;
        if (receivedData.switchStatus_B6 !== undefined) hudGroup3Data.switchStatus_B6 = receivedData.switchStatus_B6;
        if (receivedData.switchStatus_B5 !== undefined) hudGroup3Data.switchStatus_B5 = receivedData.switchStatus_B5;
        if (receivedData.switchStatus_B7 !== undefined) hudGroup3Data.switchStatus_B7 = receivedData.switchStatus_B7;

        // 处理报警数据（顶层图层中央横幅报警）
        const alarmFields = [
            'alarmStatus_B0',
            'alarmStatus_B1',
            'alarmStatus_B2',
            'alarmStatus_B3',
            'alarmStatus_B4',
            'alarmStatus_B5'
        ];

        const now = Date.now();
        for (let i = 0; i < alarmFields.length; i++) {
            const fieldName = alarmFields[i];
            if (receivedData[fieldName] === 1) {
                const existing = activeAlarms.find(a => a.index === i);
                if (existing) {
                    // 弹出期间始终收到1，刷新倒计时
                    existing.lastSeen = now;
                } else {
                    // 收到1则新增弹窗
                    activeAlarms.push({ index: i, lastSeen: now });
                }
            }
            // 收到0不处理（不关闭）
        }

        drawAlarmPopups();
        
        // 更新飞机位置和航向数据（地图飞机标识）
        if (receivedData.longitude !== undefined) {
            aircraftData.longitude = receivedData.longitude;
        }
        if (receivedData.latitude !== undefined) {
            aircraftData.latitude = receivedData.latitude;
        }
        // 航向角：使用姿态航向 yaw（与HUD航向带一致），若无则回退到0
        if (receivedData.yaw !== undefined && !isNaN(receivedData.yaw)) {
            aircraftData.course = ((receivedData.yaw % 360) + 360) % 360;
        } else {aircraftData.course = 0.0;}
        
        // 更新目标位置和航向数据（地图目标标识）
        if (receivedData.targetLongitude !== undefined) {
            targetData.longitude = receivedData.targetLongitude;
        }
        if (receivedData.targetLatitude !== undefined) {
            targetData.latitude = receivedData.targetLatitude;
        }
        if (receivedData.targetCourse !== undefined) {
            targetData.course = receivedData.targetCourse;
            // 归一化到0-359度范围
            targetData.course = ((targetData.course % 360) + 360) % 360;
        }
        
        // 重新绘制 HUD
        drawHud();
        
        // 确保飞机标识Canvas存在（如果不存在则初始化）
        if (!aircraftCanvas && mapEl) {
            initAircraftCanvas();
        }
        
        // 重新绘制飞机标识和目标标识
        if (aircraftCanvas && aircraftCtx) {
            drawAircraftOnMap(aircraftData.latitude, aircraftData.longitude, aircraftData.course);
            drawTargetOnMap(targetData.latitude, targetData.longitude, targetData.course);
            drawParachuteOnMap(parachutePoint.lat, parachutePoint.lng);
            drawTargetPickOnMap(targetPickPoint.lat, targetPickPoint.lng);
            drawLaunchPickOnMap(launchPickPoint.lat, launchPickPoint.lng);
            drawWaypointPickOnMap();
            drawWaypointConnectOnMap();
        }
    });
}

// 经纬度 -> Web Mercator 像素坐标（用于瓦片定位）
function latLngToPoint(lat, lng, zoomLevel) {
    const sin = Math.sin(lat * Math.PI / 180);
    const scale = tileSize * Math.pow(2, zoomLevel);
    const x = (lng + 180) / 360 * scale;
    const y = (0.5 - Math.log((1 + sin) / (1 - sin)) / (4 * Math.PI)) * scale;
    return { x, y };
}

// Web Mercator 像素坐标 -> 经纬度（鼠标缩放回推中心）
function pointToLatLng(x, y, zoomLevel) {
    const scale = tileSize * Math.pow(2, zoomLevel);
    const lng = x / scale * 360 - 180;
    const n = Math.PI - 2 * Math.PI * y / scale;
    const lat = 180 / Math.PI * Math.atan(0.5 * (Math.exp(n) - Math.exp(-n)));
    return { lat, lng };
}

// 瓦片渲染：根据中心点和缩放计算需要的瓦片并加载
function render() {
    if (!mapEl) return;
    try {
        // 每次渲染前强制限制 zoom，下限用 FLOOR_ZOOM 与 baseMinZoom 的较大值，确保缩小限制一定生效
        const effectiveMax = getEffectiveMaxZoom(center.lat, center.lng);
        const minZoom = Math.max(FLOOR_ZOOM, baseMinZoom);
        zoom = Math.max(minZoom, Math.min(effectiveMax, zoom));

        const width = mapEl.clientWidth;
        const height = mapEl.clientHeight;
        if (width === 0 || height === 0) return;
        const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
        const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
        const startX = Math.floor(topLeft.x / tileSize);
        const startY = Math.floor(topLeft.y / tileSize);
        const endX = Math.floor((topLeft.x + width) / tileSize);
        const endY = Math.floor((topLeft.y + height) / tileSize);
        const max = Math.pow(2, zoom);

        // 清空旧瓦片（只清除img元素，保留Canvas）
        const tiles = mapEl.querySelectorAll('.tile');
        tiles.forEach(tile => tile.remove());
        
        // 确保飞机标识Canvas存在（如果不存在则初始化）
        if (!aircraftCanvas && mapEl) {
            initAircraftCanvas();
        }
        
        for (let ty = startY; ty <= endY; ty++) {
            for (let tx = startX; tx <= endX; tx++) {
                const tileX = ((tx % max) + max) % max;
                const tileY = ((ty % max) + max) % max;
                const img = new Image();
                img.className = 'tile';
                img.style.left = (tx * tileSize - topLeft.x) + 'px';
                img.style.top = (ty * tileSize - topLeft.y) + 'px';
                img.src = mapConfig.tileUrl
                    .replace('{z}', zoom)
                    .replace('{x}', tileX)
                    .replace('{y}', tileY);
                mapEl.appendChild(img);
            }
        }
        
        // 绘制飞机标识和目标标识（有数据时才绘制）
        if (aircraftCanvas && aircraftCtx) {
            if (aircraftData.latitude !== null && aircraftData.longitude !== null && aircraftData.course !== null) {
                drawAircraftOnMap(aircraftData.latitude, aircraftData.longitude, aircraftData.course);
            } else {
                // 没有飞机数据时至少清空一次覆盖层
                drawAircraftOnMap(null, null, null);
            }
            if (targetData.latitude !== null && targetData.longitude !== null && targetData.course !== null) {
                drawTargetOnMap(targetData.latitude, targetData.longitude, targetData.course);
            }
            drawParachuteOnMap(parachutePoint.lat, parachutePoint.lng);
            drawTargetPickOnMap(targetPickPoint.lat, targetPickPoint.lng);
            drawLaunchPickOnMap(launchPickPoint.lat, launchPickPoint.lng);
            drawWaypointPickOnMap();
            drawWaypointConnectOnMap();
        }
    } catch (err) {
        console.error('Error in render:', err);
    }
}

if (mapEl) {
    // 禁止地图上右键弹出上下文菜单，以便用右键拖动地图
    mapEl.addEventListener('contextmenu', (e) => e.preventDefault());

    // 鼠标右键按下：记录起点与中心，开始拖拽
    mapEl.addEventListener('mousedown', (e) => {
        if (e.button !== 2) return;  // 仅响应右键
        e.preventDefault();
        dragging = true;
        dragStart = { x: e.clientX, y: e.clientY };
        dragStartCenter = { lat: center.lat, lng: center.lng };
        mapEl.style.cursor = 'grabbing';
    });

    // 鼠标移动：根据位移调整中心经纬度
    document.addEventListener('mousemove', (e) => {
        if (!dragging || !mapEl) return;
        e.preventDefault();
        const dx = e.clientX - dragStart.x;
        const dy = e.clientY - dragStart.y;
        if (Math.abs(dx) < 1 && Math.abs(dy) < 1) return;

        const scale = tileSize * Math.pow(2, zoom);
        const dLng = -dx / scale * 360;
        const dLat = dy / scale * 360;

        center.lat = dragStartCenter.lat + dLat;
        center.lng = dragStartCenter.lng + dLng;

        center.lat = Math.max(-85, Math.min(85, center.lat));
        center.lng = ((center.lng % 360) + 360) % 360;
        if (center.lng > 180) center.lng -= 360;

        render();
    });

    // 鼠标右键抬起：结束拖拽（若处于目标点/航路点/开伞点/发射点选点模式则恢复十字光标，否则恢复默认）
    document.addEventListener('mouseup', (e) => {
        if (e.button !== 2 || !dragging) return;
        dragging = false;
        if (mapEl) {
            mapEl.style.cursor = (mapPickTargetEnabled || mapPickParachuteEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
        }
    });

    // 目标点/航路点/开伞点/发射点选点：左键点击地图时，将点击处经纬度回传给 C++（按当前选点模式回传对应 command）
    mapEl.addEventListener('click', (e) => {
        if (e.button !== 0) return;
        const inAnyPickMode = mapPickTargetEnabled || mapPickParachuteEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled;
        if (!inAnyPickMode) return;
        const rect = mapEl.getBoundingClientRect();
        const mouseX = e.clientX - rect.left;
        const mouseY = e.clientY - rect.top;
        if (mouseX < 0 || mouseX > rect.width || mouseY < 0 || mouseY > rect.height) return;
        const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
        const mouseMapX = centerPoint.x - mapEl.clientWidth / 2 + mouseX;
        const mouseMapY = centerPoint.y - mapEl.clientHeight / 2 + mouseY;
        const ll = pointToLatLng(mouseMapX, mouseMapY, zoom);
        if (mapPickTargetEnabled) {
            mapPickTargetEnabled = false;
            targetPickPoint = { lat: ll.lat, lng: ll.lng };
            if (mapEl) mapEl.style.cursor = (mapPickParachuteEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
            if (window.chrome && window.chrome.webview)
                window.chrome.webview.postMessage({ command: 'mapPickTargetResult', lat: ll.lat, lng: ll.lng });
            render();
            return;
        }
        if (mapPickParachuteEnabled) {
            mapPickParachuteEnabled = false;
            parachutePoint = { lat: ll.lat, lng: ll.lng };
            if (mapEl) mapEl.style.cursor = (mapPickTargetEnabled || mapPickLaunchEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
            if (window.chrome && window.chrome.webview)
                window.chrome.webview.postMessage({ command: 'mapPickParachuteResult', lat: ll.lat, lng: ll.lng });
            render();
            return;
        }
        if (mapPickLaunchEnabled) {
            mapPickLaunchEnabled = false;
            launchPickPoint = { lat: ll.lat, lng: ll.lng };
            if (mapEl) mapEl.style.cursor = (mapPickTargetEnabled || mapPickParachuteEnabled || mapPickWaypointEnabled) ? 'crosshair' : 'default';
            if (window.chrome && window.chrome.webview)
                window.chrome.webview.postMessage({ command: 'mapPickLaunchResult', lat: ll.lat, lng: ll.lng });
            render();
            return;
        }
        // 航路点选点：每次点击都回传一次结果，并在本地记录用于绘制序号图标
        if (mapPickWaypointEnabled) {
            waypointPickPoints.push({ lat: ll.lat, lng: ll.lng });
            if (window.chrome && window.chrome.webview)
                window.chrome.webview.postMessage({ command: 'mapPickWaypointResult', lat: ll.lat, lng: ll.lng });
            render();
        }
    });

    // 滚轮缩放：在 document 上捕获阶段监听，避免被宿主或其它元素拦截，确保地图区域滚轮一定由我们处理
    function onMapWheel(e) {
        if (!mapEl || !mapEl.contains(e.target)) return;
        const rect = mapEl.getBoundingClientRect();
        const mouseX = e.clientX - rect.left;
        const mouseY = e.clientY - rect.top;
        if (mouseX < 0 || mouseX > rect.width || mouseY < 0 || mouseY > rect.height) return;

        e.preventDefault();
        e.stopPropagation();

        const delta = e.deltaY < 0 ? 1 : -1;
        const effectiveMax = getEffectiveMaxZoom(center.lat, center.lng);
        const minZoom = Math.max(FLOOR_ZOOM, baseMinZoom);
        const nextZoom = Math.max(minZoom, Math.min(effectiveMax, zoom + delta));
        if (nextZoom === zoom) return;

        const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
        const mouseMapX = centerPoint.x - mapEl.clientWidth / 2 + mouseX;
        const mouseMapY = centerPoint.y - mapEl.clientHeight / 2 + mouseY;
        const mouseLatLng = pointToLatLng(mouseMapX, mouseMapY, zoom);

        zoom = nextZoom;

        const newCenterPoint = latLngToPoint(mouseLatLng.lat, mouseLatLng.lng, zoom);
        const newCenterX = newCenterPoint.x - mouseX + mapEl.clientWidth / 2;
        const newCenterY = newCenterPoint.y - mouseY + mapEl.clientHeight / 2;
        center = pointToLatLng(newCenterX, newCenterY, zoom);

        render();
    }
    document.addEventListener('wheel', onMapWheel, { passive: false, capture: true });

    // 鼠标移动：计算并显示当前光标对应的经纬度（lon, lat）
    mapEl.addEventListener('mousemove', (e) => {
        if (!mouseCoordTipEl || !mouseCoordEnabled) return;
        const rect = mapEl.getBoundingClientRect();
        const mouseX = e.clientX - rect.left;
        const mouseY = e.clientY - rect.top;
        if (mouseX < 0 || mouseX > rect.width || mouseY < 0 || mouseY > rect.height) return;
        const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
        const mouseMapX = centerPoint.x - mapEl.clientWidth / 2 + mouseX;
        const mouseMapY = centerPoint.y - mapEl.clientHeight / 2 + mouseY;
        const mouseLatLng = pointToLatLng(mouseMapX, mouseMapY, zoom);
        const lonStr = mouseLatLng.lng.toFixed(6);
        const latStr = mouseLatLng.lat.toFixed(6);
        mouseCoordTipEl.textContent = 'lon: ' + lonStr + '\nlat: ' + latStr;
        mouseCoordTipEl.style.left = (e.clientX + 14) + 'px';
        mouseCoordTipEl.style.top = (e.clientY + 14) + 'px';
        mouseCoordTipEl.style.visibility = 'visible';
    });
    mapEl.addEventListener('mouseleave', () => {
        if (mouseCoordTipEl) mouseCoordTipEl.style.visibility = 'hidden';
    });

    mapEl.style.cursor = 'default';
}

// 初始化飞机标识Canvas
function initAircraftCanvas() {
    if (!mapEl) {
        console.warn('initAircraftCanvas: mapEl not found');
        return;
    }
    
    if (!aircraftCanvas) {
        aircraftCanvas = document.createElement('canvas');
        aircraftCanvas.id = 'aircraft-overlay';
        aircraftCanvas.style.position = 'absolute';
        aircraftCanvas.style.top = '0';
        aircraftCanvas.style.left = '0';
        aircraftCanvas.style.width = '100%';
        aircraftCanvas.style.height = '100%';
        aircraftCanvas.style.pointerEvents = 'none';
        aircraftCanvas.style.zIndex = '15';
        mapEl.appendChild(aircraftCanvas);
        aircraftCtx = aircraftCanvas.getContext('2d');
        
        if (!aircraftCtx) {
            console.error('initAircraftCanvas: Failed to get 2d context');
            return;
        }
    }
    
    resizeAircraftCanvas();
}

// 调整地图飞机标识Canvas尺寸
function resizeAircraftCanvas() {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) return;
    const w = mapEl.clientWidth;
    const h = mapEl.clientHeight;
    aircraftCanvas.width = w;
    aircraftCanvas.height = h;
}

// 绘制地图上的飞机标识（3D箭头）和轨迹连线
function drawAircraftOnMap(lat, lng, course) {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) {
        console.warn('drawAircraftOnMap: Canvas or context not initialized');
        return;
    }
    
    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    
    aircraftCtx.clearRect(0, 0, width, height);
    
    if (lat === null || lng === null || course === null || 
        isNaN(lat) || isNaN(lng) || isNaN(course)) {
        drawTrail();
        return;
    }
    
    const aircraftPoint = latLngToPoint(lat, lng, zoom);
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    
    const aircraftX = aircraftPoint.x - topLeft.x;
    const aircraftY = aircraftPoint.y - topLeft.y;
    
    const currentTime = Date.now();
    if (aircraftTrail.length === 0 || (currentTime - lastTrailSaveTime) >= TRAIL_SAVE_INTERVAL_MS) {
        const currentPoint = { lat, lng };
        aircraftTrail.push(currentPoint);
        lastTrailSaveTime = currentTime;
        
        if (aircraftTrail.length > MAX_TRAIL_POINTS) {
            aircraftTrail.shift();
        }
    }
    
    drawTrail();
    
    if (aircraftX < -50 || aircraftX > width + 50 || aircraftY < -50 || aircraftY > height + 50) {
        return;
    }
    
    const arrowLength = 28;
    const arrowBottomWidth = 38;
    const arrowBottomY = 24;
    const arrowBottomIndent = 8;
    
    aircraftCtx.save();
    aircraftCtx.translate(aircraftX, aircraftY);
    // NED：0=北(屏幕上方)，顺时针为正；Canvas 顺时针为正，故直接用 course
    const canvasAngle = course;
    aircraftCtx.rotate(canvasAngle * Math.PI / 180);
    
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = 'transparent';
    
    aircraftCtx.fillStyle = '#cc0000';
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(-arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, arrowBottomY - arrowBottomIndent);
    aircraftCtx.lineTo(arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    const centerHighlightWidth = arrowBottomWidth * 0.3;
    const bottomHighlightY = arrowBottomY - arrowBottomIndent;
    const highlightStartY = -arrowLength + 8;
    
    const centerHighlight = aircraftCtx.createLinearGradient(-centerHighlightWidth / 2, highlightStartY, centerHighlightWidth / 2, bottomHighlightY);
    centerHighlight.addColorStop(0, 'rgba(255, 200, 200, 0.5)');
    centerHighlight.addColorStop(0.3, 'rgba(255, 160, 160, 0.4)');
    centerHighlight.addColorStop(0.7, 'rgba(255, 120, 120, 0.3)');
    centerHighlight.addColorStop(1, 'rgba(255, 80, 80, 0.2)');
    
    aircraftCtx.fillStyle = centerHighlight;
    aircraftCtx.beginPath();
    const topHighlightWidth = centerHighlightWidth;
    const bottomHighlightWidth = centerHighlightWidth * 0.2;
    aircraftCtx.moveTo(-topHighlightWidth / 2, highlightStartY);
    aircraftCtx.lineTo(topHighlightWidth / 2, highlightStartY);
    aircraftCtx.lineTo(bottomHighlightWidth / 2, bottomHighlightY);
    aircraftCtx.lineTo(-bottomHighlightWidth / 2, bottomHighlightY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    const edgeShadow = aircraftCtx.createLinearGradient(-arrowBottomWidth / 2, arrowBottomY, 0, arrowBottomY);
    edgeShadow.addColorStop(0, 'rgba(0, 0, 0, 0.4)');
    edgeShadow.addColorStop(0.5, 'rgba(0, 0, 0, 0.2)');
    edgeShadow.addColorStop(1, 'rgba(0, 0, 0, 0)');
    
    aircraftCtx.fillStyle = edgeShadow;
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(-arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    const edgeShadowRight = aircraftCtx.createLinearGradient(0, arrowBottomY, arrowBottomWidth / 2, arrowBottomY);
    edgeShadowRight.addColorStop(0, 'rgba(0, 0, 0, 0)');
    edgeShadowRight.addColorStop(0.5, 'rgba(0, 0, 0, 0.2)');
    edgeShadowRight.addColorStop(1, 'rgba(0, 0, 0, 0.4)');
    aircraftCtx.fillStyle = edgeShadowRight;
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    const axisLineWidth = 1.5;
    const axisLineColor = '#ffffff';
    const axisLineOpacity = 0.8;
    
    aircraftCtx.strokeStyle = axisLineColor;
    aircraftCtx.globalAlpha = axisLineOpacity;
    aircraftCtx.lineWidth = axisLineWidth;
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = 'transparent';
    aircraftCtx.lineCap = 'round';
    
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.stroke();
    
    aircraftCtx.globalAlpha = 1.0;
    
    aircraftCtx.strokeStyle = '#ffffff';
    aircraftCtx.lineWidth = 2;
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = '#ffffff';
    aircraftCtx.lineJoin = 'round';
    aircraftCtx.lineCap = 'round';
    
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(-arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.lineTo(arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.closePath();
    aircraftCtx.stroke();
    
    aircraftCtx.restore();
}

// 绘制地图上的目标标识（3D箭头）- 深灰色主题
// 参数：
//   lat, lng  : 目标经纬度（度）
//   course    : 目标航向角（度，0-359，0度为北，顺时针为正）
function drawTargetOnMap(lat, lng, course) {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) {
        console.warn('drawTargetOnMap: Canvas or context not initialized');
        return;
    }
    
    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    
    // 注意：不清空Canvas，因为本机标识和目标标识共享同一个Canvas
    // 清空操作在 drawAircraftOnMap 中完成
    
    if (lat === null || lng === null || course === null || 
        isNaN(lat) || isNaN(lng) || isNaN(course)) {
        return;
    }
    
    const targetPoint = latLngToPoint(lat, lng, zoom);
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    
    const targetX = targetPoint.x - topLeft.x;
    const targetY = targetPoint.y - topLeft.y;
    
    // 检查目标是否在地图可见区域内（留50像素边距）
    if (targetX < -50 || targetX > width + 50 || targetY < -50 || targetY > height + 50) {
        return;
    }
    
    // 箭头尺寸参数（与本机标识相同）
    const arrowLength = 28;
    const arrowBottomWidth = 38;
    const arrowBottomY = 24;
    const arrowBottomIndent = 8;
    
    aircraftCtx.save();
    aircraftCtx.translate(targetX, targetY);
    // NED：0=北(屏幕上方)，顺时针为正
    const canvasAngle = course;
    aircraftCtx.rotate(canvasAngle * Math.PI / 180);
    
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = 'transparent';
    
    // 深灰色主体（#555555 替代红色 #cc0000）
    aircraftCtx.fillStyle = '#555555';
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(-arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, arrowBottomY - arrowBottomIndent);
    aircraftCtx.lineTo(arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    // 中间垂直高光带（深灰色主题，使用较暗的高光）
    const centerHighlightWidth = arrowBottomWidth * 0.3;
    const bottomHighlightY = arrowBottomY - arrowBottomIndent;
    const highlightStartY = -arrowLength + 8;
    
    const centerHighlight = aircraftCtx.createLinearGradient(-centerHighlightWidth / 2, highlightStartY, centerHighlightWidth / 2, bottomHighlightY);
    centerHighlight.addColorStop(0, 'rgba(150, 150, 150, 0.4)');  // 深灰色高光起始
    centerHighlight.addColorStop(0.3, 'rgba(120, 120, 120, 0.3)');
    centerHighlight.addColorStop(0.7, 'rgba(100, 100, 100, 0.2)');
    centerHighlight.addColorStop(1, 'rgba(80, 80, 80, 0.1)');  // 底部较暗
    
    aircraftCtx.fillStyle = centerHighlight;
    aircraftCtx.beginPath();
    const topHighlightWidth = centerHighlightWidth;
    const bottomHighlightWidth = centerHighlightWidth * 0.2;
    aircraftCtx.moveTo(-topHighlightWidth / 2, highlightStartY);
    aircraftCtx.lineTo(topHighlightWidth / 2, highlightStartY);
    aircraftCtx.lineTo(bottomHighlightWidth / 2, bottomHighlightY);
    aircraftCtx.lineTo(-bottomHighlightWidth / 2, bottomHighlightY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    // 边缘阴影效果（两侧较暗）
    const edgeShadow = aircraftCtx.createLinearGradient(-arrowBottomWidth / 2, arrowBottomY, 0, arrowBottomY);
    edgeShadow.addColorStop(0, 'rgba(0, 0, 0, 0.4)');
    edgeShadow.addColorStop(0.5, 'rgba(0, 0, 0, 0.2)');
    edgeShadow.addColorStop(1, 'rgba(0, 0, 0, 0)');
    
    // 左侧阴影
    aircraftCtx.fillStyle = edgeShadow;
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(-arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    // 右侧阴影（对称）
    const edgeShadowRight = aircraftCtx.createLinearGradient(0, arrowBottomY, arrowBottomWidth / 2, arrowBottomY);
    edgeShadowRight.addColorStop(0, 'rgba(0, 0, 0, 0)');
    edgeShadowRight.addColorStop(0.5, 'rgba(0, 0, 0, 0.2)');
    edgeShadowRight.addColorStop(1, 'rgba(0, 0, 0, 0.4)');
    aircraftCtx.fillStyle = edgeShadowRight;
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    // 对称轴连线（从顶部到底部的垂直线）- 使用浅灰色
    const axisLineWidth = 1.5;
    const axisLineColor = '#cccccc';  // 浅灰色替代白色
    const axisLineOpacity = 0.8;
    
    aircraftCtx.strokeStyle = axisLineColor;
    aircraftCtx.globalAlpha = axisLineOpacity;
    aircraftCtx.lineWidth = axisLineWidth;
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = 'transparent';
    aircraftCtx.lineCap = 'round';
    
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.stroke();
    
    // 恢复全局透明度
    aircraftCtx.globalAlpha = 1.0;
    
    // 浅灰色发光边缘（外圈）- 替代白色
    aircraftCtx.strokeStyle = '#aaaaaa';  // 浅灰色替代白色
    aircraftCtx.lineWidth = 2;
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = '#aaaaaa';
    aircraftCtx.lineJoin = 'round';
    aircraftCtx.lineCap = 'round';
    
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);
    aircraftCtx.lineTo(-arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.lineTo(0, bottomHighlightY);
    aircraftCtx.lineTo(arrowBottomWidth / 2, arrowBottomY);
    aircraftCtx.closePath();
    aircraftCtx.stroke();
    
    aircraftCtx.restore();
}

// 绘制地图上的开伞点标识（简化伞形：上为等腰三角形伞盖，下为短竖线）
function drawParachuteOnMap(lat, lng) {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) return;
    if (lat === null || lng === null || isNaN(lat) || isNaN(lng)) return;

    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    const pt = latLngToPoint(lat, lng, zoom);
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    const x = pt.x - topLeft.x;
    const y = pt.y - topLeft.y;

    if (x < -45 || x > width + 45 || y < -45 || y > height + 45) return;

    aircraftCtx.save();
    aircraftCtx.translate(x, y);

    const triH = 7 * 1.15;
    const triW = 30 * 1.15;
    const lineLen = 8 * 1.15;
    const lineStartY = triH * 0.3;

    // 等腰三角形（伞盖）：顶点在上，底边在下
    aircraftCtx.fillStyle = '#1a3a8a';
    aircraftCtx.strokeStyle = '#ffffff';
    aircraftCtx.lineWidth = 1.5 * 1.15;
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -triH);
    aircraftCtx.lineTo(-triW / 2, lineStartY);
    aircraftCtx.lineTo(triW / 2, lineStartY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    aircraftCtx.stroke();

    // 竖矩形（填充+白色描边）
    const rectW = 4;
    aircraftCtx.fillStyle = '#1a3a8a';
    aircraftCtx.strokeStyle = '#ffffff';
    aircraftCtx.lineWidth = 1.5;
    aircraftCtx.fillRect(-rectW / 2, lineStartY, rectW, lineLen);
    aircraftCtx.strokeRect(-rectW / 2, lineStartY, rectW, lineLen);

    aircraftCtx.restore();
}

// 绘制地图上的目标点选点标识（准星样式：中心实心点 + 同心空心圆 + 四向从圆缘外伸的短线）
function drawTargetPickOnMap(lat, lng) {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) return;
    if (lat === null || lng === null || isNaN(lat) || isNaN(lng)) return;

    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    const pt = latLngToPoint(lat, lng, zoom);
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    const x = pt.x - topLeft.x;
    const y = pt.y - topLeft.y;

    if (x < -50 || x > width + 50 || y < -50 || y > height + 50) return;

    aircraftCtx.save();
    aircraftCtx.translate(x, y);

    const color = '#8b0000';
    const R = 8;           // 空心圆半径
    const tickLen = 6;       // 从圆缘向外延伸的短线长度
    const lineW = 1.5;

    // 同心空心圆
    aircraftCtx.strokeStyle = color;
    aircraftCtx.lineWidth = lineW;
    aircraftCtx.beginPath();
    aircraftCtx.arc(0, 0, R, 0, Math.PI * 2);
    aircraftCtx.stroke();

    // 四根短线：从圆环外缘向外延伸（上、下、左、右）
    aircraftCtx.strokeStyle = color;
    aircraftCtx.lineWidth = lineW;
    aircraftCtx.lineCap = 'round';
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -R);
    aircraftCtx.lineTo(0, -R - tickLen);
    aircraftCtx.moveTo(0, R);
    aircraftCtx.lineTo(0, R + tickLen);
    aircraftCtx.moveTo(-R, 0);
    aircraftCtx.lineTo(-R - tickLen, 0);
    aircraftCtx.moveTo(R, 0);
    aircraftCtx.lineTo(R + tickLen, 0);
    aircraftCtx.stroke();

    // 中心实心点（靶心）
    aircraftCtx.fillStyle = color;
    aircraftCtx.beginPath();
    aircraftCtx.arc(0, 0, 2.5, 0, Math.PI * 2);
    aircraftCtx.fill();

    aircraftCtx.restore();
}

// 绘制地图上的发射点选点标识（山字形状带箭头：黑色，尺寸参考开伞/目标点图标）
function drawLaunchPickOnMap(lat, lng) {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) return;
    if (lat === null || lng === null || isNaN(lat) || isNaN(lng)) return;

    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    const pt = latLngToPoint(lat, lng, zoom);
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    const x = pt.x - topLeft.x;
    const y = pt.y - topLeft.y;

    if (x < -50 || x > width + 50 || y < -50 || y > height + 50) return;

    aircraftCtx.save();
    aircraftCtx.translate(x, y);

    const black = '#000000';
    const lineW = 1.5;
    const halfW = 9;        // 山字半宽，与开伞/目标点图标相当
    const baseY = 8;       // 山字纵高 y
    const topY = -2;        // 山字两竖上端 y
    const arrowTipY = -14;  // 箭头尖端 y
    const arrowHeadW = 5;   // 箭头 V 的半宽
    const arrowHeadH = 6;   // 箭头 V 的高度

    aircraftCtx.strokeStyle = black;
    aircraftCtx.fillStyle = black;
    aircraftCtx.lineWidth = lineW;
    aircraftCtx.lineCap = 'square';
    aircraftCtx.lineJoin = 'miter';

    // 山字：左竖、右竖、底横
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(-halfW, topY);
    aircraftCtx.lineTo(-halfW, baseY);
    aircraftCtx.moveTo(halfW, topY);
    aircraftCtx.lineTo(halfW, baseY);
    aircraftCtx.moveTo(-halfW, baseY);
    aircraftCtx.lineTo(halfW, baseY);
    aircraftCtx.stroke();

    // 箭头竖线：从底边中心向上
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, baseY);
    aircraftCtx.lineTo(0, arrowTipY);
    aircraftCtx.stroke();

    // 箭头 V 形头
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, arrowTipY);
    aircraftCtx.lineTo(-arrowHeadW, arrowTipY + arrowHeadH);
    aircraftCtx.moveTo(0, arrowTipY);
    aircraftCtx.lineTo(arrowHeadW, arrowTipY + arrowHeadH);
    aircraftCtx.stroke();

    aircraftCtx.restore();
}

// 航路点选点图标：数字 + 外空心圆（类似 Word 排列序号），黑色；尺寸参数可调
const WAYPOINT_ICON_FONT_SIZE = 12;   // 航点序号数字字号（像素）
const WAYPOINT_ICON_CIRCLE_R = 10;    // 外空心圆半径（像素）
// 航路点连线中点箭头（指向高序号），可调
const WAYPOINT_CONNECT_ARROW_LEN = 12;        // 箭头沿线方向的长度（像素）
const WAYPOINT_CONNECT_ARROW_HALF_WIDTH = 5;  // 箭头底边半宽（像素）

function drawWaypointPickOnMap() {
    if (!aircraftCanvas || !aircraftCtx || !mapEl || !waypointPickPoints.length) return;

    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    const black = '#000000';

    aircraftCtx.save();
    aircraftCtx.strokeStyle = black;
    aircraftCtx.fillStyle = black;
    aircraftCtx.font = `${WAYPOINT_ICON_FONT_SIZE}px Consolas, sans-serif`;
    aircraftCtx.textAlign = 'center';
    aircraftCtx.textBaseline = 'middle';
    aircraftCtx.lineWidth = 1.5;

    for (let i = 0; i < waypointPickPoints.length; i++) {
        const p = waypointPickPoints[i];
        const pt = latLngToPoint(p.lat, p.lng, zoom);
        const x = pt.x - topLeft.x;
        const y = pt.y - topLeft.y;
        if (x < -WAYPOINT_ICON_CIRCLE_R * 2 || x > width + WAYPOINT_ICON_CIRCLE_R * 2 ||
            y < -WAYPOINT_ICON_CIRCLE_R * 2 || y > height + WAYPOINT_ICON_CIRCLE_R * 2) continue;

        aircraftCtx.beginPath();
        aircraftCtx.arc(x, y, WAYPOINT_ICON_CIRCLE_R, 0, Math.PI * 2);
        aircraftCtx.stroke();

        const num = (i + 1).toString();
        aircraftCtx.fillText(num, x, y);
    }

    aircraftCtx.restore();
}

// 航路点连线：按航点序号顺序，将所有航点用黑色细线依次连起来，
// 且连线在两端跳过航点空心圆的半径，只连接到圆的外缘
function drawWaypointConnectOnMap() {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) return;
    if (!waypointConnectEnabled || waypointPickPoints.length < 2) return;

    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };

    aircraftCtx.save();
    aircraftCtx.strokeStyle = '#000000';
    aircraftCtx.lineWidth = 1.5;
    aircraftCtx.lineJoin = 'round';
    aircraftCtx.lineCap = 'round';

    // 逐段绘制：每一段单独 beginPath/moveTo/lineTo，
    // 并在两端沿方向向量缩短一个 WAYPOINT_ICON_CIRCLE_R
    for (let i = 0; i < waypointPickPoints.length - 1; i++) {
        const p1 = waypointPickPoints[i];
        const p2 = waypointPickPoints[i + 1];

        const pt1 = latLngToPoint(p1.lat, p1.lng, zoom);
        const pt2 = latLngToPoint(p2.lat, p2.lng, zoom);

        let x1 = pt1.x - topLeft.x;
        let y1 = pt1.y - topLeft.y;
        let x2 = pt2.x - topLeft.x;
        let y2 = pt2.y - topLeft.y;

        const dx = x2 - x1;
        const dy = y2 - y1;
        const len = Math.sqrt(dx * dx + dy * dy);
        if (!len || len <= WAYPOINT_ICON_CIRCLE_R * 2) {
            continue; // 两个航点太近，画了也会在圆内，直接跳过
        }

        const ux = dx / len;
        const uy = dy / len;

        // 起点从第一个圆外缘开始，终点在第二个圆外缘结束
        const sx = x1 + ux * WAYPOINT_ICON_CIRCLE_R;
        const sy = y1 + uy * WAYPOINT_ICON_CIRCLE_R;
        const ex = x2 - ux * WAYPOINT_ICON_CIRCLE_R;
        const ey = y2 - uy * WAYPOINT_ICON_CIRCLE_R;

        // 如果整个线段都在视野之外，则可以简单跳过
        const minX = Math.min(sx, ex);
        const maxX = Math.max(sx, ex);
        const minY = Math.min(sy, ey);
        const maxY = Math.max(sy, ey);
        if (maxX < -20 || minX > width + 20 || maxY < -20 || minY > height + 20) {
            continue;
        }

        aircraftCtx.beginPath();
        aircraftCtx.moveTo(sx, sy);
        aircraftCtx.lineTo(ex, ey);
        aircraftCtx.stroke();

        // 中点画箭头，指向高序号（p2）一端
        const mx = (sx + ex) / 2;
        const my = (sy + ey) / 2;
        const halfLen = WAYPOINT_CONNECT_ARROW_LEN / 2;
        const tipX = mx + ux * halfLen;
        const tipY = my + uy * halfLen;
        const baseCx = mx - ux * halfLen;
        const baseCy = my - uy * halfLen;
        const px = -uy;
        const py = ux;
        const leftX = baseCx + px * WAYPOINT_CONNECT_ARROW_HALF_WIDTH;
        const leftY = baseCy + py * WAYPOINT_CONNECT_ARROW_HALF_WIDTH;
        const rightX = baseCx - px * WAYPOINT_CONNECT_ARROW_HALF_WIDTH;
        const rightY = baseCy - py * WAYPOINT_CONNECT_ARROW_HALF_WIDTH;
        aircraftCtx.beginPath();
        aircraftCtx.moveTo(tipX, tipY);
        aircraftCtx.lineTo(leftX, leftY);
        aircraftCtx.lineTo(rightX, rightY);
        aircraftCtx.closePath();
        aircraftCtx.fillStyle = '#000000';
        aircraftCtx.fill();
    }

    aircraftCtx.restore();
}

// 绘制飞机轨迹连线
function drawTrail() {
    if (!aircraftCtx || aircraftTrail.length < 2) return;
    
    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    
    aircraftCtx.strokeStyle = '#ff6b6b';
    aircraftCtx.lineWidth = 2;
    aircraftCtx.lineCap = 'round';
    aircraftCtx.lineJoin = 'round';
    
    aircraftCtx.beginPath();
    let firstPoint = true;
    
    for (let i = 0; i < aircraftTrail.length; i++) {
        const point = latLngToPoint(aircraftTrail[i].lat, aircraftTrail[i].lng, zoom);
        const x = point.x - topLeft.x;
        const y = point.y - topLeft.y;
        
        if (x >= -10 && x <= width + 10 && y >= -10 && y <= height + 10) {
            if (firstPoint) {
                aircraftCtx.moveTo(x, y);
                firstPoint = false;
            } else {
                aircraftCtx.lineTo(x, y);
            }
        } else if (!firstPoint) {
            firstPoint = true;
        }
    }
    aircraftCtx.stroke();
}

// 初始化底部信息栏Canvas
function initBottomInfoCanvas() {
    if (!bottomInfoCanvas) {
        bottomInfoCanvas = document.createElement('canvas');
        bottomInfoCanvas.id = 'bottomInfoCanvas';
        bottomInfoCanvas.style.position = 'fixed';
        bottomInfoCanvas.style.bottom = '0';
        bottomInfoCanvas.style.left = '0';
        bottomInfoCanvas.style.zIndex = '10000';
        bottomInfoCanvas.style.pointerEvents = 'none';
        document.body.appendChild(bottomInfoCanvas);
        bottomInfoCtx = bottomInfoCanvas.getContext('2d');
    }
    resizeBottomInfoCanvas();
}

// 调整底部信息栏Canvas尺寸
function resizeBottomInfoCanvas() {
    if (!bottomInfoCanvas || !bottomInfoCtx) return;
    const w = window.innerWidth;
    const h = window.innerHeight;
    bottomInfoCanvas.width = w;
    bottomInfoCanvas.height = h;
    bottomInfoCanvas.style.width = w + 'px';
    bottomInfoCanvas.style.height = h + 'px';
}

// 计算累计飞行距离（使用Haversine公式，不考虑垂直方向）
function calculateTotalDistance() {
    if (!aircraftTrail || aircraftTrail.length < 2) {
        return 0;
    }
    
    let totalDistance = 0;
    
    for (let i = 1; i < aircraftTrail.length; i++) {
        const prev = aircraftTrail[i - 1];
        const curr = aircraftTrail[i];
        
        const R = 6371000;
        const lat1Rad = prev.lat * Math.PI / 180;
        const lat2Rad = curr.lat * Math.PI / 180;
        const deltaLatRad = (curr.lat - prev.lat) * Math.PI / 180;
        const deltaLngRad = (curr.lng - prev.lng) * Math.PI / 180;
        
        const a = Math.sin(deltaLatRad / 2) * Math.sin(deltaLatRad / 2) +
                  Math.cos(lat1Rad) * Math.cos(lat2Rad) *
                  Math.sin(deltaLngRad / 2) * Math.sin(deltaLngRad / 2);
        const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
        const distance = R * c;
        
        totalDistance += distance;
    }
    
    return totalDistance;
}

// 底部信息栏（地速、垂直速度、GPS时间、累计距离）
function drawBottomInfoBar(w, h, data, colors) {
    if (!bottomInfoCtx || !bottomInfoCanvas) {
        initBottomInfoCanvas();
        if (!bottomInfoCtx) return;
    }
    
    resizeBottomInfoCanvas();
    
    const actualW = window.innerWidth;
    const actualH = window.innerHeight;
    
    const barHeight = 42;
    const barY = actualH - barHeight;
    
    bottomInfoCtx.clearRect(0, 0, actualW, actualH);
    
    bottomInfoCtx.fillStyle = 'rgba(100, 100, 100, 0.7)';
    bottomInfoCtx.fillRect(0, barY, actualW, barHeight);
    
    bottomInfoCtx.fillStyle = '#ffffff';
    bottomInfoCtx.font = '14px Consolas, monospace';
    bottomInfoCtx.textAlign = 'center';
    bottomInfoCtx.textBaseline = 'middle';
    
    const groundSpeed = (data && data.groundSpeed !== undefined) ? data.groundSpeed.toFixed(1) : '0.0';
    const verticalSpeed = (data && data.verticalSpeed !== undefined) ? data.verticalSpeed.toFixed(1) : '0.0';
    const hour = (data && data.gpsHour !== undefined) ? String(data.gpsHour).padStart(2, '0') : '00';
    const minute = (data && data.gpsMinute !== undefined) ? String(data.gpsMinute).padStart(2, '0') : '00';
    const second = (data && data.gpsSecond !== undefined) ? String(data.gpsSecond).padStart(2, '0') : '00';
    const gpsTime = hour + ':' + minute + ':' + second;
    
    const totalDistance = calculateTotalDistance();
    let distanceText = '';
    if (totalDistance >= 1000) {
        distanceText = (totalDistance / 1000).toFixed(2) + ' km';
    } else {
        distanceText = totalDistance.toFixed(1) + ' m';
    }
    
    const text1 = '-> ' + groundSpeed + ' m/s';
    const text2 = '^ ' + verticalSpeed + ' m/s';
    const text3 = 'GPS ' + gpsTime;
    const text4 = 'Dist ' + distanceText;
    const spacing = 30;
    
    const text1Width = bottomInfoCtx.measureText(text1).width;
    const text2Width = bottomInfoCtx.measureText(text2).width;
    const text3Width = bottomInfoCtx.measureText(text3).width;
    const text4Width = bottomInfoCtx.measureText(text4).width;
    
    const totalWidth = text1Width + spacing + text2Width + spacing + text3Width + spacing + text4Width;
    const startX = (actualW - totalWidth) / 2 + text1Width / 2;
    const centerY = barY + barHeight / 2;
    
    bottomInfoCtx.fillText(text1, startX, centerY);
    bottomInfoCtx.fillText(text2, startX + text1Width / 2 + spacing + text2Width / 2, centerY);
    bottomInfoCtx.fillText(text3, startX + text1Width / 2 + spacing + text2Width + spacing + text3Width / 2, centerY);
    bottomInfoCtx.fillText(text4, startX + text1Width / 2 + spacing + text2Width + spacing + text3Width + spacing + text4Width / 2, centerY);
}

// 初始化报警弹窗Canvas
function initAlarmCanvas() {
    if (!alarmCanvas) {
        alarmCanvas = document.createElement('canvas');
        alarmCanvas.id = 'alarmCanvas';
        alarmCanvas.style.position = 'fixed';
        alarmCanvas.style.top = '0';
        alarmCanvas.style.left = '0';
        alarmCanvas.style.zIndex = '20000';  // 最顶层
        alarmCanvas.style.pointerEvents = 'none';  // 默认不拦截事件
        document.body.appendChild(alarmCanvas);
        alarmCtx = alarmCanvas.getContext('2d');

        // 点击事件放在 document 上，避免遮挡地图拖动
        document.addEventListener('click', (e) => {
            const x = e.clientX;
            const y = e.clientY;
            const w = window.innerWidth;
            const h = window.innerHeight;
            const centerX = w / 2;
            const alarmWidth = 400;
            const alarmHeight = 50;
            const alarmSpacing = 10;

            // 通信丢包弹窗：居中偏下，点击即关闭
            if ((packetLossAlarm.show || packetLossAlarm.phase === 'countdown') && packetLossAlarm.phase !== 'idle') {
                const packetLossBaseY = h * 0.75;
                if (x >= centerX - alarmWidth / 2 && x <= centerX + alarmWidth / 2 &&
                    y >= packetLossBaseY && y <= packetLossBaseY + alarmHeight) {
                    if (packetLossAlarm.intervalId) clearInterval(packetLossAlarm.intervalId);
                    packetLossAlarm.intervalId = null;
                    packetLossAlarm.show = false;
                    packetLossAlarm.phase = 'idle';
                    drawAlarmPopups();
                    e.stopPropagation();
                    return;
                }
            }

            if (!activeAlarms || activeAlarms.length === 0) return;
            const baseY = h * 0.2;
            for (let i = 0; i < activeAlarms.length; i++) {
                const alarmY = baseY + i * (alarmHeight + alarmSpacing);
                if (x >= centerX - alarmWidth / 2 && x <= centerX + alarmWidth / 2 &&
                    y >= alarmY && y <= alarmY + alarmHeight) {
                    activeAlarms.splice(i, 1);
                    drawAlarmPopups();
                    e.stopPropagation();
                    break;
                }
            }
        }, true);
    }
    resizeAlarmCanvas();
}

// 调整报警弹窗Canvas尺寸
function resizeAlarmCanvas() {
    if (!alarmCanvas || !alarmCtx) return;
    const w = window.innerWidth;
    const h = window.innerHeight;
    alarmCanvas.width = w;
    alarmCanvas.height = h;
    alarmCanvas.style.width = w + 'px';
    alarmCanvas.style.height = h + 'px';
}

// 绘制报警弹窗（协议报警 + 通信丢包报警，样式与 UdpData.h alarmStatus 一致）
function drawAlarmPopups() {
    if (!alarmCtx || !alarmCanvas) {
        initAlarmCanvas();
        if (!alarmCtx) return;
    }

    resizeAlarmCanvas();

    const w = window.innerWidth;
    const h = window.innerHeight;
    const centerX = w / 2;
    const alarmWidth = 400;
    const alarmHeight = 50;
    const alarmSpacing = 10;
    const cornerRadius = 8;

    // 清空画布
    alarmCtx.clearRect(0, 0, w, h);

    alarmCtx.fillStyle = '#000000';
    alarmCtx.font = 'bold 18px Consolas, monospace';
    alarmCtx.textAlign = 'center';
    alarmCtx.textBaseline = 'middle';

    // 1) 协议报警（顶层中央偏上，与 UdpData.h 90-95 一致）
    const baseY = h * 0.2;
    const now = Date.now();
    if (activeAlarms && activeAlarms.length > 0) {
        activeAlarms = activeAlarms.filter(a => (now - a.lastSeen) < 30000);
        for (let i = 0; i < activeAlarms.length; i++) {
            const alarmIndex = activeAlarms[i].index;
            const alarmY = baseY + i * (alarmHeight + alarmSpacing);
            const alarmText = alarmNames[alarmIndex] || ('\u62a5\u8b66' + (alarmIndex + 1));
            const x = centerX - alarmWidth / 2;
            const y = alarmY;
            alarmCtx.fillStyle = 'rgba(255, 200, 0, 0.9)';
            alarmCtx.beginPath();
            alarmCtx.moveTo(x + cornerRadius, y);
            alarmCtx.lineTo(x + alarmWidth - cornerRadius, y);
            alarmCtx.quadraticCurveTo(x + alarmWidth, y, x + alarmWidth, y + cornerRadius);
            alarmCtx.lineTo(x + alarmWidth, y + alarmHeight - cornerRadius);
            alarmCtx.quadraticCurveTo(x + alarmWidth, y + alarmHeight, x + alarmWidth - cornerRadius, y + alarmHeight);
            alarmCtx.lineTo(x + cornerRadius, y + alarmHeight);
            alarmCtx.quadraticCurveTo(x, y + alarmHeight, x, y + alarmHeight - cornerRadius);
            alarmCtx.lineTo(x, y + cornerRadius);
            alarmCtx.quadraticCurveTo(x, y, x + cornerRadius, y);
            alarmCtx.closePath();
            alarmCtx.fill();
            alarmCtx.strokeStyle = '#000000';
            alarmCtx.lineWidth = 2;
            alarmCtx.stroke();
            alarmCtx.fillStyle = '#000000';
            alarmCtx.fillText(alarmText, centerX, alarmY + alarmHeight / 2);
        }
    }

    // 2) 通信丢包报警（居中偏下，错开协议弹窗，样式一致）
    if (packetLossAlarm.show || packetLossAlarm.phase === 'countdown') {
        const packetLossBaseY = h * 0.75;
        const px = centerX - alarmWidth / 2;
        const py = packetLossBaseY;
        let text = '\u6ce8\u610f\uff1a\u901a\u4fe1\u4e22\u5305';  // 注意：通信丢包
        if (packetLossAlarm.phase === 'countdown' && packetLossAlarm.countdownEnd > 0) {
            const secLeft = Math.max(0, Math.ceil((packetLossAlarm.countdownEnd - Date.now()) / 1000));
            text = '\u901a\u4fe1\u6062\u590d\uff0c' + secLeft + 's\u540e\u5173\u95ed';  // 通信恢复，Xs后关闭
        }
        alarmCtx.fillStyle = 'rgba(255, 200, 0, 0.9)';
        alarmCtx.beginPath();
        alarmCtx.moveTo(px + cornerRadius, py);
        alarmCtx.lineTo(px + alarmWidth - cornerRadius, py);
        alarmCtx.quadraticCurveTo(px + alarmWidth, py, px + alarmWidth, py + cornerRadius);
        alarmCtx.lineTo(px + alarmWidth, py + alarmHeight - cornerRadius);
        alarmCtx.quadraticCurveTo(px + alarmWidth, py + alarmHeight, px + alarmWidth - cornerRadius, py + alarmHeight);
        alarmCtx.lineTo(px + cornerRadius, py + alarmHeight);
        alarmCtx.quadraticCurveTo(px, py + alarmHeight, px, py + alarmHeight - cornerRadius);
        alarmCtx.lineTo(px, py + cornerRadius);
        alarmCtx.quadraticCurveTo(px, py, px + cornerRadius, py);
        alarmCtx.closePath();
        alarmCtx.fill();
        alarmCtx.strokeStyle = '#000000';
        alarmCtx.lineWidth = 2;
        alarmCtx.stroke();
        alarmCtx.fillStyle = '#000000';
        alarmCtx.fillText(text, centerX, py + alarmHeight / 2);
    }
}

// 窗口大小改变时，调整所有Canvas尺寸并重新渲染
window.addEventListener('resize', () => {
    resizeHud();
    resizeBottomInfoCanvas();
    resizeAircraftCanvas();
    resizeAlarmCanvas();
    drawHud();
    render();
    drawAlarmPopups();
});

// 初始化飞机标识Canvas
initAircraftCanvas();

// 初始化底部信息栏Canvas（延迟初始化，确保窗口尺寸已确定）
setTimeout(() => {
    initBottomInfoCanvas();
    initAlarmCanvas();
    initHudGroup2Canvas();  // 第三步：初始化HUD组2 Canvas
    initHudGroup3Canvas();  // 第三步：初始化HUD组3 Canvas
    if (typeof drawHud === 'function') {
        drawHud();
    }
    drawAlarmPopups();
}, 100);

// 初始渲染
render();
