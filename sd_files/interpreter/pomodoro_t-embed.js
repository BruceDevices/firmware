// Pomodoro for Bruce (T-Embed CC1101) - userspace BJS script
// Controls: encoder rotate = NEXT/PREV, encoder press = SEL, side button = ESC
// During a phase: SEL = pause/resume, NEXT = skip to next phase, ESC = exit

var display = require('display');
var keyboard = require('keyboard');
var audio = require('audio');
var led = require('led');

var W = display.width();
var H = display.height();
var CX = Math.floor(W / 2);
var CY = Math.floor(H / 2);

var BG = display.color(0, 0, 0);
var FG = display.color(220, 220, 220);
var DIM = display.color(96, 96, 96);
var YELLOW = display.color(255, 220, 0);
var RED = display.color(220, 30, 0);
var GREEN = display.color(0, 200, 30);
var CYAN = display.color(0, 180, 220);

var WORK = 0;
var SHORT_BREAK = 1;
var LONG_BREAK = 2;

function phaseLabel(p) {
    if (p === WORK) return "WORK";
    if (p === SHORT_BREAK) return "SHORT BREAK";
    return "LONG BREAK";
}
function phaseColor(p) {
    if (p === WORK) return RED;
    if (p === SHORT_BREAK) return GREEN;
    return CYAN;
}
function ledForPhase(p, dim) {
    var v = dim ? 50 : 180;
    if (p === WORK) led.setColor(v, 0, 0);
    else if (p === SHORT_BREAK) led.setColor(0, v, 0);
    else led.setColor(0, Math.floor(v * 0.6), v);
}

function pad2(n) { return n < 10 ? "0" + n : "" + n; }

// ---- Setup screen ----
var workMin = 25;
var shortMin = 5;
var longMin = 15;
var rounds = 4;
var soundOn = true;

var FIELDS = ["Work", "Short break", "Long break", "Rounds", "Sound", "[ START ]"];
var minRange = [1, 1, 1, 2, 0, 0];
var maxRange = [90, 60, 60, 8, 1, 0];

function getFieldValue(i) {
    if (i === 0) return workMin + " min";
    if (i === 1) return shortMin + " min";
    if (i === 2) return longMin + " min";
    if (i === 3) return "" + rounds;
    if (i === 4) return soundOn ? "ON" : "OFF";
    return "";
}
function bumpField(i, dir) {
    if (i === 4) { soundOn = !soundOn; return; }
    if (i === 5) return;
    var vals = [workMin, shortMin, longMin, rounds, 0];
    var v = vals[i] + dir;
    if (v < minRange[i]) v = maxRange[i];
    if (v > maxRange[i]) v = minRange[i];
    if (i === 0) workMin = v;
    else if (i === 1) shortMin = v;
    else if (i === 2) longMin = v;
    else if (i === 3) rounds = v;
}

function drawSetup(sel) {
    display.fillScreen(BG);
    display.setTextSize(2);
    display.setTextColor(YELLOW, BG);
    display.setTextAlign('center', 'top');
    display.drawString("Pomodoro", CX, 6);

    display.setTextSize(1);
    var rowY = 36;
    var rowH = 14;
    for (var i = 0; i < FIELDS.length; i++) {
        var y = rowY + i * rowH;
        var hi = (i === sel);
        var color = hi ? FG : DIM;
        display.setTextColor(color, BG);
        if (i === 5) {
            display.setTextAlign('center', 'top');
            display.drawString(FIELDS[i], CX, y);
            if (hi) {
                display.drawFillRect(CX - 40, y + 10, 80, 2, FG);
            }
        } else {
            display.setTextAlign('left', 'top');
            display.drawString(FIELDS[i], 10, y);
            display.setTextAlign('right', 'top');
            display.drawString(getFieldValue(i), W - 10, y);
        }
    }
    display.setTextAlign('left', 'top');
}

function configure() {
    var sel = 0;
    drawSetup(sel);
    while (true) {
        if (keyboard.getEscPress()) return false;
        if (keyboard.getNextPress()) {
            if (sel === 5) {
                sel = 0;
            } else if (sel === 4) {
                soundOn = !soundOn;
            } else {
                bumpField(sel, 1);
            }
            drawSetup(sel);
        }
        if (keyboard.getPrevPress()) {
            bumpField(sel, -1);
            drawSetup(sel);
        }
        if (keyboard.getSelPress()) {
            if (sel === 5) return true;
            sel = sel + 1;
            drawSetup(sel);
        }
        delay(50);
    }
}

