// created by hxd57. V2
// repository: https://github.com/HawkstoNGriM/m5-bruce-tamagochi-lite
var dbStore = {fs: "sd", path: "/pet.json"}; // fs can be "sd" or "littlefs"
var storage = require("storage");
var display = require('display');
var keyboard = require('keyboard');
var dialog = require('dialog');
var serialApi = require('serial');

var width = display.width;
var height = display.height;
var color = display.color;
var fillScreen = display.fill;
var drawString = display.drawString;
var setTextColor = display.setTextColor;
var setTextSize = display.setTextSize;

var serialCmd = serialApi.cmd;
var pastelColors = {
  "Peach": color(255, 223, 186),
  "Mint": color(186, 255, 201),
  "Pink": color(255, 186, 255),
  "Blue": color(186, 225, 255),
  "Yellow": color(255, 255, 186),
  "White": color(255, 255, 255),
  "Lavender": color(230, 230, 250),
  "Coral": color(255, 127, 80),
  "Aqua": color(127, 255, 212),
  "Beige": color(245, 245, 220)
};
var currentBgColor = pastelColors["Peach"];

var faceColors = {
  "Black": color(0, 0, 0),
  "White": color(255, 255, 255),
  "Red": color(255, 0, 0),
  "Blue": color(0, 0, 255),
  "Green": color(0, 255, 0),
  "Purple": color(128, 0, 128)
};
var faceColor = faceColors["Black"]; // Default face color

function Pet(name, type, hunger, cleanliness, happiness, timeLastFed, timeLastPet, timeLastCleaned) {
  this.name = name || "gotchi"; // Changed default name to "gotchi"
  this.type = type || "cat";
  this.hunger = Math.min(100, Math.max(0, hunger !== undefined ? hunger : 0)); // Ensure hunger is between 0% and 100%
  this.cleanliness = Math.min(100, Math.max(0, cleanliness !== undefined ? cleanliness : 100)); // Ensure cleanliness is between 0% and 100%
  this.happiness = Math.min(100, Math.max(0, happiness !== undefined ? happiness : 50)); // Ensure happiness is between 0% and 100%
  var time = now();
  this.timeLastFed = timeLastFed || time;
  this.timeLastPet = timeLastPet || time;
  this.timeLastCleaned = timeLastCleaned || time; // Track cleaning time
}

Pet.prototype = {
  feed: function() {
    this.hunger = Math.max(0, this.hunger - 10); // Reduce hunger by 10%, but not below 0%
    this.timeLastFed = now();
    this.happiness = Math.min(100, this.happiness + 20); // Increase happiness by 20%, but not above 100%
    display.fill(currentBgColor);
    dialog.message(this.name + " has been fed!");
  },
  clean: function() {
    this.cleanliness = 100; // Fully clean
    this.timeLastCleaned = now();
    display.fill(currentBgColor);
    dialog.message(this.name + " is now clean!");
  },
  pet: function() {
    this.happiness = Math.min(100, this.happiness + 10); // Reduced happiness gain
    this.timeLastPet = now();
    display.fill(currentBgColor);
    dialog.message(this.name + " loves your petting!");
  },
  updateHunger: function() {
    var time = now();
    var elapsed = time - this.timeLastFed;
    this.hunger = Math.min(100, Math.max(0, this.hunger + Math.floor(elapsed / 7200000) * 10)); // Ensure hunger is between 0% and 100%
  },
  updateHappiness: function() {
    var time = now();
    var elapsed = time - this.timeLastPet;
    this.happiness = Math.max(0, this.happiness - Math.floor(elapsed / 3600000) * 5); // Ensure happiness is between 0% and 100%
  },
  updateCleanliness: function() {
    var time = now();
    var elapsed = time - this.timeLastCleaned;
    var hours = elapsed / 3600000;
    this.cleanliness = Math.max(0, 100 - Math.floor(hours * 5)); // Ensure cleanliness is between 0% and 100%
  },
  save: function() {
    var petData = {
      name: this.name,
      type: this.type,
      hunger: this.hunger,
      cleanliness: this.cleanliness,
      happiness: this.happiness,
      timeLastFed: this.timeLastFed,
      timeLastPet: this.timeLastPet,
      timeLastCleaned: this.timeLastCleaned // Save cleaning time
    };
    var jsonString = JSON.stringify(petData) + "\n";
    storage.write(dbStore, jsonString);
  }
};


