var display = require("display");
var keyboard = require("keyboard");

// Tip Calculator - Multiple currencies, dynamic layout
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(25, 30, 35);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(76, 175, 80);
var CYAN = display.color(0, 188, 212);
var GRAY = display.color(100, 100, 100);
var YELLOW = display.color(255, 193, 7);

var currencies = ["USD", "EUR", "GBP", "CZK", "JPY", "CHF"];
var currencyIndex = 0;

var billAmount = 50;
var tipPercent = 15;
var splitCount = 1;
var editField = 0; // 0=bill, 1=tip, 2=split, 3=currency
var shouldExit = false;

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  var curr = currencies[currencyIndex];

  // Title with currency
  display.setTextColor(GREEN);
  display.setTextSize(1);
  display.drawString("TIP CALC", 5, 3);
  display.setTextColor(editField === 3 ? CYAN : YELLOW);
  display.drawString("[" + curr + "]", screenWidth - 35, 3);

  // Bill amount - BIGGER
  display.setTextColor(editField === 0 ? CYAN : WHITE);
  display.setTextSize(2);
  display.drawString(billAmount + " " + curr, 5, 18);

  // Tip and Split on same line
  display.setTextSize(1);
  display.setTextColor(editField === 1 ? CYAN : WHITE);
  display.drawString("Tip:" + tipPercent + "%", 5, 40);

  display.setTextColor(editField === 2 ? CYAN : WHITE);
  display.drawString("Split:" + splitCount, screenWidth / 2, 40);

  // Calculations
  var tipAmount = (billAmount * tipPercent) / 100;
  var total = billAmount + tipAmount;
  var perPerson = total / splitCount;

  // Divider
  display.drawLine(5, 55, screenWidth - 5, 55, GRAY);

  // Results
  display.setTextColor(GRAY);
  display.setTextSize(1);
  display.drawString("Tip: " + tipAmount.toFixed(2) + " " + curr, 5, 62);
  display.drawString("Total: " + total.toFixed(2) + " " + curr, 5, 77);

  // Per person - BIG and dynamic width
  display.setTextColor(GREEN);
  display.setTextSize(2);
  var perPersonStr = perPerson.toFixed(2) + " " + curr;
  var perPersonWidth = perPersonStr.length * 12;

  // Calculate positions to fit
  var startX = 5;
  var labelX = startX + perPersonWidth + 5;

  // If too wide, show on two lines
  if (labelX + 50 > screenWidth) {
    display.drawString(perPersonStr, 5, 95);
    display.setTextSize(1);
    display.drawString("/person", 5, 115);
  } else {
    display.drawString(perPersonStr, startX, 100);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.drawString("/person", labelX, 108);
  }

  // Controls
  display.setTextColor(GRAY);
  display.setTextSize(1);
  display.drawString("SEL:Next PREV/NEXT:Adj", 5, screenHeight - 10);
}

drawScreen();

while (!shouldExit) {
  if (keyboard.getSelPress()) {
    editField = (editField + 1) % 4;
    drawScreen();
  }
  if (keyboard.getPrevPress()) {
    if (editField === 0) billAmount = Math.max(1, billAmount - 5);
    else if (editField === 1) tipPercent = Math.max(0, tipPercent - 5);
    else if (editField === 2) splitCount = Math.max(1, splitCount - 1);
    else
      currencyIndex =
        (currencyIndex - 1 + currencies.length) % currencies.length;
    drawScreen();
  }
  if (keyboard.getNextPress()) {
    if (editField === 0) billAmount = Math.min(99999, billAmount + 5);
    else if (editField === 1) tipPercent = Math.min(100, tipPercent + 5);
    else if (editField === 2) splitCount = Math.min(50, splitCount + 1);
    else currencyIndex = (currencyIndex + 1) % currencies.length;
    drawScreen();
  }
  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  delay(80);
}
