<script>
	import Btn from '$lib/components/Btn.svelte';
	import { base } from '$app/paths';
	import { current_page, Page } from '$lib/store';
	$current_page = Page.Donate;
</script>

<main class="rounded-lg bg-gradient-to-br from-[#3d0664] via-[#4b0082] to-[#2a0036] px-5 py-15 text-center shadow-2xl">
	<section class="intro">
		<h1 class="mt-10 mb-10 text-5xl font-bold" data-i18n="donate_heading">Support the Bruce Project</h1>
		<p class="mx-auto mb-5 max-w-2xl text-xl" data-i18n="donate_intro">Show your support for Bruce Projects through various donation platforms.</p>
		<h3 class="mt-8 text-2xl" data-i18n="donate_subheading">Help true open-source software and hardware thrive!</h3>
		<p class="mx-auto mb-5 max-w-2xl text-xl" data-i18n="donate_text">
			Every contribution helps push the Bruce Project forward. Join our Discord community for a Donator role!
		</p>
	</section>

	<section>
		<h2 class="mt-8 text-2xl" data-i18n="donation_platforms">Donation Platforms</h2>
		<div class="mt-5 flex flex-wrap justify-center gap-4">
			<Btn href="https://www.patreon.com/c/BruceFirmware">Patreon</Btn>
			<Btn href="https://ko-fi.com/brucefw">Ko-fi</Btn>
			<Btn href="https://opencollective.com/brucefirmware">OpenCollective</Btn>
		</div>
	</section>

	<section>
		<h2 class="mt-8 text-2xl" data-i18n="crypto_donations">Cryptocurrency Donations</h2>
		<div class="mt-5 flex flex-wrap justify-center gap-4">
			<div class="crypto">
				<h3 class="mt-8 text-2xl" data-i18n="btc">Bitcoin (BTC)</h3>
				<a href="bitcoin:bc1ql5uvqda7ndxjkcf7c2kw9vv2rcpmqsjs5xzsq3">
					<img class="m-2.5 h-38 w-38 rounded-lg bg-white p-2.5" src="{base}/img/btc.png" alt="BTC QR Code" />
				</a>
			</div>
			<div class="crypto">
				<h3 class="mt-8 text-2xl" data-i18n="xmr">Monero (XMR)</h3>
				<a href="monero:89GEo1iF4CB8ALkXJgwmUCDrRkYgHnQvZgK23Q2AKBPRdXBMXXzmYQ82sSTzFkKnmePcm85153XwZ7xbukmsG4RY5GyAN9r">
					<img class="m-2.5 h-38 w-38 rounded-lg bg-white p-2.5" src="{base}/img/xmr.png" alt="XMR QR Code" />
				</a>
			</div>
		</div>
	</section>

	<section>
		<h2 class="mt-8 text-2xl" data-i18n="contributor_support">Support the Contributors</h2>
		<div class="mt-5 flex flex-wrap justify-center gap-4">
			<Btn href="https://buymeacoffee.com/bmorcelliz">Donate to bmorcelli</Btn>
			<Btn href="https://github.com/sponsors/bmorcelli">Sponsor on GitHub</Btn>
			<Btn href="https://www.paypal.com/paypalme/smoochieelee">Donate to Smoochiee (PCB Design)</Btn>
		</div>
	</section>
</main>
