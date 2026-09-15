var display = require('display');
var keyboardApi = require('keyboard');

var fillScreen = display.fill;
var drawRect = display.drawRect;
var drawFillRect = display.drawFillRect;
var drawString = display.drawString;
var setTextColor = display.setTextColor;
var setTextSize = display.setTextSize;

var getPrevPress = keyboardApi.getPrevPress;
var getSelPress = keyboardApi.getSelPress;
var getNextPress = keyboardApi.getNextPress;

var WIDTH = 240
var HEIGHT = 135;
var BLACK = 0;
var WHITE = 16777215;
var GREEN = 65280;
var BLUE = 255;
var YELLOW = 16776960;
var CYAN = 65535;
var MAGENTA = 16711935;
var ORANGE = 16753920;
var PURPLE = 8388736;
var DARKBLUE = 128;
var DARKRED = 8388608;
var LIGHTGREEN = 8453888;
var LIGHTBLUE = 10025880;
var GRAY = 8421504;
var GOLD = 16766720;
var BROWN = 10824234;
var RED = 255;
var STATE_MAIN_MENU = 0;
var STATE_BREAKOUT = 1;
var STATE_SNAKE = 2;
var STATE_SPACE_SHOOTER = 3;
var STATE_SLOTS = 4;
var STATE_FLAPPY_BIRD = 5;
var STATE_BLACKJACK = 6;
var STATE_PAUSED = 10;
var STATE_GAME_OVER = 11;
var STATE_LEVEL_UP = 12;
var STATE_EXIT_CONFIRM = 13;
var gameState = STATE_MAIN_MENU;
var prevGameState = -1;
var menuSelection = 0;
var pauseMenuSelection = 0;
var exitConfirmSelection = 0;
var staticDrawn = false;
var mainMenuStaticDrawn = false;
var pauseStaticDrawn = false;
var frameCounter = 0;
var menuLastSelState = false;
var mainMenuScroll = 0;
var MENU_VISIBLE_ITEMS = 5;
var menuOptions = ["BREAKOUT", "SNAKE", "SPACE SHOOTER", "SLOTS", "FLAPPY BIRD", "BLACKJACK", "QUIT"];
var menuLastSelection;
function drawMainMenu() {
    if (!mainMenuStaticDrawn || menuSelection !== menuLastSelection) {
        fillScreen(BLACK);
        setTextSize(3);
        setTextColor(YELLOW);
        drawString("ARCADE GAMES", 15, 15);
        drawFillRect(10, 40, WIDTH - 20, HEIGHT - 50, BLACK);
        drawRect(9, 39, WIDTH - 18, HEIGHT - 48, WHITE);
        setTextSize(1);
        setTextColor(WHITE);
        if (menuSelection >= mainMenuScroll + MENU_VISIBLE_ITEMS) {
            mainMenuScroll = menuSelection - MENU_VISIBLE_ITEMS + 1;
        } else if (menuSelection < mainMenuScroll) {
            mainMenuScroll = menuSelection;
        }
        var startY = 50;
        var itemHeight = 15;
        for (var i = 0; i < menuOptions.length; i++) {
            if (i < mainMenuScroll || i >= mainMenuScroll + MENU_VISIBLE_ITEMS) continue;
            var displayIdx = i - mainMenuScroll;
            if (i === menuSelection) {
                setTextColor(GREEN);
                drawFillRect(55, startY + displayIdx * itemHeight - 2, 130, 15, GRAY);
                drawString("> " + menuOptions[i], 60, startY + 4 + displayIdx * itemHeight);
            } else {
                setTextColor(WHITE);
                drawString(" " + menuOptions[i], 60, startY + 4 + displayIdx * itemHeight);
            }
        }
        if (mainMenuScroll > 0) {
            setTextColor(YELLOW);
            drawString("▲", WIDTH / 2 - 5, startY - 8);
        }
        if (mainMenuScroll + MENU_VISIBLE_ITEMS < menuOptions.length) {
            setTextColor(YELLOW);
            drawString("▼", WIDTH / 2 - 5, startY + MENU_VISIBLE_ITEMS * itemHeight + 8);
        }

        setTextColor(CYAN);
        drawString("Developed by msi", WIDTH - 95, HEIGHT - 10);

        mainMenuStaticDrawn = true;
        menuLastSelection = menuSelection;
    }
}
function drawPauseMenu() {
    if (!pauseStaticDrawn || pauseMenuSelection !== menuLastSelection) {
        fillScreen(BLACK);
        setTextSize(2);
        setTextColor(WHITE);
        drawString("PAUSED", 80, 40);
        var options = ["CONTINUE", "MAIN MENU", "QUIT"];
        setTextSize(1);
        for (var i = 0; i < options.length; i++) {
            if (i === pauseMenuSelection) {
                setTextColor(YELLOW);
                drawFillRect(70, 60 + i * 15, 100, 12, GRAY);
                drawString("> " + options[i], 75, 65 + i * 15);
            } else {
                setTextColor(WHITE);
                drawString(" " + options[i], 75, 65 + i * 15);
            }
        }
        setTextColor(CYAN);
        drawString("Developed by msi", WIDTH - 95, HEIGHT - 10);
        pauseStaticDrawn = true;
        menuLastSelection = pauseMenuSelection;
    }
}
function drawGameOverMenu() {
    if (!staticDrawn) {
        fillScreen(BLACK);
        setTextSize(2);
        setTextColor(YELLOW);
        drawString("GAME OVER", 65, 20);
        setTextColor(WHITE);
        setTextSize(1);
        if (prevGameState === STATE_FLAPPY_BIRD) drawString("Score: " + flappyScore, 90, 47);
        else if (prevGameState === STATE_SNAKE) drawString("Score: " + snakeScore, 90, 47);
        else if (prevGameState === STATE_SPACE_SHOOTER) drawString("Score: " + spaceScore, 90, 47);
        else if (prevGameState === STATE_BREAKOUT) drawString("Score: " + breakoutScore, 90, 47);
        var options = ["RESTART", "MAIN MENU", "QUIT"];
        for (var i = 0; i < options.length; i++) {
            if (i === pauseMenuSelection) {
                setTextColor(YELLOW);
                drawFillRect(70, 60 + i * 15, 100, 12, GRAY);
                drawString("> " + options[i], 75, 65 + i * 15);
            } else {
                setTextColor(WHITE);
                drawString(" " + options[i], 75, 65 + i * 15);
            }
        }

        setTextColor(CYAN);
        drawString("Developed by msi", WIDTH - 95, HEIGHT - 10);

        staticDrawn = true;
    }
}
function drawLevelUpMenu() {
    if (!staticDrawn) {
        fillScreen(BLACK);
        setTextSize(2);
        setTextColor(GREEN);
        drawString("LEVEL UP", 75, 40);
        var options = ["CONTINUE", "MAIN MENU", "QUIT"];
        setTextSize(1);
        for (var i = 0; i < options.length; i++) {
            if (i === pauseMenuSelection) {
                setTextColor(YELLOW);
                drawFillRect(70, 60 + i * 15, 100, 12, GRAY);
                drawString("> " + options[i], 75, 65 + i * 15);
            } else {
                setTextColor(WHITE);
                drawString(" " + options[i], 75, 65 + i * 15);
            }
        }

        setTextColor(CYAN);
        drawString("Developed by msi", WIDTH - 95, HEIGHT - 10);

        staticDrawn = true;
    }
}
function drawExitConfirm() {
    fillScreen(BLACK);
    setTextSize(2);
    setTextColor(YELLOW);
    drawString("EXIT?", 90, 40);
    setTextSize(1);
    var options = ["YES", "NO"];
    for (var i = 0; i < options.length; i++) {
        if (i === exitConfirmSelection) {
            setTextColor(GREEN);
            var highlightWidth = 40;
            var highlightHeight = 15;
            drawFillRect(70 + i * 60, 80, highlightWidth, highlightHeight, GRAY);
            drawString("> " + options[i], 75 + i * 60, 85);
        } else {
            setTextColor(WHITE);
            drawString(" " + options[i], 75 + i * 60, 85);
        }
    }

    setTextColor(CYAN);
    drawString("Developed by msi", WIDTH - 95, HEIGHT - 10);

    staticDrawn = true;
}
var PADDLE_WIDTH = 40;
var PADDLE_HEIGHT = 6;
var BALL_SIZE = 5;
var BRICK_WIDTH = 20;
var BRICK_HEIGHT = 10;
var BRICK_MARGIN = 2;
var BRICK_ROWS = 5;
var BRICK_COLS = 10;
var BRICK_COLORS = [YELLOW, ORANGE, YELLOW, GREEN, BLUE, PURPLE];
var BREAKOUT_STATE_START = 0;
var BREAKOUT_STATE_PLAYING = 1;
var BREAKOUT_STATE_GAME_OVER = 2;
var BREAKOUT_STATE_WIN = 3;
var BREAKOUT_STATE_NEXT_LEVEL = 4;
var breakoutState = BREAKOUT_STATE_START;
var paddle = { x: WIDTH / 2 - PADDLE_WIDTH / 2, y: HEIGHT - 15, width: PADDLE_WIDTH, height: PADDLE_HEIGHT, speed: 12, color: WHITE, lastX: WIDTH / 2 - PADDLE_WIDTH / 2, lastY: HEIGHT - 15 };
var ball = { x: WIDTH / 2, y: HEIGHT - 20 - BALL_SIZE, size: BALL_SIZE, speedX: 0, speedY: 0, color: WHITE, stuck: true, lastX: WIDTH / 2, lastY: HEIGHT - 20 - BALL_SIZE };
var bricks = [];
var breakoutScore = 0;
var breakoutLives = 3;
var breakoutLevel = 1;
var breakoutStaticDrawn = false;
var breakoutLastStaticDrawnState = -1;
var breakoutSelPressCount = 0;
var breakoutSelPressWindowStart = -1;
var breakoutSelPressWindow = 400;
var breakoutSelPressThreshold = 4;
var breakoutLastSelState = false;
var breakoutIsPaused = false;
var breakoutPauseCooldown = 0;
var lastScoreDrawn = -1;
var lastLivesDrawn = -1;
function resetBreakout() {
    paddle.x = WIDTH / 2 - PADDLE_WIDTH / 2;
    ball.x = paddle.x + paddle.width / 2;
    ball.y = paddle.y - ball.size;
    ball.stuck = true;
    resetBall();
    if (breakoutState === BREAKOUT_STATE_START || breakoutState === BREAKOUT_STATE_GAME_OVER) {
        breakoutScore = 0;
        breakoutLives = 3;
        breakoutLevel = 1;
    }
    createBricks();
    breakoutState = BREAKOUT_STATE_PLAYING;
    breakoutPauseCooldown = 0;
    breakoutStaticDrawn = false;
    breakoutLastStaticDrawnState = -1;
}
function resetBall() {
    ball.speedX = (Math.random() * 2 - 1) * 2;
    ball.speedY = -3;
}
function createBricks() {
    bricks = [];
    for (var i = 0; i < BRICK_ROWS; i++) {
        for (var j = 0; j < BRICK_COLS; j++) {
            var colorIndex = i % BRICK_COLORS.length;
            var strength = 1;
            if (Math.random() < 0.1 * breakoutLevel) strength = 2;
            bricks.push({ x: j * (BRICK_WIDTH + BRICK_MARGIN) + 10, y: i * (BRICK_HEIGHT + BRICK_MARGIN) + 20, width: BRICK_WIDTH, height: BRICK_HEIGHT, color: BRICK_COLORS[colorIndex], strength: strength, hit: false, changed: true });
        }
    }
}
function breakoutNextLevel() {
    breakoutLevel++;
    paddle.width = Math.max(PADDLE_WIDTH - (breakoutLevel - 1) * 3, 20);
    resetBall();
    ball.stuck = true;
    breakoutState = BREAKOUT_STATE_NEXT_LEVEL;
    breakoutStaticDrawn = false;
    breakoutLastStaticDrawnState = -1;
    audio.tone(700, 200);
    audio.tone(900, 200);
}
function drawBreakout() {
    switch (breakoutState) {
        case BREAKOUT_STATE_START: drawBreakoutStartScreen(); break;
        case BREAKOUT_STATE_PLAYING:
            if (!breakoutIsPaused) {
                drawBreakoutPlayScreen();
            }
            break;
        case BREAKOUT_STATE_GAME_OVER: drawBreakoutGameOverScreen(); break;
        case BREAKOUT_STATE_WIN: drawBreakoutWinScreen(); break;
        case BREAKOUT_STATE_NEXT_LEVEL: drawBreakoutNextLevelScreen(); break;
    }
}
function drawBreakoutStartScreen() {
    if (!breakoutStaticDrawn || breakoutState !== breakoutLastStaticDrawnState) {
        fillScreen(BLACK);
        setTextSize(2);
        setTextColor(YELLOW);
        drawString("BREAKOUT", WIDTH / 2 - 48, 10);
        setTextSize(1);
        setTextColor(WHITE);
        drawString("PREV: Go Left", WIDTH / 2 - 39, 45);
        drawString("NEXT: Go Right", WIDTH / 2 - 42, 61);
        drawString("M5 (Press): Release Ball", WIDTH / 2 - 69, 77);
        drawString("M5 (Hold): Pause", WIDTH / 2 - 48, 93);
        setTextColor(GREEN);
        drawString("Press M5 to Begin", WIDTH / 2 - 51, 125);
        breakoutStaticDrawn = true;
        breakoutLastStaticDrawnState = breakoutState;
    }
}
function drawBreakoutPlayScreen() {
    if (!breakoutStaticDrawn || breakoutState !== breakoutLastStaticDrawnState ||
        breakoutScore !== lastScoreDrawn || breakoutLives !== lastLivesDrawn) {
        if (breakoutScore !== lastScoreDrawn || breakoutLives !== lastLivesDrawn) {
            drawFillRect(0, 0, WIDTH, 20, BLACK);
            setTextSize(1);
            setTextColor(WHITE);
            drawString("Score: " + breakoutScore, 10, 5);
            drawString("Level: " + breakoutLevel, 100, 5);
            drawString("Lives:", 180, 5);
            for (var i = 0; i < breakoutLives; i++) drawFillRect(215 + i * 8, 5, 5, 5, GREEN);
            lastScoreDrawn = breakoutScore;
            lastLivesDrawn = breakoutLives;
        } else {
            fillScreen(BLACK);
            setTextSize(1);
            setTextColor(WHITE);
            drawString("Score: " + breakoutScore, 10, 5);
            drawString("Level: " + breakoutLevel, 100, 5);
            drawString("Lives:", 180, 5);
            for (var i = 0; i < breakoutLives; i++) drawFillRect(215 + i * 8, 5, 5, 5, GREEN);
            for (var i = 0; i < bricks.length; i++) {
                if (!bricks[i].hit) {
                    drawFillRect(bricks[i].x, bricks[i].y, bricks[i].width, bricks[i].height, bricks[i].color);
                    if (bricks[i].strength > 1) drawRect(bricks[i].x + 2, bricks[i].y + 2, bricks[i].width - 4, bricks[i].height - 4, WHITE);
                }
            }
            breakoutStaticDrawn = true;
        }
        breakoutLastStaticDrawnState = breakoutState;
    }
    for (var i = 0; i < bricks.length; i++) {
        if (bricks[i].hit && bricks[i].changed) {
            drawFillRect(bricks[i].x, bricks[i].y, BRICK_WIDTH, BRICK_HEIGHT, BLACK);
            bricks[i].changed = false;
        }
    }
    drawFillRect(paddle.lastX, paddle.lastY, paddle.width, paddle.height, BLACK);
    drawFillRect(paddle.x, paddle.y, paddle.width, paddle.height, paddle.color);
    paddle.lastX = paddle.x;
    paddle.lastY = paddle.y;

    drawFillRect(ball.lastX - ball.size / 2, ball.lastY - ball.size / 2, ball.size, ball.size, BLACK);
    if (ball.y - ball.size / 2 < 20) ball.y = 20 + ball.size / 2;
    drawFillRect(ball.x - ball.size / 2, ball.y - ball.size / 2, ball.size, ball.size, ball.color);
    ball.lastX = ball.x;
    ball.lastY = ball.y;
}
function drawBreakoutGameOverScreen() {
    if (!breakoutStaticDrawn || breakoutState !== breakoutLastStaticDrawnState) {
        fillScreen(BLACK);
        setTextSize(2);
        setTextColor(WHITE);
        drawString("GAME OVER", WIDTH / 2 - 54, 40);
        setTextSize(1);
        setTextColor(WHITE);
        drawString("Score: " + breakoutScore, WIDTH / 2 - 24, 80);
        setTextColor(YELLOW);
        drawString("M5 to Play Again", 72, 100);
        drawString("PREV to Menu", 84, 115);
        breakoutStaticDrawn = true;
        breakoutLastStaticDrawnState = breakoutState;
        audio.tone(400, 300);
        audio.tone(300, 300);
    }
}
function drawBreakoutWinScreen() {
    if (!breakoutStaticDrawn || breakoutState !== breakoutLastStaticDrawnState) {
        fillScreen(BLACK);
        setTextSize(2);
        setTextColor(GREEN);
        drawString("YOU WIN!", WIDTH / 2 - 48, 40);
        setTextSize(1);
        setTextColor(WHITE);
        drawString("Score: " + breakoutScore, WIDTH / 2 - 24, 80);
        setTextColor(YELLOW);
        drawString("M5 to Play Again", WIDTH / 2 - 66, 100);
        drawString("PREV to Menu", WIDTH / 2 - 30, 115);
        breakoutStaticDrawn = true;
        breakoutLastStaticDrawnState = breakoutState;
        audio.tone(800, 200);
        audio.tone(1000, 200);
    }
}
function drawBreakoutNextLevelScreen() {
    if (!breakoutStaticDrawn || breakoutState !== breakoutLastStaticDrawnState) {
        fillScreen(BLACK);
        setTextSize(2);
        setTextColor(GREEN);
        drawString("LEVEL UP!", WIDTH / 2 - 48, 40);
        setTextSize(1);
        setTextColor(WHITE);
        drawString("Score: " + breakoutScore, WIDTH / 2 - 24, 80);
        setTextColor(YELLOW);
        drawString("M5 to Continue", WIDTH / 2 - 66, 100);
        breakoutStaticDrawn = true;
        breakoutLastStaticDrawnState = breakoutState;
    }
}
function updateBreakout() {
    if (breakoutState !== BREAKOUT_STATE_PLAYING || breakoutIsPaused) return;
    if (getPrevPress()) paddle.x -= paddle.speed;
    else if (getNextPress()) paddle.x += paddle.speed;
    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + paddle.width > WIDTH) paddle.x = WIDTH - paddle.width;
    if (ball.stuck) {
        ball.x = paddle.x + paddle.width / 2;
        ball.y = paddle.y - ball.size;
        return;
    }
    ball.x += ball.speedX;
    ball.y += ball.speedY;
    if (ball.x - ball.size / 2 < 0 || ball.x + ball.size / 2 > WIDTH) {
        ball.speedX = -ball.speedX;
        audio.tone(500, 100);
    }
    if (ball.y - ball.size / 2 < 0) {
        ball.speedY = -ball.speedY;
        audio.tone(500, 100);
    }
    if (ball.y - ball.size / 2 < 20) {
        ball.y = 20 + ball.size / 2;
        ball.speedY = Math.abs(ball.speedY);
        audio.tone(500, 100);
    }
    if (ball.y + ball.size / 2 > HEIGHT) {
        breakoutLives--;
        audio.tone(200, 300);
        if (breakoutLives <= 0) breakoutState = BREAKOUT_STATE_GAME_OVER;
        else ball.stuck = true;
        return;
    }
    if (ball.y + ball.size / 2 > paddle.y - 2 && ball.y - ball.size / 2 < paddle.y + paddle.height && ball.x + ball.size / 2 > paddle.x && ball.x - ball.size / 2 < paddle.x + paddle.width) {
        var hitPos = (ball.x - (paddle.x + paddle.width / 2)) / (paddle.width / 2);
        ball.speedX = hitPos * 4;
        ball.speedY = -Math.abs(ball.speedY) * 1.05;
        audio.tone(600, 150);
    }
    for (var i = 0; i < bricks.length; i++) {
        if (!bricks[i].hit && ball.x + ball.size / 2 > bricks[i].x && ball.x - ball.size / 2 < bricks[i].x + bricks[i].width && ball.y + ball.size / 2 > bricks[i].y && ball.y - ball.size / 2 < bricks[i].y + bricks[i].height) {
            bricks[i].strength--;
            bricks[i].changed = true;
            if (bricks[i].strength <= 0) {
                bricks[i].hit = true;
                breakoutScore += 10 * breakoutLevel;
                audio.tone(700, 100);
            } else {
                breakoutScore += 5;
                audio.tone(650, 100);
            }
            var overlapLeft = ball.x + ball.size / 2 - bricks[i].x;
            var overlapRight = bricks[i].x + bricks[i].width - (ball.x - ball.size / 2);
            var overlapTop = ball.y + ball.size / 2 - bricks[i].y;
            var overlapBottom = bricks[i].y + bricks[i].height - (ball.y - ball.size / 2);
            var minOverlap = Math.min(overlapLeft, overlapRight, overlapTop, overlapBottom);
            if (minOverlap === overlapLeft || minOverlap === overlapRight) ball.speedX = -ball.speedX;
            else ball.speedY = -ball.speedY;
            break;
        }
    }
    var remainingBricks = 0;
    for (var i = 0; i < bricks.length; i++) if (!bricks[i].hit) remainingBricks++;
    if (remainingBricks === 0) {
        if (breakoutLevel < 5) breakoutNextLevel();
        else breakoutState = BREAKOUT_STATE_WIN;
    }
}
var GRID_SIZE = 10;
var HUD_HEIGHT = 20;
var COLS = Math.floor(WIDTH / GRID_SIZE);
var ROWS = Math.floor((HEIGHT - HUD_HEIGHT) / GRID_SIZE);
var cApple = 10079232;
var SNAKE_STATE_MENU = 0;
var SNAKE_STATE_GAME = 1;
var SNAKE_STATE_PAUSED = 2;
var SNAKE_STATE_GAME_OVER = 3;
var snakeState = SNAKE_STATE_MENU;
var snake = [];
var direction = 0;
var nextDirection = 0;
var food = { x: 8, y: 4 };
var snakeScore = 0;
var snakeHighScore = 0;
var snakeFrameCounter = 0;
var snakeSpeed = 10;
var snakeSpeedIncrease = true;
var snakeStaticDrawn = false;
var snakeLastStaticDrawnState = -1;
var snakeLastSelState = false;
var snakeTotalDelay = 300;
var snakeDelayTime = snakeTotalDelay;
var snakeCanMove = true;
var snakeTime = now();
var snakePrevTime = now();
var snakeErasePos = null;
var snakeLastScore = -1;
var snakeLastHighScore = -1;
var snakeForceHudRedraw = false;
function resetSnake() {
    snake = [
        { x: 4, y: 4 },
        { x: 3, y: 4 },
        { x: 2, y: 4 },
        { x: 1, y: 4 }
    ];
    direction = 3;
    nextDirection = 3;
    snakeScore = 0;
    snakeFrameCounter = 0;
    placeFood();
    snakeState = SNAKE_STATE_GAME;
    snakeStaticDrawn = false;
    snakeLastStaticDrawnState = -1;
    snakeDelayTime = snakeTotalDelay;
    snakeErasePos = null;
    snakeLastScore = -1;
    snakeLastHighScore = -1;
    snakeForceHudRedraw = true;
}
function placeFood() {
    var validPos = false;
    var maxAttempts = 100;
    var attempts = 0;


    var minX = 1;
    var maxX = COLS - 2;
    var minY = 1;
    var maxY = ROWS - 2;

    while (!validPos && attempts < maxAttempts) {
        attempts++;
        var newX = Math.floor(Math.random() * (maxX - minX)) + minX;
        var newY = Math.floor(Math.random() * (maxY - minY)) + minY;

        validPos = true;


        for (var i = 0; i < snake.length; i++) {
            if (snake[i].x === newX && snake[i].y === newY) {
                validPos = false;
                break;
            }
        }

        if (validPos) {
            food.x = newX;
            food.y = newY;
            break;
        }
    }


    if (!validPos) {
        food.x = 10;
        food.y = 10;
    }
}
function drawSnake() {
    switch (snakeState) {
        case SNAKE_STATE_MENU:
            drawSnakeMenu();
            break;
        case SNAKE_STATE_GAME:
            if (!snakeStaticDrawn || snakeState !== snakeLastStaticDrawnState) {
                fillScreen(BLACK);
                snakeStaticDrawn = true;
                snakeLastStaticDrawnState = snakeState;
                snakeForceHudRedraw = true;
            }

            if (snakeErasePos) {
                drawFillRect(snakeErasePos.x * GRID_SIZE, snakeErasePos.y * GRID_SIZE + HUD_HEIGHT, GRID_SIZE, GRID_SIZE, BLACK);
                snakeErasePos = null;
            }

            if (snake.length > 0) {
                drawFillRect(snake[0].x * GRID_SIZE, snake[0].y * GRID_SIZE + HUD_HEIGHT, GRID_SIZE, GRID_SIZE, YELLOW);
                for (var i = 1; i < snake.length; i++) {
                    drawFillRect(snake[i].x * GRID_SIZE, snake[i].y * GRID_SIZE + HUD_HEIGHT, GRID_SIZE, GRID_SIZE, GREEN);
                }
            }

            drawFillRect(food.x * GRID_SIZE, food.y * GRID_SIZE + HUD_HEIGHT, GRID_SIZE, GRID_SIZE, cApple);

            if (snakeScore !== snakeLastScore || snakeHighScore !== snakeLastHighScore || snakeForceHudRedraw) {
                drawSnakeScore();
                snakeLastScore = snakeScore;
                snakeLastHighScore = snakeHighScore;
                snakeForceHudRedraw = false;
            }
            break;
        case SNAKE_STATE_PAUSED:
            break;
        case SNAKE_STATE_GAME_OVER:
            break;
    }
}
function drawSnakeMenu() {
    if (!snakeStaticDrawn || snakeState !== snakeLastStaticDrawnState) {
        fillScreen(BLACK);
        setTextSize(3);
        setTextColor(GREEN);
        drawString("SNAKE", 75, 10);
        setTextSize(1);
        setTextColor(WHITE);
        drawString("PRESS M5 TO START", 66, 55);
        drawString("PREV: TURN LEFT", 75, 80);
        drawString("NEXT: TURN RIGHT", 72, 95);
        drawString("M5: PAUSE", 90, 110);
        snakeStaticDrawn = true;
        snakeLastStaticDrawnState = snakeState;
    }
}
function drawSnakeScore() {
    drawFillRect(0, 0, WIDTH, HUD_HEIGHT, BLACK);
    setTextSize(1);
    setTextColor(WHITE);
    drawString("SCORE: " + snakeScore, 5, 5);
    drawString("HIGH SCORE: " + snakeHighScore, 150, 5);
}
function updateSnakeDelayTime() {
    var timePassed = snakeTime - snakePrevTime;
    if (timePassed == 0) {
        snakeTime = now();
        return;
    }
    snakePrevTime = snakeTime;
    snakeTime = now();
    snakeDelayTime -= timePassed;
}
function updateSnake() {
    if (snakeState !== SNAKE_STATE_GAME || snake.length === 0) return;
    updateSnakeDelayTime();
    if (snakeDelayTime < 0) {
        snakeCanMove = true;

        var newHeadX = snake[0].x;
        var newHeadY = snake[0].y;

        switch (direction) {
            case 0: newHeadY -= 1; break;
            case 1: newHeadY += 1; break;
            case 2: newHeadX -= 1; break;
            case 3: newHeadX += 1; break;
        }

        if (newHeadX < 0 || newHeadX >= COLS || newHeadY < 0 || newHeadY >= ROWS) {
            snakeGameOver();
            return;
        }

        for (var i = 0; i < snake.length; i++) {
            if (newHeadX === snake[i].x && newHeadY === snake[i].y) {
                snakeGameOver();
                return;
            }
        }

        snake.unshift({ x: newHeadX, y: newHeadY });

        if (newHeadX === food.x && newHeadY === food.y) {
            snakeScore += 10;
            if (snakeScore > snakeHighScore) snakeHighScore = snakeScore;

            placeFood();

            audio.tone(600, 150);
        } else {
            var tail = snake.pop();
            snakeErasePos = { x: tail.x, y: tail.y };
        }

        direction = nextDirection;
        snakeDelayTime = snakeTotalDelay;
    }
}
function snakeGameOver() {
    snakeState = SNAKE_STATE_GAME_OVER;
    gameState = STATE_GAME_OVER;
    prevGameState = STATE_SNAKE;
    staticDrawn = false;
    snakeStaticDrawn = false;
    if (snakeScore > snakeHighScore) snakeHighScore = snakeScore;
}
var PLAYER_SIZE = 16;
var ENEMY_SIZE = 14;
var BULLET_SIZE = 5;
var POWERUP_SIZE = 10;
var EXPLOSION_MAX_SIZE = 22;
var SPACE_STATE_MENU = 0;
var SPACE_STATE_GAME = 1;
var SPACE_STATE_GAME_OVER = 2;
var SPACE_STATE_LEVEL_UP = 3;
var spaceState = SPACE_STATE_MENU;
var player = { x: WIDTH / 2, y: HEIGHT - 25, width: PLAYER_SIZE, height: PLAYER_SIZE, speed: 10, lives: 3, weaponLevel: 1, weaponTime: 0, invincible: false, invincibleTime: 0, lastX: WIDTH / 2, lastY: HEIGHT - 25 };
var bullets = [];
var enemies = [];
var enemyBullets = [];
var explosions = [];
var stars = [];
var powerups = [];
var spaceScore = 0;
var spaceHighScore = 0;
var spaceLevel = 1;
var spaceFrameCounter = 0;
var enemySpawnRate = 50;
var enemyShootRate = 100;
var bossActive = false;
var boss = null;
var killCount = 0;
var levelUpThreshold = 20;
var fireRate = 6;
var lastFireTime = 0;
var spaceLastSelState = false;
var spaceIsPaused = false;
var spaceStaticDrawn = false;
var spaceLastStaticDrawnState = -1;
function resetSpaceShooter() {
    player.x = WIDTH / 2;
    player.y = HEIGHT - 25;
    player.lastX = player.x;
    player.lastY = player.y;
    player.lives = 3;
    player.weaponLevel = 1;
    player.weaponTime = 0;
    player.invincible = false;
    player.invincibleTime = 0;
    bullets = [];
    enemies = [];
    enemyBullets = [];
    explosions = [];
    powerups = [];
    boss = null;
    spaceScore = 0;
    spaceLevel = 1;
    spaceFrameCounter = 0;
    enemySpawnRate = 50;
    enemyShootRate = 100;
    bossActive = false;
    killCount = 0;
    lastFireTime = 0;
    spaceLastSelState = false;
    spaceIsPaused = false;
    createStars();
    spaceState = SPACE_STATE_GAME;
    spaceStaticDrawn = false;
    spaceLastStaticDrawnState = -1;
}
function createStars() {
    stars = [];
    for (var i = 0; i < 60; i++) stars.push({ x: Math.random() * WIDTH, y: Math.random() * HEIGHT, size: Math.random() * 2.5 + 1, speed: Math.random() * 0.8 + 0.3, lastX: 0, lastY: 0 });
}
function updateStars() {
    for (var i = 0; i < stars.length; i++) {
        var star = stars[i];
        star.y += star.speed * 0.1;
        if (star.y > HEIGHT) star.y -= HEIGHT;
    }
}
function drawStars() {
    for (var i = 0; i < stars.length; i++) {
        var star = stars[i];
        drawFillRect(star.lastX - 1, star.lastY - 1, star.size + 2, star.size + 2, BLACK);
        var brightness = (spaceFrameCounter + i * 10) % 100;
        var color = brightness < 50 ? WHITE : CYAN;
        if (i % 7 === 0) color = YELLOW;
        if (i % 11 === 0) color = MAGENTA;
        drawFillRect(star.x, star.y, star.size, star.size, color);
        star.lastX = star.x;
        star.lastY = star.y;
    }
}
function drawSpaceShooter() {
    switch (spaceState) {
        case SPACE_STATE_MENU: drawSpaceMenu(); break;
        case SPACE_STATE_GAME:
            if (!spaceIsPaused) drawSpaceGameplay();
            break;
        case SPACE_STATE_GAME_OVER: break;
        case SPACE_STATE_LEVEL_UP: break;
    }
}
function drawSpaceMenu() {
    if (!spaceStaticDrawn || spaceState !== spaceLastStaticDrawnState) {
        fillScreen(BLACK);
        setTextSize(3);
        setTextColor(CYAN);
        drawString("SPACE", 75, 15);
        setTextColor(ORANGE);
        drawString("SHOOTER", 60, 40);
        setTextSize(1);
        setTextColor(WHITE);
        drawString("PREV: GO LEFT", 10, 100);
        drawString("NEXT: GO RIGHT", 140, 100);
        drawString("M5: PAUSE", 105, 120);
        if (spaceFrameCounter % 30 < 20) {
            setTextColor(WHITE);
            drawString("PRESS M5 TO START", 70, 75);
        }
        spaceStaticDrawn = true;
        spaceLastStaticDrawnState = spaceState;
    }
}
function drawSpaceGameplay() {
    if (!spaceStaticDrawn || spaceState !== spaceLastStaticDrawnState) {
        fillScreen(BLACK);
        spaceStaticDrawn = true;
        spaceLastStaticDrawnState = spaceState;
    }
    if (spaceFrameCounter % 10 === 0) {
        updateStars();
        drawStars();
    }
    drawEnemies();
    drawBullets();
    drawEnemyBullets();
    drawExplosions();
    drawPowerups();
    if (player.x !== player.lastX || player.y !== player.lastY) {
        drawFillRect(player.lastX - player.width / 2 - 6, player.lastY - 5, player.width + 12, player.height + 15, BLACK);
        player.lastX = player.x;
        player.lastY = player.y;
    }
    drawPlayer(player.x, player.y, player.weaponLevel);
    drawSpaceHUD();
}
function drawPlayer(x, y, weaponLevel) {
    drawFillRect(x - player.width / 2, y, player.width, player.height, BLUE);
    drawFillRect(x - player.width / 2 + 3, y + 3, player.width - 6, player.height - 8, CYAN);
    drawFillRect(x - 4, y - 5, 8, 5, WHITE);
    if (spaceFrameCounter % 6 < 3) {
        drawFillRect(x - 6, y + player.height, 12, 4, YELLOW);
        drawFillRect(x - 4, y + player.height + 4, 8, 3, YELLOW);
    } else {
        drawFillRect(x - 5, y + player.height, 10, 5, YELLOW);
        drawFillRect(x - 3, y + player.height + 3, 6, 2, YELLOW);
    }
    if (weaponLevel > 1) {
        drawFillRect(x - player.width / 2 - 4, y + 3, 4, 4, YELLOW);
        drawFillRect(x + player.width / 2, y + 3, 4, 4, YELLOW);
        drawFillRect(x - player.width / 2 - 2, y + 5, 2, 6, YELLOW);
        drawFillRect(x + player.width / 2 + 1, y + 5, 2, 6, YELLOW);
    }
    if (weaponLevel > 2) {
        drawFillRect(x - player.width / 2 - 4, y + 10, 4, 4, YELLOW);
        drawFillRect(x + player.width / 2, y + 10, 4, 4, YELLOW);
    }
    if (player.invincible && spaceFrameCounter % 6 < 3) drawRect(x - player.width / 2 - 2, y - 2, player.width + 4, player.height + 4, WHITE);
}
function drawEnemies() {
    for (var i = 0; i < enemies.length; i++) {
        var enemy = enemies[i];
        if (enemy && enemy.active) {
            drawFillRect(enemy.lastX - 5, enemy.lastY - 5, enemy.width + 10, enemy.height + 10, BLACK);
            drawFillRect(enemy.x, enemy.y, enemy.width, enemy.height, enemy.type.color);
            enemy.lastX = enemy.x;
            enemy.lastY = enemy.y;
        }
    }
    if (bossActive && boss) {
        drawFillRect(boss.lastX - 5, boss.lastY - 5, boss.width + 10, boss.height + 10, BLACK);
        drawFillRect(boss.x, boss.y, boss.width, boss.height, MAGENTA);
        boss.lastX = boss.x;
        boss.lastY = boss.y;
    }
}
function drawBullets() {
    for (var i = 0; i < bullets.length; i++) {
        var bullet = bullets[i];
        if (bullet && bullet.active) {
            if (!bullet.lastX) bullet.lastX = bullet.x;
            if (!bullet.lastY) bullet.lastY = bullet.y;
            if (bullet.x !== bullet.lastX || bullet.y !== bullet.lastY) {
                drawFillRect(bullet.lastX - bullet.width / 2, bullet.lastY, bullet.width, bullet.height + 2, BLACK);
                drawFillRect(bullet.x - bullet.width / 2, bullet.y, bullet.width, bullet.height, GREEN);
                bullet.lastX = bullet.x;
                bullet.lastY = bullet.y;
            }
        }
    }
}
function drawEnemyBullets() {
    for (var i = 0; i < enemyBullets.length; i++) {
        var bullet = enemyBullets[i];
        if (bullet && bullet.active) {
            if (!bullet.lastX) bullet.lastX = bullet.x;
            if (!bullet.lastY) bullet.lastY = bullet.y;
            if (bullet.x !== bullet.lastX || bullet.y !== bullet.lastY) {
                drawFillRect(bullet.lastX - bullet.width / 2, bullet.lastY, bullet.width, bullet.height, BLACK);
                drawFillRect(bullet.x - bullet.width / 2, bullet.y, bullet.width, bullet.height, YELLOW);
                bullet.lastX = bullet.x;
                bullet.lastY = bullet.y;
            }
        }
    }
}
function drawExplosions() {
    for (var i = 0; i < explosions.length; i++) {
        var explosion = explosions[i];
        if (explosion && explosion.active) drawFillRect(explosion.x - explosion.size / 2, explosion.y - explosion.size / 2, explosion.size, explosion.size, YELLOW);
    }
}
function drawPowerups() {
    for (var i = 0; i < powerups.length; i++) {
        var powerup = powerups[i];
        if (powerup && powerup.active) {
            if (!powerup.lastX) powerup.lastX = powerup.x;
            if (!powerup.lastY) powerup.lastY = powerup.y;
            if (powerup.x !== powerup.lastX || powerup.y !== powerup.lastY) {
                drawFillRect(powerup.lastX - powerup.width / 2 - 2, powerup.lastY - 2, powerup.width + 4, powerup.height + 4, BLACK);
                drawFillRect(powerup.x - powerup.width / 2, powerup.y, powerup.width, powerup.height, powerup.type.color);
                powerup.lastX = powerup.x;
                powerup.lastY = powerup.y;
            }
        }
    }
}
function drawSpaceHUD() {
    drawFillRect(0, 0, WIDTH, 10, BLACK);
    setTextSize(1);
    setTextColor(WHITE);
    drawString("SCORE: " + spaceScore, 5, 5);
    drawString("LEVEL: " + spaceLevel, 100, 5);
    drawString("LIVES: " + player.lives, 180, 5);
}
function updateSpaceShooter() {
    if (spaceState !== SPACE_STATE_GAME || spaceIsPaused) return;
    spaceFrameCounter++;
    if (spaceFrameCounter - lastFireTime >= fireRate) {
        fireBullet();
        lastFireTime = spaceFrameCounter;
    }
    updateBullets();
    updateEnemies();
    updateEnemyBullets();
    updateExplosions();
    updatePowerups();
    checkCollisions();
    checkLevelProgress();
}
function updateBullets() {
    for (var i = 0; i < bullets.length; i++) {
        if (bullets[i] && bullets[i].active) {
            bullets[i].y -= bullets[i].speed;
            if (bullets[i].y + bullets[i].height < 0) {
                bullets.splice(i, 1);
                return;
            }
        }
    }
}
function updateEnemies() {
    if (spaceFrameCounter % enemySpawnRate === 0 && !bossActive) spawnEnemy();
    for (var i = 0; i < enemies.length; i++) {
        if (enemies[i] && enemies[i].active) {
            enemies[i].y += enemies[i].type.speed;
            if (enemies[i].y > HEIGHT) {
                drawFillRect(enemies[i].lastX - 5, enemies[i].lastY - 5, enemies[i].width + 10, enemies[i].height + 10, BLACK);
                enemies.splice(i, 1);
                return;
            }
            if (enemies[i].type.shootRate > 0 && spaceFrameCounter % enemies[i].type.shootRate === 0) spawnEnemyBullet(enemies[i].x + enemies[i].width / 2, enemies[i].y + enemies[i].height);
        }
    }
    if (bossActive && boss) {
        boss.y += boss.speed;
        if (boss.y + boss.height > HEIGHT) {
            boss.y = HEIGHT - boss.height;
            boss.speed = -1.5;
        } else if (boss.y < 0) {
            boss.y = 0;
            boss.speed = 1.5;
        }
        if (spaceFrameCounter % 50 === 0) spawnEnemyBullet(boss.x + boss.width / 2, boss.y + boss.height);
    }
}
function updateEnemyBullets() {
    for (var i = 0; i < enemyBullets.length; i++) {
        if (enemyBullets[i] && enemyBullets[i].active) {
            enemyBullets[i].y += enemyBullets[i].speed;
            if (enemyBullets[i].y > HEIGHT) {
                enemyBullets.splice(i, 1);
                return;
            }
        }
    }
}
function updateExplosions() {
    for (var i = 0; i < explosions.length; i++) {
        if (explosions[i] && explosions[i].active && explosions[i].life > 0) {
            explosions[i].life--;
            if (explosions[i].life <= 0) {
                drawFillRect(explosions[i].x - explosions[i].size / 2, explosions[i].y - explosions[i].size / 2, explosions[i].size, explosions[i].size, BLACK);
                explosions.splice(i, 1);
                return;
            }
        }
    }
}
function updatePowerups() {
    for (var i = 0; i < powerups.length; i++) {
        if (powerups[i] && powerups[i].active) {
            powerups[i].y += 1;
            if (powerups[i].y > HEIGHT) {
                powerups.splice(i, 1);
                return;
            }
        }
    }
}
var enemyTypes = [
    { color: BLUE, detailColor: PURPLE, health: 1, speed: 1.2, points: 10, shootRate: 0, shape: "triangle" },
    { color: LIGHTGREEN, detailColor: DARKBLUE, health: 2, speed: 1.8, points: 20, shootRate: 100, shape: "square" },
    { color: ORANGE, detailColor: MAGENTA, health: 1, speed: 2.5, points: 15, shootRate: 0, shape: "diamond" },
    { color: BLUE, detailColor: YELLOW, health: 3, speed: 0.9, points: 30, shootRate: 70, shape: "circle" }
];
var powerupTypes = [
    { type: "health", color: WHITE },
    { type: "weapon", color: YELLOW }
];
function spawnEnemy() {
    var type = enemyTypes[Math.floor(Math.random() * enemyTypes.length)];
    enemies.push({ x: Math.random() * (WIDTH - ENEMY_SIZE), y: 0, width: ENEMY_SIZE, height: ENEMY_SIZE, type: type, active: true, lastX: 0, lastY: 0 });
}
function spawnEnemyBullet(x, y) {
    enemyBullets.push({ x: x, y: y, width: BULLET_SIZE, height: BULLET_SIZE, speed: 3, active: true, lastX: 0, lastY: 0 });
}
function spawnBoss() {
    var bossWidth = Math.min(28 + (spaceLevel - 1) * 4, 56);
    var bossHeight = Math.min(28 + (spaceLevel - 1) * 4, 56);
    boss = { x: WIDTH / 2 - bossWidth / 2, y: -bossHeight, width: bossWidth, height: bossHeight, health: 25 * spaceLevel, active: true, speed: 1.5, lastX: WIDTH / 2 - bossWidth / 2, lastY: -bossHeight };
    bossActive = true;
}
function spawnPowerup(x, y) {
    if (powerups.length < 5 && Math.random() < 0.1) {
        var type = powerupTypes[Math.floor(Math.random() * powerupTypes.length)];
        powerups.push({ x: x, y: y, width: POWERUP_SIZE, height: POWERUP_SIZE, type: type, active: true, lastX: x, lastY: y });
    }
}
function fireBullet() {
    if (bullets.length >= 10) return;
    bullets.push({ x: player.x, y: player.y - player.height / 2, width: BULLET_SIZE, height: BULLET_SIZE, speed: 5, active: true, lastX: player.x, lastY: player.y - player.height / 2 });
}
function checkCollisions() {
    for (var i = 0; i < bullets.length; i++) {
        if (!bullets[i] || !bullets[i].active) continue;
        var bullet = bullets[i];
        for (var j = 0; j < enemies.length; j++) {
            if (!enemies[j] || !enemies[j].active) continue;
            var enemy = enemies[j];
            if (checkCollision(bullet.x - bullet.width / 2, bullet.y - bullet.height / 2, bullet.width, bullet.height, enemy.x, enemy.y, enemy.width, enemy.height)) {
                bullet.active = false;
                drawFillRect(bullet.lastX - bullet.width / 2, bullet.lastY, bullet.width, bullet.height + 2, BLACK);
                enemy.active = false;
                drawFillRect(enemy.lastX - 5, enemy.lastY - 5, enemy.width + 10, enemy.height + 10, BLACK);
                spaceScore += enemy.type.points;
                killCount++;
                createExplosion(enemy.x + enemy.width / 2, enemy.y + enemy.height / 2);
                spawnPowerup(enemy.x, enemy.y);
                enemies.splice(j, 1);
                bullets.splice(i, 1);
                return;
            }
        }
        if (bossActive && boss && checkCollision(bullet.x - bullet.width / 2, bullet.y - bullet.height / 2, bullet.width, bullet.height, boss.x, boss.y, boss.width, boss.height)) {
            bullet.active = false;
            drawFillRect(bullet.lastX - bullet.width / 2, bullet.lastY, bullet.width, bullet.height + 2, BLACK);
            boss.health--;
            if (boss.health <= 0) {
                bossActive = false;
                spaceScore += 500;
                killCount += 5;
                createExplosion(boss.x + boss.width / 2, boss.y + boss.height / 2);
                spaceLevelUp();
            }
            bullets.splice(i, 1);
            return;
        }
    }
    if (!player.invincible) {
        for (var j = 0; j < enemies.length; j++) {
            if (!enemies[j] || !enemies[j].active) continue;
            var enemy = enemies[j];
            if (checkCollision(player.x - player.width / 2, player.y, player.width, player.height, enemy.x, enemy.y, enemy.width, enemy.height)) {
                enemy.active = false;
                drawFillRect(enemy.lastX - 5, enemy.lastY - 5, enemy.width + 10, enemy.height + 10, BLACK);
                player.lives--;
                createExplosion(player.x, player.y + player.height, true);
                player.invincible = true;
                player.invincibleTime = 60;
                audio.tone(500, 200);
                if (player.lives <= 0) {
                    spaceState = SPACE_STATE_GAME_OVER;
                    gameState = STATE_GAME_OVER;
                    prevGameState = STATE_SPACE_SHOOTER;
                    staticDrawn = false;
                    if (spaceScore > spaceHighScore) spaceHighScore = spaceScore;
                    spaceStaticDrawn = false;
                }
                enemies.splice(j, 1);
                return;
            }
        }
        for (var j = 0; j < enemyBullets.length; j++) {
            if (!enemyBullets[j] || !enemyBullets[j].active) continue;
            var bullet = enemyBullets[j];
            if (checkCollision(player.x - player.width / 2, player.y, player.width, player.height, bullet.x - bullet.width / 2, bullet.y, bullet.width, bullet.height)) {
                bullet.active = false;
                player.lives--;
                createExplosion(player.x, player.y + player.height, true);
                player.invincible = true;
                player.invincibleTime = 60;
                audio.tone(500, 200);
                if (player.lives <= 0) {
                    spaceState = SPACE_STATE_GAME_OVER;
                    gameState = STATE_GAME_OVER;
                    prevGameState = STATE_SPACE_SHOOTER;
                    staticDrawn = false;
                    if (spaceScore > spaceHighScore) spaceHighScore = spaceScore;
                    spaceStaticDrawn = false;
                }
                bullets.splice(i, 1);
                break;
            }
        }
        if (bossActive && boss && checkCollision(player.x - player.width / 2, player.y, player.width, player.height, boss.x, boss.y, boss.width, boss.height)) {
            player.lives--;
            createExplosion(player.x, player.y + player.height, true);
            player.invincible = true;
            player.invincibleTime = 60;
            audio.tone(500, 200);
            if (player.lives <= 0) {
                spaceState = SPACE_STATE_GAME_OVER;
                gameState = STATE_GAME_OVER;
                prevGameState = STATE_SPACE_SHOOTER;
                staticDrawn = false;
                if (spaceScore > spaceHighScore) spaceHighScore = spaceScore;
                spaceStaticDrawn = false;
            }
        }
    }
    for (var i = 0; i < powerups.length; i++) {
        if (!powerups[i] || !powerups[i].active) continue;
        var powerup = powerups[i];
        if (checkCollision(player.x - player.width / 2, player.y, player.width, player.height, powerup.x - powerup.width / 2, powerup.y, powerup.width, powerup.height)) {
            powerup.active = false;
            drawFillRect(powerup.lastX - powerup.width / 2 - 2, powerup.lastY - 2, powerup.width + 4, powerup.height + 4, BLACK);
            if (powerup.type.type === "health" && player.lives < 3) player.lives++;
            else if (powerup.type.type === "weapon" && player.weaponLevel < 3) {
                player.weaponLevel++;
                player.weaponTime = 300;
            }
            powerups.splice(i, 1);
            return;
        }
    }
    if (player.invincible) {
        player.invincibleTime--;
        if (player.invincibleTime <= 0) player.invincible = false;
    }
    if (player.weaponTime > 0) {
        player.weaponTime--;
        if (player.weaponTime <= 0) player.weaponLevel = 1;
    }
}
function checkLevelProgress() {
    if (!bossActive && killCount >= levelUpThreshold) spawnBoss();
}
function spaceLevelUp() {
    spaceLevel++;
    killCount = 0;
    spaceScore += spaceLevel * 100;
    enemies = [];
    enemyBullets = [];
    enemySpawnRate = Math.max(20, 50 - spaceLevel * 5);
    enemyShootRate = Math.max(40, 100 - spaceLevel * 10);
    spaceState = SPACE_STATE_LEVEL_UP;
}
function createExplosion(x, y, isPlayerExplosion) {
    if (explosions.length < 10) {
        var explosion = { x: x, y: y, size: EXPLOSION_MAX_SIZE, active: true, life: 10 };
        if (isPlayerExplosion) explosion.size = 16;
        explosions.push(explosion);
        audio.tone(600, 150);
    }
}
function checkCollision(x1, y1, w1, h1, x2, y2, w2, h2) {
    return x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2;
}
var SLOT_STATE_MENU = 0;
var SLOT_STATE_SPIN = 1;
var SLOT_STATE_GAME_OVER = 2;
var SLOT_STATE_PAUSED = 3;
var slotState = SLOT_STATE_MENU;
var slotMoney = 300;
var slotBetOptions = [1, 2, 3, 5, 10, 20];
var slotBetIndex = 0;
var slotReels = [0, 0, 0];
var slotSymbols = ["7", "BAR", "Bell", "Chy", "Lem"];
var slotWeights = [15, 15, 20, 25, 25];
var slotStaticDrawn = false;
var slotLastSelState = false;
var slotMessage = "";
var slotMessageTimer = 0;
function resetSlots() {
    slotMoney = 500;
    slotBetIndex = 0;
    slotReels = [0, 0, 0];
    slotState = SLOT_STATE_SPIN;
    slotStaticDrawn = false;
    slotMessage = "";
    slotMessageTimer = 0;
}
function drawSlots() {
    if (!slotStaticDrawn) {
        fillScreen(BLACK);
        setTextSize(1);
        if (slotState === SLOT_STATE_MENU) {
            setTextSize(2);
            setTextColor(YELLOW);
            drawString("SLOTS", 90, 20);
            setTextSize(1);
            setTextColor(WHITE);
            drawString("M5: Bet", 100, 50);
            drawString("NEXT: Bet Change", 95, 70);
            drawString("Press M5 to Start", 80, 90);
        } else if (slotState === SLOT_STATE_SPIN) {
            setTextColor(WHITE);
            drawString("Money: " + slotMoney, 10, 10);
            drawString("PREV: Menu", 10, 30);
            drawString("Bet: " + slotBetOptions[slotBetIndex], 100, 10);
            setTextSize(2);
            setTextColor(CYAN);
            var reel1 = slotSymbols[slotReels[0]].length === 1 ? " " + slotSymbols[slotReels[0]] + " " : slotSymbols[slotReels[0]];
            var reel2 = slotSymbols[slotReels[1]].length === 1 ? " | " + slotSymbols[slotReels[1]] + " |" : " |" + slotSymbols[slotReels[1]] + "|";
            var reel3 = slotSymbols[slotReels[2]].length === 1 ? " " + slotSymbols[slotReels[2]] + " " : slotSymbols[slotReels[2]];
            var reelWidth = 45;
            var totalReelWidth = reelWidth * 3 + 30;
            var startX = (WIDTH - totalReelWidth) / 2 - 20;
            drawString(String.fromCharCode(91), startX, 60);
            drawString(reel1, startX + 10, 60);
            drawString(reel2, startX + 55, 60);
            drawString(reel3, startX + 135, 60);
            drawString(String.fromCharCode(93), startX + 180, 60);
            setTextSize(1);
            setTextColor(WHITE);
            drawString("M5: Spin", 10, 110);
            drawString("NEXT: Bet Change", 145, 110);
            if (slotMessageTimer > 0 && slotMessage !== "") {
                setTextColor(YELLOW);
                setTextSize(2);
                var centerX = 120 - 20;
                if (slotMessage === "JACKPOT!") drawString(slotMessage, centerX - 40, 110);
                else if (slotMessage === "WIN!") drawString(slotMessage, centerX - 20, 110);
                else if (slotMessage === "Pair!") drawString(slotMessage, centerX - 20, 110);
                else if (slotMessage === "So Close!") drawString(slotMessage, centerX - 40, 110);
                slotMessageTimer--;
                if (slotMessageTimer <= 0) slotMessage = "";
            }
        } else if (slotState === SLOT_STATE_GAME_OVER) {
            setTextSize(2);
            setTextColor(MAGENTA);
            drawString("BUSTED!", 80, 50);
            setTextSize(1);
            setTextColor(WHITE);
            drawString("M5 to Retry", 70, 90);
            drawString("PREV to Menu", 70, 110);
        } else if (slotState === SLOT_STATE_PAUSED) {
            drawSlotsPausedMenu();
        }
        slotStaticDrawn = true;
    }
}
function drawSlotsPausedMenu() {
    if (!slotStaticDrawn || pauseMenuSelection !== menuLastSelection) {
        fillScreen(BLACK);
        var frameX = (WIDTH - 100) / 2;
        var frameY = (HEIGHT - 60) / 2;
        drawRect(frameX, frameY, 100, 60, WHITE);
        var optionYStart = frameY + 10;
        var options = ["Resume", "Main Menu", "Exit"];
        for (var i = 0; i < 3; i++) {
            if (i === pauseMenuSelection) {
                setTextColor(YELLOW);
                drawFillRect(frameX + 5, optionYStart + i * 15, 90, 12, GRAY);
                drawString("> " + options[i], frameX + 10, optionYStart + 5 + i * 15);
            } else {
                setTextColor(WHITE);
                drawString("  " + options[i], frameX + 10, optionYStart + 5 + i * 15);
            }
        }
        slotStaticDrawn = true;
        menuLastSelection = pauseMenuSelection;
    }
}
function getWeightedRandom(weights) {
    var totalWeight = weights.reduce(function (sum, w) { return sum + w; }, 0);
    var roll = Math.random() * totalWeight;
    var cumulative = 0;
    for (var i = 0; i < weights.length; i++) {
        cumulative += weights[i];
        if (roll < cumulative) return i;
    }
    return weights.length - 1;
}
function updateSlots(selPressed) {
    if (slotState !== SLOT_STATE_SPIN) return;
    var bet = slotBetOptions[slotBetIndex];
    if (slotMoney < bet) slotBetIndex = Math.max(0, slotBetIndex - 1);
    if (selPressed) {
        slotMoney -= bet;
        slotReels = [
            getWeightedRandom(slotWeights),
            getWeightedRandom(slotWeights),
            getWeightedRandom(slotWeights)
        ];
        slotMessage = "";
        if (slotReels[0] === 0 && slotReels[1] === 0 && slotReels[2] === 0) {
            slotMoney += bet * 40;
            slotMessage = "JACKPOT!";
            slotMessageTimer = 30;
            audio.tone(1000, 500);
        } else if (slotReels[0] === slotReels[1] && slotReels[1] === slotReels[2]) {
            var multiplier;
            switch (slotReels[0]) {
                case 0: multiplier = 40; break;
                case 1: multiplier = 20; break;
                case 2: multiplier = 15; break;
                case 3: multiplier = 10; break;
                case 4: multiplier = 5; break;
            }
            slotMoney += bet * multiplier;
            slotMessage = "WIN!";
            slotMessageTimer = 20;
            audio.tone(800, 300);
        } else if (slotReels[0] === slotReels[1] || slotReels[1] === slotReels[2]) {
            slotMoney += bet * 1;
            slotMessage = "Pair!";
            slotMessageTimer = 15;
            audio.tone(600, 200);
        }
        if (slotMoney <= 0) slotState = SLOT_STATE_GAME_OVER;
        slotStaticDrawn = false;
    }
}
var BIRD_WIDTH = 16;
var BIRD_HEIGHT = 12;
var PIPE_WIDTH = 30;
var PIPE_GAP = 60;
var PIPE_SPEED = 1.5;
var GRAVITY = 0.18;
var FLAP_POWER = 3.2;
var GROUND_HEIGHT = 15;
var bird = { x: 60, y: HEIGHT / 2, width: BIRD_WIDTH, height: BIRD_HEIGHT, velocity: 0, lastY: 0 };
var pipes = [];
var flappyScore = 0;
var flappyHighScore = 0;
var pipeSpawnTimer = 0;
var groundOffset = 0;
var FLAPPY_STATE_MENU = 0;
var FLAPPY_STATE_GAME = 1;
var FLAPPY_STATE_PAUSED = 2;
var flappyState = FLAPPY_STATE_MENU;
var suppressGameOverSound = false;
var flappyStaticDrawn = false;
var flappyLastSelState = false;
function resetFlappyBird() {
    bird.y = HEIGHT / 2;
    bird.velocity = 0;
    bird.lastY = bird.y;
    pipes = [];
    flappyScore = 0;
    pipeSpawnTimer = 0;
    groundOffset = 0;
    clouds = [];
    initializeClouds();
    spawnPipe();
    flappyState = FLAPPY_STATE_MENU;
    flappyStaticDrawn = false;
    staticDrawn = false;
}
function spawnPipe() {
    var gapY = Math.random() * (HEIGHT - PIPE_GAP - GROUND_HEIGHT - 40) + 20;
    pipes.push({ x: WIDTH, y: gapY, width: PIPE_WIDTH, height: PIPE_GAP, passed: false });
}
function drawFlappyMenu() {
    if (!flappyStaticDrawn) {
        fillScreen(BLACK);
        setTextSize(3);
        setTextColor(YELLOW);
        drawString("FLAPPY BIRD", 25, 30);
        setTextSize(1);
        setTextColor(WHITE);
        drawString("PREV/NEXT: Pause", 72, 70);
        drawString("M5: Flap", 96, 85);
        setTextColor(GREEN);
        drawString("Press M5 to Start", 72, 120);
        flappyStaticDrawn = true;
    }
}
var clouds = [];

