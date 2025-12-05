// 🎓 FRUTILLA STUDIO - Onboarding & Tutorial System
// ==================================================

export class OnboardingManager {
  constructor() {
    this.isFirstRun = this.checkFirstRun();
    this.currentStep = 0;
    this.totalSteps = 0;
    this.tutorialActive = false;
    this.mode = localStorage.getItem('frutilla_ui_mode') || 'simple'; // simple or advanced
    
    this.tours = new Map();
    this.initializeDefaultTours();
  }

  checkFirstRun() {
    const hasRun = localStorage.getItem('frutilla_first_run');
    if (!hasRun) {
      localStorage.setItem('frutilla_first_run', 'true');
      return true;
    }
    return false;
  }

  initializeDefaultTours() {
    // Welcome Tour
    this.registerTour('welcome', {
      title: '👋 Welcome to Frutilla Studio',
      steps: [
        {
          target: null,
          title: 'Welcome!',
          content: `
            <h3>Welcome to Frutilla Studio!</h3>
            <p>Let's take a quick tour to get you started.</p>
            <p><strong>Choose your experience level:</strong></p>
            <div style="display: flex; gap: 12px; margin-top: 16px;">
              <button class="fl-button fl-button-primary" data-mode="simple">
                🌱 Simple Mode
                <div style="font-size: 11px; opacity: 0.8;">Perfect for beginners</div>
              </button>
              <button class="fl-button fl-button-secondary" data-mode="advanced">
                🚀 Advanced Mode
                <div style="font-size: 11px; opacity: 0.8;">Full power user interface</div>
              </button>
            </div>
          `,
          position: 'center',
          onShow: (modal) => {
            modal.querySelectorAll('[data-mode]').forEach(btn => {
              btn.addEventListener('click', (e) => {
                this.setMode(e.currentTarget.dataset.mode);
                this.nextStep();
              });
            });
          }
        },
        {
          target: '#toolbar',
          title: 'Toolbar',
          content: 'Access all main functions here. Quick access to transport controls, save/load, and settings.',
          position: 'bottom'
        },
        {
          target: '#transport-bar',
          title: 'Transport Controls',
          content: `
            Control playback here:
            <ul>
              <li>⏯️ <strong>Space</strong> - Play/Pause</li>
              <li>⏺️ <strong>Numpad *</strong> - Record</li>
              <li>⏹️ <strong>Numpad /</strong> - Stop</li>
            </ul>
          `,
          position: 'bottom'
        },
        {
          target: '#window-switcher',
          title: 'Window Switcher',
          content: `
            Switch between main views (FL Studio style):
            <ul>
              <li><strong>F5</strong> - Playlist</li>
              <li><strong>F6</strong> - Channel Rack</li>
              <li><strong>F7</strong> - Piano Roll</li>
              <li><strong>F9</strong> - Mixer</li>
            </ul>
          `,
          position: 'bottom'
        },
        {
          target: '#browser-panel',
          title: 'Browser',
          content: 'Browse sounds, presets, and plugins. Press <strong>Ctrl+B</strong> to toggle. Drag items directly into your project!',
          position: 'right'
        },
        {
          target: null,
          title: '🎉 Ready to Create!',
          content: `
            <h3>You're all set!</h3>
            <p>Here are some tips to get started:</p>
            <ul>
              <li>📁 Use <strong>Ctrl+N</strong> for new project</li>
              <li>❓ Press <strong>Ctrl+/</strong> to see all shortcuts</li>
              <li>🎵 Check out our templates for inspiration</li>
            </ul>
            <p style="margin-top: 16px;">
              <a href="#" id="show-templates">Browse Templates</a> |
              <a href="#" id="skip-tour">Start from scratch</a>
            </p>
          `,
          position: 'center',
          onShow: (modal) => {
            modal.querySelector('#show-templates')?.addEventListener('click', (e) => {
              e.preventDefault();
              this.completeTour();
              // Show template browser
              document.dispatchEvent(new CustomEvent('frutilla:showTemplates'));
            });
            modal.querySelector('#skip-tour')?.addEventListener('click', (e) => {
              e.preventDefault();
              this.completeTour();
            });
          }
        }
      ]
    });

    // Piano Roll Tour
    this.registerTour('pianoRoll', {
      title: '🎹 Piano Roll Guide',
      steps: [
        {
          target: '#piano-roll',
          title: 'Piano Roll',
          content: 'This is where you create and edit melodies. Click to add notes, drag to move them.',
          position: 'top'
        },
        {
          target: '#piano-roll-toolbar',
          title: 'Tools',
          content: 'Select different tools: Pencil, Paint, Delete, and more. Shortcuts: <strong>B</strong> (Brush), <strong>D</strong> (Delete), <strong>S</strong> (Slice)',
          position: 'bottom'
        },
        {
          target: '#piano-roll-scale',
          title: 'Scale Highlighting',
          content: 'Enable scale highlighting to stay in key. Choose from Major, Minor, Pentatonic, and more!',
          position: 'right'
        },
        {
          target: '#piano-roll-tools',
          title: 'Quick Actions',
          content: `
            Powerful tools at your fingertips:
            <ul>
              <li><strong>Alt+A</strong> - Arpeggiate</li>
              <li><strong>Alt+C</strong> - Chordize</li>
              <li><strong>Alt+R</strong> - Randomize</li>
              <li><strong>Ctrl+Q</strong> - Quantize</li>
            </ul>
          `,
          position: 'left'
        }
      ]
    });

    // Mixer Tour
    this.registerTour('mixer', {
      title: '🎚️ Mixer Guide',
      steps: [
        {
          target: '#mixer',
          title: 'Mixer',
          content: 'Mix and balance your tracks. Each channel has volume, pan, effects, and routing.',
          position: 'top'
        },
        {
          target: '.mixer-channel:first-child',
          title: 'Mixer Channel',
          content: 'Control volume with the fader, pan left/right, and add effects. Right-click for more options!',
          position: 'top'
        },
        {
          target: '#mixer-sends',
          title: 'Send FX',
          content: 'Route audio to reverb, delay, and other effects. Perfect for creating space and depth.',
          position: 'right'
        },
        {
          target: '#master-channel',
          title: 'Master Channel',
          content: 'All tracks feed into the master. Add limiters and mastering effects here.',
          position: 'top'
        }
      ]
    });
  }

