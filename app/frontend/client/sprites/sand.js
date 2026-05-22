(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#3a3220'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#4a3f28'; x.fillRect(3, 3, 42, 42);
  x.fillStyle = '#5c4f35'; x.fillRect(9, 15, 6, 6); x.fillRect(30, 30, 9, 6); x.fillRect(21, 9, 3, 3);
  x.fillStyle = '#6b5c3e'; x.fillRect(36, 18, 6, 3); x.fillRect(12, 36, 3, 3);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.sand = c;
})();
