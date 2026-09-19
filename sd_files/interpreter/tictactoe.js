var display = require("display");
var keyboard = require("keyboard");

// Tic Tac Toe Game
var screenWidth = display.width();
var screenHeight = display.height();

var BG = display.color(20, 25, 35);
var WHITE = display.color(255, 255, 255);
var RED = display.color(244, 67, 54);
var BLUE = display.color(33, 150, 243);
var GRAY = display.color(80, 80, 80);
var YELLOW = display.color(255, 235, 59);

var board = [0, 0, 0, 0, 0, 0, 0, 0, 0]; // 0=empty, 1=X, 2=O
var cursor = 4;
var turn = 1; // 1=X, 2=O
var winner = 0;
var shouldExit = false;

var cellSize = 32;
var gridX = (screenWidth - cellSize * 3) / 2;
var gridY = 20;

function checkWin() {
  var lines = [
    [0, 1, 2],
    [3, 4, 5],
    [6, 7, 8], // rows
    [0, 3, 6],
    [1, 4, 7],
    [2, 5, 8], // cols
    [0, 4, 8],
    [2, 4, 6], // diags
  ];

  for (var i = 0; i < lines.length; i++) {
    var a = lines[i][0];
    var b = lines[i][1];
    var c = lines[i][2];
    if (board[a] !== 0 && board[a] === board[b] && board[b] === board[c]) {
      return board[a];
    }
  }

  var full = true;
  for (var j = 0; j < 9; j++) {
    if (board[j] === 0) full = false;
  }
  if (full) return 3; // draw

  return 0;
}

function drawBoard() {
  display.drawFillRect(0, 0, screenWidth, screenHeight, BG);

  // Title
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawString("TIC TAC TOE", screenWidth / 2 - 35, 5);

  // Grid lines
  for (var i = 1; i < 3; i++) {
    display.drawLine(
      gridX + i * cellSize,
      gridY,
      gridX + i * cellSize,
      gridY + 3 * cellSize,
      WHITE
    );
    display.drawLine(
      gridX,
      gridY + i * cellSize,
      gridX + 3 * cellSize,
      gridY + i * cellSize,
      WHITE
    );
  }

  // Cells
  for (var row = 0; row < 3; row++) {
    for (var col = 0; col < 3; col++) {
      var idx = row * 3 + col;
      var cx = gridX + col * cellSize + cellSize / 2;
      var cy = gridY + row * cellSize + cellSize / 2;

      // Highlight cursor
      if (idx === cursor && winner === 0) {
        display.drawRect(
          gridX + col * cellSize + 2,
          gridY + row * cellSize + 2,
          cellSize - 4,
          cellSize - 4,
          YELLOW
        );
      }

      // Draw X or O
      if (board[idx] === 1) {
        display.setTextColor(RED);
        display.setTextSize(2);
        display.drawString("X", cx - 7, cy - 8);
      } else if (board[idx] === 2) {
        display.setTextColor(BLUE);
        display.setTextSize(2);
        display.drawString("O", cx - 7, cy - 8);
      }
    }
  }

  // Status
  display.setTextSize(1);
  if (winner === 0) {
    display.setTextColor(turn === 1 ? RED : BLUE);
    display.drawString(
      "Turn: " + (turn === 1 ? "X" : "O"),
      10,
      screenHeight - 25
    );
  } else if (winner === 3) {
    display.setTextColor(GRAY);
    display.drawString("Draw!", screenWidth / 2 - 18, screenHeight - 25);
  } else {
    display.setTextColor(winner === 1 ? RED : BLUE);
    display.drawString(
      (winner === 1 ? "X" : "O") + " wins!",
      screenWidth / 2 - 25,
      screenHeight - 25
    );
  }

  display.setTextColor(GRAY);
  display.drawString("SEL:Place PREV/NEXT:Move", 5, screenHeight - 12);
}

function reset() {
  board = [0, 0, 0, 0, 0, 0, 0, 0, 0];
  cursor = 4;
  turn = 1;
  winner = 0;
}

drawBoard();

while (!shouldExit) {
  if (winner === 0) {
    if (keyboard.getPrevPress()) {
      cursor = (cursor - 1 + 9) % 9;
      drawBoard();
    }
    if (keyboard.getNextPress()) {
      cursor = (cursor + 1) % 9;
      drawBoard();
    }
    if (keyboard.getSelPress()) {
      if (board[cursor] === 0) {
        board[cursor] = turn;
        winner = checkWin();
        if (winner === 0) {
          turn = turn === 1 ? 2 : 1;
        }
        drawBoard();
      }
    }
  } else {
    if (keyboard.getSelPress()) {
      reset();
      drawBoard();
    }
  }

  if (keyboard.getEscPress()) {
    shouldExit = true;
  }

  delay(80);
}
