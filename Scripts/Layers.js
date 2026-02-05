// map.js - 离线地图 + HUD 渲染
// 说明：
// 1) mapConfig / statusText 由 C++ 在 map.html 占位符处注入
// 2) C++ 调用 PostWebMessageAsJson 推送 HUD 数据，JS 在 webview message 事件里接收并更新 hudState
// 3) HUD 绘制集中在 drawHud，拆分多个小函数，方便你单独调整配色、位置、尺寸

// DOM 获取
const mapEl = document.getElementById('map');      // 地图容器（瓦片背景）
const statusEl = document.getElementById('status'); // 状态提示
const hudCanvas = document.getElementById('hud');   // HUD 画布
const hudCtx = hudCanvas ? hudCanvas.getContext('2d') : null;
let hudDpr = window.devicePixelRatio || 1;          // 设备像素比，用于抗锯齿

// 地图飞机标识Canvas（覆盖在地图上）
let aircraftCanvas = null;
let aircraftCtx = null;

// 底部信息栏Canvas（覆盖在整个主界面上）
let bottomInfoCanvas = null;
let bottomInfoCtx = null;


// ============================================================================
// HUD组1 数据结构（根据 UdpData.h 中的 HUD组1 协议定义）
// ============================================================================
// 说明：所有字段均为 int16_t 原始值，后续会根据实际协议进行缩放转换
// 协议字段映射：
//   HUD组1 协议(1)  -> pitchAngle      - 俯仰角
//   HUD组1 协议(2)  -> rollAngle       - 滚转角
//   HUD组1 协议(3)  -> yawAngle        - 航向角
//   HUD组1 协议(4-1)-> normalOverload  - 法向过载
//   HUD组1 协议(5)  -> attackAngle     - 攻角
//   HUD组1 协议(6)  -> sideslipAngle   - 侧滑角
//   HUD组1 协议(7)  -> engineRPM       - 发动机转速
//   HUD组1 协议(8)  -> baroAltitude    - 气压高度
//   HUD组1 协议(9)  -> indicatedAirspeed - 表速
//   HUD组1 协议(10) -> machNumber      - 马赫数
// ============================================================================
let hudGroup1Data = {
    // HUD组1 协议(1) - 俯仰角 (int16_t)
    // 显示部件：drawBackground() 中的 pitch 参数，用于绘制天空/地面和俯仰刻度
    pitchAngle: 0,
    
    // HUD组1 协议(2) - 滚转角 (int16_t)
    // 显示部件：drawRollScale() 中的 roll 参数，用于绘制顶部滚转刻度半圆
    rollAngle: 0,
    
    // HUD组1 协议(3) - 航向角 (int16_t)
    // 显示部件：drawHeadingTape() 中的 heading 参数，用于绘制底部航向带
    yawAngle: 0,
    
    // HUD组1 协议(4-1) - 法向过载 (int16_t)
    // 显示部件：drawBottomData() 中的 g 参数，显示在底部左侧
    normalOverload: 0,
    
    // HUD组1 协议(5) - 攻角 (int16_t)
    // 显示部件：drawBottomData() 中的 aoa 参数，显示在底部左侧
    attackAngle: 0,
    
    // HUD组1 协议(6) - 侧滑角 (int16_t)
    // 显示部件：暂未使用（预留）
    sideslipAngle: 0,
    
    // HUD组1 协议(7) - 发动机转速 (int16_t)
    // 显示部件：drawBottomData() 中的 rpm 参数，显示在底部右侧
    engineRPM: 0,
    
    // HUD组1 协议(8) - 气压高度 (int16_t)
    // 显示部件：drawValueRing() 中的 alt 参数，显示在右侧数值环
    baroAltitude: 0,
    
    // HUD组1 协议(9) - 表速 (int16_t)
    // 显示部件：drawValueRing() 中的 ias 参数，显示在左侧数值环
    indicatedAirspeed: 0,
    
    // HUD组1 协议(10) - 马赫数 (uint8_t，但按 int16_t 处理)
    // 显示部件：drawBottomData() 中的 mach 参数，显示在底部左侧
    machNumber: 0
};

// ============================================================================
// HUD层 主界面底部信息栏数据（根据 UdpData.h 中的协议定义）
// ============================================================================
let hudBottomInfoData = {
    gpsGroundSpeed: 0,
    gpsVerticalSpeed: 0,
    gpsHour: 0,
    gpsMinute: 0,
    gpsSecond: 0
};


// 兼容旧版 hudState（保留用于过渡）
let hudState = {
    pitch: 0, roll: 0, yaw: 0,     // 姿态
    ias: 0, tas: 0, alt: 0,        // 速度/高度
    mach: 0, aoa: 0, g: 1.0,       // 马赫/攻角/过载
    rpm: 0                          // 发动机转速
};

if (!mapEl) {
    console.error('Map element not found!');
} else {
    statusEl.textContent = statusText; // 注入的状态文本
}

// 瓦片相关（地图背景）
const tileSize = 256;                                // 单张瓦片像素尺寸
const minZoom = mapConfig.minZoom ?? 0;              // 最小缩放
const maxZoom = mapConfig.maxZoom ?? 18;             // 最大缩放
let zoom = Math.max(minZoom, Math.min(maxZoom, mapConfig.zoom ?? 10)); // 当前缩放
let center = { lat: mapConfig.centerLat ?? 0, lng: mapConfig.centerLng ?? 0 }; // 当前中心经纬度

// 飞机位置和航向数据（从UDP协议接收）
// 根据 UdpData.h：
//   longitude (34) - 卫星经度 (int32_t) - HUD层 地图飞机标识位置驱动1
//   latitude (35) - 卫星纬度 (int32_t) - HUD层 地图飞机标识位置驱动2
//   gpsCourse (31) - 卫星地速航向 (int16_t) - HUD层 地图飞机标识方向驱动1
let aircraftData = {
    longitude: null,   // 经度（度，C++端已缩放）
    latitude: null,    // 纬度（度，C++端已缩放）
    course: null       // 航向角（度，0-359，C++端已缩放）
};

// 飞机轨迹数据（用于绘制轨迹连线）
// 存储历史位置点，限制最大点数以避免内存过度占用
let aircraftTrail = [];
const MAX_TRAIL_POINTS = 1000000;  // 【调整参数】最大轨迹点数（示例计算如下，根据实际地面站计算机硬件性能调整）
// 内存占用示例计算：
//   每个轨迹点对象：{ lat: number, lng: number }
//   JavaScript中number是64位浮点数，占8字节
//   每个对象：2个number属性 = 16字节 + 对象元数据开销 ≈ 24字节
//   1000个点：1000 × 24字节 = 24,000字节 ≈ 24KB
//   实际内存占用可能略高（V8引擎对象开销），但不会超过50KB

