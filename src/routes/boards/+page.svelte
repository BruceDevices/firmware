<script>
	import SectionBackground from '$lib/components/SectionBackground.svelte';
	import BoardCard from '$lib/components/BoardCard.svelte';
	import Btn from '$lib/components/Btn.svelte';

	const boardImages = ['/img/bruce-pcb.png', '/img/bruce-pcb2.png'];
	const reaperImages = ['/img/reaper-pcb.png', '/img/reaper-pcb2.png'];
</script>

<!-- Hero Section -->
<section class="relative flex h-96 w-full flex-col overflow-hidden pr-4 pl-4 md:flex-row">
	<SectionBackground />
	<div class="relative z-10 flex flex-col justify-center p-8 text-white">
		<h1 class="mb-5 text-6xl font-bold max-sm:mt-20">Bruce Hardware</h1>
		<p class="mb-7 text-xl">Official and community hardware designed for Bruce firmware</p>
	</div>
</section>


<div class="min-h-screen p-8 text-white">
        <BoardCard
                images={reaperImages}
                title="Bruce RF Reaper PCB"
                description="The Bruce RF Reaper is a versatile and powerful development board designed specifically for RF exploration and IoT projects. Created by Smoochiee, this board combines multiple wireless technologies with extensive GPIO capabilities, making it perfect for both beginners or hobbists that want to learn more and advanced users interested in RF and IoT development."
        >
                <!-- Rest of your content stays exactly the same -->
                <!-- Technical Specifications -->
                <div class="mb-8">
                        <h3 class="mb-6 text-2xl font-bold text-white">Technical Specifications</h3>
                        <div class="grid grid-cols-1 gap-6 md:grid-cols-2 lg:grid-cols-3">
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Main MCU</span>
                                        <span class="text-gray-300">ESP32-S3 N16R8</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Input/Output</span>
                                        <span class="text-gray-300">6-way button matrix</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">IO Expander</span>
                                        <span class="text-gray-300">AW9523</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">RF Modules</span>
                                        <span class="text-gray-300">E01-2G4M27SX (nRF24), E07-433M20S (CC1101)</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">NFC Reader/Writer/Emulator</span>
                                        <span class="text-gray-300">ST25R3916</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Peripherals</span>
                                        <span class="text-gray-300">Vibro motor, Buzzer, IR TX/RX, iButton</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Storage</span>
                                        <span class="text-gray-300">microSD card</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">GPS Module</span>
                                        <span class="text-gray-300">ATGM336H</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Compatibility</span>
                                        <span class="text-gray-300">Flipper Zero header</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Connectivity</span>
                                        <span class="text-gray-300">Multiple UART & I2C</span>
                                </div>
                                <div class="rounded-lg border border-white/10 bg-white/5 p-4">
                                        <span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Visual</span>
                                        <span class="text-gray-300">Status LEDs</span>
                                </div>
                        </div>
                </div>

                <!-- Action Buttons -->
                <div class="flex flex-wrap justify-center gap-4 pt-8">
                        <a href="https://github.com/BruceDevices/firmware/tree/main/pcbs/Bruce_PCB_smoochie/3d" class="min-w-48 rounded-md border border-white/20 bg-white/10 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-white/20">
                                Download 3D case model
                        </a>
                        <a href="https://www.pcbway.com/project/shareproject/Bruce_RF_Reaper_PCB_5e0e8a73.html" class="min-w-48 rounded-md bg-purple-600 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-purple-500">
                                <b class="text-white">Order with PCBWay</b>
                        </a>
                        <a href="https://www.elecrow.com/bruce-pcb-rf-reaper.html" class="min-w-48 rounded-md bg-purple-600 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-purple-500">
                                <b class="text-white">Order with Elecrow</b>
                        </a>
                </div>
        </BoardCard>
</div>




