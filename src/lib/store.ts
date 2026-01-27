export enum Page {
	Home,
	Flasher,
	Donate,
	MyBruce,
	AppStore,
	Pinouts
}

import { writable } from 'svelte/store';
export const current_page = writable(Page.Home);
