(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#ffedd5'; x.fillRect(5, 2, 6, 5);
  x.fillStyle = '#78350f'; x.fillRect(5, 1, 6, 2);
  x.fillStyle = '#2563eb'; x.fillRect(4, 7, 8, 6);
  x.fillStyle = '#1e3a8a'; x.fillRect(5, 13, 2, 3); x.fillRect(9, 13, 2, 3);
  x.fillStyle = '#000000'; x.fillRect(6, 3, 1, 1); x.fillRect(9, 3, 1, 1);
  x.fillStyle = '#fca5a5'; x.fillRect(5, 6, 1, 1); x.fillRect(10, 6, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.player = c;
})();
