/**
 * `use:reveal` — fades an element up as it scrolls into view.
 *
 * The initial hidden state is added by the action (not in markup), so the
 * content is fully visible without JavaScript and only animates when the script
 * runs. Honours prefers-reduced-motion by leaving the element untouched.
 *
 * Usage: `<section use:reveal>` or `<div use:reveal={{ delay: 120 }}>`
 */
export function reveal(node: HTMLElement, params: { delay?: number } = {}) {
	if (typeof window === 'undefined') return {};

	const reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
	if (reduce || !('IntersectionObserver' in window)) return {};

	if (params.delay) node.style.setProperty('--reveal-delay', `${params.delay}ms`);
	node.classList.add('reveal');

	const io = new IntersectionObserver(
		(entries) => {
			for (const entry of entries) {
				if (entry.isIntersecting) {
					node.classList.add('is-visible');
					io.unobserve(entry.target);
				}
			}
		},
		{ threshold: 0.12, rootMargin: '0px 0px -8% 0px' }
	);

	io.observe(node);

	return {
		destroy() {
			io.disconnect();
		}
	};
}
