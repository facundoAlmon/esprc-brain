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
        dcw: "DCW",
        program: "Program",
        programTitle: "Program Sequence",
        uploadProgram: "Upload to ESP",
        runProgram: "▶️ Run",
        stopProgram: "⏹️ Stop",
        clearProgram: "🗑️ Clear All",
        addAction: "Add Action",
        loadProgram: "Load from ESP",
        duplicateAction: "Duplicate Action",
        deleteAction: "Delete Action",
        iterations: "Iterations",
        infinite: "Infinite",
        exportConfig: "Export Config",
        importConfig: "Import Config",
        programImportSuccess: "Program imported successfully!",
        programImportError: "An error occurred during program import. Invalid JSON or file format.",
        programExportError: "Failed to export program.",
        kidMode: "Kid Mode",
        kidModeTitle: "Kid Mode",
        commandButtons: "Command Buttons",
        sequence: "Sequence",
        forward: "Forward",
        forwardLeft: "Forward Left",
        forwardRight: "Forward Right",
        backward: "Backward",
        backwardLeft: "Backward Left",
        backwardRight: "Backward Right",
        left: "Left",
        right: "Right",
        horn: "Horn",
        wait: "Wait",
        runSequence: "▶️ Run Sequence",
        clearSequence: "🗑️ Clear Sequence"
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
        dcw: "DCW",
        program: "Programa",
        programTitle: "Secuencia de Programa",
        uploadProgram: "Cargar a ESP",
        runProgram: "▶️ Ejecutar",
        stopProgram: "⏹️ Detener",
        clearProgram: "🗑️ Limpiar Todo",
        addAction: "Añadir Acción",
        loadProgram: "Cargar desde ESP",
        duplicateAction: "Duplicar Acción",
        deleteAction: "Eliminar Acción",
        iterations: "Iteraciones",
        infinite: "Infinito",
        exportConfig: "Exportar Config.",
        importConfig: "Importar Config.",
        programImportSuccess: "¡Programa importado con éxito!",
        programImportError: "Ocurrió un error durante la importación del programa. Formato JSON o de archivo no válido.",
        programExportError: "Fallo al exportar el programa.",
        kidMode: "Modo Niños",
        kidModeTitle: "Modo Niños",
        commandButtons: "Botones de Comando",
        sequence: "Secuencia",
        forward: "Adelante",
        forwardLeft: "Adelante Izquierda",
        forwardRight: "Adelante Derecha",
        backward: "Atrás",
        backwardLeft: "Atrás Izquierda",
        backwardRight: "Atrás Derecha",
        left: "Izquierda",
        right: "Derecha",
        horn: "Bocina",
        wait: "Esperar",
        runSequence: "▶️ Ejecutar Secuencia",
        clearSequence: "🗑️ Limpiar Secuencia"
    }
};

