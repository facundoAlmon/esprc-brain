# Project Overview


Estoy trabajando en el proyecto **esprc-brain**, un sistema de control para un coche de radiocontrol basado en un ESP32. El proyecto consta de dos partes principales:

1.  **Firmware del ESP32:** Escrito en C++ utilizando el framework ESP-IDF. Se encuentra en el directorio `Firmware/main`.
2.  **Aplicación Web (WebApp):** Una interfaz de usuario web para controlar el coche. Está escrita en HTML, CSS y JavaScript y se encuentra en el directorio `Firmware/webapp`.

El `README.md` del proyecto proporciona una descripción detallada de las características, la estructura del proyecto y las instrucciones de configuración.

## Key Technologies

*   **Microcontroller:** ESP32
*   **Firmware:** C++, ESP-IDF
*   **Web App:** HTML, CSS, JavaScript
*   **Communication:** Wi-Fi (AP and STA modes), WebSockets, REST API, Bluetooth (for gamepads)
*   **Libraries:**
    *   **Bluepad32:** For Bluetooth gamepad support.
    *   **ArduinoJson:** For parsing and generating JSON data.
    *   **ESPAsyncWebServer:** For the web server and WebSockets.


## Development Conventions

*   The firmware is written in C++ and follows the conventions of the ESP-IDF framework.
*   The web application uses a modular structure, with separate files for HTML, CSS, and JavaScript.
*   Communication between the web app and the firmware is done using JSON over WebSockets and a RESTful API.
*   Configuration data is stored in the ESP32's non-volatile storage (NVS).




## Estructura de Archivos Relevante

```
esprc-brain/
├── Firmware/
│   ├── main/             # Código fuente principal del ESP32 (C++).
│   │   ├── src/          # Archivos .cpp con la lógica de la aplicación.
│   │   └── include/      # Archivos de cabecera .h.
│   │   └── main.c        # Punto de entrada principal del firmware.
│   │   └── index.html    # La webapp compilada que se embebe en el firmware.
│   │
│   ├── webapp/           # Código fuente de la aplicación web.
│   │   ├── src/
│   │   │   ├── index.html  # Archivo HTML principal de la webapp.
│   │   │   ├── style.css   # Estilos CSS.
│   │   │   └── script.js   # Lógica de la aplicación en JavaScript.
│   │   └── gulpfile.js     # Script de compilación de la webapp.
│   │
│   └── components/       # Librerías y componentes de ESP-IDF.
│
└── README.md             # Documentación principal del proyecto.
```

## Tarea

Tu tarea es ayudarme a realizar cambios en el proyecto. Dependiendo de la solicitud, es posible que necesites modificar el firmware del ESP32, la aplicación web o ambos.

### Modificaciones en el Firmware (C++)

-   Los archivos principales se encuentran en `Firmware/main/src` y `Firmware/main/include`.
-   El punto de entrada es `Firmware/main/main.c`.
-   El firmware utiliza el framework ESP-IDF.
-   La comunicación con la webapp se realiza a través de una API REST y WebSockets.

### Modificaciones en la Aplicación Web (HTML/CSS/JS)

-   Los archivos fuente se encuentran en `Firmware/webapp/src`.
-   La webapp se compila en un único archivo `index.html` utilizando Gulp. El `gulpfile.js` define el proceso de compilación.
-   Después de modificar la webapp, es necesario ejecutar `npm run build` en el directorio `Firmware/webapp` para generar el nuevo `index.html` y copiarlo a `Firmware/main`.
-   La interfaz de usuario se controla mediante `Firmware/webapp/src/script.js`.
-   La comunicación con el ESP32 se gestiona en `script.js` utilizando `fetch` para la API REST y `WebSocket` para la comunicación en tiempo real.

## Ejemplo de Solicitud

"Quiero agregar un nuevo botón a la aplicación web que, al presionarlo, envíe un comando al ESP32 para hacer sonar una bocina. Esto implicaría:

1.  **En la webapp:**
    *   Agregar un botón en `Firmware/webapp/src/index.html`.
    *   Agregar un event listener en `Firmware/webapp/src/script.js` para que el botón envíe una solicitud a un nuevo endpoint de la API (por ejemplo, `/api/horn`).

2.  **En el firmware:**
    *   Agregar un nuevo endpoint en el servidor web del ESP32 para manejar `/api/horn`.
    *   Implementar la lógica para activar la bocina (por ejemplo, controlar un pin GPIO).
"
