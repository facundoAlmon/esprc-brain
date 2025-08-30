// Joystick library (simplified)
const JoyStick = function(t, e, i) {
    let o = void 0 === (e = e || {}).title ? "joystick" : e.title,
        n = void 0 === e.width ? 0 : e.width,
        a = void 0 === e.height ? 0 : e.height,
        r = void 0 === e.internalFillColor ? "#00AA00" : e.internalFillColor,
        c = void 0 === e.internalLineWidth ? 2 : e.internalLineWidth,
        s = void 0 === e.internalStrokeColor ? "#003300" : e.internalStrokeColor,
        d = void 0 === e.externalLineWidth ? 2 : e.externalLineWidth,
        u = void 0 === e.externalStrokeColor ? "#008000" : e.externalStrokeColor,
        h = void 0 === e.autoReturnToCenter || e.autoReturnToCenter;
    i = i || function(t) {};
    let S = document.getElementById(t);
    S.style.touchAction = "none";
    let f = document.createElement("canvas");
    f.id = o, 0 === n && (n = S.clientWidth), 0 === a && (a = S.clientHeight), f.width = n, f.height = a, S.appendChild(f);
    let l = f.getContext("2d"),
        k = 0,
        g = 2 * Math.PI,
        x = (f.width - (f.width / 2 + 10)) / 2,
        v = x + 5,
        P = x + 30,
        m = f.width / 2,
        C = f.height / 2,
        p = f.width / 10,
        y = -1 * p,
        w = f.height / 10,
        L = -1 * w,
        F = m,
        E = C;

    function W() {
        l.beginPath(), l.arc(m, C, P, 0, g, !1), l.lineWidth = d, l.strokeStyle = u, l.stroke()
    }

    function T() {
        l.beginPath(), F < x && (F = v), F + x > f.width && (F = f.width - v), E < x && (E = v), E + x > f.height && (E = f.height - v), l.arc(F, E, x, 0, g, !1);
        let t = l.createRadialGradient(m, C, 5, m, C, 200);
        t.addColorStop(0, r), t.addColorStop(1, s), l.fillStyle = t, l.fill(), l.lineWidth = c, l.strokeStyle = s, l.stroke()
    }

    function D() {
        let t = "",
            e = F - m,
            o = E - C;
        return o >= L && o <= w && (t = "C"), o < L && (t = "N"), o > w && (t = "S"), e < y && ("C" === t ? t = "W" : t += "W"), e > p && ("C" === t ? t = "E" : t += "E"), t
    }
    "ontouchstart" in document.documentElement ? (f.addEventListener("touchstart", function(t) {
        k = 1
    }, !1), f.addEventListener("touchmove", function(t) {
        t.preventDefault();
        k && (F = t.targetTouches[0].pageX - f.offsetLeft, E = t.targetTouches[0].pageY - f.offsetTop)
    }, { passive: false }), f.addEventListener("touchend", function(t) {
        k = 0, h && (F = m, E = C)
    }, !1)) : (f.addEventListener("mousedown", function(t) {
        k = 1
    }, !1), document.addEventListener("mousemove", function(t) {
        k && (F = t.pageX - f.offsetLeft, E = t.pageY - f.offsetTop)
    }, !1), document.addEventListener("mouseup", function(t) {
        k = 0, h && (F = m, E = C)
    }, !1));
    setInterval(function() {
        l.clearRect(0, 0, f.width, f.height), W(), T();
        let t = {
            xPosition: F,
            yPosition: E,
            x: ((F - m) / x * 100).toFixed(),
            y: ((E - C) / x * 100).toFixed(),
            cardinalDirection: D()
        };
        i(t)
    }, 10);
    return {
        GetX: () => ((F - m) / x * 100).toFixed(),
        GetY: () => ((E - C) / x * 100).toFixed(),
        GetDir: () => D()
    }
};
