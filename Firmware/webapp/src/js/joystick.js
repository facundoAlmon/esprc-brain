import { state, elements, icons } from './state.js';
import { fetchAPI } from './api.js';

const TWO_PI = 2 * Math.PI;
const DEADZONE_RATIO = 0.12; // 12% of internal radius

class JoyStick {
    constructor(containerId, options) {
        options = options || {};
        this.title = options.title || 'joystick';
        this.autoReturnToCenter = options.autoReturnToCenter !== undefined ? options.autoReturnToCenter : true;
        this.callback = options.callback || function() {};

        // Colors (read from CSS vars or use fallbacks)
        this.fillColor   = options.internalFillColor   || '#E8401C';
        this.strokeColor = options.internalStrokeColor || '#7A200E';
        this.ringColor   = options.externalStrokeColor || 'rgba(232,64,28,.55)';
        this.isLight     = options.lightMode || false;

        this.container = document.getElementById(containerId);
        this.container.style.touchAction = 'none';

        this.canvas = document.createElement('canvas');
        this.canvas.id = this.title;
        this.canvas.style.display = 'block';
        this.canvas.style.borderRadius = '50%';
        // CSS fills the container regardless of pixel buffer size (fixes hidden-tab init)
        this.canvas.style.width  = '100%';
        this.canvas.style.height = '100%';

        this._resize();
        this.container.appendChild(this.canvas);
        this.ctx = this.canvas.getContext('2d');

        this.pressed = false;
        this._pointerId = null;
        this.movedX = this.centerX;
        this.movedY = this.centerY;

        this._onDown = this._onDown.bind(this);
        this._onMove = this._onMove.bind(this);
        this._onUp   = this._onUp.bind(this);

        this.canvas.addEventListener('pointerdown',   this._onDown);
        this.canvas.addEventListener('pointermove',   this._onMove);
        this.canvas.addEventListener('pointerup',     this._onUp);
        this.canvas.addEventListener('pointercancel', this._onUp);

        this._rafId = requestAnimationFrame(this._loop.bind(this));
    }

    _resize() {
        const size = this.container.clientWidth || 280;
        this.canvas.width  = size;
        this.canvas.height = size;
        this.centerX = size / 2;
        this.centerY = size / 2;
        // proportional radii: ext at 68% of half-size, handle at 36%
        this.externalRadius = (size / 2) * 0.68;
        this.internalRadius = (size / 2) * 0.36;
        this._dz = this.internalRadius * DEADZONE_RATIO;
        this.movedX = this.centerX;
        this.movedY = this.centerY;
    }

    // ── Pointer Events ───────────────────────────────────
    _onDown(e) {
        if (this._pointerId !== null) return;
        e.preventDefault();
        this.pressed = true;
        this._pointerId = e.pointerId;
        this.canvas.setPointerCapture(e.pointerId);
        this._updatePos(e);
    }

    _onMove(e) {
        if (!this.pressed || e.pointerId !== this._pointerId) return;
        e.preventDefault();
        this._updatePos(e);
    }

    _onUp(e) {
        if (e.pointerId !== this._pointerId) return;
        this.pressed = false;
        this._pointerId = null;
        if (this.autoReturnToCenter) {
            this.movedX = this.centerX;
            this.movedY = this.centerY;
        }
    }

    _updatePos(e) {
        const rect = this.canvas.getBoundingClientRect();
        const scaleX = this.canvas.width  / rect.width;
        const scaleY = this.canvas.height / rect.height;
        const x = (e.clientX - rect.left) * scaleX;
        const y = (e.clientY - rect.top)  * scaleY;

        const dx = x - this.centerX;
        const dy = y - this.centerY;
        const dist = Math.sqrt(dx * dx + dy * dy);

        if (dist > this.externalRadius) {
            const angle = Math.atan2(dy, dx);
            this.movedX = this.centerX + this.externalRadius * Math.cos(angle);
            this.movedY = this.centerY + this.externalRadius * Math.sin(angle);
        } else {
            this.movedX = x;
            this.movedY = y;
        }
    }

    // ── Animation Loop ───────────────────────────────────
    _loop() {
        this._draw();
        this.callback({
            xPosition: this.movedX,
            yPosition: this.movedY,
            x: this.GetX(),
            y: this.GetY(),
            cardinalDirection: this._cardinalDir()
        });
        this._rafId = requestAnimationFrame(this._loop.bind(this));
    }

