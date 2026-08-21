<script>
	export let title;
	export let eyebrow = '';
</script>

<article class="panel panel-interactive flex h-full flex-col p-6 text-left">
	{#if eyebrow}
		<span class="eyebrow mb-3 block">{eyebrow}</span>
	{/if}
	<h3 class="mb-3 text-xl font-semibold">{title}</h3>
	<div class="flex flex-1 flex-col justify-between gap-5">
		<slot />
	</div>
</article>
