var display = require("display");
var keyboard = require("keyboard");

// Stopwatch - BIG display, seconds only, smooth updates
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(10, 10, 20);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(76, 175, 80);
var RED = display.color(244, 67, 54);
var GRAY = display.color(80, 80, 80);

var running = false;
var startTime = 0;
var elapsed = 0;
var shouldExit = false;

function formatTime(ms) {
  var totalSec = Math.floor(ms / 1000);
  var hours = Math.floor(totalSec / 3600);
  var minutes = Math.floor((totalSec % 3600) / 60);
  var seconds = totalSec % 60;

  var h = hours < 10 ? "0" + hours : "" + hours;
  var m = minutes < 10 ? "0" + minutes : "" + minutes;
  var s = seconds < 10 ? "0" + seconds : "" + seconds;

  if (hours > 0) return h + ":" + m + ":" + s;
  return m + ":" + s;
}

function drawFullScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  // Status indicator
  display.setTextColor(running ? GREEN : GRAY);
  display.setTextSize(1);
  display.drawString(running ? "RUNNING" : "STOPPED", screenWidth / 2 - 25, 5);

  // HUGE time display - centered
  display.setTextColor(running ? GREEN : WHITE);
  display.setTextSize(3);
  var timeStr = formatTime(elapsed);
  var timeWidth = timeStr.length * 18;
  display.drawString(
    timeStr,
    (screenWidth - timeWidth) / 2,
    screenHeight / 2 - 20
  );

  // Controls at bottom
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.drawString(
    running ? "[SEL] Stop" : "[SEL] Start",
    10,
    screenHeight - 25
  );
  display.drawString("[PREV] Reset", 10, screenHeight - 12);
  display.drawString("[NEXT] Exit", screenWidth - 70, screenHeight - 12);
}

// Only update the time digits, not the whole screen
function updateTimeDisplay() {
  // Clear only time area
  display.drawFillRect(0, screenHeight / 2 - 25, screenWidth, 35, BG);

  display.setTextColor(GREEN);
  display.setTextSize(3);
  var timeStr = formatTime(elapsed);
  var timeWidth = timeStr.length * 18;
  display.drawString(
    timeStr,
    (screenWidth - timeWidth) / 2,
    screenHeight / 2 - 20
  );
}

drawFullScreen();

var lastSecond = -1;

while (!shouldExit) {
  if (keyboard.getPrevPress()) {
    if (!running) {
      elapsed = 0;
      lastSecond = -1;
      drawFullScreen();
    }
  }

  if (keyboard.getSelPress()) {
    if (running) {
      running = false;
      drawFullScreen();
    } else {
      startTime = Date.now() - elapsed;
      running = true;
      drawFullScreen();
    }
  }

  if (keyboard.getNextPress()) {
    shouldExit = true;
  }

  if (running) {
    elapsed = Date.now() - startTime;
    var currentSecond = Math.floor(elapsed / 1000);

    // Only update display when second changes (smooth, no flicker)
    if (currentSecond !== lastSecond) {
      updateTimeDisplay();
      lastSecond = currentSecond;
    }
  }

  delay(50);
}
