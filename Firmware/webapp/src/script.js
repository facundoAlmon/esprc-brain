/**
 * @file script.js
 * @description Lógica principal para la aplicación web de control del ESP32.
 * Este script maneja la interfaz de usuario, la comunicación por WebSockets,
 * las llamadas a la API, la internacionalización y el estado general de la aplicación.
 */

const translations = {
    en: {
        menu: "Menu",
        appTitle: "ESPRC Command Center",
        lightMode: "Light Mode",
        joystickA: "Joystick A",
        joystickB: "Joystick B",
        connection: "Connection",
        carConfig: "Car Config",
        ledConfig: "LED Config",
        espManage: "ESP32 Manage",
        camera: "Camera",
        settings: "Settings",
        turnLeft: "Turn Left",
        turnRight: "Turn Right",
        hazardLights: "Hazard Lights",
        headlights: "Headlights",
        swapLayout: "Swap Controls/Joystick",
        telemetry: "Telemetry",
        direction: "Direction",
        controls: "Controls",
        throttleOnLeft: "Throttle on Left",
        dir: "Dir",
        connectionSettings: "Connection Settings",
        ipAddress: "IP Address",
        cameraIP: "Camera IP",
        websocketURL: "WebSocket URL",
        apiURL: "API URL",
        wifiMode: "WiFi Mode",
        client: "Client",
        accessPoint: "Access Point",
        wifiSSID: "WiFi SSID",
        wifiPassword: "WiFi Password",
        refresh: "Refresh",
        save: "Save",
        reconnectWS: "Reconnect WS",
        carConfiguration: "Car Configuration",
        maxSpeed: "Max Speed",
        minSpeed: "Min Speed",
        alignment: "Alignment",
        leftTurnLimit: "Left Turn Limit",
        rightTurnLimit: "Right Turn Limit",
        enableBluetooth: "Enable Bluetooth",
        ledConfiguration: "LED Configuration",
        totalLeds: "Total LEDs in Strip",
        addGroup: "Add Group",
        export: "Export",
        import: "Import",
        saveLedConfig: "Save LED Config",
        espManagement: "ESP32 Management",
        clearNvs: "Clear Configuration (Hard Reset)",
        restartEsp: "Restart ESP32",
        startStream: "Start Stream",
        stopStream: "Stop Stream",
        cameraSettings: "Camera Settings",
        resolution: "Resolution",
        quality: "Quality",
        brightness: "Brightness",
        contrast: "Contrast",
        saturation: "Saturation",
        effect: "Effect",
        wbMode: "WB Mode",
        aeLevel: "AE Level",
        exposure: "Exposure",
        agcGain: "AGC Gain",
        gainCeiling: "Gain Ceiling",
        lensCorrection: "Lens Correction",
        hMirror: "H-Mirror",
        vFlip: "V-Flip",
        colorBar: "Color Bar",
        getConfig: "Get Config",
        setConfig: "Set Config",
        function: "Function",
        leds: "LEDs (e.g., \"1-5, 8, 10-12\")",
        color: "Color",
        remove: "Remove",
        ledFunctions: ['FRONT POSITION LIGHT', 'REAR POSITION LIGHT', 'BRAKE LIGHT', 'REVERSE LIGHT', 'LEFT TURNSIGNAL', 'RIGHT TURNSIGNAL', 'INTERIOR LIGHTING', 'UNDERGLOW'],
        wifiSavedAlert: "WiFi config saved. The ESP32 will restart.",
        importSuccessAlert: "LED configuration imported successfully!",
        importErrorAlert: "Error: Could not import the file. Please ensure it is a valid LED configuration JSON.",
        restartedAlert: "The ESP32 will restart.",
        settingsTitle: "Application Settings",
        language: "Language",
        theme: "Theme",
        networkAddresses: "Network Addresses",
        wifiConfig: "WiFi Configuration",
        throttleSettings: "Throttle Settings",
        steeringSettings: "Steering Settings",
        otherSettings: "Other Settings",
        autoLights: "Automatic Lights",
        autoTurnSignals: "Auto Turn Signals",
        autoTurnTol: "Auto Turn Signals Threshold",
        awb: "AWB",
        awbGain: "AWB Gain",
        aecSensor: "AEC Sensor",
        aecDsp: "AEC DSP",
        agc: "AGC",
        bpc: "BPC",
        wpc: "WPC",
        rawGma: "RAW GMA",
        dcw: "DCW"
    },
    es: {
        menu: "Menú",
        appTitle: "Centro de Comando ESPRC",
        lightMode: "Modo Claro",
        joystickA: "Joystick A",
        joystickB: "Joystick B",
        connection: "Conexión",
        carConfig: "Config. Auto",
        ledConfig: "Config. LED",
        espManage: "Admin. ESP32",
        camera: "Cámara",
        settings: "Ajustes",
        turnLeft: "Giro Izquierda",
        turnRight: "Giro Derecha",
        hazardLights: "Balizas",
        headlights: "Faros",
        swapLayout: "Intercambiar Controles/Joystick",
        telemetry: "Telemetría",
        direction: "Dirección",
        controls: "Controles",
        throttleOnLeft: "Acelerador a Izquierda",
        dir: "Dir",
        connectionSettings: "Ajustes de Conexión",
        ipAddress: "Dirección IP",
        cameraIP: "IP de la Cámara",
        websocketURL: "URL de WebSocket",
        apiURL: "URL de API",
        wifiMode: "Modo WiFi",
        client: "Cliente",
        accessPoint: "Punto de Acceso",
        wifiSSID: "SSID WiFi",
        wifiPassword: "Contraseña WiFi",
        refresh: "Actualizar",
        save: "Guardar",
        reconnectWS: "Reconectar WS",
        carConfiguration: "Configuración del Auto",
        maxSpeed: "Velocidad Máx.",
        minSpeed: "Velocidad Mín.",
        alignment: "Alineación",
        leftTurnLimit: "Límite Giro Izquierdo",
        rightTurnLimit: "Límite Giro Derecho",
        enableBluetooth: "Habilitar Bluetooth",
        autoTurnSignals: "Intermitentes Automáticos",
        autoTurnTol: "Umbral Intermitentes Automáticos",
        ledConfiguration: "Configuración de LEDs",
        totalLeds: "Total de LEDs en la Tira",
        addGroup: "Añadir Grupo",
        export: "Exportar",
        import: "Importar",
        saveLedConfig: "Guardar Config. LED",
        espManagement: "Administración del ESP32",
        clearNvs: "Limpiar Configuración (Hard Reset)",
        restartEsp: "Reiniciar ESP32",
        startStream: "Iniciar Stream",
        stopStream: "Detener Stream",
        cameraSettings: "Ajustes de Cámara",
        resolution: "Resolución",
        quality: "Calidad",
        brightness: "Brillo",
        contrast: "Contraste",
        saturation: "Saturación",
        effect: "Efecto",
        wbMode: "Modo WB",
        aeLevel: "Nivel AE",
        exposure: "Exposición",
        agcGain: "Ganancia AGC",
        gainCeiling: "Techo de Ganancia",
        lensCorrection: "Corrección de Lente",
        hMirror: "Espejo-H",
        vFlip: "Espejo-V",
        colorBar: "Barra de Color",
        getConfig: "Obtener Config.",
        setConfig: "Establecer Config.",
        function: "Función",
        leds: "LEDs (ej. \"1-5, 8, 10-12\")",
        color: "Color",
        remove: "Eliminar",
        ledFunctions: ['LUZ POSICIÓN FRONTAL', 'LUZ POSICIÓN TRASERA', 'LUZ DE FRENO', 'LUZ MARCHA ATRÁS', 'INTERMITENTE IZQUIERDO', 'INTERMITENTE DERECHO', 'LUZ INTERIOR', 'LUZ BAJOS'],
        wifiSavedAlert: "Configuración WiFi guardada. El ESP32 se reiniciará.",
        importSuccessAlert: "¡Configuración de LED importada con éxito!",
        importErrorAlert: "Error: No se pudo importar el archivo. Asegúrese de que sea un JSON de configuración de LED válido.",
        restartedAlert: "El ESP32 se reiniciará.",
        settingsTitle: "Ajustes de la Aplicación",
        language: "Idioma",
        theme: "Tema",
        networkAddresses: "Direcciones de Red",
        wifiConfig: "Configuración WiFi",
        throttleSettings: "Ajustes de Aceleración",
        steeringSettings: "Ajustes de Dirección",
        otherSettings: "Otros Ajustes",
        autoLights: "Luces Automáticas",
        awb: "AWB",
        awbGain: "Ganancia AWB",
        aecSensor: "Sensor AEC",
        aecDsp: "DSP AEC",
        agc: "AGC",
        bpc: "BPC",
        wpc: "WPC",
        rawGma: "RAW GMA",
        dcw: "DCW"
    }
};

