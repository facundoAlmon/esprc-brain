import { state, elements } from './state.js';
import { fetchAPI } from './api.js';

export function handleKidModeCommand(command) {
    state.kidSequence.push(command);
    renderKidSequence();
}

function renderKidSequence() {
    elements.kidModeSequenceContainer.innerHTML = '';
    state.kidSequence.forEach(command => {
        const iconContainer = document.createElement('div');
        iconContainer.className = 'sequence-icon';
        const commandButton = elements.kidModeCommands.querySelector(`[data-command=${command}]`);
        if (commandButton) {
            iconContainer.innerHTML = commandButton.querySelector('svg').outerHTML;
        }
        elements.kidModeSequenceContainer.appendChild(iconContainer);
    });
}

export async function runKidSequence() {
    let iterations = parseInt(elements.kidModeIterations.value, 10);
    if (elements.kidModeInfinite.checked) {
        iterations = -1; // Use -1 to signify infinite iterations to the backend
    } else if (isNaN(iterations) || iterations < 1) {
        iterations = 1; // Default to 1 if the value is invalid
    }

    await fetchAPI('api/sequence', {
        method: 'POST',
        body: JSON.stringify({
            commands: state.kidSequence,
            iterations: iterations
        }),
        headers: { 'Content-Type': 'application/json' }
    });
}

export async function stopKidSequence() {
    await fetchAPI('api/sequence/stop');
}

export function clearKidSequence() {
    state.kidSequence = [];
    renderKidSequence();
}
