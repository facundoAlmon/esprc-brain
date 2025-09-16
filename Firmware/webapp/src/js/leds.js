import { state } from './state.js';
import { translations } from './translations.js';

export function renderLedGroups() {
    document.getElementById('total_leds').value = state.ledConfig.total_leds || 12;
    const container = document.getElementById('led-groups-container');
    container.innerHTML = '';
    const functionOptions = translations[state.currentLanguage].ledFunctions;
    (state.ledConfig.grupos || []).forEach((group, index) => {
        const card = document.createElement('div');
        card.className = 'led-group-card';
        card.dataset.index = index;
        const selectOptions = functionOptions.map((name, i) => `<option value="${i}" ${group.funcion === i ? 'selected' : ''}>${name}</option>`).join('');
        const colorValue = `#${(group.color.r || 0).toString(16).padStart(2, '0')}${(group.color.g || 0).toString(16).padStart(2, '0')}${(group.color.b || 0).toString(16).padStart(2, '0')}`;
        card.innerHTML = `
            <div class="form-group"><label>${translations[state.currentLanguage].function}</label><select class="led-function">${selectOptions}</select></div>
            <div class="form-group"><label>${translations[state.currentLanguage].leds}</label><input type="text" class="led-indices" value="${group.leds || ''}"></div>
            <div class="form-group"><label>${translations[state.currentLanguage].color}</label><input type="color" class="led-color" value="${colorValue}"></div>
            <div class="form-group slider-group">
                <label>${translations[state.currentLanguage].brightness}: <span class="led-brightness-value">${group.brillo || 100}</span></label>
                <input type="range" class="led-brightness" min="0" max="100" value="${group.brillo || 100}">
            </div>
            <button class="remove-led-group danger">${translations[state.currentLanguage].remove}</button>`;
        container.appendChild(card);
    });
    container.querySelectorAll('.remove-led-group').forEach(btn => btn.addEventListener('click', (e) => removeLedGroup(e.target)));
    container.querySelectorAll('.led-brightness').forEach(slider => {
        slider.addEventListener('input', (e) => {
            e.target.closest('.slider-group').querySelector('.led-brightness-value').textContent = e.target.value;
        });
    });
}

export function addLedGroup() {
    if (!state.ledConfig.grupos) state.ledConfig.grupos = [];
    state.ledConfig.grupos.push({ funcion: 0, leds: "", color: { r: 255, g: 255, b: 255 }, brillo: 100 });
    renderLedGroups();
}

function removeLedGroup(button) {
    const card = button.closest('.led-group-card');
    const index = parseInt(card.dataset.index, 10);
    state.ledConfig.grupos.splice(index, 1);
    renderLedGroups();
}
