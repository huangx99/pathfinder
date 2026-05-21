/**
 * 溯境 Pathfinder - 前端客户端
 * 纯原生 JavaScript，无构建工具，无外部依赖
 */

// ============================================
// 1. 常量与配置
// ============================================
const TILE_SIZE = 36;
const MINIMAP_TILE_SIZE = 2;
const CAMERA_LERP = 0.08;

// 地形颜色映射
const TERRAIN_COLORS = {
  grass:        { base: '#4ade80', dark: '#16a34a', dots: '#15803d' },
  sand:         { base: '#fde68a', dark: '#d97706', dots: '#b45309' },
  water:        { base: '#38bdf8', dark: '#0284c7', dots: null },
  deep_water:   { base: '#0ea5e9', dark: '#0369a1', dots: null },
  dirt:         { base: '#a16207', dark: '#713f12', dots: '#854d0e' },
  stone:        { base: '#9ca3af', dark: '#4b5563', dots: '#6b7280' },
  dark:         { base: '#334155', dark: '#1e293b', dots: '#475569' },
  floor:        { base: '#d4d4d8', dark: '#a1a1aa', dots: null },
  forest:       { base: '#15803d', dark: '#14532d', dots: '#166534' },
  snow:         { base: '#f1f5f9', dark: '#cbd5e1', dots: '#94a3b8' },
  lava:         { base: '#ef4444', dark: '#b91c1c', dots: '#f97316' },
};

// 实体颜色映射
const ENTITY_COLORS = {
  player:       { body: '#3b82f6', head: '#fca5a5', legs: '#1e293b', accent: '#fbbf24' },
  wolf:         { body: '#94a3b8', head: '#64748b', legs: '#475569', accent: '#ef4444' },
  bear:         { body: '#854d0e', head: '#713f12', legs: '#451a03', accent: '#000000' },
  tree:         { trunk: '#854d0e', leaf1: '#166534', leaf2: '#15803d', leaf3: '#22c55e' },
  rock:         { body: '#9ca3af', dark: '#6b7280', highlight: '#d1d5db' },
  berry_bush:   { body: '#166534', berry: '#ef4444', dark: '#14532d' },
  mushroom:     { cap: '#dc2626', stem: '#fde68a', spot: '#fbbf24' },
  coin:         { face: '#eab308', edge: '#a16207', shine: '#fbbf24' },
  herb:         { stem: '#22c55e', leaf: '#4ade80', flower: '#a855f7' },
  campfire:     { wood: '#92400e', flame1: '#f97316', flame2: '#fbbf24', flame3: '#ef4444' },
  npc:          { body: '#a855f7', head: '#fcd34d', legs: '#7e22ce', accent: '#ffffff' },
  portal:       { outer: '#a855f7', inner: '#3b82f6', glow: '#c084fc' },
};

// ============================================
// 2. 状态管理
// ============================================
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
  syncState: 'bootstrapping',
  lastResponse: null,
  debugResponses: { bootstrap: null, command: null, refresh: null },
  mockMode: false,
  selectedEntityId: null,
  pendingCommand: false,
  camera: { x: 0, y: 0, targetX: 0, targetY: 0 },
  floatingTexts: [],
  screenShake: 0,
  animFrame: 0,
  lastTimestamp: 0,
};

// ============================================
// 3. 工具函数
// ============================================
function generateId(prefix) {
  const chars = 'abcdefghijklmnopqrstuvwxyz0123456789';
  let result = prefix + '_';
  for (let i = 0; i < 12; i++) {
    result += chars.charAt(Math.floor(Math.random() * chars.length));
  }
  return result;
}

function getOrCreateStorage(key, generator) {
  let val = localStorage.getItem(key);
  if (!val) {
    val = generator();
    localStorage.setItem(key, val);
  }
  return val;
}

function safeArray(arr) {
  return Array.isArray(arr) ? arr : [];
}

function safeString(str) {
  return typeof str === 'string' ? str : '';
}

function safeBool(val) {
  return typeof val === 'boolean' ? val : false;
}

function safeNumber(num) {
  return typeof num === 'number' ? num : 0;
}

function nowTimestamp() {
  const d = new Date();
  return d.toTimeString().split(' ')[0];
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
    selected_entity_id: state.selectedEntityId || '',
    selected_inventory_id: '',
    selected_area_id: '',
  };
}

// ============================================
// 4. API 适配器
// ============================================
async function postJson(path, body) {
  const response = await fetch(path, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
    },
    body: JSON.stringify(body),
  });
  if (!response.ok) {
    const err = new Error(`${path} failed: ${response.status}`);
    err.status = response.status;
    throw err;
  }
  return response.json();
}

async function bootstrap() {
  const payload = {
    client_id: state.clientId,
    session_id: state.sessionId,
    requested_actor_key: 'player',
    requested_layer_key: 'surface',
    client_protocol_version: 1,
    create_if_missing: true,
    dev_reset_if_allowed: false,
  };
  const data = await postJson('/api/client/bootstrap', payload);
  state.debugResponses.bootstrap = data;
  return data;
}

async function submitOption(optionId) {
  state.clientSequence += 1;
  const payload = {
    session_id: state.sessionId,
    client_id: state.clientId,
    client_sequence: state.clientSequence,
    known_projection_version: state.projectionVersion,
    submit_mode: 'option_id',
    option_id: optionId,
    selection_context: emptySelectionContext(),
  };
  const data = await postJson('/api/client/command', payload);
  state.debugResponses.command = data;
  return data;
}

async function refresh() {
  const payload = {
    session_id: state.sessionId,
    client_id: state.clientId,
    known_projection_version: state.projectionVersion,
    requested_scopes: ['full_safe_world'],
    requested_layer_key: 'surface',
    viewport_center_x: 0,
    viewport_center_y: 0,
  };
  const data = await postJson('/api/client/refresh', payload);
  state.debugResponses.refresh = data;
  return data;
}

async function resetWorld() {
  const payload = {
    session_id: state.sessionId,
    client_id: state.clientId,
    confirmed: true,
  };
  const data = await postJson('/api/client/reset', payload);
  return data;
}

// ============================================
// 5. Mock 模式数据
// ============================================
let mockPlayerX = 2;
let mockPlayerY = 2;
let mockProjVersion = 1;

