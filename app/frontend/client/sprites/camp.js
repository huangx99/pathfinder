(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#451a03'; x.fillRect(0, 0, 16, 16);
  x.fillStyle = '#7c2d12'; x.fillRect(3, 3, 10, 10);
  x.fillStyle = '#9a3412'; x.fillRect(5, 5, 6, 6);
  x.fillStyle = '#c2410c'; x.fillRect(7, 7, 2, 2);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.camp = c;
})();
