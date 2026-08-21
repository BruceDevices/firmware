<script>
	import { base } from '$app/paths';
	import Icon from '$lib/components/Icon.svelte';

	let { images = [], title = '', description = '', className, children } = $props();

	let currentImageIndex = $state(0);
	let intervalId = $state(null);

	function nextImage() {
		currentImageIndex = (currentImageIndex + 1) % images.length;
	}

	function prevImage() {
		currentImageIndex = (currentImageIndex - 1 + images.length) % images.length;
	}

	function startInterval() {
		if (images.length > 1) {
			intervalId = setInterval(() => {
				currentImageIndex = (currentImageIndex + 1) % images.length;
			}, 3000);
		}
	}

	function pauseInterval() {
		if (intervalId) {
			clearInterval(intervalId);
			intervalId = null;
		}
	}

	function resumeInterval() {
		startInterval();
	}

	// Start the interval when component mounts
	$effect(() => {
		startInterval();
		return () => {
			if (intervalId) {
				clearInterval(intervalId);
			}
		};
	});
</script>

<article class="panel overflow-hidden {className}">
	<!-- Image plate -->
	<div class="group relative border-b border-[var(--rule)] bg-black/40">
		<img
			src={base + images[currentImageIndex]}
			alt={title}
			class="h-72 w-full object-contain p-6"
			onmouseenter={pauseInterval}
			onmouseleave={resumeInterval}
		/>

		{#if images.length > 1}
			<div
				class="absolute inset-x-0 top-1/2 flex -translate-y-1/2 items-center justify-between px-4 opacity-0 transition-opacity duration-200 group-hover:opacity-100"
			>
				<button
					onclick={prevImage}
					aria-label="Previous image"
					class="inline-flex h-9 w-9 items-center justify-center rounded-[3px] border border-[var(--rule-strong)] bg-black/70 text-white transition-colors hover:border-[var(--color-brand)]"
				>
					<Icon name="chevron-left" size={16} />
				</button>
				<button
					onclick={nextImage}
					aria-label="Next image"
					class="inline-flex h-9 w-9 items-center justify-center rounded-[3px] border border-[var(--rule-strong)] bg-black/70 text-white transition-colors hover:border-[var(--color-brand)]"
				>
					<Icon name="chevron-right" size={16} />
				</button>
			</div>

			<!-- Position marker: ticks, not dots, matching the hero carousel. -->
			<div class="absolute bottom-4 left-1/2 flex -translate-x-1/2 gap-1.5">
				{#each images as _, i}
					<button
						onclick={() => (currentImageIndex = i)}
						aria-label="Image {i + 1}"
						aria-current={i === currentImageIndex}
						class="h-[3px] w-6 transition-colors {i === currentImageIndex ? 'bg-[var(--color-brand)]' : 'bg-white/25 hover:bg-white/45'}"
					></button>
				{/each}
			</div>
		{/if}
	</div>

	<div class="p-8">
		<h2 class="text-2xl font-semibold">{title}</h2>
		<p class="mt-3 max-w-3xl text-sm leading-relaxed text-[var(--text-dim)]">
			{description}
		</p>

		<div class="mt-8">
			{@render children()}
		</div>
	</div>
</article>
