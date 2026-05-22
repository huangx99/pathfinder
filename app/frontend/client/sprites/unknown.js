(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#0f172a'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#1e293b'; x.fillRect(12, 12, 24, 24);
  x.fillStyle = '#334155'; x.fillRect(18, 18, 12, 12);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.unknown = c;
})();
