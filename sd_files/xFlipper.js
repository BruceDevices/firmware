
let eventLoop = require("event_loop");
let gui = require("gui");

let loadingView = require("gui/loading");
let submenuView = require("gui/submenu");
let emptyView = require("gui/empty_screen");
let textInputView = require("gui/text_input");
let byteInputView = require("gui/byte_input");
let textBoxView = require("gui/text_box");
let dialogView = require("gui/dialog");
let filePicker = require("gui/file_picker");
let icon = require("gui/icon");
let flipper = require("flipper");

let press =0;
let serial = require("serial");

serial.setup("usart", 115200);


function SerialCMD(cmd) {
    serial.write(cmd);
}



// declare view instances
let views = {
    loading: loadingView.make(),
    empty: emptyView.make(),
    keyboard: textInputView.makeWith({
        header: "",
        minLength: 0,
        maxLength: 32,
        defaultText: flipper.getName(),
        defaultTextClear: true,
    }),
    helloDialog: dialogView.make(),
    bytekb: byteInputView.makeWith({
    }),
    longText: textBoxView.makeWith({
        text: "",
    }),
   xmenu: submenuView.makeWith({
        header: "xFlipper",
        items: [
            "Controller",
            "Terminal",
            "Send Command",
            "Exit app",
        ],
    }),
};

// Enable illegal filename symbols since we're not choosing filenames, gives more flexibility
// Not available in all firmwares, good idea to check if it is supported
if (doesSdkSupport(["gui-textinput-illegalsymbols"])) {
    views.keyboard.set("illegalSymbols", true);
}

views.helloDialog.set("text", "hshax ");

    views.helloDialog.set("left", "<<");
    views.helloDialog.set("center", "SELECT");
    views.helloDialog.set("right", ">>");

// selector
eventLoop.subscribe(views.xmenu.chosen, function (_sub, index, gui, eventLoop, views) {
    if (index === 0) {
        gui.viewDispatcher.switchTo(views.helloDialog);
    } else if (index === 1) {
        gui.viewDispatcher.switchTo(views.empty);
    } else if (index === 2) {
        gui.viewDispatcher.switchTo(views.empty);
    } else if (index === 3) {
        eventLoop.stop();
    }
}, gui, eventLoop, views);



// go back after the greeting dialog
eventLoop.subscribe(views.helloDialog.input, function (_sub, button, gui, views) {

    if (button === "left" && press === 0) {
        press =2;
        serial.write("nav prev\n");
        delay(100);
    } else if (button === "center" && press === 0) {
        press =2;
        serial.write("nav sel\n");
        delay(100);

    } else if (button === "right" && press === 0) {
        press =2;
        serial.write("nav next\n");
        delay(100);
    } else {
        serial.write(button);
        serial.write("\n");
        delay(100);
    }

    if (press === 2) { press=0; }

}, gui, views);

//  when the back key is pressed
eventLoop.subscribe(gui.viewDispatcher.navigation, function (_sub, _, gui, views, eventLoop) {
    if (gui.viewDispatcher.currentView === views.xmenu) {
        serial.end();
        eventLoop.stop();
        return;
    }
    gui.viewDispatcher.switchTo(views.xmenu);
}, gui, views, eventLoop);

// run UI
gui.viewDispatcher.switchTo(views.xmenu);
eventLoop.run();
