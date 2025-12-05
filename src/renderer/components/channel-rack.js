/**
 * Channel Rack Component
 * FL Studio-style 16-step sequencer with visual channel rack
 */

const { ipcRenderer } = require('electron');

class ChannelRack {
  constructor(containerId) {
    this.container = document.getElementById(containerId);
    this.channels = [];
    this.steps = 16;
    this.currentStep = -1;
    this.isPlaying = false;
    
    this.init();
    this.loadChannels();
  }

  init() {
    this.container.innerHTML = `
      <div class="channel-rack-container">
        <div class="channel-rack-toolbar">
          <button id="btn-add-channel" class="btn btn-primary">
            <span class="icon">➕</span> Add Channel
          </button>
          <button id="btn-load-preset" class="btn">
            <span class="icon">📁</span> Load Preset
          </button>
          <div class="spacer"></div>
          <button id="btn-play-pattern" class="btn btn-success">
            <span class="icon">▶️</span> Play
          </button>
          <button id="btn-stop-pattern" class="btn">
            <span class="icon">⏹️</span> Stop
          </button>
          <div class="bpm-control">
            <label>BPM:</label>
            <input type="number" id="input-bpm" value="120" min="60" max="200" step="1">
          </div>
        </div>

        <div class="channel-rack-grid">
          <div class="channel-list" id="channel-list"></div>
          <div class="step-grid" id="step-grid"></div>
        </div>

        <div class="channel-rack-footer">
          <div class="preset-buttons">
            <button class="btn-preset" data-preset="fourOnFloor">4-on-Floor</button>
            <button class="btn-preset" data-preset="backbeat">Backbeat</button>
            <button class="btn-preset" data-preset="highHat8th">Hi-Hat 8ths</button>
            <button class="btn-preset" data-preset="breakbeat">Breakbeat</button>
          </div>
        </div>
      </div>
    `;

    this.setupEventListeners();
  }

  setupEventListeners() {
    document.getElementById('btn-add-channel').addEventListener('click', () => this.addChannel());
    document.getElementById('btn-load-preset').addEventListener('click', () => this.loadPreset());
    document.getElementById('btn-play-pattern').addEventListener('click', () => this.play());
    document.getElementById('btn-stop-pattern').addEventListener('click', () => this.stop());
    
    document.querySelectorAll('.btn-preset').forEach(btn => {
      btn.addEventListener('click', (e) => this.applyPreset(e.target.dataset.preset));
    });

    document.getElementById('input-bpm').addEventListener('change', (e) => {
      this.setBPM(parseInt(e.target.value));
    });
  }

  async addChannel() {
    const name = prompt('Channel name:', `Channel ${this.channels.length + 1}`);
    if (!name) return;

    try {
      const result = await ipcRenderer.invoke('omega:channelRack:addChannel', {
        name,
        type: 'drum' // Default to drum channel
      });
      
      this.channels.push(result.channel);
      this.renderChannels();
      this.showNotification('Channel added', 'success');
    } catch (error) {
      this.showNotification('Failed to add channel', 'error');
      console.error(error);
    }
  }

  async loadChannels() {
    try {
      const result = await ipcRenderer.invoke('omega:channelRack:getChannels', {});
      this.channels = result.channels || [];
      this.renderChannels();
    } catch (error) {
      console.error('Failed to load channels:', error);
    }
  }

  renderChannels() {
    const channelList = document.getElementById('channel-list');
    const stepGrid = document.getElementById('step-grid');

    // Render channel names
    channelList.innerHTML = this.channels.map((channel, chIndex) => `
      <div class="channel-item" data-channel-id="${channel.id}">
        <div class="channel-color" style="background-color: ${this.getChannelColor(chIndex)}"></div>
        <span class="channel-name">${channel.name}</span>
        <button class="btn-small btn-mute" data-channel="${chIndex}">M</button>
        <button class="btn-small btn-solo" data-channel="${chIndex}">S</button>
        <input type="range" class="volume-slider" data-channel="${chIndex}" 
               min="0" max="100" value="${channel.volume || 100}">
      </div>
    `).join('');

    // Render step grid
    stepGrid.innerHTML = this.channels.map((channel, chIndex) => {
      const steps = Array.from({ length: this.steps }, (_, stepIndex) => {
        const isActive = channel.steps && channel.steps[stepIndex];
        const isCurrentStep = stepIndex === this.currentStep;
        
        return `
          <div class="step ${isActive ? 'active' : ''} ${isCurrentStep ? 'current' : ''}"
               data-channel="${chIndex}"
               data-step="${stepIndex}"
               onclick="channelRackInstance.toggleStep(${chIndex}, ${stepIndex})">
            <div class="step-indicator"></div>
          </div>
        `;
      }).join('');
      
      return `<div class="step-row">${steps}</div>`;
    }).join('');

    // Setup channel controls
    this.setupChannelControls();
  }

