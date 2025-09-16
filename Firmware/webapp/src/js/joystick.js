import { state, elements, icons } from './state.js';
import { fetchAPI } from './api.js';

class JoyStick {
    constructor(containerId, options, callback) {
        options = options || {};
        this.title = options.title === undefined ? 'joystick' : options.title;
        this.width = options.width === undefined ? 0 : options.width;
        this.height = options.height === undefined ? 0 : options.height;
        this.internalFillColor = options.internalFillColor === undefined ? '#00AA00' : options.internalFillColor;
        this.internalLineWidth = options.internalLineWidth === undefined ? 2 : options.internalLineWidth;
        this.internalStrokeColor = options.internalStrokeColor === undefined ? '#003300' : options.internalStrokeColor;
        this.externalLineWidth = options.externalLineWidth === undefined ? 2 : options.externalLineWidth;
        this.externalStrokeColor = options.externalStrokeColor === undefined ? '#008000' : options.externalStrokeColor;
        this.autoReturnToCenter = options.autoReturnToCenter === undefined ? true : options.autoReturnToCenter;
        this.callback = callback || function () { };

        this.container = document.getElementById(containerId);
        this.container.style.touchAction = 'none';

        this.canvas = document.createElement('canvas');
        this.canvas.id = this.title;
        if (this.width === 0) this.width = this.container.clientWidth;
        if (this.height === 0) this.height = this.container.clientHeight;
        this.canvas.width = this.width;
        this.canvas.height = this.height;
        this.container.appendChild(this.canvas);

        this.ctx = this.canvas.getContext('2d');
        this.pressed = 0;
        this.twoPi = 2 * Math.PI;

        this.internalRadius = (this.canvas.width - (this.canvas.width / 2 + 10)) / 2;
        this.internalRadius_2 = this.internalRadius + 5;
        this.externalRadius = this.internalRadius + 30;

        this.centerX = this.canvas.width / 2;
        this.centerY = this.canvas.height / 2;

        this.joyX = this.width / 10;
        this.joyY = this.height / 10;
        this.joyX_neg = -1 * this.joyX;
        this.joyY_neg = -1 * this.joyY;

        this.movedX = this.centerX;
        this.movedY = this.centerY;

        this.setupEventListeners();
        this.startAnimation();
    }

    drawExternal() {
        this.ctx.beginPath();
        this.ctx.arc(this.centerX, this.centerY, this.externalRadius, 0, this.twoPi, false);
        this.ctx.lineWidth = this.externalLineWidth;
        this.ctx.strokeStyle = this.externalStrokeColor;
        this.ctx.stroke();
    }

    drawInternal() {
        this.ctx.beginPath();
        if (this.movedX < this.internalRadius) this.movedX = this.internalRadius_2;
        if ((this.movedX + this.internalRadius) > this.canvas.width) this.movedX = this.canvas.width - this.internalRadius_2;
        if (this.movedY < this.internalRadius) this.movedY = this.internalRadius_2;
        if ((this.movedY + this.internalRadius) > this.canvas.height) this.movedY = this.canvas.height - this.internalRadius_2;

        this.ctx.arc(this.movedX, this.movedY, this.internalRadius, 0, this.twoPi, false);

        const grd = this.ctx.createRadialGradient(this.centerX, this.centerY, 5, this.centerX, this.centerY, 200);
        grd.addColorStop(0, this.internalFillColor);
        grd.addColorStop(1, this.internalStrokeColor);

        this.ctx.fillStyle = grd;
        this.ctx.fill();
        this.ctx.lineWidth = this.internalLineWidth;
        this.ctx.strokeStyle = this.internalStrokeColor;
        this.ctx.stroke();
    }

    getCardinalDirection() {
        let direction = "";
        const diffX = this.movedX - this.centerX;
        const diffY = this.movedY - this.centerY;

        if (diffY >= this.joyY_neg && diffY <= this.joyY) direction = "C";
        if (diffY < this.joyY_neg) direction = "N";
        if (diffY > this.joyY) direction = "S";
        if (diffX < this.joyX_neg) direction = (direction === "C") ? "W" : direction + "W";
        if (diffX > this.joyX) direction = (direction === "C") ? "E" : direction + "E";

        return direction;
    }

