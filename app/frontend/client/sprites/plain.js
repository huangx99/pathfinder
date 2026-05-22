(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#1e3a1e'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#166534'; x.fillRect(6, 6, 6, 6); x.fillRect(30, 24, 6, 6);
  x.fillStyle = '#14532d'; x.fillRect(18, 36, 3, 3); x.fillRect(42, 15, 3, 3);
  x.fillStyle = '#15803d'; x.fillRect(24, 12, 3, 3); x.fillRect(12, 30, 3, 3);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.plain = c;
})();
