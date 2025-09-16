import { state, elements } from './state.js';
import { fetchAPI } from './api.js';
import { updateForm, serializeForm } from './ui.js';
import { translations } from './translations.js';
import { renderLedGroups } from './leds.js';

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
}

export async function saveConfig() {
    const configBody = serializeForm('#config');
    await fetchAPI('config', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
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

export async function getCamConfig() {
    const camApiUrl = `${window.location.protocol}//${state.wifiConfig.camIP}/`;
    const json = await fetchAPI('cam-config', {}, camApiUrl);
    if (json) updateForm(json);
}

export async function setCamConfig() {
    const camApiUrl = `${window.location.protocol}//${state.wifiConfig.camIP}/`;
    const configBody = serializeForm('#cam .cam-controls');
    await fetchAPI('cam-config', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } }, camApiUrl);
}
