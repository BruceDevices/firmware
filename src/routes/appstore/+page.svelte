<script lang="ts">
	import { base } from '$app/paths';
	import { capitalize } from '$lib/helper';
	import {
		current_page,
		Page,
		loadAllData,
		isLoadingData,
		selectedCategory,
		filterAppsForCategory,
		clearCategorySelection,
		supportedDevices,
		selectedDevice,
		filteredApps,
		applyDeviceFilter,
		searchQuery,
		searchedApps,
		searchFilteredCategories,
		applySearchFilter,
		initializeSearch
	} from '$lib/store';
	import AttentionBanner from '$lib/components/AttentionBanner.svelte';
	import Icon from '$lib/components/Icon.svelte';
	import InstallationBanner from '$lib/components/InstallationBanner.svelte';
	import { onMount } from 'svelte';
	import JSZip from 'jszip';
	import installService, { type InstallProgress } from '$lib/install-service';

	const components = import.meta.glob('$lib/apps/*.md', { eager: true });

	$current_page = Page.AppStore;

	let applications = $state(Object.entries(components));
	// Modal state for app details
	// Define the App type based on your app object structure
	type App = {
		name: string;
		slug: string;
		category: string;
		version: string;
		description: string;
		owner: string;
		repo: string;
		commit: string;
		path?: string;
		files?: Array<string | { source: string; destination?: string }>;
		'supported-screen-size'?: string;
		'supported-devices'?: string | string[];
		lastUpdated: number;
		// Add any other properties as needed
	};

	let selectedApp = $state<App | null>(null);
	let showModal = $state(false);
	// Initial loading state to prevent blank appearance
	let initialLoad = $state(true);
	// Download state
	let isDownloading = $state(false);
	let downloadProgress = $state('');
	let downloadError = $state('');
	// Install popup state
	let showInstallPopup = $state(false);
	// Download completion popup state
	let showDownloadComplete = $state(false);
	// Install state
	let isInstalling = $state(false);
	let installProgress = $state<InstallProgress | null>(null);
	let showCancelConfirm = $state(false);
	// Searchable dropdown state
	let showDeviceDropdown = $state(false);
	let deviceSearchQuery = $state('');
	let filteredDevicesForSearch = $state<any[]>([]);
	// Browser compatibility state
	let isWebSerialSupported = $state(false);
	let browserUnsupportedReason = $state<string | null>(null);
	// Install enabled state
	let isInstallEnabled = $state(false);
	// Local search query for input binding
	let localSearchQuery = $state('');

	// Load all data when component mounts
	onMount(async () => {
		// Check URL parameters for install enablement
		const urlParams = new URLSearchParams(window.location.search);
		if (urlParams.get('debug') === 'true') {
			installService.setDebugEnabled(true);
		}
		if (urlParams.get('installEnabled') === 'true') {
			installService.setInstallEnabled(true);
		}

		// Check Web Serial API support
		isWebSerialSupported = installService.isWebSerialSupported();
		browserUnsupportedReason = installService.getUnsupportedReason();
		// Check if install is enabled
		isInstallEnabled = installService.isInstallEnabled();

		// Small delay to ensure initial placeholders are visible
		setTimeout(() => {
			initialLoad = false;
		}, 100);

		await loadAllData();

		// Auto-select "All" category and load apps
		filterAppsForCategory('all', 'All');
		applications = []; // Clear markdown apps to show category apps

		// Initialize search with all apps
		initializeSearch();

		// Load saved device from localStorage
		const savedDevice = localStorage.getItem('selectedDevice');
		if (savedDevice) {
			// Verify the device still exists in the loaded devices
			if ($supportedDevices.some((device) => device.name === savedDevice)) {
				applyDeviceFilter(savedDevice);
			}
		}
		// Add click outside listener
		document.addEventListener('click', handleClickOutside);

		// Cleanup listener on component destroy
		return () => {
			document.removeEventListener('click', handleClickOutside);
		};
	});

	// Sync local search query with store
	$effect(() => {
		localSearchQuery = $searchQuery;
	});

	function filter(categoryName: string, categorySlug: string) {
		if ($selectedCategory === categoryName) {
			// Reset the state - show original markdown apps
			clearCategorySelection();
			applications = Object.entries(components);
		} else {
			// Filter apps for this category
			filterAppsForCategory(categorySlug, categoryName);
			applications = []; // Clear markdown apps when showing category apps
		}
	}

	// Function to handle device filter change with localStorage
	function handleDeviceFilter(deviceName: string) {
		applyDeviceFilter(deviceName);
		localStorage.setItem('selectedDevice', deviceName);
		showDeviceDropdown = false;
		deviceSearchQuery = '';
	}

	// Filter devices based on search query
	$effect(() => {
		if (deviceSearchQuery === '') {
			filteredDevicesForSearch = $supportedDevices;
		} else {
			filteredDevicesForSearch = $supportedDevices.filter((device) => device.name.toLowerCase().includes(deviceSearchQuery.toLowerCase()));
		}
	});

	// Clear search when category changes
	$effect(() => {
		// Reset search when category or device filter changes
		if ($selectedCategory || $selectedDevice) {
			// Don't automatically clear search - let user keep their search active
		}
	});

	// Clear search when category changes
	$effect(() => {
		// Reset search when category or device filter changes
		if ($selectedCategory || $selectedDevice) {
			// Don't automatically clear search - let user keep their search active
		}
	});

	// Toggle dropdown visibility
	function toggleDeviceDropdown() {
		showDeviceDropdown = !showDeviceDropdown;
		if (showDeviceDropdown) {
			deviceSearchQuery = '';
			// Focus the search input after a brief delay
			setTimeout(() => {
				const searchInput = document.getElementById('device-search-input');
				if (searchInput) {
					searchInput.focus();
				}
			}, 50);
		}
	}

	// Close dropdown when clicking outside
	function handleClickOutside(event) {
		const dropdown = document.getElementById('device-dropdown');
		if (dropdown && !dropdown.contains(event.target)) {
			showDeviceDropdown = false;
			deviceSearchQuery = '';
		}
	}

	// Function to handle image error and show placeholder
	function handleImageError(e) {
		// Set a simple SVG placeholder
		e.target.src =
			'data:image/svg+xml;base64,' +
			btoa(`
			<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128">
				<rect width="128" height="128" fill="#4B5563"/>
				<rect x="16" y="16" width="96" height="96" fill="#6B7280" stroke="#9CA3AF" stroke-width="2"/>
				<circle cx="40" cy="40" r="8" fill="#9CA3AF"/>
				<polygon points="16,96 48,64 64,80 96,48 112,64 112,112 16,112" fill="#9CA3AF"/>
				<text x="64" y="120" font-family="Arial, sans-serif" font-size="10" text-anchor="middle" fill="#9CA3AF">No Image</text>
			</svg>
		`);
		e.target.style.display = 'block';
	}

	// Helper function to get logo URL for an app
	function getLogoUrl(appSlug: string): string {
		return `https://brucedevices.github.io/App-Store-Data/repositories/${appSlug}/logo.png`;
	}

	// Modal functions
	function openAppModal(app) {
		selectedApp = app;
		showModal = true;

		// Reset all status panels and notifications
		isDownloading = false;
		downloadProgress = '';
		downloadError = '';
		showInstallPopup = false;
		showDownloadComplete = false;
		isInstalling = false;
		installProgress = null;
	}

	function closeModal() {
		if (isInstalling) {
			showCancelConfirm = true;
		} else {
			showModal = false;
			selectedApp = null;
		}
	}

	// Format supported devices for display
	function formatSupportedDevices(supportedDevices) {
		if (!supportedDevices) return 'All devices';
		if (typeof supportedDevices === 'string') return supportedDevices;
		if (Array.isArray(supportedDevices)) return supportedDevices.join(', ');
		return 'All devices';
	}

	// Download function to create zip with all app files
	async function downloadAppFiles(app) {
		if (!app.files || app.files.length === 0) {
			downloadError = 'No files available for download';
			return;
		}

		isDownloading = true;
		downloadError = '';
		downloadProgress = 'Initializing download...';

		// Reset install-related states when starting download
		showInstallPopup = false;
		installProgress = null;
		isInstalling = false;

		// Reset install-related states when starting download
		showInstallPopup = false;
		installProgress = null;
		isInstalling = false;

		try {
			// If only one file, download directly without zip
			if (app.files.length === 1) {
				const file = app.files[0];
				let fileName, filePath;

				if (typeof file === 'string') {
					fileName = file.split('/').pop() || file;
					filePath = file;
				} else {
					fileName = file.destination || file.source.split('/').pop() || file.source;
					filePath = file.source;
				}

				// Construct URL without any encoding - clean up extra slashes
				const baseUrl = `https://raw.githubusercontent.com/${app.owner}/${app.repo}/${app.commit}`;
				const cleanPath = app.path ? app.path.replace(/^\/+|\/+$/g, '') : '';
				const cleanFilePath = filePath.replace(/^\/+/, '');

				const fileUrl = cleanPath ? `${baseUrl}/${cleanPath}/${cleanFilePath}` : `${baseUrl}/${cleanFilePath}`;

				downloadProgress = `Downloading ${fileName}...`;

				const response = await fetch(fileUrl);
				if (!response.ok) {
					throw new Error(`Failed to download ${fileName}: ${response.status} ${response.statusText}`);
				}

				const fileBlob = await response.blob();

				// Create download link for single file
				const url = URL.createObjectURL(fileBlob);
				const a = document.createElement('a');
				a.href = url;
				a.download = fileName;
				document.body.appendChild(a);
				a.click();
				document.body.removeChild(a);
				URL.revokeObjectURL(url);

				downloadProgress = '';
				showDownloadComplete = true;
				return;
			}

			// Multiple files - create zip
			const zip = new JSZip();
			const totalFiles = app.files.length;
			let completedFiles = 0;

			for (const file of app.files) {
				let fileName, filePath;

				if (typeof file === 'string') {
					// Simple file path - use raw values without any encoding
					fileName = file.split('/').pop() || file;
					filePath = file;
				} else {
					// File with source and destination - use raw values without any encoding
					fileName = file.destination || file.source.split('/').pop() || file.source;
					filePath = file.source;
				}

				// Construct URL without any encoding - clean up extra slashes
				const baseUrl = `https://raw.githubusercontent.com/${app.owner}/${app.repo}/${app.commit}`;
				const cleanPath = app.path ? app.path.replace(/^\/+|\/+$/g, '') : ''; // Remove leading/trailing slashes
				const cleanFilePath = filePath.replace(/^\/+/, ''); // Remove leading slashes

				const fileUrl = cleanPath ? `${baseUrl}/${cleanPath}/${cleanFilePath}` : `${baseUrl}/${cleanFilePath}`;

				downloadProgress = `Downloading ${fileName} (${completedFiles + 1}/${totalFiles})...`;

				try {
					const response = await fetch(fileUrl);
					if (!response.ok) {
						throw new Error(`Failed to download ${fileName}: ${response.status} ${response.statusText}`);
					}

					const fileBlob = await response.blob();
					zip.file(fileName, fileBlob);
					completedFiles++;
				} catch (error) {
					console.warn(`Failed to download ${fileName}:`, error);
					// Continue with other files even if one fails
					completedFiles++;
				}
			}

			if (completedFiles === 0) {
				throw new Error('No files could be downloaded');
			}

			downloadProgress = 'Creating zip file...';

			// Generate zip file
			const zipBlob = await zip.generateAsync({ type: 'blob' });

			// Create download link
			const url = URL.createObjectURL(zipBlob);
			const a = document.createElement('a');
			a.href = url;
			a.download = `${app.name.replace(/[^a-zA-Z0-9]/g, '_')}_v${app.version}.zip`;
			document.body.appendChild(a);
			a.click();
			document.body.removeChild(a);
			URL.revokeObjectURL(url);

			downloadProgress = '';
			showDownloadComplete = true;
		} catch (error) {
			console.error('Download failed:', error);
			downloadError = error && typeof error === 'object' && 'message' in error ? (error as { message: string }).message : 'Failed to download files';
		} finally {
			isDownloading = false;
		}
	}

	// Install function
	async function installApp(app) {
		// Check if install is enabled first
		if (!isInstallEnabled) {
			showInstallPopup = true;
			return;
		}

		// Check browser compatibility first
		if (!isWebSerialSupported) {
			showInstallPopup = true;
			return;
		}

		if (!app.files || app.files.length === 0) {
			showInstallPopup = true;
			return;
		}

		isInstalling = true;
		installProgress = null;

		// Reset download-related states when starting install
		isDownloading = false;
		downloadProgress = '';
		downloadError = '';
		showDownloadComplete = false;

		// Prepare files for installation
		const appFiles = [];

		for (const file of app.files) {
			let sourceUrl, sourceFile, fileName, destinationPath;

			if (typeof file === 'string') {
				// Construct full URL for the file
				const baseUrl = `https://raw.githubusercontent.com/${app.owner}/${app.repo}/${app.commit}`;
				const cleanPath = app.path ? app.path.replace(/^\/+|\/+$/g, '') : '';
				const cleanFilePath = file.replace(/^\/+/, '');

				sourceFile = cleanPath ? `${cleanPath}/${cleanFilePath}` : `${cleanFilePath}`;
				sourceUrl = cleanPath ? `${baseUrl}/${cleanPath}/${cleanFilePath}` : `${baseUrl}/${cleanFilePath}`;
				fileName = file.split('/').pop() || file;
			} else {
				// File with source and destination
				const baseUrl = `https://raw.githubusercontent.com/${app.owner}/${app.repo}/${app.commit}`;
				const cleanPath = app.path ? app.path.replace(/^\/+|\/+$/g, '') : '';
				const cleanFilePath = file.source.replace(/^\/+/, '');

				sourceFile = cleanPath ? `${cleanPath}/${cleanFilePath}` : `${cleanFilePath}`;
				sourceUrl = cleanPath ? `${baseUrl}/${cleanPath}/${cleanFilePath}` : `${baseUrl}/${cleanFilePath}`;

				// Use destination filename if specified, otherwise use source filename
				fileName = file.destination || file.source.split('/').pop() || file.source;
			}

			// Determine destination path based on app category
			if (app.category === 'Themes') {
				destinationPath = `/Themes/${app.name}/${fileName}`;
			} else {
				// BruceJS apps
				destinationPath = `/BruceJS/${app.category}/${fileName}`;
			}

			appFiles.push({
				source: sourceUrl,
				sourceFile: sourceFile,
				destination: destinationPath
			});
		}

		await installService.installApp(app.name, appFiles, (progress) => {
			installProgress = progress;
		});

		isInstalling = false;
	}