function loadPet() {
  var data = null;
  try {
    data = storage.read(dbStore);
  } catch (e1) {
    return null;
  }

  if (data) {
    try {
      var lines = data.split("\n");
      // Filter out empty lines
      var nonEmptyLines = [];
      for (var i = 0; i < lines.length; i++) {
        if (lines[i].trim() !== "") nonEmptyLines.push(lines[i]);
      }
      var lastLine = nonEmptyLines[nonEmptyLines.length - 1];
      if (lastLine) {
        var obj = JSON.parse(lastLine);
        // Ensure timeLastFed and timeLastPet are numbers
        obj.timeLastFed = Number(obj.timeLastFed);
        obj.timeLastPet = Number(obj.timeLastPet);
         // Handle old saves without timeLastCleaned
        if (obj.timeLastCleaned === undefined) {
          var hoursAgo = (100 - obj.cleanliness) / 5;
          obj.timeLastCleaned = now() - hoursAgo * 3600000;
        } else {
          obj.timeLastCleaned = Number(obj.timeLastCleaned);
        }
        return new Pet(obj.name, obj.type, obj.hunger, obj.cleanliness,
                       obj.happiness, obj.timeLastFed, obj.timeLastPet, obj.timeLastCleaned);
      }
    } catch (e2) {
      display.fill(currentBgColor);
      dialog.error("Failed to load pet data: " + e.message);
      return null;
    }
  }
  return null; // No pet file found
}

function drawPet(pet) {
  var screenWidth = width();
  var screenHeight = height();
  var textYSpacing = 14;

  var faces = {
    cat: [" >_< ", "=^_^=", " ^-^ "],
    dog: [" T_T ", " o_o ", " ^_^ "],
    bird: [" x_x ", " -_- ", " ^v^ "]
  };

  fillScreen(currentBgColor);
  setTextColor(faceColor);
  setTextSize(2);

  // Include cleanliness in state calculation
  var stateIndex = (pet.hunger >= 70 || pet.happiness <= 30 || pet.cleanliness <= 30) ? 0 :
                   (pet.hunger >= 30 || pet.happiness <= 50 || pet.cleanliness <= 50) ? 1 : 2;

  var face = faces[pet.type][stateIndex];
  var faceWidth = face.length * 12;
  var faceX = Math.floor((screenWidth - faceWidth) / 2);
  var faceY = Math.floor(screenHeight * 0.3);

  var time = now();
  if (time % 4000 < 200) {
    face = face.replace(/[^\s]/g, "－");
  }
  if (stateIndex === 2 && time % 1000 < 500) {
    faceX += Math.sin(time / 200) * (screenWidth * 0.1);
  }

  drawString(face, faceX, faceY);
  drawString(face, faceX + 1, faceY);

  var happyText = "Happy: " + pet.happiness + "%";
  var happyWidth = happyText.length * 12;
  var happyX = Math.floor((screenWidth - happyWidth) / 2);
  drawString(happyText, happyX, 10);

  var statusY = screenHeight - 60;
  drawString("Hngr: " + pet.hunger + "%", 10, statusY);
  drawString("Clean: " + pet.cleanliness + "%", 10, statusY + textYSpacing);

  var fedHrs = Math.floor((time - pet.timeLastFed) / 3600000);
  var petHrs = Math.floor((time - pet.timeLastPet) / 3600000);
  drawString("Fed:" + fedHrs + "h  Pet:" + petHrs + "h", 10, screenHeight - 30);
}



