// 🎹 FRUTILLA STUDIO - Knob Component (FL Studio Style)
// =========================================================

export class FLKnob extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    
    // State
    this.value = parseFloat(this.getAttribute('value') || '0.5');
    this.min = parseFloat(this.getAttribute('min') || '0');
    this.max = parseFloat(this.getAttribute('max') || '1');
    this.step = parseFloat(this.getAttribute('step') || '0.01');
    this.label = this.getAttribute('label') || '';
    this.unit = this.getAttribute('unit') || '';
    this.sensitivity = parseFloat(this.getAttribute('sensitivity') || '1');
    
    // Interaction state
    this.isDragging = false;
    this.startY = 0;
    this.startValue = 0;
  }

  connectedCallback() {
    this.render();
    this.attachEventListeners();
  }

  render() {
    const percentage = ((this.value - this.min) / (this.max - this.min)) * 100;
    const angle = (percentage / 100) * 270 - 135; // -135° to +135°
    
    // SVG arc calculation
    const radius = 20;
    const circumference = 2 * Math.PI * radius;
    const arcLength = (270 / 360) * circumference;
    const offset = arcLength - (percentage / 100) * arcLength;

    const displayValue = this.formatValue(this.value);

    this.shadowRoot.innerHTML = `
      <style>
        @import url('/src/renderer/styles/design-system.css');
        
        :host {
          display: inline-block;
        }
        
        .knob-container {
          display: flex;
          flex-direction: column;
          align-items: center;
          gap: 4px;
        }
        
        .knob-wrapper {
          width: 56px;
          height: 56px;
          position: relative;
          cursor: grab;
          user-select: none;
          -webkit-user-select: none;
        }
        
        .knob-wrapper:active {
          cursor: grabbing;
        }
        
        .knob-svg {
          width: 100%;
          height: 100%;
          filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.3));
        }
        
        .knob-track {
          fill: none;
          stroke: var(--bg-tertiary, #3C3C3C);
          stroke-width: 4;
          stroke-linecap: round;
        }
        
        .knob-progress {
          fill: none;
          stroke: var(--fl-orange-primary, #FF8C42);
          stroke-width: 4;
          stroke-linecap: round;
          transition: stroke-dashoffset 50ms ease-out;
        }
        
        .knob-center {
          fill: var(--bg-secondary, #2D2D2D);
          stroke: var(--border-normal, #4A4A4A);
          stroke-width: 1.5;
        }
        
        .knob-indicator {
          fill: var(--fl-orange-primary, #FF8C42);
          transform-origin: center;
          transition: transform 50ms ease-out;
        }
        
        .knob-label {
          font-size: 11px;
          color: var(--text-secondary, #B3B3B3);
          font-weight: 500;
          text-align: center;
          white-space: nowrap;
        }
        
        .knob-value {
          font-size: 12px;
          color: var(--text-primary, #FFFFFF);
          font-weight: 600;
          font-family: 'SF Mono', Monaco, monospace;
          text-align: center;
        }
        
        .knob-wrapper:hover .knob-progress {
          stroke: var(--fl-orange-hover, #FFA15C);
        }
      </style>
      
      <div class="knob-container">
        <div class="knob-wrapper" id="knob">
          <svg class="knob-svg" viewBox="0 0 56 56">
            <!-- Background Track -->
            <circle
              class="knob-track"
              cx="28"
              cy="28"
              r="20"
              stroke-dasharray="${arcLength} ${circumference}"
              stroke-dashoffset="0"
              transform="rotate(-135 28 28)"
            />
            
            <!-- Progress Track -->
            <circle
              class="knob-progress"
              cx="28"
              cy="28"
              r="20"
              stroke-dasharray="${arcLength} ${circumference}"
              stroke-dashoffset="${offset}"
              transform="rotate(-135 28 28)"
            />
            
            <!-- Center Circle -->
            <circle class="knob-center" cx="28" cy="28" r="14" />
            
            <!-- Indicator Line -->
            <line
              class="knob-indicator"
              x1="28"
              y1="18"
              x2="28"
              y2="12"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              transform="rotate(${angle} 28 28)"
            />
          </svg>
        </div>
        
        ${this.label ? `<div class="knob-label">${this.label}</div>` : ''}
        <div class="knob-value">${displayValue}${this.unit}</div>
      </div>
    `;
  }

  formatValue(val) {
    if (Math.abs(val) >= 1000) {
      return (val / 1000).toFixed(1) + 'k';
    }
    
    const decimals = this.step < 0.1 ? 2 : (this.step < 1 ? 1 : 0);
    return val.toFixed(decimals);
  }

  attachEventListeners() {
    const knob = this.shadowRoot.getElementById('knob');
    
    knob.addEventListener('mousedown', this.onMouseDown.bind(this));
    knob.addEventListener('dblclick', this.onDoubleClick.bind(this));
    knob.addEventListener('wheel', this.onWheel.bind(this), { passive: false });
    
    // Touch support
    knob.addEventListener('touchstart', this.onTouchStart.bind(this), { passive: false });
  }

  onMouseDown(e) {
    e.preventDefault();
    this.isDragging = true;
    this.startY = e.clientY;
    this.startValue = this.value;
    
    const onMouseMove = (e) => {
      if (!this.isDragging) return;
      
      const deltaY = this.startY - e.clientY;
      const range = this.max - this.min;
      const sensitivity = this.sensitivity * (e.shiftKey ? 0.1 : 1);
      const delta = (deltaY / 100) * range * sensitivity;
      
      this.setValue(this.startValue + delta);
    };
    
    const onMouseUp = () => {
      this.isDragging = false;
      document.removeEventListener('mousemove', onMouseMove);
      document.removeEventListener('mouseup', onMouseUp);
      this.dispatchChangeEvent();
    };
    
    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup', onMouseUp);
  }

  onTouchStart(e) {
    e.preventDefault();
    this.isDragging = true;
    this.startY = e.touches[0].clientY;
    this.startValue = this.value;
    
    const onTouchMove = (e) => {
      if (!this.isDragging) return;
      
      const deltaY = this.startY - e.touches[0].clientY;
      const range = this.max - this.min;
      const delta = (deltaY / 100) * range * this.sensitivity;
      
      this.setValue(this.startValue + delta);
    };
    
    const onTouchEnd = () => {
      this.isDragging = false;
      document.removeEventListener('touchmove', onTouchMove);
      document.removeEventListener('touchend', onTouchEnd);
      this.dispatchChangeEvent();
    };
    
    document.addEventListener('touchmove', onTouchMove, { passive: false });
    document.addEventListener('touchend', onTouchEnd);
  }

  onWheel(e) {
    e.preventDefault();
    const delta = -Math.sign(e.deltaY) * this.step * (e.shiftKey ? 0.1 : 1);
    this.setValue(this.value + delta);
    this.dispatchChangeEvent();
  }

  onDoubleClick() {
    const defaultValue = parseFloat(this.getAttribute('default') || '0.5');
    this.setValue(defaultValue);
    this.dispatchChangeEvent();
  }

  setValue(newValue) {
    // Clamp and snap to step
    let clamped = Math.max(this.min, Math.min(this.max, newValue));
    clamped = Math.round(clamped / this.step) * this.step;
    
    if (clamped !== this.value) {
      this.value = clamped;
      this.render();
      
      // Dispatch input event for real-time updates
      this.dispatchEvent(new CustomEvent('input', {
        detail: { value: this.value },
        bubbles: true,
      }));
    }
  }

  dispatchChangeEvent() {
    this.dispatchEvent(new CustomEvent('change', {
      detail: { value: this.value },
      bubbles: true,
    }));
  }

  // Attribute observation
  static get observedAttributes() {
    return ['value', 'min', 'max', 'label', 'unit'];
  }

  attributeChangedCallback(name, oldValue, newValue) {
    if (oldValue === newValue) return;
    
    switch (name) {
      case 'value':
        this.value = parseFloat(newValue);
        break;
      case 'min':
        this.min = parseFloat(newValue);
        break;
      case 'max':
        this.max = parseFloat(newValue);
        break;
      case 'label':
        this.label = newValue;
        break;
      case 'unit':
        this.unit = newValue;
        break;
    }
    
    if (this.shadowRoot) {
      this.render();
    }
  }
}

// Register custom element
customElements.define('fl-knob', FLKnob);
