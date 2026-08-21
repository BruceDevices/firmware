<script>
	export let href = '';
	export let target = '_self';
	export let variant = 'normal'; // 'normal' or 'install'
	export let selected = false;
</script>

{#if variant === 'install'}
	<a {href} {target} class="btn btn-primary ml-2 text-[0.8125rem]" style="color:#fff">
		<slot />
	</a>
{:else}
	<a
		{href}
		{target}
		class="relative rounded-[3px] px-3 py-2 text-[0.875rem] transition-colors duration-150 {selected
			? 'text-white'
			: 'text-[var(--text-dim)] hover:text-white'}"
		style="color:inherit"
		aria-current={selected ? 'page' : undefined}
	>
		<slot />
		{#if selected}
			<!-- Underline marker rather than a filled pill, so the bar reads as a
			     row of links instead of a strip of buttons. -->
			<span class="absolute inset-x-3 -bottom-[1.35rem] h-[2px] bg-[var(--color-brand)]"></span>
		{/if}
	</a>
{/if}
