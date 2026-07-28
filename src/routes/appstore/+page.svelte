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
			downloadError = (error && typeof error === 'object' && 'message' in error)
				? (error as { message: string }).message
				: 'Failed to download files';
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

<div class="mt-32 text-center">
	<AttentionBanner />
	<InstallationBanner />

	<!-- Search and Filter Section -->
	<div class="mt-6 mb-8 flex flex-col items-center gap-6">
		<!-- Combined Search and Device Filter (horizontal on large screens) -->
		<div class="flex w-full max-w-4xl flex-col items-center gap-6 lg:flex-row">
			<!-- Search Section -->
			<div class="flex flex-1 flex-col items-center gap-3">
				<div class="flex w-full max-w-md flex-col items-center gap-3 sm:flex-row">
					<div class="relative w-full">
						<input
							type="text"
							placeholder="Search..."
							bind:value={localSearchQuery}
							oninput={(e) => { if (e.target) applySearchFilter((e.target as HTMLInputElement).value); }}
							class="w-full min-w-[300px] rounded-lg border border-gray-600 bg-gray-700 px-4 py-2 pl-10 text-white transition-all duration-200 focus:border-purple-500 focus:ring-2 focus:ring-purple-500/20 focus:outline-none"
						/>
						<!-- Search Icon -->
						<svg
							class="absolute top-1/2 left-3 h-4 w-4 -translate-y-1/2 transform text-gray-400"
							fill="none"
							stroke="currentColor"
							viewBox="0 0 24 24"
						>
							<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="m21 21-6-6m2-5a7 7 0 1 1-14 0 7 7 0 0 1 14 0z"></path>
						</svg>
						{#if localSearchQuery}
							<button
								onclick={() => {
									localSearchQuery = '';
									applySearchFilter('');
								}}
								class="absolute top-1/2 right-3 -translate-y-1/2 transform text-gray-400 hover:text-white"
							>
								<svg class="h-4 w-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
									<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12"></path>
								</svg>
							</button>
						{/if}
					</div>
				</div>
			</div>

			<!-- Device Filter Section -->
			{#if initialLoad || $isLoadingData}
				<!-- Device filter loading placeholder -->
				<div class="flex flex-1 flex-col items-center gap-3">
					<div class="placeholder-shimmer h-11 w-48 rounded-lg border border-gray-600 bg-gray-700 px-4 py-2"></div>
					<div class="placeholder-shimmer h-6 w-20 rounded bg-gray-600 px-2 py-1"></div>
				</div>
			{:else if $supportedDevices.length > 0}
				<div class="flex flex-1 flex-col items-center gap-3">
					<div class="flex w-full max-w-md flex-col items-center gap-3 sm:flex-row">
						<!-- Searchable Dropdown -->
						<div class="relative w-full" id="device-dropdown">
							<button
								class="flex w-full min-w-[300px] items-center justify-between rounded-lg border border-gray-600 bg-gray-700 px-4 py-2 text-white transition-all duration-200 focus:border-purple-500 focus:ring-2 focus:ring-purple-500/20 focus:outline-none"
								onclick={toggleDeviceDropdown}
							>
								<span>{$selectedDevice}</span>
								<svg
									class="ml-2 h-4 w-4 transition-transform duration-200"
									class:rotate-180={showDeviceDropdown}
									fill="none"
									stroke="currentColor"
									viewBox="0 0 24 24"
								>
									<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="m19 9-7 7-7-7"></path>
								</svg>
							</button>

							{#if showDeviceDropdown}
								<div class="absolute z-50 mt-1 max-h-60 w-full overflow-y-auto rounded-lg border border-gray-600 bg-gray-700 shadow-xl">
									<!-- Search Input -->
									<div class="border-b border-gray-600 p-2">
										<input
											id="device-search-input"
											type="text"
											placeholder="Search devices..."
											bind:value={deviceSearchQuery}
											class="w-full rounded border border-gray-600 bg-gray-800 px-3 py-2 text-sm text-white focus:border-purple-500 focus:ring-1 focus:ring-purple-500/50 focus:outline-none"
										/>
									</div>

									<!-- Device Options -->
									<div class="max-h-48 overflow-y-auto">
										{#each filteredDevicesForSearch as device}
											<button
												class="w-full px-4 py-2 text-left text-sm text-white transition-colors duration-150 hover:bg-gray-600"
												class:bg-purple-600={$selectedDevice === device.name}
												class:hover:bg-purple-500={$selectedDevice === device.name}
												onclick={() => handleDeviceFilter(device.name)}
											>
												{device.name}
											</button>
										{:else}
											<div class="px-4 py-2 text-gray-400 text-sm">No devices found</div>
										{/each}
									</div>
								</div>
							{/if}
						</div>
					</div>
				</div>
			{/if}
		</div>

		<!-- Search Results Info -->
		{#if $searchQuery}
			<div class="text-sm text-gray-400">
				Found {$searchedApps.length} result{$searchedApps.length === 1 ? '' : 's'} for "{$searchQuery}"
			</div>
		{/if}
	</div>

	{#if initialLoad || $isLoadingData}
		<!-- Category buttons loading placeholder -->
		{#each Array(8) as _, i}
			<div class="m-1 inline-flex items-center gap-2 rounded-full border-2 border-gray-600 bg-gray-700 px-3 py-2 sm:gap-3 sm:px-6 sm:py-3">
				<div class="placeholder-shimmer h-4 w-16 rounded bg-gray-600 sm:h-6 sm:w-20"></div>
			</div>
		{/each}
	{:else}
		{#each $searchFilteredCategories as category}
			<button onclick={() => filter(category.name, category.slug)}>
				<div
					class="m-1 inline-flex items-center gap-2 rounded-full border-2 px-3 py-2 text-white transition-all duration-300 ease-in-out hover:scale-105 hover:shadow-2xl sm:gap-3 md:px-4 md:py-2"
					style="background-color: {$selectedCategory === category.name ? 'rgb(155, 81, 224)' : 'black'}; border-color: {$selectedCategory ===
					category.name
						? 'rgb(128, 51, 199)'
						: 'rgb(155, 81, 224)'};"
					onmouseenter={(e) => {
						if ($selectedCategory !== category.name) {
							e.target.style.borderColor = 'rgb(128, 51, 199)';
							e.target.style.backgroundColor = 'rgb(128, 51, 199)';
						}
					}}
					onmouseleave={(e) => {
						if ($selectedCategory !== category.name) {
							e.target.style.borderColor = 'rgb(155, 81, 224)';
							e.target.style.backgroundColor = 'black';
						}
					}}
				>
					<!-- Text -->
					<span class="text-sm font-semibold tracking-wide md:text-base">{capitalize(category.name)} ({category.count})</span>
				</div>
			</button>
		{/each}
	{/if}

	<!-- Loading state -->
	{#if initialLoad || $isLoadingData}
		<div class="mt-8">
			<!-- Loading spinner -->
			<div class="flex flex-col items-center justify-center py-12">
				<div class="relative">
					<!-- Spinning ring -->
					<div class="h-16 w-16 animate-spin rounded-full border-4 border-purple-500 border-t-transparent"></div>
					<!-- Inner pulse -->
					<div class="absolute inset-2 animate-pulse rounded-full bg-purple-500 opacity-20"></div>
				</div>
				<p class="mt-4 text-lg text-gray-300">{initialLoad ? 'Initializing app store...' : 'Loading apps and categories...'}</p>
				<p class="text-sm text-gray-500">{initialLoad ? 'Setting up the interface' : 'Please wait while we fetch the latest data'}</p>
			</div>

			<!-- Placeholder cards while loading -->
			<div class="mx-auto mt-8 grid max-w-6xl grid-cols-1 gap-4 px-2 sm:gap-6 sm:px-0 md:grid-cols-2 lg:grid-cols-3">
				{#each Array(6) as _, i}
					<div class="overflow-hidden rounded-lg bg-gray-800 shadow-lg">
						<!-- Placeholder image -->
						<div class="flex h-48 items-center justify-center bg-gray-700">
							<div class="placeholder-shimmer h-24 w-24 rounded-lg bg-gray-600"></div>
						</div>

						<!-- Placeholder content -->
						<div class="space-y-3 p-4">
							<!-- Title placeholder -->
							<div class="placeholder-shimmer h-5 w-3/4 rounded bg-gray-600"></div>
							<!-- Author placeholder -->
							<div class="placeholder-shimmer h-3 w-1/2 rounded bg-gray-700"></div>
							<!-- Version placeholder -->
							<div class="placeholder-shimmer h-3 w-1/3 rounded bg-gray-700"></div>
							<!-- Description placeholder -->
							<div class="space-y-2">
								<div class="placeholder-shimmer h-3 rounded bg-gray-700"></div>
								<div class="placeholder-shimmer h-3 w-5/6 rounded bg-gray-700"></div>
							</div>
						</div>
					</div>
				{/each}
			</div>
		</div>
	{/if}

	<!-- Category apps display -->
	{#if $searchedApps.length > 0}
		<div class="mt-8 mb-8">
			<h3 class="mb-4 text-xl font-bold">
				{$searchQuery ? `Search Results` : $selectedCategory === 'All' ? 'All Apps/Themes' : $selectedCategory}
				{$selectedDevice !== 'All Devices' ? ` for ${$selectedDevice}` : ''}
				{$searchQuery ? ` (${$searchedApps.length} found)` : ''}
			</h3>
			<div class="mx-auto grid max-w-6xl grid-cols-1 gap-4 px-2 pb-8 sm:gap-6 sm:px-0 md:grid-cols-2 lg:grid-cols-3">
				{#each $searchedApps as app}
					<div
						class="relative transform cursor-pointer overflow-hidden rounded-lg bg-gray-800 shadow-lg transition-shadow hover:scale-105 hover:shadow-xl"
						onclick={() => openAppModal(app)}
					>
						<!-- Category Pill -->
						<div class="absolute top-2 left-2 z-10">
							<span class="rounded-full px-2 py-1 text-xs font-medium text-white" style="background-color: rgb(155, 81, 224);">
								{app.category}
							</span>
						</div>

						<!-- App Logo -->
						<div class="flex h-48 items-center justify-center bg-gray-700">
							<img src={getLogoUrl(app.slug)} alt={app.name} class="max-h-32 max-w-32 object-contain" onerror={handleImageError} />
						</div>

						<!-- App Info -->
						<div class="p-4 lg:pt-6">
							<h4 class="text-lg font-bold text-white">
								{app.name}
								{#if app.category === 'Themes' && app['supported-screen-size']}
									<span class="text-sm text-gray-400">({app['supported-screen-size']})</span>
								{/if}
							</h4>
							<p class="mb-2 text-xs text-gray-400">
								By: <a
									href="https://github.com/{app.owner}"
									target="_blank"
									class="text-purple-400 underline hover:text-purple-300"
									onclick={(e) => e.stopPropagation()}>{app.owner}</a
								>
							</p>
							<p class="mb-2 text-xs text-gray-400">Version: {app.version}</p>
							<p class="text-sm text-gray-300">{app.description}</p>
						</div>
					</div>
				{/each}
			</div>
		</div>
	{:else if $searchQuery && $filteredApps.length > 0}
		<!-- No search results found -->
		<div class="mt-8 text-center">
			<div class="mx-auto max-w-md rounded-lg bg-gray-800 p-8">
				<svg class="mx-auto mb-4 h-12 w-12 text-gray-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
					<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="m21 21-6-6m2-5a7 7 0 1 1-14 0 7 7 0 0 1 14 0z"></path>
				</svg>
				<h3 class="mb-2 text-lg font-semibold text-white">No Results Found</h3>
				<p class="mb-4 text-gray-400">
					No apps found matching "{$searchQuery}". Try a different search term or clear the search to see all apps.
				</p>
				<button
					onclick={() => {
						localSearchQuery = '';
						applySearchFilter('');
					}}
					class="rounded-lg bg-purple-600 px-4 py-2 font-medium text-white transition-colors hover:bg-purple-700"
				>
					Clear Search
				</button>
			</div>
		</div>
	{/if}

	<!-- Original markdown apps (shown when no category selected) -->
	<div class="grid grid-cols-3">
		{#each applications as element}
			{@const CardApp = element[1].default}
			<a href="{base}/appstore/{element[1].metadata.id}">
				<CardApp card href="" />
			</a>
		{/each}
	</div>
</div>

<!-- App Detail Modal -->
{#if showModal && selectedApp}
	<div class="bg-opacity-50 fixed inset-0 z-50 flex items-start justify-center bg-black p-2 pt-32 sm:p-4 sm:pt-32 lg:pt-32">
		<div class="flex max-h-[80vh] w-full max-w-xs flex-col rounded-lg bg-gray-800 sm:max-h-[80vh] sm:max-w-md md:max-w-lg lg:max-w-2xl">
			<!-- Modal Header -->
			<div class="flex-shrink-0 border-b border-gray-700 bg-gray-800 p-3 sm:p-6">
				<div class="mb-3 flex items-start justify-between">
					<div class="min-w-0 flex-1 pr-3">
						<h2 class="mb-1 text-xl font-bold break-words text-white sm:text-2xl">
							{selectedApp.name}
							{#if selectedApp.category === 'Themes' && selectedApp['supported-screen-size']}
								<span class="block text-sm text-gray-400 sm:inline sm:text-lg">({selectedApp['supported-screen-size']})</span>
							{/if}
						</h2>
						<span class="rounded-full px-2 py-1 text-xs font-medium text-white sm:px-3 sm:text-sm" style="background-color: rgb(155, 81, 224);">
							{selectedApp.category}
						</span>
					</div>
					<!-- Close Button -->
					<button class="-mt-2 ml-3 flex-shrink-0 p-1 text-xl font-bold text-gray-400 hover:text-white sm:text-2xl" onclick={closeModal}> × </button>
				</div>

				<!-- Action Buttons -->
				{#if selectedApp.files && selectedApp.files.length > 0}
					<div class="flex items-center justify-center gap-2 sm:gap-3">
						<!-- Install Button -->
						<button
							class="flex items-center gap-1 rounded-lg bg-green-600 px-3 py-1.5 text-sm font-medium text-white transition-colors hover:bg-green-700 disabled:bg-green-400 sm:gap-2 sm:px-4 sm:py-2 sm:text-base"
							disabled={isInstalling || isDownloading}
							onclick={() => installApp(selectedApp)}
						>
							{#if isInstalling}
								<div class="h-3 w-3 animate-spin rounded-full border-2 border-white border-t-transparent sm:h-4 sm:w-4"></div>
								Installing...
							{:else}
								<svg class="h-3 w-3 sm:h-4 sm:w-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
									<path
										stroke-linecap="round"
										stroke-linejoin="round"
										stroke-width="2"
										d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4"
									></path>
								</svg>
								<span class="hidden sm:inline">Install</span>
								<span class="sm:hidden">Install</span>
							{/if}
						</button>
						<!-- Download Button -->
						<button
							class="flex items-center gap-1 rounded-lg bg-purple-600 px-3 py-1.5 text-sm font-medium text-white transition-colors hover:bg-purple-700 disabled:bg-purple-400 sm:gap-2 sm:px-4 sm:py-2 sm:text-base"
							disabled={isDownloading}
							onclick={() => downloadAppFiles(selectedApp)}
						>
							{#if isDownloading}
								<div class="h-3 w-3 animate-spin rounded-full border-2 border-white border-t-transparent sm:h-4 sm:w-4"></div>
								Downloading...
							{:else}
								<svg class="h-3 w-3 sm:h-4 sm:w-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
									<path
										stroke-linecap="round"
										stroke-linejoin="round"
										stroke-width="2"
										d="M12 10v6m0 0l-3-3m3 3l3-3m2 8H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z"
									></path>
								</svg>
								Download
							{/if}
						</button>
					</div>
				{/if}

				<!-- Cancel Install Confirmation -->
				{#if showCancelConfirm}
					<div class="mt-6 flex flex-col items-center justify-center rounded-lg border border-red-600 bg-red-900 p-4">
						<p class="text-center text-sm font-medium text-red-200">An installation is in progress. Are you sure you want to cancel?</p>
						<div class="mt-4 flex gap-4">
							<button
								class="rounded-lg bg-red-700 px-4 py-2 text-sm font-medium text-white transition-colors hover:bg-red-800"
								onclick={() => {
									installService.cancelInstall();
									showCancelConfirm = false;
									showModal = false;
									selectedApp = null;
								}}
							>
								Yes, Cancel
							</button>
							<button
								class="rounded-lg bg-gray-600 px-4 py-2 text-sm font-medium text-white transition-colors hover:bg-gray-700"
								onclick={() => (showCancelConfirm = false)}
							>
								No, Continue
							</button>
						</div>
					</div>
				{/if}

				<!-- Install Popup -->
				{#if showInstallPopup}
					<div class="mt-6 flex items-start justify-between rounded-lg border border-yellow-600 bg-yellow-900 p-3">
						<div>
							{#if !isInstallEnabled}
								<p class="text-sm font-medium text-yellow-200">⚠️ Install feature not implemented</p>
								<p class="mt-1 text-xs text-yellow-300">
									Direct installation is temporarily disabled. Please use the download button and upload files manually using the <a
										href="https://wiki.bruce.computer/controlling-device/webui/"
										target="_blank"
										class="underline hover:text-yellow-200">WebUI</a
									>.
								</p>
							{:else if !isWebSerialSupported}
								<p class="text-sm font-medium text-yellow-200">⚠️ Browser not supported for direct installation</p>
								<p class="mt-1 text-xs text-yellow-300">
									{browserUnsupportedReason || 'Your browser does not support Web Serial API.'} Use the download button instead and upload files manually
									using the
									<a href="https://wiki.bruce.computer/controlling-device/webui/" target="_blank" class="underline hover:text-yellow-200">WebUI</a>.
								</p>
							{:else}
								<p class="text-sm font-medium text-yellow-200">⚠️ No files available for installation</p>
								<p class="mt-1 text-xs text-yellow-300">
									This app does not have any files configured for installation. Please contact the app developer.
								</p>
							{/if}
						</div>
						<button
							class="-mt-2 ml-3 flex-shrink-0 text-lg font-bold text-yellow-400 hover:text-yellow-200"
							onclick={() => (showInstallPopup = false)}
						>
							×
						</button>
					</div>
				{/if}

				<!-- Install Progress -->
				{#if installProgress}
					<div
						class="mt-6 p-3 {installProgress.stage === 'error'
							? 'border-red-600 bg-red-900'
							: installProgress.stage === 'success'
								? 'border-green-600 bg-green-900'
								: 'border-blue-600 bg-blue-900'} rounded-lg border"
					>
						<div class="mb-2 flex items-center justify-between">
							<p
								class="{installProgress.stage === 'error'
									? 'text-red-200'
									: installProgress.stage === 'success'
										? 'text-green-200'
										: 'text-blue-200'} text-sm font-medium"
							>
								{#if installProgress.stage === 'error'}
									❌ Installation Failed
								{:else if installProgress.stage === 'success'}
									✅ Installation Complete
								{:else if installProgress.stage === 'connecting'}
									🔌 Connecting to Device
								{:else if installProgress.stage === 'uploading'}
									📤 Uploading Files
								{:else if installProgress.stage === 'verifying'}
									✅ Verifying Installation
								{:else if installProgress.stage === 'rebooting'}
									⏳ Rebooting...
								{/if}
							</p>
							{#if installProgress.stage === 'error' || installProgress.stage === 'success'}
								<button
									class="{installProgress.stage === 'error'
										? 'text-red-400 hover:text-red-200'
										: 'text-green-400 hover:text-green-200'} -mt-2 ml-3 flex-shrink-0 text-lg font-bold"
									onclick={() => (installProgress = null)}
								>
									×
								</button>
							{/if}
						</div>

						{#if installProgress.stage !== 'error'}
							<!-- Progress Bar -->
							<div class="mb-2 h-2 w-full rounded-full bg-gray-700">
								<div
									class="{installProgress.stage === 'success' ? 'bg-green-500' : 'bg-blue-500'} h-2 rounded-full transition-all duration-300"
									style="width: {installProgress.progress}%"
								></div>
							</div>
						{/if}

						<p
							class="{installProgress.stage === 'error'
								? 'text-red-300'
								: installProgress.stage === 'success'
									? 'text-green-300'
									: 'text-blue-300'} text-xs"
						>
							{installProgress.message}
							{#if installProgress.speedBps > 0}
								<span class="ml-2 font-mono">({(installProgress.speedBps / 1024).toFixed(1)} KB/s)</span>
							{/if}
						</p>

						{#if installProgress.error}
							<p class="mt-1 font-mono text-xs text-red-300">{installProgress.error}</p>
						{/if}
					</div>
				{/if}

				<!-- Download Complete Popup -->
				{#if showDownloadComplete}
					<div class="mt-6 flex items-start justify-between rounded-lg border border-green-600 bg-green-900 p-3">
						<div>
							<p class="text-sm font-medium text-green-200">✓ Download completed!</p>
							<p class="mt-1 text-xs text-green-300">
								Files have been downloaded. Upload the files to your device using the <a
									href="https://wiki.bruce.computer/controlling-device/webui/"
									target="_blank"
									class="underline hover:text-yellow-200">WebUI</a
								> - extract the files from the .zip first.
							</p>
						</div>
						<button
							class="-mt-2 ml-3 flex-shrink-0 text-lg font-bold text-green-400 hover:text-green-200"
							onclick={() => (showDownloadComplete = false)}
						>
							×
						</button>
					</div>
				{/if}

				<!-- Download Progress/Error -->
				{#if downloadProgress}
					<div class="mb-4 rounded-lg border border-blue-600 bg-blue-900 p-3">
						<p class="text-sm text-blue-200">{downloadProgress}</p>
					</div>
				{/if}
				{#if downloadError}
					<div class="mt-6 mb-4 rounded-lg border border-red-600 bg-red-900 p-3">
						<p class="text-sm text-red-200">{downloadError}</p>
						<button class="mt-1 text-xs text-red-300 underline hover:text-red-100" onclick={() => (downloadError = '')}> Dismiss </button>
					</div>
				{/if}
			</div>

			<!-- Modal Content -->
			<div class="modal-content flex-1 overflow-y-auto p-3 sm:p-6">
				<!-- App Logo -->
				<div class="mb-6 flex justify-center">
					<img src={getLogoUrl(selectedApp.slug)} alt={selectedApp.name} class="max-h-32 max-w-32 object-contain" onerror={handleImageError} />
				</div>

				<!-- Description -->
				<div class="mb-6">
					<h3 class="mb-2 text-lg font-semibold text-white">Description</h3>
					<p class="text-gray-300">{selectedApp.description || 'No description available.'}</p>
				</div>

				<!-- App Details -->
				<div class="mb-6 grid grid-cols-1 gap-6 md:grid-cols-2">
					<div>
						<h3 class="mb-2 text-lg font-semibold text-white">Details</h3>
						<div class="space-y-2 text-sm">
							<p class="text-gray-300"><span class="font-medium text-white">Version:</span> {selectedApp.version}</p>
							<p class="text-gray-300">
								<span class="font-medium text-white">Owner:</span>
								<a href="https://github.com/{selectedApp.owner}" target="_blank" class="text-purple-400 underline hover:text-purple-300">
									{selectedApp.owner}
								</a>
							</p>
							<p class="text-gray-300">
								<span class="font-medium text-white">Repository:</span>
								<a
									href="https://github.com/{selectedApp.owner}/{selectedApp.repo}"
									target="_blank"
									class="text-purple-400 underline hover:text-purple-300"
								>
									{selectedApp.repo}
								</a>
							</p>
						</div>
					</div>

					<div>
						<h3 class="mb-2 text-lg font-semibold text-white">Compatibility</h3>
						<div class="space-y-2 text-sm">
							{#if selectedApp['supported-screen-size']}
								<p class="text-gray-300"><span class="font-medium text-white">Screen Size:</span> {selectedApp['supported-screen-size']}</p>
							{:else}
								<p class="text-gray-300"><span class="font-medium text-white">Supported Devices:</span></p>
								<p class="text-xs text-gray-400">{formatSupportedDevices(selectedApp['supported-devices'])}</p>
							{/if}
						</div>
					</div>
				</div>

				<!-- Files -->
				{#if selectedApp.files && selectedApp.files.length > 0}
					<div class="mb-6">
						<h3 class="mb-2 text-lg font-semibold text-white">Files</h3>
						<div class="space-y-1 rounded bg-gray-900 p-3">
							{#each selectedApp.files as file}
								<div class="text-sm">
									{#if typeof file === 'string'}
										<span class="font-mono text-green-400">{file}</span>
									{:else}
										<span class="font-mono text-blue-400">{file.source}</span>
										<span class="text-gray-400">→</span>
										<span class="font-mono text-green-400">{file.destination}</span>
									{/if}
								</div>
							{/each}
						</div>
					</div>
				{/if}

				<!-- Footer Info -->
				<div class="border-t border-gray-700 pt-4 text-xs text-gray-500">
					<p>
						Commit:
						<a
							href="https://github.com/{selectedApp.owner}/{selectedApp.repo}/commit/{selectedApp.commit}"
							target="_blank"
							class="font-mono text-purple-400 underline hover:text-purple-300"
						>
							{selectedApp.commit}
						</a>
					</p>
					<p>Last Updated: {new Date(selectedApp.lastUpdated * 1000).toLocaleDateString()}</p>
				</div>
			</div>
		</div>
	</div>
{/if}

<style>
	/* Custom loading animations */
	@keyframes spin {
		0% {
			transform: rotate(0deg);
		}
		100% {
			transform: rotate(360deg);
		}
	}

	@keyframes pulse {
		0%,
		100% {
			opacity: 0.2;
		}
		50% {
			opacity: 0.6;
		}
	}

	.animate-spin {
		animation: spin 1s linear infinite;
	}

	.animate-pulse {
		animation: pulse 2s ease-in-out infinite;
	}

	/* Improved loading placeholder animation */
	@keyframes shimmer {
		0% {
			background-position: -200px 0;
		}
		100% {
			background-position: calc(200px + 100%) 0;
		}
	}

	.placeholder-shimmer {
		background: linear-gradient(90deg, #374151 25%, #4b5563 50%, #374151 75%);
		background-size: 200px 100%;
		animation: shimmer 1.5s infinite;
	}

	/* Modal-specific scrollbar styling */
	.modal-content ::-webkit-scrollbar {
		width: 8px;
	}

	.modal-content ::-webkit-scrollbar-track {
		background: #374151; /* gray-700 */
		border-radius: 4px;
	}

	.modal-content ::-webkit-scrollbar-thumb {
		background: #4b5563; /* gray-600 */
		border-radius: 4px;
		transition: background-color 0.2s ease;
	}

	.modal-content ::-webkit-scrollbar-thumb:hover {
		background: #8b5cf6; /* purple-500 */
	}

	/* Modal-specific Firefox scrollbar styling */
	.modal-content {
		scrollbar-width: thin;
		scrollbar-color: #4b5563 #374151; /* thumb track */
	}
</style>
