var display = require("display");
var keyboard = require("keyboard");

// Flashlight App
var screenWidth = display.width();
var screenHeight = display.height();

var WHITE = display.color(255, 255, 255);
var BLACK = display.color(0, 0, 0);
var YELLOW = display.color(255, 255, 200);
var RED = display.color(255, 0, 0);

var isOn = false;
var mode = 0; // 0 = white, 1 = warm, 2 = strobe
var shouldExit = false;
var strobeState = false;

function getColor() {
  if (mode === 0) return WHITE;
  if (mode === 1) return YELLOW;
  return strobeState ? WHITE : BLACK;
}

function drawScreen() {
  var col = isOn ? getColor() : BLACK;
  display.drawFillRect(0, 0, screenWidth, screenHeight, col);
  
  if (!isOn || mode === 2) {
    display.setTextColor(isOn ? BLACK : WHITE);
    display.setTextSize(2);
    display.drawString(isOn ? "ON" : "OFF", screenWidth/2 - 15, 20);
    
    display.setTextSize(1);
    var modeStr = mode === 0 ? "White" : (mode === 1 ? "Warm" : "Strobe");
    display.drawString("Mode: " + modeStr, screenWidth/2 - 35, 50);
    
    display.setTextColor(isOn ? BLACK : display.color(100, 100, 100));
    display.drawString("SEL: Toggle", 10, screenHeight - 35);
    display.drawString("PREV/NEXT: Mode", 10, screenHeight - 22);
    display.drawString("ESC: Exit", screenWidth - 55, screenHeight - 22);
  }
}

drawScreen();

while (!shouldExit) {
  if (keyboard.getSelPress()) {
    isOn = !isOn;
    drawScreen();
  }
  if (keyboard.getPrevPress()) {
    mode = (mode - 1 + 3) % 3;
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    mode = (mode + 1) % 3;
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    shouldExit = true;
  }
  
  if (isOn && mode === 2) {
    strobeState = !strobeState;
    display.drawFillRect(0, 0, screenWidth, screenHeight, strobeState ? WHITE : BLACK);
    delay(100);
  } else {
    delay(50);
  }
}

display.drawFillRect(0, 0, screenWidth, screenHeight, BLACK);
