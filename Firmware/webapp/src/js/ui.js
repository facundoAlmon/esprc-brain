import { state, elements, icons } from './state.js';
import { setLanguage } from './language.js';
import { initJoysticks } from './joystick.js';
import { getWifiConfig, saveWifiConfig, getConfig, saveConfig, exportConfig, importConfig, getLedConfig, saveLedConfig, exportLedConfig, importLedConfig, getCamConfig, setCamConfig } from './config.js';
import { wsReconnect, fetchAPI, connectCamMjpeg, sendWsAction } from './api.js';
import { toggleVideoRecording } from './recorder.js';
import { addLedGroup } from './leds.js';
import { uploadProgram, runProgram, stopProgram, clearProgram, showActionModal, loadProgramFromServer, exportProgram, importProgram } from './program.js';
import { handleKidModeCommand, runKidSequence, stopKidSequence, clearKidSequence } from './kidMode.js';
import { updateLightsUI } from './lights.js';
import { translations } from './translations.js';
import { refreshOtaInfo } from './ota.js';

export function setupEventListeners() {
    elements.languageSelector.addEventListener('change', (e) => setLanguage(e.target.value));
    const toggleSidebar = () => {
        elements.sidebar.classList.toggle('sidebar-hidden');
        elements.mainWrapper.classList.toggle('full-width');
    };
    elements.menuToggle.addEventListener('click', toggleSidebar);
    const bottomNavMore = document.getElementById('bottom-nav-more');
    if (bottomNavMore) bottomNavMore.addEventListener('click', toggleSidebar);
    elements.menuLinks.forEach(link => {
        link.addEventListener('click', () => openTab(link.dataset.tab));
    });
    document.querySelectorAll('.slider-group input[type="range"]').forEach(slider => {
        slider.addEventListener('input', (e) => {
            const valueEl = document.getElementById(`${e.target.id}Value`);
            if (valueEl) valueEl.textContent = e.target.value;
        });
    });
    elements.lightModeToggle.addEventListener('change', () => {
        document.body.classList.toggle('light-mode');
        localStorage.setItem('theme', document.body.classList.contains('light-mode') ? 'light' : 'dark');
        initJoysticks();
    });
    document.getElementById('add-led-group').addEventListener('click', () => addLedGroup());
    document.getElementById('import-led-input').addEventListener('change', (e) => importLedConfig(e));
    attachClick('getWifiConfigBtn', getWifiConfig);
    attachClick('saveWifiConfigBtn', saveWifiConfig);
    attachClick('wsReconnectBtn', wsReconnect);
    attachClick('getConfigBtn', getConfig);
    attachClick('saveConfigBtn', saveConfig);
    attachClick('exportLedConfigBtn', exportLedConfig);
    attachClick('saveLedConfigBtn', saveLedConfig);
    attachClick('manageESPBtn', manageESP);
    attachClick('startCamStreamBtn', startCamStream);
    attachClick('stopCamStreamBtn', stopCamStream);
    attachClick('getCamConfigBtn', getCamConfig);
    attachClick('setCamConfigBtn', setCamConfig);
    attachClick('camFullscreenBtn', enterCamFs);
    attachClick('camExitFsBtn', exitCamFs);
    attachClick('fpvFullscreenBtn', toggleFpvFs);
    attachClick('fpvStartStreamBtn', startCamStream);
    attachClick('fpvStopStreamBtn', stopCamStream);
    attachClick('camRecBtn', toggleVideoRecording);
    attachClick('fpvRecBtn', toggleVideoRecording);
    document.querySelectorAll('.cam-hold-btn').forEach(btn => btn.addEventListener('click', handleCamHoldToggle));
    document.querySelectorAll('.cam-center-btn').forEach(btn => btn.addEventListener('click', handleCamCenter));
    // Botón de freno: mantener presionado = frena; soltar = libera.
    document.querySelectorAll('.brake-btn').forEach(function(btn) {
        var press = function(e) { e.preventDefault(); btn.classList.add('active'); sendWsAction('brake_on'); };
        var release = function() { if (!btn.classList.contains('active')) return; btn.classList.remove('active'); sendWsAction('brake_off'); };
        btn.addEventListener('pointerdown', press);
        btn.addEventListener('pointerup', release);
        btn.addEventListener('pointerleave', release);
        btn.addEventListener('pointercancel', release);
    });
    document.addEventListener('fullscreenchange', function() {
        if (!document.fullscreenElement && camFsActive) exitCamFs();
        if (!document.fullscreenElement) fpvFsActive = false;
    });
    document.addEventListener('webkitfullscreenchange', function() {
        if (!document.webkitFullscreenElement && camFsActive) exitCamFs();
        if (!document.webkitFullscreenElement) fpvFsActive = false;
    });
    attachClick('uploadProgramBtn', uploadProgram);
    attachClick('runProgramBtn', runProgram);
    attachClick('stopProgramBtn', stopProgram);
    attachClick('clearProgramBtn', clearProgram);
    attachClick('addActionBtn', showActionModal);
    attachClick('loadProgramBtn', loadProgramFromServer);
    attachClick('exportConfigBtn', exportConfig);
    elements.importConfigInput.addEventListener('change', (e) => importConfig(e));

    attachClick('exportProgramBtn', exportProgram);
    elements.importProgramBtn.addEventListener('click', () => elements.importProgramInput.click());
    elements.importProgramInput.addEventListener('change', (e) => importProgram(e));
    const swapHandler = () => {
        state.joystickLayoutSwapped = !state.joystickLayoutSwapped;
        localStorage.setItem('joystickLayoutSwapped', state.joystickLayoutSwapped);
        document.getElementById('joystick-a').querySelector('.joystick-layout-single').classList.toggle('swapped', state.joystickLayoutSwapped);
    };
    elements.swapLayoutBtnA.addEventListener('click', swapHandler);
    document.querySelectorAll('.icon-toggle, .toggle').forEach(button => {
        setupTooltipEvents(button);
    });

    elements.programInfinite.addEventListener('change', () => {
        elements.programIterations.disabled = elements.programInfinite.checked;
        if (elements.programInfinite.checked) {
            elements.programIterations.value = -1; // Set to -1 for infinite
        } else {
            elements.programIterations.value = 1; // Reset to 1 when unchecked
        }
    });

    // Listener for kid mode command buttons (D-Pad and extras)
    elements.kidModeCommands.addEventListener('click', (e) => {
        const commandButton = e.target.closest('.kid-mode-btn[data-command], .d-pad-btn[data-command]');
        if (commandButton) {
            handleKidModeCommand(commandButton.dataset.command);
        }
    });

    attachClick('runSequenceBtn', runKidSequence);
    attachClick('stopSequenceBtn', stopKidSequence);
    attachClick('clearSequenceBtn', clearKidSequence);

    elements.kidModeInfinite.addEventListener('change', () => {
        elements.kidModeIterations.disabled = elements.kidModeInfinite.checked;
        if (elements.kidModeInfinite.checked) {
            elements.kidModeIterations.value = -1; // -1 for infinite
        } else {
            elements.kidModeIterations.value = 1; // Reset to 1
        }
    });
}

