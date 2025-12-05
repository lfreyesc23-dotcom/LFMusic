/**
 * Playlist View Component
 * FL Studio-style pattern/playlist arrangement view
 */

const { ipcRenderer } = require('electron');

class PlaylistView {
  constructor(containerId) {
    this.container = document.getElementById(containerId);
    this.patterns = [];
    this.tracks = 8;
    this.timelineLength = 32; // bars
    this.pixelsPerBeat = 20;
    this.selectedPattern = null;
    this.isPlaying = false;
    
    this.init();
    this.loadPatterns();
  }

  init() {
    this.container.innerHTML = `
      <div class="playlist-container">
        <div class="playlist-toolbar">
          <button id="btn-new-pattern" class="btn btn-primary">
            <span class="icon">➕</span> New Pattern
          </button>
          <button id="btn-clone-pattern" class="btn">
            <span class="icon">📋</span> Clone
          </button>
          <button id="btn-split-pattern" class="btn">
            <span class="icon">✂️</span> Split
          </button>
          <button id="btn-merge-patterns" class="btn">
            <span class="icon">🔗</span> Merge
          </button>
          <div class="spacer"></div>
          <button id="btn-snap-to-grid" class="btn btn-toggle active">
            <span class="icon">🧲</span> Snap
          </button>
          <select id="grid-size" class="select-input">
            <option value="0.25">1/16</option>
            <option value="0.5" selected>1/8</option>
            <option value="1">1/4</option>
            <option value="2">1/2</option>
            <option value="4">1 Bar</option>
          </select>
        </div>

        <div class="playlist-content">
          <div class="track-headers" id="track-headers"></div>
          <div class="timeline-wrapper">
            <div class="timeline-ruler" id="timeline-ruler"></div>
            <div class="playlist-grid" id="playlist-grid"></div>
          </div>
        </div>

        <div class="pattern-list">
          <h3>Patterns</h3>
          <div id="pattern-items"></div>
        </div>
      </div>
    `;

    this.setupEventListeners();
    this.renderGrid();
  }

  setupEventListeners() {
    document.getElementById('btn-new-pattern').addEventListener('click', () => this.createPattern());
    document.getElementById('btn-clone-pattern').addEventListener('click', () => this.clonePattern());
    document.getElementById('btn-split-pattern').addEventListener('click', () => this.splitPattern());
    document.getElementById('btn-merge-patterns').addEventListener('click', () => this.mergePatterns());
    
    const snapBtn = document.getElementById('btn-snap-to-grid');
    snapBtn.addEventListener('click', () => {
      snapBtn.classList.toggle('active');
      this.snapToGrid = snapBtn.classList.contains('active');
    });

    // Drag & drop for patterns
    const grid = document.getElementById('playlist-grid');
    grid.addEventListener('dragover', (e) => this.handleDragOver(e));
    grid.addEventListener('drop', (e) => this.handleDrop(e));
  }

  async createPattern() {
    try {
      const result = await ipcRenderer.invoke('omega:playlist:createPattern', {
        name: `Pattern ${this.patterns.length + 1}`,
        lengthInBeats: 16
      });
      
      this.patterns.push(result.pattern);
      this.renderPatternList();
      this.showNotification('Pattern created', 'success');
    } catch (error) {
      this.showNotification('Failed to create pattern', 'error');
      console.error(error);
    }
  }

  async clonePattern() {
    if (!this.selectedPattern) {
      this.showNotification('Select a pattern first', 'warning');
      return;
    }

    try {
      const result = await ipcRenderer.invoke('omega:playlist:clonePattern', {
        patternId: this.selectedPattern.id
      });
      
      this.patterns.push(result.pattern);
      this.renderPatternList();
      this.showNotification('Pattern cloned', 'success');
    } catch (error) {
      this.showNotification('Failed to clone pattern', 'error');
      console.error(error);
    }
  }

  async splitPattern() {
    if (!this.selectedPattern) {
      this.showNotification('Select a pattern placement to split', 'warning');
      return;
    }

    const splitTime = parseFloat(prompt('Split at beat:', '8'));
    if (isNaN(splitTime)) return;

    try {
      await ipcRenderer.invoke('omega:playlist:splitPattern', {
        patternId: this.selectedPattern.id,
        splitTime
      });
      
      await this.loadPatterns();
      this.showNotification('Pattern split', 'success');
    } catch (error) {
      this.showNotification('Failed to split pattern', 'error');
      console.error(error);
    }
  }

