<script lang="ts">
	import { base } from '$app/paths';
	import Btn from '$lib/components/Btn.svelte';
	import InfoRow from '$lib/components/InfoRow.svelte';
	import Icon from '$lib/components/Icon.svelte';
	import { current_page, Page } from '$lib/store';
	import { bytesToKB, bytesToMegabytes, capitalize } from '$lib/helper';
	import { readSerialPort, requestSerialPort, writeToPort } from '$lib/serial_helper';
	import { browser } from '$app/environment';
	import { onDestroy, onMount } from 'svelte';
	$current_page = Page.MyBruce;

	let port: SerialPort;
	const baud_rate = 115200;
	let connected = $state(false);
	let supported = $state(false);

	onMount(() => {
		if (!browser) return;
		supported = 'serial' in navigator;
		window.addEventListener('keydown', handleKeydown);
		return () => window.removeEventListener('keydown', handleKeydown);
	});

	onDestroy(() => {
		if (!browser) return;
		void stopNavigator();
	});

	let sdCard = $state('');
	let littleFS_Storage = $state('');
	let device = $state('');
	let version = $state('');
	let sdk = $state('');
	let mac_address = $state('');
	let wifi = $state('');
	let uptime = $state('');
	let heap_usage = $state('');
	let psram_usage = $state('');

	let loading = $state(false);
	let navigatorOpen = $state(false);
	let autoReloadMs = $state('0');
	let navCanvas = $state<HTMLCanvasElement | null>(null);

	const textDecoder = new TextDecoder();
	let navReader: ReadableStreamDefaultReader<Uint8Array> | null = null;
	let keepReading = false;
	let captureDump: { buffer: string } | null = null;
	let autoReloadInterval: ReturnType<typeof setInterval> | null = null;
	let rxBuffer = new Uint8Array();

	function parseDeviceInfo(response: string) {
		const lines = response.split('\n');
		console.log(lines);
		if (lines[0].includes('COMMAND')) {
			version = `${lines[1]} - ${lines[2]}`;
		} else {
			version = `${lines[0]} - ${lines[1]}`;
		}
		for (const line of lines) {
			if (line.includes('SDK')) {
				sdk = line.replace('SDK: ', '');
			}
			if (line.includes('MAC addr')) {
				mac_address = line.replace('MAC addr:', '');
			}
			if (line.includes('Wifi')) {
				wifi = capitalize(line.replace('Wifi: ', ''));
			}
			if (line.includes('Device')) {
				device = line.replace('Device: ', '');
			}
		}
	}

	function parseUptime(response: string) {
		const lines = response.split('\n');
		for (const line of lines) {
			if (line.includes('Uptime')) {
				uptime = line.replace('Uptime: ', '');
			}
		}
	}

	function parseFree(response: string) {
		const lines = response.split('\n');
		if (lines.length >= 3) {
			// replace method extract the numbers
			let total_heap = 0;
			let free_heap = 0;
			let total_psram = 0;
			let free_psram = 0;
			for (const line of lines) {
				if (line.includes('Total heap')) {
					total_heap = +line.replace(/\D/g, '');
				}
				if (line.includes('Free heap')) {
					free_heap = +line.replace(/\D/g, '');
				}
				if (line.includes('Total PSRAM')) {
					total_psram = +line.replace(/\D/g, '');
				}
				if (line.includes('Free PSRAM')) {
					free_psram = +line.replace(/\D/g, '');
				}
			}
			heap_usage = `${bytesToKB(total_heap - free_heap)}/${bytesToKB(total_heap)} KB`;

			if (response.includes('PSRAM')) {
				psram_usage = `${bytesToMegabytes(total_psram - free_psram)}/${bytesToMegabytes(free_psram)} MB`;
			} else {
				psram_usage = `Not Available`;
			}
		}
	}

	function parseFreeStorage(response: string) {
		console.log(response);
		const lines = response.split('\n');
		let total = 0;
		let free = 0;
		for (const line of lines) {
			if (line.includes('Total')) {
				total = +line.replace(/\D/g, '');
			}
			if (line.includes('Free')) {
				free = +line.replace(/\D/g, '');
			}
		}

		if (response.includes('[E][sd_diskio.cpp')) {
			sdCard = 'Not Installed';
		} else if (response.includes('SD Total space')) {
			sdCard = `${bytesToMegabytes(+free)} / ${bytesToMegabytes(+total)} MB`;
		} else if (response.includes('LittleFS Total space')) {
			littleFS_Storage = `${bytesToMegabytes(+free)} / ${bytesToMegabytes(+total)} MB`;
		}
	}

	async function get_info() {
		await writeToPort(port, new TextEncoder().encode('info'));
	}

	async function get_uptime() {
		await writeToPort(port, new TextEncoder().encode('uptime'));
	}

	async function get_free() {
		await writeToPort(port, new TextEncoder().encode('free'));
	}

	async function get_sd_store() {
		await writeToPort(port, new TextEncoder().encode('storage free sd'));
	}

	async function get_littlefs_usage() {
		await writeToPort(port, new TextEncoder().encode('storage free littlefs'));
	}

	async function connect_device() {
		try {
			loading = true;
			port = await requestSerialPort(baud_rate);
			if (port) {
				connected = true;

				await get_sd_store();
				await readSerialPort(port, parseFreeStorage);

				await get_littlefs_usage();
				await readSerialPort(port, parseFreeStorage);

				// in the first time it is not getting the SD Info
				await get_sd_store();
				await readSerialPort(port, parseFreeStorage);

				await get_info();
				await readSerialPort(port, parseDeviceInfo);

				await get_uptime();
				await readSerialPort(port, parseUptime);

				await get_free();
				await readSerialPort(port, parseFree);

				get_device_image();
			}
			loading = false;
		} catch (error) {
			console.error(error);
			alert('Some error occured during communication with device');
		}
	}

	async function factory_reset() {
		if (confirm('Are you sure? Factory reset will reset the content of Bruce.conf')) {
			await writeToPort(port, new TextEncoder().encode('factory_reset'));
		}
	}

	async function reboot_bruce() {
		if (confirm('Are you sure?')) {
			await writeToPort(port, new TextEncoder().encode('power reboot'));
		}
	}

	async function sendNavigatorCommand(command: string) {
		if (!port) return;
		await writeToPort(port, new TextEncoder().encode(`${command}\n`));
	}

	function concatBuffer(a: Uint8Array, b: Uint8Array) {
		const c = new Uint8Array(a.length + b.length);
		c.set(a, 0);
		c.set(b, a.length);
		return c;
	}

	async function readLoop() {
		if (!navReader) return;
		try {
			while (keepReading && navReader) {
				const { value, done } = await navReader.read();
				if (done) break;
				if (!value) continue;
				rxBuffer = concatBuffer(rxBuffer, value);
				while (rxBuffer.length) {
					if (rxBuffer[0] === 0xaa) {
						if (rxBuffer.length < 2) break;
						const size = rxBuffer[1];
						if (rxBuffer.length < size) break;
						const packet = rxBuffer.slice(0, size);
						rxBuffer = rxBuffer.slice(size);
						await renderTft(packet);
						continue;
					}
					const nextIdx = rxBuffer.indexOf(0xaa);
					let textBytes: Uint8Array;
					if (nextIdx === -1) {
						textBytes = rxBuffer;
						rxBuffer = new Uint8Array();
					} else {
						textBytes = rxBuffer.slice(0, nextIdx);
						rxBuffer = rxBuffer.slice(nextIdx);
					}
					const chunkText = textDecoder.decode(textBytes);
					if (captureDump) {
						captureDump.buffer += chunkText;
						if (captureDump.buffer.includes('[End of Dump]')) {
							const buf = captureDump.buffer;
							captureDump = null;
							try {
								const bytes = parseDumpToBytes(buf);
								await renderTft(bytes);
							} catch (error) {
								console.error('Failed to parse dump', error);
							}
						}
					}
					if (nextIdx === -1) break;
				}
			}
		} catch (error) {
			console.error('Error reading serial port', error);
		}
	}

	function startAutoReload() {
		stopAutoReload();
		const ms = parseInt(autoReloadMs || '0', 10);
		if (ms > 0 && navigatorOpen) {
			autoReloadInterval = setInterval(() => void triggerDump(), ms);
		}
	}

	function stopAutoReload() {
		if (autoReloadInterval) {
			clearInterval(autoReloadInterval);
			autoReloadInterval = null;
		}
	}

	async function startNavigator() {
		if (!connected || !port) {
			alert('Connect first');
			return;
		}
		if (!navReader) {
			navReader = port.readable.getReader();
		}
		keepReading = true;
		void readLoop();
		navigatorOpen = true;
		await sendNavigatorCommand('display start');
		await sendNavigatorCommand('nav next');
		await sendNavigatorCommand('nav prev');
		startAutoReload();
	}

	async function stopNavigator() {
		if (!navigatorOpen) return;
		navigatorOpen = false;
		stopAutoReload();
		await sendNavigatorCommand('display stop');
		keepReading = false;
		if (navReader) {
			try {
				await navReader.cancel();
			} catch {}
			navReader.releaseLock();
			navReader = null;
		}
	}

	async function triggerDump() {
		if (!port) {
			alert('Connect first');
			return;
		}
		captureDump = { buffer: '' };
		await sendNavigatorCommand('display dump');
	}

	function parseDumpToBytes(text: string) {
		if (typeof text !== 'string') throw new TypeError('Input must be a string');
		const startIdx = text.indexOf('AA');
		let cleaned = startIdx >= 0 ? text.slice(startIdx) : text;
		cleaned = cleaned.replace(/\[End of Dump\][\s\S]*$/i, '');
		const hex = cleaned.replace(/[^0-9A-Fa-f]/g, '');
		if (hex.length < 2) throw new Error('No bytes found');
		const evenLen = hex.length & ~1;
		if (evenLen === 0) throw new Error('No complete byte found');
		const arr = new Uint8Array(evenLen / 2);
		for (let i = 0, j = 0; i < evenLen; i += 2, j++) {
			arr[j] = parseInt(hex.substr(i, 2), 16);
		}
		return arr;
	}

	async function renderTft(data: Uint8Array) {
		if (!navCanvas) return;
		const ctx = navCanvas.getContext('2d');
		if (!ctx) return;

		const color565toCss = (color565: number) => {
			const r = ((color565 >> 11) & 0x1f) * (255 / 31);
			const g = ((color565 >> 5) & 0x3f) * (255 / 63);
			const b = (color565 & 0x1f) * (255 / 31);
			return `rgb(${r},${g},${b})`;
		};

		const drawRoundRect = (ctx: CanvasRenderingContext2D, input: { x: number; y: number; w: number; h: number; r: number }, fill: boolean) => {
			const { x, y, w, h, r } = input;
			ctx.beginPath();
			ctx.moveTo(x + r, y);
			ctx.arcTo(x + w, y, x + w, y + h, r);
			ctx.arcTo(x + w, y + h, x, y + h, r);
			ctx.arcTo(x, y + h, x, y, r);
			ctx.arcTo(x, y, x + w, y, r);
			ctx.closePath();
			if (fill) ctx.fill();
			else ctx.stroke();
		};

		let startData = 0;
		const getByteValue = (dataType: string) => {
			if (dataType === 'int8') return data[startData++];
			if (dataType === 'int16') {
				const value = (data[startData] << 8) | data[startData + 1];
				startData += 2;
				return value;
			}
			if (dataType.startsWith('s')) {
				const strLength = parseInt(dataType.substring(1));
				const strBytes = data.slice(startData, startData + strLength);
				startData += strLength;
				return new TextDecoder().decode(strBytes);
			}
		};

		const byteToObject = (fn: number, size: number) => {
			const keysMap: Record<number, string[]> = {
				0: ['fg'],
				1: ['x', 'y', 'w', 'h', 'fg'],
				2: ['x', 'y', 'w', 'h', 'fg'],
				3: ['x', 'y', 'w', 'h', 'r', 'fg'],
				4: ['x', 'y', 'w', 'h', 'r', 'fg'],
				5: ['x', 'y', 'r', 'fg'],
				6: ['x', 'y', 'r', 'fg'],
				7: ['x', 'y', 'x2', 'y2', 'x3', 'y3', 'fg'],
				8: ['x', 'y', 'x2', 'y2', 'x3', 'y3', 'fg'],
				9: ['x', 'y', 'rx', 'ry', 'fg'],
				10: ['x', 'y', 'rx', 'ry', 'fg'],
				11: ['x', 'y', 'x1', 'y1', 'fg'],
				12: ['x', 'y', 'r', 'ir', 'startAngle', 'endAngle', 'fg', 'bg'],
				13: ['x', 'y', 'bx', 'by', 'wd', 'fg', 'bg'],
				14: ['x', 'y', 'size', 'fg', 'bg', 'txt'],
				15: ['x', 'y', 'size', 'fg', 'bg', 'txt'],
				16: ['x', 'y', 'size', 'fg', 'bg', 'txt'],
				17: ['x', 'y', 'size', 'fg', 'bg', 'txt'],
				18: ['x', 'y', 'center', 'ms', 'fs', 'file'],
				20: ['x', 'y', 'h', 'fg'],
				21: ['x', 'y', 'w', 'fg'],
				99: ['w', 'h', 'rotation']
			};

			const result: Record<string, number | string> = {};
			let lengthLeft = size - 3;
			for (const key of keysMap[fn] || []) {
				if (['txt', 'file'].includes(key)) {
					result[key] = getByteValue(`s${lengthLeft}`) as string;
				} else if (['rotation', 'fs'].includes(key)) {
					lengthLeft -= 1;
					const value = getByteValue('int8') as number;
					result[key] = key === 'fs' ? (value === 0 ? 'SD' : 'FS') : value;
				} else {
					lengthLeft -= 2;
					result[key] = getByteValue('int16') as number;
				}
			}
			return result;
		};

		let offset = 0;
		while (offset < data.length) {
			ctx.beginPath();
			if (data[offset] !== 0xaa) break;
			startData = offset + 1;
			const size = getByteValue('int8') as number;
			const fn = getByteValue('int8') as number;
			offset += size;
			const input = byteToObject(fn, size) as Record<string, number | string>;
			ctx.lineWidth = 1;
			ctx.fillStyle = 'black';
			ctx.strokeStyle = 'black';
			switch (fn) {
				case 99:
					navCanvas.width = input.w as number;
					navCanvas.height = input.h as number;
				case 0:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.clearRect(0, 0, navCanvas.width, navCanvas.height);
					ctx.fillRect(0, 0, navCanvas.width, navCanvas.height);
					break;
				case 1:
					ctx.strokeStyle = color565toCss(input.fg as number);
					ctx.strokeRect(input.x as number, input.y as number, input.w as number, input.h as number);
					break;
				case 2:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.clearRect(input.x as number, input.y as number, input.w as number, input.h as number);
					ctx.fillRect(input.x as number, input.y as number, input.w as number, input.h as number);
					break;
				case 3:
					ctx.strokeStyle = color565toCss(input.fg as number);
					drawRoundRect(ctx, input as { x: number; y: number; w: number; h: number; r: number }, false);
					break;
				case 4:
					ctx.fillStyle = color565toCss(input.fg as number);
					drawRoundRect(ctx, input as { x: number; y: number; w: number; h: number; r: number }, true);
					break;
				case 5:
					ctx.strokeStyle = color565toCss(input.fg as number);
					ctx.arc(input.x as number, input.y as number, input.r as number, 0, Math.PI * 2);
					ctx.stroke();
					break;
				case 6:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.arc(input.x as number, input.y as number, input.r as number, 0, Math.PI * 2);
					ctx.fill();
					break;
				case 7:
					ctx.strokeStyle = color565toCss(input.fg as number);
					ctx.beginPath();
					ctx.moveTo(input.x as number, input.y as number);
					ctx.lineTo(input.x2 as number, input.y2 as number);
					ctx.lineTo(input.x3 as number, input.y3 as number);
					ctx.closePath();
					ctx.stroke();
					break;
				case 8:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.beginPath();
					ctx.moveTo(input.x as number, input.y as number);
					ctx.lineTo(input.x2 as number, input.y2 as number);
					ctx.lineTo(input.x3 as number, input.y3 as number);
					ctx.closePath();
					ctx.fill();
					break;
				case 9:
					ctx.strokeStyle = color565toCss(input.fg as number);
					ctx.beginPath();
					ctx.ellipse(input.x as number, input.y as number, input.rx as number, input.ry as number, 0, 0, Math.PI * 2);
					ctx.stroke();
					break;
				case 10:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.beginPath();
					ctx.ellipse(input.x as number, input.y as number, input.rx as number, input.ry as number, 0, 0, Math.PI * 2);
					ctx.fill();
					break;
				case 11:
					ctx.strokeStyle = color565toCss(input.fg as number);
					ctx.moveTo(input.x as number, input.y as number);
					ctx.lineTo(input.x1 as number, input.y1 as number);
					ctx.stroke();
					break;
				case 12: {
					ctx.strokeStyle = color565toCss(input.fg as number);
					const radius = ((input.r as number) + (input.ir as number)) / 2;
					ctx.lineWidth = (input.r as number) - (input.ir as number) || 1;
					const sa = ((input.startAngle as number) + 90 || 0) * (Math.PI / 180);
					const ea = ((input.endAngle as number) + 90 || 0) * (Math.PI / 180);
					ctx.beginPath();
					ctx.arc(input.x as number, input.y as number, radius, sa, ea);
					ctx.stroke();
					break;
				}
				case 13:
					ctx.strokeStyle = color565toCss(input.fg as number);
					ctx.lineWidth = (input.wd as number) || 1;
					ctx.moveTo(input.x as number, input.y as number);
					ctx.lineTo(input.bx as number, input.by as number);
					ctx.stroke();
					break;
				case 14:
				case 15:
				case 16:
				case 17: {
					if (input.bg === input.fg) input.bg = 0;
					ctx.fillStyle = color565toCss(input.bg as number);
					const text = String(input.txt ?? '').replaceAll('\n', '');
					const fontWidth = (input.size as number) * 4.5;
					let offsetX = 0;
					if (fn === 15) offsetX = text.length * fontWidth;
					if (fn === 14) offsetX = (text.length * fontWidth) / 2;
					ctx.clearRect((input.x as number) - offsetX, input.y as number, text.length * fontWidth, (input.size as number) * 8);
					ctx.fillRect((input.x as number) - offsetX, input.y as number, text.length * fontWidth, (input.size as number) * 8);
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.font = `${(input.size as number) * 8}px monospace`;
					ctx.textBaseline = 'top';
					ctx.textAlign = fn === 14 ? 'center' : fn === 15 ? 'right' : 'left';
					ctx.fillText(text, input.x as number, input.y as number);
					break;
				}
				case 18:
					break;
				case 19:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.fillRect(input.x as number, input.y as number, 1, 1);
					break;
				case 20:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.fillRect(input.x as number, input.y as number, 1, input.h as number);
					break;
				case 21:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.fillRect(input.x as number, input.y as number, input.w as number, 1);
					break;
			}
		}
	}

	const handleKeydown = async (event: KeyboardEvent) => {
		if (!navigatorOpen) return;
		let dir: string | null = null;
		if (event.key === 'ArrowUp') dir = 'up';
		else if (event.key === 'ArrowDown') dir = 'down';
		else if (event.key === 'ArrowLeft') dir = 'prev';
		else if (event.key === 'ArrowRight') dir = 'next';
		else if (event.key === 'Enter') dir = 'sel';
		else if (event.key === 'Backspace') dir = 'esc';
		else if (event.key === 'PageUp') dir = 'nextpage';
		else if (event.key === 'PageDown') dir = 'prevpage';
		else if (event.key.toLowerCase() === 'h') dir = 'sel 700';
		else if (event.key.toLowerCase() === 'r') {
			event.preventDefault();
			await triggerDump();
			return;
		} else if (event.key === 'Escape') {
			event.preventDefault();
			await stopNavigator();
			return;
		}
		if (dir) {
			event.preventDefault();
			await sendNavigatorCommand(`nav ${dir}`);
			setTimeout(() => void triggerDump(), 500);
		}
	};

	const devices = [
		{
			name: 'M5StickC',
			img: 'm5stick.png'
		},
		{
			name: 'M5Stack Core',
			img: 'core2.png'
		},
		{
			name: 'Cardputer',
			img: 'cardputer.png'
		},
		{
			name: 'Lilygo T-Embed',
			img: 't-embed.png'
		},
		{
			name: 'CYD',
			img: 'cyd.png'
		},
		{
			name: 'Phantom',
			img: 'cyd.png'
		},
		{
			name: 'Smoochiee Board',
			img: 'bruce-pcb.png'
		}
	];

	let img = $state('');
	function get_device_image() {
		let _img = devices.find((_name) => device.includes(_name.name));
		if (_img != null) {
			img = _img.img;
		} else {
			img = 'bruce-logo.png';
		}
	}
