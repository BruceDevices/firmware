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

var paddleY = 150;
var paddleX = 140;
var paddleWidth = 40;
var paddleHeight = 6;
var paddleSpeed = 4;

var aiY = 20;
var aiX = 140;
var aiWidth = 40;
var aiHeight = 6;
var aiSpeed = 1.8;

var ballX = 160;
var ballY = 85;
var ballSize = 6;
var ballSpeedX = 4;
var ballSpeedY = 4;

var playerScore = 0;
var aiScore = 0;
var playerLives = 3;
var level = 1;
var gameRunning = true;
var gameOver = false;
var menuOpen = false;
var menuSelection = 0;

var screenWidth = width();
var screenHeight = height();

var paddleColor = 0xFFFFFF;
var aiColor = 0xFFFFFF;
var ballColor = 0xFFFF00;
var bgColor = 0x000000;

function resetBall() {
    ballX = screenWidth / 2;
    ballY = screenHeight / 2;
    ballSpeedX = 0;
    ballSpeedY = 0;

    for (var countdown = 3; countdown > 0; countdown--) {
        fillScreen(bgColor);
        drawFillRect(paddleX, paddleY, paddleWidth, paddleHeight, paddleColor);
        drawFillRect(aiX, aiY, aiWidth, aiHeight, aiColor);
        drawFillRect(ballX, ballY, ballSize, ballSize, ballColor);

        for (var i = 0; i < screenWidth; i += 8) {
            drawFillRect(i, screenHeight / 2 - 1, 4, 2, 0x666666);
        }

        setTextSize(1);
        drawString("Score: " + playerScore, 5, 5);
        drawString("Lives: " + playerLives, 5, 15);
        drawString("Level: " + level, 5, 25);
        drawString("AI: " + aiScore, screenWidth - 50, 5);

        setTextSize(3);
        drawString(countdown.toString(), screenWidth / 2 - 8, screenHeight / 2 - 10);

        delay(500);
    }

    var speed = 4;
    ballSpeedX = 0;
    ballSpeedY = speed;
}

function updateAI() {
    var aiCenter = aiX + aiWidth / 2;
    var ballCenter = ballX + ballSize / 2;
    var currentAiSpeed = aiSpeed + (level - 1) * 0.5;

    if (currentAiSpeed > 6) currentAiSpeed = 6;

    if (ballSpeedY < 0) {
        if (aiCenter < ballCenter - 5) {
            aiX += currentAiSpeed;
            if (aiX > screenWidth - aiWidth) aiX = screenWidth - aiWidth;
        } else if (aiCenter > ballCenter + 5) {
            aiX -= currentAiSpeed;
            if (aiX < 0) aiX = 0;
        }
    }
}

function updateBall() {
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    if (ballX <= 0 || ballX >= screenWidth - ballSize) {
        ballSpeedX = -ballSpeedX;
    }

    if (ballSpeedY > 0 &&
        ballY + ballSize >= paddleY &&
        ballY < paddleY + paddleHeight &&
        ballX + ballSize > paddleX &&
        ballX < paddleX + paddleWidth) {

        var hitPos = (ballX + ballSize / 2 - paddleX) / paddleWidth;
        hitPos = Math.max(0.05, Math.min(0.95, hitPos));
        var baseAngle = (hitPos - 0.5) * 80;
        var randomVariation = (Math.random() - 0.5) * 30;
        var angle = baseAngle + randomVariation;
        var speed = Math.sqrt(ballSpeedX * ballSpeedX + ballSpeedY * ballSpeedY);
        speed += (Math.random() - 0.5) * 1;
        if (speed < 3) speed = 3;
        if (speed > 8) speed = 8;
        ballSpeedX = Math.sin(angle * Math.PI / 180) * speed;
        ballSpeedY = -Math.abs(Math.cos(angle * Math.PI / 180) * speed);
        ballY = paddleY - ballSize - 1;
        playerScore++;
    }

    if (ballSpeedY < 0 &&
        ballY <= aiY + aiHeight &&
        ballY + ballSize > aiY &&
        ballX + ballSize > aiX &&
        ballX < aiX + aiWidth) {

        var hitPos = (ballX + ballSize / 2 - aiX) / aiWidth;
        hitPos = Math.max(0.05, Math.min(0.95, hitPos));
        var baseAngle = (hitPos - 0.5) * 80;
        var randomVariation = (Math.random() - 0.5) * 30;
        var angle = baseAngle + randomVariation;
        var speed = Math.sqrt(ballSpeedX * ballSpeedX + ballSpeedY * ballSpeedY);
        speed += (Math.random() - 0.5) * 1;
        if (speed < 3) speed = 3;
        if (speed > 8) speed = 8;
        ballSpeedX = Math.sin(angle * Math.PI / 180) * speed;
        ballSpeedY = Math.abs(Math.cos(angle * Math.PI / 180) * speed);
        ballY = aiY + aiHeight + 1;
        aiScore++;
    }

    if (ballY >= screenHeight + 5) {
        playerLives--;
        paddleX = screenWidth / 2 - paddleWidth / 2;
        aiX = screenWidth / 2 - aiWidth / 2;
        if (playerLives <= 0) {
            gameOver = true;
            gameRunning = false;
        } else {
            resetBall();
        }
    }

    if (ballY <= -5) {
        if (aiScore > 0 && aiScore % 3 === 0 && aiScore > level * 3) {
            level++;
        }
        paddleX = screenWidth / 2 - paddleWidth / 2;
        aiX = screenWidth / 2 - aiWidth / 2;
        resetBall();
    }
}

