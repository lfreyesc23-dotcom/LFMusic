const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const OmegaStudioBridge = require('./omega-bridge');

let mainWindow;
let omegaBridge;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1600,
    height: 1000,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      enableRemoteModule: true
    },
    backgroundColor: '#1a1a1a',
    titleBarStyle: 'hiddenInset',
    frame: true
  });

  mainWindow.loadFile('src/renderer/index.html');

  // Open DevTools in development
  if (process.env.NODE_ENV === 'development') {
    mainWindow.webContents.openDevTools();
  }

  mainWindow.on('closed', () => {
    if (omegaBridge) {
      omegaBridge.stopBackend();
    }
    mainWindow = null;
  });
}

// Initialize Omega Studio Bridge
function initializeOmegaBridge() {
  omegaBridge = new OmegaStudioBridge();
  console.log('✅ OmegaStudio Bridge initialized');
  
  // Start backend (optional - can work standalone with mock data)
  // omegaBridge.startBackend();
}

// IPC handlers for legacy compatibility
ipcMain.handle('launch-omega-studio', async () => {
  if (omegaBridge) {
    omegaBridge.startBackend();
  }
  return { success: true };
});

ipcMain.handle('get-audio-devices', async () => {
  // Use bridge to get audio devices
  if (omegaBridge) {
    return await omegaBridge.sendCommand('getAudioDevices', {});
  }
  return { inputs: [], outputs: [] };
});

ipcMain.handle('start-recording', async () => {
  // Bridge to recording functionality
  if (omegaBridge) {
    return await omegaBridge.sendCommand('startRecording', {});
  }
  return { success: true };
});

ipcMain.handle('apply-autotune', async (event, params) => {
  // Bridge to autotune/pitch correction
  if (omegaBridge) {
    return await omegaBridge.sendCommand('applyAutotune', params);
  }
  return { success: true, params };
});

// App lifecycle
app.whenReady().then(() => {
  initializeOmegaBridge();
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (omegaBridge) {
    omegaBridge.stopBackend();
  }
  
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('quit', () => {
  if (omegaBridge) {
    omegaBridge.stopBackend();
  }
});
