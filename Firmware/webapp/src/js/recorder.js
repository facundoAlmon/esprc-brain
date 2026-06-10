import { state } from './state.js';
import { translations } from './translations.js';
import { connectCamMjpeg } from './api.js';

// Webapp-only MJPEG video recorder.
//
// Browsers do NOT refresh a multipart/x-mixed-replace <img> when it is drawn
// onto a canvas — drawImage() only ever yields the first JPEG frame, so the
// recorder can't simply copy the <img>. Instead, while recording, it takes
// over the camera's single MJPEG connection: it fetch()es the stream, splits
// it into JPEG frames (FFD8..FFD9 markers), paints each frame onto a hidden
// canvas (recorded via captureStream + MediaRecorder) and keeps the on-screen
// <img> live by swapping per-frame blob URLs. When recording stops, the
// normal <img> stream connection is restored.

const REC_MAX_FPS = 25;

let active = false;
let aborter = null;
let mediaRecorder = null;
let chunks = [];
let recMime = '';
let clockTimer = null;
let recStartTs = 0;
let streamUrl = '';
let prevFrameUrl = null;
let frameBusy = false;
let canvas = null;
let ctx = null;

function t(key) {
    return (translations[state.currentLanguage] && translations[state.currentLanguage][key]) || key;
}

function sleep(ms) {
    return new Promise(function(resolve) { setTimeout(resolve, ms); });
}

function pickMimeType() {
    if (!window.MediaRecorder) return null;
    const candidates = ['video/webm;codecs=vp9', 'video/webm;codecs=vp8', 'video/webm', 'video/mp4'];
    for (let i = 0; i < candidates.length; i++) {
        if (MediaRecorder.isTypeSupported(candidates[i])) return candidates[i];
    }
    return '';
}

function camMjpegUrl() {
    if (state.cameraInfo && state.cameraInfo.mjpegUrl) return state.cameraInfo.mjpegUrl;
    const ip = state.wifiConfig && state.wifiConfig.camIP;
    return ip ? 'http://' + ip + ':81/mjpeg' : null;
}

export function isVideoRecording() {
    return active;
}

export function toggleVideoRecording() {
    if (active) stopVideoRecording();
    else startVideoRecording();
}

export async function startVideoRecording() {
    if (active) return;
    const url = camMjpegUrl();
    if (!url) { alert(t('recNoStream')); return; }
    if (pickMimeType() === null || !window.AbortController ||
        !window.createImageBitmap || !HTMLCanvasElement.prototype.captureStream) {
        alert(t('recUnsupported'));
        return;
    }

    active = true;
    streamUrl = url;
    canvas = null;
    ctx = null;
    chunks = [];
    updateRecUI(true);

    // Free the camera's single MJPEG connection before fetching it ourselves
    const img = document.getElementById('camImg');
    if (img && img.getAttribute('src')) img.src = '';
    await sleep(400);
    if (!active) return; // stopped while waiting

    aborter = new AbortController();
    readMjpegStream(streamUrl, aborter.signal, handleFrame).catch(function(e) {
        if (active) {
            console.error('MJPEG fetch failed:', e);
            stopVideoRecording();
            alert(t('recNoStream'));
        }
    });
}

export function stopVideoRecording() {
    if (!active) return;
    active = false;
    if (aborter) { aborter.abort(); aborter = null; }
    if (clockTimer) { clearInterval(clockTimer); clockTimer = null; }
    setRecBadges('');
    updateRecUI(false);
    if (mediaRecorder) {
        try { mediaRecorder.stop(); } catch (e) {}
        mediaRecorder = null;
    }
    if (prevFrameUrl) {
        const u = prevFrameUrl;
        prevFrameUrl = null;
        setTimeout(function() { URL.revokeObjectURL(u); }, 2000);
    }
    // Reconnect the live <img> stream once the fetch socket has closed
    const url = streamUrl;
    setTimeout(function() { connectCamMjpeg(url); }, 400);
}

// ── MJPEG fetch + frame splitting ────────────

