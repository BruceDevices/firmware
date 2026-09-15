var display = require('display');
var keyboard = require('keyboard');

var width = display.width;
var height = display.height;
var color = display.color;
var fillScreen = display.fill;
var drawFillRect = display.drawFillRect;
var drawRect = display.drawRect;
var drawString = display.drawString;
var setTextColor = display.setTextColor;
var setTextSize = display.setTextSize;

var getPrevPress = keyboard.getPrevPress;
var getNextPress = keyboard.getNextPress;
var getSelPress = keyboard.getSelPress;
var getEscPress = keyboard.getEscPress;
var getAnyPress = keyboard.getAnyPress;

var playerX = 160;
var playerY = 130;
var playerTargetX = 160;
var playerLane = 1;
var playerSpeed = 6;
var playerScore = 0;

var cars = [];
var carSpawnTimer = 0;
var gameSpeed = 4;

var gameRunning = true;
var gameOver = false;
var menuOpen = false;
var menuSelection = 0;

var screenWidth = width();
var screenHeight = height();

var playerColor = 0x0066FF;
var enemyColor1 = 0x333333;
var enemyColor2 = 0x666666;
var enemyColor3 = 0xFFFFFF;
var roadColor = 0x000000;
var lineColor = 0xFFFFFF;
var bgColor = 0x000000;

var roadLines = [];
var laneWidth = 60;
var roadStart = 40;

function initGame() {
    playerX = Math.floor(roadStart + laneWidth * 1.5);
    playerTargetX = playerX;
    playerY = screenHeight - 40;
    playerLane = 1;
    playerScore = 0;
    cars = [];
    carSpawnTimer = 0;
    gameSpeed = 4;
    gameOver = false;
    gameRunning = true;

    roadLines = [];
    for (var i = 0; i < 15; i++) {
        roadLines.push({
            y: i * 24
        });
    }
}

function updateRoadLines() {
    for (var i = 0; i < roadLines.length; i++) {
        roadLines[i].y += gameSpeed;
        if (roadLines[i].y > screenHeight) {
            roadLines[i].y = -20;
        }
    }
}

function updatePlayer() {
    if (menuOpen) {
        if (getNextPress()) {
            menuSelection = menuSelection < 1 ? menuSelection + 1 : 0;
            delay(150);
        }
        if (getPrevPress()) {
            menuSelection = menuSelection > 0 ? menuSelection - 1 : 1;
            delay(150);
        }
        if (getSelPress()) {
            if (menuSelection === 0) {
                menuOpen = false;
                gameRunning = true;
            } else {
                initGame();
                menuOpen = false;
            }
            delay(300);
        }
        return;
    }

    if (getNextPress() && playerLane < 3) {
        playerLane++;
        playerTargetX = Math.floor(roadStart + laneWidth * (playerLane + 0.5));
    }
    if (getPrevPress() && playerLane > 0) {
        playerLane--;
        playerTargetX = Math.floor(roadStart + laneWidth * (playerLane + 0.5));
    }

    if (playerX !== playerTargetX) {
        var diff = playerTargetX - playerX;
        if (Math.abs(diff) <= 10) {
            playerX = playerTargetX;
        } else {
            playerX += diff > 0 ? 10 : -10;
        }
    }

    if (getSelPress()) {
        if (gameOver) {
            initGame();
            delay(300);
        } else {
            menuOpen = true;
            gameRunning = false;
            menuSelection = 0;
            delay(300);
        }
    }

    if (gameRunning && !gameOver) {
        playerScore += 1;
        gameSpeed += 0.005;
        if (gameSpeed > 8) gameSpeed = 8;
    }
}

function createCar() {
    var lane = Math.floor(Math.random() * 4);
    var carX = Math.floor(roadStart + laneWidth * (lane + 0.5));
    var carType = Math.floor(Math.random() * 3);
    var color = carType === 0 ? enemyColor1 : carType === 1 ? enemyColor2 : enemyColor3;

    for (var i = 0; i < cars.length; i++) {
        var existingCar = cars[i];
        if (Math.abs(existingCar.x - carX) < 20 && existingCar.y < 50) {
            return;
        }
    }

    cars.push({
        x: carX,
        y: -20,
        lane: lane,
        speed: Math.floor(2 + Math.random() * 3),
        color: color,
        width: 12,
        height: 20
    });
}

