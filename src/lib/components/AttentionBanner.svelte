<script>
	import { onMount } from 'svelte';
	import Icon from '$lib/components/Icon.svelte';

	let showBanner = $state(true);
	let autoHide = $state(false);

	onMount(() => {
		// Check if banner should be permanently hidden
		const autoHideEnabled = localStorage.getItem('attentionBannerAutoHide');
		if (autoHideEnabled === 'true') {
			autoHide = true;
			showBanner = false;
		}
	});

	// Reactive effect to handle autoHide changes
	$effect(() => {
		if (autoHide) {
			localStorage.setItem('attentionBannerAutoHide', 'true');
			showBanner = false;
		} else if (typeof autoHide === 'boolean' && localStorage.getItem('attentionBannerAutoHide') === 'true') {
			localStorage.removeItem('attentionBannerAutoHide');
			showBanner = true;
		}
	});

	function closeBanner() {
		// Temporary close - just hide for this session
		showBanner = false;
	}

	const points = [
		'Always read the code before executing',
		'Watch for scams selling "premium" scripts',
		'Official resources only via Bruce App Store',
		'Report suspicious activity to our moderators'
	];
</script>

{#if showBanner}
	<!-- Severity is carried by a left rule and the icon colour rather than a
	     filled alert block, so the notice sits inside the page instead of
	     shouting over it. -->
	<aside class="relative border border-l-2 border-[var(--rule)] border-l-yellow-500/80 bg-yellow-500/[0.04] p-5 text-left">
		<button
			class="absolute top-4 right-4 inline-flex h-7 w-7 items-center justify-center rounded-[3px] text-[var(--text-faint)] transition-colors hover:bg-white/5 hover:text-white"
			onclick={closeBanner}
			title="Close banner"
			aria-label="Close banner"
		>
			<Icon name="close" size={15} />
		</button>

		<div class="flex items-center gap-2.5 pr-10 text-yellow-500">
			<Icon name="warning" size={16} />
			<h3 class="eyebrow" style="color:inherit">Attention</h3>
		</div>

		<p class="mt-3 max-w-3xl text-sm leading-relaxed text-white">
			Only trust open-source scripts you can verify! Bruce is open-source under AGPL License - never pay for scripts, firmware forks or themes.
		</p>

		<ul class="mt-4 grid gap-2 sm:grid-cols-2">
			{#each points as point}
				<li class="flex items-start gap-2.5 text-sm leading-relaxed text-[var(--text-dim)]">
					<span class="mt-[0.45rem] h-px w-3 shrink-0 bg-[var(--rule-strong)]"></span>
					<span>{point}</span>
				</li>
			{/each}
		</ul>

		<label class="mt-5 flex w-fit cursor-pointer items-center gap-2 border-t border-[var(--rule)] pt-4 text-xs text-[var(--text-faint)]">
			<input type="checkbox" bind:checked={autoHide} class="h-3.5 w-3.5 accent-[var(--color-brand)]" />
			<span>Don't show this banner again</span>
		</label>
	</aside>
{/if}
