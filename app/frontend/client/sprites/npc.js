(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#fed7aa'; x.fillRect(5, 2, 6, 5);
  x.fillStyle = '#111827'; x.fillRect(5, 1, 6, 2);
  x.fillStyle = '#d97706'; x.fillRect(4, 7, 8, 6);
  x.fillStyle = '#78350f'; x.fillRect(5, 13, 2, 3); x.fillRect(9, 13, 2, 3);
  x.fillStyle = '#000000'; x.fillRect(6, 3, 1, 1); x.fillRect(9, 3, 1, 1);
  x.fillStyle = '#fcd34d'; x.fillRect(5, 6, 1, 1); x.fillRect(10, 6, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.npc = c;
})();
