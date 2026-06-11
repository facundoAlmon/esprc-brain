import { elements } from './state.js';
import { fetchAPI } from './api.js';

const PIN_FIELDS = [
    { key: 'pinLedStrip',   id: 'pin-led-strip',   label: 'LED Strip' },
    { key: 'pinMotorEn',    id: 'pin-motor-en',    label: 'Motor EN (PWM)' },
    { key: 'pinMotorDir1',  id: 'pin-motor-dir1',  label: 'Motor DIR1' },
    { key: 'pinMotorDir2',  id: 'pin-motor-dir2',  label: 'Motor DIR2' },
    { key: 'pinSteerServo', id: 'pin-steer-servo', label: 'Steering Servo' },
    { key: 'pinPanServo',   id: 'pin-pan-servo',   label: 'Pan Servo' },
    { key: 'pinTiltServo',  id: 'pin-tilt-servo',  label: 'Tilt Servo' },
];

let pinsData = null;

function setPinsStatus(msg, isError) {
    var el = document.getElementById('pins-status');
    if (!el) return;
    el.textContent = msg || '';
    el.className = 'pins-status' + (isError ? ' error' : '');
}

export async function loadPins() {
    pinsData = await fetchAPI('api/pins');
    if (!pinsData) { setPinsStatus('Could not load pin config.', true); return; }
    PIN_FIELDS.forEach(function(f) {
        var inp = document.getElementById(f.id);
        if (inp) inp.value = pinsData[f.key];
        var hint = document.getElementById(f.id + '-default');
        if (hint && pinsData.defaults) hint.textContent = 'default: ' + pinsData.defaults[f.key];
    });
    var chipEl = document.getElementById('pins-chip-type');
    if (chipEl) chipEl.textContent = pinsData.chipType || '';
    setPinsStatus('');
}

export async function savePins() {
    var body = {};
    var valid = true;
    PIN_FIELDS.forEach(function(f) {
        var inp = document.getElementById(f.id);
        if (!inp) return;
        var v = parseInt(inp.value, 10);
        if (isNaN(v) || v < 0 || v > 48) { valid = false; setPinsStatus('Invalid GPIO number in ' + f.label, true); return; }
        body[f.key] = v;
    });
    if (!valid) return;
    if (!confirm('Save pin configuration and reboot the ESP?')) return;
    setPinsStatus('Saving and rebooting...');
    var btn = document.getElementById('pins-save-btn');
    if (btn) btn.disabled = true;
    var baseUrl = elements.urlAPI && elements.urlAPI.value ? elements.urlAPI.value : '';
    try {
        var res = await fetch(baseUrl + 'api/pins', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
        if (res.ok) {
            setPinsStatus('Saved. Reconnect after reboot.');
        } else {
            setPinsStatus('Error: ' + res.status, true);
            if (btn) btn.disabled = false;
        }
    } catch (e) {
        setPinsStatus('Network error. Device may be rebooting.', false);
    }
}

export async function resetPins() {
    if (!confirm('Reset all pins to compile-time defaults and reboot?')) return;
    setPinsStatus('Resetting and rebooting...');
    var btn = document.getElementById('pins-reset-btn');
    if (btn) btn.disabled = true;
    var baseUrl = elements.urlAPI && elements.urlAPI.value ? elements.urlAPI.value : '';
    try {
        var res = await fetch(baseUrl + 'api/pins/reset', { method: 'POST' });
        if (res.ok) {
            setPinsStatus('Reset to defaults. Reconnect after reboot.');
        } else {
            setPinsStatus('Error: ' + res.status, true);
            if (btn) btn.disabled = false;
        }
    } catch (e) {
        setPinsStatus('Network error. Device may be rebooting.', false);
    }
}

export function initPins() {
    var saveBtn = document.getElementById('pins-save-btn');
    if (saveBtn) saveBtn.addEventListener('click', savePins);
    var resetBtn = document.getElementById('pins-reset-btn');
    if (resetBtn) resetBtn.addEventListener('click', resetPins);
}