    setupEventListeners() {
        const move = (e) => {
            if (this.pressed) {
                const rect = this.canvas.getBoundingClientRect();
                let x, y;
                if (e.type === 'touchmove') {
                    x = e.targetTouches[0].clientX - rect.left;
                    y = e.targetTouches[0].clientY - rect.top;
                } else {
                    x = e.clientX - rect.left;
                    y = e.clientY - rect.top;
                }
                this.movedX = x;
                this.movedY = y;
            }
        };

        const up = () => {
            this.pressed = 0;
            if (this.autoReturnToCenter) {
                this.movedX = this.centerX;
                this.movedY = this.centerY;
            }
        };

        this.canvas.addEventListener('mousedown', () => this.pressed = 1, false);
        this.canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            this.pressed = 1;
            move(e);
        }, { passive: false });

        document.addEventListener('mousemove', move, false);
        document.addEventListener('touchmove', move, { passive: false });

        document.addEventListener('mouseup', up, false);
        document.addEventListener('touchend', up, false);
    }

    startAnimation() {
        setInterval(() => {
            this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
            this.drawExternal();
            this.drawInternal();
            const data = {
                xPosition: this.movedX,
                yPosition: this.movedY,
                x: this.GetX(),
                y: this.GetY(),
                cardinalDirection: this.getCardinalDirection()
            };
            this.callback(data);
        }, 10);
    }

    GetX() {
        return ((this.movedX - this.centerX) / this.internalRadius * 100).toFixed();
    }

    GetY() {
        return ((this.movedY - this.centerY) / this.internalRadius * 100).toFixed();
    }

    GetDir() {
        return this.getCardinalDirection();
    }
}


let joy1, joy2A, joy2B;

export function initJoysticks() {
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
    joy1 = new JoyStick('joy1Div', joystickOptions, () => { });
    joy2A = new JoyStick('joy2ADiv', joystickOptions, () => { });
    joy2B = new JoyStick('joy2BDiv', joystickOptions, () => { });
    elements[`recordBtnA`].addEventListener('click', () => handleRecord());
    elements[`recordBtnB`].addEventListener('click', () => handleRecord());
    elements.recordBtnA.innerHTML = icons.recordOff;
    elements.recordBtnB.innerHTML = icons.recordOff;
}

export function startActionLoop() {
    setInterval(() => {
        if (!state.socketOnline) return;
        let actBody = {};
        if (state.activeTab === 'joystick-a') {
            const yVal = parseFloat(joy1.GetY());
            const xVal = parseFloat(joy1.GetX());
            actBody = {
                motorSpeed: Math.trunc(Math.min(100, Math.abs(yVal)) * 1024 / 100),
                motorDirection: yVal < 0 ? "F" : "B",
                steerDirection: xVal < 0 ? "L" : "R",
                steerAng: Math.trunc(Math.min(100, Math.abs(xVal)) * 512 / 100),
                ms: 500
            };
        } else if (state.activeTab === 'joystick-b') {
            const acelIzq = document.getElementById("acelIzq").classList.contains('active');
            const joyX = parseFloat(acelIzq ? joy2B.GetX() : joy2A.GetX());
            const joyY = parseFloat(acelIzq ? joy2A.GetY() : joy2B.GetY());
            actBody = {
                motorSpeed: Math.trunc(Math.min(100, Math.abs(joyY)) * 1024 / 100),
                motorDirection: joyY < 0 ? "F" : "B",
                steerDirection: joyX < 0 ? "L" : "R",
                steerAng: Math.trunc(Math.min(100, Math.abs(joyX)) * 512 / 100),
                ms: 500
            };
        } else { return; }
        state.wsSocket.send(JSON.stringify(actBody));
    }, 100);
}

export async function handleRecord() {
    state.recordingProgram = !state.recordingProgram;
    elements[`recordBtnA`].classList.toggle('active', state.recordingProgram);
    elements[`recordBtnB`].classList.toggle('active', state.recordingProgram);
    let path = (state.recordingProgram) ? 'api/recording/start' : 'api/recording/stop';
    await fetchAPI(path)
}

export function handleJoystickLayoutSwap() {
    state.joystickLayoutSwapped = !state.joystickLayoutSwapped;
    const joy1Container = document.querySelector('#joystick-a .joystick-container');
    const joy1Info = document.querySelector('#joystick-a .joystick-info-card');
    if (state.joystickLayoutSwapped) {
        joy1Container.style.order = 2;
        joy1Info.style.order = 1;
    } else {
        joy1Container.style.order = 1;
        joy1Info.style.order = 2;
    }
}