document.addEventListener('DOMContentLoaded', () => {
    const app = {
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

        icons: {
            turn_left: `<svg  xmlns=\"http://www.w3.org/2000/svg\"  width=\"24\"  height=\"24\"  viewBox=\"0 0 24 24\"  fill=\"none\"  stroke=\"currentColor\"  stroke-width=\"2\"  stroke-linecap=\"round\"  stroke-linejoin=\"round\"  class=\"icon icon-tabler icons-tabler-outline icon-tabler-arrow-big-left-lines\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none"/><path stroke=\"none\" fill=\"currentColor\" d=\"M12 15v3.586a1 1 0 0 1 -1.707 .707l-6.586 -6.586a1 1 0 0 1 0 -1.414l6.586 -6.586a1 1 0 0 1 1.707 .707v3.586h3v6h-3z" /><path d=\"M21 15v-6" /><path d=\"M18 15v-6" /></svg>`, 
            turn_right: `<svg  xmlns=\"http://www.w3.org/2000/svg\"  width=\"24\"  height=\"24\"  viewBox=\"0 0 24 24\"  fill=\"none\"  stroke=\"currentColor\"  stroke-width=\"2\"  stroke-linecap=\"round\"  stroke-linejoin=\"round\"  class=\"icon icon-tabler icons-tabler-outline icon-tabler-arrow-big-right-lines\"><path stroke=\"none\" fill=\"none\" d=\"M0 0h24v24H0z" /><path stroke=\"none\" fill=\"currentColor\" d=\"M12 9v-3.586a1 1 0 0 1 1.707 -.707l6.586 6.586a1 1 0 0 1 0 1.414l-6.586 6.586a1 1 0 0 1 -1.707 -.707v-3.586h-3v-6h3z" /><path d=\"M3 9v6" /><path d=\"M6 9v6" /></svg>`, 
            hazard: `<svg  xmlns=\"http://www.w3.org/2000/svg\"  width=\"24\"  height=\"24\"  viewBox=\"0 0 24 24\"  fill=\"none\"  stroke=\"currentColor\"  stroke-width=\"2\"  stroke-linecap=\"round\"  stroke-linejoin=\"round\"  class=\"icon icon-tabler icons-tabler-outline icon-tabler-alert-triangle\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none"/><path d=\"M12 9v4" /><path fill=\"none"  stroke=\"currentColor\" d=\"M10.363 3.591l-8.106 13.534a1.914 1.914 0 0 0 1.636 2.871h16.214a1.914 1.914 0 0 0 1.636 -2.87l-8.106 -13.536a1.914 1.914 0 0 0 -3.274 0z" /><path d=\"M12 16h.01" /></svg>`, 
            headlights_off: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24">    <mask id=\"lineMdCarLightDimmedOff0\">        <g fill=\"none\" stroke=\"#fff\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\">            <path stroke-dasharray=\"12\" stroke-dashoffset=\"0\" d=\"M12 5.5l-9 2.5"/>            <path stroke-dasharray=\"12\" stroke-dashoffset=\"0\" d=\"M12 10.5l-9 2.5"/>            <path stroke-dasharray=\"12\" stroke-dashoffset=\"0\" d=\"M12 15.5l-9 2.5"/>            <path stroke=\"#000\" stroke-width=\"6\" d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>            <path stroke-dasharray=\"40\" stroke-dashoffset=\"0\" d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>            <path stroke=\"#000\" stroke-dasharray=\"28\" stroke-dashoffset=\"0\" d=\"M-1 11h26\" transform=\"rotate(45 12 12)"/>            <path stroke-dasharray=\"28\" stroke-dashoffset=\"0\" d=\"M-1 13h26\" transform=\"rotate(45 12 12)"/>        </g>    </mask>    <rect width=\"24\" height=\"24\" fill=\"currentColor\" mask=\"url(#lineMdCarLightDimmedOff0)"/></svg>`, 
            headlights_pos: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\"><path fill=\"currentColor\" d=\"M13 4.8c-4 0-4 14.4 0 14.4s9-2.7 9-7.2s-5-7.2-9-7.2m.1 12.4C12.7 16.8 12 15 12 12s.7-4.8 1.1-5.2C16 6.9 20 8.7 20 12c0 3.3-4.1 5.1-6.9 5.2M8 10.5c0 .5-.1 1-.1 1.5v.6L2.4 14l-.5-1.9L8 10.5M2 7l7.4-1.9c-.2.3-.4.7-.5 1.2c-.1.3-.2.7-.3 1.1L2.5 8.9L2 7m6.2 8.5c.1.7.3 1.4.5 1.9L2.4 19l-.5-1.9l6.3-1.6Z"/></svg>`, 
            headlights_low: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24">    <mask id=\"lineMdCarLightFilled0\">        <g fill=\"none\" stroke=\"#fff\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\">            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 6h-6"/>            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 10h-6"/>            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 14h-6"/>            <path stroke-dasharray=\"8\" fill=\"none\" stroke-dashoffset=\"0\" d=\"M11 18h-6"/>            <path fill=\"none\" stroke=\"#000\" stroke-width=\"6\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z" />            <path fill=\"none\" fill-opacity=\"1\" stroke-dasharray=\"40\" stroke-dashoffset=\"0\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>        </g>    </mask>    <rect width=\"24\" height=\"24\" fill=\"currentColor\" mask=\"url(#lineMdCarLightFilled0)"/></svg>`, 
            headlights_high: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24">    <mask id=\"lineMdCarLightFilled0\">        <g fill=\"none"    stroke=\"currentColor\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\">            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 6h-6"/>            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 10h-6"/>            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 14h-6"/>            <path stroke-dasharray=\"8\" stroke-dashoffset=\"0\" d=\"M11 18h-6"/>            <path stroke=\"#000\" stroke-width=\"6\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z" />            <path fill=\"#fff\" fill-opacity=\"1\" stroke-dasharray=\"40\" stroke-dashoffset=\"0\"                d=\"M21 12c0 -3.31 -3.5 -6.25 -8.25 -6.25c-0.5 0 -1.75 2.75 -1.75 6.25c0 3.5 1.25 6.25 1.75 6.25c4.75 0 8.25 -2.94 8.25 -6.25Z"/>        </g>    </mask>    <rect width=\"24\" height=\"24\" fill=\"currentColor\" mask=\"url(#lineMdCarLightFilled0)"/></svg>`, 
            bluetooth: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\" viewBox=\"0 0 20 20\"><path fill=\"currentColor\" d=\"m9.41 0l6 6l-4 4l4 4l-6 6H9v-7.59l-3.3 3.3l-1.4-1.42L8.58 10l-4.3-4.3L5.7 4.3L9 7.58V0h.41zM11 4.41V7.6L12.59 6L11 4.41zM12.59 14L11 12.41v3.18L12.59 14z"/></svg>`, 
            default: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\" viewBox=\"0 0 21 21\"><path fill=\"none\" stroke=\"currentColor\" stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"m8.5 10.5l-4 4l4 4m8-4h-12m8-12l4 4l-4 4m4-4h-12"/></svg>`, 
            acelIzq: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"icon icon-tabler icons-tabler-outline icon-tabler-switch-horizontal\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none"/><path d=\"M16 3l4 4l-4 4" /><path d=\"M10 7l10 0" /><path d=\"M8 13l-4 4l4 4" /><path d=\"M4 17l9 0" /></svg>`,
            swap: `<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"icon icon-tabler icons-tabler-outline icon-tabler-switch-horizontal\"><path stroke=\"none\" d=\"M0 0h24v24H0z\" fill=\"none"/><path d=\"M16 3l4 4l-4 4" /><path d=\"M10 7l10 0" /><path d=\"M8 13l-4 4l4 4" /><path d=\"M4 17l9 0" /></svg>`
        },

        elements: {},

        init() {
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
                turnLeftBtnA: document.getElementById('turn-left-btn-a'),
                turnRightBtnA: document.getElementById('turn-right-btn-a'),
                hazardBtnA: document.getElementById('hazard-btn-a'),
                headlightsBtnA: document.getElementById('headlights-btn-a'),
                swapLayoutBtnA: document.getElementById('swap-layout-btn-a'),
                turnLeftBtnB: document.getElementById('turn-left-btn-b'),
                turnRightBtnB: document.getElementById('turn-right-btn-b'),
                hazardBtnB: document.getElementById('hazard-btn-b'),
                headlightsBtnB: document.getElementById('headlights-btn-b'),
                acelIzq: document.getElementById('acelIzq'),
                programIterations: document.getElementById('programIterations'),
                programInfinite: document.getElementById('programInfinite'),
                exportConfigBtn: document.getElementById('exportConfigBtn'),
                importConfigInput: document.getElementById('import-config-input'),
                exportProgramBtn: document.getElementById('exportProgramBtn'),
                importProgramBtn: document.getElementById('importProgramBtn'),
                importProgramInput: document.getElementById('import-program-input'),
                runSequenceBtn: document.getElementById('runSequenceBtn'),
                clearSequenceBtn: document.getElementById('clearSequenceBtn'),
                kidModeCommands: document.getElementById('kid-mode-commands'),
                kidModeSequenceContainer: document.getElementById('kid-mode-sequence-container'),
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
            this.attachClick('uploadProgramBtn', this.uploadProgram);
            this.attachClick('runProgramBtn', this.runProgram);
            this.attachClick('stopProgramBtn', this.stopProgram);
            this.attachClick('clearProgramBtn', this.clearProgram);
            this.attachClick('addActionBtn', this.showActionModal);
            this.attachClick('loadProgramBtn', this.loadProgramFromServer);
            this.attachClick('exportConfigBtn', this.exportConfig);
            this.elements.importConfigInput.addEventListener('change', (e) => this.importConfig(e));

            this.attachClick('exportProgramBtn', this.exportProgram);
            this.elements.importProgramBtn.addEventListener('click', () => this.elements.importProgramInput.click());
            this.elements.importProgramInput.addEventListener('change', (e) => this.importProgram(e));
            const swapHandler = () => {
                this.joystickLayoutSwapped = !this.joystickLayoutSwapped;
                localStorage.setItem('joystickLayoutSwapped', this.joystickLayoutSwapped);
                document.getElementById('joystick-a').querySelector('.joystick-layout-single').classList.toggle('swapped', this.joystickLayoutSwapped);
            };
            this.elements.swapLayoutBtnA.addEventListener('click', swapHandler);
            document.querySelectorAll('.icon-toggle, .toggle').forEach(button => {
                this.setupTooltipEvents(button);
            });

            this.elements.programInfinite.addEventListener('change', () => {
                this.elements.programIterations.disabled = this.elements.programInfinite.checked;
                if (this.elements.programInfinite.checked) {
                    this.elements.programIterations.value = -1; // Set to -1 for infinite
                } else {
                    this.elements.programIterations.value = 1; // Reset to 1 when unchecked
                }
            });

            // Listener para los botones de secuencia del modo niños (D-Pad y extras)
            this.elements.kidModeCommands.addEventListener('click', (e) => {
                const commandButton = e.target.closest('.kid-mode-btn[data-command], .d-pad-btn[data-command]');
                if (commandButton) {
                    this.handleKidModeCommand(commandButton.dataset.command);
                }
            });

            this.attachClick('runSequenceBtn', this.runKidSequence);
            this.attachClick('clearSequenceBtn', this.clearKidSequence);
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
                    const yVal = parseFloat(this.joy1.GetY());
                    const xVal = parseFloat(this.joy1.GetX());
                    actBody = {
                        motorSpeed: Math.trunc(Math.min(100, Math.abs(yVal)) * 1024 / 100),
                        motorDirection: yVal < 0 ? "F" : "B",
                        steerDirection: xVal < 0 ? "L" : "R",
                        steerAng: Math.trunc(Math.min(100, Math.abs(xVal)) * 512 / 100),
                        ms: 500
                    };
                } else if (this.activeTab === 'joystick-b') {
                    const acelIzq = document.getElementById("acelIzq").classList.contains('active');
                    const joyX = parseFloat(acelIzq ? this.joy2B.GetX() : this.joy2A.GetX());
                    const joyY = parseFloat(acelIzq ? this.joy2A.GetY() : this.joy2B.GetY());
                    actBody = {
                        motorSpeed: Math.trunc(Math.min(100, Math.abs(joyY)) * 1024 / 100),
                        motorDirection: joyY < 0 ? "F" : "B",
                        steerDirection: joyX < 0 ? "L" : "R",
                        steerAng: Math.trunc(Math.min(100, Math.abs(joyX)) * 512 / 100),
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

        initLights() {
            ['A', 'B'].forEach(tab => {
                this.elements[`turnLeftBtn${tab}`].addEventListener('click', () => this.handleTurnSignal('left'));
                this.elements[`turnRightBtn${tab}`].addEventListener('click', () => this.handleTurnSignal('right'));
                this.elements[`hazardBtn${tab}`].addEventListener('click', () => this.handleHazard());
                this.elements[`headlightsBtn${tab}`].addEventListener('click', () => this.handleHeadlights());
            });
            this.elements.acelIzq.addEventListener('click', () => {
                this.lightsState.acelIzq = !this.lightsState.acelIzq;
                this.elements.acelIzq.classList.toggle('active', this.lightsState.acelIzq);
            });
            this.elements.acelIzq.innerHTML = this.icons.acelIzq;
            this.updateLightsUI();
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
            this.elements.turnLeftBtnA.classList.toggle('active', turnLeft && !hazard);
            this.elements.turnRightBtnA.classList.toggle('active', turnRight && !hazard);
            this.elements.hazardBtnA.classList.toggle('active', hazard);
            this.elements.turnLeftBtnB.classList.toggle('active', turnLeft && !hazard);
            this.elements.turnRightBtnB.classList.toggle('active', turnRight && !hazard);
            this.elements.hazardBtnB.classList.toggle('active', hazard);
            this.elements.acelIzq.classList.toggle('active', acelIzq);
            if (this.activeTab === 'joystick-a') {
                this.elements.turnLeftBtnA.innerHTML = this.icons.turn_left;
                this.elements.turnRightBtnA.innerHTML = this.icons.turn_right;
                this.elements.hazardBtnA.innerHTML = this.icons.hazard;
                this.updateButtonIcon(this.elements.headlightsBtnA, this.lightsState.headlights > 0, 'headlights');
                this.elements.turnLeftBtnB.innerHTML = '';
                this.elements.turnRightBtnB.innerHTML = '';
                this.elements.hazardBtnB.innerHTML = '';
                this.elements.headlightsBtnB.innerHTML = '';
            } else if (this.activeTab === 'joystick-b') {
                this.elements.turnLeftBtnA.innerHTML = '';
                this.elements.turnRightBtnA.innerHTML = '';
                this.elements.hazardBtnA.innerHTML = '';
                this.elements.headlightsBtnA.innerHTML = '';
                this.elements.turnLeftBtnB.innerHTML = this.icons.turn_left;
                this.elements.turnRightBtnB.innerHTML = this.icons.turn_right;
                this.elements.hazardBtnB.innerHTML = this.icons.hazard;
                this.updateButtonIcon(this.elements.headlightsBtnB, this.lightsState.headlights > 0, 'headlights');
            } else {
                this.elements.turnLeftBtnA.innerHTML = '';
                this.elements.turnRightBtnA.innerHTML = '';
                this.elements.hazardBtnA.innerHTML = '';
                this.elements.headlightsBtnA.innerHTML = '';
                this.elements.turnLeftBtnB.innerHTML = '';
                this.elements.turnRightBtnB.innerHTML = '';
                this.elements.hazardBtnB.innerHTML = '';
                this.elements.headlightsBtnB.innerHTML = '';
            }
            this.elements.swapLayoutBtnA.innerHTML = this.icons.swap;
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
            }
        },

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
            element.addEventListener('mouseleave', () => this.hideTooltip());
            element.addEventListener('touchstart', e => {
                this.tooltipTimeout = setTimeout(() => this.showTooltip(e), 500);
            }, { passive: true });
            element.addEventListener('touchend', () => {
                clearTimeout(this.tooltipTimeout);
                this.hideTooltip();
            }, { passive: true });
            element.addEventListener('touchcancel', () => {
                clearTimeout(this.tooltipTimeout);
                this.hideTooltip();
            }, { passive: true });
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

        async exportConfig() {
            try {
                const configData = await this.fetchAPI('api/config/backup');
                if (configData) {
                    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(configData, null, 2));
                    const downloadAnchorNode = document.createElement('a');
                    downloadAnchorNode.setAttribute("href", dataStr);
                    downloadAnchorNode.setAttribute("download", "esprc_config_backup.json");
                    document.body.appendChild(downloadAnchorNode);
                    downloadAnchorNode.click();
                    downloadAnchorNode.remove();
                    alert('Configuration exported successfully!');
                } else {
                    alert('Failed to export configuration.');
                }
            } catch (error) {
                console.error('Error exporting config:', error);
                alert('An error occurred during configuration export.');
            }
        },

        async importConfig(event) {
            const file = event.target.files[0];
            if (!file) return;

            const reader = new FileReader();
            reader.onload = async (e) => {
                try {
                    const importedConfig = JSON.parse(e.target.result);
                    const response = await this.fetchAPI('api/config/restore', {
                        method: 'POST',
                        body: JSON.stringify(importedConfig),
                        headers: { 'Content-Type': 'application/json' }
                    });
                    if (response === null || response.ok) { // Assuming 200 OK or 204 No Content for success
                        alert('Configuration imported successfully! ESP32 will restart if WiFi settings were changed.');
                        // Optionally, refresh config after a short delay if no restart
                        setTimeout(() => this.getConfig(), 3000);
                    } else {
                        alert('Failed to import configuration. Please check the file format.');
                    }
                } catch (error) {
                    console.error('Error importing config:', error);
                    alert('An error occurred during configuration import. Invalid JSON or file format.');
                }
                event.target.value = ''; // Clear the file input
            };
            reader.readAsText(file);
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
        },

        programSequence: [],
        draggedIndex: null,

        async uploadProgram() {
            this.updateSequenceFromUI();
            const programForBackend = this.programSequence.map(p => {
                if (p.action === 'lights') {
                    return { action: p.lightAction, duration: p.duration };
                }
                return p;
            });
            await this.fetchAPI('api/program', { 
                method: 'POST', 
                body: JSON.stringify(programForBackend),
                headers: { 'Content-Type': 'application/json' } 
            });
        },

        async runProgram() {
            let iterations = parseInt(this.elements.programIterations.value, 10);
            if (isNaN(iterations) || iterations < -1) {
                iterations = 1; // Default to 1 if invalid
            }

            let url = 'api/program/run';
            if (iterations !== 1) { // Only add parameter if not default
                url += `?iterations=${iterations}`;
            }
            await this.fetchAPI(url);
        },
        async stopProgram() { await this.fetchAPI('api/program/stop'); },
        async clearProgram() {
            await this.fetchAPI('api/program/clear');
            this.programSequence = [];
            this.renderProgramSequence();
        },
        async loadProgramFromServer() {
            const programFromServer = await this.fetchAPI('api/program');
            if (!programFromServer) return;
            this.programSequence = programFromServer.map(action => {
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
            this.renderProgramSequence();
        },

        async exportProgram() {
            try {
                const programData = await this.fetchAPI('api/program');
                if (programData) {
                    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(programData, null, 2));
                    const downloadAnchorNode = document.createElement('a');
                    downloadAnchorNode.setAttribute("href", dataStr);
                    downloadAnchorNode.setAttribute("download", "esprc_program.json");
                    document.body.appendChild(downloadAnchorNode);
                    downloadAnchorNode.click();
                    downloadAnchorNode.remove();
                } else {
                    alert(translations[this.currentLanguage].programExportError);
                }
            } catch (error) {
                console.error('Error exporting program:', error);
                alert(translations[this.currentLanguage].programExportError);
            }
        },

        async importProgram(event) {
            const file = event.target.files[0];
            if (!file) return;

            const reader = new FileReader();
            reader.onload = async (e) => {
                try {
                    const program = JSON.parse(e.target.result);
                    if (!Array.isArray(program)) {
                        throw new Error("Invalid JSON format for program. Must be an array.");
                    }

                    await this.fetchAPI('api/program', {
                        method: 'POST',
                        body: JSON.stringify(program),
                        headers: { 'Content-Type': 'application/json' }
                    });

                    await this.loadProgramFromServer();
                    alert(translations[this.currentLanguage].programImportSuccess);
                } catch (error) {
                    console.error('Error importing program:', error);
                    alert(translations[this.currentLanguage].programImportError);
                }
                event.target.value = '';
            };
            reader.readAsText(file);
        },
        showActionModal() {
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
                    this.addAction(e.target.dataset.actionType);
                    document.getElementById('action-modal-overlay').remove();
                });
            });
        },
        addAction(type) {
            const newAction = { action: type, duration: 1000 };
            if (type === 'move') {
                newAction.motorSpeed = 0;
                newAction.steerAngle = 0;
            }
            if (type === 'lights') {
                newAction.lightAction = 'lights_cycle';
            }
            this.programSequence.push(newAction);
            this.renderProgramSequence();
        },
        deleteAction(index) {
            this.programSequence.splice(index, 1);
            this.renderProgramSequence();
        },
        duplicateAction(index) {
            this.updateSequenceFromUI();
            const actionToDuplicate = JSON.parse(JSON.stringify(this.programSequence[index]));
            this.programSequence.splice(index + 1, 0, actionToDuplicate);
            this.renderProgramSequence();
        },
        updateSequenceFromUI() {
            const container = document.getElementById('program-sequence-container');
            const newSequence = [];
            container.querySelectorAll('.action-card').forEach((card) => {
                const oldIndex = parseInt(card.dataset.index, 10);
                const action = { 
                    action: this.programSequence[oldIndex].action, 
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
            this.programSequence = newSequence;
        },
        renderProgramSequence() {
            const container = document.getElementById('program-sequence-container');
            container.innerHTML = '';
            this.programSequence.forEach((action, index) => {
                const card = document.createElement('div');
                card.className = 'action-card';
                card.dataset.index = index;
                card.innerHTML = this.getActionCardHTML(action, index);
                container.appendChild(card);
            });
            this.attachActionCardListeners();
        },
        getActionCardHTML(action, index) {
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
                            <input type="range" class="action-speed" min="-255" max="255" value="${action.motorSpeed}">
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
        },

        attachActionCardListeners() {
            document.querySelectorAll('.delete-action-btn').forEach(btn => {
                btn.onclick = (e) => {
                    e.stopPropagation();
                    this.deleteAction(parseInt(e.currentTarget.dataset.index, 10));
                }
            });

            document.querySelectorAll('.duplicate-action-btn').forEach(btn => {
                btn.onclick = (e) => {
                    e.stopPropagation();
                    this.duplicateAction(parseInt(e.currentTarget.dataset.index, 10));
                }
            });

            document.querySelectorAll('.action-card .icon-toggle, .action-card .toggle, .action-card .danger, .action-card .secondary').forEach(button => {
                this.setupTooltipEvents(button);
            });

            document.querySelectorAll('.action-card input[type="range"]').forEach(slider => {
                slider.addEventListener('input', (e) => {
                    if (e.target.previousElementSibling){
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
                    this.draggedIndex = parseInt(card.dataset.index, 10);
                    setTimeout(() => card.classList.add('dragging'), 0);
                });

                handle.addEventListener('dragend', (e) => {
                    const card = e.currentTarget.closest('.action-card');
                    if(card) card.classList.remove('dragging');
                    this.draggedIndex = null;
                });
            });

            container.addEventListener('dragover', (e) => {
                e.preventDefault();
                const afterElement = this.getDragAfterElement(container, e.clientY);
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
                this.updateSequenceFromUI();
                this.renderProgramSequence();
            });
        },

        getDragAfterElement(container, y) {
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
        },

        handleKidModeCommand(command) {
            this.kidSequence.push(command);
            this.renderKidSequence();
        },

        renderKidSequence() {
            this.elements.kidModeSequenceContainer.innerHTML = '';
            this.kidSequence.forEach(command => {
                const iconContainer = document.createElement('div');
                iconContainer.className = 'sequence-icon';
                const commandButton = this.elements.kidModeCommands.querySelector(`[data-command=${command}]`);
                if (commandButton) {
                    iconContainer.innerHTML = commandButton.querySelector('svg').outerHTML;
                }
                this.elements.kidModeSequenceContainer.appendChild(iconContainer);
            });
        },

        async runKidSequence() {
            await this.fetchAPI('api/sequence', {
                method: 'POST',
                body: JSON.stringify({ commands: this.kidSequence }),
                headers: { 'Content-Type': 'application/json' }
            });
        },

        clearKidSequence() {
            this.kidSequence = [];
            this.renderKidSequence();
        }
    };

    window.app = app;
    app.init();
});
