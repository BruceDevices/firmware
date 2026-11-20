var display = require("display");
var keyboard = require("keyboard");
var http = require("http");
var wifi = require("wifi");
var dialog = require("dialog");

// Currency Converter - Real-time rates
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(15, 25, 35);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(76, 200, 100);
var CYAN = display.color(0, 200, 255);
var YELLOW = display.color(255, 200, 0);
var GRAY = display.color(100, 100, 100);
var RED = display.color(255, 80, 80);

// Currency list
var currencies = [
  "USD",
  "EUR",
  "GBP",
  "CZK",
  "JPY",
  "CHF",
  "CAD",
  "AUD",
  "CNY",
  "PLN",
];
var currencySymbols = {
  USD: "$",
  EUR: "€",
  GBP: "£",
  CZK: "Kč",
  JPY: "¥",
  CHF: "Fr",
  CAD: "C$",
  AUD: "A$",
  CNY: "¥",
  PLN: "zł",
};

// Fallback rates (EUR base) - used if no internet
var fallbackRates = {
  USD: 1.08,
  EUR: 1.0,
  GBP: 0.86,
  CZK: 25.3,
  JPY: 162.5,
  CHF: 0.94,
  CAD: 1.47,
  AUD: 1.65,
  CNY: 7.85,
  PLN: 4.32,
};

var rates = {};
var lastUpdate = "Offline";
var fromIndex = 1; // EUR
var toIndex = 0; // USD
var amount = 100;
var editField = 0; // 0=amount, 1=from, 2=to
var shouldExit = false;

function fetchRates() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.drawString(
    "Fetching rates...",
    screenWidth / 2 - 50,
    screenHeight / 2 - 10
  );

  // Try to fetch from free API
  try {
    // Using exchangerate-api free endpoint
    var response = http.get("https://api.exchangerate-api.com/v4/latest/EUR");
    if (response && response.rates) {
      rates = response.rates;
      rates.EUR = 1.0;
      var d = new Date();
      lastUpdate =
        d.getHours() + ":" + (d.getMinutes() < 10 ? "0" : "") + d.getMinutes();
      return true;
    }
  } catch (e) {
    // API failed, use fallback
  }

  // Use fallback rates
  rates = fallbackRates;
  lastUpdate = "Offline";
  return false;
}

function convert(amt, from, to) {
  var fromCurr = currencies[from];
  var toCurr = currencies[to];

  // Convert through EUR
  var eurAmount = amt / (rates[fromCurr] || 1);
  return eurAmount * (rates[toCurr] || 1);
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  // Title and status
  display.setTextColor(GREEN);
  display.setTextSize(1);
  display.drawString("CURRENCY", 5, 3);
  display.setTextColor(lastUpdate === "Offline" ? RED : GRAY);
  display.drawString(lastUpdate, screenWidth - 50, 3);

  // FROM section
  display.setTextColor(editField === 0 ? CYAN : WHITE);
  display.setTextSize(2);
  display.drawString("" + amount, 10, 20);

  display.setTextColor(editField === 1 ? CYAN : YELLOW);
  display.setTextSize(1);
  display.drawString(currencies[fromIndex], screenWidth - 40, 27);

  // Arrow
  display.setTextColor(GREEN);
  display.setTextSize(2);
  display.drawString("=", screenWidth / 2 - 8, 45);

  // TO section - result
  var result = convert(amount, fromIndex, toIndex);
  var resultStr = result.toFixed(2);

  display.setTextColor(GREEN);
  display.setTextSize(2);
  display.drawString(resultStr, 10, 65);

  display.setTextColor(editField === 2 ? CYAN : YELLOW);
  display.setTextSize(1);
  display.drawString(currencies[toIndex], screenWidth - 40, 72);

  // Exchange rate info
  display.setTextColor(GRAY);
  var rate1 = convert(1, fromIndex, toIndex).toFixed(4);
  display.drawString(
    "1 " + currencies[fromIndex] + " = " + rate1 + " " + currencies[toIndex],
    10,
    92
  );

  // Quick rates display
  display.drawLine(5, 105, screenWidth - 5, 105, GRAY);
  display.setTextColor(WHITE);
  display.drawString("EUR->USD:" + (rates.USD || "?"), 5, 110);
  display.drawString(
    "EUR->CZK:" + (Math.round(rates.CZK) || "?"),
    screenWidth / 2,
    110
  );

  // Controls
  display.setTextColor(GRAY);
  display.drawString("SEL:Fld </>:Adj ESC:Exit", 5, screenHeight - 10);
}

// Check WiFi and fetch rates
if (!wifi.isConnected || !wifi.isConnected()) {
  dialog.info("Connect to WiFi for live rates", true);
}
fetchRates();
drawScreen();

while (!shouldExit) {
  if (keyboard.getSelPress()) {
    editField = (editField + 1) % 3;
    drawScreen();
  }

  if (keyboard.getPrevPress()) {
    if (editField === 0) {
      if (amount > 1000) amount -= 100;
      else if (amount > 100) amount -= 50;
      else if (amount > 10) amount -= 10;
      else amount = Math.max(1, amount - 1);
    } else if (editField === 1)
      fromIndex = (fromIndex - 1 + currencies.length) % currencies.length;
    else toIndex = (toIndex - 1 + currencies.length) % currencies.length;
    drawScreen();
  }

  if (keyboard.getNextPress()) {
    if (editField === 0) {
      if (amount >= 1000) amount += 100;
      else if (amount >= 100) amount += 50;
      else if (amount >= 10) amount += 10;
      else amount += 1;
    } else if (editField === 1) fromIndex = (fromIndex + 1) % currencies.length;
    else toIndex = (toIndex + 1) % currencies.length;
    drawScreen();
  }

  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  delay(80);
}
