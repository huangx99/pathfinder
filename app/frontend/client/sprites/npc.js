(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');

  // Shadow
  x.fillStyle = 'rgba(0,0,0,0.2)';
  x.fillRect(12, 42, 24, 4);

  // Robe body
  x.fillStyle = '#7c2d12';
  x.fillRect(14, 18, 20, 22);
  x.fillRect(12, 20, 4, 18);
  x.fillRect(32, 20, 4, 18);

  // Hood
  x.fillStyle = '#9a3412';
  x.fillRect(14, 6, 20, 14);
  x.fillRect(12, 10, 4, 10);
  x.fillRect(32, 10, 4, 10);

  // Face (skin inside hood)
  x.fillStyle = '#fdba74';
  x.fillRect(18, 12, 12, 10);

  // Eyes
  x.fillStyle = '#1e293b';
  x.fillRect(20, 15, 3, 3);
  x.fillRect(27, 15, 3, 3);

  // Belt
  x.fillStyle = '#fbbf24';
  x.fillRect(14, 28, 20, 3);

  // Hands
  x.fillStyle = '#fdba74';
  x.fillRect(10, 24, 4, 4);
  x.fillRect(34, 24, 4, 4);

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.npc = c;
})();