function initializeClouds() {
    clouds = [];
    for (var i = 0; i < 5; i++) {
        clouds.push({
            x: Math.random() * WIDTH,
            y: Math.random() * 40 + 5,
            width: Math.random() * 30 + 20,
            height: Math.random() * 10 + 10,
            speed: Math.random() * 0.3 + 0.1,
            lastX: 0
        });
    }
}

function updateClouds() {
    for (var i = 0; i < clouds.length; i++) {
        var cloud = clouds[i];
        cloud.x -= cloud.speed;
        if (cloud.x + cloud.width < 0) {
            cloud.x = WIDTH;
            cloud.y = Math.random() * 40 + 5;
            cloud.width = Math.random() * 30 + 20;
            cloud.height = Math.random() * 10 + 10;
        }
    }
}

function drawClouds() {
    for (var i = 0; i < clouds.length; i++) {
        var cloud = clouds[i];
        if (cloud.lastX > 0) {
            drawFillRect(cloud.lastX, cloud.y, cloud.width, cloud.height, BLACK);
        }
        drawFillRect(cloud.x, cloud.y, cloud.width, cloud.height, GRAY);
        cloud.lastX = cloud.x;
    }
}

function drawFlappyBird() {
    switch (flappyState) {
        case FLAPPY_STATE_MENU:
            drawFlappyMenu();
            break;
        case FLAPPY_STATE_GAME:
            if (!flappyStaticDrawn) {
                fillScreen(BLACK);
                drawFillRect(0, HEIGHT - GROUND_HEIGHT, WIDTH, GROUND_HEIGHT, BROWN);
                drawFillRect(0, HEIGHT - GROUND_HEIGHT, WIDTH, 3, GREEN);
                if (clouds.length === 0) {
                    initializeClouds();
                }
                flappyStaticDrawn = true;
            }

            updateClouds();
            drawClouds();

            for (var i = 0; i < pipes.length; i++) {
                var clearWidth = pipes[i].width + PIPE_SPEED + 2;
                drawFillRect(pipes[i].x, 0, clearWidth, pipes[i].y, BLACK);
                drawFillRect(pipes[i].x, pipes[i].y + pipes[i].height, clearWidth, HEIGHT - pipes[i].y - pipes[i].height - GROUND_HEIGHT, BLACK);
            }

            drawFillRect(WIDTH / 2 - 15, 20, 30, 24, BLACK);
            setTextSize(2);
            setTextColor(WHITE);
            drawString("" + flappyScore, WIDTH / 2 - 5, 30);

            for (var i = 0; i < pipes.length; i++) {
                drawFillRect(pipes[i].x, 0, pipes[i].width, pipes[i].y, GREEN);
                drawFillRect(pipes[i].x, pipes[i].y + pipes[i].height, pipes[i].width, HEIGHT - pipes[i].y - pipes[i].height - GROUND_HEIGHT, GREEN);
            }

            drawFillRect(0, HEIGHT - GROUND_HEIGHT + 5, WIDTH, 2, YELLOW);
            drawFillRect(bird.x - bird.width / 2, bird.lastY - bird.height / 2, bird.width, bird.height, BLACK);
            drawFillRect(bird.x - bird.width / 2, bird.y - bird.height / 2, bird.width, bird.height, YELLOW);
            break;
        case FLAPPY_STATE_PAUSED:
            break;
    }
}
function updateFlappyBird() {
    if (flappyState !== FLAPPY_STATE_GAME) return;
    bird.lastY = bird.y;
    bird.velocity += GRAVITY;
    bird.y += bird.velocity;
    if (bird.y < 0) {
        bird.y = 0;
        bird.velocity = 0;
    }
    if (bird.y > HEIGHT - GROUND_HEIGHT - BIRD_HEIGHT / 2) {
        prevGameState = STATE_FLAPPY_BIRD;
        gameState = STATE_GAME_OVER;
        if (!suppressGameOverSound) {
            audio.tone(400, 300); audio.tone(300, 300);
        }
        suppressGameOverSound = false;
        flappyState = FLAPPY_STATE_MENU;
        staticDrawn = false;
        if (flappyScore > flappyHighScore) flappyHighScore = flappyScore;
        return;
    }
    pipeSpawnTimer++;
    if (pipeSpawnTimer >= 100) {
        spawnPipe();
        pipeSpawnTimer = 0;
    }
    for (var i = 0; i < pipes.length; i++) {
        pipes[i].x -= PIPE_SPEED;
        if (pipes[i].x + pipes[i].width < 0) {
            pipes.splice(i, 1);
            i--;
            continue;
        }
        if (!pipes[i].passed && pipes[i].x + pipes[i].width < bird.x - BIRD_WIDTH / 2) {
            pipes[i].passed = true;
            flappyScore++;
            audio.tone(800, 100);
        }
        if (checkCollision(bird.x - BIRD_WIDTH / 2, bird.y - BIRD_HEIGHT / 2, BIRD_WIDTH, BIRD_HEIGHT,
            pipes[i].x, 0, pipes[i].width, pipes[i].y) ||
            checkCollision(bird.x - BIRD_WIDTH / 2, bird.y - BIRD_HEIGHT / 2, BIRD_WIDTH, BIRD_HEIGHT,
                pipes[i].x, pipes[i].y + pipes[i].height, pipes[i].width, HEIGHT - pipes[i].y - pipes[i].height - GROUND_HEIGHT)) {
            prevGameState = STATE_FLAPPY_BIRD;
            gameState = STATE_GAME_OVER;
            if (!suppressGameOverSound) {
                audio.tone(400, 300);
            }
            suppressGameOverSound = false;
            flappyState = FLAPPY_STATE_MENU;
            staticDrawn = false;
            if (flappyScore > flappyHighScore) flappyHighScore = flappyScore;
            return;
        }
    }
    groundOffset = (groundOffset + 1) % 20;
}
var blackjack = {
    playerMoney: 1000,
    currentBet: 0,
    deck: [],
    playerHand: [],
    dealerHand: [],
    playerBust: false,
    dealerBust: false,
    playerBlackjack: false,
    dealerBlackjack: false,
    state: 0,
    resultMessage: ""
};
var BET_OPTIONS = [10, 25, 50, 100];
var selectedBetIndex = 0;
var lastBet = 10;
var SUITS = ['H', 'D', 'S', 'C'];
var VALUES = ['A', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K'];
function resetBlackjack() {
    blackjack.playerMoney = 1000;
    blackjack.currentBet = 0;
    blackjack.deck = [];
    blackjack.playerHand = [];
    blackjack.dealerHand = [];
    blackjack.playerBust = false;
    blackjack.dealerBust = false;
    blackjack.playerBlackjack = false;
    blackjack.state = 1;
    blackjack.resultMessage = "";
    selectedBetIndex = 0;
    staticDrawn = false;
    drawBlackjack.firstDraw = true;
    drawBlackjack.lastMoney = -1;
    drawBlackjack.lastBetIndex = -1;
    drawBlackjack.lastPauseSelection = -1;
    drawBlackjack.lastState = -1;
    if (typeof blackjack.prevState === 'undefined') {
        blackjack.prevState = -1;
    }
}
function createBlackjackDeck() {
    blackjack.deck = [];
    for (var i = 0; i < SUITS.length; i++) {
        for (var j = 0; j < VALUES.length; j++) {
            blackjack.deck.push({ suit: SUITS[i], value: VALUES[j] });
        }
    }
    shuffleBlackjackDeck();
}
function shuffleBlackjackDeck() {
    for (var i = blackjack.deck.length - 1; i > 0; i--) {
        var j = Math.floor(Math.random() * (i + 1));
        var temp = blackjack.deck[i];
        blackjack.deck[i] = blackjack.deck[j];
        blackjack.deck[j] = temp;
    }
}
function drawBlackjackCard() {
    if (typeof blackjack.deck === 'undefined' || blackjack.deck.length === 0) {
        createBlackjackDeck();
    }
    return blackjack.deck.pop();
}
function calculateHandValue(hand) {
    if (!hand || hand.length === 0) return 0;
    var value = 0;
    var aces = 0;
    for (var i = 0; i < hand.length; i++) {
        var card = hand[i];
        if (card.value === 'A') {
            aces++;
            value += 11;
        } else if (card.value === 'J' || card.value === 'Q' || card.value === 'K') {
            value += 10;
        } else {
            value += parseInt(card.value);
        }
    }
    while (value > 21 && aces > 0) {
        value -= 10;
        aces--;
    }
    return value;
}
function startBlackjackGame(betAmount) {
    if (typeof blackjack.deck === 'undefined' || blackjack.deck.length === 0) {
        createBlackjackDeck();
    }
    if (blackjack.playerMoney < betAmount) return false;
    blackjack.currentBet = betAmount;
    blackjack.playerMoney -= betAmount;
    lastBet = betAmount;
    blackjack.state = 2;
    blackjack.playerHand = [drawBlackjackCard(), drawBlackjackCard()];
    blackjack.dealerHand = [drawBlackjackCard(), drawBlackjackCard()];
    blackjack.playerBlackjack = calculateHandValue(blackjack.playerHand) === 21;
    blackjack.dealerBlackjack = calculateHandValue(blackjack.dealerHand) === 21;
    if (blackjack.playerBlackjack || blackjack.dealerBlackjack) {
        dealerTurn();
    }
    staticDrawn = false;
    return true;
}
function hit() {
    if (blackjack.state !== 2) return;
    var card = drawBlackjackCard();
    if (!card) {
        createBlackjackDeck();
        card = drawBlackjackCard();
    }
    if (card) {
        blackjack.playerHand.push(card);
        var handValue = calculateHandValue(blackjack.playerHand);
        if (handValue > 21) {
            blackjack.playerBust = true;
            dealerTurn();
        } else if (handValue === 21) {
            dealerTurn();
        }
        staticDrawn = false;
    }
}
function stand() {
    if (blackjack.state !== 2) return;
    if (!blackjack.dealerHand || blackjack.dealerHand.length === 0) {
        blackjack.dealerHand = [];
    }
    dealerTurn();
    staticDrawn = false;
}
function dealerTurn() {
    if (blackjack.dealerBlackjack) {
        determineWinner();
        return;
    }
    if (!blackjack.playerBust && !blackjack.playerBlackjack) {
        if (!blackjack.dealerHand) {
            blackjack.dealerHand = [];
        }
        var dealerValue = calculateHandValue(blackjack.dealerHand);
        while (dealerValue < 17) {
            var card = drawBlackjackCard();
            if (!card) {
                createBlackjackDeck();
                card = drawBlackjackCard();
            }
            if (card) {
                blackjack.dealerHand.push(card);
                dealerValue = calculateHandValue(blackjack.dealerHand);
                staticDrawn = false;
                delay(500);
            } else {
                break;
            }
        }
        if (dealerValue > 21) blackjack.dealerBust = true;
    }
    determineWinner();
}
function determineWinner() {
    blackjack.state = 3;
    var playerValue = calculateHandValue(blackjack.playerHand);
    var dealerValue = calculateHandValue(blackjack.dealerHand || []);

    if (blackjack.playerBlackjack && blackjack.dealerBlackjack) {
        blackjack.playerMoney += blackjack.currentBet;
        blackjack.resultMessage = "Push (Blackjack vs. Blackjack)";
    } else if (blackjack.playerBlackjack) {
        blackjack.playerMoney += Math.floor(blackjack.currentBet * 2.5);
        blackjack.resultMessage = "Blackjack! You Win!";
        audio.tone(800, 300);
    } else if (blackjack.dealerBlackjack) {
        blackjack.resultMessage = "Dealer Blackjack! You Lose!";
        audio.tone(500, 200);
    } else if (blackjack.playerBust) {
        blackjack.resultMessage = "Bust! You Lose!";
        audio.tone(500, 200);
    } else if (blackjack.dealerBust || playerValue > dealerValue) {
        blackjack.playerMoney += blackjack.currentBet * 2;
        blackjack.resultMessage = "You Win!";
        audio.tone(800, 300);
    } else if (playerValue === dealerValue) {
        blackjack.playerMoney += blackjack.currentBet;
        blackjack.resultMessage = "Push!";
    } else {
        blackjack.resultMessage = "You Lose!";
        audio.tone(500, 200);
    }

    if (!blackjack.resultMessage.includes(" ")) {
        blackjack.resultMessage = blackjack.resultMessage + " Game";
    }

    staticDrawn = false;
}
function drawBlackjackPauseMenu() {
    fillScreen(BLACK);
    setTextSize(2);
    setTextColor(GOLD);
    drawString("PAUSED", 70, 20);
    drawRect(90, 40, 60, 60, GRAY);
    setTextSize(1);
    var options = ["Resume", "Main Menu", "Quit"];
    for (var i = 0; i < options.length; i++) {
        if (i === pauseMenuSelection) {
            setTextColor(YELLOW);
            drawFillRect(95, 48 + i * 15, 50, 12, GRAY);
            drawString("> " + options[i], 100, 55 + i * 15);
        } else {
            setTextColor(WHITE);
            drawString("  " + options[i], 100, 55 + i * 15);
        }
    }
}
function drawBlackjack() {
    if (!staticDrawn || blackjack.state !== drawBlackjack.lastState ||
        (blackjack.state === 2 && blackjack.playerHand.length !== drawBlackjack.lastPlayerHandLength)) {
        fillScreen(BLACK);
        staticDrawn = true;

        if (blackjack.state === 0) {
            setTextSize(2);
            setTextColor(GOLD);
            drawString("BLACKJACK", 70, 20);
            setTextSize(1);
            setTextColor(WHITE);
            drawString("Money: " + blackjack.playerMoney, 70, 50);
            drawString("M5: Play", 70, 70);
        } else if (blackjack.state === 1) {
            setTextSize(2);
            setTextColor(GOLD);
            drawString("PLACE YOUR BET", 50, 20);
            setTextSize(1);
            setTextColor(WHITE);
            drawString("Money: " + blackjack.playerMoney, 10, 50);
            for (var i = 0; i < BET_OPTIONS.length; i++) {
                if (i === selectedBetIndex) {
                    setTextColor(YELLOW);
                    drawString("> " + BET_OPTIONS[i], 100, 50 + i * 15);
                } else {
                    setTextColor(WHITE);
                    drawString("  " + BET_OPTIONS[i], 100, 50 + i * 15);
                }
            }
            setTextColor(WHITE);
            drawString("PREV/NEXT: Change", 10, 115);
            drawString("M5: Confirm", 150, 115);
        } else if (blackjack.state === 2) {
            setTextSize(1);
            setTextColor(WHITE);
            drawString("Money: " + blackjack.playerMoney, 5, 10);
            drawString("Bet: " + blackjack.currentBet, 190, 10);
            drawString("Dealer's Hand:", 5, 25);
            for (var i = 0; i < blackjack.dealerHand.length; i++) {
                if (i === 1 && !blackjack.playerBust && blackjack.dealerHand.length === 2 &&
                    calculateHandValue(blackjack.playerHand) !== 21) {
                    drawFillRect(5 + i * 30, 40, 20, 30, GRAY);
                } else if (blackjack.dealerHand[i]) {
                    drawFillRect(5 + i * 30, 40, 20, 30, WHITE);
                    setTextColor(blackjack.dealerHand[i].suit === 'H' ||
                        blackjack.dealerHand[i].suit === 'D' ? GRAY : BLACK);
                    drawString(blackjack.dealerHand[i].value + blackjack.dealerHand[i].suit, 7 + i * 30, 55);
                }
            }
            setTextColor(YELLOW);
            drawString("Your Hand: " + calculateHandValue(blackjack.playerHand), 5, 75);
            setTextColor(WHITE);
            for (var i = 0; i < blackjack.playerHand.length; i++) {
                if (blackjack.playerHand[i]) {
                    drawFillRect(5 + i * 30, 90, 20, 30, WHITE);
                    setTextColor(blackjack.playerHand[i].suit === 'H' ||
                        blackjack.playerHand[i].suit === 'D' ? GRAY : BLACK);
                    drawString(blackjack.playerHand[i].value + blackjack.playerHand[i].suit, 7 + i * 30, 105);
                }
            }
            if (!blackjack.playerBust && !blackjack.playerBlackjack) {
                setTextSize(1);
                setTextColor(WHITE);
                drawString("PREV: Menu", 15, 125);
                drawString("M5: Stand", 95, 125);
                drawString("NEXT: Hit", 170, 125);
            }
        } else if (blackjack.state === 3) {
            setTextSize(1);
            setTextColor(WHITE);
            drawString("Money: " + blackjack.playerMoney, 5, 10);
            drawString("Bet: " + blackjack.currentBet, 190, 10);
            drawString("Dealer's Hand: " + calculateHandValue(blackjack.dealerHand), 5, 25);
            for (var i = 0; i < blackjack.dealerHand.length; i++) {
                if (blackjack.dealerHand[i]) {
                    drawFillRect(5 + i * 30, 40, 20, 30, WHITE);
                    setTextColor(blackjack.dealerHand[i].suit === 'H' ||
                        blackjack.dealerHand[i].suit === 'D' ? GRAY : BLACK);
                    drawString(blackjack.dealerHand[i].value + blackjack.dealerHand[i].suit, 7 + i * 30, 55);
                }
            }
            setTextColor(YELLOW);
            drawString("Your Hand: " + calculateHandValue(blackjack.playerHand), 5, 75);
            setTextColor(WHITE);
            for (var i = 0; i < blackjack.playerHand.length; i++) {
                if (blackjack.playerHand[i]) {
                    drawFillRect(5 + i * 30, 90, 20, 30, WHITE);
                    setTextColor(blackjack.playerHand[i].suit === 'H' ||
                        blackjack.playerHand[i].suit === 'D' ? GRAY : BLACK);
                    drawString(blackjack.playerHand[i].value + blackjack.playerHand[i].suit, 7 + i * 30, 105);
                }
            }

            setTextSize(2);
            setTextColor(blackjack.resultMessage.includes("Win") ? GRAY :
                blackjack.resultMessage.includes("Lose") ? YELLOW : WHITE);

            try {
                var lines = blackjack.resultMessage.split(" ");
                if (lines.length >= 2) {
                    drawString(lines[0], 180 - (lines[0].length * 10) / 2, 60);
                    drawString(lines[1], 180 - (lines[1].length * 10) / 2, 80);
                } else {
                    drawString(blackjack.resultMessage, 180 - (blackjack.resultMessage.length * 10) / 2, 70);
                }
            } catch (e) {
                drawString("Game Over", 70, 70);
            }

            setTextSize(1);
            setTextColor(WHITE);
            var nextHandText = "M5: Next Hand";
            var nextHandX = 120 - (nextHandText.length * 5) / 2;
            drawString(nextHandText, nextHandX, 125);
        } else if (blackjack.state === 4) {
            fillScreen(BLACK);
            drawRect(60, 30, 120, 70, GRAY);
            setTextSize(1);
            var options = ["Resume", "Main Menu", "Quit"];
            for (var i = 0; i < options.length; i++) {
                if (i === pauseMenuSelection) {
                    setTextColor(YELLOW);
                    drawFillRect(95, 47 + i * 15, 50, 10, GRAY);
                    drawString("> " + options[i], 100, 50 + i * 15);
                } else {
                    setTextColor(WHITE);
                    drawString("  " + options[i], 100, 50 + i * 15);
                }
            }
        }
        drawBlackjack.lastState = blackjack.state;
        drawBlackjack.lastPlayerHandLength = blackjack.playerHand.length;
    }
}
drawBlackjack.lastState = -1;
drawBlackjack.lastPlayerHandLength = 0;

function updateBlackjack() {
    if (gameState !== STATE_BLACKJACK) return;
    if (blackjack.state === 2 && (blackjack.playerBust || blackjack.playerBlackjack || blackjack.dealerBlackjack)) {
        dealerTurn();
    }
}
function handleInput() {
    var currentSelState = getSelPress();
    var prevPressed = getPrevPress();
    var nextPressed = getNextPress();
    var shouldExit = false;
    switch (gameState) {
        case STATE_MAIN_MENU:
            if (prevPressed) {
                menuSelection = (menuSelection - 1 + menuOptions.length) % menuOptions.length;
                mainMenuStaticDrawn = false;
            }
            if (nextPressed) {
                menuSelection = (menuSelection + 1) % menuOptions.length;
                mainMenuStaticDrawn = false;
            }
            if (currentSelState && !menuLastSelState) {
                if (menuSelection === 0) gameState = STATE_BREAKOUT;
                else if (menuSelection === 1) gameState = STATE_SNAKE;
                else if (menuSelection === 2) gameState = STATE_SPACE_SHOOTER;
                else if (menuSelection === 3) { resetSlots(); gameState = STATE_SLOTS; }
                else if (menuSelection === 4) { resetFlappyBird(); gameState = STATE_FLAPPY_BIRD; }
                else if (menuSelection === 5) { resetBlackjack(); gameState = STATE_BLACKJACK; }
                else if (menuSelection === 6) { gameState = STATE_EXIT_CONFIRM; exitConfirmSelection = 1; }
                staticDrawn = false;
                mainMenuStaticDrawn = false;
                menuLastSelState = false;
                breakoutLastSelState = false;
                snakeLastSelState = false;
                spaceLastSelState = false;
                slotLastSelState = false;
            }
            menuLastSelState = currentSelState;
            break;
        case STATE_BREAKOUT:
            if (currentSelState && !breakoutLastSelState) {
                breakoutSelPressCount++;
                if (breakoutSelPressCount === 1) breakoutSelPressWindowStart = Date.now();
                if (Date.now() - breakoutSelPressWindowStart <= breakoutSelPressWindow) {
                    if (breakoutSelPressCount >= breakoutSelPressThreshold) {
                        breakoutIsPaused = true;
                        gameState = STATE_PAUSED;
                        prevGameState = STATE_BREAKOUT;
                        breakoutSelPressCount = 0;
                        breakoutSelPressWindowStart = -1;
                        breakoutPauseCooldown = 30;
                        pauseStaticDrawn = false;
                    }
                } else {
                    breakoutSelPressCount = 1;
                    breakoutSelPressWindowStart = Date.now();
                }
                if (breakoutState === BREAKOUT_STATE_START) {
                    resetBreakout();
                } else if (breakoutState === BREAKOUT_STATE_PLAYING && ball.stuck) {
                    ball.stuck = false;
                    resetBall();
                } else if (breakoutState === BREAKOUT_STATE_GAME_OVER || breakoutState === BREAKOUT_STATE_WIN) {
                    resetBreakout();
                } else if (breakoutState === BREAKOUT_STATE_NEXT_LEVEL) {
                    resetBreakout();
                }
            } else if (!currentSelState && breakoutLastSelState) {
                breakoutLastSelState = false;
            }
            if (prevPressed && (breakoutState === BREAKOUT_STATE_GAME_OVER || breakoutState === BREAKOUT_STATE_WIN)) {
                gameState = STATE_MAIN_MENU;
                staticDrawn = false;
                mainMenuStaticDrawn = false;
                menuLastSelState = false;
                breakoutLastSelState = false;
            }
            if (breakoutState === BREAKOUT_STATE_PLAYING && !breakoutIsPaused) {
                if (prevPressed) paddle.x -= paddle.speed;
                if (nextPressed) paddle.x += paddle.speed;
                if (paddle.x < 0) paddle.x = 0;
                if (paddle.x + paddle.width > WIDTH) paddle.x = WIDTH - paddle.width;
            }
            breakoutLastSelState = currentSelState;
            break;
        case STATE_SNAKE:
            if (currentSelState && !snakeLastSelState) {
                if (snakeState === SNAKE_STATE_MENU) resetSnake();
                else if (snakeState === SNAKE_STATE_GAME) {
                    snakeState = SNAKE_STATE_PAUSED;
                    gameState = STATE_PAUSED;
                    prevGameState = STATE_SNAKE;
                    pauseStaticDrawn = false;
                }
            }
            if (prevPressed && snakeState === SNAKE_STATE_GAME) {
                switch (direction) {
                    case 0: nextDirection = 2; break;
                    case 1: nextDirection = 3; break;
                    case 2: nextDirection = 1; break;
                    case 3: nextDirection = 0; break;
                }
            }
            if (nextPressed && snakeState === SNAKE_STATE_GAME) {
                switch (direction) {
                    case 0: nextDirection = 3; break;
                    case 1: nextDirection = 2; break;
                    case 2: nextDirection = 0; break;
                    case 3: nextDirection = 1; break;
                }
            }
            snakeLastSelState = currentSelState;
            break;
        case STATE_SPACE_SHOOTER:
            if (currentSelState && !spaceLastSelState) {
                if (spaceState === SPACE_STATE_MENU) resetSpaceShooter();
                else if (spaceState === SPACE_STATE_GAME) {
                    spaceIsPaused = true;
                    gameState = STATE_PAUSED;
                    prevGameState = STATE_SPACE_SHOOTER;
                    pauseStaticDrawn = false;
                }
            }
            if (prevPressed) player.x -= player.speed;
            if (nextPressed) player.x += player.speed;
            if (player.x < player.width / 2) player.x = player.width / 2;
            if (player.x > WIDTH - player.width / 2) player.x = WIDTH - player.width / 2;
            spaceLastSelState = currentSelState;
            break;
        case STATE_SLOTS:
            if (currentSelState && !slotLastSelState) {
                if (slotState === SLOT_STATE_MENU) resetSlots();
                else if (slotState === SLOT_STATE_SPIN) updateSlots(true);
                else if (slotState === SLOT_STATE_GAME_OVER) resetSlots();
                else if (slotState === SLOT_STATE_PAUSED) {
                    if (pauseMenuSelection === 0) {
                        slotState = SLOT_STATE_SPIN;
                        slotStaticDrawn = false;
                    } else if (pauseMenuSelection === 1) {
                        gameState = STATE_MAIN_MENU;
                        resetSlots();
                        staticDrawn = false;
                        mainMenuStaticDrawn = false;
                        menuLastSelState = false;
                        breakoutLastSelState = false;
                        snakeLastSelState = false;
                        spaceLastSelState = false;
                        slotLastSelState = false;
                    } else if (pauseMenuSelection === 2) {
                        gameState = STATE_EXIT_CONFIRM;
                        prevGameState = STATE_SLOTS;
                        exitConfirmSelection = 1;
                        staticDrawn = false;
                    }
                }
            }
            if (slotState === SLOT_STATE_SPIN) {
                if (prevPressed) {
                    slotState = SLOT_STATE_PAUSED;
                    pauseMenuSelection = 0;
                    slotStaticDrawn = false;
                }
                if (nextPressed) {
                    slotBetIndex = (slotBetIndex + 1) % slotBetOptions.length;
                    slotStaticDrawn = false;
                }
            } else if (slotState === SLOT_STATE_PAUSED) {
                if (prevPressed) {
                    pauseMenuSelection = (pauseMenuSelection - 1 + 3) % 3;
                    slotStaticDrawn = false;
                }
                if (nextPressed) {
                    pauseMenuSelection = (pauseMenuSelection + 1) % 3;
                    slotStaticDrawn = false;
                }
            } else if (slotState === SLOT_STATE_GAME_OVER) {
                if (prevPressed) {
                    gameState = STATE_MAIN_MENU;
                    staticDrawn = false;
                    mainMenuStaticDrawn = false;
                    menuLastSelState = false;
                    slotLastSelState = false;
                }
            }
            slotLastSelState = currentSelState;
            break;
        case STATE_FLAPPY_BIRD:
            if (currentSelState && !flappyLastSelState) {
                if (flappyState === FLAPPY_STATE_MENU) {
                    flappyState = FLAPPY_STATE_GAME;
                    flappyStaticDrawn = false;
                } else if (flappyState === FLAPPY_STATE_GAME) {
                    bird.velocity = -FLAP_POWER;
                    audio.tone(600, 150);
                }
            }
            if (prevPressed && flappyState === FLAPPY_STATE_GAME) {
                flappyState = FLAPPY_STATE_PAUSED;
                gameState = STATE_PAUSED;
                prevGameState = STATE_FLAPPY_BIRD;
                pauseStaticDrawn = false;
            }
            if (nextPressed && flappyState === FLAPPY_STATE_GAME) {
                flappyState = FLAPPY_STATE_PAUSED;
                gameState = STATE_PAUSED;
                prevGameState = STATE_FLAPPY_BIRD;
                pauseStaticDrawn = false;
            }
            flappyLastSelState = currentSelState;
            break;
        case STATE_BLACKJACK:
            var betMenuSelection = selectedBetIndex;
            if (typeof blackjack.prevState === 'undefined') {
                blackjack.prevState = -1;
            }
            if (currentSelState && !menuLastSelState) {
                if (blackjack.state === 1) {
                    startBlackjackGame(BET_OPTIONS[betMenuSelection]);
                } else if (blackjack.state === 2) {
                    stand();
                } else if (blackjack.state === 3) {
                    if (blackjack.playerMoney <= 0) {
                        prevGameState = STATE_BLACKJACK;
                        gameState = STATE_GAME_OVER;
                        staticDrawn = false;
                    } else {
                        blackjack.dealerHand = [];
                        blackjack.playerHand = [];
                        blackjack.currentBet = 0;
                        blackjack.playerBust = false;
                        blackjack.dealerBust = false;
                        blackjack.playerBlackjack = false;
                        blackjack.state = 1;
                        betMenuSelection = 0;
                        staticDrawn = false;
                    }
                } else if (blackjack.state === 4) {
                    if (pauseMenuSelection === 0) {
                        blackjack.state = blackjack.prevState;
                        staticDrawn = false;
                    } else if (pauseMenuSelection === 1) {
                        gameState = STATE_MAIN_MENU;
                        resetBlackjack();
                        staticDrawn = false;
                        mainMenuStaticDrawn = false;
                        menuLastSelState = false;
                    } else if (pauseMenuSelection === 2) {
                        gameState = STATE_EXIT_CONFIRM;
                        prevGameState = STATE_BLACKJACK;
                        exitConfirmSelection = 1;
                        staticDrawn = false;
                    }
                }
            }
            if (nextPressed && blackjack.state === 1) {
                betMenuSelection = (betMenuSelection + 1) % BET_OPTIONS.length;
                staticDrawn = false;
            }
            if (prevPressed && blackjack.state === 1) {
                betMenuSelection = (betMenuSelection - 1 + BET_OPTIONS.length) % BET_OPTIONS.length;
                staticDrawn = false;
            }
            if (prevPressed && blackjack.state === 2) {
                blackjack.prevState = blackjack.state;
                blackjack.state = 4;
                pauseMenuSelection = 0;
                staticDrawn = false;
            }
            if (blackjack.state === 4) {
                if (prevPressed) {
                    pauseMenuSelection = (pauseMenuSelection - 1 + 3) % 3;
                    staticDrawn = false;
                }
                if (nextPressed) {
                    pauseMenuSelection = (pauseMenuSelection + 1) % 3;
                    staticDrawn = false;
                }
            }
            if (nextPressed && blackjack.state === 2 && !blackjack.playerBust && !blackjack.playerBlackjack) {
                hit();
            }
            selectedBetIndex = betMenuSelection;
            menuLastSelState = currentSelState;
            break;
        case STATE_PAUSED:
            if (prevPressed) {
                pauseMenuSelection = (pauseMenuSelection - 1 + 3) % 3;
                pauseStaticDrawn = false;
            }
            if (nextPressed) {
                pauseMenuSelection = (pauseMenuSelection + 1) % 3;
                pauseStaticDrawn = false;
            }
            if (currentSelState && !menuLastSelState && (prevGameState !== STATE_BREAKOUT || breakoutPauseCooldown <= 0)) {
                if (pauseMenuSelection === 0) {
                    gameState = prevGameState;
                    if (prevGameState === STATE_BREAKOUT) {
                        breakoutIsPaused = false;
                        breakoutStaticDrawn = false;
                        breakoutLastStaticDrawnState = -1;
                    }
                    if (prevGameState === STATE_SPACE_SHOOTER) {
                        spaceIsPaused = false;
                        spaceStaticDrawn = false;
                        spaceLastStaticDrawnState = -1;
                    }
                    if (prevGameState === STATE_SNAKE) {
                        snakeState = SNAKE_STATE_GAME;
                        snakeStaticDrawn = false;
                        snakeLastStaticDrawnState = -1;
                    }
                    if (prevGameState === STATE_FLAPPY_BIRD) {
                        flappyState = FLAPPY_STATE_GAME;
                        flappyStaticDrawn = false;
                    }
                    staticDrawn = false;
                    pauseStaticDrawn = false;
                } else if (pauseMenuSelection === 1) {
                    gameState = STATE_MAIN_MENU;
                    staticDrawn = false;
                    mainMenuStaticDrawn = false;
                    menuLastSelState = false;
                    breakoutLastSelState = false;
                    snakeLastSelState = false;
                    spaceLastSelState = false;
                    slotLastSelState = false;
                    flappyLastSelState = false;
                    snakeState = SNAKE_STATE_MENU;
                    if (prevGameState === STATE_SPACE_SHOOTER) resetSpaceShooter();
                    if (prevGameState === STATE_FLAPPY_BIRD) flappyState = FLAPPY_STATE_MENU;
                    prevGameState = -1;
                } else if (pauseMenuSelection === 2) {
                    gameState = STATE_EXIT_CONFIRM;
                    exitConfirmSelection = 1;
                    staticDrawn = false;
                }
            }
            menuLastSelState = currentSelState;
            if (prevGameState === STATE_BREAKOUT && breakoutPauseCooldown > 0) breakoutPauseCooldown--;
            break;
        case STATE_GAME_OVER:
            if (prevPressed) {
                pauseMenuSelection = (pauseMenuSelection - 1 + 3) % 3;
                staticDrawn = false;
            }
            if (nextPressed) {
                pauseMenuSelection = (pauseMenuSelection + 1) % 3;
                staticDrawn = false;
            }
            if (currentSelState && !menuLastSelState) {
                if (pauseMenuSelection === 0) {
                    if (prevGameState === STATE_FLAPPY_BIRD) resetFlappyBird();
                    else if (prevGameState === STATE_SNAKE) resetSnake();
                    else if (prevGameState === STATE_SPACE_SHOOTER) resetSpaceShooter();
                    else if (prevGameState === STATE_BREAKOUT) resetBreakout();
                    else if (prevGameState === STATE_BLACKJACK) resetBlackjack();
                    gameState = prevGameState;
                    staticDrawn = false;
                } else if (pauseMenuSelection === 1) {
                    gameState = STATE_MAIN_MENU;
                    staticDrawn = false;
                    mainMenuStaticDrawn = false;
                    menuLastSelState = false;
                    breakoutLastSelState = false;
                    snakeLastSelState = false;
                    spaceLastSelState = false;
                    slotLastSelState = false;
                    snakeState = SNAKE_STATE_MENU;
                    if (prevGameState === STATE_SPACE_SHOOTER) resetSpaceShooter();
                    prevGameState = -1;
                } else if (pauseMenuSelection === 2) {
                    gameState = STATE_EXIT_CONFIRM;
                    exitConfirmSelection = 1;
                    staticDrawn = false;
                }
            }
            menuLastSelState = currentSelState;
            if (gameState === STATE_MAIN_MENU) menuLastSelState = false;
            break;
        case STATE_LEVEL_UP:
            if (prevPressed) {
                gameState = STATE_MAIN_MENU;
                staticDrawn = false;
                mainMenuStaticDrawn = false;
                menuLastSelState = false;
                breakoutLastSelState = false;
                snakeLastSelState = false;
                spaceLastSelState = false;
                slotLastSelState = false;
            }
            if (currentSelState && !menuLastSelState) {
                if (prevGameState === STATE_SPACE_SHOOTER) {
                    spaceState = SPACE_STATE_GAME;
                    gameState = prevGameState;
                    spaceStaticDrawn = false;
                } else if (prevGameState === STATE_BREAKOUT) {
                    resetBreakout();
                    gameState = prevGameState;
                    breakoutStaticDrawn = false;
                }
            }
            menuLastSelState = currentSelState;
            break;
        case STATE_EXIT_CONFIRM:
            if (prevPressed) {
                exitConfirmSelection = (exitConfirmSelection - 1 + 2) % 2;
                staticDrawn = false;
            }
            if (nextPressed) {
                exitConfirmSelection = (exitConfirmSelection + 1) % 2;
                staticDrawn = false;
            }
            if (currentSelState && !menuLastSelState) {
                if (exitConfirmSelection === 0) {
                    shouldExit = true;
                } else {
                    gameState = STATE_MAIN_MENU;
                    staticDrawn = false;
                    mainMenuStaticDrawn = false;
                    menuLastSelState = false;
                    breakoutLastSelState = false;
                    snakeLastSelState = false;
                    spaceLastSelState = false;
                    slotLastSelState = false;
                    flappyLastSelState = false;
                }
            }
            menuLastSelState = currentSelState;
            break;
    }
    return shouldExit;
}
function main() {
    gameState = STATE_MAIN_MENU;
    createStars();
    while (true) {
        var startTime = Date.now();
        if (handleInput()) break;
        switch (gameState) {
            case STATE_MAIN_MENU: drawMainMenu(); break;
            case STATE_BREAKOUT: updateBreakout(); drawBreakout(); break;
            case STATE_SNAKE: updateSnake(); drawSnake(); break;
            case STATE_SPACE_SHOOTER: updateSpaceShooter(); drawSpaceShooter(); break;
            case STATE_SLOTS: updateSlots(false); drawSlots(); break;
            case STATE_FLAPPY_BIRD: updateFlappyBird(); drawFlappyBird(); break;
            case STATE_BLACKJACK: updateBlackjack(); drawBlackjack(); break;
            case STATE_PAUSED: drawPauseMenu(); break;
            case STATE_GAME_OVER: drawGameOverMenu(); break;
            case STATE_LEVEL_UP: drawLevelUpMenu(); break;
            case STATE_EXIT_CONFIRM: drawExitConfirm(); break;
        }
        frameCounter++;
        var frameTime = Date.now() - startTime;
        delay(Math.max(1, 33 - frameTime));
    }
}
main();
