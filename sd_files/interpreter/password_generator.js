var display = require("display");
var keyboard = require("keyboard");

// Password Generator - BIGGER UI
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(15, 20, 30);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(76, 175, 80);
var YELLOW = display.color(255, 193, 7);
var GRAY = display.color(100, 100, 100);
var CYAN = display.color(0, 200, 255);

var length = 12;
var password = "";
var shouldExit = false;

var upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
var lower = "abcdefghijklmnopqrstuvwxyz";
var numbers = "0123456789";
var symbols = "!@#$%&*+-=?";

function generatePassword() {
  var charset = upper + lower + numbers + symbols;
  password = "";
  for (var i = 0; i < length; i++) {
    var idx = Math.floor(Math.random() * charset.length);
    password += charset.charAt(idx);
  }
}

function getStrength() {
  if (length >= 16) return "STRONG";
  if (length >= 12) return "MEDIUM";
  return "WEAK";
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  // Title
  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.drawString("PASSWORD GENERATOR", 5, 5);

  // Big password display box
  display.drawFillRect(5, 22, screenWidth - 10, 45, display.color(30, 35, 45));
  display.drawRect(5, 22, screenWidth - 10, 45, CYAN);

  // Password in BIG text
  display.setTextColor(WHITE);
  display.setTextSize(2);

  var displayPwd = password;
  var maxChars = Math.floor((screenWidth - 20) / 12);
  if (displayPwd.length > maxChars) {
    // Show in two lines if too long
    display.drawString(displayPwd.substring(0, maxChars), 10, 28);
    display.drawString(displayPwd.substring(maxChars), 10, 48);
  } else {
    display.drawString(displayPwd, 10, 35);
  }

  // Length indicator - bigger
  display.setTextColor(YELLOW);
  display.setTextSize(2);
  display.drawString("Len: " + length, 10, 75);

  // Strength indicator - bigger
  var strength = getStrength();
  var strengthColor =
    strength === "STRONG"
      ? GREEN
      : strength === "MEDIUM"
      ? YELLOW
      : display.color(255, 100, 100);
  display.setTextColor(strengthColor);
  display.drawString(strength, screenWidth - 80, 75);

  // Controls - bigger
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.drawString("SEL: Generate New", 10, screenHeight - 25);
  display.drawString("PREV/NEXT: Length", 10, screenHeight - 12);
}

generatePassword();
drawScreen();

while (!shouldExit) {
  if (keyboard.getSelPress()) {
    generatePassword();
    drawScreen();
  }
  if (keyboard.getPrevPress()) {
    length = Math.max(6, length - 2);
    generatePassword();
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    length = Math.min(24, length + 2);
    generatePassword();
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    shouldExit = true;
  }
  delay(80);
}
