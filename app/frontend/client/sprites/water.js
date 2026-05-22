(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#1a2d4a'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#2563eb'; x.fillRect(6, 9, 12, 3); x.fillRect(24, 21, 15, 3); x.fillRect(12, 33, 9, 3);
  x.fillStyle = '#1e40af'; x.fillRect(3, 24, 6, 6); x.fillRect(36, 12, 6, 6);
  x.fillStyle = '#3b82f6'; x.fillRect(18, 42, 9, 3); x.fillRect(30, 6, 6, 3);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.water = c;
})();
