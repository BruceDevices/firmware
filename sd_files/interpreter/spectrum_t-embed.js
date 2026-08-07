/*
 *  RF / IR SPECTRUM DETECTOR
 *  Target: LilyGo T-Embed CC1101 Plus (also works on any Bruce board that
 *  has both a CC1101 module and an IR receiver).
 *
 *  Listens on RF (CC1101) and IR AT THE SAME TIME (via subghz.readDual,
 *  which runs both receivers concurrently), tells you if a captured
 *  signal is RF or IR, and saves it into the right Bruce folder:
 *      RF -> /BruceRF/<name>.sub
 *      IR -> /BruceIR/<name>.ir
 *  So you never have to guess which kind of radio a device uses.
 *
 *  Usage: Scripts -> spectrum_t-embed -> Start detector
 *  Point the device at the remote and press its buttons.
 *  It first tries to DECODE the signal, and if that fails it captures it
 *  as RAW so nothing is missed.
 */

var display = require('display');
var keyboard = require('keyboard');
var dialog = require('dialog');
var subghz = require('subghz');
var ir = require('ir');
var storage = require('storage');

/* ---- display ---- */
var fillScreen = display.fill;
var color = display.color;
var width = display.width;
var height = display.height;
var drawString = display.drawString;
var setTextColor = display.setTextColor;
var setTextSize = display.setTextSize;
var drawRect = display.drawRect;
var drawFillRect = display.drawFillRect;
var drawFillCircle = display.drawFillCircle;
var drawFastVLine = display.drawFastVLine;
var drawFastHLine = display.drawFastHLine;

/* ---- input / dialog ---- */
var getAnyPress = keyboard.getAnyPress;
var keyboardPrompt = keyboard.keyboard;
var numKeyboard = keyboard.numKeyboard;
var dialogChoice = dialog.choice;
var dialogInfo = dialog.info;
var dialogSuccess = dialog.success;
var dialogError = dialog.error;

/* ---- radio + storage ---- */
var subghzSetFrequency = subghz.setFrequency;
var subghzRead = subghz.read;
var subghzReadRaw = subghz.readRaw;
var subghzReadDual = subghz.readDual;
var subghzTransmitFile = subghz.transmitFile;
var irRead = ir.read;
var irReadRaw = ir.readRaw;
var irTransmitFile = ir.transmitFile;
var storageWrite = storage.write;
var storageMkdir = storage.mkdir;
var storageReaddir = storage.readdir;
var storageSpaceSD = storage.spaceSDCard;
var storageRemove = storage.remove;

/* ---- palette ---- */
var C_BG  = color(0, 0, 0);
var C_TXT = color(235, 235, 235);
var C_DIM = color(70, 70, 70);
var C_RF  = color(255, 96, 0);
var C_IR  = color(255, 205, 0);
var C_CYN = color(0, 220, 255);

/* ---- folders ---- */
var RF_FOLDER = "/BruceRF";
var IR_FOLDER = "/BruceIR";
var RF_EXT = ".sub";
var IR_EXT = ".ir";

/* ---- settings ---- */
var rfFreq = 433.92;      /* MHz */
var listenWin = 2;        /* seconds per band */
var rfMode = "Auto";      /* Auto (decode then RAW) | Raw */
var irMode = "Auto";      /* Auto (parse then RAW)  | Raw */
var storagePref = "Auto"; /* Auto | sd | littlefs */

var FREQS = [315.0, 330.0, 390.0, 418.0, 433.42, 433.92, 434.42, 868.35, 915.0];

var rfHits = 0;
var irHits = 0;
var cycles = 0;

/* spectrum drawing area (set by drawScanScreen) */
var specX = 0, specY = 0, specW = 0, specH = 0;

/* ---------- helpers ---------- */

function field(content, key) {
    var lines = content.split("\n");
    for (var i = 0; i < lines.length; i++) {
        var ln = lines[i];
        var p = ln.indexOf(":");
        if (p > 0 && ln.substring(0, p) === key) return ln.substring(p + 1).trim();
    }
    return "";
}

function sdAvailable() {
    try {
        var s = storageSpaceSD();
        return (typeof s.total !== "undefined") && (s.total > 0);
    } catch (e) { return false; }
}

function pickFs() {
    if (storagePref === "sd") return "sd";
    if (storagePref === "littlefs") return "littlefs";
    return sdAvailable() ? "sd" : "littlefs";
}

function sanitizeName(n) {
    var out = "";
    for (var i = 0; i < n.length; i++) {
        var c = n.charAt(i);
        if ((c >= "A" && c <= "Z") || (c >= "a" && c <= "z") || (c >= "0" && c <= "9") || c === "_" || c === "-") {
            out += c;
        } else if (c === " ") {
            out += "_";
        }
    }
    if (out.length > 28) out = out.substring(0, 28);
    return out;
}

