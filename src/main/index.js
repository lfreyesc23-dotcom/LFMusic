const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { spawn } = require('child_process');

let mainWindow;
let omegaStudioProcess;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
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
    mainWindow = null;
  });
}

// Launch OmegaStudio DAW
function launchOmegaStudio() {
  const omegaStudioPath = path.join(
    __dirname,
    '../../OmegaStudio/build/OmegaStudio_artefacts/Release/Omega Studio.app/Contents/MacOS/Omega Studio'
  );

  try {
    omegaStudioProcess = spawn(omegaStudioPath, [], {
      detached: true,
      stdio: 'ignore'
    });

    omegaStudioProcess.unref();
    console.log('✅ OmegaStudio DAW launched');
  } catch (error) {
    console.error('❌ Failed to launch OmegaStudio:', error);
  }
}

// IPC handlers
ipcMain.handle('launch-omega-studio', async () => {
  launchOmegaStudio();
  return { success: true };
});

ipcMain.handle('get-audio-devices', async () => {
  // This will communicate with OmegaStudio's audio engine
  return {
    inputs: [],
    outputs: []
  };
});

ipcMain.handle('start-recording', async () => {
  // Bridge to OmegaStudio recording
  return { success: true };
});

ipcMain.handle('apply-autotune', async (event, params) => {
  // Bridge to OmegaStudio autotune/pitch correction
  return { success: true, params };
});

// App lifecycle
app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (omegaStudioProcess) {
    omegaStudioProcess.kill();
  }
  
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('quit', () => {
  if (omegaStudioProcess) {
    omegaStudioProcess.kill();
  }
});
