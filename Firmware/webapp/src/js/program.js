import { state, elements } from './state.js';
import { fetchAPI } from './api.js';
import { translations } from './translations.js';
import { setupTooltipEvents } from './ui.js';

export async function uploadProgram() {
    updateSequenceFromUI();
    const programForBackend = state.programSequence.map(p => {
        if (p.action === 'lights') {
            return { action: p.lightAction, duration: p.duration };
        }
        return p;
    });
    await fetchAPI('api/program', {
        method: 'POST',
        body: JSON.stringify(programForBackend),
        headers: { 'Content-Type': 'application/json' }
    });
}

export async function runProgram() {
    let iterations = parseInt(elements.programIterations.value, 10);
    if (isNaN(iterations) || iterations < -1) {
        iterations = 1; // Default to 1 if invalid
    }

    let url = 'api/program/run';
    if (iterations !== 1) { // Only add parameter if not default
        url += `?iterations=${iterations}`;
    }
    await fetchAPI(url);
}
export async function stopProgram() { await fetchAPI('api/program/stop'); }
export async function clearProgram() {
    await fetchAPI('api/program/clear');
    state.programSequence = [];
    renderProgramSequence();
}
export async function loadProgramFromServer() {
    const programFromServer = await fetchAPI('api/program');
    if (!programFromServer) return;
    state.programSequence = programFromServer.map(action => {
        const frontendAction = { duration: action.duration };
        if (action.action === 'move' || action.action === 'wait') {
            frontendAction.action = action.action;
            if (action.action === 'move') {
                frontendAction.motorSpeed = action.motorSpeed;
                frontendAction.steerAngle = action.steerAngle;
            }
        } else {
            frontendAction.action = 'lights';
            frontendAction.lightAction = action.action;
        }
        return frontendAction;
    });
    renderProgramSequence();
}

export async function exportProgram() {
    try {
        const programData = await fetchAPI('api/program');
        if (programData) {
            const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(programData, null, 2));
            const downloadAnchorNode = document.createElement('a');
            downloadAnchorNode.setAttribute("href", dataStr);
            downloadAnchorNode.setAttribute("download", "esprc_program.json");
            document.body.appendChild(downloadAnchorNode);
            downloadAnchorNode.click();
            downloadAnchorNode.remove();
        } else {
            alert(translations[state.currentLanguage].programExportError);
        }
    } catch (error) {
        console.error('Error exporting program:', error);
        alert(translations[state.currentLanguage].programExportError);
    }
}

export async function importProgram(event) {
    const file = event.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = async (e) => {
        try {
            const program = JSON.parse(e.target.result);
            if (!Array.isArray(program)) {
                throw new Error("Invalid JSON format for program. Must be an array.");
            }

            await fetchAPI('api/program', {
                method: 'POST',
                body: JSON.stringify(program),
                headers: { 'Content-Type': 'application/json' }
            });

            await loadProgramFromServer();
            alert(translations[state.currentLanguage].programImportSuccess);
        } catch (error) {
            console.error('Error importing program:', error);
            alert(translations[state.currentLanguage].programImportError);
        }
        event.target.value = '';
    };
    reader.readAsText(file);
}
export function showActionModal() {
    const modalHTML = `
        <div class="modal-overlay" id="action-modal-overlay">
            <div class="modal-content">
                <h3 data-i18n="addAction">Add Action</h3>
                <div class="modal-actions">
                    <button class="modal-action-btn" data-action-type="move">Move & Steer</button>
                    <button class="modal-action-btn" data-action-type="wait">Wait</button>
                    <button class="modal-action-btn" data-action-type="lights">Lights</button>
                </div>
                <button id="close-modal-btn" class="danger">Cancel</button>
            </div>
        </div>
    `;
    document.body.insertAdjacentHTML('beforeend', modalHTML);
    document.getElementById('action-modal-overlay').addEventListener('click', (e) => {
        if (e.target.id === 'action-modal-overlay' || e.target.id === 'close-modal-btn') {
            document.getElementById('action-modal-overlay').remove();
        }
    });
    document.querySelectorAll('.modal-action-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            addAction(e.target.dataset.actionType);
            document.getElementById('action-modal-overlay').remove();
        });
    });
}
function addAction(type) {
    const newAction = { action: type, duration: 1000 };
    if (type === 'move') {
        newAction.motorSpeed = 0;
        newAction.steerAngle = 0;
    }
    if (type === 'lights') {
        newAction.lightAction = 'lights_cycle';
    }
    state.programSequence.push(newAction);
    renderProgramSequence();
}
function deleteAction(index) {
    state.programSequence.splice(index, 1);
    renderProgramSequence();
}
function duplicateAction(index) {
    updateSequenceFromUI();
    const actionToDuplicate = JSON.parse(JSON.stringify(state.programSequence[index]));
    state.programSequence.splice(index + 1, 0, actionToDuplicate);
    renderProgramSequence();
}
function updateSequenceFromUI() {
    const container = document.getElementById('program-sequence-container');
    const newSequence = [];
    container.querySelectorAll('.action-card').forEach((card) => {
        const oldIndex = parseInt(card.dataset.index, 10);
        const action = {
            action: state.programSequence[oldIndex].action,
            duration: parseInt(card.querySelector('.action-duration-number').value, 10)
        };

        if (action.action === 'move') {
            action.motorSpeed = parseInt(card.querySelector('.action-speed').value, 10);
            action.steerAngle = parseInt(card.querySelector('.action-steer').value, 10);
        }
        if (action.action === 'lights') {
            action.lightAction = card.querySelector('.action-light-select').value;
        }
        newSequence.push(action);
    });
    state.programSequence = newSequence;
}
function renderProgramSequence() {
    const container = document.getElementById('program-sequence-container');
    container.innerHTML = '';
    state.programSequence.forEach((action, index) => {
        const card = document.createElement('div');
        card.className = 'action-card';
        card.dataset.index = index;
        card.innerHTML = getActionCardHTML(action, index);
        container.appendChild(card);
    });
    attachActionCardListeners();
}
function getActionCardHTML(action, index) {
    const header = `
        <div class="action-card-header">
            <span class="drag-handle" draggable="true">↕️</span>
            <h5 data-i18n="action_${action.action}">${action.action.replace('_', ' ').toUpperCase()}</h5>
            <div>
                <button class="duplicate-action-btn secondary" data-index="${index}" data-tooltip-key="duplicateAction">❐</button>
                <button class="delete-action-btn danger" data-index="${index}" data-tooltip-key="deleteAction">X</button>
            </div>
        </div>`;

    let body = '';
    if (action.action === 'move') {
        body = `
            <div class="form-grid">
                <div class="form-group slider-group">
                    <label>Speed: <span id="action-speed-value-${index}">${action.motorSpeed}</span></label>
                    <input type="range" class="action-speed" min="-1024" max="1024" value="${action.motorSpeed}">
                </div>
                <div class="form-group slider-group">
                    <label>Steer: <span id="action-steer-value-${index}">${action.steerAngle}</span></label>
                    <input type="range" class="action-steer" min="-550" max="550" value="${action.steerAngle}">
                </div>
            </div>`;
    }
    if (action.action === 'lights') {
        body = `
            <div class="form-group">
                <label data-i18n="lightAction">Light Action</label>
                <select class="action-light-select">
                    <option value="lights_cycle" ${action.lightAction === 'lights_cycle' ? 'selected' : ''}>Cycle Headlights</option>
                    <option value="hazards_toggle" ${action.lightAction === 'hazards_toggle' ? 'selected' : ''}>Toggle Hazards</option>
                    <option value="left_turn_toggle" ${action.lightAction === 'left_turn_toggle' ? 'selected' : ''}>Toggle Left Turn</option>
                    <option value="right_turn_toggle" ${action.lightAction === 'right_turn_toggle' ? 'selected' : ''}>Toggle Right Turn</option>
                </select>
            </div>`;
    }

    const footer = `
        <div class="action-card-footer">
            <div class="form-group duration-group">
                <label>Duration (ms)</label>
                <div class="duration-controls">
                    <input type="range" class="action-duration-slider" min="100" max="10000" value="${action.duration}">
                    <input type="number" class="action-duration-number" value="${action.duration}" min="100">
                </div>
            </div>
        </div>`;

    return header + '<div class="action-card-body">' + body + footer + '</div>';
}

