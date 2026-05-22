(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#2a2a2a'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#404040'; x.fillRect(6, 18, 36, 30);
  x.fillStyle = '#525252'; x.fillRect(12, 12, 24, 12);
  x.fillStyle = '#e5e5e5'; x.fillRect(18, 6, 12, 9); x.fillRect(15, 9, 3, 3); x.fillRect(30, 9, 3, 3);
  x.fillStyle = '#737373'; x.fillRect(9, 30, 9, 6); x.fillRect(30, 36, 6, 6);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.mountain = c;
})();
