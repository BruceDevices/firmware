<script lang="ts">
	import { current_page, Page } from '$lib/store';
	import manifests from '$lib/data/manifests.json';
	import SectionBackground from '$lib/components/SectionBackground.svelte';
	import Icon from '$lib/components/Icon.svelte';

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

	function getBuildPath(releaseTag, selectedDevice) {
		// using alternative Launcher CORS proxy until Bruce's proxy doesn't get fixed
		// Change "activated" to False to get back to original code,
		// keep the alternative proxy in the code, DO NOT remove!!
		const activated = true;

		if (activated && releaseTag === 'betaRelease') {
			return (
				'https://launcher-cors-proxy-99894582617.europe-west1.run.app/?url=https://github.com/BruceDevices/firmware/releases/download/' +
				releaseTag +
				'/Bruce-' +
				encodeURIComponent(selectedDevice) +
				'.bin'
			);
		}

		return 'https://bruce.iceis.co.uk/service/github/' + releaseTag + '/Bruce-' + encodeURIComponent(selectedDevice) + '.bin';
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
									path: getBuildPath(releaseTag, selectedDevice),
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

	const is_active = (first: string, cmp: string) => String(first == cmp);

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

<!-- Hero -->
<section class="relative overflow-hidden border-b border-[var(--rule)]">
	<SectionBackground />
	<div class="absolute inset-0 bg-gradient-to-r from-[var(--color-ink)] via-[var(--color-ink)]/85 to-transparent"></div>
	<div class="shell relative z-10 py-20 md:py-24">
		<span class="eyebrow">Web installer</span>
		<h1 data-i18n="hero_title" class="mt-4 text-4xl font-semibold md:text-6xl">Bruce Web Flasher</h1>
		<p data-i18n="hero_description" class="lede mt-5 max-w-xl">Flash your device easily with our online installer!</p>
	</div>
</section>

<div class="shell py-16">
	<!-- Instructions -->
	<section class="panel p-8">
		<h2 class="text-xl font-semibold" data-i18n="flashing_instructions_title">Flashing Instructions</h2>
		<p class="mt-4 text-sm text-white" data-i18n="flashing_instruction_1">
			<strong>Connect your device, then select "Flash" and click connect.</strong>
		</p>
		<p class="mt-2 text-sm text-[var(--text-dim)]" data-i18n="flashing_instruction_2">
			If asked to put your device into <strong class="text-white">download mode</strong>, do the following:
		</p>

		<dl class="mt-6 grid gap-px overflow-hidden border border-[var(--rule)] bg-[var(--rule)] md:grid-cols-2">
			<div class="bg-[var(--color-ink)] p-4" data-i18n="flashing_instruction_cardputer">
				<dt class="meta">Cardputer</dt>
				<dd class="mt-1.5 text-sm leading-relaxed text-[var(--text-dim)]">
					Turn off and unplug from USB, hold the btn G0 (upper right corner), then connect via USB.
				</dd>
			</div>
			<div class="bg-[var(--color-ink)] p-4" data-i18n="flashing_instruction_stickcs">
				<dt class="meta">StickCs</dt>
				<dd class="mt-1.5 text-sm leading-relaxed text-[var(--text-dim)]">
					Turn off, connect one side of a jumper cable into GND and the other side in G0, plug in USB, then remove the jumper cable.
				</dd>
			</div>
			<div class="bg-[var(--color-ink)] p-4">
				<dt class="meta">T-Embed</dt>
				<dd class="mt-1.5 text-sm leading-relaxed text-[var(--text-dim)]">
					Keep encoder center button pressed and press RST button (CC1101 this btn is on the board, beside ESP32-S3 chip).
				</dd>
			</div>
			<div class="bg-[var(--color-ink)] p-4">
				<dt class="meta">T-Deck</dt>
				<dd class="mt-1.5 text-sm leading-relaxed text-[var(--text-dim)]">Keep the trackpad button pressed and press RST (in the left side).</dd>
			</div>
		</dl>

		<p class="mt-4 text-sm text-[var(--text-dim)]">
			If you are in linux, you may need to run
			<code class="rounded-[2px] border border-[var(--rule)] bg-black/50 px-1.5 py-0.5 font-mono text-xs text-white"
				>sudo setfacl -m u::rw /dev/ttyACM0</code
			> to be able to flash.
		</p>
	</section>

	<!-- Step 1: release -->
	<section class="panel mt-6 p-8">
		<div class="flex items-baseline gap-3">
			<span class="meta tabular-nums">01</span>
			<h2 class="text-xl font-semibold" data-i18n="version_select_title">Select Release</h2>
		</div>

		<div class="mt-5 flex flex-wrap items-start gap-2">
			<button id="latest" class="chip min-w-[9rem]" data-active={is_active(selectedVersion, 'Last')} onclick={() => toggleRelease('Last')}>
				Latest {latestVersionTag}
			</button>
			<button id="beta" class="chip min-w-[9rem]" data-active={is_active(selectedVersion, 'Beta')} onclick={() => toggleRelease('Beta')}>
				Beta
			</button>
			<div class="flex flex-col gap-2">
				<button id="other" class="chip min-w-[9rem]" data-active={is_active(selectedVersion, 'Other')} onclick={() => toggleRelease('Other')}>
					Other
				</button>
				<div id="otherReleaseContainer" style="display: none;">
					{#if loading}
						<p class="meta">Loading tags...</p>
					{:else if error}
						<p class="meta text-red-400">Error: {error}</p>
					{:else}
						<select id="otherReleaseDropdown" class="field min-w-[9rem] bg-[var(--color-ink)]" onchange={() => updateReleaseDate()}>
							{#each versionTags.filter( (release: { tag_name: string }) => /^v?\d+\.\d+(\.\d+)?$/.test(release.tag_name) ) as versionTag, i (versionTag.tag_name)}
								<option value={versionTag.tag_name} data-release-date={versionTag.updated_at}
									>{versionTag.tag_name}{i === 0 ? ' (Latest)' : ''}</option
								>
							{/each}
						</select>
					{/if}
				</div>
			</div>
		</div>

		<div id="releaseDate" class="invisible mt-4" data-i18n="release_date">
			<span class="meta">Released: {formatDateTime(selectedReleaseDate)}</span>
		</div>
	</section>

	<!-- Step 2: device -->
	<section class="panel mt-6 p-8">
		<div class="flex items-baseline gap-3">
			<span class="meta tabular-nums">02</span>
			<h2 class="text-xl font-semibold" data-i18n="select_device_manufacturer_category_title">Select Device Manufacturer/Category</h2>
		</div>

		<div class="mt-5 flex flex-wrap gap-2">
			{#each Object.keys(manifests) as category}
				<button class="chip min-w-[9rem]" data-active={is_active(selectedCategory, category)} onclick={() => toggleDeviceCategory(category)}>
					{category}
				</button>
			{/each}
		</div>

		{#if selectedCategory}
			<hr class="rule my-7" />
			<h3 class="eyebrow" data-i18n="select_device_title">Select Device</h3>
			<ul class="mt-4 flex flex-wrap gap-2">
				{#each manifests[selectedCategory] as device}
					<li>
						<input
							type="radio"
							name="type"
							value={device.id}
							id={device.id}
							class="peer sr-only"
							bind:group={selectedDevice}
							onchange={() => {
								if (selectedCategory === 'launcher') {
									downloadFile(device.id);
								}
							}}
						/>
						<label class="chip min-w-[11rem]" data-active={is_active(selectedDevice, device.id)} for={device.id}>{device.name}</label>
					</li>
				{/each}
			</ul>
		{/if}
	</section>

	<!-- Step 3: install -->
	{#if selectedDevice}
		<section class="panel mt-6 p-8">
			<div class="flex items-baseline gap-3">
				<span class="meta tabular-nums">03</span>
				<h2 class="text-xl font-semibold" data-i18n="select_how_to_install_firmware_title">Choose How to Install Firmware</h2>
			</div>

			<div class="mt-6 flex flex-wrap items-center gap-3">
				<esp-web-install-button style={selectedDevice ? 'display:block' : 'display:none'}>
					<button slot="activate" class="btn btn-primary">
						<Icon name="plug" size={15} /> Connect to device
					</button>
				</esp-web-install-button>

				<button class="btn btn-outline" onclick={() => downloadFile(selectedDevice)}>
					<Icon name="download" size={15} /> Download firmware .bin
				</button>
			</div>
		</section>
	{/if}
</div>
