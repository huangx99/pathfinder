(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');

  // Trunk
  x.fillStyle = '#3f2e18';
  x.fillRect(20, 28, 8, 18);
  x.fillStyle = '#5c4024';
  x.fillRect(21, 28, 2, 18);

  // Roots
  x.fillStyle = '#3f2e18';
  x.fillRect(16, 42, 6, 3);
  x.fillRect(26, 42, 6, 3);

  // Canopy layers (bottom to top)
  x.fillStyle = '#064e3b';
  x.fillRect(8, 22, 32, 10);
  x.fillRect(10, 14, 28, 10);
  x.fillRect(14, 6, 20, 10);

  // Highlight
  x.fillStyle = '#047857';
  x.fillRect(10, 24, 8, 6);
  x.fillRect(12, 16, 8, 6);
  x.fillRect(16, 8, 6, 6);

  // Deep shadow
  x.fillStyle = '#022c22';
  x.fillRect(28, 24, 8, 6);
  x.fillRect(26, 16, 8, 6);

  // Berries / details
  x.fillStyle = '#dc2626';
  x.fillRect(14, 18, 2, 2);
  x.fillRect(30, 20, 2, 2);
  x.fillRect(22, 10, 2, 2);

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.tree = c;
})();
