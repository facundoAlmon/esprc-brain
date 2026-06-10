import { state, elements, icons } from './state.js';
import { updateButtonIcon } from './ui.js';
import { sendWsAction } from './api.js';

const LIGHT_BTN_SUFFIXES = ['A', 'B', 'Fpv'];
const TAB_TO_SUFFIX = { 'joystick-a': 'A', 'joystick-b': 'B', 'fpv': 'Fpv' };

export function initLights() {
    LIGHT_BTN_SUFFIXES.forEach(tab => {
        if (!elements[`turnLeftBtn${tab}`]) return;
        elements[`turnLeftBtn${tab}`].addEventListener('click', () => handleTurnSignal('left'));
        elements[`turnRightBtn${tab}`].addEventListener('click', () => handleTurnSignal('right'));
        elements[`hazardBtn${tab}`].addEventListener('click', () => handleHazard());
        elements[`headlightsBtn${tab}`].addEventListener('click', () => handleHeadlights());
    });
    elements.acelIzq.addEventListener('click', () => {
        state.lightsState.acelIzq = !state.lightsState.acelIzq;
        elements.acelIzq.classList.toggle('active', state.lightsState.acelIzq);
    });
    elements.acelIzq.innerHTML = icons.acelIzq;
    updateLightsUI();
}

function handleTurnSignal(direction) {
    state.lightsState.turnLeft = (direction === 'left') ? !state.lightsState.turnLeft : false;
    state.lightsState.turnRight = (direction === 'right') ? !state.lightsState.turnRight : false;
    if (state.lightsState.hazard) {
        state.lightsState.hazard = false;
        sendWsAction('hazards_toggle');
    }
    sendWsAction(direction === 'left' ? 'left_turn_toggle' : 'right_turn_toggle');
    updateLightsUI();
}

function handleHazard() {
    state.lightsState.hazard = !state.lightsState.hazard;
    state.lightsState.turnLeft = false;
    state.lightsState.turnRight = false;
    sendWsAction('hazards_toggle');
    updateLightsUI();
}

function handleHeadlights() {
    state.lightsState.headlights = (state.lightsState.headlights + 1) % 4;
    sendWsAction('headlights_cycle');
    updateLightsUI();
}

export function updateLightsUI() {
    const { turnLeft, turnRight, hazard, acelIzq } = state.lightsState;
    const activeSuffix = TAB_TO_SUFFIX[state.activeTab];
    LIGHT_BTN_SUFFIXES.forEach(tab => {
        if (!elements[`turnLeftBtn${tab}`]) return;
        elements[`turnLeftBtn${tab}`].classList.toggle('active', turnLeft && !hazard);
        elements[`turnRightBtn${tab}`].classList.toggle('active', turnRight && !hazard);
        elements[`hazardBtn${tab}`].classList.toggle('active', hazard);
        if (tab === activeSuffix) {
            elements[`turnLeftBtn${tab}`].innerHTML = icons.turn_left;
            elements[`turnRightBtn${tab}`].innerHTML = icons.turn_right;
            elements[`hazardBtn${tab}`].innerHTML = icons.hazard;
            updateButtonIcon(elements[`headlightsBtn${tab}`], state.lightsState.headlights > 0, 'headlights');
        } else {
            elements[`turnLeftBtn${tab}`].innerHTML = '';
            elements[`turnRightBtn${tab}`].innerHTML = '';
            elements[`hazardBtn${tab}`].innerHTML = '';
            elements[`headlightsBtn${tab}`].innerHTML = '';
        }
    });
    elements.acelIzq.classList.toggle('active', acelIzq);
    elements.swapLayoutBtnA.innerHTML = icons.swap;
}
