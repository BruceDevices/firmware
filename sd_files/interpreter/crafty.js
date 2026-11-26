// CURSEDSOUND // CRAFTY remote for Bruce (M5StickC Plus2)
// Controls a Storz & Bickel CRAFTY / CRAFTY+ over BLE.
// Needs the ble.connect/read/write GATT functions (Bruce fork "ble-gatt-client").
//
// Buttons:  M5 (front) = select TEMP / BOOST / HEAT
//           B  (right) = +   (on HEAT: on)
//           PWR (left) = -   (on HEAT: off)
//           B + PWR together = exit

var display = require('display');
var keyboard = require('keyboard');
var ble = require('ble');

var BASE = '-4c45-4b43-4942-265a524f5453';
var SVC1 = '00000001' + BASE;
var SVC2 = '00000002' + BASE;
var SVC3 = '00000003' + BASE;
var CUR_TEMP = '00000011' + BASE;
var TARGET = '00000021' + BASE;
var BOOST = '00000031' + BASE;
var BATTERY = '00000041' + BASE;
var HEAT_ON = '00000081' + BASE;
var HEAT_OFF = '00000091' + BASE;
var SERIAL = '00000052' + BASE;
var PRJ = '00000093' + BASE;
var PRJ2 = '000001c3' + BASE;

var BLACK = display.color(4, 4, 8);
var PANEL = display.color(18, 18, 24);
var ACID = display.color(200, 255, 0);
var HOT = display.color(255, 46, 136);
var WHITE = display.color(255, 255, 255);
var DIM = display.color(110, 110, 120);

var W = display.width();
var H = display.height();
var spr = display.createSprite();

function rd16(svc, chr) {
  var v = ble.read(svc, chr);
  if (!v || v.length < 2) return -1;
  return v[0] | (v[1] << 8);
}

function wr16(svc, chr, val) {
  return ble.write(svc, chr, new Uint8Array([val & 255, (val >> 8) & 255]));
}

function tenths(v) {
  return Math.round(v / 10);
}

// ---------------------------------------------------------------- drawing
function glitchTitle(x, y, size) {
  spr.setTextSize(size);
  spr.setTextAlign('left', 'top');
  var g = Math.floor(now() / 120) % 19 === 0;
  var dx = g ? 3 : 1;
  spr.setTextColor(HOT);
  spr.drawText('CURSEDSOUND', x - dx, y + (g ? 1 : 0));
  spr.setTextColor(ACID);
  spr.drawText('CURSEDSOUND', x + dx, y);
  spr.setTextColor(WHITE);
  spr.drawText('CURSEDSOUND', x, y);
}

function message(line1, line2, color) {
  spr.fill(BLACK);
  glitchTitle(8, 8, 2);
  spr.setTextAlign('center', 'middle');
  spr.setTextSize(2);
  spr.setTextColor(color || WHITE);
  spr.drawText(line1, W / 2, H / 2 + 6);
  if (line2) {
    spr.setTextSize(1);
    spr.setTextColor(DIM);
    spr.drawText(line2, W / 2, H / 2 + 30);
  }
  spr.pushSprite();
}

function box(x, y, w, h, label, value, selected, accent) {
  if (selected) {
    spr.drawFillRect(x, y, w, h, accent);
  } else {
    spr.drawFillRect(x, y, w, h, PANEL);
    spr.drawRect(x, y, w, h, accent);
  }
  spr.setTextAlign('center', 'top');
  spr.setTextSize(1);
  spr.setTextColor(selected ? BLACK : DIM);
  spr.drawText(label, x + w / 2, y + 4);
  spr.setTextSize(2);
  spr.setTextColor(selected ? BLACK : WHITE);
  spr.drawText(value, x + w / 2, y + 16);
}

