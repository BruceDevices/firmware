<script lang="ts">
	import { current_page, Page } from '$lib/store';
	import manifests from '$lib/data/manifests.json';
	import SectionBackground from '$lib/components/SectionBackground.svelte';

	$current_page = Page.Flasher;
	let selectedVersion = $state('Last');
	let selectedReleaseDate = $state('');
	let selectedDevice = $state('');
	let selectedCategory = $state('');
	let versionTags: Array<{ tag_name: string; updated_at: string }> = $state([]);

	$effect(() => {
		updateManifest();
	});

	function formatDateTime(str) {
		let date = new Date(str).toLocaleDateString();
		let time = new Date(str).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
		return `${date} ${time}`;
	}

	function downloadFile(file: string) {
		const releaseTag =
			selectedVersion === 'Beta'
				? 'betaRelease'
				: selectedVersion === 'Latest'
					? latestVersionTag
					: document.getElementById('otherReleaseDropdown').value;

		const fileUrl = 'https://github.com/BruceDevices/firmware/releases/download/' + releaseTag + '/Bruce-' + encodeURIComponent(file) + '.bin';

		const link = document.createElement('a');
		link.href = fileUrl;
		link.download = file;
		link.style.display = 'none';
		document.body.appendChild(link);
		link.click();
		document.body.removeChild(link);
	}

	function toggleDeviceCategory(category: string) {
		selectedCategory = category;
		selectedDevice = '';
	}

	function toggleRelease(version: string) {
		selectedVersion = version;
		const otherReleaseContainer = document.getElementById('otherReleaseContainer');
		otherReleaseContainer.style.display = version === 'Other' ? 'block' : 'none';
		updateReleaseDate();
	}

	function updateReleaseDate() {
		let element: HTMLElement | null = null;

		if (selectedVersion === 'Other') {
			const dropdown = document.getElementById('otherReleaseDropdown') as HTMLSelectElement;
			element = dropdown?.options[dropdown.selectedIndex];
		} else {
			element = document.getElementById(selectedVersion === 'Last' ? 'latest' : 'beta');
		}

		selectedReleaseDate = element?.getAttribute('data-release-date') || '';

		document.getElementById('releaseDate').classList.remove('invisible');
	}

	function findDeviceById(id) {
		for (const category of Object.values(manifests)) {
			const device = category.find((d) => d.id === id);
			if (device) return device;
		}
		return null;
	}

	function updateManifest() {
		if (selectedVersion && selectedDevice) {
			const button = document.querySelector('esp-web-install-button');
			if (button) {
				const releaseTag =
					selectedVersion === 'Beta'
						? 'betaRelease'
						: selectedVersion === 'Latest'
							? latestVersionTag
							: document.getElementById('otherReleaseDropdown').value;

				const manifest = {
					name: selectedDevice,
					new_install_prompt_erase: true,
					builds: [
						{
							chipFamily: findDeviceById(selectedDevice).family,
							improv: false,
							parts: [
								{
									path:
										'https://bruce.iceis.co.uk/service/github/' +
										releaseTag +
										'/Bruce-' +
										encodeURIComponent(selectedDevice) +
										'.bin',
									offset: 0
								}
							]
						}
					]
				};

				const json = JSON.stringify(manifest);
				const blob = new Blob([json], { type: 'application/json' });

				button.manifest = URL.createObjectURL(blob);

				button.style.display = 'block';
			}
		}
	}

	const active_el = (first: string, cmp: string) => (first == cmp ? 'bg-[#9B51E0] text-white' : '');

	// Get GitHub release tags
	let latestVersionTag: string = $state('');
	let latestVersionReleaseDate: string = $state('');
	let loading = $state(true);
	let error = $state('');

	const repo = 'pr3y/Bruce';

	$effect(() => {
		fetchTags();
	});

	async function fetchTags() {
		loading = true;
		error = '';
		try {
			const res = await fetch(`https://api.github.com/repos/${repo}/releases?per_page=100`);
			if (!res.ok) throw new Error('Failed to fetch tags');
			const data = await res.json();
			// Only include tags matching x.x or x.x.x, keep both tag_name and updated_at
			versionTags = data.map((release: { tag_name: string; updated_at: string }) => ({
				tag_name: release.tag_name,
				updated_at: release.updated_at
			}));
			latestVersionTag = versionTags.length > 0 ? versionTags[0].tag_name : '';
			latestVersionReleaseDate = versionTags.length > 0 ? versionTags[0].updated_at : '';
			document.getElementById('latest').setAttribute('data-release-date', latestVersionReleaseDate);
			selectedReleaseDate = latestVersionReleaseDate;

			const betaRelease = versionTags.find((r) => r.tag_name === 'betaRelease');
			document.getElementById('beta').setAttribute('data-release-date', betaRelease.updated_at);

			updateReleaseDate();
		} catch (e) {
			error = e.message;
		}
		loading = false;
	}