// 轨迹点保存时间间隔控制
let lastTrailSaveTime = 0;  // 上次保存轨迹点的时间戳（毫秒）
const TRAIL_SAVE_INTERVAL_MS = 500;  // 【调整参数9】轨迹点保存间隔（毫秒）

// 鼠标拖拽相关状态
let dragging = false;                                // 是否正在拖动
let dragStart = { x: 0, y: 0 };                      // 鼠标按下时坐标
let dragStartCenter = { lat: 0, lng: 0 };            // 按下时的地图中心

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
    
    // 计算安全边距：确保即使旋转后也能覆盖整个可见区域
    // 使用对角线长度作为基础，乘以3倍确保完全覆盖（考虑最大旋转角度±90度）
    const diagonal = Math.sqrt(w * w + h * h);
    const safeMargin = diagonal * 3.0;  // 更大的安全边距，确保覆盖所有情况
    
    hudCtx.translate(centerX, centerY);
    hudCtx.rotate(roll * Math.PI / 180);
    const pitchPxPerDeg = 3.5;               //刻度间距，单位：像素/度
    const pitchOffset = pitch * pitchPxPerDeg;
    hudCtx.translate(0, pitchOffset);
    
    // 不设置裁剪区域，让 Canvas 自然处理
    // 但确保绘制区域足够大，覆盖所有可能的绘制范围

    // 填满整个HUD的天空和地面
    hudCtx.fillStyle = colors.sky;
    hudCtx.fillRect(-safeMargin, -safeMargin, safeMargin * 2, safeMargin);
    hudCtx.fillStyle = colors.ground;
    hudCtx.fillRect(-safeMargin, 0, safeMargin * 2, safeMargin);

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
    // 在绘制刻度线之前，设置一个足够大的裁剪区域
    // 确保所有刻度线都能绘制，即使部分超出画布边界
    const maxTickLength = 35;
    const tickSafeMargin = maxTickLength * 3;  // 足够大的安全边距
    hudCtx.beginPath();
    hudCtx.rect(-tickSafeMargin, -tickSafeMargin, tickSafeMargin * 2, tickSafeMargin * 2);
    hudCtx.clip();
    
    hudCtx.strokeStyle = colors.line;
    hudCtx.fillStyle = colors.text;
    hudCtx.font = '10px Consolas, monospace';
    hudCtx.lineWidth = 1.5;
    
    // 强制设置 lineCap 和 lineJoin，确保刻度线完整绘制
    hudCtx.lineCap = 'square';
    hudCtx.lineJoin = 'miter';
    
    for (let deg = -30; deg <= 30; deg += 2) {
        if (deg === 0) continue;
        const y = -deg * pitchPxPerDeg;
        const len = (Math.abs(deg) % 10 === 0) ? 35 : 27;
        
        // 绘制左侧刻度线（从 -len 到 -10）
        // 使用独立的路径，确保每条线都能完整绘制
        hudCtx.beginPath();
        hudCtx.moveTo(-len, y);
        hudCtx.lineTo(-10, y);
        hudCtx.stroke();
        
        // 绘制右侧刻度线（从 10 到 len）
        hudCtx.beginPath();
        hudCtx.moveTo(10, y);
        hudCtx.lineTo(len, y);
        hudCtx.stroke();
        
        // 绘制数字标签（只在10度倍数处）
        if (Math.abs(deg) % 10 === 0) {
            hudCtx.textAlign = 'right';
            hudCtx.fillText(`${Math.abs(deg)}`, -len - 4, y + 3);
            hudCtx.textAlign = 'left';
            hudCtx.fillText(`${Math.abs(deg)}`, len + 4, y + 3);
        }
    }
    
    // 恢复默认的 lineCap 和 lineJoin
    hudCtx.lineCap = 'butt';
    hudCtx.lineJoin = 'miter';

    hudCtx.restore();
}

// 机头中心标识（小十字，中心为空心小圆）
// 参数：cx, cy 为符号中心；colors.line 用于描边
function drawAircraftSymbol(cx, cy, colors) {
    hudCtx.strokeStyle = colors.line;
    hudCtx.lineWidth = 2;
    
    // ========================================================================
    // 尺寸参数（可根据需要调整）
    // ========================================================================
    const crossArmLengthHorizontal = 15;  // 十字臂长度（左右方向，像素）
    const crossArmLengthVertical = 10;    // 十字臂长度（上下方向，像素）
    const centerCircleRadius = 3;         // 中心空心圆半径（像素）
    // ========================================================================
    
    // 绘制十字（上下左右四个方向）
    hudCtx.beginPath();
    // 左侧横线
    hudCtx.moveTo(cx - crossArmLengthHorizontal, cy);
    hudCtx.lineTo(cx - centerCircleRadius, cy);
    // 右侧横线
    hudCtx.moveTo(cx + centerCircleRadius, cy);
    hudCtx.lineTo(cx + crossArmLengthHorizontal, cy);
    // 上方竖线
    hudCtx.moveTo(cx, cy - crossArmLengthVertical);
    hudCtx.lineTo(cx, cy - centerCircleRadius);
    // 下方竖线
    hudCtx.moveTo(cx, cy + centerCircleRadius);
    hudCtx.lineTo(cx, cy + crossArmLengthVertical);
    hudCtx.stroke();
    
    // 绘制中心空心圆
    hudCtx.beginPath();
    hudCtx.arc(cx, cy, centerCircleRadius, 0, Math.PI * 2);
    hudCtx.stroke();
}

