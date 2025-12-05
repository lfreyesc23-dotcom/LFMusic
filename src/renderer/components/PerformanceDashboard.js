// 📊 FRUTILLA STUDIO - Performance Dashboard
// ============================================

export class PerformanceDashboard {
  constructor(container) {
    this.container = container;
    this.metrics = {
      cpuUsage: 0,
      memoryUsage: 0,
      latency: 0,
      bufferSize: 512,
      sampleRate: 44100,
      polyphony: 0,
      maxPolyphony: 128,
      pluginCPU: new Map(),
      dropouts: 0,
      lastDropout: null
    };
    
    this.updateInterval = 100; // ms
    this.history = {
      cpu: [],
      memory: [],
      polyphony: [],
      maxSamples: 100
    };
    
    this.render();
    this.startMonitoring();
  }

  render() {
    this.container.innerHTML = `
      <div class="performance-dashboard">
        <div class="dashboard-header">
          <h3>⚡ Performance Monitor</h3>
          <button class="fl-button fl-button-ghost fl-button-sm" id="reset-metrics">Reset</button>
        </div>
        
        <div class="metrics-grid">
          <!-- CPU Usage -->
          <div class="metric-card">
            <div class="metric-header">
              <span class="metric-icon">💻</span>
              <span class="metric-title">CPU Usage</span>
            </div>
            <div class="metric-value" id="cpu-value">0%</div>
            <div class="metric-bar">
              <div class="metric-bar-fill" id="cpu-bar" style="width: 0%"></div>
            </div>
            <canvas id="cpu-chart" width="200" height="40"></canvas>
          </div>
          
          <!-- Memory Usage -->
          <div class="metric-card">
            <div class="metric-header">
              <span class="metric-icon">🧠</span>
              <span class="metric-title">Memory</span>
            </div>
            <div class="metric-value" id="memory-value">0 MB</div>
            <div class="metric-bar">
              <div class="metric-bar-fill" id="memory-bar" style="width: 0%"></div>
            </div>
            <canvas id="memory-chart" width="200" height="40"></canvas>
          </div>
          
          <!-- Latency -->
          <div class="metric-card">
            <div class="metric-header">
              <span class="metric-icon">⏱️</span>
              <span class="metric-title">Latency</span>
            </div>
            <div class="metric-value" id="latency-value">0 ms</div>
            <div class="metric-info">
              <div>Buffer: <span id="buffer-size">512</span></div>
              <div>Rate: <span id="sample-rate">44.1</span> kHz</div>
            </div>
          </div>
          
          <!-- Polyphony -->
          <div class="metric-card">
            <div class="metric-header">
              <span class="metric-icon">🎵</span>
              <span class="metric-title">Polyphony</span>
            </div>
            <div class="metric-value" id="polyphony-value">0 / 128</div>
            <div class="metric-bar">
              <div class="metric-bar-fill" id="polyphony-bar" style="width: 0%"></div>
            </div>
            <canvas id="polyphony-chart" width="200" height="40"></canvas>
          </div>
          
          <!-- Dropouts -->
          <div class="metric-card ${this.metrics.dropouts > 0 ? 'metric-card-warning' : ''}">
            <div class="metric-header">
              <span class="metric-icon">⚠️</span>
              <span class="metric-title">Buffer Dropouts</span>
            </div>
            <div class="metric-value" id="dropouts-value">0</div>
            <div class="metric-info" id="dropout-info">No dropouts</div>
          </div>
          
          <!-- Status -->
          <div class="metric-card metric-card-status">
            <div class="metric-header">
              <span class="metric-icon">✓</span>
              <span class="metric-title">Status</span>
            </div>
            <div class="status-indicator" id="status-indicator">
              <div class="status-dot status-dot-good"></div>
              <div class="status-text">Excellent</div>
            </div>
          </div>
        </div>
        
        <!-- Plugin CPU Usage -->
        <div class="plugin-cpu-section">
          <h4>🔌 Plugin CPU Usage</h4>
          <div class="plugin-cpu-list" id="plugin-cpu-list">
            <div class="plugin-cpu-empty">No plugins loaded</div>
          </div>
        </div>
        
        <!-- Recommendations -->
        <div class="recommendations-section" id="recommendations">
          <!-- Dynamic recommendations will appear here -->
        </div>
      </div>
    `;
    
    this.attachStyles();
    this.attachEventListeners();
    this.initializeCharts();
  }

