"use strict";

// dist/index.js
var __importDefault =
	(exports && exports.__importDefault) ||
	function (mod) {
		return mod && mod.__esModule ? mod : { default: mod };
	};
Object.defineProperty(exports, "__esModule", { value: true });
var display_1 = __importDefault(require("display"));
var keyboard_1 = __importDefault(require("keyboard"));
__allow("wifi");
var wifi_1 = __importDefault(require("wifi"));
var PRICE_UPDATE_INTERVAL = 2e4;
var ESC_CHECK_INTERVAL = 10;
var logoWidth = 172;
var logoHeight = 46;
var coinGeckoLogo = new Uint8Array([
	0, 0, 254, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 192, 255, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 240, 3, 248, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 124, 0, 128, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30, 0, 0,
	31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 192, 3, 0, 0, 112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224,
	1, 0, 0, 224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224, 0, 0, 0, 192, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 112, 0, 0, 0, 128, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 56, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 255, 7, 0, 7,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 192, 255, 15, 0, 6, 0, 0, 8, 0, 0, 0, 0, 0, 8,
	0, 0, 0, 0, 0, 0, 0, 12, 240, 255, 31, 0, 14, 0, 0, 127, 0, 0, 30, 0, 128, 127, 0, 0, 0, 224, 0,
	0, 0, 14, 248, 195, 127, 0, 12, 0, 192, 255, 1, 0, 14, 0, 192, 255, 0, 0, 0, 224, 0, 0, 0, 6, 252,
	188, 255, 7, 28, 0, 224, 247, 1, 0, 14, 0, 224, 245, 1, 0, 0, 224, 0, 0, 0, 7, 254, 62, 255, 31,
	28, 0, 224, 192, 3, 0, 0, 0, 240, 192, 1, 0, 0, 224, 0, 0, 0, 3, 254, 255, 255, 63, 24, 0, 240,
	128, 131, 15, 140, 121, 112, 192, 131, 15, 248, 225, 240, 240, 1, 7, 255, 126, 255, 127, 24, 0,
	112, 128, 227, 63, 206, 255, 120, 128, 195, 31, 252, 227, 120, 252, 7, 3, 255, 190, 255, 255, 56,
	0, 120, 0, 224, 63, 142, 255, 57, 0, 224, 63, 254, 231, 60, 252, 7, 3, 255, 221, 255, 255, 48, 0,
	112, 0, 112, 112, 142, 195, 57, 88, 242, 120, 14, 231, 30, 14, 14, 3, 255, 227, 255, 251, 48, 0,
	120, 0, 120, 112, 206, 195, 57, 248, 115, 112, 7, 238, 15, 14, 14, 3, 255, 255, 255, 251, 56, 0,
	120, 0, 112, 240, 206, 195, 57, 248, 243, 127, 7, 224, 15, 15, 14, 131, 255, 255, 255, 127, 48, 0,
	112, 128, 123, 224, 142, 195, 121, 128, 243, 127, 7, 224, 31, 7, 14, 131, 255, 255, 255, 63, 48,
	0, 112, 128, 123, 240, 206, 195, 121, 128, 115, 4, 7, 226, 29, 15, 14, 131, 255, 255, 255, 63, 48,
	0, 240, 192, 115, 112, 206, 195, 113, 192, 115, 112, 7, 239, 60, 14, 14, 131, 255, 255, 255, 15,
	56, 0, 224, 225, 241, 120, 142, 193, 241, 225, 243, 120, 14, 231, 56, 30, 15, 131, 255, 255, 247,
	7, 24, 0, 192, 255, 225, 63, 206, 195, 225, 255, 227, 63, 254, 227, 120, 252, 7, 135, 255, 255,
	255, 7, 24, 0, 128, 255, 192, 31, 206, 195, 193, 127, 195, 31, 252, 227, 240, 248, 3, 134, 255,
	255, 255, 1, 24, 0, 0, 62, 128, 15, 140, 129, 1, 31, 131, 7, 240, 160, 160, 240, 1, 198, 255, 255,
	255, 1, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 206, 255, 255, 255, 0, 12, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 206, 255, 255, 127, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 220, 255, 255, 63, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 255,
	255, 63, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 255, 255, 31, 0, 3, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 240, 255, 255, 15, 128, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 240, 255, 255, 15, 192, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224,
	255, 255, 7, 224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 192, 255, 255, 7, 112, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 255, 255, 7, 56, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 7, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	252, 255, 135, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 255, 247, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 192, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 255, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
]);
function clearScreen() {
	display_1.default.fill(display_1.default.color(0, 0, 0));
}
function printCoingeckoLogo() {
	display_1.default.drawXBitmap(
		15,
		125,
		coinGeckoLogo,
		logoWidth,
		logoHeight,
		display_1.default.color(255, 255, 255)
	);
}
function showCoinsPrice() {
	while (true) {
		if (keyboard_1.default.getEscPress()) break;
		var COINS = [
			{ name: "bitcoin", symbol: "BTC" },
			{
				name: "ethereum",
				symbol: "ETH",
			},
			{
				name: "litecoin",
				symbol: "LTC",
			},
		];
		var prices = {};
		for (var _i = 0, COINS_1 = COINS; _i < COINS_1.length; _i++) {
			var coin = COINS_1[_i];
			prices[coin.name] = "failed";
		}
		try {
			var response = wifi_1.default.httpFetch(
				"https://api.coingecko.com/api/v3/simple/price?ids=" +
					COINS.map(function (coin2) {
						return coin2.name;
					}).join(",") +
					"&vs_currencies=usd",
				{
					method: "GET",
					headers: { "Content-Type": "application/json" },
				}
			);
			var json = JSON.parse(to_string(response.body));
			for (var _a = 0, COINS_2 = COINS; _a < COINS_2.length; _a++) {
				var coin = COINS_2[_a];
				if (json[coin.name] && json[coin.name].usd !== void 0) {
					prices[coin.name] = json[coin.name].usd;
				}
			}
		} catch (error) {}
		clearScreen();
		display_1.default.setTextSize(3);
		var y = 15;
		for (var _b = 0, COINS_3 = COINS; _b < COINS_3.length; _b++) {
			var coin = COINS_3[_b];
			var price = prices[coin.name];
			display_1.default.drawText("".concat(coin.symbol, ": ").concat(price, " $"), 15, y);
			y += 35;
		}
		printCoingeckoLogo();
		var waited = 0;
		while (waited < PRICE_UPDATE_INTERVAL) {
			if (keyboard_1.default.getEscPress()) return;
			delay(ESC_CHECK_INTERVAL);
			waited += ESC_CHECK_INTERVAL;
		}
	}
}
function showNotConnected() {
	wifi_1.default.connectDialog();
	if (!wifi_1.default.connected()) {
		display_1.default.drawText("Not connected.", 5, 5);
	}
}
function main() {
	var isWifiConnected = wifi_1.default.connected();
	if (!isWifiConnected) {
		showNotConnected();
	}
	showCoinsPrice();
	while (!keyboard_1.default.getEscPress()) {
		delay(ESC_CHECK_INTERVAL);
	}
}
main();
