import { state, elements, icons } from './state.js';
import { updateButtonIcon } from './ui.js';
import { sendWsAction } from './api.js';

export function initLights() {
    ['A', 'B'].forEach(tab => {
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
    elements.turnLeftBtnA.classList.toggle('active', turnLeft && !hazard);
    elements.turnRightBtnA.classList.toggle('active', turnRight && !hazard);
    elements.hazardBtnA.classList.toggle('active', hazard);
    elements.turnLeftBtnB.classList.toggle('active', turnLeft && !hazard);
    elements.turnRightBtnB.classList.toggle('active', turnRight && !hazard);
    elements.hazardBtnB.classList.toggle('active', hazard);
    elements.acelIzq.classList.toggle('active', acelIzq);
    if (state.activeTab === 'joystick-a') {
        elements.turnLeftBtnA.innerHTML = icons.turn_left;
        elements.turnRightBtnA.innerHTML = icons.turn_right;
        elements.hazardBtnA.innerHTML = icons.hazard;
        updateButtonIcon(elements.headlightsBtnA, state.lightsState.headlights > 0, 'headlights');
        elements.turnLeftBtnB.innerHTML = '';
        elements.turnRightBtnB.innerHTML = '';
        elements.hazardBtnB.innerHTML = '';
        elements.headlightsBtnB.innerHTML = '';
    } else if (state.activeTab === 'joystick-b') {
        elements.turnLeftBtnA.innerHTML = '';
        elements.turnRightBtnA.innerHTML = '';
        elements.hazardBtnA.innerHTML = '';
        elements.headlightsBtnA.innerHTML = '';
        elements.turnLeftBtnB.innerHTML = icons.turn_left;
        elements.turnRightBtnB.innerHTML = icons.turn_right;
        elements.hazardBtnB.innerHTML = icons.hazard;
        updateButtonIcon(elements.headlightsBtnB, state.lightsState.headlights > 0, 'headlights');
    } else {
        elements.turnLeftBtnA.innerHTML = '';
        elements.turnRightBtnA.innerHTML = '';
        elements.hazardBtnA.innerHTML = '';
        elements.headlightsBtnA.innerHTML = '';
        elements.turnLeftBtnB.innerHTML = '';
        elements.turnRightBtnB.innerHTML = '';
        elements.hazardBtnB.innerHTML = '';
        elements.headlightsBtnB.innerHTML = '';
    }
    elements.swapLayoutBtnA.innerHTML = icons.swap;
}