  attachStyles() {
    const style = document.createElement('style');
    style.textContent = `
      .performance-dashboard {
        padding: var(--space-lg);
        background: var(--bg-secondary);
        border-radius: var(--radius-lg);
        height: 100%;
        overflow-y: auto;
      }
      
      .dashboard-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: var(--space-lg);
      }
      
      .dashboard-header h3 {
        font-size: var(--font-size-xl);
        font-weight: var(--font-weight-semibold);
        color: var(--text-primary);
      }
      
      .metrics-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
        gap: var(--space-md);
        margin-bottom: var(--space-lg);
      }
      
      .metric-card {
        background: var(--bg-tertiary);
        border-radius: var(--radius-md);
        padding: var(--space-md);
        border: 1px solid var(--border-subtle);
        transition: all var(--transition-fast);
      }
      
      .metric-card:hover {
        border-color: var(--border-normal);
        box-shadow: var(--shadow-sm);
      }
      
      .metric-card-warning {
        border-color: var(--fl-red-danger);
        background: rgba(231, 76, 60, 0.1);
      }
      
      .metric-card-status {
        grid-column: span 2;
      }
      
      .metric-header {
        display: flex;
        align-items: center;
        gap: var(--space-sm);
        margin-bottom: var(--space-sm);
      }
      
      .metric-icon {
        font-size: 20px;
      }
      
      .metric-title {
        font-size: var(--font-size-sm);
        color: var(--text-secondary);
        font-weight: var(--font-weight-medium);
      }
      
      .metric-value {
        font-size: var(--font-size-2xl);
        font-weight: var(--font-weight-bold);
        color: var(--text-primary);
        font-family: var(--font-mono);
        margin-bottom: var(--space-sm);
      }
      
      .metric-bar {
        height: 8px;
        background: var(--bg-secondary);
        border-radius: var(--radius-full);
        overflow: hidden;
        margin-bottom: var(--space-sm);
      }
      
      .metric-bar-fill {
        height: 100%;
        background: linear-gradient(90deg, var(--fl-green-success), var(--fl-yellow-warning), var(--fl-red-danger));
        transition: width var(--transition-fast);
        border-radius: var(--radius-full);
      }
      
      .metric-info {
        font-size: var(--font-size-xs);
        color: var(--text-tertiary);
        display: flex;
        gap: var(--space-md);
      }
      
      .status-indicator {
        display: flex;
        align-items: center;
        gap: var(--space-sm);
      }
      
      .status-dot {
        width: 12px;
        height: 12px;
        border-radius: 50%;
        animation: pulse 2s ease-in-out infinite;
      }
      
      .status-dot-good {
        background: var(--fl-green-success);
        box-shadow: 0 0 8px var(--fl-green-success);
      }
      
      .status-dot-warning {
        background: var(--fl-yellow-warning);
        box-shadow: 0 0 8px var(--fl-yellow-warning);
      }
      
      .status-dot-bad {
        background: var(--fl-red-danger);
        box-shadow: 0 0 8px var(--fl-red-danger);
      }
      
      .status-text {
        font-size: var(--font-size-lg);
        font-weight: var(--font-weight-semibold);
        color: var(--text-primary);
      }
      
      .plugin-cpu-section {
        background: var(--bg-tertiary);
        border-radius: var(--radius-md);
        padding: var(--space-md);
        margin-bottom: var(--space-lg);
      }
      
      .plugin-cpu-section h4 {
        font-size: var(--font-size-md);
        font-weight: var(--font-weight-semibold);
        color: var(--text-primary);
        margin-bottom: var(--space-md);
      }
      
      .plugin-cpu-list {
        display: flex;
        flex-direction: column;
        gap: var(--space-sm);
      }
      
      .plugin-cpu-item {
        display: flex;
        align-items: center;
        gap: var(--space-sm);
        padding: var(--space-sm);
        background: var(--bg-secondary);
        border-radius: var(--radius-sm);
      }
      
      .plugin-cpu-name {
        flex: 1;
        font-size: var(--font-size-sm);
        color: var(--text-primary);
      }
      
      .plugin-cpu-value {
        font-family: var(--font-mono);
        font-size: var(--font-size-sm);
        color: var(--fl-orange-primary);
        font-weight: var(--font-weight-semibold);
      }
      
      .plugin-cpu-bar {
        width: 100px;
        height: 4px;
        background: var(--bg-tertiary);
        border-radius: var(--radius-full);
        overflow: hidden;
      }
      
      .plugin-cpu-bar-fill {
        height: 100%;
        background: var(--fl-orange-primary);
        transition: width var(--transition-fast);
      }
      
      .plugin-cpu-empty {
        text-align: center;
        padding: var(--space-lg);
        color: var(--text-tertiary);
        font-size: var(--font-size-sm);
      }
      
      .recommendations-section {
        background: var(--bg-tertiary);
        border-radius: var(--radius-md);
        padding: var(--space-md);
      }
      
      .recommendation-item {
        display: flex;
        align-items: start;
        gap: var(--space-sm);
        padding: var(--space-sm);
        background: var(--bg-secondary);
        border-radius: var(--radius-sm);
        margin-bottom: var(--space-sm);
        border-left: 3px solid var(--fl-orange-primary);
      }
      
      .recommendation-icon {
        font-size: 18px;
      }
      
      .recommendation-text {
        flex: 1;
        font-size: var(--font-size-sm);
        color: var(--text-primary);
      }
    `;
    document.head.appendChild(style);
  }

