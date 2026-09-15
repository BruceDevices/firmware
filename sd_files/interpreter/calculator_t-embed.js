var display = require('display');
var keyboardApi = require('keyboard');

var width = display.width;
var height = display.height;
var color = display.color;
var drawFillRect = display.drawFillRect;
var drawRect = display.drawRect;
var drawString = display.drawString;
var setTextColor = display.setTextColor;
var setTextSize = display.setTextSize;

var getPrevPress = keyboardApi.getPrevPress;
var getNextPress = keyboardApi.getNextPress;
var getSelPress = keyboardApi.getSelPress;

// SCRIPT NOT MINE! Script author : sadecemsi
// change by timenetworks
// i remade this script for t-embed screen
var screenWidth = width();
var screenHeight = height();

var COLOR_BLACK = color(0, 0, 0);
var COLOR_WHITE = color(255, 255, 255);
var COLOR_GRAY = color(128, 128, 128);
var COLOR_BLUE = color(0, 100, 200);
var COLOR_GREEN = color(0, 150, 0);
var COLOR_RED = color(200, 0, 0);
var COLOR_ORANGE = color(255, 165, 0);

var display = "0";
var currentNumber = "";
var previousNumber = "";
var operator = "";
var waitingForNewNumber = false;

var keyboard = [
    ["C", "DEL", "+/-", "*"],
    ["7", "8", "9", "/"],
    ["4", "5", "6", "-"],
    ["1", "2", "3", "+"],
    ["0", ".", "=", "EXIT"]
];

var selectedRow = 0;
var selectedCol = 0;

var KEYBOARD_START_Y = 50;
var KEYBOARD_START_X = 70;
var KEY_SPACING = 2;
var KEY_WIDTH = (screenWidth - KEYBOARD_START_X - KEY_SPACING)/4;
var KEY_HEIGHT = (screenHeight - KEYBOARD_START_Y - KEY_SPACING)/5;


var redraw = true;
var shouldExit = false;

function exit() {
    shouldExit = true;
}

function setupScreen() {
    drawFillRect(0, 0, screenWidth, screenHeight, COLOR_BLACK);
    drawTitle();
    drawDisplay();
    drawKeyboard();
}

function drawTitle() {
    setTextColor(COLOR_WHITE);
    setTextSize(1);
    drawString("Calculator", screenWidth/2-30, 5);
}

function drawDisplay() {
    var displayWidth = screenWidth - 20;
    var displayX = 10;
    drawFillRect(displayX, 18, displayWidth, 25, COLOR_WHITE);
    drawRect(displayX, 18, displayWidth, 25, COLOR_GRAY);

    setTextColor(COLOR_BLACK);
    setTextSize(1);

    var displayText = display;
    if (displayText.length > 18) {
        displayText = displayText.substring(displayText.length - 18);
    }

    var textWidth = displayText.length * 6;
    var textX = displayX + displayWidth - textWidth - 5;
    if (textX < displayX + 5) textX = displayX + 5;

    drawString(displayText, textX, 28);
}

function drawKeyboard() {
    for (var row = 0; row < keyboard.length; row++) {
        for (var col = 0; col < keyboard[row].length; col++) {
            var key = keyboard[row][col];
            if (key === "") continue;

            var x = KEYBOARD_START_X + col * (KEY_WIDTH + KEY_SPACING);
            var y = KEYBOARD_START_Y + row * (KEY_HEIGHT + KEY_SPACING);

            var keyColor = COLOR_WHITE;
            var textColor = COLOR_BLACK;

            if (row === selectedRow && col === selectedCol) {
                keyColor = COLOR_BLUE;
                textColor = COLOR_WHITE;
            }

            if (key === "C") {
                keyColor = (row === selectedRow && col === selectedCol) ? COLOR_RED : color(255, 200, 200);
            } else if (key === "=") {
                keyColor = (row === selectedRow && col === selectedCol) ? COLOR_GREEN : color(200, 255, 200);
            } else if (key === "EXIT") {
                keyColor = (row === selectedRow && col === selectedCol) ? COLOR_RED : color(255, 150, 150);
            } else if ("+-*/".indexOf(key) >= 0 || key === "+/-") {
                keyColor = (row === selectedRow && col === selectedCol) ? COLOR_ORANGE : color(255, 230, 200);
            }

            drawFillRect(x, y, KEY_WIDTH, KEY_HEIGHT, keyColor);
            drawRect(x, y, KEY_WIDTH, KEY_HEIGHT, COLOR_GRAY);

            setTextColor(textColor);
            setTextSize(1);

            var textX = x + KEY_WIDTH / 2 - 3;
            var textY = y + KEY_HEIGHT / 2 - 3;

            if (key === "EXIT") {
                textX = x + 3;
                textY = y + KEY_HEIGHT / 2 - 3;
            }

            drawString(key, textX, textY);
        }
    }
}

