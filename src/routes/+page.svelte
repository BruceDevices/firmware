<script lang="ts">
	import { onMount } from 'svelte';
	const VERSION = '1.16';

	import News from '$lib/components/News.svelte';
	import Btn from '$lib/components/Btn.svelte';
	import Icon from '$lib/components/Icon.svelte';
	import CompatibilityTable from '$lib/components/CompatibilityTable.svelte';
	import SectionBackground from '$lib/components/SectionBackground.svelte';
	import { base } from '$app/paths';
	import { current_page, Page } from '$lib/store';

	$current_page = Page.Home;

	let activeIndex = $state(0);
	let slides = [
		{ src: `${base}/img/bruce-pcb.png`, name: 'Bruce PCB V2' },
		{ src: `${base}/img/reaper-pcb2.png`, name: 'Bruce RF Reaper' },
		{ src: `${base}/img/cardputer.png`, name: 'M5Stack Cardputer' },
		{ src: `${base}/img/core2.png`, name: 'M5Stack Core2' },
		{ src: `${base}/img/cyd.png`, name: 'Cheap Yellow Display' },
		{ src: `${base}/img/lilygo.png`, name: 'LilyGo T-Display' },
		{ src: `${base}/img/t-embed.png`, name: 'LilyGo T-Embed' },
		{ src: `${base}/img/m5stick.png`, name: 'M5Stack StickC' }
	];

	let interval: number;
	onMount(() => {
		interval = setInterval(() => {
			activeIndex = (activeIndex + 1) % slides.length;
		}, 3000);
		return () => clearInterval(interval);
	});

	function setSlide(idx: number) {
		activeIndex = idx;
	}
</script>