function attachActionCardListeners() {
    document.querySelectorAll('.delete-action-btn').forEach(btn => {
        btn.onclick = (e) => {
            e.stopPropagation();
            deleteAction(parseInt(e.currentTarget.dataset.index, 10));
        }
    });

    document.querySelectorAll('.duplicate-action-btn').forEach(btn => {
        btn.onclick = (e) => {
            e.stopPropagation();
            duplicateAction(parseInt(e.currentTarget.dataset.index, 10));
        }
    });

    document.querySelectorAll('.action-card .icon-toggle, .action-card .toggle, .action-card .danger, .action-card .secondary').forEach(button => {
        setupTooltipEvents(button);
    });

    document.querySelectorAll('.action-card input[type="range"]').forEach(slider => {
        slider.addEventListener('input', (e) => {
            if (e.target.previousElementSibling) {
                const valueEl = e.target.previousElementSibling.querySelector('span');
                if (valueEl) valueEl.textContent = e.target.value;
            }
        });

        const handle = slider.closest('.action-card').querySelector('.drag-handle');
        slider.addEventListener('touchstart', () => handle.setAttribute('draggable', false), { passive: true });
        slider.addEventListener('touchend', () => handle.setAttribute('draggable', true), { passive: true });
    });

    document.querySelectorAll('.duration-group').forEach(group => {
        const slider = group.querySelector('.action-duration-slider');
        const numberInput = group.querySelector('.action-duration-number');

        slider.addEventListener('input', () => numberInput.value = slider.value);
        numberInput.addEventListener('input', () => slider.value = numberInput.value);
    });

    const container = document.getElementById('program-sequence-container');
    document.querySelectorAll('.drag-handle').forEach(handle => {
        handle.addEventListener('dragstart', (e) => {
            const card = e.currentTarget.closest('.action-card');
            state.draggedIndex = parseInt(card.dataset.index, 10);
            setTimeout(() => card.classList.add('dragging'), 0);
        });

        handle.addEventListener('dragend', (e) => {
            const card = e.currentTarget.closest('.action-card');
            if (card) card.classList.remove('dragging');
            state.draggedIndex = null;
        });
    });

    container.addEventListener('dragover', (e) => {
        e.preventDefault();
        const afterElement = getDragAfterElement(container, e.clientY);
        const dragging = document.querySelector('.dragging');
        if (dragging) {
            if (afterElement == null) {
                container.appendChild(dragging);
            } else {
                container.insertBefore(dragging, afterElement);
            }
        }
    });

    container.addEventListener('drop', (e) => {
        e.preventDefault();
        updateSequenceFromUI();
        renderProgramSequence();
    });
}

function getDragAfterElement(container, y) {
    const draggableElements = [...container.querySelectorAll('.action-card:not(.dragging)')];
    return draggableElements.reduce((closest, child) => {
        const box = child.getBoundingClientRect();
        const offset = y - box.top - box.height / 2;
        if (offset < 0 && offset > closest.offset) {
            return { offset: offset, element: child };
        } else {
            return closest;
        }
    }, { offset: Number.NEGATIVE_INFINITY }).element;
}
