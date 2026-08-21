<script>
	let props = $props();
</script>

<!-- Definition row: label left, value right, hairline between. Reads like a
     spec sheet, which is what the device readout is. -->
<div class="flex items-baseline justify-between gap-4 border-b border-[var(--rule)] py-2 last:border-b-0">
	<span class="meta shrink-0 tracking-wide">{props.label}</span>
	<span class="truncate text-right font-mono text-sm text-white">{props.value}</span>
</div>