async function readMjpegStream(url, signal, onFrame) {
    const resp = await fetch(url, { signal: signal, cache: 'no-store' });
    if (!resp.ok || !resp.body) throw new Error('HTTP ' + resp.status);
    const reader = resp.body.getReader();
    let buf = new Uint8Array(0);
    for (;;) {
        const chunk = await reader.read();
        if (chunk.done) break;
        const merged = new Uint8Array(buf.length + chunk.value.length);
        merged.set(buf, 0);
        merged.set(chunk.value, buf.length);
        buf = merged;
        for (;;) {
            const soi = indexOfMarker(buf, 0xD8, 0);
            if (soi < 0) {
                // keep last byte: it may be the 0xFF of a marker split across chunks
                buf = buf.length > 1 ? buf.slice(buf.length - 1) : buf;
                break;
            }
            const eoi = indexOfMarker(buf, 0xD9, soi + 2);
            if (eoi < 0) {
                if (soi > 0) buf = buf.slice(soi);
                break;
            }
            onFrame(new Blob([buf.slice(soi, eoi + 2)], { type: 'image/jpeg' }));
            buf = buf.slice(eoi + 2);
        }
        if (buf.length > 2000000) buf = new Uint8Array(0); // runaway safety
    }
}

function indexOfMarker(buf, secondByte, from) {
    for (let i = from; i < buf.length - 1; i++) {
        if (buf[i] === 0xFF && buf[i + 1] === secondByte) return i;
    }
    return -1;
}

// ── Frame handling ───────────────────────────

function handleFrame(blob) {
    if (!active || frameBusy) return; // drop frames while a decode is in flight
    frameBusy = true;
    createImageBitmap(blob).then(function(bmp) {
        if (!active) { bmp.close(); frameBusy = false; return; }
        if (!canvas) {
            canvas = document.createElement('canvas');
            canvas.width = bmp.width;
            canvas.height = bmp.height;
            ctx = canvas.getContext('2d');
            ctx.drawImage(bmp, 0, 0);
            setupRecorder();
        } else {
            if (canvas.width !== bmp.width || canvas.height !== bmp.height) {
                canvas.width = bmp.width;
                canvas.height = bmp.height;
            }
            ctx.drawImage(bmp, 0, 0);
        }
        bmp.close();
        showFrame(blob);
        frameBusy = false;
    }).catch(function() { frameBusy = false; });
}

// Keep the on-screen <img> live while we own the stream connection
function showFrame(blob) {
    const img = document.getElementById('camImg');
    if (!img) return;
    const u = URL.createObjectURL(blob);
    img.src = u;
    if (prevFrameUrl) URL.revokeObjectURL(prevFrameUrl);
    prevFrameUrl = u;
}

// ── MediaRecorder lifecycle (starts on first decoded frame) ──

function setupRecorder() {
    const mime = pickMimeType();
    const stream = canvas.captureStream(REC_MAX_FPS);
    try {
        mediaRecorder = mime
            ? new MediaRecorder(stream, { mimeType: mime, videoBitsPerSecond: 4000000 })
            : new MediaRecorder(stream);
    } catch (e) {
        console.error('MediaRecorder failed:', e);
        stopVideoRecording();
        alert(t('recUnsupported'));
        return;
    }
    recMime = mediaRecorder.mimeType || mime || 'video/webm';
    mediaRecorder.ondataavailable = function(e) { if (e.data && e.data.size > 0) chunks.push(e.data); };
    mediaRecorder.onstop = saveRecording;
    mediaRecorder.start(1000);
    recStartTs = Date.now();
    clockTimer = setInterval(updateRecClock, 500);
    updateRecClock();
}

function saveRecording() {
    if (!chunks.length) return;
    const blob = new Blob(chunks, { type: recMime });
    chunks = [];
    const ext = recMime.indexOf('mp4') !== -1 ? 'mp4' : 'webm';
    const d = new Date();
    const pad = function(n) { return (n < 10 ? '0' : '') + n; };
    const name = 'esprc_' + d.getFullYear() + pad(d.getMonth() + 1) + pad(d.getDate()) +
                 '_' + pad(d.getHours()) + pad(d.getMinutes()) + pad(d.getSeconds()) + '.' + ext;
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = name;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(function() { URL.revokeObjectURL(url); }, 10000);
}

// ── UI helpers ───────────────────────────────

function updateRecUI(isOn) {
    document.querySelectorAll('.cam-rec-btn').forEach(function(b) { b.classList.toggle('active', isOn); });
}

function setRecBadges(text) {
    ['recTime', 'recTimeFpv'].forEach(function(id) {
        const el = document.getElementById(id);
        if (el) el.textContent = text;
    });
}

function updateRecClock() {
    const s = Math.floor((Date.now() - recStartTs) / 1000);
    const pad = function(n) { return (n < 10 ? '0' : '') + n; };
    setRecBadges('REC ' + pad(Math.floor(s / 60)) + ':' + pad(s % 60));
}
