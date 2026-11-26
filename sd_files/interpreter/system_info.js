var display = require("display");
var keyboard = require("keyboard");
var device = require("device");

// System Info App
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(20, 25, 35);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(76, 175, 80);
var YELLOW = display.color(255, 193, 7);
var RED = display.color(244, 67, 54);
var CYAN = display.color(0, 188, 212);
var GRAY = display.color(120, 120, 120);

var shouldExit = false;
var page = 0;
var maxPages = 2;

function getBatteryColor(level) {
  if (level > 50) return GREEN;
  if (level > 20) return YELLOW;
  return RED;
}

function drawBatteryBar(x, y, w, h, level) {
  display.drawRect(x, y, w, h, WHITE);
  display.drawFillRect(
    x + 2,
    y + 2,
    Math.floor(((w - 4) * level) / 100),
    h - 4,
    getBatteryColor(level)
  );
}

function drawPage0() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.drawString("SYSTEM INFO", screenWidth / 2 - 35, 5);

  var battery = device.getBatteryCharge();
  var board = device.getBoard();

  display.setTextColor(WHITE);
  display.drawString("Board: " + board, 10, 25);

  display.drawString("Battery: " + battery + "%", 10, 45);
  drawBatteryBar(10, 58, 100, 12, battery);

  display.setTextColor(GRAY);
  display.drawString("Screen: " + screenWidth + "x" + screenHeight, 10, 80);

  display.setTextColor(WHITE);
  display.drawString("Page 1/2", screenWidth - 50, screenHeight - 12);
}

function drawPage1() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.drawString("MEMORY & TIME", screenWidth / 2 - 40, 5);

  display.setTextColor(WHITE);
  var d = new Date();
  var timeStr =
    d.getHours() + ":" + (d.getMinutes() < 10 ? "0" : "") + d.getMinutes();
  var dateStr = d.getFullYear() + "-" + (d.getMonth() + 1) + "-" + d.getDate();

  display.drawString("Time: " + timeStr, 10, 30);
  display.drawString("Date: " + dateStr, 10, 50);

  display.setTextColor(GRAY);
  display.drawString("Uptime: " + Math.floor(Date.now() / 1000) + "s", 10, 75);

  display.setTextColor(WHITE);
  display.drawString("Page 2/2", screenWidth - 50, screenHeight - 12);
}

function drawScreen() {
  if (page === 0) drawPage0();
  else drawPage1();

  display.setTextColor(GRAY);
  display.setTextSize(1);
  display.drawString("<PREV  NEXT>", screenWidth / 2 - 40, screenHeight - 25);
}

drawScreen();

while (!shouldExit) {
  if (keyboard.getPrevPress()) {
    page = (page - 1 + maxPages) % maxPages;
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    page = (page + 1) % maxPages;
    drawScreen();
  }
  if (keyboard.getSelPress()) {
    shouldExit = true;
  }

  delay(100);
}
