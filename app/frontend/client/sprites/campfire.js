(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');

  // Ground stones
  x.fillStyle = '#57534e';
  x.fillRect(8, 38, 8, 4);
  x.fillRect(18, 40, 10, 4);
  x.fillRect(32, 38, 8, 4);
  x.fillRect(12, 36, 6, 4);
  x.fillRect(28, 36, 6, 4);

  // Wood logs
  x.fillStyle = '#451a03';
  x.fillRect(14, 32, 20, 6);
  x.fillRect(18, 28, 12, 8);

  // Fire base
  x.fillStyle = '#ea580c';
  x.fillRect(16, 24, 16, 10);

  // Fire mid
  x.fillStyle = '#facc15';
  x.fillRect(18, 18, 12, 10);

  // Fire top
  x.fillStyle = '#fef08a';
  x.fillRect(20, 12, 8, 8);
  x.fillRect(22, 8, 4, 6);

  // Glow dots
  x.fillStyle = '#f97316';
  x.fillRect(14, 22, 2, 2);
  x.fillRect(32, 20, 2, 2);
  x.fillRect(18, 10, 2, 2);

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.campfire = c;
})();
