// 📊 FRUTILLA STUDIO - Waveform Renderer (Edison Style)
// ========================================================

export class WaveformRenderer {
  constructor(canvas, options = {}) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.options = {
      backgroundColor: '#2D2D2D',
      waveformColor: '#FF8C42',
      waveformColorAlt: '#FFA15C',
      centerLineColor: '#4A4A4A',
      gridColor: '#3C3C3C',
      cursorColor: '#FFFFFF',
      selectionColor: 'rgba(255, 140, 66, 0.2)',
      showGrid: true,
      showCenterLine: true,
      stereo: false,
      ...options
    };
    
    this.audioBuffer = null;
    this.zoomLevel = 1;
    this.scrollOffset = 0;
    this.selection = null;
    
    this.setupCanvas();
  }

  setupCanvas() {
    // High DPI support
    const dpr = window.devicePixelRatio || 1;
    const rect = this.canvas.getBoundingClientRect();
    
    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;
    
    this.ctx.scale(dpr, dpr);
    this.canvas.style.width = rect.width + 'px';
    this.canvas.style.height = rect.height + 'px';
  }

  setAudioBuffer(buffer) {
    this.audioBuffer = buffer;
    this.render();
  }

  setZoom(level) {
    this.zoomLevel = Math.max(0.1, Math.min(100, level));
    this.render();
  }

  setScroll(offset) {
    if (!this.audioBuffer) return;
    
    const maxOffset = Math.max(0, this.audioBuffer.length - this.getVisibleSamples());
    this.scrollOffset = Math.max(0, Math.min(maxOffset, offset));
    this.render();
  }

  setSelection(start, end) {
    this.selection = start !== null && end !== null ? { start, end } : null;
    this.render();
  }

  getVisibleSamples() {
    if (!this.audioBuffer) return 0;
    return Math.floor(this.audioBuffer.length / this.zoomLevel);
  }

  render() {
    if (!this.audioBuffer) {
      this.renderEmpty();
      return;
    }

    const width = this.canvas.clientWidth;
    const height = this.canvas.clientHeight;

    // Clear
    this.ctx.fillStyle = this.options.backgroundColor;
    this.ctx.fillRect(0, 0, width, height);

    // Grid
    if (this.options.showGrid) {
      this.renderGrid(width, height);
    }

    // Center line
    if (this.options.showCenterLine) {
      this.renderCenterLine(width, height);
    }

    // Selection
    if (this.selection) {
      this.renderSelection(width, height);
    }

    // Waveform
    if (this.options.stereo && this.audioBuffer.numberOfChannels > 1) {
      this.renderStereoWaveform(width, height);
    } else {
      this.renderMonoWaveform(width, height);
    }
  }

  renderEmpty() {
    const width = this.canvas.clientWidth;
    const height = this.canvas.clientHeight;

    this.ctx.fillStyle = this.options.backgroundColor;
    this.ctx.fillRect(0, 0, width, height);

    this.ctx.strokeStyle = this.options.centerLineColor;
    this.ctx.lineWidth = 1;
    this.ctx.beginPath();
    this.ctx.moveTo(0, height / 2);
    this.ctx.lineTo(width, height / 2);
    this.ctx.stroke();

    this.ctx.fillStyle = '#808080';
    this.ctx.font = '14px -apple-system, BlinkMacSystemFont, sans-serif';
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText('No audio loaded', width / 2, height / 2);
  }

  renderGrid(width, height) {
    this.ctx.strokeStyle = this.options.gridColor;
    this.ctx.lineWidth = 1;

    // Horizontal lines
    const horizontalLines = 5;
    for (let i = 0; i <= horizontalLines; i++) {
      const y = (height / horizontalLines) * i;
      this.ctx.beginPath();
      this.ctx.moveTo(0, y);
      this.ctx.lineTo(width, y);
      this.ctx.stroke();
    }

    // Vertical lines (time grid)
    const pixelsPerSecond = (width / this.getVisibleSamples()) * this.audioBuffer.sampleRate;
    const secondInterval = pixelsPerSecond > 50 ? 1 : (pixelsPerSecond > 25 ? 2 : 5);
    const verticalLines = Math.floor(this.getVisibleSamples() / this.audioBuffer.sampleRate / secondInterval);
    
    for (let i = 0; i <= verticalLines; i++) {
      const x = (width / verticalLines) * i;
      this.ctx.beginPath();
      this.ctx.moveTo(x, 0);
      this.ctx.lineTo(x, height);
      this.ctx.stroke();
    }
  }

  renderCenterLine(width, height) {
    this.ctx.strokeStyle = this.options.centerLineColor;
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    this.ctx.moveTo(0, height / 2);
    this.ctx.lineTo(width, height / 2);
    this.ctx.stroke();
  }

  renderSelection(width, height) {
    if (!this.selection) return;

    const visibleSamples = this.getVisibleSamples();
    const startX = ((this.selection.start - this.scrollOffset) / visibleSamples) * width;
    const endX = ((this.selection.end - this.scrollOffset) / visibleSamples) * width;

    this.ctx.fillStyle = this.options.selectionColor;
    this.ctx.fillRect(startX, 0, endX - startX, height);

    // Selection borders
    this.ctx.strokeStyle = this.options.waveformColor;
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    this.ctx.moveTo(startX, 0);
    this.ctx.lineTo(startX, height);
    this.ctx.moveTo(endX, 0);
    this.ctx.lineTo(endX, height);
    this.ctx.stroke();
  }

  renderMonoWaveform(width, height) {
    const channelData = this.audioBuffer.getChannelData(0);
    const visibleSamples = this.getVisibleSamples();
    const step = Math.max(1, Math.floor(visibleSamples / width));
    
    this.ctx.fillStyle = this.options.waveformColor;
    this.ctx.strokeStyle = this.options.waveformColor;
    this.ctx.lineWidth = 1;

    const centerY = height / 2;
    const amplitude = height / 2 - 4;

    // Draw waveform
    this.ctx.beginPath();
    
    for (let x = 0; x < width; x++) {
      const startIdx = this.scrollOffset + Math.floor((x / width) * visibleSamples);
      const endIdx = Math.min(channelData.length, startIdx + step);
      
      let min = 0;
      let max = 0;
      
      // Find min/max in this segment (for peak waveform)
      for (let i = startIdx; i < endIdx; i++) {
        const sample = channelData[i] || 0;
        min = Math.min(min, sample);
        max = Math.max(max, sample);
      }
      
      const yMin = centerY - min * amplitude;
      const yMax = centerY - max * amplitude;
      
      // Draw vertical line for this pixel
      this.ctx.moveTo(x, yMax);
      this.ctx.lineTo(x, yMin);
    }
    
    this.ctx.stroke();
  }

  renderStereoWaveform(width, height) {
    const leftData = this.audioBuffer.getChannelData(0);
    const rightData = this.audioBuffer.getChannelData(1);
    const visibleSamples = this.getVisibleSamples();
    const step = Math.max(1, Math.floor(visibleSamples / width));
    
    const channelHeight = height / 2;
    const amplitude = channelHeight / 2 - 4;

    // Left channel (top half)
    this.ctx.fillStyle = this.options.waveformColor;
    this.ctx.strokeStyle = this.options.waveformColor;
    this.ctx.beginPath();
    
    const leftCenterY = channelHeight / 2;
    
    for (let x = 0; x < width; x++) {
      const startIdx = this.scrollOffset + Math.floor((x / width) * visibleSamples);
      const endIdx = Math.min(leftData.length, startIdx + step);
      
      let min = 0;
      let max = 0;
      
      for (let i = startIdx; i < endIdx; i++) {
        const sample = leftData[i] || 0;
        min = Math.min(min, sample);
        max = Math.max(max, sample);
      }
      
      const yMin = leftCenterY - min * amplitude;
      const yMax = leftCenterY - max * amplitude;
      
      this.ctx.moveTo(x, yMax);
      this.ctx.lineTo(x, yMin);
    }
    
    this.ctx.stroke();

    // Right channel (bottom half)
    this.ctx.fillStyle = this.options.waveformColorAlt;
    this.ctx.strokeStyle = this.options.waveformColorAlt;
    this.ctx.beginPath();
    
    const rightCenterY = channelHeight + channelHeight / 2;
    
    for (let x = 0; x < width; x++) {
      const startIdx = this.scrollOffset + Math.floor((x / width) * visibleSamples);
      const endIdx = Math.min(rightData.length, startIdx + step);
      
      let min = 0;
      let max = 0;
      
      for (let i = startIdx; i < endIdx; i++) {
        const sample = rightData[i] || 0;
        min = Math.min(min, sample);
        max = Math.max(max, sample);
      }
      
      const yMin = rightCenterY - min * amplitude;
      const yMax = rightCenterY - max * amplitude;
      
      this.ctx.moveTo(x, yMax);
      this.ctx.lineTo(x, yMin);
    }
    
    this.ctx.stroke();

    // Divider line
    this.ctx.strokeStyle = this.options.centerLineColor;
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    this.ctx.moveTo(0, channelHeight);
    this.ctx.lineTo(width, channelHeight);
    this.ctx.stroke();
  }

  // Utility: Get sample at pixel position
  pixelToSample(x) {
    const width = this.canvas.clientWidth;
    const visibleSamples = this.getVisibleSamples();
    return this.scrollOffset + Math.floor((x / width) * visibleSamples);
  }

  // Utility: Get pixel position from sample
  sampleToPixel(sample) {
    const width = this.canvas.clientWidth;
    const visibleSamples = this.getVisibleSamples();
    return ((sample - this.scrollOffset) / visibleSamples) * width;
  }
}

