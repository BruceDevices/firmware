var display = require("display");
var keyboard = require("keyboard");

// Dice Roller App
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(30, 20, 10);
var WHITE = display.color(255, 255, 255);
var RED = display.color(220, 50, 50);
var CREAM = display.color(255, 250, 240);
var GRAY = display.color(100, 100, 100);

var diceCount = 2;
var diceSides = 6;
var results = [];
var total = 0;
var shouldExit = false;

function rollDice() {
  results = [];
  total = 0;
  for (var i = 0; i < diceCount; i++) {
    var roll = Math.floor(Math.random() * diceSides) + 1;
    results.push(roll);
    total += roll;
  }
}

function drawDie(x, y, size, value) {
  display.drawFillRect(x, y, size, size, CREAM);
  display.drawRect(x, y, size, size, GRAY);

  display.setTextColor(RED);
  display.setTextSize(2);
  var textX = x + size / 2 - (value >= 10 ? 12 : 6);
  var textY = y + size / 2 - 8;
  display.drawString("" + value, textX, textY);
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawString(
    "DICE: " + diceCount + "d" + diceSides,
    screenWidth / 2 - 30,
    5
  );

  // Draw dice
  var dieSize = 35;
  var spacing = 5;
  var totalWidth = diceCount * dieSize + (diceCount - 1) * spacing;
  var startX = (screenWidth - totalWidth) / 2;

  if (results.length > 0) {
    for (var i = 0; i < Math.min(results.length, 4); i++) {
      var x = startX + i * (dieSize + spacing);
      drawDie(x, 25, dieSize, results[i]);
    }

    // Total
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.drawString("Total: " + total, screenWidth / 2 - 45, 70);
  } else {
    display.setTextColor(GRAY);
    display.setTextSize(1);
    display.drawString("Press SEL to roll", screenWidth / 2 - 50, 50);
  }

  // Controls
  display.setTextSize(1);
  display.setTextColor(GRAY);
  display.drawString("SEL: Roll | ESC: Exit", 10, screenHeight - 25);
  display.drawString("PREV: Dice-  NEXT: Dice+", 10, screenHeight - 12);
}

drawScreen();

while (!shouldExit) {
  if (keyboard.getSelPress()) {
    rollDice();
    drawScreen();
  }
  if (keyboard.getPrevPress()) {
    diceCount = Math.max(1, diceCount - 1);
    results = [];
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    diceCount = Math.min(4, diceCount + 1);
    results = [];
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  delay(80);
}