function attachClick(id, handler) {
    const element = document.getElementById(id);
    if (element) {
        element.addEventListener('click', handler.bind(this));
    }
}

export function openTab(tabId) {
    const wasCamTab = (state.activeTab === 'cam' || state.activeTab === 'fpv');
    const isCamTab = (tabId === 'cam' || tabId === 'fpv');
    if (wasCamTab && !isCamTab) stopCamStatsPolling();
    if (state.activeTab === 'fpv' && tabId !== 'fpv') leaveFpv();
    state.activeTab = tabId;
    elements.tabContents.forEach(content => content.classList.remove('active'));
    elements.menuLinks.forEach(link => link.classList.remove('active'));
    document.getElementById(tabId).classList.add('active');
    document.querySelectorAll(`.menu-link[data-tab="${tabId}"]`).forEach(el => el.classList.add('active'));
    if (window.innerWidth <= 992) {
        elements.sidebar.classList.add('sidebar-hidden');
        elements.mainWrapper.classList.add('full-width');
    }
    updateLightsUI();
    switch (tabId) {
        case 'config': getConfig(); break;
        case 'led-config': getLedConfig(); break;
        case 'conexion': getWifiConfig(); break;
        case 'cam': getCamConfig(); startCamStatsPolling(); break;
        case 'manage': refreshOtaInfo(); break;
        case 'fpv':
            enterFpv();
            startCamStatsPolling();
            requestAnimationFrame(initJoysticks);
            break;
        case 'joystick-a':
        case 'joystick-b':
            requestAnimationFrame(initJoysticks);
            break;
    }
}

export function updateButtonIcon(button, isActive, forceType = null) {
    let iconType = forceType;
    if (!iconType) {
        if (button.id.includes('Scan')) iconType = 'bluetooth';
        else iconType = 'default';
    } 
    if (button.id.startsWith('headlights-btn')) {
        const stateMap = ['off', 'pos', 'low', 'high'];
        const iconName = `headlights_${stateMap[state.lightsState.headlights]}`;
        button.innerHTML = icons[iconName];
    }
}

