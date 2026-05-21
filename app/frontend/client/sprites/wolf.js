(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#4b5563'; x.fillRect(3, 6, 11, 6); x.fillRect(10, 3, 4, 4);
  x.fillStyle = '#1f2937'; x.fillRect(3, 12, 2, 4); x.fillRect(7, 12, 2, 4); x.fillRect(11, 12, 2, 4);
  x.fillStyle = '#dc2626'; x.fillRect(11, 4, 1, 1);
  x.fillStyle = '#374151'; x.fillRect(4, 7, 2, 2); x.fillRect(8, 8, 3, 2);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.wolf = c;
})();
