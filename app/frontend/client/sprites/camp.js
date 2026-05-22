(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');
  x.fillStyle = '#451a03'; x.fillRect(0, 0, 48, 48);
  x.fillStyle = '#7c2d12'; x.fillRect(9, 9, 30, 30);
  x.fillStyle = '#9a3412'; x.fillRect(15, 15, 18, 18);
  x.fillStyle = '#c2410c'; x.fillRect(21, 21, 6, 6);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.camp = c;
})();