function drawGame() {
    fillScreen(bgColor);

    drawFillRect(paddleX, paddleY, paddleWidth, paddleHeight, paddleColor);
    drawFillRect(aiX, aiY, aiWidth, aiHeight, aiColor);
    drawFillRect(ballX, ballY, ballSize, ballSize, ballColor);

    for (var i = 0; i < screenWidth; i += 8) {
        drawFillRect(i, screenHeight / 2 - 1, 4, 2, 0x666666);
    }

    setTextSize(1);
    drawString("Score: " + playerScore, 5, 5);
    drawString("Lives: " + playerLives, 5, 15);
    drawString("Level: " + level, 5, 25);
    drawString("AI: " + aiScore, screenWidth - 50, 5);
    drawString("Developed by MSI", screenWidth - 100, screenHeight - 15);

    if (gameOver && !menuOpen) {
        drawFillRect(40, 45, 200, 90, 0x202040);
        drawFillRect(41, 46, 198, 88, 0xFF6464);

        setTextSize(2);
        drawString("GAME OVER", 75, 65);
        setTextSize(1);
        drawString("Final Score: " + playerScore, 85, 85);
        drawString("Level: " + level, 80, 100);
        drawString("Press any key to restart", 70, 115);
    }

    if (menuOpen) {
        drawFillRect(50, 50, 180, 100, 0x193250);
        drawFillRect(51, 51, 178, 98, 0x0096FF);

        setTextSize(2);
        drawString("MENU", 115, 70);

        if (menuSelection === 0) {
            drawFillRect(70, 90, 140, 15, 0x0064C8);
        }
        if (menuSelection === 1) {
            drawFillRect(70, 110, 140, 15, 0x0064C8);
        }

        setTextSize(1);
        drawString("Continue", 90, 95);
        drawString("New Game", 90, 115);
        drawString("Scroll/Prev-Next, Select", 65, 135);
    }
}

fillScreen(bgColor);
setTextSize(1);
drawString('Scroll/Prev = Left/Right', 5, 50);
drawString('Select = Menu', 5, 65);
drawString('Press Select to Start', 5, 80);

while (true) {
    if (getSelPress()) {
        delay(300);
        break;
    }
}

resetBall();

while (true) {
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
                playerScore = 0;
                aiScore = 0;
                playerLives = 3;
                level = 1;
                gameOver = false;
                gameRunning = true;
                menuOpen = false;
                resetBall();
            }
            delay(300);
        }
    } else if (gameOver) {
        if (getNextPress() || getPrevPress() || getSelPress()) {
            playerScore = 0;
            aiScore = 0;
            playerLives = 3;
            level = 1;
            gameOver = false;
            gameRunning = true;
            resetBall();
            delay(300);
        }
    } else if (gameRunning) {
        if (getNextPress() && paddleX + paddleWidth < screenWidth) {
            paddleX += paddleSpeed;
            delay(10);
        }
        if (getPrevPress() && paddleX > 0) {
            paddleX -= paddleSpeed;
            delay(10);
        }
        if (getSelPress()) {
            menuOpen = true;
            gameRunning = false;
            menuSelection = 0;
            delay(300);
        }

        updateAI();
        updateBall();
    }

    drawGame();

    if (getEscPress()) {
        break;
    }

    delay(20);
}
