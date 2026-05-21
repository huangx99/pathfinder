(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#0f2e1c'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#14532d'; x.fillRect(2, 2, 2, 2); x.fillRect(10, 6, 2, 2);
  x.fillStyle = '#064e3b'; x.fillRect(6, 10, 1, 1); x.fillRect(13, 3, 1, 1);
  x.fillStyle = '#166534'; x.fillRect(4, 12, 2, 1); x.fillRect(8, 4, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.forest = c;
})();
