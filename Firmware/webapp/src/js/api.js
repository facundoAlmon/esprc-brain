import { state, elements } from './state.js';

export async function fetchAPI(endpoint, options = {}, baseUrl = null) {
    const url = (baseUrl || elements.urlAPI.value) + endpoint;
    try {
        const response = await fetch(url, options);
        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        if (response.status === 204 || (options.method === 'POST' && response.headers.get("Content-Length") === "2")) return null;
        return await response.json();
    } catch (error) {
        console.error(`API call to ${endpoint} failed:`, error);
        return null;
    }
}

export function connectWebSockets() {
    try {
        state.wsSocket = new WebSocket(elements.urlWS.value);
        state.wsSocket.onopen = () => state.socketOnline = true;
        state.wsSocket.onclose = () => state.socketOnline = false;
        state.wsSocket.onerror = (err) => console.error('Main WS Error:', err);
    } catch (err) {
        console.error('Failed to create main WebSocket:', err);
    }
}

export function wsReconnect() {
    connectWebSockets();
}

// Connect camera via MJPEG (lowest-latency, no memory leak vs. blob URLs).
// mjpegUrl: full URL like "http://192.168.x.x/mjpeg"
export function connectCamMjpeg(mjpegUrl) {
    if (!mjpegUrl) return;
    const img = document.getElementById('camImg');
    if (!img) return;
    img.src = mjpegUrl;
    state.socketCamOnline = true;
    img.onerror = () => { state.socketCamOnline = false; };
}

// Legacy WebSocket camera stream (kept for backward compatibility).
// Uses canvas + createImageBitmap to avoid memory leaks from object URLs.
export function connectCamWebSocket(camIP) {
    if (!camIP || state.socketCamOnline) return;
    const camWsUrl = `ws://${camIP}/ws`;
    try {
        state.wsSocketCam = new WebSocket(camWsUrl);
        const canvas = document.getElementById('camCanvas');
        const ctx = canvas ? canvas.getContext('2d') : null;
        state.wsSocketCam.binaryType = 'blob';
        state.wsSocketCam.onmessage = async (event) => {
            if (ctx) {
                const bitmap = await createImageBitmap(event.data);
                canvas.width = bitmap.width;
                canvas.height = bitmap.height;
                ctx.drawImage(bitmap, 0, 0);
                bitmap.close();
            }
        };
        state.wsSocketCam.onopen = () => state.socketCamOnline = true;
        state.wsSocketCam.onclose = () => state.socketCamOnline = false;
        state.wsSocketCam.onerror = (err) => console.error('Cam WS Error:', err);
    } catch (err) {
        console.error('Failed to create cam WebSocket:', err);
    }
}

// Auto-discover camera from brain and connect MJPEG stream.
export async function autoConnectCamera() {
    try {
        const r = await fetch('/api/camera');
        if (!r.ok) return;
        const d = await r.json();
        state.cameraInfo = d.available ? d : null;
        if (d.available && d.mjpegUrl) {
            connectCamMjpeg(d.mjpegUrl);
        }
    } catch (_) {}
}

export function sendWsAction(action) {
    if (!state.socketOnline) return;
    const payload = { action: action };
    state.wsSocket.send(JSON.stringify(payload));
}