    // ── Drawing ──────────────────────────────────────────
    _draw() {
        const ctx = this.ctx;
        const cx = this.centerX, cy = this.centerY;
        const ext = this.externalRadius, int = this.internalRadius, dz = this._dz;
        const W = this.canvas.width, H = this.canvas.height;

        ctx.clearRect(0, 0, W, H);

        const dx = this.movedX - cx;
        const dy = this.movedY - cy;
        const dist = Math.sqrt(dx * dx + dy * dy);
        const normDist = Math.min(1, dist / ext);

        // ── 1. Background — fill entire canvas circle ─────
        const fullR = cx; // full radius of the clipped canvas circle
        ctx.beginPath();
        ctx.arc(cx, cy, fullR, 0, TWO_PI);
        const bgGrad = ctx.createRadialGradient(cx, cy, 0, cx, cy, fullR);
        if (this.isLight) {
            bgGrad.addColorStop(0,              'rgba(230,227,223,0.99)');
            bgGrad.addColorStop(ext / fullR,    'rgba(220,217,213,0.99)');
            bgGrad.addColorStop(1,              'rgba(210,207,203,0.99)');
        } else {
            bgGrad.addColorStop(0,              'rgba(22,22,25,0.99)');
            bgGrad.addColorStop(ext / fullR,    'rgba(13,13,15,0.99)');
            bgGrad.addColorStop(1,              'rgba(8,8,10,0.99)');
        }
        ctx.fillStyle = bgGrad;
        ctx.fill();

        // ── 2. Compass guide lines ────────────────────────
        ctx.save();
        ctx.strokeStyle = this.isLight ? 'rgba(0,0,0,0.07)' : 'rgba(255,255,255,0.06)';
        ctx.lineWidth = 1;
        for (let i = 0; i < 8; i++) {
            const a = (i * Math.PI) / 4;
            const startR = dz * 1.5;
            const endR   = ext - 5;
            ctx.beginPath();
            ctx.moveTo(cx + startR * Math.cos(a), cy + startR * Math.sin(a));
            ctx.lineTo(cx + endR   * Math.cos(a), cy + endR   * Math.sin(a));
            ctx.stroke();
        }
        ctx.restore();

        // ── 3. Deadzone ring ──────────────────────────────
        ctx.save();
        ctx.beginPath();
        ctx.arc(cx, cy, dz, 0, TWO_PI);
        ctx.setLineDash([3, 5]);
        ctx.lineWidth = 1;
        ctx.strokeStyle = this.isLight ? 'rgba(0,0,0,0.15)' : 'rgba(255,255,255,0.13)';
        ctx.stroke();
        ctx.setLineDash([]);
        ctx.restore();

        // ── 4. External ring ──────────────────────────────
        ctx.beginPath();
        ctx.arc(cx, cy, ext, 0, TWO_PI);
        ctx.lineWidth = 1.5;
        ctx.strokeStyle = this.ringColor;
        ctx.stroke();

        // ── 5. Directional sector + velocity line ─────────
        if (dist > dz) {
            const angle = Math.atan2(dy, dx);
            const alpha = Math.min(1, (dist - dz) / (ext - dz));

            // Arc sector glow in movement direction
            ctx.save();
            ctx.beginPath();
            ctx.arc(cx, cy, ext - 7, angle - Math.PI / 3.5, angle + Math.PI / 3.5);
            ctx.lineWidth = 7;
            ctx.strokeStyle = `rgba(232,64,28,${alpha * 0.55})`;
            ctx.lineCap = 'round';
            ctx.stroke();

            // Outer ring pulse when at boundary
            if (alpha > 0.65) {
                const pulseAlpha = (alpha - 0.65) / 0.35;
                ctx.beginPath();
                ctx.arc(cx, cy, ext + 1, 0, TWO_PI);
                ctx.lineWidth = 2.5;
                ctx.strokeStyle = `rgba(232,64,28,${pulseAlpha * 0.7})`;
                ctx.stroke();
            }
            ctx.restore();

            // Velocity line
            ctx.save();
            ctx.beginPath();
            ctx.moveTo(cx, cy);
            ctx.lineTo(this.movedX, this.movedY);
            ctx.lineWidth = 1;
            ctx.strokeStyle = `rgba(232,64,28,${alpha * 0.35})`;
            ctx.stroke();
            ctx.restore();
        }

        // ── 6. Handle ─────────────────────────────────────
        const isActive = this.pressed && dist > dz;

        // Handle shadow / outer glow
        if (isActive && normDist > 0.5) {
            ctx.save();
            ctx.beginPath();
            ctx.arc(this.movedX, this.movedY, int + 5, 0, TWO_PI);
            ctx.lineWidth = 2.5;
            const glowAlpha = (normDist - 0.5) / 0.5;
            ctx.strokeStyle = `rgba(232,64,28,${glowAlpha * 0.6})`;
            ctx.stroke();
            ctx.restore();
        }

        // Handle fill
        const hx = this.movedX - int * 0.3;
        const hy = this.movedY - int * 0.3;
        const hGrad = ctx.createRadialGradient(hx, hy, 1, this.movedX, this.movedY, int);
        if (isActive) {
            hGrad.addColorStop(0, this._lighten(this.fillColor, 0.3));
            hGrad.addColorStop(1, this.strokeColor);
        } else {
            const mid = this.isLight ? 'rgba(130,130,135,0.75)' : 'rgba(100,100,108,0.85)';
            const dark = this.isLight ? 'rgba(90,90,95,0.85)' : 'rgba(50,50,58,0.9)';
            hGrad.addColorStop(0, mid);
            hGrad.addColorStop(1, dark);
        }

        ctx.beginPath();
        ctx.arc(this.movedX, this.movedY, int, 0, TWO_PI);
        ctx.fillStyle = hGrad;
        ctx.fill();
        ctx.lineWidth = 1.5;
        ctx.strokeStyle = isActive ? this.strokeColor : (this.isLight ? 'rgba(100,100,105,0.5)' : 'rgba(160,160,168,0.35)');
        ctx.stroke();

        // Center dot
        ctx.beginPath();
        ctx.arc(cx, cy, 2.5, 0, TWO_PI);
        ctx.fillStyle = this.isLight ? 'rgba(0,0,0,0.2)' : 'rgba(255,255,255,0.15)';
        ctx.fill();
    }

