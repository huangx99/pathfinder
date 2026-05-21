(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#3a3220'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#4a3f28'; x.fillRect(1, 1, 14, 14);
  x.fillStyle = '#5c4f35'; x.fillRect(3, 5, 2, 2); x.fillRect(10, 10, 3, 2); x.fillRect(7, 3, 1, 1);
  x.fillStyle = '#6b5c3e'; x.fillRect(12, 6, 2, 1); x.fillRect(4, 12, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.sand = c;
})();
