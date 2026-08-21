<script lang="ts">
	import '../app.css';
	import NavLink from '$lib/components/NavLink.svelte';
	import Icon from '$lib/components/Icon.svelte';
	let { children } = $props();
	import { base } from '$app/paths';
	import { page } from '$app/state';
	// Mobile nav
	let navOpen = $state(false);

	/*
	 * The active item is derived from the URL rather than the `current_page`
	 * store. That store is module-level mutable state: on the server it is
	 * shared between requests, and each page assigns it *after* the layout has
	 * already rendered the header — so the SSR'd markup highlighted whatever
	 * route happened to render last. Reading the pathname is per-request and
	 * correct on the first paint.
	 */
	const nav = [
		{ href: `${base}/`, label: 'Home', match: '/' },
		{ href: 'https://github.com/pr3y/Bruce', label: 'GitHub', external: true },
		{ href: `${base}/flasher`, label: 'Install', match: '/flasher' },
		{ href: 'https://wiki.bruce.computer', label: 'Docs', external: true },
		{ href: `${base}/appstore`, label: 'App Store', match: '/appstore' },
		{ href: `${base}/build_theme.html`, label: 'Theme Builder' },
		{ href: `${base}/my_bruce`, label: 'Bruce Lab', match: '/my_bruce' },
		{ href: `${base}/boards`, label: 'Boards', match: '/boards' },
		{ href: `${base}/donate`, label: 'Donate', match: '/donate' }
	];

	// Strip the base path and any trailing slash so "/bruce/boards/" and
	// "/boards" both resolve to the same key.
	let path = $derived.by(() => {
		let p = page.url.pathname;
		if (base && p.startsWith(base)) p = p.slice(base.length);
		return p.replace(/\/+$/, '') || '/';
	});

	const isActive = (item: { match?: string }) => item.match !== undefined && path === item.match;

	const social = [
		{ href: 'https://discord.gg/WJ9XF9czVT', img: 'discord.svg', label: 'Discord' },
		{ href: 'https://youtube.com/@Bruce-fw', img: 'youtube.svg', label: 'YouTube' },
		{ href: 'https://reddit.com/r/brucefw', img: 'reddit.svg', label: 'Reddit' },
		{ href: 'https://www.instagram.com/bruce_firmware/', img: 'instagram.svg', label: 'Instagram' },
		{ href: 'mailto:contact@bruce.computer', img: 'email.svg', label: 'Email' },
		{ href: 'https://matrix.to/#/#general:matrix.bruce.computer', img: 'matrix.svg', label: 'Matrix' }
	];
</script>

<svelte:head>
	<title>Bruce Firmware</title>
	<meta charset="UTF-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1.0" />
	<meta name="description" content="Predatory ESP32 Firmware Bruce" />
</svelte:head>

<header class="fixed top-0 left-0 z-[100] w-full border-b border-[var(--rule)] bg-[var(--color-surface)]/92 backdrop-blur-md">
	<div class="shell flex h-16 items-center justify-between gap-6">
		<a href={base || '/'} class="flex shrink-0 items-center" aria-label="Bruce Firmware, home">
			<img src="{base}/img/bruce.png" alt="Bruce" class="h-9 w-auto" />
		</a>

		<nav class="hidden items-center gap-0.5 lg:flex" aria-label="Main">
			{#each nav as item}
				<NavLink
					href={item.href}
					target={item.external ? '_blank' : '_self'}
					selected={isActive(item)}
					variant={item.label === 'Install' ? 'install' : 'normal'}
				>
					{item.label}
				</NavLink>
			{/each}
		</nav>

		<button
			class="-mr-2 inline-flex h-10 w-10 items-center justify-center rounded-[3px] text-white transition-colors hover:bg-white/5 lg:hidden"
			onclick={() => (navOpen = true)}
			aria-label="Open navigation"
			aria-expanded={navOpen}
		>
			<Icon name="menu" size={20} />
		</button>
	</div>
</header>

{#if navOpen}
	<div class="fixed inset-0 z-[1000] flex flex-col bg-[var(--color-surface)] lg:hidden">
		<div class="shell flex h-16 shrink-0 items-center justify-between border-b border-[var(--rule)]">
			<img src="{base}/img/bruce.png" alt="Bruce" class="h-9 w-auto" />
			<button
				class="-mr-2 inline-flex h-10 w-10 items-center justify-center rounded-[3px] text-white transition-colors hover:bg-white/5"
				onclick={() => (navOpen = false)}
				aria-label="Close navigation"
			>
				<Icon name="close" size={20} />
			</button>
		</div>
		<nav class="shell flex flex-col divide-y divide-[var(--rule)] overflow-y-auto py-2" aria-label="Main">
			{#each nav as item}
				<a
					href={item.href}
					target={item.external ? '_blank' : '_self'}
					onclick={() => (navOpen = false)}
					class="flex items-center justify-between py-4 text-base text-white"
					style="color:#fff"
				>
					<span class={isActive(item) ? 'text-[var(--color-brand)]' : ''}>{item.label}</span>
					<span class="text-[var(--text-faint)]">
						<Icon name={item.external ? 'external' : 'arrow-right'} size={16} />
					</span>
				</a>
			{/each}
		</nav>
	</div>
{/if}

<main class="pt-16">
	{@render children()}
</main>

<!-- No top margin: `main` absorbs the slack, so a margin here would push the
     page past the viewport and add a scrollbar on short routes. -->
<footer class="border-t border-[var(--rule)] bg-[var(--color-surface)]">
	<div class="shell py-12">
		<div class="flex flex-col gap-8 sm:flex-row sm:items-start sm:justify-between">
			<div class="max-w-sm">
				<img src="{base}/img/bruce.png" alt="Bruce" class="h-9 w-auto" />
				<p class="mt-4 text-sm leading-relaxed text-[var(--text-dim)]">
					The powerful open-source ESP32 firmware designed for offensive security and Red Team operations.
				</p>
			</div>

			<div class="flex flex-col gap-4">
				<span class="eyebrow">Community</span>
				<div class="flex flex-wrap items-center gap-2">
					{#each social as s}
						<a
							href={s.href}
							target={s.href.startsWith('mailto:') ? '_self' : '_blank'}
							rel="noopener noreferrer"
							title={s.label}
							aria-label={s.label}
							class="inline-flex h-10 w-10 items-center justify-center rounded-[3px] border border-[var(--rule)] transition-colors hover:border-[var(--rule-strong)] hover:bg-white/5"
						>
							<img src="{base}/img/{s.img}" alt="" class="h-5 w-5" />
						</a>
					{/each}
				</div>
			</div>
		</div>

		{#if path === '/flasher'}
			<hr class="rule my-8" />
			<p class="meta leading-relaxed" data-i18n="footer_note">
				Flasher customized by
				<a href="https://github.com/emericklaw">emericklaw</a>,
				<a href="https://unveroleone.com">unveroleone</a>,
				<a href="https://github.com/bmorcelli">bmorcelli</a> and
				<a href="https://github.com/pr3y">pr3y</a> — Installer powered by
				<a href="https://esphome.github.io/esp-web-tools/" class="inline-flex items-center gap-1">
					ESP Web Tools <Icon name="wrench" size={12} class="inline-block" />
				</a>
			</p>
		{/if}
	</div>
</footer>
