(() => {
  const c = document.createElement('canvas');
  c.width = 16; c.height = 16;
  const x = c.getContext('2d');
  x.fillStyle = '#451a03'; x.fillRect(3, 12, 10, 3);
  x.fillStyle = '#ea580c'; x.fillRect(5, 5, 6, 8);
  x.fillStyle = '#eab308'; x.fillRect(7, 3, 2, 6);
  x.fillStyle = '#fef08a'; x.fillRect(7, 4, 1, 2); x.fillRect(8, 6, 1, 1);
  window.SPRITES = window.SPRITES || {};
  window.SPRITES.campfire = c;
})();
