<script>
	import { base } from '$app/paths';

	let { items = [], autoPlay = false, interval = 3000, showDots = true, showArrows = true } = $props();

	let currentIndex = $state(0);
	let carouselContainer = $state();
	let autoPlayInterval = $state();

	// Auto-play functionality
	function startAutoPlay() {
		if (autoPlay && items.length > 1) {
			autoPlayInterval = setInterval(() => {
				nextSlide();
			}, interval);
		}
	}

	function stopAutoPlay() {
		if (autoPlayInterval) {
			clearInterval(autoPlayInterval);
			autoPlayInterval = null;
		}
	}

	// Navigation functions
	function nextSlide() {
		currentIndex = (currentIndex + 1) % items.length;
	}

	function prevSlide() {
		currentIndex = currentIndex === 0 ? items.length - 1 : currentIndex - 1;
	}

	function goToSlide(index) {
		currentIndex = index;
	}

	// Lifecycle
	$effect(() => {
		if (autoPlay) {
			startAutoPlay();
		}

		return () => {
			stopAutoPlay();
		};
	});

	// Handle mouse events for auto-play
	function handleMouseEnter() {
		if (autoPlay) stopAutoPlay();
	}

	function handleMouseLeave() {
		if (autoPlay) startAutoPlay();
	}
</script>

<div
	class="relative mx-auto flex justify-center overflow-hidden rounded-lg text-center"
	bind:this={carouselContainer}
	onmouseenter={handleMouseEnter}
	onmouseleave={handleMouseLeave}
	role="region"
	aria-label="Image carousel"
>
	<!-- Carousel Track -->
	<div
		class="flex items-center justify-center shadow-lg transition-transform duration-500 ease-in-out"
		style="transform: translateX(-{currentIndex * 100}%)"
	>
		{#each items as item, index}
			<div class="w-full">
				<img src="{base}/img/{item}" alt={`Slide ${index + 1}`} class="h-auto w-96 object-cover" />
			</div>
		{/each}
	</div>

	<!-- Navigation Arrows -->
	{#if showArrows && items.length > 1}
		<button
			class="absolute top-1/2 left-4 -translate-y-1/2 transform rounded-full bg-black/50 p-2 text-white transition-all duration-200 hover:bg-black/70 focus:ring-2 focus:ring-white focus:outline-none"
			onclick={prevSlide}
			aria-label="Previous slide"
		>
			<svg class="h-6 w-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
				<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 19l-7-7 7-7" />
			</svg>
		</button>

		<button
			class="absolute top-1/2 right-4 -translate-y-1/2 transform rounded-full bg-black/50 p-2 text-white transition-all duration-200 hover:bg-black/70 focus:ring-2 focus:ring-white focus:outline-none"
			onclick={nextSlide}
			aria-label="Next slide"
		>
			<svg class="h-6 w-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
				<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5l7 7-7 7" />
			</svg>
		</button>
	{/if}

	<!-- Dots Indicator -->
	<div class="absolute bottom-4 left-1/2 flex -translate-x-1/2 transform space-x-2">
		{#each items as _, index}
			<button
				class="h-3 w-3 rounded-full transition-all duration-200 focus:ring-2 focus:ring-white focus:outline-none {index === currentIndex
					? 'bg-white'
					: 'bg-white/50 hover:bg-white/70'}"
				onclick={() => goToSlide(index)}
				aria-label="Go to slide {index + 1}"
			></button>
		{/each}
	</div>
</div>
