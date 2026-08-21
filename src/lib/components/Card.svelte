<script lang="ts">
	// `index` prints a monospaced ordinal in the corner — a structural marker
	// that gives the grid rhythm without resorting to decorative icon badges.
	export let index: string | number | undefined = undefined;
</script>

<article class="panel panel-interactive relative flex h-full flex-col p-6 text-left">
	{#if index !== undefined}
		<span class="meta absolute top-5 right-5 tabular-nums">{index}</span>
	{/if}
	<slot />
</article>