    _lighten(hex, amount) {
        const r = parseInt(hex.slice(1,3),16);
        const g = parseInt(hex.slice(3,5),16);
        const b = parseInt(hex.slice(5,7),16);
        return `rgb(${Math.min(255,r+Math.round(amount*120))},${Math.min(255,g+Math.round(amount*60))},${Math.min(255,b+Math.round(amount*40))})`;
    }

    // ── Output ───────────────────────────────────────────
    GetX() {
        const dx = this.movedX - this.centerX;
        const dy = this.movedY - this.centerY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist < this._dz) return '0';
        return (dx / this.internalRadius * 100).toFixed();
    }

    GetY() {
        const dx = this.movedX - this.centerX;
        const dy = this.movedY - this.centerY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist < this._dz) return '0';
        return (dy / this.internalRadius * 100).toFixed();
    }

    GetDir() { return this._cardinalDir(); }

    _cardinalDir() {
        const joyX = this.canvas.width  / 10;
        const joyY = this.canvas.height / 10;
        const diffX = this.movedX - this.centerX;
        const diffY = this.movedY - this.centerY;
        let dir = '';
        if      (diffY < -joyY) dir = 'N';
        else if (diffY >  joyY) dir = 'S';
        else                    dir = 'C';
        if      (diffX < -joyX) dir = dir === 'C' ? 'W' : dir + 'W';
        else if (diffX >  joyX) dir = dir === 'C' ? 'E' : dir + 'E';
        return dir;
    }

    destroy() {
        cancelAnimationFrame(this._rafId);
        this.canvas.removeEventListener('pointerdown',   this._onDown);
        this.canvas.removeEventListener('pointermove',   this._onMove);
        this.canvas.removeEventListener('pointerup',     this._onUp);
        this.canvas.removeEventListener('pointercancel', this._onUp);
        if (this.container.contains(this.canvas)) this.container.removeChild(this.canvas);
    }
}


