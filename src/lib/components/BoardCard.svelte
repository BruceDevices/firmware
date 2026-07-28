<script>
	import { base } from '$app/paths';

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

<!-- Board Card -->
<div class="mx-auto">
	<div class="overflow-hidden rounded-lg border border-white/10 bg-white/5 {className}">
		<!-- Board Image Section -->
		<div class="relative mx-auto max-w-2xl">
			<div class="group relative">
				<img
					src={base + images[currentImageIndex]}
					alt={title}
					class="h-72 w-full rounded-lg object-cover max-sm:p-5"
					onmouseenter={pauseInterval}
					onmouseleave={resumeInterval}
				/>

				<!-- Navigation Buttons -->
				{#if images.length > 1}
					<div class="absolute inset-0 flex items-center justify-between px-5 opacity-0 transition-opacity duration-300 group-hover:opacity-100">
						<button
							onclick={prevImage}
							class="flex h-12 w-12 cursor-pointer items-center justify-center rounded-full border-none bg-black/50 text-lg text-white transition-all duration-300 hover:bg-black/80"
						>
							&lt;
						</button>
						<button
							onclick={nextImage}
							class="flex h-12 w-12 cursor-pointer items-center justify-center rounded-full border-none bg-black/50 text-lg text-white transition-all duration-300 hover:bg-black/80"
						>
							&gt;
						</button>
					</div>
				{/if}
			</div>
		</div>

		<!-- Board Content -->
		<div class="p-8 text-center">
			<h2 class="mb-6 text-3xl font-bold">{title}</h2>

			<div class="mb-8">
				<p class="leading-relaxed text-gray-300">
					{description}
				</p>
			</div>

			<!-- Slot for technical specifications and action buttons -->
			{@render children()}
		</div>
	</div>
</div>
