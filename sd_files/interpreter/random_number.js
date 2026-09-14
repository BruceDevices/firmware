var display = require("display");
var keyboard = require("keyboard");

// Random Number Generator
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(25, 15, 35);
var WHITE = display.color(255, 255, 255);
var CYAN = display.color(0, 255, 255);
var PURPLE = display.color(156, 39, 176);
var GRAY = display.color(100, 100, 100);

var minVal = 1;
var maxVal = 100;
var result = 0;
var hasResult = false;
var shouldExit = false;
var editMode = 0; // 0 = ready, 1 = edit min, 2 = edit max

function random(min, max) {
  return Math.floor(Math.random() * (max - min + 1)) + min;
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  display.setTextColor(PURPLE);
  display.setTextSize(1);
  display.drawString("RANDOM NUMBER", screenWidth / 2 - 45, 5);

  // Range display
  display.setTextColor(editMode === 1 ? CYAN : WHITE);
  display.drawString("Min: " + minVal, 20, 30);

  display.setTextColor(editMode === 2 ? CYAN : WHITE);
  display.drawString("Max: " + maxVal, screenWidth - 70, 30);

  // Result
  if (hasResult) {
    display.setTextColor(CYAN);
    display.setTextSize(3);
    display.drawString("" + result, screenWidth / 2 - 25, 55);
  } else {
    display.setTextColor(GRAY);
    display.setTextSize(2);
    display.drawString("Press SEL", screenWidth / 2 - 45, 60);
  }

  // Controls
  display.setTextSize(1);
  display.setTextColor(GRAY);
  if (editMode === 0) {
    display.drawString("SEL: Generate", 10, screenHeight - 35);
    display.drawString("PREV: Edit Min", 10, screenHeight - 22);
    display.drawString("NEXT: Edit Max", screenWidth - 80, screenHeight - 22);
  } else {
    display.drawString("PREV/NEXT: +/- value", 10, screenHeight - 35);
    display.drawString("SEL: Confirm", 10, screenHeight - 22);
  }
}

drawScreen();

while (!shouldExit) {
  if (editMode === 0) {
    if (keyboard.getSelPress()) {
      result = random(minVal, maxVal);
      hasResult = true;
      drawScreen();
    }
    if (keyboard.getPrevPress()) {
      editMode = 1;
      drawScreen();
    }
    if (keyboard.getNextPress()) {
      editMode = 2;
      drawScreen();
    }
    if (keyboard.getEscPress()) {
      shouldExit = true;
    }
  } else {
    if (keyboard.getPrevPress()) {
      if (editMode === 1) {
        minVal = Math.max(0, minVal - 1);
      } else {
        maxVal = Math.max(minVal + 1, maxVal - 1);
      }
      drawScreen();
    }
    if (keyboard.getNextPress()) {
      if (editMode === 1) {
        minVal = Math.min(maxVal - 1, minVal + 1);
      } else {
        maxVal = Math.min(9999, maxVal + 1);
      }
      drawScreen();
    }
    if (keyboard.getSelPress()) {
      editMode = 0;
      hasResult = false;
      drawScreen();
    }
  }

  delay(80);
}
