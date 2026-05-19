(function() {
  "use strict";

  // ============================================================
  // DOM References
  // ============================================================
  const $ = (id) => document.getElementById(id);

  const dom = {
    phaseLabel: $("phase-label"),
    dangerLabel: $("danger-label"),
    goalLabel: $("goal-label"),
    turnBadge: $("turn-badge"),
    objectGrid: $("object-grid"),
    situationText: $("situation-text"),
    recentEvents: $("recent-events"),
    objectDetail: $("object-detail"),
    actionList: $("action-list"),
    companionContent: $("companion-content"),
    executionContent: $("execution-content"),
    logDrawer: $("log-drawer"),
    drawerEvents: $("drawer-events"),
    drawerKnowledge: $("drawer-knowledge"),
    drawerSystem: $("drawer-system"),
    toast: $("toast"),
    btnWait: $("btn-wait"),
    btnTeach: $("btn-teach"),
    btnKnowledge: $("btn-knowledge"),
    btnLog: $("btn-log"),
    btnReset: $("btn-reset"),
    drawerClose: $("drawer-close"),
  };

  // ============================================================
  // UI State (only UI state, no rule state)
  // ============================================================
  const uiState = {
    sessionId: localStorage.getItem("pf_playable_session_id") || newSessionId(),
    projection: null,
    replyText: null,
    serverTurn: 0,
    tone: "neutral",
    selectedObjectKey: null,
    logDrawerOpen: false,
    drawerTab: "events",
    isBusy: false,
    activeBusyKey: "",
    lastError: null,
  };

  const eventLog = [];       // { turn, text, tone, ts }
  const knowledgeLog = [];   // { kind, lines } — snapshots from projection

  localStorage.setItem("pf_playable_session_id", uiState.sessionId);

  function newSessionId() {
    return "s_" + Math.random().toString(36).slice(2) + "_" + Date.now();
  }

  // ============================================================
  // Helpers
  // ============================================================
  function textOf(safeText, fallback) {
    if (!safeText) return fallback || "";
    return safeText.zh_cn || fallback || "";
  }

  function el(tag, className, text) {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
  }

  function clear(node) {
    while (node.firstChild) node.removeChild(node.firstChild);
  }

  function showToast(msg, duration) {
    dom.toast.textContent = msg;
    dom.toast.classList.remove("hidden");
    if (duration) {
      setTimeout(function() {
        dom.toast.classList.add("hidden");
      }, duration);
    }
  }

  // ============================================================
  // API Calls
  // ============================================================
  async function postForm(url, fields) {
    var body = new URLSearchParams();
    for (var k in fields) {
      if (fields.hasOwnProperty(k)) body.append(k, fields[k]);
    }
    var resp = await fetch(url, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body,
    });
    if (!resp.ok) throw new Error("HTTP " + resp.status);
    return resp.json();
  }

  async function bootstrap(reset) {
    if (uiState.isBusy) return;
    uiState.isBusy = true;
    uiState.activeBusyKey = reset ? "reset" : "bootstrap";
    setBusyUI(true);
    try {
      var data = await postForm("/api/playable/bootstrap", {
        session_id: uiState.sessionId,
        reset: reset ? "true" : "false",
        language: "zh_cn",
      });
      if (reset) {
        eventLog.length = 0;
        knowledgeLog.length = 0;
        uiState.selectedObjectKey = null;
      }
      processResponse(data);
    } catch (e) {
      uiState.lastError = "连接后端失败，请确认服务运行在 1999 端口。";
      showToast(uiState.lastError, 3000);
      addEvent("系统", uiState.lastError, "system");
    } finally {
      uiState.isBusy = false;
      uiState.activeBusyKey = "";
      setBusyUI(false);
    }
  }

  async function sendTurn(inputText, action) {
    if (uiState.isBusy) return;
    var cmd = (inputText || "").trim();
    if (!cmd && !action) return;

    uiState.isBusy = true;
    uiState.activeBusyKey = action
      ? action.actionKey || action.inputText || "action"
      : "command";
    setBusyUI(true);

    try {
      var fields = {
        session_id: uiState.sessionId,
        client_turn: String(uiState.serverTurn + 1),
        input_kind: action ? "projection_action" : "free_text",
        input_text: cmd,
      };
      if (action) {
        fields.action_key = action.actionKey || "";
        if (action.targetRefs && action.targetRefs.length > 0) {
          fields.target_object_ref = action.targetRefs[0];
        }
      }
      var data = await postForm("/api/playable/turn", fields);
      processResponse(data);
    } catch (e) {
      uiState.lastError = "请求失败：" + e.message;
      showToast(uiState.lastError, 3000);
      addEvent("系统", uiState.lastError, "warning");
    } finally {
      uiState.isBusy = false;
      uiState.activeBusyKey = "";
      setBusyUI(false);
    }
  }

  function setBusyUI(busy) {
    dom.btnReset.disabled = busy || uiState.activeBusyKey === "reset";
    dom.btnReset.classList.toggle("is-busy", busy && uiState.activeBusyKey === "reset");
    dom.btnWait.disabled = busy;
    dom.btnTeach.disabled = busy;

    var allBtns = dom.actionList.querySelectorAll("button");
    for (var i = 0; i < allBtns.length; i++) {
      var b = allBtns[i];
      var isActive = busy && b.dataset.busyKey === uiState.activeBusyKey;
      b.disabled = busy;
      b.classList.toggle("is-busy", isActive);
    }
  }

  // ============================================================
  // Event Log
  // ============================================================
  function addEvent(turnLabel, text, tone) {
    eventLog.push({
      turn: turnLabel,
      text: text,
      tone: tone || "neutral",
      ts: Date.now(),
    });
    if (eventLog.length > 200) eventLog.shift();
  }

  // ============================================================
  // Normalization Layer
  // All projection data flows through here.
  // Frontend never infers rules — it only restructures backend data.
  // ============================================================

  function normalizeProjection(raw) {
    if (!raw) return null;
    return {
      sceneTitle: textOf(raw.scene_title, "未知地点"),
      sceneSummary: (raw.scene_summary || []).map(function(s) { return textOf(s); }),
      objectCards: normalizeObjects(raw.object_cards || []),
      actionBar: normalizeActions(raw.action_bar || []),
      actorKnowledge: raw.actor_knowledge || [],
      recipientKnowledge: raw.recipient_knowledge || [],
      cognitionLines: raw.cognition_lines || [],
      conditionHints: raw.condition_hints || [],
      dangerHints: raw.danger_hints || [],
      issues: raw.issues || [],
    };
  }

  function normalizeObjects(cards) {
    return cards.map(function(card) {
      var tags = card.safe_tags || [];
      return {
        key: card.object_ref_key || "",
        name: textOf(card.display_name, "未知"),
        desc: textOf(card.description, ""),
        tags: tags,
        tagLabels: readableTags(tags),
        badges: (card.knowledge_badges || []).map(function(b) {
          return { key: b.badge_key, label: textOf(b.label), tone: b.tone_key };
        }),
        actions: normalizeActions(card.actions || []),
        primaryTag: classifyObjectTag(tags),
      };
    });
  }

  function normalizeActions(actions) {
    return actions.map(function(a) {
      return {
        actionKey: a.action_key || "",
        affordance: a.affordance || "Unknown",
        label: textOf(a.label, "行动"),
        inputText: a.input_text || "",
        enabled: a.enabled !== false,
        disabledReason: a.disabled_reason
          ? textOf(a.disabled_reason.summary_text)
          : "",
        targetRefs: a.target_object_refs || [],
      };
    });
  }


  function knowledgeSummary(k) {
    if (!k) return "";
    var parts = [
      textOf(k.subject_label),
      textOf(k.predicate_label),
      textOf(k.effect_summary),
    ].filter(Boolean);
    var status = textOf(k.confidence_label);
    var summary = parts.join(" → ");
    return status ? summary + "（" + status + "）" : summary;
  }

  function normalizeReasoning(response) {
    // Transitional: extract P39-like info from available fields.
    // When backend adds dedicated companion_reasoning, replace this.
    var reply = textOf(response.reply_text, "");
    var cognition = (response.projection && response.projection.cognition_lines) || [];
    var recipientK = (response.projection && response.projection.recipient_knowledge) || [];

    var goal = "";
    var nextStep = "";
    var reason = "";
    var blocked = "";

    // Try cognition lines first (closest to agent reasoning)
    if (cognition.length > 0) {
      var c = cognition[0];
      goal = textOf(c.aspect_label) || textOf(c.target_label) || "";
      nextStep = textOf(c.claim_summary) || "";
    }

    // Fall back to reply_text analysis
    if (!goal && reply) {
      var goalMatch = reply.match(
        /(?:同伴|Agent|agent).{0,10}(?:目标|打算|想要|正在|判断)[：:\s]*([^。\n]+)/
      );
      if (goalMatch) goal = goalMatch[1].trim();

      var stepMatch = reply.match(
        /(?:下一步|准备|计划|将|会).{0,8}([^。\n]{4,30})/
      );
      if (stepMatch) nextStep = nextStep || stepMatch[1].trim();
    }

    // Check recipient knowledge for teach-related info
    var companionKnowledgeLines = [];
    if (recipientK.length > 0) {
      var lastK = recipientK[recipientK.length - 1];
      if (!goal) {
        goal = "同伴已掌握知识";
      }
      for (var i = 0; i < recipientK.length; i++) {
        companionKnowledgeLines.push(knowledgeSummary(recipientK[i]));
      }
      if (!nextStep) {
        nextStep = "危险靠近时，会按已掌握知识尝试行动。";
      }
    }

    return {
      visible: !!(goal || nextStep || cognition.length > 0 || companionKnowledgeLines.length > 0),
      goal: goal,
      nextStep: nextStep,
      reason: reason,
      blocked: blocked,
      knowledgeLines: companionKnowledgeLines,
    };
  }

  function normalizeExecution(_response) {
    // Placeholder for P40 execution status.
    // When backend adds execution_status field, fill from it.
    return {
      visible: false,
      goal: "",
      step: "",
      status: "",
      interrupt: "",
      blocked: "",
    };
  }

  // ---- Tag helpers (display only, no rule inference) ----
  var TAG_LABELS = {
    fruit: "果实", leaf: "叶片", tool: "工具", stone: "石质",
    axe: "斧", material: "材料", wood: "木质", maintenance: "可维护",
    sharp: "锋利", dull: "钝", decayed: "腐坏", fresh: "新鲜",
    fuel: "燃料", dry: "干燥", ignition: "可点火", fire: "火源",
    light_source: "光源", generated_item: "可制作", depleted: "已耗尽",
    threat: "威胁", creature: "生物", dormant: "潜伏", foreshadowing: "预兆",
    approaching: "接近中", confronting: "对峙中", avoiding_fire: "避火",
    resolved: "已退去", agent_wildlife: "野生Agent",
  };

  var TAG_CLASS = {
    threat: "danger", approaching: "danger", confronting: "danger",
    foreshadowing: "warning", dormant: "warning",
    tool: "tool", axe: "tool", maintenance: "tool",
    material: "material", wood: "material", stone: "material",
    fuel: "material", dry: "material",
    fire: "danger", light_source: "tool",
  };

  function readableTags(tags) {
    var out = [];
    for (var i = 0; i < tags.length; i++) {
      var t = tags[i];
      if (t === "red" || t === "green") continue;
      if (TAG_LABELS[t]) out.push(TAG_LABELS[t]);
    }
    return out;
  }

  function classifyObjectTag(tags) {
    for (var i = 0; i < tags.length; i++) {
      if (TAG_CLASS[tags[i]]) return TAG_CLASS[tags[i]];
    }
    return "";
  }

  // ============================================================
  // Process Response
  // ============================================================
  function processResponse(data) {
    if (!data || !data.projection) {
      uiState.lastError = "后端未返回可显示的游戏内容。";
      showToast(uiState.lastError, 3000);
      addEvent("系统", uiState.lastError, "warning");
      return;
    }

    uiState.serverTurn = data.server_turn || 0;
    uiState.tone = data.tone || "neutral";
    uiState.projection = normalizeProjection(data.projection);
    uiState.replyText = textOf(data.reply_text, "");

    // Record event
    if (uiState.replyText) {
      addEvent("回合 " + uiState.serverTurn, uiState.replyText, uiState.tone);
    }

    // Record knowledge snapshot
    var proj = data.projection;
    if (proj.actor_knowledge && proj.actor_knowledge.length > 0) {
      knowledgeLog.push({
        kind: "actor",
        turn: uiState.serverTurn,
        lines: proj.actor_knowledge,
      });
    }
    if (proj.recipient_knowledge && proj.recipient_knowledge.length > 0) {
      knowledgeLog.push({
        kind: "recipient",
        turn: uiState.serverTurn,
        lines: proj.recipient_knowledge,
      });
    }
    if (knowledgeLog.length > 50) knowledgeLog.splice(0, knowledgeLog.length - 50);

    renderAll(data);
  }

  // ============================================================
  // Render All
  // ============================================================
  function renderAll(response) {
    renderTopBar();
    renderObjects();
    renderSituation();
    renderActions();
    renderCompanionCard(response);
    renderExecutionCard(response);
    renderDrawerContent();
  }

  // ---- Top Bar ----
  function renderTopBar() {
    var p = uiState.projection;
    if (!p) return;

    dom.phaseLabel.textContent = p.sceneTitle || "探索中";

    // Danger level from danger_hints
    var dangers = p.dangerHints;
    var dangerText = "安全";
    var dangerClass = "";
    if (dangers.length > 0) {
      var hasConfronting = dangers.some(function(d) {
        return (d.danger_key || "").indexOf("confronting") >= 0 ||
               (d.danger_key || "").indexOf("approaching") >= 0;
      });
      var hasForeshadow = dangers.some(function(d) {
        return (d.danger_key || "").indexOf("foreshadowing") >= 0;
      });
      if (hasConfronting) {
        dangerText = "危险：接近中";
        dangerClass = "level-danger";
      } else if (hasForeshadow) {
        dangerText = "危险：有预兆";
        dangerClass = "level-warning";
      } else {
        dangerText = "有潜在危险";
        dangerClass = "level-warning";
      }
    }
    dom.dangerLabel.textContent = dangerText;
    dom.dangerLabel.className = "danger-tag " + dangerClass;

    // Goal — from scene summary or cognition
    var goal = "探索世界";
    if (p.cognitionLines && p.cognitionLines.length > 0) {
      goal = textOf(p.cognitionLines[0].aspect_label) || goal;
    }
    dom.goalLabel.textContent = "目标：" + goal;

    dom.turnBadge.textContent = "回合 " + uiState.serverTurn;
  }

  // ---- Objects (Left Panel) ----
  function renderObjects() {
    clear(dom.objectGrid);
    var p = uiState.projection;
    if (!p || p.objectCards.length === 0) return;

    p.objectCards.forEach(function(obj) {
      var chip = el("div", "object-chip");
      if (obj.key === uiState.selectedObjectKey) {
        chip.classList.add("selected");
      }
      chip.dataset.objectKey = obj.key;
      chip.addEventListener("click", function() { selectObject(obj.key); });

      // Icon from primary tag
      var icon = iconForObject(obj);
      var iconSpan = el("span", "obj-icon", icon);
      chip.appendChild(iconSpan);

      // Info
      var info = el("div", "obj-info");
      info.appendChild(el("div", "obj-name", obj.name));
      var meta = obj.tagLabels.slice(0, 2).join(" · ");
      if (!meta && obj.desc) meta = obj.desc.slice(0, 20);
      info.appendChild(el("div", "obj-meta", meta || "未知"));
      chip.appendChild(info);

      // Badge
      if (obj.primaryTag) {
        var badge = el("span", "obj-badge " + obj.primaryTag, obj.primaryTag === "danger" ? "⚠" : obj.primaryTag === "warning" ? "⚡" : obj.primaryTag);
        if (obj.primaryTag === "danger") badge.textContent = "⚠";
        else if (obj.primaryTag === "warning") badge.textContent = "⚡";
        else if (obj.primaryTag === "tool") badge.textContent = "🔧";
        else if (obj.primaryTag === "material") badge.textContent = "📦";
        chip.appendChild(badge);
      }

      dom.objectGrid.appendChild(chip);
    });
  }

  function iconForObject(obj) {
    var tags = obj.tags || [];
    if (tags.indexOf("fire") >= 0 || tags.indexOf("light_source") >= 0) return "🔥";
    if (tags.indexOf("axe") >= 0 || tags.indexOf("tool") >= 0) return "🪓";
    if (tags.indexOf("fruit") >= 0) return "🍎";
    if (tags.indexOf("leaf") >= 0) return "🌿";
    if (tags.indexOf("wood") >= 0 || tags.indexOf("material") >= 0) return "🪵";
    if (tags.indexOf("creature") >= 0 || tags.indexOf("agent_wildlife") >= 0) return "🐺";
    if (tags.indexOf("threat") >= 0) return "⚠";
    return "📦";
  }

  function selectObject(key) {
    if (uiState.selectedObjectKey === key) {
      uiState.selectedObjectKey = null;
    } else {
      uiState.selectedObjectKey = key;
    }
    if (uiState.projection) {
      renderObjects();
      renderSituation();
      renderActions();
    }
  }

  // ---- Situation (Center Panel) ----
  function renderSituation() {
    var p = uiState.projection;
    if (!p) return;

    // Scene summary: first line or combined
    dom.situationText.textContent = p.sceneSummary.slice(0, 2).join(" ") || "世界安静地等待你的下一步。";

    // Recent events (last 3)
    clear(dom.recentEvents);
    var recent = eventLog.slice(-3);
    recent.forEach(function(ev) {
      var item = el("div", "event-item tone-" + ev.tone, ev.text);
      dom.recentEvents.appendChild(item);
    });

    // Object detail (if selected)
    renderObjectDetail();
  }

  function renderObjectDetail() {
    var p = uiState.projection;
    var key = uiState.selectedObjectKey;
    if (!key || !p) {
      dom.objectDetail.classList.add("hidden");
      return;
    }

    var obj = null;
    for (var i = 0; i < p.objectCards.length; i++) {
      if (p.objectCards[i].key === key) { obj = p.objectCards[i]; break; }
    }
    if (!obj) {
      dom.objectDetail.classList.add("hidden");
      return;
    }

    dom.objectDetail.classList.remove("hidden");
    clear(dom.objectDetail);
    dom.objectDetail.appendChild(el("div", "detail-name", obj.name));
    if (obj.desc) dom.objectDetail.appendChild(el("div", "detail-desc", obj.desc));

    // Knowledge about this object
    var knowledge = p.actorKnowledge || [];
    var relatedKnowledge = knowledge.filter(function(k) {
      var subj = textOf(k.subject_label, "");
      return subj && obj.name.indexOf(subj) >= 0;
    });
    if (relatedKnowledge.length > 0) {
      var kDiv = el("div", "detail-desc");
      kDiv.style.marginTop = "4px";
      kDiv.style.color = "var(--knowledge)";
      kDiv.textContent = "已知：" + relatedKnowledge.map(function(k) {
        return textOf(k.predicate_label) + textOf(k.effect_summary);
      }).join("；");
      dom.objectDetail.appendChild(kDiv);
    }

    // Tags
    if (obj.tagLabels.length > 0) {
      var tagRow = el("div", "detail-tags");
      obj.tagLabels.forEach(function(l) {
        tagRow.appendChild(el("span", "detail-tag", l));
      });
      dom.objectDetail.appendChild(tagRow);
    }
  }

  // ---- Actions (Right Panel) ----
  function renderActions() {
    clear(dom.actionList);
    var p = uiState.projection;
    if (!p) return;

    var actions = [];
    var selectedKey = uiState.selectedObjectKey;

    if (selectedKey) {
      // Actions from selected object's card
      var obj = null;
      for (var i = 0; i < p.objectCards.length; i++) {
        if (p.objectCards[i].key === selectedKey) { obj = p.objectCards[i]; break; }
      }
      if (obj && obj.actions) {
        actions = actions.concat(obj.actions);
      }
      // Also include action_bar items targeting this object
      p.actionBar.forEach(function(a) {
        if (a.targetRefs.indexOf(selectedKey) >= 0) {
          actions.push(a);
        }
      });
    } else {
      // No selection: show action_bar items without specific target
      p.actionBar.forEach(function(a) {
        if (!a.targetRefs || a.targetRefs.length === 0) {
          actions.push(a);
        }
      });
    }

    // Deduplicate by actionKey
    var seen = {};
    var unique = [];
    actions.forEach(function(a) {
      var k = a.actionKey || a.label;
      if (!seen[k]) { seen[k] = true; unique.push(a); }
    });

    if (unique.length === 0) {
      var hint = el("div", "event-item tone-neutral");
      hint.textContent = selectedKey
        ? "选中对象无可执行动作"
        : "选择一个对象以查看可执行动作";
      dom.actionList.appendChild(hint);
      return;
    }

    unique.forEach(function(a) {
      dom.actionList.appendChild(renderActionButton(a));
    });
  }

  function renderActionButton(action) {
    var btn = el("button", "action-btn");
    btn.textContent = action.label || "行动";
    btn.dataset.busyKey = action.actionKey || action.label || "";
    btn.disabled = !action.enabled;

    // Visual style based on affordance
    var aff = (action.affordance || "").toLowerCase();
    if (aff === "tryeat" || aff === "try_eat") {
      btn.classList.add("primary");
    } else if (aff === "avoid") {
      btn.classList.add("danger");
    } else if (aff === "teach" || aff === "coordinate") {
      btn.classList.add("warning");
    }

    // Disabled with reason
    if (!action.enabled && action.disabledReason) {
      btn.title = action.disabledReason;
    }

    btn.addEventListener("click", function() {
      sendTurn(action.inputText || action.label || "", action);
    });

    return btn;
  }

  // ---- Companion Card (P39) ----
  function renderCompanionCard(response) {
    clear(dom.companionContent);
    var reasoning = normalizeReasoning(response || {});

    if (!reasoning.visible) {
      dom.companionContent.appendChild(
        el("div", "card-line", "同伴暂无明确行动。")
      );
      return;
    }

    if (reasoning.goal) {
      dom.companionContent.appendChild(
        el("div", "card-line goal", "目标：" + reasoning.goal)
      );
    }
    if (reasoning.nextStep) {
      dom.companionContent.appendChild(
        el("div", "card-line", "下一步：" + reasoning.nextStep)
      );
    }
    if (reasoning.reason) {
      dom.companionContent.appendChild(
        el("div", "card-line", "原因：" + reasoning.reason)
      );
    }
    if (reasoning.blocked) {
      dom.companionContent.appendChild(
        el("div", "card-line blocked", "阻塞：" + reasoning.blocked)
      );
    }
    if (reasoning.knowledgeLines && reasoning.knowledgeLines.length > 0) {
      dom.companionContent.appendChild(
        el("div", "card-line", "携带：火把（用于守营验证）")
      );
      reasoning.knowledgeLines.forEach(function(line) {
        dom.companionContent.appendChild(
          el("div", "card-line knowledge", "知识：" + line)
        );
      });
    }
  }

  // ---- Execution Card (P40 placeholder) ----
  function renderExecutionCard(_response) {
    clear(dom.executionContent);
    var exec = normalizeExecution(_response || {});

    if (!exec.visible) {
      dom.executionContent.appendChild(
        el("div", "card-line", "执行系统尚未接入。")
      );
      return;
    }

    if (exec.goal) {
      dom.executionContent.appendChild(
        el("div", "card-line", "当前执行：" + exec.goal)
      );
    }
    if (exec.step) {
      dom.executionContent.appendChild(
        el("div", "card-line", "步骤：" + exec.step)
      );
    }
    if (exec.status) {
      dom.executionContent.appendChild(
        el("div", "card-line", "状态：" + exec.status)
      );
    }
    if (exec.interrupt) {
      dom.executionContent.appendChild(
        el("div", "card-line blocked", "打断：" + exec.interrupt)
      );
    }
    if (exec.blocked) {
      dom.executionContent.appendChild(
        el("div", "card-line blocked", "阻塞：" + exec.blocked)
      );
    }
  }

  // ============================================================
  // Log Drawer
  // ============================================================
  function toggleDrawer(open) {
    if (open === undefined) open = !uiState.logDrawerOpen;
    uiState.logDrawerOpen = open;
    if (open) {
      dom.logDrawer.classList.remove("hidden");
      renderDrawerContent();
    } else {
      dom.logDrawer.classList.add("hidden");
    }
  }

  function switchDrawerTab(tab) {
    uiState.drawerTab = tab;
    var tabs = dom.logDrawer.querySelectorAll(".drawer-tab");
    tabs.forEach(function(t) {
      t.classList.toggle("active", t.dataset.tab === tab);
    });
    dom.drawerEvents.classList.toggle("hidden", tab !== "events");
    dom.drawerKnowledge.classList.toggle("hidden", tab !== "knowledge");
    dom.drawerSystem.classList.toggle("hidden", tab !== "system");
  }

  function renderDrawerContent() {
    // Events
    clear(dom.drawerEvents);
    if (eventLog.length === 0) {
      dom.drawerEvents.appendChild(el("div", "drawer-event", "暂无事件记录。"));
    } else {
      for (var i = eventLog.length - 1; i >= 0; i--) {
        var ev = eventLog[i];
        var div = el("div", "drawer-event");
        div.appendChild(el("div", "event-turn", ev.turn));
        div.appendChild(el("span", "", ev.text));
        dom.drawerEvents.appendChild(div);
      }
    }

    // Knowledge
    clear(dom.drawerKnowledge);
    var p = uiState.projection;
    if (!p || (!p.actorKnowledge.length && !p.recipientKnowledge.length)) {
      dom.drawerKnowledge.appendChild(
        el("div", "drawer-system-line", "暂无知识记录。")
      );
    } else {
      if (p.actorKnowledge.length > 0) {
        dom.drawerKnowledge.appendChild(
          el("div", "knowledge-section-title", "我的知识")
        );
        p.actorKnowledge.forEach(function(k) {
          var line = el("div", "drawer-knowledge-line");
          line.textContent = knowledgeSummary(k);
          dom.drawerKnowledge.appendChild(line);
        });
      }
      if (p.recipientKnowledge.length > 0) {
        dom.drawerKnowledge.appendChild(
          el("div", "knowledge-section-title", "同伴知识")
        );
        p.recipientKnowledge.forEach(function(k) {
          var line = el("div", "drawer-knowledge-line");
          line.textContent = knowledgeSummary(k);
          dom.drawerKnowledge.appendChild(line);
        });
      }
    }

    // System
    clear(dom.drawerSystem);
    var issues = p ? p.issues : [];
    var conditionHints = p ? p.conditionHints : [];
    if (issues.length === 0 && conditionHints.length === 0) {
      dom.drawerSystem.appendChild(
        el("div", "drawer-system-line", "暂无系统信息。")
      );
    } else {
      issues.forEach(function(issue) {
        dom.drawerSystem.appendChild(
          el("div", "drawer-system-line", textOf(issue.safe_summary))
        );
      });
      conditionHints.forEach(function(hint) {
        dom.drawerSystem.appendChild(
          el("div", "drawer-system-line", textOf(hint.summary_text))
        );
      });
    }
  }

  // ============================================================
  // Event Handlers
  // ============================================================

  // Bottom bar buttons
  dom.btnWait.addEventListener("click", function() {
    // Look for wait action in action_bar, else send free text
    var p = uiState.projection;
    var waitAction = null;
    if (p) {
      for (var i = 0; i < p.actionBar.length; i++) {
        var a = p.actionBar[i];
        if (
          (a.actionKey || "").indexOf("wait") >= 0 ||
          (a.label || "").indexOf("等待") >= 0
        ) {
          waitAction = a;
          break;
        }
      }
    }
    if (waitAction) {
      sendTurn(waitAction.inputText || "等待", waitAction);
    } else {
      sendTurn("等待", null);
    }
  });

  dom.btnTeach.addEventListener("click", function() {
    // Look for teach action in action_bar, else send free text
    var p = uiState.projection;
    var teachAction = null;
    if (p) {
      for (var i = 0; i < p.actionBar.length; i++) {
        var a = p.actionBar[i];
        if (
          (a.actionKey || "").indexOf("teach") >= 0 ||
          (a.label || "").indexOf("教") >= 0
        ) {
          teachAction = a;
          break;
        }
      }
    }
    if (teachAction) {
      sendTurn(teachAction.inputText || "教同伴", teachAction);
    } else {
      sendTurn("教同伴", null);
    }
  });

  dom.btnKnowledge.addEventListener("click", function() {
    toggleDrawer(true);
    switchDrawerTab("knowledge");
  });

  dom.btnLog.addEventListener("click", function() {
    toggleDrawer(true);
    switchDrawerTab("events");
  });

  dom.btnReset.addEventListener("click", function() {
    if (confirm("确定重新开始吗？当前进度会丢失。")) {
      bootstrap(true);
    }
  });

  // Drawer
  dom.drawerClose.addEventListener("click", function() {
    toggleDrawer(false);
  });

  dom.logDrawer.addEventListener("click", function(e) {
    if (e.target.classList.contains("drawer-tab")) {
      switchDrawerTab(e.target.dataset.tab);
    }
  });

  // Close drawer on background click
  dom.logDrawer.addEventListener("click", function(e) {
    if (e.target === dom.logDrawer) {
      toggleDrawer(false);
    }
  });

  // Global: deselect object on escape-like tap elsewhere
  document.addEventListener("click", function(e) {
    // If click is on the center panel (not on object or action), allow deselection hint
    // We don't auto-deselect - user must click the selected object again to deselect
  });

  // ============================================================
  // Init
  // ============================================================
  bootstrap(false);
})();