function updateCars() {
    carSpawnTimer++;
    if (carSpawnTimer > 40 - gameSpeed * 2) {
        carSpawnTimer = 0;
        createCar();
        if (Math.random() < 0.4) {
            createCar();
        }
        if (Math.random() < 0.2) {
            createCar();
        }
    }

    for (var i = cars.length - 1; i >= 0; i--) {
        var car = cars[i];
        car.y += car.speed + gameSpeed;

        if (car.y > screenHeight + 30) {
            cars.splice(i, 1);
            continue;
        }

        if (car.x + 8 > playerX - 8 &&
            car.x - 8 < playerX + 8 &&
            car.y + 18 > playerY - 12 &&
            car.y - 2 < playerY + 12) {

            gameOver = true;
            gameRunning = false;
        }
    }
}

function drawRoad() {
    fillScreen(bgColor);

    drawFillRect(roadStart, 0, laneWidth * 4, screenHeight, roadColor);

    for (var i = 1; i < 4; i++) {
        var lineX = roadStart + laneWidth * i;
        for (var j = 0; j < roadLines.length; j++) {
            drawFillRect(lineX - 1, roadLines[j].y, 2, 10, lineColor);
        }
    }

    drawFillRect(roadStart - 2, 0, 2, screenHeight, lineColor);
    drawFillRect(roadStart + laneWidth * 4, 0, 2, screenHeight, lineColor);
}

function drawPlayer() {
    var x = playerX - 6;
    var y = playerY - 10;

    drawFillRect(x + 2, y, 8, 4, 0x333333);
    drawFillRect(x + 1, y + 4, 10, 8, playerColor);
    drawFillRect(x, y + 12, 12, 6, playerColor);
    drawFillRect(x + 1, y + 18, 10, 2, 0x0044AA);
    drawFillRect(x + 4, y + 1, 4, 2, 0xFFFFFF);
    drawFillRect(x + 1, y + 15, 2, 2, 0x000000);
    drawFillRect(x + 9, y + 15, 2, 2, 0x000000);
}

function drawCars() {
    for (var i = 0; i < cars.length; i++) {
        var car = cars[i];
        var x = car.x - car.width / 2;
        var y = car.y;

        drawFillRect(x + 2, y + 16, 8, 4, 0x333333);
        drawFillRect(x + 1, y + 8, 10, 8, car.color);
        drawFillRect(x, y, 12, 8, car.color);
        drawFillRect(x + 1, y, 10, 2, 0xAA0000);
        drawFillRect(x + 4, y + 17, 4, 2, 0xFFFFFF);
        drawFillRect(x + 1, y + 3, 2, 2, 0x000000);
        drawFillRect(x + 9, y + 3, 2, 2, 0x000000);
    }
}

function drawGame() {
    drawRoad();
    drawPlayer();
    drawCars();

    setTextSize(1);
    drawString("Score: " + playerScore, 5, 5);
    drawString("Speed: " + Math.floor(gameSpeed * 10), 5, 15);
    drawString("Lane: " + (playerLane + 1), 5, 25);
    drawString("Developed by MSI", screenWidth - 100, screenHeight - 15);

    if (gameOver && !menuOpen) {
        drawFillRect(40, 50, 200, 80, 0x202040);
        drawFillRect(41, 51, 198, 78, 0xFF6464);

        setTextSize(2);
        drawString("CRASHED!", 75, 70);
        setTextSize(1);
        drawString("Final Score: " + playerScore, 80, 90);
        drawString("Press Select to restart", 65, 110);
    }

    if (menuOpen) {
        drawFillRect(50, 50, 180, 80, 0x193250);
        drawFillRect(51, 51, 178, 78, 0x0096FF);

        setTextSize(2);
        drawString("PAUSE", 100, 70);

        if (menuSelection === 0) {
            drawFillRect(70, 85, 120, 15, 0x0064C8);
        }
        if (menuSelection === 1) {
            drawFillRect(70, 105, 120, 15, 0x0064C8);
        }

        setTextSize(1);
        drawString("Continue", 90, 90);
        drawString("New Game", 90, 110);
    }
}

fillScreen(bgColor);
setTextSize(1);
drawString('Highway Racer', 5, 40);
drawString('Prev/Next = Change Lane', 5, 55);
drawString('Dodge the traffic!', 5, 70);
drawString('Select = Menu', 5, 85);
drawString('Press Select to Start', 5, 100);

while (true) {
    if (getSelPress()) {
        delay(300);
        break;
    }
}

initGame();

while (true) {
    updatePlayer();

    if (gameRunning && !gameOver && !menuOpen) {
        updateRoadLines();
        updateCars();
    }

    drawGame();

    if (getEscPress()) {
        break;
    }

    delay(16);
}
