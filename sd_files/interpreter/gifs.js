var display = require('display');
var serialApi = require('serial');

var println = display.println;
var drawFillRect = display.drawFillRect;
var color = display.color;
var gifOpen = display.gifOpen;
var serialPrintln = serialApi.println;

function main() {
  // gifOpen - Returns GIF. If it returns null, it means there was an error opening the file.
  const patrick = gifOpen('littlefs', '/gifs/half_patrick.gif');
  const spongebob = gifOpen('littlefs', '/gifs/half_spongebob.gif');
  const rick = gifOpen('littlefs', '/gifs/half_rick.gif');
  const rainbow = gifOpen('littlefs', '/gifs/half_rainbow.gif');

  // Check if all GIFs failed to load
  if (!patrick && !spongebob && !rick && !rainbow) {
    println("Cannot load gifs");
    println("Make sure that they are in /gifs folder");
    delay(3000);
    throw new Error("Cannot load gifs");
  }

  var elapsed;
  var frames = 0;
  var fps = 0;
  var startTime = now();

  while(true) {
    patrick.playFrame(0, 0); // Plays a frame of the GIF at the specified position.
    spongebob.playFrame(60, 0);
    rick.playFrame(120, 0);
    rainbow.playFrame(180, 0);

    frames++;
    if (frames % 10 === 0) {
      elapsed = (now() - startTime) / 1000;
      fps = frames / elapsed;
      serialPrintln(fps);
    }

    // To remove gif you can use:
    // const dimensions = rainbow.dimensions(); // Retrieves the dimensions of the Rainbow GIF.
    // drawFillRect(x + 120, y, dimensions.width, dimensions.height, color(0, 0, 0)); // Clears the previous frame.
    // rainbow.close(); // Closes the Rainbow GIF to release resources.

    // patrick.playFrame(x, y);
    // You can move GIFs by changing x and y. Ensure you clear the old position
    // with drawFillRect(x, y, dimensions.width, dimensions.height, color(0, 0, 0))
    // and patrick.reset() it because GIFs are often optimized to draw only the pixels that change between frames.
  
    if (getAnyPress()) {
      break; // Exits the loop when a button is pressed.
    }
  }
}

main();