  attachEventListeners() {
    const resetButton = this.container.querySelector('#reset-metrics');
    resetButton?.addEventListener('click', () => this.resetMetrics());
  }

  initializeCharts() {
    this.cpuChart = this.createChart('cpu-chart');
    this.memoryChart = this.createChart('memory-chart');
    this.polyphonyChart = this.createChart('polyphony-chart');
  }

  createChart(canvasId) {
    const canvas = this.container.querySelector(`#${canvasId}`);
    if (!canvas) return null;
    
    const ctx = canvas.getContext('2d');
    return { canvas, ctx };
  }

  drawChart(chart, data, color = '#FF8C42') {
    if (!chart) return;
    
    const { canvas, ctx } = chart;
    const width = canvas.width;
    const height = canvas.height;
    
    ctx.clearRect(0, 0, width, height);
    
    if (data.length < 2) return;
    
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    const step = width / (data.length - 1);
    const max = Math.max(...data, 1);
    
    data.forEach((value, i) => {
      const x = i * step;
      const y = height - (value / max) * height;
      
      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    
    ctx.stroke();
    
    // Fill gradient
    ctx.lineTo(width, height);
    ctx.lineTo(0, height);
    ctx.closePath();
    
    const gradient = ctx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, color + '40');
    gradient.addColorStop(1, color + '00');
    ctx.fillStyle = gradient;
    ctx.fill();
  }

  startMonitoring() {
    this.monitoringInterval = setInterval(() => {
      this.updateMetrics();
      this.updateUI();
      this.checkRecommendations();
    }, this.updateInterval);
  }

  stopMonitoring() {
    if (this.monitoringInterval) {
      clearInterval(this.monitoringInterval);
    }
  }

  updateMetrics() {
    // Simulate metrics (replace with actual audio engine calls)
    this.metrics.cpuUsage = Math.random() * 60 + 10; // 10-70%
    this.metrics.memoryUsage = Math.random() * 200 + 100; // 100-300 MB
    this.metrics.polyphony = Math.floor(Math.random() * 32); // 0-32 voices
    this.metrics.latency = (this.metrics.bufferSize / this.metrics.sampleRate) * 1000;
    
    // Add to history
    this.history.cpu.push(this.metrics.cpuUsage);
    this.history.memory.push(this.metrics.memoryUsage);
    this.history.polyphony.push(this.metrics.polyphony);
    
    // Trim history
    if (this.history.cpu.length > this.history.maxSamples) {
      this.history.cpu.shift();
      this.history.memory.shift();
      this.history.polyphony.shift();
    }
  }

  updateUI() {
    // CPU
    this.updateMetricDisplay('cpu', this.metrics.cpuUsage, '%');
    this.drawChart(this.cpuChart, this.history.cpu, '#FF8C42');
    
    // Memory
    this.updateMetricDisplay('memory', this.metrics.memoryUsage, ' MB');
    this.drawChart(this.memoryChart, this.history.memory, '#4ECDC4');
    
    // Latency
    const latencyEl = this.container.querySelector('#latency-value');
    if (latencyEl) latencyEl.textContent = this.metrics.latency.toFixed(1) + ' ms';
    
    // Polyphony
    const polyphonyEl = this.container.querySelector('#polyphony-value');
    if (polyphonyEl) {
      polyphonyEl.textContent = `${this.metrics.polyphony} / ${this.metrics.maxPolyphony}`;
    }
    const polyphonyBar = this.container.querySelector('#polyphony-bar');
    if (polyphonyBar) {
      const percentage = (this.metrics.polyphony / this.metrics.maxPolyphony) * 100;
      polyphonyBar.style.width = percentage + '%';
    }
    this.drawChart(this.polyphonyChart, this.history.polyphony, '#A29BFE');
    
    // Status
    this.updateStatus();
  }

  updateMetricDisplay(name, value, unit) {
    const valueEl = this.container.querySelector(`#${name}-value`);
    const barEl = this.container.querySelector(`#${name}-bar`);
    
    if (valueEl) {
      valueEl.textContent = value.toFixed(1) + unit;
    }
    
    if (barEl) {
      barEl.style.width = Math.min(value, 100) + '%';
    }
  }

  updateStatus() {
    const indicator = this.container.querySelector('#status-indicator');
    if (!indicator) return;
    
    const dot = indicator.querySelector('.status-dot');
    const text = indicator.querySelector('.status-text');
    
    if (this.metrics.cpuUsage > 80 || this.metrics.dropouts > 5) {
      dot.className = 'status-dot status-dot-bad';
      text.textContent = 'Critical';
    } else if (this.metrics.cpuUsage > 60 || this.metrics.dropouts > 0) {
      dot.className = 'status-dot status-dot-warning';
      text.textContent = 'Warning';
    } else {
      dot.className = 'status-dot status-dot-good';
      text.textContent = 'Excellent';
    }
  }

  checkRecommendations() {
    const recommendations = [];
    
    if (this.metrics.cpuUsage > 70) {
      recommendations.push({
        icon: '💻',
        text: 'High CPU usage detected. Consider freezing tracks or increasing buffer size.'
      });
    }
    
    if (this.metrics.latency > 20) {
      recommendations.push({
        icon: '⏱️',
        text: `High latency (${this.metrics.latency.toFixed(1)}ms). Decrease buffer size for better responsiveness.`
      });
    }
    
    if (this.metrics.polyphony > this.metrics.maxPolyphony * 0.8) {
      recommendations.push({
        icon: '🎵',
        text: 'Approaching maximum polyphony. Some notes may be dropped.'
      });
    }
    
    this.renderRecommendations(recommendations);
  }

  renderRecommendations(recommendations) {
    const section = this.container.querySelector('#recommendations');
    if (!section) return;
    
    if (recommendations.length === 0) {
      section.style.display = 'none';
      return;
    }
    
    section.style.display = 'block';
    section.innerHTML = `
      <h4 style="font-size: var(--font-size-md); font-weight: var(--font-weight-semibold); color: var(--text-primary); margin-bottom: var(--space-md);">
        💡 Recommendations
      </h4>
      ${recommendations.map(rec => `
        <div class="recommendation-item">
          <span class="recommendation-icon">${rec.icon}</span>
          <span class="recommendation-text">${rec.text}</span>
        </div>
      `).join('')}
    `;
  }

  resetMetrics() {
    this.metrics.dropouts = 0;
    this.metrics.lastDropout = null;
    this.history.cpu = [];
    this.history.memory = [];
    this.history.polyphony = [];
    
    this.updateUI();
  }

  // Public API for integration
  setPluginCPU(pluginName, cpuPercentage) {
    this.metrics.pluginCPU.set(pluginName, cpuPercentage);
    this.updatePluginList();
  }

  updatePluginList() {
    const listEl = this.container.querySelector('#plugin-cpu-list');
    if (!listEl) return;
    
    if (this.metrics.pluginCPU.size === 0) {
      listEl.innerHTML = '<div class="plugin-cpu-empty">No plugins loaded</div>';
      return;
    }
    
    const sorted = Array.from(this.metrics.pluginCPU.entries())
      .sort((a, b) => b[1] - a[1]);
    
    listEl.innerHTML = sorted.map(([name, cpu]) => `
      <div class="plugin-cpu-item">
        <span class="plugin-cpu-name">${name}</span>
        <div class="plugin-cpu-bar">
          <div class="plugin-cpu-bar-fill" style="width: ${Math.min(cpu, 100)}%"></div>
        </div>
        <span class="plugin-cpu-value">${cpu.toFixed(1)}%</span>
      </div>
    `).join('');
  }

  reportDropout() {
    this.metrics.dropouts++;
    this.metrics.lastDropout = new Date();
    
    const dropoutsEl = this.container.querySelector('#dropouts-value');
    const dropoutInfo = this.container.querySelector('#dropout-info');
    
    if (dropoutsEl) dropoutsEl.textContent = this.metrics.dropouts;
    if (dropoutInfo) {
      dropoutInfo.textContent = `Last: ${this.metrics.lastDropout.toLocaleTimeString()}`;
    }
    
    const card = this.container.querySelector('.metric-card-warning');
    if (card) {
      card.classList.add('metric-card-warning');
    }
  }

  destroy() {
    this.stopMonitoring();
  }
}

export default PerformanceDashboard;
