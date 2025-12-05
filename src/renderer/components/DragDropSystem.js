// 🎨 FRUTILLA STUDIO - Visual Feedback & Drag/Drop System
// =========================================================

export class DragDropManager {
  constructor() {
    this.dragData = null;
    this.ghostElement = null;
    this.dropZones = new Map();
    this.snapGuides = [];
    this.snapThreshold = 8; // pixels
    
    this.attachGlobalListeners();
    this.createSnapGuidesContainer();
  }

  // === DRAG OPERATIONS ===
  
  makeDraggable(element, options = {}) {
    element.setAttribute('draggable', 'true');
    element.style.cursor = 'grab';
    
    element.addEventListener('dragstart', (e) => {
      e.stopPropagation();
      element.style.cursor = 'grabbing';
      
      this.dragData = {
        type: options.type || 'generic',
        data: options.data || {},
        sourceElement: element,
        ghostHTML: options.ghostHTML || element.innerHTML,
        ghostStyle: options.ghostStyle || {}
      };
      
      // Create ghost element
      this.createGhost(e, element, options);
      
      // Add dragging class
      element.classList.add('fl-dragging');
      
      // Callback
      if (options.onStart) options.onStart(e, this.dragData);
    });
    
    element.addEventListener('drag', (e) => {
      if (this.ghostElement) {
        this.updateGhostPosition(e);
      }
      
      // Show snap guides
      this.updateSnapGuides(e);
    });
    
    element.addEventListener('dragend', (e) => {
      element.style.cursor = 'grab';
      element.classList.remove('fl-dragging');
      
      this.removeGhost();
      this.hideSnapGuides();
      
      if (options.onEnd) options.onEnd(e, this.dragData);
      
      this.dragData = null;
    });
  }

  createGhost(e, element, options) {
    this.ghostElement = document.createElement('div');
    this.ghostElement.className = 'fl-drag-ghost';
    this.ghostElement.innerHTML = this.dragData.ghostHTML;
    
    // Default ghost styles
    Object.assign(this.ghostElement.style, {
      position: 'fixed',
      pointerEvents: 'none',
      zIndex: '10000',
      opacity: '0.8',
      transform: 'translate(-50%, -50%) scale(0.95)',
      transition: 'transform 100ms ease-out',
      background: 'var(--bg-elevated)',
      border: '2px solid var(--fl-orange-primary)',
      borderRadius: 'var(--radius-md)',
      padding: 'var(--space-sm)',
      boxShadow: 'var(--shadow-xl)',
      color: 'var(--text-primary)',
      fontSize: 'var(--font-size-sm)',
      fontWeight: 'var(--font-weight-medium)',
      ...this.dragData.ghostStyle
    });
    
    document.body.appendChild(this.ghostElement);
    this.updateGhostPosition(e);
    
    // Animate in
    requestAnimationFrame(() => {
      this.ghostElement.style.transform = 'translate(-50%, -50%) scale(1)';
    });
  }

  updateGhostPosition(e) {
    if (!this.ghostElement || e.clientX === 0 && e.clientY === 0) return;
    
    this.ghostElement.style.left = e.clientX + 'px';
    this.ghostElement.style.top = e.clientY + 'px';
  }

  removeGhost() {
    if (this.ghostElement) {
      this.ghostElement.style.transform = 'translate(-50%, -50%) scale(0.8)';
      this.ghostElement.style.opacity = '0';
      
      setTimeout(() => {
        this.ghostElement?.remove();
        this.ghostElement = null;
      }, 100);
    }
  }

  // === DROP ZONES ===
  
  registerDropZone(element, options = {}) {
    const id = options.id || `dropzone-${Date.now()}-${Math.random()}`;
    
    this.dropZones.set(id, {
      element,
      accepts: options.accepts || ['*'],
      onDragEnter: options.onDragEnter,
      onDragOver: options.onDragOver,
      onDragLeave: options.onDragLeave,
      onDrop: options.onDrop
    });
    
    element.setAttribute('data-dropzone-id', id);
    
    element.addEventListener('dragenter', (e) => {
      e.preventDefault();
      
      if (this.canAcceptDrop(id)) {
        element.classList.add('fl-drop-target');
        
        const zone = this.dropZones.get(id);
        if (zone.onDragEnter) zone.onDragEnter(e, this.dragData);
      }
    });
    
    element.addEventListener('dragover', (e) => {
      e.preventDefault();
      
      if (this.canAcceptDrop(id)) {
        e.dataTransfer.dropEffect = 'move';
        
        const zone = this.dropZones.get(id);
        if (zone.onDragOver) zone.onDragOver(e, this.dragData);
      } else {
        e.dataTransfer.dropEffect = 'none';
      }
    });
    
    element.addEventListener('dragleave', (e) => {
      if (e.target === element) {
        element.classList.remove('fl-drop-target');
        
        const zone = this.dropZones.get(id);
        if (zone.onDragLeave) zone.onDragLeave(e, this.dragData);
      }
    });
    
    element.addEventListener('drop', (e) => {
      e.preventDefault();
      e.stopPropagation();
      
      element.classList.remove('fl-drop-target');
      
      if (this.canAcceptDrop(id)) {
        const zone = this.dropZones.get(id);
        if (zone.onDrop) zone.onDrop(e, this.dragData);
        
        // Visual feedback
        this.showDropFeedback(element);
      }
    });
    
    return id;
  }

