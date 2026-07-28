<script lang="ts">
	import { base } from '$app/paths';
	import Btn from '$lib/components/Btn.svelte';
	import InfoRow from '$lib/components/InfoRow.svelte';
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
		if(lines[0].includes('COMMAND')) {
			version = `${lines[1]} - ${lines[2]}`;
		} else {
			version = `${lines[0]} - ${lines[1]}`;
		}
		for(const line of lines){
			if(line.includes('SDK')) { sdk = line.replace('SDK: ', ''); }
			if(line.includes('MAC addr')) { mac_address = line.replace('MAC addr:', ''); }
			if(line.includes('Wifi')) { wifi = capitalize(line.replace('Wifi: ', '')); }
			if(line.includes('Device')) { device = line.replace('Device: ', ''); }
		}
	}

	function parseUptime(response: string) {
		const lines = response.split('\n');
		for(const line of lines) {
			if(line.includes('Uptime')){
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
			for(const line of lines) {
				if(line.includes('Total heap')) {
					total_heap = +line.replace(/\D/g, '');
				}
				if(line.includes('Free heap')) {
					free_heap = +line.replace(/\D/g, '');
				}
				if(line.includes('Total PSRAM')) {
					total_psram = +line.replace(/\D/g, '');
				}
				if(line.includes('Free PSRAM')) {
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
		for(const line of lines) {
			if(line.includes('Total')) {
				total = +line.replace(/\D/g, '');
			}
			if(line.includes('Free')) {
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

		const drawRoundRect = (
			ctx: CanvasRenderingContext2D,
			input: { x: number; y: number; w: number; h: number; r: number },
			fill: boolean
		) => {
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
					ctx.strokeRect(
						input.x as number,
						input.y as number,
						input.w as number,
						input.h as number
					);
					break;
				case 2:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.clearRect(
						input.x as number,
						input.y as number,
						input.w as number,
						input.h as number
					);
					ctx.fillRect(
						input.x as number,
						input.y as number,
						input.w as number,
						input.h as number
					);
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
					ctx.ellipse(
						input.x as number,
						input.y as number,
						input.rx as number,
						input.ry as number,
						0,
						0,
						Math.PI * 2
					);
					ctx.stroke();
					break;
				case 10:
					ctx.fillStyle = color565toCss(input.fg as number);
					ctx.beginPath();
					ctx.ellipse(
						input.x as number,
						input.y as number,
						input.rx as number,
						input.ry as number,
						0,
						0,
						Math.PI * 2
					);
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
					const sa = (((input.startAngle as number) + 90) || 0) * (Math.PI / 180);
					const ea = (((input.endAngle as number) + 90) || 0) * (Math.PI / 180);
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
					ctx.clearRect(
						(input.x as number) - offsetX,
						input.y as number,
						text.length * fontWidth,
						(input.size as number) * 8
					);
					ctx.fillRect(
						(input.x as number) - offsetX,
						input.y as number,
						text.length * fontWidth,
						(input.size as number) * 8
					);
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

{#if !supported}
	<h1 class="mt-32 text-center text-3xl font-bold">Unsupported browser</h1>
	<h1 class="mt-5 mb-10 text-center text-3xl font-bold">Please use a Chromium based browser</h1>
{:else if !connected}
	<div class="flex items-center justify-center text-center">
		<Btn className="mt-32 mb-10" onclick={connect_device}>Connect</Btn>
	</div>
{:else if loading}
	<div class="mt-32 flex flex-col items-center justify-center">
		<div class="h-16 w-16 animate-spin rounded-full border-4 border-blue-600 border-t-transparent" role="status"></div>

		<p class="mt-5 mb-5 text-center text-white" style="color:white;">Loading... This may take a few seconds</p>
	</div>
{:else}
	<div class="mx-auto mt-32 max-w-5xl rounded-lg">
		<div class="flex items-stretch justify-between gap-6">
			<div class="flex-1 min-w-0">
				<div class="space-y-3">
					<InfoRow label="Firmware" value={version} />
					<InfoRow label="SD card" value={sdCard} />
					<InfoRow label="LittleFS" value={littleFS_Storage} />
					<InfoRow label="Hardware" value={device} />
					<InfoRow label="MAC Address" value={mac_address} />
					<InfoRow label="WiFi" value={wifi} />
					<InfoRow label="Heap usage" value={heap_usage} />
					<InfoRow label="PSRAM usage" value={psram_usage} />
					<!-- <InfoRow label="Total heap" value={total_heap} />
					<InfoRow label="Free heap" value={free_heap} />
					<InfoRow label="Total PSRAM" value={total_psram} />
					<InfoRow label="Free PSRAM" value={free_psram} /> -->
					<InfoRow label="Uptime" value={uptime} />
					<InfoRow label="SDK" value={sdk} />
				</div>
			</div>

			<div class="flex w-1/2 flex-shrink-0 items-center justify-center">
				<img
					src="{base}/img/{img}"
					alt="Bruce device"
					class="h-auto w-auto max-h-full max-w-full object-contain"
				/>
			</div>
		</div>
		<div class="mt-10 mb-10">
			<Btn href="{base}/flasher">Update</Btn>
			<Btn onclick={factory_reset}>Factory Reset</Btn>
			<Btn onclick={reboot_bruce}>Reboot</Btn>
			<Btn onclick={startNavigator}>Navigator</Btn>
		</div>
	</div>

	{#if navigatorOpen}
		<div
			class="fixed inset-0 z-50 flex items-center justify-center bg-black/70 p-4"
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
			<div class="w-full max-w-5xl rounded-2xl border border-white/10 bg-[#0f0f14] shadow-2xl">
				<div class="flex flex-wrap items-center justify-between gap-3 border-b border-white/10 px-5 py-4">
					<div class="flex items-center gap-3">
						<h2 id="navigator-title" class="text-lg font-semibold text-white">Device Navigator</h2>
						<span class="rounded-full border border-white/10 px-3 py-1 text-xs text-white/70"
							>Serial</span
						>
					</div>
					<div class="flex flex-wrap items-center gap-2">
						<button
							class="rounded-lg border border-white/10 bg-white/5 px-3 py-2 text-sm font-semibold text-white transition hover:bg-[#9B51E0]"
							onclick={stopNavigator}
						>
							Close
						</button>
					</div>
				</div>

				<div class="grid gap-6 p-6 lg:grid-cols-[1.2fr_0.8fr]">
					<div class="rounded-xl border border-white/10 bg-black p-3">
						<canvas bind:this={navCanvas} class="h-auto w-full rounded-lg bg-black" width="320" height="240"></canvas>
					</div>
					<div class="space-y-4">
						<div class="rounded-xl border border-white/10 bg-white/5 p-3">
							<div class="grid gap-2">
								<div class="grid grid-cols-3 gap-2">
									<button
										class="nav-btn"
										onclick={() => void sendNavigatorCommand('nav prevpage')}
									>
										Pg↑
									</button>
									<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav up')}>▲</button>
									<button
										class="nav-btn"
										onclick={() => void sendNavigatorCommand('nav sel 700')}
									>
										H
									</button>
								</div>
								<div class="grid grid-cols-3 gap-2">
									<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav prev')}>
										◀
									</button>
									<button
										class="nav-btn nav-ok"
										onclick={() => void sendNavigatorCommand('nav sel')}
									>
										OK
									</button>
									<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav next')}>
										▶
									</button>
								</div>
								<div class="grid grid-cols-3 gap-2">
									<button
										class="nav-btn"
										onclick={() => void sendNavigatorCommand('nav nextpage')}
									>
										Pg↓
									</button>
									<button
										class="nav-btn"
										onclick={() => void sendNavigatorCommand('nav down')}
									>
										▼
									</button>
									<button class="nav-btn" onclick={() => void sendNavigatorCommand('nav esc')}>
										⟲
									</button>
								</div>
							</div>
						</div>

						<div class="rounded-xl border border-white/10 bg-white/5 p-4 text-xs text-white/70">
							<p class="mb-2 font-semibold text-white">Shortcuts</p>
							<p>Arrows = Navigation, Enter = OK, Backspace = Back</p>
							<p>PageUp/PageDown = PgUp/PgDn, H = Sel hold, R = Reload</p>
							<br>
							<p class="mb-2 font-semibold text-white">Limitations</p>
							<p>Images are not rendered in the Serial Navigator.</p>
						</div>
					</div>
				</div>

				<div class="flex items-center justify-between gap-3 border-t border-white/10 px-6 py-4">
					<span class="text-xs text-white/70">After each command, you can force reload with Reload.</span>
					<button
						class="rounded-lg bg-[#9B51E0] px-4 py-2 text-sm font-semibold text-white transition hover:scale-105 hover:bg-[#8033C7]"
						onclick={triggerDump}
					>
						Reload
					</button>
				</div>
			</div>
		</div>
	{/if}
{/if}

<style>
	.nav-btn {
		border-radius: 0.75rem;
		border: 1px solid rgba(255, 255, 255, 0.1);
		background: rgba(255, 255, 255, 0.05);
		padding: 0.6rem 0.75rem;
		font-size: 0.8rem;
		font-weight: 600;
		color: white;
		transition: background 0.2s ease, transform 0.2s ease;
	}

	.nav-btn:hover {
		background: #9b51e0;
	}

	.nav-ok {
		background: #9b51e0;
	}
</style>