</script>

<div class="shell py-12">
	<header class="mb-8 max-w-2xl">
		<span class="eyebrow">Apps and themes</span>
		<h1 class="mt-3 text-3xl font-semibold md:text-4xl">Bruce App Store</h1>
	</header>

	<div class="space-y-3">
		<AttentionBanner />
		<InstallationBanner />
	</div>

	<!-- Filters -->
	<div class="mt-8 flex flex-col gap-3 lg:flex-row">
		<div class="relative flex-1">
			<span class="pointer-events-none absolute top-1/2 left-3 -translate-y-1/2 text-[var(--text-faint)]">
				<Icon name="search" size={15} />
			</span>
			<input
				type="text"
				placeholder="Search apps and themes"
				aria-label="Search apps and themes"
				bind:value={localSearchQuery}
				oninput={(e) => {
					if (e.target) applySearchFilter((e.target as HTMLInputElement).value);
				}}
				class="field pl-9"
			/>
			{#if localSearchQuery}
				<button
					onclick={() => {
						localSearchQuery = '';
						applySearchFilter('');
					}}
					aria-label="Clear search"
					class="absolute top-1/2 right-2 inline-flex h-7 w-7 -translate-y-1/2 items-center justify-center rounded-[3px] text-[var(--text-faint)] transition-colors hover:bg-white/5 hover:text-white"
				>
					<Icon name="close" size={14} />
				</button>
			{/if}
		</div>

		{#if initialLoad || $isLoadingData}
			<div class="placeholder-shimmer h-[2.4rem] w-full rounded-[3px] lg:w-72"></div>
		{:else if $supportedDevices.length > 0}
			<div class="relative w-full lg:w-72" id="device-dropdown">
				<button class="field flex items-center justify-between text-left" onclick={toggleDeviceDropdown} aria-expanded={showDeviceDropdown}>
					<span class="truncate">{$selectedDevice}</span>
					<span class="ml-2 shrink-0 text-[var(--text-faint)] transition-transform duration-200" class:rotate-180={showDeviceDropdown}>
						<Icon name="chevron-down" size={15} />
					</span>
				</button>

				{#if showDeviceDropdown}
					<div
						class="absolute z-50 mt-1 w-full overflow-hidden rounded-[3px] border border-[var(--rule-strong)] bg-[var(--color-surface)] shadow-2xl"
					>
						<div class="border-b border-[var(--rule)] p-2">
							<input id="device-search-input" type="text" placeholder="Search devices..." bind:value={deviceSearchQuery} class="field text-xs" />
						</div>
						<div class="max-h-56 overflow-y-auto py-1">
							{#each filteredDevicesForSearch as device}
								<button
									class="w-full px-3 py-2 text-left text-sm transition-colors hover:bg-white/5 {$selectedDevice === device.name
										? 'text-[var(--color-brand)]'
										: 'text-[var(--text-dim)]'}"
									onclick={() => handleDeviceFilter(device.name)}
								>
									{device.name}
								</button>
							{:else}
								<div class="px-3 py-2 text-sm text-[var(--text-faint)]">No devices found</div>
							{/each}
						</div>
					</div>
				{/if}
			</div>
		{/if}
	</div>

	<!-- Categories -->
	<div class="mt-4 flex flex-wrap gap-2">
		{#if initialLoad || $isLoadingData}
			{#each Array(8) as _}
				<div class="placeholder-shimmer h-[2.1rem] w-24 rounded-[3px]"></div>
			{/each}
		{:else}
			{#each $searchFilteredCategories as category}
				<button class="chip" data-active={String($selectedCategory === category.name)} onclick={() => filter(category.name, category.slug)}>
					{capitalize(category.name)}
					<span class="ml-1.5 font-mono text-[0.6875rem] text-[var(--text-faint)]">{category.count}</span>
				</button>
			{/each}
		{/if}
	</div>

	{#if $searchQuery}
		<p class="meta mt-4">Found {$searchedApps.length} result{$searchedApps.length === 1 ? '' : 's'} for "{$searchQuery}"</p>
	{/if}

	<!-- Loading -->
	{#if initialLoad || $isLoadingData}
		<div class="mt-10">
			<p class="text-sm text-[var(--text-dim)]">{initialLoad ? 'Initializing app store...' : 'Loading apps and categories...'}</p>
			<p class="meta mt-1">{initialLoad ? 'Setting up the interface' : 'Please wait while we fetch the latest data'}</p>

			<div class="mt-6 grid gap-px overflow-hidden border border-[var(--rule)] bg-[var(--rule)] sm:grid-cols-2 lg:grid-cols-3">
				{#each Array(6) as _}
					<div class="bg-[var(--color-ink)] p-5">
						<div class="placeholder-shimmer mb-4 h-28 w-full rounded-[3px]"></div>
						<div class="placeholder-shimmer h-4 w-3/4 rounded"></div>
						<div class="placeholder-shimmer mt-2 h-3 w-1/2 rounded"></div>
						<div class="placeholder-shimmer mt-3 h-3 w-full rounded"></div>
					</div>
				{/each}
			</div>
		</div>
	{/if}

	<!-- Results -->
	{#if $searchedApps.length > 0}
		<section class="mt-10">
			<h2 class="eyebrow mb-4">
				{$searchQuery ? 'Search Results' : $selectedCategory === 'All' ? 'All Apps/Themes' : $selectedCategory}
				{$selectedDevice !== 'All Devices' ? ` — ${$selectedDevice}` : ''}
			</h2>

			<div class="grid gap-px overflow-hidden border border-[var(--rule)] bg-[var(--rule)] sm:grid-cols-2 lg:grid-cols-3">
				{#each $searchedApps as app}
					<button class="group bg-[var(--color-ink)] p-5 text-left transition-colors hover:bg-white/[0.03]" onclick={() => openAppModal(app)}>
						<div class="mb-4 flex h-28 items-center justify-center border border-[var(--rule)] bg-black/40">
							<img src={getLogoUrl(app.slug)} alt="" class="max-h-20 max-w-20 object-contain" onerror={handleImageError} />
						</div>

						<div class="flex items-baseline justify-between gap-3">
							<h3 class="truncate font-semibold text-white">{app.name}</h3>
							<span class="meta shrink-0">{app.category}</span>
						</div>

						{#if app.category === 'Themes' && app['supported-screen-size']}
							<p class="meta mt-1">{app['supported-screen-size']}</p>
						{/if}

						<p class="meta mt-1">v{app.version} · {app.owner}</p>
						<p class="mt-3 line-clamp-3 text-sm leading-relaxed text-[var(--text-dim)]">{app.description}</p>
					</button>
				{/each}
			</div>
		</section>
	{:else if $searchQuery && $filteredApps.length > 0}
		<div class="panel mt-10 p-10 text-center">
			<span class="inline-flex text-[var(--text-faint)]"><Icon name="search" size={22} /></span>
			<h3 class="mt-4 text-lg font-semibold">No Results Found</h3>
			<p class="mx-auto mt-2 max-w-md text-sm text-[var(--text-dim)]">
				No apps found matching "{$searchQuery}". Try a different search term or clear the search to see all apps.
			</p>
			<button
				class="btn btn-primary mt-6"
				onclick={() => {
					localSearchQuery = '';
					applySearchFilter('');
				}}
			>
				Clear Search
			</button>
		</div>
	{/if}

	<!-- Original markdown apps (shown when no category selected) -->
	{#if applications.length > 0}
		<div class="mt-10 grid gap-px overflow-hidden border border-[var(--rule)] bg-[var(--rule)] sm:grid-cols-2 lg:grid-cols-3">
			{#each applications as element}
				{@const CardApp = element[1].default}
				<a href="{base}/appstore/{element[1].metadata.id}" class="bg-[var(--color-ink)]">
					<CardApp card href="" />
				</a>
			{/each}
		</div>
	{/if}
</div>

<!-- App Detail Modal -->
{#if showModal && selectedApp}
	<div class="fixed inset-0 z-[200] flex items-start justify-center overflow-y-auto bg-black/80 p-4 pt-24 backdrop-blur-sm">
		<div class="flex max-h-[80vh] w-full max-w-2xl flex-col border border-[var(--rule-strong)] bg-[var(--color-surface)]">
			<!-- Header -->
			<div class="shrink-0 border-b border-[var(--rule)] p-6">
				<div class="flex items-start justify-between gap-4">
					<div class="min-w-0">
						<span class="eyebrow">{selectedApp.category}</span>
						<h2 class="mt-2 text-xl font-semibold break-words">
							{selectedApp.name}
							{#if selectedApp.category === 'Themes' && selectedApp['supported-screen-size']}
								<span class="ml-1 font-mono text-sm font-normal text-[var(--text-faint)]">{selectedApp['supported-screen-size']}</span>
							{/if}
						</h2>
					</div>
					<button
						class="-mt-1 inline-flex h-8 w-8 shrink-0 items-center justify-center rounded-[3px] text-[var(--text-faint)] transition-colors hover:bg-white/5 hover:text-white"
						onclick={closeModal}
						aria-label="Close"
					>
						<Icon name="close" size={16} />
					</button>
				</div>

				{#if selectedApp.files && selectedApp.files.length > 0}
					<div class="mt-5 flex flex-wrap gap-3">
						<button class="btn btn-primary" disabled={isInstalling || isDownloading} onclick={() => installApp(selectedApp)}>
							{#if isInstalling}
								<span class="spinner"></span> Installing...
							{:else}
								<Icon name="upload" size={15} /> Install
							{/if}
						</button>
						<button class="btn btn-outline" disabled={isDownloading} onclick={() => downloadAppFiles(selectedApp)}>
							{#if isDownloading}
								<span class="spinner"></span> Downloading...
							{:else}
								<Icon name="download" size={15} /> Download
							{/if}
						</button>
					</div>
				{/if}

				<!-- Cancel Install Confirmation -->
				{#if showCancelConfirm}
					<div class="mt-5 border border-l-2 border-[var(--rule)] border-l-red-500 bg-red-500/[0.06] p-4">
						<p class="text-sm text-white">An installation is in progress. Are you sure you want to cancel?</p>
						<div class="mt-4 flex gap-3">
							<button
								class="btn btn-primary"
								onclick={() => {
									installService.cancelInstall();
									showCancelConfirm = false;
									showModal = false;
									selectedApp = null;
								}}
							>
								Yes, Cancel
							</button>
							<button class="btn btn-quiet" onclick={() => (showCancelConfirm = false)}>No, Continue</button>
						</div>
					</div>
				{/if}

				<!-- Install notice -->
				{#if showInstallPopup}
					<div class="relative mt-5 border border-l-2 border-[var(--rule)] border-l-yellow-500/80 bg-yellow-500/[0.05] p-4 pr-10">
						<button
							class="absolute top-3 right-3 inline-flex h-7 w-7 items-center justify-center rounded-[3px] text-[var(--text-faint)] transition-colors hover:bg-white/5 hover:text-white"
							onclick={() => (showInstallPopup = false)}
							aria-label="Dismiss"
						>
							<Icon name="close" size={14} />
						</button>
						<div class="flex items-center gap-2 text-yellow-500">
							<Icon name="warning" size={15} />
							{#if !isInstallEnabled}
								<p class="text-sm font-medium">Install feature not implemented</p>
							{:else if !isWebSerialSupported}
								<p class="text-sm font-medium">Browser not supported for direct installation</p>
							{:else}
								<p class="text-sm font-medium">No files available for installation</p>
							{/if}
						</div>
						<p class="mt-2 text-xs leading-relaxed text-[var(--text-dim)]">
							{#if !isInstallEnabled}
								Direct installation is temporarily disabled. Please use the download button and upload files manually using the
								<a href="https://wiki.bruce.computer/controlling-device/webui/" target="_blank">WebUI</a>.
							{:else if !isWebSerialSupported}
								{browserUnsupportedReason || 'Your browser does not support Web Serial API.'} Use the download button instead and upload files manually
								using the <a href="https://wiki.bruce.computer/controlling-device/webui/" target="_blank">WebUI</a>.
							{:else}
								This app does not have any files configured for installation. Please contact the app developer.
							{/if}
						</p>
					</div>
				{/if}

				<!-- Install Progress -->
				{#if installProgress}
					{@const isError = installProgress.stage === 'error'}
					{@const isDone = installProgress.stage === 'success'}
					<div
						class="relative mt-5 border border-l-2 border-[var(--rule)] p-4 {isError
							? 'border-l-red-500 bg-red-500/[0.06]'
							: isDone
								? 'border-l-emerald-500 bg-emerald-500/[0.06]'
								: 'border-l-[var(--color-brand)] bg-[var(--wash)]'}"
					>
						<div class="flex items-center gap-2 {isError ? 'text-red-400' : isDone ? 'text-emerald-400' : 'text-[var(--color-brand)]'}">
							{#if isError}
								<Icon name="no" size={15} /> <span class="text-sm font-medium">Installation Failed</span>
							{:else if isDone}
								<Icon name="yes" size={15} /> <span class="text-sm font-medium">Installation Complete</span>
							{:else if installProgress.stage === 'connecting'}
								<Icon name="plug" size={15} /> <span class="text-sm font-medium">Connecting to Device</span>
							{:else if installProgress.stage === 'uploading'}
								<Icon name="upload" size={15} /> <span class="text-sm font-medium">Uploading Files</span>
							{:else if installProgress.stage === 'verifying'}
								<Icon name="yes" size={15} /> <span class="text-sm font-medium">Verifying Installation</span>
							{:else if installProgress.stage === 'rebooting'}
								<Icon name="clock" size={15} /> <span class="text-sm font-medium">Rebooting...</span>
							{/if}

							{#if isError || isDone}
								<button
									class="absolute top-3 right-3 inline-flex h-7 w-7 items-center justify-center rounded-[3px] text-[var(--text-faint)] transition-colors hover:bg-white/5 hover:text-white"
									onclick={() => (installProgress = null)}
									aria-label="Dismiss"
								>
									<Icon name="close" size={14} />
								</button>
							{/if}
						</div>

						{#if !isError}
							<div class="mt-3 h-[3px] w-full bg-white/10">
								<div
									class="h-[3px] transition-all duration-300 {isDone ? 'bg-emerald-500' : 'bg-[var(--color-brand)]'}"
									style="width: {installProgress.progress}%"
								></div>
							</div>
						{/if}

						<p class="mt-2 text-xs text-[var(--text-dim)]">
							{installProgress.message}
							{#if installProgress.speedBps > 0}
								<span class="ml-2 font-mono">({(installProgress.speedBps / 1024).toFixed(1)} KB/s)</span>
							{/if}
						</p>

						{#if installProgress.error}
							<p class="mt-1 font-mono text-xs text-red-400">{installProgress.error}</p>
						{/if}
					</div>
				{/if}

				<!-- Download complete -->
				{#if showDownloadComplete}
					<div class="relative mt-5 border border-l-2 border-[var(--rule)] border-l-emerald-500 bg-emerald-500/[0.06] p-4 pr-10">
						<button
							class="absolute top-3 right-3 inline-flex h-7 w-7 items-center justify-center rounded-[3px] text-[var(--text-faint)] transition-colors hover:bg-white/5 hover:text-white"
							onclick={() => (showDownloadComplete = false)}
							aria-label="Dismiss"
						>
							<Icon name="close" size={14} />
						</button>
						<div class="flex items-center gap-2 text-emerald-400">
							<Icon name="yes" size={15} />
							<p class="text-sm font-medium">Download completed</p>
						</div>
						<p class="mt-2 text-xs leading-relaxed text-[var(--text-dim)]">
							Files have been downloaded. Upload the files to your device using the
							<a href="https://wiki.bruce.computer/controlling-device/webui/" target="_blank">WebUI</a> - extract the files from the .zip first.
						</p>
					</div>
				{/if}

				{#if downloadProgress}
					<div class="mt-5 border border-[var(--rule)] bg-[var(--wash)] p-3">
						<p class="text-sm text-[var(--text-dim)]">{downloadProgress}</p>
					</div>
				{/if}
				{#if downloadError}
					<div class="mt-5 border border-l-2 border-[var(--rule)] border-l-red-500 bg-red-500/[0.06] p-3">
						<p class="text-sm text-red-300">{downloadError}</p>
						<button class="meta mt-1 underline" onclick={() => (downloadError = '')}>Dismiss</button>
					</div>
				{/if}
			</div>

			<!-- Body -->
			<div class="modal-content flex-1 overflow-y-auto p-6">
				<div class="mb-6 flex h-32 items-center justify-center border border-[var(--rule)] bg-black/40">
					<img src={getLogoUrl(selectedApp.slug)} alt="" class="max-h-24 max-w-24 object-contain" onerror={handleImageError} />
				</div>

				<h3 class="eyebrow">Description</h3>
				<p class="mt-2 text-sm leading-relaxed text-[var(--text-dim)]">{selectedApp.description || 'No description available.'}</p>

				<div class="mt-6 grid gap-6 md:grid-cols-2">
					<div>
						<h3 class="eyebrow">Details</h3>
						<dl class="mt-2">
							<div class="flex items-baseline justify-between gap-4 border-b border-[var(--rule)] py-2">
								<dt class="meta">Version</dt>
								<dd class="font-mono text-sm">{selectedApp.version}</dd>
							</div>
							<div class="flex items-baseline justify-between gap-4 border-b border-[var(--rule)] py-2">
								<dt class="meta">Owner</dt>
								<dd class="truncate text-sm">
									<a href="https://github.com/{selectedApp.owner}" target="_blank">{selectedApp.owner}</a>
								</dd>
							</div>
							<div class="flex items-baseline justify-between gap-4 py-2">
								<dt class="meta">Repository</dt>
								<dd class="truncate text-sm">
									<a href="https://github.com/{selectedApp.owner}/{selectedApp.repo}" target="_blank">{selectedApp.repo}</a>
								</dd>
							</div>
						</dl>
					</div>

					<div>
						<h3 class="eyebrow">Compatibility</h3>
						<dl class="mt-2">
							{#if selectedApp['supported-screen-size']}
								<div class="flex items-baseline justify-between gap-4 py-2">
									<dt class="meta">Screen Size</dt>
									<dd class="font-mono text-sm">{selectedApp['supported-screen-size']}</dd>
								</div>
							{:else}
								<div class="py-2">
									<dt class="meta">Supported Devices</dt>
									<dd class="mt-1.5 text-xs leading-relaxed text-[var(--text-dim)]">{formatSupportedDevices(selectedApp['supported-devices'])}</dd>
								</div>
							{/if}
						</dl>
					</div>
				</div>

				{#if selectedApp.files && selectedApp.files.length > 0}
					<div class="mt-6">
						<h3 class="eyebrow">Files</h3>
						<div class="mt-2 space-y-1 border border-[var(--rule)] bg-black/40 p-3">
							{#each selectedApp.files as file}
								<div class="font-mono text-xs break-all">
									{#if typeof file === 'string'}
										<span class="text-[var(--text-dim)]">{file}</span>
									{:else}
										<span class="text-[var(--text-faint)]">{file.source}</span>
										<span class="mx-1 inline-flex translate-y-[2px] text-[var(--text-faint)]"><Icon name="arrow-right" size={11} /></span>
										<span class="text-[var(--text-dim)]">{file.destination}</span>
									{/if}
								</div>
							{/each}
						</div>
					</div>
				{/if}

				<div class="mt-6 border-t border-[var(--rule)] pt-4">
					<p class="meta">
						Commit
						<a href="https://github.com/{selectedApp.owner}/{selectedApp.repo}/commit/{selectedApp.commit}" target="_blank" class="font-mono">
							{selectedApp.commit}
						</a>
					</p>
					<p class="meta mt-1">Last Updated {new Date(selectedApp.lastUpdated * 1000).toLocaleDateString()}</p>
				</div>
			</div>
		</div>
	</div>
{/if}

<style>
	@keyframes shimmer {
		0% {
			background-position: -200px 0;
		}
		100% {
			background-position: calc(200px + 100%) 0;
		}
	}

	.placeholder-shimmer {
		background: linear-gradient(90deg, rgba(255, 255, 255, 0.04) 25%, rgba(255, 255, 255, 0.09) 50%, rgba(255, 255, 255, 0.04) 75%);
		background-size: 200px 100%;
		animation: shimmer 1.5s infinite;
	}

	@keyframes spin {
		to {
			transform: rotate(360deg);
		}
	}

	.spinner {
		width: 0.85rem;
		height: 0.85rem;
		border: 2px solid rgba(255, 255, 255, 0.35);
		border-top-color: #fff;
		border-radius: 50%;
		animation: spin 0.7s linear infinite;
	}

	.line-clamp-3 {
		display: -webkit-box;
		-webkit-line-clamp: 3;
		-webkit-box-orient: vertical;
		overflow: hidden;
	}

	.modal-content::-webkit-scrollbar {
		width: 8px;
	}

	.modal-content::-webkit-scrollbar-track {
		background: transparent;
	}

	.modal-content::-webkit-scrollbar-thumb {
		background: rgba(255, 255, 255, 0.16);
	}

	.modal-content::-webkit-scrollbar-thumb:hover {
		background: var(--color-brand);
	}

	.modal-content {
		scrollbar-width: thin;
		scrollbar-color: rgba(255, 255, 255, 0.16) transparent;
	}
</style>
