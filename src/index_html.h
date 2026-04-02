#pragma once

// The chat UI is embedded as a raw string literal so main.cpp stays readable.
// Everything (HTML, CSS, JS) is inline — no external dependencies.
static const char INDEX_HTML[] = R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>PlaneTalk</title>
<style>
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: #1a1a1e;
    color: #e0e0e0;
    font-family: system-ui, -apple-system, sans-serif;
    font-size: 16px;
    height: 100dvh;
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  #topbar {
    background: #26262c;
    border-bottom: 1px solid #3a3a44;
    padding: 10px 16px;
    display: flex;
    align-items: center;
    gap: 8px;
    flex-shrink: 0;
  }

  #topbar .label {
    color: #888;
    font-size: 13px;
    white-space: nowrap;
  }

  #nick-display {
    font-weight: 600;
    color: #a78bfa;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  #nick-edit-btn {
    background: none;
    border: none;
    cursor: pointer;
    color: #888;
    padding: 4px;
    border-radius: 4px;
    line-height: 1;
    font-size: 16px;
    flex-shrink: 0;
  }
  #nick-edit-btn:hover { color: #a78bfa; background: #3a3a44; }

  #nick-form {
    display: none;
    flex: 1;
    gap: 6px;
  }
  #nick-form.visible { display: flex; }

  #nick-input {
    flex: 1;
    background: #1a1a1e;
    border: 1px solid #3a3a44;
    border-radius: 6px;
    color: #e0e0e0;
    padding: 4px 8px;
    font-size: 15px;
    min-width: 0;
  }
  #nick-input:focus { outline: none; border-color: #a78bfa; }

  #nick-save-btn {
    background: #a78bfa;
    border: none;
    border-radius: 6px;
    color: #1a1a1e;
    font-weight: 700;
    padding: 4px 12px;
    cursor: pointer;
    font-size: 14px;
    flex-shrink: 0;
  }
  #nick-save-btn:hover { background: #c4b5fd; }

  #nick-cancel-btn {
    background: none;
    border: none;
    color: #888;
    cursor: pointer;
    padding: 4px 8px;
    font-size: 14px;
    border-radius: 6px;
    flex-shrink: 0;
  }
  #nick-cancel-btn:hover { color: #e0e0e0; background: #3a3a44; }

  #messages {
    flex: 1;
    overflow-y: auto;
    padding: 12px 16px;
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .msg {
    line-height: 1.4;
    word-break: break-word;
  }

  .msg .nick {
    font-weight: 600;
    color: #a78bfa;
    margin-right: 4px;
  }

  .msg .text { color: #e0e0e0; }

  .msg .ts {
    font-size: 11px;
    color: #555;
    margin-left: 6px;
    white-space: nowrap;
  }

  .msg.system {
    font-style: italic;
    color: #666;
    font-size: 14px;
  }

  .msg.system .nick { color: #555; }
  .msg.system .text { color: #666; }

  .msg.pm .nick { color: #c084fc; }
  .msg.pm .text { color: #d8b4fe; }

  .pm-badge {
    display: inline-block;
    font-size: 10px;
    font-weight: 700;
    background: #4c1d95;
    color: #c084fc;
    border-radius: 3px;
    padding: 1px 4px;
    margin-right: 4px;
    vertical-align: middle;
    letter-spacing: 0.05em;
  }

  /* Clickable nick wrapper — only on non-system messages. */
  .nick-btn {
    background: none;
    border: none;
    padding: 0;
    cursor: pointer;
    font: inherit;
    color: inherit;
  }

  #pm-banner {
    display: none;
    background: #2e1065;
    border-top: 1px solid #4c1d95;
    border-bottom: 1px solid #4c1d95;
    padding: 6px 16px;
    font-size: 13px;
    color: #c084fc;
    align-items: center;
    gap: 8px;
    flex-shrink: 0;
  }
  #pm-banner.visible { display: flex; }
  #pm-banner-text { flex: 1; }
  #pm-close-btn {
    background: none;
    border: none;
    color: #c084fc;
    cursor: pointer;
    font-size: 16px;
    line-height: 1;
    padding: 2px 4px;
    border-radius: 4px;
  }
  #pm-close-btn:hover { background: #4c1d95; }

  #status-bar {
    text-align: center;
    font-size: 12px;
    color: #555;
    padding: 2px 0;
    flex-shrink: 0;
  }
  #status-bar.connected { color: #4ade80; }
  #status-bar.disconnected { color: #f87171; }

  #inputarea {
    background: #26262c;
    border-top: 1px solid #3a3a44;
    padding: 10px 16px;
    display: flex;
    gap: 8px;
    flex-shrink: 0;
  }

  #msg-input {
    flex: 1;
    background: #1a1a1e;
    border: 1px solid #3a3a44;
    border-radius: 8px;
    color: #e0e0e0;
    padding: 10px 14px;
    font-size: 16px;
    min-width: 0;
  }
  #msg-input:focus { outline: none; border-color: #a78bfa; }

  #send-btn {
    background: #a78bfa;
    border: none;
    border-radius: 8px;
    color: #1a1a1e;
    font-weight: 700;
    padding: 10px 18px;
    cursor: pointer;
    font-size: 16px;
    flex-shrink: 0;
    min-width: 64px;
  }
  #send-btn:hover { background: #c4b5fd; }
  #send-btn:disabled { background: #3a3a44; color: #666; cursor: default; }

  #welcome-modal {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.75);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 100;
    padding: 16px;
  }

  #welcome-modal .modal-card {
    background: #26262c;
    border: 1px solid #3a3a44;
    border-radius: 12px;
    padding: 28px 24px;
    max-width: 420px;
    width: 100%;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  #welcome-modal h1 {
    font-size: 20px;
    font-weight: 700;
    color: #a78bfa;
    margin: 0;
  }

  #welcome-modal p {
    font-size: 15px;
    color: #c0c0c0;
    line-height: 1.5;
    margin: 0;
  }

  #welcome-modal .url-row {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  #welcome-modal .url-field {
    flex: 1;
    background: #1a1a1e;
    border: 1px solid #3a3a44;
    border-radius: 6px;
    color: #a78bfa;
    font-family: monospace;
    font-size: 15px;
    padding: 6px 10px;
    min-width: 0;
    cursor: text;
  }
  #welcome-modal .url-field:focus { outline: none; border-color: #a78bfa; }

  #welcome-modal .url-open-link {
    color: #a78bfa;
    font-size: 14px;
    white-space: nowrap;
    text-decoration: underline;
  }

  #welcome-dismiss-btn {
    align-self: flex-end;
    background: #a78bfa;
    border: none;
    border-radius: 8px;
    color: #1a1a1e;
    font-weight: 700;
    font-size: 15px;
    padding: 10px 22px;
    cursor: pointer;
  }
  #welcome-dismiss-btn:hover { background: #c4b5fd; }