function uniqueName(fsName, folder, base, ext) {
    var existing = [];
    try { existing = storageReaddir({fs: fsName, path: folder}); } catch (e) { existing = []; }
    var name = base;
    var i = 1;
    while (true) {
        var taken = false;
        for (var j = 0; j < existing.length; j++) {
            if (existing[j] === name + ext) { taken = true; break; }
        }
        if (!taken) return name;
        i++;
        name = base + "_" + i;
    }
}

function saveContent(fsName, folder, name, ext, content) {
    try { storageMkdir({fs: fsName, path: folder}); } catch (mkdirErr) {}
    try { return storageWrite({fs: fsName, path: folder + "/" + name + ext}, content, "w"); }
    catch (writeErr) { return false; }
}

function normalizeIR(content) {
    var hdr = "Filetype: IR signals file";
    if (content.indexOf(hdr) === 0) {
        return "Filetype: Bruce IR File" + content.substring(hdr.length);
    }
    return content;
}

/* ---------- capture info ---------- */

function describeRF(content) {
    var freq = field(content, "Frequency");
    var proto = field(content, "Protocol");
    var bit = field(content, "Bit");
    var key = field(content, "Key");
    var te = field(content, "TE");
    var raw = field(content, "RAW_Data");
    var out = [];
    if (freq !== "") {
        out.push("Freq: " + (parseFloat(freq) / 1000000).toFixed(3) + " MHz");
    } else {
        out.push("Freq: " + rfFreq.toFixed(3) + " MHz");
    }
    out.push("Protocol: " + (proto !== "" ? proto : (raw !== "" ? "RAW" : "?")));
    if (bit !== "") out.push("Bits: " + bit);
    if (key !== "") out.push("Key: " + key);
    if (te !== "") out.push("TE: " + te);
    if (raw !== "") {
        out.push("RAW: " + (raw.length > 30 ? raw.substring(0, 30) + "..." : raw));
    }
    return out;
}

function describeIR(content) {
    var type = field(content, "type");
    var proto = field(content, "protocol");
    var addr = field(content, "address");
    var cmd = field(content, "command");
    var bits = field(content, "bits");
    var val = field(content, "value");
    var data = field(content, "data");
    var out = [];
    out.push("Type: " + (type !== "" ? type : "?"));
    if (proto !== "") out.push("Protocol: " + proto);
    if (addr !== "") out.push("Address: " + addr);
    if (cmd !== "") out.push("Command: " + cmd);
    if (bits !== "") out.push("Bits: " + bits);
    if (val !== "") out.push("Value: " + val);
    if (data !== "") out.push("Pulses: " + data.split(",").length);
    return out;
}

/* ---------- drawing ---------- */

function drawPanel(x, y, w, h, title, sub, active, bandColor) {
    if (active) {
        drawFillRect(x, y, w, h, bandColor);
    } else {
        drawFillRect(x, y, w, h, C_BG);
        drawRect(x, y, w, h, C_DIM);
    }
    setTextSize(2);
    if (active) setTextColor(C_BG, bandColor); else setTextColor(C_DIM, C_BG);
    drawString(title, x + 6, y + 3);
    setTextSize(1);
    if (active) setTextColor(C_BG, bandColor); else setTextColor(C_TXT, C_BG);
    drawString(sub, x + 6, y + h - 13);
    if (active) drawFillCircle(x + w - 11, y + h / 2, 3, C_BG);
    setTextColor(C_TXT, C_BG);
}

function drawFooter() {
    setTextSize(1);
    setTextColor(C_TXT, C_BG);
    drawString("RF:" + rfHits + "  IR:" + irHits + "  sweep:" + cycles, 4, height() - 28);
    drawString("press a button to stop", 4, height() - 16);
}

function drawScanScreen(active) {
    var W = width();
    var H = height();
    fillScreen(C_BG);
    setTextSize(1);
    setTextColor(C_CYN, C_BG);
    drawString("RF / IR  SPECTRUM DETECTOR", 4, 3);

    var pw = W - 8;
    var ph = 34;
    var rfY = 22;
    var irY = rfY + ph + 4;

    drawPanel(4, rfY, pw, ph, "RF " + rfFreq.toFixed(2) + " MHz",
              active === "RF" || active === "BOTH" ? "LISTENING..." : "on standby",
              active === "RF" || active === "BOTH", C_RF);
    drawPanel(4, irY, pw, ph, "IR  38 kHz",
              active === "IR" || active === "BOTH" ? "LISTENING..." : "on standby",
              active === "IR" || active === "BOTH", C_IR);

    specX = 4;
    specY = irY + ph + 6;
    specW = W - 8;
    specH = H - specY - 32;

    drawFooter();
}

