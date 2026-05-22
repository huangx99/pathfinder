(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#2d3139'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#4c525e'; x.fillRect(3, 3, 42, 42);
  x.fillStyle = '#6b7280'; x.fillRect(9, 12, 15, 12); x.fillRect(27, 30, 12, 9);
  x.fillStyle = '#52525b'; x.fillRect(6, 33, 9, 6); x.fillRect(33, 9, 6, 6);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.rocky = c;
})();
