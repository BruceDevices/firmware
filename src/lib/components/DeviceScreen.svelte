<script lang="ts">
	import { onMount } from 'svelte';

	// Faithful mock of the real Bruce firmware UI: the icon-carousel main menu
	// (big central icon between chevrons, name below, status bar on top, purple
	// frame from drawMainBorder) followed by a WiFi scan screen — the same titled
	// frame + inverted rows + signal/lock the device actually draws.
	//
	// Icons reproduce the shapes each menu draws in drawIcon() (WifiMenu, BleMenu,
	// RFMenu, RFIDMenu) rather than generic glyphs.
	const items = ['WiFi', 'Bluetooth', 'RF', 'RFID'] as const;

	const networks = [
		{ ssid: 'HomeNet_42', ch: 6, rssi: -46, lock: true },
		{ ssid: 'FBI_Surveillance_Van', ch: 1, rssi: -63, lock: true },
		{ ssid: 'Free_Public_WiFi', ch: 11, rssi: -74, lock: false },
		{ ssid: 'NETGEAR-5G', ch: 36, rssi: -81, lock: true },
		{ ssid: 'pwned_by_bruce', ch: 9, rssi: -55, lock: false }
	];

	// Timeline (ticks): 0..3 carousel items, 4 back to WiFi, 5 enter, 6..10 scan
	// reveals, 11..14 hold, then loop.
	const TICKS = 16;
	let frame = $state(0);

	const scanning = $derived(frame >= 5);
	const idx = $derived(frame <= 4 ? frame % items.length : 0);
	const shown = $derived(scanning ? Math.max(0, Math.min(frame - 5, networks.length)) : 0);

	function bars(rssi: number) {
		return rssi > -55 ? 4 : rssi > -68 ? 3 : rssi > -80 ? 2 : 1;
	}

	onMount(() => {
		const reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
		if (reduce) {
			frame = TICKS - 3;
			return;
		}
		const id = setInterval(() => (frame = (frame + 1) % TICKS), 900);
		return () => clearInterval(id);
	});
</script>

