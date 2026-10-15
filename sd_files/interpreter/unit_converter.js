var display = require("display");
var keyboard = require("keyboard");

// Unit Converter - Clean design, more units
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(15, 20, 30);
var WHITE = display.color(255, 255, 255);
var CYAN = display.color(0, 220, 255);
var ORANGE = display.color(255, 165, 0);
var GRAY = display.color(100, 100, 100);
var GREEN = display.color(100, 255, 100);

var categories = ["Length", "Weight", "Temp", "Volume", "Speed", "Data"];
var catIndex = 0;

var allUnits = {
  Length: ["m", "km", "cm", "mm", "ft", "in", "mi", "yd"],
  Weight: ["kg", "g", "mg", "lb", "oz", "ton"],
  Temp: ["°C", "°F", "K"],
  Volume: ["L", "mL", "gal", "qt", "pt", "cup", "fl oz"],
  Speed: ["m/s", "km/h", "mph", "kn"],
  Data: ["B", "KB", "MB", "GB", "TB"],
};

// Conversion factors to base unit
var convFactors = {
  Length: {
    m: 1,
    km: 1000,
    cm: 0.01,
    mm: 0.001,
    ft: 0.3048,
    in: 0.0254,
    mi: 1609.34,
    yd: 0.9144,
  },
  Weight: {
    kg: 1,
    g: 0.001,
    mg: 0.000001,
    lb: 0.453592,
    oz: 0.0283495,
    ton: 1000,
  },
  Volume: {
    L: 1,
    mL: 0.001,
    gal: 3.78541,
    qt: 0.946353,
    pt: 0.473176,
    cup: 0.24,
    "fl oz": 0.0295735,
  },
  Speed: { "m/s": 1, "km/h": 0.277778, mph: 0.44704, kn: 0.514444 },
  Data: { B: 1, KB: 1024, MB: 1048576, GB: 1073741824, TB: 1099511627776 },
};

var fromIndex = 0;
var toIndex = 1;
var value = 1;
var editField = 0; // 0=value, 1=from, 2=to, 3=category
var shouldExit = false;

function getUnits() {
  return allUnits[categories[catIndex]];
}

function convert(val, from, to) {
  var cat = categories[catIndex];
  var units = getUnits();
  var fromU = units[from];
  var toU = units[to];

  // Temperature special case
  if (cat === "Temp") {
    var celsius = val;
    if (fromU === "°F") celsius = ((val - 32) * 5) / 9;
    else if (fromU === "K") celsius = val - 273.15;

    if (toU === "°F") return (celsius * 9) / 5 + 32;
    if (toU === "K") return celsius + 273.15;
    return celsius;
  }

  // Standard conversion via base unit
  var factors = convFactors[cat];
  var base = val * factors[fromU];
  return base / factors[toU];
}

function drawScreen() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  var units = getUnits();

  // Category selector at top
  display.setTextColor(editField === 3 ? CYAN : ORANGE);
  display.setTextSize(1);
  display.drawString(
    "< " + categories[catIndex] + " >",
    screenWidth / 2 - 30,
    5
  );

  // FROM section - big box
  display.drawFillRect(5, 22, screenWidth - 10, 35, display.color(30, 35, 45));
  display.drawRect(5, 22, screenWidth - 10, 35, editField <= 1 ? CYAN : GRAY);

  display.setTextColor(editField === 0 ? GREEN : WHITE);
  display.setTextSize(2);
  display.drawString("" + value, 12, 30);

  display.setTextColor(editField === 1 ? CYAN : WHITE);
  display.setTextSize(1);
  display.drawString(units[fromIndex], screenWidth - 45, 38);

  // Arrow
  display.setTextColor(ORANGE);
  display.setTextSize(2);
  display.drawString("=", screenWidth / 2 - 8, 60);

  // TO section - big box with result
  display.drawFillRect(5, 75, screenWidth - 10, 35, display.color(30, 35, 45));
  display.drawRect(5, 75, screenWidth - 10, 35, editField === 2 ? CYAN : GRAY);

  var result = convert(value, fromIndex, toIndex);
  // Smart formatting
  var resultStr;
  if (Math.abs(result) >= 10000 || (Math.abs(result) < 0.01 && result !== 0)) {
    resultStr = result.toExponential(2);
  } else {
    resultStr = "" + Math.round(result * 1000) / 1000;
  }

  display.setTextColor(ORANGE);
  display.setTextSize(2);
  display.drawString(resultStr, 12, 83);

  display.setTextColor(editField === 2 ? CYAN : WHITE);
  display.setTextSize(1);
  display.drawString(units[toIndex], screenWidth - 45, 91);

  // Controls
  display.setTextColor(GRAY);
  display.drawString("SEL:Field </>:Adjust", 5, screenHeight - 10);
}

drawScreen();

while (!shouldExit) {
  var units = getUnits();

  if (keyboard.getSelPress()) {
    editField = (editField + 1) % 4;
    drawScreen();
  }

  if (keyboard.getPrevPress()) {
    if (editField === 0) {
      if (value > 100) value -= 100;
      else if (value > 10) value -= 10;
      else value = Math.max(0, value - 1);
    } else if (editField === 1)
      fromIndex = (fromIndex - 1 + units.length) % units.length;
    else if (editField === 2)
      toIndex = (toIndex - 1 + units.length) % units.length;
    else {
      catIndex = (catIndex - 1 + categories.length) % categories.length;
      fromIndex = 0;
      toIndex = 1;
    }
    drawScreen();
  }

  if (keyboard.getNextPress()) {
    if (editField === 0) {
      if (value >= 100) value += 100;
      else if (value >= 10) value += 10;
      else value += 1;
    } else if (editField === 1) fromIndex = (fromIndex + 1) % units.length;
    else if (editField === 2) toIndex = (toIndex + 1) % units.length;
    else {
      catIndex = (catIndex + 1) % categories.length;
      fromIndex = 0;
      toIndex = 1;
    }
    drawScreen();
  }

  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  delay(80);
}
