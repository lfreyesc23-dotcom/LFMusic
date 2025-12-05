const { ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');

/**
 * IPC Bridge para comunicación Electron <-> OmegaStudio C++
 */
class OmegaStudioBridge {
    constructor() {
        this.backendProcess = null;
        this.isConnected = false;
        this.setupIPC();
    }

    /**
     * Iniciar el proceso de C++ backend
     */
    startBackend() {
        const backendPath = path.join(__dirname, '../../OmegaStudio/build/OmegaStudio_artefacts/Release/Omega Studio.app/Contents/MacOS/Omega Studio');
        
        this.backendProcess = spawn(backendPath, [], {
            stdio: ['pipe', 'pipe', 'pipe', 'ipc']
        });

        this.backendProcess.stdout.on('data', (data) => {
            console.log(`[Backend]: ${data}`);
        });

        this.backendProcess.stderr.on('data', (data) => {
            console.error(`[Backend Error]: ${data}`);
        });

        this.backendProcess.on('close', (code) => {
            console.log(`Backend process exited with code ${code}`);
            this.isConnected = false;
        });

        this.isConnected = true;
    }

    /**
     * Configurar manejadores IPC
     */
    setupIPC() {
        // ==================== PLAYLIST ====================
        ipcMain.handle('playlist:createPattern', async (event, name) => {
            return this.sendCommand('playlist', 'createPattern', { name });
        });

        ipcMain.handle('playlist:addToPlaylist', async (event, patternId, trackIndex, startTime, length) => {
            return this.sendCommand('playlist', 'addToPlaylist', { 
                patternId, trackIndex, startTime, length 
            });
        });

        ipcMain.handle('playlist:makeUnique', async (event, instanceId) => {
            return this.sendCommand('playlist', 'makeUnique', { instanceId });
        });

        ipcMain.handle('playlist:clonePattern', async (event, instanceId) => {
            return this.sendCommand('playlist', 'clonePattern', { instanceId });
        });

        ipcMain.handle('playlist:getTracks', async (event) => {
            return this.sendCommand('playlist', 'getTracks', {});
        });

        ipcMain.handle('playlist:getPatterns', async (event) => {
            return this.sendCommand('playlist', 'getPatterns', {});
        });

        // ==================== CHANNEL RACK ====================
        ipcMain.handle('channelRack:addChannel', async (event, name, type) => {
            return this.sendCommand('channelRack', 'addChannel', { name, type });
        });

        ipcMain.handle('channelRack:setStep', async (event, channelId, stepIndex, active) => {
            return this.sendCommand('channelRack', 'setStep', { channelId, stepIndex, active });
        });

        ipcMain.handle('channelRack:setStepVelocity', async (event, channelId, stepIndex, velocity) => {
            return this.sendCommand('channelRack', 'setStepVelocity', { channelId, stepIndex, velocity });
        });

        ipcMain.handle('channelRack:loadPattern', async (event, channelId, patternName) => {
            return this.sendCommand('channelRack', 'loadPattern', { channelId, patternName });
        });

        ipcMain.handle('channelRack:getChannels', async (event) => {
            return this.sendCommand('channelRack', 'getChannels', {});
        });

        ipcMain.handle('channelRack:createLayer', async (event, name, childIds) => {
            return this.sendCommand('channelRack', 'createLayer', { name, childIds });
        });

        // ==================== PIANO ROLL ====================
        ipcMain.handle('pianoRoll:setScale', async (event, rootNote, scale) => {
            return this.sendCommand('pianoRoll', 'setScale', { rootNote, scale });
        });

        ipcMain.handle('pianoRoll:chordize', async (event, notes, chordType) => {
            return this.sendCommand('pianoRoll', 'chordize', { notes, chordType });
        });

        ipcMain.handle('pianoRoll:arpeggiate', async (event, notes, settings) => {
            return this.sendCommand('pianoRoll', 'arpeggiate', { notes, settings });
        });

        ipcMain.handle('pianoRoll:applyStrum', async (event, notes, settings) => {
            return this.sendCommand('pianoRoll', 'applyStrum', { notes, settings });
        });

        ipcMain.handle('pianoRoll:addGhostTrack', async (event, trackId, name, colour) => {
            return this.sendCommand('pianoRoll', 'addGhostTrack', { trackId, name, colour });
        });

        // ==================== BROWSER ====================
        ipcMain.handle('browser:search', async (event, criteria) => {
            return this.sendCommand('browser', 'search', criteria);
        });

        ipcMain.handle('browser:addToFavorites', async (event, itemPath) => {
            return this.sendCommand('browser', 'addToFavorites', { itemPath });
        });

        ipcMain.handle('browser:getRecent', async (event, count) => {
            return this.sendCommand('browser', 'getRecent', { count });
        });

        ipcMain.handle('browser:startPreview', async (event, itemPath) => {
            return this.sendCommand('browser', 'startPreview', { itemPath });
        });

        ipcMain.handle('browser:stopPreview', async (event) => {
            return this.sendCommand('browser', 'stopPreview', {});
        });

        // ==================== SYNTHS ====================
        ipcMain.handle('wavetableSynth:setPosition', async (event, position) => {
            return this.sendCommand('wavetableSynth', 'setPosition', { position });
        });

        ipcMain.handle('wavetableSynth:setUnison', async (event, voices, detune, spread) => {
            return this.sendCommand('wavetableSynth', 'setUnison', { voices, detune, spread });
        });

        ipcMain.handle('wavetableSynth:loadWavetable', async (event, type) => {
            return this.sendCommand('wavetableSynth', 'loadWavetable', { type });
        });

        ipcMain.handle('fmSynth:setAlgorithm', async (event, algorithm) => {
            return this.sendCommand('fmSynth', 'setAlgorithm', { algorithm });
        });

        ipcMain.handle('fmSynth:setOperator', async (event, opIndex, ratio, level, feedback) => {
            return this.sendCommand('fmSynth', 'setOperator', { opIndex, ratio, level, feedback });
        });

        // ==================== EFFECTS ====================
        ipcMain.handle('effects:flanger:setParams', async (event, rate, depth, feedback, mix) => {
            return this.sendCommand('effects', 'flangerParams', { rate, depth, feedback, mix });
        });

        ipcMain.handle('effects:chorus:setParams', async (event, voices, rate, depth, mix) => {
            return this.sendCommand('effects', 'chorusParams', { voices, rate, depth, mix });
        });

        ipcMain.handle('effects:stutter:trigger', async (event, length) => {
            return this.sendCommand('effects', 'stutterTrigger', { length });
        });

        // ==================== EXPORT ====================
        ipcMain.handle('export:project', async (event, settings) => {
            return this.sendCommand('export', 'project', settings);
        });

        ipcMain.handle('export:stems', async (event, settings) => {
            return this.sendCommand('export', 'stems', settings);
        });

        ipcMain.handle('export:midi', async (event, settings) => {
            return this.sendCommand('export', 'midi', settings);
        });

        // ==================== PERFORMANCE ====================
        ipcMain.handle('performance:freezeTrack', async (event, trackId) => {
            return this.sendCommand('performance', 'freezeTrack', { trackId });
        });

        ipcMain.handle('performance:getCPUUsage', async (event) => {
            return this.sendCommand('performance', 'getCPUUsage', {});
        });

        ipcMain.handle('performance:setLowLatency', async (event, enabled) => {
            return this.sendCommand('performance', 'setLowLatency', { enabled });
        });

        // ==================== PRESETS ====================
        ipcMain.handle('presets:load', async (event, type, name) => {
            return this.sendCommand('presets', 'load', { type, name });
        });

        ipcMain.handle('presets:save', async (event, type, name, data) => {
            return this.sendCommand('presets', 'save', { type, name, data });
        });

        ipcMain.handle('presets:list', async (event, type) => {
            return this.sendCommand('presets', 'list', { type });
        });
    }

    /**
     * Enviar comando al backend C++
     */
    async sendCommand(module, action, params) {
        if (!this.isConnected) {
            throw new Error('Backend not connected');
        }

        // Por ahora retornamos mock data
        // TODO: Implementar comunicación real via socket/pipe
        return {
            success: true,
            module,
            action,
            params,
            result: this.getMockData(module, action, params)
        };
    }

    /**
     * Mock data para desarrollo
     */
    getMockData(module, action, params) {
        switch(module) {
            case 'playlist':
                if (action === 'getTracks') {
                    return [
                        { index: 0, name: 'Main', colour: '#FF6B35', instances: [] },
                        { index: 1, name: 'Drums', colour: '#F7931E', instances: [] }
                    ];
                }
                if (action === 'getPatterns') {
                    return [
                        { id: 1, name: 'Pattern 1', colour: '#4ECDC4', lengthInBars: 4 },
                        { id: 2, name: 'Pattern 2', colour: '#95E1D3', lengthInBars: 4 }
                    ];
                }
                break;
            
            case 'channelRack':
                if (action === 'getChannels') {
                    return [
                        { id: 1, name: 'Kick', type: 'Instrument', steps: Array(16).fill(false) },
                        { id: 2, name: 'Snare', type: 'Instrument', steps: Array(16).fill(false) }
                    ];
                }
                break;
            
            case 'browser':
                if (action === 'search') {
                    return [
                        { name: 'Kick 01.wav', type: 'Sample', bpm: 120, key: 'C', rating: 5 },
                        { name: 'Snare 02.wav', type: 'Sample', bpm: 125, key: 'C', rating: 4 }
                    ];
                }
                break;
        }
        
        return null;
    }

    /**
     * Detener backend
     */
    stopBackend() {
        if (this.backendProcess) {
            this.backendProcess.kill();
            this.isConnected = false;
        }
    }
}

module.exports = OmegaStudioBridge;
