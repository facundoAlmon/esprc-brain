import { state, elements, icons } from './state.js';
import { setLanguage } from './language.js';
import { initJoysticks } from './joystick.js';
import { getWifiConfig, saveWifiConfig, getConfig, saveConfig, exportConfig, importConfig, getLedConfig, saveLedConfig, exportLedConfig, importLedConfig, getCamConfig, setCamConfig } from './config.js';
import { wsReconnect, fetchAPI } from './api.js';
import { addLedGroup } from './leds.js';
import { uploadProgram, runProgram, stopProgram, clearProgram, showActionModal, loadProgramFromServer, exportProgram, importProgram } from './program.js';
import { handleKidModeCommand, runKidSequence, stopKidSequence, clearKidSequence } from './kidMode.js';
import { updateLightsUI } from './lights.js';
import { translations } from './translations.js';

export function setupEventListeners() {
    elements.languageSelector.addEventListener('change', (e) => setLanguage(e.target.value));
    elements.menuToggle.addEventListener('click', () => {
        elements.sidebar.classList.toggle('sidebar-hidden');
        elements.mainWrapper.classList.toggle('full-width');
    });
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
    state.activeTab = tabId;
    elements.tabContents.forEach(content => content.classList.remove('active'));
    elements.menuLinks.forEach(link => link.classList.remove('active'));
    document.getElementById(tabId).classList.add('active');
    document.querySelector(`.menu-link[data-tab="${tabId}"]`).classList.add('active');
    if (window.innerWidth <= 992) {
        elements.sidebar.classList.add('sidebar-hidden');
        elements.mainWrapper.classList.add('full-width');
    }
    updateLightsUI();
    switch (tabId) {
        case 'config': getConfig(); break;
        case 'led-config': getLedConfig(); break;
        case 'conexion': getWifiConfig(); break;
        case 'cam': getCamConfig(); break;
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
    form.querySelectorAll('input, select').forEach(el => {
        if (el.id) {
            if (el.type === 'checkbox') {
                configBody[el.id] = el.checked ? 1 : 0;
            } else if (el.classList.contains('icon-toggle')) {
                configBody[el.id] = el.classList.contains('active') ? 1 : 0;
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

async function startCamStream() {
    const camApiUrl = `${window.location.protocol}//${state.wifiConfig.camIP}/`;
    await fetchAPI('enable-stream', {}, camApiUrl);
}

async function stopCamStream() {
    const camApiUrl = `${window.location.protocol}//${state.wifiConfig.camIP}/`;
    await fetchAPI('disable-stream', {}, camApiUrl);
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
