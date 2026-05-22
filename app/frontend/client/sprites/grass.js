(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#1e3a1e'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#14532d'; x.fillRect(3, 12, 6, 6); x.fillRect(24, 36, 6, 6);
  x.fillStyle = '#064e3b'; x.fillRect(36, 9, 3, 3); x.fillRect(15, 27, 3, 3);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.grass = c;
})();