<!-- Hero -->
<section class="relative overflow-hidden border-b border-[var(--rule)]">
	<SectionBackground />
	<!-- Scrim: keeps the animated background readable behind body text without
	     hiding it, so the original artwork still carries the section. -->
	<div class="absolute inset-0 bg-gradient-to-r from-[var(--color-ink)] via-[var(--color-ink)]/85 to-transparent"></div>

	<div class="shell relative z-10 grid items-center gap-10 py-20 md:grid-cols-[1fr_1fr] md:py-28">
		<div>
			<span class="eyebrow">Open-source ESP32 firmware</span>
			<h1 class="mt-4 text-5xl leading-[1.05] font-semibold md:text-6xl">Bruce Firmware</h1>
			<p class="lede mt-5 max-w-lg">The powerful open-source ESP32 firmware designed for offensive security and Red Team operations.</p>
			<div class="mt-8 flex flex-wrap gap-3">
				<a href="{base}/flasher" class="btn btn-primary" style="color:#fff">
					Install Now <Icon name="arrow-right" size={15} />
				</a>
				<a href="https://github.com/BruceDevices/Firmware" target="_blank" rel="noopener" class="btn btn-outline" style="color:#fff">
					<Icon name="github" size={15} /> Explore GitHub
				</a>
			</div>
		</div>

		<div class="relative hidden md:block">
			<!-- Fixed frame + object-contain: the artwork ranges from 1:1 (device
			     photos) to 2.79:1 (reaper PCB), so sizing by `w-auto` let every
			     aspect ratio settle at a different scale. Each slide now fits the
			     same box. -->
			<div class="relative h-[21rem] w-full">
				{#each slides as slide, i}
					<img
						src={slide.src}
						alt={slide.name}
						class="absolute inset-0 h-full w-full object-contain transition-opacity duration-700"
						style="opacity: {i === activeIndex ? 1 : 0}"
					/>
				{/each}
			</div>
			<div class="mt-3 flex items-center justify-center gap-4">
				<span class="meta w-40 text-right">{slides[activeIndex].name}</span>
				<div class="flex gap-1.5">
					{#each slides as slide, i}
						<button
							onclick={() => setSlide(i)}
							aria-label={slide.name}
							aria-current={i === activeIndex}
							class="h-[3px] w-6 transition-colors {i === activeIndex ? 'bg-[var(--color-brand)]' : 'bg-white/20 hover:bg-white/40'}"
						></button>
					{/each}
				</div>
			</div>
		</div>
	</div>
</section>

<!-- Features -->
<section class="shell py-20">
	<div class="mb-10 max-w-2xl">
		<span class="eyebrow">Why Bruce</span>
		<h2 class="mt-3 text-3xl font-semibold md:text-4xl">Built in the open, for real hardware</h2>
	</div>

	<div class="grid gap-px overflow-hidden border border-[var(--rule)] bg-[var(--rule)] sm:grid-cols-2 lg:grid-cols-3">
		<div class="bg-[var(--color-ink)] p-7">
			<h3 class="mb-3 text-lg font-semibold">True Open-Source</h3>
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">
				Bruce fw is licensed under <a href="https://www.gnu.org/licenses/agpl-3.0.en.html#license-text" target="_blank">AGPL</a>, and Hardware is
				<a href="https://ohwr.org/project/cernohl/-/wikis/uploads/3eff4154d05e7a0459f3ddbf0674cae4/cern_ohl_p_v2.txt" target="_blank"
					>CERN-OHL-P-2.0</a
				>. Free as in freedom.
			</p>
		</div>
		<div class="bg-[var(--color-ink)] p-7">
			<h3 class="mb-3 text-lg font-semibold">Cross-Platform</h3>
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">Bruce runs seamlessly on M5Stack, LilyGo, and other ESP32-based devices.</p>
		</div>
		<div class="bg-[var(--color-ink)] p-7">
			<h3 class="mb-3 text-lg font-semibold">2.4/5Ghz Wi-Fi Attacks</h3>
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">
				Supports Evil Portal, Wardriving, EAPOL handshake capture, Deauth
				<a href="https://github.com/BruceDevices/firmware/?tab=readme-ov-file#wifi" target="_blank">and more</a>.
			</p>
		</div>
		<div class="bg-[var(--color-ink)] p-7">
			<h3 class="mb-3 text-lg font-semibold">Documentation</h3>
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">
				The project has every information about the features and modules supported available on <a href="https://wiki.bruce.computer/">wiki</a>.
			</p>
		</div>
		<div class="bg-[var(--color-ink)] p-7">
			<h3 class="mb-3 text-lg font-semibold">SubGHz and RFID</h3>
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">Supports several modules and devices with frequency transceivers.</p>
		</div>
		<div class="bg-[var(--color-ink)] p-7">
			<h3 class="mb-3 text-lg font-semibold">Active Community</h3>
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">Regular updates and community-driven improvements.</p>
		</div>
	</div>
</section>

<!-- News -->
<section class="shell py-8">
	<div class="mb-8 flex items-end justify-between gap-6">
		<div>
			<span class="eyebrow">News</span>
			<h2 class="mt-3 text-3xl font-semibold">Latest from the project</h2>
		</div>
	</div>

	<div class="grid gap-4 md:grid-cols-2">
		<News title="Release v{VERSION}" eyebrow="Firmware">
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">
				Our new Release is out now! Update your device
				<a href="https://bruce.computer/flasher" target="_blank">now</a>
			</p>
			<div class="flex flex-wrap gap-3">
				<Btn href="https://github.com/BruceDevices/firmware/releases/tag/{VERSION}">Read Changelog</Btn>
			</div>
		</News>

		<News title="Bruce RF Reaper" eyebrow="Hardware">
			<p class="text-sm leading-relaxed text-[var(--text-dim)]">Open Source Bruce PCB, fully compatible with Bruce</p>
			<div class="flex flex-wrap gap-3">
				<Btn href="https://bruce.computer/boards">Download</Btn>
				<Btn href="https://www.elecrow.com/bruce-pcb-rf-reaper.html" outline>Buy</Btn>
			</div>
		</News>
	</div>
</section>

<CompatibilityTable />

<!-- Help -->
<section class="shell py-20">
	<div class="panel flex flex-col items-start gap-6 p-8 md:flex-row md:items-center md:justify-between">
		<div>
			<h2 class="text-2xl font-semibold">Need more help?</h2>
			<p class="mt-2 text-sm text-[var(--text-dim)]">
				Check out our <a href="https://wiki.bruce.computer/faq/" target="_blank" rel="noopener noreferrer">FAQ</a>!
			</p>
		</div>
		<div class="flex flex-wrap gap-3">
			<Btn href="https://discord.gg/WJ9XF9czVT">Join us on Discord!</Btn>
			<Btn href="https://forum.bruce.computer" outline>Join our forum!</Btn>
		</div>
	</div>
</section>
