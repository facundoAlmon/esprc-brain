import { state, elements } from './state.js';
import { fetchAPI } from './api.js';
import { translations } from './translations.js';

function t(key, fallback) {
    return (translations[state.currentLanguage] && translations[state.currentLanguage][key]) || fallback;
}

function fmtBytes(n) {
    if (n == null || isNaN(n)) return '-';
    if (n < 1024) return `${n} B`;
    if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KB`;
    return `${(n / (1024 * 1024)).toFixed(2)} MB`;
}

export async function refreshOtaInfo() {
    const info = await fetchAPI('api/ota/info');
    const infoEl = document.getElementById('ota-info');
    if (!infoEl) return;
    if (!info) {
        infoEl.textContent = t('otaInfoUnavailable', 'OTA info unavailable.');
        return;
    }
    const rows = [];
    if (info.running) {
        rows.push(`<dt>${t('otaRunningPartition', 'Running partition')}</dt>` +
                  `<dd>${info.running.label} @ 0x${info.running.address.toString(16)} (${fmtBytes(info.running.size)})</dd>`);
    }
    if (info.next) {
        rows.push(`<dt>${t('otaNextPartition', 'Target slot')}</dt>` +
                  `<dd>${info.next.label} (${fmtBytes(info.next.size)})</dd>`);
    }
    if (info.app) {
        if (info.app.version) {
            rows.push(`<dt>${t('otaAppVersion', 'App version')}</dt><dd>${info.app.version}</dd>`);
        }
        if (info.app.compile_date) {
            rows.push(`<dt>${t('otaBuildDate', 'Built')}</dt>` +
                      `<dd>${info.app.compile_date} ${info.app.compile_time || ''}</dd>`);
        }
        if (info.app.idf_version) {
            rows.push(`<dt>IDF</dt><dd>${info.app.idf_version}</dd>`);
        }
    }
    infoEl.innerHTML = `<dl class="ota-info-grid">${rows.join('')}</dl>`;
}

function setOtaStatus(text, isError = false) {
    const el = document.getElementById('ota-status');
    if (!el) return;
    el.textContent = text || '';
    el.classList.toggle('error', !!isError);
}

function setOtaProgress(percent, label) {
    const bar = document.getElementById('ota-progress-bar');
    const wrap = document.getElementById('ota-progress');
    const text = document.getElementById('ota-progress-text');
    if (wrap) wrap.style.display = percent == null ? 'none' : 'block';
    if (bar) bar.style.width = `${Math.max(0, Math.min(100, percent || 0))}%`;
    if (text) text.textContent = label || (percent != null ? `${percent.toFixed(1)}%` : '');
}

let selectedFile = null;

function setSelectedFile(file) {
    selectedFile = file;
    const label = document.getElementById('ota-file-label');
    const zone  = document.getElementById('ota-drop-zone');
    if (!label || !zone) return;
    if (file) {
        label.removeAttribute('data-i18n');
        label.textContent = `${file.name} (${fmtBytes(file.size)})`;
        zone.classList.add('has-file');
    } else {
        label.setAttribute('data-i18n', 'otaDropZone');
        label.textContent = t('otaDropZone', 'Drop .bin file here or click to choose');
        zone.classList.remove('has-file');
    }
}

export async function uploadFirmware() {
    const input = document.getElementById('ota-file-input');
    const uploadBtn = document.getElementById('ota-upload-btn');
    const file = selectedFile || (input && input.files && input.files[0]);
    if (!file) {
        alert(t('otaSelectFile', 'Please select a firmware .bin file first.'));
        return;
    }

    if (!file.name.toLowerCase().endsWith('.bin')) {
        if (!confirm(t('otaNotBinConfirm', 'File does not end with .bin. Continue anyway?'))) return;
    }

    if (!confirm(t('otaConfirmStart', `Upload ${file.name} (${(file.size/1024/1024).toFixed(2)} MB) and reboot?`))) return;

    uploadBtn.disabled = true;
    setOtaStatus(t('otaUploading', 'Uploading firmware...'));
    setOtaProgress(0, '0%');

    // Usamos XMLHttpRequest porque fetch() todavía no expone un stream de progreso
    // de subida portable en todos los navegadores.
    const url = (elements.urlAPI.value || '') + 'api/ota';
    const xhr = new XMLHttpRequest();

    xhr.upload.addEventListener('progress', (e) => {
        if (e.lengthComputable) {
            const pct = (e.loaded / e.total) * 100;
            setOtaProgress(pct, `${pct.toFixed(1)}% (${fmtBytes(e.loaded)} / ${fmtBytes(e.total)})`);
        }
    });

    xhr.onreadystatechange = () => {
        if (xhr.readyState !== 4) return;
        uploadBtn.disabled = false;
        if (xhr.status >= 200 && xhr.status < 300) {
            setOtaProgress(100, '100%');
            setOtaStatus(t('otaSuccess', 'Firmware uploaded. The device is restarting...'));
            setTimeout(() => {
                setOtaStatus(t('otaPostReboot', 'Reconnect to the ESP and reload this page after a few seconds.'));
                refreshOtaInfo();
            }, 8000);
        } else {
            const detail = xhr.responseText || `HTTP ${xhr.status}`;
            setOtaStatus(`${t('otaFailed', 'OTA failed:')} ${detail}`, true);
            setOtaProgress(null);
        }
    };
    xhr.onerror = () => {
        uploadBtn.disabled = false;
        setOtaStatus(t('otaNetworkError', 'Network error during OTA upload.'), true);
        setOtaProgress(null);
    };

    xhr.open('POST', url, true);
    xhr.setRequestHeader('Content-Type', 'application/octet-stream');
    xhr.send(file);
}

export function initOta() {
    const refreshBtn = document.getElementById('ota-refresh-btn');
    if (refreshBtn) refreshBtn.addEventListener('click', refreshOtaInfo);

    const uploadBtn = document.getElementById('ota-upload-btn');
    if (uploadBtn) uploadBtn.addEventListener('click', uploadFirmware);

    const fileInput = document.getElementById('ota-file-input');
    if (fileInput) {
        fileInput.addEventListener('change', () => {
            const f = fileInput.files && fileInput.files[0];
            setSelectedFile(f || null);
        });
    }

    const zone = document.getElementById('ota-drop-zone');
    if (zone) {
        zone.addEventListener('click', () => {
            const inp = document.getElementById('ota-file-input');
            if (inp) inp.click();
        });

        zone.addEventListener('dragover', (e) => {
            e.preventDefault();
            e.stopPropagation();
            zone.classList.add('drag-over');
        });

        zone.addEventListener('dragenter', (e) => {
            e.preventDefault();
            e.stopPropagation();
            zone.classList.add('drag-over');
        });

        zone.addEventListener('dragleave', (e) => {
            e.stopPropagation();
            if (!zone.contains(e.relatedTarget)) {
                zone.classList.remove('drag-over');
            }
        });

        zone.addEventListener('drop', (e) => {
            e.preventDefault();
            e.stopPropagation();
            zone.classList.remove('drag-over');
            const files = e.dataTransfer && e.dataTransfer.files;
            if (files && files.length > 0) {
                setSelectedFile(files[0]);
            }
        });
    }
}