</script>

<div class="shell py-12">
	<header class="mb-8 max-w-2xl">
		<span class="eyebrow">Device console</span>
		<h1 class="mt-3 text-3xl font-semibold md:text-4xl">Bruce Lab</h1>
	</header>

	{#if !supported}
		<div class="panel p-10 text-center">
			<h2 class="text-xl font-semibold">Unsupported browser</h2>
			<p class="mt-2 text-sm text-[var(--text-dim)]">Please use a Chromium based browser</p>
		</div>
	{:else if !connected}
		<div class="panel flex flex-col items-start gap-6 p-10 md:flex-row md:items-center md:justify-between">
			<div>
				<h2 class="text-xl font-semibold">Connect your device</h2>
				<p class="mt-2 text-sm text-[var(--text-dim)]">Bruce Lab reads the device state over Web Serial. Nothing leaves your browser.</p>
			</div>
			<Btn onclick={connect_device}>
				<Icon name="plug" size={15} /> Connect
			</Btn>
		</div>
	{:else if loading}
		<div class="panel flex flex-col items-center justify-center gap-4 p-16">
			<span class="lab-spinner"></span>
			<p class="text-sm text-[var(--text-dim)]">Loading... This may take a few seconds</p>
		</div>
	{:else}
		<div class="panel overflow-hidden">
			<div class="grid gap-px bg-[var(--rule)] lg:grid-cols-[1fr_0.9fr]">
				<!-- Readout -->
				<div class="bg-[var(--color-ink)] p-7">
					<h2 class="eyebrow mb-4">Device</h2>
					<dl>
						<InfoRow label="Firmware" value={version} />
						<InfoRow label="SD card" value={sdCard} />
						<InfoRow label="LittleFS" value={littleFS_Storage} />
						<InfoRow label="Hardware" value={device} />
						<InfoRow label="MAC Address" value={mac_address} />
						<InfoRow label="WiFi" value={wifi} />
						<InfoRow label="Heap usage" value={heap_usage} />
						<InfoRow label="PSRAM usage" value={psram_usage} />
						<InfoRow label="Uptime" value={uptime} />
						<InfoRow label="SDK" value={sdk} />
					</dl>
				</div>

				<!-- Portrait -->
				<div class="flex items-center justify-center bg-[var(--color-ink)] p-7">
					<img src="{base}/img/{img}" alt="Bruce device" class="max-h-80 w-auto max-w-full object-contain" />
				</div>
			</div>

			<div class="flex flex-wrap gap-3 border-t border-[var(--rule)] p-6">
				<Btn href="{base}/flasher">Update</Btn>
				<Btn onclick={startNavigator} outline>
					<Icon name="terminal" size={15} /> Navigator
				</Btn>
				<Btn onclick={reboot_bruce} outline>
					<Icon name="refresh" size={15} /> Reboot
				</Btn>
				<Btn onclick={factory_reset} outline>Factory Reset</Btn>
			</div>
		</div>
	{/if}
</div>

{#if connected && !loading && navigatorOpen}
	<div
		class="fixed inset-0 z-[200] flex items-center justify-center bg-black/80 p-4 backdrop-blur-sm"
		role="dialog"
		aria-modal="true"
		aria-labelledby="navigator-title"
		tabindex="0"
		onclick={(event) => {
			if (event.currentTarget === event.target) {
				void stopNavigator();
			}
		}}
		onkeydown={(event) => {
			if (event.currentTarget === event.target && (event.key === 'Enter' || event.key === ' ')) {
				event.preventDefault();
				void stopNavigator();
			}
		}}
	>
		<div class="w-full max-w-5xl border border-[var(--rule-strong)] bg-[var(--color-surface)]">
			<div class="flex flex-wrap items-center justify-between gap-3 border-b border-[var(--rule)] px-6 py-4">
				<div class="flex items-baseline gap-3">
					<h2 id="navigator-title" class="text-lg font-semibold">Device Navigator</h2>
					<span class="meta">Serial</span>
				</div>
				<button class="btn btn-quiet" onclick={stopNavigator}>Close</button>
			</div>

			<div class="grid gap-px bg-[var(--rule)] lg:grid-cols-[1.2fr_0.8fr]">
				<div class="bg-[var(--color-ink)] p-6">
					<canvas bind:this={navCanvas} class="h-auto w-full bg-black" width="320" height="240"></canvas>
				</div>

				<div class="space-y-5 bg-[var(--color-ink)] p-6">
					<!-- D-pad: icons replace the arrow glyphs that used to be typed
					     straight into the markup. -->
					<div class="grid grid-cols-3 gap-2">
						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav prevpage')} title="Page up" aria-label="Page up">
							<Icon name="chevrons-up" size={16} />
						</button>
						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav up')} title="Up" aria-label="Up">
							<Icon name="chevron-up" size={16} />
						</button>
						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav sel 700')} title="Select, hold" aria-label="Select, hold">
							<span class="font-mono text-xs">H</span>
						</button>

						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav prev')} title="Previous" aria-label="Previous">
							<Icon name="chevron-left" size={16} />
						</button>
						<button class="nav-btn nav-ok" onclick={() => void sendNavigatorCommand('nav sel')} title="Select" aria-label="Select">
							<span class="font-mono text-xs">OK</span>
						</button>
						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav next')} title="Next" aria-label="Next">
							<Icon name="chevron-right" size={16} />
						</button>

						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav nextpage')} title="Page down" aria-label="Page down">
							<Icon name="chevrons-down" size={16} />
						</button>
						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav down')} title="Down" aria-label="Down">
							<Icon name="chevron-down" size={16} />
						</button>
						<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav esc')} title="Back" aria-label="Back">
							<Icon name="corner-back" size={16} />
						</button>
					</div>

					<div class="border border-[var(--rule)] p-4">
						<h3 class="eyebrow">Shortcuts</h3>
						<p class="mt-2 text-xs leading-relaxed text-[var(--text-dim)]">Arrows = Navigation, Enter = OK, Backspace = Back</p>
						<p class="text-xs leading-relaxed text-[var(--text-dim)]">PageUp/PageDown = PgUp/PgDn, H = Sel hold, R = Reload</p>

						<h3 class="eyebrow mt-4">Limitations</h3>
						<p class="mt-2 text-xs leading-relaxed text-[var(--text-dim)]">Images are not rendered in the Serial Navigator.</p>
					</div>
				</div>
			</div>

			<div class="flex flex-wrap items-center justify-between gap-3 border-t border-[var(--rule)] px-6 py-4">
				<span class="meta">After each command, you can force reload with Reload.</span>
				<button class="btn btn-primary" onclick={triggerDump}>
					<Icon name="refresh" size={15} /> Reload
				</button>
			</div>
		</div>
	</div>
{/if}

<style>
	.nav-btn {
		display: inline-flex;
		align-items: center;
		justify-content: center;
		border: 1px solid var(--rule);
		background: var(--wash);
		border-radius: 3px;
		padding: 0.7rem;
		color: #fff;
		cursor: pointer;
		transition:
			background-color 0.18s ease,
			border-color 0.18s ease;
	}

	.nav-btn:hover {
		border-color: var(--color-brand);
		background: rgba(155, 81, 224, 0.16);
	}

	.nav-ok {
		border-color: var(--color-brand);
		background: var(--color-brand);
	}

	.nav-ok:hover {
		background: var(--color-brand-strong);
	}

	@keyframes lab-spin {
		to {
			transform: rotate(360deg);
		}
	}

	.lab-spinner {
		width: 2.5rem;
		height: 2.5rem;
		border: 2px solid rgba(255, 255, 255, 0.12);
		border-top-color: var(--color-brand);
		border-radius: 50%;
		animation: lab-spin 0.8s linear infinite;
	}
</style>