  registerTour(id, config) {
    this.tours.set(id, config);
  }

  startTour(tourId) {
    const tour = this.tours.get(tourId);
    if (!tour) {
      console.error(`Tour "${tourId}" not found`);
      return;
    }

    this.currentTour = tour;
    this.currentStep = 0;
    this.totalSteps = tour.steps.length;
    this.tutorialActive = true;

    this.showStep();
  }

  showStep() {
    const step = this.currentTour.steps[this.currentStep];
    
    // Create modal
    const modal = this.createTutorialModal(step);
    
    // Position modal
    if (step.target) {
      this.positionModalNearTarget(modal, step.target, step.position);
    } else {
      this.positionModalCenter(modal);
    }
    
    // Highlight target
    if (step.target) {
      this.highlightElement(step.target);
    }
    
    // Callback
    if (step.onShow) {
      step.onShow(modal);
    }
  }

  createTutorialModal(step) {
    // Remove existing modal
    document.getElementById('tutorial-modal')?.remove();
    document.getElementById('tutorial-overlay')?.remove();
    document.getElementById('tutorial-highlight')?.remove();

    // Create overlay
    const overlay = document.createElement('div');
    overlay.id = 'tutorial-overlay';
    overlay.style.cssText = `
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: rgba(0, 0, 0, 0.7);
      z-index: 9998;
      animation: fadeIn 200ms ease-out;
    `;
    document.body.appendChild(overlay);

    // Create modal
    const modal = document.createElement('div');
    modal.id = 'tutorial-modal';
    modal.className = 'fl-modal';
    modal.style.cssText = `
      position: fixed;
      z-index: 9999;
      max-width: 400px;
      animation: slideInUp 200ms ease-out;
    `;

    const progress = this.totalSteps > 1 
      ? `<div style="font-size: 11px; color: var(--text-tertiary); margin-top: 8px;">
           Step ${this.currentStep + 1} of ${this.totalSteps}
         </div>`
      : '';

    modal.innerHTML = `
      <div class="fl-modal-header">
        <div>
          <h3 class="fl-modal-title">${step.title}</h3>
          ${progress}
        </div>
        <button class="fl-button fl-button-ghost" id="tutorial-close">✕</button>
      </div>
      <div class="fl-modal-body">
        ${step.content}
      </div>
      <div class="fl-modal-footer" style="justify-content: space-between;">
        <button class="fl-button fl-button-ghost" id="tutorial-skip">
          Skip Tutorial
        </button>
        <div style="display: flex; gap: 8px;">
          ${this.currentStep > 0 
            ? '<button class="fl-button fl-button-secondary" id="tutorial-prev">← Previous</button>'
            : ''}
          ${this.currentStep < this.totalSteps - 1
            ? '<button class="fl-button fl-button-primary" id="tutorial-next">Next →</button>'
            : '<button class="fl-button fl-button-primary" id="tutorial-finish">Finish ✓</button>'}
        </div>
      </div>
    `;

    document.body.appendChild(modal);

    // Event listeners
    modal.querySelector('#tutorial-close').addEventListener('click', () => this.closeTour());
    modal.querySelector('#tutorial-skip').addEventListener('click', () => this.closeTour());
    modal.querySelector('#tutorial-prev')?.addEventListener('click', () => this.prevStep());
    modal.querySelector('#tutorial-next')?.addEventListener('click', () => this.nextStep());
    modal.querySelector('#tutorial-finish')?.addEventListener('click', () => this.completeTour());

    return modal;
  }

