<script lang="ts">
    import pinouts from '$lib/data/pinouts.json';
    import SectionBackground from '$lib/components/SectionBackground.svelte';
    import { current_page, Page } from '$lib/store';

    $current_page = Page.Pinouts; 

    let selectedDevice = $state('');
    let selectedModule = $state('');

    const devices = Object.keys(pinouts);
    
    let availableModules = $derived(selectedDevice ? Object.keys(pinouts[selectedDevice]) : []);
    let activePinout = $derived(
        selectedDevice && selectedModule ? pinouts[selectedDevice][selectedModule] : null
    );

    const RANDOM_PALETTE = ['#ffaa00', '#0088ff', '#9b51e0', '#00aa00', '#ff00ff', '#00ffff'];

    const getPinColor = (label: string, index: number) => {
        const pin = label.toUpperCase();
        if (pin === 'VCC' || pin === '5V' || pin === '3.3V') return '#ff4444'; 
        if (pin === 'GND') return '#777777'; 
        
        return RANDOM_PALETTE[index % RANDOM_PALETTE.length];
    };

    const btnClass = (active: boolean) => 
        `flex min-h-[2.5rem] min-w-[10rem] cursor-pointer items-center justify-center rounded-lg border-2 border-[#9B51E0] px-5 py-2.5 transition-all duration-300 ease-in-out ${
            active ? 'bg-[#9B51E0] text-white' : 'text-purple-500 hover:bg-[#9B51E0] hover:text-white'
        }`;
</script>

<section class="relative flex h-[500px] w-full flex-col overflow-hidden px-4 md:flex-row">
    <SectionBackground />
    <div class="relative z-10 flex flex-col justify-center p-8 text-white">
        <h1 class="mb-5 text-4xl font-bold md:text-6xl">Component Pinouts</h1>
        <p class="mb-7 text-xl opacity-80">Simple wiring guides for your Bruce hardware expansions.</p>
    </div>
</section>

<div class="container mt-10">
    <div class="flex flex-col items-center space-y-8">
        <h2 class="text-2xl font-bold text-white">Select Device</h2>
        <div class="flex flex-wrap justify-center gap-4">
            {#each devices as device}
                <button 
                    onclick={() => { selectedDevice = device; selectedModule = ''; }}
                    class={btnClass(selectedDevice === device)}
                >
                    {device}
                </button>
            {/each}
            
            <div class="coming-soon-card">
                <span class="text-sm uppercase tracking-tighter opacity-60">More Coming Soon</span>
            </div>
        </div>

        {#if selectedDevice}
            <h2 class="text-2xl font-bold text-white mt-5">Select Module</h2>
            <div class="flex flex-wrap justify-center gap-4">
                {#each availableModules as mod}
                    <button 
                        onclick={() => selectedModule = mod}
                        class={btnClass(selectedModule === mod)}
                    >
                        {mod}
                    </button>
                {/each}
            </div>
        {/if}
    </div>

    {#if activePinout}
        <div class="mt-16 rounded-xl border-2 border-purple-500 bg-[#9B51E0]/10 p-8 shadow-[0px_0px_10px_rgba(155,81,224,0.2)]">
            <div class="grid md:grid-cols-2 gap-12">
                
                <div class="space-y-6">
                    <div>
                        <h2 class="text-3xl font-bold text-white mb-2">{selectedModule}</h2>
                        <p class="text-gray-300 leading-relaxed italic">{activePinout.info}</p>
                    </div>

                    {#if activePinout.warning}
                        <div class="rounded-lg border border-red-500/50 bg-red-500/10 p-4 text-red-200 text-sm">
                            <strong class="text-red-500 uppercase">Warning:</strong> {activePinout.warning}
                        </div>
                    {/if}
                </div>

                <div class="bg-black/40 rounded-lg p-6 border border-purple-500/30">
                    <h3 class="text-purple-500 font-bold mb-4 uppercase tracking-wider">Wiring Map</h3>
                    <div class="space-y-3">
                        {#each activePinout.connections as conn, i}
                            <div class="flex items-center justify-between py-2 border-b border-white/10 last:border-0">
                                <div class="flex items-center gap-3">
                                    <div 
                                        class="w-3 h-3 rounded-full shadow-sm" 
                                        style="background: {getPinColor(conn.module, i)}"
                                    ></div>
                                    <span class="text-gray-300 font-mono text-sm uppercase">{conn.module}</span>
                                </div>
                                <span class="text-white font-bold font-mono text-lg">→ {conn.device}</span>
                            </div>
                        {/each}
                    </div>
                </div>
            </div>
        </div>
    {/if}
</div>

<div class="h-20"></div>

<style>
    .container {
        width: 90%;
        max-width: 1000px;
        margin: 0 auto;
    }

    .coming-soon-card {
        display: flex;
        min-height: 2.5rem;
        min-width: 10rem;
        items-center: center;
        justify-content: center;
        border-radius: 0.5rem;
        border: 2px dashed rgba(155, 81, 224, 0.3);
        background: rgba(155, 81, 224, 0.05); 
        padding: 0.625rem 1.25rem;
        color: #9B51E0;
        opacity: 0.5;
        cursor: default;
        transition: all 0.3s ease;
    }

    .coming-soon-card:hover {
        opacity: 0.8;
        border-color: rgba(155, 81, 224, 0.6);
        transform: translateY(-1px);
    }
</style>