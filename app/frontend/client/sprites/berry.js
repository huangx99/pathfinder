(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#14532d'; x.fillRect(2, 4, 12, 10); x.fillRect(4, 2, 8, 12);
  x.fillStyle = '#dc2626'; x.fillRect(4, 5, 2, 2); x.fillRect(10, 8, 2, 2); x.fillRect(7, 3, 2, 2);
  x.fillStyle = '#991b1b'; x.fillRect(5, 9, 1, 1); x.fillRect(9, 6, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.berry = c;
})();