// Invisible pan/tilt pad over the FPV stream. Drag anywhere on it: the offset
// from the touch origin maps to -100..100 per axis; returns to 0 on release.
class CamPad {
    constructor(containerId) {
        this.el = document.getElementById(containerId);
        this.ring = document.getElementById('fpvPadRing');
        this.dot = document.getElementById('fpvPadDot');
        this.x = 0;
        this.y = 0;
        this._pointerId = null;
        this._originX = 0;
        this._originY = 0;
        this.el.style.touchAction = 'none';

        this._onDown = this._onDown.bind(this);
        this._onMove = this._onMove.bind(this);
        this._onUp   = this._onUp.bind(this);
        this.el.addEventListener('pointerdown',   this._onDown);
        this.el.addEventListener('pointermove',   this._onMove);
        this.el.addEventListener('pointerup',     this._onUp);
        this.el.addEventListener('pointercancel', this._onUp);
    }

    _onDown(e) {
        if (this._pointerId !== null) return;
        if (!state.camServoEnabled) return;
        e.preventDefault();
        this._pointerId = e.pointerId;
        this.el.setPointerCapture(e.pointerId);
        this._originX = e.clientX;
        this._originY = e.clientY;
        this.x = 0;
        this.y = 0;
        const rect = this.el.getBoundingClientRect();
        if (this.ring) {
            this.ring.style.display = 'block';
            this.ring.style.left = (e.clientX - rect.left) + 'px';
            this.ring.style.top  = (e.clientY - rect.top)  + 'px';
        }
        this._moveDot(e.clientX, e.clientY, rect);
    }

    _onMove(e) {
        if (e.pointerId !== this._pointerId) return;
        e.preventDefault();
        const rect = this.el.getBoundingClientRect();
        const radius = Math.min(140, Math.min(rect.width, rect.height) * 0.3);
        let dx = e.clientX - this._originX;
        let dy = e.clientY - this._originY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist > radius) {
            dx = dx / dist * radius;
            dy = dy / dist * radius;
        }
        this.x = Math.round(dx / radius * 100);
        this.y = Math.round(dy / radius * 100);
        this._moveDot(this._originX + dx, this._originY + dy, rect);
    }

    _onUp(e) {
        if (e.pointerId !== this._pointerId) return;
        this._pointerId = null;
        this.x = 0;
        this.y = 0;
        if (this.ring) this.ring.style.display = 'none';
        if (this.dot)  this.dot.style.display  = 'none';
    }

    _moveDot(clientX, clientY, rect) {
        if (!this.dot) return;
        this.dot.style.display = 'block';
        this.dot.style.left = (clientX - rect.left) + 'px';
        this.dot.style.top  = (clientY - rect.top)  + 'px';
    }

    GetX() { return this.x; }
    GetY() { return this.y; }
}


let joy1, joy2A, joy2B, joy3, joyFpv;
let fpvPad = null;
let _resizeTimer = null;

// Only updates CSS visibility — does NOT recreate joysticks (avoids state loss on config refresh)
export function updateCamJoyVisibility() {
    const display = state.camServoEnabled ? '' : 'none';
    const container = document.getElementById('camJoyContainer');
    if (container) container.style.display = display;
    document.querySelectorAll('.cam-ctl-wrap').forEach(function(el) {
        el.style.display = display;
    });
}

export function initJoysticks() {
    const styles   = getComputedStyle(document.documentElement);
    const isLight  = document.body.classList.contains('light-mode');
    const fillColor   = styles.getPropertyValue('--joy-fill').trim()   || '#E8401C';
    const strokeColor = styles.getPropertyValue('--joy-stroke').trim() || '#7A200E';
    const ringColor   = styles.getPropertyValue('--joy-ring').trim()   || 'rgba(232,64,28,.55)';

    const opts = {
        autoReturnToCenter: true,
        internalFillColor:   fillColor,
        internalStrokeColor: strokeColor,
        externalStrokeColor: ringColor,
        lightMode: isLight,
        callback: () => {}
    };

    if (joy1)   joy1.destroy();
    if (joy2A)  joy2A.destroy();
    if (joy2B)  joy2B.destroy();
    if (joy3)   joy3.destroy();
    if (joyFpv) joyFpv.destroy();

    joy1   = new JoyStick('joy1Div',   { ...opts, title: 'joy1'   });
    joy2A  = new JoyStick('joy2ADiv',  { ...opts, title: 'joy2A'  });
    joy2B  = new JoyStick('joy2BDiv',  { ...opts, title: 'joy2B'  });
    joy3   = new JoyStick('joy3Div',   { ...opts, title: 'joy3'   });
    joyFpv = new JoyStick('joyFpvDiv', { ...opts, title: 'joyFpv' });

    if (!fpvPad && document.getElementById('fpvCamPad')) fpvPad = new CamPad('fpvCamPad');

    elements.recordBtnA.onclick = () => handleRecord();
    elements.recordBtnB.onclick = () => handleRecord();
    elements.recordBtnA.innerHTML = icons.recordOff;
    elements.recordBtnB.innerHTML = icons.recordOff;
    if (elements.recordBtnFpv) {
        elements.recordBtnFpv.onclick = () => handleRecord();
        elements.recordBtnFpv.innerHTML = icons.recordOff;
    }
}