function getMockCells() {
  const cells = [];
  for (let x = 0; x < 7; x++) {
    for (let y = 0; y < 7; y++) {
      let terrain = 'grass';
      if (x === 3 && y === 3) terrain = 'water';
      else if (x === 4 && y === 3) terrain = 'water';
      else if (x === 3 && y === 4) terrain = 'water';
      else if (x > 4 || y > 4) terrain = 'sand';
      else if (x === 1 && y === 1) terrain = 'forest';
      cells.push({
        cell_id: `cell_${x}_${y}`,
        x: x,
        y: y,
        terrain_key: terrain,
        terrain_display: terrain,
        layer_key: 'surface',
      });
    }
  }
  return cells;
}

function getMockEntities() {
  const entities = [
    {
      entity_id: 'ent_player',
      entity_key: 'player',
      object_key: 'actor',
      display_name: 'Player',
      x: mockPlayerX,
      y: mockPlayerY,
      hp: 100,
      max_hp: 100,
      facing: 'down',
    },
    {
      entity_id: 'ent_tree_1',
      entity_key: 'tree',
      object_key: 'tree',
      display_name: 'Pine Tree',
      x: 1,
      y: 1,
      hp: 30,
      max_hp: 30,
    },
    {
      entity_id: 'ent_tree_2',
      entity_key: 'tree',
      object_key: 'tree',
      display_name: 'Oak Tree',
      x: 5,
      y: 1,
      hp: 40,
      max_hp: 40,
    },
    {
      entity_id: 'ent_rock_1',
      entity_key: 'rock',
      object_key: 'rock',
      display_name: 'Boulder',
      x: 0,
      y: 3,
      hp: 50,
      max_hp: 50,
    },
  ];
  return entities;
}

function getMockCommands() {
  const commands = [
    {
      option_id: 'opt_wait',
      label_text: 'Wait',
      enabled: true,
      disabled_reason_text: '',
    },
    {
      option_id: 'opt_look',
      label_text: 'Look Around',
      enabled: true,
      disabled_reason_text: '',
    },
    {
      option_id: 'opt_move_north',
      label_text: 'Move North',
      enabled: mockPlayerY > 0,
      disabled_reason_text: 'Blocked',
    },
    {
      option_id: 'opt_move_south',
      label_text: 'Move South',
      enabled: mockPlayerY < 6,
      disabled_reason_text: 'Blocked',
    },
    {
      option_id: 'opt_move_east',
      label_text: 'Move East',
      enabled: mockPlayerX < 6,
      disabled_reason_text: 'Blocked',
    },
    {
      option_id: 'opt_move_west',
      label_text: 'Move West',
      enabled: mockPlayerX > 0,
      disabled_reason_text: 'Blocked',
    },
    {
      option_id: 'opt_gather',
      label_text: 'Gather',
      enabled: (mockPlayerX === 1 && mockPlayerY === 1) || (mockPlayerX === 5 && mockPlayerY === 1),
      disabled_reason_text: 'No resources nearby',
    },
  ];
  return commands;
}

function getMockBootstrap() {
  mockProjVersion = 1;
  return {
    session_id: state.sessionId,
    server_protocol_version: 1,
    projection_version: mockProjVersion,
    full_projection: {
      projection_version: mockProjVersion,
      actor_key: 'player',
      active_layer_key: 'surface',
      visible_cells: getMockCells(),
      visible_entities: getMockEntities(),
      inventories: [],
      knowledge: [],
      area_effects: [],
      safe_summary_keys: ['mock_mode'],
    },
    available_commands: getMockCommands(),
    event_feed: [
      { event_kind: 'system', message: 'Welcome to Pathfinder Mock Mode.', timestamp: nowTimestamp() },
      { event_kind: 'hint', message: 'Try moving around and gathering resources.', timestamp: nowTimestamp() },
    ],
    frontend_hints: ['Mock mode: actions are simulated locally.'],
    warning_keys: [],
  };
}

function getMockCommandResponse(optionId) {
  mockProjVersion += 1;
  let eventMessage = 'You wait.';

  if (optionId === 'opt_move_north' && mockPlayerY > 0) {
    mockPlayerY -= 1;
    eventMessage = 'You move north.';
  } else if (optionId === 'opt_move_south' && mockPlayerY < 6) {
    mockPlayerY += 1;
    eventMessage = 'You move south.';
  } else if (optionId === 'opt_move_east' && mockPlayerX < 6) {
    mockPlayerX += 1;
    eventMessage = 'You move east.';
  } else if (optionId === 'opt_move_west' && mockPlayerX > 0) {
    mockPlayerX -= 1;
    eventMessage = 'You move west.';
  } else if (optionId === 'opt_gather') {
    eventMessage = 'You gather some wood.';
  } else if (optionId === 'opt_look') {
    eventMessage = 'You look around. You see trees and rocks.';
  }

  // Update player entity position
  const entities = getMockEntities();
  const player = entities.find(e => e.entity_id === 'ent_player');
  if (player) {
    player.x = mockPlayerX;
    player.y = mockPlayerY;
  }

  return {
    session_id: state.sessionId,
    accepted_client_sequence: state.clientSequence,
    base_projection_version: state.projectionVersion,
    new_projection_version: mockProjVersion,
    requires_full_refresh: false,
    result: {
      command_id: 'cmd_mock_' + mockProjVersion,
      command_key: optionId.replace('opt_', ''),
      result_kind: 'succeeded',
      actor_key: 'player',
      target_ref: '',
      failure_reason_keys: [],
      warning_keys: [],
    },
    projection_patch: {
      base_projection_version: state.projectionVersion,
      new_projection_version: mockProjVersion,
      requires_full_refresh: false,
      changed_cells: [],
      changed_entities: [
        { op: 'replace', entity_id: 'ent_player', data: { ...player } },
      ],
      changed_inventories: [],
      changed_knowledge: [],
      changed_area_effects: [],
    },
    available_commands: getMockCommands(),
    event_feed: [
      { event_kind: 'action', message: eventMessage, timestamp: nowTimestamp() },
    ],
    experiences: [],
    frontend_hints: [],
    warning_keys: [],
  };
}

// ============================================
// 6. Canvas 渲染引擎
// ============================================
let canvas, ctx, minimapCanvas, minimapCtx;
let canvasW, canvasH;

function initCanvas() {
  canvas = document.getElementById('gameCanvas');
  ctx = canvas.getContext('2d');
  ctx.imageSmoothingEnabled = false;

  minimapCanvas = document.getElementById('minimapCanvas');
  minimapCtx = minimapCanvas.getContext('2d');
  minimapCtx.imageSmoothingEnabled = false;

  resizeCanvas();
  window.addEventListener('resize', resizeCanvas);
}

function resizeCanvas() {
  canvasW = window.innerWidth;
  canvasH = window.innerHeight;
  canvas.width = canvasW;
  canvas.height = canvasH;

  minimapCanvas.width = 150;
  minimapCanvas.height = 150;
}