  unregisterDropZone(id) {
    const zone = this.dropZones.get(id);
    if (zone) {
      zone.element.removeAttribute('data-dropzone-id');
      this.dropZones.delete(id);
    }
  }

  canAcceptDrop(zoneId) {
    if (!this.dragData) return false;
    
    const zone = this.dropZones.get(zoneId);
    if (!zone) return false;
    
    return zone.accepts.includes('*') || zone.accepts.includes(this.dragData.type);
  }

  showDropFeedback(element) {
    const feedback = document.createElement('div');
    feedback.className = 'fl-drop-feedback';
    feedback.style.cssText = `
      position: absolute;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: var(--fl-orange-primary);
      opacity: 0.3;
      pointer-events: none;
      border-radius: inherit;
      animation: dropPulse 400ms ease-out;
    `;
    
    element.style.position = 'relative';
    element.appendChild(feedback);
    
    setTimeout(() => feedback.remove(), 400);
  }

  // === SNAP GUIDES ===
  
  createSnapGuidesContainer() {
    this.snapGuidesContainer = document.createElement('div');
    this.snapGuidesContainer.id = 'snap-guides-container';
    this.snapGuidesContainer.style.cssText = `
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      pointer-events: none;
      z-index: 9999;
    `;
    document.body.appendChild(this.snapGuidesContainer);
  }

  addSnapPoint(x, y, type = 'both') {
    this.snapGuides.push({ x, y, type });
  }

  clearSnapPoints() {
    this.snapGuides = [];
  }

  updateSnapGuides(e) {
    if (this.snapGuides.length === 0) return;
    
    const mouseX = e.clientX;
    const mouseY = e.clientY;
    
    // Clear existing guides
    this.snapGuidesContainer.innerHTML = '';
    
    // Check for snapping
    let snappedX = null;
    let snappedY = null;
    
    for (const guide of this.snapGuides) {
      if (guide.type === 'vertical' || guide.type === 'both') {
        if (Math.abs(mouseX - guide.x) < this.snapThreshold) {
          snappedX = guide.x;
        }
      }
      
      if (guide.type === 'horizontal' || guide.type === 'both') {
        if (Math.abs(mouseY - guide.y) < this.snapThreshold) {
          snappedY = guide.y;
        }
      }
    }
    
    // Draw snap guides
    if (snappedX !== null) {
      const guide = document.createElement('div');
      guide.style.cssText = `
        position: absolute;
        left: ${snappedX}px;
        top: 0;
        bottom: 0;
        width: 1px;
        background: var(--fl-orange-primary);
        box-shadow: 0 0 4px var(--fl-orange-primary);
      `;
      this.snapGuidesContainer.appendChild(guide);
    }
    
    if (snappedY !== null) {
      const guide = document.createElement('div');
      guide.style.cssText = `
        position: absolute;
        top: ${snappedY}px;
        left: 0;
        right: 0;
        height: 1px;
        background: var(--fl-orange-primary);
        box-shadow: 0 0 4px var(--fl-orange-primary);
      `;
      this.snapGuidesContainer.appendChild(guide);
    }
  }

  hideSnapGuides() {
    if (this.snapGuidesContainer) {
      this.snapGuidesContainer.innerHTML = '';
    }
  }

  // === FILE DROP HANDLING ===
  
  attachGlobalListeners() {
    // Prevent default drag behavior on document
    document.addEventListener('dragover', (e) => {
      e.preventDefault();
    });
    
    document.addEventListener('drop', (e) => {
      e.preventDefault();
    });
  }

