(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#2a2a2a'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#404040'; x.fillRect(2, 6, 12, 10);
  x.fillStyle = '#525252'; x.fillRect(4, 4, 8, 4);
  x.fillStyle = '#e5e5e5'; x.fillRect(6, 2, 4, 3); x.fillRect(5, 3, 1, 1); x.fillRect(10, 3, 1, 1);
  x.fillStyle = '#737373'; x.fillRect(3, 10, 3, 2); x.fillRect(10, 12, 2, 2);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.mountain = c;
})();
