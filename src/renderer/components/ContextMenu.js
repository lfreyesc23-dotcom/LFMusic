// 🖱️ FRUTILLA STUDIO - Context Menu System
// ==========================================

export class ContextMenuManager {
  constructor() {
    this.activeMenu = null;
    this.recentActions = [];
    this.maxRecent = 5;
    
    this.attachGlobalListeners();
  }

  attachGlobalListeners() {
    // Close menu on click outside
    document.addEventListener('click', (e) => {
      if (this.activeMenu && !e.target.closest('.fl-context-menu')) {
        this.close();
      }
    });
    
    // Close on Escape
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape' && this.activeMenu) {
        this.close();
      }
    });
    
    // Prevent default context menu
    document.addEventListener('contextmenu', (e) => {
      const target = e.target.closest('[data-context-menu]');
      if (target) {
        e.preventDefault();
      }
    });
  }

  show(x, y, items, options = {}) {
    this.close(); // Close any existing menu
    
    const menu = document.createElement('div');
    menu.className = 'fl-context-menu';
    menu.style.left = x + 'px';
    menu.style.top = y + 'px';
    
    // Add recently used section if enabled
    if (options.showRecent && this.recentActions.length > 0) {
      items = [
        { type: 'section', label: 'Recently Used' },
        ...this.recentActions.map(action => ({ ...action, recent: true })),
        { type: 'separator' },
        ...items
      ];
    }
    
    items.forEach(item => {
      if (item.type === 'separator') {
        const separator = document.createElement('div');
        separator.className = 'fl-context-menu-separator';
        menu.appendChild(separator);
      } else if (item.type === 'section') {
        const section = document.createElement('div');
        section.className = 'fl-context-menu-section';
        section.style.cssText = `
          padding: 8px 12px 4px;
          font-size: 11px;
          font-weight: 600;
          color: var(--text-tertiary);
          text-transform: uppercase;
          letter-spacing: 0.5px;
        `;
        section.textContent = item.label;
        menu.appendChild(section);
      } else {
        const menuItem = this.createMenuItem(item);
        menu.appendChild(menuItem);
      }
    });
    
    document.body.appendChild(menu);
    this.activeMenu = menu;
    
    // Adjust position if menu goes off-screen
    this.adjustPosition(menu, x, y);
  }

  createMenuItem(item) {
    const menuItem = document.createElement('div');
    menuItem.className = 'fl-context-menu-item';
    
    if (item.disabled) {
      menuItem.classList.add('disabled');
    }
    
    if (item.danger) {
      menuItem.classList.add('danger');
    }
    
    // Icon
    if (item.icon) {
      const icon = document.createElement('span');
      icon.textContent = item.icon;
      icon.style.marginRight = '8px';
      menuItem.appendChild(icon);
    }
    
    // Label
    const label = document.createElement('span');
    label.textContent = item.label;
    menuItem.appendChild(label);
    
    // Shortcut
    if (item.shortcut) {
      const shortcut = document.createElement('span');
      shortcut.className = 'fl-context-menu-shortcut';
      shortcut.textContent = item.shortcut;
      menuItem.appendChild(shortcut);
    }
    
    // Submenu indicator
    if (item.submenu) {
      const arrow = document.createElement('span');
      arrow.textContent = '▸';
      arrow.style.marginLeft = 'auto';
      menuItem.appendChild(arrow);
    }
    
    // Click handler
    if (!item.disabled) {
      menuItem.addEventListener('click', (e) => {
        e.stopPropagation();
        
        if (item.submenu) {
          this.showSubmenu(menuItem, item.submenu);
        } else if (item.action) {
          item.action();
          this.addToRecent(item);
          this.close();
        }
      });
      
      // Submenu on hover
      if (item.submenu) {
        menuItem.addEventListener('mouseenter', () => {
          this.showSubmenu(menuItem, item.submenu);
        });
      }
    }
    
    return menuItem;
  }

  showSubmenu(parentItem, items) {
    // Remove existing submenus
    document.querySelectorAll('.fl-context-menu-submenu').forEach(sm => sm.remove());
    
    const submenu = document.createElement('div');
    submenu.className = 'fl-context-menu fl-context-menu-submenu';
    
    items.forEach(item => {
      if (item.type === 'separator') {
        const separator = document.createElement('div');
        separator.className = 'fl-context-menu-separator';
        submenu.appendChild(separator);
      } else {
        const menuItem = this.createMenuItem(item);
        submenu.appendChild(menuItem);
      }
    });
    
    document.body.appendChild(submenu);
    
    // Position submenu
    const parentRect = parentItem.getBoundingClientRect();
    const submenuRect = submenu.getBoundingClientRect();
    
    let left = parentRect.right;
    let top = parentRect.top;
    
    // Adjust if going off-screen
    if (left + submenuRect.width > window.innerWidth) {
      left = parentRect.left - submenuRect.width;
    }
    
    if (top + submenuRect.height > window.innerHeight) {
      top = window.innerHeight - submenuRect.height - 10;
    }
    
    submenu.style.left = left + 'px';
    submenu.style.top = top + 'px';
  }

  adjustPosition(menu, x, y) {
    const rect = menu.getBoundingClientRect();
    
    let adjustedX = x;
    let adjustedY = y;
    
    // Check right edge
    if (x + rect.width > window.innerWidth) {
      adjustedX = window.innerWidth - rect.width - 10;
    }
    
    // Check bottom edge
    if (y + rect.height > window.innerHeight) {
      adjustedY = window.innerHeight - rect.height - 10;
    }
    
    // Check top edge
    if (adjustedY < 0) {
      adjustedY = 10;
    }
    
    // Check left edge
    if (adjustedX < 0) {
      adjustedX = 10;
    }
    
    menu.style.left = adjustedX + 'px';
    menu.style.top = adjustedY + 'px';
  }

  close() {
    if (this.activeMenu) {
      this.activeMenu.remove();
      this.activeMenu = null;
    }
    
    // Remove submenus
    document.querySelectorAll('.fl-context-menu-submenu').forEach(sm => sm.remove());
  }

  addToRecent(item) {
    if (item.recent) return; // Don't add recent items to recent
    
    // Remove if already exists
    this.recentActions = this.recentActions.filter(a => a.label !== item.label);
    
    // Add to beginning
    this.recentActions.unshift({
      label: item.label,
      icon: item.icon,
      action: item.action
    });
    
    // Trim to max
    if (this.recentActions.length > this.maxRecent) {
      this.recentActions = this.recentActions.slice(0, this.maxRecent);
    }
  }
}