function showHeartAnimation() {
  var screenWidth = width();
  var screenHeight = height();
  var heartText = "<3-<3";
  var heartWidth = heartText.length * 12;
  var heartX = Math.floor((screenWidth - heartWidth) / 2);
  var heartY = Math.floor(screenHeight * 0.5);

  // Use the current background color
  fillScreen(currentBgColor);
  setTextColor(faceColor);
  setTextSize(1);

  // Draw the heart
  drawString(heartText, heartX, heartY);

  delay(1000);

  var heartWidth2 = 7 * 12;
  var heartX = Math.floor((screenWidth - heartWidth2) / 2);
  var heartY = Math.floor(screenHeight * 0.5);
  fillScreen(currentBgColor);
  setTextColor(faceColor);
  setTextSize(1);
  drawString(heartText, heartX, heartY);

  delay(1000);

  var heartWidth2 = 3 * 12;
  var heartX = Math.floor((screenWidth - heartWidth2) / 2);
  var heartY = Math.floor(screenHeight * 0.5);
  fillScreen(currentBgColor);
  setTextColor(color(255, 0, 0));
  setTextSize(1);
  drawString(heartText, heartX, heartY);

  delay(1000);

  // Restore the original background color
  fillScreen(currentBgColor);
}

var pet = loadPet();

if (!pet) {
  fillScreen(currentBgColor);
  setTextColor(faceColor);
  setTextSize(2);
  var name = keyboard.keyboard("", 12, "Pet's name?") || "gotchi"; // Default to "gotchi"
  var type = dialog.choice([["Cat", "cat"], ["Dog", "dog"], ["Bird", "bird"]]) || "cat";
  pet = new Pet(name, type);
  pet.save();
}

var updateTime=0;
while (true) {
  if(now()-updateTime>500){
    pet.updateHunger();
    pet.updateHappiness();
    pet.updateCleanliness();
    drawPet(pet);
    updateTime = now();
  }

  if (keyboard.getNextPress()) {
    var choice = dialog.choice([
      ["Pet", "pet"],
      ["Feed", "feed"],
      ["Clean", "clean"],
      ["Heart", "heart"],
      ["Settings", "settings"],
      ["New Pet", "newpet"],
      ["Exit", "exit"]
    ]) || "";

    if (choice === "exit") {
      pet.save(); // Save before exiting
      break;
    }

    if (choice === "settings") {
      var setting = dialog.choice([
        ["Change BG Color", "bgcolor"],
        ["Face Color", "facecolor"],
        ["Back", "back"]
      ]);
      if (setting === "bgcolor") {
        var colorChoice = dialog.choice([
          "Peach",
          "Mint",
          "Pink",
          "Blue",
          "Yellow",
          "White",
          "Lavender",
          "Coral",
          "Aqua",
          "Beige"
        ]);
        if (colorChoice && pastelColors[colorChoice]) {
          currentBgColor = pastelColors[colorChoice];
        }
      }
      else if (setting === "facecolor") {
        var fc = dialog.choice([
          ["Black", "black"],
          ["White", "white"],
          ["Red", "red"],
          ["Blue", "blue"],
          ["Green", "green"],
          ["Purple", "purple"]
        ]);
        faceColor = faceColors[fc];
      }
    }
    else if (choice === "newpet") {
      var confirm = dialog.choice([
        ["Yes (will delete old pet)", "yes"],
        ["No (cancel)", "no"]
      ]);

      if (confirm === "yes") {
        serialCmd("storage remove JS-scripts/bruce_0.sub");
        serialCmd("storage remove pet.json");

        // Create a new pet
        fillScreen(currentBgColor);
        setTextColor(faceColor);
        setTextSize(2);
        var name = keyboard.keyboard("", 12, "Pet's name?") || "gotchi"; // Default to "gotchi"
        var type = dialog.choice([["Cat", "cat"], ["Dog", "dog"], ["Bird", "bird"]]) || "cat";
        pet = new Pet(name, type);
        pet.save();
      }
    }
    else if (choice === "heart") {
      showHeartAnimation();
    }
    else if (choice && pet[choice]) {
      pet[choice]();
      delay(1000);
    }
    pet.save(); // Save after performing an action
  }

  delay(10);
}
