(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#0f172a'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#1e293b'; x.fillRect(4, 4, 8, 8);
  x.fillStyle = '#334155'; x.fillRect(6, 6, 4, 4);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.unknown = c;
})();
