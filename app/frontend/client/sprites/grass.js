(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#1e3a1e'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#14532d'; x.fillRect(1, 4, 2, 2); x.fillRect(8, 12, 2, 2);
  x.fillStyle = '#064e3b'; x.fillRect(12, 3, 1, 1); x.fillRect(5, 9, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.grass = c;
})();