function draw(s) {
  spr.fill(BLACK);

  // header
  glitchTitle(6, 4, 1);
  spr.setTextAlign('right', 'top');
  spr.setTextSize(1);
  spr.setTextColor(s.battery < 20 ? HOT : ACID);
  spr.drawText('BAT ' + s.battery + '%', W - 6, 4);

  // big current temperature
  var hot = s.heater;
  var big = s.cur > 0 ? String(s.cur) : '---';
  spr.setTextAlign('center', 'middle');
  spr.setTextSize(5);
  spr.setTextColor(s.ready ? ACID : WHITE);
  spr.drawText(big, W / 2 - 8, 42);
  spr.drawCircle(W / 2 - 8 + big.length * 15 + 7, 27, 4, s.ready ? ACID : WHITE);

  // status + mode
  spr.setTextSize(1);
  spr.setTextColor(s.ready ? ACID : (hot ? HOT : DIM));
  var status = s.ready ? 'READY' : (hot ? 'HEATING' : 'IDLE');
  if (s.mode !== 'NORMAL' && hot) status = status + ' / ' + s.mode;
  spr.drawText(status, W / 2, 68);

  // progress bar current -> target
  var bx = 10, by = 78, bw = W - 20;
  spr.drawFillRect(bx, by, bw, 4, PANEL);
  var frac = Math.max(0, Math.min(1, (s.cur - 40) / (s.target + s.boost - 40)));
  if (s.cur > 0) spr.drawFillRect(bx, by, Math.round(bw * frac), 4, hot ? HOT : ACID);

  // three controls
  var bwid = Math.floor((W - 24) / 3);
  var y = 88;
  box(6, y, bwid, 40, 'TEMP', s.target + 'C', s.sel === 0, ACID);
  box(12 + bwid, y, bwid, 40, 'BOOST', '+' + s.boost, s.sel === 1, HOT);
  box(18 + bwid * 2, y, bwid, 40, 'HEAT', hot ? 'ON' : 'OFF', s.sel === 2, hot ? HOT : ACID);

  spr.pushSprite();
}

// ---------------------------------------------------------------- device
function connect() {
  while (true) {
    message('SEARCHING...', 'turn CRAFTY on, close other apps', ACID);
    if (ble.connect('STORZ&BICKEL', 6)) return true;
    message('NOT FOUND', 'M5: retry   B+PWR: exit', HOT);
    while (true) {
      if (keyboard.getEscPress()) return false;
      if (keyboard.getSelPress()) break;
      delay(40);
    }
  }
}

function readState(s, full) {
  var v;
  v = rd16(SVC1, CUR_TEMP);
  if (v >= 0) s.cur = tenths(v);
  v = rd16(SVC1, BATTERY);
  if (v >= 0) s.battery = v;
  v = rd16(SVC3, PRJ);
  if (v >= 0) {
    s.heater = (v & 0x10) !== 0;
    s.mode = (v & 0x40) ? 'SUPERBOOST' : ((v & 0x20) ? 'BOOST' : 'NORMAL');
  }
  v = rd16(SVC3, PRJ2);
  if (v >= 0) s.ready = (v & 0x04) !== 0 && s.heater;
  if (full) {
    v = rd16(SVC1, TARGET);
    if (v >= 0) {
      var t = tenths(v);
      if (t > 210) t = Math.round((t - 32) / 1.8);
      s.target = t;
    }
    v = rd16(SVC1, BOOST);
    if (v >= 0) s.boost = tenths(v);
  }
}

function clampBoost(s) {
  if (s.boost < 0) s.boost = 0;
  if (s.boost > 99) s.boost = 99;
  if (s.target + s.boost > 210) s.boost = 210 - s.target;
}

function main() {
  if (!connect()) return;

  var s = {
    cur: 0, target: 180, boost: 10, battery: 0,
    heater: false, ready: false, mode: 'NORMAL', sel: 0
  };
  readState(s, true);

  var lastPoll = 0;
  var lastFull = now();
  var pendingAt = 0;      // when a temp/boost change should be sent
  var dirty = true;

  while (true) {
    var t = now();

    // ---- input
    if (keyboard.getEscPress()) break;
    if (keyboard.getSelPress()) { s.sel = (s.sel + 1) % 3; dirty = true; }
    var up = keyboard.getNextPress();
    var down = keyboard.getPrevPress();
    if (up || down) {
      var d = up ? 1 : -1;
      if (s.sel === 0) {
        s.target = Math.max(40, Math.min(210, s.target + d));
        clampBoost(s);
        pendingAt = t + 700;
      } else if (s.sel === 1) {
        s.boost = s.boost + d;
        clampBoost(s);
        pendingAt = t + 700;
      } else {
        ble.write(SVC1, up ? HEAT_ON : HEAT_OFF, new Uint8Array([0, 0]));
        s.heater = up;
      }
      dirty = true;
    }

    // ---- send pending temperature / boost once buttons are released
    if (pendingAt && t >= pendingAt) {
      wr16(SVC1, TARGET, s.target * 10);
      wr16(SVC1, BOOST, s.boost * 10);
      pendingAt = 0;
      lastFull = t;
    }

    // ---- poll device
    if (t - lastPoll > 1000 && !pendingAt) {
      if (!ble.isConnected()) {
        message('LINK LOST', 'reconnecting...', HOT);
        if (!connect()) break;
        readState(s, true);
      } else {
        var full = t - lastFull > 5000;
        readState(s, full);
        if (full) lastFull = t;
      }
      lastPoll = now();
      dirty = true;
    }

    if (dirty) {
      draw(s);
      dirty = false;
    }
    delay(30);
  }
}

main();
ble.disconnect();
spr.deleteSprite();
display.fill(0);