  positionModalCenter(modal) {
    modal.style.left = '50%';
    modal.style.top = '50%';
    modal.style.transform = 'translate(-50%, -50%)';
  }

  positionModalNearTarget(modal, targetSelector, position = 'bottom') {
    const target = document.querySelector(targetSelector);
    if (!target) {
      this.positionModalCenter(modal);
      return;
    }

    const rect = target.getBoundingClientRect();
    const modalRect = modal.getBoundingClientRect();
    const gap = 16;

    let left, top;

    switch (position) {
      case 'top':
        left = rect.left + rect.width / 2 - modalRect.width / 2;
        top = rect.top - modalRect.height - gap;
        break;
      case 'bottom':
        left = rect.left + rect.width / 2 - modalRect.width / 2;
        top = rect.bottom + gap;
        break;
      case 'left':
        left = rect.left - modalRect.width - gap;
        top = rect.top + rect.height / 2 - modalRect.height / 2;
        break;
      case 'right':
        left = rect.right + gap;
        top = rect.top + rect.height / 2 - modalRect.height / 2;
        break;
      default:
        left = rect.left + rect.width / 2 - modalRect.width / 2;
        top = rect.bottom + gap;
    }

    // Clamp to viewport
    left = Math.max(gap, Math.min(left, window.innerWidth - modalRect.width - gap));
    top = Math.max(gap, Math.min(top, window.innerHeight - modalRect.height - gap));

    modal.style.left = left + 'px';
    modal.style.top = top + 'px';
    modal.style.transform = 'none';
  }

  highlightElement(selector) {
    const target = document.querySelector(selector);
    if (!target) return;

    const highlight = document.createElement('div');
    highlight.id = 'tutorial-highlight';
    
    const rect = target.getBoundingClientRect();
    
    highlight.style.cssText = `
      position: fixed;
      left: ${rect.left - 4}px;
      top: ${rect.top - 4}px;
      width: ${rect.width + 8}px;
      height: ${rect.height + 8}px;
      border: 2px solid var(--fl-orange-primary);
      border-radius: 8px;
      box-shadow: 0 0 0 4px rgba(255, 140, 66, 0.2),
                  0 0 20px rgba(255, 140, 66, 0.4);
      z-index: 9997;
      pointer-events: none;
      animation: pulse 2s ease-in-out infinite;
    `;
    
    document.body.appendChild(highlight);
  }

  nextStep() {
    if (this.currentStep < this.totalSteps - 1) {
      this.currentStep++;
      this.showStep();
    }
  }

  prevStep() {
    if (this.currentStep > 0) {
      this.currentStep--;
      this.showStep();
    }
  }

  completeTour() {
    this.closeTour();
    
    // Save completion
    localStorage.setItem(`frutilla_tour_${this.currentTour}`, 'completed');
    
    // Show completion feedback
    this.showNotification('✅ Tutorial completed!', 'success');
  }

  closeTour() {
    document.getElementById('tutorial-modal')?.remove();
    document.getElementById('tutorial-overlay')?.remove();
    document.getElementById('tutorial-highlight')?.remove();
    
    this.tutorialActive = false;
    this.currentTour = null;
  }

  setMode(mode) {
    this.mode = mode;
    localStorage.setItem('frutilla_ui_mode', mode);
    
    document.body.classList.toggle('frutilla-simple-mode', mode === 'simple');
    document.body.classList.toggle('frutilla-advanced-mode', mode === 'advanced');
    
    this.showNotification(`Switched to ${mode} mode`, 'info');
  }

  getMode() {
    return this.mode;
  }

  showNotification(message, type = 'info') {
    const notification = document.createElement('div');
    notification.className = `fl-notification fl-notification-${type}`;
    notification.textContent = message;
    notification.style.cssText = `
      position: fixed;
      top: 20px;
      right: 20px;
      padding: 12px 20px;
      background: var(--bg-elevated);
      border-left: 4px solid var(--fl-${type === 'success' ? 'green-success' : 'orange-primary'});
      border-radius: var(--radius-md);
      box-shadow: var(--shadow-lg);
      z-index: 10000;
      animation: slideInRight 200ms ease-out, fadeOut 300ms ease-in 2.5s forwards;
    `;
    
    document.body.appendChild(notification);
    
    setTimeout(() => notification.remove(), 3000);
  }

  // Auto-start on first run
  autoStart() {
    if (this.isFirstRun) {
      setTimeout(() => this.startTour('welcome'), 1000);
    }
  }
}

// Create global instance
window.onboarding = new OnboardingManager();

// Auto-start on load
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', () => {
    window.onboarding.autoStart();
  });
} else {
  window.onboarding.autoStart();
}

export default OnboardingManager;
