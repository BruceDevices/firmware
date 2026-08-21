<script>
	import { onMount } from 'svelte';
	import Icon from '$lib/components/Icon.svelte';

	let showBanner = $state(true);
	let autoHide = $state(false);

	onMount(() => {
		// Check if banner should be permanently hidden
		const autoHideEnabled = localStorage.getItem('installationBannerAutoHide');
		if (autoHideEnabled === 'true') {
			autoHide = true;
			showBanner = false;
		}
	});

	// Reactive effect to handle autoHide changes
	$effect(() => {
		if (autoHide) {
			localStorage.setItem('installationBannerAutoHide', 'true');
			showBanner = false;
		} else if (typeof autoHide === 'boolean' && localStorage.getItem('installationBannerAutoHide') === 'true') {
			localStorage.removeItem('installationBannerAutoHide');
			showBanner = true;
		}
	});

	function closeBanner() {
		// Temporary close - just hide for this session
		showBanner = false;
	}
</script>

{#if showBanner}
	<aside class="relative border border-l-2 border-[var(--rule)] border-l-[var(--color-brand)] bg-[var(--wash)] p-5 text-left">
		<button
			class="absolute top-4 right-4 inline-flex h-7 w-7 items-center justify-center rounded-[3px] text-[var(--text-faint)] transition-colors hover:bg-white/5 hover:text-white"
			onclick={closeBanner}
			title="Close banner"
			aria-label="Close banner"
		>
			<Icon name="close" size={15} />
		</button>

		<div class="flex items-center gap-2.5 pr-10 text-[var(--color-brand)]">
			<Icon name="info" size={16} />
			<h3 class="eyebrow" style="color:inherit">Installation</h3>
		</div>

		<p class="mt-3 text-sm leading-relaxed text-white">To install apps or themes, put files on the SD card/LittleFS:</p>

		<dl class="mt-4 grid gap-2 sm:grid-cols-2">
			<div class="flex items-baseline gap-3">
				<dt class="meta w-14 shrink-0">Apps</dt>
				<dd class="font-mono text-xs break-all text-[var(--text-dim)]">/BruceJS/&lt;CategoryName&gt;/File.js</dd>
			</div>
			<div class="flex items-baseline gap-3">
				<dt class="meta w-14 shrink-0">Themes</dt>
				<dd class="font-mono text-xs break-all text-[var(--text-dim)]">/Themes/&lt;ThemeName&gt;/</dd>
			</div>
		</dl>

		<label class="mt-5 flex w-fit cursor-pointer items-center gap-2 border-t border-[var(--rule)] pt-4 text-xs text-[var(--text-faint)]">
			<input type="checkbox" bind:checked={autoHide} class="h-3.5 w-3.5 accent-[var(--color-brand)]" />
			<span>Don't show this banner again</span>
		</label>
	</aside>
{/if}
