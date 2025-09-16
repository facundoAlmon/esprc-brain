import { translations } from './translations.js';
import { state, elements } from './state.js';
import { renderLedGroups } from './leds.js';


export function setLanguage(lang) {
    state.currentLanguage = lang;
    localStorage.setItem('language', lang);
    document.documentElement.lang = lang;

    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.dataset.i18n;
        if (translations[lang] && translations[lang][key]) {
            el.textContent = translations[lang][key];
        }
    });
    if (state.ledConfig.grupos && state.ledConfig.grupos.length > 0) {
        renderLedGroups();
    }
}

export function setupLanguage() {
    const savedLang = localStorage.getItem('language') || 'en';
    setLanguage(savedLang);
    elements.languageSelector.value = savedLang;
}