</style>
</head>
<body>

<div id="welcome-modal">
  <div class="modal-card">
    <h1>Welcome to PlaneTalk</h1>
    <p>Your device may warn that this WiFi has no internet — that's expected. Keep this network connected to stay in the chat.</p>
    <p>For the best experience, open the chat in your browser:</p>
    <div class="url-row">
      <input class="url-field" type="text" value="192.168.4.1" readonly>
      <a class="url-open-link" href="http://192.168.4.1" target="_blank">Open in browser</a>
    </div>
    <button id="welcome-dismiss-btn">Start chatting</button>
  </div>
</div>

<div id="topbar">
  <span class="label">You are</span>
  <span id="nick-display">...</span>
  <button id="nick-edit-btn" title="Change nickname">&#9998;</button>
  <div id="nick-form">
    <input id="nick-input" type="text" maxlength="20" placeholder="New nickname">
    <button id="nick-save-btn">Save</button>
    <button id="nick-cancel-btn">Cancel</button>
  </div>
</div>

<div id="messages"></div>
<div id="status-bar">Connecting...</div>

<div id="pm-banner">
  <span id="pm-banner-text"></span>
  <button id="pm-close-btn" title="Close PM">&#x2715;</button>
</div>

<div id="inputarea">
  <input id="msg-input" type="text" maxlength="500" placeholder="Type a message..." autocomplete="off" autofocus>
  <button id="send-btn" disabled>Send</button>
