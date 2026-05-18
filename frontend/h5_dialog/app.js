(function() {
  const chatArea = document.getElementById('chat-area');
  const stateSummary = document.getElementById('state-summary');
  const actorKnowledge = document.getElementById('actor-knowledge');
  const recipientKnowledge = document.getElementById('recipient-knowledge');
  const debugPanel = document.getElementById('debug-panel');
  const quickActions = document.getElementById('quick-actions');
  const inputForm = document.getElementById('input-form');
  const inputText = document.getElementById('input-text');
  const turnInfo = document.getElementById('turn-info');

  let sessionId = localStorage.getItem('pf_session_id') || generateSessionId();
  localStorage.setItem('pf_session_id', sessionId);
  let clientTurn = 0;

  function generateSessionId() {
    return 's_' + Math.random().toString(36).slice(2) + '_' + Date.now();
  }

  function appendMessage(text, cls) {
    const div = document.createElement('div');
    div.className = 'message ' + cls;
    div.textContent = text;
    chatArea.appendChild(div);
    chatArea.scrollTop = chatArea.scrollHeight;
  }

  function setStateSummary(lines) {
    stateSummary.textContent = lines.join(' | ');
  }

  function setKnowledge(actorLines, recipLines) {
    actorKnowledge.replaceChildren();
    recipientKnowledge.replaceChildren();
    actorLines.slice(0, 5).forEach(l => {
      const d = document.createElement('div');
      d.className = 'knowledge-line';
      d.textContent = l;
      actorKnowledge.appendChild(d);
    });
    recipLines.slice(0, 5).forEach(l => {
      const d = document.createElement('div');
      d.className = 'knowledge-line';
      d.textContent = l;
      recipientKnowledge.appendChild(d);
    });
  }

  function setDebug(keys) {
    debugPanel.textContent = keys.length ? '调试: ' + keys.join(', ') : '';
  }

  function setQuickActions(actions) {
    quickActions.replaceChildren();
    actions.forEach(a => {
      const btn = document.createElement('button');
      btn.textContent = a.label_text;
      btn.disabled = !a.enabled;
      btn.addEventListener('click', () => {
        sendTurn(a.input_text);
      });
      quickActions.appendChild(btn);
    });
  }

  function updateTurnInfo() {
    turnInfo.textContent = '回合 ' + clientTurn;
  }

  async function sendTurn(text) {
    if (!text) return;
    appendMessage(text, 'player');
    inputText.value = '';
    clientTurn++;
    updateTurnInfo();

    try {
      const body = 'session_id=' + encodeURIComponent(sessionId) +
                   '&input_text=' + encodeURIComponent(text) +
                   '&client_turn=' + clientTurn;
      const resp = await fetch('/api/dialog', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body
      });
      if (!resp.ok) {
        appendMessage('网络错误 (' + resp.status + ')', 'warning');
        return;
      }
      const data = await resp.json();
      if (data.reply_text) {
        appendMessage(data.reply_text, 'system');
      }
      setStateSummary(data.state_summary_lines || []);
      setKnowledge(data.actor_knowledge_lines || [], data.recipient_knowledge_lines || []);
      setDebug(data.debug_keys || []);
      setQuickActions(data.quick_actions || []);
    } catch (e) {
      appendMessage('请求失败: ' + e.message, 'warning');
    }
  }

  inputForm.addEventListener('submit', function(e) {
    e.preventDefault();
    const text = inputText.value.trim();
    if (text) sendTurn(text);
  });

  // Initial observe
  sendTurn('观察');
})();
