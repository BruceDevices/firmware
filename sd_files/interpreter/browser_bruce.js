var dialog = require('dialog');
__allow("wifi");
var wifi = require('wifi');
var display = require('display');
var keyboard = require('keyboard');

var tftWidth = display.width();
var tftHeight = display.height();

var request = { body: '' };

function drawWindow(title) {
  display.fill(0);
  display.drawRoundRect(
    5,
    5,
    tftWidth - 10,
    tftHeight - 10,
    5,
    BRUCE_PRICOLOR
  );
  display.setTextSize(2);
  display.setTextAlign('center', 'top');
  display.drawText(
    title.length > 20 ? title.substring(0, 20) : title,
    tftWidth / 2,
    5
  );
  display.setTextAlign('left', 'top');
  display.drawText('loading...', 20, 40);
}

var textViewer = dialog.createTextViewer(request.body, {
  fontSize: 1,
  startX: 10,
  startY: 25,
  width: tftWidth - 2 * 10,
  height: tftHeight - 25 - 10,
  indentWrappedLines: true,
});

var history = [];

function goToPage(url) {
  console.log(url);
  drawWindow(url.substring(url.indexOf('://') + 3));
  textViewer.clear();
  try {
    request = wifi.httpFetch(
      'https://www.w3.org/services/html2txt?url='.concat(
        url,
        '&noinlinerefs=on&endrefs=on'
      ),
      {
        method: 'GET',
      }
    );
  } catch (error) {
    console.log(JSON.stringify(error));
    request.body = 'error\n\n';
  }
  history.push(url);
  textViewer.setText(request.body);
}

/// TODO: Use storage.write('browser.js', 'https://newsite.com,\n  ', 'append', '// insert websites here') to add new websites
var websites = [
  'https://github.com/pr3y/Bruce/wiki',
  'https://en.cppreference.com/w',
  'https://randomnerdtutorials.com',
  // insert websites here
];

function selectWebsite() {
  drawWindow('Select Website');
  var websitesChoice = {};
  for (var index = 0; index < websites.length; index++) {
    var website = websites[index];
    websitesChoice[website.substring(website.indexOf('://') + 3)] = website;
  }
  if (request.status) {
    websitesChoice['Cancel'] = 'Cancel';
  }
  websitesChoice['Quit'] = 'Quit';
  return dialog.choice(websitesChoice);
}

function selectSection() {
  var websitesSections = {};
  var getMaxLines = textViewer.getMaxLines();
  for (var index = 0; index < getMaxLines; index++) {
    var textVieverLine = textViewer.getLine(index);
    if (textVieverLine[0] !== ' ' && textVieverLine.length) {
      websitesSections[textVieverLine] = String(index);
    }
  }
  websitesSections['Cancel'] = 'Cancel';
  var choice = dialog.choice(websitesSections);
  return choice === 'Cancel' ? -1 : parseInt(choice, 10);
}

function main() {
  var _a;
  var url = selectWebsite();
  if (url === 'Quit') {
    return;
  }
  goToPage(url);
  while (true) {
    if (keyboard.getSelPress()) {
      var visibleText = textViewer.getVisibleText();
      var choicesMatch = [];
      choicesMatch.push('Go To Selection');
      choicesMatch.push.apply(
        choicesMatch,
        visibleText.match(/\[\d+\][^\s\[,\]]*/g) || []
      );
      choicesMatch.push('Go Back');
      choicesMatch.push('Select Website');
      choicesMatch.push('Cancel');
      choicesMatch.push('Quit');
      var choice = dialog.choice(choicesMatch);
      if (choice === 'Quit') {
        break;
      }
      if (choice === 'Cancel') {
        drawWindow(url.substring(url.indexOf('://') + 3));
        continue;
      }
      if (choice === 'Go Back') {
        if (history.length > 1) {
          history.pop();
          var newUrl = history.pop();
          if (newUrl) {
            url = newUrl;
            console.log('url:', url);
            goToPage(url);
          }
        }
        drawWindow(url.substring(url.indexOf('://') + 3));
        continue;
      }
      if (choice === 'Go To Selection') {
        var line = selectSection();
        drawWindow(url.substring(url.indexOf('://') + 3));
        if (line === -1) {
          continue;
        }
        console.log('Go to line:', line);
        textViewer.scrollToLine(line);
        continue;
      }
      var chosenUrl = '';
      if (choice === 'Select Website') {
        chosenUrl = selectWebsite();
        if (chosenUrl === 'Cancel') {
          drawWindow(url.substring(url.indexOf('://') + 3));
          continue;
        }
        if (chosenUrl === 'Quit') {
          return;
        }
      } else {
        // Typescript magic. this syntax: choice.match(/\d+/)?.[0] was translated to this:
        // optional chain is not supported in bruce js (now)
        var searchTextIndex = request.body.indexOf(
          ' ' +
            ((_a = choice.match(/\d+/)) === null || _a === void 0
              ? void 0
              : _a[0]) +
            '. http'
        );
        var searchedUrl = request.body.substring(
          searchTextIndex,
          request.body.indexOf('\n', searchTextIndex + 1)
        );
        chosenUrl = searchedUrl.substring(searchedUrl.indexOf('.') + 2);
      }
      if (chosenUrl !== '') {
        url = chosenUrl;
        goToPage(chosenUrl);
      }
    }
    if (keyboard.getPrevPress()) {
      textViewer.scrollUp();
    }
    if (keyboard.getNextPress()) {
      textViewer.scrollDown();
    }
    textViewer.draw();
    delay(100);
  }
}
main();
