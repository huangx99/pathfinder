(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#78350f'; x.fillRect(7, 10, 2, 6);
  x.fillStyle = '#064e3b'; x.fillRect(4, 7, 8, 4); x.fillRect(5, 4, 6, 3); x.fillRect(6, 1, 4, 3);
  x.fillStyle = '#0f766e'; x.fillRect(5, 8, 2, 1); x.fillRect(8, 5, 2, 1);
  x.fillStyle = '#047857'; x.fillRect(7, 3, 2, 1); x.fillRect(6, 6, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.tree = c;
})();
