const { ipcRenderer } = require('electron');

// Navigation
document.querySelectorAll('.nav-item').forEach(item => {
  item.addEventListener('click', () => {
    // Remove active class from all items
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    item.classList.add('active');

    // Hide all views
    document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));

    // Show selected view
    const viewName = item.getAttribute('data-view');
    const view = document.getElementById(`${viewName}-view`);
    if (view) {
      view.classList.add('active');
    }
  });
});

// Launch DAW
document.getElementById('launchDAW').addEventListener('click', async () => {
  updateStatus('Launching OmegaStudio DAW...');
  try {
    await ipcRenderer.invoke('launch-omega-studio');
    updateStatus('OmegaStudio DAW launched successfully ✅');
  } catch (error) {
    updateStatus('Failed to launch DAW ❌');
    console.error(error);
  }
});

// Recording Controls
let isRecording = false;
let isPlaying = false;

document.getElementById('recordBtn').addEventListener('click', async () => {
  if (!isRecording) {
    isRecording = true;
    updateStatus('Recording... 🔴');
    document.getElementById('recordBtn').style.opacity = '0.7';
    
    try {
      await ipcRenderer.invoke('start-recording');
    } catch (error) {
      console.error('Recording error:', error);
      isRecording = false;
    }
  } else {
    isRecording = false;
    updateStatus('Recording stopped ⏹');
    document.getElementById('recordBtn').style.opacity = '1';
  }
});

document.getElementById('playBtn').addEventListener('click', () => {
  if (!isPlaying) {
    isPlaying = true;
    updateStatus('Playing... ▶');
    document.getElementById('playBtn').style.opacity = '0.7';
  } else {
    isPlaying = false;
    updateStatus('Playback stopped ⏹');
    document.getElementById('playBtn').style.opacity = '1';
  }
});

document.getElementById('stopBtn').addEventListener('click', () => {
  isRecording = false;
  isPlaying = false;
  updateStatus('Stopped ⏹');
  document.getElementById('recordBtn').style.opacity = '1';
  document.getElementById('playBtn').style.opacity = '1';
});

// Autotune Controls
const autotuneAmount = document.getElementById('autotuneAmount');
const autotuneAmountValue = document.getElementById('autotuneAmountValue');
const retuneSpeed = document.getElementById('retuneSpeed');
const retuneSpeedValue = document.getElementById('retuneSpeedValue');
const humanize = document.getElementById('humanize');
const humanizeValue = document.getElementById('humanizeValue');

autotuneAmount.addEventListener('input', (e) => {
  autotuneAmountValue.textContent = `${e.target.value}%`;
});

retuneSpeed.addEventListener('input', (e) => {
  retuneSpeedValue.textContent = `${e.target.value}ms`;
});

humanize.addEventListener('input', (e) => {
  humanizeValue.textContent = `${e.target.value}%`;
});

document.getElementById('applyAutotune').addEventListener('click', async () => {
  const params = {
    amount: autotuneAmount.value,
    key: document.getElementById('autotuneKey').value,
    retuneSpeed: retuneSpeed.value,
    humanize: humanize.value
  };

  updateStatus('Applying autotune...');
  
  try {
    const result = await ipcRenderer.invoke('apply-autotune', params);
    if (result.success) {
      updateStatus('Autotune applied successfully ✅');
    }
  } catch (error) {
    updateStatus('Failed to apply autotune ❌');
    console.error(error);
  }
});

// AI Features
document.querySelectorAll('.ai-feature .btn-apply').forEach((btn, index) => {
  btn.addEventListener('click', () => {
    const feature = btn.closest('.ai-feature').querySelector('h3').textContent;
    updateStatus(`Applying ${feature}...`);
    
    setTimeout(() => {
      updateStatus(`${feature} applied successfully ✅`);
    }, 1500);
  });
});

// Beat Cards
document.querySelectorAll('.btn-play-beat').forEach(btn => {
  btn.addEventListener('click', (e) => {
    const beatCard = e.target.closest('.beat-card');
    const beatName = beatCard.querySelector('h3').textContent;
    updateStatus(`Playing ${beatName}...`);
  });
});

// Utility Functions
function updateStatus(message) {
  document.getElementById('statusText').textContent = message;
}

// CPU Usage Simulation (replace with actual monitoring)
setInterval(() => {
  const cpuUsage = Math.floor(Math.random() * 30) + 10;
  document.getElementById('cpuUsage').textContent = `${cpuUsage}%`;
}, 2000);

// Initialize
updateStatus('Ready');
console.log('🍓 Frutilla Studio initialized');