// 速度矢量符号（现在作为机头指示符，固定在HUD中心位置）
// 参数：
//   cx, cy      : 符号中心（屏幕中心）
//   aoa         : 攻角（度，当前固定为0，不再使用）
//   sideslip    : 侧滑角（度，当前固定为0，不再使用）
//   pitchPxPerDeg : 俯仰刻度像素/度（保留参数，当前不使用）
//   roll        : 滚转角（度，用于坐标系变换，使符号随滚转角旋转）
//   colors      : 配色
function drawVelocityVector(cx, cy, aoa, sideslip, pitchPxPerDeg, roll, colors) {
    hudCtx.save();
    
    // ========================================================================
    // 尺寸参数（可根据需要调整）
    // ========================================================================
    const wingOuterLength = 33;     // 机翼外侧长度（左右两侧，像素）
    const wingInnerLength = 14;     // 机翼内侧长度（左右两侧，像素）
    const wingTipOffset = 7;        // 机翼尖端垂直偏移（像素）
    const centerDotRadius = 3;      // 中心点半径（像素）
    const noseOffset = 3;            // 机头垂直偏移（像素）
    // ========================================================================
    
    // 在滚转后的坐标系中计算速度矢量位置
    // 先移动到中心点，然后旋转坐标系（与drawBackground保持一致）
    hudCtx.translate(cx, cy);
    hudCtx.rotate(roll * Math.PI / 180);
    
    // 根据攻角和侧滑角计算速度矢量位置（在滚转后的坐标系中）
    // 攻角影响垂直位置（正攻角向上移动）
    // 侧滑角影响水平位置（正侧滑角向右移动）
    const aoaOffset = -aoa * pitchPxPerDeg;  // 攻角偏移（负号因为屏幕Y轴向下）
    const sideslipOffset = sideslip * pitchPxPerDeg;  // 侧滑角偏移
    
    // 计算速度矢量符号的中心位置（在滚转后的坐标系中）
    const vx = sideslipOffset;
    const vy = aoaOffset;
    
    hudCtx.strokeStyle = colors.line;
    hudCtx.lineWidth = 2;
    
    // 绘制速度矢量符号（恢复为原先的小机翼形状）
    hudCtx.beginPath();
    // 左侧机翼
    hudCtx.moveTo(vx - wingOuterLength, vy);
    hudCtx.lineTo(vx - wingInnerLength, vy);
    hudCtx.lineTo(vx - 8, vy + wingTipOffset);
    hudCtx.lineTo(vx, vy + noseOffset);
    // 机头
    hudCtx.lineTo(vx + 8, vy + wingTipOffset);
    hudCtx.lineTo(vx + wingInnerLength, vy);
    hudCtx.lineTo(vx + wingOuterLength, vy);
    hudCtx.stroke();
    
    // 绘制中心点
    hudCtx.beginPath();
    hudCtx.arc(vx, vy, centerDotRadius, 0, Math.PI * 2);  //
    hudCtx.stroke();
    
    hudCtx.restore();
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
    
    // 设置文字样式（在循环外设置一次，避免重复设置）
    hudCtx.fillStyle = colors.text;  // 确保使用文字颜色
    hudCtx.font = '10px Consolas, monospace';
    hudCtx.textAlign = 'center';

    // 先绘制每1度的小刻度（跳过5的倍数，避免与后续刻度重叠）
    hudCtx.strokeStyle = colors.line;
    for (let d = -45; d <= 45; d += 1) {
        // 跳过5的倍数，这些位置会被后续的短刻度和长刻度覆盖
        if (d % 5 === 0) continue;
        
        const x = centerX + d * pxPerDeg;
        if (x < left || x > left + width) continue;
        
        // 小刻度长度设为2，小于短刻度的4
        const tickLen = 2;
        
        hudCtx.beginPath();
        hudCtx.moveTo(x, y + height);
        hudCtx.lineTo(x, y + height - tickLen);
        hudCtx.stroke();
    }
    
    // 绘制所有刻度（每5度一个短刻度，每10度一个长刻度+数字）
    for (let d = -45; d <= 45; d += 5) {
        const hdgRaw = heading + d;
        const hdg = Math.round((hdgRaw + 360) % 360) + 1;
        const x = centerX + d * pxPerDeg;
        if (x < left || x > left + width) continue;
        
        // 判断是否为10度倍数：只使用 d % 10 === 0，避免重复绘制
        // d 是固定步长（每5度），所以 d % 10 === 0 的位置就是每10度的位置
        const isMajorTick = (d % 10 === 0);
        const tickLen = isMajorTick ? 8 : 4;
        
        // 绘制刻度线
        hudCtx.strokeStyle = colors.line;
        hudCtx.beginPath();
        hudCtx.moveTo(x, y + height);
        hudCtx.lineTo(x, y + height - tickLen);
        hudCtx.stroke();
        
        // 10度倍数处显示数字标签（只在 d % 10 === 0 时绘制，避免重复）
        if (isMajorTick) {
            // 确保 fillStyle 是文字颜色
            hudCtx.fillStyle = colors.text;
            // 计算要显示的航向值：确保是10的倍数
            // 使用 hdgRaw 计算，四舍五入到最近的10的倍数，并确保是0-359范围内的值
            const displayHdg = Math.round(hdgRaw / 10) * 10;
            const normalizedHdg = ((displayHdg % 360) + 360) % 360;  // 归一化到 0-359
            hudCtx.fillText(normalizedHdg.toString().padStart(3, '0'), x, y + height - 14);
        }
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
    
    let totalDistance = 0;  // 累计距离（米）
    
    // 遍历轨迹点，计算相邻两点间的距离
    for (let i = 1; i < aircraftTrail.length; i++) {
        const prev = aircraftTrail[i - 1];
        const curr = aircraftTrail[i];
        
        // 使用Haversine公式计算两点间的大圆距离
        const R = 6371000;  // 地球半径（米）
        const lat1Rad = prev.lat * Math.PI / 180;
        const lat2Rad = curr.lat * Math.PI / 180;
        const deltaLatRad = (curr.lat - prev.lat) * Math.PI / 180;
        const deltaLngRad = (curr.lng - prev.lng) * Math.PI / 180;
        
        const a = Math.sin(deltaLatRad / 2) * Math.sin(deltaLatRad / 2) +
                  Math.cos(lat1Rad) * Math.cos(lat2Rad) *
                  Math.sin(deltaLngRad / 2) * Math.sin(deltaLngRad / 2);
        const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
        const distance = R * c;  // 距离（米）
        
        totalDistance += distance;
    }
    
    return totalDistance;
}

// 底部信息栏（地速、垂直速度、GPS时间、累计距离）- 绘制在整个主界面底部
function drawBottomInfoBar(w, h, data, colors) {
    // 确保Canvas已初始化（解决启动时不显示的问题）
    if (!bottomInfoCtx || !bottomInfoCanvas) {
        initBottomInfoCanvas();
        if (!bottomInfoCtx) return;
    }
    
    // 每次绘制前都更新尺寸，确保窗口尺寸正确
    resizeBottomInfoCanvas();
    
    // 重新获取窗口尺寸
    const actualW = window.innerWidth;
    const actualH = window.innerHeight;
    
    //底部阴影高度设置
    const barHeight = 42;  // 高度减小4像素（从50改为46）
    const barY = actualH - barHeight;  // 从窗口底部往上46像素
    
    // 清空画布（使用实际窗口尺寸）
    bottomInfoCtx.clearRect(0, 0, actualW, actualH);
    
    // 绘制灰色透明背景条
    bottomInfoCtx.fillStyle = 'rgba(100, 100, 100, 0.7)';
    bottomInfoCtx.fillRect(0, barY, actualW, barHeight);
    
    // 设置文字样式
    bottomInfoCtx.fillStyle = '#ffffff';
    bottomInfoCtx.font = '14px Consolas, monospace';
    bottomInfoCtx.textAlign = 'center';  // 改为居中
    bottomInfoCtx.textBaseline = 'middle';  // 垂直居中
    
    // 准备四个数据项的文本
    const groundSpeed = (data && data.groundSpeed !== undefined) ? data.groundSpeed.toFixed(1) : '0.0';
    const verticalSpeed = (data && data.verticalSpeed !== undefined) ? data.verticalSpeed.toFixed(1) : '0.0';
    const hour = (data && data.gpsHour !== undefined) ? String(data.gpsHour).padStart(2, '0') : '00';
    const minute = (data && data.gpsMinute !== undefined) ? String(data.gpsMinute).padStart(2, '0') : '00';
    const second = (data && data.gpsSecond !== undefined) ? String(data.gpsSecond).padStart(2, '0') : '00';
    const gpsTime = hour + ':' + minute + ':' + second;
    
    // 计算累计飞行距离
    const totalDistance = calculateTotalDistance();
    let distanceText = '';
    if (totalDistance >= 1000) {
        distanceText = (totalDistance / 1000).toFixed(2) + ' km';
    } else {
        distanceText = totalDistance.toFixed(1) + ' m';
    }
    
    // 计算文本宽度
    const text1 = '-> ' + groundSpeed + ' m/s';
    const text2 = '^ ' + verticalSpeed + ' m/s';
    const text3 = 'GPS ' + gpsTime;
    const text4 = 'Dist ' + distanceText;
    const spacing = 30;  // 数据项之间的间距（像素）
    
    const text1Width = bottomInfoCtx.measureText(text1).width;
    const text2Width = bottomInfoCtx.measureText(text2).width;
    const text3Width = bottomInfoCtx.measureText(text3).width;
    const text4Width = bottomInfoCtx.measureText(text4).width;
    
    // 计算总宽度和起始位置（居中，使用实际窗口宽度）
    const totalWidth = text1Width + spacing + text2Width + spacing + text3Width + spacing + text4Width;
    const startX = (actualW - totalWidth) / 2 + text1Width / 2;
    const centerY = barY + barHeight / 2;  // 垂直居中
    
    // 绘制四个数据项，紧凑排列
    bottomInfoCtx.fillText(text1, startX, centerY);
    bottomInfoCtx.fillText(text2, startX + text1Width / 2 + spacing + text2Width / 2, centerY);
    bottomInfoCtx.fillText(text3, startX + text1Width / 2 + spacing + text2Width + spacing + text3Width / 2, centerY);
    bottomInfoCtx.fillText(text4, startX + text1Width / 2 + spacing + text2Width + spacing + text3Width + spacing + text4Width / 2, centerY);
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
    
    // 清除之前的裁剪区域，确保不受之前绘制的影响
    // 通过设置一个足够大的裁剪区域来"清除"之前的裁剪
    hudCtx.beginPath();
    hudCtx.rect(-10000, -10000, 20000, 20000);  // 足够大的裁剪区域
    hudCtx.clip();
    
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
    // 绘制基准弧线（半圆），可以通过调整 arcRange 参数控制两侧留白大小
    const scaleRange = 60;  // 刻度范围的一半（±60度）
    
    // 可调整参数：控制基准弧线的范围（相对于刻度范围）
    // arcRange 表示弧线覆盖的角度范围（度），默认值可以根据需要调整
    // 例如：如果 scaleRange=60，arcRange=120 表示弧线覆盖 ±60度（与刻度范围一致）
    //       arcRange=150 表示弧线覆盖 ±75度（两侧各多15度留白）
    //       arcRange=180 表示完整半圆（两侧各多30度留白）
    const arcRange = 150;  // 可调整：基准弧线的角度范围（度），默认150度（±75度）
    
    // 计算弧线的起始和结束角度
    // 角度转换：0度在顶部，顺时针为正（-90度偏移使0度在顶部）
    const arcHalfRange = arcRange / 2;  // 弧线范围的一半
    const startAngle = (-arcHalfRange - 90) * Math.PI / 180;  // 起始角度
    const endAngle = (arcHalfRange - 90) * Math.PI / 180;     // 结束角度
    
    // 绘制基准弧线
    hudCtx.beginPath();
    hudCtx.arc(0, 0, radius, startAngle, endAngle);
    hudCtx.stroke();

    // 刻度线：从半圆弧向外延伸
    hudCtx.fillStyle = colors.text;
    hudCtx.font = '10px Consolas, monospace';
    hudCtx.textAlign = 'center';
    
    // 设置线条样式（在循环外设置一次，避免重复设置）
    // 强制设置所有绘制属性，确保刻度线能正确绘制
    hudCtx.strokeStyle = colors.line;
    hudCtx.lineWidth = 1;
    hudCtx.lineCap = 'square';  // 确保刻度线端点完整
    hudCtx.lineJoin = 'miter';
    hudCtx.miterLimit = 10;
    
    // 不设置裁剪区域，让 Canvas 自然处理
    // 这样可以确保所有刻度线都能绘制，即使部分超出画布边界
    
    // 计算要显示的滚转角范围（以当前roll为中心）
    // scaleRange 已在上面定义（±60度）
    const minDisplayRoll = roll - scaleRange;  // 例如：roll=15时，minDisplayRoll=-45
    const maxDisplayRoll = roll + scaleRange;  // 例如：roll=15时，maxDisplayRoll=75
    
    // 计算需要显示的10的倍数范围
    const minDisplayRoll10 = Math.floor(minDisplayRoll / 10) * 10;  // 向下取整到10的倍数
    const maxDisplayRoll10 = Math.ceil(maxDisplayRoll / 10) * 10;   // 向上取整到10的倍数
    
    // 先绘制所有刻度线（每1度一个小刻度，每5度一个中等刻度，每10度一个长刻度）
    for (let deg = -scaleRange; deg <= scaleRange; deg += 1) {
        // 计算这个刻度位置对应的显示值（实际滚转角）
        let displayRoll = deg + roll;
        // 归一化到-180到180范围
        while (displayRoll > 180) displayRoll -= 360;
        while (displayRoll < -180) displayRoll += 360;
        
        // 角度转换：0度在顶部，顺时针为正（-90度偏移使0度在顶部）
        // 注意：使用 deg（相对位置）来计算角度，而不是 displayRoll
        // deg 范围：-60 到 +60，对应角度：-150度 到 -30度（相对于顶部）
        const rad = (deg - 90) * Math.PI / 180;
        
        // 验证角度是否有效（防止 NaN 或 Infinity）
        if (!isFinite(rad)) continue;
        
        const r1 = radius - 10;                   // 刻度起点（弧内侧）
        
        // 根据实际滚转角值（displayRoll）判断刻度类型，而不是相对位置（deg）
        // 这样可以确保刻度类型正确，即使 roll 角不是整数
        const nearest10 = Math.round(displayRoll / 10) * 10;
        const distTo10 = Math.abs(displayRoll - nearest10);
        const nearest5 = Math.round(displayRoll / 5) * 5;
        const distTo5 = Math.abs(displayRoll - nearest5);
        
        // 根据刻度类型决定长度
        let r2;
        if (distTo10 < 0.5) {
            // 非常接近10的倍数：最长刻度（用于显示数字的位置）
            r2 = radius + 10;
        } else if (distTo5 < 0.5) {
            // 非常接近5的倍数：中等刻度
            r2 = radius + 6;
        } else {
            // 其他：短刻度（每1度）
            r2 = radius + 2;
        }
        
        // 计算刻度线的起点和终点坐标
        const x1 = r1 * Math.cos(rad);
        const y1 = r1 * Math.sin(rad);
        const x2 = r2 * Math.cos(rad);
        const y2 = r2 * Math.sin(rad);
        
        // 验证坐标是否有效（防止 NaN 或 Infinity）
        if (!isFinite(x1) || !isFinite(y1) || !isFinite(x2) || !isFinite(y2)) {
            continue;
        }
        
        // 绘制刻度线（使用独立的路径，确保每条线都能完整绘制）
        // 强制设置 strokeStyle，确保绘制正确
        hudCtx.strokeStyle = colors.line;
        hudCtx.beginPath();
        hudCtx.moveTo(x1, y1);
        hudCtx.lineTo(x2, y2);
        hudCtx.stroke();
        
        // 强制刷新绘制（某些浏览器可能需要）
        // 注意：这可能会影响性能，但能确保绘制正确
    }
    
    // 恢复默认的 lineCap
    hudCtx.lineCap = 'butt';
    
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
            hudCtx.fillText(Math.abs(normalizedRoll).toString(), labelX, labelY + 4);
        }
    }

    hudCtx.restore();
}

