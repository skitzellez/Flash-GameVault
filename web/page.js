// Copyright (c) 2026 Skitzelles - part of Game Vault, https://github.com/skitzellez/xbox-game-vault
// PolyForm Noncommercial License 1.0.0: not for sale, and this notice stays in every copy.
//
// Game Vault · Flash page: loads https://flash.gamevault/game.swf in Ruffle and plays it with the controller.
// The app turns the controller into real key and mouse events. Flash games were
// made for a keyboard and mouse, so: D-pad / left stick = arrow keys, A = Space, B = X, X = Z, Y = C, Menu = Enter,
// LB = Shift, RB = Ctrl; right stick moves a mouse pointer, RT (or LT) clicks.

"use strict";

const host = window.chrome && window.chrome.webview;
function log(text) {
    try { host.postMessage(String(text)); } catch (e) { }
}
for (const level of ["log", "info", "warn", "error"]) {
    const original = console[level].bind(console);
    console[level] = (...a) => { original(...a); log(level + ": " + a.map(String).join(" ")); };
}
window.addEventListener("error", e => log("error: " + e.message));
window.addEventListener("unhandledrejection", e => log("rejected: " + e.reason));

const ruffle = window.RufflePlayer.newest();
const player = ruffle.createPlayer();
document.getElementById("stage").appendChild(player);
const api = player.ruffle ? player.ruffle() : player;
player.addEventListener("loadedmetadata", () => {
    const m = api.metadata || {};
    log("swf: version " + m.swfVersion + (m.isActionScript3 ? " AS3" : " AS1/2") + ", " + m.width + "x" +
        m.height + ", " + m.frameRate + " fps");
});
player.addEventListener("loadeddata", () => log("swf: running"));
api.load({ url: "https://flash.gamevault/game.swf" })
    .then(() => { log("ruffle: loading"); player.focus(); })
    .catch(e => log("ruffle: could not load: " + e));

// --- controller ---
// The app sends the controller as real key and mouse events (DevTools Input); here only the pointer the right stick
// moves is drawn, from {"cx", "cy"}.
const cursor = document.getElementById("cursor");
let shownAt = 0;
host && host.addEventListener("message", e => {
    const p = e.data;
    if (typeof p.cx !== "number") return;
    cursor.style.display = "block";
    cursor.style.transform = "translate(" + (p.cx - 3) + "px," + (p.cy - 2) + "px)";
    shownAt = performance.now();
});

setInterval(() => {
    if (cursor.style.display === "block" && performance.now() - shownAt > 4000) cursor.style.display = "none";
}, 500);
