// Snake Example

var display = require('display');
var keyboard = require('keyboard');

var width = display.width;
var height = display.height;
var color = display.color;

var screenWidth = width();
var screenHeight = height();
var cBG = color(0, 0, 0);
var cSnake = color(150, 150, 255);
var cApple = color(120, 255, 120);
var gridSize = 8;
var headX = 4;
var headY = 4;
var appleX = 8;
var appleY = 4;
var headDir = 3; // 0 = up, 1 = down, 2 = left, 3 = right
var totalDelay = 400;
var delayTime = totalDelay;
var tails = [];
var canMove = true;
var time = now();
var prevTime = now();
tails.push([headX, headY]);
var exitApp = false;

function init() {
    headX = 4;
    headY = 4;
    appleX = 8;
    appleY = 4;
    headDir = 3; // 0 = up, 1 = down, 2 = left, 3 = right
    totalDelay = 400;
    delayTime = totalDelay;
    tails = [];
    canMove = true;
    time = now();
    prevTime = now();
    tails.push([headX, headY]);

    display.fill(cBG);
    display.setTextSize(2);
    display.drawFillRect(headX * gridSize, headY * gridSize, gridSize, gridSize, cSnake);
    display.drawFillRect(appleX * gridSize, appleY * gridSize, gridSize, gridSize, cApple);
}

init();

function gameOver() {
    display.setTextSize(2);
    display.drawFillRect(0, 0, screenWidth, screenHeight, color(255, 0, 0));
    display.setTextColor(cBG);
    display.drawString("Game Over", screenWidth / 2 - 60, screenHeight / 2 - 10);
    delay(3000);
    init();
};

function updateTails() {
    if (tails.length > 0) {
        var lastTail = tails.pop();
        display.drawFillRect(lastTail[0] * gridSize, lastTail[1] * gridSize, gridSize, gridSize, cBG);
        tails.unshift([headX, headY]);

        for (var i = 0; i < tails.length; i++) {
            display.drawRect(tails[i][0] * gridSize + 1, tails[i][1] * gridSize + 1, gridSize - 2, gridSize - 2, cSnake);
        }
    }
};

function updateDelayTime() {
    var timePassed = time - prevTime;
    if (timePassed == 0) {
        time = now();
        return;
    }
    prevTime = time;
    time = now();
    delayTime -= timePassed;
};

function isOnSnake(x, y) {
    if (headX === x && headY === y) {
        return true;
    }
    for (var i = 0; i < tails.length; i++) {
        if (tails[i][0] === x && tails[i][1] === y) {
            return true;
        }
    }
    return false;
}

function updateSnake() {
    if (canMove) {
        switch (keyboard.getKeysPressed()[0]) {
            case ";":
                if (headDir !== 1) {
                    headDir = 0;
                    canMove = false;
                }
                break;
            case ".":
                if (headDir !== 0) {
                    headDir = 1;
                    canMove = false;
                }
                break;
            case ",":
                if (headDir !== 3) {
                    headDir = 2;
                    canMove = false;
                }
                break;
            case "/":
                if (headDir !== 2) {
                    headDir = 3;
                    canMove = false;
                }
                break;
        }
    }

    if (delayTime < 0) {
        canMove = true;
        var prevX = headX;
        var prevY = headY;

        switch (headDir) {
            case 0:
                headY -= 1;
                break;
            case 1:
                headY += 1;
                break;
            case 2:
                headX -= 1;
                break;
            case 3:
                headX += 1;
                break
        }

        if (headX !== prevX || headY !== prevY) {
            display.drawFillRect(prevX * gridSize, prevY * gridSize, gridSize, gridSize, cBG);
            display.drawFillRect(headX * gridSize, headY * gridSize, gridSize, gridSize, cSnake);
        }

        if (headX === appleX && headY === appleY) {
            do {
                appleX = Math.floor(Math.random() * (screenWidth / gridSize));
                appleY = Math.floor(Math.random() * (screenHeight / gridSize));
            } while (isOnSnake(appleX, appleY));
            display.drawFillRect(appleX * gridSize, appleY * gridSize, gridSize, gridSize, cApple);
            tails.push([prevX, prevY]);
            totalDelay -= 10;
        }

        updateTails();

        if (headX < 0 || headX >= screenWidth / gridSize || headY < 0 || headY >= screenHeight / gridSize) {
            gameOver();
        }

        for (var i = 1; i < tails.length; i++) {
            if (headX === tails[i][0] && headY === tails[i][1]) {
                gameOver();
            }
        }

        delayTime = totalDelay;
    }
}

while (!exitApp) {
    updateDelayTime();
    updateSnake();
    if (keyboard.getEscPress()) {
        exitApp = true;
    }
}
