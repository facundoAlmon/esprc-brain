import { state, elements } from './state.js';
import { fetchAPI } from './api.js';
import { updateForm, serializeForm, updateCamHoldButtons } from './ui.js';
import { translations } from './translations.js';
import { renderLedGroups } from './leds.js';
import { updateCamJoyVisibility } from './joystick.js';

var SERVO_PRESETS = {
    sg90:  { minUs: 500,  maxUs: 2400 },
    s3003: { minUs: 600,  maxUs: 2400 },
    custom: null
};

function applyServoPreset(typeSelectId, minId, maxId) {
    var sel = document.getElementById(typeSelectId);
    if (!sel) return;
    var preset = SERVO_PRESETS[sel.value];
    if (preset) {
        document.getElementById(minId).value = preset.minUs;
        document.getElementById(maxId).value = preset.maxUs;
    }
}

export function initCamServoUI() {
    var toggle = document.getElementById('camServoEnabled');
    var options = document.getElementById('camServoOptions');
    if (!toggle || !options) return;

    toggle.addEventListener('change', function() {
        options.style.display = toggle.checked ? '' : 'none';
        state.camServoEnabled = toggle.checked;
        updateCamJoyVisibility();
    });

    var panTypeEl = document.getElementById('panServoType');
    var tiltTypeEl = document.getElementById('tiltServoType');
    if (panTypeEl) {
        panTypeEl.addEventListener('change', function() {
            applyServoPreset('panServoType', 'panMinUs', 'panMaxUs');
        });
    }
    if (tiltTypeEl) {
        tiltTypeEl.addEventListener('change', function() {
            applyServoPreset('tiltServoType', 'tiltMinUs', 'tiltMaxUs');
        });
    }
}

function updateMotorTypeVisibility() {
    var sel = document.getElementById('motorType');
    var dc = document.getElementById('dcMotorOptions');
    var esc = document.getElementById('escOptions');
    if (!sel) return;
    var isEsc = sel.value == '1';
    if (dc) dc.style.display = isEsc ? 'none' : '';
    if (esc) esc.style.display = isEsc ? '' : 'none';
}

export function initMotorUI() {
    var sel = document.getElementById('motorType');
    if (sel) sel.addEventListener('change', updateMotorTypeVisibility);

    var phases = [
        ['escCalHigh', 'high'],
        ['escCalNeutral', 'neutral'],
        ['escCalLow', 'low'],
        ['escCalEnd', 'end']
    ];
    phases.forEach(function(p) {
        var btn = document.getElementById(p[0]);
        if (btn) btn.addEventListener('click', function() { escCalibrate(p[1]); });
    });
}

export async function escCalibrate(phase) {
    await fetchAPI('api/esc/calibrate', {
        method: 'POST',
        body: JSON.stringify({ phase: phase }),
        headers: { 'Content-Type': 'application/json' }
    });
}

export async function getWifiConfig() {
    const json = await fetchAPI('wifi');
    if (!json) return;
    state.wifiConfig = json;
    updateForm(json);
}

