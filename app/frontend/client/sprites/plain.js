(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#1e3a1e'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#166534'; x.fillRect(2, 2, 2, 2); x.fillRect(10, 8, 2, 2);
  x.fillStyle = '#14532d'; x.fillRect(6, 12, 1, 1); x.fillRect(14, 5, 1, 1);
  x.fillStyle = '#15803d'; x.fillRect(8, 4, 1, 1); x.fillRect(4, 10, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.plain = c;
})();
