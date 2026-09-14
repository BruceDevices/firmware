var display = require("display");
var keyboard = require("keyboard");

// Color Picker / RGB Viewer
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(30, 30, 30);
var WHITE = display.color(255, 255, 255);
var GRAY = display.color(100, 100, 100);

var r = 255;
var g = 128;
var b = 0;
var editChannel = 0; // 0=R, 1=G, 2=B
var shouldExit = false;

function toHex2(n) {
  var hex = "0123456789ABCDEF";
  return hex.charAt(Math.floor(n / 16)) + hex.charAt(n % 16);
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  
  // Color preview
  display.drawFillRect(10, 10, 80, 60, display.color(r, g, b));
  display.drawRect(10, 10, 80, 60, WHITE);
  
  // Hex value
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawString("#" + toHex2(r) + toHex2(g) + toHex2(b), 100, 15);
  
  // RGB values
  display.setTextColor(editChannel === 0 ? display.color(255, 100, 100) : GRAY);
  display.drawString("R: " + r, 100, 35);
  
  display.setTextColor(editChannel === 1 ? display.color(100, 255, 100) : GRAY);
  display.drawString("G: " + g, 100, 50);
  
  display.setTextColor(editChannel === 2 ? display.color(100, 100, 255) : GRAY);
  display.drawString("B: " + b, 100, 65);
  
  // Sliders
  var sliderY = 80;
  var sliderW = screenWidth - 40;
  
  // R slider
  display.drawRect(20, sliderY, sliderW, 8, display.color(100, 50, 50));
  display.drawFillRect(20, sliderY, Math.floor(sliderW * r / 255), 8, display.color(255, 0, 0));
  
  // G slider
  display.drawRect(20, sliderY + 12, sliderW, 8, display.color(50, 100, 50));
  display.drawFillRect(20, sliderY + 12, Math.floor(sliderW * g / 255), 8, display.color(0, 255, 0));
  
  // B slider
  display.drawRect(20, sliderY + 24, sliderW, 8, display.color(50, 50, 100));
  display.drawFillRect(20, sliderY + 24, Math.floor(sliderW * b / 255), 8, display.color(0, 0, 255));
  
  // Controls
  display.setTextColor(WHITE);
  display.drawString("SEL:Channel PREV/NEXT:Value", 5, screenHeight - 12);
}

drawScreen();

while (!shouldExit) {
  if (keyboard.getSelPress()) {
    editChannel = (editChannel + 1) % 3;
    drawScreen();
  }
  if (keyboard.getPrevPress()) {
    if (editChannel === 0) r = Math.max(0, r - 5);
    else if (editChannel === 1) g = Math.max(0, g - 5);
    else b = Math.max(0, b - 5);
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    if (editChannel === 0) r = Math.min(255, r + 5);
    else if (editChannel === 1) g = Math.min(255, g + 5);
    else b = Math.min(255, b + 5);
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    shouldExit = true;
  }
  
  delay(50);
}
