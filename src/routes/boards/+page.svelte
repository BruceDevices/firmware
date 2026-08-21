<script>
	import SectionBackground from '$lib/components/SectionBackground.svelte';
	import BoardCard from '$lib/components/BoardCard.svelte';
	import Btn from '$lib/components/Btn.svelte';
	import { current_page, Page } from '$lib/store';

	$current_page = Page.Boards;

	const boardImages = ['/img/bruce-pcb.png', '/img/bruce-pcb2.png'];
	const reaperImages = ['/img/reaper-pcb.png', '/img/reaper-pcb2.png'];

	// Specs are data, not markup — one <dl> renders them all consistently.
	const reaperSpecs = [
		['Main MCU', 'ESP32-S3 N16R8'],
		['Input/Output', '6-way button matrix'],
		['IO Expander', 'AW9523'],
		['RF Modules', 'E01-2G4M27SX (nRF24), E07-433M20S (CC1101)'],
		['NFC Reader/Writer/Emulator', 'ST25R3916'],
		['Peripherals', 'Vibro motor, Buzzer, IR TX/RX, iButton'],
		['Storage', 'microSD card'],
		['GPS Module', 'ATGM336H'],
		['Compatibility', 'Flipper Zero header'],
		['Connectivity', 'Multiple UART & I2C'],
		['Visual', 'Status LEDs']
	];

	const v2Specs = [
		['Main Processor', 'ESP32-S3-WROOM-1 NF'],
		['Memory', '16MB Flash, 8MB Octal PSRAM'],
		['Power Input', 'USB-C, 3.3V regulated'],
		['RF Modules', 'E07-433M20S and E01-2G4M27SX'],
		['GPIO Extender', 'Adafruit AW9523BTQR']
	];
</script>

<!-- Hero -->
<section class="relative overflow-hidden border-b border-[var(--rule)]">
	<SectionBackground />
	<div class="absolute inset-0 bg-gradient-to-r from-[var(--color-ink)] via-[var(--color-ink)]/85 to-transparent"></div>
	<div class="shell relative z-10 py-20 md:py-24">
		<span class="eyebrow">Hardware</span>
		<h1 class="mt-4 text-5xl font-semibold md:text-6xl">Bruce Hardware</h1>
		<p class="lede mt-5 max-w-xl">Official and community hardware designed for Bruce firmware</p>
	</div>
</section>

<div class="shell space-y-6 py-16">
	<BoardCard
		images={reaperImages}
		title="Bruce RF Reaper PCB"
		description="The Bruce RF Reaper is a versatile and powerful development board designed specifically for RF exploration and IoT projects. Created by Smoochiee, this board combines multiple wireless technologies with extensive GPIO capabilities, making it perfect for both beginners or hobbists that want to learn more and advanced users interested in RF and IoT development."
	>
		<h3 class="eyebrow mb-4">Technical Specifications</h3>
		<dl class="grid gap-px overflow-hidden border border-[var(--rule)] bg-[var(--rule)] sm:grid-cols-2 lg:grid-cols-3">
			{#each reaperSpecs as [label, value]}
				<div class="bg-[var(--color-ink)] p-4">
					<dt class="meta">{label}</dt>
					<dd class="mt-1.5 text-sm text-white">{value}</dd>
				</div>
			{/each}
		</dl>

		<div class="mt-8 flex flex-wrap gap-3">
			<a href="https://github.com/BruceDevices/firmware/tree/main/pcbs/Bruce_PCB_smoochie/3d" class="btn btn-outline" style="color:#fff">
				Download 3D case model
			</a>
			<a href="https://www.pcbway.com/project/shareproject/Bruce_RF_Reaper_PCB_5e0e8a73.html" class="btn btn-primary" style="color:#fff">
				Order with PCBWay
			</a>
			<a href="https://www.elecrow.com/bruce-pcb-rf-reaper.html" class="btn btn-primary" style="color:#fff"> Order with Elecrow </a>
		</div>
	</BoardCard>

	<BoardCard
		images={boardImages}
		title="Bruce PCB V2"
		description="The Bruce PCB V2 is a versatile and powerful development board designed specifically for RF exploration and IoT projects. Created by Smoochiee, this board combines multiple wireless technologies with extensive GPIO capabilities, making it perfect for both beginners or hobbists that want to learn more and advanced users interested in RF and IoT development."
	>
		<h3 class="eyebrow mb-4">Some Technical Specifications</h3>
		<dl class="grid gap-px overflow-hidden border border-[var(--rule)] bg-[var(--rule)] sm:grid-cols-2 lg:grid-cols-3">
			{#each v2Specs as [label, value]}
				<div class="bg-[var(--color-ink)] p-4">
					<dt class="meta">{label}</dt>
					<dd class="mt-1.5 text-sm text-white">{value}</dd>
				</div>
			{/each}
		</dl>

		<div class="mt-8 flex flex-wrap gap-3">
			<a href="https://github.com/BruceDevices/firmware/tree/main/pcbs" class="btn btn-outline" style="color:#fff"> Download Gerber and BOM Files </a>
			<a
				href="https://raw.githubusercontent.com/pr3y/Bruce/refs/heads/main/pcbs/Bruce_PCB_smoochie/gerber/BRUCE_MANUAL_BUILD.zip"
				class="btn btn-outline"
				style="color:#fff"
			>
				Download Manual Build Files
			</a>
			<a href="https://github.com/BruceDevices/firmware/tree/main/pcbs/Bruce_PCB_smoochie/3d" class="btn btn-outline" style="color:#fff">
				Download 3D case model
			</a>
			<a href="https://www.pcbway.com/project/shareproject/Bruce_PCB_Smoochiee_d6a0284b.html" class="btn btn-primary" style="color:#fff">
				Order with PCBWay
			</a>
			<a href="https://www.elecrow.com/bruce-pcb-v2-smoochiee-1.html" class="btn btn-primary" style="color:#fff"> Order with Elecrow </a>
		</div>
	</BoardCard>

	<div class="grid gap-6 md:grid-cols-2">
		<BoardCard images={['/img/bruce_ultramarines.png']} title="Bruce Ultramarines" description="Compact PCB to use with m5sticks.">
			<Btn href="https://github.com/BruceDevices/firmware/tree/main/pcbs/M5Stick_Intermidiate_ultramarines">Download files</Btn>
		</BoardCard>

		<BoardCard images={['/img/pirata-pcb.png']} title="Pirata PCB" description="Pirata's PCB to m5sticks with NRF24 and CC1101 switches support.">
			<Btn href="https://github.com/BruceDevices/firmware/tree/main/pcbs/StickCPlus_PCB_Pirata">Download files</Btn>
		</BoardCard>
	</div>

	<p class="border-t border-[var(--rule)] pt-10 text-center text-sm text-[var(--text-faint)]">More coming soon...</p>
</div>

<!-- Partner -->
<section class="shell pb-20">
	<div class="panel p-8 md:p-12">
		<span class="eyebrow">Partner With Us</span>
		<h2 class="mt-3 max-w-2xl text-3xl font-semibold">Resellers, sponsors and manufacturing partners</h2>
		<div class="mt-6 max-w-3xl space-y-4 text-sm leading-relaxed text-[var(--text-dim)]">
			<p>
				Are you interested in becoming a trusted reseller or sponsor for the Bruce project? We welcome partnerships with companies and investors who
				share our vision for open-source hardware development like Elecrow and PCBWay.
			</p>
			<p>
				For partnership inquiries, please contact us at
				<a href="mailto:contact@bruce.computer">contact@bruce.computer</a>
				with your company details and proposal.
			</p>
		</div>
	</div>
</section>
