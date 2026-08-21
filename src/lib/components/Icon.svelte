<script lang="ts">
	/*
	 * Inline icon set. Deliberately local rather than an icon package: the site
	 * needs a dozen glyphs, and this keeps the bundle free of a dependency and
	 * the CSP free of an external font/sprite request.
	 *
	 * Every icon replaces an emoji that used to be typed into the markup, so the
	 * names map to what they mean here (`yes`/`no` rather than `check`/`cross`).
	 */
	let {
		name,
		size = 16,
		stroke = 1.75,
		class: klass = ''
	} = $props<{
		name: string;
		size?: number | string;
		stroke?: number;
		class?: string;
	}>();

	// Stroke-drawn paths on a 24x24 grid.
	const paths: Record<string, string[]> = {
		yes: ['M20 6 9 17l-5-5'],
		no: ['M18 6 6 18', 'M6 6l12 12'],
		info: ['M12 16v-4', 'M12 8h.01', 'M21 12a9 9 0 1 1-18 0 9 9 0 0 1 18 0Z'],
		warning: ['M10.29 3.86 1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0Z', 'M12 9v4', 'M12 17h.01'],
		close: ['M18 6 6 18', 'M6 6l12 12'],
		menu: ['M4 7h16', 'M4 12h16', 'M4 17h16'],
		search: ['M11 18a7 7 0 1 0 0-14 7 7 0 0 0 0 14Z', 'M21 21l-4.35-4.35'],
		download: ['M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4', 'M7 10l5 5 5-5', 'M12 15V3'],
		upload: ['M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4', 'M17 8l-5-5-5 5', 'M12 3v12'],
		plug: ['M12 22v-5', 'M9 8V2', 'M15 8V2', 'M18 8v3a6 6 0 0 1-12 0V8Z'],
		clock: ['M12 21a9 9 0 1 0 0-18 9 9 0 0 0 0 18Z', 'M12 7v5l3 2'],
		wrench: ['M14.7 6.3a4 4 0 0 0 5 5l-9.4 9.4a2.1 2.1 0 0 1-3-3Z', 'M14.7 6.3 18 3'],
		'volume-low': ['M11 5 6 9H2v6h4l5 4V5Z', 'M15.5 9.5a3 3 0 0 1 0 5'],
		'volume-high': ['M11 5 6 9H2v6h4l5 4V5Z', 'M15.5 9.5a3 3 0 0 1 0 5', 'M18.5 6.5a7 7 0 0 1 0 11'],
		'arrow-right': ['M5 12h14', 'M13 6l6 6-6 6'],
		'arrow-up': ['M12 19V5', 'M6 11l6-6 6 6'],
		'arrow-down': ['M12 5v14', 'M6 13l6 6 6-6'],
		'chevron-up': ['M6 15l6-6 6 6'],
		'chevron-down': ['M6 9l6 6 6-6'],
		'chevron-left': ['M15 6l-6 6 6 6'],
		'chevron-right': ['M9 6l6 6-6 6'],
		'chevrons-up': ['M6 17l6-6 6 6', 'M6 11l6-6 6 6'],
		'chevrons-down': ['M6 7l6 6 6-6', 'M6 13l6 6 6-6'],
		'corner-back': ['M9 14 4 9l5-5', 'M20 20v-7a4 4 0 0 0-4-4H4'],
		external: ['M15 3h6v6', 'M10 14 21 3', 'M21 14v5a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5'],
		github: [
			'M9 19c-5 1.5-5-2.5-7-3m14 6v-3.87a3.37 3.37 0 0 0-.94-2.61c3.14-.35 6.44-1.54 6.44-7A5.44 5.44 0 0 0 20 4.77 5.07 5.07 0 0 0 19.91 1S18.73.65 16 2.48a13.38 13.38 0 0 0-7 0C6.27.65 5.09 1 5.09 1A5.07 5.07 0 0 0 5 4.77a5.44 5.44 0 0 0-1.5 3.78c0 5.42 3.3 6.61 6.44 7A3.37 3.37 0 0 0 9 18.13V22'
		],
		book: ['M4 19.5A2.5 2.5 0 0 1 6.5 17H20', 'M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2Z'],
		chip: ['M9 3v3', 'M15 3v3', 'M9 18v3', 'M15 18v3', 'M3 9h3', 'M3 15h3', 'M18 9h3', 'M18 15h3', 'M6 6h12v12H6z'],
		grid: ['M3 3h7v7H3z', 'M14 3h7v7h-7z', 'M14 14h7v7h-7z', 'M3 14h7v7H3z'],
		heart: ['M20.8 5.6a5 5 0 0 0-7.1 0L12 7.3l-1.7-1.7a5 5 0 1 0-7.1 7.1l8.8 8.8 8.8-8.8a5 5 0 0 0 0-7.1Z'],
		shield: ['M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10Z'],
		terminal: ['M4 17l6-5-6-5', 'M12 19h8'],
		refresh: ['M21 12a9 9 0 1 1-3-6.7', 'M21 3v6h-6']
	};

	let d = $derived(paths[name] ?? []);
</script>

<svg
	class={klass}
	width={size}
	height={size}
	viewBox="0 0 24 24"
	fill="none"
	stroke="currentColor"
	stroke-width={stroke}
	stroke-linecap="round"
	stroke-linejoin="round"
	aria-hidden="true"
	focusable="false"
>
	{#each d as segment}
		<path d={segment} />
	{/each}
</svg>