export function updateForm(jsonData) {
    Object.keys(jsonData).forEach(key => {
        const el = document.getElementById(key);
        if (el) {
            if (el.type === 'checkbox') {
                el.checked = jsonData[key] == 1;
            } else if (el.classList.contains('icon-toggle')) {
                const isActive = jsonData[key] == 1;
                el.classList.toggle('active', isActive);
                updateButtonIcon(el, isActive);
            } else {
                el.value = jsonData[key];
            }
            const valueEl = document.getElementById(`${key}Value`);
            if (valueEl) valueEl.textContent = jsonData[key];
        }
    });
    if (jsonData.wifiMode == "AP") document.getElementById('apMode').checked = true
    if (jsonData.wifiMode == "CLI") document.getElementById('cliMode').checked = true
}

export function serializeForm(formSelector) {
    const form = document.querySelector(formSelector);
    const configBody = {};
    form.querySelectorAll('input, select, button.icon-toggle').forEach(el => {
        if (el.id) {
            if (el.tagName === 'BUTTON') {
                configBody[el.id] = el.classList.contains('active') ? 1 : 0;
            } else if (el.type === 'checkbox') {
                configBody[el.id] = el.checked ? 1 : 0;
            } else if (el.type === 'range' || el.type === 'number') {
                configBody[el.id] = parseInt(el.value, 10);
            } else if (el.tagName === 'SELECT' || el.type !== 'radio' || el.checked) {
                configBody[el.id] = el.value;
            }
        }
    });
    return configBody;
}

export function setupTooltipEvents(element) {
    element.addEventListener('mouseenter', e => showTooltip(e));
    element.addEventListener('mouseleave', () => hideTooltip());
    element.addEventListener('touchstart', e => {
        state.tooltipTimeout = setTimeout(() => showTooltip(e), 500);
    }, { passive: true });
    element.addEventListener('touchend', () => {
        clearTimeout(state.tooltipTimeout);
        hideTooltip();
    }, { passive: true });
    element.addEventListener('touchcancel', () => {
        clearTimeout(state.tooltipTimeout);
        hideTooltip();
    }, { passive: true });
}

function showTooltip(e) {
    const target = e.currentTarget;
    const tooltipKey = target.dataset.tooltipKey;
    if (!tooltipKey) return;
    const tooltipText = translations[state.currentLanguage][tooltipKey] || tooltipKey;
    elements.tooltip.textContent = tooltipText;
    elements.tooltip.classList.add('visible');
    const targetRect = target.getBoundingClientRect();
    const tooltipRect = elements.tooltip.getBoundingClientRect();
    let top = targetRect.top - tooltipRect.height - 8;
    let left = targetRect.left + (targetRect.width / 2) - (tooltipRect.width / 2);
    if (top < 0) top = targetRect.bottom + 8;
    if (left < 0) left = 5;
    if (left + tooltipRect.width > window.innerWidth) left = window.innerWidth - tooltipRect.width - 5;
    elements.tooltip.style.top = `${top}px`;
    elements.tooltip.style.left = `${left}px`;
}

function hideTooltip() {
    elements.tooltip.classList.remove('visible');
}