  enableFileDrop(element, callback) {
    element.addEventListener('dragover', (e) => {
      e.preventDefault();
      e.stopPropagation();
      element.classList.add('fl-file-drop-target');
    });
    
    element.addEventListener('dragleave', (e) => {
      if (e.target === element) {
        element.classList.remove('fl-file-drop-target');
      }
    });
    
    element.addEventListener('drop', (e) => {
      e.preventDefault();
      e.stopPropagation();
      element.classList.remove('fl-file-drop-target');
      
      const files = Array.from(e.dataTransfer.files);
      if (files.length > 0 && callback) {
        callback(files);
        this.showDropFeedback(element);
      }
    });
  }
}

// === HOVER EFFECTS MANAGER ===

export class HoverEffectsManager {
  constructor() {
    this.activeTooltip = null;
    this.tooltipTimeout = null;
    this.tooltipDelay = 500; // ms
  }

  enableTooltip(element, content, options = {}) {
    let timeout = null;
    
    element.addEventListener('mouseenter', (e) => {
      timeout = setTimeout(() => {
        this.showTooltip(e, content, options);
      }, options.delay || this.tooltipDelay);
    });
    
    element.addEventListener('mouseleave', () => {
      clearTimeout(timeout);
      this.hideTooltip();
    });
    
    element.addEventListener('mousemove', (e) => {
      if (this.activeTooltip && !options.fixed) {
        this.updateTooltipPosition(e, options);
      }
    });
  }

  showTooltip(e, content, options = {}) {
    this.hideTooltip();
    
    const tooltip = document.createElement('div');
    tooltip.className = 'fl-tooltip';
    tooltip.innerHTML = content;
    
    if (options.position === 'top') {
      tooltip.style.bottom = '100%';
      tooltip.style.marginBottom = '8px';
    }
    
    document.body.appendChild(tooltip);
    this.activeTooltip = tooltip;
    
    this.updateTooltipPosition(e, options);
  }

  updateTooltipPosition(e, options = {}) {
    if (!this.activeTooltip) return;
    
    const offset = options.offset || { x: 10, y: 10 };
    
    this.activeTooltip.style.left = (e.clientX + offset.x) + 'px';
    this.activeTooltip.style.top = (e.clientY + offset.y) + 'px';
  }

  hideTooltip() {
    if (this.activeTooltip) {
      this.activeTooltip.remove();
      this.activeTooltip = null;
    }
  }

  addRippleEffect(element) {
    element.addEventListener('click', (e) => {
      const ripple = document.createElement('span');
      ripple.className = 'fl-ripple';
      
      const rect = element.getBoundingClientRect();
      const size = Math.max(rect.width, rect.height);
      const x = e.clientX - rect.left - size / 2;
      const y = e.clientY - rect.top - size / 2;
      
      ripple.style.cssText = `
        position: absolute;
        width: ${size}px;
        height: ${size}px;
        left: ${x}px;
        top: ${y}px;
        background: rgba(255, 255, 255, 0.3);
        border-radius: 50%;
        transform: scale(0);
        animation: ripple 600ms ease-out;
        pointer-events: none;
      `;
      
      element.style.position = 'relative';
      element.style.overflow = 'hidden';
      element.appendChild(ripple);
      
      setTimeout(() => ripple.remove(), 600);
    });
  }
}

// Create global instances
window.dragDrop = new DragDropManager();
window.hoverEffects = new HoverEffectsManager();

// Add animations to stylesheet
const style = document.createElement('style');
style.textContent = `
  .fl-dragging {
    opacity: 0.5;
    transform: scale(0.95);
    transition: opacity 100ms, transform 100ms;
  }
  
  .fl-drop-target {
    background: rgba(255, 140, 66, 0.1) !important;
    border: 2px dashed var(--fl-orange-primary) !important;
    transition: all 150ms ease-out;
  }
  
  .fl-file-drop-target {
    background: rgba(255, 140, 66, 0.15) !important;
    border: 2px dashed var(--fl-orange-primary) !important;
    box-shadow: inset 0 0 20px rgba(255, 140, 66, 0.2);
  }
  
  @keyframes dropPulse {
    0% {
      opacity: 0;
      transform: scale(0.95);
    }
    50% {
      opacity: 0.5;
    }
    100% {
      opacity: 0;
      transform: scale(1);
    }
  }
  
  @keyframes ripple {
    to {
      transform: scale(4);
      opacity: 0;
    }
  }
  
  /* Smooth transitions */
  .fl-smooth-hover {
    transition: all 150ms cubic-bezier(0.4, 0, 0.2, 1);
  }
  
  .fl-smooth-hover:hover {
    transform: translateY(-2px);
    box-shadow: var(--shadow-md);
  }
  
  .fl-smooth-hover:active {
    transform: translateY(0);
    box-shadow: var(--shadow-sm);
  }
`;
document.head.appendChild(style);

export { DragDropManager, HoverEffectsManager };
