var display = require('display');
var keyboard = require('keyboard');
var device = require('device');

var screenWidth = display.width();
var screenHeight = display.height();
var battery = device.getBatteryCharge();
var board = device.getBoard();
var redraw=true;

display.setTextSize(2);
function updateBtn() {
    while (true) {
        if(keyboard.getEscPress()) {
            display.drawFillRect(0, 0, screenWidth, screenHeight, display.color(255, 0, 0));
            display.setTextColor(display.color(0,0,0));
            display.drawString("Exiting", screenWidth / 2 - 60, screenHeight / 2 - 10);
            delay(3000);
            break; // Exits the game when a prev button is pressed.;
        }
        if(keyboard.getPrevPress()) {
            display.setTextColor(display.color(255,0,0));
            display.drawFillRect(0, 0, screenWidth, screenHeight, display.color(0, 0, 0));
            display.drawString("Prev Btn Pressed ", 10, screenHeight/2-8);
            redraw=true;
        }
        if(keyboard.getSelPress()) {
            display.setTextColor(display.color(0,255,0));
            display.drawFillRect(0, 0, screenWidth, screenHeight, display.color(0, 0, 0));
            display.drawString("Selec Btn Pressed", 10, screenHeight/2-8);
            redraw=true;
        }
        if(keyboard.getNextPress()) {
            display.setTextColor(display.color(0,0,255));
            display.drawFillRect(0, 0, screenWidth, screenHeight, display.color(0, 0, 0));
            display.drawString("Next Btn Pressed ", 10, screenHeight/2-8);
            redraw=true;
        }
        if(redraw==true) {
            battery = device.getBatteryCharge();
            display.drawString("Battery level: " + battery, 10, 10);
            display.drawString("Board: " + board, 10, 27);
            display.drawString("Sel + Next to Exit", 10, screenHeight-22);
            display.drawRect(5,5,screenWidth-10, screenHeight-10, display.color(150,20,210));
            delay(200);
            redraw=false;
        }
    }
}

updateBtn();