// ---- Run-phase screen ----
function drawRunFrame(phase, round, remainSec, totalSec, paused) {
    display.fillScreen(BG);

    display.setTextSize(1);
    display.setTextColor(YELLOW, BG);
    display.setTextAlign('center', 'top');
    var title = (phase === WORK) ? ("Pomodoro #" + round) : ("Break");
    display.drawString(title, CX, 4);

    display.setTextSize(2);
    display.setTextColor(phaseColor(phase), BG);
    display.drawString(phaseLabel(phase), CX, 22);

    var mins = Math.floor(remainSec / 60);
    var secs = remainSec % 60;
    display.setTextSize(4);
    display.setTextColor(FG, BG);
    display.drawString(pad2(mins) + ":" + pad2(secs), CX, CY - 18);

    var barW = W - 24;
    var barX = 12;
    var barY = CY + 22;
    var barH = 8;
    display.drawRect(barX, barY, barW, barH, FG);
    var fill = 0;
    if (totalSec > 0) {
        fill = Math.floor((barW - 2) * (totalSec - remainSec) / totalSec);
    }
    if (fill < 0) fill = 0;
    if (fill > barW - 2) fill = barW - 2;
    display.drawFillRect(barX + 1, barY + 1, fill, barH - 2, phaseColor(phase));
    display.drawFillRect(barX + 1 + fill, barY + 1, barW - 2 - fill, barH - 2, BG);

    display.setTextSize(1);
    display.setTextColor(DIM, BG);
    display.drawString("Round " + round + "  SEL=pause  NEXT=skip", CX, H - 14);

    if (paused) {
        display.setTextColor(YELLOW, BG);
        display.drawString("-- PAUSED --", CX, H - 26);
    }
    display.setTextAlign('left', 'top');
}

function runPhase(phase, round, durationSec) {
    var startMs = now();
    var pausedAt = 0;
    var pausedTotal = 0;
    var paused = false;
    var lastShown = -1;

    drawRunFrame(phase, round, durationSec, durationSec, false);
    ledForPhase(phase, false);

    while (true) {
        var t = now();

        if (keyboard.getEscPress()) return "exit";
        if (keyboard.getNextPress()) return "skip";
        if (keyboard.getSelPress()) {
            if (!paused) { paused = true; pausedAt = t; ledForPhase(phase, true); }
            else { paused = false; pausedTotal += (t - pausedAt); ledForPhase(phase, false); }
            lastShown = -1;
            delay(150);
        }

        var elapsedMs = paused ? (pausedAt - startMs - pausedTotal)
                               : (t - startMs - pausedTotal);
        var elapsedSec = Math.floor(elapsedMs / 1000);
        var remaining = durationSec - elapsedSec;
        if (remaining < 0) remaining = 0;

        if (!paused && remaining === 0) return "done";

        if (remaining !== lastShown) {
            drawRunFrame(phase, round, remaining, durationSec, paused);
            lastShown = remaining;
        }
        delay(50);
    }
}

function phaseEndAlarm(justEnded, nextPhase) {
    display.fillScreen(BG);
    display.setTextSize(2);
    display.setTextColor(phaseColor(nextPhase), BG);
    display.setTextAlign('center', 'top');
    var banner = (justEnded === WORK) ? "BREAK TIME" : "BACK TO WORK";
    display.drawString(banner, CX, CY - 16);
    display.setTextSize(1);
    display.setTextColor(FG, BG);
    display.drawString("SEL: continue   ESC: stop", CX, CY + 16);
    display.setTextAlign('left', 'top');

    var workEnded = (justEnded === WORK);
    var beeps = soundOn ? 6 : 0;
    for (var i = 0; i < beeps; i++) {
        if (keyboard.getSelPress()) { delay(150); return true; }
        if (keyboard.getEscPress()) return false;
        ledForPhase(nextPhase, false);
        if (workEnded) { audio.tone(880, 120); delay(60); audio.tone(1320, 200); }
        else           { audio.tone(660, 200); delay(60); audio.tone(440, 250); }
        led.off();
        var startWait = now();
        while (now() - startWait < 400) {
            if (keyboard.getSelPress()) { delay(150); return true; }
            if (keyboard.getEscPress()) return false;
            delay(10);
        }
    }
    ledForPhase(nextPhase, false);
    while (true) {
        if (keyboard.getSelPress()) { delay(150); return true; }
        if (keyboard.getEscPress()) return false;
        delay(50);
    }
}

function nextBreakPhase(round) {
    return (round % rounds === 0) ? LONG_BREAK : SHORT_BREAK;
}

// ---- Main ----
if (!configure()) {
    led.off();
    exit();
}

var round = 1;
while (true) {
    var workSec = workMin * 60;
    var r = runPhase(WORK, round, workSec);
    if (r === "exit") { led.off(); exit(); }

    var brk = nextBreakPhase(round);
    if (r !== "skip") {
        if (!phaseEndAlarm(WORK, brk)) { led.off(); exit(); }
    }

    var brkSec = (brk === LONG_BREAK ? longMin : shortMin) * 60;
    r = runPhase(brk, round, brkSec);
    if (r === "exit") { led.off(); exit(); }
    if (r !== "skip") {
        if (!phaseEndAlarm(brk, WORK)) { led.off(); exit(); }
    }

    round = round + 1;
    if (round > 99) round = 1;
}
