/**
 * Bruce UI Designer Pro — Core Application Logic
 * ================================================
 * Features: Multi-scene, variables, undo/redo, layer ordering,
 * save/load .bruceproj, ZIP export, live C code generation,
 * interactive buttons, image assets, size estimation.
 */

// ═══════════════════════════════════════════════════════════════
// State
// ═══════════════════════════════════════════════════════════════
const State = {
    settings: {
        appId: 'my_app',
        appName: 'My App',
        author: 'Bruce Dev',
        board: 'esp32',
        width: 240,
        height: 135
    },
    variables: [],          // { id, type, name, value }
    scenes: [
        { id: 'scene_0', name: 'Main', elements: [], x: 0, y: 0 }
    ],
    assets: [],             // { id, name, dataURL, w, h }
    activeSceneId: 'scene_0',
    selectedElementId: null
};

// Viewport state for infinite canvas
const Viewport = {
    panX: 0,
    panY: 0,
    zoom: 1,
    MIN_ZOOM: 0.15,
    MAX_ZOOM: 5,
    isPanning: false,
    spaceHeld: false
};

// Undo / Redo stacks (snapshots as JSON strings)
const undoStack = [];
const redoStack = [];
const MAX_UNDO = 60;

function pushUndo() {
    undoStack.push(JSON.stringify({
        settings: State.settings,
        variables: State.variables,
        scenes: State.scenes,
        assets: State.assets,
        activeSceneId: State.activeSceneId,
        selectedElementId: State.selectedElementId
    }));
    if (undoStack.length > MAX_UNDO) undoStack.shift();
    redoStack.length = 0;
}

function doUndo() {
    if (undoStack.length === 0) return;
    redoStack.push(JSON.stringify({
        settings: State.settings,
        variables: State.variables,
        scenes: State.scenes,
        assets: State.assets,
        activeSceneId: State.activeSceneId,
        selectedElementId: State.selectedElementId
    }));
    let snapshot = JSON.parse(undoStack.pop());
    Object.assign(State, snapshot);
    UI.fullRefresh();
}

function doRedo() {
    if (redoStack.length === 0) return;
    undoStack.push(JSON.stringify({
        settings: State.settings,
        variables: State.variables,
        scenes: State.scenes,
        assets: State.assets,
        activeSceneId: State.activeSceneId,
        selectedElementId: State.selectedElementId
    }));
    let snapshot = JSON.parse(redoStack.pop());
    Object.assign(State, snapshot);
    UI.fullRefresh();
}

// ═══════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════
function uid(prefix) {
    return prefix + '_' + Math.random().toString(36).substr(2, 7);
}

function getActiveScene() {
    return State.scenes.find(s => s.id === State.activeSceneId);
}

function getElement(id) {
    for (let s of State.scenes) {
        let el = s.elements.find(e => e.id === id);
        if (el) return el;
    }
    return null;
}

function hexToRgb565(hex) {
    hex = (hex || '#ffffff').replace('#', '');
    let r = parseInt(hex.substring(0, 2), 16) || 0;
    let g = parseInt(hex.substring(2, 4), 16) || 0;
    let b = parseInt(hex.substring(4, 6), 16) || 0;
    let v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return '0x' + v.toString(16).toUpperCase().padStart(4, '0');
}

function showToast(msg) {
    let t = document.getElementById('toast');
    if (!t) {
        t = document.createElement('div');
        t.id = 'toast';
        t.className = 'toast';
        document.body.appendChild(t);
    }
    t.textContent = msg;
    t.classList.add('show');
    clearTimeout(t._timer);
    t._timer = setTimeout(() => t.classList.remove('show'), 2000);
}

function estimateSize() {
    // rough: header(42) + code estimate + asset sizes
    let codeLen = (document.getElementById('code-output').textContent || '').length;
    let assetBytes = State.assets.reduce((s, a) => s + (a.dataURL.length * 0.75), 0);
    let total = 42 + codeLen * 4 + assetBytes; // very rough ELF multiplier
    if (total < 1024) return total + ' B';
    return (total / 1024).toFixed(1) + ' KB';
}