<div class="device-screen scanlines">
	<!-- Status bar (drawStatusBar): clock · title · SD + battery -->
	<div class="statusbar">
		<span>21:06</span>
		<span class="brand">BRUCE</span>
		<span class="flex items-center gap-1">
			<span>SD</span>
			<span class="batt"><span></span></span>
		</span>
	</div>

	{#if !scanning}
		<!-- Icon-carousel main menu -->
		<div class="carousel">
			<span class="chev">‹</span>

			<div class="icon" style="color: var(--color-brand)">
				{#key idx}
					<div class="icon-anim">
						{#if items[idx] === 'WiFi'}
							<svg viewBox="0 0 100 100" fill="none">
								<circle cx="50" cy="70" r="7" fill="currentColor" />
								<path d="M34 58 Q50 42 66 58" stroke="currentColor" stroke-width="7" stroke-linecap="round" />
								<path d="M22 49 Q50 25 78 49" stroke="currentColor" stroke-width="7" stroke-linecap="round" />
							</svg>
						{:else if items[idx] === 'Bluetooth'}
							<svg viewBox="0 0 24 24" fill="currentColor">
								<path
									d="M17.71 7.71 12 2h-1v7.59L6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 11 14.41V22h1l5.71-5.71L13.41 12l4.3-4.29zM13 5.83l1.88 1.88L13 9.59V5.83zm1.88 10.46L13 18.17v-3.76l1.88 1.88z"
								/>
							</svg>
						{:else if items[idx] === 'RF'}
							<svg viewBox="0 0 100 100" fill="none">
								<circle cx="50" cy="34" r="8" fill="currentColor" />
								<polygon points="50,44 37,76 63,76" fill="currentColor" />
								<path d="M36 22 Q26 37 36 52" stroke="currentColor" stroke-width="5" stroke-linecap="round" />
								<path d="M30 14 Q14 37 30 60" stroke="currentColor" stroke-width="5" stroke-linecap="round" />
								<path d="M64 22 Q74 37 64 52" stroke="currentColor" stroke-width="5" stroke-linecap="round" />
								<path d="M70 14 Q86 37 70 60" stroke="currentColor" stroke-width="5" stroke-linecap="round" />
							</svg>
						{:else}
							<svg viewBox="0 0 100 100" fill="none">
								<rect x="20" y="24" width="60" height="52" rx="8" stroke="currentColor" stroke-width="6" />
								<circle cx="36" cy="60" r="4.5" fill="currentColor" />
								<path d="M45 60 Q45 51 36 51" stroke="currentColor" stroke-width="5" stroke-linecap="round" />
								<path d="M53 60 Q53 43 36 43" stroke="currentColor" stroke-width="5" stroke-linecap="round" />
							</svg>
						{/if}
					</div>
				{/key}
			</div>

			<span class="chev">›</span>
		</div>

		<div class="menu-name">{items[idx]}</div>
	{:else}
		<!-- WiFi scan screen: drawMainBorderWithTitle + inverted rows -->
		<div class="title">WIFI</div>
		<ul class="scan">
			{#each networks.slice(0, shown) as net, i}
				<li class:sel={i === 0}>
					<svg width="9" height="11" viewBox="0 0 9 11" class="shrink-0" aria-hidden="true">
						<rect x="0" y="4" width="9" height="6" rx="1" fill="currentColor" />
						{#if net.lock}
							<path d="M2 4 V2.5 A2.5 2.5 0 0 1 7 2.5 V4" fill="none" stroke="currentColor" stroke-width="1" />
						{:else}
							<path d="M2 4 V2.5 A2.5 2.5 0 0 1 6.4 1.7" fill="none" stroke="currentColor" stroke-width="1" />
						{/if}
					</svg>
					<span class="ssid">{net.ssid}</span>
					<span class="ch">c{net.ch}</span>
					<span class="bars">
						{#each [1, 2, 3, 4] as b}
							<i class:on={b <= bars(net.rssi)} style="height: {2 + b * 2}px"></i>
						{/each}
					</span>
				</li>
			{/each}
		</ul>
		<div class="hint">
			{shown < networks.length ? 'scanning 2.4GHz' : `${networks.length} networks found`}<span class="cursor"></span>
		</div>
	{/if}
</div>

<style>
	.device-screen {
		--fg: var(--color-brand);
		position: relative;
		width: 100%;
		max-width: 22rem;
		margin-inline: auto;
		aspect-ratio: 240 / 175;
		border-radius: 10px;
		border: 2px solid color-mix(in srgb, var(--color-brand) 65%, transparent);
		background:
			radial-gradient(120% 130% at 50% -10%, rgba(155, 81, 224, 0.1), transparent 60%),
			var(--color-surface);
		box-shadow:
			inset 0 0 0 1px rgba(255, 255, 255, 0.03),
			0 30px 60px -30px rgba(155, 81, 224, 0.6),
			0 0 44px -18px rgba(155, 81, 224, 0.75);
		overflow: hidden;
		font-family: var(--font-mono);
		color: #fff;
		display: flex;
		flex-direction: column;
	}

	.statusbar {
		display: flex;
		align-items: center;
		justify-content: space-between;
		padding: 5px 10px 6px;
		font-size: 0.55rem;
		letter-spacing: 0.06em;
		color: rgba(255, 255, 255, 0.55);
		border-bottom: 1px solid color-mix(in srgb, var(--color-brand) 30%, transparent);
	}
	.statusbar .brand {
		color: var(--color-brand);
		letter-spacing: 0.22em;
	}
	.batt {
		display: inline-block;
		width: 14px;
		height: 8px;
		border: 1px solid rgba(255, 255, 255, 0.45);
		border-radius: 1px;
		padding: 1px;
	}
	.batt > span {
		display: block;
		width: 65%;
		height: 100%;
		background: var(--color-brand);
	}

	/* --- Carousel main menu --- */
	.carousel {
		flex: 1;
		display: flex;
		align-items: center;
		justify-content: center;
		gap: 6%;
	}
	.chev {
		font-size: 1.8rem;
		line-height: 1;
		color: var(--color-brand);
		opacity: 0.85;
		animation: chev-pulse 1.8s ease-in-out infinite;
	}
	.carousel .chev:last-child {
		animation-delay: 0.9s;
	}
	@keyframes chev-pulse {
		0%,
		100% {
			opacity: 0.4;
		}
		50% {
			opacity: 0.95;
		}
	}
	.icon {
		width: 46%;
		max-width: 108px;
		aspect-ratio: 1;
	}
	.icon svg {
		width: 100%;
		height: 100%;
		display: block;
		filter: drop-shadow(0 0 10px rgba(155, 81, 224, 0.45));
	}
	.icon-anim {
		width: 100%;
		height: 100%;
		animation: icon-in 0.42s cubic-bezier(0.22, 1, 0.36, 1);
	}
	@keyframes icon-in {
		from {
			opacity: 0;
			transform: scale(0.82);
		}
		to {
			opacity: 1;
			transform: scale(1);
		}
	}
	.menu-name {
		text-align: center;
		font-size: 1rem;
		font-weight: 700;
		letter-spacing: 0.02em;
		padding-bottom: 14px;
	}

	/* --- Scan screen --- */
	.title {
		text-align: center;
		font-size: 0.72rem;
		font-weight: 700;
		letter-spacing: 0.14em;
		color: var(--color-brand);
		padding: 4px 0 6px;
	}
	.scan {
		flex: 1;
		list-style: none;
		margin: 0;
		padding: 0 8px;
		display: flex;
		flex-direction: column;
		gap: 3px;
	}
	.scan li {
		display: flex;
		align-items: center;
		gap: 7px;
		padding: 3px 7px;
		border-radius: 2px;
		font-size: 0.66rem;
		color: rgba(255, 255, 255, 0.82);
		animation: row-in 0.3s ease;
	}
	.scan li.sel {
		background: var(--color-brand);
		color: #000;
	}
	@keyframes row-in {
		from {
			opacity: 0;
			transform: translateX(-6px);
		}
		to {
			opacity: 1;
			transform: none;
		}
	}
	.ssid {
		flex: 1;
		min-width: 0;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.ch {
		opacity: 0.7;
	}
	.bars {
		display: flex;
		align-items: flex-end;
		gap: 2px;
		height: 12px;
	}
	.bars i {
		width: 3px;
		background: currentColor;
		opacity: 0.25;
	}
	.bars i.on {
		opacity: 1;
	}
	.hint {
		font-size: 0.58rem;
		color: rgba(255, 255, 255, 0.45);
		padding: 6px 12px 10px;
	}
</style>