function playSweep(bandColor, frames) {
    var bars = 16;
    var bw = specW / bars;
    var baseY = specY + specH;
    for (var f = 0; f < frames; f++) {
        if (getAnyPress()) break;
        drawFillRect(specX, specY, specW, specH, C_BG);
        for (var i = 0; i < bars; i++) {
            var hgt = Math.floor(specH * (0.22 + 0.68 * Math.abs(Math.sin(f * 0.9 + i * 0.7))));
            if (hgt > specH) hgt = specH;
            drawFillRect(specX + i * bw, baseY - hgt, bw - 2, hgt, (i % 3 === 1) ? bandColor : C_DIM);
        }
        drawFastVLine(specX + Math.floor(specW * ((f % 10) / 10)), specY, specH, C_TXT);
        drawFastHLine(specX, baseY, specW, C_DIM);
        delay(45);
    }
    drawFillRect(specX, specY, specW, specH, C_BG);
    drawFastHLine(specX, baseY, specW, C_DIM);
}

function playSweepDual(frames) {
    var bars = 16;
    var bw = specW / bars;
    var baseY = specY + specH;
    for (var f = 0; f < frames; f++) {
        if (getAnyPress()) break;
        drawFillRect(specX, specY, specW, specH, C_BG);
        for (var i = 0; i < bars; i++) {
            var hgt = Math.floor(specH * (0.22 + 0.68 * Math.abs(Math.sin(f * 0.9 + i * 0.7))));
            if (hgt > specH) hgt = specH;
            var col = (i % 3 === 1) ? C_RF : ((i % 3 === 2) ? C_IR : C_DIM);
            drawFillRect(specX + i * bw, baseY - hgt, bw - 2, hgt, col);
        }
        drawFastVLine(specX + Math.floor(specW * ((f % 10) / 10)), specY, specH, C_TXT);
        drawFastHLine(specX, baseY, specW, C_DIM);
        delay(45);
    }
    drawFillRect(specX, specY, specW, specH, C_BG);
    drawFastHLine(specX, baseY, specW, C_DIM);
}

function drawResult(kind, lines) {
    var H = height();
    fillScreen(C_BG);
    setTextSize(2);
    setTextColor(kind === "RF" ? C_RF : C_IR, C_BG);
    drawString(kind + " SIGNAL!", 4, 4);
    setTextSize(1);
    setTextColor(C_TXT, C_BG);
    var y = 30;
    for (var i = 0; i < lines.length; i++) {
        if (y > H - 24) break;
        drawString(lines[i], 6, y);
        y += 12;
    }
    setTextColor(C_DIM, C_BG);
    drawString("captured - choose action...", 6, H - 16);
}

/* ---------- save ---------- */

function doSave(kind, content) {
    var folder = kind === "RF" ? RF_FOLDER : IR_FOLDER;
    var ext = kind === "RF" ? RF_EXT : IR_EXT;
    var fsName = pickFs();

    if (kind === "IR") content = normalizeIR(content);

    var defName = (kind === "RF" ? "rf_" : "ir_") + (kind === "RF" ? rfHits : irHits);
    var name = keyboardPrompt(defName, 24, "File name:");
    if (name === "") return "rescan";
    name = sanitizeName(name);
    if (name === "") return "rescan";

    var target = uniqueName(fsName, folder, name, ext);
    var ok = saveContent(fsName, folder, target, ext, content);
    if (ok) {
        dialogSuccess("Saved " + folder + "/" + target + ext, true);
    } else {
        dialogError("Save failed on " + fsName + "!", true);
    }
    return "rescan";
}

function writeTemp(fsName, tmpPath, content) {
    try { return storageWrite({fs: fsName, path: tmpPath}, content, "w"); }
    catch (writeErr) { return false; }
}

function replayCapture(kind, content) {
    var fsName = pickFs();
    var ext = kind === "RF" ? RF_EXT : IR_EXT;
    var tmp = "/_replay" + ext;
    if (kind === "IR") content = normalizeIR(content);
    if (!writeTemp(fsName, tmp, content)) {
        dialogError("Replay write failed", true);
        return;
    }
    var ok = false;
    if (kind === "RF") ok = subghzTransmitFile(tmp, true);
    else ok = irTransmitFile(tmp, true);
    try { storageRemove({fs: fsName, path: tmp}); } catch (removeErr) {}
    if (ok) dialogSuccess("Replayed " + kind + " signal", true);
    else dialogError("Replay failed", true);
}

function handleCapture(kind, content) {
    var lines = kind === "RF" ? describeRF(content) : describeIR(content);
    drawResult(kind, lines);
    delay(1200);

    while (true) {
        var actions = {};
        actions["Save to /Bruce" + kind] = "save";
        actions["Replay it"] = "replay";
        actions["Discard + rescan"] = "rescan";
        actions["Exit"] = "exit";

        var ch = dialogChoice(actions);
        if (ch === "save") return doSave(kind, content);
        if (ch === "replay") { replayCapture(kind, content); continue; }
        if (ch === "exit") return "exit";
        return "rescan";
    }
}

