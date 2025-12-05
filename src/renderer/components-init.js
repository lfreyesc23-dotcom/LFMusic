/**
 * Component Initializer
 * Loads and initializes Frutilla Studio advanced components
 */

const PlaylistView = require('./components/playlist-view');
const ChannelRack = require('./components/channel-rack');

// Advanced components instances
let playlistView = null;
let channelRack = null;

/**
 * Initialize advanced DAW components
 * Call this from app.js when needed
 */
function initAdvancedComponents() {
  try {
    // Create container elements if they don't exist
    ensureContainers();
    
    // Initialize Playlist View
    if (!playlistView) {
      playlistView = new PlaylistView('playlist-view-container');
      console.log('✅ Playlist View initialized');
    }
    
    // Initialize Channel Rack
    if (!channelRack) {
      channelRack = new ChannelRack('channel-rack-container');
      // Make globally accessible for onclick handlers
      window.channelRackInstance = channelRack;
      console.log('✅ Channel Rack initialized');
    }
    
    return { playlistView, channelRack };
  } catch (error) {
    console.error('❌ Failed to initialize advanced components:', error);
    return null;
  }
}

/**
 * Ensure container elements exist in DOM
 */
function ensureContainers() {
  // Check if containers already exist
  if (!document.getElementById('playlist-view-container')) {
    // Create Playlist container
    const playlistContainer = document.createElement('div');
    playlistContainer.id = 'playlist-view-container';
    playlistContainer.className = 'advanced-component-container';
    playlistContainer.style.display = 'none'; // Hidden by default
    document.body.appendChild(playlistContainer);
  }
  
  if (!document.getElementById('channel-rack-container')) {
    // Create Channel Rack container
    const channelRackContainer = document.createElement('div');
    channelRackContainer.id = 'channel-rack-container';
    channelRackContainer.className = 'advanced-component-container';
    channelRackContainer.style.display = 'none'; // Hidden by default
    document.body.appendChild(channelRackContainer);
  }
}

/**
 * Show Playlist View
 */
function showPlaylistView() {
  if (!playlistView) {
    const components = initAdvancedComponents();
    if (!components) return false;
  }
  
  hideAllAdvancedViews();
  document.getElementById('playlist-view-container').style.display = 'block';
  console.log('📊 Showing Playlist View');
  return true;
}

/**
 * Show Channel Rack
 */
function showChannelRack() {
  if (!channelRack) {
    const components = initAdvancedComponents();
    if (!components) return false;
  }
  
  hideAllAdvancedViews();
  document.getElementById('channel-rack-container').style.display = 'block';
  console.log('🥁 Showing Channel Rack');
  return true;
}

/**
 * Hide all advanced views
 */
function hideAllAdvancedViews() {
  const containers = document.querySelectorAll('.advanced-component-container');
  containers.forEach(container => {
    container.style.display = 'none';
  });
}

/**
 * Get component instances
 */
function getComponents() {
  return {
    playlistView,
    channelRack
  };
}

module.exports = {
  initAdvancedComponents,
  showPlaylistView,
  showChannelRack,
  hideAllAdvancedViews,
  getComponents
};