async function manageESP() {
    const configBody = {
        clearPreferences: document.getElementById("clearPreferences").checked,
        restartESP: true
    };
    await fetchAPI('manage', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
    alert(translations[state.currentLanguage].restartedAlert);
}

function startCamStream() {
    if (state.cameraInfo && state.cameraInfo.mjpegUrl) {
        connectCamMjpeg(state.cameraInfo.mjpegUrl);
        return;
    }
    const ip = state.wifiConfig && state.wifiConfig.camIP;
    if (!ip) { console.warn('No camera IP configured'); return; }
    connectCamMjpeg(`http://${ip}:81/mjpeg`);
}

function stopCamStream() {
    const img = document.getElementById('camImg');
    if (img) { img.src = ''; }
    state.socketCamOnline = false;
}

// ── Camera fullscreen ────────────────────────
let camFsActive = false;
let camImgOriginalParent = null;

function enterCamFs() {
    const img = document.getElementById('camImg');
    const fsStream = document.getElementById('camFsStream');
    const fsOverlay = document.getElementById('camFsOverlay');
    if (!img || !fsStream || !fsOverlay) return;
    camImgOriginalParent = img.parentNode;
    fsStream.appendChild(img);
    fsOverlay.classList.add('active');
    camFsActive = true;
    if (fsOverlay.requestFullscreen) fsOverlay.requestFullscreen().catch(function() {});
    else if (fsOverlay.webkitRequestFullscreen) fsOverlay.webkitRequestFullscreen();
}

function exitCamFs() {
    const img = document.getElementById('camImg');
    const fsOverlay = document.getElementById('camFsOverlay');
    if (!img || !fsOverlay) return;
    if (camImgOriginalParent) {
        // Re-insert img as first child of .cam-stream (before .cam-overlay)
        const firstChild = camImgOriginalParent.firstChild;
        camImgOriginalParent.insertBefore(img, firstChild);
    }
    fsOverlay.classList.remove('active');
    camFsActive = false;
    if (document.fullscreenElement) document.exitFullscreen().catch(function() {});
    else if (document.webkitFullscreenElement) document.webkitExitFullscreen();
}

// ── Camera servo quick controls ──────────────
// Hold toggle persists on the firmware side (WS action writes NVS);
// the webapp mirrors the state for button highlighting.
function handleCamHoldToggle() {
    if (!state.socketOnline) return;
    state.camHoldMode = !state.camHoldMode;
    sendWsAction('cam_hold_toggle');
    updateCamHoldButtons();
}

function handleCamCenter() {
    sendWsAction('cam_center');
}

export function updateCamHoldButtons() {
    document.querySelectorAll('.cam-hold-btn').forEach(function(btn) {
        btn.classList.toggle('active', state.camHoldMode);
    });
}

// ── FPV view ─────────────────────────────────
// The MJPEG <img> is moved (not duplicated) into the FPV container — the
// camera only accepts one stream connection at a time.
let fpvFsActive = false;

function enterFpv() {
    const img = document.getElementById('camImg');
    const fpvStream = document.getElementById('fpvStream');
    if (img && fpvStream) fpvStream.appendChild(img);
    if (img && !img.getAttribute('src')) startCamStream();
}

function leaveFpv() {
    if (fpvFsActive) exitFpvFs();
    const img = document.getElementById('camImg');
    const home = document.querySelector('#cam .cam-stream');
    if (img && home && img.parentNode !== home) home.insertBefore(img, home.firstChild);
}

function toggleFpvFs() {
    if (fpvFsActive) { exitFpvFs(); return; }
    const cont = document.getElementById('fpvContainer');
    if (!cont) return;
    fpvFsActive = true;
    if (cont.requestFullscreen) cont.requestFullscreen().catch(function() {});
    else if (cont.webkitRequestFullscreen) cont.webkitRequestFullscreen();
}

function exitFpvFs() {
    fpvFsActive = false;
    if (document.fullscreenElement) document.exitFullscreen().catch(function() {});
    else if (document.webkitFullscreenElement) document.webkitExitFullscreen();
}

// ── Camera stats polling ─────────────────────
let camStatsTimer = null;

function startCamStatsPolling() {
    if (camStatsTimer) return;
    camStatsTimer = setInterval(updateCamStats, 2000);
}

function stopCamStatsPolling() {
    if (camStatsTimer) { clearInterval(camStatsTimer); camStatsTimer = null; }
    setFpsBadges('');
}

function setFpsBadges(text) {
    ['fps', 'fpsFull', 'fpsFpv'].forEach(function(id) {
        var el = document.getElementById(id);
        if (el) el.textContent = text;
    });
}

function updateCamStats() {
    var statsEl = document.getElementById('statsEnabled');
    if (statsEl && !statsEl.checked) {
        setFpsBadges('');
        return;
    }
    var ip = (state.cameraInfo && state.cameraInfo.ip) || (state.wifiConfig && state.wifiConfig.camIP);
    if (!ip) return;
    fetch('http://' + ip + '/api/stats').then(function(r) {
        if (!r.ok) return null;
        return r.json();
    }).then(function(d) {
        if (!d) return;
        setFpsBadges((d.enabled && typeof d.fps === 'number') ? d.fps.toFixed(1) + ' FPS' : '');
    }).catch(function() {});
}

export function setupUI() {
    elements.urlWS.value = `ws://${state.hostname}/ws`;
    elements.urlAPI.value = `${window.location.protocol}//${state.hostname}/`;
    if (localStorage.getItem('theme') === 'light') {
        document.body.classList.add('light-mode');
        elements.lightModeToggle.checked = true;
    }
    if (window.innerWidth <= 992) {
        elements.sidebar.classList.add('sidebar-hidden');
        elements.mainWrapper.classList.add('full-width');
    }
    state.joystickLayoutSwapped = localStorage.getItem('joystickLayoutSwapped') === 'true';
    document.getElementById('joystick-a').querySelector('.joystick-layout-single').classList.toggle('swapped', state.joystickLayoutSwapped);
    document.getElementById('joystick-a').querySelector('.joystick-layout-single').classList.toggle('active', state.joystickLayoutSwapped);
}
