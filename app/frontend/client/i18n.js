/**
 * Pathfinder 前端翻译模块 (i18n)
 *
 * 设计原则：
 * - 扁平键值，便于检索
 * - 支持按 section 组织（entity / cmd / ui / terrain），自动展平
 * - 预留多语言切换接口（当前默认中文）
 * - 新增翻译只需往字典里加键值对，零代码改动
 */

const I18N = {
  // ---------- 实体名称 ----------
  // entity_key 或 display_name_key → 中文名
  entity: {
    // entity_key 映射
    player: '玩家',
    tree: '松树',
    pine_tree: '松树',
    oak_tree: '橡树',
    wolf: '狼',
    beast: '野兽',
    bear: '熊',
    npc: '村民',
    companion: '同伴',
    merchant: '商人',
    campfire: '营火',
    fire: '火焰',
    rock: '岩石',
    boulder: '巨石',
    stone: '石头',
    berry: '浆果丛',
    berry_bush: '浆果丛',
    red_berry: '红浆果',
    mushroom: '蘑菇',
    herb: '草药',
    coin: '金币',
    wood: '木材',
    torch: '火把',
    loose_stone: '碎石',
    unknown_bush: '未知灌木',
    strange_tracks: '奇怪足迹',
    strange_object: '奇怪物体',
    generic_item: '物品',
    prey_entity_type: '猎物',
    npc_type: '村民',

    // display_name_key 映射 (entity.xxx)
    'entity.player': '玩家',
    'entity.tree': '松树',
    'entity.wolf': '狼',
    'entity.beast': '野兽',
    'entity.npc': '村民',
    'entity.campfire': '营火',
    'entity.rock': '岩石',
    'entity.berry': '浆果丛',
    'entity.berry_bush': '浆果丛',
    'entity.red_berry': '红浆果',
    'entity.mushroom': '蘑菇',
    'entity.herb': '草药',
    'entity.coin': '金币',
    'entity.wood': '木材',
    'entity.torch': '火把',
    'entity.loose_stone': '碎石',
    'entity.unknown_bush': '未知灌木',
    'entity.strange_tracks': '奇怪足迹',
    'entity.strange_object': '奇怪物体',
    'entity.generic_item': '物品',

    // harvest 产出的 display_name_key ({key}_name)
    wood_name: '木材',
    red_berry_name: '红浆果',
    torch_name: '火把',
    stone_name: '石头',

    // 旧 h5 兼容 (直接 display_name)
    'Pine Tree': '松树',
    'Oak Tree': '橡树',
    'Boulder': '巨石',
    Entity: '实体',
  },

  // ---------- 命令名称 ----------
  cmd: {
    wait: '等待',
    move: '移动',
    move_north: '向上',
    move_south: '向下',
    move_east: '向右',
    move_west: '向左',
    gather: '采集',
    chop: '砍伐',
    look: '观察',
    attack: '攻击',
    talk: '交谈',
    trade: '交易',
    craft: '制作',
    build: '建造',
    use: '使用',
    drop: '丢弃',
    pick_up: '拾取',
    rest: '休息',
    explore: '探索',
  },

  // ---------- UI 文本 ----------
  ui: {
    loading: '等待后端数据…',
    empty_world: '世界投影为空',
    empty_world_hint: '后端尚未返回可见格子和实体',
    no_commands: '暂无可用命令',
    move_hint: '移动',
    move_hint_dpad: '使用左下摇杆',
    move_hint_wasd: 'WASD 移动',
    command_submit_failed: '命令提交失败',
    request_pending: '请求处理中，请稍候',
    option_expired: '该选项已过期，正在刷新…',
    version_mismatch: '状态不同步，刷新后再确认',
    exec_doing: '执行',
    exec_success: '执行成功',
    exec_result: '执行结果',
    exec_failed: '执行失败',
    move_input: '移动输入',
    no_move_cmd: '没有可用移动命令',
    full_refresh_required: '后端要求全量刷新',
    patch_mismatch: 'Patch 版本不匹配',
    missing_projection: '本地缺少世界投影，执行全量刷新',
    command_blocked: '命令被阻止',
    bootstrap_ok: 'Bootstrap 成功',
    bootstrap_failed: 'Bootstrap 失败',
    backend_unavailable: '无法连接后端，请确认服务已启动 (端口 1999)',
    backend_offline: '无法连接后端',
    refresh_ok: '刷新成功',
    refresh_failed: '刷新失败',
    world_reset: '世界已重置',
    reset_confirm: '确定要重置世界吗？当前会话将被清除。',
    reset_failed: '重置失败',
    unknown_cmd: '未知命令',
    unknown_entity: '未知实体',
    joystick_init_failed: '移动方向键初始化失败',
    client_boot: '先驱者客户端启动…',
    sync: '同步',
    request: '请求',
    layer: '层',
    cells: '格子',
    entities: '实体',
    inventory: '背包',
    coord: '坐标',
    backend_real: '真实后端',
    mock_mode: 'MOCK 模式',
    dir_up: '向上',
    dir_down: '向下',
    dir_left: '向左',
    dir_right: '向右',
  },

  // ---------- 结果状态 ----------
  result: {
    succeeded: '成功',
    noop: '无变化',
    deferred: '已排队',
    blocked: '被阻止',
    failed: '失败',
    interrupted: '被打断',
    unknown: '未知结果',
  },

  // ---------- 日志前缀 ----------
  log: {
    event: '事件',
    hint: '提示',
    warning: '警告',
  },

  // ---------- 地形名称 ----------
  terrain: {
    grass: '草地',
    forest: '森林',
    water: '水域',
    sand: '沙地',
    mountain: '山地',
    plain: '平原',
    stone_field: '石原',
    water_edge: '水边',
    blocked: '阻塞',
    deep_water: '深水',
    unknown: '未知',
  },
};