</div>

<script>
(function () {
  'use strict';

  const welcomeModal      = document.getElementById('welcome-modal');
  const welcomeDismissBtn = document.getElementById('welcome-dismiss-btn');

  // sessionStorage is cleared when the tab/browser is closed, so the modal
  // reappears on every new session (e.g. reconnecting to the AP) but not on
  // every page navigation within the same session.
  if (sessionStorage.getItem('welcome-dismissed')) {
    welcomeModal.style.display = 'none';
  }

  welcomeDismissBtn.addEventListener('click', function () {
    sessionStorage.setItem('welcome-dismissed', '1');
    welcomeModal.style.display = 'none';
  });

  const messagesEl   = document.getElementById('messages');
  const msgInput     = document.getElementById('msg-input');
  const sendBtn      = document.getElementById('send-btn');
  const statusBar    = document.getElementById('status-bar');
  const nickDisplay  = document.getElementById('nick-display');
  const nickEditBtn  = document.getElementById('nick-edit-btn');
  const nickForm     = document.getElementById('nick-form');
  const nickInput    = document.getElementById('nick-input');
  const nickSaveBtn  = document.getElementById('nick-save-btn');
  const nickCancelBtn = document.getElementById('nick-cancel-btn');
  const pmBanner     = document.getElementById('pm-banner');
  const pmBannerText = document.getElementById('pm-banner-text');
  const pmCloseBtn   = document.getElementById('pm-close-btn');

  let currentNick = '...';
  let ws = null;
  let reconnectTimer = null;
  // null means public chat; a non-empty string is the nick we are PMing.
  let pmTarget = null;

  function setStatus(text, cls) {
    statusBar.textContent = text;
    statusBar.className = cls || '';
  }

  function openPm(nick) {
    pmTarget = nick;
    pmBannerText.textContent = 'Private message to ' + nick;
    pmBanner.classList.add('visible');
    msgInput.focus();
  }

  function closePm() {
    pmTarget = null;
    pmBanner.classList.remove('visible');
  }

  pmCloseBtn.addEventListener('click', closePm);

  // ts is a Date object; absent means no timestamp is shown.
  // isPm flags the message as a private message for distinct styling.
  function appendMessage(nick, text, ts, isSystem, isPm) {
    const div = document.createElement('div');
    div.className = 'msg' + (isSystem ? ' system' : '') + (isPm ? ' pm' : '');

    const nickSpan = document.createElement('span');
    nickSpan.className = 'nick';
    nickSpan.textContent = nick + ':';

    // System nicks and the user's own nick are not clickable.
    if (!isSystem && nick !== currentNick) {
      const btn = document.createElement('button');
      btn.className = 'nick-btn';
      btn.appendChild(nickSpan);
      btn.addEventListener('click', function () { openPm(nick); });
      div.appendChild(btn);
    } else {
      div.appendChild(nickSpan);
    }

    if (isPm) {
      const badge = document.createElement('span');
      badge.className = 'pm-badge';
      badge.textContent = 'PM';
      div.appendChild(badge);
    }

    const textSpan = document.createElement('span');
    textSpan.className = 'text';
    textSpan.textContent = ' ' + text;

    div.appendChild(textSpan);

    if (ts) {
      const tsSpan = document.createElement('span');
      tsSpan.className = 'ts';
      tsSpan.textContent = ts.getHours().toString().padStart(2, '0') + ':' +
                           ts.getMinutes().toString().padStart(2, '0');
      div.appendChild(tsSpan);
    }

    messagesEl.appendChild(div);
    messagesEl.scrollTop = messagesEl.scrollHeight;
  }

  function handleMessage(data) {
    let msg;
    try { msg = JSON.parse(data); } catch (e) { return; }

    if (msg.type === 'message') {
      const isSystem = msg.nick === 'System';
      appendMessage(msg.nick, msg.text, new Date(), isSystem, !!msg.pm);

      // When the user we are PMing disconnects, auto-close PM mode.
      // Leave messages have the form "NickName has left" from the System nick.
      if (isSystem && pmTarget) {
        const leavePattern = pmTarget + ' has left';
        if (msg.text === leavePattern) {
          closePm();
        }
      }

    } else if (msg.type === 'nick') {
      // Broadcast notification only — own nick is updated exclusively via
      // the welcome message the server sends directly to the requesting client,
      // preventing false updates when two users share the same nickname.
      appendMessage('System', msg.old + ' is now known as ' + msg.new, null, true, false);

      // If the user we are PMing changed their nick, follow the rename.
      if (pmTarget && msg.old === pmTarget) {
        openPm(msg.new);
      }

    } else if (msg.type === 'history') {
      // Render each buffered message in order. History never contains PMs
      // because PMs are not stored in the message buffer.
      (msg.messages || []).forEach(function (raw) {
        let m;
        try { m = JSON.parse(raw); } catch (e) { return; }
        if (m.type === 'message') {
          appendMessage(m.nick, m.text, null, m.nick === 'System', false);
        }
      });

    } else if (msg.type === 'welcome') {
      // Server sends our assigned nickname on connect.
      currentNick = msg.nick;
      nickDisplay.textContent = currentNick;

    } else if (msg.type === 'error') {
      appendMessage('System', 'Error: ' + msg.text, null, true, false);
    }
  }

  function connect() {
    const url = 'ws://' + location.host + '/ws';
    ws = new WebSocket(url);

    ws.onopen = function () {
      setStatus('Connected', 'connected');
      sendBtn.disabled = false;
      if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null; }
    };

    ws.onmessage = function (event) {
      handleMessage(event.data);
    };

    ws.onclose = function () {
      setStatus('Disconnected — reconnecting...', 'disconnected');
      sendBtn.disabled = true;
      ws = null;
      reconnectTimer = setTimeout(connect, 2000);
    };

    ws.onerror = function () {
      // onclose fires right after onerror, so reconnect is handled there.
      ws.close();
    };
  }

  function sendMessage() {
    const text = msgInput.value.trim();
    if (!text || !ws || ws.readyState !== WebSocket.OPEN) return;
    const payload = { type: 'message', text: text };
    if (pmTarget) {
      payload.to = pmTarget;
    }
    ws.send(JSON.stringify(payload));
    msgInput.value = '';
    msgInput.focus();
  }

  sendBtn.addEventListener('click', sendMessage);
  msgInput.addEventListener('keydown', function (e) {
    if (e.key === 'Enter') sendMessage();
  });

  // Nickname editing.
  nickEditBtn.addEventListener('click', function () {
    nickDisplay.style.display = 'none';
    nickEditBtn.style.display = 'none';
    nickForm.classList.add('visible');
    nickInput.value = currentNick;
    nickInput.focus();
    nickInput.select();
  });

  function closeNickForm() {
    nickForm.classList.remove('visible');
    nickDisplay.style.display = '';
    nickEditBtn.style.display = '';
  }

  nickCancelBtn.addEventListener('click', closeNickForm);

  function saveNick() {
    const newNick = nickInput.value.trim();
    if (!newNick || !ws || ws.readyState !== WebSocket.OPEN) { closeNickForm(); return; }
    ws.send(JSON.stringify({ type: 'nick', nick: newNick }));
    closeNickForm();
  }

  nickSaveBtn.addEventListener('click', saveNick);
  nickInput.addEventListener('keydown', function (e) {
    if (e.key === 'Enter') saveNick();
    if (e.key === 'Escape') closeNickForm();
  });

  connect();

  // On mobile, opening the soft keyboard fires a visualViewport resize event
  // but does NOT shrink window.innerHeight reliably across browsers. Setting
  // body height explicitly to visualViewport.height keeps the flex layout
  // contained to the visible area so the input bar stays above the keyboard.
  if (window.visualViewport) {
    window.visualViewport.addEventListener('resize', function () {
      const nearBottom = messagesEl.scrollHeight - messagesEl.scrollTop - messagesEl.clientHeight < 50;
      document.body.style.height = window.visualViewport.height + 'px';
      if (nearBottom) { messagesEl.scrollTop = messagesEl.scrollHeight; }
    });
  }
}());
</script>
</body>
</html>
)rawhtml";