</script>

<svelte:head>
	<script type="module" src="/vendor/bruce-esp-web-tools/web/install-button.js?module"></script>
</svelte:head>

<section class="relative flex h-[500px] w-full flex-col overflow-hidden pr-4 pl-4 md:flex-row">
	<SectionBackground />
	<div class="relative z-10 flex flex-col justify-center p-8 text-white">
		<h1 data-i18n="hero_title" class="mb-5 text-4xl font-bold md:text-6xl">Bruce Web Flasher</h1>
		<p data-i18n="hero_description" class="mb-7 text-xl">Flash your device easily with our online installer!</p>
	</div>
</section>

<div
	class="mx-auto mt-[30px] max-w-[800px] rounded-xl border-2 border-[#9B51E0] bg-[#9B51E0]/10 p-5 text-white shadow-[0px_0px_10px_rgba(155,81,224,0.3)]"
>
	<h2 class="text-center text-[1.8rem]" data-i18n="flashing_instructions_title">Flashing Instructions</h2>
	<p data-i18n="flashing_instruction_1"><strong>Connect your device, then select "Flash" and click connect.</strong></p>
	<p data-i18n="flashing_instruction_2">If asked to put your device into <strong>download mode</strong>, do the following:</p>
	<ul class="ml-[30px] list-disc pl-[10px]">
		<li data-i18n="flashing_instruction_cardputer">
			<strong>Cardputer:</strong> Turn off and unplug from USB, hold the btn G0 (upper right corner), then connect via USB.
		</li>
		<li data-i18n="flashing_instruction_stickcs">
			<strong>StickCs:</strong> Turn off, connect one side of a jumper cable into GND and the other side in G0, plug in USB, then remove the jumper cable.
		</li>
		<li data-i18n="flashing_instruction_stickcs">
			<strong>T-Embed:</strong> Keep encoder center button pressed and press RST button (CC1101 this btn is on the board, beside ESP32-S3 chip).
		</li>
		<li data-i18n="flashing_instruction_stickcs"><strong>T-Deck:</strong> Keep the trackpad button pressed and press RST (in the left side).</li>
		<li>
			<p>If you are in linux, you may need to run <code>sudo setfacl -m u::rw /dev/ttyACM0</code> to be able to flash.</p>
		</li>
	</ul>
</div>

