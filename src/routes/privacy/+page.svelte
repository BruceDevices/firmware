<script>
	import Privacy from './privacy.svelte';
	import { current_page, Page } from '$lib/store';

	// Not a nav route, but it still has to clear whichever item was lit before.
	$current_page = Page.Other;
</script>

<Privacy />