  setupChannelControls() {
    document.querySelectorAll('.btn-mute').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const channel = parseInt(e.target.dataset.channel);
        this.toggleMute(channel);
      });
    });

    document.querySelectorAll('.btn-solo').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const channel = parseInt(e.target.dataset.channel);
        this.toggleSolo(channel);
      });
    });

    document.querySelectorAll('.volume-slider').forEach(slider => {
      slider.addEventListener('input', (e) => {
        const channel = parseInt(e.target.dataset.channel);
        const volume = parseFloat(e.target.value) / 100;
        this.setChannelVolume(channel, volume);
      });
    });
  }

  async toggleStep(channel, step) {
    try {
      const result = await ipcRenderer.invoke('omega:channelRack:toggleStep', {
        channelIndex: channel,
        step
      });
      
      // Update local state
      if (this.channels[channel]) {
        if (!this.channels[channel].steps) {
          this.channels[channel].steps = Array(this.steps).fill(false);
        }
        this.channels[channel].steps[step] = result.active;
      }
      
      this.renderChannels();
    } catch (error) {
      console.error('Failed to toggle step:', error);
    }
  }

  async applyPreset(presetName) {
    const channelIndex = this.getSelectedChannelIndex();
    if (channelIndex === -1) {
      this.showNotification('Select a channel first', 'warning');
      return;
    }

    try {
      await ipcRenderer.invoke('omega:channelRack:loadPreset', {
        channelIndex,
        presetName
      });
      
      await this.loadChannels();
      this.showNotification(`Preset "${presetName}" loaded`, 'success');
    } catch (error) {
      this.showNotification('Failed to load preset', 'error');
      console.error(error);
    }
  }

  async play() {
    try {
      await ipcRenderer.invoke('omega:channelRack:play', {});
      this.isPlaying = true;
      this.startStepAnimation();
      
      document.getElementById('btn-play-pattern').classList.add('active');
      this.showNotification('Playing', 'success');
    } catch (error) {
      console.error('Failed to play:', error);
    }
  }

  async stop() {
    try {
      await ipcRenderer.invoke('omega:channelRack:stop', {});
      this.isPlaying = false;
      this.currentStep = -1;
      
      document.getElementById('btn-play-pattern').classList.remove('active');
      this.renderChannels();
      this.showNotification('Stopped', 'info');
    } catch (error) {
      console.error('Failed to stop:', error);
    }
  }

  async setBPM(bpm) {
    try {
      await ipcRenderer.invoke('omega:channelRack:setBPM', { bpm });
      this.showNotification(`BPM set to ${bpm}`, 'info');
    } catch (error) {
      console.error('Failed to set BPM:', error);
    }
  }

  async toggleMute(channel) {
    try {
      const result = await ipcRenderer.invoke('omega:channelRack:setMute', {
        channelIndex: channel,
        mute: !this.channels[channel].muted
      });
      
      this.channels[channel].muted = result.muted;
      document.querySelector(`.btn-mute[data-channel="${channel}"]`)
        .classList.toggle('active', result.muted);
    } catch (error) {
      console.error('Failed to toggle mute:', error);
    }
  }

  async toggleSolo(channel) {
    try {
      const result = await ipcRenderer.invoke('omega:channelRack:setSolo', {
        channelIndex: channel,
        solo: !this.channels[channel].solo
      });
      
      this.channels[channel].solo = result.solo;
      document.querySelector(`.btn-solo[data-channel="${channel}"]`)
        .classList.toggle('active', result.solo);
    } catch (error) {
      console.error('Failed to toggle solo:', error);
    }
  }

  async setChannelVolume(channel, volume) {
    try {
      await ipcRenderer.invoke('omega:channelRack:setVolume', {
        channelIndex: channel,
        volume
      });
    } catch (error) {
      console.error('Failed to set volume:', error);
    }
  }

  startStepAnimation() {
    const bpm = parseInt(document.getElementById('input-bpm').value);
    const stepDuration = (60 / bpm / 4) * 1000; // Duration per 16th note in ms

    const animate = () => {
      if (!this.isPlaying) return;
      
      this.currentStep = (this.currentStep + 1) % this.steps;
      this.renderChannels();
      
      setTimeout(animate, stepDuration);
    };

    animate();
  }

  getSelectedChannelIndex() {
    const selected = document.querySelector('.channel-item.selected');
    return selected ? parseInt(selected.dataset.channelId) : -1;
  }

  getChannelColor(index) {
    const colors = [
      '#e74c3c', '#3498db', '#2ecc71', '#f39c12',
      '#9b59b6', '#1abc9c', '#e67e22', '#95a5a6'
    ];
    return colors[index % colors.length];
  }

  showNotification(message, type = 'info') {
    console.log(`[${type.toUpperCase()}] ${message}`);
  }
}

// Global instance for onclick handlers
let channelRackInstance;

module.exports = ChannelRack;
