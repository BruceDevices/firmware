var display = require("display");
var keyboard = require("keyboard");

// Counter App
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(15, 25, 35);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(76, 175, 80);
var RED = display.color(244, 67, 54);
var CYAN = display.color(0, 188, 212);
var GRAY = display.color(100, 100, 100);

var count = 0;
var step = 1;
var shouldExit = false;

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.drawString("COUNTER", screenWidth / 2 - 25, 5);

  // Main count display
  display.setTextColor(count >= 0 ? GREEN : RED);
  display.setTextSize(3);
  var countStr = "" + count;
  var textW = countStr.length * 18;
  display.drawString(countStr, screenWidth / 2 - textW / 2, 35);

  // Step indicator
  display.setTextColor(GRAY);
  display.setTextSize(1);
  display.drawString("Step: " + step, screenWidth / 2 - 25, 75);

  // Buttons visualization
  display.setTextColor(RED);
  display.drawString("[-]", 20, screenHeight - 35);
  display.setTextColor(GREEN);
  display.drawString("[+]", screenWidth - 40, screenHeight - 35);

  // Controls
  display.setTextColor(WHITE);
  display.drawString("PREV", 18, screenHeight - 22);
  display.drawString("NEXT", screenWidth - 42, screenHeight - 22);

  display.setTextColor(GRAY);
  display.drawString(
    "SEL:Step ESC:Exit",
    screenWidth / 2 - 50,
    screenHeight - 12
  );
}

drawScreen();

while (!shouldExit) {
  if (keyboard.getPrevPress()) {
    count -= step;
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    count += step;
    drawScreen();
  }
  if (keyboard.getSelPress()) {
    step = step === 1 ? 5 : step === 5 ? 10 : step === 10 ? 100 : 1;
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  delay(50);
}