function moveSelection(direction) {
    if (direction === "up") {
        if (selectedRow > 0) {
            selectedRow--;
        } else {
            selectedRow = keyboard.length - 1;
        }
    } else if (direction === "down") {
        if (selectedRow < keyboard.length - 1) {
            selectedRow++;
        } else {
            selectedRow = 0;
        }
    } else if (direction === "left") {
        if (selectedCol > 0) {
            selectedCol--;
        } else {
            selectedRow = (selectedRow - 1 + keyboard.length) % keyboard.length;
            selectedCol = keyboard[selectedRow].length - 1;
        }
    } else if (direction === "right") {
        if (selectedCol < keyboard[selectedRow].length - 1) {
            selectedCol++;
        } else {
            selectedRow = (selectedRow + 1) % keyboard.length;
            selectedCol = 0;
        }
    }

    while (keyboard[selectedRow][selectedCol] === "") {
        selectedCol = (selectedCol + 1) % keyboard[selectedRow].length;
    }
}

function pressKey() {
    var key = keyboard[selectedRow][selectedCol];
    if (key === "") return;

    if (key === "C") {
        clear();
    } else if (key === "DEL") {
        backspace();
    } else if (key === "=") {
        calculate();
    } else if (key === "+/-") {
        toggleSign();
    } else if (key === "EXIT") {
        exit();
    } else if ("+-*/".indexOf(key) >= 0) {
        setOperator(key);
    } else if (key === ".") {
        addDecimal();
    } else {
        addNumber(key);
    }

    redraw = true;
}

function toggleSign() {
    if (currentNumber !== "" && currentNumber !== "0") {
        if (currentNumber.indexOf("-") === 0) {
            currentNumber = currentNumber.substring(1);
        } else {
            currentNumber = "-" + currentNumber;
        }
        display = currentNumber;
    }
}

function addNumber(num) {
    if (waitingForNewNumber) {
        currentNumber = num;
        waitingForNewNumber = false;
    } else {
        if (currentNumber === "0") {
            currentNumber = num;
        } else {
            currentNumber += num;
        }
    }
    display = currentNumber;
}

function addDecimal() {
    if (waitingForNewNumber) {
        currentNumber = "0.";
        waitingForNewNumber = false;
    } else {
        if (currentNumber.indexOf(".") === -1) {
            currentNumber += ".";
        }
    }
    display = currentNumber;
}

function setOperator(op) {
    if (operator !== "" && !waitingForNewNumber) {
        calculate();
    }

    previousNumber = currentNumber;
    operator = op;
    waitingForNewNumber = true;
    display = currentNumber;
}

function calculate() {
    if (operator === "" || waitingForNewNumber) return;

    var prev = parseFloat(previousNumber);
    var curr = parseFloat(currentNumber);
    var result = 0;

    try {
        switch (operator) {
            case "+": result = prev + curr; break;
            case "-": result = prev - curr; break;
            case "*": result = prev * curr; break;
            case "/":
                if (curr === 0) {
                    display = "Error: Div by 0";
                    clear();
                    return;
                }
                result = prev / curr;
                break;
        }

        if (result.toString().length > 12) {
            if (Math.abs(result) >= 1e6 || Math.abs(result) < 0.001) {
                result = result.toExponential(3);
            } else {
                result = result.toPrecision(6);
            }
        }

        currentNumber = result.toString();
        display = currentNumber;
        operator = "";
        waitingForNewNumber = true;

    } catch (e) {
        display = "Error";
        clear();
    }
}

function clear() {
    display = "0";
    currentNumber = "";
    previousNumber = "";
    operator = "";
    waitingForNewNumber = false;
}

function backspace() {
    if (currentNumber.length > 1) {
        currentNumber = currentNumber.slice(0, -1);
        display = currentNumber;
    } else {
        currentNumber = "0";
        display = "0";
    }
}

function updateDisplay() {
    var displayWidth = screenWidth - 20;
    var displayX = 10;
    drawFillRect(displayX, 18, displayWidth, 25, COLOR_WHITE);
    drawRect(displayX, 18, displayWidth, 25, COLOR_GRAY);

    setTextColor(COLOR_BLACK);
    setTextSize(1);

    var displayText = display;
    if (displayText.length > 18) {
        displayText = displayText.substring(displayText.length - 18);
    }

    var textWidth = displayText.length * 6;
    var textX = displayX + displayWidth - textWidth - 5;
    if (textX < displayX + 5) textX = displayX + 5;

    drawString(displayText, textX, 28);
    drawKeyboard();
}

setTextSize(1);
setupScreen();

while (!shouldExit) {
    if (getPrevPress()) {
        moveSelection("left");
        redraw = true;
    }
    if (getNextPress()) {
        moveSelection("right");
        redraw = true;
    }
    if (getSelPress()) {
        pressKey();
    }

    if (redraw) {
        updateDisplay();
        redraw = false;
    }

    delay(50);
}