// ═══════════════════════════════════════════════════════════════
// UI Controller
// ═══════════════════════════════════════════════════════════════
const UI = {
    init() {
        this.bindCanvas();
        this.bindSettings();
        this.bindToolbar();
        this.bindTabs();
        this.bindGlobalEvents();
        this.bindTopbarButtons();
        this.bindSaveLoad();
        this.renderSceneList();
        this.renderVarList();
        this.renderCanvas();
        this.updateCode();
        this.fitToView();

        // Auto-save to localStorage every 30s
        setInterval(() => {
            try {
                localStorage.setItem('bruce_designer_autosave', JSON.stringify(State));
            } catch(e) {}
        }, 30000);

        // Restore auto-save if present
        try {
            let saved = localStorage.getItem('bruce_designer_autosave');
            if (saved) {
                let parsed = JSON.parse(saved);
                if (parsed.settings && parsed.scenes) {
                    Object.assign(State, parsed);
                    this.syncSettingsUI();
                    this.fullRefresh();
                }
            }
        } catch(e) {}
    },

    fullRefresh() {
        this.syncSettingsUI();
        this.renderSceneList();
        this.renderVarList();
        this.renderCanvas();
        this.renderProperties();
        this.updateCode();
        this.applyViewport();
    },

    syncSettingsUI() {
        document.getElementById('app-id').value = State.settings.appId;
        document.getElementById('app-name').value = State.settings.appName;
        document.getElementById('app-author').value = State.settings.author;
        document.getElementById('target-board').value = State.settings.board || 'esp32';
        document.getElementById('canvas-dims').textContent = State.settings.width + ' × ' + State.settings.height;
        this.applyScreenSize();
    },

    // ── Infinite Canvas ──
    applyViewport() {
        let world = document.getElementById('canvas-world');
        if (world) {
            world.style.transform = `translate(${Viewport.panX}px, ${Viewport.panY}px) scale(${Viewport.zoom})`;
        }
        document.getElementById('zoom-level').textContent = Math.round(Viewport.zoom * 100) + '%';
        this.drawCanvasBg();
    },

    fitToView() {
        let container = document.getElementById('canvas-container');
        if (!container) return;
        let cRect = container.getBoundingClientRect();
        let W = State.settings.width;
        let H = State.settings.height;
        // Fit width and height with some padding
        let scaleX = (cRect.width - 80) / W;
        let scaleY = (cRect.height - 80) / H;
        Viewport.zoom = Math.max(Viewport.MIN_ZOOM, Math.min(Viewport.MAX_ZOOM, scaleX, scaleY, 2)); // cap at 200%
        Viewport.panX = Math.round((cRect.width - W * Viewport.zoom) / 2);
        Viewport.panY = Math.round((cRect.height - H * Viewport.zoom) / 2) - 10;
        this.applyViewport();
    },

    drawCanvasBg() {
        let container = document.getElementById('canvas-container');
        if (!container) return;
        let bg = document.getElementById('bg-canvas');
        if (!bg) {
            bg = document.createElement('canvas');
            bg.id = 'bg-canvas';
            bg.className = 'canvas-bg';
            container.insertBefore(bg, container.firstChild);
        }
        let rect = container.getBoundingClientRect();
        if (bg.width !== rect.width || bg.height !== rect.height) {
            bg.width = rect.width;
            bg.height = rect.height;
        }
        let ctx = bg.getContext('2d');
        ctx.clearRect(0, 0, bg.width, bg.height);
        
        let spacing = 20 * Viewport.zoom;
        if (spacing < 8) return; // don't draw if too dense
        
        ctx.fillStyle = 'rgba(255, 255, 255, 0.05)';
        let startX = Viewport.panX % spacing;
        let startY = Viewport.panY % spacing;
        
        for (let x = startX; x < bg.width; x += spacing) {
            for (let y = startY; y < bg.height; y += spacing) {
                ctx.beginPath();
                ctx.arc(x, y, 1, 0, Math.PI * 2);
                ctx.fill();
            }
        }
    },

    bindCanvas() {
        const self = this;
        let container = document.getElementById('canvas-container');
        
        // Window resize
        window.addEventListener('resize', () => this.drawCanvasBg());

        // Zoom controls
        document.getElementById('btn-zoom-in').addEventListener('click', () => {
            Viewport.zoom = Math.min(Viewport.MAX_ZOOM, Viewport.zoom * 1.25);
            self.applyViewport();
        });
        document.getElementById('btn-zoom-out').addEventListener('click', () => {
            Viewport.zoom = Math.max(Viewport.MIN_ZOOM, Viewport.zoom / 1.25);
            self.applyViewport();
        });
        document.getElementById('btn-zoom-fit').addEventListener('click', () => this.fitToView());

        // Wheel to pan/zoom
        container.addEventListener('wheel', (e) => {
            e.preventDefault();
            if (e.ctrlKey || e.metaKey) {
                // Zoom around mouse pointer
                let cRect = container.getBoundingClientRect();
                let mouseX = e.clientX - cRect.left;
                let mouseY = e.clientY - cRect.top;
                
                let zoomFactor = e.deltaY > 0 ? 0.9 : 1.1;
                let newZoom = Math.max(Viewport.MIN_ZOOM, Math.min(Viewport.MAX_ZOOM, Viewport.zoom * zoomFactor));
                
                let scaleChange = newZoom / Viewport.zoom;
                Viewport.panX = mouseX - (mouseX - Viewport.panX) * scaleChange;
                Viewport.panY = mouseY - (mouseY - Viewport.panY) * scaleChange;
                Viewport.zoom = newZoom;
            } else {
                // Pan
                Viewport.panX -= e.deltaX;
                Viewport.panY -= e.deltaY;
            }
            self.applyViewport();
        }, { passive: false });

        // Middle mouse or space+drag to pan
        let panStartX, panStartY, startPanX, startPanY;
        
        const onPanMove = (e) => {
            if (!Viewport.isPanning) return;
            Viewport.panX = startPanX + (e.clientX - panStartX);
            Viewport.panY = startPanY + (e.clientY - panStartY);
            self.applyViewport();
        };
        
        const onPanEnd = () => {
            Viewport.isPanning = false;
            container.classList.remove('panning');
            document.removeEventListener('mousemove', onPanMove);
            document.removeEventListener('mouseup', onPanEnd);
        };

        container.addEventListener('mousedown', (e) => {
            if (e.button === 1 || (e.button === 0 && Viewport.spaceHeld)) {
                e.preventDefault();
                Viewport.isPanning = true;
                container.classList.add('panning');
                panStartX = e.clientX;
                panStartY = e.clientY;
                startPanX = Viewport.panX;
                startPanY = Viewport.panY;
                document.addEventListener('mousemove', onPanMove);
                document.addEventListener('mouseup', onPanEnd);
            }
        });
    },

    // ── Settings ──
    bindSettings() {
        const self = this;
        document.getElementById('app-id').addEventListener('input', function() { pushUndo(); State.settings.appId = this.value; self.updateCode(); });
        document.getElementById('app-name').addEventListener('input', function() { pushUndo(); State.settings.appName = this.value; self.updateCode(); });
        document.getElementById('app-author').addEventListener('input', function() { pushUndo(); State.settings.author = this.value; self.updateCode(); });

        document.getElementById('target-board').addEventListener('change', function() {
            pushUndo();
            State.settings.board = this.value;
            // Auto-select a reasonable screen preset
            const boardDefaults = {
                'esp32': '240x135', 'esp32s3': '320x240', 'esp32s2': '240x135',
                'esp32c3': '240x135', 'esp32c6': '240x135', 'custom': null
            };
            let preset = boardDefaults[this.value];
            if (preset) {
                document.getElementById('screen-size').value = preset;
                let [w, h] = preset.split('x');
                State.settings.width = parseInt(w);
                State.settings.height = parseInt(h);
                self.applyScreenSize();
            }
            self.updateCode();
        });

        document.getElementById('screen-size').addEventListener('change', function() {
            pushUndo();
            if (this.value === 'custom') {
                document.getElementById('custom-size-group').classList.remove('hidden');
            } else {
                document.getElementById('custom-size-group').classList.add('hidden');
                let [w, h] = this.value.split('x');
                State.settings.width = parseInt(w);
                State.settings.height = parseInt(h);
                self.applyScreenSize();
            }
            self.updateCode();
        });

        document.getElementById('btn-apply-custom-size').addEventListener('click', function() {
            pushUndo();
            State.settings.width = parseInt(document.getElementById('custom-width').value) || 240;
            State.settings.height = parseInt(document.getElementById('custom-height').value) || 135;
            self.applyScreenSize();
            self.updateCode();
        });

        document.getElementById('btn-add-scene').addEventListener('click', function() {
            pushUndo();
            let name = prompt('Scene name:', 'Scene ' + (State.scenes.length + 1));
            if (!name) return;
            let lastScene = State.scenes[State.scenes.length - 1];
            let newX = lastScene ? lastScene.x + State.settings.width + 100 : 0;
            let newScene = { id: uid('scene'), name, elements: [], x: newX, y: 0 };
            State.scenes.push(newScene);
            State.activeSceneId = newScene.id;
            self.renderSceneList();
            self.renderCanvas();
            self.updateCode();
        });

        document.getElementById('btn-add-var').addEventListener('click', function() {
            let name = prompt('Variable name (e.g. counter):');
            if (!name) return;
            pushUndo();
            State.variables.push({ id: uid('var'), type: 'int', name, value: '0' });
            self.renderVarList();
            self.updateCode();
        });

        // Layer buttons
        document.getElementById('btn-layer-up').addEventListener('click', () => this.moveLayer(1));
        document.getElementById('btn-layer-down').addEventListener('click', () => this.moveLayer(-1));
    },

    applyScreenSize() {
        let boardName = document.getElementById('target-board');
        let boardLabel = boardName ? boardName.options[boardName.selectedIndex].text : State.settings.board;
        const labelText = boardLabel + ' — ' + State.settings.width + '×' + State.settings.height;
        document.getElementById('canvas-dims').textContent = State.settings.width + ' × ' + State.settings.height;
        
        document.querySelectorAll('.device-frame').forEach(frame => {
            let screen = frame.querySelector('.screen');
            if (screen) {
                screen.style.width = State.settings.width + 'px';
                screen.style.height = State.settings.height + 'px';
            }
            let label = frame.querySelector('.frame-label');
            if (label) label.textContent = frame.dataset.sceneName + ' (' + labelText + ')';
        });
        this.renderCanvas();
    },

    moveLayer(dir) {
        let scene = getActiveScene();
        if (!scene || !State.selectedElementId) return;
        let idx = scene.elements.findIndex(e => e.id === State.selectedElementId);
        if (idx < 0) return;
        let newIdx = idx + dir;
        if (newIdx < 0 || newIdx >= scene.elements.length) return;
        pushUndo();
        [scene.elements[idx], scene.elements[newIdx]] = [scene.elements[newIdx], scene.elements[idx]];
        this.renderCanvas();
        this.renderElementList();
        this.updateCode();
    },

    // ── Toolbar ──
    bindToolbar() {
        const self = this;
        document.querySelectorAll('.toolbar button').forEach(btn => {
            btn.addEventListener('click', function() {
                let tool = this.dataset.tool;
                if (!tool) return;
                if (tool === 'image') {
                    showToast('Drag & drop a PNG/BMP file onto the canvas');
                    return;
                }
                pushUndo();
                self.addElement(tool);
            });
        });

        // Drag & drop images onto canvas
        const world = document.getElementById('canvas-world');
        world.addEventListener('dragover', e => e.preventDefault());
        world.addEventListener('drop', (e) => {
            e.preventDefault();
            
            // Find which scene we dropped on
            let targetFrame = e.target.closest('.device-frame');
            if (targetFrame && targetFrame.dataset.sceneId) {
                State.activeSceneId = targetFrame.dataset.sceneId;
            }

            if (!e.dataTransfer.files || !e.dataTransfer.files[0]) return;
            let file = e.dataTransfer.files[0];
            if (!file.type.match(/image\/(png|bmp|jpeg|jpg)/)) {
                showToast('Unsupported file type. Use PNG or BMP.');
                return;
            }
            let reader = new FileReader();
            reader.onload = (event) => {
                let img = new Image();
                img.onload = () => {
                    pushUndo();
                    let assetId = uid('img');
                    let safeName = file.name.split('.')[0].replace(/[^a-zA-Z0-9_]/g, '_');
                    State.assets.push({ id: assetId, name: safeName, dataURL: event.target.result, w: img.width, h: img.height });
                    
                    // If we dropped inside a specific scene, try to place it at cursor coords relative to scene
                    let dropX = 0, dropY = 0;
                    if (targetFrame) {
                        let rect = targetFrame.querySelector('.screen').getBoundingClientRect();
                        dropX = Math.round((e.clientX - rect.left) / Viewport.zoom);
                        dropY = Math.round((e.clientY - rect.top) / Viewport.zoom);
                    }
                    
                    self.addElement('image', { 
                        assetId, 
                        x: dropX, 
                        y: dropY, 
                        w: Math.min(img.width, State.settings.width), 
                        h: Math.min(img.height, State.settings.height) 
                    });
                    showToast('Image added: ' + safeName);
                };
                img.src = event.target.result;
            };
            reader.readAsDataURL(file);
        });

        // Track cursor position on canvas relative to active scene
        world.addEventListener('mousemove', (e) => {
            let targetScreen = e.target.closest('.screen');
            if (targetScreen) {
                let rect = targetScreen.getBoundingClientRect();
                let x = Math.round((e.clientX - rect.left) / Viewport.zoom);
                let y = Math.round((e.clientY - rect.top) / Viewport.zoom);
                document.getElementById('cursor-pos').textContent = `x: ${x}  y: ${y}`;
            }
        });

        // Align buttons
        document.querySelectorAll('.align-bar button').forEach(btn => {
            btn.addEventListener('click', function() {
                let dir = this.dataset.align;
                if (dir) self.alignElement(dir);
            });
        });
    },

    // ── Align selected element to screen ──
    alignElement(dir) {
        let el = getElement(State.selectedElementId);
        if (!el) { showToast('Select an element first'); return; }
        pushUndo();
        let W = State.settings.width;
        let H = State.settings.height;
        let elW = el.w || 0;
        let elH = el.h || 0;

        switch (dir) {
            case 'left':        el.x = 0; break;
            case 'center-h':    el.x = Math.round((W - elW) / 2); break;
            case 'right':       el.x = W - elW; break;
            case 'top':         el.y = 0; break;
            case 'center-v':    el.y = Math.round((H - elH) / 2); break;
            case 'bottom':      el.y = H - elH; break;
            case 'center-both':
                el.x = Math.round((W - elW) / 2);
                el.y = Math.round((H - elH) / 2);
                break;
        }
        this.renderCanvas();
        this.renderProperties();
        this.updateCode();
        showToast('Aligned: ' + dir);
    },

    // ── Tabs ──
    bindTabs() {
        document.querySelectorAll('.tab').forEach(tab => {
            tab.addEventListener('click', function() {
                document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
                document.querySelectorAll('.tab-content').forEach(c => c.classList.add('hidden'));
                this.classList.add('active');
                document.getElementById(this.dataset.target).classList.remove('hidden');
            });
        });
    },

    // ── Top bar actions ──
    bindTopbarButtons() {
        document.getElementById('btn-undo').addEventListener('click', doUndo);
        document.getElementById('btn-redo').addEventListener('click', doRedo);

        document.getElementById('btn-duplicate').addEventListener('click', () => {
            let el = getElement(State.selectedElementId);
            if (!el) return;
            pushUndo();
            let copy = JSON.parse(JSON.stringify(el));
            copy.id = uid('el');
            copy.x += 10;
            copy.y += 10;
            getActiveScene().elements.push(copy);
            State.selectedElementId = copy.id;
            this.renderCanvas();
            this.renderProperties();
            this.updateCode();
            showToast('Element duplicated');
        });

        document.getElementById('btn-delete').addEventListener('click', () => this.deleteSelected());

        document.getElementById('btn-copy-code').addEventListener('click', () => {
            let code = document.getElementById('code-output').textContent;
            navigator.clipboard.writeText(code).then(() => showToast('Code copied to clipboard'));
        });

        document.getElementById('btn-export').addEventListener('click', exportZip);
    },

    // ── Global keyboard shortcuts ──
    bindGlobalEvents() {
        const self = this;
        
        document.addEventListener('keyup', (e) => {
            if (e.code === 'Space') {
                Viewport.spaceHeld = false;
                document.getElementById('canvas-container').classList.remove('space-held');
            }
        });

        document.addEventListener('keydown', (e) => {
            if (e.code === 'Space' && e.target.tagName !== 'INPUT' && e.target.tagName !== 'TEXTAREA' && e.target.tagName !== 'SELECT') {
                e.preventDefault();
                if (!Viewport.spaceHeld) {
                    Viewport.spaceHeld = true;
                    document.getElementById('canvas-container').classList.add('space-held');
                }
                return;
            }

            // Don't capture when typing in inputs
            if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT' || e.target.tagName === 'TEXTAREA') return;

            if (e.key === 'Delete' || e.key === 'Backspace') {
                self.deleteSelected();
            }
            if (e.ctrlKey && e.key === 'z') { e.preventDefault(); doUndo(); }
            if (e.ctrlKey && e.key === 'y') { e.preventDefault(); doRedo(); }
            if (e.ctrlKey && e.key === 'd') {
                e.preventDefault();
                document.getElementById('btn-duplicate').click();
            }

            // Arrow keys to nudge
            if (['ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].includes(e.key)) {
                let el = getElement(State.selectedElementId);
                if (!el) return;
                e.preventDefault();
                pushUndo();
                let step = e.shiftKey ? 10 : 1;
                if (e.key === 'ArrowUp') el.y -= step;
                if (e.key === 'ArrowDown') el.y += step;
                if (e.key === 'ArrowLeft') el.x -= step;
                if (e.key === 'ArrowRight') el.x += step;
                self.renderCanvas();
                self.renderProperties();
                self.updateCode();
            }
        });

        // Click on canvas background to deselect is handled in renderCanvas per device-frame
    },

    deleteSelected() {
        if (!State.selectedElementId) return;
        pushUndo();
        let scene = getActiveScene();
        scene.elements = scene.elements.filter(el => el.id !== State.selectedElementId);
        State.selectedElementId = null;
        this.renderCanvas();
        this.renderProperties();
        this.updateCode();
    },

    // ── Save / Load ──
    bindSaveLoad() {
        document.getElementById('btn-save').addEventListener('click', () => {
            let data = JSON.stringify(State, null, 2);
            let blob = new Blob([data], { type: 'application/json' });
            let a = document.createElement('a');
            a.href = URL.createObjectURL(blob);
            a.download = State.settings.appId + '.bruceproj';
            a.click();
            showToast('Project saved');
        });

        document.getElementById('btn-load').addEventListener('click', () => {
            document.getElementById('file-load').click();
        });

        document.getElementById('file-load').addEventListener('change', (e) => {
            let file = e.target.files[0];
            if (!file) return;
            let reader = new FileReader();
            reader.onload = (event) => {
                try {
                    let data = JSON.parse(event.target.result);
                    pushUndo();
                    Object.assign(State, data);
                    this.fullRefresh();
                    showToast('Project loaded: ' + file.name);
                } catch(err) {
                    showToast('Error loading project file');
                }
            };
            reader.readAsText(file);
            e.target.value = '';
        });
    },

    // ═══════════════════════════════════════════════════════════
    // Add Element
    // ═══════════════════════════════════════════════════════════
    addElement(type, extra = {}) {
        let scene = getActiveScene();
        let W = State.settings.width;
        let H = State.settings.height;
        let defaults = {
            text:      { w: 50, h: 12, text: 'Label',  color: '#ffffff' },
            rect:      { w: 60, h: 40, text: '',       color: '#ffffff' },
            fill_rect: { w: 60, h: 40, text: '',       color: '#ff6b00' },
            border:    { w: W,  h: H,  text: 'Title',  color: '#ffffff' },
            line:      { w: 80, h: 1,  text: '',       color: '#ffffff' },
            circle:    { w: 30, h: 30, text: '',       color: '#ffffff' },
            progress:  { w: 100,h: 10, text: '',       color: '#22c55e' },
            button:    { w: 70, h: 20, text: 'Button', color: '#ffffff' },
            image:     { w: 32, h: 32, text: '',       color: '#ffffff' }
        };
        let d = defaults[type] || defaults.rect;
        let el = {
            id: uid('el'),
            type,
            x: 10, y: 10,
            w: d.w, h: d.h,
            text: d.text,
            color: d.color,
            varBind: '',
            action: 'none',
            actionKey: 'check_select_press',
            actionTarget: '',
            progressValue: 75,
            ...extra
        };
        scene.elements.push(el);
        State.selectedElementId = el.id;
        this.renderCanvas();
        this.renderProperties();
        this.updateCode();
    },

    // ═══════════════════════════════════════════════════════════
    // Render: Scene List
    // ═══════════════════════════════════════════════════════════
    renderSceneList() {
        const list = document.getElementById('scene-list');
        list.innerHTML = '';
        const self = this;
        State.scenes.forEach((s, idx) => {
            let li = document.createElement('li');
            let nameSpan = document.createElement('span');
            nameSpan.textContent = s.name;
            li.appendChild(nameSpan);
            if (s.id === State.activeSceneId) li.classList.add('active');

            li.addEventListener('click', () => {
                State.activeSceneId = s.id;
                State.selectedElementId = null;
                self.renderSceneList();
                self.renderCanvas();
                self.renderProperties();
            });

            // Double-click to rename
            nameSpan.addEventListener('dblclick', (e) => {
                e.stopPropagation();
                let newName = prompt('Rename scene:', s.name);
                if (newName) {
                    pushUndo();
                    s.name = newName;
                    self.renderSceneList();
                    self.updateCode();
                }
            });

            if (idx > 0) {
                let del = document.createElement('span');
                del.textContent = '×';
                del.className = 'delete-btn';
                del.onclick = (e) => {
                    e.stopPropagation();
                    pushUndo();
                    State.scenes = State.scenes.filter(sc => sc.id !== s.id);
                    if (State.activeSceneId === s.id) State.activeSceneId = State.scenes[0].id;
                    self.renderSceneList();
                    self.renderCanvas();
                    self.updateCode();
                };
                li.appendChild(del);
            }
            list.appendChild(li);
        });
    },

    // ═══════════════════════════════════════════════════════════
    // Render: Variable List
    // ═══════════════════════════════════════════════════════════
    renderVarList() {
        const list = document.getElementById('var-list');
        list.innerHTML = '';
        const self = this;
        State.variables.forEach(v => {
            let li = document.createElement('li');
            li.textContent = `${v.type} ${v.name} = ${v.value}`;
            let del = document.createElement('span');
            del.textContent = '×';
            del.className = 'delete-btn';
            del.onclick = () => {
                pushUndo();
                State.variables = State.variables.filter(x => x.id !== v.id);
                self.renderVarList();
                self.updateCode();
            };
            li.appendChild(del);
            list.appendChild(li);
        });
    },

    // ═══════════════════════════════════════════════════════════
    // Render: Element List (layer panel)
    // ═══════════════════════════════════════════════════════════
    renderElementList() {
        const elList = document.getElementById('element-list');
        elList.innerHTML = '';
        let scene = getActiveScene();
        if (!scene) return;
        const self = this;
        scene.elements.forEach(el => {
            let li = document.createElement('li');
            let label = el.text ? `${el.type} "${el.text}"` : `${el.type} (${el.x},${el.y})`;
            li.textContent = label;
            if (el.id === State.selectedElementId) li.classList.add('active');
            li.onclick = () => {
                State.selectedElementId = el.id;
                self.renderCanvas();
                self.renderProperties();
            };
            elList.appendChild(li);
        });
    },

    // ═══════════════════════════════════════════════════════════
    // Render: Canvas
    // ═══════════════════════════════════════════════════════════
    renderCanvas() {
        const world = document.getElementById('canvas-world');
        // Clear existing frames
        world.querySelectorAll('.device-frame').forEach(el => el.remove());
        
        this.renderElementList();
        const self = this;

        let boardName = document.getElementById('target-board');
        let boardLabel = boardName ? boardName.options[boardName.selectedIndex].text : State.settings.board;
        const labelText = boardLabel + ' — ' + State.settings.width + '×' + State.settings.height;

        State.scenes.forEach(scene => {
            let frame = document.createElement('div');
            frame.className = 'device-frame';
            if (scene.id === State.activeSceneId) frame.classList.add('active');
            frame.style.left = (scene.x || 0) + 'px';
            frame.style.top = (scene.y || 0) + 'px';
            frame.dataset.sceneId = scene.id;
            frame.dataset.sceneName = scene.name;

            let label = document.createElement('div');
            label.className = 'frame-label';
            label.textContent = scene.name + ' (' + labelText + ')';
            frame.appendChild(label);

            let screen = document.createElement('div');
            screen.className = 'screen';
            screen.style.width = State.settings.width + 'px';
            screen.style.height = State.settings.height + 'px';
            frame.appendChild(screen);

            // Drag scene logic
            frame.addEventListener('mousedown', (e) => {
                if (e.target.classList.contains('ui-element') || e.target.classList.contains('resize-handle')) return;
                
                // Select scene
                if (State.activeSceneId !== scene.id) {
                    State.activeSceneId = scene.id;
                    State.selectedElementId = null;
                    self.renderSceneList();
                    self.renderCanvas();
                    self.renderProperties();
                    return;
                }

                // Deselect element if clicked empty space
                if (State.selectedElementId) {
                    State.selectedElementId = null;
                    self.renderCanvas();
                    self.renderProperties();
                }

                if (e.button !== 0 || Viewport.spaceHeld) return; // let canvas pan handle middle click / space
                e.stopPropagation();

                let isDragging = false;
                let startX = e.clientX, startY = e.clientY;
                let origX = scene.x || 0, origY = scene.y || 0;

                const onMove = (ev) => {
                    isDragging = true;
                    scene.x = Math.round(origX + (ev.clientX - startX) / Viewport.zoom);
                    scene.y = Math.round(origY + (ev.clientY - startY) / Viewport.zoom);
                    frame.style.left = scene.x + 'px';
                    frame.style.top = scene.y + 'px';
                };
                const onUp = () => {
                    if (isDragging) {
                        pushUndo();
                        self.updateCode();
                    }
                    document.removeEventListener('mousemove', onMove);
                    document.removeEventListener('mouseup', onUp);
                };
                document.addEventListener('mousemove', onMove);
                document.addEventListener('mouseup', onUp);
            });

            // Render elements
            scene.elements.forEach(el => {
                self.renderElement(el, screen, scene.id);
            });

            world.appendChild(frame);
        });
    },

    renderElement(el, screen, sceneId) {
        const self = this;
        let div = document.createElement('div');
        div.className = 'ui-element';
        if (el.id === State.selectedElementId) div.classList.add('selected');

        div.style.left = el.x + 'px';
        div.style.top = el.y + 'px';

        let needsSize = (el.type !== 'text');
        if (needsSize) {
            div.style.width = el.w + 'px';
            div.style.height = el.h + 'px';
        }

        // Render type-specific appearance
        switch (el.type) {
            case 'text':
                div.textContent = el.varBind ? `{${el.varBind}}` : el.text;
                div.style.color = el.color;
                div.style.fontSize = '10px';
                div.style.whiteSpace = 'nowrap';
                break;
            case 'rect':
                div.style.border = `1px solid ${el.color}`;
                break;
            case 'fill_rect':
                div.style.backgroundColor = el.color;
                div.style.opacity = '0.9';
                break;
            case 'border':
                div.style.border = `1px solid ${el.color}`;
                let titleBar = document.createElement('div');
                titleBar.textContent = el.text;
                titleBar.style.cssText = `position:absolute;top:0;left:0;right:0;height:12px;background:${el.color};color:#000;font-size:9px;padding:1px 4px;`;
                div.appendChild(titleBar);
                break;
            case 'line':
                div.style.borderTop = `1px solid ${el.color}`;
                div.style.height = '1px';
                break;
            case 'circle':
                div.style.border = `1px solid ${el.color}`;
                div.style.borderRadius = '50%';
                break;
            case 'progress':
                div.style.border = `1px solid ${el.color}`;
                let fill = document.createElement('div');
                fill.style.cssText = `width:${el.progressValue || 75}%;height:100%;background:${el.color};`;
                div.appendChild(fill);
                break;
            case 'button':
                div.style.border = `1px solid ${el.color}`;
                div.style.color = el.color;
                div.style.display = 'flex';
                div.style.alignItems = 'center';
                div.style.justifyContent = 'center';
                div.style.fontSize = '9px';
                div.textContent = el.text;
                break;
            case 'image':
                let asset = State.assets.find(a => a.id === el.assetId);
                if (asset) {
                    let img = document.createElement('img');
                    img.src = asset.dataURL;
                    img.style.cssText = 'width:100%;height:100%;pointer-events:none;image-rendering:pixelated;';
                    div.appendChild(img);
                } else {
                    div.style.backgroundColor = '#333';
                    div.style.display = 'flex';
                    div.style.alignItems = 'center';
                    div.style.justifyContent = 'center';
                    div.style.fontSize = '8px';
                    div.style.color = '#888';
                    div.textContent = 'IMG';
                }
                break;
        }

        // ── Drag ──
        let isDragging = false, dragStartX, dragStartY, origX, origY;
        div.addEventListener('mousedown', (e) => {
            if (e.target.classList.contains('resize-handle')) return;
            e.stopPropagation();
            if (State.selectedElementId !== el.id || State.activeSceneId !== sceneId) {
                State.selectedElementId = el.id;
                State.activeSceneId = sceneId; // select scene when element is clicked
                self.renderSceneList();
                self.renderCanvas();
                self.renderProperties();
                return;
            }
            isDragging = true;
            dragStartX = e.clientX; dragStartY = e.clientY;
            origX = el.x; origY = el.y;

            const onMove = (ev) => {
                if (!isDragging) return;
                el.x = Math.round((origX + (ev.clientX - dragStartX) / Viewport.zoom) / 1) * 1;
                el.y = Math.round((origY + (ev.clientY - dragStartY) / Viewport.zoom) / 1) * 1;
                div.style.left = el.x + 'px';
                div.style.top = el.y + 'px';
            };
            const onUp = (ev) => {
                if (isDragging) {
                    isDragging = false;
                    
                    // Check if dropped into a different scene
                    // Temporarily hide the dragged div so we can detect what's underneath
                    div.style.visibility = 'hidden';
                    let targetUnder = document.elementFromPoint(ev.clientX, ev.clientY);
                    div.style.visibility = 'visible';
                    
                    let targetFrame = targetUnder ? targetUnder.closest('.device-frame') : null;
                    let targetSceneId = targetFrame ? targetFrame.dataset.sceneId : null;
                    
                    let changed = false;
                    if (targetSceneId && targetSceneId !== sceneId) {
                        // Move to new scene
                        let oldScene = State.scenes.find(s => s.id === sceneId);
                        let newScene = State.scenes.find(s => s.id === targetSceneId);
                        if (oldScene && newScene) {
                            // Calculate new relative coordinates
                            let oldWorldX = (oldScene.x || 0) + el.x;
                            let oldWorldY = (oldScene.y || 0) + el.y;
                            
                            el.x = oldWorldX - (newScene.x || 0);
                            el.y = oldWorldY - (newScene.y || 0);
                            
                            oldScene.elements = oldScene.elements.filter(e => e.id !== el.id);
                            newScene.elements.push(el);
                            State.activeSceneId = newScene.id;
                            changed = true;
                        }
                    } else if (el.x !== origX || el.y !== origY) {
                        changed = true;
                    }
                    
                    if (changed) pushUndo();
                    self.renderElementList();
                    self.renderProperties();
                    self.updateCode();
                    
                    // Always re-render canvas if it moved scenes to correct the DOM parent
                    if (targetSceneId && targetSceneId !== sceneId) {
                        self.renderCanvas();
                    }
                }
                document.removeEventListener('mousemove', onMove);
                document.removeEventListener('mouseup', onUp);
            };
            document.addEventListener('mousemove', onMove);
            document.addEventListener('mouseup', onUp);
        });

        // ── Resize handles ──
        if (needsSize) {
            ['se', 'e', 's'].forEach(pos => {
                let h = document.createElement('div');
                h.className = 'resize-handle handle-' + pos;

                h.addEventListener('mousedown', (e) => {
                    e.stopPropagation();
                    let rsx = e.clientX, rsy = e.clientY, ow = el.w, oh = el.h;

                    const onMove = (ev) => {
                        let dx = (ev.clientX - rsx) / Viewport.zoom;
                        let dy = (ev.clientY - rsy) / Viewport.zoom;
                        if (pos === 'se' || pos === 'e') el.w = Math.max(5, Math.round(ow + dx));
                        if (pos === 'se' || pos === 's') el.h = Math.max(5, Math.round(oh + dy));
                        div.style.width = el.w + 'px';
                        div.style.height = el.h + 'px';
                    };
                    const onUp = () => {
                        if (el.w !== ow || el.h !== oh) pushUndo();
                        self.renderProperties();
                        self.updateCode();
                        document.removeEventListener('mousemove', onMove);
                        document.removeEventListener('mouseup', onUp);
                    };
                    document.addEventListener('mousemove', onMove);
                    document.addEventListener('mouseup', onUp);
                });
                div.appendChild(h);
            });
        }

        screen.appendChild(div);
    },

    // ═══════════════════════════════════════════════════════════
    // Render: Properties Panel
    // ═══════════════════════════════════════════════════════════
    renderProperties() {
        const panel = document.getElementById('properties-panel');
        let el = getElement(State.selectedElementId);

        if (!el) {
            panel.innerHTML = '<p class="muted">Select an element on the canvas</p>';
            return;
        }

        const self = this;
        let html = `<label style="margin-top:0;font-weight:600;color:var(--accent)">${el.type.toUpperCase()}</label>`;

        // Position
        html += `<div class="prop-group">
            <div><label>X:</label><input type="number" id="prop-x" value="${el.x}"></div>
            <div><label>Y:</label><input type="number" id="prop-y" value="${el.y}"></div>
        </div>`;

        // Size (for non-text elements)
        if (el.type !== 'text') {
            html += `<div class="prop-group">
                <div><label>W:</label><input type="number" id="prop-w" value="${el.w}"></div>
                <div><label>H:</label><input type="number" id="prop-h" value="${el.h}"></div>
            </div>`;
        }

        // Text
        if (['text', 'button', 'border'].includes(el.type)) {
            html += `<label>Text:</label><input type="text" id="prop-text" value="${el.text}">`;
            if (el.type === 'text' && State.variables.length > 0) {
                html += `<label>Bind to Variable:</label>
                    <select id="prop-varBind">
                        <option value="">None (static text)</option>
                        ${State.variables.map(v => `<option value="${v.name}" ${el.varBind===v.name?'selected':''}>${v.name} (${v.type})</option>`).join('')}
                    </select>`;
            }
        }

        // Progress value
        if (el.type === 'progress') {
            html += `<label>Default Value (%):</label><input type="number" id="prop-progress" min="0" max="100" value="${el.progressValue || 75}">`;
        }

        // Color
        if (el.type !== 'image') {
            html += `<label>Color:</label>
                <div class="color-picker-wrap">
                    <input type="color" id="prop-color" value="${el.color}">
                    <span class="hex-display">${hexToRgb565(el.color)}</span>
                </div>`;
        }

        // Button-specific
        if (el.type === 'button') {
            html += `<hr class="prop-divider">
                <label>Hardware Key:</label>
                <select id="prop-key">
                    <option value="check_select_press" ${el.actionKey==='check_select_press'?'selected':''}>SELECT (OK)</option>
                    <option value="check_next_press" ${el.actionKey==='check_next_press'?'selected':''}>NEXT (→/↓)</option>
                    <option value="check_prev_press" ${el.actionKey==='check_prev_press'?'selected':''}>PREV (←/↑)</option>
                    <option value="check_escape_press" ${el.actionKey==='check_escape_press'?'selected':''}>ESCAPE (Back)</option>
                </select>
                <label>On Press:</label>
                <select id="prop-action">
                    <option value="none" ${el.action==='none'?'selected':''}>No action</option>
                    <option value="call" ${el.action==='call'?'selected':''}>Call function</option>
                    <option value="scene" ${el.action==='scene'?'selected':''}>Switch scene</option>
                </select>`;
            if (el.action === 'call') {
                html += `<label>Function Name:</label><input type="text" id="prop-target" value="${el.actionTarget}" placeholder="my_function">`;
            } else if (el.action === 'scene') {
                html += `<label>Target Scene:</label>
                    <select id="prop-target-scene">
                        ${State.scenes.map(s => `<option value="${s.name}" ${el.actionTarget===s.name?'selected':''}>${s.name}</option>`).join('')}
                    </select>`;
            }
        }

        panel.innerHTML = html;

        // ── Bind property events ──
        ['x','y','w','h'].forEach(k => {
            let inp = document.getElementById('prop-' + k);
            if (inp) inp.addEventListener('change', () => { pushUndo(); el[k] = parseInt(inp.value); self.renderCanvas(); self.updateCode(); });
        });

        let ti = document.getElementById('prop-text');
        if (ti) ti.addEventListener('input', () => { pushUndo(); el.text = ti.value; self.renderCanvas(); self.updateCode(); });

        let ci = document.getElementById('prop-color');
        if (ci) ci.addEventListener('input', () => {
            pushUndo();
            el.color = ci.value;
            panel.querySelector('.hex-display').textContent = hexToRgb565(el.color);
            self.renderCanvas();
            self.updateCode();
        });

        let vb = document.getElementById('prop-varBind');
        if (vb) vb.addEventListener('change', () => { pushUndo(); el.varBind = vb.value; self.renderCanvas(); self.updateCode(); });

        let pi = document.getElementById('prop-progress');
        if (pi) pi.addEventListener('change', () => { pushUndo(); el.progressValue = parseInt(pi.value); self.renderCanvas(); self.updateCode(); });

        // Button events
        let ki = document.getElementById('prop-key');
        if (ki) ki.addEventListener('change', () => { pushUndo(); el.actionKey = ki.value; self.updateCode(); });

        let ai = document.getElementById('prop-action');
        if (ai) ai.addEventListener('change', () => {
            pushUndo();
            el.action = ai.value;
            el.actionTarget = '';
            self.renderProperties(); // re-render to show correct target input
            self.updateCode();
        });

        let pt = document.getElementById('prop-target');
        if (pt) pt.addEventListener('input', () => { pushUndo(); el.actionTarget = pt.value; self.updateCode(); });

        let pts = document.getElementById('prop-target-scene');
        if (pts) pts.addEventListener('change', () => { pushUndo(); el.actionTarget = pts.value; self.updateCode(); });
    },

    // ═══════════════════════════════════════════════════════════
    // C Code Generation
    // ═══════════════════════════════════════════════════════════
    updateCode() {
        // ── BAM Manifest ──
        let manifest = {
            appid: State.settings.appId,
            name: State.settings.appName,
            version: "1.0",
            author: State.settings.author,
            entry_point: "app_main",
            target_arch: State.settings.board
        };
        if (State.assets.length > 0) manifest.assets_dir = "assets";
        document.getElementById('manifest-output').textContent = JSON.stringify(manifest, null, 4);

        // ── C Code ──
        let c = '';
        c += `#include "bruce_api.h"\n`;
        if (State.assets.length > 0) c += `#include "bruce_assets.h"\n`;
        if (State.variables.length > 0) c += `#include <stdio.h>\n`;
        c += `\n`;

        // Screen dimensions as constants
        c += `#define SCREEN_WIDTH  ${State.settings.width}\n`;
        c += `#define SCREEN_HEIGHT ${State.settings.height}\n\n`;

        // Global variables
        if (State.variables.length > 0) {
            c += `/* ── Global State ── */\n`;
            State.variables.forEach(v => { c += `${v.type} ${v.name} = ${v.value};\n`; });
            c += `\n`;
        }

        // Scene enum
        let sceneNames = State.scenes.map(s => 'SCENE_' + s.name.toUpperCase().replace(/[^A-Z0-9]/g, '_'));
        if (State.scenes.length > 1) {
            c += `/* ── Scenes ── */\n`;
            c += `typedef enum {\n`;
            c += sceneNames.map((n, i) => `    ${n}${i < sceneNames.length - 1 ? ',' : ''}`).join('\n') + '\n';
            c += `} AppScene;\n\n`;
            c += `AppScene current_scene = ${sceneNames[0]};\n\n`;
        }

        // Custom function stubs
        let funcs = new Set();
        State.scenes.forEach(s => s.elements.forEach(el => {
            if (el.type === 'button' && el.action === 'call' && el.actionTarget) funcs.add(el.actionTarget);
        }));
        if (funcs.size > 0) {
            c += `/* ── Custom Functions ── */\n`;
            funcs.forEach(fn => {
                c += `void ${fn}(BruceAPI* api) {\n    // TODO: implement\n}\n\n`;
            });
        }

        // app_main
        c += `void app_main(BruceAPI* api) {\n`;
        c += `    if (!api) return;\n\n`;
        if (State.variables.some(v => State.scenes.some(s => s.elements.some(e => e.varBind === v.name)))) {
            c += `    char buf[64];\n\n`;
        }
        c += `    while (1) {\n`;
        c += `        api->clear_screen(0x0000);\n\n`;

        const genScene = (scene) => {
            let out = '';
            scene.elements.forEach(el => {
                let color = hexToRgb565(el.color);
                switch (el.type) {
                    case 'text':
                        if (el.varBind) {
                            out += `        snprintf(buf, sizeof(buf), "${el.text}: %d", ${el.varBind});\n`;
                            out += `        api->draw_string(buf, ${el.x}, ${el.y}, ${color});\n`;
                        } else {
                            out += `        api->draw_string("${el.text}", ${el.x}, ${el.y}, ${color});\n`;
                        }
                        break;
                    case 'rect':
                        out += `        api->draw_rect(${el.x}, ${el.y}, ${el.w}, ${el.h}, ${color});\n`;
                        break;
                    case 'fill_rect':
                        out += `        api->fill_rect(${el.x}, ${el.y}, ${el.w}, ${el.h}, ${color});\n`;
                        break;
                    case 'border':
                        out += `        api->draw_main_border_with_title("${el.text}");\n`;
                        break;
                    case 'line':
                        out += `        api->fill_rect(${el.x}, ${el.y}, ${el.w}, 1, ${color}); /* line */\n`;
                        break;
                    case 'circle':
                        out += `        api->draw_rect(${el.x}, ${el.y}, ${el.w}, ${el.h}, ${color}); /* circle approximation */\n`;
                        break;
                    case 'progress':
                        out += `        /* Progress bar */\n`;
                        out += `        api->draw_rect(${el.x}, ${el.y}, ${el.w}, ${el.h}, ${color});\n`;
                        out += `        api->fill_rect(${el.x + 1}, ${el.y + 1}, (${el.w - 2}) * ${el.progressValue || 75} / 100, ${el.h - 2}, ${color});\n`;
                        break;
                    case 'image':
                        let asset = State.assets.find(a => a.id === el.assetId);
                        if (asset) {
                            out += `        api->draw_image(${el.x}, ${el.y}, ${asset.name}_data, ${asset.name}_width, ${asset.name}_height);\n`;
                        }
                        break;
                    case 'button':
                        out += `        /* Button: "${el.text}" */\n`;
                        out += `        api->draw_rect(${el.x}, ${el.y}, ${el.w}, ${el.h}, ${color});\n`;
                        out += `        api->draw_string("${el.text}", ${el.x + 4}, ${el.y + 5}, ${color});\n`;
                        if (el.action !== 'none' && el.actionKey) {
                            out += `        if (api->${el.actionKey}()) {\n`;
                            if (el.action === 'call' && el.actionTarget) {
                                out += `            ${el.actionTarget}(api);\n`;
                            } else if (el.action === 'scene' && el.actionTarget) {
                                let sn = 'SCENE_' + el.actionTarget.toUpperCase().replace(/[^A-Z0-9]/g, '_');
                                out += `            current_scene = ${sn};\n`;
                            }
                            out += `        }\n`;
                        }
                        break;
                }
            });
            return out;
        };

        if (State.scenes.length > 1) {
            c += `        switch (current_scene) {\n`;
            State.scenes.forEach((scene, i) => {
                c += `            case ${sceneNames[i]}: {\n`;
                c += genScene(scene);
                c += `                break;\n`;
                c += `            }\n`;
            });
            c += `        }\n\n`;
        } else {
            c += genScene(State.scenes[0]);
            c += `\n`;
        }

        // ESC exit (if not already mapped)
        let escMapped = State.scenes.some(s => s.elements.some(e => e.type === 'button' && e.actionKey === 'check_escape_press'));
        if (!escMapped) {
            c += `        if (api->check_escape_press()) break;\n\n`;
        }

        c += `        api->bruce_delay(50);\n`;
        c += `    }\n`;
        c += `}\n`;

        let codeOut = document.getElementById('code-output');
        codeOut.textContent = c;

        // Syntax highlight
        if (window.Prism) {
            Prism.highlightElement(codeOut);
            Prism.highlightElement(document.getElementById('manifest-output'));
        }

        // Update size badge
        document.getElementById('size-badge').textContent = '~' + estimateSize();
    }
};

// ═══════════════════════════════════════════════════════════════
// Export to ZIP
// ═══════════════════════════════════════════════════════════════
function exportZip() {
    if (typeof JSZip === 'undefined') {
        showToast('JSZip not loaded. Check your internet connection.');
        return;
    }
    let zip = new JSZip();
    zip.file('application.bam', document.getElementById('manifest-output').textContent);
    zip.file('app.c', document.getElementById('code-output').textContent);

    if (State.assets.length > 0) {
        let folder = zip.folder('assets');
        State.assets.forEach(a => {
            let data = a.dataURL.split(',')[1];
            folder.file(a.name + '.png', data, { base64: true });
        });
    }

    zip.generateAsync({ type: 'blob' }).then(blob => {
        let a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = State.settings.appId + '_project.zip';
        a.click();
        showToast('Project exported as ZIP');
    });
}

// ═══════════════════════════════════════════════════════════════
// Boot
// ═══════════════════════════════════════════════════════════════
window.addEventListener('load', () => UI.init());
