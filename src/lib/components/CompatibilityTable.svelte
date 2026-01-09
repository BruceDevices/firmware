<script lang="ts">
	import type { DeviceCompatibility, FeatureRow } from '$lib/types/device';
	import devicesData from '$lib/data/devices.json';
	import { onMount } from 'svelte';
	import { fade } from 'svelte/transition';

	const compatibilityData: DeviceCompatibility[] = devicesData;

	// Get features dynamically from the first object
	let features: (keyof FeatureRow)[] = [];
	if (compatibilityData.length > 0) {
		features = Object.keys(compatibilityData[0]).filter((k) => k !== 'device') as (keyof FeatureRow)[];
	}

	let scrollContainer: HTMLElement;
	let showGradient = false;

	function toggleDetailedView() {
		const detailedRows = document.querySelectorAll('.detailed');
		detailedRows.forEach((row) => {
			row.classList.toggle('hidden');
		});
		document.getElementById('toggleDetailedView')!.textContent = detailedRows[0].classList.contains('hidden')
			? 'Show Detailed View'
			: 'Hide Detailed View';

		// Recheck gradient visibility after toggling detailed view
		setTimeout(checkGradientVisibility, 100);
	}

	function checkGradientVisibility() {
		if (!scrollContainer) return;

		const isScrollable = scrollContainer.scrollWidth > scrollContainer.clientWidth;
		const isScrolledToEnd = scrollContainer.scrollLeft + scrollContainer.clientWidth >= scrollContainer.scrollWidth - 1;

		showGradient = isScrollable && !isScrolledToEnd;
	}

	function handleScroll() {
		checkGradientVisibility();
	}

	onMount(() => {
		checkGradientVisibility();

		// Also check on window resize
		const handleResize = () => checkGradientVisibility();
		window.addEventListener('resize', handleResize);

		return () => {
			window.removeEventListener('resize', handleResize);
		};
	});
</script>

<div class="container">
	<div class="mx-auto mt-10 w-[90%] text-center">
		<h1 class="mb-5 text-3xl font-bold text-white">Compatible Devices</h1>
		<h2 class="mb-5 text-lg text-white">
			This table shows the compatibility of various devices with Bruce's features. Click the button to toggle detailed view for more information.
		</h2>
		<button
			id="toggleDetailedView"
			class="mb-5 rounded bg-[#9B51E0] px-4 py-2 font-semibold text-white hover:bg-[#8033C7]"
			on:click={toggleDetailedView}
		>
			Show Detailed View
		</button>
		<div class="table-wrapper relative">
			<div class="w-full overflow-x-auto whitespace-nowrap" bind:this={scrollContainer} on:scroll={handleScroll}>
				<table class="w-full border-collapse rounded-lg bg-neutral-900">
					<thead>
						<tr>
							<th class="sticky left-0 z-10 bg-neutral-900 px-4 py-3 text-center text-base text-white">Device</th>
							{#each features as feat}
								<th
									class="px-4 py-3 text-center text-base text-white {feat === 'Screen' ||
									feat === 'ESP' ||
									feat === 'Battery' ||
									feat === 'Flash' ||
									feat === 'PSRAM'
										? 'detailed hidden'
										: ''}">{feat.replace('_', ' ')}</th
								>
							{/each}
						</tr>
					</thead>
					<tbody>
						{#each compatibilityData as row, index}
							<tr class="group even:bg-neutral-800 hover:bg-neutral-700">
								<td
									class="sticky left-0 z-10 px-4 py-3 text-center text-base text-white {index % 2 === 0
										? 'bg-neutral-900'
										: 'bg-neutral-800'} group-hover:bg-neutral-700">{row.device}</td
								>
								{#each features as key}
									<td
										title={typeof row[key] === 'string' ? row[key] : ''}
										class={key === 'Screen' || key === 'ESP' || key === 'Battery' || key === 'Flash' || key === 'PSRAM' ? 'detailed hidden' : ''}
									>
										{#if (key === 'Screen' || key === 'ESP' || key === 'Battery' || key === 'Flash' || key === 'PSRAM') && typeof row[key] === 'string'}
											{row[key]}
										{:else if key === 'NFC' && typeof row[key] === 'string' && row[key] !== 'Module Required'}
											✅
										{:else if key === 'Mic'}
											{typeof row[key] === 'string' ? '✅' : '❌'}
											{#if typeof row[key] === 'string'}<span class="detailed hidden">{row[key]}</span>{/if}
										{:else if key === 'Audio'}
											{row[key] === 'Tone' ? '🔈' : typeof row[key] === 'string' ? '🔊' : '❌'}
											{#if typeof row[key] === 'string'}<span class="detailed hidden">{row[key] !== 'Tone' ? 'Full - ' : ''}{row[key]}</span>{/if}
										{:else}
											{row[key] === true ? '✅' : row[key] === false ? '❌' : row[key] === 'Module Required' ? 'ℹ️' : ''}
										{/if}
									</td>
								{/each}
							</tr>
						{/each}
					</tbody>
				</table>
			</div>
			{#if showGradient}
				<div class="scroll-gradient" transition:fade={{ duration: 300 }}></div>
			{/if}
		</div>
		<br />
		<h2 class="text-lg">
			For <strong>Wiring Diagrams</strong> check the
			<a href="https://github.com/BruceDevices/firmware/tree/main/media/connections"><b>connections</b></a>
			or <a href="https://wiki.bruce.computer"><b>Wiki</b></a>!
		</h2>
		<h2 class="text-lg">Every feature is also listed on Github.</h2>
	</div>
</div>

<style>
	.container {
		width: 90%;
		max-width: 100%;
		margin: 0 auto;
	}

	.table-wrapper {
		position: relative;
	}

	.scroll-gradient {
		position: absolute;
		top: 0;
		right: 0;
		bottom: 0;
		width: 100px;
		background: linear-gradient(to left, rgba(0, 0, 0, 0.9), transparent);
		pointer-events: none;
		z-index: 5;
	}
</style>
