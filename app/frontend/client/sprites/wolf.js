(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');

  // Shadow
  x.fillStyle = 'rgba(0,0,0,0.2)';
  x.fillRect(10, 40, 28, 4);

  // Body
  x.fillStyle = '#475569';
  x.fillRect(12, 22, 24, 14);
  x.fillRect(8, 18, 14, 12);

  // Chest highlight
  x.fillStyle = '#94a3b8';
  x.fillRect(10, 20, 8, 10);

  // Legs
  x.fillStyle = '#334155';
  x.fillRect(14, 34, 5, 8);
  x.fillRect(22, 34, 5, 8);
  x.fillRect(29, 34, 5, 8);

  // Head
  x.fillStyle = '#475569';
  x.fillRect(6, 10, 14, 12);
  x.fillRect(4, 12, 6, 8);

  // Snout
  x.fillStyle = '#cbd5e1';
  x.fillRect(2, 14, 6, 6);

  // Nose
  x.fillStyle = '#0f172a';
  x.fillRect(1, 15, 3, 3);

  // Eye
  x.fillStyle = '#fbbf24';
  x.fillRect(8, 14, 3, 3);
  x.fillStyle = '#0f172a';
  x.fillRect(9, 15, 1, 1);

  // Ears
  x.fillStyle = '#334155';
  x.fillRect(6, 6, 4, 6);
  x.fillRect(12, 6, 4, 6);

  // Tail
  x.fillStyle = '#475569';
  x.fillRect(34, 16, 8, 4);
  x.fillRect(40, 12, 4, 6);

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.wolf = c;
})();
