<script lang="ts">
	export let href: string | undefined = undefined;
	export let className: string = '';
	export let outline: boolean = false;
</script>

<!-- Renders as a link when href is given, otherwise as a real button, so the
     onclick handlers in Bruce Lab get proper keyboard/AT semantics. -->
{#if href}
	<a {href} class="btn {outline ? 'btn-outline' : 'btn-primary'} {className}" {...$$restProps} style="color:#fff">
		<slot />
	</a>
{:else}
	<button type="button" class="btn {outline ? 'btn-outline' : 'btn-primary'} {className}" {...$$restProps}>
		<slot />
	</button>
{/if}