function drawHud() {
    if (!hudCanvas || !hudCtx) return;
    const w = hudCanvas.clientWidth || 360; 
    const h = (hudCanvas.clientHeight || 360); // 与 resizeHud 保持一致
    hudCtx.clearRect(0, 0, w, h);
    if (!hudGroup1Data) return;

    // ========================================================================
    // 从 HUD组1 数据结构中提取显示值
    // ========================================================================
    // 使用转换函数将原始 int16_t 值转换为显示值
    // 注意：当前转换函数中的缩放系数需要根据实际协议调整
    // ========================================================================
    const displayData = convertHudGroup1Data(hudGroup1Data);
    
    // HUD组1 协议(1) - 俯仰角 → drawBackground() 中的 pitch 参数
    const pitch = displayData.pitchAngle;
    
    // HUD组1 协议(2) - 滚转角 → drawRollScale() 中的 roll 参数
    const roll = displayData.rollAngle;
    
    // HUD组1 协议(3) - 航向角 → drawHeadingTape() 中的 heading 参数
    const yaw = displayData.yawAngle;
    
    // HUD组1 协议(9) - 表速 → drawValueRing() 左侧数值环的 ias 参数
    const ias = displayData.indicatedAirspeed;
    
    // HUD组1 协议(8) - 气压高度 → drawValueRing() 右侧数值环的 alt 参数
    const alt = displayData.baroAltitude;
    
    // HUD组1 协议(10) - 马赫数 → drawBottomData() 中的 mach 参数
    const mach = displayData.machNumber;
    
    // HUD组1 协议(5) - 攻角 → drawBottomData() 中的 aoa 参数，以及速度矢量符号
    const aoa = displayData.attackAngle;
    
    // HUD组1 协议(6) - 侧滑角 → 速度矢量符号
    const sideslip = displayData.sideslipAngle;
    
    // HUD组1 协议(4-1) - 法向过载 → drawBottomData() 中的 g 参数
    const g = displayData.normalOverload;
    
    // HUD组1 协议(7) - 发动机转速 → drawBottomData() 中的 rpm 参数
    const rpm = displayData.engineRPM;
    
    // 俯仰刻度像素/度（用于速度矢量符号的位置计算）
    const pitchPxPerDeg = 3.5;  // 与 drawBackground 中的值保持一致

    const centerX = w / 2;
    // 布局：地平线居中；滚转在顶部；航向带靠底部
    const centerY = h / 2;      // 姿态中心（地平线在此高度）
    const rollY = 25;            // 滚转刻度顶部Y（画布顶部，正切）
    const rollRadius = w * 0.95 / 2; // 绘制半径 = 直径(360) / 2 * 75%
    const headingY = h - 31;    // 底部航向带 Y

    const colors = {
        // 典型PFD配色：亮蓝天空、土褐地面；线条/文字白，地平线短绿线
        sky: '#5fa8d3',
        ground: '#8b5a2b',
        line: '#f8f8f8',
        text: '#f8f8f8',
        ringDot: '#ffffff',
        horizon: '#6bff6b'
    };

    // ========================================================================
    // HUD 绘制部分 - 各显示部件对应的 HUD组1 协议字段
    // ========================================================================
    
    // HUD组1 协议(1) + (2) - 背景（天空/地面）+ 俯仰刻度 + 滚转
    // pitch: HUD组1 协议(1) - pitchAngle（俯仰角）
    // roll: HUD组1 协议(2) - rollAngle（滚转角）
    drawBackground(w, h, centerX, centerY, pitch, roll, colors);

    // HUD组1 协议(2) - 滚转刻度（顶部，正切于画布顶部，直径 = 360 * 0.95 = 342）
    // roll: HUD组1 协议(2) - rollAngle（滚转角）
    drawRollScale(centerX, rollY, rollRadius, roll, colors);

    // HUD组1 协议(3) - 航向带（靠底部上方）
    // yaw: HUD组1 协议(3) - yawAngle（航向角）
    drawHeadingTape(centerX, headingY, 240, 22, ((yaw % 360) + 360) % 360, colors);

    // 机头中心标识（小十字，固定显示在屏幕中心，不依赖数据）
    // drawAircraftSymbol(centerX, centerY, colors);  // 已注释：改用速度矢量符号作为机头指示符
    
    // 速度矢量符号（固定在HUD中心位置，作为机头指示符）
    // 注意：不再使用攻角和侧滑角数据移动位置，固定在中心
    // roll: HUD组1 协议(2) - rollAngle（滚转角，用于坐标系变换）
    drawVelocityVector(centerX, centerY, 0, 0, pitchPxPerDeg, roll, colors);

    // HUD组1 协议(9) - 左侧数值环（表速）
    // ias: HUD组1 协议(9) - indicatedAirspeed（表速）
    drawValueRing(60, centerY, Math.round(ias).toString(), 'IAS', colors);
    
    // HUD组1 协议(8) - 右侧数值环（气压高度）
    // alt: HUD组1 协议(8) - baroAltitude（气压高度）
    drawValueRing(w - 60, centerY, Math.round(alt).toString(), 'ALT', colors);

    // HUD组1 协议(10) + (5) + (4-1) + (7) - 底部数据栏
    // mach: HUD组1 协议(10) - machNumber（马赫数）
    // aoa: HUD组1 协议(5) - attackAngle（攻角）
    // g: HUD组1 协议(4-1) - normalOverload（法向过载）
    // rpm: HUD组1 协议(7) - engineRPM（发动机转速）
    drawBottomData(w, h, {
        mach: mach.toFixed(2),
        aoa: aoa.toFixed(1),
        g: g.toFixed(1),
        rpm: Math.round(rpm)
    }, colors);
    
    // 步骤4：绘制底部信息栏（地速、垂直速度、GPS时间）- 在整个主界面底部
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
    // 使用窗口尺寸而不是HUD画布尺寸（drawBottomInfoBar内部会重新获取实际尺寸）
    drawBottomInfoBar(window.innerWidth, window.innerHeight, bottomInfoData, colors);
}

