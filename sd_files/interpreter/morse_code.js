var display = require("display");
var keyboard = require("keyboard");

// Morse Code Translator - BIG output
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(5, 15, 25);
var WHITE = display.color(255, 255, 255);
var YELLOW = display.color(255, 235, 59);
var CYAN = display.color(0, 200, 255);
var GRAY = display.color(100, 100, 100);

var morseCode = {
  A: ".-",
  B: "-...",
  C: "-.-.",
  D: "-..",
  E: ".",
  F: "..-.",
  G: "--.",
  H: "....",
  I: "..",
  J: ".---",
  K: "-.-",
  L: ".-..",
  M: "--",
  N: "-.",
  O: "---",
  P: ".--.",
  Q: "--.-",
  R: ".-.",
  S: "...",
  T: "-",
  U: "..-",
  V: "...-",
  W: ".--",
  X: "-..-",
  Y: "-.--",
  Z: "--..",
  0: "-----",
  1: ".----",
  2: "..---",
  3: "...--",
  4: "....-",
  5: ".....",
  6: "-....",
  7: "--...",
  8: "---..",
  9: "----.",
  " ": "/",
};

var chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
var charIndex = 0;
var text = "";
var shouldExit = false;

function getMorse(str) {
  var result = "";
  for (var i = 0; i < str.length; i++) {
    var ch = str.charAt(i).toUpperCase();
    if (morseCode[ch]) {
      result += morseCode[ch] + " ";
    }
  }
  return result;
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  // Title
  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.drawString("MORSE CODE", screenWidth / 2 - 32, 3);

  // Text input box
  display.drawRect(5, 15, screenWidth - 10, 20, GRAY);
  display.setTextColor(WHITE);
  var displayText =
    text.length > 18 ? "..." + text.substring(text.length - 15) : text;
  display.drawString(displayText + "_", 10, 22);

  // Character selector - bigger
  display.setTextColor(YELLOW);
  display.setTextSize(2);
  display.drawString(
    "[" + chars.charAt(charIndex) + "]",
    screenWidth / 2 - 18,
    40
  );

  // Show prev/next chars
  display.setTextSize(1);
  display.setTextColor(GRAY);
  var prevChar = chars.charAt((charIndex - 1 + chars.length) % chars.length);
  var nextChar = chars.charAt((charIndex + 1) % chars.length);
  display.drawString(prevChar, screenWidth / 2 - 45, 47);
  display.drawString(nextChar, screenWidth / 2 + 35, 47);

  // MORSE OUTPUT - BIG BOX
  display.drawFillRect(5, 62, screenWidth - 10, 50, display.color(20, 30, 40));
  display.drawRect(5, 62, screenWidth - 10, 50, CYAN);

  var morse = getMorse(text);
  display.setTextColor(YELLOW);
  display.setTextSize(2); // BIG morse output

  // Wrap morse code if needed
  var maxChars = Math.floor((screenWidth - 20) / 12);
  if (morse.length <= maxChars) {
    display.drawString(morse, 10, 75);
  } else {
    // Two lines
    display.setTextSize(1);
    display.drawString(morse.substring(0, maxChars * 2), 10, 68);
    if (morse.length > maxChars * 2) {
      display.drawString(morse.substring(maxChars * 2, maxChars * 4), 10, 82);
    }
    if (morse.length > maxChars * 4) {
      display.drawString(morse.substring(maxChars * 4, maxChars * 6), 10, 96);
    }
  }

  // Controls
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.drawString("</>Char [OK]Add [ESC]Del", 5, screenHeight - 10);
}

drawScreen();

while (!shouldExit) {
  if (keyboard.getPrevPress()) {
    charIndex = (charIndex - 1 + chars.length) % chars.length;
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    charIndex = (charIndex + 1) % chars.length;
    drawScreen();
  }
  if (keyboard.getSelPress()) {
    text = text + chars.charAt(charIndex);
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    if (text.length > 0) {
      text = text.substring(0, text.length - 1);
      drawScreen();
    } else {
      shouldExit = true;
    }
  }
  delay(80);
}