// Create global instance
window.contextMenu = new ContextMenuManager();

// === CONTEXT MENU PRESETS ===

export const ContextMenuPresets = {
  // Track context menu
  track: (trackId, trackName) => [
    { icon: '✂️', label: 'Cut', shortcut: 'Ctrl+X', action: () => console.log('Cut', trackId) },
    { icon: '📋', label: 'Copy', shortcut: 'Ctrl+C', action: () => console.log('Copy', trackId) },
    { icon: '📄', label: 'Paste', shortcut: 'Ctrl+V', action: () => console.log('Paste', trackId) },
    { icon: '🗑️', label: 'Delete', shortcut: 'Del', action: () => console.log('Delete', trackId), danger: true },
    { type: 'separator' },
    { icon: '📂', label: 'Duplicate', shortcut: 'Ctrl+D', action: () => console.log('Duplicate', trackId) },
    { icon: '🔗', label: 'Clone', action: () => console.log('Clone', trackId) },
    { type: 'separator' },
    { icon: '🎨', label: 'Color', submenu: [
      { label: 'Red', action: () => console.log('Color red') },
      { label: 'Orange', action: () => console.log('Color orange') },
      { label: 'Yellow', action: () => console.log('Color yellow') },
      { label: 'Green', action: () => console.log('Color green') },
      { label: 'Blue', action: () => console.log('Color blue') },
      { label: 'Purple', action: () => console.log('Color purple') },
    ]},
    { icon: '✏️', label: 'Rename', shortcut: 'F2', action: () => console.log('Rename', trackId) },
    { type: 'separator' },
    { icon: '🔒', label: 'Lock', action: () => console.log('Lock', trackId) },
    { icon: '👁️', label: 'Hide', action: () => console.log('Hide', trackId) },
  ],

  // Clip/Pattern context menu
  clip: (clipId) => [
    { icon: '✂️', label: 'Cut', shortcut: 'Ctrl+X', action: () => console.log('Cut clip', clipId) },
    { icon: '📋', label: 'Copy', shortcut: 'Ctrl+C', action: () => console.log('Copy clip', clipId) },
    { icon: '🗑️', label: 'Delete', shortcut: 'Del', action: () => console.log('Delete clip', clipId), danger: true },
    { type: 'separator' },
    { icon: '📂', label: 'Duplicate', shortcut: 'Ctrl+D', action: () => console.log('Duplicate clip', clipId) },
    { icon: '🔗', label: 'Make Unique', shortcut: 'Ctrl+U', action: () => console.log('Make unique', clipId) },
    { type: 'separator' },
    { icon: '✨', label: 'Quantize', shortcut: 'Ctrl+Q', action: () => console.log('Quantize', clipId) },
    { icon: '🎵', label: 'Transpose', submenu: [
      { label: '+12 (1 Octave)', action: () => console.log('Transpose +12') },
      { label: '+1 (Semitone)', action: () => console.log('Transpose +1') },
      { label: '-1 (Semitone)', action: () => console.log('Transpose -1') },
      { label: '-12 (1 Octave)', action: () => console.log('Transpose -12') },
    ]},
    { icon: '⏱️', label: 'Time Stretch', action: () => console.log('Time stretch', clipId) },
    { type: 'separator' },
    { icon: '🎨', label: 'Color', submenu: [
      { label: 'Pattern Color 1', action: () => console.log('Color 1') },
      { label: 'Pattern Color 2', action: () => console.log('Color 2') },
      { label: 'Pattern Color 3', action: () => console.log('Color 3') },
    ]},
    { icon: '✏️', label: 'Rename', shortcut: 'F2', action: () => console.log('Rename', clipId) },
  ],

  // Mixer channel context menu
  mixerChannel: (channelId) => [
    { icon: '🔇', label: 'Solo', shortcut: 'S', action: () => console.log('Solo', channelId) },
    { icon: '🔕', label: 'Mute', shortcut: 'M', action: () => console.log('Mute', channelId) },
    { icon: '🎙️', label: 'Arm Recording', shortcut: 'R', action: () => console.log('Arm', channelId) },
    { type: 'separator' },
    { icon: '📋', label: 'Copy Channel State', action: () => console.log('Copy state', channelId) },
    { icon: '📄', label: 'Paste Channel State', action: () => console.log('Paste state', channelId) },
    { type: 'separator' },
    { icon: '🔌', label: 'Add Plugin', submenu: [
      { label: 'EQ', action: () => console.log('Add EQ') },
      { label: 'Compressor', action: () => console.log('Add Compressor') },
      { label: 'Reverb', action: () => console.log('Add Reverb') },
      { label: 'Delay', action: () => console.log('Add Delay') },
      { type: 'separator' },
      { label: 'Browse All...', action: () => console.log('Browse plugins') },
    ]},
    { icon: '📊', label: 'Show Automation', action: () => console.log('Show automation', channelId) },
    { type: 'separator' },
    { icon: '🎨', label: 'Channel Color', action: () => console.log('Channel color', channelId) },
    { icon: '✏️', label: 'Rename', shortcut: 'F2', action: () => console.log('Rename', channelId) },
    { type: 'separator' },
    { icon: '🗑️', label: 'Delete Channel', action: () => console.log('Delete', channelId), danger: true },
  ],

  // Piano Roll note context menu
  pianoRollNote: (noteId) => [
    { icon: '✂️', label: 'Cut', shortcut: 'Ctrl+X', action: () => console.log('Cut note', noteId) },
    { icon: '📋', label: 'Copy', shortcut: 'Ctrl+C', action: () => console.log('Copy note', noteId) },
    { icon: '🗑️', label: 'Delete', shortcut: 'Del', action: () => console.log('Delete note', noteId), danger: true },
    { type: 'separator' },
    { icon: '🎵', label: 'Quantize', shortcut: 'Ctrl+Q', action: () => console.log('Quantize note', noteId) },
    { icon: '🎹', label: 'Chordize', submenu: [
      { label: 'Major', action: () => console.log('Major chord') },
      { label: 'Minor', action: () => console.log('Minor chord') },
      { label: 'Diminished', action: () => console.log('Diminished chord') },
      { label: 'Augmented', action: () => console.log('Augmented chord') },
      { label: '7th', action: () => console.log('7th chord') },
    ]},
    { icon: '🎸', label: 'Arpeggiate', action: () => console.log('Arpeggiate', noteId) },
    { icon: '🌊', label: 'Strum', action: () => console.log('Strum', noteId) },
    { icon: '🎲', label: 'Humanize', action: () => console.log('Humanize', noteId) },
  ],

  // Browser item context menu
  browserItem: (itemPath) => [
    { icon: '▶️', label: 'Preview', shortcut: 'Space', action: () => console.log('Preview', itemPath) },
    { icon: '➕', label: 'Add to Project', shortcut: 'Enter', action: () => console.log('Add', itemPath) },
    { type: 'separator' },
    { icon: '⭐', label: 'Add to Favorites', action: () => console.log('Favorite', itemPath) },
    { icon: '🏷️', label: 'Add Tag', action: () => console.log('Tag', itemPath) },
    { icon: '⭐', label: 'Rate', submenu: [
      { label: '⭐⭐⭐⭐⭐', action: () => console.log('Rate 5') },
      { label: '⭐⭐⭐⭐', action: () => console.log('Rate 4') },
      { label: '⭐⭐⭐', action: () => console.log('Rate 3') },
      { label: '⭐⭐', action: () => console.log('Rate 2') },
      { label: '⭐', action: () => console.log('Rate 1') },
    ]},
    { type: 'separator' },
    { icon: '📂', label: 'Show in Finder', action: () => console.log('Show in finder', itemPath) },
    { icon: 'ℹ️', label: 'File Info', action: () => console.log('File info', itemPath) },
  ],
};

export default ContextMenuManager;
