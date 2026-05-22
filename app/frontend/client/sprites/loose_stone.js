(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');

  x.fillStyle = 'rgba(2, 6, 23, 0.22)';
  x.fillRect(10, 39, 29, 4);
  x.fillRect(15, 42, 19, 2);

  x.fillStyle = '#334155';
  x.fillRect(8, 31, 15, 8);
  x.fillRect(12, 26, 8, 7);
  x.fillRect(21, 29, 18, 11);
  x.fillRect(27, 24, 8, 7);

  x.fillStyle = '#64748b';
  x.fillRect(11, 29, 8, 4);
  x.fillRect(23, 31, 10, 5);
  x.fillRect(28, 26, 5, 3);

  x.fillStyle = '#cbd5e1';
  x.fillRect(13, 28, 4, 2);
  x.fillRect(25, 31, 5, 2);
  x.fillRect(29, 25, 3, 1);

  x.fillStyle = '#1e293b';
  x.fillRect(8, 37, 15, 3);
  x.fillRect(21, 38, 18, 3);
  x.fillRect(35, 32, 4, 6);

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.loose_stone = c;
})();