function worldToScreen(wx, wy) {
  return {
    x: (wx * TILE_SIZE) - state.camera.x + canvasW / 2,
    y: (wy * TILE_SIZE) - state.camera.y + canvasH / 2,
  };
}

function screenToWorld(sx, sy) {
  return {
    x: (sx + state.camera.x - canvasW / 2) / TILE_SIZE,
    y: (sy + state.camera.y - canvasH / 2) / TILE_SIZE,
  };
}

function getTerrainColor(terrainKey) {
  return TERRAIN_COLORS[terrainKey] || TERRAIN_COLORS.dark;
}

// 渲染地形 tile
function renderTile(wx, wy, terrainKey, timestamp) {
  const pos = worldToScreen(wx, wy);
  const x = pos.x;
  const y = pos.y;

  // 屏幕外裁剪
  if (x < -TILE_SIZE || x > canvasW + TILE_SIZE || y < -TILE_SIZE || y > canvasH + TILE_SIZE) {
    return;
  }

  const colors = getTerrainColor(terrainKey);

  // 基础填充
  ctx.fillStyle = colors.base;
  ctx.fillRect(x, y, TILE_SIZE + 0.5, TILE_SIZE + 0.5);

  // 纹理点缀
  if (terrainKey === 'grass' || terrainKey === 'forest') {
    const seed = (wx * 374761 + wy * 668265) % 100;
    ctx.fillStyle = colors.dots || colors.dark;
    if (seed < 30) {
      ctx.fillRect(x + 6, y + 8, 2, 2);
    }
    if (seed < 15) {
      ctx.fillRect(x + 20, y + 18, 2, 2);
    }
    if (seed < 8) {
      ctx.fillRect(x + 14, y + 26, 2, 2);
    }
  } else if (terrainKey === 'sand') {
    const seed = (wx * 374761 + wy * 668265) % 100;
    ctx.fillStyle = colors.dots || colors.dark;
    if (seed < 20) {
      ctx.fillRect(x + 10, y + 14, 2, 2);
    }
  } else if (terrainKey === 'water' || terrainKey === 'deep_water') {
    // 水波纹动画
    const wave1 = Math.sin(timestamp * 0.002 + wx * 0.5 + wy * 0.3) * 2;
    const wave2 = Math.sin(timestamp * 0.003 + wx * 0.3 + wy * 0.7) * 1.5;
    ctx.fillStyle = 'rgba(255,255,255,0.15)';
    ctx.fillRect(x + 4, y + 12 + wave1, 8, 1.5);
    ctx.fillRect(x + 18, y + 20 + wave2, 10, 1);
  } else if (terrainKey === 'lava') {
    const pulse = Math.sin(timestamp * 0.004 + wx * wy) * 0.3;
    ctx.fillStyle = `rgba(251, 191, 36, ${0.2 + pulse})`;
    ctx.fillRect(x + 8, y + 20, 10, 3);
  }
}

// 渲染实体 - 像素风格
function renderEntity(entity, timestamp) {
  const pos = worldToScreen(entity.x, entity.y);
  const cx = pos.x + TILE_SIZE / 2;
  const cy = pos.y + TILE_SIZE / 2;

  // 屏幕外裁剪
  if (pos.x < -TILE_SIZE * 2 || pos.x > canvasW + TILE_SIZE * 2 ||
      pos.y < -TILE_SIZE * 2 || pos.y > canvasH + TILE_SIZE * 2) {
    return;
  }

  const key = entity.entity_key || '';
  const colors = ENTITY_COLORS[key] || ENTITY_COLORS.rock;
  const facing = entity.facing || 'down';
  const isSelected = entity.entity_id === state.selectedEntityId;
  const breathe = Math.sin(timestamp * 0.003 + (entity.x * 100) + (entity.y * 50)) * 0.5;

  ctx.save();

  // 选中光环
  if (isSelected) {
    ctx.strokeStyle = '#0ea5e9';
    ctx.lineWidth = 2;
    ctx.globalAlpha = 0.6 + 0.3 * Math.sin(timestamp * 0.005);
    ctx.beginPath();
    ctx.ellipse(cx, pos.y + TILE_SIZE - 4, 14, 6, 0, 0, Math.PI * 2);
    ctx.stroke();
    ctx.globalAlpha = 1;
  }

  // 阴影
  ctx.fillStyle = 'rgba(0,0,0,0.25)';
  ctx.beginPath();
  ctx.ellipse(cx, pos.y + TILE_SIZE - 3, 12, 4, 0, 0, Math.PI * 2);
  ctx.fill();

  // 实体渲染分支
  if (key === 'player') {
    renderPlayerEntity(cx, cy, colors, facing, timestamp, breathe);
  } else if (key === 'tree' || key === 'pine_tree' || key === 'oak_tree') {
    renderTreeEntity(cx, cy, colors, timestamp, breathe);
  } else if (key === 'rock' || key === 'boulder' || key === 'stone') {
    renderRockEntity(cx, cy, colors, timestamp);
  } else if (key === 'wolf' || key === 'bear' || key === 'animal') {
    renderAnimalEntity(cx, cy, colors, facing, timestamp, breathe, key);
  } else if (key === 'berry_bush') {
    renderBerryBush(cx, cy, colors, timestamp, breathe);
  } else if (key === 'mushroom') {
    renderMushroom(cx, cy, colors);
  } else if (key === 'coin' || key === 'gold') {
    renderCoin(cx, cy, colors, timestamp);
  } else if (key === 'herb') {
    renderHerb(cx, cy, colors, timestamp, breathe);
  } else if (key === 'campfire' || key === 'fire') {
    renderCampfire(cx, cy, colors, timestamp);
  } else if (key === 'npc' || key === 'merchant') {
    renderNPCEntity(cx, cy, colors, facing, timestamp, breathe);
  } else if (key === 'portal') {
    renderPortal(cx, cy, colors, timestamp);
  } else {
    // 通用实体：彩色方块 + 首字母
    renderGenericEntity(cx, cy, colors, entity, timestamp);
  }

  ctx.restore();

  // 浮动文本
  renderFloatingTexts(timestamp);
}

