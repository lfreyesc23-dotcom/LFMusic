// ⌨️ FRUTILLA STUDIO - Keyboard Shortcuts Manager (FL Studio Style)
// =====================================================================

export class KeyboardShortcutsManager {
  constructor() {
    this.shortcuts = new Map();
    this.isEnabled = true;
    this.overlayVisible = false;
    this.registeredCallbacks = new Map();
    
    this.initializeDefaultShortcuts();
    this.attachGlobalListeners();
    this.createOverlay();
  }

  initializeDefaultShortcuts() {
    // === TRANSPORT CONTROLS ===
    this.register('Space', 'Play/Pause', 'transport.playPause');
    this.register('Home', 'Go to Start', 'transport.goToStart');
    this.register('End', 'Go to End', 'transport.goToEnd');
    this.register('Numpad*', 'Record', 'transport.record');
    this.register('Numpad/', 'Stop', 'transport.stop');
    
    // === WINDOW SWITCHING (FL Studio Style) ===
    this.register('F5', 'Playlist View', 'window.playlist');
    this.register('F6', 'Step Sequencer / Channel Rack', 'window.channelRack');
    this.register('F7', 'Piano Roll', 'window.pianoRoll');
    this.register('F8', 'Plugin Picker', 'window.pluginPicker');
    this.register('F9', 'Mixer', 'window.mixer');
    this.register('F10', 'MIDI Settings', 'window.midiSettings');
    this.register('F11', 'Song Info', 'window.songInfo');
    this.register('F12', 'Close All Windows', 'window.closeAll');
    
    // === FILE OPERATIONS ===
    this.register('Ctrl+N', 'New Project', 'file.new');
    this.register('Ctrl+O', 'Open Project', 'file.open');
    this.register('Ctrl+S', 'Save Project', 'file.save');
    this.register('Ctrl+Shift+S', 'Save As', 'file.saveAs');
    this.register('Ctrl+R', 'Recent Projects', 'file.recent');
    
    // === EDIT OPERATIONS ===
    this.register('Ctrl+Z', 'Undo', 'edit.undo');
    this.register('Ctrl+Y', 'Redo', 'edit.redo');
    this.register('Ctrl+Shift+Z', 'Redo Alt', 'edit.redo');
    this.register('Ctrl+X', 'Cut', 'edit.cut');
    this.register('Ctrl+C', 'Copy', 'edit.copy');
    this.register('Ctrl+V', 'Paste', 'edit.paste');
    this.register('Delete', 'Delete', 'edit.delete');
    this.register('Ctrl+A', 'Select All', 'edit.selectAll');
    this.register('Ctrl+D', 'Duplicate', 'edit.duplicate');
    
    // === BROWSER & SEARCH ===
    this.register('Ctrl+B', 'Browser', 'browser.toggle');
    this.register('Ctrl+F', 'Find', 'search.find');
    this.register('Alt+F', 'File Browser', 'browser.files');
    
    // === TOOLS & MODES ===
    this.register('B', 'Brush Tool', 'tool.brush');
    this.register('P', 'Paint Tool', 'tool.paint');
    this.register('D', 'Delete Tool', 'tool.delete');
    this.register('Z', 'Zoom Tool', 'tool.zoom');
    this.register('S', 'Slice Tool', 'tool.slice');
    this.register('M', 'Mute Tool', 'tool.mute');
    
    // === QUANTIZATION ===
    this.register('Ctrl+Q', 'Quick Quantize', 'edit.quantize');
    this.register('Alt+Q', 'Quantize Settings', 'edit.quantizeSettings');
    
    // === PIANO ROLL SPECIFIC ===
    this.register('Shift+Up', 'Transpose Up Octave', 'pianoRoll.transposeUpOctave');
    this.register('Shift+Down', 'Transpose Down Octave', 'pianoRoll.transposeDownOctave');
    this.register('Ctrl+Up', 'Transpose Up Semitone', 'pianoRoll.transposeUpSemitone');
    this.register('Ctrl+Down', 'Transpose Down Semitone', 'pianoRoll.transposeDownSemitone');
    this.register('Alt+A', 'Arpeggiate', 'pianoRoll.arpeggiate');
    this.register('Alt+C', 'Chordize', 'pianoRoll.chordize');
    this.register('Alt+R', 'Randomize', 'pianoRoll.randomize');
    this.register('Alt+S', 'Strum', 'pianoRoll.strum');
    
    // === MIXER SPECIFIC ===
    this.register('Ctrl+L', 'Link to Controller', 'mixer.linkController');
    this.register('Alt+L', 'Load Mixer State', 'mixer.loadState');
    this.register('Alt+S', 'Save Mixer State', 'mixer.saveState');
    this.register('Ctrl+Shift+C', 'Copy Mixer Track', 'mixer.copyTrack');
    this.register('Ctrl+Shift+V', 'Paste Mixer Track', 'mixer.pasteTrack');
    
    // === ZOOM & NAVIGATION ===
    this.register('Ctrl+MouseWheel', 'Zoom Horizontal', 'zoom.horizontal');
    this.register('Shift+MouseWheel', 'Zoom Vertical', 'zoom.vertical');
    this.register('Ctrl+Alt+MouseWheel', 'Zoom Both', 'zoom.both');
    this.register('Ctrl+Shift+H', 'Zoom to All', 'zoom.all');
    this.register('Ctrl+Shift+S', 'Zoom to Selection', 'zoom.selection');
    
    // === PATTERN & CLIP OPERATIONS ===
    this.register('Ctrl+Shift+C', 'Clone Pattern', 'pattern.clone');
    this.register('Ctrl+Shift+M', 'Make Unique', 'pattern.makeUnique');
    this.register('F4', 'Next Pattern', 'pattern.next');
    this.register('Shift+F4', 'Previous Pattern', 'pattern.previous');
    
    // === METRONOME & RECORDING ===
    this.register('Ctrl+M', 'Toggle Metronome', 'metronome.toggle');
    this.register('Ctrl+Shift+M', 'Metronome Settings', 'metronome.settings');
    this.register('Ctrl+Alt+R', 'Recording Filter', 'record.filter');
    
    // === HELP & UI ===
    this.register('F1', 'Help', 'help.show');
    this.register('Ctrl+/', 'Show All Shortcuts', 'shortcuts.show');
    this.register('Alt+F4', 'Exit', 'app.exit');
    this.register('F11', 'Toggle Fullscreen', 'window.fullscreen');
  }