  async mergePatterns() {
    const selected = this.getSelectedPatternInstances();
    if (selected.length < 2) {
      this.showNotification('Select at least 2 patterns to merge', 'warning');
      return;
    }

    try {
      await ipcRenderer.invoke('omega:playlist:mergePatterns', {
        patternIds: selected.map(p => p.id)
      });
      
      await this.loadPatterns();
      this.showNotification('Patterns merged', 'success');
    } catch (error) {
      this.showNotification('Failed to merge patterns', 'error');
      console.error(error);
    }
  }

  async loadPatterns() {
    try {
      const result = await ipcRenderer.invoke('omega:playlist:getPatterns', {});
      this.patterns = result.patterns || [];
      this.renderPatternList();
      this.renderGrid();
    } catch (error) {
      console.error('Failed to load patterns:', error);
    }
  }

  renderPatternList() {
    const container = document.getElementById('pattern-items');
    container.innerHTML = this.patterns.map((pattern, index) => `
      <div class="pattern-item" 
           draggable="true"
           data-pattern-id="${pattern.id}"
           style="background-color: ${this.getPatternColor(index)}">
        <span class="pattern-number">${index + 1}</span>
        <span class="pattern-name">${pattern.name}</span>
        <span class="pattern-length">${pattern.lengthInBeats}♩</span>
      </div>
    `).join('');

    // Add drag listeners
    container.querySelectorAll('.pattern-item').forEach(item => {
      item.addEventListener('dragstart', (e) => this.handlePatternDragStart(e));
      item.addEventListener('click', (e) => this.selectPattern(e.target.dataset.patternId));
    });
  }

  renderGrid() {
    const grid = document.getElementById('playlist-grid');
    const ruler = document.getElementById('timeline-ruler');
    const headers = document.getElementById('track-headers');

    // Render track headers
    headers.innerHTML = Array.from({ length: this.tracks }, (_, i) => `
      <div class="track-header">Track ${i + 1}</div>
    `).join('');

    // Render timeline ruler
    ruler.innerHTML = Array.from({ length: this.timelineLength }, (_, i) => `
      <div class="ruler-mark" style="left: ${i * this.pixelsPerBeat * 4}px">${i + 1}</div>
    `).join('');

    // Render grid cells
    const gridHTML = [];
    for (let track = 0; track < this.tracks; track++) {
      for (let bar = 0; bar < this.timelineLength; bar++) {
        gridHTML.push(`
          <div class="grid-cell" 
               data-track="${track}" 
               data-bar="${bar}"
               style="left: ${bar * this.pixelsPerBeat * 4}px; top: ${track * 60}px">
          </div>
        `);
      }
    }
    grid.innerHTML = gridHTML.join('');
  }

  handlePatternDragStart(e) {
    e.dataTransfer.effectAllowed = 'copy';
    e.dataTransfer.setData('patternId', e.target.dataset.patternId);
  }

  handleDragOver(e) {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  }

  async handleDrop(e) {
    e.preventDefault();
    const patternId = e.dataTransfer.getData('patternId');
    
    // Calculate track and position from mouse coordinates
    const rect = e.target.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    
    const track = Math.floor(y / 60);
    let position = (x / this.pixelsPerBeat) / 4; // Convert to bars

    // Snap to grid
    if (this.snapToGrid) {
      const gridSize = parseFloat(document.getElementById('grid-size').value);
      position = Math.round(position / gridSize) * gridSize;
    }

    try {
      await ipcRenderer.invoke('omega:playlist:addToPlaylist', {
        patternId,
        track,
        startTime: position * 4 // Convert bars to beats
      });
      
      await this.loadPatterns();
      this.showNotification('Pattern added to playlist', 'success');
    } catch (error) {
      this.showNotification('Failed to add pattern', 'error');
      console.error(error);
    }
  }

  selectPattern(patternId) {
    this.selectedPattern = this.patterns.find(p => p.id === patternId);
    
    // Visual feedback
    document.querySelectorAll('.pattern-item').forEach(item => {
      item.classList.toggle('selected', item.dataset.patternId === patternId);
    });
  }

  getSelectedPatternInstances() {
    return document.querySelectorAll('.pattern-item.selected');
  }

  getPatternColor(index) {
    const colors = [
      '#ff6b6b', '#4ecdc4', '#45b7d1', '#f9ca24', 
      '#6c5ce7', '#a29bfe', '#fd79a8', '#fdcb6e'
    ];
    return colors[index % colors.length];
  }

  showNotification(message, type = 'info') {
    // Placeholder for notification system
    console.log(`[${type.toUpperCase()}] ${message}`);
  }
}

module.exports = PlaylistView;