function renderPlayerEntity(cx, cy, colors, facing, timestamp, breathe) {
  const bob = Math.sin(timestamp * 0.008) * 1;
  const yOff = bob;

  // 身体
  ctx.fillStyle = colors.body;
  ctx.fillRect(cx - 8, cy - 6 + yOff, 16, 14);

  // 头部
  ctx.fillStyle = colors.head;
  ctx.fillRect(cx - 6, cy - 14 + yOff, 12, 8);

  // 眼睛
  ctx.fillStyle = '#1e293b';
  let eyeOffX = 0;
  if (facing === 'right') eyeOffX = 2;
  else if (facing === 'left') eyeOffX = -2;
  ctx.fillRect(cx - 3 + eyeOffX, cy - 11 + yOff, 2, 2);
  ctx.fillRect(cx + 1 + eyeOffX, cy - 11 + yOff, 2, 2);

  // 腿
  ctx.fillStyle = colors.legs;
  const legSwing = Math.sin(timestamp * 0.012) * 2;
  ctx.fillRect(cx - 7, cy + 8 + yOff + legSwing, 5, 6);
  ctx.fillRect(cx + 2, cy + 8 + yOff - legSwing, 5, 6);

  // 披风
  ctx.fillStyle = colors.accent;
  ctx.fillRect(cx - 10, cy - 4 + yOff, 3, 10);
}

function renderTreeEntity(cx, cy, colors, timestamp, breathe) {
  const yOff = breathe;

  // 树干
  ctx.fillStyle = colors.trunk;
  ctx.fillRect(cx - 4, cy - 2, 8, 18);

  // 树冠 - 多层圆形模拟像素树
  ctx.fillStyle = colors.leaf1;
  ctx.fillRect(cx - 14, cy - 18 + yOff, 28, 20);
  ctx.fillRect(cx - 10, cy - 26 + yOff, 20, 10);

  ctx.fillStyle = colors.leaf2;
  ctx.fillRect(cx - 10, cy - 14 + yOff, 20, 14);
  ctx.fillRect(cx - 6, cy - 22 + yOff, 12, 8);

  ctx.fillStyle = colors.leaf3;
  ctx.fillRect(cx - 4, cy - 18 + yOff, 8, 8);
}

function renderRockEntity(cx, cy, colors, timestamp) {
  ctx.fillStyle = colors.body;
  ctx.fillRect(cx - 10, cy - 4, 20, 14);
  ctx.fillRect(cx - 6, cy - 10, 12, 8);
  ctx.fillRect(cx - 2, cy - 14, 6, 6);

  ctx.fillStyle = colors.highlight;
  ctx.fillRect(cx - 8, cy - 6, 4, 3);
  ctx.fillRect(cx - 4, cy - 12, 3, 2);
}

function renderAnimalEntity(cx, cy, colors, facing, timestamp, breathe, key) {
  const yOff = breathe;
  const isAggressive = key === 'wolf' || key === 'bear';
  const bodyW = isAggressive ? 18 : 14;

  // 身体
  ctx.fillStyle = colors.body;
  ctx.fillRect(cx - bodyW / 2, cy - 6 + yOff, bodyW, 12);

  // 头部
  ctx.fillStyle = colors.head;
  let headOffX = 0;
  if (facing === 'right') headOffX = 6;
  else if (facing === 'left') headOffX = -6;
  ctx.fillRect(cx - 5 + headOffX, cy - 12 + yOff, 10, 8);

  // 耳朵
  ctx.fillStyle = colors.body;
  ctx.fillRect(cx - 6 + headOffX, cy - 16 + yOff, 3, 4);
  ctx.fillRect(cx + 3 + headOffX, cy - 16 + yOff, 3, 4);

  // 眼睛 (红色表示攻击性)
  ctx.fillStyle = colors.accent;
  let eyeOffX = 0;
  if (facing === 'right') eyeOffX = 1;
  else if (facing === 'left') eyeOffX = -1;
  ctx.fillRect(cx - 3 + headOffX + eyeOffX, cy - 10 + yOff, 2, 2);
  ctx.fillRect(cx + 1 + headOffX + eyeOffX, cy - 10 + yOff, 2, 2);

  // 腿
  ctx.fillStyle = colors.legs;
  ctx.fillRect(cx - 6, cy + 6 + yOff, 3, 5);
  ctx.fillRect(cx + 3, cy + 6 + yOff, 3, 5);
}

function renderBerryBush(cx, cy, colors, timestamp, breathe) {
  const yOff = breathe;
  ctx.fillStyle = colors.body;
  ctx.fillRect(cx - 10, cy - 8 + yOff, 20, 18);
  ctx.fillRect(cx - 6, cy - 14 + yOff, 12, 8);

  // 浆果
  ctx.fillStyle = colors.berry;
  ctx.fillRect(cx - 6, cy - 8 + yOff, 3, 3);
  ctx.fillRect(cx + 4, cy - 6 + yOff, 3, 3);
  ctx.fillRect(cx - 2, cy - 2 + yOff, 3, 3);
}

function renderMushroom(cx, cy, colors) {
  ctx.fillStyle = colors.stem;
  ctx.fillRect(cx - 2, cy - 4, 4, 10);

  ctx.fillStyle = colors.cap;
  ctx.fillRect(cx - 7, cy - 12, 14, 10);
  ctx.fillRect(cx - 5, cy - 16, 10, 6);

  ctx.fillStyle = colors.spot;
  ctx.fillRect(cx - 4, cy - 10, 2, 2);
  ctx.fillRect(cx + 2, cy - 14, 2, 2);
}

