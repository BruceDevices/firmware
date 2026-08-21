<script lang="ts">
	import type { DeviceCompatibility, FeatureRow } from '$lib/types/device';
	import devicesData from '$lib/data/devices.json';
	import Icon from '$lib/components/Icon.svelte';
	import { onMount } from 'svelte';
	import { fade } from 'svelte/transition';

	const compatibilityData: DeviceCompatibility[] = devicesData;

	// Get features dynamically from the first object
	let features: (keyof FeatureRow)[] = [];
	if (compatibilityData.length > 0) {
		features = Object.keys(compatibilityData[0]).filter((k) => k !== 'device') as (keyof FeatureRow)[];
	}

	// Columns only shown in the detailed view. Driven by state now instead of
	// toggling .hidden on queried nodes, so the markup stays declarative.
	const detailedKeys = ['Screen', 'ESP', 'Battery', 'Flash', 'PSRAM'];
	const isDetailed = (k: string) => detailedKeys.includes(k);

	let scrollContainer: HTMLElement;
	let showGradient = $state(false);
	let detailed = $state(false);

	function toggleDetailedView() {
		detailed = !detailed;
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

<section class="shell py-20">
	<div class="mb-8 flex flex-col gap-6 md:flex-row md:items-end md:justify-between">
		<div class="max-w-2xl">
			<span class="eyebrow">Hardware support</span>
			<h2 class="mt-3 text-3xl font-semibold md:text-4xl">Compatible Devices</h2>
			<p class="lede mt-4 text-base">
				This table shows the compatibility of various devices with Bruce's features. Click the button to toggle detailed view for more information.
			</p>
		</div>
		<button class="btn btn-outline shrink-0" onclick={toggleDetailedView} aria-pressed={detailed}>
			{detailed ? 'Hide Detailed View' : 'Show Detailed View'}
		</button>
	</div>

	<div class="relative border border-[var(--rule)]">
		<div class="w-full overflow-x-auto" bind:this={scrollContainer} onscroll={handleScroll}>
			<table class="w-full border-collapse text-sm whitespace-nowrap">
				<thead>
					<tr class="border-b border-[var(--rule)]">
						<th class="sticky left-0 z-10 bg-[var(--color-surface)] px-4 py-3 text-left font-medium text-[var(--text-dim)]">Device</th>
						{#each features as feat}
							{#if !isDetailed(feat) || detailed}
								<th class="bg-[var(--color-surface)] px-4 py-3 text-center font-medium text-[var(--text-dim)]">{feat.replace('_', ' ')}</th>
							{/if}
						{/each}
					</tr>
				</thead>
				<tbody>
					{#each compatibilityData as row}
						<tr class="group border-b border-[var(--rule)] transition-colors last:border-b-0 hover:bg-white/[0.03]">
							<td class="sticky left-0 z-10 bg-[var(--color-ink)] px-4 py-2.5 text-left font-medium">{row.device}</td>
							{#each features as key}
								{#if !isDetailed(key) || detailed}
									<td class="px-4 py-2.5 text-center" title={typeof row[key] === 'string' ? row[key] : ''}>
										{#if isDetailed(key) && typeof row[key] === 'string'}
											<span class="font-mono text-xs text-[var(--text-dim)]">{row[key]}</span>
										{:else if key === 'NFC' && typeof row[key] === 'string' && row[key] !== 'Module Required'}
											<span class="inline-flex text-[var(--color-brand)]" title="Supported"><Icon name="yes" size={15} /></span>
										{:else if key === 'Mic'}
											{#if typeof row[key] === 'string'}
												<span class="inline-flex items-center justify-center gap-1.5 text-[var(--color-brand)]" title={row[key]}>
													<Icon name="yes" size={15} />
													{#if detailed}<span class="font-mono text-xs text-[var(--text-dim)]">{row[key]}</span>{/if}
												</span>
											{:else}
												<span class="inline-flex text-[var(--text-faint)]" title="Not supported"><Icon name="no" size={15} /></span>
											{/if}
										{:else if key === 'Audio'}
											{#if row[key] === 'Tone'}
												<span class="inline-flex items-center justify-center gap-1.5 text-[var(--color-brand)]" title="Tone">
													<Icon name="volume-low" size={15} />
													{#if detailed}<span class="font-mono text-xs text-[var(--text-dim)]">Tone</span>{/if}
												</span>
											{:else if typeof row[key] === 'string'}
												<span class="inline-flex items-center justify-center gap-1.5 text-[var(--color-brand)]" title="Full - {row[key]}">
													<Icon name="volume-high" size={15} />
													{#if detailed}<span class="font-mono text-xs text-[var(--text-dim)]">Full - {row[key]}</span>{/if}
												</span>
											{:else}
												<span class="inline-flex text-[var(--text-faint)]" title="Not supported"><Icon name="no" size={15} /></span>
											{/if}
										{:else if row[key] === true}
											<span class="inline-flex text-[var(--color-brand)]" title="Supported"><Icon name="yes" size={15} /></span>
										{:else if row[key] === false}
											<span class="inline-flex text-[var(--text-faint)]" title="Not supported"><Icon name="no" size={15} /></span>
										{:else if row[key] === 'Module Required'}
											<span class="inline-flex text-[var(--text-dim)]" title="Module Required"><Icon name="info" size={15} /></span>
										{/if}
									</td>
								{/if}
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

	<!-- The glyphs above are unlabelled, so the legend states them once. -->
	<dl class="mt-4 flex flex-wrap items-center gap-x-6 gap-y-2">
		<div class="flex items-center gap-2">
			<dt class="inline-flex text-[var(--color-brand)]"><Icon name="yes" size={14} /></dt>
			<dd class="meta">Supported</dd>
		</div>
		<div class="flex items-center gap-2">
			<dt class="inline-flex text-[var(--text-faint)]"><Icon name="no" size={14} /></dt>
			<dd class="meta">Not supported</dd>
		</div>
		<div class="flex items-center gap-2">
			<dt class="inline-flex text-[var(--text-dim)]"><Icon name="info" size={14} /></dt>
			<dd class="meta">Module required</dd>
		</div>
		<div class="flex items-center gap-2">
			<dt class="inline-flex text-[var(--color-brand)]"><Icon name="volume-low" size={14} /></dt>
			<dd class="meta">Tone only</dd>
		</div>
	</dl>

	<div class="mt-6 space-y-1 text-sm text-[var(--text-dim)]">
		<p>
			For <strong class="text-white">Wiring Diagrams</strong> check the
			<a href="https://github.com/BruceDevices/firmware/tree/main/media/connections">connections</a>
			or <a href="https://wiki.bruce.computer">Wiki</a>!
		</p>
		<p>Every feature is also listed on Github.</p>
	</div>
</section>

<style>
	.scroll-gradient {
		position: absolute;
		top: 0;
		right: 0;
		bottom: 0;
		width: 100px;
		background: linear-gradient(to left, rgba(10, 10, 10, 0.95), transparent);
		pointer-events: none;
		z-index: 5;
	}
</style>