// 初始化 HUD 尺寸并绘制一次（窗口尺寸变化时也重绘）
resizeHud();
drawHud();

// ============================================================================
// 数据转换函数：将 HUD组1 原始数据转换为显示值
// ============================================================================
// 说明：根据实际协议定义进行缩放转换
// 当前为占位符实现，用户需要根据实际协议调整缩放系数
// ============================================================================
function convertHudGroup1Data(rawData) {
    return {
        // HUD组1 协议(1) - 俯仰角：原始值转显示值（度）
        // 示例：如果协议中为 0.01度单位，则除以100；如果为1度单位，则除以1
        pitchAngle: rawData.pitchAngle / 1.0,  // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(2) - 滚转角：原始值转显示值（度）
        rollAngle: rawData.rollAngle / 1.0,    // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(3) - 航向角：原始值转显示值（度）
        yawAngle: rawData.yawAngle / 1.0,      // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(4-1) - 法向过载：原始值转显示值（g）
        // 示例：如果协议中为 0.1g单位，则除以10
        normalOverload: rawData.normalOverload / 10.0,  // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(5) - 攻角：原始值转显示值（度）
        attackAngle: rawData.attackAngle / 10.0,  // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(6) - 侧滑角：原始值转显示值（度）
        sideslipAngle: rawData.sideslipAngle / 1.0,  // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(7) - 发动机转速：原始值转显示值（RPM）
        engineRPM: rawData.engineRPM / 1.0,     // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(8) - 气压高度：原始值转显示值（米）
        baroAltitude: rawData.baroAltitude / 1.0,  // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(9) - 表速：原始值转显示值（m/s 或 km/h）
        indicatedAirspeed: rawData.indicatedAirspeed / 1.0,  // TODO: 根据实际协议调整缩放系数
        
        // HUD组1 协议(10) - 马赫数：原始值转显示值（无量纲）
        // 示例：如果协议中为百分比（0-100），则除以100；如果为0.01单位，则除以100
        machNumber: rawData.machNumber / 100.0  // TODO: 根据实际协议调整缩放系数
    };
}

