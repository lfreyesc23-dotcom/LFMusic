// 🎚️ FRUTILLA STUDIO - Fader Component (FL Studio Mixer Style)
// ================================================================

export class FLFader extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    
    // State
    this.value = parseFloat(this.getAttribute('value') || '0.75');
    this.min = parseFloat(this.getAttribute('min') || '0');
    this.max = parseFloat(this.getAttribute('max') || '1');
    this.step = parseFloat(this.getAttribute('step') || '0.01');
    this.label = this.getAttribute('label') || '';
    this.dbMode = this.hasAttribute('db-mode');
    
    // Interaction
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
    const displayValue = this.dbMode ? this.toDb(this.value) : this.value.toFixed(2);

    this.shadowRoot.innerHTML = `
      <style>
        @import url('/src/renderer/styles/design-system.css');
        
        :host {
          display: inline-block;
        }
        
        .fader-container {
          display: flex;
          flex-direction: column;
          align-items: center;
          gap: 8px;
          height: 100%;
          min-height: 200px;
        }
        
        .fader-wrapper {
          flex: 1;
          width: 36px;
          position: relative;
          background: var(--bg-tertiary, #3C3C3C);
          border-radius: 8px;
          cursor: ns-resize;
          user-select: none;
          -webkit-user-select: none;
          box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.3);
        }
        
        .fader-track {
          position: absolute;
          left: 50%;
          top: 8px;
          bottom: 8px;
          width: 4px;
          transform: translateX(-50%);
          background: var(--bg-secondary, #2D2D2D);
          border-radius: 2px;
        }
        
        .fader-fill {
          position: absolute;
          left: 0;
          right: 0;
          bottom: 0;
          background: linear-gradient(
            to top,
            var(--fl-orange-primary, #FF8C42) 0%,
            var(--fl-orange-hover, #FFA15C) 100%
          );
          border-radius: 2px;
          transition: height 50ms ease-out;
          box-shadow: 0 0 8px rgba(255, 140, 66, 0.4);
        }
        
        .fader-thumb {
          position: absolute;
          left: 50%;
          width: 32px;
          height: 12px;
          background: linear-gradient(to bottom, #4A4A4A, #3C3C3C);
          border: 2px solid var(--fl-orange-primary, #FF8C42);
          border-radius: 4px;
          transform: translateX(-50%);
          box-shadow: 
            0 2px 4px rgba(0, 0, 0, 0.4),
            inset 0 1px 0 rgba(255, 255, 255, 0.1);
          transition: transform 50ms ease-out, border-color 100ms ease-out;
          cursor: grab;
        }
        
        .fader-thumb:active {
          cursor: grabbing;
        }
        
        .fader-wrapper:hover .fader-thumb {
          transform: translateX(-50%) scale(1.05);
          border-color: var(--fl-orange-hover, #FFA15C);
        }
        
        .fader-marks {
          position: absolute;
          right: 4px;
          top: 8px;
          bottom: 8px;
          width: 8px;
          display: flex;
          flex-direction: column;
          justify-content: space-between;
        }
        
        .fader-mark {
          height: 1px;
          width: 4px;
          background: var(--text-tertiary, #808080);
          opacity: 0.5;
        }
        
        .fader-mark.major {
          width: 8px;
          height: 2px;
          opacity: 0.7;
        }
        
        .fader-label {
          font-size: 11px;
          color: var(--text-secondary, #B3B3B3);
          font-weight: 500;
          text-align: center;
          white-space: nowrap;
        }
        
        .fader-value {
          font-size: 12px;
          color: var(--text-primary, #FFFFFF);
          font-weight: 600;
          font-family: 'SF Mono', Monaco, monospace;
          text-align: center;
          min-width: 48px;
        }
        
        .fader-unity {
          position: absolute;
          left: -4px;
          right: -4px;
          height: 2px;
          background: var(--fl-green-success, #2ECC71);
          opacity: 0.6;
          pointer-events: none;
        }
      </style>
      
      <div class="fader-container">
        <div class="fader-wrapper" id="fader">
          <!-- Track -->
          <div class="fader-track">
            <div class="fader-fill" style="height: ${percentage}%"></div>
          </div>
          
          <!-- Scale marks -->
          <div class="fader-marks">
            <div class="fader-mark major"></div>
            <div class="fader-mark"></div>
            <div class="fader-mark"></div>
            <div class="fader-mark major"></div>
            <div class="fader-mark"></div>
            <div class="fader-mark"></div>
            <div class="fader-mark major"></div>
          </div>
          
          <!-- Unity gain marker (0dB) -->
          ${this.dbMode ? '<div class="fader-unity" style="top: 20%"></div>' : ''}
          
          <!-- Thumb -->
          <div class="fader-thumb" style="top: ${100 - percentage}%"></div>
        </div>
        
        ${this.label ? `<div class="fader-label">${this.label}</div>` : ''}
        <div class="fader-value">${displayValue}${this.dbMode ? ' dB' : ''}</div>
      </div>
    `;
  }

  toDb(linearValue) {
    if (linearValue <= 0) return '-∞';
    const db = 20 * Math.log10(linearValue);
    return db.toFixed(1);
  }

  fromDb(dbValue) {
    return Math.pow(10, dbValue / 20);
  }

  attachEventListeners() {
    const fader = this.shadowRoot.getElementById('fader');
    
    fader.addEventListener('mousedown', this.onMouseDown.bind(this));
    fader.addEventListener('dblclick', this.onDoubleClick.bind(this));
    fader.addEventListener('wheel', this.onWheel.bind(this), { passive: false });
    
    // Touch support
    fader.addEventListener('touchstart', this.onTouchStart.bind(this), { passive: false });
  }

  onMouseDown(e) {
    e.preventDefault();
    
    // Check if clicking on the track directly
    const faderRect = this.shadowRoot.getElementById('fader').getBoundingClientRect();
    const clickY = e.clientY - faderRect.top;
    const faderHeight = faderRect.height;
    const clickPercentage = 1 - (clickY / faderHeight);
    
    // If clicking far from thumb, jump to position
    const currentPercentage = (this.value - this.min) / (this.max - this.min);
    if (Math.abs(clickPercentage - currentPercentage) > 0.05) {
      const newValue = this.min + clickPercentage * (this.max - this.min);
      this.setValue(newValue);
      this.dispatchChangeEvent();
    }
    
    this.isDragging = true;
    this.startY = e.clientY;
    this.startValue = this.value;
    
    const onMouseMove = (e) => {
      if (!this.isDragging) return;
      
      const deltaY = this.startY - e.clientY;
      const range = this.max - this.min;
      const sensitivity = e.shiftKey ? 0.1 : 1;
      const delta = (deltaY / faderRect.height) * range * sensitivity;
      
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
    
    const faderRect = this.shadowRoot.getElementById('fader').getBoundingClientRect();
    
    const onTouchMove = (e) => {
      if (!this.isDragging) return;
      
      const deltaY = this.startY - e.touches[0].clientY;
      const range = this.max - this.min;
      const delta = (deltaY / faderRect.height) * range;
      
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
    // Reset to unity gain (0.75 = 0dB in typical scaling)
    const defaultValue = this.dbMode ? 0.75 : parseFloat(this.getAttribute('default') || '0.75');
    this.setValue(defaultValue);
    this.dispatchChangeEvent();
  }

  setValue(newValue) {
    let clamped = Math.max(this.min, Math.min(this.max, newValue));
    clamped = Math.round(clamped / this.step) * this.step;
    
    if (clamped !== this.value) {
      this.value = clamped;
      this.render();
      
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

  static get observedAttributes() {
    return ['value', 'min', 'max', 'label', 'db-mode'];
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
      case 'db-mode':
        this.dbMode = newValue !== null;
        break;
    }
    
    if (this.shadowRoot) {
      this.render();
    }
  }
}

customElements.define('fl-fader', FLFader);
