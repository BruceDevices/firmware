<script>
	import { base } from '$app/paths';
	// Dimmed further than before because the redesign puts real body copy over
	// it; the artwork still reads, the text stays legible.
	let { opacity = 0.45 } = $props();
</script>

<img
	src="{base}/img/purple-aqua.gif"
	alt=""
	aria-hidden="true"
	class="pointer-events-none absolute inset-0 h-full w-full object-cover"
	style="opacity: {opacity}"
/>
