(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#0f2e1c'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#14532d'; x.fillRect(6, 6, 6, 6); x.fillRect(30, 18, 6, 6);
  x.fillStyle = '#064e3b'; x.fillRect(18, 30, 3, 3); x.fillRect(39, 9, 3, 3);
  x.fillStyle = '#166534'; x.fillRect(12, 36, 6, 3); x.fillRect(24, 12, 3, 3);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.forest = c;
})();
