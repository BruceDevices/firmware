var display = require("display");
var keyboard = require("keyboard");

// Snake Game - Turn Left/Right controls
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(10, 20, 10);
var WHITE = display.color(255, 255, 255);
var GREEN = display.color(0, 220, 0);
var DARK_GREEN = display.color(0, 150, 0);
var RED = display.color(255, 50, 50);
var GRAY = display.color(80, 80, 80);

var CELL = 8;
var COLS = Math.floor(screenWidth / CELL);
var ROWS = Math.floor((screenHeight - 18) / CELL);
var OFFSET_Y = 15;

var snake = [{ x: Math.floor(COLS / 2), y: Math.floor(ROWS / 2) }];
// Direction: 0=right, 1=down, 2=left, 3=up
var direction = 0;
var dirX = [1, 0, -1, 0];
var dirY = [0, 1, 0, -1];

var food = { x: 0, y: 0 };
var score = 0;
var gameOver = false;
var shouldExit = false;
var speed = 150;

function spawnFood() {
  var valid = false;
  while (!valid) {
    food.x = Math.floor(Math.random() * (COLS - 2)) + 1;
    food.y = Math.floor(Math.random() * (ROWS - 2)) + 1;
    valid = true;
    for (var i = 0; i < snake.length; i++) {
      if (snake[i].x === food.x && snake[i].y === food.y) {
        valid = false;
        break;
      }
    }
  }
}

function drawCell(x, y, color) {
  display.drawFillRect(
    x * CELL,
    y * CELL + OFFSET_Y,
    CELL - 1,
    CELL - 1,
    color
  );
}

function drawGame() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  // Score
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawString("Score: " + score, 5, 3);
  display.drawString("<L  R>", screenWidth - 45, 3);

  // Border
  display.drawRect(0, OFFSET_Y, COLS * CELL, ROWS * CELL, GRAY);

  // Food
  drawCell(food.x, food.y, RED);

  // Snake
  for (var i = 0; i < snake.length; i++) {
    drawCell(snake[i].x, snake[i].y, i === 0 ? GREEN : DARK_GREEN);
  }

  if (gameOver) {
    display.setTextColor(RED);
    display.setTextSize(2);
    display.drawString(
      "GAME OVER",
      screenWidth / 2 - 55,
      screenHeight / 2 - 15
    );
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.drawString(
      "SEL: Retry",
      screenWidth / 2 - 30,
      screenHeight / 2 + 12
    );
  }
}

function turnLeft() {
  direction = (direction + 3) % 4; // Turn left (counter-clockwise)
}

function turnRight() {
  direction = (direction + 1) % 4; // Turn right (clockwise)
}

function update() {
  if (gameOver) return;

  var newHead = {
    x: snake[0].x + dirX[direction],
    y: snake[0].y + dirY[direction],
  };

  // Wall collision
  if (
    newHead.x < 0 ||
    newHead.x >= COLS ||
    newHead.y < 0 ||
    newHead.y >= ROWS
  ) {
    gameOver = true;
    return;
  }

  // Self collision
  for (var i = 0; i < snake.length; i++) {
    if (snake[i].x === newHead.x && snake[i].y === newHead.y) {
      gameOver = true;
      return;
    }
  }

  snake.unshift(newHead);

  if (newHead.x === food.x && newHead.y === food.y) {
    score += 10;
    spawnFood();
    if (speed > 80) speed -= 5;
  } else {
    snake.pop();
  }
}

function reset() {
  snake = [{ x: Math.floor(COLS / 2), y: Math.floor(ROWS / 2) }];
  direction = 0;
  score = 0;
  gameOver = false;
  speed = 150;
  spawnFood();
}

spawnFood();
drawGame();

var lastUpdate = Date.now();

while (!shouldExit) {
  if (!gameOver) {
    // PREV = Turn Left, NEXT = Turn Right
    if (keyboard.getPrevPress()) {
      turnLeft();
    }
    if (keyboard.getNextPress()) {
      turnRight();
    }
  } else {
    if (keyboard.getSelPress()) {
      reset();
      drawGame();
    }
  }

  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  // Snake moves forward automatically
  var now = Date.now();
  if (now - lastUpdate > speed && !gameOver) {
    update();
    drawGame();
    lastUpdate = now;
  }

  delay(20);
}
