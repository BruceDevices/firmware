<script>
	import Btn from '$lib/components/Btn.svelte';
	import { base } from '$app/paths';
	import { current_page, Page } from '$lib/store';
	$current_page = Page.Donate;

	const platforms = [
		{ href: 'https://www.patreon.com/c/BruceFirmware', label: 'Patreon' },
		{ href: 'https://ko-fi.com/brucefw', label: 'Ko-fi' },
		{ href: 'https://opencollective.com/brucefirmware', label: 'OpenCollective' }
	];

	const contributors = [
		{ href: 'https://buymeacoffee.com/bmorcelliz', label: 'Donate to bmorcelli' },
		{ href: 'https://github.com/sponsors/bmorcelli', label: 'Sponsor on GitHub' },
		{ href: 'https://www.paypal.com/paypalme/smoochieelee', label: 'Donate to Smoochiee (PCB Design)' }
	];

	const crypto = [
		{
			name: 'Bitcoin (BTC)',
			i18n: 'btc',
			img: 'btc.png',
			uri: 'bitcoin:bc1ql5uvqda7ndxjkcf7c2kw9vv2rcpmqsjs5xzsq3',
			address: 'bc1ql5uvqda7ndxjkcf7c2kw9vv2rcpmqsjs5xzsq3'
		},
		{
			name: 'Monero (XMR)',
			i18n: 'xmr',
			img: 'xmr.png',
			uri: 'monero:89GEo1iF4CB8ALkXJgwmUCDrRkYgHnQvZgK23Q2AKBPRdXBMXXzmYQ82sSTzFkKnmePcm85153XwZ7xbukmsG4RY5GyAN9r',
			address: '89GEo1iF4CB8ALkXJgwmUCDrRkYgHnQvZgK23Q2AKBPRdXBMXXzmYQ82sSTzFkKnmePcm85153XwZ7xbukmsG4RY5GyAN9r'
		}
	];
</script>

<!-- The plum gradient is kept from the previous design, moved into the hero
     band only so the rest of the page sits on the standard page background. -->
<section class="border-b border-[var(--rule)] bg-gradient-to-br from-[var(--color-plum-1)] via-[var(--color-plum-2)] to-[var(--color-plum-3)]">
	<div class="shell py-20 md:py-24">
		<span class="eyebrow" style="color:rgba(255,255,255,0.55)">Support</span>
		<h1 class="mt-4 max-w-3xl text-4xl font-semibold md:text-5xl" data-i18n="donate_heading">Support the Bruce Project</h1>
		<p class="mt-5 max-w-2xl text-lg leading-relaxed text-white/75" data-i18n="donate_intro">
			Show your support for Bruce Projects through various donation platforms.
		</p>
		<p class="mt-6 max-w-2xl text-base text-white/70" data-i18n="donate_subheading">Help true open-source software and hardware thrive!</p>
		<p class="mt-2 max-w-2xl text-base text-white/70" data-i18n="donate_text">
			Every contribution helps push the Bruce Project forward. Join our Discord community for a Donator role!
		</p>
	</div>
</section>

<div class="shell space-y-6 py-16">
	<section class="panel p-8">
		<h2 class="eyebrow" data-i18n="donation_platforms">Donation Platforms</h2>
		<div class="mt-5 flex flex-wrap gap-3">
			{#each platforms as p}
				<Btn href={p.href}>{p.label}</Btn>
			{/each}
		</div>
	</section>

	<section class="panel p-8">
		<h2 class="eyebrow" data-i18n="crypto_donations">Cryptocurrency Donations</h2>
		<div class="mt-6 grid gap-6 sm:grid-cols-2">
			{#each crypto as c}
				<div class="flex items-center gap-5">
					<a href={c.uri} class="shrink-0" aria-label={c.name}>
						<img class="h-32 w-32 rounded-[3px] bg-white p-2" src="{base}/img/{c.img}" alt="{c.name} QR Code" />
					</a>
					<div class="min-w-0">
						<h3 class="text-base font-semibold" data-i18n={c.i18n}>{c.name}</h3>
						<p class="mt-2 font-mono text-[0.6875rem] leading-relaxed break-all text-[var(--text-faint)]">{c.address}</p>
					</div>
				</div>
			{/each}
		</div>
	</section>

	<section class="panel p-8">
		<h2 class="eyebrow" data-i18n="contributor_support">Support the Contributors</h2>
		<div class="mt-5 flex flex-wrap gap-3">
			{#each contributors as c}
				<Btn href={c.href} outline>{c.label}</Btn>
			{/each}
		</div>
	</section>
</div>
