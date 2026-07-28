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
		? 'inline-block rounded-lg border-2 border-[#9B51E0] bg-transparent px-6 py-3 font-semibold text-white transition-all duration-300 ease-in-out hover:scale-105 hover:bg-[#9B51E0] hover:!text-white'
		: 'rounded-lg px-2 py-2.5 xl:px-4 text-white border-b-2 border-transparent hover:!border-[#9B51E0] hover:!text-[#9B51E0] transition-colors duration-200'} {selected ? '!border-purple-500 text-purple-500' : ''} whitespace-nowrap"
>
	<slot />
</a>
