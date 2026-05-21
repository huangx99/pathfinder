(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#1a2d4a'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#2563eb'; x.fillRect(2, 3, 4, 1); x.fillRect(8, 7, 5, 1); x.fillRect(4, 11, 3, 1);
  x.fillStyle = '#1e40af'; x.fillRect(1, 8, 2, 2); x.fillRect(12, 4, 2, 2);
  x.fillStyle = '#3b82f6'; x.fillRect(6, 14, 3, 1); x.fillRect(10, 2, 2, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.water = c;
})();