// Rebuild joysticks on window resize (debounced)
window.addEventListener('resize', () => {
    clearTimeout(_resizeTimer);
    _resizeTimer = setTimeout(initJoysticks, 220);
});

export function startActionLoop() {
    // "Last active input wins": only transmit while there is real input, plus
    // one neutral frame on release (stops the car and lets the BT gamepad take
    // over). While recording a program we always transmit so idle stretches
    // are recorded with their real duration.
    let wasActive = false;
    setInterval(() => {
        if (!state.socketOnline) return;
        let body = {};
        if (state.activeTab === 'joystick-a') {
            const yVal = parseFloat(joy1.GetY());
            const xVal = parseFloat(joy1.GetX());
            body = {
                motorSpeed:     Math.trunc(Math.min(100, Math.abs(yVal)) * 1024 / 100),
                motorDirection: yVal < 0 ? 'F' : 'B',
                steerDirection: xVal < 0 ? 'L' : 'R',
                steerAng:       Math.trunc(Math.min(100, Math.abs(xVal)) * 512 / 100),
                ms: 500
            };
            if (state.camServoEnabled && joy3) {
                body.panAng  = Math.trunc(parseFloat(joy3.GetX()) * 512 / 100);
                body.tiltAng = Math.trunc(parseFloat(joy3.GetY()) * 512 / 100);
            }
        } else if (state.activeTab === 'fpv') {
            const yVal = parseFloat(joyFpv.GetY());
            const xVal = parseFloat(joyFpv.GetX());
            body = {
                motorSpeed:     Math.trunc(Math.min(100, Math.abs(yVal)) * 1024 / 100),
                motorDirection: yVal < 0 ? 'F' : 'B',
                steerDirection: xVal < 0 ? 'L' : 'R',
                steerAng:       Math.trunc(Math.min(100, Math.abs(xVal)) * 512 / 100),
                ms: 500
            };
            if (state.camServoEnabled && fpvPad) {
                body.panAng  = Math.trunc(fpvPad.GetX() * 512 / 100);
                body.tiltAng = Math.trunc(fpvPad.GetY() * 512 / 100);
            }
        } else if (state.activeTab === 'joystick-b') {
            const acelIzq = document.getElementById('acelIzq').classList.contains('active');
            const joyX = parseFloat(acelIzq ? joy2B.GetX() : joy2A.GetX());
            const joyY = parseFloat(acelIzq ? joy2A.GetY() : joy2B.GetY());
            body = {
                motorSpeed:     Math.trunc(Math.min(100, Math.abs(joyY)) * 1024 / 100),
                motorDirection: joyY < 0 ? 'F' : 'B',
                steerDirection: joyX < 0 ? 'L' : 'R',
                steerAng:       Math.trunc(Math.min(100, Math.abs(joyX)) * 512 / 100),
                ms: 500
            };
        } else { return; }
        const isActive = body.motorSpeed > 0 || body.steerAng > 0 ||
                         !!(body.panAng) || !!(body.tiltAng);
        if (isActive || wasActive || state.recordingProgram) {
            state.wsSocket.send(JSON.stringify(body));
        }
        wasActive = isActive;
    }, 100);
}

export async function handleRecord() {
    state.recordingProgram = !state.recordingProgram;
    elements.recordBtnA.classList.toggle('active', state.recordingProgram);
    elements.recordBtnB.classList.toggle('active', state.recordingProgram);
    if (elements.recordBtnFpv) elements.recordBtnFpv.classList.toggle('active', state.recordingProgram);
    const path = state.recordingProgram ? 'api/recording/start' : 'api/recording/stop';
    await fetchAPI(path);
}

export function handleJoystickLayoutSwap() {
    state.joystickLayoutSwapped = !state.joystickLayoutSwapped;
    const layout = document.querySelector('#joystick-a .joystick-layout-single');
    if (layout) layout.classList.toggle('swapped', state.joystickLayoutSwapped);
}
