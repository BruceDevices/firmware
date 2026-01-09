<script lang="ts">
	import { base } from '$app/paths';

	let { card, image, category, title, subtitle, author, slideshow, source, filename, download_link, children } = $props();
	import Carousel from '$lib/components/Carousel.svelte';
	import { capitalize } from '$lib/helper';
	import Btn from './Btn.svelte';
	import categories from '$lib/data/categories.json';

	function get_icon(cat: string): string {
		const tmp = categories.filter((val) => val.name == cat);
		if (tmp.length == 0) return '';
		else return tmp[0].emoji;
	}
</script>

{#if card}
	<div class="mx-auto mt-5 mb-5 max-w-md overflow-hidden rounded-xl bg-gray-800 shadow-2xl">
		<!-- Placeholder Image -->
		<div class="relative">
			<div class="flex h-64 w-full items-center justify-center bg-gradient-to-br">
				<img src="{base}/img/{image}" class="h-48" alt={image} />
			</div>

			<!-- Icon -->
			<div class="bg-opacity-50 absolute top-4 right-4 flex items-center space-x-2 rounded-full bg-black px-3 py-1">
				<span class="text-xs font-medium text-white">{get_icon(category)} {capitalize(category)}</span>
			</div>
		</div>

		<!-- Card Info Section -->
		<div class="bg-gray-800 p-6 text-white">
			<div class="mb-4 flex items-center justify-between">
				<div class="flex-1">
					<h2 class="mb-1 text-xl font-bold text-white">{title}</h2>
					<p class="text-sm text-gray-400">{subtitle}</p>
				</div>

				<!-- Install Button. Not implemented yet -->
				<!-- <button class="rounded-lg bg-orange-500 px-6 py-2 font-bold text-white transition-colors hover:bg-orange-600"> INSTALL </button> -->
			</div>

			<!-- Author Section -->
			<div class="border-t border-gray-700 pt-4">
				<div class="flex items-center justify-center space-x-3">
					<div>
						<p class="text-sm font-medium text-white">Created by {author}</p>
					</div>
				</div>
			</div>
		</div>
	</div>
{:else}
	<div class="relative h-96 w-full overflow-hidden bg-gradient-to-r from-indigo-900 via-purple-600 to-gray-900">
		<!-- Background placeholder -->
		<div class="absolute inset-0 flex items-center justify-center bg-gray-700 opacity-50"></div>

		<!-- Content overlay -->
		<div class="relative z-10 flex h-full items-center px-8">
			<!-- Left side - App info -->
			<div class="flex-1 text-white">
				<h1 class="mb-2 text-5xl font-bold">{title}</h1>
				<p class="mb-1 text-sm text-gray-300">{subtitle}</p>
				<!-- <p class="text-xs text-gray-400 mb-6">Contains ads · In-app purchases</p> -->

				<!-- App icon and stats -->
				<div class="mb-6 flex items-center">
					<!-- Stats. Maybe in future -->
					<!-- <div class="flex space-x-6 text-center">
                        <div>
                            <div class="text-lg font-semibold">3.8★</div>
                            <div class="text-xs text-gray-400">110.8M reviews</div>
                        </div>
                        <div>
                            <div class="text-lg font-semibold">5B+</div>
                            <div class="text-xs text-gray-400">Downloads</div>
                        </div>
                        <div>
                            <div class="text-lg font-semibold">🅰️</div>
                            <div class="text-xs text-gray-400">Rated for 3+ · users 0</div>
                        </div>
                    </div> -->
				</div>

				<!-- Action buttons -->
				<div class="flex items-center space-x-4">
					{#if !download_link.includes('http')}
						<Btn href={base + download_link}>Download</Btn>
					{:else}
						<!-- Ignore base if link point to another website -->
						<Btn href={download_link}>Download</Btn>
					{/if}
				</div>
			</div>
		</div>
	</div>

	<div class="mt-10">
		<Carousel items={slideshow} autoPlay={true} interval={4000} />
	</div>

	<div class="p-20">
		<h2 class="mb-10 text-4xl font-bold">Description</h2>
		{@render children()}

		{#if source != null && source != ''}
			<a class="text-2xl font-bold" href={source}><p>Source code</p></a>
		{/if}
		<a class="text-2xl font-bold underline" target="_blank" href="https://github.com/BruceDevices/firmware/edit/WebPage{filename}">
			<p>Improve this page</p>
		</a>
	</div>
{/if}