// ============================================================================
// 接收 native HUD 数据（C++ PostWebMessageAsJson 推送）
// ============================================================================
if (window.chrome && window.chrome.webview) {
    window.chrome.webview.addEventListener('message', (e) => {
        const receivedData = e.data || null;
        if (!receivedData) {
            return;
        }
        
        // ====================================================================
        // 从接收到的 JSON 数据中提取 HUD组1 协议字段
        // ====================================================================
        // 注意：C++ 端发送的字段名可能与协议字段名不同，需要映射
        // 当前映射关系（根据 SendHudMessage 函数）：
        //   pitch -> pitchAngle
        //   roll -> rollAngle
        //   yaw -> yawAngle
        //   g -> normalOverload
        //   aoa -> attackAngle
        //   rpm -> engineRPM
        //   alt -> baroAltitude
        //   ias -> indicatedAirspeed
        //   mach -> machNumber
        // ====================================================================
        
        // 更新 HUD组1 原始数据（保持 int16_t 原始值，不进行缩放）
        // 注意：如果 C++ 端已经发送了缩放后的值，这里需要反向缩放或直接使用
        hudGroup1Data.pitchAngle = receivedData.pitch ?? 0;
        hudGroup1Data.rollAngle = receivedData.roll ?? 0;
        hudGroup1Data.yawAngle = receivedData.yaw ?? 0;
        hudGroup1Data.normalOverload = receivedData.g ?? 0;
        hudGroup1Data.attackAngle = receivedData.aoa ?? 0;
        hudGroup1Data.sideslipAngle = receivedData.sideslipAngle ?? 0;  // 如果 C++ 端发送了此字段
        hudGroup1Data.engineRPM = receivedData.rpm ?? 0;
        hudGroup1Data.baroAltitude = receivedData.alt ?? 0;
        hudGroup1Data.indicatedAirspeed = receivedData.ias ?? 0;
        hudGroup1Data.machNumber = receivedData.mach ?? 0;
        
        // 步骤2：更新底部信息栏数据
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
        
        // 更新飞机位置和航向数据（地图飞机标识）
        // C++端已经做了缩放：
        //   longitude = pPacket->longitude / 100000.0f
        //   latitude = pPacket->latitude / 100000.0f
        //   gpsCourse = pPacket->gpsCourse / 10.0f
        // JS端直接使用C++端已经缩放后的值
        if (receivedData.longitude !== undefined) {
            aircraftData.longitude = receivedData.longitude;  // C++端已缩放，直接使用
        }
        
        if (receivedData.latitude !== undefined) {
            aircraftData.latitude = receivedData.latitude;  // C++端已缩放，直接使用
        }
        
        if (receivedData.gpsCourse !== undefined) {
            aircraftData.course = receivedData.gpsCourse;  // C++端已缩放，直接使用
            // 归一化到0-359度范围
            aircraftData.course = ((aircraftData.course % 360) + 360) % 360;
        }
        
        // 兼容旧版 hudState（保留用于过渡）
        hudState = receivedData;
        
        // 重新绘制 HUD
        drawHud();
        
        // 重新绘制飞机标识
        if (aircraftCanvas && aircraftCtx) {
            drawAircraftOnMap(aircraftData.latitude, aircraftData.longitude, aircraftData.course);
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

// 初始化地图飞机标识Canvas覆盖层
function initAircraftCanvas() {
    if (!mapEl) {
        console.warn('initAircraftCanvas: mapEl not found');
        return;
    }
    
    // 检查是否已存在Canvas
    if (!aircraftCanvas) {
        aircraftCanvas = document.createElement('canvas');
        aircraftCanvas.id = 'aircraft-overlay';
        aircraftCanvas.style.position = 'absolute';
        aircraftCanvas.style.top = '0';
        aircraftCanvas.style.left = '0';
        aircraftCanvas.style.width = '100%';
        aircraftCanvas.style.height = '100%';
        aircraftCanvas.style.pointerEvents = 'none';  // 不阻挡地图交互
        aircraftCanvas.style.zIndex = '15';  // 在地图瓦片之上，HUD之下
        mapEl.appendChild(aircraftCanvas);
        aircraftCtx = aircraftCanvas.getContext('2d');
        
        if (!aircraftCtx) {
            console.error('initAircraftCanvas: Failed to get 2d context');
            return;
        }
        
        console.log('initAircraftCanvas: Canvas initialized successfully');
    }
    
    // 调整Canvas尺寸
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
// 参数：
//   lat, lng  : 飞机经纬度（度）
//   course    : 航向角（度，0-359，0度为北，顺时针为正）
function drawAircraftOnMap(lat, lng, course) {
    if (!aircraftCanvas || !aircraftCtx || !mapEl) {
        console.warn('drawAircraftOnMap: Canvas or context not initialized');
        return;
    }
    
    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    
    // 清空Canvas
    aircraftCtx.clearRect(0, 0, width, height);
    
    // 检查数据是否有效
    if (lat === null || lng === null || course === null || 
        isNaN(lat) || isNaN(lng) || isNaN(course)) {
        // 即使当前数据无效，也绘制轨迹（如果有历史数据）
        drawTrail();
        return;
    }
    
    // 将飞机经纬度转换为地图像素坐标
    const aircraftPoint = latLngToPoint(lat, lng, zoom);
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    
    // 计算飞机在地图Canvas上的位置
    const aircraftX = aircraftPoint.x - topLeft.x;
    const aircraftY = aircraftPoint.y - topLeft.y;
    
    // 更新轨迹数据（每3秒保存一个位置点）
    // 注意：只存储经纬度，不存储像素坐标（像素坐标在绘制时实时计算）
    const currentTime = Date.now();
    
    // 检查是否到了保存时间间隔（每3秒保存一次）
    if (aircraftTrail.length === 0 || (currentTime - lastTrailSaveTime) >= TRAIL_SAVE_INTERVAL_MS) {
        const currentPoint = { lat, lng };
        aircraftTrail.push(currentPoint);
        lastTrailSaveTime = currentTime;
        
        // 限制轨迹点数（避免内存过度占用）
        // 内存占用：1000个点 × 24字节 ≈ 24KB
        if (aircraftTrail.length > MAX_TRAIL_POINTS) {
            aircraftTrail.shift();  // 移除最旧的点
        }
    }
    
    // 绘制轨迹连线
    drawTrail();
    
    // 检查飞机是否在地图可见区域内（留50像素边距）
    if (aircraftX < -50 || aircraftX > width + 50 || aircraftY < -50 || aircraftY > height + 50) {
        return;  // 飞机不在可见区域，不绘制箭头
    }
    
    // ========================================================================
    // 等腰三角形箭头尺寸参数（可根据需要调整）
    // ========================================================================
    // 【调整参数】箭头总长度
    const arrowLength = 28;        // 箭头总长度（像素）
    
    // 【调整参数】等腰三角形宽度（调整这个参数可以让等腰部分更宽或更窄）
    const arrowBottomWidth = 38;    // 箭头底部宽度（像素，指底部外角的宽度）
                                     // 增大此值 → 等腰部分更宽
                                     // 减小此值 → 等腰部分更窄
    
    // 【调整参数】底部位置
    const arrowBottomY = 24;        // 底部外角Y坐标（从顶部算起）
    
    // 【调整参数】底部收缩深度（调整这个参数可以让底部收缩更多或更少）
    const arrowBottomIndent = 8;    // 底部向内收的深度（像素，V形凹陷）
                                     // 增大此值 → 底部收缩更多（V形凹陷更深）
                                     // 减小此值 → 底部收缩更少（V形凹陷更浅）
    // ========================================================================
    
    aircraftCtx.save();
    aircraftCtx.translate(aircraftX, aircraftY);
    
    // 航向角转换：GPS航向角0度为北（向上），顺时针为正
    // Canvas坐标系：0度为右（向右），逆时针为正
    // 需要转换：canvasAngle = -gpsAngle + 90
    const canvasAngle = -course + 90;
    aircraftCtx.rotate(canvasAngle * Math.PI / 180);
    
    // 绘制等腰三角形箭头（底边往里收）
    // 等腰三角形：两条边相等，底边中间往里收（V形凹陷）
    
    // 1. 绘制红色主体（基础等腰三角形）
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = 'transparent';
    
    // 创建基础红色填充
    aircraftCtx.fillStyle = '#cc0000';
    aircraftCtx.beginPath();
    // 等腰三角形：顶部尖锐点 -> 左侧底部外角（直线边） -> 底部中心（往里收，V形凹陷） -> 右侧底部外角（直线边） -> 回到顶部
    aircraftCtx.moveTo(0, -arrowLength);  // 顶部尖锐点
    aircraftCtx.lineTo(-arrowBottomWidth / 2, arrowBottomY);  // 左侧底部外角（等腰三角形的左侧边）
    aircraftCtx.lineTo(0, arrowBottomY - arrowBottomIndent);  // 底部中心（往里收，V形凹陷）
    aircraftCtx.lineTo(arrowBottomWidth / 2, arrowBottomY);  // 右侧底部外角（等腰三角形的右侧边）
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    // 2. 绘制中间垂直高光带（对称轴切割效果）
    // 高光带从顶部下方更远的位置开始，避免在顶部形成三角形阴影
    const centerHighlightWidth = arrowBottomWidth * 0.3;  // 高光宽度
    const bottomHighlightY = arrowBottomY - arrowBottomIndent;  // 底部中心Y坐标
    const highlightStartY = -arrowLength + 8;  // 【调整参数7】高光起始位置（从顶部下方开始，增大此值可让高光更靠下，减少前端阴影）
    
    const centerHighlight = aircraftCtx.createLinearGradient(-centerHighlightWidth / 2, highlightStartY, centerHighlightWidth / 2, bottomHighlightY);
    centerHighlight.addColorStop(0, 'rgba(255, 200, 200, 0.5)');  // 起始位置较亮（降低不透明度）
    centerHighlight.addColorStop(0.3, 'rgba(255, 160, 160, 0.4)');
    centerHighlight.addColorStop(0.7, 'rgba(255, 120, 120, 0.3)');
    centerHighlight.addColorStop(1, 'rgba(255, 80, 80, 0.2)');  // 底部较暗
    
    aircraftCtx.fillStyle = centerHighlight;
    aircraftCtx.beginPath();
    // 高光带形状（从顶部下方开始，逐渐变窄）
    const topHighlightWidth = centerHighlightWidth;
    const bottomHighlightWidth = centerHighlightWidth * 0.2;
    aircraftCtx.moveTo(-topHighlightWidth / 2, highlightStartY);
    aircraftCtx.lineTo(topHighlightWidth / 2, highlightStartY);
    aircraftCtx.lineTo(bottomHighlightWidth / 2, bottomHighlightY);
    aircraftCtx.lineTo(-bottomHighlightWidth / 2, bottomHighlightY);
    aircraftCtx.closePath();
    aircraftCtx.fill();
    
    // 3. 添加边缘阴影效果（两侧较暗）
    const edgeShadow = aircraftCtx.createLinearGradient(-arrowBottomWidth / 2, arrowBottomY, 0, arrowBottomY);
    edgeShadow.addColorStop(0, 'rgba(0, 0, 0, 0.4)');  // 边缘较暗
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
    
    // 4. 绘制对称轴连线（从顶部到底部的垂直线）
    // 【调整参数8】对称轴连线样式
    const axisLineWidth = 1.5;  // 对称轴连线宽度（像素）
    const axisLineColor = '#ffffff';  // 对称轴连线颜色
    const axisLineOpacity = 0.8;  // 对称轴连线不透明度（0-1）
    
    aircraftCtx.strokeStyle = axisLineColor;
    aircraftCtx.globalAlpha = axisLineOpacity;
    aircraftCtx.lineWidth = axisLineWidth;
    aircraftCtx.shadowBlur = 0;
    aircraftCtx.shadowColor = 'transparent';
    aircraftCtx.lineCap = 'round';
    
    aircraftCtx.beginPath();
    aircraftCtx.moveTo(0, -arrowLength);  // 从顶部开始
    aircraftCtx.lineTo(0, bottomHighlightY);  // 到底部中心（V形凹陷处）
    aircraftCtx.stroke();
    
    // 恢复全局透明度
    aircraftCtx.globalAlpha = 1.0;
    
    // 5. 绘制白色发光边缘（外圈）
    // 注意：shadowBlur会在所有方向产生阴影，包括前端，可能导致前端出现三角形阴影
    // 如果需要移除前端阴影，可以减小shadowBlur或移除它
    aircraftCtx.strokeStyle = '#ffffff';
    aircraftCtx.lineWidth = 2;
    aircraftCtx.shadowBlur = 0;  // 【调整参数5】白色边缘发光效果，设为0可完全移除阴影
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

// 绘制飞机轨迹连线
function drawTrail() {
    if (!aircraftCtx || aircraftTrail.length < 2) return;
    
    const width = mapEl.clientWidth;
    const height = mapEl.clientHeight;
    const centerPoint = latLngToPoint(center.lat, center.lng, zoom);
    const topLeft = { x: centerPoint.x - width / 2, y: centerPoint.y - height / 2 };
    
    // 绘制轨迹连线
    aircraftCtx.strokeStyle = '#ff6b6b';  // 红色轨迹线
    aircraftCtx.lineWidth = 2;
    aircraftCtx.lineCap = 'round';
    aircraftCtx.lineJoin = 'round';
    
    aircraftCtx.beginPath();
    let firstPoint = true;
    
    for (let i = 0; i < aircraftTrail.length; i++) {
        // 实时计算每个轨迹点的像素坐标（地图缩放/移动时自动更新）
        const point = latLngToPoint(aircraftTrail[i].lat, aircraftTrail[i].lng, zoom);
        const x = point.x - topLeft.x;
        const y = point.y - topLeft.y;
        
        // 只绘制可见区域内的点
        if (x >= -10 && x <= width + 10 && y >= -10 && y <= height + 10) {
            if (firstPoint) {
                aircraftCtx.moveTo(x, y);
                firstPoint = false;
            } else {
                aircraftCtx.lineTo(x, y);
            }
        } else if (!firstPoint) {
            // 如果点不在可见区域，但之前有点在可见区域，需要断开连线
            firstPoint = true;
        }
    }
    aircraftCtx.stroke();
}

// 瓦片渲染：根据中心点和缩放计算需要的瓦片并加载
function render() {
    if (!mapEl) return;
    try {
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
        
        // 绘制飞机标识（有数据时才绘制）
        if (aircraftCanvas && aircraftCtx && aircraftData.latitude !== null && aircraftData.longitude !== null && aircraftData.course !== null) {
            drawAircraftOnMap(aircraftData.latitude, aircraftData.longitude, aircraftData.course);
        }
    } catch (err) {
        console.error('Error in render:', err);
    }
}

if (mapEl) {
    // 鼠标按下：记录起点与中心，开始拖拽
    mapEl.addEventListener('mousedown', (e) => {
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

    // 鼠标抬起：结束拖拽
    document.addEventListener('mouseup', () => {
        if (!dragging) return;
        dragging = false;
        if (mapEl) {
            mapEl.style.cursor = 'grab';
        }
    });

    // 滚轮缩放：以鼠标位置为锚点，缩放并重新计算中心
    mapEl.addEventListener('wheel', (e) => {
        e.preventDefault();
        const rect = mapEl.getBoundingClientRect();
        const mouseX = e.clientX - rect.left;
        const mouseY = e.clientY - rect.top;
        if (mouseX < 0 || mouseX > rect.width || mouseY < 0 || mouseY > rect.height) {
            return;
        }

        const delta = e.deltaY < 0 ? 1 : -1;
        const nextZoom = Math.max(minZoom, Math.min(maxZoom, zoom + delta));
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
    }, { passive: false });

    mapEl.style.cursor = 'grab';
}

// 窗口大小改变时，调整所有Canvas尺寸并重新渲染
window.addEventListener('resize', () => {
    resizeHud();
    resizeBottomInfoCanvas();
    resizeAircraftCanvas();
    drawHud();
    render();
});

// 初始化飞机标识Canvas
initAircraftCanvas();

// 初始化底部信息栏Canvas（延迟初始化，确保窗口尺寸已确定）
// 使用 setTimeout 确保在窗口完全加载后再初始化
setTimeout(() => {
    initBottomInfoCanvas();
    // 立即绘制一次，确保显示
    if (typeof drawHud === 'function') {
        drawHud();
    }
}, 100);

// 初始渲染
render();