document.addEventListener('DOMContentLoaded', () => {
    const app = {
        // =====================================================================
        // ESTADO DE LA APLICACIÓN
        // =====================================================================
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

        // =====================================================================
        // ICONOS SVG
        // =====================================================================
        icons: {
            turn_left: `<svg  xmlns=\"http://www.w3.org/2000/svg\"  width=\"24\"  height=\"24\"  viewBox=\"0 0 24 24\"  fill=\"none\"  stroke=\"currentColor\"  stroke-width=\"2\"  stroke-linecap=\"round\"  stroke-linejoin=\"round\"  class=\"icon icon-tabler icons-tabler-outline icon-tabler-arrow-big-left-lines\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none\"/><path stroke=\"none\" fill=\"currentColor\" d=\"M12 15v3.586a1 1 0 0 1 -1.707 .707l-6.586 -6.586a1 1 0 0 1 0 -1.414l6.586 -6.586a1 1 0 0 1 1.707 .707v3.586h3v6h-3z\" /><path d=\"M21 15v-6\" /><path d=\"M18 15v-6\" /></svg>`, 
            turn_right: `<svg  xmlns=\"http://www.w3.org/2000/svg\"  width=\"24\"  height=\"24\"  viewBox=\"0 0 24 24\"  fill=\"none\"  stroke=\"currentColor\"  stroke-width=\"2\"  stroke-linecap=\"round\"  stroke-linejoin=\"round\"  class=\"icon icon-tabler icons-tabler-outline icon-tabler-arrow-big-right-lines\"><path stroke=\"none\" fill=\"none\" d=\"M0 0h24v24H0z\" /><path stroke=\"none\" fill=\"currentColor\" d=\"M12 9v-3.586a1 1 0 0 1 1.707 -.707l6.586 6.586a1 1 0 0 1 0 1.414l-6.586 6.586a1 1 0 0 1 -1.707 -.707v-3.586h-3v-6h3z\" /><path d=\"M3 9v6\" /><path d=\"M6 9v6\" /></svg>`, 
            hazard: `<svg  xmlns=\"http://www.w3.org/2000/svg\"  width=\"24\"  height=\"24\"  viewBox=\"0 0 24 24\"  fill=\"none\"  stroke=\"currentColor\"  stroke-width=\"2\"  stroke-linecap=\"round\"  stroke-linejoin=\"round\"  class=\"icon icon-tabler icons-tabler-outline icon-tabler-alert-triangle\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none\"/><path d=\"M12 9v4\" /><path fill=\"none\"  stroke=\"currentColor\" d=\"M10.363 3.591l-8.106 13.534a1.914 1.914 0 0 0 1.636 2.871h16.214a1.914 1.914 0 0 0 1.636 -2.87l-8.106 -13.536a1.914 1.914 0 0 0 -3.274 0z\" /><path d=\"M12 16h.01\" /></svg>`, 
            headlights_off: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24">    <mask id=\"lineMdCarLightDimmedOff0\">        <g fill=\"none\" stroke=\"#fff\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\">            <path stroke-dasharray=\"12\" stroke-dashoffset=\"0\" d=\"M12 5.5l-9 2.5"/>            <path stroke-dasharray=\"12\" stroke-dashoffset=\"0\" d=\"M12 10.5l-9 2.5"/>            <path stroke-dasharray=\"12\" stroke-dashoffset=\"0\" d=\"M12 15.5l-9 2.5"/>            <path stroke=\"#000\" stroke-width=\"6\" d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>            <path stroke-dasharray=\"40\" stroke-dashoffset=\"0\" d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>            <path stroke=\"#000\" stroke-dasharray=\"28\" stroke-dashoffset=\"0\" d=\"M-1 11h26\" transform=\"rotate(45 12 12)"/>            <path stroke-dasharray=\"28\" stroke-dashoffset=\"0\" d=\"M-1 13h26\" transform=\"rotate(45 12 12)"/>        </g>    </mask>    <rect width=\"24\" height=\"24\" fill=\"currentColor\" mask=\"url(#lineMdCarLightDimmedOff0)"/></svg>`, 
            headlights_pos: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\"><path fill=\"currentColor\" d=\"M13 4.8c-4 0-4 14.4 0 14.4s9-2.7 9-7.2s-5-7.2-9-7.2m.1 12.4C12.7 16.8 12 15 12 12s.7-4.8 1.1-5.2C16 6.9 20 8.7 20 12c0 3.3-4.1 5.1-6.9 5.2M8 10.5c0 .5-.1 1-.1 1.5v.6L2.4 14l-.5-1.9L8 10.5M2 7l7.4-1.9c-.2.3-.4.7-.5 1.2c-.1.3-.2.7-.3 1.1L2.5 8.9L2 7m6.2 8.5c.1.7.3 1.4.5 1.9L2.4 19l-.5-1.9l6.3-1.6Z"/></svg>`, 
            headlights_low: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24">    <mask id=\"lineMdCarLightFilled0\">        <g fill=\"none\" stroke=\"#fff\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\">            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 6h-6"/>            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 10h-6"/>            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 14h-6"/>            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 18h-6"/>            <path fill=\"none\" stroke=\"#000\" stroke-width=\"6\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z" />            <path fill=\"none\" fill-opacity=\"1\" stroke-dasharray=\"40\" stroke-dashoffset=\"0\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>        </g>    </mask>    <rect width=\"24\" height=\"24\" fill=\"currentColor\" mask=\"url(#lineMdCarLightFilled0)"/></svg>`, 
            headlights_high: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24">    <mask id=\"lineMdCarLightFilled0\">        <g fill=\"none\"    stroke=\"currentColor\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\">            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 6h-6"/>            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 10h-6"/>            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 14h-6"/>            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 18h-6"/>            <path stroke=\"#000\" stroke-width=\"6\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z" />            <path fill=\"#fff\" fill-opacity=\"1\" stroke-dasharray=\"40\" stroke-dashoffset=\"0\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>        </g>    </mask>    <rect width=\"24\" height=\"24\" fill=\"currentColor\" mask=\"url(#lineMdCarLightFilled0)"/></svg>`, 
            bluetooth: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\" viewBox=\"0 0 20 20\"><path fill=\"currentColor\" d=\"m9.41 0l6 6l-4 4l4 4l-6 6H9v-7.59l-3.3 3.3l-1.4-1.42L8.58 10l-4.3-4.3L5.7 4.3L9 7.58V0h.41zM11 4.41V7.6L12.59 6L11 4.41zM12.59 14L11 12.41v3.18L12.59 14z"/></svg>`, 
            default: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\" viewBox=\"0 0 21 21\"><path fill=\"none\" stroke=\"currentColor\" stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"m8.5 10.5l-4 4l4 4m8-4h-12m8-12l4 4l-4 4m4-4h-12"/></svg>`, 
            acelIzq: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"icon icon-tabler icons-tabler-outline icon-tabler-switch-horizontal\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none\"/><path d=\"M16 3l4 4l-4 4" /><path d=\"M10 7l10 0" /><path d=\"M8 13l-4 4l4 4" /><path d=\"M4 17l9 0" /></svg>`,
            swap: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"icon icon-tabler icons-tabler-outline icon-tabler-switch-horizontal\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none\"/><path d=\"M16 3l4 4l-4 4" /><path d=\"M10 7l10 0" /><path d=\"M8 13l-4 4l4 4" /><path d=\"M4 17l9 0" /></svg>`
        },

        // =====================================================================
        // ELEMENTOS DE LA INTERFAZ
        // =====================================================================
        elements: {},

        // =====================================================================
        // INICIALIZACIÓN
        // =====================================================================
        init() {
            // Cachear elementos del DOM
            this.elements = {
                menuLinks: document.querySelectorAll('.menu-link'),
                tabContents: document.querySelectorAll('.tab-content'),
                urlWS: document.getElementById('urlWS'),
                urlAPI: document.getElementById('urlAPI'),
                lightModeToggle: document.getElementById('lightModeToggle'),
                languageSelector: document.getElementById('languageSelector'),
                menuToggle: document.getElementById('menu-toggle'),
                sidebar: document.getElementById('sidebar'),
                mainWrapper: document.querySelector('.main-wrapper'),
                tooltip: document.getElementById('tooltip'),

                // Botones de luces - Tab A
                turnLeftBtnA: document.getElementById('turn-left-btn-a'),
                turnRightBtnA: document.getElementById('turn-right-btn-a'),
                hazardBtnA: document.getElementById('hazard-btn-a'),
                headlightsBtnA: document.getElementById('headlights-btn-a'),
                swapLayoutBtnA: document.getElementById('swap-layout-btn-a'),

                // Botones de luces - Tab B
                turnLeftBtnB: document.getElementById('turn-left-btn-b'),
                turnRightBtnB: document.getElementById('turn-right-btn-b'),
                hazardBtnB: document.getElementById('hazard-btn-b'),
                headlightsBtnB: document.getElementById('headlights-btn-b'),
                //swapLayoutBtnB: document.getElementById('swap-layout-btn-b'),
                acelIzq: document.getElementById('acelIzq'),
            };

            this.setupLanguage();
            this.setupUI();
            this.setupEventListeners();
            this.initJoysticks();
            this.connectWebSockets();
            this.loadInitialData();
            this.startActionLoop();
            this.initLights();
        },

        setupLanguage() {
            const savedLang = localStorage.getItem('language') || 'en';
            this.setLanguage(savedLang);
            this.elements.languageSelector.value = savedLang;
        },

        setLanguage(lang) {
            this.currentLanguage = lang;
            localStorage.setItem('language', lang);
            document.documentElement.lang = lang;

            document.querySelectorAll('[data-i18n]').forEach(el => {
                const key = el.dataset.i18n;
                if (translations[lang][key]) {
                    el.textContent = translations[lang][key];
                }
            });
            if (this.ledConfig.grupos && this.ledConfig.grupos.length > 0) {
                this.renderLedGroups();
            }
        },

        setupUI() {
            this.elements.urlWS.value = `ws://${this.hostname}/ws`;
            this.elements.urlAPI.value = `${window.location.protocol}//${this.hostname}/`;
            if (localStorage.getItem('theme') === 'light') {
                document.body.classList.add('light-mode');
                this.elements.lightModeToggle.checked = true;
            }
            if (window.innerWidth <= 992) {
                this.elements.sidebar.classList.add('sidebar-hidden');
                this.elements.mainWrapper.classList.add('full-width');
            }
            // Restaurar el estado del layout
            this.joystickLayoutSwapped = localStorage.getItem('joystickLayoutSwapped') === 'true';
            document.getElementById('joystick-a').querySelector('.joystick-layout-single').classList.toggle('swapped', this.joystickLayoutSwapped);
            document.getElementById('joystick-a').querySelector('.joystick-layout-single').classList.toggle('active', this.joystickLayoutSwapped);
        },

        setupEventListeners() {
            this.elements.languageSelector.addEventListener('change', (e) => this.setLanguage(e.target.value));
            this.elements.menuToggle.addEventListener('click', () => {
                this.elements.sidebar.classList.toggle('sidebar-hidden');
                this.elements.mainWrapper.classList.toggle('full-width');
            });
            this.elements.menuLinks.forEach(link => {
                link.addEventListener('click', () => this.openTab(link.dataset.tab));
            });
            document.querySelectorAll('.slider-group input[type="range"]').forEach(slider => {
                slider.addEventListener('input', (e) => {
                    const valueEl = document.getElementById(`${e.target.id}Value`);
                    if (valueEl) valueEl.textContent = e.target.value;
                });
            });
            this.elements.lightModeToggle.addEventListener('change', () => {
                document.body.classList.toggle('light-mode');
                localStorage.setItem('theme', document.body.classList.contains('light-mode') ? 'light' : 'dark');
                this.initJoysticks();
            });
            document.getElementById('add-led-group').addEventListener('click', () => this.addLedGroup());
            document.getElementById('import-led-input').addEventListener('change', (e) => this.importLedConfig(e));
            this.attachClick('getWifiConfigBtn', this.getWifiConfig);
            this.attachClick('saveWifiConfigBtn', this.saveWifiConfig);
            this.attachClick('wsReconnectBtn', this.wsReconnect);
            this.attachClick('getConfigBtn', this.getConfig);
            this.attachClick('saveConfigBtn', this.saveConfig);
            this.attachClick('exportLedConfigBtn', this.exportLedConfig);
            this.attachClick('saveLedConfigBtn', this.saveLedConfig);
            this.attachClick('manageESPBtn', this.manageESP);
            this.attachClick('startCamStreamBtn', this.startCamStream);
            this.attachClick('stopCamStreamBtn', this.stopCamStream);
            this.attachClick('getCamConfigBtn', this.getCamConfig);
            this.attachClick('setCamConfigBtn', this.setCamConfig);

            // Event listener para el botón de swap
            const swapHandler = () => {
                this.joystickLayoutSwapped = !this.joystickLayoutSwapped;
                localStorage.setItem('joystickLayoutSwapped', this.joystickLayoutSwapped);
                document.getElementById('joystick-a').querySelector('.joystick-layout-single').classList.toggle('swapped', this.joystickLayoutSwapped);
            };
            this.elements.swapLayoutBtnA.addEventListener('click', swapHandler);
            //this.elements.swapLayoutBtnB.addEventListener('click', swapHandler); // Ambos botones hacen lo mismo por ahora

            document.querySelectorAll('.icon-toggle, .toggle').forEach(button => {
                this.setupTooltipEvents(button);
            });
        },

        attachClick(id, handler) {
            const element = document.getElementById(id);
            if (element) {
                element.addEventListener('click', handler.bind(this));
            }
        },

        initJoysticks() {
            const styles = getComputedStyle(document.documentElement);
            const joystickOptions = {
                "title": "Car Control",
                "autoReturnToCenter": true,
                width: 300,
                height: 300,
                internalFillColor: styles.getPropertyValue('--joystick-handle-color').trim(),
                internalStrokeColor: styles.getPropertyValue('--joystick-handle-stroke-color').trim(),
                externalStrokeColor: styles.getPropertyValue('--joystick-border-color').trim(),
            };
            document.getElementById('joy1Div').innerHTML = '';
            document.getElementById('joy2ADiv').innerHTML = '';
            document.getElementById('joy2BDiv').innerHTML = '';
            this.joy1 = new JoyStick('joy1Div', joystickOptions, () => {});
            this.joy2A = new JoyStick('joy2ADiv', joystickOptions, () => {});
            this.joy2B = new JoyStick('joy2BDiv', joystickOptions, () => {});
        },

        connectWebSockets() {
            try {
                this.wsSocket = new WebSocket(this.elements.urlWS.value);
                this.wsSocket.onopen = () => this.socketOnline = true;
                this.wsSocket.onclose = () => this.socketOnline = false;
                this.wsSocket.onerror = (err) => console.error('Main WS Error:', err);
            } catch (err) {
                console.error('Failed to create main WebSocket:', err);
            }
        },

        wsReconnect() {
            this.connectWebSockets();
        },

        connectCamWebSocket(camIP) {
            if (!camIP || this.socketCamOnline) return;
            const camWsUrl = `ws://${camIP}/ws`;
            try {
                this.wsSocketCam = new WebSocket(camWsUrl);
                this.wsSocketCam.onmessage = (event) => {
                    document.getElementById('camImg').src = URL.createObjectURL(event.data);
                };
                this.wsSocketCam.onopen = () => this.socketCamOnline = true;
                this.wsSocketCam.onclose = () => this.socketCamOnline = false;
                this.wsSocketCam.onerror = (err) => console.error('Cam WS Error:', err);
            } catch (err) {
                console.error('Failed to create cam WebSocket:', err);
            }
        },

        async fetchAPI(endpoint, options = {}, baseUrl = null) {
            const url = (baseUrl || this.elements.urlAPI.value) + endpoint;
            try {
                const response = await fetch(url, options);
                if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                if (response.status === 204 || (options.method === 'POST' && response.headers.get("Content-Length") === "2")) return null;
                return await response.json();
            } catch (error) {
                console.error(`API call to ${endpoint} failed:`, error);
                return null;
            }
        },

        loadInitialData() {
            this.getWifiConfig();
        },

        openTab(tabId) {
            this.activeTab = tabId;
            this.elements.tabContents.forEach(content => content.classList.remove('active'));
            this.elements.menuLinks.forEach(link => link.classList.remove('active'));
            document.getElementById(tabId).classList.add('active');
            document.querySelector(`.menu-link[data-tab="${tabId}"]`).classList.add('active');
            if (window.innerWidth <= 992) {
                this.elements.sidebar.classList.add('sidebar-hidden');
                this.elements.mainWrapper.classList.add('full-width');
            }

            // Actualiza la UI de las luces para asegurar consistencia al cambiar de pestaña
            this.updateLightsUI();

            switch (tabId) {
                case 'config': this.getConfig(); break;
                case 'led-config': this.getLedConfig(); break;
                case 'conexion': this.getWifiConfig(); break;
                case 'cam': this.getCamConfig(); break;
            }
        },

        startActionLoop() {
            setInterval(() => {
                if (!this.socketOnline) return;
                let actBody = {};
                if (this.activeTab === 'joystick-a') {
                    actBody = {
                        motorSpeed: Math.trunc(Math.abs(this.joy1.GetY()) * 1023 / 100),
                        motorDirection: this.joy1.GetY() < 0 ? "F" : "B",
                        steerDirection: this.joy1.GetX() < 0 ? "L" : "R",
                        steerAng: Math.trunc(Math.abs(this.joy1.GetX()) * 512 / 100),
                        ms: 500
                    };
                } else if (this.activeTab === 'joystick-b') {
                    const acelIzq = document.getElementById("acelIzq").classList.contains('active');
                    const joyX = acelIzq ? this.joy2B.GetX() : this.joy2A.GetX();
                    const joyY = acelIzq ? this.joy2A.GetY() : this.joy2B.GetY();
                    actBody = {
                        motorSpeed: Math.trunc(Math.abs(joyY) * 1023 / 100),
                        motorDirection: joyY < 0 ? "F" : "B",
                        steerDirection: joyX < 0 ? "L" : "R",
                        steerAng: Math.trunc(Math.abs(joyX) * 512 / 100),
                        ms: 500
                    };
                } else { return; }
                this.wsSocket.send(JSON.stringify(actBody));
            }, 100);
        },

        sendWsAction(action) {
            if (!this.socketOnline) return;
            const payload = { action: action };
            this.wsSocket.send(JSON.stringify(payload));
        },

        // =====================================================================
        // LÓGICA DE LUCES
        // =====================================================================
        initLights() {
            // Asignar eventos a ambos sets de botones de luces
            ['A', 'B'].forEach(tab => {
                this.elements[`turnLeftBtn${tab}`].addEventListener('click', () => this.handleTurnSignal('left'));
                this.elements[`turnRightBtn${tab}`].addEventListener('click', () => this.handleTurnSignal('right'));
                this.elements[`hazardBtn${tab}`].addEventListener('click', () => this.handleHazard());
                this.elements[`headlightsBtn${tab}`].addEventListener('click', () => this.handleHeadlights());
            });

            // El botón acelIzq es único para el joystick B y se maneja de forma independiente.
            this.elements.acelIzq.addEventListener('click', () => {
                this.lightsState.acelIzq = !this.lightsState.acelIzq;
                this.elements.acelIzq.classList.toggle('active', this.lightsState.acelIzq);
            });
            this.elements.acelIzq.innerHTML = this.icons.acelIzq; // Asignar SVG una sola vez.

            this.updateLightsUI(); // Llamada inicial para establecer el estado correcto de los SVG de las luces.
        },

        handleTurnSignal(direction) {
            this.lightsState.turnLeft = (direction === 'left') ? !this.lightsState.turnLeft : false;
            this.lightsState.turnRight = (direction === 'right') ? !this.lightsState.turnRight : false;
            
            if (this.lightsState.hazard) {
                this.lightsState.hazard = false;
                this.sendWsAction('hazards_toggle'); 
            }
            
            this.sendWsAction(direction === 'left' ? 'left_turn_toggle' : 'right_turn_toggle');
            this.updateLightsUI();
        },

        handleHazard() {
            this.lightsState.hazard = !this.lightsState.hazard;
            this.lightsState.turnLeft = false;
            this.lightsState.turnRight = false;
            this.sendWsAction('hazards_toggle');
            this.updateLightsUI();
        },

        handleHeadlights() {
            this.lightsState.headlights = (this.lightsState.headlights + 1) % 4;
            this.sendWsAction('headlights_cycle');
            this.updateLightsUI();
        },

        updateLightsUI() {
            const { turnLeft, turnRight, hazard, acelIzq } = this.lightsState;

            // --- Actualizar el estado de la clase 'active' para todos los botones ---
            this.elements.turnLeftBtnA.classList.toggle('active', turnLeft && !hazard);
            this.elements.turnRightBtnA.classList.toggle('active', turnRight && !hazard);
            this.elements.hazardBtnA.classList.toggle('active', hazard);

            this.elements.turnLeftBtnB.classList.toggle('active', turnLeft && !hazard);
            this.elements.turnRightBtnB.classList.toggle('active', turnRight && !hazard);
            this.elements.hazardBtnB.classList.toggle('active', hazard);
            this.elements.acelIzq.classList.toggle('active', acelIzq);

            // --- Actualizar dinámicamente los SVG solo para los botones de luces en la pestaña activa ---
            if (this.activeTab === 'joystick-a') {
                // Poner SVGs en la Pestaña A
                this.elements.turnLeftBtnA.innerHTML = this.icons.turn_left;
                this.elements.turnRightBtnA.innerHTML = this.icons.turn_right;
                this.elements.hazardBtnA.innerHTML = this.icons.hazard;
                this.updateButtonIcon(this.elements.headlightsBtnA, this.lightsState.headlights > 0, 'headlights');

                // Quitar SVGs de la Pestaña B
                this.elements.turnLeftBtnB.innerHTML = '';
                this.elements.turnRightBtnB.innerHTML = '';
                this.elements.hazardBtnB.innerHTML = '';
                this.elements.headlightsBtnB.innerHTML = '';
            } else if (this.activeTab === 'joystick-b') {
                // Quitar SVGs de la Pestaña A
                this.elements.turnLeftBtnA.innerHTML = '';
                this.elements.turnRightBtnA.innerHTML = '';
                this.elements.hazardBtnA.innerHTML = '';
                this.elements.headlightsBtnA.innerHTML = '';

                // Poner SVGs en la Pestaña B
                this.elements.turnLeftBtnB.innerHTML = this.icons.turn_left;
                this.elements.turnRightBtnB.innerHTML = this.icons.turn_right;
                this.elements.hazardBtnB.innerHTML = this.icons.hazard;
                this.updateButtonIcon(this.elements.headlightsBtnB, this.lightsState.headlights > 0, 'headlights');
            } else {
                // Si ninguna pestaña de joystick está activa, quitar todos los SVGs de luces
                this.elements.turnLeftBtnA.innerHTML = '';
                this.elements.turnRightBtnA.innerHTML = '';
                this.elements.hazardBtnA.innerHTML = '';
                this.elements.headlightsBtnA.innerHTML = '';
                
                this.elements.turnLeftBtnB.innerHTML = '';
                this.elements.turnRightBtnB.innerHTML = '';
                this.elements.hazardBtnB.innerHTML = '';
                this.elements.headlightsBtnB.innerHTML = '';
            }
            
            // Asignar siempre el icono de swap, ya que es estático
            this.elements.swapLayoutBtnA.innerHTML = this.icons.swap;
            //this.elements.swapLayoutBtnB.innerHTML = this.icons.swap;
        },

        updateButtonIcon(button, isActive, forceType = null) {
            let iconType = forceType;
            if (!iconType) {
                if (button.id.includes('Scan')) iconType = 'bluetooth';
                else iconType = 'default';
            }
            
            if (button.id.startsWith('headlights-btn')) {
                const stateMap = ['off', 'pos', 'low', 'high'];
                const iconName = `headlights_${stateMap[this.lightsState.headlights]}`;
                button.innerHTML = this.icons[iconName];
            } else {
//                const iconNameOn = `${iconType}_on`;
                //const iconNameOff = `${iconType}_off`;
                //button.innerHTML = this.icons[iconName];
//                button.innerHTML = isActive 
//                    ? (this.icons[iconNameOn] || this.icons[iconType]) 
//                    : (this.icons[iconNameOff] || this.icons[iconType]);
            }
        },

        // =====================================================================
        // HELPERS Y OTROS
        // =====================================================================
        updateForm(jsonData) {
            Object.keys(jsonData).forEach(key => {
                const el = document.getElementById(key);
                if (el) {
                    if (el.type === 'checkbox') {
                        el.checked = jsonData[key] == 1;
                    } else if (el.classList.contains('icon-toggle')) {
                        const isActive = jsonData[key] == 1;
                        el.classList.toggle('active', isActive);
                        this.updateButtonIcon(el, isActive);
                    } else {
                        el.value = jsonData[key];
                    }
                    const valueEl = document.getElementById(`${key}Value`);
                    if (valueEl) valueEl.textContent = jsonData[key];
                }
            });
            if (jsonData.wifiMode == "AP") document.getElementById('apMode').checked = true
            if (jsonData.wifiMode == "CLI") document.getElementById('cliMode').checked = true
        },

        serializeForm(formSelector) {
            const form = document.querySelector(formSelector);
            const configBody = {};
            form.querySelectorAll('input, select').forEach(el => {
                if (el.id) {
                    if (el.type === 'checkbox') {
                        configBody[el.id] = el.checked ? 1 : 0;
                    } else if (el.classList.contains('icon-toggle')) {
                        configBody[el.id] = el.classList.contains('active') ? 1 : 0;
                    } else if (el.type === 'range' || el.type === 'number') {
                        configBody[el.id] = parseInt(el.value, 10);
                    } else if (el.tagName === 'SELECT' || el.type !== 'radio' || el.checked) {
                        configBody[el.id] = el.value;
                    }
                }
            });
            return configBody;
        },

        setupTooltipEvents(element) {
            element.addEventListener('mouseenter', e => this.showTooltip(e));
            element.addEventListener('mouseleave', e => this.hideTooltip(e));
            element.addEventListener('touchstart', e => {
                this.tooltipTimeout = setTimeout(() => this.showTooltip(e), 500);
            }, { passive: true });
            element.addEventListener('touchend', e => {
                clearTimeout(this.tooltipTimeout);
                this.hideTooltip(e);
            });
        },

        showTooltip(e) {
            const target = e.currentTarget;
            const tooltipKey = target.dataset.tooltipKey;
            if (!tooltipKey) return;
            const tooltipText = translations[this.currentLanguage][tooltipKey] || tooltipKey;
            this.elements.tooltip.textContent = tooltipText;
            this.elements.tooltip.classList.add('visible');
            const targetRect = target.getBoundingClientRect();
            const tooltipRect = this.elements.tooltip.getBoundingClientRect();
            let top = targetRect.top - tooltipRect.height - 8;
            let left = targetRect.left + (targetRect.width / 2) - (tooltipRect.width / 2);
            if (top < 0) top = targetRect.bottom + 8;
            if (left < 0) left = 5;
            if (left + tooltipRect.width > window.innerWidth) left = window.innerWidth - tooltipRect.width - 5;
            this.elements.tooltip.style.top = `${top}px`;
            this.elements.tooltip.style.left = `${left}px`;
        },

        hideTooltip() {
            this.elements.tooltip.classList.remove('visible');
        },

        async getWifiConfig() {
            const json = await this.fetchAPI('wifi');
            if (!json) return;
            this.wifiConfig = json;
            this.updateForm(json);
            //this.connectCamWebSocket(json.camIP);
        },

        async saveWifiConfig() {
            const configBody = {
                wifiName: document.getElementById('wifiName').value,
                wifiPass: document.getElementById('wifiPass').value,
                wifiMode: document.getElementById('cliMode').checked ? "CLI" : "AP"
            };
            await this.fetchAPI('wifi', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
            alert(translations[this.currentLanguage].wifiSavedAlert);
        },
        
        async getConfig() {
            const json = await this.fetchAPI('config');
            if (!json) return;
            this.config = json;
            this.updateForm(json);
        },

        async saveConfig() {
            const configBody = this.serializeForm('#config');
            await this.fetchAPI('config', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
            this.getConfig();
        },

        async getLedConfig() {
            const json = await this.fetchAPI('api/leds');
            if (!json) return;
            this.ledConfig = json;
            this.renderLedGroups();
        },

        async saveLedConfig() {
            const container = document.getElementById('led-groups-container');
            const groups = [];
            container.querySelectorAll('.led-group-card').forEach(card => {
                const group = {
                    funcion: parseInt(card.querySelector('.led-function').value, 10),
                    leds: card.querySelector('.led-indices').value,
                    color: {
                        r: parseInt(card.querySelector('.led-color').value.substr(1, 2), 16),
                        g: parseInt(card.querySelector('.led-color').value.substr(3, 2), 16),
                        b: parseInt(card.querySelector('.led-color').value.substr(5, 2), 16)
                    },
                    brillo: parseInt(card.querySelector('.led-brightness').value, 10)
                };
                groups.push(group);
            });
            const configBody = {
                total_leds: parseInt(document.getElementById('total_leds').value, 10),
                grupos: groups
            };
            await this.fetchAPI('api/leds', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
            this.getLedConfig();
        },

        exportLedConfig() {
            const container = document.getElementById('led-groups-container');
            const groups = [];
            container.querySelectorAll('.led-group-card').forEach(card => {
                const group = {
                    funcion: parseInt(card.querySelector('.led-function').value, 10),
                    leds: card.querySelector('.led-indices').value,
                    color: {
                        r: parseInt(card.querySelector('.led-color').value.substr(1, 2), 16),
                        g: parseInt(card.querySelector('.led-color').value.substr(3, 2), 16),
                        b: parseInt(card.querySelector('.led-color').value.substr(5, 2), 16)
                    },
                    brillo: parseInt(card.querySelector('.led-brightness').value, 10)
                };
                groups.push(group);
            });
            const configToSave = {
                total_leds: parseInt(document.getElementById('total_leds').value, 10),
                grupos: groups
            };
            const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(configToSave, null, 2));
            const downloadAnchorNode = document.createElement('a');
            downloadAnchorNode.setAttribute("href", dataStr);
            downloadAnchorNode.setAttribute("download", "led_config.json");
            document.body.appendChild(downloadAnchorNode);
            downloadAnchorNode.click();
            downloadAnchorNode.remove();
        },

        importLedConfig(event) {
            const file = event.target.files[0];
            if (!file) return;
            const reader = new FileReader();
            reader.onload = (e) => {
                try {
                    const importedConfig = JSON.parse(e.target.result);
                    if (typeof importedConfig.total_leds !== 'number' || !Array.isArray(importedConfig.grupos)) {
                        throw new Error("Invalid JSON format for LED config.");
                    }
                    this.ledConfig = importedConfig;
                    this.renderLedGroups();
                    alert(translations[this.currentLanguage].importSuccessAlert);
                } catch (error) {
                    console.error("Failed to import LED config:", error);
                    alert(translations[this.currentLanguage].importErrorAlert);
                }
                event.target.value = '';
            };
            reader.readAsText(file);
        },

        renderLedGroups() {
            document.getElementById('total_leds').value = this.ledConfig.total_leds || 12;
            const container = document.getElementById('led-groups-container');
            container.innerHTML = '';
            const functionOptions = translations[this.currentLanguage].ledFunctions;
            (this.ledConfig.grupos || []).forEach((group, index) => {
                const card = document.createElement('div');
                card.className = 'led-group-card';
                card.dataset.index = index;
                const selectOptions = functionOptions.map((name, i) => `<option value="${i}" ${group.funcion === i ? 'selected' : ''}>${name}</option>`).join('');
                const colorValue = `#${(group.color.r || 0).toString(16).padStart(2, '0')}${(group.color.g || 0).toString(16).padStart(2, '0')}${(group.color.b || 0).toString(16).padStart(2, '0')}`;
                card.innerHTML = `
                    <div class="form-group"><label>${translations[this.currentLanguage].function}</label><select class="led-function">${selectOptions}</select></div>
                    <div class="form-group"><label>${translations[this.currentLanguage].leds}</label><input type="text" class="led-indices" value="${group.leds || ''}"></div>
                    <div class="form-group"><label>${translations[this.currentLanguage].color}</label><input type="color" class="led-color" value="${colorValue}"></div>
                    <div class.form-group slider-group">
                        <label>${translations[this.currentLanguage].brightness}: <span class="led-brightness-value">${group.brillo || 100}</span></label>
                        <input type="range" class="led-brightness" min="0" max="100" value="${group.brillo || 100}">
                    </div>
                    <button class="remove-led-group danger">${translations[this.currentLanguage].remove}</button>`;
                container.appendChild(card);
            });
            container.querySelectorAll('.remove-led-group').forEach(btn => btn.addEventListener('click', (e) => this.removeLedGroup(e.target)));
            container.querySelectorAll('.led-brightness').forEach(slider => {
                slider.addEventListener('input', (e) => {
                    e.target.closest('.slider-group').querySelector('.led-brightness-value').textContent = e.target.value;
                });
            });
        },

        addLedGroup() {
            if (!this.ledConfig.grupos) this.ledConfig.grupos = [];
            this.ledConfig.grupos.push({ funcion: 0, leds: "", color: { r: 255, g: 255, b: 255 }, brillo: 100 });
            this.renderLedGroups();
        },

        removeLedGroup(button) {
            const card = button.closest('.led-group-card');
            const index = parseInt(card.dataset.index, 10);
            this.ledConfig.grupos.splice(index, 1);
            this.renderLedGroups();
        },

        async manageESP() {
            const configBody = {
                clearPreferences: document.getElementById("clearPreferences").checked,
                restartESP: true
            };
            await this.fetchAPI('manage', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } });
            alert(translations[this.currentLanguage].restartedAlert);
        },
        
        async getCamConfig() {
            const camApiUrl = `${window.location.protocol}//${this.wifiConfig.camIP}/`;
            const json = await this.fetchAPI('cam-config', {}, camApiUrl);
            if (json) this.updateForm(json);
        },

        async setCamConfig() {
            const camApiUrl = `${window.location.protocol}//${this.wifiConfig.camIP}/`;
            const configBody = this.serializeForm('#cam .cam-controls');
            await this.fetchAPI('cam-config', { method: 'POST', body: JSON.stringify(configBody), headers: { 'Content-Type': 'application/json' } }, camApiUrl);
        },

        async startCamStream() {
            const camApiUrl = `${window.location.protocol}//${this.wifiConfig.camIP}/`;
            await this.fetchAPI('enable-stream', {}, camApiUrl);
        },

        async stopCamStream() {
            const camApiUrl = `${window.location.protocol}//${this.wifiConfig.camIP}/`;
            await this.fetchAPI('disable-stream', {}, camApiUrl);
        }
    };

    window.app = app;
    app.init();
});