// ── 自动展平 ──
// 同时注册裸键和带 section 前缀的键（如 "loading" 和 "ui.loading"）
const DICT = {};
for (const [sectionName, section] of Object.entries(I18N)) {
  for (const [k, v] of Object.entries(section)) {
    DICT[k] = v;                         // 裸键
    DICT[sectionName + '.' + k] = v;     // 命名空间键
  }
}

// ── 核心 API ──

/**
 * 基础翻译函数
 * @param {string} key — 翻译键
 * @param {string} [fallback] — 回退文本，不传则返回 key 本身
 * @returns {string}
 */
function t(key, fallback) {
  if (typeof key !== 'string') return fallback ?? String(key);
  return DICT[key] ?? fallback ?? key;
}

/**
 * 翻译实体名称（综合 entity_key / display_name_key / display_name）
 * @param {Object} fields — 实体字段对象
 * @returns {string}
 */
function tEntity(fields) {
  if (!fields) return t('unknown_entity');

  // 优先级 1：display_name_key（如 "entity.player" / "wood_name"）
  const dnk = fields.display_name_key;
  if (dnk && DICT[dnk]) return DICT[dnk];

  // 优先级 2：display_name（旧版兼容，如 "Pine Tree"）
  const dn = fields.display_name || fields.name;
  if (dn && DICT[dn]) return DICT[dn];

  // 优先级 3：entity_key（如 "tree" / "wolf"）
  const ek = fields.entity_key;
  if (ek && DICT[ek]) return DICT[ek];

  // 优先级 4：object_key
  const ok = fields.object_key;
  if (ok && DICT[ok]) return DICT[ok];

  // 回退：返回原始 display_name 或 entity_key，或 "未知实体"
  return dn || ek || ok || t('unknown_entity');
}

/**
 * 翻译命令标签
 * @param {string} commandKey — 命令键（如 "wait", "move_north"）
 * @param {string} [labelText] — 后端返回的原始 label_text
 * @returns {string}
 */
function tCmd(commandKey, labelText) {
  if (commandKey && DICT[commandKey]) return DICT[commandKey];
  if (labelText && DICT[labelText]) return DICT[labelText];
  return labelText || commandKey || '';
}

// 暴露到全局（供 app.js 及后期扩展使用）
window.I18N = I18N;
window.I18N_DICT = DICT;
window.t = t;
window.tEntity = tEntity;
window.tCmd = tCmd;
