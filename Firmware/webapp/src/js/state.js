// Main application state
export const state = {
    hostname: localStorage.getItem('hostname') || window.location.hostname,
    config: {},
    wifiConfig: {},
    ledConfig: { total_leds: 12, grupos: [] },
    lightsState: {
        turnLeft: false,
        turnRight: false,
        hazard: false,
        headlights: 0, // 0: off, 1: position, 2: low, 3: high
        acelIzq: false
    },
    activeTab: 'joystick-a',
    wsSocket: null,
    wsSocketCam: null,
    socketOnline: false,
    socketCamOnline: false,
    currentLanguage: 'en',
    tooltipTimeout: null,
    joystickLayoutSwapped: false,
    kidSequence: [],
    kidModeActiveCommand: null,
    kidModeInterval: null,
    programSequence: [],
    draggedIndex: null,
    recordingProgram: false
};

export const icons = {
    turn_left: `<svg  xmlns="http://www.w3.org/2000/svg"  width="24"  height="24"  viewBox="0 0 24 24"  fill="none"  stroke="currentColor"  stroke-width="2"  stroke-linecap="round"  stroke-linejoin="round"  class="icon icon-tabler icons-tabler-outline icon-tabler-arrow-big-left-lines"><path stroke="none" d="M0 0h24v24H0z" fill="none"/><path stroke="none" fill="currentColor" d="M12 15v3.586a1 1 0 0 1 -1.707 .707l-6.586 -6.586a1 1 0 0 1 0 -1.414l6.586 -6.586a1 1 0 0 1 1.707 .707v3.586h3v6h-3z" /><path d="M21 15v-6" /><path d="M18 15v-6" /></svg>`,
    turn_right: `<svg  xmlns="http://www.w3.org/2000/svg"  width="24"  height="24"  viewBox="0 0 24 24"  fill="none"  stroke="currentColor"  stroke-width="2"  stroke-linecap="round"  stroke-linejoin="round"  class="icon icon-tabler icons-tabler-outline icon-tabler-arrow-big-right-lines"><path stroke="none" fill="none" d="M0 0h24v24H0z" /><path stroke="none" fill="currentColor" d="M12 9v-3.586a1 1 0 0 1 1.707 -.707l6.586 6.586a1 1 0 0 1 0 1.414l-6.586 6.586a1 1 0 0 1 -1.707 -.707v-3.586h-3v-6h3z" /><path d="M3 9v6" /><path d="M6 9v6" /></svg>`,
    hazard: `<svg  xmlns="http://www.w3.org/2000/svg"  width="24"  height="24"  viewBox="0 0 24 24"  fill="none"  stroke="currentColor"  stroke-width="2"  stroke-linecap="round"  stroke-linejoin="round"  class="icon icon-tabler icons-tabler-outline icon-tabler-alert-triangle"><path stroke="none" d="M0 0h24v24H0z" fill="none"/><path d="M12 9v4" /><path fill="none"  stroke="currentColor" d="M10.363 3.591l-8.106 13.534a1.914 1.914 0 0 0 1.636 2.871h16.214a1.914 1.914 0 0 0 1.636 -2.87l-8.106 -13.536a1.914 1.914 0 0 0 -3.274 0z" /><path d="M12 16h.01" /></svg>`,
    headlights_off: `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">    <mask id="lineMdCarLightDimmedOff0">        <g fill="none" stroke="#fff" stroke-linecap="round" stroke-linejoin="round" stroke-width="2">            <path stroke-dasharray="12" stroke-dashoffset="0" d="M12 5.5l-9 2.5"/>            <path stroke-dasharray="12" stroke-dashoffset="0" d="M12 10.5l-9 2.5"/>            <path stroke-dasharray="12" stroke-dashoffset="0" d="M12 15.5l-9 2.5"/>            <path stroke="#000" stroke-width="6" d="M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>            <path stroke-dasharray="40" stroke-dashoffset="0" d="M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>            <path stroke="#000" stroke-dasharray="28" stroke-dashoffset="0" d="M-1 11h26" transform="rotate(45 12 12)"/>            <path stroke-dasharray="28" stroke-dashoffset="0" d="M-1 13h26" transform="rotate(45 12 12)"/>        </g>    </mask>    <rect width="24" height="24" fill="currentColor" mask="url(#lineMdCarLightDimmedOff0)"/></svg>`,
    headlights_pos: `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path fill="currentColor" d="M13 4.8c-4 0-4 14.4 0 14.4s9-2.7 9-7.2s-5-7.2-9-7.2m.1 12.4C12.7 16.8 12 15 12 12s.7-4.8 1.1-5.2C16 6.9 20 8.7 20 12c0 3.3-4.1 5.1-6.9 5.2M8 10.5c0 .5-.1 1-.1 1.5v.6L2.4 14l-.5-1.9L8 10.5M2 7l7.4-1.9c-.2.3-.4.7-.5 1.2c-.1.3-.2.7-.3 1.1L2.5 8.9L2 7m6.2 8.5c.1.7.3 1.4.5 1.9L2.4 19l-.5-1.9l6.3-1.6Z"/></svg>`,
    headlights_low: `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">    <mask id="lineMdCarLightFilled0">        <g fill="none" stroke="#fff" stroke-linecap="round" stroke-linejoin="round" stroke-width="2">            <path stroke-dasharray="8" fill="none" stroke-dashoffset="0" d="M11 6h-6"/>            <path stroke-dasharray="8" fill="none" stroke-dashoffset="0" d="M11 10h-6"/>            <path stroke-dasharray="8" fill="none" stroke-dashoffset="0" d="M11 14h-6"/>            <path stroke-dasharray="8" fill="none" stroke-dashoffset="0" d="M11 18h-6"/>            <path fill="none" stroke="#000" stroke-width="6"                d="M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z" />            <path fill="none" fill-opacity="1" stroke-dasharray="40" stroke-dashoffset="0"                d="M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>        </g>    </mask>    <rect width="24" height="24" fill="currentColor" mask="url(#lineMdCarLightFilled0)"/></svg>`,
    headlights_high: `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">    <mask id="lineMdCarLightFilled0">        <g fill="none"    stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" stroke-width="2">            <path stroke-dasharray="8" stroke-dashoffset="0" d="M11 6h-6"/>            <path stroke-dasharray="8" stroke-dashoffset="0" d="M11 10h-6"/>            <path stroke-dasharray="8" stroke-dashoffset="0" d="M11 14h-6"/>            <path stroke-dasharray="8" stroke-dashoffset="0" d="M11 18h-6"/>            <path stroke="#000" stroke-width="6"                d="M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z" />            <path fill="#fff" fill-opacity="1" stroke-dasharray="40" stroke-dashoffset="0"                d="M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>        </g>    </mask>    <rect width="24" height="24" fill="currentColor" mask="url(#lineMdCarLightFilled0)"/></svg>`,
    bluetooth: `<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200" viewBox="0 0 20 20"><path fill="currentColor" d="m9.41 0l6 6l-4 4l4 4l-6 6H9v-7.59l-3.3 3.3l-1.4-1.42L8.58 10l-4.3-4.3L5.7 4.3L9 7.58V0h.41zM11 4.41V7.6L12.59 6L11 4.41zM12.59 14L11 12.41v3.18L12.59 14z"/></svg>`,
    default: `<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200" viewBox="0 0 21 21"><path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="m8.5 10.5l-4 4l4 4m8-4h-12m8-12l4 4l-4 4m4-4h-12"/></svg>`,
    acelIzq: `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="icon icon-tabler icons-tabler-outline icon-tabler-switch-horizontal"><path stroke="none" d="M0 0h24v24H0z" fill="none"/><path d="M16 3l4 4l-4 4" /><path d="M10 7l10 0" /><path d="M8 13l-4 4l4 4" /><path d="M4 17l9 0" /></svg>`,
    swap: `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="icon icon-tabler icons-tabler-outline icon-tabler-switch-horizontal"><path stroke="none" d="M0 0h24v24H0z" fill="none"/><path d="M16 3l4 4l-4 4" /><path d="M10 7l10 0" /><path d="M8 13l-4 4l4 4" /><path d="M4 17l9 0" /></svg>`,
    recordOff: `<svg fill="none" width="800px" height="800px" viewBox="0 0 1920 1920" xmlns="http://www.w3.org/2000/svg">
                    <path fill="currentColor" d="M960 0c529.36 0 960 430.645 960 960 0 529.36-430.64 960-960 960-529.355 0-960-430.64-960-960C0 430.645 430.645 0 960 0Zm0 112.941c-467.125 0-847.059 379.934-847.059 847.059 0 467.12 379.934 847.06 847.059 847.06 467.12 0 847.06-379.94 847.06-847.06 0-467.125-379.94-847.059-847.06-847.059Zm0 313.726c294.55 0 533.33 238.781 533.33 533.333 0 294.55-238.78 533.33-533.33 533.33-294.552 0-533.333-238.78-533.333-533.33 0-294.552 238.781-533.333 533.333-533.333Z" fill-rule="evenodd"/>
                </svg>`,
    recordOn: `<svg fill="none" width="800px" height="800px" viewBox="0 0 1920 1920" xmlns="http://www.w3.org/2000/svg">
                    <path fill="currentColor" d="M960 0c529.36 0 960 430.645 960 960 0 529.36-430.64 960-960 960-529.355 0-960-430.64-960-960C0 430.645 430.645 0 960 0Zm0 112.941c-467.125 0-847.059 379.934-847.059 847.059 0 467.12 379.934 847.06 847.059 847.06 467.12 0 847.06-379.94 847.06-847.06 0-467.125-379.94-847.059-847.06-847.059Zm0 313.726c294.55 0 533.33 238.781 533.33 533.333 0 294.55-238.78 533.33-533.33 533.33-294.552 0-533.333-238.78-533.333-533.33 0-294.552 238.781-533.333 533.333-533.333Z" fill-rule="evenodd"/>
                </svg>`
};

export const elements = {};
