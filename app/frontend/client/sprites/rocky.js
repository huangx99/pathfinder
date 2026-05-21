(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#2d3139'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#4c525e'; x.fillRect(1, 1, 14, 14);
  x.fillStyle = '#6b7280'; x.fillRect(3, 4, 5, 4); x.fillRect(9, 10, 4, 3);
  x.fillStyle = '#52525b'; x.fillRect(2, 11, 3, 2); x.fillRect(11, 3, 2, 2);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.rocky = c;
})();
