(function() {
  const $ = (id) => document.getElementById(id);

  const sceneTitle = $('scene-title');
  const sceneSubtitle = $('scene-subtitle');
  const sceneSummary = $('scene-summary');
  const feedbackLog = $('feedback-log');
  const objectList = $('object-list');
  const actorKnowledge = $('actor-knowledge');
  const recipientKnowledge = $('recipient-knowledge');
  const hintList = $('hint-list');
  const actionBar = $('action-bar');
  const commandForm = $('command-form');
  const commandInput = $('command-input');
  const resetButton = $('reset-button');
  const turnInfo = $('turn-info');

  let sessionId = localStorage.getItem('pf_playable_session_id') || createSessionId();
  let clientTurn = 0;
  let pending = false;
  let activeBusyKey = '';
  localStorage.setItem('pf_playable_session_id', sessionId);

  function createSessionId() {
    return 's_' + Math.random().toString(36).slice(2) + '_' + Date.now();
  }

  function textOf(safeText, fallback) {
    if (!safeText) return fallback || '';
    return safeText.zh_cn || fallback || '';
  }

  function clear(node) {
    node.replaceChildren();
  }

  function div(className, text) {
    const node = document.createElement('div');
    node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
  }

  function appendFeedback(text, tone) {
    const item = div('feedback-item tone-' + (tone || 'neutral'), text);
    feedbackLog.appendChild(item);
    while (feedbackLog.children.length > 20) feedbackLog.removeChild(feedbackLog.firstChild);
    feedbackLog.scrollTop = feedbackLog.scrollHeight;
  }

  async function postForm(url, fields) {
    const body = new URLSearchParams(fields);
    const response = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });
    if (!response.ok) throw new Error('HTTP ' + response.status);
    return response.json();
  }

  async function bootstrap(reset) {
    if (pending) return;
    pending = true;
    activeBusyKey = reset ? 'reset' : 'bootstrap';
    setBusy(true);
    try {
      const data = await postForm('/api/playable/bootstrap', {
        session_id: sessionId,
        reset: reset ? 'true' : 'false',
        language: 'zh_cn'
      });
      if (reset) {
        clientTurn = 0;
        clear(feedbackLog);
      }
      renderResponse(data);
    } catch (error) {
      appendFeedback('连接后端失败，请确认服务是否运行在 1999 端口。', 'warning');
    } finally {
      pending = false;
      activeBusyKey = '';
      setBusy(false);
    }
  }

  async function sendTurn(inputText, action) {
    if (pending) return;
    const command = (inputText || '').trim();
    if (!command && !action) return;
    pending = true;
    activeBusyKey = action ? (action.action_key || action.input_text || command) : 'command';
    setBusy(true);
    clientTurn += 1;
    try {
      const fields = {
        session_id: sessionId,
        client_turn: String(clientTurn),
        input_kind: action ? 'projection_action' : 'free_text',
        input_text: command
      };
      if (action) {
        fields.action_key = action.action_key || '';
        if (action.target_object_refs && action.target_object_refs.length > 0) {
          fields.target_object_ref = action.target_object_refs[0];
        }
      }
      const data = await postForm('/api/playable/turn', fields);
      renderResponse(data);
    } catch (error) {
      appendFeedback('请求失败：' + error.message, 'warning');
    } finally {
      pending = false;
      activeBusyKey = '';
      setBusy(false);
      commandInput.value = '';
    }
  }

  function setBusy(value) {
    commandInput.disabled = value;
    resetButton.disabled = value && activeBusyKey === 'reset';
    resetButton.classList.toggle('is-busy', value && activeBusyKey === 'reset');
    [actionBar, objectList].forEach((container) => {
      container.querySelectorAll('button').forEach((button) => {
        const isActive = value && button.dataset.busyKey === activeBusyKey;
        button.disabled = button.dataset.enabled !== 'true' || isActive;
        button.classList.toggle('is-busy', isActive);
      });
    });
  }

  function renderResponse(response) {
    if (!response || !response.projection) {
      appendFeedback('后端没有返回可显示的游戏内容。', 'warning');
      return;
    }
    turnInfo.textContent = '回合 ' + (response.server_turn || 0);
    appendFeedback(textOf(response.reply_text, '世界安静地等待你的下一步。'), response.tone || 'neutral');
    renderProjection(response.projection);
  }

  function renderProjection(projection) {
    sceneTitle.textContent = textOf(projection.scene_title, '未知地点');
    sceneSubtitle.textContent = '初醒探索 / 经验学习';
    renderTextList(sceneSummary, projection.scene_summary, '暂无场景信息。');
    renderObjects(projection.object_cards || []);
    renderActions(projection.action_bar || []);
    renderKnowledge(actorKnowledge, projection.actor_knowledge || [], '你还没有形成稳定知识。');
    renderKnowledge(recipientKnowledge, projection.recipient_knowledge || [], '同伴还不知道你学到了什么。');
    renderHints(projection);
  }

  function renderTextList(container, list, emptyText) {
    clear(container);
    if (!list || list.length === 0) {
      container.appendChild(div('empty', emptyText));
      return;
    }
    list.forEach((item) => container.appendChild(div('text-line', textOf(item))));
  }

  function renderObjects(cards) {
    clear(objectList);
    if (cards.length === 0) {
      objectList.appendChild(div('empty', '这里暂时没有你能尝试的物体。'));
      return;
    }
    cards.forEach((card) => {
      const node = div('object-card');
      node.dataset.key = card.object_ref_key || '';
      node.appendChild(div('object-name', textOf(card.display_name, '未知物体')));
      node.appendChild(div('object-desc', textOf(card.description, '你还不了解它。')));
      const tags = readableTags(card.safe_tags || []);
      if (tags.length > 0) {
        const tagRow = div('tag-row');
        tagRow.appendChild(div('tag-title', '你现在只能分辨这些线索：')); 
        tags.forEach((label) => tagRow.appendChild(div('tag-chip', label)));
        node.appendChild(tagRow);
      }
      const badges = div('badge-row');
      (card.knowledge_badges || []).forEach((badge) => badges.appendChild(div('badge', textOf(badge.label))));
      if (badges.children.length > 0) node.appendChild(badges);
      const actions = div('card-actions');
      (card.actions || []).forEach((action) => actions.appendChild(actionButton(action)));
      if (actions.children.length > 0) node.appendChild(actions);
      objectList.appendChild(node);
    });
  }


  function readableTags(tags) {
    const labels = [];
    const map = {
      fruit: '果实类对象',
      leaf: '叶类对象',
      tool: '可尝试使用',
      stone: '石质材料',
      axe: '斧类工具',
      material: '材料类对象',
      wood: '木质材料',
      maintenance: '可用于维护',
      sharp: '当前状态：锋利',
      dull: '当前状态：变钝',
      decayed: '当前状态：腐坏',
      fresh: '当前状态：新鲜',
      fuel: '可作为燃料',
      dry: '干燥易燃',
      ignition: '可点火',
      fire: '火源',
      light_source: '光源',
      generated_item: '可制作/生成',
      depleted: '已耗尽',
      threat: '危险征兆',
      creature: '生物',
      dormant: '暂未靠近',
      foreshadowing: '正在观察',
      approaching: '正在靠近',
      confronting: '正在对峙',
      avoiding_fire: '正在避火',
      resolved: '暂时退去',
      agent_wildlife: '野生 Agent'
    };
    tags.forEach((tag) => {
      if (tag === 'red' || tag === 'green') return;
      if (map[tag]) labels.push(map[tag]);
    });
    return labels;
  }

  function renderActions(actions) {
    clear(actionBar);
    actions.forEach((action) => actionBar.appendChild(actionButton(action)));
  }

  function actionButton(action) {
    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = textOf(action.label, '行动');
    button.dataset.enabled = action.enabled ? 'true' : 'false';
    button.dataset.busyKey = action.action_key || action.input_text || textOf(action.label, '行动');
    const isActive = pending && button.dataset.busyKey === activeBusyKey;
    button.disabled = !action.enabled || isActive;
    button.classList.toggle('is-busy', isActive);
    button.addEventListener('click', () => sendTurn(action.input_text || '', action));
    return button;
  }

  function renderKnowledge(container, lines, emptyText) {
    clear(container);
    if (lines.length === 0) {
      container.appendChild(div('empty', emptyText));
      return;
    }
    lines.forEach((line) => {
      const node = div('knowledge-line');
      const subject = textOf(line.subject_label);
      const predicate = textOf(line.predicate_label);
      const effect = textOf(line.effect_summary);
      node.appendChild(div('knowledge-main', [subject, predicate, effect].filter(Boolean).join(' → ')));
      if (predicate.indexOf('对') === 0) {
        node.appendChild(div('knowledge-target', '目标知识：这条经验只适用于“' + predicate + '”这个目标关系。'));
      }
      const confidence = line.confidence_label ? textOf(line.confidence_label) : '';
      if (confidence) node.appendChild(div('knowledge-sub', confidence));
      const reason = causalReasonFromKeys(line.reason_keys || []);
      if (reason) node.appendChild(div('knowledge-causal', reason));
      container.appendChild(node);
    });
  }

  function causalReasonFromKeys(keys) {
    if (keys.includes('causal_ambiguous_dose_window')) {
      return '还不能下结论：近期连续尝试过相似东西，结果可能不是单一原因；等一会儿再单独试会更清楚。';
    }
    if (keys.includes('causal_confirmation_capped_by_alternative_explanation')) {
      return '还不能下结论：还有别的可能原因没排除，先当作经验记录。';
    }
    return '';
  }

  function renderHints(projection) {
    clear(hintList);
    const hints = [];
    (projection.condition_hints || []).forEach((item) => hints.push({ tone: 'warning', text: textOf(item.summary_text) }));
    (projection.danger_hints || []).forEach((item) => {
      hints.push({ tone: 'danger', text: textOf(item.hint_text) });
      (item.countermeasure_labels || []).forEach((label) => hints.push({ tone: 'neutral', text: textOf(label) }));
    });
    (projection.issues || []).forEach((item) => hints.push({ tone: item.severity_key || 'system', text: textOf(item.safe_summary) }));
    if (hints.length === 0) {
      hintList.appendChild(div('empty', '暂时没有额外提示。'));
      return;
    }
    hints.forEach((item) => hintList.appendChild(div('hint-item tone-' + item.tone, item.text)));
  }

  commandForm.addEventListener('submit', (event) => {
    event.preventDefault();
    sendTurn(commandInput.value, null);
  });

  resetButton.addEventListener('click', () => bootstrap(true));

  bootstrap(false);
})();