/* ---------- radio phases ---------- */

function dualPhase() {
    // Listens to RF and IR at the same time. Decode pass first (when either
    // band is in Auto mode), then a RAW pass for any band that is set to Raw
    // or that the decoder missed. readDual returns {rf, ir} strings.
    var bothRaw = (rfMode === "Raw" && irMode === "Raw");

    var res = subghzReadDual(listenWin, bothRaw);
    var rf = res.rf;
    var ir = res.ir;

    var needRaw = false;
    if (!bothRaw) {
        if (rfMode === "Raw") needRaw = true;
        if (irMode === "Raw") needRaw = true;
        if (rfMode === "Auto" && rf === "") needRaw = true;
        if (irMode === "Auto" && ir === "") needRaw = true;
    }
    if (needRaw) {
        var res2 = subghzReadDual(listenWin, true);
        if (rf === "" && res2.rf !== "") rf = res2.rf;
        if (ir === "" && res2.ir !== "") ir = res2.ir;
    }
    return { rf: rf, ir: ir };
}

function detectorLoop() {
    rfHits = 0;
    irHits = 0;
    cycles = 0;
    while (true) {
        cycles++;
        drawScanScreen("BOTH");
        playSweepDual(12);
        if (getAnyPress()) break;

        var res = dualPhase();
        var action = "";
        if (res.rf !== "") {
            rfHits++;
            action = handleCapture("RF", res.rf);
            if (action === "exit") break;
        }
        if (res.ir !== "") {
            irHits++;
            action = handleCapture("IR", res.ir);
            if (action === "exit") break;
        }
        if (getAnyPress()) break;
    }
}

/* ---------- menus ---------- */

function freqMenu() {
    var opts = {};
    for (var i = 0; i < FREQS.length; i++) {
        opts[FREQS[i].toFixed(3) + " MHz"] = String(FREQS[i]);
    }
    opts["Manual entry"] = "manual";
    var ch = dialogChoice(opts);
    if (ch === "manual") {
        var v = numKeyboard(rfFreq.toFixed(3), 8, "Freq MHz");
        if (v !== "") {
            var f = parseFloat(v);
            if (f > 0) rfFreq = f;
        }
    } else if (ch !== "") {
        rfFreq = parseFloat(ch);
    }
}

function winMenu() {
    var opts = {};
    opts["1 second"] = "1";
    opts["2 seconds"] = "2";
    opts["3 seconds"] = "3";
    opts["4 seconds"] = "4";
    opts["5 seconds"] = "5";
    var ch = dialogChoice(opts);
    if (ch !== "") listenWin = parseInt(ch, 10);
}

function modeMenu(which) {
    var opts = {};
    opts["Auto (decode, then RAW)"] = "auto";
    opts["RAW only (faster cycle)"] = "raw";
    var ch = dialogChoice(opts);
    if (which === "RF") {
        if (ch === "auto") rfMode = "Auto";
        else if (ch === "raw") rfMode = "Raw";
    } else {
        if (ch === "auto") irMode = "Auto";
        else if (ch === "raw") irMode = "Raw";
    }
}

function storageMenu() {
    var opts = {};
    opts["Auto (SD if available)"] = "auto";
    opts["SD card"] = "sd";
    opts["LittleFS (internal)"] = "littlefs";
    var ch = dialogChoice(opts);
    if (ch === "auto") storagePref = "Auto";
    else if (ch === "sd") storagePref = "sd";
    else if (ch === "littlefs") storagePref = "littlefs";
}

function mainMenu() {
    var opts = {};
    opts["Freq: " + rfFreq.toFixed(3) + " MHz"] = "freq";
    opts["Listen window: " + listenWin + " s"] = "win";
    opts["RF mode: " + rfMode] = "rfmode";
    opts["IR mode: " + irMode] = "irmode";
    opts["Storage: " + storagePref] = "storage";
    opts["Start detector"] = "start";
    opts["Exit"] = "exit";
    return dialogChoice(opts);
}

/* ---------- main ---------- */

dialogInfo("RF/IR detector. Listens to both bands at the same time. Point the remote at the device and press its buttons. Captures are saved to /BruceRF or /BruceIR.", true);

while (true) {
    fillScreen(C_BG);
    var ch = mainMenu();
    if (ch === "" || ch === "exit") break;
    if (ch === "freq") freqMenu();
    else if (ch === "win") winMenu();
    else if (ch === "rfmode") modeMenu("RF");
    else if (ch === "irmode") modeMenu("IR");
    else if (ch === "storage") storageMenu();
    else if (ch === "start") {
        fillScreen(C_BG);
        detectorLoop();
        fillScreen(C_BG);
    }
}