  register(keyCombo, description, action, callback = null) {
    const normalizedKey = this.normalizeKeyCombo(keyCombo);
    
    this.shortcuts.set(normalizedKey, {
      keyCombo,
      description,
      action,
      category: action.split('.')[0]
    });
    
    if (callback) {
      this.registeredCallbacks.set(action, callback);
    }
  }

  on(action, callback) {
    this.registeredCallbacks.set(action, callback);
  }

  off(action) {
    this.registeredCallbacks.delete(action);
  }

  normalizeKeyCombo(keyCombo) {
    // Normalize key combination to lowercase for comparison
    return keyCombo.toLowerCase()
      .replace(/\+/g, ' ')
      .split(' ')
      .sort()
      .join('+');
  }

  attachGlobalListeners() {
    document.addEventListener('keydown', this.handleKeyDown.bind(this));
    document.addEventListener('keyup', this.handleKeyUp.bind(this));
  }

  handleKeyDown(e) {
    if (!this.isEnabled) return;
    
    // Don't trigger shortcuts when typing in inputs
    if (e.target.matches('input, textarea, [contenteditable]')) {
      // Allow Ctrl+A, Ctrl+C, Ctrl+V, Ctrl+X in inputs
      const allowedInInputs = ['ctrl+a', 'ctrl+c', 'ctrl+v', 'ctrl+x', 'ctrl+z', 'ctrl+y'];
      const currentCombo = this.getKeyCombo(e);
      if (!allowedInInputs.includes(currentCombo)) {
        return;
      }
    }
    
    const keyCombo = this.getKeyCombo(e);
    const shortcut = this.shortcuts.get(keyCombo);
    
    if (shortcut) {
      e.preventDefault();
      e.stopPropagation();
      
      this.executeAction(shortcut.action, e);
      
      // Visual feedback
      this.showShortcutFeedback(shortcut);
    }
    
    // Show overlay on Ctrl+/
    if ((e.ctrlKey || e.metaKey) && e.key === '/') {
      e.preventDefault();
      this.toggleOverlay();
    }
  }

  handleKeyUp(e) {
    // Reserved for future use (e.g., tool modes)
  }

  getKeyCombo(e) {
    const keys = [];
    
    if (e.ctrlKey || e.metaKey) keys.push('ctrl');
    if (e.shiftKey) keys.push('shift');
    if (e.altKey) keys.push('alt');
    
    const key = e.key.toLowerCase();
    
    // Special key mappings
    if (key === ' ') keys.push('space');
    else if (key === 'delete') keys.push('delete');
    else if (key === 'backspace') keys.push('backspace');
    else if (key === 'enter') keys.push('enter');
    else if (key === 'escape') keys.push('escape');
    else if (key === 'arrowup') keys.push('up');
    else if (key === 'arrowdown') keys.push('down');
    else if (key === 'arrowleft') keys.push('left');
    else if (key === 'arrowright') keys.push('right');
    else if (key.startsWith('f') && !isNaN(key.slice(1))) keys.push(key); // F1-F12
    else if (!['control', 'shift', 'alt', 'meta'].includes(key)) keys.push(key);
    
    return keys.sort().join('+');
  }

  executeAction(action, event) {
    const callback = this.registeredCallbacks.get(action);
    
    if (callback) {
      callback(event);
    } else {
      // Dispatch custom event for unhandled actions
      document.dispatchEvent(new CustomEvent('frutilla:shortcut', {
        detail: { action, event }
      }));
    }
  }

