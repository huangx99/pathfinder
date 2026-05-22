(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');

  // Bush body
  x.fillStyle = '#14532d';
  x.fillRect(8, 16, 32, 28);
  x.fillRect(12, 8, 24, 10);

  // Highlight
  x.fillStyle = '#22c55e';
  x.fillRect(10, 18, 10, 10);
  x.fillRect(14, 10, 8, 6);

  // Shadow
  x.fillStyle = '#052e16';
  x.fillRect(28, 28, 10, 14);

  // Berries
  x.fillStyle = '#dc2626';
  x.fillRect(12, 20, 4, 4);
  x.fillRect(22, 14, 4, 4);
  x.fillRect(30, 24, 4, 4);
  x.fillRect(16, 32, 4, 4);
  x.fillRect(28, 36, 4, 4);
  x.fillRect(20, 26, 4, 4);

  // Berry shine
  x.fillStyle = '#fca5a5';
  x.fillRect(13, 21, 2, 2);
  x.fillRect(23, 15, 2, 2);
  x.fillRect(31, 25, 2, 2);

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.berry = c;
})();
