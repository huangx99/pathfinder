(() => {
  'use strict';

  // ---------------------------------------------------------------------------
  // 1. State
  // ---------------------------------------------------------------------------
  const state = {
    clientId: '',
    sessionId: '',
    clientSequence: 0,
    projectionVersion: 0,
    projection: null,
    availableCommands: [],
    eventFeed: [],
    frontendHints: [],
    warningKeys: [],
    connectionMode: 'unknown',
    requestState: 'idle',
    syncState: 'empty',
    lastError: null,
    logs: [],
    pendingDirection: null,
    lastDirectionSubmitTime: 0,
  };

  const DIRECTION_COOLDOWN_MS = 80;
  const DIRECTION_REPEAT_MS = 120;

  // ---------------------------------------------------------------------------
  // 2. DOM refs
  // ---------------------------------------------------------------------------
  const canvas = document.getElementById('gameCanvas');
  const ctx = canvas.getContext('2d');
  const modeBadge = document.getElementById('modeBadge');
  const sessionLine = document.getElementById('sessionLine');
  const versionLine = document.getElementById('versionLine');
  const actorLine = document.getElementById('actorLine');
  const questBody = document.getElementById('questBody');
  const commandList = document.getElementById('commandList');
  const logEntries = document.getElementById('logEntries');
  const debugPre = document.getElementById('debugPre');
  const resetBtn = document.getElementById('resetBtn');
  const toastContainer = document.getElementById('toastContainer');
  const joystickZone = document.getElementById('joystickZone');
  const dpadButtons = Array.from(document.querySelectorAll('.dpad-btn'));
  const wasdKeys = {
    w: null, a: null, s: null, d: null,
  };

  // ---------------------------------------------------------------------------
  // 3. Utils
  // ---------------------------------------------------------------------------
  function uuid() {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, c => {
      const r = Math.random() * 16 | 0;
      return (c === 'x' ? r : (r & 0x3 | 0x8)).toString(16);
    });
  }

  function safe(v, fallback) {
    return v !== undefined && v !== null ? v : fallback;
  }

  function log(msg, kind = 'info') {
    const entry = { time: new Date().toLocaleTimeString(), text: String(msg), kind };
    state.logs.push(entry);
    if (state.logs.length > 100) state.logs.shift();
    renderLogs();
  }

  function toast(msg, kind = 'info') {
    const el = document.createElement('div');
    el.className = 'toast ' + kind;
    el.textContent = msg;
    toastContainer.appendChild(el);
    setTimeout(() => el.remove(), 3000);
  }

  function setConnectionMode(mode) {
    state.connectionMode = mode;
    const map = {
      unknown: { cls: 'unknown', text: '连接中…' },
      real_backend: { cls: 'real', text: '真实后端' },
      mock_dev: { cls: 'mock', text: 'MOCK 模式' },
      offline_error: { cls: 'error', text: '连接失败' },
    };
    const m = map[mode] || map.unknown;
    modeBadge.className = 'mode-badge ' + m.cls;
    modeBadge.textContent = m.text;
  }

  function setRequestState(s) {
    state.requestState = s;
    resetBtn.disabled = (s !== 'idle');
  }

  // ---------------------------------------------------------------------------
  // 4. Session storage
  // ---------------------------------------------------------------------------
  function loadSession() {
    state.clientId = localStorage.getItem('pathfinder_client_id') || ('web_' + uuid().slice(0, 8));
    state.sessionId = localStorage.getItem('pathfinder_session_id') || ('session_' + uuid().slice(0, 8));
    localStorage.setItem('pathfinder_client_id', state.clientId);
    localStorage.setItem('pathfinder_session_id', state.sessionId);
  }

  function saveSession() {
    localStorage.setItem('pathfinder_client_id', state.clientId);
    localStorage.setItem('pathfinder_session_id', state.sessionId);
  }

  function clearSession() {
    state.sessionId = 'session_' + uuid().slice(0, 8);
    state.clientSequence = 0;
    state.projectionVersion = 0;
    state.projection = null;
    state.availableCommands = [];
    saveSession();
  }

  // ---------------------------------------------------------------------------
  // 5. API
  // ---------------------------------------------------------------------------
  async function postJson(path, body) {
    const url = path;
    const response = await fetch(url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(body),
    });
    const data = await response.json().catch(() => ({}));
    if (!response.ok) {
      const err = new Error(data.message || `${path} HTTP ${response.status}`);
      err.status = response.status;
      err.data = data;
      throw err;
    }
    return data;
  }

  async function apiBootstrap() {
    return postJson('/api/client/bootstrap', {
      client_id: state.clientId,
      session_id: state.sessionId,
      requested_actor_key: 'player',
      requested_layer_key: 'surface',
      client_protocol_version: 1,
      create_if_missing: true,
      dev_reset_if_allowed: false,
    });
  }

  async function apiCommand(optionId) {
    state.clientSequence += 1;
    return postJson('/api/client/command', {
      session_id: state.sessionId,
      client_id: state.clientId,
      client_sequence: state.clientSequence,
      known_projection_version: state.projectionVersion,
      submit_mode: 'option_id',
      option_id: optionId,
      selection_context: emptySelectionContext(),
    });
  }

  async function apiRefresh() {
    return postJson('/api/client/refresh', {
      session_id: state.sessionId,
      client_id: state.clientId,
      known_projection_version: state.projectionVersion,
      requested_scopes: ['full_safe_world'],
      requested_layer_key: 'surface',
      viewport_center_x: 0,
      viewport_center_y: 0,
    });
  }

  async function apiReset() {
    return postJson('/api/client/reset', {
      session_id: state.sessionId,
      client_id: state.clientId,
      confirmed: true,
    });
  }

  function emptySelectionContext() {
    return {
      target: {
        target_kind: 'none',
        target_coord: null,
        target_entity_id: '',
        target_actor_key: '',
        target_item_key: '',
        target_inventory_id: '',
        target_area_id: '',
        knowledge_key: '',
        recipe_key: '',
      },
      selected_actor_key: '',
      selected_entity_id: '',
      selected_inventory_id: '',
      selected_area_id: '',
    };
  }

  // ---------------------------------------------------------------------------
  // 6. Projection helpers
  // ---------------------------------------------------------------------------
  function applyFullProjection(proj) {
    state.projection = {
      projection_version: safe(proj.projection_version, 0),
      actor_key: safe(proj.actor_key, ''),
      active_layer_key: safe(proj.active_layer_key, 'surface'),
      visible_cells: safe(proj.visible_cells, []),
      visible_entities: safe(proj.visible_entities, []),
      inventories: safe(proj.inventories, []),
      knowledge: safe(proj.knowledge, []),
      area_effects: safe(proj.area_effects, []),
      safe_summary_keys: safe(proj.safe_summary_keys, []),
    };
  }

  function applyPatch(patch) {
    if (!state.projection) return false;
    const p = state.projection;
    p.projection_version = safe(patch.new_projection_version, p.projection_version);

    function mergeById(list, changes, idKey) {
      for (const ch of changes) {
        const id = ch[idKey];
        const idx = list.findIndex(x => x[idKey] === id);
        if (ch.op === 'remove' || ch.op === 'Remove') {
          if (idx >= 0) list.splice(idx, 1);
        } else if (ch.op === 'clear' || ch.op === 'Clear') {
          list.length = 0;
        } else {
          if (idx >= 0) list[idx] = ch;
          else list.push(ch);
        }
      }
    }

    mergeById(p.visible_cells, safe(patch.changed_cells, []), 'cell_id');
    mergeById(p.visible_entities, safe(patch.changed_entities, []), 'entity_id');
    mergeById(p.inventories, safe(patch.changed_inventories, []), 'inventory_id');
    mergeById(p.knowledge, safe(patch.changed_knowledge, []), 'actor_key');
    mergeById(p.area_effects, safe(patch.changed_area_effects, []), 'area_effect_id');
    return true;
  }

  // ---------------------------------------------------------------------------
  // 7. Actor coord helpers
  // ---------------------------------------------------------------------------
  function getActorCoord() {
    if (!state.projection) return null;
    const actorKey = state.projection.actor_key;
    // Try visible_entities first
    for (const e of state.projection.visible_entities) {
      const fields = e.fields || {};
      if (fields.actor_key === actorKey) {
        const x = parseInt(fields.x, 10);
        const y = parseInt(fields.y, 10);
        if (!isNaN(x) && !isNaN(y)) return { x, y, layer_key: fields.layer_key || 'surface' };
      }
      if (e.entity_id && e.entity_id.startsWith(actorKey)) {
        const x = parseInt(fields.x, 10);
        const y = parseInt(fields.y, 10);
        if (!isNaN(x) && !isNaN(y)) return { x, y, layer_key: fields.layer_key || 'surface' };
      }
    }
    // Fallback: try visible_cells for actor
    for (const c of state.projection.visible_cells) {
      const f = c.fields || {};
      if (f.actor_key === actorKey) {
        const x = parseInt(f.x, 10);
        const y = parseInt(f.y, 10);
        if (!isNaN(x) && !isNaN(y)) return { x, y, layer_key: f.layer_key || 'surface' };
      }
    }
    return null;
  }

  // ---------------------------------------------------------------------------
  // 8. Move direction resolution
  // ---------------------------------------------------------------------------
  function resolveMoveOption(direction) {
    const actorCoord = getActorCoord();
    if (!actorCoord) return null;

    const dirMap = {
      up:    { dx: 0, dy: -1 },
      down:  { dx: 0, dy: 1 },
      left:  { dx: -1, dy: 0 },
      right: { dx: 1, dy: 0 },
    };
    const d = dirMap[direction];
    if (!d) return null;

    const targetX = actorCoord.x + d.dx;
    const targetY = actorCoord.y + d.dy;

    for (const cmd of state.availableCommands) {
      if (cmd.command_kind !== 'Move' && cmd.command_kind !== 'move') continue;
      if (!cmd.enabled) continue;
      const tc = cmd.target && cmd.target.target_coord;
      if (!tc) continue;
      if (tc.x === targetX && tc.y === targetY && (tc.layer_key || 'surface') === actorCoord.layer_key) {
        return cmd.option_id;
      }
    }
    return null;
  }

  function hasMoveCommands() {
    return state.availableCommands.some(c =>
      (c.command_kind === 'Move' || c.command_kind === 'move') && c.enabled
    );
  }

  function commandLabel(cmd, fallback) {
    if (!cmd) return fallback || '未知命令';
    return cmd.label_text || cmd.command_kind || cmd.option_id || fallback || '未知命令';
  }

  function resultKindOf(resp) {
    const result = resp && resp.result ? resp.result : {};
    return String(result.result_kind || 'unknown').toLowerCase();
  }

  function resultText(kind) {
    const map = {
      succeeded: '成功',
      noop: '无变化',
      deferred: '已排队',
      blocked: '被阻止',
      failed: '失败',
      interrupted: '被打断',
      unknown: '未知结果',
    };
    return map[kind] || kind;
  }

  // ---------------------------------------------------------------------------
  // 9. Submit flow
  // ---------------------------------------------------------------------------
  async function submitOption(optionId) {
    if (state.requestState !== 'idle') {
      toast('请求处理中，请稍候', 'warn');
      return;
    }
    const found = state.availableCommands.find(c => c.option_id === optionId);
    if (!found) {
      toast('该选项已过期，正在刷新…', 'warn');
      await doRefresh();
      return;
    }

    const label = commandLabel(found, optionId);
    setRequestState('submitting_command');
    log(`正在执行: ${label}`, 'info');
    try {
      const resp = await apiCommand(optionId);
      const outcome = applyCommandResponse(resp);
      if (outcome.status === 'needs_refresh') {
        log(`状态不同步，刷新后再确认: ${label}`, 'warn');
        setRequestState('idle');
        await doRefresh();
        return;
      }

      const kind = outcome.resultKind;
      if (kind === 'succeeded') {
        log(`执行成功: ${label}`, 'success');
      } else if (kind === 'noop' || kind === 'deferred') {
        log(`执行结果: ${label}（${resultText(kind)}）`, 'info');
      } else {
        const reason = outcome.failureReasons.length ? `: ${outcome.failureReasons.join(', ')}` : '';
        log(`执行${resultText(kind)}: ${label}${reason}`, kind === 'blocked' ? 'warn' : 'error');
      }
      appendEvents(resp.event_feed, resp.frontend_hints, resp.warning_keys);
    } catch (err) {
      handleError('命令提交失败', err);
    } finally {
      setRequestState('idle');
      render();
    }
  }

  async function submitDirection(direction, force = false) {
    if (state.requestState !== 'idle') return;
    const now = Date.now();
    if (!force && now - state.lastDirectionSubmitTime < DIRECTION_COOLDOWN_MS) return;
    const optionId = resolveMoveOption(direction);
    if (!optionId) {
      if (force) log(`没有可用移动命令: ${directionText(direction)}`, 'warn');
      return;
    }
    state.lastDirectionSubmitTime = now;
    await submitOption(optionId);
  }

  function applyCommandResponse(resp) {
    state.sessionId = safe(resp.session_id, state.sessionId);

    const result = resp.result || {};
    const failureReasons = safe(result.failure_reason_keys, []);
    const outcome = {
      status: 'ok',
      resultKind: resultKindOf(resp),
      failureReasons,
    };

    const patch = resp.projection_patch;
    if (safe(resp.requires_full_refresh, false)) {
      log('后端要求全量刷新', 'warn');
      outcome.status = 'needs_refresh';
      return outcome;
    }
    if (patch) {
      const baseVer = safe(patch.base_projection_version, 0);
      const localVer = state.projectionVersion;
      if (baseVer !== 0 && baseVer !== localVer) {
        log(`Patch 版本不匹配，本地=${localVer}，补丁基线=${baseVer}`, 'warn');
        outcome.status = 'needs_refresh';
        return outcome;
      }
      if (!applyPatch(patch)) {
        log('本地缺少世界投影，执行全量刷新', 'warn');
        outcome.status = 'needs_refresh';
        return outcome;
      }
    }

    state.projectionVersion = safe(resp.new_projection_version, state.projectionVersion);
    state.availableCommands = safe(resp.available_commands, []);
    state.eventFeed = safe(resp.event_feed, []);
    state.frontendHints = safe(resp.frontend_hints, []);
    state.warningKeys = safe(resp.warning_keys, []);

    if (failureReasons.length) {
      toast('命令被阻止', 'warn');
    }

    saveSession();
    return outcome;
  }

  // ---------------------------------------------------------------------------
  // 10. Bootstrap / Refresh / Reset
  // ---------------------------------------------------------------------------
  async function doBootstrap() {
    setRequestState('bootstrapping');
    try {
      const resp = await apiBootstrap();
      state.sessionId = safe(resp.session_id, state.sessionId);
      state.projectionVersion = safe(resp.projection_version, 0);
      applyFullProjection(resp.full_projection);
      state.availableCommands = safe(resp.available_commands, []);
      state.eventFeed = safe(resp.event_feed, []);
      state.frontendHints = safe(resp.frontend_hints, []);
      state.warningKeys = safe(resp.warning_keys, []);
      setConnectionMode('real_backend');
      state.syncState = 'synced';
      appendEvents(resp.event_feed, resp.frontend_hints, resp.warning_keys);
      log('Bootstrap 成功', 'success');
      saveSession();
    } catch (err) {
      if (err.status === 404 || err.message && err.message.includes('Failed to fetch')) {
        setConnectionMode('offline_error');
        log('无法连接后端，请确认服务已启动 (端口 1999)', 'error');
        toast('无法连接后端', 'error');
      } else {
        handleError('Bootstrap 失败', err);
      }
    } finally {
      setRequestState('idle');
      render();
    }
  }

  async function doRefresh() {
    if (state.requestState !== 'idle') return;
    setRequestState('refreshing');
    try {
      const resp = await apiRefresh();
      state.sessionId = safe(resp.session_id, state.sessionId);
      state.projectionVersion = safe(resp.projection_version, 0);
      applyFullProjection(resp.full_projection);
      state.availableCommands = safe(resp.available_commands, []);
      state.eventFeed = safe(resp.event_feed, []);
      state.frontendHints = safe(resp.frontend_hints, []);
      state.warningKeys = safe(resp.warning_keys, []);
      state.syncState = 'synced';
      appendEvents(resp.event_feed, resp.frontend_hints, resp.warning_keys);
      log('刷新成功', 'success');
      saveSession();
    } catch (err) {
      handleError('刷新失败', err);
    } finally {
      setRequestState('idle');
      render();
    }
  }

  async function doReset() {
    if (state.requestState !== 'idle') return;
    if (!confirm('确定要重置世界吗？当前会话将被清除。')) return;
    setRequestState('resetting');
    try {
      const resp = await apiReset();
      const boot = resp.bootstrap || resp;
      state.sessionId = safe(boot.session_id, state.sessionId);
      state.projectionVersion = safe(boot.projection_version, 0);
      applyFullProjection(boot.full_projection);
      state.availableCommands = safe(boot.available_commands, []);
      state.eventFeed = safe(boot.event_feed, []);
      state.frontendHints = safe(boot.frontend_hints, []);
      state.warningKeys = safe(boot.warning_keys, []);
      setConnectionMode('real_backend');
      state.syncState = 'synced';
      state.clientSequence = 0;
      appendEvents(boot.event_feed, boot.frontend_hints, boot.warning_keys);
      log('世界已重置', 'success');
      saveSession();
    } catch (err) {
      handleError('重置失败', err);
    } finally {
      setRequestState('idle');
      render();
    }
  }

  function handleError(title, err) {
    const msg = err.message || String(err);
    log(`${title}: ${msg}`, 'error');
    toast(`${title}: ${msg}`, 'error');
    state.lastError = { title, message: msg, raw: err };
  }

  function appendEvents(events, hints, warnings) {
    for (const e of safe(events, [])) {
      const text = e.title_text || e.body_text || JSON.stringify(e);
      log(`[事件] ${text}`, 'info');
    }
    for (const h of safe(hints, [])) {
      log(`[提示] ${h.text || ''}`, 'info');
    }
    for (const w of safe(warnings, [])) {
      log(`[警告] ${w}`, 'warn');
    }
  }

  // ---------------------------------------------------------------------------
  // 11. Rendering
  // ---------------------------------------------------------------------------
  const TILE_SIZE = 48;

  function viewportTilesX() { return Math.floor(canvas.width / TILE_SIZE); }
  function viewportTilesY() { return Math.floor(canvas.height / TILE_SIZE); }

  function resizeCanvas() {
    // Align canvas to integer multiples of TILE_SIZE to avoid partial-tile gaps
    canvas.width = Math.floor(window.innerWidth / TILE_SIZE) * TILE_SIZE;
    canvas.height = Math.floor(window.innerHeight / TILE_SIZE) * TILE_SIZE;
    renderMap();
  }

  function render() {
    renderStatus();
    renderMap();
    renderCommands();
    renderLogs();
    renderDebug();
  }

  function renderStatus() {
    sessionLine.textContent = 'Session: ' + (state.sessionId || '—').slice(0, 24);
    versionLine.textContent = 'Version: ' + state.projectionVersion;
    actorLine.textContent = 'Actor: ' + (state.projection ? state.projection.actor_key : '—');

    const summary = [];
    if (state.projection) {
      summary.push('层: ' + state.projection.active_layer_key);
      summary.push('格子: ' + state.projection.visible_cells.length);
      summary.push('实体: ' + state.projection.visible_entities.length);
      summary.push('背包: ' + state.projection.inventories.length);
    }
    const coord = getActorCoord();
    if (coord) summary.push(`坐标: ${coord.x}, ${coord.y}`);
    summary.push('同步: ' + state.syncState);
    if (state.requestState !== 'idle') summary.push('请求: ' + state.requestState);
    questBody.textContent = summary.join(' | ') || '等待后端数据…';
  }

  function renderMap() {
    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);

    if (!state.projection) {
      ctx.fillStyle = '#05070a';
      ctx.fillRect(0, 0, w, h);
      ctx.fillStyle = '#475569';
      ctx.font = '10px monospace';
      ctx.textAlign = 'center';
      ctx.fillText('等待世界投影…', w / 2, h / 2);
      return;
    }

    const cells = state.projection.visible_cells;
    const entities = state.projection.visible_entities;
    const actorCoord = getActorCoord();

    if (!cells.length && !entities.length) {
      ctx.fillStyle = '#05070a';
      ctx.fillRect(0, 0, w, h);
      ctx.fillStyle = '#475569';
      ctx.font = '10px monospace';
      ctx.textAlign = 'center';
      ctx.fillText('世界投影为空', w / 2, h / 2);
      ctx.font = '9px monospace';
      ctx.fillText('后端尚未返回可见格子和实体', w / 2, h / 2 + 18);
      return;
    }

    const halfX = Math.floor(viewportTilesX() / 2);
    const halfY = Math.floor(viewportTilesY() / 2);
    let viewMinX, viewMinY;
    if (actorCoord) {
      viewMinX = actorCoord.x - halfX;
      viewMinY = actorCoord.y - halfY;
    } else {
      let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
      for (const c of cells) {
        const f = c.fields || {};
        const x = parseInt(f.x, 10); const y = parseInt(f.y, 10);
        if (!isNaN(x)) { minX = Math.min(minX, x); maxX = Math.max(maxX, x); }
        if (!isNaN(y)) { minY = Math.min(minY, y); maxY = Math.max(maxY, y); }
      }
      for (const e of entities) {
        const f = e.fields || {};
        const x = parseInt(f.x, 10); const y = parseInt(f.y, 10);
        if (!isNaN(x)) { minX = Math.min(minX, x); maxX = Math.max(maxX, x); }
        if (!isNaN(y)) { minY = Math.min(minY, y); maxY = Math.max(maxY, y); }
      }
      if (!isFinite(minX)) { minX = 0; maxX = 0; }
      if (!isFinite(minY)) { minY = 0; maxY = 0; }
      const centerX = Math.floor((minX + maxX) / 2);
      const centerY = Math.floor((minY + maxY) / 2);
      viewMinX = centerX - halfX;
      viewMinY = centerY - halfY;
    }

    ctx.fillStyle = '#05070a';
    ctx.fillRect(0, 0, w, h);

    const exploredSet = new Set();
    for (const c of cells) {
      const f = c.fields || {};
      const x = parseInt(f.x, 10);
      const y = parseInt(f.y, 10);
      if (isNaN(x) || isNaN(y)) continue;
      exploredSet.add(`${x},${y}`);
      const px = (x - viewMinX) * TILE_SIZE;
      const py = (y - viewMinY) * TILE_SIZE;
      // Only skip tiles fully outside the left/top edge; canvas is TILE_SIZE-aligned
      if (px < -TILE_SIZE || py < -TILE_SIZE) continue;

      const terrain = f.terrain_key || f.terrain || 'unknown';
      const sprite = window.SPRITES && window.SPRITES[terrain];
      if (sprite) {
        ctx.drawImage(sprite, px, py);
      } else {
        ctx.fillStyle = terrainColor(terrain);
        ctx.fillRect(px, py, TILE_SIZE, TILE_SIZE);
      }
    }

    const actorKey = state.projection.actor_key;
    for (const e of entities) {
      const f = e.fields || {};
      const x = parseInt(f.x, 10);
      const y = parseInt(f.y, 10);
      if (isNaN(x) || isNaN(y)) continue;
      const px = (x - viewMinX) * TILE_SIZE;
      const py = (y - viewMinY) * TILE_SIZE;
      if (px < -TILE_SIZE || py < -TILE_SIZE) continue;

      const isActor = f.actor_key === actorKey || e.entity_id === actorKey || e.entity_id.startsWith(actorKey);
      if (isActor) {
        const sprite = window.SPRITES && window.SPRITES.player;
        if (sprite) ctx.drawImage(sprite, px, py);
        else drawEntityFallback(ctx, px + TILE_SIZE / 2, py + TILE_SIZE / 2, TILE_SIZE * 0.7, f, true);
      } else {
        const kind = f.entity_kind || f.kind || f.object_key || 'unknown';
        let spriteKey = null;
        if (kind.includes('tree') || kind.includes('Tree')) spriteKey = 'tree';
        else if (kind.includes('beast') || kind.includes('wolf') || kind.includes('Beast') || kind.includes('Wolf')) spriteKey = 'wolf';
        else if (kind.includes('item') || kind.includes('berry') || kind.includes('Berry') || kind.includes('rock') || kind.includes('Rock')) spriteKey = 'berry';
        else if (kind.includes('npc') || kind.includes('NPC') || kind.includes('companion') || kind.includes('Companion')) spriteKey = 'npc';
        else if (kind.includes('campfire') || kind.includes('Campfire') || kind.includes('fire') || kind.includes('Fire')) spriteKey = 'campfire';
        else spriteKey = 'unknown';

        const sprite = window.SPRITES && window.SPRITES[spriteKey];
        if (sprite) ctx.drawImage(sprite, px, py);
        else drawEntityFallback(ctx, px + TILE_SIZE / 2, py + TILE_SIZE / 2, TILE_SIZE * 0.7, f, false);
      }
    }

    // Fog of war for unexplored cells
    const vtw = viewportTilesX();
    const vth = viewportTilesY();
    for (let vy = 0; vy < vth; ++vy) {
      for (let vx = 0; vx < vtw; ++vx) {
        const worldX = viewMinX + vx;
        const worldY = viewMinY + vy;
        if (!exploredSet.has(`${worldX},${worldY}`)) {
          const px = vx * TILE_SIZE;
          const py = vy * TILE_SIZE;
          // Soft fog — no grid lines, just a very subtle dark fill
          ctx.fillStyle = '#070a0e';
          ctx.fillRect(px, py, TILE_SIZE, TILE_SIZE);
        }
      }
    }
  }

  function terrainColor(key) {
    const map = {
      grass: '#1a3a2a',
      forest: '#0f2e1c',
      water: '#1a2d4a',
      sand: '#3a3220',
      mountain: '#2a2a2a',
      plain: '#1e2e1e',
      // P57 noise terrain colors
      stone_field: '#4a4a40',
      water_edge: '#1a4a3a',
      blocked: '#3a1a1a',
      deep_water: '#0a1a3a',
      unknown: '#141e2e',
    };
    return map[key] || map.unknown;
  }

  function drawEntityFallback(ctx, cx, cy, size, fields, isActor) {
    ctx.save();
    ctx.fillStyle = 'rgba(0,0,0,0.3)';
    ctx.beginPath();
    ctx.ellipse(cx, cy + size * 0.4, size * 0.35, size * 0.12, 0, 0, Math.PI * 2);
    ctx.fill();

    const kind = fields.entity_kind || fields.kind || fields.object_key || 'unknown';
    if (isActor) {
      ctx.fillStyle = '#38bdf8';
      ctx.fillRect(cx - size * 0.2, cy - size * 0.35, size * 0.4, size * 0.55);
      ctx.fillStyle = '#bae6fd';
      ctx.beginPath();
      ctx.arc(cx, cy - size * 0.35, size * 0.18, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#0f172a';
      ctx.fillRect(cx - size * 0.06, cy - size * 0.38, size * 0.04, size * 0.04);
      ctx.fillRect(cx + size * 0.02, cy - size * 0.38, size * 0.04, size * 0.04);
    } else if (kind.includes('tree') || kind.includes('Tree')) {
      ctx.fillStyle = '#3f2e18';
      ctx.fillRect(cx - size * 0.08, cy - size * 0.1, size * 0.16, size * 0.35);
      ctx.fillStyle = '#1e4d2b';
      ctx.beginPath();
      ctx.arc(cx, cy - size * 0.25, size * 0.3, 0, Math.PI * 2);
      ctx.fill();
    } else if (kind.includes('beast') || kind.includes('wolf') || kind.includes('Beast')) {
      ctx.fillStyle = '#7f1d1d';
      ctx.beginPath();
      ctx.ellipse(cx, cy, size * 0.3, size * 0.2, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#fca5a5';
      ctx.beginPath();
      ctx.arc(cx + size * 0.15, cy - size * 0.05, size * 0.1, 0, Math.PI * 2);
      ctx.fill();
    } else if (kind.includes('item') || kind.includes('berry') || kind.includes('rock')) {
      ctx.fillStyle = '#fbbf24';
      ctx.beginPath();
      ctx.arc(cx, cy, size * 0.15, 0, Math.PI * 2);
      ctx.fill();
    } else {
      ctx.fillStyle = '#94a3b8';
      ctx.fillRect(cx - size * 0.15, cy - size * 0.15, size * 0.3, size * 0.3);
    }
    ctx.restore();
  }

  function renderCommands() {
    commandList.innerHTML = '';
    const cmds = state.availableCommands;
    if (!cmds.length) {
      commandList.innerHTML = '<div style="color:var(--text-dim);font-size:12px;padding:8px 0;">暂无可用命令</div>';
      return;
    }
    // Separate move and non-move commands
    const moveCmds = [];
    const otherCmds = [];
    for (const c of cmds) {
      if (c.command_kind === 'Move' || c.command_kind === 'move') moveCmds.push(c);
      else otherCmds.push(c);
    }

    // Show move hint if there are move commands
    if (moveCmds.length) {
      const hint = document.createElement('div');
      hint.style.cssText = 'font-size:11px;color:var(--text-dim);padding:4px 0;border-bottom:1px solid rgba(148,163,184,0.1);margin-bottom:6px;';
      hint.textContent = '移动: ' + (window.innerWidth <= 768 ? '使用左下摇杆' : 'WASD 移动');
      commandList.appendChild(hint);
    }

    for (const c of otherCmds) {
      const btn = document.createElement('button');
      btn.className = 'cmd-btn';
      btn.disabled = !c.enabled || state.requestState !== 'idle';
      const label = c.label_text || c.command_kind || c.option_id;
      const kindSpan = document.createElement('span');
      kindSpan.className = 'cmd-kind';
      kindSpan.textContent = c.command_kind || '';
      btn.textContent = label;
      btn.appendChild(kindSpan);
      btn.onclick = () => submitOption(c.option_id);
      if (!c.enabled && c.disabled_reason_text) {
        btn.title = c.disabled_reason_text;
      }
      commandList.appendChild(btn);
    }
  }

  function renderLogs() {
    logEntries.innerHTML = '';
    for (const entry of state.logs.slice(-40)) {
      const div = document.createElement('div');
      div.className = 'log-entry ' + entry.kind;
      div.textContent = `[${entry.time}] ${entry.text}`;
      logEntries.appendChild(div);
    }
    logEntries.scrollTop = logEntries.scrollHeight;
  }

  function renderDebug() {
    debugPre.textContent = JSON.stringify({
      connectionMode: state.connectionMode,
      requestState: state.requestState,
      syncState: state.syncState,
      projectionVersion: state.projectionVersion,
      clientSequence: state.clientSequence,
      availableCommandsCount: state.availableCommands.length,
      hasMoveCommands: hasMoveCommands(),
      actorCoord: getActorCoord(),
    }, null, 2);
  }

  // ---------------------------------------------------------------------------
  // 12. Mobile D-pad
  // ---------------------------------------------------------------------------
  let directionHoldTimer = null;
  let activeDirectionButton = null;
  let activeDirectionPointerId = null;

  function directionText(direction) {
    const map = { up: '向上', down: '向下', left: '向左', right: '向右' };
    return map[direction] || direction;
  }

  function stopDirectionHold() {
    if (directionHoldTimer) {
      clearInterval(directionHoldTimer);
      directionHoldTimer = null;
    }
    if (activeDirectionButton) {
      activeDirectionButton.classList.remove('active');
      activeDirectionButton = null;
    }
    activeDirectionPointerId = null;
    state.pendingDirection = null;
  }

  function startDirectionHold(direction, button) {
    stopDirectionHold();
    activeDirectionButton = button;
    activeDirectionButton.classList.add('active');
    activeDirectionPointerId = null;
    state.pendingDirection = direction;
    log(`移动输入: ${directionText(direction)}`, 'info');
    submitDirection(direction, true);
    directionHoldTimer = setInterval(() => {
      if (state.pendingDirection) {
        submitDirection(state.pendingDirection);
      }
    }, DIRECTION_REPEAT_MS);
  }

  function initJoystick() {
    if (!dpadButtons.length) {
      log('移动方向键初始化失败', 'error');
      return;
    }

    for (const button of dpadButtons) {
      const direction = button.dataset.dir;
      if (!direction) continue;

      button.addEventListener('contextmenu', e => e.preventDefault());

      if (window.PointerEvent) {
        button.addEventListener('pointerdown', e => {
          e.preventDefault();
          activeDirectionPointerId = e.pointerId;
          button.setPointerCapture(e.pointerId);
          startDirectionHold(direction, button);
        });

        const endPointer = e => {
          if (activeDirectionPointerId !== null && e.pointerId !== activeDirectionPointerId) return;
          e.preventDefault();
          if (button.hasPointerCapture && button.hasPointerCapture(e.pointerId)) {
            button.releasePointerCapture(e.pointerId);
          }
          stopDirectionHold();
        };

        button.addEventListener('pointerup', endPointer);
        button.addEventListener('pointercancel', endPointer);
      } else {
        button.addEventListener('touchstart', e => {
          e.preventDefault();
          startDirectionHold(direction, button);
        }, { passive: false });
        button.addEventListener('touchend', e => {
          e.preventDefault();
          stopDirectionHold();
        }, { passive: false });
        button.addEventListener('touchcancel', e => {
          e.preventDefault();
          stopDirectionHold();
        }, { passive: false });
        button.addEventListener('mousedown', e => {
          e.preventDefault();
          startDirectionHold(direction, button);
        });
        window.addEventListener('mouseup', stopDirectionHold);
      }
    }
  }

  // ---------------------------------------------------------------------------
  // 13. Keyboard WASD
  // ---------------------------------------------------------------------------
  const pressedKeys = new Set();

  function initKeyboard() {
    window.addEventListener('keydown', e => {
      const key = e.key.toLowerCase();
      if (['w','a','s','d','arrowup','arrowdown','arrowleft','arrowright'].includes(key)) {
        e.preventDefault();
        if (pressedKeys.has(key)) return;
        pressedKeys.add(key);
        let dir = null;
        if (key === 'w' || key === 'arrowup') dir = 'up';
        if (key === 's' || key === 'arrowdown') dir = 'down';
        if (key === 'a' || key === 'arrowleft') dir = 'left';
        if (key === 'd' || key === 'arrowright') dir = 'right';
        if (dir) {
          highlightWasd(dir, true);
          submitDirection(dir);
        }
      }
    });

    window.addEventListener('keyup', e => {
      const key = e.key.toLowerCase();
      pressedKeys.delete(key);
      let dir = null;
      if (key === 'w' || key === 'arrowup') dir = 'up';
      if (key === 's' || key === 'arrowdown') dir = 'down';
      if (key === 'a' || key === 'arrowleft') dir = 'left';
      if (key === 'd' || key === 'arrowright') dir = 'right';
      if (dir) highlightWasd(dir, false);
    });
  }

  function highlightWasd(dir, active) {
    const map = { up: 'w', down: 's', left: 'a', right: 'd' };
    const el = document.querySelector('.wasd-hint');
    if (!el) return;
    const keys = el.querySelectorAll('.wasd-key');
    const target = map[dir];
    keys.forEach(k => {
      if (k.textContent.toLowerCase() === target) {
        k.classList.toggle('active', active);
      }
    });
  }

  // ---------------------------------------------------------------------------
  // 14. Event bindings
  // ---------------------------------------------------------------------------
  resetBtn.addEventListener('click', doReset);
  window.addEventListener('resize', resizeCanvas);

  // ---------------------------------------------------------------------------
  // 15. Startup
  // ---------------------------------------------------------------------------
  async function start() {
    loadSession();
    setConnectionMode('unknown');
    resizeCanvas();
    initJoystick();
    initKeyboard();
    log('先驱者客户端启动…');
    await doBootstrap();
  }

  start();
})();
