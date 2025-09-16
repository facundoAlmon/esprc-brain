import { state, elements } from './state.js';
import { setupLanguage } from './language.js';
import { setupUI, setupEventListeners } from './ui.js';
import { initJoysticks, startActionLoop } from './joystick.js';
import { connectWebSockets } from './api.js';
import { getWifiConfig } from './config.js';
import { initLights } from './lights.js';

function init() {
    elements.menuLinks = document.querySelectorAll('.menu-link');
    elements.tabContents = document.querySelectorAll('.tab-content');
    elements.urlWS = document.getElementById('urlWS');
    elements.urlAPI = document.getElementById('urlAPI');
    elements.lightModeToggle = document.getElementById('lightModeToggle');
    elements.languageSelector = document.getElementById('languageSelector');
    elements.menuToggle = document.getElementById('menu-toggle');
    elements.sidebar = document.getElementById('sidebar');
    elements.mainWrapper = document.querySelector('.main-wrapper');
    elements.tooltip = document.getElementById('tooltip');
    elements.turnLeftBtnA = document.getElementById('turn-left-btn-a');
    elements.turnRightBtnA = document.getElementById('turn-right-btn-a');
    elements.hazardBtnA = document.getElementById('hazard-btn-a');
    elements.headlightsBtnA = document.getElementById('headlights-btn-a');
    elements.swapLayoutBtnA = document.getElementById('swap-layout-btn-a');
    elements.turnLeftBtnB = document.getElementById('turn-left-btn-b');
    elements.turnRightBtnB = document.getElementById('turn-right-btn-b');
    elements.hazardBtnB = document.getElementById('hazard-btn-b');
    elements.headlightsBtnB = document.getElementById('headlights-btn-b');
    elements.acelIzq = document.getElementById('acelIzq');
    elements.programIterations = document.getElementById('programIterations');
    elements.programInfinite = document.getElementById('programInfinite');
    elements.exportConfigBtn = document.getElementById('exportConfigBtn');
    elements.importConfigInput = document.getElementById('import-config-input');
    elements.exportProgramBtn = document.getElementById('exportProgramBtn');
    elements.importProgramBtn = document.getElementById('importProgramBtn');
    elements.importProgramInput = document.getElementById('import-program-input');
    elements.runSequenceBtn = document.getElementById('runSequenceBtn');
    elements.stopSequenceBtn = document.getElementById('stopSequenceBtn');
    elements.clearSequenceBtn = document.getElementById('clearSequenceBtn');
    elements.kidModeCommands = document.getElementById('kid-mode-commands');
    elements.kidModeSequenceContainer = document.getElementById('kid-mode-sequence-container');
    elements.kidModeIterations = document.getElementById('kidModeIterations');
    elements.kidModeInfinite = document.getElementById('kidModeInfinite');
    elements.recordBtnA = document.getElementById('record-btn-a');
    elements.recordBtnB = document.getElementById('record-btn-b');

    setupLanguage();
    setupUI();
    setupEventListeners();
    initJoysticks();
    connectWebSockets();
    loadInitialData();
    startActionLoop();
    initLights();
}

function loadInitialData() {
    getWifiConfig();
}

document.addEventListener('DOMContentLoaded', init);