  showShortcutFeedback(shortcut) {
    // Create temporary visual feedback
    const feedback = document.createElement('div');
    feedback.className = 'shortcut-feedback';
    feedback.textContent = shortcut.description;
    feedback.style.cssText = `
      position: fixed;
      top: 20px;
      right: 20px;
      background: var(--bg-elevated);
      color: var(--text-primary);
      padding: 8px 16px;
      border-radius: 8px;
      box-shadow: var(--shadow-lg);
      font-size: 12px;
      font-weight: 600;
      z-index: 10000;
      animation: slideInRight 200ms ease-out, fadeOut 300ms ease-in 1.5s forwards;
      pointer-events: none;
    `;
    
    document.body.appendChild(feedback);
    
    setTimeout(() => feedback.remove(), 2000);
  }

  createOverlay() {
    const overlay = document.createElement('div');
    overlay.id = 'keyboard-shortcuts-overlay';
    overlay.className = 'fl-modal-overlay';
    overlay.style.display = 'none';
    
    overlay.innerHTML = `
      <div class="fl-modal" style="max-width: 900px; max-height: 80vh;">
        <div class="fl-modal-header">
          <h2 class="fl-modal-title">⌨️ Keyboard Shortcuts</h2>
          <button class="fl-button fl-button-ghost" id="close-shortcuts">✕</button>
        </div>
        <div class="fl-modal-body" id="shortcuts-content" style="overflow-y: auto;">
          <!-- Content will be generated dynamically -->
        </div>
        <div class="fl-modal-footer">
          <button class="fl-button fl-button-secondary" id="close-shortcuts-footer">Close</button>
        </div>
      </div>
    `;
    
    document.body.appendChild(overlay);
    
    // Close handlers
    overlay.addEventListener('click', (e) => {
      if (e.target === overlay) this.hideOverlay();
    });
    
    overlay.querySelector('#close-shortcuts').addEventListener('click', () => this.hideOverlay());
    overlay.querySelector('#close-shortcuts-footer').addEventListener('click', () => this.hideOverlay());
    
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape' && this.overlayVisible) {
        this.hideOverlay();
      }
    });
  }

  toggleOverlay() {
    if (this.overlayVisible) {
      this.hideOverlay();
    } else {
      this.showOverlay();
    }
  }

  showOverlay() {
    const overlay = document.getElementById('keyboard-shortcuts-overlay');
    const content = document.getElementById('shortcuts-content');
    
    // Group shortcuts by category
    const categories = {};
    
    for (const [key, shortcut] of this.shortcuts) {
      const cat = shortcut.category;
      if (!categories[cat]) categories[cat] = [];
      categories[cat].push(shortcut);
    }
    
    // Generate HTML
    const categoryNames = {
      transport: '▶️ Transport',
      window: '🪟 Windows',
      file: '📁 File',
      edit: '✏️ Edit',
      browser: '🔍 Browser',
      tool: '🛠️ Tools',
      pianoRoll: '🎹 Piano Roll',
      mixer: '🎚️ Mixer',
      zoom: '🔍 Zoom',
      pattern: '🎼 Patterns',
      metronome: '🎵 Metronome',
      help: '❓ Help'
    };
    
    let html = '<div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(400px, 1fr)); gap: 24px;">';
    
    for (const [cat, shortcuts] of Object.entries(categories)) {
      html += `
        <div>
          <h3 style="font-size: 14px; font-weight: 600; color: var(--fl-orange-primary); margin-bottom: 12px;">
            ${categoryNames[cat] || cat.toUpperCase()}
          </h3>
          <table style="width: 100%; font-size: 12px;">
      `;
      
      shortcuts.forEach(shortcut => {
        html += `
          <tr style="border-bottom: 1px solid var(--border-subtle);">
            <td style="padding: 8px 0; color: var(--text-secondary);">${shortcut.description}</td>
            <td style="padding: 8px 0; text-align: right;">
              <code style="background: var(--bg-tertiary); padding: 4px 8px; border-radius: 4px; font-family: var(--font-mono); font-size: 11px;">
                ${shortcut.keyCombo}
              </code>
            </td>
          </tr>
        `;
      });
      
      html += '</table></div>';
    }
    
    html += '</div>';
    
    content.innerHTML = html;
    overlay.style.display = 'flex';
    this.overlayVisible = true;
  }

  hideOverlay() {
    const overlay = document.getElementById('keyboard-shortcuts-overlay');
    overlay.style.display = 'none';
    this.overlayVisible = false;
  }

  enable() {
    this.isEnabled = true;
  }

  disable() {
    this.isEnabled = false;
  }
}

// Create global instance
window.keyboardShortcuts = new KeyboardShortcutsManager();

// Add necessary animations to stylesheet
const style = document.createElement('style');
style.textContent = `
  @keyframes slideInRight {
    from {
      transform: translateX(100px);
      opacity: 0;
    }
    to {
      transform: translateX(0);
      opacity: 1;
    }
  }
  
  @keyframes fadeOut {
    from { opacity: 1; }
    to { opacity: 0; }
  }
`;
document.head.appendChild(style);

export default KeyboardShortcutsManager;