<div class="container">
	<h2 class="mt-5 flex items-center justify-center p-2 text-2xl font-bold" data-i18n="version_select_title">Select Release</h2>
	<div class="max-xs:flex-col mb-5 flex flex-wrap items-start justify-center gap-4">
		<button
			id="latest"
			class="{active_el(
				selectedVersion,
				'Last'
			)} flex min-h-[2.5rem] w-32 min-w-[10rem] cursor-pointer items-center justify-center rounded-lg border-2 border-purple-500 px-5 py-2.5 text-purple-500 transition-all duration-300 ease-in-out peer-checked:bg-[#9B51E0] peer-checked:text-white hover:bg-[#9B51E0] hover:text-white"
			onclick={() => toggleRelease('Last')}>Latest {latestVersionTag}</button
		>
		<button
			id="beta"
			class="{active_el(
				selectedVersion,
				'Beta'
			)} flex min-h-[2.5rem] w-32 min-w-[10rem] cursor-pointer items-center justify-center rounded-lg border-2 border-purple-500 px-5 py-2.5 text-purple-500 transition-all duration-300 ease-in-out peer-checked:bg-[#9B51E0] peer-checked:text-white hover:bg-[#9B51E0] hover:text-white"
			onclick={() => toggleRelease('Beta')}>Beta</button
		>
		<div class="flex flex-col items-center">
			<button
				id="other"
				class="{active_el(
					selectedVersion,
					'Other'
				)}  min-h-[2.5rem] w-32 min-w-[10rem] cursor-pointer rounded-lg border-2 border-purple-500 px-5 py-2.5 text-purple-500 transition-all duration-300 ease-in-out peer-checked:bg-[#9B51E0] peer-checked:text-white hover:bg-[#9B51E0] hover:text-white"
				onclick={() => toggleRelease('Other')}>Other</button
			>
			<div id="otherReleaseContainer" style="display: none;">
				{#if loading}
					<p>Loading tags...</p>
				{:else if error}
					<p>Error: {error}</p>
				{:else}
					<select
						id="otherReleaseDropdown"
						class="mt-2 min-h-[2.5rem] w-32 min-w-[10rem] rounded-lg border-2 border-purple-500 bg-black p-2 px-5 py-2.5 text-purple-500 transition-all duration-300 ease-in-out"
						onchange={() => updateReleaseDate()}
					>
						{#each versionTags.filter( (release: { tag_name: string }) => /^v?\d+\.\d+(\.\d+)?$/.test(release.tag_name) ) as versionTag, i (versionTag.tag_name)}
							<option value={versionTag.tag_name} data-release-date={versionTag.updated_at}>{versionTag.tag_name}{i === 0 ? ' (Latest)' : ''}</option>
						{/each}
					</select>
				{/if}
			</div>
		</div>
	</div>
	<div id="releaseDate" class="invisible mt-2 text-center text-sm font-bold text-gray-400" data-i18n="release_date">
		Released: <span id="releaseDateValue" class="font-normal">{formatDateTime(selectedReleaseDate)}</span>
	</div>
</div>

<div class="container">
	<h2 class="mt-5 p-2 text-center text-2xl font-bold" data-i18n="select_device_manufacturer_category_title">Select Device Manufacturer/Category</h2>
	<div class="mb-5 flex flex-wrap items-center justify-center gap-4 max-sm:flex-col">
		{#each Object.keys(manifests) as category}
			<button
				class="{active_el(
					selectedCategory,
					category
				)} flex min-h-[2.5rem] w-32 min-w-[10rem] cursor-pointer items-center justify-center rounded-lg border-2 border-purple-500 px-5 py-2.5 text-purple-500 transition-all duration-300 ease-in-out peer-checked:bg-[#9B51E0] peer-checked:text-white hover:bg-[#9B51E0] hover:text-white"
				onclick={() => toggleDeviceCategory(category)}>{category}</button
			>
		{/each}
	</div>

	{#if selectedCategory}
		<h2 class="p-2 text-center text-2xl font-bold" data-i18n="select_device_title">Select Device</h2>
		<ul class="mb-5 flex flex-wrap items-center justify-center gap-4 text-center max-sm:flex-col">
			{#each manifests[selectedCategory] as device}
				<li class="flex-shrink-0">
					<input
						type="radio"
						name="type"
						value={device.id}
						id={device.id}
						class="hidden"
						bind:group={selectedDevice}
						onchange={() => {
							if (selectedCategory === 'launcher') {
								downloadFile(device.id);
							}
						}}
					/>
					<label
						class="{active_el(
							selectedDevice,
							device.id
						)} font-inter flex inline-block min-h-[3rem] w-48 min-w-[12rem] cursor-pointer items-center justify-center rounded-lg border-2 border-[#9B51E0] px-[15px] py-[10px] text-center text-base text-purple-500 transition-all duration-300 ease-in-out hover:bg-[#9B51E0] hover:text-white"
						for={device.id}>{device.name}</label
					>
				</li>
			{/each}
		</ul>
	{/if}
</div>

<div class="container">
	{#if selectedDevice}
		<h2 class="mt-5 flex items-center justify-center p-2 text-2xl font-bold" data-i18n="select_how_to_install_firmware_title">
			Choose How to Install Firmware
		</h2>
		<p class="mb-5 text-center">
			<esp-web-install-button style={selectedDevice ? 'display:block' : 'display:none'}>
				<button
					slot="activate"
					class="font-inter inline-block cursor-pointer rounded-lg border-2 border-purple-500 bg-purple-500 px-[15px] py-[10px] text-base font-semibold text-white transition-all duration-300 ease-in-out hover:scale-105 hover:border-purple-600 hover:bg-purple-600"
				>
					CONNECT TO DEVICE
				</button>
			</esp-web-install-button>
		</p>

		<p class="mt-3 mb-5 text-center">
			<button
				class="font-inter inline-block cursor-pointer rounded-lg border-2 border-[#9B51E0] px-[15px] py-[10px] text-center text-base text-[#9B51E0] transition-all duration-300 ease-in-out hover:scale-105 hover:bg-[#9B51E0] hover:font-semibold hover:text-white"
				onclick={() => downloadFile(selectedDevice)}
			>
				DOWNLOAD FIRMWARE .BIN
			</button>
		</p>
	{/if}
</div>

<div class="container">&nbsp;</div>

<style>
	.container {
		width: 90%;
		max-width: 100%;
		margin: 0 auto;
	}
</style>
