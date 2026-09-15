var display = require('display');
var keyboardApi = require('keyboard');
var audio = require('audio');

var keyboardPrompt = keyboardApi.keyboard;
var tone = audio.tone;
var print = display.print;


// generate a DTMF signal from a number string  https://en.wikipedia.org/wiki/DTMF

const dtmfTable = {
    '1': { lowFreq: 697, highFreq: 1209 },
    '2': { lowFreq: 697, highFreq: 1336 },
    '3': { lowFreq: 697, highFreq: 1477 },
    'A': { lowFreq: 697, highFreq: 1633 },
    '4': { lowFreq: 770, highFreq: 1209 },
    '5': { lowFreq: 770, highFreq: 1336 },
    '6': { lowFreq: 770, highFreq: 1477 },
    'B': { lowFreq: 770, highFreq: 1633 },
    '7': { lowFreq: 852, highFreq: 1209 },
    '8': { lowFreq: 852, highFreq: 1336 },
    '9': { lowFreq: 852, highFreq: 1477 },
    'C': { lowFreq: 852, highFreq: 1633 },
    '*': { lowFreq: 941, highFreq: 1209 },
    '0': { lowFreq: 941, highFreq: 1336 },
    '#': { lowFreq: 941, highFreq: 1477 },
    'D': { lowFreq: 941, highFreq: 1633 }
};

var s = keyboardPrompt("0", 99, "Enter number");

for (var i = 0; i < s.length; i++) {
    if (!(s[i] in dtmfTable)) {
        print("invalid digit (skipped): " + s[i]);
        continue;
    }
    // else
    var curr_tone = dtmfTable[s[i]];
    tone(curr_tone.lowFreq, 500);
    tone(curr_tone.highFreq, 500);
}
