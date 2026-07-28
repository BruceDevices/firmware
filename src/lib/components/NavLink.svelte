<script>
	export let href = '';
	export let target = '_self';
	export let variant = 'normal'; // 'normal' or 'install'
	export let selected = false;
</script>

<a
	{href}
	{target}
	class="{variant === 'install'
		? 'inline-block rounded-lg border-2 border-[#9B51E0] bg-transparent px-6 py-3 font-semibold text-white transition-all duration-300 ease-in-out hover:scale-105 hover:bg-[#9B51E0]'
		: 'rounded-lg px-4 py-2.5 text-white'} {selected ? 'border-b-2 border-purple-500 text-purple-500' : ''}"
>
	<slot />
</a>
