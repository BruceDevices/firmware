var display = require("display");
var keyboard = require("keyboard");

// Reaction Time Test - Whole Seconds Only
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(10, 15, 25);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(0, 255, 120);
var RED = display.color(255, 60, 60);
var YELLOW = display.color(255, 220, 0);
var GRAY = display.color(80, 80, 80);
var CYAN = display.color(0, 220, 255);

var state = 0; // 0=ready, 1=wait, 2=go, 3=result, 4=early
var goTime = 0;
var reactionTime = 0;
var bestTime = 99999;
var shouldExit = false;

function drawScreen() {
  if (state === 0) {
    // READY SCREEN - Cool design
    display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

    // Border
    display.drawRect(3, 3, screenWidth - 6, screenHeight - 6, CYAN);

    // Title
    display.setTextColor(CYAN);
    display.setTextSize(2);
    display.drawString("REACTION", screenWidth / 2 - 50, 12);

    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.drawString("TEST YOUR SPEED", screenWidth / 2 - 48, 35);

    // Instructions in box
    display.drawFillRect(
      10,
      50,
      screenWidth - 20,
      45,
      display.color(20, 30, 40)
    );
    display.drawRect(10, 50, screenWidth - 20, 45, GRAY);

    display.setTextColor(GRAY);
    display.drawString("1. Press SEL to begin", 18, 58);
    display.drawString("2. Wait for GREEN", 18, 70);
    display.drawString("3. Press SEL as fast as you can!", 18, 82);

    // Best time
    if (bestTime < 99999) {
      display.setTextColor(YELLOW);
      display.setTextSize(1);
      var bestSec = Math.floor(bestTime / 1000);
      display.drawString("BEST: " + bestSec + " sec", 10, screenHeight - 18);
    }

    display.setTextColor(GREEN);
    display.drawString("[SEL] START", screenWidth - 70, screenHeight - 18);
  } else if (state === 1) {
    // WAIT SCREEN - Red
    display.drawFillRect(0, 0, screenWidth, screenHeight, RED);
    display.setTextColor(WHITE);
    display.setTextSize(3);
    display.drawString("WAIT", screenWidth / 2 - 38, screenHeight / 2 - 12);
  } else if (state === 2) {
    // GO SCREEN - Green
    display.drawFillRect(0, 0, screenWidth, screenHeight, GREEN);
    display.setTextColor(WHITE);
    display.setTextSize(3);
    display.drawString("GO!", screenWidth / 2 - 28, screenHeight / 2 - 12);
  } else if (state === 3) {
    // RESULT SCREEN - Clean design
    display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

    // Get whole seconds
    var seconds = Math.floor(reactionTime / 1000);

    // Rating based on ms still but show seconds
    var rating, ratingColor;
    if (reactionTime < 200) {
      rating = "SUPERHUMAN!";
      ratingColor = CYAN;
    } else if (reactionTime < 250) {
      rating = "AMAZING!";
      ratingColor = GREEN;
    } else if (reactionTime < 300) {
      rating = "GREAT!";
      ratingColor = GREEN;
    } else if (reactionTime < 400) {
      rating = "GOOD";
      ratingColor = YELLOW;
    } else if (reactionTime < 600) {
      rating = "OK";
      ratingColor = YELLOW;
    } else {
      rating = "SLOW";
      ratingColor = RED;
    }

    // Top label
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.drawString("YOUR TIME", screenWidth / 2 - 30, 15);

    // Big time display - centered
    display.setTextColor(ratingColor);
    display.setTextSize(4);
    var secStr = "" + seconds;
    var secWidth = secStr.length * 24;
    display.drawString(secStr, screenWidth / 2 - secWidth / 2 - 10, 35);

    // "sec" label next to number
    display.setTextSize(2);
    display.drawString("s", screenWidth / 2 + secWidth / 2, 45);

    // Rating in box
    display.drawFillRect(
      15,
      78,
      screenWidth - 30,
      25,
      display.color(30, 40, 50)
    );
    display.setTextColor(ratingColor);
    display.setTextSize(2);
    var ratingWidth = rating.length * 12;
    display.drawString(rating, screenWidth / 2 - ratingWidth / 2, 82);

    // Controls at bottom
    display.setTextColor(GRAY);
    display.setTextSize(1);
    display.drawString("[SEL] RETRY", 15, screenHeight - 15);
    display.drawString("[ESC] EXIT", screenWidth - 65, screenHeight - 15);
  } else if (state === 4) {
    // TOO EARLY SCREEN
    display.drawFillRect(0, 0, screenWidth, screenHeight, YELLOW);
    display.setTextColor(display.color(40, 40, 40));
    display.setTextSize(2);
    display.drawString(
      "TOO EARLY!",
      screenWidth / 2 - 60,
      screenHeight / 2 - 20
    );
    display.setTextSize(1);
    display.drawString(
      "Wait for the green screen!",
      screenWidth / 2 - 75,
      screenHeight / 2 + 10
    );
  }
}

drawScreen();

while (!shouldExit) {
  if (state === 0) {
    if (keyboard.getSelPress()) {
      state = 1;
      drawScreen();

      // Random delay 1.5-4 seconds
      var waitTime = 1500 + Math.floor(Math.random() * 2500);
      var waitStart = Date.now();
      var tooEarly = false;

      while (Date.now() - waitStart < waitTime) {
        if (keyboard.getSelPress()) {
          tooEarly = true;
          break;
        }
        delay(5);
      }

      if (tooEarly) {
        state = 4;
        drawScreen();
        delay(1500);
        state = 0;
        drawScreen();
      } else {
        state = 2;
        goTime = Date.now();
        drawScreen();
      }
    }
    if (keyboard.getEscPress()) {
      shouldExit = true;
    }
  } else if (state === 2) {
    if (keyboard.getSelPress()) {
      reactionTime = Date.now() - goTime;
      if (reactionTime < bestTime) bestTime = reactionTime;
      state = 3;
      drawScreen();
    }
    delay(1);
  } else if (state === 3) {
    if (keyboard.getSelPress()) {
      state = 0;
      drawScreen();
    }
    if (keyboard.getEscPress()) {
      shouldExit = true;
    }
    delay(50);
  } else {
    delay(50);
  }
}