<div class="min-h-screen p-8 text-white">
	<BoardCard
		images={boardImages}
		title="Bruce PCB V2"
		description="The Bruce PCB V2 is a versatile and powerful development board designed specifically for RF exploration and IoT projects. Created by Smoochiee, this board combines multiple wireless technologies with extensive GPIO capabilities, making it perfect for both beginners or hobbists that want to learn more and advanced users interested in RF and IoT development."
	>
		<!-- Technical Specifications -->
		<div class="mb-8">
			<h3 class="mb-6 text-2xl font-bold text-white">Some Technical Specifications</h3>
			<div class="grid grid-cols-1 gap-6 md:grid-cols-2 lg:grid-cols-3">
				<div class="rounded-lg border border-white/10 bg-white/5 p-4">
					<span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Main Processor</span>
					<span class="text-gray-300">ESP32-S3-WROOM-1 NF</span>
				</div>
				<div class="rounded-lg border border-white/10 bg-white/5 p-4">
					<span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Memory</span>
					<span class="text-gray-300">16MB Flash, 8MB Octal PSRAM</span>
				</div>
				<div class="rounded-lg border border-white/10 bg-white/5 p-4">
					<span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">Power Input</span>
					<span class="text-gray-300">USB-C, 3.3V regulated</span>
				</div>
				<div class="rounded-lg border border-white/10 bg-white/5 p-4">
					<span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">RF Modules</span>
					<span class="text-gray-300">E07-433M20S and E01-2G4M27SX</span>
				</div>
				<div class="rounded-lg border border-white/10 bg-white/5 p-4">
					<span class="mb-2 block text-sm font-bold tracking-wide text-purple-400 uppercase">GPIO Extender</span>
					<span class="text-gray-300">Adafruit AW9523BTQR</span>
				</div>
			</div>
		</div>

		<!-- Action Buttons -->
		<div class="flex flex-wrap justify-center gap-4 pt-8">
			<a
				href="https://github.com/BruceDevices/firmware/tree/main/pcbs"
				class="min-w-48 rounded-md border border-white/20 bg-white/10 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-white/20"
			>
				Download Gerber and BOM Files
			</a>
			<a
				href="https://raw.githubusercontent.com/pr3y/Bruce/refs/heads/main/pcbs/Bruce_PCB_smoochie/gerber/BRUCE_MANUAL_BUILD.zip"
				class="min-w-48 rounded-md border border-white/20 bg-white/10 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-white/20"
			>
				Download Manual Build Files
			</a>
			<a
				href="https://github.com/BruceDevices/firmware/tree/main/pcbs/Bruce_PCB_smoochie/3d"
				class="min-w-48 rounded-md border border-white/20 bg-white/10 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-white/20"
			>
				Download 3D case model
			</a>
			<a
				href="https://www.pcbway.com/project/shareproject/Bruce_PCB_Smoochiee_d6a0284b.html"
				class="min-w-48 rounded-md bg-purple-600 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-purple-500"
			>
				<b class="text-white">Order with PCBWay</b>
			</a>
      <a
				href="https://www.elecrow.com/bruce-pcb-v2-smoochiee-1.html"
				class="min-w-48 rounded-md bg-purple-600 px-6 py-3 text-center font-bold text-white transition-all duration-300 hover:bg-purple-500"
			>
				<b class="text-white">Order with Elecrow</b>
			</a>
		</div>
	</BoardCard>
</div>

<div class="grid grid-cols-2 gap-4 p-4 max-sm:grid-cols-1">
	<BoardCard
		className="max-sm:w-full w-128 h-140"
		images={['/img/bruce_ultramarines.png']}
		title="Bruce Ultramarines"
		description="Compact PCB to use with m5sticks."
	>
		<div class="mt-13 flex justify-center">
			<Btn href="https://github.com/BruceDevices/firmware/tree/main/pcbs/M5Stick_Intermidiate_ultramarines">Download files</Btn>
		</div>
	</BoardCard>

	<BoardCard
		className="max-sm:w-full w-128 h-140"
		images={['/img/pirata-pcb.png']}
		title="Pirata PCB"
		description="Pirata's PCB to m5sticks with NRF24 and CC1101 switches support."
	>
		<div class="flex justify-center">
			<Btn href="https://github.com/BruceDevices/firmware/tree/main/pcbs/StickCPlus_PCB_Pirata">Download files</Btn>
		</div>
	</BoardCard>
</div>

<h1 class="mt-16 text-center text-4xl font-bold">More coming soon...</h1>
<!-- Partner Section -->
<div class="py-16">
	<div class="container mx-auto px-4">
		<h1 class="mb-8 text-center text-4xl font-bold">Partner With Us</h1>
		<div class="mx-auto max-w-4xl space-y-6 text-center">
			<p class="text-lg leading-relaxed text-gray-300">
				Are you interested in becoming a trusted reseller or sponsor for the Bruce project? We welcome partnerships with companies and investors who
				share our vision for open-source hardware development like Elecrow and PCBWay.
			</p>
			<p class="text-lg leading-relaxed text-gray-300">
				For partnership inquiries, please contact us at
				<a href="mailto:contact@bruce.computer" class="text-purple-400 underline hover:text-purple-300"> contact@bruce.computer </a>
				with your company details and proposal.
			</p>
		</div>
	</div>
</div>