export async function saveWifiConfig() {
    const configBody = {
        wifiName: document.getElementById('wifiName').value,
        wifiPass: document.getElementById('wifiPass').value,
        wifiMode: document.getElementById('cliMode').checked ? "CLI" : "AP"
    };
    await fetchAPI('wifi', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
    alert(translations[state.currentLanguage].wifiSavedAlert);
}

export async function getConfig() {
    const json = await fetchAPI('config');
    if (!json) return;
    state.config = json;
    updateForm(json);
    state.camServoEnabled = !!(json.camServoEnabled);
    state.camHoldMode = !!(json.camHoldMode);
    var opts = document.getElementById('camServoOptions');
    if (opts) opts.style.display = state.camServoEnabled ? '' : 'none';
    updateMotorTypeVisibility();
    updateCamJoyVisibility();
    updateCamHoldButtons();
}

export async function saveConfig() {
    const configBody = serializeForm('#config');
    // El <select> serializa como string; ArduinoJson no convierte string→int.
    if (configBody.motorType !== undefined) configBody.motorType = parseInt(configBody.motorType, 10);
    const prevType = state.config && state.config.motorType;
    await fetchAPI('config', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
    if (prevType !== undefined && prevType != configBody.motorType) {
        alert(translations[state.currentLanguage].motorTypeRebootAlert);
        return; // el ESP se reinicia; evitar re-leer config mientras tanto
    }
    getConfig();
}

export async function exportConfig() {
    try {
        const configData = await fetchAPI('api/config/backup');
        if (configData) {
            const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(configData, null, 2));
            const downloadAnchorNode = document.createElement('a');
            downloadAnchorNode.setAttribute("href", dataStr);
            downloadAnchorNode.setAttribute("download", "esprc_config_backup.json");
            document.body.appendChild(downloadAnchorNode);
            downloadAnchorNode.click();
            downloadAnchorNode.remove();
            alert('Configuration exported successfully!');
        } else {
            alert('Failed to export configuration.');
        }
    } catch (error) {
        console.error('Error exporting config:', error);
        alert('An error occurred during configuration export.');
    }
}

export async function importConfig(event) {
    const file = event.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = async (e) => {
        try {
            const importedConfig = JSON.parse(e.target.result);
            const response = await fetchAPI('api/config/restore', {
                method: 'POST',
                body: JSON.stringify(importedConfig),
                headers: { 'Content-Type': 'application/json' }
            });
            if (response === null || response.ok) { // Assuming 200 OK or 204 No Content for success
                alert('Configuration imported successfully! ESP32 will restart if WiFi settings were changed.');
                // Optionally, refresh config after a short delay if no restart
                setTimeout(() => getConfig(), 3000);
            } else {
                alert('Failed to import configuration. Please check the file format.');
            }
        } catch (error) {
            console.error('Error importing config:', error);
            alert('An error occurred during configuration import. Invalid JSON or file format.');
        }
        event.target.value = ''; // Clear the file input
    };
    reader.readAsText(file);
}

export async function getLedConfig() {
    const json = await fetchAPI('api/leds');
    if (!json) return;
    state.ledConfig = json;
    renderLedGroups();
}

export async function saveLedConfig() {
    const container = document.getElementById('led-groups-container');
    const groups = [];
    container.querySelectorAll('.led-group-card').forEach(card => {
        const group = {
            funcion: parseInt(card.querySelector('.led-function').value, 10),
            leds: card.querySelector('.led-indices').value,
            color: {
                r: parseInt(card.querySelector('.led-color').value.substr(1, 2), 16),
                g: parseInt(card.querySelector('.led-color').value.substr(3, 2), 16),
                b: parseInt(card.querySelector('.led-color').value.substr(5, 2), 16)
            },
            brillo: parseInt(card.querySelector('.led-brightness').value, 10)
        };
        groups.push(group);
    });
    const configBody = {
        total_leds: parseInt(document.getElementById('total_leds').value, 10),
        grupos: groups
    };
    await fetchAPI('api/leds', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
    getLedConfig();
}

export function exportLedConfig() {
    const container = document.getElementById('led-groups-container');
    const groups = [];
    container.querySelectorAll('.led-group-card').forEach(card => {
        const group = {
            funcion: parseInt(card.querySelector('.led-function').value, 10),
            leds: card.querySelector('.led-indices').value,
            color: {
                r: parseInt(card.querySelector('.led-color').value.substr(1, 2), 16),
                g: parseInt(card.querySelector('.led-color').value.substr(3, 2), 16),
                b: parseInt(card.querySelector('.led-color').value.substr(5, 2), 16)
            },
            brillo: parseInt(card.querySelector('.led-brightness').value, 10)
        };
        groups.push(group);
    });
    const configToSave = {
        total_leds: parseInt(document.getElementById('total_leds').value, 10),
        grupos: groups
    };
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(configToSave, null, 2));
    const downloadAnchorNode = document.createElement('a');
    downloadAnchorNode.setAttribute("href", dataStr);
    downloadAnchorNode.setAttribute("download", "led_config.json");
    document.body.appendChild(downloadAnchorNode);
    downloadAnchorNode.click();
    downloadAnchorNode.remove();
}

export function importLedConfig(event) {
    const file = event.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (e) => {
        try {
            const importedConfig = JSON.parse(e.target.result);
            if (typeof importedConfig.total_leds !== 'number' || !Array.isArray(importedConfig.grupos)) {
                throw new Error("Invalid JSON format for LED config.");
            }
            state.ledConfig = importedConfig;
            renderLedGroups();
            alert(translations[state.currentLanguage].importSuccessAlert);
        } catch (error) {
            console.error("Failed to import LED config:", error);
            alert(translations[state.currentLanguage].importErrorAlert);
        }
        event.target.value = '';
    };
    reader.readAsText(file);
}

function camApiUrl() {
    const ip = (state.cameraInfo && state.cameraInfo.ip) || (state.wifiConfig && state.wifiConfig.camIP);
    return ip ? `http://${ip}/` : null;
}

export async function getCamConfig() {
    const url = camApiUrl();
    if (!url) return;
    const json = await fetchAPI('api/config', {}, url);
    if (json) updateForm(json);
}

export async function setCamConfig() {
    const url = camApiUrl();
    if (!url) return;
    const configBody = serializeForm('#cam .cam-controls');
    if (configBody.framesize !== undefined) configBody.framesize = parseInt(configBody.framesize, 10);
    await fetchAPI('api/config', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } }, url);
}
