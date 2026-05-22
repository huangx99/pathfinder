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
    selectedEntityId: null,
    floatingTexts: [],
    screenShake: 0,
    lastTimestamp: 0,
    lastMapRenderTime: 0,
    actorAnimation: null,
    actorFacing: 'down',
    cameraCoord: null,
    cameraLockedToActor: false,
    cameraDrag: null,
    pinchZoom: null,
    zoom: 1,
    pathTarget: null,
    activePath: [],
    pathRunning: false,
    pathParticles: [],
    lastTap: null,
  };

  const DIRECTION_COOLDOWN_MS = 80;
  const DIRECTION_REPEAT_MS = 120;
  const MAP_FRAME_INTERVAL_MS = 120;
  const ACTOR_MOVE_ANIMATION_MS = 220;
  const MIN_ZOOM = 0.6;
  const MAX_ZOOM = 1.8;

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
  const followBtn = document.getElementById('followBtn');
  const zoomInBtn = document.getElementById('zoomInBtn');
  const zoomOutBtn = document.getElementById('zoomOutBtn');
  const joystickZone = document.getElementById('joystickZone');
  const joystickBase = document.getElementById('joystickBase');
  const joystickThumb = document.getElementById('joystickThumb');
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

  function addFloatingText(worldX, worldY, text, color) {
    state.floatingTexts.push({
      x: worldX, y: worldY, text, color,
      startTime: performance.now(),
      life: 2000,
    });
  }

  function addPathPixelBurst(worldX, worldY) {
    const colors = ['#38bdf8', '#7dd3fc', '#facc15', '#e0f2fe'];
    for (let i = 0; i < 14; ++i) {
      const angle = (Math.PI * 2 * i / 14) + Math.random() * 0.35;
      const speed = 0.45 + Math.random() * 0.65;
      state.pathParticles.push({
        x: worldX + 0.5,
        y: worldY + 0.5,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        color: colors[i % colors.length],
        startTime: performance.now(),
        life: 360 + Math.random() * 220,
      });
    }
  }

  function renderPathParticles(timestamp, viewMinX, viewMinY) {
    const tile = tileSize();
    state.pathParticles = state.pathParticles.filter(p => {
      const elapsed = timestamp - p.startTime;
      if (elapsed > p.life) return false;
      const progress = elapsed / p.life;
      const px = (p.x + p.vx * progress - viewMinX) * tile;
      const py = (p.y + p.vy * progress - viewMinY) * tile;
      const size = Math.max(2, Math.floor(tile * (0.08 - progress * 0.035)));
      ctx.save();
      ctx.globalAlpha = 1 - progress;
      ctx.fillStyle = p.color;
      ctx.fillRect(Math.round(px - size / 2), Math.round(py - size / 2), size, size);
      ctx.restore();
      return true;
    });
  }

  function renderFloatingTexts(timestamp, viewMinX, viewMinY) {
    const tile = tileSize();
    state.floatingTexts = state.floatingTexts.filter(ft => {
      const elapsed = timestamp - ft.startTime;
      if (elapsed > ft.life) return false;
      const progress = elapsed / ft.life;
      const px = (ft.x - viewMinX) * tile + tile / 2;
      const py = (ft.y - viewMinY) * tile - 36 * progress;
      const alpha = 1 - progress;
      if (alpha <= 0 || px < -tile || px > canvas.width + tile || py < -tile || py > canvas.height + tile) {
        return alpha > 0;
      }
      ctx.save();
      ctx.globalAlpha = alpha;
      ctx.fillStyle = ft.color || '#ffffff';
      ctx.font = 'bold 12px "Courier New", monospace';
      ctx.textAlign = 'center';
      ctx.shadowColor = 'rgba(0,0,0,0.8)';
      ctx.shadowBlur = 3;
      ctx.fillText(ft.text, px, py);
      ctx.restore();
      return true;
    });
  }

  function setConnectionMode(mode) {
    state.connectionMode = mode;
    const map = {
      unknown: { cls: 'unknown', text: t('loading', '连接中…') },
      real_backend: { cls: 'real', text: t('backend_real', '真实后端') },
      mock_dev: { cls: 'mock', text: t('mock_mode', 'MOCK 模式') },
      offline_error: { cls: 'error', text: t('backend_offline', '连接失败') },
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
    const actorCoord = getActorCoord() || { x: 0, y: 0 };
    return postJson('/api/client/refresh', {
      session_id: state.sessionId,
      client_id: state.clientId,
      known_projection_version: state.projectionVersion,
      requested_scopes: ['full_safe_world'],
      requested_layer_key: 'surface',
      viewport_center_x: actorCoord.x,
      viewport_center_y: actorCoord.y,
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
    state.actorAnimation = null;
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
    if (!state.cameraCoord || state.cameraLockedToActor) {
      state.cameraCoord = getActorCoord();
    }
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

  function easeOutCubic(t) {
    return 1 - Math.pow(1 - t, 3);
  }

  function sameCoord(a, b) {
    if (!a || !b) return false;
    return a.x === b.x && a.y === b.y && (a.layer_key || 'surface') === (b.layer_key || 'surface');
  }

  function getRenderActorCoord(timestamp = performance.now()) {
    const anim = state.actorAnimation;
    if (!anim) return getActorCoord();
    const progress = Math.min(1, Math.max(0, (timestamp - anim.startTime) / anim.duration));
    const eased = easeOutCubic(progress);
    const coord = {
      x: anim.fromX + (anim.toX - anim.fromX) * eased,
      y: anim.fromY + (anim.toY - anim.fromY) * eased,
      layer_key: anim.layer_key,
    };
    if (progress >= 1) {
      state.actorAnimation = null;
      return { x: anim.toX, y: anim.toY, layer_key: anim.layer_key };
    }
    return coord;
  }

  function startActorMoveAnimation(fromCoord, toCoord) {
    if (!fromCoord || !toCoord || sameCoord(fromCoord, toCoord)) return;
    const dx = toCoord.x - fromCoord.x;
    const dy = toCoord.y - fromCoord.y;
    if (Math.abs(dx) >= Math.abs(dy) && dx !== 0) {
      state.actorFacing = dx > 0 ? 'right' : 'left';
    } else if (dy !== 0) {
      state.actorFacing = dy > 0 ? 'down' : 'up';
    }
    state.actorAnimation = {
      fromX: fromCoord.x,
      fromY: fromCoord.y,
      toX: toCoord.x,
      toY: toCoord.y,
      layer_key: toCoord.layer_key || fromCoord.layer_key || 'surface',
      startTime: performance.now(),
      duration: ACTOR_MOVE_ANIMATION_MS,
    };
  }

  function getPlayerSprite(timestamp) {
    const sprites = window.SPRITES || {};
    const frames = sprites.playerWalk && sprites.playerWalk[state.actorFacing || 'down'];
    if (!frames || !frames.length) return sprites.player;
    if (!state.actorAnimation) return frames[0];
    const elapsed = Math.max(0, timestamp - state.actorAnimation.startTime);
    return frames[1 + (Math.floor(elapsed / 72) % Math.max(1, frames.length - 1))] || frames[0];
  }

  function updateCameraForActor(actorCoord) {
    if (!actorCoord) return null;
    if (!state.cameraLockedToActor) {
      if (!state.cameraCoord || state.cameraCoord.layer_key !== actorCoord.layer_key) {
        state.cameraCoord = { ...actorCoord };
      }
      return state.cameraCoord;
    }
    if (!state.cameraCoord || state.cameraCoord.layer_key !== actorCoord.layer_key) {
      state.cameraCoord = { ...actorCoord };
      return state.cameraCoord;
    }

    const halfX = Math.max(2, Math.floor(viewportTilesX() / 2));
    const halfY = Math.max(2, Math.floor(viewportTilesY() / 2));
    const deadX = Math.max(2, halfX - 2);
    const deadY = Math.max(1, halfY - 2);
    let cameraX = state.cameraCoord.x;
    let cameraY = state.cameraCoord.y;

    if (actorCoord.x < cameraX - deadX) cameraX = actorCoord.x + deadX;
    else if (actorCoord.x > cameraX + deadX) cameraX = actorCoord.x - deadX;

    if (actorCoord.y < cameraY - deadY) cameraY = actorCoord.y + deadY;
    else if (actorCoord.y > cameraY + deadY) cameraY = actorCoord.y - deadY;

    state.cameraCoord = {
      x: Math.round(cameraX),
      y: Math.round(cameraY),
      layer_key: actorCoord.layer_key || 'surface',
    };
    return state.cameraCoord;
  }

  function centerCameraOnActor(lock = false) {
    const actorCoord = getActorCoord();
    if (!actorCoord) return;
    state.cameraCoord = { ...actorCoord };
    state.cameraLockedToActor = lock;
    updateFollowButton();
    renderMap();
  }

  function updateFollowButton() {
    if (!followBtn) return;
    followBtn.textContent = state.cameraLockedToActor ? '跟随：开' : '跟随：关';
    followBtn.classList.toggle('active', state.cameraLockedToActor);
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

  function directionBetween(from, to) {
    if (!from || !to) return null;
    const dx = to.x - from.x;
    const dy = to.y - from.y;
    if (dx === 0 && dy === -1) return 'up';
    if (dx === 0 && dy === 1) return 'down';
    if (dx === -1 && dy === 0) return 'left';
    if (dx === 1 && dy === 0) return 'right';
    return null;
  }

  function cellKey(x, y) {
    return `${x},${y}`;
  }

  function isCellWalkable(cell) {
    const f = cell && cell.fields ? cell.fields : {};
    const blocked = String(f.blocks_movement || '').toLowerCase();
    if (blocked === 'true' || blocked === '1' || blocked === 'yes') return false;
    return true;
  }

  function buildVisibleWalkableSet() {
    const walkable = new Set();
    if (!state.projection) return walkable;
    for (const cell of state.projection.visible_cells) {
      const f = cell.fields || {};
      const x = parseInt(f.x, 10);
      const y = parseInt(f.y, 10);
      if (Number.isNaN(x) || Number.isNaN(y) || !isCellWalkable(cell)) continue;
      walkable.add(cellKey(x, y));
    }
    return walkable;
  }

  function findPathTo(target) {
    const start = getActorCoord();
    if (!start || !target) return [];
    if (start.x === target.x && start.y === target.y) return [];

    const walkable = buildVisibleWalkableSet();
    const startKey = cellKey(start.x, start.y);
    const targetKey = cellKey(target.x, target.y);
    walkable.add(startKey);
    if (!walkable.has(targetKey)) return null;

    const queue = [{ x: start.x, y: start.y }];
    const cameFrom = new Map();
    cameFrom.set(startKey, null);
    const dirs = [
      { dx: 0, dy: -1 },
      { dx: 0, dy: 1 },
      { dx: -1, dy: 0 },
      { dx: 1, dy: 0 },
    ];

    for (let head = 0; head < queue.length; ++head) {
      const cur = queue[head];
      if (cur.x === target.x && cur.y === target.y) break;
      for (const d of dirs) {
        const nx = cur.x + d.dx;
        const ny = cur.y + d.dy;
        const nk = cellKey(nx, ny);
        if (!walkable.has(nk) || cameFrom.has(nk)) continue;
        cameFrom.set(nk, cellKey(cur.x, cur.y));
        queue.push({ x: nx, y: ny });
      }
    }

    if (!cameFrom.has(targetKey)) return null;
    const path = [];
    let currentKey = targetKey;
    while (currentKey && currentKey !== startKey) {
      const [x, y] = currentKey.split(',').map(Number);
      path.push({ x, y, layer_key: start.layer_key || 'surface' });
      currentKey = cameFrom.get(currentKey);
    }
    path.reverse();
    return path;
  }

  function commandLabel(cmd, fallback) {
    if (!cmd) return fallback || t('ui.unknown_cmd', '未知命令');
    return cmd.label_text || cmd.command_kind || cmd.option_id || fallback || t('ui.unknown_cmd', '未知命令');
  }

  function resultKindOf(resp) {
    const result = resp && resp.result ? resp.result : {};
    return String(result.result_kind || 'unknown').toLowerCase();
  }

  function resultText(kind) {
    const map = {
      succeeded: t('result.succeeded', '成功'),
      noop: t('result.noop', '无变化'),
      deferred: t('result.deferred', '已排队'),
      blocked: t('result.blocked', '被阻止'),
      failed: t('result.failed', '失败'),
      interrupted: t('result.interrupted', '被打断'),
      unknown: t('result.unknown', '未知结果'),
    };
    return map[kind] || kind;
  }

  // ---------------------------------------------------------------------------
  // 9. Submit flow
  // ---------------------------------------------------------------------------
  async function submitOption(optionId) {
    if (state.requestState !== 'idle') {
      toast(t('ui.request_pending', '请求处理中，请稍候'), 'warn');
      return { status: 'busy', resultKind: 'busy', failureReasons: [] };
    }
    const found = state.availableCommands.find(c => c.option_id === optionId);
    if (!found) {
      toast(t('ui.option_expired', '该选项已过期，正在刷新…'), 'warn');
      await doRefresh();
      return { status: 'expired', resultKind: 'expired', failureReasons: [] };
    }

    const label = commandLabel(found, optionId);
    setRequestState('submitting_command');
    log(t('ui.exec_doing', '正在执行') + `: ${label}`, 'info');
    try {
      const resp = await apiCommand(optionId);
      const outcome = applyCommandResponse(resp);
      if (outcome.status === 'needs_refresh') {
        log(t('ui.version_mismatch', '状态不同步，刷新后再确认') + `: ${label}`, 'warn');
        setRequestState('idle');
        await doRefresh();
        return outcome;
      }

      const kind = outcome.resultKind;
      if (kind === 'succeeded') {
        log(t('ui.exec_success', '执行成功') + `: ${label}`, 'success');
      } else if (kind === 'noop' || kind === 'deferred') {
        log(t('ui.exec_result', '执行结果') + `: ${label}（${resultText(kind)}）`, 'info');
      } else {
        const reason = outcome.failureReasons.length ? `: ${outcome.failureReasons.join(', ')}` : '';
        log(t('ui.exec_doing', '执行') + `${resultText(kind)}: ${label}${reason}`, kind === 'blocked' ? 'warn' : 'error');
      }
      appendEvents(resp.event_feed, resp.frontend_hints, resp.warning_keys);
      return outcome;
    } catch (err) {
      state.screenShake = 8;
      handleError(t('ui.command_submit_failed', '命令提交失败'), err);
      return { status: 'error', resultKind: 'failed', failureReasons: [err.message || 'error'] };
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
      if (force) log(t('ui.no_move_cmd', '没有可用移动命令') + `: ${directionText(direction)}`, 'warn');
      return;
    }
    state.lastDirectionSubmitTime = now;
    return await submitOption(optionId);
  }

  async function runPathTo(target) {
    if (state.pathRunning) {
      toast('正在寻路中', 'warn');
      return;
    }
    const path = findPathTo(target);
    state.pathTarget = target;
    state.activePath = path || [];
    renderMap();

    if (path === null) {
      toast('当前可见区域内没有可走路径', 'warn');
      log(`无法寻路到 ${target.x},${target.y}`, 'warn');
      return;
    }
    if (!path.length) {
      toast('已经在目标格', 'info');
      return;
    }

    state.pathRunning = true;
    log(`开始寻路到 ${target.x},${target.y}，共 ${path.length} 步`, 'info');
    try {
      while (state.activePath.length) {
        const current = getActorCoord();
        const next = state.activePath[0];
        const direction = directionBetween(current, next);
        if (!direction) {
          log('寻路中断：当前位置已变化，需要重新规划', 'warn');
          break;
        }
        const optionId = resolveMoveOption(direction);
        if (!optionId) {
          log(`寻路中断：后端当前不允许${directionText(direction)}`, 'warn');
          toast('路径被阻挡或需要重新探索', 'warn');
          break;
        }
        const outcome = await submitOption(optionId);
        if (!outcome || outcome.resultKind !== 'succeeded') {
          log('寻路中断：移动未成功', 'warn');
          break;
        }
        const after = getActorCoord();
        if (!after || after.x !== next.x || after.y !== next.y) {
          log('寻路中断：后端位置与路径不一致', 'warn');
          break;
        }
        const reached = state.activePath.shift();
        addPathPixelBurst(reached.x, reached.y);
        renderMap();
        await new Promise(resolve => setTimeout(resolve, 60));
      }
    } finally {
      state.pathRunning = false;
      if (!state.activePath.length) {
        state.pathTarget = null;
      }
      renderMap();
    }
  }

  function applyCommandResponse(resp) {
    state.sessionId = safe(resp.session_id, state.sessionId);
    const actorCoordBefore = getRenderActorCoord(performance.now()) || getActorCoord();

    const result = resp.result || {};
    const failureReasons = safe(result.failure_reason_keys, []);
    const outcome = {
      status: 'ok',
      resultKind: resultKindOf(resp),
      failureReasons,
    };

    const patch = resp.projection_patch;
    if (safe(resp.requires_full_refresh, false)) {
      log(t('ui.full_refresh_required', '后端要求全量刷新'), 'warn');
      outcome.status = 'needs_refresh';
      return outcome;
    }
    if (patch) {
      const baseVer = safe(patch.base_projection_version, 0);
      const localVer = state.projectionVersion;
      if (baseVer !== 0 && baseVer !== localVer) {
        log(t('ui.patch_mismatch', 'Patch 版本不匹配') + `，本地=${localVer}，补丁基线=${baseVer}`, 'warn');
        outcome.status = 'needs_refresh';
        return outcome;
      }
      if (!applyPatch(patch)) {
        log(t('ui.missing_projection', '本地缺少世界投影，执行全量刷新'), 'warn');
        outcome.status = 'needs_refresh';
        return outcome;
      }
    }

    const actorCoordAfter = getActorCoord();
    if (outcome.resultKind === 'succeeded' && actorCoordBefore && actorCoordAfter && !sameCoord(actorCoordBefore, actorCoordAfter)) {
      startActorMoveAnimation(actorCoordBefore, actorCoordAfter);
    }

    state.projectionVersion = safe(resp.new_projection_version, state.projectionVersion);
    state.availableCommands = safe(resp.available_commands, []);
    state.eventFeed = safe(resp.event_feed, []);
    state.frontendHints = safe(resp.frontend_hints, []);
    state.warningKeys = safe(resp.warning_keys, []);

    if (failureReasons.length) {
      toast(t('ui.command_blocked', '命令被阻止'), 'warn');
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
      log(t('ui.bootstrap_ok', 'Bootstrap 成功'), 'success');
      saveSession();
    } catch (err) {
      if (err.status === 404 || err.message && err.message.includes('Failed to fetch')) {
        setConnectionMode('offline_error');
        log(t('ui.backend_unavailable', '无法连接后端，请确认服务已启动 (端口 1999)'), 'error');
        toast(t('ui.backend_offline', '无法连接后端'), 'error');
      } else {
        handleError(t('ui.bootstrap_failed', 'Bootstrap 失败'), err);
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
      log(t('ui.refresh_ok', '刷新成功'), 'success');
      saveSession();
    } catch (err) {
      handleError(t('ui.refresh_failed', '刷新失败'), err);
    } finally {
      setRequestState('idle');
      render();
    }
  }

  async function doReset() {
    if (state.requestState !== 'idle') return;
    if (!confirm(t('ui.reset_confirm', '确定要重置世界吗？当前会话将被清除。'))) return;
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
      log(t('ui.world_reset', '世界已重置'), 'success');
      saveSession();
    } catch (err) {
      handleError(t('ui.reset_failed', '重置失败'), err);
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
      log(`[${t('log.event', '事件')}] ${text}`, 'info');
    }
    for (const h of safe(hints, [])) {
      log(`[${t('log.hint', '提示')}] ${h.text || ''}`, 'info');
    }
    for (const w of safe(warnings, [])) {
      log(`[${t('log.warning', '警告')}] ${w}`, 'warn');
    }
  }

  // ---------------------------------------------------------------------------
  // 11. Rendering
  // ---------------------------------------------------------------------------
  const TILE_SIZE = 48;

  function tileSize() { return TILE_SIZE * state.zoom; }

  function viewportTilesX() { return Math.max(1, Math.floor(canvas.width / tileSize())); }
  function viewportTilesY() { return Math.max(1, Math.floor(canvas.height / tileSize())); }

  function clampZoom(value) {
    return Math.max(MIN_ZOOM, Math.min(MAX_ZOOM, value));
  }

  function updateZoomButtons() {
    if (zoomOutBtn) zoomOutBtn.disabled = state.zoom <= MIN_ZOOM + 0.001;
    if (zoomInBtn) zoomInBtn.disabled = state.zoom >= MAX_ZOOM - 0.001;
  }

  function setZoomAt(clientX, clientY, nextZoom) {
    const before = screenToWorld(clientX, clientY);
    const oldZoom = state.zoom;
    state.zoom = clampZoom(nextZoom);
    updateZoomButtons();
    if (Math.abs(state.zoom - oldZoom) < 0.001) return false;
    state.cameraLockedToActor = false;
    updateFollowButton();
    const after = screenToWorld(clientX, clientY);
    if (state.cameraCoord) {
      state.cameraCoord.x += before.x - after.x;
      state.cameraCoord.y += before.y - after.y;
    }
    renderMap();
    return true;
  }

  function visibleViewMin(cameraCoord = state.cameraCoord || getActorCoord()) {
    const halfX = Math.floor(viewportTilesX() / 2);
    const halfY = Math.floor(viewportTilesY() / 2);
    if (!cameraCoord) return { x: 0, y: 0 };
    return {
      x: cameraCoord.x - halfX,
      y: cameraCoord.y - halfY,
    };
  }

  function screenToWorld(clientX, clientY) {
    const rect = canvas.getBoundingClientRect();
    const sx = clientX - rect.left;
    const sy = clientY - rect.top;
    const tile = tileSize();
    const viewMin = visibleViewMin();
    return {
      x: Math.floor(viewMin.x + sx / tile),
      y: Math.floor(viewMin.y + sy / tile),
      sx,
      sy,
    };
  }

  function worldToScreen(x, y, viewMinX = state.viewMinX || 0, viewMinY = state.viewMinY || 0) {
    const tile = tileSize();
    return {
      x: (x - viewMinX) * tile,
      y: (y - viewMinY) * tile,
    };
  }

  function resizeCanvas() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
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
      summary.push(t('layer', '层') + ': ' + state.projection.active_layer_key);
      summary.push(t('cells', '格子') + ': ' + state.projection.visible_cells.length);
      summary.push(t('entities', '实体') + ': ' + state.projection.visible_entities.length);
      summary.push(t('inventory', '背包') + ': ' + state.projection.inventories.length);
    }
    const coord = getActorCoord();
    if (coord) summary.push(t('coord', '坐标') + `: ${coord.x}, ${coord.y}`);
    summary.push(t('sync', '同步') + ': ' + state.syncState);
    if (state.requestState !== 'idle') summary.push(t('request', '请求') + ': ' + state.requestState);
    questBody.textContent = summary.join(' | ') || t('ui.loading', '等待后端数据…');
  }

  function renderMap(timestamp = performance.now()) {
    state.lastTimestamp = timestamp;
    const w = canvas.width;
    const h = canvas.height;
    const tile = tileSize();

    ctx.save();
    if (state.screenShake > 0) {
      const sx = (Math.random() - 0.5) * state.screenShake;
      const sy = (Math.random() - 0.5) * state.screenShake;
      ctx.translate(sx, sy);
      state.screenShake *= 0.85;
      if (state.screenShake < 0.5) state.screenShake = 0;
    }

    ctx.clearRect(-8, -8, w + 16, h + 16);

    if (!state.projection) {
      ctx.fillStyle = '#091423';
      ctx.fillRect(0, 0, w, h);
      ctx.fillStyle = '#475569';
      ctx.font = '10px monospace';
      ctx.textAlign = 'center';
      ctx.fillText(t('ui.loading', '等待世界投影…'), w / 2, h / 2);
      ctx.restore();
      return;
    }

    const cells = state.projection.visible_cells;
    const entities = state.projection.visible_entities;
    const logicActorCoord = getActorCoord();
    const renderActorCoord = getRenderActorCoord(timestamp) || logicActorCoord;
    const cameraCoord = updateCameraForActor(logicActorCoord) || logicActorCoord || renderActorCoord;

    if (!cells.length && !entities.length) {
      ctx.fillStyle = '#091423';
      ctx.fillRect(0, 0, w, h);
      ctx.fillStyle = '#475569';
      ctx.font = '10px monospace';
      ctx.textAlign = 'center';
      ctx.fillText(t('ui.empty_world', '世界投影为空'), w / 2, h / 2);
      ctx.font = '9px monospace';
      ctx.fillText(t('ui.empty_world_hint', '后端尚未返回可见格子和实体'), w / 2, h / 2 + 18);
      ctx.restore();
      return;
    }

    let { x: viewMinX, y: viewMinY } = visibleViewMin(cameraCoord);
    if (!cameraCoord) {
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
      state.cameraCoord = { x: centerX, y: centerY, layer_key: 'surface' };
      ({ x: viewMinX, y: viewMinY } = visibleViewMin(state.cameraCoord));
    }

    ctx.fillStyle = '#091423';
    ctx.fillRect(0, 0, w, h);

    for (const c of cells) {
      const f = c.fields || {};
      const x = parseInt(f.x, 10);
      const y = parseInt(f.y, 10);
      if (isNaN(x) || isNaN(y)) continue;
      const px = (x - viewMinX) * tile;
      const py = (y - viewMinY) * tile;
      if (px < -tile || py < -tile || px > w + tile || py > h + tile) continue;

      const terrain = f.terrain_key || f.terrain || 'unknown';
      const sprite = window.SPRITES && window.SPRITES[terrain];
      if (sprite) {
        ctx.drawImage(sprite, px, py, tile, tile);
      } else {
        ctx.fillStyle = terrainColor(terrain);
        ctx.fillRect(px, py, tile, tile);
      }

      ctx.strokeStyle = 'rgba(15, 23, 42, 0.18)';
      ctx.lineWidth = 1;
      ctx.strokeRect(px, py, tile, tile);
    }

    if (state.activePath.length || state.pathTarget) {
      ctx.save();
      const pixel = Math.max(3, Math.floor(tile / 8));
      const pulse = Math.floor(timestamp / 180) % 2;
      state.activePath.forEach((step, index) => {
        const p = worldToScreen(step.x, step.y, viewMinX, viewMinY);
        const cx = Math.round(p.x + tile / 2 - pixel / 2);
        const cy = Math.round(p.y + tile / 2 - pixel / 2);
        ctx.fillStyle = ((index + pulse) % 2 === 0) ? '#38bdf8' : '#facc15';
        ctx.fillRect(cx, cy, pixel, pixel);
        ctx.fillStyle = 'rgba(224, 242, 254, 0.65)';
        ctx.fillRect(cx + pixel, cy, pixel, pixel);
        if (index > 0) {
          const prev = state.activePath[index - 1];
          const pp = worldToScreen(prev.x, prev.y, viewMinX, viewMinY);
          const x1 = Math.round(pp.x + tile / 2);
          const y1 = Math.round(pp.y + tile / 2);
          const x2 = Math.round(p.x + tile / 2);
          const y2 = Math.round(p.y + tile / 2);
          ctx.fillStyle = 'rgba(56, 189, 248, 0.55)';
          if (x1 === x2) {
            const y = Math.min(y1, y2) + pixel;
            ctx.fillRect(x1 - Math.floor(pixel / 2), y, pixel, Math.max(pixel, Math.abs(y2 - y1) - pixel * 2));
          } else if (y1 === y2) {
            const x = Math.min(x1, x2) + pixel;
            ctx.fillRect(x, y1 - Math.floor(pixel / 2), Math.max(pixel, Math.abs(x2 - x1) - pixel * 2), pixel);
          }
        }
      });
      if (state.pathTarget) {
        const p = worldToScreen(state.pathTarget.x, state.pathTarget.y, viewMinX, viewMinY);
        ctx.fillStyle = 'rgba(250, 204, 21, 0.28)';
        ctx.fillRect(Math.round(p.x + pixel), Math.round(p.y + pixel), Math.round(tile - pixel * 2), pixel);
        ctx.fillRect(Math.round(p.x + pixel), Math.round(p.y + tile - pixel * 2), Math.round(tile - pixel * 2), pixel);
        ctx.fillRect(Math.round(p.x + pixel), Math.round(p.y + pixel), pixel, Math.round(tile - pixel * 2));
        ctx.fillRect(Math.round(p.x + tile - pixel * 2), Math.round(p.y + pixel), pixel, Math.round(tile - pixel * 2));
      }
      ctx.restore();
    }

    const actorKey = state.projection.actor_key;
    const sortedEntities = [...entities].sort((a, b) => {
      const ay = parseInt((a.fields || {}).y, 10) || 0;
      const by = parseInt((b.fields || {}).y, 10) || 0;
      return ay - by;
    });
    for (const e of sortedEntities) {
      const f = e.fields || {};
      let x = parseInt(f.x, 10);
      let y = parseInt(f.y, 10);
      if (isNaN(x) || isNaN(y)) continue;
      const isActor = f.actor_key === actorKey || e.entity_id === actorKey || e.entity_id.startsWith(actorKey);
      if (isActor && renderActorCoord) {
        x = renderActorCoord.x;
        y = renderActorCoord.y;
      }
      const px = (x - viewMinX) * tile;
      const py = (y - viewMinY) * tile;
      if (px < -tile || py < -tile || px > w + tile || py > h + tile) continue;

      ctx.fillStyle = 'rgba(0,0,0,0.18)';
      ctx.beginPath();
      ctx.ellipse(px + tile / 2, py + tile - 3, tile * 0.28, 4, 0, 0, Math.PI * 2);
      ctx.fill();

      const entityKey = f.entity_key || f.entity_kind || f.kind || f.object_key || '';
      const movingEntity = isActor || /beast|wolf|npc|companion/i.test(entityKey);
      const breathe = movingEntity ? Math.sin(timestamp * 0.003 + x * 0.7 + y * 0.4) * 1.2 : 0;
      const drawY = py + breathe;

      if (state.selectedEntityId === e.entity_id) {
        const pulse = 0.5 + 0.3 * Math.sin(timestamp * 0.005);
        ctx.save();
        ctx.strokeStyle = '#0ea5e9';
        ctx.lineWidth = 2;
        ctx.globalAlpha = pulse;
        ctx.setLineDash([4, 3]);
        ctx.beginPath();
        ctx.ellipse(px + tile / 2, py + tile - 4, tile * 0.35, 5, 0, 0, Math.PI * 2);
        ctx.stroke();
        ctx.setLineDash([]);
        ctx.globalAlpha = 1;
        ctx.restore();
      }

      if (isActor) {
        const sprite = getPlayerSprite(timestamp);
        if (sprite) ctx.drawImage(sprite, px, drawY, tile, tile);
        else drawEntityFallback(ctx, px + tile / 2, drawY + tile / 2, tile * 0.7, f, true);
      } else {
        const kind = entityKey || f.display_name_key || 'unknown';
        let spriteKey = null;
        if (kind.includes('tree') || kind.includes('Tree')) spriteKey = 'tree';
        else if (kind.includes('beast') || kind.includes('wolf') || kind.includes('Beast') || kind.includes('Wolf')) spriteKey = 'wolf';
        else if (kind.includes('stone') || kind.includes('Stone') || kind.includes('rock') || kind.includes('Rock')) spriteKey = 'loose_stone';
        else if (kind.includes('berry') || kind.includes('Berry') || kind.includes('bush') || kind.includes('Bush')) spriteKey = 'berry';
        else if (kind.includes('npc') || kind.includes('NPC') || kind.includes('companion') || kind.includes('Companion')) spriteKey = 'npc';
        else if (kind.includes('campfire') || kind.includes('Campfire') || kind.includes('fire') || kind.includes('Fire')) spriteKey = 'campfire';
        else spriteKey = 'unknown';

        const sprite = window.SPRITES && window.SPRITES[spriteKey];
        if (sprite) ctx.drawImage(sprite, px, drawY, tile, tile);
        else drawEntityFallback(ctx, px + tile / 2, drawY + tile / 2, tile * 0.7, f, false);
      }
    }

    state.viewMinX = viewMinX;
    state.viewMinY = viewMinY;
    renderPathParticles(timestamp, viewMinX, viewMinY);
    renderFloatingTexts(timestamp, viewMinX, viewMinY);

    ctx.restore();
  }

  function terrainColor(key) {
    const map = {
      grass: '#285c31',
      forest: '#1c4b31',
      water: '#24558c',
      sand: '#82663c',
      mountain: '#555968',
      plain: '#316739',
      // P57 noise terrain colors
      stone_field: '#67685a',
      water_edge: '#26705a',
      blocked: '#6b3436',
      deep_water: '#163a72',
      unknown: '#263956',
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
      commandList.innerHTML = '<div style="color:var(--text-dim);font-size:12px;padding:8px 0;">' + t('ui.no_commands', '暂无可用命令') + '</div>';
      return;
    }
    // Separate move and non-move commands
    const otherCmds = [];
    for (const c of cmds) {
      if (c.command_kind !== 'Move' && c.command_kind !== 'move') otherCmds.push(c);
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
  // 12. Mobile joystick
  // ---------------------------------------------------------------------------
  let directionHoldTimer = null;
  let activeDirectionPointerId = null;
  const JOYSTICK_DEAD_ZONE = 18;
  const JOYSTICK_MAX_RADIUS = 48;

  function directionText(direction) {
    const map = { up: t('dir_up', '向上'), down: t('dir_down', '向下'), left: t('dir_left', '向左'), right: t('dir_right', '向右') };
    return map[direction] || direction;
  }

  function stopDirectionHold() {
    if (directionHoldTimer) {
      clearInterval(directionHoldTimer);
      directionHoldTimer = null;
    }
    if (joystickBase) {
      joystickBase.classList.remove('active');
    }
    if (joystickThumb) {
      joystickThumb.style.transform = 'translate(-50%, -50%)';
    }
    activeDirectionPointerId = null;
    state.pendingDirection = null;
  }

  function setJoystickDirection(direction) {
    if (!direction || direction === state.pendingDirection) return;
    state.pendingDirection = direction;
    log(t('ui.move_input', '移动输入') + `: ${directionText(direction)}`, 'info');
    submitDirection(direction, true);
  }

  function startDirectionHold(direction) {
    if (directionHoldTimer) clearInterval(directionHoldTimer);
    setJoystickDirection(direction);
    directionHoldTimer = setInterval(() => {
      if (state.pendingDirection) {
        submitDirection(state.pendingDirection);
      }
    }, DIRECTION_REPEAT_MS);
  }

  function directionFromJoystick(dx, dy) {
    const distance = Math.sqrt(dx * dx + dy * dy);
    if (distance < JOYSTICK_DEAD_ZONE) return null;
    if (Math.abs(dx) >= Math.abs(dy)) {
      return dx >= 0 ? 'right' : 'left';
    }
    return dy >= 0 ? 'down' : 'up';
  }

  function updateJoystickFromPoint(clientX, clientY) {
    if (!joystickBase || !joystickThumb) return null;
    const rect = joystickBase.getBoundingClientRect();
    const centerX = rect.left + rect.width / 2;
    const centerY = rect.top + rect.height / 2;
    let dx = clientX - centerX;
    let dy = clientY - centerY;
    const distance = Math.sqrt(dx * dx + dy * dy);
    if (distance > JOYSTICK_MAX_RADIUS) {
      const scale = JOYSTICK_MAX_RADIUS / distance;
      dx *= scale;
      dy *= scale;
    }
    joystickThumb.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;
    return directionFromJoystick(dx, dy);
  }

  function initJoystick() {
    if (!joystickBase || !joystickThumb) {
      log(t('ui.joystick_init_failed', '移动摇杆初始化失败'), 'error');
      return;
    }

    joystickBase.addEventListener('contextmenu', e => e.preventDefault());

    const begin = e => {
      e.preventDefault();
      joystickBase.classList.add('active');
      if (e.pointerId !== undefined) {
        activeDirectionPointerId = e.pointerId;
        joystickBase.setPointerCapture(e.pointerId);
      }
      const direction = updateJoystickFromPoint(e.clientX, e.clientY);
      if (direction) startDirectionHold(direction);
    };

    const move = e => {
      if (activeDirectionPointerId !== null && e.pointerId !== activeDirectionPointerId) return;
      e.preventDefault();
      const direction = updateJoystickFromPoint(e.clientX, e.clientY);
      if (direction) {
        if (!directionHoldTimer) startDirectionHold(direction);
        else setJoystickDirection(direction);
      } else {
        state.pendingDirection = null;
      }
    };

    const end = e => {
      if (activeDirectionPointerId !== null && e.pointerId !== undefined && e.pointerId !== activeDirectionPointerId) return;
      e.preventDefault();
      if (e.pointerId !== undefined && joystickBase.hasPointerCapture && joystickBase.hasPointerCapture(e.pointerId)) {
        joystickBase.releasePointerCapture(e.pointerId);
      }
      stopDirectionHold();
    };

    if (window.PointerEvent) {
      joystickBase.addEventListener('pointerdown', begin);
      joystickBase.addEventListener('pointermove', move);
      joystickBase.addEventListener('pointerup', end);
      joystickBase.addEventListener('pointercancel', end);
    } else {
      joystickBase.addEventListener('touchstart', e => {
        if (!e.touches.length) return;
        begin(e.touches[0]);
      }, { passive: false });
      joystickBase.addEventListener('touchmove', e => {
        if (!e.touches.length) return;
        move(e.touches[0]);
      }, { passive: false });
      joystickBase.addEventListener('touchend', e => end(e.changedTouches[0] || e), { passive: false });
      joystickBase.addEventListener('touchcancel', e => end(e.changedTouches[0] || e), { passive: false });
      joystickBase.addEventListener('mousedown', begin);
      window.addEventListener('mousemove', e => {
        if (directionHoldTimer) move(e);
      });
      window.addEventListener('mouseup', end);
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
  // 14. Animation loop
  // ---------------------------------------------------------------------------
  function gameLoop(timestamp) {
    state.lastTimestamp = timestamp;
    if (state.actorAnimation || state.cameraDrag || state.activePath.length || state.pathParticles.length || timestamp - state.lastMapRenderTime >= MAP_FRAME_INTERVAL_MS || state.screenShake > 0 || state.floatingTexts.length > 0) {
      state.lastMapRenderTime = timestamp;
      renderMap(timestamp);
    }
    requestAnimationFrame(gameLoop);
  }

  // ---------------------------------------------------------------------------
  // 15. Event bindings
  // ---------------------------------------------------------------------------
  resetBtn.addEventListener('click', doReset);
  window.addEventListener('resize', resizeCanvas);

  if (followBtn) {
    followBtn.addEventListener('click', () => {
      if (state.cameraLockedToActor) {
        state.cameraLockedToActor = false;
        updateFollowButton();
        renderMap();
      } else {
        centerCameraOnActor(true);
      }
    });
    updateFollowButton();
  }

  if (zoomInBtn) {
    zoomInBtn.addEventListener('click', () => {
      setZoomAt(canvas.width / 2, canvas.height / 2, state.zoom * 1.18);
    });
  }

  if (zoomOutBtn) {
    zoomOutBtn.addEventListener('click', () => {
      setZoomAt(canvas.width / 2, canvas.height / 2, state.zoom / 1.18);
    });
  }

  updateZoomButtons();

  function selectEntityAt(worldX, worldY) {
    if (!state.projection) return false;
    let clicked = null;
    let minDist = Infinity;
    let clickEx = 0, clickEy = 0;
    for (const ent of state.projection.visible_entities) {
      const f = ent.fields || {};
      const ex = parseInt(f.x, 10);
      const ey = parseInt(f.y, 10);
      if (isNaN(ex) || isNaN(ey)) continue;
      const dx = worldX - ex;
      const dy = worldY - ey;
      const dist = Math.sqrt(dx * dx + dy * dy);
      if (dist < 1.2 && dist < minDist) {
        minDist = dist;
        clicked = ent;
        clickEx = ex;
        clickEy = ey;
      }
    }

    if (clicked) {
      state.selectedEntityId = clicked.entity_id;
      const f = clicked.fields || {};
      const name = (typeof tEntity === 'function') ? tEntity(f) : (f.display_name || f.name || f.entity_key || f.object_key || 'Entity');
      addFloatingText(clickEx, clickEy - 0.5, name, '#0ea5e9');
      renderMap();
      return true;
    }
    state.selectedEntityId = null;
    renderMap();
    return false;
  }

  function isDoubleTapCell(world) {
    const now = Date.now();
    const last = state.lastTap;
    state.lastTap = { x: world.x, y: world.y, time: now };
    return !!last && last.x === world.x && last.y === world.y && now - last.time <= 420;
  }

  canvas.addEventListener('pointerdown', (e) => {
    if (!state.projection) return;
    if (e.pointerType === 'touch' && e.isPrimary === false) return;
    const world = screenToWorld(e.clientX, e.clientY);
    state.cameraDrag = {
      pointerId: e.pointerId,
      startX: e.clientX,
      startY: e.clientY,
      lastX: e.clientX,
      lastY: e.clientY,
      moved: false,
      startWorld: world,
    };
    if (canvas.setPointerCapture) canvas.setPointerCapture(e.pointerId);
  });

  canvas.addEventListener('pointermove', (e) => {
    const drag = state.cameraDrag;
    if (!drag || drag.pointerId !== e.pointerId || !state.cameraCoord) return;
    const dx = e.clientX - drag.lastX;
    const dy = e.clientY - drag.lastY;
    const totalDx = e.clientX - drag.startX;
    const totalDy = e.clientY - drag.startY;
    if (Math.hypot(totalDx, totalDy) > 5) drag.moved = true;
    if (drag.moved) {
      state.cameraLockedToActor = false;
      updateFollowButton();
      const tile = tileSize();
      state.cameraCoord = {
        x: state.cameraCoord.x - dx / tile,
        y: state.cameraCoord.y - dy / tile,
        layer_key: state.cameraCoord.layer_key || 'surface',
      };
      drag.lastX = e.clientX;
      drag.lastY = e.clientY;
      renderMap();
    }
  });

  canvas.addEventListener('pointerup', (e) => {
    const drag = state.cameraDrag;
    if (!drag || drag.pointerId !== e.pointerId) return;
    if (canvas.releasePointerCapture && canvas.hasPointerCapture && canvas.hasPointerCapture(e.pointerId)) {
      canvas.releasePointerCapture(e.pointerId);
    }
    state.cameraDrag = null;
    if (drag.moved) return;

    const world = screenToWorld(e.clientX, e.clientY);
    if (isDoubleTapCell(world)) {
      state.cameraLockedToActor = false;
      updateFollowButton();
      runPathTo({ x: world.x, y: world.y, layer_key: (getActorCoord() || {}).layer_key || 'surface' });
    } else {
      selectEntityAt(world.x, world.y);
    }
  });

  canvas.addEventListener('pointercancel', (e) => {
    if (state.cameraDrag && state.cameraDrag.pointerId === e.pointerId) {
      state.cameraDrag = null;
    }
  });

  canvas.addEventListener('wheel', (e) => {
    e.preventDefault();
    const factor = e.deltaY < 0 ? 1.12 : 0.9;
    setZoomAt(e.clientX, e.clientY, state.zoom * factor);
  }, { passive: false });

  canvas.addEventListener('touchstart', (e) => {
    if (e.touches.length === 2) {
      const a = e.touches[0];
      const b = e.touches[1];
      state.cameraDrag = null;
      state.pinchZoom = {
        distance: Math.hypot(a.clientX - b.clientX, a.clientY - b.clientY),
        zoom: state.zoom,
        centerX: (a.clientX + b.clientX) / 2,
        centerY: (a.clientY + b.clientY) / 2,
      };
    }
  }, { passive: false });

  canvas.addEventListener('touchmove', (e) => {
    if (e.touches.length !== 2 || !state.pinchZoom) return;
    e.preventDefault();
    const a = e.touches[0];
    const b = e.touches[1];
    const distance = Math.hypot(a.clientX - b.clientX, a.clientY - b.clientY);
    if (state.pinchZoom.distance <= 0) return;
    const centerX = (a.clientX + b.clientX) / 2;
    const centerY = (a.clientY + b.clientY) / 2;
    setZoomAt(centerX, centerY, state.pinchZoom.zoom * (distance / state.pinchZoom.distance));
  }, { passive: false });

  canvas.addEventListener('touchend', (e) => {
    if (e.touches.length < 2) state.pinchZoom = null;
  }, { passive: false });

  canvas.addEventListener('touchcancel', () => {
    state.pinchZoom = null;
  }, { passive: false });

  // ---------------------------------------------------------------------------
  // 15. Startup
  // ---------------------------------------------------------------------------
  async function start() {
    loadSession();
    setConnectionMode('unknown');
    resizeCanvas();
    initJoystick();
    initKeyboard();
    log(t('ui.client_boot', '先驱者客户端启动…'));
    requestAnimationFrame(gameLoop);
    await doBootstrap();
  }

  start();
})();
