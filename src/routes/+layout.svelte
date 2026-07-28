<script lang="ts">
	import '../app.css';
	import { current_page, Page } from '$lib/store';
	import NavLink from '$lib/components/NavLink.svelte';
	let { children } = $props();
	import { asset, resolve } from '$app/paths';
	// Mobile nav
	let navOpen = $state(false);
</script>

<svelte:head>
	<title>Bruce Firmware</title>
	<meta charset="UTF-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1.0" />
	<meta name="description" content="Predatory ESP32 Firmware Bruce" />
	<style>
		* {
			box-sizing: border-box;
		}

		html {
			scroll-behavior: smooth;
			overflow-x: hidden !important;
		}

		body {
			font-family: 'Inter', sans-serif;
			margin: 0;
			padding: 0;
			background-color: #0a0a0a;
			color: #ffffff;
		}

		a {
			color: #943ee5;
		}
	</style>
</svelte:head>

<header class="fixed top-0 left-0 z-[100] w-full bg-[#111] py-3.5">
	<div class="mx-auto w-11/12 max-w-full">
		<nav class="flex items-center justify-between">
			<a href={resolve('/')} class="flex flex-shrink-0 items-center">
				<img src={asset('/img/bruce.png')} alt="Bruce Logo" class="h-[50px] w-auto" />
			</a>
			<button
				class="inline-block text-white hover:text-purple-500 lg:hidden"
				onclick={() => (navOpen = true)}
				style="color:white;"
				aria-label="Open navigation">☰</button
			>
			<div class="hidden items-center gap-2 xl:gap-5 lg:flex">
				<NavLink href={resolve('/')} selected={$current_page == Page.Home}>Home</NavLink>
				<NavLink href="https://github.com/BruceDevices/firmware" target="_blank">GitHub</NavLink>
				<NavLink href={resolve('/flasher')} selected={$current_page == Page.Flasher} variant="install">Install</NavLink>
				<NavLink href="https://wiki.bruce.computer" target="_blank">Docs</NavLink>

				<NavLink href={resolve('/appstore')} selected={$current_page == Page.AppStore}>App Store</NavLink>
				<NavLink href={asset('/build_theme.html')}>Theme Builder</NavLink>
				<NavLink href={resolve('/my_bruce')} selected={$current_page == Page.MyBruce}>Bruce Lab</NavLink>
				<NavLink href={resolve('/boards')} selected={$current_page == Page.Boards}>Boards</NavLink>
				<NavLink href={resolve('/donate')} selected={$current_page == Page.Donate}>Donate</NavLink>
				<!-- <Dropdown title="Bruce Lab" links={bruce_lab_links}></Dropdown> -->
				<!-- <NavLink href="/boards">Boards</NavLink>
				<NavLink href="/community">Community</NavLink> -->
			</div>
			{#if navOpen}
				<div class="fixed top-0 z-[1000] flex h-full w-full flex-col items-center bg-[#111] pt-[60px] transition-[right] duration-300 ease-in-out">
					<button class="close-btn" onclick={() => (navOpen = false)} aria-label="Close navigation">✖</button>
					<NavLink href={resolve('/')} selected={$current_page == Page.Home}>Home</NavLink>
					<NavLink href="https://github.com/BruceDevices/firmware" target="_blank">GitHub</NavLink>
					<NavLink href={resolve('/flasher')} selected={$current_page == Page.Flasher} variant="install">Install</NavLink>
					<NavLink href="https://wiki.bruce.computer" target="_blank">Docs</NavLink>
					<NavLink href={resolve('/appstore')} selected={$current_page == Page.AppStore}>App Store</NavLink>
					<NavLink href={asset('/build_theme.html')}>Theme Builder</NavLink>
					<NavLink href={resolve('/my_bruce')} selected={$current_page == Page.MyBruce}>Bruce Lab</NavLink>
					<NavLink href={resolve('/boards')} selected={$current_page == Page.Boards}>Boards</NavLink>
					<NavLink href={resolve('/donate')} selected={$current_page == Page.Donate}>Donate</NavLink>
					<!-- <Dropdown title="Bruce Lab"></Dropdown> -->
				</div>
			{/if}
		</nav>
	</div>
</header>

{@render children()}

<!-- Footer -->
<footer class="bg-[#111] py-5 text-center">
	<div class="mx-auto my-0 w-90 max-w-full">
		<div class="mb-4 flex items-center justify-center gap-5">
			<a href="https://discord.gg/WJ9XF9czVT" target="_blank" rel="noopener noreferrer" class="inline-block">
				<img src={asset('/img/discord.svg')} alt="Discord" class="h-10 w-10 transition-transform duration-200 ease-in-out hover:scale-110" />
			</a>
			<a href="https://youtube.com/@Bruce-fw" target="_blank" rel="noopener noreferrer" class="inline-block">
				<img src={asset('/img/youtube.svg')} alt="YouTube" class="h-10 w-10 transition-transform duration-200 ease-in-out hover:scale-110" />
			</a>
			<a href="https://reddit.com/r/brucefw" target="_blank" rel="noopener noreferrer" class="inline-block">
				<img src={asset('/img/reddit.svg')} alt="Reddit" class="h-10 w-10 transition-transform duration-200 ease-in-out hover:scale-110" />
			</a>
			<a href="https://www.instagram.com/bruce_firmware/" target="_blank" rel="noopener noreferrer" class="inline-block">
				<img src={asset('/img/instagram.svg')} alt="Instagram" class="h-10 w-10 transition-transform duration-200 ease-in-out hover:scale-110" />
			</a>
			<a href="mailto:contact@bruce.computer" class="inline-block">
				<img src={asset('/img/email.svg')} alt="Email" class="h-10 w-10 transition-transform duration-200 ease-in-out hover:scale-110" />
			</a>
			<a href="https://matrix.to/#/#general:matrix.bruce.computer" target="_blank" rel="noopener noreferrer" class="inline-block">
				<img src={asset('/img/matrix.svg')} alt="Matrix" class="h-10 w-10 transition-transform duration-200 ease-in-out hover:scale-110" />
			</a>
		</div>
	</div>
	{#if $current_page == Page.Flasher}
		<p data-i18n="footer_note">
			Flasher customized by
			<a href="https://github.com/emericklaw">emericklaw</a>,
			<a href="https://unveroleone.com">unveroleone</a>,
			<a href="https://github.com/bmorcelli">bmorcelli</a> and
			<a href="https://github.com/pr3y">pr3y</a> — Installer powered by
			<a href="https://esphome.github.io/esp-web-tools/">ESP Web Tools 🛠️</a>
		</p>
	{/if}
</footer>

<style>
</style>
