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

export function connectCamWebSocket(camIP) {
    if (!camIP || state.socketCamOnline) return;
    const camWsUrl = `ws://${camIP}/ws`;
    try {
        state.wsSocketCam = new WebSocket(camWsUrl);
        state.wsSocketCam.onmessage = (event) => {
            document.getElementById('camImg').src = URL.createObjectURL(event.data);
        };
        state.wsSocketCam.onopen = () => state.socketCamOnline = true;
        state.wsSocketCam.onclose = () => state.socketCamOnline = false;
        state.wsSocketCam.onerror = (err) => console.error('Cam WS Error:', err);
    } catch (err) {
        console.error('Failed to create cam WebSocket:', err);
    }
}

export function sendWsAction(action) {
    if (!state.socketOnline) return;
    const payload = { action: action };
    state.wsSocket.send(JSON.stringify(payload));
}