// Thumbnail waveform generator
export class WaveformThumbnail {
  static async generate(audioBuffer, width = 200, height = 60) {
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    
    const ctx = canvas.getContext('2d');
    
    // Background
    ctx.fillStyle = '#2D2D2D';
    ctx.fillRect(0, 0, width, height);
    
    // Waveform
    const channelData = audioBuffer.getChannelData(0);
    const step = Math.floor(channelData.length / width);
    
    ctx.fillStyle = '#FF8C42';
    ctx.strokeStyle = '#FF8C42';
    ctx.lineWidth = 1;
    
    const centerY = height / 2;
    const amplitude = height / 2 - 2;
    
    ctx.beginPath();
    
    for (let x = 0; x < width; x++) {
      const startIdx = x * step;
      const endIdx = Math.min(channelData.length, startIdx + step);
      
      let min = 0;
      let max = 0;
      
      for (let i = startIdx; i < endIdx; i++) {
        const sample = channelData[i] || 0;
        min = Math.min(min, sample);
        max = Math.max(max, sample);
      }
      
      const yMin = centerY - min * amplitude;
      const yMax = centerY - max * amplitude;
      
      ctx.moveTo(x, yMax);
      ctx.lineTo(x, yMin);
    }
    
    ctx.stroke();
    
    return canvas.toDataURL();
  }
}
