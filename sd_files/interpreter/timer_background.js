var runtime = require("runtime");
var audio = require("audio");
var display = require("display");
var keyboard = require("keyboard");
var gpio = require("gpio");

var alarmTime = 0;
var isAlarmTimeActive = false;
var alarmMinutes = 0;
var alarmSeconds = 10;

var halfWidth = display.width() / 2;
var colorBlack = display.color(0, 0, 0);
var colorWhite = display.color(255, 255, 255);

gpio.pinMode(19, OUTPUT);

runtime.main(function () {
  isAlarmTimeActive = false;
  while (true) {
    display.setTextAlign('center', 'top');
    display.setTextSize(2);
    var secondsPadded = alarmSeconds < 10 ? ("0" + alarmSeconds) : alarmSeconds;
    display.setTextColor(colorWhite, colorBlack);
    display.drawText(alarmMinutes + ":" + secondsPadded, halfWidth, 10);
    if (keyboard.getNextPress()) {
      alarmSeconds += 1;
      if (alarmSeconds >= 60) {
        alarmSeconds = 0;
        alarmMinutes += 1;
      }
    }
    if (keyboard.getSelPress()) {
      var alarmDurationMs = (alarmMinutes * 60 + alarmSeconds) * 1000;
      alarmTime = now() + alarmDurationMs;
      isAlarmTimeActive = true;
      return;
    }
    if (keyboard.getPrevPress()) {
      exit();
    }
    delay(50);
  }
});

setInterval(function () {
  if (isAlarmTimeActive) {
    var timeRemainingMs = alarmTime - now();
    // display time remaining
    if (timeRemainingMs <= 0) {
      alarmMinutes = 0;
      alarmSeconds = 0;
    }
    else {
      var totalSecondsRemaining = Math.ceil(timeRemainingMs / 1000);
      alarmMinutes = Math.floor(totalSecondsRemaining / 60);
      alarmSeconds = totalSecondsRemaining % 60;
    }
    var secondsPadded = alarmSeconds < 10 ? ("0" + alarmSeconds) : alarmSeconds;
    display.setTextAlign('center', 'top');
    display.setTextSize(1);
    display.setTextColor(colorWhite, colorBlack);
    display.drawText(alarmMinutes + ":" + secondsPadded, display.width() / 2, 10);
  }
  if (isAlarmTimeActive && now() >= alarmTime) {
    isAlarmTimeActive = false;
    gpio.digitalWrite(19, HIGH);
    audio.tone(440, 200);
    delay(300);

    gpio.digitalWrite(19, LOW);
    audio.tone(440, 200);
    delay(300);

    gpio.digitalWrite(19, HIGH);
    audio.tone(440, 200);
    delay(300);

    gpio.digitalWrite(19, LOW);
    runtime.toForeground();
  }
}, 500);