function renderCoin(cx, cy, colors, timestamp) {
  const spin = Math.sin(timestamp * 0.005);
  const scaleX = Math.abs(spin) * 0.7 + 0.3;
  ctx.save();
  ctx.translate(cx, cy);
  ctx.scale(scaleX, 1);

  ctx.fillStyle = colors.edge;
  ctx.beginPath();
  ctx.arc(0, 0, 7, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = colors.face;
  ctx.beginPath();
  ctx.arc(0, 0, 5, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = colors.shine;
  ctx.fillRect(-1, -3, 2, 6);

  ctx.restore();
}

function renderHerb(cx, cy, colors, timestamp, breathe) {
  const yOff = breathe;
  ctx.fillStyle = colors.stem;
  ctx.fillRect(cx - 1, cy - 4, 2, 12);

  ctx.fillStyle = colors.leaf;
  ctx.fillRect(cx - 5, cy - 8 + yOff, 4, 3);
  ctx.fillRect(cx + 1, cy - 6 + yOff, 4, 3);

  ctx.fillStyle = colors.flower;
  ctx.fillRect(cx - 2, cy - 12 + yOff, 4, 4);
  ctx.fillRect(cx - 3, cy - 11 + yOff, 6, 2);
  ctx.fillRect(cx - 1, cy - 13 + yOff, 2, 6);
}

function renderCampfire(cx, cy, colors, timestamp) {
  // 木头
  ctx.fillStyle = colors.wood;
  ctx.fillRect(cx - 10, cy + 6, 8, 4);
  ctx.fillRect(cx + 2, cy + 6, 8, 4);
  ctx.fillRect(cx - 6, cy + 4, 12, 4);

  // 火焰动画
  const f1 = Math.sin(timestamp * 0.008) * 2;
  const f2 = Math.cos(timestamp * 0.012) * 1.5;
  const f3 = Math.sin(timestamp * 0.006) * 1;

  ctx.fillStyle = colors.flame1;
  ctx.fillRect(cx - 4, cy - 4 + f1, 8, 8);

  ctx.fillStyle = colors.flame2;
  ctx.fillRect(cx - 3, cy - 2 + f2, 6, 6);

  ctx.fillStyle = colors.flame3;
  ctx.fillRect(cx - 2, cy + f3, 4, 4);
}

function renderNPCEntity(cx, cy, colors, facing, timestamp, breathe) {
  const bob = Math.sin(timestamp * 0.006) * 0.5;
  const yOff = breathe + bob;

  ctx.fillStyle = colors.body;
  ctx.fillRect(cx - 7, cy - 6 + yOff, 14, 13);

  ctx.fillStyle = colors.head;
  ctx.fillRect(cx - 5, cy - 13 + yOff, 10, 7);

  ctx.fillStyle = '#1e293b';
  let eyeOffX = 0;
  if (facing === 'right') eyeOffX = 1;
  else if (facing === 'left') eyeOffX = -1;
  ctx.fillRect(cx - 2 + eyeOffX, cy - 11 + yOff, 2, 2);
  ctx.fillRect(cx + 1 + eyeOffX, cy - 11 + yOff, 2, 2);

  ctx.fillStyle = colors.legs;
  ctx.fillRect(cx - 6, cy + 7 + yOff, 4, 5);
  ctx.fillRect(cx + 2, cy + 7 + yOff, 4, 5);

  // NPC 标记
  ctx.fillStyle = colors.accent;
  ctx.fillRect(cx - 2, cy - 18 + yOff, 4, 3);
}

function renderPortal(cx, cy, colors, timestamp) {
  const pulse = Math.sin(timestamp * 0.004) * 0.3 + 0.7;

  ctx.fillStyle = colors.outer;
  ctx.globalAlpha = pulse;
  ctx.beginPath();
  ctx.ellipse(cx, cy, 14, 8, 0, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = colors.inner;
  ctx.globalAlpha = pulse * 0.8;
  ctx.beginPath();
  ctx.ellipse(cx, cy, 10, 6, 0, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = colors.glow;
  ctx.globalAlpha = pulse * 0.5;
  ctx.beginPath();
  ctx.ellipse(cx, cy, 6, 4, 0, 0, Math.PI * 2);
  ctx.fill();

  ctx.globalAlpha = 1;
}

function renderGenericEntity(cx, cy, colors, entity, timestamp) {
  const pulse = Math.sin(timestamp * 0.003) * 0.1 + 1;
  const displayName = entity.display_name || entity.entity_key || '?';
  const firstChar = displayName.charAt(0).toUpperCase();

  // 主体方块
  ctx.fillStyle = colors.body || '#9ca3af';
  const size = 10;
  ctx.fillRect(cx - size, cy - size, size * 2, size * 2);

  // 首字母
  ctx.fillStyle = '#ffffff';
  ctx.font = 'bold 10px monospace';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(firstChar, cx, cy);
}

// 浮动文字
function addFloatingText(x, y, text, color) {
  state.floatingTexts.push({
    x, y, text, color,
    startTime: performance.now(),
    life: 2000,
  });
}

function renderFloatingTexts(timestamp) {
  state.floatingTexts = state.floatingTexts.filter(ft => {
    const elapsed = timestamp - ft.startTime;
    if (elapsed > ft.life) return false;

    const progress = elapsed / ft.life;
    const pos = worldToScreen(ft.x, ft.y);
    const yOff = -30 * progress;
    const alpha = 1 - progress;

    ctx.save();
    ctx.globalAlpha = alpha;
    ctx.fillStyle = ft.color || '#ffffff';
    ctx.font = 'bold 11px "Courier New", monospace';
    ctx.textAlign = 'center';
    ctx.shadowColor = 'rgba(0,0,0,0.7)';
    ctx.shadowBlur = 2;
    ctx.fillText(ft.text, pos.x + TILE_SIZE / 2, pos.y + yOff);
    ctx.restore();

    return true;
  });
}

// ============================================
// 7. 主渲染循环
// ============================================
function gameLoop(timestamp) {
  state.lastTimestamp = timestamp;

  // 清空画布
  ctx.fillStyle = '#0f172a';
  ctx.fillRect(0, 0, canvasW, canvasH);

  const proj = state.projection;
  if (!proj) {
    // 空 projection 提示
    ctx.fillStyle = '#475569';
    ctx.font = '13px "Courier New", monospace';
    ctx.textAlign = 'center';
    ctx.fillText('World projection is empty. Waiting for server data...', canvasW / 2, canvasH / 2);
    state.animFrame = requestAnimationFrame(gameLoop);
    return;
  }

  // 相机跟随玩家
  const playerEntity = safeArray(proj.visible_entities).find(e => e.entity_key === 'player' || e.object_key === 'actor');
  if (playerEntity) {
    state.camera.targetX = playerEntity.x * TILE_SIZE;
    state.camera.targetY = playerEntity.y * TILE_SIZE;
  }
  state.camera.x += (state.camera.targetX - state.camera.x) * CAMERA_LERP;
  state.camera.y += (state.camera.targetY - state.camera.y) * CAMERA_LERP;

  // 屏幕震动
  ctx.save();
  if (state.screenShake > 0) {
    const sx = (Math.random() - 0.5) * state.screenShake;
    const sy = (Math.random() - 0.5) * state.screenShake;
    ctx.translate(sx, sy);
    state.screenShake *= 0.85;
    if (state.screenShake < 0.5) state.screenShake = 0;
  }

  // 渲染地形
  const cells = safeArray(proj.visible_cells);
  for (const cell of cells) {
    renderTile(safeNumber(cell.x), safeNumber(cell.y), cell.terrain_key || 'dark', timestamp);
  }

  // 渲染实体 (按 Y 坐标排序实现深度遮挡)
  const entities = safeArray(proj.visible_entities);
  const sorted = [...entities].sort((a, b) => safeNumber(a.y) - safeNumber(b.y));
  for (const entity of sorted) {
    renderEntity(entity, timestamp);
  }

  ctx.restore();

  // 渲染小地图
  renderMinimap(proj, timestamp);

  state.animFrame = requestAnimationFrame(gameLoop);
}

function renderMinimap(proj, timestamp) {
  if (window.innerWidth < 768) return;

  minimapCtx.fillStyle = 'rgba(15, 23, 42, 0.9)';
  minimapCtx.fillRect(0, 0, 150, 150);

  if (!proj) return;

  const cells = safeArray(proj.visible_cells);
  for (const cell of cells) {
    const mx = 10 + cell.x * 8;
    const my = 10 + cell.y * 8;
    const colors = getTerrainColor(cell.terrain_key || 'dark');
    minimapCtx.fillStyle = colors.base;
    minimapCtx.fillRect(mx, my, 8, 8);
  }

  // 玩家位置
  const player = safeArray(proj.visible_entities).find(e => e.entity_key === 'player');
  if (player) {
    minimapCtx.fillStyle = '#ef4444';
    minimapCtx.fillRect(10 + player.x * 8 + 2, 10 + player.y * 8 + 2, 4, 4);
  }
}

// ============================================
// 8. UI 更新
// ============================================
function updateStatusPanel() {
  document.getElementById('sessionId').textContent = state.sessionId ? state.sessionId.slice(0, 16) + '...' : '-';
  document.getElementById('actorKey').textContent = state.projection?.actor_key || '-';
  document.getElementById('projVersion').textContent = state.projectionVersion || '-';

  const dot = document.getElementById('syncDot');
  const text = document.getElementById('syncText');
  const badge = document.getElementById('mockBadge');

  dot.className = 'sync-dot';
  if (state.mockMode) {
    badge.style.display = 'block';
  } else {
    badge.style.display = 'none';
  }

  switch (state.syncState) {
    case 'syncing':
      dot.classList.add('syncing');
      text.textContent = 'Syncing';
      break;
    case 'synced':
      dot.classList.add('synced');
      text.textContent = 'Synced';
      break;
    case 'error':
      dot.classList.add('error');
      text.textContent = 'Error';
      break;
    case 'refresh_needed':
      dot.classList.add('refresh-needed');
      text.textContent = 'Refresh Needed';
      break;
    default:
      dot.classList.add('syncing');
      text.textContent = state.syncState;
  }
}

function updateCommandPanel() {
  const list = document.getElementById('commandList');
  list.innerHTML = '';

  const commands = safeArray(state.availableCommands);
  if (commands.length === 0) {
    list.innerHTML = '<div class="command-btn-empty">No actions available</div>';
    return;
  }

  for (const cmd of commands) {
    const btn = document.createElement('button');
    btn.className = 'command-btn';
    btn.disabled = !safeBool(cmd.enabled) || state.pendingCommand;
    if (state.pendingCommand) {
      btn.classList.add('loading');
    }

    const label = document.createElement('span');
    label.className = 'cmd-label';
    label.textContent = safeString(cmd.label_text);
    btn.appendChild(label);

    const reason = safeString(cmd.disabled_reason_text);
    if (reason && !cmd.enabled) {
      const reasonEl = document.createElement('span');
      reasonEl.className = 'cmd-reason';
      reasonEl.textContent = reason;
      btn.appendChild(reasonEl);
    }

    if (cmd.enabled && !state.pendingCommand) {
      btn.addEventListener('click', () => handleCommandClick(cmd.option_id));
    }

    list.appendChild(btn);
  }
}

function updateEventLog() {
  const list = document.getElementById('eventList');

  const events = safeArray(state.eventFeed);
  const hints = safeArray(state.frontendHints);
  const warnings = safeArray(state.warningKeys);

  const allItems = [];

  for (const ev of events) {
    const msg = ev.message || ev.event_kind || JSON.stringify(ev);
    allItems.push({ type: 'info', text: msg, time: ev.timestamp || nowTimestamp() });
  }

  for (const hint of hints) {
    allItems.push({ type: 'hint', text: hint, time: nowTimestamp() });
  }

  for (const w of warnings) {
    allItems.push({ type: 'warning', text: w, time: nowTimestamp() });
  }

  if (allItems.length === 0) {
    list.innerHTML = '<div class="event-empty">No events yet</div>';
    return;
  }

  list.innerHTML = '';
  const isMobile = window.innerWidth < 768;
  const displayItems = isMobile ? allItems.slice(-3) : allItems.slice(-20);

  for (const item of displayItems) {
    const div = document.createElement('div');
    div.className = `event-item event-${item.type}`;

    const ts = document.createElement('span');
    ts.className = 'event-timestamp';
    ts.textContent = `[${item.time}]`;

    div.appendChild(ts);
    div.appendChild(document.createTextNode(' ' + item.text));
    list.appendChild(div);
  }

  // 自动滚动到底部
  list.scrollTop = list.scrollHeight;
}

function updateDebugPanel() {
  const content = document.getElementById('debugContent');
  const activeTab = document.querySelector('.debug-tab.active');
  const tab = activeTab ? activeTab.dataset.tab : 'bootstrap';

  const data = state.debugResponses[tab];
  if (data) {
    content.textContent = JSON.stringify(data, null, 2);
  } else {
    content.textContent = `No ${tab} response yet.`;
  }
}

function showToast(message, type) {
  const container = document.getElementById('toastContainer');
  const toast = document.createElement('div');
  toast.className = `toast toast-${type || 'info'}`;
  toast.textContent = message;
  container.appendChild(toast);
  setTimeout(() => {
    if (toast.parentNode) toast.parentNode.removeChild(toast);
  }, 3000);
}

// ============================================
// 9. 交互处理
// ============================================
async function handleCommandClick(optionId) {
  if (state.pendingCommand) return;
  state.pendingCommand = true;
  updateCommandPanel();

  try {
    let response;
    if (state.mockMode) {
      await delay(300);
      response = getMockCommandResponse(optionId);
    } else {
      state.syncState = 'syncing';
      updateStatusPanel();
      response = await submitOption(optionId);
    }

    await processCommandResponse(response);
  } catch (err) {
    console.error('Command failed:', err);
    showToast('Command failed: ' + (err.message || 'Network error'), 'error');
    state.screenShake = 8;
    state.syncState = 'error';
  } finally {
    state.pendingCommand = false;
    updateCommandPanel();
    updateStatusPanel();
  }
}

async function processCommandResponse(response) {
  state.lastResponse = response;

  // 检查是否需要全量刷新
  if (safeBool(response.requires_full_refresh)) {
    showToast('Sync required, refreshing...', 'info');
    await doRefresh();
    return;
  }

  // 检查 patch 版本
  const patch = response.projection_patch;
  if (patch) {
    if (safeNumber(patch.base_projection_version) !== state.projectionVersion) {
      showToast('Version mismatch, refreshing...', 'warning');
      await doRefresh();
      return;
    }
    applyPatch(patch);
    state.projectionVersion = safeNumber(patch.new_projection_version);
  } else {
    // 没有 patch，全量刷新
    await doRefresh();
    return;
  }

  // 更新 commands
  state.availableCommands = safeArray(response.available_commands);

  // 追加事件
  appendEvents(safeArray(response.event_feed), safeArray(response.frontend_hints), safeArray(response.warning_keys));

  // 处理 result
  if (response.result) {
    const result = response.result;
    if (result.result_kind === 'succeeded') {
      showToast('Success!', 'success');
    }
    if (result.result_kind === 'failed') {
      const reasons = safeArray(result.failure_reason_keys);
      showToast('Failed: ' + (reasons.join(', ') || 'Unknown error'), 'error');
      state.screenShake = 6;
    }
    if (safeArray(result.warning_keys).length > 0) {
      showToast('Warning: ' + result.warning_keys.join(', '), 'warning');
    }
  }

  state.syncState = 'synced';
  updateUI();
}

function appendEvents(events, hints, warnings) {
  for (const ev of events) {
    state.eventFeed.push(ev);
  }
  for (const hint of hints) {
    state.frontendHints.push(hint);
  }
  for (const w of warnings) {
    state.warningKeys.push(w);
  }

  // 限制长度
  if (state.eventFeed.length > 100) state.eventFeed = state.eventFeed.slice(-50);
  if (state.frontendHints.length > 50) state.frontendHints = state.frontendHints.slice(-25);
  if (state.warningKeys.length > 50) state.warningKeys = state.warningKeys.slice(-25);
}

// ============================================
// 10. Patch 应用
// ============================================
function applyPatch(patch) {
  if (!state.projection || !patch) return;

  const proj = state.projection;

  // changed_cells
  const changedCells = safeArray(patch.changed_cells);
  for (const change of changedCells) {
    const op = change.op || 'replace';
    if (op === 'clear') {
      proj.visible_cells = [];
    } else if (op === 'remove') {
      const id = change.cell_id || (change.data && change.data.cell_id);
      if (id) {
        proj.visible_cells = safeArray(proj.visible_cells).filter(c => c.cell_id !== id);
      }
    } else {
      // add / update / replace
      const data = change.data || change;
      const id = data.cell_id;
      if (id) {
        const idx = safeArray(proj.visible_cells).findIndex(c => c.cell_id === id);
        if (idx >= 0) {
          proj.visible_cells[idx] = { ...proj.visible_cells[idx], ...data };
        } else {
          proj.visible_cells.push(data);
        }
      }
    }
  }

  // changed_entities
  const changedEntities = safeArray(patch.changed_entities);
  for (const change of changedEntities) {
    const op = change.op || 'replace';
    if (op === 'clear') {
      proj.visible_entities = [];
    } else if (op === 'remove') {
      const id = change.entity_id || (change.data && change.data.entity_id);
      if (id) {
        proj.visible_entities = safeArray(proj.visible_entities).filter(e => e.entity_id !== id);
      }
    } else {
      const data = change.data || change;
      const id = data.entity_id;
      if (id) {
        const idx = safeArray(proj.visible_entities).findIndex(e => e.entity_id === id);
        if (idx >= 0) {
          proj.visible_entities[idx] = { ...proj.visible_entities[idx], ...data };
        } else {
          proj.visible_entities.push(data);
        }
      }
    }
  }

  // changed_inventories
  const changedInvs = safeArray(patch.changed_inventories);
  for (const change of changedInvs) {
    const op = change.op || 'replace';
    const id = change.inventory_id || (change.data && change.data.inventory_id);
    if (op === 'clear') {
      proj.inventories = [];
    } else if (op === 'remove' && id) {
      proj.inventories = safeArray(proj.inventories).filter(i => i.inventory_id !== id);
    } else if (id) {
      const idx = safeArray(proj.inventories).findIndex(i => i.inventory_id === id);
      if (idx >= 0) {
        proj.inventories[idx] = { ...proj.inventories[idx], ...(change.data || change) };
      } else {
        proj.inventories.push(change.data || change);
      }
    }
  }

  // changed_knowledge
  const changedKnow = safeArray(patch.changed_knowledge);
  for (const change of changedKnow) {
    const op = change.op || 'replace';
    const key = change.actor_key || (change.data && change.data.actor_key);
    if (op === 'clear') {
      proj.knowledge = [];
    } else if (op === 'remove' && key) {
      proj.knowledge = safeArray(proj.knowledge).filter(k => k.actor_key !== key);
    } else if (key) {
      const idx = safeArray(proj.knowledge).findIndex(k => k.actor_key === key);
      if (idx >= 0) {
        proj.knowledge[idx] = { ...proj.knowledge[idx], ...(change.data || change) };
      } else {
        proj.knowledge.push(change.data || change);
      }
    }
  }

  // changed_area_effects
  const changedAreas = safeArray(patch.changed_area_effects);
  for (const change of changedAreas) {
    const op = change.op || 'replace';
    const id = change.area_effect_id || (change.data && change.data.area_effect_id);
    if (op === 'clear') {
      proj.area_effects = [];
    } else if (op === 'remove' && id) {
      proj.area_effects = safeArray(proj.area_effects).filter(a => a.area_effect_id !== id);
    } else if (id) {
      const idx = safeArray(proj.area_effects).findIndex(a => a.area_effect_id === id);
      if (idx >= 0) {
        proj.area_effects[idx] = { ...proj.area_effects[idx], ...(change.data || change) };
      } else {
        proj.area_effects.push(change.data || change);
      }
    }
  }
}

// ============================================
// 11. Bootstrap / Refresh / Reset
// ============================================
async function doBootstrap() {
  document.getElementById('loadingText').textContent = 'Bootstrapping...';

  try {
    let data;
    if (state.mockMode) {
      await delay(500);
      data = getMockBootstrap();
    } else {
      data = await bootstrap();
    }

    // 保存 session
    if (data.session_id) {
      state.sessionId = data.session_id;
      localStorage.setItem('pathfinder_session_id', data.session_id);
    }

    // 处理 full_projection
    if (data.full_projection) {
      state.projection = data.full_projection;
      state.projectionVersion = safeNumber(data.projection_version || data.full_projection.projection_version);
    }

    // commands
    state.availableCommands = safeArray(data.available_commands);

    // events
    state.eventFeed = safeArray(data.event_feed);
    state.frontendHints = safeArray(data.frontend_hints);
    state.warningKeys = safeArray(data.warning_keys);

    state.syncState = 'synced';
    state.debugResponses.bootstrap = data;

    hideLoading();
    updateUI();
    showToast(state.mockMode ? 'Mock Mode Active' : 'Connected!', 'success');
  } catch (err) {
    console.error('Bootstrap failed:', err);

    if (err.status === 404 || err.message && err.message.includes('fetch')) {
      // 自动进入 Mock 模式
      state.mockMode = true;
      showToast('Backend not found, entering Mock Mode', 'warning');
      await doBootstrap();
    } else {
      document.getElementById('loadingText').textContent = 'Connection failed. Retrying...';
      setTimeout(doBootstrap, 3000);
    }
  }
}

async function doRefresh() {
  try {
    state.syncState = 'syncing';
    updateStatusPanel();

    let data;
    if (state.mockMode) {
      await delay(300);
      data = getMockBootstrap();
    } else {
      data = await refresh();
    }

    if (data.full_projection) {
      state.projection = data.full_projection;
      state.projectionVersion = safeNumber(data.projection_version || data.full_projection.projection_version);
    }

    state.availableCommands = safeArray(data.available_commands);
    appendEvents(safeArray(data.event_feed), safeArray(data.frontend_hints), safeArray(data.warning_keys));

    state.syncState = 'synced';
    state.debugResponses.refresh = data;
    updateUI();
  } catch (err) {
    console.error('Refresh failed:', err);
    state.syncState = 'error';
    updateStatusPanel();
    showToast('Refresh failed: ' + (err.message || 'Network error'), 'error');
  }
}

async function doReset() {
  if (!confirm('Reset the world? This will clear your session.')) return;

  try {
    if (state.mockMode) {
      mockPlayerX = 2;
      mockPlayerY = 2;
      showToast('World reset (Mock)', 'success');
      await doBootstrap();
    } else {
      const data = await resetWorld();
      if (data.bootstrap) {
        state.sessionId = data.session_id || state.sessionId;
        state.projection = data.bootstrap.full_projection;
        state.projectionVersion = safeNumber(data.bootstrap.projection_version);
        state.availableCommands = safeArray(data.bootstrap.available_commands);
        state.eventFeed = safeArray(data.bootstrap.event_feed);
        state.frontendHints = safeArray(data.bootstrap.frontend_hints);
        state.warningKeys = safeArray(data.bootstrap.warning_keys);
        state.debugResponses.bootstrap = data.bootstrap;
        updateUI();
        showToast('World has been reset!', 'success');
      } else {
        // 没有 bootstrap 数据，重新 bootstrap
        await doBootstrap();
      }
    }
  } catch (err) {
    console.error('Reset failed:', err);
    showToast('Reset failed: ' + (err.message || 'Error'), 'error');
  }
}

// ============================================
// 12. UI 统一更新
// ============================================
function updateUI() {
  updateStatusPanel();
  updateCommandPanel();
  updateEventLog();
  updateDebugPanel();
  updateQuestPanel();
}

function updateQuestPanel() {
  const questContent = document.getElementById('questContent');
  const proj = state.projection;

  if (!proj) {
    questContent.textContent = 'Connecting...';
    return;
  }

  const summaries = safeArray(proj.safe_summary_keys);
  if (summaries.length > 0) {
    questContent.textContent = summaries.join(', ');
  } else if (state.mockMode) {
    questContent.textContent = 'Mock: Explore the 7x7 world';
  } else {
    questContent.textContent = 'Explore the world';
  }
}

function hideLoading() {
  document.getElementById('loadingScreen').classList.add('hidden');
  setTimeout(() => {
    const el = document.getElementById('loadingScreen');
    if (el) el.style.display = 'none';
  }, 500);
}

function delay(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// ============================================
// 13. 事件监听
// ============================================
function setupEventListeners() {
  // Canvas 点击 - 实体选择
  canvas.addEventListener('click', (e) => {
    const rect = canvas.getBoundingClientRect();
    const sx = e.clientX - rect.left;
    const sy = e.clientY - rect.top;
    const world = screenToWorld(sx, sy);

    // 查找点击的实体
    const entities = safeArray(state.projection?.visible_entities);
    let clicked = null;
    let minDist = Infinity;

    for (const entity of entities) {
      const ex = safeNumber(entity.x);
      const ey = safeNumber(entity.y);
      const dx = world.x - (ex + 0.5);
      const dy = world.y - (ey + 0.5);
      const dist = Math.sqrt(dx * dx + dy * dy);
      if (dist < 0.8 && dist < minDist) {
        minDist = dist;
        clicked = entity;
      }
    }

    if (clicked) {
      state.selectedEntityId = clicked.entity_id;
      addFloatingText(clicked.x, clicked.y - 0.5, clicked.display_name || clicked.entity_key, '#0ea5e9');
    } else {
      state.selectedEntityId = null;
    }
  });

  // Debug 标签切换
  document.querySelectorAll('.debug-tab').forEach(tab => {
    tab.addEventListener('click', () => {
      document.querySelectorAll('.debug-tab').forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      updateDebugPanel();
    });
  });

  // Debug 面板展开时更新
  document.getElementById('debugPanel').addEventListener('toggle', updateDebugPanel);

  // 重置按钮
  document.getElementById('resetBtn').addEventListener('click', doReset);

  // 键盘快捷键
  document.addEventListener('keydown', (e) => {
    if (e.key === 'r' || e.key === 'R') {
      if (e.ctrlKey || e.metaKey) {
        e.preventDefault();
        doRefresh();
      }
    }
    if (e.key === 'F5') {
      e.preventDefault();
      doRefresh();
    }
  });
}

// ============================================
// 14. 初始化入口
// ============================================
function init() {
  // 检查 Mock 模式
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.get('mock') === '1') {
    state.mockMode = true;
  }

  // 读取或生成 client_id / session_id
  state.clientId = getOrCreateStorage('pathfinder_client_id', () => generateId('web'));
  state.sessionId = getOrCreateStorage('pathfinder_session_id', () => generateId('session'));

  // 初始化 Canvas
  initCanvas();

  // 设置事件监听
  setupEventListeners();

  // 启动渲染循环
  state.animFrame = requestAnimationFrame(gameLoop);

  // 启动
  doBootstrap();
}

// DOM 加载完成后启动
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
