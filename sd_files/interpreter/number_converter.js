var display = require("display");
var keyboard = require("keyboard");

// Binary/Hex/Decimal Converter
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(10, 10, 20);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(0, 255, 128);
var ORANGE = display.color(255, 165, 0);
var BLUE = display.color(100, 150, 255);
var GRAY = display.color(100, 100, 100);

var decValue = 255;
var shouldExit = false;

function toBinary(n) {
  if (n === 0) return "0";
  var result = "";
  while (n > 0) {
    result = (n % 2) + result;
    n = Math.floor(n / 2);
  }
  return result;
}

function toHex(n) {
  if (n === 0) return "0";
  var hexChars = "0123456789ABCDEF";
  var result = "";
  while (n > 0) {
    result = hexChars.charAt(n % 16) + result;
    n = Math.floor(n / 16);
  }
  return result;
}

function toOctal(n) {
  if (n === 0) return "0";
  var result = "";
  while (n > 0) {
    result = (n % 8) + result;
    n = Math.floor(n / 8);
  }
  return result;
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawString("NUMBER CONVERTER", screenWidth / 2 - 50, 5);

  // Decimal
  display.setTextColor(ORANGE);
  display.drawString("DEC:", 10, 25);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.drawString("" + decValue, 50, 22);

  // Hexadecimal
  display.setTextSize(1);
  display.setTextColor(GREEN);
  display.drawString("HEX:", 10, 48);
  display.setTextColor(WHITE);
  display.drawString("0x" + toHex(decValue), 50, 48);

  // Binary
  display.setTextColor(BLUE);
  display.drawString("BIN:", 10, 65);
  display.setTextColor(WHITE);
  var binStr = toBinary(decValue);
  if (binStr.length > 16) binStr = "..." + binStr.substring(binStr.length - 13);
  display.drawString(binStr, 50, 65);

  // Octal
  display.setTextColor(GRAY);
  display.drawString("OCT:", 10, 82);
  display.setTextColor(WHITE);
  display.drawString(toOctal(decValue), 50, 82);

  // Controls
  display.setTextColor(GRAY);
  display.drawString("PREV/NEXT: +/- value", 10, screenHeight - 22);
  display.drawString("SEL: x10  ESC: Exit", 10, screenHeight - 10);
}

drawScreen();
var multiplier = 1;

while (!shouldExit) {
  if (keyboard.getPrevPress()) {
    decValue = Math.max(0, decValue - multiplier);
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    decValue = Math.min(65535, decValue + multiplier);
    drawScreen();
  }
  if (keyboard.getSelPress()) {
    multiplier = multiplier === 1 ? 10 : multiplier === 10 ? 100 : 1;
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  delay(80);
}
