<script>
	import Icon from '$lib/components/Icon.svelte';

	let props = $props();

	let isOpen = $state(false);

	function toggleDropdown() {
		isOpen = !isOpen;
	}

	function handleClickOutside(event) {
		if (!event.target.closest('.relative.inline-block')) {
			isOpen = false;
		}
	}
</script>

<svelte:window onclick={handleClickOutside} />

<div class="relative inline-block text-left">
	<button
		type="button"
		class="inline-flex items-center gap-1.5 rounded-[3px] px-3 py-2 text-sm text-[var(--text-dim)] transition-colors hover:text-white"
		onclick={toggleDropdown}
		aria-expanded={isOpen}
	>
		{props.title}
		<span class="transition-transform duration-200" class:rotate-180={isOpen}>
			<Icon name="chevron-down" size={14} />
		</span>
	</button>

	{#if isOpen}
		<div
			class="absolute right-0 z-50 mt-1 w-56 overflow-hidden rounded-[3px] border border-[var(--rule-strong)] bg-[var(--color-surface)] py-1 shadow-2xl"
		>
			{#each props.links as link}
				<a href={link.href} class="block px-3 py-2 text-sm text-[var(--text-dim)] transition-colors hover:bg-white/5 hover:text-white">
					{link.title}
				</a>
			{/each}
		</div>
	{/if}
</div>
